/*
** Copyright (c) 2001, 2005 Ingres Corporation
**
**
*/

#include    <compat.h>
#include    <pc.h>
#include    <cs.h>
#include    <er.h>
#include    <lk.h>
#include    <pm.h>
#include    <cx.h>
#include    <cxprivate.h>
#include    <me.h>
#include    <nm.h>
#include    <lo.h>
#include    <di.h>
#include    <st.h>
#include    <tr.h>
#include    <mh.h>

/**
**
**  Name: CXMSG.C - Cluster broadcast message support.
**
**  Description:
**
**      This module contains routines which implement a means to
**	broadcast with confirmation of receipt messages to all
**	members of an Ingres cluster which have connected to
**	a message channel.
**
**	This is the primary tool used to maintain cluster
**	state transitions, such as nodes joining or leaving the
**	the cluster.
**
**	Implementation is based on the CX DLM.   The following
**	protocol is used to exploit the blocking notification
**	feature of the DLM to pass short messages in the lock
**	value blocks.
**
**	Description of Hello/Goodbye broadcast protocol:
**	
**	- Writer obtains "Send" lock in X mode.  This serializes message
**	  broadcasts for a channel, and also prevents sending a message
**	  while a channel is shutting down.
**	  
**	- Writer then requests "Hello" lock in NL mode, which should be
**	  granted synchronously.   "Hello" lock is then converted to X
**	  mode which will block against the "Hello" locks held in S mode,
**	  by the receivers.   We do this is two steps, so the X request is
**	  on the conversion queue.  A direct request for X mode would be
**	  on the wait queue, and since conversion requests compatible with
**	  the group grant mode are granted, even if there are waiters
**	  under the Tru64 DLM (and others?), this request must be on
**	  the conversion queue, and flags specifying converters must wait
**	  behind converters must be specified.
**	  
**	- Blocking notification routine for "Hello" lock is triggered, 
**	  and "monitor" thread is resumed.   This thread checks all the
**	  channels for pending messages, and if a message is pending,
**	  the "receive" (connection) thread for that channel is resumed.
**	  The reason two threads are involved is that if the "receive"
**	  thread were to be resumed for both blocking and completion
**	  notifications, confusion could result if receiver was
**	  waiting for a completion resume, and was resumed by the
**	  blocking notification, or even worst, if both resumes occured
**	  before a single suspend, the blocking notification would be
**	  lost at the Ingres level.
**
**	  Note: It also appears that under Tru64 5.0a, blocking notification
**	  may be lost at the signal level.
**	  
**	- If the receiver process has not yet allocated its "Goodbye" lock,
**	  it does so at this point.    This request for shared access should 
**	  always be granted, since so long as this protocol is followed, no 
**	  process could be holding "Goodbye" in exclusive mode.
**	  
**	- Reader then converts its lock on "Hello" lock NL mode, and 
**	  then immediately requests conversion of this lock back to share 
**	  mode.    This conversion will sit behind writers request for 
**	  exclusive access, and reader will block here until writer has 
**	  released his "Hello" lock.
**	  
**	- Writer when granted finally granted access to the "Hello" 
**	  lock resource, request a NL lock on "Goodbye", then queues
**	  a conversion request for an X lock.   Reasons are the same as
**	  for the two step acquisiton of the X lock on "Hello".
**	  
**	- Writer then calls the user function passed to the CXmsg_send
**	  routine, and on return updates the "Hello" lock value block 
**	  with the message calculated, and then releases his "Hello"
**	  lock.
**	  
**	- Writer now waits for completion of his exclusive lock request on 
**	  the "Goodbye" lock resource.
**	  
**	- The readers which are now granted share access to the "Hello" lock 
**	  resource, read the lock value.
**	  
**	- Reader processes message by passing it to the function registered 
**	  with CXmsg_connect.   If by chance, lock value block is marked  
**	  invalid, "invalid" flag is set to true, and compensatory actions
**	  are required.
**	   
**	- On return from function, readers convert their "Goodbye" locks to  
**	  NL, and put an asynchronous conversion request to convert this 
**	  lock back to share mode.   When this convert is granted, it is 
**	  known that all readers have either processed the message or died.
**	  
**	- When writer is granted exclusive access to the "Goodbye" lock 
**	  resource, writer knows that all existing readers have acted on 
**	  the message.
**	  
**	- Finally the writer releases locks held on "Goodbye" and "Send"
**	  locks. 
**
**	- The cycle is complete, and all processes are in a consistent 
**	  state.
**	  
**	"Hello" lock has the key LK_MSG, ( ( <channel> <<2 ) + 0 ).
**	"Goodbye" lock has the key LK_MSG, ( ( <channel> <<2 ) + 1 ).
**	"Send" lock has the key LK_MSG, ( ( <channel> <<2 ) + 2 ).
**
**	Public routines
**
**          CXmsg_monitor	- Dedicate current thread to monitoring
**				  for pending messages.
**
**	    CXmsg_shutdown	- Tell message system to shutdown.
**
**          CXmsg_connect	- Dedicate current thread to receiving
**				  messages for a channel.
**
**	    CXmsg_disconnect 	- Release resources held for this channel.
**
**	    CXmsg_channel_ready - Check if channel is locally ready to receive.
**
**	    CXmsg_send		- Broadcast a message to a channel.
**
**	    ---- Following relate to "long" messages ----
**
**	    CXmsg_stow		- Store a "long" message for later
**				  retrieval by any cluster member
**				  through use of a "chit", or reserve
**				  space for such a message.
**
**	    CXmsg_append	- Store a "long" message into space
**				  previously reserved by CXmsg_stow.
**
**	    CXmsg_redeem	- Retrieve "long" message in whole or
**			  	  in part, via a chit presumably 
**				  distributed by CXmsg_send or in
**				  a DLM lock value block, or some
**				  other cluster wide means.
**
**	    CXmsg_release	- Release resources held by a "chit".
**
**	    CXmsg_clmclose	- Release CLM resource for a node.
**
**	Internal routines
**
**	    cx_msg_receive	- Body of message receive processing
**				  broken out of CXmsg_connect for
**				  clarity.
**
**	    cx_msg_blk_rtn	- Blocking notification routine for
**				  marking channels as having a pending
**				  message, and resuming the monitor
**				  thread.
**
**	    cx_mcb_format	- Format an MCB.
**
**	    cx_msg_clm_lock	- Reserve CLM for a node.
**
**          cx_msg_clm_unlock	- Release CLM for a node.
**
**          cx_msg_clm_file_init - Initialize CX_CLM_FILE structures.
**
**	    cx_clm_file_reserve	- Put dibs on a piece of the CLM file.
**
**	    cx_clm_file_write	- Write out data to shared CLM file.
**
**	    cx_clm_file_read	- Read in data from shared CLM file.
**
**	    cx_clm_file_force	- Write full blocks to clm file.
**
**	    cx_clm_file_fault	- Read full blocks from clm file.
**
[@func_list@]...
**
**
**  History:
**      01-Mar-2001 (devjo01)
**          Created.
**	12-sep-2002 (devjo01)
**	    Split CXmsg_stow into CXmsg_stow & CXmsg_append.  Add 'offset'
**	    parameter to CXmsg_redeem.
**	24-oct-2003 (kinte01)
**	    Add include of me.h
**	08-jan-2004 (devjo01)
**	    Linux support.
**	11-feb-2004 (devjo01)
**	    Address unlikely race scenario, where CXmsg_disconnect
**	    is called while message processing is in progress.
**	29-feb-2004 (devjo01)
**	    Correct instance in CXmsg_send, where we were passing
**	    a pointer to the wrong lock.  Renamed variables used
**	    to reference lock control blocks for better clarity.
**	28-mar-2004 (devjo01)
**	    Correctly return E_CL2C10_CX_E_NOSUPPORT in the various
**	    long message functions as needed.
**	04-apr-2004 (devjo01)
**	    Add CX_CLM_FILE support as a message passing mechanism
**	    of last resort which passes messages through shared files.
**	23-apr-2004 (devjo01)
**	    Carelessly wrote CSMTw_semaphore where CSw_semaphore was
**	    intended.  Correcting this mistake.
**	28-apr-2004 (devjo01)
**	    Replace bad ref to 'amount' where 'length' was intended
**          in cx_clm_file_write().
**	07-may-2004 (devjo01)
**	    Add tr.h to surpress implicit declaration warning on VMS.
**	14-may-2004 (devjo01)
**	    Add CXmsg_clmclose to close CLM connections to foreign
**	    nodes.
**      15-Sep-2004 (devjo01)
**          Fixed bad tests in CX_PARANOIA sections for cx_clm_file_read
**          and CXmsg_clmclose.
**      04-nov-2004 (thaju02)
**          Added alloc in cx_msg_clm_file_init(). Changed num_pages in 
**          cx_clm_file_read() and cx_clm_file_write() to SIZE_TYPE.
**	02-nov-2004 (devjo01)
**	    Add CX_F_NO_DOMAIN to CXdlm_request calls to keep CMO
**	    locks out of the recovery domain.
**	08-feb-2005 (devjo01)
**	    Use AST version of suspend/resume for message monitor thread
**	    to avoid rare missed BAST problem.
**	01-mar-2005 (mutma03/devjo01) to fix star issue 13990057 (dbevent does
**          not get raised in clustered environment) added code to force out
**	    actual allocation of disc space for the cx_clm_msg_file. Fixed
**	    cx_clm_file_read to return actual message. 
**	12-apr-2005 (devjo01)
**	    Use new CX_MSG struct instead of overloading CX_CMO so as to
**	    allow synchronized short messages of 32 bytes.
**	20-jun-2005 (devjo01)
**	    'cx_key' is now a struct, rather than a pointer.
**	    Change initializations to reflect this.
**      30-Jun-2005 (hweho01)
**          Make the inclusion of cxmsg implementations dependent on  
**          define string conf_CLUSTER_BUILD for axp_osf platform.
**      03-nov-2010 (joea)
**          Declare cx_clm_file_reserve and cx_clm_file_read as static.
**/


/*
**	OS independent declarations
*/
GLOBALREF	CX_PROC_CB	CX_Proc_CB;

GLOBALREF	CX_STATS	CX_Stats;

static		CX_MM_CB	CX_Msg_Mon = { 0, };

static		CX_MSG_CB	CX_Msg_CB[CX_MAX_CHANNEL];

/*
**	Macros & local defines
*/
#define CX_MSG_HELLO_KEY	0
#define CX_MSG_GOODBYE_KEY	1
#define CX_MSG_SEND_KEY		2

#define CX_CLMF_SEQBITS		24	/* Bits for CLMF chit sequence #s*/

/*
** Macros for calculating lk_key1 value for LK_MSG lock keys.
**
** Regular short msg keys will always be < 256, while
** CLM keys will always be >= 256.  Actual current usage
** with 16 nodes and 2 channels is 0..0x6, and 0x0100..0x1000.
*/
#define	CX_MSG_KEY(channel,subkey) (((channel)<<2)+(subkey))
#define CX_CLM_KEY(node)	   ((node)<<8)

/*
** Extract Node portion of chit.
*/
#define CX_CLMF_NODE_FROM_CHIT(chit) \
 (((chit) & ~((1 << CX_CLMF_SEQBITS)-1)) >> CX_CLMF_SEQBITS)
 
/*
** Extract sequence portion of chit.
*/
#define CX_CLMF_SEQ_FROM_CHIT(chit) \
 ((chit) & ((1 << CX_CLMF_SEQBITS)-1))
 
/*
** Calculate chit value for CLM file implementation.  Top bits of chit 
** encode the node number, while bottom bits keep the sequence LSBs.
*/
#define CX_CLMF_CHIT(node, sequence) \
 (((node) << CX_CLMF_SEQBITS)|CX_CLMF_SEQ_FROM_CHIT(sequence))

/*
** What page s/b used for a given sequence # in the CLM file implementation.
*/
#define CX_CLMF_PAGE_FROM_SEQ(sequence) \
 ((sequence) % CX_Proc_CB.cx_clm_blocks)

#define CX_CLMF_PAGES(length) \
 (((length) + CX_Proc_CB.cx_clm_blksize - 1) / CX_Proc_CB.cx_clm_blksize )

/*
** What page s/b used for a given chit in CLM file implementation.
*/
#define CX_CLMF_PAGE_FROM_CHIT(chit) \
  CX_CLMF_PAGE_FROM_SEQ(CX_CLMF_SEQ_FROM_CHIT(chit))
 
/*
** Adjust external offset/length to allow for header.
*/
#define CX_CLMF_ADJ_OFFSET(offset) \
 ((offset) + sizeof(CX_MSG_HEAD))

/*
** Calc chit for the buffer holding chit plus an offset.
**
** Macro preserves high bits used for node, adds how many buffers are
** skipped by the offset to the extracted sequence number, then
** masks that sum back into the sequence number portion of the field,
** JIC adding the offset overflows the sequence number into the node.
*/
#define CX_CLMF_CHIT_PLUS(chit,offset) \
 ( ((chit) & ~((1 << CX_CLMF_SEQBITS)-1)) | \
   ((((chit) & ((1 << CX_CLMF_SEQBITS)-1)) + \
    (CX_CLMF_ADJ_OFFSET(offset)/CX_Proc_CB.cx_clm_blksize)) & \
     ((1 << CX_CLMF_SEQBITS)-1)) )

/*
**	Forward references.
*/
static VOID cx_msg_blk_rtn( CX_REQ_CB *preq, i4 );
static STATUS cx_msg_receive( CX_MSG_CB *pmcb ); 
static STATUS cx_mcb_format( u_i4 channel, CX_MSG_CB *pmcb );

#if defined(axp_osf) && defined(conf_CLUSTER_BUILD)
static STATUS cxmsg_axp_grab_msg_bits( CX_CMO *oldval, CX_CMO *pnewval,
                        PTR pdata, bool invalidin );
static STATUS cxmsg_axp_free_msg_bits( CX_CMO *oldval, CX_CMO *pnewval,
                        PTR pdata, bool invalidin );
#endif


/*{
** Name: CXmsg_monitor          - Body of code for message monitor thread.
**
** Description:
**
**      This routine performs the task of redispatching message receive
**	threads.   When a blocking notification is received on a
**	channels "Hello" lock, the thread running THIS code is resumed.
**	This prevents any problems from simultaneously receiving
**	completion and blocking notifications in the context of the
**	same thread.
**
**	This routine will check each of the message channels opened,
**	and if a message is pending, will not suspend itself indefinitly
**	until it sees the message channel is ready, and has issued
**	a resume.
**
**	One way around this awkward two thread system, would be to
**	allow thread suspension on a particular event, with the
**	triggering of another event remembered, so that when the
**	first suspend is satisfied, the thread would immediately
**	resume on the second suspend, event if the event that
**	satifies the second suspend, occured befor the first suspend.
**
**	We would then be able to keep the resumes for blocking notification
**	and lock completion separate, and even queue up multiple lock 
**	requests, and assure that we will process them in the desired order.
**
** Inputs:
**	none 
**
** Outputs:
**	none 
**
**	Returns:
**		OK			- Normal successful completion.
**		E_CL2C1B_CX_E_BADCONTEXT - CXmsg_monitor already called.
**		
**	Exceptions:
**	    none
**
** Side Effects:
**	    CX_Msg_Mon.cx_mm_sid set.
**
** History:
**	13-mar-2001 (devjo01)
**	    Created.
*/
STATUS
CXmsg_monitor( void )
{
    STATUS		 status;
    u_i4		 suspectlocks, spin, maxpollint, cleancycles, channel;
    i4			 checklocks;
    CX_MSG_CB		*pmcb;

# ifdef CX_PARANOIA
    if ( CX_ST_UNINIT == CX_Proc_CB.cx_state ||
	 CX_Msg_Mon.cx_mm_online )
	return E_CL2C1B_CX_E_BADCONTEXT;
# endif /*CX_PARANOIA*/

    CSget_sid( &CX_Msg_Mon.cx_mm_sid );
    CX_Msg_Mon.cx_mm_online = 1;

    /* <INC> get this from config */
    maxpollint = 0;
	
    CX_Stats.cx_msg_mon_poll_interval = maxpollint;
    cleancycles = 0;

    while ( CX_Proc_CB.cx_state != CX_ST_DYING && CX_Msg_Mon.cx_mm_online )
    {
	/* 
	** Wait for something to do, optionally checking periodically
	** that a blocking notification has not been missed.
	*/
	checklocks = FALSE;
	status = CSsuspend_for_AST( CX_Stats.cx_msg_mon_poll_interval ?
			    CS_TIMEOUT_MASK : 0, 
			    - CX_Stats.cx_msg_mon_poll_interval, 0 );
	if ( OK != status )
	{
	    /* Resumed for periodic missed notification check */
	    CScancelled( (PTR)0 );
	    suspectlocks = 0;
	    checklocks = TRUE;
	}
	else if ( maxpollint )
	{
	    /* Normal resume in response to blocking notification */
	    cleancycles++;
	}

	do
	{
	    spin = 0;
	    for ( channel = 0; channel < CX_MAX_CHANNEL; channel++ )
	    {
		pmcb = CX_Msg_CB + channel;
		if ( pmcb->cx_msg_pending )
		{
		    if ( !pmcb->cx_msg_ready )
		    {
			/* Channel is still processing last message. */
			spin = 1;
		    }
		    else
		    {
			pmcb->cx_msg_pending = FALSE;

			/* Activate channel */
			CSresume( pmcb->cx_msg_hello_lkreq.cx_owner.sid );
		    }
		}
		else if ( checklocks && pmcb->cx_msg_ready )
		{
		    if ( CXdlm_is_blocking( &pmcb->cx_msg_hello_lkreq ) )
		    {
			/*
			** Lock is a blocking lock, double check
			** "pending" to make sure this really
			** was a missed notification, then kick off
			** receive regardless.
			*/
			if ( !pmcb->cx_msg_pending )
			{
			    /*
			    ** Probable missed notificaton,
			    ** decrease poll interval.  It may be possible,
			    ** that DLM structures have been updated and
			    ** signal is about to be issued, but JIC.
			    */
			    if ( suspectlocks & ( 1 << channel ) )
			    {
				/* Virtually certain to be a missed block */
				if ( CX_Stats.cx_msg_mon_poll_interval > 1 )
				{
				    CX_Stats.cx_msg_mon_poll_interval = 
				     CX_Stats.cx_msg_mon_poll_interval / 2;
				}
				CX_Stats.cx_msg_missed_blocks++;
				cleancycles = 0;
			    }
			    else
			    {
				/* Check again in 1 ms. */
				suspectlocks |= ( 1 << channel );
				spin = 1;
			    }
			}
			if ( !spin )
			{
			    pmcb->cx_msg_pending = FALSE;
			    CSresume( pmcb->cx_msg_hello_lkreq.cx_owner.sid );
			}
		    } /* is blocking */
		}
	    } /* end-for */

	    if ( spin )
	    {
		/* Take a short break & check again */
		status = CSsuspend_for_AST( CS_TIMEOUT_MASK, -1, 0 );
		if ( status )
		    CScancelled( (PTR)0 );
	    }
	} while ( spin );

	if ( cleancycles > 10 )
	{
	    cleancycles = 0;
	    CX_Stats.cx_msg_mon_poll_interval
	     += CX_Stats.cx_msg_mon_poll_interval;
	    if ( CX_Stats.cx_msg_mon_poll_interval > maxpollint )
		CX_Stats.cx_msg_mon_poll_interval = maxpollint;
	}
    } /* end-while */

    /*
    ** Channels should have been disconnect first, but just in case.
    */
    for ( channel = 0; channel < CX_MAX_CHANNEL; channel++ )
    {
	if ( CXmsg_channel_ready( channel ) )
	    (void)CXmsg_disconnect( channel );
    }
    return status;
} /* CXmsg_monitor */



/*{
** Name: CXmsg_shutdown		- Tell message system to terminate.
**
** Description:
**
**      This routine flags message monitor session for shutdown,
**	then resumes session so it can clean itself up.
**
** Inputs:
**	none 
**
** Outputs:
**	none 
**
**	Returns:
**		OK			- Normal successful completion.
**		E_CL2C1B_CX_E_BADCONTEXT - CXmsg_monitor not on-line
**		
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	23-mar-2001 (devjo01)
**	    Created.
*/
STATUS
CXmsg_shutdown(void)
{
# ifdef CX_PARANOIA
    if ( CX_ST_UNINIT == CX_Proc_CB.cx_state ||
	 !CX_Msg_Mon.cx_mm_online )
	return E_CL2C1B_CX_E_BADCONTEXT;
# endif /*CX_PARANOIA*/

    CX_Msg_Mon.cx_mm_online = 0;
    /* Not really in AST, but MMT expects this style resume. */
    CSresume_from_AST( CX_Msg_Mon.cx_mm_sid );
    return OK;
} /* CXmsg_shutdown */



/*{
** Name: CXmsg_connect          - Body of code for a message channel thread.
**
** Description:
**
**      This routine performs the task of managing a message channel.
**	Normally this thread is suspended, until resumed indirectly
**	through a blocking notification waking the monitor thread.
**
**	All message locks for this channel are held in the context of 
**	a private transaction ID for this thread.  
**
**	To prevent deadlock, and to maximize messaging efficiancy,
**	responses to messages which require obtaining locks, or need
**	to send messages or will take significant processing time, or
**	which may block in any way, should be done in the context of
**	yet another thread which would be dispatched by the function
**	registered for this channel.
**
**	Before connection can be activated, message monitor thread
**	must be active.
**
** Inputs:
**      channel 	- Index number of channel [ 0 .. CX_MAX_CHANNEL - 1 ]
**
**	pfunc		- Pointer to user's message receive processing function.
**
**	pdata		- Pointer to optional auxilliary application data.
**			  This address must remain valid while channel is
**			  connected.
**
** Outputs:
**	none 
**
**	Returns:
**		OK			- Normal successful completion.
**		E_CL2C11_CX_E_BADPARAM	- Bad channel #.
**		E_CL2C1B_CX_E_BADCONTEXT - CXconnect not yet called.
**		Other codes may be propagated from failed DLM calls.
**		
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	01-mar-2001 (devjo01)
**	    Created.
*/
STATUS
CXmsg_connect( u_i4 channel, CX_MSG_RCV_FUNC *pfunc, PTR pdata )
{
    STATUS		 status;
    CX_MSG_CB		*pmcb;
    CX_REQ_CB		*plk;

    pmcb = CX_Msg_CB + channel;

# ifdef CX_PARANOIA
    if ( channel > CX_MAX_CHANNEL )
	return E_CL2C11_CX_E_BADPARAM;
    if ( CX_ST_UNINIT == CX_Proc_CB.cx_state || 
	 CX_NONZERO_ID( &pmcb->cx_msg_transid ) )
	return E_CL2C1B_CX_E_BADCONTEXT;
# endif /*CX_PARANOIA*/

    /*
    ** Confirm that message monitor thread is running, if
    ** not, poll until it is, or enough time has gone by
    ** to indicate a logic error or monitor failure.
    **
    ** This guards against possible race conditions in which 
    ** the message connection thread is scheduled ahead of the 
    ** monitor thread, and winds up triggering a blocking 
    ** notification before the monitor is ready.
    **
    ** Amendment to above: Keep polling indefinitely, since
    ** if main CSP thread is scheduled ahead of this, and
    ** is performing 1st CSP recovery, then monitor thread
    ** may not start up for some time.
    */
    while ( !CX_Msg_Mon.cx_mm_online )
    {
	CSsuspend( CS_TIMEOUT_MASK, 1, 0 );
    }

    status = cx_mcb_format( channel, pmcb );

    if ( OK == status )
    {
	pmcb->cx_msg_rcvfunc = pfunc;
	pmcb->cx_msg_rcvauxdata = pdata;

	/* Finish init "Hello" lock */
	plk = &pmcb->cx_msg_hello_lkreq;
	plk->cx_new_mode = LK_S;
	plk->cx_user_func = cx_msg_blk_rtn;
	plk->cx_user_extra = (PTR)pmcb;

	pmcb->cx_msg_pending = FALSE;
	pmcb->cx_msg_ready = TRUE;

	/* 
	** We should not get a deadlock here, since message XACT is not 
	** holding any other locks.  
	*/
	status = CXdlm_request( CX_F_STATUS | CX_F_PCONTEXT | CX_F_NO_DOMAIN,
				plk, &pmcb->cx_msg_transid );
	if ( OK == status )
	    status = CXdlm_wait( 0, plk, 0 );

	if ( !CX_ERR_STATUS(status) )
	{
	    /* Don't care yet if value block is invalid. */
	    status = OK;
	}

    }

    /*
    ** Session will spend it's life looping here, until disconnect
    ** is requested by registered message processing function, or
    ** by CXmsg_disconnect.
    */
    while ( OK == status && CX_NONZERO_ID( &pmcb->cx_msg_transid ) )
    {
	status = CSsuspend( 0, 0, 0 );
	if ( OK != status )
	{
	    /* Unexpected error */
	    break;
	}
	if ( !CX_NONZERO_ID( &pmcb->cx_msg_transid ) )
	{
	    /* CXmsg_disconnect has been called */
	    break;
	}
	pmcb->cx_msg_ready = FALSE;
	status = cx_msg_receive( pmcb );
	if ( OK == status )
	    pmcb->cx_msg_ready = TRUE;
    }

    if ( E_CL2C05_CX_I_DONE == status )
    {
	/* Convert to standard success code. */
	status = OK;
    }
    return status;
} /* CXmsg_connect */



/*{
** Name: CXmsg_disconnect          - Disconnect from a message channel.
**
** Description:
**
**      This routine frees any resources held for the connection to
**	a message channel.
**
** Inputs:
**      channel 	- Index number of channel [ 0 .. CX_MAX_CHANNEL - 1 ]
**
** Outputs:
**	none 
**
**	Returns:
**		OK			- Normal successful completion.
**		E_CL2C11_CX_E_BADPARAM	- Bad channel #.
**		E_CL2C1B_CX_E_BADCONTEXT - CXconnect not yet called.
**		Other codes may be propagated from failed DLM calls.
**		
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	01-mar-2001 (devjo01)
**	    Created.
**	11-feb-2004 (devjo01)
**	    Address unlikely race scenario, where CXmsg_disconnect
**	    is called while message processing is in progress.
*/
STATUS
CXmsg_disconnect( u_i4 channel )
{
# define CX_MSG_DISCONNECT_WAIT	10

    STATUS		 status, rstatus;
    CX_REQ_CB		 slk;
    CX_MSG_CB		*pmcb;
    CS_SID		 channelsid;
    LK_UNIQUE		 tranid;
    i4			 countdown;

    pmcb = CX_Msg_CB + channel;


# ifdef CX_PARANOIA
    if ( channel > CX_MAX_CHANNEL )
	return E_CL2C11_CX_E_BADPARAM;
    if ( CX_ST_UNINIT == CX_Proc_CB.cx_state || 
	 !CX_NONZERO_ID( &pmcb->cx_msg_transid ) )
	return E_CL2C1B_CX_E_BADCONTEXT;
# endif /*CX_PARANOIA*/

    (void)MEfill( sizeof(CX_REQ_CB), '\0', (PTR)&slk );

    while ( CX_NONZERO_ID(&pmcb->cx_msg_transid) )
    {
	channelsid = pmcb->cx_msg_hello_lkreq.cx_owner.sid;

	/*
	** Step 1: Generate our ad'hoc tranid.
	** 
	** We are not running in same session as channel
	** processing code, so we should not use the
	** transaction associated with the message channel.
	** This removes any chance of a spurious deadlock,
	** and satisfies the CX DLM's need for a transid.
	*/
	status = CXunique_id( &tranid );
	if ( status )
	{
	    /* Unexpected error, bail. */
	    break;
	}

	/*
	** Step 2: Get "Send" in exclusive mode.
	**
	** Need SEND lock, to prevent message transmission while
	** we dismantle this connection.
	*/
	slk.cx_key.lk_type = LK_MSG;
	slk.cx_key.lk_key1 = CX_MSG_KEY(channel,CX_MSG_SEND_KEY);
	slk.cx_new_mode = LK_X;
	status = CXdlm_request( CX_F_STATUS | CX_F_PCONTEXT | CX_F_NO_DOMAIN,
				&slk, &tranid ); 
	/* Possible asynchronous completion.  Wait for final result. */
	status = CXdlm_wait( 0, &slk, 0 );

	if ( CX_ERR_STATUS(status) )
	{
	    /* Unexpected error, bail. */
	    break;
	}

	/*
	** Step 3: Release message channel locks.
	**
	** Press on, even if release is unsuccessful (highly unlikely)
	*/
	if ( CX_NONZERO_ID(&pmcb->cx_msg_hello_lkreq.cx_lock_id) )
	{
	    /*
	    ** Wait here until any message receipt in progress has
	    ** ended, or an excessive amount of time has passed.
	    */
	    countdown = CX_MSG_DISCONNECT_WAIT;
	    while ( FALSE == pmcb->cx_msg_ready && countdown-- > 0 )
	    {
		/* Wait a second. */
		(void)CSsuspend( CS_TIMEOUT_MASK, 1, 0 );
	    }

	    /* Now free the lock. */
	    rstatus = CXdlm_release( 0, &pmcb->cx_msg_hello_lkreq );
	    if ( OK == status ) status = rstatus;
	}

	if ( CX_NONZERO_ID(&pmcb->cx_msg_goodbye_lkreq.cx_lock_id) )
	{
	    rstatus = CXdlm_release( 0, &pmcb->cx_msg_goodbye_lkreq );
	    if ( OK == status ) status = rstatus;
	}

	/*
	** Step 4: Clear MSG CB & resume message channel, so it can
	**	   shut itself down.
	*/
	(void)MEfill( sizeof(CX_MSG_CB), '\0', (PTR)pmcb );
	if ( channelsid )
	    CSresume( channelsid );
    }

    /*
    ** Step 5: Release SEND lock if held.
    */
    if ( CX_NONZERO_ID(&slk.cx_lock_id) )
    {
	/* Release send lock. */
	rstatus = CXdlm_release( 0, &slk );
	if ( OK == status ) status = rstatus;
    }

    return status;
} /* CXmsg_disconnect */



/*{
** Name: CXmsg_send	- Broadcast a message.
**
** Description:
**
**      This routine broadcasts a message to all processes connected
**	to a "channel".   The message sent, is generated by a CMO
**	style callback function passed to this routine.   
**
**	This somewhat indirect way of passing the message allows the 
**	message to maintain state information between the processes, 
**	with each sender generating the new message value without any 
**	chance of simultaneous updates, and the risk of lost state 
**	updates.
**	
**	User provided function must not update the old value passed
**	to it.   If it needs to return additional information, or
**	additional information needs to be passed to the update function
**	to perform its work, then this additional data should be made
**	accessable through 'pdata'.
**
**	Routine used to provide message value MUST NOT request any
**	additional lock.  Doing so could lead to undetected deadlocks.
**	
** Inputs:
**
**      channel	- Index number of channel [ 0 .. CX_MAX_CHANNEL - 1 ]
**
**	pcmo		- Pointer to CMO to be filled with message sent.
**			  May be NULL if caller doesn't care.
**
**	pfunc		- address of user callback routine with the
**			  following parameters:
**
**		poldval		- Ptr to current CMO value of message.
**		
**		pnewval		- Ptr to buf to hold new message (CMO) value.
**		
**		pdata		- Pointer to auxillary data.
**		
**		invaliddata	- bool value set to true if old CMO value is
**				  invalid when send function is called.
**
**			  If a NULL 'pfunc' is passed, message sent is
**			  not dependent on lock value associated with this
**			  channel, and new message will simply overwrite
**			  old message.   In this case, 'pdata' MUST be a
**			  CMO containing new message.
**
**	pdata		- Pointer to auxillary data used by callback.  Must
**			  be a CMO containing new message if 'pfunc' NULL.
**
** Outputs:
**
**	none
**
**	Returns:
**		OK			- Normal successful completion.
**		E_CL2C11_CX_E_BADPARAM	- Bad channel #.
**		E_CL2C1B_CX_E_BADCONTEXT - No connection to channel.
**		Other codes may be propagated from failed DLM calls.
**		
**	Exceptions:
**	    none
**
** Side Effects:
**
**	On any DLM error channel is closed.
**
** History:
**	01-mar-2001 (devjo01)
**	    Created.
**	29-feb-2004 (devjo01)
**	    Correct instance in step 3a, where wrong lock pointer
**	    was passed to CXdlm_wait, which fortunately should
**	    not typically be called for a down conversion.  Renamed
**	    variables used to improve clarity.
**	12-apr-2005 (devjo01)
**	    Use new CX_MSG struct instead of overloading CX_CMO so as to
**	    allow synchronized short messages of 32 bytes.
*/
STATUS
CXmsg_send( u_i4 channel, CX_MSG *pmsg, CX_MSG_SEND_FUNC *pfunc, PTR pdata )
{
    STATUS		 status, rstatus;
    bool		 invalidblk;
    CX_MSG_CB		 mcb;
    CX_REQ_CB		*phellolk, *pgoodbyelk, sendlk;
    CX_MSG		*pnewmsg;
    CX_MSG		 tempmsg;

# ifdef CX_PARANOIA
    if ( channel > CX_MAX_CHANNEL ||
	 (NULL == pfunc && NULL == pdata) )
	return E_CL2C11_CX_E_BADPARAM;
    if ( CX_ST_UNINIT == CX_Proc_CB.cx_state ) 
	return E_CL2C1B_CX_E_BADCONTEXT;
# endif /*CX_PARANOIA*/

    phellolk = &mcb.cx_msg_hello_lkreq;
    pgoodbyelk = &mcb.cx_msg_goodbye_lkreq;

    (void)MEfill( sizeof(CX_REQ_CB), '\0', (PTR)&sendlk );

    for ( ; /* Something to break out of */ ; )
    {
	status = cx_mcb_format( channel, &mcb );
	if ( OK != status )
	    break;
	
	sendlk.cx_key.lk_type = LK_MSG;
	sendlk.cx_key.lk_key1 = CX_MSG_KEY(channel,CX_MSG_SEND_KEY);

	/*
	** Step 1: Get "Send" in exclusive mode.
	**
	** This serializes message delivery, and prevents
	** deadlocks with a process connecting to the channel.
	*/
	sendlk.cx_new_mode = LK_X;
	status = CXdlm_request( CX_F_STATUS | CX_F_PCONTEXT | CX_F_NO_DOMAIN,
				&sendlk, &mcb.cx_msg_transid ); 
	if ( OK == status )
	{
	    /* Asynchronous completion.  Wait for final result. */
	    status = CXdlm_wait( 0, &sendlk, 0 );
	}
	if ( CX_ERR_STATUS(status) )
	    break;

	/*
	** Step 2a: Get "Hello" in NULL mode.
	**
	** We get lock first in NL mode, then convert it to exclusive
	** mode, because otherwise when the receive logic downgrades
	** it's lock on "Hello" to NL, then converts it back to 
	** shared, it will be immediately granted, even though we
	** have a pending X request here.   This is because conversions
	** are granted immediately if requested mode is compatible
	** with all granted modes, and if a waiter has not yet been
	** moved to grant status, the incompatible conversion will
	** jump ahead.
	*/
	phellolk->cx_new_mode = LK_N;
	status = CXdlm_request( CX_F_STATUS | CX_F_PCONTEXT | CX_F_NODEADLOCK |
	                        CX_F_NO_DOMAIN, phellolk, &mcb.cx_msg_transid ); 
	if ( OK == status )
	{
	    /* Asynchronous completion.  NB, s/b granted synchronously. */
	    status = CXdlm_wait( 0, phellolk, 0 );
	}
	if ( CX_ERR_STATUS(status) )
	    break;


	/*
	** Step 2b: Get "Hello" in exclusive mode. 
	**
	** This will trigger blocking routine registered
	** with this channel.
	*/
	phellolk->cx_new_mode = LK_X;
	status = CXdlm_convert( CX_F_STATUS | CX_F_NODEADLOCK | CX_F_OWNSET, 
	  phellolk ); 
	if ( OK == status )
	{
	    /* Asynchronous completion.  Wait for final result. */
	    status = CXdlm_wait( 0, phellolk, 0 );
	}

	if ( CX_OK_STATUS( status ) )
	{
	    invalidblk = FALSE;
	}
	else if ( E_CL2C08_CX_W_SUCC_IVB == status )
	{
	    invalidblk = TRUE;
	}
	else
	{
	    /* Unexpected error */
	    break;
	}

	/*
	** Step 3a: LK_N request on goodbye lock.
	*/
	pgoodbyelk->cx_new_mode = LK_N;
	status = CXdlm_request( CX_F_STATUS | CX_F_PCONTEXT | CX_F_NO_DOMAIN |
				CX_F_NODEADLOCK, pgoodbyelk,
				&mcb.cx_msg_transid );
	if ( OK == status )
	{
	    /* Asynchronous completion.  Wait for final result. */
	    status = CXdlm_wait( 0, pgoodbyelk, 0 );
	}
	if ( CX_ERR_STATUS( status ) )
	{
	    /* Unexpected error */
	    break;
	}

	/*
	** Step 3b: Upgrade to LK_X.
	*/
	pgoodbyelk->cx_new_mode = LK_X;
	status = CXdlm_convert( CX_F_STATUS | CX_F_NODEADLOCK | CX_F_OWNSET,
	  pgoodbyelk ); 
	if ( CX_ERR_STATUS( status ) )
	{
	    /* Unexpected error */
	    break;
	}

	/*
	** Step 4: Prepare new message for send 
	*/
	if ( pfunc )
	{
	    if ( OK == (*pfunc)( (CX_MSG *)&mcb.cx_msg_hello_lkreq.cx_value,
				 &tempmsg, pdata, invalidblk ) )
	    {
		/* User generated valid new message.  Update value block. */
		pnewmsg = &tempmsg;
	    }
	    else
	    {
		/* Keep old value. */
		pnewmsg = (CX_MSG *)&mcb.cx_msg_hello_lkreq.cx_value;
	    }
	}
	else
	{
	    pnewmsg = (CX_MSG *)pdata;
	}

	(void)MEcopy( pnewmsg, CX_MSG_VALUE_SIZE, 
		      &mcb.cx_msg_hello_lkreq.cx_value );

	/*
	** Step 5: Release "Hello" lock to "send" the message.
	*/
	rstatus = CXdlm_release( 0, phellolk );
	if ( CX_ERR_STATUS(  rstatus ) )
	{
	    /* Unexpected error */
	    break;
	}

	/*
	** Step 6: Wait for completion of queued request on "Goodbye" lock.
	*/
	if ( OK == status )
	    status = CXdlm_wait( 0, pgoodbyelk, 0 );
	else if ( CX_OK_STATUS( status ) )
	    status = OK;
	break;
    }

    if ( CX_NONZERO_ID( &phellolk->cx_lock_id ) )
    {
	/* Release "hello" lock if not already done so. */
	rstatus = CXdlm_release( 0, phellolk );
	if ( OK == status ) status = rstatus;
    }

    if ( CX_NONZERO_ID( &pgoodbyelk->cx_lock_id ) )
    {
	/*
	** At this point, if all has gone well, all readers have 
	** received and acted on message.
	*/

	/*
	** Step 7: Release goodbye lock, completing cycle.
	*/
	rstatus = CXdlm_release( 0, pgoodbyelk );
	if ( OK == status ) status = rstatus;
    }

    if ( CX_NONZERO_ID( &sendlk.cx_lock_id ) )
    {
	/*
	** Step 8: Release "Send" lock.
	**
	** This enables next sender to procede.
	*/
	rstatus = CXdlm_release( 0, &sendlk );
	if ( OK == status ) status = rstatus;
    }

    if ( ( OK == status ) && pmsg )
    {
	(void)MEcopy( &tempmsg, CX_MSG_VALUE_SIZE, pmsg ); 
    }

    return status;
} /* CXmsg_send */


/************************************************************************
**		    Cluster Long Message Routines (CLM)			**
 ************************************************************************/

/*
**  Cluster Long Messages differ from the regular "short" messages in 
**  that they can be quite large, are not broadcast, and have no co-
**  ordination or guarantee of delivery.  The sender of a long message
**  gets a "chit" when he writes his message, and the receiver redeems
**  this chit to retrieve the message.  Sender passes chit by some other
**  means (E.g. Lock value block, Short message), and naturally does
**  not pass chit until the message is ready to send.
**  
**  There are alternate CLM implementations:
**  
**  CX_CLM_IMC:	Pass messages through Internal Memory Channel.
**  
**  This is the prefered way when available, since it is very fast.
**  However availability is limited to Tru64 boxes with appropriate
**  hardware installed.   Implementation uses memory buffers in each
**  node which through clever system software, and a very fast inter-
**  connect are kept in sync between machines.  Chit here is an index
**  into fixed sized cells in the buffer, and message is written to
**  one or more cells forming a circular buffer.
**  
**  CX_CLM_FILE:	Pass messages through shared files.
**  
**  This is a far slower method, but has the virtue of being portable
**  to any system supporting clusters, since if you have clustering
**  you have shared files.   Each node opens a file of the same size
**  in II_DATABASE/default.   Files are treated as a circular array
**  of cells, with each cell the optimum size for I/O (right now
**  fixed at 4K for EXT3).   Chit encodes both originating node, and
**  a sequence number which is used to hash the starting cell for a
**  message.  Current algoritm requires exclusive access to a file for
**  writing, but permits multiple readers.
*/

struct _CX_CLM_FCB {
    CS_SEMAPHORE	clmfcb_sem;	/* serializes access within proc. */
    DI_IO		clmfcb_dio;	/* shared file handle */
    char		clmfcb_filename[8]; /* Big enough for 2 digit node#s*/
    PTR			clmfcb_buf;	/* I/O buffer. */
    i4			clmfcb_bufpages;/* Buffer size in memory pages */
    i4			clmfcb_bufsize; /* Buffer size in memory pages */
    i4			clmfcb_bufstartchit;/* 1st chit in current write */
    i4			clmfcb_bufchit;/* Last chit processed. */
    i4			clmfcb_bufwoff; /* current write pos in buffer. */
    i4			clmfcb_flags;   /* misc status flags. */
#define CX_CLMF_F_NOREALLOC	(1<<0)	/* Stop trying to grow buffer */
};

typedef struct _CX_CLM_FCB CX_CLM_FCB;

static	CX_CLM_FCB	Clm_fcbs[CX_MAX_NODES] ZERO_FILL;
static	i4		Clm_fdir_len;		/* Length of path to files */
static	char		Clm_fdir[DI_PATH_MAX+1];/* Diretory path to files */


/*{
** Name: cx_msg_clm_lock - Serialize access to a CLM resource.
**
** Description:
**
**      Updating the CLM storage file is neither thread safe, or process
**	safe, so we need to be sure only one thread in the whole system
**	writes to the file, and that no one is reading at that point.
**
**	Reading the message file is somewhat less restrictive, and
**	a share lock will suffice.  However since all threads in a
**	process share one file handle, and one I/O buffer per node
**	they correspond with, we still need to serialize intra-process
**	access to a given node.
**
** Inputs:
**
**	node 		- node number of node to reserve.
**
**      preq		- pointer to CX DLM request block.
**
**	mode		- LK_X if writing, LK_S if reading, you
**			  may only write to your own node.
**
** Outputs:
**
**	none
**
**	Returns:
**		OK	- Normal successful completion.
**		Otherwise any of the myriad bad codes returned by
**		the routines used here.
**
**	Exceptions:
**	    none
**
** Side Effects:
**
**	Lock of the appropriate strength held.
**	Exclusive mutex reserving local resources held.
**
** History:
**	04-apr-2004 (devjo01)
**	    Created.
*/
static STATUS
cx_msg_clm_lock( i4 node, CX_REQ_CB *preq, i4 mode )
{
    STATUS	 status, tstatus;
    CX_CLM_FCB  *fcb;
    LK_LOCK_KEY  key;
    union {
	CS_SID		s;
	LK_UNIQUE	t;
    }		 tranid;
    
    for ( ; /* Something to break out of */ ; )
    {
	fcb = &Clm_fcbs[node - 1];

	/* First get lock, since we'd rather sleep than mutex spin */
	key.lk_type = LK_MSG;
	key.lk_key1 = CX_CLM_KEY(node);
	CX_INIT_REQ_CB(preq, mode, &key);
	CSget_cpid(&preq->cx_owner);

	/*
	** Use session ID as a "fake" transaction ID.  This is only
	** for the DLM's benefit, to make sure multiple requests for
	** this lock within the server are not seen as the same.
	** There is no real problem if this faux XID conflicts
	** with a real XID, as we don't need to check for deadlock
	** (There are no further lock requests between getting this
	** lock and releasing it, so no chance of deadlock), and the
	** CLM locks have no impact on recovery.
	*/
	tranid.t.lk_uhigh = 0; 
	tranid.s = preq->cx_owner.sid;

	status = CXdlm_request( CX_F_OWNSET|CX_F_PCONTEXT|CX_F_STATUS| 
	 CX_F_NODEADLOCK|CX_F_IGNOREVALUE|CX_F_NO_DOMAIN, preq, &tranid.t );
	if ( OK == status )
	{
	    status = CXdlm_wait(0, preq, 0 );
	}
	if ( E_CL2C01_CX_I_OKSYNC != status && OK != status )
	{
	    BREAK_ON_BAD_STATUS(status);
	}

	/* Get exclusive access to process level resources */
	status = CSp_semaphore(1, &fcb->clmfcb_sem);
	if ( OK == status ) break;
	
	REPORT_BAD_STATUS(status);

	tstatus = CXdlm_release(0, preq);
	BREAK_ON_BAD_STATUS(tstatus);
	break;
    }
    return status;
}


/*{
** Name: cx_msg_clm_unlock - Release CLM resource.
**
** Description:
**
**      Releases the mutexes & locks obtained by cx_msg_clm_lock.
**
** Inputs:
**
**	node 		- node number of node to talk with.
**
**      preq		- pointer to CX DLM request block.
**
**
** Outputs:
**
**	none
**
**	Returns:
**		OK	- Normal successful completion.
**		Otherwise any of the myriad bad codes returned by
**		the routines used here.
**
**	Exceptions:
**	    none
**
** Side Effects:
**
**	Releases the mutexes & locks obtained by cx_msg_clm_lock.
**
** History:
**	04-apr-2004 (devjo01)
**	    Created.
*/
static STATUS
cx_msg_clm_unlock( i4 node, CX_REQ_CB *preq )
{
    STATUS	status, tstatus;

    tstatus = CSv_semaphore(&Clm_fcbs[node-1].clmfcb_sem);
    REPORT_BAD_STATUS(tstatus);

    status = CXdlm_release(0, preq);
    REPORT_BAD_STATUS(status);
    if ( OK == status ) status = tstatus;
    return status;
}


/*{
** Name: cx_msg_clm_init - Initialize the CLM file implementation.
**
** Description:
**
**	This routine should only be called by CXinitialize.  Routine
**	Initializes all internal data for the Long Message passing
**	through file implementation, and creates/recreates the message
**	file for this node.
**	
** Inputs:
**
**	none
**
** Outputs:
**
**	none
**
**	Returns:
**		OK	- Normal successful completion.
**		Otherwise any of the myriad bad codes returned by
**		the routines used here.
**
**	Exceptions:
**	    none
**
** Side Effects:
**
**	Message file of configured size created in II_DATA/default.
**
** History:
**	04-apr-2004 (devjo01)
**	    Created.
*/
STATUS
cx_msg_clm_file_init( void )
{
    CX_CLM_FCB	*fcb;
    CX_NODE_CB  *sms;
    LOCATION	 loc;
    CX_REQ_CB	 reqcb;
    STATUS	 status, tstatus;
    CL_ERR_DESC  syserr; 
    i4		 node;
    char	*dirspec;
    i4		 haveclmlock = 0;
    i4		 fnamelen;
    i4		 i;
    SIZE_TYPE	 alloc;

# ifdef CX_PARANOIA
    if ( CX_Proc_CB.cx_clm_blksize == 0 ||
         (ME_MPAGESIZE % CX_Proc_CB.cx_clm_blksize) != 0 )
	return E_CL2C11_CX_E_BADPARAM;
# endif

    for ( ; /* Something to break out of */ ; )
    {
	/* Get directory path to the primary data location */
	status = LOingpath( ING_DBDIR, NULL, LO_DB, &loc);
	BREAK_ON_BAD_STATUS(status);

	LOtos(&loc, &dirspec);
	Clm_fdir_len = STlcopy( dirspec, Clm_fdir, DI_PATH_MAX );
	if ( '\0' != *(dirspec+Clm_fdir_len) )
	{
	    /* Path is too long. */
	    status = E_CL2C19_CX_E_BADCONFIG;
	}

	/* Format control structures */
	fcb = Clm_fcbs;
	for ( node = 1; node <= CX_MAX_NODES; node++, fcb++ )
	{
	    STprintf(fcb->clmfcb_filename,"iiclm%d", node);
	    status =
	      CSw_semaphore( &fcb->clmfcb_sem, CS_SEM_SINGLE,
	                       fcb->clmfcb_filename );
	    BREAK_ON_BAD_STATUS(status);
	}
	if ( OK != status ) break;
	
	fcb = &Clm_fcbs[CX_Proc_CB.cx_node_num - 1];

	/* We require ME_MPAGESIZE t/b a multiple of CLM I/O blksize. */
	status = MEget_pages( ME_IO_MASK, 1, "", &fcb->clmfcb_buf,
	                      &alloc, &syserr );
	BREAK_ON_BAD_STATUS(status);
	fcb->clmfcb_bufpages = 1;
	fcb->clmfcb_bufsize = ME_MPAGESIZE;

	if ( CX_Proc_CB.cx_flags & CX_PF_IS_CSP )
	{
	    /*
	    ** The CSP for this node initializes the message file.
	    **
	    ** File is deleted and recreated to assure that no one
	    ** inadvertently picks up a stale chit.
	    */

	    status = cx_msg_clm_lock( CX_Proc_CB.cx_node_num, &reqcb, LK_X );
	    if ( status ) break;
	    haveclmlock++;

	    /* Initialize shared data items with node scope. */
	    sms = CX_Proc_CB.cx_ncb;

	    status = CSw_semaphore( &sms->cx_clm_sem, CS_SEM_MULTI,
	                       "cx_clm_sem" );
	    BREAK_ON_BAD_STATUS(status);

	    /*
	    ** Randomize initial chit as further protection against
	    ** a stale chit being cashed in.
	    */
	    MHsrand(CX_Proc_CB.cx_pid);
	    sms->cx_clm_chitseq = (i4)MHrand();

	    fnamelen = STlength(fcb->clmfcb_filename);

	    /* Delete old file if it exists */
	    status = DIdelete( &fcb->clmfcb_dio, Clm_fdir, Clm_fdir_len,
	              fcb->clmfcb_filename, fnamelen, &syserr );
	    if ( OK != status && DI_FILENOTFOUND != status )
	    {
		REPORT_BAD_STATUS(status);
	    }

	    /* Create file (will have zero length until DIalloc) */
	    status = DIcreate( &fcb->clmfcb_dio, Clm_fdir, Clm_fdir_len,
	              fcb->clmfcb_filename, fnamelen,
		      CX_Proc_CB.cx_clm_blksize, &syserr );
	    BREAK_ON_BAD_STATUS(status);
	}

	/* Always open the file. */
	status = DIopen( &fcb->clmfcb_dio, Clm_fdir, Clm_fdir_len,
		  fcb->clmfcb_filename, STlength(fcb->clmfcb_filename),
		  CX_Proc_CB.cx_clm_blksize, DI_IO_WRITE, DI_SYNC_MASK,
		  &syserr );
	BREAK_ON_BAD_STATUS(status);

	if ( CX_Proc_CB.cx_flags & CX_PF_IS_CSP )
	{
	    /* allocate the blocks physically as other processes may
	       need to write messages
	    */ 
	    status = DIgalloc( &fcb->clmfcb_dio, CX_Proc_CB.cx_clm_blocks,
	                      &i, &syserr );
	    REPORT_BAD_STATUS(status);
	}
	break;
    }

    if ( haveclmlock )
    {
	tstatus = cx_msg_clm_unlock( CX_Proc_CB.cx_node_num, &reqcb );
	if (OK == status) status = tstatus;
	REPORT_BAD_STATUS(tstatus);
    }

    return status;
}


/*{
** Name: cx_msg_clm_reserve - Get a new chit & reserve space.
**
** Description:
**
**	Generate new chit based on a sequence number in shared memory,
**	then bump the sequence number by the number of blocks we'll need
**	to write.
**	
** Inputs:
**
**      pchit		- pointer to chit to fill with new value.
**
**	length		- how many bytes of users data to reserve for.
**
** Outputs:
**
**	*pchit		- holds new chit value.
**
**	Returns:
**		OK	- Normal successful completion.
**		Otherwise any of the myriad bad codes returned by
**		the routines used here.
**
**	Exceptions:
**	    none
**
** Side Effects:
**
**	Shared chit value bumped.
**
** History:
**	04-apr-2004 (devjo01)
**	    Created.
*/
static STATUS
cx_clm_file_reserve( i4 *pchit, i4 length )
{
    CX_NODE_CB  *sms;
    STATUS	 status;

    sms = CX_Proc_CB.cx_ncb;

    for ( ; /* Something to break out of */ ; )
    {
	status = CSp_semaphore( 1, &sms->cx_clm_sem );
	BREAK_ON_BAD_STATUS(status);
	
	*pchit = 
	 CX_CLMF_CHIT( CX_Proc_CB.cx_node_num, sms->cx_clm_chitseq );
	sms->cx_clm_chitseq += CX_CLMF_PAGES(CX_CLMF_ADJ_OFFSET(length));

	status = CSv_semaphore( &sms->cx_clm_sem );
	REPORT_BAD_STATUS(status);
	break;
    }
    return status;
}


/*{
** Name: cx_clm_file_force - Write full blocks to file.
**
** Description:
**
**      Routine writes out an integer number of blocks from buffer.
**	Normally this is done in one write, but if near the end
**	of the circular message buffer file, it will wrap.
**
**	Placement and size of write taken from fcbclm_bufchit,
**	and fcbclm_bufwoff respectively.
**
** Inputs:
**
**      fcb		- Pointer to control block this file.
**
** Outputs:
**
**	syserr		- May have OS error info if failure.
**
**	Returns:
**		OK	- Normal successful completion.
**		Otherwise any of the myriad bad codes returned by
**		the routines used here.
**
**	Exceptions:
**	    none
**
** Side Effects:
**
**	Stuff gets written to disk.
**
** History:
**	04-apr-2004 (devjo01)
**	    Created.
*/
static STATUS
cx_clm_file_force( CX_CLM_FCB  *fcb, CL_ERR_DESC *syserr )
{
    STATUS	 status;
    i4		 page, pagesleft, numpages, offset;

    pagesleft = CX_CLMF_PAGES(fcb->clmfcb_bufwoff);
    page = CX_CLMF_PAGE_FROM_CHIT(fcb->clmfcb_bufchit);
    offset = 0;
    
    while ( pagesleft > 0 )
    {
	numpages = ((page + pagesleft) > CX_Proc_CB.cx_clm_blocks) ? 
	  CX_Proc_CB.cx_clm_blocks - page : pagesleft;
	status = DIwrite( &fcb->clmfcb_dio, &numpages, 
	    page, fcb->clmfcb_buf + offset, syserr );
	BREAK_ON_BAD_STATUS(status);

	pagesleft -= numpages;
	offset += numpages * CX_Proc_CB.cx_clm_blksize;
	page = 0; /* we either wrote it all or wrapped. */
    }
    return status;
}

/*{
** Name: cx_msg_clm_fault - Read full blocks from file.
**
** Description:
**
**      Routine reads an integer number of blocks into buffer.
**	Normally this is dome in one read, but if near the end
**	of the circular message buffer file, read will wrap.
**
** Inputs:
**
**      fcb		- Pointer to control block this file.
**
** Outputs:
**
**	syserr		- May have OS error info if failure.
**
**	Returns:
**		OK	- Normal successful completion.
**		Otherwise any of the myriad bad codes returned by
**		the routines used here.
**
**	Exceptions:
**	    none
**
** Side Effects:
**
**	Stuff gets read from disk.
**
** History:
**	04-apr-2004 (devjo01)
**	    Created.
*/
static STATUS
cx_clm_file_fault( CX_CLM_FCB *fcb, i4 chit, i4 pages, CL_ERR_DESC *syserr )
{
    STATUS	 status;
    i4		 page, pagesleft, numpages, offset;

    pagesleft = pages;
    page = CX_CLMF_PAGE_FROM_CHIT(chit);
    offset = 0;
    
    while ( pagesleft > 0 )
    {
	numpages = ((page + pagesleft) > CX_Proc_CB.cx_clm_blocks) ? 
	  CX_Proc_CB.cx_clm_blocks - page : pagesleft;
	status = DIread( &fcb->clmfcb_dio, &numpages, 
	    page, fcb->clmfcb_buf, syserr );
	BREAK_ON_BAD_STATUS(status);

	pagesleft -= numpages;
	page = 0; /* we either read it all or wrapped. */
	offset += numpages * CX_Proc_CB.cx_clm_blksize;
    }
    return status;
}


/*{
** Name: cx_msg_clm_write - Write to a reserved chit.
**
** Description:
**
**	Only the originating node should write to a chit, and then only
**	to a chit it got from cx_msg_clm_reserve.
**	
**	Routine allows for interleaved writes between users (inefficient),
**	and will grow the transfer buffer if needed.  Write is fully
**	forced to disk when offset + length = totlen.
**
** Inputs:
**
**      chit		- chit to write.
**
**	data		- pointer to data to write.
**
**	offset		- logical offset into message to write to.
**
**	length		- number of bytes this write.
**
**	totlen		- total message size.
**
** Outputs:
**
**	none
**
**	Returns:
**		OK	- Normal successful completion.
**		Otherwise any of the myriad bad codes returned by
**		the routines used here.
**
**	Exceptions:
**	    none
**
** Side Effects:
**
**	Disk writes, changes to fcb for local node.
**
** History:
**	04-apr-2004 (devjo01)
**	    Created.
**	28-apr-2004 (devjo01)
**	    Replace bad ref to 'amount' where 'length' was intended.
**          (Thanks to Teresa King for spotting this.)
*/
static STATUS
cx_clm_file_write( i4 chit, PTR data, i4 offset, i4 length, i4 totlen )
{
    STATUS	 status, tstatus;
    CX_REQ_CB	 reqcb;
    CX_CLM_FCB	*fcb;
    CL_ERR_DESC  syserr;
    i4		 holdlock = 0;
    SIZE_TYPE	 numpages;
    i4		 bufchit;
    i4		 doffset, amount, left;
    PTR		 pbuf;

    for ( ; /* Something to break out of */ ; )
    {
	status = cx_msg_clm_lock( CX_Proc_CB.cx_node_num, &reqcb, LK_X );
	BREAK_ON_BAD_STATUS(status);
	holdlock++;

	fcb = &Clm_fcbs[CX_Proc_CB.cx_node_num - 1];

	/* Calculate chit for our buffer */
	bufchit = CX_CLMF_CHIT_PLUS(chit, offset);

	/* If someone else is halfway through a write, force it out. */
	if ( fcb->clmfcb_bufchit != 0 &&
	     fcb->clmfcb_bufstartchit != chit )
	{
	    status = cx_clm_file_force( fcb, &syserr );
	    BREAK_ON_BAD_STATUS(status);

	    fcb->clmfcb_bufchit = 0;

	    if ( 0 != offset &&
	         0 != (CX_CLMF_ADJ_OFFSET(offset) % CX_Proc_CB.cx_clm_blksize) )
	    {
		/* Fault in our partially output long msg. */
		status = cx_clm_file_fault( fcb, bufchit, 1, &syserr );
		BREAK_ON_BAD_STATUS(status);
	    }
	}

	fcb->clmfcb_bufstartchit = chit;
	fcb->clmfcb_bufchit = bufchit;

	doffset = CX_CLMF_ADJ_OFFSET(offset) % CX_Proc_CB.cx_clm_blksize;
	left = length;

	/* Create larger buffer if needed, and not failed before */
	if ( doffset + length > fcb->clmfcb_bufsize &&
	     !(fcb->clmfcb_flags & CX_CLMF_F_NOREALLOC) )
	{
	    /* Reallocate a larger buffer */
	    tstatus = MEget_pages( ME_IO_MASK, fcb->clmfcb_bufpages +
	               fcb->clmfcb_bufpages, "", &pbuf,
		       &numpages, &syserr );
	    if ( OK == tstatus )
	    {
		if ( fcb->clmfcb_bufwoff )
		{
		    MEcopy( fcb->clmfcb_buf, fcb->clmfcb_bufwoff, pbuf );
		}
		tstatus = MEfree_pages( fcb->clmfcb_buf,
		     fcb->clmfcb_bufpages, &syserr );
		REPORT_BAD_STATUS(tstatus);  /* Note it but not fatal */

		fcb->clmfcb_bufpages = numpages;
		fcb->clmfcb_bufsize = numpages * ME_MPAGESIZE;
		fcb->clmfcb_buf = pbuf;
	    }
	    else
	    {
		fcb->clmfcb_flags |= CX_CLMF_F_NOREALLOC;
	    }
	}

	pbuf = fcb->clmfcb_buf;

	if ( 0 == offset )
	{
	    /* Initialize message header */
	    ((CX_MSG_HEAD *)pbuf)->cx_lmh_length = length; /* Raw length */
	    ((CX_MSG_HEAD *)pbuf)->cx_lmh_chit = chit;
	}

	while ( OK == status && ( left > 0 ) )
	{
	    amount = left;
	    if ( doffset + amount > fcb->clmfcb_bufsize )
		amount = fcb->clmfcb_bufsize - doffset;

	    MEcopy( data, amount, pbuf + doffset );
	    left -= amount;
	    offset += amount;
	    fcb->clmfcb_bufwoff = doffset + amount;

	    if ( 0 == left )
		break;

	    /* Here only if buffer too small. */
	    status = cx_clm_file_force( fcb, &syserr );
	    BREAK_ON_BAD_STATUS(status);
	    
	    fcb->clmfcb_bufwoff = doffset = 0;
	    data += amount;
	    fcb->clmfcb_bufchit = CX_CLMF_CHIT_PLUS(chit, offset);
	}

	if ( offset == totlen )
	{
	    /* Finished writing message, force it out */
	    status = cx_clm_file_force( fcb, &syserr );
	    REPORT_BAD_STATUS(status);

	    /* We're done, clean up so next caller won't force buffer */
	    fcb->clmfcb_bufchit = fcb->clmfcb_bufstartchit =
	     fcb->clmfcb_bufwoff = 0;
	}

	break;
    }
    if ( holdlock )
    {
	tstatus = cx_msg_clm_unlock( CX_Proc_CB.cx_node_num, &reqcb );
	REPORT_BAD_STATUS(tstatus);
    }
    return status;
}


/*{
** Name: cx_msg_clm_read - Read from a chit.
**
** Description:
**
**	Routine allows for some buffering so multiple threads may get
**	the same message without refreshing it.  However, Ingres should
**	not need this capacity.
**
**	You may not read from your own node file!
**
**	Routine will grow the transfer buffer if needed.
**
** Inputs:
**
**      chit		- chit to write.
**
**	buffer		- place to put retreived data.
**
**	bufsize		- size of destination buffer.
**
**	offset		- logical offset into message to write to.
**
**	retrieved	- int buffer to hold retreived length.
**			  if retreived is negative, bufsize
**			  bytes were retrieved, and total size is
**			  negation of retrieved.  If we're reading
**			  the middle/end of a message, you may not
**			  have access to the internal header, and
**			  we cannot return length, or check chit.
**
** Outputs:
**
**	none
**
**	Returns:
**		OK	- Normal successful completion.
**		Otherwise any of the myriad bad codes returned by
**		the routines used here.
**
**	Exceptions:
**	    none
**
** Side Effects:
**
**	Disk reads, changes to fcb for local node.
**
** NOTE:
**
**	If you don't read from offset zero, it is possible for you
**	to read data that is no longer part of the chit.  This could
**	only happen if your chit was just about to get overwritten,
**	you got the 1st part, then the writter clobbered the blocks
**	just read and the blocks you're about to read.  This should
**	be very unlikely except in the case of an underconfigured
**	circular buffer file.  In practice Ingres typically redeems
**	entire long message at once.
**
** History:
**	04-apr-2004 (devjo01)
**	    Created.
*/
static STATUS
cx_clm_file_read( i4 chit, PTR buffer, i4 bufsize, i4 offset, i4 *retreived )
{
    STATUS	 status, tstatus;
    CX_REQ_CB	 reqcb;
    CX_CLM_FCB	*fcb;
    CL_ERR_DESC  syserr;
    i4		 holdlock = 0;
    SIZE_TYPE	 numpages;
    i4		 node;
    i4		 bufchit;
    i4		 doffset, amount, left;
    PTR		 pbuf;

    node = CX_CLMF_NODE_FROM_CHIT(chit);

# ifdef CX_PARANOIA
    if (node < 1 || node > CX_MAX_NODES || node == CX_Proc_CB.cx_node_num)
	return E_CL2C11_CX_E_BADPARAM;
# endif

    for ( ; /* Something to break out of */ ; )
    {
	status = cx_msg_clm_lock( node, &reqcb, LK_S );
	BREAK_ON_BAD_STATUS(status);
	holdlock++;

	fcb = &Clm_fcbs[node - 1];

	/* Calculate chit for our buffer */
	bufchit = chit;
	if ( 0 != offset )
	    bufchit = CX_CLMF_CHIT_PLUS(chit, offset);

	if ( fcb->clmfcb_bufchit != bufchit )
	{
	    /* Don't care about buffer contents. */
	    fcb->clmfcb_bufwoff = 0;
	}

	doffset = CX_CLMF_ADJ_OFFSET(offset) % CX_Proc_CB.cx_clm_blksize;

	/* Create larger buffer if needed, and not failed before */
	if ( (doffset + bufsize) > fcb->clmfcb_bufsize &&
	     !(fcb->clmfcb_flags & CX_CLMF_F_NOREALLOC) )
	{
	    numpages = fcb->clmfcb_bufpages + fcb->clmfcb_bufpages;
	    if ( 0 == fcb->clmfcb_bufpages )
	    {
		/* 1st reference to this node, must open file */
		status = DIopen( &fcb->clmfcb_dio, Clm_fdir,
			  Clm_fdir_len, fcb->clmfcb_filename, 
			  STlength(fcb->clmfcb_filename),
			  CX_Proc_CB.cx_clm_blksize, DI_IO_READ, 0,
			  &syserr );
		if ( DI_FILENOTFOUND == status )
		{
		    status = E_CL2C2C_CX_E_MSG_NOSUCH;
		}
		BREAK_ON_BAD_STATUS(status);
		numpages = 1;
	    }

	    /* Alloc initial buffer / Reallocate a larger buffer */
	    tstatus = MEget_pages( ME_IO_MASK, numpages, "", &pbuf,
		       &numpages, &syserr );
	    if ( OK == tstatus )
	    {
		if ( fcb->clmfcb_bufwoff )
		{
		    MEcopy( fcb->clmfcb_buf, fcb->clmfcb_bufwoff, pbuf );
		}
		if ( fcb->clmfcb_buf )
		{
		    tstatus = MEfree_pages( fcb->clmfcb_buf,
		     fcb->clmfcb_bufpages, &syserr );
		    REPORT_BAD_STATUS(tstatus);  /* Note it but not fatal */
		}
		fcb->clmfcb_bufpages = numpages;
		fcb->clmfcb_bufsize = numpages * ME_MPAGESIZE;
		fcb->clmfcb_buf = pbuf;
	    }
	    else
	    {
		if ( NULL == fcb->clmfcb_buf )
		{
		    status = tstatus;
		    REPORT_BAD_STATUS(status);
		    break;
		}
		fcb->clmfcb_flags |= CX_CLMF_F_NOREALLOC;
	    }
	}

	pbuf = fcb->clmfcb_buf;
	left = bufsize;

	*retreived = -1;	/* Set don't know indicator */

	while ( OK == status && ( left > 0 ) )
	{
	    amount = left;
	    if ( doffset + amount > fcb->clmfcb_bufsize )
		amount = fcb->clmfcb_bufsize - doffset;

	    if ( fcb->clmfcb_bufchit != bufchit ||
	         amount > fcb->clmfcb_bufwoff )
	    {
		fcb->clmfcb_bufwoff = 0; /* In case we fail */
		status = cx_clm_file_fault( fcb, bufchit, 
		              CX_CLMF_PAGES(doffset + amount), &syserr );
		BREAK_ON_BAD_STATUS(status);
	    }

	    fcb->clmfcb_bufchit = CX_CLMF_CHIT_PLUS(chit, offset);

	    if ( 0 == offset )
	    {
		/* Examine internal message header */
		if ( chit != ((CX_MSG_HEAD *)pbuf)->cx_lmh_chit )
		{
		    fcb->clmfcb_bufchit = 0;
		    status = E_CL2C2C_CX_E_MSG_NOSUCH;
		    break;
		}

		*retreived = ((CX_MSG_HEAD *)pbuf)->cx_lmh_length;
		if ( *retreived > bufsize )
		{
		    *retreived = -(*retreived);
		}
		else if ( bufsize > *retreived )
		{
		    amount = left = *retreived;
		}
	    }
	    /* copy actual message*/ 
	    MEcopy( pbuf+doffset, amount, buffer );
	    
	    left -= amount;
	    offset += amount;
	    fcb->clmfcb_bufwoff = fcb->clmfcb_bufsize *
	     CX_CLMF_PAGES(doffset + amount);
	    doffset = 0;
	}

	break;
    }
    if ( holdlock )
    {
	tstatus = cx_msg_clm_unlock( node, &reqcb );
	REPORT_BAD_STATUS(tstatus);
    }
    return status;
}



/*{
** Name: CXmsg_stow	- Store a "long" message for later retreival.
**
** Description:
**
**      This routine stores a message too large to send in the standard
**	rather tiny "message" structure, and returns a chit for
**	later redemption by any and all nodes in the cluster.
**
** Inputs:
**
**      pchit		- pointer to an i4 which will hold the CX message
**	    		  chit. (Not to be confused with a BLOB coupon)
**
**	pdata		- Pointer to a memory buffer with arbitrary contents.
**			  If pointer is NULL, resources are reserved for
**			  a message of "length" bytes to be stowed,
**			  but data is not copied.  Later data should
**			  be copied in with CX_msg_append.  This allows
**			  messages to be constructed from multiple
**			  source buffers.
**
**	length		- size of data in bytes.
**
** Outputs:
**
**	none
**
**	Returns:
**		OK			- Normal successful completion.
**		E_CL2C11_CX_E_BADPARAM	- Bad length.
**		E_CL2C12_CX_E_INSMEM	- Insufficient resources for
**					  request.
**		E_CL2C1B_CX_E_BADCONTEXT - CX uninitialized.
**	Exceptions:
**	    none
**
** Side Effects:
**
**	None.
**
** History:
**	29-may-2002 (devjo01)
**	    Created.
**	12-sep-2002 (devjo01)
**	    Allow for NULL pdata.
**	28-mar-2004 (devjo01)
**	    Correctly return E_CL2C10_CX_E_NOSUPPORT as needed.
**	04-apr-2004 (devjo01)
**	    Add CX_CLM_FILE support.
*/
STATUS
CXmsg_stow( i4 *pchit, PTR pdata, i4 length )
{
    STATUS	 status;

# ifdef CX_PARANOIA
    if ( length <= 0 || length > CX_MAX_MSG_STOW )
	return E_CL2C11_CX_E_BADPARAM;
    if ( CX_ST_UNINIT == CX_Proc_CB.cx_state ) 
	return E_CL2C1B_CX_E_BADCONTEXT;
# endif /*CX_PARANOIA*/

    switch ( CX_Proc_CB.cx_clm_type )
    {
    case CX_CLM_FILE:
	status = cx_clm_file_reserve(pchit, length);
	if ( OK == status && pdata )
	    status = cx_clm_file_write(*pchit, pdata, 0, length, length);
	break;

    case CX_CLM_IMC:
    /*
    **	Start OS dependent declarations. (outdent 4)
    */
#if defined(axp_osf) && defined(conf_CLUSTER_BUILD)
    /*
    **	Tru64 implementation
    */
    {
	union {
	    CX_CMO		cmo;
	    CX_MSG_HEAD		header;
	}		 workbuf; /* Work buffer */
	i4		 ndata; /* Holds "length" going in,
				   "chitidx" on return */
	CX_MSG_FRAG	*ptarg;

	/* Do a quick scram if IMC missing or uninitialized. */
	if ( NULL == CX_Proc_CB.cxaxp_imc_txa )
	    return E_CL2C10_CX_E_NOSUPPORT;

	if ( length > CX_MSG_MAX_CHIT * CX_MSG_FRAGSIZE / 25 )
	{
	    /* Don't let any one alloc hog too much memory */
	    /* <FIX_ME> see notes on how to increase size of msg region */
	    return E_CL2C12_CX_E_INSMEM;
	}

	ndata = length;

	status = CXcmo_update( CX_CMO_CXMSG_IDX, &workbuf.cmo,
			 cxmsg_axp_grab_msg_bits, (PTR)&ndata );
	if ( OK == status )
	{
	    if ( 0 == ndata )
	    {
		status = E_CL2C12_CX_E_INSMEM;
	    }
	    else
	    {
		/*
		** Copy length, and chit id in.
		** Warning! this is WRITE-ONLY memory!
		** Even the implicit read needed to
		** update a portion of the native
		** 8 byte write unit is prohibited.
		**
		** We construct our header in regular
		** memory, then copy it as a unit.
		**
		** MEcopy macro on Tru64 does not respect
		** the stringent access requirements,
		** so we use bcopy instead (which does).
		*/
		
		/* Step 1, calc target address */
		ptarg = (CX_MSG_FRAG *)((PTR)CX_Proc_CB.cxaxp_imc_txa +
		 (ndata - 1) * CX_MSG_FRAGSIZE);

		/* Step 2, put together 8 byte header */
		workbuf.header.cx_lmh_length = length;
		workbuf.header.cx_lmh_chit = (ndata & CX_MSG_CHIT_MASK) |
		 ((~CX_MSG_CHIT_MASK) & (i4)MHrand());

		/* Step 3, copy header to GSM. */
		bcopy( (PTR)&workbuf, ptarg, sizeof(workbuf.header) );

		if ( NULL != pdata )
		{
		    /* Step 4, copy data if 'pdata' not NULL. */
		    status = CXmsg_append( workbuf.header.cx_lmh_chit, 
					   pdata, 0, length );
		}

		if ( OK == status )
		{
		    /* Step 5, return chit value if successful. */
		    *pchit = workbuf.header.cx_lmh_chit;
		}
	    }
	}
    }
#else /* all others */
    status = E_CL2C10_CX_E_NOSUPPORT;
#endif /* axp_osf */

	/* Restore proper indenting */
	break;

    default:
	status = E_CL2C10_CX_E_NOSUPPORT;
    }
    return status;
} /* CXmsg_stow */

#if defined(axp_osf) && defined(conf_CLUSTER_BUILD)

/*
**  CMO update function used to locate and reserve a
**  range of IMC memory for passing a long message.
**
**  IMC memory for long memory passing is organized 
**  as an array of 256, 512 byte chunks.  One or
**  more consecutive chunks are allocatted to a message
**  using a bit array kept in a CMO object.
**    
**  For the algorithm used here to work, message 
**  must be <= (31 * 512) - sizeof(CX_MSG_HEAD).   That
**  is to say no message can reserve more than 31 bits
**  including space for the "long" msg header struct.
**
**  <FIX_ME>  We should take advantage of 64 bit
**  integers to do a quicker scan.
**
**  <FIX_ME>  Keeping bit array in a CMO object, allows us
**  to reuse code, but we need to be wary that this
**  limits us to only 128K of message memory, and a max
**  of only 256 distinct outstanding messages, even of
**  minimum size.  Fix for this would be to generalize
**  CMO update code to work on objects of larger size.
*/
static STATUS
cxmsg_axp_grab_msg_bits( CX_CMO *oldval, CX_CMO *pnewval,
                        PTR pdata, bool invalidin )
{
    u_i4	         *pbits;
    u_i4		 mask;
    i4			 frags, fromlast, i, j, chitidx;

    bcopy( (PTR)oldval, (PTR)pnewval, sizeof(CX_CMO) );

    /* Assume space not found */
    chitidx = 0;

    /*
    ** Get message length, passed in through *pdata,
    ** and use it to calculate the number of consecutive
    ** message block fragments (and hence bits) will be
    ** needed.
    */
    i = *(i4 *)pdata;
    i += sizeof(CX_MSG_HEAD);	/* Allow for message header */
    frags = ( i + CX_MSG_FRAGSIZE - 1 ) / CX_MSG_FRAGSIZE; 

    /*
    ** Loop over each 4 byte "word", and treat it as an array of
    ** bits.  We need to find a string of "frags" consecutive
    ** clear bits, keeping in mind the possibility that this
    ** bit string may span two of the words being checked.
    */
    for ( i = 0, fromlast = 0, pbits = (u_i4 *)pnewval;
	  i < sizeof(CX_CMO) / sizeof(u_i4);
	  i++, pbits++ )
    {
	/* If word entirely full, skip it */
	if ( ((u_i4)~0) == *pbits )
	{
	    fromlast = 0;
	    continue;
	}

	/* If some frags fit in previous word, check for spanning string */
	if ( 0 != fromlast )
	{
	    if ( 0 == ( *pbits & ((1 << (frags - fromlast)) - 1) ) )
	    {
		chitidx = 32 * i + 1 - fromlast;
		break;
	    }
	    fromlast = 0;
	}

	/* Check for a string in this word */
	mask = ( (1 << frags) - 1 );
	for ( j = 0; j <= 32 - frags; j++ )
	{
	    if ( 0 == (*pbits & mask) )
	    {
		chitidx = 32 * i + 1 + j;
		break;
	    }
	    mask <<= 1;
	}

	if ( chitidx )
	    break;

	/* See how many bits are free at the end, to allow spanning. */
	for ( mask = 0x80000000, fromlast = 0;
	      fromlast < frags;
	      mask >>= 1, fromlast++ )
	{
	    if ( mask & *pbits )
	    {
		break;
	    }
	}
    }

    if ( chitidx )
    {
	/* Set bits */
	if ( fromlast )
	{
	    /* Spanned */
	    *(pbits - 1) |= ((1 << fromlast) - 1) << ( 32 - fromlast );
	    *pbits |= ((1 << (frags - fromlast)) - 1);
	}
	else
	{
	    *pbits |= ((1 << frags) - 1) << j;
	}
    }

    /* Return chit index (or zero if no space) through pdata */
    *(i4 *)pdata = chitidx;

    /* Always return OK */
    return OK;
} /* cxmsg_axp_grab_msg_bits */

#endif /* axp_osf */




/*{
** Name: CXmsg_append	- Store a "long" message for later retreival.
**
** Description:
**
**      This routine stores data for a message too large to send 
**	in the standard	rather tiny "message" structure into an
**	abstract resource previously reserved by CXmsg_stow being
**	called with a NULL 'pdata' parameter.
**
** Inputs:
**
**      chit		- CX message chit previously returned by
**			  CXmsg_stow.
**
**	pdata		- Pointer to a memory buffer with arbitrary contents.
**			  If pointer is NULL, resources are reserved for
**			  a message of "length" bytes to be stowed,
**			  but data is not copied.  Later data should
**			  be copied in with CX_msg_append.  This allows
**			  messages to be constructed from multiple
**			  source buffers.
**
**	offset		- offset into reserved area to start write.
**			  Must be a multiple of eight.
**
**	length		- size of data in bytes. 'offset' + 'length'
**			  must not exceed amount initially reserved
**			  by CXmsg_stow.  Amount written is padded
**			  out to a multiple of 8 with trailing zeroes
**			  if need be.
**
** Outputs:
**
**	none
**
**	Returns:
**		OK			- Normal successful completion.
**		E_CL2C11_CX_E_BADPARAM	- Bad length, and/or bad Chit.
**		E_CL2C12_CX_E_INSMEM	- Insufficient resources for
**					  request.
**		E_CL2C1B_CX_E_BADCONTEXT - CX uninitialized.
**	Exceptions:
**	    none
**
** Side Effects:
**
**	None.
**
** Notes:
**	Tru64 IMC implementation is not checking for memory latency
**	effects, and is trusting that updates written to memory
**	have been fully propagated.   Since this propagation is
**	VERY fast, this should never be a problem between separate
**	nodes of a cluster, since by the time remote cluster gets
**	the chit, and is ready to act on it, update is virtually
**	certain to have been propagated.   However, since writing
**	node is the last to have its local read copy of the IMC
**	global shared memory updated, and it already has the chit
**	in hand, it may just be possible for this to occur.  To avoid
**	any possible problem, we don't check the length and chit
**	value stored in GSM, and we only allow 'offset' values
**	on eight byte boundries.
**
** History:
**	12-sep-2002 (devjo01)
**	    Split out from CXmsg_stow.
**	28-mar-2004 (devjo01)
**	    Correctly return E_CL2C10_CX_E_NOSUPPORT as needed.
**	04-apr-2004 (devjo01)
**	    Add CX_CLM_FILE support.
*/
STATUS
CXmsg_append( i4 chit, PTR pdata, i4 offset, i4 length, i4 totlen )
{
    STATUS	 status;
    i4		 ichit;

# ifdef CX_PARANOIA
    if ( length <= 0 || offset < 0 || (offset & 0x7) ||
	 (offset + length) > CX_MAX_MSG_STOW )
	return E_CL2C11_CX_E_BADPARAM;
    if ( CX_ST_UNINIT == CX_Proc_CB.cx_state ) 
	return E_CL2C1B_CX_E_BADCONTEXT;
# endif /*CX_PARANOIA*/

    switch ( CX_Proc_CB.cx_clm_type )
    {
    case CX_CLM_FILE:
	status = cx_clm_file_write(chit, pdata, offset, length, totlen);
	break;

    case CX_CLM_IMC:

    /* Start outdent by 4. */
    ichit = chit & CX_MSG_CHIT_MASK;
    if ( ichit <= 0 || ichit > CX_MSG_MAX_CHIT )
	return E_CL2C11_CX_E_BADPARAM;

    /*
    **	Start OS dependent declarations.
    */
#if defined(axp_osf) && defined(conf_CLUSTER_BUILD)
    /*
    **	Tru64 implementation
    */
    {
	union {
	    CX_MSG_HEAD		header;
	    char		tail[8];
	    i4			zeds[2];
	}		 workbuf; /* Work buffer */
	i4		 cpyamt;
	CX_MSG_FRAG	*ptarg;

	/* Do a quick scram if IMC is missing or uninitialized. */
	if ( NULL == CX_Proc_CB.cxaxp_imc_txa )
	    return E_CL2C10_CX_E_NOSUPPORT;

	/* Step 1, calc target address */
	ptarg = (CX_MSG_FRAG*)( (PTR)CX_Proc_CB.cxaxp_imc_txa + 
	 ((ichit - 1) * CX_MSG_FRAGSIZE) + sizeof(workbuf.header) + offset );

	/* Step 2, copy body of message */
	if ( 0 != ( cpyamt = ( length & ~(0x7) ) ) )
	{
	    bcopy( pdata, ptarg, cpyamt );
	    pdata += cpyamt;
	    ptarg = (CX_MSG_FRAG*)( (PTR)ptarg + cpyamt );
	}

	/* Step 3, copy any trailing fragment */
	if ( length & 0x7 )
	{
	    workbuf.zeds[0] = workbuf.zeds[1] = 0;
	    bcopy( pdata, (PTR)&workbuf.tail, length & 0x7 );
	    bcopy( (PTR)&workbuf, ptarg, sizeof(workbuf.tail) );
	}
	status = OK;
    }
#else /* all others */
    status = E_CL2C10_CX_E_NOSUPPORT;
#endif /* axp_osf */

	/* Restore correct indentation */
	break;

    default:
	status = E_CL2C10_CX_E_NOSUPPORT;
    }
    return status;
} /* CXmsg_append */



/*{
** Name: CXmsg_redeem	- Retreive a "long" message via a chit.
**
** Description:
**
**      This routine retreives a message too large to send in the standard
**	rather tiny "message" structure, via a chit propagated by
**	other means (most likely a standard message block).
**
** Inputs:
**
**      chit		- i4 which will holds the CX message chit.
**
**	buffer		- Pointer to buffer to receive "long" message.
**
**	bufsize		- size of buffer in bytes.  Note, since normally
**			  caller will know the message size, message
**			  will be silently truncated if caller chooses
**			  to provide an insufficently large buffer.
**
**	offset		- # of bytes to skip in message before copying
**			  into buffer, should be non-zero only if
**			  continuing read of a partially read message.
** 
**	retreived	- pointer to i4 to be filled with actual amount
**			  needed, or retreived.  Note, since normally
**			  caller will know the message size, message
**			  will be truncated without error status if caller
**			  chooses to provide an insufficently large buffer.
**
** Outputs:
**
**	none
**
**	Returns:
**		OK			- Normal successful completion.
**		E_CL2C11_CX_E_BADPARAM	- Invalid chit.
**		E_CL2C1B_CX_E_BADCONTEXT - CX uninitialized.
**		E_CL2C2C_CX_E_MSG_NOSUCH - Message for chit looks bad.
**	Exceptions:
**	    none
**
** Side Effects:
**
**	None.
**
**	Note: for the same reasons as stated in CXmsg_append, offset
**	must be a multiple of 8 bytes.
**
** History:
**	29-may-2002 (devjo01)
**	    Created.
**	12-sep-2002 (devjo01)
**	    Added offset to allow partial reads into one or more
**	    separate memory regions.
**	28-mar-2004 (devjo01)
**	    Correctly return E_CL2C10_CX_E_NOSUPPORT as needed.
**	04-apr-2004 (devjo01)
**	    Add CX_CLM_FILE support.
*/
STATUS
CXmsg_redeem( i4 chit, PTR buffer, i4 bufsize, i4 offset, i4 *retreived )
{
    STATUS	 status;
    i4		 ichit;

# ifdef CX_PARANOIA
    if ( bufsize <= 0 )
	return E_CL2C11_CX_E_BADPARAM;
    if ( !retreived )
	return E_CL2C11_CX_E_BADPARAM;
    if ( CX_ST_UNINIT == CX_Proc_CB.cx_state ) 
	return E_CL2C1B_CX_E_BADCONTEXT;
# endif /*CX_PARANOIA*/

    *retreived = 0;

    switch ( CX_Proc_CB.cx_clm_type )
    {
    case CX_CLM_FILE:
	status = cx_clm_file_read(chit, buffer, bufsize, offset, retreived);
	break;

    case CX_CLM_IMC:

    /* Start outdent by 4 */
    ichit = chit & CX_MSG_CHIT_MASK;
    if ( ichit <= 0 || ichit > CX_MSG_MAX_CHIT )
	return E_CL2C11_CX_E_BADPARAM;

    /*
    **	Start OS dependent declarations.
    */
#if defined(axp_osf) && defined(conf_CLUSTER_BUILD)
    /*
    **	Tru64 implementation
    */
    {
	CX_MSG_FRAG	*psmsg;
	i4		 length, cpyamt;
	union {
	    CX_MSG_HEAD		header;
	    char		tail[8];
	}		 workbuf;

	for ( ; /* Something to break out of */ ; )
	{
	    /* Do a quick scram if IMC is missing or uninitialized. */
	    if ( NULL == CX_Proc_CB.cxaxp_imc_rxa )
	    {
		status = E_CL2C10_CX_E_NOSUPPORT;
		break;
	    }

	    /* Step 1, calc source address */
	    psmsg = (CX_MSG_FRAG *)CX_Proc_CB.cxaxp_imc_rxa + (ichit - 1);

	    /* Step 2, Grab 8 byte header */
	    bcopy( psmsg, &workbuf, sizeof(workbuf.header) );
	    length = workbuf.header.cx_lmh_length;
	    
	    /*
	    ** Step 3, check that message is still valid.
	    ** Depending on usage not finding a message could
	    ** be a critical error, or an expected condition.
	    */
	    if ( workbuf.header.cx_lmh_chit != chit )
	    {
		status = E_CL2C2C_CX_E_MSG_NOSUCH;
		break;
	    }

	    /*
	    ** Step 4, verify that stored length is sensible,
	    ** then store number of bytes left to be retrieved,
	    ** modified by offset.
	    */
	     
	    if ( length <= 0 ||
		 length > CX_MAX_MSG_STOW ||
		 ((PTR)psmsg + length) >
		  ((PTR)CX_Proc_CB.cxaxp_imc_rxa + CX_MSG_IMC_SEGSIZE) )
	    {
		status = E_CL2C1A_CX_E_CORRUPT;
		break;
	    }

	    status = OK;

	    if ( offset >= length )
	    {
		/* Reading past end of message */
		break;
	    }

	    psmsg = (CX_MSG_FRAG *)((PTR)psmsg + 
	      sizeof(workbuf.header) + offset);
	    length -= offset;

	    *retreived = length;

	    if ( length > bufsize ) 
		length = bufsize;

	    /* Step 5, copy body of message */
	    if ( 0 != ( cpyamt = ( length & ~(0x7) ) ) )
	    {
		bcopy( psmsg, buffer, cpyamt );
		buffer += cpyamt;
		psmsg = (CX_MSG_FRAG *)((PTR)psmsg + cpyamt);
	    }

	    /* Step 6, copy any trailing fragment */
	    if ( length & 0x7 )
	    {
		bcopy( psmsg, (PTR)workbuf.tail, 8 );
		bcopy( workbuf.tail, buffer, length & 0x7 );
	    }
	    break;
	}
    }
#else /* all others */
    status = E_CL2C10_CX_E_NOSUPPORT;
#endif /* axp_osf */

	/* Restore correct indentation. */
	break;

    default:
	status = E_CL2C10_CX_E_NOSUPPORT;
    }
    return status;
} /* CXmsg_redeem */


/*{
** Name: CXmsg_release	- Release resources associated with a chit.
**
** Description:
**
**      This routine releases the implementation dependent resources
**	associated with a "chit", clearing chit afterwards.
**
**	This is typically done as port of the sequence
**
**	stat = CXmsg_stow( &chit, ... );
**	if ( OK == stat )
**	{
**	    // put "chit" in "small" message block, and send it.
**	    . . .
**	    stat = CXmsg_send( channel, , &msg );
**	
**	    // When send returns, everyone connected to channel
**	    // will have had a chance to copy "long" message text
**	    // into their own local storage, so we can free the
**	    // global copy.
**	    CXmsg_release( &chit );
**	}
**
** Inputs:
**
**      pchit		- pointer to an i4 which holds a chit value
**			  previously returned by CXmsg_stow.
**
** Outputs:
**
**	*pchit		is zeroed.
**
**	Returns:
**		OK			- Normal successful completion.
**		E_CL2C11_CX_E_BADPARAM	- Invalid chit.
**		E_CL2C1B_CX_E_BADCONTEXT - CX uninitialized.
**	Exceptions:
**	    none
**
** Side Effects:
**
**	None.
**
** History:
**	29-may-2002 (devjo01)
**	    Created.
**	12-sep-2002 (devjo01)
**	    Clear header for message being released.
**	28-mar-2004 (devjo01)
**	    Correctly return E_CL2C10_CX_E_NOSUPPORT as needed.
**	04-apr-2004 (devjo01)
**	    Add CX_CLM_FILE support.
*/
STATUS
CXmsg_release( i4 *pchit )
{
    STATUS	 status;
    i4		 chit, ichit;

# ifdef CX_PARANOIA
    if ( CX_ST_UNINIT == CX_Proc_CB.cx_state ) 
	return E_CL2C1B_CX_E_BADCONTEXT;
    if ( !pchit )
	return E_CL2C11_CX_E_BADPARAM;
# endif /*CX_PARANOIA*/

    switch ( CX_Proc_CB.cx_clm_type )
    {
    case CX_CLM_FILE:
	return OK;
	break;

    case CX_CLM_IMC:
	/* Start outdent by 4. */

    chit = *pchit;
    ichit = CX_MSG_CHIT_MASK & chit;
    if ( ichit <= 0 || ichit > CX_MSG_MAX_CHIT )
	return E_CL2C11_CX_E_BADPARAM;

    /*
    **	Start OS dependent declarations.
    */
#if defined(axp_osf) && defined(conf_CLUSTER_BUILD)
    /*
    **	Tru64 implementation
    */
    {
	CX_MSG_FRAG	*psmsg;
	CX_MSG_HEAD     header;
	CX_CMO		cmo;

	do
	{
	    /* Do a quick scram if IMC is missing or uninitialized. */
	    if ( NULL == CX_Proc_CB.cxaxp_imc_txa )
	    {
		status = E_CL2C10_CX_E_NOSUPPORT;
		break;
	    }

	    /* ndata holds chit value going it, secondary status on return */
	    status = CXcmo_update( CX_CMO_CXMSG_IDX, &cmo,
			 cxmsg_axp_free_msg_bits, (PTR)&chit );
	    if ( OK == status )
	    {
		status = (STATUS)chit;
	    }
	    if ( OK == status )
	    {
		*pchit = 0;
	    }
	} while (0);
    }
#else /* all others */
    status = E_CL2C10_CX_E_NOSUPPORT;
#endif /* axp_osf */

	/* Restore correct indent */
	break;

    default:
	status = E_CL2C10_CX_E_NOSUPPORT;
    }
    return status;
} /* CXmsg_release */

#if defined(axp_osf) && defined(conf_CLUSTER_BUILD)
/*
**  CMO update function used to release (free) a
**  range of IMC memory.
*/
static STATUS
cxmsg_axp_free_msg_bits( CX_CMO *oldval, CX_CMO *pnewval,
                        PTR pdata, bool invalidin )
{
    u_i4	         *pbits;
    u_i4		 mask;
    i4			 frags, bitsover;
    union {
	CX_CMO		cmo;
	CX_MSG_HEAD	header;
	i4		zeds[2];
    }		 workbuf; /* Work buffer */
    i4		 length, chit, chitidx, startbit;
    CX_MSG_FRAG *ptarg;
    CX_MSG_FRAG *psmsg;

    bcopy( (PTR)oldval, (PTR)pnewval, sizeof(CX_CMO) );

    /* Convert chit into index form. */
    chit = *(i4 *)pdata;
    chitidx = (chit & CX_MSG_CHIT_MASK) - 1;
    *(i4 *)pdata = OK;

    /* Step 1, calc victim addresses */
    ptarg = (CX_MSG_FRAG *)CX_Proc_CB.cxaxp_imc_txa + chitidx;
    psmsg = (CX_MSG_FRAG *)CX_Proc_CB.cxaxp_imc_rxa + chitidx;

    for ( ; /* Something to break out of */ ; )
    {
	/* Step 2, Grab 8 byte header */
	bcopy( psmsg, &workbuf, sizeof(workbuf.header) );
	if ( chit != workbuf.header.cx_lmh_chit )
	{
	    *(i4 *)pdata = E_CL2C2C_CX_E_MSG_NOSUCH;
	    break;
	}

	length = workbuf.header.cx_lmh_length;
	
	if ( length <= 0 ||
	     length > CX_MAX_MSG_STOW ||
	     ((PTR)psmsg + length + sizeof(CX_MSG_HEAD)) >
	      ((PTR)CX_Proc_CB.cxaxp_imc_rxa + CX_MSG_IMC_SEGSIZE) )
	{
	    *(i4 *)pdata = E_CL2C1A_CX_E_CORRUPT;
	    break;
	}

	/* Step 3, Zap message header, to help catch stale message fetches */
	workbuf.zeds[0] = workbuf.zeds[1] = 0;
	bcopy( (PTR)&workbuf, ptarg, sizeof(workbuf.zeds) );

	frags = ( length + sizeof(CX_MSG_HEAD) + CX_MSG_FRAGSIZE - 1 ) / 
	  CX_MSG_FRAGSIZE; 

	/* Step 4, calculate and clear bits for message frags */
	pbits = (u_i4 *)pnewval + chitidx / 32;
	startbit = (chitidx & 0x1f);
	if ( startbit + frags > 32 )
	{
	    /* Msg bits spanned a word kill off 1st set of bits */
	    bitsover = startbit + frags - 32;
	    frags = frags - bitsover;

	    /* Kill some bits */
	    mask = ((1 << frags) - 1) << startbit;

	    if ( ( *pbits & mask ) != mask )
	    {
		/* Not all bits are set! What goes? */
		*(i4 *)pdata = E_CL2C1A_CX_E_CORRUPT;
		break;
	    }

	    *pbits ^= mask;

	    /* Set up to kill the rest. */
	    pbits++;
	    frags = bitsover;
	    startbit = 0;
	}

	/* Kill remaining message bits */
	mask = ((1 << frags) - 1) << startbit;

	if ( ( *pbits & mask ) != mask )
	{
	    /* Not all bits are set! What goes? */
	    *(i4 *)pdata = E_CL2C1A_CX_E_CORRUPT;
	    break;
	}

	*pbits ^= mask;
	break;
    }

    /* Always return OK */
    return OK;
} /* cxmsg_axp_free_msg_bits */

#endif /* axp_osf */


/*{
** Name: CXmsg_clmclose	- Release CLM resources associated with a node.
**
** Description:
**
**      This routine releases the implementation dependent resources
**	associated with a "long message connection" to a node.
**
** Inputs:
**
**      node		- Node to release resources for.
**
** Outputs:
**
**	None
**
**	Returns:
**		OK			- Normal successful completion.
**		E_CL2C1B_CX_E_BADCONTEXT - CX uninitialized.
**	Exceptions:
**	    none
**
** Side Effects:
**
**	None.
**
** History:
**	14-may-2004 (devjo01)
**	    Created.
**	28-Mar-2005 (jenjo02)
**	    After freeing clmfcb_buf, null clfmcb_bufsize,
**	    clmfcb_buf so cx_clm_file_read() will notice and
**	    re-open the file the next time around.
**	11-Aug-2005 (fanch01)
**	    Last change didn't quite cover the case where
**	    clmfcb_bufwoff!=0 causing a SEGV during memcpy.
**	    Setting it to 0 here.
**	    
*/
STATUS
CXmsg_clmclose( i4 node )
{
    STATUS	 status, tstatus;
    CL_ERR_DESC  syserr;

# ifdef CX_PARANOIA
    if ( CX_ST_UNINIT == CX_Proc_CB.cx_state ) 
	return E_CL2C1B_CX_E_BADCONTEXT;
# endif /*CX_PARANOIA*/

    switch ( CX_Proc_CB.cx_clm_type )
    {
    case CX_CLM_FILE:
	{
	    CX_CLM_FCB	*fcb;

	    fcb = &Clm_fcbs[node - 1];

	    /* Get exclusive access to process level resources */
	    status = CSp_semaphore(1, &fcb->clmfcb_sem);
	    if ( OK != status ) break;
	
	    if ( 0 != fcb->clmfcb_bufpages )
	    {
		tstatus = DIclose( &fcb->clmfcb_dio, &syserr );
		REPORT_BAD_STATUS(tstatus);
		tstatus = MEfree_pages( fcb->clmfcb_buf,
				       fcb->clmfcb_bufpages, &syserr );
		REPORT_BAD_STATUS(tstatus);
		fcb->clmfcb_bufpages = 0;
		fcb->clmfcb_bufsize = 0;
		fcb->clmfcb_bufwoff = 0;
		fcb->clmfcb_buf = NULL;
	    }

	    tstatus = CSv_semaphore(&fcb->clmfcb_sem);
	    REPORT_BAD_STATUS(tstatus);
	}
	break;

    case CX_CLM_IMC:
	status = OK;
	break;

    default:
	status = E_CL2C10_CX_E_NOSUPPORT;
    }
    return status;
} /* CXmsg_clmclose */


/*{
** Name: CXmsg_channel_ready - Check if channel ready.
**
** Description:
**
**      This allows a thread to check if a message channel is
**	available before sending a message.
** 
**	Since processes receive their own messages, and processing
**	logic may depend on this, this procedure allows caller
**	to guard against race conditions in which a thread
**	broadcasting a message during initialization, winds
**	up getting scheduled ahead of the thread setting up
**	the receive channel.
**
** Inputs:
**
**	channel		- Channel number.
**
** Outputs:
**
**	none
**
**	Returns:
**		TRUE	- Channel is setup to receive messages on
**			  caller's process.
**		FALSE	- Message channel is not yet ready, or has
**			  failed.
**		
**	Exceptions:
**	    none
**
** Side Effects:
**
**	none.
**
** History:
**	09-mar-2001 (devjo01)
**	    Created.
*/
bool
CXmsg_channel_ready( u_i4 channel )
{
    return ( (channel < CX_MAX_CHANNEL) &&
	  CX_NONZERO_ID( &CX_Msg_CB[channel].cx_msg_hello_lkreq.cx_lock_id ) );
} /* CXmsg_channel_ready */




/*{
** Name: cx_msg_receive	- Read half of DLM protocol.
**
** Description:
**
**      This routine performs the read half of the message protocol.
**
**	Please see sir 103715, or the banner for CXmsg_send for
**	a description of this.
**	
**	Semantic processing of message is performed by the user routine
**	registered when connection with this channel is established.
**
**	User routine MUST NOT request any additional locks, or block in
**	any way.  Doing so could lead to undetected deadlocks.
**
**	Routine may also be called in the context of any session or no
**	session when invoked because of a signal driven blocking
**	notification routine, so routine should not do anything it
**	could not do in the context of a signal handler.
**  	
** Inputs:
**
**	pmcb		- Pointer to MCB this channel.
**
** Outputs:
**
**	none
**
**	Returns:
**		OK			- Normal successful completion.
**		E_CL2C05_CX_I_DONE	- User function requests channel
**					  shutdown.
**		
**	Exceptions:
**	    none
**
** Side Effects:
**
**	<INC>.
**
** History:
**	05-mar-2001 (devjo01)
**	    Created.
*/
static STATUS
cx_msg_receive( CX_MSG_CB *pmcb )
{
    STATUS		 status;
    bool		 invalidblk, finalmsg;
    CX_REQ_CB		*phlk, *pglk;

    for ( ; /* Something to break out of */ ; )
    {
	phlk = &pmcb->cx_msg_hello_lkreq;
	pglk = &pmcb->cx_msg_goodbye_lkreq;

	/* Step 1: Allocate "goodbye" lock if not already done. */
	if ( !CX_NONZERO_ID( &pglk->cx_lock_id ) )
	{
	    pglk->cx_new_mode = LK_S;
	    status = CXdlm_request( CX_F_STATUS | CX_F_PCONTEXT |
				    CX_F_NODEADLOCK | CX_F_NO_DOMAIN, pglk,
				    &pmcb->cx_msg_transid );
	    if ( OK == status )
		status = CXdlm_wait( 0, pglk, 0 );
	    if ( CX_ERR_STATUS( status ) )
		break;
	}

	/* Step 2: Convert "Hello" to NULL mode. */
	phlk->cx_new_mode = LK_N;
	status = CXdlm_convert( CX_F_STATUS | CX_F_NODEADLOCK | CX_F_OWNSET,
	  phlk ); 
	if ( OK == status )
	    status = CXdlm_wait( 0, phlk, 0 );
	if ( CX_ERR_STATUS( status ) )
	    break;

	/* Step 3: Convert "Hello" back to shared. */
	phlk->cx_new_mode = LK_S;
	(void)CXdlm_convert( CX_F_STATUS | CX_F_NODEADLOCK | CX_F_OWNSET,
	  phlk ); 
	status = CXdlm_wait( 0, phlk, 0 );
	if ( OK == status )
	    invalidblk = FALSE;
	else if ( E_CL2C08_CX_W_SUCC_IVB == status )
	    invalidblk = TRUE;
	else 
	    break;
	
	/* Step 4: Call registered user function */
	finalmsg = (*pmcb->cx_msg_rcvfunc)( (CX_MSG*)&phlk->cx_value,
				 pmcb->cx_msg_rcvauxdata, invalidblk );

	/* Step 5: Downgrade "Goodbye" lock to acknowlege message receipt. */
	pglk->cx_new_mode = LK_N;
	status = CXdlm_convert( CX_F_STATUS | CX_F_OWNSET | CX_F_NODEADLOCK,
	  pglk );
	if ( OK == status )
	    status = CXdlm_wait( 0, pglk, 0 );
	if ( CX_ERR_STATUS( status ) )
	{
	    /* Unexpected error */
	    break;
	}

	/* Step 6: Convert "Goodbye" back to share to complete cycle. */
	pglk->cx_new_mode = LK_S;
	(void)CXdlm_convert( CX_F_STATUS | CX_F_OWNSET | CX_F_NODEADLOCK, 
	  pglk );
	status = CXdlm_wait( 0, pglk, 0 );
	break;
    }

    if ( !CX_ERR_STATUS( status ) && finalmsg )
    {
	status = E_CL2C05_CX_I_DONE;
    }

    return status;
} /* cx_msg_receive */




/*{
** Name: cx_msg_blk_rtn	- "Hello" lock blocking notice routine.
**
** Description:
**
**      This is just a wrapper to convert parameters to the values
**	expected by cx_msg_receive.
**
** Inputs:
**
**	preq		- Pointer to lock request block this channel.
**
** Outputs:
**
**	none
**
**	Returns:
**		none
**		
**	Exceptions:
**	    none
**
** Side Effects:
**
**	Message monitor thread is woken.
**
** History:
**	05-mar-2001 (devjo01)
**	    Created.
**	29-aug-2002 (devjo01)
**	    Add unused 2nd parameter 'ignore' to match CX_USER_FUNC.
*/
static VOID
cx_msg_blk_rtn( CX_REQ_CB *preq, i4 ignore )
{
    CX_Stats.cx_msg_notifies++;
    ((CX_MSG_CB *)(preq->cx_user_extra))->cx_msg_pending = TRUE;
    CSresume_from_AST( CX_Msg_Mon.cx_mm_sid );
} /* cx_msg_blk_rtn */




/*{
** Name: cx_mcb_format	- Partially format an MCB.
**
** Description:
**
**      This partially initializes the fields of a 
**	'M'essage 'C'ontrol 'B'lock.
**
** Inputs:
**
**	preq		- Pointer to lock request block this channel.
**
** Outputs:
**
**	none
**
**	Returns:
**		none
**		
**	Exceptions:
**	    none
**
** Side Effects:
**
**	none.
**
** History:
**	05-mar-2001 (devjo01)
**	    Created.
**	20-jul-2005 (devjo01)
**	    cx_key is now a struct.
*/
static STATUS
cx_mcb_format( u_i4 channel, CX_MSG_CB *pmcb )
{
    STATUS	 status;

    /* Zap entire MCB */
    (void)MEfill( sizeof(CX_MSG_CB), '\0', (PTR)pmcb );

    /* Partially init "Hello" lock */
    pmcb->cx_msg_hello_lkreq.cx_key.lk_type = LK_MSG;
    pmcb->cx_msg_hello_lkreq.cx_key.lk_key1 =
     CX_MSG_KEY(channel,CX_MSG_HELLO_KEY);

    /* Partially init "Goodbye" lock. */
    pmcb->cx_msg_goodbye_lkreq.cx_key.lk_type = LK_MSG;
    pmcb->cx_msg_goodbye_lkreq.cx_key.lk_key1 =
     CX_MSG_KEY(channel,CX_MSG_GOODBYE_KEY);

    status = CXunique_id( &pmcb->cx_msg_transid );
    return status;
} /* cx_mcb_format */



/* EOF:CXMSG.C */
