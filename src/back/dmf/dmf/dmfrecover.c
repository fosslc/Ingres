/*
** Copyright (c) 1987, 2005 Ingres Corporation
**
**
*/

#include    <compat.h>
#include    <gl.h>
#include    <tm.h>
#include    <me.h>
#include    <cs.h>
#include    <cx.h>
#include    <di.h>
#include    <pc.h>
#include    <tr.h>
#include    <er.h>
#include    <ex.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <lk.h>
#include    <lg.h>
#include    <st.h>
#include    <ulf.h>
#include    <scf.h>
#include    <adf.h>
#include    <dmf.h>
#include    <dm.h>
#include    <dmccb.h>
#include    <dmrcb.h>
#include    <dmtcb.h>
#include    <dmxcb.h>
#include    <dml.h>
#include    <dmpp.h>
#include    <dmp.h>
#include    <dm1b.h>
#include    <dmve.h>
#include    <dm0p.h>
#include    <dm0m.h>
#include    <dm0llctx.h>
#include    <dm0l.h>
#include    <dmfrcp.h>
#include    <dmfacp.h>
#include    <dm2d.h>
#include    <dm0c.h>
#include    <dm2t.h>
#include    <dmftrace.h>
#include    <dmd.h>
#include    <dm2f.h>
#include    <dm1c.h>
#include    <dm2r.h>

/**
**
**  Name: DMFRECOVER.C - DMF Recovery Routines.
**
**  Description:
**
**	This file contains routines common to the RCP and the CSP.
**
**	dmr_recover	- Perform the recovery work of the machine failure
**			  recovery.
**	recover_willing_commit - Recovered the willing commit transaction.
**
**  History:    
**	01-jul-1987 (Derek)
**	    Created from DMFRECOVER.C.
**	Summer-1988 (EDHSU)
**	    Various changes for fast commit project.
**	19-sep-88 (rogerk)
**	    Changed many routine names to be unique to 7 characters:
**	    dmr_get_redo_list, dmr_get_tabid, dmr_mini_p0, dmr_mini_p1,
**	    dmr_rbi_insert, dmr_rbi_lookup, dmr_rdmu_insert, dmr_rdmu_lookup,
**	    dmr_redo_finale, dmr_redo_init, dmr_redo_lookup, dmr_redo_process.
**	31-oct-1988 (rogerk)
**	    Print out free_wait and stall_wait statistics in dmr_log_statistics.
**	13-dec-1988 (rogerk)
**	    Added rtx_recover_type field to the RTX struct.  Whenever an rtx
**	    is allocated, its type is set to RTX_UNDO.  If it is moved off of
**	    the abort list and onto the redo list, its type is changed to
**	    RTX_REDO.
**	18-Jan-1989 (ac)
**	    Added 2PC support.
**       6-sep-1989 (rogerk)
**          Call dm2d_del_db when fail in open_db.  That way we don't save
**          the bad context around for the next time the database is accessed.
**          So if the database is patched or destroyed/recreated we don't
**          mark it inconsistent the next time we see it in recovery.
**	11-sep-1989 (rogerk)
**	    Handle recover of DM0L_DMU log records during rollback.
**	    Added lx_id parameter to dm0l_opendb/dm0l_closedb calls.
**	 2-oct-1989 (rogerk)
**	    Fix fast commit bug in dmr_p1 code when processing ET records.
**	    Check for RDB_FAST_COMMIT in rdb_status, not DB_FAST_COMMIT.
**	 2-oct-1989 (rogerk)
**	    Add handling of DM0L_FSWAP log records during rollback.
**	 7-dec-1989 (annie)
**	    BUG 8981: always set tx in dmr_p2
**		Problem showed up as ACP refusing to start up when it found
**		a mismatched ET record.
**	26-dec-89 (greg)
**	    Pass high and low tran_id separately to TRdisplay (for UNIX).
**	10-jan-1990 (rogerk)
**	    When opening database, pass DM2D_OFFLINE_CSP flag if database
**	    is being opened by the csp during startup recovery.  This way
**	    we can tell if the database is open by the CSP as either the
**	    DM2D_OFFLINE_CSP or DM2D_CSP flags will be set.
**	12-feb-1990 (rogerk)
**	    Added parameter to dmr_db_known to specify whether to return
**	    rdb even if instance id does not match.  This is used to check
**	    if specified database id is already in use by a different database.
**	25-apr-90 (bryanp)
**	    Enhance Roger's 12-Feb check to handle the situation where a
**	    database has been opened, closed, then opened again in the same
**	    CP and the second open has re-used the internal DB id of a
**	    different database. In this case we must infer 2 close operations:
**	    (1) an implied close of this database (the one being opened), and
**	    (2) an implied close of the other database (the one whose ID has
**	    now been re-used).
**      24-aug-1990 (bryanp)
**          Notify LG of the logfile BOF at the end of dmr_p1(), rather than
**          waiting for the end of machine failure recovery. This allows LG to
**          keep more accurate journal windows for journalled databases which
**          were open at the time of the crash.
**      25-feb-1991 (rogerk)
**          Add handling of DM0L_TEST log records during rollback.
**          These log records are ignored in UNDO processing.
**          Added for Archiver Stability project.
**      7-mar-1991 (bryanp)
**          Fix a database inconsistent bug in dmr_udprocess().
**      25-mar-1991 (rogerk)
**          Added support to recognize databases which were undergoing
*           Set Nologging work.  These databases are marked inconsistent.
**      16-jul-1991 (bryanp)
**          B38527: Added support for DM0L_OLD_MODIFY & DM0L_OLD_INDEX.
**          Also, pass DM0L_SM0_CLOSEPURGE to dmve_sm0_closepurge, even though
**          this record is not currently used by UNDO processing.
**	22-aug-91 (jnash) merged 24-sep-1990 (rogerk)
**	    Merged 6.3 changes into 6.5.
**	22-aug-91 (jnash) 14-jun-1991 (Derek)
**	    Updated types of log records that can be expected with the new
**	    allocation routines (eliminate DM0LBALLOC, DMOLBPEXTEND, 
**	    DM0LBDEALLOC, DM0LCALLOC log record types, add DM0LEXTEND, 
**	    DM0LALLOC, DM0LDEALLOC, DM0LASSOC).
**      28-oct-91 (jnash)
**          In recover_willing_commit, add "ignored" param to dm0l_opendb 
**	    (caught by LINT).
**	 3-nov-1991 (rogerk)
**	    Added DM0LOVFL log record for REDO of File Extend operations.
**	    During redo processing, call dm0l_ovfl when this log record 
**	    is encountered.
**      18-nov-1991 (bryanp)
**          Pass the tr_dis_id components separately to TRdisplay.
**	20-Nov-1991 (rmuth)
**	    Integrated rogerk's to take out writting of before images in
**	    prepare redo. See comments in code for more explanation.
**	16-jan-1992 (bryanp)
**	    Trace to II_RCP.LOG information about DB's undergoing REDO.
**	10-feb-1992 (jnash) 
**	    Send additional diagnostic messages to RCP log when database
**	    marked inconsistent.
**	11-feb-1992 (bryanp)
**	    Yet more debugging and diagnostic messages.
**	14-may-1992 (bryanp)
**	    B38112: Write DM0L_DBINCONST ET if DB is already inconsistent.
**	18-jun-1992 (jnash)
**	    If II_DMFRCP_STOP_ON_INCONS_DB set, crash rather than
**	    marking a database inconsistent.
**	7-july-1992 (jnash)
**	    Add DMF function prototypes.
**	22-jul-1992 (bryanp)
**	    Renamed dmr_log_statistics, dmr_log_error and dmr_error_translate 
**	    to dmr_* names so that I can link this module into the server.
**	29-sep-1992 (bryanp)
**	    Moved dump_transaction call to dmr_abort.
**	29-sep-1992 (nandak)
**	    Use one common transaction ID type.
**	30-sep-1992 (jnash)
**	    B45016. Fix error messages that flagged dmr_process_redo routine
**	    as ABORT processing rather than REDO.
**      6-oct-1992 (nandak)
**          Fix printing of XA transaction id.
**	8-nov-92 (ed)
**	    remove DB_MAXNAME dependency
**	12-oct-1992 (bryanp)
**	    Fix TRdisplay of tr_status field.
**	26-oct-1992 (rogerk)
**	    Reduced Logging Project:
**	      - Added changes associated with new log records.
**	14-dec-1992 (rogerk)
**	    Reduced Logging Project (phase 2): Completely rewritten.
**	05-feb-1993 (rogerk & jnash)
**	    Fix free list trashing problem, remove code which tried to take
** 	    rdmu structure off of the "hash queue" - which did not exist.    
**	10-feb-1993 (jnash)
**	    Add new verbose flag to dmd_log() - pass FALSE here.
**	15-mar-1993 (rogerk)
**	    Reduced Logging - Phase IV:
**	    Set new dmve_logging flag to signal logging during UNDO and REDO.
**	    Renamed prev_lsn to compensated_lsn.
**	    Changed Abortsave and EndMini log records to be written as CLR's.
**	        The compensated_lsn field is now used to skip backwards in
**		in the log file rather than using the log addresses in the
**		log records themselves.
**	    Clarified use of LSN vs. Log Adress.  Log Reads return Log
**		Addresses.  The LSN of a record can be extracted from the
**		log record header.
**	    Changed the log record format to include the database and
**		transaction ids rather than having them in the lctx_header.
**	    Changed the RCP to track databases via the external database id
**		rather than an internal LG id.  RDB list is now hashed by
**		the external database id.  LGshow calls for transaction and
**		database information were changed to return the external
**		database id in the LG_DATABASE and LG_TRANSACTION info blocks.
**	    Removed dm0l_opendb/dm0l_closedb calls to use LGadd/LGremove.
**		We no longer add the database using the same internal LG DBID
**		that it was previously open with;  a normal LGadd is done with
**		no ADDONLY flag.  We alter the lg db info to show any required
**		journal work - this used to be done automatically by LGadd
**		when the old LG_ADDONLY flag was passed.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**		Pass correct transaction handle to dm0l_read.
**		Enable DM1307 tracepoint to echo LSN's during recovery passes.
**		The unique key for an RDB is now db_id + node_id.
**		Remove all uses of the global rcp_node_id, since in a multi-
**		    node logging system the dmfrecover.c code is simultaneously
**		    managing the recovery of multiple log files. To get the
**		    correct node id for a particular operation, we now track
**		    the node ID in the LCTX and RDB structures, and use one of
**		    those as appropriate.
**		Format the db_sback_lsn field in the LG_DATABASE.
**		Adjusted some rcp_action tests to accomodate cluster needes.
**		Add dbs with the DM2D_CANEXIST flag set.
**		Removed the DM2D_CSP flag since the DMF Physical Layer code
**		    no longer needs to know the difference between an RCP open
**		    and a CSP open.
**	24-may-1993 (bryanp)
**	    Include <pc.h> before <lk.h> to pick up PID definition.
**	    Format ALL the database status bits.
**	21-jun-1993 (bryanp)
**	    Properly trace log addresses in verbose UNDO log record tracing.
**	21-jun-1993 (jnash)
**	    Inconsistent database changes.  
**	    	If a database is marked either closed or inconsistent 
**		    prior to the start of the recovery, deallocate rtx's
**		    found during the analysis pass.
**		If a database goes inconsistent during a recovery, retain
**	   	    ownership of the db's transactions.  This is required
**		    to properly clean up at the end.
**	21-jun-1993 (rogerk)
**	    Fixed dmr_complete_recovery call to pass num_nodes instead of
**	    node_id.  Passing node_id caused us to not logdump when database
**	    inconsistencies are found.
**	    Check for boundary conditions where a transaction is ended just
**	    previous to a Consistency Point and ends up listed in the BCP
**	    even though an ET record has been written.  This causes us to
**	    add the transaction to our unresolved list and since we never
**	    encounter the ET (its before the CP) we think we need to abort it.
**	    Normally, such a transaction is found to be committed when we
**	    start trying to abort it.  But if the DB is found to be closed,
**	    we get confused because we think there are still transactions to
**	    resolve.  Also if the BOF gets moved forward past the ET record
**	    then we can't go back and read the ET - we just have to assume
**	    that it has been committed.
**	26-jul-1993 (bryanp)
**	    When creating the rcp_lock_list, if this is the CSP, pass
**		LK_MASTER_CSP to LKcreate_list().
**	    Re-enable logging of ET log records while we figure out how to
**		handle inconsistent databases properly.
**	    We no longer log closedb log records, so don't look for them.
**	    Fix formatting problems in format_dis_tran_id for XA tran types.
**	    If we thought a transaction was in willing commit state, but the
**		first undo log record we actually process for this transaction
**		(following whatever CLR jumping we do) is a BT or ET, then
**		the transaction is actually NOT in willing commit state, so
**		change the transaction type to REDO_UNDO to reflect this.
**	26-jul-1993 (jnash)
**	    Write periodic status messages during undo and redo.  Also fix
**	    a few compiler warnings.
**	26-jul-1993 (rogerk)
**	  - Changed journal and dump window tracking in the logging system.
**	    Dumped old lfb journal and dump windows and added new archive
**	    window information.
**	  - Took out references and declarations of unused 'lga' variables.
**	  - Put back logging of ET records for transactions processed in undo
**	    recovery.  This was accidentally commented out in the last
**	    integration.
**	  - Add optimization to skip redo pass when there are no databases
**	    or transactions to recover.
**	  - Added rtx_orig_last_lga, used to store the log address of the
**	    last log record written by the transaction before recovery started.
**	    This is used to dump the transaction records in error processing.
**	  - Created routine 'trace_record' to consolidate calls which output
**	    log records and their header info to the trace file.  Added tracing
**	    of records which are skipped in REDO processing due to NonRedo
**	    operations.
**	  - Changed some RCP trace outputs to try to make it more readable.
**	  - Added trace point DM1312 which bypasses logdumps that occur when
**	    recovery processing fails.
**	23-aug-1993 (bryanp)
**	    2PC fixes:
**		Handle online recovery manual abort of prepared transaction.
**		Don't need to reacquire locks in online recovery of prepared
**		    transaction, just change ownership to RCP.
**	    Add wait reason LXB_LOGSPACE_WAIT.
**	23-aug-1993 (jnash)
**	  - During offline recovery, open databases as fast-commit, single 
**	    cache, "not RCP" (DM2D_BMSINGLE, DM2D_FASTCOMMIT, ~DM2D_ONLINE_RCP) 
**	    so that the buffer manager does not do unnecessary forces.
**	  - Change E_DM943D_RCP_DBREDO_ERROR and E_DM943E_RCP_DBUNDO_ERROR 
**	    error to pass lsn's rather than log addresses.
**	23-aug-1993 (rogerk)
**	  - Took out direct updating of the database config file to mark
**	    recovery processing complete when the database is journaled.
**	    We cannot turn off the config file open mask until ALL work
**	    is finished, including archiving.
**	  - Added implied setting of DM1315 to get page LSN tracing when
**	    verbose recovery mode is specified.
**	24-aug-1993 (wolf)
**	    Add #include st.h to resolve link errors
**	20-sep-1993 (rogerk)
**	    Initialize dmve_rolldb_action list when dmve cb is initialized.
**      23-sep-1993 (iyer)
**          The structure of DB_DIS_TRAN_ID has changed. It now has  
**          DB_XA_EXTD_DIS_TRAN_ID as the structure for a XA distributed
**          transaction ID. This is an extended version of the original
**          XA Id and contains two additional members: branch_seqnum and
**          branch_flag. This change effects the dmr_format_dis_tran_id ()
**          function which formats and prints the distributed XA ID.
**	14-oct-93 (jrb)
**          Added support for DM0LEXTALTER.
**	18-oct-1993 (jnash)
**	    Write iircp.log 'percent done' info at 30 second intervals.  This 
**	    eliminates flooding the error log with messages when recovering 
**	    large log intervals.  Also fix output with large log files.
**	18-oct-1993 (jnash & rogerk)
**	    Working on Willing Commit recovery bugs:
**	    Replaced dm0l calls in recover_willing_commit routine with 
**	    read_next_log_record.  Changed routine to assume that the first log
**	    record to process is already resident in the tx structure.
**	    Changed offline recovery to not actually request a database lock
**	    because this prevented willing commit transaction recovery from
**	    reacquiring their database locks.
**	    Changed handling of Willing Commit transactions to set them into
**	    reassoc_wait state at the end of recovery.
**	18-oct-1993 (rogerk)
**	    Added display of orphan transaction status and transaction reserved
**	    logfile space to the dmr_format_transaction routine.
**	    Took out the use of the LG_END flag to signal the cleanup of
**	    control blocks left around by failed processes in dmr_complete.
**	    Added ownername to rtx structure and add_rtx call so that xacts
**	    can be restored with their original owner.
**	22-nov-1993 (jnash)
**	    B56865.  Fix periodic display of RCP progress where negative
**	    percentages printed if bof log file seq <> eof log file seq.  
**	    Also, shorten SIprintf() output and fix comments.
**	31-jan-1994 (rogerk)
**	    Added dmr_preinit_recovery routine to release locks that prevent
**	    the opendb calls from succeeding (config and core catalog locks).
**	    This routine now also does the adopting of transaction lock lists
**	    to the RCP.
**	31-jan-1994 (rogerk)
**	    Fixed handling of transactions which have written an ET record
**	    but which have not been removed from the Logging System.
**	    During the analysis pass, if an ET record is found for an xact
**	    that was part of our online recovery context then we cannot remove
**	    it from the open transaction list.  Doing this causes us to not
**	    know to call LGend for the xact in dmr_complete_recovery.  The
**	    rtx must be left around, but marked TRAN_COMPLETED.
**	11-oct-93 (johnst)
**	    Bug #56449
**	    Changed TRdisplay format args from %x to %p where necessary to
**	    display pointer types. We need to distinguish this on, eg 64-bit
**	    platforms, where sizeof(PTR) > sizeof(int).
**	31-jan-1994 (jnash)
**	    Display LSN's as (%d,%d).
**	15-apr-1994 (chiku)
**	    Bug56702: be able to recover after online recovery encountered dead 
**		      logfull:
**
**		      1) LG_check_logfull() was changed to return dead logfull
**			 indication.
**		      2) LG_allocate_lbb(), LG_write(), LGwrite() and all 
**			 dvme_xxxx() routines are changed to return dead 
**			 logfull indication.
**		      3) For online recovery, dmr_undo_pass() is changed to 
**			 detect dead logfull return code and sets the 
**			 recovery state, accordingly.
**		      4) For online recovery, dmr_complete_recovery() is 
**			 changed to mark all databases being recovered as 
**	  		 inconsistent; and return error status to 
**			 rcp_recover(), which results in
**			 RCP shutting down the installation.
**		      5) For offline recovery, dmr_init_recovery() is changed to
**			 ensure that journal entries for already-inconsistent 
**			 data- bases will be archived.
**		      6) For offline recovery, dmr_complete_recovery() is 
**			 changed to trigger journal entries for already-
**			 inconsistent data- bases to be purged.
**	23-may-1994 (jnash)
**	    When the last log of the log file is read during offline
**	    recoveries, call LGalter(LG_A_LFLSN) (set last forced lsn) to 
**	    tell the logging system the LSN of the last record.  This fixes 
**	    a problem where value was not set, and should work on standalone 
**	    and clustered installations.
**			 ensure that journal entries for already-inconsistent data-
**			 bases will be archived.
**		      6) For offline recovery, dmr_complete_recovery() is changed
**			 to trigger journal entries for already-inconsistent data-
**			 bases to be purged.
**	15-may-1994 (rmuth)
**	    b59248, a server failure in between a LGend and LKrelease on a 
**		    READONLY transaction would leave the orphaned lock list 
**	            around. This caused by the following..
**	    a) The incorrect pid was stored in the rlp, the now defunct 
**	       "pr_ipid" value was used instead of "pr_pid". Checking for
**	       orphan lock lists for this process used this invalid value and
**	       hence found nothing.
**	    b) The code assumed that there would only ever be one orphaned
**	       lock list. The code now continues until LK indicates that
**	       there are no more lists.
**	06-mar-1996 (stial01 for bryanp)
**	    Use dm0m_lcopy to manipulate the dm0l_record field in the RCP, 
**          since it may exceed 64K in length.
**	22-Apr-1996 (jenjo02)
**	    Added lgs_no_logwriter stat (count of times a logwriter was
**	    needed but unavailable) to dmr_log_statistics().
**	03-jun-1996 (canor01)
**	    Global variables do not need semaphore protection, since routines
**	    are called in single-threaded startup.
**	28-Jun-1996 (jenjo02)
**	    Added lgs_timer_write_time, lgs_timer_write_idle stats to
**	    show what's driving the gcmt thread.
**	01-Oct-1996 (jenjo02)
**	    Added lgs_max_wq (max #buffers on write queue), and
**	    lgs_max_wq_count (#times that max hit).
**	17-oct-1996 (shero03)
**	    Added support for RTree rtdel, rtrep, rtput
**      28-feb-1997 (stial01)
**          Added process_sv_database_lock(), and code to make sure that
**          if online partial recovery of a row locking transaction, 
**          we must have exclusive SV_DATABASE lock. This will block
**          connections to the database while it is being recovered.
**	25-Aug-1997 (jenjo02)
**	    Added log_id parm to dm0p_toss_pages() calls.
**	18-Nov-1997 (hanch04)
**	    Cast result to i4.
**      08-Dec-1997 (hanch04)
**          Use new la_block field when block number is needed instead
**          of calculating it from the offset
**          Keep track of block number and offset into the current block.
**          for support of logs > 2 gig
**      28-may-1998 (stial01)
**          Added NONREDO processing for DM0LSM1RENAME
**	29-Jan-1998 (jenjo02)
**	    Partitioned log files:
**	    added lgh_partitions to log header display.
**      29-Jul-1998 (strpa05)
**              Fix Bug 79361 - axp.osf iircp.log reports wrong number
**              of databases need recovery.  Initialize rcp_db_count
**              to zero. (x int from oping12 c/no 428371)
**      22-Feb-1999 (bonro01)
**          Replace references to svcb_log_id with
**          dmf_svcb->svcb_lctx_ptr->lctx_lgid
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
**	30-Nov-1999 (jenjo02)
**	    Added DB_DIS_TRAN_ID parm to LKcreate_list() prototype.
**	15-Dec-1999 (jenjo02)
**	    Added DB_DIS_TRAN_ID parm to LGbegin() prototype.
**	    Removed DB_DIS_TRAN_ID parm from LKcreate_list() prototype.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	19-may-2001 (devjo01) s103715
**	    Add node information to recovery messages.
**      13-Jul-2001 (horda03) Bug 105246
**          During recovery the LSN number determined after to the Analysis
**          phase must be reset into the header record, as the UNDO phase may
**          move the LSN past the analysis LSN.
**      08-Oct-2002 (hanch04)
**          Removed dm0m_lcopy,dm0m_lfill.  Use MEcopy, MEfill instead.
**	17-jul-2003 (penga03)
**	    Added include adf.h.
**	08-Aug-2003 (inifa01)
**	    Misc GENERIC build change
**	    moved include of 'dm1b.h' before include of 'dmve.h'
**      18-feb-2004 (stial01)
**          Fixed incorrect casting of length arguments.
**      06-apr-2005 (stial01)
**          Added more error handling for some types of redo/undo errors.
**      05-may-2005 (stial01)
**          LGadd for RCP_R_CSP_ONLINE should set flags |= LG_CSP_LDB
**      02-jun-2005 (stial01)
**          Modified previous change to work for online & offline recovery
**      13-jun-2005 (stial01)
**          Renamed LG_CSP_LDB -> LG_CSP_RECOVER
**      25-oct-2005 (stial01)
**          Set DSC_INDEX_INCONST if index marked inconsistent.
**	08-dec-2005 (devjo01) b115592
**	    Make sure databases are opened with DMCM protocol in effect
**	    for online recovery conexts in a cluster environment.
**      30-dec-2005 (stial01 for huazh01)
**          Align buffer passed to LKshow 
**	15-Mar-2006 (jenjo02)
**	    Moved dmr_format_transaction, dmr_format_dis_tran_id,
**	    dmr_log_statistics functionality to dmdlog.
**      29-nov-2005 (horda03) Bug 48659/INGSRV 2716
**          Only Journal ET record where a Journaled BT record has beenm logged.
**	11-Sep-2006 (jonj)
**	    Add LK_LLID ll_id to add_rtx() prototype for recovery of 
**	    LOGFULL_COMMIT transactions.
**	25-Oct-2006 (kschendel) b122120
**	    Add a hash lookup for nonredo entries;  with large logs and
**	    CP intervals, and with the Datallegro change to do insert-select
**	    with (nonredo) LOAD for heaps, the number of nonredo entries
**	    can be large enough to make linear list searching too slow.
**	07-Mar-2007 (jonj)
**	    At end of online redo, toss the cache.
**      10-dec-2007 (stial01)
**          Added rcp_archive(), archive the log before redo/undo processing
**      14-jan-2008 (stial01)
**          apply_redo(), apply_undo() Declare exception handler
**          add_invalid_tbl() unfix tbio/buffer after dmve exceptions.
**          Call dmve_get_tab() instead of extract_tabid
**      28-jan-2008 (stial01)
**          Redo/undo error handling during offline recovery via rcpconfig.
**      31-jan-2008 (stial01)
**          add_invalid_table() redo/undo error on index should default to
**          -rcp_continue_ignore_table (without prompt) 
**          Added -rcp_stop_lsn, to set II_DMFRCP_STOP_ON_INCONS_DB
**	30-May-2008 (jonj)
**	    LSNs are 2 u_i4's; display them in hex rather than
**	    signed decimal.
**	10-Nov-2008 (jonj)
**	    SIR 120874 Change function prototypes to pass DB_ERROR *dberr
**	    instead of i4 *err_code, use new form uleFormat.
**	18-Nov-2008 (jonj)
**	    dm0c_?, dm2d_? functions converted to DB_ERROR *
**	21-Nov-2008 (jonj)
**	    SIR 120874: dm0m_? functions converted to DB_ERROR *
**	25-Nov-2008 (jonj)
**	    SIR 120874: dm0l_? functions converted to DB_ERROR *
**	26-Nov-2008 (jonj)
**	    SIR 120874: dm2t_?, dm2r_? functions converted to DB_ERROR *
**	05-Dec-2008 (jonj)
**	    SIR 120874: dm0p_? functions converted to DB_ERROR *
**      05-may-2009 (stial01)
**          Fixes for II_DMFRCP_PROMPT_ON_INCONS_DB / Db_incons_prompt
**      11-jun-2009
**          More changes for II_DMFRCP_STOP_ON_INCONS_DB and NEW cbf parms
**          offline_recovery_action, online_recovery_action
**      05-may-2009
**	    Partial undo of 05-may-2009 changes (and put back the comment)       
**      01-jul-2009 (stial01)
**          Don't MEfill RTBL (it zeroes fields used by dm0m_deallocate) 
**	19-Aug-2009 (kschendel) 121804
**	    Need cx.h for proper CX declarations (gcc 4.3).
**	04-Feb-2010 (jonj)
**	    SIR 121619 MVCC: Add bufid parameter to dm0l_read().
**/
/*
** Forward function references and global definitions
*/

GLOBALREF LG_LGID	rcp_lg_id;
GLOBALREF i4	rcp_db_id;
GLOBALREF DB_TRAN_ID	rcp_tran_id;
GLOBALREF   char	Db_stop_on_incons_db;
GLOBALREF   i4		Db_startup_error_action;
GLOBALREF   i4		Db_pass_abort_error_action;
GLOBALREF   i4		Db_recovery_error_action;

/* external variables */

static	DB_STATUS	dmr_update_bof(
					RCP		*rcp,
					i4		node_id);
 
static	DB_STATUS	check_recovery(
					RCP		*rcp,
					RDB		*rdb);

static	DB_STATUS	close_config(
					RCP		*rcp,
					RDB		*rdb);

static	VOID		mark_invalid(
					RCP		*rcp,
					RDB		*rdb,
					i4		reason);

static	DB_STATUS	apply_redo(
					RCP		*rcp,
					DM0L_HEADER	*record,
					LG_LA		*lga,
					DMP_DCB		*dcb,
					i4		lock_list);

static	DB_STATUS	apply_undo(
					RCP		*rcp,
					RTX		*rtx,
					DM0L_HEADER	*record,
					LG_LA		*lga);

static	VOID		find_next_abort_rtx(
					RCP		*rcp,
					RTX		**return_tx);

static	DB_STATUS	read_first_log_record(
					RCP		*rcp);

static	DB_STATUS	read_next_log_record(
					RCP		*rcp,
					RTX		*rtx);


static	DB_STATUS	recover_willing_commit(
					RCP		*rcp,
					RTX		*tx );

static	VOID		lookup_rdb(
					RCP		*rcp,
					i4		dbid,
					i4		node_id,
					RDB		**return_db);

static	VOID		lookup_rtx(
					RCP		*rcp,
					DB_TRAN_ID	*tran_id,
					RTX		**return_tx);

static	VOID		lookup_nonredo(
					RCP		*rcp,
					i4		dbid,
					DB_TAB_ID	*tabid,
					RDMU		**return_dmu);

static	VOID		lookup_rlp(
					RCP		*rcp,
					i4		process_id,
					RLP		**return_lp);

static	DB_STATUS	add_rdb(
					RCP		*rcp,
					DMP_LCTX	*lctx,
					i4		dbid,
					DB_DB_NAME	*dbname,
					DB_OWN_NAME	*dbowner,
					DM_PATH		*path,
					i4		path_length,
					i4		internal_dbid,
					LG_LA		*start_recover_addr,
					LG_LSN		*start_recover_lsn,
					i4		recover_flags);

static	DB_STATUS	add_rtx(
					RCP		*rcp,
					DMP_LCTX	*lctx,
					DB_TRAN_ID	*tran_id,
					LK_LLID		ll_id,
					LG_LXID		lx_id,
					LG_LA		*first_lga,
					LG_LA		*last_lga,
					LG_LA		*cp_lga,
					RDB		*rdb,
					char		*user_name);

static	DB_STATUS	add_nonredo(
					RCP		*rcp,
					i4		dbid,
					DB_TAB_ID	*tabid,
					LG_LSN		*lsn);

static	DB_STATUS	add_rlp(
					RCP		*rcp,
					i4		process_id,
					i4		internal_id);

static	VOID		remove_rdb(RDB *rdb);
static	VOID		remove_rtx(RTX *rtx);
static	VOID		remove_nonredo(RDMU *rdmu);
static	VOID		remove_rlp(RLP *rlp);

static	VOID		nuke_rdb(RCP *rcp);

static	VOID		nuke_rtx(
					RCP	 	*rcp,
					RDB		*rdb);

static	VOID		nuke_nonredo(RCP *rcp);

static	VOID		nuke_rlp(RCP *rcp);

static	VOID		dump_transaction(
					RCP		*rcp,
					RTX		*rtx);

static	VOID		display_recovery_context(RCP *rcp);

static	VOID		trace_record(
					LG_RECORD	*record_header,
					DM0L_HEADER	*record,
					LG_LA		*la,
					i4		logfile_blocksize);

static i4 		percent_done(
					DMP_LCTX	*lctx,
					LG_LA		*current_la);

static DB_STATUS	dmr_preinit_recovery(
					RCP		*rcp);

static DB_STATUS        process_sv_database_lock(
					RCP                 *rcp,
					RLP                 *rlp,
					LK_LKB_INFO         *lkb,
					bool                *lock_released);

static	VOID		lookup_invalid_tbl(
					RDB		*rdb,
					DB_TAB_ID	*tabid,
					DM0L_HEADER	*record,
					RTBL		**return_tbl);

static	DB_STATUS	add_invalid_tbl(
					RCP		*rcp,
					RDB		*rdb,
					RTBL		*rtbl,
					DMP_LCTX	*lctx,
					DM0L_HEADER     *record);


static	DB_STATUS	mark_tbl_incons (
					RCP		*rcp,
					RDB		*rdb);

static DB_STATUS	rcp_archive( 
					RCP		*rcp,
					DMP_LCTX 	*lctx);

static STATUS		ex_handler(EX_ARGS *ex_args);

static VOID		recover_force_pages (RCP *rcp);

static	bool		rtbl_ignore_page(
					RTBL		*tbl,
					DM0L_HEADER	*record);

#define			    TRACE_INCR 30  	/* Time increment, in seconds, 
						** between TRdisplays of RCP 
						** status information. */


/*{
** Name: dmr_recover	- Perform the recovery work of the machine failure
**		   recovery.
**
** Description:
**      This routine drives the main algorithm of the machine failure
**	recovery.
**
** Inputs:
**	type			Whether recovery is being done by the RCP or CSP
**
** Outputs:
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      30-Sep-1986 (ac)
**          Created for Jupiter.
**	29-nov-1988 (rogerk)
**	    Save log header EOF value in rcp_eof for the REDO recovery code.
**	    This field should be set in both the machine and server failure
**	    cases.
**	2-Mar-1989 (ac)
**	    Added 2PC support
**	15-mar-1993 (bryanp)
**	    Cluster support: Pass node_id down as 0 to lower-level routines..
**	    Remove header parameter; the correct logfile header is accessed by
**	    performing an LGshow.
**	21-jun-1993 (rogerk)
**	    Fixed dmr_complete_recovery call to pass num_nodes instead of
**	    node_id.  Passing node_id caused us to not logdump when database
**	    inconsistencies are found.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	23-aug-1993 (rogerk)
**	    Added implied setting of DM1315 to get page LSN tracing when
**	    verbose recovery mode is specified.
**	15-oct-98 (stephenb)
**	    Re-write the transaction log header at the end of a successful 
**	    recovery so that the current highest transaction ID is maintained
**	    rather than the one from the last CP (currently in the header).
**	    Re-using transaction IDs after recovery causes major problems 
**	    with replicator (BUG 79489).
**      16-jul-2001 (horda03) bug 105246.
**          Update the Log header information with the LSN obtained during the
**          analysis phase, as the LSN may be moved beyound the LSN by the UNDO
**          phase.
**	20-Apr-2004 (jenjo02)
**	    Made "recovery_needed" an output parm.
*/
DB_STATUS
dmr_recover(
i4		type,
bool		*recovery_needed)
{
    CL_ERR_DESC		sys_err;
    LG_HEADER		*header;
    DB_STATUS		status = E_DB_OK;
    RCP			*rcp;
    i4			i;
    STATUS		cl_status;
    i4		length;
    i4			error;
    RCP_QUEUE		*st;
    RDB			*rdb;
    i4		node_id = 0;
    DMP_LCTX		*lctx;
    LG_HDR_ALTER_SHOW	alter_parms;
    DB_TRAN_ID          save_tranid;

    lctx = dmf_svcb->svcb_lctx_ptr;
    header = &lctx->lctx_lg_header;

    /*
    ** Get the log header so that we can check the status.
    */

    if (cl_status = LGshow(LG_A_HEADER,
			    (PTR)header, sizeof(*header), &length, 
			    &sys_err))
    {
	_VOID_ uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
		(char *)NULL, 0L, (i4 *)NULL, &error, 0);
	_VOID_ uleFormat(NULL, E_DM9017_BAD_LOG_SHOW, &sys_err, ULE_LOG, NULL,
		(char *)NULL, 0L, (i4 *)NULL, &error, 1,
		0, LG_A_HEADER);
	_VOID_ uleFormat(NULL, E_DM9401_RCP_LOGINIT, &sys_err, ULE_LOG, NULL,
		(char *)NULL, 0L, (i4 *)NULL, &error, 0);
	return (E_DB_ERROR);
    }

    status = dmr_alloc_rcp( &rcp, type, (DMP_LCTX **)0, (i4)0 );
    if (status != E_DB_OK)
    {
	uleFormat(NULL, E_DM943C_DMR_RECOVER, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
	    (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	return (E_DB_ERROR);
    }

    /*
    ** If the log file indicates that the EOF is valid but that the last
    ** consistency point address is not known, then scan backwards to
    ** find the last CP.
    */
    if (header->lgh_status == LGH_EOF_OK)
    {
	if (status = dmr_get_cp(rcp, (i4)0))
	{
	    TRdisplay("\nCouldn't find the last consistency point.\n");
	    return (E_DB_ERROR);
	}
    }


    /*
    ** Update the log file status to VALID now that the EOF, BOF and CP
    ** addresses have all been found.  It is now ready to be used.
    **
    ** Remember the previous status as it indicates whether or not
    ** recovery processing is needed before bringing the system online.
    **
    ** If recovery is done below, there will likely be update made to
    ** the log file and the EOF moved forward.  If we crash during
    ** recovery processing, we will need to scan for the new EOF during
    ** RCP startup (indicated by the VALID status).
    */
    *recovery_needed = (header->lgh_status != LGH_OK);
    header->lgh_status = LGH_VALID;
    TRdisplay("%@ RCP: Marking Log File status VALID.\n");

     alter_parms.lg_hdr_lg_header = *header;
     alter_parms.lg_hdr_lg_id = lctx->lctx_lgid;
    cl_status = LGalter(LG_A_NODELOG_HEADER, (PTR)&alter_parms,
			sizeof(alter_parms),&sys_err);
    if (cl_status != OK)
    {
	_VOID_ uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
	    (char *)NULL, 0L, (i4 *)NULL, &error, 0);
	_VOID_ uleFormat(NULL, E_DM900B_BAD_LOG_ALTER, &sys_err, ULE_LOG, NULL,
	    (char *)NULL, 0L, (i4 *)NULL, &error, 1,
	    0, LG_A_NODELOG_HEADER);
	_VOID_ uleFormat(NULL, E_DM9405_RCP_GETCP, &sys_err, ULE_LOG, NULL,
		(char *)NULL, 0L, (i4 *)NULL, &error, 0);
	return (E_DB_ERROR);
    }

    cl_status = LGforce(LG_HDR, lctx->lctx_lxid, 0, 0, &sys_err);
    if (cl_status != OK)
    {
	_VOID_ uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
	    (char *)NULL, 0L, (i4 *)NULL, &error, 0);
	_VOID_ uleFormat(NULL, E_DM9010_BAD_LOG_FORCE, &sys_err, ULE_LOG, NULL,
	    (char *)NULL, 0L, (i4 *)NULL, &error, 1,
	    0, lctx->lctx_lxid);
	_VOID_ uleFormat(NULL, E_DM9405_RCP_GETCP, &sys_err, ULE_LOG, NULL,
		(char *)NULL, 0L, (i4 *)NULL, &error, 0);
	return (E_DB_ERROR);
    }

    if ( !(*recovery_needed) )
	return (E_DB_OK);

    /*
    ** Initialize LCTX fields from the Log Header.
    */
    lctx->lctx_eof.la_sequence = header->lgh_end.la_sequence;
    lctx->lctx_eof.la_block    = header->lgh_end.la_block;
    lctx->lctx_eof.la_offset   = header->lgh_end.la_offset;
    lctx->lctx_bof.la_sequence = header->lgh_begin.la_sequence;
    lctx->lctx_bof.la_block    = header->lgh_begin.la_block;
    lctx->lctx_bof.la_offset   = header->lgh_begin.la_offset;
    lctx->lctx_cp.la_sequence  = header->lgh_cp.la_sequence;
    lctx->lctx_cp.la_block     = header->lgh_cp.la_block;
    lctx->lctx_cp.la_offset    = header->lgh_cp.la_offset;


    for (;;)
    {
	status = dmr_offline_context(rcp, node_id);
	if (status != E_DB_OK)
	    break;

	status = dmr_analysis_pass(rcp, node_id);
	if (status != E_DB_OK)
	    break;

        save_tranid.db_high_tran =
                lctx->lctx_lg_header.lgh_tran_id.db_high_tran;
        save_tranid.db_low_tran =
                lctx->lctx_lg_header.lgh_tran_id.db_low_tran;

	status = dmr_init_recovery(rcp);
	if (status != E_DB_OK)
	    break;

	status = dmr_redo_pass(rcp, node_id);
	if (status != E_DB_OK)
	    break;

	status = dmr_undo_pass(rcp);
	if (status != E_DB_OK)
	    break;

	/*
	** During RCP Restart recovery, now that all the records needed since
	** the last CP have been used, we can move the log file BOF forward
	** to the highest spot indicated by any SETBOF log records found in
	** the analysis pass.
	**
	** We may end up moving the log file BOF forward even more in 
	** dmr_complete_recovery when transactions are marked aborted.
	*/
	if (rcp->rcp_action == RCP_R_STARTUP)
	{
	    status = dmr_update_bof(rcp, node_id);
	    if (status != E_DB_OK)
		break;
	}

	/*
	** Alter header to indicate current highest transaction ID, gleaned on
	** the analysis pass
        ** We need to do another LGshow to refresh the header first (UNDO may
        ** have moved the LSN past the analysis' LSN).
	*/
        if (cl_status = LGshow(LG_A_HEADER,
                               (PTR)header, sizeof(*header), &length,
                               &sys_err))
        {
            _VOID_ uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
                              (char *)NULL, 0L, (i4 *)NULL, &error, 0);
            _VOID_ uleFormat(NULL, E_DM9017_BAD_LOG_SHOW, &sys_err, ULE_LOG, NULL,
                              (char *)NULL, 0L, (i4 *)NULL, &error, 1,
                              0, LG_A_HEADER);
            _VOID_ uleFormat(NULL, E_DM9401_RCP_LOGINIT, &sys_err, ULE_LOG, NULL,
                              (char *)NULL, 0L, (i4 *)NULL, &error, 0);
            return (E_DB_ERROR);
        }

	header->lgh_tran_id.db_high_tran = save_tranid.db_high_tran;
	header->lgh_tran_id.db_low_tran =  save_tranid.db_low_tran;

	alter_parms.lg_hdr_lg_header = *header;
	alter_parms.lg_hdr_lg_id = lctx->lctx_lgid;
	cl_status = LGalter(LG_A_NODELOG_HEADER, (PTR)&alter_parms,
			    sizeof(alter_parms),&sys_err);
	if (cl_status != OK)
	{
	    _VOID_ uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
			      (char *)NULL, 0L, (i4 *)NULL, &error, 0);
	    _VOID_ uleFormat(NULL, E_DM900B_BAD_LOG_ALTER, &sys_err, ULE_LOG, NULL,
			      (char *)NULL, 0L, (i4 *)NULL, &error, 1,
			      0, LG_A_NODELOG_HEADER);
	    _VOID_ uleFormat(NULL, E_DM9405_RCP_GETCP, &sys_err, ULE_LOG, NULL,
			      (char *)NULL, 0L, (i4 *)NULL, &error, 0);
	    return (E_DB_ERROR);
	}

	cl_status = LGforce(LG_HDR, lctx->lctx_lxid, 0, 0, &sys_err);
	if (cl_status != OK)
	{
	    _VOID_ uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
			      (char *)NULL, 0L, (i4 *)NULL, &error, 0);
	    _VOID_ uleFormat(NULL, E_DM9010_BAD_LOG_FORCE, &sys_err, ULE_LOG, NULL,
			      (char *)NULL, 0L, (i4 *)NULL, &error, 1,
			      0, lctx->lctx_lxid);
	    _VOID_ uleFormat(NULL, E_DM9405_RCP_GETCP, &sys_err, ULE_LOG, NULL,
			      (char *)NULL, 0L, (i4 *)NULL, &error, 0);
	    return (E_DB_ERROR);
	}
    
	status = dmr_complete_recovery(rcp, 1);
	if (status != E_DB_OK)
	    break;

	break;
    }

    if (status != E_DB_OK)
    {
	uleFormat(&rcp->rcp_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
	    (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	uleFormat(NULL, E_DM943C_DMR_RECOVER, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
	    (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
    }

    /*
    ** Deallocate structures on the RDB and RTX queues.
    */
    dmr_cleanup(rcp);

    /*
    ** Deallocate the RCP
    */
    dm0m_deallocate((DM_OBJECT **)&rcp);

    return(status);
}

/*
**
**	15-apr-1994 (chiku)
**		Bug56702: initialize rcp_state field.
**      29-Jul-1998 (strpa05)
**              Fix Bug 79361 - axp.osf iircp.log reports wrong number
**              of databases need recovery.  Initialize rcp_db_count
**              to zero. (x int from oping12 c/no 428371)
**	15-Oct-2007 (jonj)
**	    Init rcp_cluster_enabled boolean; some dm0p semantics need
**	    to know if this is a clustered installation regardless of
**	    recovery "type".
*/
STATUS
dmr_alloc_rcp(
RCP **result_rcp,
i4 type,
DMP_LCTX    **lctx_array,
i4	    num_nodes)
{
    CL_ERR_DESC		sys_err;
    RCP			*rcp;
    DB_STATUS		status = E_DB_OK;
    i4			i;
    i4		length;
    i4		error;
    RCP_QUEUE		*st;
    RDB			*rdb;
    DB_ERROR		local_dberr;

    /*
    ** Allocate and initialize the RCP.
    */
    status = dm0m_allocate(sizeof(RCP) + 
			sizeof(DMVE_CB) +
			(RCP_DB_HASH + 1) * sizeof(RDB *) +
			RCP_TX_HASH * sizeof(RTX *) +
			RCP_DMU_HASH * sizeof(RDMU *) +
			RCP_LP_HASH * sizeof(RLP *),
		    0, RCP_CB, RCP_ASCII_ID, 0, (DM_OBJECT **)result_rcp,
		    &local_dberr);
    if (status != E_DB_OK)
    {
	uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
	    (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	uleFormat(NULL, E_DM9407_RCP_RECOVER, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
	    (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	return(E_DB_ERROR);
    }

    /*
    ** Init database and transaction lists and hash queues.
    ** The hash tables come after the RCP state block.
    */
    rcp = *result_rcp;
    CLRDBERR(&rcp->rcp_dberr);
    rcp->rcp_db_count = 0;
    rcp->rcp_dmve_cb = ((char *)rcp + sizeof(RCP));
    rcp->rcp_rdb_map = (RDB**) ((char *)rcp->rcp_dmve_cb + sizeof(DMVE_CB));
    rcp->rcp_txh = 
      (RTX**) ((char *)rcp->rcp_rdb_map + (RCP_DB_HASH + 1) * sizeof(RDB *));
    rcp->rcp_dmuh = 
      (RDMU **) ((char *)rcp->rcp_txh + RCP_TX_HASH * sizeof(RTX *));
    rcp->rcp_lph = (RLP**) ((char *)rcp->rcp_dmuh + RCP_DMU_HASH * sizeof(RDMU *));
    rcp->rcp_db_state.stq_next = (RCP_QUEUE *)&rcp->rcp_db_state.stq_next;
    rcp->rcp_db_state.stq_prev = (RCP_QUEUE *)&rcp->rcp_db_state.stq_next;
    rcp->rcp_tx_state.stq_next = (RCP_QUEUE *)&rcp->rcp_tx_state.stq_next;
    rcp->rcp_tx_state.stq_prev = (RCP_QUEUE *)&rcp->rcp_tx_state.stq_next;
    rcp->rcp_rdmu_state.stq_next = (RCP_QUEUE *)&rcp->rcp_rdmu_state.stq_next;
    rcp->rcp_rdmu_state.stq_prev = (RCP_QUEUE *)&rcp->rcp_rdmu_state.stq_next;

    if (lctx_array)
	rcp->rcp_lctx = lctx_array;
    else
	rcp->rcp_lctx = &dmf_svcb->svcb_lctx_ptr;

    /* Zero out all the hash table pointers */

    MEfill( (RCP_DB_HASH + 1) * sizeof(RDB *), 0, (PTR) rcp->rcp_rdb_map);
    MEfill(RCP_TX_HASH * sizeof(RTX *), 0, (PTR) rcp->rcp_txh);
    MEfill(RCP_DMU_HASH * sizeof(RDMU *), 0, (PTR) rcp->rcp_dmuh);
    MEfill(RCP_LP_HASH * sizeof(RLP *), 0, (PTR) rcp->rcp_lph);

    rcp->rcp_action = type;
    rcp->rcp_tx_count = 0;
    rcp->rcp_rdtx_count = 0;
    rcp->rcp_willing_commit = 0;

    /* Note for all if running in a cluster */
    rcp->rcp_cluster_enabled = CXcluster_enabled();

    /*
    ** Set rcp verbose tracing if DM1306 trace point is defined.
    ** Also define DM1315 to get page LSN tracing if the verbose mode
    ** is specified.
    */
    rcp->rcp_verbose = 0;
    if (DMZ_ASY_MACRO(6))
    {
	rcp->rcp_verbose = 1;
	DMZ_SET_MACRO(dmf_svcb->svcb_trace, 1315);
    }

    TRdisplay("%@ RCP-INIT: Action %w %s\n",
		RCP_ACTION, rcp->rcp_action,
		(rcp->rcp_cluster_enabled) ? "Clustered" : "");

    /*
     * Bug56702: initialize rcp_state.
     */
    rcp->rcp_state = RCP_OK;
    return (OK);
}

/*{
** Name: dmr_online_context - Gather information for online recovery.
**
** Description:
**	This routine builds the recovery context for an Online Recovery
**	event.  It is called by the RCP when some action requires RCP recovery:
**
**	    - server crash
**	    - pass abort
**	    - willing commit manual override
**
**	The recovery context is built by doing repeated LGshows to the logging
**	system to find all Databases and Transactions that are listed as
**	needing recovery.
**
**	All databases and transactions will be recovered in parallel.
**
** Inputs:
**	rcp			Recovery Context
**
** Outputs:
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      14-dec-1992 (rogerk)
**	    Reduced Logging Project: Written.
**	15-mar-1993 (rogerk)
**	    Reduced Logging - Phase IV:
**	    Changed the RCP to track databases via the external database id
**		rather than an internal LG id.  RDB list is now hashed by
**		the external database id.  LGshow calls for transaction and
**		database information were changed to return the external
**		database id in the LG_DATABASE and LG_TRANSACTION info blocks.
**	26-apr-1993 (bryanp)
**	    6.5 Cluster Support:
**		Pass a start_recovery_lsn to add_rdb. Since we don't know what
**		    LSN to pass, pass a zeroed LSN.
**		Pass node_id parameter to lookup_rdb. In non-cluster cases, this
**		    will always be zero, but passing it this way lets us
**		    minimize the number of "if cluster" tests in the code.
**	23-aug-1993 (bryanp)
**	    Handle manual-abort requests for prepared transactions.
**	18-oct-1993 (rogerk)
**	    Added ownername to rtx structure and add_rtx call so that xacts
**	    can be restored with their original owner.
**	15-may-1994 (rmuth)
**	    b59248, a server failure in between a LGend and LKrelease on a 
**		    READONLY transaction would leave the orphaned lock list 
**	            around. This caused by the following..
**	    a) The incorrect pid was stored in the rlp, the now defunct 
**	       "pr_ipid" value was used instead of "pr_pid". Checking for
**	       orphan lock lists for this process used this invalid value and
**	       hence found nothing.
*/
DB_STATUS
dmr_online_context(
RCP		*rcp)
{
    LG_DATABASE		db;
    LG_PROCESS		lpb;
    LG_TRANSACTION	tr;
    LG_LSN		zero_lsn;
    RDB			*rdb;
    RTX			*rtx;
    RLP			*rlp;
    DB_DB_NAME		*dbname;
    DB_OWN_NAME		*dbowner;
    DM_PATH		*dbpath;
    i4			*i4_ptr;
    i4			path_len;
    i4		lgshow_index;
    i4		length;
    i4		recover_flags;
    i4		err_arg;
    DB_STATUS		status = E_DB_OK;
    STATUS		cl_status;
    CL_ERR_DESC		sys_err;
    DMP_LCTX		*lctx = rcp->rcp_lctx[0];
    i4			error;

    CLRDBERR(&rcp->rcp_dberr);

    zero_lsn.lsn_high = zero_lsn.lsn_low = 0;

    TRdisplay("%@ RCP-P0: Online Recovery: Build Recovery Context.\n");
    TRdisplay("\tLog File Window:   BOF <%d,%d,%d> EOF <%d,%d,%d>.\n",
	lctx->lctx_bof.la_sequence,
	lctx->lctx_bof.la_block,
	lctx->lctx_bof.la_offset,
	lctx->lctx_eof.la_sequence,
	lctx->lctx_eof.la_block,
	lctx->lctx_eof.la_offset);
    TRdisplay("\tConsistency Point: CP  <%d,%d,%d>.\n",
	lctx->lctx_cp.la_sequence,
	lctx->lctx_cp.la_block,
	lctx->lctx_cp.la_offset);

    /*
    ** Ask Logging System which databases require Redo Recovery.
    ** Add each such db to our database context.  These are databases
    ** which were open with Fast Commit protocols and whose cache contents
    ** have been lost due to a process failure.  They will need full
    ** database redo processing since the last consistency point.
    **
    ** The list will not at this time include those databases which were
    ** not open with Fast Commit or those which need only specific
    ** transaction recovery - like pass-abort or manual-abort handling.
    ** Those databases will be added into the recovery context below
    ** following the collecting of the transaction recovery list.
    **
    ** Each LGshow returns one database from the Logging System.
    ** We cycle through these until we have seen every db.  We are,
    ** of course, only interested in those which require recovery.
    **
    ** The LGshow loop terminates when lgshow_index returns with value 0.
    ** This indicates that there are no more databases to show.
    */
    TRdisplay("\n\tFind Databases needing recovery:\n");
    lgshow_index = 0;
    while (status == E_DB_OK)
    {
	cl_status = LGshow(LG_N_DATABASE, (PTR)&db, sizeof(db), 
			    &lgshow_index, &sys_err);
	if (cl_status != OK)
	{
	    uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	    SETDBERR(&rcp->rcp_dberr, 0, E_DM9017_BAD_LOG_SHOW);
	    err_arg = LG_N_DATABASE;
	    status = E_DB_ERROR;
	    break;
	}

	/*
	** If we have exhausted the database list, then we are finished.
	*/
	if (lgshow_index == 0)
	    break;

	/* 
	** Skip databases which do not need recovery.
	*/
	if ((db.db_status & DB_RECOVER) == 0)
	{
	    if ((rcp->rcp_verbose) && (! (db.db_status & DB_NOTDB)))
	    {
		TRdisplay("\t    %~t: No REDO recovery needed.\n",
			DB_MAXNAME, db.db_buffer);
	    }
	    continue;
	}

	/*
	** Extract DB information out of info buffer where it has
	** been packed into the db_buffer field.  Sanity check the
	** size of the buffer to make sure it actually holds what
	** we expect it to.
	*/
	i4_ptr = (i4 *) &db.db_buffer[2 * DB_MAXNAME + 4];
	I4ASSIGN_MACRO(*i4_ptr, path_len);
	if (db.db_l_buffer < (2 * DB_MAXNAME + path_len + 8))
	{
	    TRdisplay("LGshow DB buffer size mismatch (%d vs %d)\n",
		(2 * DB_MAXNAME + path_len + 8), db.db_l_buffer);
	    SETDBERR(&rcp->rcp_dberr, 0, E_DM940E_RCP_RCPRECOVER);
	    status = E_DB_ERROR;
	    break;
	}

	dbname = (DB_DB_NAME *) &db.db_buffer[0];
	dbowner = (DB_OWN_NAME *) &db.db_buffer[DB_MAXNAME];
	dbpath = (DM_PATH *) &db.db_buffer[2 * DB_MAXNAME + 8];

	/*
	** Format RDB flags based on db status.
	** Fast Commit db's need full redo.
	*/
	recover_flags = RDB_RECOVER | RDB_REDO;
	if (db.db_status & DB_JOURNAL)
	    recover_flags |= RDB_JOURNAL;

	if (db.db_status & DB_NOTDB)
	    recover_flags = RDB_NOTDB;


	TRdisplay("\t    %~t:  ID: %x, flags: %v\n", 
	    sizeof(DB_DB_NAME), dbname, db.db_database_id, 
	    "RECOVER,JOURNAL,REDO,,NOTDB,FAST_COMMIT,INVALID", recover_flags);

	/*
	** Consistency check:
	** Look for an already existing occurrence of this database.
	** It is an error for one to exist.
	*/
	lookup_rdb(rcp, db.db_database_id, lctx->lctx_node_id, &rdb);
	if (rdb)
	{
	    TRdisplay("DMR-P0: Database %x already being aborted\n",
		db.db_database_id);
	    SETDBERR(&rcp->rcp_dberr, 0, E_DM940E_RCP_RCPRECOVER);
	    status = E_DB_ERROR;
	    break;
	}

	status = add_rdb(rcp, lctx, db.db_database_id, 
		    dbname, dbowner, dbpath, (i4) path_len, 
		    db.db_id, &lctx->lctx_cp, &zero_lsn,
		    recover_flags);
	if (status != E_DB_OK)
	    break;
    }


    /*
    ** Ask Logging System which transactions require Recovery.
    ** Add each such transaction to our recovery context.
    **
    ** Each LGshow returns one transaction from the Logging System.
    ** We cycle through these until we have seen every transaction
    ** in the system.  We are, of course, only interested in those
    ** which require recovery.
    **
    ** The LGshow loop terminates when lgshow_index returns with value 0.
    ** This indicates that there are no more transactions to show.
    */
    TRdisplay("\n\tFind Transactions needing recovery:\n");
    lgshow_index = 0;
    while (status == E_DB_OK)
    {
	cl_status = LGshow(LG_N_TRANSACTION, (PTR)&tr, sizeof(tr), 
			    &lgshow_index, &sys_err);
	if (cl_status != OK)
	{
	    uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	    SETDBERR(&rcp->rcp_dberr, 0, E_DM9017_BAD_LOG_SHOW);
	    err_arg = LG_N_TRANSACTION;
	    status = E_DB_ERROR;
	    break;
	}

	/*
	** If we have exhausted the transaction list, then we are finished.
	*/
	if (lgshow_index == 0)
	    break;

	/* 
	** Skip transactions which do not need recovery.
	*/
	if ((tr.tr_status & (TR_RECOVER | TR_MAN_ABORT | TR_PASS_ABORT)) == 0)
	{
	    if (rcp->rcp_verbose)
	    {
		TRdisplay("\t    Xact %x%x (db %x) does not need recovery.\n",
		    tr.tr_eid.db_high_tran, tr.tr_eid.db_low_tran,
		    tr.tr_database_id);
	    }
	    continue;
	}

	TRdisplay("\t    Xact %x%x (ID %x) Database %x (%x)\n",
	    tr.tr_eid.db_high_tran, tr.tr_eid.db_low_tran, tr.tr_id,
	    tr.tr_database_id, tr.tr_lpd_id);
	TRdisplay("\t\tProcess: %x, Last Log Record <%d,%d,%d>\n",
	    tr.tr_pr_id,
	    tr.tr_last.la_sequence,
	    tr.tr_last.la_block,
	    tr.tr_last.la_offset);
	if ((tr.tr_status & TR_WILLING_COMMIT) && 
	    ( ! (tr.tr_status & TR_MAN_ABORT)))
	{
	    TRdisplay("\t        Recovery Type: Willing Commit\n");
	}
	else
	{
	    TRdisplay("\t        Recovery Type: %v\n",
		",,,,,,,UNDO,,,,PASS_ABORT,,,,,,,MAN_ABORT",
		tr.tr_status & (TR_RECOVER|TR_MAN_ABORT|TR_PASS_ABORT) );
	}

	/*
	** Consistency Check:  Check if this transaction is already
	**     set in our recovery context.  If so, log a message
	**     and skip the new information.
	**
	**     This check is historical - I don't really see how we
	**     can find a duplicate transaction id.  Old comments seem
	**     to indicate that we have incomplete abort processing still
	**     outstanding when we are being called to get new context.
	**
	**     If we can't figure out a legal way for this situation 
	**     to occur we should probably turn it into a real error.
	*/
	lookup_rtx(rcp, &tr.tr_eid, &rtx);
	if (rtx)
	{
	    TRdisplay("DMR-P0: Transaction (%x,%x) already being aborted,\n",
		    tr.tr_eid.db_high_tran, tr.tr_eid.db_low_tran);
	    TRdisplay("\t skipping second occurrence of same ID\n");
	    continue;
	}

	/*
	** Look up the information on the Process which owned this xact.
	** If we do not yet have the information, then build it.
	*/
	lookup_rlp(rcp, tr.tr_pr_id, &rlp);
	if (rlp == 0)
	{
	    lpb.pr_id = tr.tr_pr_id;
	    cl_status = LGshow(LG_S_APROCESS, (PTR)&lpb, sizeof(lpb), 
				    &length, &sys_err);
	    if (cl_status != OK)
	    {
		uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
		SETDBERR(&rcp->rcp_dberr, 0, E_DM9017_BAD_LOG_SHOW);
		err_arg = LG_S_APROCESS;
		status = E_DB_ERROR;
		break;
	    }

	    if (length == 0)
	    {
		TRdisplay("The server %x is not known to the logging system",
		    tr.tr_pr_id);
		SETDBERR(&rcp->rcp_dberr, 0, E_DM940E_RCP_RCPRECOVER);
		status = E_DB_ERROR;
		break;
	    }

	    status = add_rlp(rcp, tr.tr_pr_id, lpb.pr_pid);
	    if (status != E_DB_OK)
		break;

	    lookup_rlp(rcp, tr.tr_pr_id, &rlp);
	}

	/*
	** Look up the information on the Database which owned this xact.
	** If we do not yet have the information then build it.
	*/
	lookup_rdb(rcp, tr.tr_database_id, lctx->lctx_node_id, &rdb);
	if (rdb == 0)
	{
	    /* Show the database by its internal DBID */
	    db.db_id = tr.tr_db_id;
	    cl_status = LGshow(LG_S_ADATABASE, (PTR)&db, sizeof(db), 
				    &length, &sys_err);
	    if (cl_status != OK)
	    {
		uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
		SETDBERR(&rcp->rcp_dberr, 0, E_DM9017_BAD_LOG_SHOW);
		err_arg = LG_S_ADATABASE;
		status = E_DB_ERROR;
		break;
	    }

	    if (length == 0)
	    {
		TRdisplay("Database %x (%x) is not known to the logging system",
		    tr.tr_database_id, tr.tr_db_id);
		SETDBERR(&rcp->rcp_dberr, 0, E_DM940E_RCP_RCPRECOVER);
		status = E_DB_ERROR;
		break;
	    }

	    /*
	    ** Extract DB information out of info buffer where it has
	    ** been packed into the db_buffer field.  Sanity check the
	    ** size of the buffer to make sure it actually holds what
	    ** we expect it to.
	    */
	    i4_ptr = (i4 *) &db.db_buffer[2 * DB_MAXNAME + 4];
	    I4ASSIGN_MACRO(*i4_ptr, path_len);
	    if (db.db_l_buffer < (2 * DB_MAXNAME + path_len + 8))
	    {
		TRdisplay("LGshow DB buffer size mismatch (%d vs %d)\n",
		    (2 * DB_MAXNAME + path_len + 8), db.db_l_buffer);
		SETDBERR(&rcp->rcp_dberr, 0, E_DM940E_RCP_RCPRECOVER);
		status = E_DB_ERROR;
		break;
	    }

	    dbname = (DB_DB_NAME *) &db.db_buffer[0];
	    dbowner = (DB_OWN_NAME *) &db.db_buffer[DB_MAXNAME];
	    dbpath = (DM_PATH *) &db.db_buffer[2 * DB_MAXNAME + 8];

	    /*
	    ** Format RDB flags based on db status.
	    ** Since this RDB was not listed in the databases requiring full
	    ** redo, it must be a partial redo database (redo needs only be
	    ** done for those transactions which are being recovered).
	    */
	    recover_flags = RDB_RECOVER | RDB_REDO_PARTIAL;
	    if (db.db_status & DB_JOURNAL)
		recover_flags |= RDB_JOURNAL;

	    if (db.db_status & DB_NOTDB)
		recover_flags = RDB_NOTDB;

	    TRdisplay("\t    %~t ID: %x, flags: %v\n", 
		sizeof(DB_DB_NAME), dbname, db.db_database_id, 
		"RECOVER,JOURNAL,REDO,,NOTDB,FAST_COMMIT,INVALID", 
		recover_flags);

	    status = add_rdb(rcp, lctx, db.db_database_id,
			dbname, dbowner, dbpath, (i4) path_len, 
			db.db_id, &lctx->lctx_cp, &zero_lsn,
			recover_flags);
	    if (status != E_DB_OK)
		break;

	    lookup_rdb(rcp, tr.tr_database_id, lctx->lctx_node_id, &rdb);
	}


	/*
	** Now build the Transaction Context.
	**
	** Pass the LSN of last log record written for this transaction.
	** This is where we will begin abort processing.
	*/
	status = add_rtx(rcp, lctx, &tr.tr_eid, (LK_LLID)tr.tr_lock_id, tr.tr_id, 
		    (LG_LA *) &tr.tr_first, (LG_LA *) &tr.tr_last, 
		    (LG_LA *) &tr.tr_cp, rdb, tr.tr_user_name);
	if (status != E_DB_OK)
	    break;

	lookup_rtx(rcp, &tr.tr_eid, &rtx);

	/*
	** Initialize transaction recovery type.
	*/
	rtx->rtx_recover_type = RTX_REDO_UNDO;

	/*
	** If the transaction is inactive, list it as completed so we know
	** that no undo work is actually required (other than to remove
	** the transaction from the logging system at the end of recovery).
	*/
	if (tr.tr_status & TR_INACTIVE)
	    rtx->rtx_status |= RTX_TRAN_COMPLETED;

	/*
	** Set pass abort status so dmr_complete_recovery processing will
	** know how to correctly end the transaction.
	*/
	if (tr.tr_status & TR_PASS_ABORT)
	    rtx->rtx_status |= RTX_PASS_ABORT;

	/*
	** Willing commit transactions need no redo or undo processing since
	** they are not being aborted and all their updates have been forced
	** to the database at the time of the Prepare to Commit.  We will 
	** need to reaquire their locks and re-establish their transaciton.
	*/
	if ((tr.tr_status & (TR_WILLING_COMMIT|TR_MAN_ABORT)) ==
							TR_WILLING_COMMIT)
	    rtx->rtx_recover_type = RTX_WILLING_COMMIT;

	/*
	** If a prepared transaction is being aborted manually, record it:
	*/
	if (tr.tr_status & TR_MAN_ABORT)
	    rtx->rtx_status |= RTX_MAN_ABORT;
    }

    if (status != E_DB_OK)
    {
	/* If got LGshow error, then pass the show error parameter */
	if (rcp->rcp_dberr.err_code == E_DM9017_BAD_LOG_SHOW)
	{
	    uleFormat(&rcp->rcp_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &error, 1,
		0, err_arg);
	}
	else
	{
	    uleFormat(&rcp->rcp_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	}

	SETDBERR(&rcp->rcp_dberr, 0, E_DM942D_RCP_ONLINE_CONTEXT);
	return (E_DB_ERROR);
    }

    return (E_DB_OK);
}

/*{
** Name: dmr_offline_context - Gather information for offline recovery.
**
** Description:
**	This routine builds the recovery context for an offline recovery
**	event.  It is called by the RCP and CSP to recover work left by
**	a failed node or installation.
**
**	This routine only builds the initial recovery context by decoding
**	the last Consistency Point log record and collecting information
**	about what databases and transactions were open at that time.
**
**	This context will be (perhaps greatly) altered during the analysis
**	pass.  At the end of the analysis pass we will know what databases
**	and transactions need recovery.
**
** Inputs:
**	rcp			Recovery Context
**
** Outputs:
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      14-dec-1992 (rogerk)
**	    Reduced Logging Project: Written.
**	15-mar-1993 (rogerk)
**	    Reduced Logging - Phase IV:
**	    Clarified use of LSN vs. Log Adress.  Log Reads return Log
**		Addresses.  The LSN of a record can be extracted from the
**		log record header.
**	    Changed the log record format to include the database and
**		transaction ids rather than having them in the lctx_header.
**	    Changed the RCP to track databases via the external database id
**		rather than an internal LG id.  RDB list is now hashed by
**		the external database id.  LGshow calls for transaction and
**		database information were changed to return the external
**		database id in the LG_DATABASE and LG_TRANSACTION info blocks.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**		Add node_id parameter.
**		Pass correct transaction handle to dm0l_read.
**		Pass start recovery LSN to add_rdb.
**		Pass node_id paramter to lookup_rdb.
**	21-jun-1993 (rogerk)
**	    Added RTX_FOUND_ONLY_IN_CP status to indicate that a transaction
**	    was added to the active list by being listed in the BCP record
**	    at the start of the offline recovery.  This status is turned off if
**	    any actual log record is found for that transaction in the analysis
**	    pass.  If the RCP finds a transaction listed as open in a CP but
**	    with no other evidence of the transaction's existence, then it
**	    is possible that the transaction was actually resolved and that
**	    the ET record lies just previous to the CP (a current possible
**	    boundary condition).  The RCP will assume that it is safe to treat
**	    a transaction marked RTX_FOUND_ONLY_IN_CP as resolved if the db
**	    is found to be marked closed or if the tx's last written log record
**	    lies previous to the Log File BOF.
**	18-oct-1993 (rogerk)
**	    Added ownername to rtx structure and add_rtx call so that xacts
**	    can be restored with their original owner.
**	6-Apr-2005 (schka24)
**	    If the CP has a transaction recorded as being in willing-
**	    commit state, mark it so here.   That way, the rcp log display
**	    correctly notes them.
**       1-May-2006 (hanal04) bug 116059
**          BCP records are not guaranteed to be contiguous. Skip over
**          non-BCP records. We'll exit we we hit CP_LAST or EOF.
*/
DB_STATUS
dmr_offline_context(
RCP		*rcp,
i4		node_id)
{
    DMP_LCTX		*lctx = rcp->rcp_lctx[node_id];
    DB_STATUS		status = E_DB_OK;
    DM0L_HEADER		*record;
    DM0L_BCP		*bcp;
    RDB			*rdb;
    RTX			*rtx;
    LG_LA		la;
    LG_LA		txla;
    LG_LA		tx1la;
    DB_TRAN_ID		tranid;
    bool		complete_cp_read = FALSE;
    i4		dbid;
    i4		recover_flags;
    i4		i;
    i4			tx_status;
    i4			error;

    CLRDBERR(&rcp->rcp_dberr);

    TRdisplay("%@ RCP-P0: Offline Recovery: Build Recovery Context.\n");
    TRdisplay("\tLog File Window:   BOF <%d,%d,%d> EOF <%d,%d,%d>.\n",
	lctx->lctx_bof.la_sequence,
	lctx->lctx_bof.la_block,
	lctx->lctx_bof.la_offset,
	lctx->lctx_eof.la_sequence,
	lctx->lctx_eof.la_block,
	lctx->lctx_eof.la_offset);
    TRdisplay("\tConsistency Point: CP  <%d,%d,%d>.\n",
	lctx->lctx_cp.la_sequence,
	lctx->lctx_cp.la_block,
	lctx->lctx_cp.la_offset);
    TRdisplay("%@ RCP-P0: Reading Information from Consistency Point.\n");

    /*
    ** Position the log file to the beginning of the last BCP.
    */
    status = dm0l_position(DM0L_FLGA, lctx->lctx_lxid,
			    &lctx->lctx_cp, lctx, &rcp->rcp_dberr);
    if (status != E_DB_OK)
    {
	uleFormat(&rcp->rcp_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
	    (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	SETDBERR(&rcp->rcp_dberr, 0, E_DM9408_RCP_P0);
	return (E_DB_ERROR);
    }

    /*
    ** Begin reading the CP records.  There will likely be more than one
    ** of them to read through.
    **
    ** The CP records will indicate which databases and transactions were
    ** active at the time the Consistency Point was taken.
    */
    for (;;)
    {
	status = dm0l_read(lctx, lctx->lctx_lxid, &record, &la,
				(i4*)NULL, &rcp->rcp_dberr);
	if (status != E_DB_OK)
	    break;

	/*
	** If we find a record other than a Consistency Point record, then
	** skip over it. BCP log entries are not guaranteed to be
        ** contiguous.
	*/
	if (record->type != DM0LBCP)
	{
	    continue;
	}

	bcp = (DM0L_BCP*) record;


	/*
	** Decode the entries in the Consistency Point record.
	** CP records come in 2 flavors:
	**
	**   - CP_DB : Gives list of open databases
	**   - CP_TX : Gives list of open transactions
	**
	** The CP_DB records will preceed the CP_TX records and the
	** last record will be marked CP_LAST.
	*/

	if (bcp->bcp_type == CP_DB)
	{
	    /*
	    ** Create an RDB for each database in record.
	    */
	    for (i = 0; i < bcp->bcp_count; i++)
	    {
		dbid = bcp->bcp_subtype.type_db[i].db_ext_dbid;

		/*
		** Skip non-database entries used for LG bookkeeping.
		*/
		if (bcp->bcp_subtype.type_db[i].db_status & DB_NOTDB)
		    continue;

		/*
		** Format RDB flags based on db status.
		*/
		recover_flags = RDB_RECOVER;
		if (bcp->bcp_subtype.type_db[i].db_status & DB_JOURNAL)
		    recover_flags |= RDB_JOURNAL;

		/*
		** Fast Commit db's need full redo, non fast commit need only
		** perform redo on open transactions.
		*/
		if (bcp->bcp_subtype.type_db[i].db_status & DB_FAST_COMMIT)
		    recover_flags |= RDB_REDO;
		else
		    recover_flags |= RDB_REDO_PARTIAL;

		if (bcp->bcp_subtype.type_db[i].db_status & DB_NOTDB)
		    recover_flags = RDB_NOTDB;

		/*
		** Consistency Check : Check if database id is already known.
		** It should be impossible for the same consistency point
		** to reference two databases with the same database id.
		*/
		lookup_rdb(rcp, dbid, lctx->lctx_node_id, &rdb);
		if (rdb)
		{
		    TRdisplay("%@ RCP-P0: Duplicate DB in CP: id %x\n", dbid);
		    SETDBERR(&rcp->rcp_dberr, 0, E_DM9408_RCP_P0);
		    status = E_DB_ERROR;
		    break;
		}

		if (rcp->rcp_verbose)
		{
		    TRdisplay("\t    %~t ID: %x, flags: %v\n", 
			sizeof (DB_DB_NAME),
			&bcp->bcp_subtype.type_db[i].db_name, dbid, 
			"RECOVER,JOURNAL,REDO,,NOTDB,FAST_COMMIT,INVALID", 
			recover_flags);
		}

		status = add_rdb(rcp, lctx, dbid, 
			    &bcp->bcp_subtype.type_db[i].db_name,
			    (DB_OWN_NAME*)&bcp->bcp_subtype.type_db[i].db_owner,
			    (DM_PATH *) &bcp->bcp_subtype.type_db[i].db_root,
			    bcp->bcp_subtype.type_db[i].db_l_root,
			    0, &lctx->lctx_cp, &record->lsn, recover_flags);
		if (status != E_DB_OK)
		    break;
	    }
	}
	else if (bcp->bcp_type == CP_TX)
	{
	    /*
	    ** Create an RTX for each transaction in record.
	    */
	    for (i = 0; i < bcp->bcp_count; i++)
	    {
		dbid = bcp->bcp_subtype.type_tx[i].tx_dbid;
		tx_status = bcp->bcp_subtype.type_tx[i].tx_status;
		STRUCT_ASSIGN_MACRO(bcp->bcp_subtype.type_tx[i].tx_tran,tranid);
		STRUCT_ASSIGN_MACRO(bcp->bcp_subtype.type_tx[i].tx_last, txla);
		STRUCT_ASSIGN_MACRO(bcp->bcp_subtype.type_tx[i].tx_first,tx1la);

		if (rcp->rcp_verbose)
		{
		    TRdisplay("\t    Xact %x%x Database %x\n",
			tranid.db_high_tran, tranid.db_low_tran, dbid);
		    TRdisplay("\t         Last Log Record <%d,%d,%d>\n",
			txla.la_sequence,
			txla.la_block,
			txla.la_offset);
		    if (tx_status & TR_WILLING_COMMIT)
			TRdisplay("\t         Recovery Type: Willing Commit\n");
		    else
			TRdisplay("\t         Recovery Type: UNDO\n");
		}

		/*
		** Check the Log Address of the last written log record of
		** this transaction for legality.  If the log record falls
		** previous to the Log File BOF then we make the assumption
		** that this transaction was actually resolved and its ET
		** written, but that the BCP got written before the logging
		** system was informed that the xact had ended (and thus it
		** got listed in the CP).
		*/
		if (LGA_LTE(&txla, &lctx->lctx_bof))
		{
		    TRdisplay("\t      Transaction Presumed Resolved -\n");
		    TRdisplay("\t\tLog Record occurs before Log File BOF\n");
		    continue;
		}

		/*
		** Find the RDB that owns this transaction.
		*/
		lookup_rdb(rcp, dbid, lctx->lctx_node_id, &rdb);
		if (rdb == 0)
		{
		    TRdisplay("%@ RCP-P0: Can't find DB in CP: id %x\n", dbid);
		    SETDBERR(&rcp->rcp_dberr, 0, E_DM9408_RCP_P0);
		    status = E_DB_ERROR;
		    break;
		}

		/*
		** Add transaction context with information from the BCP.
		** Since the CP address just previous to the first log record
		** is not logged in the BCP, use the current log file BOF.
		** This is guaranteed to be previous to the first log record.
		*/
		status = add_rtx(rcp, lctx, &tranid, (LK_LLID)0, 0, 
		    &tx1la, &txla, &lctx->lctx_bof, rdb, 
		    bcp->bcp_subtype.type_tx[i].tx_user_name.db_own_name);
		if (status != E_DB_OK)
		    break;

		/*
		** Add RTX_FOUND_ONLY_IN_CP status to the transaction.  This
		** will be turned off if an actual log record is found for the
		** transaction.  It is possible for a transaction marked in
		** this state to have actually been resolved and an ET record
		** written, but for it to still be listed in the BCP.
		*/
		lookup_rtx(rcp, &tranid, &rtx);
		rtx->rtx_status |= RTX_FOUND_ONLY_IN_CP;

		/* If the transaction was marked as a willing commit
		** transaction, make it so for recovery.  This isn't strictly
		** necessary, as we'll see the P1 record during UNDO
		** recovery, but it's more straightforward this way.
		*/
		if (tx_status & TR_WILLING_COMMIT)
		{
		    rtx->rtx_recover_type = RTX_WILLING_COMMIT;
		    ++ rcp->rcp_willing_commit;
		}
	    }
	}

	/*
	** Check for the last record of the CP group. 
	*/
	if (bcp->bcp_flag & CP_LAST)
	{
	    complete_cp_read = TRUE;
	    break;
	}
    }


    /*
    ** Report an error if we could not successfully complete processing of 
    ** the CP records.
    */
    if ( ! complete_cp_read)
    {
	TRdisplay("%@ RCP-P0: Found non CP log record at <%d,%d,%d>\n",
	    la.la_sequence,
	    la.la_block,
	    la.la_offset);
	SETDBERR(&rcp->rcp_dberr, 0, E_DM942E_RCP_INCOMPLETE_CP);
	status = E_DB_ERROR;
    }

    if (status != E_DB_OK)
    {
	uleFormat(&rcp->rcp_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
	    (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);

	SETDBERR(&rcp->rcp_dberr, 0, E_DM9408_RCP_P0);
	return (E_DB_ERROR);
    }

    return (E_DB_OK);
}

/*{
** Name: dmr_init_recovery - Initialize recovery context.
**
** Description:
**	This routine is called when the recovery context is complete, following
**	the analysis pass of the log file.
**
**	It reads through the list of databases and transactions we are about
**	to recover and does any pre-processing necessary:
**
**	During Offline Recovery:
**	    Call the Logging System to add each database being recovered.
**	    Open each database for the current process
**
**	During Online Recovery:
**	    Attach to each transaction's lock list and release all physical
**		config file locks and core catalog page locks to avoid 
**	        deadlocking during the attempt to open the databases.
**	    Open each database for the current process
**	    Adopt ownership of each transaction and release all physical
**		page locks to avoid deadlocking during recovery.
**
**	Prior to building the recovery context, each database is checked by
**	looking in its config file to see if that database is really listed
**	as open.  A database that is listed as closed cannot need recovery.
**	This is done because an accurate count of opendbs of each database
**	is not kept in the logfile information and we cannot tell during the
**	analysis pass which databases had been successfully closed before the
**	failure occurred.  When we see a closedb record in the analysis pass,
**	we leave the database context around expecting this routine to check
**	to see if the database is really closed.
**
** Inputs:
**	rcp			Recovery Context
**
** Outputs:
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      14-dec-1992 (rogerk)
**	    Reduced Logging Project: Written.
**	15-mar-1993 (rogerk)
**	    Reduced Logging - Phase IV:
**	    Changed the RCP to track databases via the external database id
**		rather than an internal LG id.  RDB list is now hashed by
**		the external database id.  LGshow calls for transaction and
**		database information were changed to return the external
**		database id in the LG_DATABASE and LG_TRANSACTION info blocks.
**	    Removed dm0l_opendb/dm0l_closedb calls to use LGadd/LGremove.
**		We no longer add the database using the same internal LG DBID
**		that it was previously open with;  a normal LGadd is done with
**		no ADDONLY flag.  We alter the lg db info to show any required
**		journal work - this used to be done automatically by LGadd
**		when the old LG_ADDONLY flag was passed.
**	26-apr-1993 (bryanp/andys/keving)
**	    6.5 Cluster Support:
**		When performing multi-node recovery, there is an RDB per
**		    database needing recovery per node. Hence we must pass the
**		    proper node ID to check_recovery so that it knows on which
**		    node this RDB is to be checked.
**		When calling LGadd, pass a DM0L_ADDDB structure, rather than
**		    passing a pointer to the middle of the DM0L_OPENDB
**		    log record. This removes the dependency on having the fields
**		    in the DM0L_OPENDB log record exactly matching the
**		    corresponding fields in the DM0L_ADDDB.
**		Adjusted some rcp_action tests to accomodate cluster needes.
**		Add dbs with the DM2D_CANEXIST flag set.
**	21-june-1993 (jnash)
**	    Assume ownership of transactions in databases marked inconsistent.
**	26-jul-1993 (bryanp)
**	    When creating the rcp_lock_list, if this is the CSP, pass
**		LK_MASTER_CSP to LKcreate_list().
**	26-jul-1993 (rogerk)
**	  - Added rtx_orig_last_lga, used to store the log address of the
**	    last log record written by the transaction before recovery started.
**	    This value is not altered in the redo or undo pass.  Set the
**	    original last_lga here before starting recovery processing.
**	  - Changed some RCP trace outputs to try to make it more readable.
**	23-aug-1993 (jnash)
**	  - During offline recovery, open databases as fast-commit, single 
**	    cache, "not RCP" (DM2D_BMSINGLE, DM2D_FASTCOMMIT, ~DM2D_ONLINE_RCP) 
**	    so that the buffer manager does not do unnecessary forces.
**	  - Changed DM2D_RCP to DM2D_ONLINE_RCP as it is specified online
**	    during offline recovery.
**	18-oct-1993 (jnash & rogerk)
**	  - Changed offline recovery to not actually request a database lock
**	    because this prevented willing commit transaction recovery from
**	    reacquiring their database locks.
**	  - Remove extraneous ';' from after if statement that cause if block
**	    to be always executed.  This was causing us to set the journal
**	    window for non-journaled dbs.
**	  - Changed to pass original transaction owner in the LGbegin call
**	    so that willing commit xacts will be restored with the proper user.
**	  - Added LG_NORESERVE flag to xact begins since the transactions
**	    restored in offline recovery will be writing CLR records but will
**	    not have any reserved logspace.
**	31-jan-1994 (rogerk)
**	    Added dmr_preinit_recovery routine to release locks that prevent
**	    the opendb calls from succeeding (config and core catalog locks).
**	    This routine now also does the adopting of transaction lock lists
**	    to the RCP.
**	15-apr-1994 (chiku)
**	    Bug56702: allow journal entries for already inconsistent databases
**		      to be archived.
** 	08-apr-1997 (wonst02)
** 	    Parameter change for dm2d_add_db().
**      27-May-1999 (hanal04)
**          Parameter change for dm2d_add_db(). b97083.
**	08-dec-2005 (devjo01) b115592
**	    See DM2D_DMCM in flag word passed to dm2d_add_db if
**	    this is an online recovery context, and this is
**	    a clustered installation.
*/
DB_STATUS
dmr_init_recovery(
RCP		*rcp)
{
    DB_STATUS		status = E_DB_OK;
    STATUS		cl_status;
    RCP_QUEUE		*st;
    RDB			*rdb;
    RTX			*tx;
    LG_CONTEXT		ctx;
    LG_TRANSACTION	tr;
    LG_DATABASE		db;
    DB_TRAN_ID		tran_id;
    DM2D_LOC_ENTRY	location;
    DM0L_ADDDB		add;
    CL_ERR_DESC		sys_err;
    i4		flags;
    i4		lock_mode;
    i4		length;
    i4		dm2mode;
    DMP_LCTX		*lctx;
    i4			error;

    CLRDBERR(&rcp->rcp_dberr);

    /*
    ** Create lock list to use during recovery processing.
    */
    flags = (LK_ASSIGN | LK_NONPROTECT | LK_MASTER);
    if ((rcp->rcp_action == RCP_R_CSP_STARTUP) ||
	(rcp->rcp_action == RCP_R_CSP_ONLINE))
	flags |= LK_MASTER_CSP;

    cl_status = LKcreate_list(flags,
	(LK_LLID)0, (LK_UNIQUE *)0, (LK_LLID *)&rcp->rcp_lock_list, 
	0, &sys_err);
    if (cl_status != OK)
    {
	uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL, 
	    (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	uleFormat(NULL, E_DM901A_BAD_LOCK_CREATE, &sys_err, ULE_LOG, NULL, 
	    (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	SETDBERR(&rcp->rcp_dberr, 0, E_DM9409_RCP_P1);
	return(E_DB_ERROR);
    }

    /*
    ** Initializing databases for recovery currently requires us to execute
    ** a logical open of these databases.  Unfortunately, to some degree this
    ** violates the recovery model since this logical open requires certain
    ** resources to be free and in a consistent state - even though recovery
    ** has not yet been performed on the database.
    **
    ** Ideally, nothing would be required of the database until REDO recovery
    ** was run, at which time the database would be physically consistent
    ** and those objects which are updated with physical locks would be
    ** accessable.  Today, we have special requirements on the Config File
    ** and the core catalog pages since they are accessed before redo recovery.
    */
    TRdisplay("%@ RCP-P1: Initialize System for Recovery.\n");
    status = dmr_preinit_recovery(rcp);
    if (status != E_DB_OK)
    {
	uleFormat(&rcp->rcp_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
	    (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	SETDBERR(&rcp->rcp_dberr, 0, E_DM9409_RCP_P1);
	return (E_DB_ERROR);
    }

    /*
    ** Check each database in the database recovery list.  This list includes
    ** all databases have have been accessed since the last Consistency Point.
    **
    ** Verify that the database needs recovery by checking the open count
    ** in the database config file.  The open_count (which is actually not
    ** a count but a bitmask of nodes which have the database open) indicates
    ** whether the database is currently open or closed - or in the case of
    ** crash recovery, whether the database was open or closed at the time
    ** of the crash.
    **
    ** Ingres guarantees that the physical database is uptodate and consistent,
    ** and that all necessary log records have been archived to the journal
    ** files, before registering it as closed in the config file.
    **
    ** If a database which is found to be Not Open, then it must have been
    ** successfully closed before the recovery action was signaled.
    */
    TRdisplay("%@ RCP-P1: Check Recovery Context.\n");
    for (st = rcp->rcp_db_state.stq_next;
	st != (RCP_QUEUE *)&rcp->rcp_db_state.stq_next;
	st = rdb->rdb_state.stq_next)
    {
	rdb = (RDB *)((char *)st - CL_OFFSETOF(RDB, rdb_state));

	if (rdb->rdb_status & RDB_NOTDB)
	    continue;

	status = check_recovery(rcp, rdb);
	if (status != E_DB_OK)
	{
	    /*
	    ** If an error occurs processing a database, that database will
	    ** not be recovered, but we continue processing so the rest of
	    ** the databases can be recovered.
	    */
	    uleFormat(&rcp->rcp_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	    uleFormat(NULL, E_DM9430_RCP_CHECK_REC_ERROR, (CL_ERR_DESC *)NULL, ULE_LOG,
		NULL, (char *)NULL, (i4)0, (i4 *)NULL, &error, 1,
		sizeof(rdb->rdb_name), &rdb->rdb_name);
	    mark_invalid(rcp, rdb, DSC_OPENDB_INCONSIST);
	    CLRDBERR(&rcp->rcp_dberr);
	    status = E_DB_OK;
	}
    }

    /*
    ** Trace output indicating recovery context.
    */
    display_recovery_context(rcp);
    TRdisplay("%@ RCP-P1: Prepare Databases for Recovery.\n");

    /*
    ** Prepare each database for recovery:
    ** 
    **     - During offline (startup) recovery call the logging system to
    **       mark the database open for the installation and to indicate
    **       that recovery is in progress.  (During online recovery,
    **       the database is already listed as open by the logging system).
    **
    **     - Add and open the database for this process so that database
    **       operations can be performed on it.
    */
    for (st = rcp->rcp_db_state.stq_next;
	st != (RCP_QUEUE *)&rcp->rcp_db_state.stq_next;
	st = rdb->rdb_state.stq_next)
    {
	rdb = (RDB *)((char *)st - CL_OFFSETOF(RDB, rdb_state));

	/*
	** Skip dbs not needed during recovery.
	*/
	if ( ( ! (rdb->rdb_status & RDB_RECOVER)) &&
	    /*
	     * Bug56702: databases that was open but marked
	     *		 may have journal entries to archive.
	     */
             (! ( ( rdb->rdb_status & RDB_ALREADY_INCONSISTENT ) &&
			   ( rdb->rdb_status & RDB_INVALID ) ) ) )
	    continue;

	lctx = rdb->rdb_lctx;

	for (;;)
	{
	    /*
	    ** During offline recovery, add the database to the logging 
	    ** system.  This restores the LG context that existed when
	    ** the system crashed and allows new work to be done on behalf
	    ** of this transaction.  It also lets the archiver see if there
	    ** is journal work to do.
	    **
	    ** After adding the DB, the LG state is altered to indicate any
	    ** necessary journal archiving required.
	    **
	    ** For the purpose of this particular processing, CSP "online"
	    ** processing is not really online: the logging system on the failed
	    ** node has been vaporized, and so we must restore the LG context
	    ** that existed when that node crashed (although we're actually
	    ** restoring the context on this node, not on that node).
	    */
	    if (rcp->rcp_action != RCP_R_ONLINE)
	    {


		/*
		** Construct a DM0L_ADDDB structure which describes this 
		** database. This database information is stored within 
		** the logging system and retrieved by various callers using 
		** LGshow(). Since the root location is variable length, the 
		** length computation in the LGadd call is somewhat ugly.
		*/

		MEcopy((char *)&rdb->rdb_name, sizeof(rdb->rdb_name), 
			(char *)&add.ad_dbname);
		MEcopy((char *)&rdb->rdb_owner, sizeof(rdb->rdb_owner), 
			(char *)&add.ad_dbowner);
		MEcopy((char *)&rdb->rdb_root, sizeof(rdb->rdb_root), 
			(char *)&add.ad_root);
		add.ad_l_root = rdb->rdb_l_root;
		add.ad_dbid = rdb->rdb_ext_dbid;

		flags = (rdb->rdb_status & RDB_JOURNAL) ? LG_JOURNAL : 0;

#if defined(conf_CLUSTER_BUILD)
		TRdisplay("%@ RCP-P1: Prepare %~t for node %d\n",
		    DB_MAXNAME, (PTR)&add.ad_dbname,
		    lctx->lctx_node_id);

		if (lctx->lctx_node_id != CXnode_number(NULL))
		{
		    /*
		    ** CSP is performing recovery for another node
		    **
		    ** Pass a flag to make sure LPD gets private LDB
		    ** LDB context contains log jnl address (for local log)
		    ** CSP needs private LDB context (for remote log)
		    */
		    flags |= LG_CSP_RECOVER;
		}
#endif
 
		cl_status = LGadd(lctx->lctx_lgid, flags, (char *) &add,
		    ((char *)&add.ad_root) - ((char *)&add.ad_dbname) + 
							    add.ad_l_root,
		    &rdb->rdb_dbopen_id, &sys_err);
		if (cl_status != OK)
		{
		    uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
			(char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
		    uleFormat(NULL, E_DM900A_BAD_LOG_DBADD, &sys_err, ULE_LOG, NULL,
			(char *)NULL, 0L, (i4 *)NULL, &error, 4,
			0, lctx->lctx_lgid,
			sizeof(add.ad_dbname), &add.ad_dbname,
			sizeof(add.ad_dbowner), &add.ad_dbowner,
			add.ad_l_root, &add.ad_root);
		    SETDBERR(&rcp->rcp_dberr, 0, E_DM9409_RCP_P1);
		    status = E_DB_ERROR;
		    break;
		}

		/*
		** Lookup the database's internal LG dbid for use in future
		** LG calls.
		*/
		db.db_database_id = rdb->rdb_ext_dbid;
		cl_status = LGshow(LG_S_DBID, (PTR)&db, sizeof(db),
				    &length, &sys_err);
		if (cl_status != OK)
		{
		    uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
			(char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
		    uleFormat(NULL, E_DM9017_BAD_LOG_SHOW, &sys_err, ULE_LOG, NULL,
			(char *)NULL, (i4)0, (i4 *)NULL, &error, 1,
			0, LG_S_DBID);
		    SETDBERR(&rcp->rcp_dberr, 0, E_DM9409_RCP_P1);
		    status = E_DB_ERROR;
		    break;
		}
		rdb->rdb_lg_dbid = db.db_id;

		/*
		** If this database is journaled, then alter the LG database
		** information to show that archiving will be needed for the
		** database.
		**
		** Since we don't accurately determine in the analysis pass
		** where the 1st and last journal records are for each database,
		** we tell the logging system that potentially the entire
		** log file must be archived (using the Begin and End
		** addresses as the journal window).  Hopefully the archiver
		** will be able to use the database config file's previous
		** journal address to reduce the amount of logfile actually
		** needed to process.
		*/
		if ((rdb->rdb_status & RDB_JOURNAL) &&
		    (RCP_R_CSP_ONLINE != rcp->rcp_action))
		{
		    db.db_id = rdb->rdb_lg_dbid;
		    STRUCT_ASSIGN_MACRO(lctx->lctx_bof,
					db.db_f_jnl_la);
		    STRUCT_ASSIGN_MACRO(lctx->lctx_eof,
					db.db_l_jnl_la);
	 
		    cl_status = LGalter(LG_A_DBCONTEXT, (PTR)&db, sizeof(db),
			&sys_err);
		    if (cl_status != OK)
		    {
			uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
			    (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
			uleFormat(NULL, E_DM900B_BAD_LOG_ALTER, &sys_err, ULE_LOG, 
			    NULL, (char *)NULL, (i4)0, (i4 *)NULL, &error,
			    1, 0, LG_A_DBCONTEXT);
			SETDBERR(&rcp->rcp_dberr, 0, E_DM9409_RCP_P1);
			status = E_DB_ERROR;
			break;
		    }
		}
	    }

	    /* If no recovery needed, don't attempt DMF open */
	    if ( !(rdb->rdb_status & RDB_RECOVER) )
		break;

	    /*
	    ** Build a DCB for the database in this process.
	    ** This opens the database config file and initializes the
	    ** Database Control Block.
	    */
	    MEmove(8, "$default", ' ', sizeof(location.loc_name), 
		    (PTR)&location.loc_name);
	    location.loc_l_extent = rdb->rdb_l_root;
	    MEmove(location.loc_l_extent, (PTR)&rdb->rdb_root, ' ', 
		sizeof(location.loc_extent), location.loc_extent);

	    flags = DM2D_RECOVER | DM2D_CANEXIST;
	    if (rdb->rdb_status & RDB_JOURNAL)
		flags |= DM2D_JOURNAL;
	    if (rdb->rdb_status & RDB_FAST_COMMIT)
		flags |= DM2D_FASTCOMMIT;

	    /*
	    ** Offline recoveries can take advantate of fastcommit and 
	    ** single cache efficiencies.(e.g. reduce unnecessary forces).
	    */
	    if (rcp->rcp_action == RCP_R_STARTUP)
		flags |= DM2D_BMSINGLE | DM2D_FASTCOMMIT;

	    /*
	    ** Online recoveries need to respect DMCM if this is a cluster.
	    */
	    if ((rcp->rcp_action == RCP_R_ONLINE || 
	         rcp->rcp_action == RCP_R_CSP_ONLINE) &&
		 rcp->rcp_cluster_enabled )
	    {
		flags |= DM2D_DMCM;
	    }
	    dm2mode = DM2D_A_WRITE;
	    status = dm2d_add_db(flags, &dm2mode, 
		     &rdb->rdb_name, (DB_OWN_NAME *)&rdb->rdb_owner, 1, 
		     (DM2D_LOC_ENTRY *)&location, &rdb->rdb_dcb, 0L, &rcp->rcp_dberr);
	    if (status != E_DB_OK)
		break;

	    /* 
	    ** Assign the process database reference id of the recovery
	    ** process to the dcb_log_id.
	    */
	    rdb->rdb_dcb->dcb_log_id = rcp_db_id;

	    /*
	    ** Open the database for this RDB.
	    **
	    ** The open is not logged, nor is the open communicated to the
	    ** logging system here (via LGadd).  If the database open needs
	    ** to be communicated to the logging system (as during RCP
	    ** startup recovery), it is done separately above.
	    **
	    ** During online recovery, we must pass the DM2D_ONLINE_RCP 
	    ** flag to enable special locking and cache access modes required
	    ** when we are doing recovery concurrent with database access.
	    */
	    flags = (DM2D_RECOVER | DM2D_NLG);

	    if ((rcp->rcp_action != RCP_R_STARTUP) &&
		(rcp->rcp_action != RCP_R_CSP_STARTUP))
	    {
		flags |= DM2D_ONLINE_RCP;
	    }

	    /*
	    ** Decide whether or not to request the database lock.
	    **
	    ** It is not currently really necessary to ever request the
	    ** lock since during online recovery the database is still
	    ** locked by the process which we are recovering and in offline
	    ** recovery the database is offline.
	    **
	    ** Furthermore, we cannot request an exclusive database lock
	    ** in all cases during offline recovery since willing commit
	    ** transactions must be able to reacquire the database locks
	    ** they held when they entered the secure state.
	    **
	    ** So currently we never request a real database lock here
	    ** even though we will in many cases treat the database as
	    ** locked exclusively during recovery processing.
	    */
	    flags |= DM2D_NLK;

	    /*
	    ** Pick the database lock mode.  Note that this decision is made
	    ** independent from the decision above of whether to actually
	    ** request a database lock.  This lock mode will be stored in
	    ** the DCB even if the DM2D_NLK flag is specified.  If an exclusive
	    ** lock is specified, then the database will be treated as though
	    ** it is opened exclusively.
	    **
	    ** We choose and exclusive lock mode during offline recovery and
	    ** in online recovery cases when full database REDO is required.
	    ** In these modes the database is kept offline during recovery so
	    ** we know we have exclusive access to it.
	    **
	    ** During online recovery where access to the database is allowed
	    ** during recovery, a shared lock mode is specified.
	    **
	    ** Note that during CSP startup recovery, we may actually need to
	    ** open the database on multiple nodes.
	    */

	    lock_mode = DM2D_IX;
	    if ((rcp->rcp_action == RCP_R_STARTUP) ||
		(rcp->rcp_action == RCP_R_CSP_STARTUP) ||
		(rdb->rdb_status & RDB_REDO) ||
		(rdb->rdb_status & RDB_XSVDB_LOCK))
	    {
		lock_mode = DM2D_X;
	    }

	    status = dm2d_open_db(rdb->rdb_dcb, DM2D_A_WRITE, lock_mode, 
		rcp->rcp_lock_list, flags, &rcp->rcp_dberr);
	    if (status != E_DB_OK)
		break;

	    break;
	}

	/*
	** If an error occurs processing a database, that database will
	** not be recovered, but we continue processing so the rest of
	** the databases can be recovered.
	*/
	if (status != E_DB_OK)
	{
	    uleFormat(NULL, E_DM9431_RCP_OPENDB_ERROR, &sys_err, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &error, 1,
		sizeof(rdb->rdb_name), &rdb->rdb_name);
	    mark_invalid(rcp, rdb, DSC_OPENDB_INCONSIST);
	    CLRDBERR(&rcp->rcp_dberr);
	    status = E_DB_OK;
	}

    }


    /*
    ** Prepare each transaction for recovery.
    **
    ** During online recovery, we must assume ownership of the lock lists
    ** owned by the Undo transactions.  This allows us to safely request
    ** page locks during recovery without blocking (since any needed logical
    ** locks should already be held) and lets us release the locks when
    ** recovery is complete.
    ** 
    ** We also release all physical page locks held by the transactions
    ** previous to starting recovery processing.  This ensures that we
    ** don't hang ourselves during recovery by waiting on a short term
    ** catalog lock that is held by one of the transactions being recovered.
    **
    ** During offline recovery, all locks are requested on the RCP internal
    ** lock list.
    */
    for (st = rcp->rcp_tx_state.stq_next;
	st != (RCP_QUEUE *)&rcp->rcp_tx_state.stq_next;
	st = tx->rtx_state.stq_next)
    {
	tx = (RTX *)((char *)st - CL_OFFSETOF(RTX, rtx_state));
	rdb = tx->rtx_rdb;

	/*
	** During Offline Recovery, re-establish the transaction context
	** in the logging system so the transaction can be backed out.
	** Note that we do this even for inconsistent databases. 
	*/
	if (rcp->rcp_action != RCP_R_ONLINE)
	{
	    /*
	    ** Begin a transaction to re-establish the recover xact
	    ** context.  After creating a transaction, alter it to reflect 
	    ** the same state as the original xact:
	    **
	    **     - transaction id
	    **     - log_id handle
	    **     - first and last log records written by the transaction
	    **
	    ** Pass the NORESERVE flag since transactions restored in offline
	    ** recovery will be writing CLR records but will have no logspace
	    ** reserved to them.
	    */
	    flags = LG_NORESERVE;
	    flags |= ((rdb->rdb_status & RDB_JOURNAL) ? LG_JOURNAL : 0);

	    cl_status = LGbegin(flags, rdb->rdb_dbopen_id, &tran_id,&tx->rtx_id,
		sizeof(DB_OWN_NAME), tx->rtx_user_name.db_own_name, 
		(DB_DIS_TRAN_ID*)NULL, &sys_err);
	    if (cl_status != OK)
	    {
		uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
		uleFormat(NULL, E_DM900C_BAD_LOG_BEGIN, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, &error, 1,
		    0, rdb->rdb_dbopen_id);
		SETDBERR(&rcp->rcp_dberr, 0, E_DM9409_RCP_P1);
		status = E_DB_ERROR;
		break;
	    }

	    ctx.lg_ctx_id = tx->rtx_id;
	    STRUCT_ASSIGN_MACRO(tx->rtx_tran_id, ctx.lg_ctx_eid);
	    STRUCT_ASSIGN_MACRO(tx->rtx_last_lga, ctx.lg_ctx_last);
	    STRUCT_ASSIGN_MACRO(tx->rtx_first_lga, ctx.lg_ctx_first);
	    STRUCT_ASSIGN_MACRO(tx->rtx_cp_lga, ctx.lg_ctx_cp);
 
	    cl_status = LGalter(LG_A_CONTEXT, (PTR)&ctx, sizeof(ctx), &sys_err);
	    if (cl_status != OK)
	    {
		uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
		uleFormat(NULL, E_DM900B_BAD_LOG_ALTER, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, &error, 1,
		    0, LG_A_CONTEXT);
		SETDBERR(&rcp->rcp_dberr, 0, E_DM9409_RCP_P1);
		status = E_DB_ERROR;
		break;
	    }
	}

	/*
	** If Online Recovery, assume ownership of the transaction and release
	** all of its physical page locks. Note that the transaction lock list
	** has already been adopted by the rcp in the dmr_preinit_recovery
	** routine.
	*/
	if (rcp->rcp_action == RCP_R_ONLINE)
	{
	    tr.tr_pr_id = dmf_svcb->svcb_lctx_ptr->lctx_lgid;
	    tr.tr_id = tx->rtx_id;
	    CSget_sid(&tr.tr_sess_id);

	    cl_status = LGalter(LG_A_LXB_OWNER, (PTR)&tr, sizeof(tr), &sys_err);
	    if (cl_status != OK)
	    {
		uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
			    (char *)NULL, 0L, (i4 *)NULL, &error, 0);
		uleFormat(NULL, E_DM900B_BAD_LOG_ALTER, &sys_err, ULE_LOG, 
		    NULL, (char *)NULL, 0L, (i4 *)NULL, &error, 1,
		    0, LG_A_LXB_OWNER);
		SETDBERR(&rcp->rcp_dberr, 0, E_DM9409_RCP_P1);
		break;
	    }

	    /*
	    ** Release any short term physical locks held by this transaction.
	    */
	    cl_status = LKrelease(LK_AL_PHYS, tx->rtx_ll_id, (LK_LKID *)0, 
				(LK_LOCK_KEY *)0, (LK_VALUE *)0, &sys_err);
	    if (cl_status != OK)
	    {
		uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
		uleFormat(NULL, E_DM901B_BAD_LOCK_RELEASE, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, &error, 1,
		    0, tx->rtx_ll_id);
		SETDBERR(&rcp->rcp_dberr, 0, E_DM9409_RCP_P1);
		status = E_DB_ERROR;
		break;
	    }
	}
	else if (rcp->rcp_action == RCP_R_CSP_ONLINE)
	{
	    cl_status = LKcreate_list(
			    (LK_NONPROTECT | LK_MASTER | LK_MASTER_CSP),
			    (LK_LLID)0,
			    (LK_UNIQUE *) &tx->rtx_tran_id, 
			    (LK_LLID *)&tx->rtx_ll_id, 
			    0, &sys_err);
	    if (cl_status != OK)
	    {
		uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
		uleFormat(NULL, E_DM901A_BAD_LOCK_CREATE, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, 0L, (i4 *)NULL, &error, 0);
		SETDBERR(&rcp->rcp_dberr, 0, E_DM9409_RCP_P1);
		status = E_DB_ERROR;
		break;
	    }

	    TRdisplay("\tTransaction %x%x now has lock list %x\n",
		tx->rtx_tran_id.db_high_tran, tx->rtx_tran_id.db_low_tran,
		tx->rtx_ll_id);
	}
	else
	{
	    tx->rtx_ll_id = rcp->rcp_lock_list;
	}

	/*
	** Store original last lga for the transaction before doing any
	** transaction recovery that may update the value in rtx_last_lga.
	*/
	STRUCT_ASSIGN_MACRO(tx->rtx_last_lga, tx->rtx_orig_last_lga);

	/*
	** If an error occurs establishing the transaction context, we
	** fail here to allow Offline recovery to process the databases.
	** Errors in the lock context are more likely lgk errors than
	** database problems.
	*/
	if (status != E_DB_OK)
	{
	    uleFormat(NULL, E_DM9432_RCP_OPENTX_ERROR, &sys_err, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &error, 3,
		sizeof(tx->rtx_rdb->rdb_name), &tx->rtx_rdb->rdb_name,
		0, tx->rtx_tran_id.db_high_tran, 
		0, tx->rtx_tran_id.db_low_tran);
	    SETDBERR(&rcp->rcp_dberr, 0, E_DM9409_RCP_P1);
	    return (E_DB_ERROR);
	}
    }

    /*
    ** Handle any outstanding OPENDB events that we caused through
    ** LGadd calls above.  This is necessary so that the databases
    ** added to the logging system will be useable during undo recovery.
    ** Note that this call may also process open requests by other servers.
    */

    /* 
    ** We need to poke the LG subsystem to process events. It doesn't 
    ** matter which lx_id we use as long as its legal. Therefore we
    ** just use the 0th lx_id which is valid in both CSP and RCP
    */

    dmr_check_event(rcp, LG_NOWAIT, rcp->rcp_lctx[0]->lctx_lxid);

    return (E_DB_OK);
}

/*{
** Name: dmr_complete_recovery - clean up after recovery action
**
** Description:
**	This routine is called at the completion of recovery processing, 
**	following the redo and undo passes.
**
**	It reads through the list of databases and transactions we are about
**	to recover and does any post-processing necessary:
**
**	During Offline Recovery:
**	    Force and toss all pages from the buffer manager.
**	    Log the Abort records for all rolled back transactions.
**	    Mark the config file closed for all recovered databases.
**	    Close all databases opened for recovery and inform the Logging
**		system of the close action.
**
**	During Online Recovery:
**	    Force and toss all pages from the buffer manager.
**	    Log the Abort records for all rolled back transactions.
**	    Inform the Logging System of the completion of these transactions.
**	    Release all transaction lock lists.
**	    Close all databases opened for recovery and inform the Logging
**		system of the completion of database recovery.
**
**	If recovery failed for any databases, the config file is updated
**	to show the inconsistency and the Logging System is informed of the
**	database state.
**
** Inputs:
**	rcp			Recovery Context
**
** Outputs:
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      14-dec-1992 (rogerk)
**	    Reduced Logging Project: Written.
**	15-mar-1993 (rogerk)
**	    Reduced Logging - Phase IV:
**	    Changed the RCP to track databases via the external database id
**		rather than an internal LG id.  RDB list is now hashed by
**		the external database id.
**	    Removed dm0l_closedb call in favor of direct LGremove.
**	26-apr-1993 (bryanp)
**	    6.5 Cluster support:
**		Added node_id parameter.
**		CSP only needs to call this once.
**		If incons_error is found then we dump all logs.
**		Adjusted some rcp_action tests to accomodate cluster needes.
**	26-jul-1993 (bryanp)
**	    Re-enable logging of ET log records while we figure out how to
**	    handle inconsistent databases properly.
**	26-jul-1993 (rogerk)
**	  - Put back logging of ET records for those transactions which were
**	    rolled back.  This was accidentally commented out in the last
**	    integration.
**	  - Added trace point DM1312 to bypass logdump when recovery fails.
**	23-aug-1993 (rogerk)
**	  - Took out direct updating of the database config file to mark
**	    recovery processing complete when the database is journaled.
**	    We cannot turn off the config file open mask until ALL work
**	    is finished, including archiving.
**	  - Folded willing-commit failure flag into the general recovery
**	    failure status so we only have to keep track of one status.
**	18-oct-1993 (jnash & rogerk)
**	    Changed offline recovery to not actually request a database lock
**	    because this prevented willing commit transaction recovery from
**	    reacquiring their database locks.
**	    Changed handling of Willing Commit transactions to set them into
**	    reassoc_wait state at the end of recovery.  This means we can also
**	    do the LGremove at the end of recovery even if there are willing
**	    commit transactions outstanding.
**	    Removed the LG_RCP flag to LGend, the control blocks belonging
**	    to failed servers are now always cleaned up in the LG_A_DBRECOVER
**	    call.
**	15-apr-1994 (chiku)
**	    Bug56702: do not conclude transactions being recovered on dead
**	              logfull situation.
**		      do not release locks if online recovery encountered 
**		      dead logfull.
**		      purge journal entries for already inconsistent databases.
**		      return error if dead logfull was detected during online
**		      recovery.
*/
DB_STATUS
dmr_complete_recovery(
RCP		*rcp,
i4		num_nodes)
{
    DB_TAB_TIMESTAMP    ctime;
    RCP_QUEUE		*st;
    RDB			*db;
    RTX			*tx;
    i4		flags;
    i4		et_flags;
    i4		et_state;
    i4		lk_action;
    i4			i;
    CL_ERR_DESC		sys_err;
    DB_STATUS		status = E_DB_OK;
    STATUS		cl_status;
    bool		incons_error = FALSE;
    bool		incons_pc = FALSE;
    i4			error;

    CLRDBERR(&rcp->rcp_dberr);

    TRdisplay("%@ RCP-P3: Begin Recovery Cleanup Phase.\n");

    /*
    ** Consistency Checks:
    **
    **     Verify that each transaction was correctly recovered.
    */
    for (st = rcp->rcp_tx_state.stq_next;
	st != (RCP_QUEUE *)&rcp->rcp_tx_state.stq_next;
	st = tx->rtx_state.stq_next)
    {
	tx = (RTX *)((char *)st - CL_OFFSETOF(RTX, rtx_state));

	if (( ! (tx->rtx_status & RTX_COMPLETE)) &&
	    (tx->rtx_rdb->rdb_status & RDB_RECOVER))
	{
	    uleFormat(NULL, E_DM9433_RCP_INCOMP_TX_ERROR, &sys_err, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &error, 3,
		sizeof(tx->rtx_rdb->rdb_name), &tx->rtx_rdb->rdb_name,
		0, tx->rtx_tran_id.db_high_tran, 
		0, tx->rtx_tran_id.db_low_tran);
	    mark_invalid(rcp, tx->rtx_rdb, DSC_RECOVER_INCONST);
	}
    }


    for (;;)
    {
	/*
	** Call the buffer manager to write out the pages updated during
	** recovery.  The pages must be tossed since the RCP does not
	** guarantee that changes to cached pages will be recognized via
	** cache locks once recovery is complete.
	** DM0P_TOSS_IGNORE So we write what we can out to disk
	*/
	status = dm0p_force_pages(rcp->rcp_lock_list, &rcp_tran_id,
			    (i4)0, DM0P_TOSS_IGNORE, &rcp->rcp_dberr);
	if (status != E_DB_OK)
	{
	    recover_force_pages(rcp);

	    /* Try DM0P_TOSS_IGNORE again */
	    status = dm0p_force_pages(rcp->rcp_lock_list, &rcp_tran_id,
			    (i4)0, DM0P_TOSS_IGNORE, &rcp->rcp_dberr);
	    if (status)
		break;
	}

	/*
	** Post process each transaction
	**
	**     - Log the End Transaction records for recovered transactions.
	**       If recovery failed, log inconsistent state.
	**
	**     - Inform Logging System that the transaction is complete (online
	**       recovery only).
	**
	**     - Release the transaction's lock list (online recovery only).
	**
	** Willing Commit transactions that were restored are left active,
	** but are set in orphaned reassoc_wait state until they can be
	** adopted by the coordinator for resolution.
	*/
	for (st = rcp->rcp_tx_state.stq_next;
	    st != (RCP_QUEUE *)&rcp->rcp_tx_state.stq_next;
	    st = tx->rtx_state.stq_next)
	{
	    tx = (RTX *)((char *)st - CL_OFFSETOF(RTX, rtx_state));

	    /*
	     * Bug56702: if dead logfull, do not attempt to conclude
	     *		 the status of transactions.
	     */
	    if ( rcp->rcp_state != RCP_LG_GONE ) {
	    	/*
	     	** Willing commit transactions are set into orphaned reassoc_wait
	    	** state.  No further post processing is required of these
	    	** transactions.
	    	*/
	    	if (tx->rtx_recover_type == RTX_WILLING_COMMIT)
	    	{
			cl_status = LGalter(LG_A_REASSOC, (PTR) &tx->rtx_id, 
		    	sizeof(tx->rtx_id), &sys_err);
			if (cl_status != OK)
			{
		    	 uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
				(char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
		   	 uleFormat(NULL, E_DM900B_BAD_LOG_ALTER, &sys_err, ULE_LOG, NULL,
				(char *)NULL, (i4)0, (i4 *)NULL, &error, 1, 
				0, LG_A_REASSOC);
			 SETDBERR(&rcp->rcp_dberr, 0, E_DM9434_DMR_COMPLETE_ERROR);
		   	 status = E_DB_ERROR;
		   	 break;
			}
			continue;
	    	}

	    	/*
	    	** If recovery failed on this database and the DBA has defined
	    	** that we should not mark databases inconsistent, then we skip
	    	** post-processing on this database.  The RCP will fail at the
	    	** end of this recovery cycle.
	    	*/
	    	if ((tx->rtx_rdb->rdb_status & RDB_INVALID) && 
			( ! (tx->rtx_rdb->rdb_status & RDB_ALREADY_INCONSISTENT)) &&
			( ! (tx->rtx_status & RTX_TRAN_COMPLETED)) &&
	    		( Db_recovery_error_action == RCP_STOP_ON_INCONS_DB ))
	    	{
			incons_error = TRUE;
			continue;
	    	}

	    	/*
	    	** If the transaction was successfully aborted then log the
	    	** end transaction log record.
	    	**
	    	** If recovery failed on this database and this transaction
	    	** was left open, then mark it inconsistent.
	    	**
	    	** This step is skipped on transactions which were bypassed
	    	** in dmr_init_recovery due to the database being already
	    	** inconsistent (when rtx_id is 0).  In this case it is assumed
	    	** that any recovery post-processing necessary was done at the
	    	** time that the database was actually marked inconsistent.
	    	*/
	    	if (tx->rtx_id &&
			((tx->rtx_status & RTX_ABORT_FINISHED) ||
		     	((tx->rtx_rdb->rdb_status & RDB_INVALID) && 
				( ! (tx->rtx_status & RTX_TRAN_COMPLETED)))))
	    	{
			et_flags = DM0L_MASTER;
			if ( (tx->rtx_rdb->rdb_status & RDB_JOURNAL) &&
                             (tx->rtx_status & RTX_JNL_TX) )
			    et_flags |= DM0L_JOURNAL;

			/*
			** Mark the transaction state as an aborted transaction.  If
			** the database is being marked inconsistent, then list the
			** transaction completion state as INCONSISTENT.  Rollforward
			** understands this class of ET log record and will ignore it,
			** treating the transaction as incomplete so that it can be
			** correctly rolled back at the end of the rollforward phase.
			*/
			et_state = DM0L_ABORT;
			if (tx->rtx_rdb->rdb_status & RDB_INVALID)
			    et_state = DM0L_DBINCONST;

			status = dm0l_et(tx->rtx_id, et_flags, et_state, 
			    &tx->rtx_tran_id, tx->rtx_rdb->rdb_ext_dbid,
			    &ctime, &rcp->rcp_dberr);
			if (status != E_DB_OK)
			    break;


			TRdisplay("\tEnd Transaction Record written for xact %x%x\n",
			    tx->rtx_tran_id.db_high_tran, tx->rtx_tran_id.db_low_tran);

	    	}
	    }


	    /*
	    ** Remove the Completed transaction from the logging system.
	    */
	    if (tx->rtx_id)
	    {
		cl_status = LGend(tx->rtx_id, (i4)0, &sys_err);
		if (cl_status != OK)
		{
		    uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
			(char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
		    uleFormat(NULL, E_DM900E_BAD_LOG_END, &sys_err, ULE_LOG, NULL,
			(char *)NULL, (i4)0, (i4 *)NULL, &error, 1, 
			0, tx->rtx_id);
		    SETDBERR(&rcp->rcp_dberr, 0, E_DM9434_DMR_COMPLETE_ERROR);
		    status = E_DB_ERROR;
		    break;
		}
	    }
            /*
	     * Bug56702: if dead logfull during online recovery, do not 
	     * 		 release locks.
	     */
	    if ( (rcp->rcp_action == RCP_R_ONLINE) && (rcp->rcp_state == RCP_LG_GONE) )
	        continue;

	    /*
	    ** If this transaction had a different lock list than the
	    ** default RCP one, then release it.
	    */
	    if (tx->rtx_ll_id && (tx->rtx_ll_id != rcp->rcp_lock_list))
	    {
		lk_action = LK_ALL;
		if ((rcp->rcp_action == RCP_R_ONLINE) && 
		    ( ! (tx->rtx_status & RTX_PASS_ABORT)))
		{
		    lk_action |= LK_RELATED;
		}

		cl_status = LKrelease(lk_action, tx->rtx_ll_id, (LK_LKID *)0, 
				    (LK_LOCK_KEY *)0, (LK_VALUE *)0, &sys_err);
		if (cl_status != OK)
		{
		    uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
			(char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
		    uleFormat(NULL, E_DM901B_BAD_LOCK_RELEASE, &sys_err, ULE_LOG, 
			NULL, (char *)NULL, (i4)0, (i4 *)NULL, &error, 1, 
			0, tx->rtx_ll_id);
		    SETDBERR(&rcp->rcp_dberr, 0, E_DM9434_DMR_COMPLETE_ERROR);
		    status = E_DB_ERROR;
		    break;
		}
	    }
	}

	if (status != E_DB_OK)
	    break;

	/*
	** Post process each database
	**
	**     - Inform Logging System that the recovery is complete (online
	**       recovery only).
	**
	**     - Update the config file to show that recovery is complete on
	**       the database (offline recovery only).
	*/
	for (st = rcp->rcp_db_state.stq_next;
	    st != (RCP_QUEUE *)&rcp->rcp_db_state.stq_next;
	    st = db->rdb_state.stq_next)
	{
	    db = (RDB *)((char *)st - CL_OFFSETOF(RDB, rdb_state));

	    /*
	    ** Skip databases on which we never attempted to do recovery.
	    */ 
	    if ( ! (db->rdb_status & (RDB_RECOVER | RDB_INVALID)))
		continue;

	    /*
	    ** Skip inconsistent database which were already inconsistent
	    ** when we started recovery processing.
	    */
	    /*
	     * Bug56702: need to purge journal entries for
	     * 		 already inconsistent databases.
	     *
	    if (db->rdb_status & RDB_ALREADY_INCONSISTENT)
		continue;
	     *
	     */

	    /*
	    ** If table recovery error, mark table(s) inconsistent
	    ** Then mark the database inconsistent -
	    */
	    if (db->rdb_status & RDB_TABLE_RECOVERY_ERROR)
	    {
		/* Mark invalid tables inconsistent */
		(void) mark_tbl_incons(rcp, db);
		CLRDBERR(&rcp->rcp_dberr);
	    }

	    /*
	    ** Check for willing commit failures on the database.
	    **
	    ** The default handling for a failure in an attempt to restore
	    ** a willing commit transaction is to abort the transaction,
	    ** allow recovery to complete, and then mark the database
	    ** inconsistent.
	    **
	    ** If the willing commit failure flag is set, then fold this
	    ** status now into the general recovery failure flag.
	    */
	    if (db->rdb_status & RDB_WILLING_FAIL)
		db->rdb_status |= RDB_INVALID;

	    /*
	    ** Skip inconsistent databases if the DBA has defined that
	    ** we should not mark databases inconsistent. The RCP will 
	    ** fail at the end of this recovery cycle.
	    */
	    if ( (db->rdb_status & RDB_INVALID) &&
               /* 
		* Bug56702: do not cause crash for already
		*	    inconsistent databases.
		*/
	       !(db->rdb_status & RDB_ALREADY_INCONSISTENT) )
	    {
		incons_error = TRUE;
		if (Db_recovery_error_action == RCP_STOP_ON_INCONS_DB)
		{
		    TRdisplay("\tDatabase %~t NOT RECOVERED!\n", 
			sizeof(DB_DB_NAME), &db->rdb_name);
		    continue;
		}
	    }

	    /*
	    ** During OFFLINE recovery, we have to update the config file
	    ** to turn off the open count now that the database has been
	    ** made consistent (in online recovery, this happens through
	    ** the normal process of the database being closed following
	    ** recovery completion).
	    **
	    ** If recovery failed on the database, we update the config
	    ** file to show that it is inconsistent.
	    **
	    ** If recovery succeeded, but there is still an open Willing
	    ** Commit transaction, then we do not update the config file
	    ** since the database is still open.
	    **
	    ** If there is journal work to process, then we can't turn
	    ** off the open count yet, we have to wait until the database
	    ** is removed from the logging system during the normal database
	    ** close procedure.  NOTE THAT THIS DOES NOT CURRENTLY WORK
	    ** FOR CLUSTER HANDLING.
	    */
	    if (((rcp->rcp_action != RCP_R_ONLINE) && 
			( ! (db->rdb_status & RDB_WILLING_COMMIT)) &&
			( ! (db->rdb_status & RDB_JOURNAL))) ||
		(db->rdb_status & RDB_INVALID) ||
		(db->rdb_status & RDB_TABLE_RECOVERY_ERROR))
	    {
		status = close_config(rcp, db);
		if (status != E_DB_OK)
		{
		    /*
		    ** Failure to update the config file is logged, but does
		    ** not halt recovery processing.  The open status of the
		    ** config file should cause it to be marked inconsistent
		    ** the next time it can be successfully opened.
		    */
		    uleFormat(&rcp->rcp_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
			(char *)NULL, 0L, (i4 *)NULL, &error, 1,
			sizeof(DB_DB_NAME), &db->rdb_name);
		    status = E_DB_OK;
		    CLRDBERR(&rcp->rcp_dberr);
		}
	    }

	    /*
	    ** If recovery failed on the database, inform the logging
	    ** system that we just marked the database inconsistent.
	    */
	    if ((rcp->rcp_action == RCP_R_ONLINE) &&
		(db->rdb_status & (RDB_INVALID | RDB_TABLE_RECOVERY_ERROR)))
	    {
		cl_status = LGalter(LG_A_DB, (PTR) &db->rdb_lg_dbid, 
				    sizeof(db->rdb_lg_dbid), &sys_err);
		if (cl_status != OK)
		{
		    uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
				(char *)NULL, 0L, (i4 *)NULL, &error, 0);
		    uleFormat(NULL, E_DM900B_BAD_LOG_ALTER, &sys_err, ULE_LOG, 
			NULL, (char *)NULL, 0L, (i4 *)NULL, &error, 1,
			0, LG_A_DB);
		    SETDBERR(&rcp->rcp_dberr, 0, E_DM9434_DMR_COMPLETE_ERROR);
		    break;
		}
	    }

	    /*
	    ** Close and delete the database context for this process.
	    */
	    if (db->rdb_dcb)
	    {
		flags = DM2D_NLG;

		/*
		** See note in dmr_init_recovery concerning db open mode.
		*/
		flags |= DM2D_NLK;

		/*
		** Close the database if it was successfully opened.
		*/
		if (db->rdb_dcb->dcb_ref_count)
		{
		    status = dm2d_close_db(db->rdb_dcb, rcp->rcp_lock_list, 
			flags, &rcp->rcp_dberr);
		    if (status != E_DB_OK)
			break;
		}

		status = dm2d_del_db(db->rdb_dcb, &rcp->rcp_dberr);
		if (status != E_DB_OK)
		    break;

		db->rdb_dcb = (DMP_DCB *)0;
	    }

	    /*
	    ** During OFFLINE recovery, we have to inform the logging
	    ** system that we are closing the database since we did
	    ** a dm0l_opendb at the start of recovery.
	    */
	    if ((rcp->rcp_action != RCP_R_ONLINE) && 
		(db->rdb_dbopen_id != 0))
	    {
		cl_status = LGremove(db->rdb_dbopen_id, &sys_err);
		if (cl_status != OK)
		{
		    uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
			(char *)NULL, 0L, (i4 *)NULL, &error, 0);
		    uleFormat(NULL, E_DM9016_BAD_LOG_REMOVE, &sys_err, ULE_LOG, 
			NULL, (char *)NULL, 0L, (i4 *)NULL, &error, 1,
			0, db->rdb_dbopen_id);
		    SETDBERR(&rcp->rcp_dberr, 0, E_DM9434_DMR_COMPLETE_ERROR);
		    break;
		}
	    }

	    /*
	    ** If online recovery, inform the logging system that the
	    ** database has been fully recovered.
	    */
	    if (rcp->rcp_action == RCP_R_ONLINE)
	    {
		cl_status = LGalter(LG_A_DBRECOVER, (PTR) &db->rdb_lg_dbid, 
				    sizeof(db->rdb_lg_dbid), &sys_err);
		if (cl_status != OK)
		{
		    uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
				(char *)NULL, 0L, (i4 *)NULL, &error, 0);
		    uleFormat(NULL, E_DM900B_BAD_LOG_ALTER, &sys_err, ULE_LOG, 
			NULL, (char *)NULL, 0L, (i4 *)NULL, &error, 1,
			0, LG_A_DBRECOVER);
		    SETDBERR(&rcp->rcp_dberr, 0, E_DM9434_DMR_COMPLETE_ERROR);
		    break;
		}
	    }

	    if ( ! (db->rdb_status & RDB_INVALID))
	    {
		TRdisplay("\tDatabase %~t successfully recovered\n", 
		    sizeof(DB_DB_NAME), &db->rdb_name);
		if (db->rdb_status & RDB_TABLE_RECOVERY_ERROR)
		{
		    TRdisplay("However, since recovery failed for some index(s), database %~t has been marked inconsistent\n",
		    sizeof(DB_DB_NAME), &db->rdb_name);
		}

	    }
	}

	if (status != E_DB_OK)
	    break;

	/*
	** Call the buffer manager again to make sure that ALL pages are
	** purged out of the buffer manager.  Previously, we tossed all
	** pages owned by the rcp transaction - that should have tossed
	** all pages.  This call makes sure there were not any pages left
	** around not owned and also tosses any pages read in during closedb
	** processing.
	*/
	dm0p_toss_pages(0, 0, rcp->rcp_lock_list, (i4)0, 1);

	/*
	** Destroy the RCP lock list.
	*/
	cl_status = LKrelease(LK_ALL, rcp->rcp_lock_list, (LK_LKID *)0, 
		(LK_LOCK_KEY *)0, (LK_VALUE *)0, &sys_err);
	if (cl_status != OK)
	{
	    uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	    uleFormat(NULL, E_DM901B_BAD_LOCK_RELEASE, &sys_err, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, &error, 1,
		0, rcp->rcp_lock_list);
	    SETDBERR(&rcp->rcp_dberr, 0, E_DM9434_DMR_COMPLETE_ERROR);
	    status = E_DB_ERROR;
	    break;
	}

	if ( incons_error || incons_pc )
	{
	    /*
	    ** Dump all log file records since the last consistency point
	    ** to the trace output to assist in debugging.
	    */
	    if ( incons_error || 
		Db_recovery_error_action == RCP_STOP_ON_INCONS_DB )
	    {
		for (i = 0; i < num_nodes; i++)
		{
		    if ( ! DMZ_ASY_MACRO(12))
			dmr_dump_log(rcp, i);
		}
	    }
	    /*
	    ** If the DBA has specified to crash rather than mark databases
	    ** inconsistent then return an error which should cause the
	    ** RCP to fail.
	    */
	    if (Db_recovery_error_action == RCP_STOP_ON_INCONS_DB)
	    {
		SETDBERR(&rcp->rcp_dberr, 0, E_DM942A_RCP_DBINCON_FAIL);
		break;
	    }
	}

	/*
	** Handle any outstanding CLOSEDB events that we caused through
	** LGremove calls above. 
	** Note that this call may also process open requests by other servers.
	*/

	/* 
	** We need to poke the LG subsystem to process events. It doesn't 
	** matter which lx_id we use as long as its legal. Therefore we
	** just use the 0th lx_id which is valid in both CSP and RCP
	*/

	dmr_check_event(rcp, LG_NOWAIT, rcp->rcp_lctx[0]->lctx_lxid);

	TRdisplay("%@ Recovery Operations Completed.\n");

	/*
	 * Bug56702: return error status so that RCP
	 *	     would bring the installation down
	 *	     during online recovery.
	 */
	if ( rcp->rcp_state == RCP_LG_GONE )
		return(E_DB_ERROR);
	else
		return (E_DB_OK);
    }

    uleFormat(&rcp->rcp_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
	(char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
    SETDBERR(&rcp->rcp_dberr, 0, E_DM9434_DMR_COMPLETE_ERROR);

    return (E_DB_ERROR);
}

/*{
** Name: dmr_analysis_pass - Analysis Pass of recovery processing
**
** Description:
**	This function performs the Analysis Pass of recovery processing.
**
**	It reads once through the log file from the last Consistency Point
**	up to the current End-of-File and gather information needed during
**	the subsequent Redo and Undo phases.
**
**	  - During Offline recovery, the recovery context is built during
**	    this analysis pass.  Opendb, BT and ET log records are tracked
**	    so that at the end of the analysis pass we have a list of all 
**	    (potentially) open databases and transactions.
**
**	  - The analysis pass also looks for operations which are listed as
**	    NONREDO.  These are updates which modify an entire table to such
**	    an extent that previous updates to the table can no longer be
**	    redone (modify and destroy are examples).  When such an operation
**	    is found it is added into a list of nonredo operations to be used
**	    in the redo pass.
**
**	  - Prepare to Commit records are recognized and transactions are
**	    listed as in Willing Commit state so that they are not aborted
**	    in the undo phase.
**
**	  - When ECP or SETBOF records are found, the BOF value therein is
**	    saved so the log file Begin-of-File mark can be updated.
**
**	Closedb records are not tracked and any database which is listed
**	as open at any time during the analysis pass is included in the
**	database list.  Any databases which are actually not open are
**	discarded in the dmr_init_recovery routine.
**
**	It is also possible for transactions to be included which were
**	actually completed if the transaction was ended just previous to 
**	the last CP.  These edge cases are discarded during the undo pass
**	when the ET record is encountered.
**
**	In a multi-node logging system such as the VAXCluster, this routine
**	is called once for each node which is being recovered. The node_id
**	parameter identifies the particular node to use. Each analysis pass
**	accumulates its information into an overall list of the databases and
**	transactions that were active at the time of the crash.
**
** Inputs:
**	rcp			Recovery Context
**	node_id			Indicates which particular node is to be
**				analyzed in a multi-node logging system. In a
**				single-node logging system, node_id should be 0.
**
** Outputs:
**	err_code		Reason for error return.
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      14-dec-1992 (rogerk)
**	    Reduced Logging Project: Written.
**	15-mar-1993 (rogerk)
**	    Reduced Logging - Phase IV:
**	    Clarified use of LSN vs. Log Adress.  Log Reads return Log
**		Addresses.  The LSN of a record can be extracted from the
**		log record header.
**	    Changed the log record format to include the database and
**		transaction ids rather than having them in the lctx_header.
**	    Changed the RCP to track databases via the external database id
**		rather than an internal LG id.
**	    Removed checks for reused database id's since its now impossible.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**		Add node_id parameter.
**		Pass correct transaction handle to dm0l_read.
**		Enable DM1307 tracepoint to echo LSN's during recovery passes.
**		Pass start_recover_lsn argument to add_rdb.
**		Pass node_id argument to lookup_rdb.
**	26-jul-1993 (bryanp)
**	    We no longer log closedb log records, so don't look for them.
**	26-jul-1993 (rogerk)
**	  - Fix setting of xact last-log-address value when P1 record found.
**	    Use 'la' value instead of garbage 'lga'.
**	  - Changed some RCP trace outputs to try to make it more readable.
**	23-aug-1993 (jnash)
**	    Add iimonitor "show session" status display.  
**	18-oct-1993 (jnash)
**	    Write iircp.log 'percent done' info at 30 second intervals.  This 
**	    eliminates flooding the error log with messages when recovering 
**	    large log intervals.
**	18-oct-1993 (rogerk)
**	    Add check for database which was open without Fast Commit and then
**	    later opened with fast commit.  In this case we must enable full
**	    redo.  In cases where database use is mixed (fast commit and not)
**	    then we check redo on all transactions.
**	    Made updating of BOF from setbof and ecp log records conditional
**	    upon it moving the bof forward.  We should never move the log
**	    file bof backward.
**	    Added ownername to rtx structure and add_rtx call so that xacts
**	    can be restored with their original owner.
**	31-jan-1994 (rogerk)
**	    Fixed handling of transactions which have written an ET record
**	    but which have not been removed from the Logging System.
**	    During the analysis pass, if an ET record is found for an xact
**	    that was part of our online recovery context then we cannot remove
**	    it from the open transaction list.  Doing this causes us to not
**	    know to call LGend for the xact in dmr_complete_recovery.  The
**	    rtx must be left around, but marked TRAN_COMPLETED.
**	23-may-1994 (jnash)
**	    When the last log of the log file is read during offline
**	    recoveries, call LGalter(LG_A_LFLSN) (set last forced lsn) to 
**	    tell the logging system the LSN of the last record.  This fixes 
**	    a problem where value was not set, and should work on standalone 
**	    and clustered installations.
**      28-may-1998 (stial01)
**          dmr_analysis_pass() Added NONREDO processing for DM0LSM1RENAME
**	15-oct-98 (stephenb)
**	    Keep track of current highest TX_ID in the LCTX, we need to write it
**	    back to the log header on restart recovery so that we never re-use 
**	    transaction IDs after crash recovery (it screws up replicator).
**	28-Mar-2005 (jenjo02)
**	    lctx_node_id is meaningful; node_id is not.
**	9-Apr-2005 (schka24)
**	    Don't recognize anything other than a LAST P1 as an indicator
**	    of a valid willing-commit transaction.
**	    Integrate inifa01's fix for 111693 INGREP 153: only track the
**	    transaction ID if it's higher!
*/
DB_STATUS
dmr_analysis_pass(
RCP		*rcp,
i4		node_id)
{
    RDMU		*rdmu;
    RTX			*rtx;
    RDB			*rdb;
    DMP_LCTX		*lctx = rcp->rcp_lctx[node_id];
    LG_RECORD		*record_header;
    DM0L_HEADER		*record;
    DM0L_OPENDB		*open;
    DM0L_BT		*btran;
    DM0L_SETBOF		*setbof;
    DM0L_ECP		*ecp;
    DM0L_P1		*p1rec;
    DB_TRAN_ID		*tran_id;
    DB_TAB_ID		tabid;
    LG_LA		la;
    LG_LA		prev_la;
    LG_LSN		lsn;
    i4		dbid;
    i4		recover_flags;
    i4		records_processed = 0;
    DB_STATUS		status;
    char		buffer[80];
    SCF_CB 		scf_cb;
    i4	  	pc_done;
    i4	  	cur_time;
    i4	  	tr_lasttime = TMsecs();
    CL_ERR_DESC		sys_err;
    LG_LFLSN_ALTER	lsn_alter;
    char		nodequalbuf[22];
    i4			error;

    CLRDBERR(&rcp->rcp_dberr);

    if ( lctx->lctx_node_id )
	STprintf(nodequalbuf," for node %d",lctx->lctx_node_id);
    else
	nodequalbuf[0] = '\0';

    TRdisplay("%@ RCP-P1: Start Analysis Pass%s.\n", nodequalbuf);
    TRdisplay("\tAnalysis Window:  CP <%d,%d,%d> to EOF <%d,%d,%d>.\n",
	    lctx->lctx_cp.la_sequence,
	    lctx->lctx_cp.la_block,
	    lctx->lctx_cp.la_offset,
	    lctx->lctx_eof.la_sequence,
	    lctx->lctx_eof.la_block,
	    lctx->lctx_eof.la_offset);

    scf_cb.scf_length = sizeof(scf_cb);
    scf_cb.scf_type = SCF_CB_TYPE;
    scf_cb.scf_ascii_id = SCF_ASCII_ID;
    scf_cb.scf_facility = DB_DMF_ID;
    scf_cb.scf_session = DB_NOSESSION;
    scf_cb.scf_ptr_union.scf_buffer = (PTR) buffer;
    scf_cb.scf_nbr_union.scf_atype = SCS_A_MINOR;

    if (rcp->rcp_action == RCP_R_ONLINE || rcp->rcp_action == RCP_R_CSP_ONLINE )
    {
	STprintf(buffer, 
	    "RCP Analysis Pass:%d logs processed (approx %d%%)", 0,0);
	scf_cb.scf_len_union.scf_blength = STlength(buffer);
	(VOID) scf_call(SCS_SET_ACTIVITY, &scf_cb);
    }

    /*
    ** Initialize start of scan.
    */
    STRUCT_ASSIGN_MACRO(lctx->lctx_cp, prev_la);

    /*
    ** Position the Log File to the last Consistency Point.
    */
    status = dm0l_position(DM0L_FLGA, lctx->lctx_lxid,
			    &lctx->lctx_cp, lctx, &rcp->rcp_dberr);

    while (status == E_DB_OK)
    {
	/*
	** Read forward until the end of the log file looking for interesting
	** information:
	**
	**	- Open, Close database log records
	**	- Begin, End Transaction log records
	**	- Secure Operations.
	**	- Setbof and ECP log records
	**	- Non-Redo operations
	**
	** The following are interesting special cases:
	**
	**      - duplicate DBID because of closedb just before CP
	*/

	/*
	** If we are executing Online Recovery from the RCP then check
	** periodically for RCP background work to do like processing
	** open database requests.
	*/
	records_processed++;
	if ((rcp->rcp_action == RCP_R_ONLINE) && 
	    ((records_processed % 300) == 0))
	{
	    dmr_check_event(rcp, LG_NOWAIT, 0);
	}

	/*
	** Check for periodic status updates.
	*/
	if ((records_processed % 500) == 0)
	{
	    pc_done = percent_done(lctx, &la);

	    STprintf(buffer,
		"RCP Analysis Pass:%d logs processed (approx %d%%)",
		records_processed, pc_done);

	    /*
	    ** Write note to iircp.log, but not too often.
	    */
	    cur_time = TMsecs();
	    if (cur_time >= (tr_lasttime + TRACE_INCR))
	    {
		TRdisplay("%@ %s\n", buffer);
		tr_lasttime = cur_time;
	    }

	    /*
	    ** If online recovery, update IIMONITOR comment line with
	    ** information about REDO progress.
	    */
	    if (rcp->rcp_action == RCP_R_ONLINE || rcp->rcp_action == RCP_R_CSP_ONLINE )
	    {
		scf_cb.scf_len_union.scf_blength = STlength(buffer);
		(VOID) scf_call(SCS_SET_ACTIVITY, &scf_cb);
	    }
	}

	/*
	** If we have read up to the point where the Log File EOF was when
	** we started recovery processing then we can stop with the analysis
	** pass - we have seen everything there is to see.
	*/
	if (LGA_GTE(&prev_la, &lctx->lctx_eof))
	{
	    if (rcp->rcp_action != RCP_R_ONLINE)
	    {
		/*
		** If offline recovery, tell the logging system the lsn 
		** the last record in the file.
		*/
		lsn_alter.lg_lflsn_lg_id = lctx->lctx_lgid;
	  	lsn_alter.lg_lflsn_lsn = record->lsn;

		status = LGalter(LG_A_LFLSN, (PTR)&lsn_alter,
		    sizeof(LG_LFLSN_ALTER), &sys_err);
		if (status)
		{
		    /* Errors not fatal */
		    uleFormat(NULL, status, (CL_ERR_DESC *)&sys_err, ULE_LOG, NULL,
			(char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
		    uleFormat(NULL, E_DM900B_BAD_LOG_ALTER, (CL_ERR_DESC *)&sys_err,
			 ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL, 
			&error, 1, 0, LG_A_LFLSN);
		    status = E_DB_OK;
		}
	    }
	    break;
	}

	status = dm0l_read(lctx, lctx->lctx_lxid, &record, &la,
				(i4*)NULL, &rcp->rcp_dberr);
	if (status != E_DB_OK)
	{
	    /*
	    ** Reaching EOF here is an expected way to terminate the
	    ** analysis pass.
	    */
	    if (rcp->rcp_dberr.err_code == E_DM9238_LOG_READ_EOF)
	    {
		/*
		** If offline recovery, tell the logging system the lsn 
		** the last record in the file.
		*/
		if (rcp->rcp_action != RCP_R_ONLINE)
		{
		    lsn_alter.lg_lflsn_lg_id = lctx->lctx_lgid;
		    lsn_alter.lg_lflsn_lsn = record->lsn;

		    status = LGalter(LG_A_LFLSN, (PTR)&lsn_alter,
			sizeof(LG_LFLSN_ALTER), &sys_err);
		    if (status)
		    {
			uleFormat(NULL, status, (CL_ERR_DESC *)&sys_err, ULE_LOG, 
			    NULL, (char *)NULL, (i4)0, (i4 *)NULL, 
			    &error, 0);
			uleFormat(NULL, E_DM900B_BAD_LOG_ALTER, 
			    (CL_ERR_DESC *)&sys_err, ULE_LOG, 
			    NULL, (char *)NULL, (i4)0, (i4 *)NULL, 
			    &error, 1, 0, LG_A_LFLSN);
		    }
		}

		status = E_DB_OK;
		CLRDBERR(&rcp->rcp_dberr);
		break;
	    }

	    break;
	}

	record_header = (LG_RECORD *)lctx->lctx_header;
	dbid = record->database_id;
	tran_id = &record->tran_id;
	/* If this tran ID really higher than our highest, track highest */
	if (tran_id->db_high_tran > lctx->lctx_lg_header.lgh_tran_id.db_high_tran
	  || (tran_id->db_high_tran == lctx->lctx_lg_header.lgh_tran_id.db_high_tran
	      && tran_id->db_low_tran > lctx->lctx_lg_header.lgh_tran_id.db_low_tran))
	{
	    lctx->lctx_lg_header.lgh_tran_id.db_high_tran = tran_id->db_high_tran;
	    lctx->lctx_lg_header.lgh_tran_id.db_low_tran = tran_id->db_low_tran;
	}
	STRUCT_ASSIGN_MACRO(la, prev_la);

#ifdef xDEBUG
	if (DMZ_ASY_MACRO(7))
	    TRdisplay("\tAnalysis LSN: (%x,%x)\n",
			record->lsn.lsn_high, record->lsn.lsn_low);
#endif

	/*
	** Process based on the record type.
	*/
	switch (record->type)
	{
	case DM0LBT:

	    /*
	    ** If this is an Online Recovery and we have already computed the
	    ** recovery context, then we do not record new transactions here.
	    */
	    if (rcp->rcp_action == RCP_R_ONLINE)
		break;

	    /*
	    ** Lookup the transaction to see if it is already in the recover
	    ** list.  It is possible that the transaction was listed in the
	    ** last CP and so we have already built the xact context.
	    */
	    lookup_rtx(rcp, tran_id, &rtx);
	    if (rtx)
		break;

	    /*
	    ** If this database is being recovered, then record the start of
	    ** this new transaction.  If the ET is not found then this
	    ** transaction will be rolled back.
	    */
	    lookup_rdb(rcp, dbid, lctx->lctx_node_id, &rdb);
	    if (rdb)
	    {
		btran = (DM0L_BT *) record;
		if (rcp->rcp_verbose)
		{
		    TRdisplay("\t<%d,%d,%d>\tBegin Transaction: %x%x DB %x\n",
			la.la_sequence,
			la.la_block,
			la.la_offset,
			tran_id->db_high_tran, tran_id->db_low_tran, dbid);
		}

		status = add_rtx(rcp, lctx, tran_id, (LK_LLID)0, 0, &la, &la, 
		    &lctx->lctx_cp, rdb, btran->bt_name.db_own_name);
		if (status != E_DB_OK)
		    break;
	    }

	    break;

	case DM0LET:

	    /*
	    ** End Transaction Log Record was found so this transaction
	    ** has completed.
	    **
	    ** If the transaction is on the open transaction list then
	    ** then it needs to be marked complete and the undo/redo
	    ** counts updated.
	    */
	    lookup_rtx(rcp, tran_id, &rtx);
	    if (rtx)
	    {
		if (rcp->rcp_verbose)
		{
		    TRdisplay("\t<%d,%d,%d>\tEnd Transaction:   %x%x DB %x\n",
			la.la_sequence,
			la.la_block,
			la.la_offset,
			tran_id->db_high_tran, tran_id->db_low_tran, dbid);
		}

		rtx->rtx_status |= RTX_TRAN_COMPLETED;

		rcp->rcp_tx_count--;
		rtx->rtx_rdb->rdb_bt_cnt--;

		/*
		** If the transaction was in willing commit state, then remove 
		** that state and decrement the willing commit count.
		*/
		if (rtx->rtx_recover_type == RTX_WILLING_COMMIT)
		{
		    rtx->rtx_recover_type = RTX_REDO_UNDO;
		    rcp->rcp_willing_commit--;
		}
	    }

	    /*
	    ** If the database is undergoing REDO then increment the redo
	    ** counts.
	    */
	    lookup_rdb(rcp, dbid, lctx->lctx_node_id, &rdb);
	    if (rdb && (rdb->rdb_status & RDB_REDO))
	    {
		rcp->rcp_rdtx_count++;
		rdb->rdb_et_cnt++;
	    }

	    /*
	    ** In online recovery the transaction, if it was part of our
	    ** original recovery context, needs to be preserved so that we will
	    ** remove the transaction from the Logging System at the end of
	    ** recovery processing. This situation occurs if the commit/abort
	    ** log record had been written but a server crash occurred before
	    ** the LGend call was made.
	    **
	    ** In non-online recovery, the transaction context can be removed.
	    */
	    if (rtx && (rcp->rcp_action != RCP_R_ONLINE))
		remove_rtx(rtx);

	    break;

	case DM0LOPENDB:

	    open = (DM0L_OPENDB *) record;
	    if (rcp->rcp_verbose)
	    {
		TRdisplay("\t<%d,%d,%d>\tOpen  Database: ID: %x, %~t\n",
		    la.la_sequence,
		    la.la_block,
		    la.la_offset,
		    dbid, sizeof(DB_DB_NAME), &open->o_dbname);
	    }

	    /*
	    ** If this is an Online Recovery and we have already computed the
	    ** recovery context, then we do not record new databases here.
	    */
	    if (rcp->rcp_action == RCP_R_ONLINE)
		break;

	    /*
	    ** If we have already added this database to the RDB list, then
	    ** we can just skip it.
	    */
	    lookup_rdb(rcp, dbid, lctx->lctx_node_id, &rdb);
	    if (rdb)
	    {
		/*
		** If the current rdb specifies that only partial redo is
		** required (the database was not open with fast commit),
		** but this invocation of the database uses fast commit,
		** then upgrade the recovery type to include full REDO
		** processing.
		*/
		if ((rdb->rdb_status & RDB_REDO_PARTIAL) &&
		    (open->o_header.flags & DM0L_FASTCOMMIT))
		{
		    TRdisplay("\t    Update Db ID: %x to full REDO.\n", dbid);
		    rdb->rdb_status |= RDB_REDO;
		    rdb->rdb_status &= ~RDB_REDO_PARTIAL;
		}

		rdb->rdb_refcnt++;
		break;
	    }

	    /*
	    ** Add this new instance of the Database to our db context.
	    **
	    ** Pass log address of the Open Database log record as the
	    ** starting point for which REDO processing is required.  Any
	    ** updates logged previous to this point must have been forced
	    ** to the database when it was last closed.
	    */
	    recover_flags = RDB_RECOVER;
	    if (open->o_header.flags & DM0L_JOURNAL)
		recover_flags |= RDB_JOURNAL;

	    /*
	    ** Fast Commit db's need full redo, non fast commit need only
	    ** perform redo on open transactions.
	    */
	    if (open->o_header.flags & DM0L_FASTCOMMIT)
		recover_flags |= RDB_REDO;
	    else
		recover_flags |= RDB_REDO_PARTIAL;

	    if (open->o_header.flags & DM0L_NOTDB)
		recover_flags = RDB_NOTDB;

	    if (rcp->rcp_verbose)
	    {
		TRdisplay("\t    Recovery Flags: %v\n",
		    "RECOVER,JOURNAL,REDO,,NOTDB,FAST_COMMIT,INVALID", 
		    recover_flags);
	    }

	    status = add_rdb(rcp, lctx, dbid, &open->o_dbname, 
			&open->o_dbowner, &open->o_root, open->o_l_root, 
			0, &la, &record->lsn, recover_flags);
	    if (status != E_DB_OK)
		break;

	    break;

	case DM0LP1:
	    /*
	    ** If this is an Online Recovery and we have already computed the
	    ** recovery context, then we do not record new transactions here.
	    */
	    if (rcp->rcp_action == RCP_R_ONLINE)
		break;

	    /*
	    ** If (and only if) this P1 is the LAST P1, the transaction
	    ** has been put into Prepare to Commit state.  It must be
	    ** recovered and its transaction state reconstructed, 
	    ** but not aborted.
	    **
	    ** Reset its recover type to Willing Commit.  This means that
	    ** no undo will be required.  Set the address of the Secure log 
	    ** record in the rtx's last log record address field so it can 
	    ** be quickly looked up later.
	    **
	    ** We ignore non-LAST P1's, except for common actions.
	    */
	    lookup_rtx(rcp, tran_id, &rtx);
	    p1rec = (DM0L_P1 *) record;
	    if (p1rec->p1_flag == P1_LAST
	      && rtx && (rtx->rtx_recover_type != RTX_WILLING_COMMIT))
	    {
		if (rcp->rcp_verbose)
		{
		    TRdisplay("\t<%d,%d,%d>\tPrepare to Commit: Tranid: %x%x\n",
			la.la_sequence,
			la.la_block,
			la.la_offset,
			tran_id->db_high_tran, tran_id->db_low_tran);
		}

		rtx->rtx_recover_type = RTX_WILLING_COMMIT;
		rcp->rcp_willing_commit++;

		if (rtx->rtx_rdb->rdb_status & RDB_REDO)
		    rcp->rcp_rdtx_count++;
	    }

	    /*
	    ** Set transaction state as having seen a log record.
	    */
	    if (rtx)
	    {
		STRUCT_ASSIGN_MACRO(la, rtx->rtx_last_lga);
		rtx->rtx_status &= ~RTX_FOUND_ONLY_IN_CP;
	    }

	    break;

	case DM0LSETBOF:
	    /*
	    ** Record the new BOF address in the rcp.  LG will later be reset
	    ** to the Hightest valued BOF address that we encountered.
	    */
	    setbof = (DM0L_SETBOF *) record;
	    TRdisplay("\t<%d,%d,%d>\tSet BOF Record: New BOF: <%d,%d,%d>\n",
		la.la_sequence,
		la.la_block,
		la.la_offset,
		setbof->sb_newbof.la_sequence,
		setbof->sb_newbof.la_block,
		setbof->sb_newbof.la_offset);

	    if (LGA_GT(&setbof->sb_newbof, &lctx->lctx_bof))
		STRUCT_ASSIGN_MACRO(setbof->sb_newbof, lctx->lctx_bof);
	    break;

	case DM0LECP:
	    /*
	    ** Same as above SETBOF - extract the BOF from the End Consistency
	    ** Point log record.
	    */
	    ecp = (DM0L_ECP *) record;
	    TRdisplay("\t<%d,%d,%d>\tEnd CP  Record: New BOF: <%d,%d,%d>\n",
		la.la_sequence,
		la.la_block,
		la.la_offset,
		ecp->ecp_bof.la_sequence,
		ecp->ecp_bof.la_block,
		ecp->ecp_bof.la_offset);

	    if (LGA_GT(&ecp->ecp_bof, &lctx->lctx_bof))
		STRUCT_ASSIGN_MACRO(ecp->ecp_bof, lctx->lctx_bof);
	    break;
	
	default:

	    /*
	    ** If the recovery context is being computed in the analysis
	    ** pass (offline recovery), and this record belongs to a
	    ** transaction we may have to recover, then set its LSN in the
	    ** xact context to keep track of its highest written log record
	    ** address.  This tells us where to start abort processing should
	    ** we find that this transaction needs to be rolled back.
	    */
	    if (rcp->rcp_action == RCP_R_ONLINE)
		break;

	    lookup_rtx(rcp, tran_id, &rtx);
	    if (rtx)
	    {
		STRUCT_ASSIGN_MACRO(la, rtx->rtx_last_lga);
		MEcopy((char *)&record->lsn, sizeof(record->lsn), 
		    (char *)&rtx->rtx_last_lsn);
		rtx->rtx_status &= ~RTX_FOUND_ONLY_IN_CP;
	    }
	    break;
	}

	/*
	** If an error occurred during the above processing, then break.
	*/
	if (status != E_DB_OK)
	    break;

	/*
	** Look for Non-Redo log records.  These identify operations which
	** make it impossible to attempt REDO of any actions logged prior
	** to the Non-Redo operation.
	**
	** When a Non-Redo log record is found, its LSN is remembered for the
	** Redo Pass.
	*/
	if (record->flags & DM0L_NONREDO)
	{
	    lookup_rdb(rcp, dbid, lctx->lctx_node_id, &rdb);
	    if (rdb)
	    {
		MEcopy((char *)&record->lsn, sizeof(record->lsn), (char *)&lsn);
		dmve_get_tab(record, &tabid, NULL, NULL);
		if (rcp->rcp_verbose)
		{
		    TRdisplay("\t<%d,%d,%d>\tNon-Redo operation on DB: %x.\n",
			la.la_sequence,
			la.la_block,
			la.la_offset);
		    TRdisplay("\t    Table (%d,%d) LSN (%x,%x) operation %d.\n",
			tabid.db_tab_base, tabid.db_tab_index, 
			lsn.lsn_high, lsn.lsn_low, record->type);
		}

		/*
		** Add the Non-Redo information.
		** If there has already been a non-redo operation on this table
		** then update the already existing rdmu object.
		*/
		lookup_nonredo(rcp, dbid, &tabid, &rdmu);
		if (rdmu == 0)
		{
		    status = add_nonredo(rcp, dbid, &tabid, &lsn);
		    if (status)
			break;

		    /* for DM0LSM1RENAME, there are two tables */
		    if (record->type == DM0LSM1RENAME)
		    {
			DM0L_SM1_RENAME     *sm1_rec;

			sm1_rec = (DM0L_SM1_RENAME *) record;
			status = add_nonredo(rcp, dbid, &sm1_rec->sm1_tmp_id,
				&lsn);
		    }
		}
		else
		{
		    STRUCT_ASSIGN_MACRO(lsn, rdmu->lsn);
		}
	    }
	}
    }

    (VOID) scf_call(SCS_CLR_ACTIVITY, &scf_cb);

    if (status == E_DB_OK)
    {
        TRdisplay("%@ RCP-P1: Analysis Pass Complete%s.\n",nodequalbuf);

	/* FIX ME if we can't archive should we continue? */
	status = rcp_archive(rcp, lctx);

	return (E_DB_OK);
    }


    /*
    ** Error Handling:
    */
    uleFormat(&rcp->rcp_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
	(char *)NULL, (i4)0, (i4 *)NULL, &error, 0);

    SETDBERR(&rcp->rcp_dberr, 0, E_DM9435_DMR_ANALYSIS_PASS);
    return (E_DB_ERROR);
}

/*{
** Name: dmr_redo_pass - Redo Pass of recovery processing
**
** Description:
**	This routine implements the Redo pass of recovery processing.
**
**	It reads forward through the log file from the last Consistency Point
**	to the current End-of-File and applies REDO recovery for each log
**	record found that satisfies the following conditions:
**
**	    - The database is being recovered.
**
**	    - The log record comes after the spot to which we must begin
**            redo processing for the particular database.
**
**	    - Either the database was open with Fast Commit and needs full
**	      redo processing OR the log record's transaction is explicitely
**	      being recovered (will be undone).
**
**	    - There was no nonredo operation logged later in the log file
**	      for the table specified in the log record.
**
**	The redo pass will periodically call dmr_check_event to see if there
**	are outstanding opendb requests.
**
**	In a multi-node logging system, such as the VAXCluster, this routine
**	perform the redo pass for a single node. It should be called multiple
**	times, once for each node which is being recovered. The node_id
**	argument distinguishes the particular nodes.
**
** Inputs:
**	rcp			Recovery Context
**	node_id			node identification number
**
** Outputs:
**	err_code		Reason for error return.
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      14-dec-1992 (rogerk)
**	    Reduced Logging Project: Written.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**		Pass correct transaction handle to dm0l_read.
**		Enable trace point DM1307 to echo LSNs during recovery passes.
**		Pass node_id argument to lookup_rdb.
**	20-jul-1993 (jnash)
**	    If offline recovery, issue periodic messages noting that we are
**	    making progress.
**	26-jul-1993 (rogerk)
**	  - Fix references to unused 'lga' variable in error messages.
**	  - Add optimization to skip redo pass when there are no databases
**	    or transactions to recover.
**	  - Created routine 'trace_record' to consolidate calls which output
**	    log records and their header info to the trace file.  Added tracing
**	    of records which are skipped in REDO processing due to NonRedo
**	    operations.
**	23-aug-1993 (jnash)
**	  - Change E_DM943D_RCP_DBREDO_ERROR error to pass lsn rather than
**	    log address, as error message indicates.
**	  - Add iimonitor "show session" status display.  
**	18-oct-1993 (jnash)
**	    Write iircp.log 'percent done' info at 30 second intervals.  This 
**	    eliminates flooding the error log with messages when recovering 
**	    large log intervals.
**	28-Mar-2005 (jenjo02)
**	    lctx_node_id is meaningful; node_id is not.
*/
DB_STATUS
dmr_redo_pass(
RCP		*rcp,
i4		node_id)
{
    DMP_LCTX		*lctx = rcp->rcp_lctx[node_id];
    RDB			*rdb;
    RTX			*rtx;
    RDMU		*rdmu;
    RTBL		*rtbl;
    RCP_QUEUE		*st;
    LG_RECORD		*record_header;
    DM0L_HEADER		*record;
    DB_TRAN_ID		*tran_id;
    DB_TAB_ID		tabid;
    LG_LA		la;
    LG_LA		prev_la;
    i4		dbid;
    i4		lock_list;
    i4		records_processed = 0;
    i4		pc_done;
    i4			redo_dbs;
    i4			recover_dbs;
    DB_STATUS		status;
    LG_LSN		lsn;
    char		buffer[80];
    SCF_CB 		scf_cb;
    i4	  	cur_time;
    i4	  	tr_lasttime = TMsecs();
    char		nodequalbuf[22];
    i4			error;
    DB_STATUS		local_status;
    i4			local_error;

    CLRDBERR(&rcp->rcp_dberr);


    if ( lctx->lctx_node_id )
	STprintf(nodequalbuf," for node %d",lctx->lctx_node_id);
    else
	nodequalbuf[0] = '\0';

    /*
    ** If there are no databases needing REDO, then skip
    ** REDO processing.
    */

    /*
    ** Consistency check:  we want to make sure that we do not accidentally
    ** skip required recovery processing so we double check that the count
    ** above is accurate.  This reduces the impact of a bug in counting
    ** recoverable databases.
    **
    ** Check the database and transaction list for any recovery entries.
    ** If there are any, give a warning and continue with the redo pass.
    */
    recover_dbs = 0;
    redo_dbs = 0;

    for (st = rcp->rcp_db_state.stq_next;
	st != (RCP_QUEUE *)&rcp->rcp_db_state.stq_next;
	st = rdb->rdb_state.stq_next)
    {
	rdb = (RDB *)((char *)st - CL_OFFSETOF(RDB, rdb_state));
	if (rdb->rdb_status & RDB_RECOVER)
	{
	    recover_dbs++;
	    if ( rdb->rdb_status & (RDB_REDO | RDB_REDO_PARTIAL) )
		redo_dbs++;
	}
    }

    if ( rcp->rcp_db_count != recover_dbs &&
	 (rcp->rcp_db_count = recover_dbs) )
	TRdisplay("Warning: Redo Pass - recoverdb count mismatch.\n");

    /* Why read through the log if no REDO needed? */
    if ( redo_dbs == 0 )
    {
	TRdisplay("%@ RCP-P2: Redo Pass - No recovery needed%s.\n",
		   nodequalbuf);
	return (E_DB_OK);
    }

    TRdisplay("%@ RCP-P2: Start Redo Pass%s.\n", nodequalbuf);
    TRdisplay("\tRedo Window:  CP <%d,%d,%d> to EOF <%d,%d,%d>.\n",
	    lctx->lctx_cp.la_sequence,
	    lctx->lctx_cp.la_block,
	    lctx->lctx_cp.la_offset,
	    lctx->lctx_eof.la_sequence,
	    lctx->lctx_eof.la_block,
	    lctx->lctx_eof.la_offset);
    if (rcp->rcp_verbose)
	TRdisplay("%78*=\n");

    scf_cb.scf_length = sizeof(scf_cb);
    scf_cb.scf_type = SCF_CB_TYPE;
    scf_cb.scf_ascii_id = SCF_ASCII_ID;
    scf_cb.scf_facility = DB_DMF_ID;
    scf_cb.scf_session = DB_NOSESSION;
    scf_cb.scf_ptr_union.scf_buffer = (PTR) buffer;
    scf_cb.scf_nbr_union.scf_atype = SCS_A_MINOR;

    if ( rcp->rcp_action == RCP_R_ONLINE || rcp->rcp_action == RCP_R_CSP_ONLINE )
    {
	STprintf(buffer, 
	    "RCP Redo Pass: %d logs processed (approx %d%%)", 0, 0);
	scf_cb.scf_len_union.scf_blength = STlength(buffer);
	(VOID) scf_call(SCS_SET_ACTIVITY, &scf_cb);
    }

    /*
    ** Initialize start of scan.
    */
    STRUCT_ASSIGN_MACRO(lctx->lctx_cp, prev_la);

    /*
    ** Position the Log File to the last Consistency Point.
    */
    status = dm0l_position(DM0L_FLGA, lctx->lctx_lxid,
			    &lctx->lctx_cp, lctx, &rcp->rcp_dberr);

    while (status == E_DB_OK)
    {
	/*
	** Position the Log File to the last Consistency Point.
	**
	** Read forward until the end of the log file and apply redo
	** to each log record which:
	**
	**	- belongs to a database undergoing full REDO recovery OR
	**	- belongs to a transaction being undone
	**
	** If the logged table has been modified by a Non-Redo operation
	** subsequent to this log record (discovered in the analysis pass)
	** then no redo is attempted.
	*/

	/*
	** If we are executing Online Recovery from the RCP then check
	** periodically for RCP background work to do like processing
	** open database requests.
	*/
	records_processed++;
	if ((rcp->rcp_action == RCP_R_ONLINE) && 
	    ((records_processed % 200) == 0))
	{
	    dmr_check_event(rcp, LG_NOWAIT, lctx->lctx_lxid);
	}

	/*
	** Check for periodic status updates.
	*/
	if ((records_processed % 500) == 0)
	{
	    pc_done = percent_done(lctx, &la);

	    STprintf(buffer,
		"RCP Redo Pass: %d logs processed (approx %d%%)",
		records_processed, pc_done);

	    /*
	    ** Write note to iircp.log, but not too often.
	    */
	    cur_time = TMsecs();
	    if (cur_time >= (tr_lasttime + TRACE_INCR))
	    {
		TRdisplay("%@ %s\n", buffer);
		tr_lasttime = cur_time;
	    }

	    /*
	    ** If online recovery, update IIMONITOR comment line with
	    ** information about REDO progress.
	    */
	    if (rcp->rcp_action == RCP_R_ONLINE || rcp->rcp_action == RCP_R_CSP_ONLINE )
	    {
		scf_cb.scf_len_union.scf_blength = STlength(buffer);
		(VOID) scf_call(SCS_SET_ACTIVITY, &scf_cb);
	    }
	}

	/*
	** If we have read up to the point where the Log File EOF was when
	** we started recovery processing then we can stop with the analysis
	** pass - we have seen everything there is to see.
	*/
	if (LGA_GTE(&prev_la, &lctx->lctx_eof))
	    break;

	status = dm0l_read(lctx, lctx->lctx_lxid, &record, &la,
				(i4*)NULL, &rcp->rcp_dberr);
	if (status != E_DB_OK)
	{
	    /*
	    ** Reaching EOF here is the expected way to terminate the
	    ** analysis pass.
	    */
	    if (rcp->rcp_dberr.err_code == E_DM9238_LOG_READ_EOF)
	    {
		status = E_DB_OK;
		CLRDBERR(&rcp->rcp_dberr);
		break;
	    }

	    break;
	}

	STRUCT_ASSIGN_MACRO(la, prev_la);
	record_header = (LG_RECORD *)lctx->lctx_header;
	dbid = record->database_id;
	MEcopy((char *)&record->lsn, sizeof(record->lsn), (char *)&lsn);
	tran_id = &record->tran_id;
	lock_list = rcp->rcp_lock_list;

#ifdef xDEBUG
	if (DMZ_ASY_MACRO(7))
	    TRdisplay("\tRedo LSN: (%x,%x)\n",
			record->lsn.lsn_high, record->lsn.lsn_low);
#endif

	/*
	** If the database or described by this log record is not being being
	** recovered then skip it.
	*/
	lookup_rdb(rcp, dbid, lctx->lctx_node_id, &rdb);
	if ((rdb == NULL) || ( ! (rdb->rdb_status & RDB_RECOVER)))
	    continue;

	/*
	** Skip records which fall previous to the spot at which we must
	** begin redo processing for this database (a database may not need
	** redo from the start of the last CP if it was not opened until
	** later in the log file).
	*/
	if (LGA_LT(&la, &rdb->rdb_first_lga))
	{
	    continue;
	}

	/*
	** If REDO is not needed on this particular transaction, then skip it.
	** The database may require REDO only of a subset of transactions:
	**
	**     - If it is a Non Fast Commit database, then only uncommitted
	**       transactions must be redone.
	**
	**     - Pass Abort and Willing Commit recovery requires no REDO
	**       processing.
	*/
	if ((rdb->rdb_status & (RDB_REDO | RDB_REDO_PARTIAL)) == 0)
	    continue;

	if ( rdb->rdb_status & RDB_REDO_PARTIAL )
	{
	    lookup_rtx(rcp, tran_id, &rtx);

	    if ((rtx == NULL) || (rtx->rtx_recover_type != RTX_REDO_UNDO))
		continue;

	    /*
	    ** During REDO of a particular transaction, use that xact's lock
	    ** list for lock requests.  Otherwise use the generic rcp one.
	    */
	    lock_list = rtx->rtx_ll_id;
	}

	/* Trace the record if we're verbose */
	if (rcp->rcp_verbose)
	    trace_record(record_header, record, &la, lctx->lctx_bksz);

	/*
	** If a Non-Redo operation has been performed on this table subsequent
	** to this log record, then we cannot redo this operation.
	*/
	dmve_get_tab(record, &tabid, NULL, NULL);
	lookup_nonredo(rcp, dbid, &tabid, &rdmu);
	if (rdmu && (LSN_GTE(&rdmu->lsn, &lsn)))
	{
	    if (rcp->rcp_verbose)
		TRdisplay("\tRECORD SKIPPED due to NonRedo operation.\n");
	    continue;
	}

	/* Skip log record for tables marked invalid */
	rtbl = NULL;
	if (rdb->rdb_status & RDB_TABLE_RECOVERY_ERROR)
	{
	   lookup_invalid_tbl(rdb, &tabid, record, &rtbl);
	   if (rtbl &&
		(rtbl->rtbl_flag & RTBL_IGNORE_TABLE) ||
			rtbl_ignore_page(rtbl, record) == TRUE)
	   {
#ifdef xDEBUG
		    dmd_log(FALSE, (PTR) record, lctx->lctx_bksz);
		    TRdisplay("\tREDO SKIPPED due to INVALID status.\n");
#endif
		    rtbl->redo_cnt++;
		    continue;
	    }
	}

	/*
	** Apply Redo Recovery for this log record.
	*/
	status = apply_redo(rcp, record, &la,
			    rdb->rdb_dcb, lock_list);

#ifdef xDEBUG
	/* DEBUG: if an index, force an error! */
	if (tabid.db_tab_index > 100 && tabid.db_tab_index % 5 == 0)
	{
	    SETDBERR(&rcp->rcp_dberr, 0, E_DM9638_DMVE_REDO);
	    status = E_DB_ERROR;
	}
#endif

	if (status != E_DB_OK)
	{
	    /*
	    ** If an error occurs during REDO processing, we mark the RDB
	    ** for that database as invalid and try to keep on processing
	    ** other databases that need recovery.
	    */
	    uleFormat(&rcp->rcp_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &error, 0);

	    /* If possible, mark table or index invalid for redo/undo */
	    status = add_invalid_tbl(rcp, rdb, rtbl, lctx, record);
	    if (status != E_DB_OK)
	    {
		uleFormat(NULL, E_DM943D_RCP_DBREDO_ERROR, (CL_ERR_DESC *)NULL, ULE_LOG, 
		    NULL, (char *)NULL, (i4)0, (i4 *)NULL, &error, 3,
		    sizeof(rdb->rdb_name), &rdb->rdb_name,
		    0, record->lsn.lsn_high, 0, record->lsn.lsn_low);
		mark_invalid(rcp, rdb, DSC_REDO_INCONSIST);
	    }
	    dmd_log(FALSE, (PTR) record, lctx->lctx_bksz);
	    TRdisplay("\tREDO error.\n");
	    status = E_DB_OK;
	    CLRDBERR(&rcp->rcp_dberr);
	}
    }

    if (rcp->rcp_verbose)
	TRdisplay("%78*=\n");

    (VOID) scf_call(SCS_CLR_ACTIVITY, &scf_cb);

    if (status == E_DB_OK)
    {
	if ( rcp->rcp_action == RCP_R_CSP_ONLINE ||
	     rcp->rcp_action == RCP_R_ONLINE )
	{
	    /*
	    ** Call the buffer manager to write out any pages updated during
	    ** redo.  The pages must be tossed since during online REDO
	    ** we may not have taken SV_PAGE locks and any UNDOs that follow
	    ** must cache lock if need be.
	    */
	    status = dm0p_force_pages(rcp->rcp_lock_list, &rcp_tran_id,
				(i4)0, DM0P_TOSS, &rcp->rcp_dberr);
	}
	if (status == E_DB_OK)
	{
	    TRdisplay("%@ RCP-P2: Redo Pass Complete%s.\n", nodequalbuf);
	    return (E_DB_OK);
	}
    }

    /*
    ** Error Handling:
    */
    uleFormat(&rcp->rcp_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
	(char *)NULL, (i4)0, (i4 *)NULL, &error, 0);

    SETDBERR(&rcp->rcp_dberr, 0, E_DM940A_RCP_P2);
    return (E_DB_ERROR);
}

/*{
** Name: dmr_undo_pass - Undo Pass of recovery processing
**
** Description:
**	This routine implements the Undo pass of recovery processing.
**
**	It reads backward through the log file from the EOF until all
**	outstanding transactions have been resolved.
**
**	It uses the rcp_tx_state queue as the list of transactions needing
**	recovery.  Each transacaction on this list must be resolved through
**	one of the following ways:
**
**	    - encountering an ET record: meaning that the transaction
**	      had actually already completed.
**
**	    - encountering a Prepare to Commit record and successfully
**	      re-establishing the transaction contexted.
**
**	    - rolling back the transaction until the BT record is found.
**
**	The undo pass does not really read each record in the log file
**	as it goes backwards.  On each loop, it scans its transaction list 
**	to find the next highest LSN to process and jumps directly to the
**	next record to process.
**
**	During the undo pass all transactions are undone concurrently and
**	each record processed is done so in reverse LSN order.
**
**	The undo pass will periodically call dmr_check_event to see if there
**	are outstanding opendb requests.
**
**	In a multi-node logging system, such as the VAXCluster, the UNDO pass
**	actually processes all the log files simultaneously. That is, the
**	transactions on the transaction list may come from all the various
**	log files being recovered; at any instant, the UNDO pass advances
**	(recedes?) to the log record with the highest LSN, which in general may
**	cause the UNDO pass to bounce around among all the various log files.
**
**	Thus this pass, as opposed to the analysis and redo passes, has no
**	node_id parameter.
**
** Inputs:
**	rcp			Recovery Context
**
** Outputs:
**	err_code		Reason for error return.
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      14-dec-1992 (rogerk)
**	    Reduced Logging Project: Written.
**	15-mar-1993 (rogerk)
**	    Reduced Logging - Phase IV:
**	    Renamed prev_lsn to compensated_lsn.
**	    Changed Abortsave and EndMini log records to be written as CLR's.
**	        The compensated_lsn field is now used to skip backwards in
**		in the log file rather than using the log addresses in the
**		log records themselves.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	21-jun-1993 (bryanp)
**	    Properly trace log addresses in verbose UNDO log record tracing.
**	26-jul-1993 (bryanp)
**	    If we thought a transaction was in willing commit state, but the
**		first undo log record we actually process for this transaction
**		(following whatever CLR jumping we do) is a BT or ET, then
**		the transaction is actually NOT in willing commit state, so
**		change the transaction type to REDO_UNDO to reflect this.
**	20-jul-1993 (jnash)
**	    Write periodic status messages.
**	26-jul-1993 (rogerk)
**	    Added trace point DM1312 to bypass logdump when recovery fails.
**	23-aug-1993 (bryanp)
**	    2PC fixes:
**		Don't need to reacquire locks in online recovery of prepared
**		    transaction, just change ownership to RCP.
**	23-aug-1993 (jnash)
**	  - Change E_DM943E_RCP_DBUNDO_ERROR to pass lsn rather than 
**	    log address.
**	  - Add iimonitor "show session" status display.  
**	18-oct-1993 (jnash)
**	    Write iircp.log 'percent done' info at 30 second intervals.  This 
**	    eliminates flooding the error log with messages when recovering 
**	    large log intervals.
**	    Added rcp parameter to recover_willing_commit routine.
**	15-apr-1994 (chiku)
**	    Bug56702: mark recovery state as RCP_LG_GONE when detecting
**		      a dead logfull.
**	7-Apr-2005 (schka24)
**	    Observe willing-commit-done as well as tx-complete as an
**	    indicator to not read another log record in the main loop.
**	    This prevents a bad log read and recovery failure when the
**	    P1 stream is immediately preceded by a BT.
**	    Ensure that the P1 stream is complete before embarking on
**	    willing-commit recovery;  undo the transaction otherwise.
*/
DB_STATUS
dmr_undo_pass(
RCP		*rcp)
{
    RTX			*rtx;
    RDMU		*rdmu;
    RTBL		*rtbl;
    LG_RECORD		*record_header;
    DM0L_HEADER		*record;
    DB_TRAN_ID		*tranid;
    DB_TAB_ID		tabid;
    LG_LA		la;
    LG_LSN		prev_lsn;
    i4		dbid;
    i4		records_processed = 0;
    i4		records_read = 0;
    i4			read_a_log_record = 0;
    DB_STATUS		status;
    LG_LSN		lsn;
    char		buffer[80];
    SCF_CB 		scf_cb;
    i4	  	cur_time;
    i4	  	tr_lasttime = TMsecs();
    i4		pc_done;
    i4			error;
    DB_STATUS		local_status;
    i4			local_error;

    CLRDBERR(&rcp->rcp_dberr);

    /* Why read through the log if no UNDO needed? */
    if ( rcp->rcp_db_count == 0 )
    {
	TRdisplay("%@ RCP-P3: Undo Pass - No recovery needed.\n");
	return (E_DB_OK);
    }

    TRdisplay("%@ RCP-P3: Start Undo Phase.\n");
    if (rcp->rcp_verbose)
	TRdisplay("%78*=\n");

    scf_cb.scf_length = sizeof(scf_cb);
    scf_cb.scf_type = SCF_CB_TYPE;
    scf_cb.scf_ascii_id = SCF_ASCII_ID;
    scf_cb.scf_facility = DB_DMF_ID;
    scf_cb.scf_session = DB_NOSESSION;
    scf_cb.scf_ptr_union.scf_buffer = (PTR) buffer;
    scf_cb.scf_nbr_union.scf_atype = SCS_A_MINOR;

    if (rcp->rcp_action == RCP_R_ONLINE || rcp->rcp_action == RCP_R_CSP_ONLINE )
    {
	STprintf(buffer, "RCP Undo recovery: %d log records processed", 0);
	scf_cb.scf_len_union.scf_blength = STlength(buffer);
	(VOID) scf_call(SCS_SET_ACTIVITY, &scf_cb);
    }

    prev_lsn.lsn_high = 0;
    prev_lsn.lsn_low  = 0;

    /*
    ** Scan through the list of abort transactions and read the last record
    ** for each transaction. This "primes the pump" of the undo algorithm;
    ** subsequence processing will locate the log record with the highest LSN,
    ** undo that log record, and then read the next (previous) log record for
    ** that transaction. This priming of the pump is necessary because log
    ** records do not contain a "prev_lsn" field in general; only CLR records
    ** contain this field.
    */
    status = read_first_log_record(rcp);

    for (;status == OK;)
    {
	/*
	** Unless this is the first time through this loop, we've just finished
	** processing the current log record for transaction 'rtx'. Go read
	** the next log record for transaction 'rtx' into the rtx record
	** buffers. If, however, the record we just read was the BT record,
	** or a stream of P1's, this transaction is settled and we don't
	** need to read any more records for it.  (Indeed, we must not,
	** since there is no "next" log record.)
	*/
	if (read_a_log_record != 0 && 
	    (rtx->rtx_status & (RTX_ABORT_FINISHED | RTX_WC_FINISHED)) == 0)
	{
	    status = read_next_log_record(rcp, rtx);
	    if (status)
		break;
	}
	read_a_log_record = 1;

	/*
	** Scan through the list of abort transactions and compare the LSNs
	** of the next record to process for each one.  Return the tx with
	** the highest valued LSN.
	**
	** If there are no more transactions in the undo list, then the undo 
	** phase is complete.
	*/
	find_next_abort_rtx(rcp, &rtx);
	if (rtx == 0)
	{
	    status = E_DB_OK;
	    break;
	}

	record_header = &rtx->rtx_lg_record;
	record        = (DM0L_HEADER *)rtx->rtx_dm0l_record;
	la            = rtx->rtx_lg_rec_la;

	if (rcp->rcp_verbose)
	    trace_record(record_header, record, &la, rtx->rtx_lctx->lctx_bksz);

	/*
	** Consistency Check:
	**
	**   - Verify that we are making progress through the undo phase.
	**     Check that the LSN returned is legal and that it is previous
	**     to the last processed record.
	*/
	if ((LSN_GTE(&rtx->rtx_last_lsn, &prev_lsn)) &&
	    (prev_lsn.lsn_high != 0))
	{
	    /* XXXXX error message */
	    TRdisplay("%@ RCP-P3: Undo Phase not making progress:\n");
	    TRdisplay(
	    "\tCurrent LA:<%d,%d,%d> LSN:(%x,%x), Previous LSN:(%x,%x)\n",
		la.la_sequence, la.la_block,
		la.la_offset,
		rtx->rtx_last_lsn.lsn_high, rtx->rtx_last_lsn.lsn_low,
		prev_lsn.lsn_high, prev_lsn.lsn_low);
	    SETDBERR(&rcp->rcp_dberr, 0, E_DM9437_RCP_P3);
	    status = E_DB_ERROR;
	    break;
	}

	/*
	** Issue periodic monitoring messages.
	*/
	++records_read;
	if ((records_read % 500) == 0)
	{
	    STprintf(buffer,
	       "RCP Undo Pass: %d log records processed", records_read);

	    /*
	    ** Write note to iircp.log, but not too often.
	    */
	    cur_time = TMsecs();
	    if (cur_time >= (tr_lasttime + TRACE_INCR))
	    {
		TRdisplay("%@ %s\n", buffer);
		tr_lasttime = cur_time;
	    }

	    /*
	    ** If online recovery, update IIMONITOR information.
	    ** 
	    ** We should one day enhance this output to include the 
	    ** percentage done by keeping the count of the number of records
	    ** in each transaction (in the rtx), totally them here.
	    */
	    if (rcp->rcp_action == RCP_R_ONLINE || rcp->rcp_action == RCP_R_CSP_ONLINE )
	    {
		scf_cb.scf_len_union.scf_blength = STlength(buffer);
		(VOID) scf_call(SCS_SET_ACTIVITY, &scf_cb);
	    }
	}

	prev_lsn = rtx->rtx_last_lsn;

	/*
	** If the previous record processed for this transaction was a CLR
	** then we want to jump backwards in the transaction chain to the
	** record which was compensated and start recovery processing at
	** the log record previous to it.
	**
	** We skip each log record for this transaction until we find the
	** one whose LSN matches the compensated LSN.
	*/
	if (rtx->rtx_status & RTX_CLR_JUMP)
	{
	    /*
	    ** If we haven't yet reached the LSN we're looking for,
	    ** go on to the next record.
	    */
	    if (LSN_GT(&record->lsn, &rtx->rtx_clr_lsn))
		continue;

	    /*
	    ** Consistency Check:
	    ** If we find an LSN less than the one we are looking for, then
	    ** something is wrong - the record compensated by this LSN is
	    ** not on our transaction chain.
	    */
	    if (LSN_LT(&record->lsn, &rtx->rtx_clr_lsn))
	    {
		/* XXXX ERROR */
		TRdisplay("DMR_ABORT: CLR lsn not found.\n");
		SETDBERR(&rcp->rcp_dberr, 0, E_DM9437_RCP_P3);
		status = E_DB_ERROR;
		break;
	    }
	    
	    /*
	    ** We have found the compensated record. Turn off the
	    ** CLR_JUMP flag so that the next record for this transaction
	    ** will be processed.
	    */
	    rtx->rtx_status &= ~RTX_CLR_JUMP;
	    /*
	    ** SAVEPOINT and BMINI compensated records are not
	    ** written to the log. The compensating ABORTSAVE
	    ** and EMINI records contain the LSN of the transaction
	    ** at the time the SAVEPOINT or BMINI was established,
	    ** so we must fall thru to process this log record.
	    */
	    if ( rtx->rtx_clr_type != DM0LABORTSAVE &&
		 rtx->rtx_clr_type != DM0LEMINI )
		continue;
	}

	/*
	** Look for special log records that cause us to jump back to a 
	** particular spot in the log stream.  Set the CLR_JUMP flag that
	** will cause us to skip log records for this transaction until
	** we jump over those between this CLR and the record it compensates.
	*/
	if (record->flags & DM0L_CLR)
	{
	    STRUCT_ASSIGN_MACRO(record->compensated_lsn, rtx->rtx_clr_lsn);
	    rtx->rtx_clr_type = record->type;
	    rtx->rtx_status |= RTX_CLR_JUMP;
	    continue;
	}

	/*
	** Look for special log record types:
	**
	**     ET  - transaction was committed - mark complete.
	**     BT  - transaction abort completed.
	**     P1  - Willing Commit - restore transaction's context.
	*/

	/*
	** If an End Transaction Log Record was found for this transaction
	** then it actually had already completed.
	**
	** This can occur during online recovery if the commit/abort log 
	** record had been written but a server crash occurred before the
	** LGend call was made.
	**
	** This can occur during offline recovery if the commit/abort log
	** record had been written just prior to a CP event, and the CP
	** log record was written before the LGend call was made.
	*/
	if (record->type == DM0LET)
	{
	    /*
	    ** Mark completed and decrement the count of transactions 
	    ** left to resolve.
	    */
	    rtx->rtx_status |= RTX_TRAN_COMPLETED;
	    rcp->rcp_tx_count--;

	    /*
	    ** If the transaction state had been Willing Commit, then decrement
	    ** the count of such transactions.  This one's context does not
	    ** need to be rebuilt since it had already completed.
	    */
	    if (rtx->rtx_recover_type == RTX_WILLING_COMMIT)
	    {
		rcp->rcp_willing_commit--;
		rtx->rtx_recover_type = RTX_REDO_UNDO;
	    }

	    continue;
	}

	if (record->type == DM0LBT)
	{
	    if (record->flags & DM0L_2ND_BT)
	    {
		/* This is the second BT record for the TX. This means
		** that the TX started out updating non-journaled tables
		** but subsequently updated a journaled table. So indcate
		** thet the ET needs to be journaled, and continue processing
		** to the first BT record for the TX.
		*/
		rtx->rtx_status |= RTX_JNL_TX;

		continue;
	    }
	    if (record->flags & DM0L_JOURNAL)
	    {
		rtx->rtx_status |= RTX_JNL_TX;
	    }

	    rtx->rtx_status |= RTX_ABORT_FINISHED;
	    rcp->rcp_tx_count--;

	    /*
	    ** If the transaction state had been Willing Commit, then decrement
	    ** the count of such transactions.  This one's context does not
	    ** need to be rebuilt since it had already completed.
	    ** This really should not happen since we should have stopped
	    ** at the P1;  and, the P1's are log-forced before we mark the
	    ** tx as willing commit within the logging system.  So, note
	    ** this condition in the trace log so that we can understand it.
	    */
	    if (rtx->rtx_recover_type == RTX_WILLING_COMMIT)
	    {
		rcp->rcp_willing_commit--;
		rtx->rtx_recover_type = RTX_REDO_UNDO;
		TRdisplay("Unexpected BT log record for a willing-commit transaction\n");
		TRdisplay(
		"\tCurrent LA:<%d,%d,%d> LSN:(%x,%x), Previous LSN:(%x,%x)\n",
		    la.la_sequence, la.la_block,
		    la.la_offset,
		    rtx->rtx_last_lsn.lsn_high, rtx->rtx_last_lsn.lsn_low,
		    prev_lsn.lsn_high, prev_lsn.lsn_low);
		/* Would be good to trdisplay the distributed tran ID,
		** but we don't keep it around - FIXME.
		*/
		TRdisplay("\tTransaction will be removed from willing-commit state and aborted.\n");
	    }

	    continue;
	}


	/*****************************************************************
	**
	** Willing Commit Processing:
	**
	******************************************************************/

	/*
	** If the last Prepare to Commit log record is encountered as the
	** FIRST rollback record, then the transaction is in Willing Commit
	** state.  If the transaction is not already listed as such, then
	** change it.
	**
	** It is possible during both online and offline recovery to not
	** recognize a willing commit transaction until the Undo phase due
	** to the short window between the writing of the P1 log record and
	** the LGalter call which informs the Logging System of the transaction
	** state.  A crash or CP during this window can cause this case.
	**
	** If we see a P1, but it's not the last one, the Prepare to Commit
	** did not complete.  Treat the transaction as an ordinary one (if
	** it's not being so treated already), and abort it.
	*/
	if (record->type == DM0LP1)
	{
	    DM0L_P1 *p1 = (DM0L_P1 *) record;

	    if (p1->p1_flag == P1_LAST)
	    {
		/* Looks like a valid willing-commit.  Recover it as such,
		** unless we're trying to abort it instead (eg lartool).
		*/
		if (rtx->rtx_recover_type != RTX_WILLING_COMMIT
		  && ( ! (rtx->rtx_status & (RTX_ABORT_INPROGRESS | RTX_MAN_ABORT))) )
		{
		    /* Go into willing-commit recovery state */
		    rtx->rtx_recover_type = RTX_WILLING_COMMIT;
		    ++ rcp->rcp_willing_commit;
		}
		/* If we're (now) in willing-commit recovery for this
		** transaction, do that.  For RCP Online recovery, no re-
		** establishment of transaction context is needed.
		** All the required context management has been performed
		** during dmr_init_recovery, which has set the ownership
		** of the transaction and lock list(s) to the RCP.
		** 
		** During offline recovery, we need to process the P1
		** stream, which completes the transaction context and
		** reissues all the locks that the transaction had been
		** holding pre-crash.  If we can't do that successfully,
		** we'll abort the transaction instead, and cause the
		** database to go inconsistent (or fail the RCP) afterwards.
		** FIXME FIXME FIXME
		** WE REALLY NEED A DIFFERENT cause for going inconsistent.
		** This is a logical inconsistency, not a physical one;
		** after resolving any potential data integrity issues,
		** it's perfectly safe (physically) to force the DB consistent.
		**
		** In either case, after successful processing, the willing-
		** commit transaction is re-established, and we mark the
		** transaction as "no more processing needed".
		*/

		if (rtx->rtx_recover_type == RTX_WILLING_COMMIT)
		{
		    if ((rcp->rcp_action & RCP_R_ONLINE) != 0)
			status = E_DB_OK;
		    else
			status = recover_willing_commit(rcp, rtx);
		    if (status == E_DB_OK)
		    {
			rtx->rtx_status |= RTX_WC_FINISHED;
			rcp->rcp_tx_count--;
		    }
		    else if (Db_recovery_error_action != RCP_STOP_ON_INCONS_DB)
		    {
			/*
			** XXXX error message.
			*/
			TRdisplay("Willing Commit Restoration Failed\n");
			TRdisplay("\tTransaction will be aborted\n");
			TRdisplay("\tDatabase will then be marked INCONSISTENT.\n");
			TRdisplay("\tThis is a Logical, not Physical, inconsistency; if no\n");
			TRdisplay("\tother errors occur, the database may be forced consistent.\n");

			rcp->rcp_willing_commit--;
			rtx->rtx_recover_type = RTX_REDO_UNDO;
			rtx->rtx_rdb->rdb_status |= RDB_WILLING_FAIL;
			status = E_DB_OK;
			CLRDBERR(&rcp->rcp_dberr);
		    }
		    else
		    {
			SETDBERR(&rcp->rcp_dberr, 0, E_DM942A_RCP_DBINCON_FAIL);
			status = E_DB_ERROR;
			break;
		    }
		    continue;
		}
		/* else not in willing-commit recovery, just eat this P1,
		** move on with the abort
		*/
	    }
	    /* else not a LAST P1, treat it as J Random LogRecord, fall thru
	    ** to switch the transaction (if needed) into regular redo-undo
	    ** recovery, and abort it.  (Getting here means that we saw a
	    ** non-LAST P1 but no LAST P1.)
	    */
	}

	/*
	** If this transaction was listed as in willing commit state, and
	** we encounter a non DM0LP1 log record, then assume that the
	** transaction is no longer in willing commit state - it must have
	** never actually had the LAST P1 record written or else it had
	** previously started to be backed out.
	**
	** We switch the state to being aborted.
	*/
	if ((rtx->rtx_recover_type == RTX_WILLING_COMMIT) &&
	    ((rtx->rtx_status & RTX_COMPLETE) == 0))
	{
	    rcp->rcp_willing_commit--;
	    rtx->rtx_recover_type = RTX_REDO_UNDO;
	}

	/* Skip log record for tables marked invalid */
	rtbl = NULL;
	if (rtx->rtx_rdb->rdb_status & RDB_TABLE_RECOVERY_ERROR)
	{
	   dmve_get_tab(record, &tabid, NULL, NULL);
	   lookup_invalid_tbl(rtx->rtx_rdb, &tabid, record, &rtbl);
	   if (rtbl &&
		(rtbl->rtbl_flag & RTBL_IGNORE_TABLE) ||
			rtbl_ignore_page(rtbl, record) == TRUE)
	    {
#ifdef xDEBUG
		dmd_log(FALSE, (PTR) record, rtx->rtx_lctx->lctx_bksz);
		TRdisplay("\tUNDO SKIPPED due to INVALID table status.\n");
#endif
		rtbl->undo_cnt++;
		continue;
	    }
	}

	/*
	** Apply Undo Recovery for this log record.
	*/
	status = apply_undo(rcp, rtx, record, &la);

#ifdef xDEBUG
	/* DEBUG: if an index, force an error! */
	if (tabid.db_tab_index > 100 && tabid.db_tab_index % 5 == 0)
	{
	    SETDBERR(&rcp->rcp_dberr, 0, E_DM9639_DMVE_UNDO);
	    status = E_DB_ERROR;
	}
#endif

	if (status != E_DB_OK)
	{
	    /*
	    ** If an error occurs during UNDO processing, we mark the RDB
	    ** for that database as invalid and try to keep on processing
	    ** other databases that need recovery.
	    */
            /*
	     * Bug56702: mark recovery state as dead logfull.
	     */
	    if ( rcp->rcp_dberr.err_code == E_DM9070_LOG_LOGFULL ) 
	    {
	    	rcp->rcp_state = RCP_LG_GONE;
	    } 
	    else
	    {
	    	uleFormat(&rcp->rcp_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
			(char *)NULL, (i4)0, (i4 *)NULL, &error, 0);

		/* If possible, mark table or index invalid for redo/undo */
		status = add_invalid_tbl(rcp, rtx->rtx_rdb, rtbl, 
			    rtx->rtx_lctx, record);
		if (status != E_DB_OK)
		{
		    uleFormat(NULL, E_DM943E_RCP_DBUNDO_ERROR, (CL_ERR_DESC *)NULL, ULE_LOG, 
			NULL, (char *)NULL, (i4)0, (i4 *)NULL, &error, 5,
			sizeof(rtx->rtx_rdb->rdb_name), &rtx->rtx_rdb->rdb_name,
			0, record->lsn.lsn_high, 0, record->lsn.lsn_low, 
			0, rtx->rtx_tran_id.db_high_tran, 
			0, rtx->rtx_tran_id.db_low_tran);
		    mark_invalid(rcp, rtx->rtx_rdb, DSC_UNDO_INCONSIST);
		}
	    	dmd_log(FALSE, (PTR) record, rtx->rtx_lctx->lctx_bksz);
		TRdisplay("\tUNDO error.\n");
	    }

	    if ( ! DMZ_ASY_MACRO(12))
		dump_transaction(rcp, rtx);

	    status = E_DB_OK;
	    CLRDBERR(&rcp->rcp_dberr);
	}

	/*
	** If we are executing Online Recovery from the RCP then check
	** periodically for RCP background work to do like processing
	** open database requests.
	*/
	records_processed++;
	if ((rcp->rcp_action == RCP_R_ONLINE) && 
	    ((records_processed % 100) == 0))
	{
	    dmr_check_event(rcp, LG_NOWAIT, rtx->rtx_lctx->lctx_lxid);
	}
    }

    if (rcp->rcp_verbose)
	TRdisplay("%78*=\n");

    (VOID) scf_call(SCS_CLR_ACTIVITY, &scf_cb);

    if (status == E_DB_OK)
    {
        TRdisplay("%@ RCP-P3: Undo Complete.\n");
	return (E_DB_OK);
    }

    /*
    ** Error Handling:
    */
    uleFormat(&rcp->rcp_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
	(char *)NULL, (i4)0, (i4 *)NULL, &error, 0);

    SETDBERR(&rcp->rcp_dberr, 0, E_DM9437_RCP_P3);
    return (E_DB_ERROR);
}

/*{
** Name: dmr_preinit_recovery - Prepare database state for initialization phase.
**
** Description:
**	Initializing databases for recovery currently requires us to execute
**	a logical open of these databases.  Unfortunately, to some degree this
**	violates the recovery model since this logical open requires certain
**	resources to be free and in a consistent state - even though recovery
**	has not yet been performed on the database.
**
**	Ideally, nothing would be required of the database until REDO recovery
**	was run, at which time the database would be physically consistent
**	and those objects which are updated with physical locks would be
**	accessable.  Today, we have special requirements on the Config File
**	and the core catalog pages since they are accessed before redo recovery.
**
**	This routine runs through this list of recover transactions and
**	releases physical core catalog page locks and config file locks.
**
**	The objects protected by these locks (core catalog pages and config
**	files) are guaranteed by the DBMS routines which manage and update
**	them to be in such a state that they can be accessed by opendb.
**	This does not mean that they are completely consistent (they won't
**	be until recovery is complete) or that they are even physically
**	consistent (they won't be until REDO is complete), but only that
**	they can be accessed in the way that opendb uses them.
**
**	As a side effect of releasing locks, this routine adopts ownership
**	of the lock lists of the transactions needing recovery.
**
** Inputs:
**	rcp			Recovery Context
**
** Outputs:
**	err_code		Reason for error return.
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	The lock lists previously owned by the recover transactions are adopted
**	by the RCP.
**
** History:
**      31-jan-1994 (rogerk)
**	    Written during bugfixing of Reduced Logging Project.
**      28-feb-1997 (stial01)
**          Added process_sv_database_lock(), and code to make sure that
**          if online partial recovery of a row locking transaction, 
**          we must have exclusive SV_DATABASE lock. This will block
**          connections to the database while it is being recovered.
**      20-may-1999 (stial01)
**          Fixed parameter to process_sv_database_lock() B94438
**	11-Sep-2006 (jonj)
**	    If transaction lock list can't be found via tran_id
**	    and we know the transaction's lock list id, try
**	    finding it that way. LOGFULL_COMMIT transactions
**	    update the tran_id in LG, but not in LK.
*/
static DB_STATUS
dmr_preinit_recovery(
RCP		*rcp)
{
    RCP_QUEUE		*st;
    RTX			*tx;
    LK_LKB_INFO		*lkb;
    LK_LKB_INFO		*lkb_info;
    LK_LKID		lockid;
    u_i4		context;
    u_i4		length;
    char		info_buffer[600];
    char		*buffer;
    i4			buf_size;
    i4		i;
    i4             j;
    bool		lock_released;
    STATUS		cl_status;
    CL_ERR_DESC		sys_err;
    DB_STATUS		status = E_DB_OK;
    LK_LLID             lli;
    RLP                 *rlp;
    RDB                 *rdb;
    LK_UNIQUE           svcb_uniqid;
    i4			error;

    CLRDBERR(&rcp->rcp_dberr);

    /*
    ** During OFFLINE or CSP recovery, no locks are currently held and no
    ** pre-initialization is needed here.
    */
    if (rcp->rcp_action != RCP_R_ONLINE)
	return (E_DB_OK);

    buffer = (PTR) ME_ALIGN_MACRO(info_buffer, sizeof(ALIGN_RESTRICT));
    buf_size = sizeof(info_buffer) - (buffer - info_buffer);

    /*
    ** Adopt each transaction needed for recovery.
    ** 
    ** Look through the locks owned by that transaction and release any
    ** config locks or core system catalog page locks.
    **
    ** Also check for any EXCL row locks which would require exclusive
    ** access to the database during recovery.
    */
    for (st = rcp->rcp_tx_state.stq_next;
	st != (RCP_QUEUE *)&rcp->rcp_tx_state.stq_next;
	st = tx->rtx_state.stq_next)
    {
	tx = (RTX *)((char *)st - CL_OFFSETOF(RTX, rtx_state));

	/*
	** Assume ownership of the transaction lock list so that we can make
	** lock calls with it.
	*/
	cl_status = LKcreate_list((i4) LK_RECOVER, (LK_LLID) 0, 
			    (LK_UNIQUE *) &tx->rtx_tran_id, 
			    (LK_LLID *) &tx->rtx_ll_id, 0, 
			    &sys_err);
	if (cl_status != OK)
	{
	    /*
	    ** Not found by tran_id. If a lock list id is supplied, try finding it
	    ** that way. LOGFULL_COMMIT protocols update the tran_id in logging, but not
	    ** in the lock list block, so this mismatch is anticipated.
	    */
	    if ( tx->rtx_ll_id )
		cl_status = LKcreate_list((i4) LK_RECOVER | LK_RECOVER_LLID, (LK_LLID) 0, 
				    (LK_UNIQUE *) &tx->rtx_tran_id, 
				    (LK_LLID *) &tx->rtx_ll_id, 0, 
				    &sys_err);
	    if ( cl_status )
	    {
		uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
		uleFormat(NULL, E_DM901A_BAD_LOCK_CREATE, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, 0L, (i4 *)NULL, &error, 0);
		status = E_DB_ERROR;
		break;
	    }
	}

	TRdisplay("\tTransaction %x%x associated to lock list %x\n",
	    tx->rtx_tran_id.db_high_tran, tx->rtx_tran_id.db_low_tran,
	    tx->rtx_ll_id);

	/*
	** Call LKshow on this lock list until no more locks are returned.
	** Any that are of type LK_CONFIG or are LK_PAGE and are on core
	** catalog pages we need to release.
	**
	** If any EXCL ROW locks are found, update tx and associated rdb status
	*/
	context = 0;
	for (;;)
	{
	    cl_status = LKshow(LK_S_LIST_LOCKS, tx->rtx_ll_id, 0, 0, 
			    buf_size, (PTR)buffer, &length, 
			    &context, &sys_err); 
	    if (cl_status != OK)
	    {
		uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
		uleFormat(NULL, E_DM901D_BAD_LOCK_SHOW, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, &error, 1,
		    0, tx->rtx_ll_id);
		status = E_DB_ERROR;
		break;
	    }

	    if (length == 0)
		break;

	    /*
	    ** Look through each lock returned in the info buffer.
	    */
	    lock_released = FALSE;
	    lkb_info = (LK_LKB_INFO *) buffer;
	    for (i = 0; i < ((i4)length / sizeof(LK_LKB_INFO)); i++)
	    {
		lkb = &lkb_info[i];

		/*
		** If any row locking transactions update the
		** transaction and database status.
		*/
		if (lkb->lkb_key[0] == LK_ROW && lkb->lkb_grant_mode == LK_X)
		{
		    tx->rtx_status |= RTX_XROW_LOCK;
		    tx->rtx_rdb->rdb_status |= RDB_XROW_LOCK;
		}

		/*
		** If the lock is a database Config File lock or is a
		** sconcur catalog page lock then release it.
		*/
		if ((lkb->lkb_key[0] == LK_CONFIG) ||
		    ((lkb->lkb_key[0] == LK_PAGE) &&
			(lkb->lkb_key[2] <= DM_SCONCUR_MAX)))
		{
		    lockid.lk_unique = lkb->lkb_id;
		    lockid.lk_common = lkb->lkb_rsb_id;

		    TRdisplay("\t    Releasing lock %x (type %s).\n",
			lkb->lkb_id, (lkb->lkb_key[0] == LK_CONFIG) ? 
			"CONFIG" : "SCONCUR PAGE");

		    cl_status = LKrelease((i4) 0, tx->rtx_ll_id, &lockid, 
				    (LK_LOCK_KEY *)0, (LK_VALUE *)0, &sys_err);
		    if (cl_status != OK)
		    {
			uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
			    (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
			uleFormat(NULL, E_DM901B_BAD_LOCK_RELEASE, &sys_err, ULE_LOG,
			    NULL, (char *)NULL, (i4)0, (i4 *)NULL, &error,
			    1, 0, tx->rtx_ll_id);
			status = E_DB_ERROR;
			break;
		    }

		    lock_released = TRUE;
		}
	    }

	    if (status != E_DB_OK)
		break;

	    /*
	    ** If there are more locks to check for this lock list then
	    ** loop back around for another LKshow call.
	    **
	    ** If we have released any locks returned in the previous
	    ** LKshow call then we have to reset the show context and start
	    ** over from the beginning of the lock list.  This ensures
	    ** that we will not miss any locks as a result of changing the
	    ** lock list in the middle of showing it.
	    */
	    if (context)
	    {
		if (lock_released)
		    context = 0;
		continue;
	    }

	    break;
	}

	if (status != E_DB_OK)
	    break;
    }

    if (status != E_DB_OK) 
    {
	uleFormat(NULL, E_DM9432_RCP_OPENTX_ERROR, &sys_err, ULE_LOG, NULL,
	    (char *)NULL, (i4)0, (i4 *)NULL, &error, 3,
	    sizeof(tx->rtx_rdb->rdb_name), &tx->rtx_rdb->rdb_name,
	    0, tx->rtx_tran_id.db_high_tran, 
	    0, tx->rtx_tran_id.db_low_tran);
	SETDBERR(&rcp->rcp_dberr, 0, E_DM9443_DMR_PREINIT_RECOVER);
	return (E_DB_ERROR);
    }

    /*
    ** If any databases will undergo online RDB_REDO_PARTIAL recovery,
    ** and there are open row locking transactions, we must make sure
    ** we have EXCL access to the database during the recovery.
    **
    ** RDB_REDO_PARTIAL recovery is done for non-fast-commit db's
    **
    **     Row locking is disallowed for MULTIPLE servers using different
    **     data caches.
    **
    **     Row locking is allowed for SINGLE server mode, in which case the
    **     EXCL SV_DATABASE lock is acquired on a separate lock list the
    **     RCP inherits.
    */
    for (i = 0; (status == E_DB_OK) && (i < RCP_LP_HASH); i++)
    {
	for (rlp = rcp->rcp_lph[i]; rlp != NULL; rlp = rlp->rlp_next)
	{
	    /*
	    ** If the dead process was running in SINGLE-SERVER mode,
	    ** we should find an XSVDB lock list for it.
	    */
	    context = 0;
	    cl_status = LKshow( LK_S_XSVDB, 0, 0, (LK_LOCK_KEY *)&rlp->rlp_pid,
			    sizeof(lli), (PTR)&lli, (u_i4 *)&length,
			    (u_i4 *)&context, &sys_err);
	    if (cl_status != OK)
	    {
		uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL, 
		    (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
		uleFormat(NULL, E_DM901D_BAD_LOCK_SHOW, &sys_err, ULE_LOG, NULL, 
		    (char *)NULL, (i4)0, (i4 *)NULL, &error, 1,
		    0, LK_S_XSVDB);
		status = E_DB_ERROR;
		break;
	    }

	    /*
	    ** If XSVDB lock list was found, save the lock list id
	    */
	    if (context != 0)
	    {
		rlp->rlp_svdb_ll_id = lli;
		rlp->rlp_status |= RLP_XSVDB_LOCK;
	    }
	}
    }

    for (i = 0; status == E_DB_OK && i < RCP_LP_HASH; i++)
    {
	for (rlp = rcp->rcp_lph[i]; rlp != NULL; rlp = rlp->rlp_next)
	{
	    if (rlp->rlp_status & RLP_XSVDB_LOCK)
	    {
		/*
		** Assume ownership of the server lock list
		** so that we can make lock calls with it.
		*/
		cl_status = LKcreate_list(
				(i4) LK_RECOVER | LK_RECOVER_LLID,
				(LK_LLID) 0, (LK_UNIQUE *)&svcb_uniqid, 
				&rlp->rlp_svdb_ll_id, 0, &sys_err);
		if (cl_status != OK)
		{
		    uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
			(char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
		    uleFormat(NULL, E_DM901A_BAD_LOCK_CREATE, &sys_err, ULE_LOG, NULL,
			(char *)NULL, 0L, (i4 *)NULL, &error, 0);
		    status = E_DB_ERROR;
		    break;
		}

		/*
		** Call LKshow on this lock list until no more locks are 
		** returned.
		*/
		context = 0;
		for (;;)
		{
		    cl_status = LKshow(LK_S_LIST_LOCKS, rlp->rlp_svdb_ll_id,
				    0, 0, buf_size, (PTR)buffer, &length, 
				    &context, &sys_err); 
		    if (cl_status != OK)
		    {
			uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
			    (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
			uleFormat(NULL, E_DM901D_BAD_LOCK_SHOW, &sys_err, ULE_LOG, 
			    NULL, (char *)NULL, (i4)0, (i4 *)NULL, &error,
			    1, 0, lli);
			status = E_DB_ERROR;
			break;
		    }

		    if (length == 0)
			break;

		    /*
		    ** Look through each lock returned in the info buffer.
		    */
		    lock_released = FALSE;
		    lkb_info = (LK_LKB_INFO *) buffer;
		    for (j = 0; j < ((i4)length / sizeof(LK_LKB_INFO)); j++)
		    {
			lkb = &lkb_info[j];

			/*
			** If any EXCL SV_DATABASE locks, update rdb status
			*/
			if (lkb->lkb_key[0] == LK_SV_DATABASE 
			    && lkb->lkb_grant_mode == LK_X)
			{
			    status = process_sv_database_lock(rcp, rlp, 
				lkb, &lock_released);
			    if (status != E_DB_OK)
				break;
			}
		    }

		    if (status != E_DB_OK)
			break;

		    /*
		    ** If there are more locks to check for this lock list then
		    ** loop back around for another LKshow call.
		    **
		    ** If we have released any locks returned in the previous
		    ** LKshow call then we have to reset the show context and 
		    ** start over from the beginning of the lock list. 
		    ** This ensures that we will not miss any locks as a
		    ** result of changing the lock list in the middle
		    ** of showing it.
		    */
		    if (context)
		    {
			if (lock_released)
			    context = 0;
			continue;
		    }

		    break;
		}
	    }

	    if (status != E_DB_OK)
		break;
	}
    }

    /*
    **
    ** For each database undergoing partial recovery:
    **
    **     Check if we have inherited a lock list with an EXCL SV_DATABASE lock.
    **     Set RDB_XSVDB_LOCK in rdb_status if we do.
    **
    **     Check if the database has uncommitted row locking transactions,
    **     we must have an EXCL SV_DATABASE lock for that database.
    **
    **     Release SV_DATABASE lock if no row locking transactions being 
    **     recovered.
    */
    for (st = rcp->rcp_db_state.stq_next;
	(status == E_DB_OK) && (st != (RCP_QUEUE *)&rcp->rcp_db_state.stq_next);
	st = rdb->rdb_state.stq_next)
    {
	rdb = (RDB *)((char *)st - CL_OFFSETOF(RDB, rdb_state));

	/*
	** If partial redo, make sure we have SV_DATABASE lock
	*/
	if ( (rdb->rdb_status & RDB_RECOVER)
	    && (rdb->rdb_status & RDB_REDO_PARTIAL))
	{
	    if (rdb->rdb_status & RDB_XSVDB_LOCK)
	    {
		/*
		** Release EXCL SV_DATABASE lock if it is not needed.
		** We need it if we are recovering row locking transactions.
		*/
		if ( (rdb->rdb_status & RDB_XROW_LOCK) == 0)
		{
		    lockid.lk_unique = rdb->rdb_svdb_lkb_id;
		    lockid.lk_common = rdb->rdb_svdb_rsb_id;
		    cl_status = LKrelease((i4) 0, rdb->rdb_svdb_ll_id, 
				    &lockid, (LK_LOCK_KEY *)0, (LK_VALUE *)0,
				    &sys_err);
		    if (cl_status == OK)
		    {
			rdb->rdb_status &= ~RDB_XSVDB_LOCK;
		    }
		    else
		    {
			uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
			    (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
			uleFormat(NULL, E_DM901B_BAD_LOCK_RELEASE, &sys_err, ULE_LOG,
			    NULL, (char *)NULL, (i4)0, (i4 *)NULL, &error,
			    1, 0, rdb->rdb_svdb_ll_id);
			status = E_DB_ERROR;
		    }
		}
	    }
	    else if (rdb->rdb_status & RDB_XROW_LOCK)
	    {
		uleFormat(NULL, E_DM9446_RCP_NO_SVDB_LOCK, (CL_ERR_DESC *)NULL, ULE_LOG,
		    NULL, (char *)NULL, (i4)0, (i4 *)NULL, &error, 1,
		    sizeof(rdb->rdb_name), &rdb->rdb_name);
		status = E_DB_ERROR;
		break;
	    }
	}
    }

    if (status != E_DB_OK) 
    {
	uleFormat(NULL, E_DM9447_RCP_SVDB_LOCK_ERROR, &sys_err, ULE_LOG, NULL,
	    (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	SETDBERR(&rcp->rcp_dberr, 0, E_DM9443_DMR_PREINIT_RECOVER);
	return (E_DB_ERROR);
    }

    return (E_DB_OK);
}

/*{
** Name: dmr_update_bof - Update Log File BOF in the logfile header.
**
** Description:
**	At completion of the analysis pass of offline recovery, the rcp
**	updates the logfile header to show the BOF as updated by the
**	last ECP or SETBOF log record.
**
**	This restores the BOF to the last known consistent state.  Records
**	in the log file after this spot may need to be processed by the
**	archiver before the logging system can move the BOF forward any more.
**
** Inputs:
**	rcp			Recovery Context
**	node_id			Identifies which node's logfile is to be
**				affected, in a multi-node logging system.
**
** Outputs:
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      14-dec-1992 (rogerk)
**	    Reduced Logging Project: Written.
**	15-mar-1993 (bryanp)
**	    Cluster support: added node_id parameter.
**	18-oct-1993 (rogerk)
**	    Made updating of BOF conditional upon it moving the bof forward.
**	    We should never move the log file bof backward.
*/
static DB_STATUS
dmr_update_bof(
RCP		*rcp,
i4		node_id)
{
    DMP_LCTX		*lctx = rcp->rcp_lctx[node_id];
    i4		length;
    STATUS		cl_status = OK;
    CL_ERR_DESC		sys_err;
    LG_HDR_ALTER_SHOW	alter_parms;
    i4			error;

    CLRDBERR(&rcp->rcp_dberr);

    TRdisplay("%@ UPDATE_BOF: BOF after Recovery: <%d,%d,%d>\n",
	    lctx->lctx_bof.la_sequence,
	    lctx->lctx_bof.la_block,
	    lctx->lctx_bof.la_offset);

    for (;;)
    {
	/* Show the header. */

	alter_parms.lg_hdr_lg_id = lctx->lctx_lgid;
	cl_status = LGshow(LG_A_NODELOG_HEADER, (PTR)&alter_parms,
			sizeof(alter_parms), &length, &sys_err);
	if (cl_status != OK)
	{
	    SETDBERR(&rcp->rcp_dberr, 0, E_DM9017_BAD_LOG_SHOW);
	    break;
	}

	if (LGA_GT(&lctx->lctx_bof, &alter_parms.lg_hdr_lg_header.lgh_begin))
	{
	    STRUCT_ASSIGN_MACRO(lctx->lctx_bof,
		alter_parms.lg_hdr_lg_header.lgh_begin);
	}

	cl_status = LGalter(LG_A_NODELOG_HEADER, (PTR)&alter_parms,
				sizeof(alter_parms), &sys_err);
	if (cl_status != OK)
	{
	    SETDBERR(&rcp->rcp_dberr, 0, E_DM900B_BAD_LOG_ALTER);
	    break;
	}

	return (E_DB_OK);
    }

    uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
	(char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
    uleFormat(&rcp->rcp_dberr, 0, &sys_err, ULE_LOG, NULL,
	(char *)NULL, (i4)0, (i4 *)NULL, &error, 1,
	0, LG_A_NODELOG_HEADER);

    SETDBERR(&rcp->rcp_dberr, 0, E_DM9436_DMR_UPDATE_BOF);
    return (E_DB_ERROR);
}

/*{
** Name: dmr_get_cp - Calculate the log address of the last CP.
**
** Description:
**      This routine calculates the log address of the last CP.
**
**	It is called during machine failure recovery when the log file is
**	opened and the log file header status indicates that the Last CP
**	value in the header cannot be trusted.
**
**	The log file is read from the EOF backwards until a Begin CP record
**	if found for which there is a matching End CP record.
**
**	Both the Last CP address and the BOF values are updated in the
**	Log Header from the Consistency Point information.
**
** Inputs:
**	rcp			    - The Recovery Control block
**	node_id			    - indicator of a specific logfile to process
**
** Outputs:
**	err_code		    - reason for error return.
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      30-Sep-1986 (ac)
**          Created for Jupiter.
**	19-may-1988 (edhsu)
**	    added code for the fast commit project.
**	15-mar-1993 (bryanp)
**	    Cluster support:
**		Changed arguments to be rcp, node_id. 
**		Pass correct transaction handle to dm0l_read.
**       1-May-2006 (hanal04) bug 116059
**          BCP records are not guaranteed to be contiguous. Scan backwards
**          from the ECP to the first BCP record which will be flagged
**          with the new CP_FIRST flag.
*/
DB_STATUS
dmr_get_cp(
RCP		*rcp,
i4		node_id)
{
    DB_STATUS		status = E_DB_OK;
    CL_ERR_DESC		sys_err;
    DM0L_HEADER		*record;
    LG_LA		la;
    LG_LSN		lsn;
    LG_LA		bof;
    DM0L_ECP		*ecp;
    DM0L_BCP            *bcp;
    LG_HEADER		*header;
    DMP_LCTX		*lctx = rcp->rcp_lctx[node_id];
    i4			error;

    CLRDBERR(&rcp->rcp_dberr);

    for (;;)
    {
	header = &lctx->lctx_lg_header;

	if (status = dm0l_position(DM0L_LAST, lctx->lctx_lxid,
		(LG_LA *)&(header->lgh_end), lctx, &rcp->rcp_dberr))
	{
	    break;
	}

	for (;;)
	{
	    if (status = dm0l_read(lctx, lctx->lctx_lxid, &record,
				    &la, (i4*)NULL, &rcp->rcp_dberr))
	    {
		break;
	    }
	    if (record->type != DM0LECP)
		continue;

	    ecp = (DM0L_ECP*) record;

	    /*  Get the BOF. */

	    STRUCT_ASSIGN_MACRO((*((LG_LA *)&ecp->ecp_bof)), bof);

	    do
	    {
		if (status = dm0l_read(lctx, lctx->lctx_lxid, &record,
					    &la, (i4*)NULL, &rcp->rcp_dberr))
		{
		    break;
		}
	    } while (record->type != DM0LBCP);
	    if (status != E_DB_OK && rcp->rcp_dberr.err_code != E_DM9238_LOG_READ_EOF)
		break;

	    do
	    {
		if (status = dm0l_read(lctx, lctx->lctx_lxid, &record,
					    &la, (i4*)NULL, &rcp->rcp_dberr))
		{
		    break;
		}
                if (record->type == DM0LBCP)
                {
                    bcp = (DM0L_BCP*) record;
                    if(bcp->bcp_flag & CP_FIRST)
                    {
                        status = dm0l_read(lctx, lctx->lctx_lxid, &record,
                                            &la, (i4*)NULL, &rcp->rcp_dberr);
                        break;
                    }
                }
	    } while (rcp->rcp_dberr.err_code != E_DM9238_LOG_READ_EOF);

	    if (status == E_DB_OK || rcp->rcp_dberr.err_code == E_DM9238_LOG_READ_EOF)
	    {
		status = E_DB_OK;
		CLRDBERR(&rcp->rcp_dberr);
		STRUCT_ASSIGN_MACRO(la, header->lgh_cp);
	    }
	    break;
	}
	break;
    }
    if (status)
    {
	uleFormat(&rcp->rcp_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
	    (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	uleFormat(NULL, E_DM9405_RCP_GETCP, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
	    (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);

	header->lgh_status = LGH_BAD;
	SETDBERR(&rcp->rcp_dberr, 0, E_DM9405_RCP_GETCP);
	status = E_DB_ERROR;
    }
    else
    {
	if ((header->lgh_end.la_sequence == bof.la_sequence && 
	     header->lgh_end.la_block >= bof.la_block) ||
	    (header->lgh_end.la_sequence > bof.la_sequence &&
	     header->lgh_end.la_block < bof.la_block))
	{
	    STRUCT_ASSIGN_MACRO(bof, header->lgh_begin);
	}
	header->lgh_status = LGH_RECOVER;	
    }
    return(status);
}

/*{
** Name: dmr_cleanup
**
** Description:
**	Cleans up recovery context so RCP structure can be deallocated.
**	This routine is called at the completion of a recovery event.
**
** Inputs:
**	rcp			Recovery Context
**
** Outputs:
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      14-dec-1992 (rogerk)
**	    Reduced Logging Project: Written.
*/
VOID
dmr_cleanup(
RCP		*rcp)
{
    nuke_nonredo(rcp);
    nuke_rtx(rcp, (RDB *)0);
    nuke_rdb(rcp);
    nuke_rlp(rcp);
}

/*{
** Name: check_recovery - Check config file to test if recovery is needed.
**
** Description:
**	This routine opens the configuration file and looks to see if this
**	database truly is open on this node. We may discover that the database
**	is in fact closed, which can happen during race conditions where the
**	database has been closed but the logfile still contains log records
**	describing (recently completed) operations against that database. If
**	we find that the database is closed, then we know that all updated pages
**	were safely flushed out to disk before the config file open mask was
**	reset, and hence we do not need to perform any recovery.
**
**	We may also discover in the config file that this database has been
**	marked inconsistent, in which case any attempts to perform recovery
**	will be futile.
**
** Inputs:
**	rcp			Recovery Context
**	rdb			The database we're interested in.
**
** Outputs:
**	err_code		Reason for error return.
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      14-dec-1992 (rogerk)
**	    Reduced Logging Project: Written.
**	26-apr-1993 (bryanp)
**	    6.5 Cluster Support:
**		We can't use a global rcp_node_id, since the database may need
**		    or not need recovery on several nodes simultaneously.
**		    Instead, we look at the actual node id for this particular
**		    RDB, which is stored in rdb_node_id.
**	21-june-1993 (jnash)
**	    If database had been previously marked closed and we found
**	    unresolved tx's, issue error.  Also, deallocate rtx's if 
**	    database is either marked closed or inconsistent.
**	21-jun-1993 (rogerk)
**	    Add further checks when deallocating rtx's when the database is
**	    marked closed but all transactions were not resolved.  Ignore
**	    those transactions which are marked RTX_FOUND_ONLY_IN_CP and
**	    only complain if other transactions found unresolved.
*/
static DB_STATUS
check_recovery(
RCP		*rcp,
RDB		*rdb)
{
    DMP_DCB	    local_dcb;
    DM0C_CNF	    *cnf = 0;
    RCP_QUEUE	    *st;
    RTX		    *rtx;
    DB_STATUS	    status;
    i4	    open_mask;
    i4		    unexpected_xacts;
    bool	    database_open = FALSE;
    bool	    database_inconsistent = FALSE;
    i4		    error;

    CLRDBERR(&rcp->rcp_dberr);

    /*
    ** Initialize a dummy DCB to use for opening the Config File.
    ** (this seems kinda messy - why does dm0c require a dcb).
    */
    MEfill( sizeof(local_dcb), 0, (char *)&local_dcb );
    local_dcb.dcb_type = DCB_CB;
    local_dcb.dcb_length = sizeof(DMP_DCB);
    local_dcb.dcb_ascii_id = DCB_ASCII_ID;

    MEmove(sizeof(rdb->rdb_name), (PTR)&rdb->rdb_name, ' ',
	    sizeof(local_dcb.dcb_name), (PTR)&local_dcb.dcb_name);
    MEmove(sizeof(rdb->rdb_owner), (PTR)&rdb->rdb_owner, ' ',
	    sizeof(local_dcb.dcb_db_owner), (PTR)&local_dcb.dcb_db_owner);
    MEmove(rdb->rdb_l_root, (PTR)&rdb->rdb_root, ' ',
	    sizeof(local_dcb.dcb_location.physical), 
	    (PTR)&local_dcb.dcb_location.physical);

    local_dcb.dcb_location.phys_length = rdb->rdb_l_root;
    local_dcb.dcb_access_mode = DCB_A_WRITE;
    local_dcb.dcb_served = DCB_MULTIPLE;
    local_dcb.dcb_db_type = DCB_PUBLIC;
    local_dcb.dcb_status = 0;

    for (;;)
    {
	/*
	** Open the config file
	*/
	status = dm0c_open(&local_dcb, DM0C_PARTIAL, dmf_svcb->svcb_lock_list, 
			    &cnf, &rcp->rcp_dberr);
	if (status != E_DB_OK)
	    break;

	/*
	** Check the open count for this node.
	*/
	open_mask = 1 << rdb->rdb_node_id;
	database_open = ((cnf->cnf_dsc->dsc_open_count & open_mask) != 0);
	database_inconsistent = ((cnf->cnf_dsc->dsc_status & DSC_VALID) == 0);

	/*
	** If the database is already inconsistent, report it
	*/
	if (database_inconsistent)
	{
	    TRdisplay("%@ Recovery processing bypassed on Database %~t.\n",
		sizeof(DB_DB_NAME), &rdb->rdb_name);
	    TRdisplay("\tDatabase is marked inconsistent from a previous ");
	    TRdisplay("recovery failure.\n");
	    TRdisplay("\t%d transaction not REDONE.\n", rdb->rdb_et_cnt);
	    TRdisplay("\t%d transaction not UNDONE.\n", rdb->rdb_bt_cnt);

	    rdb->rdb_status |= (RDB_ALREADY_INCONSISTENT | RDB_INVALID);
	    rdb->rdb_status &= ~(RDB_RECOVER | RDB_REDO | RDB_REDO_PARTIAL);

	    rcp->rcp_db_count--;
	    rcp->rcp_rdtx_count -= rdb->rdb_et_cnt;
	    /* nuke_rtx decrements rcp_tx_count */

	    /* 
	    ** Remove related rtx's
	    */
	    nuke_rtx(rcp, rdb);
	}

	/*
	** If the database is closed, skip recovery processing
	*/
	else if ( ! database_open)
	{
	    if (rcp->rcp_verbose)
	    {
		TRdisplay("\tDatabase %~t is Closed. No recovery needed.\n",
		    sizeof(DB_DB_NAME), &rdb->rdb_name);
	    }

	    /*
	    ** Check for transactions which we still have listed as unresolved
	    ** even though the database is marked closed.
	    **
	    ** Tranasactions which are listed as RTX_FOUND_ONLY_IN_CP are
	    ** presumed to have been completed, but that their ET records
	    ** fell before the CP from which we started recovery.  These
	    ** are silently ignored.
	    **
	    ** If we found actual transaction log records in the analysis
	    ** pass for a transaction which had no ET record then it is
	    ** unexpected to find the database closed.  We complain in
	    ** this case but continue processing (it may be the case that
	    ** the database was previously inconsistent but was patched
	    ** with verifydb?).
	    */
	    unexpected_xacts = 0;
	    st = rcp->rcp_tx_state.stq_next;
	    while (st != (RCP_QUEUE *)&rcp->rcp_tx_state.stq_next)
	    {
		rtx = (RTX *)((char *)st - CL_OFFSETOF(RTX, rtx_state));
		st = rtx->rtx_state.stq_next;

		if ((rtx->rtx_rdb == rdb) &&
		    ((rtx->rtx_status & RTX_FOUND_ONLY_IN_CP) == 0))
		{
		    unexpected_xacts++;
		    TRdisplay("\t    Warning: No ET found for Transaction %x%x.\n",
			rtx->rtx_tran_id.db_high_tran, 
			rtx->rtx_tran_id.db_low_tran);
		}
	    }

	    /*
	    ** If the database contains unresolved transactions, something
	    ** is wrong.  
	    */
	    if (unexpected_xacts)
	    {
		uleFormat(NULL, E_DM943F_RCP_TX_WO_DB, (CL_ERR_DESC *)NULL, ULE_LOG, 
		    NULL, (char *)NULL, (i4)0, (i4 *)NULL, &error, 2,
		    sizeof(DB_DB_NAME), &rdb->rdb_name, 0, rdb->rdb_bt_cnt);
	    }

	    rdb->rdb_status &= ~(RDB_RECOVER | RDB_REDO | RDB_REDO_PARTIAL);
	    rcp->rcp_db_count--;

	    /* 
	    ** Remove related rtx's
	    */
	    nuke_rtx(rcp, rdb);
	}

	/*
	** Close the config file.
	*/
	status = dm0c_close(cnf, (i4) 0, &rcp->rcp_dberr);
	if (status != E_DB_OK)
	    break;

	return (E_DB_OK);
    }

    /*
    ** Error handling:
    **
    */
    uleFormat(&rcp->rcp_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
	(char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
    SETDBERR(&rcp->rcp_dberr, 0, E_DM942F_RCP_CHECK_RECOVERY);

    return (E_DB_ERROR);
}

/*{
** Name: close_config - Update config file to mark database closed 
**			(or inconsistent)
**
** Description:
**
** Inputs:
**	rcp			Recovery Context
**	rdb			The particular database whose recovery is done.
**
** Outputs:
**	err_code		Reason for error return.
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      14-dec-1992 (rogerk)
**	    Reduced Logging Project: Written.
**	26-apr-1993 (bryanp)
**	    6.5 Cluster Support:
**		Get the node ID from the rdb structure, rather than using global
**		    variables, so that we set/reset the correct bit in the
**		    open_count bitmask.
**	31-jan-1994 (rogerk)
**	    Changed to return OK when RDB_ALREADY_INCONSISTENT rather than
**	    passing no return value.
*/
static DB_STATUS
close_config(
RCP		*rcp,
RDB		*rdb)
{
    DMP_DCB	    local_dcb;
    DM0C_CNF	    *cnf = 0;
    DB_STATUS	    status = E_DB_OK;
    i4	    open_mask;
    i4		    error;

    CLRDBERR(&rcp->rcp_dberr);

    for (;;)
    {
	/*
	** If the database was already marked inconsistent, then there is
	** no work to do here.
	*/
	if (rdb->rdb_status & RDB_ALREADY_INCONSISTENT)
	    return (E_DB_OK);

	/*
	** Format a local DCB to use for the config file update since we
	** will not have always succeeded in opening the database when we
	** get to here.
	*/
	MEfill(sizeof(local_dcb), 0, (char *)&local_dcb);
	MEmove(sizeof(rdb->rdb_name), (PTR)&rdb->rdb_name, ' ',
		sizeof(local_dcb.dcb_name), (PTR)&local_dcb.dcb_name);
	MEmove(sizeof(rdb->rdb_owner), (PTR)&rdb->rdb_owner, ' ',
		sizeof(local_dcb.dcb_db_owner), (PTR)&local_dcb.dcb_db_owner);
	MEmove(rdb->rdb_l_root, (PTR)&rdb->rdb_root, ' ',
		sizeof(local_dcb.dcb_location.physical), 
		(PTR)&local_dcb.dcb_location.physical);
	local_dcb.dcb_location.phys_length = rdb->rdb_l_root;
	local_dcb.dcb_access_mode = DCB_A_WRITE;
	local_dcb.dcb_served = DCB_MULTIPLE;
	local_dcb.dcb_db_type = DCB_PUBLIC;
	local_dcb.dcb_status = 0;
	    
	/*
	** Open the config file
	*/
	status = dm0c_open(&local_dcb, DM0C_PARTIAL, dmf_svcb->svcb_lock_list, 
			    &cnf, &rcp->rcp_dberr);
	if (status != E_DB_OK)
	    break;

	/*
	** Mark the database closed now that recovery is complete.
	*/
	open_mask = 1 << rdb->rdb_node_id;
	cnf->cnf_dsc->dsc_open_count &= ~open_mask;

	/*
	** If recovery failed on this database, then mark the database
	** inconsistent.  Include the reason for the inconsistency.
	*/
	if (rdb->rdb_status & (RDB_INVALID | RDB_TABLE_RECOVERY_ERROR))
	{
	    if (rdb->rdb_status & RDB_INVALID)
	    {
		uleFormat(NULL, E_DM943B_RCP_DBINCONSISTENT, (CL_ERR_DESC *)NULL,  
		    ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL, &error, 2,
		    sizeof(DB_DB_NAME), &rdb->rdb_name,
		    sizeof(DB_OWN_NAME), &rdb->rdb_owner);
	    }
	    else
	    {
		uleFormat(NULL, E_DM944A_RCP_DBINCONSISTENT, (CL_ERR_DESC *)NULL,  
		    ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL, &error, 2,
		    sizeof(DB_DB_NAME), &rdb->rdb_name,
		    sizeof(DB_OWN_NAME), &rdb->rdb_owner);
	    }

	    cnf->cnf_dsc->dsc_status &= ~DSC_VALID;
	    cnf->cnf_dsc->dsc_inconst_time = TMsecs();
	    cnf->cnf_dsc->dsc_inconst_code = rdb->rdb_inconst_code;

	    /*
	    ** If inconsistency was caused by NOLOGGING transaction, turn
	    ** of the nologging bit in the config file now.
	    */
	    if (cnf->cnf_dsc->dsc_status & DSC_NOLOGGING)
                cnf->cnf_dsc->dsc_status &= ~DSC_NOLOGGING;
	}

	status = dm0c_close(cnf, DM0C_UPDATE, &rcp->rcp_dberr);
	if (status != E_DB_OK)
	    break;

	return (E_DB_OK);
    }

    uleFormat(&rcp->rcp_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
	(char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
    SETDBERR(&rcp->rcp_dberr, 0, E_DM9438_RCP_CLOSE_CONFIG);
    return (E_DB_ERROR);
}

/*{
** Name: mark_invalid - Mark database invalid in recovery context
**
** Description:
**
** Inputs:
**	rcp			Recovery Context
**
** Outputs:
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      14-dec-1992 (rogerk)
**	    Reduced Logging Project: Written.
*/
static VOID
mark_invalid(
RCP		*rcp,
RDB		*rdb,
i4		reason)
{
    rdb->rdb_status |= RDB_INVALID;
    rdb->rdb_status &= ~(RDB_RECOVER | RDB_REDO | 
				RDB_REDO_PARTIAL | RDB_WILLING_COMMIT);
    rdb->rdb_inconst_code = reason;
}

/*{
** Name: apply_redo - Apply REDO log record to the database.
**
** Description:
**
** Inputs:
**	rcp			Recovery Context
**
** Outputs:
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      14-dec-1992 (rogerk)
**	    Reduced Logging Project: Written.
**	15-mar-1993 (jnash & rogerk)
**	    Reduced Logging - Phase IV:
**	    Set new dmve_logging flag to request logging during UNDO and REDO.
**	    Changed to take Log Address parameter, and extract lsn from record.
**	26-apr-1993 (bryanp)
**	    Cluster project support.
**	26-jul-1993 (bryanp)
**	    Created routine 'trace_record' to consolidate calls which output
**	    log records and their header info to the trace file.
**	20-sep-1993 (rogerk)
**	    Initialize dmve_rolldb_action list field.
**	23-feb-2004 (thaju02)
**	    Initialize dmve_flags.
**	5-Apr-2004 (schka24)
**	    Init raw-data pointer fields.
**	11-Oct-2007 (jonj)
**	    Notify dmve if CSP recovery.
*/
static DB_STATUS
apply_redo(
RCP		*rcp,
DM0L_HEADER	*record,
LG_LA		*la,
DMP_DCB		*dcb,
i4		lock_list)
{
    DB_STATUS	    status;
    DMVE_CB	    *dmve = (DMVE_CB *)rcp->rcp_dmve_cb;
    i4		    i;
    EX_CONTEXT	    context;
    i4		    error;

    CLRDBERR(&rcp->rcp_dberr);

    /*
    ** Declare exception handler before the redo action
    ** When an exception occurs, execution will continue here.
    */
    if (EXdeclare(ex_handler, &context) != OK)
    {
	EXdelete();
	SETDBERR(&rcp->rcp_dberr, 0, E_DM004A_INTERNAL_ERROR);
	return (E_DB_ERROR);
    }

    /*
    ** Format the DMVE control block for the redo action.
    */
    dmve->dmve_type = DMVECB_CB;
    dmve->dmve_length = sizeof(DMVE_CB);
    dmve->dmve_ascii_id = DMVE_ASCII_ID;
    dmve->dmve_next = 0;
    dmve->dmve_prev = 0;
    dmve->dmve_owner = 0;
    dmve->dmve_action = DMVE_REDO;
    dmve->dmve_logging = TRUE;
    dmve->dmve_tbio = NULL;
    for (i = 0; i < DM_RAWD_TYPES; ++i)
    {
	dmve->dmve_rawdata[i] = NULL;
	dmve->dmve_rawdata_size[i] = 0;
    }

    STRUCT_ASSIGN_MACRO(rcp_tran_id, dmve->dmve_tran_id);
    dmve->dmve_dcb_ptr = dcb;
    dmve->dmve_lk_id = lock_list;
    dmve->dmve_log_id = 0;
    dmve->dmve_db_lockmode = 0;
    dmve->dmve_rolldb_action = NULL;

    if ( rcp->rcp_cluster_enabled )
    {
	/* Tell dmve it's CSP doing recovery */
	dmve->dmve_flags = DMVE_CSP;
    }
    else
	dmve->dmve_flags = 0;

    dmve->dmve_lg_addr = *la;
    dmve->dmve_log_rec = (PTR) record;

    /*
    ** Call recovery processing for this record.
    */
    status = dmve_redo(dmve);
    if (status != E_DB_OK)
    {
	uleFormat(&dmve->dmve_error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
	    (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	SETDBERR(&rcp->rcp_dberr, 0, E_DM9439_APPLY_REDO);
    }

    EXdelete();
    return (status);
}

/*{
** Name: apply_undo - Apply UNDO log record to the database.
**
** Description:
**
** Inputs:
**	rcp			Recovery Context
**
** Outputs:
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      14-dec-1992 (rogerk)
**	    Reduced Logging Project: Written.
**	15-mar-1993 (rogerk & jnash)
**	    Reduced Logging - Phase IV:
**	    Set new dmve_logging flag to request logging during UNDO and REDO.
**	    Changed to take Log Address parameter, and extract lsn from record.
**	26-apr-1993 (bryanp)
**	    Cluster support: added lsn parameters.
**	26-jul-1993 (bryanp)
**	    Created routine 'trace_record' to consolidate calls which output
**	    log records and their header info to the trace file.
**	20-sep-1993 (rogerk)
**	    Initialize dmve_rolldb_action list field.
**	15-apr-1994 (chiku)
**	    Bug56702: initialize the err_data field.
**		      return dead logfull indication.
**	23-feb-2004 (thaju02)
**	    Initialize dmve_flags.
**	5-Apr-2004 (schka24)
**	    Init raw-data pointer fields.
**	11-Oct-2007 (jonj)
**	    Notify dmve if CSP recovery.
**	17-Nov-2008 (jonj)
**	    Use dmve_logfull instead of dmve_error.err_data to preserve
**	    E_DM9070_LOG_LOGFULL for Bug 56702.
*/
static DB_STATUS
apply_undo(
RCP		*rcp,
RTX		*rtx,
DM0L_HEADER	*record,
LG_LA		*la)
{
    DB_STATUS	    save_status;
    DMVE_CB	    *dmve = (DMVE_CB *)rcp->rcp_dmve_cb;
    i4		    i;
    EX_CONTEXT	    context;
    i4		    error;

    CLRDBERR(&rcp->rcp_dberr);

    /*
    ** Declare exception handler before the undo action
    ** When an exception occurs, execution will continue here.
    */
    if (EXdeclare(ex_handler, &context) != OK)
    {
	EXdelete();
	SETDBERR(&rcp->rcp_dberr, 0, E_DM004A_INTERNAL_ERROR);
	return (E_DB_ERROR);
    }

    /*
    ** Format the DMVE control block for the redo action.
    */
    dmve->dmve_type = DMVECB_CB;
    dmve->dmve_length = sizeof(DMVE_CB);
    dmve->dmve_ascii_id = DMVE_ASCII_ID;
    dmve->dmve_next = 0;
    dmve->dmve_prev = 0;
    dmve->dmve_owner = 0;
    dmve->dmve_action = DMVE_UNDO;
    dmve->dmve_logging = TRUE;
    dmve->dmve_tbio = NULL;
    for (i = 0; i < DM_RAWD_TYPES; ++i)
    {
	dmve->dmve_rawdata[i] = NULL;
	dmve->dmve_rawdata_size[i] = 0;
    }

    STRUCT_ASSIGN_MACRO(rtx->rtx_tran_id, dmve->dmve_tran_id);
    dmve->dmve_dcb_ptr = rtx->rtx_rdb->rdb_dcb;
    dmve->dmve_lk_id = rtx->rtx_ll_id;
    dmve->dmve_log_id = rtx->rtx_id;
    dmve->dmve_db_lockmode = 0;
    dmve->dmve_rolldb_action = NULL;

    if ( rcp->rcp_cluster_enabled )
    {
	/* Tell dmve it's CSP doing recovery */
	dmve->dmve_flags = DMVE_CSP;
    }
    else
	dmve->dmve_flags = 0;

    dmve->dmve_lg_addr = *la;
    dmve->dmve_log_rec = (PTR) record;

    /*
    ** Mark recovery processing as having started for this transaction.
    */
    rtx->rtx_status |= RTX_ABORT_INPROGRESS;

    /*
    ** Call recovery processing for this record.
    ** Save the return status for processing below.
    */
    save_status = dmve_undo(dmve);

    if (save_status != E_DB_OK)
    {
	uleFormat(&dmve->dmve_error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
	    (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	/*
	 * Bug56702: return dead logfull indication.
	 */
	if (dmve->dmve_logfull == E_DM9070_LOG_LOGFULL )
		SETDBERR(&rcp->rcp_dberr, 0, E_DM9070_LOG_LOGFULL);
	else
		SETDBERR(&rcp->rcp_dberr, 0, E_DM943A_APPLY_UNDO);
    }

    EXdelete();
    return (save_status);
}

/*{
** Name: find_next_abort_rtx - Find next record to abort in Undo phase.
**
** Description:
**	This routine selects the next transaction whose record should be undone.
**	The selection algorithm is to choose the transaction whose next log
**	record has the greatest Log Sequence Number. This ensures that the undo
**	processing is performed in descending order by LSN. In a multi-log
**	system such as the VAXCluster, the transactions are potentially from
**	several different log files, and so the UNDO processing may jump from
**	logfile to logfile. However, the Log Sequence Numbers provide a single
**	unified ordering for the log records which matches the order in which
**	these operations were originally performed.
**
**	In a single node logging system, the log records are placed in the same
**	order by LSN as they are by Log Address, but in the multi-node logging
**	systems there is no single ordering by Log Address, since log records
**	from two different log files cannot by compared by Log Address, only by
**	Log Sequence Number.
**
** Inputs:
**	rcp			Recovery Context
**
** Outputs:
**	return_tx		RTX of transaction with next record to abort.
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      14-dec-1992 (rogerk)
**	    Reduced Logging Project: Written.
**	15-mar-1993 (bryanp)
**	    Cluster support: distinguish between lga and lsn. Importantly, this
**	    routine now drives undo-by-lsn, rather than undo-by-lga, which is
**	    a fundamental aspect of cluster recovery.
*/
static VOID
find_next_abort_rtx(
RCP		*rcp,
RTX		**return_tx)
{
    RCP_QUEUE	    *st;
    RTX		    *tx;

    *return_tx = NULL;

    for (st = rcp->rcp_tx_state.stq_next;
	st != (RCP_QUEUE *)&rcp->rcp_tx_state.stq_next;
	st = tx->rtx_state.stq_next)
    {
	tx = (RTX *)((char *)st - CL_OFFSETOF(RTX, rtx_state));

	/*
	** Skip transactions not being recovered.
	*/
	if ((tx->rtx_status & RTX_COMPLETE) ||
	    (tx->rtx_rdb->rdb_status & RDB_INVALID))
	    continue;

	if ((*return_tx == NULL) ||
	    (LSN_GT(&tx->rtx_last_lsn, &((*return_tx)->rtx_last_lsn))))
	{
	    *return_tx = tx;
	}
    }

    return;
}

/*{
** Name: read_first_log_record		- read each transaction's first logrec.
**
** Description:
**	Scan through the list of abort transactions and read the last record
**	for each transaction. This "primes the pump" of the undo algorithm;
**	subsequent processing will locate the log record with the highest LSN,
**	undo that log record, and then read the next (previous) log record for
**	that transaction. This priming of the pump is necessary because log
**	records do not contain a "prev_lsn" field in general; only CLR records
**	contain this field.
**
**	In a single node logging system, the log records are placed in the same
**	order by LSN as they are by Log Address, but in the multi-node logging
**	systems there is no single ordering by Log Address, since log records
**	from two different log files cannot by compared by Log Address, only by
**	Log Sequence Number.
**
** Inputs:
**	rcp			Recovery Context
**
** Outputs:
**	err_code		Reason for error return.
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	26-apr-1993 (bryanp)
**	    Written as part of 6.5 cluster support.
*/
static DB_STATUS
read_first_log_record(
RCP		*rcp)
{
    RCP_QUEUE	    *st;
    RTX		    *tx;
    DB_STATUS	    status = E_DB_OK;
    i4		    error;

    CLRDBERR(&rcp->rcp_dberr);

    for (st = rcp->rcp_tx_state.stq_next;
	st != (RCP_QUEUE *)&rcp->rcp_tx_state.stq_next;
	st = tx->rtx_state.stq_next)
    {
	tx = (RTX *)((char *)st - CL_OFFSETOF(RTX, rtx_state));

	/*
	** Skip transactions not being recovered.
	*/
	if ((tx->rtx_status & RTX_COMPLETE) ||
	    (tx->rtx_rdb->rdb_status & RDB_INVALID))
	    continue;

	status = read_next_log_record(rcp, tx);
	if (status)
	{
	    /* ERROR PROCESSING - error message - dump xacts - give reason */
	    uleFormat(&rcp->rcp_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
			(char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	    SETDBERR(&rcp->rcp_dberr, 0, E_DM9437_RCP_P3);
	    /* *err_code = XXX Need new message */
	    break;
	}
    }

    return (status);
}

/*
** Name: read_next_log_record	    - read the next log record for this xact.
**
** Description:
**	Given a particular transaction, reads the transaction's last log record
**	into the LG_RECORD and dm0l fields in the rtx. Updates the various
**	rtx fields to point to the next (previous) record.
**
** Inputs:
**	rcp			Recovery Context
**	rtx			Transaction to read.
**
** Outputs:
**	err_code		Reason for error return.
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	26-apr-1993 (bryanp)
**	    Written for Cluster support.
**		Enable use of trace point 1307 to echo LSNs during
**		    recovery passes.
**		Pass correct transaction handle to dm0l_read.
**	21-jun-1993 (bryanp)
**	    Save this log record's log address in rtx_lg_rec_la for tracing.
**	06-mar-1996 (stial01 for bryanp)
**	    Use dm0m_lcopy to manipulate the dm0l_record field in the RCP, 
**          since it may exceed 64K in length.
*/
static DB_STATUS
read_next_log_record(RCP *rcp, RTX *rtx)
{
    DB_STATUS		status = E_DB_OK;
    LG_LA		la;
    LG_LSN		lsn;
    DMP_LCTX		*lctx;
    LG_RECORD		*record_header;
    DM0L_HEADER		*record;
    i4			error;

    CLRDBERR(&rcp->rcp_dberr);

    for (;;)
    {
	/*
	** Position to and then read the indicated log record.
	*/
	STRUCT_ASSIGN_MACRO(rtx->rtx_last_lga, la);
	lctx = rtx->rtx_lctx;

	status = dm0l_position(DM0L_BY_LGA, rtx->rtx_id, &la,
				lctx, &rcp->rcp_dberr);
	if (status != E_DB_OK)
	{
	    /* ERROR PROCESSING - error message - dump xacts - give reason */
	    uleFormat(&rcp->rcp_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
			(char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	    SETDBERR(&rcp->rcp_dberr, 0, E_DM9437_RCP_P3);
	    /* *err_code = XXX Need new message */
	    break;
	}

	status = dm0l_read(lctx, rtx->rtx_id, &record, &la,
				(i4*)NULL, &rcp->rcp_dberr);
	if (status != E_DB_OK)
	{
	    /* ERROR PROCESSING - error message - dump xacts - give reason */
	    uleFormat(&rcp->rcp_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
			(char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	    SETDBERR(&rcp->rcp_dberr, 0, E_DM9437_RCP_P3);
	    /* *err_code = XXX Need new message */
	    break;
	}

	record_header = (LG_RECORD *) lctx->lctx_header;

#ifdef xDEBUG
	if (DMZ_ASY_MACRO(7))
	    TRdisplay("\tUndo LSN: (%x,%x)\n",
			record->lsn.lsn_high, record->lsn.lsn_low);
#endif

	/*
	** Consistency Check:
	**
	**   - Verify that the log record read matches the transaction we
	**     think we are aborting.
	*/
	if ((record->tran_id.db_low_tran != rtx->rtx_tran_id.db_low_tran) ||
	    (record->tran_id.db_high_tran != rtx->rtx_tran_id.db_high_tran))
	{
	    /* XXXXX error message */
	    TRdisplay("%@ RCP-P3: Undo Phase transaction id mismatch:\n");
	    SETDBERR(&rcp->rcp_dberr, 0, E_DM9437_RCP_P3);
	    status = E_DB_ERROR;
	    break;
	}

	/*
	** Set the RTX log address for the next log record to process for
	** this transaction. 
	*/
	rtx->rtx_last_lga.la_sequence = record_header->lgr_prev.la_sequence;
	rtx->rtx_last_lga.la_block    = record_header->lgr_prev.la_block;
	rtx->rtx_last_lga.la_offset   = record_header->lgr_prev.la_offset;

	rtx->rtx_last_lsn = record->lsn;

	rtx->rtx_lg_rec_la = la;

	/*
	** Stash this record away into the appropriate points in the rtx.
	*/
	MEcopy((PTR)record_header, sizeof(*record_header),
		(PTR)&rtx->rtx_lg_record);
	MEcopy((PTR)record, *(i4 *)record, (PTR)rtx->rtx_dm0l_record);

	break;
    }

    return (status);
}

/*
** recover_willing_commit - Recovered the willing commit transaction.
**
** Description:
**	This routine rebuilds the transaction context of the WILLING COMMIT
**	transaction in the logging system.
**
**	The caller of this routine must have read the last log record for
**	the willing commit transaction using the transaction context 'tx'.
**	This last log record must be labeled P1_LAST (the true last record
**	marking the willing commit state) and must be resident in the tx
**	log record buffer.
**
**	On exit of this routine, the transaction context will likely not
**	hold the same log record as it did at the start.  If the routine
**	returns an error, the log record it holds will be the record being
**	processed when the error occurred.
**
** Inputs:
**	rcp			Recovery Context
**	tx			Transaction Context
**
** Outputs:
**	    err_code			Error code.
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	18-Jan-1989 (ac)
**	    created.
**      28-Oct-91 (jnash)
**          Add "ignored" param to dm0l_opendb.
**	14-dec-92 (rogerk)
**	    Add formatting of LG and LK error statuses.
**	18-dec-93 (rogerk)
**	    Changed handling of re-establishment of willing commit
**	    transactions.  The LGadd and LGbegin calls to rebuild the xact
**	    context are no longer made here, but are done in dmr_init_recovery
**	    for ALL transactions.  Willing commit transactions are altered
**	    here, and then left around following the completion of normal
**	    recovery.
**	15-mar-1993 (bryanp)
**	    Just a history comment: This code does not work on the cluster, and
**	    I have not made any attempt to make it work on the cluster.
**		Pass correct transaction handle to dm0l_read.
**	18-oct-1993 (jnash & rogerk)
**	    Numerous fixes:
**	     - Replaced dm0l calls with read_next_log_record to utilize the tx
**	       memory context.  
**	     - Changed routine to assume that the first log record to process 
**	       is already resident in the tx structure, otherwise
**	       we start the recovery loop attempting to process the second P1
**	       record.  
**	     - Added rcp parameter to pass to read_next_log_record.
**	     - use the original last log address to pass to LGalter from
**	       rtx_orig_last_lga instead of the rtx_last_lga value which
**	       has been modified by reads in the undo pass.
**	     - Make LKreqeust() calls NOWAIT.  There is no reason that they
**	       should not succeed.  
**	9-Apr-2005 (schka24)
**	    Trace the P1's for posterity, when verbose.
**	14-Apr-2005 (schka24)
**	    jnash & rogerk forgot that | binds tighter than ?:, ended up
**	    making all restored locks PHYSICAL locks.  During a manual
**	    lartool abort, that caused page locks to be dropped too soon.
**	    If other transactions were active at the same time, they could
**	    grab some of those locks during the abort, causing deadlock,
**	    undo failure, and an inconsistent database.
*/
static DB_STATUS
recover_willing_commit(
RCP		    *rcp,
RTX		    *tx)
{
    RDB			*rdb = tx->rtx_rdb;
    DM0L_HEADER		*record;
    DM0L_P1		*p1;
    i4			i;
    i4			lkflag;
    LG_CONTEXT		ctx;
    LG_LA		la;
    LG_RECORD		*record_header;
    LK_LLID		lock_list = 0;
    LK_LLID		rel_lock_list = 0;
    LK_LOCK_KEY		lk_key;	
    DB_STATUS		status = E_DB_OK;
    STATUS		cl_status;
    CL_ERR_DESC		sys_err;
    i4			error;

    CLRDBERR(&rcp->rcp_dberr);

    /*
    ** Initialize local context with the log record currently in the tx
    ** context.  This record should have been read by undo pass routine
    ** and will be the P1_LAST log record.
    ** Caller has already traced this one, if verbose.
    */
    record = (DM0L_HEADER *)tx->rtx_dm0l_record;
    la = tx->rtx_lg_rec_la;

    for(;;)
    {
	p1 = (DM0L_P1 *) record;
	if (p1->p1_flag == P1_LAST)
	{
	    /*
	    ** Mark database as being in willing commit state so that we
	    ** don't close it when recovery processing is complete.
	    */
	    rdb->rdb_status |= RDB_WILLING_COMMIT;

	    /*
	    ** Alter the transaction context with information from the
	    ** P1 log record.  Most of this information has already been
	    ** stored in the transaction context (and presumably should
	    ** match the information from the CP or BT log record when
	    ** the transaction was initialized), but the distributed
	    ** id was not known until now.
	    */
	    ctx.lg_ctx_id = tx->rtx_id;
	    STRUCT_ASSIGN_MACRO((*((DB_TRAN_ID *)&(p1->p1_tran_id))), 
								ctx.lg_ctx_eid);
	    STRUCT_ASSIGN_MACRO((*((LG_LA *)&(p1->p1_first_lga))), 
							    ctx.lg_ctx_first);
	    STRUCT_ASSIGN_MACRO((*((LG_LA *)&(tx->rtx_orig_last_lga))), 
							    ctx.lg_ctx_last);
	    STRUCT_ASSIGN_MACRO((*((LG_LA *)&(p1->p1_cp_lga))), ctx.lg_ctx_cp);
	    ctx.lg_ctx_dis_eid = p1->p1_dis_tran_id;
 
	    cl_status = LGalter(LG_A_CONTEXT, (PTR)&ctx, sizeof(ctx), &sys_err);
	    if (cl_status != OK)
	    {
		uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
		uleFormat(NULL, E_DM900B_BAD_LOG_ALTER, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, &error, 1,
		    0, LG_A_CONTEXT);
		SETDBERR(&rcp->rcp_dberr, 0, E_DM9421_RECOVER_WILLING_COMMIT);
		status = E_DB_ERROR;
		break;
	    }

	    /*
	    ** Set transaction in Willing Commit State.
	    */
	    cl_status = LGalter(LG_A_WILLING_COMMIT, (PTR)&ctx, sizeof(ctx), 
		&sys_err);
	    if (cl_status != OK)
	    {
		uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
		uleFormat(NULL, E_DM900B_BAD_LOG_ALTER, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, &error, 1,
		    0, LG_A_WILLING_COMMIT);
		SETDBERR(&rcp->rcp_dberr, 0, E_DM9421_RECOVER_WILLING_COMMIT);
		status = E_DB_ERROR;
		break;
	    }

	    /* Create a session lock list for the transaction. */

	    cl_status = LKcreate_list(
		    (LK_ASSIGN | LK_NONPROTECT | LK_NOINTERRUPT),
		    (LK_LLID)0, (LK_UNIQUE *)0, (LK_LLID *)&rel_lock_list, 
		    0, &sys_err);
	    if (cl_status != OK)
	    {
		uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
		SETDBERR(&rcp->rcp_dberr, 0, E_DM901A_BAD_LOCK_CREATE);
		status = E_DB_ERROR;
		break;
	    }		    

	    /* Create a lock list for the transaction. */

	    cl_status = LKcreate_list((i4)0, (LK_LLID)rel_lock_list,
			(LK_UNIQUE*)&(ctx.lg_ctx_eid), 
			(LK_LLID *)&lock_list, 0, 
		        &sys_err);
	    if (cl_status != OK)
	    {
		uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
		SETDBERR(&rcp->rcp_dberr, 0, E_DM901A_BAD_LOCK_CREATE);
		status = E_DB_ERROR;
		break;
	    }		    
	}
	else if (p1->p1_flag == P1_RELATED)
	{
	    /* Request each lock in the log record on the related lock list. */

	    for (i = 0; i < p1->p1_count; i++)
	    {
		MEcopy((char *)p1->p1_type_lk[i].lkb_key, sizeof(lk_key), 
		    (char *)&lk_key);

		lkflag = LK_NOWAIT;
		if (p1->p1_type_lk[i].lkb_attribute & LOCK_PHYSICAL)
		    lkflag |= LK_PHYSICAL;
		cl_status = LKrequest(lkflag,
		    rel_lock_list, &lk_key, p1->p1_type_lk[i].lkb_grant_mode, 
		    (LK_VALUE *)0, (LK_LKID *)0, (i4)0, &sys_err);
		if (cl_status != OK)
		{
		    uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
			(char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
		    uleFormat(NULL, E_DM901C_BAD_LOCK_REQUEST, &sys_err, 
                        ULE_LOG, NULL, 0, 0, 0, &error, 2, 
			0, p1->p1_type_lk[i].lkb_grant_mode, 0, rel_lock_list);
		    SETDBERR(&rcp->rcp_dberr, 0, E_DM9421_RECOVER_WILLING_COMMIT);
		    status = E_DB_ERROR;
		    break;
		}
	    }
	}
	else
	{
	    /* Request each lock in the log record on the lock list. */

	    for (i = 0; i < p1->p1_count; i++)
	    {
		MEcopy((char *)p1->p1_type_lk[i].lkb_key, sizeof(lk_key), 
		    (char *)&lk_key);
		
		lkflag = LK_NOWAIT;
		if (p1->p1_type_lk[i].lkb_attribute & LOCK_PHYSICAL)
		    lkflag |= LK_PHYSICAL;
		cl_status = LKrequest(lkflag,
		    lock_list, &lk_key, p1->p1_type_lk[i].lkb_grant_mode, 
		    (LK_VALUE *)0, (LK_LKID *)0, (i4)0, &sys_err);
		if (cl_status != OK)
		{
		    uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
			(char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
		    uleFormat(NULL, E_DM901C_BAD_LOCK_REQUEST, &sys_err, 
                        ULE_LOG, NULL, 0, 0, 0, &error, 2, 
			0, p1->p1_type_lk[i].lkb_grant_mode, 0, lock_list);
		    SETDBERR(&rcp->rcp_dberr, 0, E_DM9421_RECOVER_WILLING_COMMIT);
		    status = E_DB_ERROR;
		    break;
		}
	    }
	}
	if (status != E_DB_OK)
	    break;

	/*
	** Fetch next log record for this transaction into the tx context.
	*/
	status = read_next_log_record(rcp, tx);
	if (status)
	    break;

	if (record->type != DM0LP1)
	    break;

	/* Init local context from record just read, and trace it. */
	record_header = &tx->rtx_lg_record;
	record = (DM0L_HEADER *)tx->rtx_dm0l_record;
	la = tx->rtx_lg_rec_la;
	if (rcp->rcp_verbose)
	    trace_record(record_header, record, &la, tx->rtx_lctx->lctx_bksz);
    }

    if (status != E_DB_OK)
    {
	/* Clean up the work we have done so far. */

	if (lock_list)
	{
	    cl_status = LKrelease(LK_ALL | LK_RELATED, lock_list, 
			(LK_LKID *)0, 
			(LK_LOCK_KEY *)0, 
			(LK_VALUE *)0, &sys_err);
	    if (cl_status != OK)
	    {
		uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
		uleFormat(NULL, E_DM901B_BAD_LOCK_RELEASE, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, &error, 1,
		    0, lock_list);
	    }
	}
	else if (rel_lock_list)
	{
	    cl_status = LKrelease(LK_ALL, rel_lock_list, 
			(LK_LKID *)0, 
			(LK_LOCK_KEY *)0, 
			(LK_VALUE *)0, &sys_err);
	    if (cl_status != OK)
	    {
		uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
		uleFormat(NULL, E_DM901B_BAD_LOCK_RELEASE, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, &error, 1,
		    0, rel_lock_list);
	    }
	}

	uleFormat(&rcp->rcp_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
	    (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	SETDBERR(&rcp->rcp_dberr, 0, E_DM9421_RECOVER_WILLING_COMMIT);
    }

    return(status);
}

/*{
** Name: lookup_rdb
**
** Description:
**	Locate a particular RDB structure, given its database ID and node ID.
**
**	In a single node logging system, there is only one RDB for a particular
**	database, but in a multi-node logging system we must manage the
**	recovery of a particular database in multiple logfiles simultaneously,
**	and so we have an RDB per database per node.
**
** Inputs:
**	rcp			Recovery Context
**	dbid			database ID
**	node_id			node ID
**
** Outputs:
**	return_db		Set to point to the correct RDB
**				If no RDB is found, this is set to NULL.
**	Returns:
**	    VOID
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      14-dec-1992 (rogerk)
**	    Reduced Logging Project: Written.
**	15-mar-1993 (rogerk)
**	    Reduced Logging - Phase IV:
**	    Changed RCP to track databases by their external dbid's rather
**		than the internal LG id.  Removed checking for reused
**		database ids as this is now impossible.
**	26-apr-1993 (bryanp)
**	    6.5 Cluster Support:
**		Added node_id parameter to allow the caller to specify on which
**		    node we want the RDB.
**		Changed lookup code to look for a match on both the db_id AND
**		    the node ID.
*/
static VOID
lookup_rdb(
RCP		*rcp,
i4		dbid,
i4		node_id,
RDB		**return_db)
{
    RCP_QUEUE	    *st;
    RDB		    *db;

    *return_db = NULL;

    /*
    ** If lookup by exact ID then we can hash the ID to do a quick lookup.
    */
    db = (RDB *) &rcp->rcp_rdb_map[dbid & RCP_DB_HASH];
    while (db = db->rdb_next)
    {
	if (db->rdb_ext_dbid == dbid && db->rdb_node_id == node_id)
	{
	    *return_db = db;
	    return;
	}
    }

    return;
}

/*{
** Name: add_rdb
**
** Description:
**	Allocates and initializes an instance of an RDB structure describing
**	the specified database on the specified node. Note that in a multi-node
**	logging system there may actually be one RDB per database per node, so
**	proper use of the node ID is essential to use of the correct RDB.
**
** Inputs:
**	rcp			Recovery Context
**	lctx			Logging system context block for this DB
**	dbid			Database ID.
**	dbname			Database Name
**	dbowner			Database owner
**	path			Root location path information
**	path_length		Actual length of path information
**	internal_dbid		The ID by which the logging system knows thisDB
**	start_recover_addr	Our initial guess at where in the logfile we'll
**				    begin recovery for this database.
**	start_recover_lsn	Log Sequence Number of the log record located
**				    at start_recover_addr.
**	recover_flags		Flags indicating the type of recovery needed.
**
** Outputs:
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      14-dec-1992 (rogerk)
**	    Reduced Logging Project: Written.
**	26-apr-1993 (bryanp)
**	    Cluster support:
**		Added lctx parameter, used it to set rdb_lctx.
**		Track both start_recover_addr and start_recover_lsn.
**		Set rdb_node_id from lctx_node_id.
*/
static DB_STATUS
add_rdb(
RCP		*rcp,
DMP_LCTX	*lctx,
i4		dbid,
DB_DB_NAME	*dbname,
DB_OWN_NAME	*dbowner,
DM_PATH		*path,
i4		path_length,
i4		internal_dbid,
LG_LA		*start_recover_addr,
LG_LSN		*start_recover_lsn,
i4		recover_flags)
{
    DB_STATUS	    status;
    RDB		    *rdb;
    RDB		    **rdbq;
    i4		    error;

    CLRDBERR(&rcp->rcp_dberr);

    /*
    ** Allocate an RDB structure.
    */
    status = dm0m_allocate(sizeof(RDB), 0, RDB_CB, RDB_ASCII_ID, 
			(char *)rcp, (DM_OBJECT **)&rdb, &rcp->rcp_dberr);
    if (status != E_DB_OK)
    {
	uleFormat(&rcp->rcp_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
	    (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	SETDBERR(&rcp->rcp_dberr, 0, E_DM9408_RCP_P0); /* XXXX */
	return(status);
    }

    rdb->rdb_dcb = (DMP_DCB *)0;
    rdb->rdb_ext_dbid = dbid;
    rdb->rdb_dbopen_id = 0;
    rdb->rdb_lg_dbid = internal_dbid;
    rdb->rdb_lctx = lctx;
    rdb->rdb_node_id = lctx->lctx_node_id;
    rdb->rdb_l_root = path_length;
    STRUCT_ASSIGN_MACRO(*dbname, rdb->rdb_name);
    STRUCT_ASSIGN_MACRO(*dbowner, rdb->rdb_owner);
    STRUCT_ASSIGN_MACRO(*path, rdb->rdb_root);
    STRUCT_ASSIGN_MACRO(*start_recover_addr, rdb->rdb_first_lga);
    STRUCT_ASSIGN_MACRO(*start_recover_lsn,  rdb->rdb_first_lsn);

    rdb->rdb_refcnt = 1;
    rdb->rdb_et_cnt = 0;
    rdb->rdb_bt_cnt = 0;
    rdb->rdb_status = recover_flags;

    rdb->rdb_tbl_invalid.stq_next = (RCP_QUEUE *)&rdb->rdb_tbl_invalid.stq_next;
    rdb->rdb_tbl_invalid.stq_prev = (RCP_QUEUE *)&rdb->rdb_tbl_invalid.stq_next;

    /*
    ** Put RDB on hash bucket queue.
    */
    rdbq = &rcp->rcp_rdb_map[dbid & RCP_DB_HASH];
    rdb->rdb_next = *rdbq;
    rdb->rdb_prev = (RDB*) rdbq;
    if (*rdbq)
	(*rdbq)->rdb_prev = rdb;
    *rdbq = rdb;

    /*
    ** Insert RDB on active queue.
    */
    rdb->rdb_state.stq_next = rcp->rcp_db_state.stq_next;
    rdb->rdb_state.stq_prev = (RCP_QUEUE *)&rcp->rcp_db_state.stq_next;
    rcp->rcp_db_state.stq_next->stq_prev = &rdb->rdb_state;
    rcp->rcp_db_state.stq_next = &rdb->rdb_state;

    rcp->rcp_db_count++;

    return (E_DB_OK);
}

/*{
** Name: remove_rdb
**
** Description:
**
** Inputs:
**	rcp			Recovery Context
**
** Outputs:
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      14-dec-1992 (rogerk)
**	    Reduced Logging Project: Written.
*/
static VOID
remove_rdb(RDB	*rdb)
{
    /*
    ** Remove RDB from the active state queue.
    */
    rdb->rdb_state.stq_next->stq_prev = rdb->rdb_state.stq_prev;
    rdb->rdb_state.stq_prev->stq_next = rdb->rdb_state.stq_next;

    /*
    ** Remove from its hash bucket queue.
    */
    if (rdb->rdb_next)
	rdb->rdb_next->rdb_prev = rdb->rdb_prev;
    rdb->rdb_prev->rdb_next = rdb->rdb_next;

    /*
    ** Deallocate the control block.
    */
    dm0m_deallocate((DM_OBJECT **)&rdb);
}

/*{
** Name: nuke_rdb
**
** Description:
**
** Inputs:
**	rcp			Recovery Context
**
** Outputs:
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      14-dec-1992 (rogerk)
**	    Reduced Logging Project: Written.
*/
static VOID
nuke_rdb(
RCP		*rcp)
{
    RCP_QUEUE	    *st;
    RDB		    *rdb;

    while ((st = rcp->rcp_db_state.stq_next) != 
			(RCP_QUEUE *) &rcp->rcp_db_state.stq_next)
    {
	rdb = (RDB *)((char *)st - CL_OFFSETOF(RDB, rdb_state));

	rcp->rcp_db_count--;
	remove_rdb(rdb);
    }

    return;
}
/*{
** Name: lookup_rtx
**
** Description:
**
** Inputs:
**	rcp			Recovery Context
**
** Outputs:
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      14-dec-1992 (rogerk)
**	    Reduced Logging Project: Written.
*/
static VOID
lookup_rtx(
RCP		*rcp,
DB_TRAN_ID	*tran_id,
RTX		**return_tx)
{
    RTX		    *tx;

    *return_tx = NULL;

    tx = (RTX *) &rcp->rcp_txh[tran_id->db_low_tran % RCP_TX_HASH];

    while (tx = tx->rtx_next)
    {
	if ((tx->rtx_tran_id.db_low_tran == tran_id->db_low_tran) &&
	    (tx->rtx_tran_id.db_high_tran == tran_id->db_high_tran))
	{
	    *return_tx = tx;
	    return;
	}
    }

    return;
}

/*{
** Name: add_rtx
**
** Description:
**
** Inputs:
**	rcp			Recovery Context
**	lctx			Logfile context block to use.
**	tran_id			Transaction ID
**	lx_id			Internal TX identifier
**	first_lga		First log record written by TX.
**	last_lga		Last log record written by TX
**	cp_lga			Address of CP previous to BT record.
**	rdb			Database information
**
** Outputs:
**	err_code		Failure status
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      14-dec-1992 (rogerk)
**	    Reduced Logging Project: Written.
**	15-mar-1993 (rogerk)
**	    Reduced Logging - Phase IV:
**	    Clarified use of LSN vs. Log Adress.  Log Reads return Log
**		Addresses.  The LSN of a record can be extracted from the
**		log record header.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**		Add LCTX parameter, store into rtx_lctx.
**	18-oct-1993 (rogerk)
**	    Added ownername parameter so transaction can be restored with
**	    its original owner.
**	15-Sep-2006 (jonj)
**	    Add (optional) ll_id input param.
*/
static DB_STATUS
add_rtx(
RCP		*rcp,
DMP_LCTX	*lctx,
DB_TRAN_ID	*tran_id,
LK_LLID		ll_id,
LG_LXID		lx_id,
LG_LA		*first_lga,
LG_LA		*last_lga,
LG_LA		*cp_lga,
RDB		*rdb,
char		*user_name)
{
    DB_STATUS	    status;
    RTX		    *rtx;
    RTX		    **rtxq;
    i4		    error;

    CLRDBERR(&rcp->rcp_dberr);

    /*
    ** Allocate an RTX structure.
    */
    status = dm0m_allocate(sizeof(RTX), 0, RTX_CB, RTX_ASCII_ID, 
			(char *)rcp, (DM_OBJECT **)&rtx, &rcp->rcp_dberr);
    if (status != E_DB_OK)
    {
	uleFormat(&rcp->rcp_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
	    (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	SETDBERR(&rcp->rcp_dberr, 0, E_DM9408_RCP_P0); /* XXXX */
	return(status);
    }

    /*
    ** Fill in the Transaction information.
    */

    STRUCT_ASSIGN_MACRO(*tran_id, rtx->rtx_tran_id);
    STRUCT_ASSIGN_MACRO(*last_lga, rtx->rtx_last_lga);
    STRUCT_ASSIGN_MACRO(*first_lga, rtx->rtx_first_lga);
    STRUCT_ASSIGN_MACRO(*cp_lga, rtx->rtx_cp_lga);
    MEcopy((PTR) user_name, DB_MAXNAME, (PTR) rtx->rtx_user_name.db_own_name);
    rtx->rtx_id = lx_id;
    rtx->rtx_rdb = rdb;
    rtx->rtx_recover_type = RTX_REDO_UNDO;
    rtx->rtx_status = 0;
    rtx->rtx_lctx = lctx;
    rtx->rtx_ll_id = ll_id;

    /*
    ** Put RTX on hash bucket queue.
    */
    rtxq = &rcp->rcp_txh[rtx->rtx_tran_id.db_low_tran % RCP_TX_HASH];
    rtx->rtx_next = *rtxq;
    rtx->rtx_prev = (RTX*) rtxq;
    if (*rtxq)
	(*rtxq)->rtx_prev = rtx;
    *rtxq = rtx;

    /*
    ** Insert RTX on active queue.
    */
    rtx->rtx_state.stq_next = rcp->rcp_tx_state.stq_next;
    rtx->rtx_state.stq_prev = (RCP_QUEUE *)&rcp->rcp_tx_state.stq_next;
    rcp->rcp_tx_state.stq_next->stq_prev = &rtx->rtx_state;
    rcp->rcp_tx_state.stq_next = &rtx->rtx_state;

    rcp->rcp_tx_count++;
    rdb->rdb_bt_cnt++;

    return (E_DB_OK);
}

/*{
** Name: remove_rtx
**
** Description:
**
** Inputs:
**	rcp			Recovery Context
**
** Outputs:
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      14-dec-1992 (rogerk)
**	    Reduced Logging Project: Written.
*/
static VOID
remove_rtx(RTX	*rtx)
{
    /*
    ** Remove RTX from the active state queue.
    */
    rtx->rtx_state.stq_next->stq_prev = rtx->rtx_state.stq_prev;
    rtx->rtx_state.stq_prev->stq_next = rtx->rtx_state.stq_next;

    /*
    ** Remove from its hash bucket queue.
    */
    if (rtx->rtx_next)
	rtx->rtx_next->rtx_prev = rtx->rtx_prev;
    rtx->rtx_prev->rtx_next = rtx->rtx_next;

    /*
    ** Deallocate the control block.
    */
    dm0m_deallocate((DM_OBJECT **)&rtx);
}

/*{
** Name: nuke_rtx
**
** Description:
**	Destroy all rtx's associated with specified rdb, or from all 
**	rdb's.
**
** Inputs:
**	rcp			Recovery Context
**
** Outputs:
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      14-dec-1992 (rogerk)
**	    Reduced Logging Project: Written.
*/
static VOID
nuke_rtx(
RCP		*rcp,
RDB		*rdb)
{
    RCP_QUEUE	    *st;
    RTX		    *rtx;

    st = rcp->rcp_tx_state.stq_next;
    while (st != (RCP_QUEUE *)&rcp->rcp_tx_state.stq_next)
    {
	rtx = (RTX *)((char *)st - CL_OFFSETOF(RTX, rtx_state));
	st = rtx->rtx_state.stq_next;

	if ((rdb == 0) || (rtx->rtx_rdb == rdb))
	{
	    rcp->rcp_tx_count--;
	    remove_rtx(rtx);
	}
    }

    return;
}

/*{
** Name: lookup_nonredo
**
** Description:
**
** Inputs:
**	rcp			Recovery Context
**	dbid			Database ID
**	tabid			Table base/index ID
**
** Outputs:
**	return_dmu		Nonredo entry if found, else NULL
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      14-dec-1992 (rogerk)
**	    Reduced Logging Project: Written.
**	25-Oct-2006 (kschendel) b122120
**	    Use hash table lookup to speed things up.
*/
static VOID
lookup_nonredo(
RCP		*rcp,
i4		dbid,
DB_TAB_ID	*tabid,
RDMU		**return_dmu)
{
    RCP_QUEUE	    *st;
    RDMU	    *rdmu;

    *return_dmu = NULL;

    /* Only hash on DB id and base id, so that index and base table
    ** entries are on the same hash chain.
    */

    rdmu = (RDMU *) &rcp->rcp_dmuh[(u_i4)(dbid + tabid->db_tab_base) % RCP_DMU_HASH];

    while (rdmu = rdmu->rdmu_next)
    {
	if ((rdmu->dbid == dbid) &&
	    (rdmu->tabid.db_tab_base == tabid->db_tab_base))
	{
	    /*
	    ** If found a match on the Base Table Id, check the index id.
	    ** A base table DMU satisfies any request for that table, even
	    ** if secondary index or partition.
	    ** A secondary index or partition DMU has to match the request
	    ** exactly;  do not return a secondary index DMU for a request
	    ** on a base table.  (These DMU's would only be logged for
	    ** operations specifically on that index or partition;  normally,
	    ** a base table DMU is logged that covers everything.)
	    **
	    ** New entries are added to the front of the applicable list,
	    ** so if both a matching base table DMU and a matching secondary 
	    ** index DMU exist, we'll find the one with the highest LSN.
	    */
	    if ((rdmu->tabid.db_tab_index == 0) ||
		(rdmu->tabid.db_tab_index == tabid->db_tab_index))
	    {
		*return_dmu = rdmu;
		return;
	    }
	}
    }

    return;
}

/*{
** Name: add_nonredo
**
** Description:
**
** Inputs:
**	rcp			Recovery Context
**	dbid			Database ID
**	tabid			Table base/index ID
**	lsn			Log sequence number of entry to add
**
** Outputs:
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      14-dec-1992 (rogerk)
**	    Reduced Logging Project: Written.
**	25-Oct-2006 (kschendel) b122120
**	    Use hash table lookup to speed things up.
*/
static DB_STATUS
add_nonredo(
RCP		*rcp,
i4		dbid,
DB_TAB_ID	*tabid,
LG_LSN		*lsn)
{
    DB_STATUS	    status;
    RDMU	    *rdmu;
    RDMU	    **rdmuq;
    i4		    error;

    CLRDBERR(&rcp->rcp_dberr);

    /*
    ** Allocate an RDMU structure.
    */
    status = dm0m_allocate(sizeof(RDMU), 0, RDMU_CB, RDMU_ASCII_ID, 
			(char *)rcp, (DM_OBJECT **)&rdmu, &rcp->rcp_dberr);
    if (status != E_DB_OK)
    {
	uleFormat(&rcp->rcp_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
	    (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	SETDBERR(&rcp->rcp_dberr, 0, E_DM9408_RCP_P0); /* XXXX */
	return(status);
    }

    rdmu->dbid = dbid;
    STRUCT_ASSIGN_MACRO(*tabid, rdmu->tabid);
    STRUCT_ASSIGN_MACRO(*lsn, rdmu->lsn);

    /*
    ** Put RDMU on hash bucket queue.
    */
    rdmuq = &rcp->rcp_dmuh[(u_i4)(dbid + tabid->db_tab_base) % RCP_DMU_HASH];
    rdmu->rdmu_next = *rdmuq;
    rdmu->rdmu_prev = (RDMU*) rdmuq;
    if (*rdmuq)
	(*rdmuq)->rdmu_prev = rdmu;
    *rdmuq = rdmu;

    /*
    ** Insert on Non-Redo Queue.
    */
    rdmu->rdmu_state.stq_next = rcp->rcp_rdmu_state.stq_next;
    rdmu->rdmu_state.stq_prev = (RCP_QUEUE *)&rcp->rcp_rdmu_state.stq_next;
    rcp->rcp_rdmu_state.stq_next->stq_prev = &rdmu->rdmu_state;
    rcp->rcp_rdmu_state.stq_next = &rdmu->rdmu_state;

    return (E_DB_OK);
}

/*{
** Name: remove_nonredo
**
** Description:
**
** Inputs:
**	rcp			Recovery Context
**
** Outputs:
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      14-dec-1992 (rogerk)
**	    Reduced Logging Project: Written.
*/
static VOID
remove_nonredo(RDMU	*rdmu)
{
    /*
    ** Remove RTX from the active state queue.
    */
    rdmu->rdmu_state.stq_next->stq_prev = rdmu->rdmu_state.stq_prev;
    rdmu->rdmu_state.stq_prev->stq_next = rdmu->rdmu_state.stq_next;

    /*
    ** Remove from its hash bucket queue.
    */
    if (rdmu->rdmu_next)
	rdmu->rdmu_next->rdmu_prev = rdmu->rdmu_prev;
    rdmu->rdmu_prev->rdmu_next = rdmu->rdmu_next;

    /*
    ** Deallocate the control block.
    */
    dm0m_deallocate((DM_OBJECT **)&rdmu);
}

/*{
** Name: nuke_nonredo
**
** Description:
**
** Inputs:
**	rcp			Recovery Context
**
** Outputs:
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      14-dec-1992 (rogerk)
**	    Reduced Logging Project: Written.
*/
static VOID
nuke_nonredo(
RCP		*rcp)
{
    RCP_QUEUE	    *st;
    RDMU	    *rdmu;

    while ((st = rcp->rcp_rdmu_state.stq_next) != 
			(RCP_QUEUE *) &rcp->rcp_rdmu_state.stq_next)
    {
	rdmu = (RDMU *)((char *)st - CL_OFFSETOF(RDMU, rdmu_state));
	remove_nonredo(rdmu);
    }

    return;
}

/*{
** Name: lookup_rlp
**
** Description:
**
** Inputs:
**	rcp			Recovery Context
**
** Outputs:
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      14-dec-1992 (rogerk)
**	    Reduced Logging Project: Written.
*/
static VOID
lookup_rlp(
RCP		*rcp,
i4		process_id,
RLP		**return_lp)
{
    RLP		    *lp;

    *return_lp = NULL;

    lp = (RLP *) &rcp->rcp_lph[process_id % RCP_LP_HASH];
    while (lp = lp->rlp_next)
    {
	if (lp->rlp_lpb_id == process_id)
	{
	    *return_lp = lp;
	    return;
	}
    }

    return;
}

/*{
** Name: add_rlp 
**
** Description:
**
** Inputs:
**	rcp			Recovery Context
**
** Outputs:
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      14-dec-1992 (rogerk)
**	    Reduced Logging Project: Written.
**	15-may-1994 (rmuth)
**	    b59248, rename some variables as was confusing.
*/
static DB_STATUS
add_rlp(
RCP		*rcp,
i4		lg_process_id,
i4		real_process_id)
{
    DB_STATUS	    status;
    RLP		    *rlp;
    RLP		    **lpq;
    i4		    error;

    CLRDBERR(&rcp->rcp_dberr);

    /*
    ** Allocate an RLP structure.
    */
    status = dm0m_allocate(sizeof(RLP), 0, RLP_CB, RLP_ASCII_ID, 
			(char *)rcp, (DM_OBJECT **)&rlp, &rcp->rcp_dberr);
    if (status != E_DB_OK)
    {
	uleFormat(&rcp->rcp_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
	    (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	SETDBERR(&rcp->rcp_dberr, 0, E_DM9408_RCP_P0); /* XXXX */
	return(status);
    }

    rlp->rlp_lpb_id = lg_process_id;
    rlp->rlp_pid = real_process_id;	    
    rlp->rlp_status = 0;

    /*
    ** Put RLP on hash bucket queue.
    */
    lpq = &rcp->rcp_lph[lg_process_id % RCP_LP_HASH];
    rlp->rlp_next = *lpq;
    rlp->rlp_prev = (RLP*) lpq;
    if (*lpq)
	(*lpq)->rlp_prev = rlp;
    *lpq = rlp;

    return (E_DB_OK);
}

/*{
** Name: remove_rlp
**
** Description:
**
** Inputs:
**	rcp			Recovery Context
**
** Outputs:
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      14-dec-1992 (rogerk)
**	    Reduced Logging Project: Written.
*/
static VOID
remove_rlp(RLP	*rlp)
{
    /*
    ** Remove from its hash bucket queue.
    */
    if (rlp->rlp_next)
	rlp->rlp_next->rlp_prev = rlp->rlp_prev;
    rlp->rlp_prev->rlp_next = rlp->rlp_next;

    /*
    ** Deallocate the control block.
    */
    dm0m_deallocate((DM_OBJECT **)&rlp);
}

/*{
** Name: nuke_rlp
**
** Description:
**
** Inputs:
**	rcp			Recovery Context
**
** Outputs:
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      14-dec-1992 (rogerk)
**	    Reduced Logging Project: Written.
*/
static VOID
nuke_rlp(
RCP		*rcp)
{
    RCP_QUEUE	    *st;
    RLP		    *rlp;
    i4	    i;

    for (i = 0; i < RCP_LP_HASH; i++)
    {
	while (rcp->rcp_lph[i])
	{
	    rlp = rcp->rcp_lph[i];

	    remove_rlp(rlp);
	}
    }

    return;
}

/*{
** Name: display_recovery_context
**
** Description:
**
** Inputs:
**	    header			The log file header.
** Outputs:
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      14-dec-1992 (rogerk)
**	    Reduced Logging Project: Written.
**	26-jul-1993 (rogerk)
**	    Changed some RCP trace outputs to try to make it more readable.
*/
static VOID
display_recovery_context(
RCP		*rcp)
{
    DMP_LCTX		*lctx = rcp->rcp_lctx[0];
    RCP_QUEUE		*st;
    RDB			*db;
    RTX			*tx;
    RDMU		*rdmu;
    RLP			*lp;
    i4			i;

    TRdisplay("%@ RCP-P1: Recovery context complete:\n");
    TRdisplay("\t%d Databases need recovery.\n", rcp->rcp_db_count);
    TRdisplay("\t%d Transactions must be Redone.\n", rcp->rcp_rdtx_count);
    TRdisplay("\t%d Transactions must be Undone.\n", rcp->rcp_tx_count);
    TRdisplay("\t%d Transactions in Willing Commit state.\n",
	rcp->rcp_willing_commit);

    for (st = rcp->rcp_db_state.stq_next;
	st != (RCP_QUEUE *)&rcp->rcp_db_state.stq_next;
	st = db->rdb_state.stq_next)
    {
	db = (RDB *)((char *)st - CL_OFFSETOF(RDB, rdb_state));

	if (db->rdb_status & RDB_RECOVER)
	{
	    TRdisplay("\tDatabase %~t, ID %x: REDO from LGA <%d,%d,%d>\n",
		sizeof(DB_DB_NAME), &db->rdb_name, db->rdb_ext_dbid,
		db->rdb_first_lga.la_sequence,
		db->rdb_first_lga.la_block,
		db->rdb_first_lga.la_offset);

	    if (rcp->rcp_verbose)
	    {
		TRdisplay("\t    Name: %~t, Owner: %~t\n",
		    sizeof(DB_DB_NAME), &db->rdb_name,
		    sizeof(DB_OWN_NAME), &db->rdb_owner);
		TRdisplay("\t    Refcnt: %d, status Ext Id: %x, DCB: %p\n",
		    db->rdb_refcnt, db->rdb_status, 
		    db->rdb_ext_dbid, db->rdb_dcb);
		TRdisplay("\t    Root: %~t\n", db->rdb_l_root, &db->rdb_root);
	    }

	    TRdisplay("\t    %d completed transactions require REDO.\n",
		db->rdb_et_cnt);
	    TRdisplay("\t    %d open transactions require REDO then UNDO.\n",
		db->rdb_bt_cnt);

	    for (st = rcp->rcp_tx_state.stq_next;
		st != (RCP_QUEUE *)&rcp->rcp_tx_state.stq_next;
		st = tx->rtx_state.stq_next)
	    {
		tx = (RTX *)((char *)st - CL_OFFSETOF(RTX, rtx_state));

		if (tx->rtx_rdb == db)
		{
		   TRdisplay("\t    Transaction %x%x: Undo at LGA <%d,%d,%d>\n",
			tx->rtx_tran_id.db_high_tran, 
			tx->rtx_tran_id.db_low_tran,
			tx->rtx_last_lga.la_sequence,
			tx->rtx_last_lga.la_block,
			tx->rtx_last_lga.la_offset);

		    if (rcp->rcp_verbose)
		    {
		       TRdisplay("\t\tID %x, locklist %x, status %x, type %d\n",
			    tx->rtx_id, tx->rtx_ll_id, tx->rtx_status,
			    tx->rtx_recover_type);
		    }
		}
	    }
	}
	else if ((rcp->rcp_verbose) && (! (db->rdb_status & RDB_NOTDB)))
	{
	    TRdisplay("\tDatabase %~t, ID %x: No Recovery %s\n",
		sizeof(DB_DB_NAME), &db->rdb_name, db->rdb_ext_dbid,
		(db->rdb_status & RDB_INVALID) ? "Possible" : "Needed");
	}

	if (db->rdb_status & RDB_INVALID)
	{
	    TRdisplay("\tDatabase %~t is inconsistent.\n", 
		sizeof(DB_DB_NAME), &db->rdb_name);
	}
    }

    if (rcp->rcp_rdmu_state.stq_next != 
	    (RCP_QUEUE *)&rcp->rcp_rdmu_state.stq_next)
    {
	TRdisplay("\tNon-Redo Operations: No REDO recovery on these tables prior to LSN.\n");
    }

    for (st = rcp->rcp_rdmu_state.stq_next;
	st != (RCP_QUEUE *)&rcp->rcp_rdmu_state.stq_next;
	st = rdmu->rdmu_state.stq_next)
    {
	rdmu = (RDMU *)((char *)st - CL_OFFSETOF(RDMU, rdmu_state));

	TRdisplay("\t    DB %x, Table (%d, %d) \tat LSN (%x,%x)\n",
	    rdmu->dbid, rdmu->tabid.db_tab_base, rdmu->tabid.db_tab_index,
	    rdmu->lsn.lsn_high, rdmu->lsn.lsn_low);
    }

    if (rcp->rcp_verbose)
    {
	TRdisplay("\tOrphaned Process List:\n");
	for (i = 0; i < RCP_LP_HASH; i++)
	{
	    lp = (RLP *) &rcp->rcp_lph[i];
	    while (lp = lp->rlp_next)
	    {
		TRdisplay("\t    Process %x LGID %x\n",
		    lp->rlp_pid, lp->rlp_lpb_id);
	    }
	}
    }
}

/*{
** Name: dmr_dump_log
**
** Description:
**
** Inputs:
**	    RCP			    The RCP structure
**	    node_id		    Which node's log to dump
**
** Outputs:
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      14-dec-1992 (rogerk)
**	    Reduced Logging Project: Written.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**		Pass correct transaction handle to dm0l_read.
**		Added node_id argument
*/
VOID
dmr_dump_log(
RCP	*rcp,
i4	node_id)
{
    DMP_LCTX		*lctx = rcp->rcp_lctx[node_id];
    DM0L_HEADER		*record;
    LG_RECORD		*record_header;
    LG_LA		la;
    DB_STATUS		status;
    i4		error;
    DB_ERROR		local_dberr;

    TRdisplay("###########################################################\n");
    TRdisplay("%@ DUMP_LOG: Dumping Log File from CP (%x, %x, %x)\n",
	    lctx->lctx_cp.la_sequence,
	    lctx->lctx_cp.la_block,
	    lctx->lctx_cp.la_offset);

    /*
    ** Position the log file to the beginning of the last BCP.
    */
    status = dm0l_position(DM0L_FLGA, lctx->lctx_lxid,
			    &lctx->lctx_cp, lctx, &local_dberr);
    if (status != E_DB_OK)
    {
	uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
	    (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	return;
    }

    for (;;)
    {
	/*  Read the next record. */

	status = dm0l_read(lctx, lctx->lctx_lxid, &record, &la,
				(i4*)NULL, &local_dberr);
	if (status != E_DB_OK)
	    break;

	record_header = (LG_RECORD *)lctx->lctx_header;
	trace_record(record_header, record, &la, lctx->lctx_bksz);
    }

    TRdisplay("###########################################################\n");
    return;
}

/*{
** Name: dump_transaction -- echo the transaction to the TRdisplay file.
**
** Description:
**	It's a real bummer when the abort of a transaction fails. The database
**	goes inconsistent, and people get depressed.
**
**	Usually, the first question that comes to mind is:
**
**	    What log records had that transaction written?
**
**	because that is often a clue to help figure out why the transaction
**	failed. Plus, it often gives you a clue about exactly what parts of
**	the database have become damaged.
**
** Inputs:
**	    rcp				The recovery context.
**	    rtx				The transaction control block.
** Outputs:
**	    None
**	Returns:
**	    VOID
**
** History:
**	11-feb-1992 (bryanp)
**	    Created to track down temporary table bugs.
**	14-dec-1992 (rogerk)
**	    Removed unneeded dmve context.
**	    Added RCP parameter.
**	26-apr-1993 (bryanp)
**	    6.5 Cluster Support:
**		Pass correct transaction handle to dm0l_read.
**	26-jul-1993 (rogerk)
**	    Added rtx_orig_last_lga, used to store the log address of the
**	    last log record written by the transaction before recovery started.
**	    This value is not altered in the redo or undo pass.  Used this
**	    value here to position for dumping transaction log records.
*/
static VOID
dump_transaction(
RCP		    *rcp,
RTX		    *rtx)
{
    DMP_LCTX		*lctx = rtx->rtx_lctx;
    DM0L_HEADER		*record;
    LG_RECORD		*record_header;
    LG_LA		la;
    DB_STATUS		status;
    i4		error;
    DB_ERROR		local_dberr;

    TRdisplay("###########################################################\n");
    TRdisplay("%@ DUMP_TRANSACTION: Dumping Transaction %x%x\n",
	    rtx->rtx_tran_id.db_high_tran, rtx->rtx_tran_id.db_low_tran);

    /*	Position the log file to last record for this transaction. */

    status = dm0l_position(DM0L_BY_LGA, rtx->rtx_id, 
		(LG_LA *)&rtx->rtx_orig_last_lga, lctx, &local_dberr);
    if (status != E_DB_OK)
    {
	uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
	    (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	uleFormat(NULL, E_DM940C_RCP_ABORT, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
	    (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	return;
    }

    for (;;)
    {
	/*  Read the next record. */

	status = dm0l_read(lctx, rtx->rtx_id, &record, &la,
				(i4*)NULL, &local_dberr);
	if (status != E_DB_OK)
	{
	    uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	    uleFormat(NULL, E_DM940C_RCP_ABORT, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	    break;
	}

	record_header = (LG_RECORD *)lctx->lctx_header;
	trace_record(record_header, record, &la, lctx->lctx_bksz);

	if (record->type == DM0LBT)
	    break;
    }

    TRdisplay("###########################################################\n");
    return;
}

/*{
** Name: trace_record -- Format log record and its header info to trace file.
**
** Description:
**	Wrapper to format record Log Address, LSN, DBID, TRANID along with
**	the record itself to the trace file.
**
**	If DM1309 is set, log record is formatted in verbose mode.
**
** Inputs:
**	record_header		- Log record header returned by LGread.
**	record			- Pointer to log record
**	la			- Log Address of record
**	logfile_blocksize	- The block size of the system Log File.
**
** Outputs:
**	None
** Returns:
**	VOID
**
** History:
**	26-jul-1993 (rogerk)
**	    Created.
*/
static VOID
trace_record(
LG_RECORD	*record_header,
DM0L_HEADER	*record,
LG_LA		*la,
i4		logfile_blocksize)
{
    bool	verbose_flag = FALSE;

    TRdisplay("(LA=(%d,%d,%d),PREV_LA=(%d,%d,%d),LSN=(%x,%x))\n", 
	la->la_sequence, 
	la->la_block,
	la->la_offset,
	record_header->lgr_prev.la_sequence,
	record_header->lgr_prev.la_block,
	record_header->lgr_prev.la_offset,
	record->lsn.lsn_high, record->lsn.lsn_low);
    TRdisplay("\t\tXID: %x%x\n",
	record->tran_id.db_high_tran,
	record->tran_id.db_low_tran);

    if (DMZ_ASY_MACRO(9))
	verbose_flag = TRUE;

    dmd_log(verbose_flag, (PTR) record, logfile_blocksize);
}

/*{
** Name: percent_done -- Find percentage of log file we have completed.
**
** Description:
**	Given where we started, where we will end and the current position,
**	return the amount we have done.
**
** Inputs:
**	lctx			- logging system context
**	current_la		- current log position
**
** Outputs:
**	None
** Returns:
**	VOID
**
** History:
**	23-aug-1993 (jnash)
**	    Created (from logstat code) for use in iimonitor status line 
**	    display.
**	18-oct-1993 (jnash)
**	    Fix problem with large log files.
**	22-nov-1993 (jnash)
**	    B56865.  Fix problem where lctx_bkcnt used instead of 
**	    lctx_lg_header.lgh_count.
*/
static i4
percent_done(
DMP_LCTX	*lctx,
LG_LA		*current_la)
{
    i4	    blocks_recov;      
    i4	    blocks_total;      

    /*
    ** Hande case when called prior to first dm0l_read.
    */
    if (current_la->la_block == 0)
	return(0);

    /*
    ** Calculate amount of logfile recovered so far as a percentage
    ** of the size of the current CP window being processed.
    */
    blocks_recov = current_la->la_block - lctx->lctx_cp.la_block + 1;
    if (blocks_recov <= 0)
	blocks_recov += lctx->lctx_lg_header.lgh_count;

    blocks_total = lctx->lctx_eof.la_block - lctx->lctx_cp.la_block + 1;
    if (blocks_total <= 0)
	blocks_total += lctx->lctx_lg_header.lgh_count;

    /*
    ** Round calculations up to avoid divide by zero problems.
    */
    blocks_recov++;
    blocks_total++;

    return(blocks_recov * 100 / blocks_total);
}


/*{
** Name: process_sv_database_lock
**
** Description:
**	When a server is started in SINGLE-SERVER mode, it creates a
**      separate server lock list for acquiring EXCL SV_DATABASE lock(s).
**      These SV_DATABASE lock(s) are acquired using the single server
**      lock list so that should the server crash, the RCP will inherit
**      the SV_DATABASE lock(s)
**      The RCP needs to keep the EXCL SV_DATABASE lock during online
**      partial recovery of a row locking transactions. In all other
**      cases, it should release the lock, allowing concurrent access
**      to the database during online recovery.
**
** Inputs:
**	rcp			Recovery Context
**      rlp                     Process context
**      lkb                     LKB information for SV_DATABASE lock
**
** Outputs:
**      lock_released           True if we released SV_DATABASE lock
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      28-feb-1996 (stial01)  
**          Written.
*/
static DB_STATUS
process_sv_database_lock(
RCP                 *rcp,
RLP                 *rlp,
LK_LKB_INFO         *lkb,
bool                *lock_released)
{
    LK_LKID             lockid;
    RDB                 *rdb;
    RCP_QUEUE           *st;
    DB_STATUS           status = E_DB_OK;
    STATUS              cl_status;
    CL_ERR_DESC          sys_err;
    bool                rdb_found = FALSE;
    LK_LOCK_KEY         db_key;
    i4			error;

    CLRDBERR(&rcp->rcp_dberr);

    /*
    ** We have an X SV_DATABASE lock, find the RDB for the database and
    ** update it
    */
    for (st = rcp->rcp_db_state.stq_next;
	st != (RCP_QUEUE *)&rcp->rcp_db_state.stq_next;
	st = rdb->rdb_state.stq_next)
    {
	rdb = (RDB *)((char *)st - CL_OFFSETOF(RDB, rdb_state));

	/*
	** For SV_DATABASE locks, the lock key is the database name
	*/
	if (MEcmp((PTR)&rdb->rdb_name, (PTR)&lkb->lkb_key[1], 
		LK_KEY_LENGTH) == 0)
	{
	    rdb_found = TRUE;
	    break;
	}
    }

    if ( (rdb_found == TRUE)
	&& (rdb->rdb_status & RDB_REDO_PARTIAL))
    {
	rdb->rdb_status |= RDB_XSVDB_LOCK;
	rdb->rdb_svdb_ll_id = rlp->rlp_svdb_ll_id;
	rdb->rdb_svdb_lkb_id = lkb->lkb_id;
	rdb->rdb_svdb_rsb_id = lkb->lkb_rsb_id;
    }
    else
    {
	/*
	** If no RDB is found, or we are not doing PARTIAL recovery on
	** this database, release the SV_DATABASE lock
	*/

	lockid.lk_unique = lkb->lkb_id;
	lockid.lk_common = lkb->lkb_rsb_id;

	TRdisplay("\t    Releasing SV_DATABASE lock.\n");

	cl_status = LKrelease((i4) 0, rlp->rlp_svdb_ll_id, &lockid, 
		(LK_LOCK_KEY *)0, (LK_VALUE *)0, &sys_err);
	if (cl_status == OK)
	{
	    *lock_released = TRUE;
	    rlp->rlp_status &= ~RDB_XSVDB_LOCK;
	}
	else
	{
	    uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	    uleFormat(NULL, E_DM901B_BAD_LOCK_RELEASE, &sys_err, ULE_LOG,
		NULL, (char *)NULL, (i4)0, (i4 *)NULL, &error,
		1, 0, rlp->rlp_svdb_ll_id);
	    status = E_DB_ERROR;
	}
    }

    return (status);
}


/*{
** Name: lookup_invalid_tbl
**
** Description:
**
** Inputs:
**      rdb			Database Context
**      tabid
**      record			log record
**      rtbl
**
** Outputs:
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      06-apr-2005 (stial01)
**          Created.
*/
static VOID
lookup_invalid_tbl(
RDB		*rdb,
DB_TAB_ID	*tabid,
DM0L_HEADER	*record,
RTBL		**return_tbl)
{
    RCP_QUEUE	    *st;
    RTBL	    *rtbl;
    DB_TAB_ID	    loc_tabid;

    *return_tbl = NULL;

    dmve_get_tab(record, &loc_tabid, (DB_TAB_NAME*)0, (DB_OWN_NAME *)0);
    if (loc_tabid.db_tab_base != tabid->db_tab_base ||
	loc_tabid.db_tab_index != tabid->db_tab_index)
    {
	TRdisplay("lookup_invalid_tbl: %d,%d FIXING -> %d,%d )\n",
		tabid->db_tab_base,tabid->db_tab_index,
		loc_tabid.db_tab_base, loc_tabid.db_tab_index);
	STRUCT_ASSIGN_MACRO(loc_tabid, *tabid);
    }

    for (st = rdb->rdb_tbl_invalid.stq_next;
	st != (RCP_QUEUE *)&rdb->rdb_tbl_invalid.stq_next;
	st = rtbl->rtbl_state.stq_next)
    {
	rtbl = (RTBL *)((char *)st - CL_OFFSETOF(RTBL, rtbl_state));

	if (rtbl->tabid.db_tab_base == tabid->db_tab_base
		 && rtbl->tabid.db_tab_index == tabid->db_tab_index)
	{
	    *return_tbl = rtbl;
	    return;
	}
    }

    return;
}


/*{
** Name: add_invalid_tbl
**
** Description:
**
** Inputs:
**	rcp			Recovery Context
**      rdb			Database Context
**      rtbl			Table context
**      lctx			Logging Context
**      record			log record
**
** Outputs:
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      06-apr-2005 (stial01)
**          Created.
*/
static DB_STATUS
add_invalid_tbl(
RCP		*rcp,
RDB		*rdb,
RTBL		*rtbl,
DMP_LCTX	*lctx,
DM0L_HEADER	*record)
{
    DB_STATUS	    status;
    i4		    dbid = record->database_id;
    DB_TAB_ID	    tabid;
    DB_TAB_NAME	    tabname;
    DB_OWN_NAME     ownname;
    i4		    length;
    STATUS	    cl_status;
    CL_ERR_DESC      sys_err;
    DB_STATUS	    loc_status = E_DB_OK;
    i4		    loc_error;
    DMP_TABLE_IO    *tbio;
    DMVE_CB	    *dmve = (DMVE_CB *)rcp->rcp_dmve_cb;
    LG_RECOVER	    recover_set;
    LG_RECOVER      recover_show;
    i4		    rtbl_flag = 0;
    i4		    error;
    DM_PAGENO	    err_pages[3]; /* max pages for a log record */
    i4		    err_page_cnt = 0;
    i4		    error_action;
    

    error_action = Db_recovery_error_action;

    CLRDBERR(&rcp->rcp_dberr);

    dmve_get_tab(record, &tabid, &tabname, &ownname);

    /* Is there a table associated with this log record */
    if (tabid.db_tab_base == 0)
	return (E_DB_ERROR);

    /* must fail if this is core catalog */
    if (tabid.db_tab_base <= DM_SCONCUR_MAX)
	return (E_DB_ERROR);

    TRdisplay("%@ RCP-ERROR  DB=%~t TABLE=%~t LSN=%d,%d \n",
	sizeof(rdb->rdb_name), &rdb->rdb_name,
	sizeof(DB_TAB_NAME), tabname.db_tab_name,
	record->lsn.lsn_high, record->lsn.lsn_low);

    /*
    **
    ** Online error actions are
    **     CONTINUE_IGNORE_DB
    **     STOP
    **
    ** Offline error actions are
    **     CONTINUE_IGNORE_DB
    **     STOP
    **
    **     CONTINUE_IGNORE_TABLE (new)
    **     CONTINUE_IGNORE_PAGE (unsupported)
    **     PROMPT (unsupported)
    **
    */

    if (tabid.db_tab_index > 0)
    {
	/* index error action during startup is always CONTINUE_INGORE_TABLE */
	if (rcp->rcp_action == RCP_R_STARTUP)
	{
	    TRdisplay("Error Action (INDEX): CONTINUE_IGNORE_TABLE\n");
	    error_action = RCP_CONTINUE_IGNORE_TABLE;
	}
    }

    if ( error_action == RCP_CONTINUE_IGNORE_DB || 
	    error_action == RCP_STOP_ON_INCONS_DB ||
	    rcp->rcp_cluster_enabled ||
	    rcp->rcp_action != RCP_R_STARTUP)
    {
	return (E_DB_ERROR);
    }

    if (error_action != RCP_CONTINUE_IGNORE_TABLE &&
	error_action != RCP_CONTINUE_IGNORE_PAGE &&
	error_action != RCP_PROMPT)
    {
	/* shouldn't get here */
	return (E_DB_ERROR);
    }

    /*
    ** If dmve left the tbio fixed, dm2d_close_db() will fail
    ** Also if dmve left pages fixed, dm0p_force_page() will fail
    ** (most likely to happen after an exception, E_DM004A_INTERNAL_ERROR
    */
    if (dmve->dmve_tbio)
    {
	tbio = dmve->dmve_tbio;
	TRdisplay("RCP unfixing tbio for %~t\n",
		sizeof(DB_TAB_NAME), tabname.db_tab_name);
	loc_status = dmve_unfix_tabio(dmve, &tbio, DMVE_INVALID);
    }

    /* check for predefined error handling */
    if (error_action == RCP_CONTINUE_IGNORE_TABLE)
    {
	TRdisplay("Error Action defined: CONTINUE_IGNORE_TABLE\n");
	recover_show.lg_flag = LG_A_RCP_IGNORE_TABLE;
	rtbl_flag = RTBL_IGNORE_TABLE;
	/* Fall through and add this table to invalid tables list */
    }
    else if (error_action == RCP_CONTINUE_IGNORE_PAGE)
    {
	TRdisplay("Error Action defined: CONTINUE_IGNORE_PAGE\n");
	recover_show.lg_flag = LG_A_RCP_IGNORE_LSN;
	rtbl_flag = 0;
	/* Fall through and add this table to invalid tables list */
    }
    else if (error_action == RCP_PROMPT)
    {
	TRdisplay("Error Action defined: PROMPT\n");
	TRdisplay(
	"WARNING The RCP encountered an error and is waiting for input.\n");
	TRdisplay("Valid input is:\n");
	TRdisplay("rcpconfig -rcp_continue_ignore_db=%~t\n",
		DB_MAXNAME, &rdb->rdb_name);
	TRdisplay("OR\n");
	TRdisplay("rcpconfig -rcp_continue_ignore_table=%~t\n",
		DB_MAXNAME, tabname.db_tab_name);
	TRdisplay("OR\n");
	TRdisplay("rcpconfig -rcp_continue_ignore_lsn=%d,%d\n",
		record->lsn.lsn_high, record->lsn.lsn_low);
	TRdisplay("OR\n");
	TRdisplay("rcpconfig -rcp_stop_lsn=%d,%d\n",
		record->lsn.lsn_high, record->lsn.lsn_low);

	/* Set lg_flag to invalid value, so rcpstat can tell we are waiting */
	recover_set.lg_flag = -1;
	recover_set.lg_lsn.lsn_high = record->lsn.lsn_high;
	recover_set.lg_lsn.lsn_low = record->lsn.lsn_low;
	MEcopy((char *)&rdb->rdb_name, sizeof(rdb->rdb_name), 
			&recover_set.lg_dbname[0]);
	MEcopy(tabname.db_tab_name, sizeof(tabname.db_tab_name), 
			&recover_set.lg_tabname[0]);
	
	if (status = LGalter(LG_A_INIT_RECOVER, (PTR)&recover_set, 
			sizeof(recover_set), &sys_err))
	{
	    uleFormat(NULL, status, (CL_ERR_DESC *)&sys_err, ULE_LOG , NULL, 
		    (char * )0, 0L, (i4 *)NULL, &error, 0);
	    uleFormat(NULL, E_DM900B_BAD_LOG_ALTER, &sys_err, ULE_LOG, 
		    NULL, (char *)NULL, 0L, (i4 *)NULL, &error, 1,
		    0, LG_A_INIT_RECOVER);
	}

	/* Keep checking for lg event until we see we got one! */
	TRdisplay("Waiting for RECOVER event...\n");
	for ( ; status == OK; )
	{
	    PCsleep(500);

	    TRdisplay(".");
	    /* FIX ME should we set timeout?? */
	    dmr_check_event(rcp, LG_NOWAIT, lctx->lctx_lxid);

	    cl_status = LGshow(LG_S_SHOW_RECOVER, 
			    (PTR)&recover_show, sizeof(recover_show), 
			    &length, &sys_err);
	    if (cl_status != OK)
	    {
		uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
			    (char *)NULL, 0L, (i4 *)NULL, &error, 0);
		dmr_log_error(E_DM9017_BAD_LOG_SHOW, &sys_err, LG_S_LGSTS, 
		    E_DM9410_RCP_SHUTDOWN);
		break;
	    }

	    if (recover_show.lg_flag == LG_A_RCP_IGNORE_DB ||
		recover_show.lg_flag == LG_A_RCP_IGNORE_TABLE ||
		recover_show.lg_flag == LG_A_RCP_IGNORE_LSN ||
		recover_show.lg_flag == LG_A_RCP_STOP_LSN)
		break;
	}

	TRdisplay("\n");

	if (recover_show.lg_flag == LG_A_RCP_IGNORE_DB)
	{
	    TRdisplay("RCP wakeup by rcpconfig -rcp_continue_ignore_db=%~t\n",
		DB_MAXNAME, &rdb->rdb_name);
	    return (E_DB_ERROR);
	}
	else if (recover_show.lg_flag == LG_A_RCP_IGNORE_TABLE)
	{
	    TRdisplay("RCP wakeup by rcpconfig -rcp_continue_ignore_table\n",
		DB_MAXNAME, tabname.db_tab_name);
	    rtbl_flag = RTBL_IGNORE_TABLE;
	    /* Fall through and add this table to invalid tables list */
	}
	else if (recover_show.lg_flag == LG_A_RCP_IGNORE_LSN)
	{
	    TRdisplay("RCP wakeup by rcpconfig -rcp_continue_ignore_lsn\n",
		record->lsn.lsn_high, record->lsn.lsn_low);
	    rtbl_flag = 0;
	    /* Fall through and add this table to invalid tables list */
	}
	else if (recover_show.lg_flag == LG_A_RCP_STOP_LSN)
	{
	    TRdisplay("RCP wakeup by rcpconfig -rcp_stop_lsn\n",
		record->lsn.lsn_high, record->lsn.lsn_low);
	    /* NOTE: changing Db_recovery_error_action */
	    Db_recovery_error_action = RCP_STOP_ON_INCONS_DB;
	    TRdisplay("Current RCP error action %30w\n",
		RCP_ERROR_ACTION, Db_recovery_error_action);
	    return (E_DB_ERROR);
	}
	else
	{
	    return (E_DB_ERROR);
	}

	/* FIX ME is code below needed? */
	/* Not waiting anymore, update LGD */
	MEfill(sizeof(recover_set), 0, (char *)&recover_set);
	if (status = LGalter(LG_A_INIT_RECOVER, (PTR)&recover_set, 
			sizeof(recover_set), &sys_err))
	{
	    uleFormat(NULL, status, (CL_ERR_DESC *)&sys_err, ULE_LOG , NULL, 
		    (char * )0, 0L, (i4 *)NULL, &error, 0);
	    uleFormat(NULL, E_DM900B_BAD_LOG_ALTER, &sys_err, ULE_LOG, 
		    NULL, (char *)NULL, 0L, (i4 *)NULL, &error, 1,
		    0, LG_A_INIT_RECOVER);
	}
    }

    if (!rtbl)
    {
	/*
	** Allocate an RTBL structure.
	*/
	status = dm0m_allocate(sizeof(RTBL), DM0M_ZERO, RTBL_CB, RTBL_ASCII_ID, 
			    (char *)rcp, (DM_OBJECT **)&rtbl, &rcp->rcp_dberr);
	if (status != E_DB_OK)
	{
	    uleFormat(&rcp->rcp_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	    return (E_DB_ERROR);
	}

	STRUCT_ASSIGN_MACRO(tabid, rtbl->tabid);
	STRUCT_ASSIGN_MACRO(tabname, rtbl->tabname);
	STRUCT_ASSIGN_MACRO(ownname, rtbl->ownname);

	/*
	** Insert on Invalid tables Queue.
	*/
	rtbl->rtbl_state.stq_next = rdb->rdb_tbl_invalid.stq_next;
	rtbl->rtbl_state.stq_prev = (RCP_QUEUE *)&rdb->rdb_tbl_invalid.stq_next;
	rdb->rdb_tbl_invalid.stq_next->stq_prev = &rtbl->rtbl_state;
	rdb->rdb_tbl_invalid.stq_next = &rtbl->rtbl_state;
	rtbl->lsn.lsn_high = record->lsn.lsn_high;
	rtbl->lsn.lsn_low = record->lsn.lsn_low;
	rtbl->tran_id.db_low_tran = record->tran_id.db_low_tran;
	rtbl->tran_id.db_high_tran = record->tran_id.db_high_tran;
	rtbl->rtbl_flag = rtbl_flag;
	rtbl->redo_cnt = 0;
	rtbl->undo_cnt = 0;
        rtbl->err_page_cnt = 0;

	TRdisplay("\tAdd %~t,%~t to invalid table list.\n",
	    sizeof(DB_TAB_NAME), tabname.db_tab_name,
	    sizeof(DB_OWN_NAME), ownname.db_own_name);

	/* indicate table recovery error for this db */
	rdb->rdb_status |= RDB_TABLE_RECOVERY_ERROR;

	/* set inconsistency code */
	if (tabid.db_tab_index <= 0)
	    rdb->rdb_inconst_code = DSC_TABLE_INCONST;
	else if (rdb->rdb_inconst_code != DSC_TABLE_INCONST)
	    rdb->rdb_inconst_code = DSC_INDEX_INCONST;
    }

    rtbl->rtbl_flag = rtbl_flag;
    if (rtbl_flag == RTBL_IGNORE_TABLE)
    {
	uleFormat(NULL, E_DM944E_RCP_INCONS_TABLE, (CL_ERR_DESC *)NULL,  
	ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL, &error, 4,
	sizeof(DB_TAB_NAME), tabname.db_tab_name,
	sizeof(DB_OWN_NAME), ownname.db_own_name,
	0, record->lsn.lsn_high, 0, record->lsn.lsn_low);
    }

    /*
    ** We can ignore errors on up to MAX_ERR_PAGES per table
    ** After an error, ignore subsequent record updates to the SAME page
    ** (to do otherwise could lead to unpredictable behavior,
    ** including memory overlays because dmve routines
    ** make assumptions based page_lsn)
    */ 
    if (recover_show.lg_flag == LG_A_RCP_IGNORE_LSN && rtbl_flag == 0)
    {
	switch (record->type)
	{
	    case DM0LPUT: 
		err_pages[err_page_cnt++] = 
			((DM0L_PUT *)record)->put_tid.tid_tid.tid_page;
		break;
	    case DM0LDEL:
		err_pages[err_page_cnt++] = 
			((DM0L_DEL *)record)->del_tid.tid_tid.tid_page;
		break;
	    case DM0LREP:
		err_pages[err_page_cnt++] = 
			((DM0L_REP *)record)->rep_otid.tid_tid.tid_page;
		if (((DM0L_REP *)record)->rep_ntid.tid_tid.tid_page
		    != ((DM0L_REP *)record)->rep_otid.tid_tid.tid_page)
		    err_pages[err_page_cnt++] = 
			    ((DM0L_REP *)record)->rep_ntid.tid_tid.tid_page;
		break;
 
	    case DM0LBTPUT:
		err_pages[err_page_cnt++] = 
			((DM0L_BTPUT *)record)->btp_bid.tid_tid.tid_page;
		break;

	    case DM0LBTDEL:
		err_pages[err_page_cnt++] = 
			((DM0L_BTDEL *)record)->btd_bid.tid_tid.tid_page;
		break;

	    default: /* other log records will set RTBL_IGNORE_TABLE */
		break;
	}

	if (!err_page_cnt)
	{
	    /* FIX ME report error */
	    TRdisplay("After errors for this log record type, ignoring table\n");
	    rtbl->rtbl_flag = RTBL_IGNORE_TABLE;
	}
	else
	{
	   /* add to list of page to ignore */
	   i4 ii, jj;
	   for (ii = 0; ii < err_page_cnt; ii++)
	   {
		for (jj = 0; jj < rtbl->err_page_cnt; jj++)
		    if (err_pages[ii] == rtbl->err_pages[jj])
			break;
		if (jj == rtbl->err_page_cnt)
		{
		    /* Not found */
		    if (rtbl->err_page_cnt < MAX_ERR_PAGES)
		    {
			rtbl->err_pages[rtbl->err_page_cnt++] = err_pages[ii];

			uleFormat(NULL, E_DM944D_RCP_INCONS_PAGE, (CL_ERR_DESC *)NULL,  
			    ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL, &error, 5,
			    sizeof(DB_TAB_NAME), tabname.db_tab_name,
			    sizeof(DB_OWN_NAME), ownname.db_own_name,
			    0, record->lsn.lsn_high, 0, record->lsn.lsn_low,
			    0, err_pages[ii]);
		    }
		    else
		    {
			/* Max errors for this table */
			rtbl->rtbl_flag = RTBL_IGNORE_TABLE;

			uleFormat(NULL, E_DM944E_RCP_INCONS_TABLE, (CL_ERR_DESC *)NULL,  
			    ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL, &error, 4,
			    sizeof(DB_TAB_NAME), tabname.db_tab_name,
			    sizeof(DB_OWN_NAME), ownname.db_own_name,
			    0, record->lsn.lsn_high, 0, record->lsn.lsn_low);

			break;
		    }
		}
	   }
	}
    }

    if (rdb->rdb_status & RDB_REDO)
	rtbl->redo_cnt++;
    else
	rtbl->undo_cnt++;

    return (E_DB_OK);
}


/*{
** Name: mark_tbl_incons
**
** Description:
**
** Inputs:
**	rcp			Recovery Context
**      rdb			Database Context
**
** Outputs:
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      06-apr-2005 (stial01)
**          Created.
**	11-Apr-2008 (kschendel)
**	    dm2r position call updated, fix here.
*/
static DB_STATUS
mark_tbl_incons (
RCP		*rcp,
RDB		*rdb)
{
    RTBL	    	*rtbl;
    RCP_QUEUE       	*st;
    DMP_DCB	    	*dcb = rdb->rdb_dcb;
    DB_STATUS       	status = E_DB_OK;
    DB_STATUS	    	tmp_status = E_DB_OK;
    DMP_RELATION    	rel_rec;
    DB_TAB_ID		table_id;
    DM_TID	    	rel_tid;
    DMP_RCB	    	*rel_rcb;
    i4		    	log_id = 0;
    i4			icnt = 0;
    DB_TAB_TIMESTAMP	stamp;
    DM2R_KEY_DESC       qual_list[1];
    i4			error;

    CLRDBERR(&rcp->rcp_dberr);

    TRdisplay("%@ LIST-INCONSISTENT-TABLES for database %~t,%~t\n",
	sizeof(rdb->rdb_name), &rdb->rdb_name,
	sizeof(rdb->rdb_owner), &rdb->rdb_owner);

    for (st = rdb->rdb_tbl_invalid.stq_next;
	st != (RCP_QUEUE *)&rdb->rdb_tbl_invalid.stq_next;
	st = rtbl->rtbl_state.stq_next)
    {
	rtbl = (RTBL *)((char *)st - CL_OFFSETOF(RTBL, rtbl_state));
	icnt++;
	TRdisplay("\t%~t,%~t is inconsistent, %d redo %d undo skipped\n",
	    sizeof(DB_TAB_NAME), rtbl->tabname.db_tab_name,
	    sizeof(DB_OWN_NAME), rtbl->ownname.db_own_name,
	    rtbl->redo_cnt, rtbl->undo_cnt);
    }

    TRdisplay("%@ MARK-INCONSISTENT-TABLES for database %~t,%~t\n",
	sizeof(rdb->rdb_name), &rdb->rdb_name,
	sizeof(rdb->rdb_owner), &rdb->rdb_owner);

    do
    {
	/* Open iirelation catalog */
	table_id.db_tab_base = DM_B_RELATION_TAB_ID;
	table_id.db_tab_index = DM_I_RELATION_TAB_ID;
	status = dm2t_open(dcb, &table_id, DM2T_IX, DM2T_UDIRECT, DM2T_A_WRITE,
		(i4)0, (i4)20, (i4)0,
		log_id, rcp->rcp_lock_list, 
		(i4)0, (i4)0, (i4)0, &rcp_tran_id,
		&stamp, &rel_rcb, (DML_SCB *)0, &rcp->rcp_dberr);
	if (status != E_DB_OK)
	    break;

	/* Turn off logging, replace fails otherwise */
	rel_rcb->rcb_logging = 0;

	qual_list[0].attr_number = DM_1_RELATION_KEY;
	qual_list[0].attr_operator = DM2R_EQ;

	/*
	** Loop through all invalid tables and mark each one inconsistent.
	*/
	for (st = rdb->rdb_tbl_invalid.stq_next;
	    st != (RCP_QUEUE *)&rdb->rdb_tbl_invalid.stq_next;
	    st = rtbl->rtbl_state.stq_next)
	{
	    rtbl = (RTBL *)((char *)st - CL_OFFSETOF(RTBL, rtbl_state));

	    qual_list[0].attr_value = (char *)&rtbl->tabid;

	    /*
	    ** Position on iirelation
	    */
	    status = dm2r_position( rel_rcb, DM2R_QUAL, qual_list,
			     (i4)1, (DM_TID *)0, &rcp->rcp_dberr);
	    if (status != E_DB_OK)
		break;

	    /*
	    ** Look for record in iirelation
	    ** It's not an error if we don't find it in iirelation,
	    ** it may have been dropped.
	    */
	    for (;;)
	    {
		status = dm2r_get( rel_rcb, &rel_tid, DM2R_GETNEXT,
				   (char *)&rel_rec, &rcp->rcp_dberr);
		if (status != E_DB_OK)
		{
		    if (rcp->rcp_dberr.err_code == E_DM0055_NONEXT)
		    {
			status = E_DB_OK;
			CLRDBERR(&rcp->rcp_dberr);
		    }
		    break;
		}

		/*
		** Check table, need to match on both base and index
		*/
		if ((rel_rec.reltid.db_tab_base != rtbl->tabid.db_tab_base)
		 || (rel_rec.reltid.db_tab_index != rtbl->tabid.db_tab_index))
		{
		    continue;
		}

		rel_rec.relstat2 |= TCB2_PHYS_INCONSISTENT;
		rel_rec.relstat2 |= TCB2_LOGICAL_INCONSISTENT;

		status = dm2r_replace(rel_rcb, &rel_tid, DM2R_BYPOSITION,
			(char *)&rel_rec, (char *)NULL, &rcp->rcp_dberr);

		if (status != E_DB_OK)
		    break;

		icnt--;

		if (rel_rec.reltid.db_tab_index > 0)
		{
		    uleFormat(NULL, E_DM944B_RCP_IDXINCONS, (CL_ERR_DESC *)NULL,  
		    ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL, &error, 2,
		    sizeof(rel_rec.relid), &rel_rec.relid,
		    sizeof(rel_rec.relowner), &rel_rec.relowner);
		}
		else
		{
		    uleFormat(NULL, E_DM944C_RCP_TABLEINCONS, (CL_ERR_DESC *)NULL,  
		    ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL, &error, 2,
		    sizeof(rel_rec.relid), &rel_rec.relid,
		    sizeof(rel_rec.relowner), &rel_rec.relowner);
		}

		break;
	    }

	    if ( status != E_DB_OK )
		break;
	}

    } while (FALSE);

    if (status != E_DB_OK)
    {
	uleFormat(&rcp->rcp_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
	    (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
    }

    /* Close the relation catalog */
    if (rel_rcb)
    {
	status = dm2t_close(rel_rcb, 0, &rcp->rcp_dberr);
	if (status != E_DB_OK)
	{
	    uleFormat(&rcp->rcp_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	}
    }

    if (icnt)
	TRdisplay("%@RCP: Error marking invalid table inconsistent.\n");

    /* return OK, the database gets marked inconsistent anyway */
    return (E_DB_OK);

}



/*
** Name: rcp_archive -- perform archiving in recovery server startup.
**
** Description:
**	This routine is called by the RCP process to perform archiving
**	after the log analysis phase, before performing redo-undo recovery.
**
** Inputs:
**	lctx			- logfile context block
**
** Outputs:
**	err_code		- reason for error completion
**
** Returns:
**	E_DB_OK
**	E_DB_ERROR
**
** History:
**      10-dec-2007 (stial01)
**          Created from csp_archive().
*/
static DB_STATUS
rcp_archive(
RCP		*rcp,
DMP_LCTX *lctx)
{
    ACB			*acb;
    ACB			*a;
    DB_STATUS		complete_status;
    DB_ERROR		complete_dberr;
    STATUS		status;
    i4			error;

    CLRDBERR(&rcp->rcp_dberr);

    if (CXcluster_enabled())
	return (E_DB_OK);

    /*
    ** Allocate and initialize the acb, init the  Archiver database.
    */
    status = dma_prepare(DMA_RCP, &acb, lctx, &rcp->rcp_dberr);
    if (status != E_DB_OK)
    {
	uleFormat(&rcp->rcp_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
	    (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	TRdisplay("%@ Error in RCP archiving -- Can't prepare Archiver.\n");

	SETDBERR(&rcp->rcp_dberr, 0, E_DM9851_ACP_INITIALIZE_EXIT); /* FIX -- need new msg */
	return(E_DB_ERROR);
    }

    a = acb;

    for (;;)
    {
	/*
	** Build archive context by analyzing this node's log file.
	*/
	status = dma_offline_context(a);
	if (status != E_DB_OK)
	{
	    uleFormat(&a->acb_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	    TRdisplay("%@ RCP RCP_ARCHIVE: Archive Cycle Failed!\n");
	    SETDBERR(&rcp->rcp_dberr, 0, E_DM9851_ACP_INITIALIZE_EXIT); /* FIX -- need new msg */
	    break;
	}

	/*
	** Get config file information for each database, open journal 
	** and dump files, skip over CP and position just past it.
	*/
	status = dma_soc(a);
	if (status != E_DB_OK)
	{
	    uleFormat(&a->acb_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	    TRdisplay("%@ RCP RCP_ARCHIVE: Archive Cycle dma_soc Failed!\n");
	    SETDBERR(&rcp->rcp_dberr, 0, E_DM9851_ACP_INITIALIZE_EXIT); /* FIX -- need new msg */
	    break;
	}

	/*
	** Copy the log to the journal and dump files.
	*/
	status = dma_archive(a);
	if (status != E_DB_OK)
	{
	    uleFormat(&a->acb_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	    TRdisplay("%@ RCP RCP_ARCHIVE: Archive dma_archive Failed!\n");
	    SETDBERR(&rcp->rcp_dberr, 0, E_DM9851_ACP_INITIALIZE_EXIT); /* FIX -- need new msg */
	    break;
	}

	/*
	** Clean up at end of cycle.
	*/
	status = dma_eoc(a);
	if (status != E_DB_OK)
	{
	    uleFormat(&a->acb_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	    TRdisplay("%@ RCP RCP_ARCHIVE: Archive Cycle dma_eoc Failed!\n");
	    SETDBERR(&rcp->rcp_dberr, 0, E_DM9851_ACP_INITIALIZE_EXIT); /* FIX -- need new msg */
	    break;
	}

	TRdisplay("%@ RCP: Archive Cycle Completed Successfully.\n");

	break;
    }

    /*	Complete archive processing. */
    complete_status = dma_complete(&a, &complete_dberr);
    if (complete_status != E_DB_OK)
    {
	uleFormat(&complete_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
	    (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);

	if ( status == E_DB_OK )
	{
	    status = complete_status;
	    rcp->rcp_dberr = complete_dberr;
	}

	TRdisplay("%@ Error terminating ACP -- Can't complete Archiver.\n");
    }

    return (status);
}


/*{
** Name: ex_handler	- recovery exception handler.
**
** Description:
**      This routine will catch all exceptions processing redo/undo action. 
**
** Inputs:
**      ex_args                         Exception arguments.
**
** Outputs:
**	Returns:
**	    EXDECLARE
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**     16-jan-2008 (stial01)
**          cloned from exception handler in dmfinit
*/
static STATUS
ex_handler(
EX_ARGS		    *ex_args)
{
    i4	    err_code;
    i4	    status;

#ifdef VMS
    status = sys$setast(0);
#endif
    
    if (ex_args->exarg_num == EX_DMF_FATAL)
    {
	err_code = E_DM904A_FATAL_EXCEPTION;
    }
    else
    {
	err_code = E_DM9049_UNKNOWN_EXCEPTION;
    }

    /*
    ** Report the exception.
    */
    ulx_exception( ex_args, DB_DMF_ID, err_code, FALSE );

#ifdef VMS
    if (status == 9)
	sys$setast(1);
#endif

    return (EXDECLARE);
}


/*{
** Name: recover_force_pages	- Try to clean up invalid tables in cache
**
** Description: Try to clean up invalid tables in cache
**
**
** Inputs:
**      rcp				Recovery context
**
** Outputs:
**      none
**
** Side Effects:
**	    none
**
** History:
**      28-jan-2008 (stial01)
**          Created.
*/
static VOID
recover_force_pages (
RCP		*rcp)
{
    RDB			*rdb;
    RTBL	    	*rtbl;
    RCP_QUEUE       	*rdb_st;
    RCP_QUEUE		*st;
    DB_STATUS       	status = E_DB_OK;
    DB_STATUS	    	tmp_status = E_DB_OK;
    i4			dm0p_flags;

    TRdisplay("%@ RECOVER FORCE PAGES\n");
    for (rdb_st = rcp->rcp_db_state.stq_next;
	rdb_st != (RCP_QUEUE *)&rcp->rcp_db_state.stq_next;
	rdb_st = rdb->rdb_state.stq_next)
    {
	rdb = (RDB *)((char *)rdb_st - CL_OFFSETOF(RDB, rdb_state));
	if ((rdb->rdb_status & RDB_TABLE_RECOVERY_ERROR) == 0 ||
		!rdb->rdb_dcb)
	    continue;

	dm0p_flags = (DM0P_INVALID_UNFIX | DM0P_INVALID_UNLOCK | DM0P_INVALID_UNMODIFY);

	TRdisplay("%@ RECOVER FORCE PAGES for database %~t,%~t\n",
	    sizeof(rdb->rdb_name), &rdb->rdb_name,
	    sizeof(rdb->rdb_owner), &rdb->rdb_owner);

	for (st = rdb->rdb_tbl_invalid.stq_next;
	    st != (RCP_QUEUE *)&rdb->rdb_tbl_invalid.stq_next;
	    st = rtbl->rtbl_state.stq_next)
	{
	    rtbl = (RTBL *)((char *)st - CL_OFFSETOF(RTBL, rtbl_state));
	    dm0p_unfix_invalid_table(rdb->rdb_dcb, &rtbl->tabid, dm0p_flags); 
	}
    }

    return;
}


static bool
rtbl_ignore_page(
RTBL		*rtbl,
DM0L_HEADER	*record)
{
    DM_PAGENO	    err_pages[3]; /* max pages for a log record */
    i4		    err_page_cnt = 0;
    i4		    ii, jj;

    if (!rtbl)
	return (FALSE);

    if (rtbl->err_page_cnt == 0)
	return (FALSE);

    /* Get the pages updated by this log record */
    switch (record->type)
    {
	case DM0LPUT: 
	    err_pages[err_page_cnt++] = 
		    ((DM0L_PUT *)record)->put_tid.tid_tid.tid_page;
	    break;
	case DM0LDEL:
	    err_pages[err_page_cnt++] = 
		    ((DM0L_DEL *)record)->del_tid.tid_tid.tid_page;
	    break;
	case DM0LREP:
	    err_pages[err_page_cnt++] = 
		    ((DM0L_REP *)record)->rep_otid.tid_tid.tid_page;
	    if (((DM0L_REP *)record)->rep_ntid.tid_tid.tid_page
		!= ((DM0L_REP *)record)->rep_otid.tid_tid.tid_page)
		err_pages[err_page_cnt++] = 
			((DM0L_REP *)record)->rep_ntid.tid_tid.tid_page;
	    break;

	case DM0LBTPUT:
	    err_pages[err_page_cnt++] = 
		    ((DM0L_BTPUT *)record)->btp_bid.tid_tid.tid_page;
	    break;

	case DM0LBTDEL:
	    err_pages[err_page_cnt++] = 
		    ((DM0L_BTDEL *)record)->btd_bid.tid_tid.tid_page;
	    break;

	default: /* other log records will set RTBL_IGNORE_TABLE */
	    break;
    }

    /*
    ** if its not a record update... 
    ** since the table is already invalid...
    ** we have to ignore this and subsequent updates to this table
    */
    if (!err_page_cnt)
    {
	rtbl->rtbl_flag = RTBL_IGNORE_TABLE;
	return (TRUE);
    }

    for (ii = 0; ii < err_page_cnt; ii++)
    {
	for (jj = 0; jj < rtbl->err_page_cnt; jj++)
	    if (err_pages[ii] == rtbl->err_pages[jj])
		break;
    }

    if (jj == rtbl->err_page_cnt)
	return (FALSE);
    else
	return (TRUE);
}
