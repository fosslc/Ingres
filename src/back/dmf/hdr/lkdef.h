/*
** Copyright (c) 1985, 2005 Ingres Corporation
**
*/

/**
** Name: LKDEF.H - Definition of LK types and constants.
**
** Description:
**      This file defines the types and constants that the routines
**      in LK operate on.  These types describe lock lists,
**      resouces, and locks on resources by lock list.
**
** History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable logging and locking system.
**	7-oct-1992 (bryanp)
**	    Add prototypes for the new LKMO code.
**	16-feb-1993 (bryanp)
**	    6.5 Cluster Project:
**	    => Modified return type of LK_do_unlock()
**	24-may-1993 (bryanp)
**	    Separated lkb's and rsb's into separate memory pools, primarily
**		to break the 64K limit on the sbk table which many customers
**		are hitting.
**	    Added LK_my_pid for use by lkmutex.c
**	    Define lkd_status bit "LKD_IN_CSP_COMPLETION" for mutex optimization
**	    Added E_DMA017_LK_MUTEX_RELEASED pseudo-message for mutex
**		optimization.
**	    Added lkreq argument to LK_wakeup.
**	    Added pid field to LKREQ structure to trace Process ID of requestor.
**	    Replace uses of llb_async_status and llb_async_value since
**		this technique didn't work correctly for LK_MULTITHREAD lock
**		lists.
**	    Added lots of new statistics.
**	    Add minimum legal values for LLB and BLK parameters.
**	21-jun-1993 (bryanp)
**	    Add prototype for LK_csp_rcp_online();
**	    Add special LKREQ message which triggers the CSP into the debugger.
**	    Add prototype for LK_cancel().
**	    Changed arguments to LK_suspend().
**	    Moved LK_COMP_HANDLE type here from LKRQST.C for use by LK_cancel().
**	26-jul-1993 (bryanp)
**	    Define LKREQ_STALLED_CONVERSION for use by cluster lock stalling.
**	    Replaced usage of "short" with "i2" or "nat" as appropriate.
**	    Add error message definitions.
**	23-aug-1993 (andys)
**	    Update LK_do_new_lock_request prototype.
**	31-jan-1994 (mikem)
**	    Fixed comment within a comment error which caused compiler
**	    warning on su4_us5.
**	23-may-1994 (bryanp) B58498
**	    Add prototypes for LKintrcvd, LKintrack.
**      16-nov-1994 (medji01)
**          Mutex Granularity Project
**              - Add mutexes to the RSB, LLB, and LKH (_LKBQ).
**              - Changed lkmutex function prototypes to reflect parm
**                changes.
**		- Added LK_xxx_SEMAPHORE #defines.
**	21-aug-1995 (dougb) bug #70696
**	    Add new LLB fields required in LKdeadlock() in fix for bug #70696.
**	    Update prototype for LK_do_new_request_lock().
**	06-Dec-1995 (jenjo02)
**	    Added distinct mutex for LKevents, lkd_ev_mutex.
**	11-Dec-1995 (jenjo02)
**	    Removed lkd_sm_mutex. Added additional per-queue mutexes.
**	19-Dec-1995 (jenjo02)
**	    Added "mode" parm to LK_mutex(), either SHAREd or EXCLusive.
**	03-jan-1996 (duursma)
**	    Deleted unused field unsent_gdlck_search field.  Replaced it with
**	    new field walk_wait_for to count calls to walk_wait_for_graph().
**	26-feb-1996 (duursma)
**	    Removed spurious VERSION markers from jenjo02's integration.
**	01-mar-1996 (duursma)
**	    Fixed free_res parameter of LK_do_new_lock_request, it should 
**	    be a i4  not a bool, because it is a bit mask.
**	14-mar-1996 (nick)
**	    Add prototypes for LK_[get|rel]_critical_sems().
**	14-May-1996 (jenjo02)
**	    Added llb_deadlock_rsb, lkd_dlock_mutex.
**	30-may-1996 (pchang)
**	    Added LLB as a parameter to LK_allocate_cb() to enable the
**	    reservation of SBK table resource for use by the recovery 
**	    process. (B76879)
**	21-aug-1996 (boama01)
**	    Multiple chgs associated w. fixes for Cluster Support and
**	    semaphore probs on AXP/VMS:
**	    - Change *RSB parameter of LK_try_convert() and LK_try_wait() to
**	      **RSB, since it can be changed by LK_try_wait().
**	    - Added LKD_CSP_HOLD_NEW_RQSTS and LKD_CSP_HELD_NEW_RQST bits to
**	      lkd_status to hold up new AST-delivered requests during CSP
**	      recovery stalled-request completion.
**	    - Added LK_MUTEX_TRACE symbol to control production of LK mutex
**	      trace entries (see generic!back!dmf!lk LKMUTEX.C).
**	    - ALPHA ONLY: Added fld before first semaphore in LKD struct to
**	      get all sems on longword alignment.
**	11-sep-96 (nick)
**	    Define new result flag LKREQ_CB_DEALLOCATED which is set in the 
**	    LKREQ when the resources which were specified as 'free on fail' are
**	    deallocated.
**	13-dec-96 (cohmi01)
**	    Adj prototypes for LK_request_lock(), LK_do_new_lock_request().
**	03-Oct-1996 (jenjo02)
**	    Added llb_deadlock_llb.
**	04-Oct-1996 (jenjo02)
**	    Added *lkb to LK_COMP_HANDLE in which to pass back the LKB to avoid
**	    the LLB->LKB chain scan in LK_get_completion_status.
**	08-Oct-1996 (jenjo02)
**	    Added lkd_mutex to block simultaneous updates of
**	    lkd_rqst_id.
**      01-aug-1996 (nanpr01 for ICL keving)
**          Add CBT structure to hold process' Callback Thread info. These 
**          hang off lkd.
**          Add CBACK structure to hold callbacks waiting to be run. These 
**          hang off the appropriate cbt.
**          Added callback details to LKB and LKREQ to support blocking    
**          callbacks. 
**          Add rsb_cback_count to RSBs to store # of LKBs in granted queue
**          with valid cback. 
**          LK_try_wait and LK_try_convert now return a STATUS.
**	01-aug-1996 (nanpr01 for jenjo02)
**	    Added semaphores to make keving's CBT/CBACK stuff work.
**	    Added cback_key in which to save lock key when callback is fired.
**	15-Nov-1996 (jenjo02)
**	    Removed rsb_dlock, rsb_dlock_mutex, llb_deadlock_rsb, llb_deadlock_llb.
**	    Added deadlock wait queue, 2 new stats, dlock_wakeups,
**	    dlock_max_q_len, to LKD.
**	    Added LLB_ON_DLOCK_LIST, LLB_DEADLOCK status bits,
**	    llb_deadlock_wakeup.
**	06-Dec-1996 (jenjo02)
**	    Removed lkd_mutex.
**      23-jan-1997 (stial01)
**          Added message code for error handling in lock propagate.
**	25-Feb-1997 (jenjo02)
**	    Added lkd_ew_stamp, llb_ew_stamp, removed unused llb_timeout.
**      27-feb-1997 (stial01)
**          Added LLB_XSVDB.
**	11-Apr-1997 (jenjo02)
**	    Added new stats for callback threads.
**	    Removed cback_mutex, lkd_cbt_next/prev, lkd_cbt_q_mutex.
**	    Moved callback function prototypes to here from lkcback.c.
**	    Removed function prototype for LK_rmutex(), no longer used.
**	22-Apr-1997 (jenjo02)
**	    Removed nick's LKREQ_CB_DEALLOCATED flag. When the LKB itself
**	    is one of the deallocated resources it cannot be viewed
**	    in a consistent state; in fact, the LKB may have well been
**	    reallocated to another thread!
**	05-May-1997 (jenjo02)
**	  - Added LKB_PROPAGATE flag in support of
**	    lock propagation.
**	  - Upped LKD_MAXIMUM_LKH/RSH from 65000 to aleph.
**      21-May-1997 (stial01)
**          Added LLB_XROW
**	20-Jul-1998 (jenjo02)
**	    llb_event_type, llb_event_value changed from 
**	    i4  to LK_EVENT structure.
**	08-Sep-1998 (jenjo02)
**	    Added PHANTOM_RESUME_BUG define.
**      05-Nov-1998 (hanal04)
**          Added sid and pid to completion handle to protect against use
**          of wrong lkb->lkreq in LK_get_completion_status. Also added
**          definition for LKREQ_COLLECTED. b90388. Crossed from 1.2/01.
**	16-Nov-1998 (jenjo02)
**	  o When CL-available, use ReaderWriter Lock for lkd_dlock_lock
**	    (deadlock matrix), which is an ideal candidate (many readers,
**	    one writer, the deadlock thread). When unavailable, a regular
**	    CS_SEMAPHORE is used instead. Added defines and macros to
**	    support either mode.
**	  o Added max_lkd_per_txn (max locks allocated per txn) stat.
**	    Cross-process thread identity changed to CS_CPID structure
**	    from PID and SID.
**	  o Removed PHANTOM_RESUME_BUG code.
**	14-Jan-1999 (jenjo02)
**	    Rework of deadlock wait queue to focus on LKBs instead of
**	    LLBs, provide for multiple waiting LKBs per LLB.
**	02-Aug-1999 (jenjo02)
**	    Implement preallocated RSB/LKB stash per LLB.
**	    Each RSB now contains an embedded LKB.
**	    Added LLB parm to LK_deallocate_cb() prototype.
**	    Added rsb_waiters, rsb_converters counts to RSB.
**      20-Aug-1999 (hanal04) Bug 97316 INGSRV921
**         Added E_DMA039_LK_HARD_LOCK_LIMIT to report cases where
**         llb->llb_lkb_count hits the hard limit imposed by its
**         datatype.
**	10-Oct-1999 (devjo01)
**	   SIR 99124 (partial cross of jenjo02 change 437107).
**	    llb_event_type, llb_event_value changed from 
**	    nat to LK_EVENT structure.
**	20-Apr-2000 (devjo01)
**	   Add built in lock performance statistics stuff.
**	10-May-2000 (devjo01)
**	   Put lock requests in shared memory when running clustered.
**	   Add LK_send_to_csp.  Add LK_CLUSTER_RUNDOWN macro. (s101483).
**	27-Aug-2001 (devjo01)
**	   Remove obsolete declaration of LK_do_convert.  This function
**	   is only called within LKRQST.C, and is now a static function
**	   in that module.
**	06-Oct-1999 (jenjo02)
**	    Reduced max LLBs to 65535, created LLB_ID type to reinstate
**	    instance + id for lock lists. Other upper limits (LKBs, RSBs)
**	    reduced from 4,294,967,296, which CBF doesn't support, to
**	    2,147,483,647, which it does. LK_ID changed from u_i4 to i4.
**	22-Oct-1999 (jenjo02)
**	    Removed reader/writer lkd_dlock_lock deadlock wait-for graph
**	    blocking mechanism. deadlock() is now able to traverse the
**	    graph using RSB mutexes and no longer needs this coarse lock.
**	30-Nov-1999 (jenjo02)
**	    Added DB_DIS_TRAN_ID llb_dis_tran_id to LLB for shared
**	    XA lock lists.
**	15-Dec-1999 (jenjo02)
**	    Removed llb_dis_tran_id. Mechanics of sharing pushed up
**	    into LG.
**	02-Feb-2000 (jenjo02)
**	    Inline CSp|v_semaphore calls. LK_mutex, LK_unmutex are
**	    now macros calling LK_mutex_fcn(), LK_unmutex_fcn() iff
**	    a semaphore error occurs.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	04-Oct-2000 (jenjo02)
**	    Added LKcp_resume macro to call CSresume
**	    directly if local waiter.
**	12-apr-2001 (devjo01)
**	    Put embedded CX structures within LKB & RSB for 103715.
**	18-Sep-2001 (jenjo02)
**	    Added LLB_ALLOC flag, set when new lock/resource
**	    is being allocated/deallocated to a lock list to
**	    prevent the LLB's LKB/RSB stash from being scavenged.
**	17-may-2002 (devjo01)
**	    Put constants for 'flag' arg to LK_do_unlock here.
**	20-Aug-2002 (jenjo02)
**	    Added blocking_cpid, blocking_mode to LK_COMP_HANDLE,
**      23-Oct-2002 (hanch04)
**          Added definition for LK_size
**	    blocking_mode to LKREQ.
**	12-nov-2002 (horda03) Bug 105321
**	    Added LLB_CANCELLED_EVENT to show when a LK_event() wait has
**	    been cancelled. That is, LKevent() will not expect a thread
**	    to be resumed when LK_cancel()returns LK_CANCEL.
**      28-Dec-2003 (horda03) Bug 111461/INGSRV 2634
**          Dirty scans are now being made of the LKBQ link list. If
**          a change to the pointers of the LKBQ elements is detected,
**          the scan starts at the beginning of the list. To ensure
**          that the correct start location is used, made lkbq_next
**          volatile.
**	25-jan-2004 (devjo01)
**	    Resurrect cbt_mutex for those ports which do not currently
**	    support compare and swap.
**	13-feb-2004 (devjo01)
**	    Correct error in LK_HOOK_CBT macro.
**      22-sep-2004 (fanch01)
**          Replace meaningless LKB_TEMPORARY flag with LKB_LOCAL.
**	    If set, then this lock is not taken cluster wide if
**	    running clustered Ingres.  This is OK if we know that
**	    the resource is only accessible on one node, but it is
**	    a gross logic error to either take a local lock on the
**	    same resource from two nodes (they can't see each other),
**	    or to take the lock globally, then convert it locally.
**	    For convenience, convert will always use the same scope
**	    as the original request.
**      7-oct-2004 (thaju02)
**          Use SIZE_TYPE to allow memory pools > 2Gig.
**	05-may-2005 (devjo01)
**	    - Add LKD_DISTRIBUTED_DEADLOCK as part of making the old
**	    VMS specific distributed deadlock support code generic,
**	    and integrating it to r3.
**	    - New structure LK_DSM_CONTEXT. (deadlock search msg)
**	    - Add prototypes for several DSM functions.
**	    - Remove local LKSB.
**	28-may-2005 (devjo01)
**	    Remove duplicated LK_DSM_CONTEXT supplanted by fanch's version.
**	06-jun-2005 (devjo01)
**	    Correct benign mis-declaration which led to compiler warnings
**	    which were not reported on VMS.
**	21-jun-2005 (devjo01)
**	    Add LKD_NO_DLM_GLC.
**	25-jul-2005 (abbjo03)
**	    In LKBQ, declare lkbq_next and lkbq_prev as volatile so that
**	    optimization will not cause changes in the list to be not visible
**	    to other processes.
**	02-jan-2007 (abbjo03)
**	    Add file and sequence parameters to LK_local_deadlock for tracing
**	    purposes.
**	27-feb-2007 (jonj/abbjo03)
**	    Add optional output argument to LK_flush_dsm_context_write to
**	    accumulate number of records written.  For clusters, define
**	    LK_TRACE_CALLBACK_ENABLED.
**	19-Mar-2007 (jonj)
**	    Add IsRundown to LK_cancel_event_wait prototype.
**	24-May-2007 (jonj)
**	    Add LKDEADLOCK_DEBUG definitions, LK_DLOCK_TRACE macro 
**	    for circular trace file - VMS clusters only.
**	01-Oct-2007 (jonj)
**	    Removed distinct _CBACK structure, now embedded in LKB
**	    delete lk_cback_alloc(), lk_cback_dealloc()
**	    prototypes.
**	26-Oct-2007 (jonj)
**	    Add missing function prototype for LK_get_next_dsm().
**	31-Jan-2008 (jonj)
**	    Add values for "must_retry" return parm in LK_local_deadlock()
**	    Change LK_local_deadlock() prototype param max_request_mode
**	    from i4 to pointer to i4.
**	30-Apr-2008 (jonj)
**	    Add LK_DSM *inlkdsm to LK_flush_dsm_context() prototype
**	    to identify caller (LOCL/DIST) for debugging.
**	08-Jan-2009 (jonj)
**	    Elided LKREQ structure and use, as requests haven't been 
**	    packaged and sent to the CSP in years. What async stuff
**	    of use remains from LKREQ was integrated into the LKB.
**	    Deleted obsolete function prototypes, removed some which
**	    are really statics (LK_do_convert, LK_do_new_lock_request).
**	    Add definable LK_MUTEX_DEBUG, use LK_SEMAPHORE rather than
**	    CS_SEMAPHORE; if LK_MUTEX_DEBUG is defined here, LK_SEMAPHORE
**	    will be a structure with file/line information of last
**	    successful mutex/unmutex call, useful especially for solving
**	    mutex self-deadlocks during development.
**/

# ifndef CX_H_INCLUDED
# include <cx.h>
# endif

/*
**  Forward and/or External typedef/struct references.
*/
typedef struct _LKD	LKD;
typedef struct _LKB	LKB;
typedef struct _LLB	LLB;
typedef struct _LKBQ	LKBQ;
typedef struct _LLBQ	LLBQ;
typedef struct _RSB	RSB;
typedef struct _RSH	RSH;
typedef struct _LKH	LKH;
typedef struct _CBT     CBT;  
typedef struct _CBACK   CBACK;
typedef struct _LLB_ID  LLB_ID;

/*
** Maximum sizes for the various LKD structures. The numbers picked are
** fairly arbitrary; they happened to be the numbers used by the old VMS
** Cluster locking system.
**
** The minimum values are fairly arbitrary, too. I selected them by monkeying
** around with my system until I found a smallest value with which I could
** start a (barely) usable system. Real-life systems will have values much
** larger than these.
*/
#define	LKD_MAXNUM_LLB	2147483647
#define LKD_MINNUM_LLB	30

#define LKD_MAXNUM_BLK	2147483647
#define LKD_MINNUM_BLK	       100

#define LKD_MAXNUM_RSB	2147483647
#define LKD_MINNUM_RSB         100

#define LKD_MAXNUM_LKH	2147483647
#define	LKD_MINNUM_LKH	    1

#define LKD_MAXNUM_RSH	2147483647
#define LKD_MINNUM_RSH      1

/*
**  Constants for the free_res argument of LK_do_new_lock_request().
*/
#define                 LK_FREE_LKB	0x01L
#define			LK_FREE_RSB	0x02L
#define			LK_FREED_LKB	0x04L
#define			LK_FREED_RSB	0x08L

/*
**  Constants for the 'flag' argument of LK_do_unlock().
*/
#define			LK_DO_UNLOCK_CANCEL	1
#define			LK_DO_UNLOCK_RUNDOWN	2

/*
** If you change these #undef's to #define's, the locking system begins
** to produce voluminous TRdisplays...
*/
#undef	LK_TRACE
#undef	CSP_DEBUG 		
#undef	LKD_DEBUG	
#undef	CB_DEBUG 
#undef	xEVENT_DEBUG
#undef	CSP_DEBUG_LEVEL2 
/*
** If you change this #undef to a #define, you allow LK mutex traces to be
** stored/produced by the code.  You must still define the II_LKMUTEX_TRACE
** environment symbol at ingstart time to actually activate tracing.
*/
/* #undef LK_MUTEX_TRACE  */

/*
** Debugging macros
*/
#ifdef CB_DEBUG	
#define CB_CHECK(ctlBlock,fldAddr,eyecatch,location)  \
	if (fldAddr != eyecatch)			\
	{	\
	    TRdisplay("%@ %s validation error at %s. Expected eyecatch, found %d.\n", \
	       	      ctlBlock,location,fldAddr);     \
	}
#else
#define CB_CHECK(ctlBlock,fldAddr,eyecatch,location)  ;
#endif

#ifdef LK_TRACE	
#define	LK_WHERE(whereat)  \
    { \
	TRdisplay("%@ %s \n", whereat); \
	TRflush(); \
    }
#else
#define LK_WHERE(wherename)  ;
#endif

/* Option to encode successful p/v's with source info for debugging */
#undef	LK_MUTEX_DEBUG

/* What a LK_SEMAPHORE looks like */
#ifdef LK_MUTEX_DEBUG
typedef struct _LK_SEMAPHORE
{
    CS_SEMAPHORE	mutex;		/* The semaphore/mutex */
    PTR			file;		/* Where last set or released */
    i4			line;
} LK_SEMAPHORE;
#else
/* In the normal, non-debug case, LK_SEMAPHORE is simply a CS_SEMAPHORE */
#define	LK_SEMAPHORE CS_SEMAPHORE
#endif /* LK_MUTEX_DEBUG */

/*
** Constants used by LK_mutex()
*/
#define	SEM_SHARE	0
#define	SEM_EXCL	1

/* Macros to acquire/release an LK mutex */
#if defined(LK_MUTEX_TRACE) || defined(VAX) || defined(LGK_NO_INTERRUPTS)
#define		LK_mutex(mode, sem) \
	   LK_mutex_fcn(mode, sem, __FILE__, __LINE__)
#define		LK_unmutex(sem) \
	   LK_unmutex_fcn(sem, __FILE__, __LINE__)
#else

#ifdef LK_MUTEX_DEBUG
/* Call the approprate LK function to annotate where p/v'd */
#define		LK_mutex(mode, sem) \
	   LK_mutex_fcn(mode, sem, __FILE__, __LINE__)
#define		LK_unmutex(sem) \
	   LK_unmutex_fcn(sem, __FILE__, __LINE__)
#else
/* Non-debug, direct call to p/v, call LK function if error */
#define		LK_mutex(mode, sem) \
	( ( CSp_semaphore(mode, sem) ) \
	 ? LK_mutex_fcn(mode, sem, __FILE__, __LINE__) : OK )
#define		LK_unmutex(sem) \
	( ( CSv_semaphore(sem) ) \
	 ? LK_unmutex_fcn(sem, __FILE__, __LINE__) : OK )
#endif /* LK_MUTEX_DEBUG */

#endif /* LK_MUTEX_TRACE */

/*
** Depending on whether C&S is enabled.  Handle mainatainance
** of the call back list using either Compare & Swap (desired),
** or as a stopgap for those platforms which do not yet support
** Compare & Swap operations the standard LK_mutex reserve/release
** logic.  'conf_CAS_ENABLED' is mandatory for cluster builds.
*/
#if defined(conf_CAS_ENABLED)

	/*
	** Unhook current CBACK list using atomic compare & swap.
	**
	** This is done to allow for the possibility that signal
	** handle/AST code may be adding a new element at this moment.
	** SH/AST code may not attempt to grab a mutex, since if the
	** mutex is held, a mutex deadlock would result.
	**
	** This can only happen with clustered Ingres, but there is
	** no advantage to maintain separate logic for the "normal"
	** case (except removing the need to code C&S routines).
	*/

# define LK_HOOK_CBT(cbt, oldoffset, newoffset) \
	do \
	{ \
	    oldoffset = cbt->cbt_cback_list; \
	} while ( CScas4( &cbt->cbt_cback_list, oldoffset, newoffset ) )

#else
	
# define LK_HOOK_CBT(cbt, oldoffset, newoffset) \
	(void)LK_mutex(SEM_EXCL, &cbt->cbt_mutex); \
	oldoffset = cbt->cbt_cback_list; \
	cbt->cbt_cback_list = newoffset; \
	(void)LK_unmutex(&cbt->cbt_mutex)

#endif

/* If local waiter, call CSresume directly */
#define		LKcp_resume(cpid) \
	( (cpid)->pid == LGK_my_pid ) \
	 ? CSresume((cpid)->sid) \
	 : CScp_resume(cpid)

typedef struct
{
    SIZE_TYPE	proc_llb_offset;	/* LLB of request */
    SIZE_TYPE	lkb_offset;		/* LKB of request */
    u_i4	request_id;		/* Unique identifier of request */
    /*b90388*/
    CS_CPID     cpid;			/* Id of lock requestor */
    CS_CPID	blocking_cpid;		/* CPID of lock blocker */
    i4		blocking_mode;		/* Lock mode of blocker */
} LK_COMP_HANDLE;

/*}
** Name: LK_ID - Internal form of an LKB/RSB identifier.
**
** Description:
**
** History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable logging and locking system.
**	26-jul-1993 (bryanp)
**	    Turned component types into u_i2 rather than unsigned short.
**	06-Oct-1999 (jenjo02)
**	    Changed to simple typedef instead of structure containing
**	    only a u_i4. LK_ID's identify LKBs and RSBs.
*/
typedef	u_i4	LK_ID;

/*}
** Name: LLB_ID - Internal form of an LLB identifier.
**
** Description:
**
** History:
**	06-Oct-1999 (jenjo02)
**	    Created, to distinguish from LK_ID.
*/
struct _LLB_ID
{
    u_i2	id_instance;	/* Instantiation of this LLB */
    u_i2	id_id;		/* Invariant LLB number */
}; 

/*}
** Name: LLBQ - LLB Queue 
**
** Description:	
**      The LLBQ structure is also used to queue LLB's waiting for an
**      LKevent or deadlock detection, in which case it is a queue of 
**	LLB's, not of LKB's!
**
** History:
**	09-feb-1995 (medji01)
**	    Added comment block
*/
struct _LLBQ
{
    SIZE_TYPE	    llbq_next;		/* Next LKB or LLB */
    SIZE_TYPE	    llbq_prev;		/* Previous LKB or LLB */
};

/*}
** Name: LKB - Lock Block.
**
** Description:
**      A lock block describes a lock that a lock list has on a resource.
**      There is one lock block for each resource that is locked by a lock
**      list.  This lock block is queued to one of the resource state queues
**      and is queued to the hash queue used to locate the lock block by
**      lock list identifier.
**
**	Diagram of single LKB queued to an RSB.
**
**       RSB
**	+------------------+
**      |                  |<-----------+<----------+
**      |rsb_grant_q_next  .----------+ |           |        
**      |                  |          v |           |
**      |------------------|         +--.---------+-.----------+
**      |                  |      +->|            |            |
**      |rsb_grant_q_prev  .------+  | lkb_q_next | lkb_q_prev |
**      |                  |         |            |            |
**      |------------------|         +------------+------------+
**
**      Diagram of multiple LKB's queued to an RSB.
**
**       RSB
**	+------------------+
**      |                  |<-----------+<----------+
**      |rsb_grant_q_next  .----------+ |           |        
**      |                  |          v |           |
**      |------------------|         +--.---------+-.----------+
**      |                  |      +->|            |            |
**      |rsb_grant_q_prev  .------+  | lkb_q_next | lkb_q_prev |
**      |                  |         |            |            |
**      |------------------|         +--.---------+------------+
**                                      |            +
**                                      v            |
**                                   +---------------.---------+
**                                   |            |            |
**                                   | lkb_q_next | lkb_q_prev |
**                                   |            |            |
**                                   +-------------------------+
**
** History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable logging and locking system.
**	26-jul-1993 (bryanp)
**	    Replaced "short" with "nat" or "i2" as appropriate.
**      01-aug-1996 (nanpr01 for ICL keving)
**          Added callback details to support blocking callbacks.
**	05-May-1997 (jenjo02)
**	    Added LKB_PROPAGATE flag in support of
**	    lock propagation.
**	14-Jan-1999 (jenjo02)
**	    Added LKB_ON_DLOCK_LIST, LKB_DEADLOCKED, LKB_ON_LLB_WAIT
**	    flags to lkb_attribute.
**	    Added lkb_llb_wait for multiple waiting LKBs per LLB.
**	    Added lkb_dead_wait to maintain list of LKBs not yet
**	    deadlock checked, lkb_deadlock_wakeup to stamp the
**	    the deadlock cycle when this LKB was added to the list.
**	02-Aug-1999 (jenjo02)
**	    Added LKB_IS_RSBS attribute.
**	22-Oct-1999 (jenjo02)
**	    Shrunk lkb_grant_mode, lkb_request_mode, to i2's from u_i4's.
**	21-Aug-2001 (jenjo02)
**	    Added lkb_alloc_llb;
**	28-Feb-2002 (jenjo02)
**	    Renamed lkb_lkh_offset to lkbq_lkh, moved inside of lkb_llbq.
**      28-Dec-2003 (horda03) Bug 111461/INGSRV 2634
**          Made lkbq_next volatile.
**	22-Jun-2004 (jenjo02)
**	    Added LKB_RSB_IS_MUTEXED flag to tell LK_do_unlock the
**	    RSB mutexed is already held.
**      22-sep-2004 (fanch01)
**          Remove meaningless LKB_TEMPORARY flag.
**	01-Oct-2007 (jonj)
**	    Embed CBACK structure in LKB, remove LK_ON_CBACK_LIST
**	    attribute, unused.
**	23-Oct-2007 (jonj)
**	    Replace cback_lockid with cback_rsb_id, add cback_posts.
**	26-Oct-2007 (jonj)
**	    Added LKB_STALLED, replacing LKREQ_STALLED_? flags in LKREQ.
**	19-Nov-2007 (jonj)
**	    Add LOCK_MODE, LKB_STATE, LKB_ATTRIBUTE defines.
**	08-Jan-2009 (jonj)
**	    Eliminated LKREQ, integrating what's useful into the LKB
**	    itself.
*/
struct _LKB
{
    SIZE_TYPE	    lkb_q_next;		/* Next LKB on resource state queue. */
    SIZE_TYPE	    lkb_q_prev;		/* Previous LKB on resource state
					** queue. */
    i2		    lkb_type;		/* The type of block. */
#define	LKB_TYPE	1
    i2		    lkb_count;		/* Count of physical requests. */
    LK_ID	    lkb_id;		/* Identifier for this block. */
    i2    	    lkb_grant_mode;	/* Granted mode of lock. */
    i2     	    lkb_request_mode;	/* Requested mode of lock. */
/* String values for lock mode: */
#define	LOCK_MODE "\
N,IS,IX,S,SIX,U,X"

    i4	    	    lkb_state;		/* The state of the lock. */
#define LKB_GRANTED	1
#define LKB_CONVERT	2
#define LKB_WAITING	3
/* String values of above values: */
#define	LKB_STATE "\
,GR,CV,WT"


    i4	    	    lkb_attribute;	/* Lock attributes. */
#define	LKB_PHYSICAL		0x0001
#define	LKB_CANCEL		0x0004
#define	LKB_ENQWAIT		0x0008
#define	LKB_CONCANCEL		0x0010
#define	LKB_PROPAGATE		0x0020
#define LKB_ON_DLOCK_LIST	0x0040  /* LKB is on deadlock check queue */
#define LKB_DEADLOCKED		0x0080  /* LKB has deadlocked */
#define LKB_ON_LLB_WAIT		0x0100  /* LKB is on LLB's list of waiters */
#define LKB_STALLED		0x0200  /* LKB is stalled awaiting recovery */
#define LKB_DISTRIBUTED_LOCK    0x0400  /* LKB is for a distributed lock */
#define LKB_RSB_IS_MUTEXED	0x0800  /* LKB's RSB is mutexed */
#define LKB_END_OF_DL_CYCLE	0x1000  /* LKB's dist dlock cycle terminated */
#define LKB_VALADDR_PROVIDED	0x2000  /* Return lock value when resumed */
#define LKB_COLLECTED		0x4000  /* LKB state resolved after wait */
#define LKB_IS_RSBS		0x8000	/* LKB is embedded in RSB */
/* String values of above values: */
#define	LKB_ATTRIBUTE "\
PHYSICAL,2,CANCEL,ENQWAIT,CONCANCEL,PROP,ON_DLOCK_L,DLOCKED,ON_LLB_W,\
STALLED,DISTRIB,RSB_MUTEXED,EODLC,VALADDR,COLLECTED,IS_RSBS"

    struct _CBACK
    {
	SIZE_TYPE   cback_q_next;	/* Next CBACK on CBT queue */
	SIZE_TYPE   cback_cbt_id;       /* CBT on server where lock taken */
	LKcallback  cback_fcn;	        /* Callback Function */
	PTR         cback_arg;	        /* Callback argument */
	LK_ID	    cback_rsb_id;	/* RSB when cback established */
	i4	    cback_posts;	/* #posts to CBACK */	    
	i4	    cback_blocked_mode; /* Mode of lock being blocked */
    } 	    lkb_cback;
    struct _LKBQ
    {
	volatile SIZE_TYPE   lkbq_next;	/* Next LKB on LKB hash queue. */
	volatile SIZE_TYPE   lkbq_prev;	/* Previous LKB on LKB hash queue. */
	SIZE_TYPE   lkbq_rsb;		/* RSB that this LKB is queued on. */
	SIZE_TYPE   lkbq_llb;		/* LLB that this LKB is queued on. */
	SIZE_TYPE   lkbq_lkh;		/* Offset of owning LKH entry */
    }		    lkb_lkbq;
    LLBQ	    lkb_llbq;		/* LLB queue of LKBs */
    LLBQ	    lkb_llb_wait;	/* LLB queue of waiting LKBs */
    LLBQ	    lkb_dead_wait;	/* LKD queue of LKBs awaiting deadlock check */
    LLBQ	    lkb_stall_wait;	/* LKD queue of stalled requests */
    i4		    lkb_deadlock_wakeup; /* Deadlock thread wakeup tick */
    u_i4	    lkb_request_id;	/* unique request ID of LLB */
    CS_CPID    	    lkb_cpid;		/* Id of lock requestor and/or owner */
    SIZE_TYPE	    lkb_alloc_llb;	/* LLB that allocated this LKB */
    CX_REQ_CB	    lkb_cx_req;		/* Request block for DLM. */

    /* 
    ** The following are things that used to be in "LKREQ", now deprecated.
    **
    ** They contain stuff that might be set asynchronously while a lock request
    ** waits and are retrieved from the LKB by LK_get_completion_status()
    ** when the requestor's thread is resumed.
    */
    LK_VALUE	    lkb_value;		/* lock value when resumed */
    CS_CPID	    lkb_blocking_cpid;	/* Who's blocking this lock */
    i4		    lkb_blocking_mode;	/* ...and at what mode */
    STATUS	    lkb_status;		/* async result of lock request */
};


/*}
** Name: LKD - The Lock database.
**
** Description:
**      This structure contains all the global context required to manage the
**      lock database.  This information includes: the lock lists, the lock 
**      blocks, the resource blocks, the hash queues, etc.
**
**	Diagram of inportant pointers in the LKD.
**
** History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable logging and locking system.
**	24-may-1993 (bryanp)
**	    Define lkd_status bit "LKD_IN_CSP_COMPLETION" for mutex optimization
**	    Added synch_complete and asynch_complete statistics.
**	    Added csp_msgs_rcvd and csp_wakeups_sent statistics.
**	    Separated lkb's and rsb's into separate memory pools, primarily
**		to break the 64K limit on the sbk table which many customers
**		are hitting.
**	26-jul-1993 (bryanp)
**	    Replaced uses of "short" with "nat" and "i2" as appropriate.
**	09-feb-1995 (medji01)
**	    Added comment block
**	03-jan-1996 (duursma)
**	    Deleted unused field unsent_gdlck_search field.  Replaced it with
**	    new field walk_wait_for to count calls to walk_wait_for_graph().
**	06-Dec-1995 (jenjo02)
**	    Added distinct mutex for LKevents, lkd_ev_mutex.
**	    Defined new mutexes, one for each
**	    queue/list anchored in LKD. lkd_mutex is now only used to
**	    protect sensitive fields in the LKD itself.
**	03-jan-1996 (duursma)
**	    Deleted unused field unsent_gdlck_search field.  Replaced it with
**	    new field walk_wait_for to count calls to walk_wait_for_graph().
**	12-Jan-1996 (jenjo02)
**	    Moved lkd_stat.next_id out of stats, renaming it
**	    lkd_next_llbname. Unique LLB id is now assigned when 
**	    LLB is allocated under the protection of the lbk_mutex.
**	    Removed lkd_mutex, no longer needed.
**	14-May-1996 (jenjo02)
**	    Added lkd_dlock_mutex.
**	21-aug-1996 (boama01)
**	    Chgs to LKD struct for Cluster Support and AXP semaphores:
**	    - Added LKD_CSP_HOLD_NEW_RQSTS and LKD_CSP_HELD_NEW_RQST bits to
**	      lkd_status to hold up new AST-delivered requests during CSP
**	      recovery stalled-request completion.
**	    - ALPHA ONLY: Added lkd_axp_alignmt fld before first semaphore
**	      to get all sems on longword alignment.
**	08-Oct-1996 (jenjo02)
**	    Added lkd_mutex to block simultaneous updates of
**	    lkd_rqst_id.
**	01-aug-1996 (nanpr01 for jenjo02)
**	    Added lkd_cbt_q_mutex for CBT/CBACK.
**	08-Oct-1996 (jenjo02)
**	    Added lkd_mutex to block simultaneous updates of
**	    lkd_rqst_id.
**	15-Nov-1996 (jenjo02)
**	    Added deadlock wait queue in support of Deadlock Detector threadl
**	    also 2 new stats, dlock_wakeups and dlock_max_q_len.
**	25-Feb-1997 (jenjo02)
**	    Added lkd_ew_stamp.
**	11-Apr-1997 (jenjo02)
**	    Added new stats for callback threads.
**	    Changed stats to u_i4's.
**	    Removed lkd_cbt_next/prev, lkd_cbt_q_mutex.
**	02-Aug-1999 (jenjo02)
**	    Added lists_that_locked, rsb_alloc, lkb_alloc counters.
**	22-Oct-1999 (jenjo02)
**	    Deleted lkd_dlock_lock.
**	21-jun-2005 (devjo01)
**	    Add LKD_NO_DLM_GLC.
**	13-Mar-2006 (jenjo02)
**	    Deprecated lkd_rqst_id, lkd_ew_stamp.
**	16-Aug-2007 (jonj)
**	    Add LKD_PURGE_DLOCK_LIST
**	01-Oct-2007 (jonj)
**	    Deleted lkd_cback_free.
**	19-Nov-2007 (jonj)
**	    Add LKD_STATUS define
**	31-Jan-2008 (jonj)
**	    Delete unused lkd_ivb_seen.
**	    Add lkd_ldead_sid, lkd_ldead_running for
**	    distributed deadlock thread synchronicity.
**	04-Apr-2008 (jonj)
**	    Add LKD_NO_ULOCKS lkd_status, true if update locks
**	    not supported.
**	29-Aug-2008 (jonj)
**	    Relocate LKD_PURGE_DLOCK_LIST from lkd_status to
**	    its own variable to avoid corrupting concurrent
**	    fetches of lkd_status.
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: add lkd_tstat, stats by lock type.
*/
struct _LKD
{
    i4         	    lkd_extra;
    i4         	    lkd_extra1;
    i4         	    lkd_size;		/* The size of the control block. */
    i2              lkd_type;		/* The type of the control block. */
#define	LKD_TYPE	127
    i2	    	    lkd_subtype;	/* The subtype of this control block. */
#define	LKD_LOADER	1
#define	LKD_CODE	2
#define	LKD_LLB		3
#define	LKD_LLBS	4
#define	LKD_RSH		5
#define	LKD_LKH		6
#define	LKD_BLK		7
#define	LKD_BLKS	8
    i4         	    lkd_status;		/* Lock database status. */
#define	LKD_AST_ENABLE		0x0001L
#define	LKD_CLUSTER_SYSTEM	0x0002L
#define	LKD_CSP_RUNDOWN		0x0004L
#define	LKD_IN_CSP_COMPLETION	0x0008L
#define LKD_CSP_HOLD_NEW_RQSTS  0x0010L
#define LKD_CSP_HELD_NEW_RQST   0x0020L
#define LKD_DISTRIBUTE_DEADLOCK 0x0040L /* Don't use DLM for deadlock. */
#define LKD_NO_DLM_GLC          0x0080L /* DLM has no Group lock container. */
#define LKD_NO_ULOCKS   	0x0200L /* No LK_U lock support */
#define LKD_SHOW_DEADLOCKS	0x8000L /* Display deadlock info */
/* String values of above values: */
#define	LKD_STATUS "\
AST_ENABLE,CLUSTER,CSP_RUNDOWN,CSP_COMPL,HOLD_NEW,HELD_NEW,\
NO_DLM_GLC,100,NO_ULOCKS,400,800,1000,2000,4000,SHOW_DEAD"

    i4		    lkd_purge_dlock;	/* DIST dlock needs purge of ON_DLOCK */
    PID             lkd_csp_id;		/* the process ID of the CSP */

    i4         	    lkd_rsh_size;	/* Size of RSB hash table. */
    i4	    	    lkd_lkh_size;	/* Size of LKB hash table. */
    i4         	    lkd_max_lkb; 	/* Default maximum LKB per lock list. */
    i4         	    lkd_llb_inuse;	/* Active LLB entries. */
    i4         	    lkd_rsb_inuse;	/* Count of RSB's. */
    i4         	    lkd_lkb_inuse;	/* Count of LKB's. */
#ifdef VMS
#ifdef ALPHA
    char            lkd_axp_alignmt[2];	/* to align rest of sems on longs */
#endif
#endif
    LK_SEMAPHORE    lkd_lbk_mutex;	/* Mutex to protect LBK */
    i4         	    lkd_lbk_allocated;	/* num LBK's allocated */
    i4         	    lkd_lbk_count;	/* Allocated LBK entries. */
    i4         	    lkd_lbk_size;	/* Maximum LBK entries. */
    SIZE_TYPE	    lkd_lbk_free;	/* Offset to Next free LBK. */
    SIZE_TYPE	    lkd_lbk_table;	/* Offset to LBK offset table. */
    i4	    	    lkd_next_llbname;	/* Next unique LLB number. */
    LK_SEMAPHORE    lkd_sbk_mutex;	/* Mutex to protect SBK */
    i4         	    lkd_sbk_allocated;	/* num SBK's allocated */
    i4	    	    lkd_sbk_count;	/* Allocate SBK entries. */
    i4	    	    lkd_sbk_size;	/* Maximum SBK entries. */
    SIZE_TYPE	    lkd_sbk_free;	/* Offset to Next free SBK. */
    SIZE_TYPE	    lkd_sbk_table;	/* offset to SBK offset table. */
    LK_SEMAPHORE    lkd_rbk_mutex;	/* Mutex to protect RBK */
    i4         	    lkd_rbk_allocated;	/* num RBK's allocated */
    i4	    	    lkd_rbk_count;	/* Allocated RBK entries. */
    i4	    	    lkd_rbk_size;	/* Maximum RBK entries. */
    SIZE_TYPE	    lkd_rbk_free;	/* Offset to Next free RBK. */
    SIZE_TYPE	    lkd_cbt_free;	/* Offset to next free CBT */
    SIZE_TYPE	    lkd_cback_free;	/* Offset to next free CBACK */
    SIZE_TYPE	    lkd_rbk_table;	/* offset to RBK offset table. */
    /*
    ** Note: Each bucket in the LKH/RSH has it's own mutex
    */
    SIZE_TYPE	    lkd_lkh_table;	/* offset to LKH hash table. */
    SIZE_TYPE	    lkd_rsh_table;      /* offset to RSH hash table. */
    LK_SEMAPHORE    lkd_llb_q_mutex;	/* mutex to block LLB queue */
    SIZE_TYPE	    lkd_llb_next;	/* Largest LLB on queue. */
    SIZE_TYPE	    lkd_llb_prev;	/* Smallest LLB on queue. */
    LK_SEMAPHORE    lkd_dw_q_mutex;	/* mutex to latch deadlock wait queue */
    i4	    	    lkd_dw_q_count;	/* number of LLBs on the deadlock queue */
    SIZE_TYPE	    lkd_dw_next; 	/* Next LLB on deadlock wait queue. */
    SIZE_TYPE	    lkd_dw_prev; 	/* Last LLB on deadlock wait queue. */
    LK_SEMAPHORE    lkd_ew_q_mutex;	/* mutex to latch ew queue */
    SIZE_TYPE	    lkd_ew_next; 	/* Next LLB on event wait queue. */
    SIZE_TYPE	    lkd_ew_prev; 	/* Last LLB on event wait queue. */
    LK_SEMAPHORE    lkd_stall_q_mutex;	/* mutex to latch stall queue */
    SIZE_TYPE	    lkd_stall_next;	/* next lock on stall queue. */
    SIZE_TYPE	    lkd_stall_prev;	/* last lock on stall queue. */
    i4	    	    lkd_lock_stall;	/* type of lock stall: */
#define LK_STALL_ALL_BUT_CSP		1
#define	LK_STALL_LOGICAL_LOCKS		2
    i4	    	    lkd_node_fail;	/* cluster node failure indication */
    i4	    	    lkd_csid;		/* the cluster ID of the current node */
    i4	    	    lkd_dlk_handler;	/* Really a (void (*)()), this is used
					** only in the VMS Cluster version of
					** the locking system, where it
					** contains the CSP function which
					** performs deadlock searching.
					*/
    CS_SID	    lkd_ldead_sid;	/* SID of local LKdeadlock_thread() */
    CS_ASET	    lkd_ldead_running;	/* zero if it's asleep */
    struct
    {
      u_i4	    create_list; 	/* # of create list calls. */
      u_i4	    lists_that_locked;	/* # LLBs that actually set locks */
      u_i4	    rsb_alloc;		/* # RSB allocations */
      u_i4	    lkb_alloc;		/* # LKB allocations */
      u_i4	    release_partial;	/* # of partial releases. */
      u_i4	    release_all; 	/* # of release lists. */
      u_i4	    convert_search;	/* # of convert deadlock searchs. */
      u_i4	    deadlock_search;    /* # of deadlock searches performed. */
      u_i4	    cancel;		/* # of timeout cancel's. */
      u_i4	    enq; 		/* # of enq requested. */
      u_i4	    deq; 		/* # of deq requested. */
      u_i4	    gdlck_search;	/* # of pending global deadlock 
					** search requests. 
					*/
      u_i4	    gdeadlock;		/* # of global deadlock detected. */
      u_i4	    gdlck_grant; 	/* # of global locks grant before the
					** the global deadlock search.
					*/
      u_i4	    totl_gdlck_search;	/* # of global deadlock search requests. */
      u_i4	    gdlck_call;		/* # of global deadlock search calls. */
      u_i4	    gdlck_sent;		/* # of global deadlock messages sent. */
      u_i4	    cnt_gdlck_call;	/* # of continue deadlock search calls. */
      u_i4	    cnt_gdlck_sent;	/* # of continue deadlock message sent. */
      u_i4	    walk_wait_for;      /* # of calls to walk_wait_for_graph. */
      u_i4	    sent_all;		/* # of sent all deadlock search request. */
      u_i4	    synch_complete;	/* # of SS$_SYNCH completions. */
      u_i4	    asynch_complete;	/* # of enq_complete completions */
      u_i4	    csp_msgs_rcvd;	/* # of msgs received by CSP so far */
      u_i4	    csp_wakeups_sent;	/* # of LK_wakeup calls made by CSP */
      u_i4	    allocate_cb; 	/* # of allocate_cb calls */
      u_i4	    deallocate_cb;	/* # of deallocate_cb calls */
      u_i4	    sbk_highwater;	/* highwater mark of SBK allocation */
      u_i4	    lbk_highwater;	/* highwater mark of LBK allocation */
      u_i4	    rbk_highwater;	/* highwater mark of RBK allocation */
      u_i4	    rsb_highwater;	/* highwater mark of RSB usage */
      u_i4	    lkb_highwater;	/* highwater mark of LKB usage */
      u_i4	    llb_highwater;	/* highwater mark of LLB usage */
      u_i4	    max_lcl_dlk_srch;	/* max length of a deadlocksearch */
      u_i4	    dlock_locks_examined;/* number of lkb's seen by deadlock */
      u_i4	    max_rsrc_chain_len; /* max length of a RSB chain search */
      u_i4	    max_lock_chain_len; /* max length of a LKB chain search */
      u_i4	    dlock_wakeups;	/* number of deadlock thread wakeups */
      u_i4	    dlock_max_q_len;    /* max length of deadlock wait queue */
      u_i4	    cback_wakeups;	/* # of callback thread wakeups */
      u_i4	    cback_cbacks;	/* # of callback invocations */
      u_i4	    cback_stale;	/* # of stale or ignored callbacks */
      u_i4	    max_lkb_per_txn;	/* Max # of LKBs in a transaction */
    }		    lkd_stat;
    LK_TSTAT	    lkd_tstat[LK_MAX_TYPE+1];
};

/*}
** Name: LKH - Lock Hash Table Entry
**
** Description:
**
** History:
**	18-jan-1995 (medji01)
**	    Mutex Granularity Project
**		- Added definition
*/
struct _LKH  
{
    SIZE_TYPE       lkh_lkbq_next;	/* First LKBQ in chain */
    SIZE_TYPE       lkh_lkbq_prev;	/* Prior LKBQ in chain (always zero) */
    i2              lkh_type;		/* The type of block */
#define	LKH_TYPE	4
    i2              lkh_reserved;	/* Padding */
    LK_SEMAPHORE    lkh_mutex;		/* LKH semaphore */
};

/*}
** Name: LLB - Lock list block.
**
** Description:
**      This block contains information about a lock list.
**
**      If the llb_status specifies LLB_PARENT_SHARED, then the lock list
**      should be used as a pointer to a shared lock list (given by
**      llb_shared_llb).  All requests made on the lock list should be
**      handled as requests on the shared lock list.  No locks should actually
**      be held on the parent list.
**
**      If the llb_status specifies LLB_SHARED, then the lock list is a
**      shared lock list.  There must be at least one LLB_PARENT_SHARED type
**      lock list that points to this list through the llb_shared_llb pointer.
**      When the last parent list is released, then this list and its locks
**      are released.
**
** History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable logging and locking system.
**	24-may-1993 (bryanp)
**	    Remove llb_async_value since this technique
**		didn't work correctly for LK_MULTITHREAD lock lists.
**	26-jul-1993 (bryanp)
**	    Replaced "short" with "nat" or "i2" as appropriate.
**	08-sep-93 (swm)
**	    Changed type of session id. from i4  to CS_SID to match recent
**	    CL interface revision.
**	21-aug-1995 (dougb) bug #70696
**	    Add new LLB fields required in LKdeadlock() in fix for bug #70696.
**	14-May-1996 (jenjo02)
**	    Added llb_deadlock_rsb.
**	03-Oct-1996 (jenjo02)
**	    Added llb_deadlock_llb.
**	15-Nov-1996 (jenjo02)
**	    Removed llb_deadlock_rsb, llb_deadlock_llb.
**	16-Nov-1996 (jenjo02)
**	    Added LLB_ON_DLOCK_LIST, LLB_DEADLOCK status bits,
**	    llb_deadlock_wakeup.
**	25-Feb-1997 (jenjo02)
**	    Replaced unused llb_timeout with llb_ew_stamp.
**	20-Jul-1998 (jenjo02)
**	    llb_event_type, llb_event_value changed from 
**	    i4 to LK_EVENT structure.
**	26-Jan-1999 (jenjo02)
**	    Removed LLB_ON_DLOCK_LIST, LLB_DEADLOCK. Those attributes
**	    are now carried in individual LKBs.
**	    Removed llb_deadlock_wakeup, replaced llb_wait with
**	    llb_lkb_wait (list of waiting LKBs).
**	02-Aug-1999 (jenjo02)
**	    RSB/LKB stashes. Added llb_lkb_stash, llb_rsb_stash, llb_lkb_alloc,
**	    llb_rsb_alloc.
**      20-Aug-1999 (hanal04) Bug 97316 INGSRV921.
**          Change llb_lkb_count from i2 to i4 so that we can take
**          a greater number of row level locks before wrapping round
**          and hitting errors. Ensure next_llb_lkb_count in lkrqst.c
**          is of the same type.
**          Also added pad to ensure byte alignment.
**	30-Nov-1999 (jenjo02)
**	    Added DB_DIS_TRAN_ID llb_dis_tran_id to LLB for shared
**	    XA lock lists.
**	15-Dec-1999 (jenjo02)
**	    Removed llb_dis_tran_id. Mechanics of sharing pushed up
**	    into LG.
**	28-Feb-2002 (jenjo02)
**	    Added LLB_MULTITHREAD. It's now an attribute of a 
**	    lock list rather than only a run-time flag.
**	    Renamed llb_wait_count to llb_waiters. "LLB_WAITING" is
**	    now contrived based on llb_wait_count and is never set
**	    in llb_status.
**	11-Oct-1999 (jenjo02)
**	    llb_event_type, llb_event_value changed from 
**	    nat to LK_EVENT structure.
**      24-Jan-2003 (horda03) Bug 109528
**          Added llb_status_mutex to protect updates to the llb_status.
**	14-Sep-2004 (sheco02)
**	    Back out x-integration change 471218
**	13-Mar-2006 (jenjo02)
**	    Deprecated llb_ew_stamp, changed llb_stamp from i4 to u_i4.
**	16-Aug-2007 (jonj)
**	    Replaced unused llb_deadlock_current, llb_deadlock_next,
**	    llb_deadlock_maxrqst with llb_dlm_msg_search, llb_dlm_node,
**	    llb_dlm_spare.
**	19-Nov-2007 (jonj)
**	    Add LLB_STATUS define.
**	31-Jan-2008 (jonj)
**	    Replace llb_dlm_msg_search, llb_dlm_node,
**	    llb_dlm_spare with cluster-only 
**	    llb_dist_stamp array.
**      16-Jun-2009 (hanal04) Bug 122117
**          Added LLB_FORCEABORT.
*/
struct _LLB
{
    SIZE_TYPE	    llb_q_next;		/* The next active LLB. */
    SIZE_TYPE	    llb_q_prev;		/* The previous active LLB. */
    i2		    llb_type;		/* Type of block. */
#define	LLB_TYPE	3
    i2              llb_pad;            /* pad to ensure i4s are byte
                                        ** aligned */
    i4         	    llb_lkb_count;	/* The count of locks on this list. */
    LLB_ID	    llb_id;	    	/* Lock list identifier. */
    i4         	    llb_status;		/* Lock list status information. */
					/* These values are currently viewed
					** by IPM and are hard-coded into the
					** ipm code (temporarily).  Until this
					** changes, the values of the following
					** LLB constants should not be changed
					** without communicating the changes
					** to the IPM owners. */
#define LLB_WAITING	    0x00001L
#define	LLB_NONPROTECT	    0x00002L
#define	LLB_ORPHANED	    0x00004L
#define	LLB_EWAIT	    0x00008L
#define	LLB_RECOVER	    0x00010L
#define	LLB_MASTER	    0x00020L
#define	LLB_ESET	    0x00040L
#define	LLB_EDONE	    0x00080L
#define	LLB_NOINTERRUPT	    0x00100L
#define LLB_SHARED          0x00200L
#define LLB_PARENT_SHARED   0x00400L
#define	LLB_ENQWAITING	    0x00800L
#define	LLB_GDLK_SRCH	    0x01000L
#define	LLB_ST_ENQ	    0x02000L
#define	LLB_MASTER_CSP	    0x04000L
#define LLB_ALLOC	    0x08000L
#define LLB_XSVDB           0x20000L    /* X SV_DATABASE lock on this llb */
#define LLB_XROW            0x40000L    /* X ROW lock on this llb         */
#define LLB_MULTITHREAD     0x80000L    /* Multithreaded lock list */
#define LLB_CANCELLED_EVENT 0x100000L    /* LK_event() wait cancelled      */
#define LLB_FORCEABORT      0x200000L   /* Session has been chosen for
                                        ** force-abort but hasn't noticed it 
                                        ** yet; any lock waits should not wait 
                                        ** but rather return with LK_INTR_FA.
                                        ** This prevents undetected log/lock 
                                        ** deadlocks. 
                                        */
/* String values of above values: */
#define	LLB_STATUS "\
WAIT,NONPROTECT,ORPHAN,EWAIT,RECOVER,MASTER,ESET,EDONE,\
NOINTERRUPT,SHARED,PSHARED,ENQWAIT,GDLOCK_SRCH,STALL_ENQ,\
MASTER_CSP,ALLOC,10000,XSVDB,XROW,MULTITHREAD,CANC_EVENT,FORCEABORT"

    i4		    llb_llkb_count;	/* The count of logical locks on
					** this list. */
    i4		    llb_max_lkb;	/* The maximum lkbs allowed on this 
					** list. */
    LLBQ	    llb_llbq;		/* Queue of LKB's for this LLB. */
    LLBQ	    llb_ev_wait;	/* queue of LLB's waiting for an event */
    SIZE_TYPE	    llb_related_llb;	/* The related lock list. */
    i4	    	    llb_related_count;	/* The count of related LLB's. */
    i4	    	    llb_name[2];	/* The unique lock list identifier. */
    u_i4    	    llb_search_count;	/* Last local deadlock search for this list. */
    LLBQ	    llb_lkb_wait;	/* Queue of waiting LKBs */
    i4	    	    llb_waiters;	/* Number of LKBs on llb_lkb_wait */
    CS_CPID    	    llb_cpid;		/* session identifier of LLB creator */
    i4	    	    llb_async_status;	/* used to convey LK_NOLOCKS back. */
    SIZE_TYPE	    llb_shared_llb;	/* Pointer to shared lock list - used
                                        ** only if status LLB_PARENT_SHARED */
    i4	    	    llb_connect_count;  /* Number of llb's that reference this
                                        ** list through llb_shared_llb - used
                                        ** only if status LLB_SHARED */
    LK_EVENT	    llb_event;		/* type and value of LKevent which
					** s being waited on. */
    u_i4	    llb_evflags;	/* zero or LK_CROSS_PROCESS_EVENT */
#define	LK_CROSS_PROCESS_EVENT  0x0001
    u_i4    	    llb_stamp;		/* The stamp of the current lock request. */
    SIZE_TYPE	    llb_tick;		/* The current clock tick when making
					** the global deadlock search request.
					** ??? This is never updated.
					*/
    LLB		    *llb_next_mod;	/*
					** Next pointer in a queue maintained
					** by LKdeadlock() so that it can
					** quickly return to lock lists
					** modified during its search.
					*/
    u_i4    	    llb_gsearch_count;  /*
					** Last global-local deadlock search
					** on this lock list.
					*/
    SIZE_TYPE	    llb_lkb_stash;	/* Stash of preallocated LKBs */
    SIZE_TYPE	    llb_rsb_stash;	/* Stash of preallocated RSBs */
    i4	    	    llb_lkb_alloc;	/* Number of LKBs allocated */
    i4	    	    llb_rsb_alloc;	/* Number of RSBs allocated */
    LK_SEMAPHORE    llb_mutex;		/* LLB semaphore */
#if defined(conf_CLUSTER_BUILD)
#define	LLB_MAX_STAMPS_PER_NODE	8 	/* this is really arbitrary... */
    u_i4	    llb_dist_stamp[CX_MAX_NODES][LLB_MAX_STAMPS_PER_NODE];
					/* The last n cluster deadlock
					** checks by node on this 
					** lock list.
					*/
#endif /* CLUSTER_BUILD */
}; 

/*}
** Name: RSB - Resource Block.
**
** Description:
**      A resource block contains information about a resource being locked.
**      There is one resource block for every unique resource identifier, all
**      lock requests for this resource have their lock blocks queued from
**      here.  The resource also contains the current value of the resource.
**
** History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable logging and locking system.
**	24-may-1993 (bryanp)
**	    Removed resource block padding now that resource blocks are managed
**	    from a different memory table than lock blocks. RSB size no longer
**	    must exceed lock block size.
**	26-jul-1993 (bryanp)
**	    Replaced "short" with "nat" or "i2" as appropriate.
**	08-Jan-1996 (jenjo02)
**	    Added rsb_dlock, set in deadlock() when rsb_mutex is
**	    temporarily released. While set, no other thread may
**	    acquire the RSB mutex.
**      01-aug-1996 (nanpr01 for ICL keving)
**          Add rsb_cback_count. Stores # of LKBs with callback currently
**          on granted queue.
**	15-Nov-1996 (jenjo02)
**	    Removed rsb_dlock, rsb_dlock_mutex.
**	    Added rsb_waiters, rsb_converters counts to RSB.
**	22-Oct-1999 (jenjo02)
**	    Shrunk rsb_grant_mode, rsb_convert_mode from u_i4's to u_i2's.
**	    Added rsb_dlock_depth.
**	21-Aug-2001 (jenjo02)
**	    Added rsb_alloc_llb;
**	08-may-2001 (devjo01)
**	    Shrunk rsb_waiters & rsb_converters to i2 to force rsb_lkb
**	    to meet strictest alignment requirements (8 bytes on axp_osf).
**	31-Jan-2007 (jon)
**	    Add RSB_MUTEXED_LOCL/DIST defines.
*/
struct _RSB
{
    SIZE_TYPE	    rsb_q_next;		/* Next RSB on resource name hash
					** queue. */
    SIZE_TYPE	    rsb_q_prev;		/* Previous RSB on resource name hash
					** queue. */
    i2		    rsb_type;		/* Type of block. */
#define	RSB_TYPE	2
    i2	    	    rsb_invalid;	/* Indicate whether the rsb_value is
					** invalid. */
    i2    	    rsb_grant_mode;	/* The summary grant mode of all granted
					** and converting locks. */
    i2     	    rsb_convert_mode;	/* The grant mode of the first lock
					** waiting to convert. */
    LK_ID	    rsb_id;		/* Identifier for block. */
    SIZE_TYPE	    rsb_grant_q_next;	/* Head of the queue of granted or
					** converting locks. */
    SIZE_TYPE	    rsb_grant_q_prev;
    i4         	    rsb_cback_count;    /* # of LKBs on grant q with a cback */
    SIZE_TYPE	    rsb_wait_q_next;	/* Head of the queue of waiting locks. */
    SIZE_TYPE	    rsb_wait_q_prev;
    i2		    rsb_waiters;	/* Number of waiting locks */
    i2		    rsb_converters;	/* Number of converting locks */
    LK_LOCK_KEY     rsb_name;		/* Resouce name */
    i4	    	    rsb_value[2];	/* The Ingres value associated with the
					** resource. */
    SIZE_TYPE	    rsb_rsh_offset;	/* Offset of owning RSH engry */
#define		RSB_MUTEXED_LOCL	0x10000	/* Mutexed by LKdeadlock_thread */
#define		RSB_MUTEXED_DIST	0x20000	/* Mutexed by LKdist_deadlock_thread */
    i4	    	    rsb_dlock_depth;    /* Owner and level of deadlock() that mutexed RSB */
    SIZE_TYPE	    rsb_alloc_llb;	/* LLB that allocated this RSB */
    LKB		    rsb_lkb;		/* Embedded LKB */
    LK_SEMAPHORE    rsb_mutex;		/* RSB semaphore */

};

/*}
** Name: RSH - Resource Hash Table Entry
**
** Description:
**      This control block anchors the resource synonym chain.
**
** History:
**	18-jan-1995 (medji01)
**	    Mutex Granularity Project
**		- Added definition
**	04-Apr-2008 (jonj)
**	    Made rsh_rsb_next volatile.
*/
struct _RSH  
{
    volatile SIZE_TYPE rsh_rsb_next;	/* First RSB in chain */
    SIZE_TYPE       rsh_rsb_prev;	/* Prior RSB in chain (always zero) */
    i2              rsh_type;		/* The type of block */
#define	RSH_TYPE	5
    i2              rsh_reserved;	/* padding */
    LK_SEMAPHORE    rsh_mutex;		/* RSH semaphore */
};

/*}  
** Name: CBT - Callback Thread descriptor. 
**  
** Description:
**      Each blocking callback thread allocates a CBT and hangs it off the lkd.
**      It holds the thread's process and session ids. It also holds the 
**      'triggered' flag which indicates to other threads whether they need to 
**      CSresume the callback thread.
**  
** History: 
**      01-aug-1996 (nanpr01 for ICL keving)
**          Created.
**	01-aug-1996 (nanpr01 for jenjo02)
**	    Added cbt_mutex.
**	11-Apr-1997 (jenjo02)
**	    Added new stats for callback threads.
**      29-aug-2002 (devjo01)
**          Remove depreciated fields 'cbt_cback_prev', and 'cbt_mutex'.
**          Renamed 'cbt_cback_next' to 'cbt_cback_list'.  CBACKs
**          are now maintained as a singly linked list updated using
**          Atomic Compare & Swap routines, rather than mutex protection,
**          so as to allow list updates from Signal Handler/AST level.
**	01-Oct-2007 (jonj)
**	    Delete unused cbt_cback_count.
*/ 
struct _CBT 
{ 
    i4         	    cbt_q_next; /* Next callback thread descriptor. */
    i4         	    cbt_q_prev; /* Previous */
    i2              cbt_type;           /* Type of block. */
#define	CBT_TYPE	11
    i2              cbt_reserved;       /* Padding */
    CS_CPID	    cbt_cpid;		/* Callback thread's session id */
    i4              cbt_triggered;      /* False if cbt is suspended waiting */
                                        /* for work */ 
    i4              cbt_cback_list;     /* callbacks to be run */
    struct
    {
      u_i4     	    wakeups;        	/* # of times this thread has been awoken*/  
      u_i4     	    cbacks;         	/* # of callbacks executed */
      u_i4     	    stale;          	/* # of stale callbacks */
    } 		    cbt_stat;
#if !defined(conf_CAS_ENABLED)
    LK_SEMAPHORE    cbt_mutex;          /* mutex to lock down CBT */
#endif
};

/*}
** Name: LK_DSM_CONTEXT - Deadlock search message context.
**
** Description:
**	In certain clustered environments (i.e. VMS), the DLM cannot
**	treat locks held by one process as belonging to separate
**	transactions, and can not therefore do the work of deadlock
**	checking.  In this case, we do our own distributed deadlock
**	checks.  Each RCP in the cluster establishes two LK_DSM_CONTEXT
**	instances for communicating TO the other nodes.  One is used
**	by LKdeadlock() to initiate a distributed deadlock search,
**	while the other is used by LKdeadlock_continue for the messages
**	generated when processing incoming LK_DSM messages.
**
**	As this is only accessed by the RCP, it is not allocated out
**	of LGK memory.
**
** History:
**      05-may-2005 (fanch01/devjo01)
**          Created. 
**	04-Jan-2007 (jonj)
**	    Add lk_dsm_ctx_msg_seq for debugging.
**	20-Aug-2009 (drivi01)
**	    Prototyped LK_rundown.
*/ 
typedef struct _LK_DSM_CONTEXT_PRIV
{
    i4		lk_dsm_ctx_head;	/* current read/write position */
    i4		lk_dsm_ctx_tail;	/* trailing guard */
    i4		lk_dsm_ctx_mark;	/* speculative write position */
    i4		lk_dsm_ctx_handle_alloc;/* true if transport allocated */
    i4		lk_dsm_ctx_active;	/* active since last flush? */
    u_i4	lk_dsm_ctx_msg_seq;	/* Message sequence */
    CX_PPP_CTX  lk_dsm_ctx_handle;	/* underlying transport handle */
    LK_DSM     *lk_dsm_ctx_ring;	/* dsm buffer */
} LK_DSM_CONTEXT_PRIV;

/*
**
** Public DSM context structure.  Client uses this to access
** DSM transport.
**
*/
#define LK_DSM_CONTEXT_MAGIC 0xe7a1
#define LK_DSM_CONTEXT_NELEM 1000

typedef struct _LK_DSM_CONTEXT 
{
    i4		lk_dsm_magic;		/* explicitly deallocated if != MAGIC */
    LK_DSM_CONTEXT_PRIV	lk_dsm_ctx_priv[CX_MAX_NODES];/* mngmnt structure */
    CX_CONFIGURATION	*cx_configuration;	/* CX node configuration */
} LK_DSM_CONTEXT;


FUNC_EXTERN	VOID	LK_dump_stall_queue(
                            char 	*message);

FUNC_EXTERN	STATUS	LK_meminit(
                            CL_ERR_DESC *sys_err);

FUNC_EXTERN	PTR	LK_allocate_cb(
			    i4  	type,
			    LLB		*l);
FUNC_EXTERN	void	LK_deallocate_cb(
			    i4 	type, 
			    PTR 	cb,
			    LLB		*l);

/* In LKinit.c */
FUNC_EXTERN	VOID	LK_rundown(
			    i4 pid);

/* In LKmutex.c */
FUNC_EXTERN	STATUS	LK_imutex(
			    LK_SEMAPHORE *amutex, 
			    char 	*mutex_name);
FUNC_EXTERN	STATUS	LK_mutex_fcn(
			    i4		 mode,
			    LK_SEMAPHORE *amutex,
			    char	 *file,
			    i4		 line);
FUNC_EXTERN	STATUS	LK_unmutex_fcn(
			    LK_SEMAPHORE *amutex,
			    char	 *file,
			    i4		 line);
FUNC_EXTERN	STATUS	LK_get_critical_sems(
			    void);
FUNC_EXTERN	STATUS	LK_rel_critical_sems(
			    void);
FUNC_EXTERN void	LKintrack(void);
FUNC_EXTERN void	LKintrcvd(void);

/* Functions in LKRQST.C: */
FUNC_EXTERN     STATUS  LK_try_wait(
                            RSB **rsb, 
                            i4  lock_mode);       
FUNC_EXTERN     STATUS  LK_try_convert(
                            RSB **rsb, 
                            i4  new_group_mode);
FUNC_EXTERN	LKB *   LK_local_deadlock(LKB *current_lkb, LKB *top_lkb,
                            i4 *max_request_mode, LK_DSM *inlkdsm, 
			    LK_DSM_CONTEXT *lkdsm_context, i4 depth,
			    i4 *must_retry);

/* Possible values returned to "must_retry", above */
#define		LKMR_MUTEX_CONFLICT	0x0001 	/* RSB mutex conflict */
#define		LKMR_TOP_POSTED		0x0002 	/* top_lkb async posted */
#define		LKMR_RECHECK		0x0004 	/* Rerun LOCL check using 
						** Cluster mode returned in 
						** "max_request_mode" */
#define		LKMR_STATE_CHANGE	0x0008 	/* LKB state changed after
						** RSB mutex acquired */

/* Functions in LKRLSE.C: */
FUNC_EXTERN	STATUS	LK_cancel(
			    LK_LLID	lock_list_id,
			    SIZE_TYPE	*lkb_to_cancel,
			    CL_ERR_DESC	*sys_err);
FUNC_EXTERN	STATUS	LK_do_unlock(
			    LKB 	*lkb, 
			    LK_VALUE 	*value, 
			    i4 		flag,
			    bool	llbIsMutexed);
FUNC_EXTERN	STATUS	LK_do_cancel(
			    SIZE_TYPE 	*lkb_to_cancel);

/* Functions in LKEVENT.C: */
FUNC_EXTERN	STATUS  LK_cancel_event_wait(
			    LLB 	*llb, 
			    LKD 	*lkd,
			    bool	IsRundown,
			    CL_ERR_DESC *sys_err);
FUNC_EXTERN	STATUS  LK_event(
			    i4		flag,
			    LK_LLID	lock_list_id,
			    LK_EVENT	*event,
			    CL_ERR_DESC *sys_err);
FUNC_EXTERN	VOID	LK_resume(
			    LKB 	*lkb);
FUNC_EXTERN	VOID	LK_suspend(
			    LKB		*lkb,
			    i4		flags);
FUNC_EXTERN	i4	LK_llb_on_list(
			    LLB 	*llb, 
			    LKD 	*lkd);

/* Functions in LKMO.C: */
FUNC_EXTERN	STATUS	LKmo_attach_lk(void);

/* Functions in LKCBACK.C: */
FUNC_EXTERN	STATUS 		lk_cback_put(
				    i4		blocked_mode,
				    LKB		*lkb);
FUNC_EXTERN	STATUS 		lk_cback_fire(
				    RSB		*rsb,
				    i4		blocked_lock_mode);
FUNC_EXTERN     STATUS          lk_cback_post(
                                    LKB         *lkb,
                                    bool        from_AST );

#if defined(conf_CLUSTER_BUILD)
#define LK_TRACE_CALLBACK_ENABLED
#if defined(VMS)
#define LKDEADLOCK_DEBUG
#endif
#endif

# if defined(LK_TRACE_CALLBACK_ENABLED)
FUNC_EXTERN     void lk_callback_do_trace(i4, LK_LOCK_KEY *);
#  define       LK_CALLBACK_TRACE( loc, key ) \
                 lk_callback_do_trace((loc),(key));
# else
#  define       LK_CALLBACK_TRACE( loc, key )
# endif /* defined(LK_TRACE_CALLBACK_ENABLED) */

/* For VMS Cluster deadlock debugging: */
#if defined(LKDEADLOCK_DEBUG)
/* Function in lkdlock.c */
FUNC_EXTERN	void Lk_dlock_trace( char *fmt, ... );

/* GLOBALDEFs in lkdlock.c */
GLOBALREF	bool	Lk_cluster_deadlock_debug;

#define		LK_DLOCK_TRACE \
    if ( Lk_cluster_deadlock_debug ) \
	Lk_dlock_trace

#else
#define		LK_DLOCK_TRACE	if (0)

#endif /* defined(LKDEADLOCK_DEBUG) */


/* Functions in LKSIZE.C: */
FUNC_EXTERN	VOID		LK_size(i4 	    object,
					SIZE_TYPE   *size);

/* Functions in LKDLOCK.C: */
FUNC_EXTERN	STATUS LK_alloc_dsm_context( LK_DSM_CONTEXT **hcontext );
FUNC_EXTERN	STATUS LK_dealloc_dsm_context( LK_DSM_CONTEXT **hcontext );
FUNC_EXTERN	STATUS LK_mark_dsm_context( LK_DSM_CONTEXT *pcontext );
FUNC_EXTERN	STATUS LK_unmark_dsm_context( LK_DSM_CONTEXT *pcontext );
FUNC_EXTERN	STATUS LK_flush_dsm_context( LK_DSM *inlkdsm,
					     LK_DSM_CONTEXT *pcontext,
					     u_i4 *writes );
FUNC_EXTERN	LK_DSM *LK_get_free_dsm( LK_DSM_CONTEXT *pcontext, i4 node );
FUNC_EXTERN	VOID	LK_get_next_dsm( LK_DSM_CONTEXT *dsmc, i4 node,
				LK_DSM **dsm, i4 *dsm_num );


/*
** LK internal error messages. These messages are defined in ERDMF.MSG and
** are issued by LK code.
*/
#define	    E_DMA000_LK_LLB_TOO_MANY		(E_DM_MASK | 0xA000L)
#define	    E_DMA001_LK_BLK_TOO_MANY		(E_DM_MASK | 0xA001L)
#define	    E_DMA002_LK_LKH_TOO_MANY		(E_DM_MASK | 0xA002L)
#define	    E_DMA003_LK_RSH_TOO_MANY		(E_DM_MASK | 0xA003L)
#define	    E_DMA004_LK_BAD_CSPID		(E_DM_MASK | 0xA004L)
#define	    E_DMA005_LK_BAD_CSPID_VAL		(E_DM_MASK | 0xA005L)
#define	    E_DMA006_LK_NODE_BADPARAM		(E_DM_MASK | 0xA006L)
#define	    E_DMA007_LK_NODE_BADPARAM		(E_DM_MASK | 0xA007L)
#define	    E_DMA008_LK_NODE_BADSELF		(E_DM_MASK | 0xA008L)
#define	    E_DMA009_LK_MUTEX			(E_DM_MASK | 0xA009L)
#define	    E_DMA00A_LK_UNMUTEX			(E_DM_MASK | 0xA00AL)
#define	    E_DMA00B_LK_IMUTEX			(E_DM_MASK | 0xA00BL)
#define	    E_DMA00C_LK_RMUTEX			(E_DM_MASK | 0xA00CL)
#define	    E_DMA00D_TOO_MANY_LOG_LOCKS		(E_DM_MASK | 0xA00DL)
#define	    E_DMA00E_LK_NO_RSBS			(E_DM_MASK | 0xA00EL)
#define	    E_DMA00F_LK_NO_LKBS			(E_DM_MASK | 0xA00FL)
#define	    E_DMA010_LK_NO_LKBS			(E_DM_MASK | 0xA010L)
#define	    E_DMA011_LK_NO_LLBS			(E_DM_MASK | 0xA011L)
#define	    E_DMA012_LK_NO_LLBS			(E_DM_MASK | 0xA012L)
#define	    E_DMA013_LK_BAD_INIT		(E_DM_MASK | 0xA013L)
#define	    E_DMA014_LK_INIT_MO_ERROR		(E_DM_MASK | 0xA014L)
#define	    E_DMA015_LKEVENT_SYNC_ERROR		(E_DM_MASK | 0xA015L)
#define	    E_DMA016_LKEVENT_LIST_ERROR		(E_DM_MASK | 0xA016L)
#define	    E_DMA017_LK_MUTEX_RELEASED		(E_DM_MASK | 0xA017L)
#define	    E_DMA018_LK_LLB_TOO_FEW		(E_DM_MASK | 0xA018L)
#define	    E_DMA019_LK_BLK_TOO_FEW		(E_DM_MASK | 0xA019L)
#define	    E_DMA020_LK_LKH_TOO_FEW		(E_DM_MASK | 0xA020L)
#define	    E_DMA021_LK_RSH_TOO_FEW		(E_DM_MASK | 0xA021L)
#define	    E_DMA022_LK_RSB_TOO_MANY		(E_DM_MASK | 0xA022L)
#define	    E_DMA023_LK_RSB_TOO_FEW		(E_DM_MASK | 0xA023L)
#define	    E_DMA024_LK_EXPAND_LIST_FAILED	(E_DM_MASK | 0xA024L)
#define	    E_DMA025_LK_NODE_ENQERR		(E_DM_MASK | 0xA025L)
#define	    E_DMA026_LK_NODE_BADNODE		(E_DM_MASK | 0xA026L)
#define	    E_DMA027_LK_NODE_BADRLSE		(E_DM_MASK | 0xA027L)
#define	    E_DMA028_LK_NODE_DEQERR		(E_DM_MASK | 0xA028L)
#define	    E_DMA029_LKALTER_SYNC_ERROR		(E_DM_MASK | 0xA029L)
#define	    E_DMA02A_LKALTER_NEGATIVE_ARG	(E_DM_MASK | 0xA02AL)
#define	    E_DMA02B_LKALTER_ARG_SIZE_ERR	(E_DM_MASK | 0xA02BL)
#define	    E_DMA02C_LKALTER_MISSING_ARG	(E_DM_MASK | 0xA02CL)
#define	    E_DMA02D_LKCLIST_SYNC_ERROR		(E_DM_MASK | 0xA02DL)
#define	    E_DMA02E_LKRLSE_SYNC_ERROR		(E_DM_MASK | 0xA02EL)
#define	    E_DMA02F_LKRQST_SYNC_ERROR		(E_DM_MASK | 0xA02FL)
#define	    E_DMA030_LK_SHOW_BAD_PARAM		(E_DM_MASK | 0xA030L)
#define	    E_DMA031_LK_SHOW_BAD_PARAM		(E_DM_MASK | 0xA031L)
#define	    E_DMA032_LK_SHOW_BAD_PARAM		(E_DM_MASK | 0xA032L)
#define	    E_DMA033_LK_SHOW_NO_REL_LIST	(E_DM_MASK | 0xA033L)
#define	    E_DMA034_LK_SHOW_NO_SHR_LIST	(E_DM_MASK | 0xA034L)
#define	    E_DMA035_LK_SHOW_NO_LOCK_KEY	(E_DM_MASK | 0xA035L)
#define     E_DMA036_LK_PROP_NO_LOCK_KEY        (E_DM_MASK | 0xA036L)
#define     E_DMA037_LK_PROP_NO_RSB             (E_DM_MASK | 0xA037L)
#define     E_DMA038_LK_PROP_BAD_RSB            (E_DM_MASK | 0xA038L)
#define     E_DMA039_LK_HARD_LOCK_LIMIT         (E_DM_MASK | 0xA039L)
