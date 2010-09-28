/*
** Copyright (c) 1986, 2009 Ingres Corporation
**
NO_OPTIM=dr6_us5 nc4_us5 ris_u64 i64_aix rs4_us5 r64_us5 i64_hpu
*/

#include    <compat.h>
#include    <gl.h>
#include    <nm.h>
#include    <cv.h>
#include    <cs.h>
#include    <di.h>
#include    <er.h>
#include    <me.h>
#include    <pc.h>
#include    <st.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <tr.h>
#include    <ulf.h>
#include    <dmf.h>
#include    <dm.h>
#include    <dmccb.h>
#include    <lk.h>
#include    <cx.h>
#include    <lg.h>
#include    <csp.h>
#include    <dmfcsp.h>
#include    <lkdef.h>
#include    <lgkdef.h>
#include    <lgclustr.h>

/* New function for tracing LK requests; under compile directive: */

#ifdef LK_TRACE
#include    <st.h>
static i4  LK_rqst_trace();
#endif


/**
**
**  Name: LKRQST.C - Implements the LKrequest function of the locking system
**
**  Description:
**	This module contains the code which implements LKrequest.
**	
**          LKrequestWithCallback -- request or convert a lock on a resource.
**          LKrequest -- wrapper to LKrequestWithCallback. Just passes a NULL
**                       callback function.
**	    LKdeadlock_thread -- support for optimistic deadlock detection.
**		         Please note that over the years deadlock has been
**			 extensively reworked, and that major routines have
**			 been renamed more than once.  Please be attentive
**			 when reading the change history.  This routine
**			 in is lkrqst.c rather than lkdlock.c because at one
**			 point deadlock detection was done at the time
**			 lock was moved onto the wait queue as part of a 
**			 blocked request.  We should consider moving all the
**			 deadlock routines to lkdlock.c.
**
**  History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable logging and locking system.
**	17-nov-1992 (bryanp)
**	    Return LK_BUSY, not LK_NOTGRANTED, when nowait lock would wait.
**	18-jan-1993 (bryanp)
**	    Call CL_CLEAR_ERR to clear the sys_err.
**	16-feb-1993 (andys)
**	    If we set LLB_WAITING, set llb_wait also.
**	10-mar-1993 (andys)
**	    Call LK_suspend in the cluster case in LK_request_lock.
**	    Correct arguments to another call to LK_suspend. 
**	    Set status before calling LK_suspend in LK_request_lock.
**	    Check for SS$_EXENQLM and map to LK_NOLOCKS in LKrequest.
**	    Only initialize lkb_attribute once in LK_request_lock.
**	    Add diagnostics behind #ifdef CSP_DEBUG.
**	24-may-1993 (bryanp)
**	    Mutex optimizations: Drop the mutex before calling LK_csp_send or
**		LK_wakeup, so that we're not blocked in a system call while
**		holding the mutex.
**	    Fix LK_get_completion_status() problems with shared lock lists.
**	    Make sure we pass lkreq LK_wakeup, so that we use the right
**		Process ID when we awaken the session that requested the lock.
**	    Set lkreq.pid to this process's Process ID.
**	    Re-position check for llkb_count so that we are more precise in our
**		enforcement of the max locks per lock list limit.
**	    Maintain synch_complete and asynch_complete statistics.
**	    Remove useless assignment of LK_NEW_LOCK status when already set.
**	    Remove llb_async_status field; use lkreq.status instead.
**	    Keep additional statistics on allocations, searches, etc.
**	21-jun-1993 (bryanp)
**	    It's not common that there is no completion LKREQ, but it's not
**		necessarily an error. LK_DEADLOCK, like LK_NOLOCKS, is
**		transmitted back without an LKREQ, as is LK_BUSY. Furthermore,
**		cancellation completions don't use LKREQ's. So if there's no
**		completion LKREQ, we just use the llb_async_status.
**	    Export the "compatible" matrix for use by LK_deadlock().
**	    Fixed lock completion status bugs involving timed out and
**		interrupted locks.
**	    Check for and handle E_DMA017_LK_MUTEX_RELEASED from LK_cancel.
**	    Clean up some of the ifdef'd tracing code.
**	    Move SS$_EXENQLM => LK_NOLOCKS mapping to closer to the $enq calls.
**	    Remove "status_addr", "value_addr", and "timeout" arguments
**		from LK_suspend.
**	30-jun-1993 (shailaja)
**	    Fixed compiler warnings on semantics change in ANSI C.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (bryanp)
**	    Fixed bugs in lock stalling support. 
**	    Re-arranged code for legibility.
**	    Include <tr.h>
**	    Added some casts to pacify compilers.
**	23-aug-1993 (andys)
**	    Initialize lkb->lkb_type in LK_request_lock so that it will be 
**	    reused correctly if freed early, e.g. when an EXENQLM occurs.
**	23-aug-1993 (rogerk)
**	    Fixed to not return NOLOCKS when we rerequest a lock we already
**	    own while at the limit of locks we are allowed to hold.
**	20-sep-1993 (bryanp) merged 23-mar-1993 (bryanp) -- B47950,B50252
**	    Fix a number of bugs surrounding handling of invalid lock values:
**	    1) When a lock is marked invalid, we should not be resetting the
**		value to 0. Resetting the value to zero can cause the value to
**		eventually be re-incremented back into a range of lock values
**		which have been seen before, which can confuse callers into
**		believing that a lock value has not changed when in fact it
**		has (the lock was set to a value, invalidated, reset to 0, and
**		then eventually re-incremented back to the already seen value).
**	    2) When a lock is marked invalid, we must notify VMS of this so
**		that VMS can notify any other nodes.
**	    3) When a sys$enq call completes synchronously (SS$_SYNCH is
**		returned), we must check the lock status block for the
**		SS$_VALNOTVALID return code. This means that the lock was
**		synchronously granted/converted but that the value is marked
**		invalid. We must ensure that in this case we mark the
**		rsb_invalid field in the RSB so that LK_VALNOTVALID is returned
**		to the caller.
**	    4) VMS doesn't support marking a resource as value-not-valid when
**		converting down from >=SIX and not providing the value. Since
**		we don't need this functionality, we'll return LK_BADPARAM
**		instead of trying to mark the value invalid (and ending up
**		marking it locally invalid but globally valid).
**	18-oct-1993 (rmuth)
**	    CL prototype, add <lgclustr.h>.
**	12-oct-1993 (tad)
**	    Bug #56449
**	    Changed %x to %p for pointer values in 
**	    LK_do_new_lock_request(), and enq_complete().
**	02-feb-1994 (pearl)
**	    Bug 57376.  In LK_request(), if LK_VAL_NOTVALID is returned
**	    from the call to LK_request_lock(), we should call
**	    LK_complete_request() to ensure that we have a valid lock mode
**	    in the lock value block (LK_VALUE).
**	14-dec-1994 (medji01)
**	    Mutex Granularity Project
**		- Remove LKD mutex acquisition and free from LKrequest().
**		- Remove LKD mutex free and reacquisition around LK_csp_send()
**		  from LK_request().
**		- In LK_request_lock():
**		  o Latch RSH mutex during search
**		  o Latch LKD mutex around LK_allocate_cb() and 
**		    LK_deallocate_cb() calls.
**		  o Latch RSH when inserting RSB into hash queue.
**		- In LK_do_new_lock_request():
**		  o Acquire the LKD, RSH, and RSB mutexes when granting a
**		    lock.
**		  o Acquire the RSH and RSB mutexes when denying a lock.
**		  o Acquire the LKH mutex when adding an LKB
**		  o Acquire the LLB mutex when attaching an LKB if the LLB is
**		    shared.
**		- In LK_do_convert():
**		  o Acquire RSB mutex during grant queue scans
**		  o Acquire LKD, RSH, and RSB mutexes before calling
**		    LK_try_wait() and LK_try_convert()
**		  o In cluster code acquire LLB mutex for shared LLB's
**		- Removed mutex_held parameter from LK_get_completion_status().
**		- Acquire the LKD mutex in convert_must_stall() when processing
**		  the stall queue.
**		- Acquire the LKD mutex in request_must_stall() when processing
**		  the stall queue.
**	28-mar-1995 (medji01)
**	    64K+ Lock Limit Project
**		- Removed references to id_instance in LK_ID.
**		- Changed error messages to reference id_id instead
**		  of id_instance.
**	20-may-95 (hanch04)
**		- move check for rsb_hash_bucket->rsh_mutex.cs_test_set != 1
**		- to ifdef  LKD_DEBUG
**	06-july-1995 (thaju02)
**	    Modified LK_request for -max_stall_time=...
**	    argument for ckpdb.
**	21-Sep-1995 (johna)
**	    Back out the above change. (from 418541)
**	19-jul-1995 (canor01)
**	    Mutex Granularity Project
**		- Acquire the LKD mutex before calls to LK_deallocate_cb()
**	20-jul-1995 (canor01)
**	    Mutex Granularity Project
**		- Acquire the LKD mutex before calls to LK_try_wait()
**		  and LK_try_convert()
**	09-aug-1995 (thaju02)
**	    Back out of changes made during implementation of ckpdb
**	    stall time feature.  Removed LK_OVERRIDE_NIW checking,
**	    instead ckpdb creates a new lock list.
**	21-Sep-1995 (johna)
**	    Back out the above change. (from 418541)
**	16-aug-1995 (duursma)
**	    Initialize request_id in send_dlock_message (none of its callers
**	    do).  Also, set lkreq.function there instead of at call sites.
**	22-aug-1995 (dougb)
**	    As a related update to the fix for bug #70696, update the
**	    parameters to LK_do_new_lock_request().  free_res is now a bit
**	    mask.
**	30-aug-1995 (dougb) bug #70696
**	    Export LK_convert_mode[][] for use in LKDLOCK.C (on VMS only).
**	05-sep-1995 (duursma)
**	    Now taking advantage of the fact that the VMS distributed lock
**	    manager does "live lock" detection, which means we don't have
**	    to check for it here any more.  This led to various simplifications
**	    in LK_do_new_lock_request() and LK_do_convert().
**	    In addition, added a few more local deadlock checks since a request
**	    waiting globally may now also be part of a local loop.
**	29-dec-1995 (dougb) bug 74007
**	    Get new LK_get_completion_status() performance feature working
**	    when it's called from LK_request(). (bug 74007)
**	    Don't access violate in LKrequest() if LKD_DEBUG is enabled and
**	    a lock_key wasn't passed.  I don't know why the check was
**	    commented out...
**	    Release a few more semaphores in the Cluster-specific code.  And,
**	    don't re-acquire the lkd_mutex in convert/request_must_stall!
**	12-Dec-1995 (jenjo02)
**	    LK_allocate_cb, LK_deallocate_cb made responsible
**	    for their own mutex protection - no longer necessary
** 	    to acquire LKD mutex prior to call.
**	    Moved accounting of lkd_llb|rsb|lkb_inuse to 
**	    LK_allocate|deallocate_cb().
**	    Added a host of new list/queue mutexes to relieve the
**	    pressure on lkd_mutex.
**	20-Dec-1995 (jenjo02)
**	    In LK_request_lock(),
**	    reversed order of RSH, LKH hash searches. System stats
**	    show a great percentage of requests are "re-requests"
**	    for the same resource. Searching in the LKH->RSB 
**	    direction tells us at once whether the LLB already
**	    has a lock on the resource, saving a RSH search if
**	    a matching RSB is found.
**	04-Jan-1996 (jenjo02)
**	    In LK_get_completion_status(),
**	    search for LKB in prior direction from LLB; LKBs are
**	    added to the end of the chain, so this path should
**	    be shorter.
**	08-Jan-1996 (duursma) bug #71331
**	    LKdeadlock's counterpart to the fix for bug 71331:
**	    don't recurse onto the same RSB for convert requests.
**	08-jan-1996 (duursma)
**	    Removed irrelevant TRdisplay() message re locally incompatible locks
**	08-Jan-1996 (duursma) bug #71338
**	    Fix for bug 71338.  When computing the max request mode for
**	    convert requests, ignore the wait queue.
**	08-jan-1996 (duursma) bug #71339
**	    When a convert request is granted, it is possible, if unlikely,
**	    that the grant queue has become non-contiguous.  Check for that
**	    situation here and if it occurs, move the out-of-order request
**	    to the tail of the grant queue. (Bug 71339)
**	11-jan-1996 (duursma) bug #71340
**	    Created new routine compute_group_modes() to fix bug 71340.
**	    Replace all manual settings of rsb_grant_mode and rsb_convert_mode
**	    with calls to compute_group_modes()
**	08-feb-1996 (duursma)
**	    Undid last change (compute_group_modes()).  It appears that the
**	    rest of the LK facility isn't ready for a correct (VMS DLM style)
**	    implementation of group grant mode and group convert mode.
**	22-mar-1996 (canor01)
**	    Undid change for 71338.  This causes true deadlocks to go
**	    undetected.
**	23-Apr-1996 (jenjo02)
**	    Cleaned up code in deadlock() with respect to concurrent
**	    threads. RSB->LKB queues were not being protected adequately
**	    during traversal allowing other threads to modify the queues
**	    and causing deadlock() to get lost and, typically, loop
**	    forever.
**	14-May-1996 (jenjo02)
**	    Added lkd_dlock_mutex to single-thread deadlock().
**	30-may-1996 (pchang)
**	    Added LLB as a parameter to the call to LK_allocate_cb() to enable
**	    the reservation of SBK table resource for use by the recovery
**	    process. (B76879)
**	21-jun-1996 (pchang)
**	    In LK_request_lock(), take the new RSB off the hash queue before 
**	    freeing it, should allocation of a new LKB fail.  (B77374)
**	01-jul-1996 (pchang)
**	    Another instance the above bug was found.  Instead of duplicating
**	    the above change, we now delay insertion of the new RSB into the
**	    hash queue until we have succeeded in allocating a new LKB.  This
**	    also closes a small timing window in which the new RSB could be
**	    referenced by another session while we're still building the LKB.
**	23-jul-96 (nick)
**	    Catch logical locks created via implicit conversion from physical.
**	21-aug-1996 (boama01)
**	    Corrections arising from Cluster Support tests and 1.2/00(axp.vms)
**	    QA testing, but relevant for all platforms:
**	    - Changed all "#ifdef CSP_DEBUG" diagnostics to use new global
**	      csp_debug, for runtime activation; added GLOBALREF for it; also
**	      improved and supplemented traces.
**	    - Added LK_rqst_trace() to trace lock requests from non-CSP
**	      servers (CSP has its own tracing and log file).
**	    - Changes to enq_complete(); must store returned lock value in RSB
**	      when completing lock or convert-up request; added lkreq ptr for
**	      access to lkreq.
**	    - Changes to LK_try_wait() and all its callers; when Clustered RSB
**	      ends up unused, we deallocate it, but that unmutexes it too; must
**	      let callers know this so they do not attempt unmutex as well.
**	      Chg *RSB parm to **RSB so caller can see it's been nulled.
**	    - Changes to LK_request_lock(), LK_do_new_lock_request(),
**	      LK_do_convert() and dequeue_lock(); clean up mutex protocols,
**	      esp. for RSB and LLB, to avoid LK_MUTEX/UNMUTEX errors.
**	    - Fixed LKrequest() for probs arising from slowed-down Cluster node.
**	      Synchronous lock would not arrive before timeout expired; then
**	      calls to LK_get_completion_status() would not fold LK_GRANTED
**	      into OK status, causing requestor to complain (msgs CL100B,
**	      DM901C, DM9021, et al).
**	      However, I'm still not sure that enq_complete() properly delivers
**	      accurate statuses to the lkb->lkb_lkreq.status field!
**	12-sep-96 (nick)
**	    Various changes to fix the LKB and RSB leaks which were happening
**	    when locks fail to get granted for various reasons.  On some 
**	    sites, this leakage was severe enough to bring down the 
**	    installation every day or so. #78432
**	13-dec-96 (cohmi01)
**	    Pass callers sys_err parm down to LK_do_new_lock_request() and
**	    to LK_do_convert(). Use it upon failure to pass session id
**	    and pid of lock holder back to caller upon failure. (Bug 79763)
**	04-Oct-1996 (jenjo02)
**	    Added *lkb to LK_COMP_HANDLE in which to pass back the LKB to avoid
**	    the LLB->LKB chain scan in LK_get_completion_status.
**	08-Oct-1996 (jenjo02)
**	    Wrapped updating of lkd_rqst_id in lkd_mutex.
**	15-Oct-1996 (jenjo02)
**	    In deadlock(), added LLB to aid in detecting an LKB which no longer 
**	    belongs to the wait-for graph.
**	01-aug-1996 (nanpr01 for ICL keving)
**	    Add Blocking Callbacks facility to LK.
**	    Provide a new interface, LKrequestWithCallback. Callers of this
**	    interface can, optionally, provide a callback which (once the
**	    lock has been granted) will be invoked whenever the lock blocks
**	    another thread wanting a lock on the same resource. LKrequest
**	    is recoded as a call to LKrequestWithCallback with a NULL
**	    callback.
**	    LK_try_wait/convert routines now return a STATUS.
**	23-sep-1996 (canor01)
**	    Moved global data definitions to lkdata.c.
**	11-Nov-1996 (jenjo02)
**	    Moved call to LK_suspend() after a LK_do_convert() inside of
**	    LK_do_convert() so the LLB can be marked LLB_WAITING while
**	    the rsb_mutex is held. When done without the rsb_mutex, a
**	    window is opened whereby deadlock() may encounter a converting
**	    LKB whose LLB isn't yet flagged as waiting. The LLB is 
**	    skipped by deadlock() and a deadlock goes undetected.
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
**	15-Nov-1996 (jenjo02)
**	   Optimistic deadlock detection.
**	   Instead of doing deadlock detection every time we must wait for
**	   a lock, queue the waiting LLB to a deadlock wait queue and let the
**	   (new) deadlock detector thread periodically run the list and
**	   resolve deadlocks.
**      22-nov-1996 (dilma04)
**          Row-Level Locking Project:
**          Removed two lines from the Nick's fix for bugs 77890 and 76463 
**          which used to return LK_NEW_LOCK status when physical_to_logical
**          conversion request is waiting. This made it possible to request
**          a new lock while waiting and caused LK_REQUEST_BAD_PARAM error. 
**	06-Dec-1996 (jenjo02)
**	    Rearranged manipulation of RSH to avoid holding rsh_mutex too long.
**	    Removed lkd_mutex protection of lkd_rqst_id, which was creating
**	    significant blockage. lkd_rqst_id is paired with a lock list when
**	    it's needed, so duplicates which might result from not mutexing
**	    its value should be ok. Increment lkd_rqst_id while holding the
**	    llb_mutex to at least ensure that its value will be unique within
**	    a lock list.
**      12-dec-96 (dilma04)
**         In LK_request_lock(): if a resource is locked by a logical lock
**         and a physical lock is being requested, grant a LOGICAL lock.
**	18-dec-1996 (wonst02)
**	    Add lock_key parameter to CSsuspend call to track locks.
**	09-Jan-1997 (jenjo02)
**	   Callers of LK_suspend(), LK_resume(), must hold lkd_dlock_mutex,
**	   which will be released by the called function.
**      23-jan-1997 (stial01 for nanpr01)
**          Added new function LKpropagate to request locks for a
**          new sibling page during a btree leaf page split.
**	04-feb-97 (chech02)
**	   Fixed compiler error for VMS alpha port.
**	07-Feb-1997 (jenjo02)
**	    Check status of LK_cancel() of deadlock victim. If the cancel
**	    was successful, only then consider the deadlock condition a
**	    success and resume the victim's thread.
**      28-feb-1997 (stial01)
**         Update llb_status if we've added an EXCL SV_DATABASE lock
**	06-mar-1997 (nanpr01)
**	    Clustered lock request fails in LK_get_completion status
**	    since request_id does not match.
**    13-mar-97 (toumi01)
**        Move # for #ifdef etc. to column 1 as required by many 
**        compilers.
**	22-Apr-1997 (jenjo02)
**	   Removed LKREQ_CB_DEALLOCATED flag, whose use was causing
**	   memory leaks when the LKB it was being set in was deallocated.
**	   Instead, changed dequeue_cbs() to be passed a pointer to int
**	   "res_to_free" in which flags will be set indicating which, if
**	   any resources were actually freed.
**      07-May-1997 (dilma04)
**         Bug 81895. Return LK_COV_LOCK if an intend page lock is converted
**         to a covering mode.
**      14-may-1997 (dilma04)
**          Return LK_LOG_LOCK if a physical lock has been converted to
**          logical.
**	15-May-1997 (jenjo02)
**	   Added LK_INTERRUPTABLE flag, used to make this request interruptable
**	   even if the lock list is marked non-interruptable.
**	19-May-1997 (jenjo02)
**	   Skip LK_NOLOCKS check if propagating.
**      21-May-1997 (stial01)
**         Update llb_status if we've added an EXCL ROW lock
**      13-jun-1997 (dilma04)
**           Zero lkb_count, if physical lock has been converted to logical.
**      02-jul-97 (dilma04)
**          Update llb_status if converting a SV_DATABASE or ROW lock
**          from shared mode to exclusive.
**	06-Aug-1997 (jenjo02)
**	  o Moved the handling and awakening of the deadlocked thread to
**	    LK_cancel(), LK_do_cancel().
**	  o Delay CSget_sid() call until we really need a sid, i.e. about to
**	    suspend, or if sending a request thru the csp.
**      01-Sep-1997 (horda03,thaal01)
**          #IFDEF VMS added since CL_ERR_DESC has different structure on VMS
**	03-Sep-1997 (jenjo02)
**	    Bug 85177, fix from Brian Macker. deadlock() was picking the oldest
**	    rather than the newest lock requestor as the victim. Pre 2.0 releases
**	    of Ingres always victimized the newest lock requestor, so this change
**	    reinstates that semantic.
**      05-sep-1997 (bobmart)
**          Change comment character on horda03's last "ifdef VMS" integ.
**	09-sep-97 (rosga02)
**	    Corrected another clustered installation locking bug. In the 
**	    enq_complete() function a local status field was used instead of
**	    lkb->lkb_lkreq.status that was set only in a case of a new lock.
**	    in case of Converted locks the field was left to its initial
**	    value that could be any garbage.  That led to strange error codes 
**	    sometimes coming from LKrequest, when due to a load on a clustered
**	    installation lock conversion requests may wait for a lock to be
**	    granted by the VMS global lock manager.  		 	
**	18-Sep-1997 (jenjo02)
**	    Backed out 03-Sep-1997 change. It caused some deadlocks to go undetected
**	    at worst, and did a no better job of picking a victim then the original
**	    code at best.
**	20-feb-1998 (kinte01)
**	    Readded declaration for status as it is used in other contexts
**	    than just the one in which rosga02 submitted his change above.
**	08-Sep-1998 (jenjo02)
**	    Added PHANTOM_RESUME_BUG debug code.
**	09-Sep-1998 (jenjo02)
**	    Added new stat, maximum number of locks allocated per transaction.
**	    Modified LKH/RSH list searches to do so without the need of
**	    a mutex.
**	    Removed un-used "CB_CHECK" macros, which just clutter up the code.
**	23-Sep-1998 (jenjo02)
**	    In LK_try_convert(), LK_try_wait(), recheck the LKB after
**	    acquiring the dlock_mutex. Deadlock activity may have
**	    changed the LKB state, in which case we musn't resume
**	    the waiting thread - it may already be resumed! This
**	    appears to be the source of the PHANTOM_RESUME_BUG.
**      05-Nov-1998 (hanal04)
**          Record the sid and pid in the completion handle for added
**          protection against use of wrong lkb->lkreq in
**          LK_get_completion_status. Mark LKREQs as collected to prevent
**          reuse of old LKREQ responses. (b90388). Crossed from 1.2/01
**	16-Nov-1998 (jenjo02)
**	  o Cross-process thread identity changed to CS_CPID structure
**	    from PID and SID.
**	  o Installed macros to use ReaderWriter locks when CL-available
**	    for blocking the deadlock matrix.
**	  o Removed PHANTOM_RESUME_BUG code.
**      18-Jan-1999 (hweho01)
**          Turned off optimization for NCR (nc4_us5) to fix error:
**          E_SC0206_CANNOT_PROCESS.
**	08-Feb-1998 (jenjo02)
**	    Modified to deal with >1 waiting LKB per lock list (LLB). 
**	    LKdeadlock_thread() now deals with a list of waiting LKBs, not
**	    LLBs, deadlock() modified to handle this. Instead of cancelling
**	    a deadlocked request itself (which has probable timing windows),
**	    LKdeadlock_thread() posts the deadlocked status in the LKB and the 
**	    waiting thread is resumed and bears the responsibility of 
**	    cancelling the lock request.
**      20-Aug-1999 (hanal04) Bug 97316 INGSRV921
**          Return LK_NOLOCKS in LK_request_lock() when we hit the hard
**          limit for the number of locks on the llb.
**	16-feb-1999 (nanpr01)
**	    Support for update mode lock.
**	28-may-1999 (nanpr01)
**	    SIR 91157 : If flag LK_QUIET_TOO_MANY is set, donot log 
**	    DMA00D message.
**      28-jul-1999 (hweho01)
**          Added NO_OPTIM for ris_u64 to eliminate error "E_DM934D_
**          DM0P_BAD_PAGE" during blob sep test.
**	02-Aug-1999 (jenjo02)
**	    Implement preallocated LKBs/RSBs. Each RSB now contains an
**	    embedded LKB, obviating the need to always allocate one 
**	    separately.
**	    Count RSB waiters and converters in rsb_waiters, rsb_converters
**	    to short-circuit unneeded calls to LK_try_wait, LK_try_convert.
**	    Rewrote deadlock() to save recursion information on the stack
**	    instead of the LLB, simplifying the code.
**	    Copy lkreq to the the LKB only when it can't otherwise be
**	    avoided.
**	    Fill in sys_err with lock blocker info only when it might be
**	    meaningful, like after a deadlock or lock stall, instead of
**	    all the time.
**	    General attempt to streamline and unclutter the code.
**	06-Oct-1999 (jenjo02)
**	    Handle to LLBs is not LLB_ID, not LK_ID.
**	22-Oct-1999 (jenjo02)
**	    Removed dependence on and use of lkd_dlock_lock. 
**	    Rewrote deadlock() to intelligently traverse the wait-for 
**	    graph using RSB mutexes.
**	13-Dec-1999 (jenjo02)
**	    Added trace point DM309, which, when set, TRdisplays
**	    information about the lock lists and locks involved in
**	    deadlocks.
**	    For this, a new static function show_deadlock() was
**	    added, and a new lgd_status bit "LGD_SHOW_DEADLOCKS"
**	    set in response to set [no]trace point DM309 by LGalter().
**      30-Jun-2000 (horda03) X-Int of fix for bug 48904 (VMS only)
**        30-mar-94 (rachael)
**           Added call to CSnoresnow to provide a temporary fix/workaround
**           for sites experiencing bug 48904. CSnoresnow checks the current
**           thread for being CSresumed too early.  If it has been, that is if 
**           scb->cs_mask has the EDONE bit set, CScancelled is called.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	12-apr-2001 (devjo01)
**	    s103715 (Portable cluster support)
**	12-Jun-2001 (horda03)
**	    In the port for 1.2 to 2.0 something has been lost to the change
**	    I made on 29-Aug-1997. But anyhow, things have moved on, and now
**	    VMS has the missing fields within the CL_ERR_DESC structure.
**	    The temporary VMS fix can now be removed, which fixes Bug 104932
**	18-Sep-2001 (jenjo02)
**	    When allocating new lock/resource, set LLB_ALLOC to 
**	    keep scavenge() from stealing our RSB/LKB stash.
**	07-Nov-2001 (jenjo02)
**	    BUG 106306, mutex deadlock during scavenge(). When allocatin
**	    a new LKB, pass RSB in (unused) llb_tick so that mutex 
**	    deadlocks can be avoided.
**	15-Jan-2002 (jenjo02)
**	    Another instance of B106306, this one involving LLB and RSH
**	    mutexes. Releasing LLB mutex while allocating RSB/LKB solves
**	    all such mutex deadlocks.
**      15-Jan-2002 (hweho01)
**          Turned off optimization for rs4_us5 to eliminate errors
**          "DM92CB_DM1P", "DM92DD_DM1P_FIX_HEADER" and "DM92D0_DM1P_GETFREE"
**          during blob00.sep test.  Compiler: C for AIX Compiler V.5.0.1.2.
**	16-Jan-2002 (jenjo02)
**	    Backed out 15-Jan-2002 change for B106306. It introduces new,
**	    more exotic problems.
**	28-Feb-2002 (jenjo02)
**	    Changes to prevent/handle "phantom resume", scavenge() problem
**	    when allocating LKBs, made MULTITHREAD an attribute of
**	    a lock list rather than just a run-time option.
**      20-Aug-2002 (jenjo02)
**          Added support for User-set TIMEOUT=NOWAIT, triggered by a
**          lock timeout value of DMC_C_NOWAIT. Return LK_UBUSY when
**          these blocks occur to distinguish them from internally
**          produced LK_BUSY nowaits.
**          Cleaned up reporting of blocking CPID, added lock mode
**          of blocker to sys_err->moreinfo.
**	22-aug-2002 (devjo01)
**	    Put in support for "local" locks.  That is, locks whose scope
**	    is local to the current node in a clustered ingres installation.
**	    Incompatable local locks on different nodes, don't conflict,
**	    and they are treated exactly as locks in the non-cluster case.
**	29-aug-2002 (devjo01)
**	    Put in support for callbacks from DLM.
**      23-Oct-2002 (gygro01)
**          b87049 - Make sure that the resource holder and the deadlocked
**          sessionid is different in the error message by walking through
**          the resource holder queue and pick a different than the current
**          session. This is done in LK_do_new_lock_request()and
**          LK_do_convert().
**      24-oct-2002 (horda03) Bug 109002
**          On a Clustered Ingres Installation, if a lock request is not
**          locally compatible with LK_NOWAIT flag set, then the LKB and
**          RSB blocks created will not be released. This will eventually
**          lead to all Locks being used an a system crash.
**	12-nov-2002 (horda03) Bug 105321 INGSRV 1502
**	    Following a user interrupt, iF LKcancel() returns LK_CANCEL,
**	    then the lock request has been granted before it could be
**	    cancelled. Rather than calling CScancelled(), call CSsuspend()
**	    as a resume is in the pipe-line. But just to be safe, suspend
**	    with a timeout.
**      24-Jul-2003 (hweho01)
**          Added r64_us5 to NO_OPTIM list, turned off optimizer for AIX 
**          64-bit build, avoid error "E_DM934D_DM0P_BAD_PAGE" during  
**          HandoffQA BE/blob00.sep test.
**          Compiler : VisualAge 5.023.
**	04-sep-2003 (abbjo03)
**	    Changes to use VMS headers provided by HP. Also add missing argument
**	    to call to dequeue_cbs().
**	29-dec-2003 (sheco02)
**	    Change CScas_long to CScas4 to resolve undefinded symbol.
**	08-jan-2004 (devjo01)
**	    Minor changes to hide differences between VMS, Linux & Tru64
**	    CX implementations.
**	25-jan-2004 (devjo01)
**	    If conf_CAS_ENABLED not defined, use mutexes to protect
**	    callback structures. N/B conf_CLUSTER_BUILD requires
**	    conf_CAS_ENABLED, but not visa-versa.
**	26-Feb-2004 (devjo01)
**	    - Correct handling of distributed NOWAIT locks.
**	    - Correct parameter passed to CScas4 in LK_AST_callback.
**	03-Apr-2004 (devjo01)
**	    Lock conversion using the DLM could copy the value block
**	    prematurely for an asynchronous request due to a bad
**	    flag test.
**	20-may-2004 (devjo01)
**	    Need to be able to convert downward even if stalled,
**	    otherwise undetected deadlock is possible with RCP
**	    trying to fault in a page, but never getting the cache
**	    lock for the page because a DBMS attached to another
**	    cache containing the page forces the page after getting
**	    a blocking notification, but stalls trying to downgrade
**	    his page cache lock.
**	02-Sep-2004 (bonro01)
**	    Remove invalid change 471356
**      7-oct-2004 (thaju02)
**          Change offset to SIZE_TYPE.
**	05-jan-2005 (devjo01)
**	    Change test for determining whether a lock is distributed in
**	    LK_try_wait.
**	09-feb-2005 (devjo01)
**	    Close tiny window where cx_user_func was reset to NULL.
**	09-feb-2005 (devjo01)
**	    Correct previous change.
**	06-apr-2005 (devjo01)
**	    Conversions to the same write mode will now update the LVB.
**	    Ingres does not make use of this at present, but this puts
**	    us in line with other lock managers.
**	22-Apr-2005 (fanch01)
**	    Add lg.h for LG_LSN type.
**	05-may-2005 (devjo01)
**	    - Add generic distributed deadlock support.  See separate banner
**	    in LKdeadlock_thread for overview of design.
**	    - Now that it's set, use lkd_csid instaead of CXnode_number()
**	    to fetch the node number of the current instance.
**	18-May-2005 (shaha03)
**	    Bug # 114534, Added i64_hpu for NOOPTIM list.
**	06-jun-2005 (devjo01)
**	    Correct non-cluster build problem introduced with change 477093.
**	21-jun-2005 (devjo01)
**	    - Do not overwrite cx_owner when converting a lock, and
**	    LKD_NO_DLM_GLC is set.
**	    - 'cx_key' is now a struct.
**	    - Set cx_user_extra as the offset between lkb & lkb.lkb_cx_req.
**	    - Pass CX_F_PROXY_OK to CXdlm routines, as these requests
**	    may be redirected at need when running under VMS.
**      18-oct-2005 (horda03) Bug 115405/INGSRV3463
**          Fixed E_CL103E error message parameters.
**	24-oct-2005 (devjo01)
**	    - In LK_do_convert make sure new_group_mode is properly
**	      initialized even when converting to the same mode. Also
**	      make sure session id is set if GLC not available.
**	    - Add casts for return values from CX_PTR2RELOFF.
**	16-nov-2005 (devjo01)
**	    - Rename CXdlm_mark_as_deadlocked to CXdlm_mark_as_posted,
**	      and set cx_status locally to conform with new more
**	      generic usage of CXdlm_mark_as_posted.
**	23-nov-2005 (devjo01) b115583
**	    - Add return of distinct code LK_NOTHELD to provide better
**	      diagnostics and handling of locks implicitly freed by the
**	      exit of a process under Clustered Ingres for OpenVMS.
**	27-oct-2006 (abbjo03)
**	    Supplement 7-oct-2004 change: additional offset types that should
**	    be declared SIZE_TYPE.  Remove PARANOIA #define and fix some #if
**	    PARANOIA to #ifdefs.  Clean up some trace messages.
**	21-nov-2006 (jenjo02/abbjo03)
**	    In LK_local_deadlock, if cvt-cvt deadlock on other than top_lkb,
**	    post and wake up current_lkb or it'll sleep forever.  Use
**	    lkreq.request_id in o_stamp instead of un-thread-safe llb_stamp. 
**	    Enhance some trace messages.  Do not return LK_NOTHELD.  Handle
**	    the ASYNC DOWNGRADE case from enq_complete.  Remove #if 0 code.
**	01-Dec-2006 (kiria01) b117225
**	    Correct parameter access descriptions for the lockid parameters.
**	02-jan-2007 (jenjo02/abbjo03)
**	    Additional trace code made dependent on II_CLUSTER_DEADLOCK_DEBUG
**	    logical.  Make LK_main_dsmc a local static.
**	03-jan-2007 (abbjo03)
**	    Supplement 2-jan change: LK_DEADLOCK_MSG also has to depend on
**	    lk_cluster_deadlock_debug.
**	18-jan-2007 (jenjo02/abbjo03)
**	    Send continuation msg if current_lkb is a converter and there are
**	    no other locks before it.
**	28-Feb-2007 (jonj)
**	    Ignore lk_stall_request() when caller has requested LK_NOSTALL.
**	13-Jul-2007 (jonj/joea)
**	    Use two separate buffers for lock information.
**	01-Oct-2007 (jonj)
**	    Delete separately allocated CBACK structure, instead embed in
**	    LKB.
**	15-Nov-2007 (jonj)
**	    Be watchful of mutex failures as the owning process may be
**	    dead and the underlying structure must be considered
**	    inconsistent.
**	30-Nov-2007 (jonj)
**	    Anticipate DLM possibly returning
**	    E_CL2C3?_CX_E_OS_CONFIG_ERR, return as LK_NOLOCKS.
**      08-Mar-2004 (hanal04) Bug 111911 INGSRV2736
**          Code changes for bug 87049 do not match those from ingres25.
**          The current code still allowed a failing lock requestor to
**          report a blocking pid/sid that is the same as the requestor's
**          sid/pid.
**	09-Dec-2008 (jonj)
**	    SIR 120874: use new form uleFormat, CL_CLEAR_ERR.
**      17-dec-2008 (joea)
**          Replace READONLY/WSCREADONLY by const.
**	08-jun-2009 (jonj/joea)
**	    In LK_request, when checking for implicit conversions, always
**	    check the LKB hash queue unless propagating.
*/


GLOBALREF i4  csp_debug;
GLOBALREF char	LK_compatible[8];
static LK_DSM_CONTEXT *LK_main_dsmc;

# ifdef VMS
GLOBALCONSTDEF
# else
static const
# endif
		char LK_convert_mode [8][8] =
{
/*                N       IS      IX      S       SIX       U      X	*/
/* N   */    { LK_N,   LK_IS,  LK_IX,  LK_S,   LK_SIX,    LK_U,  LK_X },
/* IS  */    { LK_IS,  LK_IS,  LK_IX,  LK_S,   LK_SIX,    LK_U,  LK_X },
/* IX  */    { LK_IX,  LK_IX,  LK_IX,  LK_SIX, LK_SIX,    LK_X,  LK_X },
/* S   */    { LK_S,   LK_S,   LK_SIX, LK_S,   LK_SIX,    LK_U,  LK_X },
/* SIX */    { LK_SIX, LK_SIX, LK_SIX, LK_SIX, LK_SIX,    LK_SIX,LK_X },
/* U   */    { LK_U,   LK_U,   LK_X,   LK_S,   LK_SIX,    LK_U,  LK_X },
/* X   */    { LK_X,   LK_X,   LK_X,   LK_X,   LK_X,      LK_X,  LK_X } 
};				    /* Table [grant mode][request mode]
				    ** to new mode */

/* Access the id of this process' callback thread descriptor */
GLOBALREF i4  LK_mycbt;

/*
** Forward declarations for static functions:
*/
static STATUS	LK_request(i4 flags, LK_LLID lock_list_id,
			    LK_LOCK_KEY *lock_key, u_i4 lock_mode,
			    LK_VALUE *value, LK_LKID *lockid,
                            LKcallback callback, PTR callback_arg,
			    LK_COMP_HANDLE *completion_handle,
			    CL_ERR_DESC *sys_err);

static STATUS	LK_do_new_lock_request( 
		  	    LLB			*proc_llb,
			    LKB 		*lkb, 
			    RSB			**rsb,
			    i4			flags,
			    LK_COMP_HANDLE	*completion,
			    CL_ERR_DESC		*sys_err);

static STATUS	LK_do_convert(
			    LLB			*proc_llb,
			    LKB			*lkb,
			    i4			flags,
			    i4			mode,
			    LK_VALUE		*value,
			    LK_COMP_HANDLE	*completion,
			    LKcallback		callback,
			    PTR			callback_arg,
			    CL_ERR_DESC     	*sys_err);

static VOID	show_deadlock(LKB *dead_lkb, i4 stamp);

static VOID	LK_get_completion_status(
    			    LK_COMP_HANDLE  	*completion_handle,
    			    STATUS	    	*status,
    			    LK_VALUE	    	*value,
    			    CL_ERR_DESC	    	*sys_err);

static  STATUS 	dequeue_lock( RSB **rsbp, LKB *lkb );
static  STATUS	dequeue_convert( LKB *lkb, bool *lock_granted );
static	STATUS	send_dlock_message( CL_ERR_DESC *sys_err );
static VOID	lk_stall_request(LKB *lkb, i4 lineno);
static STATUS   lk_handle_ivb( LLB *llb, LKB *lkb, i4 flags, bool syncgrant, 
		    i4 waitflags, i4 timeout, i4 lineno );
static VOID	dequeue_cbs(
		    LLB *llb,
		    RSB **rsbp, 
		    LKB **lkbp, 
		    bool  FreeRsbIfFail);
static STATUS	locate_rsb(
		    LK_LOCK_KEY *lock_key,
		    RSB **rsb,
		    CL_ERR_DESC *sys_err);

# if defined(conf_CLUSTER_BUILD)

static VOID	LK_AST_callback( CX_REQ_CB *cx_req, i4 blocked_mode );

static VOID	lk_set_dlm_callback( LKB *lkb, RSB *rsb );

static bool LK_dup_msg(
		LK_DSM_CONTEXT	*lkdsm_context,
		CX_BLKI		*pblkinfo,
		LK_DSM		*inlkdsm,
		RSB		*rsb,
		LKB		*top_lkb,
		i4		max_request_mode );

GLOBALREF i4 rcp_node_id;

# define LK_DEFAULT_BLKI_COUNT	64

# define LOCL_BLKI	0
# define DIST_BLKI	1

static CX_BLKI	Lk_default_blki_array[2][LK_DEFAULT_BLKI_COUNT];
static CX_BLKI *Lk_blki_pointer[2] = { Lk_default_blki_array[0], Lk_default_blki_array[1] };
static i4	Lk_blki_count[2] = { LK_DEFAULT_BLKI_COUNT,  LK_DEFAULT_BLKI_COUNT };
# endif


#ifdef xDEBUG

STATUS lk_cback_print(i4 block_mode, PTR arg, LK_LKID *lockid)
{
    TRdisplay("%@: Lock on lock list 0x%p blocked a lock of mode 0x%x but no callback was provided.\n", arg, block_mode);
    return(OK);
}

#endif

/*{
** Name: LKrequest	- Request a lock.
**
** Description:
**      This function handles requesting a lock. It is a call to 
**      LKrequestWithCallback with null callback and argument. See
**      LKrequestWithCallback for full description.
**
** Inputs:
**      flags                           A combination of LK_NOWAIT, LK_STATUS
**					LK_PHYSICAL, LK_IGNORE_LOCK_LIMIT, and 
**					LK_CONVERT if this is a convert request.
**      lock_list_id                    The lock list identifier returned 
**                                      from a previous call to LK_create_list
**      lock_key                        A pointer to the resource name to lock.
**      lock_mode                       The requested lock mode for the
**					resource.
**      timeout                         Optional value specifying the number of 
**                                      seconds to wait before aborting lock 
**                                      request.
**	value				Optionally, if this is a convert down
**					from LK_X or LK_SIX then this value
**					replaces the value in the lock.
**
** Input/Outputs:
**	lockid				Optional pointer to location to return
**					lock identifier for straight lock grants
**					or to specify existing lockid for lock
**					conversion either explicit or implicit.
**					For a simple request, caller must zero
**					the block to avoid unexpected conversions.
**
** Outputs:
**	lockid				Optional pointer to location to return
**					lock identifier.
**	value				If this is a request other then convert
**					down from LKX or LK_SIZ then return the
**					current value of the lock.
**	Returns:
**	    OK				Lock granted.
**	    LK_NEW_LOCK			Lock granted and never existed before.
**	    LK_BADPARAM			Bad parameter.
**	    LK_NOLOCKS			No available lock memory.
**	    LK_DEADLOCK			Deadlock detected.
**	    LK_TIMEOUT			Lock timed out before being granted.
**	    LK_BUSY			Lock could not immediately be granted.
**	    LK_INTR_GRANT		Lock granted, adn interrupt was
**					delivered.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable logging and locking system.
**	18-jan-1993 (bryanp)
**	    Call CL_CLEAR_ERR to clear the sys_err.
**	24-may-1993 (bryanp)
**	    Check for and handle E_DMA017_LK_MUTEX_RELEASED from LK_release.
**	    Remove useless assignment of LK_NEW_LOCK status when already set.
**	21-jun-1993 (bryanp)
**	    Check for and handle E_DMA017_LK_MUTEX_RELEASED from LK_cancel.
**	    Fixed lock completion status bugs involving timed out and
**		interrupted locks.
**	    Clean up some of the ifdef'd tracing code.
**	01-aug-1996 (nanpr01 for ICL keving)
**	    Add blocking callbacks to LK. Recoded as a call to 
**	    LKrequestWithCallback with NULL callback. Original code
**	    moved to LKrequestWithhCallback.
**      30-Jun-2000 (horda03) X-Int of fix for bug 48904 (VMS only)
**        30-mar-94 (rachael)
**           Added call to CSnoresnow to provide a temporary fix/workaround
**           for sites experiencing bug 48904. CSnoresnow checks the current
**           thread for being CSresumed too early.  If it has been, that is if 
**           scb->cs_mask has the EDONE bit set, CScancelled is called.
*/ 
STATUS
LKrequest(
i4		    flags,
LK_LLID             lock_list_id,
LK_LOCK_KEY         *lock_key,
u_i4               lock_mode,
LK_VALUE            *value,
LK_LKID             *lockid,
i4             timeout,
CL_ERR_DESC         *sys_err)
{
    STATUS status = OK;

    LK_WHERE("LKrequest")

    /* Remove any Rogue Resumes */
    CSnoresnow( "LK!LKRQST.C::LKrequest", 1);

    status = LKrequestWithCallback(flags, lock_list_id, lock_key, lock_mode,
                                   value, lockid, timeout, 
#ifdef xDEBUG
                                   lk_cback_print, (PTR) lock_list_id,
#else 
                                   (LKcallback) NULL, (PTR) NULL, 
#endif
                                   sys_err);
    return(status);
}


/*{
** Name: LKrequestWithCallback  - Request a lock.
**
** Description:
**      This function handles requesting a lock.  The lock can either be 
**      already granted to the lock list or it can be a new request.  If it is  
**      a request for an existing granted lock, the mode is checked to
**	determine if any lock conversion needs to take place to satisfy the
**	request.  The variable 'LK_convert_mode[][]' is a mapping from grant
**	mode and request mode to new mode.  If granted mode equals request mode
**	or new mode equals granted mode then nothing has to happen, otherwise a
**	lock mode conversion is performed.  If this is a request for a new lock
**	that this lock list doesn't currently own then the resource is looked
**	up and the lock is granted if compatible with all other locks on the
**	resource.  The lock request can be directed to abort if the lock can't
**	immediately be granted using the LK_NOWAIT flag, or if the lock has 
**	waited for longer then a interval expressed in seconds or deadlock has 
**	been detected.
**
**	Using the LK_STATUS qualifier an alternate success status will be 
**	returned if this lock did not already belong to the current transaction.
**	The LK_PHYSICAL qualifier is used on locks which will be released 
**	before end of transaction.  Every LKrequest call for this lock by a 
**	transaction must include the LK_PHYSICAL flag.  If the LK_PHYSICAL flag
**	is left off a LKrequest call a convert or a LK_release for this lock 
**	will fail.  LK_CONVERT is used to trigger a conversion for a lock.  A 
**	conversion from a high lock mode to a low lock mode is only legal when 
**	used with the LK_PHYSICAL flag.
**
**	If an address to receive the value is specified, the value stored with 
**	the lock will be returned in that variable when the lock is granted.  
**	If this is a conversion from X or SIX to a lower lock mode, the value 
**	in the callers value variable is stored with the lock.
**
** Unix implementation:
**
**	The vms version of this routine in lkinterface.mar contains a case
**	where LK_request() can return SS$_MBFULL.  This can not happen 
**	currently in the unix implementation, so is not handled in this 
**	external routine.
**
** Callback:
**      A callback facility has been added to LK. This is to support the
**      DMCM buffer manager protocol, but may be useful for other purposes
**      too.
**
**      An LK client can specify a Blocking Callback when it requests (or
**      converts) a lock. If this lock blocks another lock from being granted 
**      then the callback will be run. The callback function is run by an
**      additional thread available in all database servers and the recovery
**      process. The callback function will be run by the Blocking Callback
**      thread in the same process as the thread which originally requested
**      the lock.
**
**      After a callback has been fired it is reset, so it will only be run
**      once. A callback can be replaced on a convert call.
**
**      A lot of care is needed when using callbacks. Some of the implications
**      are not immediately obvious .....
**
** Inputs:
**      flags                           A combination of LK_NOWAIT, LK_STATUS
**					LK_PHYSICAL, LK_IGNORE_LOCK_LIMIT,
**					LK_LOCAL and LK_CONVERT if this is
**					a convert request.
**      lock_list_id                    The lock list identifier returned 
**                                      from a previous call to LK_create_list
**      lock_key                        A pointer to the resource name to lock.
**      lock_mode                       The requested lock mode for the
**					resource.
**      timeout                         Optional value specifying the number of 
**                                      seconds to wait before aborting lock 
**                                      request.
**	value				Optionally, if this is a convert down
**					from LK_X or LK_SIX then this value
**					replaces the value in the lock.
**
** Input/Outputs:
**	lockid				Optional pointer to location to return
**					lock identifier for straight lock grants
**					or to specify existing lockid for lock
**					conversion either explicit or implicit.
**					For a simple request, caller must zero
**					the block to avoid unexpected conversions.
**
** Outputs:
**	value				If this is a request other then convert
**					down from LK_X or LK_SIX then return
**					the current value of the lock.
**	Returns:
**	    OK				Lock granted.
**	    LK_NEW_LOCK			Lock granted and never existed before.
**	    LK_BADPARAM			Bad parameter.
**	    LK_NOLOCKS			No available lock memory.
**	    LK_DEADLOCK			Deadlock detected.
**	    LK_TIMEOUT			Lock timed out before being granted.
**	    LK_BUSY			Lock could not immediately be granted.
**	    LK_INTR_GRANT		Lock granted, and interrupt was
**					delivered.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable logging and locking system.
**	18-jan-1993 (bryanp)
**	    Call CL_CLEAR_ERR to clear the sys_err.
**	24-may-1993 (bryanp)
**	    Check for and handle E_DMA017_LK_MUTEX_RELEASED from LK_release.
**	    Remove useless assignment of LK_NEW_LOCK status when already set.
**	21-jun-1993 (bryanp)
**	    Check for and handle E_DMA017_LK_MUTEX_RELEASED from LK_cancel.
**	    Fixed lock completion status bugs involving timed out and
**		interrupted locks.
**	    Clean up some of the ifdef'd tracing code.
**	14-dec-1994 (medji01)
**	    Mutex Granularity Project
**		- Remove LKD mutex acquisition and free from LKrequest().
**	21-aug-1996 (boama01)
**	    If LK_cancel() returns bad status (not OK or LK_CANCEL), it should
**	    always return immediately, even if we are a Cluster (moved logic
**	    up).  Also, we must fold LK_GRANTED to OK after every call to
**	    LK_get_completion_status(), or it will be seen by callers as error.
**	    Also: uncommented "if (lock_key)" in CSP_DEBUG mode to prevent
**	    access violation when lock_key is null.
**	01-aug-1996 (nanpr01 for ICL keving)
**	    Created to add Blocking Callbacks to LK.
**	01-aug-1996 (nanpr01)
**	    Initialize the status to OK.
**	18-dec-1996 (wonst02)
**	    Add lock_key parameter to CSsuspend call to track locks.
**	08-May-1997 (jenjo02)
**	    Ensure that LK_PROPAGATE flag is off.
**	28-Feb-2002 (jenjo02)
**	    If LK_PHANTOM returned from get_completion_status, retry the 
**	    lock request.
**	12-nov-2002 (horda03) Bug 105321.
**	    Avoid potential for spurious Resumes of a session following a user
**	    interrupt, by suspending the session if LKcancel() returns LK_CANCEL
**	    i.e. the lock request was granted before it could be cancelled, so a
**	    RESUME of the session is pending. Bug 105321 occurs because the
**	    CScancelled() call doesn't delete the pending resume - this can
**	    occur if the resume if from a different process (a CP resume), and
**	    this server hasn't received the CP message yet.
**	26-Jan-2004 (jenjo02)
**	    Remove DB_TAB_PARTITION_MASK from lk_key3 for 
**	    PAGE/ROW locks (unless temp table - lk_key2 < 0)
**	21-nov-2006 (jenjo02/abbjo03)
**	    Handle the ASYNC DOWNGRADE case from enq_complete.
**	21-Feb-2007 (jonj)
**	    Delay taking llb_mutex after CXdlm_wait til we actually need it,
**	    watch for deallocated LKB.
**	25-Feb-2007 (kschendel) SIR 122890
**	    Make sure that non-interruptable suspends are truly
**	    non-interruptable (and can't be timed out, either).
**	21-Mar-2007 (jonj)
**	    Resubmit lock request if status == LK_REDO_REQUEST.
**	15-Nov-2007 (jonj)
**	    If LK_NOTHELD, clear caller's lockid, if any.
**	04-Apr-2008 (jonj)
**	    Exclude LK_U locks if lkd_status & LKD_NO_ULOCKS
**	    rather than conf_CLUSTER_BUILD defined.
**	    Use common LKkey_to_string() function when 
**	    displaying locks instead of lengthy redundant code.
**	09-May-2008 (jonj)
**	    Remove CScancelled() after CXdlm_wait(). Changes to cxdlm
**	    seem to have obviated the need.
**      16-Jun-2009 (hanal04) Bug 122117
**          Make a final dirty check that we have not been force aborted
**          before waiting on a lock that may be part of a LOG/LOCK deadlock.
*/
STATUS
LKrequestWithCallback(
i4		    flags,
LK_LLID             lock_list_id,
LK_LOCK_KEY         *lock_key,
u_i4               lock_mode,
LK_VALUE            *value,
LK_LKID             *lockid,
i4             timeout,
LKcallback          callback,
PTR                 callback_arg,
CL_ERR_DESC         *sys_err)
{
    LKD		*lkd = (LKD *)LGK_base.lgk_lkd_ptr;
    STATUS	status = OK;
    i4		sleep_mask, timer;
    STATUS	sleep_status;
    STATUS	local_status;
    LK_COMP_HANDLE		completion_handle;
    i4			err_code;
    bool		force_abort;

    /* Remove any Rogue Resumes */
    CSnoresnow( "LK!LKRQST.C::LKrequestWithCallback", 1);

    LK_WHERE("LKrequestWithCallback")

    CL_CLEAR_ERR(sys_err);
    /* 
    ** Clear internal LK_PROPAGATE flag to avoid misuse. 
    */
    flags &= ~(LK_PROPAGATE);

    /* Externally (User) set TIMEOUT=NOWAIT? */
    if ( timeout == DMC_C_NOWAIT )
        flags |= LK_NOWAIT;

    completion_handle.blocking_cpid.pid = 0;

    /* Use LK_X instead of LK_U if Update locks not supported (VMS) */
    if ( lock_mode == LK_U && lkd->lkd_status & LKD_NO_ULOCKS )
	lock_mode = LK_X;

    do
    {
	status = LK_request(flags, lock_list_id, lock_key, lock_mode, value,
				    lockid,
				    callback, callback_arg,
				    &completion_handle,
				    sys_err);
	/* If we must wait, set up how to wait */
	if (status == LK_NOINTRWAIT || status == OK)
	{
	    sleep_mask = CS_LOCK_MASK | CS_INTERRUPT_MASK;
	    timer = timeout;

	    /* If non-interruptable, reset interrupt mask, clear timeout */
	    if ( status == LK_NOINTRWAIT &&
		 (flags & LK_INTERRUPTABLE) == 0 )
	    {
		sleep_mask &= ~CS_INTERRUPT_MASK;
		timer = 0;
	    }
	    if (timer > 0)
		sleep_mask |= CS_TIMEOUT_MASK;
	    status = OK;
	}
	/* If DLM lock no longer held, then LKB has been discarded */
	else if ( status == LK_NOTHELD && lockid )
	{
	    lockid->lk_common = 0;
	    lockid->lk_unique = 0;
	}

#ifdef LK_TRACE
	if (LK_rqst_trace())
	{
	    TRdisplay("%@ LKrequest: request issued, status=%x, handle=%p.%d\n",
		status, 
		(LLB *)LGK_PTR_FROM_OFFSET(completion_handle.proc_llb_offset),
		completion_handle.request_id);
	    if (lockid)
		TRdisplay("  lockid: %x.%x", lockid->lk_unique, lockid->lk_common);
	    else
		TRdisplay("  lockid: (not supplied)");
	    if (lock_key)
	    {
		char	keydispbuf[128];
		TRdisplay("  lk_type: %d %s\n",
			lock_key->lk_type,
			LKkey_to_string(lock_key, keydispbuf));
	    }
	    else
		TRdisplay("  lk_type/lk_key: (not supplied)\n");
	    TRdisplay("  flags: %v  mode: %d\n",
	    "NOWAIT,CONVERT,PHYSICAL,STATUS,ESCALATE,,\
    NODLOCK,TEMP,NONINTR", flags, lock_mode);
	    TRflush();
	}
#endif
	if (status == LK_GRANTED)
	{
	    status = OK;
	}
	else if (status == OK)
	{
#ifdef LKD_DEBUG
	    if (lock_key)
		TRdisplay("LKrequest %@ sleeping\n\
     (%x %x %x %x %x %x %x)\n", lock_key->lk_type,
		    lock_key->lk_key1,
		    lock_key->lk_key2,
		    lock_key->lk_key3,
		    lock_key->lk_key4,
		    lock_key->lk_key5,
		    lock_key->lk_key6);
#endif /* LKD_DEBUG */

	    if ( !(lkd->lkd_status & LKD_CLUSTER_SYSTEM) ||
		  (LK_LOCAL & flags) )
	    {
		RSB		*rsb;
		LLB		*llb, *related_llb = (LLB *)0;

                /* Check to see if we have been marked for force abort before 
                ** we suspend
                */
               
		llb = (LLB *)LGK_PTR_FROM_OFFSET(completion_handle.proc_llb_offset);
                if (llb->llb_related_llb)
                {
                    related_llb = (LLB *)
                            LGK_PTR_FROM_OFFSET(llb->llb_related_llb);
                }

                /* We'll dirty read the llb. Testing suggests it's not a
                ** reasonable possibility to get this far into DMF without
                ** the force abort being flagged already.
                */
                if((llb->llb_status & LLB_FORCEABORT) ||
                   (related_llb && (related_llb->llb_status & LLB_FORCEABORT)))
                {
                    sleep_status = E_CS0008_INTERRUPTED;
                    force_abort = TRUE;
                }
                else
                {
                    force_abort = FALSE;
		    /* Suspend until done, interrupt, or timeout.
		    ** Loop only if the CS screws up and delivers an interrupt
		    ** or timeout when we explicitly didn't want one.
		    */
		    do
			sleep_status = CSsuspend(sleep_mask, timer, (PTR)lock_key);
		    while (sleep_status != OK
			  && (sleep_mask & CS_INTERRUPT_MASK) == 0
			  && (sleep_status == E_CS0008_INTERRUPTED
			      || sleep_status == E_CS0009_TIMEOUT));
                }
		if (sleep_status)
		{
		    /*
		    ** We were interrupted or we timed out. Cancel the pending lock
		    ** request (it may be granted before we can do so) and return
		    ** the appropriate lock status
		    */

#ifdef LKD_DEBUG
		    TRdisplay("LKrequest %@ cancel %x sleep_stat was %x\n", 
			    lock_list_id, sleep_status);
#endif /* LKD_DEBUG */

		    status = LK_cancel(lock_list_id, 
					&completion_handle.lkb_offset,
					sys_err);

		    if (status != OK && status != LK_CANCEL)
		    {
			return (status);
		    }

		    if ( (status == LK_CANCEL) &&
			 (CSsuspend(CS_TIMEOUT_MASK, 10, 0) == E_CS0009_TIMEOUT))
		    {
			TRdisplay( "LKrequestWithCallback() - Interrupt for GRANTED lock. No resume received\n");
		    }

		    CScancelled((PTR) 0);

		    if (status == LK_CANCEL)
		    {
			/*
			** We tried to cancel the pending lock request, but before
			** we could do so it had already completed.
			*/
			LK_get_completion_status(&completion_handle, &status,
						 value, sys_err);
			if (status == LK_GRANTED)
			    status = OK;

			/*
			** If lock request succeeded even though an interrupt
			** was given, then return INTERRUPTED anyway so that
			** we don't ignore the interrupt.
			*/
			if ((status == OK || status == LK_NEW_LOCK) &&
			    sleep_status != E_CS0009_TIMEOUT)
			{
			    status = LK_INTR_GRANT;
			}
			
		    }
		    else
		    {
			/*
			** Successfully cancelled the lock: return timeout or
			** interrupt depending on what CSsuspend told us.
			*/
			if (sleep_status == E_CS0009_TIMEOUT)
			    status = LK_TIMEOUT;
			else
                        {
                            if (force_abort)
                                status = LK_INTR_FA;
                            else
			        status = LK_INTERRUPT;
                        }
		    }
		}
		else
		{
		    /*
		    ** We were neither interrupted nor timed out
		    */
#ifdef LKD_DEBUG
		    if (lock_key)
			TRdisplay("LKrequestWithCallback %@ wakeup\n\
(%x %x %x %x %x %x %x)\n", lock_key->lk_type,
			    lock_key->lk_key1,
			    lock_key->lk_key2,
			    lock_key->lk_key3,
			    lock_key->lk_key4,
			    lock_key->lk_key5,
			    lock_key->lk_key6);
#endif /* LKD_DEBUG */

		    LK_get_completion_status(&completion_handle, &status, 
					     value, sys_err);

		    if (status == LK_GRANTED)
			status = OK;
		}
	    }
	    else
	    {
		/* 103715 note:
		**
		** DLM takes care of a lot of the details regarding canceling
		** timed out or interrupted requests, but requires access to 
		** 'lkb_cx_req'.   At one time there was a rigid division
		** in the LK code where only internal routines referenced
		** the internal lock structures ( E.g. LLB, RSB,  & LKB).
		** However, IMHO there is really no compelling reason for this,
		** and a significant amount of overhead would need to be
		** introduced in order to hide the type refs, so I'm completely 
		** ignoring the artificial scoping of the data types, and 
		** putting this code in-line.
		*/
		STATUS	 	MutexStatus;
		u_i4	 	flag;
		LKB		*lkb, *next_lkb, *prev_lkb;
		RSB		*rsb;
		LLB		*llb, *proc_llb;


		proc_llb = 
		    (LLB *)LGK_PTR_FROM_OFFSET(completion_handle.proc_llb_offset);
		lkb = (LKB *)LGK_PTR_FROM_OFFSET(completion_handle.lkb_offset);
		rsb = (RSB *)LGK_PTR_FROM_OFFSET(lkb->lkb_lkbq.lkbq_rsb);
		llb = (LLB *)LGK_PTR_FROM_OFFSET(lkb->lkb_lkbq.lkbq_llb);

		/*
		** "proc_llb" is requestor's LLB.
		** "llb" is the one holding the locks.
		*/

		/* In proc_llb, not llb, note we're waiting on ENQ */
		if ( proc_llb != llb )
		    proc_llb->llb_status |= LLB_ENQWAITING;

		/* One more dist deadlock search in progress */
		if ( !(flags & LK_NODEADLOCK) )
		{
		    lkd->lkd_stat.totl_gdlck_search++;
		    lkd->lkd_stat.gdlck_search++;
		}

		flag = 0;
		if (sleep_mask & CS_INTERRUPT_MASK)
		{
		    flag = CX_F_INTERRUPTOK;
		}
		status = CXdlm_wait( flag, &lkb->lkb_cx_req, timer );

		/*
		** If new locks are stalled and this request is a new
		** grant or up-conversion, then handle it as if
		** a CL2C08 was received, i.e., remove the granted
		** DLM lock and retry the request after the stall is
		** cleared. This prevents requests queued after the
		** lock that received the CL2C08 and triggered the
		** stall from being granted or converted.
		*/
		if ( E_CL2C08_CX_W_SUCC_IVB == status ||
			(status == OK
			 && lkd->lkd_lock_stall 
			 && !(flags & LK_NOSTALL)
			 && lkb->lkb_grant_mode < lkb->lkb_request_mode) )
		{
		    status = lk_handle_ivb( llb, lkb, flags, 
		    		FALSE, flag, timer, __LINE__ );
		}

		/* proc_llb is no longer waiting on the DLM */
		if ( proc_llb != llb )
		    proc_llb->llb_status &= ~LLB_ENQWAITING;

		/* One less dist deadlock search in progress */
		if ( !(flags & LK_NODEADLOCK) )
		    lkd->lkd_stat.gdlck_search--;
		/* Count another asynch request complete */
		lkd->lkd_stat.asynch_complete++;


		switch ( status )
		{
		case E_CL2C01_CX_I_OKSYNC:
		    /* Synchronously granted from lk_handle_ivb */
		    status = OK;
		    break;
		case OK:
		    /* Normal successful completion */
		    break;
		case E_CL2C04_CX_I_OKINTR:
		    /* Successful completion, but interrupt received */
		    status = LK_INTR_GRANT;
		    break;
		case E_CL2C08_CX_W_SUCC_IVB:
		    /*
		    ** Granted, but DLM lock block invalid. If caller is
		    ** interested in the lock value block, then OK status
		    ** is converted to LK_VAL_NOTVALID.
		    */
		    lkb->lkb_cx_req.cx_value[2] = 1;
		    status = OK;
		    break;
		case E_CL2C0A_CX_W_TIMEOUT:
		    /* Request timed out, and DLM lock operation was canceled */
		    status = LK_TIMEOUT;
		    break;
		case E_CL2C0B_CX_W_INTR:
		    /* Request was interrupted and DLM lock op canceled. */
		    status = LK_INTERRUPT;
		    break;
		case E_CL2C21_CX_E_DLM_DEADLOCK:
		    lkb->lkb_attribute |= LKB_DEADLOCKED;
		    status = LK_DEADLOCK;
		    break;
		case E_CL2C39_CX_E_OS_CONFIG_ERR:
		case E_CL2C3A_CX_E_OS_CONFIG_ERR:
		case E_CL2C3B_CX_E_OS_CONFIG_ERR:
		    /* OS resource depletion */
		    status = LK_NOLOCKS;
		    break;
		case LK_REDO_REQUEST:
		    /* lk_handle_ivb could not do down-conversion */
		    break;
		default:
		    /* Unexpected failure. */
		    TRdisplay("%@ enq_complete, lock status was %x\n",
				lkb->lkb_cx_req.cx_status);
		    break;
		}

#ifdef CSP_DEBUG
		if (csp_debug)
		{
		    LK_UNIQUE	lock_id;

		    CX_EXTRACT_LOCK_ID( &lock_id, &lkb->lkb_cx_req.cx_lock_id );
		    TRdisplay(
		      "%@ enq_complete  lkb %p DLM lkid %x.%x stat %d\n", 
			lkb, lock_id.lk_uhigh, lock_id.lk_ulow,
			lkb->lkb_cx_req.cx_status);
		    TRdisplay("  lkb lock value: %4.4{ %x%}\n",
			  lkb->lkb_cx_req.cx_value );
		    TRflush();
		}
#endif

		if ( OK == status || LK_INTR_GRANT == status )
		{
		    /* Request is granted at new mode */

		    if ( MutexStatus = LK_mutex(SEM_EXCL, &rsb->rsb_mutex) )
			return (MutexStatus);

		    lkb->lkb_attribute &= ~(LKB_ENQWAIT |
					    LKB_VALADDR_PROVIDED |
					    LKB_DEADLOCKED |
					    LKB_END_OF_DL_CYCLE);

		    if (lkb->lkb_request_mode >= lkb->lkb_grant_mode)
		    {
			/* Freshen local value from DLM value */
			rsb->rsb_value[0] = lkb->lkb_cx_req.cx_value[0];
			rsb->rsb_value[1] = lkb->lkb_cx_req.cx_value[1];
			rsb->rsb_invalid = lkb->lkb_cx_req.cx_value[2];
		    }

		    if ( value )
		    { 
			value->lk_high = rsb->rsb_value[0];
			value->lk_low = rsb->rsb_value[1];
			value->lk_mode = lkb->lkb_request_mode;
			if (rsb->rsb_invalid)
			    status = LK_VAL_NOTVALID;
		    }

		    /* 103715 note: 
		    **
		    ** Check if request was a waiter before setting NEW_LOCK
		    ** status.  Original b71339 fix broke this logic, since
		    ** it made it possible for a converter to return NEW_LOCK.
		    */
		    if ( OK == status && 
			 LKB_WAITING == lkb->lkb_state &&
			 (flags & LK_STATUS) )
		    {
			status = LK_NEW_LOCK;
		    }

		    prev_lkb = (LKB *)LGK_PTR_FROM_OFFSET(lkb->lkb_q_prev);

		    if (LKB_WAITING == lkb->lkb_state ||
			/* A regular waiting lock request */
			LKB_CONVERT == prev_lkb->lkb_state)
			/* 
			** A convert request, but the previous request is also
			** a convert request! This means that the grant queue
			** has become non-contiguous (bug 71339).
			** In either case, move the request to the head of 
			** the grant queue.  It's an easy place to find.
			*/
		    {
			next_lkb = (LKB *)LGK_PTR_FROM_OFFSET(lkb->lkb_q_next);
			next_lkb->lkb_q_prev = lkb->lkb_q_prev;
			prev_lkb->lkb_q_next = lkb->lkb_q_next;

			lkb->lkb_q_next = rsb->rsb_grant_q_next;
			lkb->lkb_q_prev =
			 LGK_OFFSET_FROM_PTR(&rsb->rsb_grant_q_next);
			next_lkb =
			 (LKB *)LGK_PTR_FROM_OFFSET(rsb->rsb_grant_q_next);
			next_lkb->lkb_q_prev =
			    rsb->rsb_grant_q_next = LGK_OFFSET_FROM_PTR(lkb);
		    }

		    if ( lockid )
		    {
			lockid->lk_unique = lkb->lkb_id;
			lockid->lk_common = rsb->rsb_id;
		    }

		    switch ( lkb->lkb_state )
		    {
		    case LKB_CONVERT:
			/* one less converter. */
			rsb->rsb_converters--;
			break;
		    case LKB_WAITING:
			/* one less waiter. */
			rsb->rsb_waiters--;
			break;
		    default:
			/* Unexpected value.  Major logic lapse. */
			TRdisplay("%@ Saw unexpected lkb_state (%d) @%s:%d\n",
			  lkb->lkb_state, __FILE__, __LINE__ );
			/* Let hell break loose, so we don't overlook this. */
			status = LK_UNEXPECTED;
		    }

		    lkb->lkb_state = LKB_GRANTED;

		    /* 103715 note:
		    **
		    ** Concern raised in old comment above is valid.  If a
		    ** new lock, group modes should only change if mode
		    ** granted is stronger.  If a conversion, mode may
		    ** theoretically decrease ( I'd expect downgrades to
		    ** be synchronously completed, but since we don't control
		    ** all the DLM's Ingres may possibly use, we should be
		    ** prepared ), in which case we should recalculate the
		    ** group modes if the previously granted mode matches
		    ** the group mode.
		    */
		    if ( lkb->lkb_request_mode >= rsb->rsb_grant_mode )
		    {
			rsb->rsb_grant_mode = rsb->rsb_convert_mode = 
			 lkb->lkb_request_mode;
		    }
		    else
		    {
			LKB             *next_lkb;
			SIZE_TYPE       lkb_offset;
			SIZE_TYPE       end_offset = 
				LGK_OFFSET_FROM_PTR(&rsb->rsb_grant_q_next);
			i4              group_mode = LK_N;
			i4              convert_mode = LK_N;

			/* We're on the grant queue now */
			lkb->lkb_grant_mode = lkb->lkb_request_mode;

			/* Recompute group grant and convert modes */
			for ( lkb_offset = rsb->rsb_grant_q_next;
			      lkb_offset != end_offset;
			      lkb_offset = next_lkb->lkb_q_next )
			{
			    next_lkb = (LKB*)LGK_PTR_FROM_OFFSET(lkb_offset);

			    if ( next_lkb->lkb_grant_mode > group_mode )
				group_mode = next_lkb->lkb_grant_mode;
			    if ( next_lkb->lkb_state == LKB_CONVERT )
			    {
				convert_mode = next_lkb->lkb_grant_mode;
				break;
			    }
			}
			if ( rsb->rsb_converters == 0 )
			    convert_mode = group_mode;
			     
			rsb->rsb_grant_mode = group_mode;
			rsb->rsb_convert_mode = convert_mode;
		    }
		    lkb->lkb_grant_mode = lkb->lkb_request_mode;

		    /* Maintain rsb_mutex while we "resume" */
		    LK_resume(lkb);

		    (void)LK_unmutex(&rsb->rsb_mutex);

		    /* Count another successful dist grant */
		    lkd->lkd_stat.gdlck_grant++;
		}
		else 
		{
		    /* Request failed. */ 
		    if ( !completion_handle.blocking_cpid.pid )
		    {
			/* Give blocking_cpid every opportunity to be filled. */
			completion_handle.blocking_cpid =
			 lkb->lkb_blocking_cpid;
			completion_handle.blocking_mode =
			 lkb->lkb_blocking_mode;
		    }
		    
		    /* Cleanup lock request, ignore status */
		    (VOID)LK_cancel(lock_list_id, 
				    &completion_handle.lkb_offset,
				    sys_err);
		}
	    }
	}
    } while ( status == LK_PHANTOM || status == LK_REDO_REQUEST );

    /* If lock could not be granted, return pid/sid/mode of blocker */
    if ( status && (status == LK_BUSY || status == LK_DEADLOCK ||
                    status == LK_TIMEOUT) )
    {
        STprintf(sys_err->moreinfo[0].data.string, 
#ifdef VMS /* Show process id in hex */
		"[%x %p] Mode %d",
#else
		"[%d %p] Mode %d",
#endif
            completion_handle.blocking_cpid.pid,
            completion_handle.blocking_cpid.sid,
            completion_handle.blocking_mode);
        sys_err->moreinfo[0].size = STlength(sys_err->moreinfo[0].data.string);
        /* Transform "busy" status from internal to external */
        if ( status == LK_BUSY && timeout == DMC_C_NOWAIT )
            status = LK_UBUSY;
    }

    return(status);
}

/*{
** Name: LK_request	- Request a lock.
**
** Description:
**      This function handles requesting a lock.  The lock can either be 
**      already granted to the lock list or it can be a new request.  If it is  
**      a request for an existing granted lock, the mode is checked to
**	determine if any lock conversion needs to take place to satisfy the
**	request.  The variable 'LK_convert_mode[][]' is a mapping from grant
**	mode and request mode to new mode.  If granted mode equals request mode
**	or new mode equals granted mode then nothing has to happen, otherwise a
**	lock mode conversion is performed.  If this is a request for a new lock
**	that this lock list doesn't currently own then the resource is looked
**	up and the lock is granted if compatible with all other locks on the
**	resource.  The lock request can be directed to abort if the lock can't
**	immediately be granted using the LK_NOWAIT flag, or if the lock has 
**	waited for longer then a interval expressed in seconds or deadlock has 
**	been detected.
**
**	Using the LK_STATUS qualifier an alternate success status will be 
**	returned if this lock did not already belong to the current transaction.
**	The LK_PHYSICAL qualifier is used on locks which will be released before
**	end of transaction.  Every LKrequest call for this lock by a transaction
**	must include the LK_PHYSICAL flag.  If the LK_PHYSICAL flag is left off
**	a LKrequest call a convert or a LK_release for this lock will fail.  
**	LK_CONVERT is used to trigger a conversion for a lock.  A conversion 
**	from a high lock mode to a low lock mode is only legal when used with 
**	the LK_PHYSICAL flag.
**
**	If an address to receive the value is specified, the value stored with 
**	the lock will be returned in that variable when the lock is granted.  
**	If this is a conversion from X or SIX to a lower lock mode, the value 
**	in the callers value variable is stored with the lock.
**
**	A word on LLB_MULTITHREAD lock lists:
**
**	A lock list may be created explicitly as a multithreaded list or
**	implicitly by connecting a new handle to an extant lock list as seen
**	in LKcreate_list(). These LLBs will have a status of
**	LLB_MULTITHREAD and will be the lock list on which the locks (LLBs)
**	are queued.
**
**	The best example of a multithreaded list is the one used to manage
**	the DMF cache. Other examples of multithreaded lists are those 
**	created when running a parallel query.
**
**	The assertion with LLB_MULTITHREAD lists is that multiple threads
**	will make concurrent requests for new locks, conversions of existing 
**	locks, and lock releases, either implicitly by way of a handle LLB
**	or explicitly by direct reference to the multithreaded LLB's llb_id.
**	To encourage concurrency, the multithreaded LLB's mutex will only be
**	acquired when adding or removing a lock to the list; in all other
**	cases (physical conversions, releases) the llb_mutex will not be taken
**	and instead the resource itself (RSB) will be mutexed.
**	resource itself (RSB) will be mutexed.
**
**	DMF cache locks are typically up- and down-converted in a torrent
**	of activity; removing the llb_mutex from contention should increase
**	concurrency in the cache.
**
**	With non-multithreaded lists, the llb_mutex is taken right away
**	for no other reason than to prevent it from being converted to a
**	multithreaded list during parallel query. As these lists are by 
**	definition only exposed to a single thread, there will be no block
**	on the llb_mutex.
**
**	
**
** Inputs:
**      flags                           LK_NOWAIT - Don't wait if lock cannot
**					    be granted immediately
**					LK_PHYSICAL - request a physical lock
**					LK_IGNORE_LOCK_LIMIT - escalating
**				            lock level
**					LK_CONVERT - lock conversion request
**					LK_STATUS
**      lock_list_id                    The lock list identifier returned 
**                                      from a previous call to LK_create_list
**      lock_key                        A pointer to the resource name to lock.
**      lock_mode                       The requested lock mode for the
**					resource.
**	value				Optionally, if this is a convert down
**					from LK_X or LK_SIX then this value
**					replaces the value in the lock.
**      timeout                         Optional value specifying the number of 
**                                      seconds to wait before aborting lock 
**                                      request.
**      callback_fn 
**      callback_arg
**
** Input/Outputs:
**	lockid				Optional pointer to location to return
**					lock identifier for straight lock grants
**					or to specify existing lockid for lock
**					conversion either explicit or implicit.
**					For a simple request, caller must zero
**					the block to avoid unexpected conversions.
**
** Outputs:
**	value				If this is a request other then convert
**					down from LK_X or LK_SIX then return
**					the current value of the lock.
**	sys_err				internal error info.
**	completion_handle		Magic handle that can be passed back
**					in to LK_get_completion_status to get
**					the completion status, lockid, and value
**					of the lock once it's been granted.
**	Returns:
**	    OK				Lock could not be immediately granted,
**					caller must suspend.
**	    LK_GRANTED			Lock granted.
**	    LK_NEW_LOCK			Lock granted and never existed before.
**	    LK_BADPARAM			Bad parameter.
**	    LK_NOLOCKS			No available lock memory.
**	    LK_DEADLOCK			Deadlock detected.
**	    LK_TIMEOUT			Lock timed out before being granted.
**	    LK_BUSY			Lock could not immediately be granted.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable logging and locking system.
**	24-may-1993 (bryanp)
**	    Mutex optimizations -- drop the mutex before sending the message
**		to the CSP so that we're not blocked holding the mutex.
**	    Set lkreq.pid to this process's Process ID.
**	    Re-position check for llkb_count so that we are more precise in our
**		enforcement of the max locks per lock list limit.
**	26-jul-1993 (bryanp)
**	    Added some casts to pacify compilers.
**	20-sep-1993 (bryanp) merged 23-mar-1993 (bryanp)
**	    Fix a number of bugs surrounding handling of invalid lock values:
**	    4) VMS doesn't support marking a resource as value-not-valid when
**		converting down from >=SIX and not providing the value. Since
**		we don't need this functionality, we'll return LK_BADPARAM
**		instead of trying to mark the value invalid (and ending up
**		marking it locally invalid but globally valid).
**	02-feb-1994 (pearl)
**	    Bug 57376.  In LK_request(), if LK_VAL_NOTVALID is returned
**	    from the call to LK_request_lock(), we should call
**	    LK_complete_request() to ensure that we have a valid lock mode
**	    in the lock value block (LK_VALUE).
**	14-dec-1994 (medji01)
**	    Mutex Granularity Project
**		- Remove LKD mutex free and reacquisition around 
**		  LK_csp_send().
**	28-mar-1995 (medji01)
**	    64K+ Lock Limit Project
**		- Removed references to id_instance in LK_ID.
**		- Changed error messages to reference id_id instead
**		  of id_instance.
**	06-july-1995 (thaju02)
**	    Added checks of LK_OVERRIDE_NIW bit, implementation
**	    of -max_stall_time=... argument for ckpdb.
**      09-aug-1995 (thaju02)
**          Back out of changes made during implementation of ckpdb
**          stall time feature.  Removed LK_OVERRIDE_NIW checking,
**          instead ckpdb creates a new lock list.
**	08-Oct-1996 (jenjo02)
**	    Wrapped updating of lkd_rqst_id in lkd_mutex.
**	01-aug-1996 (nanpr01 for ICL keving)
**          Add callback and callback_arg arguments, pass on as required.
**	01-aug-1996 (nanpr01)
**	    Initialize lkreq.status to 0.
**	06-mar-1997 (nanpr01)
**	    Clustered lock request fails in LK_get_completion status
**	    since request_id does not match.
**	08-May-1997 (jenjo02)
**	    Code changes to support LK_propagate(). When called with
**	    LK_PROPAGATE flag, the LLB's mutex has already been taken.
**	15-May-1997 (jenjo02)
**	   Added LK_INTERRUPTABLE flag, used to make this request interruptable
**	   even if the lock list is marked non-interruptable.
**	19-May-1997 (jenjo02)
**	   Skip LK_NOLOCKS check if propagating.
**      05-Nov-1998 (hanal04)
**          Record the sid and pid in the completion handle for added
**          protection against use of wrong lkb->lkreq in
**          LK_get_completion_status. (b90388). Crossed from 1.2/01.
**	08-Feb-1998 (jenjo02)
**	  o Record CS_cpid of lock requestor in LKREQ at time of request
**	    instead of waiting til suspend. This caused problems in clusters.
**	  o Check for "conversions" to the same mode, grant immediately. 
**	28-Feb-2002 (jenjo02)
**	    LK_MULTITHREAD is now an attribute of a lock list as well as
**	    a run-time flag. Added llb_status to text of E_CL103E.
**	22-Jun-2004 (jenjo02)
**	    Shared and multithreaded lock lists must mutex the
**	    resource's RSB before changing the physical pin count.
**
** History from LK_request_lock():
** 
**	1992 (bryanp)
**	    created.
**	16-feb-1993 (bryanp)
**	    Added history section. Fixed cluster bugs. Moved setting of lockid
**	    in lkreq from after convert() call to before convert() call so that
**	    if convert awakens the lock requestor, we're sure that we've set
**	    the lkreq fields before the requestor is awoken.
**	24-may-1993 (bryanp)
**	    Re-position check for llkb_count so that we are more precise in our
**		enforcement of the max locks per lock list limit.
**	    Maintain synch_complete and asynch_complete statistics.
**	21-jun-1993 (bryanp)
**	    Remove "status_addr", "value_addr", and "timeout" arguments
**		from LK_suspend.
**	23-aug-1993 (andys)
**	    Initialize lkb->lkb_type so that it will be reused correctly
**	    if freed early, e.g. when an EXENQLM occurs. 
**	    Pass flag to LK_request_new_lock asking it to free resources
**	    if a failure occurs. 
**	23-aug-1993 (rogerk)
**	    Move check for exceeding logical lock limit to after where we
**	    check for an implicit lock conversion - or for a rerequest of
**	    a lock we already hold.  We don't want to return OUT-OF-LOCKS
**	    in these conditions.
**	14-dec-1994 (medji01)
**	    Mutex Granularity Project
**		- Latch RSH mutex during search
**		- Latch LKD mutex around LK_allocate_cb() and 
**		  LK_deallocate_cb() calls
**		- Latch RSH when inserting a new RSB into the hash queue.
**	22-aug-1995 (dougb)
**	    As a related update to the fix for bug #70696, update the
**	    parameters to LK_do_new_lock_request().
**	    Init more of an LKB structure to ease debugging.
**	12-sep-1995 (canor01)
**	    Mutex Granularity Project
**		- Acquire the LKD mutex before calls to LK_try_wait()
**		  and LK_try_convert()
**	13-sep-1995 (dougb)
**	    LK_suspend() the LKB *before* returning when on a cluster.
**	12-Dec-1995 (jenjo02)
**	    LK_allocate_cb, LK_deallocate_cb made responsible
**	    for their own mutex protection - no longer necessary
** 	    to acquire LKD mutex prior to call.
**	    Added a host of new list/queue mutexes to relieve the
**	    pressure on lkd_mutex.
**	20-Dec-1995 (jenjo02)
**	    Reversed order of RSH, LKH hash searches. System stats
**	    show a great percentage of requests are "re-requests"
**	    for the same resource. Searching in the LKH->RSB 
**	    direction tells us at once whether the LLB already
**	    has a lock on the resource, saving a RSH search if
**	    a matching RSB is found.
**	29-dec-1995 (dougb) bug 74007
**	    Get new LK_get_completion_status() performance feature working
**	    when it's called from LK_request().  Ensure output_lkb is
**	    updated in all sucessful cases.
**	30-may-1996 (pchang)
**	    Added LLB as a parameter to the call to LK_allocate_cb() to enable
**	    the reservation of SBK table resource for use by the recovery
**	    process. (B76879)
**	21-jun-1996 (pchang)
**	    Take the new RSB off the hash queue before freeing it, should 
**	    allocation of a new LKB fail.  (B77374)
**	01-jul-1996 (pchang)
**	    Another instance the above bug was found.  Instead of duplicating
**	    the above change, we now delay insertion of the new RSB into the
**	    hash queue until we have succeeded in allocating a new LKB.  This
**	    also closes a small timing window in which the new RSB could be
**	    referenced by another session while we're still building the LKB.
**	23-jul-96 (nick)
**	    Catch implicit conversions which change lock state from physical
**	    to logical.
**	21-aug-1996 (boama01)
**	    Make sure all taken mutexes (eg., LLB) released before all
**	    returns, even in error situations (even mutex failures); a mutex
**	    deadlock is no way to report problems or hang the transaction!
**	    Also fix unbalanced unmutexes for clusters.
**	    Also changed release_mutex parm of LK_do_new_lock_request() call
**	    to FALSE from TRUE; only 1 LK_wakeup in that routine uses this
**	    parm, and all other wakeups are hard-coded as FALSE, so let's
**	    be consistent.
**	    Also, if LK_do_new_lock_request deallocates the RSB, it may do
**	    the same with the LKB, so you can't test RSB ptr within LKB;
**	    since this occurs only on errors, we can set a "bogus"
**	    LKREQ.function value for _DEBUGGER, and test that here to tell
**	    us both blocks are gone.
**	05-Sep-1996 (jenjo02)
**	    While spinning on dlock_mutex, release llb_mutex as well.
**	11-sep-96 (nick)
**	    Changes to stop leakage of RSBs and LKBs. #78432
**	13-dec-96 (cohmi01)
**	    Add sys_err parm for passing back error information. (Bug 79763)
**	01-aug-1996 (nanpr01 for ICL keving)
**	    Copy callback fn and args to newly created LKBs.
**      22-nov-1996 (dilma04)
**          Row-Level Locking Project:
**          Removed two lines from the Nick's fix for bugs 77890 and 76463
**          which used to return LK_NEW_LOCK status when physical_to_logical
**          conversion request is waiting. This made it possible to request
**          a new lock while waiting and caused LK_REQUEST_BAD_PARAM error. 
**      12-dec-96 (dilma04)
**         If a resource is locked by a logical lock and a physical lock
**         is being requested, a LOGICAL lock must be granted.
**	06-mar-1997 (nanpr01)
**	    Clustered lock request fails in LK_get_completion status
**	    since request_id does not match.
**	22-Apr-1997 (jenjo02)
**	   Removed LKREQ_CB_DEALLOCATED flag, whose use was causing
**	   memory leaks when the LKB it was being set in was deallocated.
**	   Instead, changed dequeue_cbs() to be passed a pointer to int
**	   "res_to_free" in which flags will be set indicating which, if
**	   any resources were actually freed.
**      07-May-1997 (dilma04)
**         Bug 81895. Return LK_COV_LOCK if an intend page lock is converted
**         to a covering mode.
**	08-May-1997 (jenjo02)
**	    Code changes to support LK_propagate(). When called with
**	    LK_PROPAGATE flag, the LLB's mutex has already been taken,
**	    but we'll free it before returning.
**      14-may-1997 (dilma04)
**          Return LK_LOG_LOCK if a physical lock has been converted to
**          logical.
**      13-jun-1997 (dilma04)
**           Zero lkb_count, if physical lock has been converted to logical.
**	08-Feb-1998 (jenjo02)
**	  o Using requestor's input lockid (if any), check for an implicit
**	    conversion and avoid a search of the LKH.
**      20-Aug-1999 (hanal04) Bug 97316 INGSRV921
**           Check for llb_lkb_count hard limit and return LK_NOLOCKS
**           if we are going to exceed it. Check assumes llb->llb_lkb_count
**           is signed and will become negative when it wraps.
**	30-aug-1999 (nanpr01)
**	     Turn off the LKB_PHYSICAL flag only once.
**	15-Jan-2002 (jenjo02)
**	    Another instance of B106306, this one involving LLB and RSH
**	    mutexes. Releasing LLB mutex while allocating RSB/LKB solves
**	    all such mutex deadlocks.
**	16-Jan-2002 (jenjo02)
**	    Backed out 15-Jan-2002 change for B106306. It introduces new,
**	    more exotic problems.
**	28-Feb-2002 (jenjo02)
**	     Check for suspect RSB after LKB allocation and retry RSB/LKB
**	     allocation if RSB is no longer valid (B106306).
**	07-Mar-2002 (jenjo02)
**	     Clear "matching_rsb" at top of "do" loop search for RSB.
**    12-Nov-2002 (jenjo02)
**         Instance of B106306 found involving RSH/RSB allocation
**         mutex deadlock. Solution is identical to LKB/RSB
**         deadlock resolution (if RSH mutex freed, toss RSB,
**         retry).
**      19-Dec-2002 (jenjo02)
**           When solution, above, causes RSB/LKB to be freed,
**           type it with "RSB_TYPE", "LKB_TYPE" as rsb_type,
**           lkb_type have yet to be populated.
**	20-may-2004 (devjo01)
**	    Need to be able to convert downward even if stalled,
**	    otherwise undetected deadlock is possible with RCP
**	    trying to fault in a page, but never getting the cache
**	    lock for the page because a DBMS attached to another
**	    cache containing the page forces the page after getting
**	    a blocking notification, but stalls trying to downgrade
**	    his page cache lock.
**	10-Aug-2004 (jenjo02)
**	    After mutexing the LLB, check that it may have been
**	    converted to a SHARED list; if so, switch to
**	    that list.
**	22-Apr-2005 (jenjo02)
**	    Pin RSB to prevent its deallocation while this request
**	    is stalled.
**	13-Mar-2006 (jenjo02)
**	    Deprecated lkd_rqst_id, use llb_stamp instead.
**	04-Apr-2008 (jonj)
**	    Reject conversion requests on LKBs that are already
**	    waiting, with LK_BADPARAM.
**
** End of history from LK_request_lock()
**
**	08-Jan-2009 (jonj)
**	    Incorporated LK_request_lock() here.
**	    Don't mutex LLB_MULTITHREAD lists unless adding a new lock,
**	    mutex the RSB instead.
**	    Eliminate use of LKREQ structure.
**	31-Jan-2009 (jonj)
**	    Oops, when checking for an implicit conversion to the same
**	    mode, be sure to include the checking of the mode.
**	29-Sep-2009 (jonj)
**	    When checking for implicit conversion in a multithreaded
**	    lock list, llb_mutex must be held to block concurrent changes
**	    to the LLB's list of locks.
**	11-Oct-2009 (jonj)
**	    llb_mutex isn't quite right. As the lkb_hash is exposed to 
**	    multiple concurrent threads, search the hash while holding
**	    its mutex.
**	15-Oct-2009 (jonj)
**	    When restarting lkb hash search, forgot to reset
**	    to the start of the list, lkb_hash_bucket.
**	18-Nov-2009 (jonj)
**	    For implicit conversion on a MULTITHREAD list, if the
**	    LKB is not found in the hash, keep the mutex on 
**	    lkh_hash until the lock is added to the hash. 
**	    This keeps concurrent users of the LLB from adding the 
**	    same lock.
**	02-Dec-2009 (jonj)
**	    Gad, that last attempt introduced a LLB/LKH mutex deadlock
**	    with LK_do_unlock when one multithreaded thread is
**	    escalating (getting rid of its page locks) and another
**	    thread is adding new page locks to the same list, alternately
**	    a mutex deadlock with LKconnect between the same mutexes.
**	    Rework and document llb/lkh/rsb mutex interactions here when 
**	    looking for implicit conversions.
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Keep stats by lock type.
**      28-Sep-2010 (horda03)
**          The wrong value (LK_NOINTERRUPT) was being used to check for a
**          lock list that was not interruptable, this could allow lock
**          requests to be interrupted that shouldn't be (and vice versa).
*/
static STATUS
LK_request(
i4		    flags,
LK_LLID		    lock_list_id,
LK_LOCK_KEY	    *lock_key,
u_i4		    lock_mode,
LK_VALUE	    *value,
LK_LKID		    *lockid,
LKcallback          callback,
PTR                 callback_arg,
LK_COMP_HANDLE 	    *completion,
CL_ERR_DESC	    *sys_err)
{
    LKD                 *lkd = (LKD *)LGK_base.lgk_lkd_ptr;
    LLB                 *llb;
    LLB                 *proc_llb;
    LKB                 *lkb;
    RSB                 *rsb;
    LLB_ID		*input_list_id = (LLB_ID *)&(lock_list_id);
    SIZE_TYPE		*lbk_table;
    SIZE_TYPE		*sbk_table;
    RSH			*rsh_table;
    LKH			*lkh_table;
    SIZE_TYPE		end_offset;
    SIZE_TYPE		llb_offset;
    SIZE_TYPE		lkh_offset;
    SIZE_TYPE		lkbq_offset;
    STATUS		status;
    i4			err_code;
    STATUS		MutexStatus;
    bool		matching_rsb;
    RSB			*next_rsb;
    RSB			*prev_rsb;
    LKBQ		*lkbq;
    LKH 		*lkb_hash_bucket;
    RSH			*rsb_hash_bucket;
    u_i4		rsb_hash_value;
    i4			hash_chain_length;
    bool    		FreeRsbIfFail;
    bool		physical_to_logical;
    bool    		intend_to_covering;
    LKBQ		*next_lkbq, *prev_lkbq;
    LLBQ		*next_llbq, *prev_llbq;
    LKB			*prev_lkb;

    LK_WHERE("LK_request")

    /* Validate lock_mode is within range and lock list is valid. */

    if (lock_mode > LK_X || input_list_id->id_id == 0 ||
	(i4) input_list_id->id_id > lkd->lkd_lbk_count)
    {   
	uleFormat(NULL, E_CL103D_LK_REQUEST_BAD_PARAM, (CL_ERR_DESC *)NULL, ULE_LOG,
			NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 3,
			0, input_list_id->id_id, 0, lkd->lkd_lbk_count,
			0, lock_mode);
       return (LK_BADPARAM);
    }

    lbk_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lkd->lkd_lbk_table);
    llb = (LLB *)LGK_PTR_FROM_OFFSET(lbk_table[input_list_id->id_id]);

    if ( llb->llb_type != LLB_TYPE ||
	llb->llb_id.id_instance != input_list_id->id_instance ||
	(llb->llb_waiters && (llb->llb_status & LLB_MULTITHREAD) == 0
	 && !(flags & LK_PROPAGATE)) )
    {
	uleFormat(NULL, E_CL103E_LK_REQUEST_BAD_PARAM, (CL_ERR_DESC *)NULL, ULE_LOG,
			NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 4,
			0, *(u_i4*)input_list_id, 0, llb->llb_type,
			0, *(u_i4*)&llb->llb_id, 
			0, llb->llb_status);
	return (LK_BADPARAM);
    }

    /*
    ** If lock list is a handle to a shared list, then use the actual shared
    ** list llb for the lock request.  Save the handle llb to send to the
    ** suspend routine as it contains this process' PID, AST, and ASTP parms.
    */
    proc_llb = llb;

    /* Return handle LLB to caller */
    completion->proc_llb_offset = LGK_OFFSET_FROM_PTR(proc_llb);

    if (llb->llb_status & LLB_PARENT_SHARED)
    {
	if (llb->llb_shared_llb == 0)
	{
	    uleFormat(NULL, E_CL1055_LK_REQUEST_BADPARAM, (CL_ERR_DESC *)NULL, ULE_LOG,
			NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 1,
			0, input_list_id->id_id);
	    return (LK_BADPARAM);
	}

	llb = (LLB *)LGK_PTR_FROM_OFFSET(llb->llb_shared_llb);

	if ( llb->llb_type != LLB_TYPE ||
	    (llb->llb_status & LLB_SHARED) == 0 )
	{
	    uleFormat(NULL, E_CL1055_LK_REQUEST_BADPARAM, (CL_ERR_DESC *)NULL, ULE_LOG,
			NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 3,
			0, input_list_id->id_id, 0, llb->llb_type, 
			0, llb->llb_status);
	    return (LK_BADPARAM);
	}
	/* The shared LLB will be LLB_MULTITHREAD */
    }

    llb_offset = LGK_OFFSET_FROM_PTR(llb);

    /*  Process convert requests. */

    if (flags & LK_CONVERT)
    {
	if (lockid == NULL)
	{
	    uleFormat(NULL, E_CL103F_LK_REQUEST_BAD_PARAM, (CL_ERR_DESC *)NULL, ULE_LOG,
			NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 0);
	    return (LK_BADPARAM);
	}

	/*
	**	The convert request is valid if the lockid is within range
	**	and the LKB is active and the LKB is owned by this callers
	**	LLB and the request specified PHYSICAL and the LKB has been
	**	used for PHYSICAL requests.
	*/

	if (lockid->lk_unique == 0 ||
	    lockid->lk_unique > lkd->lkd_sbk_count)
	{
	    uleFormat(NULL, E_CL1040_LK_REQUEST_BAD_PARAM, (CL_ERR_DESC *)NULL, ULE_LOG,
			NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 2,
			0, lockid->lk_unique, 0, lkd->lkd_sbk_count);
	    return (LK_BADPARAM);
	}

	sbk_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lkd->lkd_sbk_table);
	lkb = (LKB *)LGK_PTR_FROM_OFFSET(sbk_table[lockid->lk_unique]);
	rsb = (RSB *)LGK_PTR_FROM_OFFSET(lkb->lkb_lkbq.lkbq_rsb);

	if (lkb->lkb_type != LKB_TYPE ||
	    lkb->lkb_lkbq.lkbq_llb != llb_offset ||
	    !rsb || rsb->rsb_id != lockid->lk_common ||
	    (flags & LK_PHYSICAL) == 0 ||
	    (lkb->lkb_attribute & LKB_PHYSICAL) == 0)
	{
	    uleFormat(NULL, E_CL1041_LK_REQUEST_BAD_PARAM, (CL_ERR_DESC *)NULL, ULE_LOG,
			NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 3,
			0, lkb->lkb_id, 0, lkb->lkb_type, 
			0, lockid->lk_unique);
	    return (LK_BADPARAM);
	}

	if (lkb->lkb_grant_mode >= LK_SIX && lock_mode < lkb->lkb_grant_mode)
	{
	    if (value == NULL)
	    {
		/*
		** The old lock manager used to support this operation, and
		** interpret it as "convert the lock down to a lower mode, and
		** at the same time mark the value as invalid." However, this
		** is not supported by the VMS Distributed Lock Manager. That
		** is, the only time that a holder of the lock can explicitly
		** mark the value invalid is when the lock is dequeued.
		** Therefore, the lock manager now rejects this request as
		** illegal, which I believe is OK because I don't think we ever
		** actually use this functionality.
		*/
		return (LK_BADPARAM);
	    }
	}

	/* Mutex the RSB if multithreaded list */
	if ( llb->llb_status & LLB_MULTITHREAD )
	{
	    if ( rsb )
	    {
		if ( MutexStatus = LK_mutex(SEM_EXCL, &rsb->rsb_mutex) )
		    return(MutexStatus);

		/* Recheck everything after waiting for mutex */
		if ( lkb->lkb_lkbq.lkbq_llb != llb_offset ||
		     lockid->lk_unique != lkb->lkb_id ||
		     lockid->lk_common != rsb->rsb_id ||
		     (flags & LK_PHYSICAL) == 0 ||
		     (lkb->lkb_attribute & LKB_PHYSICAL) == 0 )
		{
		    (VOID)LK_unmutex(&rsb->rsb_mutex);
		    rsb = (RSB*)NULL;
		}
	    }
	}

	if ( !rsb )
	{
	    uleFormat(NULL, E_CL1041_LK_REQUEST_BAD_PARAM, 
		    (CL_ERR_DESC *)NULL, ULE_LOG,
		    NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 3,
		    0, lkb->lkb_id, 0, lkb->lkb_type, 
		    0, lockid->lk_unique);
	    return (LK_BADPARAM);
	}

	/* If "converting" to granted mode, grant without furthur ado. */
	/*
	** s103715 note.  Only if not clustered, or if mode held precludes
	** updaters from other nodes.  Without this, buffer page checks
	** would fail to see that page version had been updated on another
	** node, since page in cache is normally held in IS, which allows
	** page updaters to modify the page on the remote node.
	*/
	if (lkb->lkb_grant_mode == lock_mode &&
	    lkb->lkb_state == LKB_GRANTED &&
	    (lkb->lkb_grant_mode > LK_IS || 
	     !(lkb->lkb_attribute & LKB_DISTRIBUTED_LOCK)))
	{
	    status = LK_GRANTED;

	    if (value)
	    {
		value->lk_high = rsb->rsb_value[0];
		value->lk_low  = rsb->rsb_value[1];
		value->lk_mode = lock_mode;
		if (rsb->rsb_invalid)
		    status = LK_VAL_NOTVALID;
	    }

	    if ( llb->llb_status & LLB_MULTITHREAD )
		(VOID)LK_unmutex(&rsb->rsb_mutex);

	    return(status);
	}
	/* continue on with "LKB" */
    }
    else
    {
	/*
	** At this point we have eliminated an explicit convert request.
	*/

	/* Ensure that a lock_key was passed in. */

	if (lock_key == NULL)
	{
	    uleFormat(NULL, E_CL1042_LK_REQUEST_BAD_PARAM, (CL_ERR_DESC *)NULL, ULE_LOG,
			    NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 0);
	    return (LK_BADPARAM);
	}

	/* Check for an implicit "conversion" to the already granted mode. */
	if (lockid &&
	    lockid->lk_unique && lockid->lk_unique <= lkd->lkd_sbk_count &&
	    lockid->lk_common && lockid->lk_common <= lkd->lkd_rbk_count)
	{
	    sbk_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lkd->lkd_sbk_table);
	    lkb = (LKB *)LGK_PTR_FROM_OFFSET(sbk_table[lockid->lk_unique]);
	    rsb = (RSB *)LGK_PTR_FROM_OFFSET(lkb->lkb_lkbq.lkbq_rsb);

	    /*
	    ** Ensure that what we think is an LKB based on what was passed in
	    ** lockid belongs to this LLB and matches the lock request.
	    */

	    if ( lkb->lkb_type == LKB_TYPE && 
	         lkb->lkb_lkbq.lkbq_llb == llb_offset &&
		 rsb && rsb->rsb_id == lockid->lk_common &&
		 rsb->rsb_name.lk_type == lock_key->lk_type &&
		 rsb->rsb_name.lk_key1 == lock_key->lk_key1 &&
		 rsb->rsb_name.lk_key2 == lock_key->lk_key2 &&
		 rsb->rsb_name.lk_key3 == lock_key->lk_key3 &&
		 rsb->rsb_name.lk_key4 == lock_key->lk_key4 &&
		 rsb->rsb_name.lk_key5 == lock_key->lk_key5 &&
		 rsb->rsb_name.lk_key6 == lock_key->lk_key6 )
	    {
		/* Mutex the RSB if multithreaded list */
	        if ( llb->llb_status & LLB_MULTITHREAD )
		{
		    if ( MutexStatus = LK_mutex(SEM_EXCL, &rsb->rsb_mutex) )
		        return(MutexStatus);
		    /* Recheck everything after waiting for mutex */
		    if ( lkb->lkb_type != LKB_TYPE || 
			 lkb->lkb_lkbq.lkbq_llb != llb_offset ||
			 rsb->rsb_id != lockid->lk_common ||
			 rsb->rsb_name.lk_type != lock_key->lk_type ||
			 rsb->rsb_name.lk_key1 != lock_key->lk_key1 ||
			 rsb->rsb_name.lk_key2 != lock_key->lk_key2 ||
			 rsb->rsb_name.lk_key3 != lock_key->lk_key3 ||
			 rsb->rsb_name.lk_key4 != lock_key->lk_key4 ||
			 rsb->rsb_name.lk_key5 != lock_key->lk_key5 ||
			 rsb->rsb_name.lk_key6 != lock_key->lk_key6 )
		    {
			(VOID)LK_unmutex(&rsb->rsb_mutex);
			lkb = (LKB*)NULL;
		    }
		}
	    }
	    else
	        lkb = (LKB*)NULL;

	    /*
	    ** Skip this shortcut if changing physical attribute of lock.
	    **
	    ** s103715 note:
	    **
	    ** Only immediately grant the lock if NOT a distributed
	    ** lock (clustered, not local), or mode held precludes
	    ** an updater on any other node.
	    */
	    if ( lkb && 
		 lkb->lkb_grant_mode == lock_mode &&
	         lkb->lkb_state == LKB_GRANTED && 
		 ((flags & LK_PHYSICAL && lkb->lkb_attribute & LKB_PHYSICAL) ||
		 ((flags & LK_PHYSICAL) == 0 && (lkb->lkb_attribute & LKB_PHYSICAL) == 0))
		 && (lkb->lkb_grant_mode > LK_IS ||
		     !(lkb->lkb_attribute & LKB_DISTRIBUTED_LOCK)) )
	    {
		if (lkb->lkb_attribute & LKB_PHYSICAL)
		{
		    /* Note: this count is bumped only for implicit physical conversions */
		    lkb->lkb_count++;
		}

		status = LK_GRANTED;

		if (value)
		{
		    value->lk_high = rsb->rsb_value[0];
		    value->lk_low  = rsb->rsb_value[1];
		    value->lk_mode = lock_mode;
		    if (rsb->rsb_invalid)
			status = LK_VAL_NOTVALID;
		}

		if ( llb->llb_status & LLB_MULTITHREAD )
		    (VOID)LK_unmutex(&rsb->rsb_mutex);

		return(status);
	    }
	}
	else
	    lkb = (LKB*)NULL;
	/* Continue on with lkb or NULL */
    }

    if (lkd->lkd_lock_stall != 0 &&
	LGK_my_pid == lkd->lkd_csp_id &&
	(flags & LK_PHYSICAL) == 0)
    {
	/*
	** Logical locks on master-csp lock lists can be automatically
	** granted during lock stalls. This avoids all sorts of gross code
	** in the buffer manager to avoid making logical page lock requests
	** during CSP node failure undo and redo recovery.
	*/
	if ( lkb && llb->llb_status & LLB_MULTITHREAD )
	    (VOID)LK_unmutex(&rsb->rsb_mutex);

	return (LK_GRANTED);
    }

    /*
    ** All parameters have been checked; now either convert an existing lock
    ** or create a new lock.
    */


    /*
    ** Mutex the LLB unless propagating, in which case the mutex is already held
    **
    ** However, do not mutex MULTITHREADed LLBs
    */
    if ((flags & LK_PROPAGATE) == 0 && !(llb->llb_status & LLB_MULTITHREAD) &&
        (MutexStatus = LK_mutex(SEM_EXCL, &llb->llb_mutex)))
    {
	return(MutexStatus);
    }

    if ( llb->llb_status & LLB_PARENT_SHARED )
    {
	/*
	** Then this list was converted to a SHARED multithreaded list while
	** we waited for the mutex.
	**
	** Unmutex the handle, switch llb to the SHARED list, but
	** don't mutex it, yet.
	*/
	proc_llb = llb;
	llb = (LLB*)LGK_PTR_FROM_OFFSET(llb->llb_shared_llb);
	(VOID)LK_unmutex(&proc_llb->llb_mutex);
    }

    /* proc_llb is handle, llb is list holding locks */

    /*
    ** We retain the mutex on proc_llb to prevent it from being
    ** converted to a SHARED multithreaded list while we go about
    ** our business.
    */

    llb_offset = LGK_OFFSET_FROM_PTR(llb);

    /* Set lock requestor in completion for all */
    CSget_cpid(&completion->cpid);

    /* Increase the lock request stamp. */
    completion->request_id = ++llb->llb_stamp;

    /*  Process explicit convert requests. */

    if (flags & LK_CONVERT)
    {
	lkd->lkd_tstat[0].convert++;
	lkd->lkd_tstat[rsb->rsb_name.lk_type].convert++;

	/* If LKB is already waiting on another request, reject this one */
	if ( lkb->lkb_attribute & (LKB_ON_LLB_WAIT | LKB_ENQWAIT) )
	{
	    char	keydispbuf[128];

	    TRdisplay("%@ %d LK_do_convert: LKB already waiting/enq mode %w flags %x\n"
		      "\tLLB %x status %v\n"
		      "\tLKB %x state %w attr %v mode (%w,%w) dlm %x\n"
		      "\tCX owner %x,%x LKB owner %x,%x request %x,%x\n"
		      "\t%s\n",
		      __LINE__,
		      LOCK_MODE, lock_mode,
		      flags,
		      *(u_i4*)&llb->llb_id,
		      LLB_STATUS, llb->llb_status,
		      lkb->lkb_id,
		      LKB_STATE, lkb->lkb_state,
		      LKB_ATTRIBUTE, lkb->lkb_attribute,
		      LOCK_MODE, lkb->lkb_grant_mode,
		      LOCK_MODE, lkb->lkb_request_mode,
#if defined(conf_CLUSTER_BUILD)
		      lkb->lkb_cx_req.cx_lock_id,
		      lkb->lkb_cx_req.cx_owner.pid,
		      lkb->lkb_cx_req.cx_owner.sid,
#else
		      0,
		      0,
		      0,
#endif
		      lkb->lkb_cpid.pid,
		      lkb->lkb_cpid.sid,
		      completion->cpid.pid,
		      completion->cpid.sid,
		      LKkey_to_string(&rsb->rsb_name, keydispbuf));

	    uleFormat(NULL, E_CL1041_LK_REQUEST_BAD_PARAM, (CL_ERR_DESC *)NULL, ULE_LOG,
			NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 3,
			0, lkb->lkb_id, 0, lkb->lkb_type, 
			0, lkb->lkb_attribute);

	    if ( llb->llb_status & LLB_MULTITHREAD )
		(VOID)LK_unmutex(&rsb->rsb_mutex);
	    else
		LK_unmutex(&llb->llb_mutex);
	    return (LK_BADPARAM);
	}

	/* Update LKB thread "owner" -before- maybe stalling */
	lkb->lkb_cpid = completion->cpid;

	if ((lkd->lkd_lock_stall == LK_STALL_ALL_BUT_CSP) &&
	    (LGK_my_pid != lkd->lkd_csp_id) &&
	    (lock_mode > lkb->lkb_grant_mode) &&
	    !(flags & LK_NOSTALL) )
	{
#ifdef LK_TRACE
	    if (LK_rqst_trace())
	    {
		TRdisplay("%@ LK_request_lock Convert; stalled!\n");
		TRflush();
	    }
#endif
	    if ( llb->llb_status & LLB_MULTITHREAD )
		(VOID)LK_unmutex(&rsb->rsb_mutex);
	    else
		(VOID)LK_unmutex(&llb->llb_mutex);
	    lk_stall_request(lkb, __LINE__);
	    if ( llb->llb_status & LLB_MULTITHREAD )
		MutexStatus = LK_mutex(SEM_EXCL, &rsb->rsb_mutex);
	    else
		MutexStatus = LK_mutex(SEM_EXCL, &llb->llb_mutex);
	    if ( MutexStatus )
		return(MutexStatus);
	}

	/* Do explicit conversion of physical lock */
	status = LK_do_convert(proc_llb, lkb, 
			       flags,
			       lock_mode,
			       value, completion,
			       callback, callback_arg, sys_err);
	/* Returns without rsb_mutex */

#ifdef LK_TRACE
	if (LK_rqst_trace())
	{
	TRdisplay("%@ LK_request_lock Convert; LK_do_convert status = %d\n",
	    status);
	TRflush();
	}
#endif

	if ( status == OK && llb->llb_status & LLB_NOINTERRUPT )
	{
	    /* Notify of non-interruptable wait */
	    status = LK_NOINTRWAIT;
	}

	if ( !(llb->llb_status & LLB_MULTITHREAD) )
	    (VOID) LK_unmutex(&llb->llb_mutex);
	return(status);
    }


    /*
    ** At this point we have eliminated an explicit convert request.
    **
    ** If "lkb" is not NULL, then this in an implicit conversion
    ** discovered from "lockid" and its RSB is mutexed.
    **
    ** If "lkb" is NULL, then check for implicit conversion by 
    ** "lock_key".
    */

    if ( !lkb )
    {
	/* 
	**  Compute the hash bucket values for the resource name, and for the
	**  lockid by lock list.
	*/
	rsb_hash_value = (lock_key->lk_type + 
	    lock_key->lk_key1 + lock_key->lk_key2 + 
	    lock_key->lk_key3 + lock_key->lk_key4 +
	    lock_key->lk_key5 + lock_key->lk_key6);

	rsh_table = (RSH *)LGK_PTR_FROM_OFFSET(lkd->lkd_rsh_table);
	rsb_hash_bucket = (RSH *)&rsh_table
	    [(unsigned)rsb_hash_value % lkd->lkd_rsh_size];
	lkh_table = (LKH *)LGK_PTR_FROM_OFFSET(lkd->lkd_lkh_table);
	lkb_hash_bucket = (LKH *)&lkh_table
	    [((unsigned)rsb_hash_value + (unsigned)llb_offset) %
					lkd->lkd_lkh_size];

	/*
	** If lock exists on this lock list, then this is an implicit conversion
	**
	** If propagating a lock to a lock list, the lock will
	** be new to the list.
	*/
	if ( !(flags & LK_PROPAGATE) && 
	     (llb->llb_status & LLB_MULTITHREAD || llb->llb_lkb_count) )
	{
	    /*
	    ** If MULTITHREAD, we need -both- the llb and lkh mutexes, 
	    ** in that order.
	    */
	    if ( llb->llb_status & LLB_MULTITHREAD &&
		 (MutexStatus = LK_mutex(SEM_EXCL, &llb->llb_mutex)) )
	    {
		return(MutexStatus);
	    }
	    /*
	    **  Search the LKB hash queue to see if the current
	    **  list already has a lock on the resource.
	    **  Do it with the lkh_mutex as many threads/LLBs may be
	    **  manipulating the same hash list.
	    **
	    **  Multhreaded list mutex concerns:
	    **
	    **	For the release of a single lock, LKrelease will
	    ** 	  o hold the rsb, want the lkh
	    **    o hold the rsb, want the llb
	    **
	    **  For the release of a series of locks on the lock list
	    **  LKrelease will
	    **    o hold the llb, want the rsb
	    **    o hold the llb + rsb, want the lkh
	    **
	    **  so we must be vigilant in here that we don't take
	    **  and hold these mutexes in an order that might
	    **  deadlock with LKrelease.
	    */

	    lkbq = (LKBQ *)lkb_hash_bucket;
	    lkh_offset = LGK_OFFSET_FROM_PTR(lkb_hash_bucket);
	    
	    if (MutexStatus = LK_mutex(SEM_EXCL, &lkb_hash_bucket->lkh_mutex))
	    {
	        if ( llb->llb_status & LLB_MULTITHREAD )
		    (VOID)LK_unmutex(&llb->llb_mutex);
		return(MutexStatus);
	    }

	    /* We now hold llb_mutex, lkh_mutex */
	    
	    hash_chain_length = 0;
	    lkbq_offset = lkbq->lkbq_next;

	    while (lkbq_offset)
	    {
		lkbq = (LKBQ *)LGK_PTR_FROM_OFFSET(lkbq_offset);
		lkbq_offset = lkbq->lkbq_next;

		hash_chain_length++;

		/* Skip those not belonging to this LLB */
		if (lkbq->lkbq_llb == llb_offset)
		{
		    rsb = (RSB *)LGK_PTR_FROM_OFFSET(lkbq->lkbq_rsb);

		    if (rsb->rsb_name.lk_type == lock_key->lk_type &&
			rsb->rsb_name.lk_key1 == lock_key->lk_key1 &&
			rsb->rsb_name.lk_key2 == lock_key->lk_key2 &&
			rsb->rsb_name.lk_key3 == lock_key->lk_key3 &&
			rsb->rsb_name.lk_key4 == lock_key->lk_key4 &&
			rsb->rsb_name.lk_key5 == lock_key->lk_key5 &&
			rsb->rsb_name.lk_key6 == lock_key->lk_key6)
		    {
			/*
			** If multithreaded list, release the hash and llb,
			** mutex the RSB and recheck it.
			*/
			if ( llb->llb_status & LLB_MULTITHREAD )
			{
			    (VOID)LK_unmutex(&lkb_hash_bucket->lkh_mutex);
			    (VOID)LK_unmutex(&llb->llb_mutex);
			    /* We now hold no mutexes */

			    if ( MutexStatus = LK_mutex(SEM_EXCL, &rsb->rsb_mutex) )
				return(MutexStatus);
			    if (rsb->rsb_name.lk_type == lock_key->lk_type &&
				rsb->rsb_name.lk_key1 == lock_key->lk_key1 &&
				rsb->rsb_name.lk_key2 == lock_key->lk_key2 &&
				rsb->rsb_name.lk_key3 == lock_key->lk_key3 &&
				rsb->rsb_name.lk_key4 == lock_key->lk_key4 &&
				rsb->rsb_name.lk_key5 == lock_key->lk_key5 &&
				rsb->rsb_name.lk_key6 == lock_key->lk_key6 &&
				lkbq->lkbq_rsb == LGK_OFFSET_FROM_PTR(rsb) &&
				lkbq->lkbq_llb == llb_offset)
			    {
				lkb = (LKB *)((char *)lkbq - CL_OFFSETOF(LKB, lkb_lkbq));
				/* We now hold only the rsb mutex, not the llb or lkb */
				break;
			    }
			    /*
			    ** RSB changed while we waited for its mutex,
			    ** remutex the llb, hash and start over.
			    */
			    (VOID)LK_unmutex(&rsb->rsb_mutex);
			    if ( MutexStatus = LK_mutex(SEM_EXCL, &llb->llb_mutex) )
				return(MutexStatus);
			    if ( MutexStatus = LK_mutex(SEM_EXCL, &lkb_hash_bucket->lkh_mutex) )
			    {
				(VOID)LK_unmutex(&llb->llb_mutex);
				return(MutexStatus);
			    }

			    lkbq = (LKBQ *)lkb_hash_bucket;
			    lkbq_offset = lkbq->lkbq_next;
			    hash_chain_length = 0;
			}
			else
			{
			    lkb = (LKB *)((char *)lkbq - CL_OFFSETOF(LKB, lkb_lkbq));
			    (VOID)LK_unmutex(&lkb_hash_bucket->lkh_mutex);
			    break;
			}
		    }
		}
	    }

	    /* 
	    ** If lkb was not found and it's a MULTITHREADed list,
	    ** then retain the lkh_mutex to prevent a concurrent 
	    ** thread sharing this LLB from adding the same lock 
	    ** to the hash.
	    */
	    if ( !lkb && !(llb->llb_status & LLB_MULTITHREAD) )
		(VOID)LK_unmutex(&lkb_hash_bucket->lkh_mutex);

	    if (hash_chain_length > lkd->lkd_stat.max_lock_chain_len)
		lkd->lkd_stat.max_lock_chain_len = hash_chain_length;
	}

    }

    if (lkb)
    {
	/* If multithreaded, we hold the lkb's RSB mutex */

	physical_to_logical = FALSE;
        intend_to_covering = FALSE;
	    
	/*	LKB was found.  This is an implicit conversion. */

	lkd->lkd_tstat[0].request_convert++;
	lkd->lkd_tstat[rsb->rsb_name.lk_type].request_convert++;

	/* If LKB is already waiting on another request, reject this one */
	if ( lkb->lkb_attribute & (LKB_ON_LLB_WAIT | LKB_ENQWAIT) )
	{
	    char	keydispbuf[128];

	    TRdisplay("%@ %d LK_do_convert: LKB already waiting/enq mode %w flags %x\n"
		      "\tLLB %x status %v\n"
		      "\tLKB %x state %w attr %v mode (%w,%w) dlm %x\n"
		      "\tCX owner %x,%x LKB owner %x,%x request %x,%x\n"
		      "\t%s\n",
		      __LINE__,
		      LOCK_MODE, lock_mode,
		      flags,
		      *(u_i4*)&llb->llb_id,
		      LLB_STATUS, llb->llb_status,
		      lkb->lkb_id,
		      LKB_STATE, lkb->lkb_state,
		      LKB_ATTRIBUTE, lkb->lkb_attribute,
		      LOCK_MODE, lkb->lkb_grant_mode,
		      LOCK_MODE, lkb->lkb_request_mode,
#if defined(conf_CLUSTER_BUILD)
		      lkb->lkb_cx_req.cx_lock_id,
		      lkb->lkb_cx_req.cx_owner.pid,
		      lkb->lkb_cx_req.cx_owner.sid,
#else
		      0,
		      0,
		      0,
#endif
		      lkb->lkb_cpid.pid,
		      lkb->lkb_cpid.sid,
		      completion->cpid.pid,
		      completion->cpid.sid,
		      LKkey_to_string(&rsb->rsb_name, keydispbuf));

	    uleFormat(NULL, E_CL1041_LK_REQUEST_BAD_PARAM, (CL_ERR_DESC *)NULL, ULE_LOG,
			NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 3,
			0, lkb->lkb_id, 0, lkb->lkb_type, 
			0, lkb->lkb_attribute);

	    if ( llb->llb_status & LLB_MULTITHREAD )
		LK_unmutex(&rsb->rsb_mutex);
	    else
		LK_unmutex(&llb->llb_mutex);
	    return (LK_BADPARAM);
	}

	/*	Update LKB type flags. */

        if ((flags & LK_PHYSICAL) == 0)
        {
            if (lkb->lkb_attribute & LKB_PHYSICAL)
            {
                physical_to_logical = TRUE;
                lkb->lkb_count = 0;
                lkb->lkb_attribute &= ~LKB_PHYSICAL;
            }
        }
        else if (lkb->lkb_attribute & LKB_PHYSICAL)
        {
	    /* Count another physical lock on this resource */
	    lkb->lkb_count++;
        }
        else
        {
            /*
            ** The "fall-through" is the case if the resource is locked
            ** by a logical lock and we are requesting a physical lock
            ** on it. In this case a LOGICAL lock must be granted. 
            */
        }

	/*
	**	Return lockid if requested. Note that we do this before we call
	**	convert. The lockid information will not change during the
	**	convert call (the lkb and rsb are known at this point). However,
	**	in a cluster system the convert call may awaken the lock
	**	requestor and so he may awaken from his CSsuspend call before
	**	we return from the convert() call. The lock requestor will
	**	re-acquire the mutex before completing his lock request, but
	**	still it seems preferable to set the returned lockid here,
	**	rather than after the convert call, so that all the output
	**	information is filled in before we awaken the requestor.
	*/

	if ( lockid )
	{
	    lockid->lk_unique = lkb->lkb_id;
	    lockid->lk_common = rsb->rsb_id;
	}

#ifdef CSP_DEBUG_LEVEL2
	if (csp_debug)
	TRdisplay("%@ LK: Lockid set to %x.%x by implicit convert\n",
		    lkb->lkb_id, rsb->rsb_id);
#endif

	/* Set LKB thread "owner" -before- maybe stalling */
	lkb->lkb_cpid = completion->cpid;

	if ((lkd->lkd_lock_stall == LK_STALL_ALL_BUT_CSP) &&
	    (LGK_my_pid != lkd->lkd_csp_id) &&
	    !(flags & LK_NOSTALL) )
	{
	    if ( llb->llb_status & LLB_MULTITHREAD )
		(VOID)LK_unmutex(&rsb->rsb_mutex);
	    else
		(VOID)LK_unmutex(&llb->llb_mutex);
	    lk_stall_request(lkb, __LINE__);
	    if ( llb->llb_status & LLB_MULTITHREAD )
		MutexStatus = LK_mutex(SEM_EXCL, &rsb->rsb_mutex);
	    else
	        MutexStatus = LK_mutex(SEM_EXCL, &llb->llb_mutex);
	    if ( MutexStatus )
		return(MutexStatus);
	}

	if (physical_to_logical &&
	    (llb->llb_llkb_count >= llb->llb_max_lkb) &&
	    ((llb->llb_status & (LLB_MASTER | LLB_RECOVER)) == 0) && 
	    ((flags & LK_IGNORE_LOCK_LIMIT) == 0))
	{
    	    if ((flags & LK_QUIET_TOO_MANY) == 0)
	    {
	        uleFormat(NULL, E_DMA00D_TOO_MANY_LOG_LOCKS, (CL_ERR_DESC *)NULL, 
			   ULE_LOG, NULL, (char *)NULL, 0L, (i4 *)NULL, 
			   &err_code, 4, 0, llb->llb_status, 0, flags,
			   0, llb->llb_llkb_count, 0, llb->llb_max_lkb);
	    }
	    if ( llb->llb_status & LLB_MULTITHREAD )
		LK_unmutex(&rsb->rsb_mutex);
	    else
		(VOID) LK_unmutex(&llb->llb_mutex);
	    return (LK_NOLOCKS);
	}

        if (lock_key->lk_type == LK_PAGE &&
            lkb->lkb_grant_mode < LK_S && lock_mode >= LK_S)
        {
            intend_to_covering = TRUE;
        }

	/*
	**	Convert the lock to a new mode based on current mode and
	**	requested mode.
	**
	**	Returns without the rsb_mutex.
	*/

	status = LK_do_convert(proc_llb, lkb, flags,
			       LK_convert_mode[lkb->lkb_grant_mode][lock_mode],
			       value, completion, 
			       callback, callback_arg, sys_err);

#ifdef LK_TRACE
	if (LK_rqst_trace())
	{
	    TRdisplay(
	"%@ LK_request_lock Implicit Convert; LK_do_convert status = %d\n",
		status);
	    TRflush();
	}
#endif
	/* Process outcome of implicit conversion: */
	if ( status == OK || status == LK_GRANTED )
	{
	    if ( physical_to_logical )
	    {
		/* Count another logical lock */
		if (++llb->llb_llkb_count > lkd->lkd_stat.max_lkb_per_txn)
		    lkd->lkd_stat.max_lkb_per_txn = llb->llb_llkb_count;

		if ( status == LK_GRANTED && flags & LK_STATUS )
		    status = LK_LOG_LOCK;
	    }

	    if ( status == LK_OK && llb->llb_status & LLB_NOINTERRUPT )
	        status = LK_NOINTRWAIT;
	    else if ( status == LK_GRANTED && intend_to_covering && flags & LK_STATUS )
	        status = LK_COV_LOCK;
	}

	if ( !(llb->llb_status & LLB_MULTITHREAD) )
	    (VOID) LK_unmutex(&llb->llb_mutex);

	return (status);
    }

    /*  Matching LKB was not found. */

    /*
    ** If multithreaded list, the llb and lkh mutexes are still held,
    ** keeping others at bay while we resolve the RSB issue.
    ** Unwind them in the opposite order, lkh, then llb.
    **
    ** If not multithreaded, we hold only the llb_mutex.
    */


    /*
    ** Before allocating a new RSB and/or LKB, make sure that
    ** we don't blow the logical lock limit for this lock list.
    */
    if (llb->llb_llkb_count >= llb->llb_max_lkb &&
	(llb->llb_status & (LLB_MASTER | LLB_RECOVER)) == 0 && 
	(flags & (LK_PHYSICAL | LK_IGNORE_LOCK_LIMIT | LK_PROPAGATE)) == 0)
    {
	if ((flags & LK_QUIET_TOO_MANY) == 0)
	{
	    uleFormat(NULL, E_DMA00D_TOO_MANY_LOG_LOCKS, (CL_ERR_DESC *)NULL, 
			ULE_LOG, NULL, (char *)NULL, 0L, (i4 *)NULL, 
			&err_code, 4, 0, llb->llb_status, 0, flags,
			0, llb->llb_llkb_count, 0, llb->llb_max_lkb);
	}
	if ( llb->llb_status & LLB_MULTITHREAD )
	    (VOID)LK_unmutex(&lkb_hash_bucket->lkh_mutex);
	(VOID) LK_unmutex(&llb->llb_mutex);
	return (LK_NOLOCKS);
    }

    /*
    **	Search the RSB hash bucket for a matching key.
    **  rsb_hash_bucket has already been computed 
    */

    /*
    ** Tell all that we're allocating new lock and/or resource.
    ** This keeps our LLB stash of RSBs and LKBs from being
    ** scavenged by others.
    */
    llb->llb_status |= LLB_ALLOC;

    do
    {
	/*
	** Lock hash chain exclusively. If it's determined that a
	** matching resource does not exist, we don't want to leave
	** a window open whereby a second thread discovers the same
	** condition and simultaneously tries to create a duplicate RSB.
	*/
	if (MutexStatus = LK_mutex(SEM_EXCL, &rsb_hash_bucket->rsh_mutex))
	{
	    if ( llb->llb_status & LLB_MULTITHREAD )
		(VOID)LK_unmutex(&lkb_hash_bucket->lkh_mutex);
	    llb->llb_status &= ~LLB_ALLOC;
	    (VOID)LK_unmutex(&llb->llb_mutex);
	    return (MutexStatus);
	}

	hash_chain_length = 0;
	FreeRsbIfFail = FALSE;
	matching_rsb = FALSE;

	for (rsb = (RSB *) rsb_hash_bucket;
	     rsb->rsb_q_next;
	    )
	{
	    rsb = (RSB *)LGK_PTR_FROM_OFFSET(rsb->rsb_q_next);
	    if (rsb_hash_bucket != (RSH *)LGK_PTR_FROM_OFFSET(rsb->rsb_rsh_offset))
	    {
		TRdisplay("%@ Panic: LK_request_lock #1 - Bad RSB %d, LLB %d\n",
		    rsb->rsb_id, llb->llb_id.id_id);
	    }
	    hash_chain_length++;

	    if (rsb->rsb_name.lk_key4 == lock_key->lk_key4 &&
		rsb->rsb_name.lk_key3 == lock_key->lk_key3 &&
		rsb->rsb_name.lk_key2 == lock_key->lk_key2 &&
		rsb->rsb_name.lk_key1 == lock_key->lk_key1 &&
		rsb->rsb_name.lk_key5 == lock_key->lk_key5 &&
		rsb->rsb_name.lk_key6 == lock_key->lk_key6 &&
		rsb->rsb_name.lk_type == lock_key->lk_type)
	    {
		/*
		** Unlock the hash chain, lock the RSB, recheck.
		*/
		(VOID)LK_unmutex(&rsb_hash_bucket->rsh_mutex);

		/*
		** If multithread, unwind the lkh and llb
		** mutexes before taking the rsb mutex.
		*/
		if ( llb->llb_status & LLB_MULTITHREAD )
		{
		    (VOID)LK_unmutex(&lkb_hash_bucket->lkh_mutex);
		    llb->llb_status &= ~LLB_ALLOC;
		    (VOID)LK_unmutex(&llb->llb_mutex);
		}
		if (MutexStatus = LK_mutex(SEM_EXCL, &rsb->rsb_mutex))
		{
		    if ( !(llb->llb_status & LLB_MULTITHREAD) )
		    {
			llb->llb_status &= ~LLB_ALLOC;
			(VOID)LK_unmutex(&llb->llb_mutex);
		    }
		    return (MutexStatus);
		}
		/*
		** Check again after mutex wait
		*/
		if (rsb->rsb_type == RSB_TYPE &&
		    rsb->rsb_name.lk_key4 == lock_key->lk_key4 &&
		    rsb->rsb_name.lk_key3 == lock_key->lk_key3 &&
		    rsb->rsb_name.lk_key2 == lock_key->lk_key2 &&
		    rsb->rsb_name.lk_key1 == lock_key->lk_key1 &&
		    rsb->rsb_name.lk_key5 == lock_key->lk_key5 &&
		    rsb->rsb_name.lk_key6 == lock_key->lk_key6 &&
		    rsb->rsb_name.lk_type == lock_key->lk_type)
		{
#ifdef LK_TRACE
		    if (LK_rqst_trace())
		    {
			TRdisplay(
	"%@ LK_request_lock; matching RSB found, id: %x, value: %x %x\n",
			    rsb->rsb_id, rsb->rsb_value[0], rsb->rsb_value[1]);
			TRflush();
		    }
#endif
		    matching_rsb = TRUE;
		    break;
		}
		/*
		** Hmm. RSB matched, but we waited for its mutex and 
		** now it's changed. Unlock the RSB, relock the RSH,
		** and restart the search.
		*/
		(VOID)LK_unmutex(&rsb->rsb_mutex);

		/* If multithread, first retake llb, hash mutexes */
		if ( llb->llb_status & LLB_MULTITHREAD )
		{
		    if ( MutexStatus = LK_mutex(SEM_EXCL, &llb->llb_mutex) )
			return(MutexStatus);
		    llb->llb_status |= LLB_ALLOC;
		    if ( MutexStatus = LK_mutex(SEM_EXCL, &lkb_hash_bucket->lkh_mutex) )
		    {
			llb->llb_status &= ~LLB_ALLOC;
			(VOID)LK_unmutex(&llb->llb_mutex);
			return(MutexStatus);
		    }
		}
		if ( MutexStatus = LK_mutex(SEM_EXCL, &rsb_hash_bucket->rsh_mutex) )
		{
		    if ( llb->llb_status & LLB_MULTITHREAD )
			(VOID)LK_unmutex(&lkb_hash_bucket->lkh_mutex);
		    llb->llb_status &= ~LLB_ALLOC;
		    (VOID)LK_unmutex(&llb->llb_mutex);
		    return (MutexStatus);
		}
		rsb = (RSB *) rsb_hash_bucket;
		hash_chain_length = 0;
	    }
	}

	/*
	** If a matching RSB was found, the RSH is unlocked and
	** the RSB is locked. If MULTITHREADed, the llb and lkh 
	** mutexes are also held.
	**
	** If a matching RSB was NOT found, the RSH remains locked.
	*/

	if (hash_chain_length > lkd->lkd_stat.max_rsrc_chain_len)
	    lkd->lkd_stat.max_rsrc_chain_len = hash_chain_length;

        if (matching_rsb)
        {
	    /*
	    ** If multithreaded list, research the hash queue
	    ** to see if this lock list now has a lock on the
	    ** resource.
	    **
	    ** If it does, then recursively call LK_request
	    ** to handle the implicit conversion.
	    **
	    ** If is does not, then fall through to allocate a 
	    ** new LKB.
	    */
	    if ( llb->llb_status & LLB_MULTITHREAD )
	    {
		SIZE_TYPE	rsb_offset = LGK_OFFSET_FROM_PTR(rsb);

		/* We hold rsb_mutex, retake llb, hash mutexes */
		if ( MutexStatus = LK_mutex(SEM_EXCL, &llb->llb_mutex) )
		{
		    (VOID)LK_unmutex(&rsb->rsb_mutex);
		    return(MutexStatus);
		}
		llb->llb_status |= LLB_ALLOC;
		if ( MutexStatus = LK_mutex(SEM_EXCL, &lkb_hash_bucket->lkh_mutex) )
		{
		    llb->llb_status &= ~LLB_ALLOC;
		    (VOID)LK_unmutex(&llb->llb_mutex);
		    (VOID)LK_unmutex(&rsb->rsb_mutex);
		    return(MutexStatus);
		}

	        lkbq = (LKBQ*)lkb_hash_bucket;
		lkbq_offset = lkbq->lkbq_next;

		while ( lkbq_offset )
		{
		    lkbq = (LKBQ*)LGK_PTR_FROM_OFFSET(lkbq_offset);
		    lkbq_offset = lkbq->lkbq_next;

		    if ( lkbq->lkbq_llb == llb_offset &&
		         lkbq->lkbq_rsb == rsb_offset )
		    {
		        (VOID)LK_unmutex(&rsb->rsb_mutex);
		        (VOID)LK_unmutex(&lkb_hash_bucket->lkh_mutex);
			llb->llb_status &= ~LLB_ALLOC;
		        (VOID)LK_unmutex(&llb->llb_mutex);

			return(LK_request(flags,
					  lock_list_id,
					  lock_key,
					  lock_mode,
					  value,
					  lockid,
					  callback,
					  callback_arg,
					  completion,
					  sys_err));
		    }
		}
	    }
        }
        else 
	{
	    /*
	    ** No matching RSB was found.
	    ** This must be a new lock request and will require a new RSB
	    ** as well as a new LKB to be allocated.
	    */
#ifdef LK_TRACE
	    if (LK_rqst_trace())
	    {
		TRdisplay("%@ LK_request_lock; matching RSB NOT found\n");
		TRflush();
	    }
#endif

	    /*
            ** Let LK_allocate_cb() know which mutex is held.
            ** To avoid mutex deadlocks, LK_allocate_cb() (scavenge())
            ** may release the mutex; if it does, it'll
            ** notify us by setting llb_tick to zero, and we'll have
            ** to recheck the RSB, below, to make sure it hasn't been
            ** deallocated or reused for another resource. If that's the
            ** case, we loop back to start the whole process over.
            */
            llb->llb_tick = LGK_OFFSET_FROM_PTR(&rsb_hash_bucket->rsh_mutex);

            /*
	    ** A successful allocate returns with the RSB's mutex locked.
	    */
	    if ((rsb = (RSB *)LK_allocate_cb(RSB_TYPE, llb)) == (RSB *)NULL)
	    {
		(VOID) LK_unmutex(&rsb_hash_bucket->rsh_mutex);

		uleFormat(NULL, E_DMA00E_LK_NO_RSBS, (CL_ERR_DESC *)NULL, ULE_LOG,
				NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 0);

		if ( llb->llb_status & LLB_MULTITHREAD )
		    (VOID)LK_unmutex(&lkb_hash_bucket->lkh_mutex);
		llb->llb_status &= ~(LLB_ALLOC);
		(VOID) LK_unmutex(&llb->llb_mutex);
		return (LK_NOLOCKS);
	    }

	    /*
	    ** If the RSH was released in order to allocate the
	    ** RSB, toss the RSB and research the hash chain.
	    */
	    if ( llb->llb_tick == 0 )
	    {
	      /* Toss the RSB, start over */
	      (VOID)LK_unmutex(&rsb_hash_bucket->rsh_mutex);
	      LK_deallocate_cb(RSB_TYPE, (PTR)rsb, llb);
	      rsb = (RSB*)NULL;
	      continue;
	    }

	    /*  Initialize the new RSB. */

	    /* If the request fails for any reason after this, toss the RSB */

	    FreeRsbIfFail = TRUE;
	    rsb->rsb_wait_q_next =
		rsb->rsb_wait_q_prev = LGK_OFFSET_FROM_PTR(&rsb->rsb_wait_q_next);
	    rsb->rsb_grant_q_next =
		rsb->rsb_grant_q_prev = LGK_OFFSET_FROM_PTR(&rsb->rsb_grant_q_next);
	    rsb->rsb_cback_count = 0;
	    rsb->rsb_type = RSB_TYPE;
	    rsb->rsb_grant_mode = LK_N;
	    rsb->rsb_convert_mode = LK_N;
	    rsb->rsb_name.lk_type = lock_key->lk_type;
	    rsb->rsb_name.lk_key1 = lock_key->lk_key1;
	    rsb->rsb_name.lk_key2 = lock_key->lk_key2;
	    rsb->rsb_name.lk_key3 = lock_key->lk_key3;
	    rsb->rsb_name.lk_key4 = lock_key->lk_key4;
	    rsb->rsb_name.lk_key5 = lock_key->lk_key5;
	    rsb->rsb_name.lk_key6 = lock_key->lk_key6;
	    rsb->rsb_value[0] = 0;
	    rsb->rsb_value[1] = 0;
	    rsb->rsb_invalid = 0;
	    rsb->rsb_rsh_offset = LGK_OFFSET_FROM_PTR(rsb_hash_bucket);
		
	    /*
	    **  Insert the new RSB into the RSB hash queue
	    **  now to avoid holding the hash mutex "too long".
	    **
	    **  We still hold EXCL lock on hash mutex.
	    **
	    **  Note that if something fails after this, we must
	    **  remove the RSB from the hash queue.
	    */
	    rsb->rsb_q_next = rsb_hash_bucket->rsh_rsb_next;
	    rsb->rsb_q_prev = LGK_OFFSET_FROM_PTR(rsb_hash_bucket);
	    if (rsb_hash_bucket->rsh_rsb_next)
	    {
		next_rsb = (RSB *)LGK_PTR_FROM_OFFSET(rsb_hash_bucket->rsh_rsb_next);
		next_rsb->rsb_q_prev = LGK_OFFSET_FROM_PTR(rsb);
	    }
	    rsb_hash_bucket->rsh_rsb_next = LGK_OFFSET_FROM_PTR(rsb);

	    /* Release hash mutex */
	    (VOID) LK_unmutex(&rsb_hash_bucket->rsh_mutex);
	}

	/* Utilize the LKB embedded in the RSB, if unused */
	if ( rsb->rsb_lkb.lkb_type == 0 )
	{
	    lkb = &rsb->rsb_lkb;
	    /* Count as CB "allocation"; we count a dealloc when LKB released */
	    lkd->lkd_stat.allocate_cb++;
	}
	else
	{
	    /*
	    ** Allocate a new LKB.
	    */

	    /*
	    ** Let LK_allocate_cb() know which mutex is involved.
	    ** To avoid mutex deadlocks, LK_allocate_cb() (scavenge())
	    ** may release the mutex; if it does, it'll
	    ** notify us by setting llb_tick to zero, and we'll have
	    ** to recheck the RSB, below, to make sure it hasn't been
	    ** deallocated or reused for another resource. If that's the
	    ** case, we loop back to start the whole process over.
	    */
	    llb->llb_tick = LGK_OFFSET_FROM_PTR(&rsb->rsb_mutex);

	    if ((lkb = (LKB *)LK_allocate_cb(LKB_TYPE, llb)) == (LKB *)NULL)
	    {
		if (!matching_rsb)
		{
		    /*
		    ** Remove RSB from hash queue, free the RSB,
		    ** and unlock rsb_mutex.
		    */
		    dequeue_cbs(llb, &rsb, (LKB **)NULL, FreeRsbIfFail);
		}
		else
		    (VOID) LK_unmutex(&rsb->rsb_mutex);

		uleFormat(NULL, E_DMA010_LK_NO_LKBS, (CL_ERR_DESC *)NULL, ULE_LOG,
				NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 0);

		if ( llb->llb_status & LLB_MULTITHREAD )
		    (VOID)LK_unmutex(&lkb_hash_bucket->lkh_mutex);
		llb->llb_status &= ~(LLB_ALLOC);
		(VOID) LK_unmutex(&llb->llb_mutex);
		return (LK_NOLOCKS);
	    }

            /*
            ** Recheck the RSB if its mutex was released while the
            ** LKB was being allocated.
            */
            if ( llb->llb_tick == 0 &&
                (rsb->rsb_type != RSB_TYPE || MEcmp((PTR)&rsb->rsb_name,
                                                (PTR)lock_key,
                                                sizeof(rsb->rsb_name))) )
            {
                /* Give up on this RSB, toss the LKB, start over */
                (VOID) LK_unmutex(&rsb->rsb_mutex);
                rsb = (RSB*)NULL;
                LK_deallocate_cb(LKB_TYPE, (PTR)lkb, llb);
                lkb = (LKB*)NULL;
            }
	}
    } while ( rsb == (RSB*)NULL );

    /*
    **	At this point we have a new LKB. Initialize some pieces.
    **
    **  We hold the llb and rsb mutexes and, if MULTITHREADed
    **  list, the lkh_mutex.
    */
    lkb->lkb_type = LKB_TYPE;

    lkd->lkd_tstat[0].request_new++;
    lkd->lkd_tstat[rsb->rsb_name.lk_type].request_new++;

    /* We know the lkb/rsb pair now - connect the dots */

    completion->lkb_offset = LGK_OFFSET_FROM_PTR(lkb);
    lkb->lkb_request_id = completion->request_id;
    lkb->lkb_status = OK;

    if ( lockid )
    {
        lockid->lk_unique = lkb->lkb_id;
	lockid->lk_common = rsb->rsb_id;
    }

    /* For LK_resume, in case request must wait */
    if ( value )
        lkb->lkb_attribute |= LKB_VALADDR_PROVIDED;

    lkb->lkb_lkbq.lkbq_rsb = LGK_OFFSET_FROM_PTR(rsb);
    lkb->lkb_lkbq.lkbq_llb = LGK_OFFSET_FROM_PTR(llb);
    lkb->lkb_request_mode = lock_mode;
    lkb->lkb_grant_mode = LK_N;

    /* Copy in Callback Details */
    lkb->lkb_cback.cback_fcn = callback;
    lkb->lkb_cback.cback_arg = callback_arg;

    /* Set LKB thread "owner" */
    lkb->lkb_cpid = completion->cpid;

    /*
    ** Optimistically link the LKB to its hash and LLB queues;
    ** if we later fail, we'll unlink and free the LKB.
    */

    /* Insert the LKB onto the LKB hash queue. */

    /* If multithreaded list, we already hold the lkh_mutex */

    if ( !(llb->llb_status & LLB_MULTITHREAD) &&
         (MutexStatus = LK_mutex(SEM_EXCL, &lkb_hash_bucket->lkh_mutex)) )
    {
	(VOID)LK_unmutex(&rsb->rsb_mutex);
	llb->llb_status &= ~(LLB_ALLOC);
	(VOID)LK_unmutex(&llb->llb_mutex);
	return(MutexStatus);
    }

    lkb->lkb_lkbq.lkbq_lkh  = LGK_OFFSET_FROM_PTR(lkb_hash_bucket);
    lkb->lkb_lkbq.lkbq_next = lkb_hash_bucket->lkh_lkbq_next;
    lkb->lkb_lkbq.lkbq_prev = LGK_OFFSET_FROM_PTR(lkb_hash_bucket);
    if (lkb_hash_bucket->lkh_lkbq_next)
    {
	lkbq = (LKBQ *)LGK_PTR_FROM_OFFSET(lkb_hash_bucket->lkh_lkbq_next);
	lkbq->lkbq_prev = LGK_OFFSET_FROM_PTR(&lkb->lkb_lkbq);
    }
    lkb_hash_bucket->lkh_lkbq_next = LGK_OFFSET_FROM_PTR(&lkb->lkb_lkbq);
    (VOID) LK_unmutex(&lkb_hash_bucket->lkh_mutex);

    /* Insert the LKB onto the tail of the LLB's LKB queue. */

    lkb->lkb_llbq.llbq_next = LGK_OFFSET_FROM_PTR(&llb->llb_llbq.llbq_next);
    lkb->lkb_llbq.llbq_prev = llb->llb_llbq.llbq_prev;
    prev_llbq = (LLBQ *)LGK_PTR_FROM_OFFSET(llb->llb_llbq.llbq_prev);
    prev_llbq->llbq_next =
	llb->llb_llbq.llbq_prev = LGK_OFFSET_FROM_PTR(&lkb->lkb_llbq);

    /* Count another logical lock */
    if ( (flags & LK_PHYSICAL) == 0 )
    {
	if (++llb->llb_llkb_count > lkd->lkd_stat.max_lkb_per_txn)
	    lkd->lkd_stat.max_lkb_per_txn = llb->llb_llkb_count;
    }

    /*	Increment count of locks held by this LLB. */

    llb->llb_lkb_count++;


    /* Update llb_status if we've added an EXCL SV_DATABASE or ROW lock */
    if ( (llb->llb_status & (LLB_XROW | LLB_XSVDB)) == 0 && 
    		lkb->lkb_request_mode == LK_X )
    {
	if ( rsb->rsb_name.lk_type == LK_ROW )
	    llb->llb_status |= LLB_XROW;
	else if ( rsb->rsb_name.lk_type == LK_SV_DATABASE )
	    llb->llb_status |= LLB_XSVDB;
    }

    /*
    ** We're done fiddling with the LLB lock queues and status,
    ** so it's safe to release the llb_mutex.
    **
    ** However, we'll continue to hold the rsb_mutex to protect
    ** the LKB/RSB.
    */
    llb->llb_status &= ~(LLB_ALLOC);
    (VOID)LK_unmutex(&llb->llb_mutex);
    
    /*
    ** If we're currently stalling some or all new lock requests, this lock
    ** request may need to be stalled.
    **
    ** While we are stalled, we must pin the RSB to
    ** prevent another thread from:
    **	o finds our new RSB, locks it
    **  o unlocks it
    **  o now there are no more grants or waiters
    **    so the RSB is deallocated
    ** We get unstalled and our RSB is gone or is something
    ** else altogether.
    */
    if ( lkd->lkd_lock_stall == LK_STALL_ALL_BUT_CSP &&
	 LGK_my_pid != lkd->lkd_csp_id && !(flags & LK_NOSTALL) )
    {
	/* Prevent RSB from being deallocated while we're stalled */
	rsb->rsb_waiters++;
	(VOID) LK_unmutex(&rsb->rsb_mutex);
	lk_stall_request(lkb, __LINE__);
	if ( MutexStatus = LK_mutex(SEM_EXCL, &rsb->rsb_mutex) )
	    return(MutexStatus);
	/* Unpin the RSB, continue with new lock request */
	rsb->rsb_waiters--;
    }

    /* Returns with rsb_mutex, unless rsb was set to  NULL */
    status = LK_do_new_lock_request(proc_llb, lkb, &rsb, flags,
					completion,
				        sys_err);

    /* Deal with the outcome */
    switch ( status )
    {
        case OK:
	    /* Must wait for completion */
	    if ( llb->llb_status & LLB_NOINTERRUPT )
	        status = LK_NOINTRWAIT;
	    if ( flags & LK_STATUS )
	        lkb->lkb_status = LK_NEW_LOCK;
	    /* "value" will be redeemed when lock list is resumed */
	    break;

	case LK_GRANTED:
	    /* New lock has been granted */
	    if ( flags & LK_STATUS )
	        status = LK_NEW_LOCK;
	    if ( value )
	    {
		lkb->lkb_attribute &= ~LKB_VALADDR_PROVIDED;
		value->lk_high = rsb->rsb_value[0];
		value->lk_low = rsb->rsb_value[1];
		value->lk_mode = lkb->lkb_request_mode;
		if (rsb->rsb_invalid)
		    status = LK_VAL_NOTVALID;
	    }
	    break;

	default:
	    /* Unable to grant or wait for lock */

	    /* Delink and free the LKB and possibly the RSB */
	    dequeue_cbs(llb, &rsb, &lkb, FreeRsbIfFail);

	    completion->lkb_offset = 0;

	    if ( lockid )
	    {
		lockid->lk_unique = 0;
		lockid->lk_common = 0;
	    }
	    break;
    }

    /* If we still have an RSB, then release its mutex */
    if ( rsb )
        (VOID)LK_unmutex(&rsb->rsb_mutex);

    return (status);
}

/*
** Name: LK_do_new_lock_request - perform a lock request
**
** History:
**	20-sep-1993 (bryanp) merged 23-mar-1993 (bryanp)
**	    Fix a number of bugs surrounding handling of invalid lock values:
**	    3) When a sys$enq call completes synchronously (SS$_SYNCH is
**		returned), we must check the lock status block for the
**		SS$_VALNOTVALID return code. This means that the lock was
**		synchronously granted/converted but that the value is marked
**		invalid. We must ensure that in this case we mark the
**		rsb_invalid field in the RSB so that LK_VALNOTVALID is returned
**		to the caller.
**	12-oct-1993 (tad)
**	    Bug #56449
**	    Changed %x to %p for pointer values.
**	14-dec-1994 (medji01)
**	    Mutex Granularity Project
**		- Acquire the LKD, RSH, and RSB mutexes when granting a lock
**		- Acquire the RSH and RSB mutexes when denying a lock.
**		- Acquire the LKH mutex when adding an LKB
**		- Acquire the LLB mutex when attaching an LKB if the LLB is
**		  shared.
**	28-mar-1995 (medji01)
**	    64K+ Lock Limit Project
**		- Removed references to id_instance in LK_ID.
**		- Changed error messages to reference id_id instead
**		  of id_instance.
**	22-aug-1995 (dougb)
**	    As a related update to the fix for bug #70696, update the
**	    parameters to LK_do_new_lock_request().  free_res is now a bit
**	    mask.  Also, we no longer need to init lkb->lkb_type.
**      06-sep-1995 (duursma)
**	    Simplifications due to the fact that we now rely on the VMS
**	    distributed lock manager to detect live lock (if we are on a
**	    cluster).  Ignore local compatibility on a cluster because we will
**	    enqueue the lock request anyway.  As a result, local deadlock may
**	    occur even if a request was queued, so do local deadlock search
**	    regardless of value of "gdlk_search".  Also added some error
**	    checks resulting in TRdisplay() calls.
**	14-Dec-1995 (jenjo02)
**	    Function caller must ensure that llb_mutex 
**	    and rsb_mutex's are held.
**	08-jan-1996 (duursma)
**	    Removed irrelevant TRdisplay() message re locally incompatible locks
**	15-jan-1996 (duursma) bug #71340
**	    Use compute_group_modes() to compute group grant, convert modes.
**	08-feb-1996 (duursma)
**	    Undid last change (also see file header comment)
**	21-aug-1996 (boama01)
**	    For a cluster, if $ENQ has a problem, the RSB and LKB are both
**	    deallocated; we have to let the caller know this.  For now,
**	    we can set a "bogus" LKREQ.function value for _DEBUGGER which
**	    will indicate not to use these any further.
**	11-sep-96 (nick)
**	    Changes to stop leakage of RSBs and LKBs. #78432
**	13-dec-96 (cohmi01)
**	    Add sys_err parm, pass back id of holder upon failure. (Bug 79763)
**	01-aug-1996 (nanpr01 for ICL keving)
**	    Support for Blocking Callbacks.
**	01-aug-1996 (nanpr01 for jenjo02)
**	    Release rsb_mutex before returning on bad status from 
**	    lk_cback_fire().
**	11-Nov-1996 (jenjo02)
**	    Removed 01-aug-1996 change. rsb_mutex must remain locked
**	    on return from this function.
**	15-Nov-1996 (jenjo02)
**	   Optimistic deadlock detection.
**	   Instead of doing deadlock detection every time we must wait for
**	   a lock, queue the waiting LLB to a deadlock wait queue and let the
**	   (new) deadlock detector thread periodically run the list and
**	   resolve deadlocks.
**      28-feb-1997 (stial01)
**         Update llb_status if we've added an EXCL SV_DATABASE lock
**	22-Apr-1997 (jenjo02)
**	   Removed LKREQ_CB_DEALLOCATED flag, whose use was causing
**	   memory leaks when the LKB it was being set in was deallocated.
**	   Instead, changed dequeue_cbs() to be passed a pointer to int
**	   "res_to_free" in which flags will be set indicating which, if
**	   any resources were actually freed.
**      21-May-1997 (stial01)
**         Update llb_status if we've added an EXCL ROW lock
**	26-Jan-2004 (jenjo02)
**	   Don't forget to set blocking_mode in lkreq...
**	26-Feb-2004 (devjo01)
**	   Correct handling of distributed NOWAIT locks.
**  22-sep-2004 (fanch01)
**     Remove meaningless LK_TEMPORARY flag.
**	30-Apr-2008 (jonj)
**	   On new distributed LKBs, clean out lkb_cx_req before use
**	   to avoid misleading artifacts from lockstat.
**      15-Sep-2008 (hanal04) Bug 120900
**         The current code still allows a failing lock requestor to
**         report a blocking pid/sid that is the same as the requestor's
**         sid/pid.
*/
static STATUS
LK_do_new_lock_request( 
LLB		*proc_llb,
LKB 		*lkb, 
RSB		**rsbp,
i4		flags,
LK_COMP_HANDLE	*completion,
CL_ERR_DESC	*sys_err)
{
    LKD                 *lkd = (LKD *)LGK_base.lgk_lkd_ptr;
    LLB			*llb, *owner_llb;
    LKB			*next_lkb;
    LKB			*prev_lkb;
    LKB			*blocking_lkb;
    LLBQ		*prev_llbq;
    LLBQ		*next_llbq;
    RSB			*rsb;
    RSB			*prev_rsb;
    STATUS		status;
    STATUS		MutexStatus;
    LKH    		*lkh_table;
    SIZE_TYPE		*lbk_table;
    RSH    		*rsh_table;
    RSB			*next_rsb;
    LKBQ		*lkbq;
    LKBQ		*next_lkbq;
    LKH			*lkb_hash_bucket;
    RSH			*rsb_hash_bucket;
    u_i4		rsb_hash_value;
    u_i4		dlmflags;
    i4			err_code;
    SIZE_TYPE		end_offset;
    SIZE_TYPE		lkb_offset;
    SIZE_TYPE		llbq_offset;
    i4			lock_granted;
    i4			is_locally_compatible;
    i4			distributed_lock;

    LK_WHERE("LK_do_new_lock_request")

    /* NB: RSB is the only thing mutexed by the caller, the LLB is not */

    /* "*rsbp" will be set to NULL if RSB is freed for some unlikely reason */
    rsb = *rsbp;

    /* "proc_llb" is the lock requestor's, "llb" is the list holding locks */

    llb = (LLB *)LGK_PTR_FROM_OFFSET(lkb->lkb_lkbq.lkbq_llb);
    
#ifdef CSP_DEBUG_LEVEL2
    if (csp_debug)
	TRdisplay("%@ LK_new_lk_rqst: Lockid set to %x.%x for new lock\n",
			lkb->lkb_id, rsb->rsb_id);
#endif

    /*	Set attributes of the lock before potential suspend */

    distributed_lock = (lkd->lkd_status & LKD_CLUSTER_SYSTEM) &&
		       !(flags & LK_LOCAL); 
    if (distributed_lock)
    {
	lkb->lkb_attribute |= LKB_DISTRIBUTED_LOCK;
    }

    if (flags & LK_PHYSICAL)
    {
	lkb->lkb_attribute |= LKB_PHYSICAL;
	lkb->lkb_count = 1;
    }
    else
	lkb->lkb_count = 0;

    /*
    ** Now check if the lock can be granted.
    ** To avoid live lock situation, check if any other locks are waiting
    ** in either conversion or waiting queue. If other locks are waiting
    ** in either queue, the new lock request is queued at the end of the
    ** waiting queue. If both queues are empty, then determine if the new
    ** lock request is compatible with the other granted locks.
    */

    is_locally_compatible =
	/* check whether conversion queue is empty */
	( ( rsb->rsb_converters == 0 )

	 /* check whether wait queue is empty */
	 && ( rsb->rsb_waiters == 0 )

	 /* check whether request is compatible with group grant mode */
	 && ( 1 & ( LK_compatible [rsb->rsb_grant_mode]
		   >> lkb->lkb_request_mode)));

    if ( is_locally_compatible ||
	 ( distributed_lock && (flags & LK_NOWAIT) == 0) )
    {
	/*
	** Either we're on a cluster and we're allowed to wait, or we are
	** locally compatible.  If we're on a cluster we don't really care 
	** about local compatibility.  We'll let the DLM handle it.
	*/

	/* Assume the lock will be granted */
	lock_granted = TRUE;
	
# if defined(conf_CLUSTER_BUILD)
	if ( distributed_lock )
	{
	    /* Clean and setup CX request block */
	    CX_INIT_REQ_CB(&lkb->lkb_cx_req, lkb->lkb_request_mode,
				&rsb->rsb_name);
	    lkb->lkb_cx_req.cx_owner = lkb->lkb_cpid;
	    lkb->lkb_cx_req.cx_user_extra =
		 (void *)CX_PTR2RELOFF(&lkb->lkb_cx_req,lkb);
	    lk_set_dlm_callback(lkb, rsb);

	    dlmflags = (flags & CX_LK_SHARED_REQ_FLAGS) |
	     CX_F_STATUS | CX_F_OWNSET | CX_F_PROXY_OK;
	    switch ( rsb->rsb_name.lk_type )
	    {
	    case LK_CONTROL:
	    case LK_AUDIT:
	    case LK_CONFIG:
	    case LK_OPEN_DB:
		/*
		** These locks can always be requested by a process
		** that is not [yet] a full member of the cluster.
		** (has not called CXjoin()).   Because of
		** this, they must be called in process context,
		** as opposed to group context.  This is OK
		** because these locks have no recovery consequences,
		** but are used to pass information and serialize
		** access to internal resources.
		*/
		dlmflags |= CX_F_PCONTEXT;
		break;

	    case LK_BM_LOCK:
	    case LK_BM_DATABASE:
	    case LK_BM_TABLE:
		if ( lkd->lkd_status & LKD_NO_DLM_GLC )
		{
		    /*
		    ** If we lack a true group lock container, then
		    ** take these particular locks in the context of
		    ** the CSP (VMS cluster only).
		    */
		    dlmflags |= CX_F_IS_CSP;
		}
		/* Drop into next case */

	    default:
		/* 
		** All other lock types are taken in group context. 
		** Locks covering activity that may be rolled back,
		** or that should only appear on a "protected"
		** lock list, MUST be obtained in a group context,
		** which in turn requires that the calling process
		** be "joined" to the cluster.  Exceptions should
		** be made carefully on a case by case basis.
		*/
		break;
	    }

	    /* Count another enqueue */
	    lkd->lkd_stat.enq++;

	    status = CXdlm_request( dlmflags,
		       &lkb->lkb_cx_req, (LK_UNIQUE *)llb->llb_name );

	    if ( E_CL2C08_CX_W_SUCC_IVB == status )
	    {
		/*
		** Distributed lock manager reports that value
		** block associated with lock has been marked
		** invalid.  This indicates that a lock holder
		** has abended while holding a lock in an update
		** mode.  If processing of locks has not already
		** been stalled, then do so now.  If LK had not
		** already been stalled, then broadcast the need
		** for the master CSP to perform recovery.
		*/
		status = lk_handle_ivb( llb, lkb, flags, TRUE, 0, 0, __LINE__ );
	    }

	    if ( E_CL2C01_CX_I_OKSYNC == status ||
		 E_CL2C08_CX_W_SUCC_IVB == status ||
		 (OK == status && (dlmflags & CX_F_NOWAIT))
	       )
	    {
		/*
		**  Lock was synchronously granted by the DLM.
		**  Compatible, mark LKB as granted and update RSB group
		**  grant mode.
		*/
		lkd->lkd_stat.synch_complete++;

		if ( E_CL2C08_CX_W_SUCC_IVB != status )
		{
		    /*
		    ** Refresh RSB lock values from DLM.
		    */
		    rsb->rsb_value[0] = lkb->lkb_cx_req.cx_value[0];
		    rsb->rsb_value[1] = lkb->lkb_cx_req.cx_value[1];
		    rsb->rsb_invalid = lkb->lkb_cx_req.cx_value[2];
		}
		else
		{
		    /*
		    ** DLM lock was granted, but associated value
		    ** block has been invalidated.  Mark rsb_invalid.
		    ** If caller is interested in value block, he
		    ** will get an LK_VAL_NOTVALID return code.
		    */
		    rsb->rsb_invalid = 1;
		}

#ifdef CSP_DEBUG
		if (csp_debug)
		{
		    LK_UNIQUE	lock_id;

		    TRdisplay(
		     "%@ CSP: enqueue was synchronously granted.\n");

		    CX_EXTRACT_LOCK_ID( &lock_id, &lkb->lkb_cx_req.cx_lock_id );
		    TRdisplay("  DLM lkid %x.%x  lock type: %d  lock key:"
		     "%6(%x %) (%t)\n",
		      lock_id.lk_uhigh, lock_id.lk_ulow,
		      rsb->rsb_name.lk_type,
		      rsb->rsb_name.lk_key1, rsb->rsb_name.lk_key2, 
		      rsb->rsb_name.lk_key3, rsb->rsb_name.lk_key4, 
		      rsb->rsb_name.lk_key5, rsb->rsb_name.lk_key6,
		      sizeof(LK_LOCK_KEY), (char*)&rsb->rsb_name.lk_key1);
		    TRdisplay("  RETURNED lock value: %4.4{ %x%}\n",
		      lkb->lkb_cx_req.cx_value);
		    if ( rsb->rsb_invalid )
			TRdisplay("  lock value is INVALID\n");
		    TRflush();
		}
#endif
		status = LK_GRANTED;
	    }
	    else if ( OK != status )
	    {
		if ( E_CL2C21_CX_E_DLM_DEADLOCK == status )
		{
		    /* 103715 note:
		    **
		    ** This case for immediately detected 
		    ** deadlocks was added for 103715.
		    */
		    status = LK_DEADLOCK;
		}
		else if ( E_CL2C27_CX_E_DLM_NOGRANT == status )
		{
		    /* 103715 note:
		    **
		    ** Here if LK_NOWAIT, and convert not globally
		    ** compatible.
		    */
		    status = LK_BUSY;
		}
		else if ( status == E_CL2C39_CX_E_OS_CONFIG_ERR ||
		          status == E_CL2C3A_CX_E_OS_CONFIG_ERR ||
		          status == E_CL2C3B_CX_E_OS_CONFIG_ERR )
		{
		    /* OS resource depletion */
		    status = LK_NOLOCKS;
		}

		return(status);
	    }
	    else
	    {
		/*
		** Request is pending. DLM will resume session when the
		** request completes.
		*/
		lock_granted = FALSE;
#ifdef CSP_DEBUG
		if (csp_debug)
		    TRdisplay(
		     "%@ CSP: enqueue pending with RETURNED lock value.\n");
#endif
	    }
	} /* if DLM */
# endif

	if ( lock_granted )
	{
	    /*
	    **  Compatible, mark LKB as granted and update RSB group
	    **  grant mode.
	    */

	    lkb->lkb_grant_mode = lkb->lkb_request_mode;
	    if (lkb->lkb_request_mode > rsb->rsb_grant_mode)
		rsb->rsb_convert_mode = rsb->rsb_grant_mode = lkb->lkb_request_mode;

	    /* Queue the LKB to the head of the RSB grant queue. */

	    lkb->lkb_q_next = rsb->rsb_grant_q_next;
	    lkb->lkb_q_prev = LGK_OFFSET_FROM_PTR(&rsb->rsb_grant_q_next);
	    next_lkb = (LKB *)LGK_PTR_FROM_OFFSET(rsb->rsb_grant_q_next);
	    next_lkb->lkb_q_prev =
		rsb->rsb_grant_q_next = LGK_OFFSET_FROM_PTR(lkb);

	    lkb->lkb_state = LKB_GRANTED;

            if (lkb->lkb_cback.cback_fcn)
                rsb->rsb_cback_count++;

	    status = LK_GRANTED;
	}
    }
    else
	lock_granted = FALSE;

    if (!lock_granted)
    {
	/*
	** before waiting, timing out, or deadlocking, store the session
	** id of at least one holder of the lock for error msgs. This will
	** be found up by LK_get_completion_status.  If clustered it is
	** possible that there will be no local blocker, in which case
	** this info will be set elsewhere (if possible), or left as
	** zero (where obtaining this info is judged too expensive.).
	**
	**  Note also that on a cluster the lock may timeout before we
	**  ever get a chance to see if it's globally compatible or not.
        **
        ** Make sure we pick a different holder than the current session.
	*/
        end_offset = LGK_OFFSET_FROM_PTR(&rsb->rsb_grant_q_next); 
	blocking_lkb = (LKB*)NULL;

        for (lkb_offset = rsb->rsb_grant_q_next;
             lkb_offset != end_offset;
             lkb_offset = next_lkb->lkb_q_next)
        {
	    next_lkb = (LKB *)LGK_PTR_FROM_OFFSET(lkb_offset);
            if(next_lkb->lkb_cpid.sid != lkb->lkb_cpid.sid ||
               next_lkb->lkb_cpid.pid != lkb->lkb_cpid.pid)
            {
		blocking_lkb = next_lkb;
                break;
            }
        }
 
	if ( blocking_lkb )
	{
	    /* Blocker on local node */
	    completion->blocking_cpid = blocking_lkb->lkb_cpid;
	    completion->blocking_mode = blocking_lkb->lkb_grant_mode;
	}
	else
	{
	    /* Unknown blocker, maybe, on remote node */
	    completion->blocking_cpid.pid = 0;
	    completion->blocking_cpid.sid = 0;
	    completion->blocking_mode = 0;
	}

	/*
	**  The lock can't be granted immediately.
	**  If the NOWAIT flag was specified return an error.
	**  If deadlock is detected then return an error.
	**  Otherwise suspend the lock list.
	*/

	if (flags & LK_NOWAIT)
	    return (LK_BUSY);

	/*
	** If lock is not granted, we might be blocked by a local lock.
	** Do local deadlock detection regardless of whether global deadlock
	** search is required (removed requirement that gdlk_search be FALSE)
	** Optimistic deadlock detection.
	** 
	** Rather than checking for deadlock every time we're
	** about to wait, go ahead and wait and let the deadlock
	** detector thread (LKdeadlock_thread()) periodically do the
	** checking.
	*/

	if (llb->llb_status & LLB_RECOVER)
	{
	    /* 
	    ** If the lock list is owned by RCP, check who is holding 
	    ** the resource.
	    */
	    end_offset = LGK_OFFSET_FROM_PTR(&rsb->rsb_grant_q_next);
	    for (lkb_offset = rsb->rsb_grant_q_next;
		    lkb_offset != end_offset;
		    lkb_offset = next_lkb->lkb_q_next)
	    {
		next_lkb = (LKB *)LGK_PTR_FROM_OFFSET(lkb_offset);
		owner_llb = (LLB *)
			    LGK_PTR_FROM_OFFSET(next_lkb->lkb_lkbq.lkbq_llb);

		if (owner_llb->llb_status & LLB_ORPHANED)
		{
		    /*
		    ** We don't want to wait for this lock request,
		    ** because it's blocked by a lock held by an orphaned
		    ** lock list. Locks on orphaned lock lists are waiting
		    ** for the RCP to clean them up, so we don't want to
		    ** have the situation where the RCP is waiting on one lock
		    ** list for a lock held by another lock list which is
		    ** the responsibility of the RCP; this would be an
		    ** undetected deadlock. Instead, we have this special
		    ** return code called LK_RETRY in this case.
		    **
		    ** In a cluster installation, note that we must cancel the
		    ** outstanding SYS$ENQ if we issued it before returning
		    ** the LK_RETRY status to the caller. We have to
		    ** allow for the case that SYS$DEQ may report to us that
		    ** the lock is magically granted.
		    */
		    if ( !distributed_lock )
			return (LK_RETRY);

		    /* 103715 note:
		    **
		    ** 'gdlk_search' always set for cluster case, so above
		    ** if (gdlk_search) is redundant.  Also allow dequeue_lock
		    ** to return LK_CANCEL if lock was granted.  This is
		    ** both "better", and simpler behaviour.
		    **
		    ** <FIXME> note, Ingres has always aborted the blocked
		    ** lock request, even if the mode the orphaned lock is
		    ** held in is compatible with the requested mode.  This
		    ** seems wrong, since in that case, the orphaned list
		    ** is not blocking grant of the lock, and is not party
		    ** to any potential deadlock situation.  We should
		    ** confirm that a "incompatibility" test should be added
		    ** before allowing the presense of an "orphaned" lock
		    ** list to abort the current request.
		    */
		    status = dequeue_lock( rsbp, lkb );

		    if ( status == OK )
		        status = LK_RETRY;

		    if ( status != LK_CANCEL )
			return status;

		    break;
		}
	    }
	}

	status = OK;

	/*  Take LKB off free list and finish initializing. */

	lkb->lkb_state = LKB_WAITING;

	/* Queue the LKB to the tail of the RSB wait queue */

	lkb->lkb_q_next = LGK_OFFSET_FROM_PTR(&rsb->rsb_wait_q_next);
	lkb->lkb_q_prev = rsb->rsb_wait_q_prev;
	prev_lkb = (LKB *)LGK_PTR_FROM_OFFSET(rsb->rsb_wait_q_prev);
	prev_lkb->lkb_q_next =
	    rsb->rsb_wait_q_prev = LGK_OFFSET_FROM_PTR(lkb);

	lkd->lkd_tstat[0].wait++;
	lkd->lkd_tstat[rsb->rsb_name.lk_type].wait++;

        /* 
        ** (ICL keving)
        ** If this resource has lkbs with callbacks in its granted queue and  
        ** if this lock has been placed at the head of the waiting queue
        ** and if the converting queue is empty then check to see if callbacks
        ** should be fired.
        */

	if ( rsb->rsb_waiters++ == 0 && rsb->rsb_cback_count
		&& rsb->rsb_converters == 0 )
        {
	    /* Fire any callbacks that are conflicting */
	    status = lk_cback_fire(rsb, lkb->lkb_request_mode);
	    /* Just return status */ 
	    if (status) 
		return(status);
        }

	/* Set this while holding rsb_mutex */
	if ( distributed_lock )
	    lkb->lkb_attribute |= LKB_ENQWAIT;

	/*  Suspend this lock request. */
	LK_suspend(lkb, flags);
    }
    return (status);
}


/*{
** Name: LK_do_convert - Convert a granted lock to a different mode.
**
** Description:
**	All the arguments have been verified, now try and convert from 
**	grant_mode to request_mode.  Look for the special case of a 
**	conversion with no interference. Then check to see if it is 
**	compatible, if so grant it.  If not compatible return if 
**	'noqueue'  or 'ignore_econsist' is set,	or conversion deadlock,
**	or resource deadlock.
**
**	Note that the conversion can be to a higher or to a lower mode.
**
** Inputs:
**      lkb                             The lock to try and convert.
**	mode				The new lock mode.
**      nowait                          If set and lock will wait, abort.
**
** Outputs:
**	Returns:
**	    status			In a non-cluster installation, the
**					lock conversion status is returned:
**
**					OK if conversion request must wait.
**					LK_GRANTED if conversion was granted.
**					LK_VAL_NOTVALID if conversion was
**					    granted and caller asked for the
**					    current lock value and there is none
**					LK_BUSY if conversion would have
**					    to wait but caller requested
**					    LK_NOWAIT in the flags.
**					LK_DEADLOCK if the conversion resulted
**					    in a deadlock and the caller didn't
**					    pass the LK_NODEADLOCK flag.
**					LK_RETRY if this is apparently an RCP
**					    related lock list and it would have
**					    waited on a lock held by an
**					    orphaned lock list.
**
**	    In a cluster installation, the same status codes are returned but
**	    the return code is ignored. The return status is sent back to the
**	    actual lock requestor through the "wakeup" function.
**
**	Exceptions:
**	    none.
**
** Side Effects:
**	    none
**
** History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable logging and locking system.
**	16-feb-1993 (andys)
**	    If we set LLB_WAITING, set llb_wait also.
**	26-jul-1993 (bryanp)
**	    Renamed to LK_do_convert and exported, for use by LK_alter().
**	20-sep-1993 (bryanp) merged 23-mar-1993 (bryanp)
**	    Fix a number of bugs surrounding handling of invalid lock values:
**	    3) When a sys$enq call completes synchronously (SS$_SYNCH is
**		returned), we must check the lock status block for the
**		SS$_VALNOTVALID return code. This means that the lock was
**		synchronously granted/converted but that the value is marked
**		invalid. We must ensure that in this case we mark the
**		rsb_invalid field in the RSB so that LK_VALNOTVALID is returned
**		to the caller.
**	    4) VMS doesn't support marking a resource as value-not-valid when
**		converting down from >=SIX and not providing the value. Since
**		we don't need this functionality, we'll return LK_BADPARAM
**		instead of trying to mark the value invalid (and ending up
**		marking it locally invalid but globally valid).
**	15-dec-1994 (medji01)
**	    Mutex Granularity Project
**		- Acquire RSB mutex during grant queue scans
**		- Acquire LKD, RSH, and RSB mutexes before calling
**		  LK_try_wait() and LK_try_convert()
**		- In cluster code acquire LLB mutex for shared LLB's
**	28-mar-1995 (medji01)
**	    64K+ Lock Limit Project
**		- Removed references to id_instance in LK_ID.
**		- Changed error messages to reference id_id instead
**		  of id_instance.
**      06-sep-1995 (duursma)
**	    Simplifications due to the fact that we now rely on the VMS
**	    distributed lock manager to detect live lock (if we are on a
**	    cluster).  Ignore local compatibility on a cluster because we will
**	    enqueue the lock request anyway.  As a result, local deadlock may
**	    occur even if a request was queued, so do local deadlock search
**	    regardless of value of "gdlk_search".  Also added some error
**	    checks resulting in TRdisplay() calls.
**	15-Dec-1995 (jenjo02)
**	    llb_mutex must be held by function caller and
**	    remains latched on return from this function.
**	15-jan-1996 (duursma) bug #71340
**	    Use compute_group_modes() to compute group grant, convert modes.
**	08-feb-1996 (duursma)
**	    Undid last change (also see file header comment)
**	24-jul-96 (nick)
**	    Changed loop to avoid compiler warning messages.
**	21-aug-1996 (boama01)
**	    Changes to LK_try_wait() and LK_try_convert() calls; chg *RSB
**	    parm to **RSB.
**	    Also reworked unmutexes of RSB when returning from conversion
**	    deadlocks; only unmutex just before return, or you could end up
**	    with lock actually being granted async after dequeue_conversion()
**	    but after RSB was already unmutexed.
**	16-dec-96 (cohmi01)
**	    Add sys_err parm, pass back id of holder upon failure. (Bug 79763)
**	05-Sep-1996 (jenjo02)
**	    While spinning on dlock_mutex, release llb_mutex as well.
**	01-aug-1996 (nanpr01 for ICL keving)
**	    If a callback function has been passed in with this call then
**	    let it overwrite the callback currently in the LKB.
**	    LK_try_wait/convert routines now return a status.
**	    Check for callbacks to fire if our new mode clashes with
**	    waiters.
**	01-aug-1996 (nanpr01 for jenjo02)
**	    Release rsb_mutex before returning on bad status from
**	    lk_cback_put().
**	11-Nov-1996 (jenjo02)
**	    Moved call to LK_suspend() after a LK_do_convert() inside of
**	    LK_do_convert() so the LLB can be marked LLB_WAITING while
**	    the rsb_mutex is held. When done without the rsb_mutex, a
**	    window is opened whereby deadlock() may encounter a converting
**	    LKB whose LLB isn't yet flagged as waiting. The LLB is 
**	    skipped by deadlock() and a deadlock goes undetected.
**	09-Jan-1997 (jenjo02)
**	    Check for non-conversion first (new mode = old mode), don't
**	    lock rsb_mutex and make a quick exit.
**      02-jul-97 (dilma04)
**          Update llb_status if converting a SV_DATABASE or ROW lock 
**          from shared mode to exclusive.
**      23-Oct-2002 (gygro01)
**         b87049 - Make sure that the resource holder and the deadlocked
**         sessionid is different in the error message by walking through
**         the resource holder queue and pick a different than the current
**         session.
**      07-Jan-2003 (jenjo02)
**         Removed reference to lkreq->blocking_mode which isn't defined
**         until after 2.6.
**	26-Feb-2004 (devjo01)
**	   Correct handling of distributed NOWAIT locks.
**	06-apr-2005 (devjo01)
**	   Conversions to the same write mode will now update the LVB.
**	21-jun-2005 (devjo01)
**	    Do not overwrite cx_owner when converting a lock, and
**	    LKD_NO_DLM_GLC is set.
**	05-Aug-2005 (jenjo02)
**	   When can't convert and LK_NOWAIT, restore request_mode
**	   to grant_mode so lockstat won't show misleading info
**	   about the lock.
**	24-oct-2005 (devjo01)
**	    Make sure new_group_mode is properly initialized even when
**	    converting to the same mode.
**	04-Apr-2008 (jonj)
**	    Set LKB_ENQWAIT before making DLM call so that another
**	    request on this LKB will be rejected before doing damage.
**	25-Aug-2008 (jonj)
**	    If owning DLM process has died (LK_NOTHELD) and this is the
**	    RCP trying to recover that process, then grant the locally
**	    compatible lock to give recovery every chance to succeed.
**	    Do not overwrite lock owner's callback info if no GLC to
**	    prevent callback from being directed to the wrong callback
**	    thread.
**	09-Jan-2009 (jonj)
**	    Implement new LLB_MULTITHREAD mutexing strategy.
**	    Initialize LK_COMP_HANDLE, callback stuff in here.
*/
static STATUS
LK_do_convert(
LLB		*proc_llb,
LKB		*lkb,
i4		flags,
i4		mode,
LK_VALUE	*value,
LK_COMP_HANDLE	*completion,
LKcallback	callback,
PTR		callback_arg,
CL_ERR_DESC     *sys_err)
{
    LKD		    *lkd = (LKD *)LGK_base.lgk_lkd_ptr;
    RSB	            *rsb;
    LKB		    *next_lkb;
    LKB		    *prev_lkb;
    LKB		    *blocking_lkb;
    i4		    try_conversion;
    i4		    try_wake_up;
    i4		    is_compat;
    bool	    lock_granted;
    bool	    gdlk_search;
    i4              new_cback;
    i4		    new_group_mode;
    STATUS	    status, local_status;
    STATUS	    MutexStatus;
    i4		    previous_mode;
    SIZE_TYPE	    end_offset;
    SIZE_TYPE	    lkb_offset;
    LLB		    *llb, *owner_llb;
    i4		    is_locally_compatible;

    LK_WHERE("LK_do_convert")

    /* Fill in the blanks common to explicit and implicit conversions */
    completion->lkb_offset = LGK_OFFSET_FROM_PTR(lkb);
    lkb->lkb_request_id = completion->request_id;
    lkb->lkb_status = OK;


#ifdef LK_TRACE
    if (LK_rqst_trace())
    {
	TRdisplay("%@ LK_do_convert Entry: lkb: %p, mode: %d, prev_mode: %d\n",
	    lkb, mode, lkb->lkb_grant_mode);
	TRflush();
    }
#endif
    /* "proc_llb" is the lock requestor's, "llb" is the list holding locks */

    llb = (LLB *)LGK_PTR_FROM_OFFSET(lkb->lkb_lkbq.lkbq_llb);
    rsb = (RSB *)LGK_PTR_FROM_OFFSET(lkb->lkb_lkbq.lkbq_rsb);

    /*
    ** If multithreaded list, then rsb_mutex is held, otherwise
    ** llb_mutex is held.
    */

    /* The lock must be granted before a convert is legal. */

    if (lkb->lkb_state != LKB_GRANTED)
    {
	if ( llb->llb_status & LLB_MULTITHREAD )
	    (VOID)LK_unmutex(&rsb->rsb_mutex);
	return (LK_CVTNOTGRANT);
    }


    /*
    ** If this is a non-conversion (new mode = old mode) and it's
    ** not a cluster, we can process it quickly and avoid having
    ** to lock the RSB.
    */
    if ((previous_mode = lkb->lkb_grant_mode) == mode &&
	(lkb->lkb_attribute & LKB_DISTRIBUTED_LOCK) == 0)
    {
	status = LK_GRANTED;

	if ( value )
	{
	    value->lk_high = rsb->rsb_value[0];
	    value->lk_low  = rsb->rsb_value[1];
	    value->lk_mode = mode;
	    if (rsb->rsb_invalid)
		status = LK_VAL_NOTVALID;
	}

	if ( llb->llb_status & LLB_MULTITHREAD )
	    (VOID)LK_unmutex(&rsb->rsb_mutex);
	    
	return(status);
    }

    /* If we don't have the rsb_mutex, now we need it */
    if ( !(llb->llb_status & LLB_MULTITHREAD) )
    {
	if (MutexStatus = LK_mutex(SEM_EXCL, &rsb->rsb_mutex))
	    return(MutexStatus);
    }

    /*
    ** It's not necessary to lock the deadlock matrix here while
    ** changing lkb_request_mode because deadlock() only looks at
    ** lkb_request_modes of converting or waiting LKBs, and this 
    ** LKB is not yet in either of those states, hence won't be 
    ** seen by deadlock().
    */

    lkb->lkb_request_mode = mode;
    
    if (previous_mode >= LK_SIX && lkb->lkb_request_mode <= previous_mode)
    {
	if ( value )
	{
	    lkb->lkb_cx_req.cx_value[0] = rsb->rsb_value[0] = value->lk_high;
	    lkb->lkb_cx_req.cx_value[1] = rsb->rsb_value[1] = value->lk_low;
#ifdef LK_TRACE
	    if (LK_rqst_trace())
	    {
		TRdisplay("%@ LK_do_convert; lkb/rsb lock value set to: %x %x\n", 
		    value->lk_high, value->lk_low);
		TRflush();
	    }
#endif
	    lkb->lkb_cx_req.cx_value[2] = rsb->rsb_invalid = 0;
    	}
	else
	{
	    /*
	    ** The old lock manager used to mark the value invalid here (by
	    ** setting rsb->rsb_invalid to 1). However, this is not supported
	    ** by the VMS Distributed Lock Manager. That is, the only time
	    ** that a holder of the lock can explicitly mark the value invalid
	    ** is when the lock is dequeued. Therefore, the lock manager now
	    ** leaves the value the way it is and leaves the rsb_invalid set to
	    ** 0.
	    ** 
	    ** Actually, this code never runs, because LKrequest() rejects
	    ** converting down without a value as LK_BADPARAM.
	    **
	    ** rsb->rsb_invalid = 1;
	    */
	}
    }

    /*
    ** (ICL keving)
    ** If the caller has passed in a blocking callback function then
    ** we overwrite the callback currently in the LKB.
    */
    new_cback = FALSE;
    if (callback != (LKcallback) NULL)
    {
        /* If there was no callback before then inc. rsb's cback count */
        if (lkb->lkb_cback.cback_fcn == (LKcallback) NULL)
        {
            rsb->rsb_cback_count++;
            new_cback = TRUE;
        }
        lkb->lkb_cback.cback_fcn = callback;
        lkb->lkb_cback.cback_arg = callback_arg;
    }

    new_group_mode = rsb->rsb_grant_mode;

    for (status = LK_GRANTED;;)
    {
	is_compat = FALSE;
	try_wake_up = FALSE;
	try_conversion = FALSE;
	lock_granted = FALSE;
	gdlk_search = FALSE;
	is_locally_compatible = TRUE;

	/*
	** If we're converting this lock to the same mode, then the conversion
	** can be immediately granted in our local lock manager. However, if
	** this is a cluster installation we still must re-request the lock
	** from the distributed lock manager (in order to refresh the lock
	** value).
	*/

	if (previous_mode == lkb->lkb_request_mode)
	    break;

	/*
	** If this LKB is the only granted entry, we can again immediately
	** grant the conversion in our local lock manager. In addition, since
	** we may be converting "down", we also want to check to see if any
	** of the waiting lock requests can now be granted. Again, if this is
	** a cluster installation we must issue the VMS SYS$ENQ call to convert
	** the lock to its new mode.
	*/

	if ( lkb->lkb_q_next == LGK_OFFSET_FROM_PTR(&rsb->rsb_grant_q_next) &&
	     lkb->lkb_q_prev == LGK_OFFSET_FROM_PTR(&rsb->rsb_grant_q_next) )
	{
	    try_wake_up = TRUE;
	    break;
	}

	/*
	** If the grant mode of the converting lock is equal to the grant
	** mode of the lock on the head of the convert queue, then we have
	** to recalculate the group grant mode and if the conversion is
	** granted we may have to try and convert other locks waiting 
	** to convert.
	*/
	if (previous_mode == rsb->rsb_convert_mode)
	{
	    /*
	    ** If the granted mode of the converting lock equals the granted 
	    ** mode of the lock at the head of the conversion queue then we 
	    ** have to try converting some locks if this conversion is granted.
	    ** Otherwise no conversions qualify.
	    */

	    try_conversion = TRUE;
	    new_group_mode = LK_N;

	    end_offset = LGK_OFFSET_FROM_PTR(&rsb->rsb_grant_q_next);
	    for (lkb_offset = rsb->rsb_grant_q_next;
		lkb_offset != end_offset;
		lkb_offset = next_lkb->lkb_q_next)
	    {
		next_lkb = (LKB *)LGK_PTR_FROM_OFFSET(lkb_offset);

		if (next_lkb != lkb && 
		    next_lkb->lkb_grant_mode > new_group_mode)
		{
		    new_group_mode = next_lkb->lkb_grant_mode;
		}
	    }
	}

	if ((LK_compatible[new_group_mode] >> lkb->lkb_request_mode) & 1)
	{
	    /*
	    ** The mode we're converting this lock to is compatible with the
	    ** group grant mode of all the other locks which have been locally
	    ** granted. The lock may be locally granted. If this is a cluster
	    ** installation, then we must convert the lock in the DLM
	    ** We remember the fact that the lock is locally compatible:
	    */
	    is_compat = TRUE;
	    break;
	}

	is_locally_compatible = FALSE;

	if ((lkb->lkb_attribute & LKB_DISTRIBUTED_LOCK)
	    && ((flags & LK_NOWAIT) == 0))
	{
	    /* 
	    ** If on a cluster and we're allowed to wait, don't worry about
	    ** local compatibility.  Lock must be queued with the DLM.
	    */
	    break;
	}

	/*
	** If we get here, then the lock conversion cannot yet be granted,
	** because it is not compatible with other granted locks. It will
	** have to wait for these (local) locks to be released.
	*/
	status = OK;
	break;
    } 

    if (status == LK_GRANTED)
    {
# if defined(conf_CLUSTER_BUILD)
	if ((lkb->lkb_attribute & LKB_DISTRIBUTED_LOCK) == 0)
	{
	    /*
	    ** In a non-cluster installation, once the lock is locally
	    ** compatible we can grant it; there's nothing more to do.
	    */
	    lock_granted = TRUE;
	}
	else
	{
	    /*
	    ** The lock convert request is locally compatible. Now check 
	    ** whether it is globally compatible.
	    */
	    
	    /*	Change various counters. */

	    lkd->lkd_stat.enq++;

	    /*	Issue the ENQ lock request. */

#ifdef CSP_DEBUG
	    if (csp_debug)
	    TRdisplay("%@ CSP: Enqueuing conversion of %x to mode %d.\n",
			lkb->lkb_id,
			lkb->lkb_request_mode);
#endif

	    /* Setup CX request block */
	    lkb->lkb_cx_req.cx_new_mode = lkb->lkb_request_mode;
	    lkb->lkb_cx_req.cx_key = rsb->rsb_name;
	    lkb->lkb_cx_req.cx_user_extra =
		    (void *)CX_PTR2RELOFF(&lkb->lkb_cx_req,lkb);
	    /*
	    ** If no GLC:
	    **		o Do not overwrite lock "owner" PID.
	    **		o Do not overwrite the callback info
	    **		  established by that lock owner.
	    **		  The callback, when triggered,
	    **		  must be directed to the lock owner's
	    **		  callback thread, not this converter's.
	    */
	    if ( lkd->lkd_status & LKD_NO_DLM_GLC )
	    {
		if ( completion->cpid.pid == lkb->lkb_cx_req.cx_owner.pid )
		    lkb->lkb_cx_req.cx_owner.sid = completion->cpid.sid;
	    }
            else
            {
		lkb->lkb_cx_req.cx_owner = completion->cpid;
		lk_set_dlm_callback(lkb, rsb);
            }

	    /*
	    ** Set LKB_ENQWAIT now so that another request on
	    ** this LKB will be rejected.
	    */
	    lkb->lkb_attribute |= LKB_ENQWAIT;

	    status = CXdlm_convert( 
		  (flags & CX_LK_SHARED_REQ_FLAGS) |
		   CX_F_STATUS | CX_F_OWNSET | CX_F_PROXY_OK, 
		   &lkb->lkb_cx_req );

	    if ( E_CL2C08_CX_W_SUCC_IVB == status )
	    {
		/*
		** Distributed lock manager reports that value
		** block associated with lock has been marked
		** invalid.  This indicates that a lock holder
		** has abended while holding a lock in an update
		** mode.  If processing of locks has not already
		** been stalled, then do so now.  If LK had not
		** already been stalled, then broadcast the need
		** for the master CSP to perform recovery.
		*/
		status = lk_handle_ivb( llb, lkb, flags, TRUE, 0, 0, __LINE__ );
	    }

	    if ( E_CL2C01_CX_I_OKSYNC == status ||
		 E_CL2C08_CX_W_SUCC_IVB == status ||
		 (OK == status && (flags & CX_F_NOWAIT))
	       )
	    {
		/* Un-"busy" this LKB */
		lkb->lkb_attribute &= ~LKB_ENQWAIT;

		lkd->lkd_stat.synch_complete++;

		if ( E_CL2C08_CX_W_SUCC_IVB != status )
		{
		    rsb->rsb_value[0] = lkb->lkb_cx_req.cx_value[0];
		    rsb->rsb_value[1] = lkb->lkb_cx_req.cx_value[1];
		    rsb->rsb_invalid = lkb->lkb_cx_req.cx_value[2];
		}
		else
		{
		    rsb->rsb_invalid = 1;
		}
#ifdef CSP_DEBUG
		if (csp_debug)
		{
		LK_UNIQUE	lock_id;

		CX_EXTRACT_LOCK_ID( &lock_id, &lkb->lkb_cx_req.cx_lock_id );
		TRdisplay("%@ CSP: Conversion %s was synchronously granted.\n",
		    (lkb->lkb_request_mode < previous_mode) ? "DOWN" : "UP");
		TRdisplay("DLM lkid %x.%x %s lock value: %4.4{ %x%}\n",
		  lock_id.lk_uhigh, lock_id.lk_ulow,
    		  (previous_mode >= LK_SIX && 
		      lkb->lkb_request_mode < previous_mode)
			? "STORED" : "RETURNED",
		  lkb->lkb_cx_req.cx_value);
		if ( rsb->rsb_invalid )
		    TRdisplay("  lock value is INVALID\n");
		TRflush();
		}
#endif
		status = LK_GRANTED;

		lock_granted = TRUE;
		if (! is_locally_compatible)
		    TRdisplay("%@ LK_do_convert: locally incompatible lock \
granted by VMS lock manager!\n");
	    }
	    else if ( OK != status )
	    {
		/* Un-"busy" this LKB */
		lkb->lkb_attribute &= ~LKB_ENQWAIT;

		if ( E_CL2C21_CX_E_DLM_DEADLOCK == status )
		{
		    /* 103715 note:
		    **
		    ** This case for immediately detected 
		    ** deadlocks was added for 103715.
		    */
		    status = LK_DEADLOCK;
		}
		else if ( E_CL2C27_CX_E_DLM_NOGRANT == status )
		{
		    /* 103715 note:
		    **
		    ** Here if LK_NOWAIT, and convert not globally
		    ** compatible.
		    */
		    status = LK_BUSY;
		}
		else if ( E_CL2C25_CX_E_DLM_NOTHELD == status )
		{
		    char	keydispbuf[256];

		    /*
		    ** If RCP recovery, the DLM lock owning process may have died.
		    ** During LK_rundown on that process, only NON_PROTECT locks
		    ** are released and there's a chance that the RCP/DMVE will
		    ** request an implicit lock conversion on a dead-owner lock.
		    ** We want recovery to succeed, so we'll "un-distribute" this
		    ** locally compatible lock and pretend that nothing out of
		    ** the ordinary happened.
		    */
		    if ( llb->llb_status & LLB_RECOVER )
		    {
			TRdisplay("%@ LK_do_convert: LK_NOTHELD, granting local for recovery\n"
			    "\t llb %x status %x owner %x.%x\n"
			    "\t lkb %x flags %x state %d attr %x grant %d req %d dlm %x\n"
			    "\t cx_owner %x,%x key %s\n",
			    *(u_i4*)&llb->llb_id, 
			    llb->llb_status, llb->llb_cpid.pid, llb->llb_cpid.sid,
			    lkb->lkb_id,
			    flags,
			    lkb->lkb_state,
			    lkb->lkb_attribute,
			    lkb->lkb_grant_mode, 
			    lkb->lkb_request_mode,
			    lkb->lkb_cx_req.cx_lock_id,
			    lkb->lkb_cx_req.cx_owner.pid,
			    lkb->lkb_cx_req.cx_owner.sid,
			    LKkey_to_string(&rsb->rsb_name, keydispbuf));

			lkb->lkb_attribute &= ~LKB_DISTRIBUTED_LOCK;
			status = LK_GRANTED;
			lock_granted = TRUE;
		    }
		    else
		    {
			/* This is serious. At least tell what we know */
			TM_STAMP	tim;
			char	err_buf[256];
			CS_SID	sid;

			TMget_stamp(&tim);
			TMstamp_str(&tim, err_buf);
			CSget_sid(&sid);

			STprintf(err_buf + STlength(err_buf),
				    " %x LK_do_convert: LK_NOTHELD llb %x status %x owner %x.%x\n"
				    "\t lkb %x flags %x state %d attr %x grant %d req %d dlm %x\n"
				    "\t cx_owner %x,%x key %s\n",
				    sid,
				    *(u_i4*)&llb->llb_id, 
				    llb->llb_status, llb->llb_cpid.pid, llb->llb_cpid.sid,
				    lkb->lkb_id,
				    flags,
				    lkb->lkb_state,
				    lkb->lkb_attribute,
				    lkb->lkb_grant_mode, 
				    lkb->lkb_request_mode,
				    lkb->lkb_cx_req.cx_lock_id,
				    lkb->lkb_cx_req.cx_owner.pid,
				    lkb->lkb_cx_req.cx_owner.sid,
				    LKkey_to_string(&rsb->rsb_name, keydispbuf));
			ERlog(err_buf, STlength(err_buf), sys_err);
			CS_breakpoint() ;

			/*
			** No DLM lock is held on the resource. 
			**
			** Discard the LKB, return LK_NOTHELD
			*/
			lkb->lkb_attribute &= ~LKB_DISTRIBUTED_LOCK;
			lkb->lkb_attribute |= LKB_RSB_IS_MUTEXED;
			(VOID)LK_do_unlock(lkb, (LK_VALUE*)NULL, (i4)0, FALSE);

			/* Lock ID no longer valid. */
			return(LK_NOTHELD);
		    }

		}
		else if ( status == E_CL2C39_CX_E_OS_CONFIG_ERR ||
		          status == E_CL2C3A_CX_E_OS_CONFIG_ERR ||
		          status == E_CL2C3B_CX_E_OS_CONFIG_ERR )
		{
		    /* OS resource depletion */
		    status = LK_NOLOCKS;
		}

		if ( status != LK_GRANTED )
		{
		    /* 
		    ** Conversion failed immediately.
		    */
		    (VOID)LK_unmutex(&rsb->rsb_mutex);
		    return status;
		}
	    }
	    else
	    {
		/*
		** The lock was locally compatible, but was not
		** synchronously granted by the DLM. Therefore, there
		** may be a lock on another node blocking this conversion
		** request. Thus, we will have to wait for our conversion to
		** be granted, and this may mean that there is a possible
		** deadlock, so we should request a global deadlock search.
		*/
#ifdef CSP_DEBUG
		if (csp_debug)
		{
		TRdisplay("%@ CSP: conversion %s enqueue pending",
		    (mode < previous_mode) ? "DOWN" : "UP");
		TRdisplay(" with %s lock value\n",
    		  (previous_mode >= LK_SIX && lkb->lkb_request_mode < previous_mode)
			? "STORED" : "RETURNED");
		}
#endif
		gdlk_search = TRUE;
	    }
	}
# else
	lock_granted = TRUE;
# endif
    }

    if (!lock_granted)
    {
	/*
	** At this point the lock may have to wait
	*/

        /*
        ** before waiting, timing out, deadlocking or returning BUSY,
        ** store the session id of another holder of the lock for
        ** error msgs. This will be found by LK_get_completion_status.
	**
	** Note that in a cluster, our LKB may be the only LKB on this node.
        */

        /* b111911 - Make sure to pick a different holder than the current */
        end_offset = LGK_OFFSET_FROM_PTR(&rsb->rsb_grant_q_next);
	blocking_lkb = (LKB*)NULL;

        for (lkb_offset = rsb->rsb_grant_q_next;
             lkb_offset != end_offset;
             lkb_offset = next_lkb->lkb_q_next)
        {
            next_lkb = (LKB *)LGK_PTR_FROM_OFFSET(lkb_offset);
            if(next_lkb->lkb_cpid.sid != lkb->lkb_cpid.sid ||
                    next_lkb->lkb_cpid.pid != lkb->lkb_cpid.pid)
            {
	        blocking_lkb = next_lkb;
                break;
            }
        }

	if ( blocking_lkb )
	{
	    /* Blocker on local node */
	    completion->blocking_cpid = blocking_lkb->lkb_cpid;
	    completion->blocking_mode = blocking_lkb->lkb_grant_mode;
	}
	else
	{
	    /* Unknown blocker, maybe, on remote node */
	    completion->blocking_cpid.pid = 0;
	    completion->blocking_cpid.sid = 0;
	    completion->blocking_mode = 0;
	}

	if (flags & LK_NOWAIT)
	{
	    /* Can't convert, restore request_mode = grant_mode */
	    lkb->lkb_request_mode = lkb->lkb_grant_mode;
	    /* Un-"busy" this LKB */
	    lkb->lkb_attribute &= ~LKB_ENQWAIT;
	    (VOID) LK_unmutex(&rsb->rsb_mutex);
	    return (LK_BUSY);
	}

	/*
	**  Not compatible, check for conversion deadlocks by checking that this
	**  locks granted mode is incompatible with the requested mode of 
	**  converting ahead of it. In a cluster installation, the lock may be
	**  locally compatible but not globally compatible, in which case
	**  lock_granted is false but gdlk_search is true.
	**
	**  Note also that on a cluster the lock may timeout before we
	**  ever get a chance to see if it's globally compatible or not.
	*/
	
	if ( (flags & LK_NODEADLOCK) == 0 && rsb->rsb_converters )
	{
	    /*
	    ** Lock was not locally compatible, and NODEADLOCK flag was off.
	    */
	    end_offset = LGK_OFFSET_FROM_PTR(&rsb->rsb_grant_q_next);
	    lkb_offset = rsb->rsb_grant_q_prev;

	    lkd->lkd_stat.convert_search++;

	    next_lkb = (LKB *)LGK_PTR_FROM_OFFSET(lkb_offset);
	    while (lkb_offset != end_offset &&
		    next_lkb->lkb_state == LKB_CONVERT)
	    {
		next_lkb = (LKB *)LGK_PTR_FROM_OFFSET(lkb_offset);
		if ((LK_compatible[lkb->lkb_grant_mode] >>
						next_lkb->lkb_request_mode) & 1)
		    lkb_offset = next_lkb->lkb_q_prev;
		else
		    break;
	    }


	    /*
	    ** Optimistic deadlock detection.
	    ** 
	    ** Rather than checking for deadlock every time we're
	    ** about to wait, go ahead and wait and let the deadlock
	    ** detector thread (LKdeadlock_thread()) periodically do the
	    ** checking.
	    */
	    if ((lkb_offset != end_offset &&
		next_lkb->lkb_state == LKB_CONVERT) )
	    {
		/* Return id and request mode of real blocking converter */
		completion->blocking_cpid = next_lkb->lkb_cpid;
		completion->blocking_mode = next_lkb->lkb_grant_mode;

		if (lkb->lkb_attribute & LKB_DISTRIBUTED_LOCK)
		{
		    if (gdlk_search)
		    {
			/*
			** If lock is not granted, we might be blocked by a
			** local lock.  Do local deadlock detection regardless
			** of whether global deadlock search is required
			** (removed requirement that gdlk_search be FALSE)
			*/
			status = dequeue_convert( lkb, &lock_granted );
		    }
		    /* Un-"busy" this LKB */
		    lkb->lkb_attribute &= ~LKB_ENQWAIT;
		}
		if ( !lock_granted )
		{
		    lkd->lkd_tstat[0].convert_deadlock++;
		    lkd->lkd_tstat[rsb->rsb_name.lk_type].convert_deadlock++;

		    if ( lkd->lkd_status & LKD_SHOW_DEADLOCKS )
		    {
			i4	stamp = lkd->lkd_tstat[0].convert_deadlock;
			show_deadlock(next_lkb, stamp);
			show_deadlock(lkb, stamp);
		    }
		    (VOID) LK_unmutex(&rsb->rsb_mutex);
		    return (LK_DEADLOCK);
		}
	    }
	}

	if ( !lock_granted && llb->llb_status & LLB_RECOVER )
	{
	    /* 
	    ** If the lock list is owned by RCP, check who is holding the
	    ** resource.
	    */
		
	    end_offset = LGK_OFFSET_FROM_PTR(&rsb->rsb_grant_q_next);
	    for (lkb_offset = rsb->rsb_grant_q_next;
		    lkb_offset != end_offset;
		    lkb_offset = next_lkb->lkb_q_next)
	    {
		next_lkb = (LKB *)LGK_PTR_FROM_OFFSET(lkb_offset);
		owner_llb = (LLB *)
			    LGK_PTR_FROM_OFFSET(next_lkb->lkb_lkbq.lkbq_llb);

		if (llb->llb_status & LLB_ORPHANED)
		{
		    /*
		    ** We don't want to wait for this conversion request,
		    ** because it's blocked by a lock held by an orphaned
		    ** lock list. Locks on orphaned lock lists are waiting
		    ** for the RCP to clean them up, so we don't want to
		    ** have the situation where the RCP is waiting on one lock
		    ** list for a lock held by another lock list which is
		    ** the responsibility of the RCP; this would be an
		    ** undetected deadlock. Instead, we have this special
		    ** return code called LK_RETRY in this case.
		    **
		    ** In a cluster installation, note that we must cancel the
		    ** outstanding SYS$ENQ if we issued it before returning
		    ** the LK_RETRY status to the caller. We have code to
		    ** allow for the case that SYS$DEQ may report to us that
		    ** the lock is magically granted, although I think that's
		    ** impossible in this case because the holder of the lock
		    ** which is blocking us is in ORPHANED state and couldn't
		    ** possibly have released its lock yet.
		    */
		    if (gdlk_search)
		    {
			status = dequeue_convert( lkb, &lock_granted );
		    }
		    /* Un-"busy" this LKB */
		    lkb->lkb_attribute &= ~LKB_ENQWAIT;

		    if (lock_granted)
			break;

		    (VOID) LK_unmutex(&rsb->rsb_mutex);
		    return (LK_RETRY);
		}
	    }
	}
    }
    

    if (lock_granted)
    {
	/*
	** Lock convert request is granted, update RSB group grant 
	** mode and convert mode. 
	*/

	if (try_wake_up)
	{
	    /*
	    ** This lock request was the only granted request.
	    ** Set the new granted mode in the LKB and call try_wait to see
	    ** if converting the lock will allow any waiters to be granted.
	    */

	    lkb->lkb_grant_mode = lkb->lkb_request_mode;

	    if ( rsb->rsb_waiters )
		LK_try_wait(&rsb, lkb->lkb_grant_mode); 
	    else
		rsb->rsb_grant_mode = rsb->rsb_convert_mode 
				= lkb->lkb_grant_mode;
	}
	else if (is_compat)
	{
	    /*
	    ** The mode we're converting this lock to is compatible with the
	    ** group grant mode of all the other locks which have been locally
	    ** granted.
	    */
	    lkb->lkb_grant_mode = lkb->lkb_request_mode;
	    if (lkb->lkb_request_mode > new_group_mode)
		new_group_mode = lkb->lkb_request_mode;
	    if ( try_conversion && (rsb->rsb_converters || rsb->rsb_waiters) )
		LK_try_convert(&rsb, new_group_mode); 
	    else
		rsb->rsb_grant_mode = rsb->rsb_convert_mode = new_group_mode;
	}

	if ((lkb->lkb_attribute & LKB_DISTRIBUTED_LOCK) == 0)
	{
	    /* 103715 note: 
	    ** Blocking call backs are handled by DLM on clustered systems.
	    */

	    /* 
	    ** (ICL keving) We have granted the convert. If we have converted up and
	    ** have a callback or we have just gained a callback then check to
	    ** see if we are clashing with the first waiter.
	    */
	    if ( (lkb->lkb_cback.cback_fcn &&
		 lkb->lkb_request_mode > previous_mode) || new_cback )
	    {
		/* Find first converting LKB */
		if ( rsb->rsb_converters )
		{
		    end_offset = LGK_OFFSET_FROM_PTR(&rsb->rsb_grant_q_next);
		    for (lkb_offset = rsb->rsb_grant_q_next;
			 lkb_offset != end_offset;
			 lkb_offset = next_lkb->lkb_q_next)
		    {
			next_lkb = (LKB *)LGK_PTR_FROM_OFFSET(lkb_offset);
			if (next_lkb->lkb_state == LKB_CONVERT)
			    break;
		    }
		    if (next_lkb->lkb_state == LKB_CONVERT)
		    {
			/* Found first converter. Does it clash? */
			if (((LK_compatible[lkb->lkb_grant_mode] >> 
			      next_lkb->lkb_request_mode) & 1) == 0)
			{
			    if (local_status = lk_cback_put(next_lkb->lkb_request_mode, 
						  lkb))
			    {
				(VOID)LK_unmutex(&rsb->rsb_mutex);
				return (local_status);
			    }
			}
			
		    }
		}
		/* No Converters - check head of wait queue */
		else if ( rsb->rsb_waiters )
		{
		    next_lkb = (LKB *)LGK_PTR_FROM_OFFSET(rsb->rsb_wait_q_next);
		    if (((LK_compatible[lkb->lkb_grant_mode] >> 
			  next_lkb->lkb_request_mode) & 1) == 0)
		    {
			if (local_status = lk_cback_put(next_lkb->lkb_request_mode, 
					      lkb))
			{
			    (VOID)LK_unmutex(&rsb->rsb_mutex);
			    return (local_status);
			}
		    }
		}    
	    }       
	}

	status = LK_GRANTED;

	/*
	** (boama01, 21-aug-1996) Quick note: no need to check if rsb was
	** set to NULL by LK_try_wait() above; since we just granted a lock,
	** its grant queue isn't empty and it would not have been deallocated.
	*/

	if ( value && lkb->lkb_request_mode >= previous_mode )
	{
	    /*
	    ** The caller is not converting down and has requested the current
	    ** lock value. If this is a cluster installation, we must first
	    ** update the RSB's copy of the lock value from the LKSB's copy
	    ** which we got from VMS.
	    */
	    value->lk_high = rsb->rsb_value[0];
	    value->lk_low = rsb->rsb_value[1];
	    value->lk_mode = lkb->lkb_request_mode;
	    if (rsb->rsb_invalid)
		status = LK_VAL_NOTVALID;
	}
	/* Un-"busy" this LKB */
	lkb->lkb_attribute &= ~LKB_ENQWAIT;

#ifdef LK_TRACE
	if (LK_rqst_trace())
	{
	    TRdisplay("%@ LK_do_convert; conversion granted, status: %x\n", status); 
	    TRflush();
	}
#endif
	
	/* Release rsb_mutex before possibly taking llb_mutex */
	(VOID) LK_unmutex(&rsb->rsb_mutex);

        /*
        ** Update llb_status if converting SV_DATABASE or ROW lock to EXCL 
        */
	if ( mode == LK_X &&
		(rsb->rsb_name.lk_type == LK_SV_DATABASE ||
		 rsb->rsb_name.lk_type == LK_ROW) &&
	     (llb->llb_status & (LLB_XROW | LLB_XSVDB)) == 0 )
	{
	    /* Mutex llb if multithread */
	    if ( llb->llb_status & LLB_MULTITHREAD &&
	         (MutexStatus = LK_mutex(SEM_EXCL, &llb->llb_mutex)) )
	    {
		return(MutexStatus);
	    }
	    if ( rsb->rsb_name.lk_type == LK_SV_DATABASE )
		llb->llb_status |= LLB_XSVDB;
	    else if ( rsb->rsb_name.lk_type == LK_ROW )
		llb->llb_status |= LLB_XROW;
	    if ( llb->llb_status & LLB_MULTITHREAD )
	        (VOID)LK_unmutex(&llb->llb_mutex);
	}

	return (status);
    }

    /*
    ** We only get here if the conversion request will wait. There is much
    ** bookkeeping to do to record the waiting lock request.
    **
    ** By now, we've stored blocking cpid info in completion.
    */

    /* Tell LK_resume to fetch lock value, status into LKB */
    if ( value )
        lkb->lkb_attribute |= LKB_VALADDR_PROVIDED;

    /* Record blocking info in LKB for extraction when resumed */
    lkb->lkb_blocking_cpid = completion->blocking_cpid;
    lkb->lkb_blocking_mode = completion->blocking_mode;

    lkb->lkb_status = status = OK;

    /*  Remove the LKB from the RSB grant queue. */

    next_lkb = (LKB *)LGK_PTR_FROM_OFFSET(lkb->lkb_q_next);
    prev_lkb = (LKB *)LGK_PTR_FROM_OFFSET(lkb->lkb_q_prev);
    next_lkb->lkb_q_prev = lkb->lkb_q_prev;
    prev_lkb->lkb_q_next = lkb->lkb_q_next;

    /*  Insert LKB onto the tail of the RSB grant queue. */

    lkb->lkb_q_prev = rsb->rsb_grant_q_prev;
    lkb->lkb_q_next = LGK_OFFSET_FROM_PTR(&rsb->rsb_grant_q_next);
    prev_lkb = (LKB *)LGK_PTR_FROM_OFFSET(rsb->rsb_grant_q_prev);
    prev_lkb->lkb_q_next =
	rsb->rsb_grant_q_prev = LGK_OFFSET_FROM_PTR(lkb);

#ifdef LK_TRACE
    if (LK_rqst_trace())
    {
	TRdisplay("%@ LK_do_convert; convert must wait, suspending\n"); 
	TRflush();
    }
#endif
    
    /*	Make this list wait for the lock to be granted. */

    lkd->lkd_tstat[0].convert_wait++;
    lkd->lkd_tstat[rsb->rsb_name.lk_type].convert_wait++;
    lkb->lkb_state = LKB_CONVERT;

    /*	Store the convert mode if this is the first convert on the queue. */
    if ( rsb->rsb_converters++ == 0 )
        rsb->rsb_convert_mode = new_group_mode;

    /*  Suspend the lock request */

    LK_suspend(lkb, flags);

    /* If this lkb has a cback then decrement rsb's cback count */
    if (lkb->lkb_cback.cback_fcn != (LKcallback) NULL)
        rsb->rsb_cback_count--;

    /* And check to see if we can fire any callbacks */
    if ( rsb->rsb_cback_count && rsb->rsb_converters == 1 )
    {
        /* Fire any callbacks that are conflicting */
        if (local_status = lk_cback_fire(rsb, lkb->lkb_request_mode))
	{
	    (VOID) LK_unmutex(&rsb->rsb_mutex);
            return(local_status);
	}
    }

    /* Set this while holding rsb_mutex */
    if ( lkb->lkb_attribute & LKB_DISTRIBUTED_LOCK )
        lkb->lkb_attribute |= LKB_ENQWAIT;

    (VOID) LK_unmutex(&rsb->rsb_mutex);
    return (OK);
}

/*{
** Name: LKdeadlock_thread - Optimistic Deadlock Detection (ODD) Thread.
**
** Description:
**
**	Periodically awakens to peruse waiting LLBs and see if they
**	are involved in a deadlock. 
**	
**	If the wait results in a deadlock, a suspended thead is
**	awakened and deadlock status returned. The thread which last
**	requested the resource is selected as the deadlock victim,
**	retaining the semantics extant when deadlocks were checked
**	on every wait.
**
**	This thread is initialized during recovery server startup
**	and runs only on the recovery server. 
**
**	Its premature termination is a fatal server error.
**
**	- Addendum for distributed deadlock detection.
**
**	   On some platforms that support clustering (i.e. VMS), the
**	Distributed Lock Manager treats all locks taken by one process as
**	being in one transaction.  This simplistic treatment prevents us from
**	depending on the DLM to handle  deadlock detection, and flags are
**	passed to the DLM to suppress deadlock detection at that level. 
**
**	   When running in this environment, LKD_DISTRIBUTE_DEADLOCK is
**	set in lkd_status, and Ingres is responsible for detecting its
**	own deadlock cycles.   To do this the RCP uses both the core
**	of the regular deadlock detection thread (LKdeadlock_thread below),
**	and a separate thread (LKdist_deadlock_thread in lkdlock.c) for
**	processing deadlock messages from other nodes.
**	
**	   LKdeadlock_thread does the normal checking for a local deadlock
**      cycle via a recursive descent of LK_local_deadlock() calls.  As each
**	branch is searched if we don't find a local deadlock then as we unwind
**	the LK_local_deadlock() recursion and if the waiting LKB is flagged as
**	an LKB_DISTRIBUTED_LOCK we query the DLM using CXdlm_get_blki to get
**	a list of all the blocking lock holders not on this node.  For each
**	distinct node in this list we prepare an LK_DSM for later transmission.
**
**	   The memory used to hold the returned blocking lock info starts
**	as a simple array, but if CXdlm_get_blki ever allocates a bigger
**	array, we keep this larger array and free any previously allocated
**	array.  Note there is currently a strong assumption that only one
**	instance of LKdeadlock_thread is running at any one time.  If we
**	want at some future date to run more than one LKdeadlock_thread
**	instance, some of the static variables dedicated to this purpose will
**	need to be made part of a per-instance control structure.
**
**	   The memory used to transmit the LK_DSM structures is organized
**	as 'n' separate arrays where 'n' is the number of foreign nodes
**	statically configured in the Ingres cluster. (I.e. we allocate
**	two arrays when running in a three node cluster regardless of if
**	the other nodes are currently up or down.)
**
**	   If at any point we find that a top level deadlock candidate
**	(an LKB taken off the deadlock queue) is part of a local deadlock,
**	we discard the LK_DSM's gathered for that LKB before sending,
**	except in the (hopefully) rare case where we've filled one or
**	more of the LK_DSM arrays, and had to flush out some records.
**	The algorithm is tolerant of receiving messages which loop
**	back to locks which have been released, so this will not cause
**	a problem.
**
**	   At the end of every deadlock check cycle, all pending LK_DSM
**	messages are sent to the target node where they will be handled
**	by a separate thread dedicated to processing DSM messages
**	(LKdist_deadlock_thread in lkdlock.c).
**
**
** Inputs:
**	dmc_cb
**	 .type			Must be set to DMC_CONTROL_CB
**	 .length		Must be at least sizeof(DMC_CB)
**
** Outputs:
**	dmc_cb
**	 .error.err_code	Error code, if any.
**	
**	Returns:
**	    E_DB_OK
**	    E_DB_FATAL
**	
**	Exceptions:
**	    none
**
** Side Effects:
**
** History:
**	14-Nov-1996 (jenjo02)
**	    Invented.
**	07-Feb-1997 (jenjo02)
**	    Check status of LK_cancel() of deadlock victim. If the cancel
**	    was successful, only then consider the deadlock condition a
**	    success and resume the victim's thread.
**	06-Aug-1997 (jenjo02)
**	    Moved the handling and awakening of the deadlocked thread to
**	    LK_cancel(), LK_do_cancel().
**	08-Feb-1999 (jenjo02)
**	    LKBs, not LLBs appear on the deadlock wait queue. When a deadlock
**	    is detected, resume the waiting thread instead of trying to cancel
**	    the lock ourselves. The lock requestor will be responsible for
**	    cancelling the request. Tweaked sleep cycle.
**	22-Oct-1999 (jenjo02)
**	    Removed use of lkd_dlock_lock, reorganized code appropriately.
**	09-may-2005 (devjo01)
**	    Hook in distributed deadlock logic.
**	01-Jun-2007 (jonj)
**	    Check for "must_retry" mutex conflicts with dist thread.
**	    Relocated all distributed deadlock tracing stuff to lkdlock,
**	    use LK_DLOCK_TRACE macro.
**	11-Jan-2008 (jonj)
**	    o Better synchronization with LKdist_deadlock_thread. Expose
**	      our SID to it so that when we go to sleep and it needs us
**	      to do something, it'll wake us up.
**	    o Do not "hiccup" wait on mutex collisions if OS threading
**	      (in anticipation of posix threading in VMS).
**	    o If notified by LK_local_deadlock() that our top_lkb has
**	      been async posted, toss any accumulated DSMs for the
**	      resource and move on.
**	15-Mar-2010 (jonj)
**	    Earlier change to add stats by lock type missed one for clusters.
*/
DB_STATUS
LKdeadlock_thread(
DMC_CB		*dmc_cb)
{
    LKD		*lkd = (LKD *)LGK_base.lgk_lkd_ptr;
    LLB		*victim_llb;
    LKB		*victim_lkb;
    RSB		*victim_rsb;
    LKB		*lkb;
    LLBQ	*llbq;
    LLBQ	*next_llbq;
    LLBQ	*prev_llbq;
    SIZE_TYPE	end_offset = LGK_OFFSET_FROM_PTR(&lkd->lkd_dw_next);
    STATUS	status;
    i4          err_code;
    i4		must_retry;
    u_i4	wakes = 0;
    char	keydispbuf[256];
    i4		MaxRequestMode;
					/* all times are in milliseconds */
#define DLOCK_MAX_SLEEP     5000	/* max sleep interval */
#define DLOCK_MIN_SLEEP     10		/* minimum sleep interval between wakeups */
#define DLOCK_DELTA_SLEEP   10		/* increment to extend sleep interval */

    i4		ms_timeout = DLOCK_MAX_SLEEP; /* the current sleep interval */
    i4		delta = DLOCK_DELTA_SLEEP;

    i4		deadlocks;
    i4		queue_len;
    i4		dist_deadlocks;
    bool	OSthreads;

    TRdisplay("%@ Local Deadlock thread %p started on node %d\n",
		dmc_cb->dmc_session_id, lkd->lkd_csid);

    /* Note threading model */
    OSthreads = CS_is_mt();

    /* Expose SID of this thread to LKdist_deadlock_thread() */
    CSget_sid(&lkd->lkd_ldead_sid);

# if defined(conf_CLUSTER_BUILD)
    if ( lkd->lkd_status & LKD_DISTRIBUTE_DEADLOCK )
    {
	/* 
	** Set up for performing our own distributed deadlock
	*/
	
	/*
	** Allocate a separate block of LK_DSM messages
	** for each potential member of the cluster.  
	**
	** Only required if we cannot depend on DLM to
	** handle distributed deadlock detection.
	*/
	status = LK_alloc_dsm_context( &LK_main_dsmc );
	if ( status )
	{
	    uleFormat(NULL, status, (CL_ERR_DESC *)NULL, ULE_LOG,
		    NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 0);
	    return E_DB_ERROR;
	}
    }
# endif /* CLUSTER_BUILD */

    /* Sleep until our interval expires or we're interrupted */
    for (;;)
    {
	/* Cycle between 10ms and 5000ms nap times */
	if ((ms_timeout += delta) > DLOCK_MAX_SLEEP)
	{
	    ms_timeout = DLOCK_MAX_SLEEP;
	    delta = -DLOCK_DELTA_SLEEP;
	}
	else if (ms_timeout <= 0)
	{
	    ms_timeout = DLOCK_MIN_SLEEP;
	    delta = DLOCK_DELTA_SLEEP;
	}

# if defined(conf_CLUSTER_BUILD)
	/* Tell LKdist_deadlock_thread we are sleeping */
	if ( lkd->lkd_status & LKD_DISTRIBUTE_DEADLOCK )
	    CS_ACLR(&lkd->lkd_ldead_running);
# endif /* CLUSTER_BUILD */

	/*
	** Sleep until our interval expires, we're interrupted,
	** or LKdist_deadlock_thread() wakes us.
	*/
	status = CSsuspend(CS_INTERRUPT_MASK | CS_TIMEOUT_MASK, - ms_timeout, 
				(PTR)0);

# if defined(conf_CLUSTER_BUILD)
	/* Tell LKdist_deadlock_thread we are running */
	if ( lkd->lkd_status & LKD_DISTRIBUTE_DEADLOCK )
	    CS_TAS(&lkd->lkd_ldead_running);
# endif /* CLUSTER_BUILD */

	/* When interrupted, it's time to shutdown */
	if (status == E_CS0008_INTERRUPTED)
	{
	    TRdisplay("%@ Local Deadlock thread %p terminated on node %d\n",
			dmc_cb->dmc_session_id, lkd->lkd_csid);
	    return(E_DB_OK);
	}

	if ( (++wakes % 100) == 0 ) 
	    LK_DLOCK_TRACE("%@ %d LKdeadlock_thread: woke from CSsuspend %d\n", __LINE__, wakes);

	/* If no LKBs waiting to be checked, go back to long sleep */
	if (lkd->lkd_dw_prev == end_offset)
	{
	    ms_timeout = DLOCK_MAX_SLEEP;
	    delta = DLOCK_DELTA_SLEEP;
	    continue;
	}

	/* Lock the deadlock wait queue */
	if ( status = LK_mutex(SEM_EXCL, &lkd->lkd_dw_q_mutex) )
	    return(status);

	/* Count another wakeup */
	lkd->lkd_stat.dlock_wakeups++;

	/* Track the max number of LKBs on the queue per wakeup */
	if (lkd->lkd_dw_q_count > lkd->lkd_stat.dlock_max_q_len)
	    lkd->lkd_stat.dlock_max_q_len = lkd->lkd_dw_q_count;

	/* Reset count of deadlocks found during this wakeup */
	deadlocks = queue_len = 0;

	/* Note beginning number of distributed deadlocks */
	dist_deadlocks = lkd->lkd_stat.gdeadlock;
	
	/*
	** Find LKBs to check for deadlocks.
	*/
	while (lkd->lkd_dw_prev != end_offset)
	{
	    /*
	    ** Waiting LKBs are added to the head of the
	    ** deadlock queue, therefore long waiters
	    ** will tend to congregate towards the end.
	    */
	    llbq = (LLBQ *)LGK_PTR_FROM_OFFSET(lkd->lkd_dw_prev);

	    lkb = (LKB *)((char *)llbq - CL_OFFSETOF(LKB, lkb_dead_wait));

	    /*
	    ** Traipsing backwards thru the queue, we may run into
	    ** LKBs that have been added during this wakeup.
	    ** We're only interested in those which have been on
	    ** the queue prior to this wakeup.
	    */
	    if (lkb->lkb_deadlock_wakeup == lkd->lkd_stat.dlock_wakeups)
	    {
		break;
	    }

	    /*
	    ** Remove LKB from the deadlock queue 
	    */
	    next_llbq = (LLBQ *)LGK_PTR_FROM_OFFSET(lkb->lkb_dead_wait.llbq_next);
	    prev_llbq = (LLBQ *)LGK_PTR_FROM_OFFSET(lkb->lkb_dead_wait.llbq_prev);
	    next_llbq->llbq_prev = lkb->lkb_dead_wait.llbq_prev;
	    prev_llbq->llbq_next = lkb->lkb_dead_wait.llbq_next;

	    lkd->lkd_dw_q_count--;
	    lkb->lkb_attribute &= ~(LKB_ON_DLOCK_LIST);

	    /* Release the queue */
	    (VOID)LK_unmutex(&lkd->lkd_dw_q_mutex);

	    queue_len++;

	    /*
	    ** If the LKB appears to be waiting,
	    ** check it for a deadlock.
	    */
	    MaxRequestMode = lkb->lkb_request_mode;

	    /* Reset feedback bits */
	    must_retry = 0;

	    while ( lkb->lkb_attribute & LKB_ON_LLB_WAIT &&
		     !(lkb->lkb_attribute & LKB_DEADLOCKED) &&
		     !(CXDLM_IS_POSTED(&lkb->lkb_cx_req)) )
	    {
# if defined(conf_CLUSTER_BUILD)
		if ( lkd->lkd_status & LKD_DISTRIBUTE_DEADLOCK )
		{
		    /*
		    ** Note position in output message buffers for this
		    ** lock.  As an optimization, we'll cancel delivery
		    ** of distributed deadlock messages for this lock
		    ** if we detect a local deadlock or if top_lkb
		    ** is async posted.
		    */
		    LK_mark_dsm_context( LK_main_dsmc );
		}
# endif /* CLUSTER_BUILD */

		if ( NULL !=
		     (victim_lkb = LK_local_deadlock(lkb, lkb, 
			  &MaxRequestMode, (LK_DSM*)NULL, LK_main_dsmc, 0,
			  &must_retry)) )
		{
		    /*
		    ** Resume the LKB's thread. When it comes out
		    ** of its CSsuspend, it'll find the LKB via 
		    ** LK_get_completion_status, see it deadlocked,
		    ** and call LK_cancel() to cancel the lock or
		    ** conversion, then return LK_DEADLOCK to
		    ** the client thread.
		    */

		    deadlocks++;

		    victim_rsb = (RSB*)LGK_PTR_FROM_OFFSET(victim_lkb->lkb_lkbq.lkbq_rsb);
		    victim_llb = (LLB*)LGK_PTR_FROM_OFFSET(victim_lkb->lkb_lkbq.lkbq_llb);
		    LK_DLOCK_TRACE("%@ %d p%x s%x LLB %x LKB %x rqstid %d state %w mode (%w,%w)\n\t"
		              "%s is locally deadlocked\n",
			__LINE__,
			victim_lkb->lkb_cpid.pid, 
			victim_lkb->lkb_cpid.sid,
			*(u_i4*)&victim_llb->llb_id,
			victim_lkb->lkb_id, 
			victim_lkb->lkb_request_id,
			LKB_STATE, victim_lkb->lkb_state,
			LOCK_MODE, victim_lkb->lkb_grant_mode,
			LOCK_MODE, victim_lkb->lkb_request_mode,
			LKkey_to_string(&victim_rsb->rsb_name, keydispbuf));
		    if (victim_llb->llb_status & LLB_RECOVER)
			CS_breakpoint();

# if defined(conf_CLUSTER_BUILD)
		    if ( lkd->lkd_status & LKD_DISTRIBUTE_DEADLOCK )
		    {
			/*
			** Mark request as deadlocked.  There is some
			** chance that the completion routine has
			** fired off, which would mean we were NOT
			** deadlocked after all.  (Possible if request
			** timed out, was explicitly canceled. etc.)
			*/
		        if ( CXdlm_mark_as_posted(
				&victim_lkb->lkb_cx_req ) )
			{
			    /*
			    ** Discard outgoing LKDLM messages  for this
			    ** search since a local deadlock was found.
			    */
			    victim_lkb->lkb_cx_req.cx_status =
			     E_CL2C21_CX_E_DLM_DEADLOCK;
			    LK_unmark_dsm_context( LK_main_dsmc );
			    LKcp_resume( &victim_lkb->lkb_cpid );
			}
			else
			{
			    /* We were not really deadlocked. */
			    victim_lkb->lkb_attribute &= ~LKB_DEADLOCKED;
			    lkd->lkd_tstat[0].deadlock--;
			    lkd->lkd_tstat[victim_rsb->rsb_name.lk_type].deadlock--;
			    deadlocks--;
			}
		    }
		    else
# endif /* CLUSTER_BUILD */
		    {
			LK_resume(victim_lkb);
		    }

		} /* If victim_lkb found. */

# if defined(conf_CLUSTER_BUILD)
		/* Check must_retry notices */
		else if ( must_retry )
		{
		    /*
		    ** LK_TOP_POSTED indicates the top_lkb was async posted
		    ** while in the bowels of deadlock checking, in which
		    ** case none of the other indicators matter.
		    */
		    if ( must_retry & (LKMR_TOP_POSTED | LKMR_MUTEX_CONFLICT) )
		    {
			/* Toss accumulated DSM's for this resource */
			LK_unmark_dsm_context( LK_main_dsmc );

			if ( must_retry & LKMR_MUTEX_CONFLICT )
			{
			    /* Wait 1sec, retry this LKB */
			    if ( !OSthreads )
				CSsuspend(CS_TIMEOUT_MASK, 1, 0);

			    LK_DLOCK_TRACE("%@ %d LKdeadlock_thread: retrying LKB %x DLM %x\n",
					    __LINE__,
					    lkb->lkb_id, lkb->lkb_cx_req.cx_lock_id);
			    continue;
			}
		    }
		    else if ( must_retry & LKMR_RECHECK )
		    {
			/*
			** Retain any assembled DSM's,
			** rerun deadlock check with mode
			** returned in MaxRequestMode
			*/
			LK_DLOCK_TRACE("%@ %d LKdeadlock_thread: rechecking LKB %x DLM %x (%w,%w)\n",
					__LINE__,
					lkb->lkb_id, lkb->lkb_cx_req.cx_lock_id,
					LOCK_MODE, lkb->lkb_request_mode, 
					LOCK_MODE, MaxRequestMode);
			continue;
		    }
		}
		   
# endif /* CLUSTER_BUILD */
		break;

	    } /* while ( lkb->lkb_attribute & LKB_ON_LLB_WAIT ... */
	    /* 
	    ** Reacquire the queue lock,
	    ** restart the search from the end of the queue.
	    */
	    if ( status = LK_mutex(SEM_EXCL, &lkd->lkd_dw_q_mutex) )
		return(status);
	} /* while (lkd->lkd_dw_prev != end_offset) */

	/* Release the queue */
	(VOID)LK_unmutex(&lkd->lkd_dw_q_mutex);

# if defined(conf_CLUSTER_BUILD)

	if ( lkd->lkd_status & LKD_DISTRIBUTE_DEADLOCK )
	{
	    /*
	    ** Send out any distributed deadlock messages,
	    ** not already sent.  Only needed if LK cannot
	    ** depend on DLM to handle deadlock.
	    */
	    LK_flush_dsm_context( (LK_DSM*)NULL, 
	    			  LK_main_dsmc, 
				  &lkd->lkd_stat.gdlck_sent );
	}
# endif /* CLUSTER_BUILD */

	/* If any local/dist deadlocks, halve the sleep time */
	deadlocks += lkd->lkd_stat.gdeadlock - dist_deadlocks;
	if ( deadlocks )
	    ms_timeout /= 2;

	if (queue_len)
	    LK_DLOCK_TRACE("%@ %d LKdeadlock sleeping %d, deadlocks: %d, qlen: %d\n",
		__LINE__, ms_timeout + delta, deadlocks, queue_len);
    } /* for (;;) */
}

/*{
** Name: LK_local_deadlock - Search for deadlock on local node..
**
** Description:
**      Starting from a given LKB that is waiting, compute a wait for
**	path for this lock list.  If we eventual find the starting lock
**	list then a deadlock exists.
**
**	In the presence of "fair scheduling", this becomes more complex. The
**	fair scheduling protocol, which is implemented in LK_request(), says
**	that if any lock request is already waiting in either the convert or
**	wait queues, then this new lock request must wait as well. This
**	avoids the "livelock" situation where requests in (or conversions to)
**	relatively high lock modes will wait "forever" while lower lock modes
**	continue to be granted.
**
**	However, this means that an active locking system may encounter the
**	situation where some of the locks in the wait queue are actually
**	compatible with ALL the granted locks.
**
**	The deadlock search code takes as its starting point a lock which
**	is waiting for a particular resource. It then examines the resource's
**	granted queue to determine which of the granted locks are blocking
**	the resource. For each of those granted locks, it in turn repeats the
**	process to determine what is blocking them. If, eventually, it arrives
**	back at the original lock list which is about to wait, it reports
**	that a deadlock has been detected. If it exhausts it search without
**	re-arriving at the original lock list, then it reports that there is
**	no deadlock and the lock request is allowed to wait.
**
**	When attempting to determine which granted locks are blocking the
**	resource, we use the following algorithm:
**
**		1) Compute the maximum requested lock mode of all the locks
**		   which are waiting in either the granted or waiting queues
**		   ahead of this lock request.
**
**		   Note that "max" here is the numerical lock mode maximum
**		   except that max(S,IX) = SIX.
**
**		   Note also that since lock requests are granted in the order
**		   in which they are queue, lock requests which are queued
**		   BEHIND this lock request are not considered. Their lock modes
**		   do not affect the decision about which grantees are blocking
**		   us.
**		2) For each granted lock, if the lock is granted in a mode
**		   which is incompatible with the max requested lock mode
**		   computed in step (1), this granted lock request is blocking
**		   us from acquiring this resource.
**
**	In a cluster situation where DLM is not managing deadlock, please
**	see banner for LKdeadlock_thread for additional info on the
**	distributed deadlock algorithm.
**
** Inputs:
**      current_lkb                     The lock block that is waiting.
**	top_lkb				The lock that started it all.
**	MaxRequestMode			Cumulative cluster-wide max       
**					request mode of current_lkb
**					if Dist, lkb_request_mode if Local 
**	inlkdsm				Address of a distributed deadlock
**					search message.  Is always NULL,
**					except when LK_local_deadlock is 
**					called as part of a continuation
**					of a distributed deadlock check.
**	lkdsm_context			Distributed deadlock context to use.
**	depth				The depth of descent into the matrix,
**					recorded in "lgd_stat.max_lcl_dlk_srch".
**
** Outputs:
**	Returns:
**	    LKB *			LKB deadlock victim, or
**					NULL if deadlock does not exist.
**	Exceptions:
**	    none
**
** Side Effects:
**	    All RSB grant and wait queues are frozen via rsb_mutex
**	    while the graph is examined.
**
** History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable logging and locking system.
**	24-may-1993 (bryanp)
**	    Maintain statistics on deadlock search lengths.
**	15-Dec-1995 (jenjo02)
**	    rsb_mutex, llb_mutex are held on entry and exit.
**	    To avoid deadlocking with mutexes held on other RSBs,
**	    set "rsb_dlock", then release the RSB mutex. Other
**	    threads must not acquire the RSB mutex while rsb_dlock
**	    is set. When the deadlock search terminates, the RSB 
**	    mutex is reacquired and rsb_dlock cleared.
**	08-Jan-1996 (duursma) bug #71331
**	    LKdeadlock's counterpart to the fix for bug 71331:
**	    don't recurse onto the same RSB for convert requests.
**	08-Jan-1996 (duursma) bug #71338
**	    Fix for bug 71338.  When computing the max request mode for
**	    convert requests, ignore the wait queue.
**	22-mar-1996 (canor01)
**	    Undid change for 71338.  This causes true deadlocks to go
**	    undetected.
**	23-Apr-1996 (jenjo02)
**	    Cleaned up code in deadlock() with respect to concurrent
**	    threads. RSB->LKB queues were not being protected adequately
**	    during traversal allowing other threads to modify the queues
**	    and causing deadlock() to get lost and, typically, loop
**	    forever.
**	14-May-1996 (jenjo02)
**	    Added lkd_dlock_mutex to single-thread deadlock().
**	    It's not possible to use the LLB to store recursion information
**	    when concurrent threads are allowed to run thru this code
**	    at the same time. Until a better repair can be made, we'll
**	    use this new mutex to single-thread this function.
**	15-Oct-1996 (jenjo02)
**	    Added LLB to aid in detecting an LKB which no longer belongs
**	    to the wait-for graph.
**	31-Oct-1996 (jenjo02)
**	    To "freeze" the graph, SHARE lock each LLB visited 
**	    as we descend the graph, releasing the semaphore only when
**	    we detect a deadlock or ascend back up thru the graph.
**	    With both the RSB and LLB thus locked, we no longer need
**	    to save next_lkb's RSB and LLB info in its LLB.
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
**	15-Nov-1996 (jenjo02)
**	    Now called only by LKdeadlock_thread() deadlock detector thread.
**	    Added **victim into which we return the chosen LKB.
**	    Caller must hold lkd_dlock_mutex.
**	03-Sep-1997 (jenjo02)
**	    Bug 85177, fix from Brian Macker. deadlock() was picking the oldest
**	    rather than the newest lock requestor as the victim. Code added to
**	    find the newest requestor.
**	18-Sep-1997 (jenjo02)
**	    Backed out 03-Sep-1997 change. It caused some deadlocks to go undetected
**	    at worst, and did a no better job of picking a victim then the original
**	    code at best.
**	08-Feb-1999 (jenjo02)
**	    Modified to handle multiple waiting LKBs per LLB.
**	10-Aug-1999 (jenjo02)
**	    Rewrote to call itself recursively for each descent instead of the
**	    confusing stowing and reloading of the recursion info in the LLB.
**	    Now that this code runs in a separate thread, stack depth isn't the
**	    concern it was when each thread invoked deadlock() while deep in
**	    the bowels of Ingres.
**	22-Oct-1999 (jenjo02)
**	    Removed dependence on lkd_dlock_lock. Instead, we lock and
**	    unlock individual RSB mutexes as we descend and ascend the graph, 
**	    thereby freezing each branch of the graph instead of the entire graph.
**      13-Oct-2004 (hanal04) Bug 112946 INGSRV2954
**          Ensure lkd->lkd_stat.deadlock_search is only incremented once
**          for each deadlock search. Failure to do so can lead to
**          unexpected recursion and stack overflow.
**	09-may-2005 (devjo01)
**	    - Hook in distributed deadlock logic, rename from deadlock() to
**	    LK_local_deadlock as this is now called by both LKdeadlock_thread,
**	    and LKdist_deadlock_thread.
**	    _ Allow top_lkb to be null in the case where we are processing
**	    a distributed deadlock search message.
**	    - Add parameters for passing max_request_mode and LK_DSM and a
**	    LK_DSM_CONTEXT to allow info needed for a continued deadlock
**	    search to be cleanly passed.
**	05-Aug-2005 (jenjo02)
**	    Make sure current_lkb's lkbq_llb and lkbq_rsb resolve
**	    to real LLBs and RSBs to avoid the rare dereference of
**	    NULL.
**	21-nov-2006 (jenjo02/abbjo03)
**	    If cvt-cvt deadlock on other than top_lkb, post and wake up
**	    current_lkb or it'll sleep forever.  Use lkreq.request_id in
**	    o_stamp instead of un-thread-safe llb_stamp. 
**	31-Jan-2007 (jonj)
**	    Add must_retry return variable for LKdist_deadlock_thread.
**	    If an RSB is mutexed by the local thread, then unwind,    
**	    releasing all amassed RSB mutexes to avoid mutex deadlock.
**	    LKdist_deadlock_thread will have to retry this call after
**	    waiting a bit.
**	12-Feb-2007 (jonj)
**	    Always check for remote blockers.
**	16-Feb-2007 (jonj)
**	    When called in the top level continuation case, restrict
**	    rsb_grant_queue search to singular "current_lkb".
**	    Replaced localized "nodeseen" predicate with more robust
**	    LK_dup_msg() to minimize duplicate distributed messages.
**	30-Apr-2007 (jonj)
**	    When claiming ownership of an RSB mutex, or the bits from
**	    rsb_depth rather than assigning them lest a late arriver's
**	    intent bit gets trampled.
**	01-Jun-2007 (jonj)
**	    Apply "must_retry" mutex to local vs dist conflicts
**	    as well as dist vs local.
**	    Use CXDLM_IS_POSTED macro to check for asynchronously
**	    completed lock requests.
**	    Relocated all distributed deadlock tracing stuff to lkdlock,
**	    use LK_DLOCK_TRACE macro.
**	18-Jun-2007 (jonj)
**	    Before recursing, make sure wait_rsb is not null.
**	20-Jun-2007 (jonj)
**	    Count deadlock stats here rather than in lkrlse
**	    so we can tally local vs distributed.
**	06-Sep_2007 (jonj)
**	    Continuation of distributed deadlock detection misses,
**	    too much messaging between nodes. Attempt to "cluster stamp"
**	    visited LLBs. Improved tracing for bug finding.
**	    If during a continuation an LKB is encountered that has not
**	    been locally deadlock checked, return "must_retry".
**	01-Oct-2007 (jonj)
**	    Repairs to LLB "cluster stamp" setting/checking.
**	21-Oct-2007 (jonj)
**	    Must "Recheck" continuation if top_lkb to catch otherwise
**	    missed deadlock when MaxRequestMode != ClusterMaxRequestMode.
**	22-Oct-2007 (jonj)
**	    If continuation msg is LK_DSM_IN_RETRY, don't check
**	    cluster stamps. This method replaces saving/restoring
**	    cluster stamp on "must_retry", which covered only the
**	    current LKB, not all stamped LKBs.
**	11-Jan-2008 (jonj)
**	    o Add more exotic "already visited" stamps for cluster
**	      to better know when a lock list has already been checked
**	      and reduce unnecessary messaging.
**	    o Recursively recheck for incompatible locks if
**	      cluster mode isn't the same as local mode.
**	    o Do not check for blockers if current_lkb hasn't 
**	      been locally deadlock checked to cut back on
**	      unneeded messaging.
**	    o Try to avoid sending messages to nodes which have
**	      no granted locks.
**	30-Apr-2008 (jonj)
**	    Delete unused "msg_flags" from LK_DSM, add "msg_node"
**	    for debugging to see where message came from.
**	    Set DLM lockid of original blocking lock in LK_DSM,
**	    check for loopbacks to reduce endless hops.
**	12-May-2008 (jonj)
**	    Watch for LKB state change when recursing on waiting
**	    lock list, restart the list of waiters.
**	23-Jun-2008 (jonj)
**	    Do LK_DLOCK_TRACE on LKMR_STATE_CHANGE only if inlkdsm.
**	29-Aug-2008 (jonj)
**	    Relocate LKD_PURGE_DLOCK_LIST to lkd_purge_dlock.
**	26-Jan-2009 (jonj)
**	    Do not retain rsb_mutex when setting LKMR_RECHECK,
**	    avoiding possible E_CL2517_CS_SMPR_DEADLOCK in
**	    LK_dist thread.
*/
LKB *
LK_local_deadlock(LKB *current_lkb, LKB *top_lkb, i4 *pMaxRequestMode,
                  LK_DSM *inlkdsm, LK_DSM_CONTEXT *lkdsm_context, i4 depth,
		  i4 *must_retry)
{
    RSB		*rsb, *wait_rsb;
    LKB		*wait_lkb;
    LKB		*next_lkb;
    LKB		*dead_lkb;
    LLBQ	*llbq;
    SIZE_TYPE	next_lock_offset;
    SIZE_TYPE	llbq_offset;
    LLB		*llb;
    LLB		*next_llb;
    LKD		*lkd = (LKD *)LGK_base.lgk_lkd_ptr;
    i4		cover_mode;
    SIZE_TYPE	end_offset;
    i4		err_code;
    i4		rsb_depth;
    char	keydispbuf[256];
    char	keydispbuf2[256];
    bool	IsLocallyCompatible = TRUE;
    i4		GrantsConverts = 0;
    i4		MaxRequestMode = *pMaxRequestMode;
    i4		i;

    LK_WHERE("deadlock")

    /* Increment the depth of this graph */
    if ( depth++ == 0 )
    {
	if ( inlkdsm )
	    lkd->lkd_stat.cnt_gdlck_call++;
	/* Don't increment local search if doing recheck */
	else if ( !(*must_retry & LKMR_RECHECK) )
	    lkd->lkd_stat.deadlock_search++;
	*must_retry = 0;
    }
    *must_retry &= ~LKMR_RECHECK;

    if ( depth > lkd->lkd_stat.max_lcl_dlk_srch )
	lkd->lkd_stat.max_lcl_dlk_srch = depth;

    /*
    ** Some invariants:
    **  top_lkb - The LKB which initiated this search.
    **  current_lkb - LKB which led us to this resource.
    **  rsb - Resource blocking current_lkb's request.
    **  MaxRequestMode - Maximum of lock modes of lock
    **   Mode required by current_lkb and all modes held
    **   by locks "ahead" of current_lkb.
    **  llb - Lock list containing this request.
    **
    **  end_offset - Offset to last LKB in rsb's grant queue.
    **  next_lock_offset - Offset to next LKB we're going to check.
    **  next_lkb - Next LKB we're going to check in rsb's grant queue.
    **  next_llb - LLB containing next_lkb.
    */

    llb = (LLB *)LGK_PTR_FROM_OFFSET(current_lkb->lkb_lkbq.lkbq_llb);
    rsb = (RSB *)LGK_PTR_FROM_OFFSET(current_lkb->lkb_lkbq.lkbq_rsb);

    /* Careful, the LKB may have been suddenly deallocated. */
    if ( !llb || !rsb )
	return((LKB*)NULL);

    /* If at any time top_lkb gets async posted, abandon everything. */
    if ( top_lkb && CXDLM_IS_POSTED(&top_lkb->lkb_cx_req) )
    {
	*must_retry |= LKMR_TOP_POSTED;
	return((LKB*)NULL);
    }

    /*
    ** Stamp the RSB to identify the mutex owner to prevent
    ** mutex deadlocks between the local and distributed threads.
    **
    ** If the rsb_mutex is owned by or intended to be owned by the 
    ** opposing thread (Locl or Dist), abandon this search and
    ** return "LKMR_MUTEX_CONFLICT" to the caller in must_retry.
    */
    if ( (!inlkdsm && rsb->rsb_dlock_depth & RSB_MUTEXED_DIST) ||
         (inlkdsm && rsb->rsb_dlock_depth & RSB_MUTEXED_LOCL) )
    {
	if ( inlkdsm )
	    LK_DLOCK_TRACE("%@ %d LK_local_deadlock: Dist RSB mutex conflict,"
			    " must retry msg %d,%d,%d\n",
		    __LINE__,
		    inlkdsm->msg_search, inlkdsm->o_node, inlkdsm->msg_seq);
	else
	    LK_DLOCK_TRACE("%@ %d LK_local_deadlock: Locl RSB mutex conflict,"
			    " must retry LKB %x\n",
		    __LINE__,
		    current_lkb->lkb_id);

	*must_retry |= LKMR_MUTEX_CONFLICT;
	return((LKB*)NULL);
    }

    if ( inlkdsm )
	rsb_depth = depth | RSB_MUTEXED_DIST;
    else
	rsb_depth = depth | RSB_MUTEXED_LOCL;

    /*
    ** If this RSB's mutex hasn't been taken yet by this thread, take it now
    ** to prevent the grant/convert/wait queues from changing.
    ** Note the depth at which the mutex was taken; it can only
    ** be released at that depth.
    **
    ** NB: LKdist_deadlock_thread mutexes the initial RSB; LKdeadlock_thread
    **     does not.
    */
    if ( !(rsb->rsb_dlock_depth & (rsb_depth & (RSB_MUTEXED_DIST | RSB_MUTEXED_LOCL))) )
    {
	STATUS	MutexStatus;

	/* Note intent before waiting for mutex */
	rsb->rsb_dlock_depth |= rsb_depth & (RSB_MUTEXED_DIST | RSB_MUTEXED_LOCL);
	/*
	** Watch for failed mutex - owner may be dead.
	**
	** By the time we get the RSB's mutex, the lock may have been
	** granted or the LKB may no longer belong to this locklist/resource.
	*/
	if ( (MutexStatus = LK_mutex(SEM_EXCL, &rsb->rsb_mutex)) != OK ||
	     current_lkb->lkb_state == LKB_GRANTED ||
	     current_lkb->lkb_attribute & LKB_DEADLOCKED ||
	     current_lkb->lkb_lkbq.lkbq_llb != LGK_OFFSET_FROM_PTR(llb) ||
	     current_lkb->lkb_lkbq.lkbq_rsb != LGK_OFFSET_FROM_PTR(rsb) )
	{
	    /* Return notification of the state change */
	    *must_retry |= LKMR_STATE_CHANGE;
	    rsb->rsb_dlock_depth &= ~rsb_depth;
	    if ( MutexStatus == OK )
		LK_unmutex(&rsb->rsb_mutex);
	    return((LKB *)NULL);
	}
	rsb->rsb_dlock_depth |= rsb_depth;
    }

    /*
    ** If current_lkb is a waiter and its lock list holds no other locks,
    ** then it can't possibly be deadlocked, unless continuation case, where
    ** anything is possible.
    */
    if ( !inlkdsm && current_lkb->lkb_state == LKB_WAITING && llb->llb_lkb_count == 1 )
    {
        /* If RSB was mutexed at this depth, release it */
        if ( (rsb->rsb_dlock_depth & rsb_depth) == rsb_depth )
        {
	    rsb->rsb_dlock_depth &= ~rsb_depth;
            LK_unmutex(&rsb->rsb_mutex);
        }
        return((LKB*)NULL);
    }

    /*
    ** Compute the "max request mode" of all locks which are waiting
    ** for this resource, either converting or truly waiting. Locks waiting
    ** "behind" this lock do not count. Also include the request mode of
    ** this lock.
    */

    if ( MaxRequestMode != LK_X && (inlkdsm == NULL || depth > 1) )
    {
	/*
	** Optimization. If current_lkb's request_mode is LK_X, or
	** if we encounter any LK_X request_modes, break out the
	** loop; we won't find one greater.
	**
	** Also MaxRequestMode passed by LKdist_deadlock_thread is also
	** guaranteed to already reflect the effective blocking mode
	** of the lock and all locks queued ahead of it, since it was
	** calculated by examining ALL locks ahead of the original lock
	** cluster wide so there is no need to recalculate this here.
	*/
	next_lkb = NULL;
	if ( rsb->rsb_converters )
	{
	    end_offset = LGK_OFFSET_FROM_PTR(&rsb->rsb_grant_q_next);
	    for (next_lock_offset = rsb->rsb_grant_q_next;
		next_lock_offset != end_offset;
		next_lock_offset = next_lkb->lkb_q_next)
	    {
		next_lkb = (LKB *)LGK_PTR_FROM_OFFSET(next_lock_offset);

		if (next_lkb->lkb_state == LKB_CONVERT)
		{
		    /* 
		    ** Only INCREASE cummulative MaxRequestMode.
		    ** 
		    ** MaxRequestMode = LK_convert_mode[MaxRequestMode]
		    **  [next_lkb->lkb_request_mode];
		    */
		    cover_mode = LK_convert_mode[MaxRequestMode]
		                                [next_lkb->lkb_request_mode];
		    if ( MaxRequestMode < cover_mode )
		    {
			MaxRequestMode = cover_mode;
		    }
		    if (MaxRequestMode == LK_X || next_lkb == current_lkb)
			break;
		}
	    }
	}

	if ( MaxRequestMode != LK_X && next_lkb != current_lkb )
	{
	    if ( rsb->rsb_waiters )
	    {
		end_offset = LGK_OFFSET_FROM_PTR(&rsb->rsb_wait_q_next);
		for (next_lock_offset = rsb->rsb_wait_q_next;
		     next_lock_offset != end_offset;
		     next_lock_offset = next_lkb->lkb_q_next)
		{
		    next_lkb = (LKB *)LGK_PTR_FROM_OFFSET(next_lock_offset);
			
		    /* 
		    ** Only INCREASE cummulative MaxRequestMode.
		    ** 
		    ** MaxRequestMode = LK_convert_mode[MaxRequestMode]
		    **  [next_lkb->lkb_request_mode];
		    */
		    cover_mode = LK_convert_mode[MaxRequestMode]
		                                [next_lkb->lkb_request_mode];
		    if ( MaxRequestMode < cover_mode )
		    {
			MaxRequestMode = cover_mode;
		    }
		    if (MaxRequestMode == LK_X || next_lkb == current_lkb)
			break;
		}
	    }
	}
    }


    /*
    ** When called from top level continuation case (LKdist_deadlock_thread)
    ** "current_lkb" marks the spot in the grant/convert/wait queue
    ** of the 1st incompatible lock it wants us to deadlock check.
    ** We'll go from there to the end of the grant/convert queue.
    */
    if ( inlkdsm && depth == 1 )
    {
	if ( current_lkb->lkb_state == LKB_WAITING )
	{
	    /*
	    ** If current LKB offered by LKdist_deadlock_thread
	    ** is WAITING, it's already eliminated anything
	    ** on the grant/convert queue from consideration,
	    ** so don't bother rechecking the grant/convert queue.
	    */
	    next_lock_offset = LGK_OFFSET_FROM_PTR(&rsb->rsb_grant_q_next);
	}
	else
	{
	    /* Start with current_lkb if GR/CV */
	    next_lock_offset = LGK_OFFSET_FROM_PTR(current_lkb);
	}
	/* End at end of grant q */
	end_offset = LGK_OFFSET_FROM_PTR(&rsb->rsb_grant_q_next);
    }
    else
    {
	/* Start at front of grant q */
	next_lock_offset = rsb->rsb_grant_q_next;

	/*
	** Go up to current_lkb if CV, 
	** end of grant/convert if WT.
	*/
	if ( current_lkb->lkb_state == LKB_CONVERT )
	    end_offset = LGK_OFFSET_FROM_PTR(current_lkb);
	else
	    end_offset = LGK_OFFSET_FROM_PTR(&rsb->rsb_grant_q_next);
    }

    while ( next_lock_offset != end_offset )
    {
	next_lkb = (LKB *)LGK_PTR_FROM_OFFSET(next_lock_offset);
	next_lock_offset = next_lkb->lkb_q_next;

	/* 
	** We may encounter LKBs which were DEADLOCKED during a previous
	** deadlock() cycle but which haven't had time to be cancelled
	** yet. If we do, skip over them.
	*/
	if (next_lkb->lkb_attribute & LKB_DEADLOCKED)
	    continue;

	next_llb = (LLB *)LGK_PTR_FROM_OFFSET(next_lkb->lkb_lkbq.lkbq_llb);

#if defined(conf_CLUSTER_BUILD)
	/*
	** If continuation above level 1, check if this LKB is the blocking
	** lock that started the search; if so, then elide the loopback 
	** by immediately returning.
	*/
	if ( inlkdsm 
	     && depth > 1
	     && inlkdsm->o_blk_node == rcp_node_id
	     && inlkdsm->o_blk_lockid == next_lkb->lkb_cx_req.cx_lock_id )
	{
	    LK_DLOCK_TRACE("%@ %d LK_local_deadlock: Node %d loopback on lockid %x depth %d\n"
		"\to_llb %x o_lkb %x o_lockid %x o_node %d\n"
		"\t%s\n"
		"\tLLB %x LKB %x\n"
		"\t%s\n",
		__LINE__,
		rcp_node_id,
		next_lkb->lkb_cx_req.cx_lock_id,
		depth,
		inlkdsm->o_llb_id,
		inlkdsm->o_lkb_id,
		inlkdsm->o_lockid.lk_ulow,
		inlkdsm->o_node,
		LKkey_to_string(&inlkdsm->o_lock_key, keydispbuf),
		*(u_i4*)&next_llb->llb_id,
		next_lkb->lkb_id,
		LKkey_to_string(&rsb->rsb_name, keydispbuf2));

	    /* If RSB was mutexed at this depth, release it */
	    if ( (rsb->rsb_dlock_depth & rsb_depth) == rsb_depth )
	    {
		rsb->rsb_dlock_depth &= ~rsb_depth;
		LK_unmutex(&rsb->rsb_mutex);
	    }
	    return((LKB*)NULL);
	}
#endif /* CLUSTER_BUILD */

	/* Count another on the grant/convert queue ahead of current_lkb */
	GrantsConverts++;

	lkd->lkd_stat.dlock_locks_examined++;

	/*  
	** If this granted lock's grant mode is incompatible with the
	** max request mode of the waiters, then this lock request is
	** blocking the resource and may be causing deadlock.
	**
	** When called from LKdist, "current_lkb" may be a converter
	** whose request mode, rather than grant mode, is incompatible
	** with the Cluster Mode and is blocking a lock on another
	** node. We have to deadlock check on this incompatibility as
	** this converter may still block the other node after the
	** conversion is granted at its requested mode.
	*/
	if ( (inlkdsm && depth == 1
	      && ((LK_compatible[next_lkb->lkb_request_mode] >>
			MaxRequestMode) & 1) == 0)
	    ||
	     ((LK_compatible[next_lkb->lkb_grant_mode] >> 
			MaxRequestMode) & 1) == 0 )
	{
	    IsLocallyCompatible = FALSE;

	    /*
	    **  If this lock is incompatible and it belongs to the
	    **  the original lock list that started the search,
	    **  then we have deadlock. A lock list never waits for itself
	    **  however, so we must be careful not to erroneously signal
	    **  a deadlock when we are just converting a currently-granted
	    **  lock to an incompatible mode. Thus we don't report deadlock
	    **  here if the list which we're currently checking ('llb') is
	    **  the list which is making the original lock request.
	    **
	    **  In the continuation case where we've arrived back at the
	    **  originating node (inlkdsm && top_lkb):
	    ** 		Originating transaction is deadlocked during an
	    ** 		2nd or subsequent traversal of a section
	    ** 		of the deadlock graph local to its node.
	    **
	    ** 		I.E. N1:T1(R1) -> N2:T2(R1) -> N1:T3(R2) -> T1(R1).
	    */

	    if ( (!inlkdsm || top_lkb) &&
	        (next_lkb->lkb_lkbq.lkbq_llb ==
		  top_lkb->lkb_lkbq.lkbq_llb &&
		 (inlkdsm ||
		     top_lkb->lkb_lkbq.lkbq_llb !=
		      current_lkb->lkb_lkbq.lkbq_llb)) )
	    {
		if ( lkd->lkd_status & LKD_SHOW_DEADLOCKS )
		    show_deadlock(current_lkb,
			lkd->lkd_stat.deadlock_search);

		/* Mark the top LKB as deadlocked while RSB mutex is held */
		top_lkb->lkb_attribute |= LKB_DEADLOCKED;

		/* 
		** Stuff the blocking thread's id in the victim's
		** lkreq for error reporting. In the continuation
		** case, lkdlock will fill this in.
		*/
		if ( inlkdsm )
		{
		    /* Count another distributed deadlock */
		    lkd->lkd_stat.gdeadlock++;
		}
		else
		{
		    /* Count another local deadlock */
		    if ( top_lkb->lkb_state == LK_CONVERT )
		    {
			lkd->lkd_tstat[0].convert_deadlock++;
			lkd->lkd_tstat[rsb->rsb_name.lk_type].convert_deadlock++;
		    }
		    else
		    {
			lkd->lkd_tstat[0].deadlock++;
			lkd->lkd_tstat[rsb->rsb_name.lk_type].deadlock++;
		    }

		    top_lkb->lkb_blocking_cpid = current_lkb->lkb_cpid;
		    top_lkb->lkb_blocking_mode = current_lkb->lkb_grant_mode;
		}

		/* If RSB was mutexed at this depth, release it */
		if ( (rsb->rsb_dlock_depth & rsb_depth) == rsb_depth )
		{
		    rsb->rsb_dlock_depth &= ~rsb_depth;
		    LK_unmutex(&rsb->rsb_mutex);
		}
		return(top_lkb);
	    }

	    /*
	    **  If this lock list is currently waiting for a lock and
	    **  it hasn't participated in the current deadlock search
	    **  then recurse on this lock list.
	    **  But don't recurse onto the same RSB for convert requests.
	    **  (bug 71331)
	    */
	    if (next_lkb->lkb_state != LKB_GRANTED)
	    {
		if ( inlkdsm )
		    LK_DLOCK_TRACE("%@ %d LK_local_deadlock: Skipped CV (%w,%w)"
			" llb %x dlm %x depth %d\n", 
			__LINE__,
			LOCK_MODE, next_lkb->lkb_grant_mode,
			LOCK_MODE, next_lkb->lkb_request_mode,
			*(u_i4*)&next_llb->llb_id,
			next_lkb->lkb_cx_req.cx_lock_id, depth);
		continue;
	    }

	    /* If no top LKB, then we won't find a local deadlock */
	    if ( !top_lkb && next_lkb->lkb_attribute & LKB_ON_DLOCK_LIST )
		continue;

	    /*
	    ** Skip this LLB if no waiters or already searched
	    ** during this local cycle.
	    */
	    if ( next_llb->llb_waiters &&
		       ( (inlkdsm == NULL &&
			    next_llb->llb_search_count !=
			    lkd->lkd_stat.deadlock_search) ||
			 (inlkdsm != NULL &&
			    next_llb->llb_gsearch_count !=
			    lkd->lkd_stat.cnt_gdlck_call) ) )
	    {
		i4	llb_waiters;

		/*
		** Recurse on this lock list
		** and mark its LLB as locally searched.
		*/

		if ( !inlkdsm )
		    next_llb->llb_search_count = lkd->lkd_stat.deadlock_search;
#if defined(conf_CLUSTER_BUILD)
		else
		{
		    next_llb->llb_gsearch_count = lkd->lkd_stat.cnt_gdlck_call;

		    /*
		    ** Continuation. Check if this LKB's LLB has
		    ** already been cluster searched, but not if it
		    ** has yet to be locally checked.
		    */
		    if ( !(next_lkb->lkb_attribute & LKB_ON_DLOCK_LIST) )
		    {
			for ( i = LLB_MAX_STAMPS_PER_NODE-1; i >= 0; i-- )
			{
			    if ( inlkdsm->msg_search ==
				 next_llb->llb_dist_stamp[inlkdsm->o_node-1][i] )
			    {
				break;
			    }
			}

			/* If already cluster-searched, skip */
			if ( i >= 0 )
			{
			    LK_DLOCK_TRACE("%@ %d LK_local_deadlock: been there, done that "
				"MaxMode %w depth %d GrCv %d\n\t" 
				"LLB %x LKB %x state %w attr %x mode (%w,%w) dlm %x post %d\n\t"
				"\t%s\n\t"
				"cstamp %#.4{%d %} msg_search %d o_node %d\n",
				__LINE__,
				LOCK_MODE, MaxRequestMode, depth,
				GrantsConverts,
				*(u_i4*)&next_llb->llb_id, next_lkb->lkb_id, 
				LKB_STATE, next_lkb->lkb_state,
				next_lkb->lkb_attribute,
				LOCK_MODE, next_lkb->lkb_grant_mode,
				LOCK_MODE, next_lkb->lkb_request_mode, 
				next_lkb->lkb_cx_req.cx_lock_id,
				CXDLM_IS_POSTED(&next_lkb->lkb_cx_req),
				LKkey_to_string(&rsb->rsb_name, keydispbuf),
				LLB_MAX_STAMPS_PER_NODE, &next_llb->llb_dist_stamp[inlkdsm->o_node-1], 0,
				inlkdsm->msg_search,
				inlkdsm->o_node);

			    continue;
			}
		    }
		}
#endif /* CLUSTER_BUILD */

		/* 
		** In the "normal" case (non LK_MULTITHREAD, shared LLBs)
		** there will be a single waiting LKB on the LLB.
		** LK_MULTITHREAD-type lock lists may, however, have
		** multiple waiting LKBs (these lock lists are shared by
		** multiple threads), so to accomodate this case a 
		** list of waiting LKBs is hung off the LLBs and we
		** must involve all of them in the wait-for graph.
		*/
		    
		do
		{
		    llb_waiters = next_llb->llb_waiters;

		    /* Pick first waiting LKB on LLB's list */
		    llbq_offset = next_llb->llb_lkb_wait.llbq_next;

		    /* Check each LKB on LLB's wait queue */
		    while ( llb_waiters == next_llb->llb_waiters && llbq_offset &&
			    llbq_offset !=
				LGK_OFFSET_FROM_PTR(&next_llb->llb_lkb_wait.llbq_next) )
		    {
			i4	WaitRequestMode;

			llbq = (LLBQ *)LGK_PTR_FROM_OFFSET(llbq_offset);

			wait_lkb = 
			    (LKB *)((char *)llbq - CL_OFFSETOF(LKB,lkb_llb_wait));
			wait_rsb = (RSB*)LGK_PTR_FROM_OFFSET(wait_lkb->lkb_lkbq.lkbq_rsb);

			llbq_offset = wait_lkb->lkb_llb_wait.llbq_next;

#if defined(conf_CLUSTER_BUILD)
			/* Make sure we have an RSB... */
			if ( inlkdsm && wait_rsb )
			{
			    LK_DLOCK_TRACE("%@ %d LK_local_deadlock: recursing "
				"MaxMode %w depth %d\n\t" 
				"fr LLB %x LKB %x state %w attr %x mode (%w,%w) dlm %x post %d\n\t"
				"\t%s\n\t"
				"\tcstamp %#.4{%d %}\n\t"
                                "to LLB %x LKB %x state %w attr %x mode (%w,%w) dlm %x post %d\n\t"
				"\t%s\n\t"
				"\tcstamp %#.4{%d %}\n",
				__LINE__,
				LOCK_MODE, MaxRequestMode, depth,
				*(u_i4*)&llb->llb_id,
				current_lkb->lkb_id,
				LKB_STATE, current_lkb->lkb_state,
                                current_lkb->lkb_attribute,
                                LOCK_MODE, current_lkb->lkb_grant_mode,
				LOCK_MODE, current_lkb->lkb_request_mode,
				current_lkb->lkb_cx_req.cx_lock_id,
				CXDLM_IS_POSTED(&current_lkb->lkb_cx_req),
				LKkey_to_string(&rsb->rsb_name, keydispbuf),
				LLB_MAX_STAMPS_PER_NODE, &llb->llb_dist_stamp[inlkdsm->o_node-1], 0,
                                *(u_i4*)&next_llb->llb_id, wait_lkb->lkb_id, 
                                LKB_STATE, wait_lkb->lkb_state,
                                wait_lkb->lkb_attribute,
                                LOCK_MODE, wait_lkb->lkb_grant_mode,
				LOCK_MODE, wait_lkb->lkb_request_mode, 
				wait_lkb->lkb_cx_req.cx_lock_id,
				CXDLM_IS_POSTED(&wait_lkb->lkb_cx_req),
				LKkey_to_string(&wait_rsb->rsb_name, keydispbuf2),
				LLB_MAX_STAMPS_PER_NODE, &next_llb->llb_dist_stamp[inlkdsm->o_node-1], 0);
			}
#endif /* CLUSTER_BUILD */

			WaitRequestMode = wait_lkb->lkb_request_mode;

			while ( (dead_lkb = LK_local_deadlock(wait_lkb, top_lkb,
				      &WaitRequestMode, inlkdsm, 
				      lkdsm_context, depth, must_retry))
				|| *must_retry )
			{

			    if ( dead_lkb )
			    {
				/* Dump the top LKB and list if we're back at the top */
				if ( lkd->lkd_status & LKD_SHOW_DEADLOCKS &&
				     current_lkb == top_lkb )
				{
				    show_deadlock(top_lkb, lkd->lkd_stat.deadlock_search);
				}
			    }
			    else if ( *must_retry & LKMR_RECHECK )
			    {
				LK_DLOCK_TRACE("%@ %d LK_local_deadlock: %s %d %d"
					       " rechecking LKB %x DLM %x (%w,%w)\n",
					__LINE__,
					(inlkdsm) ? "DIST" : "LOCL",
					(inlkdsm) 
					  ? lkd->lkd_stat.cnt_gdlck_call 
					  : lkd->lkd_stat.deadlock_search,
					depth,
					wait_lkb->lkb_id,
					wait_lkb->lkb_cx_req.cx_lock_id,
					LOCK_MODE, wait_lkb->lkb_request_mode,
					LOCK_MODE, WaitRequestMode);

				continue;
			    }

			    /* Deadlock, or some other must_retry */

			    /*
			    ** If lock state changed while waiting for RSB mutex,
			    ** break to retry LLB waiters, if any.
			    */
			    if ( *must_retry & LKMR_STATE_CHANGE )
				 break;

			    /* Release the RSB if we mutexed it */
			    if ( (rsb->rsb_dlock_depth & rsb_depth) == rsb_depth )
			    {
				rsb->rsb_dlock_depth &= ~rsb_depth;
				LK_unmutex(&rsb->rsb_mutex);
			    }
			    return(dead_lkb);
			} /* while ( dead_lkb = LK_local_deadlock ... */

			/*
			** If wait_lkb state change while waiting for RSB mutex,
			** restart from LLB waiters.
			*/
			if ( *must_retry & LKMR_STATE_CHANGE )
			{
#if defined(conf_CLUSTER_BUILD)
			    if ( inlkdsm )
			    {
				LK_DLOCK_TRACE("%@ %d LK_local_deadlock: STATE CHANGE "
				    "MaxMode %w depth %d\n\t" 
				    "fr LLB %x LKB %x state %w attr %x mode (%w,%w) dlm %x post %d\n\t"
				    "\t%s\n\t"
				    "\tcstamp %#.4{%d %}\n\t"
				    "to LLB %x LKB %x state %w attr %x mode (%w,%w) dlm %x post %d\n\t"
				    "\t%s\n\t"
				    "\tcstamp %#.4{%d %}\n",
				    __LINE__,
				    LOCK_MODE, MaxRequestMode, depth,
				    *(u_i4*)&llb->llb_id,
				    current_lkb->lkb_id,
				    LKB_STATE, current_lkb->lkb_state,
				    current_lkb->lkb_attribute,
				    LOCK_MODE, current_lkb->lkb_grant_mode,
				    LOCK_MODE, current_lkb->lkb_request_mode,
				    current_lkb->lkb_cx_req.cx_lock_id,
				    CXDLM_IS_POSTED(&current_lkb->lkb_cx_req),
				    LKkey_to_string(&rsb->rsb_name, keydispbuf),
				    LLB_MAX_STAMPS_PER_NODE, &llb->llb_dist_stamp[inlkdsm->o_node-1], 0,
				    *(u_i4*)&next_llb->llb_id, wait_lkb->lkb_id, 
				    LKB_STATE, wait_lkb->lkb_state,
				    wait_lkb->lkb_attribute,
				    LOCK_MODE, wait_lkb->lkb_grant_mode,
				    LOCK_MODE, wait_lkb->lkb_request_mode, 
				    wait_lkb->lkb_cx_req.cx_lock_id,
				    CXDLM_IS_POSTED(&wait_lkb->lkb_cx_req),
				    LKkey_to_string(&wait_rsb->rsb_name, keydispbuf2),
				    LLB_MAX_STAMPS_PER_NODE, &next_llb->llb_dist_stamp[inlkdsm->o_node-1], 0);
			    }
#endif /* CLUSTER_BUILD */
			    *must_retry &= ~LKMR_STATE_CHANGE;
			    llb_waiters = 0;
			    break;
			}

#if defined(conf_CLUSTER_BUILD)
			/*
			** If continuation and next_lkb has been locally
			** deadlock checked by LKdeadlock_thread(),
			** stamp it's LLB as cluster-searched.
			*/
			if ( inlkdsm &&
			     !(next_lkb->lkb_attribute & LKB_ON_DLOCK_LIST) )

			{
			    /* Stamp current_lkb's LLB as cluster-searched */
			    for ( i = 0; i < LLB_MAX_STAMPS_PER_NODE-1; i++ )
			    {
				/* Shuffle extant stamps */
				next_llb->llb_dist_stamp[inlkdsm->o_node-1][i] =
				    next_llb->llb_dist_stamp[inlkdsm->o_node-1][i+1];
			    }

			    /* Drop us in the last slot */
			    next_llb->llb_dist_stamp[inlkdsm->o_node-1][i] =
					inlkdsm->msg_search;
			}
#endif /* CLUSTER_BUILD */
		    } /* while ( llb_waiters == next_llb->llb_waiters ... */
		/* If the LLB's waiters list changes, restart the list */
		} while ( llb_waiters != next_llb->llb_waiters );
	    } /* if (next_llb->llb_waiters && */
	} /* if (((LK_compatible[MaxRequestMode] >> */
    } /* while(end_offset != next_lock_offset) */

    /* End of RSB's granted queue */

# if defined(conf_CLUSTER_BUILD)

    /*
    ** First, look for excuses -not- to check for blockers and
    ** send messages.
    **
    ** Setting lkdsm_context = NULL along the way keeps that
    ** from happening.
    */
    
    /* If top_lkb has been async posted, notify caller, abandon this lkb */
    if ( top_lkb && CXDLM_IS_POSTED(&top_lkb->lkb_cx_req) )
    {
	*must_retry |= LKMR_TOP_POSTED;
	lkdsm_context = NULL;
    }

    /*
    ** Do not check for blockers and send messages if this lock request has
    ** not yet been locally deadlock checked.
    **
    ** Note that these LKBs are not cluster stamped at this time.
    */
    if ( lkdsm_context && current_lkb->lkb_attribute & LKB_ON_DLOCK_LIST )
    {
	if ( inlkdsm )
	{
	    LK_DLOCK_TRACE("%@ %d LK_local_deadlock: NOCFB ON_DLOCK llb %x lkb %x "
			    "depth %d\n\t"
			    "\t%s\n",
			    __LINE__,
			    *(u_i4*)&llb->llb_id,
			    current_lkb->lkb_id,
			    depth,
			    LKkey_to_string(&rsb->rsb_name, keydispbuf));

	    /* Inform LKdist that we need local deadlock checking */
	    lkd->lkd_purge_dlock = TRUE;
	}

	/* Skip messaging on current_lkb */
	lkdsm_context = NULL;
    }

    /* Check continuation cases: */
    if ( lkdsm_context && inlkdsm )
    {
	/* Do not continue continuation msgs from depth == 1 */
	if ( depth == 1 )
	{
	    LK_DLOCK_TRACE("%@ %d LK_local_deadlock: NOCFB depth %d llb %x lkb %x "
				"dlm %x\n\t"
				"%s\n",
			    __LINE__,
			    depth,
			    *(u_i4*)&llb->llb_id,
			    current_lkb->lkb_id,
			    current_lkb->lkb_cx_req.cx_lock_id,
			    LKkey_to_string(&rsb->rsb_name, keydispbuf));
	    lkdsm_context = NULL;
	}

	/* ...or if this top_lkb's resource */
	else if ( top_lkb 
		  && current_lkb->lkb_lkbq.lkbq_rsb == top_lkb->lkb_lkbq.lkbq_rsb )
	{
	    LK_DLOCK_TRACE("%@ %d LK_local_deadlock: NOCFB on top RSB llb %x lkb %x "
				"dlm %x depth %d\n\t"
				"%s\n",
			    __LINE__,
			    *(u_i4*)&llb->llb_id,
			    current_lkb->lkb_id,
			    current_lkb->lkb_cx_req.cx_lock_id,
			    depth,
			    LKkey_to_string(&rsb->rsb_name, keydispbuf));
	    lkdsm_context = NULL;
	}
    }

    /*
    ** Only perform this check if distributed lock, and DLM is not handling
    ** this for us.
    **
    ** Check for remote blockers ahead of current_lkb if:
    **
    **		The current_lkb is not granted in an exclusive mode
    **		(there can be no blockers ahead of current_lkb).
    **
    **		-and-
    **	
    **		The resource is not granted in an exclusive mode
    **		or current_lkb is locally compatible.
    **
    **		-and-
    **
    **		Continuation (inlkdsm != NULL) or
    **		    current_lkb is not async posted
    **		    (the CV/WT is still enqueued)
    */

    if ( lkdsm_context 
         && lkd->lkd_status & LKD_DISTRIBUTE_DEADLOCK
         && current_lkb->lkb_attribute & LKB_DISTRIBUTED_LOCK
         && current_lkb->lkb_cx_req.cx_status != E_CL2C21_CX_E_DLM_DEADLOCK
         && current_lkb->lkb_cx_req.cx_status != E_CL2C22_CX_E_DLM_CANCEL
	 && current_lkb->lkb_grant_mode < LK_SIX
	 && (rsb->rsb_grant_mode < LK_SIX || IsLocallyCompatible)
	 && ( inlkdsm || !(CXDLM_IS_POSTED(&current_lkb->lkb_cx_req)) ) )
    {
	/*
	** Check for distributed deadlock.
	**
	** Look for locks on other nodes which
	** may be blocking the granting of current_lkb.
	**
	** What we really want to know is if those blocking
	** locks are waiting on some other resource, namely
	** "top_lkb", indicating a deadlock. But we can't know
	** that until we send a message to each "blocking"
	** node and let it figure out if the blocking lock is
	** waiting on some resource, so we have to send
	** a LK_DSM message to find out.
	**
	** Complicating this quest is that what appears to be
	** a blocking lock now may not be blocking or may not even
	** exist by the time the remote node receives and processes 
	** the message, and that this local node and/or the remote 
	** node may be lacking any granted locks whatsoever 
	** (that annoying Heisenberg effect).
	*/
	CX_BLKI 	*pblkinfo;
	LK_DSM  	*plkdsm;
	RSB		*top_rsb = (RSB*)NULL;
	LLB		*top_llb = (LLB*)NULL;
	i4	 	cnt, blkcnt;
	i4	 	BlkiX;
	i4	 	ClusterMaxRequestMode;
	LK_UNIQUE	dlm, top_dlm;
	STATUS		status;
	i4		NodesVisited = 0;

	/*
	** There are no blockers of this resource that are
	** part of a deadlock cycle locally. Check for
	** blockers on other nodes.
	*/

	/* Strictly for debugging... */
	CX_EXTRACT_LOCK_ID( &dlm, &current_lkb->lkb_cx_req.cx_lock_id );
	if ( top_lkb )
	{
	    top_rsb = (RSB*)LGK_PTR_FROM_OFFSET(top_lkb->lkb_lkbq.lkbq_rsb);
	    top_llb = (LLB*)LGK_PTR_FROM_OFFSET(top_lkb->lkb_lkbq.lkbq_llb);
	    CX_EXTRACT_LOCK_ID( &top_dlm, &top_lkb->lkb_cx_req.cx_lock_id );
	}

	BlkiX = (inlkdsm) ? DIST_BLKI : LOCL_BLKI;

	/* Seed max request mode */
	ClusterMaxRequestMode = MaxRequestMode;

	status = CXdlm_get_blki( (current_lkb->lkb_state == LKB_CONVERT) ?
	                         CX_F_CVT_CVT : 0, 
				 &current_lkb->lkb_cx_req,
				 BlkiX,
				 Lk_blki_pointer[BlkiX], &Lk_blki_count[BlkiX],
				 &ClusterMaxRequestMode,
				 &pblkinfo, &blkcnt );

	LK_DLOCK_TRACE("%@ %d LK_local_deadlock: %s %d depth %d %w CFB %d (%w,%w)\n\t"
	    "top_llb %x top_lkb %x state %w attr %x mode (%d,%d) dlm %x post %d\n\t"
	    "llb %x lkb %x state %w attr %x mode (%w,%w) dlm %x post %d\n\t"
	    "%s LocallyCompat %d GC %d\n",
	    __LINE__,
	    (inlkdsm) ? "DIST" : "LOCL", 
	    (inlkdsm) ? lkd->lkd_stat.cnt_gdlck_call : lkd->lkd_stat.deadlock_search,
	    depth,
	    LKB_STATE, current_lkb->lkb_state,
	    blkcnt,
	    LOCK_MODE, MaxRequestMode,
	    LOCK_MODE, ClusterMaxRequestMode,
	    (top_llb) ? *(u_i4*)&top_llb->llb_id : 0,
	    (top_lkb) ? top_lkb->lkb_id : 0,
	    LKB_STATE, (top_lkb) ? top_lkb->lkb_state: 0,
	    (top_lkb) ? top_lkb->lkb_attribute : 0,
	    (top_lkb) ? top_lkb->lkb_grant_mode : 0,
	    (top_lkb) ? top_lkb->lkb_request_mode : 0,
	    (top_lkb) ? top_dlm.lk_ulow : 0,
	    (top_lkb) ? CXDLM_IS_POSTED(&top_lkb->lkb_cx_req) : 0,
	    *(u_i4*)&llb->llb_id,
	    current_lkb->lkb_id, 
	    LKB_STATE, current_lkb->lkb_state,
	    current_lkb->lkb_attribute,
	    LOCK_MODE, current_lkb->lkb_grant_mode,
	    LOCK_MODE, current_lkb->lkb_request_mode,
	    dlm.lk_ulow,
	    CXDLM_IS_POSTED(&current_lkb->lkb_cx_req),
	    LKkey_to_string(&rsb->rsb_name, keydispbuf),
	    IsLocallyCompatible,
	    GrantsConverts);

	if ( OK == status && blkcnt > 0 )
	{
	    /* Process info returned by CXdlm_get_blki */
	    if ( pblkinfo != Lk_blki_pointer[BlkiX] )
	    {
		/*
		** Buffer passed to CXdlm_get_blki was too small.
		** Retain new larger buffer for use now on.
		*/
		if ( Lk_blki_pointer[BlkiX] != Lk_default_blki_array[BlkiX] )
		{
		    /* Deallocate too small CX_BLKI array */
		    MEfree( (PTR)Lk_blki_pointer[BlkiX] );
		}
		Lk_blki_pointer[BlkiX] = pblkinfo;
	    }

	    /*
	    ** If current_lkb is locally compatible but is not
	    ** cluster compatible, then there may be intervening
	    ** cluster-incompatible locks that are
	    ** preventing current_lkb from being granted.
	    **
	    **		That node	GR	IX
	    **		That node	GR	IX
	    **		This node	GR	IX
	    **		This node	GR	IX
	    **		That node	WT	S	<----
	    **		Current LKB	WT	IS
	    **
	    **		MaxRequestMode == IX
	    **		IsLocallyCompatible = TRUE
	    **		ClusterMaxRequestMode = SIX
	    **
	    ** If there are grants/converts on the resource,
	    ** recheck current_lkb locally, this time using the
	    ** computed ClusterMaxRequestMode.
	    **
	    ** Note that LK_RECHECK recursive calls add on to
	    ** any already-assembled messages from greater
	    ** depths in the graph.
	    */
	    if ( IsLocallyCompatible 
		 && ClusterMaxRequestMode != MaxRequestMode
		 && rsb->rsb_grant_q_next != LGK_OFFSET_FROM_PTR(&rsb->rsb_grant_q_next) )
	    {
		LK_DLOCK_TRACE("%@ %d LK_local_deadlock: %s depth %d Recheck max %w,%w\n\t"
		    "top_llb %x top_lkb %x state %d attr %x mode (%d,%d) dlm %x\n\t"
		    "%s\n\t"
		    "llb %x lkb %x state %w attr %x mode (%w,%w) dlm %x post %d\n\t"
		    "%s\n",
		    __LINE__,
		    (inlkdsm) ? "DIST" : "LOCL", 
		    depth,
		    LOCK_MODE, MaxRequestMode,
		    LOCK_MODE, ClusterMaxRequestMode,
		    (top_lkb) ? *(u_i4*)&top_llb->llb_id : 0,
		    (top_lkb) ? top_lkb->lkb_id : 0,
		    (top_lkb) ? top_lkb->lkb_state : 0,
		    (top_lkb) ? top_lkb->lkb_attribute : 0,
		    (top_lkb) ? top_lkb->lkb_grant_mode : 0,
		    (top_lkb) ? top_lkb->lkb_request_mode : 0,
		    (top_lkb) ? top_dlm.lk_ulow : 0,
		    (top_rsb) ? LKkey_to_string(&top_rsb->rsb_name, keydispbuf) : "",
		    *(u_i4*)&llb->llb_id,
		    current_lkb->lkb_id, 
		    LKB_STATE, current_lkb->lkb_state,
		    current_lkb->lkb_attribute,
		    LOCK_MODE, current_lkb->lkb_grant_mode,
		    LOCK_MODE, current_lkb->lkb_request_mode,
		    dlm.lk_ulow,
		    CXDLM_IS_POSTED(&current_lkb->lkb_cx_req),
		    LKkey_to_string(&rsb->rsb_name, keydispbuf2));

		*pMaxRequestMode = ClusterMaxRequestMode;
		*must_retry |= LKMR_RECHECK;

		/* Release the RSB if we mutexed it */
		if ( (rsb->rsb_dlock_depth & rsb_depth) == rsb_depth )
		{
		    rsb->rsb_dlock_depth &= ~rsb_depth;
		    LK_unmutex(&rsb->rsb_mutex);
		}

		return((LKB*)NULL);
	    }

	    /* Loop over blockers and queue one message per node */
	    for ( pblkinfo = Lk_blki_pointer[BlkiX], cnt = 0;
	          cnt < blkcnt;
		  cnt++, pblkinfo++ )
	    {
		if ( !(NodesVisited & (1 << pblkinfo->cx_blki_csid))
		    &&
		     !(LK_dup_msg(lkdsm_context, pblkinfo, inlkdsm,
				  rsb, top_lkb, ClusterMaxRequestMode))
		    &&
			( pblkinfo->cx_blki_queue == CX_BLKI_GRANTQ ||
			    (rsb->rsb_grant_mode == LK_N &&
			     inlkdsm &&
			       pblkinfo->cx_blki_lkid.lk_uhigh == 
				inlkdsm->o_lockid.lk_uhigh &&
			       pblkinfo->cx_blki_lkid.lk_ulow == 
				inlkdsm->o_lockid.lk_ulow) ) )
		{
		    /* Note we've visited this node */
		    NodesVisited |= (1 << pblkinfo->cx_blki_csid);

		    /*
		    ** If continuation and this blocking lock is the same as
		    ** the lock that started the search, then we have a loopback
		    ** and we ignore it rather than continuing the insanity.
		    */
		    if ( inlkdsm 
			 && inlkdsm->o_blk_node == pblkinfo->cx_blki_csid
			 && inlkdsm->o_blk_lockid == pblkinfo->cx_blki_lkid.lk_ulow )
		    {
			LK_DLOCK_TRACE("%@ %d LK_local_deadlock: Node %d loopback to node %d "
			    "lockid %x depth %d\n"
			    "\to_llb %x o_lkb %x o_lockid %x o_node %d\n"
			    "\t%s\n"
			    "\tLLB %x LKB %x DLM %x\n"
			    "\t%s\n",
			    __LINE__,
			    rcp_node_id,
			    pblkinfo->cx_blki_csid,
			    pblkinfo->cx_blki_lkid.lk_ulow,
			    depth,
			    inlkdsm->o_llb_id,
			    inlkdsm->o_lkb_id,
			    inlkdsm->o_lockid.lk_ulow,
			    inlkdsm->o_node,
			    LKkey_to_string(&inlkdsm->o_lock_key, keydispbuf),
			    *(u_i4*)&llb->llb_id,
			    current_lkb->lkb_id,
			    dlm.lk_ulow,
			    LKkey_to_string(&rsb->rsb_name, keydispbuf2));
			
			continue;
		    }

		    /* 
		    ** Only send the message if we've not already sent
		    ** one for this resource to this node, and this
		    ** is not a waiter or converter, except for the special
		    ** case where the local grant/convert queue is empty
		    ** and the waiter/converter is the originating lock.
		    */

		    plkdsm = LK_get_free_dsm( lkdsm_context, pblkinfo->cx_blki_csid );
                    if (!plkdsm)
                    {
			TRdisplay( "%@ LK_local_deadlock: plkdsm == NULL! , CSID=%x\n",
				     pblkinfo->cx_blki_csid); TRflush();
			CS_breakpoint() ;
			continue;
                    }

		    plkdsm->lock_key = rsb->rsb_name; 
		    /*
		    ** lockid.lk_uhigh has the blocking lock's DLM lockid 
		    ** lockid.lk_ulow has current_lkb's DLM lockid 
		    */
		    CX_EXTRACT_LOCK_ID( &plkdsm->lockid,
			    &current_lkb->lkb_cx_req.cx_lock_id );
		    plkdsm->lockid.lk_uhigh = pblkinfo->cx_blki_lkid.lk_ulow;
		    plkdsm->lock_cum_mode = ClusterMaxRequestMode;
		    plkdsm->lock_state = current_lkb->lkb_state;

		    /* Identify the node sending this message */
		    plkdsm->msg_node = rcp_node_id;

		    if ( inlkdsm )
		    {
			/* Continuation case */
			plkdsm->o_lock_key = inlkdsm->o_lock_key;
			plkdsm->o_llb_id = inlkdsm->o_llb_id;
			plkdsm->o_node = inlkdsm->o_node;
			plkdsm->o_stamp = inlkdsm->o_stamp;
			plkdsm->o_lockid = inlkdsm->o_lockid;
			plkdsm->o_lkb_id = inlkdsm->o_lkb_id;
			plkdsm->o_blk_node = inlkdsm->o_blk_node;
			plkdsm->o_blk_pid = inlkdsm->o_blk_pid;
			plkdsm->o_blk_sid = inlkdsm->o_blk_sid;
			plkdsm->o_blk_gtmode = inlkdsm->o_blk_gtmode;
			plkdsm->msg_search = inlkdsm->msg_search;
			/* Retain orginating msg_seq (depth) */
			plkdsm->msg_seq = inlkdsm->msg_seq;
			plkdsm->msg_hops = inlkdsm->msg_hops;
			plkdsm->o_blk_lockid = inlkdsm->o_blk_lockid;
		    }
		    else
		    {
			/* Starting distributed search case */
			plkdsm->o_lock_key = top_rsb->rsb_name;
			CX_EXTRACT_LOCK_ID( &plkdsm->o_lockid,
			    &current_lkb->lkb_cx_req.cx_lock_id );
			plkdsm->o_lkb_id = top_lkb->lkb_id;
			plkdsm->o_blk_node = pblkinfo->cx_blki_csid;
			plkdsm->o_blk_pid = (PID)0; /* Don't know yet. */
			plkdsm->o_blk_sid = (CS_SID)0;
			plkdsm->o_blk_gtmode = pblkinfo->cx_blki_rqmode;
			/*
			** Init the msg cluster stamp, consisting of
			** the local deadlock search sequence that 
			** started this distributed search plus the
			** depth at which this message originated
			** plus the node on which this message originated.
			*/
			plkdsm->msg_search = lkd->lkd_stat.deadlock_search;
			plkdsm->msg_seq = depth;
			plkdsm->o_node = rcp_node_id;

			/* Remember the original blocker for loopback checks */
			plkdsm->o_blk_lockid = pblkinfo->cx_blki_lkid.lk_ulow;

			plkdsm->msg_hops = 0;
			plkdsm->o_stamp = top_lkb->lkb_request_id;
			if ( current_lkb == top_lkb )
			{
			    plkdsm->o_llb_id = *(u_i4*)&llb->llb_id;
			}
			else
			{
			    /* use next_llb as scratch to get top_lkb's llb_id */
			    next_llb = (LLB *)LGK_PTR_FROM_OFFSET( \
			     top_lkb->lkb_lkbq.lkbq_llb);
			    plkdsm->o_llb_id = *(u_i4*)&next_llb->llb_id;
			}

			/* Starting a new cycle */
			top_lkb->lkb_attribute &= ~LKB_END_OF_DL_CYCLE;
		    }

		    LK_DLOCK_TRACE("%@ %d LK_local_deadlock: MSG %d,%d,%d from %s depth %d: [%d of %d]\n\t"
			"Blocked in node %d by %d o_node %d o_blk_node %d LocallyCompat %d\n\t"
			"llb %x cur_lkb %x cur_state %w cur_mode (%w,%w) cur_attr %x cur_post %d\n\t"
			"cur_key %s\n\t"
			"cstamp %#.4{%d %}\n\t"

			"o_llb=%x seq %d "
			"o_lkb %x o_request %d o_attr %x\n\t"
			"o_key %s\n\t"
			"cum_mode %w o_dlm %x state %w\n\t"
			"blk_dlm %x blk_q %w blk_mode %w\n\n",

			__LINE__,
			plkdsm->msg_search, plkdsm->o_node, plkdsm->msg_seq,
			(inlkdsm) ? "DIST" : "LOCL", 
			depth,
			cnt + 1, blkcnt,

			rcp_node_id, pblkinfo->cx_blki_csid,
			plkdsm->o_node, plkdsm->o_blk_node,
			IsLocallyCompatible,

			*(u_i4*)&llb->llb_id,
			current_lkb->lkb_id, 
			LKB_STATE, current_lkb->lkb_state,
			LOCK_MODE, current_lkb->lkb_grant_mode,
			LOCK_MODE, current_lkb->lkb_request_mode,
			current_lkb->lkb_attribute,
			CXDLM_IS_POSTED(&current_lkb->lkb_cx_req),

			LKkey_to_string(&rsb->rsb_name, keydispbuf),
			LLB_MAX_STAMPS_PER_NODE, &llb->llb_dist_stamp[plkdsm->o_node-1], 0,

			plkdsm->o_llb_id,
			(inlkdsm) ? lkd->lkd_stat.gdlck_call : lkd->lkd_stat.dlock_wakeups, 

			plkdsm->o_lkb_id,
			plkdsm->o_stamp,
			(top_lkb) ? top_lkb->lkb_attribute : 0,
			(top_rsb) ? LKkey_to_string(&top_rsb->rsb_name, keydispbuf2) : "",

			LOCK_MODE, plkdsm->lock_cum_mode, 
			plkdsm->o_lockid.lk_ulow,
			LKB_STATE, plkdsm->lock_state,

		        pblkinfo->cx_blki_lkid.lk_ulow,
			"GR,CV,WT", pblkinfo->cx_blki_queue,
			LOCK_MODE, pblkinfo->cx_blki_rqmode);
		}
	    }
	}
	else if ( E_CL2C25_CX_E_DLM_NOTHELD == status )
	{
	    /*
	    ** This may possibly happen if process holding the
	    ** lock prematurely exits (abends).  If the lock is
	    ** not held in a group lock container (all platforms
	    ** get some locks in process context, and VMS has no GLC
	    ** so it gets all its locks in process context) it
	    ** is implicitly released at the DLM level, but will
	    ** exist at the LK level until cleanup is performed.
	    **
	    ** Lock can safely be ignored for distributed deadlock
	    ** purposes.
	    */
	    LK_DLOCK_TRACE("%@ %d p%x s%x LKB %x rqstid %d state %w"
		      " Got E_CL2C25_CX_E_DLM_NOTHELD\n",
		    __LINE__,
		    current_lkb->lkb_cpid.pid, 
		    current_lkb->lkb_cpid.sid,
		    current_lkb->lkb_id, 
		    current_lkb->lkb_request_id,
		    LKB_STATE, current_lkb->lkb_state);
	    /* Do nothing */;
	}
	else if ( E_CL2C21_CX_E_DLM_DEADLOCK == status )
	{
	    /*
	    ** Convert - Convert deadlock detected. We lose.
	    ** Note, current_lkb not top_lkb is the victim
	    ** in this case, since the deadlock cycle is entirely
	    ** on the current resource.  Purely local cvt-cvt
	    ** deadlock never get this far as they are immediately
	    ** detected during the convert request.
	    **
	    ** There is no danger of both converters on two nodes
	    ** both being selected as deadlock victims, since
	    ** CXdlm_get_blki will not return as a blocking request
	    ** any conversion requests queued behind the current
	    ** request.
	    **
	    ** Last CXBLKI in returned array has info about
	    ** the other participant in the cvt-cvt deadlock.
	    */
	    /* Ignore if already notified */
	    if ( !(current_lkb->lkb_attribute & LKB_DEADLOCKED) )
	    {
	        LK_DLOCK_TRACE("%@ %d p%x s%x Distributed cvt-cvt deadlock: \n\t"
			"llb %x dead_lkb %x dead_state %w dead_mode (%w,%w) top_lkb %x\n",
		    __LINE__,
		    current_lkb->lkb_cpid.pid, 
		    current_lkb->lkb_cpid.sid,
		    *(u_i4*)&llb->llb_id,
		    current_lkb->lkb_id, 
		    LKB_STATE, current_lkb->lkb_state, 
		    LOCK_MODE, current_lkb->lkb_grant_mode,
		    LOCK_MODE, current_lkb->lkb_request_mode,
		    (top_lkb) ? top_lkb->lkb_id : 0);

	        if ( lkd->lkd_status & LKD_SHOW_DEADLOCKS )
		    show_deadlock(current_lkb, lkd->lkd_stat.deadlock_search);

	        /* Mark the current LKB as deadlocked while RSB mutex is held */
	        current_lkb->lkb_attribute |= LKB_DEADLOCKED;

	        /* 
	        ** Stuff the blocking thread's id in the victim's
	        ** lkb for error reporting.  Note, we don't have
	        ** session info, so this is left as zero.
	        */
	        current_lkb->lkb_blocking_cpid.pid =
	            (pblkinfo + blkcnt - 1)->cx_blki_pid;
	        current_lkb->lkb_blocking_cpid.sid = 0; /* Don't know it */
	        current_lkb->lkb_blocking_mode =
	            (pblkinfo + blkcnt - 1)->cx_blki_gtmode;

	        if ( top_lkb == current_lkb )
	        {
		    /*
		    ** Only return lkb if this is the top level LKB.
		    ** The reason for this is that while we've detected
		    ** a distributed cvt-cvt deadlock for ONE of the
		    ** transactions blocking top_lkb, there may be
		    ** more than one transaction blocking original lock
		    ** and those other transactions also need to be checked
		    ** for deadlock cycles.
		    */

		    /* If RSB was mutexed at this depth, release it */
		    if ( (rsb->rsb_dlock_depth & rsb_depth) == rsb_depth )
		    {
			rsb->rsb_dlock_depth &= ~rsb_depth;
		        LK_unmutex(&rsb->rsb_mutex);
		    }
		    return(top_lkb);
	        }
	        else if ( CXdlm_mark_as_posted( &current_lkb->lkb_cx_req ) )
	        {
		    /* Count another global deadlock */
		    lkd->lkd_stat.gdeadlock++;

		    current_lkb->lkb_cx_req.cx_status = E_CL2C21_CX_E_DLM_DEADLOCK;
		    LKcp_resume(&current_lkb->lkb_cpid);
		}
		else
		    /* Already posted, revoke deadlock */
		    current_lkb->lkb_attribute &= ~LKB_DEADLOCKED;
	    }
	}
	else if ( status != OK ) 
	{
	    /*
	    ** Report but continue.  This is probably not
	    ** benign, and if a missed deadlock causes
	    ** system to hang, we need to know why, but
	    ** crashing the system JIC seems extreme.
	    */
	    uleFormat(NULL, status, (CL_ERR_DESC *)NULL, ULE_LOG,
		    NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 0);
	}
    }

# endif /* CLUSTER_BUILD */

    /* Release the RSB mutex if we acquired it */
    if ( (rsb->rsb_dlock_depth & rsb_depth) == rsb_depth )
    {
	rsb->rsb_dlock_depth &= ~rsb_depth;
	LK_unmutex(&rsb->rsb_mutex);
    }

    /*	Deadlock not found. */

    return ((LKB *)NULL);
}
# if defined(conf_CLUSTER_BUILD)

/*{
** Name: LK_dup_msg - Check for duplicate distributed message.
**
** Description:
**	Returns TRUE if a distributed message has already been 
**	queued for a llb_id + lkb_id + stamp + lock_key + node combination.
**
** Inputs:
**      lkdsm_context		Distributed deadlock context to use.
**	pblkinfo		Info about the blocking lock's node.
**	inlkdsm			not-NULL if continuation message.
**	rsb			The blocked resource.
**	top_lkb			The lock that started it all.
**	max_request_mode	Max request mode computed for current_lkb.
**
** Outputs:
**	none
**
**	Exceptions:
**	    none
**
** Side Effects:
**	none
**
** History:
**	16-Feb-2007 (jonj)
**	    Created.
**	07-Jul-2007 (jonj)
**	    Must also qualify message by lkb_id, stamp.
*/
static bool
LK_dup_msg(
LK_DSM_CONTEXT	*lkdsm_context,
CX_BLKI		*pblkinfo,
LK_DSM		*inlkdsm,
RSB		*rsb,
LKB		*top_lkb,
i4		max_request_mode )
{
    LLB		*llb;
    LK_DSM	*dsm;
    u_i4	llb_id;
    u_i4	lkb_id;
    u_i4	stamp;
    i4		dsm_num;

    /* Resolve originating lock identity */
    if ( inlkdsm )
    {
	llb_id = inlkdsm->o_llb_id;
	lkb_id = inlkdsm->o_lkb_id;
	stamp  = inlkdsm->o_stamp;
    }
    else
    {
	llb = (LLB*)LGK_PTR_FROM_OFFSET(top_lkb->lkb_lkbq.lkbq_llb);
	llb_id = *(u_i4*)&llb->llb_id;
	lkb_id = top_lkb->lkb_id;
	stamp = top_lkb->lkb_request_id;
    }

    dsm = (LK_DSM*)NULL;
	
    /* Loop through messages already prepared on this blocking node */
    do
    {
	LK_get_next_dsm(lkdsm_context, pblkinfo->cx_blki_csid, &dsm, &dsm_num);

	/*
	** Look for a message already sent for this o_llb_id,
	** lkb_id, stamp, lock key combination on this node.
	*/
	if ( dsm && dsm->o_llb_id == llb_id &&
		    dsm->o_lkb_id == lkb_id &&
		    dsm->o_stamp  == stamp &&
		    dsm->lock_key.lk_type == rsb->rsb_name.lk_type &&
		    dsm->lock_key.lk_key1 == rsb->rsb_name.lk_key1 &&
		    dsm->lock_key.lk_key2 == rsb->rsb_name.lk_key2 &&
		    dsm->lock_key.lk_key3 == rsb->rsb_name.lk_key3 &&
		    dsm->lock_key.lk_key4 == rsb->rsb_name.lk_key4 &&
		    dsm->lock_key.lk_key5 == rsb->rsb_name.lk_key5 &&
		    dsm->lock_key.lk_key6 == rsb->rsb_name.lk_key6 )
	{
	    /* Found duplicate, upgrade max_request_mode if greater */
	    if ( max_request_mode > dsm->lock_cum_mode )
		dsm->lock_cum_mode = max_request_mode;
	    return(TRUE);
	}
    } while ( dsm );

return(FALSE);

}
# endif /* CLUSTER_BUILD */

/*{
** Name: show_deadlock - TRdisplay information about deadlock.
**
** Description:
**	TRdisplays information about a lock list involved in a deadlock.
**
**
** Inputs:
**      dead_lkb                     A lock block that is waiting.
**	stamp			     An arbitrary number displayed
**				     to visually tie the lock lists 
**				     together.
**
** Outputs:
**	none
**
**	Exceptions:
**	    none
**
** Side Effects:
**	none
**
** History:
**	13-Dec-1999 (jenjo02)
**	    Created.
**	26-Jan-2004 (jenjo02)
**	    Remove DB_TAB_PARTITION_MASK from lk_key3 for 
**	    PAGE/ROW locks (unless temp table - lk_key2 < 0)
**	04-Apr-2008 (jonj)
**	    Use common LKkey_to_string() formatting function.
*/
static VOID
show_deadlock(LKB *dead_lkb, i4 stamp)
{
    LKD		*lkd = (LKD *)LGK_base.lgk_lkd_ptr;
    LKB		*lkb;
    LLB 	*llb = (LLB *)LGK_PTR_FROM_OFFSET(dead_lkb->lkb_lkbq.lkbq_llb);
    RSB 	*rsb;
    SIZE_TYPE	end_offset;
    SIZE_TYPE	llbq_offset;
    LLBQ	*llbq;
    char	keydispbuf[128];

    end_offset = LGK_OFFSET_FROM_PTR(&llb->llb_llbq.llbq_next);

    /*
    ** Display all the locks on the blocked lock's lock list, visually
    ** flag the blocked lock with "--->".
    */
    TRdisplay("%@ DEADLOCK %d Llb: %x Tran_id: %x%x Session: %d:%x\n", 
	stamp,
	*(u_i4*)&llb->llb_id,
	llb->llb_name[0], llb->llb_name[1],
	llb->llb_cpid.pid, llb->llb_cpid.sid);

    for ( llbq_offset = llb->llb_llbq.llbq_next;
	  llbq_offset != end_offset;
	  llbq_offset = llbq->llbq_next )
    {
	llbq = (LLBQ*)LGK_PTR_FROM_OFFSET(llbq_offset);
	lkb = (LKB*)((char*)llbq - CL_OFFSETOF(LKB,lkb_llbq));
	rsb = (RSB*)LGK_PTR_FROM_OFFSET(lkb->lkb_lkbq.lkbq_rsb);

	if ( lkb == dead_lkb )
	    TRdisplay("%@ --->");
	else 
	    TRdisplay("%@     ");

	TRdisplay("Lkb: %x Rsb: %x Gr: %3w Req: %3w State: %2w ",
	    lkb->lkb_id, rsb->rsb_id,
	    LOCK_MODE, lkb->lkb_grant_mode,
	    LOCK_MODE, lkb->lkb_request_mode,
	    LKB_STATE, lkb->lkb_state);

	/* Show the lock's owner if different that the list's */
	if ( lkb->lkb_cpid.pid != llb->llb_cpid.pid ||
	     lkb->lkb_cpid.sid != llb->llb_cpid.sid )
	{
	    TRdisplay("Session: %d:%x ",
		lkb->lkb_cpid.pid, lkb->lkb_cpid.sid);
	}

	if (lkb->lkb_attribute & LKB_PHYSICAL)
	    TRdisplay("PHYS(%d) ", lkb->lkb_count);
	else
	    TRdisplay("%8* ");

	TRdisplay(" %s\n", LKkey_to_string(&rsb->rsb_name, keydispbuf));
    }

    return;
}

/*{
** Name: LK_try_convert	- Try converting some locks.
**
** Description:
**        This internal routine is called when a lock is removed from 
**        the granted queue and has a grant_mode equal to the 
**        group convert mode, or when the head of the convert 
**        queue is removed by unlock.
**
** Inputs:
**      rsb                             The resource to attempt conversion on.
**      mode                            The group mode to use in the
**					calculations.
**
** Outputs:
**	Returns:
**	    STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    Always store mode in rsb_grant_mode or rsb_convert_mode either
**	    directly or by calling try_wait.
**
** History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable logging and locking system.
**	20-sep-1993 (bryanp) merged 23-mar-1993 (bryanp)
**	    Fix a number of bugs surrounding handling of invalid lock values:
**	    3) When a sys$enq call completes synchronously (SS$_SYNCH is
**		returned), we must check the lock status block for the
**		SS$_VALNOTVALID return code. This means that the lock was
**		synchronously granted/converted but that the value is marked
**		invalid. We must ensure that in this case we mark the
**		rsb_invalid field in the RSB so that LK_VALNOTVALID is returned
**		to the caller.
**	28-mar-1995 (medji01)
**	    64K+ Lock Limit Project
**		- Removed references to id_instance in LK_ID.
**		- Changed error messages to reference id_id instead
**		  of id_instance.
**	06-sep-1995 (duursma)
**	    If we're on a cluster, all locks on the convert queue are
**	    ENQWAITing, so there's nothing more to do here.  Added an error
**	    check to verify that such a lock indeed has LKB_ENQWAIT bit
**	    turned on.
**	15-Dec-1995 (jenjo02)
**	    Caller must ensure that rsb_mutex is held prior to calling
**	    this function. Function returns with the mutex held.
**	15-jan-1996 (duursma) bug #71340
**	    Use compute_group_modes() to compute group grant, convert modes.
**	08-feb-1996 (duursma)
**	    Undid last change (also see file header comment)
**	21-aug-1996 (boama01)
**	    Chged *RSB parm to **RSB, due to requirement by LK_try_wait();
**	    see that fn for details.
**	01-aug-1996 (nanpr01 for ICL keving)
**	    If the RSB has callbacks and this routine causes a new LKB to get 
**	    to the head of the CONVERT queue then check for callbacks to fire.
**	    This routine now returns a status.
**	23-Sep-1998 (jenjo02)
**	    In LK_try_convert(), LK_try_wait(), recheck the LKB after
**	    acquiring the dlock_mutex. Deadlock activity may have
**	    changed the LKB state, in which case we musn't resume
**	    the waiting thread - it may already be resumed! This
**	    appears to be the source of the PHANTOM_RESUME_BUG.
**	26-Oct-2007 (jonj)
**	    Ignore LKBs that are stalled awaiting recovery.
*/
STATUS
LK_try_convert(
RSB                 **rsbptr,
i4                  mode)
{
    RSB 	    *rsb = *rsbptr;
    LKD		    *lkd = (LKD *)LGK_base.lgk_lkd_ptr;
    LKB		    *lkb;
    STATUS	    status = OK;
    SIZE_TYPE       lkb_offset;
    SIZE_TYPE       end_offset;
    i4		    current_mode = mode;
    i4             released_locks = FALSE;
    SIZE_TYPE       rcmpt_offset;
    SIZE_TYPE       rcmpt_end_offset;
    LKB		    *rcmpt_lkb;
    CL_ERR_DESC	    sys_err;
    
    LK_WHERE("LK_try_convert")

    /*  Check for some conversions on the conversion queue. */

    if ( rsb->rsb_converters )
    {
	rsb->rsb_grant_mode = current_mode;
        rsb->rsb_convert_mode = current_mode;

	/*
        ** Now scan forward trying to convert locks. We're looking for locks
	** which are blocked in the local stage of lock conversion. If the
	** lock is not in CONVERT state, or if it's already in the ENQWAIT
	** state, or it's deadlocked, then we continue on to the next lock.
	*/

	end_offset = LGK_OFFSET_FROM_PTR(&rsb->rsb_grant_q_next);
	lkb_offset = rsb->rsb_grant_q_next;

	while (lkb_offset != end_offset)
        {
	    lkb = (LKB *)LGK_PTR_FROM_OFFSET(lkb_offset);
	    lkb_offset = lkb->lkb_q_next; /* next on the grant queue */

            if (lkb->lkb_state != LKB_CONVERT ||
		lkb->lkb_attribute & (LKB_DEADLOCKED | LKB_STALLED))
                continue;

            if (((LK_compatible[current_mode] >>
				    lkb->lkb_request_mode) & 1) == 0)
            {
                /*
		** If converting this lock might cause the grant mode to
		**  change.
		*/

                if (lkb->lkb_grant_mode != current_mode)
                    break;

                /* Recompute grant mode excluding next and try again. */

		current_mode = LK_N;
		rcmpt_end_offset = LGK_OFFSET_FROM_PTR(&rsb->rsb_grant_q_next);
		for (rcmpt_offset = rsb->rsb_grant_q_next;
		    rcmpt_offset != rcmpt_end_offset;
		    rcmpt_offset = rcmpt_lkb->lkb_q_next)
		{
		    rcmpt_lkb = (LKB *)LGK_PTR_FROM_OFFSET(rcmpt_offset);
		    if (rcmpt_lkb != lkb &&
			rcmpt_lkb->lkb_grant_mode > current_mode)
		    {
			current_mode = rcmpt_lkb->lkb_grant_mode;
		    }
		}
                if (((LK_compatible[current_mode] >>
				    lkb->lkb_request_mode) & 1 ) == 0)
                {
                    rsb->rsb_convert_mode = current_mode;
                    break;
                }
            }

	    /*
	    ** The lock convert request is locally compatible. If this is
	    ** not a clustered installation, we can now grant the lock
	    ** request. If this IS a clustered installation, we must now
	    ** check whether the convert request is globally compatible, by
	    ** calling SYS$ENQ to convert the lock request.
	    */

	    if ((lkb->lkb_attribute & LKB_DISTRIBUTED_LOCK) == 0)
	    {
		lkb->lkb_grant_mode = lkb->lkb_request_mode;
		if (lkb->lkb_request_mode > current_mode)
		    current_mode = lkb->lkb_request_mode;
		rsb->rsb_grant_mode = current_mode;
		rsb->rsb_convert_mode = current_mode;
                /*
                ** If this lkb has a callback then increment rsb's cback_count
                */
                if (lkb->lkb_cback.cback_fcn != (LKcallback) NULL)
                    rsb->rsb_cback_count++;

                released_locks = TRUE;

		lkb->lkb_state = LKB_GRANTED;

		/* Count one less resource converter */
		rsb->rsb_converters--;

		LK_resume(lkb);
	    }
	    else
	    {
		/*
		** Locks that are waiting in a queue must have been queued
		** with the VMS lock manager before.  It is an error for 
		** any such lock to not be ENQWAITing.
		TRdisplay("LK_try_convert error: waiting lkb not ENQWAIT!\n");
		*/
	    }
        }
    }

    if ( rsb->rsb_converters == 0 )
    {
        /* Nothing left on the conversion queue, try the wait queue. */
	if ( rsb->rsb_waiters )
	    status = LK_try_wait(rsbptr, current_mode);
	else
	    rsb->rsb_grant_mode = rsb->rsb_convert_mode = current_mode;
    }
    else
    {
        /* If we released any then check for callbacks */
        if (released_locks && rsb->rsb_cback_count)
        {
            /*
            ** Still elements in the wait queue, so
            ** Fire any callbacks that are conflicting
            */
            status = lk_cback_fire(rsb, lkb->lkb_request_mode);
        }
    }

    return (status);
}

/*{
** Name: LK_try_wait - Try granting some waiting locks.
**
** Description:
**      This internal routine is called when the granted and convert 
**      queues are empty, or when unlock removes the last 
**      entry from the convert queue, or when the head of the 
**      wait queue is removed.
**
**	In a non-cluster installation, this routine is never called when both
**	the grant queue and the wait queue for this resource are empty.
**	However, in a cluster installation this situation may arise. For
**	example, if a resource has exactly one waiting lock request, and no
**	(local) granted lock requests, then if that waiting lock request gets
**	cancelled (due to interrupt or timeout), this routine gets called with
**	a totally empty resource. We then detect this situation and free up the
**	resource block here.
**
** Inputs:
**      rsb                             The resource to try granting waiters on.
**      mode                            The group grant mode to use.
**
** Outputs:
**	Returns:
**	    STATUS.
**	Exceptions:
**	    none
**
** Side Effects:
**	    Always stores mode into rsb_grant_mode and rsb_convert_mode.
**
** History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable logging and locking system.
**	21-jun-1993 (bryanp)
**	    Fixed cluster-specific list walking bugs. In the cluster environment
**	    we need to skip over lock requests which are already known to be
**	    locally compatible and are just waiting for DLM granting to occur.
**	20-sep-1993 (bryanp) merged 23-mar-1993 (bryanp)
**	    Fix a number of bugs surrounding handling of invalid lock values:
**	    3) When a sys$enq call completes synchronously (SS$_SYNCH is
**		returned), we must check the lock status block for the
**		SS$_VALNOTVALID return code. This means that the lock was
**		synchronously granted/converted but that the value is marked
**		invalid. We must ensure that in this case we mark the
**		rsb_invalid field in the RSB so that LK_VALNOTVALID is returned
**		to the caller.
**	28-mar-1995 (medji01)
**	    64K+ Lock Limit Project
**		- Removed references to id_instance in LK_ID.
**		- Changed error messages to reference id_id instead
**		  of id_instance.
**	06-sep-1995 (duursma)
**	    If we're on a cluster, all locks on the wait queue are ENQWAITing,
**	    so there's nothing more to do here.  Added an error check to verify
**	    that such a lock indeed has LKB_ENQWAIT bit turned on.
**	15-Dec-1995 (jenjo02)
**	    Caller must ensure that rsb_mutex is held prior to calling
**	    this function. Function returns with the mutex held.
**	15-jan-1996 (duursma) bug #71340
**	    Use compute_group_modes() to compute group grant, convert modes.
**	08-feb-1996 (duursma)
**	    Undid last change (also see file header comment)
**	21-aug-1996 (boama01)
**	    Chged *RSB parm to **RSB so it can be nulled; when Clustered RSB
**	    ends up unused, we deallocate it, but that unmutexes it too; must
**	    let callers know this so they do not attempt unmutex as well.
**	01-aug-1996 (nanpr01 for ICL keving)
**	    If the RSB has callbacks and this routine causes a new LKB to get 
**	    to the head of the WAITING queue then check for callbacks to fire.
**	    This routine now returns a status.
**	01-aug-1996 (nanpr01 for jenjo02)
**	    Initialize status=OK.
**	23-Sep-1998 (jenjo02)
**	    In LK_try_convert(), LK_try_wait(), recheck the LKB after
**	    acquiring the dlock_mutex. Deadlock activity may have
**	    changed the LKB state, in which case we musn't resume
**	    the waiting thread - it may already be resumed! This
**	    appears to be the source of the PHANTOM_RESUME_BUG.
**	05-jan-2005 (devjo01)
**	    Change test for determining whether a lock is distributed.
**	22-Apr-2005 (jenjo02)
**	    Don't presume that any LKBs will be on the wait list.
**	26-Oct-2007 (jonj)
**	    Ignore LKBs that are stalled awaiting recovery.
*/
STATUS
LK_try_wait(
RSB                 **rsbptr,
i4                  mode)
{
    RSB 	    *rsb = *rsbptr;
    LKD		    *lkd = (LKD *)LGK_base.lgk_lkd_ptr;
    LKB		    *lkb;
    LKB		    *next_lkb;
    LKB		    *prev_lkb;
    RSB		    *next_rsb;
    RSB		    *prev_rsb;
    RSH		    *rsb_hash_bucket;
    STATUS	    status = OK;
    SIZE_TYPE       lkb_offset;
    SIZE_TYPE       end_offset;
    i4              released_locks = FALSE;
    CL_ERR_DESC	    sys_err;

    LK_WHERE("LK_try_wait")

    /*
    ** keep removing entries as long as locks are granted. In the cluster,
    ** skip over any entry which is already in ENQ wait state. We exit the
    ** loop when we've touched each waiting lkb or when we find the first
    ** incompatible LKB.
    */

    end_offset = LGK_OFFSET_FROM_PTR(&rsb->rsb_wait_q_next);
    lkb_offset = rsb->rsb_wait_q_next;
    lkb = (LKB*)NULL;

    while (lkb_offset != end_offset)
    {
	lkb = (LKB *)LGK_PTR_FROM_OFFSET(lkb_offset);
	lkb_offset = lkb->lkb_q_next;	/* next lock on wait queue */

        if (((LK_compatible[mode] >> lkb->lkb_request_mode) & 1) == 0)
	    break;

	/* Don't grant deadlocked or already enqueued locks */
	if (lkb->lkb_attribute & (LKB_DEADLOCKED | LKB_ENQWAIT | LKB_STALLED))
	    continue;

	/*
	** This lock appears to be locally compatible. If this is not a cluster
	** installation, we can now simply grant this lock. If this is a
	** cluster installation, then the next step is to enqueue this lock
	** request with the VMS lock manager.
	*/
	if ((lkb->lkb_attribute & LKB_DISTRIBUTED_LOCK) == 0)
	{
	    /*
	    ** Transfer from the wait queue to the head of the 
	    ** grant queue.
	    */

	    next_lkb = (LKB *)LGK_PTR_FROM_OFFSET(lkb->lkb_q_next);
	    prev_lkb = (LKB *)LGK_PTR_FROM_OFFSET(lkb->lkb_q_prev);
	    next_lkb->lkb_q_prev = lkb->lkb_q_prev;
	    prev_lkb->lkb_q_next = lkb->lkb_q_next;

	    lkb->lkb_q_next = rsb->rsb_grant_q_next;
	    lkb->lkb_q_prev = LGK_OFFSET_FROM_PTR(&rsb->rsb_grant_q_next);
	    next_lkb = (LKB *)LGK_PTR_FROM_OFFSET(rsb->rsb_grant_q_next);
	    next_lkb->lkb_q_prev =
		rsb->rsb_grant_q_next = LGK_OFFSET_FROM_PTR(lkb);

	    /* Count one less resource waiter */
	    rsb->rsb_waiters--;

	    if (lkb->lkb_request_mode > mode)
		mode = lkb->lkb_request_mode;
	    lkb->lkb_state = LKB_GRANTED;
	    lkb->lkb_grant_mode = lkb->lkb_request_mode;

	    LK_resume(lkb);

            /* 
            ** (ICL keving)
            ** If this lkb has a callback then increment rsb's cback_count.
            */
            if (lkb->lkb_cback.cback_fcn != (LKcallback) NULL)
                rsb->rsb_cback_count++;

            released_locks = TRUE;
	}
	else
	{
	    /*
	    ** Must be ENQWAITing.  All locks on convert or wait queues
	    ** were queued by the VMS lock manager.
	    */
	    TRdisplay("LK_try_wait error: convert lkb %x not ENQWAIT!\n", lkb);
	}
    }

    rsb->rsb_grant_mode = rsb->rsb_convert_mode = mode;

    /*
    ** (ICL keving)
    ** If there is anything remaining on the wait queue see if we can
    ** fire any callbacks on the granted queue.
    */
    if ( released_locks && rsb->rsb_cback_count && rsb->rsb_waiters )
    {
        /*
        ** Still elements in the wait queue, so
        ** Fire any callbacks that are conflicting
        */
        status = lk_cback_fire(rsb, lkb->lkb_request_mode);
    }

    /*
    ** In a cluster installation, we just might happen to have been called
    ** with a totally empty resource block. If this happens, then we now
    ** clean up the resource block.
    */
    if ((!status) && lkb && (lkb->lkb_attribute & LKB_DISTRIBUTED_LOCK))
    {
	/* Check to see if the grant/conversion/wait queue is empty. */

        if (rsb->rsb_grant_q_next ==
				LGK_OFFSET_FROM_PTR(&rsb->rsb_grant_q_next))
        {
	    /* Check to see if wait queue is also empty. */

	    if ( rsb->rsb_waiters == 0 )
	    {
		/* Take RSB off the RSB hash queue. */
		rsb_hash_bucket = (RSH *)LGK_PTR_FROM_OFFSET(rsb->rsb_rsh_offset);
		if ( status = LK_mutex(SEM_EXCL, &rsb_hash_bucket->rsh_mutex) )
		    return(status);

		if (rsb->rsb_q_next)
		{
		    next_rsb = (RSB *)LGK_PTR_FROM_OFFSET(rsb->rsb_q_next);
		    next_rsb->rsb_q_prev = rsb->rsb_q_prev;
		}
		prev_rsb = (RSB *)LGK_PTR_FROM_OFFSET(rsb->rsb_q_prev);
		prev_rsb->rsb_q_next = rsb->rsb_q_next;
		(VOID) LK_unmutex(&rsb_hash_bucket->rsh_mutex);

		LK_deallocate_cb( RSB_TYPE, (PTR)rsb, (LLB*)NULL );
		/*
		** This is a tad dangerous! The callers of LK_try_wait()
		** likely aren't prepared for the RSB which they handed
		** to this function to be obliterated. Pontential
		** memory leaks...
		** (boama01, 21-aug-1996): You bet it's dangerous, but not in
		** the way you thought!  Since LK_deallocate_cb() also
		** unmutexes the RSB, we must "null" the RSB ptr to let the
		** caller know not to do it himself!
		*/
		*rsbptr = (RSB *)NULL;
	    }
	}
    }

    return(status);
}

/*
** Name: LK_get_completion_status	- lock request completion handling
**
** Description:
**	During the time that a lock request is waiting, the lock requestor is
**	asleep. When the request completes, the requestor is awoken and must
**	ascertain the results of the lock request. Those results are held in
**	the appropriate LKB structure. 
**
**	This routine locates the LKB structure and fills in the 
**	completion status, and lock value from the fields in that LKB.
**
** Inputs:
**	completion_handle		    - handle which specifies the
**					      appropriate LKB structure.
**
** Outputs:
**	status				    - the lock completion status (this
**					      is different from the CSsuspend
**					      status return, of course).
**	value				    - the value associated with this
**					      lock.
**
** Returns:
**	VOID
**
** History:
**	16-feb-1993 (bryanp)
**	    Added history section, fixed cluster bugs. In particular, made
**	    this routine acquire the LK_mutex, since it is illegal to go
**	    traversing an LLB's LLBQ queue of LKB's without mutex protection.
**	24-may-1993 (bryanp)
**	    Fix LK_get_completion_status() problems with shared lock lists:
**		- If no matching completion handle found, complain, unless
**		    llb_async_status is set to LK_NOLOCKS, in which case the
**		    reason we didn't find an LKREQ is because we're out of
**		    shared memory to allocate one, in which case LK_NOLOCKS is
**		    the correct status to return.
**		- If this lock list is a handle to a shared lock list, look
**		    for the LKREQ in the shared lock list's locks.
**	    Remove most uses of llb_async_status field; use lkreq.status
**		instead (but see previous comment for special LK_NOLOCKS case).
**	    Added mutex_held argument so that I could call this routine while
**		already holding the mutex.
**	21-jun-1993 (bryanp)
**	    It's not common that there is no completion LKREQ, but it's not
**		necessarily an error. LK_DEADLOCK, like LK_NOLOCKS, is
**		transmitted back without an LKREQ, as is LK_BUSY. Furthermore,
**		cancellation completions don't use LKREQ's. So if there's no
**		completion LKREQ, we just use the llb_async_status.
**	15-dec-1994 (medji01
**	    Mutex Granularity Project
**		- Removed mutex_held parameter.
**	28-mar-1995 (medji01)
**	    64K+ Lock Limit Project
**		- Removed references to id_instance in LK_ID.
**		- Changed error messages to reference id_id instead
**		  of id_instance.
**		- Added input_lkb parameter to elminate the lengthy
**		  LLB-LKB chain traversal when possible.
**	04-Jan-1996 (jenjo02)
**	    Search for LKB in prior direction from LLB; LKBs are
**	    added to the end of the chain, so this path should
**	    be shorter.
**	04-Oct-1996 (jenjo02)
**	    Embeded request's LKB pointer in LK_COMP_HANDLE. Lock
**	    LLB if we must search its LKB chain.
**	19-Nov-1996 (jenjo02)
**	    Optimistic Deadlock Detection.
**	    Added test for LLB_DEADLOCK. If a deadlock occurred, the LKB
**	    may have been removed and its status obviously of no used.
**      05-Nov-1998 (hanal04)
**          Added sid and pid to completion handle to prevent possible
**          attempts to use the incorrect lkb->lkreq when obtaining the
**          completion status. Once we have collected an LKREQ mark it
**          as collected to prevent reuse. b90388. Cross from 1.2/01
**	10-Aug-1999 (jenjo02)
**	    Added local_lkreq into which LKB's lkreq is copied to ward off
**	    references to a now-freed lkb_lkreq.
**	28-Feb-2002 (jenjo02)
**	    If we should still be waiting (Phantom Resume), cancel the lock
**	    request, return LK_PHANTOM for LKrequest() to retry the lock.
**	18-Jun-2002 (jenjo02)
**	    Pass proc_llb's, not llb's, id to LK_cancel() for SHARED
**	    lists (BUG 108071, STAR 11971463).
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Don't overwrite settled on blocking info
**	    from LKB, unless unknown.
*/
static VOID
LK_get_completion_status(
    LK_COMP_HANDLE  *completion,
    STATUS	    *status,
    LK_VALUE	    *value,
    CL_ERR_DESC	    *sys_err)
{
    LKD		    *lkd = (LKD *)LGK_base.lgk_lkd_ptr;
    LLB		    *proc_llb;
    LLB		    *llb;
    LKB		    *lkb;
    bool	    found_lkb = FALSE;
    SIZE_TYPE       end_offset;
    SIZE_TYPE       llbq_offset;
    LLBQ	    *llbq;
    i4		    err_code;
    CL_ERR_DESC     local_sys_err;
    /* b90388 */
    i4              lkbq_retries;
    char            *lkb_retry_env;
    i4              retry;
    static i4       G_lkb_retries = 1;
    STATUS          lkb_status;
    STATUS          mutex_status;
    STATUS	    local_status;

    
    LK_WHERE("LK_get_completion_status")

    /* b90388 - Set the retry count for the LKREQ scan loop */
    if(G_lkb_retries == 1)
    {
       NMgtAt("II_LKB_RETRY", &lkb_retry_env);
       if(lkb_retry_env && *lkb_retry_env)
       {
          lkb_status = CVan(lkb_retry_env, &retry);
          if(!(lkb_status))
          {
             G_lkb_retries = retry;
          }
          else
          {
             G_lkb_retries = 3;
          }
       }
       else
       {
          G_lkb_retries = 3;
       }
    }

    /*
    ** Validate the LLB
    */

    /* proc_llb is our LLB of request, the one that suspended */
    proc_llb = (LLB *)LGK_PTR_FROM_OFFSET(completion->proc_llb_offset);
    /* llb is the one holding the locks */
    llb = proc_llb;

    if (llb->llb_status & LLB_PARENT_SHARED)
    {
	if (llb->llb_shared_llb == 0)
	{
	    uleFormat(NULL, E_CL1055_LK_REQUEST_BADPARAM, (CL_ERR_DESC *)NULL, ULE_LOG,
			NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 1,
			0, llb->llb_id.id_id);
	    *status = LK_BADPARAM;
	    return;
	}

	llb = (LLB *)LGK_PTR_FROM_OFFSET(llb->llb_shared_llb);

	if ((llb->llb_type != LLB_TYPE) ||
	    ((llb->llb_status & LLB_SHARED) == 0))
	{
	    uleFormat(NULL, E_CL1055_LK_REQUEST_BADPARAM, (CL_ERR_DESC *)NULL, ULE_LOG,
			NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 3,
			0, llb->llb_id.id_id, 0, llb->llb_type, 
			0, llb->llb_status);
	    *status = LK_BADPARAM;
	    return;
	}
    }

    /*
    ** Locate the LKB we need to extract information from
    */
    if (completion->lkb_offset)
    {
	found_lkb = TRUE;
	lkb = (LKB *)LGK_PTR_FROM_OFFSET(completion->lkb_offset);
    }
    else
    {
       /* b90388 - added retry loop. G_lkb_retries for tuning.  */
       /* This loop is a temporary fix required until the LKREQ */
       /* delivery is synchronised with the following scan loop */
       for(lkbq_retries = 0; lkbq_retries < G_lkb_retries; lkbq_retries++)
       {
	  end_offset = LGK_OFFSET_FROM_PTR(&llb->llb_llbq.llbq_next);

	  /* Lock the LLB while we scan the LLB->LKB queue */
	  if (LK_mutex(SEM_EXCL, &llb->llb_mutex))
	     TRdisplay("Panic: LLB mutex in LK_get_completion_status!\n"); 

	  for (llbq_offset = llb->llb_llbq.llbq_prev;
	     llbq_offset != end_offset;
	     llbq_offset = llbq->llbq_prev)
	  {
	     llbq = (LLBQ *)LGK_PTR_FROM_OFFSET(llbq_offset);
	     lkb = (LKB *)((char *)llbq - CL_OFFSETOF(LKB,lkb_llbq));

	     /* b90388 -Tighten check to prevent use of wrong lkb. */
	     /* If the lock request is cancelled a separate LKREQ is used */
	     /* in LK_cancel. Dissabling this LKREQ if rqst granted before*/
	     /* we could cancel will not therefore be a problem.          */
	     if ((lkb->lkb_request_id == completion->request_id) &&
		((lkb->lkb_attribute & LKB_COLLECTED) == 0)       &&
		(lkb->lkb_cpid.sid == completion->cpid.sid) &&
		(lkb->lkb_cpid.pid == completion->cpid.pid))
	     {
		found_lkb = TRUE;
		lkb->lkb_attribute |= LKB_COLLECTED;
		completion->lkb_offset = 
			LGK_OFFSET_FROM_PTR(lkb);
		break;
	     }
	  }
	  (VOID)LK_unmutex(&llb->llb_mutex);
	  /* b90388 - Break out of retry loop if LKB found */
	  if(found_lkb == TRUE)
	     break;
       } /* end retry loop*/

    }

    /*
    ** Extract the status information if we located the correct LKB
    */
    if (found_lkb)
    {
	/*
	** If this lock request deadlocked, cancel the lock/conversion,
	** return deadlocked status to client.
	**
	** If we should be waiting, Phantom Resume has struck. Cancel
	** the lock/conversion, return LK_PHANTOM to silently retry 
	** the LKrequest.
	*/
	if ( lkb->lkb_attribute & (LKB_ON_LLB_WAIT |
				   LKB_ON_DLOCK_LIST |
				   LKB_DEADLOCKED) )
	{
	    LKB		local_lkb;

	    /* Preserve LKB in local structure */
	    local_lkb = *lkb;
	    lkb = &local_lkb;
	    
	    /* Pass PARENT_SHARED (proc_llb's) list id */
	    lkb->lkb_status = LK_cancel(*(LK_LLID*)&proc_llb->llb_id,
					&completion->lkb_offset,
					&local_sys_err);
	    /* One must assume that the LKB has now been freed! */

	    if ( lkb->lkb_status == LK_CANCEL )
		/* The request was completed, all is well */
		lkb->lkb_status = OK;
	    else if ( lkb->lkb_status == OK )
	    {
		if ( lkb->lkb_attribute & LKB_DEADLOCKED )
		    lkb->lkb_status = LK_DEADLOCK;
		else
		{
		    /* We should be waiting, not running */
		    uleFormat(NULL, E_CL1058_LK_REQUEST_RETRIED, (CL_ERR_DESC *)NULL, ULE_LOG,
			NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 8,
			0, *(u_i4*)&llb->llb_id, 
			0, lkb->lkb_id,
			0, lkb->lkb_lkbq.lkbq_rsb,
			0, lkb->lkb_request_mode,
			0, lkb->lkb_cpid.pid,
			0, lkb->lkb_cpid.sid,
			0, lkb->lkb_state,
			0, lkb->lkb_attribute);
		    lkb->lkb_status = LK_PHANTOM;
		}
	    }
	}

	*status = lkb->lkb_status;

	/* Update blocking lock info, if unknown */
	if ( !completion->blocking_cpid.pid )
	{
	    completion->blocking_cpid = lkb->lkb_blocking_cpid;
	    completion->blocking_mode = lkb->lkb_blocking_mode;
	}
	
	/* Retrieve the lock value at time lock granted */
	if ( value )
	    *value = lkb->lkb_value;
    }
    else
    {
	*status = llb->llb_async_status;

	switch (llb->llb_async_status)
	{
	    case LK_NOLOCKS:
	    case LK_DEADLOCK:
	    case LK_BUSY:
		break;

	    default:
		TRdisplay("%@ LK_completion: no LKREQ found!\n");
		break;
	}
    }

#ifdef CSP_DEBUG
    if (csp_debug)
    TRdisplay("%@ LK_completion: end status is %x\n", *status);
#endif 
    return;
}


/*
** Name: dequeue_lock
**
** Description:
**	Invoke the distributed lock manager to dequeue the lock.
**	If by chance the lock had already been granted then invoke
**	the lock manager again to make sure the lock is cancelled.
**	If this lock is the only lock for the resource then free
**	the resource, and take it off the hash queue.
**
** Inputs:
**	RSB	the resource block
**	LKB	the lock block
**
** Returns:
**	VOID
**
** History:
**	11-sep-1995 (duursma) created to reduce code duplication
**	21-aug-1996 (boama01)
**	    If deallocating RSB, let caller know by nulling RSB ptr in LKB,
**	    so he doesn't go and try unmutexing it.
**	11-sep-96 (nick)
**	   Resource counter is decremented by call to deallocate the RSB
**	   so don't do it here as well.
*/

static STATUS
dequeue_lock( RSB **rsbp, LKB *lkb )
{
    LKD		*lkd = (LKD *)LGK_base.lgk_lkd_ptr;
    int		status;
    RSB		*rsb, *next_rsb;
    RSB		*prev_rsb;

    lkd->lkd_stat.deq++;

    rsb = *rsbp;

    status = CXdlm_cancel( CX_F_PROXY_OK, &lkb->lkb_cx_req );

    if ( status != E_CL2C09_CX_W_CANT_CAN )
    {
	lkb->lkb_attribute |= LKB_CANCEL;

	/*
	** This may be the only lock for this resource. If so,
	** we can go ahead and free the resource.
	**
	** Check to see if the grant/conversion/wait queue is empty.
	*/
	if (rsb->rsb_grant_q_next ==
	    LGK_OFFSET_FROM_PTR(&rsb->rsb_grant_q_next))
	{
	    /* Check to see if wait queue is also empty. */

	    if (rsb->rsb_wait_q_next ==
		LGK_OFFSET_FROM_PTR(&rsb->rsb_wait_q_next))
	    {
		/* Take RSB off the RSB hash queue. */

		if (rsb->rsb_q_next)
		{
		    next_rsb =
			(RSB *)LGK_PTR_FROM_OFFSET(rsb->rsb_q_next);
		    next_rsb->rsb_q_prev = rsb->rsb_q_prev;
		}
		prev_rsb =
		    (RSB *)LGK_PTR_FROM_OFFSET(rsb->rsb_q_prev);
		prev_rsb->rsb_q_next = rsb->rsb_q_next;

		LK_deallocate_cb( rsb->rsb_type, (PTR)rsb, (LLB*)NULL );

		/* Be sure to let caller know the RSB is gone. */
		*rsbp = (RSB*)NULL;
	    }
	}
	if ( E_CL2C25_CX_E_DLM_NOTHELD == status )
	{
	    /* Lock ID no longer valid, but we don't care. */
	    status = OK;
	}
    }
    else
    {
	/* Async request granted before it could be canceled. */
	status = LK_CANCEL;
    }
    return status;
}

/*
** Name: dequeue_convert - helper routine to dequeue conversion requests
**
** Description:
**	Invoke the VMS distributed lock manager to dequeue a previously
**	enqueued conversion request, and do the necessary bookkeeping.
**
** Inputs:
**	lkb	      	lock block
**
** Outputs:
**	lock_granted	set when lock was granted by the time we tried
**			to dequeue it
**
** Returns:
**	status of sys$deq call
**
** History:
**	11-sep-1995 (duursma) created to reduce code duplication
*/
static STATUS
dequeue_convert ( LKB *lkb, bool *lock_granted )
{
    LKD		*lkd = (LKD *)LGK_base.lgk_lkd_ptr;
    STATUS      status;

    /* Cancel the enq request. */

    lkd->lkd_stat.deq++;

    status = CXdlm_cancel( CX_F_PROXY_OK, &lkb->lkb_cx_req );

    if ( status != E_CL2C09_CX_W_CANT_CAN )
    {
#ifdef CSP_DEBUG
	if (csp_debug)
	{
	    LK_UNIQUE	lock_id;

	    CX_EXTRACT_LOCK_ID( &lock_id, &lkb->lkb_cx_req.cx_lock_id );
	    TRdisplay("%@ DLM Lock convert %x.%x cancelled\n;",
	      lock_id.lk_uhigh, lock_id.lk_ulow );
	    TRdisplay("  STORED lock value: %4.4{ %x%}\n",
	      lkb->lkb_cx_req.cx_value);
	}
#endif
	if ( E_CL2C25_CX_E_DLM_NOTHELD == status )
	{
	    /*
	    ** Lock ID no longer valid, assume this is not a problem.
	    ** If this is a problem we'll find out on the next operation
	    ** on this LK.
	    */
	    status = OK;
	}

	lkb->lkb_attribute |= LKB_CONCANCEL;
    }
    else
    {
	/* If the conversion request has already been granted, great. */
	*lock_granted = TRUE;
	status = LK_CANCEL;
    }
    /* Either way call CXdlm_wait to assure any async resume is consumed. */
    CXdlm_wait( 0, &lkb->lkb_cx_req, 0 );
    return (status);
}


/*
** Name: lk_handle_ivb	    - 
**
** Description:
**
**	Handle the case where process has received grant of a 
**	distributed lock, but finds the lock value invalid.
**	We NEVER intentionally invalidate a DLM value block.
**	(Ingres "invalid" data status, is maintained in the
**	DLM value block like any other data.)  Therefore if an 
**	invalid	value block IS seen, it indicates that a lock
**	client has failed while holding a distributed lock in
**	an update mode, and we cannot safely grant distributed
**	locks, until master CSP/RCP process has had a chance to 
**	rollback transactions for the failed process.
**
**	IF the caller is the master CSP/RCP, then we process
**	as usual.  IF not, caller will need to do the following:
**
**	 1) Decide if other processes should be informed of failure.
**	    Failure message should be broadcast IFF:
**	     a) Message has not already been broadcast.
**	     b) Locks are held in a process scope.  If DLM locks
**		associated with Ingres locks are held in a node 
**		context, then lock values will not be invalidated 
**		unless an entire node fails, in which case, master 
**		CSP will be informed of failure through the deadman 
**		lock on the CSP for the failed node.  Non-CSP/RCP
**		process failures in this case will NOT invalidate
**		lock value block, and will NOT prematurely release
**		the distributed lock held, and therefore can be
**		handled by the local RCP in the same way as is done
**		in the non-cluster case.
**
**	 2) Undo last DLM operation, without modifying lock value
**	    block.
**
**	 3) Stall this lock request.
**
**	 4) When resumed, redo request.
**
**
** Inputs:
**	llb		- Lock list
**	lkb		- Request which detected IVB.
**
**      syncgrant	- If true, IVB was detected during synchronous
**			  processing of request.  Otherwise IVB was
**			  detected during lock completion.
**
**	waitflags	- Flags to pass to CXdlm_wait if needed.
**
**	timeout		- timeout to specify if redoing completion,
**			  else ignored.
**
** Outputs:
**	None.
**
** Returns:
**	None.
**                                     
** Side effects:
**	Session is stalled until lock request is resumed.
**
** History:
**	23-apr-2001 (devjo01)
**	    Created.
**	01-Mar-2007 (jonj)
**	    Oops, init status = OK.
**	    Don't stall LK_NOSTALL requests.
**	20-Mar-2007 (jonj)
**	    If syncgrant, both llb_mutex nor rsb_mutex
**	    are held; otherwise neither are.
**	    Release those mutexes before doing anything that
**	    might result in a CSsuspend.
**	    If down-converting would cause another lock to be
**	    granted, don't - cancel the lock request instead.
**	15-Nov-2007 (jonj)
**	    Don't restall if already stalled.
*/
static STATUS
lk_handle_ivb( LLB *llb, LKB *lkb, i4 flags, bool syncgrant, i4 waitflags, i4 timeout, i4 lineno )
{
    STATUS	 status = OK;
    LKD		*lkd = (LKD *)LGK_base.lgk_lkd_ptr;
    RSB		*rsb;
    CSP_CX_MSG	 ivbmsg;
    CL_ERR_DESC  sys_err;
    char	 keydispbuf[256];

    /*
    ** 0) If this is the CSP process, or installation is
    **    running down, then ignore IVB status, and return OK or 
    **    E_CL2C01_CX_I_OKSYNC as indicated by syncgrant.
    */
    if ( (lkd->lkd_status & LKD_CSP_RUNDOWN) ||
	 LGK_my_pid == lkd->lkd_csp_id )
    {
	return syncgrant ? E_CL2C01_CX_I_OKSYNC : OK;
    }

    rsb = (RSB*)LGK_PTR_FROM_OFFSET(lkb->lkb_lkbq.lkbq_rsb);
	
    TRdisplay("%@ lk_handle_ivb %d: (entry) LKB %x LLB %x state %2w Gr %w Rq %w\n"
		    "\tflags %x KEY %s\n",
	    lineno,
	    lkb->lkb_id,
	    *(u_i4*)&llb->llb_id,
	    LKB_STATE, lkb->lkb_state,
	    LOCK_MODE, lkb->lkb_grant_mode,
	    LOCK_MODE, lkb->lkb_request_mode,
	    flags,
	    LKkey_to_string(&rsb->rsb_name, keydispbuf));

    /*
    ** 1) Decide if we need to inform other processes of failure,
    **    but not if we're already stalled.
    */
    if ( lkd->lkd_lock_stall == 0 && OK == CXalter( CX_A_IVB_SEEN, (PTR)1 ) )
    {
	/*
	** Yes. Master CSP is not guaranteed to immediately detect
	** a node failure via the grant of the DMN lock held, and
	** for implementations in which an invalid lock block is
	** possible under circumstances other than node failure,
	** the CSP process will never directly detect an IVB at
	** all.   Therefore a process that does detect this must
	** stall locks and inform the CSP, so it can determine if
	** IVB is the result of a node failure or not, and in any case
	** revalidate the lock blocks and resume the stalled locks.
	** 
	** If other processes/sessions had already set this on this
	** node, CXalter will return E_CL2C05_CX_I_DONE.  This will
	** prevent a flood of redundant CSP_MSG_5_RPTIVB messages.
	*/

	/* Stall locks */
	(void)LKalter( LK_A_STALL, 0, &sys_err );

	/* Note that locks are now stalled */
	TRdisplay("%@ lk_handle_ivb %d: Lock requests are stalled\n",
			lineno);

	/* Tell the world. */
	ivbmsg.csp_msg_action = CSP_MSG_5_RPTIVB;
	ivbmsg.csp_msg_node = lkd->lkd_csid;
	status = CXmsg_send( CSP_CHANNEL, NULL, NULL, (PTR)&ivbmsg );
	if ( OK != status )
	    return status;
    }

    /* If mutexes are held, release them */
    if ( syncgrant )
    {
	(VOID)LK_unmutex(&rsb->rsb_mutex);
	(VOID)LK_unmutex(&llb->llb_mutex);
    }

    /*
    ** 2) Undo previous DLM lock operation if a new request, or
    **    a conversion to a higher mode.
    */
    if ( lkb->lkb_grant_mode < lkb->lkb_request_mode )
    {
	/*
	** We can't revert to previous mode if doing so will cause other locks
	** behind us to be granted. Doing so rearranges the DLM grant queue
	** such that it is no longer in sync with the LK grant queue and
	** may induce an undetectable deadlock.
	*/
	if ( (lkb->lkb_state == LKB_CONVERT && 
		rsb->rsb_converters > 1 || rsb->rsb_waiters)
	     || (lkb->lkb_state == LKB_WAITING && rsb->rsb_waiters > 1) )
	{
	    TRdisplay("%@ lk_handle_ivb %d: (ERR ) status %x LKB %x LLB %x state %2w Gr %w Rq %w\n"
			"\tconverts:%d waiters:%d KEY %s\n",
		    lineno,
		    status,
		    lkb->lkb_id,
		    *(u_i4*)&llb->llb_id,
		    LKB_STATE, lkb->lkb_state,
		    LOCK_MODE, lkb->lkb_grant_mode,
		    LOCK_MODE, lkb->lkb_request_mode,
		    rsb->rsb_converters, rsb->rsb_waiters,
		    LKkey_to_string(&rsb->rsb_name, keydispbuf));

	    /* The DLM lock has been granted, so must be released */
	    (VOID)CXdlm_release( CX_F_PROXY_OK, &lkb->lkb_cx_req );

	    /* This will cancel/redo the lock request from the top */
	    status = LK_REDO_REQUEST;
	}
	else
	{
	    TRdisplay("%@ lk_handle_ivb %d: (undo) status %x LKB %x LLB %x state %2w Gr %w Rq %w\n"
			"\tconverts:%d waiters:%d KEY %s\n",
		    lineno,
		    status,
		    lkb->lkb_id,
		    *(u_i4*)&llb->llb_id,
		    LKB_STATE, lkb->lkb_state,
		    LOCK_MODE, lkb->lkb_grant_mode,
		    LOCK_MODE, lkb->lkb_request_mode,
		    rsb->rsb_converters, rsb->rsb_waiters,
		    LKkey_to_string(&rsb->rsb_name, keydispbuf));
	    /*
	    ** Request was convert upwards. Revert to previously
	    ** held mode (NL if this was a new request.)  We do
	    ** not need to do this if the request mode was less
	    ** than the old grant mode, since holding DLM lock at the
	    ** new lower level cannot interfere with any RCP/CSP
	    ** recovery that is likely to be triggered by the
	    ** failure which caused this lock value to become 
	    ** invalid.
	    */
	    lkb->lkb_cx_req.cx_new_mode = lkb->lkb_grant_mode;
	    status = CXdlm_convert( CX_F_IGNOREVALUE | CX_F_OWNSET | CX_F_PROXY_OK,
				    &lkb->lkb_cx_req );
	    /* Convert downward should always be granted synchronously,
	       but wait call is put here just to be conservative. */
	    if ( status == OK )
		status = CXdlm_wait( waitflags, &lkb->lkb_cx_req, timeout );
	}
    }

    /* Accept any "good" status */
    if ( status == OK || status == E_CL2C01_CX_I_OKSYNC || status == E_CL2C08_CX_W_SUCC_IVB )
    {
	/*
	** 3) Stall this request, unless unstallable.
	*/
	if ( !(flags & LK_NOSTALL) )
	    lk_stall_request(lkb, __LINE__);
	
	TRdisplay("%@ lk_handle_ivb %d: (redo) status %x LKB %x LLB %x state %2w Gr %w Rq %w\n"
			"\tflags %x KEY %s\n",
		lineno,
		status,
		lkb->lkb_id,
		*(u_i4*)&llb->llb_id,
		LKB_STATE, lkb->lkb_state,
		LOCK_MODE, lkb->lkb_grant_mode,
		LOCK_MODE, lkb->lkb_request_mode,
		flags,
		LKkey_to_string(&rsb->rsb_name, keydispbuf));

	/*
	** 4) Redo request
	**
	** Note, if we skipped "undo" phase, this is still done, since
	** converting to the same mode will still refresh the LVB.
	*/
	lkb->lkb_cx_req.cx_new_mode = lkb->lkb_request_mode;
	status = CXdlm_convert( CX_F_STATUS | CX_F_OWNSET | CX_F_PROXY_OK,
				&lkb->lkb_cx_req );

	if ( status == OK && !syncgrant )
	{
	    /*
	    ** If 2nd convert we must check if we need to wait, since
	    ** the normal call to CXdlm_wait has already passed.
	    */
	    status = CXdlm_wait( waitflags, &lkb->lkb_cx_req, timeout );
	}
    }

    /* If mutexes were held, reacquire them */
    if ( syncgrant )
    {
	STATUS	MutexStatus;

	if ( MutexStatus = LK_mutex(SEM_EXCL, &llb->llb_mutex) )
	    return(MutexStatus);
	if ( MutexStatus = LK_mutex(SEM_EXCL, &rsb->rsb_mutex) )
	{
	    (VOID)LK_unmutex(&llb->llb_mutex);
	    return(MutexStatus);
	}
    }

    return status;
} /* lk_handle_ivb */


/*
** Name: lk_stall_request	    - 
**
** Description:
**
**	Suspend caller until stalled requests resumed.
**
** Inputs:
**	lkb	- lock block for stalled request.
**
** Outputs:
**	None.
**
** Returns:
**	VOID
**                                     
** History:
**	16-apr-2001 (devjo01)
**	    Created.
**	19-Mar-2007 (jonj)
**	    Clean up TRdisplay a bit.
**	    Unconditionally display info.
**	    Order stall list FIFO, not LIFO.
**	21-Mar-2007 (jonj)
**	    llb_mutex not held when called, llb not needed.
**	26-Oct-2007 (jonj)
**	    LKB_STALLED replaces LKREQ_STALLED_DURING_REQUEST.
*/
static VOID
lk_stall_request( LKB *lkb, i4 lineno )
{
    LKD		*lkd = (LKD *)LGK_base.lgk_lkd_ptr;
    LLBQ	*prev_llbq;
    RSB		*rsb;
    LLB 	*llb;
    char	keydispbuf[256];

    if ( (LK_mutex(SEM_EXCL, &lkd->lkd_stall_q_mutex)) == OK )
    { 
	/* Check again under mutex to see if stall still required. */
	if ( lkd->lkd_lock_stall )
	{
	    lkb->lkb_attribute |= LKB_STALLED;

	    /*
	    ** Place last on stall queue so that when resumed, locks are
	    ** resumed in their request order.
	    */
	    lkb->lkb_stall_wait.llbq_next =
		    LGK_OFFSET_FROM_PTR(&lkd->lkd_stall_next);
	    lkb->lkb_stall_wait.llbq_prev = lkd->lkd_stall_prev;
	    prev_llbq = (LLBQ *)LGK_PTR_FROM_OFFSET(lkd->lkd_stall_prev);
	    prev_llbq->llbq_next = lkd->lkd_stall_prev =
		    LGK_OFFSET_FROM_PTR(&lkb->lkb_stall_wait);

	    rsb = (RSB*)LGK_PTR_FROM_OFFSET(lkb->lkb_lkbq.lkbq_rsb);
	    llb = (LLB*)LGK_PTR_FROM_OFFSET(lkb->lkb_lkbq.lkbq_llb);

	    TRdisplay("%@ %d Stalling LKB %x LLB %x state %2w Gr %w Rq %w att %v\n"
			"\tKEY %s\n",
		lineno,
		lkb->lkb_id,
		*(u_i4*)&llb->llb_id,
		LKB_STATE, lkb->lkb_state,
		LOCK_MODE, lkb->lkb_grant_mode,
		LOCK_MODE, lkb->lkb_request_mode,
		LKB_ATTRIBUTE, lkb->lkb_attribute,
		LKkey_to_string(&rsb->rsb_name, keydispbuf));

	    (VOID) LK_unmutex(&lkd->lkd_stall_q_mutex);

	    CSsuspend( CS_LOCK_MASK, 0, (PTR)&rsb->rsb_name );
	}
	else
	    (VOID) LK_unmutex(&lkd->lkd_stall_q_mutex);
    }
    return;
} /* lk_stall_request */

#ifdef LK_TRACE
/*{
** Name: LK_rqst_trace()	- Trace non-CSP lock requests
**
** Description:
**	This function acts simply as a switch to enable tracing of lock
**	requests made by servers.  The entire trace capability must be
**	compiled with the LK_TRACE preprocessor symbol defined.  At runtime,
**	the environment variable "II_LK_RQST_TRACE" must also be defined
**	for tracing to take place.  If so, a trace log is created for each
**	server with lock requests (except for CSP in Clustered installations).
**	The log is placed in the directory with the other Ingres logs, and
**	is named "lock_request.log" ("lock_request.log_node" on a VMS node).
**
**	A static switch controls whether tracing is enabled or not once the
**	environment variable is present.  Tracing continues until the variable
**	is absent.  This allows the diagnostician to enable tracing for a
**	brief period and then disable it again.  While this requires the
**	overhead of a variable lookup on every call, it permits the impact
**	of tracing activity on performance to be limited to the activity
**	under scrutiny.  Note that each time the variable is redefined,
**	the same trace file will be written to.
**
**	To get the maximum tracing level, compile the lock facility also with
**	the CSP_DEBUG preprocessor symbol defined and run with II_CSP_DEBUG
**	variable also defined.  This will cause detailed trace info ordinarily
**	produced for CSP lock servicing to be produced for the native locking
**	activity of servers in non-Clustered installations.
** Inputs:
**	none
**
** Outputs:
**	none
**
**	Returns:
**	    0		Tracing is disabled.
**	    1		Tracing is enabled.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	Sets static variable for trace status.
**	Writes trace entries to external file.
**
** History:
**	21-aug-1996 (boama01)
**	    Created for VMS. NOT tested for Unix, NT, etc.
*/
static i4  trd = 0;
static i4
LK_rqst_trace()  
{
    char *trace_env;
    LKD	 *lkd = (LKD *)LGK_base.lgk_lkd_ptr;

    /* Determine if tracing environment variable is on */
    NMgtAt("II_LK_RQST_TRACE", &trace_env);
    if (trace_env && *trace_env)
    {
	/* Variable is on; open new trace file if necessary */
	if (!trd)
	{
	    char trace_file[100], *nodename, *p;
	    CL_ERR_DESC sys_err;

	    trd = 1;
#ifdef VMS
	    STcopy("ii_system:[ingres.files]lock_request.log", trace_file);
	    /*
	    ** Use the first part of the VMS node name to qualify the
	    ** trace file name (up to the colon).
	    */
	    NMgtAt("SYS$NODE", &nodename);
	    if (nodename && *nodename)
	    {
		for (p=nodename; *p; p++)
		    if (*p == ':')
		    {
			*p = 0;
			break;
		    }
		STcat(trace_file, "_");
		STcat(trace_file, nodename);
	    }
#else
#ifdef UNIX
	    STcopy("$II_SYSTEM/ingres/files/lock_request.log", trace_file);
#else
	    STcopy("lock_request.log", trace_file);
#endif
#endif
	    TRset_file(TR_T_OPEN, trace_file, STlength(trace_file), &sys_err);
#ifdef CSP_DEBUG
	    /*
	    ** If CSP_DEBUG tracing was turned on, activate the GLOBAL
	    ** switch for this server, so that all trace entries which depend
	    ** on it will produce their traces.
	    */
	    NMgtAt("II_CSP_DEBUG", &trace_env);
	    if (trace_env && *trace_env)
		csp_debug = TRUE;
#endif
	}                         
	return(1);
    }
    return(0); 
}
#endif /* LK_TRACE */

/*
** Name: dequeue_cbs
**
** Description:
**	Deallocate the control blocks identified.
**
** Inputs:
**	llb		lock list
**	rsb		resource block
**	lkb		lock block, or NULL
**	FreeRsbIfFail	TRUE if RSB should be freed.
**
** Returns:
**	VOID
**
** Side Effects:
**	none ( other than those as a result of LK_deallocate_cb() ).
**
** History:
**	11-sep-96 (nick)
**	    created
**	22-Apr-1997 (jenjo02)
**	   Removed LKREQ_CB_DEALLOCATED flag, whose use was causing
**	   memory leaks when the LKB it was being set in was deallocated.
**	   Instead, changed dequeue_cbs() to be passed a pointer to int
**	   "res_to_free" in which flags will be set indicating which, if
**	   any resources were actually freed.
**	09-sep-2002 (devjo01)
**	   Dispose of any CBACK struct pessimistically allocated to LKB.
**	01-Oct-2007 (jonj)
**	   CBACK now part of LKB.
*/
static VOID
dequeue_cbs(LLB *llb, RSB **rsbp, LKB **lkbp, bool FreeRsbIfFail)
{
    RSB 	*rsb;
    RSH		*rsb_hash_bucket;
    RSB		*prev_rsb;
    RSB		*next_rsb;
    LKB 	*lkb, *prev_lkb;
    LKH		*lkb_hash_bucket;
    LKBQ	*next_lkbq, *prev_lkbq;
    LLBQ	*next_llbq, *prev_llbq;

    LK_WHERE("dequeue_cbs")

    if ( lkbp && (lkb = *lkbp) != NULL )
    {
	/* Unlink from LLB and hash, free the LKB */

	(VOID) LK_mutex(SEM_EXCL, &llb->llb_mutex);

	/* Preserve our stash */
	llb->llb_status |= LLB_ALLOC;

	/* Remove LKB from the LKB hash queue. */
	lkb_hash_bucket = (LKH *)LGK_PTR_FROM_OFFSET(lkb->lkb_lkbq.lkbq_lkh);

	(VOID) LK_mutex(SEM_EXCL, &lkb_hash_bucket->lkh_mutex);
	if (lkb->lkb_lkbq.lkbq_next)
	{
	    next_lkbq = (LKBQ *)LGK_PTR_FROM_OFFSET(lkb->lkb_lkbq.lkbq_next);
	    next_lkbq->lkbq_prev = lkb->lkb_lkbq.lkbq_prev;
	}
	prev_lkbq = (LKBQ *)LGK_PTR_FROM_OFFSET(lkb->lkb_lkbq.lkbq_prev);
	prev_lkbq->lkbq_next = lkb->lkb_lkbq.lkbq_next;
	(VOID) LK_unmutex(&lkb_hash_bucket->lkh_mutex);

	/* Clear LKH stuff out of the LKB */
	lkb->lkb_lkbq.lkbq_lkh = lkb->lkb_lkbq.lkbq_llb = 
	    lkb->lkb_lkbq.lkbq_rsb = 
	    lkb->lkb_lkbq.lkbq_next = lkb->lkb_lkbq.lkbq_prev = 0;
	
	prev_lkb = (LKB *)LGK_PTR_FROM_OFFSET(lkb->lkb_q_prev);

	/* Remove LKB from the LLB's LKB queue. */

	prev_llbq = (LLBQ *)LGK_PTR_FROM_OFFSET(lkb->lkb_llbq.llbq_prev);
	next_llbq = (LLBQ *)LGK_PTR_FROM_OFFSET(lkb->lkb_llbq.llbq_next);
	next_llbq->llbq_prev = lkb->lkb_llbq.llbq_prev;
	prev_llbq->llbq_next = lkb->lkb_llbq.llbq_next;
	
	/*	Decrement count of locks held by this LLB. */

	llb->llb_lkb_count--;
	if ((lkb->lkb_attribute & LKB_PHYSICAL) == 0)
	    llb->llb_llkb_count--;

	/* Release the LKB to the free queue */
	LK_deallocate_cb(lkb->lkb_type, (PTR)lkb, llb);

	llb->llb_status &= ~(LLB_ALLOC);
	(VOID) LK_unmutex(&llb->llb_mutex);

	*lkbp = (LKB*)NULL;
    }

    if ( rsbp && (rsb = *rsbp) != NULL )
    {
	if ( FreeRsbIfFail )
	{
	    rsb_hash_bucket = (RSH *)LGK_PTR_FROM_OFFSET(rsb->rsb_rsh_offset);
	    (VOID) LK_mutex(SEM_EXCL, &rsb_hash_bucket->rsh_mutex);
	    if (rsb->rsb_q_next)
	    {
		next_rsb = (RSB *)LGK_PTR_FROM_OFFSET(rsb->rsb_q_next);
		next_rsb->rsb_q_prev = rsb->rsb_q_prev;
	    }
	    prev_rsb = (RSB *)LGK_PTR_FROM_OFFSET(rsb->rsb_q_prev);
	    prev_rsb->rsb_q_next = rsb->rsb_q_next;
	    (VOID) LK_unmutex(&rsb_hash_bucket->rsh_mutex);

	    LK_deallocate_cb(rsb->rsb_type, (PTR)rsb, llb);
	}
	else
	    (VOID)LK_unmutex(&rsb->rsb_mutex);

	*rsbp = (RSB*)NULL;

    }
}

/*{
** Name: LKpropagate - Propagate locks from old page to new page.
**
** Description:
**
** Inputs:
**      flag                            flag to be set for LKrequest.
**
**	lock_list_id			propagator's lock list id.
**
**      lock_key                        The lock_key for an existing RESOURCE 
**					or LOCK request.
**      new_lock_key                    The lock_key for a RESOURCE or LOCK
**                                      request where it is to be propagated.
** Outputs:
**
**      Returns:
**          OK                          Successful completion
**          LK_BADPARARM                Bad parameter.
**
**      Exceptions:
**         None.
**
** Side Effects:
**          none
**
** History:
**      23-jan-1997 (stial01 for nanpr01)
**          Created.
**	05-May-1997 (jenjo02)
**	    Reconstructed to avoid mutex deadlocks.
**	09-May-1997 (jenjo02)
**	    Added lock_list_id to prototype.
**	17-Mar-2004 (jenjo02)
**	    Be sensitive to the possiblity that the
**	    caller's lock list may be a handle to
**	    a shared list - that's the one on which
**	    the locks are actually held.
**	01-Oct-2007 (jonj)
**	    CBACK is now part of LKB.
**	09-Jan-2009 (jonj)
**	    Integrated static LK_propagate; there's really no need
**	    for it.
*/
STATUS
LKpropagate(
i4                  flags,
LK_LLID		    lock_list_id,
LK_LOCK_KEY         *lock_key,
LK_LOCK_KEY         *new_lock_key,
CL_ERR_DESC         *sys_err)
{
    LKD			*lkd = (LKD *)LGK_base.lgk_lkd_ptr;
    LLB_ID		*input_list_id = (LLB_ID *)&(lock_list_id);
    SIZE_TYPE		*lbk_table;
    LLB			*llb, *proc_llb;
    RSB                 *rsb;
    SIZE_TYPE           end_offset;
    STATUS              status, MutexStatus;
    LKB                 *lkb;
    LK_COMP_HANDLE      completion_handle;
    SIZE_TYPE           lkb_offset;
    SIZE_TYPE		prop_llb_offset;
    LK_LLID             llb_id;
    i4             	err_code;
    i4			prop_count = 0;

    CL_CLEAR_ERR(sys_err);

    lbk_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lkd->lkd_lbk_table);
    llb = (LLB *)LGK_PTR_FROM_OFFSET(lbk_table[input_list_id->id_id]);

    /* proc_llb is the LLB of request */
    proc_llb = llb;
    
    /*
    ** If lock list is a handle to a shared list, then use the actual shared
    ** list llb for the propagate request.
    */
    if (llb->llb_status & LLB_PARENT_SHARED)
    {
	if (llb->llb_shared_llb == 0)
	{
	    uleFormat(NULL, E_CL1055_LK_REQUEST_BADPARAM, (CL_ERR_DESC *)NULL, ULE_LOG,
			NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 1,
			0, input_list_id->id_id);
	    return (LK_BADPARAM);
	}

	llb = (LLB *)LGK_PTR_FROM_OFFSET(llb->llb_shared_llb);

	if ( llb->llb_type != LLB_TYPE ||
	    (llb->llb_status & LLB_SHARED) == 0 )
	{
	    uleFormat(NULL, E_CL1055_LK_REQUEST_BADPARAM, (CL_ERR_DESC *)NULL, ULE_LOG,
			NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 3,
			0, input_list_id->id_id, 0, llb->llb_type, 
			0, llb->llb_status);
	    return (LK_BADPARAM);
	}
    }

    /* lbb is propagator's LLB holding locks */
    prop_llb_offset = LGK_OFFSET_FROM_PTR(llb);
 
    /*
    ** Find the RSB whose locks we want to propagate
    */
    if (status = locate_rsb(lock_key, &rsb, sys_err))
    {
        uleFormat(NULL, E_DMA038_LK_PROP_BAD_RSB, (CL_ERR_DESC *)NULL,
            ULE_LOG, NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code,
            1, 0, "LKpropagate");
    }
    else if (rsb)
    {
	/*
	** For each granted or converting lock in the RSB,
	** request an identical lock for the new lock key.
	**
	** We exempt the caller's lock list from propagation
	** because it is assumed that it already holds a lock
	** on the new resource.
	**
	** To avoid mutex deadlocks, we must release the RSB
	** while making the new lock request, leaving an opening
	** in which the RSB's grant queue may change. After each
	** new lock request, we must continue the grant queue
	** search from the beginning. To avoid finding a lock
	** we've already processed, we set a flag in the LKB
	** and skip over those locks with the flag set. Once
	** the grant queue has been exhausted, we'll have to 
	** go back and reset those LKB flags.
	**
	** It is assumed that no two threads can be propagating
	** locks on the same RSB at the same time, otherwise
	** we're up the creek!
	*/
	end_offset = LGK_OFFSET_FROM_PTR(&rsb->rsb_grant_q_next);
	lkb_offset = rsb->rsb_grant_q_next;

	while (rsb && status == OK && lkb_offset != end_offset)
	{
	    lkb = (LKB *)LGK_PTR_FROM_OFFSET(lkb_offset);
	    lkb_offset = lkb->lkb_q_next;

	    /* Skip this LLB if it's the propagator's */
	    if ( lkb->lkb_lkbq.lkbq_llb != prop_llb_offset &&
	        (lkb->lkb_attribute & LKB_PROPAGATE) == 0 &&
	        (lkb->lkb_state == LKB_GRANTED ||
		 lkb->lkb_state == LKB_CONVERT))
	    {
		llb = (LLB *)LGK_PTR_FROM_OFFSET(lkb->lkb_lkbq.lkbq_llb);

		(VOID)LK_unmutex(&rsb->rsb_mutex);

		/*
		** Lock the LLB to prevent it from being simultaneously
		** updated by a concurrent thread. Once we get the
		** mutex, recheck that the LKB is still owned by
		** this LLB and RSB. If not, abandon it.
		*/
		if ( MutexStatus = LK_mutex(SEM_EXCL, &llb->llb_mutex) )
		    return(MutexStatus);

		if (lkb->lkb_lkbq.lkbq_llb == LGK_OFFSET_FROM_PTR(llb) &&
		    lkb->lkb_lkbq.lkbq_rsb == LGK_OFFSET_FROM_PTR(rsb))
		{
		    /*
		    ** Request a new lock on this LKB's LLB for the
		    ** new lock key. We set the LK_PROPAGATE flag here
		    ** to notify LK_request() that we hold the LLB's
		    ** mutex. LK_request() will release the mutex for
		    ** us before returning. The LLB being propagated
		    ** to may well be waiting on another request, but
		    ** LK_request will overlook that when it sees
		    ** LK_PROPAGATE.
		    */
		    llb_id = *(LK_LLID*)&llb->llb_id;

		    lkb->lkb_attribute |= LKB_PROPAGATE;

		    prop_count++;
		    
		    status = LK_request(flags | (LK_PROPAGATE | LK_NOWAIT |
						LK_NODEADLOCK), 
			llb_id, new_lock_key,
			lkb->lkb_grant_mode, 
			(LK_VALUE *)NULL, (LK_LKID *)NULL,
			lkb->lkb_cback.cback_fcn, 
			lkb->lkb_cback.cback_arg,
			&completion_handle,
			sys_err);

		    if (status == LK_NEW_LOCK || status == LK_GRANTED)
			status = OK;
		}
		else
		    (VOID)LK_unmutex(&llb->llb_mutex);

		/*
		** While the RSB was unmutexed, its locks may have
		** been released and the RSB deallocated or
		** reallocated to a different lock_key.
		*/
		if (rsb->rsb_type == RSB_TYPE)
		{
		    if ( MutexStatus = LK_mutex(SEM_EXCL, &rsb->rsb_mutex) )
			return(MutexStatus);
		    if (rsb->rsb_type == RSB_TYPE &&
			rsb->rsb_name.lk_type == lock_key->lk_type &&
			rsb->rsb_name.lk_key1 == lock_key->lk_key1 &&
			rsb->rsb_name.lk_key2 == lock_key->lk_key2 &&
			rsb->rsb_name.lk_key3 == lock_key->lk_key3 &&
			rsb->rsb_name.lk_key4 == lock_key->lk_key4 &&
			rsb->rsb_name.lk_key5 == lock_key->lk_key5 &&
			rsb->rsb_name.lk_key6 == lock_key->lk_key6)
		    {
			lkb_offset = rsb->rsb_grant_q_next;
			continue;
		    }
		    LK_unmutex(&rsb->rsb_mutex);
		}
		rsb = (RSB *)NULL;
	    }
	}

	if (rsb)
	{
	    /*
	    ** Make one last trip thru the RSB grant queue, this
	    ** time resetting any remaining LKB_PROPAGATE bits.
	    */
	    if (prop_count)
	    {
		for (lkb_offset = rsb->rsb_grant_q_next;
		     lkb_offset != end_offset;
		     lkb_offset = lkb->lkb_q_next)
		{
		    lkb = (LKB *)LGK_PTR_FROM_OFFSET(lkb_offset);
		    lkb->lkb_attribute &= ~ LKB_PROPAGATE;
		}
	    }
	    (VOID)LK_unmutex(&rsb->rsb_mutex);
	}
    }

    return (status);
}

/*
** Name: locate_rsb - locates the rsb in the rsbchain.
**
** Description:
**      This routine contains the code which implements
**   locating a resource block.
**
** History:
**      23-jan-1997 (stial01 for nanpr01)
**          Created.
*/
static STATUS
locate_rsb(
LK_LOCK_KEY         *lock_key,
RSB                 **rsb,
CL_ERR_DESC         *sys_err)
{
    LKD                 *lkd = (LKD *)LGK_base.lgk_lkd_ptr;
    RSH                 *rsh_table;
    u_i4           rsb_hash_value;
    RSH                 *rsb_hash_bucket;
    RSB                 *local_rsb;
    i4             err_code;
    SIZE_TYPE		rsh_offset;
    SIZE_TYPE		rsb_offset;
 
    *rsb = (RSB *)NULL;
 
    if (lock_key == NULL)
    {
        uleFormat(NULL, E_DMA036_LK_PROP_NO_LOCK_KEY, (CL_ERR_DESC *)NULL,
                        ULE_LOG, NULL, (char *)NULL, 0L, (i4 *)NULL,
                        &err_code, 1,
                        0, "locate_rsb");
        return(FAIL);
    }
 
    /*
    ** Get the RSB of the lock.
    */
    rsb_hash_value = (lock_key->lk_type + lock_key->lk_key1 +
        lock_key->lk_key2 + lock_key->lk_key3 + lock_key->lk_key4 +
        lock_key->lk_key5 + lock_key->lk_key6);
 
    rsh_table = (RSH *)LGK_PTR_FROM_OFFSET(lkd->lkd_rsh_table);
    rsb_hash_bucket = (RSH *)&rsh_table
	[(unsigned)rsb_hash_value % lkd->lkd_rsh_size];
 
    rsh_offset = LGK_OFFSET_FROM_PTR(rsb_hash_bucket);
    rsb_offset = rsb_hash_bucket->rsh_rsb_next;

    /*
    ** Search the RSB hash bucket for a matching key,
    ** no mutex.
    */
    while (rsb_offset)
    {
        local_rsb = (RSB *)LGK_PTR_FROM_OFFSET(rsb_offset);
	rsb_offset = local_rsb->rsb_q_next;
 
        if (local_rsb->rsb_name.lk_type == lock_key->lk_type &&
            local_rsb->rsb_name.lk_key1 == lock_key->lk_key1 &&
            local_rsb->rsb_name.lk_key2 == lock_key->lk_key2 &&
            local_rsb->rsb_name.lk_key3 == lock_key->lk_key3 &&
            local_rsb->rsb_name.lk_key4 == lock_key->lk_key4 &&
            local_rsb->rsb_name.lk_key5 == lock_key->lk_key5 &&
            local_rsb->rsb_name.lk_key6 == lock_key->lk_key6)
        {
	    /*
	    ** Lock the RSB, then recheck it
	    */
	    if (LK_mutex(SEM_EXCL, &local_rsb->rsb_mutex))
		return(FAIL);
	    if (local_rsb->rsb_type == RSB_TYPE &&
		local_rsb->rsb_rsh_offset == rsh_offset &&
	        local_rsb->rsb_name.lk_type == lock_key->lk_type &&
		local_rsb->rsb_name.lk_key1 == lock_key->lk_key1 &&
		local_rsb->rsb_name.lk_key2 == lock_key->lk_key2 &&
		local_rsb->rsb_name.lk_key3 == lock_key->lk_key3 &&
		local_rsb->rsb_name.lk_key4 == lock_key->lk_key4 &&
		local_rsb->rsb_name.lk_key5 == lock_key->lk_key5 &&
		local_rsb->rsb_name.lk_key6 == lock_key->lk_key6)
	    {
		/* Found it. Return with RSB mutex locked */
		*rsb = local_rsb;
		return(OK);
	    }

	    /* 
	    ** RSB changed while we waited for its mutex.
	    ** Unlock it and restart the RSH search.
	    */
	    (VOID)LK_unmutex(&local_rsb->rsb_mutex);
	}
	if (local_rsb->rsb_type != RSB_TYPE ||
	    local_rsb->rsb_rsh_offset != rsh_offset ||
	    local_rsb->rsb_q_next != rsb_offset)
	{
	    rsb_offset = rsb_hash_bucket->rsh_rsb_next;
	}
    }
    return(OK);  /* RSB not found.  If table or DB is locked,  */
                      /* this is OK.  For now just assume the best. */
}


# if defined(conf_CLUSTER_BUILD)

/*
** Name: LK_AST_callback - wrapper for blocking callbacks
**
** Description:
**	This routine is called by an asynchronous callback
**	from the DLM.  It takes the parameters passed from 
**	the DLM, and calculates the parameters expected by
**	the DBMS callback routines.
**
** History:
**      29-aug-2002 (devjo01)
**          Created.
**	01-Oct-2007 (jonj)
**	    Embed CBACK in LKB.
**	23-Oct-2007 (jonj)
**	    Just fill in cback_blocked_mode, call lk_back_post.
*/
static VOID
LK_AST_callback( CX_REQ_CB *cx_req, i4 blocked_mode )
{
    LKB		*lkb;

    lkb = (LKB *)CX_RELOFF2PTR(cx_req, cx_req->cx_user_extra);

    LK_CALLBACK_TRACE( 0, &cx_req->cx_key );

# ifdef	PARANOIA
    if ( !lkb )
    {
	TRdisplay( "%@ LK_AST_callback: 'lkb' missing!\n" );
	return;
    }
    if ( LKB_TYPE != lkb->lkb_type )
    {
	TRdisplay( "%@ LK_AST_callback: bad type for lkb at %p\n", lkb );
	return;
    }
# endif /* PARANOIA */

    lkb->lkb_cback.cback_blocked_mode = blocked_mode;
    lk_cback_post( lkb, TRUE );
}


/*
** Name: lk_set_dlm_callback - Set DLM blocking notification if needed.
**
** Description:
**	This routine is centralizes the logig used to set
**	up blocking notification if requested by the user
**	in a clustered environment.
**
** History:
**      06-sep-2002 (devjo01)
**          Created.
**	09-feb-2005 (devjo01)
**	    Close tiny window where cx_user_func was reset to NULL.
**	09-feb-2005 (devjo01)
**	    cx_user_func was not getting cleared if lkb->lkb_cback_fcn
**	    was NULL and lkb->lkb_cback_off was 0.
**	01-Oct-2007 (jonj)
**	    Embed CBACK in LKB.
**	15-Oct-2007 (jonj)
**	    Delete LK_CALLBACK_TRACE( 10 ). Not really useful, and
**	    it takes trace buffer space from more useful entries.
**	    Do not clear cback_q_next; the CBACK may be in the
**	    midst of being processed by the callback thread.
**	23-Oct-2007 (jonj)
**	    cback_lockid replaced by cback_rsb_id.
*/
static VOID
lk_set_dlm_callback( LKB *lkb, RSB *rsb )
{
    if ( lkb->lkb_cback.cback_fcn != (LKcallback)NULL )
    {
	/*
	** Have DLM call wrapper function if blocking
	** notification is triggered.
	*/
	lkb->lkb_cx_req.cx_user_func = LK_AST_callback;
	lkb->lkb_cback.cback_cbt_id = LK_mycbt;
	lkb->lkb_cback.cback_rsb_id = rsb->rsb_id;
    }
    else
	lkb->lkb_cx_req.cx_user_func = NULL;
}
# endif
