/*#define ORPHANED uncomment to activate orphaned conversation code*/
/*
**Copyright (c) 2004 Ingres Corporation
*/

#include	<compat.h>
#include 	<errno.h>
#include	<gl.h>
#include    	<systypes.h>
#include        <clpoll.h>
#include	<er.h>
#include	<gcccl.h>
#include	<cm.h>
#include    	<me.h>
#include	<nm.h>
#include    	<st.h>
#include	<gcatrace.h>
#include	<clconfig.h>
#include    	<bsi.h>
#include        <lo.h>
#include        <pm.h>
#include        <tr.h>

#include        <sys/time.h>

#ifdef xCL_SUN_LU62_EXISTS

/* 
** appc also defines "OK", luckily to the sane value we define it
** to. We undefine OK to avoid compiler warnings etc.
*/
#undef OK

#if defined(su4_us5) || defined(sui_us5)
#include    	"/opt/SUNWconn/snap2p/p2p_lib/include/api_const.h"
#include    	"/opt/SUNWconn/snap2p/p2p_lib/include/api.h"
#else
#include    	"/usr/sunlink/snap2p/p2p_lib/api_const.h"
#include    	"/usr/sunlink/snap2p/p2p_lib/api.h"
#endif /* su5_us5 sui_us5 */

/* External variables */

/*
** Forward functions
*/

static VOID GClu62sm();
static VOID GClu62er();
static STATUS GClu62name();
static VOID GClu62reg();
static VOID GClu62fdreg();
static VOID GClu62read();
static VOID GClu62timer();
static VOID GClu62timeout();
static VOID GClu62getcid();
static i4 GClu62gettime();
static STATUS GClu62alloc2();

/*
** local variables
*/

static u_char GClu62_version = 1;

static char localbuf[8];
static i4  halfplex = 0;



#define DEFAULT_TIMER_S 100                /* ms: time to prepare to receive 
    	    	    	    	    	      after last operation was send */

#define DEFAULT_TIMER_R 2000
#define MAX_TIMER_R 128000                 /* ms: time to prepare to receive
    	    	    	    	    	      after last operation was 
    	    	    	    	    	      receive */

#define DEFAULT_TIMER_W 500
static i4  GClu62_timer_w = DEFAULT_TIMER_W;
                                           /* ms: max time before we do an
    	    	    	    	    	      appc_wait */

#define DEFAULT_TIMER_B 4000
static i4  GClu62_timer_b = DEFAULT_TIMER_B;
    	    	    	    	    	   /* ms: time before we do an
    	    	    	    	    	      appc_wait when running in
    	    	    	    	    	      background mode; ie with a
     	    	    	    	    	      listen posted but no active
    	    	    	    	    	      conversations */

/* Per connection control block */

typedef struct _GC_PCB
{
    	GCC_P_PLIST    	*send_parm_list;    /* Send parameter list */
    	GCC_P_PLIST    	*rcv_parm_list;     /* Receive parameter list */
   	i4 	    	conv_state;         /* State of connection */
    	u_long	    	conv_id;            /* Id of conversation */
    	u_long	    	conv_id2;           /* Id of conversation (2) */
    	i4 	timeout;            /* Current timeout period */
        i4         start_msec;
        i4         stop_msec;
    	bool 	    	registered; 	    /* registered for event? */
    	bool	    	timer_running;      /* timer running? */
    	i4 	    	total_send; 	    /* total number of sends rqstd */
    	i4 	    	send_in_conv_rcv;   /* sends when in CONV_RCV state */
    	i4 	    	total_rcv;  	    /* total number of rcvs rqstd */
    	i4 	    	rcv_in_conv_send;   /* rcvs when in CONV_SEND state */
        i4              atrys;              /* allocation retries */
        bool            invoked;            /* invoked? or invoking*/
        char            duplex;             /* halfplex, penddup or duplex */ 
#define IDLEN 4
        char            pkey[IDLEN];        /* id to be returned to server */
} GC_PCB;

/* Per driver control block */

typedef struct _GC_DCB
{
    	char	    	gateway_name[MAX_TP_NAME + 1]; 
    	u_long	    	gateway_fd; 	    /* fd of gateway socket */
    	char	    	tp_name[MAX_TP_NAME + 1];
    	u_long	    	tp_id;              /* id of our tp */
    	u_long	    	listen_conversation;
    	short	    	conversations;	    /* total number of convs */
    	bool	    	listen_posted;      /* listen_posted? */
    	WAIT	    	appc_wait_parms;
    	GCC_P_PLIST 	*reg_parm_list[MAX_WAIT_LIST];
    	    	    	    	            /* parms associated with
    	    	    	    	    	       registered operation */
    	short           number_of_timers;   /* number of timers running */
    	GC_PCB          *tim_pcb[MAX_WAIT_LIST];
    	    	    	    	    	    /* pcb's of timer conversations */
    	i4         tim_expiration[MAX_WAIT_LIST];
    	    	    	    	    	    /* time to expiration */
    	struct timeval  timtod;             /* snapshot of wallclock time */
} GC_DCB;


static GC_PCB *GClu62lpcbs();
static GC_PCB *GClu62newpcb();

static i4  GClu62_trace = 0;

#define GCTRACE(n) if (GClu62_trace >= n) (void)TRdisplay

/* Actions in GClu62sm. */

#define	GC_REG		0	/* open listen port */
#define	GC_LSN		1	/* wait for inbound connection */
#define GC_LSNIND       2   	/* LISTEN indication */
#define GC_LSNCMP  	3	/* LISTEN completes */
#define GC_LSNTKN   	4   	/* listen wait for data token */
#define	GC_CONN  	5	/* connect to host */
#define GC_CONNCMP      6       /* CONNECT completes */
#define GC_CONN_ALLOC2  7       /* CONNECT allocate 2nd session for
                                   pseudo-duplex connection */
#define GC_CONNTKN  	8   	/* connect wait for data token */
#define	GC_SND  	9	/* send TPDU */
#define GC_SNDTKN   	10  	/* send wait for data token */
#define	GC_SNDCMP	11	/* SEND completes */
#define	GC_RCV		12	/* receive TPDU */
#define	GC_RCVCMP	13	/* RECEIVE completes */
#define	GC_DISCONN	14	/* close a connection */

/* Action names for tracing. */

static char *gc_names[] = { 
	"REG", "LSN", "LSNIND", "LSNCMP", "LSNTKN", "CONN", "CONNCMP",
	"CONN_ALLOC2", "CONNTKN", "SND", "SNDTKN", "SNDCMP", "RCV",
        "RCVCMP", "DISCONN"
};

/* Operation for GClu62reg (others are defined in appc header file) */

#define CANCEL 127

/* Length of Connect message */

#define CONNECT_LENGTH 80

/* debugging orphaned pcb prototypes and table */
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
**    19-Aug-91 (cmorris)
**	Shell copied from gccasync.
**    08-Apr-92 (cmorris)
**  	Reworked handling of appc's wait list
**    08-Apr-92 (cmorris)
**  	Added checks for exceeding max. connections we can support
**    29-Apr-92 (cmorris)
**  	Don't assume that data token comes with data during listen and
**  	connect handshakes:- some platforms may send token separately.
**    30-Apr-92 (cmorris)
**  	If there's no receive posted to us, we are in receive state _and_
**  	we need to do a send, poll for the data token.
**    30-Apr-92 (cmorris)
**  	Should be checking for resource error UNSUCCESSFUL_RETRY,
**  	not UNSUCCESSFUL.
**    01-May-92 (cmorris)
**  	If we get a fatal error from appc_wait, reenter the state machine
**  	for each conversation so that each completed with an error.
**    08-May-92 (cmorris)
**  	Updated for Peer-to-peer 7.0.
**    21-May-92 (cmorris)
**  	Peek for listen completion rather than risk being blocked.
**    26-May-92 (cmorris)
**  	Can't use delayed session allocation right now:- if this option is 
**  	used, the Gateway never returns any errors when session are not
**  	allocated!!
**    07-Jul-92 (cmorris)
**  	Added support for calling cnos when we're dealing with parallel
**  	sessions.
**     23-jul-92 (kevinm)
**      ifdef xCL_SUN_LU62_EXISTS for most of the file in order to fix
**      problem with building for those machines that don't use SUN_LU62.
**    21-Sep-92 (cmorris)
**  	When we complete a disconnect, decrement, not increment, the count 
**  	of extant conversations!!
**    21-Sep-92 (cmorris)
**      On listen or connect, increment conversation count before any 
**  	possibility of error, so that a subsequent disconnect decrements
**  	the count (correctly) back to its original value.
**    22-Sep-92 (cmorris)
**  	Added support for reduced (background) polling rate for appc_wait
**  	when there is a listen posted but no active conversations.
**    02-Oct-92 (cmorris)
**  	Use the new SUB_WITHIN_SESSION_LIMIT flag to allocate to avoid
**  	blocking on dependent LUs in cases where the session is already
**  	in use.
**    09-Oct-92 (cmorris)
**  	Updated for "implied cnos" functionality.
**    13-Oct-92 (cmorris)
**  	Allow background polling rate to be specified by environment
**  	variable.
**    20-Oct-92 (cmorris)
**  	Improved error logging by including the identifier if the API
**  	function that caused the error.
**    21-Oct-92 (cmorris)
**  	When we are going to poll for data token, make sure completion
**  	of a receive hasn't already picked it up.
**    23-Oct-92 (cmorris)
**  	Remove registering of file descriptor for write and its attendant 
**  	callback:- its never used.
**    23-Oct-92 (cmorris)
**  	Get rid of counts for registered readers and writers. They were
**  	only used (and incorrectly!) for determining whether to register
**  	a file descriptor with clpoll.
**    14-dec-92 (peterk)
**	reverse order of includes of <sys/time.h> and <systypes.h> so that
**	<systypes.h> comes first.  On solaris 2.1, sys/time.h includes
**	sys/types.h and therefore gets compiler errors when its include
**	follows compat.h
**    17-Dec-92 (gautam)
**      Changed GClu62name() to use PMget().
**    30-dec-92 (mikem)
**	su4_us5 port.  Reversed order of #include's of systypes.h and time.h
**	to get rid of acc compile errors complaining about invalid type of
**	u_char ...
**    08-Jan-92 (cmorris)
**  	Timer mechanism enhancements.
**    18-Jan-93 (edg)
**  	Try to get trace symbol from PM if not found from NM.
**    26-Jan-93 (cmorris)
**  	Enhancements to algorithm for handling data token:- 
**  	(i) degrade the frequency with which we give up the token 
**  	after a receive to a low background rate; and (ii) when
**  	we get the token back after a send, check for an outstanding
**  	send and if there isn't one, hand the token right back.
**    01-Feb-93 (cmorris)
**  	If a receive compltes without getting the data token and there is
**  	a send outstanding, make sure send is re-registered for read completion
**  	to enable it to subsequently pick up data token.
**    01-Feb-93 (cmorris)
**  	Don't store listen and connect parameter lists in pcb, there's no 
**  	need to do so.
**    02-Feb-93 (cmorris)
**  	Make all errors go through GClu62er().
**    03-Feb-93 (cmorris)
**  	Added GClu62fdreg() to centralize registering of file descriptor
**  	and the algorithm for deducing the timeout to be used.
**    19-Feb-93 (cmorris)
**  	Added description of token-passing protocol.
**    22-Feb-93 (cmorris)
**  	Null out pcb pointer in parameter list when pcb is freed.
**    04-Mar-93 (cmorris)
**      Fix PM error handling to call GClu62er, not GClu62reg!!
**    16-mar-93 (smc)
**	Moved <systypes.h> to after <compat.h> as this fixes warnings on
**	axp_osf.
**    29-Jun-93 (cmorris)
**  	Flush on send instead of doing flush separately. This saves an
**  	interaction with the gateway for each send.
**    15-jul-93 (ed)
**	adding <gl.h> after <compat.h>
**    12-apr-95 (brucel)
**	For Solaris, SNA link headers from vendor (SUN) are in a different
**      path than for SunOS.
**    06-jul-95 (canor01)
**	include <cs.h> for CS_SEMAPHORE definition. add "#ifdef MCT"
**    06-jul-95 (canor01)
**	include <errno.h>
**    10-nov-1995 (murf)
**      Added sui_us5 to all areas specifically defined with su4_us5.
**      Reason, to port Solaris for Intel.
**    02-apr-1996 (kch)
**	Change the unconditional calls to gettimeofday() to be conditional
**	on the xCL_GETTIMEOFDAY_TIMEONLY and xCL_GETTIMEOFDAY_TIME_AND_TZ
**	capability.
**    03-jun-1996 (canor01)
**	Remove MCT semaphores, which were doing little to make the code
**	MT-safe.
**	19-sep-96 (nick)
**	    usl_us5 defines xCL_GETTIMEOFDAY_TIME_AND_TZ now so the frig
**	    in here isn't required.
**    18-Mar-97 (tlong)
**      Added code to support dual (pseudo-duplex) conversations.
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
    **  Client is always remote.
    */
    parms->options = 0;

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
    }

    GCTRACE(1)("GClu62: %p operation %s\n", parms,
    	gc_names[ parms->state ] );

    parms->generic_status = OK;
    CL_CLEAR_ERR( &parms->system_status );

    /* Start sm. */

    GClu62sm( parms );
    return OK;
}

static VOID
GClu62sm( parms )
GCC_P_PLIST	*parms;
{
    register	GC_PCB		*pcb = (GC_PCB *)parms->pcb;
    GC_DCB			*dcb = (GC_DCB *)parms->pce->pce_dcb;
    int		rc;
    char        *nm;
    CL_ERR_DESC system_status;
    char    	*trace;
    char    	*halfplexc;
    MC_SEND_DATA mc_send_data_parms;
    MC_RECEIVE_IMMEDIATE mc_receive_immediate_parms;
    MC_RECEIVE_AND_WAIT mc_recv_and_wait;
    MC_GET_ATTRIBUTES mc_get_attributes_parms;

 top:
    GCTRACE(2)("GClu62sm: %p state %s\n", parms, gc_names[ parms->state ] );

    switch( parms->state )
    {

    case GC_REG:
    {
    	TP_START tp_start_parms;
    	TP_LISTEN tp_listen_parms;
    	int i;

	/*
	** Register before the first listen.
	*/


#ifdef ORPHANED
        cleart();
#endif

	NMgtAt( "II_GCCL_TRACE", &trace );
	if( !( trace && *trace )
	    && PMget( "!.gccl_trace_level", &trace ) != OK )
	{
		GClu62_trace  = 0;
	}
	else
	{
		GClu62_trace  = atoi( trace );
	}

        NMgtAt( "II_HALF_DUPLEX", &halfplexc);
        if (halfplexc && *halfplexc)
        {
            halfplex  = atoi(halfplexc);
        }

        GCTRACE(4)("TRACE = %d; NOT_DUP = %d\n", GClu62_trace, halfplex);

        /* Allocate per driver control block */
	dcb = (GC_DCB *)MEreqmem( 0, sizeof( *dcb ), TRUE, (STATUS *)0 );
	parms->pce->pce_dcb = (PTR)dcb;
        if( !dcb )
    	{
    	    GClu62er(parms, GC_OPEN_FAIL, 0, OK);
    	    goto complete;
    	}

    	dcb->conversations = 0;
    	dcb->listen_posted = FALSE;

    	/* Initialize wait list */
    	dcb->appc_wait_parms.nbio = (u_char) 1;
    	dcb->appc_wait_parms.number_of_conv_ids = 0;
    	for (i = 0; i <MAX_WAIT_LIST; i++)
    	    dcb->appc_wait_parms.conv_id_list[i] = 0;

    	/* Initialize timer list */
    	dcb->number_of_timers = 0;

    	/* Deduce our transaction program name */
    	if (GClu62name(parms->pce->pce_pid, parms->function_parms.open.port_id, 
             dcb->tp_name, dcb->gateway_name) != OK)
        {
	    GCTRACE(1)("GClu62sm:No TP name information in PM resource file\n");
    	    GClu62er(parms, GC_OPEN_FAIL, 0, OK);
            goto complete;
        }

    	STpolycat(3, dcb->gateway_name, ".", dcb->tp_name, 
    	    	  parms->function_parms.open.lsn_port);

    	/* Attach to gateway */
    	STcopy(dcb->tp_name, (char *)tp_start_parms.log_tp_name);
    	STcopy(dcb->gateway_name, (char *)tp_start_parms.gateway_name);
    	if(rc = tp_start(&tp_start_parms, dcb->gateway_name))
    	{
    	    /* Start failed; decipher error */
    	    GClu62er(parms, GC_OPEN_FAIL, VERB_TP_START, rc);
    	    goto complete;
    	}

    	/* Save gateway specific info. */
    	dcb->gateway_fd = tp_start_parms.gateway_fd;
    	dcb->tp_id = tp_start_parms.tp_id;

    	/* See whether this is a "no listen" protocol */

    	if(!parms->pce->pce_flags & PCT_NO_PORT)
    	{
	    /* Now post a listen to gateway */
            tp_listen_parms.tp_name_is_ascii = (u_char) 1;
    	    STcopy(dcb->tp_name, (char *)tp_listen_parms.tp_name);
	    tp_listen_parms.queue_depth = 7;
    	    tp_listen_parms.sync_level = SYNC_NONE;
    	    tp_listen_parms.conv_type = MAPPED_CONVERSATION;
    	    tp_listen_parms.security_required = SECURITY_REQ_NONE;
    	    if (rc = tp_listen(&tp_listen_parms))
	    {
		/* Listen failed; decipher error */
		GClu62er(parms, GC_OPEN_FAIL, VERB_TP_LISTEN, rc);
		goto complete;
	    }

    	    /* save the listen conversation id */
    	    dcb->listen_conversation = tp_listen_parms.listen_conv_id;
	}

    	/*
	** Registration completed.
	*/

	/* Note protocol is open and print message to that effect. */
	GCTRACE(1)("GClu62sm: Registered at [%s.%s]\n", 
    	           dcb->gateway_name, dcb->tp_name);
	goto complete;
    } /* end case GC_REG */

    case GC_LSN:
    {
	/*
	** Extend a listen to accept a connection. In LU6.2 terminology
	** this translates into doing a listen for an incoming allocate
    	** request.
	*/

	/* Allocate pcb */
        pcb = GClu62newpcb(parms,dcb);
        if (!pcb)
        {
    	    GClu62er(parms, GC_LISTEN_FAIL, 0, OK);
	    goto complete;
        }

    	/* Make sure we can handle another connection */
    	if (++dcb->conversations > MAX_WAIT_LIST)
	{
            dcb->conversations--;
    	    GClu62er(parms, GC_LISTEN_FAIL, 0, OK);
	    goto complete;
	}

    	/* Add conversation id to wait list */
    	dcb->listen_posted = TRUE;
	GClu62reg(dcb, pcb, (u_char) READABLE, parms);
	parms->state = GC_LSNIND;
	return;
    } /* end case GC_LSN */

    case GC_LSNIND:
    {
    	TP_WAIT_REMOTE_START tp_wait_remote_start_parms;
    	PEEK peek_parms;

    	/*
    	** Before trying to pick up the listen, check that
    	** there's something in the listen queue. We have to
    	** do this as we may be being driven because of some
    	** other problem in the Gateway, rather than there being
    	** a listen to pick up. tp_wait_remote_start (below), as
    	** its name suggests, is blocking. Sigh.
    	*/
    	dcb->listen_posted = FALSE;
    	peek_parms.conv_id = pcb->conv_id;
    	rc = peek(peek_parms);
    	switch (rc)
	{
	case OK:

	    /*
	    ** Pick up listen indication
	    */
    	    tp_wait_remote_start_parms.listen_conv_id = pcb->conv_id;
    	    tp_wait_remote_start_parms.pip_length = 0;
    	    tp_wait_remote_start_parms.pip_data = NULL;
    	    if(rc = tp_wait_remote_start(&tp_wait_remote_start_parms))
	    {
	    	/* Remote start failed; decipher error */
	    	GClu62er(parms, GC_LISTEN_FAIL, VERB_TP_WAIT_REMOTE_START, rc);
	    	goto complete;
	    }

    	    /* We picked up a listen. Save details of conversation */	    
    	    pcb->conv_id = tp_wait_remote_start_parms.conv_id;
	    pcb->conv_state = CONV_RCV;

    	    /* Get attributes of conversation */
    	    mc_get_attributes_parms.conv_id = pcb->conv_id;
    	    if(rc = mc_get_attributes(&mc_get_attributes_parms))
	    {
	    	/* Can't get attributes; decipher error */
	    	GClu62er(parms, GC_LISTEN_FAIL, VERB_MC_GET_ATTRIBUTES, rc);
	    	goto complete;
	    }

    	    /* Make sure confirmation wasn't requested */
    	    if (mc_get_attributes_parms.sync_level != SYNC_NONE)
	    {
	    	/* Don't support confirmation */
	    	GClu62er(parms, GC_LISTEN_FAIL, 0, OK);
	    	goto complete;
	    }

    	    /* Add conversation id to wait list */
    	    GClu62reg(dcb, pcb, (u_char) READABLE, parms);
	    parms->state = GC_LSNCMP;
	    return;

	case UNSUCCESSFUL_RETRY:

    	    /* no listen yet, readd conversation to wait list */
    	    dcb->listen_posted = TRUE;
    	    GClu62reg(dcb, pcb, (u_char) READABLE, parms);
    	    return;

    	default:
 
    	    /* indicate listen error */
	    GClu62er(parms, GC_LISTEN_FAIL, VERB_PEEK, rc);
	    goto complete;
	}

    } /* end case GC_LSNIND */

    case GC_LSNCMP:
    {
    	u_char connect_string[CONNECT_LENGTH];

        pcb->invoked = TRUE;

        /* connection successful, get the time */
        pcb->start_msec = GClu62gettime();

	/*
	** Completion of listen. Do a receive of initial send by peer
	*/

    	mc_receive_immediate_parms.conv_id = pcb->conv_id;
    	mc_receive_immediate_parms.type = SUB_NONE;
    	mc_receive_immediate_parms.length = CONNECT_LENGTH;
    	mc_receive_immediate_parms.data = connect_string;
    	rc = mc_receive_immediate(&mc_receive_immediate_parms);
    	switch (rc)
        {
	case OK:
    	    /* See what was received */
    	    switch (mc_receive_immediate_parms.what_received_data_type)
	    {
	    case WR_NONE:
    	    	/*
		** Check we didn't get the data token before we got
    	    	** the data!
    	    	*/
    	    	if (mc_receive_immediate_parms.what_received_flag_type ==
    	    	    WR_SEND)
		{
		    /* We did get the token so treat this as fatal error */
    	            GClu62er(parms, GC_LISTEN_FAIL, 0, OK);
    	    	    goto complete;
		}

    	    	/* No data yet; readd conversation to wait list */
        	GClu62reg(dcb, pcb, (u_char) READABLE, parms);
    	    	return;

	    case WR_DATA_COMPLETE:
    	    	/* 
    	    	** We don't care about version here. We're just going
    	    	** to send back the only thing we support...
    	    	*/

                if (connect_string[0] == GC_LU62_VERSION_1 || halfplex)
                {
                    pcb->duplex = NOT_DUP;
                }
                else if (connect_string[0] == GC_LU62_VERSION_2)
                {
                    pcb->duplex = PEND_DUP;
                }
                else if (connect_string[0] == GC_ATTN)
                {
    	            mc_recv_and_wait.conv_id = pcb->conv_id;
    	            mc_recv_and_wait.type = SUB_NONE;
    	            mc_recv_and_wait.length = CONNECT_LENGTH;
    	            mc_recv_and_wait.data = connect_string;
    	            rc = mc_receive_immediate(&mc_recv_and_wait);
                    if (rc != OK)
                    {
    	               GClu62er(parms, GC_LISTEN_FAIL, 0, OK);
    	    	       goto complete;
                    }
                    {
                      GC_PCB *spcb;
                      spcb = GClu62lpcbs(pcb,(GC_PCB *)&connect_string[0]);
                      if (spcb == (GC_PCB *)NULL)
                      {
                         GCTRACE(4)("Primary pcb no longer valid error! \n");
                         /* big trouble, main pcb is nolonger */
                         GClu62er(parms, GC_LISTEN_FAIL, 0, OK);
                         goto complete;
                      }
                      else if (spcb->duplex != PEND_DUP)
                      {
		         /* We got second request without first ? */
                         /* this might be overkill */
    	                 GClu62er(parms, GC_LISTEN_FAIL, 0, OK);
    	    	         goto complete;
                      }
                      else
                      {
                          pcb = spcb;
                      }
                    } /* end pseudo block */

                    pcb->duplex = DUPLEX;
                    goto complete;        /* this conversation stays in recv */
                }
                
    	    	/* Check to make sure peer gave us the data token */
    	    	if (mc_receive_immediate_parms.what_received_flag_type !=
    	    	    WR_SEND)
		{
		    /* We didn't get token so wait for it before completing */
    	            GClu62reg(dcb, pcb, (u_char) READABLE, parms);
    	    	    parms->state = GC_LSNTKN;
    	    	    return;
		}

    	    	pcb->conv_state = CONV_SEND;
    	    	break;

    	    case WR_DATA_INCOMPLETE:
    	    	/* 
    	    	** Looks like we got a larger record than we can cope with.
    	    	** Treat this as a fatal error by dropping through to the
    	    	** default.
    	    	*/

    	    default:
    	        GClu62er(parms, GC_LISTEN_FAIL, 0, OK);
    	    	goto complete;
	    }

    	    break;

    	case UNSUCCESSFUL_RETRY:
    	    /* nothing to receive yet, readd conversation to wait list */
    	    GClu62reg(dcb, pcb, (u_char) READABLE, parms);
    	    return;

    	default:
	    /* receive failed, indicate listen error */
	    GClu62er(parms, GC_LISTEN_FAIL, VERB_MC_RECEIVE_IMMEDIATE, rc);
	    goto complete;
	}

    	/* 
    	** Do a send containing protocol version number and 
    	** indicate that we're prepared to receive again.
        */
    	mc_send_data_parms.conv_id = pcb->conv_id;
    	mc_send_data_parms.type = SUB_PREPARE_TO_RECEIVE_FLUSH;
    	mc_send_data_parms.nbio = 1;
        if (pcb->duplex == PEND_DUP)
        {
    	    /* this conversatin stays in send */
    	    mc_send_data_parms.type = SUB_FLUSH;
            connect_string[0] = GC_LU62_VERSION_2;
            MEcopy((char *)pcb, IDLEN, (char *)&connect_string[1]);
    	    mc_send_data_parms.length = 5;
        }
        else
        {
            connect_string[0] = GC_LU62_VERSION_1;
    	    mc_send_data_parms.length = 1;
        }
        mc_send_data_parms.data = connect_string;
    	mc_send_data_parms.map_name[0] = '\0';
    	if (rc = mc_send_data(&mc_send_data_parms))
	{
	    /* send failed, indicate connect error */
	    GClu62er(parms, GC_LISTEN_FAIL, VERB_MC_SEND_DATA, rc);
	    goto complete;
	}
	    
    	pcb->conv_state = CONV_RCV;

    	/* Drive completion exit */
	goto complete;
    } /* end case GC_LSNCMP */

    case GC_LSNTKN:
    {
    	u_char connect_string[CONNECT_LENGTH];

	/*
	** Waiting for data token before completing listen. Do a receive
    	** to try to pick it up.
	*/

    	mc_receive_immediate_parms.conv_id = pcb->conv_id;
    	mc_receive_immediate_parms.type = SUB_NONE;
    	mc_receive_immediate_parms.length = 0;
    	mc_receive_immediate_parms.data = (u_char *) NULL;
    	rc = mc_receive_immediate(&mc_receive_immediate_parms);
    	switch (rc)
        {
	case OK:
    	    /* See what was received */
    	    switch (mc_receive_immediate_parms.what_received_data_type)
	    {
	    case WR_NONE:
		/* Check for the data token */
    	    	if (mc_receive_immediate_parms.what_received_flag_type ==
    	    	    WR_SEND)
		{
		    /* We did get the token so move into send state */
    	    	    pcb->conv_state = CONV_SEND;
    	    	    break;
		}

    	    	/* No token yet; readd conversation to wait list */
        	GClu62reg(dcb, pcb, (u_char) READABLE, parms);
    	    	return;

	    case WR_DATA_COMPLETE:
    	    case WR_DATA_INCOMPLETE:
		/* 
    	    	** As we're waiting for the token, we shouldn't have
    	    	** gotten any data here. Just drop through to default.
    	    	*/

    	    	if (mc_receive_immediate_parms.what_received_flag_type ==
    	    	    WR_SEND)
        	    pcb->conv_state = CONV_SEND;

    	    default:
    	        GClu62er(parms, GC_LISTEN_FAIL, 0, OK);
    	    	goto complete;
	    }

    	    break;

    	case UNSUCCESSFUL_RETRY:
    	    /* nothing to receive yet, readd conversation to wait list */
    	    GClu62reg(dcb, pcb, (u_char) READABLE, parms);
    	    return;

    	default:
	    /* receive failed, indicate listen error */
	    GClu62er(parms, GC_LISTEN_FAIL, VERB_MC_RECEIVE_IMMEDIATE, rc);
	    goto complete;
	}

    	/* 
    	** Do a send containing protocol version number and 
    	** indicate that we're prepared to receive again.
        */
    	mc_send_data_parms.conv_id = pcb->conv_id;
    	mc_send_data_parms.type = SUB_PREPARE_TO_RECEIVE_FLUSH;
    	mc_send_data_parms.nbio = 1;
        if (pcb->duplex == PEND_DUP)
        {
    	    /* this conversatin stays in send */
    	    mc_send_data_parms.type = SUB_FLUSH;
            connect_string[0] = GC_LU62_VERSION_2;
            MEcopy((char *)pcb, IDLEN, (char *)&connect_string[1]);
    	    mc_send_data_parms.length = 5;
        }
        else
        {
            connect_string[0] = GC_LU62_VERSION_1;
    	    mc_send_data_parms.length = 1;
        }
        mc_send_data_parms.data = connect_string;
    	mc_send_data_parms.map_name[0] = '\0';
    	if (rc = mc_send_data(&mc_send_data_parms))
	{
	    /* send failed, indicate connect error */
	    GClu62er(parms, GC_LISTEN_FAIL, VERB_MC_SEND_DATA, rc);
	    goto complete;
	}
	    
    	pcb->conv_state = CONV_RCV;

    	/* Drive completion exit */
	goto complete;
    } /* end case GC_LSNTKN */

    case GC_CONN:
    {
    	MC_ALLOCATE mc_allocate_parms;
	u_char buf[1];
	if (halfplex)
	    buf[0] = GC_LU62_VERSION_1;
        else
	    buf[0] = GC_LU62_VERSION_2;


	/* Allocate pcb */
        pcb = GClu62newpcb(parms,dcb);
        if (!pcb)
        {
    	    GClu62er(parms, GC_CONNECT_FAIL, 0, OK);
	    goto complete;
	}

    	/* Make sure we can handle another connection */
    	if (++dcb->conversations > MAX_WAIT_LIST)
	{
            dcb->conversations--;
    	    GClu62er(parms, GC_CONNECT_FAIL, 0, OK);

    	    /* Free up pcb */
	    MEfree( (PTR)pcb );

	    goto complete;
	}

#ifdef ORPHANED
        storepcb(pcb);
#endif

    	/* Allocate conversation */

    	/* 
    	** The unique session name (which identifies both the local and
    	** partner lu's) is in the node_id connect parameter; the tp_name
    	** is in the port_id connect parameter.
    	*/
    	STcopy(parms->function_parms.connect.node_id, 
    	       (char *)mc_allocate_parms.unique_session_name);
    	mc_allocate_parms.tp_name_is_ascii = 1;
    	STcopy(parms->function_parms.connect.port_id,
               (char *)mc_allocate_parms.tp_name);
    	mc_allocate_parms.type = SUB_WITHIN_SESSION_LIMIT;
    	mc_allocate_parms.sync_level = SYNC_NONE;
    	mc_allocate_parms.security_select = SECURITY_NONE;
    	mc_allocate_parms.pip_length = 0;
    	mc_allocate_parms.pip_data = NULL;
    	if (rc = mc_allocate(&mc_allocate_parms))
        {
    	    /* Some error occurred */
	    GClu62er(parms, GC_CONNECT_FAIL, VERB_MC_ALLOCATE, rc);
    	    goto complete;
	}

    	/* Allocate ok; save details of conversation */
    	pcb->conv_id = mc_allocate_parms.conv_id;
    	pcb->conv_state = CONV_SEND;

    	/* 
    	** Do a send containing protocol version number and 
    	** indicate that we're prepared to receive
        */
    	mc_send_data_parms.conv_id = pcb->conv_id;
    	mc_send_data_parms.type = SUB_PREPARE_TO_RECEIVE_FLUSH;
    	mc_send_data_parms.nbio = 1;
    	mc_send_data_parms.length = 1;
    	mc_send_data_parms.data = buf;                 
    	mc_send_data_parms.map_name[0] = '\0';
    	if (rc = mc_send_data(&mc_send_data_parms))
	{
	    /* send failed, indicate connect error */
	    GClu62er(parms, GC_CONNECT_FAIL, VERB_MC_SEND_DATA, rc);
	    goto complete;
	}
	    
    	pcb->conv_state = CONV_RCV;

    	/* Add conversation id to wait list */
    	GClu62reg(dcb, pcb, (u_char) READABLE, parms);
	parms->state = GC_CONNCMP;
	return;
    } /* end case GC_CONN */

    case GC_CONNCMP:
    {
    	u_char connect_string[CONNECT_LENGTH];

        /* connection successful, get the time */
        pcb->start_msec = GClu62gettime();

	/*
	** Completion of connect. Do a receive of response to allocate
    	** request.
	*/

    	mc_receive_immediate_parms.conv_id = pcb->conv_id;
    	mc_receive_immediate_parms.type = SUB_NONE;
    	mc_receive_immediate_parms.length = CONNECT_LENGTH;
    	mc_receive_immediate_parms.data = connect_string;
    	rc = mc_receive_immediate(&mc_receive_immediate_parms);
    	switch (rc)
        {
	case OK:
    	    /* See what was received */
    	    switch (mc_receive_immediate_parms.what_received_data_type)
	    {
	    case WR_NONE:
    	    	/*
		** Check we didn't get the data token before we got
    	    	** the data!
    	    	*/
    	    	if (mc_receive_immediate_parms.what_received_flag_type ==
    	    	    WR_SEND)
		{
		    /* We did get the token so treat this as fatal error */
    	            GClu62er(parms, GC_CONNECT_FAIL, 0, OK);
    	    	    pcb->conv_state = CONV_SEND;
    	    	    goto complete;
		}

    	    	/* No data yet; readd conversation to wait list */
        	GClu62reg(dcb, pcb, (u_char) READABLE, parms);
    	    	return;

	    case WR_DATA_COMPLETE:
		/* 
    	    	** See what version number we got back. If it's not the
    	    	** only one we support, generate connect error.
    	    	*/
                {
    	    	if ((halfplex && mc_receive_immediate_parms.length != 1) ||
		   (mc_receive_immediate_parms.length != 5))
		{
    	            GClu62er(parms, GC_CONNECT_FAIL, 0, OK);
    	    	    goto complete;
		}
                /* Got duplex when we requested halfplex?, blow it off */
    	    	if (connect_string[0] == GC_LU62_VERSION_2 && halfplex)
		{
    	            GClu62er(parms, GC_CONNECT_FAIL, 0, OK);
    	    	    goto complete;
		}

                /* check for version 2 or 1; anything else is an error */
    	    	if (connect_string[0] == GC_LU62_VERSION_2)
		{
                    /* save key */
                    MEcopy((char *)&connect_string[1], IDLEN, pcb->pkey);
		    parms->state = GC_CONN_ALLOC2;
		    pcb->atrys = 0;
		    goto top;
		}
    	    	else if (connect_string[0] != GC_LU62_VERSION_1)
                {
    	            GClu62er(parms, GC_CONNECT_FAIL, 0, OK);
    	    	    goto complete;
                }

    	    	/* 
    	    	** We successfully negotiated a protocol version. Check 
    	    	** to make sure peer gave us the data token.
    	    	*/
    	    	if (mc_receive_immediate_parms.what_received_flag_type !=
    	    	    WR_SEND)
		{
		    /* We didn't get token so wait for it before completing */
    	    	    GClu62reg(dcb, pcb, (u_char) READABLE, parms);
    	    	    parms->state = GC_CONNTKN;
    	    	    return;
		}

    	    	pcb->conv_state = CONV_SEND;

    	    	/* Set prepare-to-receive timer */
    	    	GClu62timer(dcb, pcb);
    	    	break;
                }

    	    case WR_DATA_INCOMPLETE:
    	    	/* 
    	    	** Looks like we got a larger record than we can cope with.
    	    	** Treat this as a fatal error by dropping through to the
    	    	** default. 
    	    	*/

    	    default:
    	        GClu62er(parms, GC_CONNECT_FAIL, 0, OK);
    	    	goto complete;
	    }

    	    break;

    	case UNSUCCESSFUL_RETRY:
    	    /* nothing to receive yet, readd conversation to wait list */
    	    GClu62reg(dcb, pcb, (u_char) READABLE, parms);
    	    return;

    	default:
	    /* receive failed, indicate connect error */
	    GClu62er(parms, GC_CONNECT_FAIL, VERB_MC_RECEIVE_IMMEDIATE, rc);
	    goto complete;
	}

    	/* Drive completion exit */
	goto complete;
    } /* end case GC_CONNCMP */

    case GC_CONN_ALLOC2:
    {
        GCTRACE(1)("Second Allocate try: %d\n", ++pcb->atrys);

        /*  allocate the second (receive side) conversation */
        if (GClu62alloc2(parms) == OK)
        {
            GCTRACE(4)("pseudo duplex conv allocated\n");
            dcb->conversations++;
            GCTRACE(1)("Active conversations: %d\n", dcb->conversations);
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


    case GC_CONNTKN:
    {

	/*
	** Waiting for data token before completing connect. Do a receive 
    	** to try to pick it up.
	*/

    	mc_receive_immediate_parms.conv_id = pcb->conv_id;
    	mc_receive_immediate_parms.type = SUB_NONE;
    	mc_receive_immediate_parms.length = 0;
    	mc_receive_immediate_parms.data = (u_char *) NULL;
    	rc = mc_receive_immediate(&mc_receive_immediate_parms);
    	switch (rc)
        {
	case OK:
    	    /* See what was received */
    	    switch (mc_receive_immediate_parms.what_received_data_type)
	    {
	    case WR_NONE:
		/* Check for the data token */
    	    	if (mc_receive_immediate_parms.what_received_flag_type ==
    	    	    WR_SEND)
		{
		    /* We did get the token so drive completion */
    	    	    pcb->conv_state = CONV_SEND;
    	    	    goto complete;
		}

    	    	/* No token yet; readd conversation to wait list */
        	GClu62reg(dcb, pcb, (u_char) READABLE, parms);
    	    	return;

	    case WR_DATA_COMPLETE:
    	    case WR_DATA_INCOMPLETE:
		/* 
    	    	** As we're waiting for the token, we shouldn't have
    	    	** gotten any data here. Just drop through to default.
    	    	*/

    	    	if (mc_receive_immediate_parms.what_received_flag_type ==
    	    	    WR_SEND)
        	    pcb->conv_state = CONV_SEND;

    	    default:
    	        GClu62er(parms, GC_CONNECT_FAIL, 0, OK);
    	    	goto complete;
	    }

    	    break;

    	case UNSUCCESSFUL_RETRY:
    	    /* nothing to receive yet, readd conversation to wait list */
    	    GClu62reg(dcb, pcb, (u_char) READABLE, parms);
    	    return;

    	default:
	    /* receive failed, indicate connect error */
	    GClu62er(parms, GC_CONNECT_FAIL, VERB_MC_RECEIVE_IMMEDIATE, rc);
	    goto complete;
	}

    } /* end case GC_CONNTKN */

    case GC_SND:
    {
    	MC_REQUEST_TO_SEND mc_request_to_send_parms;

	GCTRACE(2)("GClu62sm: pcb %p conv state %d send size %d\n", 
                   pcb, pcb->conv_state, parms->buffer_lng );

	/*
	** Check to see if we are in SEND state. If we are not, we will
    	** have to hold on to the send until we are.
	*/
        if (pcb->conv_state == CONV_SEND || pcb->duplex == DUPLEX)
        {
    	    /* We're in send state so go ahead and try send */
    	    pcb->send_parm_list = parms;
    	    parms->state = GC_SNDCMP;
    	    goto top;
        }
        else if (pcb->conv_state == CONV_RCV)
        {
    	    /* 
    	    ** Can't do send right now. Send a Request-to-Send to peer
    	    ** and save our send until we're redriven in
    	    ** this state when we move into conversation receive state.
    	    */
    	    pcb->send_in_conv_rcv++;
    	    mc_request_to_send_parms.conv_id = pcb->conv_id;
    	    if (rc = mc_request_to_send(&mc_request_to_send_parms))
	    {
    	    	/* We couldn't request token so treat this as fatal error */
    	        GClu62er(parms, GC_SEND_FAIL, VERB_MC_REQUEST_TO_SEND, rc);
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
    	    if (pcb->rcv_parm_list)
	    {
		parms->state = GC_SNDCMP;
    	    	return;
	    }
    	    else
	    {
		/* No receive posted, poll for data token */
    	    	parms->state = GC_SNDTKN;
    	    	goto top;
	    }
        }
        else
        {
     	    /*default:*/
    	    /* Any other state is an error */
    	    GClu62er(parms, GC_SEND_FAIL, 0, OK);
    	    goto complete;
	}
	/*return; NOT REACHED */
    } /* end case GC_SND */

    case GC_SNDTKN:
    {

    	/* 
    	** Make sure we're still in receive state. It may be that we're
    	** being driven by the completion of a receive that picked up the
    	** send token and thus has already moved connection into send state.
    	** In that case we can merely redrive send.
    	*/
    	if (pcb->conv_state == CONV_SEND)
	{
	    /* We did already get the token so redrive send */
    	    parms->state = GC_SNDCMP;
    	    goto top;
	}

	/*
	** Waiting for data token before completing send. Do a receive
    	** to try to pick it up.
	*/

    	mc_receive_immediate_parms.conv_id = pcb->conv_id;
    	mc_receive_immediate_parms.type = SUB_NONE;
    	mc_receive_immediate_parms.length = 0;
    	mc_receive_immediate_parms.data = (u_char *) NULL;
    	rc = mc_receive_immediate(&mc_receive_immediate_parms);
    	switch (rc)
        {
	case OK:
    	    /* See what was received */
    	    switch (mc_receive_immediate_parms.what_received_data_type)
	    {
	    case WR_NONE:
		/* Check for the data token */
    	    	if (mc_receive_immediate_parms.what_received_flag_type ==
    	    	    WR_SEND)
		{
		    /* 
    	    	    ** We did get the token so move into send state 
    	    	    ** and redrive send
    	    	    */
    	    	    pcb->conv_state = CONV_SEND;
    	    	    parms->state = GC_SNDCMP;
    	    	    goto top;
		}

    	    	/* No token yet; readd conversation to wait list */
        	GClu62reg(dcb, pcb, (u_char) READABLE, parms);
    	    	return;

    	    case WR_DATA_INCOMPLETE:
		/* Check for the data token */
    	    	if (mc_receive_immediate_parms.what_received_flag_type ==
    	    	    WR_SEND)
		{
		    /* 
    	    	    ** We did get the token so move into send state 
    	    	    ** and redrive send
    	    	    */
    	    	    pcb->conv_state = CONV_SEND;
    	    	    parms->state = GC_SNDCMP;
    	    	    goto top;
		}

		/* 
    	    	** As we didn't get token and there is data, drop
    	    	** into send complete state and wait to be driven
    	    	** from a completed receive that picks up the token.
    	    	*/
    	    	parms->state = GC_SNDCMP;
    	    	return;

	    case WR_DATA_COMPLETE:
    	    	/* 
    	    	** How can the data be complete when we put up a zero
    	    	** length buffer? Just drop through to default case.
    	    	*/

    	    default:
    	        GClu62er(parms, GC_SEND_FAIL, 0, OK);
    	    	pcb->send_parm_list = NULL;
    	    	goto complete;
	    }

    	    break;

    	case UNSUCCESSFUL_RETRY:
    	    /* nothing to receive yet, readd conversation to wait list */
    	    GClu62reg(dcb, pcb, (u_char) READABLE, parms);
    	    return;

    	default:
	    /* receive failed, indicate send error */
	    GClu62er(parms, GC_SEND_FAIL, VERB_MC_RECEIVE_IMMEDIATE, rc);
    	    pcb->send_parm_list = NULL;
	    goto complete;
	}

    } /* end case GC_SNDTKN */

    case GC_SNDCMP:
    {
    	MC_PREPARE_TO_RECEIVE mc_prepare_to_receive_parms;

    	/* If the prepare to receive timer is running, cancel it */
    	if (pcb->timer_running)
    	{
	    pcb->timeout = -1;
    	    GClu62timer(dcb, pcb);
	}

	/*
	** Send complete. Attempt the send.
	*/

    	GClu62getcid(pcb, WRITABLE, &mc_send_data_parms.conv_id);
    	mc_send_data_parms.type = SUB_FLUSH;
    	mc_send_data_parms.nbio = 1;
    	mc_send_data_parms.length = parms->buffer_lng;
    	mc_send_data_parms.data = (u_char *) parms->buffer_ptr;
    	mc_send_data_parms.map_name[0] = '\0';
    	rc = mc_send_data(&mc_send_data_parms);
    	switch (rc)
	{
    	case OK:
    	    /* 
    	    ** Send succeeded; check whether we got request-to-send
    	    ** from peer.
    	    */
    	    pcb->total_send++;
    	    pcb->send_parm_list = NULL;
    	    pcb->timeout = DEFAULT_TIMER_S;

    	    if (mc_send_data_parms.request_to_send_received)
	    {
		/* Give peer the data token and flush previous send */
    	    	mc_prepare_to_receive_parms.conv_id = pcb->conv_id;
    	    	mc_prepare_to_receive_parms.type = SUB_FLUSH;
    	    	if (rc = mc_prepare_to_receive(&mc_prepare_to_receive_parms))
		{
        	    /* Error sending prepare-to-receive */
    	    	    GClu62er(parms, GC_SEND_FAIL, VERB_MC_PREPARE_TO_RECEIVE,
    	    	    	     rc);
    	    	    pcb->send_parm_list = NULL;
    	    	    goto complete;
	        }		    

    	    	pcb->conv_state = CONV_RCV;

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

    	    break;

    	case UNSUCCESSFUL_RETRY:
    	    /* 
    	    ** Send failed due to resource limitation; check whether we got
    	    ** request-to-send from peer.
    	    */
    	    if (mc_send_data_parms.request_to_send_received)
	    {
		/* Give peer the data token and flush */
    	    	mc_prepare_to_receive_parms.conv_id = pcb->conv_id;
    	    	mc_prepare_to_receive_parms.type = SUB_FLUSH;
    	    	if (rc = mc_prepare_to_receive(&mc_prepare_to_receive_parms))
		{
        	    /* Error sending prepare-to-receive */
    	    	    GClu62er(parms, GC_SEND_FAIL, VERB_MC_PREPARE_TO_RECEIVE,
    	    	    	     rc);
    	    	    pcb->send_parm_list = NULL;
    	    	    goto complete;
	        }		    

    	    	pcb->conv_state = CONV_RCV;

    	    	/* Do we have a saved receive? */
    	    	if (pcb->rcv_parm_list)

		    /* Yes, drive state machine with it */
    	    	    GClu62sm(pcb->rcv_parm_list);
	    }
	    else
    	    {

    	    	/* 
    	    	** Start the prepare-to-receive timer and
    	    	** readd conversation to wait list. If the timer
    	    	** expires before we are redriven for the send,
    	    	** the send will have to wait until we get the data
    	    	** token back; otherwise, the send will be redriven
    	    	** and the timer cancelled.
    	    	*/
    	    	GClu62timer(dcb, pcb);
    	    	GClu62reg(dcb, pcb, (u_char) WRITABLE, parms);
    	    }

            return;

    	default:
	    /* send failed, indicate send error */
	    GClu62er(parms, GC_SEND_FAIL, VERB_MC_SEND_DATA, rc);
    	    pcb->send_parm_list = NULL;
	    goto complete;
	}
	    
    	goto complete;
    } /* end case GC_SNDCMP */


    case GC_RCV:
    {
	/*
	** Check to see if conversation is in RCV state. If we are not, we will
    	** have to hold on to the receive until we are.
	*/

        
	GCTRACE(2)("GClu62sm: pcb %p conv state %d read size %d registered %d\n", 
    	    	   pcb, pcb->conv_state, parms->buffer_lng, pcb->registered);

    	if (pcb->conv_state == CONV_RCV ||  pcb->duplex == DUPLEX)
	{
    	    /* 
    	    ** We're in receive state so add conversation to wait list, 
    	    */

    	    GClu62reg(dcb, pcb, (u_char) READABLE, parms);
    	    pcb->rcv_parm_list = parms;
    	    parms->state = GC_RCVCMP;
    	    return;
        }
        else if (pcb->conv_state == CONV_SEND)
        {
    	    /* 
    	    ** Can't do receive right now. Simply hold onto receive until
    	    ** we move into RCV state. At that time, we'll be redriven in
    	    ** this state.
    	    */
    	    pcb->rcv_in_conv_send++;
    	    pcb->rcv_parm_list = parms;
    	    parms->state = GC_RCV;
    	    return;
        }
        else
        { /* default: */
    	    /* Any other state is an error */
    	    GClu62er(parms, GC_RECEIVE_FAIL, 0, OK);
    	    goto complete;
	}

/*	return; NOTREACHED */
    } /* end case GC_RCV */

    case GC_RCVCMP:
    {
    	MC_PREPARE_TO_RECEIVE mc_prepare_to_receive_parms;

	/*
	** Receive complete. Attempt receive.
	*/

        GClu62getcid(pcb, READABLE, &mc_receive_immediate_parms.conv_id);
    	mc_receive_immediate_parms.type = SUB_NONE;
    	mc_receive_immediate_parms.length = parms->buffer_lng;
    	mc_receive_immediate_parms.data = (u_char *) parms->buffer_ptr;
    	rc = mc_receive_immediate(&mc_receive_immediate_parms);
    	switch (rc)
	{
	case OK:

    	    /* See what was received */
    	    switch (mc_receive_immediate_parms.what_received_data_type)
	    {
	    case WR_NONE:
    	    	/* See if we got given data token */
    	    	if (mc_receive_immediate_parms.what_received_flag_type ==
    	    	    WR_SEND)
	    	{
		    pcb->conv_state = CONV_SEND;

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
    	    	    	if (pcb->timeout  == 0)
			{
    	    	            /* Give peer the data token and flush */
    	    	    	    mc_prepare_to_receive_parms.conv_id = 
    	    	    	    	pcb->conv_id;
    	    	    	    mc_prepare_to_receive_parms.type = SUB_FLUSH;
    	    	    	    if (rc = mc_prepare_to_receive(
    	    	    	    	    	&mc_prepare_to_receive_parms))
    	    	    	    {
        	    	    	/* Error sending prepare-to-receive */
    	    	    	    	GClu62er(parms, GC_SEND_FAIL, 
    	    	    	     	    	 VERB_MC_PREPARE_TO_RECEIVE, rc);
    	    	    	    	pcb->rcv_parm_list = NULL;
    	        	    	goto complete;
			    }

    	    	    	    /*
    	    	    	    ** As we gave the token straight back, we
			    ** reregister for read completion.
			    */
			    pcb->conv_state = CONV_RCV;
    	    	    	    parms->state = GC_RCVCMP;
    	    	    	    GClu62reg(dcb, pcb, (u_char) READABLE, parms);
			}
			else
			{
    	    	    	    GClu62timer(dcb, pcb);

			    /* Bump timeout for next time around */
			    if (pcb->timeout < MAX_TIMER_R)
    	    	    	    	pcb->timeout <<= 1;
			}
			
		    } /* end of else */
	    	}
    	    	else
		{
        	    /* Just readd conversation to wait list */
        	    GClu62reg(dcb, pcb, (u_char) READABLE, parms);
		}

    	    	return;

	    case WR_DATA_COMPLETE:
    	    	/* Receive is complete */
    	    	pcb->total_rcv++;
    	    	parms->buffer_lng = mc_receive_immediate_parms.length;
    	    	pcb->rcv_parm_list = NULL;
    	    	pcb->timeout = DEFAULT_TIMER_R;

    	    	/* See if we got given data token */
    	    	if (mc_receive_immediate_parms.what_received_flag_type ==
    	    	    WR_SEND)
	    	{
		    pcb->conv_state = CONV_SEND;

    	    	    /* Do we have a saved send? */
    	    	    if (pcb->send_parm_list)

		    	/* Yes, drive state machine with it */
    	    	    	GClu62sm(pcb->send_parm_list);
    	    	    else
		    {
    	    	    	/* Start "prepare-to-receive" timer */
    	    	    	GClu62timer(dcb, pcb);

			/* Bump timeout for next time around */
			if (pcb->timeout < MAX_TIMER_R)
    	    	    	    pcb->timeout <<= 1;
		    }
	    	}
    	    	else
		{
		    /*
		    ** No data token. If there is a saved send, we have
		    ** to re-register it for read completion as it needs to
		    ** continue to poll for data token.
		    */
		    if (pcb->send_parm_list)
			GClu62reg(dcb, pcb, (u_char) READABLE,
				  pcb->send_parm_list);
		}

    	    	break;

	    case  WR_DATA_INCOMPLETE:
    	    	/* 
    	    	** Looks like we got a larger record than we can cope with.
    	    	** Treat this as a fatal error by dropping through to the
    	    	** default.
    	    	*/

    	    default:
    	        GClu62er(parms, GC_RECEIVE_FAIL, 0, OK);
    	    	pcb->rcv_parm_list = NULL;
    	    	goto complete;
	    }
      
    	    break;

	case UNSUCCESSFUL_RETRY:
    	    /* nothing to receive yet, readd conversation to wait list */
    	    GClu62reg(dcb, pcb, (u_char) READABLE, parms);
    	    return;

    	default:
	    /* receive failed, indicate error */
	    GClu62er(parms, GC_RECEIVE_FAIL, VERB_MC_RECEIVE_IMMEDIATE, rc);
    	    pcb->rcv_parm_list = NULL;
	    goto complete;
	}

        goto complete;
    } /* end case GC_RCVCMP */

    case GC_DISCONN:
    {
    	GCC_P_PLIST    	*compl_parm_list;
    	MC_DEALLOCATE mc_deallocate_parms;
	GET_STATE get_state_parms;

    	if (!pcb)
	    /* nothing to do */
    	    goto complete;

    	/* Cancel any event registration */
    	if (pcb->registered)
    	    GClu62reg(dcb, pcb, (u_char) CANCEL, (GCC_P_PLIST *) NULL);

    	/* Cancel the prepare-to-receive timer if its running */
    	if (pcb->timer_running)
	{
	    pcb->timeout = -1;
    	    GClu62timer(dcb, pcb);
	}

    	/* Drive any outstanding send/receive with an error */
    	if (pcb->send_parm_list)
	{
    	    compl_parm_list = pcb->send_parm_list;
    	    pcb->send_parm_list = NULL;	
	    GClu62er(compl_parm_list, GC_SEND_FAIL, 0, OK);

    	    GCTRACE(1)("GClu62sm: %p complete %s status %d\n", 
    	    	       compl_parm_list, gc_names[ compl_parm_list->state ],
    	    	       compl_parm_list->generic_status );

    	    (*compl_parm_list->compl_exit) (compl_parm_list->compl_id);
	}

    	if (pcb->rcv_parm_list)
	{
    	    compl_parm_list = pcb->rcv_parm_list;
    	    pcb->rcv_parm_list = NULL;	
	    GClu62er(compl_parm_list, GC_RECEIVE_FAIL, 0, OK);

    	    GCTRACE(1)("GClu62sm: %p complete %s status %d\n", 
    	    	       compl_parm_list, gc_names[ compl_parm_list->state ],
    	    	       compl_parm_list->generic_status );

    	    (*compl_parm_list->compl_exit) (compl_parm_list->compl_id);
	}

        if (pcb->duplex == DUPLEX)
        {
            GClu62getcid(pcb, WRITABLE, &mc_deallocate_parms.conv_id);
    	    mc_deallocate_parms.log_data_length = 0;
    	    mc_deallocate_parms.log_data = NULL;
    	    mc_deallocate_parms.type = SUB_FLUSH;
    	    /* Now issue deallocate */
    	    if (rc = mc_deallocate(&mc_deallocate_parms))
	    {
        	/* Error disconnecting  */
    	    	GClu62er(parms, GC_DISCONNECT_FAIL, VERB_MC_DEALLOCATE, rc);
	    }
	    else
	    {
	        GCTRACE(4)("outgoing session deallocated\n");
	    }

            GClu62getcid(pcb, READABLE, &get_state_parms.conv_id);
	    if (get_state(&get_state_parms))
	    {
		GCTRACE(1)("get state incoming failed\n");
	    }
	    else
	    {
	        GCTRACE(4)("incoming conversation state = %x\n",
		     get_state_parms.conv_state);
	        pcb->conv_state = get_state_parms.conv_state;
	    }

            GClu62getcid(pcb, READABLE, &mc_deallocate_parms.conv_id);
    	    mc_deallocate_parms.log_data_length = 0;
    	    mc_deallocate_parms.log_data = NULL;
	    if (pcb->conv_state == CONV_END_CONV)
    	        mc_deallocate_parms.type = SUB_LOCAL;      
            else
    	        mc_deallocate_parms.type = SUB_ABEND_PROG;
    	    /* Now issue deallocate */
    	    if (rc = mc_deallocate(&mc_deallocate_parms))
	    {
        	/* Error disconnecting  */
    	    	GClu62er(parms, GC_DISCONNECT_FAIL, VERB_MC_DEALLOCATE, rc);
	    }
	    else
	    {
	        GCTRACE(4)("incoming session deallocated\n");
	    }

        }  /* end duplex */
        else
        {
	    GClu62getcid(pcb, READABLE, &get_state_parms.conv_id);
	    if (get_state(&get_state_parms))
	    {
		GCTRACE(1)("get state halfplex failed\n");
	    }
	    else
	    {
	        GCTRACE(4)("halfplex conversation state = %x\n",
  	                get_state_parms.conv_state);
	        pcb->conv_state = get_state_parms.conv_state;
            }

    	    switch (pcb->conv_state)
	    {
    	    case CONV_RCV:
    	    case CONV_SEND:
    	    case CONV_END_CONV:
    	        mc_deallocate_parms.conv_id = pcb->conv_id;
    	        mc_deallocate_parms.log_data_length = 0;
    	        mc_deallocate_parms.log_data = NULL;

    	        /* See what sort of deallocate to issue */

    	        if (pcb->conv_state == CONV_RCV)

            	    /* 
    	    	    ** We're in receive state so we can't issue a normal
    	    	    ** deallocate as we don't have the data token. Issue a
    	    	    ** deallocate (abend)
    	    	    */
    	            mc_deallocate_parms.type = SUB_ABEND_PROG;
    	        else if (pcb->conv_state == CONV_SEND)

    	    	    /* Issue a normal deallocate */
    	            mc_deallocate_parms.type = SUB_FLUSH;
    	        else

    	            /* 
    	            ** We have already picked up an incoming deallocate so
    	            ** just issue a local deallocate.
    	            */
                    mc_deallocate_parms.type = SUB_LOCAL;

    	        /* Now issue deallocate */
    	        if (rc = mc_deallocate(&mc_deallocate_parms))

        	    /* Error disconnecting  */
    	            GClu62er(parms, GC_DISCONNECT_FAIL, VERB_MC_DEALLOCATE, rc);
    	        break;
            }

	}

        GCTRACE(4)("pcb: 0x%p, connect time: %d msec\n",
                         pcb,
			 pcb->start_msec?GClu62gettime() - pcb->start_msec:0);

    	GCTRACE(2) 
        ("GClu62sm: pcb %p %d of %d sends in RCV; %d of %d receives in SEND\n",
    	 pcb, pcb->send_in_conv_rcv, pcb->total_send,
         pcb->rcv_in_conv_send, pcb->total_rcv);

        if (pcb->duplex == DUPLEX)
    	    dcb->conversations -= 2;
        else
    	    dcb->conversations -= 1;

        GCTRACE(1)("Current conversations: %d\n", dcb->conversations);
#ifdef ORPHANED
        clearpcb(pcb);
#endif


    	/* Free up pcb */
	MEfree( (PTR)pcb );
	parms->pcb = (PTR) NULL; 
    	goto complete;
    } /* end case GC_DISCONN */

    } /* end switch */

complete:
    /*
    ** Drive completion routine.
    */

    GCTRACE(1)("GClu62sm: %p complete %s status %x\n", parms, 
		gc_names[ parms->state ], parms->generic_status );

    (*parms->compl_exit)(parms->compl_id);
}


/*
** Name: GClu62name - determine gateway name and tp name
**
** Description:
**	Returns our gateway and transaction program name, suitable for handing
**      to tp_start and tp_listen. Uses the value of 
**  	ii.(host).gcc.*.sna_lu62.port, if defined, otherwise the input port id.
**
**  	Additionally, uses the value of ii.(host).gcc.*.sna_lu62.poll to set
**  	the background polling rate.
**
** Inputs:
**  	string identifier for LU 6.2
**	the default port id
**
** Returns:
**	the resulting gateway and transaction program name
**
** History
**	21-Aug-91 (cmorris)
**	    Created.
**	17-Dec-92 (gautam)
**	    Changed to use PMget().
**  	01-Feb-93 (cmorris)
**  	    Updated function description to match previous change.
*/
static STATUS
GClu62name(proto, port_id, tp_name, gateway_name)
char    *proto;
char	*port_id;
char 	*tp_name;
char	*gateway_name;
{
    char pmsym[128];
    char *period;
    char *val = 0 ;

    /*
    ** For the listen port, we're looking for the PM resource
    **  ii.(host).gcc.*.sna_lu62.port:     xxx 
    */

    STprintf(pmsym, "!.%s.port", proto);

    /* check to see if entry for given user */
    if( PMget( pmsym,  &val) != OK )
    {
	if (!port_id || *port_id == EOS)
        {
            return FAIL;
        }
        else
        {
            /* use input */
            val = port_id ;
        }
    }

    /* 
    ** Now split up name into gateway and tp components.
    ** The format is gateway_name.tp_name
    */

    period = STchr(val, '.');    	
    if (period == NULL)
    {
        STcopy(val, gateway_name);
        *tp_name = '\0';
    }
    else
    {
	/* Copy up to period */
    	STncpy( gateway_name, val, period - val);
	gateway_name[ period - val ] = EOS;

    	/* Copy after period */
    	++period;
    	STcopy(period, tp_name);
    }

    /*
    ** For the background polling rate look for the PM resource
    **  ii.(host).gcc.*.sna_lu62.poll:     xxx 
    */
    STprintf(pmsym, "!.%s.poll", proto);

    /* check to see if entry for given user */
    if( PMget( pmsym,  &val) == OK )
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
**  	the API function that returned the error
**
** Returns:
**	nothing
**
** History
**	04-Sep-91 (cmorris)
**	    Created.
**  	13-Oct-92 (cmorris)
**  	    Also set the text associated with any appc error.
**  	15-Oct-92 (cmorris)
**  	    Also check for resource failure errors when testing
**  	    whether we should move into CONV_END_CONV state.
**  	20-Oct-92 (cmorris)
**  	    Use the error name in the patched header file even
**  	    though it doesn't match the documentation!!
**  	22-Oct-92 (cmorris)
**  	    If we get an allocation error, reset conversation state
**  	    to reset:- there was never a conversation.
**	07-oct-1993 (tad)
**	    Bug #56449
**	    Changed %x to %p for pointer values.
**
*/
static VOID
GClu62er(parms, generic_status, api_function, rc)
GCC_P_PLIST *parms;
STATUS	    generic_status;
int 	    api_function;
int 	    rc;
{
    GC_PCB	*pcb = (GC_PCB *)parms->pcb;
    char	*s = parms->system_status.moreinfo[0].data.string;
    GET_APPC_ERROR_MSG get_appc_error_msg_parms;

    /* Firstly, set up error in parameter list */
    parms->generic_status = generic_status;

    if (rc != OK)
    {

        /* use BS_SYSERR for now... */
    	SETCLERR(&parms->system_status, BS_SYSERR, 0);

    	/* Get text associated with error */
    	get_appc_error_msg_parms.error_return_code = (u_long) rc;
    	get_appc_error_msg(&get_appc_error_msg_parms);    
    	STprintf(s, "(%x:%d) %s", rc, api_function, 
    	    	 get_appc_error_msg_parms.msg);

    	/* when setting length, zap linefeed at end of string... */
    	parms->system_status.moreinfo[0].size = STlength(s) -1;

    	/* See whether we got allocate error */
    	if ((rc & 0xffff0000) == ALLOCATION_ERROR)

    	    /* There was never a conversation... */
    	    pcb->conv_state = CONV_RESET;

    	/* See whether we got a deallocate error */
    	if (((rc & 0xffff0000) == DEALLOCATION_ERROR) ||
            ((rc & 0xffff0000) == RESOURCE_FAILURE))

	    /* Update state of conversation */
	    pcb->conv_state = CONV_END_CONV;
    }

    GCTRACE(1)("GClu62er: %p lu62 status %x\n", parms, rc );
}


/*
** Name: GClu62reg - Register conversation for event notification
**
** Description:
**      Adds a conversation to the APPC wait list, together
**  	with the operation for which it is waiting.
**
** Inputs:
**	dcb - driver control block 
**  	pcb - per-connection control block
**  	waiting type (readable or writable)
**  	parameter list for event
**
** Returns:
**	nothing
**
** History
**	04-Sep-91 (cmorris)
**	    Created.
**  	09-Jul-92 (cmorris)
**  	    When cancelling registration, decrement registered counter.
**  	10-Jul-92 (cmorris)
**  	    If no of registered (readers writers) drops to zero, unregister
**  	    file descriptor for (read write) completion.
**  	22-Sep-92 (cmorris)
**  	    Always reregister for read completion as the timeout period
**  	    may have changed since last registration.
*/
static VOID
GClu62reg(dcb, pcb, operation, parms)
GC_DCB	    *dcb;
GC_PCB	    *pcb;
u_char	    operation;
GCC_P_PLIST *parms;
{
    u_i4	    i, j,f;
    f = 0;

    if (operation == CANCEL || pcb->registered)
    {
        GCTRACE(3)("Canceling conv_id %x\n", pcb->conv_id);
	/* remove conversation from wait list */
    	for (i = 0; i < MAX_WAIT_LIST; i++)
	{
/*            GCTRACE(3)("Checking id %x\n",dcb->appc_wait_parms.conv_id_list[i]);*/
	    if (dcb->appc_wait_parms.conv_id_list[i] == pcb->conv_id)
	    {
		f = 1;
    	    	dcb->appc_wait_parms.conv_id_list[i] = 0;
		dcb->appc_wait_parms.number_of_conv_ids--;
    	    	pcb->registered = FALSE;

    	    	/* See whether there's still a waiter or timer running */
    	    	if (!dcb->appc_wait_parms.number_of_conv_ids &&
    	    	    !dcb->number_of_timers) 

    	    	    /* Unregister file descriptor */
    	    	    (VOID) GClu62fdreg(dcb, (VOID (*)) 0);
		break;
	    }
	} /* end of for */

	if (f == 0)
	    GCTRACE(3)("\nid NOT FOUND error??\n");
    }

    if (operation == CANCEL)
	return;
    
/*    GCTRACE(3)("Registering pcb %x, conv_id %x\n", pcb, pcb->conv_id);*/
    /* Add conversation to wait list */
    for (i = 0; i < MAX_WAIT_LIST; i++)
    {
        if (dcb->appc_wait_parms.conv_id_list[i] == 0)
        {
      	    dcb->appc_wait_parms.conv_id_list[i] = pcb->conv_id; 
            GClu62getcid(pcb, operation, &dcb->appc_wait_parms.conv_id_list[i]);  
   	    dcb->appc_wait_parms.conv_id_rw_list[i] = operation;
    	    dcb->appc_wait_parms.number_of_conv_ids++;
            dcb->reg_parm_list[i] = parms;
       	    pcb->registered = TRUE;
    	    break;
        }
    }

    /* Register for read completion */
    (VOID)GClu62fdreg(dcb, GClu62read);
}


/*
** Name: GClu62fdreg - Register file descriptor 
**
** Description:
**      Registers file descriptor for read completion with appropriate
**  	timeout.
**
** Inputs:
**	dcb - driver control block 
**  	func - function to register (null to unregister)
**
** Returns:
**	nothing
**
** History
**	03-Feb-93 (cmorris)
**	    Created.
*/
static VOID
GClu62fdreg(dcb, func)
GC_DCB	    *dcb;
VOID        (*func)();
{
    i4 timer;

    if (func)
    {
	/* Work out what timeout should be */
    	if (!dcb->number_of_timers &&
	    dcb->appc_wait_parms.number_of_conv_ids == 1 &&
	    dcb->listen_posted)
	{

	    /* Polling for listen completion - use background timer */
    	    timer = GClu62_timer_b;
	}
	else if (!dcb->number_of_timers)
    	{
	    /* No timers. Use normal wait timer */
	    timer = GClu62_timer_w;
	}
	else
	{
    	    /* There are timers. See whether there are waiters */
    	    if(!dcb->appc_wait_parms.number_of_conv_ids)
		timer = dcb->tim_expiration[0];
	    else
	    {
		/* 
    	    	** There are timers and waiters. If the only waiter
    	    	** is for listen completion, use the minimum of the
		** background wait timer and first timer expiration.
    	    	** Otherwise, use the minimum of the normal wait timer 
    	    	** and first timer expiration. 
		*/
	    	if (dcb->appc_wait_parms.number_of_conv_ids == 1 &&
	    	    dcb->listen_posted)
    	    	{
    	    	    if (GClu62_timer_b < 0)
			timer = dcb->tim_expiration[0];
		    else
		    	timer = dcb->tim_expiration[0] < GClu62_timer_b?
		                dcb->tim_expiration[0] : GClu62_timer_b;
		}
    	    	else
		    timer = dcb->tim_expiration[0] < GClu62_timer_w?
		            dcb->tim_expiration[0] : GClu62_timer_w;
	    }
	}

	/* Register for read completion */
    	(VOID)iiCLfdreg(dcb->gateway_fd, FD_READ, func,	(PTR) dcb, timer);
    }
    else
	/* Unregister */
    	(VOID)iiCLfdreg(dcb->gateway_fd, FD_READ, func,	(PTR) NULL, -1);
}


/*
** Name: GClu62timer - Register conversation for timer notification
**
** Description:
**      Adds a conversation to the timer list, together
**  	with the time to expiration.
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
**	    Created.
**	07-oct-1993 (tad)
**	    Bug #56449
**	    Changed %x to %p for pointer values.
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
    
    GCTRACE(2) ("GClu62timer: pcb %p timeout %d\n", pcb, pcb->timeout);

    /* Make sure we have a "real" time - a negative time cancels the timer */
    if (pcb->timeout < 0)
    {
    	/* Remove this pcb from timer list */
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

    } /* end of if timeout < 0 */
    else
    {
    	/* Take time snapshot */
 
# if defined(xCL_GETTIMEOFDAY_TIMEONLY)
	gettimeofday(&newtod);
# elif defined(xCL_GETTIMEOFDAY_TIME_AND_TZ)
	gettimeofday(&newtod, &newzone);
# endif

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
    	    if (dcb->tim_expiration[i] >= pcb->timeout)
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
    	dcb->tim_expiration[i] = pcb->timeout;
        dcb->number_of_timers++;

    	/* Store time snapshot */
    	dcb->timtod.tv_sec = newtod.tv_sec;
    	dcb->timtod.tv_usec = newtod.tv_usec;
    } /* end of else */

    /* Reregister for read completion */
    (VOID)GClu62fdreg(dcb, GClu62read);
}


/*
** Name: GClu62read - Read callback
**
** Description:
**  	Callback for read completion. Drives completions of any
**  	expired timers and polls for an event.
**
** Inputs:
**  	driver control block
**
** Returns:
**	nothing
**
** History
**	10-Jan-92 (cmorris)
**	    Created.
**  	09-Jul-92 (cmorris)
**  	    Don't shuffle up entries in timer table until after we've got
**  	    list of all expired timers:- shuffling up each time we come
**  	    across an expired timer causes subsequent expired timers to be
**  	    missed.
**  	22-Sep-92 (cmorris)
**  	    Take out special casing of single timer expiration:- because 
**  	    we also use a timer to poll for appc_wait(), we may falsely
**  	    assume the timer has expired when, in fact, we were awakened
**  	    because it's time to call appc_wait().
**  	29-Oct-92 (cmorris)
**  	    Incorporated GClu62wait.
**  	30-Oct-92 (cmorris)
**  	    Rejigged to only process a single timer per pass.
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
    int	    	    rc;
    GCC_P_PLIST     *parms;

    GCTRACE(3) ("GClu62read: %d waiter(s), %d timer(s)\n",
                dcb->appc_wait_parms.number_of_conv_ids,
    	        dcb->number_of_timers);

    /* Unless we are polling for events, update status of timers */
    if (dcb->number_of_timers && GClu62_timer_w != 0)
    {
	/* Take snapshot of time */
# if defined(xCL_GETTIMEOFDAY_TIMEONLY)
	gettimeofday(&newtod);
# elif defined(xCL_GETTIMEOFDAY_TIME_AND_TZ)
	gettimeofday(&newtod, &newzone);
# endif

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

    	/* Store time snapshot */
    	dcb->timtod.tv_sec = newtod.tv_sec;
    	dcb->timtod.tv_usec = newtod.tv_usec;
    }

    /* See whether first timer has expired */
    if (dcb->number_of_timers && dcb->tim_expiration[0] == 0)
    {
	/* 
    	** Timer has expired, remove entry from list and
    	** save it as the expired timer.
    	*/
    	pcb = dcb->tim_pcb[0];
        pcb->timer_running = FALSE;
    	dcb->number_of_timers--;

    	/* Shuffle up remaining timers */
    	for (j = 0, i = 1; j < dcb->number_of_timers ; j++, i++)
	{
	    dcb->tim_pcb[j] = dcb->tim_pcb[i];
    	    dcb->tim_expiration[j] = dcb->tim_expiration[i];
    	}

    	/* Call timer expiration function */
	GClu62timeout(pcb);

    	/*
    	** Set the wait timeout to 0 to make sure that we get redriven 
    	** immediately to process another expired timer or process
    	** an event.
    	*/
    	GClu62_timer_w = 0;    	
    } /* end of if */
    else
    {
	int f=0;

    	/* Poll for an event */
    	rc = appc_wait(&dcb->appc_wait_parms);
    	switch (rc)
    	{
    	case OK:

    	    /* 
    	    ** Remove event's conversation from the appc_wait
    	    ** parameter list.
    	    */
    	    for (i = 0; i < MAX_WAIT_LIST; i++)
	    {
	    	if (dcb->appc_wait_parms.conv_id_list[i] == 
    	    	    dcb->appc_wait_parms.posted_conv_id)
	    	{
		    f=1;
		    dcb->appc_wait_parms.conv_id_list[i] = 0;
    	    	    dcb->appc_wait_parms.number_of_conv_ids--;
    	    	    parms = dcb->reg_parm_list[i];
    	    	    pcb = (GC_PCB *) parms->pcb;
    	    	    pcb->registered = FALSE;

    		    /* Reenter the state machine with event */
    	    	    GClu62sm(parms);
    	    	    break;
	    	} /* end of if */

	    } /* end of for */
	    if (f == 0) GCTRACE(3)("\nNOT FOUND error\n");

    	    /* 
    	    ** Set appc_wait timeout to 0 to make sure we get
    	    ** redriven immediately in the hope there's another
    	    ** event to pick up.
    	    */
    	    GClu62_timer_w = 0;
    	    break;

    	case UNSUCCESSFUL_RETRY:
    	    GClu62_timer_w = DEFAULT_TIMER_W;
    	    break;
    	
    	default:

    	    /*
    	    ** A fatal error has occurred. Most likely we are no longer
    	    ** able to talk to the gateway. Reenter the protocol machine 
    	    ** for each conversation that was registered for event completion.
	    ** This will cause an appc function to be called for each
	    ** conversation and (hopefully) the error will recur and allow
	    ** each event completion to be driven with the error.
	    */
	    GCTRACE(2) ("GClu62read: appc_wait error %x\n", rc);
    	    for (i = 0; i < MAX_WAIT_LIST; i++)
	    {
    	    	if (dcb->appc_wait_parms.conv_id_list[i] != 0)
	    	{
    	    	    dcb->appc_wait_parms.conv_id_list[i] = 0;
        	    dcb->appc_wait_parms.number_of_conv_ids--;
    	    	    parms = dcb->reg_parm_list[i];
    	    	    pcb = (GC_PCB *) parms->pcb;
    	    	    if ( pcb != NULL )
		    {
			pcb->registered = FALSE;
		    }

    		    /* Reenter the state machine with event */
    	    	    GClu62sm(parms);
	    	} /* end of if */

	    } /* end of for */

    	} /* end of switch */

    } /* end of else */

    /* See if we still have a non-null number of waiters or timers */
    if (dcb->appc_wait_parms.number_of_conv_ids || dcb->number_of_timers) 

	/* Register for read completion */
    	(VOID)GClu62fdreg(dcb, GClu62read);
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
** Inputs:
**  	pcb - per-connection control block
**
** Returns:
**	nothing
**
** History
**	15-Jan-92 (cmorris)
**	    Created.
**  	01-Feb-93 (cmorris)
**  	    Don't drive both receive and send after error on
**  	    prepeare-to-receive. The completion of one may cause a
**  	    disconnect to be issued, thus releasing the underlying 
**  	    resources including the pcb.
**	07-oct-1993 (tad)
**	    Bug #56449
**	    Changed %x to %p for pointer values.
*/
static VOID
GClu62timeout(pcb)
GC_PCB 	*pcb;
{
    GCC_P_PLIST 	    *compl_parm_list;
    MC_PREPARE_TO_RECEIVE mc_prepare_to_receive_parms;
    int rc;

    GCTRACE(2) ("GClu62timeout: pcb %p\n", pcb);

    if (pcb->duplex != DUPLEX)
    {
        /* Give peer the data token and flush */
        mc_prepare_to_receive_parms.conv_id = pcb->conv_id;
        mc_prepare_to_receive_parms.type = SUB_FLUSH;
        if (rc = mc_prepare_to_receive(&mc_prepare_to_receive_parms))
        {
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
    	        	     VERB_MC_PREPARE_TO_RECEIVE, rc);

    	        GCTRACE(1)("GClu62timeout: %p complete %s status %d\n", 
    	       	      compl_parm_list, gc_names[ compl_parm_list->state ],
    	    	       compl_parm_list->generic_status );
    	        (*compl_parm_list->compl_exit) (compl_parm_list->compl_id);
	    }
    	    else if (pcb->send_parm_list)
	    {
    	        compl_parm_list = pcb->send_parm_list;
        	    pcb->send_parm_list = NULL;	
        	GClu62er(compl_parm_list, GC_SEND_FAIL, 
    	    	         VERB_MC_PREPARE_TO_RECEIVE, rc);

    	        GCTRACE(1)("GClu62timeout: %p complete %s status %d\n", 
    	    	       compl_parm_list, gc_names[ compl_parm_list->state ],
    	    	       compl_parm_list->generic_status );

    	        (*compl_parm_list->compl_exit) (compl_parm_list->compl_id);
	    }
        }		    
        else
        {
      	    pcb->conv_state = CONV_RCV;

            /* Do we have a saved receive? */
            if (pcb->rcv_parm_list)

	        /* Yes, drive state machine with it */
    	        GClu62sm(pcb->rcv_parm_list);
        }
    }
    else
    {
      	pcb->conv_state = CONV_RCV;

        /* Do we have a saved receive? */
        if (pcb->rcv_parm_list)

	    /* Yes, drive state machine with it */
    	    GClu62sm(pcb->rcv_parm_list);
    }
}

/* Places in passed pointer(cvid) the proper conversation id given the
** state, value  of duplex and side of conversation
*/

static VOID GClu62getcid(pcb, state, cvid)
GC_PCB *pcb;
i4  state;
u_long        *cvid;
{

    if (pcb->duplex != DUPLEX)
    {
        *cvid  = pcb->conv_id;
        return;
    }

    /* duplex conversations */
    if (WRITABLE == state)
    {
        if (pcb->invoked)
        {
           *cvid  = pcb->conv_id;
        }
        else
        {
            *cvid  = pcb->conv_id2;
        }
    }
    else if (READABLE == state)
    {
        if (pcb->invoked)
        {
            *cvid  = pcb->conv_id2;
        }
        else
        {
            *cvid  = pcb->conv_id;
        }
    }
/*    GCTRACE(4)("GClu62getcid returns %x\n", *cvid);*/
    return;
}


/*
** called by server when an id is received 
** save important info from the temporary pcb and get rid it
*/

static GC_PCB *GClu62lpcbs(pcb, lpcb)
GC_PCB *pcb;
GC_PCB *lpcb;
{
    GCTRACE(3)("\nlinking %x to %x\n", pcb, lpcb);

    lpcb->conv_id2 = pcb->conv_id;
    lpcb->duplex = DUPLEX;

    /* Free up pcb */
    MEfree( (PTR)pcb );

    GCTRACE(3)("\n PCB *** %x *** GONE\n", pcb);

    return lpcb;
}

/* get time */
static i4 GClu62gettime()
{
    struct timeval newtod;
    struct timezone newzone;
    i4         time_msec;

    /* Take snapshot of time */
# if defined(xCL_GETTIMEOFDAY_TIMEONLY)
    gettimeofday(&newtod);
# elif defined(xCL_GETTIMEOFDAY_TIME_AND_TZ)
    gettimeofday(&newtod, &newzone);
# endif
    time_msec = ((newtod.tv_sec * 1000) + (newtod.tv_usec) / 1000);

    return (time_msec);
}

/*
** Allocate and initailize a pcb
*/

static GC_PCB *GClu62newpcb(parms,dcb)
GCC_P_PLIST *parms;
GC_DCB *dcb;
{
   GC_PCB *pcb;

    pcb = (GC_PCB *)MEreqmem( 0, sizeof( *pcb ), TRUE, (STATUS *)0 );

    if (pcb)
    {
        /* Initialize pcb */
        /* save this address */
        parms->pcb = (PTR)pcb;
       
    	pcb->conv_id = dcb->listen_conversation;
	pcb->conv_state = CONV_RESET;
    	pcb->timeout = DEFAULT_TIMER_R;
    	pcb->registered = FALSE;
    	pcb->timer_running = FALSE;
    	pcb->total_send = 0;
    	pcb->send_in_conv_rcv = 0;
    	pcb->total_rcv = 0;
    	pcb->rcv_in_conv_send = 0;
        pcb->invoked = FALSE;
        pcb->duplex = NOT_DUP;
	pcb->start_msec = 0; 
	pcb->stop_msec = 0; 
    }
    return pcb;
}

/*
** Allocate and send appropriate commands for the second
** side of a duplex conversation 
*/
static STATUS GClu62alloc2(parms)
GCC_P_PLIST *parms;
{
    /* alloc conversation */
    /* save conversation_id in conv_id2 */
    /* send GC_ATTN */
    /* send pkey */

    MC_ALLOCATE mc_allocate_parms;
    MC_SEND_DATA mc_send_data_parms;
    u_char buf[4];
    int rc;
    int i;

    GC_PCB *pcb = (GC_PCB *)parms->pcb;
    GC_DCB *dcb = (GC_DCB *)parms->pce->pce_dcb;

    GCTRACE(4)("IN: GClu62alloc2\n");
    /* Make sure we can handle another connection */
    if (dcb->conversations + 1 > MAX_WAIT_LIST)
    {
        GCTRACE(4)("GClu62alloc2  too many connections error\n");
    	GClu62er(parms, GC_CONNECT_FAIL, 0, OK);
	return FAIL;
    }

    /* Allocate conversation */

    /* 
    ** The unique session name (which identifies both the local and
    ** partner lu's) is in the node_id connect parameter; the tp_name
    ** is in the port_id connect parameter.
    */
    STcopy(parms->function_parms.connect.node_id, 
    	       (char *)mc_allocate_parms.unique_session_name);
    mc_allocate_parms.tp_name_is_ascii = 1;
    STcopy(parms->function_parms.connect.port_id,
               (char *)mc_allocate_parms.tp_name);
    mc_allocate_parms.type = SUB_WITHIN_SESSION_LIMIT;
    mc_allocate_parms.sync_level = SYNC_NONE;
    mc_allocate_parms.security_select = SECURITY_NONE;
    mc_allocate_parms.pip_length = 0;
    mc_allocate_parms.pip_data = NULL;
    rc = mc_allocate(&mc_allocate_parms);
    if (rc == ACTIVATION_FAILURE_RETRY || rc == ALLOCATION_FAILURE_RETRY)
    {
        return FAIL;
    }
    if (rc != OK)
    {
        GCTRACE(4)("GClu62alloc2  allocation error\n");
    	/* Some error occurred */
	GClu62er(parms, GC_CONNECT_FAIL, VERB_MC_ALLOCATE, rc);
    	return FAIL;
    }

    /* Allocate ok; save details of conversation */
    pcb->conv_id2 = mc_allocate_parms.conv_id;
    pcb->conv_state = CONV_SEND;

    /* 
    ** Do a send containing a listen up command
    */
    mc_send_data_parms.conv_id = pcb->conv_id2;
    mc_send_data_parms.type = SUB_FLUSH;
    mc_send_data_parms.nbio = 1;
    mc_send_data_parms.length = 1;
    buf[0] = GC_ATTN; 
    mc_send_data_parms.data = &buf[0];
    mc_send_data_parms.map_name[0] = '\0';
    if (rc = mc_send_data(&mc_send_data_parms))
    {
        GCTRACE(4)("GClu62alloc2  send attn error\n");
        /* send failed, indicate connect error */
        GClu62er(parms, GC_CONNECT_FAIL, VERB_MC_SEND_DATA, rc);
        return FAIL; 
    }
	    
    /* 
    ** Do a send containing our key 
    */
    mc_send_data_parms.conv_id = pcb->conv_id2;
    mc_send_data_parms.type = SUB_FLUSH;
    mc_send_data_parms.nbio = 1;
    mc_send_data_parms.length = IDLEN;
    MEcopy(pcb->pkey, IDLEN, (char *)&buf[0]); 
    mc_send_data_parms.data = &buf[0];
    mc_send_data_parms.map_name[0] = '\0';
    if (rc = mc_send_data(&mc_send_data_parms))
    {
        GCTRACE(4)("GClu62alloc2  send id error\n");
        /* send failed, indicate connect error */
        GClu62er(parms, GC_CONNECT_FAIL, VERB_MC_SEND_DATA, rc);
        return FAIL; 
    }

    GCTRACE(4)("OUT: GClu62alloc2\n");
    pcb->duplex = DUPLEX;
    return OK;
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

# endif /* xCL_SUN_LU62_EXISTS */
