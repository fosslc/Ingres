/*
** Copyright (c) 2004, 2010 Ingres Corporation
*/

#include <compat.h>
#include <gl.h>
#include <er.h>
#include <me.h>
#include <sp.h>
#include <mo.h>
#include <si.h>
#include <st.h>
#include <tr.h>

#include <iicommon.h>
#include <gca.h>
#include <gcm.h>
#include <gcu.h>
#include <gcdint.h>
#include <gcdmsg.h>
#include <gcc.h>
#include <gccpci.h>

/*
** Name: gcdgcc.c
**
** Description:
**	GCD server interface to GCC.
**
** History:
**	 3-Jun-99 (gordy)
**	    Created.
**	14-Sep-99 (gordy)
**	    Implemented JDBC error messages.
**	18-Oct-99 (gordy)
**	    Added flow control to GCC/MSG sub-systems.
**	21-Dec-99 (gordy)
**	    Added check for clients idle for excessive period.
**	17-Jan-00 (gordy)
**	    Fine-tune the client idle checking.
**	 3-Mar-00 (gordy)
**	    Moved CCB init to general initializations.
**	10-Apr-01 (loera01) Bug 104459
**	    When JCI_RQST_FAIL is received, go to disconnect state
**          (JCS_DISC) and do a disconnect (JCAS_16) instead of
**          going to the abort state (JCS_ABRT) and abort (JCAS_14).
**          Otherwise the JDBC server will not clean up aborted sessions.
**	20-Nov-02 (wansh01)    
** 	    Added and set isLocal indicator in CCB. 
**	18-Dec-02 (gordy)
**	    Converted for Data Access Server (GCD).
**	15-Jan-03 (gordy)
**	    Moved TL packet handling to separate TL implementations
**	    and abstracted with Service Provider interfaces.  FSM
**	    simplified since now only drives GCC protocol drivers.
**	    XOFF now implemented in FSM.
**	 6-Jun-03 (gordy)
**	    Added state machine input to detect receive completion
**	    during connection shutdown so that receive is not re-posted.
**	    Protocol information now stored in PIB.  Attach CCB as MIB
**	    object after GCC_LISTEN successfully completes and detach
**	    when CCB is freed.
**	 4-Jan-05 (gordy)
**	    Free RCB when FSM error occurs since it won't be used again.
**	21-Apr-06 (gordy)
**	    Simplified PCB/RCB handling.
**	 5-Dec-07 (gordy)
**	    Added accept() entry point to GCC service provider to allow
**	    GCC an opportunity to reject a connection with an error code.
**      05-Jun-08 (Ralph Loen)  SIR 120457
**          In gcc_sm(), added display message on successfully opened network 
**          ports to TLA_OCMP state to match equivalent on the GCC.
**      09-Feb-2010 (smeke01) b123226, b113797 
**	    MOlongout/MOulongout now take i8/u_i8 parameter.
**      22-Apr-10 (rajus01) SIR 123621
**          After a successful network open request update PIB with
**          the actual_port_id.
*/	

/*
** Forward references.
*/

static	void		gcc_check_protocols();
static	void		gcc_get_service( GCD_RCB * );
static	void		gcc_xoff( GCD_CCB *, bool );
static	STATUS		gcc_accept( TL_SRVC *, GCD_CCB * );
static	void		gcc_send( GCD_RCB * );
static	void		gcc_disc( GCD_CCB * );
static	void		gcc_abort( GCD_CCB *, STATUS );
static	void		gcc_request( GCD_RCB * );
static	void		gcc_eval( GCD_RCB * );
static	void		gcc_complete( PTR );
static	void		gcc_sm( GCD_RCB * );

/*
** GCC Service Provider definition passed to TL Services.
*/

static	GCC_SRVC	gcc_service = { gcc_accept, gcc_xoff, 
					gcc_send, gcc_disc };

/*
** GCCCL Protocol table.
*/

static	GCC_PCT		*gcc_pct = NULL;

/* 
** Request queue.  
**
** New requests are queued while requests 
** are being processed.
*/ 

static	bool		gcc_active = FALSE;
static	QUEUE		rcb_q;

/*
** GCC FSM definitions.
*/

#define	TLS_IDLE	0	/* FSM initial/final state */
#define	TLS_OPEN	1	/* GCC_OPEN request active */
#define	TLS_LSTN	2	/* GCC_LISTEN request active */
#define	TLS_RECV	3	/* GCC_RECEIVE request active */
#define	TLS_RCVX	4	/* Receive XOFF */
#define	TLS_SEND	5	/* GCC_SEND and GCC_RECEIVE requests active */
#define	TLS_SNDX	6	/* GCC_SEND request active, Receive XOFF */
#define	TLS_SHUT	7	/* GCC_SEND request active, disonnecting */
#define	TLS_DISC	8	/* GCC_DISCONNECT request active */

#define	TLS_CNT		9

static	char	*states[ TLS_CNT ] =
{
    "TLS_IDLE", "TLS_OPEN", "TLS_LSTN", "TLS_RECV", "TLS_RCVX", 
    "TLS_SEND", "TLS_SNDX", "TLS_SHUT", "TLS_DISC",
};

#define	TLI_OPEN_RQST	0	/* Issue GCC_OPEN */
#define TLI_LSTN_RQST	1	/* Issue GCC_LISTEN */
#define	TLI_RECV_RQST	2	/* Issue GCC_RECEIVE (for XON) */
#define	TLI_SEND_RQST	3	/* Issue GCC_SEND */
#define	TLI_DISC_RQST	4	/* Issue GCC_DISCONNECT */
#define	TLI_RQST_FAIL	5	/* GCC request has failed */
#define	TLI_RQST_DONE	6	/* GCC request has completed */
#define	TLI_RECV_DONE	7	/* GCC_RECEIVE has completed */
#define	TLI_RECV_DONX	8	/* GCC_RECEIVE has completed (XOFF) */
#define	TLI_RECV_SHUT	9	/* GCC_RECEIVE completed during shutdown */
#define	TLI_SEND_DONE	10	/* GCC_SEND has completed */
#define	TLI_SEND_DONQ	11	/* GCC_SEND has completed, requests queued */

#define	TLI_CNT		12

static	char	*inputs[ TLI_CNT ] =
{
    "TLI_OPEN_RQST", "TLI_LSTN_RQST", "TLI_RECV_RQST", "TLI_SEND_RQST", 
    "TLI_DISC_RQST", "TLI_RQST_FAIL", "TLI_RQST_DONE", 
    "TLI_RECV_DONE", "TLI_RECV_DONX", "TLI_RECV_SHUT",
    "TLI_SEND_DONE", "TLI_SEND_DONQ", 
};

#define	TLA_NOOP	0	/* Do nothing */
#define	TLA_OPEN	1	/* Issue GCC_OPEN request */
#define	TLA_OCMP	2	/* GCC_OPEN completed */
#define	TLA_OERR	3	/* GCC_OPEN failed */
#define	TLA_NEWC	4	/* New connection (CCB) */
#define	TLA_LSTN	5	/* Issue GCC_LISTEN request */
#define TLA_LCMP	6	/* GCC_LISTEN completed */
#define	TLA_LERR	7	/* GCC_LISTEN failed */
#define	TLA_RECV	8	/* Issue GCC_RECEIVE request */
#define	TLA_DATA	9	/* Process GCC_RECEIVE data */
#define	TLA_SEND	10	/* Issue GCC_SEND request */
#define	TLA_QEUE	11	/* Queue GCC_SEND request */
#define	TLA_DEQU	12	/* Get GCC_SEND request from queue */
#define	TLA_FLSH	13	/* Flush GCC_SEND request queue */
#define	TLA_DISC	14	/* Issue GCC_DISCONNECT request */
#define	TLA_ABRT	15	/* Abort connection */
#define	TLA_FRCB	16	/* Free RCB */
#define	TLA_FCCB	17	/* Free CCB & RCB */

#define	TLA_CNT		18

static	char	*actions[ TLA_CNT ] =
{
    "TLA_NOOP", "TLA_OPEN", "TLA_OCMP", "TLA_OERR", 
    "TLA_NEWC", "TLA_LSTN", "TLA_LCMP", "TLA_LERR", 
    "TLA_RECV", "TLA_DATA", 
    "TLA_SEND", "TLA_QEUE", "TLA_DEQU", "TLA_FLSH", 
    "TLA_DISC", "TLA_ABRT", "TLA_FRCB", "TLA_FCCB",
};

#define	TLAS_00		0
#define	TLAS_01		1
#define	TLAS_02		2
#define	TLAS_03		3
#define	TLAS_04		4
#define	TLAS_05		5
#define	TLAS_06		6
#define	TLAS_07		7
#define	TLAS_08		8
#define	TLAS_09		9
#define	TLAS_10		10
#define	TLAS_11		11
#define	TLAS_12		12
#define	TLAS_13		13
#define	TLAS_14		14
#define	TLAS_15		15
#define	TLAS_16		16
#define	TLAS_17		17

#define	TLAS_CNT	18
#define	TLA_MAX		3

static u_i1	act_seq[ TLAS_CNT ][ TLA_MAX ] =
{
    { TLA_OPEN,	0,	  0	   },	/* TLAS_00 */
    { TLA_OERR,	TLA_FCCB, 0        },	/* TLAS_01 */
    { TLA_OCMP,	TLA_NEWC, TLA_FCCB },	/* TLAS_02 */
    { TLA_LSTN,	0,	  0	   },	/* TLAS_03 */
    { TLA_LERR,	TLA_FCCB, 0	   },	/* TLAS_04 */	/* TODO: DISC pcb? */
    { TLA_LCMP,	TLA_NEWC, TLA_RECV },	/* TLAS_05 */
    { TLA_RECV,	TLA_DATA, 0	   },	/* TLAS_06 */
    { TLA_DATA,	0,	  0	   },	/* TLAS_07 */
    { TLA_RECV, TLA_FRCB, 0	   },	/* TLAS_08 */
    { TLA_SEND, 0,	  0	   },	/* TLAS_09 */
    { TLA_QEUE, 0,	  0	   },	/* TLAS_10 */
    { TLA_FRCB,	TLA_DEQU, TLA_SEND },	/* TLAS_11 */
    { TLA_DISC,	0,	  0	   },	/* TLAS_12 */
    { TLA_FLSH,	TLA_DISC, 0	   },	/* TLAS_13 */
    { TLA_ABRT,	TLA_DISC, 0	   },	/* TLAS_14 */
    { TLA_ABRT,	TLA_FLSH, TLA_DISC },	/* TLAS_15 */
    { TLA_FRCB,	0,	  0	   },	/* TLAS_16 */
    { TLA_FCCB,	0,	  0	   },	/* TLAS_17 */
};

/*
** Name: sm_info
**
** Description:
**	State machine definition table.  This is a concise list
**	of the states and expected inputs with their resulting
**	new states and actions.  This table is used to build
**	the sparse state machine table (matrix).
**
** History:
**	 4-Jun-99 (gordy)
**	    Created.
**	 4-Nov-99 (gordy)
**	    Added entries for TL interrupts.
**	15-Jan-03 (gordy)
**	    Reworked completely for removal of TL processing,
**	    addition to XOFF support.
**	 6-Jun-03 (gordy)
**	    Don't repost receives during shutdown.
*/

static struct
{
    u_i1	state;
    u_i1	input;
    u_i1	next;
    u_i1	action;

} sm_info[] =
{
    /*
    ** Accept new clients.
    */
    { TLS_IDLE,	TLI_OPEN_RQST,	TLS_OPEN,	TLAS_00 },
    { TLS_IDLE,	TLI_LSTN_RQST,	TLS_LSTN,	TLAS_03 },
    { TLS_OPEN,	TLI_RQST_FAIL,	TLS_IDLE,	TLAS_01 },
    { TLS_OPEN,	TLI_RQST_DONE,	TLS_IDLE,	TLAS_02 },
    { TLS_LSTN,	TLI_RQST_FAIL,	TLS_IDLE,	TLAS_04 },
    { TLS_LSTN,	TLI_RQST_DONE,	TLS_RECV,	TLAS_05 },

    /*
    ** General processing.  Wait for client request.  
    **
    ** Usually, a single active receive request is 
    ** maintained during the life of the connection.  
    ** Upper layers may signal for a pause (XOFF) in 
    ** data processing, which is only recognized once
    ** the current receive completes.  A TLI_RECV_RQST 
    ** is only issued when processing is resumed (XON).
    ** 
    ** Only a single send request may be active, so 
    ** subsequent requests are queued until the active 
    ** request completes.
    */
    { TLS_RECV,	TLI_RECV_RQST,	TLS_RECV,	TLAS_16 },
    { TLS_RECV,	TLI_SEND_RQST,	TLS_SEND,	TLAS_09 },
    { TLS_RECV,	TLI_DISC_RQST,	TLS_DISC,	TLAS_12 },
    { TLS_RECV,	TLI_RQST_FAIL,	TLS_DISC,	TLAS_14 },
    { TLS_RECV,	TLI_RECV_DONE,	TLS_RECV,	TLAS_06 },
    { TLS_RECV,	TLI_RECV_DONX,	TLS_RCVX,	TLAS_07 },
    { TLS_RECV,	TLI_RECV_SHUT,	TLS_RCVX,	TLAS_16 },

    { TLS_RCVX,	TLI_RECV_RQST,	TLS_RECV,	TLAS_08 },
    { TLS_RCVX,	TLI_SEND_RQST,	TLS_SNDX,	TLAS_09 },
    { TLS_RCVX,	TLI_DISC_RQST,	TLS_DISC,	TLAS_12 },
    { TLS_RCVX,	TLI_RQST_FAIL,	TLS_DISC,	TLAS_14 },

    { TLS_SEND,	TLI_RECV_RQST,	TLS_SEND,	TLAS_16 },
    { TLS_SEND,	TLI_SEND_RQST,	TLS_SEND,	TLAS_10 },
    { TLS_SEND,	TLI_DISC_RQST,	TLS_SHUT,	TLAS_16 },
    { TLS_SEND,	TLI_RQST_FAIL,	TLS_DISC,	TLAS_15 },
    { TLS_SEND,	TLI_RECV_DONE,	TLS_SEND,	TLAS_06 },
    { TLS_SEND,	TLI_RECV_DONX,	TLS_SNDX,	TLAS_07 },
    { TLS_SEND,	TLI_RECV_SHUT,	TLS_SNDX,	TLAS_16 },
    { TLS_SEND,	TLI_SEND_DONE,	TLS_RECV,	TLAS_16 },
    { TLS_SEND,	TLI_SEND_DONQ,	TLS_SEND,	TLAS_11 },

    { TLS_SNDX,	TLI_RECV_RQST,	TLS_SEND,	TLAS_08 },
    { TLS_SNDX,	TLI_SEND_RQST,	TLS_SNDX,	TLAS_10 },
    { TLS_SNDX,	TLI_DISC_RQST,	TLS_SHUT,	TLAS_16 },
    { TLS_SNDX,	TLI_RQST_FAIL,	TLS_DISC,	TLAS_15 },
    { TLS_SNDX,	TLI_SEND_DONE,	TLS_RCVX,	TLAS_16 },
    { TLS_SNDX,	TLI_SEND_DONQ,	TLS_SNDX,	TLAS_11 },

    /*
    ** Shut down a connection.
    */
    { TLS_SHUT,	TLI_RECV_RQST,	TLS_SHUT,	TLAS_16 },
    { TLS_SHUT,	TLI_SEND_RQST,	TLS_SHUT,	TLAS_16 },
    { TLS_SHUT,	TLI_DISC_RQST,	TLS_SHUT,	TLAS_16 },
    { TLS_SHUT,	TLI_RQST_FAIL,	TLS_DISC,	TLAS_13 },
    { TLS_SHUT,	TLI_RECV_DONE,	TLS_SHUT,	TLAS_16 },
    { TLS_SHUT,	TLI_RECV_DONX,	TLS_SHUT,	TLAS_16 },
    { TLS_SHUT,	TLI_RECV_SHUT,	TLS_SHUT,	TLAS_16 },
    { TLS_SHUT,	TLI_SEND_DONE,	TLS_DISC,	TLAS_12 },
    { TLS_SHUT,	TLI_SEND_DONQ,	TLS_SHUT,	TLAS_11 },

    { TLS_DISC,	TLI_RECV_RQST,	TLS_DISC,	TLAS_16 },
    { TLS_DISC,	TLI_SEND_RQST,	TLS_DISC,	TLAS_16 },
    { TLS_DISC,	TLI_DISC_RQST,	TLS_DISC,	TLAS_16 },
    { TLS_DISC,	TLI_RQST_FAIL,	TLS_DISC,	TLAS_16 },
    { TLS_DISC,	TLI_RQST_DONE,	TLS_IDLE,	TLAS_17 },
    { TLS_DISC,	TLI_RECV_DONE,	TLS_DISC,	TLAS_16 },
    { TLS_DISC,	TLI_RECV_DONX,	TLS_DISC,	TLAS_16 },
    { TLS_DISC,	TLI_RECV_SHUT,	TLS_DISC,	TLAS_16 },
    { TLS_DISC,	TLI_SEND_DONE,	TLS_DISC,	TLAS_16 },
    { TLS_DISC,	TLI_SEND_DONQ,	TLS_DISC,	TLAS_16 },
};

static struct
{

    u_i1	next;
    u_i1	action;

} smt[ TLS_CNT ][ TLI_CNT ] ZERO_FILL; 



/*
** Name: gcd_gcc_init
**
** Description:
**	Initialize the GCD server GCC interface.
**
** Inputs:
**	None.
**
** Outputs:
**	None.
**
** Returns:
**	STATUS		OK or FAIL.
**
** History:
**	 3-Jun-99 (gordy)
**	    Created.
**	21-Dec-99 (gordy)
**	    Queue added for active CCBs.
**	11-Jan-00 (gordy)
**	    Initialize the client check time.  Config file info
**	    checked when loaded.
**	 3-Mar-00 (gordy)
**	    Moved CCB init to general initializations.
**	18-Dec-02 (gordy)
**	    Multiple protocols now supported.
**	 6-Jun-03 (gordy)
**	    Protocol information now stored in PIB.
*/

STATUS
gcd_gcc_init( void )
{
    GCD_PIB	*pib;
    STATUS	status;
    CL_ERR_DESC	cl_err;
    i4		protocols_enabled = 0;
    u_i1	i;

    GCD_global.nl_hdr_sz = SZ_NL_PCI;
    GCD_global.tl_hdr_sz = DAM_TL_HDR_SZ;
    QUinit( &rcb_q );
    MEfill( sizeof( smt ), 0xff, (PTR)smt );

    if ( GCD_global.client_idle_limit )
    {
	TMnow( &GCD_global.client_check );
	GCD_global.client_check.TM_secs += GCD_global.client_idle_limit;
    }

    for( i = 0; i < ARR_SIZE( sm_info ); i++ )
    {
	smt[ sm_info[i].state ][ sm_info[i].input ].next = sm_info[i].next;
	smt[ sm_info[i].state ][ sm_info[i].input ].action = sm_info[i].action;
    }

    if ( GCpinit( &gcc_pct, &status, &cl_err ) != OK )
    {
	gcu_erlog( 0, GCD_global.language, status, &cl_err, 0, NULL );
	return( FAIL );
    }

    /*
    ** Initialize protocols and open listen ports.
    */
    for( i = 0; i < gcc_pct->pct_n_entries; i++ )
    {
	GCC_PCE *pce = &gcc_pct->pct_pce[ i ];
	GCD_CCB	*ccb;
	GCD_RCB	*rcb;
	char	*val, sym[ 128 ];
	
	/*
	** Skip protocols which don't allow listen ports.
	*/
	if ( pce->pce_flags & PCT_NO_PORT )  continue;

	/*
	** See if protocol enabled.
	*/
	STprintf( sym, "!.%s.status", pce->pce_pid );
	if ( PMget( sym, &val ) != OK  ||  STcasecmp( val, ERx("ON") ) )
	    continue;

	/*
	** Allocate and initialize resources.  Issue OPEN request.
	*/
	if ( 
	     ! (pib = (GCD_PIB *)MEreqmem(0, sizeof(GCD_PIB), TRUE, NULL))  ||
	     ! (ccb = gcd_new_ccb())  ||  
	     ! (rcb = gcd_new_rcb( ccb, 0 )) 
	   )
	{
	    if ( pib )
	    {
		MEfree( (PTR)pib );
		if ( ccb )  gcd_del_ccb( ccb );
	    }

	    gcu_erlog( 0, GCD_global.language, 
		       E_GC4808_NO_MEMORY, &cl_err, 0, NULL );
	    return( FAIL );
	}

	QUinit( &pib->q );
	STcopy( pce->pce_pid, pib->pid );
	STcopy( GCD_global.host, pib->host );

	/*
	** Get listen port.
	*/
	STprintf( sym, "!.%s.port", pib->pid );
	if ( PMget( sym, &val ) == OK )  
	    STcopy( val, pib->addr );
	else  if ( *pce->pce_port != EOS )
	    STcopy( pce->pce_port, pib->addr );
	else
	    STcopy( "II", pib->addr );

	QUinsert( &pib->q, GCD_global.pib_q.q_prev );

	ccb->gcc.pib = pib;
	ccb->gcc.pce = pce;
	rcb->ccb = ccb;
	rcb->request = TLI_OPEN_RQST;
	gcc_request( rcb );
	protocols_enabled++;
    }

    /*
    ** Make sure at least one protocol was available.
    */
    if ( ! protocols_enabled )
    {
	gcu_erlog( 0, GCD_global.language, 
		   E_GC4804_NTWK_CONFIG, &cl_err, 0, NULL );
	return( FAIL );
    }

    /*
    ** Check protocol open status.  If at least one protocol
    ** is OPEN or has not yet failed, return successfully.
    */
    for(
	 pib = (GCD_PIB *)GCD_global.pib_q.q_next;
	 (QUEUE *)pib != &GCD_global.pib_q;
	 pib = (GCD_PIB *)pib->q.q_next
       )
	if ( ! (pib->flags & PCT_OPN_FAIL) )  return( OK );

    /*
    ** All protocols are disable or have failed.
    */
    return( FAIL );
}


/*
** Name: gcd_gcc_term
**
** Description:
**	Shutdown the GCC interface.
**
** Inputs:
**	None.
**
** Outputs:
**	None.
**
** Returns:
**	void
**
** History:
**	 3-Jun-99 (gordy)
**	    Created.
*/

void
gcd_gcc_term( void )
{
    STATUS	status;
    CL_ERR_DESC	cl_err;

    GCpterm( gcc_pct, &status, &cl_err );
    gcc_pct = NULL;

    return;
}


/*
** Name: gcd_idle_check
**
** Description:
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	21-Dec-99 (gordy)
**	    Created.
**	17-Jan-00 (gordy)
**	    Make sure it is time to do idle check.  Clients idle
**	    limit is initialized if 0 and ignored if negative.
**	    Adjust idle check for next client to reach limit.
*/

void
gcd_idle_check( void )
{
    QUEUE	*q, *next;
    SYSTIME	now;

    /*
    ** Is it time to check the clients?
    */
    TMnow( &now );
    if ( ! GCD_global.client_idle_limit  ||  
	 TMcmp( &now, &GCD_global.client_check ) < 0 )  return;

    if ( GCD_global.gcd_trace_level >= 3 )
	TRdisplay( "%4d    GCD checking for idle clients\n", -1 );

    /*
    ** Set our next check time to one full period.
    ** If there are any clients which will timeout
    ** prior to that time, we adjust the next check
    ** time accordingly below.
    */
    STRUCT_ASSIGN_MACRO( now, GCD_global.client_check );
    GCD_global.client_check.TM_secs += GCD_global.client_idle_limit;

    for( q = GCD_global.ccb_q.q_next; q != &GCD_global.ccb_q; q = next )
    {
	GCD_CCB *ccb = (GCD_CCB *)q;
	next = q->q_next;

	/*
	** If idle limit time is 0 then the connection
	** is initializing and we initialize the idle 
	** limit.  If idle limit time is negative then 
	** the connection is being aborted and we do 
	** nothing.  Otherwise, check to see if client
	** has exceeded its idle limit.
	*/
	if ( ! ccb->gcc.idle_limit.TM_secs )
	{
	    TMnow( &ccb->gcc.idle_limit );
	    ccb->gcc.idle_limit.TM_secs += GCD_global.client_idle_limit;
	}
	else  if (  ccb->gcc.idle_limit.TM_secs > 0 )
	{  
	    if ( TMcmp( &now, &ccb->gcc.idle_limit ) >= 0 )
		gcc_abort( ccb, E_GC480D_IDLE_LIMIT );
	    else  
	    {
		if ( GCD_global.gcd_trace_level >= 5 )
		    TRdisplay( "%4d    GCD Idle %d seconds\n", ccb->id, 
			       GCD_global.client_idle_limit -
			       (ccb->gcc.idle_limit.TM_secs - now.TM_secs) );

		/*
		** If this client will timeout prior to our
		** next check time, reset the check time to
		** match this client.
		*/
		if ( TMcmp( &ccb->gcc.idle_limit, 
			    &GCD_global.client_check ) < 0 )
		{
		    STRUCT_ASSIGN_MACRO( ccb->gcc.idle_limit,
					 GCD_global.client_check );
		}
	    }
	}
    }

    return;
}


/*
** Name: gcc_check_protocols
**
** Description:
**	Checks status of protocol initializations.  Logs error and 
**	terminates server when all protocols have failed.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	18-Dec-02 (gordy)
**	    Created.
**	 6-Jun-03 (gordy)
**	    Protocol information stored in PIB.
*/

static void
gcc_check_protocols()
{
    GCD_PIB	*pib;
    CL_ERR_DESC	cl_err;

    /*
    ** Check for any protocols which have not yet failed.
    */
    for( 
	 pib = (GCD_PIB *)GCD_global.pib_q.q_next;
	 (QUEUE *)pib != &GCD_global.pib_q;
	 pib = (GCD_PIB *)pib->q.q_next
       )
	if ( ! (pib->flags & PCT_OPN_FAIL) )  return;	/* We be done! */

    /*
    ** All protocols have failed.  Log failure and exit.
    */
    gcu_erlog( 0, GCD_global.language, 
		E_GC4806_NTWK_INIT_FAIL, &cl_err, 0, NULL );
    GCshut();
    return;
}


/*
** Name: gcc_get_service
**
** Description:
**	Queries each TL Service Provider to determine which
**	services the current connection.  Issues a Disconnect
**	request if no Service Provider accepts the connection.
**	RCB is consumed in all cases.
**
** Input:
**	rcb	Request control block.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	15-Jan-03 (gordy)
**	    Created.
**	 5-Dec-07 (gordy)
**	    TL Service Provider is saved in gcc_accept() eliminating
**	    a race condition which previously had to be worked around.
*/

static void
gcc_get_service( GCD_RCB *rcb )
{
    GCD_CCB	*ccb = (GCD_CCB *)rcb->ccb;
    u_i4	i;

    /*
    ** Scan the TL Service Providers asking each
    ** if they will accept the connection.
    */
    for( i = 0; i < GCD_global.tl_srvc_cnt; i++ )
    {
	/*
	** Check if current TL Service Provider services this
	** request.  If provider accepts the connection, the
	** provider will be saved in gcc_accept().
	*/
	if ( (*GCD_global.tl_services[i]->accept)( &gcc_service, rcb ) )
	    return;	/* Connection accepted.  We be done! */
    }

    /*
    ** No TL Service Provider accepted the connection.
    **
    ** We need to consume the RCB and shutdown the
    ** connection.  This is easily handled by using
    ** the RCB to issue a disconnect request.
    */
    rcb->request = TLI_DISC_RQST;
    gcc_request( rcb );
    return;
}


/*
** Name: gcc_accept
**
** Description:
**	Indicates that the TL Service is accepting the connection.
**	Provides the GCC Service the opportunity to reject the
**	connection with a specific error condition.  If a status
**	other than OK is returned, the TL Service should reject
**	the new connection and return the status to the client
**	if possible.
**
**	During connection establishment, the GCC Service provider
**	does not have the capability of returning an error code by
**	itself.  Many conditions related to physical communications 
**	may cause a connection to be rejected and preclude returning
**	any type of error status.  Other conditions unrelated to
**	the physical connection may require rejection and which
**	need to be reported to the client.
**
**	This entry point must be called from TL_SRVC->accept() if 
**	the TL Service accepts the connection.  It does not need to 
**	be called if the TL Service is rejecting the connection.  
**	It must not be called if the TL Service does not service 
**	the request passed to TL_SRVC->accept().
**
** Input:
**	TL_SRVC *	The TL Service servicing the connection.
**	GCD_CCB *	Connection control block.
**
** Output:
**	None.
**
** Returns:
**	STATUS		OK or error code.
**
** History:
**	 5-Dec-07 (gordy)
**	    Created.
*/

static STATUS
gcc_accept( TL_SRVC *tl_srvc, GCD_CCB *ccb )
{
    char buff[16];

    /*
    ** Reject if server is closed to new connections.
    */
    if ( GCD_global.flags & GCD_SVR_CLOSED )  
    {
	if ( GCD_global.gcd_trace_level >= 1 )
	    TRdisplay( "%4d    GCD server closed\n" );

	return(E_GC481F_GCD_SVR_CLOSED);
    }

    /*
    ** Enforce client connection limit.
    */
    if ( GCD_global.client_max > 0  &&
         GCD_global.client_cnt >= GCD_global.client_max )
    {
	if ( GCD_global.gcd_trace_level >= 1 )
	    TRdisplay( "%4d    GCD client limit reached: %d of %d\n",
		       ccb->id, GCD_global.client_cnt, GCD_global.client_max );

	return( E_GC480E_CLIENT_MAX );
    }

    /*
    ** Accept the client connection.
    */
    MOulongout( 0, (u_i8)ccb->id, sizeof( buff ), buff );
    MOattach( MO_INSTANCE_VAR, GCD_MIB_CONNECTION, buff, (PTR)ccb );
    ccb->gcc.tl_service = tl_srvc;
    ccb->gcc.flags |= GCD_GCC_CONN;
    GCD_global.client_cnt++;

    return( OK );
}


/*
** Name: gcc_xoff
**
** Description:
**	Enable or disable receiving messages on a client network connection.
**
** Input:
**	ccb	Connection control block.
**	pause	TRUE to pause (disable), FALSE to continue (enable).
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	18-Oct-99 (gordy)
**	    Created.
**	15-Jan-03 (gordy)
**	    Abstracted for GCC Service Provider.
**	 6-Oct-03 (gordy)
**	    Don't allocate RCB message buffer for XON GCC Receive
**	    request since a new RCB and buffer are allocated when
**	    the request is processed.
*/

static void
gcc_xoff( GCD_CCB *ccb, bool pause )
{
    if ( pause )
    {
	if ( ! (ccb->gcc.flags & GCD_GCC_XOFF)  &&
	     GCD_global.gcd_trace_level >= 5 )
	    TRdisplay( "%4d    GCD IB XOFF requested\n", ccb->id );

	/*
	** Simply flag XOFF.  When GCC_RECEIVE completes
	** the TL state machine will pause waiting for XON.
	*/
	ccb->gcc.flags |= GCD_GCC_XOFF;
    }
    else  
    {
	GCD_RCB *rcb;

	if ( ccb->gcc.flags & GCD_GCC_XOFF  &&  
	     GCD_global.gcd_trace_level >= 5 )
	    TRdisplay( "%4d    GCD IB XON requested\n", ccb->id );

	/*
	** Remove XOFF indication.  Need to issue a receive
	** request so TL state machine will resume calling
	** GCC_RECEIVE.  Don't need message buffer since a
	** new RCB is allocated for the actual receive.
	*/
	ccb->gcc.flags &= ~GCD_GCC_XOFF;

	if ( ! (rcb = gcd_new_rcb( ccb, 0 )) )
	    gcc_abort( ccb, E_GC4808_NO_MEMORY );
	else
	{
	    rcb->request = TLI_RECV_RQST;
	    rcb->ccb = ccb;
	    gcc_request( rcb );
	}
    }

    return;
}


/*
** Name: gcc_send
**
** Description:
**	Send a client network message.
**
** Input:
**	rcb	Request control block.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	 3-Jun-99 (gordy)
**	    Created.
**	15-Jan-03 (gordy)
**	    Abstracted for GCC Service Provider.
*/

static void
gcc_send( GCD_RCB *rcb )
{
    rcb->request = TLI_SEND_RQST;
    gcc_request( rcb );
    return;
}


/*
** Name: gcc_disc
**
** Description:
**	Disconnect a client network connection.
**
** Input:
**	ccb	Connection control block.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	15-Jan-03 (gordy)
**	    Created.
**	 6-Jun-03 (gordy)
**	    Mark connection as being shutdown (to disable posting receives).
*/

static void
gcc_disc( GCD_CCB *ccb )
{
    GCD_RCB *rcb;

    /*
    ** No further requests should be forwarded
    ** to the TL Service Provider once a disconnect
    ** has been request, so drop the service and
    ** mark connection as being shutdown.
    */
    ccb->gcc.tl_service = NULL;
    ccb->gcc.flags |= GCD_GCC_SHUT;

    /*
    ** An RCB is required to shutdown the connection.
    ** If one cannot be allocated, an abort is issued
    ** which will attempt to shutdown the connection
    ** using a previously saved RCB.  No buffer is
    ** needed for disconnect.
    */
    if ( ! (rcb = gcd_new_rcb( ccb, 0 )) )
	gcc_abort( ccb, E_GC4808_NO_MEMORY );
    else
    {
	rcb->request = TLI_DISC_RQST;
	gcc_request( rcb );
    }

    return;
}


/*
** Name: gcc_abort
**
** Description:
**	Abort a connection.
**
** Input:
**	ccb	Connection control block.
**	status	Error status.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	 3-Jun-99 (gordy)
**	    Created.
**	21-Dec-99 (gordy)
**	    Reset idle limit to avoid contention.
**	17-Jan-00 (gordy)
**	    Idle limit reset is now negative rather than 0
**	    (0 indicates an uninitialized control block).
**	15-Jan-03 (gordy)
**	    Adapted for GCC Service Provider.
*/

static void
gcc_abort( GCD_CCB *ccb, STATUS status )
{
    GCD_RCB *rcb;

    /*
    ** Avoid contention with background processing.
    */
    ccb->gcc.idle_limit.TM_secs = -1;

    /*
    ** By saving an RCB during connection establishment,
    ** we provide a way to handle connection aborts when
    ** an RCB may not be available (memory allocation
    ** errors) and provide a mechanism whereby aborts can
    ** be limited to only one occurance per connection.
    */
    if ( (rcb = (GCD_RCB *)ccb->gcc.abort_rcb) )
    {
	ccb->gcc.abort_rcb = NULL;
	rcb->request = TLI_RQST_FAIL;
	rcb->gcc.p_list.generic_status = status;
	gcc_request( rcb );
    }
    
    return;
}


/*
** Name: gcc_request
**
** Description:
**
** Input:
**	rcb	Request control block.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	 3-Jun-99 (gordy)
**	    Created.
*/

static void
gcc_request( GCD_RCB *rcb )
{
    if ( gcc_active )
    {
	if ( GCD_global.gcd_trace_level >= 5 )
	    TRdisplay( "%4d    GCD queueing request %s\n",
			rcb->ccb->id, inputs[ rcb->request ] );

	QUinsert( &rcb->q, rcb_q.q_prev );
	return;
    }

    gcc_active = TRUE;
    gcc_sm( rcb );

    while( (rcb = (GCD_RCB *)QUremove( rcb_q.q_next )) )
    {
	if ( GCD_global.gcd_trace_level >= 5 )
	    TRdisplay( "%4d    GCD processing queued event %s\n",
			rcb->ccb->id, inputs[ rcb->request ] );

	gcc_sm( rcb );
    }

    gcc_active = FALSE;
    return;
}


/*
** Name: gcc_eval
**
** Description:
**	Evaluate the request and adjust based on additional
**	state information.
**
**	Expands TLI_RECV_DONE based on type of message received.
**	Check TLI_SEND_DONE for queued send requests.
**
** Input:
**	rcb		Request control block.
**
** Output:
**	rcb->request	New request ID.
**
** Returns:
**	void
**
** History:
**	 3-Jun-99 (gordy)
**	    Created.
**	 4-Nov-99 (gordy)
**	    Added TL interrupts.
**      10-Apr-01 (loera01) Bug 104459
**          When an error is detected, set input to JCI_ABRT_RQST instead of
**	    JCI_REQUEST_FAIL.
**	20-Dec-02 (gordy)
**	    Dispatch based on packet header ID.
**	15-Jan-03 (gordy)
**	    No longer processes TL packet header.
**	 6-Jun-03 (gordy)
**	    Check for receive completion during connection shutdown.
*/

static void
gcc_eval( GCD_RCB *rcb )
{
    GCD_CCB	*ccb = rcb->ccb;
    GCC_P_PLIST	*p_list = &rcb->gcc.p_list;
    u_i1	orig = rcb->request;

    switch( rcb->request )
    {
    case TLI_RECV_DONE :
	if ( ccb->gcc.flags & GCD_GCC_SHUT )
	    rcb->request = TLI_RECV_SHUT;
	else  if ( ccb->gcc.flags & GCD_GCC_XOFF )  
	    rcb->request = TLI_RECV_DONX;
	break;

    case TLI_SEND_DONE :
	if ( ! QUEUE_EMPTY(&ccb->gcc.send_q) )  rcb->request = TLI_SEND_DONQ;
	break;
    }

    if ( orig != rcb->request  &&  GCD_global.gcd_trace_level >= 4 )
	TRdisplay( "%4d    GCD gcc_eval %s => %s\n", 
		   ccb->id, inputs[ orig ], inputs[ rcb->request ] );

    return;
}


/*
** Name: gcc_complete
**
** Description:
**	Callback routine to handle GCC completions.
**
** Input:
**	ptr	Completion ID, a request control block.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	 4-Jun-99 (gordy)
**	    Created.
**	21-Dec-99 (gordy)
**	    Update client idle limit.
**   	10-Apr-01 (loera01) Bug 104459
**          Change default error input to JCI_ABRT_RQST instead of JCI_RQST_FAIL
**          and set error status.
**	20-Nov-02 (wansh01) 
**	    Set isLocal in CCB if input is JCI_LSTN_RQST.  
**	15-Jan-03 (gordy)
**	    Listen results now processed in FSM action.
*/

static void
gcc_complete( PTR ptr )
{
    GCD_RCB	*rcb = (GCD_RCB *)ptr;
    GCD_CCB	*ccb = rcb->ccb;
    GCC_P_PLIST	*p_list = &rcb->gcc.p_list;
    u_i1	orig = rcb->request;

    /*
    ** Update the idle time limit on any network completion.
    */
    if ( ! GCD_global.client_idle_limit )
	ccb->gcc.idle_limit.TM_secs = 0;
    else
    {
	TMnow( &ccb->gcc.idle_limit );
	ccb->gcc.idle_limit.TM_secs += GCD_global.client_idle_limit;
    }

    /*
    ** Check for request failure or convert request
    ** type to the associated request completion.  
    **
    ** We don't care if a disconnect request fails
    ** and we need to distinguish between send/recv 
    ** failures and the disconnect completion, so 
    ** always return TLI_RQST_DONE for disconnect.
    */
    if ( p_list->generic_status != OK )
	rcb->request = (rcb->request == TLI_DISC_RQST)
		       ? TLI_RQST_DONE : TLI_RQST_FAIL;
    else
	switch( rcb->request )
	{
	case TLI_RECV_RQST : rcb->request = TLI_RECV_DONE; break;
	case TLI_SEND_RQST : rcb->request = TLI_SEND_DONE; break;

	case TLI_OPEN_RQST : 
	case TLI_LSTN_RQST :
	case TLI_DISC_RQST : rcb->request = TLI_RQST_DONE; break;
    
	default :
	    p_list->generic_status = E_GC4809_INTERNAL_ERROR;
	    rcb->request = TLI_RQST_FAIL;
	    return;
	}

    if ( GCD_global.gcd_trace_level >= 4 )
	TRdisplay( "%4d    GCD GCC completion %s => %s, 0x%x\n", 
		   rcb->ccb->id, inputs[ orig ], inputs[ rcb->request ], 
		   p_list->generic_status );

    gcc_request( rcb );
    return;
}


/*
** Name: gcc_sm
**
** Description:
**	State machine processing loop for the GCD server GCC
**	processing.
**
** Inputs:
**	sid	ID of session to activate.
**
** Outputs:
**	None.
**
** Returns:
**	void
**
** History:
**	 3-Jun-99 (gordy)
**	    Created.
**	10-Sep-99 (gordy)
**	    Parameterized initial TL connection info.
**	18-Oct-99 (gordy)
**	    Added flow control to GCC/MSG sub-systems.
**	 4-Nov-99 (gordy)
**	    Added TL interrupts.
**	16-Nov-99 (gordy)
**	    Skip unknown connection parameters to permit
**	    future extensibility (client can't known our
**	    protocol level until we respond to the request).
**	13-Dec-99 (gordy)
**	    Negotiate GCC parameters prior to session start
**	    so that values are available to session layer.
**	20-Dec-02 (gordy)
**	    Packet header ID now protocol dependent.  Added
**	    TL connection parameter for character-set ID.
**	15-Jan-03 (gordy)
**	    TL packet processing moved to TL Service Providers.
**	 6-Jun-03 (gordy)
**	    Attach PIB as MIB object when OPEN completes.  Attach CCB 
**	    as MIB object when LISTEN completes, detach when freed.  
**	    Save local/remote addresses from LISTEN.
**	 4-Jan-05 (gordy)
**	    Free RCB when FSM error occurs since it won't be used again.
**	21-Aug-06 (usha)
**	    Added stats info for stats command in GCD admin interface.
**	 5-Dec-07 (gordy)
**	    Moved connection instantiation to gcc_accept() Service
**	    Provider entry point.
**      05-Jun-08 (Ralph Loen)  SIR 120457
**          Added display message on successfully opened network ports
**          to TLA_OCMP state to match equivalent on the GCC.
**      02-Dec-09 (rajus01) Bug: 120552, SD issue:129268
**	    When starting more than one GCC server the symbolic ports are not
**	    incremented (Example: II1, II2) in the startup messages, while the
**	    actual portnumber is increasing and printed correctly. This problem
**	    in all of the GC[B|C|D] servers.
**	    Log the actual_port_id for a successful network open request. 
**      22-Apr-10 (rajus01) SIR 123621
**          After a successful network open request update PIB with
**          the actual_port_id.
*/

static void
gcc_sm( GCD_RCB *rcb )
{
    GCD_CCB	*ccb = rcb->ccb;
    u_i4	ccb_id = ccb->id;
    GCC_P_PLIST	*p_list;
    STATUS	status;
    u_i1	i, next_state, action;

    gcc_eval( rcb );

    if ( ccb->gcc.state >= TLS_CNT  ||  rcb->request >= TLI_CNT  ||
	 smt[ ccb->gcc.state ][ rcb->request ].next >= TLS_CNT  ||
	 smt[ ccb->gcc.state ][ rcb->request ].action >= TLAS_CNT )
    {
	ER_ARGUMENT	args[2];

	if ( GCD_global.gcd_trace_level >= 1 )
	    TRdisplay( "%4d    GCD sm invalid %d %d => %d %d\n",
			ccb_id, ccb->gcc.state, rcb->request,
			smt[ ccb->gcc.state ][ rcb->request ].next,
			smt[ ccb->gcc.state ][ rcb->request ].action );

	args[0].er_size = sizeof( rcb->request );
	args[0].er_value = (PTR)&rcb->request;
	args[1].er_size = sizeof( ccb->gcc.state );
	args[1].er_value = (PTR)&ccb->gcc.state;
	gcu_erlog( 0, GCD_global.language, 
		   E_GC4810_TL_FSM_STATE, NULL, 2, (PTR)args );

	gcd_del_rcb( rcb );
	return;
    }

    if ( GCD_global.gcd_trace_level >= 3 )
	TRdisplay( "%4d    GCD state %s input %s => %s\n", ccb_id, 
		   states[ ccb->gcc.state ], inputs[ rcb->request ],
		   states[ smt[ ccb->gcc.state ][ rcb->request ].next ] );

    action = smt[ ccb->gcc.state ][ rcb->request ].action;
    next_state = smt[ ccb->gcc.state ][ rcb->request ].next;

    for( i = 0; i < TLA_MAX; i++ )
    {
	if ( 
	     GCD_global.gcd_trace_level >= 4  &&  
	     act_seq[action][i] != TLA_NOOP 
	   )
	    TRdisplay( "%4d    GCD action %s\n", ccb_id, 
		       actions[ act_seq[ action ][ i ] ] );

	switch( act_seq[ action ][ i ] )
	{
	case TLA_NOOP :		/* Do nothing, end of action sequence */
	    i = TLA_MAX;
	    break;

	case TLA_OPEN :		/* Issue GCC_OPEN request */
	    p_list = &rcb->gcc.p_list;
	    p_list->pce = ccb->gcc.pce;
	    p_list->function_invoked = GCC_OPEN;
	    p_list->function_parms.open.port_id = ccb->gcc.pib->addr;
	    p_list->function_parms.open.lsn_port = NULL;
	    p_list->function_parms.open.actual_port_id = NULL;
	    p_list->compl_exit = gcc_complete;
	    p_list->compl_id = (PTR)rcb;
	    p_list->pcb = NULL;
	    p_list->options = 0;
	    p_list->buffer_lng = 0;
	    p_list->buffer_ptr = NULL;

	    status = (*ccb->gcc.pce->pce_protocol_rtne)( GCC_OPEN, p_list );
	    if ( status != OK )
	    {
		rcb->request = TLI_RQST_FAIL;
		rcb->gcc.p_list.generic_status = status;
		gcc_request( rcb );
	    }
	    break;

	case TLA_OCMP :		/* GCC_OPEN completed */
	{
	    GCD_PIB	*pib = ccb->gcc.pib;
	    char	port[ GCC_L_PORT * 2 + 5 ];
	    ER_ARGUMENT	args[2];
            char        buffer[ GCC_L_PROTOCOL + GCC_L_PORT + 16 ];
	    p_list = &rcb->gcc.p_list;

	    /*
	    ** Process request results.
	    */
	    pib->flags |= PCT_OPEN;

	    if ( p_list->function_parms.open.lsn_port )
		STcopy(p_list->function_parms.open.lsn_port, pib->port);
	    else
		STcopy( pib->addr, pib->port );

	    MOattach( 0, GCD_MIB_PROTOCOL, pib->pid, (PTR)pib );

	    /*
	    ** Log successful OPEN
	    */
	    if ( ! STcompare( pib->addr, pib->port ) )
		STcopy( pib->addr, port );
            else if( p_list->function_parms.open.actual_port_id != NULL )
	    {
		STprintf( port, "%s (%s)", 
		    p_list->function_parms.open.actual_port_id, pib->port );
		STcopy(p_list->function_parms.open.actual_port_id, pib->addr );
	    }
	    else
		STprintf( port, "%s (%s)", pib->addr, pib->port );


	    args[0].er_size = STlength( pib->pid );
	    args[0].er_value = pib->pid;

	    if( p_list->function_parms.open.actual_port_id != NULL )
	    {
	        args[1].er_size = 
			STlength( p_list->function_parms.open.actual_port_id );
	        args[1].er_value = p_list->function_parms.open.actual_port_id;
	    }
	    else
	    {
	        args[1].er_size = STlength( port );
	        args[1].er_value = port;
	    }
	    gcu_erlog( 0, GCD_global.language, 
			E_GC4803_NTWK_OPEN, NULL, 2, (PTR)args );
            STprintf(buffer, "    %s port = %s\n", pib->pid, port);
            SIstd_write(SI_STD_OUT, buffer);
	    break;
	}
	
	case TLA_OERR :		/* GCC_OPEN failed */
	{
	    ER_ARGUMENT	args[2];

	    ccb->gcc.pib->flags |= PCT_OPN_FAIL;

	    /*
	    ** Log OPEN failure and check overall status.
	    */
	    args[0].er_size = 4;
	    args[0].er_value = "OPEN";
	    args[1].er_size = sizeof( rcb->gcc.p_list.generic_status );
	    args[1].er_value = (PTR)&rcb->gcc.p_list.generic_status;
	    gcu_erlog( 0, GCD_global.language, 
			E_GC4805_NTWK_RQST_FAIL, NULL, 2, (PTR)args );

	    gcc_check_protocols();
	    break;
	}

	case TLA_NEWC :		/* New connection (CCB) */
	{
	    GCD_CCB *lsn_ccb;
	    GCD_RCB *lsn_rcb;

	    /*
	    ** The listen RCB will ultimately be used as the
	    ** abort RCB (see TLA_LCMP) and does not require
	    ** a buffer.
	    */
	    if ( ! (lsn_ccb = gcd_new_ccb())  ||  
		 ! (lsn_rcb = gcd_new_rcb( lsn_ccb, 0 )) )
	    {
		if ( lsn_ccb )  gcd_del_ccb( lsn_ccb );

		/*
		** Can't issue new listen request, 
		** so treat same as request failure.
		*/
		ccb->gcc.pib->flags &= ~PCT_OPEN;
		ccb->gcc.pib->flags |= PCT_OPN_FAIL;
		gcc_check_protocols();
		break;
	    }

	    /*
	    ** Re-issue the listen request.
	    */
	    lsn_ccb->gcc.pib = ccb->gcc.pib;
	    lsn_ccb->gcc.pce = ccb->gcc.pce;
	    lsn_rcb->request = TLI_LSTN_RQST;
	    gcc_request( lsn_rcb );
	    break;
	}

	case TLA_LSTN :		/* Issue GCC_LISTEN request */
	    p_list = &rcb->gcc.p_list;
	    p_list->pce = ccb->gcc.pce;
	    p_list->function_invoked = GCC_LISTEN;
	    p_list->function_parms.listen.port_id = ccb->gcc.pib->port;
	    p_list->function_parms.listen.node_id = NULL;
	    p_list->function_parms.listen.lcl_addr = NULL;
	    p_list->function_parms.listen.rmt_addr = NULL;
	    p_list->compl_exit = gcc_complete;
	    p_list->compl_id = (PTR)rcb;
	    p_list->pcb = NULL;
	    p_list->options = 0;
	    p_list->buffer_lng = 0;
	    p_list->buffer_ptr = NULL;

	    status = (*ccb->gcc.pce->pce_protocol_rtne)( GCC_LISTEN, p_list );
	    if ( status != OK )
	    {
		rcb->request = TLI_RQST_FAIL;
		rcb->gcc.p_list.generic_status = status;
		gcc_request( rcb );
	    }
	    break;

	case TLA_LCMP :		/* GCC_LISTEN completed */
	{
	    /*
	    ** Process request results.
	    */
	    ccb->gcc.pcb = rcb->gcc.p_list.pcb;
	    ccb->isLocal = ((rcb->gcc.p_list.options & GCC_LOCAL_CONNECT) != 0);

	    if ( rcb->gcc.p_list.function_parms.listen.lcl_addr )
	    {
		STcopy( ccb->gcc.pib->pid, ccb->gcc.lcl_addr.protocol );
		STcopy( rcb->gcc.p_list.function_parms.listen.lcl_addr->node_id,
			ccb->gcc.lcl_addr.node_id );
		STcopy( rcb->gcc.p_list.function_parms.listen.lcl_addr->port_id,
			ccb->gcc.lcl_addr.port_id );
	    }

	    if ( rcb->gcc.p_list.function_parms.listen.rmt_addr )
	    {
		STcopy( ccb->gcc.pib->pid, ccb->gcc.rmt_addr.protocol );
		STcopy( rcb->gcc.p_list.function_parms.listen.rmt_addr->node_id,
			ccb->gcc.rmt_addr.node_id );
		STcopy( rcb->gcc.p_list.function_parms.listen.rmt_addr->port_id,
			ccb->gcc.rmt_addr.port_id );
	    }

	    /*
	    ** Save current RCB to be used if abort occurs.
	    */
	    ccb->gcc.abort_rcb = (PTR)rcb;
	    break;
	}

	case TLA_LERR :		/* GCC_LISTEN failed */
	{
	    ER_ARGUMENT	args[2];

	    args[0].er_size = 6;
	    args[0].er_value = "LISTEN";
	    args[1].er_size = sizeof( rcb->gcc.p_list.generic_status );
	    args[1].er_value = (PTR)&rcb->gcc.p_list.generic_status;
	    gcu_erlog( 0, GCD_global.language, 
			E_GC4805_NTWK_RQST_FAIL, NULL, 2, (PTR)args );

	    /*
	    ** Flag protocol as failed and check overall status.
	    */
	    ccb->gcc.pib->flags &= ~PCT_OPEN;
	    ccb->gcc.pib->flags |= PCT_OPN_FAIL;
	    gcc_check_protocols();
	    break;
	}

	case TLA_RECV :		/* Issue GCC_RECEIVE request */
	{
	    GCD_RCB *rcv_rcb;

	    if ( ! (rcv_rcb = gcd_new_rcb( ccb, -1 )) )
	    {
		gcc_abort( ccb, E_GC4808_NO_MEMORY );
		break;
	    }

	    /*
	    ** The RCB is initialized for building outbound
	    ** messages.  Reset for receiving TL packets.
	    ** Space has been reserved in the RCB buffer for
	    ** the NL Header.  The protocol driver expects
	    ** space prior to the receive buffer for the NL
	    ** header, so set parameters accordingly.
	    */
	    rcv_rcb->buf_ptr = rcv_rcb->buffer + GCD_global.nl_hdr_sz;
	    rcv_rcb->buf_len = 0;
	    rcv_rcb->request = TLI_RECV_RQST;

	    p_list = &rcv_rcb->gcc.p_list;
	    p_list->pce = ccb->gcc.pce;
	    p_list->pcb = ccb->gcc.pcb;
	    p_list->function_invoked = GCC_RECEIVE;
	    p_list->compl_exit = gcc_complete;
	    p_list->compl_id = (PTR)rcv_rcb;
	    p_list->options = 0;
	    p_list->buffer_lng = RCB_AVAIL( rcv_rcb );
	    p_list->buffer_ptr = (char *)rcv_rcb->buf_ptr;

	    status = (*ccb->gcc.pce->pce_protocol_rtne)( GCC_RECEIVE, p_list );
	    if ( status != OK )  
	    {
		if ( GCD_global.gcd_trace_level >= 1 )
		    TRdisplay( "%4d    GCD GCC receive error: 0x%x\n",
			       ccb_id, status );

		rcv_rcb->request = TLI_RQST_FAIL;
		rcv_rcb->gcc.p_list.generic_status = status;
		gcc_request( rcv_rcb );
	    }
	    break;
	}

	case TLA_DATA : 	/* Process GCC_RECEIVE data */
            {
            static gcd_data_in_bits = 0;

	    p_list = &rcb->gcc.p_list;
	    rcb->buf_len += p_list->buffer_lng;

	    if ( GCD_global.gcd_trace_level >= 5 )
	    {
		TRdisplay( "%4d    GCD received %d bytes\n",
			   ccb->id, rcb->buf_len );
		gcu_hexdump( (char *)rcb->buf_ptr, rcb->buf_len );
	    }

	    ccb->gcc.gcd_msgs_in++;
            ccb->gcc.gcd_data_in +=
		     (gcd_data_in_bits + p_list->buffer_lng) / 1024;
            gcd_data_in_bits =  (gcd_data_in_bits + p_list->buffer_lng) % 1024;

	    if ( ccb->gcc.tl_service )  
		(*ccb->gcc.tl_service->data)( rcb );	/* Pass to TL */
	    else
		gcc_get_service( rcb );	/* Search for TL Service Provider */
            }
	    break;

	case TLA_SEND :		/* Issue GCC_SEND request */
          {
            static  gcd_data_out_bits = 0;

	    rcb->request = TLI_SEND_RQST;
	    p_list = &rcb->gcc.p_list;
	    p_list->pce = ccb->gcc.pce;
	    p_list->pcb = ccb->gcc.pcb;
	    p_list->function_invoked = GCC_SEND;
	    p_list->compl_exit = gcc_complete;
	    p_list->compl_id = (PTR)rcb;
	    p_list->options = GCC_FLUSH_DATA;
	    p_list->buffer_lng = rcb->buf_len;
	    p_list->buffer_ptr = (char *)rcb->buf_ptr;

	    if ( GCD_global.gcd_trace_level >= 5 )
	    {
		TRdisplay( "%4d    GCD sending %d bytes\n",
			   ccb_id, p_list->buffer_lng );
		gcu_hexdump( p_list->buffer_ptr, p_list->buffer_lng );
	    }

	    status = (*ccb->gcc.pce->pce_protocol_rtne)( GCC_SEND, p_list );
	    if ( status != OK )  
	    {
		if ( GCD_global.gcd_trace_level >= 1 )
		    TRdisplay( "%4d    GCD GCC send error: 0x%x\n",
			       ccb_id, status );

		rcb->request = TLI_RQST_FAIL;
		rcb->gcc.p_list.generic_status = status;
		gcc_request( rcb );
	    }
            
            ccb->gcc.gcd_msgs_out++;
	    ccb->gcc.gcd_data_out +=
			 (gcd_data_out_bits + p_list->buffer_lng) / 1024;  
            gcd_data_out_bits = (gcd_data_out_bits + p_list->buffer_lng) % 1024;
           }
	    break;

	case TLA_QEUE :		/* Queue GCC_SEND request, disable IO */
	    QUinsert( &rcb->q, ccb->gcc.send_q.q_prev );
	    if ( ccb->gcc.tl_service )
		(*ccb->gcc.tl_service->xoff)( ccb, TRUE );	/* XOFF */
	    break;

	case TLA_DEQU :		/* Get GCC_SEND request from queue */
	    rcb = (GCD_RCB *)QUremove( ccb->gcc.send_q.q_next );
	    ccb = rcb->ccb;	/* Shouldn't change! */
	    ccb_id = ccb->id;

	    /* When the send queue is empty, enable outbound IO */
	    if ( QUEUE_EMPTY( &ccb->gcc.send_q )  &&  ccb->gcc.tl_service )
		(*ccb->gcc.tl_service->xoff)( ccb, FALSE );	/* XON */
	    break;

	case TLA_FLSH :		/* Flush GCC_SEND request queue */
	{
	    GCD_RCB *frcb;

	    while( (frcb = (GCD_RCB *)QUremove(ccb->gcc.send_q.q_next)) )
		gcd_del_rcb( frcb );
	    break;
	}

	case TLA_DISC :		/* Issue GCC_DISCONNECT request */
	    /*
	    ** Shutdown the client connection.
	    */
	    rcb->request = TLI_DISC_RQST;
	    p_list = &rcb->gcc.p_list;
	    p_list->pce = ccb->gcc.pce;
	    p_list->pcb = ccb->gcc.pcb;
	    p_list->function_invoked = GCC_DISCONNECT;
	    p_list->compl_exit = gcc_complete;
	    p_list->compl_id = (PTR)rcb;
	    p_list->options = 0;
	    p_list->buffer_lng = 0;
	    p_list->buffer_ptr = NULL;

	    status = (*ccb->gcc.pce->pce_protocol_rtne)( GCC_DISCONNECT, 
							 p_list );
	    if ( status != OK )  
	    {
		if ( GCD_global.gcd_trace_level >= 1 )
		    TRdisplay( "%4d    GCD GCC disconnect error: 0x%x\n",
			       ccb_id, status );

		rcb->request = TLI_RQST_DONE;	/* Don't fail disconnect */
		gcc_request( rcb );
	    }
	    break;

	case TLA_ABRT :		/* Abort connection */
	    if ( rcb->gcc.p_list.generic_status != OK  &&  
		 rcb->gcc.p_list.generic_status != FAIL )
	    {
		gcu_erlog( 0, GCD_global.language, 
			   E_GC4807_CONN_ABORT, NULL, 0, NULL );
		gcu_erlog( 0, GCD_global.language, 
			   rcb->gcc.p_list.generic_status, NULL, 0, NULL );
	    }

	    /*
	    ** Inform the TL Service Provider that the connection
	    ** is being aborted.  Drop the service so that no other
	    ** requests are forwarded and mark connection as being
	    ** shutdown.
	    */
	    if ( ccb->gcc.tl_service )
	    {
		(*ccb->gcc.tl_service->abort)( ccb );
		ccb->gcc.tl_service = NULL;
		ccb->gcc.flags |= GCD_GCC_SHUT;
	    }
	    break;

	case TLA_FRCB :		/* Free RCB */
	    gcd_del_rcb( rcb );
	    rcb = NULL;
	    break;

	case TLA_FCCB :		/* Free CCB & RCB */
	    if ( ccb->gcc.flags & GCD_GCC_CONN )
	    {
		char	buff[16];

		GCD_global.client_cnt--;
		ccb->gcc.flags &= ~GCD_GCC_CONN;
		MOulongout( 0, (u_i8)ccb->id, sizeof( buff ), buff );
		MOdetach( GCD_MIB_CONNECTION, buff );
	    }

	    gcd_del_rcb( rcb );
	    rcb = NULL;
	    ccb->gcc.state = next_state;	/* Won't get done below */
	    gcd_del_ccb( ccb );
	    ccb = NULL;
	    break;
	}
    }

    /*
    ** Make the state transition now that actions have been performed.
    */
    if ( ccb )  ccb->gcc.state = next_state;
    return;
}

