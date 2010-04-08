/*
**Copyright (c) 2004 Ingres Corporation
**
*/

/*
NO_OPTIM = r64_us5
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <di.h>
#include    <er.h>
#include    <pc.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <dmf.h>
#include    <ulf.h>
#include    <lg.h>
#include    <lgdef.h>
#include    <lgdstat.h>
#include    <lgkdef.h>

/**
**
**  Name: LGEVENT.C - Implements the LGevent function of the logging system
**
**  Description:
**	This module contains the code which implements LGevent.
**	
**	    LGevent -- Await an event in the logging system
**	    LG_signal_event -- signal the occurrence of an event
**	    LG_suspend	    -- record that this transaction is suspending
**	    LG_resume	    -- resume a suspended transaction
**	    LG_suspend_lbb  -- attach an LXB to a log buffer's wait queue
**	    LG_resume_lbb   -- resume LXBs on a log buffer's wait queue
**
**  History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable logging and locking system.
**	26-oct-1992 (bryanp)
**	    Add includes of dmf.h and ulf.h.
**	29-oct-1992 (bryanp)
**	    Improve error message logging.
**	15-mar-1993 (rogerk)
**	    Reduced Logging - Phase IV:
**	    Removed EBACKUP, FBACKUP, 1PURGE_ACTIVE, 2PURGE_COMPLETE states.
**	17-mar-1993 (rogerk)
**	    Moved lgd status values to lgdstat.h so that they are visible
**	    by callers of LGshow(LG_S_LGSTS).
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	31-jan-1994 (jnash)
**	    Fix lint detected sys_err un-initialization.
**	05-Jan-1995 (jenjo02)
**	    Defined new mask for LGevent suspends, CS_LGEVENT_MASK.
**	    Mutex granularity project. Semaphores now must be explicitly
**	    named in calls to LKMUTEX.C
**	18-Jan-1996 (jenjo02)
**          lbk_table replaced with lfbb_table (LFBs) and ldbb_table
**          (LDBs) to ensure that semaphores now embedded in those
**          structures are not corrupted. Similarly, sbk_table 
**          replaced with lpbb_table (LPBs), lxbb_table (LXBs), and
**          lpdb_table (LPDs).
**	25-Jan-1996 (jenjo02)
**	    Mutex granularity project. Many semaphores replace
**	    the lone lgd_mutex.
**	    Added lgd_mutex_is_locked boolean to LG_signal_event()
**	    prototype.
**	21-Feb-1996 (jenjo02)
**	    LG_resume: Resume LXB only if it's actually waiting. Testing 
**	    kept running into the situation whereby an LXB was queued
**	    on an lbb_wait queue, wanting notification when the
**	    buffer hit the disk. In the mean time, the thread
**	    continued processing, but when RCP finished the i/o,
**	    it resumed the already running thread. In this instance,
**	    DI was trying to open a file, calling CScause_event()
**	    to induce a slave to actually do the open, and CSsuspend()-ing
**	    until the slave completed its open work. In the meantime,
**	    LGresume was being called from RCP on write completion,
**	    CSresume()-ing the DI path and causing CSsuspend() to
**	    think the event (the slave's open) had already completed,
**	    manifesting itself in an "file open error".
**	19-Apr-1996 (jenjo02)
**	    In LG_suspend() put new waiters on the end of the queue
**	    instead of the front.
**	16-Jul-1996 (jenjo02)
**	    Changed LG_signal_event() to accept a second parm of lgd_status
**	    bits to be turned off at the same time the 1st parm bits are
**	    being turned on.
**	06-Nov-1996 (jenjo02)
**	    Changed LG_E_xxxxxxxx event values to match lgd_status
**	    values to lessen the time we must hold lgd_status_mutex.
**	07-Nov-1996 (jenjo02)
**	    Rewrote the way that LXBs waiting on LG events are awoken.
**	    Previously, all LXBs in the lgd_wait_event queue were resumed,
**	    at which time they called back in to LG_event() to get the current
**	    event mask and went back to sleep if the event(s) they were not
**	    interested in had not yet been signalled. This lead to a lot of 
**	    contention on lgd_status_mutex and the unnecessary resumption
**	    of threads.
**	    Now, only the LXBs which are actually waiting on the signalled
**	    event(s) are resumed, at which time they're presented with an
**	    up-to-date mask of all currently signalled events.
**	    Two new internal functions added, LG_suspend_event() and 
**	    LG_resume_event(), to deal specifically with the event wait queue.
**	19-Feb-1997 (jenjo02)
**	    LG_event() was taking a SHARE lock on lgd_status_mutex, then calling
**	    LG_suspend_event() to add its LXB to the event wait list. If multiple
**	    threads do this concurrently, corruption of the wait list may
**	    ensue because the event wait list relies on lgd_status_mutex
**	    to protect it. Changed to EXCL lock.
**	16-Nov-1998 (jenjo02)
**	    Cross-process thread identity changed to CS_CPID structure
**	    from PID and SID.
**	19-Feb-1999 (jenjo02)
**	    Added new functions LG_suspend_lbb(), LG_resume_lbb(). 
**      08-Dec-1999 (hanal04) Bug 97015 INGSRV 819
**          (hanje04) X-Integ of 444135 from oping20. Changed to E_DMA495
**          because E_DMA493 already exists in main.
**          Prevent process spins in LG_resume_event(), LG_resume() and
**          LG_resume_lbb() prevent process spins caused by the
**          end_offset exit condition never being met. There is an
**          underlying concurrency issue that will need to be addressed
**          if clients start seeing E_DMA493.
**	    Added new functions LG_suspend_lbb(), LG_resume_lbb().
**	24-Aug-1999 (jenjo02)
**	    Dropped lgd_status_mutex, lfb_cp_mutex, use lgd_mutex instead.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	04-Oct-2000 (jenjo02)
**	    Replaced CScp_resume() with LGcp_resume macro.
**	17-sep-2001 (somsa01)
**	    Due to lass cross-integration, changed nat's and longnat's to
**	    i4's. Also, added missing variable definition in LG_resume_lbb().
**	17-sep-2001 (somsa01)
**	    Backing out changes for bug 97015, as they are not needed in
**	    2.6+.
**	12-nov-2002 (horda03) Bug 105321 INGSRV 1502
**	    Following a user interrupt, iF LG_cancel() returns LG_CANCELGRANT, 
**	    then the lock request has been granted before it could be 
**	    cancelled. Rather than calling CScancelled(), call CSsuspend()
**	    as a resume is in the pipe-line. But just to be safe, suspend
**	    with a timeout.
**      12-Feb-2003 (hweho01)
**          Turned off optimizer for AIX hybrid build by using 
**          the 64-bit configuration string (r64_us5), because the    
**          error happens only with the 64-bit compiler option.  
**	08-Apr-2004 (jenjo02)
**	    Added "minitran" wait reason, corresponds to
**	    LXB_MINI_WAIT.
**      7-oct-2004 (thaju02)
**          Use SIZE_TYPE to allow memory pools > 2Gig.
**	15-Mar-2006 (jenjo02)
**	    lxb_wait_reason defines moved to lg.h, lgd_stat.wait
**	    cleaned up.
**	11-Sep-2006 (jonj)
**	    Add new "commit" wait reason for LOGFULL COMMIT.
**	6-Apr-2007 (kschendel) SIR 122890
**	    Use an intermediate for aliasing external i4 id's and LG_ID.
**	    Originally this was to fix gcc -O2's strict aliasing, but
**	    Ingres has so many other strict-aliasing violations that
**	    -fno-strict-aliasing is needed.  The id/LG_ID thing was so
**	    egregious, though, that I've kept this change.
**	09-Dec-2008 (jonj)
**	    SIR 120874: use new form uleFormat, CL_CLEAR_ERR.
*/

/*
** Forward declarations for internal functions:
*/
static STATUS	    LG_event(
			i4                 flag,
			LG_ID		    *lx_id,
			i4		    event_mask,
			LXB		    **lxb,
			STATUS		    *async_status,
			CL_ERR_DESC	    *sys_err);

static STATUS       LG_cancel(
			i4		    flags,
			LG_ID		    *lx_id);

static VOID         LG_resume_event();

static VOID         LG_suspend_event(
			LXB		    *lxb);


/* 
** These wait reason literals are provided for iimonitor, ipm, or
** anyone else interested in why an LXB is asleep.
*/
static char *wait_reason[] = 
{
	"not waiting",
	"log force",
	"split buffer",
	"hdr i/o",
	"ckpdb",
	"opendb",
	"bcpstall",
	"logfull",
	"free buffer",
	"last buffer write",
	"buffer i/o",
	"log event",
	"logwriter sleep",
	"minitran",
	"commit"
};

/*{
** Name: LGevent	- Wait for logging event.
**
** Description:
**      This routine will wait for a specified logging system
**	event to occur, or will return the current logging system
**	events.  If the flag is set to LG_NOWAIT, the current event
**	mask is returned.
**
** Inputs:
**      flag                            LG_NOWAIT	- return immediately
**					LG_INTRPTABLE	- request interruptable
**	lx_id				Logging system transaction id.
**	event_mask			Mask of any event to wait for.
**					Any set of LG_E_* constants.
**					LG_E_M_ABORT	- event to manually 
**							  abort a transaction.
**					LG_E_M_COMMIT	- event to manually 
**							  commit a transaction.
**
** Outputs:
**      events                          The current set of events.
**	Returns:
**	    OK
**	    LG_BADPARAM		    Parameter(s) in error.
**	    LG_INTERRUPT	    Event interrupted.
**	    LG_NOTLOADED	    Logging system not loaded.
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable logging and locking system.
**	25-Jan-1996 (jenjo02)
**	    Removed acquisition and freeing of lgd_mutex.
**	    Defined LGEVENT_MASK for CSsuspend() to distinguish
**	    log event waits from other wait types.
**	07-Nov-1996 (jenjo02)
**	    Rewrote the way that LXBs waiting on LG events are awoken.
**	    Previously, all LXBs in the lgd_wait_event queue were resumed,
**	    at which time they called back in to LG_event() to get the current
**	    event mask and went back to sleep if the event(s) they were not
**	    interested in had not yet been signalled. This lead to a lot of 
**	    contention on lgd_status_mutex and the unnecessary resumption
**	    of threads.
**	    Now, only the LXBs which are actually waiting on the signalled
**	    event(s) are resumed, at which time they're presented with an
**	    up-to-date mask of all currently signalled events.
**	    Two new internal functions added, LG_suspend_event() and 
**	    LG_resume_event(), to deal specifically with the event wait queue.
**      12-nov-2002 (horda03) Bug 105321.
**          Avoid potential for spurious Resumes of a session following a user
**          interrupt, by suspending the session if LG_cancel() returns 
**          LG_CANCELGRANT i.e. the lock request was granted before it could be 
**          cancelled, so a RESUME of the session is pending. Bug 105321 occurs
**          because the CScancelled() call doesn't delete the pending resume - 
**          this can occur if the resume if from a different process (a CP 
**          resume), and this server hasn't received the CP message yet. 
*/
STATUS
LGevent(flag, lx_id, event_mask, events, sys_err)
i4                  flag;
LG_LXID		    lx_id;
i4		    event_mask;
i4		    *events;
CL_ERR_DESC	    *sys_err;
{
    LGD    	    *lgd = (LGD *)LGK_base.lgk_lgd_ptr;
    LXB		    *lxb;
    LG_I4ID_TO_ID   xid_to_id;
    i4	    sleep_flag;
    STATUS	    status;
    STATUS	    async_status;

    xid_to_id.id_i4id = lx_id;
    status = LG_event(flag, &xid_to_id.id_lgid, event_mask, &lxb,
			  &async_status, sys_err);

    if (status == LG_MUST_SLEEP)
    {
	/* LG_event() must "wait" (ie. sleep) for the event */
	    
	sleep_flag = CS_LGEVENT_MASK;

	if (flag & LG_INTRPTABLE)
	    sleep_flag |= CS_INTERRUPT_MASK;

	status = CSsuspend(sleep_flag, 0, 0);

	/*
	** We will be awakened when interrupted
	** or when one of our events is signalled.
	*/
	if (status == E_CS0008_INTERRUPTED)
	{
	    STATUS lg_status;

	    status = LG_INTERRUPT;

	    if (lg_status = LG_cancel(flag, &xid_to_id.id_lgid))
		status = lg_status;
	    
            /* If the LG_cancel returns LG_CANCELGRANT, this means that the
            ** Logging event occured whilst session was attempting to cancel
            ** its event wait. So a Resume is on its way. To avoid "rogue
            ** resumes" lets wait a short period for the resume to arrive.
            */
            if ( (status == LG_CANCELGRANT) &&
                 (CSsuspend(CS_TIMEOUT_MASK, 10, 0) == E_CS0009_TIMEOUT))
            {
               TRdisplay( "LGevent() - Interrupt for GRANTED event. No resume received\n");
            }

	    CScancelled((PTR) NULL);
	}
    }

    if (status == OK)
	*events = lxb->lxb_events;

    return(status);
}

/*{
** Name: LG_event	- Wait for logging event.
**
** Description:
**      This routine allows a transaction to wait for a specific
**	logging system event.  It can optionally just poll the current
**	event status.
**
** Inputs:
**      flag                            Modifier flags
**	lx_id				Logging system transaction id.
**	event_mask			Event mask.
**
** Outputs:
**      events                          Events actually recorded.
**	Returns:
**	    OK				At least one of the masked events is
**					currently active, or flag specified
**					LG_NOWAIT
**	    LG_MUST_SLEEP		None of the masked events is currently
**					active and you did not say nowait.
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable logging and locking system.
**	15-mar-1993 (rogerk)
**	    Reduced Logging - Phase IV:
**	    Removed EBACKUP, FBACKUP, 1PURGE_ACTIVE, 2PURGE_COMPLETE states.
**	31-jan-1994 (jnash)
**	    Fix lint detected sys_err un-initialization.
**	25-Jan-1996 (jenjo02)
**	    lgd_mutex no longer held on entry to function. Other, new,
**	    mutexes scattered thruout.
**	06-Nov-1996 (jenjo02)
**	    Changed LG_E_xxxxxxxx event values to match lgd_status
**	    values to lessen the time we must hold lgd_status_mutex.
**	07-Nov-1996 (jenjo02)
**	    Rewrote the way that LXBs waiting on LG events are awoken.
**	    Previously, all LXBs in the lgd_wait_event queue were resumed,
**	    at which time they called back in to LG_event() to get the current
**	    event mask and went back to sleep if the event(s) they were not
**	    interested in had not yet been signalled. This lead to a lot of 
**	    contention on lgd_status_mutex and the unnecessary resumption
**	    of threads.
**	    Now, only the LXBs which are actually waiting on the signalled
**	    event(s) are resumed, at which time they're presented with an
**	    up-to-date mask of all currently signalled events.
**	    Two new internal functions added, LG_suspend_event() and 
**	    LG_resume_event(), to deal specifically with the event wait queue.
*/
static STATUS
LG_event(
i4                  flag,
LG_ID		    *lx_id,
i4		    event_mask,
LXB		    **our_lxb,
STATUS		    *async_status,
CL_ERR_DESC	    *sys_err)
{
    register LGD    *lgd = (LGD *)LGK_base.lgk_lgd_ptr;
    register LXB    *lxb;
    register LPD    *lpd;
    register LDB    *ldb;
    SIZE_TYPE	    *lxbb_table;
    i4	    err_code;
    STATUS	    status;
    bool	    must_mutex = ((flag & LG_NOWAIT) == 0);

    LG_WHERE("LG_event")

    CL_CLEAR_ERR(sys_err);

    /*	Check the lx_id. */

    if (lx_id->id_id == 0 || (i4)lx_id->id_id > lgd->lgd_lxbb_count)
    {
	uleFormat(NULL, E_DMA451_LG_EVENT_BAD_ID, (CL_ERR_DESC *)NULL,
		    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 2,
		    0, lx_id->id_id, 0, lgd->lgd_lxbb_count);
	return (LG_BADPARAM);
    }

    lxbb_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lgd->lgd_lxbb_table);
    lxb = (LXB *)LGK_PTR_FROM_OFFSET(lxbb_table[lx_id->id_id]);

    if (lxb->lxb_type != LXB_TYPE ||
	lxb->lxb_id.id_instance != lx_id->id_instance)
    {
	uleFormat(NULL, E_DMA452_LG_EVENT_BAD_XACT, (CL_ERR_DESC *)NULL,
		    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
		    0, lxb->lxb_type, 0, lxb->lxb_id.id_instance,
		    0, lx_id->id_instance);
	return (LG_BADPARAM);
    }

    lpd = (LPD *)LGK_PTR_FROM_OFFSET(lxb->lxb_lpd);
    ldb = (LDB *)LGK_PTR_FROM_OFFSET(lpd->lpd_ldb);

    /* Return pointer to transaction's LXB */
    *our_lxb = lxb;

    /*
    ** Get list of outstanding events in system.
    */

    /*
    ** If caller just wants a picture of the current status, 
    ** dirty-read the lgd and ldb, otherwise we must
    ** lock the lgd/ldb to prevent the status from 
    ** changing between the time we look at it and
    ** the time we decide to suspend. If the event is
    ** signalled in that window, we'll miss the event
    ** and sleep forever.
    */
    if (must_mutex)
    {
	if (status = LG_mutex(SEM_EXCL, &lgd->lgd_mutex))
	    return(status);
    }

    /*
    ** Now that the returned events (LG_E_xxxxxxx) have the
    ** same values as the LGD events, we can just copy the event word
    ** instead of translating from LGD to LG_E format.
    */
    lxb->lxb_event_mask = event_mask;
    lxb->lxb_events     = lgd->lgd_status;

    /*
    ** Check for events that are specific to the clients database.
    */
    if (ldb->ldb_status & LDB_BACKUP)
    {
	if (must_mutex) 
	{
	    if (status = LG_mutex(SEM_EXCL, &ldb->ldb_mutex))
	    return(status);
	}

	if (lgd->lgd_status & LGD_CKP_SBACKUP)
	{
	    if ((ldb->ldb_status & LDB_BACKUP) &&
		(ldb->ldb_status & LDB_STALL) &&
		(ldb->ldb_lxbo_count == 0))
	    {
		lxb->lxb_events |= LG_E_SBACKUP;
	    }
	}

	/*
	** Check for backup error status.
	** Note that there is no LGD status for this event.
	*/
	if ((ldb->ldb_status & LDB_BACKUP) &&
	    (ldb->ldb_status & LDB_CKPERROR))
	{
	    lxb->lxb_events |= LG_E_BACKUP_FAILURE;
	}

	if (must_mutex)
	    (VOID)LG_unmutex(&ldb->ldb_mutex);
    }

    /*
    ** Check for requested events.
    ** If none currently active, client will have to sleep until the next
    ** event is signaled (if NOWAIT specified, just return current events).
    */
    if ((flag & LG_NOWAIT) || (lxb->lxb_events & lxb->lxb_event_mask))
    {
	if (must_mutex)
	    (VOID)LG_unmutex(&lgd->lgd_mutex);
	return (OK);
    }

    /*
    ** Wait for event.
    **
    ** When the thread is resumed, it will have either
    ** been interrupted or had one of its awaited 
    ** events signalled.
    */
    LG_suspend_event(lxb);

    (VOID)LG_unmutex(&lgd->lgd_mutex);

    return (LG_MUST_SLEEP);
}

/*{
** Name: LG_signal_event	- Signal an event.
**
** Description:
**      This routine notes that a event has been declared
**	and resumes those processes waiting for any 
**	currently signalled event.
**
** Inputs:
**      set_event                       The event status to be set.
**	unset_event			lgd_status bits to be reset
**      lgd_status_is_locked		TRUE if caller holds lgd_status_mutex
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
**	29-Jan-1996 (jenjo02)
**	    Added lgd_status_is_locked parm. We need to lock the
**	    lgd_status_mutex to update lgd_status; some callers already
**	    have that mutex locked for other reasons - if so,
**	    they'll tell us, and we won't try to acquire the
**	    mutex again, leaving it locked on exit from this
**	    function.
**	16-Jul-1996 (jenjo02)
**	    Changed LG_signal_event() to accept a second parm of lgd_status
**	    bits to be turned off at the same time the 1st parm bits are
**	    being turned on.
**	07-Nov-1996 (jenjo02)
**	    Fctn may now be called with a NULL event to trigger a scan
**	    and resumption of signalled LXBs.
*/
VOID
LG_signal_event(i4 set_event,
		i4 unset_event,
		bool lgd_status_is_locked)
{
    register LGD	*lgd = (LGD *)LGK_base.lgk_lgd_ptr;

    LG_WHERE("LG_signal_event")

    /* lgd_mutex protects lgd_status and lgd_wait_event queue */
    if (lgd_status_is_locked == FALSE &&
	LG_mutex(SEM_EXCL, &lgd->lgd_mutex)) 
	return;

    /*
    ** Reset specified status bits, if any.
    */
    lgd->lgd_status &= ~(unset_event);

    /*
    ** Set signalled events, if any.
    */
    lgd->lgd_status |= set_event;

    /*
    ** Resume any threads waiting on currently signalled events.
    */
    LG_resume_event();

    if (lgd_status_is_locked == FALSE)
	(VOID)LG_unmutex(&lgd->lgd_mutex);

    return;
}

/*{
** Name: LG_suspend_event	- Suspend an LXB pending the arrival 
**	 			  of an event.
**
** Description:
**	This function suspends an LXB in anticipation of
**	the arrival of some logging event.
**
** Inputs:
**      lxb                       The LXB to be suspended.
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
**	07-Nov-1996 (jenjo02)
**	    Created.
**	    Caller must hold lgd_status_mutex.
*/
static VOID
LG_suspend_event(LXB *lxb)
{
    register LGD    *lgd = (LGD *)LGK_base.lgk_lgd_ptr;
    register LWQ    *lwq = &lgd->lgd_wait_event;
    register LPD    *lpd;
    register LPB    *lpb;
    register LDB    *ldb;
    LXBQ	    *prev_lxbq;

    LG_WHERE("LG_suspend_event")

    CSget_cpid(&lxb->lxb_cpid);

    lpd = (LPD *)LGK_PTR_FROM_OFFSET(lxb->lxb_lpd);
    ldb = (LDB *)LGK_PTR_FROM_OFFSET(lpd->lpd_ldb);
    lpb = (LPB *)LGK_PTR_FROM_OFFSET(lpd->lpd_lpb);

    /*  Change various counters. */

    lgd->lgd_stat.wait[LG_WAIT_ALL]++;
    lgd->lgd_stat.wait[LG_WAIT_EVENT]++;
    lxb->lxb_stat.wait++;
    ldb->ldb_stat.wait++;
    lpb->lpb_stat.wait++;

    /*	Queue to the tail of the event wait queue. */

    /* Note that lgd_mutex protects the event wait queue */

    lxb->lxb_wait.lxbq_prev = lwq->lwq_lxbq.lxbq_prev;
    lxb->lxb_wait.lxbq_next = LGK_OFFSET_FROM_PTR(&lwq->lwq_lxbq.lxbq_next);
    prev_lxbq = (LXBQ *)LGK_PTR_FROM_OFFSET(lwq->lwq_lxbq.lxbq_prev);
    prev_lxbq->lxbq_next =
	lwq->lwq_lxbq.lxbq_prev = LGK_OFFSET_FROM_PTR(&lxb->lxb_wait);

    lxb->lxb_wait_lwq = LGK_OFFSET_FROM_PTR(lwq);

    lwq->lwq_count++;

    lxb->lxb_status |= LXB_WAIT;
    lxb->lxb_wait_reason = LG_WAIT_EVENT;

    /*
    ** Provide a pointer to the wait reason literal for CS_suspend
    */
    lxb->lxb_wait_reason_string = wait_reason[LG_WAIT_EVENT];

    return;
}

/*{
** Name: LG_resume_event	- Resume LXB(s) which are awaiting
**			          signalled events.
**
** Description:
**      This routine is called to scan the event wait queue for LXB's 
**	waiting for signalled events and awaken them.
**
** Inputs:
**	none
** Outputs:
**	Returns:
**	    None
**	Exceptions:
**	    none
**
** Side Effects:
**	    Sessions awaiting currently signalled events
**	    are resumed.
**
** History:
**	07-Nov-1996 (jenjo02)
**	    Created.
**	    Caller must hold lgd_status_mutex.
**	19-Feb-1998 (jenjo02)
**	    Clear lxb_wait_lwq after removing LXB from the queue.
**      08-Dec-1999 (hanal04) Bug 97015 INGSRV 819
**	    (hanje04) X-Integ of 444135 from oping20. Changed to E_DMA495
**	    because E_DMA493 already exists in main.
**          If end_offset does not break us out of the while loop
**          then report E_DMA493 rather than go into a tight spin.
*/
static VOID
LG_resume_event(
)
{
    register LGD    *lgd = (LGD *)LGK_base.lgk_lgd_ptr;
    register LWQ    *lwq = &lgd->lgd_wait_event;
    register LXBQ   *lxbq;
    register LXBQ   *next_lxbq;
    register LXBQ   *prev_lxbq;
    register LXB    *lxb;
    SIZE_TYPE	    end_offset;
    SIZE_TYPE	    lxbq_offset;

    LG_WHERE("LG_resume_event")

    /*
    ** Scan the event wait queue for LXBs awaiting
    ** currently signalled events and awaken them.
    */
    end_offset = LGK_OFFSET_FROM_PTR(&lwq->lwq_lxbq.lxbq_next);
    lxbq_offset = lwq->lwq_lxbq.lxbq_next;

    while (lxbq_offset != end_offset)
    {
	lxbq = (LXBQ *)LGK_PTR_FROM_OFFSET(lxbq_offset);
	lxbq_offset = lxbq->lxbq_next;

	lxb = (LXB *)((char *)lxbq - CL_OFFSETOF(LXB,lxb_wait));

	/* Return current events to caller */
	lxb->lxb_events = lgd->lgd_status;
	
	/*
	** If LXB is interested in contrived events (those 
	** for which there is no directly equivalent lgd_status flag),
	** contrive them.
	*/
	if (lxb->lxb_event_mask & (LG_E_SBACKUP | LG_E_BACKUP_FAILURE))
	{
	    register LPD    *lpd;
	    register LDB    *ldb;

	    lpd = (LPD *)LGK_PTR_FROM_OFFSET(lxb->lxb_lpd);
	    ldb = (LDB *)LGK_PTR_FROM_OFFSET(lpd->lpd_ldb);
	    /*
	    ** Check for events that are specific to the clients database.
	    */
	    if (ldb->ldb_status & LDB_BACKUP)
	    {
		if (LG_mutex(SEM_EXCL, &ldb->ldb_mutex))
		    return;

		if (lgd->lgd_status & LGD_CKP_SBACKUP)
		{
		    if ((ldb->ldb_status & LDB_BACKUP) &&
			(ldb->ldb_status & LDB_STALL) &&
			(ldb->ldb_lxbo_count == 0))
		    {
			lxb->lxb_events |= LG_E_SBACKUP;
		    }
		}

		/*
		** Check for backup error status.
		** Note that there is no LGD status for this event.
		*/
		if ((ldb->ldb_status & LDB_BACKUP) &&
		    (ldb->ldb_status & LDB_CKPERROR))
		{
		    lxb->lxb_events |= LG_E_BACKUP_FAILURE;
		}

		(VOID)LG_unmutex(&ldb->ldb_mutex);
	    }
	}

	/*
	** If LXB is awaiting an event that's arrived,
	** remove it from the event wait queue and
	** wake it up.
	*/
	if (lxb->lxb_events & lxb->lxb_event_mask)
	{
	    /*  Remove from LXBQ. */
	    next_lxbq = (LXBQ *)LGK_PTR_FROM_OFFSET(lxbq->lxbq_next);
	    prev_lxbq = (LXBQ *)LGK_PTR_FROM_OFFSET(lxbq->lxbq_prev);
	    next_lxbq->lxbq_prev = lxbq->lxbq_prev;
	    prev_lxbq->lxbq_next = lxbq->lxbq_next;
	    lwq->lwq_count--;

            /* HORDA03 - lxb_status cannot be changed by the LG_cancel
            **           as LG_cancel requires the lgd_status_mutex to
            **           remove the lxb from the wait queue. Can't take
            **           the lxb_mutex as this may cause a Semaphore
            **           deadlock.
            */
	    lxb->lxb_status &= ~LXB_WAIT;
	    lxb->lxb_wait_reason = 0;
	    lxb->lxb_wait_lwq = 0;

	    LGcp_resume(&lxb->lxb_cpid);
	}
    }

    return;
}

/*{
** Name: LG_resume	- Resume suspended LXB(s).
**
** Description:
**	This routine is called to awaken all LXBs on a 
**	given wait queue.
**
**	If the wait queue is the lfb_wait_buffer (free-buffer wait)
**	queue, the *caller* has to take and release lfb_fq_mutex
**	around this call.  For other wait queues, we'll use the
**	mutex embedded into the LWQ internally.
**
**	See LG_suspend for a more complete discussion of mutexing
**	and the proper use of the various wait queues.
**
** Inputs:
**      lwq				The queue of waiter to wakeup.
**	lfb				The log file block if and only if
**					we're waking free-buffer waiters.
**					Otherwise, null.
**
** Outputs:
**	Returns:
**	    None
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable logging and locking system.
**	22-Jan-1996 (jenjo02)
**	    Lock wait queue mutex.
**	21-Feb-1996 (jenjo02)
**	    Resume LXB only if it's actually waiting. Testing kept
**	    running into the situation whereby an LXB was queued
**	    on an lbb_wait queue, wanting notification when the
**	    buffer hit the disk. In the meantime, the thread
**	    continued processing, but when the i/o slave  finished the i/o,
**	    it resumed the already running thread. In this instance,
**	    DI was trying to open a file, calling CScause_event()
**	    to induce a slave to actually do the open, and CSsuspend()-ing
**	    until the slave completed its open work. In the meantime,
**	    LGresume was being called from RCP on write completion,
**	    CSresume()-ing the DI path and causing CSsuspend() to
**	    think the event (the slave's open) had already completed,
**	    manifesting itself in an "file open error".
**	05-May-1998 (jenjo02)
**	    Lock the LXB before checking lxb_status to avoid the odd
**	    race condition.
**	19-Feb-1999 (jenjo02)
**	    Clear lxb_wait_lwq after removing LXB from wait queue. 
**      08-Dec-1999 (hanal04) Bug 97015 INGSRV 819
**          (hanje04) X-Integ of 444135 from oping20. Changed to E_DMA495
**          because E_DMA493 already exists in main.
**          If end_offset does not break us out of the while loop
**          then report E_DMA493 rather than go into a tight spin.
**	13-Aug-2001 (jenjo02)
**	    Resume only as many threads as there are free buffers,
**	    if resuming for free buffers...
**	18-Sep-2001 (jenjo02)
**	    Removed unneeded mutexing of LXBs on wait queue.
**	20-Sep-2006 (kschendel)
**	    Free-buffer wait queue is now protected by LFB fq mutex,
**	    and the LFB is passed in for that case.
**	22-Oct-2008 (jonj)
**	    BUG 121113: Back out 13-Aug-2001 change to resume only as 
**	    many threads as there are free buffers. If one of the 
**	    un-resumed threads is the recovery thread (dm0l_bcp, for
**	    example) everyone may lock up waiting for either a free
**	    buffer or on a BCPSTALL.
*/
VOID
LG_resume(
    LWQ	    *lwq,
    LFB	    *lfb)
{
    register LGD    *lgd = (LGD *)LGK_base.lgk_lgd_ptr;
    register LXBQ   *lxbq;
    register LXBQ   *next_lxbq;
    register LXBQ   *prev_lxbq;
    register LXB    *lxb;
    SIZE_TYPE	    end_offset;


    LG_WHERE("LG_resume")

    /*	Remove LXB's from wait queue and restart. */

    if (lfb == NULL)
	LG_mutex(SEM_EXCL, &lwq->lwq_mutex);

    end_offset = LGK_OFFSET_FROM_PTR(&lwq->lwq_lxbq.lxbq_next);

    while (lwq->lwq_lxbq.lxbq_next != end_offset)
    {
	/*  Get the LXBQ. */

	lxbq = (LXBQ *)LGK_PTR_FROM_OFFSET(lwq->lwq_lxbq.lxbq_next);

	/*  Remove from LXBQ. */
	next_lxbq = (LXBQ *)LGK_PTR_FROM_OFFSET(lxbq->lxbq_next);
	prev_lxbq = (LXBQ *)LGK_PTR_FROM_OFFSET(lxbq->lxbq_prev);
	next_lxbq->lxbq_prev = lxbq->lxbq_prev;
	prev_lxbq->lxbq_next = lxbq->lxbq_next;
	lwq->lwq_count--;

	/*  Calculate LXB address. */

	lxb = (LXB *)((char *)lxbq - CL_OFFSETOF(LXB,lxb_wait));

	/* This check should not be needed as LXB must be waiting */
	if (lxb->lxb_status & LXB_WAIT)
	{
	    lxb->lxb_status &= ~LXB_WAIT;
	    lxb->lxb_wait_reason = 0;
	    lxb->lxb_wait_lwq = 0;
	    LGcp_resume(&lxb->lxb_cpid);
	}

	/* Resume 'em all and let them sort it out */
    }

    if (lfb == NULL)
	(VOID)LG_unmutex(&lwq->lwq_mutex);

    return;
}

/*{
** Name: LG_suspend	- Suspend caller.
**
** Description:
**      This routine marks the caller as suspended.
**	The LXB is placed onto the given LWQ.
**
**	In general, the caller ought to be holding some sort of mutex
**	that blocks resumes on the same wait queue.  It's necessary
**	to make sure that the wait condition isn't fully cleared before
**	the LXB makes it onto the wait queue, because then nobody will
**	be left to resume this LXB once it goes onto the wait queue.
**	In some cases, the necessary mutex is sufficiently global
**	that we don't need the LWQ mutex;  for instance, this is the
**	case for the wait-for-free-buffers queue.  In other cases,
**	the resume-blocking mutex is less global in scope than the
**	wait queue, so we need the wait queue mutex.  An example of
**	the latter is the MINI wait, resume-protected by the SHARED
**	LXB mutex, but the mini-wait queue is global to the lgd.
**
**	Currently known wait queues and resume-protection mutexes:
**	- The LFB's wait-for-free-buffers queue needs the lfb_fq_mutex
**	- The LGD mini-stall queue needs the SHARED LXB mutex
**	- The LGD wait-stall queue for BCPSTALL or LOGFULL needs the lgd mutex
**	- The (same) LGD wait-stall queue for CKPDB stall needs the
**	  database's LDB mutex
**	- The LGD open-event queue needs the LDB mutex
**
**	At present, the LFB buffer-wait queue is the only one where we
**	can omit taking the LWQ mutex.  The lfb_fq_mutex fully protects
**	the LWQ in that case.
**
** Inputs:
**      lxb                             The LXB to suspend.
**      lwq				An LWQ to queue the LXB to.
**      indirect_stat_addr		The address to store IO return status.
**
** Outputs:
**	Returns:
**	    None
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable logging and locking system.
**	22-Jan-1996 (jenjo02)
**	    LXB mutex must be held on entry.
**	    Wait queue mutex blocks while queue is being updated.
**	    lxb_wait_lwq defined to point to head of wait queue
**	    (LWQ) so that when the LXB is removed from the queue
**	    we can find the LWQ, lock it, and decrement its count.
**	19-Apr-1996 (jenjo02)
**	    Add new waiters to the end of the queue instead of
**	    the front.
**	20-Sep-2006 (kschendel)
**	    (Caller must) protect the buffer wait queue with the
**	    lfb_fq_mutex, instead of the usual LWQ mutex.
*/
VOID
LG_suspend(
register LXB        *lxb,
register LWQ	    *lwq,
STATUS	    	    *indirect_stat_addr)
{
    LFB		    *lfb;
    register LGD    *lgd = (LGD *)LGK_base.lgk_lgd_ptr;
    register LPD    *lpd;
    register LPB    *lpb;
    register LDB    *ldb;
    LXBQ	    *prev_lxbq;
    bool	    waiting_for_buffers;

    LG_WHERE("LG_suspend")

    CSget_cpid(&lxb->lxb_cpid);

    lpd = (LPD *)LGK_PTR_FROM_OFFSET(lxb->lxb_lpd);
    ldb = (LDB *)LGK_PTR_FROM_OFFSET(lpd->lpd_ldb);
    lpb = (LPB *)LGK_PTR_FROM_OFFSET(lpd->lpd_lpb);
    lfb = (LFB *)LGK_PTR_FROM_OFFSET(lxb->lxb_lfb_offset);
    waiting_for_buffers = (lwq == &lfb->lfb_wait_buffer);

    /*  Change various counters. */

    lgd->lgd_stat.wait[LG_WAIT_ALL]++;
    lgd->lgd_stat.wait[lxb->lxb_wait_reason]++;
    lxb->lxb_stat.wait++;
    ldb->ldb_stat.wait++;
    lpb->lpb_stat.wait++;

    /*	Queue to the tail of some wait queue. */

    if (! waiting_for_buffers)
    {
	if (LG_mutex(SEM_EXCL, &lwq->lwq_mutex))
	{
	    TRdisplay("LG_mutex failed in LG_suspend\n");
	    return;
	}
    }

    lxb->lxb_wait.lxbq_prev = lwq->lwq_lxbq.lxbq_prev;
    lxb->lxb_wait.lxbq_next = LGK_OFFSET_FROM_PTR(&lwq->lwq_lxbq.lxbq_next);
    prev_lxbq = (LXBQ *)LGK_PTR_FROM_OFFSET(lwq->lwq_lxbq.lxbq_prev);
    prev_lxbq->lxbq_next =
	lwq->lwq_lxbq.lxbq_prev = LGK_OFFSET_FROM_PTR(&lxb->lxb_wait);

    lxb->lxb_wait_lwq = LGK_OFFSET_FROM_PTR(lwq);

    lwq->lwq_count++;

    lxb->lxb_status |= LXB_WAIT;

    if (! waiting_for_buffers)
	(VOID)LG_unmutex(&lwq->lwq_mutex);

    /*
    ** Provide a pointer to the wait reason literal for CS_suspend
    */
    lxb->lxb_wait_reason_string = wait_reason[lxb->lxb_wait_reason];

    return;
}

/*{
** Name: LG_resume_lbb	- Resume LXBs on log buffer's wait queue
**
** Description:
**	This routine is called to awaken all LXBs on a 
**	log buffer's wait queue.
**
**	Log buffer I/O waits can be handled more expeditiously
**	than other waits. Because of the mutexing mechanism on
**	buffers, the wait queue does not also need to be mutexed.
**
** Inputs:
**	lxb				The thread doing this resume.
**      lbb				The log buffer with waiters.
**	status				The event completion status.
**
**	The calling thread must hold the LBB's lbb_mutex
**
** Outputs:
**	Returns:
**	    None
**	Exceptions:
**	    none
**
** Side Effects:
**	    Threads are resumed.
**
** History:
**	19-Feb-1999 (jenjo02)
**	    Written.
**      08-Dec-1999 (hanal04) Bug 97015 INGSRV 819
**	    (hanje04) X-Integ of 444135 from oping20. Changed to E_DMA495
**	    because E_DMA493 already exists in main.
**          If end_offset does not break us out of the while loop
**          then report E_DMA493 rather than go into a tight spin.
**	17-Mar-2000 (jenjo02)
**	    Cleaner protocol rules in LGwrite/LGforce guarantee 
**	    that waiting LXBs don't need mutexing here.
**	13-Nov-2002 (horda03)
**	    Don't resume if the LBB is nolonger in a wait state.
*/
VOID
LG_resume_lbb(
LXB	    *lxb,
LBB	    *lbb,
i4 	    status)
{
    register LGD    *lgd = (LGD *)LGK_base.lgk_lgd_ptr;
    register LPD    *lpd;
    register LPB    *lpb;
    register LDB    *ldb;
    register LXBQ   *lxbq;
    register LXBQ   *next_lxbq;
    register LXBQ   *prev_lxbq;
    register LXB    *wait_lxb;
    SIZE_TYPE	    end_offset;


    LG_WHERE("LG_resume_lbb")

    /* LBB's lbb_mutex must be locked */

    /*	Remove LXBs from wait queue and restart. */

    end_offset = LGK_OFFSET_FROM_PTR(&lbb->lbb_wait);

    while (lbb->lbb_wait.lxbq_next != end_offset)
    {
	/*  Get the LXBQ. */

	lxbq = (LXBQ *)LGK_PTR_FROM_OFFSET(lbb->lbb_wait.lxbq_next);

	/*  Calculate LXB address. */

	wait_lxb = (LXB *)((char *)lxbq - CL_OFFSETOF(LXB,lxb_wait));

        /* Only remove from the LXBQ and resume the session if the
        ** lxb is still waiting. Bug 105321
        */
        if (wait_lxb->lxb_status & LXB_WAIT)
        {
	   /*  Remove from LXBQ. */
	   next_lxbq = (LXBQ *)LGK_PTR_FROM_OFFSET(lxbq->lxbq_next);
	   prev_lxbq = (LXBQ *)LGK_PTR_FROM_OFFSET(lxbq->lxbq_prev);
	   next_lxbq->lxbq_prev = lxbq->lxbq_prev;
	   prev_lxbq->lxbq_next = lxbq->lxbq_next;
	   lbb->lbb_wait_count--;

	   /* Increment actual wait stats */

	   lpd = (LPD *)LGK_PTR_FROM_OFFSET(wait_lxb->lxb_lpd);
	   ldb = (LDB *)LGK_PTR_FROM_OFFSET(lpd->lpd_ldb);
	   lpb = (LPB *)LGK_PTR_FROM_OFFSET(lpd->lpd_lpb);

	   lgd->lgd_stat.wait[LG_WAIT_ALL]++;
	   lgd->lgd_stat.wait[wait_lxb->lxb_wait_reason]++;
	   wait_lxb->lxb_stat.wait++;
	   ldb->ldb_stat.wait++;
	   lpb->lpb_stat.wait++;

	   wait_lxb->lxb_status &= ~(LXB_WAIT | LXB_WAIT_LBB);
	   wait_lxb->lxb_wait_reason = 0;
	   wait_lxb->lxb_wait_lwq = 0;

	   /* Resume the waiting thread */
	   LGcp_resume(&wait_lxb->lxb_cpid);
        }
        else
        {
#ifdef VMS
           TRdisplay("%@ LG_resume_lbb. Didn't resume PID %08x, SIB %08x\n",
#else
           TRdisplay("%@ LG_resume_lbb. Didn't resume PID %d, SIB %08x\n",
#endif
                   wait_lxb->lxb_cpid.pid, wait_lxb->lxb_cpid.sid);
        }
    }

    return;
}

/*{
** Name: LG_suspend_lbb	- Attach LXB to buffer wait queue.
**
** Description:
**      This function inserts an LXB onto a log buffer's wait queue.
**
**	Log buffer I/O waits can be handled more expeditiously
**	than other waits. Because of the mutexing mechanism on
**	buffers, the wait queue does not also need to be mutexed.
**
** Inputs:
**      lxb                             The LXB to suspend.
**      lbb				An LBB to queue the LXB to.
**      indirect_stat_addr		The address to store IO return status.
**
** Outputs:
**	Returns:
**	    None
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	19-Feb-1999 (jenjo02)
**	    Written.
*/
VOID
LG_suspend_lbb(
register LXB        *lxb,
	 LBB	    *lbb,
STATUS	    	    *indirect_stat_addr)
{
    LXBQ	    *prev_lxbq;

    LG_WHERE("LG_suspend_lbb")

    /* It's presumed that this LXB and LBB mutexes are locked */

    CSget_cpid(&lxb->lxb_cpid);

    /*	Queue to the tail of log buffer's wait queue */

    lxb->lxb_wait.lxbq_prev = lbb->lbb_wait.lxbq_prev;
    lxb->lxb_wait.lxbq_next = LGK_OFFSET_FROM_PTR(&lbb->lbb_wait);
    prev_lxbq = (LXBQ *)LGK_PTR_FROM_OFFSET(lbb->lbb_wait.lxbq_prev);
    prev_lxbq->lxbq_next =
	lbb->lbb_wait.lxbq_prev = LGK_OFFSET_FROM_PTR(&lxb->lxb_wait);

    /* Point wait_lwq to buffer */
    lxb->lxb_wait_lwq = LGK_OFFSET_FROM_PTR(lbb);

    lbb->lbb_wait_count++;

    lxb->lxb_status |= (LXB_WAIT | LXB_WAIT_LBB);

    /*
    ** Provide a pointer to the wait reason literal for CS_suspend
    */
    lxb->lxb_wait_reason_string = wait_reason[lxb->lxb_wait_reason];

    return;
}

/*{
** Name: LG_cancel	- Cancel event wait for LG return
**
** Description:
**	If thread waits for any LG event or condition and is interrupted
**	or cancelled while waiting for the request to complete, this
**	routine is called to remove the LXB from the appropriate
**	wait queue.
**
**	If the event has already completed, then return SS$_CANCELGRANT
**
** Inputs:
**	lxb				- Waiting thread's LXB.
**
** Outputs:
**	Returns:
**	    OK				- wait cancelled
**	    LG_CANCELGRANT		- wait completed before cancelled
**	    LG_BADPARAM		- bad lx_id given to routine
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable logging and locking system.
**	18-oct-1993 (rmuth)
**	    prototype.
**	25-Jan-1996 (jenjo02)
**	    Mutex granularity project. New semaphores replace
**	    singular lgd_mutex.
**	    lxb_wait_lwq defined to point to head of wait queue
**	    (LWQ) so that we can lock its semaphore and 
**	    decrement its count.
**	10-Nov-1996 (jenjo02)
**	    Generalized to cancel wait on any lwq.
**	19-Feb-1998 (jenjo02)
**	    Clear lxb_wait_lwq after removing LXB from the queue.
*/
/* ARGSUSED */
static
STATUS
LG_cancel(
    i4		    flags,
    LG_ID	    *lx_id)
{
    register LGD    *lgd = (LGD *)LGK_base.lgk_lgd_ptr;
    register LXBQ   *lxbq;
    register LXB    *lxb;
    LXBQ	    *next_lxbq;
    LXBQ	    *prev_lxbq;
    register LWQ    *lwq;
    SIZE_TYPE	    *lxbb_table;
    i4	    err_code;
    STATUS	    status;

    LG_WHERE("LG_cancel")

    /*	Check the lx_id. */
    if (lx_id->id_id == 0 || (i4)lx_id->id_id > lgd->lgd_lxbb_count)
    {
	uleFormat(NULL, E_DMA453_LG_CANCEL_BAD_ID, (CL_ERR_DESC *)NULL,
		    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 2,
		    0, lx_id->id_id, 0, lgd->lgd_lxbb_count);
	return (LG_BADPARAM);
    }

    lxbb_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lgd->lgd_lxbb_table);
    lxb = (LXB *)LGK_PTR_FROM_OFFSET(lxbb_table[lx_id->id_id]);

    if (lxb->lxb_type != LXB_TYPE ||
	lxb->lxb_id.id_instance != lx_id->id_instance)
    {
	uleFormat(NULL, E_DMA454_LG_CANCEL_BAD_XACT, (CL_ERR_DESC *)NULL,
		    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
		    0, lxb->lxb_type, 0, lxb->lxb_id.id_instance,
		    0, lx_id->id_instance);
	return (LG_BADPARAM);
    }

    if (status = LG_mutex(SEM_EXCL, &lxb->lxb_mutex))
	return(status);

    /*
    ** If lxb is not on wait queue, then the LG request has already
    ** been granted - return CANCELGRANT.
    */
    if ((lxb->lxb_status & LXB_WAIT) == 0)
    {
	(VOID)LG_unmutex(&lxb->lxb_mutex);
	return (LG_CANCELGRANT);
    }

    /* Take lxb off of the appropriate wait queue */
    lxbq = &lxb->lxb_wait;
    lwq  = (LWQ *)LGK_PTR_FROM_OFFSET(lxb->lxb_wait_lwq);
    lxb->lxb_wait_lwq = 0;

    /*
    ** event wait queue is protected by lgd_mutex
    */
    if (lwq == (LWQ *)&lgd->lgd_wait_event)
    {
	if (status = LG_mutex(SEM_EXCL, &lgd->lgd_mutex))
	    return(status);
    }
    else
    {
	if (status = LG_mutex(SEM_EXCL, &lwq->lwq_mutex))
	    return(status);
    }

    /* HORDA03 - Although we have the LXB mutex, the lxb_status may
    **           have changed because of a call to LG_resume_lbb.
    **           The LG_resume_lbb doesn't take the lxb_mutex, it
    **           relies on the lgd_status_mutex - becasue this mutex
    **           is already held, it can't take the lxb_mutex as a
    **           semaphore deadlock may occur.
    **           By now, we've taken all the mutexes we need to protect
    **           the cancelling og an LGevent wait, so check that the
    **           event is still waiting.
    */

    if (lxb->lxb_status & LXB_WAIT)
    {
       next_lxbq = (LXBQ *)LGK_PTR_FROM_OFFSET(lxbq->lxbq_next);
       prev_lxbq = (LXBQ *)LGK_PTR_FROM_OFFSET(lxbq->lxbq_prev);
       next_lxbq->lxbq_prev = lxbq->lxbq_prev;
       prev_lxbq->lxbq_next = lxbq->lxbq_next;

       lwq->lwq_count--;
       lxb->lxb_status &= ~LXB_WAIT;

       status = OK;
    }
    else
    {
#ifdef VMS
       TRdisplay("%@ LG_cancel. LGevent already granted for PID %08x, SIB %08x\n",
#else
       TRdisplay("%@ LG_cancel. LGevent already granted for PID %d, SIB %08x\n",
#endif
                 lxb->lxb_cpid.pid, lxb->lxb_cpid.sid);

        status = LG_CANCELGRANT;
    }

    if (lwq == (LWQ *)&lgd->lgd_wait_event)
	LG_unmutex(&lgd->lgd_mutex);
    else
	LG_unmutex(&lwq->lwq_mutex);

    (VOID)LG_unmutex(&lxb->lxb_mutex);

    return (status);
}
