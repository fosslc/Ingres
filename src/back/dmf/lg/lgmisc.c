/*
**Copyright (c) 2004 Ingres Corporation
**
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <er.h>
#include    <st.h>
#include    <ex.h>
#include    <lo.h>
#include    <me.h>
#include    <nm.h>
#include    <di.h>
#include    <pc.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <tr.h>
#include    <dmf.h>
#include    <ulf.h>
#include    <lk.h>
#include    <lg.h>
#include    <lgdef.h>
#include    <lgdstat.h>
#include    <lgkdef.h>
#include    <lkdef.h>
#include    <dm.h>

/**
**
**  Name: LGMISC.C -- Miscellaneous support routines for LG
**
**  Description:
**	This file contains various LG subroutines which didn't seem to fit in
**	anywhere else.
**
**
**  History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable logging and locking system.
**	16-oct-1992 (jnash) merged 29-jul-1992 (rogerk)
**	    Reduced Logging Project: Changed actions associated with some of
** 	    the Consistency Point protocols. The system now requires
**	    that all servers participate in Consistency Point flushes, not
**	    just fast commit servers:
**            - Added notion of CPAGENT process status.  This signifies that
**              the process controls a data cache that must be flushed when
**              a consistency point is signaled.  Currently, all DBMS servers
**              will have this status and no other process type will.
**            - Added alter entry point LG_A_CPAGENT which causes the CPAGENT 
**              process status to be defined in the current process.  
**            - Changed the LG_A_BCPDONE case to signal the CPFLUSH event if 
**              there are any CPAGENT servers active rather than just 
**              fast commit ones.  
**            - Changed LG_check_fc_servers name to LG_check_cp_servers.
**            - Changed LG_last_fc_server name to LG_last_cp_server.
**            - Removed references to obsolete lgd->lgd_n_servers counter.
**	17-nov-1992 (bryanp)
**	    Added LGidnum_from_id() to extract the ID portion of an LG id.
**	    Currently, this function is used only by the archiver.
**	03-jan-1993 (jnash)
**          Reduced Logging project. 
**	      - Include lgd_reserved_space in used logspace calculation in 
**		LG_check_logfull().  Add LG_used_logspace() routine, now used 
**		when inquiring minds want to know.
**	      - In LG_set_bof(), defer decision to turn off archiving, logfull 
**		and force abort until we have determined that BOF has moved 
**		forward.
**	18-jan-1993 (rogerk)
**	    Changed name of LG_set_bof to LG_archive_complete.
**	15-mar-1993 (rogerk)
**	    Reduced Logging - Phase IV:
**	    Removed system EBACKUP, FBACKUP and database EBACKUP states.
**	17-mar-1993 (rogerk)
**	    Moved lgd status values to lgdstat.h so that they are visible
**	    by callers of LGshow(LG_S_LGSTS).
**	26-apr-1993 (andys)
**	    Cluster 6.5 Project I
**	    Renamed stucture members of LG_LA to new names. This means
**	    replacing lga_high with la_sequence, and lga_low with la_offset.
**	24-may-1993 (bryanp)
**	    Fix arguments to PCforce_exit(). This fixes some shared cache
**		rundown problems.
**	24-may-1993 (rogerk)
**	    Fixed problem in LG_archive_complete which cause hanging in 
**	    online backup operations.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (bryanp)
**	    Fix a problem in LG_compute_bof() when transactions from both local
**		and remote logs are simultaneously active.
**	    Include <tr.h>
**	26-jul-1993 (rogerk)
**	  - Changed journal and dump window tracking in the logging system.
**	    The journal and dump windows are now tracked using the actual
**	    log addresses of the first and last records which define the
**	    window rather than the address of the CP previous to those
**	    log records.  Added a new field - lfb_archive_window_prevcp - 
**	    which gives the consistency point address before which we know 
**	    there is no archiver processing required.  Consolidated the system
**	    journal and dump windows into a single lfb_archive_window.
**	  - Changed LG_archive_complete routine to reset the journal and dump
**	    windows using the archiver stopping address and to store the
**	    previous CP address.
**	  - Changed LG_compute_bof to use the new archive_window_prevcp value.
**	  - Include respective CL headers for all cl calls.
**	23-aug-1993 (bryanp)
**	    Added support to LG_force_abort for man-abort of prepared xacts.
**	18-oct-1993 (bryanp)
**	    Added get_critical_sems implementation, patterned on 6.4 one.
**	18-oct-1993 (rogerk)
**	    Added LG_orphan_xact and LG_adopt_xact routines.
**	    Added argument to LG_used_logspace to indicate whether or not
**	    to include reserved logspace.
**	04-jan-1994 (chiku)
**          Changed LG_chekc_logfull() to call LG_unmutex() before PCExit() on 
**          logfull conditions.  [BUG#: 56702].
**	31-jan-1994 (jnash)
**	    Fix lint detected problem: remove unused declaration of
**	    check_archive_off().
**	15-apr-1994 (chiku)
**          Bug56702: Changed LG_chekc_logfull() to return E_DM9070_LOG_LOGFULL on
**          logfull conditions.
**	20-jul-1995 (canor01)
**	    LG_force_abort() should pass an SCB to the abort_handler instead
**	    of a SID
**	05-Jan-1996 (jenjo02)
**	    Mutex granularity project. Semaphores must now be explicitly
**	    named when calling LKMUTEX functions.
**	14-mar-96 (nick)
**	    Replace LK_mutex() and LK_unmutex() with LK_get_critical_sems()
**	    and LK_rel_critical_sems() respectively.
**	29-Mar-1996 (jenjo02)
**	    Lock appropriate LG semaphores in get|rel_critical_sems().
**	29-May-1996 (jenjo02)
**	    Added lxb parm to LG_check_logfull() to pass to  LG_abort_oldest().
**	    LG_abort_oldest() may abort the LXB which is running the
**	    current thread, in which case its mutex is already locked -
**	    the function needs to know that.
**	08-Oct-1996 (jenjo02)
**	    In get_critical_sems, rel_critical_sems, call CSp|v_semaphore
**	    directly instead of using LG_mutex|unmutex to avoid cluttering
**	    the log with misleading errors.
**	13-Nov-1997 (jenjo02)
**	    When resetting back status in LDB, also reset LDB_STALL_RW
**	    flag (new).
**      08-Dec-1997 (hanch04)
**          Initialize new block field in LG_LA 
**          Calculate number of blocks from new block field in LG_LA 
**          for support of logs > 2 gig
**	18-Dec-1997 (jenjo02)
**	    Added LG_my_pid GLOBAL.
**	16-Nov-1998 (jenjo02)
**	    Cross-process thread identity changed to CS_CPID structure
**	    from PID and SID.
**	15-Mar-1999 (jenjo02)
**	    Removed LDB_STALL_RW flag.
**      23-Jun-99 (hweho01)
**          Added *CS_find_scb() function prototype. Without the 
**          explicit declaration, the default int return value   
**          for a function will truncate a 64-bit address
**          on ris_u64 platform.
**	03-Mar-2000 (jenjo02)
**	    As part of fix for BUG 98473, ensure that LG_abort_oldest()
**	    allows a single force-abort to take place at a time.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	16-Apr-1999 (hanch04)
**	    Call CSfind_scb, not CS_find_scb
**	24-Aug-1999 (jenjo02)
**	    Dropped lgd_status_mutex, lfb_cp_mutex, use lgd_mutex instead.
**	    lfb_current_lbb_mutex replaced by individual LBB's lbb_mutex.
**	    Dropped lgd_lxb_aq_mutex and inactive LXB queue.
**	    lgd_wait_free and lgd_wait_split queues consolidated into
**	    lgd_wait_buffer queue.
**	15-Dec-1999 (jenjo02)
** 	    Added support for SHARED transactions. When force-aborting
**	    any participant, all must be force-aborted.
**	04-Oct-2000 (jenjo02)
**	    Deleted CS_find_scb() prototype.
**	    Replaced CScp_resume() with LGcp_resume macro.
**      01-aug-2002 (jenjo02)
**          LG_abort_transaction: Init lxbq, before referencing it (b108439)
**      22-oct-2002 (stial01)
**          LG_abort_transaction: fixed loop through shared transaction handles
**          Added LG_xa_abort().
**      7-oct-2004 (thaju02)
**          Use SIZE_TYPE to allow memory pools > 2Gig.
**      31-Mar-2005 (horda03) Bug 114192 / INGSRV 3224
**          When a Dead process is identified, before releasing the
**          lgd->lgd_lpb_q_mutex, set the LPB_DEAD flag to prevent other
**          servers trying to Rundown the same process.
**      31-Mar-2006 (stial01)
**          LG_archive_complete() Moved JSWITCH processing here.
**          Always send LG_E_ARCHIVE_DONE.
**	20-Sep-2006 (kschendel)
**	    LG-resume calls changed, update here.
**	6-Apr-2007 (kschendel)
**	    Remove LGidnum_from_id, not used anywhere.
**	16-Aug-2007 (kibro01) b118626
**	    Add status check so repq manager can detect that a transaction
**	    has entered the force-abort state
**      28-feb-2008 (stial01)
**          LG_check_dead() Check for lone logwriter sleeping & unassigned bufs
**	07-Aug-2009 (drivi01)
**	    In order to port to Visual Studio 2008, cleanup the warnings.
**	03-Nov-2010 (jonj) SIR 124685 Prototype Cleanup
**	    Delete non-conforming LG_status_is_abort() function.
**	    Those that need to know this must use LGshow().
*/

static	STATUS	get_critical_sems(void);
static	STATUS	rel_critical_sems(void);

/*{
** Name: LG_archive_complete	- Mark completion of archive event.
**
** Description:
**	This routine is called via LGalter by the archiver whenever
**	an archive cycle is completed.
**
**	It updates the database and system dump and journal windows
**	and moves the Log File BOF forward if possible.
**
**	Its input is two log addresses: the log address at which archive
**	processing stopped; and the last CP encountered by the archiver 
**	during the most recent archive pass.
**
**	Each database journal and dump window is moved up to reflect the
**	archive work just completed (the start of each window is moved
**	up forward to the archiver completion address).  Any dump or journal
**	window for which the archiver completion address beyond the end
**	window address is reset to zero.
**
**	The system archive window is moved up to reflect the new union of
**	all database journal and dump windows.
**
**	The system archive_window_prevcp is set with the value given by
**	the archiver as the address of the last Consistency Point encountered 
**	during the archive scan.  If the archiver provided value is not set
**	then the current archive_winodow_prevcp value is preserved.  This
**	address will be used by the LG_compute_bof call which is made here
**	to determine how far up the Log File BOF can be moved.
**
**	This routine also does post-purge database processing.  A database
**	which is in PURGE state and has just had its journal window closed
**	has a CLOSEDB action signaled for it.
**
** Inputs:
**	acp_stopping_lga		Log Address to which the ACP did
**					archive processing.
**	acp_last_cp			Last Consistency Point encountered by
**					the acp during its archive pass. May
**					be zero if no CP's were encountered.
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
**	03-jan-1993 (jnash)
**	    Reduced logging project.  Defer decision to turn off
**	    archiving, logfull and force abort until it has been determined
**	    that BOF has moved.
**	18-jan-1993 (rogerk)
**	    Changed name from LG_set_bof.
**	15-mar-1993 (rogerk)
**	    Reduced Logging - Phase IV:
**	    Removed ONLINE PURGE database state.
**	24-may-1993 (rogerk)
**	    Changed comparisons using purge log address to take into account
**	    that the purge address includes the last processed record.  So that
**	    if the purge address is exactly equal to the database's jnl or
**	    dmp window end then the window has been completely processed.
**	    Failure to do this caused online backup on an unopen database to
**	    stall because the checkpoint process thought the dump window never
**	    got fully archived.
**	26-jul-1993 (rogerk)
**	    Changed journal and dump window tracking in the logging system.
**	    The journal and dump windows are now tracked using the actual
**	    log addresses of the first and last records which define the
**	    window rather than the address of the CP previous to those
**	    log records.  Added a new field - lfb_archive_window_prevcp - 
**	    which gives the consistency point address before which we know 
**	    there is no archiver processing required.  Consolidated the system
**	    journal and dump windows into a single lfb_archive_window.
**	23-Jan-1996 (jenjo02)
**	    Mutex granularity project. Lock LFB and LDB mutexes.
**	16-Feb-2000 (jenjo02)
**	    LG_signal_event() call incorrectly indicated lgd_mutex was held 
**	    by this function; it should indicate it's not lest a deadlock
**	    ensue.
**	19-Jul-2001 (jenjo02)
**	    Wrap updating of archive window log addresses with lgd_mutex
**	    instead of ldb_mutex and lgd_mutex. This closes a hole 
**	    in which lfb_archive_window_start could assume a value
**	    that is not the union of all open databases, leading to
**	    DM9845 failures in the archiver.
**	21-Mar-2002 (jenjo02)
**	    If ldb_type != LDB_TYPE, don't restart queue, just
**	    keep going.
**	22-Jul-2005 (jenjo02)
**	    Fix race condition exposed by MAC port. Ensure that
**	    LDB's are mutexed before checking status.
**	01-Nov-2006 (jonj)
**	    Using consistent ldb_q, ldb mutex ordering throughout the
**	    code means we can take and hold the ldb_q and mutex LDBs
**	    without deadlock danger. Remove "release the LDB, remutex
**	    the queue" entanglements.
*/
VOID
LG_archive_complete(
register LFB	*lfb,
LG_LA    *acp_stopping_lga,
LG_LA	 *acp_last_cp)
{
    register LGD    *lgd = (LGD *)LGK_base.lgk_lgd_ptr;
    register LDB    *ldb;
    LG_LA	    min_start_address;
    SIZE_TYPE	    end_offset;
    SIZE_TYPE	    ldb_offset;
    i4		    lgd_status;
    bool	    jswitch = FALSE;

    min_start_address.la_sequence = 0;
    min_start_address.la_block    = 0;
    min_start_address.la_offset   = 0;

    /*
    ** Scan the list of LDB's and move each first-journal and first-dump
    ** to reflect the new BOF.  As we go compute the new system first-journal
    ** and first-dump addresses.
    **
    ** If we find that we have completed all journal processing for a database
    ** that has no openers then signal the CLOSEDB to the RCP.
    */

    /*
    ** When both the lgd_ldb_q and ldb must be mutexed, always take
    ** the lgd_ldb_q_mutex, then ldb_mutex.
    */

    /* 
    ** Lock the LDB queue while we scan the list.
    **
    ** Also take the lgd_mutex to prevent the LDB and LFB archive
    ** window values from changing while we do these computations.
    **
    ** lgd_mutex also blocks changes to lgd_status.
    */
    if ( LG_mutex(SEM_EXCL, &lgd->lgd_ldb_q_mutex) ||
	 LG_mutex(SEM_EXCL, &lgd->lgd_mutex) )
	return;

    /*
    ** Turn off the system-wide purge state. We'll turn if back on
    ** if we find any LDBs in a purge state.
    */
    lgd_status = 0;

    end_offset = LGK_OFFSET_FROM_PTR(&lgd->lgd_ldb_next);

    for (ldb_offset = lgd->lgd_ldb_prev;
	ldb_offset != end_offset;)
    {
	ldb = (LDB *)LGK_PTR_FROM_OFFSET(ldb_offset);
	ldb_offset = ldb->ldb_prev;
	
	if (ldb->ldb_type != LDB_TYPE)
	    continue;

	(VOID)LG_mutex(SEM_EXCL, &ldb->ldb_mutex);

	/*
	** Move the first-dump marker up to reflect the new lga.  All
	** records previous to this have been processed by the archiver.
	*/
	if (LGA_GT(acp_stopping_lga, &ldb->ldb_d_first_la))
	    ldb->ldb_d_first_la = *acp_stopping_lga;

	/*
	** If we have finished processing all dump records for this
	** database then reset the dump markers.
	*/
	if ((ldb->ldb_d_first_la.la_sequence == 0) ||
	    (LGA_GTE(&ldb->ldb_d_first_la, &ldb->ldb_d_last_la)))
	{
	    ldb->ldb_d_first_la.la_sequence = 0;
	    ldb->ldb_d_first_la.la_block    = 0;
	    ldb->ldb_d_first_la.la_offset   = 0;
	    ldb->ldb_d_last_la.la_sequence  = 0;
	    ldb->ldb_d_last_la.la_block     = 0;
	    ldb->ldb_d_last_la.la_offset    = 0;
	}


	/*
	** Move the first-journal marker up to reflect the new lga.  All
	** records previous to this have been processed by the archiver.
	*/
	if (LGA_GT(acp_stopping_lga, &ldb->ldb_j_first_la))
	    ldb->ldb_j_first_la = *acp_stopping_lga;

	/*
	** If we have finished processing all journal records for this
	** database then reset the dump markers.
	**
	** If the database is closed, then signal the CLOSEDB event.  This
	** will allow the RCP to complete marking the database closed and
	** release the DM0L_DRAIN lock.
	*/
	if ((ldb->ldb_j_first_la.la_sequence == 0) ||
	    (LGA_GTE(&ldb->ldb_j_first_la, &ldb->ldb_j_last_la)))
	{
	    ldb->ldb_j_first_la.la_sequence = 0;
	    ldb->ldb_j_first_la.la_block    = 0;
	    ldb->ldb_j_first_la.la_offset   = 0;
	    ldb->ldb_j_last_la.la_sequence  = 0;
	    ldb->ldb_j_last_la.la_block     = 0;
	    ldb->ldb_j_last_la.la_offset    = 0;

	    /*
	    ** Signal that a journaled database is ready to be released.
	    */
	    if ((ldb->ldb_status & LDB_PURGE) &&
		(ldb->ldb_lpd_count == 0))
	    {
		ldb->ldb_status |= LDB_CLOSEDB_PEND;
		lgd_status |= LGD_CLOSEDB;
	    }

	    ldb->ldb_status &= ~(LDB_PURGE);
	}

	/*
	** Check if journal switch requested for this database
	*/ 
	if (ldb->ldb_status & LDB_JSWITCH)
	{
	    jswitch = TRUE;
	    ldb->ldb_status &= ~(LDB_JSWITCH);
	}

	/*
	** Compute minimum starting address of all the dump and journal
	** windows.  The system archive window will be moved up to this
	** address below.
	*/
	if ((min_start_address.la_sequence == 0) ||
	    ((ldb->ldb_j_first_la.la_sequence) && 
		(LGA_LT(&ldb->ldb_j_first_la, &min_start_address))))
	{
	    min_start_address = ldb->ldb_j_first_la;
	}

	if ((min_start_address.la_sequence == 0) ||
	    ((ldb->ldb_d_first_la.la_sequence) && 
		(LGA_LT(&ldb->ldb_d_first_la, &min_start_address))))
	{
	    min_start_address = ldb->ldb_d_first_la;
	}

	/* Set the system purgedb status if any LDBs are in that state */
	if (ldb->ldb_status & LDB_PURGE)
	    lgd_status |= LGD_PURGEDB;

	/* Release the LDB */
	(VOID)LG_unmutex(&ldb->ldb_mutex);
    }

    /* Finished with the LDB queue */
    (VOID) LG_unmutex(&lgd->lgd_ldb_q_mutex);

    /*
    ** Reset the system archive window using the minimum address of any of
    ** the database dump or journal windows.
    */
    lfb->lfb_archive_window_start = min_start_address;

    /*
    ** Store the address of the last Consistency Point encountered by the
    ** archiver.  The compute_bof routine uses this as address to which it
    ** may move the Log File BOF without losing records still needed by
    ** the archiver.
    **
    ** If no new CPs were found by the archiver then preserve the current
    ** previous CP value.
    */
    if (acp_last_cp->la_sequence)
	lfb->lfb_archive_window_prevcp = *acp_last_cp;

    /*
    ** Check if we can close the system archive window.
    */
    if ((lfb->lfb_archive_window_start.la_sequence == 0) ||
	(LGA_GTE(&lfb->lfb_archive_window_start, &lfb->lfb_archive_window_end)))
    {
	lfb->lfb_archive_window_start.la_sequence  = 0;
	lfb->lfb_archive_window_start.la_block     = 0;
	lfb->lfb_archive_window_start.la_offset    = 0;
	lfb->lfb_archive_window_end.la_sequence    = 0;
	lfb->lfb_archive_window_end.la_block       = 0;
	lfb->lfb_archive_window_end.la_offset      = 0;
	lfb->lfb_archive_window_prevcp.la_sequence = 0;
	lfb->lfb_archive_window_prevcp.la_block    = 0;
	lfb->lfb_archive_window_prevcp.la_offset   = 0;
    }
    
    /*
    ** Turn off the Archive, PURGEDB, CLOSEDB events.
    */
    lgd->lgd_status &= ~(LGD_ARCHIVE | LGD_PURGEDB | LGD_CLOSEDB);

    /* Update lgd_status with any new bits (PURGEDB, CLOSEDB) */
    lgd->lgd_status |= lgd_status;

    /* Any databases to close? */
    if ( lgd->lgd_status & LGD_CLOSEDB )
	/* TRUE: lgd_mutex is held */
	LG_signal_event(LGD_CLOSEDB, 0, TRUE);

    /*
    ** Turn off LOGFULL since we may have just freed up log space.  
    ** If not then LOGFULL will be resignaled on the next request 
    ** for logspace.
    */
    if (lgd->lgd_status & LGD_LOGFULL)
    {
	lgd->lgd_status &= ~(LGD_LOGFULL);
	LG_resume(&lgd->lgd_wait_stall, NULL);
    }

    /*
    ** With the Archive event turned off above, call compute_bof to determine if the
    ** log file BOF can be moved forward in light of the work we have just
    ** done. This may in turn cause the ARCHIVE event to be resignaled.
    */

    LG_compute_bof(lfb, lfb->lfb_header.lgh_cpcnt * lfb->lfb_header.lgh_l_cp);

    /*
    ** Check if journal switch requested for ANY database
    ** JSWITCH requested BEFORE dma_online_context was done. 
    ** JSWITCH requested AFTER dma_online_context was NOT done.
    **
    ** No need for archiver to distinguish if JSWITCH was done or not.
    ** JSP alterdb -next_jnl_file needs to verify if JSWITCH completed. 
    */
    if (jswitch)
    {
#ifdef xDEBUG
	TRdisplay("LG_archive_complete: JSWITCH requested\n");
#endif
    }

    /*
    ** No need for archiver to distinguish if JSWITCH was done or not.
    ** JSP alterdb -next_jnl_file needs to verify JSWITCH completed.
    **
    ** Always send LG_E_ARCHIVE_DONE
    */
    LG_signal_event(LG_E_ARCHIVE_DONE, LGD_JSWITCH, TRUE);

    (VOID)LG_unmutex(&lgd->lgd_mutex);

    return;
}

/*{
** Name: LG_check_cp_servers.
**
** Description:
**	Check if all servers have the same CP condition value.
**	If they do then return TRUE, otherwise return FALSE.
**
**	Called during LG_A_CPFDONE and LG_A_CPWAITDONE to see if all
**	servers have responded.
**
** Inputs:
**	Value - condition value to check for.
**
** Outputs:
**	Returns:
**	    TRUE    - all servers have condition value specified.
**	    FALSE   - not all servers have condition.
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable logging and locking system.
**	16-oct-1992 (jnash) merged 29-jul-1992 (rogerk)
**	    Reduced logging Project: Changed routine to check for all servers
**	    instead of just Fast Commit servers.  The system now requires
**	    that all servers participate in Consistency Point flushes.
**	    Changed routine name from LG_check_fc_servers to 
**	    LG_check_cp_servers.  Added the notion of CPAGENT process 
**	    status.  This signifies that the process controls a data 
**	    cache that must be flushed when a consistency point is 
**	    signaled.  Currently, all DBMS servers will have this status 
**	    and no other process type will.
**	24-Jan-1996 (jenjo02)
**	    New mutexes to protect LPB queue and LPBs.
**	28-Feb-1996 (jenjo02)
**	    Check count of non-void cpagents before locking and
**	    walking lpb queue.
*/
bool
LG_check_cp_servers(i4 value)
{
    register LGD    *lgd = (LGD *)LGK_base.lgk_lgd_ptr;
    register LPB    *lpb;
    SIZE_TYPE	    end_offset;
    SIZE_TYPE	    lpb_offset;

    if (lgd->lgd_n_cpagents == 0)
	return(TRUE);

    if (LG_mutex(SEM_EXCL, &lgd->lgd_lpb_q_mutex))
	return(FALSE);

    end_offset = LGK_OFFSET_FROM_PTR(&lgd->lgd_lpb_next);

    for (lpb_offset = lgd->lgd_lpb_next;
         lpb_offset != end_offset;)
    {
	lpb = (LPB *)LGK_PTR_FROM_OFFSET(lpb_offset);
        lpb_offset = lpb->lpb_next;

	if (lpb->lpb_status & LPB_CPAGENT && 
	   (lpb->lpb_status & LPB_VOID) == 0 &&
	    lpb->lpb_cond != value)
	{
	    /*
	    ** Release the lpb_q mutex before locking the LPB
	    ** to avoid deadlocking with code going the other
	    ** way (LPB->lpb_q), then relock and recheck.
	    */
	    (VOID)LG_unmutex(&lgd->lgd_lpb_q_mutex);
	    if (LG_mutex(SEM_EXCL, &lpb->lpb_mutex))
		return(FALSE);
	    /*
	    ** Recheck LPB after semaphore wait
	    */
	    if (lpb->lpb_type == LPB_TYPE &&
		lpb->lpb_status & LPB_CPAGENT && 
	       (lpb->lpb_status & LPB_VOID) == 0 &&
		lpb->lpb_cond != value)
	    {
		(VOID)LG_unmutex(&lpb->lpb_mutex);
		return (FALSE);
	    }
	    /*
	    ** Something about the LPB changed while we
	    ** waited for its mutex. Restart the lpb_q scan.
	    */
	    (VOID)LG_unmutex(&lpb->lpb_mutex);
	    if (LG_mutex(SEM_EXCL, &lgd->lgd_lpb_q_mutex))
		return(FALSE);
	    lpb_offset = lgd->lgd_lpb_next;
	}
    }

    (VOID)LG_unmutex(&lgd->lgd_lpb_q_mutex);

    return (TRUE);
}

/*{
** Name: LG_last_cp_server
**
** Description:
**	When a server is removed from the system, check to see if we
**	are in the middle of a consistency point.  If we are and we
**	are waiting for this server, then we can now proceed.
**
**	NOTE that this implies that recovery for the exiting server must be
**	complete before the lpb is removed and this routine called.
**
** Inputs:
**	NONE
**
** Outputs:
**	Returns:
**	    NONE
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable logging and locking system.
**	16-oct-1992 (jnash) merged 29-jul-1992 (rogerk)
**	    6.5 Recovery Project: Changed routine to check for all servers
**	    instead of just Fast Commit servers.  The system now requires
**	    that all servers participate in Consistency Point flushes.
**	    Changed routine name from LG_last_fc_server to LG_last_cp_server.
**	    Added the notion of CPAGENT process status.  This signifies that
**	    the process controls a data cache that must be flushed when
**	    a consistency point is signaled.  Currently, all DBMS servers
**	    will have this status and no other process type will.
**	24-Jan-1996 (jenjo02)
**	    New mutexes to protect LPB queue and LPBs.
**	28-Feb-1996 (jenjo02)
**	    Check (new) count of cpagents instead of locking and walking
**	    lpb queue.
**	07-Aug-2009 (drivi01)
**	    Cleanup precedence warning in effort to port to Visual
**	    Studio 2008.
*/
VOID
LG_last_cp_server(void)
{
    register LGD    *lgd = (LGD *)LGK_base.lgk_lgd_ptr;

    /* If not in middle of consistency point, no work to do */
    if ((lgd->lgd_status & (LGD_CPFLUSH | LGD_CPWAKEUP)) == 0)
	return;

    if (LG_mutex(SEM_EXCL, &lgd->lgd_mutex))
	return;
    /*
    ** Check again after semaphore wait
    */
    if (lgd->lgd_status & (LGD_CPFLUSH | LGD_CPWAKEUP))
    {
	/*
	** Check count of fast commit servers left. If count has been
	** reduced to zero, then any CP protocol can go directly to the ECP.
	*/
	if (lgd->lgd_n_cpagents == 0)
	{
	    LG_signal_event(LGD_ECP, LGD_CPFLUSH | LGD_CPWAKEUP, TRUE);
	}
	else if (lgd->lgd_status & LGD_CPFLUSH)
	{
	    if (LG_check_cp_servers(LPB_CPWAIT))
	    {
		LG_signal_event(LGD_CPWAKEUP, LGD_CPFLUSH, TRUE);
	    }
	}
	else if (lgd->lgd_status & LGD_CPWAKEUP)
	{
	    if (LG_check_cp_servers(LPB_CPREADY))
	    {
		LG_signal_event(LGD_ECP, LGD_CPWAKEUP, TRUE);
	    }
	}
    }

    (VOID)LG_unmutex(&lgd->lgd_mutex);

    return;
}

/*{
** Name: LG_signal_check - check if need CP when signaling PURGEDB
**
** Description:
**	In order for the PURGEDB event to be processed, we have to do
**	consistency point.  If there is already one in progress, then
**	we have to signal a new one.  We do this by setting the delayed
**	bcp flag in the lgd struct.  When the first CP is done, a new
**	CP will be be immediately started.
**
** Inputs:
**
** Outputs:
**	Returns:
**	    NONE
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable logging and locking system.
**	24-Jan-1996 (jenjo02)
**	    Mutex granularity project changes.
**	    All callers of LG_signal_check() were turning off
**	    LGD_ECPDONE in lgd_status, so that was moved into here
**	    so it can be done under the protection of the lgd_status_mutex.
*/
VOID
LG_signal_check(void)
{
    register LGD    *lgd = (LGD *)LGK_base.lgk_lgd_ptr;

    if (LG_mutex(SEM_EXCL, &lgd->lgd_mutex))
	return;

    if (lgd->lgd_no_bcp)
    {
        lgd->lgd_delay_bcp = 1;
	LG_signal_event(LGD_PURGEDB, LGD_ECPDONE, TRUE);
    }
    else 
    {
	lgd->lgd_no_bcp = 1;
	LG_signal_event(LGD_PURGEDB | LGD_CPNEEDED, LGD_ECPDONE, TRUE);
    }

    (VOID)LG_unmutex(&lgd->lgd_mutex);

    return;
}

/*{
** Name: LG_abort_oldest - Find the oldest transaction and force abort it.
**
** Description:
**	This routine is called when we have passed the force_abort limit
**	and need to abort the oldest transaction in order to free up log
**	space.  It is also called from end_tran if force_aborting a 
**	transaction has not resulted in allowing us to free up any log
**	space (log space is reclaimed in intervals of Consistency Points -
**	if more than one Transactions are active in the oldest CP when
**	the force_abort limit is reached, then all transactions in that
**	CP window will have to be aborted before we can reclaim any log space).
**
**	This routine looks for the oldest active transaction that can be
**	picked as a victim.  If it is in the oldest Consistency Point then
**	it marks it for FORCE_ABORT and signals the server to abort the
**	transaction.
**
** Inputs:
**	None
**
** Outputs:
**	Returns:
**	    None
**	Exceptions:
**	    None
**
** Side Effects:
**	None
**
** History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable logging and locking system.
**	24-Jan-1996 (jenjo02)
**	    New mutexes to protect various queues and structures.
**	29-May-1996 (jenjo02)
**	    Added lxb parm to LG_check_logfull() to pass to  LG_abort_oldest().
**	    LG_abort_oldest() may abort the LXB which is running the
**	    current thread, in which case its mutex is already locked -
**	    the function needs to know that.
**	03-Mar-2000 (jenjo02)
**	    As part of fix for BUG 98473, ensure that LG_abort_oldest()
**	    allows a single force-abort to take place at a time.
*/
VOID
LG_abort_oldest(LXB *calling_lxb)
{
    register LGD        *lgd = (LGD *)LGK_base.lgk_lgd_ptr;
    register LFB	*lfb = &lgd->lgd_local_lfb;
    register LXB	*lxb;
    SIZE_TYPE		end_offset;
    SIZE_TYPE		lxb_offset;

    /* Find the oldest active transaction that can be aborted. */

    if (LG_mutex(SEM_EXCL, &lgd->lgd_lxb_q_mutex))
	return;

    end_offset = LGK_OFFSET_FROM_PTR(&lgd->lgd_lxb_next);

    for (lxb_offset = lgd->lgd_lxb_next;
	 lxb_offset != end_offset;
	 lxb_offset = lxb->lxb_next)
    {
	lxb = (LXB *)LGK_PTR_FROM_OFFSET(lxb_offset);

	    /* Can't abort NOABORT transactions. */
	if (lxb->lxb_status & LXB_NOABORT)
	    continue;

	(VOID)LG_unmutex(&lgd->lgd_lxb_q_mutex);

	/* 
	** Don't force abort the transaction if transaction is already being 
	** aborted or if the transaction is in a WILLING COMMIT state.
	*/
	if (lxb->lxb_status & (LXB_ABORT | LXB_WILLING_COMMIT))
	    return;

	/*
	** If the oldest transaction is not in the first CP window, then
	** don't bother aborting it.  It is not the cause of the logfull
	** condition, something else is preventing the first CP window from
	** begin archived.
	*/
	if ((lxb->lxb_cp_lga.la_sequence != lfb->lfb_header.lgh_begin.la_sequence) ||
	    (lxb->lxb_cp_lga.la_block != lfb->lfb_header.lgh_begin.la_block) ||
	    (lxb->lxb_cp_lga.la_offset != lfb->lfb_header.lgh_begin.la_offset))
	    return;   

	/*
	** Found the transaction that is holding up the log file.  Mark
	** it to be aborted and signal the server to do the abort.
	**
	** If it's the transaction which is currently active (the one
	** that got us here), its mutex must already be locked.
	*/
	if (lxb != calling_lxb)
	{
	    if (LG_mutex(SEM_EXCL, &lxb->lxb_mutex))
		return;
	    /*
	    ** Check everything again after semaphore wait
	    */
	    if (lxb->lxb_status & (LXB_ABORT | LXB_WILLING_COMMIT) ||
	       (lxb->lxb_cp_lga.la_sequence != lfb->lfb_header.lgh_begin.la_sequence) ||
		(lxb->lxb_cp_lga.la_block != lfb->lfb_header.lgh_begin.la_block) ||
		(lxb->lxb_cp_lga.la_offset != lfb->lfb_header.lgh_begin.la_offset))
	    {
		(VOID)LG_unmutex(&lxb->lxb_mutex);
		return;
	    }
	}
	lxb->lxb_status |= LXB_FORCE_ABORT;

	/* Force abort the transaction */
	LG_abort_transaction(lxb);

	if (lxb != calling_lxb)
	    (VOID)LG_unmutex(&lxb->lxb_mutex);

	return;
    }

    (VOID)LG_unmutex(&lgd->lgd_lxb_q_mutex);


    return;

}

/*{
** Name: LG_abort_transaction - Add a transaction to the abort list.
**
** Description:
**	This routine adds an LXB to the installation-wide queue of
**	transactions to be aborted, either FORCE_ABORT or MAN_ABORT.
**
**	If the transaction is SHARED, all HANDLE LXBs are instead
**	added to the list.
**
** Inputs:
**	LXB	*lxb		The LXB whose transaction is to be
**				aborted.
**
** Outputs:
**	Returns:
**	    None
**	Exceptions:
**	    None
**
** Side Effects:
**	One or more LXBs are scheduled to be aborted.
**
** History:
**	15-Dec-1999 (jenjo02)
**	    Created for SHARED transactions.
**	18-Jul-2006 (jonj)
**	    XA support; manually dispose of disassociated transaction
**	    handles - they're not owned by any user thread and can't
**	    be force-aborted.
**	    Deprecated LG_xa_abort().
*/
VOID
LG_abort_transaction(LXB *lxb)
{
    LGD         *lgd = (LGD *)LGK_base.lgk_lgd_ptr;
    LPD		*lpd;
    LPB		*lpb;
    LXB		*handle_lxb;
    LXBQ	*lxbq, *handle_lxbq, *prev_lxbq;

    /* LXB is presumed to be mutexed */

    /* Mutex the abort queue */
    (VOID)LG_mutex(SEM_EXCL, &lgd->lgd_mutex);

    /* Set appropriate status in LGD */
    if ( lxb->lxb_status & LXB_FORCE_ABORT )
	lgd->lgd_status |= LGD_FORCE_ABORT;
    else
	lgd->lgd_status |= LGD_MAN_ABORT;

    if ( lxb->lxb_status & LXB_SHARED )
    {
	/*
	** SHARED transaction.
	**
	** Add all connected handle LXBs to the abort list,
	** but not the SHARED transaction itself.
	*/
	lxbq = (LXBQ *)LGK_PTR_FROM_OFFSET(lxb->lxb_handles.lxbq_next);
	handle_lxb = (LXB*)((char*)lxbq - CL_OFFSETOF(LXB, lxb_handles));
    }
    else
    {
	handle_lxb = lxb;
    }
	
    do 
    {
	if ( lxb->lxb_status & LXB_SHARED )
	{
	    /*
	    ** SHARED transaction.
	    **
	    ** Add all connected handle LXBs to the abort list,
	    ** but not the SHARED transaction itself.
	    */
	    /* Skip if txn already aborting */
	    if ( handle_lxb->lxb_status & LXB_ABORT )
	    {
		handle_lxbq = (LXBQ*)LGK_PTR_FROM_OFFSET
			    (handle_lxb->lxb_handles.lxbq_next);
		handle_lxb = 
		    (LXB*)((char*)handle_lxbq - CL_OFFSETOF(LXB,lxb_handles));
		continue;
	    }

	    if ( lxb->lxb_status & LXB_FORCE_ABORT )
		handle_lxb->lxb_status |= LXB_FORCE_ABORT;
	    else
		handle_lxb->lxb_status |= LXB_MAN_ABORT;
	}

	/* Add this LXB to the end of the list of those to be aborted */
	handle_lxb->lxb_lgd_abort.lxbq_prev = 
	    lgd->lgd_lxb_abort.lxbq_prev;
	handle_lxb->lxb_lgd_abort.lxbq_next = 
	    LGK_OFFSET_FROM_PTR(&lgd->lgd_lxb_abort);
	prev_lxbq = (LXBQ *)
	    LGK_PTR_FROM_OFFSET(lgd->lgd_lxb_abort.lxbq_prev);
	prev_lxbq->lxbq_next = lgd->lgd_lxb_abort.lxbq_prev = 
	    LGK_OFFSET_FROM_PTR(&handle_lxb->lxb_lgd_abort);
	lgd->lgd_lxb_aborts++;

	lpd = (LPD *)LGK_PTR_FROM_OFFSET(handle_lxb->lxb_lpd);
	lpb = (LPB *)LGK_PTR_FROM_OFFSET(lpd->lpd_lpb);

	/* Wake up the process's force abort thread */
	LGcp_resume(&lpb->lpb_force_abort_cpid);

	if ( lxb->lxb_status & LXB_SHARED )
	{
	    handle_lxbq = (LXBQ*)LGK_PTR_FROM_OFFSET
		    (handle_lxb->lxb_handles.lxbq_next);
	    handle_lxb = 
		    (LXB*)((char*)handle_lxbq - CL_OFFSETOF(LXB,lxb_handles));
	}

    } while ( lxb->lxb_status & LXB_SHARED &&
	      handle_lxbq != (LXBQ *)&lxb->lxb_handles );

    (VOID)LG_unmutex(&lgd->lgd_mutex);

    return;
}

/*{
** Name: LG_compute_bof - Compute new BOF and signal archiver if necessary
**
** Description:
**	This routine is called when we may have done something to allow
**	us to move the logfile BOF forward.  This includes:
**
**	    - ending the oldest transaction
**	    - completing a conisistency point
**	    - completing an archive cycle
**
**	It should be safe, however, to call this routine at any time.  If
**	none of the above have been done, the routine will just have no
**	result.
**
**	The log file BOF should be the minimum of the following:
**	    1)	The address of the BCP before the oldest active transaction
**		(if there is one) - given by lgd_lxb_next->lxb_cp_lga.
**	    2)	The address of the latest BCP record written - given by
**		lgd_header.lgh_cp.
**	    3)	The address of the BCP before the start of the current
**		archive window - given by lfb_archive_window_prevcp.
**
**	If there are journaled transactions to archive and the correct number
**	of CP's have been executed then wake up the archiver.
**
**	The number of CP's to execute before waking up the archiver is given
**	by the parameter 'archive_interval'.  This will generally be the
**	user-setable lgh_cpcnt.  In some cases, however, this routine may
**	be called to start the archiver if there is anything that can be
**	archived - when the log is full or when the RCP is aborting
**	transactions.  In this case, 'archive_interval' will be zero.
**
** Inputs:
**	archive_interval  - The size of the archive window at which to invoke
**			    the archiver.  Specified in number of blocks.
**
** Outputs:
**	Returns:
**	    None
**	Exceptions:
**	    None
**
** Side Effects:
**	None
**
** History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable logging and locking system.
**	26-jul-1993 (bryanp)
**	    Fix a problem in LG_compute_bof() when transactions from both local
**		and remote logs are simultaneously active.
**	26-jul-1993 (rogerk)
**	  - Changed to use the new lfb_archive_window_prevcp as the address
**	    to which we can move the BOF without upsetting archive processing.
**	    We now move the BOF to the lesser of this value and the oldest
**	    transaction prev_cp value.
**	  - Also changed the logic which decides whether to signal the
**	    archiver to use the lfb_archive_window to determine the amount
**	    of the log file ready to archive rather then the difference 
**	    between the BOF and oldest transaction prevcp.  This also means
**	    that the archiver will be woken up to archive non-committed
**	    transaction work.
**	  - Changed archive interval calculations to use unsigned arithmetic
**	    to avoid overflow problems should the logfile size approach maxi4.
**	23-Jan-1996 (jenjo02)
**	    Mutex granularity project. Many new mutexes to ease the load
**	    on singular lgd_mutex.
**	    Added lfb_cp_mutex_is_locked to prototype, TRUE if caller holds
**	    lfb_cp_mutex EXCL, in which case we'll return with it still 
**	    held, otherwise we'll acquire and free the semaphore ourselves.
**      04-Mar-2004 (inifa01) XINT for b107356 INGSRV 1728 by (bolke01)
**          Once a journalled record has been written to the log file and
**          committed the archiver needs to be woken at a subsequent CP.
**          This happens when most/all logging activity is for journalled
**          databases and when a journalled databse is closed.  When subsequent
**          logging activity is not journalled the archive window is not moved
**          on therefore we need to signal the archiver after the required
**          number of CP's have passed.(107356)
*/
VOID
LG_compute_bof(register LFB *lfb, i4 archive_interval)
{
    register LGD    *lgd = (LGD *)LGK_base.lgk_lgd_ptr;
    LG_LA	    ncp;
    SIZE_TYPE	    lxb_offset;
    SIZE_TYPE	    end_offset;
    register LXB    *lxb;
    u_i4	    archive_window_size;
    bool	    lxb_found = FALSE;
    bool	    signal_archive = FALSE;

    /* Caller must hold lgd_mutex */

    /*
    ** Find the BCP address that defines the spot to which the BOF can be 
    ** moved.  This is the lesser of:
    **
    **     - The latest BCP that has no active transactions before it.
    **     - The latest BCP that has no archive work before it.
    **     - The latest BCP in the system.
    **
    ** The first is found by finding the oldest transaction and extracting
    ** the CP address stored in the LXB.  This is the latest CP previous
    ** to the BT record of that transaction.
    **
    ** The second is recorded in lfb_archive_window_prevcp.
    ** The third is recorded in lfb_header.lgh_cp.
    **
    ** Note that if the CSP is currently performing node failure recovery
    ** then any transactions for remote logs do not count -- we want to find 
    ** the oldest active transaction in the log described by "lfb".
    */
    
    end_offset = LGK_OFFSET_FROM_PTR(&lgd->lgd_lxb_next);
	
    for (lxb_offset = lgd->lgd_lxb_next;
	lxb_offset != end_offset;
	lxb_offset = lxb->lxb_next)
    {
	lxb = (LXB *)LGK_PTR_FROM_OFFSET(lxb_offset);
	lxb_offset = lxb->lxb_next;

	if (lxb->lxb_status & LXB_ACTIVE &&
	    lxb->lxb_lfb_offset == LGK_OFFSET_FROM_PTR(lfb))
	{
	    ncp = lxb->lxb_cp_lga;
	    lxb_found = TRUE;
	    break;
	}
	if ((lxb->lxb_status & LXB_ACTIVE) == 0)
	{
	    lxb_offset = lgd->lgd_lxb_next;
	}
    }

    if (lxb_found ==  FALSE)
	ncp = lfb->lfb_header.lgh_cp;

    if ((lfb->lfb_archive_window_prevcp.la_sequence) &&
	(LGA_LT(&lfb->lfb_archive_window_prevcp, &ncp)))
    {
	ncp = lfb->lfb_archive_window_prevcp;
    }

    /*
    ** Move the BOF forward to its new spot.
    */
    if (LGA_GT(&ncp, &lfb->lfb_header.lgh_begin))
    {
	lfb->lfb_header.lgh_begin = ncp;
    }

    /*
    ** Check the outstanding Archive Window to determine whether it is
    ** sufficiently large to warrant signaling archiver processing.
    **
    ** Note: The logging system documents that it invokes the archiver
    ** every N consistency points.  But really this check invokes the
    ** archiver each time the archive window reaches a size that would
    ** theoretically hold N consistency points, regardless of how many
    ** CPs there actually are.
    */

    if (lfb->lfb_archive_window_start.la_sequence)
    {
	/* Check for log file wrap-around */
	if (lfb->lfb_archive_window_end.la_sequence > 
				    lfb->lfb_archive_window_start.la_sequence)
	{
	    archive_window_size = 
		lfb->lfb_header.lgh_count - 
		(lfb->lfb_archive_window_start.la_block - 
				  lfb->lfb_archive_window_end.la_block);
	}
	else
	{
	    archive_window_size = lfb->lfb_archive_window_end.la_block - 
			      lfb->lfb_archive_window_start.la_block;
	}

	/*
	** If the archive interval has been reached then signal the ACP.
	*/
	if (archive_interval && (archive_window_size >= archive_interval))
	{
	    signal_archive = TRUE;
	}
        else if (archive_interval && LGA_GT(&lfb->lfb_header.lgh_cp,&lfb->lfb_archive_window_prevcp))
        {
#ifdef xDEBUG
          TRdisplay("%@ Non-journalled logging activity initiated archive activity after Journalled update without DB close\n");
#endif /* xDEBUG  */
              signal_archive = TRUE;
        }

	/*
	** If an archiver interval of zero has been specified, or if we are
	** a logfull or near-logfull situation then signal the archiver in 
	** any time space can potentially be freed by archiving.
	** Only start the archiver if there is a new CP to which to move the 
	** archive window (we don't want to signal an archive cycle every time
	** a log record is written in a near-logfull situation).
	*/
	if ((archive_interval == 0) ||
	    (lgd->lgd_status & (LGD_FORCE_ABORT | LGD_LOGFULL)))

	{
	    if (LGA_GT(&lfb->lfb_header.lgh_cp,&lfb->lfb_archive_window_prevcp))
	    {
		signal_archive = TRUE;
	    }
	}
    }
    else
    {
	/*
	** Make sure the ARCHIVE status is off since there is nothing to 
	** archive.  (I'm not sure there is a need for this here, as the
	** archive status is also turned off in LG_archive_complete before
	** calling this routine.  Perhaps it just makes sure that we can't
	** get into a state where there is nothing to archive but that the
	** archiver is being signaled to check for work to do.)
	** (jenjo02): LG_archive_complete() needs to turn it off before
	**	      calling us in case we decided to resignal it; if
	**	      it's not turned off, LG_signal_event() will think
	**	      it already signalled and not resume the waiters.
	*/
	lgd->lgd_status &= ~(LGD_ARCHIVE);
    }

    /*
    ** If archiver is needed, wake it up.
    */
    if (signal_archive)
	LG_signal_event(LGD_ARCHIVE, 0, TRUE);

    /* Return with lgd_mutex still locked */
    return;
}

/*{
** Name: LG_check_dead() - Find any processes that have abnormally exited.
**
** Description:
**	Run throught the logging systems process list and find check by
**	using kill(pid, 0) whether that process is still alive.  This is
**	necessary for any process which does not cleanup in the normal
**	fashion (ie. processes that that are killed with "-9", or that
**	are exited while in the debugger).
**
**	With the fast commit changes, the the LPB may or may not be
**	deleted by LG_rundown().  This makes looping through the LPB list
**	while calling LG_rundown() much more difficult.  In order to 
**	simplify this problem, LG_check_dead quits after it has found the
**	first dead pid.  Any other pid's will be found the next time we
**	check (ie. 5 seconds later).  Abnormal server death had better be
**	an rare event, or there are other things to fix.
**
** OUTSTANDING PROBLEM:
**
**	Some of the dbutil utilities only connect to the LK system rather than
**	the LG system.  These programs will never be found by the following
**	routine if they abnormally exit (this should only happen in the kill
**	-9 case as PCatexit() should call the rundown handlers in other cases).
**		outstanding as of 12/29/88 (mmm)
**
** Inputs:
**	none.
**
** Outputs:
**	none.
**
**	Returns:
**	    VOID.
**
** Side Effects:
**	    {LG,LK}_rundown() are called which clean up slots in the lg table 
**	    which are owned by dead processes.
**
** History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable logging and locking system.
**	24-Jan-1996 (jenjo02)
**	    Mutex granularity project. Removed acquisition and freeing
**	    lgd_mutex. LPB queue mutex is used instead.
**	    Continue searching LPB queue to find all dead pids instead
**	    of bailing out after the first one is found and cleaned up.
**      12-apr-2005 (horda03) Bug 114192/INGSRV 3224
**          Ensure only one server Rundowns a dead process. Once a dead
**          process is detected which requires Rundown, set the LPB_DEAD
**          flag which will prevent other servers trying to rundown the
**          dead server BEFORE releasing the lgd_lpb_q_mutex mutex.
**	15-May-2007 (toumi01)
**	    For supportability add process info to shared memory.
**/
STATUS
LG_check_dead(CL_ERR_DESC *sys_err)
{
    register LGD *lgd = (LGD *)LGK_base.lgk_lgd_ptr;
    register LPB *lpb;
    SIZE_TYPE	lpb_offset;
    SIZE_TYPE	end_offset;
    PID		saved_pid;
    STATUS	status;
    i4	err_code;
    LXBQ	*lwq;
    LXB 	*lwlxb;

    CL_CLEAR_ERR(sys_err);

    /* check to see if every process is still alive */

    if (status = LG_mutex(SEM_EXCL, &lgd->lgd_lpb_q_mutex))
	return(status);

    end_offset = LGK_OFFSET_FROM_PTR(&lgd->lgd_lpb_next);
    lpb_offset = lgd->lgd_lpb_next;

    while (lpb_offset != end_offset)
    {
	lpb = (LPB *)LGK_PTR_FROM_OFFSET(lpb_offset);
	lpb_offset = lpb->lpb_next;

	/*
	** Check if rundown has already been called for this server - if so
	** then go to check next one.
	*/
	if (lpb->lpb_status & LPB_DEAD)
	    continue;

	saved_pid = lpb->lpb_pid;
	if (PCis_alive(saved_pid) == FALSE)
	{
          /* This process is defunct, so indicate that it is dead,
          ** to prevent other servers from trying to rundown the 
          ** process - as we're about to release the lgd_lpb_q_mutex
          */
          lpb->lpb_status |= LPB_DEAD;

	    /*
	    ** LG_rundown may change the LPB queue, so we must release
	    ** its mutex. When we come back, we'll reacquire the
	    ** queue mutex and continue the search from the top
	    ** of the queue.
	    */
	    (VOID)LG_unmutex(&lgd->lgd_lpb_q_mutex);

	    /* process does not exist anymore */

	    uleFormat(NULL, E_DMA469_PROCESS_HAS_DIED, (CL_ERR_DESC *)NULL,
			    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 1,
			    0, saved_pid);

	    LK_rundown(saved_pid);
	    LG_rundown(saved_pid);
	    LGK_deadinfo(saved_pid);

	    (VOID)LG_mutex(SEM_EXCL, &lgd->lgd_lpb_q_mutex);
	    lpb_offset = lgd->lgd_lpb_next;
	}
    }

    (VOID)LG_unmutex(&lgd->lgd_lpb_q_mutex);

#ifdef UNIX
    /* Call cs to cleanup up any programs which may have owned a server
    ** slot but never have exited without freeing it.  This includes
    ** programs which may never have opened any database (and thus are
    ** not known to LG and may not show up in the above "check for dead
    ** server loop".  Programs that fall into that category are ipm, logstat,
    ** ...
    */

    CS_check_dead();
#endif

    if (lgd->lgd_lwlxb.lwq_count != 1)
	return (OK);

    /*
    ** Check for unassigned buffers, and logwriter sleeping
    */
    lwq = (LXBQ *)LGK_PTR_FROM_OFFSET(lgd->lgd_lwlxb.lwq_lxbq.lxbq_next);
    lwlxb = (LXB *)((char *)lwq - CL_OFFSETOF(LXB,lxb_wait));
    if (lwlxb->lxb_assigned_buffer == 0)
    {
	LFB			*lfb = &lgd->lgd_local_lfb;
	LBB			*lbb = NULL;
	LBB			*scan_lbb;
	SIZE_TYPE		wq_end_offset = 
				LGK_OFFSET_FROM_PTR(&lfb->lfb_wq_next);

	/* Lock the write queue */
	LG_mutex(SEM_EXCL, &lfb->lfb_wq_mutex);

	/* Look at the first buffer */
	if (lfb->lfb_wq_next != wq_end_offset)
	{
	    scan_lbb = (LBB *)LGK_PTR_FROM_OFFSET(lfb->lfb_wq_next);
	    if ( (scan_lbb->lbb_state & LBB_PRIM_IO_NEEDED) && !scan_lbb->lbb_owning_lxb)
		lbb = scan_lbb;
	}

	/* Release the write queue, mutex and recheck this buffer */
	LG_unmutex(&lfb->lfb_wq_mutex);

	if ( lbb )
	{
	    /* Lock the LBB's mutex, check it again */
	    LG_mutex(SEM_EXCL, &lbb->lbb_mutex);
	    if ( (lbb->lbb_state & LBB_PRIM_IO_NEEDED) && !lbb->lbb_owning_lxb
			&& lwlxb->lxb_assigned_buffer == 0)
	    {
#ifdef xDEBUG
		TRdisplay("check_dead_thread assign_logwriter!\n");
#endif
		assign_logwriter(lgd, lbb, NULL); /* FIX ME lpb */
	    }
	    (VOID)LG_unmutex(&lbb->lbb_mutex);
	}
    }	    

    return (OK);
}

/*{
** Name: LG_lpb_shrbuf_disconnect - Disconnect proc from shared buffer manager.
**
** Description:
**	This routine does the work associated with the exit of a process
**	that is connected to a shared buffer manager.
**
**	It should be called whenever an LGcall is made by a server to clean
**	up its LPB.  It does not need to be called when an LPB is cleaned up
**	through an LG call made by the RCP.
**
**	This routine checks if this is the last connection to the shared buffer
**	manager.  If it is then it looks for servers waiting to be recovered
**	and runs the LG_rundown routine for those servers.
**
**	The LG mutex should be held when this routine is called.  It may
**	be released by this routine and reaquired before returning.
**
** Inputs:
**	bm_id	    - id of shared buffer manager.
**	lpb	    - lpb of exiting process.
**
** Outputs:
**	Returns:
**	    None
**	Exceptions:
**	    None
**
** Side Effects:
**	None
**
** History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable logging and locking system.
**	24-Jan-1996 (jenjo02)
**	    Mutex granularity project. lgd_mutex replaced with more
**	    granular LPD queue mutex. Rewrote to use a single pass
**	    thru the LPB queue instead of two.
*/
VOID
LG_lpb_shrbuf_disconnect(i4    bm_id)
{
    register LGD    *lgd = (LGD *)LGK_base.lgk_lgd_ptr;
    register LPB    *lpb;
    SIZE_TYPE	    end_offset;
    SIZE_TYPE	    lpb_offset;

    /*
    ** Check for any connections left to this buffer manager.
    ** If there are still servers connected to it then there is
    ** nothing for us to do.
    **
    ** If there are no active servers left, but there are LPB_DEAD
    ** process control blocks left around, then call LG_rundown for
    ** each DEAD server to activate recovery.
    */

    if (LG_mutex(SEM_EXCL, &lgd->lgd_lpb_q_mutex))
	return;

    end_offset = LGK_OFFSET_FROM_PTR(&lgd->lgd_lpb_next);
    lpb_offset = lgd->lgd_lpb_next;

    while (lpb_offset != end_offset)
    {
	lpb = (LPB *)LGK_PTR_FROM_OFFSET(lpb_offset);

	lpb_offset = lpb->lpb_next;

	if (lpb->lpb_status & LPB_SHARED_BUFMGR &&
	    lpb->lpb_bufmgr_id == bm_id)
	{
	    if ((lpb->lpb_status & LPB_DEAD) == 0)
	    {
		(VOID)LG_unmutex(&lgd->lgd_lpb_q_mutex);
		return;
	    }
	    if ((lpb->lpb_status & LPB_RUNAWAY) == 0)
	    {

		/*
		** Call rundown routine for each server so recovery will 
		** clean up its transactions.  Must start loop at top of 
		** lpb list after releasing LPB queue mutex.
		**
		** Before calling rundown, mark lpb as LPB_FOREIGN_RUNDOWN 
		** so LG_rundown will know it is in a recursive call.
		*/
		lpb->lpb_status |= LPB_FOREIGN_RUNDOWN;
		(VOID) LG_unmutex(&lgd->lgd_lpb_q_mutex);

		LG_rundown(lpb->lpb_pid);

    /*** LK_rundown ? ? */

		(VOID) LG_mutex(SEM_EXCL, &lgd->lgd_lpb_q_mutex);

		lpb_offset = lgd->lgd_lpb_next;

#if 0
		lpb->lpb_status &= ~LPB_FOREIGN_RUNDOWN; /*hmmm isnt it gone?*/
#endif
	    }
	}
    }
    (VOID)LG_unmutex(&lgd->lgd_lpb_q_mutex);

    return;
}

/*{
** Name: LG_lpb_shrbuf_nuke - Kill all svrs connected to specified shared cache.
**
** Description:
**	This routine kills off all servers which are connected to a shared
**	buffer manager so that recovery can proceed on databases open by
**	that buffer manager.
**
**	It is used whenever a shared-buffer-manager server dies abnormally
**	and other servers are connected to the same cache.  In this case, all
**	those other servers must be terminated before recovery can proceed.
**
**	This routine cycles through the LPB list looking for servers with
**	the same buffer manager id and does a kill -9 on them.
**
**	This routine must make sure that it does not kill servers while
**	holding critical installation resources, as this will hang the system.
**	To ensure this, this routine collects all such critical resources
**	before terminating any servers.  Currently, these critical resources
**	are:
**
**	    LG_mutex : this is assumed to be held by the caller of this routine.
**	    LK_mutex : currently, this is the same mutex as LG_mutex.
**	    LGK ACB list semaphore : must be acquired.
**
** 	NOTE - If the system semaphores are broken up, we may have to add more 
**	semaphore requests here.
**
** Inputs:
**	bm_id	    - id of shared buffer manager.
**
** Outputs:
**	Returns:
**	    None
**	Exceptions:
**	    None
**
** Side Effects:
**	None
**
** History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable logging and locking system.
**	24-may-1993 (bryanp)
**	    Fix arguments to PCforce_exit(). This fixes some shared cache
**		rundown problems.
**	18-oct-1993 (bryanp)
**	    Added get_critical_sems implementation, patterned on 6.4 one.
**	24-Jan-1996 (jenjo02)
**	    New semaphores to walk LPB queue.
**	    lgd_lpb_q_mutex must be held EXCL on entry.
**	14-mar-96 (nick)
**	    [get|rel]_critical_sems() no longer have a parameter.
**	27-apr-99 (thaju02)
**	    added parantheses to check of 
**	    ((lpb->lpb_status & (LPB_DEAD | LPB_DYING)) == 0).
**	    When a shared buffer manager server dies abnormally, the
**	    other servers connected to the same cache were not being
**	    killed, resulting in installation hang. (B96725)
**	26-may-99 (thaju02)
**	    Avoid killing myself here, wait until all the mutexes
**	    have been released. (B96725)
*/
VOID
LG_lpb_shrbuf_nuke(i4 bm_id, PID *nuke_me)
{ 
    register LGD    *lgd = (LGD *)LGK_base.lgk_lgd_ptr;
    register LPB    *lpb;
    SIZE_TYPE lpb_offset;
    SIZE_TYPE end_offset;
    STATUS  status;
    CL_ERR_DESC	    sys_err;
    PID		    my_pid;

    /* what is my pid */
    PCpid(&my_pid);

    /*
    ** Before killing any servers, we must first obtain any Installation 
    ** critical resources that servers can hold to make sure that a server
    ** is not holding them when we kill it.
    **
    ** This prevents the server from exiting while in the middle of changing 
    ** an LGK control structure or while holding a system semaphore (which will
    ** cause the system to hang).
    **
    ** If we fail trying to obtain this semaphore, there is really not much we
    ** can do - we can't fail to execute rundown as that would be bad.  We just
    ** continue and kill the server hoping that it isn't holding a critical 
    ** resource.
    */
    status = get_critical_sems();
    if (status)
	TRdisplay("LG_lpb_shrbuf_nuke: get_critical_sems returns %d\n", status);

    /*
    ** Look for other servers connected to this buffer manager.
    ** Kill any that are still active. We don't check the return code from
    ** PCforce_exit because we don't really have any recovery action if it
    ** should fail; furthermore, the process which we're trying to call
    ** PCforce_exit() on may already have died, and so PCforce_exit not
    ** uncommonly fails with a "non-existent process" error that is actually
    ** not an error at all in this case.
    **
    ** Note that the caller has already locked lgd_lpb_q_mutex EXCL
    */

    end_offset = LGK_OFFSET_FROM_PTR(&lgd->lgd_lpb_next);

    for (lpb_offset = lgd->lgd_lpb_next;
	 lpb_offset != end_offset;
	 lpb_offset = lpb->lpb_next)
    {
	lpb = (LPB *)LGK_PTR_FROM_OFFSET(lpb_offset);

	if ((lpb->lpb_status & LPB_SHARED_BUFMGR) &&
	    (lpb->lpb_bufmgr_id == bm_id) &&
	    ((lpb->lpb_status & (LPB_DEAD | LPB_DYING)) == 0))
	{
	    if (lpb->lpb_pid == my_pid)
	    {
		/* 
		** avoid killing myself here, because at this
		** point-in-time, I am holding the 'critical'
		** semaphores along with the lpb queue mutex.
		*/
		*nuke_me = my_pid;
	    }
	    else
	    {
	        (void) PCforce_exit(lpb->lpb_pid, &sys_err);
	        lpb->lpb_status |= LPB_DYING;
	    }
	}
    }

    /*
    ** Release ACB list semaphore.
    */
    status = rel_critical_sems();
    if (status)
	TRdisplay("LG_lpb_shrbuf_nuke: rel_critical_sems returns %d\n", status);

    return;
}

/*{
** Name: LG_lpb_node_nuke - Kill all servers in this cluster node.
**
** Description:
**	This routine kills off all servers in a cluster node due to the   
**	death of the RCP so that node recovery can take place.
**
**	It is used whenever the RCP dies abnormally and servers remain.
**	In this case, all servers must be terminated before node recovery 
**	can proceed.
**
**	This routine cycles through the LPB list looking for servers
**	and does a kill -9 on them.
**
**	This routine must make sure that it does not kill servers while
**	holding critical installation resources, as this will hang the system.
**	To ensure this, this routine collects all such critical resources
**	before terminating any servers.
**
** 	NOTE - If the system semaphores are broken up, we may have to add more 
**	semaphore requests here.
**
** Inputs:
**
** Outputs:
**	Returns:
**	    None
**	Exceptions:
**	    None
**
** Side Effects:
**	None
**
** History:
**	10-Apr-2007 (jonj)
**	    Created.
*/
VOID
LG_lpb_node_nuke(PID *nuke_me)
{ 
    register LGD    *lgd = (LGD *)LGK_base.lgk_lgd_ptr;
    register LPB    *lpb;
    SIZE_TYPE 	    lpb_offset;
    SIZE_TYPE 	    end_offset;
    STATUS  	    status;
    CL_ERR_DESC	    sys_err;
    PID		    my_pid;

    /* what is my pid */
    PCpid(&my_pid);

    /*
    ** Before killing any servers, we must first obtain any Installation 
    ** critical resources that servers can hold to make sure that a server
    ** is not holding them when we kill it.
    **
    ** This prevents the server from exiting while in the middle of changing 
    ** an LGK control structure or while holding a system semaphore (which will
    ** cause the system to hang).
    **
    ** If we fail trying to obtain this semaphore, there is really not much we
    ** can do - we can't fail to execute rundown as that would be bad.  We just
    ** continue and kill the server hoping that it isn't holding a critical 
    ** resource.
    */
    status = get_critical_sems();
    if (status)
	TRdisplay("LG_lpb_node_nuke: get_critical_sems returns %d\n", status);

    /*
    ** Look for other servers in this node.
    ** Kill any that are still active. We don't check the return code from
    ** PCforce_exit because we don't really have any recovery action if it
    ** should fail; furthermore, the process which we're trying to call
    ** PCforce_exit() on may already have died, and so PCforce_exit not
    ** uncommonly fails with a "non-existent process" error that is actually
    ** not an error at all in this case.
    **
    ** Note that the caller has already locked lgd_lpb_q_mutex EXCL
    */

    end_offset = LGK_OFFSET_FROM_PTR(&lgd->lgd_lpb_next);

    for (lpb_offset = lgd->lgd_lpb_next;
	 lpb_offset != end_offset;
	 lpb_offset = lpb->lpb_next)
    {
	lpb = (LPB *)LGK_PTR_FROM_OFFSET(lpb_offset);

	if ( (lpb->lpb_status & 
		(LPB_ARCHIVER | LPB_MASTER | LPB_DEAD | LPB_DYING)) == 0 )
	{
	    if (lpb->lpb_pid == my_pid)
	    {
		/* 
		** avoid killing myself here, because at this
		** point-in-time, I am holding the 'critical'
		** semaphores along with the lpb queue mutex.
		*/
		*nuke_me = my_pid;
	    }
	    else
	    {
	        (void) PCforce_exit(lpb->lpb_pid, &sys_err);
	        lpb->lpb_status |= LPB_DYING;
	    }
	}
    }

    status = rel_critical_sems();
    if (status)
	TRdisplay("LG_lpb_node_nuke: rel_critical_sems returns %d\n", status);

    return;
}

/*
** Name: get_critical_sems	    - get installation critical resources.
**
** Description:
**	This routine is used by LG_rundown when it wants to kill off
**	selected servers without bringing down the entire installation.
**
**	To do this, LG must make sure that when it kills a server, that the
**	server is not holding any system critical semaphore - otherwise the
**	installation could be left in a corrupted state.
**
**	To ensure that the servers will not be holding any system semaphores
**	when they die, this routine collects all such semaphores on behalf
**	of the calling process.
**
**	Currently there is no CL registry of critical resources, nor is there
**	any mainline registry. This routine takes its best shot by acquiring
**	the following critical semaphores:
**
**	    LG_mutex		    : held by our caller already.
**	    LK_mutex		    : to lock out locking system activities.
**	ifdef UNIX
**	    Cs_sm_cb->css_spinlock  : to lock out CS shmem activities.
**	endif
**
**	Other semaphores which we might need to consider holding include:
**
**	    Lgk_mem->mem_ext_sem    : to lock out internal LG/LK memory mgt
**
**	Currently, we do not bother to acquire these semaphores because these
**	semaphores are only taken while also holding either the LG_mutex or the
**	LK_mutex, so we argue that we've locked them out effectively already.
**
**	Based on testing, we may need to add additional semaphores to the list.
**
** Inputs:
**	none.
**
** Outputs:
**	none.
**
**	Returns:
**	    STATUS
**
** History:
**      24-jul-89 (rogerk)
**	    created.
**	18-oct-1993 (bryanp)
**	    6.5 version for the "portable" LG/LK (not so portable, here...)
**	14-mar-96 (nick)
**	   LKD mutex no longer exists - call LK to collect all appropos
**	   semaphores.
**	29-Mar-1996 (jenjo02)
**	   Lock all LG semaphores necessary to prevent other processes
**	   from executing LG requests. lgd_mutex no longer suffices.
**	08-Oct-1996 (jenjo02)
**	    In get_critical_sems, rel_critical_sems, call CSp|v_semaphore
**	    directly instead of using LG_mutex|unmutex to avoid cluttering
**	    the log with misleading errors.
**	14-Nov-2007 (jonj)
**	    Do -not- acquire lgd_mutex, lfb_mutex as holding
**	    these may induce mutex deadlocks with still running
**	    threads.
*/
static STATUS
get_critical_sems(void)
{
    register LGD  *lgd = (LGD *)LGK_base.lgk_lgd_ptr;
    register LPB  *lpb;
    register LXB  *lxb;
    SIZE_TYPE	*lpbb_table;
    SIZE_TYPE	*lxbb_table;
    i4	i;
    STATUS	status;
    STATUS	ret_status = OK;

    /*
    ** First, take all semaphores which might be used by
    ** an external LG function. If we fail to get a mutex,
    ** keep going.
    */ 

    /* 
    ** LGadd(), LGclose(), LGremove() enter on an LPB, 
    ** so lock all LPBs.
    */
    (VOID)CSp_semaphore(SEM_EXCL, &lgd->lgd_lpbb_mutex);
    lpbb_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lgd->lgd_lpbb_table);
    for (i = 1; i <= lgd->lgd_lpbb_count; i++)
    {
	lpb = (LPB *)LGK_PTR_FROM_OFFSET(lpbb_table[i]);
	if (lpb->lpb_type == LPB_TYPE)
	    (VOID)CSp_semaphore(SEM_EXCL, &lpb->lpb_mutex);
    }

    /*
    ** LGend(), LGforce(), LGreserve(), LGwrite() enter on an LXB,
    ** so lock all LXBs and the LXBB.
    */
    (VOID)CSp_semaphore(SEM_EXCL, &lgd->lgd_lxbb_mutex);
    lxbb_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lgd->lgd_lxbb_table);
    for (i = 1; i <= lgd->lgd_lxbb_count; i++)
    {
	lxb = (LXB *)LGK_PTR_FROM_OFFSET(lxbb_table[i]);
	if (lxb->lxb_type == LXB_TYPE)
	    (VOID)CSp_semaphore(SEM_EXCL, &lxb->lxb_mutex);
    }

    /*
    ** Get critical LK semaphores
    */
    if (status = LK_get_critical_sems())
    {
	TRdisplay("LG_rundown: get_critical_sems LK_get_critical_sems, status: %d\n",
				status);
	ret_status = status;
    }

#ifdef UNIX
    CS_get_critical_sems();
#endif

    return (ret_status);
}

/*
** Name: rel_critical_sems: Release the critical semaphore resources.
**
** Description:
**	This routine is called once we have killed off a process. Since we
**	were holding the resources ourselves, we believe that the other
**	process is not holding them, so we can now release them.
**
**	This routine must be matched to get_critical_sems -- whatever that
**	routine acquires, this routines should release.
**
** Inputs:
**	None
**
** Outputs:
**	None.
**
** Returns:
**	STATUS
**
** History:
**	18-oct-1993 (bryanp)
**	    Created, patterned after the 6.4 support.
**	14-mar-96 (nick)
**	    Let LK decide which semaphores to release.
**	29-Mar-1996 (jenjo02)
**	   Unlock all LG semaphores acquired by get_critical_sems.
**	08-Oct-1996 (jenjo02)
**	    In get_critical_sems, rel_critical_sems, call CSp|v_semaphore
**	    directly instead of using LG_mutex|unmutex to avoid cluttering
**	    the log with misleading errors.
**	14-Nov-2007 (jonj)
**	    Do -not- acquire lgd_mutex, lfb_mutex as holding
**	    these may induce mutex deadlocks with still running
**	    threads.
*/
static STATUS
rel_critical_sems(void)
{
    register LGD  *lgd = (LGD *)LGK_base.lgk_lgd_ptr;
    register LPB  *lpb;
    register LXB  *lxb;
    SIZE_TYPE	*lpbb_table;
    SIZE_TYPE	*lxbb_table;
    i4	i;

#ifdef UNIX
    CS_rel_critical_sems();
#endif

    /*
    ** Release LK semaphores
    */
    (VOID)LK_rel_critical_sems();

    /* 
    ** Unwind all LG semaphores acquired by get_critical_sems(),
    ** releasing them in the opposite order in which acquired, and
    ** ignoring status (we may or may not have actually acquired
    ** a particular semaphore).
    */

    lxbb_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lgd->lgd_lxbb_table);
    for (i = lgd->lgd_lxbb_count; i ; i--)
    {
	lxb = (LXB *)LGK_PTR_FROM_OFFSET(lxbb_table[i]);
	if (lxb->lxb_type == LXB_TYPE)
	    (VOID)CSv_semaphore(&lxb->lxb_mutex);
    }
    (VOID)CSv_semaphore(&lgd->lgd_lxbb_mutex);

    lpbb_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lgd->lgd_lpbb_table);
    for (i = lgd->lgd_lpbb_count; i ; i--)
    {
	lpb = (LPB *)LGK_PTR_FROM_OFFSET(lpbb_table[i]);
	if (lpb->lpb_type == LPB_TYPE)
	    (VOID)CSv_semaphore(&lpb->lpb_mutex);
    }
    (VOID)CSv_semaphore(&lgd->lgd_lpbb_mutex);

    return(OK);
}

/*{
** Name: LG_Disable_dual_logging - Disable the dual logging and signal RCP
**
** Description:
**	This routine disables dual logging and signals RCP that this event
**	has occurred. We also request that the log file header(s) be written
**	so that the proper log file state is recorded in the logfile itself.
**
** Inputs:
**	active_log  - active log after disabling the dual logging.
** Outputs:
**	Returns:
**	    None
**	Exceptions:
**	    None
**
** Side Effects:
**	None
**
** History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable logging and locking system.
**	22-oct-1992 (bryanp)
**	    New logging and locking system version.
**	    Call write_headers() to update the log file state in the logfile.
**	    Only bother disabling dual logging and writing the headers if the
**	    master is still alive. If lgd->lgd_master_lpb is 0, then the
**	    master is already running down and so we don't want to try to do
**	    any further I/O.
**	24-Jan-1996 (jenjo02)
**	    New mutex for LFB.
*/
VOID
LG_disable_dual_logging(i4 active_log, register LFB *lfb)
{
    register LGD *lgd = (LGD *)LGK_base.lgk_lgd_ptr;
    LG_ID   lx_id;
    STATUS  status;
    i4 err_code;

    if (LG_mutex(SEM_EXCL, &lfb->lfb_mutex))
	return;
    lfb->lfb_status &= ~LFB_DUAL_LOGGING;
    lfb->lfb_status |= LFB_DISABLE_DUAL_LOGGING;
    lfb->lfb_active_log = active_log;
    (VOID)LG_unmutex(&lfb->lfb_mutex);

    if (lgd->lgd_master_lpb != 0) /* if master is still alive */
    {
	LG_signal_event(LGD_DISABLE_DUAL_LOGGING, 0, FALSE);

	/*
	** Update the log file header(s) on disk to show the new logging state.
	** By passing a 0 transaction ID, we notify write_headers that we
	** cannot support suspend/resume processing here.
	*/
	lx_id.id_id = lx_id.id_instance = 0;
	status = LG_write_headers( &lx_id, lfb, (STATUS *)NULL );
	if (status)
	    uleFormat(NULL, status, (CL_ERR_DESC *)NULL,
		    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 0);

	uleFormat(NULL, E_DMA467_DISABLE_LOGGING, (CL_ERR_DESC *)NULL,
		    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 0);

	lfb->lfb_status &= ~LFB_DISABLE_DUAL_LOGGING;
    }
}

/*{
** Name: LG_check_logfull - Check for logfull/force_abort conditions
**
** Description:
**      This routine is called whenever the log is full enough (greater
**      than lgd_checkstall) to warrant checking for logfull or force-abort
**      conditions.
**
**      If we have reached the force_abort limit, then we must find the oldest
**      transaction in the system and signal it to be aborted.
**
**      If we have reached the logfull limit, then we must stall the logging
**      system until logspace is freed up.  Usually this means until the oldest
**      transactions get aborted.  (Note that if we hit logfull we will signal
**      force_abort even if we have not actually reached the force_abort limit).
**
**      Logfull is computed in one of two ways.  First there is an installation
**      startup parameter that gives the logfull limit. Usually it will be set
**      to around 95% of the logfile size.  When the logfill becomes this full,
**      then LOGFULL will be signaled.
**
**      Secondly, the logging system keeps track of how much logspace would
**      be required to recover all of the active transactions in the system
**      should a machine failure occur.  These reqirements are described below.
**
**      Should the logfile reach a limit that exceeds either of the two logfull
**      limits, then LOGFULL will be signaled.
**
**      Should the logging system recognize that it is approaching a logfull
**      condition while executing a consistency point, it will stall the logging
**      system until the CP is complete.  This is because filling up the logfile
**      during a CP often leads to force-aborting all active transactions
**      (a result of moving the logfile BOF marker only by CP boundaries).
**      If we stall the logging system for this reason, we will not signal
**      force_abort.
**
**
**      Requirements for computing dynamic logfull limit.
**
**          The logging system must make sure that there is always enough space
**          to recover all the active transactions.  This requires there to be
**          available:
**
**	   	- space to writes CLRs for all updates in all active 
**		  transactions, reflected by lgd_reserved_space and
**		  (factored as part of the used logspace below).
**
**              - one log buffer for each active transaction.  This is
**                required to write and force each End Transaction record.
**
**              - one log buffer for each active DMU operation.  This is
**                required so the system can write and flush a DM0L_ABORTSAVE
**                record each time it completes recovery on a DMU operation.
**
**              - two log buffers in order to execute two Consistency Points.
**                It may be necessary to write and flush ECP records in order
**                to reclaim space for transactions after being recovered.
**
**              - six more buffers; for no particular reason, just to be safe.
**                Just in case someone adds something that requires more disk
**                space without altering this algorithm, we get a little leeway
**                to notice it internally before we corrupt anybody's database.
**
** Inputs:
**      None
**
** Outputs:
**      Returns:
**          None
**      Exceptions:
**          None
**
** Side Effects:
**      None
**
** History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable logging and locking system.
**	03-jan-1993 (jnash)
**          Reduced Logging project.   This project adds more complexity in 
**	    logfull calculations, and we haven't yet worked out solutions 
**	    to all the problems.  Basically, it becomes critical that 
**	    we know exactly what logging takes place writes after LOGFULL 
**	    (the "real" logfull -- not the CP stall) has been signalled.   
**	    This project includes reserved space to that previously 
**	    noted, and also includes one additional log buffer for each 
**	    CP that may be performed during transaction backout,
**	    with the reserved space to estimate the number of CPs.
**	    This is not perfect as CPs are variable length.
**	    A more difficult problem, not resolved here, can be viewed
**	    at its most pathological by considering a 1 page cache.  Then,
**	    each page touched during recovery causes the buffer manager 
**	    to force another page to make room.  These forces play havoc 
**	    with the space reservation mechanism, and so far we have no 
**	    solution.  We just assume for now that the cache is large 
**	    enough that this doesn't occur.
**	18-oct-1993 (rogerk)
**	    Changed to return immediately if the used space is below the
**	    check-stall limit.  Put this check in here and removed the similar
**	    check-stall check from the current callers of check_logfull.
**	    Added new logfull checking calculations.  Removed accounting
**	    for CPs in recovery as this is now done by LG_calc_reserved_space.
**	    Removed use of lgd_dmu_cnt as its function was superseeded by
**	    logspace reservation in 6.5.  Added local computation of the
**	    lgd_stall_limit, it is no longer used outside of this routine.
**	    Added new checks for logfile size growing during logfull conditions
**	    and we now crash the installation if the logfile gets near the
**	    point at which it will wrap around.
**	04-jan-1994 (chiku)
**          Changed LG_chekc_logfull() to call LG_unmutex() before PCExit() on 
**          logfull conditions.  [BUG#: 56702].
**	15-apr-1994 (chiku)
**          Bug56702: Changed LG_chekc_logfull() to return E_DM9070_LOG_LOGFULL on
**          logfull conditions.
**	24-Jan-1996 (jenjo02)
**	    New mutexes for LFB, queues, and other structures.
**	    Added lgd_archiver_lpb so we don't have to lock and search
**	    the lpb queue to find the Archiver's LPB.
**	29-May-1996 (jenjo02)
**	    Added lxb parm to LG_check_logfull() to pass to  LG_abort_oldest().
**	    LG_abort_oldest() may abort the LXB which is running the
**	    current thread, in which case its mutex is already locked -
**	    the function needs to know that.
**	02-Mar-2000 (jenjo02)
**	    Compute lgd_stall_limit using lgd_protect_count (number of active
**	    transactions) rather than lgd_lxb_inuse (number of active and
**	    inactive transactions). Only active ones will need space for
**	    an ET.
**	5-Jan-2006 (kschendel)
**	    Reserved space totals change to u_i8 for large logs, fix here.
*/
STATUS
LG_check_logfull(register LFB *lfb, LXB *lxb)
{
    register LGD *lgd = (LGD *)LGK_base.lgk_lgd_ptr;
    i4     used;
    i4	lgd_stall_limit;
    i4	lgd_crash_limit;
    i4	lgd_death_limit;
    i4	error;
    STATUS	status;


    /*
    ** Calculate log space currently in use, including reserved space.
    ** If the used space is beyond the checkstall limit, then continue
    ** on to check for various logfull and stall limits.
    **
    ** The check_stall limit is an optimization to bypass all the space
    ** calculations below when we obviously have logspace to spare.
    */
    used = LG_used_logspace(lfb, TRUE);
    if (used <= lgd->lgd_check_stall)
	return(OK);

    /* lock lgd_mutex to block all that BOF/EOF stuff */
    if (status = LG_mutex(SEM_EXCL, &lgd->lgd_mutex))
	return(status);

    /*
    ** Check if the archiver is running - if the archiver has died and
    ** there are journaled transactions running then the log file will
    ** fill up pretty fast.
    */
    if (lgd->lgd_archiver_lpb == 0)
        LG_signal_event(LGD_START_ARCHIVER, 0, TRUE);

    /*
    ** Check if we can move the BOF forward or if the archiver can
    ** be signaled to free up some log space.  Normally, we only
    ** wake up the archiver if lgh_cpcnt Consistency Points are
    ** stacked up ready for archiving.  Here we wake it up if any
    ** CP's are available to archive.
    **
    ** NOTE: Don't want to compute_bof() during RCP machine failure
    ** recovery. The BOF would get set to the last CP and hence lose
    ** transactions that should be recovered.
    */
    if (lgd->lgd_status & LGD_ONLINE)
    {
        LG_compute_bof(lfb, (i4)0);

	/* Recompute used logspace after possibly moving BOF forward. */
	used = LG_used_logspace(lfb, TRUE);
    }

    /*
    ** If the abort limit has been reached signal the recovery system
    ** to abort oldest transaction(s).  The oldest transaction will
    ** be force-aborted only if it is holding up the system.  If the
    ** oldest transaction is not in the oldest CP window, then something
    ** else (probably the archiver) is holding up moving the BOF forward.
    */
    if ((used > lfb->lfb_header.lgh_l_abort) &&
        ((lgd->lgd_status & LGD_FORCE_ABORT) == 0))
    {
	/* LG_abort_oldest will take lgd_mutex, so release it */
	(VOID)LG_unmutex(&lgd->lgd_mutex);
        LG_abort_oldest(lxb);
	if (status = LG_mutex(SEM_EXCL, &lgd->lgd_mutex))
	    return(status);
    }

    /*
    ** Determine various LOGFULL limits:
    **
    **       A) lgd_stall_limit : Logfull stall point.
    **
    **		Determined from the system logfull setting.  We move it back
    **		one log buffer for each active transaction (to ensure we can
    **		force the ET records) and add in an arbitrary fudge factor.
    **
    **		When this limit is reached new LGreserve requests will hang
    **	        in LOGFULL state.  Sessions which have already reserved needed
    **		logspace are able to continue (rollbacks for example).
    **
    **	     B) lgd_crash_limit : Logfull panic point.
    **
    **		Set to 98% of the logfile size.  If the used + reserved space
    **		space reaches this amount in an online system, then the system
    **		will be brought down to attempt offline recovery.
    **
    **		The hope is that this limit can never be reached since:
    **
    **		  - The Logfull Stall Point must be before this, and no new
    **		    space can be reserved after that limit is reached.
    **
    **		  - During LOGFULL, the only log writes that are allowed are
    **		    those associated with rollback processing which should be
    **		    using up already reserved space.
    **
    **		If the used-space amount increases during LOGFULL conditions
    **		then it may be sign that the logspace reservation algorithms
    **		are not working properly and we are using up logspace faster
    **		than expected.
    **
    **		We shut down and fail over to offline recovery with the hope
    **		that in that more controlled environment we will waste less
    **		log space.  We do not want to reach condition (C) below.
    **
    **       C) lgd_death_limit: Total devastation
    **
    **		Set to 100% of the logfile size (minus 5 blocks).
    **
    **		If the actual used space (not reserved) reaches this limit
    **		then we are toast.  We stop processing (even in offline
    **		recovery) to avoid wrapping around the log file and writing
    **		over important log records.
    **
    **		When this happens the user must initialize the logfile.
    **
    **		The 5 blocks are left around in the hopes that we can write
    **		a mechanism that can bring the system up without recovering
    **		databases so we can archive out the current logfile.  If we
    **		can archive the journal records then at least the user can
    **		rollforward the open databases to recover them.
    */
  
    lgd_stall_limit = lfb->lfb_header.lgh_l_logfull - lgd->lgd_protect_count - 15;
    lgd_crash_limit = (lfb->lfb_header.lgh_count * 98) / 100;
    lgd_death_limit = lfb->lfb_header.lgh_count - 5;


    /*
    ** If we have reached the lgd_stall limit then signal logfull (and force
    ** abort if it has not already been signaled).
    */
    if (used > lgd_stall_limit)
    {
        LG_signal_event(LGD_LOGFULL, 0, TRUE);
        if ((lgd->lgd_status & LGD_FORCE_ABORT) == 0)
	{
	    /* LG_abort_oldest will take lgd_mutex, so release it */
	    (VOID)LG_unmutex(&lgd->lgd_mutex);
            LG_abort_oldest(lxb);
	    if (status = LG_mutex(SEM_EXCL, &lgd->lgd_mutex))
		return(status);
	}
    }

    /*
    ** If we are executing a consistency point and we are past the cpstall
    ** limit, then signal logfull, but don't signal force-abort.  This prevents
    ** us encompassing too much of the logfile in a single CP window - which
    ** can lead to unfriendly force-abort behavior.
    */
    if ((used > lgd->lgd_cpstall) && (lgd->lgd_no_bcp))
        LG_signal_event(LGD_LOGFULL, 0, TRUE);

    /*
    ** If the logfile grows to 98% used then we begin to get a bit nervous.
    ** We only allow new writes to proceed in offline recovery.
    */
    if (used > lgd_crash_limit)
    {
	if (lgd->lgd_status & LGD_ONLINE)
	{
	    uleFormat(NULL, E_DMA48E_LOGFULL_EXCEEDED, (CL_ERR_DESC *)NULL,
		    ULE_LOG, NULL, NULL, 0, NULL, &error, 7,
		    0, lfb->lfb_header.lgh_begin.la_sequence,
		    0, lfb->lfb_header.lgh_begin.la_block,
		    0, lfb->lfb_header.lgh_begin.la_offset,
		    0, lfb->lfb_header.lgh_end.la_sequence,
		    0, lfb->lfb_header.lgh_end.la_block,
		    0, lfb->lfb_header.lgh_end.la_offset,
		    sizeof(lfb->lfb_reserved_space), &lfb->lfb_reserved_space);

	    LG_signal_event(LGD_EMER_SHUTDOWN | LGD_IMM_SHUTDOWN, 0, TRUE);
	    (VOID)LG_unmutex(&lgd->lgd_mutex);
	    return(E_DM9070_LOG_LOGFULL);
	}

	/*
	** Recalculate used space ignoring any reserved bytes.  Don't allow
	** the system to write past lgd_death_limit.
	*/
	used = LG_used_logspace(lfb, FALSE);
	if (used > lgd_death_limit)
	{
	    uleFormat(NULL, E_DMA48F_LOGFILE_FULL, (CL_ERR_DESC *)NULL,
		    ULE_LOG, NULL, NULL, 0, NULL, &error, 7,
		    0, lfb->lfb_header.lgh_begin.la_sequence,
		    0, lfb->lfb_header.lgh_begin.la_block,
		    0, lfb->lfb_header.lgh_begin.la_offset,
		    0, lfb->lfb_header.lgh_end.la_sequence,
		    0, lfb->lfb_header.lgh_end.la_block,
		    0, lfb->lfb_header.lgh_end.la_offset,
		    sizeof(lfb->lfb_reserved_space), &lfb->lfb_reserved_space);

	    LG_signal_event(LGD_EMER_SHUTDOWN | LGD_IMM_SHUTDOWN, 0, TRUE);
	    (VOID)LG_unmutex(&lgd->lgd_mutex);
	    return(E_DM9070_LOG_LOGFULL);

	}
    }

    (VOID)LG_unmutex(&lgd->lgd_mutex);

    return(OK);
}

/*{
** Name: LG_cleanup_checkpoint - Clean up backup state on database.
**
** Description:
**	This routine is used to clean up the system and database backup
**	states following the exit of a checkpoint process - or the change
**	in state of a database undergoing backup.
**
**	If called with an LPB parameter, then the process passed in should
**	be a checkpoint process which is exiting.  The database it has
**	open is checked for necessary cleanup - that is turning off the
**	BACKUP states if no other checkpoint is running on it.
**
**	It then checks whether the system backup status needs to be reset.
**	If there are no other checkpoints running in the system, then the
**	system backup state is turned off.
**
**	This routine can be called with a Zero LPB argument just to calculate
**	the appropriate system backup status based on status values of
**	the currently open databases.
**
** Inputs:
**	lpb		    Process control block of exiting process.
**
** Outputs:
**	Returns:
**	    None
**	Exceptions:
**	    None
**
** Side Effects:
**	None
**
** History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable logging and locking system.
**	15-mar-1993 (rogerk)
**	    Reduced Logging - Phase IV:
**	    Removed system EBACKUP, FBACKUP and database EBACKUP states.
**	24-Jan-1996 (jenjo02)
**	    New mutexes. If called with LPB, lpb_mutex assumed to be held.
**	13-Nov-1997 (jenjo02)
**	    When resetting back status in LDB, also reset LDB_STALL_RW
**	    flag (new).
**	15-Mar-1999 (jenjo02)
**	    Removed LDB_STALL_RW flag.
**	26-Apr-2000 (thaju02)
**	    Turn off LDB_CKP_LKRQST. (B101303)
**      08-jun-2005 (horda03) Bug 114666/INGDBA322
**          Need to hold the lgd_ldb_q_mutex until the lgd_status flag
**          is cleared. If this mutex isn't held, then a DB could be
**          flaged LDB_BACKUP and switch on LGD_CKP_SBACKUP after the
**          ldb_status for the DB has been checked by this process. If
**          this was the only DB which is being backed up, then this
**          routine will clear the LGD_CKP_SBACKUP flag anf the other 
**          process will hang.
*/
VOID
LG_cleanup_checkpoint(register LPB *lpb)
{
    register LGD *lgd = (LGD *)LGK_base.lgk_lgd_ptr;
    register LDB *ldb;
    register LDB *bdb;
    register LDB *other_ldb;
    register LPD *lpd;
    register LPB *other_lpb;
    SIZE_TYPE	lpd_offset;
    SIZE_TYPE	lpb_offset;
    SIZE_TYPE	ldb_offset;
    i4		backup_found;

    /*
    ** If process was passed in, then check for cleanup work on the databases
    ** open by that process.
    */
    if (lpb != NULL)
    {
	/*
	** Find checkpoint database - assume there can only be one.
	*/
	bdb = (LDB *)NULL;

	for (lpd_offset = lpb->lpb_lpd_next;
	     lpd_offset != LGK_OFFSET_FROM_PTR(&lpb->lpb_lpd_next);
	     lpd_offset = lpd->lpd_next)
	{
	    lpd = (LPD *)LGK_PTR_FROM_OFFSET(lpd_offset);
	    if (lpd->lpd_type == LPD_TYPE)
	    {
		bdb = (LDB *)LGK_PTR_FROM_OFFSET(lpd->lpd_ldb);

		if (bdb->ldb_status & LDB_BACKUP)
		    break;
	    }
	}

	/*
	** If database was undergoing checkpoint - and there are no other
	** checkpoints running on this database - then turn off the checkpoint
	** status - also reset the system checkpoint status if appropriate.
	*/
	if (bdb && (bdb->ldb_status & LDB_BACKUP))
	{
	    if (LG_mutex(SEM_EXCL, &bdb->ldb_mutex))
		return;
	    /*
	    ** Check status again after semaphore wait.
	    */
	    if (bdb->ldb_status & LDB_BACKUP)
	    {
		/*
		** Check for any other checkpoint processes running on this
		** database.  Normally, we do not allow concurrent checkpoints of
		** the same database.  However, a second checkpoint process may
		** open the database before finding out that it cannot run (due to
		** an already running checkpoint).  In this case, we do not want to
		** clean up the checkpoint state.
		*/
		if (LG_mutex(SEM_EXCL, &lgd->lgd_lpb_q_mutex))
		    return;
		for (lpb_offset = lgd->lgd_lpb_next;
		     lpb_offset != LGK_OFFSET_FROM_PTR(&lgd->lgd_lpb_next);)
		{
		    other_lpb = (LPB *)LGK_PTR_FROM_OFFSET(lpb_offset);
		    lpb_offset = other_lpb->lpb_next;

		    /*
		    ** If find another checkpoint process, check if it is running
		    ** on this database.
		    */
		    if (other_lpb != lpb &&
			other_lpb->lpb_status & LPB_CKPDB)
		    {
			/* 
			** Release the lpb_q mutex before locking the LPB
			** to avoid deadlocking with code going the opposite
			** way (LPB->lpb_q), then reclaim it and recheck the LPB.
			*/
			(VOID)LG_unmutex(&lgd->lgd_lpb_q_mutex);
			if (LG_mutex(SEM_EXCL, &other_lpb->lpb_mutex))
			    return;
			if (LG_mutex(SEM_EXCL, &lgd->lgd_lpb_q_mutex))
			    return;
			/*
			** check LPB status after semaphore wait
			*/
			if (other_lpb->lpb_type == LPB_TYPE &&
			    other_lpb->lpb_status & LPB_CKPDB)
			{
			    for (lpd_offset = other_lpb->lpb_lpd_next;
				 lpd_offset !=
					LGK_OFFSET_FROM_PTR(&other_lpb->lpb_lpd_next);
				 lpd_offset = lpd->lpd_next)
			    {
				lpd = (LPD *)LGK_PTR_FROM_OFFSET(lpd_offset);
				if (lpd->lpd_type == LPD_TYPE)
				{
				    other_ldb = (LDB *)LGK_PTR_FROM_OFFSET(lpd->lpd_ldb);

				    /*
				    ** If this checkpoint process has the database open,
				    ** then there is no cleanup to perform - it will be
				    ** cleaned up when the second checkpoint exits.
				    */
				    if (other_ldb == bdb)
				    {
					(VOID)LG_unmutex(&other_lpb->lpb_mutex);
					(VOID)LG_unmutex(&lgd->lgd_lpb_q_mutex);
					(VOID)LG_unmutex(&bdb->ldb_mutex);
					return;
				    }
				}
			    }
			}
			else
			{
			    /*
			    ** Something about the LPB changed while we
			    ** waited on its mutex. Restart the lpb_q scan.
			    */
			    lpb_offset = lgd->lgd_lpb_next;
			}

			(VOID)LG_unmutex(&other_lpb->lpb_mutex);
		    }
		}
		(VOID)LG_unmutex(&lgd->lgd_lpb_q_mutex);

		/*
		** Turn off database backup status.
		** Do resume in case there are processes waiting for backup
		** state to clear.
		*/
		bdb->ldb_status &= 
			~(LDB_BACKUP | LDB_FBACKUP |
				    LDB_STALL | LDB_CKPDB_PEND | 
				    LDB_CKPLK_STALL | LDB_CKPERROR);
		LG_resume(&lgd->lgd_ckpdb_event, NULL);

	    }
	    (VOID)LG_unmutex(&bdb->ldb_mutex);
	}
    }

    /*
    ** Check if system backup status needs to be reset.
    ** Cycle through the list of open databases looking for any left in backup
    ** state.  Turn off the system backup status if there are none left.
    **
    */
    backup_found = FALSE;
              
    if (LG_mutex(SEM_EXCL, &lgd->lgd_ldb_q_mutex))
	return;

    for (ldb_offset = lgd->lgd_ldb_next;
	 ldb_offset != LGK_OFFSET_FROM_PTR(&lgd->lgd_ldb_next);
	 ldb_offset = ldb->ldb_next)
    {
	ldb = (LDB *)LGK_PTR_FROM_OFFSET(ldb_offset);

	if (ldb->ldb_status & LDB_BACKUP)
	{
	    backup_found = TRUE;
	    break;
	}
    }

    /* Need to hold the lgd->lgd_ldb_q_mutex to ensure
    ** another thread can't set LGD_CKP_SBACKUP before we clear it.
    */

    if (backup_found == FALSE)
    {
	if (LG_mutex(SEM_EXCL, &lgd->lgd_mutex))
        {
            LG_unmutex(&lgd->lgd_ldb_q_mutex);

	    return;
        }

	lgd->lgd_status &= ~(LGD_CKP_SBACKUP);
	(VOID)LG_unmutex(&lgd->lgd_mutex);
    }

    LG_unmutex(&lgd->lgd_ldb_q_mutex);

    return;
}

/*
** Name: LG_force_abort		    Called by the force abort thread
**
** Description:
**	This routine is called by the force abort thread to signal the
**	correct session(s) to abort its transaction.
**
** Inputs:
**	None
**
** Outputs:
**	None
**
** Returns:
**	STATUS
**
** History:
**	30-oct-1992 (bryanp)
**	    The new logging and locking system.
**	23-aug-1993 (bryanp)
**	    The force-abort thread also needs to handle the manual abort of a
**	    prepared transaction. Added code to check for LXB_MAN_ABORT xacts.
**	08-sep-93 (swm)
**	    Changed sid type from i4  to CS_SID to match recent CL
**	    interface revision.
**	24-Jan-1996 (jenjo02)
**	    Mutex granularity schemes no longer solely dependent on
**	    lgd_mutex.
**	15-Jun-1999 (jenjo02)
**	    Mutex LXB, retest it. The txn may have already terminated by
**	    the time we get the mutex...
**	15-Dec-1999 (jenjo02)
**	    Replaced lgd_force_id with a queue of LXBs to be aborted.
**	    Abort all which appear on the list and belong to this process.
**	28-Jul-2006 (jonj)
**	    Fix broken LXB_ORPHAN | LXB_SHARED_HANDLE test.
*/
STATUS
LG_force_abort(CL_ERR_DESC *sys_err)
{
    register LGD	*lgd = (LGD *)LGK_base.lgk_lgd_ptr;
    register LXB	*lxb;
    register LPD	*lpd;
    register LPB	*lpb;
    LXBQ		*lxbq, *next_lxbq, *prev_lxbq;
    i4			faborts = 0;
    i4			maborts = 0;
    bool		RecoverOrphanXacts = FALSE;

    CL_CLEAR_ERR(sys_err);

    /*
    ** Run down the chain of to-be-aborted transactions and find all transactions
    ** which are appropriately marked as needing force-abort or manual abort
    ** processing by this process.
    **
    ** For each found, remove it from the queue and call the force abort handler.
    */

    (VOID)LG_mutex(SEM_EXCL, &lgd->lgd_mutex);

    lxbq = (LXBQ *)LGK_PTR_FROM_OFFSET(lgd->lgd_lxb_abort.lxbq_next);

    while ( lxbq != (LXBQ *)&lgd->lgd_lxb_abort )
    {
	lxb = (LXB *)((char *)lxbq - CL_OFFSETOF(LXB, lxb_lgd_abort));
	lpd = (LPD *)LGK_PTR_FROM_OFFSET(lxb->lxb_lpd);
	lpb = (LPB *)LGK_PTR_FROM_OFFSET(lpd->lpd_lpb);

	lxbq = (LXBQ *)LGK_PTR_FROM_OFFSET(lxb->lxb_lgd_abort.lxbq_next);

	if ( lxb->lxb_status & LXB_FORCE_ABORT )
	    faborts++;
	else if ( lxb->lxb_status & LXB_MAN_ABORT )
	    maborts++;

	/* Find next LXB belonging to this process which is to be aborted */
	if ( lpb->lpb_pid == LG_my_pid )
	{
	    /* Remove LXB from abort list */
	    next_lxbq = (LXBQ*)LGK_PTR_FROM_OFFSET(lxb->lxb_lgd_abort.lxbq_next);
	    prev_lxbq = (LXBQ*)LGK_PTR_FROM_OFFSET(lxb->lxb_lgd_abort.lxbq_prev);
	    next_lxbq->lxbq_prev = lxb->lxb_lgd_abort.lxbq_prev;
	    prev_lxbq->lxbq_next = lxb->lxb_lgd_abort.lxbq_next;
	    lgd->lgd_lxb_aborts--;

	    /* Release the queue's mutex */
	    (VOID)LG_unmutex(&lgd->lgd_mutex);

	    /* Mutex the LXB */
	    (VOID)LG_mutex(SEM_EXCL, &lxb->lxb_mutex);

	    /* Check again after waiting for the LXB mutex */
	    if ( lxb->lxb_status & (LXB_FORCE_ABORT | LXB_MAN_ABORT) &&
		 lpd == (LPD *)LGK_PTR_FROM_OFFSET(lxb->lxb_lpd) &&
		 lpb->lpb_force_abort )
	    {
		/*
		** Look for disassociated XA transaction log handles.
		**
		** While disassociated, they're "owned" by the RCP 
		** rather than by any server session, in an idle state, and
		** can't be force-aborted in the usual way.
		**
		** Rather we simply remove their association with
		** the global XA transaction by removing their
		** Log/Locking contexts.
		**
		** If the global transaction (SHARED) has just one of
		** these handles remaining, we mark the SHARED transaction
		** for recovery and the the RCP take over.
		** This last handle and its SHARED LXB will be deallocated
		** when recovery completes (see LGend()).
		*/
		if ( lxb->lxb_status & LXB_ORPHAN &&
		     lxb->lxb_status & LXB_SHARED_HANDLE )
		{
		    CL_ERR_DESC	sys_err;
		    i4		err_code;
		    STATUS	stat;
		    LG_I4ID_TO_ID log_id;
		    i4		lock_id = lxb->lxb_lock_id;
		    LXB		*slxb;

		    log_id.id_lgid = lxb->lxb_id;
		    /* Look up the SHARED LXB */
		    slxb = (LXB*)LGK_PTR_FROM_OFFSET(lxb->lxb_shared_lxb);

		    (VOID)LG_mutex(SEM_EXCL, &slxb->lxb_mutex);

		    /* Is this the last handle connection? */
		    if ( slxb->lxb_handle_count > 1 )
		    {
			(VOID)LG_unmutex(&slxb->lxb_mutex);
			(VOID)LG_unmutex(&lxb->lxb_mutex);

			/* Toss the log handle, reducing slxb's handle_count */
			stat = LGend(log_id.id_i4id, 0, &sys_err);
			if ( stat != OK )
			{
			    uleFormat(NULL, stat, &sys_err, ULE_LOG, NULL,
				(char *)NULL, 0L, (i4 *)NULL, &err_code, 0);
			    uleFormat(NULL, E_DM900E_BAD_LOG_END, &sys_err, ULE_LOG, NULL,
				(char *)NULL, 0L, (i4 *)NULL, &err_code, 1,
				0, log_id.id_i4id);
			}

			/* ...and its lock list handle, if any */
			if ( lock_id )
			{
			    stat = LKrelease(LK_ALL, lock_id, (LK_LKID *)NULL, 
							(LK_LOCK_KEY *)NULL, (LK_VALUE *)NULL, &sys_err);
			    if ( stat != OK )
			    {
				uleFormat(NULL, stat, &sys_err, ULE_LOG, NULL,
				    (char *)NULL, 0L, (i4 *)NULL, &err_code, 0);
				uleFormat(NULL, E_DM901B_BAD_LOCK_RELEASE, &sys_err, ULE_LOG, NULL,
				    (char *)NULL, 0L, (i4 *)NULL, &err_code, 1, 0, lock_id);
			    }
			}
		    }
		    else
		    {
			/*
			** Last handle, mark SHARED txn for recovery, 
			** un-ABORT the handle as we don't want recovery
			** to see it and get confused.
			*/
			lxb->lxb_status &= ~(LXB_FORCE_ABORT | LXB_MAN_ABORT);
			slxb->lxb_status |= LXB_RECOVER;
			RecoverOrphanXacts = TRUE;

			(VOID)LG_unmutex(&slxb->lxb_mutex);
			(VOID)LG_unmutex(&lxb->lxb_mutex);
		    }
		    lxb = (LXB*)NULL;
		}
		else
		    (*lpb->lpb_force_abort)
			(CSfind_scb(lxb->lxb_cpid.sid));
	    }
	    
	    /* Release the LXB, restart the queue */
	    if ( lxb )
		(VOID)LG_unmutex(&lxb->lxb_mutex);
	    (VOID)LG_mutex(SEM_EXCL, &lgd->lgd_mutex);
	    lxbq = (LXBQ *)LGK_PTR_FROM_OFFSET(lgd->lgd_lxb_abort.lxbq_next);
	    faborts = 0;
	    maborts = 0;
	}
    }

    /* If no txns in FORCE ABORT or MAN ABORT remain, reset LGD status */
    if ( faborts == 0 )
	lgd->lgd_status &= ~LGD_FORCE_ABORT;
    if ( maborts == 0 )
	lgd->lgd_status &= ~LGD_MAN_ABORT;

    if ( RecoverOrphanXacts )
	LG_signal_event(LGD_RECOVER, 0, TRUE);

    (VOID)LG_unmutex(&lgd->lgd_mutex);
    
    return (OK);
}

/*
** Name: LG_used_logspace		- find amount of used logspace
**
** DescriptionL:
**      This routine is used to fine the amount of currently used logfile 
**	space, including space reserved for the recovery system (if indicated).
**
** Inputs:
**	LFB			Logging system context
**	include_reserved	Indicates whether to include reserved log
**				file space as well as written space.
**
** Outputs:
**      The amount of used logspace in blocks.
**
** History:
**      03-jan-1993 (jnash)
**          Created as part of Reduced logging project.
**	26-apr-1993 (bryanp)
**	    Cluster support: logfile information is now in the LFB structure.
**	18-oct-1993 (rogerk)
**	    Tried to change calculations to not overflow on MAXI4 sized
**	    log files.  Moved checks for negative lfb_reserved space to
**	    to lgwrite/lgreserve routines.
**	    Added argument to LG_used_logspace to indicate whether or not
**	    to include reserved logspace.
**	24-Jan-1996 (jenjo02)
**	    Mutex granularity project. 
**	    This is done using a dirty read of lfb_header, so the 
**	    result may be an approximation.
*/
i4
LG_used_logspace(
register LFB	*lfb, 
bool	include_reserved)
{
    i4	blocks_used;

    /* Check for log file wrap-around */
    if (lfb->lfb_header.lgh_end.la_sequence > 
				    lfb->lfb_header.lgh_begin.la_sequence)
    {
	blocks_used = 
	    lfb->lfb_header.lgh_count + 1 -
	    (lfb->lfb_header.lgh_begin.la_block - 
			      lfb->lfb_header.lgh_end.la_block);
    }
    else
    {
	blocks_used = lfb->lfb_header.lgh_end.la_block - 
		 lfb->lfb_header.lgh_begin.la_block + 1;
    }

    if (include_reserved)
	blocks_used += (i4)(lfb->lfb_reserved_space / lfb->lfb_header.lgh_size) + 1;

    return(blocks_used); 
}

/*
** Name: LG_orphan_xact	- Disown transaction and mark it orphaned.
**
** Description:
**	Disown transaction and mark it orphaned.  The transaction will
**	remain in the system in an orphaned state until it can be adopted
**	by a new owner.
**
**	That new owner (either a dbms thread or the rcp) will adopt the
**	transaction via an LG_A_OWNER alter call.
**
**	While in an orphaned state, the transaction is linked to its own
**	orphan LPD and to the RCP's LPB.
**
** Inputs:
**	lxb	- transaction context
**
** Outputs:
**	none
**
** History:
**      18-oct-1993 (rogerk)
**          Created as part of 2PC fixes.
**	24-Jan-1996 (jenjo02)
**	    LXB's mutex must be held on entry to function.
**	22-Oct-1996 (jenjo02)
**	    Don't clear lxb_pid, lxb_sid if LXB is in a wait state.
*/
STATUS
LG_orphan_xact(register LXB *lxb)
{ 
    register LGD        *lgd = (LGD *)LGK_base.lgk_lgd_ptr;
    register LPB	*master_lpb;
    register LPD	*lpd;
    register LDB	*cur_ldb;
    register LPD	*cur_lpd;
    LPD			*next_lpd;
    LXBQ		*next_lxbq;
    LXBQ		*prev_lxbq;
    STATUS		status;


    /* Find RCP process control block. */
    master_lpb = (LPB *)LGK_PTR_FROM_OFFSET(lgd->lgd_master_lpb);

    cur_lpd = (LPD *)LGK_PTR_FROM_OFFSET(lxb->lxb_lpd);
    cur_ldb = (LDB *)LGK_PTR_FROM_OFFSET(cur_lpd->lpd_ldb);

    /*
    ** Allocate and initialize an orphan LPD structure.
    ** Returns with lpd_type set.
    */
    if ((lpd = (LPD *)LG_allocate_cb(LPD_TYPE)) == 0) 
	return (LG_EXCEED_LIMIT);

    lpd->lpd_stat.write = 0;
    lpd->lpd_stat.force = 0;
    lpd->lpd_stat.wait = 0;
    lpd->lpd_stat.begin = 0;
    lpd->lpd_stat.end = 0;

    /*
    ** Associate the new lpd with the current transaction's database.
    */
    if (status = LG_mutex(SEM_EXCL, &cur_ldb->ldb_mutex))
	return(status);
    lpd->lpd_ldb = cur_lpd->lpd_ldb;
    cur_ldb->ldb_lpd_count++;

    /*
    ** Unlink the transaction from its current LPD.
    */
    next_lxbq = (LXBQ *)LGK_PTR_FROM_OFFSET(lxb->lxb_owner.lxbq_next);
    prev_lxbq = (LXBQ *)LGK_PTR_FROM_OFFSET(lxb->lxb_owner.lxbq_prev);
    next_lxbq->lxbq_prev = lxb->lxb_owner.lxbq_prev;
    prev_lxbq->lxbq_next = lxb->lxb_owner.lxbq_next;
    cur_lpd->lpd_lxb_count--;

    /*
    ** Link the orphaned transaction onto the new LPD.
    */
    lxb->lxb_lpd = LGK_OFFSET_FROM_PTR(lpd);
    lxb->lxb_owner.lxbq_next = LGK_OFFSET_FROM_PTR(&lpd->lpd_lxbq.lxbq_next);
    lxb->lxb_owner.lxbq_prev = LGK_OFFSET_FROM_PTR(&lpd->lpd_lxbq.lxbq_next);
    lpd->lpd_lxbq.lxbq_next = LGK_OFFSET_FROM_PTR(&lxb->lxb_owner);
    lpd->lpd_lxbq.lxbq_prev = LGK_OFFSET_FROM_PTR(&lxb->lxb_owner);
    lpd->lpd_lxb_count = 1;

    if ((lxb->lxb_status & LXB_WAIT) == 0)
    {
	lxb->lxb_cpid.sid = 0;
	lxb->lxb_cpid.pid = 0;
    }
    lxb->lxb_status |= LXB_ORPHAN;
    (VOID)LG_unmutex(&cur_ldb->ldb_mutex);

    /*
    ** Link the new LPD to the recovery process LPB.
    */
    if (status = LG_mutex(SEM_EXCL, &master_lpb->lpb_mutex))
	return(status);
    lpd->lpd_lpb = lgd->lgd_master_lpb;
    lpd->lpd_next = master_lpb->lpb_lpd_next;
    lpd->lpd_prev = LGK_OFFSET_FROM_PTR(&master_lpb->lpb_lpd_next);

    next_lpd = (LPD *)LGK_PTR_FROM_OFFSET(master_lpb->lpb_lpd_next);
    next_lpd->lpd_prev = LGK_OFFSET_FROM_PTR(lpd);
    master_lpb->lpb_lpd_next = LGK_OFFSET_FROM_PTR(lpd);
    master_lpb->lpb_lpd_count++;

    (VOID)LG_unmutex(&master_lpb->lpb_mutex);

    return (OK);
}

/*
** Name: LG_adopt_xact	- Assume ownership of an orphaned transaction.
**
** Description:
**	Adopt an orphaned transaction that had previously be disassociated
**	from its DBMS session client.  Queue the transaction to the caller's
**	lpb and lpd.
**
**	The old orphan LPD is deallocated by the adoption.
**
** Inputs:
**	lxb	- transaction context
**	lpd	- adopter's database context
**
** Outputs:
**	none
**
** History:
**      18-oct-1993 (rogerk)
**          Created as part of 2PC fixes.
**	24-Jan-1996 (jenjo02)
**	    LXB mutex must be held on entry to function.
*/
STATUS
LG_adopt_xact(
register LXB	*lxb,
register LPD	*lpd)
{ 
    register LGD        *lgd = (LGD *)LGK_base.lgk_lgd_ptr;
    register LPB	*lpb;
    register LPB	*cur_lpb;
    register LDB	*cur_ldb;
    register LPD	*orphan_lpd;
    LPD			*next_lpd;
    LPD			*prev_lpd;
    LXBQ		*next_lxbq;
    LXBQ		*prev_lxbq;
    STATUS		status;

    orphan_lpd = (LPD *)LGK_PTR_FROM_OFFSET(lxb->lxb_lpd);
    cur_lpb = (LPB *)LGK_PTR_FROM_OFFSET(orphan_lpd->lpd_lpb);
    cur_ldb = (LDB *)LGK_PTR_FROM_OFFSET(orphan_lpd->lpd_ldb);
    lpb = (LPB *)LGK_PTR_FROM_OFFSET(lpd->lpd_lpb);

    /*
    ** Unlink the orphan lpd from the recovery process LPB.
    */
    if (status = LG_mutex(SEM_EXCL, &cur_lpb->lpb_mutex))
	return(status);
    next_lpd = (LPD *)LGK_PTR_FROM_OFFSET(orphan_lpd->lpd_next);
    prev_lpd  = (LPD *)LGK_PTR_FROM_OFFSET(orphan_lpd->lpd_prev);
    next_lpd->lpd_prev = orphan_lpd->lpd_prev;
    prev_lpd->lpd_next = orphan_lpd->lpd_next;
    cur_lpb->lpb_lpd_count--;
    (VOID)LG_unmutex(&cur_lpb->lpb_mutex);

    if (status = LG_mutex(SEM_EXCL, &cur_ldb->ldb_mutex))
	return(status);
    /*
    ** Unlink the transaction from its orphan lpd.
    */
    next_lxbq = (LXBQ *)LGK_PTR_FROM_OFFSET(lxb->lxb_owner.lxbq_next);
    prev_lxbq = (LXBQ *)LGK_PTR_FROM_OFFSET(lxb->lxb_owner.lxbq_prev);
    next_lxbq->lxbq_prev = lxb->lxb_owner.lxbq_prev;
    prev_lxbq->lxbq_next = lxb->lxb_owner.lxbq_next;
    cur_ldb->ldb_lpd_count--;

    /*
    ** Restore the id of the caller's session.
    */
    CSget_cpid(&lxb->lxb_cpid);
    lxb->lxb_lpd = LGK_OFFSET_FROM_PTR(lpd);
    lxb->lxb_status &= ~LXB_ORPHAN;

    /*
    ** Associate the LXB with the caller's process and database.
    */
    lxb->lxb_owner.lxbq_next = lpd->lpd_lxbq.lxbq_next;
    lxb->lxb_owner.lxbq_prev = LGK_OFFSET_FROM_PTR(&lpd->lpd_lxbq.lxbq_next);

    next_lxbq = (LXBQ *)LGK_PTR_FROM_OFFSET(lpd->lpd_lxbq.lxbq_next);
    next_lxbq->lxbq_prev = LGK_OFFSET_FROM_PTR(&lxb->lxb_owner);
    lpd->lpd_lxbq.lxbq_next = LGK_OFFSET_FROM_PTR(&lxb->lxb_owner);
    lpd->lpd_lxb_count++;
    (VOID)LG_unmutex(&cur_ldb->ldb_mutex);

    /*
    ** Toss the orphan LPD.
    */
    LG_deallocate_cb(LPD_TYPE, (PTR)orphan_lpd);

    return (OK);
}
