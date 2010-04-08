/*
**Copyright (c) 2004 Ingres Corporation
**
*/

/*
NO_OPTIM = r64_us5 rs4_us5
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <di.h>
#include    <er.h>
#include    <pc.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <tr.h>
#include    <dmf.h>
#include    <ulf.h>
#include    <dm.h>
#include    <lg.h>
#include    <lgdef.h>
#include    <lgdstat.h>
#include    <lgkdef.h>

/**
**
**  Name: LGEND.C - Implements the LGend function of the logging system
**
**  Description:
**	This module contains the code which implements LGend.
**	
**	    LGend -- End this transaction in the logging system
**
**  History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable logging and locking system.
**	16-oct-1992 (jnash) for rogerk
**	    Reduced logging project.  Removed references to obsolete 
**	    lgd->lgd_n_servers counter.  Change last_fc_server() refs
**	    to last_cp_server().
**	04-jan-1993 (jnash)
**	    Reduced logging project.  Free reserved log file space prior
**	    to destroying LXB.
**	15-mar-1993 (rogerk)
**	    Reduced Logging - Phase IV:  Removed EBACKUP database state.
**	17-mar-1993 (rogerk)
**	    Moved lgd status values to lgdstat.h so that they are visible
**	    by callers of LGshow(LG_S_LGSTS).
**	26-apr-1993 (andys/bryanp)
**	    Cluster 6.5 Project I
**		Renamed stucture members of LG_LA to new names. This means
**		    replacing lga_high with la_sequence, and lga_low with
**		    la_offset.
**		When we signal the backup to start, we record the current EOF in
**		    the ldb_sbackup field of the LDB. This will be used by
**		    higher-level DMF protocols to decide whether a page has
**		    been changed since the backup began. DMF, however, needs
**		    an LSN, not an LG_LA, to make this check, since it wishes
**		    to compare a log sequence number on a page against the LSN
**		    of the start of the backup. Therefore, we also set
**		    ldb->ldb_sback_lsn to be the most recent LSN that we have
**		    assigned.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (bryanp)
**	    When ending a logwriter thread, if it wasn't idle, perform its
**		assigned task(s) before removing it entirely.
**	    Include <tr.h>
**	26-jul-1993 (rogerk)
**	    Changed journal and dump window tracking in the logging system.
**	18-oct-1993 (rogerk)
**	    Removed obsolete LG_SESSION_ABORT handling.
**	    Took out descriptions of old unsupported flags values.
**	    Changed cleanup of recovered transactions.  Took out use of LG_RCP
**	    flag which used to indicate to clean up control blocks linked to
**	    the completed lxb.  We now only clean up parent control blocks
**	    for transactions marked orphaned.
**	18-oct-1993 (rogerk)
**	    Removed unused lgd_dmu_cnt, lxb_dmu_cnt fields.
**      31-jan-1994 (mikem)
**          bug #56478
**          0 logwriter threads caused installation to hang, fixed by adding
**          special case code to handle synchonous "in-line" write's vs.
**          asynchronous writes.  The problem was that code that scheduled
**          "synchronous" writes with direct calls to "write_block" did not
**          handle correctly the mutex being released during the call.  In the
**          synchronous case one must hold the LG mutex across the DIwrite()
**          call.  Added "release_mutex" argument to LG_do_write().
**	31-jan-1994 (bryanp) B56560
**	    When a logwriter is trying to clean up its unwritten blocks, if a
**		block cannot be written, the logwriter should return without
**		finishing the context cleanup. This allows some other process's
**		check-dead thread to give it a try.
**	31-jan-1994 (jnash)
**	    Fix lint detected problems: remove unused variables in 
**	    LG_end_tran(), initialize sys_err. 
**	05-Jan-1995 (jenjo02)
**	    Mutex granularity project. Semaphores must now be explicitly
**	    named in calls to LKMUTEX.C
**	18-Jan-1996 (jenjo02)
**          lbk_table replaced with lfbb_table (LFBs) and ldbb_table
**          (LDBs) to ensure that semaphores now embedded in those
**          structures are not corrupted. Similarly, sbk_table 
**          replaced with lpbb_table (LPBs), lxbb_table (LXBs), and
**          lpdb_table (LPDs).
**	27-Feb-1996 (jenjo02)
**	    Rearranged code in LG_end_tran() so that first thing done
**	    is to remove LXB from its queues. This to avoid contention
**	    with other threads which may be walking those queues and
**	    are forced to wait on the lxb_mutex of the LXB being ended.
**	05-Sep-1996 (jenjo02)
**	    Removed "release_mutex" argument from LG_do_write().
**	27-Sep-1996 (jenjo02)
**	    Moved ldb_lxbo_count-- to where it's protected by the ldb_mutex.
**	13-Dec-1996 (jenjo02)
**	    ldb_lxbo_count now counts the number of non-READONLY protected 
**	    transactions, active or inactive.
**	15-Jan-1997 (jenjo02)
**	    Fixed bad line of code in LG_end_tran which failed to detect
**	    orphaned LXBs.
**	11-Nov-1997 (jenjo02)
**	    Redefined ldb_lxbo_count to include only active, protected
**	    transactions.
**      08-Dec-1997 (hanch04)
**        New block field in LG_LA for support of logs > 2 gig
**	17-Dec-1997 (jenjo02)
**	  o LG_do_write() loops while lxb_assigned_buffer, so there's no
**	    need to also do that here.
**	  o Replaced idle LogWriter queue with a static queue of all
**	    LogWriter threads.
**	24-Aug-1998 (jenjo02)
**	    When ending an active transaction, remove it from the
**	    active transaction hash queue.
**	24-Aug-1999 (jenjo02)
**	    Dropped lgd_status_mutex, lfb_cp_mutex, use lgd_mutex instead.
**	    Renamed lgd_lxb_aq_mutex to lgd_lxb_q_mutex; there is no longer
**	    an "inactive" LXB queue; only active, protected txns appear
**	    on the LXB queue.
**	15-Dec-1999 (jenjo02)
**	    Added support for SHARED transactions.
**	02-Feb-2000 (jenjo02)
**	    Fixed 2 typos - LK_mutex/unmutex should be LG_mutex/unmutex -
**	    an innocuous goof.
**	14-Mar-2000 (jenjo02)
**	    Removed static LG_end() function and the unnecessary level
**	    of indirection it induced. LGend() now does all the work.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      12-Feb-2003 (hweho01)
**          Turned off optimizer for AIX hybrid build by using 
**          the 64-bit configuration string (r64_us5), because the    
**          error happens only with the 64-bit compiler option.  
**      10-May-2003 (hweho01)
**          Still need to turn off optimization for AIX 32-bit build.
**          Got error "E_DMA415_LGEND_BAD_ID" during creating iidbdb.
**          Compiler : VisualAge 5.023.
**      11-Jun-2004 (horda03) Bug 112382/INGSRV2838
**          Ensure that the current LBB is stil "CURRENT" after
**          the lbb_mutex is acquired. If not, re-acquire.
**      7-oct-2004 (thaju02)
**          Use SIZE_TYPE to allow memory pools > 2Gig.
**	20-Sep-2006 (kschendel)
**	    LG-resume call changed, update here.
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
** Forward declarations of static functions:
*/

/*{
** Name: LGend	- End transaction.
**
** Description:
**      Inform the logging system that this transaction has completed.
**
** Inputs:
**      lx_id                           Transaction identifier.
**	flag				0 or LG_START.
**
**					0 : normal termination of the
**					    transaction. i.e remove
**					    lxb from the logging system.
**
**					LG_START : Logging system is comming
**					online.  Check for logspace that can
**					be freed up after having recovered
**					incomplete transactions in restart
**					recovery.
**					
** Outputs:
**      sys_err                         Reason for error return status.
**	Returns:
**	    OK
**	    LG_BADPARAM
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable logging and locking system.
**	16-oct-1992 (jnash) for rogerk
**	    Reduced logging project.  Removed references to obsolete 
**	    lgd->lgd_n_servers counter.
**	18-oct-1993 (rogerk)
**	    Removed obsolete LG_SESSION_ABORT handling.
**	24-Jan-1996 (jenjo02)
**	    Mutex granularity project changes.
**	24-Aug-1999 (jenjo02)
**	    Dropped lgd_status_mutex, lfb_cp_mutex, use lgd_mutex instead.
**	    Renamed lgd_lxb_aq_mutex to lgd_lxb_q_mutex; there is no longer
**	    an "inactive" LXB queue; only active, protected txns appear
**	    on the LXB queue.
*/
STATUS
LGend(
LG_LXID             external_lx_id,
i4		    flag,
CL_ERR_DESC	    *sys_err)
{
    register LGD        *lgd = (LGD *)LGK_base.lgk_lgd_ptr;
    register LXB	*lxb;
    register LFB	*lfb;
    LXB			*next_lxb;
    SIZE_TYPE		*lxbb_table;
    i4			err_code;
    STATUS		status;
    LG_I4ID_TO_ID	lx_id;

    LG_WHERE("LGend")

    CL_CLEAR_ERR(sys_err);

    /*	Check the lx_id. */

    lx_id.id_i4id = external_lx_id;
    if (lx_id.id_lgid.id_id == 0 || (i4)lx_id.id_lgid.id_id > lgd->lgd_lxbb_count)
    {
	uleFormat(NULL, E_DMA415_LGEND_BAD_ID, (CL_ERR_DESC *)NULL,
		    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 2,
		    0, lx_id.id_lgid.id_id, 0, lgd->lgd_lxbb_count);
	return (LG_BADPARAM);
    }

    lxbb_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lgd->lgd_lxbb_table);
    lxb = (LXB *)LGK_PTR_FROM_OFFSET(lxbb_table[lx_id.id_lgid.id_id]);

    if (lxb->lxb_type != LXB_TYPE ||
	lxb->lxb_id.id_instance != lx_id.id_lgid.id_instance)
    {
	uleFormat(NULL, E_DMA416_LGEND_BAD_XACT, (CL_ERR_DESC *)NULL,
		    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
		    0, lxb->lxb_type, 0, lxb->lxb_id.id_instance,
		    0, lx_id.id_lgid.id_instance);
	return (LG_BADPARAM);
    }
    lfb = (LFB *)LGK_PTR_FROM_OFFSET(lxb->lxb_lfb_offset);

    if (status = LG_mutex(SEM_EXCL, &lxb->lxb_mutex))
	return(status);

    /*
    ** Handle RCP startup protocol.
    **
    ** At the end of rcp startup, we call LGend with the LG_START flag.
    ** Its not clear why this is needed and in any case its pretty clear
    ** that it does not belong in this routine.
    */
    if (flag & LG_START)
    {
	    /*
	    ** RCP startup: move the bof forward if possible and check for
	    ** force abort and logfull cleanup.
	    **
	    ** Comment by rogerk:  I don't see why any of this is needed.
	    ** This is always the rcp rcp_lx_id that lgend is being called
	    ** with and in fact it is not being ended (we return just below).
	    ** I don't see how the rcp_lx_id xact can ever be listed as
	    ** LXB_FORCE_ABORT nor how we would be in LOGFULL during rcp
	    ** startup.  In any case, I did not remove the code since I
	    ** could not do testing to make sure it was OK.  But I think
	    ** that it is likely this stuff can be removed.
	    */

	    if (status = LG_mutex(SEM_EXCL, &lgd->lgd_mutex))
		return(status);

	    /*
	    ** Move the BOF forward if possible.  If there is anything to
	    ** archive then signal the archiver.
	    */
	    LG_compute_bof(lfb, (i4)0);

	    /*
	    ** If this was a force-abort transaction, then check if aborting
	    ** this transaction has fixed the force-abort condition.
	    ** If the new oldest transaction is in the same consistency point
	    ** as the transaction that was just force aborted, then we need
	    ** to force-abort the next transaction as well.
	    */
	    if (lxb->lxb_status & LXB_FORCE_ABORT ||
		lgd->lgd_status & LGD_LOGFULL)
	    {

		/*
		** If we are in logfull state, then this transaction ending may
		** have freed up space in the log file.  Wake up everybody 
		** waiting and let them try again.
		*/
		if (lgd->lgd_status & LGD_LOGFULL)
		{
		    lgd->lgd_status &= ~(LGD_LOGFULL);
		    LG_resume(&lgd->lgd_wait_stall, NULL);
		}

		if (lxb->lxb_status & LXB_FORCE_ABORT)
		{
		    if (lgd->lgd_lxb_next != LGK_OFFSET_FROM_PTR(&lgd->lgd_lxb_next))
		    {
			next_lxb = (LXB *)LGK_PTR_FROM_OFFSET(lgd->lgd_lxb_next);

			if (next_lxb->lxb_type == LXB_TYPE &&
			    LGA_EQ(&next_lxb->lxb_cp_lga, &lxb->lxb_cp_lga))
			{
			    /*
			    ** Select a victim to force-abort.  If it finds 
			    ** one, it will turn LGD_FORCE_ABORT status back on.
			    */
			    (VOID)LG_unmutex(&lgd->lgd_mutex);
			    LG_abort_oldest(lxb);
			    if (status = LG_mutex(SEM_EXCL, &lgd->lgd_mutex))
				return(status);
			}
		    }
		}
	    }
	    
	    (VOID)LG_unmutex(&lgd->lgd_mutex);
	    (VOID)LG_unmutex(&lxb->lxb_mutex);
	    return (OK);
    }

    /*	Common end transaction processing code. */

    LG_end_tran(lxb, flag);
    return (OK);
}

/*{
** Name: LG_end_tran - Remove the transaction context from the logging system.
**
** Description:
**	This routine removes the transaction context from the logging system.
**
** Inputs:
**	lxb				The transaction context block.
**	flag				LG_LOGFULL_COMMIT if the transaction
**					is to be retained.
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
**	04-jan-1993 (jnash)
**	    Reduced logging project.  Unreserve any remaining log file space 
**	    reserved by this transaction.
**	26-jul-1993 (bryanp)
**	    When ending a logwriter thread, if it wasn't idle, perform its
**	    assigned task(s) before removing it entirely.
**	26-jul-1993 (rogerk)
**	    Changed journal and dump window tracking in the logging system.
**	    Use new journal log address fields.
**	18-oct-1993 (rogerk)
**	    Changed cleanup of recovered transactions.  Took out use of LG_RCP
**	    flag which used to indicate to clean up control blocks linked to
**	    the completed lxb.  We now only clean up parent control blocks
**	    for transactions marked orphaned.  We depend on rcp protocols to
**	    ensure that control blocks owned by failed processes are cleaned
**	    up properly (through the LG_A_DBRECOVER call).  Changed to call
**	    LG_remove directly rather than duplicating that code here.
**	    Changed logspace reservation counters to work with unsigned logic.
**	    Added logspace reservation error message.
**	18-oct-1993 (rogerk)
**	    Removed unused lgd_dmu_cnt, lxb_dmu_cnt fields.
**      31-jan-1994 (mikem)
**          bug #56478
**          0 logwriter threads caused installation to hang, fixed by adding
**          special case code to handle synchonous "in-line" write's vs.
**          asynchronous writes.  The problem was that code that scheduled
**          "synchronous" writes with direct calls to "write_block" did not
**          handle correctly the mutex being released during the call.  In the
**          synchronous case one must hold the LG mutex across the DIwrite()
**          call.  Added "release_mutex" argument to LG_do_write().
**	31-jan-1994 (bryanp) B56560
**	    When a logwriter is trying to clean up its unwritten blocks, if a
**		block cannot be written, the logwriter should return without
**		finishing the context cleanup. This allows some other process's
**		check-dead thread to give it a try.
**	31-jan-1994 (jnash)
**	    Fix lint detected problems: remove unused prev_lpd, next_lpd, 
**	    prev_lpb, next_lpb.
**	22-Jan-1996 (jenjo02)
**	    lxb_mutex must be held on entry to this function.
**	27-Feb-1996 (jenjo02)
**	    Rearranged code in LG_end_tran() so that first thing done
**	    is to remove LXB from its queues. This to avoid contention
**	    with other threads which may be walking those queues and
**	    are forced to wait on the lxb_mutex of the LXB being ended.
**	26-Jun-1996 (jenjo02)
**	    Count only active protected txns in lgd_protect_count.
**	05-Sep-1996 (jenjo02)
**	    Removed "release_mutex" argument from LG_do_write().
**	15-Jan-1997 (jenjo02)
**	    Fixed bad line of code in LG_end_tran which failed to detect
**	    orphaned LXBs.
**	11-Nov-1997 (jenjo02)
**	    Redefined ldb_lxbo_count to include only active, protected
**	    transactions.
**	17-Dec-1997 (jenjo02)
**	  o LG_do_write() loops while lxb_assigned_buffer, so there's no
**	    need to also do that here.
**	  o Replaced idle LogWriter queue with a static queue of all
**	    LogWriter threads.
**	24-Aug-1998 (jenjo02)
**	    When ending an active transaction, remove it from the
**	    active transaction hash queue.
**	24-Aug-1999 (jenjo02)
**	    Dropped lgd_status_mutex, lfb_cp_mutex, use lgd_mutex instead.
**	    Renamed lgd_lxb_aq_mutex to lgd_lxb_q_mutex; there is no longer
**	    an "inactive" LXB queue; only active, protected txns appear
**	    on the LXB queue.
**	15-Dec-1999 (jenjo02)
**	    Added support for SHARED transactions.
**	08-Apr-2004 (jenjo02)
**	    Support for BMINI/EMINI and mini-transaction
**	    waiters.
**	20-Apr-2004 (jenjo02)
**	    Added support for LOGFULL_COMMIT.
**	6-Jan-2006 (kschendel)
**	    Need to make reserved space totals u_i8 for large logs, fix here.
**	15-Mar-2006 (jenjo02)
**	    Init lxb_last_lxn as it now appears in logstat.
**	    lxb_wait_reason defines move to lg.h
**	21-Jun-2006 (jonj)
**	    Handle inverted LGend of XA transactions when LGend is issued
**	    by the RCP on SHARED LXB instead of HANDLEs.
**	28-Jul-2006 (jonj)
**	    Internally, use LG_RCP in flag to indicate the HANDLEs
**	    are being unwound inside-out (SHARED==>HANDLE).
**	28-Aug-2006 (jonj)
**	    Handle (new) LFD queue when ending XA transaction.
**	11-Sep-2006 (jonj)
**	    LOGFULL_COMMIT: Do not reset transaction and get
**	    a new tran_id until the shared transaction's
**	    ET has been written. Until that happens all
**	    other handles are stalled on LG_WAIT_COMMIT.
**	    When we've reset the transaction, wake up
**	    those waiters.
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Remove active LXB from LDB's ldb_active_lxbq.
**	    Maintain ldb_lgid_low, ldb_lgid_high.
*/
VOID
LG_end_tran(
LXB		*lxb,
i4		flag)
{
    register LGD        *lgd = (LGD *)LGK_base.lgk_lgd_ptr;
    register LPD	*lpd = (LPD *)LGK_PTR_FROM_OFFSET(lxb->lxb_lpd);
    register LPB	*lpb = (LPB *)LGK_PTR_FROM_OFFSET(lpd->lpd_lpb);
    register LDB	*ldb = (LDB *)LGK_PTR_FROM_OFFSET(lpd->lpd_ldb);
    register LFB	*lfb = (LFB *)LGK_PTR_FROM_OFFSET(lxb->lxb_lfb_offset);
    LXB                 *temp_lxb;
    LDB			*temp_ldb;
    LPD			*temp_lpd;
    LXB			*next_lxb;
    LXB			*prev_lxb;
    LXBQ		*next_lxbq;
    LXBQ		*prev_lxbq;
    LWQ			*lwq;
    i4			oldest_tran;
    i4			err_code;
    i4			lxbo_count;
    bool		orphaned_xact;
    STATUS		status;
    CL_ERR_DESC		sys_err;
    LBB			*current_lbb;
    LXB			*slxb;
    DI_IO		di;
    LFD			*lfd;
    LXBQ		*handle_lxbq;
    LXB			*handle_lxb;

    if (lxb->lxb_status & LXB_LOGWRITER_THREAD)
    {
	if (lxb->lxb_wait_reason != LG_WAIT_WRITEIO)
	{
	    /*
	    ** this logwriter was not idle, which means that somebody scheduled
	    ** a write for it. We can't just clean up the transaction; we must
	    ** uphold our duty to the buffer and carry out our assigned task.
	    **
	    ** Note that LG_do_write() will loop until there are no more
	    ** buffers to be written. This is because 
	    ** at the completion of the first write the logwriter may
	    ** attempt to reschedule itself for a subsequent write.
	    ** When there are no more, we'll return here to remove the
	    ** logwriter thread from the system.
	    **
	    ** If at some point we encounter an error writing out buffers, then
	    ** we abandon our attempt to clean up this logwriter thread and
	    ** just leave the context around for some other process's check-dead
	    ** thread to handle. (B56560)
	    */
	    if (lxb->lxb_assigned_buffer)
	    {
		status = LG_do_write(lxb, &sys_err);
		if (status)
		{
		    uleFormat(NULL, status, &sys_err, ULE_LOG,
				NULL, NULL, 0, NULL, &err_code, 0);
		    return;
		}
	    }
	}
	/*
	** Remove this LogWriter LXB from the queue of Logwriters and
	** decrement the count of known logwriter threads. 
	*/
	lxb->lxb_status &= ~(LXB_LOGWRITER_THREAD | LXB_WAIT);
	lxb->lxb_wait_reason = 0;

	lwq = &lgd->lgd_lwlxb;
	if (LG_mutex(SEM_EXCL, &lwq->lwq_mutex))
	    return;
	next_lxbq = (LXBQ *)LGK_PTR_FROM_OFFSET(lxb->lxb_wait.lxbq_next);
	prev_lxbq = (LXBQ *)LGK_PTR_FROM_OFFSET(lxb->lxb_wait.lxbq_prev);
	next_lxbq->lxbq_prev = lxb->lxb_wait.lxbq_prev;
	prev_lxbq->lxbq_next = lxb->lxb_wait.lxbq_next;
	lwq->lwq_count--;
	(VOID)LG_unmutex(&lwq->lwq_mutex);
    }

    /*
    **  Check for the oldest active transaction and
    **	dequeue from active queue.
    **  Only active, protected txns appear on the LXB queue.
    */

    /*
    ** Only force-abort txns can be "kept" 
    **
    ** LOGFULL_COMMIT may be indicated if a transaction
    ** is force-aborted for logfull but wants to continue.
    ** (SET SESSION WITH ON_LOGFULL = COMMIT | NOTIFY)
    **
    ** If the (shared) transaction has written it's ET
    ** we want to remove it from the active queue and place it
    ** last on the inactive queue as if has just begun;
    ** the log_id handle must remain the same as the 
    ** continuing transaction still refererences it,
    ** but we do want to acquire a new tran_id to 
    ** distinguish subsequent log activity.
    */
    if ( !(lxb->lxb_status & LXB_FORCE_ABORT) )
	flag &= ~LG_LOGFULL_COMMIT;
	

    oldest_tran = FALSE;
    if (lxb->lxb_status & LXB_ACTIVE)
    {
	if (LG_mutex(SEM_EXCL, &lgd->lgd_lxb_q_mutex))
	    return;
	if (lgd->lgd_lxb_next == LGK_OFFSET_FROM_PTR(lxb))
	    oldest_tran = TRUE;
	lgd->lgd_protect_count--;
	/* count one less active, protected txn for ckpdb */
	lxbo_count = --ldb->ldb_lxbo_count;
	/* Remove LXB from active txn hash queue */
	{
	    LXH	*next_lxh, *prev_lxh;

	    next_lxh = (LXH *)LGK_PTR_FROM_OFFSET(lxb->lxb_lxh.lxh_lxbq.lxbq_next);
	    prev_lxh = (LXH *)LGK_PTR_FROM_OFFSET(lxb->lxb_lxh.lxh_lxbq.lxbq_prev);

	    next_lxh->lxh_lxbq.lxbq_prev = lxb->lxb_lxh.lxh_lxbq.lxbq_prev;
	    prev_lxh->lxh_lxbq.lxbq_next = lxb->lxb_lxh.lxh_lxbq.lxbq_next;
	}
	/* Remove LXB from active queue */
	next_lxb = (LXB *)LGK_PTR_FROM_OFFSET(lxb->lxb_next);
	prev_lxb = (LXB *)LGK_PTR_FROM_OFFSET(lxb->lxb_prev);
	next_lxb->lxb_prev = lxb->lxb_prev;
	prev_lxb->lxb_next = lxb->lxb_next;

	(VOID)LG_unmutex(&lgd->lgd_lxb_q_mutex);
    }


    /*	Change various counters. */

    /* Don't count the SHARED LXB in stats */
    if ( (lxb->lxb_status & LXB_SHARED) == 0 )
    {
	lgd->lgd_stat.end++;
	lpb->lpb_stat.end++;
	lfb->lfb_stat.end++;
	ldb->ldb_stat.end++;
    }


    if (LG_mutex(SEM_EXCL, &ldb->ldb_mutex))
	return;

    /* Set xid slot "inactive" (0) in xid array */
    SET_XID_INACTIVE(lxb);

    /*
    ** Remove LXB from MVCC LDB's active queue, if still on it.
    ** LGwrite put the LXB on this queue when it did its
    ** first log write and removed it when it wrote its ET,
    ** so, assuming it wrote an ET, lxbq_next will be zero,
    ** but we're covering our bases...
    */
    if ( lxb->lxb_active_lxbq.lxbq_next != 0 )
    {
	next_lxbq = (LXBQ *)LGK_PTR_FROM_OFFSET(lxb->lxb_active_lxbq.lxbq_next);
	prev_lxbq = (LXBQ *)LGK_PTR_FROM_OFFSET(lxb->lxb_active_lxbq.lxbq_prev);
	next_lxbq->lxbq_prev = lxb->lxb_active_lxbq.lxbq_prev;
	prev_lxbq->lxbq_next = lxb->lxb_active_lxbq.lxbq_next;

	lxb->lxb_active_lxbq.lxbq_next =
	    lxb->lxb_active_lxbq.lxbq_prev = 0;
    }

    /*
    ** If this ending transaction was the lowest or highest
    ** open lgid know in this MVCC database, then find the new 
    ** lowest or highest known lgid, update ldb_lgid_low|high.
    **
    ** ldb_lgid_low and ldb_lgid_high are used by LGshow(LG_S_CRIB_?)
    ** to frame the xid_array portion involved with this 
    ** MVCC database.
    **
    ** Note that if there are no other open transactions in
    ** the DB, low/high will end as zero.
    */
    if ( ldb->ldb_status & LDB_MVCC &&
         lxb->lxb_id.id_id == ldb->ldb_lgid_low ||
         lxb->lxb_id.id_id == ldb->ldb_lgid_high )
    {
	SIZE_TYPE	*lxbb_table;
	LXB		*next_lxb, *prev_lxb;
	LPD		*next_lpd, *prev_lpd;
	i4		i;
	u_i2		old_low, old_high;

	lxbb_table = (SIZE_TYPE*)LGK_PTR_FROM_OFFSET(lgd->lgd_lxbb_table);


	old_low = ldb->ldb_lgid_low;
	old_high = ldb->ldb_lgid_high;

	/* If this LXB was lowest, search forward for a new lowest */
        if ( lxb->lxb_id.id_id == ldb->ldb_lgid_low )
	{
	    ldb->ldb_lgid_low = 0;
	    for ( i = lxb->lxb_id.id_id+1;
	          i <= old_high;
		  i++ )
	    {
		next_lxb = (LXB*)LGK_PTR_FROM_OFFSET(lxbb_table[i]);
		if ( next_lxb->lxb_type == LXB_TYPE )
		{
		    next_lpd = (LPD*)LGK_PTR_FROM_OFFSET(next_lxb->lxb_lpd);

		    if ( next_lpd && next_lpd->lpd_ldb == lpd->lpd_ldb )
		    {
			ldb->ldb_lgid_low = i;
			break;
		    }
		}
	    }
	}

	/* If this LXB was highest, search backward for a new highest */
        if ( lxb->lxb_id.id_id == ldb->ldb_lgid_high )
	{
	    ldb->ldb_lgid_high = 0;
	    for ( i = lxb->lxb_id.id_id-1;
	          i >= old_low;
		  i-- )
	    {
		prev_lxb = (LXB*)LGK_PTR_FROM_OFFSET(lxbb_table[i]);
		if ( prev_lxb->lxb_type == LXB_TYPE )
		{
		    prev_lpd = (LPD*)LGK_PTR_FROM_OFFSET(prev_lxb->lxb_lpd);

		    if ( prev_lpd && prev_lpd->lpd_ldb == lpd->lpd_ldb )
		    {
			ldb->ldb_lgid_high = i;
			break;
		    }
		}
	    }
	}
	    
    }

    /* LOGFULL_COMMIT txns remain connected to the LDB, LPB, etc */
    if ( (flag & LG_LOGFULL_COMMIT) == 0 )
    {
	ldb->ldb_lxb_count--;
	lpd->lpd_lxb_count--;

	/* Remove LXB from LPD queue. */
	/* Done under the protection of the ldb_mutex */

	next_lxbq = (LXBQ *)LGK_PTR_FROM_OFFSET(lxb->lxb_owner.lxbq_next);
	prev_lxbq = (LXBQ *)LGK_PTR_FROM_OFFSET(lxb->lxb_owner.lxbq_prev);
	next_lxbq->lxbq_prev = lxb->lxb_owner.lxbq_prev;
	prev_lxbq->lxbq_next = lxb->lxb_owner.lxbq_next;
    }

    /*
    ** If this is an active, protected transaction holding up an Online Backup,
    ** we've decremented the count of transactions the checkpoint process
    ** is waiting for and if this is the last one, signal the backup to
    ** continue.
    */
    if (ldb->ldb_status & LDB_BACKUP &&
	ldb->ldb_status & LDB_STALL &&
	lxb->lxb_status & LXB_ACTIVE &&
	lxbo_count == 0)
    {
	/*
	** The apparent last active protected transaction for this database just
	** completed. Before we go on to signal the next part of the backup,
	** re-verify the active protected transaction count.
	** It may not yet be zero, as we let LG_DELAYBT transactions start
	** and run while the backup waits. If it's not zero, don't
	** signal the backup yet; it'll have to wait til we come thru
	** here again, eventually the number should be zero.
	*/

	/* lock the LXB queue to protect ldb_lxbo_count, check it for real */
	if (LG_mutex(SEM_EXCL, &lgd->lgd_lxb_q_mutex))
	    return;
	if (ldb->ldb_lxbo_count == 0)
	{
	    /*
	    ** Now unconditionally signal LGD_CKP_SBACKUP.  We used to
	    ** sometimes signal CPNEEDED, and wait until that was done
	    ** before signalling LGD_CKP_SBACKUP.  Now ckpdb will take
	    ** care of forcing a CP before we get to this state.
	    **
	    ** When we signal the backup to start, we record the current
	    ** EOF in the ldb_sbackup field of the LDB. This will be used
	    ** by higher-level DMF protocols to decide whether a page has
	    ** been changed since the backup began. DMF, however, needs an
	    ** LSN, not an LG_LA, to make this check, since it wishes to
	    ** compare a log sequence number on a page against the LSN of
	    ** the start of the backup. Therefore, we also set
	    ** ldb->ldb_sback_lsn to be the most recent LSN that we have
	    ** assigned.
	    */
	    while(current_lbb = (LBB *)LGK_PTR_FROM_OFFSET(lfb->lfb_current_lbb))
	    {
               if (LG_mutex(SEM_EXCL, &current_lbb->lbb_mutex))
		   return;

               if ( current_lbb == (LBB *)LGK_PTR_FROM_OFFSET(lfb->lfb_current_lbb) )
               {
		  /* current_lbb is still the "CURRENT" lbb. */
                  break;
               }

               (VOID)LG_unmutex(&current_lbb->lbb_mutex);

#ifdef xDEBUG
               TRdisplay( "LG_end(1):: Scenario for INGSRV2838 handled. EOF=<%d, %d, %d>\n",
                          lfb->lfb_header.lgh_end.la_sequence,
                          lfb->lfb_header.lgh_end.la_block,
                          lfb->lfb_header.lgh_end.la_offset);
#endif
            }

	    STRUCT_ASSIGN_MACRO(lfb->lfb_header.lgh_end, ldb->ldb_sbackup);
	    ldb->ldb_sback_lsn = lfb->lfb_header.lgh_last_lsn;
	    (VOID)LG_unmutex(&current_lbb->lbb_mutex);
	    (VOID)LG_unmutex(&lgd->lgd_lxb_q_mutex);
	    (VOID)LG_unmutex(&ldb->ldb_mutex);

	    LG_signal_event(LGD_CKP_SBACKUP, 0, FALSE);
	}
	else
	{
	    (VOID)LG_unmutex(&lgd->lgd_lxb_q_mutex);
	    (VOID)LG_unmutex(&ldb->ldb_mutex);
	}
    }
    else
	(VOID)LG_unmutex(&ldb->ldb_mutex);


    if (lxb->lxb_reserved_space)
    {
	/* lfb_mutex blocks concurrent update of lfb_reserved_space */
	if (LG_mutex(SEM_EXCL, &lfb->lfb_mutex))
	    return;
	/*
	** Free remaining logfile space reserved by this transaction.  
	*/
	if (lfb->lfb_reserved_space >= lxb->lxb_reserved_space)
	{
	    lfb->lfb_reserved_space -= lxb->lxb_reserved_space;
	}
	else
	{
	    uleFormat(NULL, E_DMA491_LFB_RESERVED_SPACE, (CL_ERR_DESC *)NULL,
		    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 1,
		    sizeof(lfb->lfb_reserved_space), &lfb->lfb_reserved_space);
	    lfb->lfb_reserved_space = 0;
	}
	lxb->lxb_reserved_space = 0;
	(VOID)LG_unmutex(&lfb->lfb_mutex);
    }

    /*
    ** If this was the oldest transaction, then move the BOF forward (if
    ** we can) and wake up the archiver (if appropriate).
    **
    ** If we are in force-abort state, check if we need to select a new
    ** victim to abort.
    **
    ** If we are in logfull state, wake up the stalled users since we may
    ** have just freed up the log file.
    */
    if (oldest_tran)
    {
	/*
	** If this was the oldest transaction, then move the BOF forward
	** if we can.  If there are free CP windows for the archiver to
	** process - and there are lgh_cpcnt of them - then wake up the
	** archiver.  If we are in logfull or force-abort state then
	** the archiver will be woken up if there is anything to archive.
	*/

	if (LG_mutex(SEM_EXCL, &lgd->lgd_mutex))
	    return;

	LG_compute_bof(lfb,
		lfb->lfb_header.lgh_cpcnt * lfb->lfb_header.lgh_l_cp);

	if (lgd->lgd_status & LGD_LOGFULL ||
	    lxb->lxb_status & LXB_FORCE_ABORT)
	{
	    /*
	    ** If we are in logfull state, then this transaction ending may 
	    ** have freed up space in the log file.  Wake up everybody 
	    ** waiting and let them try again.
	    */
	    if (lgd->lgd_status & LGD_LOGFULL)
	    {
		lgd->lgd_status &= ~(LGD_LOGFULL);
		LG_resume(&lgd->lgd_wait_stall, NULL);
	    }

	    /*
	    ** If this was a force-abort transaction, then check if aborting
	    ** this transaction has fixed the force-abort condition.
	    ** If the new oldest transaction is in the same consistency 
	    ** point as the transaction that was just force aborted, then 
	    ** we need to force-abort the next transaction as well.
	    */
	    if (lxb->lxb_status & LXB_FORCE_ABORT)
	    {
		if (lgd->lgd_lxb_next != LGK_OFFSET_FROM_PTR(&lgd->lgd_lxb_next))
		{
		    next_lxb = (LXB *)LGK_PTR_FROM_OFFSET(lgd->lgd_lxb_next);

		    if (next_lxb->lxb_type == LXB_TYPE &&
			(next_lxb->lxb_status & 
			    (LXB_ABORT | LXB_NOABORT)) == 0 &&
			LGA_EQ(&next_lxb->lxb_cp_lga, &lxb->lxb_cp_lga))
		    {
			/*
			** Select a victim to force-abort.  If it finds 
			** one, it will turn LGD_FORCE_ABORT status back on.
			*/
			/* LG_abort_oldest will acquire lgd_mutex */
			(VOID)LG_unmutex(&lgd->lgd_mutex);
			LG_abort_oldest((LXB *)NULL);
			if (LG_mutex(SEM_EXCL, &lgd->lgd_mutex))
			    return;
		    }
		}
	    }
	}
	(VOID)LG_unmutex(&lgd->lgd_mutex);
    }

    orphaned_xact = ((lxb->lxb_status & LXB_ORPHAN) != 0);

    /* If handle to a SHARED LXB, remove from its list */
    slxb = (LXB*)NULL;
    if ( lxb->lxb_status & LXB_SHARED_HANDLE &&
	 lxb->lxb_shared_lxb )
    {
	slxb = (LXB*)LGK_PTR_FROM_OFFSET(lxb->lxb_shared_lxb);

	if ( slxb->lxb_status & LXB_SHARED )
	{
	    (VOID)LG_mutex(SEM_EXCL, &slxb->lxb_mutex);

	    /*
	    ** If Handle that's ending was (still) in a
	    ** mini-transaction, wake up any others
	    ** waiting to begin their mini-transactions.
	    **
	    ** Normally, the session will have done   
	    ** a LG_A_EMINI before getting here; this
	    ** just catches txn aborts that might occur
	    ** while IN_MINI.
	    */
	    if ( lxb->lxb_status & LXB_IN_MINI )
	    {
		slxb->lxb_status &= ~LXB_IN_MINI;
		if ( lgd->lgd_wait_mini.lwq_count )
		    LG_resume(&lgd->lgd_wait_mini, NULL);
	    }

	    /*
	    ** If LOGFULL_COMMIT, we shouldn't be in a MINI
	    ** as the FORCE_ABORT is caught outside of MINIs.
	    **
	    ** We'll keep all HANDLEs and get a new tran_id,
	    ** below, for the SHARED transaction.
	    */
	    if ( (flag & LG_LOGFULL_COMMIT) == 0 )
	    {
		next_lxbq = (LXBQ*)LGK_PTR_FROM_OFFSET(lxb->lxb_handles.lxbq_next);
		prev_lxbq = (LXBQ*)LGK_PTR_FROM_OFFSET(lxb->lxb_handles.lxbq_prev);
		next_lxbq->lxbq_prev = lxb->lxb_handles.lxbq_prev;
		prev_lxbq->lxbq_next = lxb->lxb_handles.lxbq_next;

#ifdef LG_SHARED_DEBUG
		TRdisplay("%@ HANDLE lxb %x deallocated from SHARED %x, count %d\n",
		    *(u_i4*)&lxb->lxb_id, *(u_i4*)&slxb->lxb_id,
		    slxb->lxb_handle_count-1);
#endif /* LG_SHARED_DEBUG */

		/*
		** If handles remain, or SHARED txn is being
		** recovered inside-out by the RCP (flag & LG_RCP)
		** retain the SHARED LXB 
		*/
		if ( --slxb->lxb_handle_count || flag & LG_RCP )
		{
		    (VOID)LG_unmutex(&slxb->lxb_mutex);
		    slxb = (LXB*)NULL;
		}
	    }
	}
	else
	    slxb = (LXB*)NULL;
    }

    /*
    ** For LOGFULL_COMMIT, we keep all LXBs. 
    **
    ** Reset the LXB to look like it's just been
    ** allocated, pretty much.
    */
    if ( flag & LG_LOGFULL_COMMIT )
    {
	lxb->lxb_status &= ~(LXB_ABORT | LXB_ACTIVE | LXB_ET);
	lxb->lxb_status |= LXB_INACTIVE;

	lxb->lxb_cp_lga = lfb->lfb_header.lgh_cp;
	lxb->lxb_sequence = 0;
	lxb->lxb_first_lga.la_sequence = 0;
	lxb->lxb_first_lga.la_block    = 0;
	lxb->lxb_first_lga.la_offset   = 0;
	lxb->lxb_last_lga.la_sequence  = 0;
	lxb->lxb_last_lga.la_block     = 0;
	lxb->lxb_last_lga.la_offset    = 0;
	lxb->lxb_first_lsn.lsn_high = 
	    lxb->lxb_first_lsn.lsn_low = 0;
	lxb->lxb_last_lsn.lsn_high = 
	    lxb->lxb_last_lsn.lsn_low = 0;
	lxb->lxb_wait_lsn.lsn_high = 
	    lxb->lxb_wait_lsn.lsn_low  = 0;
	lxb->lxb_stat.split = 0;
	lxb->lxb_stat.write = 0;
	lxb->lxb_stat.force = 0;
	lxb->lxb_stat.wait = 0;
	lxb->lxb_in_mini = 0;
	lxb->lxb_handle_wts = 0;
	lxb->lxb_handle_ets = 0;

	/* Initialize transaction hash pointers */
	lxb->lxb_lxh.lxh_lxbq.lxbq_next = 
	    lxb->lxb_lxh.lxh_lxbq.lxbq_next = 0;

	if ( slxb )
	{
	    /*
	    ** If the SHARED txn ET has been written, then
	    ** all HANDLEs have done their LOGFULL_COMMIT
	    ** call here and it's time to reset the SHARED
	    ** LXB by removing it from the active queue
	    ** and advancing the BOF, if possible.
	    ** To do that, we call ourselves recursively
	    ** on the SHARED LXB. When we come back,
	    ** we'll have a new transaction id which 
	    ** will have been propagated to the HANDLEs
	    ** and any HANDLEs waiting for this to happen
	    ** can be resumed.
	    */
	    if ( slxb->lxb_status & LXB_ET )
	    {
		/* Release the handle */
		(VOID)LG_unmutex(&lxb->lxb_mutex);
		lxb = (LXB*)NULL;
		/* Do recursive LOGFULL_COMMIT on the shared LXB */
		LG_end_tran(slxb, flag);
		/* Resume any waiting handles */
		LG_resume(&lgd->lgd_wait_commit, 0);
	    }
	    else
		(VOID)LG_unmutex(&slxb->lxb_mutex);
	}
	else
	{
	    /* Now get a new tran_id */
	    LG_assign_tran_id(lfb, &lxb->lxb_lxh.lxh_tran_id, FALSE);

	    /*
	    ** If SHARED transaction, update the transaction id of
	    ** all handles while still holding the SHARED lxb_mutex.
	    ** Also make sure the LXB_FORCE_ABORT status bit is cleared.
	    */
	    if ( lxb->lxb_status & LXB_SHARED )
	    {
		handle_lxbq = (LXBQ*)LGK_PTR_FROM_OFFSET(lxb->lxb_handles.lxbq_next);

		while ( handle_lxbq != (LXBQ*)&lxb->lxb_handles )
		{
		    handle_lxb = (LXB*)((char*)handle_lxbq - CL_OFFSETOF(LXB,lxb_handles));
		    handle_lxbq = (LXBQ*)LGK_PTR_FROM_OFFSET(handle_lxbq->lxbq_next);
		    handle_lxb->lxb_lxh.lxh_tran_id = lxb->lxb_lxh.lxh_tran_id;
		    handle_lxb->lxb_status &= ~LXB_FORCE_ABORT;
		}
	    }
	}

	if ( lxb )
	    (VOID)LG_unmutex(&lxb->lxb_mutex);
    }
    else
    {
	/*
	** Normally, shared transactions are ended "outside to in",
	** by LGend-ing each shared HANDLE until all handles are
	** gone, then LGend-ing the SHARED LXB.
	**
	** Certain abort and manual commit situations with shared XA transactions
	** are handled differently, with the LGend of the SHARED
	** LXB being done before all HANDLES have been deallocated.
	** This will only happen when a SHARED XA transaction has
	** disassociated HANDLES attached to it during recovery or
	** manual commit and in these cases we reverse the process and 
	** end the transaction pieces inside-to-out (SHARED->HANDLEs)
	** recursivly calling LG_end_tran on each handle, but with the
	** LG_RCP flag bit so we know not to deallocate the SHARED
	** until we get back here (see check for LG_RCP just above)
	**
	** See additional information in LG_force_abort(), 
	** LG_A_COMMIT, LG_A_ABORT.
	*/
	if ( lxb->lxb_status & LXB_SHARED && lxb->lxb_handle_count )
	{
	    handle_lxbq = (LXBQ*)LGK_PTR_FROM_OFFSET(lxb->lxb_handles.lxbq_next);

	    /* LGend each handle */
	    while ( handle_lxbq != (LXBQ*)&lxb->lxb_handles )
	    {
		handle_lxb = (LXB*)((char*)handle_lxbq - CL_OFFSETOF(LXB,lxb_handles));
		handle_lxbq = (LXBQ*)LGK_PTR_FROM_OFFSET(handle_lxbq->lxbq_next);
		(VOID)LG_unmutex(&lxb->lxb_mutex);
		LG_end_tran(handle_lxb, flag | LG_RCP);
		(VOID)LG_mutex(SEM_EXCL, &lxb->lxb_mutex);
	    }
	    /* Fall through to finish off the SHARED LXB... */
	}

	/* 
	** Last thing we need to do before deallocating the LXB is
	** to check for any post-commit deletes that need to be
	** executed. They can only exist in a XA_STARTed Global
	** Transaction and will only be some if the
	** transaction branches did any DROP, MODIFY type DDL
	** operations.
	*/
	while ( lxb->lxb_lfd_next != LGK_OFFSET_FROM_PTR(&lxb->lxb_lfd_next) )
	{
	    lfd = (LFD*)LGK_PTR_FROM_OFFSET(lxb->lxb_lfd_next);

	    /* If successful commit, delete the file */
	    if ( lxb->lxb_status & LXB_ET && !(lxb->lxb_status & LXB_ABORT) )
	    {
		/* Don't really care about the outcome, I don't think */
		(VOID) DIdelete(&di,
				lfd->lfd_fdel.fdel_path,
				lfd->lfd_fdel.fdel_plen,
				lfd->lfd_fdel.fdel_file,
				lfd->lfd_fdel.fdel_flen,
				&sys_err);
	    }

	    /* Remove LFD from LXB queue and deallocate it */
	    lxb->lxb_lfd_next = lfd->lfd_next;
	    LG_deallocate_cb(LFD_TYPE, (PTR)lfd);
	}

	/*
	** Deallocate the LXB, thereby freeing the lxb_mutex
	*/
	LG_deallocate_cb(LXB_TYPE, (PTR)lxb);

	/*
	** Check if parent control blocks need to be cleaned up.
	**
	** If the transaction was listed as orphaned, then clean up the orphaned
	** LPD to which it was attached (and also the ldb if its reference count
	** reaches zero).
	*/
	if (orphaned_xact)
	{
	    LG_I4ID_TO_ID xid;		/* Keep C compilers happy */

	    xid.id_lgid = lpd->lpd_id;
	    LGremove((LG_DBID)xid.id_i4id, &sys_err);
	}

	/* If unreferenced SHARED LXB, end its transaction and release it */
	if ( slxb )
	{
#ifdef LG_SHARED_DEBUG
	    TRdisplay("%@ SHARED lxb %x deallocated\n",
			*(u_i4*)&slxb->lxb_id);
#endif /* LG_SHARED_DEBUG */

	    LG_end_tran(slxb, flag);
	}
    }

    return;
}
