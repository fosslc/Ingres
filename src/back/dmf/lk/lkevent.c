/*
** Copyright (c) 1992, 2004 Ingres Corporation
**
*/
#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <di.h>
#include    <er.h>
#include    <me.h>
#include    <pc.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <tr.h>
#include    <ulf.h>
#include    <dmf.h>
#include    <dm.h>
#include    <lk.h>
#include    <cx.h>
#include    <lkdef.h>
#include    <lgkdef.h>

/*
NO_OPTIM = rs4_us5 ris_u64 i64_aix r64_us5
*/

/**
**
**  Name: LKEVENT.C - Implements the LKevent function of the locking system
**
**  Description:
**	This module contains the code which implements LKevent.
**	
**	    LKevent -- set, clear, or wait on an event
**
**  History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable logging and locking system.
**	13-nov-1992 (bryanp)
**	    ERsend is now ERlog.
**	15-dec-1992 (bryanp)
**	    Lock values were not getting set properly when a lock request
**	    completed asynchronously. Resume was setting the llb_async_value
**	    but it needs to also set the value in the LKREQ.
**	18-jan-1993 (bryanp)
**	    Call CL_CLEAR_ERR to clear the sys_err.
**	24-may-1993 (bryanp)
**	    Mutex optimizations -- when LK_cancel returns MUTEX_RELEASED, don't
**		try to re-release the mutex.
**	    Use the pid field from the lkreq, not the pid field from the llb,
**		when resuming an lkreq, since the llb may be a shared llb.
**	    Remove llb_value_addr; use LKREQ_VALADDR_PROVIDED instead.
**	    Remove llb_async_value; use lkreq.value instead.
**	    Remove llb_async_status; use lkreq.status instead.
**	21-jun-1993 (bryanp)
**	    In LK_resume, lkreq->flags should have been lkreq->result_flags.
**	30-jun-1993 (shailaja)
**	    Cast to eliminate semantics change in ANSI C.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (bryanp)
**	    Include <tr.h>
**	    Added some casts to pacify compilers.
**	08-feb-1994 (pearl)
**	    Bug 57376.  In LK_resume(), update the lock value block
**	    even if the rsb is marked invalid.
**	24-feb-94 (swm)
**	    Bug #60425
**	    Corrected type of local session_id variable from i4 to CS_SID
**	    in LKevent();
**	11-nov-1994 (medji01)
**	    Mutex Granularity Project
**		- Changed LK_xmutex() calls to pass mutex address.
**              - Removed LK_mutex() and LK_unmutex() calls for LKD mutex from
**                LKevent()
**              - Acquire LKD/LLB mutexes when traversing event wait queue.
**                queue
**	28-mar-1995 (medji01)
**	    64K+ Lock Limit Project
**		- Removed references to id_instance in LK_ID.
**		- Changed error messages to reference id_id instead
**		  of id_instance.
**	19-jul-1995 (canor01)
**	    Mutex Granularity Project
**		- Acquire LKD mutex before LLB mutex when using both
**	11-Dec-1995 (jenjo02)
**	    Defined lkd_ew_q_mutex to protect event wait queue instead
**	    of using lkd_mutex. Moved LK_cancel_event_wait() here from
**	    lkrlse.c in an attempt to isolate all LKevent-related
**	    code.
**	02-Jul-1996 (jenjo02)
**	    Cleaned up mutex protection of event wait queue.
**	16-Nov-1996 (jenjo02)
**	    Deadlock Detector thread support. In LKsuspend(), add LLB
**	    to deadlock queue; in LKresume(), remove it.
**	    Ifdef'd code which check for an LLB already on the event wait
**	    queue. This code must have been originally placed to track
**	    a rare bug, one I've never encountered. Cleaned up code to 
**	    ensure that the event status bits are maintained correctly.
**	11-Dec-1996 (jenjo02)
**	    Resume session defined by lkb_lkreq.pid, lkb_lkreq.session_id
**	    instead of llb_pid, llb_sid.
**	16-Dec-1996 (jenjo02)
**	    LK_suspend(), LK_resume(): caller must hold lkd_dlock_mutex,
**	    which the function will release.
**	07-Feb-1997 (jenjo02)
**	    Untangled a situation in LK_event() whereby
**	    a thread holding the llb_mutex and wanting the lkd_ew_q_mutex
**	    would deadlock with another thread holding the lkd_ew_q_mutex
**	    and wanting an llb_mutex.
**	25-Feb-1997 (jenjo02)
**	    Added lkd_ew_stamp, llb_ew_stamp. 07-Feb fix above opened a window
**	    in which a just-resumed LLB could replace itself on the event
**	    wait queue before LK_E_CLR had finished traversing it, leading to
**	    the immediate re-resumption of the thread and some pretty bad
**	    thrashing. The two new fields are used to stamp those threads we
**	    resume in a single pass of the wait queue; if we encounter them
**	    again during this traversal. they are ignored.
**	16-Apr-1997 (jenjo02)
**	    Changed event wait queue to LIFO so that just-added events can
**	    be picked off quickly without having to traverse a lengthy
**	    wait queue.
**	06-Aug-1997 (jenjo02)
**	    Lock the LLB while in LK_resume();
**	    Defer setting LKB's pid and sid until we're actually going 
**	    to suspend.
**	22-Jun-1998 (jenjo02)
**	    Do not CSresume LLBs that have been selected for deadlock to
**	    avoid double CSresumes (the other by the deadlock code in LKcancel()),
**	    which can lead to premature thread resumption and CL103E errors.
**	20-Jul-1998 (jenjo02)
**	    LKevent() event type and value enlarged and changed from 
**	    u_i4's to *LK_EVENT structure.
**	08-Sep-1998 (jenjo02)
**	    Added PHANTOM_RESUME_BUG debug code.
**	16-Nov-1998 (jenjo02)
**	  o Cross-process thread identity changed to CS_CPID structure
**	    from PID and SID.
**	  o Installed macros to use ReaderWriter locks when CL-available
**	    for blocking the deadlock matrix.
**	  o Removed PHANTOM_RESUME_BUG code.
**	14-Jan-1999 (jenjo02)
**	    Before inserting LLB on deadlock wait list, check to see that
**	    it's not already on the list. This can happen with LK_MULTITHREAD
**	    "shared" lock lists and needs a more robust architectural fix
**	    to avoid missing deadlocks! LKBs, not LLBs, should be the objects
**	    on the deadlock wait queue.
**	08-Feb-1999 (jenjo02)
**	  o When doing a LK_E_CLR event function, don't mutex the requestor's
**	    LLB. There's no need and it avoids a potential mutex deadlock
**	    (BUG 94790).
**	  o Deadlock wait queue is now a list of LKBs, not LLBs. Handle accordingly.
**	25-Jun-1999 (ahaal01)
**	    Added NO_OPTIM for rs4_us5 to prevent E_CL1033_LK_EVENT_BAD_PARAM
**	    error from createdb utility.
**      21-May-1999 (hweho01)
**          Added NO_OPTIM for ris_u64 to eliminate error when trying to
**          bring up recovery server.
**	06-Oct-1999 (jenjo02)
**	    Handle to LLBs is now thru LLB_ID, not LK_ID.
**	22-Oct-1999 (jenjo02)
**	    Deleted lkd_dlock_lock.
**	24-Mar-2000 (jenjo02)
**	    When validating lock list, test LLB_SHARED, not LLB_PARENT_SHARED.
**	    PARENT_SHARED can be waited on; the referenced SHARED list
**	    cannot.
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
**	04-Oct-2000 (jenjo02)
**	    Replaced CScp_resume() with LKcp_resume macro.
**      28-Dec-2000 (horda03)
**          An LLB mutex is being held when resuming a session
**          which is waiting for a DBEvent in another server.
**          On some platforms, performing a cross-process resume
**          will cause the process performing the Resume to be
**          descheduled; thus allowing the resumed session to
**          execute and attempt to acquire a mutex before the
**          resumng processes has released it. Results in POOR
**          performance. Bug 103596.
**	12-apr-2001 (devjo01)
**	    s103715 (Portable cluster support)
**	21-May-2001 (hanje04)
**	    The above horda03 change was Xed from 2.0 by change 450601.
**	    However changes to LK, LG and LGK in main have made the
**	    changes in this file obselete. Backing them out.
**	16-aug-2001 (toumi01)
**	    speculative i64_aix NO_OPTIM change for beta xlc_r - FIXME !!!
**	12-nov-2002 (horda03) Bug 105321 INGSRV 1502
**	    Following a user interrupt, iF LKcancel() returns LK_CANCEL,
**	    then the lock request has been granted before it could be
**	    cancelled. Rather than calling CScancelled(), call CSsuspend()
**	    as a resume is in the pipe-line. But just to be safe, suspend
**	    with a timeout.
**      23-Jul-2003 (hweho01)
**          Turned off optimization for AIX 64-bit build (r64_us5).
**          Eliminated error during creating iidbdb.
**          Compiler : VisualAge 5.023.
**      7-oct-2004 (thaju02)
**          Use SIZE_TYPE to allow memory pools > 2Gig.
**	05-jan-2005 (devjo01)
**	    Change test for determining whether to rely on a DLM completion
**	    in LK_resume.
**      18-oct-2005 (horda03) Bug 115405/INGSRV3463
**          Fixed E_CL1034 error message parameters.
**	09-Dec-2008 (jonj)
**	    SIR 120874: use new form uleFormat, CL_CLEAR_ERR.
*/

/*{
** Name: LKevent()	- Handle processing of a request service.
**
** Description:
**	This function makes the appropriate call to make the request and
**	manages mapping and storing the error codes.
**
** Inputs:
**      flag                            Event flags:
**						LK_E_SET
**						LK_E_WAIT
**                                      	LK_E_CLR
**						LK_E_INTERRUPT
**	lock_list_id			The lock list to use.
**	event				Pointer to event type and value.
**
**  Outputs:
**	err_code			Reason for error status return.
**
**	Returns:
**    	    OK
**    	    LK_BADPARAM
**	    LK_INTERRUPT
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
**	    Mutex optimizations -- when LK_cancel returns MUTEX_RELEASED, don't
**	    try to re-release the mutex.
**	24-feb-94 (swm)
**	    Bug #60425
**	    Corrected type of local session_id variable from i4 to CS_SID
**	    in LKevent();
**	11-nov-1994 (medji01)
**	    Mutex Granularity Project
**              - Removed LK_mutex() and LK_unmutex() calls for LKD mutex from
**                LKevent()
**      30-Jun-2000 (horda03) X-Int of fix for bug 48904 (VMS only)
**        30-mar-94 (rachael)
**           Added call to CSnoresnow to provide a temporary fix/workaround
**           for sites experiencing bug 48904. CSnoresnow checks the current
**           thread for being CSresumed too early.  If it has been, that is if 
**           scb->cs_mask has the EDONE bit set, CScancelled is called.
**	28-Feb-2002 (jenjo02)
**	     Any non-zero status returned from CSsuspend means "interrupted",
**	     not just E_CS0008_INTERRUPTED.
**	20-aug-2002 (devjo01)
**	     Pass pointer to event structure, so CSmonitor can provide added
**	     diagnostic info.
**	12-nov-2002 (horda03) Bug 105321.
**	    Avoid potential for spurious Resumes of a session following a user
**	    interrupt, by suspending the session if LKcancel() returns LK_CANCEL
**	    i.e. the lock request was granted before it could be cancelled, so a
**	    RESUME of the session is pending. Bug 105321 occurs because the
**	    CScancelled() call doesn't delete the pending resume - this can
**	    occur if the resume if from a different process (a CP resume), and
**	    this server hasn't received the CP message yet.
**	12-nov-2002 (horda03) Bug 105321 addition.
**	    Extension to above bug fix. If an event wait was cancelled, then don't
**	    wait for a RESUME as one isn't sent. This prevents the session waiting
**	    10 seconds for a resume, which during SHUTDOWN caused an E_GCFFFF and
**	    a E_GC0024 to be reported. Added new LLB status flag LLB_CANCELLED_EVENT.
**	16-Sep-2005 (thaju02)
**	    Address compilation errors introduced by xinteg of change 461066
**	    for bug 105321.
**	28-Nov-2006 (kiria01) b115062
**	    Don't flush resumes unless we are called in an event wait
**          preparation state. If this were to be done in an EWAIT call
**	    then we may wipe resumes that we would like to wait for.
**	09-Jan-2009 (jonj)
**	    Remove unused LK_COMP_HANDLE* from LK_cancel() prototype.
**	17-Nov-2009 (kschendel) SIR 122890
**	    More parens on ?: expression (paranoia).
*/
STATUS
LKevent(
i4		flag,
LK_LLID		lock_list_id,
LK_EVENT	*event,
CL_ERR_DESC	*sys_err)
{
    STATUS	status;
    LLB_ID      *input_list_id = (LLB_ID *) &lock_list_id;
    SIZE_TYPE   *lbk_table;
    LLB         *llb;
    LKD         *lkd = (LKD *)LGK_base.lgk_lkd_ptr;

    LK_WHERE("LKevent")

    CL_CLEAR_ERR(sys_err);

    /* Remove any Rogue Resumes */
    /* ... if we are setting up for an event wait */
    if (flag & LK_E_SET)
	CSnoresnow( "LK!LKEVENT.C::LKevent", 1);

    if ((flag & LK_E_SET) &&
        (input_list_id->id_id != 0) &&
        (i4)input_list_id->id_id <= lkd->lkd_lbk_count)
    {
       lbk_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lkd->lkd_lbk_table);
       llb = (LLB *)LGK_PTR_FROM_OFFSET(lbk_table[input_list_id->id_id]);

       if (llb->llb_status & LLB_ESET)
       {
          TRdisplay( "%@ LKevent(). llb_status ESET set. status = %08x\n",
                     llb->llb_status);

          if (CSsuspend(CS_TIMEOUT_MASK, 10, 0) == E_CS0009_TIMEOUT)
          {
             TRdisplay( "%@ LKevent(). Suspend timeout. llb_status = %08x\n\n",
                      llb->llb_status);
          }
          else
          {
             TRdisplay( "%@ LKevent(). RESUMED. LLB_STATUS = %08x\n\n",
                        llb->llb_status);
          }
       }
    }

    status = LK_event(flag, lock_list_id, event, sys_err);

    if (status == LK_GRANTED)
	status = OK;
    else if ( status == OK && CSsuspend( (flag & LK_E_INTERRUPT)
				  ? (CS_LKEVENT_MASK | CS_INTERRUPT_MASK)
				  : CS_LKEVENT_MASK, 
				  0, (PTR)event ) )
    {

#ifdef xEVENT_DEBUG
	CS_SID	session_id;
	CSget_sid(&session_id);
	TRdisplay("%@ %x: llb %x was interrupted fro event\n", session_id,
		    lock_list_id);
#endif

	lbk_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lkd->lkd_lbk_table);
	llb = (LLB *)LGK_PTR_FROM_OFFSET(lbk_table[input_list_id->id_id]);

	status = LK_cancel(lock_list_id, (SIZE_TYPE *)NULL, sys_err);

	if (llb->llb_status & LLB_CANCELLED_EVENT)
	{
	    /* Event was cancelled, don't expect any subsequent resume */
	    llb->llb_status &= ~LLB_CANCELLED_EVENT;
	}
	else if ( (status == LK_CANCEL) &&
		  (CSsuspend(CS_TIMEOUT_MASK, 10, 0) == E_CS0009_TIMEOUT))
	{
	    TRdisplay( "%@ LKevent() - Interrupt for GRANTED event. No resume received\n");
	}

	CScancelled((PTR) 0);

	status = LK_INTERRUPT;
    }

    return(status);
}

/*{
** Name: LK_event	- Event services.
**
** Description:
**      This routines implements simple event waiting and event signalling
**	services.  A lock list that waits for a event, will wait until the
**	event is signaled by another caller. 
**
**	To wait for an event, a client must first set the event using
**	LKevent(LK_E_SET).  The caller must assure that the event it is
**	waiting for does not occur before this call.  If it does then
**	the wait call may never wake up.
**
**	The client then issues the call: LKevent(LK_E_WAIT).  This will
**	suspend the caller until the event is signalled by some other
**	thread using LKevent(LK_E_CLR).  If the event has already been
**	signalled - and was signalled after the call to LKevent(LK_E_SET),
**	then the LK_E_WAIT call will return immediately.
**
**	To signal an event, use LKevent(LK_E_CLR).  This will wakeup all
**	threads waiting on an LKevent(LK_E_WAIT) call for the event being
**	signalled.
**
**	If the mask LK_E_CROSS_PROCESS is specified, then the event mechanism
**	will work between cooperating processes.  LKevent calls will only
**	affect other processes that also specify this flag.
**
**
** Inputs:
**      flag                            Event option:
**					LK_E_SET    - set up for event wait
**					LK_E_WAIT   - wait for event
**					LK_E_CLR    - signal event
**					LK_E_CROSS_PROCESS - allow event to
**						be signalled between processes.
**	lock_list_id			Lock list to wait.
**	event				Pointer to type and value of event.
**
** Outputs:
**      err_code                        Reason for error return status.
**	Returns:
**	    OK
**	    LK_BADPARAM
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable logging and locking system.
**	26-jul-1993 (bryanp)
**	    Added some casts to pacify compilers.
**	06-oct-93 (swm)
**	    Bugs #56447, #56449
**	    Fixed session_id to be a CS_SID rather than a nat. Print as %p
**	    in TRdisplay to avoid truncation on platforms where the size of
**	    a pointer is greater than the size of an int.
**      13-dec-1994 (medji01)
**          Mutex Granularity Project
**              - Acquire LKD/LLB mutexes when traversing event wait queue.
**	28-mar-1995 (medji01)
**	    64K+ Lock Limit Project
**		- Removed references to id_instance in LK_ID.
**		- Changed error messages to reference id_id instead
**		  of id_instance.
**	02-Jul-1996 (jenjo02)
**	    Cleaned up mutex protection of event wait queue.
**	08-oct-1996 (nanpr01 for jenjo02)
**	    bracket the check correctly.
**	16-Nov-1996 (jenjo02)
**	    Ifdef'd code which check for an LLB already on the event wait
**	    queue. This code must have been originally placed to track
**	    a rare bug, one I've never encountered. Cleaned up code to 
**	    ensure that the event status bits are maintained correctly.
**	07-Feb-1997 (jenjo02)
**	    Untangled a situation in LK_event() whereby
**	    a thread holding the llb_mutex and wanting the lkd_ew_q_mutex
**	    would deadlock with another thread holding the lkd_ew_q_mutex
**	    and wanting an llb_mutex.
**	25-Feb-1997 (jenjo02)
**	    Added lkd_ew_stamp, llb_ew_stamp. 07-Feb fix above opened a window
**	    in which a just-resumed LLB could replace itself on the event
**	    wait queue before LK_E_CLR had finished traversing it, leading to
**	    the immediate re-resumption of the thread and some pretty bad
**	    thrashing. The two new fields are used to stamp those threads we
**	    resume in a single pass of the wait queue; if we encounter them
**	    again during this traversal. they are ignored.
**	16-Apr-1997 (jenjo02)
**	    Changed event wait queue to LIFO so that just-added events can
**	    be picked off quickly without having to traverse a lengthy
**	    wait queue.
**	08-Feb-1999 (jenjo02)
**	  o When doing a LK_E_CLR event function, don't mutex the requestor's
**	    LLB. There's no need and it avoids a potential mutex deadlock
**	    (BUG 94790).
**	06-Oct-1999 (jenjo02)
**	    Handle to LLBs is now thru LLB_ID, not LK_ID.
**	24-Mar-2000 (jenjo02)
**	    When validating lock list, test LLB_SHARED, not LLB_PARENT_SHARED.
**	    PARENT_SHARED can be waited on; the referenced SHARED list
**	    cannot.
**	28-Feb-2002 (jenjo02)
**	    Reorganized code to encourage better throughput.
**	12-nov-2002 (horda03) Bug 105321.
**	    Clear the LLB_CANCELLED_EVENT flag.
**	04-Jul-15 (wanfr01)
**	    Bug 112670, INGREP163
**	    Hold the LLB mutex while adding it to the event wait queue to
**	    avoid race condition under heavy replicator activity.
**	    Note this change follows the expected mutex order used by
**	    LK_event (LK_E_CLR) and LK_cancel_event_wait (i.e. LLB mutex is 
**	    first grabbed then the event wait queue mutex is grabbed)
**	13-Mar-2006 (jenjo02)
**	    Simplified list traversal. Make but one pass rather than
**	    pounding through it and mutex-dancing maybe forever.
**	    Delete llb_ew_stamp, lkd_ew_stamp.
**	11-Feb-2008 (jonj)
**	    Ignore lock_list_id for LK_E_CLR-only type calls. 
**	    There are legitimate cases (dm2tInvalidateTCB()) in which 
**	    the calling thread has no lock list of its own, but still
**	    needs to signal the firing of some event to waiting lock lists.
**	    Whether the caller has a lock list or not is irrelevant
**	    when signalling an event.
*/
STATUS
LK_event(
i4                  flag,
LK_LLID		    lock_list_id,
LK_EVENT	    *event,
CL_ERR_DESC	    *sys_err)
{
    LKD		*lkd = (LKD *)LGK_base.lgk_lkd_ptr;
    LLB		*llb;
    LLB		*wait_llb;
    LLBQ	*llbq;
    LLBQ	*next_llbq;
    LLBQ	*prev_llbq;
    LLB_ID	*input_list_id = (LLB_ID *) &lock_list_id;
    STATUS	status = LK_GRANTED;
    SIZE_TYPE	llbq_offset;
    SIZE_TYPE	end_offset;
    SIZE_TYPE	*lbk_table;
    i4		err_code;
    u_i4	ew_stamp;
    CS_SID	session_id;

    LK_WHERE("LK_event")

#ifdef xEVENT_DEBUG
    CSget_sid(&session_id);
#endif

    /* Validate the lock list id, unless LK_E_CLR. */
    if ( !(flag & LK_E_CLR) )
    {
	if (input_list_id->id_id == 0 ||
		(i4)input_list_id->id_id > lkd->lkd_lbk_count )
	{
	    uleFormat(NULL, E_CL1033_LK_EVENT_BAD_PARAM, (CL_ERR_DESC *)NULL, ULE_LOG,
			    NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 2,
			    0, input_list_id->id_id, 0, lkd->lkd_lbk_count);
	    return (LK_BADPARAM);
	}

	lbk_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lkd->lkd_lbk_table);
	llb = (LLB *)LGK_PTR_FROM_OFFSET(lbk_table[input_list_id->id_id]);

	if (llb->llb_type != LLB_TYPE || 
	    llb->llb_id.id_instance != input_list_id->id_instance ||
	    llb->llb_status & LLB_SHARED)
	{
	    uleFormat(NULL, E_CL1034_LK_EVENT_BAD_PARAM, (CL_ERR_DESC *)NULL, ULE_LOG,
			    NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 3,
			    0, *(u_i4*)input_list_id,
			    0, llb->llb_type,
			    0, *(u_i4*)&llb->llb_id);
	    return (LK_BADPARAM);
	}
    }


    /*
    ** LK_E_SET and LK_E_WAIT can both be requested at the
    ** same time; LK_E_CLR must be separate.
    */
    if (flag & LK_E_SET)
    {
	/* First lock the list, then the LLB */
	LK_mutex(SEM_EXCL, &lkd->lkd_ew_q_mutex);
	LK_mutex(SEM_EXCL, &llb->llb_mutex);

	if (llb->llb_status & LLB_ESET)
	{
	    uleFormat(NULL, E_CL1035_LK_EVENT_BAD_PARAM, (CL_ERR_DESC *)NULL, ULE_LOG,
			NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 2,
			0, llb->llb_status, 0, flag);
	    LK_unmutex(&llb->llb_mutex);
	    LK_unmutex(&lkd->lkd_ew_q_mutex);
	    return (LK_BADPARAM);
	}
	
	/* Prepare the LLB before inserting on the event list */
	CSget_cpid(&llb->llb_cpid);
	llb->llb_status |= LLB_ESET;
	llb->llb_status &= ~(LLB_EWAIT | LLB_EDONE);

	/* Clear the LLB_CANCELLED_EVENT mask */
	llb->llb_status &= ~LLB_CANCELLED_EVENT;

	llb->llb_event = *event;
	llb->llb_evflags = (flag & LK_E_CROSS_PROCESS) ? LK_CROSS_PROCESS_EVENT : 0;

#ifdef xEVENT_DEBUG
	if (LK_llb_on_list(llb, lkd))
	{
	    uleFormat(NULL, E_DMA016_LKEVENT_LIST_ERROR, (CL_ERR_DESC *)NULL, ULE_LOG,
			NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 1,
			0, llb->llb_id.id_id);
	    TRdisplay("ERROR: Lock list %d already on event list!\n",
		    llb->llb_id.id_id);
	    LK_unmutex(&llb->llb_mutex);
	    LK_unmutex(&lkd->lkd_ew_q_mutex);
	    return (LK_BADPARAM);
	}
#endif

	/*
	** ESET  means the LLB is on the wait queue
	** EWAIT means the LLB is waiting on an event
	** EDONE means the event arrived before the thread suspended.
	*/

	/* Insert LLB on the END of the wait queue (FIFO ) */
	llb->llb_ev_wait.llbq_next = LGK_OFFSET_FROM_PTR(&lkd->lkd_ew_next);
	llb->llb_ev_wait.llbq_prev = lkd->lkd_ew_prev;
	prev_llbq = (LLBQ *)LGK_PTR_FROM_OFFSET(lkd->lkd_ew_prev);
	prev_llbq->llbq_next = lkd->lkd_ew_prev =
	    LGK_OFFSET_FROM_PTR(&llb->llb_ev_wait);

	LK_unmutex(&llb->llb_mutex);
	LK_unmutex(&lkd->lkd_ew_q_mutex);

#ifdef xEVENT_DEBUG
	TRdisplay("%@ %p: llb %x added to event wait queue\n",
		    session_id, llb->llb_id);
#endif
    }

    if (flag & LK_E_WAIT)
    {
	/*  Wait for an event. */
	if ( LK_mutex(SEM_EXCL, &llb->llb_mutex) )
	    return (E_DMA015_LKEVENT_SYNC_ERROR);
	    
	if ( llb->llb_status & LLB_EDONE )
	{
#ifdef xEVENT_DEBUG
	    TRdisplay("%@ %p: llb %x found event already occurred\n", session_id,
			llb->llb_id);
#endif
	    llb->llb_status &= ~(LLB_EDONE);
	}
	else
	{
#ifdef xEVENT_DEBUG
	    TRdisplay("%@ %p: llb %x will now wait for event\n", session_id,
			llb->llb_id);
#endif
	    llb->llb_status |= LLB_EWAIT;
	    status = OK;
	}
	(VOID) LK_unmutex(&llb->llb_mutex);
    }

    if ( (flag & (LK_E_CLR | LK_E_SET | LK_E_WAIT) ) == LK_E_CLR )
    {
	/* 	Signal waiters that an event has occurred */

	/*	Traverse the wait queue front to back. */

	/* Lock the list while we traverse it once */
	if (LK_mutex(SEM_EXCL, &lkd->lkd_ew_q_mutex))
	    return (E_DMA015_LKEVENT_SYNC_ERROR);

	end_offset = LGK_OFFSET_FROM_PTR(&lkd->lkd_ew_next);

	for (llbq_offset = lkd->lkd_ew_next;
	     llbq_offset !=  end_offset;
	    )
	{
	    llbq = (LLBQ *)LGK_PTR_FROM_OFFSET(llbq_offset);
	    llbq_offset = llbq->llbq_next;

	    wait_llb = (LLB *)((char *)llbq - CL_OFFSETOF(LLB,llb_ev_wait));
	    /*
	    ** Note that "wait_llb" may be the "llb" of this caller
	    ** due to the screwy way that SCE events are implemented.
	    ** The llb_cpid of the event waiter, however, will be 
	    ** another thread, not the caller's.
	    */

#ifdef xEVENT_DEBUG
	    TRdisplay("%@ %p: llb %x being checked for event\n", session_id,
			wait_llb->llb_id);
#endif

	    /*
	    ** If the event types and values don't match, or if the PID's
	    ** are different and both threads did not specify CROSS_PROCESS
	    ** then skip this LLB.
	    */
	    if ( wait_llb->llb_event.value == event->value &&
		 wait_llb->llb_event.type_low == event->type_low &&
		 wait_llb->llb_event.type_high == event->type_high &&
		 (wait_llb->llb_cpid.pid == LGK_my_pid ||
		     (wait_llb->llb_evflags & LK_CROSS_PROCESS_EVENT &&
			  flag & LK_E_CROSS_PROCESS)) )
	    {
		/* Lock the LLB */
		LK_mutex(SEM_EXCL, &wait_llb->llb_mutex);

		/* Remove LLB from the wait queue */

		llbq = (LLBQ *)&wait_llb->llb_ev_wait.llbq_next;
		next_llbq = (LLBQ *)LGK_PTR_FROM_OFFSET(llbq->llbq_next);
		prev_llbq = (LLBQ *)LGK_PTR_FROM_OFFSET(llbq->llbq_prev);
		next_llbq->llbq_prev = llbq->llbq_prev;
		prev_llbq->llbq_next = llbq->llbq_next;

		/* LLB no longer on event wait list */
		wait_llb->llb_status &= ~(LLB_ESET);

#ifdef xEVENT_DEBUG
		TRdisplay("%@ %p: llb %x being removed from event list\n", session_id,
			    wait_llb->llb_id);
#endif

		/* Is it waiting? */
		if (wait_llb->llb_status & LLB_EWAIT)
		{
		    /* Then wake it up */
		    wait_llb->llb_status &= ~(LLB_EWAIT);
		    LKcp_resume(&wait_llb->llb_cpid);
		}
		else
		{
#ifdef xEVENT_DEBUG
		    TRdisplay("%@ %p: clear before set for llb %x\n", session_id,
				wait_llb->llb_id);
#endif
		    /* Then the event arrived before suspend */
		    wait_llb->llb_status |= LLB_EDONE;
		}
		(VOID) LK_unmutex(&wait_llb->llb_mutex);
	    }
	}
	(VOID) LK_unmutex(&lkd->lkd_ew_q_mutex);
    }

    return(status);
}

/*{
** Name: LK_resume	- Resume a lock request.
**
** Description:
**      This routine is called to resume a suspended lock request,
**	first removing it from its LLB wait queue and deadlock queue.
**
** Inputs:
**      lkb                             The LKB
**	    lkb_cpid			The process/session to resume.
**
** Outputs:
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable logging and locking system.
**	15-dec-1992 (bryanp)
**	    If the llb_value_addr is set, then the lock completion value needs
**	    to be copied into the designated value block.
**	24-may-1993 (bryanp)
**	    Use the pid field from the lkreq, not the pid field from the llb,
**		when resuming an lkreq, since the llb may be a shared llb.
**	    Remove llb_value_addr; use LKREQ_VALADDR_PROVIDED instead.
**	    Remove llb_async_value; use lkreq.value instead.
**	    Remove llb_async_status; use lkreq.status instead.
**	21-jun-1993 (bryanp)
**	    In LK_resume, lkreq->flags should have been lkreq->result_flags.
**	    Converted to function prototypes.
**	08-feb-1994 (pearl)
**	    Bug 57376.  In LK_resume(), update the lock value block
**	    even if the rsb is marked invalid.
**	16-Nov-1996 (jenjo02)
**	    Remove LLB from deadlock queue, if it's on it.
**	11-Dec-1996 (jenjo02)
**	    Resume session defined by lkb_lkreq.pid, lkb_lkreq.session_id
**	    instead of llb_pid, llb_sid.
**	16-Dec-1996 (jenjo02)
**	    LK_suspend(), LK_resume(): caller must hold lkd_dlock_mutex,
**	    which the function will release.
**	06-Aug-1997 (jenjo02)
**	    Lock the LLB while in LK_resume();
**	19-Jan-1998 (jenjo02)
**	    Backed out 06-Aug-1997 change to lock the LLB. This causes
**	    mutex deadlocks in a multi-server DMCM environment.
**	22-Jun-1998 (jenjo02)
**	    Do not CSresume LLBs that have been selected for deadlock to
**	    avoid double CSresumes (the other by the deadlock code in LKcancel()),
**	    which can lead to premature thread resumption and CL103E errors.
**	28-Feb-2002 (jenjo02)
**	    NEVER touch llb_status without holding llb_mutex. Removed reset
**	    of LLB_WAITING, relying on value of llb_waiters (which is blocked
**	    by lkd_dw_q_mutex and thread-safe).
**	05-jan-2005 (devjo01)
**	    Change test for determining whether to rely on a DLM completion
**	    routine to test LKB_DISTRIBUTED_LOCK in lkb_attribute for a
**	    lock by lock determination, rather than LKD_CLUSTER_SYSTEM and
**	    treating all locks as distributed.
**      15-Apr-2005 (hanal04) Bug 114316 INGSRV3257
**          LLB_WAITING was still being used to trip the phantom resume
**          diags. Modify the test to check llb_waiters under the
**          lkd_dw_q_mutex to prevent false positives.
**	3-Feb-2006 (kschendel)
**	    I think Alex meant to take out the original flag-test diag,
**	    it's littering the dbms logs.  Remove it.
**	13-Mar-2006 (jenjo02)
**	    Add explanation of how llb_waiting can expectedly be zero.
**	09-Jan-2009 (jonj)
**	    Deleted use of LKREQ; LKB_VALADDR_PROVIDED and lkb_value,
**	    lkb_cpid replace it. Resetting of LLB_ENQWAITING now done
**	    elsewhere where it makes more sense.
*/
VOID
LK_resume(LKB *lkb)
{
    LKD                 *lkd = (LKD *)LGK_base.lgk_lkd_ptr;
    LLB                 *llb;
    LLBQ		*next_llbq;
    LLBQ		*prev_llbq;
    RSB			*rsb;
    bool		awaken_session = TRUE;

    LK_WHERE("LK_resume")

    /*
    ** Removed Alex's "llb_waiting" phantom resume check after
    ** diagnosing how llb_waiting ends up being zero in here.
    ** 
    ** Two threads are waiting on locks on the same resource
    ** and both timeout (sarjo01's ordent stress test induces
    ** this).
    **
    ** Thread A handles its timeout via 
    **   LK_cancel->LK_do_cancel->LK_do_unlock
    **		LK_do_cancel removes its LKB from the
    **		dead_wait and llb_wait queues, resets
    **		LKB_ON_DLOCK_LIST and LKB_ON_LLB_WAIT,
    **		and decrements its llb_waiters to zero.
    **		LK_do_unlock then tries to mutex the RSB
    **		to remove itself from the RSB wait queue,
    **		but is blocked by Thread B.
    **
    ** Thread B in the meantime has made its way through
    **   LK_cancel->LK_do_cancel->LK_do_unlock->LK_try_wait
    **		LK_try_wait sees Thread A's LKB on the mutexed
    **		RSB wait queue and calls here to wake it up,
    **		but by now Thread A has marked itself as    
    **		"not waiting"
    **
    ** This is "normal", though intertwined, behavior and
    ** should be innocuous as long as Thread B doesn't attempt
    ** to resume Thread A, which it does not.
    **
    ** Slightly different timing could have Thread B all the
    ** way through here via LK_try_wait and in fact resuming
    ** Thread A before it has a chance to remove itself from
    ** the dead_wait and llb_wait queues, but that should
    ** result in Thread A's lock being granted while it's
    ** trying to cancel it, and that scenario is handled with
    ** CScancelled() in LKrequest.
    */


    llb = (LLB *)LGK_PTR_FROM_OFFSET(lkb->lkb_lkbq.lkbq_llb);
    rsb = (RSB *)LGK_PTR_FROM_OFFSET(lkb->lkb_lkbq.lkbq_rsb);

    /* Remove LKB from LLBs wait and LKD deadlock queues */
    if (lkb->lkb_attribute & (LKB_ON_DLOCK_LIST | LKB_ON_LLB_WAIT))
    {
	/* lkd_dw_q_mutex protects both queues */
	if (LK_mutex(SEM_EXCL, &lkd->lkd_dw_q_mutex))
	    return;

	if (lkb->lkb_attribute & LKB_ON_DLOCK_LIST)
	{
	    next_llbq = (LLBQ *)LGK_PTR_FROM_OFFSET(lkb->lkb_dead_wait.llbq_next);
	    prev_llbq = (LLBQ *)LGK_PTR_FROM_OFFSET(lkb->lkb_dead_wait.llbq_prev);
	    next_llbq->llbq_prev = lkb->lkb_dead_wait.llbq_prev;
	    prev_llbq->llbq_next = lkb->lkb_dead_wait.llbq_next;
	    lkb->lkb_attribute &= ~(LKB_ON_DLOCK_LIST);
	    lkd->lkd_dw_q_count--;
	}
	if (lkb->lkb_attribute & LKB_ON_LLB_WAIT)
	{
	    next_llbq = (LLBQ *)LGK_PTR_FROM_OFFSET(lkb->lkb_llb_wait.llbq_next);
	    prev_llbq = (LLBQ *)LGK_PTR_FROM_OFFSET(lkb->lkb_llb_wait.llbq_prev);
	    next_llbq->llbq_prev = lkb->lkb_llb_wait.llbq_prev;
	    prev_llbq->llbq_next = lkb->lkb_llb_wait.llbq_next;
	    lkb->lkb_attribute &= ~(LKB_ON_LLB_WAIT);
	    /* One less waiter on this LLB */
	    llb->llb_waiters--;
	}
	else 
	    awaken_session = FALSE;
	(VOID)LK_unmutex(&lkd->lkd_dw_q_mutex);
    }

    if (lkb->lkb_attribute & LKB_DISTRIBUTED_LOCK)
    {
	/* s103715 note:
	**
	** Session is always awoken by completion routine, and value is
	** rerturned at that point.
	*/
    	awaken_session = FALSE;
    }
    else if (lkb->lkb_attribute & LKB_VALADDR_PROVIDED)
    {
	/*
	** Lock requestor is expecting lock value, cache it in the LKB 
	** to be retrieved by LK_get_completion_status()
	*/

	lkb->lkb_attribute &= ~LKB_VALADDR_PROVIDED;

	if (rsb->rsb_invalid)
	    lkb->lkb_status = LK_VAL_NOTVALID;
	lkb->lkb_value.lk_high = rsb->rsb_value[0];
	lkb->lkb_value.lk_low = rsb->rsb_value[1];
	lkb->lkb_value.lk_mode = lkb->lkb_grant_mode;
    }

    if (awaken_session)
	LKcp_resume(&lkb->lkb_cpid);

    return;
}

/*{
** Name: LK_suspend	- Suspend a lock list.
**
** Description:
**      A lock list needs to be suspended because the lock or conversion
**	could not be granted immediately.
**
**	This routine adds the ungrantable LKB to the lock list's queue
**	of waiting-to-be-granted locks and to the waiting-to-be-deadlock-
**	checked queue.
**
** Inputs:
**      lkb                             The LKB to suspend.
**	flags				"LK_NODEADLOCK" is checked for.
**
** Outputs:
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable logging and locking system.
**	15-dec-1992 (bryanp)
**	    Convert value_addr to offset before storing it in llb_value_addr.
**	24-may-1993 (bryanp)
**	    Remove llb_value_addr; use LKREQ_VALADDR_PROVIDED instead.
**	    Remove llb_async_status; use lkreq.status instead.
**	21-jun-1993 (bryanp)
**	    Officially removed arguments "status_addr", "value_addr", and
**		"timeout"; these are all handled through the LKREQ technique now
**	15-Dec-1995 (jenjo02)
**	    Mutex redux. Caller is assumed to hold llb_mutex.
**	15-Nov-1996 (jenjo02)
**	    Optimistic deadlock detection.
**	    Added "flags" parm to prototype, put LLB on deadlock wait queue
**	    if needed.
**	11-Dec-1996 (jenjo02)
**	    Resume session defined by lkb_lkreq.pid, lkb_lkreq.session_id
**	    instead of llb_pid, llb_sid.
**	16-Dec-1996 (jenjo02)
**	    LK_suspend(), LK_resume(): caller must hold lkd_dlock_mutex,
**	    which the function will release.
**	06-Aug-1997 (jenjo02)
**	    Defer setting LKB's pid and sid until we're actually going 
**	    to suspend.
**	14-Jan-1999 (jenjo02)
**	    Before inserting LLB on deadlock wait list, check to see that
**	    it's not already on the list. This can happen with LK_MULTITHREAD
**	    "shared" lock lists and needs a more robust architectural fix
**	    to avoid missing deadlocks! LKBs, not LLBs, should be the objects
**	    on the deadlock wait queue.
**	10-Aug-1999 (jenjo02)
**	    Removed "status" parm from prototype. Caller is now responsible for
**	    setting lkb_lkreq.status.
**	28-Feb-2002 (jenjo02)
**	    NEVER touch llb_status without holding llb_mutex. Removed setting
**	    of LLB_WAITING, relying on value of llb_waiters (which is blocked
**	    by lkd_dw_q_mutex and thread-safe).
**	09-Jan-2009 (jonj)
**	    Deleted "proc_llb" from prototype, unused.
*/
VOID
LK_suspend(
LKB		    *lkb,
i4		    flags)
{
    LLB		    *llb;
    LKD             *lkd = (LKD *)LGK_base.lgk_lkd_ptr;
    LLBQ	    *next_llbq, *prev_llbq;

    LK_WHERE("LK_suspend")

    /* The list holding the locks */
    llb = (LLB *)LGK_PTR_FROM_OFFSET(lkb->lkb_lkbq.lkbq_llb);
    
    /* lkd_dw_q_mutex protects both the LLB wait and LKD deadlock queues */
    if (LK_mutex(SEM_EXCL, &lkd->lkd_dw_q_mutex))
	return;

    /* Insert LKB on the tail of the LLBs list of waiting LKBs */
    lkb->lkb_llb_wait.llbq_prev = llb->llb_lkb_wait.llbq_next;
    lkb->lkb_llb_wait.llbq_next = LGK_OFFSET_FROM_PTR(&llb->llb_lkb_wait.llbq_next);
    prev_llbq = (LLBQ *)LGK_PTR_FROM_OFFSET(llb->llb_lkb_wait.llbq_prev);
    prev_llbq->llbq_next = llb->llb_lkb_wait.llbq_prev =
	LGK_OFFSET_FROM_PTR(&lkb->lkb_llb_wait.llbq_next);
    lkb->lkb_attribute |= LKB_ON_LLB_WAIT;

    /* One more waiter on this LLB */
    llb->llb_waiters++;

    if ((flags & LK_NODEADLOCK) == 0)
    {
	/* Insert LKB on the head of the deadlock detection queue */
	lkb->lkb_dead_wait.llbq_next = lkd->lkd_dw_next;
	lkb->lkb_dead_wait.llbq_prev = LGK_OFFSET_FROM_PTR(&lkd->lkd_dw_next);
	next_llbq = (LLBQ *)LGK_PTR_FROM_OFFSET(lkd->lkd_dw_next);
	next_llbq->llbq_prev = lkd->lkd_dw_next =
	    LGK_OFFSET_FROM_PTR(&lkb->lkb_dead_wait);
	lkb->lkb_attribute |= LKB_ON_DLOCK_LIST;
	lkb->lkb_deadlock_wakeup = lkd->lkd_stat.dlock_wakeups;
	lkd->lkd_dw_q_count++;
    }

    (VOID)LK_unmutex(&lkd->lkd_dw_q_mutex);

    return;
}

/*{
** Name: LK_llb_on_list  - See if LLB already on event wait queue
**
** Description:
**      We assert that when putting an LLB on the lkd_ew_wait queue, that llb
**      should not already be on the wait queue. If it is, we return an error
**      rather than placing the LLB on the list twice, and thus corrupting
**      the event queue. This is to help in tracking down some bug elsewhere
**      in the system which is placing the LLB on the queue multiple times.
**
** Inputs:
**      llb                             The LLB to check
**	lkd     			LKD pointer
**
** Outputs:
**	Returns:
**	    i4                          0 - LLB not currently on the list
**                                      1 - LLB is on the list
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable logging and locking system.
**	16-Nov-1996 (jenjo02)
**	    Wrapped code that calls this in ifdef xEVENT_DEBUG. In
**	    a properly operating installation, this check should not
**	    be necessary.
**
*/
i4
LK_llb_on_list(LLB *llb, LKD *lkd)
{
    SIZE_TYPE	    end_offset;
    SIZE_TYPE	    llbq_offset;
    LLB		    *wait_llb;
    LLBQ	    *llbq;
    CL_ERR_DESC	    sys_err;

    LK_WHERE("LK_llb_on_list")

    end_offset = LGK_OFFSET_FROM_PTR(&lkd->lkd_ew_next);

    for (llbq_offset = lkd->lkd_ew_next;
	 llbq_offset !=  end_offset;
	)
    {
	llbq = (LLBQ *)LGK_PTR_FROM_OFFSET(llbq_offset);
	llbq_offset = llbq->llbq_next;

	wait_llb = (LLB *)((char *)llbq - CL_OFFSETOF(LLB,llb_ev_wait));

	if (wait_llb == llb)
	{
	    TRdisplay("ERROR! LLB is already on list!\n");
	    ERlog("ERROR! LLB is already on list!\n",
		    sizeof("ERROR! LLB is already on list!\n")-1,
		    &sys_err);
	    return (1);
	}

	if (llbq_offset != llbq->llbq_next)
	    llbq_offset = lkd->lkd_ew_next;
    }

    /*
    ** Success: llb is not currently on list:
    */
    return (0);
}

/*
** Name: LK_cancel_event_wait		- cancel a lock event wait.
**
** Description:
** If this lock list is waiting for a lock event, cancel the wait:
**	remove the lock list from the event wait queue if it's on there.
**	turn off the event flags.
**
** History:
**	23-oct-1992 (bryanp)
**	    Created.
**	14-dec-1994 (medji01)
**	    Mutex Granularity Project
**		- Acquire the LKD mutex for the search of the event wait queue.
**	11-Dec-1995 (jenjo02)
**	    Moved here from lkrlse.c to isolate all LKevent-related code.
**	    Protect event wait queue with lkd_ew_q_mutex instead of lkd_mutex.
**      12-Nov-2002 (horda03) Bug 105321
**          Set LLB_CANCELLED_EVENT flag, to highlight to LKevent() that this
**          event wait has been cancelled, so no RESUME is pending.
**	13-Mar-2006 (jenjo02)
**	    Deal with all possible event states, and in an orderly manner.
**	19-Mar-2007 (jonj)
**	    Add IsRundown parm, in which case the pid/sid need not "match".
*/
STATUS
LK_cancel_event_wait( LLB *llb, LKD *lkd, bool IsRundown, CL_ERR_DESC *sys_err)
{
    LLBQ	*llbq;
    LLBQ	*next_llbq;
    LLBQ	*prev_llbq;
    CS_CPID	cpid;

    LK_WHERE("LK_cancel_event_wait")

    /* Caller holds llb_mutex */

    if ( llb->llb_status & (LLB_ESET | LLB_EWAIT | LLB_EDONE) )
    {
	CSget_cpid(&cpid);

	/* Only if LK_rundown or event state relates to this thread... */
	if ( IsRundown ||
	     (llb->llb_cpid.pid == cpid.pid &&
	      llb->llb_cpid.sid == cpid.sid) )
	{
	    /* If on event wait queue, remove it */
	    if (llb->llb_status & LLB_ESET)
	    {
		/* Release LLB, mutex queue, remutex LLB, recheck */
		LK_unmutex(&llb->llb_mutex);
		LK_mutex(SEM_EXCL, &lkd->lkd_ew_q_mutex);
		LK_mutex(SEM_EXCL, &llb->llb_mutex);

		if ( llb->llb_status & LLB_ESET )
		{
		    llbq = (LLBQ *)&llb->llb_ev_wait.llbq_next;

		    next_llbq = (LLBQ *)LGK_PTR_FROM_OFFSET(llbq->llbq_next);
		    prev_llbq = (LLBQ *)LGK_PTR_FROM_OFFSET(llbq->llbq_prev);
		    next_llbq->llbq_prev = llbq->llbq_prev;
		    prev_llbq->llbq_next = llbq->llbq_next;

		    llb->llb_status &= ~LLB_ESET;
		}
		(VOID) LK_unmutex(&lkd->lkd_ew_q_mutex);
	    }

	    llb->llb_status &= ~(LLB_EWAIT | LLB_EDONE);

	    /* Indicate that the event has been cancelled, 
	    ** so no resumes expected
	    */
	    llb->llb_status |= LLB_CANCELLED_EVENT;
	}
    }


    return(OK);
}
