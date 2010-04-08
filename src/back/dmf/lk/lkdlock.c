/*
** Copyright (c) 1992, 2007 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <di.h>
#include    <er.h>
#include    <me.h>
#include    <nm.h>
#include    <pc.h>
#include    <st.h>
#include    <sl.h>
#include    <cv.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <tr.h>
#include    <ulf.h>
#include    <dmf.h>
#include    <dm.h>
#include    <dmccb.h>
#include    <lk.h>
#include    <cx.h>
#include    <lkdef.h>
#include    <lgkdef.h>

#if defined(conf_CLUSTER_BUILD)

/*
** Name: LKDLOCK.C	- Cluster Global Deadlock searching services.
**
** Description:
**	This portion of the locking system is used solely by the DMFCSP Cluster
**	Support Process and is thus only part of the Cluster Locking System.
**
** History:
**	Summer, 1992 (bryanp)
**	    Working on the new logging and locking system.
**	16-feb-1993 (andys)
**	    Check for LLB_WAITING status before we try to access the
**	    llb_wait field. We try to only set llb_wait when we are waiting
**	    and to simultaneously set LLB_WAITING.
**	15-mar-1993 (andys)
**	    Removed "#if 0" to allow deadlock searching to proceed in 
**	    LK_deadlock. Updated code to use 6.5 routines.
**	24-may-1993 (bryanp)
**	    New arguments to LK_wakeup for mutex optimizations.
**	    Also added lkreq argument to LK_wakeup for shared locklists.
**	21-jun-1993 (bryanp)
**	    Worked on deadlock detection bugs introduced by the 6.5 usage of
**	    fair scheduling lock request algorithms in the cluster.
**	30-jun-1993 (shailaja)
**	    Fixed compiler warning on empty translation unit.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (bryanp)
**	    Include <tr.h>
**	21-aug-1995 (dougb)
**	    As a related update to the fix for bug #70696, ensure we return
**	    SS$_INSFMEM when required. The bug fix may increase the fan-out
**	    for a deadlock search (potentially, by a large amount).  Thus,
**	    it is a "bad" idea to have any fixed limits on the number of
**	    output messages.  We must also update msg_cnt to reflect the
**	    number of messages which really should have been sent.
**	30-aug-1995 (dougb) bug #70696
**	    Send out messages specifying the maximum request mode of the
**	    waiting request.
**	31-aug-1995 (dougb) bug #70696
**	    Recurse through the local wait-for graph to exhaust it
**	    completely.
**	18-sep-1995 (duursma) bug #71331
**	    Don't recurse to the same RSB for convert requests.
**	19-sep-1995 (dougb) bug #71332
**	    Ensure we don't update the lock database while a server process is
**	    attempting to use it.  In particular, avoid a race condition when
**	    updating lkb->lkreq.status.  Don't want CSP making an update
**	    while a DBMS thread is executing code in LKrequest().
**	    Correct LK_CANCEL and SS$_CANCELGRANT handling in found_it().  The
**	    LK_CANCEL return value is a major error here.  SS$_CANCELGRANT
**	    means that the request has been granted -- but it should still be
**	    stalled if necessary (don't call LK_wakeup() with LK_NOSTATUS).
**	20-sep-1995 (duursma) bug #71334
**	    In order to detect initial local portions of global deadlock
**	    loops, change LKdeadlock so that it sends out deadlock messages
**	    about each of the blocking resources on the local node.
**	    Split off new routine walk_wait_for_graph() to facilitate
**	    this change.
**	04-jan-1996 (duursma) bug #71334
**	    Added more_info() routine and various other debugging printouts.
**	    Removed some comments in LKdeadlock() which were no longer valid.
**	    Changed the meaning of OK and LK_DEADLOCK. OK now means no deadlock
**	    was detected (but the search may have to continue), while
**	    LK_DEADLOCK means that deadlock was detected (and taken care of).
**	23-Feb-1996 (jenjo02)
**	    Brought up-to-date semaphore-wise. lkb_mutex has been supplanted
**	    by a host of new and exciting semaphores.
**	15-May-1996 (jenjo02)
**	    Brought forward repairs to lkrqst.c's deadlock() function to
**	    like code in walk_wait_for_graph().
**	30-may-1996 (pchang)
**	    Added LLB as a parameter to the call to LK_allocate_cb() to enable
**	    the reservation of SBK table resource for use by the recovery
**	    process. (B76879)
**	    Also, removed trigraph from comment while I'm here.
**	21-aug-1996 (boama01)
**	    Multiple chgs to LKdeadlock(), walk_wait_for_graph() and found_it()
**	    for bugs uncovered in Cluster tests:
**          - Replaced "i4 *rsh_table" with "RSH *rsh_table" to prevent
**            access violation in DMFCSP.
**          - Replaced "RSB *rsb_hash_table" with "RSH *rsb_hash_table".
**          - Replaced (void)LK_mutex/unmutex(&rsb_hash_bucket->rsb_mutex)
**                with (void)LK_mutex/unmutex(&rsb_hash_bucket->rsh_mutex)
**	      in the "Search the RSB hash bucket for a matching key" logic;
**            the rsb_hash_bucket at the moment points to the RSH block.
**	    - Improved and supplemented DEADLOCK_DEBUG traces; moved more_info
**	      call in LKdeadlock() to where it makes sense; also added
**	      search_length decrement in recursive part of walk_wait_for_graph.
**	    - Replaced (void)LG_unmutex(&original_rsb->rsb_mutex) with
**            (void)LK_unmutex(&original_rsb->rsb_mutex) to fix errors
**              E_CL250A_CS_NO_SEMAPHORE  E_DMA42F_LG_UNMUTEX
**	        E_CL2517_CS_SMPR_DEADLOCK
**            when running clustered VMS installation.
**	    - found_it() was prototyped and coded with a *LKD parm, but called
**	      from walk_wait_for_graph() with a *LKB parm!  Needless to say,
**	      this caused mem corruption probs.  A *LKD parm is pointless
**	      anyway, since this can be determined globally.  Chgd found_it()
**	      and its callers to only use *LLB parm.  Also removed an extra
**	      unmutex of RSB in walk_wait_for_graph(), before it calls
**	      found_it().
**	14-Nov-1996 (jenjo02)
**	    Deadlock() cleanup. Instead of taking all those SHARED mutexes
**	    (RSB, LLB) and checking for rsb_dlock, etc., do the following:
**	      o Whenever a value which affects the wait-for graph is
**	        about to be changed, lock an installation-wide mutes,
**	        lkd_dlock_mutex, make the change, and release the mutex.
**	      o All variables which affect the graph must be mutexed in
**	        this manner.
**		    - the RSB grant and wait queues
**		    - llb_wait (LKB *)
**		    - LLB_WAITING status
**		    - lkb_state
**		    - lkb_request_mode
**	      o The deadlock() code will take an EXCL lock on lkd_dlock_mutex
**		while it does its business. If the protocol is adhered to,
**	        this guarantees that the graph will not change while its
**		being computed, therefore there is no need for deadlock()
**	        to take any lower-level mutexes.
**	16-Dec-1996 (jenjo02)
**	    In found_it(), lock lkd_dlock_mutex before calling LK_resume().
**	19-Mar-1997 (jenjo02)
**	  - Instead of scanning the LLB array for lock lists of interest,
**	    use the queue of LLBs that haven't been deadlock searched. This
**	    queue was created for the interval-based deadlock detection 
**	    thread.
**	  - Changed walk_wait_for_graph() to return LK_DEADLOCK if a deadlock
**	    was detected instead of calling found_it(). Caller is responsible 
**	    for calling found_it(). Caller must also acquire lkd_dlock_mutex
**	    before calling walk_wait_for_graph().
**	14-Aug-1997 (toumi01)
**	    Move #ifdef etc. to column 1, as required by many compilers.
**	16-Nov-1998 (jenjo02)
**	    Cross-process thread identity changed to CS_CPID structure
**	    from PID and SID.
**	14-Jan-1999 (jenjo02)
**	    Recheck LLB_ON_DLOCK_LIST state after waiting for mutex in
**	    LKdeadlock().
**	16-feb-1999 (nanpr01)
**	    Support for update mode lock.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	12-apr-2001 (devjo01)
**	    s103715 (Portable cluster support)
**	20-Aug-2002 (jenjo02)
**	    Updated to current LK architecture:
**	    - LLB may have multiple waiting lock requests (llb_lkb_wait)
**	      instead of just one (llb_wait), hence the "unit of wait"
**	      is an LKB, not an LLB.
**	    - Lock requestors are responsible for cancelling themselves
**	      when a deadlock occurs. Attempts to LK_cancel() a 
**	      lock request in an indeterminate state from another thread
**	      are frought with timing-related problems.
**	    - Mutexing scheme in wait-for graph totally redone (for the
**	      last time?); it'd be nice if the two functions "deadlock()"
**	      in LKrequest and "walk_wait_for_graph" could be combined
**	      into one so they don't keep getting out of synch!
**	17-sep-2003 (abbjo03)
**	    Changes to use VMS headers provided by HP.
**	23-oct-2003 (devjo01)
**	    Correct structure names for rsb_name within VMS only scope.
**	24-oct-2003 (devjo01)
**	    Change ALL instances of rsb_name references to conform to an
**	    LK_LOCK_KEY.
**	24-oct-2003 (devjo01)
**	    Change instances of lkb_lksb.lock_id references to 
**	    lkb_cx_req.cx_lock_id
**	26-jan-2004 (devjo01)
**	    Use CX_EXTRACT_LOCK_ID macro to copy OS varying cx_lock_id
**	    to the LKDSM lockid member, and/or a standard LK_UNIQUE.
**	    This allows for a consistent external DLM lock id
**	    representation across all platforms while allowing the
**	    internal representation to conform to the native DLM's
**	    expectation.
**      7-oct-2004 (thaju02)
**          Use SIZE_TYPE to allow memory pools > 2Gig.
**	30-Dec-2004 (schka24)
**	    Remove special case for iixprotect.
**	10-may-2005 (devjo01)
**	    Made generic and moved into RCP as part of sirs 114136 & 114504.
**	06-jun-2005 (devjo01)
**	    Corrected extern reference to LK_read_ppp_ctx.
**	    Correct other compiler warnings not returned under VMS.
**	16-nov-2005 (devjo01)
**	    Rename CXdlm_mark_as_deadlocked to CXdlm_mark_as_posted,
**	    and set cx_status locally to conform with new more
**	    generic usage of CXdlm_mark_as_posted.
**	21-nov-2006 (jenjo02/abbjo03)
**	    Don't discard messages originating from this node where lock_key
**	    does not match lkb_id->lock_key.  Enhance some trace messages.
**	    Remove commented out code.
**	02-jan-2007 (jenjo02/abbjo03)
**	    Additional trace code made dependent on II_CLUSTER_DEADLOCK_DEBUG
**	    logical.  Make LK_cont_dsmc and LK_read_ppp_ctx local statics.
**	    Reduce MAX_DSMS_PER_BUFFER to 744 to make the buffer fit within
**	    64K.
**	03-jan-2007 (abbjo03)
**	    Supplement 2-jan change: LK_DEADLOCK_MSG also has to depend on
**	    lk_cluster_deadlock_debug.
**	18-jan-2007 (jenjo02/abbjo030
**	    Compute max messages per buffer instead of hard-coding it.
**	    Add msg_seq to LK_DSM for debugging.  Add TRflush()es to ensure
**	    we don't miss debugging info.
**	01-Jun-2007 (jonj)
**	    Implement circular in-memory trace buffer to cache deadlock
**	    trace messages, display them on demand via iimonitor.
**	15-aug-2007 (jonj/joea)
**	    Enhance tracing interface via iimonitor.
**	06-Sep-2007 (jonj)
**	    Yet more iimonitor functions for tracing.
**	    Subvert looking for "waiting" LKBs, letting the more robust
**	    LK_local_deadlock() do that with mutex protection.
**	    When msg received is "back home" to the originating node
**	    on the originating key, check if the resource blocker is
**	    o_lkb - if not, continue the search.
**	09-Dec-2008 (jonj)
**	    SIR 120874: use new form uleFormat, CL_CLEAR_ERR.
*/


/*
** The LK_compatible matrix is defined in LKRQST.C, as is LK_convert_mode.
*/
GLOBALREF char	LK_compatible[];

static CX_PPP_CTX	 LK_read_ppp_ctx;

static LK_DSM_CONTEXT	*LK_cont_dsmc;

GLOBALCONSTREF char LK_convert_mode [8] [8];

#define		TRACE_OFF	0
#define		TRACE_BUFFER	1
#define		TRACE_DIRECT	2
GLOBALDEF i4 		Lk_cluster_deadlock_debug = TRACE_OFF;

static    bool		debug_cpp = FALSE;


static char *dsm_svc_name(void);

static void LK_dsm_context_priv_init( 
			LK_DSM_CONTEXT_PRIV *lk_dsm_priv);

static STATUS LK_flush_dsm_context_write( 
			LK_DSM *inlkdsm,
			LK_DSM_CONTEXT_PRIV *lk_dsm_ctx_priv, 
			i4 count, 
			u_i4 *writes,
			i4 node);

static		bool	cdt_cancel = FALSE;
static		bool	cdt_resume = FALSE;
static		u_i4	cdt_reads = 0;
static		u_i4	cdt_max_hops = 0;
static		u_i4	cdt_futile = 0;
static		u_i4	cdt_retries = 0;
static		u_i4	cdt_retry_sleeps = 0;
static		u_i4	cdt_local_wakes = 0;
static		u_i4	cdt_eoc_dead = 0;
static		u_i4	cdt_lg_dead = 0;
static		i4	Lk_dlock_trace_count	= 0;


/* defined or not in lkdef.h */
#ifdef LKDEADLOCK_DEBUG

#define		LK_DLOCK_MSG_SIZE	126
#define		LK_DLOCK_ENTRY_MAX	60000

typedef	struct	_LK_DLOCK_ENTRY	
{
    i2		len;
    char	msg[LK_DLOCK_MSG_SIZE];
} LK_DLOCK_ENTRY;

LK_DLOCK_ENTRY	Lk_dlock_traces[LK_DLOCK_ENTRY_MAX];

/* Function registered with iimonitor "lkcdtbuffer" to turn on in-buffer tracing */
static i4
lk_cdt_buffer( i4 (*output_fcn)(PTR, i4, char *), char *command )
{
    char 	buf[128];

    /* Turn on circular buffer tracing */
    Lk_cluster_deadlock_debug = TRACE_BUFFER;
    TRformat( output_fcn, (PTR)1, buf, sizeof(buf), 
		"In-memory Cluster deadlock tracing is enabled\n" );
    TRdisplay("%@ In-memory Cluster deadlock tracing is enabled\n");
    return(OK);
}

/* Function registered with iimonitor "lkcdtoff" to turn off all tracing */
static i4
lk_cdt_off( i4 (*output_fcn)(PTR, i4, char *), char *command )
{
    char 	buf[128];

    /* Turn off tracing */
    Lk_cluster_deadlock_debug = TRACE_OFF;
    TRformat( output_fcn, (PTR)1, buf, sizeof(buf), 
		"Cluster deadlock tracing is disabled\n" );
    TRdisplay("%@ Cluster deadlock tracing is disabled\n");
    return(OK);
}

/* Function registered with iimonitor "lkcdtdirect" to turn on direct debug tracing */
static i4
lk_cdt_direct( i4 (*output_fcn)(PTR, i4, char *), char *command )
{
    char 	buf[128];

    /* Start direct tracing */
    Lk_dlock_trace_count = 0;
    Lk_cluster_deadlock_debug = TRACE_DIRECT;
    TRformat( output_fcn, (PTR)1, buf, sizeof(buf), 
		"Direct Cluster deadlock tracing is enabled\n" );
    TRdisplay("%@ Direct Cluster deadlock tracing is enabled\n");
    return(OK);
}

/* Function registered with iimonitor "lkcdtcancel" to cancel in-progress dump */
static i4
lk_cdt_cancel( i4 (*output_fcn)(PTR, i4, char *), char *command )
{
    char 	buf[128];

    /* Cancel dump of in-memory traces */
    cdt_cancel = TRUE;
    TRformat( output_fcn, (PTR)1, buf, sizeof(buf), 
		"In-memory trace dump cancelled\n" );
    TRdisplay("%@ In-memory trace dump cancelled\n");
    return(OK);
}

/* Function registered with iimonitor "lkcdtresume" to resume stalled dist thread */
static i4
lk_cdt_resume( i4 (*output_fcn)(PTR, i4, char *), char *command )
{
    char 	buf[128];

    /* Resume dist thread */
    cdt_resume = TRUE;
    TRformat( output_fcn, (PTR)1, buf, sizeof(buf), 
		"LKdist thread resumed\n" );
    TRdisplay("%@ LKdist thread resumed\n");
    return(OK);
}

/* Function registered with iimonitor "lkcdtstats" to show some stats */
static i4
lk_cdt_stats( i4 (*output_fcn)(PTR, i4, char *), char *command )
{
    LKD		*lkd = (LKD *)LGK_base.lgk_lkd_ptr;
    char 	buf[128];

    TRformat( output_fcn, (PTR)1, buf, sizeof(buf), 
		"lkcdtstats: Reads %d Messages %d Retries %d Sleeps %d Wakes %d MaxHops %d(%d) Ld %d Dd (%d,%d)\n",
		    cdt_reads, lkd->lkd_stat.gdlck_call, cdt_retries, 
		    cdt_retry_sleeps, cdt_local_wakes, cdt_max_hops, cdt_futile,
		    lkd->lkd_tstat[0].deadlock, cdt_eoc_dead, cdt_lg_dead );
    TRdisplay("%@ lkcdtstats: Reads %d Messages %d Retries %d Sleeps %d Wakes %d MaxHops %d(%d) Ld %d Dd (%d,%d)\n",
		    cdt_reads, lkd->lkd_stat.gdlck_call, cdt_retries, 
		    cdt_retry_sleeps, cdt_local_wakes, cdt_max_hops, cdt_futile,
		    lkd->lkd_tstat[0].deadlock, cdt_eoc_dead, cdt_lg_dead );
    return(OK);
}

/* Function registered with iimonitor "lkcdtreset" to reset stats */
static i4
lk_cdt_reset( i4 (*output_fcn)(PTR, i4, char *), char *command )
{
    LKD		*lkd = (LKD *)LGK_base.lgk_lkd_ptr;
    char 	buf[128];

    cdt_reads = 0;
    lkd->lkd_stat.gdlck_call = 0;
    cdt_retries = 0;
    cdt_retry_sleeps = 0;
    cdt_local_wakes = 0;
    cdt_max_hops = 0;
    cdt_futile = 0;
    lkd->lkd_tstat[0].deadlock = 0;
    cdt_eoc_dead = 0;
    cdt_lg_dead = 0;

    TRformat( output_fcn, (PTR)1, buf, sizeof(buf), 
		"lkcdtreset: Reads %d Messages %d Retries %d Sleeps %d Wakes %d MaxHops %d(%d) Ld %d Dd %d\n",
		    cdt_reads, lkd->lkd_stat.gdlck_call, cdt_retries, 
		    cdt_retry_sleeps, cdt_local_wakes, cdt_max_hops, cdt_futile,
		    lkd->lkd_tstat[0].deadlock, cdt_eoc_dead, cdt_lg_dead );
    TRdisplay("%@ lkcdtreset: Reads %d Messages %d Retries %d Sleeps %d Wakes %d MaxHops %d(%d) Ld %d Dd %d\n",
		    cdt_reads, lkd->lkd_stat.gdlck_call, cdt_retries, 
		    cdt_retry_sleeps, cdt_local_wakes, cdt_max_hops, cdt_futile,
		    lkd->lkd_tstat[0].deadlock, cdt_eoc_dead, cdt_lg_dead );
    return(OK);
}

/* Function registered with iimonitor "lkcdtdump" to dump the in-memory traces */
static i4
lk_cdt_dump( i4 (output_fcn)(PTR, i4, char *), char *command )
{
    i4		ti, et;
    char	buf[128];
    LK_DLOCK_ENTRY	*msg;
    i4		written = 0;
    STATUS	status;

    /* Shut down tracing first */
    Lk_cluster_deadlock_debug = TRACE_OFF;
    cdt_cancel = FALSE;

    if ( ( et = Lk_dlock_trace_count ) < LK_DLOCK_ENTRY_MAX )
	ti = 0;
    else
	ti = et - LK_DLOCK_ENTRY_MAX;

    TRdisplay("\n%@ Beginning dump of %d cluster deadlock trace entries...\n", 
		et - ti);

    /* Dump cached trace entries to the log */
    while ( ti < et )
    {
	msg = Lk_dlock_traces + (ti % LK_DLOCK_ENTRY_MAX);
	TRwrite( (PTR)1, msg->len, (char*)&msg->msg );
	ti++;

	/* Periodically, check for interrupt */
	if ( (++written % 100) == 0 )
	{
	    /* Hiccup wait to allow interrupt */
	    status = CSsuspend(CS_INTERRUPT_MASK | CS_TIMEOUT_MASK, 1, 0);

	    if ( cdt_cancel || (status && status != E_CS0009_TIMEOUT) )
	    {
		TRformat( output_fcn, (PTR)1, buf, sizeof(buf), 
			"...interrupted...\n" );
		TRdisplay("\n%@ ...interrupted...\n");
		break;
	    }
	}
    }

    TRformat( output_fcn, (PTR)1, buf, sizeof(buf), 
		"Dumped %d trace messages to IIRCP log...\n",
			written);

    TRformat( output_fcn, (PTR)1, buf, sizeof(buf), 
		"Resetting trace count to zero...\n" );

    Lk_dlock_trace_count = 0;

    TRdisplay("\n%@ ...Ended dump of %d cluster deadlock trace entries\n", 
		written);

    cdt_cancel = FALSE;
    return(OK);
}

/*
** Callback function invoked by TR function do_format() when
** it is ready to write the formatted message piece.
*/
static void
move_to_tbuf(
PTR	arg,
i4	len,
char	*locl_buf )
{
    LK_DLOCK_ENTRY	*msg;
    i4			ti;

    ti = CSadjust_counter( &Lk_dlock_trace_count, 1 );
    msg = Lk_dlock_traces + ( (ti - 1) % LK_DLOCK_ENTRY_MAX);
    msg->len = len;
    MEcopy(locl_buf, msg->len, &msg->msg);
}

/*
** External function called when Lk_cluster_deadlock_debug is TRUE
** via LK_DLOCK_TRACE macro to format the trace message into the next 
** available slot in the circular trace buffer Lk_dlock_traces
*/

#if defined(__STDARG_H__)
void Lk_dlock_trace( char *fmt, ... )
#else
static void Lk_dlock_trace(va_alist)
vadcl
#endif
{
    va_list	ap;
    char	locl_buf[LK_DLOCK_MSG_SIZE];

#if defined(__STDARG_H__)
    va_start(ap, fmt);
#else
    char	*fmt;
    va_start(ap);
    fmt = va_arg(ap,char*);
#endif

    if ( Lk_cluster_deadlock_debug == TRACE_BUFFER )
	TRformat_to_buffer( move_to_tbuf, (PTR)1, locl_buf, sizeof(locl_buf), fmt, ap );
    else if ( Lk_cluster_deadlock_debug == TRACE_DIRECT )
    {
	TRformat_to_buffer( TRwrite, (PTR)1, locl_buf, sizeof(locl_buf), fmt, ap );
	TRflush();
    }

    va_end(ap);
}

/*
** Local debugging routine to print out additional info on resource
*/
# define LK_DEADLOCK_MSG( msg, node, lkbid ) \
       LK_DLOCK_TRACE("%@ %d LKdist_deadlock_thread %d:%x %s. %d\n", \
	       __LINE__, node, lkbid, msg, IsPosted )

#else 
#define LK_DEADLOCK_MSG( msg, node, lkbid )
#endif /* LKDEADLOCK_DEBUG */


/*
** Name: LKdist_deadlock_thread	- Perform cluster wide deadlock detection.
**
** Description:
**
**	This routine forms the body of the thread responsible for 
**	continuation of distributed deadlock searches.
**
**	Please see the distributed deadlock addendum to LKdeadlock_thread
**	for an overview of this process.
**
**	This routine consumes LK_DSM messages sent to it from other
**	nodes.  LK_DSM messages are generated by LK_local_deadlock
**	as it traverses the originating node's segment of the deadock
**	graph.
**
**	If a message processed here represents a lock that ultimately
**	originated from the node we are running on, we check if the lock
**	is still waiting.
**
**	If it isn't, there is no deadlock, and we're done.  
**
**	If it is we check to see if the incoming message is for the same
**	resource.  If it is then the original lock is deadlocked.
**
**	Otherwise the deadlock graph is just passing through our local
**	node again for another resource.  Traversing the local section
**	of the deadlock graph for blockers of this resource may in turn
**	generate additonal messages to karoom about the installation.
**
**
** Inputs:
**	dmf_cb			DMF control block (currently unused).
**
** Outputs:
**	none
**
**	Returns:
**	    Typically will not return for the life of the server.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	Summer, 1992 (bryanp)
**	    Working on the new logging and locking system.
**	16-feb-1993 (andys)
**	    Check for LLB_WAITING status before we try to access the
**	    llb_wait field. We try to only set llb_wait when we are waiting
**	    and to simultaneously set LLB_WAITING.
**	24-may-1993 (bryanp)
**	    New arguments to LK_wakeup for mutex optimizations.
**	    Also added lkreq argument to LK_wakeup for shared locklists.
**	21-jun-1993 (bryanp)
**	    Worked on deadlock detection bugs introduced by the 6.5 usage of
**	    fair scheduling lock request algorithms in the cluster.
**	21-aug-1995 (dougb) bug #70696
**	    As a related update to the fix for bug #70696, ensure we return
**	    SS$_INSFMEM when required. The bug fix may increase the fan-out
**	    for a deadlock search (potentially, by a large amount).  Thus,
**	    it is a "bad" idea to have any fixed limits on the number of
**	    output messages.  We must also update msg_cnt to reflect the
**	    number of messages which really should have been sent.
**	    Update the comments above to reflect reality (eg. what status is
**	    actually returned).
**	30-aug-1995 (dougb) bug #70696
**	    Send out messages specifying the maximum request mode of the
**	    waiting request.
**	31-aug-1995 (dougb) bug #70696
**	    Recurse through the local wait-for graph to exhaust it
**	    completely.
**	18-sep-1995 (duursma) bug #71331
**	    Don't recurse to the same RSB for convert requests.
**	19-sep-1995 (dougb) bug #71332
**	    Ensure we don't update the lock database while a server process is
**	    attempting to use it.  In particular, avoid a race condition when
**	    updating lkb->lkreq.status.  Don't want CSP making an update while
**	    a DBMS thread is executing code in LKrequest().  Hold lk_mutex.
**	    Use new interface to found_it() -- may return an error status now.
**	20-sep-1995 (duursma) bug #71334
**	    Split off new walk_wait_for_graph() and reorganized.
**	23-Feb-1996 (jenjo02)
**	    Brought up-to-date semaphore-wise. lkb_mutex has been supplanted
**	    by a host of new and exciting semaphores.
**	30-may-1996 (pchang)
**	    Added LLB as a parameter to the call to LK_allocate_cb() to enable
**	    the reservation of SBK table resource for use by the recovery
**	    process. (B76879)
**	21-aug-1996 (boama01)
**          Replaced "RSB *rsb_hash_table" with "RSH *rsb_hash_table".
**          Replaced "i4 *rsh_table" with "RSH *rsh_table" to prevent
**            access violation in DMFCSP.
**          Replaced (void)LK_mutex/unmutex(&rsb_hash_bucket->rsb_mutex)
**              with (void)LK_mutex/unmutex(&rsb_hash_bucket->rsh_mutex)
**	      in the "Search the RSB hash bucket for a matching key" logic;
**            the rsb_hash_bucket at the moment points to the RSH block.
**	    Improved and supplemented DEADLOCK_DEBUG traces; moved more_info
**	      call to where it makes sense.
**	14-Nov-1996 (jenjo02)
**	    Deadlock() cleanup. Instead of taking all those SHARED mutexes
**	    (RSB, LLB) and checking for rsb_dlock, etc., do the following:
**	      o Whenever a value which affects the wait-for graph is
**	        about to be changed, lock an installation-wide mutes,
**	        lkd_dlock_mutex, make the change, and release the mutex.
**	      o All variables which affect the graph must be mutexed in
**	        this manner.
**		    - the RSB grant and wait queues
**		    - llb_wait (LKB *)
**		    - LLB_WAITING status
**		    - lkb_state
**		    - lkb_request_mode
**	      o The deadlock() code will take an EXCL lock on lkd_dlock_mutex
**		while it does its business. If the protocol is adhered to,
**	        this guarantees that the graph will not change while its
**		being computed, therefore there is no need for deadlock()
**	        to take any lower-level mutexes.
**	16-Dec-1996 (jenjo02)
**	    In found_it(), lock lkd_dlock_mutex before calling LK_resume().
**	19-Mar-1997 (jenjo02)
**	  - Instead of scanning the LLB array for lock lists of interest,
**	    use the queue of LLBs that haven't been deadlock searched. This
**	    queue was created for the interval-based deadlock detection 
**	    thread.
**	  - Changed walk_wait_for_graph() to return LK_DEADLOCK if a deadlock
**	    was detected instead of calling found_it(). Caller is responsible 
**	    for calling found_it(). Caller must also acquire lkd_dlock_mutex
**	    before calling walk_wait_for_graph().
**	14-Jan-1999 (jenjo02)
**	    Recheck LLB_ON_DLOCK_LIST state after waiting for mutex.
**	10-may-2005 (devjo01)
**	    Renamed from LKdeadlock to LKdist_deadlock_thread, made generic and 
**	    moved into RCP as part of sirs 114136 & 114504.
**	    - Removed whole "in_msg == NULL" section.  Routine is only
**	      called to process distributed deadlock continuation messages.
**	      Initial DSMs are created in LKdeadlock_thread.
**	    - Removed parameters related to outputting messages, as this
**	      is now handled by the LK "dsm" functions.
**	    - Use CXdlm_mark_as_deadlocked && LKcp_resume to assure
**	      a deadlock victim is awoken only once, and in the same
**	      context, and with the same code as if the DLM had detected
**	      deadlock itself.
**	12-Feb-2007 (jonj)
**	    To avoid CPU saturation, periodically hiccup-wait to give
**	    other RCP threads (e.g. LKdeadlock_thread) a chance to run.
**	    A real hiccup wait would be nice here; the best we can do 
**	    is a 1-second suspend.
**	    Do NOT cycle through the RSB grant queue, just call LK_local_deadlock
**	    on the first-encountered incompatible lock, which will in turn
**	    cycle through the RSB grant queue and find all that are
**	    incompatible.
**	    Add "must_retry" logic to avoid RSB mutex deadlocks with
**	    the competing LKdeadlock_thread.
**	21-Feb-2007 (jonj)
**	    Actually, we must cycle through the RSB grant/convert queue to
**	    ensure that continuation messages are sent for all incompatible
**	    blocked locks.
**	    Add no-read CXppp_read() before calling LK_flush_dsm_context to
**	    release just-consumed message space and unstall any stalled
**	    readers.
**	    If all grant/convert locks are compatible, we must check for
**	    incompatible locks on the wait queue as they may well be the
**	    blocking lock(s) revealed by CXdlm_get_blki().
**	01-Jun-2007 (jonj)
**	    Implement LK_DLOCK_TRACE macro, check for asynchronously
**	    completed lock requests using CXDLM_IS_POSTED macro.
**	18-Jun-2007 (jonj)
**	    Check that wait_rsb is non-NULL.
**	20-Jun-2007 (jonj)
**	    When cycle completes, make sure that original lock is
**	    the first incompatible grant/convert/wait request on the
**	    resource; o_lkb is not involved in a deadlock if there
**	    are others preceding it.
**	    Fill in in_msg o_blk_pid, sid on first use for error 
**	    message tokens.
**	06-Sep-2007 (jonj)
**	    Yet more iimonitor functions for tracing.
**	    Subvert looking for "waiting" LKBs, letting the more robust
**	    LK_local_deadlock() do that with mutex protection.
**	    When msg received is "back home" to the originating node
**	    on the originating key, check if the resource blocker is
**	    o_lkb - if not, continue the search.
**	    When "must_retry" indicated, instead of immediately suspending,
**	    mark the message as must-retry, continue with other messages,
**	    then reprocess any in a retry state.
**	13-Sep-2007 (kibro01)
**	    Fix compilation failure caused by cdt_xxxx not being inside
**	    correct #define
**	08-Oct-2007 (jonj)
**	    Move cdt_xxxx statics to where all those #if defined(LKDEADLOCK_DEBUG)
**	    won't be needed.
**	    If closure and no incompatible lock request found, discard
**	    message.
**	22-Oct-2007 (jonj)
**	    When "must_retry" returned from LK_local_deadlock(), flag
**	    message with LK_DSM_IN_RETRY so the next time, LK_local_deadlock
**	    will ignore already-set cluster stamps.
**	01-Nov-2007 (jonj)
**	    Needed mutexes may be held by a dead process, so be careful.
**	08-Nov-2007 (jonj)
**	    Replace MAX_DSMS_PER_BUFFER with 8 * (NumNodes-1);
**	11-Jan-2008 (jonj)
**	    o Copy "must_retry" messages to a "RetryBuffer" so we can
**	      release the read-buffer space for reuse and hopefully
**	      reduce the number of CXppp_write timeouts, which occur
**	      when the writers to the buffer catch up to the reader.
**	    o In anticipation of Posix threads in VMS, don't hiccup
**	      wait if OS threads.
**	    o Add lkcdtreset iimonitor function to reset stats.
**	    o Rework what we do to determine if we're really deadlocked
**	      when the message is from the originating node on the
**	      originating key. Specifically, check for preceding
**	      incompatible locks who's lock list is waiting.
**	    o When trolling the grant queue for incompatible locks,
**	      skip those whose lock list is not waiting. LK_local_deadlock
**	      would do this on our behalf, but we can eliminate
**	      them quicker here.
**	    o If LK_local_deadlock returns "must_retry" then there's
**	      something we need done by the local LKdeadlock_thread,
**	      like release a mutex or run local deadlock on some
**	      lock that hasn't been locally checked yet. If we see
**	      that the local thread is asleep, CSresume it.
**	22-feb-2008 (joea)
**	    Ensure the thread responds to interrupts when only one node has
**	    been configured.
**	04-Apr-2008 (jonj)
**	    Add (i4)MaxRequestMode local variable, seed from 
**	    (i2)in_msg->lock_cum_mode before passing to LK_local_deadlock()
**	    which expects a pointer to i4, not i2.
**	30-Apr-2008 (jonj)
**	    Push LKB_END_OF_DL_CYCLE test on different key to later, after
**	    the differing key has been checked for deadlock. On more than 2
**	    nodes, the order in which messages are received may close the
**	    cycle prematurely, leaving a potentially unanalysed message
**	    as a deadlock source.
**	    Add "msg_node" to debug message so we can know the sending
**	    node.
**	    Count a hop on each msg, not just when back to the node of
**	    origin.
**	29-Aug-2008 (jonj)
**	    Relocate LKD_PURGE_DLOCK_LIST to lkd_purge_dlock to lkd_status
**	    corruption.
**	09-Jan-2009 (jonj)
**	    lkb_lkbq LKBQ excised from LKB. Also fix in_msg->o_stamp to 
**	    compare vs lkb_request_id instead of llb_stamp.
**	    Before committing to rsb_mutex, check if the local deadlock
**	    thread holds or wants the mutex; if so, give deference to 
**	    the local thread, then retry the message later.
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Maintain stats by lock type.
*/
DB_STATUS
LKdist_deadlock_thread( DMC_CB *dmc_cb )
{
    i4			max_dsms_per_buffer = 65535 / sizeof(LK_DSM);
    LKD			*lkd = (LKD *)LGK_base.lgk_lkd_ptr;
    LLB			*llb, *o_llb, *dead_llb, *wait_llb;
    LKB			*lkb, *o_lkb, *wait_lkb, *dead_lkb;
    RSB			*rsb, *o_rsb, *wait_rsb, *dead_rsb;
    RSH			*rsb_hash_bucket;
    LLBQ		*llbq;
    u_i4		rsb_hash_value;
    LK_LOCK_KEY		*lock_key, *o_lock_key;
    RSH			*rsh_table;
    SIZE_TYPE		end_offset, lkb_offset;
    SIZE_TYPE		*sbk_table;
    i4			err_code, status;
    LK_DSM		*msgs, *in_msg, *can_msg;
    i4			msgcount, NumNodes, MaxMsgsPerRead;
    char		*param;
    i4			must_retry;
    bool		matching_rsb;
    bool		AllCompat, DeadlockChecked;
    bool		InRetry;
    i4			i, j, RetryMask;
    i4			IsPosted = 0;
    char		keydispbuf[256];
    char		keydispbuf2[256];
    char		keydispbuf3[256];
    LK_DSM		*RetryBuffer;
    u_i4		MsgsWritten;
    bool		OSthreads;
    i4			GrantsConverts;
    i4			MaxRequestMode;
    i4			MaxHops;
    i4			MaxHopsDebug;

    TRdisplay( "%@ Dist Deadlock thread %x started on node %d\n",
		dmc_cb->dmc_session_id, lkd->lkd_csid);

    OSthreads = CS_is_mt();

    /* Get number of configured nodes into NumNodes */
    status = CXcluster_nodes( &NumNodes, NULL );
    if ( status )
    {
	uleFormat(NULL, status, (CL_ERR_DESC *)NULL, ULE_LOG,
		NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 0);
	return E_DB_ERROR;
    }

    if ( NumNodes <= 1 )
    {
	/*
	** Only one node in "cluster" so we can't have continuation 
	** messages.  Suspend thread until shutdown.
	*/
	status = CSsuspend( CS_BIO_MASK | CS_INTERRUPT_MASK, 0, 0 );
	/* When interrupted, it's time to shutdown */
	if (status == E_CS0008_INTERRUPTED)
	    status = E_DB_OK;

	TRdisplay( "%@ Dist Deadlock thread %x terminated on node %d with status %x\n",
		    dmc_cb->dmc_session_id, lkd->lkd_csid, status);

	return (status);
    }

#if defined(LKDEADLOCK_DEBUG)
    /* Register trace functions with iimonitor */
    IICSmon_register( "lkcdtbuffer", lk_cdt_buffer );
    IICSmon_register( "lkcdtdirect", lk_cdt_direct );
    IICSmon_register( "lkcdtdump", lk_cdt_dump );
    IICSmon_register( "lkcdtoff", lk_cdt_off );
    IICSmon_register( "lkcdtcancel", lk_cdt_cancel );
    IICSmon_register( "lkcdtresume", lk_cdt_resume );
    IICSmon_register( "lkcdtstats", lk_cdt_stats );
    IICSmon_register( "lkcdtreset", lk_cdt_reset );

    NMgtAt("II_CLUSTER_DEADLOCK_DEBUG", &param );
    if (param && *param)
    {
	i4	debug_level;

	Lk_cluster_deadlock_debug = TRACE_BUFFER;

	(void)CVal(param, &debug_level);

	if ( debug_level == 2 )
	    debug_cpp = TRUE;
	else if ( debug_level == 0 )
	    Lk_cluster_deadlock_debug = TRACE_OFF;
    }
#endif

    /* Note initial debug state */
    TRdisplay("%@ Cluster deadlock tracing is %d \n",
		Lk_cluster_deadlock_debug);

    /*
    ** Allocate a separate block of LK_DSM messages
    ** for each potential member of the cluster.  
    **
    ** Only required if we cannot depend on DLM to
    ** handle distributed deadlock detection.
    */

    /* Read up to 8 messages per node */
    MaxMsgsPerRead = (NumNodes-1) * 8;

    TRdisplay( "%@ Dist Deadlock thread LK_DSM %d, MaxPerBuffer %d, MaxPerRead %d\n",
		sizeof(LK_DSM), max_dsms_per_buffer, MaxMsgsPerRead );

    /* Allocate a buffer to stow msgs in need of retry */
    RetryBuffer = (LK_DSM*)MEreqmem(0, 
		    MaxMsgsPerRead * sizeof(LK_DSM),
		    0, &status);
    if ( status )
    {
	uleFormat(NULL, status, (CL_ERR_DESC *)NULL, ULE_LOG,
		NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 0);
	return E_DB_ERROR;
    }

    status = CXppp_alloc( &LK_read_ppp_ctx, dsm_svc_name(), 2 * (NumNodes - 1),
                          max_dsms_per_buffer * sizeof(LK_DSM),
			  debug_cpp );
    if ( status == E_DB_OK )
    {
	status = CXppp_listen( LK_read_ppp_ctx );
	if ( status == E_DB_OK )
	    status = LK_alloc_dsm_context( &LK_cont_dsmc );
    }

    /* Just local stats for reporting */
    cdt_reads = 0;
    cdt_max_hops = 0;
    cdt_futile = 0;
    cdt_retries = 0;
    cdt_retry_sleeps = 0;
    cdt_local_wakes = 0;
    cdt_eoc_dead = 0;
    cdt_lg_dead = 0;

    InRetry = FALSE;
    RetryMask = 0;

    /*
    ** Set message hops throttles.
    **
    ** These are purely arbitrary, but ideally would be the
    ** number of points in the cluster-wide matrix in the wait-for
    ** graph of a lock.
    **
    ** Choosing values too low might mean missing a legitimate
    ** deadlock; choosing values too high just wastes messages
    ** and adds to overhead.
    **
    ** Note that when we're doing buffered tracing, the bar
    ** is lowered so that unhelpful messages don't clutter the
    ** trace logs and obliterate those which might be helpful.
    */
    MaxHops = NumNodes * 64;
    MaxHopsDebug = NumNodes * 16;

    while ( status == E_DB_OK )
    {
	/* Read the next batch of msgs, unless we're retrying the last batch */
	if ( !InRetry )
	{
	    /*
	    ** Get a pointer to buffered LK_DSM messages.  We don't ask
	    ** for the maximum possible number of contiguous messages
	    ** available to avoid any one LK_DSM source monopolizing
	    ** this thread for too many iterations.
	    */
	    status = CXppp_read( LK_read_ppp_ctx, sizeof(LK_DSM), 
				 MaxMsgsPerRead,
				 0, 
				 (char **)&msgs,
				 &msgcount );

	    /* When interrupted, it's time to shutdown */
	    if ( status )
	    {
		if ( status != E_CS0008_INTERRUPTED )
		    TRdisplay("%@ %d LKdist_deadlock_thread: CXppp_read error %x "
				"msgcount %d\n", __LINE__, status, msgcount);
		break;
	    }

	    /* Count another read */
	    cdt_reads++;

	    /* Tally total number of msgs received */
	    lkd->lkd_stat.gdlck_call += msgcount;
	}

	/* Loop over all messages received */
	for ( i = 0; i < msgcount; i++ )
	{
	    /* Retrying this batch? */
	    if ( InRetry )
	    {
		/* Find next msg for retry */
		if ( RetryMask & (1 << i) )
		{
		    RetryMask &= ~(1 << i);
		    cdt_retries++;
		    in_msg = &RetryBuffer[i];
		}
		else
		    continue;
	    }
	    else
	    {
		in_msg = &msgs[i];
	    }

	    lock_key = &in_msg->lock_key;
	    o_lock_key = &in_msg->o_lock_key;
	    in_msg->lock_cum_mode;
	    

	    /* For debugging, watch max hops... */
	    if ( in_msg->msg_hops > cdt_max_hops )
		cdt_max_hops = in_msg->msg_hops;


	    LK_DLOCK_TRACE("\n%@ %d LKdist_deadlock_thread: (r%d,m%d,h%d) %d,%d,%d,%d [%d of %d] %s\n\t"
		    "Continue gbl srch to %d lkblks o_node %d (%s) from node %d\n\t"
		    "o_llb=%x o_lkb_id=%x request %d o_dlm %x o_blk_node %d o_blk_lockid %x\n\t"
		    "o_key %s\n\t"
		    "  key %s cdlm %x bdlm %x cum_mode %w state %w\n",
		    __LINE__,
		    cdt_reads, lkd->lkd_stat.gdlck_call, cdt_max_hops,
		    in_msg->msg_search, in_msg->o_node, in_msg->msg_seq, 
		    in_msg->msg_hops,
		    i + 1, msgcount,
		    (InRetry) ? "(R)" : "",
		    lkd->lkd_lbk_count, in_msg->o_node,
		    (in_msg->o_node == lkd->lkd_csid) ? "orig" : "not orig",
		    in_msg->msg_node,
		    in_msg->o_llb_id,
		    in_msg->o_lkb_id,
		    in_msg->o_stamp,
		    in_msg->o_lockid.lk_ulow,
		    in_msg->o_blk_node,
		    in_msg->o_blk_lockid,
		    LKkey_to_string(o_lock_key, keydispbuf), 
		    LKkey_to_string(lock_key, keydispbuf2), 
		    in_msg->lockid.lk_ulow,
		    in_msg->lockid.lk_uhigh,
		    LOCK_MODE, in_msg->lock_cum_mode,
		    LKB_STATE, in_msg->lock_state);

		    
	    /* Check for too many hops. If buffered tracing, lower the bar */
	    if ( in_msg->msg_hops > MaxHops
		 || (Lk_cluster_deadlock_debug == TRACE_BUFFER 
		      && in_msg->msg_hops > MaxHopsDebug) )
	    {
		cdt_futile++;

		LK_DLOCK_TRACE("%@ %d LKdist_deadlock_thread: too many hops %d(%d), "
				"eschewing msg %d,%d,%d,%d\n",
				__LINE__,
				cdt_max_hops, cdt_futile,
				in_msg->msg_search, in_msg->o_node, 
				in_msg->msg_seq, in_msg->msg_hops);
		continue;
	    }

	    /*
	    ** First, if the original transaction is from the current node,
	    ** check to see if the transaction still exists. If not, ignore
	    ** the continue deadlock search message. The original transaction
	    ** may have already finished by the time the obsolete continue
	    ** deadlock search message comes in.
	    */

	    o_lkb = (LKB*)NULL;
	    o_llb = (LLB*)NULL;
	    o_rsb = (RSB*)NULL;
	    rsb = (RSB*)NULL;
	    IsPosted = 0;
	    must_retry = 0;

	    if (in_msg->o_node == lkd->lkd_csid)
	    {
		/*
		** We don't need the deadlock queue  mutex, as it is
		** guaranteed that the LKB is removed from the deadlock
		** queue by LKdeadlock_thread before the distributed
		** search began.
		*/
		if ( in_msg->o_lkb_id && in_msg->o_lkb_id <= lkd->lkd_sbk_count )
		{
		    sbk_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lkd->lkd_sbk_table);
		    o_lkb = (LKB *)LGK_PTR_FROM_OFFSET(sbk_table[in_msg->o_lkb_id]);
		    o_llb = (LLB *)LGK_PTR_FROM_OFFSET(o_lkb->lkb_lkbq.lkbq_llb);
		    o_rsb = (RSB *)LGK_PTR_FROM_OFFSET(o_lkb->lkb_lkbq.lkbq_rsb);

		    if ( o_lkb->lkb_type == LKB_TYPE && 
			 !(IsPosted = CXDLM_IS_POSTED(&o_lkb->lkb_cx_req)) &&
			 o_lkb->lkb_request_id == in_msg->o_stamp &&
			 o_llb && o_llb->llb_type == LLB_TYPE &&
			 o_rsb && o_rsb->rsb_type == RSB_TYPE )
		    {
			/* Defer to local searches */
			if ( o_rsb->rsb_dlock_depth & RSB_MUTEXED_LOCL )
			{
			    RetryMask |= (1 << i);
			    if ( !InRetry )
				MEcopy((char*)in_msg, 
				       sizeof(LK_DSM), 
				       (char*)&RetryBuffer[i]);
			    continue;
			}
			/* Watch for dead mutex holder */
			if ( (LK_mutex(SEM_EXCL, &o_rsb->rsb_mutex)) == OK )
			{
			    /*
			    ** After waiting for the mutex, recheck everything
			    ** to ensure that the lock is still what we think 
			    ** it was.
			    */

			    /*
			    ** NB: In the continuation case, the lock_key in in_msg->lock_key
			    **     is not necessarily the original lock key. Instead, check
			    **     that the original request_id still matches the LKB.
			    */

			    /* Same lock request and transaction? */
			    if ( o_rsb->rsb_type != RSB_TYPE || 
				 o_lkb->lkb_type != LKB_TYPE ||
				 o_llb->llb_type != LLB_TYPE ||
				 o_lkb->lkb_request_id != in_msg->o_stamp ||
				 in_msg->o_llb_id != *(u_i4*)&o_llb->llb_id ||
				 (IsPosted = CXDLM_IS_POSTED(&o_lkb->lkb_cx_req)) )
			    {
				(VOID)LK_unmutex(&o_rsb->rsb_mutex);
				o_rsb = (RSB*)NULL;
			    }
			    else
			    {
				/* Stamp the RSB to let the local thread know */
				o_rsb->rsb_dlock_depth |= RSB_MUTEXED_DIST;
			    }
			}
			else
			    o_rsb = (RSB*)NULL;
		    }
		    else
			o_rsb = (RSB*)NULL;
		}

		/*
		** If anything about the lock has changed, then
		** ignore the message.
		*/
		if ( o_rsb == (RSB*)NULL )
		{
		    LK_DEADLOCK_MSG( "lock is no longer valid",
			in_msg->o_node, in_msg->o_lockid.lk_ulow );
		    continue;
		}

		/*
		** If the original transaction exists, check if the transaction
		** is no longer in the waiting state. The lock could have
		** been granted by the time the deadlock search message came
		** in. If the transaction is still in waiting state, check
		** to see it is waiting on the same lock that required deadlock
		** search.
		**
		** Note that we now hold the rsb_mutex. 
		** This prevents the lkb state from changing.
		*/

		if ((o_lkb->lkb_state != LKB_WAITING
		     && o_lkb->lkb_state != LKB_CONVERT)
		     || o_lkb->lkb_attribute & LKB_DEADLOCKED)
		{
		    if ( o_lkb->lkb_attribute & LKB_DEADLOCKED )
		    {
		      LK_DEADLOCK_MSG( "orig lock is deadlocked",
			in_msg->o_node, in_msg->o_lockid.lk_ulow );
		    }
		    else
		    {
		      LK_DEADLOCK_MSG( "orig lock no longer blocked",
			in_msg->o_node, in_msg->o_lockid.lk_ulow );
		    }

		    o_rsb->rsb_dlock_depth &= ~RSB_MUTEXED_DIST;
		    (VOID)LK_unmutex(&o_rsb->rsb_mutex);
		    continue;
		}

		/* Now check for continuation on a different lock key */

		if ( o_rsb->rsb_name.lk_type == lock_key->lk_type &&
		     o_rsb->rsb_name.lk_key1 == lock_key->lk_key1 &&
		     o_rsb->rsb_name.lk_key2 == lock_key->lk_key2 &&
		     o_rsb->rsb_name.lk_key3 == lock_key->lk_key3 &&
		     o_rsb->rsb_name.lk_key4 == lock_key->lk_key4 &&
		     o_rsb->rsb_name.lk_key5 == lock_key->lk_key5 &&
		     o_rsb->rsb_name.lk_key6 == lock_key->lk_key6)
		{

		    /* End of cycle, we may be deadlocked. */

		    lkb = (LKB*)NULL;
		    GrantsConverts = 0;

		    /*
		    ** Check the grant/convert queue, stopping
		    ** when we find an incompatible request that's waiting,
		    ** or ourselves.
		    **
		    ** If we find another, then we are not involved
		    ** in a deadlock.
		    */
		    end_offset = LGK_OFFSET_FROM_PTR(&o_rsb->rsb_grant_q_next);
		    lkb_offset = o_rsb->rsb_grant_q_next;

		    /* NB: the grant/convert queue may be empty */
		    while ( lkb_offset != end_offset )
		    {
			GrantsConverts++;

			lkb = (LKB*)LGK_PTR_FROM_OFFSET(lkb_offset);
			llb = (LLB*)LGK_PTR_FROM_OFFSET(lkb->lkb_lkbq.lkbq_llb);
			lkb_offset = lkb->lkb_q_next;

			if ( lkb == o_lkb ||
			     (!(lkb->lkb_attribute & LKB_DEADLOCKED)
				 && ((LK_compatible[lkb->lkb_request_mode] >>
					in_msg->lock_cum_mode) & 1) == 0) )
			{
			    /* Show incompat locks up to and including us */
			    LK_DLOCK_TRACE("\tllb %x lkb %x state %w attr %x mode (%w,%w) dlm %x"
					    " wt %d\n\t"
					    "\tcstamp %#.4{%d %}\n",
				*(u_i4*)&llb->llb_id,
				lkb->lkb_id,
				LKB_STATE, lkb->lkb_state, 
				lkb->lkb_attribute,
				LOCK_MODE, lkb->lkb_grant_mode,
				LOCK_MODE, lkb->lkb_request_mode,
				lkb->lkb_cx_req.cx_lock_id,
				llb->llb_waiters,
				LLB_MAX_STAMPS_PER_NODE, &llb->llb_dist_stamp[in_msg->o_node-1], 0);

			    /*
			    ** Stop on the first that's also waiting 
			    ** and hasn't been cluster-searched
			    */
			    if ( llb->llb_waiters && lkb != o_lkb )
			    {
				for ( j = 0; j < LLB_MAX_STAMPS_PER_NODE; j++ )
				{
				    if ( llb->llb_dist_stamp[in_msg->o_node-1][j] ==
						in_msg->msg_search )
				    break;
				}

				/* If not cluster-stamped, take this LKB */
				if ( j == LLB_MAX_STAMPS_PER_NODE )
				    break;
			    }
			}
			lkb = (LKB*)NULL;
		    }

		    /* 
		    ** If no incompat and waiting gr/cv ahead of o_lkb
		    ** and the remote blocked lock is a waiter, then we are
		    ** a waiter in front of it, i.e., this is the special
		    ** case described in LK_local_deadlock in which the
		    ** blocking lock is the original waiter.
		    **
		    ** In other words, the blocked remote lock is behind
		    ** us in the wait queue and can't be granted until
		    ** we are granted, so we're deadlocked.
		    */
		    if ( !lkb && in_msg->o_lockid.lk_ulow == in_msg->lockid.lk_uhigh )
			lkb = o_lkb;


		    /*
		    ** If the first waiting incompatible lock is us,
		    ** then we are are the one blocking the remote
		    ** node and we are deadlocked.
		    **
		    ** If no waiting incompat lock, then the blocking lock
		    ** that the remote node perceived as being here has
		    ** been released or otherwise resolved, so just
		    ** discard this message and move on.
		    **
		    ** Note that o_lkb may be compatible with the cumulative
		    ** mode (IS::SIX, for example), in which case it's not
		    ** involved in a deadlock and we just move on; we're
		    ** not concerned with locks behind us.
		    */

		    if ( !lkb || lkb == o_lkb )
		    {
			/*
			** If it's us and we're incompatible, then
			** we are deadlocked.
			*/
			if ( lkb == o_lkb
			      && ((LK_compatible[o_lkb->lkb_request_mode] >>
					in_msg->lock_cum_mode) & 1) == 0
			      && CXdlm_mark_as_posted(&o_lkb->lkb_cx_req) )
			{
			    o_lkb->lkb_cx_req.cx_status = 
					E_CL2C21_CX_E_DLM_DEADLOCK;

			    cdt_eoc_dead++;

			    LK_DLOCK_TRACE("%@ %d LKdist_deadlock_thread %d: msg (%d,%d,%d) is EOC deadlocked\n\t"
				"llb %x lkb %x state %w attr %x mode (%w,%w) dlm %x GC %d\n\t"
				"\t%s\n",
				__LINE__,
				in_msg->o_node,
				in_msg->msg_search, in_msg->o_node, in_msg->msg_seq,
				*(u_i4*)&o_llb->llb_id,
				o_lkb->lkb_id,
				LKB_STATE, o_lkb->lkb_state, 
				o_lkb->lkb_attribute,
				LOCK_MODE, o_lkb->lkb_grant_mode,
				LOCK_MODE, o_lkb->lkb_request_mode,
				o_lkb->lkb_cx_req.cx_lock_id,
				GrantsConverts,
				LKkey_to_string(&o_rsb->rsb_name, keydispbuf));


			    /* Mark the LKB as deadlocked while RSB mutex is held */
			    o_lkb->lkb_attribute |= LKB_DEADLOCKED;
				    
			    /* Save information about blocker */
			    o_lkb->lkb_blocking_cpid.pid = in_msg->o_blk_pid;
			    o_lkb->lkb_blocking_cpid.sid = in_msg->o_blk_sid;
			    o_lkb->lkb_blocking_mode = in_msg->o_blk_gtmode;

			    /* Deadlocks in the RCP should never happen */
			    if ( o_lkb->lkb_cpid.pid == LGK_my_pid )
			    {
				TRdisplay("%@ %d LKdist_deadlock_thread: unexpected RCP deadlock\n\t"
					"%s\n",
					__LINE__,
					LKkey_to_string(&o_rsb->rsb_name, keydispbuf));
				/* If buffer tracing, dump what we have */
				if ( Lk_cluster_deadlock_debug == TRACE_BUFFER )
				{
				    lk_cdt_dump( NULL, NULL );

				    TRdisplay("%@ %d LKdist_deadlock_thread: suspending...\n",
					    __LINE__);
				    while ( !cdt_resume )
					CSsuspend(CS_TIMEOUT_MASK, 30, 0);
				    cdt_resume = FALSE;
				}
			    }

			    /* Count another global deadlock */
			    lkd->lkd_stat.gdeadlock++;

			    /* Resume the unfortunate */
			    LKcp_resume(&o_lkb->lkb_cpid);
			}
			else
			{
			    /*
			    ** This o_lkb is not deadlocked. If it's the
			    ** message that originated at depth==1, set
			    ** LKB_END_OF_DL_CYCLE to signal any related
			    ** msgs still in the pipeline that they can stop now.
			    */
			    if ( in_msg->msg_seq == 1 )
				o_lkb->lkb_attribute |= LKB_END_OF_DL_CYCLE;

			    LK_DLOCK_TRACE("%@ %d LKdist_deadlock_thread %d: msg (%d,%d,%d) is not deadlocked\n\t"
				"llb %x lkb %x state %w attr %x mode (%w,%w) dlm %x GC %d\n\t"
				"\t%s\n",
				__LINE__,
				in_msg->o_node,
				in_msg->msg_search, in_msg->o_node, in_msg->msg_seq,
				*(u_i4*)&o_llb->llb_id,
				o_lkb->lkb_id,
				LKB_STATE, o_lkb->lkb_state, 
				o_lkb->lkb_attribute,
				LOCK_MODE, o_lkb->lkb_grant_mode,
				LOCK_MODE, o_lkb->lkb_request_mode,
				o_lkb->lkb_cx_req.cx_lock_id,
				GrantsConverts,
				LKkey_to_string(&o_rsb->rsb_name, keydispbuf));
			}

			o_rsb->rsb_dlock_depth &= ~RSB_MUTEXED_DIST;
			LK_unmutex(&o_rsb->rsb_mutex);
			continue;
		    }

		    /* 
		    ** o_lkb is not the blocker, "lkb" is.
		    **
		    ** It's likely that when the original LOCL search
		    ** was done that o_lkb was locally compatible
		    ** with these locks, but they are cluster-incompatible.
		    ** (IS::IX locally compat, but IX::SIX is not 
		    **  cluster compatible).
		    */

		    LK_DEADLOCK_MSG( "orig lock is not the blocker",
			    in_msg->o_node, in_msg->o_lockid.lk_ulow );

		    /*
		    ** There may be Retry msgs preceding this one for
		    ** the same top_lkb that we've decided is not
		    ** the blocker. Find any and un-Retry them.
		    */
		    if ( RetryMask )
		    {
			for ( j = 0; 
			      j < i && RetryMask & (1 << j);
			      j++ )
			{
			    can_msg = &RetryBuffer[j];

			    if ( can_msg->o_node == in_msg->o_node &&
				 can_msg->msg_search == in_msg->msg_search )
			    {
				RetryMask &= ~(1 << j);

				LK_DLOCK_TRACE("\tUnretrying %d msg %d,%d,%d,%d\n",
					    j,
					    can_msg->msg_search,
					    can_msg->o_node,
					    can_msg->msg_seq,
					    can_msg->msg_hops);
			    }
			}
		    }

		    /* Just move on to next message */
		    o_rsb->rsb_dlock_depth &= ~RSB_MUTEXED_DIST;
		    LK_unmutex(&o_rsb->rsb_mutex);
		    continue;
		}

		/* Continuation on a different key... */

		/*
		** Original lkb is not directly deadlocked, but we may
		** still deadlock if there are transactions blocked on
		** the resource in in_msg that are blocked on this node.
		*/
	    }

	    /*
	    ** Original transaction is from a different node or
	    ** key may or may not be for the originating LKB.
	    */

	    /*
	    ** Search the RSB hash bucket for a matching continuation key.
	    */
	    rsh_table = (RSH *) LGK_PTR_FROM_OFFSET(lkd->lkd_rsh_table);

	    rsb_hash_value = (lock_key->lk_type + lock_key->lk_key1 +
		lock_key->lk_key2 + lock_key->lk_key3 + lock_key->lk_key4 +
		lock_key->lk_key5 + lock_key->lk_key6);

	    rsb_hash_bucket = &rsh_table
		[(unsigned)rsb_hash_value % lkd->lkd_rsh_size];


	    matching_rsb = FALSE;

	    if ( (LK_mutex(SEM_EXCL, &rsb_hash_bucket->rsh_mutex)) == OK )
	    {
		for ( rsb = (RSB *) rsb_hash_bucket; rsb->rsb_q_next; )
		{
		    rsb = (RSB *)LGK_PTR_FROM_OFFSET(rsb->rsb_q_next);

		    if (rsb->rsb_name.lk_type == lock_key->lk_type &&
			rsb->rsb_name.lk_key1 == lock_key->lk_key1 &&
			rsb->rsb_name.lk_key2 == lock_key->lk_key2 &&
			rsb->rsb_name.lk_key3 == lock_key->lk_key3 &&
			rsb->rsb_name.lk_key4 == lock_key->lk_key4 &&
			rsb->rsb_name.lk_key5 == lock_key->lk_key5 &&
			rsb->rsb_name.lk_key6 == lock_key->lk_key6)
		    {
			/*
			** Unlock the hash queue, lock the RSB, recheck.
			*/
			(VOID)LK_unmutex(&rsb_hash_bucket->rsh_mutex);

			/* Give deference to local thread */
			if ( rsb->rsb_dlock_depth & RSB_MUTEXED_LOCL )
			{
			    must_retry |= LKMR_MUTEX_CONFLICT;
			    break;
			}

			if ( (LK_mutex(SEM_EXCL, &rsb->rsb_mutex)) == OK )
			{
			    if (rsb->rsb_name.lk_type == lock_key->lk_type &&
				rsb->rsb_name.lk_key1 == lock_key->lk_key1 &&
				rsb->rsb_name.lk_key2 == lock_key->lk_key2 &&
				rsb->rsb_name.lk_key3 == lock_key->lk_key3 &&
				rsb->rsb_name.lk_key4 == lock_key->lk_key4 &&
				rsb->rsb_name.lk_key5 == lock_key->lk_key5 &&
				rsb->rsb_name.lk_key6 == lock_key->lk_key6)
			    {
				/*
				** Tell LK_local_deadlock the LKdist_deadlock_thread 
				** holds the mutex.
				*/
				rsb->rsb_dlock_depth |= RSB_MUTEXED_DIST;
				matching_rsb = TRUE;
				break;
			    }
			    (VOID)LK_unmutex(&rsb->rsb_mutex);
			    (VOID)LK_mutex(SEM_EXCL, &rsb_hash_bucket->rsh_mutex);
			    rsb = (RSB *)rsb_hash_bucket;
			}
			else
			    break;
		    }
		} /* for (rsb = (RSB *) rsb_hash_bucket; */
	    }


	    if ( must_retry & LKMR_MUTEX_CONFLICT )
	    {
		/* Mutex conflict with rsb/o_rsb, retry msg */
		if ( o_rsb )
		{
		    o_rsb->rsb_dlock_depth &= ~RSB_MUTEXED_DIST;
		    (VOID)LK_unmutex(&o_rsb->rsb_mutex);
		}
		RetryMask |= (1 << i);
		if ( !InRetry )
		    MEcopy((char*)in_msg, 
			   sizeof(LK_DSM), 
			   (char*)&RetryBuffer[i]);
			    
		continue;
	    }

	    if ( !matching_rsb )
	    {
		/* Continuation resource not found */
		(VOID)LK_unmutex(&rsb_hash_bucket->rsh_mutex);

		LK_DEADLOCK_MSG( "Continuation resource not found",
			    in_msg->o_node, in_msg->o_lockid.lk_ulow );
		    
		if ( o_rsb )
		{
		    o_rsb->rsb_dlock_depth &= ~RSB_MUTEXED_DIST;
		    (VOID)LK_unmutex(&o_rsb->rsb_mutex);
		}

		/* Abandon this message */
		continue;
	    }

	    /*
	    ** RSB was found, and its mutex is locked.
	    */

	    /*
	    ** Prime the pump by looking for LKBs that are blocked, and
	    ** incompatible with passed lock mode.
	    **
	    ** If there are no blocked LLB's on this resource then there
	    ** is no deadlock.
	    */ 

	    /* For each msg received, increment hops */
	    if ( !InRetry )
		in_msg->msg_hops++;

	    /* Perform continue deadlock search operation. */

	    /* Placehold outgoing messages JIC we find deadlock here */
	    LK_mark_dsm_context( LK_cont_dsmc );

	    /* 
	    ** Traverse grant queue, and for all granted or converting locks
	    ** which are incompatible with the cumulative lock mode, and are
	    ** on waiting LLB's, start a local deadlock search.
	    */
	    end_offset = LGK_OFFSET_FROM_PTR(&rsb->rsb_grant_q_next);
	    lkb_offset = rsb->rsb_grant_q_next;

	    GrantsConverts = 0;

	    /*
	    ** If all grant/convert locks are compatible with cumulative lock
	    ** mode, then check for incompatible waiters. CXdlm_get_blki()
	    ** does this and we may have been sent this msg because a waiter
	    ** on this resource is ahead of and blocking a waiter on another
	    ** node, yet all grant/convert locks on this node may be compatible
	    ** or the grant/convert queue may be empty.
	    */

	    for ( AllCompat = TRUE, DeadlockChecked = FALSE; !must_retry; )
	    {
		while ( lkb_offset != end_offset && !must_retry )
		{
		    lkb = (LKB *)LGK_PTR_FROM_OFFSET(lkb_offset);
		    lkb_offset = lkb->lkb_q_next;

		    /*
		    ** We may encounter LKBs which were DEADLOCKED during a previous
		    ** deadlock() cycle but which haven't had time to be cancelled
		    ** yet. If we do, skip over them.
		    */
		    if (lkb->lkb_attribute & LKB_DEADLOCKED)
		    {
			LK_DEADLOCK_MSG( "Skipping LKB_DEADLOCKED",
					in_msg->o_node, lkb->lkb_cx_req.cx_lock_id );
			continue;
		    }

		    lkd->lkd_stat.dlock_locks_examined++;

		    GrantsConverts++;

		    /*
		    ** If this lock's request mode is incompatible with the
		    ** max request mode of the cluster waiters then this lock 
		    ** request is blocking the resource and may be causing deadlock.
		    **
		    ** Note that unlike a local deadlock check that looks at granted
		    ** mode, we check compatibility vs request mode. Incompatible
		    ** requests on this node are blocking a lock on another node
		    ** and will be granted FIFO, leaving the other node's lock
		    ** still blocked by the newly granted mode.
		    */

		    if ( ((LK_compatible[lkb->lkb_request_mode] >>
				    in_msg->lock_cum_mode) & 1) == 0 )
		    {
			AllCompat = FALSE;

			/*
			** If first incompatible grant/convert and we are the
			** node blocking the initial request, then set
			** "o_blk_lockid" to this lock, replacing the one
			** suggested by getblki in LK_local_deadlock() for
			** loopback awareness.
			*/
			if ( GrantsConverts == 1
			     && in_msg->o_blk_node == lkd->lkd_csid )
			{
			     in_msg->o_blk_lockid = lkb->lkb_cx_req.cx_lock_id;
			}

			llb = (LLB *)LGK_PTR_FROM_OFFSET(lkb->lkb_lkbq.lkbq_llb);

			/* Skip over incompats that are not waiting */
			if ( llb->llb_waiters == 0 )
			    continue;

			DeadlockChecked = TRUE;

			/* If this waiter is top, then we are deadlocked */
			if ( llb == o_llb )
			{
			    dead_lkb = o_lkb;
			}
			else if ( o_lkb && o_lkb->lkb_attribute & LKB_END_OF_DL_CYCLE )
			{
			    /*
			    ** If original LKB's deadlock cycle has completed,
			    ** then we're not deadlocked.
			    */
			    LK_DEADLOCK_MSG(" end of cycle, discarding",
				in_msg->o_node, in_msg->o_lkb_id );
			    /* Check all incompat waiting grants */
			    continue;
			}
			else
			{
			    /* ...otherwise let LK_local_deadlock() figure it out */
			    LK_DLOCK_TRACE("%@ %d LKdist_deadlock_thread: Searching local graph\n\t"
				"o_llb %x o_lkb %x o_dlm %x o_state %w o_attr %x o_cum_mode %w post %d\n\t"
				"\t%s\n\t"
				"llb %x lkb %x state %w attr %x mode (%w,%w) dlm %x post %d\n\t"
				"\t%s\n\t"
				"cstamp %#.4{%d %}\n",
				__LINE__,
				in_msg->o_llb_id,
				in_msg->o_lkb_id,
				in_msg->o_lockid.lk_ulow,
				LKB_STATE, o_lkb ? o_lkb->lkb_state : in_msg->lock_state, 
				(o_lkb) ? o_lkb->lkb_attribute: 0,
				LOCK_MODE, in_msg->lock_cum_mode,
				(o_lkb) ? CXDLM_IS_POSTED(&o_lkb->lkb_cx_req) : 0,
				o_lkb ? LKkey_to_string(&o_rsb->rsb_name, keydispbuf) 
				      : "(not orig)",
				*(u_i4*)&llb->llb_id,
				lkb->lkb_id, 
				LKB_STATE, lkb->lkb_state, 
				lkb->lkb_attribute,
				LOCK_MODE, lkb->lkb_grant_mode,
				LOCK_MODE, lkb->lkb_request_mode,
				lkb->lkb_cx_req.cx_lock_id,
				CXDLM_IS_POSTED(&lkb->lkb_cx_req),
				LKkey_to_string(&rsb->rsb_name, keydispbuf2),
				LLB_MAX_STAMPS_PER_NODE, &llb->llb_dist_stamp[in_msg->o_node-1], 0);

			    /* If first time, note blocker info in in_msg */
			    if ( in_msg->o_blk_pid == 0 )
			    {
				in_msg->o_blk_pid = lkb->lkb_cpid.pid;
				in_msg->o_blk_sid = lkb->lkb_cpid.sid;
			    }

			    /*
			    ** Run the incompatible lock(s) through the mill and
			    ** see what pops out.
			    **
			    ** LK_local_deadlock will start at this first incompat
			    ** LKB and wend its way through the end of the grant/cvt
			    ** queue, checking each incompat/waiting for deadlock.
			    **
			    ** If no deadlock, it'll then check for remote incompat
			    ** locks in front of the last locally incompat LKB 
			    ** and send continuation messages to those incompat nodes.
			    */
			    MaxRequestMode = in_msg->lock_cum_mode;

			    dead_lkb = LK_local_deadlock( lkb, o_lkb,
				       &MaxRequestMode,
				       in_msg, 
				       LK_cont_dsmc,
				       0, &must_retry );
			}

			if ( dead_lkb )
			{
			    /* Cancel all retry indicators */
			    must_retry = 0;

			    /* Lock is deadlocked, post and resume it */
			    if (CXdlm_mark_as_posted(&dead_lkb->lkb_cx_req))
			    {
				cdt_lg_dead++;

				dead_lkb->lkb_cx_req.cx_status =
				     E_CL2C21_CX_E_DLM_DEADLOCK;
						   
				/* Save information about blocker */
				dead_lkb->lkb_blocking_cpid.pid =
				     in_msg->o_blk_pid;
				dead_lkb->lkb_blocking_cpid.sid =
				     in_msg->o_blk_sid;
				dead_lkb->lkb_blocking_mode =
				     in_msg->o_blk_gtmode;

				dead_llb = (LLB*)LGK_PTR_FROM_OFFSET(dead_lkb->lkb_lkbq.lkbq_llb);
				dead_rsb = (RSB*)LGK_PTR_FROM_OFFSET(dead_lkb->lkb_lkbq.lkbq_rsb);

				/* Toss accumulated DSM's for this resource */
				LK_unmark_dsm_context( LK_cont_dsmc );

				LK_DLOCK_TRACE("%@ %d LKdist_deadlock_thread %d: msg (%d,%d,%d) is deadlocked\n\t"
					"o_llb %x o_lkb %x o_dlm %x o_state %w o_attr %x\n\t"
					"dead_llb %x dead_lkb %x dead_state %w dead_attr %x dead_mode (%w,%w) dead_dlm %x\n\t"
					"\t%s\n",
					__LINE__,
					in_msg->o_node,
					in_msg->msg_search, in_msg->o_node, in_msg->msg_seq,
					o_llb ? *(u_i4*)&o_llb->llb_id : 0,
					o_lkb ? o_lkb->lkb_id : 0,
					in_msg->o_lockid.lk_ulow,
					LKB_STATE, in_msg->lock_state, 
					(o_lkb) ? o_lkb->lkb_attribute : 0,
					*(u_i4*)&dead_llb->llb_id,
					dead_lkb->lkb_id, 
					LKB_STATE, dead_lkb->lkb_state, 
					dead_lkb->lkb_attribute,
					LOCK_MODE, dead_lkb->lkb_grant_mode,
					LOCK_MODE, dead_lkb->lkb_request_mode,
					dead_lkb->lkb_cx_req.cx_lock_id,
					LKkey_to_string(&dead_rsb->rsb_name, keydispbuf));

				/* Deadlocks in the RCP are unexpected */
				if ( dead_lkb->lkb_cpid.pid == LGK_my_pid )
				{
				    TRdisplay("%@ %d LKdist_deadlock_thread: unexpected RCP deadlock\n\t"
					"%s\n",
					__LINE__,
					LKkey_to_string(&dead_rsb->rsb_name, keydispbuf));

				    if ( Lk_cluster_deadlock_debug == TRACE_BUFFER )
				    {
					lk_cdt_dump( NULL, NULL );

					TRdisplay("%@ %d LKdist_deadlock_thread: suspending...\n",
						__LINE__);
					while ( !cdt_resume )
					    CSsuspend(CS_TIMEOUT_MASK, 30, 0);
					cdt_resume = FALSE;
				    }
				}

				/*
				** LK_local_deadlock has counted a global deadlock,
				** but only if deadlock was detected there, not
				** locally.
				*/
				if ( dead_lkb == o_lkb )
				    lkd->lkd_stat.gdeadlock++;

				/* Resume the unfortunate */
				LKcp_resume(&dead_lkb->lkb_cpid);
			    }
			    else
			    {
				/* We were not really deadlocked */
				dead_lkb->lkb_attribute &= ~LKB_DEADLOCKED;
				/* Uncount global deadlock */
				if ( dead_lkb != o_lkb )
				    lkd->lkd_stat.gdeadlock--;
			    }
			} /* If dead_lkb */
			/* LK_local_deadlock has checked 'em all */
			break;
		    } /* if ( ((LK_compatible[lkb->lkb_request_mode]  */
		} /* while ( lkb_offset != end_offset && !must_retry ) */

		/*
		** End of queue.
		**
		** If all that we looked at are compatible with cumulative mode
		** or we found incompat's on the grant queue but none were
		** waiting and and we haven't checked the wait queue, then cycle
		** back to do the waiters.
		**
		** But don't check waiters if the lock blocked on
		** the remote node was a converter; converters
		** are queued ahead of waiters and can't possibly
		** be blocked by them.
		*/
		LK_DLOCK_TRACE("%@ %d LKdist_deadlock_thread: DC %d AC %d GC %d\n",
				__LINE__,
				DeadlockChecked,
				AllCompat,
				GrantsConverts);

		if ( !DeadlockChecked
		     && in_msg->lock_state == LKB_WAITING
		     && end_offset == LGK_OFFSET_FROM_PTR(&rsb->rsb_grant_q_next)
		     && rsb->rsb_waiters )
		{

		    /*
		    ** If the grant convert queue is empty
		    ** then pick the first incompat waiter, if any.
		    */
		    if ( !GrantsConverts )
		    {
			LK_DEADLOCK_MSG( "checking waiters",
				    in_msg->o_node, in_msg->o_lockid.lk_ulow );

			end_offset = LGK_OFFSET_FROM_PTR(&rsb->rsb_wait_q_next);
			lkb_offset = rsb->rsb_wait_q_next;
			continue;
		    }

		    LK_DEADLOCK_MSG( "not checking waiters",
				    in_msg->o_node, in_msg->o_lockid.lk_ulow );
		}

		break;
	    } /* for ( AllCompat = TRUE, DeadlockChecked = FALSE; !must_retry; ) */

	    /* Release the resource block(s) */
	    rsb->rsb_dlock_depth &= ~RSB_MUTEXED_DIST;
	    LK_unmutex(&rsb->rsb_mutex);
	    if ( o_rsb && o_rsb != rsb )
	    {
		o_rsb->rsb_dlock_depth &= ~RSB_MUTEXED_DIST;
		LK_unmutex(&o_rsb->rsb_mutex);
	    }

	    if ( must_retry )
	    {
		/*
		** LKMR_TOP_POSTED indicates the top_lkb was async posted
		** while in the depths of deadlock checking, in which
		** case none of the other indicators matter and we
		** can toss any accumulated DSMs for this resource.
		*/

		if ( must_retry & LKMR_TOP_POSTED )
		    LK_unmark_dsm_context( LK_cont_dsmc );
		else if ( must_retry & LKMR_MUTEX_CONFLICT )
		{
		    /* 
		    ** LKMR_MUTEX_CONFLICT
		    **
		    ** Retain any DSMs which may have been
		    ** accumulated for this resource,
		    ** queue the msg for retry, move
		    ** on to next input msg.
		    */
		    RetryMask |= (1 << i);

		    /* Copy this msg to RetryBuffer */
		    if ( !InRetry )
			MEcopy((char*)in_msg, 
			       sizeof(LK_DSM), 
			       (char*)&RetryBuffer[i]);
		}
		/* Ignore other must_retry possibilities */
	    }
	    else if ( !DeadlockChecked )
	    {
		if ( AllCompat )
		{
		    LK_DEADLOCK_MSG( "no incompatible locks",
			    in_msg->o_node, in_msg->o_lockid.lk_ulow );
		}
		else
		{
		    LK_DEADLOCK_MSG( "no waiting incompatible locks",
			    in_msg->o_node, in_msg->o_lockid.lk_ulow );
		}
	    }

	    /* Move on to next message */
	} /* for ( i = 0; i < msgcount; i++ ) */

	/* End of this group of messages.. */

	if ( !InRetry )
	{
	    /*
	    ** Unless we're dealing with the Retry buffer,
	    ** issue a no-read read to drain the swamp and
	    ** release the message buffer space for reuse
	    */
	    i4		DummyCount;

	    (VOID) CXppp_read( LK_read_ppp_ctx, sizeof(LK_DSM), 
				 0, 0, (char **)&in_msg,
				 &DummyCount );
	}

	/* Send accumulated continuation messages, if any */

	MsgsWritten = 0;

	LK_flush_dsm_context( &msgs[0], LK_cont_dsmc, &MsgsWritten );

	lkd->lkd_stat.cnt_gdlck_sent += MsgsWritten;

	/*
	** In the case of "retries" some messages
	** need the local thread to do something 
	** (release a mutex) before the messages
	** can be retried. 
	** Hiccup to give the local LKdeadlock thread 
	** an opportunity to do some work.
	*/
	if ( RetryMask )
	{
	    /* If OS threads, don't sleep */
	    if ( !OSthreads )
	    {
		cdt_retry_sleeps++;

		LK_DLOCK_TRACE("%@ %d LKdist_deadlock_thread: "
		    "Sleeping for retry of %x, retries %d sleeps %d writ %d\n",
			    __LINE__,
			    RetryMask, cdt_retries, cdt_retry_sleeps,
			    MsgsWritten);

		/*
		** I really don't want to suspend for a full 
		** second here, just hiccup to let other threads
		** run if they're computable. Having sub-second 
		** timeouts would be a boost...
		**
		** With OS-threading, this should not be needed...
		*/
		status = CSsuspend(CS_INTERRUPT_MASK | CS_TIMEOUT_MASK, 1, 0);

		if ( status == E_CS0009_TIMEOUT )
		    status = OK;
	    }

	    InRetry = TRUE;
	}
	else
	{
	    InRetry = FALSE;

	    /*
	    ** If local deadlock checks needed and 
	    ** local thread is sleeping, wake it up 
	    */
	    if ( lkd->lkd_purge_dlock )
	    {
		lkd->lkd_purge_dlock = FALSE;

		if ( CS_TAS(&lkd->lkd_ldead_running) )
		{
		    cdt_local_wakes++;
		    CSresume(lkd->lkd_ldead_sid);
		}
	    }
	}

    } /* End outer for loop */

    /* When interrupted, it's time to shutdown */
    if (status == E_CS0008_INTERRUPTED)
	status = E_DB_OK;

    /* Other status values are unexpected. */
    if ( status )
    {
	uleFormat(NULL, status, (CL_ERR_DESC *)NULL, ULE_LOG,
		    NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 0);
	status = E_DB_FATAL;
    }

    TRdisplay( "%@ Dist Deadlock thread %x terminated on node %d with status %x\n",
		dmc_cb->dmc_session_id, lkd->lkd_csid, status);

    return status;
}


/* 
** Name: dsm_svc_name - return "service" name used by DSM communications.
**
** Description:
**
**	Generates service name (once) & returns pointer to it (always).
**
** Inputs:
**	None
**
** Outputs:
**	None
**
** Returns:
**	Pointer to cached service name.
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
**	17-May-2005 (devjo01)
**	    Created.
**
*/
static char *dsm_svc_name(void)
{
    static	char	service_name[10] = { '\0', };
    char		*instid;

    if ( '\0' == service_name[0] )
    {
	STcopy("II_CSP", service_name );
	NMgtAt("II_INSTALLATION", &instid );
	if ( instid && instid[0] && instid[1] )
	{
	    service_name[6] = '_';
	    service_name[7] = instid[0];
	    service_name[8] = instid[1];
	    service_name[9] = '\0';
	}
    }
    return service_name;
}

/*
** 
** Name: LK_dsm_context_priv_init - Initialization function for
**       LK_DSM_CONTEXT_PRIV.
**
** Description:
**
**	Initializes LK_DSM_CONTEXT_PRIV.
**
** Inputs:
**	lk_dsm_priv	- Structure to initialize.
**
** Outputs:
**	None
**
** Returns:
**	None
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
**	13-May-2005 (fanch01)
**	    Created.
**
*/
static void
LK_dsm_context_priv_init( LK_DSM_CONTEXT_PRIV *lk_dsm_priv )
{
    lk_dsm_priv->lk_dsm_ctx_head = 0;
    lk_dsm_priv->lk_dsm_ctx_tail = 0;
    lk_dsm_priv->lk_dsm_ctx_mark = 0;
    lk_dsm_priv->lk_dsm_ctx_handle_alloc = 0;
    lk_dsm_priv->lk_dsm_ctx_active = 0;
    lk_dsm_priv->lk_dsm_ctx_handle = NULL;
}

/*
** 
** Name: LK_alloc_dsm_context - Allocate a DSM context.
**
** Description:
**
**	Allocates all resources associated with an LK_DSM_CONTEXT.  Does NOT
**	open communication channels.  Buffers are allocated for all configured
**	nodes, except if writeonly set when buffers are not allocated for the
**	local node.
**	
** Inputs:
**	dsmc		- DSM context to allocate and initialize.
**
** Outputs:
**	None
**
** Returns:
**	OK on success,
**	other value indicates error
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
**	13-May-2005 (fanch01)
**	    Created.
**	12-Feb-2007 (jonj)
**	    Add message sequencing lk_dsm_ctx_msg_seq to aid debugging.
**
*/
STATUS
LK_alloc_dsm_context( LK_DSM_CONTEXT **hdsmc )
{
    STATUS status = -1 /* FIX-ME */;
    CX_NODE_INFO *node_info;
    LK_DSM_CONTEXT *dsmc;
    LK_DSM_CONTEXT_PRIV *lk_dsm_ctx_priv;
    i4 node;

    do
    {
	dsmc = (LK_DSM_CONTEXT *)MEreqmem(0, sizeof(LK_DSM_CONTEXT),
			 TRUE, &status);
	if ( NULL == ( *hdsmc = dsmc ) )
	{
	    break;
	}

	/* Read cluster configuration from config.dat. */
	if ( CXcluster_nodes( NULL, &dsmc->cx_configuration) )
	{
	    break;
	}

	for ( node = 0; node < CX_MAX_NODES; node++ )
	{
	    node_info = &dsmc->cx_configuration->cx_nodes[node];

	    /* node configured? */
	    if ( node_info->cx_node_number )
	    {
		lk_dsm_ctx_priv = &dsmc->lk_dsm_ctx_priv[node];

		/* allocate buffer */
		lk_dsm_ctx_priv->lk_dsm_ctx_ring = (LK_DSM *)
		    MEreqmem(0, sizeof(LK_DSM) * LK_DSM_CONTEXT_NELEM,
                             TRUE, &status);
		if (NULL == lk_dsm_ctx_priv->lk_dsm_ctx_ring)
		{
		    MEfree((PTR)dsmc );
		    *hdsmc = NULL;
		    break;
		}

		/* initialize private context */
		LK_dsm_context_priv_init( lk_dsm_ctx_priv );

		/* Init message sequence once */
		lk_dsm_ctx_priv->lk_dsm_ctx_msg_seq = 0;
	    }
	}

	status = OK;
    } while ( 0 );

    if (status == OK)
    {
	/* mark context as good */
	dsmc->lk_dsm_magic = LK_DSM_CONTEXT_MAGIC;
    }
    return (status);
}

/*
** 
** Name: LK_free_dsm_context - Free a DSM context.
**
** Description:
**
**	Close all open channels, deallocate buffers.
**	
** Inputs:
**	dsmc		- DSM context to free.
**
** Outputs:
**	None
**
** Returns:
**	OK on success,
**	other value indicates error
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
**	13-May-2005 (fanch01)
**	    Created.
**
*/
STATUS
LK_free_dsm_context( LK_DSM_CONTEXT **hdsmc )
{
    STATUS status = OK;
    CX_NODE_INFO *node_info;
    LK_DSM_CONTEXT *dsmc = *hdsmc;
    LK_DSM_CONTEXT_PRIV *lk_dsm_ctx_priv;
    i4 node;

    for ( ;; )
    {
	if (dsmc->lk_dsm_magic != LK_DSM_CONTEXT_MAGIC)
	{
	    /* double free (or potential uninitialized) error */
	    status = -1;
	    break;
	}
	/* set to invalid regardless of later error */
	dsmc->lk_dsm_magic = 0;

	for ( node = 0; node < CX_MAX_NODES; node++ )
	{
	    node_info = &dsmc->cx_configuration->cx_nodes[node];

	    /* node configured? */
	    if ( node_info->cx_node_number )
	    {
		lk_dsm_ctx_priv = &dsmc->lk_dsm_ctx_priv[node];

		/* deallocate buffer */
		MEfree( (PTR) lk_dsm_ctx_priv->lk_dsm_ctx_ring );

		/* node has transport open? */
		if ( lk_dsm_ctx_priv->lk_dsm_ctx_handle_alloc != 0 )
		{
		    lk_dsm_ctx_priv->lk_dsm_ctx_handle_alloc = 0;
		    CXppp_disconnect( lk_dsm_ctx_priv->lk_dsm_ctx_handle );
		    CXppp_free( &lk_dsm_ctx_priv->lk_dsm_ctx_handle );
		}
	    }
	}
	MEfree( (PTR)dsmc );
	*hdsmc = NULL;
	break;
    }

    return (status);
}

/*
** 
** Name: LK_mark_dsm_context - Sets a DSM context 'mark'.
**
** Description:
**
**	Makes a note of the current position for the output buffers. The
**	buffer is speculatively populated.  If it is determined that there
**	is no need to send the accumulated DSM records they can be released
**	by LK_unmark_dsm_context.
**	
** Inputs:
**	dsmc		- DSM context to set mark on.
**
** Outputs:
**	None
**
** Returns:
**	OK on success,
**	other value indicates error
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
**	13-May-2005 (fanch01)
**	    Created.
**
*/
STATUS
LK_mark_dsm_context( LK_DSM_CONTEXT *dsmc )
{
    STATUS status = OK;
    CX_NODE_INFO *node_info;
    LK_DSM_CONTEXT_PRIV *lk_dsm_ctx_priv;
    i4 node;

    for ( ;; )
    {
	if (dsmc->lk_dsm_magic != LK_DSM_CONTEXT_MAGIC)
	{
	    /* double free (or potential uninitialized) error */
	    status = -1;
	    break;
	}

	for ( node = 0; node < CX_MAX_NODES; node++ )
	{
	    node_info = &dsmc->cx_configuration->cx_nodes[node];

	    /* node configured? */
	    if ( node_info->cx_node_number )
	    {
		lk_dsm_ctx_priv = &dsmc->lk_dsm_ctx_priv[node];

		/* set mark */
		lk_dsm_ctx_priv->lk_dsm_ctx_mark = lk_dsm_ctx_priv->lk_dsm_ctx_head;
	    }
	}

	break;
    }

    return (status);
}

/*
** 
** Name: LK_unmark_dsm_context - Releases a DSM context 'mark'.
**
** Description:
**
**	Restore current end of unflushed regions to position saved by mark.
**	This releases the speculative entries that were added to the output
**	buffer which were previously marked by LK_mark_dsm_context.
**	
** Inputs:
**	dsmc		- DSM context to release mark of.
**
** Outputs:
**	None
**
** Returns:
**	OK on success,
**	other value indicates error
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
**	13-May-2005 (fanch01)
**	    Created.
**	12-Feb-2007 (jonj)
**	    Reset message sequence number.
**	21-Aug-2007 (jonj)
**	    Do not reset message sequence,
**	    must increase for use as LLB stamp.
**
*/
STATUS
LK_unmark_dsm_context( LK_DSM_CONTEXT *dsmc )
{
    STATUS status = OK;
    CX_NODE_INFO *node_info;
    LK_DSM_CONTEXT_PRIV *lk_dsm_ctx_priv;
    i4 node;

    for ( ;; )
    {
	if (dsmc->lk_dsm_magic != LK_DSM_CONTEXT_MAGIC)
	{
	    /* double free (or potential uninitialized) error */
	    status = -1;
	    break;
	}

	for ( node = 0; node < CX_MAX_NODES; node++ )
	{
	    node_info = &dsmc->cx_configuration->cx_nodes[node];

	    /* node configured? */
	    if ( node_info->cx_node_number )
	    {
		lk_dsm_ctx_priv = &dsmc->lk_dsm_ctx_priv[node];

		/* restore head position to mark */
		lk_dsm_ctx_priv->lk_dsm_ctx_head = lk_dsm_ctx_priv->lk_dsm_ctx_mark;
	    }
	}

	break;
    }

    return (status);
}

/*
** 
** Name: LK_flush_dsm_context_write - Write from a private DSM context output
**	buffer.
**
** Description:
**
**	Write out count elements from the buffer starting at tail.
**	
** Inputs:
**	lk_dsm_ctx_priv	- DSM context to write from.
**	count		- Number of LK_DSM entries to write.
**
** Outputs:
**	None
**
** Returns:
**	OK on success,
**	other value indicates error
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
**	13-May-2005 (fanch01)
**	    Created.
**	21-Feb-2007 (jonj)
**	    Add optional *writes output param where number of
**	    records written may be accumulated.
**	30-Apr-2008 (jonj)
**	    Add LK_DSM *inlkdsm, node, to aid debugging.
**
*/
static STATUS
LK_flush_dsm_context_write( 
LK_DSM *inlkdsm,
LK_DSM_CONTEXT_PRIV *lk_dsm_ctx_priv, 
i4 count, 
u_i4 *writes,
i4 node)
{
    /* write out last piece of buffer */
    LK_DLOCK_TRACE("%@ %d LK_flush_dsm_context_write: %s call CXppp_write count %d node %d\n",
	  __LINE__, 
	  (inlkdsm) ? "DIST" : "LOCL",
	  count,
	  node);

    STATUS status =
	CXppp_write( lk_dsm_ctx_priv->lk_dsm_ctx_handle, (char *)
	   &lk_dsm_ctx_priv->lk_dsm_ctx_ring[lk_dsm_ctx_priv->lk_dsm_ctx_tail],
	   sizeof(LK_DSM), count );

    if (status != OK)
    {
	/* log warning */
	TRdisplay("%@ LK_flush_dsm_context_write: CXppp_write error %x head %d"
		  " tail %d mark %d seq %d\n", status,
		lk_dsm_ctx_priv->lk_dsm_ctx_head,
		lk_dsm_ctx_priv->lk_dsm_ctx_tail,
		lk_dsm_ctx_priv->lk_dsm_ctx_mark,
		lk_dsm_ctx_priv->lk_dsm_ctx_msg_seq); TRflush();

	/* handle other failure contexts? */

	/* non-fatal anticipated error */
    }
    else if ( writes )
        *writes += count;

    return (status);
}

/*
** 
** Name: LK_flush_dsm_context - Flush the DSM output channels.
**
** Description:
**
**	Loop over buffers for each node apart from local node.  If the
**	current end of the write region is not at the beginning of the
**	write area, open channel if not done already, and ship out all the
**	DSM records.  This may require two writes per channel if the leading
**	edge of the write region has wrapped past the end of the ring buffer.
**	
** Inputs:
**	dsmc		- DSM context to flush.
**
** Outputs:
**	None
**
** Returns:
**	OK on success,
**	other value indicates error
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
**	13-May-2005 (fanch01)
**	    Created.
**	21-Feb-2007 (jonj)
**	    Add optional *writes output param where number of
**	    records written may be accumulated.
**	30-Apr-2008 (jonj)
**	    Add LK_DSM *inlkdsm to prototype to identify caller
**	    (LOCL/DIST) as debugging aid.
*/
STATUS
LK_flush_dsm_context( 
LK_DSM	*inlkdsm,
LK_DSM_CONTEXT *dsmc, 
u_i4 *writes )
{
    STATUS status = OK;
    CX_NODE_INFO *node_info;
    LK_DSM_CONTEXT_PRIV *lk_dsm_ctx_priv;
    LK_DSM_CONTEXT_PRIV lk_dsm_ctx_copy;
    char	nodename[CX_MAX_NODE_NAME_LEN];
    i4 		node;

    for ( ;; )
    {
	if (dsmc->lk_dsm_magic != LK_DSM_CONTEXT_MAGIC)
	{
	    /* double free (or potential uninitialized) error */
	    status = -1;
	    break;
	}

	for ( node = 0; node < CX_MAX_NODES; node++ )
	{
	    node_info = &dsmc->cx_configuration->cx_nodes[node];

	    /* node configured? */
	    if ( node_info->cx_node_number )
	    {
		lk_dsm_ctx_priv = &dsmc->lk_dsm_ctx_priv[node];

		/* was channel was active */
		if ( lk_dsm_ctx_priv->lk_dsm_ctx_active == 0 )
		    continue;

		/* was transport layer opened? */
		if ( lk_dsm_ctx_priv->lk_dsm_ctx_handle_alloc == 0 )
		{
		    status = CXppp_alloc( &lk_dsm_ctx_priv->lk_dsm_ctx_handle,
		                          0, 0, 0, debug_cpp );
		    if ( status != OK )
		    {
			/* log warning */
			TRdisplay("%@ LK_flush_dsm_context: alloc error %x\n",
			    status); TRflush();

			/* handle other failure contexts? */

			/* reset context */
			LK_dsm_context_priv_init( lk_dsm_ctx_priv );

			/* non-fatal anticipated error */
			continue;
		    }
		    STlcopy( node_info->cx_node_name, nodename,
		             node_info->cx_node_name_l );
		    status = CXppp_connect(
		      lk_dsm_ctx_priv->lk_dsm_ctx_handle, 
		        node_info->cx_node_name, dsm_svc_name() );
		    if ( status != OK )
		    {
			/* log warning */
			TRdisplay("%@ LK_flush_dsm_context: connect error %x\n",
			    status); TRflush();

			/* handle other failure contexts? */

			/* reset context */
			LK_dsm_context_priv_init( lk_dsm_ctx_priv );

			/* non-fatal anticipated error */
			continue;
		    }
		    lk_dsm_ctx_priv->lk_dsm_ctx_handle_alloc = 1;
		}

		/* save a copy in case of problems */
		lk_dsm_ctx_copy = *lk_dsm_ctx_priv;

		/* writes wrapped past end of buffer? */
		if ( lk_dsm_ctx_priv->lk_dsm_ctx_head <
		     lk_dsm_ctx_priv->lk_dsm_ctx_tail)
		{
		    LK_DLOCK_TRACE("%@ %d LK_flush_dsm_context: %s call _write head %d"
			" tail %d node %d\n", 
			__LINE__, 
			(inlkdsm) ? "DIST" : "LOCL",
			LK_DSM_CONTEXT_NELEM,
			lk_dsm_ctx_priv->lk_dsm_ctx_tail,
			node_info->cx_node_number);

		    status = LK_flush_dsm_context_write( 
				inlkdsm,
				lk_dsm_ctx_priv,
				LK_DSM_CONTEXT_NELEM - lk_dsm_ctx_priv->lk_dsm_ctx_tail,
				writes,
				node_info->cx_node_number);
		    if ( status != OK )
		    {
			if (status == E_CL2C30_CX_E_OS_UNEX_FAIL)
			{
			    /*
			    ** Most likely a link disconnect.  Try resending
			    ** all messages to the same node.
			    */
			    *lk_dsm_ctx_priv = lk_dsm_ctx_copy;
			    lk_dsm_ctx_priv->lk_dsm_ctx_handle_alloc = 0;
			    TRdisplay("%@ LK_flush_dsm_context: attempt to "
				"resend to node %d\n",
				node_info->cx_node_number); TRflush();
			    --node;
			    continue;
			}
			else
			{
			    TRdisplay("%@ LK_flush_dsm_context: "
				"unexpected error %x\n", status); TRflush();
			    break;
			}
		    }

		    /* mark between tail and end of buffer? */
		    if ( lk_dsm_ctx_priv->lk_dsm_ctx_mark >
			 lk_dsm_ctx_priv->lk_dsm_ctx_tail)
		    {
			lk_dsm_ctx_priv->lk_dsm_ctx_mark = 0;
		    }

		    /* wrap tail to beginning */
		    lk_dsm_ctx_priv->lk_dsm_ctx_tail = 0;
		}

		/* more to flush out? */
		if ( lk_dsm_ctx_priv->lk_dsm_ctx_head >
		     lk_dsm_ctx_priv->lk_dsm_ctx_tail )
		{
		    LK_DLOCK_TRACE("%@ %d LK_flush_dsm_context: %s call _write head %d"
			" tail %d node %d\n", 
			__LINE__, 
			(inlkdsm) ? "DIST" : "LOCL",
			lk_dsm_ctx_priv->lk_dsm_ctx_head,
			lk_dsm_ctx_priv->lk_dsm_ctx_tail,
			node_info->cx_node_number);

		    status = LK_flush_dsm_context_write( 
				 inlkdsm,
				 lk_dsm_ctx_priv,
				 lk_dsm_ctx_priv->lk_dsm_ctx_head
				 - lk_dsm_ctx_priv->lk_dsm_ctx_tail,
				 writes,
				 node_info->cx_node_number);

		    if ( status != OK )
		    {
			if (status == E_CL2C30_CX_E_OS_UNEX_FAIL)
			{
			    /*
			    ** Most likely a link disconnect.  Try resending
			    ** all messages to the same node.
			    */
			    *lk_dsm_ctx_priv = lk_dsm_ctx_copy;
			    lk_dsm_ctx_priv->lk_dsm_ctx_handle_alloc = 0;
			    TRdisplay("%@ LK_flush_dsm_context: attempt to "
				"resend to node %d\n",
				node_info->cx_node_number); TRflush();
			    --node;
			    continue;
			}
			else
			{
			    TRdisplay("%@ LK_flush_dsm_context: "
				"unexpected error %x\n", status); TRflush();
			    break;
			}
		    }

		    /* mark between start of buffer and head? */
		    if ( lk_dsm_ctx_priv->lk_dsm_ctx_mark <
			 lk_dsm_ctx_priv->lk_dsm_ctx_head)
		    {
			lk_dsm_ctx_priv->lk_dsm_ctx_mark = lk_dsm_ctx_priv->lk_dsm_ctx_head;
		    }

		    /* tail catches head */
		    lk_dsm_ctx_priv->lk_dsm_ctx_tail = lk_dsm_ctx_priv->lk_dsm_ctx_head;
		}

		/* mark that we're caught up */
		lk_dsm_ctx_priv->lk_dsm_ctx_active = 0;
	    }
	}

	break;
    }

    return (status);
}

/*
** 
** Name: LK_get_free_dsm - Returns writeable record in the DSM output buffer.
**
** Description:
**
**	Returns pointer to current leading edge of write area, and advances
**	the leading edge.  If leading edge starts intruding on trailing edge
**	of write area then flush out some records, advancing begin of write
**	area as needed.  If this flush moves begin of write area past mark
**	pointer, adjust mark pointer also.
**	
** Inputs:
**	dsmc		- DSM context to return record from.
**	node		- Target node for the LK_DSM record.
**
** Outputs:
**	None
**
** Returns:
**	LK_DSM* to the writeable record.
**	NULL indicates error.
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
**	13-May-2005 (fanch01)
**	    Created.
**	12-Feb-2007 (jonj)
**	    Add lk_dsm_ctx_msg_seq.
**
*/
LK_DSM *
LK_get_free_dsm( LK_DSM_CONTEXT *dsmc, i4 node )
{
    STATUS status = OK;
    CX_NODE_INFO *node_info;
    LK_DSM_CONTEXT_PRIV *lk_dsm_ctx_priv;
    LK_DSM *lkdsm;
    i4 next_head;

    for ( ;; )
    {
	if (dsmc->lk_dsm_magic != LK_DSM_CONTEXT_MAGIC)
	{
	    /* double free (or potential uninitialized) error */
	    lkdsm = NULL;
	    break;
	}

	node_info = &dsmc->cx_configuration->cx_nodes[node - 1];

	/* node configured? */
	if ( node_info->cx_node_number == 0 )
	{
	    /* requested node not configured */
	    lkdsm = NULL;
	    break;
	}

	lk_dsm_ctx_priv = &dsmc->lk_dsm_ctx_priv[node - 1];

	/* next lkdsm */
	next_head = (lk_dsm_ctx_priv->lk_dsm_ctx_head + 1) % LK_DSM_CONTEXT_NELEM;
	lkdsm = &lk_dsm_ctx_priv->lk_dsm_ctx_ring[lk_dsm_ctx_priv->lk_dsm_ctx_head];

	/* overrun the tail? */
	if (next_head == lk_dsm_ctx_priv->lk_dsm_ctx_tail )
	{
	    /* complain */
	    TRdisplay("%@ warning: %s %d overrun tail for node(%d)\n",
			    __FILE__, __LINE__, node); TRflush();
	    LK_flush_dsm_context((LK_DSM*)NULL, dsmc, NULL);
	}

	/* update head */
	lk_dsm_ctx_priv->lk_dsm_ctx_head = next_head;
	/* indicate activity */
	lk_dsm_ctx_priv->lk_dsm_ctx_active = 1;

	/* Stamp with msg sequence */
	lkdsm->msg_seq = ++lk_dsm_ctx_priv->lk_dsm_ctx_msg_seq;

	break;
    }

    return (lkdsm);
}
/*
** 
** Name: LK_get_next_dsm - Returns pointer to next DSM 
** 			   between "tail" and "head"
**
** Description:
**
**	Returns pointer to next DSM between "tail" and "head" so one
**	may peruse messages accumulated for write.
**	
** Inputs:
**	dsmc		- DSM context to return record from.
**	node		- Target node for the LK_DSM record.
**	dsm_num		- index to next LK_DSM from previous call.
**
** Outputs:
**	dsm		- Pointer to LK_DSM, must be NULL on
**			  first call to this function.
**			  Will be NULL when head reached.
**	dsm_num		- updated to index of next LK_DSM
**
** Returns:
**	void
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
**	16-Feb-2007 (jonj)
**	    Created.
*/
void
LK_get_next_dsm( 
LK_DSM_CONTEXT *dsmc, 
i4 		node,
LK_DSM		**dsm,
i4		*dsm_num )
{
    CX_NODE_INFO 	*node_info;
    LK_DSM_CONTEXT_PRIV *ctx;
    LK_DSM 		*lkdsm = NULL;

    /* double free (or potential uninitialized) error? */
    if (dsmc->lk_dsm_magic == LK_DSM_CONTEXT_MAGIC)
    {
	node_info = &dsmc->cx_configuration->cx_nodes[node - 1];

	/* node configured? */
	if ( node_info->cx_node_number != 0 )
	{
	    ctx = &dsmc->lk_dsm_ctx_priv[node - 1];

	    if ( !(*dsm) )
		/* First "next", start at tail */
		*dsm_num = ctx->lk_dsm_ctx_tail;

	    /* Quit when we get to head */
	    if ( *dsm_num != ctx->lk_dsm_ctx_head )
	    {
		/* Return LK_DSM, update "next" */
		lkdsm = &ctx->lk_dsm_ctx_ring[*dsm_num];
		*dsm_num = (*dsm_num + 1) % LK_DSM_CONTEXT_NELEM;
	    }
	}
    }

    *dsm = lkdsm;
}

#else

DB_STATUS
LKdist_deadlock_thread( DMC_CB *dmc_cb )
{
    return E_DB_ERROR;
}

#endif
