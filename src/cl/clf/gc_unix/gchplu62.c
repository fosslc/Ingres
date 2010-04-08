/*#define ORPHANED uncomment to activate orphaned conversation code*/
/*
**Copyright (c) 2004 Ingres Corporation
*/

#include	<compat.h>
#include	<clconfig.h>
#include        <clpoll.h>
#include    	<systypes.h>
#include	<clsigs.h>
#include	<er.h>
#include	<gcccl.h>
#include	<cm.h>
#include    	<me.h>
#include	<nm.h>
#include    	<st.h>
#include	<gcatrace.h>
#include    	<bsi.h>
#include        <lo.h>

#include        <sys/time.h>
#include	<errno.h>

#ifdef xCL_HP_LU62_EXISTS

#include    	"/usr/include/sna/appc_c.h"
#include    	"/usr/include/sna/acssvcc.h"

/* External variables */

/*
** Forward functions
*/

static VOID GClu62sm();
static VOID GClu62er();
static STATUS GClu62name();
static VOID GClu62read();
static VOID GClu62timer();
static VOID GClu62timeout();
static VOID GClu62posted();
static STATUS GClu62alloc2();
static VOID GClu62getcid();
static i4 GClu62gettime();
static STATUS GClu62getstate();

/*
** local variables
*/
#define DEFAULT_TIMER_S 50                  /* ms: time to prepare to receive 
    	    	    	    	    	       after last operation was send */

#define DEFAULT_TIMER_R 2000
#define MAX_TIMER_R 128000                  /* ms: time to prepare to receive
                                               after last operation was recv */

#define DEFAULT_TIMER_B 4000
static i4  GClu62_timer_b = DEFAULT_TIMER_B;/* ms: time before we do a
    	    	    	    	    	       receive_allocate */

/* Length of Connect message */
#define CONNECT_LENGTH 80

/* Per connection control block */

typedef struct _GC_PCB
{
    struct mc_receive_and_post rec_and_post;/* VCB for receiving data */
    GCC_P_PLIST 	*posted_parm_list;  /* Parameter list associated
					       with posted receive:- note
					       that this could be a listen,
					       connect or receive parameter
					       list depending on function
					       for which a receive has been
					       posted */
    GCC_P_PLIST 	*lsn_parm_list;	    /* Listen parameter list */
    GCC_P_PLIST    	*send_parm_list;    /* Send parameter list */
    GCC_P_PLIST    	*rcv_parm_list;     /* Receive parameter list */
    GCC_P_PLIST 	*disconn_parm_list; /* Disconnect parameter list */
    unsigned char	tp_id[8];           /* TP ID for this conversation */
    unsigned char	tp_id_2[8];         /* TP ID for this conversation */
    unsigned char	plu_alias[8];       /* partner name */ 
    unsigned char	mode_name[8];       /* mode for this conversation */
    bool	    	invoked_tp; 	    /* Invoked TP? */
    i4             start_msec;
    i4                  duplex;             /* 1 or 2 converstions */
    i4  	    	conv_state;         /* State of connection */
    u_long	    	conversation_id;    /* Id of conversation */
    u_long	    	conversation_id_2;  /* Id of conversation */
    i4 	        timeout;            /* Current timeout period */
    bool	    	timer_running;      /* timer running? */
    u_char	    	connect_message[CONNECT_LENGTH];
    	    	    	    	            /* Initial message from peer */
    i4  	    	total_send; 	    /* total number of sends rqstd */
    i4  	    	send_in_conv_rcv;   /* sends in AP_RECEIVE_STATE state*/
    i4  	    	total_rcv;  	    /* total number of rcvs rqstd */
    i4  	    	rcv_in_conv_send;   /* rcvs in AP_SEND_STATE state */
    i4                  atrys;              /* 2nd allocation trys */
#define IDLEN 4
    unsigned char       pkey[IDLEN];
} GC_PCB;

/* Per driver control block */

#define TP_NAME_LEN	64

typedef struct _GC_DCB
{
    	unsigned char	invoked_tp_name[TP_NAME_LEN+1];
    	    	    	    	    	    /* name of listening TP */
    	unsigned char	invoking_tp_name[TP_NAME_LEN+1];
    	    	    	    	            /* name of invoking TP */
      	short	    	number_of_lus;      /* number of lu's started */
  
#define MAX_LUS 16
  
      	unsigned char   lu_alias[MAX_LUS][8];
      	    	    	    		    /* LU alias of invoking TP */
      	unsigned char	tp_id[MAX_LUS][8];
      	    	    	    	            /* id of invoking TP */
    	short	    	conversations[MAX_LUS];
    	    	    	    	    	    /* Number of conv in invoking TP */
    	short	    	total_conversations;
    	    	    	    		    /* total number of convs */
    	short           number_of_timers;   /* number of timers running */

#define MAX_CONVERSATIONS 64

    	GC_PCB          *tim_pcb[MAX_CONVERSATIONS];
    	    	    	    	    	    /* pcb's of timer conversations */
    	i4         tim_expiration[MAX_CONVERSATIONS];
    	    	    	    	    	    /* time to expiration */
    	struct timeval  timtod;             /* snapshot of wallclock time */
    	int 	    	pipe_fd[2];         /* pipe fd for timer use */
} GC_DCB;

static GC_PCB *GClu62newPCB();
static i4  GClu62_trace = 0;
static i4  halfplex = 0;

static GC_PCB *GClu62lpcbs();

#define GCTRACE(n) if (GClu62_trace >= n) (void)TRdisplay

/* Actions in GClu62sm. */

#define	GC_REG		0	/* register as TP */
#define	GC_LSN		1	/* wait for inbound conversation */
#define GC_LSNPOLL      2   	/* poll for inbound conversation */
#define GC_LSNRCV  	3	/* receive peer connect message */
#define GC_LSNCMP   	4   	/* send peer connect message and complete */
#define	GC_CONN  	5	/* connect to host */
#define GC_CONNRCV      6       /* CONNECT completes */
#define GC_CONN_ALLOC2  7       /* CONNECT allocate 2nd session for
                                   pseudo-duplex connection */
#define GC_CONNCMP  	8   	/* connect wait for data token */
#define	GC_SND  	9	/* send TPDU */
#define GC_SNDTKN   	10  	/* send wait for data token */
#define	GC_SNDRDY	11	/* SEND request and SEND state */
#define	GC_RCV		12	/* receive TPDU */
#define	GC_RCVCMP	13	/* RECEIVE completes */
#define	GC_DISCONN	14	/* close a connection */
#define GC_DISCONNCMP	15  	/* CLOSE completes */


/* Action names for tracing. */

static char *gc_names[] = { 
	"REG", "LSN", "LSNPOLL", "LSNRCV", "LSNCMP", "CONN", "CONNRCV",
	"CONN_ALLOC2", "CONNCMP", "SND", "SNDTKN", "SNDRDY", "RCV", "RCVCMP",
        "DISCONN", "DISCONNCMP"
} ;

/* Conversation state names for tracing. */
static char *st_names[] = {
  	"RESET", "SEND", "RCV", "PNDPOST"
} ;

/* initial send buffer */
unsigned char  version_buf[GC_INITIAL_SEND_LENGTH];

/* debugging orphaned pcb; prototypes and storage */
#ifdef ORPHANED

VOID cleart();
VOID storepcb();
VOID clearpcb();

#define TSIZE 100
GC_PCB *pcbt[TSIZE];
i4 st[TSIZE];

#endif


/*{
** Name: GCpdrvr    - Protocol driver routine
**
** Description:
**	GCpdrvr is the model for the protocol driver routines which interface
**	to the local system facilities through which access to specific
**	protocols is offered.  In a given environment there may be multiple
**	protocol drivers, depending on the number of separate protocols which
**	are to be supported and the specific implementation of protocol drivers
**	in the environment.  This is an implementation issue, and is dealt with
**	by appropriate construction of the Protocol Control Table (PCT) by the
**	initialization routine GCpinit().
**
**	Protocol driver routines are not called directly by the CL client, but
**	rather are invoked by pointer reference through the Protocol Control
**	Table.  This is therefore a model specification for all protocol
**	drivers, whose names are not actually known outside the CL.  They are
**	known externally only by an identifier in the PCT.
**	
**	All protocol drivers have an indentical interface, through which a set
**	of service functions is offered to clients.  Two parameters are passed
**	by the caller: a function identifier and a parameter list pointer.  The
**	function ID specifies which of the functions in the repertoire is being
**	requested.  The parm list supplies the parameters required for
**	execution of the function.  It contains both input and output
**	parameters.  It has a constant section, and a section containing
**	parameters specific to the requested function.  The interface for all
**	routines is specified here instead of with each routine, as is
**	customary, since it is identical for all.
**
**	It is the responsibility of a single protocol driver to map the service
**	interface for a particular protocol, as provided by a specific
**	environment, to the model interface specified here.  In particular, this
**	includes the management of asynchronous events in the specified way.
**	The protocol driver interface is asynchronous in the sense that when a
**	function is invoked by the client, control is returned prior to
**	completion of the function.  Notification of completion is provided
**	to the client by means of a callback routine provided by the client as a
**	service request parameter.  This routine is to be invoked when the
**	requested service is complete.
**
**	It is to be noted that execution control is guaranteed to pass to the
**	CL routine GCc_exec, and remain there for the duration of server
**	execution.  It is possible for protocol driver service requests to be
**	invoked prior to the time GCc_exec receives control.  It is not
**	required that these service requests complete until control passes to
**	GCc_exec.  This enables the detection and allowance of external
**	asynchronous events to reside in a place to which control is guaranteed
**	to pass subsequent to a service request, if this is appropriate in a
**	particular environment.  This is not required, and if the environment
**	permits, the entire mechanism of dealing with external asynchronous
**	events may be implemented in the protocol driver.  It is also to be
**	noted that when a CL client's callback routine is invoked, it is
**	guaranteed that control will be returned from that invocation when the
**	client's processing is complete, and that the client will not wait or
**	block on some external event.
**
**      As LU6.2 is a half-duplex protocol (one may only send data when in
**      possession of the data token) and Ingres/Net cannot be guaranteed to
**      be, LU6.2 protocol drivers implement a token-passing protocol.
**      However, the "normal" use of Ingres/Net tends to be half-duplex in
**      nature (a bunch of messages forming a query in one direction are 
**      followed by a bunch of messages forming a query response in the other).
**      The trick is to optimize the passing of the data token for this
**      situation whilst still being able to cope with messages that break
**      this paradigm. The basic algorithm for this token-passing protocol is 
**      as follows:
**
**      1. When a GCC_SEND operation completes, a timer is started. When the
**      timer expires, the data token is passed to one's peer by issuing a
**      prepare-to-receive. If another GCC_SEND operation is issued prior to
**      the timer expiring, the timer is cancelled. This timer is set to
**      1/10sec. The premise is to recognize when the last of a group of
**      messages is being sent in one direction and to give up the data token
**      at that point. The timer setting is chosen so that successive sends
**      may be issued without giving up the data token.

**      2. If the data token is received from one's peer, and there is no
**      GCC_SEND operation outstanding, a timer is started. When the timer
**      expires, the data token is passed back to one's peer. The setting for
**      the timer depends on a number of factors. If the last completed
**      operation was a GCC_RECEIVE, the timer is initially set to 2secs.
**      After this, each successive occasion the data token is received, the
**      timer setting is doubled until a setting of around 2 minutes is 
**  	reached. The premise is that as we received the data token after 
**      receiving data, the most likely scenario is that the next thing that
**      we will be asked to do is a GCC_SEND. If the last completed operation
**      was a GCC_SEND, the timer is set to 0, and the data token is handed
**      straight back to one's peer (in essence, the peer has polled us to see
**      whether we have any data to send).
**
**      If there was a GCC_SEND operation outstanding when we got the data
**      token, that operation is acted upon and when it completes, rule 1. once
**      again applies.
**
** Inputs:
**      Function_code                   Specifies the requested function
**          GCC_OPEN        ||
**	    GCC_CONNECT	    ||
**	    GCC_DISCONNECT  ||
**	    GCC_SEND	    ||
**	    GCC_RECEIVE	    ||
**	    GCC_LISTEN
**      parameter_list                  A pointer to a set of parameters
**					qualifying the requested function.
**					See gcccl.h for description.
**
** Outputs:
**      Contained in the parameter list, described above.
**
**	Returns:
**	    STATUS:
**		OK
**		FAIL
**
** History:
**    26-Jan-93 (cmorris)
**	Copied from gcsunlu62.c.
**    19-Feb-93 (cmorris)
**  	Added description of token-passing protocol.
**    22-Feb-93 (cmorris)
**  	Null out pcb pointer in parameter list when pcb is freed.
**    24-Feb-93 (cmorris)
**  	Cater for posted receive completing after deallocate completes.    
**    15-Apr-93 (cmorris)
**  	Added error handling for character set conversion calls.
**    03-May-93 (cmorris)
**  	Added support to end invoked TP when incoming conversations are
**  	terminated.
**    06-May-93 (cmorris)
**  	Convert TP name to EBCDIC before doing receive allocate.
**    06-May-93 (cmorris)
**  	MEcopy tp_ids rather than STlcopying them:- they're not really 
**  	character strings.
**    07-May-93 (cmorris)
**  	Salt away parameter list associated with a posted receive for
**  	use when the receive gets posted.
**    07-May-93 (cmorris)
**  	Set timer for initial listen poll, not just subsequent ones.
**    07-May-93 (cmorris)
**  	receiveandpost() can drive its completion synchronously, so make
**  	sure that we fiddle with state before the call, not after.
**    28-May-93 (cmorris)
**  	Allow for invoking TP (ie that which makes outgoing allocates) to
**  	use a different TP name from invoked TP (ie that which receives
**  	incoming allocates).
**    03-Jun-93 (cmorris)
**  	Added support for default local LUs.
**    19-Jul-93 (cmorris)
**  	Allow lu alias to be specified as part of connect's node_id
**  	parameter rather than as part of the listen logical. This
**  	allows for multiple local LU's to be used simultaneously.
**    04-Nov-93 (cmorris)
**  	Don't initialize tpstarted after we fill fields in it!!
**    14-Feb-94 (cmorris)
**	Do more complete initialization of pcb.
**    24-Feb-94 (cmorris)
**  	Rejig handling of state when error occurs and there's a receive
**  	posted: keep the conversation in pending post state until the
**  	posted receive completes with an error.
**    25-Feb-94 (cmorris)
**  	When reposting receive to poll for data token, make sure right
**  	parm list is posted and its state changed accordingly!
**    04-Mar-94 (cmorris)
**  	Use pipe to get file descriptor for use with timers.
**    10-Apr-95 (tlong)
**	Possibly fixes Bug #68038 - SEGV at Sainsbury.
**	Summary of changes:
**	Lots more tracing info, conservative index checking (i<=x instead
**	of i==x), initializes used control block instead of unused ones,
**	compresses dcb table properly, no longer intentionally crashes
**	gcc without at least a warning, now uses HP's state definitions, 
**	added mucho casting to get rid of compiler warnings.
**      03-jun-1996 (canor01)
**          New ME for operating-system threads does not need external
**          semaphores. Removed ME_stream_sem.
**    03-Jan-97 (tlong)
**      Added dual (pseudo full-duplex) conversation support.
**      27-apr-1999 (hanch04)
**          replace STindex with STchr
**      28-apr-1999 (hanch04)
**          Replaced STlcopy with STncpy
**	11-Nov-1999 (jenjo02)
**	    Use CL_CLEAR_ERR instead of SETCLERR to clear CL_ERR_DESC.
**	    The latter causes a wasted ___errno function call.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      11-dec-2002 (loera01)  SIR 109237
**          Set option flag in GCC_P_LIST to 0 (remote).
*/


STATUS
GClu62( func_code, parms )
i4		    func_code;
GCC_P_PLIST	    *parms;
{

    /*
    ** Compute initial state based on function code.
    */

    switch( func_code )
    {
    case GCC_OPEN: 	parms->state = GC_REG; break;
    case GCC_LISTEN: 	parms->state = GC_LSN; break;
    case GCC_CONNECT: 	parms->state = GC_CONN; break;
    case GCC_SEND: 	parms->state = GC_SND; break;
    case GCC_RECEIVE: 	parms->state = GC_RCV; break;
    case GCC_DISCONNECT:parms->state = GC_DISCONN; break;
    default:
        GCTRACE(1)("GClu62: Unknown input %x \n", parms->state);
        return FAIL;
        break;
    }

    /*
    ** Client is always remote.
    */ 
    parms->options = 0;

    GCTRACE(1)("GClu62: %x operation %s\n", parms, gc_names[parms->state]);

    parms->generic_status = OK;
    CL_CLEAR_ERR( &parms->system_status );

    /* Start sm. */

    GClu62sm( parms );
    return OK;
}

static VOID
GClu62sm(parms)
GCC_P_PLIST    *parms;
{
    GC_PCB     *pcb = (GC_PCB *)parms->pcb;
    GC_DCB     *dcb = (GC_DCB *)parms->pce->pce_dcb;

top:
    GCTRACE(2)("GClu62sm:top: %x/%x/%x input %s\n",
                        parms, pcb, dcb, gc_names[ parms->state ] );

    switch( parms->state )
    {

    case GC_REG:
    {
    	char   *trace;
        char   *halfplexc;
    	i4 i;

	/*
	** Register before the first listen.
	*/
        
#ifdef ORPHANED
        cleart();
#endif

	NMgtAt( "II_GCCL_TRACE", &trace );
	if( trace && *trace )
	{
	    GClu62_trace  = atoi( trace );
	}
        GCTRACE(1)("II_GCCL_TRACE value %d\n", GClu62_trace);

	NMgtAt( "II_HALF_DUPLEX", &halfplexc);
	if (halfplexc && *halfplexc)
	{
	    halfplex  = atoi(halfplexc);
	}
        GCTRACE(1)("II_HALF_DUPLEX value %d\n", halfplex);

        /* Allocate per driver control block */
	dcb = (GC_DCB *)MEreqmem( 0, sizeof( *dcb ), TRUE, (STATUS *)0 );
	parms->pce->pce_dcb = (PTR)dcb;
        if( !dcb )
    	{
            GCTRACE(1)("Allocate new dcb FAILED!\n");
    	    GClu62er(parms, GC_OPEN_FAIL, (struct appc_hdr *) NULL);
    	    goto complete;
    	}

    	dcb->total_conversations = 0;

    	/* Initialize lu alias and timer lists */
    	dcb->number_of_lus = 0;
    	dcb->number_of_timers = 0;

    	/* Deduce our invoking and invoked transaction program names */
 	GClu62name(parms->pce->pce_pid, parms->function_parms.open.port_id,
                   dcb->invoked_tp_name, dcb->invoking_tp_name);

    	STcopy((char *)dcb->invoked_tp_name,
                       parms->function_parms.open.lsn_port);

    	/* Open pipe to get file descriptor for timers */
	if (pipe(dcb->pipe_fd) < 0)
    	{
    	    GClu62er(parms, GC_OPEN_FAIL, (struct appc_hdr *) NULL);
    	    goto complete;
    	}
 
    	/*
	** Registration completed.
	*/

	/* Note protocol is open and print message to that effect. */
	GCTRACE(3)("GClu62sm: Registered at [%s:%s]\n", 
    	    	   dcb->invoked_tp_name, dcb->invoking_tp_name);
	goto complete;
    } /* end case GC_REG */

    case GC_LSN:
    {
	/*
	** Extend a listen to accept a connection. In LU6.2 terminology
	** this translates into doing a receive for an incoming allocate
    	** request.
	*/

    	/* Make sure we can handle another connection */
    	if (++dcb->total_conversations > MAX_CONVERSATIONS)
	{
    	    dcb->total_conversations--;
            GCTRACE(1)("No more conversations possible(%d)\n",
                          dcb->total_conversations);
    	    GClu62er(parms, GC_LISTEN_FAIL, (struct appc_hdr *) NULL);
	    goto complete;
	}
	GCTRACE(1)("LUs: %d, Active conversations: %d\n",
                    dcb->number_of_lus, dcb->total_conversations);

	/* Allocate pcb */
        if ((pcb = (GC_PCB *)GClu62newPCB(parms)) == 0)
	{
    	    GClu62er(parms, GC_LISTEN_FAIL, (struct appc_hdr *) NULL);
	    goto complete;
	}
	parms->pcb = (PTR)pcb;

    	pcb->lsn_parm_list = parms;
	parms->state = GC_LSNPOLL;

	/* Set timer for listen poll */
    	pcb->timeout = GClu62_timer_b;
    	GClu62timer(dcb, pcb);
	return;
    } /* end case GC_LSN */

    case GC_LSNPOLL:
    {
    	struct receive_allocate receiveallocate;
    	struct convert conv;
    	i4 i;

	/*
	** Try to receive allocate.
	*/
    	MEfill(sizeof(receiveallocate), (u_char) 0, (PTR) &receiveallocate);
    	receiveallocate.opcode = AP_RECEIVE_ALLOCATE;

    	/* Convert tp name into EBCDIC */
    	conv.opcode = SV_CONVERT;
	conv.opext = 0;
	conv.direction = SV_ASCII_TO_EBCDIC;
	conv.char_set = SV_AE;
	conv.len = STlength((char *)dcb->invoked_tp_name);
	conv.source = dcb->invoked_tp_name;
    	conv.target = receiveallocate.tp_name;
    	ACSSVC_C(&conv);
    	if (conv.primary_rc != SV_OK)
	{
    	    /* Start failed; decipher error */
    	    GClu62er(parms, GC_LISTEN_FAIL, (struct appc_hdr *) &conv);
    	    goto complete;
     	}

    	/* Pad the name with EBCDIC blanks */
    	for (i = STlength((char *)dcb->invoked_tp_name) ; i < 64 ; i++)
	    receiveallocate.tp_name[i] = 0x40;

    	APPC_C(&receiveallocate);
	if(receiveallocate.primary_rc != AP_OK)
	{
    	    /* Check to see whether there simply isn't an allocate pending */
    	    if (receiveallocate.secondary_rc == AP_ALLOCATE_NOT_PENDING)
	    {
                GCTRACE(4)("ALLOCATE_NOT_PENDING\n");

		/* Reset timer for next poll */
    	    	pcb->lsn_parm_list = parms;
    	    	GClu62timer(dcb, pcb);
	        return;
	    }
	    else
	    {
		/* Receive allocate failed; decipher error */
	    	GClu62er(parms, GC_LISTEN_FAIL, 
			 (struct appc_hdr *) &receiveallocate);
	    	goto complete;
	    }
	}
	else
	{
            GCTRACE(4)("ALLOCATE OK\n");
            GCTRACE(3)("GClu62: conv id: %d; tp id: %8x\n",
                        receiveallocate.conv_id, receiveallocate.tp_id);

            /* save details of conversation */
    	    pcb->conversation_id = receiveallocate.conv_id;
    	    MEcopy((PTR) receiveallocate.tp_id, 
    	    	   (u_i4) sizeof(receiveallocate.tp_id),
    	    	   (PTR) pcb->tp_id);
            pcb->tp_id[8] = 0;
	    pcb->conv_state = AP_RECEIVE_STATE;
    	    pcb->timeout = DEFAULT_TIMER_R;
    	    pcb->invoked_tp = TRUE;

    	    /* Make sure confirmation wasn't requested */
    	    if (receiveallocate.sync_level != AP_NONE)
	    {
	    	/* Don't support confirmation */
	    	GClu62er(parms, GC_LISTEN_FAIL, 
			 (struct appc_hdr *) &receiveallocate);
	    	goto complete;
	    }

    	    /* Make sure we have a mapped conversation */
    	    if (receiveallocate.conv_type != AP_MAPPED_CONVERSATION)
	    {
	    	GClu62er(parms, GC_LISTEN_FAIL,
			 (struct appc_hdr *) &receiveallocate);
	    	goto complete;
	    }

            /* connection successful, get the time */
            pcb->start_msec = GClu62gettime();

    	    /* Post a receive */
       	    MEfill(sizeof(pcb->rec_and_post), (u_char) 0, 
    	           (PTR) &pcb->rec_and_post);
    	    pcb->rec_and_post.opcode = AP_M_RECEIVE_AND_POST;
	    pcb->rec_and_post.opext = AP_MAPPED_CONVERSATION;
    	    MEcopy((PTR) pcb->tp_id, (u_i4) sizeof(pcb->tp_id),
    	    	   (PTR) pcb->rec_and_post.tp_id);
	    pcb->rec_and_post.conv_id = pcb->conversation_id;
	    pcb->rec_and_post.max_len = CONNECT_LENGTH;
	    pcb->rec_and_post.dptr= (unsigned char *)&pcb->connect_message;
    	    pcb->rec_and_post.callback = GClu62posted;
    	    pcb->posted_parm_list = parms;
            pcb->conv_state = AP_PEND_POST_STATE;
	    parms->state = GC_LSNRCV;
	    APPC_C(&pcb->rec_and_post);
	    if (pcb->rec_and_post.primary_rc != AP_OK)
	    {
    	    	pcb->posted_parm_list = (GCC_P_PLIST *) NULL;
		pcb->conv_state = AP_RECEIVE_STATE;
	    	GClu62er(parms, GC_LISTEN_FAIL,
    	    	    	 (struct appc_hdr *) &pcb->rec_and_post);
	    	goto complete;
	    }

    	    /* Receive successfully posted */
	    return;
	}

    } /* end case GC_LSNPOLL */

    case GC_LSNRCV:
    {
	/* receive failed, indicate listen error */
    	if (pcb->rec_and_post.primary_rc != AP_OK)
	{
            GCTRACE(1)("Receive failed for LSNRCV\n");
	    GClu62er(parms, GC_LISTEN_FAIL,
    	    	     (struct appc_hdr *) &pcb->rec_and_post);
	    goto complete;
	}

	/* Process receive of initial send by peer */
       	pcb->conv_state = AP_RECEIVE_STATE;

    	/* See what was received */
    	switch (pcb->rec_and_post.what_rcvd)
	{
	case AP_DATA_COMPLETE:
        {
            /* check on  version */
            if ( ! halfplex )
            {
                GCTRACE(1)("Received in LSNRCV   DATA COMPLETE \n");
                if (pcb->connect_message[0] == GC_LU62_VERSION_2)
                    pcb->duplex = PEND_DUP;

                /* ATTN from partner? */
                if (pcb->connect_message[0] == GC_ATTN)
                {
                    struct mc_receive_immediate r_i;

                    /* We have to receive the id!
                    ** we can't give up control here */   

                    while  (1)                       
                    {
                        MEfill(sizeof(r_i), (u_char) 0, (PTR) &r_i);
                        r_i.opcode = AP_M_RECEIVE_IMMEDIATE;
                        r_i.opext  = AP_MAPPED_CONVERSATION;
    	                MEcopy((PTR) pcb->tp_id, (u_i4) sizeof(pcb->tp_id),
                               (PTR) r_i.tp_id);
	                r_i.conv_id = pcb->conversation_id;
                        r_i.max_len = sizeof(pcb->connect_message);
                        r_i.dptr    = pcb->connect_message;
                        APPC(&r_i);
                        if (r_i.primary_rc == AP_OK)
                            break;
                        else if (r_i.primary_rc != AP_UNSUCCESSFUL)
                        {
    	                    pcb->posted_parm_list = (GCC_P_PLIST *) NULL;
	   	            GClu62er(parms, GC_LISTEN_FAIL,
                                  (struct appc_hdr *)&r_i);
	    	            goto complete;
                        }

                        sleep (1);
                    }
                    
                    {
                        GC_PCB *spcb;
                        GCTRACE(4)("Receive immediate for pcb id OK\n");
                        MEcopy(pcb->connect_message, 4, &spcb);
                        spcb  = GClu62lpcbs(pcb, spcb);
                        if (spcb == (GC_PCB *)NULL)
                        {
                            GCTRACE(4)("Primary pcb no longer valid ! \n");
                            /* big trouble, main pcb is nolonger */
      	                    pcb->posted_parm_list = (GCC_P_PLIST *) NULL;
  	   	            GClu62er(parms, GC_LISTEN_FAIL,
                                                  (struct appc_hdr *)&r_i);
  	    	            goto complete;
                        }
                        else
                        {
                            pcb = spcb;
                        }
                    }
                   
                    if (pcb->rcv_parm_list)
                    {
                        GCC_P_PLIST *sparms = pcb->rcv_parm_list;
                        pcb->rcv_parm_list = 0;
                        GClu62sm(sparms);
                    }
                }
            }

            /* old half duplex or first send of duplex */
            
   	    /* Post a receive to pick up data token */
    	    MEfill(sizeof(pcb->rec_and_post), (u_char) 0, 
    	           (PTR) &pcb->rec_and_post);
    	    pcb->rec_and_post.opcode = AP_M_RECEIVE_AND_POST;
	    pcb->rec_and_post.opext = AP_MAPPED_CONVERSATION;

    	    MEcopy((PTR) pcb->tp_id, (u_i4) sizeof(pcb->tp_id),
    	           (PTR) pcb->rec_and_post.tp_id);
	    pcb->rec_and_post.conv_id = pcb->conversation_id;

	    pcb->rec_and_post.max_len = 0;
	    pcb->rec_and_post.dptr = (unsigned char *) NULL;
    	    pcb->rec_and_post.callback = GClu62posted;
    	    pcb->posted_parm_list = parms;

            pcb->conv_state = AP_PEND_POST_STATE;
    	    parms->state = GC_LSNCMP;
	    APPC_C(&pcb->rec_and_post);
	    if (pcb->rec_and_post.primary_rc != AP_OK)
	    {
    	        pcb->posted_parm_list = (GCC_P_PLIST *) NULL;
	        pcb->conv_state = AP_RECEIVE_STATE;
	        GClu62er(parms, GC_LISTEN_FAIL,
                         (struct appc_hdr *) &pcb->rec_and_post);
	    	goto complete;
	    }
    
            /* Receive successfully posted */
            return;

	} /* end case AP_DATA_COMPLETE: */

	case AP_DATA_COMPLETE_SEND:
            GCTRACE(1)("Received DATA_COMPLETE_SEND while in LSNRCV\n");
    	case AP_SEND:
    	    pcb->conv_state = AP_SEND_STATE;

    	    /* drop through... */

    	case AP_DATA_INCOMPLETE:

    	/* 
    	** (i) We got the data token before the data!
    	** (ii) We got a larger record than we can cope with.
    	** Treat these as fatal errors by dropping through to the
    	** default.
    	*/
            GCTRACE(1)("Received DATA_INCOMPLETE in LSNRCV\n");

    	default:
    	    GClu62er(parms, GC_LISTEN_FAIL,
	        (struct appc_hdr *) &pcb->rec_and_post);
    	    goto complete;
	}

    } /* end case GC_LSNRCV */

    case GC_LSNCMP:
    {
    	struct mc_send_data senddata;
    	struct mc_prepare_to_receive preparetoreceive;

	/* Process receive of data token and complete listen */
    	pcb->conv_state = AP_RECEIVE_STATE;
    	if (pcb->rec_and_post.primary_rc == AP_OK)
        {
    	    /* See what was received */
    	    switch (pcb->rec_and_post.what_rcvd)
	    {
	    case AP_DATA_COMPLETE_SEND:
	    case AP_SEND:

	    	/* We did get the token so move into send state */
    	    	pcb->conv_state = AP_SEND_STATE;

    	    	/* 
    	    	** Do a send containing protocol version number and 
    	    	** indicate that we're prepared to receive again.
            	*/
    	    	MEfill(sizeof(senddata), (u_char) 0, (PTR) &senddata);
    	    	senddata.opcode = AP_M_SEND_DATA;
		senddata.opext = AP_MAPPED_CONVERSATION;
		MEcopy((PTR) pcb->tp_id, (u_i4) sizeof(pcb->tp_id),
                       (PTR) senddata.tp_id);
    	    	senddata.conv_id = pcb->conversation_id;
                senddata.type = AP_SEND_DATA_FLUSH;
 
                /* either forced or requested half duplex */
                if (halfplex || pcb->duplex == NOT_DUP)
                {
		    senddata.dlen = GC_INITIAL_SEND_LENGTH;
                    version_buf[0] = GC_LU62_VERSION_1;
                }
                else
                {
                    /* agree to version and send them their id */
                    GCTRACE(4)("Server Sending Protocol and Id to Client\n");
		    senddata.dlen = GC_INITIAL_SEND_LENGTH+IDLEN;
                    version_buf[0] = GC_LU62_VERSION_2;
                    MEcopy(&pcb, sizeof(pcb), &version_buf[1]);
                }
                
		senddata.dptr = version_buf;
    	    	APPC_C(&senddata);
		if (senddata.primary_rc != AP_OK)
		{
	    	    /* send failed, indicate listen error */
	    	    GClu62er(parms, GC_LISTEN_FAIL,
			     (struct appc_hdr *) &senddata);
	    	    goto complete;
	    	}

                if (halfplex || pcb->duplex == NOT_DUP)
                {
    	    	    /* Now do prepare-to-receive */
    	    	    MEfill(sizeof(preparetoreceive), (u_char) 0, 
    	    	       (PTR) &preparetoreceive);
    	    	    preparetoreceive.opcode = AP_M_PREPARE_TO_RECEIVE;
		    preparetoreceive.opext = AP_MAPPED_CONVERSATION;
		    MEcopy((PTR) pcb->tp_id, (u_i4) sizeof(pcb->tp_id),
    	    	        (PTR) preparetoreceive.tp_id);
    	    	    preparetoreceive.conv_id = pcb->conversation_id;
		    preparetoreceive.ptr_type = AP_FLUSH;
		    APPC_C(&preparetoreceive);
		    if (preparetoreceive.primary_rc != AP_OK)
		    {
	    	        /* prepare to receive failed, report listen error */
	    	        GClu62er(parms, GC_LISTEN_FAIL, 
    	    	    	     (struct appc_hdr *) &preparetoreceive);
	    	        goto complete;
	    	    }
	    
    	    	    pcb->conv_state = AP_RECEIVE_STATE;
                }

    	    	/* Drive completion exit */
	    	goto complete;

	    case AP_DATA_COMPLETE:
    	    case AP_DATA_INCOMPLETE:
                GCTRACE(1)("Received %x in LSNCMP\n",
                            pcb->rec_and_post.what_rcvd);

		/* 
    	    	** As we're waiting for the token, we shouldn't have
    	    	** gotten any data here. Just drop through to default.
    	    	*/

    	    default:
    	        GClu62er(parms, GC_LISTEN_FAIL,
			 (struct appc_hdr *) &pcb->rec_and_post);
    	    	goto complete;
	    }

	}
    	else
	{
            GCTRACE(1)("Receive failed for LSNCMP\n");
	    /* receive failed, indicate listen error */
	    GClu62er(parms, GC_LISTEN_FAIL, 
    	    	     (struct appc_hdr *) &pcb->rec_and_post);
	    goto complete;
	}

    } /* end case GC_LSNCMP */

    case GC_CONN:
    {
    	struct convert conv;
    	struct tp_started tpstarted;
    	struct mc_allocate allocate;
    	struct mc_send_data senddata;
    	struct mc_prepare_to_receive preparetoreceive;
    	char *first_period;
    	char *second_period;
    	i4 str_len;
    	i4 i;

	/* Allocate pcb */
	if ((pcb = (GC_PCB *)GClu62newPCB(parms)) == 0)
	{
    	    GClu62er(parms, GC_CONNECT_FAIL, (struct appc_hdr *) NULL);
	    goto complete;
	}
	parms->pcb = (PTR)pcb;

    	/* Make sure we can handle another connection */
    	if (++dcb->total_conversations > MAX_CONVERSATIONS)
	{
            dcb->total_conversations--;
            GCTRACE(1)("Too many conversations(%d) at GC_CONN!\n", 
                                   dcb->total_conversations);
    	    GClu62er(parms, GC_CONNECT_FAIL, (struct appc_hdr *) NULL);

    	    /* Free up pcb */
	    if (MEfree((PTR)pcb) != OK)
            {
                GCTRACE(1)("MEfree failed pcb: %x\n", pcb);
            }
            parms->pcb = (PTR)NULL;
	    goto complete;
	}
	GCTRACE(1)("LUs: %d, Active conversations: %d\n",
                    dcb->number_of_lus, dcb->total_conversations);

#ifdef ORPHANED
        storepcb(pcb);
#endif

    	pcb->timeout = DEFAULT_TIMER_S;

    	/*
	** The format of the node_id connect parameter is as follows:
    	**
    	** <lu_alias>.<plu_alias>.<mode_name> 
    	**
	** <lu_alias> may be omitted if an LU from the default pool
	** of LUs is to be used; .<mode_name> may be omitted if a blank 
    	** mode name is used.
    	**
    	** See whether we have a null lu_alias. If we have, setup to
    	** use an LU from the pool of default LUs.
	*/
    	MEfill(sizeof(tpstarted), (u_char) 0, (PTR) &tpstarted);
    	first_period = STchr(parms->function_parms.connect.node_id, '.');
    	if (first_period == NULL)
	{
	    /* illegal node_id format */
    	    GClu62er(parms, GC_CONNECT_FAIL, (struct appc_hdr *) NULL);
    	    goto complete;
	}

    	str_len = first_period - parms->function_parms.connect.node_id;
    	if (str_len == 0)
    	    MEfill(sizeof(tpstarted.lu_alias), (u_char) 0, 
    	    	   (PTR) tpstarted.lu_alias);
	else
	{
    	    /* 
    	    ** Copy lu alias and pad with ASCII blanks. It's difficult not
	    ** to have a certain amount of contempt for an interface that makes
	    ** you deal in ASCII and EBCDIC for different parameters!!
	    */
    	    STncpy(tpstarted.lu_alias, parms->function_parms.connect.node_id, 
    	    	    str_len);
    	    for (i = str_len ; i < 8 ; i++)
	    	 tpstarted.lu_alias[i] = 0x20;
	}

    	/* Check to see whether we've already started with this local LU */
    	for (i = 0 ; i < dcb->number_of_lus ; i++)
    	    if (!STbcompare((char *)tpstarted.lu_alias, 8,
                            (char *)dcb->lu_alias[i], 8, TRUE))
		break;

    	if (i < dcb->number_of_lus)
        {
            /* Already started */
    	    MEcopy((PTR) dcb->tp_id[i], sizeof(dcb->tp_id[i]),
		   (PTR) pcb->tp_id);
	    dcb->conversations[i]++;
	}
    	else
	{
    	    /* See how many local LUs we've already started with */
    	    if (dcb->number_of_lus >= MAX_LUS)
	    {

                GCTRACE(1)("Too many lus (%d)\n", dcb->number_of_lus);
	        /* Can't handle another local LU */
    	        GClu62er(parms, GC_CONNECT_FAIL, (struct appc_hdr *) NULL);
	        goto complete;
	    }

    	    /* Start with local LU */
    	    tpstarted.opcode = AP_TP_STARTED;

    	    /* Convert tp name into EBCDIC */
    	    conv.opcode = SV_CONVERT;
	    conv.opext = 0;
	    conv.direction = SV_ASCII_TO_EBCDIC;
	    conv.char_set = SV_AE;
	    conv.len = STlength((char *)dcb->invoking_tp_name);
	    conv.source = dcb->invoking_tp_name;
    	    conv.target = tpstarted.tp_name;
    	    ACSSVC_C(&conv);
    	    if (conv.primary_rc != SV_OK)
	    {
    	        /* Start failed; decipher error */
    	        GClu62er(parms, GC_CONNECT_FAIL, (struct appc_hdr *) &conv);
    	        goto complete;
     	    }

    	    /* Pad the name with EBCDIC blanks */
    	    for (i = STlength((char *)dcb->invoking_tp_name) ; i < 64 ; i++)
	    	tpstarted.tp_name[i] = 0x40;

    	    APPC_C(&tpstarted);
    	    if (tpstarted.primary_rc != AP_OK)
    	    {
    	        /* Start failed; decipher error */
    	        GClu62er(parms, GC_CONNECT_FAIL, 
			 (struct appc_hdr *) &tpstarted);
    	        goto complete;
     	    }

    	    /* Save local LU specific info. */
    	    MEcopy((PTR) tpstarted.tp_id, sizeof(tpstarted.tp_id), 
    	           (PTR) dcb->tp_id[dcb->number_of_lus]);
    	    MEcopy((PTR) tpstarted.tp_id, sizeof(tpstarted.tp_id),
    	           (PTR) pcb->tp_id);
    	    MEcopy((PTR) tpstarted.lu_alias, sizeof(tpstarted.lu_alias),
	           (PTR) dcb->lu_alias[dcb->number_of_lus]);
    	    dcb->conversations[dcb->number_of_lus] = 1;
	    dcb->number_of_lus++;
	} /* end of else need to start with local LU */

    	/* Allocate conversation */
    	MEfill(sizeof(allocate), (u_char) 0, (PTR) &allocate);
    	allocate.opcode = AP_M_ALLOCATE;
	allocate.opext = AP_MAPPED_CONVERSATION;
	MEcopy((PTR) pcb->tp_id, (u_i4) sizeof(pcb->tp_id), 
    	       (PTR) allocate.tp_id);
	allocate.sync_level = AP_NONE;
	allocate.rtn_ctl = AP_WHEN_SESSION_FREE;

    	/* Step past first period in node_id parameter and look for second */
	first_period++;
        second_period = STchr(first_period, '.');
	if (second_period == NULL)
	{
	    /* No mode name specified - whole string is plu alias */
    	    STncpy( allocate.plu_alias, first_period, STlength(first_period));

	    /* Pad with ASCII blanks */
    	    for (i = STlength(first_period) ; i < 8 ; i++)
    	        allocate.plu_alias[i] = 0x20;
            STncpy(pcb->plu_alias, allocate.plu_alias, 8);

    	    /* Set up blank mode name - EBCDIC blanks */
    	    for (i = 0 ; i < 8 ; i++)
		allocate.mode_name[i] = 0x40;
    	}
	else
	{
    	    /* Copy up to second period for plu alias */    	
    	    str_len = second_period - first_period;
    	    STncpy( allocate.plu_alias, first_period, str_len);

	    /* Pad with ASCII blanks */
    	    for (i = str_len ; i < 8 ; i++)
	    	allocate.plu_alias[i] = 0x20;
            STncpy(pcb->plu_alias, allocate.plu_alias, 8);

    	    /* Mode name is after second period */
	    ++second_period;

    	    /* Convert it into EBCDIC */
    	    conv.opcode = SV_CONVERT;
	    conv.opext = 0;
	    conv.direction = SV_ASCII_TO_EBCDIC;
	    conv.char_set = SV_AE;
	    conv.len = STlength(second_period);
	    conv.source = (unsigned char *)second_period;
    	    conv.target = allocate.mode_name;
    	    ACSSVC_C(&conv);
    	    if (conv.primary_rc != SV_OK)
	    {
    	    	/* Start failed; decipher error */
    	    	GClu62er(parms, GC_CONNECT_FAIL, (struct appc_hdr *) &conv);
    	    	goto complete;
     	    }

    	    /* Pad name with EBCDIC blanks */
    	    for (i = STlength(second_period) ; i < 8 ; i++)
	    	allocate.mode_name[i] = 0x40;
            STncpy(pcb->mode_name, allocate.mode_name, 8);
    	}

    	/* Convert tp name into EBCDIC */
    	conv.opcode = SV_CONVERT;
	conv.opext = 0;
	conv.direction = SV_ASCII_TO_EBCDIC;
	conv.char_set = SV_AE;
	conv.len = STlength(parms->function_parms.connect.port_id);
	conv.source = (unsigned char *)parms->function_parms.connect.port_id;
    	conv.target = allocate.tp_name;
    	ACSSVC_C(&conv);
    	if (conv.primary_rc != SV_OK)
	{
    	    /* Start failed; decipher error */
    	    GClu62er(parms, GC_CONNECT_FAIL, (struct appc_hdr *) &conv);
    	    goto complete;
     	}

    	/* Pad name with EBCDIC blanks */
    	for (i = STlength(parms->function_parms.connect.port_id) ; 
    	     i < 64 ; i++)
	    allocate.tp_name[i] = 0x40;

	allocate.security = AP_NONE;
	allocate.pip_dlen = 0;
    	APPC_C(&allocate);
	if (allocate.primary_rc != AP_OK)
        {

    	    /* Some error occurred */
	    GClu62er(parms, GC_CONNECT_FAIL, (struct appc_hdr *) &allocate);
    	    goto complete;
	}

    	/* Allocate ok; save details of conversation */
    	pcb->conversation_id = allocate.conv_id;
    	pcb->conv_state = AP_SEND_STATE;

        /* connection successful, get the time */
        pcb->start_msec = GClu62gettime();

    	/* 
    	** Do a send containing the preferred protocol version number 
    	** and indicate that we're prepared to receive
        */
    	MEfill(sizeof(senddata), (u_char) 0, (PTR) &senddata);
    	senddata.opcode = AP_M_SEND_DATA;
	senddata.opext = AP_MAPPED_CONVERSATION;
	MEcopy((PTR) pcb->tp_id, (u_i4) sizeof(pcb->tp_id),
    	       (PTR) senddata.tp_id);
    	senddata.conv_id = pcb->conversation_id;
	senddata.dlen = GC_INITIAL_SEND_LENGTH;
        if (halfplex)
            version_buf[0]  = GC_LU62_VERSION_1;
        else
            version_buf[0]  = GC_LU62_VERSION_2;
	senddata.dptr = version_buf;
        senddata.type = AP_SEND_DATA_FLUSH;
    	APPC_C(&senddata);
	if (senddata.primary_rc != AP_OK)
	{
	    /* send failed, indicate connect error */
	    GClu62er(parms, GC_CONNECT_FAIL, (struct appc_hdr *) &senddata);
	    goto complete;
	}
        GCTRACE(4)("Sent version <%d> to partner\n", version_buf[0]);

	    
    	/* Now do prepare-to-receive */
    	MEfill(sizeof(preparetoreceive), (u_char) 0, 
    	       (PTR) &preparetoreceive);
    	preparetoreceive.opcode = AP_M_PREPARE_TO_RECEIVE;
	preparetoreceive.opext = AP_MAPPED_CONVERSATION;
	MEcopy((PTR) pcb->tp_id, (u_i4) sizeof(pcb->tp_id),
    	       (PTR) preparetoreceive.tp_id);
    	preparetoreceive.conv_id = pcb->conversation_id;
	preparetoreceive.ptr_type = AP_FLUSH;
	APPC_C(&preparetoreceive);
	if (preparetoreceive.primary_rc != AP_OK)
	{
		    
	    /* prepare to receive failed, indicate listen error */
	    GClu62er(parms, GC_CONNECT_FAIL,
    	    	     (struct appc_hdr *) &preparetoreceive);
	    goto complete;
	}

    	/* Post a receive */
    	MEfill(sizeof(pcb->rec_and_post), (u_char) 0, 
    	       (PTR) &pcb->rec_and_post);
    	pcb->rec_and_post.opcode = AP_M_RECEIVE_AND_POST;
	pcb->rec_and_post.opext = AP_MAPPED_CONVERSATION;
    	MEcopy((PTR) pcb->tp_id, (u_i4) sizeof(pcb->tp_id),
    	       (PTR) pcb->rec_and_post.tp_id);
	pcb->rec_and_post.conv_id = pcb->conversation_id;
	pcb->rec_and_post.max_len = CONNECT_LENGTH;
	pcb->rec_and_post.dptr= (unsigned char *)&pcb->connect_message;
    	pcb->rec_and_post.callback = GClu62posted;
    	pcb->posted_parm_list = parms;
        pcb->conv_state = AP_PEND_POST_STATE;

	parms->state = GC_CONNRCV;
	APPC_C(&pcb->rec_and_post);
	if (pcb->rec_and_post.primary_rc != AP_OK)
	{
    	    pcb->posted_parm_list = (GCC_P_PLIST *) NULL;
    	    pcb->conv_state = AP_RECEIVE_STATE;
	    GClu62er(parms, GC_CONNECT_FAIL,
    	    	     (struct appc_hdr *) &pcb->rec_and_post);
	    goto complete;
	}

    	/* Receive successfully posted */
	return;
    } /* end case GC_CONN */

    case GC_CONNRCV:
    {
    	/* Process receive of initial send by peer */
    	pcb->conv_state = AP_RECEIVE_STATE;
    	if (pcb->rec_and_post.primary_rc == AP_OK)
	{
    	    /* See what was received */
    	    switch (pcb->rec_and_post.what_rcvd)
	    {
	    case AP_DATA_COMPLETE:

		/* 
    	    	** See what version number we got back. If it's not the
    	    	** ones we support, generate connect error.
    	    	*/
    	    	if (pcb->rec_and_post.dlen < GC_INITIAL_SEND_LENGTH ||
		    (*pcb->rec_and_post.dptr != GC_LU62_VERSION_1 &&
		     *pcb->rec_and_post.dptr != GC_LU62_VERSION_2 ))
		{
    	            GClu62er(parms, GC_CONNECT_FAIL, (struct appc_hdr *) NULL);
    	    	    goto complete;
		}
                GCTRACE(4)("Received version <%d>\n", *pcb->rec_and_post.dptr);

                if (*pcb->rec_and_post.dptr == GC_LU62_VERSION_2)
                {
                    MEcopy(pcb->rec_and_post.dptr+1, IDLEN, pcb->pkey);  
                    parms->state = GC_CONN_ALLOC2;
                    pcb->atrys = 0;
                    goto top;
                }

    	    	/* Post a receive to pick up send token */
    	    	MEfill(sizeof(pcb->rec_and_post), (u_char) 0, 
    	    	            (PTR) &pcb->rec_and_post);
    	    	pcb->rec_and_post.opcode = AP_M_RECEIVE_AND_POST;
	    	pcb->rec_and_post.opext = AP_MAPPED_CONVERSATION;
    	    	MEcopy((PTR) pcb->tp_id, (u_i4) sizeof(pcb->tp_id),
    	    	       (PTR) pcb->rec_and_post.tp_id);
	    	pcb->rec_and_post.conv_id = pcb->conversation_id;
	    	pcb->rec_and_post.max_len = 0;
	    	pcb->rec_and_post.dptr= (unsigned char *) NULL;
    	    	pcb->rec_and_post.callback = GClu62posted;
    	    	pcb->posted_parm_list = parms;
                pcb->conv_state = AP_PEND_POST_STATE;

    	    	parms->state = GC_CONNCMP;
	    	APPC_C(&pcb->rec_and_post);
	    	if (pcb->rec_and_post.primary_rc != AP_OK)
	    	{
    	    	    pcb->posted_parm_list = (GCC_P_PLIST *) NULL;
		    pcb->conv_state = AP_RECEIVE_STATE;
	    	    GClu62er(parms, GC_CONNECT_FAIL,
    	    	     	     (struct appc_hdr *) &pcb->rec_and_post);
	    	    goto complete;
	    	}

    	    	/* Receive successfully posted */
    	    	return;

            case AP_DATA_COMPLETE_SEND:
                GCTRACE(1)("Received DATA_COMPLETE_SEND in CONNRCV\n");

    	    	/* drop through... */

    	    case AP_SEND:
    	    	pcb->conv_state = AP_SEND_STATE;

    	    	/* drop through... */

    	    case AP_DATA_INCOMPLETE:

    	    	/* 
    	    	** (i) We got the data token before the data!
    	    	** (ii) We got a larger record than we can cope with.
    	    	** Treat these as fatal errors by dropping through to the
    	    	** default.
    	    	*/

    	    default:
    	        GClu62er(parms, GC_CONNECT_FAIL,
    	    	    	 (struct appc_hdr *) &pcb->rec_and_post);
    	    	goto complete;
	    }
	}
	else
	{
	    /* receive failed, indicate listen error */
	    GClu62er(parms, GC_CONNECT_FAIL,
    	    	     (struct appc_hdr *) &pcb->rec_and_post);
	    goto complete;
	}

    } /* end case GC_CONNRCV */

    case GC_CONN_ALLOC2:
    {
        GCTRACE(1)("Second Allocate try: %d\n", ++pcb->atrys);

        /*  allocate the second (receive side) conversation */
        if (GClu62alloc2(parms) == OK)
        {
            GCTRACE(4)("pseudo duplex conv allocated\n");
            dcb->total_conversations++;
	    GCTRACE(1)("LUs: %d, Active conversations: %d\n",
                        dcb->number_of_lus, dcb->total_conversations);
    	    pcb->rcv_parm_list = NULL;
            parms->state = GC_CONNCMP;
            goto complete;   
        }
        else
        {
            parms->state = GC_CONN_ALLOC2;    
    	    pcb->rcv_parm_list = parms;
    	    pcb->timeout = 500 * pcb->atrys;
    	    GClu62timer(dcb, pcb);
	    return;
        }

    } /* end case GC_CONN_ALLOC2 */

    case GC_CONNCMP:
    {

    	/* Process receive of data token and complete connect */
    	pcb->conv_state = AP_RECEIVE_STATE;
    	if (pcb->rec_and_post.primary_rc == AP_OK)
        {
    	    /* See what was received */
    	    switch (pcb->rec_and_post.what_rcvd)
	    {
	    case AP_SEND:

		/* We did get the token so drive completion */
    	    	pcb->conv_state = AP_SEND_STATE;
    	    	goto complete;

	    case AP_DATA_COMPLETE:
    	    case AP_DATA_INCOMPLETE:
                GCTRACE(1)("Got %x in CONNCMP\n",
                             pcb->rec_and_post.what_rcvd);

		/* 
    	    	** As we're waiting for the token, we shouldn't have
    	    	** gotten any data here. Just drop through to default.
    	    	*/

    	    default:
    	        GClu62er(parms, GC_CONNECT_FAIL,
			 (struct appc_hdr *) &pcb->rec_and_post);
    	    	goto complete;
	    }

	}
    	else
	{

	    /* receive failed, indicate listen error */
	    GClu62er(parms, GC_CONNECT_FAIL,
    	    	     (struct appc_hdr *) &pcb->rec_and_post);
	    goto complete;
	}

    } /* end case GC_CONNCMP */

    case GC_SND:
    {
    	struct mc_request_to_send requesttosend;

	GCTRACE(2)("GClu62sm: pcb %x conv state %d send size %d\n", 
                   pcb, pcb->conv_state, parms->buffer_lng );

	/*
	** Check to see if we are in SEND state. If we are not, we will
    	** have to hold on to the send until we are.
	*/

        if (pcb->duplex == DUPLEX)         /* full duplex is always in send */
        {
    	    /* We're in send state so go ahead and try send */
    	    pcb->send_parm_list = parms;
    	    parms->state = GC_SNDRDY;
    	    goto top;
        }

    	switch (pcb->conv_state)
	{
    	case AP_SEND_STATE:
    	    /* We're in send state so go ahead and try send */
    	    pcb->send_parm_list = parms;
    	    parms->state = GC_SNDRDY;
    	    goto top;

    	case AP_RECEIVE_STATE:
    	case AP_PEND_POST_STATE:
    	    /* 
    	    ** Can't do send right now. Send a Request-to-Send to peer
    	    ** and save our send until we're redriven in
    	    ** this state when we move into conversation send state.
    	    */
    	    pcb->send_in_conv_rcv++;
    	    MEfill(sizeof(requesttosend), (u_char) 0, (PTR) &requesttosend);
    	    requesttosend.opcode = AP_M_REQUEST_TO_SEND;
    	    requesttosend.opext = AP_MAPPED_CONVERSATION;
	    MEcopy((PTR) pcb->tp_id, (u_i4) sizeof(pcb->tp_id),
    	    	   (PTR) requesttosend.tp_id);
    	    requesttosend.conv_id = pcb->conversation_id;
    	    APPC_C(&requesttosend);
	    if (requesttosend.primary_rc != AP_OK)
	    {
    	    	/* We couldn't request token so treat this as fatal error */
    	        GClu62er(parms, GC_SEND_FAIL,
    	    	    	 (struct appc_hdr *) &requesttosend);
    	    	goto complete;
	    }

    	    /* Save send until we are redriven */
    	    pcb->send_parm_list = parms;

    	    /*
    	    ** If there's a receive posted there's nothing further to do:
    	    ** when it gets driven, it'll pick up the data token if it's 
    	    ** been sent to us. If there isn't a receive posted we have to
    	    ** be a little more creative and poll for the data token.
    	    */
    	    if (pcb->conv_state == AP_PEND_POST_STATE)
	    {
		parms->state = GC_SNDRDY;
    	    	break;
	    }
    	    else
	    {
		/* No receive posted, poll for data token */
    	        MEfill(sizeof(pcb->rec_and_post), (u_char) 0, 
    	    	       (PTR) &pcb->rec_and_post);
    	    	pcb->rec_and_post.opcode = AP_M_RECEIVE_AND_POST;
	    	pcb->rec_and_post.opext = AP_MAPPED_CONVERSATION;
    	    	MEcopy((PTR) pcb->tp_id, (u_i4) sizeof(pcb->tp_id),
    	    	       (PTR) pcb->rec_and_post.tp_id);
	    	pcb->rec_and_post.conv_id = pcb->conversation_id;
	    	pcb->rec_and_post.max_len = 0;
	    	pcb->rec_and_post.dptr= (unsigned char *) NULL;
    	    	pcb->rec_and_post.callback = GClu62posted;
    	    	pcb->posted_parm_list = parms;
                pcb->conv_state = AP_PEND_POST_STATE;

    	    	parms->state = GC_SNDTKN;
	    	APPC_C(&pcb->rec_and_post);
	    	if (pcb->rec_and_post.primary_rc != AP_OK)
	    	{
    	    	    pcb->posted_parm_list = (GCC_P_PLIST *) NULL;
		    pcb->conv_state = AP_RECEIVE_STATE;
	    	    GClu62er(parms, GC_SEND_FAIL,
        	    	     (struct appc_hdr *) &pcb->rec_and_post);
    	    	    pcb->send_parm_list = NULL;
    		    goto complete;
	    	}

    	    	/* Receive successfully posted */
    	    	break;
	    }

    	default:
    	    /* Any other state is an error */
    	    GClu62er(parms, GC_SEND_FAIL, (struct appc_hdr *) NULL);
    	    goto complete;
	}

	return;
    } /* end case GC_SND */

    case GC_SNDTKN:
    {

    	/* Process receive of data token and complete listen */
    	pcb->conv_state = AP_RECEIVE_STATE;
    	if (pcb->rec_and_post.primary_rc == AP_OK)
        {
    	    /* See what was received */
    	    switch (pcb->rec_and_post.what_rcvd)
	    {
	    case AP_SEND:

		/* We did get the token so drive completion */
    	    	pcb->conv_state = AP_SEND_STATE;
    	    	parms->state = GC_SNDRDY;
    	    	goto top;

	    case AP_DATA_COMPLETE_SEND:

                GCTRACE(1)("Got DATA_COMPLETE_SEND in SNDTKN (ignoring?)\n");
		/* We did get the token so drive completion */
    	    	pcb->conv_state = AP_SEND_STATE;
    	    	parms->state = GC_SNDRDY;
    	    	goto top;


	    case AP_DATA_COMPLETE:
    	    case AP_DATA_INCOMPLETE:

		/* 
    	    	** As we didn't get token and there is data, drop
    	    	** into send complete state and wait to be driven
    	    	** from a completed receive that picks up the token.
    	    	** If there is a receive outstanding, reenter the
    	    	** state machine with it.
    	    	*/
    	    	parms->state = GC_SNDRDY;
    	    	if (pcb->rcv_parm_list)
    	    	    GClu62sm(pcb->rcv_parm_list);

    	    	return;

    	    default:
    	        GClu62er(parms, GC_SEND_FAIL,
			 (struct appc_hdr *) &pcb->rec_and_post);
    	    	pcb->send_parm_list = NULL;
    	    	goto complete;
	    }

	}
    	else
	{

	    /* receive failed, indicate send error */
	    GClu62er(parms, GC_SEND_FAIL,
    	    	     (struct appc_hdr *) &pcb->rec_and_post);
    	    pcb->send_parm_list = NULL;
	    goto complete;
	}

    } /* end case GC_SNDTKN */

    case GC_SNDRDY:
    {
    	struct mc_send_data senddata;
    	struct mc_prepare_to_receive preparetoreceive;

    	/* If the prepare to receive timer is running, cancel it */
        if (pcb->duplex == NOT_DUP)      /* duplex never issues p_to_r */
        {
	    pcb->timeout = -1;
    	    GClu62timer(dcb, pcb);
        }

	/*
	** Send complete. Attempt the send.
	*/
    	MEfill(sizeof(senddata), (u_char) 0, (PTR) &senddata);
    	senddata.opcode = AP_M_SEND_DATA;
	senddata.opext = AP_MAPPED_CONVERSATION;

        GClu62getcid(pcb, AP_SEND_STATE, senddata.tp_id, &senddata.conv_id);
        senddata.type = AP_SEND_DATA_FLUSH;
	senddata.dlen = parms->buffer_lng;
	senddata.dptr = (unsigned char *) parms->buffer_ptr;
    	APPC_C(&senddata);
	if (senddata.primary_rc != AP_OK)
	{
	    /* send failed, indicate send error */
	    GClu62er(parms, GC_SEND_FAIL, (struct appc_hdr *) &senddata);
	    pcb->send_parm_list = NULL;
	    goto complete;
	}
	    
    	/* 
    	** Send succeeded; check whether we got request-to-send from peer.
    	*/

    	pcb->total_send++;
    	pcb->send_parm_list = NULL;
    	pcb->timeout = DEFAULT_TIMER_S;
        if (pcb->duplex == NOT_DUP)
        {
    	  if (senddata.rts_rcvd == AP_YES)
	  {
	    /* Give peer the data token and flush previous send */
    	    MEfill(sizeof(preparetoreceive), (u_char) 0, 
    	       	   (PTR) &preparetoreceive);
    	    preparetoreceive.opcode = AP_M_PREPARE_TO_RECEIVE;
	    preparetoreceive.opext = AP_MAPPED_CONVERSATION;
	    MEcopy((PTR) pcb->tp_id, (u_i4) sizeof(pcb->tp_id),
    	    	   (PTR) preparetoreceive.tp_id);
    	    preparetoreceive.conv_id = pcb->conversation_id;
	    APPC_C(&preparetoreceive);
	    if (preparetoreceive.primary_rc != AP_OK)
	    {
		    
	    	/* prepare to receive failed, indicate listen error */
	    	GClu62er(parms, GC_SEND_FAIL,
    	    	     	 (struct appc_hdr *) &preparetoreceive);
    	    	pcb->send_parm_list = NULL;
	    	goto complete;
	    }
	    
    	    pcb->conv_state = AP_RECEIVE_STATE;

    	    /* Do we have a saved receive? */
    	    if (pcb->rcv_parm_list)

		/* Yes, drive state machine with it */
    	    	GClu62sm(pcb->rcv_parm_list);
	  }
	  else
	  {

    	    /* Start prepare-to-receive timer */
    	    GClu62timer(dcb, pcb);
    	    
    	    /* 
    	    ** Reset timer so that we poll with data token
    	    ** when we subsequently receive it back from our peer.
    	    ** We'll continue to poll with token until such time
    	    ** as another send completes (which resets the timer
    	    ** to the send default) or a receive completes (which 
    	    ** resets the timer to the receive default).
    	    */
    	    pcb->timeout = 0;
	  }
        } /* end half duplex */

    	goto complete;

    } /* end case GC_SNDRDY */

    case GC_RCV:
    {
	/*
	** Check to see if conversation is in RCV state. If we are not, we will
    	** have to hold on to the receive until we are.
	*/

	GCTRACE(3)("GClu62sm: pcb %x conv state %d read size %d\n", 
    	    	   pcb, pcb->conv_state, parms->buffer_lng );

        /* full duplex is always receive */
        if (pcb->conv_state == AP_RECEIVE_STATE || pcb->duplex == DUPLEX)
	{
            char buf[9];
    	    /* We're in receive state so post a receive */
    	    MEfill(sizeof(pcb->rec_and_post), (u_char) 0, 
    	    	   (PTR) &pcb->rec_and_post);
    	    pcb->rec_and_post.opcode = AP_M_RECEIVE_AND_POST;
	    pcb->rec_and_post.opext = AP_MAPPED_CONVERSATION;

            GClu62getcid(pcb, AP_RECEIVE_STATE, pcb->rec_and_post.tp_id,
                          &pcb->rec_and_post.conv_id);
            GCTRACE(4)("tp_id: %8x, conv_id: %d\n",
                              buf,  pcb->rec_and_post.conv_id);

	    pcb->rec_and_post.max_len = parms->buffer_lng;
	    pcb->rec_and_post.dptr= (unsigned char *) parms->buffer_ptr;
    	    pcb->rec_and_post.callback = GClu62posted;
    	    pcb->posted_parm_list = parms;
            pcb->conv_state = AP_PEND_POST_STATE;

    	    pcb->rcv_parm_list = parms;
    	    parms->state = GC_RCVCMP;
            GCTRACE(4)("Going to issue receiveandpost\n");
	    APPC_C(&pcb->rec_and_post);
	    if (pcb->rec_and_post.primary_rc != AP_OK)
	    {
    	    	pcb->posted_parm_list = (GCC_P_PLIST *) NULL;
		pcb->conv_state = AP_RECEIVE_STATE;
	    	GClu62er(parms, GC_RECEIVE_FAIL,
        	    	 (struct appc_hdr *) &pcb->rec_and_post);
    	    	pcb->rcv_parm_list = NULL;
                GCTRACE(3)("receiveandpost failed\n");
    		goto complete;
	    }

    	    /* Receive successfully posted */
            GCTRACE(4)("receiveandpost ok\n");
    	    return;
        }
        else if (pcb->conv_state == AP_PEND_POST_STATE) 
        {
    	    /* 
    	    ** A receive is already posted, presumably to poll for the
    	    ** data token. As posted receives cannot be cancelled, we'll 
    	    ** have to wait for it to be posted before we're redriven.
    	    */

            GCTRACE(1)("Attempted receive in send, pcb: %x, parm_list: %x\n",
                                                   pcb, pcb->rcv_parm_list);
    	    pcb->rcv_in_conv_send++;
    	    pcb->rcv_parm_list = parms;
    	    parms->state = GC_RCV;
    	    return;
        }
        else if (pcb->conv_state == AP_SEND_STATE)
        {
            /* 
    	    ** Can't do receive right now. Simply hold onto receive until
    	    ** conversation moves into RCV state. At that time, we'll be
    	    ** redriven in this state.
    	    */

            GCTRACE(1)("Attempted receive in send, pcb: %x, parm_list: %x\n",
                                                   pcb, pcb->rcv_parm_list);
    	    pcb->rcv_in_conv_send++;
    	    pcb->rcv_parm_list = parms;
    	    parms->state = GC_RCV;
    	    return;

        }
        else  /* default case */
        {
    	    /* Any other state is an error */
    	    GClu62er(parms, GC_RECEIVE_FAIL, (struct appc_hdr *) NULL);
    	    goto complete;
	}
	return;
    } /* end case GC_RCV */

    case GC_RCVCMP:
    {
    	struct mc_prepare_to_receive preparetoreceive;

	/*
	** Receive complete; process it.
	*/
    	if (pcb->rec_and_post.primary_rc == AP_OK)
        {

    	    /* See what was received */
    	    switch (pcb->rec_and_post.what_rcvd)
	    {
            case AP_DATA_COMPLETE_SEND:
                GCTRACE(1)("Got DATA_COMPLETE_SEND in RCVCMP\n");
                /* Fall through though I hate to........*/

	    case AP_DATA_COMPLETE:

    	    	/* Receive is complete */
    	    	pcb->conv_state = AP_RECEIVE_STATE;
    	    	pcb->total_rcv++;
    	    	parms->buffer_lng = pcb->rec_and_post.dlen;
    	    	pcb->rcv_parm_list = NULL;
    	    	pcb->timeout = DEFAULT_TIMER_R;

		/*
		** If there is a saved send, we have to repost a receive
                ** as it needs to continue to poll for data token.
    	    	*/
                if (pcb->duplex == NOT_DUP)
                {
		  if (pcb->send_parm_list)
		  {
    	    	    MEfill(sizeof(pcb->rec_and_post), (u_char) 0, 
    	    	       	   (PTR) &pcb->rec_and_post);
    	    	    pcb->rec_and_post.opcode = AP_M_RECEIVE_AND_POST;
	    	    pcb->rec_and_post.opext = AP_MAPPED_CONVERSATION;
    	    	    MEcopy((PTR) pcb->tp_id, (u_i4) sizeof(pcb->tp_id),
    	    	    	   (PTR) pcb->rec_and_post.tp_id);
	            pcb->rec_and_post.conv_id = pcb->conversation_id;
	    	    pcb->rec_and_post.max_len = 0;
	    	    pcb->rec_and_post.dptr= (unsigned char *) NULL;
    	    	    pcb->rec_and_post.callback = GClu62posted;
    	    	    pcb->posted_parm_list = pcb->send_parm_list;
    	    	    pcb->conv_state = AP_PEND_POST_STATE;
    	    	    pcb->send_parm_list->state = GC_SNDTKN;
	    	    APPC_C(&pcb->rec_and_post);
	    	    if (pcb->rec_and_post.primary_rc != AP_OK)
	    	    {
    	    	    	pcb->posted_parm_list = (GCC_P_PLIST *) NULL;
			pcb->conv_state = AP_RECEIVE_STATE;
	    	    	GClu62er(parms, GC_RECEIVE_FAIL,
        	    	     (struct appc_hdr *) &pcb->rec_and_post);
    		    	goto complete;
	    	    }

    	    	    /* Receive successfully posted */
		  }
                }

                if (pcb->rec_and_post.what_rcvd != AP_DATA_COMPLETE_SEND)
                    break;
                /* else fall through?? */

	    case AP_SEND:

    	    	/* We got data token */
		pcb->conv_state = AP_SEND_STATE;

    	    	/* 
    	    	** Reset state of receive. It will be re-driven
    	    	** when the conversation reverts to RCV state.
    	    	*/
    	    	parms->state = GC_RCV;

    	    	/* Do we have a saved send? */
    	    	if (pcb->send_parm_list)

		    /* Yes, drive state machine with it */
    	    	    GClu62sm(pcb->send_parm_list);
    	    	else
		{
    	    	    /* 
    	    	    ** If the current timeout period is 0, we're
    	    	    ** polling with the data token. As there is no
		    ** send outstanding, hand the token right back
		    ** from whence it came. Otherwise, start timer
    	    	    ** for how long we should hold on to data token.
    	    	    */
                  if (pcb->duplex == NOT_DUP)
                  {
    	    	    if (pcb->timeout  == 0)
		    {
    	    	        /* Give peer the data token and flush */
    	    	    	MEfill(sizeof(preparetoreceive), (u_char) 0, 
    	       	    	       (PTR) &preparetoreceive);
    	    	    	preparetoreceive.opcode = AP_M_PREPARE_TO_RECEIVE;
	    	    	preparetoreceive.opext = AP_MAPPED_CONVERSATION;
	    	    	MEcopy((PTR) pcb->tp_id, (u_i4) sizeof(pcb->tp_id),
    	    	    	       (PTR) preparetoreceive.tp_id);
	                preparetoreceive.conv_id = pcb->conversation_id;
	    	    	preparetoreceive.ptr_type = AP_FLUSH;
	    	    	APPC_C(&preparetoreceive);
	    	    	if (preparetoreceive.primary_rc != AP_OK)
	    	    	{
	    	    	    GClu62er(parms, GC_RECEIVE_FAIL, 
    	    	     	    	     (struct appc_hdr *) &preparetoreceive);
    	    	    	    pcb->rcv_parm_list = NULL;
	    	    	    goto complete;
	    	    	}

    	    	    	/*
    	    	    	** As we gave the token straight back, we
			** immediately redrive the receive.
			*/
			pcb->conv_state = AP_RECEIVE_STATE;
    	    	    	parms->state = GC_RCV;
    	    	    	goto top;
		    }
		    else
		    {
    	    	        GClu62timer(dcb, pcb);

			/* Bump timer for next time around */
			if (pcb->timeout < MAX_TIMER_R)
    	    	    	    pcb->timeout <<= 1;
		    }
                  }
		} /* end of else */

    	    	return;

    	    case AP_DATA_INCOMPLETE:

                GCTRACE(1)("GC_RCVCMP: Received DATA_INCOMPLETE\n");
                GCTRACE(1)("\nCrashing ingcc?!?!?\n\n");

		/* Deliberately crash gcc...*/
		pcb = (GC_PCB *) NULL;
                pcb->conv_state = AP_RECEIVE_STATE;

    	    	/* 
    	    	** This shouldn't happen! Check to see whether nothing
		** was returned. If that's the case, repost the receive
                ** and keep one's fingers crossed; otherwise, drop through
    	    	** to an error.
    	    	*/
    	    	pcb->conv_state = AP_RECEIVE_STATE;
    	    	if (pcb->rec_and_post.dlen == 0)
    	    	{
    	    	    MEfill(sizeof(pcb->rec_and_post), (u_char) 0, 
    	    	        (PTR) &pcb->rec_and_post);
    	    	    pcb->rec_and_post.opcode = AP_M_RECEIVE_AND_POST;
	    	    pcb->rec_and_post.opext = AP_MAPPED_CONVERSATION;
    	    	    MEcopy((PTR) pcb->tp_id, (u_i4) sizeof(pcb->tp_id),
    	    	        (PTR) pcb->rec_and_post.tp_id);
	            pcb->rec_and_post.conv_id = pcb->conversation_id;
	    	    pcb->rec_and_post.max_len = parms->buffer_lng;
	    	    pcb->rec_and_post.dptr =
			(unsigned char *) parms->buffer_ptr;
    	    	    pcb->rec_and_post.callback = GClu62posted;
    	    	    pcb->posted_parm_list = parms;
                    pcb->conv_state = AP_PEND_POST_STATE;

    	    	    pcb->rcv_parm_list = parms;
    	    	    parms->state = GC_RCVCMP;
	    	    APPC_C(&pcb->rec_and_post);
	    	    if (pcb->rec_and_post.primary_rc != AP_OK)
	    	    {
    	    	    	pcb->posted_parm_list = (GCC_P_PLIST *) NULL;
		    	pcb->conv_state = AP_RECEIVE_STATE;
	    	    	GClu62er(parms, GC_RECEIVE_FAIL,
        	    	    (struct appc_hdr *) &pcb->rec_and_post);
    	    	    	pcb->rcv_parm_list = NULL;
    		    	goto complete;
	    	    }
		    
    	    	    /* Receive successfully posted */
    	    	    return;
		}

		/* drop through ... */

    	    default:
    	    	pcb->conv_state = AP_RECEIVE_STATE;    
    	        GClu62er(parms, GC_RECEIVE_FAIL,
			 (struct appc_hdr *) &pcb->rec_and_post);
    	    	pcb->rcv_parm_list = NULL;
    	    	goto complete;
	    }

	}
    	else
	{
	    /* receive failed, indicate receive error */
    	    pcb->conv_state = AP_RECEIVE_STATE;
	    GClu62er(parms, GC_RECEIVE_FAIL,
    	    	     (struct appc_hdr *) &pcb->rec_and_post);
    	    pcb->rcv_parm_list = NULL;

    	    /*
	    ** Sleight of hand warning! If the reason we got an error was
	    ** because the receive got cancelled as a result of us issuing
	    ** a deallocate, we must first drive the completion of the
	    ** receive and then drive the completion of the disconnect (which
	    ** will be sitting in state GC_DISCONNCMP waiting to be driven).
	    ** To do this in a relatively clean fashion, we drive the receive
	    ** completion from here rather than at the bottom of the protocol
	    ** machine, and then reenter the protocol machine with the 
    	    ** disconnect.
	    */
    	    if (pcb->rec_and_post.primary_rc == AP_CANCELLED)
    	    {
    	    	GCTRACE(1)("RCVCMP: AP_CANCELLED\n" );
    	    	GCTRACE(1)("GClu62sm: %x complete %s status %x\n", parms, 
		    gc_names[ parms->state ], parms->generic_status );

    	    	(*parms->compl_exit)(parms->compl_id);

    	    	/* Now redrive disconnect */
    	    	GClu62sm(pcb->disconn_parm_list);
    	    	return;
	    }
    	    else
	    	goto complete;
	}

        goto complete;
    } /* end case GC_RCVCMP */

    case GC_DISCONN:
    {
    	GCC_P_PLIST    	*compl_parm_list;
    	struct mc_deallocate deallocate;
        struct get_state getstate;

    	if (!pcb)
	    /* nothing to do */
    	    goto complete;

    	/* Drive any outstanding send with an error */
    	if (pcb->send_parm_list)
	{
    	    compl_parm_list = pcb->send_parm_list;
    	    pcb->send_parm_list = NULL;	
	    GClu62er(compl_parm_list, GC_SEND_FAIL, (struct appc_hdr *) NULL);

    	    GCTRACE(1)("GClu62sm: %x complete %s status %d\n", 
    	    	       compl_parm_list, gc_names[ compl_parm_list->state ],
    	    	       compl_parm_list->generic_status );

    	    (*compl_parm_list->compl_exit) (compl_parm_list->compl_id);
	}

    	/* Only drive outstanding receive if there's no receive posted */
    	if (pcb->rcv_parm_list && pcb->conv_state != AP_PEND_POST_STATE)
	{
    	    compl_parm_list = pcb->rcv_parm_list;
    	    pcb->rcv_parm_list = NULL;	
	    GClu62er(compl_parm_list, GC_RECEIVE_FAIL,
		     (struct appc_hdr *) NULL);

    	    GCTRACE(1)("GClu62sm: %x complete %s status %d\n", 
    	    	       compl_parm_list, gc_names[ compl_parm_list->state ],
    	    	       compl_parm_list->generic_status );

    	    (*compl_parm_list->compl_exit) (compl_parm_list->compl_id);
	}

        if (pcb->duplex == DUPLEX)
	{
          if (pcb->invoked_tp)
          {
    	    /* Deallocate, conversation 1 is in send so do type flush */
            if ((GClu62getstate(pcb,pcb->conversation_id) == OK) &&
               (pcb->conv_state == AP_SEND_STATE ||
               pcb->conv_state == AP_SEND_PENDING_STATE))
            {
                MEfill(sizeof(deallocate), (u_char) 0, (PTR) &deallocate);
    	        deallocate.opcode = AP_M_DEALLOCATE;
	        deallocate.opext = AP_MAPPED_CONVERSATION;
	        MEcopy((PTR) pcb->tp_id, (u_i4) sizeof(pcb->tp_id),
                    (PTR) deallocate.tp_id);

    	        deallocate.conv_id = pcb->conversation_id;
                deallocate.dealloc_type = AP_FLUSH;
	        APPC_C(&deallocate);

	        if (deallocate.primary_rc != AP_OK)

          	    /* Error disconnecting  */
    	            GClu62er(parms, GC_DISCONNECT_FAIL,
    	    	    	 (struct appc_hdr *) &deallocate);
                else
                    GCTRACE(1)("Dealloc conv 1 OK\n");
            }
            else
            {
                GCTRACE(1)("Dealloc1 not issued;State = %d\n", pcb->conv_state);
            }

    	    /* Deallocate, conversation 2 is in receive so do type abend */
            if ((GClu62getstate(pcb,pcb->conversation_id_2) == OK) &&
               pcb->conv_state != AP_RESET_STATE)
            {
    	        MEfill(sizeof(deallocate), (u_char) 0, (PTR) &deallocate);
    	        deallocate.opcode = AP_M_DEALLOCATE;
	        deallocate.opext = AP_MAPPED_CONVERSATION;
	        MEcopy((PTR) pcb->tp_id_2, (u_i4) sizeof(pcb->tp_id),
    	    	    (PTR) deallocate.tp_id);
    	        deallocate.conv_id = pcb->conversation_id_2;
                deallocate.dealloc_type = AP_ABEND;

	        APPC_C(&deallocate);
	        if (deallocate.primary_rc != AP_OK)

        	    /* Error disconnecting  */
    	    	    GClu62er(parms, GC_DISCONNECT_FAIL,
                         (struct appc_hdr *) &deallocate);
                else
                    GCTRACE(1)("Dealloc conv 2 OK\n");
            }
            else
            {
                GCTRACE(1)("Dealloc2 not issued;State = %d\n", pcb->conv_state);
            }
          }
          else
          {
            GCTRACE(1)("Deallocating invoking tp conversations\n");
    	    /* Issue deallocate, conversation 2 is in send so do type flush */
            if ((GClu62getstate(pcb,pcb->conversation_id_2) == OK) &&
               (pcb->conv_state == AP_SEND_STATE ||
               pcb->conv_state == AP_SEND_PENDING_STATE))
            {

    	        MEfill(sizeof(deallocate), (u_char) 0, (PTR) &deallocate);
    	        deallocate.opcode = AP_M_DEALLOCATE;
	        deallocate.opext = AP_MAPPED_CONVERSATION;
	        MEcopy((PTR) pcb->tp_id, (u_i4) sizeof(pcb->tp_id),
    	    	    (PTR) deallocate.tp_id);
    	        deallocate.conv_id = pcb->conversation_id_2;
                deallocate.dealloc_type = AP_FLUSH;
                GCTRACE(1)("Dealloc conv 2 Call\n");

	        APPC_C(&deallocate);
	        if (deallocate.primary_rc != AP_OK)

                    /* Error disconnecting  */
    	            GClu62er(parms, GC_DISCONNECT_FAIL,
    	    	        	 (struct appc_hdr *) &deallocate);
                else
                    GCTRACE(1)("Dealloc conv 2 OK\n");
            }
            else
            {
                GCTRACE(1)("Dealloc2 not issued State = %d\n", pcb->conv_state);
            }

            /* conversation 1 is in receive so do type abend */
            if ((GClu62getstate(pcb,pcb->conversation_id) == OK) &&
                pcb->conv_state != AP_RESET_STATE )
            {
    	        MEfill(sizeof(deallocate), (u_char) 0, (PTR) &deallocate);
    	        deallocate.opcode = AP_M_DEALLOCATE;
	        deallocate.opext = AP_MAPPED_CONVERSATION;
                GCTRACE(1)("Dealloc conv 1 Call\n");
	        MEcopy((PTR) pcb->tp_id, (u_i4) sizeof(pcb->tp_id),
    	            (PTR) deallocate.tp_id);
    	        deallocate.conv_id = pcb->conversation_id;
                deallocate.dealloc_type = AP_ABEND;

	        APPC_C(&deallocate);
	        if (deallocate.primary_rc != AP_OK)

                    /* Error disconnecting  */
    	            GClu62er(parms, GC_DISCONNECT_FAIL,
                              (struct appc_hdr *)&deallocate);
                else
                    GCTRACE(1)("Dealloc conv 1 OK\n");
            }
            else
            {
                GCTRACE(1)("Dealloc1 not issued;State = %d\n", pcb->conv_state);
            }
          }
	}
        else /* halfplex */
        {
            if (GClu62getstate(pcb,pcb->conversation_id) == OK)
            {
    	        switch (pcb->conv_state)
                {
    	        case AP_RECEIVE_STATE:
    	        case AP_SEND_STATE:
    	        case AP_SEND_PENDING_STATE:
    	        case AP_PEND_POST_STATE:

    	            /* Now issue deallocate */
    	            MEfill(sizeof(deallocate), (u_char) 0, (PTR) &deallocate);
    	            deallocate.opcode = AP_M_DEALLOCATE;
	            deallocate.opext = AP_MAPPED_CONVERSATION;
	            MEcopy((PTR) pcb->tp_id, (u_i4) sizeof(pcb->tp_id),
    	    	       (PTR) deallocate.tp_id);
    	            deallocate.conv_id = pcb->conversation_id;
                  
    	            /* See what sort of deallocate to issue */
    	            if (pcb->conv_state == AP_RECEIVE_STATE ||
	            	pcb->conv_state == AP_PEND_POST_STATE)

            	    /* 
    	    	    ** We're in receive state so we can't issue a normal
    	    	    ** deallocate as we don't have the data token. Issue a
    	    	    ** deallocate (abend)
    	    	    */
                        deallocate.dealloc_type = AP_ABEND;
    	            else

    	    	        /* Issue a normal deallocate */
                        deallocate.dealloc_type = AP_FLUSH;
    
                    APPC_C(&deallocate);
        	    if (deallocate.primary_rc != AP_OK)
    
            	        /* Error disconnecting  */
            	        GClu62er(parms, GC_DISCONNECT_FAIL,
    	    	    	     (struct appc_hdr *) &deallocate);
                    else
                        GCTRACE(4)("Dealloc halfplex call OK\n");
    	        break;
                }
        
            }
        }

        if (pcb->duplex == DUPLEX)
            dcb->total_conversations -= 2;
        else
            dcb->total_conversations -= 1;
	GCTRACE(1)("LUs: %d, Active conversations: %d\n",
                    dcb->number_of_lus, dcb->total_conversations);

	parms->state = GC_DISCONNCMP;

    	/* 
    	** If there is a receive posted, we can't complete disconnect until
	** such time as it gets driven with an error indicating it's been
    	** cancelled because we've deallocated. Otherwise, we just go ahead
    	** and drive the disconnect completion.
	*/
	if (pcb->conv_state != AP_PEND_POST_STATE)
	{
    	    /* Cancel the prepare-to-receive timer if its running */
	    pcb->timeout = -1;
    	    GClu62timer(dcb, pcb);
	    pcb->conv_state = AP_RESET_STATE;
	    goto top;
	}

    	/* Store parameter list for redriving from posted receive completion */
	pcb->disconn_parm_list = parms;
	return;
    } /* end case GC_DISCONN */

    case GC_DISCONNCMP:
    {
    	struct tp_ended tpended;
    	u_i4 i, j;
        GC_PCB *opcb;

    	/* 
    	** If we were the invoked TP for this conversation, we need to issue
    	** a tp_ended to remove all trace of the invoked TP.
    	*/
	if (pcb->invoked_tp)
	{
    	    MEfill(sizeof(tpended), (u_char) 0, (PTR) &tpended);
	    tpended.opcode = AP_TP_ENDED;
	    MEcopy((PTR) pcb->tp_id, (u_i4) sizeof(pcb->tp_id),
    	    	   (PTR) tpended.tp_id);
    	    APPC_C(&tpended);
	    if (tpended.primary_rc != AP_OK)

		/* Error ending invoked TP */
		GClu62er(parms, GC_DISCONNECT_FAIL,
    	    	    	 (struct appc_hdr *) &tpended);
	}
    	else
	{
	    /* 
    	    ** We were the invoking TP for this conversation. Decrement
	    ** the count of conversations that this invoking TP has extant,
	    ** and if it reaches zero, remove all trace of the invoking TP.
	    */
    	    for (i = 0 ; i < dcb->number_of_lus ; i++)
		if (!MEcmp((PTR) pcb->tp_id, (PTR) dcb->tp_id[i], 8))
		{
    	    	    if (--dcb->conversations[i] <= 0)
		    {
                        dcb->conversations[i] = 0;

    	    	    	MEfill(sizeof(tpended), (u_char) 0, (PTR) &tpended);
	    	    	tpended.opcode = AP_TP_ENDED;
	    	    	MEcopy((PTR) pcb->tp_id, (u_i4) sizeof(pcb->tp_id),
    	    	    	       (PTR) tpended.tp_id);
    	    	    	APPC_C(&tpended);
	    	    	if (tpended.primary_rc != AP_OK)

		    	    /* Error ending invoked TP */
		    	    GClu62er(parms, GC_DISCONNECT_FAIL,
    	    	    	             (struct appc_hdr *) &tpended);
		        else
		        {
			    /*
			    ** Remove invoking TP from dcb table by shuffling
    	    	    	    ** up subsequent entries.
			    */
			    for (j = i+1; j < dcb->number_of_lus; j++)
			    {
    	    	    	    	MEcopy((PTR) dcb->tp_id[j], 
    	    	    	    	       (u_i4) sizeof(dcb->tp_id[j]),
				       (PTR) dcb->tp_id[j-1]);
			    	MEcopy((PTR) dcb->lu_alias[j],
				       (u_i4) sizeof(dcb->lu_alias[j]),
				       (PTR) dcb->lu_alias[j-1]);
                                dcb->conversations[j-1] = dcb->conversations[j];
			    } /* end of for */

    	    	    	    dcb->number_of_lus--;
			} /* end of else */

		    } /* end of if zero conversations */

		    break;
		} /* end of if */

	} /* end of else */

        GCTRACE(2)("pcb: 0x%p, connect time: %d msec\n",
                         pcb,
                         pcb->start_msec ? GClu62gettime() - pcb->start_msec:0);

    	GCTRACE(2) 
        ("GClu62sm: pcb %x %d of %d sends in RCV; %d of %d receives in SEND\n",
    	               pcb, pcb->send_in_conv_rcv, pcb->total_send,
                            pcb->rcv_in_conv_send, pcb->total_rcv);

#ifdef ORPHANED
        clearpcb(pcb);
#endif

    	/* Free up pcb */
	if (MEfree((PTR)pcb) != OK)
        {
            GCTRACE(1)("MEfree failed pcb: %x\n", pcb);
        }
	parms->pcb = (PTR)NULL;
        
    	goto complete;
    } /* end case GC_DISCONNCMP */

    } /* end switch */


complete:
    /*
    ** Drive completion routine.
    */

    GCTRACE(3)("GClu62sm: %x complete %s status %x\n", parms, 
		gc_names[ parms->state ], parms->generic_status );

    (*parms->compl_exit)(parms->compl_id);

}



/*
** Name: GClu62getstate - get the state of a conversation
**
** Description:
**
** Inputs:
**	pcb pointer, conversation id
**
** Returns:
**	status                     
**
** History
**      18-Mar-96 (tlong)
**          Created.
*/
static STATUS
GClu62getstate(pcb,cid)
GC_PCB *pcb;
i4     cid;
{
    struct get_state getstate;

    MEfill(sizeof(getstate), (u_char) 0, (PTR) &getstate);
    getstate.opcode = AP_GET_STATE;
    MEcopy((PTR) pcb->tp_id, (u_i4) sizeof(pcb->tp_id), (PTR) getstate.tp_id);
    getstate.conv_id = cid;

    APPC_C(&getstate);   
    if (getstate.primary_rc == AP_OK)
    {
        pcb->conv_state = getstate.conv_state;
        return OK;
    }
    else
    {
        GCTRACE(1)("getstate, conv_id  FAILED\n");
        return FAIL;
    }
}

/*
** Name: GClu62newPCB - allocate a new pcb
**
** Description:
**
** Inputs:
**	Nothing
**
** Returns:
**	address of allocated pcb
**
** History
**      18-Mar-96 (tlong)
**          Created.
*/
static GC_PCB * 
GClu62newPCB (parms)
GCC_P_PLIST *parms;
{
    unsigned char i;
    GC_PCB *pcb;

    pcb = (GC_PCB *)MEreqmem( 0, sizeof( *pcb ), TRUE, (STATUS *)0 );
    GCTRACE(1)("Allocate pcb %x\n", pcb);

    if (!pcb)
    {
        GCTRACE(1)("Allocate new pcb FAILED!\n");
        return (GC_PCB *)0;
    }
    parms->pcb = (PTR)pcb;

    /* Initialize pcb */
    pcb->posted_parm_list = (GCC_P_PLIST *) NULL;
    pcb->lsn_parm_list = (GCC_P_PLIST *) NULL;
    pcb->send_parm_list = (GCC_P_PLIST *) NULL;
    pcb->rcv_parm_list = (GCC_P_PLIST *) NULL;
    pcb->disconn_parm_list = (GCC_P_PLIST *) NULL;
    MEfill(sizeof(pcb->tp_id), 0, pcb->tp_id);
    pcb->invoked_tp = FALSE;
    pcb->duplex = NOT_DUP;
    pcb->conv_state = AP_RESET_STATE;
    pcb->conversation_id = 0;
    pcb->timeout = 0;
    pcb->timer_running = FALSE;
    pcb->total_send = 0;
    pcb->send_in_conv_rcv = 0;
    pcb->total_rcv = 0;
    pcb->rcv_in_conv_send = 0;
    pcb->start_msec = 0;
    return pcb;
}

/*
** Name: GClu62alloc2 - allocate the second conversation in a pseudo duplex
**                      world
**
** Description:
**
** Inputs:
**	address of the pcb
**
** Returns:
**	normal status
**
** History
**      15-Mar-96 (tlong)
**          Created.
*/
static STATUS
GClu62alloc2(parms)
GCC_P_PLIST *parms;
{

    struct mc_allocate allocate;
    struct mc_send_data senddata;
    struct convert conv;
    int i,s;
    GC_PCB *pcb = (GC_PCB *)parms->pcb;


    MEcopy((PTR) pcb->tp_id, (u_i4)sizeof(pcb->tp_id), (PTR)pcb->tp_id_2);

    GCTRACE(4)("Allocating second conversation\n");

    /* Allocate conversation */
    MEfill(sizeof(allocate), (u_char) 0, (PTR) &allocate);
    allocate.opcode = AP_M_ALLOCATE;
    allocate.opext = AP_MAPPED_CONVERSATION;
    MEcopy((PTR) pcb->tp_id, (u_i4)sizeof(pcb->tp_id), (PTR)allocate.tp_id);
    allocate.sync_level = AP_NONE;
    allocate.rtn_ctl = AP_WHEN_SESSION_FREE;
   
    /* Convert tp name into EBCDIC */
    conv.opcode = SV_CONVERT;
    conv.opext = 0;
    conv.direction = SV_ASCII_TO_EBCDIC;
    conv.char_set = SV_AE;
    conv.len = STlength(parms->function_parms.connect.port_id);
    conv.source = (unsigned char *)parms->function_parms.connect.port_id;
    conv.target = allocate.tp_name;
    ACSSVC_C(&conv);
    if (conv.primary_rc != SV_OK)
    {
        /* Start failed; decipher error */
        GClu62er(parms, GC_CONNECT_FAIL, (struct appc_hdr *) &conv);
        return FAIL;
    }

    MEcopy((PTR) pcb->plu_alias, (u_i4) sizeof(pcb->plu_alias), 
    	       (PTR) allocate.plu_alias);
    MEcopy((PTR) pcb->mode_name, (u_i4) sizeof(pcb->mode_name),
    	       (PTR) allocate.mode_name);

    /* Pad name with EBCDIC blanks */
    for (i = STlength(parms->function_parms.connect.port_id); i < 64; i++)
         allocate.tp_name[i] = 0x40;

    allocate.security = AP_NONE;
    allocate.pip_dlen = 0;
    APPC_C(&allocate);
    if (allocate.primary_rc != AP_OK)
    {
        if (allocate.primary_rc == AP_ALLOCATION_ERROR &&
           (allocate.secondary_rc == AP_ALLOCATION_FAILURE_RETRY ||
            allocate.secondary_rc == AP_TRANS_PGM_NOT_AVAIL_RETRY))
        {
            GCTRACE(1)("Second Allocate soft FAIL\n");
            return FAIL;
        }
        else
        {
            GCTRACE(1)("Second Allocate hard FAIL\n");
            GClu62er(parms, GC_CONNECT_FAIL, (struct appc_hdr *) &allocate);
            return FAIL;
        }
    }
 
    /* Allocate ok; save details of conversation */
    pcb->conversation_id_2 = allocate.conv_id;

    /* 
    ** Do a send containing the 'listen up' indicator 
    */
    MEfill(sizeof(senddata), (u_char) 0, (PTR) &senddata);
    senddata.opcode = AP_M_SEND_DATA;
    senddata.opext = AP_MAPPED_CONVERSATION;
    MEcopy((PTR)pcb->tp_id, (u_i4)sizeof(pcb->tp_id), (PTR)senddata.tp_id);
    senddata.conv_id = pcb->conversation_id_2;
    senddata.dlen = GC_INITIAL_SEND_LENGTH;
    
    version_buf[0] = GC_ATTN;
    senddata.dptr = version_buf;
    senddata.type = AP_SEND_DATA_FLUSH;
    APPC_C(&senddata);
    if (senddata.primary_rc != AP_OK)
    {
        /* send failed, indicate connect error */
        GClu62er(parms, GC_CONNECT_FAIL, (struct appc_hdr *) &senddata);
        return FAIL;
    }
	    
    /* 
    ** Do a send containing our id 
    */
    MEfill(sizeof(senddata), (u_char) 0, (PTR) &senddata);
    senddata.opcode = AP_M_SEND_DATA;
    senddata.opext = AP_MAPPED_CONVERSATION;
    MEcopy((PTR)pcb->tp_id, (u_i4)sizeof(pcb->tp_id), (PTR)senddata.tp_id);
    senddata.conv_id = pcb->conversation_id_2;
    senddata.dlen = 4;
    
    MEcopy(pcb->pkey, IDLEN, &version_buf[0]);
    senddata.dptr = version_buf;
    senddata.type = AP_SEND_DATA_FLUSH;
    APPC_C(&senddata);
    if (senddata.primary_rc != AP_OK)
    {
        /* send failed, indicate connect error */
        GClu62er(parms, GC_CONNECT_FAIL, (struct appc_hdr *) &senddata);
        return FAIL;
    }

    pcb->duplex = DUPLEX;

    return OK;
}
/* end of GClu62alloc2 */

/*
** Name: GClu62name - determine invoking and invoked tp names
**
** Description:
**	Returns our invoking and invoked transaction program names.
**      For invoked transaction	program name, it uses the value of 
**      ii.(host).gcc.*.sna_lu62.port, if defined, otherwise the input port id. 
**      For invoking transaction program name, it uses the installation code, 
**      if defined, otherwise the input port id.
**
**  	Additionally, uses the value of ii.(host).gcc.*.sna_lu62.poll to set
**  	the rate at which we poll for receive allocates.
**
** Inputs:
**      protocol id (sna_lu62) from the protocol table
**	the default port id
**
** Returns:
**	the resulting invoking and invoked transaction program names
**
** History
**	26-Jan-93 (cmorris)
**	    Created from gcsunlu62.c
**  	14-Jul-93 (cmorris)
**  	    Use the default port ID for our transaction program name
**  	    if no installation code is defined.
**  	15-Jul-93 (cmorris)
**  	    Modified to return invoking/invoked tp names only.
**      28-Feb-96 (tlong)
**          Modified to use PMget().
*/
static STATUS
GClu62name(protocol_id, port_id, invoked_tp_name, invoking_tp_name)
char    *protocol_id;
char	*port_id;
char 	*invoked_tp_name;
char 	*invoking_tp_name;
{
    char *inst = 0;
    char pmsym[128];
    char *val = 0;

    NMgtAt ("II_INSTALLATION", &inst);
    if (inst && *inst)
        /* Use the installation code as the invoking tp name */
        STlcopy(inst, invoking_tp_name, TP_NAME_LEN);
    else
        /* Use input as the invoking tp name */
        STlcopy(port_id, invoking_tp_name, TP_NAME_LEN);

    /*
    ** For the listen port, we're looking for the PM resource
    **  ii.(host).gcc.*.sna_lu62.port:     xxx 
    */

    STprintf(pmsym, "!.%s.port", protocol_id);

    /* check to see if entry for given user */
    if (PMget (pmsym,  &val) != OK )
    {
	if (!port_id || *port_id == EOS)
        {
            GCTRACE(1)("GClu62name: failure\n");
            return FAIL;
        }
        else
        {
            /* use input */
            GCTRACE(4)("GClu62name: name: %s from %s \n", val, "port_id");
            val = port_id ;
        }
    }
    else
    {
       GCTRACE(4)("GClu62name: name: %s from %s \n", val, pmsym);
    }
    STlcopy(val, invoked_tp_name, TP_NAME_LEN);


    /*
    ** For the background polling rate look for the PM resource
    **  ii.(host).gcc.*.sna_lu62.poll:     xxx 
    */
    STprintf(pmsym, "!.%s.poll", protocol_id);

    /* check to see if entry for given user */
    if (PMget (pmsym,  &val) == OK )
    {
	GClu62_timer_b = atoi(val);
    	if (GClu62_timer_b < 0)
	    GClu62_timer_b = -1;
    }

    return OK ;
}

/*
** Name: GClu62er - process lu6.2 error
**
** Description:
**  	Processes an error generated by the APPC API, and sets
**  	the error up in the parameter list. Also, changes state
**  	of conversation for those errors that indicate an incoming
**  	deallocation.
**
** Inputs:
**	the parameter list for the operation that failed
**  	the GC and APPC API errors generated
**
** Returns:
**	nothing
**
** History
**	26-Jan-93 (cmorris)
**	    Created from gcsunlu62.c
**  	13-Jul-93 (cmorris)
**  	    Only change the transaction state if the error is related
**  	    to a connection.
**	02-Feb-94 (cmorris)
**	    Log a default system error when no LU6.2 error is passed in;
**	    don't reference the APPC API error from trace message when no
**	    such error is passed in; log gc and transaction states.
**	07-Feb-94 (cmorris)
**	    Log relevant sub-parameters when needed; get the api function 
**	    from the appc header rather than as a separate parameter.
*/
static VOID
GClu62er(parms, generic_status, appchdr)
GCC_P_PLIST *parms;
STATUS	    generic_status;
struct appc_hdr	*appchdr;
{
    GC_PCB	*pcb = (GC_PCB *)parms->pcb;
    char	*s = parms->system_status.moreinfo[0].data.string;
    i4          cnv_state;
    struct receive_allocate
                *receiveallocate;
    struct mc_receive_and_post
                *receiveandpost;

    if (pcb)
      cnv_state = pcb->conv_state;
    else
      cnv_state = AP_RESET_STATE;

    /* Firstly, set up error in parameter list */
    parms->generic_status = generic_status;

    /* use BS_SYSERR for now... */
    SETCLERR(&parms->system_status, BS_SYSERR, 0);

    if (appchdr != (struct appc_hdr *) NULL)
    {

	if (appchdr->primary_rc != AP_OK)
	{
    	    /* Set up error string */
    	    STprintf(s, "(%s/%s %x:%x:%x)", gc_names[parms->state],
		     st_names[cnv_state], appchdr->primary_rc,
		     appchdr->secondary_rc, appchdr->opcode);

            GCTRACE(1)("\nGClu62er: %x error str: %s\n", parms, s);

	    if (pcb)
	    {
    	        /* See whether error causes a state change */
	        switch (appchdr->primary_rc)
	        {
	        case AP_ALLOCATION_ERROR:
    	        case AP_COMM_SUBSYSTEM_ABENDED:
	        case AP_COMM_SUBSYSTEM_NOT_LOADED:
	        case AP_CONV_FAILURE_RETRY:
	        case AP_CONV_FAILURE_NO_RETRY:
	        case AP_DEALLOC_ABEND:
	        case AP_DEALLOC_ABEND_PROG:
	        case AP_DEALLOC_ABEND_SVC:
    	        case AP_DEALLOC_ABEND_TIMER:
	        case AP_DEALLOC_NORMAL:
    	
    	    	    /*
                    ** Only change state if there's no receive posted;
		    ** otherwise, the receive's completion will change
		    ** the state based on the error that's returned.
		    */
		    if (pcb->conv_state != AP_PEND_POST_STATE)
    	       	    	pcb->conv_state = AP_RESET_STATE;
    	            break;

		case AP_CANCELLED:

		    /*
		    ** This indicates cancellation of a posted receive
		    ** due to our deallocating the underlying conversation.
		    */
		    pcb->conv_state = AP_RESET_STATE;
		    break;
                    
	        }
	    }
            else
            {
                GCTRACE(1)("\nGClu62er: No pcb?\n");  
            }
	}
	else
	{
            /* Depending on the type of verb executed, log relevant info. */
	    switch (appchdr->opcode)
	    {
	    case AP_RECEIVE_ALLOCATE:

		/* Log sync. level and conversation type */
		receiveallocate = (struct receive_allocate *) appchdr;
    	    	STprintf(s, "(%s/%s %x:%d:%d:%x)", gc_names[parms->state],
		         st_names[cnv_state], appchdr->primary_rc,
			 receiveallocate->sync_level, 
			 receiveallocate->conv_type, appchdr->opcode);
                GCTRACE(1)("\nGClu62er: REC_ALLOC, error str: %s\n", s);
                break;

	    case AP_M_RECEIVE_AND_POST:
	      
		/* Log the "what rcvd", "max_len" and "dlen" parameters */
		receiveandpost = (struct mc_receive_and_post *) appchdr;
    	    	STprintf(s, "(%s/%s %x:%d:%d:%d:%x)", gc_names[parms->state],
		         st_names[cnv_state], appchdr->primary_rc,
			 receiveandpost->what_rcvd, receiveandpost->max_len,
			 receiveandpost->dlen, appchdr->opcode);
                GCTRACE(1)("\nGClu62er: REC_POST, error str: %s\n", s);
		break;

            default:

		GCTRACE(1)("\nGClu62er: Error for %x\n", appchdr->opcode);
                break;

	    }
	    
            GCTRACE(1)("\nGClu62er: %x lu62 status %x\n", parms, 
		       appchdr->primary_rc);
            GCTRACE(1)("\nGClu62er: error str2: %s\n", s);
        }
    }
    else
    {
	/* Just set up a default error string */
    	STprintf(s, "(%s/%s %x:%x:%x)", gc_names[parms->state], 
		 st_names[cnv_state], 0, 0, 0);

        GCTRACE(1)("\nGClu62er: %x lu62 status %x\n", parms, 0);
        GCTRACE(1)("\nGClu62er: error str3: %s\n", s);
    }

    parms->system_status.moreinfo[0].size = STlength(s);
}


/*
** Name: GClu62timer - Register conversation for timer notification
**
** Description:
**      Adds a conversation to the timer list, together
**  	with the time to expiration. -1 indicates timer
**	cancellation.
**
** Inputs:
**	driver control block 
**  	per-conversation control block
**
** Returns:
**	nothing
**
** History
**	13-Jan-92 (cmorris)
**	    Created from gcsunlu62.c.
**	01-Mar-94 (cmorris)
**	    When finding rank to add new timer, put it at the end of set
**	    of timers with the same timeout, not the beginning; always
**	    check to cancel pcb timer registration before setting new timeout.
**	02-Mar-94 (cmorris)
**	    Protect access to timer datastructures from SNA interrupts.
**	03-Mar-94 (cmorris)
**	    Only re-register file descriptor if a new timeout period is
**	    required.
*/
static VOID
GClu62timer(dcb, pcb)
GC_DCB	    *dcb;
GC_PCB	    *pcb;
{
    u_i4 i, j;
    struct timeval newtod;
    struct timezone newzone;
    i4 elapsed_msec;
    
    GCTRACE(3) ("GClu62timer: pcb %x timeout %d\n", pcb, pcb->timeout);

    /* Entering critical region */
    (VOID) sighold((int) SIGPOLL);

    /* Cancel timer if it's running */
    if (pcb->timer_running)
    {
        for (i = 0; i <dcb->number_of_timers ; i++)
        {
    	    if (dcb->tim_pcb[i] == pcb)
    	    {
    	        /* Found pcb:- shuffle down subsequent entries */
    	        for (j = i+1 ; j < dcb->number_of_timers ; j++)
	        {
		    dcb->tim_pcb[j-1] = dcb->tim_pcb[j];
    	    	    dcb->tim_expiration[j-1] = dcb->tim_expiration[j];
    	        }

                pcb->timer_running = FALSE;
                dcb->number_of_timers--;
    	        break;
	    } /* end of if */

        } /* end of for */

    } /* end of if */

    /* Set timer if time to expiration specified */
    if (pcb->timeout >= 0)
    {
    	/* Take time snapshot */
    	gettimeofday(&newtod, &newzone);

    	/*
    	** For each existing timer, adjust time to expiration by
    	** the amount that has expired since the last snapshot.
    	*/
    	elapsed_msec = ((newtod.tv_sec - dcb->timtod.tv_sec) * 1000) +
                       ((newtod.tv_usec - dcb->timtod.tv_usec) / 1000); 
    	for (i= 0 ; i <dcb->number_of_timers ; i++)
        {
	    dcb->tim_expiration[i] -= elapsed_msec;

    	    /* 
    	    ** If timer has become negative, reset it to 0 so that
    	    ** we'll reregister with a zero timeout.
    	    */
    	    if (dcb->tim_expiration[i] < 0)
    	    	dcb->tim_expiration[i] = 0;
    	}	   

    	/* Find rank to add timer to list */
    	for (i = 0; i <dcb->number_of_timers ; i++)
    	    if (dcb->tim_expiration[i] > pcb->timeout)
    	    	break;
    
	/* Correct rank found:- shuffle up subsequent entries */
    	for (j = dcb->number_of_timers; j > i; j--)
	{
	    dcb->tim_pcb[j] = dcb->tim_pcb[j-1];
    	    dcb->tim_expiration[j] = dcb->tim_expiration[j-1];
    	}

        /* Add conversation to timer list */
    	pcb->timer_running = TRUE;
        dcb->tim_pcb[i] = pcb;

        if (pcb->duplex == NOT_DUP)
       	    dcb->tim_expiration[i] = pcb->timeout;
        else
       	    dcb->tim_expiration[i] = 50;

        dcb->number_of_timers++;

    	/* Store time snapshot */
    	dcb->timtod.tv_sec = newtod.tv_sec;
    	dcb->timtod.tv_usec = newtod.tv_usec;
    } /* end of else */

    /* If new timeout period is required, reregister for read completion */
    if (i == 0)
    	(VOID)iiCLfdreg(dcb->pipe_fd[0], FD_READ, GClu62read, 
    	    	    	(PTR) dcb, dcb->tim_expiration[0]);

    /* Exiting critical region */
    (VOID) sigrelse((int) SIGPOLL);
    GCTRACE(4)("GClu62timer: OUT\n");
}


/*
** Name: GClu62read - Read callback
**
** Description:
**  	Callback for read completion. Drives completions of any
**  	expired timers.
**
** Inputs:
**  	driver control block
**
** Returns:
**	nothing
**
** History
**	10-Jan-93 (cmorris)
**	    Created from gcsunlu62.c.
*/
static VOID
GClu62read(dcb, error)
GC_DCB 	*dcb;
int error;
{
    struct timeval	newtod;	    /* time of day */
    struct timezone 	newzone;    /* timezone */
    GC_PCB  	    *pcb;
    i4 	    elapsed_msec;
    u_i4           i, j;
    GCC_P_PLIST     *parms;
    u_i4   	    number_of_expired_timers;
    GC_PCB  	    *expired_pcb[MAX_CONVERSATIONS];

    GCTRACE(3) ("GClu62read: %d timer(s), status %x\n", 
                	    dcb->number_of_timers, error);

    /* Entering critical region */
    (VOID) sighold((int) SIGPOLL);

    /* Take snapshot of time */
    gettimeofday(&newtod, &newzone);
    	
    /*
    ** For each existing timer, adjust time to expiration by
    ** the amount that has expired since the last snapshot.
    */
    elapsed_msec = ((newtod.tv_sec - dcb->timtod.tv_sec) * 1000) +
                   ((newtod.tv_usec - dcb->timtod.tv_usec) / 1000); 
    for (i= 0 ; i <dcb->number_of_timers ; i++)
    {
	dcb->tim_expiration[i] -= elapsed_msec;

    	/* 
    	** If timer has become negative, reset it to 0. Also, if remaining 
    	** timer is less than 10ms, reset it to 0: experience indicates
	** that we get driven for timer completion even when the elapsed
	** time has a few ms to go.
    	*/
    	if (dcb->tim_expiration[i] < 10)
    	    dcb->tim_expiration[i] = 0;
    }

    /* Store time snapshot */
    dcb->timtod.tv_sec = newtod.tv_sec;
    dcb->timtod.tv_usec = newtod.tv_usec;

    /* Build list of expired timers */

    number_of_expired_timers = 0;
    for (i = 0; i <dcb->number_of_timers; i++)
    {
    	if (dcb->tim_expiration[i] <= 0)

	    /* 
    	    ** Timer has expired, remove entry from list and
    	    ** save it as the expired timer.
    	    */
    	    expired_pcb[number_of_expired_timers++] = dcb->tim_pcb[i];
    	else
	    /* That's the end of the expired timers */
	    break;
    }

    /* Shuffle up remaining timers */
    for (j = 0; i < dcb->number_of_timers; j++, i++)
    {
	dcb->tim_pcb[j] = dcb->tim_pcb[i];
    	dcb->tim_expiration[j] = dcb->tim_expiration[i];
    }

    dcb->number_of_timers -= number_of_expired_timers;

    /* Exiting critical region */
    (VOID) sigrelse((int) SIGPOLL);

    /* For each expired timer, call timer expiration function */
    for (i = 0; i <number_of_expired_timers; i++)
    {
    	pcb = expired_pcb[i];
    	pcb->timer_running = FALSE;
	GClu62timeout(pcb);
    }

    /* See if we still have a non-null number of timers */
    if (dcb->number_of_timers) 
    {

        GCTRACE(3) ("GClu62read: timeout %d\n", dcb->tim_expiration[0]);

	/* Register for read completion */
    	(VOID)iiCLfdreg(dcb->pipe_fd[0], FD_READ, GClu62read, 
    	                (PTR) dcb, dcb->tim_expiration[0]);
    }
}


/*
** Name: GClu62timeout - Handle timer expiration
**
** Description:
**  	This function handles "prepare-to-receive" timer
**  	expiration by issuing a prepare to receive request
**  	on the conversation. If this succeeds, it reenters
**      the protocol machine with any oustanding receive. If
**  	it fails, it reenters the protocol machine with either
**  	an outstanding receive or send, driving it with the
**  	error.
**  	
**      It also handles timeouts when there is a receive posted by
**  	immediately entering the state machine for the receive.
**
** Inputs:
**  	pcb - per-connection control block
**
** Returns:
**	nothing
**
** History
**	26-Jan-93 (cmorris)
**	    Created from gcsunlu62.c.
**  	20-May-93 (cmorris)
**  	    Added support for driving receive state machine when a
**  	    receive is posted.
**  	26-May-93 (cmorris)
**  	    Added support for listen polling.
*/
static VOID
GClu62timeout(pcb)
GC_PCB 	*pcb;
{
    struct mc_prepare_to_receive preparetoreceive;
    GCC_P_PLIST 	    *compl_parm_list;

    GCTRACE(2)("GClu62timeout: pcb %x, cnv_state %x\n", pcb, pcb->conv_state);

    /* See what state the conversation is in */
    switch (pcb->conv_state)
    {
    case AP_RESET_STATE:

    	/* Reenter state machine to poll for listen */
    	compl_parm_list = pcb->lsn_parm_list;
    	pcb->lsn_parm_list = (GCC_P_PLIST *) NULL;
    	GClu62sm(compl_parm_list);
    	break;

    case AP_PEND_POST_STATE:

	/* Reenter state machine for completion of posted receive */
	compl_parm_list = pcb->posted_parm_list;
    	pcb->posted_parm_list = (GCC_P_PLIST *) NULL;
    	GClu62sm(compl_parm_list);
    	break;

    case AP_SEND_STATE:

      if (pcb->duplex != DUPLEX)            /* half duplex */
      {
    	/* Give peer the data token and flush */
    	MEfill(sizeof(preparetoreceive), (u_char) 0, (PTR) &preparetoreceive);
    	preparetoreceive.opcode = AP_M_PREPARE_TO_RECEIVE;
    	preparetoreceive.opext = AP_MAPPED_CONVERSATION;
    	MEcopy((PTR) pcb->tp_id, (u_i4) sizeof(pcb->tp_id), 
               (PTR) preparetoreceive.tp_id);
    	preparetoreceive.conv_id = pcb->conversation_id;
    	preparetoreceive.ptr_type = AP_FLUSH;
    	APPC_C(&preparetoreceive);
    	if (preparetoreceive.primary_rc != AP_OK)
    	{
            GCTRACE(1)("GClu62timeout: preparetoreceive fail (%x/%x)\n",
                                       preparetoreceive.primary_rc,
                                       preparetoreceive.secondary_rc);
	    /*
    	    ** Error sending prepare-to-receive. Look for an outstanding 
    	    ** receive or send to reenter the protocol machine with, in
    	    ** order to report the error back to the user. 
    	    */
    	    if (pcb->rcv_parm_list)
	    {
    	    	compl_parm_list = pcb->rcv_parm_list;
    	    	pcb->rcv_parm_list = NULL;	
	    	GClu62er(compl_parm_list, GC_RECEIVE_FAIL, 
    	    	    	 (struct appc_hdr *) &preparetoreceive);

    	    	GCTRACE(1)("GClu62timeout1: %x complete %s status %d\n", 
    	    	       	   compl_parm_list, gc_names[ compl_parm_list->state ],
    	    	       	   compl_parm_list->generic_status );
    	    	(*compl_parm_list->compl_exit) (compl_parm_list->compl_id);
	    }
    	    else if (pcb->send_parm_list)
	    {
    	    	compl_parm_list = pcb->send_parm_list;
    	    	pcb->send_parm_list = NULL;	
	    	GClu62er(compl_parm_list, GC_RECEIVE_FAIL, 
    	    	    	 (struct appc_hdr *)&preparetoreceive);

    	    	GCTRACE(1)("GClu62timeout2: %x complete %s status %d\n", 
    	    	           compl_parm_list, gc_names[ compl_parm_list->state ],
    	    	       	   compl_parm_list->generic_status );

    	    	(*compl_parm_list->compl_exit) (compl_parm_list->compl_id);
	    }
    	
    	}		    
    	else
    	{
    	    pcb->conv_state = AP_RECEIVE_STATE;

            /* Do we have a saved receive? */
            if (pcb->rcv_parm_list)

	    	/* Yes, drive state machine with it */
    	    	GClu62sm(pcb->rcv_parm_list);
    	}
      }  /* end half-duplex */
      else
      {     
    	    pcb->conv_state = AP_RECEIVE_STATE;

            /* Do we have a saved receive? */
            if (pcb->rcv_parm_list)
    	    	GClu62sm(pcb->rcv_parm_list); /* drive state machine with it */

      }  /* end duplex */
    break;

    case AP_RECEIVE_STATE:
            /* Do we have a saved receive? */
            if (pcb->rcv_parm_list)
    	    	GClu62sm(pcb->rcv_parm_list); /* drive state machine with it */
    break;
 
    default:
    break;

    }  /* end switch */
}

/*
** Name: GClu62posted - Handle completion of posted receive.
**
** Description:
**  	This function handles the completion of a posted receive by
**  	finding the parameter list associated with the receive and
**  	reentering the protocol machine with it.
**
** Inputs:
**  	vcb - verb control block
**  	tp_id - the tp id of the TP issuing the receive
**  	conv_id - the id of the conversation issuing the receive
**
** Returns:
**	nothing
**
** History
**	01-Feb-93 (cmorris)
**	    Created.
**  	20-May-93 (cmorris)
**  	    Rewritten to avoid directly entering state machine. This doesn't
**  	    work due to undocumented limitations in the appc interface that
**  	    preclude the issuance of appc calls from receive_and_post() 
**  	    callbacks (which is what this function is executed as). Now the
**  	    function sets a timer with a 0 timeout so that clpoll() will
**  	    immediately redrive us through the timeout mechanism. 
**  	26-May-93 (cmorris)
**  	    Save and restore the current timeout when setting 0 timeout.
*/
static
VOID
GClu62posted(receiveandpost, tp_id, conv_id)
struct mc_receive_and_post *receiveandpost;
unsigned char tp_id[8];
long conv_id;
{
    GCC_P_PLIST     *parms;
    GC_DCB 	    *dcb;
    GC_PCB 	    *pcb;
    PTR	    	    *ptr;
    i4 	    saved_timeout;

    GCTRACE(3)("GClu62posted: IN\n");

    /* Get the parm list associated with this receive */
    ptr = (PTR *)(receiveandpost + 1);
    parms = (GCC_P_PLIST *) *ptr;

    /* Get the pcb and dcb */
    pcb = (GC_PCB *) parms->pcb;
    dcb = (GC_DCB *) parms->pce->pce_dcb;

    GCTRACE(3) ("GClu62posted: pcb %x\n", pcb);

    pcb->conv_state = AP_PEND_POST_STATE;

    /* poll to reenter protocol machine */
    saved_timeout = pcb->timeout;
    pcb->timeout = 0;
    GClu62timer(dcb, pcb);
    pcb->timeout = saved_timeout;

    GCTRACE(4)("GClu62posted: OUT\n");
}


static VOID GClu62getcid(pcb, state, tpid, cvid)
GC_PCB *pcb;
i4  state;
unsigned char *tpid;
u_long        *cvid;
{

    if (pcb->duplex != DUPLEX)
    {
    	MEcopy((PTR) pcb->tp_id, (u_i4) sizeof(pcb->tp_id), tpid);
	*cvid  = pcb->conversation_id;
        return;
    }

    /* duplex conversations */
    if (AP_SEND_STATE == state)
    {
        if (pcb->invoked_tp)
        {
    	    MEcopy((PTR) pcb->tp_id, (u_i4) sizeof(pcb->tp_id), tpid);
	    *cvid  = pcb->conversation_id;
        }
        else
        {
    	    MEcopy((PTR) pcb->tp_id, (u_i4) sizeof(pcb->tp_id), tpid);
	    *cvid  = pcb->conversation_id_2;
        }
    }
    else if (AP_RECEIVE_STATE == state)
    {
        if (pcb->invoked_tp)
        {
    	    MEcopy((PTR) pcb->tp_id_2, (u_i4) sizeof(pcb->tp_id), tpid);
	    *cvid  = pcb->conversation_id_2;
        }
        else
        {
    	    MEcopy((PTR) pcb->tp_id, (u_i4) sizeof(pcb->tp_id), tpid);
	    *cvid  = pcb->conversation_id;
        }
    }
    return;
}

/*
** Name: GClu62lpcbs - link conversation ids of a duplex conversation
**
** Description:
**
** Inputs:
**	PCB pointers to be linke
**
** Returns:
**	address of linked pcb
**
** History
**      18-Mar-96 (tlong)
**          Created.
*/
static GC_PCB *GClu62lpcbs(pcb, lpcb)
GC_PCB *pcb;
GC_PCB *lpcb;
{

    GCTRACE(4)("\nlinking %x to %x\n", pcb, lpcb);

    lpcb->conversation_id_2 = pcb->conversation_id;
    MEcopy((PTR) pcb->tp_id, (u_i4) sizeof(pcb->tp_id), lpcb->tp_id_2);
    lpcb->duplex = DUPLEX;
  
    /* Free up pcb */
    if (MEfree((PTR)pcb) != OK)
    {
        GCTRACE(1)("MEfree failed pcb: %x\n", pcb);
    }
    return lpcb;
}


/*
** Name: GClu62gettime - get time in milliseconds
**
** Description:
**
** Inputs:
**	Nothing
**
** Returns:
**	current time in milliseconds
**
** History
**      18-Mar-96 (tlong)
**          Created.
*/
static i4 GClu62gettime()
{
    struct timeval newtod;
    struct timezone newzone;
    i4 	    time_msec;

    /* Take snapshot of time */
    gettimeofday(&newtod, &newzone);
    time_msec = ((newtod.tv_sec * 1000) + (newtod.tv_usec) / 1000); 

    return (time_msec);
}



/* debugging orphaned pcb code */

#ifdef ORPHANED
VOID cleart()
{
    int i;
    for (i=0;i<TSIZE;i++)
    {
        pcbt[i] = 0;
        st[i] = 0;
    }
}

VOID storepcb(p)
GC_PCB *p;
{
    int i;
    for (i=0;i<TSIZE;i++)
    {
        if (pcbt[i] == 0)
        {
            pcbt[i] = p;
            st[i] = GClu62gettime();
            break;
        }
    }
}

#define MINT 6*60*1000   /* inactivity of six minutes, change as required */

VOID clearpcb(p)
GC_PCB *p;
{
    int i;

    for (i=0;i<TSIZE;i++)
    {
        if (pcbt[i] == p)
        {
            pcbt[i] = 0;
            st[i] = 0;
        }
        else if (pcbt[i] != 0)
        {
            if ((GClu62gettime() - st[i]) > MINT)
            { 
                GCTRACE(1)("ORPHAN pcb = %x\n", pcbt[i]);
            }
        }
    }
    return;
}

#endif
/* end debugging code */

#endif /* xCL_HP_LU62_EXISTS */

