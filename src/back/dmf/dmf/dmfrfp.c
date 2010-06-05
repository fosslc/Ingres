/*
** Copyright (c) 1986, 2004 Ingres Corporation
NO_OPTIM = dr6_us5
**
*/

#include    <compat.h>
#include    <gl.h>
#include    <tm.h>
#include    <me.h>
#include    <cs.h>
#include    <cv.h>
#include    <di.h>
#include    <id.h>
#include    <jf.h>
#include    <nm.h>
#include    <st.h>
#include    <pc.h>
#include    <ck.h>
#include    <tr.h>
#include    <er.h>
#include    <ex.h>
#include    <si.h>
#include    <sr.h>
#include    <lo.h>
#include    <bt.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <lk.h>
#include    <cx.h>
#include    <lg.h>
#include    <ulf.h>
#include    <adf.h>
#include    <dmf.h>
#include    <dm.h>
#include    <dmp.h>
#include    <dm2d.h>
#include    <dmtcb.h>
#include    <dmucb.h>
#include    <dm2t.h>
#include    <dm2r.h>
#include    <dm2f.h>
#include    <dm0c.h>
#include    <dmfjsp.h>
#include    <dm1b.h>
#include    <dmve.h>
#include    <dm0d.h>
#include    <dm0j.h>
#include    <dmpp.h>
#include    <dm1cx.h>
#include    <dm0l.h>
#include    <sxf.h>
#include    <dma.h>
#include    <dmd.h>
#include    <dm0m.h>
#include    <dmftrace.h>
#include    <dmckp.h>
#include    <dmfrfp.h>
#include    <dmxe.h>
#include    <dm0llctx.h>
#include    <lgclustr.h>
#include    <dmfacp.h>
#include    <dmse.h>
#include    <dm2umct.h>
#include    <dm2umxcb.h>
#include    <dm2u.h>

/*
**  Name: DMFRFP.C - DMF Roll Forward Processor.
**
**  Description:
**
**	Prototype definitions in DMFJSP.H.
**
**      The file contains the routines that manages the recovery of a database
**	from a checkpoint and a series of journal files.  The database is
**	exclusively locked during the process.  No logging and locking activity
**	is generated except for a database lock held by the RFP.
**
{@func_list@}...
**
**
**  History:
**      22-oct-1986 (Derek)
**          Created for Jupiter.
**	16-dec-1988 (rogerk)
**	    Fix Unix compile warnings.
**	7-feb-1988 (sandyh)
**	    Fix RFP on multiple .JNL files.
**	21-jan-1989 (Edhsu)
**	    Add code to support online backup.
**	22-may-1989 (Sandyh)
**	    Fixed error messaging problems & comments.
**	 2-oct-1989 (rogerk)
**	    Added handling of DM0LFSWAP log records.
**	15-nov-1989 (walt)
**	    Fix handling of dump files with -c and -j.  The dump files
**	    were being used even in cases where the checkpoints weren't
**	    being brought in.
**	17-nov-1989 (rogerk)
**	    Don't give error message for unexpected log record when find
**	    solitary SBACKUP or EBACKUP log record in process_record.
**	22-nov-1989 (rogerk)
**	    Fixed online checkpoint problem - we were only applying one
**	    dump file during rollforward.
**	    When finished with dump file, break out of read loop so we can
**	    process the next dump file rather than returning from routine.
**	10-jan-1990 (rogerk)
**	    Save the jnl_fil_seq and dmp_fil_seq from their initial values
**	    in the config file - rather than using jnl_last and dmp_last -
**	    which may be zero if the current checkpoint required no journal
**	    or dump processing.
**	    Restore entire journal and dump history rather than just last
**	    section - in case we support going back to previous checkpoints -
**	    we don't want to lose history entries.  Also, with online backup
**	    the saved version of the config file may not have the new history
**	    entries.
**	    Added RFP structure fields to facilitate these changes.
**	29-apr-1990 (walt)
**	    Several changes to prepare(), restore_ckp():
**	    Reordered some steps so that rollforwarddb can restore all the
**	    checkpoint locations concurrently.  Formerly the default location
**	    had to be restored first to get the config file for information
**	    about the locations.  Now we save a copy of the config file in
**	    the dump location for all checkpoints (not just online backups)
**	    and use it first to get the location information.  This copy
**	    of the config file must be "re-implanted" into the default location
**	    after the restore because it's more dependable than the one
**	    saved during the checkpoint if the backup was an online backup.
**	4-may-90 (bryanp)
**	    Added the ability to roll-back to a checkpoint other than the most
**	    recent (through the un-documented #c flag). Used this new feature
**	    to enhance the checkpoint sequence number selection to be more
**	    tolerant of bad checkpoints. If the checkpoint we're planning to
**	    roll-forward from is invalid, try the previous checkpoint, and so
**	    forth, until we either find a valid checkpoint or run out of
**	    checkpoints. This makes checkpoint failure a much less serious
**	    problem.
**	    Created internal routine get_ckp_sequence() to modularize changes.
**	    Moved the rename of .cnf ==> .rfc back to prepare. Changed
**	    restore_ckp() to reset history & open_count after the restore
**	    rather than before since the restore overwrites the .cnf file.
**	    Taught rollforward how to recover the config file from the copy
**	    in the dump directory if the one in the database directory is
**	    unusable. Changed the arguments to dm0d_config_restore to allow
**	    rollforward to request the appropriate config file.
**	    Added UPDTCOPY_CONFIG argument to config_handler() and used it to
**	    make an up-to-date backup of the config file at the successful
**	    end of roll-forward. Note that making the copy here means that
**	    we don't have to update our config file backup during 'DO'
**	    processing in the config file recovery routines.
**	    Changed the meaning and usage of 'jnl_first' and 'jnl_last' in the
**	    RFP structure to support rolling forward multiple journal histories.
**	9-jul-1990 (bryanp)
**	    Fixed up some error-logging problems and enhanced some tracing so
**	    that errors encountered in rollforward are more completely
**	    reported.
**	5-oct-1990 (bryanp)
**	    Bug #33422:
**	    My fixes of 4-may-1990 broke "rollforwarddb -c +j". In this case,
**	    the code must handle a jnl_history_no value of -1 from the
**	    get_ckp_sequence() routine. Fixed.
**      19-nov-1990 (bryanp)
**          Fix use of 'err_code' in delete_files.
**      25-feb-1991 (rogerk)
**          Add check for rollforward on journal_disabled database.  Give
**          a warning message.
**      8-may-1991 (bryanp)
**          Add support for DM0LBPEXTEND record to online backup (b37428)
**      14-may-1991 (bryanp)
**          B34150 -- when we encounter an error, report it to the user, in
**          addition to logging it in the error log.
**      14-may-1991 (bryanp)
**          Fix errors noticed by lint, and by other compilers.
**      16-jul-1991 (bryanp)
**          B38527: Add support for DM0L_OLD_MODIFY & DM0L_OLD_INDEX.
**          Also, support rollforward of DM0L_SM1_RENAME & DM0L_SM2_CONFIG.
**          Support rollforward of new record DM0L_SM0_CLOSEPURGE.
**      15-aug-1991 (jnash) merged 24-sep-1990 (rogerk)
**          Merged 6.3 changes into 6.5.
**      15-aug-1991 (jnash) merged 14-jun-1991 (Derek)
**          Added supoprt for the new log records for the allocation project.
**	27-may-1992 (rogerk)
**	    Add check for DM0L_P1 (prepare to commit) log record during
**	    journal record processing (apply_j_record).  While the prepare
**	    log records should not really be journaled, there existed a
**	    6.3/6.4 bug that would sometimes cause them to be (B43916).
**	    The P1 record was added to the list of legal journal records
**	    to encounter during rollforward.  It is just ignored if found.
**	8-july-1992 (ananth)
**	    Prototyping DMF.
**      14-jul-1992 (andys)
**          Set status explicitly to E_DB_OK in DM0LP1 and DM0LALTER cases
**          in apply_j_record.
**	3-oct-1992 (bryanp)
**	    Re-arrange includes to share transaction ID type definition.
**	04-nov-92 (jrb)
**	    Renamed DCB_SORT to DCB_WORK.
**	19-nov-1992 (robf)
**	    Update security auditing to use new dma_write_record interface.
**	03-dec-1992 (jnash)
**	    Reduced logging project.  Move log record address from journal
**	    record into dmve->dmve_lg_addr.  Remove FSWAP log records.
**      11-dec-1991 (bentley, pholman)
**          ICL DRS 6000's require NO_OPTIM to prevent the complier failing
**	    during code generation: compiler error: "no table entry for op =
**          Fatal error in /usr/ccs/lib/cg,  Status 06.
**	14-dec-1992 (rogerk)
**	    Include dm1b.h before dm0l.h for prototype definitions.
**	10-feb-1993 (jnash)
**	    Add new verbose flag to dmd_log calls.  Use new dmd calls to
**	    format log records rather than using inline code.
**	    Add abortsave and savepoint log records to list of journal recs.
**	    Changed error handling action when rollforward encounters an
**	    unknown record type to not halt, but continue forward under the
**	    assumption that the record was unimportant.
**	    Fix compile warnings in DI and ME calls.
**	22-feb-1993 (jnash)
**	    Set logging during (normal) rollforward.
**	15-mar-1993 (rogerk)
**	    Reduced Logging - Phase IV:
**	    Added support for rollback phase at the end of rollforward
**		processing.  Added rollback_open_xacts routine and its
**		sub-functions.
**	    Changed record qualification checks for -b and -e flags.  Moved
**		checks up one level to read_journals.  We now halt all
**		processing when the first record which exceeds -e is found
**		and then rollback all open xacts.  (We used to continue
**		rolling forwarward all those xacts which began before the -e
**		time and ignored all those xacts which started after the -e
**		time).
**	    Added direction parameter to dm0j_open routine.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**		Altered dm0j_merge calling sequence in order to allow auditdb
**		    to call dm0j_merge.
**		Cast some parameters to quiet compiler complaints following
**		    prototyping of LK, CK, JF, SR.
**	26-apr-1993 (jnash)
**	    Add support for no logging during rollfoward via -nologging.
**	24-may-1993 (andys)
**	    Pass appropriate logfile blocksize to dmd_log and dmd_format_log
**	    rather than using hardwired 4096.
**	24-may-1993 (bryanp)
**	    Include <pc.h> before <lk.h> to pick up PID definition.
**	21-jun-1993 (rogerk)
**	    Moved rollforward processing of the config file jnl, ckp and dump
**	    history to be done immediately after restoring the checkpoint
**	    rather than at the end of rollforward processing.  This is required
**	    since the archiver will need up-to-date config file information
**	    if it should archive records written by rollforward recovery.
**	    Created new routine post_ckp_config to update the config file and
**	    moved to there most of the cnf processing from the complete routine.
**	    Created new routine validate_nockp that checks for the legality
**	    of bypassing config restore and cleaned up some of its checks
**	    (although I think these checks are still wanting in completeness)
**	    and added error handling.  Rewrote some of the main rfp loop and
**	    added comments.
**	    Added handling of Before Image CLR's in rollforward processing.
**	26-jul-1993 (bryanp)
**	    We no longer log a closedb log record, so we no longer need the
**		'lx_id' argument to dm0l_opendb() and dm0l_closedb().
**
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (rogerk)
**	  - Added support for databases which have been previously been marked
**	    inconsistent and are now being rolled forward to correct that
**	    event.  When a database cannot be recovered, any open transactions
**	    are terminated by writing an ET record with an INCONSIST flag.
**	    The transactions have not actually been rolled back so when we
**	    rollforward the database we will rebuild the incomplete xacts.
**	    Rollforward now ignores these INCONSIST ET records so that a true
**	    transaction rollback will be done; along with the writing of CLR
**	    records and a true ET.
**	  - Added post transaction processing to delete temporary delete
**	    files created by DDL statements.  Added ETA list structures
**	    and routines.  When an FRENAME log record is found which renames
**	    a file to a delete suffix '.dXX', an entry is pended to the
**	    transaction et action list.  This entry specifies to delete the
**	    temp file when the ET record is processed.
**      11-aug-93 (ed)
**          added missing includes
**	23-aug-1993 (jnash)
**	    Modify ABORTSAVE handling so that SAVEPOINT log records needn't
**	    be journaled.
**	23-aug-1993 (rogerk)
**	    Added DMU, ALTER log records to list of legal rollforward
**	    operations.
**	    Took out code which mapped all delete files to filenames
**	    constructed with statement number zero.  Rollforward now
**	    reexecutes DDL with the same files and statement number as
**	    the original transaction.
**	20-sep-1993 (andys)
**	     Fix arguments to ule_format call.
**	20-sep-1993 (jnash)
**	    Move DMFRFP_VALID_CHECKPOINT to dmfjsp.h, rename to
**	    DMFJSP_VALID_CHECKPOINT for use by alterdb.
**	20-sep-1993 (rogerk)
**	  - Removed RFP_ETA list, it is now attached to the dmve control block
**	    and contains more than just end-transaction events.
**	  - Add new dmve_rolldb_action list field in the dmve control block.
**	    This is now initialized in init_recovery and actions are pended
**	    to it in individual recovery routines.  The end transaction
**	    handler now looks through this list for file delete actions to
**	    to process.
**	  - Added tx argument to apply_j_record so that tranid can be placed
**	    in the dmve control block for the dmve recover calls.  This was
**	    needed so that rolldb event actions could store the proper tranid.
**	21-sep-1993 (rogerk)
**	    Moved rfp_add_action, rfp_lookup_action, and rfp_remove_action
**	    routines out of this module and to dmveutil.c so that executables
**	    which do not include dmfrfp will link properly.
**	18-oct-93 (jrb)
**          Added support for DM0LEXTALTER.
**	18-oct-1993 (jnash)
**	    One ule_lookup left garbage at the end of the line, so terminate it
**	18-oct-1993 (rmuth)
**	   - CL prototype, add <dm2f.h>.
**	   - Add journal time to the "we are alive" status message.
**	22-nov-1993 (jnash)
**	  - B56095.  Check jnl_first_jnl_seq during rolldb #c operations to
**	    catch operator errors in attempting to rollforward from a point
**	    where the database was not journaled.
**	  - B57056.  In post_ckp_config(), set DSC_DISABLE_JOURNAL according
**	    to how it was set at the start of the rolldb. The version
**	    in the dump directory may have the wrong status.
**	  - Remove ule_format() call that caused error messages to be written
**	    to errlog.log twice.
**	31-jan-1994 (jnash)
**	  - In process_et_action(), write message to error log when
**	    committing the tx that deletes a non-journaled table.
**	  - Restore the config file dsc_last_tbl_id from the old version
**	    of the config file.  We can't rely on the recovery of this field
**	    by dmve_create(), as it is not accurate for non-journaled tables
**	    and direct dm2t_get_tabid calls.
**	  - Permit rollforwarddb #f recovery if jnl_first_jnl_seq == 0.
**	    This permits recovery attempts in "journal disabled" or SET
**	    NOLOGGING states.
**	  - Use only ET timestamps when comparing -b/-e times.  This
**	    provides behavior that is consistent with AUDITDB, and also
**	    provides predictable results when a "-b" recovery follows a
**	    rolldb "-e" recovery.
**	  - Fix RFP_AFTER_ETIME and RFP_EXACT_ETIME bit assignments.
**	  - Fix truncated error messages.
**	18-feb-94 (stephenb)
**	    Move call to dma_write_audit() to a place where we are sure the
**	    rollforward has completed and remove failure case, we log this
**	    elsewhere.
**	21-feb-1994 (jnash)
**	  - Fix rollforwarddb -e problems:
**	     o Add rfp_etime_lsn as means to save point at which undo should
**	       begin (the lsn of journal record >= -e time).
**	     o In cleanup_undo_phase(), bypass closure of logging system, etc,
**	       when RFP_NOLOGGING.
**	     o Fix exact time matches by converting ET stamps to
**	       strings, then zeroing ms part, then converting back again
**	       prior to time comparison.
**	  - Add support for -start_lsn and -end_lsn options.
**	  - Add support for recoveries of the form:
**	    rollforwarddb #cnn -c -b (or -start_lsn).
**      14-mar-94 (stephenb)
**          Add new parameter to dma_write_audit() to match current prototype.
**	20-jun-1994 (jnash)
**	    fsync project: Open recovered databases requesting sync-at-end.
**	    This is a performance optimization permitting fsync writes
**	    on Unix.
**	14-sep-1994 (andyw)
**	    Partial Backup/Recovery Project:
**		RE-WRITE: for Table level Rollforward
**      18-nov-1994 (alison)
**          Database relocation project:
**              Relocation of default ckp,dmp,jnl,work files
**              Relocation of entire database (make a copy)
**              Relocation of table
**      22-nov-1994 (juliet)
**          Added calls to the error reporting/logging routine (ule_format).
**          Created error messages:
**              RFP_INV_ARG_FOR_DB_RECOVERY,
**              RFP_NO_CORE_TBL_RECOVERY,
**              RFP_INVALID_ARGUMENT,
**              RFP_INCONSIS_DB_NO_TBLRECV,
**              RFP_VIEW_TBL_NO_RECV,
**              RFP_GATEWAY_TBL_NO_RECV,
**              RFP_RECOVERING_MAX_TBLS,
**              RFP_DUPLICATE_TBL_SPECIFIED,
**              RFP_CANNOT_OEPN_IIRELATION,
**              RFP_CANNOT_OPEN_IIRELIDX.
**	13-Dec-1994 (liibi01)
**          Cross integration from 6.4, (Bug 56364).
**          Mark database inconsistent if we have failed to rollforward
**          because the expected alias files couldn't be found.
**	20-dec-1994 (shero03)
**	    b65976 - config file was left closed when draining a journal
**      04-jan-1995 (alison)
**          need to call rfp_open_database and rfp_close_database from
**          dmfreloc.c, no longer static routines.
**          rfp_tbl_restore_ckp() if relocating the table, make sure the
**          file is deleted from the old location.
**          dmfrfp(): rfp_relocate() not called from here anymore.
**      06-jan-1995 (shero03)
**          Support Partial Checkpoint                                  
**      10-jan-1995 (stial01)
**          If recovery disallowed for table error is RFP_NO_TBL_RECOV. 
**	28-jan-1995 (shero03)
**	    Correct checking the wait flag (B 66533)
**      16-jan-1995 (stial01)
**          BUG 66577: Added rfp_get_index_info():
**              For all tables being recovered, their indexes should also 
**              be recovered unless -noseconary_index is specified.
**          Added rfp_get_blob_info():
**              For all tables being recovered, their blob tables should also
**              be recovered unless -noblobs is specified.
**          BUG 66577: rfp_update_tbls_status() if table is being recovered  
**              without indexes and/or blobs, the indexes and/or blob tables 
**              should be marked inconsistent.
**          Delete functions rfp_prepare_get_tbls_info(), 
**              rfp_prepare_check_blob_info() and 
**              rfp_prepare_check_sec_ind_info(). 
**              These routines are never called.
**          BUG 66581: display correct error if view specified
**          BUG 66404: Changes so partial table recovery error messages are 
**              logged and displayed correctly.
**          BUG 66401: rfp_filter_log_record() dont ignore begin/end trans recs
**              Test for tbl_id for other log recs that must be processed.
**              rfp_tbl_filter_recs() not testing status correctly
**          BUG 66407: rfp_recov_invalid_tbl() check for recovery on table 
**              for which recovery is disallowed.
**          BUG 66403: rfp_prepare_tbl_recovery() apply proper case to tbl names
**          BUG 66473: rfp_prepare_db_recovery() disallow db rollforward from 
**              table checkpoint. Also ckp_mode in config is now bit pattern.
**	09-mar-1995 (shero03)
**	    Bug B67276
**	    Support relocation of dmp, jnl, or ckp directories
**      10-mar-1995 (stial01)
**          BUG 67411: rollforward/relocate journalled table gets table
**          open errors because jnl/dump records contain location numbers 
**          that are the index into the extent info in the config file.
**          We MUST restore the checkpoint to the new location, since
**          there may actually be a media error with the old location.
**          So the only way to fix this problem is to adjust location numbers
**          in the jnl/dmp records. A new routine rfp_fix_locid() was added.
**          For each jnl/dmp record processed, adjust the location number
**          in the record if the location was relocated.
**          Also moved the update of iirelation.relloc and iidevices.devloc
**          to immediately after the checkpoint is restored, although this
**          is not what was causing the problem.
**	22-mar-1995 (shero03)
**	    Bug #67410
**	    Ensure you delete the files in all locations before restoring
**	    the data.
**      23-mar-1995 (stial01)
**          rfp_add_tblcb() Don't fix prev pointer of htotalq unless there
**          is an element on the htotalq. Otherwise we trash 
**          hcb->tblhcb_hash_array[3].
**      11-apr-1995 (stial01) 
**          db_restore_ckp() Another fix to support relocation of the 
**          dmp directory.
**	11-may-1995 (thaju02)
**	    bug #68588 - table rolldb to a point in time marks table as
**	    logically inconsistent.  Upon table rolldb "to now" should
**	    reset logically inconsistent status bit.  See Partial 
**	    Recovery: Table Level Rollforwarddb paper (R. Muth, Feb. 94),
**	    Section 2.2 New Table States
**	14-may-1995 (smiba01)
**	    Added dr6_ues to NO_OPTIMISE MING hints.
**      15-may-1995 (popri01)
**          Corrected call to ule_format to pass table name as array instead
**          of as structure for DM135F error messsage parameter.
**      20-may-95 (hanch04)
**          Added include dmtcb.h
** 	01-jun-1995 (wonst02) 
**	    Bug 68886, 68866
** 	    Added rfp_recov_chkpt_tables() to ensure that the tables
** 	    specified by user were in the table-level checkpoint.
** 	08-jun-1995 (wolf)
**	    Remove recently-added, non-portable fprintf debugging statements.
**	30-jun-1995 (shero03)
**	    Added -ignore flag while processing journal records
**	30-aug-1995 (nick)
**	    Parameter passed for error text E_DM135F should be a string for all
**	    platforms not just AIX !
**	21-sep-1995 (johna)
**	    Back out the -ignore flag feature (from change 418541)
**      20-july-1995 (thaju02)
**          bug #69981 - when ignore argument specified, output error
**          message to error log, and mark database inconsistent
**          for db level rolldb or table physically inconsistent
**          for tbl level rolldb, since rolldb will continue with 
**          application of journal records.
**          bug #52556 - rolldb from tape device, returns message
**          '...beginning restore of tape...of 0 locations...'
**          now count number of locations recovering from.
**       19-oct-1995 (stial01)
**          rfp_allocate_tblcb() relation tuple buffer allocated incorrectly.
**	04-aug-1995 (thaju02)
**	    Implemented -new_table argument for table level rolldb with
**	    relocation.
**	 1-aug-1995 (allst01)
**	    Put back robf's change of 26-jul-94 for su4_cmw
**	    which was removed by change 41. viz
**	    19-jul-94 (robf)
**          Removed call to E_DM130F status message, it didn't add much
**          (since complete() already issued a success message) and
**          cluttered the error log.
**	06-feb-1996 (morayf)
**	    Made check_params structure a module typedef (CHECK_PARAMS)
**	    and made the two functions concerned use this to increase
**	    maintainability. Also some compilers don't like multiple
**	    instances of the same typedef name in the same module.
**	26-feb-1996 (nick)
**	    Bogus error check stopped '#f' working. #74371
**	17-may-1996 (hanch04)
**	    move <ck.h> after <pc.h>
**	24-may-1996 (nick)
**	    We attempted to close the config file twice when aborting
**	    'cos the database in question hasn't been checkpointed. #76683
**	13-jun-96 (nick)
**	    LINT directed changes.
**	 3-jul-96 (nick)
**	    More error handling fixes.
**	 4-jul-96 (nick)
**	    Initialise iirelation's RCB->rcb_k_mode. #77557
**	10-jul-96 (nick)
**	    Yet more error handling fixes.
**	23-aug-96 (nick)
**	    Don't check DSC_CKP_INPROGRESS for table level rollforward
**	    as this has no meaning in this context.  Note however that there 
**	    is nothing to prevent a '-c +j -table' operation when a prior 
**	    '+c -j -table' operation hasn't yet been performed and this should
**	    probably be addressed.
**	24-oct-96 (nick)
**	    Don't implement certain validations which are meaningless if
**	    we have '-j' specified. #75986
**	    Some LINT stuff.
**	6-march-1997 (angusm)
**	    Fix up config file with locations added since checkpoint
**	    (bug 49057)
*	20-sep-1996 (shero03)
**	    Support RTree put/delete/replace
**	23-sep-1996 (canor01)
**	    Moved global data definitions to dmfdata.c.
**	05-nov-1996 (shero03)
**	    Add additional RTree support (errouneously omitted).
**	13-mar-1997 (shero03)
**	    Bug from Beta site: rollforwarddb after relocate -new_ckp_location
**	    fails to find the checkpoint file after it deletes the database.
**	    Changed db_restore_ckp to use the present ckp location
**	09-Jul-1997 (shero03)
**	    Fixed a bug that TPC-C testing uncovered when extending 
**	    by 24 locations.
**      15-aug-97 (stephenb)
**          add DML_SCB parameter to dm2t_open() call.
**	25-Aug-1997 (jenjo02)
**	    Added log_id parm to dm2t_unfix_tcb() calls.
**	    Added log_id (LG_LXID) to dm2f_build_fcb(), dm2f_open_file(),
**	    dm2f_create_file(), dm2f_delete_file(), dm2f_rename(),
**	    dm2f_add_fcb() dm2f_release_fcb() prototypes so that it 
**	    can be passed on to dm2t_reclaim_tcb().
**	20-aug-19978 (rigka01) - bug #84583
**                don't process record where BT > -e
**      08-Dec-1997 (hanch04)
**          Use new la_block field in LG_LA
**          for support of logs > 2 gig
**	23-june-1998(angusm)
**	    bug 82736: change interface to dmckp_init()
**      28-Jul-1998 (nicph02) Cross-integration
**          Implemented 2 environment variables (logicals) having to do with
**          optionally giving more messages during the rollforward of
**          journals. (Sir 86486).
**	09-sept-98 (mcgem01)
**	    Bug 93106: Fix grammatical error.
**	16-dec-98 (nicph02)
**	    Missed some stuff when cross-integrating SIR 86486.
**      22-Feb-1999 (bonro01)
**          Replace references to svcb_log_id with
**          dmf_svcb->svcb_lctx_ptr->lctx_lgid
**      21-jan-1999 (hanch04)
**          replace nat and i4 with i4
**	30-mar-1999 (hanch04)
**	    Added support for raw locations
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
**	21-apr-1999 (hanch04)
**	    Replace STindex with STchr
**      28-apr-1999 (hanch04)
**          Replaced STlcopy with STncpy
**      10-May-1999 (stial01)
**          Added direction to dm0d_open call, Removed direction from 
*           dm0d_read call. Removed call to dmd_scan, dm0d_open now saves
**          the end block number (related change 438561 for B91957)
**	10-may-1999 (walro03)
**	    Remove obsolete version string dr6_ues.
**	05-Jul-99 (carsu07)
**	    When checking if journals should be applied during a rollforwarddb
**	    also check for the #f flag.  (Bug 44250)
**	10-nov-1999 (somsa01)
**	    Changed PCis_alive to CK_is_alive.
**	15-Dec-1999 (jenjo02)
**	    Added DB_DIS_TRAN_ID parm to LGbegin() prototype.
**      28-feb-2000 (gupsh01)
**          Enabled check for owner along with the table name
**          so that the table names in -table option can also
**          be specified as owner.tablenames .
**	18-Jan-2001 (jenjo02)
**	    Subvert calls to dma_write_audit() if not C2SECURE.
**	25-Mar-2001 (jenjo02)
**	    Productized RAW locations.
**      23-may-2001 (stial01)
**          rfp_tbl_restore_ckp() start DMXE_READ transaction (b104771)
**	30-may-2001 (devjo01)
**	    Add lgclustr.h for LGcluster() macro. s103715.
**      04-oct-2002 (devjo01)
**          Add cx.h for CXcluster_enabled declare.
**      22-jan-2003 (stial01)
**          Give more diagnostics if errors applying or undoing journal records,
**          and mark database inconsistent if db rollforward fails. (b107975)
**      23-jan-2003 (stial01)
**          rfp_error_diag() pass verbose flag TRUE for hexdump of log record
**      24-jan-2003 (stial01)
**          add_tx() Fixed referenced to undefined variable
**      24-jan-2003 (stial01)
**          After E_DM1306_RFP_APPLY_RECORD error, mark db inconsistent --
**          try to rollback open transactions.
**      10-feb-2003 (stial01)
**          Added exception handler for journal processing.
**          Fixed references in DMF_RFP for saving redo end lsn.
**      21-feb-2003 (stial01)
**          Fixed database name passed in new TRformats
**      18-apr-2003 (stial01)
**          Error if DM0LBTPUTOLD, DM0LBTDELOLD log records
**          Fixed MEfill args
**	17-jul-2003 (penga03)
**	    Added include adf.h.
**      08-Aug-2003 (inifa01)
**          Misc GENERIC build change
**          moved include of 'dm1b.h' before include of 'dmve.h'
**	14-aug-2003 (jenjo02)
**	    dm2t_unfix_tcb() "tcb" parm now pointer-to-pointer,
**	    obliterated by unfix to prevent multiple unfixes
**	    by the same transaction and corruption of 
**	    tcb_ref_count.
**	23-Sep-2003 (hanje04)
** 	    Corrected bad X-integration of 08-Aug-2003 by inifa01
**	24-Sep-2003 (hanje04)
**	    DOH!
** 	    Again corrected bad X-integration of 08-Aug-2003 by inifa01
**	17-nov-2003 (abbjo03)
**	    In rfp_recov_invalid_tbl(), convert relmoddate to a TM_STAMP to do
**	    a generic comparison with TMcmp_stamp().
**      18-feb-2004 (stial01)
**          Fixed incorrect casting of length arguments.
**	17-mar-2004 (somsa01)
**	    Make sure we pass a TCB pointer as the first argument to
**	    dm2t_unfix_tcb().
**	29-mar-2004 (somsa01)
**	    Undid last change, proper change was to pull in the fix to dm2t.h
**	    which properly updates the prototype definition.
**	28-nov-2005 (thaju02)
**	    In rfp_prescan_btime_qual(), do not remove xaction from list 
**	    if et is equal to -b specified; match auditdb behavior. (B115565)
**	    Also resolved some compilation warnings.
**	5-Apr-2004 (schka24)
**	    Add code for journal redo of repartitioning modify.
**	29-Apr-2004 (gorvi01)
**		Added case for DM0LDELLOCATION.
**	06-May-2004 (hanje04)
**	    Only print "Start processing journal file sequence" message
**	    during PROCESS phase in rfp_read_journals_db() otherwise we
** 	    end up with duplicate messages.
**	12-may-2004 (thaju02)
**	    Changed E_DM9666 to online modify specific E_DM9CB1.
**	30-jun-2004 (thaju02)
**	    Account for rolldb of online modify of the same
**	    table multiple times within the same chkpt.
**	    Make online context block unique on table id
**	    and bsf lsn. Modified prescan_records().
**      01-sep-2004 (stial01)
**	    Support cluster online checkpoint
**	30-aug-2004 (thaju02)
**	    Added rfp_prescan_btime_qual() & remove_qualtx() to prescan 
**	    journal files to determine xactions which satisfy the -b
**	    qualification and maintain the list of qualifying xactions.
**	    (B111470/INGSRV2635)
**	01-Sep-2004 (bonro01)
**	    Fixed x-integration of change 471359; changed longnat to i4
**	09-Sep-2004 (hanje04)
**	    BUG 112987
**	    If I_DM1369 is returned by dmckp_init() the error message printed
**	    out should state that the template file pointed to by II_CKTMPL_FILE
**	    is not owned by the installation owner which may now not be ingres.
**	30-Sep-2004 (schka24)
**	    Fix some compiler warnings.
**      21-dec-2004
**          Print message when rollforward invalidates journals.
**      07-Jan-2005 (hanje04)
**         BUG 112987
**         LOgetowner is not defined on Windows so wrap code checking
**         ownership of cktmpl.def in CK_OWNNOTING as it is in dmckp.c
**	10-jan-2005 (thaju02)
**	    Rolldb of online modify of partitioned table. Hang list
**	    of osrc blocks from octx. osrc contains rnl lsn array info
**	    extracted from log records to be passed to dmv_remodify.
**	    Added rfp_osrc_close.
**	29-Mar-2005 (hanje04)
**	    BUG 112987
**	    Checkpoint template should be owned by EUID of server as
**	    ownership of ckptmpl.def can be "changed" with missuse of II_CONFIG
**	15-Apr-2005 (hanje04)
**	    BUG 112987
**	    Use IDename instead of IDgeteuser to get effective user of process.
**    29-Mar-2005 (hanje04)
**       BUG 112987
**       Checkpoint template should be owned by EUID of server as
**       ownership of ckptmpl.def can be "changed" with missuse of II_CONFIG
**      07-apr-2005 (stial01)
**          Add exception handling to rfp_read_journals_tbl
**          Added support for -on_error_ignore_index, Fix queue handling
**      05-may-2005 (stial01)
**          Added support for -on_error_prompt, Fixed error handling
**      10-may-2005 (stial01)
**          Removed extra EXdelete
**      19-may-2005 (stial01)
**          Fixed TRformat args
**	20-May-2005 (thaju02)
**	    rfp_read_journals_db: For process phase, restore rfp_status. 
**	    (B114487)
**	15-Aug-2005 (jenjo02)
**	    Implement multiple DMF LongTerm memory pools and
**	    ShortTerm session memory.
**	    Rename TBL_HCB to TBL_HCB_CB
**      25-oct-2005 (stial01)
**          Always CONTINUE_IGNORE_TABLE jnl/dump errors for secondary index 
**          Set DSC_INDEX_INCONST/DSC_TABLE_INCONST if index/table err ignored.
**      20-mar-2006 (stial01)
**          Added rfp_sconcur_error(), SCONCUR error handling
**      22-mar-2006 (stial01)
**          rfp_sconcur_error() Fix initialization of loc_sequence.
**      03-nov-2006 (hord03) Bug 116231
**          On a clustered installation, if journalling was disabled on a
**          DB by ALTERDB, the journal files must be merged before a
**          rollforward/ckpdb begins.
**      12-apr-2007 (stial01)
**          Added support for rollforwarddb -incremental
**       25-Apr-2007 (kibro01) b115996
**          Ensure the misc_data pointer in a MISC_CB is set correctly and
**          that we correctly allocate sizeof(DMP_MISC), not sizeof(DM_OBJECT)
**          since that's what we're actually using (the type is MISC_CB)
**      12-jul-2007 (stial01)
**	    Fixes for rollforwarddb -incremental  (b118721) 
**      10-Aug-2007 (horda03) Bug 118933
**          Use supplied timestamp comparison macros to compare timestamps. On
**          some platforms (VMS for example) the High time is the second 4 bytes
**          and on others the 1st four bytes. This can lead to E_DM135F.
**      09-oct-2007 (stial01)
**          More fixes for rollfowarddb -incremental 
**          rfp_db_complete() Sometimes we NEED a lock on config before updating
**      23-oct-2007 (stial01)
**          Support table level CPP/RFP for partitioned tables (b119315)
**      04-mar-2008 (stial01)
**          Added validate_incr_jnl()
**      11-mar-2008 (stial01)
**          Journal switch after final incremental rollforwarddb
**          Fix jnl history entry in config for incremental journals
**      13-mar-2008 (stial01)
**          Fixed blk_seq for incremental journals
**      14-may-2008 (stial01)
**          Added rfp_er_handler(), can be called for more robust error handling
**      27-may-2008 (stial01)
**          Disallow JSX_CKP_SELECTED with JSX2_INCR_JNL (for now)
**          Fixed the jnl_fil_seq to search for (don't always start at 1)
**	11-Apr-2008 (kschendel)
**	    dm2r position call updated, fix here.
**	30-May-2008 (jonj)
**	    LSNs are 2 u_i4's; display them in hex rather than
**	    signed decimal.
**      03-jun-2008 (stial01)
**          Drain journals for first incremental rfp.
**          Don't merge journals during incremental rfp. (B120458)
**      27-jun-2008 (stial01)
**          rfp_db_complete() Fix jnl_la if incremental rollforwarddb
**          Move reset DSC_ROLL_FORWARD so it is also done for final incr rfp
**          Changes to advance log Last LSN if necessary after incr rfp
**      16-jul-2008 (stial01)
**          Save/Restore incremental transaction context to 'jnnnnnnn.rfp'
**	31-Oct-2008 (jonj)
**	    SIR 120874 Modified to use new DB_ERROR based uleFormat
**	    and functions.
**	13-Nov-2008 (jonj)
**	    SIR 120874 Change function prototypes to pass DB_ERROR *dberr
**	    instead of i4 *err_code. Standardize on jsx->jsx_dberr.
**      16-nov-2008 (stial01)
**          Redefined name constants without trailing blanks.
**	18-Nov-2008 (jonj)
**	    dm0d_?, dm0c_?, dm0j_?, dm2d_? functions converted to DB_ERROR *
**	21-Nov-2008 (jonj)
**	    SIR 120874: dmxe_?, dmf_rfp_? functions converted to DB_ERROR *
**	21-Nov-2008 (jonj)
**	    SIR 120874: dm0m_? functions converted to DB_ERROR *
**	25-Nov-2008 (jonj)
**	    SIR 120874: dm0l_? functions converted to DB_ERROR *
**	26-Nov-2008 (jonj)
**	    SIR 120874: dm2f_?, dm2t_?, dm2r_? functions converted to DB_ERROR *
**	08-Dec-2008 (jonj)
**	    SIR 120874: dma_? auditing functions converted to DB_ERROR *
**      04-aug-2009 (stial01)
**          rfp_check_lsn_waiter() Fixed DB_ERROR param to dm2u_load_table
**      01-apr-2010 (stial01)
**          Changes for Long IDs
*/

/*
** Global declarations
*/
GLOBALREF DMP_DCB *dmf_rfp_dcb;     /* Current DCB  */

/*  external  variables  */


#define	    OPEN_CONFIG		1
#define	    UPDATE_CONFIG	2
#define	    CLOSE_CONFIG	3
#define	    CLOSEKEEP_CONFIG	4
#define	    UNKEEP_CONFIG	5
#define	    OPENRFC_CONFIG	6
#define	    UPDTCOPY_CONFIG	7
#define     OPEN_LOCK_CONFIG    8

/*
**  Forward type references.
*/

typedef struct _RFP_FILTER_CB RFP_FILTER_CB;

typedef   struct
{
	DM_FILE     fname;
	bool        found;
} CHECK_PARAMS;

/*
** Name: RFP_FILTER_CB - Holds information needed to decide if this object
**                       is of interest to rollforward.
**
** Description:
**      Holds information needed to decide if this object is of interest to
**      DMF. Currently rollforward can filter on the following, in the
**      future we will probably add location, page, etc. Use a structure
**      as do not want to keep changing this interface.
**          a) Table id.
**
**
** History
**      14-sep-1994 (andyw)
**          Partial Backup/Recovery Project:
**              Created.
*/
struct  _RFP_FILTER_CB
{
    DB_TAB_ID           *tab_id;
};

/*
**  Definition of static variables and forward static functions.
*/

static DB_STATUS rfp_db_complete(
    DMF_RFP	    *rfp,
    DMF_JSX	    *jsx,
    DMP_DCB	    *dcb);

static  DB_STATUS   prepare(
    DMF_RFP	    *rfp,
    DMF_JSX	    *jsx,
    DMP_DCB	    *dcb);

static DB_STATUS read_journals(
    DMF_JSX	    *jsx,
    DMF_RFP	    *rfp,
    DMVE_CB	    *dmve);

static DB_STATUS rfp_read_journals_db(
    DMF_JSX	    *jsx,
    DMF_RFP	    *rfp,
    DMVE_CB	    *dmve);

static DB_STATUS rfp_read_journals_tbl(
    DMF_JSX	    *jsx,
    DMF_RFP	    *rfp,
    DMVE_CB	    *dmve);

static DB_STATUS rfp_read_incr_journals_db(
    DMF_JSX	    *jsx,
    DMF_RFP	    *rfp,
    DMVE_CB	    *dmve,
    i4		    *new_jnl_cnt);

static DB_STATUS read_dumps(
    DMF_JSX	    *jsx,
    DMF_RFP	    *rfp,
    DMVE_CB	    *dmve);

static	DB_STATUS validate_nockp(
    DMF_JSX	    *jsx,
    DMP_DCB	    *dcb);

static	DB_STATUS db_restore_ckp(
    DMF_RFP	    *rfp,
    DMF_JSX	    *jsx,
    DMP_DCB	    *dcb);

static	DB_STATUS db_post_ckp_config(
    DMF_RFP	    *rfp,
    DMF_JSX	    *jsx,
    DMP_DCB	    *dcb);

static DB_STATUS process_record(
    DMF_RFP	    *rfp,
    DMVE_CB	    *dmve,
    PTR		    record);

static DB_STATUS apply_j_record(
    DMF_RFP	    *rfp,
    RFP_TX	    *tx,
    DMVE_CB	    *dmve,
    DM0L_HEADER	    *record);

static DB_STATUS apply_d_record(
    DMF_RFP	    *rfp,
    DMF_JSX	    *jsx,
    DMVE_CB	    *dmve,
    DM0L_HEADER	    *record);

static VOID init_recovery(
    DMF_RFP	*rfp,
    DMP_DCB	*dcb,
    DMVE_CB     *dmve,
    i4	action);

static DB_STATUS config_handler(
    DMF_JSX	    *jsx,
    i4	    operation,
    DMP_DCB	    *dcb,
    DM0C_CNF	    **config);

static DB_STATUS delete_files(
    DMP_LOC_ENTRY   *location,
    char	    *filename,
    i4	    l_filename,
    CL_ERR_DESC	    *sys_err);

static DB_STATUS check_files(
    PTR      	    params,
    char	    *filename,
    i4	    l_filename,
    CL_ERR_DESC	    *sys_err);

static DB_STATUS get_ckp_sequence(
    DMF_JSX	*jsx,
    DM0C_CNF	*cnf,
    i4	*jnl_history_no,
    i4	*dmp_history_no);

static VOID	lookup_tx(
    DMF_RFP	*rfp,
    DB_TRAN_ID	*tran_id,
    RFP_TX	**return_tx);

static DB_STATUS add_tx(
    DMF_RFP		*rfp,
    DM0L_BT		*btrec);

static VOID	remove_tx(
    DMF_RFP		*rfp,
    RFP_TX		*tx);

static DB_STATUS rollback_open_xacts(
    DMF_JSX		*jsx,
    DMF_RFP		*rfp,
    DMP_DCB		*dcb,
    DMVE_CB		*dmve);

static DB_STATUS save_open_xacts(
    DMF_JSX		*jsx,
    DMF_RFP		*rfp,
    DMP_DCB		*dcb,
    DMVE_CB		*dmve,
    i4			last_jnl);

static DB_STATUS restore_open_xacts(
    DMF_JSX		*jsx,
    DMF_RFP		*rfp,
    DMP_DCB		*dcb,
    DMVE_CB		*dmve,
    i4			last_jnl);

static DB_STATUS validate_incr_jnl(
    DMF_JSX		*jsx,
    DMF_RFP		*rfp,
    DMP_DCB		*dcb,
    DMVE_CB		*dmve,
    i4			last_jnl);

static DB_STATUS
rfp_sync_last_lsn(
DMF_RFP		    *rfp,
DMF_JSX             *jsx,
DMP_DCB		    *dcb);

static DB_STATUS prepare_undo_phase(
    DMF_JSX		*jsx,
    DMF_RFP		*rfp,
    DMP_DCB		*dcb);

static DB_STATUS cleanup_undo_phase(
    DMF_JSX		*jsx,
    DMF_RFP		*rfp,
    DMP_DCB		*dcb);

static DB_STATUS undo_journal_records(
    DMF_JSX		*jsx,
    DMF_RFP		*rfp,
    DM0J_CTX		*jnl,
    DMP_DCB		*dcb,
    bool		*rollback_complete,
    DMVE_CB		*dmve);

static DB_STATUS apply_j_undo(
    DMF_JSX		*jsx,
    DMF_RFP		*rfp,
    DMP_DCB		*dcb,
    RFP_TX		*tx,
    DMVE_CB		*dmve,
    DM0L_HEADER		*record);

static VOID	qualify_record(
    DMF_JSX		*jsx,
    DMF_RFP		*rfp,
    DM0L_HEADER	    	*record);

static VOID	process_et_action(
    DMF_RFP		*rfp,
    RFP_TX		*tx);

static DB_STATUS gather_rawdata(
    DMF_JSX		*jsx,
    RFP_TX		*tx,
    DM0L_RAWDATA	*record);

static DB_STATUS reverse_gather_rawdata(
    DMF_JSX		*jsx,
    RFP_TX		*tx,
    DM0L_RAWDATA	*record);

static void delete_rawdata(
    RFP_TX		*tx);

DB_STATUS rfp_validate_cl_options(
    DMF_JSX         *jsx,
    DMF_RFP         *rfp,
    DMP_DCB	    *dcb);

static DB_STATUS rfp_prepare_validate_cnf_info(
    DMF_JSX         *jsx,
    DMP_DCB         *dcb,
    DM0C_CNF        *cnf);

static DB_STATUS rfp_prepare_db_recovery(
    DMF_RFP         *rfp,
    DMF_JSX         *jsx,
    DMP_DCB         *dcb);

static DB_STATUS rfp_init_tbl_hash(
    DMF_RFP		    *rfp);

static DB_STATUS rfp_prepare_tbl_recovery(
    DMF_RFP         *rfp,
    DMF_JSX         *jsx,
    DMP_DCB         *dcb);

static DB_STATUS rfp_prepare_drain_journals(
    DMF_JSX         *jsx,
    DMP_DCB         *dcb,
    DM0C_CNF        **config);

static DB_STATUS rfp_prepare_get_ckp_jnl_dmp_info(
    DMF_RFP         *rfp,
    DMF_JSX         *jsx,
    DM0C_CNF        *cnf,
    i4         jnl_history_no,
    i4         dmp_history_no);

static DB_STATUS rfp_recov_core_tbl(
    DMF_RFP         *rfp,
    DMF_JSX         *jsx,
    DMP_DCB         *dcb);

static DB_STATUS rfp_recov_invalid_tbl(
    DMF_RFP         *rfp,
    DMF_JSX         *jsx,
    DMP_DCB         *dcb,
    DM0C_CNF        *cnf,
    i4         jnl_history_no);

static DB_STATUS rfp_error(
    DMF_RFP             *rfp,
    DMF_JSX             *jsx,
    RFP_TBLCB		*tblcb,
    DMVE_CB		*dmve,
    i4             err_type,
    DB_STATUS           err_status);

static DB_STATUS rfp_sconcur_error(
    DMF_RFP             *rfp,
    DMF_JSX             *jsx,
    RFP_TBLCB		*tblcb,
    DMVE_CB		*dmve,
    i4             err_type,
    DB_STATUS           err_status);

static DB_STATUS rfp_allocate_tblcb(
    DMF_RFP             *rfp,
    RFP_TBLCB           **tablecb);

static VOID rfp_add_tblcb(
    DMF_RFP             *rfp,
    RFP_TBLCB           *tblcb );

static bool rfp_locate_tblcb(
    DMF_RFP             *rfp,
    RFP_TBLCB           **tblcb,
    DB_TAB_ID           *tabid);

static DB_STATUS rfp_update_tbls_status(
    DMF_RFP             *rfp,
    DMF_JSX             *jsx,
    DMP_DCB             *dcb,
#define		RFP_PREPARE_UPDATE_TBLS_STATUS          1L
#define		RFP_COMPLETE_UPDATE_TBLS_STATUS         2L
#define		RFP_UPDATE_INVALID_TBLS_STATUS		3L
    u_i4           phase);

static VOID rfp_display_recovery_tables(
    DMF_RFP             *rfp,
    DMF_JSX             *jsx );

static VOID rfp_display_no_recovery_tables(
    DMF_RFP             *rfp,
    DMF_JSX             *jsx );

static VOID rfp_display_inv_tables(
    DMF_RFP             *rfp,
    DMF_JSX             *jsx );

static DB_STATUS rfp_db_ckp_processing(
    DMF_RFP             *rfp,
    DMF_JSX             *jsx,
    DMP_DCB             *dcb);

static DB_STATUS rfp_ckp_processing(
    DMF_RFP             *rfp,
    DMF_JSX             *jsx,
    DMP_DCB             *dcb);

static DB_STATUS rfp_check_snapshots_exist(
    DMF_RFP             *rfp,
    DMF_JSX             *jsx,
    DMP_DCB             *dcb,
    i4             *number_locs);

static DB_STATUS rfp_tbl_ckp_processing(
    DMF_RFP             *rfp,
    DMF_JSX             *jsx,
    DMP_DCB             *dcb);

static DB_STATUS rfp_tbl_restore_ckp(
    DMF_RFP             *rfp,
    DMF_JSX             *jsx,
    DMP_DCB             *dcb);

static DB_STATUS rfp_tbl_post_ckp_processing(
    DMF_RFP             *rfp,
    DMF_JSX             *jsx,
    DMP_DCB             *dcb);

static bool rfp_tbl_filter_recs(
    DMF_RFP             *rfp,
    DMF_JSX             *jsx,
    RFP_FILTER_CB       *filter_cb,
    RFP_TBLCB           **tblcb );

static VOID rfp_init(
    DMF_RFP		*rfp,
    DMF_JSX		*jsx);

static VOID rfp_dump_stats(
    DMF_RFP                 *rfp,
    DMF_JSX                 *jsx );

static bool rfp_filter_log_record(
    DMF_RFP                 *rfp,
    DMF_JSX                 *jsx,
    DM0L_HEADER             *record);

static DB_STATUS rfp_read_tables(
    DMF_RFP             *rfp,
    DMF_JSX             *jsx,
    DMP_DCB             *dcb);

static DB_STATUS rfp_get_table_info(
    DMF_RFP             *rfp,
    DMF_JSX             *jsx,
    DMP_DCB             *dcb);

static DB_STATUS rfp_get_blob_info(
    DMF_RFP             *rfp,
    DMF_JSX             *jsx,
    DMP_DCB             *dcb);

DB_STATUS rfp_open_database(
    DMF_RFP             *rfp,
    DMF_JSX             *jsx,
    DMP_DCB             *dcb,
    i4		flags);

DB_STATUS rfp_close_database(
    DMF_RFP             *rfp,
    DMP_DCB             *dcb,
    i4	 	flags);

static DB_STATUS rfp_recov_chkpt_tables(
    DMF_RFP             *rfp,
    DM_PATH		*dmp_physical,
    i4			dmp_phys_length);

static DB_STATUS rfp_complete(
    DMF_RFP             *rfp,
    DMF_JSX             *jsx,
    DMP_DCB             *dcb);

static VOID rfp_xn_diag(
    DMF_JSX             *jsx,
    DMF_RFP             *rfp,
    DMVE_CB             *dmve);

static i4 dmf_diag_put_line(
    PTR			arg,
    i4			length,
    char		*buffer);

static STATUS ex_handler(
    EX_ARGS	*ex_args);

static VOID	remove_qualtx(
    DMF_RFP		*rfp,
    RFP_BQTX		*tx);

static DB_STATUS rfp_check_lsn_waiter(
    DMF_RFP         *rfp,
    DM0L_HEADER     *log_rec);

static DB_STATUS rfp_add_lsn_waiter(
    DMF_RFP         *rfp,
    RFP_OCTX        *octx);
    
static DB_STATUS rfp_osrc_close(
    DMF_RFP         *rfp,
    RFP_OCTX        *octx);

static DB_STATUS rfp_prompt(
    DMF_RFP             *rfp,
    DMF_JSX             *jsx,
    i4			*rfp_action);

static DB_STATUS rfp_er_handler(
    DMF_RFP             *rfp,
    DMF_JSX             *jsx);

static DB_STATUS rfp_jnlswitch(
    DMF_JSX		*jsx,
    DMP_DCB		*dcb);

static DB_STATUS dmf_jnlswitch(
    DMF_JSX		*jsx,
    DMP_DCB		*dcb);

#define RFP_ROLLBACK			1
#define RFP_CONTINUE_IGNORE_TBL		2
#define RFP_CONTINUE_IGNORE_ERR		3


/*{
** Name: dmfrfp	- Roll Forward Processor.
**
** Description:
**      This routine manages the recovery of a database.
**
**	The following section describes some of the highlights of the recovery
**	process:
**
**	In its simplest incarnation, recovery of a database operates in 2 steps:
**	    1) It restores the database to a known, consistent state
**	    2) It re-applies all changes which were made to the database since
**		the time that the database was saved.
**
**	Restoring the database is done by restoring a checkpoint. If the
**	checkpoint was an 'online' checkpoint, then changes which were in
**	progress during the checkpoint must be undone. This is done by reading
**	Before Image pages from the dump file and by restoring a known copy
**	of the config file (aaaaaaaa.cnf) from the dump file area (the config
**	file is not Before Image logged).
**
**	Re-applying changes is done by reading journal files and re-doing the
**	changes logged in the journal files.
**
**	Various flags may be passed to the rollforward process telling it
**	which checkpoint and which journals to use.
**
** Inputs:
[@PARAM_DESCR@]...
**
** Outputs:
[@PARAM_DESCR@]...
**	Returns:
**	    {@return_description@}
**	Exceptions:
**	    [@description_or_none@]
**
** Side Effects:
**	    [@description_or_none@]
**
** History:
**	02-feb-1989 (EdHsu)
**	    Support online backup.
**	10-may-90 (bryanp)
**	    Moved the rename of .cnf ==> .rfc back to prepare() and removed
**	    the 'rfc_exists' variable from this routine.
**      14-may-1991 (bryanp)
**          B34150 -- when we encounter an error, report it to the user, in
**          addition to logging it in the error log.
**	15-mar-1993 (rogerk)
**	    Reduced Logging - Phase IV:
**	    Added support for rollback phase at the end of rollforward
**		processing.  Added rollback_open_xacts routine and its
**		sub-functions.
**	21-jun-1993 (rogerk)
**	    Moved rollforward processing of the config file jnl, ckp and dump
**	    history to be done immediately after restoring the checkpoint
**	    rather than at the end of rollforward processing.  This is required
**	    since the archiver will need up-to-date config file information
**	    if it should archive records written by rollforward recovery.
**	    Created new routine post_ckp_config to update the config file and
**	    moved to there most of the cnf processing from the complete routine.
**	    Created new routine validate_nockp that checks for the legality
**	    of bypassing config restore and cleaned up some of its checks.
**	    Rewrote some of the main rfp loop and added comments.
**	22-nov-1993 (jnash)
**	    Remove ule_format() call following dmf_write_msg() call
**	    that caused error messages to be written to errlog.log twice.
**	18-feb-94 (stephenb)
**	    Move call to dma_write_audit() to a place where we are sure the
**	    rollforward has completed and remove failure case, we log this
**	    elsewhere.
**	9-may-94 (robf)
**          integration: removed duplicate audit request.
**	20-jun-1994 (jnash)
**	    fsync project: Open recovered databases with sync-at-end
**	    (DM2D_SYNC_CLOSE) flag.
**	14-sep-1994 (andyw)
**	    Partial Backup/Recovery Project:
**	 	Large changes to the RFP
**		    - Move last_tbl_id into the rfp
**		    - Move the db_status into the rfp
**		    - Move open_db into the rfp
**      24-jan-1995 (stial01)
**          Bug 66473: ckp_mode is now bit pattern.
**	10-jul-96 (nick)
**	    Ensure db open status is tracked correctly.
**	6-Apr-2004 (schka24)
**	    Obey -norollback even if error applying a journal record.
**	10-may-2004 (thaju02)
**	    Modified rfp_add_lsn_waiter & rfp_check_lsn_waiter.
**	    Removed reference to dm2u_rfp_load_table. Now call
**	    dm2u_load_table to restart load.
**	18-aug-2005 (thaju02) (B111452/INGSRV2636)
**	    Change for bug 107975 would mark database inconsistent and
**	    rollback database to a consistent state, if a rollforwarddb
**	    redo failure was encountered. In error handling, close of
**	    database was not forcing out modified pages to disk.
[@history_template@]...
*/
DB_STATUS
dmfrfp(
DMF_JSX		    *jsx,
DMP_DCB		    *dcb)
{
    DMVE_CB		dmve;
    DMF_RFP		rfp;
    char		line_buffer[132];
    char		ans_buffer[132];
    i4			amount_read;
    bool		ckp_processing;
    bool		dmp_processing;
    bool		jnl_processing;
    DB_STATUS		status;
    DB_STATUS		local_status;
    i4		local_error;
    i4		error;
    CL_ERR_DESC         sys_err;
    bool		db_valid = TRUE;
    bool		redo_error = FALSE;
    STATUS		tr_status;
    RFP_TX		*tx;
    DB_ERROR		local_dberr;
    i4			*err_code = &jsx->jsx_dberr.err_code;

    CLRDBERR(&jsx->jsx_dberr);

    for (;;)
    {
	if (jsx->jsx_status & JSX_TRACE)
	    TRset_file(TR_F_OPEN, "rfpdb.dbg", 9, &sys_err);

	if (jsx->jsx_status2 & JSX2_ON_ERR_PROMPT)
	{
	    /*
	    ** Set interactive input file.
	    */
	    if (tr_status = TRset_file(TR_I_OPEN, TR_INPUT, TR_L_INPUT, &sys_err))
	    {
		uleFormat(NULL, tr_status, &sys_err, ULE_LOG , NULL,
			(char * )NULL, 0L, (i4 *)NULL, &error, 0);
		status = E_DB_ERROR;
		break;
	    }

	    MEfill(sizeof(ans_buffer), '\0', ans_buffer);
	    TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
    "\nYou have specified -on_error_prompt, Do you want to continue Y/N: \n");

	    if (tr_status = TRrequest(ans_buffer, sizeof(ans_buffer), 
				&amount_read, &sys_err, NULL))
	    {
		status = E_DB_ERROR;
		break;
	    }

	    if (ans_buffer[0] != 'y' && ans_buffer[0] != 'Y')
	    {
		status = E_DB_ERROR;
		break;
	    }
	}

        /*
        ** Init fields used by all forms of rollforward in the rfp
        */
        rfp_init( &rfp, jsx);

        /*
        ** Validate the command line options that have been passed in
        */
        status = rfp_validate_cl_options( jsx, &rfp, dcb );
        if ( status != E_DB_OK )
            break;


	/*
	** Prepare the database for Rollforward.
	**
	** Find the config file and read/save the journal and checkpoint
	** information.  Determine what checkpoint saveset to use and
	** whether journal and dump processing is required.
	**
	** The journal/checkpoint history is saved so the config file can be
	** restored following the overlaying of the copy from the backup.
	*/
	status = prepare(&rfp, jsx, dcb);
	if (status != E_DB_OK)
	    break;

	/*
	** Using jsx_status (rollforward options) and database status (from
	** the config file) determine necessary rollforward actions:
	**
	**	ckp_processing - If checkpoint restore is needed.
	**	dmp_processing - If dump processing is needed.
	**	jnl_processing - If journal records need to be applied.
	*/

	/*
	** Checkpoint restore needed unless user specified -c flag.
	*/
	ckp_processing = ((jsx->jsx_status & JSX_USECKP) != 0);

	/*
	** Dump processing required if restoring database from an Online
	** Backup checkpoint and there are dump records to apply.
	*/
	dmp_processing =
		((ckp_processing) &&
		 (rfp.ckp_mode & CKP_ONLINE) &&
		 (rfp.dmp_first > 0) && (rfp.dmp_last >= rfp.dmp_first));

	/*
	** Journal processing required if the database is journaled unless
	** the user has specified the -j flag.
	*/
	jnl_processing = ( ((jsx->jsx_status & JSX_USEJNL ) !=0) &&
			   ( ((rfp.db_status & DSC_JOURNAL) != 0) ||
			 ((rfp.db_status & DSC_DISABLE_JOURNAL)!=0)));


	if (((jsx->jsx_status & JSX_USEJNL) == 0 &&
		(rfp.db_status & DSC_JOURNAL) != 0)
		|| (jsx->jsx_status & JSX_BTIME)
		|| (jsx->jsx_status & JSX_ETIME) 
		|| (jsx->jsx_status1 & JSX_START_LSN)
		|| (jsx->jsx_status1 & JSX_END_LSN))
	{
	    char	    error_buffer[ER_MAX_LEN];
	    i4	    error_length;
	    i4	    count;

	    /*
	    ** Issue a WARNING for the following rollforwarddb options
	    **   -j (when the database is journalled) 
	    **   -b, -e, -start_lsn, -end_lsn
	    ** Usage of these options should be recorded in the errlog.log
	    **
	    ** This will leave the database  in a state which does not 
	    ** correspond to the existing state of the journals.
	    ** The only supported course of action following this request
	    ** is another ROLLFORWARDDB or a CKPDB.
	    **
	    */
	    STprintf(line_buffer, "%s%s%s%s%s",
		   (jsx->jsx_status & JSX_BTIME) ? "-b " : "",
		   (jsx->jsx_status & JSX_ETIME) ? "-e " : "",
		   (jsx->jsx_status1 & JSX_START_LSN) ? "-start_lsn " : "",
		   (jsx->jsx_status1 & JSX_END_LSN) ? "-end_lsn " : "",
		   (((jsx->jsx_status & JSX_USEJNL) == 0 &&
			(rfp.db_status & DSC_JOURNAL) != 0)) ? "-j" : "");
	    uleFormat(NULL, E_DM1372_RFP_JNL_WARNING, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		    error_buffer, sizeof(error_buffer), &error_length,
		    &error, 2, STlength(line_buffer), line_buffer,
		    sizeof(dcb->dcb_name), dcb->dcb_name.db_db_name); 

	    error_buffer[error_length] = EOS;
	    SIwrite(1, "\n", &count, stdout);
	    SIwrite(error_length, error_buffer, &count, stdout);
	    SIwrite(1, "\n", &count, stdout);
	    SIwrite(1, "\n", &count, stdout);
	    SIflush(stdout);


	}

	/*
	** Verify ckp_processing flag.  If the user has specified to
	** to not apply the checkpoint (-c flag) then we require that
	** the database have been just previously restored to a valid
	** checkpoint.  Probably using -j or -e flags to halt before
	** completing journal processing.  The expectation is that this
	** rollforward operation will complete the database's recovery.
	*/
	if ( ! ckp_processing)
	{
	    status = validate_nockp(jsx, dcb);
	    if (status != E_DB_OK)
		break;

	    if (jsx->jsx_status & JSX_TRACE)
		TRdisplay("%@ RFP: Checkpoint restore step bypassed.\n");
	}


	/*
	** Overwrite the database with the checkpoint saveset.
	** The checkpointed locations are emptied and the backups restored.
	*/
	if (ckp_processing)
	{
	    if (jsx->jsx_status & JSX_TRACE)
		TRdisplay("%@ RFP: Restore ckp saveset\n");

	    status = rfp_ckp_processing(&rfp, jsx, dcb);
	    if (status != E_DB_OK)
		break;

            /*
            ** Patch the database config file to restore the checkpoint, dump,
            ** and journal history.  This is required since the config file
            ** restored by the checkpoint is the OLD one - which has old journal
            ** information that was valid back when the checkpoint was taken.
            **
            ** The new config file information is critical before we get to the
            ** journal processing below to ensure that the archiver does not
            ** write over any existing journal information or try to re-archive
            ** stuff which has already been journaled.
            **
            ** This restores the config file information which tracks
            ** interesting tidbits like:
            **
            **     The database JOURNAL state;
            **     The database last table id;
            **     The last journal file written to;
            **     The current journal file EOF;
            **     The Log Address to which the database was last archived;
            **     The next dump file to use;
            **     The last secid used;
            */
	    if ( jsx->jsx_status1 & JSX1_RECOVER_DB )
    	    {
                status = db_post_ckp_config(&rfp, jsx, dcb);
                if (status != E_DB_OK)
                    break;
	    }
        }


	/*
	** If we relocated any tables, update iirelation.relloc and
	** iidevices.devloc to reflect location changes
	*/
	if ((jsx->jsx_status1 & JSX_RELOCATE) &&
		(jsx->jsx_status1 & JSX_NLOC_LIST))
	{
	    status = reloc_tbl_process(&rfp, jsx, dcb, RELOC_FINISH_TABLES);
	    if (status != E_DB_OK)
		break;
	}

	/*
	** Open the restored database if journal or dump processing will
	** be required (don't bother if nothing to do).
	*/
	if ((dmp_processing || jnl_processing) &&
	  ((rfp.open_db & DB_OPENED) == 0))
	{
	    status = dm2d_open_db(dcb, DM2D_A_WRITE, DM2D_X,
		dmf_svcb->svcb_lock_list,
		(DM2D_NLK | DM2D_NLG | DM2D_RFP | DM2D_SYNC_CLOSE), &jsx->jsx_dberr);

	    if (status != E_DB_OK)
	    {
		uleFormat(&jsx->jsx_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
		SETDBERR(&jsx->jsx_dberr, 0, E_DM1300_RFP_OPEN_DB);
		break;
	    }

	    rfp.open_db |= DB_OPENED;
	}

	rfp.test_redo_err = 0;
	rfp.test_undo_err = 0;
	rfp.test_dump_err = 0;
	rfp.test_excp_err = 0;
	rfp.test_indx_err = 0;
	if (MEcmp(dcb->dcb_name.db_db_name, "test_rfp_dmve_error ", 20) == 0)
	{
	    char	*env;

	    NMgtAt( ERx("II_RFP_TEST_ERROR_HANDLING"), &env);
	    if (env)
	    {
		if (STcompare(env, "redo") == 0)
		    rfp.test_redo_err = 1;
		else if (STcompare(env, "undo") == 0)	
		    rfp.test_undo_err = 1;
		else if (STcompare(env, "dump") == 0)
		    rfp.test_dump_err = 1;
		else if (STcompare(env, "exception") == 0)
		    rfp.test_excp_err = 1;
		else if (STcompare(env, "index") == 0)
		    rfp.test_indx_err = 1;
	    }
	}

	/*
	** Apply dumps if restoring an online backup.
	*/
	if (dmp_processing)
	{
	    /*
	    ** Initialize recovery context for dump processing.
	    ** Dump processing is an UNDO recovery phase where Before Image
	    ** records are applied to rollback the database to its state
	    ** at the start of the checkpoint.
	    */
	    if (jsx->jsx_status & JSX_TRACE)
		TRdisplay("%@ dump processing: init recovery phase\n");

	    init_recovery(&rfp, dcb, &dmve, DMVE_UNDO);

	    /*
	    ** Process the dump files, restoring the database to a consistent
	    ** state.  This state reflects the database as it was when the
	    ** online checkpoint began and the SBACKUP record was written.
	    */
	    if (jsx->jsx_status & JSX_TRACE)
		TRdisplay("%@ dump processing: read dump phase\n");

	    status = read_dumps(jsx, &rfp, &dmve);

	    if (status != E_DB_OK)
	    {
		db_valid = FALSE;
		break;
	    }

	    if (jsx->jsx_status & JSX_TRACE)
		TRdisplay("%@ completed dump processing\n");
	}

	/*
	** Now rollforward any applicable journal updates.
	**
	** If the rollforward operation brings us to an inconsistent state
	** (with incomplete transactions open) then rollback those incomplete
	** transactions to bring the database to a consistent state.
	*/
	if (jnl_processing)
	{
	    /*
	    ** Initialize the recovery context now for forward processing.
	    */
	    if (jsx->jsx_status & JSX_TRACE)
		TRdisplay("%@ journal processing: init recovery phase\n");

	    init_recovery(&rfp, dcb, &dmve, DMVE_DO);

	    /*
	    ** Apply journaled updates to the database.
	    ** This continues until all journal files have been processed
	    ** or until some ending rollforward condition is met (such as
	    ** the -e condition).
	    */
	    if (jsx->jsx_status & JSX_TRACE)
		TRdisplay("%@ journal processing: read journal phase\n");

	    status = read_journals(jsx, &rfp, &dmve);
	    if (status != E_DB_OK)
	    {
		db_valid = FALSE;
		redo_error = TRUE;
		break;
	    }

	    if (jsx->jsx_tx_total > 0)
	    {
		if (jsx->jsx_tx_total == 1)
		{
		    TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
		        "%@ Completed processing 1 transaction.\n");
		}
		else
		{
		    TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
		        "%@ Completed processing of %d transactions.\n",
		        jsx->jsx_tx_total);
		}
	    }

	    if (jsx->jsx_status & JSX_TRACE)
		TRdisplay("%@ completed journal processing\n");

	    if ((jsx->jsx_status2 & JSX2_INCR_JNL)
		&& (jsx->jsx_status & JSX_USEJNL)
		&& (jsx->jsx_status2 & JSX2_RFP_ROLLBACK))
	    {
		/* Sync Last LSN in log header after incr rfp */
		status = rfp_sync_last_lsn(&rfp, jsx, dcb);
	    }

	    /* Display all open transactions */
	    if (rfp.rfp_tx)
		TRformat(dmf_diag_put_line, 0, line_buffer, sizeof(line_buffer),
		"Journal processing left %d transactions open\n", rfp.rfp_tx_count);

	    for (tx = rfp.rfp_tx; tx != NULL; tx = tx->tx_next)
	    {
		TRformat(dmf_diag_put_line, 0, line_buffer, sizeof(line_buffer),
			"Open transaction 0x%x%x.\n",
			tx->tx_bt.bt_header.tran_id.db_high_tran,
			tx->tx_bt.bt_header.tran_id.db_low_tran);
	    }

	    /*
	    ** If the database is left in an inconsistent state following
	    ** the application of journaled updates (there are open xacts
	    ** left) then restore the database to a consistent state by
	    ** rolling back all those incomplete transactions.
	    **
	    ** This phase is skipped if the #norollback flag is specified.
	    */
	    if ((rfp.rfp_tx_count) &&
		((rfp.rfp_status & RFP_NOROLLBACK) == 0))
	    {
		i4		save_tx_count = rfp.rfp_tx_count;

		TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
		    "%@ RFP: Restoring database to a consistent state.\n");

		init_recovery(&rfp, dcb, &dmve, DMVE_UNDO);

		status = rollback_open_xacts(jsx, &rfp, dcb, &dmve);
		if (status != E_DB_OK)
		{
		    db_valid = FALSE;
		    break;
		}

		TRformat(dmf_diag_put_line, 0, line_buffer, sizeof(line_buffer),
		    "Completed rollback of %d transactions.\n",
		    save_tx_count);
	    }

	    if ((rfp.rfp_tx_count) && (rfp.rfp_status & RFP_NOROLLBACK))
	    {
		TRformat(dmf_diag_put_line, 0, line_buffer, sizeof(line_buffer),
		    "Rollback of %d transactions bypassed.\n",
		    rfp.rfp_tx_count);
	    }

	    if ((jsx->jsx_status2 & JSX2_INCR_JNL)
		&& (jsx->jsx_status & JSX_USEJNL)
		&& (jsx->jsx_status2 & JSX2_RFP_ROLLBACK))
	    {
		/* Not required, but probably a good idea to jnlswitch now */
		rfp.rfp_status |= RFP_DID_JOURNALING;
		status = rfp_jnlswitch(jsx, dcb);
	    }
	}

	/*
	** If database is opened for dmp/jnl processing then
	** this should be closed before calling rfp_complete()
	*/
        if (rfp.open_db & DB_OPENED)
        {
            status = dm2d_close_db(dcb, dmf_svcb->svcb_lock_list,
                (DM2D_NLK | DM2D_NLG | DM2D_RFP | DM2D_SYNC_CLOSE), &jsx->jsx_dberr);

            if (status != E_DB_OK)
            {
                uleFormat(&jsx->jsx_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
                    (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
		SETDBERR(&jsx->jsx_dberr, 0, E_DM1300_RFP_OPEN_DB);
                break;
            }
            rfp.open_db &= ~DB_OPENED;
        }


	/*
	** Complete rollforward processing.
	**
	** This patches the database config file to indicate that the
	** rollforward was completed successfully.  In the absence of -j
	** or -e flags, rollforward should have brought the database to
	** its fully consistent state as described by the journals.
	**
	** If the rollforward was not complete (-j or -e used) then the
	** config file should be marked to show that the database is still
	** not fully restored and needs further rollforward processing.
	** This is done by asserting the ROLL_FORWARD status of the config
	** file indicating that rollforward progress is not complete.
	**
	** This completion processing also deletes the config.rfc file which
	** was created at the start of the rollforward.
	*/

	/* Need to reset the consistent bit in iirelation */
	/* code uncommented shero03 */
	status = rfp_complete(&rfp, jsx, dcb);
	if (status != E_DB_OK)
	    break;

	/*
	** Audit that we have successfully rolled forward the database.
	*/

	if ( dmf_svcb->svcb_status & SVCB_C2SECURE )
	    local_status = dma_write_audit( SXF_E_DATABASE,
		SXF_A_UPDATE | SXF_A_SUCCESS,
		dcb->dcb_name.db_db_name,	/* Object name (database) */
		sizeof(dcb->dcb_name.db_db_name),
		&dcb->dcb_db_owner,		/* Object owner */
		I_SX270C_ROLLDB,		/*  Message */
		TRUE,			/* Force */
		&local_dberr, NULL);

	/*
	** Close the database if it was opened for dmp or jnl processing.
	*/
	if (rfp.open_db & (DB_OPENED | DB_LOCKED))
	{
	    status = dm2d_close_db(dcb, dmf_svcb->svcb_lock_list,
				   (DM2D_NLG | DM2D_NLK), &jsx->jsx_dberr);
	    if (status != E_DB_OK)
	    {
		uleFormat(&jsx->jsx_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
		SETDBERR(&jsx->jsx_dberr, 0, E_DM1310_RFP_CLOSE_DB);
		break;
	    }
	    rfp.open_db &= ~(DB_OPENED | DB_LOCKED);
	}

	if (jsx->jsx_status & JSX_TRACE)
	    TRset_file(TR_F_CLOSE, 0, 0, &sys_err);

	return (E_DB_OK);
    }

    if ( !db_valid && (jsx->jsx_status1 & JSX1_RECOVER_DB) )
    {
	DM0C_CNF	*cnf;

	/* Update the configuration file. - mark database inconsistent */
	status = config_handler(jsx, OPEN_CONFIG, dcb, &cnf);
	if (status == E_DB_OK)
	{
            cnf->cnf_dsc->dsc_status &= ~DSC_VALID;
            cnf->cnf_dsc->dsc_inconst_time = TMsecs();
            cnf->cnf_dsc->dsc_inconst_code = DSC_RFP_FAIL;

	    status = config_handler(jsx, UPDTCOPY_CONFIG, dcb, &cnf);
	    if (status == E_DB_OK)
		status = config_handler(jsx, CLOSEKEEP_CONFIG, dcb, &cnf);
	}

	if (status != E_DB_OK)
	{
	    dmfWriteMsg(&jsx->jsx_dberr, 0, 0);
	    /* continue and try to rollback open transactions and close db */
	}
	else
	{
	    dmfWriteMsg(NULL, E_DM0100_DB_INCONSISTENT, 0);
	    TRformat(dmf_diag_put_line, 0, line_buffer, sizeof(line_buffer),
	    "Database %~t has been marked inconsistent.\n",
		sizeof(DB_DB_NAME), dcb->dcb_name.db_db_name); 
	}

	if (redo_error && rfp.rfp_tx_count && rfp.rfp_redo_err)
	{
	    if (rfp.rfp_status & RFP_NOROLLBACK)
	    {
		TRformat(dmf_diag_put_line, 0, line_buffer, sizeof(line_buffer),
			"-norollback specified, open transactions have not been rolled back.\n");
	    }
	    else
	    {
		i4		save_tx_count = rfp.rfp_tx_count;

		/*
		** Proceed as if -end_lsn=error_lsn
		** Try to rollback open transactions
		*/
		jsx->jsx_status1 |= JSX_END_LSN;
		rfp.rfp_point_in_time = TRUE;
		STRUCT_ASSIGN_MACRO(rfp.rfp_redo_end_lsn, jsx->jsx_end_lsn);

		TRformat(dmf_diag_put_line, 0, line_buffer, sizeof(line_buffer),
		    "Begin rollback of open transactions with end lsn (%x,%x)\n",
		    jsx->jsx_end_lsn.lsn_high, jsx->jsx_end_lsn.lsn_low);

		init_recovery(&rfp, dcb, &dmve, DMVE_UNDO);

		status = rollback_open_xacts(jsx, &rfp, dcb, &dmve);

		if (status == E_DB_OK)
		{
		    TRformat(dmf_diag_put_line, 0, line_buffer, sizeof(line_buffer),
		    "Completed rollback of %d transactions.\n", save_tx_count);

		    TRformat(dmf_diag_put_line, 0, line_buffer, sizeof(line_buffer),
		    "Only transactions completed before LSN (%x,%x) were applied.\n",
		    jsx->jsx_end_lsn.lsn_high, jsx->jsx_end_lsn.lsn_low
		    );

		    TRformat(dmf_diag_put_line, 0, line_buffer, sizeof(line_buffer),
		    "Rollback was attempted for any open transactions.\n");

		    TRformat(dmf_diag_put_line, 0, line_buffer, sizeof(line_buffer),
		    "You must verify all tables affected by the journal record that failed to apply.");

		    if (jsx->jsx_status2 & JSX2_INVALID_TBL_LIST)
		    {
			/*
			** Display invalid tables
			** Even though we attempted rollback open transactions, 
			** tables that we ignored errors on are still invalid.
			*/
			TRformat(dmf_diag_put_line, 0, line_buffer, sizeof(line_buffer),
			"The following are inconsistent and must be dropped and recreated.\n");

			rfp_display_inv_tables( &rfp, jsx );
		    }

		    TRformat(dmf_diag_put_line, 0, line_buffer, sizeof(line_buffer),
		    "This rollforward leaves the database in a state which does not correspond to the existing state of the journals.\n");

		    TRformat(dmf_diag_put_line, 0, line_buffer, sizeof(line_buffer),
		    "The only supported course of action following this failure is another ROLLFORWARDDB or CKPDB.");

		    TRformat(dmf_diag_put_line, 0, line_buffer, sizeof(line_buffer),
		    "WARNING: It is strongly recommended that you contact Ingres Corporation Technical Support for further information.");
		}
		else
		{
		    dmfWriteMsg(&jsx->jsx_dberr, 0, 0);
		    TRformat(dmf_diag_put_line, 0, line_buffer, sizeof(line_buffer),
		    "Unable to complete rollback of open transactions.\n");
		}
	    } /* if rollback */
	} /* if redo error */
    }

    /*
    ** Error handling:
    **    Close database if it was left open.
    **    Log appropriate error messages.
    */
    if (rfp.open_db & (DB_OPENED | DB_LOCKED))
    {
	int	flags = DM2D_NLG;
	/* 
	** If database is opened and NOT locked, then just close database.
	** If database is locked and NOT opened, then just release locks.
	** Otherwise close and unlock database.
	*/
	if ((rfp.open_db & DB_OPENED) && !(rfp.open_db & DB_LOCKED))
	    flags |= DM2D_NLK;
	else if (!(rfp.open_db & DB_OPENED) && (rfp.open_db & DB_LOCKED))
	    flags |= DM2D_JLK;
	status = dm2d_close_db(dcb, dmf_svcb->svcb_lock_list,
	    flags, &local_dberr);
	if (status != E_DB_OK)
	{
	    uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &local_error, 0);
	}
	rfp.open_db &= ~(DB_OPENED | DB_LOCKED);
    }

    /*
    ** Unless the error is an expected external error, log the error
    ** and return a generic Rollforward error to the caller.
    **
    ** Expected errors (those in the list below) are returned above and
    ** just reported to the user. (There is currently only 1 expected error.)
    */
    if ((jsx->jsx_dberr.err_code != E_DM1315_RFP_DB_BUSY))
    {
	/* only write the msg if it hasn't been written */
	if (jsx->jsx_dberr.err_code != 0)
	    dmfWriteMsg(&jsx->jsx_dberr, 0, 0);
	SETDBERR(&jsx->jsx_dberr, 0, E_DM1301_RFP_ROLL_FORWARD);
    }

    return (E_DB_ERROR);
}

/*{
** Name: rfp_db_complete	- Complete roll forward processing.
**
** Description:
**	Do completion and cleanup processing at the end of the rollforward.
**
**	The database is marked completely restored by updating the database
**	status in the config file and the RFC config file is removed.
**
**	Process cleanup is done to deallocate rollforward control blocks.
**
** Inputs:
**	rfp				Rollforward context.
**	jsx				Pointer to Journal Support info.
**      dcb				Pointer to DCB for this database.
**	db_status			Original database status info.
**
** Outputs:
**      err_code                        Reason for error return status.
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
**      21-nov-1986 (Derek)
**          Created for Jupiter.
**	02-feb-1989 (EdHsu)
**	    Support online backup.
**	10-jan-1990 (rogerk)
**	    Save the jnl_fil_seq and dmp_fil_seq from their initial values
**	    in the config file - rather than using jnl_last and dmp_last -
**	    which may be zero if the current checkpoint required no journal
**	    or dump processing.
**	    Restore entire journal and dump history rather than just last
**	    section - in case we support going back to previous checkpoints -
**	    we don't want to lose history entries.  Also, with online backup
**	    the saved version of the config file may not have the new history
**	    entries.
**	7-may-90 (bryanp)
**	    Bugfix: dump history was not being restored. Fixed.
**	    After successful end of roll-forward update backup copy of config.
**	15-mar-1993 (rogerk)
**	    Reduced Logging - Phase IV:
**	    Moved the trace message which gave the count of transactions
**		rolled forward out of this routine so that it could be logged
**		before the undo phase.  Added final completion status message.
**	21-jun-1993 (rogerk)
**	     Moved most of the config file updates to the new routine:
**	     post_ckp_config.  This is now done just after restoring the
**	     database from the checkpoint rather than after applying journals.
**	     The only config file updates done here now are to restore the
**	     config file database status to show the successful completion.
**	20-sep-1993 (andys)
**	     Fix arguments to ule_format call.
**	14-sep-1994 (andyw)
**	    Partial Backup/Recovery Project:
**	  	Move db_status into the rfp
**		Rename to rfp_db_complete.
**      20-july-1995 (thaju02)
**          bug #69981 - when ignore argument specified, output error
**          message to error log, and mark database inconsistent
**          for db level rolldb or table physically inconsistent
**          for tbl level rolldb, since rolldb will continue with
**          application of journal records.
**	23-feb-2004 (thaju02)
**	    Cleanup -b qualifying transaction contexts. (B111470/INGSRV2635)
[@history_template@]...
*/
static DB_STATUS
rfp_db_complete(
DMF_RFP		    *rfp,
DMF_JSX		    *jsx,
DMP_DCB		    *dcb)
{
    DM0C_CNF		*cnf;
    DB_STATUS		status;
    DI_IO		di_file;
    CL_ERR_DESC		sys_err;
    char		line_buffer[132];
    bool		incons = FALSE;
    i4			open_flag = OPEN_CONFIG;
    i4			error;

    CLRDBERR(&jsx->jsx_dberr);

    if (jsx->jsx_status2 & JSX2_INVALID_TBL_LIST)
    {
	/*
	** Mark all invalid tables inconsistent
	*/
	status = rfp_update_tbls_status( rfp, jsx, dcb,
					 RFP_UPDATE_INVALID_TBLS_STATUS);
	/*
	** Display all invalid tables 
	*/

	TRformat(dmf_diag_put_line, 0, line_buffer, sizeof(line_buffer),
	"The following are inconsistent and must be dropped and recreated.\n");

	rfp_display_inv_tables( rfp, jsx );
    }

    /*
    ** The config is not locked when opened by rollforwarddb (see dm0c_open)
    ** 
    ** Pass new open flag DM0C_RFP_NEEDLOCK to prevent concurrent
    ** update to config from RFP & ACP in an archive cyle
    ** (Alternatively we could call rfp_prepare_drain_journals to force
    ** archiver to finish before we open the config here)
    */
    if (rfp->rfp_status & RFP_DID_JOURNALING)
	open_flag = OPEN_LOCK_CONFIG;

    /*	Update the configuration file. */
    status = config_handler(jsx, open_flag, dcb, &cnf);

    if (status == E_DB_OK)
    {
	if (jsx->jsx_status & JSX_TRACE)
	    TRdisplay("%@ complete: update database config status.\n");

	/*
	** Turn off the CKP_INPROGRESS bit which indicates that the
	** checkpoint has not been correctly restored.
	*/
	cnf->cnf_dsc->dsc_status &= ~DSC_CKP_INPROGRESS;

	if (jsx->jsx_status2 & JSX2_INCR_JNL)
	{
	    /*	
	    ** Reset jnl_la (last log record copied to jnl)
	    ** or the archiver may report E_DM9838_ARCH_DB_BAD_LA (b120524)
	    */
	    cnf->cnf_jnl->jnl_la.la_sequence = 0;
	    cnf->cnf_jnl->jnl_la.la_block = 0;
	    cnf->cnf_jnl->jnl_la.la_offset = 0;

	    if (jsx->jsx_status & JSX_USECKP)
	    {
		/*
		** Change status of db to indicate we are in incremental
		** rollforward. This will make the database read only
		*/
		cnf->cnf_dsc->dsc_status |= DSC_INCREMENTAL_RFP;
	    }
	    else if ((jsx->jsx_status & JSX_USEJNL)
			&& (jsx->jsx_status2 & JSX2_RFP_ROLLBACK))
	    {
		TRformat(dmf_diag_put_line, 0, line_buffer, sizeof(line_buffer),
		"Completed Incremental rollforward processing.\n");

		/* reset DSC_INCREMENTAL_RFP after last incremental rfp */
		cnf->cnf_dsc->dsc_status &= ~DSC_INCREMENTAL_RFP;

		/* Turn off ROLL_FORWARD, db rolled forward completely */
		cnf->cnf_dsc->dsc_status &= ~DSC_ROLL_FORWARD;
	    }
	}
	else if (jsx->jsx_status & JSX_USEJNL)
	{
	    /*
	    ** If journal processing was done, turn off the ROLL_FORWARD status
	    ** to indicate that the database has been rolled forward completely.
	    */
	    cnf->cnf_dsc->dsc_status &= ~DSC_ROLL_FORWARD;
	}

        /*
        ** bug #69981, 20-july-1995 (thaju02) if -ignore specfied
        ** and errors were really ignored then mark
        ** database as inconsistent.
	**
	** Set dsc_inconst_code to the most serious error ignored
        */
        if (((jsx->jsx_status1 & JSX1_IGNORE_FLAG) && jsx->jsx_ignored_errors))
        {
            cnf->cnf_dsc->dsc_status &= ~DSC_VALID;
            cnf->cnf_dsc->dsc_inconst_time = TMsecs();
            cnf->cnf_dsc->dsc_inconst_code = DSC_RFP_FAIL;
	    incons = TRUE;
        }
	else if (jsx->jsx_status2 & JSX2_INVALID_TBL_LIST)
	{
            cnf->cnf_dsc->dsc_status &= ~DSC_VALID;
            cnf->cnf_dsc->dsc_inconst_time = TMsecs();
	    if (jsx->jsx_ignored_tbl_err)
		cnf->cnf_dsc->dsc_inconst_code = DSC_TABLE_INCONST;
	    else
		cnf->cnf_dsc->dsc_inconst_code = DSC_INDEX_INCONST;
	    incons = TRUE;
	}

	status = config_handler(jsx, UPDTCOPY_CONFIG, dcb, &cnf);
	if (status == E_DB_OK)
	    status = config_handler(jsx, CLOSEKEEP_CONFIG, dcb, &cnf);
	if (status != E_DB_OK)
	{
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM1313_RFP_CKP_HISTORY);
	    return (status);
	}

	if (incons)
	{
	    dmfWriteMsg(NULL, E_DM0100_DB_INCONSISTENT, 0);
	    TRformat(dmf_diag_put_line, 0, line_buffer, sizeof(line_buffer),
	    "Database %~t has been marked inconsistent.\n",
		sizeof(DB_DB_NAME), dcb->dcb_name.db_db_name);
	}
    }

    /*  Delete the RFC file. */

    status = DIdelete(&di_file, (char *)&dcb->dcb_root->physical,
	dcb->dcb_root->phys_length, "aaaaaaaa.rfc", 12, &sys_err);
    if ((status != OK) && ((jsx->jsx_status & JSX_USECKP) != 0))

    {
	uleFormat(NULL, E_DM9003_BAD_FILE_DELETE, &sys_err,
            ULE_LOG, NULL, NULL, 0, NULL, &error, 4,
	    sizeof(dcb->dcb_name), dcb->dcb_name.db_db_name,
	    4, "None",
	    dcb->dcb_root->phys_length, &dcb->dcb_root->physical,
	    12, "aaaaaaaa.rfc");
	SETDBERR(&jsx->jsx_dberr, 0, E_DM1309_RFP_RFC_DELETE);
	return (E_DB_ERROR);
    }

    /*
    ** Clean up any leftover transaction contexts.
    */
    while (rfp->rfp_tx != NULL)
	remove_tx(rfp, rfp->rfp_tx);

    while (rfp->rfp_bqtx != NULL)
	remove_qualtx(rfp, rfp->rfp_bqtx);

    TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
	"%@ Rollforward completed successfully.\n");

    /* clean up all online modify contexts */
    while (rfp->rfp_octx != NULL)
    {
	RFP_OCTX	*octx = rfp->rfp_octx;

	while (octx->octx_osrc_next)
	{
	    DM2U_OSRC_CB	*osrc = (DM2U_OSRC_CB *)octx->octx_osrc_next;
	    DMP_RNL_ONLINE	*rnl = &(osrc->osrc_rnl_online);
	    DMP_MISC		*osrc_misc_cb = osrc->osrc_misc_cb;

	    if (rnl->rnl_xlsn)
		dm0m_deallocate((DM_OBJECT **)&rnl->rnl_xlsn);
	    if (rnl->rnl_btree_xmap)
		dm0m_deallocate((DM_OBJECT **)&rnl->rnl_btree_xmap);

	    octx->octx_osrc_next = (PTR)osrc->osrc_next;
	
	    dm0m_deallocate((DM_OBJECT **)&osrc_misc_cb);
	}

	if (octx->octx_next != NULL)
	    octx->octx_next->octx_prev = octx->octx_prev;

	if (octx->octx_prev != NULL)
	    octx->octx_prev->octx_next = octx->octx_next;
	else
	    rfp->rfp_octx = octx->octx_next;

	rfp->rfp_octx_count--;
	dm0m_deallocate((DM_OBJECT **)&octx);
    }

    /* cleanup any leftover lsn waiters */
    while (rfp->rfp_lsn_wait != NULL)
    {
	RFP_LSN_WAIT	*waiter = rfp->rfp_lsn_wait;

        if (waiter->wait_next != NULL)
            waiter->wait_next->wait_prev = waiter->wait_prev;

        if (waiter->wait_prev != NULL)
            waiter->wait_prev->wait_next = waiter->wait_next;
        else
            rfp->rfp_lsn_wait = waiter->wait_next;

        rfp->rfp_lsn_wait_cnt--;
        dm0m_deallocate((DM_OBJECT **)&waiter);
    }

    return (E_DB_OK);
}

/*{
** Name: rfp_tbl_complete - Complete table level rollforward processing.
**
** Description:
**      Do completion and cleanup processing at the end of the rollforward.
**
** Inputs:
**      rfp                             Rollforward context.
**      jsx                             Pointer to Journal Support info.
**      dcb                             Pointer to DCB for this database.
**
** Outputs:
**      err_code                        Reason for error return status.
**      Returns:
**          E_DB_OK
**          E_DB_ERROR
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**	14-sep-1994 (andyw)
**	    Partial Backup/Recovery Project:
**		Created.
*/
static DB_STATUS
rfp_tbl_complete(
DMF_RFP             *rfp,
DMF_JSX             *jsx,
DMP_DCB             *dcb)
{
    DB_STATUS           status;
    char		line_buffer[132];

    CLRDBERR(&jsx->jsx_dberr);

    if (jsx->jsx_status2 & JSX2_INVALID_TBL_LIST)
    {
	/*
	** Display all invalid tables 
	*/

	TRformat(dmf_diag_put_line, 0, line_buffer, sizeof(line_buffer),
	"The following are inconsistent and must be dropped and recreated.\n");

	rfp_display_inv_tables( rfp, jsx );
    }

    /* Free memory allocated for relocate table location information */
    if (rfp->rfp_locmap)
    {
	dm0m_deallocate((DM_OBJECT **)&rfp->rfp_locmap);
    }

    /*
    ** Finished recovery on tables so mark all tables that recovery
    ** succceded on not physically inconsistent
    */
    status = rfp_update_tbls_status( rfp, jsx, dcb,
                                     RFP_COMPLETE_UPDATE_TBLS_STATUS);
    if ( status != E_DB_OK )
        return( status );

    return( E_DB_OK );
}

/*{
** Name: rfp_complete - Complete rollforward processing.
**
** Description:
**      Do completion and cleanup processing at the end of the rollforward.
**
**      The database is marked completely restored by updating the database
**      status in the config file and the RFC config file is removed.
**
**      Process cleanup is done to deallocate rollforward control blocks.
**
** Inputs:
**      rfp                             Rollforward context.
**      jsx                             Pointer to Journal Support info.
**      dcb                             Pointer to DCB for this database.
**
** Outputs:
**      err_code                        Reason for error return status.
**      Returns:
**          E_DB_OK
**          E_DB_ERROR
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      14-sep-1994 (andyw)
**	    Partial Backup/Recovery Project:
**		Created.
*/
static DB_STATUS
rfp_complete(
DMF_RFP             *rfp,
DMF_JSX             *jsx,
DMP_DCB             *dcb)
{
    DB_STATUS   status;

    CLRDBERR(&jsx->jsx_dberr);

    if ( jsx->jsx_status1 & JSX1_RECOVER_DB )
    {
        status = rfp_db_complete( rfp, jsx, dcb );
    }
    else if ( jsx->jsx_status & JSX_TBL_LIST )
    {
        /*
        ** FIXME, not sure what to do here i assume mark table
        ** consistent etc...
        */
        status = rfp_tbl_complete( rfp, jsx, dcb);
    }

    if ( status == E_DB_OK )
        rfp_dump_stats( rfp, jsx );

    return ( status );

}

/*{
** Name: prepare	- Prepare to rollforward a database.
**
** Description:
**      This routine opens that database reads the configuration files
**	and prepares for the rest of the processing.
**
**	Among the interesting things that this routine does is to rename
**	the database configuration file "aaaaaaaa.cnf" to "aaaaaaaa.rfc". This
**	is done to enable us to restart the roll-forward process in case
**	the restore doesn't work. We are GUARANTEED that "aaaaaaaa.rfc" does
**	NOT exist in the CK save-set, and so the restore process does not
**	touch it and a restarted roll-forward can find the .rfc file, figure
**	out that it has been restarted, and take appropriate action. Note that
**	it is VERY important that we rename this file as soon as we start
**	running roll-forward, because restore_ckp() will overwrite this file
**	when it runs. We want to preserve the contents of the file at the
**	start of the roll-forward process.
**
**	If things are badly enough screwed up, it may be the case that when
**	we go to rollforward this database, we cannot even successfully find
**	and open the config file "aaaaaaaa.cnf". For example, the user may
**	have accidentally deleted EVERY file in the database directory! (This
**	is not as absurd as it seems -- if the physical disk fails and a new
**	one must be formatted, the odds are that the new one will be formatted
**	clean, containing no config file or other file.)
**
**	In this case, we make an attempt to recover by looking in the II_DUMP
**	directory for a "aaaaaaaa.cnf" file. The config file is copied there
**	frequently and so the odds are good that we can find one there. If
**	one is there, we copy it over to the database directory and proceed
**	with the rollforward.
**
** Inputs:
**	jsx				Pointer to journal support context.
**	dcb				Pointer to DCB for database.
**
** Outputs:
**	rfp				Rollforward context.
**      db_status                       Database status from the config file.
**      last_tbl_id			Last table id from config file.
**      err_code                        Reason for error code.
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
**      17-nov-1986 (Derek)
**          Created for Jupiter.
**	30-jan-1989 (EdHsu)
**	    Added code to support online backup
**	10-jan-1990 (rogerk)
**	    Added jnl_count, dmp_count, rfp_jnl_history, and rfp_dmp_history
**	    so that entire journal and dump histories could be saved rather
**	    than just the values in the last entry.
**	    Added jnl_fil_seq and dmp_fil_seq to save the current file
**	    sequence numbers rather than just the value in the last history
**	    entry, which may be zero.
**	29-apr-1990 (walt)
**	    Moved the renaming of the config file (to .rfc) into restore_ckp().
**	    Made local variable rfc_exists into a parameter because
**	    restore_ckp() will need it during the rename.
**	4-may-90 (bryanp)
**	    Added support for JSX_CKP_SELECTED and JSX_LAST_VALID_CKP and
**	    enhanced the checkpoint sequence number selection to be more
**	    tolerant of bad checkpoints. If the checkpoint we're planning to
**	    roll-forward from is invalid, try the previous checkpoint, and so
**	    forth, until we either find a valid checkpoint or run out of
**	    checkpoints. This makes checkpoint failure a much less serious
**	    problem.
**	    Moved the rename of .cnf ==> .rfc back to prepare(), and removed
**	    the 'rfc_exists' parameter -- it's back to a local variable again.
**	    Taught rollforward how to recover the config file from the copy
**	    in the dump directory if the one in the database directory is
**	    unusable. Changed the arguments to dm0d_config_restore to allow
**	    rollforward to request the appropriate config file.
**	    Changed the meaning and usage of 'jnl_first' and 'jnl_last' in the
**	    RFP structure to support rolling forward multiple journal histories.
**	5-oct-1990 (bryanp)
**	    Bug #33422:
**	    Correctly handle jnl_history_no == -1, which means that no
**	    checkpoint is being used ("rollforwarddb -c +j).
**      25-feb-1991 (rogerk)
**          Add check for rollforward on journal_disabled database.  Give
**          a warning message.
**	15-mar-1993 (rogerk)
**	    Reduced Logging - Phase IV:
**	    Added support for rollback phase at the end of rollforward.
**	    Added new rfp fields - changed rfp_tx to be dynamically allocated.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**		Altered dm0j_merge calling sequence in order to allow auditdb
**		    to call dm0j_merge.
**	26-apr-1993 (jnash)
**	    Add support for no logging during rollfoward via -nologging.
**	    Add new warning messages logged when -nologging or -norollback
**	    requested.
**	21-jun-1993 (rogerk)
**	    Add jnl_la value (address to which the database has been journaled)
**	    and dmp_la value to the config information that is saved to be
**	    copied into the restored version of the config file.
**	    Changed the db_status value returned to include all of the
**	    config file status bits, rather than just journal and dump info.
**	20-sep-1993 (andys)
**	     Fix arguments to ule_format call.
**	20-sep-1993 (rogerk)
**	    Initialize rfp_dmve pointer.
**	3-oct-93  (robf)
**          Save last secid from config file
**	18-oct-1993 (jnash)
**	    One ule_lookup left garbage at the end of the line, so terminate it.
**	31-jan-1994 (jnash)
**	  - Add last_tbl_id param and maintenance.  The last table id
**	    is now restored from the original configuration file.
**	  - Replace JSX_JDISABLE with JSX_FORCE.
**	  - Fix truncated E_DM93A0_ROLLFORWARD_JNLDISABLE error message.
**	21-feb-1994 (jnash)
**	  - Add support for -start_lsn and -end_lsn parameters.  These
**	    are treated similar to -b and -e.
**	  - Allow rollforward #cxx -c recoveries.
**      23-may-1994 (andys)
**          Added call to dm2d_check_dir to prepare(). If we can't open
**	    the (.rfc) version of the config file or the (.cnf) version
**	    and we can't restore a copy from the dump directory, then
**	    check if the data directory really exists. If it doesn't then
** 	    return an appropriate error number so that rollforwarddb
**	    can print a more informative message.  [bug 60702]
**	14-sep-1994 (andyw)
**	    Partial Backup/Recovery Project:
**		Restructure for Table level rollforward and
**		Move last_tbl_id into the rfp structure and
**		Move the db_status as well
**	23-feb-2004 (thaju02)
**	    Initialize -b qualifying transaction hash queue, count, etc.
**	    (B111470/INGSRV2635)
[@history_template@]...
*/
static DB_STATUS
prepare(
DMF_RFP		    *rfp,
DMF_JSX		    *jsx,
DMP_DCB		    *dcb)
{
    i4		i;
    DB_STATUS		status;
    char		line_buffer[132];
    char	        local_buffer[ER_MAX_LEN + DB_DB_MAXNAME];
    i4			loc_error;
    i4			error;

    CLRDBERR(&jsx->jsx_dberr);

    /*
    ** Issue message indicating start of rollforwarddb
    */
    if ( jsx->jsx_status1 & JSX1_RECOVER_DB )
    {
        TRformat( dmf_put_line, 0, line_buffer, sizeof(line_buffer),
                  "%@ RFP: Preparing for database rollforward on : %t\n",
                  sizeof(dcb->dcb_name), dcb->dcb_name.db_db_name);
    }
    else if ( jsx->jsx_status & JSX_TBL_LIST )
    {
        TRformat( dmf_put_line, 0, line_buffer, sizeof(line_buffer),
                  "%@ RFP: Preparing for table level rollforward on : %t\n",
                  sizeof(dcb->dcb_name), dcb->dcb_name.db_db_name);

    }

    /*
    ** If recovery is not to the current time then record for
    ** further processing
    */
    if ( (jsx->jsx_status & JSX_ETIME) || (jsx->jsx_status1 & JSX_END_LSN))
        rfp->rfp_point_in_time = TRUE;
    else
        rfp->rfp_point_in_time = FALSE;
    /*
    ** Initialize RFP status fields.
    */
    rfp->rfp_status = 0;

    /*
    ** If the user has specified a -e time or -end_lsn, do not log
    ** undo actions when the database is rolled back to a consistent state
    ** at the end of the rollforward phase.  The CLR's should not be logged
    ** as the database is not being restored to a state described by the
    ** journal files.
    **
    ** Also check for user requested option to bypass CLR logging.
    */
    if ( rfp->rfp_point_in_time || (jsx->jsx_status1 & JSX_NORFP_LOGGING))
	 rfp->rfp_status |= RFP_NO_CLRLOGGING;

    if (jsx->jsx_status1 & JSX_NORFP_LOGGING)
    {
	/*
	** Log message indicating that recovery is not retryable.
	*/
	STncpy( local_buffer, dcb->dcb_name.db_db_name, DB_DB_MAXNAME);
	local_buffer[ DB_DB_MAXNAME ] = '\0';
	STtrmwhite(local_buffer);
	uleFormat(NULL, E_DM1346_RFP_NOLOGGING, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
	    (char *)NULL, (i4)0, (i4 *)NULL, &loc_error, 1,
	    0, local_buffer);
    }

    /*
    ** If the user has specified a -b time or -start_lsn, indicate that
    ** we are not yet processing journal records.  When the first log
    ** record is found with a timestamp greater than the -b time
    ** (or -start_lsn) then this status will be turned off and we will begin
    ** processing records.
    */
    if (jsx->jsx_status & JSX_BTIME)
	rfp->rfp_status |= RFP_BEFORE_BTIME;

    if (jsx->jsx_status1 & JSX_START_LSN)
	rfp->rfp_status |= RFP_BEFORE_BLSN;

    /*
    ** Check for user requested options that bypass the UNDO phase.
    */
    if (jsx->jsx_status & JSX_NORFP_UNDO)
    {
	rfp->rfp_status |= RFP_NOROLLBACK;

	/*
	** Log message indicating that this recovery may leave db inconsistent.
	*/
	STncpy( local_buffer, dcb->dcb_name.db_db_name, DB_DB_MAXNAME);
	local_buffer[ DB_DB_MAXNAME ] = '\0';
	STtrmwhite(local_buffer);
	uleFormat(NULL, E_DM1347_RFP_NOROLLBACK, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
	    (char *)NULL, (i4)0, (i4 *)NULL, &loc_error, 1,
	    0, local_buffer);
    }

    if ( jsx->jsx_status1 & JSX1_RECOVER_DB )
    {
        /*
        ** Database level rollforwarddb
        */
        status = rfp_prepare_db_recovery( rfp, jsx, dcb );
    }
    else if ( jsx->jsx_status & JSX_TBL_LIST )
    {
        /*
        ** Table level rollforwarddb
        */

        /*
        ** The following are not used as not overwritting the config
        ** file. The db_status is used to hold he state of the database
        ** accroding to the config file.
        */
        rfp->last_tbl_id = 0;
        rfp->db_status   = 0;

        status = rfp_prepare_tbl_recovery( rfp, jsx, dcb );

    }

    /*
    ** Initialize the rest of the RFP.
    */
    rfp->rfp_dmve = NULL;
    rfp->rfp_etime_lsn.lsn_high = -1;
    rfp->rfp_etime_lsn.lsn_low  = -1;

    rfp->rfp_tx_count = 0;
    rfp->rfp_tx = 0;
    for (i = 0; i < RFP_TXH_CNT; i++)
        rfp->rfp_txh[i] = 0;

    rfp->rfp_bqtx_count = 0;
    rfp->rfp_bqtx = NULL;
    for (i = 0; i < RFP_TXH_CNT; i++)
	rfp->rfp_bqtxh[i] = 0;

    rfp->rfp_octx_count = 0;
    rfp->rfp_octx = 0;

    rfp->rfp_lsn_wait_cnt = 0;
    rfp->rfp_lsn_wait = 0;

    jsx->jsx_tx_total = 0;
    jsx->jsx_tx_since = 0;


    return (status);
}

/*{
** Name: read_journals	- Check which operations is required
**
** Description:
**	This function decides which type of journal processing is required
**	based on the type of operation (database, table)
**
** Inputs:
**	rfp				Rollforward context.
**	dmve				Recovery control block.
**
** Outputs:
**      err_code                        Reason for error return status.
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
**      06-dec-1994 (andyw)
**	    Created.
[@history_template@]...
*/
static DB_STATUS
read_journals(
DMF_JSX		    *jsx,
DMF_RFP		    *rfp,
DMVE_CB		    *dmve)
{
    DB_STATUS status;
    i4			new_jnl_cnt;
    i4			total = 0;

    CLRDBERR(&jsx->jsx_dberr);

    /*
    ** Check type of journal processing required
    */
    if ( jsx->jsx_status2 & JSX2_INCR_JNL)
    {
	/* process new jnls */
	for ( new_jnl_cnt = 0; ; )
	{
	    status  = rfp_read_incr_journals_db(jsx, rfp, dmve, &new_jnl_cnt);
	    if (status != E_DB_OK || !new_jnl_cnt || new_jnl_cnt == total)
		break;
	    total++;
	}
	
	if (jsx->jsx_status & JSX_TRACE)
	    TRdisplay("Finished processing %d incremental journals (%d %d %d)\n",
		total, status, jsx->jsx_dberr.err_code, new_jnl_cnt);
    }
    else if ( jsx->jsx_status1 & JSX1_RECOVER_DB )
    {
	status  = rfp_read_journals_db(jsx, rfp, dmve);
    }
    else if ( jsx->jsx_status & JSX_TBL_LIST )
    {
	status = rfp_read_journals_tbl(jsx, rfp, dmve);
    }

    return (status);
}

/*{
** Name: read_journals_db  - Read and apply journal records.
**
** Description:
**      This routine reads all the journal records and call the
**      apply routine to make the actual changes.
**
**      There may be multiple journal histories all being rolled forward,
**      particular if this is a recovery from an old checkpoint. rfp->jnl_first
**      is the number of the first history, and rfp->jnl_last is the number
**      of the last history. For each history, we roll-forward the journal
**      files numbered ckp_f_jnl through ckp_l_jnl (which may be no files at
**      all if no journalling was done in this history -- both numbers will
**      be 0 in that case).
**
** Inputs:
**      rfp                             Rollforward context.
**      dmve                            Recovery control block.
**
** Outputs:
**      err_code                        Reason for error return status.
**      Returns:
**          E_DB_OK
**          E_DB_ERROR
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      21-nov-1986 (Derek)
**          Created for Jupiter.
**      10-may-90 (bryanp)
**          Changed the meaning and usage of 'jnl_first' and 'jnl_last' in the
**          RFP structure to support rolling forward multiple journal histories.
**          Now that we support rolling forward through multiple journal
**          histories, we want to be 'tolerant' of situations where there is
**          a 'gap' in the journal files. If a journal file is missing, we
**          issue a message about it, but we keep going and try to process the
**          next journal file (this is HIGHLY controversial, and may change).
**          Also, fixed a problem with #x auditing of journalled log records.
**      03-dec-1992 (jnash)
**          Reduced logging project.  Move log record address from journal
**          record into dmve->dmve_lg_addr.
**      10-feb-1993 (jnash)
**          Add FALSE param to dmd_log().
**      15-mar-1993 (rogerk)
**          Reduced Logging - Phase IV:
**          Moved -e/-b flag qualifications here from the process_record
**              routine.  We now bypass process_record altogether previous
**              to the -b time and stop all journal reading after the -e time.
**          Changed -e flag qualification to use the END transaction time
**              rather than the BEGIN transactions time  (this is done by
**              halting journal processing at the specified time and rolling
**              back all transactions not already committed).
**          Changed -b flag qualification to mean not to apply ANY records
**              logged previous to that time and to apply ALL records logged
**              after that time.  This is irrespective of what the transaction
**              begin timestamp is.
**          Added direction parameter to dm0j_open routine.
**      24-may-1993 (andys)
**          Pass appropriate logfile blocksize to dmd_log and dmd_format_log
**          rather than using hardwired 4096.
**      31-jan-1994 (jnash)
**          Remove exact time processing -- it doesn't work.  Defer
**          checking for -e time until after process_record() so that
**          ending ET is processed.
**      21-feb-1994 (jnash)
**        - Add -start_lsn, -end_lsn support.
**        - Enable exact time and -end_lsn "exact match" support.
**      14-sep-1994 (andyw)
**          Partial Backup/Recovery Project:
**              For table level rollforward add the
**              - Journal file phase to OpenCheckpoint
**              - Statistics
**              - Renamed to rfp_read_journals_db
**	30-jun-1995 (shero03)
**	    Ignore any journal errors if -ignore is on
**      20-july-1995 (thaju02)
**          bug #69981 - when ignore argument specified, output error
**          message to error log, and mark database inconsistent
**          for db level rolldb or table physically inconsistent
**          for tbl level rolldb, since rolldb will continue with
**          application of journal records.
**	23-june-1998(angusm)
**	    bug 82736: change interface to dmckp_init()
**      28-jul-1998 (nicph02) Cross-integration
**          Implemented 2 environment variables (logicals) having to do with
**          optionally giving more messages during the rollforward of
**          journals. (Sir 86486).
**      29-jul-1998 (nicph02)
**          bug 90541: Allow users to apply journals that are older than
**          the first valid journal. The new logic is that it is assumed that
**          there is no missing journals (no ckpdb -j done). If one is found
**          during processing, then rollforward is aborted (E_DM134C logged).
**      19-nov-1998 (nicph02)
**          rollforwarddb -e<time>: If there is no transaction to recover before
**          the next non journaled ckp, failure with E_DM134C. Problem is that
**          we need to read the next jnl record (after the missing jnl!) that
**          is after -e<time> thus stopping the rollforwarddb (bug 94219).
**      16-dec-98 (nicph02)
**          Missed some stuff when cross-integrating SIR 86486.
**	15-jan-1999 (nanpr01)
**	    Pass point to a pointer in dm0j_close routines.
**	23-feb-2004 (thaju02)
**	    Added call to rfp_prescan_btime_qual() to prescan journals
**	    for -b qualifying transactions. (B111470/INGSRV2635)
**	20-May-2005 (thaju02)
**	    For process phase, restore rfp_status. (B114487)
**	18-aug-2005 (thaju02) (B111471/INGSRV2637)
**	    If journal read failure encountered, mark database inconsistent
**	    and rollback open transactions.
**      26-Jul-2007 (wanfr01)
**          Bug 118854
**          Stop duplicate logging of jnl records when #x and -verbose are
**          used together
*/
static DB_STATUS
rfp_read_journals_db(
DMF_JSX             *jsx,
DMF_RFP             *rfp,
DMVE_CB             *dmve)
{

    DM0J_CTX		*jnl;
    DM0L_HEADER		*record;
    i4		i;
    i4		l_record;
    i4             record_interval;
    i4             record_time;
    i4             record_count;
    i4             byte_interval;
    i4             byte_time;
    i4             byte_count;
    DB_STATUS		status;
    char		line_buffer[132];
    bool		lsn_check = FALSE;
    DM_FILE             jnl_filename;
    DMCKP_CB            *d = &rfp->rfp_dmckp;
    char              *env;
    bool                no_jnl = FALSE;
    i4			jnl_read = 0;
    i4			jnl_done = 0;
    LG_LSN		last_rec_read_lsn;
    i4			phase;
    i4			error;
    DB_ERROR		local_dberr;
    i4			save_rfp_status;
#define	PRESCAN	0
#define	PROCESS	1

    CLRDBERR(&jsx->jsx_dberr);

    /*
    ** Initialise the DMCKP_CB structure, used during the OPENjournal stuff.
    */
    status = dmckp_init( d, DMCKP_RESTORE, &error );
    if ( status != E_DB_OK )
    {
	SETDBERR(&jsx->jsx_dberr, 0, error);
        return( status );
    }

    /*
    ** Setup the DMCKP stuff which will remain constant
    */
    d->dmckp_jnl_phase   = DMCKP_JNL_DO;
    d->dmckp_device_type = DMCKP_DISK_FILE;
    d->dmckp_jnl_path    = rfp->cur_jnl_loc.physical.name;
    d->dmckp_jnl_l_path  = rfp->cur_jnl_loc.phys_length;

    /*
    ** FIXME, test that this does not SEGV ever...
    */
    d->dmckp_first_jnl_file = rfp->rfp_jnl_history[rfp->jnl_first].ckp_f_jnl;
    d->dmckp_last_jnl_file  = rfp->rfp_jnl_history[rfp->jnl_last].ckp_l_jnl;


    /*
    ** Call the OpenJournal begin routine
    */
    status = dmckp_begin_journal_phase( d, &error );
    if ( status != E_DB_OK )
    {
	SETDBERR(&jsx->jsx_dberr, 0, error);
        return( status );
    }

    /*
    ** Check to see if the env. variables are set.  If not set, initialize
    ** to zero so they won't be used.
    */
    NMgtAt(ERx("II_RFP_JNL_RECORD_INTERVAL"), &env);
    if (env && *env)
        CVal(env, &record_interval);
    else
        record_interval = 0;
 
    NMgtAt(ERx("II_RFP_JNL_BYTE_INTERVAL"), &env);
    if (env && *env)
        CVal(env, &byte_interval);
    else
        byte_interval = 0;

    last_rec_read_lsn.lsn_high = last_rec_read_lsn.lsn_low = 0;

    if (jsx->jsx_status & JSX_BTIME)
    {
	dmve->dmve_flags |= DMVE_ROLLDB_BOPT;
	status = rfp_prescan_btime_qual(jsx, rfp, dmve);
    }

    /*
    ** For each journal history to be rolled forward, loop through the journal
    ** file sequence numbers used in that history and apply them.
    */

  for (phase = PRESCAN, save_rfp_status = rfp->rfp_status; 
	phase <= PROCESS; phase++, rfp->rfp_status = save_rfp_status)
  {
    for (rfp->jnl_history_number =  rfp->jnl_first;
	 rfp->jnl_history_number <= rfp->jnl_last;
	 rfp->jnl_history_number++)
    {
	for (i =  rfp->rfp_jnl_history[rfp->jnl_history_number].ckp_f_jnl;
	     i <= rfp->rfp_jnl_history[rfp->jnl_history_number].ckp_l_jnl;
	     i++)
	{
	    /*
	    ** A journal history with both numbers 0 indicates that no
	    ** journaling went on in this history, so we proceed to the
	    ** next history.
	    */
	    if (i == 0)
            {
               /* no_jnl is set to indicate that we skip over a non journaled
                  period. The next record to be read should be greater than
                  -e time ot there is a problem! */
               no_jnl = TRUE;
               break;
            }

            /*
            ** Determine next journal file name for the OpenJournal
            */
            dm0j_filename( DM0J_MERGE, i, -1, &jnl_filename, (DB_TAB_ID *)NULL );

            d->dmckp_jnl_file   = jnl_filename.name;
            d->dmckp_jnl_l_file = sizeof( jnl_filename );

            status = dmckp_work_journal_phase( d, &error );
            if ( status != E_DB_OK )
	    {
		SETDBERR(&jsx->jsx_dberr, 0, error);
                break;
	    }


	    /*  Open the next journal file. */

	    status = dm0j_open(DM0J_MERGE,
			(char *)&dmve->dmve_dcb_ptr->dcb_jnl->physical,
			dmve->dmve_dcb_ptr->dcb_jnl->phys_length,
			&dmve->dmve_dcb_ptr->dcb_name,
			rfp->jnl_block_size, i, 0, 0, DM0J_M_READ,
			-1, DM0J_FORWARD, (DB_TAB_ID *)0, &jnl, &jsx->jsx_dberr);
	    if (status != E_DB_OK)
	    {
		if (jsx->jsx_dberr.err_code == E_DM9034_BAD_JNL_NOT_FOUND)
		{
		    uleFormat(&jsx->jsx_dberr, 0, NULL, ULE_LOG, NULL, 
				NULL, 0, NULL, &error, 0);
		    TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
		    "%@ RFP: Journal file sequence %d not found -- skipped.\n",
		    i );

		    /* try next journal...we're trying to be fault tolerant.
		    ** This is a highly controversial decision, and may get
		    ** changed.
		    ** *err_code = E_DM1305_RFP_JNL_OPEN;
		    ** break;
		    */
		    status = E_DB_OK;
		    CLRDBERR(&jsx->jsx_dberr);
		    continue;
		}
		else
		{
		    uleFormat(&jsx->jsx_dberr, 0, NULL, ULE_LOG, NULL, 
				NULL, 0, NULL, &error, 0);
		    SETDBERR(&jsx->jsx_dberr, 0, E_DM1305_RFP_JNL_OPEN);
		    break;
		}
	    }

            /* initialize variables */
            record_time  = 1;
            byte_time    = 1;
            record_count = 0;
            byte_count   = 0;

	    if ( (!no_jnl) && ( phase == PROCESS ) )
	    TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
		"%@ RFP: Start processing journal file sequence %d.\n", i);

	    if (jsx->jsx_status & JSX_TRACE)
		TRdisplay("%@ RFP:Start processing journal file sequence %d.\n",
			     i);

	    /*  Loop through the records of this journal file. */

	    for ( ; ; )
	    {
		/*	Read the next record from this journal file. */

		status = dm0j_read(jnl, (PTR *)&record, &l_record, &jsx->jsx_dberr);
		if (status != E_DB_OK)
		{
		    if (jsx->jsx_dberr.err_code == E_DM0055_NONEXT)
		    {
			status = E_DB_OK;
			CLRDBERR(&jsx->jsx_dberr);
			break;
		    }
		    uleFormat(&jsx->jsx_dberr, 0, NULL, ULE_LOG, NULL, 
				NULL, 0, NULL, &error, 0);
		    SETDBERR(&jsx->jsx_dberr, 0, E_DM1305_RFP_JNL_OPEN);
		    if (last_rec_read_lsn.lsn_low || last_rec_read_lsn.lsn_high)
		    {
			rfp->rfp_redo_err = TRUE;
			rfp->rfp_status |= RFP_NO_CLRLOGGING;
			STRUCT_ASSIGN_MACRO(last_rec_read_lsn,
						rfp->rfp_redo_end_lsn);
			rfp->rfp_redo_end_lsn.lsn_low++;
			if (rfp->rfp_redo_end_lsn.lsn_low == 0)
			    rfp->rfp_redo_end_lsn.lsn_high += 1;
		    }
		    break;
		}
		else
		    STRUCT_ASSIGN_MACRO(record->lsn, last_rec_read_lsn);

                /*
                ** Increase count of records processed
                */
		if (phase == PROCESS)
                rfp->rfp_stats_blk.rfp_total_recs++;

		/*
		** Perform one-time check that start lsn not less than
		** the lsn of the first journal record.
		*/
		if ( (!lsn_check) && (jsx->jsx_status1 & JSX_START_LSN) )
		{
		    if (LSN_LT(&jsx->jsx_start_lsn, &record->lsn))
		    {
			char	    error_buffer[ER_MAX_LEN];
			i4	    error_length;
			i4	    count;

			uleFormat(NULL, E_DM1053_JSP_INV_START_LSN,
			    (CL_ERR_DESC *)NULL, ULE_LOOKUP, (DB_SQLSTATE *)NULL,
			    error_buffer, sizeof(error_buffer),
			    &error_length, &error, 5,
			    0, jsx->jsx_start_lsn.lsn_high,
			    0, jsx->jsx_start_lsn.lsn_low,
			    0, record->lsn.lsn_high,
			    0, record->lsn.lsn_low,
			    0, i);

			error_buffer[error_length] = EOS;
			SIwrite(error_length, error_buffer, &count, stdout);
			SIwrite(1, "\n", &count, stdout);
			SIflush(stdout);

			SETDBERR(&jsx->jsx_dberr, 0, E_DM1306_RFP_APPLY_RECORD);
			status = E_DB_ERROR;
			break;
		    }

		    lsn_check = TRUE;
		}

                /*
                ** Check counters to see if we should output a message.
                ** Note that the purpose of this code is so we can see that
                ** processing is happening.  In that regard, we want to count
                ** all records read, not just the ones that qualify with the
                ** the -b, -e, -start_lsn, and -end_lsn flags.
                */
                if (record_interval)
                {
                    record_count++;
                    if (record_count / (record_interval * record_time))
                    {
                        TRformat(dmf_put_line, 0, line_buffer,
                                sizeof(line_buffer),
                                "%@ RFP: Read journal record %d.\n",
                                record_count);
                        if (jsx->jsx_status & JSX_TRACE)
                           TRdisplay("%@ RFP: Read journal record %d.\n",
                                record_count);
                        record_time++;
                    }
                }
                if (byte_interval)
                {
                    byte_count+= l_record;
                    if (byte_count / (byte_interval * byte_time))
                    {
                        TRformat(dmf_put_line, 0, line_buffer,
                                sizeof(line_buffer),
                                "%@ RFP: Read %d bytes of journal file.\n",
                                byte_count);
                        if (jsx->jsx_status & JSX_TRACE)
                           TRdisplay("%@ RFP: Read %d bytes of journal file.\n",
                                byte_count);
                        byte_time++;
                    }
                }

		/*
		** Qualify the record for -e, -b, -start_lsn & -end_lsn flags.
		** These status bits are used just below.
		*/
		if ( (jsx->jsx_status & (JSX_BTIME | JSX_ETIME)) ||
		     (jsx->jsx_status1 & (JSX_START_LSN | JSX_END_LSN)) )
		{
		    rfp->rfp_status &= ~RFP_STRADDLE_BTIME;
		    qualify_record(jsx, rfp, record);
		}

		/*
		** If we before the begin time or start lsn, skip this record.
		*/
		if ((rfp->rfp_status & (RFP_BEFORE_BTIME | RFP_BEFORE_BLSN)) &&
			!(rfp->rfp_status & RFP_STRADDLE_BTIME))
		    continue;

		/*
		** If we are past the specified -e time then halt processing.
		**
		** If we are exactly at the specified -e time then process
		** this log record, but turn off the EXACT flag so that we
		** will halt processing before the next record.
		*/
		if (rfp->rfp_status & RFP_AFTER_ETIME)
		{
		    if (rfp->rfp_status & RFP_EXACT_ETIME)
			rfp->rfp_status &= ~RFP_EXACT_ETIME;
		    else
			break;
		}

		/*
		** If we match start or end lsn exactly, process this record,
		** we'll stop next time thru.
		** -start_lsn may equal -end_lsn, and both may
		** exactly match the user input.
		*/
		if (rfp->rfp_status & RFP_EXACT_LSN)
		    rfp->rfp_status &= ~RFP_EXACT_LSN;
                else if (rfp->rfp_status & RFP_AFTER_ELSN)
                    break;

		rfp->rfp_cur_tblcb = 0;

		if (jsx->jsx_status2 & JSX2_INVALID_TBL_LIST)
		{
		    if (( rfp_filter_log_record( rfp, jsx, record )) == 0)
		    {
			/*
			** Not interested so skip this record
			*/
			continue;
		    }
		}

                /* If we are here and no_jnl is set then that means we're
                   trying to recover over a non journaled period. Stop
                   processing immediately. */
                if (no_jnl)
                {
                    char        error_buffer[ER_MAX_LEN];
                    i4     error_length;
                    i4     count;
 
                    uleFormat(NULL, E_DM134C_RFP_INV_CHECKPOINT,
                        (CL_ERR_DESC *)NULL, ULE_LOOKUP, (DB_SQLSTATE *)NULL,
                        error_buffer, sizeof(error_buffer),
                        &error_length, &error, 2,
                        0, jsx->jsx_ckp_number,
                        0, rfp->jnl_first);
 
                    SIwrite(error_length, error_buffer, &count, stderr);
                    SIwrite(1, "\n", &count, stderr);
                    TRdisplay("%t\n", error_length, error_buffer);
                    status = E_DB_ERROR;
                    break;
                }

		jnl_read++;
		if (status == E_DB_OK)
		{
		    if (phase == PRESCAN)
			status = prescan_records(rfp, dmve, (PTR)record);
		    else
		        status = process_record(rfp, dmve, (PTR)record);
		}
		jnl_done++;

		if (status)
		{
		    /* Check if ok to continue */
		    dmfWriteMsg(&jsx->jsx_dberr, 0, 0);
		    SETDBERR(&jsx->jsx_dberr, 0, E_DM1306_RFP_APPLY_RECORD);
		    status = rfp_error(rfp, jsx, rfp->rfp_cur_tblcb,
				dmve, RFP_FAILED_TABLE, status);
		}

		if (status == E_DB_OK)
		    continue;

		/* Save lsn at time of failure */
		rfp->rfp_redo_err = TRUE;
		rfp->rfp_status |= RFP_NO_CLRLOGGING;
		STRUCT_ASSIGN_MACRO(record->lsn, rfp->rfp_redo_end_lsn);
		SETDBERR(&jsx->jsx_dberr, 0, E_DM1306_RFP_APPLY_RECORD);
		break;
	    }

	    /*  Close the current journal file. */

	    if (jnl)
	    {
		(VOID) dm0j_close(&jnl, &local_dberr);
		jnl = 0;

                /* End of journal.  Output final counts */
                if (record_interval)
                {
                    TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
                                "%@ RFP: Read total of %d journal records.\n",
                                record_count);
                    if (jsx->jsx_status & JSX_TRACE)
                        TRdisplay("%@ RFP: Read total of %d journal records.\n",
                                record_count);
                }
                if (byte_interval)
                {
                    TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
                        "%@ RFP: Read total of %d bytes of journal file.\n",
                        byte_count);
                    if (jsx->jsx_status & JSX_TRACE)
                        TRdisplay(
                        "%@ RFP: Read total of %d bytes of journal file.\n",
                        byte_count);
                }
	    }

	    if (status != E_DB_OK)
		return (status);

	    /*
	    ** If we are past the specified -e time or -end_lsn, then halt
	    ** processing.
	    */
	    if ((rfp->rfp_status & (RFP_AFTER_ETIME | RFP_AFTER_ELSN)) &&
		(phase == PROCESS))
		return (E_DB_OK);
	}

        /*
        ** FIXME lookat error handling
        */
        if ( status != E_DB_OK )
            return( status );
    }
  }   /* for(;;) loop */

    if (status != E_DB_OK)
	return (status);

    /*
    ** Call the OpenJournal end routine
    */
    status = dmckp_end_journal_phase( d, &error );
    if ( status != E_DB_OK )
    {
	SETDBERR(&jsx->jsx_dberr, 0, error);
        return( status );
    }

    return (E_DB_OK);
}

/*{
** Name: read_journals_tbl  - Read and apply journal records.
**
** Description:
**      This routine reads all the journal records and call the
**      apply routine to make the actual changes.
**
**      There may be multiple journal histories all being rolled forward,
**      particular if this is a recovery from an old checkpoint. rfp->jnl_first
**      is the number of the first history, and rfp->jnl_last is the number
**      of the last history. For each history, we roll-forward the journal
**      files numbered ckp_f_jnl through ckp_l_jnl (which may be no files at
**      all if no journalling was done in this history -- both numbers will
**      be 0 in that case).
**
** Inputs:
**      rfp                             Rollforward context.
**      dmve                            Recovery control block.
**
** Outputs:
**      err_code                        Reason for error return status.
**      Returns:
**          E_DB_OK
**          E_DB_ERROR
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**	06-dec-1994 (andyw)
**	    Created.
**	30-jun-1995 (shero03)
**	    Ignore any journal errors if -ignore is on
**      20-july-1995 (thaju02)
**          bug #69981 - when ignore argument specified, output error
**          message to error log, and mark database inconsistent
**          for db level rolldb or table physically inconsistent
**          for tbl level rolldb, since rolldb will continue with
**          application of journal records.
**      28-jul-1998 (nicph02)
**          Completed implementation of Sir 86486 for table rollforward.
**          Implemented 2 environment variables (logicals) having to do with
**          optionally giving more messages during the rollforward of
**          journals.
**      29-jul-1998 (nicph02)
**          bug 90541: Allow users to apply journals that are older than
**          the first valid journal. The new logic is that it is assumed that
**          there is no missing journals (no ckpdb -j done). If one is found
**          during processing, then rollforward is aborted (E_DM134C logged)
**      19-nov-1998 (nicph02)
**          rollforwarddb -e<time>: If there is no transaction to recover before
**          the next non journaled ckp, failure with E_DM134C. Problem is that
**          we need to read the next jnl record (after the missing jnl!) that
**          is after -e<time> thus stopping the rollforwarddb (bug 94219).
**
*/
static DB_STATUS
rfp_read_journals_tbl(
DMF_JSX             *jsx,
DMF_RFP             *rfp,
DMVE_CB             *dmve)
{

    DM0J_CTX		*jnl;
    DM0L_HEADER		*record;
    i4		i;
    i4		l_record;
    i4             record_interval;
    i4             record_time;
    i4             record_count;
    i4             byte_interval;
    i4             byte_time;
    i4             byte_count;
    DB_STATUS		status;
    char		line_buffer[132];
    bool		lsn_check = FALSE;
    DM_FILE             jnl_filename;
    DMCKP_CB            *d = &rfp->rfp_dmckp;
    char                *env;
    bool                no_jnl = FALSE;
    i4			jnl_read = 0;
    i4			jnl_done = 0;
    i4			error;
    DB_ERROR		local_dberr;

    CLRDBERR(&jsx->jsx_dberr);

    /*
    ** Initialise the DMCKP_CB structure, used during the OPENjournal stuff.
    */
    status = dmckp_init( d, DMCKP_RESTORE, &error );
    if ( status != E_DB_OK )
    {
	SETDBERR(&jsx->jsx_dberr, 0, error);
        return( status );
    }

    /*
    ** Setup the DMCKP stuff which will remain constant
    */
    d->dmckp_jnl_phase   = DMCKP_JNL_DO;
    d->dmckp_device_type = DMCKP_DISK_FILE;
    d->dmckp_jnl_path    = dmve->dmve_dcb_ptr->dcb_jnl->physical.name;
    d->dmckp_jnl_l_path  = dmve->dmve_dcb_ptr->dcb_jnl->phys_length;

    /*
    ** FIXME, test that this does not SEGV ever...
    */
    d->dmckp_first_jnl_file = rfp->rfp_jnl_history[rfp->jnl_first].ckp_f_jnl;
    d->dmckp_last_jnl_file  = rfp->rfp_jnl_history[rfp->jnl_last].ckp_l_jnl;


    /*
    ** Call the OpenJournal begin routine
    */
    status = dmckp_begin_journal_phase( d, &error );
    if ( status != E_DB_OK )
    {
	SETDBERR(&jsx->jsx_dberr, 0, error);
        return( status );
    }

    /*
    ** Check to see if the env. variables are set.  If not set, initialize
    ** to zero so they won't be used.
    */
    NMgtAt(ERx("II_RFP_JNL_RECORD_INTERVAL"), &env);
    if (env && *env)
        CVal(env, &record_interval);
    else
        record_interval = 0;
 
    NMgtAt(ERx("II_RFP_JNL_BYTE_INTERVAL"), &env);
    if (env && *env)
        CVal(env, &byte_interval);
    else
        byte_interval = 0;

    /*
    ** For each journal history to be rolled forward, loop through the journal
    ** file sequence numbers used in that history and apply them.
    */

    for (rfp->jnl_history_number =  rfp->jnl_first;
	 rfp->jnl_history_number <= rfp->jnl_last;
	 rfp->jnl_history_number++)
    {
	for (i =  rfp->rfp_jnl_history[rfp->jnl_history_number].ckp_f_jnl;
	     i <= rfp->rfp_jnl_history[rfp->jnl_history_number].ckp_l_jnl;
	     i++)
	{
	    /*
	    ** A journal history with both numbers 0 indicates that no
	    ** journaling went on in this history, so we proceed to the
	    ** next history.
	    */
	    if (i == 0)
            {
               /* no_jnl is set to indicate that we skip over a non journaled
                  period. The next record to be read should be greater than
                  -e time ot there is a problem! */
               no_jnl = TRUE;
               break;
            }

            /*
            ** Determine next journal file name for the OpenJournal
            */
            dm0j_filename( DM0J_MERGE, i, -1, &jnl_filename, (DB_TAB_ID *)NULL );

            d->dmckp_jnl_file   = jnl_filename.name;
            d->dmckp_jnl_l_file = sizeof( jnl_filename );

            status = dmckp_work_journal_phase( d, &error );
            if ( status != E_DB_OK )
	    {
		SETDBERR(&jsx->jsx_dberr, 0, error);
                break;
	    }


	    /*  Open the next journal file. */

	    status = dm0j_open(DM0J_MERGE,
			(char *)&dmve->dmve_dcb_ptr->dcb_jnl->physical,
			dmve->dmve_dcb_ptr->dcb_jnl->phys_length,
			&dmve->dmve_dcb_ptr->dcb_name,
			rfp->jnl_block_size, i, 0, 0, DM0J_M_READ,
			-1, DM0J_FORWARD, (DB_TAB_ID *)0, &jnl, &jsx->jsx_dberr);
	    if (status != E_DB_OK)
	    {
		if (jsx->jsx_dberr.err_code == E_DM9034_BAD_JNL_NOT_FOUND)
		{
		    uleFormat(&jsx->jsx_dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL,
				&error, 0);
		    TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
		    "%@ RFP: Journal file sequence %d not found -- skipped.\n",
		    i );

		    /* try next journal...we're trying to be fault tolerant.
		    ** This is a highly controversial decision, and may get
		    ** changed.
		    ** *err_code = E_DM1305_RFP_JNL_OPEN;
		    ** break;
		    */
		    status = E_DB_OK;
		    CLRDBERR(&jsx->jsx_dberr);
		    continue;
		}
		else
		{
		    uleFormat(&jsx->jsx_dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL,
				&error, 0);
		    SETDBERR(&jsx->jsx_dberr, 0, E_DM1305_RFP_JNL_OPEN);
		    break;
		}
	    }

            /* initialize variables */
            record_time  = 1;
            byte_time    = 1;
            record_count = 0;
            byte_count   = 0;

            if (!no_jnl)
	    TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
		"%@ RFP: Start processing journal file sequence %d.\n", i);

	    if (jsx->jsx_status & JSX_TRACE)
		TRdisplay("%@ RFP:Start processing journal file sequence %d.\n",
			     i);

	    /*  Loop through the records of this journal file. */

	    for ( ; ; )
	    {
		/*	Read the next record from this journal file. */

		status = dm0j_read(jnl, (PTR *)&record, &l_record, &jsx->jsx_dberr);
		if (status != E_DB_OK)
		{
		    if (jsx->jsx_dberr.err_code == E_DM0055_NONEXT)
		    {
			status = E_DB_OK;
			CLRDBERR(&jsx->jsx_dberr);
			break;
		    }
		    uleFormat(&jsx->jsx_dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL,
				&error, 0);
		    SETDBERR(&jsx->jsx_dberr, 0, E_DM1305_RFP_JNL_OPEN);
		    break;
		}
                /*
                ** Increase count of records processed
                */
                rfp->rfp_stats_blk.rfp_total_recs++;

		/*
		** Perform one-time check that start lsn not less than
		** the lsn of the first journal record.
		*/
		if ( (!lsn_check) && (jsx->jsx_status1 & JSX_START_LSN) )
		{
		    if (LSN_LT(&jsx->jsx_start_lsn, &record->lsn))
		    {
			char	    error_buffer[ER_MAX_LEN];
			i4	    error_length;
			i4	    count;

			uleFormat(NULL, E_DM1053_JSP_INV_START_LSN,
			    (CL_ERR_DESC *)NULL, ULE_LOOKUP, (DB_SQLSTATE *)NULL,
			    error_buffer, sizeof(error_buffer),
			    &error_length, &error, 5,
			    0, jsx->jsx_start_lsn.lsn_high,
			    0, jsx->jsx_start_lsn.lsn_low,
			    0, record->lsn.lsn_high,
			    0, record->lsn.lsn_low,
			    0, i);

			error_buffer[error_length] = EOS;
			SIwrite(error_length, error_buffer, &count, stdout);
			SIwrite(1, "\n", &count, stdout);
			SIflush(stdout);

			SETDBERR(&jsx->jsx_dberr, 0, E_DM1306_RFP_APPLY_RECORD);
			status = E_DB_ERROR;
			break;
		    }

		    lsn_check = TRUE;
		}

                /*
                ** Check counters to see if we should output a message.
                ** Note that the purpose of this code is so we can see that
                ** processing is happening.  In that regard, we want to count
                ** all records read, not just the ones that qualify with the
                ** the -b, -e, -start_lsn, and -end_lsn flags.
                */
                if (record_interval)
                {
                    record_count++;
                    if (record_count / (record_interval * record_time))
                    {
                        TRformat(dmf_put_line, 0, line_buffer,
                                sizeof(line_buffer),
                                "%@ RFP: Read journal record %d.\n",
                                record_count);
                        if (jsx->jsx_status & JSX_TRACE)
                           TRdisplay("%@ RFP: Read journal record %d.\n",
                                record_count);
                        record_time++;
                    }
                }
                if (byte_interval)
                {
                    byte_count+= l_record;
                    if (byte_count / (byte_interval * byte_time))
                    {
                        TRformat(dmf_put_line, 0, line_buffer,
                                sizeof(line_buffer),
                                "%@ RFP: Read %d bytes of journal file.\n",
                                byte_count);
                        if (jsx->jsx_status & JSX_TRACE)
                           TRdisplay("%@ RFP: Read %d bytes of journal file.\n",
                                byte_count);
                        byte_time++;
                    }
                }

		/*
		** Qualify the record for -e, -b, -start_lsn & -end_lsn flags.
		** These status bits are used just below.
		*/
		if ( (jsx->jsx_status & (JSX_BTIME | JSX_ETIME)) ||
		     (jsx->jsx_status1 & (JSX_START_LSN | JSX_END_LSN)) )
		{
		    qualify_record(jsx, rfp, record);
		}

		/*
		** If we before the begin time or start lsn, skip this record.
		*/
		if (rfp->rfp_status & (RFP_BEFORE_BTIME | RFP_BEFORE_BLSN))
		    continue;

		/*
		** If we are past the specified -e time then halt processing.
		**
		** If we are exactly at the specified -e time then process
		** this log record, but turn off the EXACT flag so that we
		** will halt processing before the next record.
		*/
		if (rfp->rfp_status & RFP_AFTER_ETIME)
		{
		    if (rfp->rfp_status & RFP_EXACT_ETIME)
			rfp->rfp_status &= ~RFP_EXACT_ETIME;
		    else
			break;
		}

		/*
		** If we match start or end lsn exactly, process this record,
		** we'll stop next time thru.
		** -start_lsn may equal -end_lsn, and both may
		** exactly match the user input.
		*/
		if (rfp->rfp_status & RFP_EXACT_LSN)
		    rfp->rfp_status &= ~RFP_EXACT_LSN;
                else if (rfp->rfp_status & RFP_AFTER_ELSN)
                    break;

                /*
                ** If partial rollforward then see if we are interested
                ** in this record
                */
		rfp->rfp_cur_tblcb = 0;
                if (( rfp_filter_log_record( rfp, jsx, record )) == 0)
                {
                    /*
                    ** Not interested so skip this record
                    */
                    continue;
                }


		if (jsx->jsx_status & JSX_TRACE)
		    dmd_log(FALSE, (PTR)record, rfp->jnl_block_size);

                /* If we are here and no_jnl is set then that means we're
                   trying to recover over a non journaled period. Stop
                   processing immediately. */
                if (no_jnl)
                {
                   char        error_buffer[ER_MAX_LEN];
                   i4     error_length;
                   i4     count;
 
                   uleFormat(NULL, E_DM134C_RFP_INV_CHECKPOINT,
                        (CL_ERR_DESC *)NULL, ULE_LOOKUP, (DB_SQLSTATE *)NULL,
                        error_buffer, sizeof(error_buffer),
                        &error_length, &error, 2,
                        0, jsx->jsx_ckp_number,
                        0, rfp->jnl_first);
 
                   SIwrite(error_length, error_buffer, &count, stderr);
                   SIwrite(1, "\n", &count, stderr);
                   TRdisplay("%t\n", error_length, error_buffer);
                   status = E_DB_ERROR;
                   break;
                }

		status = process_record(rfp, dmve, (PTR)record);

		if (status)
		{
		    /* Check if ok to continue */
		    dmfWriteMsg(&jsx->jsx_dberr, 0, 0);
		    SETDBERR(&jsx->jsx_dberr, 0, E_DM1306_RFP_APPLY_RECORD);
		    status = rfp_error(rfp, jsx, rfp->rfp_cur_tblcb,
				dmve, RFP_FAILED_TABLE, status);
		}

		if (status == E_DB_OK)
		{
		    CLRDBERR(&jsx->jsx_dberr);
		    continue;
		}

		SETDBERR(&jsx->jsx_dberr, 0, E_DM1306_RFP_APPLY_RECORD);
		break;
	    }

	    /*  Close the current journal file. */

	    if (jnl)
	    {
		(VOID) dm0j_close(&jnl, &local_dberr);

                /* End of journal.  Output final counts */
                if (record_interval)
                {
                    TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
                                "%@ RFP: Read total of %d journal records.\n",
                                record_count);
                    if (jsx->jsx_status & JSX_TRACE)
                        TRdisplay("%@ RFP: Read total of %d journal records.\n",
                                record_count);
                }
                if (byte_interval)
                {
                    TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
                        "%@ RFP: Read total of %d bytes of journal file.\n",
                        byte_count);
                    if (jsx->jsx_status & JSX_TRACE)
                        TRdisplay(
                        "%@ RFP: Read total of %d bytes of journal file.\n",
                        byte_count);
                }
	    }

	    if (status != E_DB_OK)
		return (status);

	    /*
	    ** If we are past the specified -e time or -end_lsn, then halt
	    ** processing.
	    */
	    if (rfp->rfp_status & (RFP_AFTER_ETIME | RFP_AFTER_ELSN))
		return (E_DB_OK);
	}

        /*
        ** FIXME lookat error handling
        */
        if ( status != E_DB_OK )
            return( status );
    }

    if (status != E_DB_OK)
	return (status);

    /*
    ** Call the OpenJournal end routine
    */
    status = dmckp_end_journal_phase( d, &error );
    if ( status != E_DB_OK )
    {
	SETDBERR(&jsx->jsx_dberr, 0, error);
        return( status );
    }

    return (E_DB_OK);
}

/*{
** Name: read_dumps	- Read and undo log records.
**
** Description:
**      This routine reads all the log records and call the
**	apply routine to make the actual undo changes.
**
** Inputs:
**	rfp				Rollforward context.
**	dmve				Recovery control block.
**
** Outputs:
**      err_code                        Reason for error return status.
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
**      13-jan-1989 (Edhsu)
**          Created for Terminator.
**	22-nov-1989 (rogerk)
**	    When finished with dump file, break out of read loop so we can
**	    process the next dump file rather than returning from routine.
**	10-feb-1993 (jnash)
**	    Add FALSE param to dmd_log().
**	24-may-1993 (andys)
**	    Pass appropriate logfile blocksize to dmd_log and dmd_format_log
**	    rather than using hardwired 4096.
**	20-sep-1993 (andys)
**	     Fix arguments to ule_format call.
**	14-sep-1994 (andyw)
**	    Partial Backup/Recovery Project:
**		Configure DMCKP_CB structure whilst reading the dump xtns
**	15-jan-1999 (nanpr01)
**	    Pass pointer to pointer on dm0d_close routine.
[@history_template@]...
*/
static DB_STATUS
read_dumps(
DMF_JSX		    *jsx,
DMF_RFP		    *rfp,
DMVE_CB		    *dmve)
{
    DM0D_CTX		*dmp;
    DM0L_HEADER		*record;
    i4		i;
    i4		l_record;
    DB_STATUS		status;
    char		line_buffer[132];
    DMCKP_CB             *d = &rfp->rfp_dmckp;
    DM_FILE             dmp_filename;
    i4			error;
    DB_ERROR		local_dberr;

    CLRDBERR(&jsx->jsx_dberr);


    /*
    ** Initialise the DMCKP_CB structure, used during the OPENdump.
    */
    status = dmckp_init( d, DMCKP_RESTORE, &error );
    if ( status != E_DB_OK )
    {
	SETDBERR(&jsx->jsx_dberr, 0, error);
        return( status );
    }

    /*
    ** Setup the DMCKP stuff which will remain constant
    */
    d->dmckp_device_type = DMCKP_DISK_FILE;
    d->dmckp_dmp_path    = rfp->cur_dmp_loc.physical.name;
    d->dmckp_dmp_l_path  = rfp->cur_dmp_loc.phys_length;

    /*
    ** FIXME, howcome these do not index into the CKP_HISTORY array the
    ** sameway as journal files do ?
    */
    d->dmckp_first_dmp_file = rfp->dmp_first;
    d->dmckp_last_dmp_file  = rfp->dmp_last;


    /*
    ** Call the OpenJournal begin routine
    */
    status = dmckp_begin_dump_phase( d, &error );
    if ( status != E_DB_OK )
    {
	SETDBERR(&jsx->jsx_dberr, 0, error);
        return( status );
    }


    for (i = rfp->dmp_last; i >= rfp->dmp_first; i--)
    {
        /*
        ** setup and call the dump work phase for OpenCheckpoint
        */
        dm0d_filename( DM0D_MERGE, i, -1, &dmp_filename );

        d->dmckp_dmp_file   = dmp_filename.name;
        d->dmckp_dmp_l_file = sizeof( dmp_filename );

        status = dmckp_work_dump_phase( d, &error );
        if ( status != E_DB_OK )
	{
	    SETDBERR(&jsx->jsx_dberr, 0, error);
            break;
	}

	/*  Open the next dump file. */

	status = dm0d_open(DM0D_MERGE, 
	    (char *)&dmve->dmve_dcb_ptr->dcb_dmp->physical,
	    dmve->dmve_dcb_ptr->dcb_dmp->phys_length,
	    &dmve->dmve_dcb_ptr->dcb_name, rfp->dmp_block_size, i, 0,
	    DM0D_EXTREME, DM0D_M_READ, -1, DM0D_BACKWARD, &dmp, &jsx->jsx_dberr);
	if (status != E_DB_OK)
	{
	    uleFormat(&jsx->jsx_dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL, &error, 0);
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM1324_RFP_DMP_OPEN);
	    break;
	}

	TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
	    "%@ RFP: Start processing dump file sequence %d.\n", i);

	if (jsx->jsx_status & JSX_TRACE)
	    TRdisplay("%@ RFP:Start processing dump file sequence %d.\n", i);

	/*  Loop through the records of this dump file. */

	for (;;)
	{
	    /*	Read the next record from this dump file. */

	    status = dm0d_read(dmp, (PTR *)&record, &l_record, &jsx->jsx_dberr);
	    if (status != E_DB_OK)
	    {
		if (jsx->jsx_dberr.err_code == E_DM0055_NONEXT)
		{
		    status = E_DB_OK;
		    CLRDBERR(&jsx->jsx_dberr);
		    break;
		}
		uleFormat(&jsx->jsx_dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL, &error, 0);
		SETDBERR(&jsx->jsx_dberr, 0, E_DM1339_RFP_DMP_READ);
		break;
	    }

            rfp->rfp_stats_blk.rfp_total_dump_recs++;

            /*
            ** If partial rollforward then see if we are interested
            ** in this record
            */
	    rfp->rfp_cur_tblcb = 0;
            if (( jsx->jsx_status1 & JSX1_RECOVER_DB ) == 0
		|| (jsx->jsx_status2 & JSX2_INVALID_TBL_LIST))
            {
                if (!( rfp_filter_log_record( rfp, jsx, record )))
                {
                    /*
                    ** Not interested so skip this record
                    */
                    continue;
                }
            }

	    if (jsx->jsx_status & JSX_TRACE)
		dmd_log(FALSE, (PTR)record, rfp->dmp_block_size);

	    status = apply_d_record(rfp, jsx, dmve, record);

	    if (status)
	    {
		/* Check if ok to continue */
		dmfWriteMsg(&jsx->jsx_dberr, 0, 0);
		SETDBERR(&jsx->jsx_dberr, 0, E_DM1325_RFP_APPLY_DRECORD);
		status = rfp_error(rfp, jsx, rfp->rfp_cur_tblcb,
			    dmve, RFP_FAILED_TABLE, status);
	    }

	    if (status == E_DB_OK)
	    {
		CLRDBERR(&jsx->jsx_dberr);
		continue;
	    }

	    SETDBERR(&jsx->jsx_dberr, 0, E_DM1325_RFP_APPLY_DRECORD);
	    break;
	}

	/*  Close the current dump file. */

	if (dmp)
	{
	    (VOID) dm0d_close(&dmp, &local_dberr);
	}

	if (status != E_DB_OK)
	    return (status);
    }

    if (status != E_DB_OK)
	return (status);

    /*
    ** Call the Opendump end routine
    */
    status = dmckp_end_dump_phase( d, &error );
    if ( status != E_DB_OK )
    {
	SETDBERR(&jsx->jsx_dberr, 0, error);
        return( status );
    }

    return (E_DB_OK);
}

/*{
** Name: validate_nockp	- Verify proper use of the -c flag
**
** Description:
**	Before allowing use of the -c flag, verify that the database has
**	previously been restored to a valid checkpoint.
**
**	The -c flag directs rollforward to do journal processing using
**	the database as it currently exists rather than first overlaying
**	the database from a backup saveset.
**
**	It is expected that -c rollforwards are used to complete restoration
**	of a database that has been partially restored using the -j or -e
**	command flags, ie:
**
**		rollforward +c -j    # restore checkpoint but don't do journals
**		rollforward -c +j    # now apply the journal work
**
**			OR
**
**		rollforward +c +j -e 10:15  # rollforward to 10:15 a.m.
**		rollforward -c +j -b 10:15  # complete rollforward processing
**
**	A -c rollforward which starts off with a database state which is
**	not consistent with the current command line flag (like using
**	"-c +j" without having first done a "+c -j") will almost certainly
**	fail when journal updates are applied to a database that is not
**	in the expected state.
**
**	This routine makes some consistency checks to verify that the database
**	is in a state from which it can be rolled forward.  It currently
**	makes very cursory checks and fails only if the database state
**	indicates that a previous rollforward attempt ended abnormally without
**	completing the cleanup code.  This is recognized either by the existence
**	of an RFC config file (which is deleted in cleanup processing) or by
**	the assertion of the DSC_CKP_INPROGRESS bit in the config file (which
**	is turned off by cleanp processing).
**
**	We do not enforce the requirement that a checkpoint had been
**	restored (in order to permit use the -c flag).
**
** Inputs:
**	jsx				Pointer to Journal Support info.
**      dcb				Pointer to DCB for this database.
**
** Outputs:
**      err_code                        Reason for error return status.
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    Config file opened and closed.
**
** History:
**	21-jun-1993 (rogerk)
**	    Written and code moved here from restore_ckp to make the
**	    code easier to read.  Cleaned up the code and added error
**	    checks.  Changed processing that occurs when RFC is found
**	    to error rather than continuing in +c mode.
**	23-aug-96 (nick)
**	    Only check DSC_CKP_INPROGRESS for database level recovery as
**	    it is meaningless for table level recovery. #77901
*/
static	DB_STATUS
validate_nockp(
DMF_JSX		    *jsx,
DMP_DCB		    *dcb)
{
    DM0C_CNF		*cnf = NULL;
    DB_STATUS		status;
    i4		local_error;
    DB_ERROR		local_dberr;

    CLRDBERR(&jsx->jsx_dberr);

    if ( jsx->jsx_status2 & JSX2_INCR_JNL)
	return (E_DB_OK);

    /*
    ** Assuming that the user is using rollforward -c after a previous
    ** run of rollforward with the +c flag, make sure that the previous
    ** rollforward restored the database properly and completed normally.
    ** Otherwise the user must start the full rollforward all over again.
    */
    for (;;)
    {
	/*
	** Check if the RFC config file exists.  If it does then the previous
	** rollforward attempt must have failed.
	*/
	status = config_handler(jsx, OPENRFC_CONFIG, dcb, &cnf);
	if (status != E_DB_INFO)
	{
	    if (status == E_DB_OK)
	    {
		status = E_DB_ERROR;
		SETDBERR(&jsx->jsx_dberr, 0, E_DM1311_RFP_MUST_RESTORE);
	    }
	    break;
	}

	/*
	** Open normal configuration file.
	*/
	status = config_handler(jsx, OPEN_CONFIG, dcb, &cnf);
	if (status != E_DB_OK)
	    break;

	/*
	** Check if the config file indicates that the previous attempt
	** to rollforward failed.
	*/
	if ((jsx->jsx_status1 & JSX1_RECOVER_DB) &&
	    (cnf->cnf_dsc->dsc_status & DSC_CKP_INPROGRESS))
	{
	    status = E_DB_ERROR;
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM1311_RFP_MUST_RESTORE);
	    break;
	}

	status = config_handler(jsx, CLOSEKEEP_CONFIG, dcb, &cnf);
	if (status != E_DB_OK)
	    break;

	return (E_DB_OK);
    }

    if (cnf)
    {
	local_dberr = jsx->jsx_dberr;

	status = config_handler(jsx, CLOSEKEEP_CONFIG, dcb, &cnf);
	if (status != E_DB_OK)
	{
	    uleFormat(&jsx->jsx_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &local_error, 0);
	}
	jsx->jsx_dberr = local_dberr;
    }

    if (jsx->jsx_dberr.err_code != E_DM1311_RFP_MUST_RESTORE)
    {
        uleFormat(&jsx->jsx_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
	    (char *)NULL, (i4)0, (i4 *)NULL, &local_error, 0);
	SETDBERR(&jsx->jsx_dberr, 0, E_DM1348_RFP_CHECK_NOCKP);
    }

    return (E_DB_ERROR);
}

/*{
** Name: db_restore_ckp	- Restore checkpoints.
**
** Description:
**      This routine manages the restoration of the checkpoint
**	files for each location.  Locations that are aliases
**	are detected and only restored once.
**
**	The correct information about all of the extra locations is contained
**	in the .cnf file which is in the dump directory. The current .cnf file
**	may not have the correct information, due to location changes which
**	may have occurred since the checkpoint was taken. Therefore, we first
**	restore the .cnf file from the dump directory, then open that file and
**	read it in to learn about the locations which were part of that
**	checkpoint. Then we restore all the locations (simultaneously if the
**	underlying CK routines support it), and then we re-restore the .cnf
**	file from the dump location and update it with the history information
**	from our 'rfp' context. The reason for the second restoring of the
**	.cnf file from the dump directory is that we cannot trust the contents
**	of the CK version of the .cnf file since it may have been being updated
**	while the checkpoint was taken (online checkpoint) and we do not have
**	before images of the .cnf file.
**
** Inputs:
**	rfp				Rollforward context.
**	jsx				Pointer to Journal Support info.
**      dcb				Pointer to DCB for this database.
**
** Outputs:
**      err_code                        Reason for error return status.
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
**      01-nov-1986 (Derek)
**          Created for Jupiter.
**	02-feb-1989 (EdHsu)
**	    Online backup support.
**	04-may-1989 (Sandyh)
**	    Cleaned up start messages for UNIX, bug 4630.
**	12-oct-1989 (Sandyh)
**	    Fixed bug 7624 - A/V on TRformat during RFP from tape.
**	 6-dec-1989 (rogerk)
**	    When restore from an online backup, restore the config file
**	    from the DUMP version.
**	29-jan-1990 (sandyh)
**	    Fixed check_params.found handling. The value was being set for
**	    the default location ckp file, but not reset for the extended
**	    locations ckp's. Also, the wrong filename was used to lookup
**	    since the filename assignment happened after the check. This
**	    problem was reported be ICL. It caused the error to occur at
**	    restore time instead of at validation.
**	29-apr-1990 (walt)
**	    Reordered some steps so that rollforwarddb can restore all the
**	    checkpoint locations concurrently.  Formerly the default location
**	    had to be restored first to get the config file for information
**	    about the locations.  Now we save a copy of the config file in
**	    the dump location for all checkpoints (not just online backups)
**	    and use it first to get the location information.  This copy
**	    of the config file must be "re-implanted" into the default location
**	    after the restore because it's more dependable than the one
**	    saved during the checkpoint if the backup was an online backup.
**	7-may-90 (bryanp)
**	    Use the checkpoint sequence number in rfp->ckp_seq. Don't update
**	    the history and open_count before the restore, since the restore
**	    overwrites the .cnf file with the old version. Instead, update the
**	    history and open_count AFTER the restore.
**	04-nov-92 (jrb)
**	    Renamed DCB_SORT to DCB_WORK.
**	21-jun-93 (rogerk)
**	    Moved checks for legality of -c flag to validate_nockp routine.
**	20-sep-1993 (andys)
**	     Fix arguments to ule_format call.
**	14-sep-1994 (andyw)
**	    Partial Backup/Recovery Project:
**	 	Rename to db_restore_ckp
**	    Changed to move assignment of filename.name to before the
**	    assignment to the dmckp structure. Also set the physical
**	    path only show the pathname (this is normally assigned within
**	    a '%~t' against two values)
**      12-Dec-1994 (liibi01)
**          Cross integration from 6.4 (Bug 56364).
**          Mark database inconsistent if we have failed to rollforward
**          because the expected alias files couldn't be found.
**	09-mar-1995 (shero03)
**	    Bug B67276
**	    Since the ckp, dmp, or jnl directories may have been 
**	    relocated since this checkpoint, save their locations
**	    from the current config file.
**	22-mar-1995 (shero03)
**	    Bug #67410
**	    Ensure you delete the files in all locations before restoring
**	    the data.
**      11-apr-1995 (stial01) 
**          Another fix to support relocation of the dmp directory. 
**          Since the dmp location may have changed fix the DCB_DUMP entry
**          in the dcb before calling dm0d_config_restore()
**	13-mar-1997 (shero03)
**	    Bug from Beta site: rollforwarddb after relocate -new_ckp_location
**	    fails to find the checkpoint file after it deletes the database.
**	    Changed db_restore_ckp to use the present ckp location
**	5-march-1997(angusm)
**	    bug 49057: We now add back in to config file any data 
**	    locations added since this checkpoint, but we need
**	    to clear those locations down, else tables which have been
**	    relocated since this checkpoint will have files in both places.
**	22-may-1997 (hanch04)
**	    ext needs to be set to dcb->dcb_ext of dcb is set up.
**      28-may-1998 (wanya01)
**          Change location name in the console message when restoring of
**          extended data location 
**	21-oct-1999 (hayke02)
**	    17-Aug-1998 (wanya01)
**	    Bug 87552: We need to skip deleting files under alias location
**	    since it may has been done already. Specially for alias location
**	    which points to root data location.
**	    10-Nov-1998 (wanya01)
**	    Change from STcompare() to MEcmp() for location name comparison.
[@history_template@]...
*/
static	DB_STATUS
db_restore_ckp(
DMF_RFP		    *rfp,
DMF_JSX		    *jsx,
DMP_DCB		    *dcb)
{
    DMP_EXT		*ext;
    DM0C_CNF		*cnf;
    i4             i;
    i4		sequence = 0;
    DB_STATUS		status;
    CL_ERR_DESC		sys_err;
    DI_IO		di_file;
    DM_FILE		filename;
    DM_FILE		alias_file;
    char		line_buffer[132];
    i4		ckp_sequence;
    DMCKP_CB		*d = &rfp->rfp_dmckp;
    i4			error;

    CLRDBERR(&jsx->jsx_dberr);

    /*
    ** Initialise the DMCKP_CB structure
    */
    status = dmckp_init( d, DMCKP_RESTORE, &error );
    if ( status != E_DB_OK )
    {
	SETDBERR(&jsx->jsx_dberr, 0, error);
        return( status );
    }

    ckp_sequence = rfp->ckp_seq;
    sequence++;

    filename.name[0] = 'c';
    filename.name[1] = (ckp_sequence / 1000) + '0';
    filename.name[2] = ((ckp_sequence / 100) % 10) + '0';
    filename.name[3] = ((ckp_sequence / 10) % 10) + '0';
    filename.name[4] = (ckp_sequence % 10) + '0';
    filename.name[5] = (sequence / 100) + '0';
    filename.name[6] = ((sequence / 10) % 10) + '0';
    filename.name[7] = (sequence % 10) + '0';
    filename.name[8] = '.';
    filename.name[9] = 'c';
    filename.name[10] = 'k';
    filename.name[11] = 'p';
    alias_file = *(DM_FILE *)"zzzz0000.ali";
    alias_file.name[4] = ((ckp_sequence / 1000) % 10) + '0';
    alias_file.name[5] = ((ckp_sequence / 100) % 10) + '0';
    alias_file.name[6] = ((ckp_sequence / 10) % 10) + '0';
    alias_file.name[7] = (ckp_sequence % 10) + '0';

    /*
    ** Setup the CK information which will remain constant across the
    ** the restore of each location. Initialize the other fields...
    */
    d->dmckp_op_type       = DMCKP_RESTORE_DB;
    d->dmckp_ckp_file      = filename.name;
    d->dmckp_ckp_l_file    = sizeof(filename);

    /*
    **  Bug b67276
    **  Save the ckp, dmp, and jnl locations from the current config
    */
    STRUCT_ASSIGN_MACRO(*dcb->dcb_ckp, rfp->cur_ckp_loc);
    *(rfp->cur_ckp_loc.physical.name + rfp->cur_ckp_loc.phys_length) = '\0';
    STRUCT_ASSIGN_MACRO(*dcb->dcb_dmp, rfp->cur_dmp_loc);
    *(rfp->cur_dmp_loc.physical.name + rfp->cur_dmp_loc.phys_length) = '\0';
    STRUCT_ASSIGN_MACRO(*dcb->dcb_jnl, rfp->cur_jnl_loc);
    *(rfp->cur_jnl_loc.physical.name + rfp->cur_jnl_loc.phys_length) = '\0';

    if (jsx->jsx_status & JSX_DEVICE)
    {
        d->dmckp_ckp_path    = jsx->jsx_device_list[0].dev_name;
        d->dmckp_ckp_l_path  = jsx->jsx_device_list[0].l_device;
        d->dmckp_device_type = DMCKP_TAPE_FILE;
	d->dmckp_dev_cnt     = jsx->jsx_dev_cnt;
    }
    else
    {
 	*(dcb->dcb_ckp->physical.name + dcb->dcb_ckp->phys_length) = '\0';
        d->dmckp_ckp_path    = rfp->cur_ckp_loc.physical.name;
        d->dmckp_ckp_l_path  = rfp->cur_ckp_loc.phys_length;
        d->dmckp_device_type = DMCKP_DISK_FILE;
	d->dmckp_dev_cnt     = jsx->jsx_dev_cnt;
    }

    /*
    **  Construct filename for checkpoint.
    **	Notice that if the ckp is run by online mode, we trust dmp sequence.
    **	Otherwise, we must use jnl sequence. The sequence number decision is
    **	made by prepare() and is passed in to us through the rfp.
    */

    /* Make sure all checkpoint files exists, */

    status = rfp_check_snapshots_exist( rfp, jsx, dcb, &d->dmckp_num_locs );
    if ( status != E_DB_OK )
    {
        return( status );
    }

    /*
    ** We saved a copy of the config file in the DUMP directory.
    ** Restore using the DUMP version.
    */
    if (jsx->jsx_status & JSX_TRACE)
        TRdisplay("%@ RFP: Restoring DUMP copy of config file.\n");

    status = dm0d_config_restore(dcb, (i4)DM0C_DUMP, ckp_sequence,
                                    &jsx->jsx_dberr);
    if (status != E_DB_OK)
    {
        uleFormat(&jsx->jsx_dberr, 0, &sys_err, ULE_LOG, NULL,
            (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
        uleFormat(NULL, E_DM133B_RFP_DUMP_CONFIG, &sys_err, ULE_LOG, NULL,
            (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	SETDBERR(&jsx->jsx_dberr, 0, E_DM1307_RFP_RESTORE);
        return (E_DB_ERROR);
    }

    /*  Now open the newly restored configuration file. We don't actually
    **  plan to use this config file in the end -- we will restore another
    **  copy from the save-set and then re-restore the dump directory version.
    **  we have restored it here solely in order to read it and thus build
    **  the 'dcb' and 'ext' information about the locations which were
    **  checkpointed.
    */

    status = config_handler(jsx, UNKEEP_CONFIG, dcb, &cnf);
    if (status == E_DB_OK)
        status = config_handler(jsx, OPEN_CONFIG, dcb, &cnf);
    if (status == E_DB_OK)
        status = config_handler(jsx, CLOSEKEEP_CONFIG, dcb, &cnf);
    if (status != E_DB_OK)
        return (status);


    if (jsx->jsx_status & JSX_TRACE)
        TRdisplay(
      "%@ RFP: Restoring checkpoint of location: %t\n%32* from file: '%t%t'.\n",
            sizeof(dcb->dcb_root->logical), &dcb->dcb_root->logical,
            dcb->dcb_ckp->phys_length, &dcb->dcb_ckp->physical,
            sizeof(filename), &filename, &sys_err);

    /*
    ** Call the checkpoint starting routine
    */
    status = dmckp_begin_restore_db( d, dcb, &error );
    if ( status != E_DB_OK )
    {
	SETDBERR(&jsx->jsx_dberr, 0, error);
        return( status );
    }

    /*
    ** Delete all the files at the root database location
    */
    TRformat( dmf_put_line, 0, line_buffer, sizeof(line_buffer),
              "%@ RFP: Deleting files at root location : %~t:\n",
              sizeof(dcb->dcb_root->logical), &dcb->dcb_root->logical);

    status = DIlistfile(&di_file, dcb->dcb_root->physical.name,
        dcb->dcb_root->phys_length, delete_files, (PTR)dcb->dcb_root,
        &sys_err);
    if (status != DI_ENDFILE)
    {
	SETDBERR(&jsx->jsx_dberr, 0, E_DM130C_RFP_DELETE_ALL);
        return (E_DB_ERROR);
    }

    /*
    ** Restore the ROOT database location, so setup the CK_CB info which
    ** are the same for all device types
    */
    *(dcb->dcb_root->physical.name + dcb->dcb_root->phys_length) = '\0';
    d->dmckp_di_path   = dcb->dcb_root->physical.name;
    d->dmckp_di_l_path = dcb->dcb_root->phys_length;

    /*
    **  Get a free device
    */
    status = rfp_wait_free(jsx, d);
    if (status != OK)
    {
	uleFormat(&jsx->jsx_dberr, E_DM1307_RFP_RESTORE, &sys_err,
		   ULE_LOG, NULL, NULL, 0, NULL, &error, 0);
	return (E_DB_ERROR);
    }
    if (jsx->jsx_status & JSX_DEVICE)
    {
	TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
		"%@ RFP: Start restore of location: %~t from tape:\n",
		sizeof(dcb->dcb_root->logical), &dcb->dcb_root->logical);
	TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
		"%4* device = '%t'\n%4* file = '%~t'\n",
		jsx->jsx_device_list[0].l_device,
                jsx->jsx_device_list[0].dev_name,
		sizeof(filename), &filename);
    }
    else
    {
	/* Use the present checkpoint location - it may have moved */
	d->dmckp_ckp_path   = rfp->cur_ckp_loc.physical.name;
	d->dmckp_ckp_l_path = rfp->cur_ckp_loc.phys_length; 
	TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
		"%@ RFP: Start restore of location: %~t from disk:\n",
		sizeof(dcb->dcb_root->logical), &dcb->dcb_root->logical);
	TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
		"%4* path = '%t'\n%4* file = '%~t'\n",
		d->dmckp_ckp_l_path, d->dmckp_ckp_path,
		sizeof(filename), &filename);
    }

    /*
    ** The following messages have been moved into the checkpoint
    ** template files. In 6.5 and earlier releases they were not,
    ** for backward compatibilty if the user is running using old style
    ** templates then issue the message
    */
    if ( d->dmckp_use_65_tmpl )
    {
        TRformat( dmf_put_line, 0, line_buffer, sizeof(line_buffer),
                 "%@ RFP: Restoring checkpoint of location: %~t:\n",
                  sizeof(dcb->dcb_root->logical), &dcb->dcb_root->logical);

        if (jsx->jsx_status & JSX_DEVICE)
        {
            TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
                "%4* tape device = '%t'\n%4* file = '%~t'\n",
                jsx->jsx_device_list[0].l_device,
		jsx->jsx_device_list[0].dev_name,
		sizeof(filename), &filename);
        }
        else
        {
            TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
                "%4* path = '%t'\n%4* file = '%~t'\n",
                dcb->dcb_ckp->phys_length, &dcb->dcb_ckp->physical,
                sizeof(filename), &filename);
        }
    }

    /*
    ** Restore the root database location
    */
    status = dmckp_pre_restore_loc( d, dcb, &error );
    if (status != OK)
    {
	SETDBERR(&jsx->jsx_dberr, 0, E_DM1307_RFP_RESTORE);
	return (E_DB_ERROR);
    }
    status = dmckp_restore_loc( d, dcb, &error );


#ifdef xDEBUG
        /*
        ** DM1317 simulates CKrestore error.
        */
        if (DMZ_ASY_MACRO(17))
            status = CK_ENDFILE;
#endif

    if ( status != E_DB_OK )
    {
	SETDBERR(&jsx->jsx_dberr, 0, E_DM1307_RFP_RESTORE);
        return( status );
    }


    /*
    ** Restore the remaining locations.
    */

    ext = dcb->dcb_ext;
    for (i = 0; i < ext->ext_count; i++)
    {
        /*  Skip journal, dump, checkpoint, root and aliased locations. */
        if (ext->ext_entry[i].flags &
            (DCB_DUMP | DCB_JOURNAL | DCB_CHECKPOINT | DCB_ROOT |
                    DCB_ALIAS | DCB_WORK | DCB_AWORK))
            continue;

        /*
        ** Set up the directory path to the data location
        */
	/*
	**  Bug 67410 - clear out the name
	*/
	*(ext->ext_entry[i].physical.name + ext->ext_entry[i].phys_length) = '\0';
        d->dmckp_di_path   = ext->ext_entry[i].physical.name;
        d->dmckp_di_l_path = ext->ext_entry[i].phys_length;

        /*
        ** Delete all files.
        */
	if (!(ext->ext_entry[i].flags & DCB_RAW))
	    status = dmckp_delete_loc( d, &error );
	/*
	**  Bug 67410 -ignore the error if there aren't any files
	*/

        /*
        ** Construct filename for checkpoint.
        */
        sequence++;
        filename.name[5] = (sequence / 100) + '0';
        filename.name[6] = ((sequence / 10) % 10) + '0';
        filename.name[7] = (sequence % 10) + '0';

        if (jsx->jsx_status & JSX_TRACE)
            TRdisplay("%@ RFP: Restoring checkpoint of location: %t\n%32* from file: '%t%t'.\n",
                sizeof(ext->ext_entry[i].logical), &ext->ext_entry[i].logical,
                dcb->dcb_ckp->phys_length, &dcb->dcb_ckp->physical,
                sizeof(filename), &filename, &sys_err);

	/*
	**  Get a free device
	*/
	status = rfp_wait_free(jsx, d);
	if (status != OK)
        {
            uleFormat(&jsx->jsx_dberr, E_DM1307_RFP_RESTORE, &sys_err,
		       ULE_LOG, NULL, NULL, 0, NULL, &error, 0);
	    return (E_DB_ERROR);
	}
	if (jsx->jsx_status & JSX_DEVICE)
	{
	    TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
		"%@ RFP: Start restore of location: %~t from tape:\n",
                 sizeof(ext->ext_entry[i].logical), &ext->ext_entry[i].logical);
	    TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
		"%4* device = '%t'\n%4* file = '%~t'\n",
		d->dmckp_ckp_l_path, d->dmckp_ckp_path,
		sizeof(filename), &filename);
        }
        else
        {
	    /* Use the present checkpoint location - it may have moved */
	    d->dmckp_ckp_path   = rfp->cur_ckp_loc.physical.name;
	    d->dmckp_ckp_l_path = rfp->cur_ckp_loc.phys_length; 
	    TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
		"%@ RFP: Start restore of location: %~t from disk:\n",
		sizeof(dcb->dcb_root->logical), &ext->ext_entry[i].logical);
	    TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
		"%4* path = '%t'\n%4* file = '%~t'\n",
		d->dmckp_ckp_l_path, d->dmckp_ckp_path,
		sizeof(filename), &filename);
        }
        /*
        ** The following messages have been moved into the checkpoint
        ** template files. In 6.5 and earlier releases they were not,
        ** for backward compatibilty if the user is running using old style
        ** templates then issue the message
        */
        if ( d->dmckp_use_65_tmpl )
        {
            TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
                    "%@ RFP: Restoring checkpoint of location: %~t:\n",
                    sizeof(ext->ext_entry[i].logical),
                    &ext->ext_entry[i].logical);

            if (jsx->jsx_status & JSX_DEVICE)
            {
                TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
                    "%4* tape device = '%t'\n%4* file = '%~t'\n",
		    d->dmckp_ckp_l_path, d->dmckp_ckp_path,
                    sizeof(filename), &filename);
            }
            else
            {
                TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
                    "%4* path = '%t'\n%4* file = '%~t'\n",
                    dcb->dcb_ckp->phys_length, &dcb->dcb_ckp->physical,
                    sizeof(filename), &filename);
            }
        }

	d->dmckp_raw_start = ext->ext_entry[i].raw_start;
	d->dmckp_raw_blocks = ext->ext_entry[i].raw_blocks;
	d->dmckp_raw_total_blocks = ext->ext_entry[i].raw_total_blocks;

        /*
        ** Restore the location
        */
	status = dmckp_pre_restore_loc( d, dcb, &error );
	if ( status != E_DB_OK )
	{
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM1307_RFP_RESTORE);
	    return( status );
	}
	status = dmckp_restore_loc( d, dcb, &error );
	if ( status != E_DB_OK )
	{
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM1307_RFP_RESTORE);
	    return( status );
	}

    }

    /*
    ** bug 49057 - clear locations we have extended to after
    ** this checkpoint.
    */

    for (i=0; i < ext->ext_count; i++)
    {
	i4	j;

	if (ext->ext_entry[i].flags & DCB_DATA)
	{
	    for (j=0; j < rfp->rfp_db_loc_count; j++) 
	    {
		if (rfp->rfp_db_ext[j].ext_location.flags == 0)
		    continue;				   
		/* Change from STcompare() to MEcmp() since db_loc_name */
		/* is not null terminated string */
 
		if (MEcmp( (char *)&ext->ext_entry[i].logical.db_loc_name,
		   (char *)&rfp->rfp_db_ext[j].ext_location.logical.db_loc_name,
		   sizeof (DB_LOC_NAME) ) == 0)
		{
		    rfp->rfp_db_ext[j].ext_location.flags=0;
                    continue;
		}
                *(rfp->rfp_db_ext[j].ext_location.physical.name + rfp->rfp_db_ext[j].ext_location.phys_length) = '\0';
             /* found extended alias data location after this ckp */
                if (STcompare( ext->ext_entry[i].physical.name,
                   rfp->rfp_db_ext[j].ext_location.physical.name) == 0)
                {
                   rfp->rfp_db_ext[j].ext_location.flags = DCB_SKIP;
                }
            }
	}
    }
    for (i=0; i < rfp->rfp_db_loc_count; i++)
    {
	/* If we extend to an alias location which points to ii_database
	** after this ckp, we have to skip it since we don't want to
	** delete files we just restored under root data location.
	*/
 
	if (rfp->rfp_db_ext[i].ext_location.flags == 0 ||
	    (rfp->rfp_db_ext[i].ext_location.flags & DCB_SKIP))
	    continue ;

	*(rfp->rfp_db_ext[i].ext_location.physical.name + rfp->rfp_db_ext[i].ext_location.phys_length) = '\0';
        d->dmckp_di_path   = rfp->rfp_db_ext[i].ext_location.physical.name;
        d->dmckp_di_l_path = rfp->rfp_db_ext[i].ext_location.phys_length;

        /*
        ** Delete all files.
        */
        status = dmckp_delete_loc( d, &error );

    }


    /*
    ** If using multiple devices, wait for last device to finish.
    */
    status = rfp_wait_all(jsx);
    if (status != OK)
    {
	SETDBERR(&jsx->jsx_dberr, 0, E_DM1307_RFP_RESTORE);
        return (E_DB_ERROR);
    }

    /*
    ** Call the checkpoint end routine.
    */
    status = dmckp_end_restore_db( d, dcb, &error );
    if ( status != E_DB_OK )
    {
	SETDBERR(&jsx->jsx_dberr, 0, error);
        return( status );
    }

    /*
    **  The dmp location may have been changed
    **  Reflect the current location in the dcb so we can open it 
    */
    for (i = 0; i < ext->ext_count; i++)
    {
	if (ext->ext_entry[i].flags & DCB_DUMP)
	    STRUCT_ASSIGN_MACRO(rfp->cur_dmp_loc,
				    ext->ext_entry[i]);
    }

    /*
    ** Re-restore the DUMP version of the config file into the default
    ** location, replacing the one that came in when the checkpoint for
    ** that location was restored.
    */
    if (jsx->jsx_status & JSX_TRACE)
        TRdisplay("%@ RFP: Re-Restoring DUMP copy of config file.\n");

    status = dm0d_config_restore(dcb, (i4)DM0C_DUMP, ckp_sequence,
                                    &jsx->jsx_dberr);
    if (status != E_DB_OK)
    {
        uleFormat(&jsx->jsx_dberr, 0, &sys_err, ULE_LOG, NULL,
            (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
        uleFormat(NULL, E_DM133B_RFP_DUMP_CONFIG, &sys_err, ULE_LOG, NULL,
            (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	SETDBERR(&jsx->jsx_dberr, 0, E_DM1307_RFP_RESTORE);
        return (E_DB_ERROR);
    }

    /*  Now open the newly restored configuration file. */

    status = config_handler(jsx, UNKEEP_CONFIG, dcb, &cnf);
    if (status == E_DB_OK)
        status = config_handler(jsx, OPEN_CONFIG, dcb, &cnf);
    if (status == E_DB_OK)
    {
        /*
        ** Don't need the dm0l_jon status any more because we disallow
        ** dynamically turn on/off journaling during online checkpoint.

        if (cnf->cnf_rel->tab_relation.relstat & TCB_JOURNAL)
            rfp->do_jnl = 1;
        else
            rfp->do_jnl = 0;

        */
        /* Update the journal history from current config file.*/

        STRUCT_ASSIGN_MACRO(rfp->old_jnl, *cnf->cnf_jnl);
        STRUCT_ASSIGN_MACRO(rfp->old_dmp, *cnf->cnf_dmp);

        /*
        ** must reset db open count to zero, otherwise, the server
        ** won't be able to access it if the count is non zero. As it turns
        ** out, this config file has just been restored from the dump
        ** directory, so it should have an open count of 0 already since the
        ** open_count is forced to 0 whenever we make a copy of the file.
        ** However, it is safe to forcibly set it here and it avoids any
        ** problems with rolling forward from old checkpoints (dm0c_close
        ** was only recently updated to set the open count to 0).
        */

        cnf->cnf_dsc->dsc_open_count = 0;

	/*  Bug B67276
	**  The ckp, dmp, or jnl locations may have been changed
	**  Reflect the current locations in the dcb and config file
	*/
	for (i = 0; i < cnf->cnf_dsc->dsc_ext_count; i++)
	{
	    if (cnf->cnf_ext[i].ext_location.flags & DCB_CHECKPOINT)
		STRUCT_ASSIGN_MACRO(rfp->cur_ckp_loc,
				    cnf->cnf_ext[i].ext_location);
	    else if (cnf->cnf_ext[i].ext_location.flags & DCB_DUMP)
		STRUCT_ASSIGN_MACRO(rfp->cur_dmp_loc,
				    cnf->cnf_ext[i].ext_location);
	    else if (cnf->cnf_ext[i].ext_location.flags & DCB_JOURNAL)
		STRUCT_ASSIGN_MACRO(rfp->cur_jnl_loc, 
				    cnf->cnf_ext[i].ext_location);
	}

	for (i = 0; i < ext->ext_count; i++)
	{
	    if (ext->ext_entry[i].flags & DCB_CHECKPOINT)
		STRUCT_ASSIGN_MACRO(rfp->cur_ckp_loc,
				    ext->ext_entry[i]);
	    else if (ext->ext_entry[i].flags & DCB_DUMP)
		STRUCT_ASSIGN_MACRO(rfp->cur_dmp_loc,
				    ext->ext_entry[i]);
	    else if (ext->ext_entry[i].flags & DCB_JOURNAL)
		STRUCT_ASSIGN_MACRO(rfp->cur_jnl_loc, 
				    ext->ext_entry[i]);
	}

        status = config_handler(jsx, UPDATE_CONFIG, dcb, &cnf);
        status = config_handler(jsx, CLOSEKEEP_CONFIG, dcb, &cnf);
    }

    /*  Delete default location alias file. */

    status = DIdelete(&di_file, (char *)&dcb->dcb_root->physical,
        dcb->dcb_root->phys_length, (char *)&alias_file,
        sizeof(alias_file), &sys_err);
    if (status != OK)
    {
        if (status == DI_FILENOTFOUND)
        {
           /*
            **  we need to prevent access to this database as a result
            **  of this failure.  Mark inconsistent with meaningful
            **  reason.
            */

            status = config_handler(jsx, UNKEEP_CONFIG, dcb, &cnf);
            if (status == E_DB_OK)
                status = config_handler(jsx, OPEN_CONFIG, dcb, &cnf);
            if (status == E_DB_OK)
            {
                cnf->cnf_dsc->dsc_status &= ~DSC_VALID;
                cnf->cnf_dsc->dsc_inconst_time = TMsecs();
                cnf->cnf_dsc->dsc_inconst_code = DSC_RFP_FAIL;
                status = config_handler(jsx, UPDATE_CONFIG, dcb, &cnf);
                status = config_handler(jsx, CLOSEKEEP_CONFIG, dcb, &cnf);
            }

	    SETDBERR(&jsx->jsx_dberr, 0, E_DM130D_RFP_BAD_RESTORE);
            return (E_DB_ERROR);
        }
        uleFormat(NULL, E_DM9003_BAD_FILE_DELETE, &sys_err,
            ULE_LOG, NULL, NULL, 0, NULL, &error, 4,
            sizeof(dcb->dcb_name), dcb->dcb_name.db_db_name,
            4, "None",
            dcb->dcb_root->phys_length, &dcb->dcb_root->physical,
            sizeof(alias_file), &alias_file);
	SETDBERR(&jsx->jsx_dberr, 0, E_DM130E_RFP_ALIAS_DELETE);
        return (E_DB_ERROR);
    }

    /*  Delete alias files from the rest of the locations.  */

    for (i = 0; i < ext->ext_count; i++)
    {
        /*  Skip journal, dump, checkpoint, root and aliased locations. */
        if (ext->ext_entry[i].flags &
            (DCB_RAW | DCB_DUMP | DCB_JOURNAL | DCB_CHECKPOINT | DCB_ROOT |
                    DCB_ALIAS | DCB_WORK | DCB_AWORK))
            continue;

        /*  Delete alias file. */

        status = DIdelete(&di_file, (char *)&ext->ext_entry[i].physical,
            ext->ext_entry[i].phys_length, (char *)&alias_file,
            sizeof(alias_file), &sys_err);
        if (status != OK)
        {
            if (status == DI_FILENOTFOUND)
            {
                status = config_handler(jsx, UNKEEP_CONFIG, dcb, &cnf);
                if (status == E_DB_OK)
                    status = config_handler(jsx, OPEN_CONFIG, dcb, &cnf);
                if (status == E_DB_OK)
                {
                    cnf->cnf_dsc->dsc_status &= ~DSC_VALID;
                    cnf->cnf_dsc->dsc_inconst_time = TMsecs();
                    cnf->cnf_dsc->dsc_inconst_code = DSC_RFP_FAIL;
                    status = config_handler(jsx, UPDATE_CONFIG, dcb, &cnf);
                    status = config_handler(jsx, CLOSEKEEP_CONFIG, dcb, &cnf);
                }

		SETDBERR(&jsx->jsx_dberr, 0, E_DM130D_RFP_BAD_RESTORE);
                return (E_DB_ERROR);
            }
            uleFormat(NULL, E_DM9003_BAD_FILE_DELETE, &sys_err,
                ULE_LOG, NULL, NULL, 0, NULL, &error, 4,
                sizeof(dcb->dcb_name), dcb->dcb_name.db_db_name,
                4, "None",
                ext->ext_entry[i].phys_length, &ext->ext_entry[i].physical,
                sizeof(alias_file), &alias_file);
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM130E_RFP_ALIAS_DELETE);
            return (E_DB_ERROR);
        }
    }

    /*  Handle stopping the roll forward at this point. */
    if ((jsx->jsx_status & JSX_USEJNL) == 0 && (jsx->jsx_status & JSX_CKPXL))
    {
        status = config_handler(jsx, OPEN_CONFIG, dcb, &cnf);
        if (status == E_DB_OK)
        {
            cnf->cnf_dsc->dsc_status &= ~DSC_CKP_INPROGRESS;
            cnf->cnf_dsc->dsc_status |= DSC_ROLL_FORWARD;
            status = config_handler(jsx, UPDATE_CONFIG, dcb, &cnf);
            if (status == E_DB_OK)
                status = config_handler(jsx, CLOSE_CONFIG, dcb, &cnf);
        }
        if (status != E_DB_OK)
            return (status);
    }

    /*  Finished restoring checkpoints. */

    return (E_DB_OK);
}

/*{
** Name: db_post_ckp_config - Fix up config file ckp, dmp, and jnl history.
**
** Description:
**
**	This routine patches the database config file to restore the
**	checkpoint, dump, and journal history.  This is required since the
**	config file restored by the checkpoint is the OLD one - which has
**	old journal information that was valid back when the checkpoint was
**	taken.
**
**	Also restores additional data locations added since checkpoint,
**	so that these remain usable. 
**
**	Config file information restored by this routine:
**
**	    The database journal state 		: dsc_status & DSC_JOURNAL
**						: dsc_status & DSC_DISABLE_JOURN
**	    The database dump state 		: dsc_status & DSC_DUMP
**
**	    The last checkpoint number		: jnl_ckp_seq
**	    The current journal number		: jnl_fil_seq
**	    The EOF of the current journal file	: jnl_blk_seq
**	    The size of the journal history	: jnl_count
**	    The journal history contents	: jnl_history[]
**	    The last dump file number		: dmp_fil_seq
**	    The EOF of the last dump file	: dmp_blk_seq
**	    The size of the dump history	: dmp_count
**	    The dump history contents		: dmp_history[]
**	    The cluster journal file info	: jnl_node_info[]
**
**	    Any additional extents added	:
** Inputs:
**	rfp				Rollforward context.
**	jsx				Pointer to Journal Support info.
**      dcb				Pointer to DCB for this database.
**
** Outputs:
**      err_code                        Reason for error return status.
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
**      21-jun-1993 (rogerk)
**          Created as part of Reduced Logging Project.  We must now bring
**	    the config file up to date before finishing rollforward processing
**	    since we may have to open the database and resolve open xacts
**	    which may require archiver processing.  The config file jnl history
**	    must be up to date when archive actions are taken.
**	    Added jnl_la and dmp_la to the list of values restored from the
**	    original config file.
**	    Also added restoration of the DSC_DISABLE_JOURNAL status from
**	    the orginal config file to prevent a rollforward attempt from
**	    turning journaling back on.
**      22-nov-1994 (jnash)
**          B57056.  Set or reset DSC_DISABLE_JOURNAL in accordance with
**	    the original db_status.  The version in the dump directory may be
**	    wrong.
**	31-jan-1994 (jnash)
**	    Add last_tbl_id param, used to restore the config file
**	    dsc_last_table_id from the old config file version.
**	14-sep-1994 (andyw)
**	    Partial Backup/Recovery Project:
**	   	Rename to db_post_ckp_config and move
**		last_tbl_id & db_status into the rfp
**	5-march-1997(angusm)
**	    Touch up config file with data locations extended
**	    since this checkpoint: bug 49057
**	09-Jul-1997 (shero03)
**	    Fixed a bug that TPC-C testing uncovered when extending 
**	    by 24 locations.
**	21-oct-1999 (hayke02)
**	    17-Aug-1998(wanya01)
**	    bug 87552: set the location which we skipped in db_restore_ckp()
**	    to be a valid data location. This is because locations extended
**	    after ckp should not be deleted during rollforwarddb.
**	    10-Nov-1998 (wanya01)
**	    Change from STcompare() to MEcmp() for location name comparison.
*/
static DB_STATUS
db_post_ckp_config(
DMF_RFP		    *rfp,
DMF_JSX		    *jsx,
DMP_DCB		    *dcb)
{
    DM0C_CNF		*cnf = 0;
    DB_STATUS		status;
    i4			i;
    i4		local_error;
    i4		added_locs;
    i4		size = 0;
    bool	jnl_processing;
    bool	ckp_processing;
    i4			error;
    DB_ERROR		local_dberr;

    CLRDBERR(&jsx->jsx_dberr);

    if (jsx->jsx_status & JSX_TRACE)
	TRdisplay("%@ RFP: Restore config file contents\n");


    for (;;)
    {
	status = config_handler(jsx, OPEN_CONFIG, dcb, &cnf);
	if (status != E_DB_OK)
	    break;
    
	/* Add space for locations added since last ckp */
	/* We have already compared old vs new data     */
	/* locations in db_restore_ckp(), but recheck anyway */
	added_locs = rfp->rfp_db_loc_count;
	for (i=0; i < cnf->cnf_dsc->dsc_ext_count; i++)
	{
		i4	j;

		if (cnf->cnf_ext[i].ext_location.flags & DCB_ROOT)
			continue;
		if (cnf->cnf_ext[i].ext_location.flags & DCB_DATA)
		{
		    for (j=0; j < rfp->rfp_db_loc_count; j++) 
		    {
			if (rfp->rfp_db_ext[j].ext_location.flags == 0)
				continue;

			/* Change from STcompare() to MEcmp() since */
			/* db_loc_name is not null terminated string */
			if (MEcmp( (char *)&cnf->cnf_ext[i].ext_location.logical.db_loc_name,
			    (char *)&rfp->rfp_db_ext[j].ext_location.logical.db_loc_name,
			    sizeof (DB_LOC_NAME) ) == 0)
			{
		    	    rfp->rfp_db_ext[j].ext_location.flags=0;
			    added_locs--;
			}
		    }
		} 
	}
        if (added_locs > 0)
	    size = (added_locs + 1) * sizeof(DM0C_EXT);
        while (cnf->cnf_free_bytes < size)
        {
	    status = dm0c_extend(cnf, &jsx->jsx_dberr);
	    if (status != E_DB_OK)
	    {
		SETDBERR(&jsx->jsx_dberr, 0, E_DM0071_LOCATIONS_TOO_MANY);
	        break;
	    }
        }

	if (jsx->jsx_status & JSX_TRACE)
	{
	    TRdisplay("%@ db_post_ckp_config: Open Config file (1)\n");
	    dump_cnf(cnf);
	}

	/*
	** Update the configuration file database state info to reflect
	** the uptodate versions of dump and journal status.
	*/
	cnf->cnf_dsc->dsc_status &= ~(DSC_JOURNAL | DSC_DUMP);
	if (rfp->db_status & DSC_JOURNAL)
	    cnf->cnf_dsc->dsc_status |= DSC_JOURNAL;
	if (rfp->db_status & DSC_DUMP)
	    cnf->cnf_dsc->dsc_status |= DSC_DUMP;

	if (rfp->db_status & DSC_DISABLE_JOURNAL)
	    cnf->cnf_dsc->dsc_status |= DSC_DISABLE_JOURNAL;
	else
	    cnf->cnf_dsc->dsc_status &= ~DSC_DISABLE_JOURNAL;

	/*
	** The config file dsc_last_table_id field is updated in dmve_create,
	** but this will not occur if the last table created prior to
	** the rollforward is non-journaled, or is associated with a
	** dm2u_get_tabid call made to simply create a unique number.
	** So rollforward updates the field with the version from the
	** original config file.
	*/
	if ((rfp->last_tbl_id < cnf->cnf_dsc->dsc_last_table_id) &&
	   ((jsx->jsx_status & JSX_TBL_LIST) == 0))
	{
	    /*
	    ** We don't expect to encounter the situation where the
	    ** orginal last table id is less the than the one recovered.
	    ** We log this condition, but otherwise do nothing about it.
	    */
	    uleFormat(NULL, E_DM134E_RFP_LAST_TABID, (CL_ERR_DESC *)NULL, ULE_LOG,
		NULL, (char *)NULL, (i4)0, (i4 *)NULL, &local_error, 3,
		sizeof(DB_DB_NAME), dcb->dcb_name.db_db_name,
		0, rfp->last_tbl_id,
		0, cnf->cnf_dsc->dsc_last_table_id);
	}
	if ((jsx->jsx_status & JSX_TBL_LIST) == 0)
	    cnf->cnf_dsc->dsc_last_table_id = rfp->last_tbl_id;

	/*
	** Restore journal history to its current state.
	*/
	cnf->cnf_jnl->jnl_la = rfp->jnl_la;
	cnf->cnf_jnl->jnl_ckp_seq = rfp->ckp_jnl_sequence;
	cnf->cnf_jnl->jnl_count = rfp->jnl_count;
	cnf->cnf_jnl->jnl_fil_seq = rfp->jnl_fil_seq;
	cnf->cnf_jnl->jnl_blk_seq = rfp->jnl_blk_seq;
	for (i = 0; i < CKP_HISTORY; i++)
	{
	    STRUCT_ASSIGN_MACRO(rfp->rfp_jnl_history[i],
						cnf->cnf_jnl->jnl_history[i]);
	}

	ckp_processing = ((jsx->jsx_status & JSX_USECKP) != 0);
	jnl_processing = ( ((jsx->jsx_status & JSX_USEJNL ) !=0) &&
			( ((rfp->db_status & DSC_JOURNAL) != 0) ||
			((rfp->db_status & DSC_DISABLE_JOURNAL)!=0)));

	/*
	** Restore dump history to its correct state.
	*/
	cnf->cnf_dmp->dmp_la = rfp->dmp_la;
	cnf->cnf_dmp->dmp_ckp_seq = rfp->ckp_dmp_sequence;
	cnf->cnf_dmp->dmp_count = rfp->dmp_count;
	cnf->cnf_dmp->dmp_fil_seq = rfp->dmp_fil_seq;
	cnf->cnf_dmp->dmp_blk_seq = rfp->dmp_blk_seq;
	for (i = 0; i < CKP_HISTORY; i++)
	{
	    STRUCT_ASSIGN_MACRO(rfp->rfp_dmp_history[i],
						cnf->cnf_dmp->dmp_history[i]);
	}

	/*
	** Restore the cluster node history.  Since we are executing
	** a rollforward, we must have merged any necessary journal/dump files
	** so we can now assume that the cluster history should be empty.
	** (is this right ???? - roger XXXX)
	** What do I do with the cnode_la????
	*/
	if (CXcluster_enabled())
	{
	    for ( i = 0; i < DM_CNODE_MAX; i++)
	    {
		cnf->cnf_jnl->jnl_node_info[i].cnode_fil_seq = 0;
		cnf->cnf_jnl->jnl_node_info[i].cnode_blk_seq = 0;

		cnf->cnf_dmp->dmp_node_info[i].cnode_fil_seq = 0;
		cnf->cnf_dmp->dmp_node_info[i].cnode_blk_seq = 0;
	    }
	}

	/*
	** update with current data locations
	*/

	/*
	** Assume:  0   : all the same
	**         >0   : more to add
	**         <0   : db at time of ckp had more locations than now
	**		  ?impossible?
	*/
	if (added_locs > 0)
	{
		for (i=0; i < rfp->rfp_db_loc_count; i++) 
		{
		    if (rfp->rfp_db_ext[i].ext_location.flags == 0)
			continue;

		    /* Here we need to set the location which we skipped
		    ** earlier in db_restore_ckp() to be a valid data location
		    ** Locations extended after ckp should not be deleted
		    ** after rollforwarddb
		    */

		    if (rfp->rfp_db_ext[i].ext_location.flags & DCB_SKIP)
			rfp->rfp_db_ext[i].ext_location.flags = DCB_DATA;
 
		    MEcopy((PTR)&rfp->rfp_db_ext[i], sizeof(DM0C_EXT),(PTR)&cnf->cnf_ext[cnf->cnf_dsc->dsc_ext_count] );
		    cnf->cnf_dsc->dsc_ext_count++;
		    cnf->cnf_free_bytes -= sizeof(DM0C_EXT);
		}
		cnf->cnf_c_ext = cnf->cnf_dsc->dsc_ext_count;
		cnf->cnf_ext[cnf->cnf_dsc->dsc_ext_count+1].length=0;
		cnf->cnf_ext[cnf->cnf_dsc->dsc_ext_count+1].type=0;
	}

	/*
	** RFP +c -j -incremental,  clear jnl history for specified checkpoint
	** (currently always from last checkpoint)
	*/
	if (ckp_processing && !jnl_processing && (jsx->jsx_status2 & JSX2_INCR_JNL) )
	{
	    i4	ckpjnl = rfp->jnl_first;
	    i4  ckpseq = cnf->cnf_jnl->jnl_history[ckpjnl].ckp_sequence;

	    if (jsx->jsx_status & JSX_TRACE)
		TRdisplay("%@ RFP Incremental ckp %d jnl %d (%d,%d)\n", 
		ckpseq, ckpjnl,
		cnf->cnf_jnl->jnl_history[ckpjnl].ckp_f_jnl,
		cnf->cnf_jnl->jnl_history[ckpjnl].ckp_l_jnl);

	    /*
	    ** Reset jnl_fil_seq to last jnl applied
	    ** Clear jnl history for this checkpoint
	    */
	    if (cnf->cnf_jnl->jnl_history[ckpjnl].ckp_f_jnl)
	    {
		cnf->cnf_jnl->jnl_fil_seq = 
			cnf->cnf_jnl->jnl_history[ckpjnl].ckp_f_jnl-1;
		cnf->cnf_jnl->jnl_blk_seq = 0;
		cnf->cnf_jnl->jnl_history[ckpjnl].ckp_f_jnl = 0;
		cnf->cnf_jnl->jnl_history[ckpjnl].ckp_l_jnl = 0;
	    }
	}

	/*
	** Rewrite the configuration file. At this point we also make
	** an up-to-date backup copy of the config file for recovery.
	*/
	if (jsx->jsx_status & JSX_TRACE)
	{
	    TRdisplay("%@ db_post_ckp_config: Updated Config file (2)\n");
	    dump_cnf(cnf);
	}

	status = config_handler(jsx, UPDTCOPY_CONFIG, dcb, &cnf);
	if (status != E_DB_OK)
	    break;

	status = config_handler(jsx, CLOSEKEEP_CONFIG, dcb, &cnf);
	if (status != E_DB_OK)
	    break;

	return (E_DB_OK);
    }

    /*
    ** Make sure we leave with the config file closed.
    */
    local_dberr = jsx->jsx_dberr;
    status = config_handler(jsx, CLOSEKEEP_CONFIG, dcb, &cnf);
    if (status != E_DB_OK)
    {
	uleFormat(&jsx->jsx_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
	    (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
    }
    jsx->jsx_dberr = local_dberr;

    uleFormat(&jsx->jsx_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
	(char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
    SETDBERR(&jsx->jsx_dberr, 0, E_DM1313_RFP_CKP_HISTORY);
    return (E_DB_ERROR);
}

/*{
** Name: process_record	- Qualify the journal record.
**
** Description:
**      This routine qualifies the record and passes the record to apply record.
**
**	The routine is also responsible for tracking transaction completion
**	states for use in undo processing.  As the database is rolled forward,
**	transaction state is built for each BT record and discarded on each
**	ET record.  At the end of the rollforward process, this state will
**	describe the list of all incomplete transactions which need to be
**	rolled back to bring the database to a consistent state.
**
** Inputs:
**      rfp                             Pointer to rollforward context.
**	dmve				Pointer to recovery context.
**      record                          Pointer to the record.
**
** Outputs:
**      err_code                        Reason for error code.
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
**      17-nov-1986 (derek)
**          Created for Jupiter.
**	17-nov-1989 (rogerk)
**	    Don't give error message for unexpected log record when find
**	    solitary SBACKUP or EBACKUP log record.
**	9-jul-1990 (bryanp)
**	    Return E_DB_ERROR if apply_j_record fails.
**	15-mar-1993 (rogerk)
**	    Reduced Logging - Phase IV:
**	    Added support for UNDO phase at end of rollforward.
**	    Moved -e/-b flag qualifications up to read_journals routine.
**	26-jul-1993 (rogerk)
**	  - Added support for databases which have been previously been marked
**	    inconsistent and are now being rolled forward to correct that
**	    event.  When a database cannot be recovered, any open transactions
**	    are terminated by writing an ET record with an INCONSIST flag.
**	    The transactions have not actually been rolled back so when we
**	    rollforward the database we will rebuild the incomplete xacts.
**	    Rollforward now ignores these INCONSIST ET records so that a true
**	    transaction rollback will be done; along with the writing of CLR
**	    records and a true ET.
**	  - Add handling for DMU operations.  Look for FRENAME operations
**	    which request the deletion of temporary or discarded files.  These
**	    files cannot be removed when the FRENAME record is actually
**	    processed since the file may be needed during a later rollback.
**	    Instead, we post a delete action to be processed when the ET
**	    record is encountered.
**	20-sep-1993 (rogerk)
**	    Added tx argument to apply_j_record so that tranid can be placed
**	    in the dmve control block for the dmve recover calls.  This was
**	    needed so that rolldb event actions could store the proper tranid.
**	21-feb-1994 (jnash)
**	    Add -start_lsn support.
[@history_template@]...
*/
static DB_STATUS
process_record(
DMF_RFP             *rfp,
DMVE_CB		    *dmve,
PTR		    record)
{
    DMF_JSX	    *jsx = rfp->rfp_jsx;
    RFP_TX	    *tx;
    DM0L_HEADER	    *h = (DM0L_HEADER *)record;
    DB_STATUS	    status;
    bool	    jnl_excp = FALSE;
    EX_CONTEXT	    context;
    i4		    error;

    CLRDBERR(&jsx->jsx_dberr);

    /*
    ** Declare exception handler.
    ** When an exception occurs, execution will continue here.
    */
    if (EXdeclare(ex_handler, &context) != OK)
    {
	EXdelete();
	SETDBERR(&jsx->jsx_dberr, 0, E_DM004A_INTERNAL_ERROR);
	return (E_DB_ERROR);
    }

    for ( ; ; )
    {
	/*	Find transaction information for this record. */

	status = E_DB_OK;
	lookup_tx(rfp, &h->tran_id, &tx);

	if (tx == 0)
	{
	    /*
	    ** If the transaction is not yet known to ROLLFORWARD, then make sure
	    ** that this is a BEGIN TRANSACTION record.  If not, then skip the
	    ** transaction and give a warning that we found an unexpected record.
	    **
	    ** If we are running with the -b flag or -start_lsn, then it may be
	    ** that the BT record for this transaction fell previous to the -b time
	    ** (or the starting lsn), so we simulate a BT being found.  We will
	    ** then proceed processing this transaction with this update.  The
	    ** user had better know what they are doing ... this is very
	    ** dangerous.  (Basically, they have to have done a rollforward
	    ** with -e #norollback specifying this timestamp just previously).
	    */

	    /*
	    ** Create TX entry for new transaction.
	    */
	    if (h->type == DM0LBT)
	    {
		status = add_tx(rfp, (DM0L_BT *)record); 
		if (status != E_DB_OK)
		{
		    dmfWriteMsg(&jsx->jsx_dberr, 0, 0);
		    break;
		}
	    }
	    else if ((rfp->rfp_jsx->jsx_status & JSX_BTIME) ||
		     (rfp->rfp_jsx->jsx_status1 & JSX_START_LSN))
	    {
		DM0L_BT	tmp_bt;

		MEfill(sizeof(DM0L_BT), '\0', &tmp_bt);
		STRUCT_ASSIGN_MACRO(h->tran_id, tmp_bt.bt_header.tran_id);
		STmove(DB_UNKNOWN_OWNER, ' ', sizeof(DB_OWN_NAME), (char *)&tmp_bt.bt_name);
		status = add_tx(rfp, &tmp_bt);
		if (status != E_DB_OK)
		{
		    dmfWriteMsg(&jsx->jsx_dberr, 0, 0);
		    break;
		}
	    }
	    else
	    {
		status = E_DB_OK;
		break;
	    }

	    lookup_tx(rfp, &h->tran_id, &tx);
	}

	tx->tx_process_cnt++;

	status = apply_j_record(rfp, tx, dmve, (DM0L_HEADER *)record);

	/*
	** Delete transaction information when ET is seen.
	**
	** Ignore DBINCONSIST End Transaction records - these are written when
	** undo processing for a transaction is halted due to a recovery failure.
	** We want these transactions to remain in the active tx list so that
	** rollback on them will be completed in the undo phase.
	*/
	if ((h->type == DM0LET) &&
	    ((((DM0L_ET *)h)->et_flag & DM0L_DBINCONST) == 0))
	{
	    remove_tx(rfp, tx);
	}

	if (rfp->test_excp_err &&
	    ((DM0L_HEADER *)record)->type == DM0LPUT &&
	    MEcmp(dmve->dmve_dcb_ptr->dcb_name.db_db_name, 
		    "test_rfp_dmve_error ", 20) == 0)
	{
	   i4 aa, bb;
	   aa = 5; bb = 0; aa = aa/bb;
	}

	break; /* ALWAYS */
    }

    EXdelete();
    return (status);
}

/*{
** Name: apply_j_record - Process journal record.
**
** Description:
**      This routine performs required actions needed to implement
**	each record.
**
** Inputs:
**	rfp				Pointer to rollforward context.
**      dmve				Recovery control block.
**      record                          Pointer to log record.
**
** Outputs:
**      err_code                        Reason for error return status.
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
**      21-nov-1986 (Derek)
**          Created for Jupiter.
**	22-jun-1987 (rogerk)
**	    Look for DM0LALTER log type.
**	 2-oct-1989 (rogerk)
**	    Handle DM0LFSWAP log records.
**	    Added new log record types to the trace output line.
**	9-jul-1990 (bryanp)
**	    Return DMVE error code to our caller if dmve_* routine fails.
**      16-jul-1991 (bryanp)
**          B38527: Add support for DM0L_OLD_MODIFY & DM0L_OLD_INDEX.
**          Also, support rollforward of DM0L_SM1_RENAME & DM0L_SM2_CONFIG.
**          Support rollforward of new record DM0L_SM0_CLOSEPURGE.
**	27-may-1992 (rogerk)
**	    Add check for DM0L_P1 (prepare to commit) log record during
**	    journal record processing.  While the prepare log records should
**	    not really be journaled, there existed a 6.3/6.4 bug that would
**	    sometimes cause them to be (B43916).  The P1 record was added to
**	    the list here of legal journal records to encounter during
**	    rollforward.  It is just ignored if found.
**      14-jul-1992 (andys)
**          Set status explicitly to E_DB_OK in DM0LP1 and DM0LALTER cases.
**	11-feb-1993 (jnash)
**	    Call dmd_format_log/dmd_format_dm_hdr to output verbose trace
**	    information, eliminate inline code.   Add new log records that
**	    are now journaled.  Changed error action when an unknown record
**	    type is encountered to continue rollforward processing after
**	    logging an error.
**	24-may-1993 (andys)
**	    Pass appropriate logfile blocksize to dmd_log and dmd_format_log
**	    rather than using hardwired 4096.
**	21-jun-1993 (rogerk)
**	    Added handling of Before Image CLR's in rollforward processing.
**	    While we never expect to rollforward normal Before Images (because
**	    they are undo-only), we do have to rollforward their CLR's.
**	    Removed unreferenced "nesting" variable, declared static.
**	23-aug-1993 (rogerk)
**	    Added DMU, ALTER log records to list of legal rollforward
**	    operations.
**	20-sep-1993 (rogerk)
**	    Added tx argument to apply_j_record so that tranid can be placed
**	    in the dmve control block for the dmve recover calls.  This was
**	    needed so that rolldb event actions could store the proper tranid.
**	18-oct-1993 (rmuth)
**	    Add journal time to the "we are alive" status message.
**	18-oct-93 (jrb)
**          Added support for DM0LEXTALTER.  (MLSorts)
**	14-sep-1994 (andyw)
**	    Partial Backup/Recovery Project:
**		Add support to the collection of statistics
**	20-sep-1996 (shero03)
**	    Support Rtree put/delete/replace
**	5-Apr-2004 (schka24)
**	    Support rawdata records for repartitioning modify.
**	08-jun-2004 (thaju02)
**	    Rolldb of online modify fails with E_DM93AF_BAD_PAGE_CNT.
**	    due to table file still open at file rename. (B112442)
**	29-jun-2004 (thaju02)
**	    Re-address of previous change which accidentally
**	    mucked up rolldb of regular modify.
**	30-jun-2004 (thaju02)
**	    Account for rolldb of online modify of the same
**	    table multiple times within the same chkpt.
**	    Make online context block unique on table id
**	    and bsf lsn.
**	05-aug-2004 (thaju02)
**	    Online modify sidefile processing fails. Subsequent
**	    attempt to rollforward fails with E_DM9C5D. 
**	    Close readnolock table. (B112789)
**	    At end xaction, cleanup any associated online
**	    control blocks. 
**	27-Jan-2005 (schka24)
**	    Oops, changes for online modify left rawdata call in the
**	    wrong case (oldmodify instead of modify) -- fix.
**	11-feb-2005 (thaju02)
**	    For online modify, if load is to be resumed, can not 
**	    delete rawdata just yet; to delete rawdata when we 
**	    close the input table. 
**      26-Jul-2007 (wanfr01)
**          Bug 118854
**          Stop duplicate logging of jnl records when #x and -verbose are
**          used together
**      23-Feb-2009 (hanal04) Bug 121652
**          Added DM0LUFMAP case to deal with FMAP updates needed for an
**          extend operation where a new FMAP will not reside in the last
**          FMAP or new FMAP itself.
[@history_template@]...
*/
static DB_STATUS
apply_j_record(
DMF_RFP             *rfp,
RFP_TX		    *tx,
DMVE_CB		    *dmve,
DM0L_HEADER	    *record)
{
    DMF_JSX	    *jsx = rfp->rfp_jsx;
    DB_STATUS	    status = E_DB_OK;
    DMF_RFP         *r = rfp;
    char	    line_buffer[132];
    char	    date[32];
    i4		    i;
    RFP_OCTX        *cand, *octx = 0;
    DB_TAB_ID	    *tab = NULL;
    i4		    error;

    CLRDBERR(&jsx->jsx_dberr);


    dmve->dmve_log_rec = (PTR)record;
    dmve->dmve_octx = (RFP_OCTX *)NULL;
    STRUCT_ASSIGN_MACRO(tx->tx_bt.bt_header.tran_id, dmve->dmve_tran_id);

    /* Copy rawdata info (if any) for this TX to dmve cb, in case this
    ** kind of log record is interested in it.
    */
    for (i = 0; i < DM_RAWD_TYPES; ++i)
    {
	dmve->dmve_rawdata[i] = NULL;
	if (tx->tx_rawdata[i] != NULL)
	    dmve->dmve_rawdata[i] = (PTR) tx->tx_rawdata[i]->misc_data;
	dmve->dmve_rawdata_size[i] = tx->tx_rawdata_size[i];
    };

    if (record->type == DM0LET)
    {
	/*
	** Give progress indication every so often.
	*/
	rfp->rfp_jsx->jsx_tx_total++;
	if ((rfp->rfp_jsx->jsx_tx_total == 100) ||
	    (rfp->rfp_jsx->jsx_tx_total == 500) ||
	    ((rfp->rfp_jsx->jsx_tx_total % 1000) == 0))
	{
	    TMstamp_str((TM_STAMP *)&((DM0L_ET *)record)->et_time, date);
	    TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
		"%@ RFP: - processed %d Xacts up to %s\n",
		rfp->rfp_jsx->jsx_tx_total, date);
	}
    }

    if (rfp->rfp_jsx->jsx_status & (JSX_VERBOSE | JSX_TRACE))
    {
	dmd_format_dm_hdr(dmf_put_line, record,
	    line_buffer, sizeof(line_buffer));
	dmd_format_log(FALSE, dmf_put_line, (char *)record,
	    rfp->jnl_block_size,
	    line_buffer, sizeof(line_buffer));
    }

    /*
    ** NOTE:
    **
    ** Spent alot of time agonising over how the following code should be
    ** structured. Used the following decisions
    ** a) Database level recovery should not encounter too large a
    **    a performance hit preparing to check if the record is needed.
    ** b) If adding a new log record then only have to go to one place
    **    to add a new log record.
    ** c) In future further restrictions on what records of interest may
    **    be added so the interface to the "is record of interest?" routine
    **    should be easily extended. I.e. Use the structure.
    */



    switch (record->type)
    {
    case DM0LBT:

        r->rfp_stats_blk.rfp_applied_recs++;
        r->rfp_stats_blk.rfp_bt++;
        return (E_DB_OK);

    case DM0LET:

        r->rfp_stats_blk.rfp_applied_recs++;
        r->rfp_stats_blk.rfp_et++;

	/*
	** if there were online modify context control blocks
	** associated with this xaction, remove and clean them up
	*/
	for (cand = rfp->rfp_octx; cand != NULL; )
	{
	    if ( !MEcmp(&cand->octx_bsf_tran_id, &record->tran_id,
			sizeof(DB_TRAN_ID)) )
	    {
		octx = cand;

		while (octx->octx_osrc_next)
		{
		    DM2U_OSRC_CB	*osrc = (DM2U_OSRC_CB *)octx->octx_osrc_next; 
		    DMP_RNL_ONLINE	*rnl = &(osrc->osrc_rnl_online); 
		    DMP_MISC		*osrc_misc_cb = osrc->osrc_misc_cb;

		    if (rnl->rnl_xlsn)
			dm0m_deallocate((DM_OBJECT **)&rnl->rnl_xlsn);
		    if (rnl->rnl_btree_xmap)
			dm0m_deallocate((DM_OBJECT **)&rnl->rnl_btree_xmap);

		    octx->octx_osrc_next = (PTR)osrc->osrc_next;

		    dm0m_deallocate((DM_OBJECT **)&osrc_misc_cb);
		}

		if (octx->octx_next != NULL)
		    octx->octx_next->octx_prev = octx->octx_prev;
		if (octx->octx_prev != NULL)
		    octx->octx_prev->octx_next = octx->octx_next;
		else
		    rfp->rfp_octx = octx->octx_next;

		rfp->rfp_octx_count--;
		cand = octx->octx_next;
		dm0m_deallocate((DM_OBJECT **)&octx);
	    }
	    else
		cand = cand->octx_next;
	}
        return (E_DB_OK);

    case DM0LASSOC:

        status = dmve_assoc(dmve);
        break;

    case DM0LDISASSOC:

        status = dmve_disassoc(dmve);
        break;

    case DM0LALLOC:

        r->rfp_stats_blk.rfp_applied_recs++;
        r->rfp_stats_blk.rfp_alloc++;
        status = dmve_alloc(dmve);
        break;

    case DM0LEXTEND:

        status = dmve_extend(dmve);
        break;

    case DM0LOVFL:

        r->rfp_stats_blk.rfp_applied_recs++;
        r->rfp_stats_blk.rfp_ovfl++;
        status = dmve_ovfl(dmve);
        break;

    case DM0LNOFULL:

        status = dmve_nofull(dmve);
        break;

    case DM0LFMAP:

        status = dmve_fmap(dmve);
        break;

    case DM0LUFMAP:

        status = dmve_ufmap(dmve);
        break;

    case DM0LPUT:

        r->rfp_stats_blk.rfp_applied_recs++;
        r->rfp_stats_blk.rfp_put++;
        status = dmve_put(dmve);
        break;

    case DM0LDEL:

        status = dmve_del(dmve);
        break;

    case DM0LBTPUTOLD:

	TRformat(dmf_diag_put_line, 0, line_buffer, sizeof(line_buffer),
	"WARNING journal file contains old format BTPUT log records\n");

	SETDBERR(&jsx->jsx_dberr, 0, E_DM1308_RFP_BAD_RECORD);
	status = E_DB_ERROR;
	break;

    case DM0LBTPUT:

        r->rfp_stats_blk.rfp_applied_recs++;
        r->rfp_stats_blk.rfp_btput++;
        status = dmve_btput(dmve);
        break;

    case DM0LBTDELOLD:

	TRformat(dmf_diag_put_line, 0, line_buffer, sizeof(line_buffer),
	"WARNING journal file contains old format BTDEL log records\n");

	SETDBERR(&jsx->jsx_dberr, 0, E_DM1308_RFP_BAD_RECORD);
	status = E_DB_ERROR;
	break;

    case DM0LBTDEL:

        r->rfp_stats_blk.rfp_applied_recs++;
        r->rfp_stats_blk.rfp_btdel++;
        status = dmve_btdel(dmve);
        break;

    case DM0LBTSPLIT:

        r->rfp_stats_blk.rfp_applied_recs++;
        r->rfp_stats_blk.rfp_btsplit++;
        status = dmve_btsplit(dmve);
        break;

    case DM0LBTOVFL:

        r->rfp_stats_blk.rfp_applied_recs++;
        r->rfp_stats_blk.rfp_btovfl++;
        status = dmve_btovfl(dmve);
        break;

    case DM0LBTFREE:

        r->rfp_stats_blk.rfp_applied_recs++;
        r->rfp_stats_blk.rfp_btfree++;
        status = dmve_btfree(dmve);
        break;

    case DM0LBTUPDOVFL:

        r->rfp_stats_blk.rfp_applied_recs++;
        r->rfp_stats_blk.rfp_btupdovfl++;
        status = dmve_btupdovfl(dmve);
        break;

    case DM0LDEALLOC:

        status = dmve_dealloc(dmve);
        break;

    case DM0LAI:

        status = dmve_aipage(dmve);
        break;

    case DM0LBI:

        if (record->flags & DM0L_CLR)
            status = dmve_bipage(dmve);
        break;

    case DM0LREP:

        status = dmve_rep(dmve);
        break;

    case DM0LCREATE:

        status = dmve_create(dmve);
        break;

    case DM0LCRVERIFY:
        status = dmve_crverify(dmve);
        break;

    case DM0LDESTROY:

        status = dmve_destroy(dmve);
        break;

    case DM0LINDEX:
    case DM0LOLDINDEX:
        status = dmve_index(dmve);
        break;

    case DM0LMODIFY:
    {
	DM0L_MODIFY	*mod_rec = (DM0L_MODIFY *)record;

	if (mod_rec->dum_flag & DM0L_ONLINE)
	{
	    for (cand = rfp->rfp_octx; cand != NULL; cand = cand->octx_next)
            {
		if ( !MEcmp(&mod_rec->dum_tbl_id, &cand->octx_bsf_tbl_id,
				sizeof(DB_TAB_ID)) &&
		    LSN_EQ(&mod_rec->dum_bsf_lsn, 
				&cand->octx_bsf_lsn))
                {
                    octx = cand;
                    break;
                }
            }
            if (!octx)
            {
                /* FIX ME - this isn't good. report something */
                status = E_DB_ERROR;
            }	
	    else
	    {
		status = rfp_osrc_close(rfp, octx);
	    }
	    delete_rawdata(tx);
	}
	else if (mod_rec->dum_flag & DM0L_START_ONLINE_MODIFY)
	{
	    /* 'do' start online modify */
	    /* locate the online modify context block (octx) */
	    for (cand = rfp->rfp_octx; cand != NULL; cand = cand->octx_next)
	    {
		if ( !MEcmp(&mod_rec->dum_rnl_tbl_id, &cand->octx_bsf_tbl_id,
				sizeof(DB_TAB_ID)) &&
		     LSN_EQ(&mod_rec->dum_bsf_lsn, 
				&cand->octx_bsf_lsn))
		{
		    octx = cand;
		    break;
		}
	    }
	    if (!octx)
	    {
		/* FIX ME - this isn't good. report something */
		status = E_DB_ERROR;
	    }
	    else
	    {
		if (record->flags & DM0L_CLR)
		{
		    /* if $online modify failed, close rnl tbl */
		    status = rfp_osrc_close(rfp, octx);
		}
		else
		{
		    dmve->dmve_octx = octx;

		    status = dmve_modify(dmve);

		    if ((status == E_DB_INFO) &&
		    	(dmve->dmve_error.err_code == E_DM9CB1_RNL_LSN_MISMATCH))
		    {
			DB_STATUS       local_status = E_DB_OK;

			local_status = rfp_add_lsn_waiter(rfp, octx);
			if (local_status)
			    status = local_status;
			else
			    status = E_DB_OK;
		    }
		    else
			delete_rawdata(tx);
		}
	    }
	}
	else  /* regular modify */
	{
	    status = dmve_modify(dmve);

	    /* Modify uses up rawdata objects, if any -- kill them */
	    delete_rawdata(tx);
	}

        break;
    }
	
    case DM0LOLDMODIFY:
        status = dmve_modify(dmve);
        break;

    case DM0LRELOCATE:

        status = dmve_relocate(dmve);
        break;

    case DM0LALTER:
        status = dmve_alter(dmve);
        break;

    case DM0LFCREATE:

        status = dmve_fcreate(dmve);
        break;

    case DM0LFRENAME:

        status = dmve_frename(dmve);
        break;

    case DM0LDMU:

        status = dmve_dmu(dmve);
        break;

    case DM0LLOCATION:

        status = dmve_location(dmve);
        break;

    case DM0LEXTALTER:

        status = dmve_ext_alter(dmve);
        break;

    case DM0LRTDEL:

        r->rfp_stats_blk.rfp_applied_recs++;
        r->rfp_stats_blk.rfp_rtdel++;
        status = dmve_rtdel(dmve);
        break;

    case DM0LRTPUT:

        r->rfp_stats_blk.rfp_applied_recs++;
        r->rfp_stats_blk.rfp_rtput++;
        status = dmve_rtput(dmve);
        break;

    case DM0LRTREP:

        r->rfp_stats_blk.rfp_applied_recs++;
        r->rfp_stats_blk.rfp_rtrep++;
        status = dmve_rtrep(dmve);
        break;

    case DM0LSM0CLOSEPURGE:

        status = dmve_sm0_closepurge(dmve);
        break;

    case DM0LSM1RENAME:

        status = dmve_sm1_rename(dmve);
        break;

    case DM0LSM2CONFIG:

        status = dmve_sm2_config(dmve);
        break;

    case DM0LRAWDATA:
	/* Rawdata records carry data, they don't actually do anything.
	** Gather up this rawdata piece and hold it in the TX, in case
	** a log record comes along that actually uses the data.
	** (Gather in the TX, not the DMVE_CB, because there could be
	** multiple concurrent transactions that all generated rawdata.
	** We're sharing a single dmve-cb among the various transactions.)
	*/
	status = gather_rawdata(jsx, tx, (DM0L_RAWDATA *) record);
	break;

    /*
    ** Log records which require no rollforward processing:
    */
    case DM0LP1:
    case DM0LBMINI:
    case DM0LEMINI:
    case DM0LSAVEPOINT:
    case DM0LABORTSAVE:
    case DM0LBSF:
    case DM0LRNLLSN:
    case DM0LESF:
    case DM0LBTINFO:
    case DM0LJNLSWITCH:

        status = E_DB_OK;
        break;

	case DM0LDELLOCATION:

        status = dmve_del_location(dmve);
        break;


    default:
        /*
        ** Unexpected log record.  Log an error, but continue processing.
        ** If the log record was some new record type unneeded for rollforward
        ** (but journaled anyway) and was not added to this list then no
        ** harm done.  If the record actually needs rollforward work, but
        ** whoever added the new log record went braindead and forgot to add
        ** rollforward support for it, then this error is bad and will likely
        ** lead to an inconsistent database.
        */
        TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
            "apply_j_rec: Unknown log record type: %d\n", record->type);
        uleFormat(NULL, E_DM1308_RFP_BAD_RECORD, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
            (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
        status = E_DB_OK;
    }

    /* rolldb online modify only */
    if ((status == E_DB_OK) && (rfp->rfp_lsn_wait_cnt))
    {
	(VOID) rfp_check_lsn_waiter(rfp, record);
	CLRDBERR(&jsx->jsx_dberr);
    }

    if (rfp->test_indx_err &&
	record->type == DM0LPUT &&
	MEcmp(dmve->dmve_dcb_ptr->dcb_name.db_db_name, 
		"test_rfp_dmve_error ", 20) == 0 &&
		((DM0L_PUT *)record)->put_tbl_id.db_tab_index != 0)

    {
	SETDBERR(&dmve->dmve_error, 0, E_DM9601_DMVE_BAD_PARAMETER);
	status = E_DB_ERROR;
    }

    if (rfp->test_redo_err &&
	record->type == DM0LPUT &&
	MEcmp(dmve->dmve_dcb_ptr->dcb_name.db_db_name, 
		"test_rfp_dmve_error ", 20) == 0)
    {
	SETDBERR(&dmve->dmve_error, 0, E_DM9601_DMVE_BAD_PARAMETER);
	status = E_DB_ERROR;
    }

    /*
    ** If one of the low-level DMVE routines failed, return that error
    ** up to our caller.
    */
    if (status != E_DB_OK && jsx->jsx_dberr.err_code != E_DM1308_RFP_BAD_RECORD)
        jsx->jsx_dberr = dmve->dmve_error;

    return (status);
}

/*{
** Name: apply_d_record - Process dump record.
**
** Description:
**      This routine performs processing of records in the dump file
**	during rollforward using an online checkpoint.
**
**	The actions in the dump file are rolled-back to restore the
**	database to its state proceeding the checkpoint.
**
**	Recovery of the database in this instance is dome almost exclusively
**	using PHYSICAL log records.  We use Before Images to back out all
**	updates including those to system catalogs and btree index pages.
**	All operations, including those performed in mini transactions
**	(btree splits for instance) are backed out.
**
**	The only non-physical log records applied are those for file
**	extending.
**
** Inputs:
**	rfp				Pointer to rollforward context.
**	jsx
**      dmve				Recovery control block.
**      record                          Pointer to log record.
**
** Outputs:
**      err_code                        Reason for error return status.
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
**      13-jan-1989 (Edhsu)
**          Created for Terminator.
**	 6-dec-1989 (rogerk)
**	    Changed dump processing to use physical rather than logical
**	    undo for btree and system catalogs.
**	9-jul-1990 (bryanp)
**	    Return DMVE status if one of the DMVE routines fails.
**      8-may-1991 (bryanp)
**          Add support for DM0LBPEXTEND record to online backup (b37428)
**	14-oct-1991 (bryanp)
**	    Removed DM0LBALLOC, DM0LCALLOC, and DM0LBPEXTEND as part of 64/65
**	    merge (extend project obsoleted these log records).
**	14-dec-1992 (rogerk)
**	    Reduced Logging Project:  Removed dmve_sbipage records.
**	21-jun-1993 (rogerk)
**	    Removed unreferenced "nesting" variable, declared static.
**	14-sep-1994 (andyw)
**	    Partial Backup/Recovery Project:
**		Add basic statistics & jsx structure to argument list
[@history_template@]...
*/
static DB_STATUS
apply_d_record(
DMF_RFP             *rfp,
DMF_JSX		    *jsx,
DMVE_CB		    *dmve,
DM0L_HEADER	    *record)
{
    DB_STATUS	    status;
    DMF_RFP	    *r = rfp;

    CLRDBERR(&jsx->jsx_dberr);

    status = E_DB_OK;

    dmve->dmve_log_rec = (PTR)record;

    /*  Process record by type. */

    switch (((DM0L_HEADER *)dmve->dmve_log_rec)->type)
    {
	case DM0LSBACKUP:
	case DM0LEBACKUP:

	    break;

	case DM0LBI:

            r->rfp_stats_blk.rfp_applied_dump_recs++;
	    status = dmve_bipage(dmve);
	    break;

	default:
	    status = E_DB_ERROR;
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM1308_RFP_BAD_RECORD);
    }

    /*
    ** If one of the low-level DMVE routines failed, return that error
    ** up to our caller.
    */
    if (status != E_DB_OK)
	jsx->jsx_dberr = dmve->dmve_error;

    return (status);
}

/*{
** Name: init_recovery	- Initialize for recovery.
**
** Description:
**      Initialize a DMVE control block for the reocvery.
**
** Inputs:
**      dcb                             Pointer to DCB for database.
**	dmve				Pointer to DMVE for database.
**
** Outputs:
**      None
**
**      Returns:
**          VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      21-nov-1986 (Derek)
**          Created for Jupiter.
**	02-feb-1989 (EdHsu)
**	    Online backup support.
**      14-may-1991 (bryanp)
**          Made this function VOID and removed err_code, to reduce lint
**          complaints about unused arguments, etc.
**	22-feb-1993 (rogerk & jnash)
**	    Initialize dmve_logging field.  Set to TRUE even though
**	    no log records are not normally written during rollforward.
**	    FALSE imdicates to bypass logging in those operations (UNDO)
**	    which currently believe they need logging.
**	20-sep-1993 (rogerk)
**	    Initialize dmve_rolldb_action list field.
**	    Add RFP argument and initialize rfp_dmve pointer.
**	23-feb-2004 (thaju02)
**	    Initialize dmve_flags.
[@history_template@]...
*/
static VOID
init_recovery(
DMF_RFP            *rfp,
DMP_DCB            *dcb,
DMVE_CB            *dmve,
i4		   action)
{

    /*	Initialize the DMVE control block. */

    dmve->dmve_next = 0;
    dmve->dmve_prev = 0;
    dmve->dmve_length = sizeof(DMVE_CB);
    dmve->dmve_type = DMVECB_CB;
    dmve->dmve_owner = 0;
    dmve->dmve_ascii_id = DMVE_ASCII_ID;
    dmve->dmve_action = action;
    dmve->dmve_dcb_ptr  = dcb;
    dmve->dmve_tran_id.db_high_tran = 0;
    dmve->dmve_tran_id.db_low_tran = 0;
    dmve->dmve_lk_id = dmf_svcb->svcb_lock_list;
    dmve->dmve_log_id = 0;
    dmve->dmve_db_lockmode = DM2D_X;
    dmve->dmve_logging = TRUE;
    dmve->dmve_rolldb_action = NULL;
    dmve->dmve_flags = 0;
    dmve->dmve_octx = (RFP_OCTX *)NULL;
    dmve->dmve_log_rec = NULL;
    CLRDBERR(&dmve->dmve_error);

    rfp->rfp_dmve = dmve;
    return;
}

/*{
** Name: config_handler	- Handle configuration file operations.
**
** Description:
**      This routine manages the opening/closing and updating of
**	the configuration file.
**
** Inputs:
**	operation			Config operation.
**	    UPDTCOPY_CONFIG		Update the config file and make backup
**      dcb                             Pointer to DCB for database.
**	cnf				Pointer to CNF pointer.
**
** Outputs:
*	err_code			Reason for error return.
**	Returns:
**	    E_DB_OK
**	    E_DB_INFO			If .rfc open and it doesn't exist.
**	    E_DB_ERROR
**	Exceptions:
**
**
** Side Effects:
**
**
** History:
**      09-mar-1987 (Derek)
**          Created for Jupiter.
**	02-feb_1989 (EdHsu)
**	    Online backup support.
**	17-may-90 (bryanp)
**	    Added UPDTCOPY_CONFIG argument to update config and make backup.
**	    Added 'default' to switch to catch programming errors.
**	20-sep-1993 (andys)
**	     Fix arguments to ule_format call.
[@history_template@]...
*/
static DB_STATUS
config_handler(
DMF_JSX		    *jsx,
i4             operation,
DMP_DCB		    *dcb,
DM0C_CNF	    **config)
{
    DM0C_CNF		*cnf = *config;
    i4		i;
    i4		open_flags = 0;
    DB_STATUS		status;
    i4			error;
    DB_ERROR		local_dberr;

    CLRDBERR(&jsx->jsx_dberr);

    switch (operation)
    {
    case OPENRFC_CONFIG:
    case OPEN_LOCK_CONFIG:
    case OPEN_CONFIG:
	if (operation == OPENRFC_CONFIG)
	    open_flags = DM0C_TRYRFC;
	else if (operation == OPEN_LOCK_CONFIG)
	    open_flags = DM0C_RFP_NEEDLOCK;

	/*  Open the configuration file. */

	*config = 0;
	status = dm0c_open(dcb, open_flags, dmf_svcb->svcb_lock_list,
                           config, &jsx->jsx_dberr);
	if (status == E_DB_OK)
	{
	    if (dcb->dcb_ext)
		return (E_DB_OK);

	    cnf = *config;

	    /*  Allocate and initialize the EXT. */

	    status = dm0m_allocate((i4)(sizeof(DMP_EXT) +
		cnf->cnf_dsc->dsc_ext_count * sizeof(DMP_LOC_ENTRY)),
		0, (i4)EXT_CB,	(i4)EXT_ASCII_ID, (char *)dcb,
		(DM_OBJECT **)&dcb->dcb_ext, &jsx->jsx_dberr);
	    if (status != E_DB_OK)
	    {
		uleFormat(&jsx->jsx_dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL, 
				&error, 0);
		SETDBERR(&jsx->jsx_dberr, 0, E_DM1304_RFP_MEM_ALLOC);
		break;
	    }

	    dcb->dcb_ext->ext_count = cnf->cnf_dsc->dsc_ext_count;
	    for (i = 0; i < cnf->cnf_dsc->dsc_ext_count; i++)
	    {
		STRUCT_ASSIGN_MACRO(cnf->cnf_ext[i].ext_location,
                                    dcb->dcb_ext->ext_entry[i]);
		if (cnf->cnf_ext[i].ext_location.flags & DCB_JOURNAL)
		    dcb->dcb_jnl = &dcb->dcb_ext->ext_entry[i];
		else if (cnf->cnf_ext[i].ext_location.flags & DCB_CHECKPOINT)
		    dcb->dcb_ckp = &dcb->dcb_ext->ext_entry[i];
		else if (cnf->cnf_ext[i].ext_location.flags & DCB_DUMP)
		    dcb->dcb_dmp = &dcb->dcb_ext->ext_entry[i];
		else if (cnf->cnf_ext[i].ext_location.flags & DCB_ROOT)
		{
		    dcb->dcb_root = &dcb->dcb_ext->ext_entry[i];
		    dcb->dcb_location = dcb->dcb_ext->ext_entry[i];
		}
	    }

	    return (E_DB_OK);
	}
	if (status == E_DB_INFO)
	    return (status);

	if (*config)
	    dm0c_close(*config, 0, &local_dberr);
	*config = 0;
	uleFormat(&jsx->jsx_dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL, &error, 0);
	SETDBERR(&jsx->jsx_dberr, 0, E_DM1302_RFP_CNF_OPEN);
	return (E_DB_ERROR);

    case CLOSEKEEP_CONFIG:
	status = dm0c_close(cnf, 0, &jsx->jsx_dberr);
	if (status == E_DB_OK)
	{
	    *config = 0;
	    return (E_DB_OK);
	}
	uleFormat(&jsx->jsx_dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL, &error, 0);
	SETDBERR(&jsx->jsx_dberr, 0, E_DM1303_RFP_CNF_CLOSE);
	return (E_DB_ERROR);

    case UNKEEP_CONFIG:

	dm0m_deallocate((DM_OBJECT **)&dcb->dcb_ext);
	dcb->dcb_root = 0;
	dcb->dcb_jnl = 0;
	dcb->dcb_dmp = 0;
	dcb->dcb_ckp = 0;
	return (E_DB_OK);

    case CLOSE_CONFIG:

	if (*config == 0)
	    return (E_DB_OK);
	status = dm0c_close(cnf, 0, &jsx->jsx_dberr);
	if (status == E_DB_OK)
	{
	    if (dcb->dcb_ext)
	    {
		/*	Deallocate the EXT. */

		dm0m_deallocate((DM_OBJECT **)&dcb->dcb_ext);
		dcb->dcb_root = 0;
		dcb->dcb_jnl = 0;
		dcb->dcb_dmp = 0;
		dcb->dcb_ckp = 0;
	    }
	    *config = 0;
	    return (status);
	}

	uleFormat(&jsx->jsx_dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL, &error, 0);
	SETDBERR(&jsx->jsx_dberr, 0, E_DM1303_RFP_CNF_CLOSE);
	return (E_DB_ERROR);

    case UPDATE_CONFIG:

	/*  Write configuration file to disk. */

	status = dm0c_close(cnf, DM0C_UPDATE | DM0C_LEAVE_OPEN, &jsx->jsx_dberr);
	if (status == E_DB_OK)
	    return (status);

	uleFormat(&jsx->jsx_dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL, &error, 0);
	SETDBERR(&jsx->jsx_dberr, 0, E_DM1303_RFP_CNF_CLOSE);
	return (E_DB_ERROR);

    case UPDTCOPY_CONFIG:

	/*  Write configuration file to disk and make backup copy */

	status = dm0c_close(cnf, DM0C_UPDATE | DM0C_LEAVE_OPEN | DM0C_COPY,
			    &jsx->jsx_dberr);
	if (status == E_DB_OK)
	    return (status);

	uleFormat(&jsx->jsx_dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL, &error, 0);
	SETDBERR(&jsx->jsx_dberr, 0, E_DM1303_RFP_CNF_CLOSE);
	return (E_DB_ERROR);

    default:
	/* programming error */

	SETDBERR(&jsx->jsx_dberr, 0, E_DM1303_RFP_CNF_CLOSE);
	return (E_DB_ERROR);
    }
    return (status);
}

/*{
** Name: delete_files	- Delete all alias file in a directory.
**
** Description:
**      This routine is called to delete all alias files and all temporary
**	table files in a directory.  This routine will only keep the following
**	file names:
**		aaaaaaaa.rfc
**
**	The following files will be deleted:
**
**		aaaaaaaa.cnf
**		aaaaaaaa.tab - ppp00000.tab	    Database  tables.
**		ppppaaaa.tab - pppppppp.tab	    Temporary tables.
**		*.m*				    Modify temps.
**		*.d*				    Deleted files.
**		*.ali				    Alias files.
**		*.sm*				    Sysmod temps.
**		Any file with wrong name	    User created junk.
**
** Inputs:
**      arg_list                        Agreed upon argument.
**	filename			Pointer to filename.
**	filelength			Length of file name.
**
** Outputs:
**      sys_err				Operating system error.
**	Returns:
**	    OK
**	    DI_ENDFILE
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      09-mar-1987 (Derek)
**          Created for Jupiter.
**      19-nov-1990 (bryanp)
**          Fixed a never-reported bug spotted by an alert compiler: change
**          err_code from a pointer to an object and pass its address.
[@history_template@]...
*/
static DB_STATUS
delete_files(
DMP_LOC_ENTRY	    *location,
char		    *filename,
i4		    l_filename,
CL_ERR_DESC	    *sys_err)
{
    DB_STATUS	    status;
    i4	    	    error;
    DI_IO	    di_file;

    if (MEcmp("aaaaaaaa.rfc", filename, 12) == 0)
    {
	return (OK);
    }

    status = DIdelete(&di_file, (char *)&location->physical,
	    location->phys_length, filename, l_filename,
	    sys_err);
    if (status == OK)
	return (status);

    uleFormat(NULL, E_DM9003_BAD_FILE_DELETE, sys_err, ULE_LOG, NULL, 
	    NULL, 0, NULL,
            &error, 4,
	    4, "None",
	    4, "None",
	    location->phys_length, &location->physical,
	    l_filename, filename);
    return (DI_ENDFILE);
}

/*{
** Name: check_files	- Check to see if checkpoint file exists.
**
** Description:
**      This routine is called to determine if a checkpoint file exists for
**	a specific location.  If it finds the file it changes the
**      input paramater check_params.found to TRUE.
**
**
** Inputs:
**      check_params                    Agreed upon argument.
**	filename			Pointer to filename.
**	filelength			Length of file name.
**
** Outputs:
**      sys_err				Operating system error.
**	Returns:
**	    OK
**	    CK_ENDFILE
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      05-nov-1987 (Jennifer)
**          Created for Jupiter.
[@history_template@]...
*/
static DB_STATUS
check_files(
PTR      	    params,
char		    *filename,
i4		    l_filename,
CL_ERR_DESC	    *sys_err)
{
    CHECK_PARAMS    *p;

    p = (CHECK_PARAMS *)params;
    if (MEcmp((char *)&p->fname, filename, 12) == 0)
    {
	p->found = TRUE;
    }
    return (OK);

}

/*
** Name: get_ckp_sequence	- from which checkpoint should we restore?
**
** Description:
**	This routine is called to decide which checkpoint sequence number is
**	to be used to restore from.
**
**	There are 3 possibilities:
**	1) User indicated no specific checkpoint. In that case, just
**	    use the most recent checkpoint. If the most recent CKP is
**	    no good, then complain.
**	2) User requested last valid checkpoint ('#c').
**	    In that case, we must find the checkpoint as follows:
**	    Beginning with the highest-numbered (most recent) history
**	    entries, we count downwards until we find a valid CKP.
**	    (hopefully, the most recent checkpoint is valid, but if not
**	    we search backwards until we find one).
**	3) User requested a specific checkpoint by sequence number. If
**	    it is a valid checkpoint, we use it; else we complain.
**
**	An invalid checkpoint is of no use, but unfortunately the CKP code
**	tends to leave invalid checkpoint entries in the CNF file when it
**	fails due to errors (disk full, etc.). By simply ignoring those
**	entries, we cause them to be MUCH less serious (the only real danger
**	is that a user could run CKP_HISTORY number of failed checkpoints,
**	in which case they're plumb out of luck).
**
**	If at the end, we cannot find a usable checkpoint, we return an
**	error.
**
**	Note that in general the journal histories and dump histories are NOT
**	in lockstep; since offline checkpoints allocate a journal history but
**	NOT a dump history, we have to be careful when trying to walk the
**	two history arrays. Therefore, the algorithms in this routine walk
**	the JOURNAL history array, and only access the dump history array when
**	trying to match a specific journal history entry to a specific dump
**	history entry. If we ever merge the two histories into a single history
**	this whole muck will become MUCH simpler.
**
** Inputs:
**	jsx		    - DMF_JSX context pointer
**	cnf		    - the config info for this database
**
** Outputs:
**	jnl_history_no	    - which journal history entry to use.
**			      -1 if none found.
**	dmp_history_no	    - which dump history entry to use.
**			      -1 if this is an OFFLINE checkpoint.
**	err_code	    - if there is an error, set to:
**			      E_DM1327_RFP_INV_CHECKPOINT
**
** Returns:
**	status		    - E_DB_OK or E_DB_ERROR
**
** History:
**	7-may-90 (bryanp)
**	    Created to support rolling back to specified checkpoints.
**	22-nov-1993 (jnash)
**	    B56095. Check jnl_first_jnl_seq during rolldb #c operations to
**	    catch operator errors in attempting to rollforward from a point
**	    where the database was not journaled.
**	31-jan-1994 (jnash)
**	  - Fix truncated E_DM134C_RFP_INV_CHECKPOINT error message.
**	  - jnl_first_jnl_seq == 0 checking no longer done here.
**	21-feb-1994 (jnash)
**	    In allowing rolldb #c -c operations, modify routine to
**	    find starting journal file even when -c specified.  The
**	    routine no longer passes back *jnl_history_no = -1 to
**	    indicate a -c request, instead requiring the caller to
**	    check jsx_status.
**      24-jan-1995 (stial01)
**          Bug 66473: ckp_mode is now bit pattern.
**      29-jul-1998 (nicph02)
**          bug 90541: Allow users to apply journals that are older than
**          the first valid journal. The new logic is that it is assumed that
**          there is no missing journals (no ckpdb -j done). If one is found
**          during processing, then rollforward is aborted (E_DM134C logged).
**
*/
static DB_STATUS
get_ckp_sequence(
DMF_JSX		*jsx,
DM0C_CNF	*cnf,
i4		*jnl_history_no,
i4		*dmp_history_no)
{
    i4	i;
    i4	j;
    i4	dump_history_found;
    char	line_buffer[132];
    bool	jnl_processing;
    i4		error;

    CLRDBERR(&jsx->jsx_dberr);


    if ( ((jsx->jsx_status & JSX_CKP_SELECTED)  == 0) &&
	  (jsx->jsx_status & JSX_LAST_VALID_CKP) == 0 )
    {
	/*
	** Use the most recent checkpoint history entry.
	*/
	*jnl_history_no = cnf->cnf_jnl->jnl_count - 1;
	if (cnf->cnf_jnl->jnl_history[*jnl_history_no].ckp_mode & 
						CKP_ONLINE)
	{
	    *dmp_history_no = cnf->cnf_dmp->dmp_count - 1;
	}
	else
	{
	    *dmp_history_no = -1;
	}
    }
    else if ( ((jsx->jsx_status & JSX_CKP_SELECTED) == 0) &&
	      (jsx->jsx_status & JSX_LAST_VALID_CKP) != 0 )
    {
	/*
	** Use the last valid checkpoint. Count backwards to find it. Note
	** that we count backward through the journal history. If a particular
	** journal history element indicates that the checkpoint was an
	** online checkpoint, then we lookup the associated dump history and
	** use it.
	*/

	for ( i = 0; i < cnf->cnf_jnl->jnl_count; i++)
	{
	    *jnl_history_no = cnf->cnf_jnl->jnl_count - (1 + i);
	    if (cnf->cnf_jnl->jnl_history[*jnl_history_no].ckp_mode & 
						CKP_ONLINE)
	    {
		dump_history_found = 0;
		for ( j = 0; j < cnf->cnf_dmp->dmp_count; j++)
		{
		    *dmp_history_no = cnf->cnf_dmp->dmp_count - (1 + j);
		    if (cnf->cnf_dmp->dmp_history[*dmp_history_no].ckp_sequence
					 ==
			cnf->cnf_jnl->jnl_history[*jnl_history_no].ckp_sequence)
		    {
			dump_history_found = 1;
			break;
		    }
		}
	    }
	    else
	    {
		/* offline checkpoints have no associated dump history */

		*dmp_history_no = -1;
		dump_history_found = 1;
	    }
	    if (dump_history_found &&
		DMFJSP_VALID_CHECKPOINT(cnf,*jnl_history_no,*dmp_history_no))
	    {
		TRformat( dmf_put_line, 0, line_buffer, sizeof(line_buffer),
			    "%@ RFP: Found a valid checkpoint to use:\n");
		TRformat( dmf_put_line, 0, line_buffer, sizeof(line_buffer),
			    "%24*    : Journal seq = %d, Dump seq = %d\n",
			    *jnl_history_no, *dmp_history_no );
		break;
	    }
	    else
	    {
		TRformat( dmf_put_line, 0, line_buffer, sizeof(line_buffer),
			    "%@ RFP: Found an INVALID checkpoint:\n");
		TRformat( dmf_put_line, 0, line_buffer, sizeof(line_buffer),
			    "%24*    : Journal seq = %d, Dump seq = %d\n",
			    *jnl_history_no, *dmp_history_no );
		TRformat( dmf_put_line, 0, line_buffer, sizeof(line_buffer),
			    "%24*    : This checkpoint will be ignored.\n");
	    }
	}
	if (i >= cnf->cnf_jnl->jnl_count)
	{
	    /*
	    ** Failed to find a valid checkpoint.
	    */
	    *dmp_history_no = *jnl_history_no = -1;
	}
    }
    else if ((jsx->jsx_status & JSX_CKP_SELECTED) != 0 &&
	     (jsx->jsx_status & JSX_LAST_VALID_CKP) == 0)
    {
	/*
	** Use the specific checkpoint that they selected. The user
	** selected a checkpoint sequence number. Translate the sequence
	** number into an array offset. If the journal history entry for this
	** sequence number indicates that the dump was an online checkpoint,
	** then look up the dump history element as well.
	*/
	for ( j = 0; j < cnf->cnf_jnl->jnl_count; j++)
	{
	    if (cnf->cnf_jnl->jnl_history[j].ckp_sequence ==
							jsx->jsx_ckp_number)
	    {
		*jnl_history_no = j;

		/*
		** Note if we plan to apply journals.
		*/
		jnl_processing =
		    (((cnf->cnf_dsc->dsc_status & DSC_JOURNAL) != 0) &&
                          ((jsx->jsx_status & JSX_USEJNL) != 0));

		/*
		** If we plan to apply journals, make sure
		** the journals are valid from that checkpoint.
		** cnf->cnf_jnl->jnl_first_jnl_seq is the oldest
		** checkpoint from which a rollforward +j may take place.
		*/
		if (cnf->cnf_dsc->dsc_cnf_version >= CNF_V4)
		{
		    if ( (jnl_processing) &&
                                (cnf->cnf_jnl->jnl_history[j].ckp_f_jnl == 0) )
		    {
			char        error_buffer[ER_MAX_LEN];
			i4     error_length;
			i4     count;

			uleFormat(NULL, E_DM134C_RFP_INV_CHECKPOINT,
			    (CL_ERR_DESC *)NULL, ULE_LOOKUP, (DB_SQLSTATE *)NULL,
			    error_buffer, sizeof(error_buffer),
			    &error_length, &error, 2,
			    0, jsx->jsx_ckp_number,
			    0, cnf->cnf_jnl->jnl_first_jnl_seq);

			SIwrite(error_length, error_buffer, &count, stderr);
			SIwrite(1, "\n", &count, stderr);
			TRdisplay("%t\n", error_length, error_buffer);

			/*
			** #f overrides this check.
			*/
			if ((jsx->jsx_status1 & JSX_FORCE) == 0)
			{
			    SETDBERR(&jsx->jsx_dberr, 0, E_DM133C_RFP_UNUSABLE_CKP);
			    return (E_DB_ERROR);
			}
		    }
		}

		if (cnf->cnf_jnl->jnl_history[j].ckp_mode & CKP_ONLINE)
		{
		    dump_history_found = 0;
		    for ( i = 0; i < cnf->cnf_dmp->dmp_count; i++)
		    {
			if (cnf->cnf_dmp->dmp_history[i].ckp_sequence ==
							jsx->jsx_ckp_number)
			{
			    dump_history_found = 1;
			    *dmp_history_no = i;
			    break;
			}
		    }
		}
		else
		{
		    *dmp_history_no = -1;
		    dump_history_found = 1;
		}
		break;
	    }
	}
	if (j >= cnf->cnf_jnl->jnl_count || ! dump_history_found)
	{
	    /*
	    ** Failed to find a valid checkpoint.
	    */
	    *dmp_history_no = *jnl_history_no = -1;
	}
    }
    else
    {
	/*
	** Use no checkpoint at all.
	*/
	*dmp_history_no = *jnl_history_no = -1;
    }

    if ((jsx->jsx_status & JSX_USECKP) != 0 &&
	    (*jnl_history_no == -1 ||
	    DMFJSP_VALID_CHECKPOINT(cnf,*jnl_history_no,*dmp_history_no) == 0))
    {
	/*
	** "The indicated checkpoint has been marked INVALID. A checkpoint may
	** be marked INVALID if the backup operation failed due to a disk or
	** tape error (such as disk full or tape write error). Additionally,
	** an online checkpoint may be marked invalid if the Archiver was
	** unable to copy the journal and dump file information to the journal
	** and dump directories (check the Archiver error log for messages).
	** You can restore your database from the most recent valid checkpoint
	** by issuing the rollforwarddb command with the '#c' argument. In a
	** future release this action will be taken automatically."
	*/
	uleFormat(NULL, E_DM1327_RFP_INV_CHECKPOINT, NULL,
		    ULE_LOG, NULL, NULL, 0, NULL, &error, 0);
	SETDBERR(&jsx->jsx_dberr, 0, E_DM133C_RFP_UNUSABLE_CKP);
	return (E_DB_ERROR);
    }

    /*
    ** At this point, we have found a valid checkpoint jnl/dmp history
    ** entry and have stored its history numbers in *dmp_history_no and
    ** *jnl_history_no. (*dmp_history_no may be -1 if the checkpoint was an
    ** offline checkpoint).
    */

    return (E_DB_OK);
}

/*
** Name: lookup_tx	- Look up transaction context
**
** Description:
**	This routine returns the TX context for a transaction identified
**	by its transaction id.
**
**	A null pointer is returned if the transaction is unknown.
**
** Inputs:
**	rfp		    - Rollforward context
**	tran_id		    - Transaction Id
**
** Outputs:
**	return_tx	    - Pointer to the transaction context
**
** Returns:
**	none
**
** History:
**	15-mar-1993 (rogerk)
**	    Created.
*/
static VOID
lookup_tx(
DMF_RFP		*rfp,
DB_TRAN_ID	*tran_id,
RFP_TX		**return_tx)
{
    RFP_TX	    *tx;


    for (tx = rfp->rfp_txh[tran_id->db_low_tran & (RFP_TXH_CNT - 1)];
	 tx != NULL;
	 tx = tx->tx_hash_next)
    {
	if ((tx->tx_bt.bt_header.tran_id.db_high_tran == tran_id->db_high_tran) &&
	    (tx->tx_bt.bt_header.tran_id.db_low_tran  == tran_id->db_low_tran))
	{
	    break;
	}
    }

    *return_tx = tx;
    return;
}

/*
** Name: add_tx	- Add context for new transaction
**
** Description:
**	This routine adds a new TX context to the list of open transactions
**	in a rollforward process.
**
**	It is called during journal record processing when a BT record
**	is found.
**
** Inputs:
**	rfp		    - Rollforward context
**      btrec		    - DM0LBT record
**
** Outputs:
**	err_code	    - reason for error
**
** Returns:
**	E_DB_OK
**	E_DB_ERROR
**
** History:
**	15-mar-1993 (rogerk)
**	    Created.
**	26-jul-1993 (rogerk)
**	    Initialize End Transaction Action list.
**	20-sep-1993 (rogerk)
**	    Remove ETA list, it was moved to dmve processing.
**	5-Apr-2004 (schka24)
**	    Init rawdata stuff.
*/
static DB_STATUS
add_tx(
DMF_RFP		    *rfp,
DM0L_BT		    *btrec)
{
    DMF_JSX	    *jsx = rfp->rfp_jsx;
    DB_STATUS	    status;
    i4		    i;
    RFP_TX	    *tx;
    RFP_TX	    **txq;
    i4		    error;

    CLRDBERR(&jsx->jsx_dberr);

    /*
    ** Allocate a TX structure.
    */
    status = dm0m_allocate(sizeof(RFP_TX), 0, RFPTX_CB, RFPTX_ASCII_ID,
			(char *)rfp, (DM_OBJECT **)&tx, &jsx->jsx_dberr);
    if (status != E_DB_OK)
    {
	uleFormat(&jsx->jsx_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
	    (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	SETDBERR(&jsx->jsx_dberr, 0, E_DM1320_RFP_TOO_MANY_TX);
	return (E_DB_ERROR);
    }

    /*
    ** Fill in the Transaction information.
    */
    STRUCT_ASSIGN_MACRO(*btrec, tx->tx_bt);
    tx->tx_clr_lsn.lsn_high = 0;
    tx->tx_clr_lsn.lsn_low  = 0;
    tx->tx_id = 0;
    tx->tx_status = 0;
    tx->tx_process_cnt = 0;
    tx->tx_undo_cnt = 0;
    for (i = 0; i < DM_RAWD_TYPES; ++i)
    {
	tx->tx_rawdata[i] = NULL;
	tx->tx_rawdata_size[i] = 0;
    }

    /*
    ** Put TX on the active transaction queue.
    */
    tx->tx_next = rfp->rfp_tx;
    tx->tx_prev = (RFP_TX *) NULL;
    if (rfp->rfp_tx != NULL)
	rfp->rfp_tx->tx_prev = tx;
    rfp->rfp_tx = tx;

    /*
    ** Put TX on hash bucket queue.
    */
    txq = &rfp->rfp_txh[tx->tx_bt.bt_header.tran_id.db_low_tran & (RFP_TXH_CNT - 1)];

    tx->tx_hash_next = *txq;
    tx->tx_hash_prev = (RFP_TX *) NULL;
    if (*txq)
	(*txq)->tx_hash_prev = tx;
    *txq = tx;

    rfp->rfp_tx_count++;

    return (E_DB_OK);
}

/*
** Name: remove_tx	- Delete transaction context
**
** Description:
**	This routine deletes the TX context for a transaction from
**	the rollforward open transaction list.
**
**	It also processes any end-transaction actions pended to the
**	transaction context.
**
**	It is called during journal record processing when an ET record
**	is found.
**
** Inputs:
**	rfp		    - Rollforward context
**	tx		    - Transaction to remove
**
** Outputs:
**	none
**
** Returns:
**	none
**
** History:
**	15-mar-1993 (rogerk)
**	    Created.
**	26-jul-1993 (rogerk)
**	    Added end-transaction processing of deleting temporary files
**	    used in DDL operations.
**	20-sep-1993 (rogerk)
**	    Moved traversing of the end-transation action list into the
**	    process_et_action routine.
*/
static VOID
remove_tx(
DMF_RFP		*rfp,
RFP_TX		*tx)
{

    /*
    ** Check for any actions that have been posted for processing when the
    ** transaction is completed.  Also cleanup any other actions posted
    ** during the duration of this transaction.
    */
    process_et_action(rfp, tx);

    /*
    ** Remove from the active transaction queue.
    */
    if (tx->tx_next != NULL)
	tx->tx_next->tx_prev = tx->tx_prev;

    if (tx->tx_prev != NULL)
	tx->tx_prev->tx_next = tx->tx_next;
    else
	rfp->rfp_tx = tx->tx_next;

    /*
    ** Remove from the transaction's hash bucket queue.
    */
    if (tx->tx_hash_next != NULL)
	tx->tx_hash_next->tx_hash_prev = tx->tx_hash_prev;

    if (tx->tx_hash_prev != NULL)
	tx->tx_hash_prev->tx_hash_next = tx->tx_hash_next;
    else
	rfp->rfp_txh[tx->tx_bt.bt_header.tran_id.db_low_tran & (RFP_TXH_CNT - 1)] =
							    tx->tx_hash_next;

    rfp->rfp_tx_count--;

    /*
    ** Deallocate the control block.
    */
    dm0m_deallocate((DM_OBJECT **)&tx);
}

/*
** Name: rollback_open_xacts	- Perform Journal UNDO processing
**
** Description:
**	This routine brings a database which has been rolled forward
**	to a consistent state by rolling back those transactions which
**	were not completely rolled forward to an End Transaction record.
**
**	It is called during the undo phase of rollforward processing
**	when there are transactions found at the end of the normal journal
**	processing which were not rolled forward to completion.
**
** Inputs:
**	jsx		    - JSP context
**	rfp		    - Rollforward context
**	dcb		    - Open database control block
**
** Outputs:
**	err_code	    - reason for error status
**
** Returns:
**	E_DB_OK
**	E_DB_ERROR
**
** History:
**	15-mar-1993 (rogerk)
**	    Created.
**	21-feb-1994 (jnash)
**	    With the added level of support for rolldb #c, it is
**	    no longer "extremely unlikely" that we will be rolling
**	    back transactions prior to the first journal in the
**	    most recent checkpoint.  Remove two lines of TRdisplay()
**	    that indicated such.
*/
static DB_STATUS
rollback_open_xacts(
DMF_JSX		*jsx,
DMF_RFP		*rfp,
DMP_DCB		*dcb,
DMVE_CB		*dmve)
{
    DM0J_CTX		*jnl = NULL;
    i4		current_jnl;
    bool		rollback_complete = FALSE;
    char		line_buffer[132];
    DB_STATUS		status;
    i4			error;
    DB_ERROR		local_dberr;

    CLRDBERR(&jsx->jsx_dberr);

    if (jsx->jsx_status & JSX_TRACE)
	TRdisplay("%@ Rolling Back open transactions.\n");

    for (;;)
    {
	/*
	** Add the database and transaction contexts to the Logging System
	** for rollforward undo processing.
	*/
	status = prepare_undo_phase(jsx, rfp, dcb);
	if (status != E_DB_OK)
	    break;

	/*
	** Read backwards through the journal records until all incomplete
	** transactions are rolled back.
	**
	** If we reach the beginning of a journal file before completing
	** rollback processing then go back to the previous journal file.
	*/
	current_jnl = rfp->rfp_jnl_history[rfp->jnl_last].ckp_l_jnl;

	while (rollback_complete == FALSE)
	{
	    /*
	    ** Consistency Check - notice case that we have proceeded
	    ** backwards past the journal file that started the last
	    ** checkpoint's journal history.
	    **
	    ** This implies that some outstanding transaction was active
	    ** across a checkpoint boundary.  This extremely improbable
	    ** condition (in fact - it may be impossible) is most likely
	    ** caused by us missing a BT record and warrants some sort of
	    ** warning message.
	    */
	    if (current_jnl < rfp->rfp_jnl_history[rfp->jnl_last].ckp_f_jnl)
	    {
		/*
		** If we reach the first journal in the database journal
		** history, then we know we have failed.
		*/
		if (current_jnl <
			rfp->rfp_jnl_history[rfp->jnl_first].ckp_f_jnl)
		{
		    status = E_DB_ERROR;
		    SETDBERR(&jsx->jsx_dberr, 0, E_DM133D_RFP_UNDO_INCOMPLETE);
		    break;
		}

		TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
		    "%@ RFP: Attempting to resolve transactions which were\n");
		TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
		    "%28* : incomplete at the end of this journal history.\n");
		TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
		    "%28* : Proceeding back to journal file %d which is\n",
		    current_jnl);
		TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
		    "%28* : previous to the first journal file created\n");
		TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
		    "%28* : since the most recent checkpoint.\n");
	    }

	    TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
		"%@ RFP: Rollback processing with journal file sequence %d.\n",
		current_jnl);

	    status = dm0j_open(DM0J_MERGE, (char *)&dcb->dcb_jnl->physical,
			dcb->dcb_jnl->phys_length, &dcb->dcb_name,
			rfp->jnl_block_size, current_jnl, rfp->ckp_jnl_sequence,
			DM0J_EXTREME, DM0J_M_READ, -1, DM0J_BACKWARD,
			(DB_TAB_ID *)NULL, &jnl, &jsx->jsx_dberr);
	    if (status != E_DB_OK)
	    {
		if (jsx->jsx_dberr.err_code == E_DM9034_BAD_JNL_NOT_FOUND)
		{
		    TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
		     "%@ RFP: Journal file sequence %d not found -- skipped.\n",
		      current_jnl);
		    TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
		     "%28* : This is an unexpected occurrence and may\n");
		    TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
		     "%28* : indicate that required journal files are\n");
		    TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
		     "%28* : missing from the database journal directory.\n");
		    TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
		     "%28* : If this is the case, this rollforward will not\n");
		    TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
		     "%28* : likely succeed.\n");

		    status = E_DB_OK;
		    CLRDBERR(&jsx->jsx_dberr);
		    current_jnl--;
		    continue;
		}
		break;
	    }

	    /*
	    ** Undo transactions using the current journal file.
	    */
	    status = undo_journal_records(jsx, rfp, jnl, dcb,
					&rollback_complete, dmve);
	    if (DB_FAILURE_MACRO(status))
		break;

	    /*
	    ** Close the current journal file.
	    */
	    if (jnl)
	    {
		status = dm0j_close(&jnl, &jsx->jsx_dberr);
		if (status != E_DB_OK)
		    break;
	    }

	    /*
	    ** If there are still remaining transaction to rollback then we
	    ** go on to process the previous journal file.
	    */
	    current_jnl--;
	}

	/*
	** Check for an error in the above journal processing.
	*/
	if (status != E_DB_OK)
	    break;

	/*
	** Complete the rollback of active transactions (by logging the
	** abort log records) and clean up the database and transaction
	** contexts.
	*/
	status = cleanup_undo_phase(jsx, rfp, dcb);
	if (status != E_DB_OK)
	    break;

	if (jsx->jsx_status & JSX_TRACE)
	    TRdisplay("%@ Rollback of open transactions completed.\n");

	return (E_DB_OK);
    }

    if (jnl)
    {
	status = dm0j_close(&jnl, &local_dberr);
	if (status != E_DB_OK)
	{
	    uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	}
    }

    uleFormat(&jsx->jsx_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
	(char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
    SETDBERR(&jsx->jsx_dberr, 0, E_DM133E_RFP_ROLLBACK_ERROR);
    return (E_DB_ERROR);
}

/*
** Name: prepare_undo_phase	- Prepare for Rollforward Rollback
**
** Description:
**	This routine establishes context in the logging system for
**	transactions which must be undone during rollforward undo processing.
**
**	Each incomplete transaction is added to the Logging System through
**	an LGbegin call so that new log records can be written on their behalf.
**
**	It is called during the undo phase of rollforward processing
**	when there are transactions found at the end of the normal journal
**	processing which were not rolled forward to completion.
**
**	If rollforward abort processing is being done in NOLOGGING mode
**	then there is no context added to the logging system.
**
** Inputs:
**	jsx		    - JSP context
**	rfp		    - Rollforward context
**	dcb		    - Open database control block
**
** Outputs:
**	err_code	    - reason for error status
**
** Returns:
**	E_DB_OK
**	E_DB_ERROR
**
** History:
**	15-mar-1993 (rogerk)
**	    Created.
**	26-jul-1993 (bryanp)
**	    We no longer log a closedb log record, so we no longer need the
**		'lx_id' argument to dm0l_opendb() and dm0l_closedb().
*/
static DB_STATUS
prepare_undo_phase(
DMF_JSX		*jsx,
DMF_RFP		*rfp,
DMP_DCB		*dcb)
{
    RFP_TX		*tx;
    DB_TRAN_ID		tran_id;
    LG_CONTEXT		ctx;
    DB_STATUS		status = E_DB_OK;
    STATUS		cl_status;
    CL_ERR_DESC		sys_err;
    i4			error;

    CLRDBERR(&jsx->jsx_dberr);

    /*
    ** During rollforwards which are not being used to fully restore the
    ** database to the end of the journal stream we do not log the updates
    ** made to bring the database to a consistent state (by rolling back
    ** transactions not completed when rollforward processing stops).
    **
    ** If no logging is being done during the rollback phase, then
    ** we don't bother adding any context to the logging system.
    */
    if (rfp->rfp_status & RFP_NO_CLRLOGGING)
	return (E_DB_OK);

    if (jsx->jsx_status & JSX_TRACE)
	TRdisplay("%@ Prepare Database and Transactions for Rollback.\n");

    rfp->rfp_status |= RFP_DID_JOURNALING;

    for (;;)
    {
	/*
	** Open the database in the logging system and write an Opendb
	** Log Record.  This establishes a context in the logging system
	** for the database and allows us to later write log records during
	** undo processing.
	**
	** The Opendb is needed to allow archiving of the database should the
	** system fail during the rollforward recovery.  (Without the Opendb
	** log record a failure would lose the database context and the archiver
	** would not process the journaled undo records.)
	*/
	status = dm0l_opendb(dmf_svcb->svcb_lctx_ptr->lctx_lgid, DM0L_JOURNAL,
	    &dcb->dcb_name, &dcb->dcb_db_owner, dcb->dcb_id,
	    &dcb->dcb_location.physical, dcb->dcb_location.phys_length,
	    &dcb->dcb_log_id, (LG_LSN *)0, &jsx->jsx_dberr);
	if (status != E_DB_OK)
	    break;

	if (jsx->jsx_status & JSX_TRACE)
	{
	    TRdisplay("%@ Database %~t (dbid %d) added to the Logging System\n",
		sizeof(DB_DB_NAME), dcb->dcb_name.db_db_name, dcb->dcb_id);
	    TRdisplay("%4* with internal id 0x%x\n", dcb->dcb_log_id);
	}

	for (tx = rfp->rfp_tx; tx != NULL; tx = tx->tx_next)
	{
	    if (jsx->jsx_status & JSX_TRACE)
	    {
		TRdisplay("%@ Restarting Transaction 0x%x%x.\n",
			tx->tx_bt.bt_header.tran_id.db_high_tran, 
			tx->tx_bt.bt_header.tran_id.db_low_tran);
	    }

	    /*
	    ** Begin a transaction to re-establish the recover xact
	    ** context.  After creating a transaction, alter it to reflect
	    ** the same transaction id as the original xact.
	    **
	    ** The transactions are started with LG_NOPROTECT status so
	    ** that the system does not try to abort them should the
	    ** process or installation fail.  These transactions also do
	    ** not prevent the BOF from moving forward as soon as the
	    ** archiver has moved all journal records to the journal files.
	    **
	    ** Add the transaction with LG_NORESERVE status to indicate that
	    ** CLR's will be logged without first reserving space for them.
	    */
	    cl_status = LGbegin((LG_JOURNAL | LG_NOPROTECT | LG_NORESERVE),
		dcb->dcb_log_id, &tran_id, &tx->tx_id,
		sizeof(DB_OWN_NAME), dcb->dcb_name.db_db_name, 
		(DB_DIS_TRAN_ID*)NULL, &sys_err);
	    if (cl_status != OK)
	    {
		uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
		uleFormat(NULL, E_DM900C_BAD_LOG_BEGIN, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, &error, 1,
		    0, dcb->dcb_log_id);
		SETDBERR(&jsx->jsx_dberr, 0, E_DM133F_RFP_UNDO_BEGIN_XACT);
		status = E_DB_ERROR;
		break;
	    }

	    ctx.lg_ctx_id = tx->tx_id;
	    STRUCT_ASSIGN_MACRO(tx->tx_bt.bt_header.tran_id, ctx.lg_ctx_eid);
	    ctx.lg_ctx_last.la_sequence  = 0;
	    ctx.lg_ctx_last.la_block     = 0;
	    ctx.lg_ctx_last.la_offset    = 0;
	    ctx.lg_ctx_first.la_sequence = 0;
	    ctx.lg_ctx_first.la_block    = 0;
	    ctx.lg_ctx_first.la_offset   = 0;
	    ctx.lg_ctx_cp.la_sequence    = 0;
	    ctx.lg_ctx_cp.la_block       = 0;
	    ctx.lg_ctx_cp.la_offset      = 0;

	    cl_status = LGalter(LG_A_CONTEXT, (PTR)&ctx, sizeof(ctx), &sys_err);
	    if (cl_status != OK)
	    {
		uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
		uleFormat(NULL, E_DM900B_BAD_LOG_ALTER, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, &error, 1,
		    0, LG_A_CONTEXT);
		SETDBERR(&jsx->jsx_dberr, 0, E_DM133F_RFP_UNDO_BEGIN_XACT);
		status = E_DB_ERROR;
		break;
	    }
	}

	/*
	** If an error occurred during transaction work then break to below.
	*/
	if (status != E_DB_OK)
	    break;

	return (E_DB_OK);
    }

    uleFormat(&jsx->jsx_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
	(char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
    SETDBERR(&jsx->jsx_dberr, 0, E_DM1340_RFP_PREPARE_UNDO);
    return (E_DB_ERROR);
}

/*
** Name: cleanup_undo_phase	- Cleanup after Rollforward Rollback
**
** Description:
**	This routine cleans up the  context in the logging system for
**	transactions which were undone during rollforward undo processing.
**
**	Each rolled back transaction is completed by logging and abort record
**	and then the xact context is removed from the Logging System.
**
**	It is called during the undo phase of rollforward processing
**	when there are transactions found at the end of the normal journal
**	processing which were not rolled forward to completion.
**
** Inputs:
**	jsx		    - JSP context
**	rfp		    - Rollforward context
**	dcb		    - Open database control block
**
** Outputs:
**	err_code	    - reason for error status
**
** Returns:
**	E_DB_OK
**	E_DB_ERROR
**
** History:
**	15-mar-1993 (rogerk)
**	    Created.
**	26-jul-1993 (bryanp)
**	    We no longer log a closedb log record, so we no longer need the
**		'lx_id' argument to dm0l_opendb() and dm0l_closedb().
**	21-feb-1994 (jnash)
**	    Don't do normal cleanup if RFP_NO_CLRLOGGING.
*/
static DB_STATUS
cleanup_undo_phase(
DMF_JSX		*jsx,
DMF_RFP		*rfp,
DMP_DCB		*dcb)
{
    RFP_TX		*tx;
    DB_TAB_TIMESTAMP    ctime;
    bool		rollback_error = FALSE;
    char		line_buffer[132];
    DB_STATUS		status = E_DB_OK;
    STATUS		cl_status;
    CL_ERR_DESC		sys_err;
    i4			error;

    CLRDBERR(&jsx->jsx_dberr);

    if (rfp->rfp_status & RFP_NO_CLRLOGGING)
	return(E_DB_OK);

    if (jsx->jsx_status & JSX_TRACE)
	TRdisplay("%@ Clean up Database and Transaction contexts.\n");

    for (;;)
    {
	for (tx = rfp->rfp_tx; tx != NULL; tx = tx->tx_next)
	{
	    if (tx->tx_status & RFP_TX_COMPLETE)
	    {
		if (jsx->jsx_status & JSX_TRACE)
		{
		    TRdisplay("%@ Writing ET record for transaction 0x%x%x.\n",
			tx->tx_bt.bt_header.tran_id.db_high_tran,
			tx->tx_bt.bt_header.tran_id.db_low_tran);
		}

		status = dm0l_et(tx->tx_id, (DM0L_JOURNAL | DM0L_MASTER),
			DM0L_ABORT, &tx->tx_bt.bt_header.tran_id, 
			dcb->dcb_id, &ctime, &jsx->jsx_dberr);
		if (status != E_DB_OK)
		    break;

		cl_status = LGend(tx->tx_id, 0, &sys_err);
		if (cl_status != OK)
		{
		    uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
			(char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
		    uleFormat(NULL, E_DM900E_BAD_LOG_END, &sys_err, ULE_LOG, NULL,
			(char *)NULL, (i4)0, (i4 *)NULL, &error, 1,
			0, tx->tx_id);
		    SETDBERR(&jsx->jsx_dberr, 0, E_DM1341_RFP_CLEANUP_UNDO);
		    status = E_DB_ERROR;
		    break;
		}
	    }
	    else
	    {
		uleFormat(NULL, E_DM1345_RFP_XACT_UNDO_FAIL,
		    (CL_ERR_DESC *)NULL, ULE_LOG, NULL, (char *)NULL,
		    (i4)0, (i4 *)NULL, &error, 3,
		    sizeof(DB_DB_NAME), dcb->dcb_name.db_db_name,
		    0, tx->tx_bt.bt_header.tran_id.db_high_tran,
		    0, tx->tx_bt.bt_header.tran_id.db_low_tran);

		TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
		    "%@ RFP:Rollforward Error: Transaction 0x%x%x, which was\n",
		    tx->tx_bt.bt_header.tran_id.db_high_tran, 
		    tx->tx_bt.bt_header.tran_id.db_low_tran);
		TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
		    "%28* : found to be in an incomplete state at the end\n");
		TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
		    "%28* : of rollforward journal processing, could not be\n");
		TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
		    "%28* : rolled back.  The database has been left in an\n");
		TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
		    "%28* : inconsistent state.\n");

		rollback_error = TRUE;
	    }
	}

	/*
	** If an error occurred during transaction work then break to below.
	*/
	if (status != E_DB_OK)
	    break;

	if (jsx->jsx_status & JSX_TRACE)
	{
	    TRdisplay("%@ Removing Database %~t (dbid %d) - Log DBID 0x%x\n",
		sizeof(DB_DB_NAME), dcb->dcb_name.db_db_name, dcb->dcb_id,
		dcb->dcb_log_id);
	}

	status = dm0l_closedb(dcb->dcb_log_id, &jsx->jsx_dberr);
	dcb->dcb_log_id = 0;
	if (status != E_DB_OK)
	    break;

	if (rollback_error)
	{
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM1342_RFP_INCOMPLETE_UNDO);
	    break;
	}

	return (E_DB_OK);
    }

    uleFormat(&jsx->jsx_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
	(char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
    SETDBERR(&jsx->jsx_dberr, 0, E_DM1341_RFP_CLEANUP_UNDO);
    return (E_DB_ERROR);
}

/*
** Name: undo_journal_records	- Perform Journal UNDO processing
**
** Description:
**	This routine reads backwards through the supplied journal
**	file rolling back updates belonging to transactions on the
**	incomplete transaction list (rfp->rfp_tx).
**
**	It is called during the undo phase of rollforward processing
**	when there are transactions found at the end of the normal journal
**	processing which were not ended with a ET record.
**
** Inputs:
**	jsx		    - JSP context
**	rfp		    - Rollforward context
**	jnl		    - Open journal file context
**	dcb		    - Open database control block
**      dmve
**
** Outputs:
**	rollback_complete   - Indicates that all active xacts have been undone
**	err_code	    - reason for error status
**
** Returns:
**	E_DB_OK
**	E_DB_ERROR
**
** History:
**	15-mar-1993 (rogerk)
**	    Created.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	24-may-1993 (andys)
**	    Pass appropriate logfile blocksize to dmd_log and dmd_format_log
**	    rather than using hardwired 4096.
**	23-aug-1993 (jnash)
**	    Modify ABORTSAVE handling so that SAVEPOINT log records needn't
**	    be journaled.
**	21-jan-1994 (jnash)
**	    When scanning backwards, if -e (or -end_lsn) specified don't
**	    start undo'ing transactions until we've gotten to the record
**	    representing the -e time (or -end_lsn).
**	06-feb-2009 (thaju02) (B121627)
**	    If we have gone backwards and log rec lsn is less than 
**	    CLR lsn, then turn off the CLR_JUMP flag so that record 
**	    for this transaction will be processed.
*/
static DB_STATUS
undo_journal_records(
DMF_JSX		*jsx,
DMF_RFP		*rfp,
DM0J_CTX	*jnl,
DMP_DCB		*dcb,
bool		*rollback_complete,
DMVE_CB		*dmve)
{
    RFP_TX		*tx;
    DM0L_HEADER		*record;
    i4		l_record;
    DB_STATUS		status = E_DB_OK;
    i4			error;
    DB_ERROR		local_dberr;

    CLRDBERR(&jsx->jsx_dberr);

    for (;;)
    {
	/*
	** Read the previous record from the current journal file.
	*/
	status = dm0j_read(jnl, (PTR *)&record, &l_record, &jsx->jsx_dberr);
	if (status != E_DB_OK)
	{
	    /*
	    ** If we hit the end of the journal file, then return status OK
	    ** back to the caller, who will open the next journal file and
	    ** call this routine again.
	    */
	    if (jsx->jsx_dberr.err_code == E_DM0055_NONEXT)
	    {
		status = E_DB_OK;
		CLRDBERR(&jsx->jsx_dberr);
	    }
	    break;
	}

	/*
	** If -e or -end_lsn specified, don't begin undo until we have
	** read backwards to journal records of interest.
	*/
	if ( ((jsx->jsx_status & JSX_ETIME) &&
	      (LSN_GTE(&record->lsn, &rfp->rfp_etime_lsn))) ||
	     ((jsx->jsx_status1 & JSX_END_LSN) &&
	      (LSN_GTE(&record->lsn, &jsx->jsx_end_lsn))) )
	{
	    if (jsx->jsx_status & JSX_TRACE)
		TRdisplay("%@ RFP: Undo skipped LSN (%x,%x).\n",
		    record->lsn.lsn_high, record->lsn.lsn_low);

	    continue;
	}

	/*
	** Check if this record belongs to one of the transactions we are
	** rolling back.  If not, then go on to the next record.
	*/
	for (tx = rfp->rfp_tx; tx != NULL; tx = tx->tx_next)
	{
	    if ((tx->tx_bt.bt_header.tran_id.db_low_tran == record->tran_id.db_low_tran) &&
		(tx->tx_bt.bt_header.tran_id.db_high_tran == record->tran_id.db_high_tran))
	    {
		break;
	    }
	}

	if (tx == NULL)
	    continue;

	/*
	** This record belongs to one of our rollback transactions.
	**
	** If the previous record processed for this transaction was a CLR
	** then we want to jump backwards in the transaction chain to the
	** record which was compensated and start recovery processing at
	** the log record previous to it.
	**
	** We skip each log record for this transaction until we find the
	** one whose LSN matches the compensated LSN.
	**
	** ABORTSAVE log records are treated as a special CLR.
	** To eliminate the need to journal savepoint log records,
	** we search for a log record with an LSN previous to the
	** compensated_lsn noted in the ABORTSAVE log record.  Also, we
	** process that record rather than skipping it.
	*/
	if (tx->tx_status & RFP_TX_ABS_JUMP)
	{
	    if (LSN_LT(&record->lsn, &tx->tx_clr_lsn))
		tx->tx_status &= ~RFP_TX_ABS_JUMP;
	    else
		continue;
	}
	else if (tx->tx_status & RFP_TX_CLR_JUMP)
	{
	    /*
	    ** If we have found the compensated record, then turn off the
	    ** CLR_JUMP flag so that the next record for this transaction
	    ** will be processed.
	    */
	    if (LSN_LT(&record->lsn, &tx->tx_clr_lsn))
		tx->tx_status &= ~RFP_TX_CLR_JUMP;
	    else
	    {
		if (LSN_EQ(&record->lsn, &tx->tx_clr_lsn))
		    tx->tx_status &= ~RFP_TX_CLR_JUMP;
		continue;
	    }
	}

	if (jsx->jsx_status & JSX_TRACE)
	    dmd_log(FALSE, (PTR)record, rfp->jnl_block_size);

	/*
	** Look for special log records that cause us to jump back to a
	** particular spot in the log stream.  Set the CLR_JUMP flag that
	** will cause us to skip log records for this transaction until
	** we jump over those between this CLR and the record it compensates.
	*/
	if (record->flags & DM0L_CLR)
	{
	    STRUCT_ASSIGN_MACRO(record->compensated_lsn, tx->tx_clr_lsn);

	    if (record->type == DM0LABORTSAVE)
		tx->tx_status |= RFP_TX_ABS_JUMP;
	    else
		tx->tx_status |= RFP_TX_CLR_JUMP;

	    continue;
	}

	/* Increment count of records processed during undo */
	tx->tx_undo_cnt++;

	/*
	** Look for special log record types:
	**
	**     BT  - transaction abort completed.
	*/

	if (record->type == DM0LBT)
	{
	    tx->tx_status |= RFP_TX_COMPLETE;

	    if (rfp->rfp_redo_err)
	    {
		char	line_buffer[ER_MAX_LEN];
		char	date[32];

		TMstamp_str((TM_STAMP *)&(tx->tx_bt.bt_time), date);
		TRformat(dmf_diag_put_line, 0, line_buffer, sizeof(line_buffer),
		    "Rollback complete for transaction 0x%x%x with BT time %s redo %d undo %d\n",
		    tx->tx_bt.bt_header.tran_id.db_high_tran,
		    tx->tx_bt.bt_header.tran_id.db_low_tran,
		    date, tx->tx_process_cnt, tx->tx_undo_cnt);
	    }

	    /*
	    ** If this completes the rollback of the last incomplete
	    ** transaction, then we can stop journal processing!
	    */
	    *rollback_complete = TRUE;
	    for (tx = rfp->rfp_tx; tx != NULL; tx = tx->tx_next)
	    {
		if ((tx->tx_status & RFP_TX_COMPLETE) == 0)
		    *rollback_complete = FALSE;
	    }

	    if (*rollback_complete)
		break;

	    continue;
	}

	if (jsx->jsx_status2 & JSX2_INVALID_TBL_LIST)
	{
	    if (( rfp_filter_log_record( rfp, jsx, record )) == 0)
	    {
		/*
		** Not interested so skip this record
		*/
		continue;
	    }
	}

	/*
	** Apply Undo Recovery for this journal record.
	*/

	status = apply_j_undo(jsx, rfp, dcb, tx, dmve, record);

	if (status != E_DB_OK)
	{
	    /* Check if ok to continue */
	    dmfWriteMsg(&jsx->jsx_dberr, 0, 0);
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM1343_RFP_JNL_UNDO);
	    status = rfp_error( rfp, jsx, rfp->rfp_cur_tblcb,
			dmve, RFP_FAILED_TABLE, status );
	}

	if (status == E_DB_OK)
	{
	    CLRDBERR(&jsx->jsx_dberr);
	    continue;
	}

	SETDBERR(&jsx->jsx_dberr, 0, E_DM1343_RFP_JNL_UNDO);
	break;
    }

    if (status == E_DB_OK)
	return (E_DB_OK);

    uleFormat(&jsx->jsx_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
	(char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
    SETDBERR(&jsx->jsx_dberr, 0, E_DM1343_RFP_JNL_UNDO);
    return (E_DB_ERROR);
}

/*
** Name: apply_j_undo	- Perform Journal UNDO processing
**
** Description:
**	This routine takes a journal log record, formats a dmve control
**	block and calls to the undo recovery code to process it.
**
**	It is called during the undo phase of rollforward processing
**	when there are transactions found at the end of the normal journal
**	processing which were not rolled forward to completion.
**
** Inputs:
**	jsx		    - JSP context
**	rfp		    - Rollforward context
**	dcb		    - Open database control block
**	tx		    - Transaction context
**      dmve
**	record		    - Journal Log Record
**
** Outputs:
**	err_code	    - reason for error status
**
** Returns:
**	E_DB_OK
**	E_DB_ERROR
**
** History:
**	15-mar-1993 (rogerk)
**	    Created.
**	24-may-1993 (andys)
**	    Pass appropriate logfile blocksize to dmd_log and dmd_format_log
**	    rather than using hardwired 4096.
**	26-Jul-2007 (wanfr01)
**	    Bug 118854 
**	    Stop duplicate logging of jnl records when #x and -verbose are
**	    used together 
*/
static DB_STATUS
apply_j_undo(
DMF_JSX		*jsx,
DMF_RFP		*rfp,
DMP_DCB		*dcb,
RFP_TX		*tx,
DMVE_CB		*dmve,
DM0L_HEADER	*record)
{
    DB_STATUS	    status;
    char	    line_buffer[132];
    i4		    error;

    CLRDBERR(&jsx->jsx_dberr);

    /*
    ** Format the DMVE control block for the undo action.
    */
    dmve->dmve_type = DMVECB_CB;
    dmve->dmve_length = sizeof(DMVE_CB);
    dmve->dmve_ascii_id = DMVE_ASCII_ID;
    dmve->dmve_next = 0;
    dmve->dmve_prev = 0;
    dmve->dmve_owner = 0;
    dmve->dmve_action = DMVE_UNDO;
    dmve->dmve_logging = TRUE;
    dmve->dmve_flags = 0;
    CLRDBERR(&dmve->dmve_error);

    /*
    ** During rollforwards which are not being used to fully restore the
    ** database to the end of the journal stream we do not log the updates
    ** made to bring the database to a consistent state (by rolling back
    ** transactions not completed when rollforward processing stops).
    */
    if (rfp->rfp_status & RFP_NO_CLRLOGGING)
	dmve->dmve_logging = FALSE;

    STRUCT_ASSIGN_MACRO(tx->tx_bt.bt_header.tran_id, dmve->dmve_tran_id);
    dmve->dmve_dcb_ptr = dcb;
    dmve->dmve_lk_id = dmf_svcb->svcb_lock_list;
    dmve->dmve_log_id = tx->tx_id;
    dmve->dmve_db_lockmode = DM2D_X;

    dmve->dmve_lg_addr.la_sequence = 0;
    dmve->dmve_lg_addr.la_block    = 0;
    dmve->dmve_lg_addr.la_offset   = 0;
    dmve->dmve_log_rec = (PTR) record;

    if (rfp->rfp_jsx->jsx_status & (JSX_VERBOSE | JSX_TRACE))
    {
	dmd_format_dm_hdr(dmf_put_line, record,
	    line_buffer, sizeof(line_buffer));
	dmd_format_log(FALSE, dmf_put_line, (char *)record,
	    rfp->jnl_block_size,
	    line_buffer, sizeof(line_buffer));
    }

    /*
    ** Call recovery processing for this record.
    */
    status = dmve_undo(dmve);

    if (rfp->test_undo_err &&
	record->type == DM0LPUT &&
	MEcmp(dcb->dcb_name.db_db_name, "test_rfp_dmve_error ", 20) == 0)
    {
	SETDBERR(&dmve->dmve_error, 0, E_DM9601_DMVE_BAD_PARAMETER);
	status = E_DB_ERROR;
    }

    if (status != E_DB_OK)
    {
	uleFormat(&dmve->dmve_error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
	    (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	dmfWriteMsg(NULL, E_DM1343_RFP_JNL_UNDO, 0);
	SETDBERR(&jsx->jsx_dberr, 0, E_DM1344_RFP_APPLY_UNDO);
    }

    return (status);
}

/*
** Name: qualify_record	- Compare record timestamp with -b and -e flags.
**
** Description:
**	This routine is called when the user has specified -b and/or -e flags
**	to determine if the given record is inside the recovery window.
**
**	Only ET timestamps are used.
**
**	The routine sets the following status fields to direct the caller's
**	journal processing:
**
**	    RFP_BEFORE_BTIME   - turned off if record is found which exceeds
**				 or matches the user supplied begin time.
**	    RFP_AFTER_ETIME    - turned on if record is found which exceeds
**				 or matches the user supplied end time.
**	    RFP_EXACT_ETIME    - turned on if record is an ET record which
**				 exactly matches the user supplied end time.
**				 We will process the COMMIT and then stop.
**				 or matches the user supplied end time.
**	    RFP_BEFORE_BLSN    - turned off if record is found which exceeds
**				 or matches the user supplied starting lsn.
**	    RFP_AFTER_ELSN     - turned on if record is found which exceeds
**				 or matches the user supplied ending lsn.
**          RFP_EXACT_LSN      - turned on if record exactly matches either
**                               the start or end lsn.  We will process the
**				 record.
**
** Inputs:
**	jsx		    - JSP context
**	rfp		    - Rollforward context
**	record		    - Journal Log Record
**
** Outputs:
**	none
**
** Returns:
**	none
**
** History:
**	15-mar-1993 (rogerk)
**	    Created.
**	31-jan-1994 (jnash)
**	  - Fix bug where RFP_AFTER_ETIME untwiddled when then should have
**	    been twiddled.
**	  - Use only ET timestamps when comparing -b/-e times.  This
**	    agrees with AUDITDB, and provides predictable
**	    results when a rolldb -e follows a rolldb -b.
**	  - Eliminate RFP_EXACT_TIME maintenance.  The concept of
**	    an "exact match" in time is not supported by the current
**	    CL interface.
**	  - Add LSN to TRdisplay() when -b, -e found.
**	21-feb-1994 (jnash)
**	    Add -start_lsn, -end_lsn support, fix exact time logic.
**	20-aug-19978 (rigka01) - bug #84583
**          don't process record where BT > -e:
**          -e processing stops at the first transaction 
**          with a ET > -e.  In the situation where there are
**          no transactions overlapping the -e time,
**          a transaction with BT > -e time is processed.
**	    Added a check to avoid processing this transaction
**	    by comparing BT to -e  time and setting the
**          RFP_AFTER_ETIME flag in the status when BT>e.
**          if BT > e.
**	    Without this fix, rollforwarddb -e -norollback 
**          processes this transaction; rollforwarddb -e without 
**          -norollback processes and then rolls back this
**          transaction.  
**	23-feb-2004 (thaju02) (B111470/INGSRV2635)
**	    For -b option, determine if record qualifies using
**	    list of -b qualifying transactions. 
**	8-nov-2006 (kibro01) (b116556)
**	    Since the fractional seconds are displayed in the audit log,
**	    it should be possible to use them to select transactions when
**	    using rollforwarddb.
**                 
*/
static VOID
qualify_record(
DMF_JSX		    *jsx,
DMF_RFP		    *rfp,
DM0L_HEADER	    *record)
{
    DB_TAB_TIMESTAMP	*timestamp;
    char	    	line_buffer[132];
    u_char	    	*tmp_dot;
    char		tmp_time[TM_SIZE_STAMP];
    RFP_BQTX		*tx;
    DB_TRAN_ID		*tran_id = &record->tran_id;

    /*
    ** Qualify the record for -start_lsn and -end_lsn params.
    */
    if ((jsx->jsx_status1 & JSX_START_LSN) &&
	(rfp->rfp_status & RFP_BEFORE_BLSN) &&
	(LSN_GTE(&record->lsn, &jsx->jsx_start_lsn)))
    {
	TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
	    "%@ RFP: User supplied -start_lsn (%x,%x) encountered.\n",
	    record->lsn.lsn_high, record->lsn.lsn_low);

	rfp->rfp_status &= ~RFP_BEFORE_BLSN;

	if (LSN_EQ(&record->lsn, &jsx->jsx_start_lsn))
	    rfp->rfp_status |= RFP_EXACT_LSN;
    }

    /*
    ** If an exact LSN match, we come thru once more before
    ** finally ending the recovery.
    */
    if ((jsx->jsx_status1 & JSX_END_LSN) &&
	(LSN_GTE(&record->lsn, &jsx->jsx_end_lsn)))
    {
	rfp->rfp_status |= RFP_AFTER_ELSN;

	/*
	** Display message the last time thru this routine.
	*/
	if (LSN_EQ(&record->lsn, &jsx->jsx_end_lsn))
	    rfp->rfp_status |= RFP_EXACT_LSN;
	else
	    TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
		"%@ RFP: User supplied -end_lsn (%x,%x) encountered.\n",
		record->lsn.lsn_high, record->lsn.lsn_low);
    }

    /*
    ** Qualify the record for -e and -b flags.  We only use ET times.
    */
    switch (record->type)
    {
      case DM0LET:
	/*
	** Equality comparisons between times in log records
	** and ascii strings do not work unless ms part is zeroed
	** (which is actually a random number).
	** kibro01 - msecs part is at worst a a sequential number and is
	** displayed in the audit log, so should be usable to select
	** transactions (b116556)
	*/
	timestamp = &((DM0L_ET *)record)->et_time;
	break;

      case DM0LBT:
        /* 
        ** if BT > -e time then surely ET > -e time so
        ** let's avoid a rollback in this case and
        ** set frp_status to RFP_AFTER_ETIME
        */
	timestamp = &((DM0L_BT *)record)->bt_time;
	break;

      default:
	timestamp = NULL;
    }

    if (jsx->jsx_status & JSX_BTIME)
    {
	if ((record->type == DM0LBT) &&
	     TMcmp_stamp((TM_STAMP *)timestamp,
			(TM_STAMP *) &jsx->jsx_bgn_time) > 0)
	    rfp->rfp_status &= ~RFP_BEFORE_BTIME;
	else
	{
	
	    /* 
	    ** Using list of xactions which span the -b specified
	    ** (BT falls before -b and corresponding ET falls after -b),
	    ** check to see if this log record qualifies.
	    */
	    for (tx = rfp->rfp_bqtxh[tran_id->db_low_tran & (RFP_TXH_CNT - 1)];
	    	tx != NULL;
		tx = tx->tx_hash_next)
	    {
		if ((tx->tx_tran_id.db_high_tran == 
				tran_id->db_high_tran) &&
			(tx->tx_tran_id.db_low_tran  == 
				tran_id->db_low_tran))
		{
		    /* this transaction does qualify, process record */
		    rfp->rfp_status |= RFP_STRADDLE_BTIME;
		}
	    }
	}
    }

    /*
    ** If the -e flag was specified and this record exceeds
    ** that timestamp then halt journal processing.
    **
    ** If the timestamp matches EXACTLY, then we halt before
    ** processing the record so that the user can specify the
    ** the same timestamp on a subsequent -b flag.  But if the
    ** time is for an End Transaction record, then we go ahead
    ** and process the record before quitting.
    **
    ** Skip this check if this is the exact match case where we
    ** process one record beyond where we have really stopped.
    */
    if ((timestamp) &&
	(jsx->jsx_status & JSX_ETIME) &&
	((rfp->rfp_status & RFP_AFTER_ETIME) == 0) &&
	(TMcmp_stamp((TM_STAMP *)timestamp, (TM_STAMP *)&jsx->jsx_end_time)
	    >= 0))
    {
	/*
	** Save the LSN of this record, we'll use it during the UNDO
	** phase to skip journals records that we are not interested in.
	*/
	rfp->rfp_etime_lsn = record->lsn;
	rfp->rfp_status |= RFP_AFTER_ETIME;

	if ((TMcmp_stamp((TM_STAMP *)timestamp,
	    (TM_STAMP *)&jsx->jsx_end_time) == 0) &&
	    (record->type == DM0LET))
	{
	    rfp->rfp_status |= RFP_EXACT_ETIME;
	}

	TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
	    "%@ RFP: User supplied -e time encountered, LSN: (%x,%x).\n",
	    rfp->rfp_etime_lsn.lsn_high, rfp->rfp_etime_lsn.lsn_low);
    }

    return;
}

/*
** Name: process_et_action - process actions at end transaction time
**
** Description:
**	During DDL operations, temporary files are created which are left
**	around in case needed by UNDO actions.  These files need to be
**	eventually deleted.
**
**	When rollforward creates one of these delete files, it appends
**	an frename action to the rollforward action list (a delete file
**	is created by a file rename operation).
**
**	When the ET record is found for that transaction, this routine
**	is called to delete each file listed in the action list.
**
**	Any other actions associated with this transaction are also deleted.
**
**	If this routine encounters an error and cannot destroy the delete
**	files, it will log an error but will continue (the routine
**	returns void anyway).  The temporary file will likely be left
**	around after the rollforward, but should cause no real problems
**	(other than wasted disk space).  The spurious file can be cleaned
**	up by a later checkpoint or sysmod operation.
**
** Inputs:
**	rfp		    - Rollforward context
**	tx		    - Transaction context
**
** Outputs:
**	none
**
** Returns:
**	none
**
** History:
**	26-jul-1993 (rogerk)
**	    Created.
**	20-sep-1993 (rogerk)
**	    Merged with more generic action list handling.  Routine changed
**	    to loop through all pend-delete actions.  Also it now cleans
**	    up other actions pended by recovery routines.
**	31-jan-1994 (jnash)
**	    Also log the deletion of non-journaled tables.
**	22-aug-1995 (canor01)
**	    initialize loc.loc_status field before passing to
**	    dm2f_delete_file(), since this now tests this field for
**	    LOC_RAW, the raw location flag. (70728)
**	5-Apr-2004 (schka24)
**	    Clean up any rawdata that we've gathered up.
*/
static VOID
process_et_action(
DMF_RFP		    *rfp,
RFP_TX		    *tx)
{
    RFP_REC_ACTION	*act_ptr = NULL;
    DMVE_CB		*dmve = rfp->rfp_dmve;
    DMP_LOC_ENTRY       ext;
    DMP_FCB             fcb;
    DI_IO		di_file;
    DMP_LOCATION        loc;
    i4		error;
    i4		i;
    DB_STATUS		status;
    DB_ERROR		local_dberr;

    /*
    ** Look through the list of actions pended by this transaction.
    ** Process any FRENAME ones and discard the rest.
    */
    for (;;)
    {
	rfp_lookup_action(dmve, &tx->tx_bt.bt_header.tran_id, 
			(LG_LSN *)0, 0, &act_ptr);
	if (act_ptr == NULL)
	    break;

	if (act_ptr->rfp_act_action == RFP_FRENAME)
	{
	    STRUCT_ASSIGN_MACRO(act_ptr->rfp_act_p_name, ext.physical);
	    STRUCT_ASSIGN_MACRO(act_ptr->rfp_act_f_name, fcb.fcb_filename);
	    ext.phys_length = act_ptr->rfp_act_p_len;
	    fcb.fcb_namelength = act_ptr->rfp_act_f_len;
	    fcb.fcb_di_ptr = &di_file;
	    fcb.fcb_last_page = 0;
	    fcb.fcb_location = (DMP_LOC_ENTRY *) 0;
	    fcb.fcb_state = FCB_CLOSED;
	    fcb.fcb_tcb_ptr = NULL;
	    loc.loc_ext = &ext;
	    loc.loc_fcb = &fcb;
	    loc.loc_id = 0;
	    loc.loc_status = 0;

	    /*
	    ** Delete the file.
	    ** Ignore file-not-found errors: these files were probably
	    ** re-renamed by some other recovery action (the destroy was
	    ** undone).
	    */
	    status = dm2f_delete_file(dmf_svcb->svcb_lock_list, dmve->dmve_log_id,
			&loc, (i4) 1, (DM_OBJECT *)0, (i4)1, &local_dberr);
	    if ((status != E_DB_OK) && (local_dberr.err_code != E_DM9291_DM2F_FILENOTFOUND))
	    {
		uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
		uleFormat(NULL, E_DM134A_RFP_SPURIOUS_FILE, (CL_ERR_DESC *)NULL, ULE_LOG,
		    NULL, (char *)NULL, (i4)0, (i4 *)NULL, &error, 2,
		    sizeof(DM_PATH), &act_ptr->rfp_act_p_name,
		    sizeof(DM_FILE), &act_ptr->rfp_act_f_name);
	    }
	}
	else if ((act_ptr->rfp_act_action == RFP_DELETE) &&
		 (act_ptr->rfp_act_tabid.db_tab_base == DM_1_RELATION_KEY) &&
		 (act_ptr->rfp_act_tabid.db_tab_index == 0))
	{
	    /*
	    ** Write a warning noting that a non-journaled table
	    ** has been dropped by rollforwarddb.  If the user really
	    ** has a problem with this, he can do a rolldb -e to a
	    ** point just prior to the drop, or can do a ckpdb -j and
	    ** unload the table, reloading it after rollfoward is complete.
	    **
	    ** Note that views, grants, etc, are journaled.
	    */
	    uleFormat(NULL, W_DM134D_RFP_NONJNL_TAB_DEL,
		(CL_ERR_DESC *)NULL, ULE_LOG, NULL, (char *)NULL, (i4)0,
		(i4 *)NULL, &error, 3,
		sizeof(DB_DB_NAME), (PTR)dmve->dmve_dcb_ptr->dcb_name.db_db_name,
		sizeof(DB_TAB_NAME), (PTR) act_ptr->rfp_act_tabname.db_tab_name,
		sizeof(DB_OWN_NAME), (PTR) act_ptr->rfp_act_tabown.db_own_name);
	}

	rfp_remove_action(dmve, act_ptr);
    }

    /* If we gathered up any rawdata info, it's of no further use at
    ** transaction end -- delete it.
    */
    delete_rawdata(tx);

}

/*{
** Name: gather_rawdata - Gather up rawdata info.
**
** Description:
**	Some of the data needed for operations like repartitioning MODIFY
**	is of indefinite length, and doesn't fit into the MODIFY log
**	record.  So, these structures are logged as RAWDATA log records,
**	preceding the MODIFY that needs them.  Multiple RAWDATA records
**	can be used to log a very large data structure; each RAWDATA
**	has the total size as well as the offset of the current piece.
**	This routine is used to gather up RAWDATA stuff as it arrives.
**
**	Each RAWDATA is stamped with a "type", which allows a variety of
**	objects to be saved up (a partition definition and a ppchar array,
**	for instance.)  Only one of each can currently be held, although
**	there's support in the log record for multiple instances.
**	For now, if we see the start of an object of type X, and we're
**	already holding an X, we'll assume that the old X is no longer
**	needed;  it's thrown away and a new X is started.
**
**	Since there might be multiple concurrent transactions recorded
**	in the journals, each one writing RAWDATA records, the TX info
**	block is used to point to and track the raw data.
**
** Inputs:
**	tx			Transaction info block
**	record			Pointer to RAWDATA log record.
**	err_code		An output
**
** Outputs:
**	Returns E_DB_xxx status
**	err_code		Specific failure reason returned here
**
** Side effects:
**	raw data info is built up, pointed to by tx info
**
** History:
**	5-Apr-2004 (schka24)
**	    Written.
*/

static DB_STATUS
gather_rawdata(DMF_JSX *jsx, RFP_TX *tx, DM0L_RAWDATA *record)
{

    DB_STATUS status;			/* The usual call status */
    i4 ix;				/* Rawdata type index */
    i4 toss_error;
    i4	error;

    CLRDBERR(&jsx->jsx_dberr);

    ix = record->rawd_type;

    /* If this is the first piece, deallocate any existing memory,
    ** and allocate a holding area for the raw data.  DMF likes its
    ** little header so allocate it as a MISC_CB.
    */
    if (record->rawd_offset == 0)
    {
	if (tx->tx_rawdata[ix] != NULL)
	    dm0m_deallocate((DM_OBJECT **) &tx->tx_rawdata[ix]);
	status = dm0m_allocate(sizeof(DMP_MISC) + record->rawd_total_size,
			0, (i4) MISC_CB, (i4) MISC_ASCII_ID,
			(char *) tx,
			(DM_OBJECT **) &tx->tx_rawdata[ix],
			&jsx->jsx_dberr);
	if (DB_FAILURE_MACRO(status))
	    return (status);
	tx->tx_rawdata_size[ix] = record->rawd_total_size;
	tx->tx_rawdata[ix]->misc_data =
		(char *) ((PTR) tx->tx_rawdata[ix] + sizeof(DMP_MISC));
    }
    /* Copy in this piece */
    if (record->rawd_offset + record->rawd_size > tx->tx_rawdata_size[ix])
    {
	uleFormat(NULL, E_DM1323_RFP_BAD_RAWDATA, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		NULL, (i4)0, NULL, &toss_error, 4,
		0, ix, 0, record->rawd_offset, 0, record->rawd_size,
		0, tx->tx_rawdata_size[ix]);
	/* Return a parameter-less error code for caller */
	SETDBERR(&jsx->jsx_dberr, 0, E_DM1306_RFP_APPLY_RECORD);
	return (E_DB_ERROR);
    }
    MEcopy( (PTR)record + sizeof(DM0L_RAWDATA),
		record->rawd_size,
		tx->tx_rawdata[ix]->misc_data + record->rawd_offset);
    return (E_DB_OK);

} /* gather_rawdata */

/*{
** Name: delete_rawdata
**
** Description:
**	This routine deletes any rawdata objects that are attached to
**	the passed-in TX data block, and nulls out the rawdata pointers.
**
**	This might be done after a log record that uses rawdata (such
**	as MODIFY), or at the end of a transaction (in case nobody used
**	the rawdata objects).
**
** Inputs:
**	tx			TX data block
**
** Outputs:
**	None (void return)
**
** History:
**	6-Apr-2004 (schka24)
**	    Written.
*/

static void
delete_rawdata(RFP_TX *tx)
{
    i4 i;

    for (i = 0; i < DM_RAWD_TYPES; ++i)
    {
	if (tx->tx_rawdata[i] != NULL)
	{
	    /* nulls out the pointer too */
	    dm0m_deallocate((DM_OBJECT **) &tx->tx_rawdata[i]);
	    tx->tx_rawdata_size[i] = 0;
	}
    }
} /* delete_rawdata */

/*{
** Name: rfp_validate_cl_options - Validates command line options passed
**                                 in.
**
**
** Description:
**
** Inputs:
**      jsx                             Pointer to checkpoint context.
**
** Outputs:
**      err_code                        Reason for error return status.
**      Returns:
**          E_DB_OK
**          E_DB_ERROR
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**	14-sep-1994 (andyw)
**	    Partial Backup/Recovery Project:
**              Created for table level rollforwarddb.
**      22-nov-1994 (juliet)
**          Added ule_format calls in order to report
**          invalid database level recovery options.  Added
**          ule_format call and set status in order to report
**          if the table attempting to be recovered is a
**          core DBMS catalog.  Added ule_format call to
**          report invalid argument.  Created error messages
**          RFP_INV_ARG_FOR_DB_RECOVERY,
**          RFP_NO_CORE_TBL_RECOVERY,
**          RFP_INVALID_ARGUMENT.
*/
DB_STATUS
rfp_validate_cl_options(
DMF_JSX         *jsx,
DMF_RFP         *rfp,
DMP_DCB	        *dcb)
{
    DB_STATUS           status = E_DB_OK;
    char                error_buffer[ER_MAX_LEN];
    i4             error_length;
    char	   *invalid_opt;
    i4			error;

    CLRDBERR(&jsx->jsx_dberr);

    if ( jsx->jsx_status1 & JSX1_RECOVER_DB )
    {
        /*
        ** The following options are invalid for database level recovery
        **    "-force_logical_consistent"
        **    "-online"
        **    "-noblobs"
        **    "-nosecondary_index"
        */
        if ( jsx->jsx_status1 & JSX1_F_LOGICAL_CONSIST )
        {
            uleFormat(NULL, E_DM1354_RFP_INV_ARG_FOR_DB_RECOVERY,
                        (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
                        error_buffer, ER_MAX_LEN, &error_length,
                        &error, 1, 24, "force_logical_consistent");
	    dmf_put_line(0, error_length, error_buffer);
            status = E_DB_ERROR;
        }

        if ( jsx->jsx_status1 & JSX1_ONLINE )
        {
            uleFormat(NULL, E_DM1354_RFP_INV_ARG_FOR_DB_RECOVERY,
                        (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
			error_buffer, ER_MAX_LEN, &error_length,
                        &error, 1, 6, "online");
	    dmf_put_line(0, error_length, error_buffer);
            status = E_DB_ERROR;
        }

        if ( jsx->jsx_status1 & JSX1_NOBLOBS )
        {
            uleFormat(NULL, E_DM1354_RFP_INV_ARG_FOR_DB_RECOVERY,
                        (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
			error_buffer, ER_MAX_LEN, &error_length,
                        &error, 1, 7, "noblobs");
	    dmf_put_line(0, error_length, error_buffer);
            status = E_DB_ERROR;
        }

        if ( jsx->jsx_status1 & JSX1_NOSEC_IND )
        {
            uleFormat(NULL, E_DM1354_RFP_INV_ARG_FOR_DB_RECOVERY,
                        (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
                        error_buffer, ER_MAX_LEN, &error_length,
                        &error, 1, 17, "nosecondary_index");
	    dmf_put_line(0, error_length, error_buffer);
            status = E_DB_ERROR;
        }

    }

    if (jsx->jsx_status2 & JSX2_INCR_JNL)
    {
        /*
        ** The following options are invalid for incremental ecovery
	**    -table
	**    +c +j
	**    For now, #c
        */
	invalid_opt = NULL;
	if (( jsx->jsx_status1 & JSX1_RECOVER_DB ) == 0)
	    invalid_opt = "table ";
	else if ((jsx->jsx_status & JSX_USECKP) 
		&& (jsx->jsx_status & JSX_USEJNL))
	    invalid_opt = "+c +j ";
	else if ((jsx->jsx_status & JSX_USEJNL)
		&& (jsx->jsx_status & JSX_NORFP_UNDO) == 0 
		&& (jsx->jsx_status2 & JSX2_RFP_ROLLBACK) == 0)
	{
	    /* MUST specify what to do at end of incremental jnl processing */
	    invalid_opt = "-rollback OR -norollback required";
	}
	else if (jsx->jsx_status & JSX_CKP_SELECTED)
        {
	    invalid_opt = "#c";
	}

	if (invalid_opt)
	{
	    uleFormat(NULL, E_DM1373_RFP_INCR_ARG,
                        (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
                        error_buffer, ER_MAX_LEN, &error_length,
                        &error, 1, STlength(invalid_opt), invalid_opt);
	    dmf_put_line(0, error_length, error_buffer);
	    status = E_DB_ERROR;
	}
    }
    if ( status != E_DB_OK )
    {
        dmfWriteMsg(NULL, E_DM1356_RFP_INVALID_ARGUMENT, 0);
    }
    return( status );
}

/*{
** Name: rfp_prepare_db_recovery        - Prepare to rollforward a database.
**
** Description:
**      This routine opens that database reads the configuration files
**      and prepares for the rest of the processing.
**
**      Among the interesting things that this routine does is to rename
**      the database configuration file "aaaaaaaa.cnf" to "aaaaaaaa.rfc". This
**      is done to enable us to restart the roll-forward process in case
**      the restore doesn't work. We are GUARANTEED that "aaaaaaaa.rfc" does
**      NOT exist in the CK save-set, and so the restore process does not
**      touch it and a restarted roll-forward can find the .rfc file, figure
**      out that it has been restarted, and take appropriate action. Note that
**      it is VERY important that we rename this file as soon as we start
**      running roll-forward, because restore_ckp() will overwrite this file
**      when it runs. We want to preserve the contents of the file at the
**      start of the roll-forward process.
**
**      If things are badly enough screwed up, it may be the case that when
**      we go to rollforward this database, we cannot even successfully find
**      and open the config file "aaaaaaaa.cnf". For example, the user may
**      have accidentally deleted EVERY file in the database directory! (This
**      is not as absurd as it seems -- if the physical disk fails and a new
**      one must be formatted, the odds are that the new one will be formatted
**      clean, containing no config file or other file.)
**
**      In this case, we make an attempt to recover by looking in the II_DUMP
**      directory for a "aaaaaaaa.cnf" file. The config file is copied there
**      frequently and so the odds are good that we can find one there. If
**      one is there, we copy it over to the database directory and proceed
**      with the rollforward.
**
** Inputs:
**      jsx                             Pointer to journal support context.
**      dcb                             Pointer to DCB for database.
**
** Outputs:
**      rfp                             Rollforward context.
**      err_code                        Reason for error code.
**      Returns:
**          E_DB_OK
**          E_DB_ERROR
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**	14-sep-1994 (andyw)
**	    Partial Backup/Recovery Project:
**              Created for Table Level Rollforwarddb.
**      24-jan-1994 (stial01)
**          BUG 66473: Disallow db rollforward from table checkpoint.
**          ckp_mode is now a bit pattern.
**	 3-jul-96 (nick)
**	    Fix error handling.
**	 20-feb-1997(angusm)
**	    save existing data locations so that if they were added
**	    AFTER this checkpoint, the database config file is still
**	    in step with information in 'iidbdb' (bug 49057)
**	02-apr-1997 (shero03)
**	    B80384: allocate the correct amount for the locations
*/
static DB_STATUS
rfp_prepare_db_recovery(
DMF_RFP             *rfp,
DMF_JSX             *jsx,
DMP_DCB             *dcb)
{
    DM0C_CNF            *cnf;
    DB_STATUS           status;
    DI_IO               di_file;
    CL_ERR_DESC         sys_err;
    i4             open_flags;
    i4             dmp_history_no;
    i4             jnl_history_no;
    i4             rfc_exists = TRUE;
    i4		size;
    bool                open_config = FALSE;
    PTR			p, p1;
    i4             i;
    i4			error;
    DB_ERROR		local_dberr;

    CLRDBERR(&jsx->jsx_dberr);

    do
    {
	status = rfp_init_tbl_hash(rfp);
	if (status != E_DB_OK)
	    break;

        /*
        ** Lock the database.
        */
        open_flags = (DM2D_JLK | DM2D_RFP);
        if ((jsx->jsx_status & JSX_WAITDB) == 0)
            open_flags |= DM2D_NOWAIT ;

        status = dm2d_open_db( dcb, DM2D_A_WRITE, DM2D_X,
                               dmf_svcb->svcb_lock_list,
                               open_flags, &jsx->jsx_dberr);
        if (status != E_DB_OK)
        {
            if (jsx->jsx_dberr.err_code == E_DM004C_LOCK_RESOURCE_BUSY)
            {
		SETDBERR(&jsx->jsx_dberr, 0, E_DM1315_RFP_DB_BUSY);
                return (E_DB_INFO);
            }
            uleFormat(&jsx->jsx_dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL, &error, 0);
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM1300_RFP_OPEN_DB);
            break;
        }

        rfp->open_db |= DB_LOCKED;

        /*
        ** Database level recovery, open the configuration file in
        ** the following order.
        ** a) The .RFC in the database root directory, if fail
        ** b) The .CNF in the database root directory, if fail
        ** c) Copy the backup copy from the DUMP and then open, if fail
        **    recovery not possible.
        */
        status = config_handler(jsx, OPENRFC_CONFIG, dcb, &cnf);
        if (status == E_DB_INFO)
        {
            rfc_exists = FALSE;
            status = config_handler(jsx, OPEN_CONFIG, dcb, &cnf);
            if (status != E_DB_OK)
            {
                status = dm0d_config_restore( dcb, (i4)0, (i4)0,
                                                &jsx->jsx_dberr );
                if (status == E_DB_OK)
                    status = config_handler(jsx, OPEN_CONFIG, dcb, &cnf);
            }
        }

        if (status == E_DB_ERROR)
        {
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM130A_RFP_NO_CNFRFC);
            break;
        }

        open_config = TRUE;

        if (jsx->jsx_status & JSX_TRACE)
        {
            TRdisplay("%@ prepare: Open Config file (1)\n");
            dump_cnf(cnf);
        }

        status = rfp_prepare_validate_cnf_info( jsx, dcb, cnf );
        if ( status != E_DB_OK )
            break;

        /*
        ** Determine which checkpoint we will roll-forward from.
        */
        status = get_ckp_sequence( jsx, cnf, &jnl_history_no, &dmp_history_no );
        if (status != E_DB_OK)
            break;

	/*
	** BUG 66473:
	** Make sure checkpoint selected is a database checkpoint
	*/
	if ( ((jnl_history_no >= 0) &&
	      (cnf->cnf_jnl->jnl_history[jnl_history_no].ckp_mode & CKP_TABLE))
	  || ((dmp_history_no >= 0) && 
	      (cnf->cnf_dmp->dmp_history[dmp_history_no].ckp_mode & CKP_TABLE)))
	{
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM1361_RFP_NO_DB_RECOV_TBL_CKPT);
	    status = E_DB_ERROR;
	    break;
	}


        /*
        ** Save the journal and dump information as the CNF file will
        ** be overwritten once we restore the checkpoint
        */
        STRUCT_ASSIGN_MACRO(*cnf->cnf_jnl, rfp->old_jnl);
        STRUCT_ASSIGN_MACRO(*cnf->cnf_dmp, rfp->old_dmp);

        /* Save copy of current data locations  - bug 49057 */
	for (i=0; i < cnf->cnf_dsc->dsc_ext_count; i++)
        {
		if (cnf->cnf_ext[i].ext_location.flags & (DCB_ROOT))
			continue;
		if (cnf->cnf_ext[i].ext_location.flags & (DCB_DATA))
		    rfp->rfp_db_loc_count++;
	}
        /* allocate space */		/* B80384 */
        size = (rfp->rfp_db_loc_count * sizeof(DM0C_EXT))
		+ sizeof(DM_OBJECT); 
        status = dm0m_allocate((i4)size, 0, (i4)DM0C_T_EXT,	
		(i4)EXT_ASCII_ID, (char *)rfp,
		(DM_OBJECT **)&p, &jsx->jsx_dberr);
        if (status != E_DB_OK)
        {
            uleFormat(&jsx->jsx_dberr, 0, NULL, ULE_LOG, 
		NULL, NULL, 0, NULL, &error, 0);
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM1304_RFP_MEM_ALLOC);
            break;
        }     
	rfp->rfp_db_ext = (DM0C_EXT *)(p + sizeof(DM_OBJECT));

        /* copy from cnf  */
	p1=(PTR)rfp->rfp_db_ext;
	for (i=0; i < cnf->cnf_dsc->dsc_ext_count; i++)
	{
		if (cnf->cnf_ext[i].ext_location.flags & (DCB_ROOT))
			continue;
		if (cnf->cnf_ext[i].ext_location.flags & DCB_DATA)
		{
        	    MEcopy((PTR)&cnf->cnf_ext[i], sizeof(DM0C_EXT), p1);
		    p1 += sizeof(DM0C_EXT);
                }
	}
        /*
        ** Return the database status and last table id to caller.
        ** db_status is used to check journal and dump requirements as well as
        ** to restore current config information into the saveset config file.
        ** last_tbl_id is used to restore the table id to what it
        ** was prior to the failure.
        */
        rfp->db_status = cnf->cnf_dsc->dsc_status;
        rfp->last_tbl_id = cnf->cnf_dsc->dsc_last_table_id;

        /*
        ** If not a restart the see if need to drain the journal files
	** Do NOT try to drain the journals for incremental rollforwarddb
	** as this will try to create a journal file
        */
	if ( rfc_exists == FALSE )
        {
            status = rfp_prepare_drain_journals( jsx, dcb, &cnf );
            if ( status != E_DB_OK )
            {
                open_config = FALSE;
                break;
            }
        }

        /*  Get the interesting information. Note that since jnl_history_no
        **  may not be the same as jnl_count-1, we set the last journal history
        **  number to be the highest journal file history number for this
        **  database. We'll roll forward each history from rfp->jnl_first
        **  through rfp->jnl_last.
        **
        ** XXXX No longer true XXX
        ** If they're not rolling back to a checkpoint,
        **  (-c +j, say), then jnl_history_no is -1, which means we should
        **  only apply the journals since the most recent checkpoint.Bug #33422.
        ** XXXX No longer true XXX
        ** Now, get_ckp_sequence() returns a set of journals for -c as well.
        */

        status = rfp_prepare_get_ckp_jnl_dmp_info( rfp, jsx, cnf,
                                   jnl_history_no, dmp_history_no );
        if ( status != E_DB_OK )
            break;


        /*
        ** Close configuration file keeping the extent information.
        */
        status = config_handler(jsx, CLOSEKEEP_CONFIG, dcb, &cnf);
        if (status != E_DB_OK)
            return (status);

        open_config = FALSE;

        /*
        ** Now we rename the .cnf file to have a .rfc extension (".rfc" ==
        ** Roll Forward Copy). This is done to enable us to restart a
        ** failed roll-forward and be able to automatically detect that a
        ** previous rollfoward had been partially run. If we are restarted,
        ** we will read from the .rfc file, not from the .cnf file, and we
        ** will skip this renaming step (since rfc_exists will be TRUE).
        */

        if ((rfc_exists == FALSE) &&
            ((jsx->jsx_status & JSX_USECKP) != 0))
        {
            status = DIrename(&di_file, (char *)&dcb->dcb_root->physical,
                dcb->dcb_root->phys_length, "aaaaaaaa.cnf", 12,
                "aaaaaaaa.rfc", 12, &sys_err);
            if (status != OK)
            {
                uleFormat(NULL, E_DM9009_BAD_FILE_RENAME, &sys_err, ULE_LOG, NULL,
                    NULL, 0, NULL, &error, 4,
                    sizeof(dcb->dcb_name), dcb->dcb_name.db_db_name,
                    dcb->dcb_root->phys_length, &dcb->dcb_root->physical,
                    12, "aaaaaaaa.rfc",
                    12, "aaaaaaaa.cnf");

		SETDBERR(&jsx->jsx_dberr, 0, E_DM130B_RFP_RENAME_CNF);
                return (E_DB_ERROR);
            }

            if (jsx->jsx_status & JSX_TRACE)
                TRdisplay("%@ RFP:Rename config file .cnf to .rfc.\n");
        }

        status = E_DB_OK;

    } while (FALSE);

    if (open_config)
    {
        DB_STATUS    lstatus;

	local_dberr = jsx->jsx_dberr;

        lstatus = config_handler(jsx, CLOSE_CONFIG, dcb, &cnf);
        if ( lstatus != E_DB_OK )
        {
            uleFormat(&jsx->jsx_dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL, &error, 0);
        }
	jsx->jsx_dberr = local_dberr;
    }

    return( status );
}

/*{
** Name: rfp_prepare_validate_cnf_info  - Validate rollforward info from
**                                        the cnf file.
**
** Description:
**
** Inputs:
**      jsx                             Pointer to journal support context.
**      dcb                             Pointer to DCB for database.
**      cnf                             Poiner to CNF file.
**
** Outputs:
**      err_code                        Reason for error code.
**      Returns:
**          E_DB_OK
**          E_DB_ERROR
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**	14-sep-1994 (andyw)
**	    Partial Backup/Recovery Project:
**              Created for Table Level Rollforwarddb.
**	24-may-96 (nick)
**	    Don't close the config file if the database isn't checkpointed
**	    as a) the caller should do this and b) we need **cnf as a 
**	    parameter to do this and not *cnf.  This function isn't
**	    supposed to have any side effects. #76683
**	24-oct-96 (nick)
**	    Ignore some validation if we specified '-j'. #75986
*/
static DB_STATUS
rfp_prepare_validate_cnf_info(
DMF_JSX             *jsx,
DMP_DCB             *dcb,
DM0C_CNF            *cnf)
{

    DB_STATUS    status = E_DB_OK;
    char         error_buffer[ER_MAX_LEN];
    char         line_buffer[132];
    i4      error_length;
    i4      count;
    i4		 error;

    CLRDBERR(&jsx->jsx_dberr);

    /*
    ** If there is no checkpoint, return error.
    */
    if (cnf->cnf_jnl->jnl_ckp_seq == 0 && cnf->cnf_dmp->dmp_ckp_seq == 0)
    {
	SETDBERR(&jsx->jsx_dberr, 0, E_DM1316_RFP_NO_CHECKPOINT);
        return (E_DB_ERROR);
    }

    if ((jsx->jsx_status2 & JSX2_INCR_JNL) && (jsx->jsx_status & JSX_USEJNL)
	    && ((cnf->cnf_dsc->dsc_status & DSC_INCREMENTAL_RFP) == 0) )
    {
	/*
	** First incremental rfp should be +c -j, this sets DSC_INCREMENTAL_RFP
	** Disallow -incremental -c +j if  DSC_INCREMENTAL_RFP is not set.
	*/
	uleFormat(NULL, E_DM1373_RFP_INCR_ARG, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
                        error_buffer, ER_MAX_LEN, &error_length,
                        &error, 1, 20, "-c +j (before +c -j)");
	SETDBERR(&jsx->jsx_dberr, 0, E_DM1356_RFP_INVALID_ARGUMENT);
	return (E_DB_ERROR);
    }

    /*
    ** Check for journal disabled status of the database.
    ** Give a warning message if the user is rolling forward a
    ** journal_disabled database.
    **
    ** If they have specified the 'force' flag to allow them to
    ** rollforward the database, then continue forward, otherwise
    ** return with an error.
    */

    if ((jsx->jsx_status & JSX_USEJNL) == 0)
    {
	return(status);
    }
    else if (cnf->cnf_dsc->dsc_status & DSC_DISABLE_JOURNAL)
    {
        /*
        ** Give warning - write to error log if we are really going
        ** to do the rollforward ('force' flag specified) and to
        ** the terminal in any case.
        */
        uleFormat(NULL, E_DM93A0_ROLLFORWARD_JNLDISABLE, (CL_ERR_DESC *)NULL,
                ULE_LOOKUP, (DB_SQLSTATE *)NULL,
                error_buffer, sizeof(error_buffer), &error_length, &error, 1,
                sizeof(dcb->dcb_name.db_db_name), dcb->dcb_name.db_db_name);

        error_buffer[error_length] = EOS;
        TRformat( dmf_put_line, 0, line_buffer, sizeof(line_buffer),
                  "%s\n", error_buffer);

        error_buffer[error_length] = EOS;
        SIwrite(error_length, error_buffer, &count, stdout);
        SIwrite(1, "\n", &count, stdout);
        SIflush(stdout);

        if ((jsx->jsx_status1 & JSX_FORCE) == 0)
        {
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM1301_RFP_ROLL_FORWARD);
            return (E_DB_ERROR);
        }

        uleFormat( NULL, 0, (CL_ERR_DESC *)NULL, ULE_MESSAGE, (DB_SQLSTATE *)NULL,
                    error_buffer, error_length, &error_length, &error, 0);
    }
    else if ((cnf->cnf_jnl->jnl_first_jnl_seq == 0) &&
                 ((cnf->cnf_dsc->dsc_status & DSC_JOURNAL) != 0))
    {
        /*
        ** The database is journaled but jnl_first_jnl_seq indicates that
        ** journals are invalid.
        **
        ** Give warning - write to error log if we are really going
        ** to do the rollforward ('force' flag specified) and to
        ** the terminal in any case.
        */
        uleFormat(NULL, E_DM134F_RFP_INV_JNL, (CL_ERR_DESC *)NULL,
                   ULE_LOOKUP, (DB_SQLSTATE *)NULL,
                   error_buffer, sizeof(error_buffer), &error_length,
                   &error, 1,
                   sizeof(dcb->dcb_name.db_db_name), dcb->dcb_name.db_db_name);

        error_buffer[error_length] = EOS;
        SIwrite(error_length, error_buffer, &count, stdout);
        SIwrite(1, "\n", &count, stdout);
        SIflush(stdout);

        if ((jsx->jsx_status1 & JSX_FORCE) == 0)
        {
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM1301_RFP_ROLL_FORWARD);
            return (E_DB_ERROR);
        }

        uleFormat(NULL, 0, (CL_ERR_DESC *)NULL, ULE_MESSAGE, (DB_SQLSTATE *)NULL,
                error_buffer, error_length, &error_length, &error, 0);
    }

    return( status );
}

/*{
** Name: rfp_prepare_drain_journals - Drain the journal file.
**
** Description:
**
**
** Inputs:
**	jsx				Pointer to journal support context.
**	dcb				Pointer to DCB for database.
**	cnf				Poiner to CNF file.
**
** Outputs:
**      err_code                        Reason for error code.
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	Any failure causes the config file to be closed.
**
** History:
**	14-sep-1994 (andyw)
**	    Partial Backup/Recovery Project:
**              Created.
**	20-dec-1994 (shero03)
**	    b65976 - Use the input config variable rather than
**	    opening and closing the config file.
**	26-feb-1996 (nick)
**	    Remove bogus check for journal disabled - this is in the 
**	    checkpoint code from which this function materialised and
**	    is of no relevance here.
**      03-nov-2006 (horda03) Bug 116231
**          Drain journals if the DB is marked DSC_DISABLE_JOURNAL as
**          we still need to merge .jxx files on a clustered installation.
*/
static DB_STATUS
rfp_prepare_drain_journals(
DMF_JSX		    *jsx,
DMP_DCB		    *dcb,
DM0C_CNF            **config)
{
    bool                consistent = TRUE;
    char                line_buffer[132];
    DB_STATUS		status;
    i4			error;
    DB_ERROR		local_dberr;

    CLRDBERR(&jsx->jsx_dberr);

    /*
    ** Check if an incremental rollforwarddb is in progress
    ** If so, the database is effectively readonly and it is not necessary 
    ** to drain the journals
    ** (we only need to do it before the first incremental rollfowarddb)
    */
    if ((*config)->cnf_dsc->dsc_status & (DSC_JOURNAL | DSC_DISABLE_JOURNAL)
	&& ((*config)->cnf_dsc->dsc_status & DSC_INCREMENTAL_RFP) == 0)
    {
	TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
	"%@ RFP: The Archiver must drain any active log records\n");
	TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
	"%28* : from the Log File to the Journal Files before\n");
	TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
	"%28* : Rollforward can proceed.  If the Archiver is\n");
	TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
	"%28* : not running this program will not continue.\n");
	TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
	"%28* : If this is the case, exit this program and have\n");
	TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
	"%28* : the appropriate person restart the archiver.\n");

	status = config_handler(jsx, CLOSE_CONFIG, dcb, &*config);
	if (status != E_DB_OK)
	    return( E_DB_ERROR );

	status = dm0l_drain(dcb, &jsx->jsx_dberr);
	if (status == E_DB_INFO)
	{
	    consistent = FALSE;
	   /* status = E_DB_OK; */
	}
        else if (status != E_DB_OK)
	{
	    uleFormat( &jsx->jsx_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		        (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM1314_RFP_DRAIN_ERROR);
	    return (E_DB_ERROR);
	}

	status = config_handler(jsx, OPEN_CONFIG, dcb, &*config);
        if ( status != E_DB_OK )
	    return( E_DB_ERROR );

	if ((jsx->jsx_status & JSX_VERBOSE) && (consistent == FALSE))
        {
	    TRformat( dmf_put_line, 0, line_buffer, sizeof(line_buffer),
	              "%@ RFP: Found Inconsistent database: %t\n",
	              sizeof(dcb->dcb_name), dcb->dcb_name.db_db_name);
        }

	/*
	** Check if incremental rollforwarddb is in progress
	** If it is, return without calling dm0j_merge 
	** 
	** Non-cluster installation
	**    dm0j_merge from CKPDB may need to create an empty jnl file
	**    if no jnl files have been created at the time of the CKPDB.
	**    However dm0j_merge is not necessary during rollforwarddb
	**    and for rollfowarddb of first incremental journal, dm0j_merge
	**    will incorrectly create a new jnl (because f_jnl = 0)
	**    Return without calling dm0j_merge
	** 
	** Cluster installation
	**    Currently rollforwarddb -incremental -c +j looks for
	**    STANDARD jnls not NODE jnls, however...
	**       dm0j_merge requires NEW node jnl history in config 
	**       dm0j_merge adds new (merged) jnls to the config 
	**    so for now dm0j_merge should NOT be done during incremental rfp,
	**    and you must ckpdb to get NODE jnls merged into STANDARD jnls
	*/
	if (jsx->jsx_status2 & JSX2_INCR_JNL)
	{
	    return (E_DB_OK);
	}

	if ( (CXcluster_enabled()) ||
	     ((*config)->cnf_jnl->jnl_history[(*config)->cnf_jnl->jnl_count-1].ckp_f_jnl
						== 0))
	{
	    /*
	    ** Merge the VAXcluster journals into one .JNL file.
	    ** We only need merge the journals from the most recent history
	    ** since all previous histories were merged by DMFCPP when it
	    ** began that checkpoint. This routine may also be called in
	    ** the non-cluster case, in which case it merely updates the
	    ** journal history and returns.
	    */
	    status = dm0j_merge( *config, (DMP_DCB *)0, (PTR)0,
	                         (DB_STATUS (*)())0, &jsx->jsx_dberr);

	    if (status != E_DB_OK)
            {
		DB_STATUS	lstatus;
		
		local_dberr = jsx->jsx_dberr;

	        lstatus = config_handler( jsx, CLOSE_CONFIG, dcb, &*config );
		jsx->jsx_dberr =  local_dberr;

	        return( E_DB_ERROR );
            }
	}
    }
    return( E_DB_OK );
}

/*{
** Name: rfp_prepare_get_ckp_jnl_dmp - Determine the checkpoint, journal
**				       and dump information.
**
** Description:
**
**
** Inputs:
**	jsx				Pointer to journal support context.
**	dcb				Pointer to DCB for database.
**	cnf				Poiner to CNF file.
**
** Outputs:
**	rfp				Rollforward context.
**      err_code                        Reason for error code.
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	Any failure causes the config file to be closed.
**
** History:
**	14-sep-1994 (andyw)
**	    Partial Backup/Recovery Project:
**		Created.
**      24-jan-1995 (stial01)
**          Bug 66473: ckp_mode is now bit pattern.
*/
static DB_STATUS
rfp_prepare_get_ckp_jnl_dmp_info(
DMF_RFP		    *rfp,
DMF_JSX             *jsx,
DM0C_CNF            *cnf,
i4		    jnl_history_no,
i4		    dmp_history_no)
{
    char                line_buffer[132];
    i4		i;

    CLRDBERR(&jsx->jsx_dberr);

    if ( jsx->jsx_status1 & JSX1_RECOVER_DB )
    {
	/*
	** Database level recovery
	**
	** Get the interesting information. Note that since jnl_history_no
	** may not be the same as jnl_count-1, we set the last journal history
	** number to be the highest journal file history number for this
	** database. We'll roll forward each history from rfp->jnl_first
	** through rfp->jnl_last.
	**
	** XXXX No longer true XXX
	** If they're not rolling back to a checkpoint,
	**  (-c +j, say), then jnl_history_no is -1, which means we should
	**  only apply the journals since the most recent checkpoint.Bug #33422.
	** XXXX No longer true XXX
	** Now, get_ckp_sequence() returns a set of journals for -c as well.
	*/

	/*
	** Determine LA to which the database has been archived and dumped to
	** Save these so that they can be copied back into the restored
	** version of the config file.
	*/
	rfp->jnl_la = cnf->cnf_jnl->jnl_la;
	rfp->dmp_la = cnf->cnf_dmp->dmp_la;


	/*
	** Number of entries in the checkpoint history for journals
	** table. Current entry is (jnl_count -1 ) in the journal history
	** table. Save for same reason as above.
	*/
	rfp->jnl_count = cnf->cnf_jnl->jnl_count;

	/*
	**  Save the date and mode of the checkpoint selected. Note to
	**  sure why we do this. As these are saved latter on when
	**  we copy the whole journal and dump histories into the
	**  rfp.
	*/
	rfp->ckp_date  = cnf->cnf_jnl->jnl_history[jnl_history_no].ckp_date;

	/*
	** Save the current journal sequence number and block within that
	** journal file. Used to copy back into the restored config file.
	*/
	rfp->jnl_fil_seq = cnf->cnf_jnl->jnl_fil_seq;
	rfp->jnl_blk_seq = cnf->cnf_jnl->jnl_blk_seq;

	/*
	** The sequence number of the last dump file, this maybe different to
	** the checkpoint as OFFLINE checkpoints do not create a dump file.
	** Saved to copy back into the restored version of the config file.
	*/
	rfp->ckp_dmp_sequence = cnf->cnf_dmp->dmp_ckp_seq;

	/*
	** Number of entries in the checkpoint history for dumps
	** table. Current entry is (dmp_count -1 ) in the dmp history
	** table. Save in rfp so can copy back into restored version of ther
	** config file.
	*/
	rfp->dmp_count = cnf->cnf_dmp->dmp_count;



	/*
	** The sequence number of the last dump file and block offset within
	** the file. These values are saved to copy back into the
	** restored config file
	*/
	rfp->dmp_fil_seq = cnf->cnf_dmp->dmp_fil_seq;
	rfp->dmp_blk_seq = cnf->cnf_dmp->dmp_blk_seq;

    }
    else if ( jsx->jsx_status & JSX_TBL_LIST )
    {
	/*
	** Table level recovery, as this does not overwrite the
	** configuration file do not need to save information
	*/

	/*
	** init bits we do not need
	*/
	rfp->jnl_la.la_sequence = 0;
	rfp->jnl_la.la_block    = 0;
	rfp->jnl_la.la_offset   = 0;
	rfp->dmp_la.la_sequence = 0;
	rfp->dmp_la.la_block    = 0;
	rfp->dmp_la.la_offset   = 0;
        rfp->jnl_fil_seq = -1;
        rfp->jnl_blk_seq = -1;
        rfp->ckp_dmp_sequence = -1;
        rfp->dmp_count = -1;
        rfp->dmp_fil_seq = -1;
        rfp->dmp_blk_seq = -1;

    }

    /*
    ** Setup RFP information used by all levels
    */

    /*
    ** Used to determine if an online checkpoint thus dump processing
    ** is required.
    ** For database recovery this is copied back into the restored
    ** config file.
    */
    rfp->ckp_mode  = cnf->cnf_jnl->jnl_history[jnl_history_no].ckp_mode;

    /*
    ** Setup the journal and dump file block size
    ** FIXME, we do not copy these back into the config file so what
    ** happens if change these values since the last checkpoint or
    ** use the #c flag.
    */
    rfp->jnl_block_size = cnf->cnf_jnl->jnl_bksz;
    rfp->dmp_block_size = cnf->cnf_dmp->dmp_bksz;

    /*
    ** Entry in the checkpoint history for journals table where rollforward
    ** recovery should start. Note the user can specify this value.
    ** For database recovery this is copied back into the restored
    ** config file.
    */
    rfp->jnl_first = jnl_history_no;

    /*
    ** Entry in the checkpoint history where we should stop rollforwarddb.
    ** May not reach here as user can use -e or -end_lsn.
    ** For database recovery this is copied back into the restored
    ** config file.
    ** FIXME may be usefull to allow user to control this for moving
    ** stuff around in the disaster recovery case
    */
    rfp->jnl_last  = cnf->cnf_jnl->jnl_count - 1;

    /*
    ** The sequence number of the last checkpoint, saved to copy back
    ** into the restored version of the config file. Also used in the
    ** cluster case.
    */
    rfp->ckp_jnl_sequence = cnf->cnf_jnl->jnl_ckp_seq;

    /*
    ** If the the recoverying from an ONLINE checkpoint, dmp_history_no
    ** holds the entry no in the checkpoint dump history table to use.
    ** Setup the fields which describe how many dump files to process,
    ** may be zero.
    ** These values are used for
    **	   - Determine what dump files need to be applied
    **	   - are saved to copy back into the restored config
    **       file latter when we copy the whole checkpoint history for
    **       dump table into the rfp
    */
    if (dmp_history_no != -1)
    {
	rfp->dmp_first= cnf->cnf_dmp->dmp_history[dmp_history_no].ckp_f_dmp;
	rfp->dmp_last = cnf->cnf_dmp->dmp_history[dmp_history_no].ckp_l_dmp;
    }

    /*
    ** Save entire journal and dump history, used in the dmp and journal
    ** processing to determine what journal/dump files to read.
    ** For a database level checkpoint these are moved back into the
    ** restored config file.
    */
    for (i = 0; i < CKP_HISTORY; i++)
    {
	STRUCT_ASSIGN_MACRO(cnf->cnf_jnl->jnl_history[i],
						rfp->rfp_jnl_history[i]);
	STRUCT_ASSIGN_MACRO(cnf->cnf_dmp->dmp_history[i],
						rfp->rfp_dmp_history[i]);
    }


    /*
    ** If restoring the checkpoint, check that we are using a VALID
    ** checkpoint.
    */
    if ((jsx->jsx_status & JSX_USECKP) != 0)
    {
	/*
	** The journal checkpoint sequence number from the journal
	** history is
	** the correct checkpoint sequence number to use. If the checkpoint
	** in question was an online checkpoint, then we will double-check
	** the correctness of the sequence number by comparing the journal
	** and dump history sequence numbers and complaining if they don't
	** match.
	*/

	/*
	** Setup the checkpoint sequence number which is actually going
	** to be used
	*/
	rfp->ckp_seq =
		cnf->cnf_jnl->jnl_history[jnl_history_no].ckp_sequence;

	/*
	** Check that it is a valid checkpoint, FIXME is this an
	** OK check
	*/
	if (cnf->cnf_jnl->jnl_history[jnl_history_no].ckp_jnl_valid == 0)
	{
	    TRformat( dmf_put_line, 0, line_buffer, sizeof(line_buffer),
    "%@ RFP: Checkpoint history of journal tables indicates  that checkpoint is invalid\n");
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM1327_RFP_INV_CHECKPOINT);
	    return (E_DB_ERROR);
	}

	/*
	** If an ONLINE checkpoint  then make sure that we have a
	** dump file and that the checkpoint dump history table
	** sequence number matches the checkpoint sequence number
	** we are recoverying. Seems very bad if this has happened.
	*/
	if ((rfp->ckp_mode & CKP_ONLINE) &&
	    (dmp_history_no == -1 || rfp->ckp_seq !=
		cnf->cnf_dmp->dmp_history[dmp_history_no].ckp_sequence) )
	{
	    TRformat( dmf_put_line, 0, line_buffer, sizeof(line_buffer),
		    "%@ RFP: History sequence number mismatch (%d, %d)\n",
		    (dmp_history_no == -1 ? -1 :
		    cnf->cnf_dmp->dmp_history[dmp_history_no].ckp_sequence),
		    cnf->cnf_jnl->jnl_history[jnl_history_no].ckp_sequence);
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM1327_RFP_INV_CHECKPOINT);
	    return (E_DB_ERROR);
	}
    }

    return( E_DB_OK );
}


static DB_STATUS
rfp_init_tbl_hash(
DMF_RFP		    *rfp)
{
    DMF_JSX		*jsx = rfp->rfp_jsx;
    RFP_TBLHCB  	*tblhcb;
    DB_STATUS		status;
    i4			i;
    i4			error;

    CLRDBERR(&jsx->jsx_dberr);

    /*
    ** Allocate the hash bucket array for the tables
    */
    /* (canor01) don't zero since this is explicitly initialized below */
    status = dm0m_allocate(
	sizeof(RFP_TBLHCB) + ( 63 * sizeof(TBL_HASH_ENTRY)),
	(i4)0, (i4)TBL_HCB_CB,
	(i4)RFP_TBLHCB_ASCII_ID, (char *)rfp,
	(DM_OBJECT **)&rfp->rfp_tblhcb_ptr, &jsx->jsx_dberr);
    if ( status != E_DB_OK )
	return(status);

    /*
    ** Initailize the hash bucket array
    */
    tblhcb = rfp->rfp_tblhcb_ptr;
    tblhcb->tblhcb_htotq.q_next = &tblhcb->tblhcb_htotq;
    tblhcb->tblhcb_htotq.q_prev = &tblhcb->tblhcb_htotq;
    tblhcb->tblhcb_hinvq.q_next = &tblhcb->tblhcb_hinvq;
    tblhcb->tblhcb_hinvq.q_prev = &tblhcb->tblhcb_hinvq;

    tblhcb->tblhcb_no_hash_buckets = 63;
    for (i = 0; i < tblhcb->tblhcb_no_hash_buckets; i++)
    {
	tblhcb->tblhcb_hash_array[i].tblhcb_headq_next = (RFP_TBLCB*)
	    &tblhcb->tblhcb_hash_array[i].tblhcb_headq_next;
            tblhcb->tblhcb_hash_array[i].tblhcb_headq_prev = (RFP_TBLCB*)
	    &tblhcb->tblhcb_hash_array[i].tblhcb_headq_next;
    }

    return (E_DB_OK);
}


/*{
** Name:
**
** Description: rfp_prepare_tbl_recovery.
**
**
** Inputs:
**	rfp
**	jsx				Pointer to journal support context.
**	dcb				Pointer to DCB for database.
**
** Outputs:
**	rfp				Rollforward context.
**      err_code                        Reason for error code.
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**
** History:
**	14-sep-1994 (andyw)
**	    Partial Backup/Recovery Project:
**		Created.
**	15-nov-1994 (andyw)
**	    Added additional argument to rfp_recov_core_tbl() passing
**	    DMP_DCB structure
**      22-nov-1994 (juliet)
**          Added ule_format calls to report table to be recovered is a
**	    core DBMS table, (RFP_NO_CORE_TBL_RECOVERY) and
**	    table level recovery is not allowed due to inconsistent
**          database (RFP_INCONSIST_DB_NO_TBLRECV).
**	23-jan-1995 (stial01) 
**	    BUG 66403: apply proper case to table names
**	 3-jul-96 (nick)
**	    Fix error handling.
**	10-jul-96 (nick)
**	    Several return statuses being ignored.
**	16-jan-1997 (nanpr01)
**	    Duplicate messages are being displayed for errors . #80068.
**	18-feb-1997 (shero03)
**	    B80661: check and see if there is a checkpoint before checking for
**	    other errors.
**      28-feb-2000 (gupsh01)
**          Enabled check for owner along with the table name
**          so that the table names in -table option can also
**          be specified as owner.tablenames .
*/
static DB_STATUS
rfp_prepare_tbl_recovery(
DMF_RFP		    *rfp,
DMF_JSX		    *jsx,
DMP_DCB		    *dcb)
{
    i4		open_flags;
    bool		open_config = FALSE;
    char                line_buffer[132];
    DB_STATUS		status, tmp_status;
    DM0C_CNF    	*cnf;
    DMP_EXT		*ext;
    DM_PATH		dmp_physical;
    int			dmp_phys_length;
    i4     	dmp_history_no;
    i4     	jnl_history_no;
    i4		i;
    RFP_LOC_MASK	*lm =0;
    i4		size;
    char                tmp_name[DB_MAXNAME]; /* for owner or table name */
    i4			error;
    DB_ERROR		local_dberr;

    CLRDBERR(&jsx->jsx_dberr);

    rfp->rfp_locmap = 0;

    /*
    ** Apply proper case to objects needing it.
    ** Adjustment performed in place, assuming that ROLLFORWARDDB 
    ** operates on only one database.
     */
    if (jsx->jsx_status & JSX_TBL_LIST) 
    {
	for (i = 0; i < jsx->jsx_tbl_cnt; i++)
	{
	    jsp_set_case(jsx, 
		    jsx->jsx_tbl_list[i].tbl_delim ? 
			jsx->jsx_delim_case : jsx->jsx_reg_case,
		    DB_TAB_MAXNAME, (char *)&jsx->jsx_tbl_list[i].tbl_name, 
		    tmp_name);

	    MEcopy(tmp_name, DB_TAB_MAXNAME, 
		    (char *)&jsx->jsx_tbl_list[i].tbl_name);

            if (STbcompare((char *)&jsx->jsx_tbl_list[i].tbl_owner.db_own_name,
                0,"", 0, 0) != 0)
            {
                jsp_set_case(jsx,
                jsx->jsx_tbl_list[i].tbl_delim ?
                        jsx->jsx_delim_case : jsx->jsx_reg_case,
                DB_OWN_MAXNAME, (char *)&jsx->jsx_tbl_list[i].tbl_owner.db_own_name,
                tmp_name);

                MEcopy(tmp_name, DB_OWN_MAXNAME,
                        (char *)&jsx->jsx_tbl_list[i].tbl_owner.db_own_name);
            }
	}
    }

    do
    {
	status = rfp_init_tbl_hash(rfp);
	if (status != E_DB_OK)
	    break;

        status = config_handler(jsx, OPEN_CONFIG, dcb, &cnf);
        if (status != E_DB_OK)
            break;

      	open_config = TRUE;

        /*
        ** If there is no checkpoint, return error.
        */
        if (cnf->cnf_jnl->jnl_ckp_seq == 0 && cnf->cnf_dmp->dmp_ckp_seq == 0)
        {
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM1316_RFP_NO_CHECKPOINT);
            status = E_DB_ERROR;
	    break;
        }

        /*
        ** If database is journaled or the user has specified the "-j" flag
        ** files. Then just return success as nothing happened
        */
        open_flags = DM2D_JLK;
        if ((jsx->jsx_status & JSX_WAITDB) == 0)   /* b66533 */
            open_flags |= DM2D_NOWAIT;

        status = rfp_open_database (rfp, jsx, dcb, open_flags);

        if (status != E_DB_OK)
        {
            return (status);
        }

        /*
        ** Check if need to drain the journal files.
        */
        status = rfp_prepare_drain_journals( jsx, dcb, &cnf);
        if ( status != E_DB_OK )
        {
            open_config = FALSE;
            break;
        }

	/* Save the location of the dmp path */
    	dmp_phys_length = dcb->dcb_dmp->phys_length;
	MEcopy(dcb->dcb_dmp->physical.name, dmp_phys_length, dmp_physical.name);

        /*  Close the configuration file. */

        status = config_handler(jsx, CLOSEKEEP_CONFIG, dcb, &cnf);
        if (status != E_DB_OK)
           return (status);

	open_config = FALSE;

        /*  Close the database. */

	status = rfp_close_database (rfp, dcb, open_flags);

        if (status != E_DB_OK)
        {
            uleFormat(&jsx->jsx_dberr, 0, NULL, ULE_LOG, NULL,
                (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM110A_CPP_CLOSE_DB);
            return (status);
        }

        TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
            "%@ CPP: Completed drain of log file for database: %~t\n",
            sizeof(DB_DB_NAME), dcb->dcb_name.db_db_name);

	/*
	** Read tables and their respective tcb control blocks
	** before any check processing against whether the table
	** is valid or not!
	*/
	status = rfp_read_tables (rfp, jsx, dcb);
	if (status != E_DB_OK)
	{
	    dmfWriteMsg(&jsx->jsx_dberr, 0, 0);
	    return (status);
	}

	/*
	** Determine which partitions,indexes need to be rolled forward 
	** for consistency of tables being rolled forward:
	**
	**   Set RFP_PART_REQUIRED for all partitions on tables selected.
	**   Set RFP_INDEX_REQUIRED for all indexes on tables selected.
	** 
	**   If -nosecondary_index NOT specified,
	**     Set RFP_RECOVER_TABLE for all indexes on tables selected.
	*/
	status = rfp_get_table_info (rfp, jsx, dcb);
	if (status != E_DB_OK)
	    break;

	/*
	** Determine which blobs need to be rolled forward for consistency
	** of tables being rolled forward:
	**
	** Set RFP_BLOB_REQUIRED for all blob tables for tables selected.
	**
	** If -noblobs NOT specified,
	**    Set RFP_RECOVER_TABLE for all blobs on tables selected.
	*/
	status = rfp_get_blob_info (rfp, jsx, dcb);
	if (status != E_DB_OK)
	    break;

	/*
	** Table level recovery requires that the core DBMS catalogs
	** and the database are consistent
	*/
	status = rfp_recov_core_tbl (rfp, jsx, dcb);

 	if (status != E_DB_OK)
	{
            /*
            ** Presently unable to recover core DBMS catalogs,
	    ** so display and log error message.
            */
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM1355_RFP_NO_CORE_TBL_RECOVERY);
	    return (status);
	}

	/*
	** Open the config file
	*/
	status = config_handler(jsx, OPEN_CONFIG, dcb, &cnf);
	if ( status != E_DB_OK )
	    break;

	open_config = TRUE;

	/*
	** Allocate the location mask structure, holds information
	** on what locations are interesting. Currently this is only
	** set whilst scanning list of tables to determine what
	** checkpoint files are around. In the future could set by
	** a -location switch to rollforwarddb.
	*/
        ext = dcb->dcb_ext;
        size = sizeof(i4) *
        (ext->ext_count+BITS_IN(i4)-1)/BITS_IN(i4);

	status = dm0m_allocate((i4)sizeof(RFP_LOC_MASK) + size,
				DM0M_ZERO, (i4)RFP_LOCM_CB,
				(i4)RFP_LOCM_ASCII_ID,
				(char *)rfp,
				(DM_OBJECT **)&lm, &jsx->jsx_dberr);
	if ( status != E_DB_OK )
	    break;

	lm->locm_bits = size * BITSPERBYTE;
	lm->locm_r_allow  = (i4 *)((char *)lm + sizeof(RFP_LOC_MASK));

	rfp->rfp_loc_mask_ptr = lm;

	if (jsx->jsx_status & JSX_TRACE)
	{
	    TRdisplay("%@ prepare: Open Config file (1)\n");
	    dump_cnf(cnf);
	}

	/*
	** Setup the db status field used to see if database is
	** journaled etc..
	*/
	rfp->db_status = cnf->cnf_dsc->dsc_status;

	/*
	** Validate basic checkpoint/journal information
	*/
	status = rfp_prepare_validate_cnf_info( jsx, dcb, cnf );
	if ( status != E_DB_OK )
	    break;

        /*
        ** Validate table relocation
        */
        if ((jsx->jsx_status1 & JSX_RELOCATE) &&
            (jsx->jsx_status1 & JSX_NLOC_LIST))
        {
	    status = reloc_tbl_process(rfp, jsx, dcb, RELOC_PREPARE_TABLES);
            if (status != E_DB_OK)
                break;
        }

	/*
	** Determine which checkpoint we will roll-forward from.
	*/
	status = get_ckp_sequence( jsx, cnf, &jnl_history_no,
                                   &dmp_history_no );
	if (status != E_DB_OK)
            break;

	/*
        ** Get the interesting information related to this checkpoint
	*/
	status = rfp_prepare_get_ckp_jnl_dmp_info( rfp, jsx, cnf,
				jnl_history_no, dmp_history_no );
	if ( status != E_DB_OK )
	    break;

	/*
	** table level recovery requires that the tables specified by
	** the user are valid
	*/
	status = rfp_recov_invalid_tbl(rfp, jsx, dcb, cnf, jnl_history_no);

	if (status != E_DB_OK)
	{
	    /*
	    ** Unable to open invalid tables which has been specified
	    ** by the user, therefore abort procedure
	    */
	    local_dberr = jsx->jsx_dberr;
	    tmp_status = config_handler(jsx, CLOSE_CONFIG, dcb, &cnf);
	    if (tmp_status > status)
		status = tmp_status;
	    else
	        jsx->jsx_dberr = local_dberr;
            return (status);
        }

	/*
	** Close the configuration file
	*/
	status = config_handler(jsx, CLOSE_CONFIG, dcb, &cnf);
	if ( status != E_DB_OK )
	    break;

	open_config = FALSE;

	/*
	** table level recovery from a table level checkpoint requires 
	** that the tables specified by the user have been checkpointed
	*/
	if ( rfp->ckp_mode & CKP_TABLE )
	    status = rfp_recov_chkpt_tables(rfp, &dmp_physical, dmp_phys_length);

	if (status != E_DB_OK)
            break;

	/*
	** Mark all tables which will be recovered physically
	** inconsistent and if necessary logically inconsistent.
	**
	** All indexes marked RFP_INDEX_REQUIRED, but not RFP_RECOVER_TABLE
	** will be marked inconsistent.
	**
	** All blobs marked RFP_BLOB_REQUIRED, but not RFP_RECOVER_TABLE
	** will be marked inconsistent.
	*/

	status = rfp_update_tbls_status( rfp, jsx, dcb,
					 RFP_PREPARE_UPDATE_TBLS_STATUS );
	if ( status != E_DB_OK )
	    break;

	/*
	** Display information gathered so far
	*/
	rfp_display_recovery_tables( rfp, jsx );
	rfp_display_no_recovery_tables( rfp, jsx );
	rfp_display_inv_tables( rfp, jsx );

    } while (FALSE);

    if (open_config)
    {
        DB_STATUS    lstatus;

	local_dberr = jsx->jsx_dberr;

        lstatus = config_handler(jsx, CLOSE_CONFIG, dcb, &cnf);
        if ( lstatus != E_DB_OK )
            uleFormat(&jsx->jsx_dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL, &error, 0);

	jsx->jsx_dberr = local_dberr;
    }

    return( status );
}

/*
** Name:
**
** Description: rfp_recov_core_tbl - Returns true if recoverying a core
**			             DBMS catalog.
**
**
** Inputs:
**	jsx				Pointer to journal support context.
**
** Outputs:
**	rfp				Rollforward context.
**      err_code                        Reason for error code.
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**
** History:
**	14-sep-1994 (andyw)
**	    Partial Backup/Recovery Project:
**	 	Created.
*/
static DB_STATUS
rfp_recov_core_tbl(
DMF_RFP		    *rfp,
DMF_JSX		    *jsx,
DMP_DCB		    *dcb)
{
    RFP_QUEUE		*totq;
    RFP_TBLCB           *tblcb = 0;
    RFP_TBLHCB          *hcb = rfp->rfp_tblhcb_ptr;

    /*
    ** Loop through all the user supplied tables gathering more info
    */
    for ( totq = hcb->tblhcb_htotq.q_next;
          totq != &hcb->tblhcb_htotq;
          totq = tblcb->tblcb_totq.q_next
          )
    {
	tblcb = (RFP_TBLCB *)((char *)totq - CL_OFFSETOF(RFP_TBLCB, tblcb_totq));

        if ((tblcb->tblcb_table_status & RFP_USER_SPECIFIED) &&
	    (tblcb->tblcb_table_status & RFP_CATALOG))
        {
	    /*
	    ** Table specified by the user and is a system catalog
	    */
	    return (E_DB_ERROR);
	}
    }
    return (E_DB_OK);
}

/*
** Name:
**
** Description: rfp_recov_invalid_tbl - Returns true if recoverying an
**			                invalid table
**
**
** Inputs:
**	jsx				Pointer to journal support context.
**
** Outputs:
**	rfp				Rollforward context.
**      err_code                        Reason for error code.
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**
** History:
**	22-nov-1994 (andyw)
**	    Partial Backup/Recovery Project:
**	 	Created.
**      23-jan-1995 (stial01)
**          BUG 66407: check for recovery on table for which recovery is
**              disallowed.
**          BUG 66581: error if view specified.
**	30-aug-1995 (nick)
**	    Parameter for E_DM135F text should be a string.
**	23-aug-1995 (canor01)
**	    BUG 70789: pass table name member of structure instead of
**		structure itself to ule_format() for both RS6000 and NT
**	01-sep-1998 (nanpr01)
**	    Bug 80014 : Must check the last modify date before rolling
**	    forward the table.
**	10-oct-1998 (nanpr01)
**	    Bug 80014 : Fix the remainder of the holes for bug 80014.
**	08-jan-1999 (nanpr01)
**	    Allow rollforward of a table on a non-journalled database, if the
**	    table has not been modified.
**      28-feb-2000 (gupsh01)
**          Enabled check for owner along with the table name
**          so that the table names in -table option can also
**          be specified as owner.tablenames .
**      10-aug-2007 (horda03) Bug 118933
**          Use TMcmp_stamp() to compare timestamps.
*/
static DB_STATUS
rfp_recov_invalid_tbl(
DMF_RFP		    *rfp,
DMF_JSX		    *jsx,
DMP_DCB		    *dcb,
DM0C_CNF	    *cnf,
i4		    jnl_history_no)
{
    RFP_QUEUE		*totq;
    RFP_TBLCB           *tblcb = 0;
    RFP_TBLHCB          *hcb = rfp->rfp_tblhcb_ptr;
    i4		i;
    bool 		table_found = FALSE;
    char                error_buffer[ER_MAX_LEN];
    i4             error_length;
    TM_STAMP		modify_date;
    i4			error;

    CLRDBERR(&jsx->jsx_dberr);

    /*
    ** Loop through all the user supplied tables gathering more info
    */
    for ( totq = hcb->tblhcb_htotq.q_next;
          totq != &hcb->tblhcb_htotq;
          totq = tblcb->tblcb_totq.q_next
          )
    {
	tblcb = (RFP_TBLCB *)((char *)totq - CL_OFFSETOF(RFP_TBLCB, tblcb_totq));

	/* 
	** If this index is created after the checkpoint,
	** it will have the disallowed bit set and we should tell
	** user to try -nosecondary_index option.
	*/
	if (tblcb->tblcb_table_status & RFP_INDEX_REQUIRED)
	{
	    if ((tblcb->tblcb_table_status & RFP_RECOVER_TABLE) == 0)
		continue;	
	}
	else if ((tblcb->tblcb_table_status & RFP_USER_SPECIFIED) == 0)
	    continue;

	if (tblcb->tblcb_table_status & RFP_VIEW)
        {
	    /*
	    ** Table specified by the user and is a view 
	    */
	    dmfWriteMsg(NULL, E_DM1358_RFP_VIEW_TBL_NO_RECV, 0);
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM135E_RFP_USR_INVALID_TABLE);
	    return (E_DB_ERROR);
	}

	if (tblcb->tblcb_table_status & RFP_GATEWAY)
        {
	    /*
	    ** Table specified by the user and is a gateway table
	    */
	    dmfWriteMsg(NULL, E_DM1359_RFP_GATEWAY_TBL_NO_RECV, 0);
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM135E_RFP_USR_INVALID_TABLE);
	    return (E_DB_ERROR);
	}

	if (tblcb->tblcb_table_status & RFP_RECOV_DISALLOWED)
        {
	    /*
	    ** Table specified by the user and recov disallowed  
	    ** for this table
	    */
	    uleFormat(NULL, E_DM135F_RFP_NO_TBL_RECOV, (CL_ERR_DESC *)NULL,
		ULE_LOG, NULL,
		error_buffer, ER_MAX_LEN, &error_length, &error, 1,
                DB_TAB_MAXNAME, tblcb->tblcb_table_name.db_tab_name);
	    dmf_put_line(0, error_length, error_buffer);
	    if ((tblcb->tblcb_table_status & RFP_USER_SPECIFIED) == 0)
		SETDBERR(&jsx->jsx_dberr, 0, E_DM1370_RFP_NO_SEC_INDEX);
	    else
		SETDBERR(&jsx->jsx_dberr, 0, E_DM135E_RFP_USR_INVALID_TABLE);
	    return (E_DB_ERROR);
	}

	TMsecs_to_stamp(tblcb->tblcb_moddate, &modify_date);
	if (TMcmp_stamp(&modify_date,
	    (TM_STAMP *)&cnf->cnf_jnl->jnl_history[jnl_history_no].ckp_date)
	    >= 0)
	{
	    /*
	    ** Table specified by the user and recov disallowed
	    ** for this table
	    */
	    uleFormat(NULL, E_DM135F_RFP_NO_TBL_RECOV, (CL_ERR_DESC *)NULL,
		ULE_LOG, NULL,
		error_buffer, ER_MAX_LEN, &error_length, &error, 1,
                DB_TAB_MAXNAME, tblcb->tblcb_table_name.db_tab_name);
	    dmf_put_line(0, error_length, error_buffer);
	    if ((tblcb->tblcb_table_status & RFP_USER_SPECIFIED) == 0)
		SETDBERR(&jsx->jsx_dberr, 0, E_DM1370_RFP_NO_SEC_INDEX);
	    else
		SETDBERR(&jsx->jsx_dberr, 0, E_DM135E_RFP_USR_INVALID_TABLE);
	    return (E_DB_ERROR);
	}
        /* 
        ** If journalling is disabled and it is table level rollforward, we 
        ** should mark the table logically inconsistent 
        */
    	if (((cnf->cnf_dsc->dsc_status & DSC_DISABLE_JOURNAL) ||
		((cnf->cnf_dsc->dsc_status & DSC_JOURNAL) == 0))  &&
             (TMcmp_stamp( (TM_STAMP *) &tblcb->tblcb_stamp12,
                    (TM_STAMP *) &cnf->cnf_jnl->jnl_history[jnl_history_no].ckp_date) >= 0))
	    rfp->rfp_point_in_time = TRUE;
    }

    /* Make sure all tables specified by user exist */
    for ( i = (jsx->jsx_tbl_cnt-1); i >= 0; i--)
    {
        /*
        ** Loop through all the user supplied tables gathering more info
        */
	table_found = FALSE;
	for ( totq = hcb->tblhcb_htotq.q_next;
	      totq != &hcb->tblhcb_htotq;
	      totq = tblcb->tblcb_totq.q_next
	      )
	{
	    tblcb = (RFP_TBLCB *)((char *)totq - 
				CL_OFFSETOF(RFP_TBLCB, tblcb_totq));

	    if ((MEcmp (tblcb->tblcb_table_name.db_tab_name,
			&jsx->jsx_tbl_list[i].tbl_name.db_tab_name,
			DB_TAB_MAXNAME)) == 0)
            {
               if (STbcompare((char *)&jsx->jsx_tbl_list[i].tbl_owner.db_own_name,
                        0,"", 0, 0) != 0)
                {
                    if ((MEcmp (tblcb->tblcb_table_owner.db_own_name,
                        &jsx->jsx_tbl_list[i].tbl_owner.db_own_name
                        ,DB_OWN_MAXNAME)) == 0)
                        table_found = TRUE;
                }
                else
                table_found = TRUE;
	    }
        }
	if (!table_found)
	{
	    uleFormat(NULL, E_DM1360_RFP_TBL_NOTFOUND, (CL_ERR_DESC *)NULL, 
		ULE_LOG, NULL,
		error_buffer, ER_MAX_LEN, &error_length, &error, 1,
		DB_TAB_MAXNAME, &jsx->jsx_tbl_list[i].tbl_name.db_tab_name);
	    dmf_put_line(0, error_length, error_buffer);
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM135E_RFP_USR_INVALID_TABLE);
	    return (E_DB_ERROR);
	}
    }
    return (E_DB_OK);
}

/*{
** Name: rfp_recov_chkpt_tables
**
** Description:
** 	Open the list of tables that were checkpointed by the 
** 	table-level checkpoint. Check to make sure that every table
** 	specified for recovery exists in the list of those checkpointed.
**
** Inputs:
**	rfp				Rollforward context.
**	jsx				Pointer to journal support context.
**	dmp_physical			The dmp physical path name
**	dmp_phys_length			ength of physical path name
**
** Outputs:
**      err_code                        Reason for error code.
** 
** Returns:
**	E_DB_OK
**	E_DB_ERROR
** 
** Exceptions:
**	none
**
** Side Effects:
**	none
**
** History:
**	01-jun-1995 (wonst02)  
** 	    Bug 68886, 68866
** 	    Added to ensure that the tables specified by user are 
** 	    in the table-level checkpoint.
**	 7-feb-1996 (nick)
**	    Rewritten ; assumptions made by the author were/are now incorrect .
*/
static DB_STATUS
rfp_recov_chkpt_tables(
DMF_RFP             *rfp,
DM_PATH		    *dmp_physical,
int		    dmp_phys_length)
{
    DMF_JSX		*jsx = rfp->rfp_jsx;
    RFP_QUEUE		*totq;
    RFP_TBLCB           *tblcb = 0;
    RFP_TBLHCB          *hcb = rfp->rfp_tblhcb_ptr;
    DB_TAB_NAME		*table_list;
    DB_STATUS       	status;
    DB_STATUS       	local_status;
    TBLLST_CTX		tblctx;
    char        	error_buffer[ER_MAX_LEN];
    i4     	error_length;
    char		table_name[DB_TAB_MAXNAME + 1];
    i4		local_err;
    i4             i;
    i4			error;

    CLRDBERR(&jsx->jsx_dberr);

    tblctx.seq = rfp->ckp_seq;
    tblctx.blkno = -1;
    tblctx.blksz = TBLLST_BKZ;
    tblctx.phys_length = dmp_phys_length;
    STRUCT_ASSIGN_MACRO(*dmp_physical, tblctx.physical);
    table_list = (DB_TAB_NAME *)&tblctx.tbuf;

    do
    {
	status = tbllst_open(jsx, &tblctx);
	if (status != E_DB_OK)
	{
	    uleFormat(&jsx->jsx_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, (char * )NULL,
		0L, (i4 *)NULL, &local_err, 0);
	    uleFormat(NULL, E_DM1365_RFP_CHKPT_LST_OPEN, (CL_ERR_DESC *)NULL, ULE_LOG, 
		NULL, error_buffer, ER_MAX_LEN, &error_length, &local_err, 1,
		sizeof(DM_FILE), tblctx.filename.name);
	    dmf_put_line(0, error_length, error_buffer);
	    status = E_DB_OK;
	    CLRDBERR(&jsx->jsx_dberr);
	    break;
	}
	while (status == E_DB_OK)
	{
	    status = tbllst_read(jsx, &tblctx);
	    if (status != E_DB_OK)
	    {
		if (status == E_DB_WARN)
		{
		    status = E_DB_OK;
		    break;
		}
		else
		{
		    uleFormat(&jsx->jsx_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
			(char * )NULL, 0L, (i4 *)NULL, &local_err, 0);
	    	    uleFormat(&jsx->jsx_dberr, E_DM1366_RFP_CHKPT_LST_READ, 
			(CL_ERR_DESC *)NULL, ULE_LOG, 
			NULL, error_buffer, ER_MAX_LEN, &error_length, 
			&local_err, 1, sizeof(DM_FILE), tblctx.filename.name);
	    	    dmf_put_line(0, error_length, error_buffer);
		}
	    }
	    else
	    {
		/*
		** For each read, we loop through the tblcb queue and for
		** interesting tables ( i.e. command line specified ), see
		** if the current page read contains it.  Since we are 
		** unlikely to have many reads ( it was (incorrectly) assumed
		** to be just one ) this will do for now.  With more than a 
		** few pages, caching the file might be more appropos to
		** avoid looping through the queue an inordinate number of
		** times.
		*/
		for ( totq = hcb->tblhcb_htotq.q_next;
		      totq != &hcb->tblhcb_htotq;
		      totq = tblcb->tblcb_totq.q_next
		      )
		{
		    tblcb = (RFP_TBLCB *)((char *)totq - 
					CL_OFFSETOF(RFP_TBLCB, tblcb_totq));

        	    if ((tblcb->tblcb_table_status & RFP_RECOVER_TABLE ) == 0)
            		continue;

		    /* need to check for this table */
		    for (i = 0; i < TBLLST_MAX_TAB; i++)
		    {
	    		if (table_list[i].db_tab_name[0] == '\0')
	    		    break;

	    		if ((MEcmp(tblcb->tblcb_table_name.db_tab_name,
			    table_list[i].db_tab_name, DB_TAB_MAXNAME)) == 0)
            		{
	    			tblcb->tblcb_table_status |= RFP_FOUND_TABLE;
				break;
	    		}
		    }
    		} 
	    }
	}

	if (status == E_DB_OK)
	{
	    for ( totq = hcb->tblhcb_htotq.q_next;
		  totq != &hcb->tblhcb_htotq;
		  totq = tblcb->tblcb_totq.q_next
		  )
	    {
		tblcb = (RFP_TBLCB *)((char *)totq - 
				CL_OFFSETOF(RFP_TBLCB, tblcb_totq));

		if ((tblcb->tblcb_table_status & 
		    (RFP_RECOVER_TABLE | RFP_FOUND_TABLE)) 
			== RFP_RECOVER_TABLE)
		{
	    	    STncpy( table_name, tblcb->tblcb_table_name.db_tab_name,
				DB_TAB_MAXNAME);
		    table_name[ DB_TAB_MAXNAME ] = '\0';
	    	    STtrmwhite(table_name);
	    	    uleFormat(NULL, E_DM1364_RFP_NO_TBL_CHKPT, (CL_ERR_DESC *)NULL, 
			ULE_LOG, NULL,
			error_buffer, ER_MAX_LEN, &error_length, &error, 1,
			0, table_name);
	    	    dmf_put_line(0, error_length, error_buffer);
	    	    status = E_DB_ERROR;
		}
	    }
	}

	local_status = tbllst_close(jsx, &tblctx);
	if (local_status != E_DB_OK)
	{
	    uleFormat(&jsx->jsx_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		(char * )NULL, 0L, (i4 *)NULL, &local_err, 0);
	    uleFormat(NULL, E_DM1367_RFP_CHKPT_LST_CLOSE, (CL_ERR_DESC *)NULL, 
		ULE_LOG, 
		NULL, error_buffer, ER_MAX_LEN, &error_length, &local_err, 1,
		sizeof(DM_FILE), tblctx.filename.name);
	    dmf_put_line(0, error_length, error_buffer);
	    status = local_status;
	}
    } while (FALSE);

    if (DB_FAILURE_MACRO(status))
	SETDBERR(&jsx->jsx_dberr, 0, E_DM1368_RFP_CHKPT_LST_ERROR);
    return(status);
}

/*{
** Name: rfp_error - Error encountered during rollforward
**
** Description:
**
**
** Inputs:
**	rfp
**	jsx			Pointer to journal support context.
**      dmve			Pointer to DMVE context 
**	err_status		status of failure
**	err_code		err_code of failure
**
**
** Outputs:
**      err_code                        Reason for error code.
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**
** History:
**	14-sep-1994 (andyw)
**	    Partial Backup/Recovery Project:
**		Created.
*/
static DB_STATUS
rfp_error(
DMF_RFP             *rfp,
DMF_JSX             *jsx,
RFP_TBLCB	    *tblcb,
DMVE_CB		    *dmve,
i4		    err_type,
DB_STATUS	    err_status)
{
    RFP_TBLHCB	    *hcb = rfp->rfp_tblhcb_ptr;
    DB_STATUS	    loc_stat;
    i4		    loc_err;
    char	    line_buffer[132];
    DB_TAB_ID	    tabid;
    DB_TAB_NAME	    tabname;
    DB_OWN_NAME	    ownname;
    i4		    local_error;
    i4		    table_error = jsx->jsx_dberr.err_code;
    bool	    apply, found;
    i4		    rfp_action = RFP_ROLLBACK;
    bool	    verbose;
    DB_STATUS	    status = E_DB_OK;
    DB_ERROR	    local_dberr;

    /*
    ** ALWAYS report the error in the errlog.log even if we are going
    ** to ignore it
    */
    dmfWriteMsg(&jsx->jsx_dberr, 0, 0);

    if (dmve->dmve_log_rec)
    {
	dmd_format_dm_hdr(dmf_diag_put_line, (DM0L_HEADER *)dmve->dmve_log_rec, 
	    line_buffer, sizeof(line_buffer));

	/* Verbose = TRUE to get a hexdump of this log record */
	if (jsx->jsx_status & JSX_TRACE)
	    verbose = TRUE;
	else
	    verbose = FALSE;
	
	dmd_format_log(verbose, dmf_diag_put_line, (char *)dmve->dmve_log_rec,
	    rfp->jnl_block_size, line_buffer, sizeof(line_buffer));

	rfp_xn_diag(jsx, rfp, dmve);
    }

    if (tblcb)
    {
	STRUCT_ASSIGN_MACRO(tblcb->tblcb_table_id, tabid);
	STRUCT_ASSIGN_MACRO(tblcb->tblcb_table_name, tabname);
	STRUCT_ASSIGN_MACRO(tblcb->tblcb_table_owner, ownname);
    }
    else if (!tblcb && dmve && dmve->dmve_log_rec)
    {
	/* this must be database recovery - or we would already have tblcb */
	/* Call dmve_get_tab to extract table information from log record */
	dmve_get_tab((char *)dmve->dmve_log_rec, &tabid, &tabname, &ownname);
    }
    else
    {
	tabid.db_tab_base = 0;
	tabid.db_tab_index = 0;
    }

    /* If no table id, then no table error handling */
    if (tabid.db_tab_base == 0)
    {
	status = rfp_er_handler(rfp, jsx);
	return (status);
    }

    TRformat(dmf_diag_put_line, 0, line_buffer, sizeof(line_buffer),
	"Error during recovery of table (%~t, %~t)\n",
	sizeof(DB_TAB_NAME), tabname.db_tab_name,
	sizeof(DB_OWN_NAME), ownname.db_own_name);

    /* Special error handling for core catalogs */
    if (tabid.db_tab_base <= DM_SCONCUR_MAX)
    {
	status = rfp_sconcur_error(rfp, jsx, tblcb, dmve, err_type, err_status);
	return (status);
    }

    if (tabid.db_tab_index > 0)
    {
	/* If secondary index, continue rollforwarddb, but ignore this index */
	rfp_action = RFP_CONTINUE_IGNORE_TBL;
    }
    else if (jsx->jsx_status2 & JSX2_ON_ERR_PROMPT)
    {
	/* Prompt for rfp_action */
	loc_stat = rfp_prompt(rfp, jsx, &rfp_action);
	if (loc_stat == E_DB_ERROR || rfp_action == RFP_ROLLBACK)
	    return (E_DB_ERROR);
    }
    else if (jsx->jsx_status1 & JSX1_ON_ERR_CONT)
    {
	/* Continue rollforwarddb, but ignore this table */
	rfp_action = RFP_CONTINUE_IGNORE_TBL;
    }
    else if (jsx->jsx_status1 & JSX1_IGNORE_FLAG)
    {
	/* Continue rfp, ignore this error */
	rfp_action = RFP_CONTINUE_IGNORE_ERR;
    }

    if (rfp_action == RFP_ROLLBACK)
    {
	return (E_DB_ERROR);
    }

    /* Here for RFP_CONTINUE_IGNORE_ERR, RFP_CONTINUE_IGNORE_TBL */
    if (!tblcb && dmve && dmve->dmve_log_rec)
    {
	found = rfp_locate_tblcb( rfp, &tblcb, &tabid);

	if (!found)
	{
	    local_dberr = jsx->jsx_dberr;

	    loc_stat = rfp_allocate_tblcb (rfp, &tblcb);
	    if (loc_stat != E_DB_OK)
	    {
		dmfWriteMsg(&jsx->jsx_dberr, 0, 0);
		return (E_DB_ERROR);
	    }
	    jsx->jsx_dberr = local_dberr;
	    STRUCT_ASSIGN_MACRO(tabid, tblcb->tblcb_table_id);
	    STRUCT_ASSIGN_MACRO(tabname, tblcb->tblcb_table_name);
	    STRUCT_ASSIGN_MACRO(ownname, tblcb->tblcb_table_owner);
	    tblcb->tblcb_table_status = RFP_FAILED_TABLE;
	    rfp_add_tblcb (rfp, tblcb);
	}
    }

    if (!tblcb)
	return (E_DB_ERROR);

    /*
    **  Add to the head of the invalid tables list
    */
    if ((tblcb->tblcb_table_status & RFP_IGNORED_ERR) == 0
	&& (tblcb->tblcb_table_status & RFP_IGNORED_DMP_JNL) == 0)
    {
	tblcb->tblcb_invq.q_next = hcb->tblhcb_hinvq.q_next;
	tblcb->tblcb_invq.q_prev = &hcb->tblhcb_hinvq;
	hcb->tblhcb_hinvq.q_next->q_prev = &tblcb->tblcb_invq;
	hcb->tblhcb_hinvq.q_next = &tblcb->tblcb_invq;
	jsx->jsx_status2 |= JSX2_INVALID_TBL_LIST;
	if (tabid.db_tab_index > 0)
	    jsx->jsx_ignored_idx_err = TRUE;
	else
	    jsx->jsx_ignored_tbl_err = TRUE;
    }

    /* 
    ** If table recovery - table was marked inconsistent and
    ** will only be marked consistent if no error occurred
    ** If db recovery - we need to mark this table inconsistent
    */
    tblcb->tblcb_table_status    |= err_type;
    tblcb->tblcb_table_err_code   = table_error;
    tblcb->tblcb_table_err_status = err_status;

    if (rfp_action == RFP_CONTINUE_IGNORE_ERR)
    {
	tblcb->tblcb_table_status |= RFP_IGNORED_ERR;

	TRformat(dmf_diag_put_line, 0, line_buffer, sizeof(line_buffer),
    "WARNING: Rollforward of %~t will continue, ignoring error for %~t.\n",
	    sizeof(DB_DB_NAME), 
	    rfp->rfp_dmve->dmve_dcb_ptr->dcb_name.db_db_name,
	    sizeof(DB_TAB_NAME), tblcb->tblcb_table_name.db_tab_name);

	return (E_DB_OK);
    }

    /* RFP_CONTINUE_IGNORE_TBL */
    tblcb->tblcb_table_status |= RFP_IGNORED_DMP_JNL;
    tblcb->tblcb_table_status &= ~RFP_RECOVER_TABLE;

    /*
    ** Remove from totq
    ** If JSX1_RECOVER_DB, leave tblcb on the totq so that 
    ** rfp_update_tbls_status called at the end of by rfp_db_complete
    ** can walk the totq (instead of invq) and mark this table invalid
    */
    if ( ( jsx->jsx_status1 & JSX1_RECOVER_DB ) == 0)
    {
	tblcb->tblcb_totq.q_next->q_prev = tblcb->tblcb_totq.q_prev;
	tblcb->tblcb_totq.q_prev->q_next = tblcb->tblcb_totq.q_next;
    }

    /*
    ** If on a hash queue then remove it
    */
    if ( (tblcb->tblcb_q_next != (RFP_TBLCB *)&tblcb->tblcb_q_next )
         || (tblcb->tblcb_q_prev != (RFP_TBLCB *)&tblcb->tblcb_q_next ))
    {

	/*
	** Table is on the total list of tables so remove
	*/
	tblcb->tblcb_q_prev->tblcb_q_next = tblcb->tblcb_q_next;
	tblcb->tblcb_q_next->tblcb_q_prev = tblcb->tblcb_q_prev;

    }

    TRformat(dmf_diag_put_line, 0, line_buffer, sizeof(line_buffer),
    "WARNING: Rollforward of %~t will continue, journal records for %~t will be skipped\n",
	sizeof(DB_DB_NAME), 
	rfp->rfp_dmve->dmve_dcb_ptr->dcb_name.db_db_name,
	sizeof(DB_TAB_NAME), tblcb->tblcb_table_name.db_tab_name);

    return (E_DB_OK);
}

static DB_STATUS
rfp_sconcur_error(
DMF_RFP             *rfp,
DMF_JSX             *jsx,
RFP_TBLCB	    *tblcb,
DMVE_CB		    *dmve,
i4		    err_type,
DB_STATUS	    err_status)
{
    DMP_DCB         *dcb = dmve->dmve_dcb_ptr;
    RFP_TBLHCB	    *hcb = rfp->rfp_tblhcb_ptr;
    DB_STATUS	    loc_stat;
    i4		    loc_err;
    char	    line_buffer[132];
    DB_TAB_ID	    tabid, sconcur_id;
    DB_TAB_NAME	    tabname, sconcur_name;
    DB_OWN_NAME	    ownname, sconcur_own;
    i4		    table_error = jsx->jsx_dberr.err_code;
    i4		    local_error;
    bool	    apply, found;
    i4		    rfp_action = RFP_ROLLBACK;
    bool	    verbose;
    bool	    sconcur = FALSE;
    char	    *old;
    DMP_RELATION    oldrel, newrel;
    DMP_RINDEX	    oldrindx, newrindx;
    DMP_ATTRIBUTE   oldatt, newatt;
    DMP_INDEX	    oldind, newind;
    DMP_DEVICE	    olddev, newdev;
    DM0L_HEADER	    *rec;
    DM0L_REP	    *rep;
    DM0L_DEL	    *del;
    char	    *new_row_log_info;
    DM_FILE	    table_file;
    DM_FILE	    ckp_file;
    i4		    ckp_sequence;
    i4		    loc_sequence;
    i4		    i;
    DB_ERROR	    local_dberr;

    CLRDBERR(&jsx->jsx_dberr);

    if (dmve)
	rec = (DM0L_HEADER *)dmve->dmve_log_rec;
    else
	rec = (DM0L_HEADER *)0;

    if (tblcb)
    {
	STRUCT_ASSIGN_MACRO(tblcb->tblcb_table_id, sconcur_id);
	STRUCT_ASSIGN_MACRO(tblcb->tblcb_table_name, sconcur_name);
	STRUCT_ASSIGN_MACRO(tblcb->tblcb_table_owner, sconcur_own);
    }
    else if (!tblcb && rec)
    {
	/* this must be database recovery - or we would already have tblcb */
	/* Call dmve_get_tab to extract table information from log record */
	dmve_get_tab((char *)rec, &sconcur_id, &sconcur_name, &sconcur_own);
    }
    else
    {
	return (E_DB_ERROR);
    }

    if (rec->type == DM0LREP)
    {
	rep = (DM0L_REP *)rec;
	old = &rep->rep_vbuf[rep->rep_tab_size + rep->rep_own_size];
	new_row_log_info = &rep->rep_vbuf[rep->rep_tab_size +
		    rep->rep_own_size + rep->rep_odata_len];
    }
    else if (rec->type == DM0LDEL)
    {
	del = (DM0L_DEL *)rec;
	old = &del->del_vbuf[del->del_tab_size + del->del_own_size];
    }
    else
    {
	return (E_DB_ERROR);
    }

    /* What table is this SCONCUR record associated with */
    tabid.db_tab_base = 0; /* Unknown */
    tabid.db_tab_index = 0; /* Unknown */
    MEfill(sizeof(DB_TAB_NAME), '\0', tabname.db_tab_name);
    MEfill(sizeof(DB_OWN_NAME), '\0', ownname.db_own_name);

    if (sconcur_id.db_tab_base == DM_B_RELATION_TAB_ID &&
			sconcur_id.db_tab_index == 0)
    {
	MEcopy(old, sizeof(DMP_RELATION), (PTR)&oldrel);
	STRUCT_ASSIGN_MACRO(oldrel.reltid, tabid);
	STRUCT_ASSIGN_MACRO(oldrel.relid, tabname);
	STRUCT_ASSIGN_MACRO(oldrel.relowner, ownname);

	if (rec->type == DM0LREP)
	{
	    dm0l_row_unpack((PTR)&oldrel, rep->rep_orec_size,
		    new_row_log_info, rep->rep_ndata_len,
		    (PTR)&newrel, rep->rep_nrec_size,
		    rep->rep_diff_offset);
	    if ((newrel.relstat & TCB_JOURNAL) 
			    && !(oldrel.relstat & TCB_JOURNAL))
	    {
		TRformat(dmf_diag_put_line, 0, 
		line_buffer, sizeof(line_buffer),
		"WARNING: Can't set journaling for non-journaled table %~t\n",
		sizeof(DB_TAB_NAME), oldrel.relid.db_tab_name);

		/* May need to restore non-jnl table from current ckp */
		if (rfp->rfp_jnl_history[rfp->jnl_history_number].ckp_sequence
			!= rfp->rfp_jnl_history[rfp->jnl_first].ckp_sequence)
		{
		    ckp_sequence = 
			rfp->rfp_jnl_history[rfp->jnl_history_number].ckp_sequence;

		    /* FIX ME check iidevices, print msg for each loc */
		    loc_sequence = 1;

		    if (MEcmp(oldrel.relloc.db_loc_name,
			(char *)&dcb->dcb_root->logical, sizeof(DB_LOC_NAME)))
		    {
			for (i = 0; i < dcb->dcb_ext->ext_count; i++)
			{
			    /*  Skip jnl, dmp, chp, root and aliased locations. */
			    if (dcb->dcb_ext->ext_entry[i].flags &
				(DCB_DUMP | DCB_JOURNAL | DCB_CHECKPOINT |
				DCB_ROOT | DCB_ALIAS | DCB_WORK | DCB_AWORK))
				continue;

			    loc_sequence++;

			    if (!MEcmp(oldrel.relloc.db_loc_name,
				    (char *)&dcb->dcb_ext->ext_entry[i].logical,
				    sizeof(DB_LOC_NAME)))
			       break;
			}
		    }

		    ckp_file.name[0] = 'c';
		    ckp_file.name[1] = (ckp_sequence / 1000) + '0';
		    ckp_file.name[2] = ((ckp_sequence / 100) % 10) + '0';
		    ckp_file.name[3] = ((ckp_sequence / 10) % 10) + '0';
		    ckp_file.name[4] = (ckp_sequence % 10) + '0';
		    ckp_file.name[5] = ( loc_sequence / 100) + '0';
		    ckp_file.name[6] = ((loc_sequence / 10) % 10) + '0';
		    ckp_file.name[7] = ( loc_sequence % 10) + '0';
		    ckp_file.name[8] = '.';
		    ckp_file.name[9] = 'c';
		    ckp_file.name[10] = 'k';
		    ckp_file.name[11] = 'p';

		    /* First location always .t00 */
		    dm2f_filename(DM2F_TAB, &tabid, 0, 0, 0, &table_file); 

		    TRformat(dmf_diag_put_line,
			0, line_buffer, sizeof(line_buffer),
			"WARNING: Table file %~t in %~t (not restored)\n",
			sizeof(table_file.name), &table_file.name,
			12, &ckp_file.name);

		    TRformat(dmf_diag_put_line,
			0, line_buffer, sizeof(line_buffer),
			"WARNING: Table file(s) for %~t in %d location(s)\n",
			sizeof(DB_TAB_NAME), oldrel.relid.db_tab_name,
			oldrel.relloccount);

		    /*
		    ** FIX ME
		    ** Prompt to see if we should restore table file(s)
		    ** Right now, user can manually restore at prompt
		    */
		}
	    }
	}
    }
    else if (sconcur_id.db_tab_base == DM_B_RELATION_TAB_ID && 
			sconcur_id.db_tab_index == DM_I_RIDX_TAB_ID)
    {
	MEcopy(old, sizeof(DMP_RINDEX), (PTR)&oldrindx);
	STRUCT_ASSIGN_MACRO(oldrindx.relname, tabname);
	STRUCT_ASSIGN_MACRO(oldrindx.relowner, ownname);
    }
    else if (sconcur_id.db_tab_base == DM_B_ATTRIBUTE_TAB_ID && 
			sconcur_id.db_tab_index == 0)
    {
	MEcopy(old, sizeof(DMP_ATTRIBUTE), (PTR)&oldatt);
	STRUCT_ASSIGN_MACRO(oldatt.attrelid, tabid);
    }
    else if (sconcur_id.db_tab_base == DM_B_INDEX_TAB_ID && 
			sconcur_id.db_tab_index == 0)
    {
	MEcopy(old, sizeof(DMP_INDEX), (PTR)&oldind);	
	tabid.db_tab_base = oldind.baseid;
	tabid.db_tab_base = 0; /* associate with base table */
    }
    else if (sconcur_id.db_tab_base == DM_B_DEVICE_TAB_ID && 
			sconcur_id.db_tab_index == 0)
    {
	MEcopy(old, sizeof(DMP_DEVICE), (PTR)&olddev);
	STRUCT_ASSIGN_MACRO(olddev.devreltid, tabid);
    }
    else
    {
	return (E_DB_ERROR);
    }

    if (tabid.db_tab_base)
    {
	found = rfp_locate_tblcb( rfp, &tblcb, &tabid);
	if (found && !tabname.db_tab_name[0])
	    STRUCT_ASSIGN_MACRO(tblcb->tblcb_table_name, tabname);
	if (found && !ownname.db_own_name[0])
	    STRUCT_ASSIGN_MACRO(tblcb->tblcb_table_owner, ownname);
    }

    /* Need tabid, tabname and ownname to handle this error */
    if (!tabid.db_tab_base || !tabname.db_tab_name[0] || !ownname.db_own_name[0])
    {
	return(E_DB_ERROR);
    }

    TRformat(dmf_diag_put_line, 0, line_buffer, sizeof(line_buffer),
	    "Error during recovery of table %~t for (%~t, %~t)\n",
	    sizeof(DB_TAB_NAME), sconcur_name.db_tab_name,
	    sizeof(DB_TAB_NAME), tabname.db_tab_name,
	    sizeof(DB_OWN_NAME), ownname.db_own_name);

    if (jsx->jsx_status2 & JSX2_ON_ERR_PROMPT)
    {
	/* Prompt for rfp_action */
	loc_stat = rfp_prompt(rfp, jsx, &rfp_action);
	if (loc_stat == E_DB_ERROR || rfp_action == RFP_ROLLBACK)
	    return (E_DB_ERROR);
    }
    else if (jsx->jsx_status1 & JSX1_ON_ERR_CONT)
    {
	/* Continue rollforwarddb, but ignore this table */
	rfp_action = RFP_CONTINUE_IGNORE_TBL;
    }
    else if (jsx->jsx_status1 & JSX1_IGNORE_FLAG)
    {
	/* Continue rfp, ignore this error */
	rfp_action = RFP_CONTINUE_IGNORE_ERR;
    }

    if (rfp_action == RFP_ROLLBACK)
    {
	return (E_DB_ERROR);
    }

    /* Here for RFP_CONTINUE_IGNORE_ERR, RFP_CONTINUE_IGNORE_TBL */
    if (!tblcb)
    {
        local_dberr = jsx->jsx_dberr;

	loc_stat = rfp_allocate_tblcb (rfp, &tblcb);
	if (loc_stat != E_DB_OK)
	{
	    dmfWriteMsg(&jsx->jsx_dberr, 0, 0);
	    return (E_DB_ERROR);
	}
	jsx->jsx_dberr = local_dberr;
	STRUCT_ASSIGN_MACRO(tabid, tblcb->tblcb_table_id);
	STRUCT_ASSIGN_MACRO(tabname, tblcb->tblcb_table_name);
	STRUCT_ASSIGN_MACRO(ownname, tblcb->tblcb_table_owner);
	tblcb->tblcb_table_status = RFP_FAILED_TABLE;
	rfp_add_tblcb (rfp, tblcb);
    }

    if (!tblcb)
	return (E_DB_ERROR);

    /*
    **  Add to the head of the invalid tables list
    */
    if ((tblcb->tblcb_table_status & RFP_IGNORED_ERR) == 0
	&& (tblcb->tblcb_table_status & RFP_IGNORED_DMP_JNL) == 0)
    {
	tblcb->tblcb_invq.q_next = hcb->tblhcb_hinvq.q_next;
	tblcb->tblcb_invq.q_prev = &hcb->tblhcb_hinvq;
	hcb->tblhcb_hinvq.q_next->q_prev = &tblcb->tblcb_invq;
	hcb->tblhcb_hinvq.q_next = &tblcb->tblcb_invq;
	jsx->jsx_status2 |= JSX2_INVALID_TBL_LIST;
	if (tabid.db_tab_index > 0)
	    jsx->jsx_ignored_idx_err = TRUE;
	else
	    jsx->jsx_ignored_tbl_err = TRUE;
    }

    /* 
    ** If table recovery - table was marked inconsistent and
    ** will only be marked consistent if no error occurred
    ** If db recovery - we need to mark this table inconsistent
    */
    tblcb->tblcb_table_status    |= err_type;
    tblcb->tblcb_table_err_code   = table_error;
    tblcb->tblcb_table_err_status = err_status;

    if (rfp_action == RFP_CONTINUE_IGNORE_ERR)
    {
	tblcb->tblcb_table_status |= RFP_IGNORED_ERR;

	TRformat(dmf_diag_put_line, 0, line_buffer, sizeof(line_buffer),
    "WARNING: Rollforward of %~t will continue, ignoring error for %~t.\n",
	    sizeof(DB_DB_NAME), dcb->dcb_name.db_db_name,
	    sizeof(DB_TAB_NAME), tblcb->tblcb_table_name.db_tab_name);

	return (E_DB_OK);
    }

    /* RFP_CONTINUE_IGNORE_TBL */
    tblcb->tblcb_table_status |= RFP_IGNORED_DMP_JNL;
    tblcb->tblcb_table_status &= ~RFP_RECOVER_TABLE;

    /*
    ** Remove from totq
    ** If JSX1_RECOVER_DB, leave tblcb on the totq so that 
    ** rfp_update_tbls_status called at the end of by rfp_db_complete
    ** can walk the totq (instead of invq) and mark this table invalid
    */
    if ( ( jsx->jsx_status1 & JSX1_RECOVER_DB ) == 0)
    {
	tblcb->tblcb_totq.q_next->q_prev = tblcb->tblcb_totq.q_prev;
	tblcb->tblcb_totq.q_prev->q_next = tblcb->tblcb_totq.q_next;
    }

    /*
    ** If on a hash queue then remove it
    */
    if ( (tblcb->tblcb_q_next != (RFP_TBLCB *)&tblcb->tblcb_q_next )
         || (tblcb->tblcb_q_prev != (RFP_TBLCB *)&tblcb->tblcb_q_next ))
    {

	/*
	** Table is on the total list of tables so remove
	*/
	tblcb->tblcb_q_prev->tblcb_q_next = tblcb->tblcb_q_next;
	tblcb->tblcb_q_next->tblcb_q_prev = tblcb->tblcb_q_prev;

    }

    TRformat(dmf_diag_put_line, 0, line_buffer, sizeof(line_buffer),
    "WARNING: Rollforward of %~t will continue, journal records for %~t will be skipped\n",
	sizeof(DB_DB_NAME), 
	rfp->rfp_dmve->dmve_dcb_ptr->dcb_name.db_db_name,
	sizeof(DB_TAB_NAME), tblcb->tblcb_table_name.db_tab_name);

    return (E_DB_OK);
}

/*{
** Name: rfp_allocate_tblcb - Allocate a new tbl control block.
**
** Description:
**
**
** Inputs:
**	rfp
**
** Outputs:
**      err_code                        Reason for error code.
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**
** History:
**	14-sep-1994 (andyw)
**	    Partial Backup/Recovery Project:
**		Created.
**      22-nov-1994 (juliet)
**          Added ule_format calls to report errors.
**      19-oct-1995 (stial01)
**          relation tuple buffer allocated incorrectly.
**      15-jun-1999 (hweho01)
**          Corrected argument mismatch in ule_format() call,
**          the first one should be an integer, not a pointer.
*/
static DB_STATUS
rfp_allocate_tblcb(
DMF_RFP             *rfp,
RFP_TBLCB	    **tablecb)
{
    DMF_JSX	    *jsx = rfp->rfp_jsx;
    RFP_TBLCB	    *tblcb;
    DB_STATUS	    status;
    i4		    error;

    CLRDBERR(&jsx->jsx_dberr);

    status = dm0m_allocate((i4)
		(sizeof( RFP_TBLCB ) + sizeof( DMP_RELATION )),
            (i4)DM0M_ZERO, (i4)TBL_CB, (i4)RFP_TBLCB_ASCII_ID,
            (char *)rfp, (DM_OBJECT **)&tblcb, &jsx->jsx_dberr);
    if (status != E_DB_OK)
    {
	dmfWriteMsg(&jsx->jsx_dberr, 0, 0);
        uleFormat(&jsx->jsx_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,(char *)NULL,
                (i4)0, NULL, &error, 0);
	/* already logged */
	CLRDBERR(&jsx->jsx_dberr);
        return( E_DB_ERROR );
    }

    tblcb->tblcb_table_status = 0;
    tblcb->tblcb_tcb_ptr= 0;

    tblcb->tblcb_q_next      =  (RFP_TBLCB *)&tblcb->tblcb_q_next;
    tblcb->tblcb_q_prev      =  (RFP_TBLCB *)&tblcb->tblcb_q_next;
    tblcb->tblcb_totq.q_next = &tblcb->tblcb_totq;
    tblcb->tblcb_totq.q_prev = &tblcb->tblcb_totq;
    tblcb->tblcb_invq.q_next = &tblcb->tblcb_invq;
    tblcb->tblcb_invq.q_prev = &tblcb->tblcb_invq;

    *tablecb = tblcb;

    return( E_DB_OK );

}

/*{
** Name: rfp_add_tblcb - Add tblcb to relevent structures
**
** Description:
**
**
** Inputs:
**	rfp
**
** Outputs:
**      err_code                        Reason for error code.
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**
** History:
**	14-sep-1994 (andyw)
**	    Partial Backup/Recovery Project:
**		Created.
**      23-mar-1995 (stial01)
**          Don't fix prev pointer of htotalq unless there
**          is an element on the htotalq. Otherwise we trash 
**          hcb->tblhcb_hash_array[3].
**	06-jun-1997 (nanpr01)
**	    Fix queue handling.
*/
static VOID
rfp_add_tblcb(
DMF_RFP             *rfp,
RFP_TBLCB	    *tblcb )
{
    RFP_TBLHCB	        *hcb = rfp->rfp_tblhcb_ptr;
    TBL_HASH_ENTRY	*h;

    /*
    ** Put on hash bucket queue
    */
    h = &hcb->tblhcb_hash_array[ tblcb->tblcb_table_id.db_tab_base %
				 hcb->tblhcb_no_hash_buckets ];

    /*
    ** setup the new tblcb next and previous ptrs
    */
    tblcb->tblcb_q_next = h->tblhcb_headq_next;
    tblcb->tblcb_q_prev = (RFP_TBLCB *)&h->tblhcb_headq_next;

    /*
    ** As adding to head of the list make last head element point to
    ** new element
    */
    h->tblhcb_headq_next->tblcb_q_prev = tblcb;

    /*
    ** Make the head of the list point to the new element
    */

    h->tblhcb_headq_next = tblcb;

    /*
    ** Add to the head of the total list of tables queue
    */
    tblcb->tblcb_totq.q_next = hcb->tblhcb_htotq.q_next;
    tblcb->tblcb_totq.q_prev = &hcb->tblhcb_htotq;
    hcb->tblhcb_htotq.q_next->q_prev = &tblcb->tblcb_totq;
    hcb->tblhcb_htotq.q_next = &tblcb->tblcb_totq;

    return;
}

/*{
** Name: rfp_locate_tblcb - Locate a tblcb via table_id.
**
** Description:
**
**
** Inputs:
**	rfp
**
** Outputs:
**      err_code                        Reason for error code.
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**
** History:
**	14-sep-1994 (andyw)
**	    Partial Backup/Recovery Project:
**		Created.
*/
static bool
rfp_locate_tblcb(
DMF_RFP             *rfp,
RFP_TBLCB	    **tblcb,
DB_TAB_ID	    *tabid)
{
    RFP_TBLHCB	        *hcb = rfp->rfp_tblhcb_ptr;
    TBL_HASH_ENTRY	*h;
    RFP_TBLCB		*t;


    /*
    ** determine hash bucket queue
    */
    h = &hcb->tblhcb_hash_array[ tabid->db_tab_base %
				 hcb->tblhcb_no_hash_buckets ];

    /*
    ** Search the tblcb list chained of this bucket for a tblcb matching
    ** the requested table
    */
    for ( t = h->tblhcb_headq_next;
	  t != (RFP_TBLCB*)&h->tblhcb_headq_next;
          t = t->tblcb_q_next )
    {
	if ((t->tblcb_table_id.db_tab_base == tabid->db_tab_base)
				&&
	    (t->tblcb_table_id.db_tab_index == tabid->db_tab_index))
	{
	    /*
	    ** Table found
	    */
	    *tblcb = t;
	    return( TRUE );

	}
    }

    /*
    ** Table not found
    */
    return( FALSE );
}


/*{
** Name: rfp_update_tbls_status
**
** Description:
**
**
** Inputs:
**	rfp
**
** Outputs:
**      err_code                        Reason for error code.
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**
** History:
**	14-sep-1994 (andyw)
**	    Partial Backup/Recovery Project:
**		Created.
**      22-nov-1994 (juliet)
**          Added ule_format calls to report errors.
**          Created error RFP_CANNOT_OPEN_IIRELATION.
**	22-nov-1994 (andyw)
**	    Changed to implement transaction handling around any
**	    system catalog updates.
**      23-jan-1995 (stial01)
**          BUG 66577: If -noblobs or -nosecondary_index was specified, there 
**          may be blob tables or secondary indexes that need to be recovered,
**          but won't be. Mark these blob tables and secondary indexes
**          physically inconsistent.
**      11-may-1995 (thaju02)
**          bug #68588 - table rolldb to a point in time marks table as
**          logically inconsistent.  Upon table rolldb "to now" should
**          reset logically inconsistent status bit.  See Partial 
**          Recovery: Table Level Rollforwarddb paper (R. Muth, Feb. 94),
**          Section 2.2 New Table States
**      20-july-1995 (thaju02)
**          bug #69981 - when ignore argument specified, output error
**          message to error log, and mark database inconsistent
**          for db level rolldb or table physically inconsistent
**          for tbl level rolldb, since rolldb will continue with
**          application of journal records.
**	16-jan-1997 (nanpr01)
**	    Duplicate messages are being displayed for errors . #80068.
**	21-jan-1997 (nanpr01)
**	    If rollforward -e is specified, notonly mark the base table
**	    logically inconsistent but also make the secondary inconsistent
**	    too. #80163
**	01-oct-1998 (nanpr01)
**	    Added new paramter to dm2r_replace for update performance 
**	    improvement.
**	14-Jan-2004 (jenjo02)
**	    Check db_tab_index > 0 for index, not simply != 0.
**	    Check relstat & TCB_INDEX rather than reltid.db_tab_index.
*/
static DB_STATUS
rfp_update_tbls_status(
DMF_RFP             *rfp,
DMF_JSX             *jsx,
DMP_DCB             *dcb,
u_i4	    phase)
{
    RFP_QUEUE		*totq;
    RFP_TBLCB		*tblcb = 0;
    RFP_TBLHCB		*hcb = rfp->rfp_tblhcb_ptr;
    DM2R_KEY_DESC       qual_list[1];
    DMP_RCB		*iirel_rcb = 0;
    DB_STATUS		status;
    DB_TRAN_ID          tran_id;
    DMP_RELATION	rel_record;
    DM_TID		tid;
    DB_TAB_TIMESTAMP	stamp;
    DB_OWN_NAME		username;
    DB_TAB_ID		iirelation;
    i4		dmxe_flag = DMXE_JOURNAL;
    i4             lock_id;
    i4             log_id = 0;
    i4		dbdb_flag = 0;
    i4             process_status;
    char                line_buffer[132];
    i4			error;

    CLRDBERR(&jsx->jsx_dberr);


    if ( jsx->jsx_status1 & JSX1_ONLINE)
    {
	/*
	** Online recovery assumes that the tables have already
	** been marked physically inconsistent. FIXME
	*/
	return( E_DB_ERROR );
    }

    {
        char        *cp = (char *)&username;

        IDname(&cp);
        MEmove(STlength(cp), cp, ' ', sizeof(username), username.db_own_name);
    }

    dbdb_flag = DM2D_NLK;
    if ((jsx->jsx_status & JSX_WAITDB) == 0)    /* b 66533 */
        dbdb_flag |= DM2D_NOWAIT;

    status = rfp_open_database(rfp, jsx, dcb, dbdb_flag);

    if (status != E_DB_OK)
    {
        return (status);
    }

    if (jsx->jsx_status & JSX_JOFF)
         dmxe_flag = 0;

    status = dmf_rfp_begin_transaction(DMXE_WRITE,
                                       dmxe_flag,
                                       dcb,
                                       dcb->dcb_log_id,
                                       &username,
                                       dmf_svcb->svcb_lock_list,
                                       &log_id,
                                       &tran_id,
                                       &lock_id,
                                       &jsx->jsx_dberr);

    /*
    ** Open iirelation
    */
    iirelation.db_tab_base = DM_B_RELATION_TAB_ID;
    iirelation.db_tab_index = 0;

    status = dm2t_open(dcb, &iirelation, DM2T_IX, DM2T_UDIRECT,
                       DM2T_A_WRITE, 0, 20, 0, log_id, lock_id,
                       0, 0, DM2T_X, &tran_id, &stamp, &iirel_rcb, (DML_SCB *)0,
		       &jsx->jsx_dberr);

    if ( status != E_DB_OK )
    {
        /* error during the opening of the iirelation catalog */
	uleFormat( &jsx->jsx_dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL, &error, 0);
	/* already logged */
	CLRDBERR(&jsx->jsx_dberr);
        dmfWriteMsg(NULL, E_DM135C_RFP_CANNOT_OPEN_IIRELATION, 0);
	return (status);
    }

    do
    {
	iirel_rcb->rcb_lk_mode = RCB_K_IX;
	iirel_rcb->rcb_access_mode = RCB_A_WRITE;

	/*
	** Turn off logging, this operation is NONREDO and cannot be
	** UNDOne.
	*/
	iirel_rcb->rcb_logging = 0;


	/*
	** Build a qualification list used for searching the system tables.
	*/
	qual_list[0].attr_number = DM_1_RELATION_KEY;
	qual_list[0].attr_operator = DM2R_EQ;

	if ( phase == RFP_PREPARE_UPDATE_TBLS_STATUS )
	{
	    /*
	    ** Mark all tables we will recover physically inconsistent
	    ** Any indexes or blobs for tables being recovered should
	    ** also be marked inconsistent, regardless of whether
	    ** or not they are recovered.
	    */
	    process_status = RFP_RECOVER_TABLE | RFP_INDEX_REQUIRED
					| RFP_BLOB_REQUIRED;	
	}
	else if (phase == RFP_COMPLETE_UPDATE_TBLS_STATUS)
	{
	    /*
	    ** Mark all tables we recovered consistent
	    */
	    process_status = RFP_RECOVER_TABLE;
	}
	else if (phase == RFP_UPDATE_INVALID_TBLS_STATUS)
	{
	    process_status = (RFP_IGNORED_DMP_JNL | RFP_IGNORED_ERR);
	}

	/*
	** Loop through all the table and if not already marked physically
	** inconsistent then do it....
	*/
	for ( totq = hcb->tblhcb_htotq.q_next;
	      totq != &hcb->tblhcb_htotq;
	      totq = tblcb->tblcb_totq.q_next
	      )
	{
	    tblcb = (RFP_TBLCB *)((char *)totq - 
				CL_OFFSETOF(RFP_TBLCB, tblcb_totq));

	    /*
	    ** Skip tables not matching process_status above
	    */
	    if ((tblcb->tblcb_table_status & process_status ) == 0)
		continue;

	    qual_list[0].attr_value = (char *)&tblcb->tblcb_table_id;

	    /*
	    ** Position on iirelation
	    */
	    status = dm2r_position( iirel_rcb, DM2R_QUAL, qual_list,
			     (i4)1,
			     (DM_TID *)0, &jsx->jsx_dberr);
	    if (status != E_DB_OK)
		break;


	    /*
	    ** Look for record in iirelation
	    */
	    for (;;)
	    {
		status = dm2r_get( iirel_rcb, &tid, DM2R_GETNEXT,
				   (char *)&rel_record, &jsx->jsx_dberr);
		if (status != E_DB_OK)
		{
		    if (jsx->jsx_dberr.err_code == E_DM0055_NONEXT)
		    {
			status = E_DB_OK;
			CLRDBERR(&jsx->jsx_dberr);
		    }
		    break;
		}

		/*
		** Check table, need to match on both base and index
		*/
		if ((rel_record.reltid.db_tab_base !=
				    tblcb->tblcb_table_id.db_tab_base)
				    ||
		    (rel_record.reltid.db_tab_index !=
				    tblcb->tblcb_table_id.db_tab_index))
		{
		    continue;
		}

		/*
		** Update physically inconsistent bit accordingly
		*/
		if ( phase == RFP_PREPARE_UPDATE_TBLS_STATUS  )
		{
		    rel_record.relstat2 |= TCB2_PHYS_INCONSISTENT;
		}
		else if ( phase == RFP_COMPLETE_UPDATE_TBLS_STATUS )
		{
                    /* bug #69981, 20-july-1995 (thaju02)
                    ** -ignore flag, and errors were ignored
                    ** then mark table as inconsistent.
                    */
                    if ((tblcb->tblcb_table_status & RFP_IGNORED_ERR) == 0 &&
			(tblcb->tblcb_table_status & RFP_IGNORED_DMP_JNL) == 0)
                        rel_record.relstat2 &= ~TCB2_PHYS_INCONSISTENT;
		}
		else if (phase == RFP_UPDATE_INVALID_TBLS_STATUS)
		{
		    rel_record.relstat2 |= TCB2_PHYS_INCONSISTENT;
		    rel_record.relstat2 |= TCB2_LOGICAL_INCONSISTENT;
		    TRformat(dmf_diag_put_line, 0, 
		    line_buffer, sizeof(line_buffer),
		    "WARNING: Marking %~t inconsistent\n",
			sizeof(tblcb->tblcb_table_name),
			tblcb->tblcb_table_name.db_tab_name);
		}


		/*
		** If a base table and point-in-time recovery then mark the
		** table as logically inconsistent.
		*/
		if ( rfp->rfp_point_in_time &&
		     (phase == RFP_PREPARE_UPDATE_TBLS_STATUS)) 
		{
		    rel_record.relstat2 |= TCB2_LOGICAL_INCONSISTENT;
		}
		else if ((phase == RFP_PREPARE_UPDATE_TBLS_STATUS) &&
		     (rel_record.relstat & TCB_INDEX) == 0)
		{
		    /* bug #68588 - table was previously marked logically 
		    ** inconsistent due to point-in-time recovery.
		    ** Must remove logically inconsistent
		    ** status upon rolldb "to now"
		    */
		    rel_record.relstat2 &= ~TCB2_LOGICAL_INCONSISTENT;
		}

		status = dm2r_replace( iirel_rcb, &tid, DM2R_BYPOSITION,
				     (char *)&rel_record, (char *)NULL, &jsx->jsx_dberr);
		if ( status != E_DB_OK )
		    break;

	    }

	    /*
	    ** If an error then bail
	    */
	    if ( status != E_DB_OK )
		break;

	}

	/*
	** If an error bail
	*/
	if ( status != E_DB_OK )
	    break;

    } while (FALSE);

    status = dm2t_close(iirel_rcb, DM2T_NOPURGE, &jsx->jsx_dberr);
    if ( status != E_DB_OK )
    {
        /*
        ** report error closing iirelation table.
        */
	dmfWriteMsg(&jsx->jsx_dberr, 0, 0);
    }

    status = dmf_rfp_commit_transaction(&tran_id, log_id, lock_id,
                                        dmxe_flag, &stamp, &jsx->jsx_dberr);
    if ( status != E_DB_OK)
    {
	dmfWriteMsg(&jsx->jsx_dberr, 0, 0);
	return (status);
    }

    /*
    ** Close the database
    */
    status = rfp_close_database (rfp, dcb, dbdb_flag);
    if (status != E_DB_OK)
    {
	dmfWriteMsg(&jsx->jsx_dberr, 0, 0);
	return (status);
    }

    return( status );
}

/*{
** Name: rfp_display_recovery_tables
**
** Description:
**
**
** Inputs:
**	rfp
**	jsx				Pointer to journal support context.
**
** Outputs:
**      err_code                        Reason for error code.
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	    E_DB_INFO			If failed to find information on
**					a table.
**	Exceptions:
**	    none
**
** Side Effects:
**
** History:
**	14-sep-1994 (andyw)
**	    Partial Backup/Recovery Project:
**		Created.
*/
static VOID
rfp_display_recovery_tables(
DMF_RFP             *rfp,
DMF_JSX             *jsx )
{
    char		line_buffer[132];
    RFP_QUEUE		*totq;
    RFP_TBLCB           *tblcb = 0;
    RFP_TBLHCB          *hcb = rfp->rfp_tblhcb_ptr;
    bool		none = TRUE;

    TRformat( dmf_put_line, 0, line_buffer, sizeof(line_buffer),
                    "The following tables will be recovered : ");

    for ( totq = hcb->tblhcb_htotq.q_next;
          totq != &hcb->tblhcb_htotq;
          totq = tblcb->tblcb_totq.q_next
          )
    {
	tblcb = (RFP_TBLCB *)((char *)totq - CL_OFFSETOF(RFP_TBLCB, tblcb_totq));

        /*
	** If not a table the user passed in on the command line
	** then skip it.
        */
        if ((tblcb->tblcb_table_status & RFP_RECOVER_TABLE ) == 0)
            continue;

	TRformat( dmf_put_line, 0, line_buffer, sizeof(line_buffer),
		       " %~t\n", DB_TAB_MAXNAME, &tblcb->tblcb_table_name );

	none = FALSE;

    }

    /*
    ** If no tables then say so
    */
    if (none)
    {
        TRformat( dmf_put_line, 0, line_buffer, sizeof(line_buffer),
 		" NONE\n");
    }
}

/*{
** Name: rfp_display_no_recovery_tables
**
** Description:
**
**
** Inputs:
**	rfp
**	jsx				Pointer to journal support context.
**
** Outputs:
**      err_code                        Reason for error code.
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	    E_DB_INFO			If failed to find information on
**					a table.
**	Exceptions:
**	    none
**
** Side Effects:
**
** History:
**	14-sep-1994 (andyw)
**	    Partial Backup/Recovery Project:
**		Created.
*/
static VOID
rfp_display_no_recovery_tables(
DMF_RFP             *rfp,
DMF_JSX             *jsx )
{
    char		line_buffer[132];
    RFP_QUEUE		*totq;
    RFP_TBLCB           *tblcb = 0;
    RFP_TBLHCB          *hcb = rfp->rfp_tblhcb_ptr;
    bool		none = TRUE;

    TRformat( dmf_put_line, 0, line_buffer, sizeof(line_buffer),
 "Additional tables required to maintain physical consistency of the database :"
	     );

    for ( totq = hcb->tblhcb_htotq.q_next;
          totq != &hcb->tblhcb_htotq;
          totq = tblcb->tblcb_totq.q_next
          )
    {
	tblcb = (RFP_TBLCB *)((char *)totq - CL_OFFSETOF(RFP_TBLCB, tblcb_totq));

        /*
	** If not a table the user passed in on the command line
	** then skip it.
        */
	if (tblcb->tblcb_table_status & RFP_RECOVER_TABLE)
	    continue;

	/*
	** Skip catalogs, views and gateway tables
	*/
	if (tblcb->tblcb_table_status & 
		(RFP_CATALOG | RFP_VIEW | RFP_GATEWAY | RFP_PARTITION))
            continue;

	none = FALSE;

	TRformat( dmf_put_line, 0, line_buffer, sizeof(line_buffer),
		       " %~t\n", DB_TAB_MAXNAME, &tblcb->tblcb_table_name );

    }

    /*
    ** If no tables then say so
    */
    if (none)
    {
        TRformat( dmf_put_line, 0, line_buffer, sizeof(line_buffer),
 		" NONE\n");
    }
}

/*{
** Name: rfp_display_inv_tables
**
** Description:
**
**
** Inputs:
**	rfp
**	jsx				Pointer to journal support context.
**
** Outputs:
**      err_code                        Reason for error code.
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	    E_DB_INFO			If failed to find information on
**					a table.
**	Exceptions:
**	    none
**
** Side Effects:
**
** History:
**	14-sep-1994 (andyw)
**	    Partial Backup/Recovery Project:
**		Created.
*/
static VOID
rfp_display_inv_tables(
DMF_RFP             *rfp,
DMF_JSX             *jsx )
{
    char		line_buffer[132];
    RFP_QUEUE		*invq;
    RFP_TBLCB           *tblcb = 0;
    RFP_TBLHCB          *hcb = rfp->rfp_tblhcb_ptr;
    bool		none = TRUE;

    TRformat( dmf_put_line, 0, line_buffer, sizeof(line_buffer),
                    "Tables found to be invalid : ");


    for ( invq = hcb->tblhcb_hinvq.q_next;
	  invq != &hcb->tblhcb_hinvq;
	  invq = tblcb->tblcb_invq.q_next)
    {

	tblcb = (RFP_TBLCB *)((char *)invq - 
			    CL_OFFSETOF(RFP_TBLCB, tblcb_invq));

        /*
	** If not an invalid table then skip it
        */
	if ((tblcb->tblcb_table_status & RFP_IGNORED_DMP_JNL ) == 0 &&
		(tblcb->tblcb_table_status & RFP_IGNORED_ERR ) == 0)
            continue;

	none = FALSE;

	TRformat( dmf_diag_put_line, 0, line_buffer, sizeof(line_buffer),
		       "Invalid table %~t owned by %~t (%d,%d) - err_code: %d\n",
		       DB_TAB_MAXNAME, tblcb->tblcb_table_name.db_tab_name,
		       DB_OWN_MAXNAME, tblcb->tblcb_table_owner.db_own_name,
		       tblcb->tblcb_table_id.db_tab_base,
		       tblcb->tblcb_table_id.db_tab_index,
		       tblcb->tblcb_table_err_code );

    }

    /*
    ** If no tables then say so
    */
    if (none)
    {
        TRformat( dmf_put_line, 0, line_buffer, sizeof(line_buffer),
 		" NONE\n");
    }
}

/*{
** Name: rfp_ckp_processing
**
** Description:
**
**
** Inputs:
**	rfp
**	jsx				Pointer to journal support context.
**	dcb				Pointer to DCB for database.
**
** Outputs:
**      err_code                        Reason for error code.
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**
** History:
**	14-sep-1994 (andyw)
**	    Partial Backup/Recovery Project:
**		Created.
*/
static DB_STATUS
rfp_ckp_processing(
DMF_RFP             *rfp,
DMF_JSX             *jsx,
DMP_DCB             *dcb)
{
    DB_STATUS status;
    DMCKP_CB  *d = &rfp->rfp_dmckp;
    PTR       tptr;
    char      tbuf[2*MAX_LOC];
    char      username[DB_OWN_MAXNAME];
    char      *cp = username;
    char      line_buffer[132];
    i4	      error;

    CLRDBERR(&jsx->jsx_dberr);

    if ( jsx->jsx_status1 & JSX1_RECOVER_DB )
    {
	/*
	** Database level recovery
	*/
	status = rfp_db_ckp_processing( rfp, jsx, dcb );
    }
    else
    {
	/*
	** Table level recovery
	*/
	status = rfp_tbl_ckp_processing( rfp, jsx, dcb );
    }

    if ((status != E_DB_OK) && (jsx->jsx_dberr.err_code == I_DM1369_RFP_NOT_DEF_TEMPLATE))
    {
	i4	l;
	char defowner[DB_OWN_MAXNAME+1]; /* owner of cktmplt.def */
	char *def_cp = &defowner[0];


	MEfill(sizeof(tbuf), '\0', tbuf);
	MEfill(sizeof(username), '\0', username);
	LOtos(&(d->dmckp_cktmpl_loc), &tptr);
        l = STlength(tptr);
	MEcopy(tptr, l, tbuf);
	IDname(&cp);

#ifdef CK_OWNNOTING
	/* get owner of cktmplt.def */
	(VOID)IDename(&def_cp);
#else
          STcopy(def_cp, "ingres");
#endif

	uleFormat(&jsx->jsx_dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL, &error, 3,
	    sizeof(dcb->dcb_name), dcb->dcb_name,
	    STlength(username), username,
	    l, tbuf);
	/* already logged */
	CLRDBERR(&jsx->jsx_dberr);

	TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
        "%@ CPP: Template file %t not owned by user %s\n",
	l, tbuf, defowner);
    }
    return( status );
}

/*{
** Name:
**
** Description: rfp_db_ckp_processing
**
**
** Inputs:
**	rfp
**	jsx				Pointer to journal support context.
**	dcb				Pointer to DCB for database.
**
** Outputs:
**      err_code                        Reason for error code.
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**
** History:
**	14-sep-1994 (andyw)
**          Partial Backup/Recovery Project:
**	        Created.
*/
static DB_STATUS
rfp_db_ckp_processing(
DMF_RFP             *rfp,
DMF_JSX             *jsx,
DMP_DCB             *dcb)
{
    DB_STATUS	status;

    CLRDBERR(&jsx->jsx_dberr);

    do
    {

        if (jsx->jsx_status & JSX_TRACE)
	    TRdisplay("%@ RFP: Restore ckp saveset\n");

	status = db_restore_ckp(rfp, jsx, dcb);
	if (status != E_DB_OK)
	    break;

	/*
	** Patch the database config file to restore the checkpoint, dump,
	** and journal history.  This is required since the config file
	** restored by the checkpoint is the OLD one - which has old journal
	** information that was valid back when the checkpoint was taken.
	**
	** The new config file information is critical before we get to the
	** journal processing below to ensure that the archiver does not
	** write over any existing journal information or try to re-archive
	** stuff which has already been journaled.
	**
	** This restores the config file information which tracks
	** interesting tidbits like:
	**
	**	   The database JOURNAL state;
	**	   The database last table id;
	**	   The last journal file written to;
	**	   The current journal file EOF;
	**	   The Log Address to which the database was last archived;
	**	   The next dump file to use;
	*/
	status = db_post_ckp_config( rfp, jsx, dcb );
	if (status != E_DB_OK)
	    break;

    } while (FALSE);

    return( status );
}


/*{
** Name: rfp_tbl_ckp_processing
**
** Description:
**
**
** Inputs:
**	rfp
**	jsx				Pointer to journal support context.
**	dcb				Pointer to DCB for database.
**
** Outputs:
**      err_code                        Reason for error code.
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**
** History:
**	14-sep-1994 (andyw)
**          Partial Backup/Recovery Project:
**              Created.
**	16-jan-1997 (nanpr01)
**	    Duplicate messages are being displayed for errors . #80068.
**	23-june-1998(angusm)
**	    Ensure status returned from rfp_tbl_restore_ckp() is passed
**	    back.
*/
static DB_STATUS
rfp_tbl_ckp_processing(
DMF_RFP             *rfp,
DMF_JSX             *jsx,
DMP_DCB             *dcb)
{
    DB_STATUS		status;
    DB_STATUS		lstatus;
    DB_OWN_NAME		username;
    i4		dbdb_flag = DM2D_NLK;
    DB_ERROR		local_dberr;

    CLRDBERR(&jsx->jsx_dberr);

    {
        char        *cp = (char *)&username;

        IDname(&cp);
        MEmove(STlength(cp), cp, ' ', sizeof(username), username.db_own_name);
    }

    if ((jsx->jsx_status & JSX_WAITDB) == 0)    /* b66533 */
	dbdb_flag |= DM2D_NOWAIT;

    status = rfp_open_database (rfp, jsx, dcb, dbdb_flag);

    if (status != E_DB_OK)
    {
        return (status);
    }

    do
    {
	/*
	** Restore the underlying files
	*/
	lstatus = rfp_tbl_restore_ckp( rfp, jsx, dcb );
	local_dberr = jsx->jsx_dberr;

	if ( lstatus != E_DB_OK )
	    break;

    } while (FALSE);

    /*
    ** Before we exit check if the database was opened within this
    ** function, if so close db.
    */
    status = rfp_close_database (rfp, dcb, dbdb_flag);

    if (status != E_DB_OK)
    {
	dmfWriteMsg(&jsx->jsx_dberr, 0, 0);
	return (status);
    }
    if (lstatus != E_DB_OK)
    {
	jsx->jsx_dberr = local_dberr;
   	status = lstatus; 
    }

    return( status );
}

/*{
** Name: rfp_tbl_restore_ckp
**
** Description:
**
**
** Inputs:
**	rfp
**	jsx				Pointer to journal support context.
**	dcb				Pointer to DCB for database.
**
** Outputs:
**      err_code                        Reason for error code.
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**
** History:
**	14-sep-1994 (andyw)
**          Partial Backup/Recovery Project:
**              Created.
**	06-jan-1995 (shero03)
**	    Changed dmckp_tab_name to be a pointer to char
**	16-jun-1997 (nanpr01)
**	    Fixed another hole. Donot reference memory after it is deallocated.
*/
static DB_STATUS
rfp_tbl_restore_ckp(
DMF_RFP             *rfp,
DMF_JSX             *jsx,
DMP_DCB             *dcb)
{
    RFP_QUEUE		*totq;
    RFP_TBLCB           *tblcb = 0;
    RFP_TBLHCB          *hcb = rfp->rfp_tblhcb_ptr;
    DMP_EXT		*ext = dcb->dcb_ext;
    RFP_LOC_MASK	*lm  = rfp->rfp_loc_mask_ptr;
    DMP_TCB		*master_tcb;
    DB_TAB_ID		master_id;
    DMP_TCB		*tcb;
    DMP_TABLE_IO        *tbio;
    DB_OWN_NAME		username;
    DB_TAB_TIMESTAMP	stamp;
    i4		i, j ;
    i4		ckp_sequence;
    i4		loc_sequence;
    i4		grant_mode = DM2T_X;
    i4		log_id;
    DB_TRAN_ID		tran_id;
    i4		lock_id;
    DM_FILE             filename;
    DMP_LOCATION	*l;
    CL_ERR_DESC         sys_err;
    DB_STATUS		status;
    i4		files_at_loc_count[ DM_LOC_MAX ];
    DMCKP_CB		*d = &rfp->rfp_dmckp;
    bool                do_relocate;
    DMP_LOC_ENTRY       *newloc;
    DB_LOC_NAME         *new_locname;
    DI_IO               di_del;
    i4			error;
    DB_ERROR		local_dberr;

    CLRDBERR(&jsx->jsx_dberr);

    /*
    ** Initialise the DMCKP_CB structure
    */
    status = dmckp_init( d, DMCKP_RESTORE, &error );
    if ( status != E_DB_OK )
    {
	SETDBERR(&jsx->jsx_dberr, 0, error);
        return( status );
    }

    /*
    ** Initialise count of files at each location
    */
    for ( i = 0; i < ext->ext_count; i++)
	files_at_loc_count[ i ] = 0;

    {
        char        *cp = (char *)&username;

        IDname(&cp);
        MEmove(STlength(cp), cp, ' ', sizeof(username), username.db_own_name);
    }

    /*
    ** This transaction is started to get transaction context required
    ** find out location and filename information for the table.
    ** We are not actually going to do any updates so make this a readonly
    ** transaction. This will get rid of a potential problem in 
    ** rfp_read_journals_tbl where we will see this DM0L_BT record in the
    ** journal, but not the DM0L_ET
    ** If we ever need to perform updates in this transaction, we need to
    ** close and re-open the database to force all log records to the journal
    */
    status = dmf_rfp_begin_transaction(DMXE_READ,
                                       0,
                                       dcb,
                                       dcb->dcb_log_id,
                                       &username,
                                       dmf_svcb->svcb_lock_list,
                                       &log_id,
                                       &tran_id,
                                       &lock_id,
                                       &jsx->jsx_dberr);

    /*
    ** Setup the DMCKP information which will remain constant across the
    ** the restore of each location. Initialize the other fields...
    */
    d->dmckp_op_type       = DMCKP_RESTORE_FILE;
    d->dmckp_ckp_file      = filename.name;
    d->dmckp_ckp_l_file    = sizeof(filename);


    if (jsx->jsx_status & JSX_DEVICE)
    {
        d->dmckp_ckp_path    = jsx->jsx_device_list[0].dev_name;
        d->dmckp_ckp_l_path  = jsx->jsx_device_list[0].l_device;
        d->dmckp_device_type = DMCKP_TAPE_FILE;

    }
    else
    {
	*(dcb->dcb_ckp->physical.name + dcb->dcb_ckp->phys_length) = '\0';
        d->dmckp_ckp_path    = dcb->dcb_ckp->physical.name;
        d->dmckp_ckp_l_path  = dcb->dcb_ckp->phys_length;
        d->dmckp_device_type = DMCKP_DISK_FILE;
    }


    /*
    ** Scan through the list of tables to determine the following information
    ** - What locations are of interest, used for checking snapshots exist
    ** - Total number of files that will be processed
    ** - Total number of files at each location
    **
    */
    for ( totq = hcb->tblhcb_htotq.q_next;
          totq != &hcb->tblhcb_htotq;
          totq = tblcb->tblcb_totq.q_next
          )
    {
	tblcb = (RFP_TBLCB *)((char *)totq - CL_OFFSETOF(RFP_TBLCB, tblcb_totq));

	/*
	** If for some reason recovery is turned off on this table
	** then skip it
	*/
	if ((tblcb->tblcb_table_status & RFP_RECOVER_TABLE) == 0)
	    continue;

	/* For partitioned tables, get file info for each partition */
	if (tblcb->tblcb_table_status & RFP_IS_PARTITIONED)
	    continue;

	master_tcb = NULL;
	master_id.db_tab_base = tblcb->tblcb_table_id.db_tab_base;
	master_id.db_tab_index = 0;
	if (tblcb->tblcb_table_status & RFP_PARTITION)
	{
	    status = dm2t_fix_tcb (dcb->dcb_id, &master_id, &tran_id,
		       dmf_svcb->svcb_lock_list, log_id, 
			   DM2T_RECOVERY, /* this means don't open file */
			   dcb, &master_tcb, &jsx->jsx_dberr);
	}

	status = dm2t_fix_tcb (dcb->dcb_id,
			       &tblcb->tblcb_table_id,
			       &tran_id,
			       lock_id,
			       log_id,
		   	       DM2T_RECOVERY,
			       dcb,
		 	       &tcb,
			       &jsx->jsx_dberr);

	tblcb->tblcb_tcb_ptr = tcb;
	tbio = &tcb->tcb_table_io;


	/*
	** Increase total number of files being processed
	*/
	d->dmckp_num_files = d->dmckp_num_files + tbio->tbio_loc_count;

	/* This information
	** is used when checking if the snapshots are availablef.*
	** Mark the corresponding bits for this table used,
	** all locations are of interest.
	*/
	for ( i = 0; i < tbio->tbio_loc_count; i++)
	{
	    /*
	    ** Indicate that this location is off interest
	    */
	    BTset( tbio->tbio_location_array[i].loc_config_id,
		   (char *)lm->locm_r_allow );

	    /*
	    ** Increase the count of files that will be processed at this
	    ** location
	    */
	    files_at_loc_count[ tbio->tbio_location_array[i].loc_config_id ]++;

	}

	status = dm2t_unfix_tcb( &tcb, (DM2T_PURGE | DM2T_NORELUPDAT),
		           	 dmf_svcb->svcb_lock_list, log_id, &jsx->jsx_dberr );
	if (status != E_DB_OK)
	{
	    dmfWriteMsg(&jsx->jsx_dberr, 0, 0);
	    return (status);
	}

	if (master_tcb)
	{
	    status = dm2t_unfix_tcb( &master_tcb, (DM2T_PURGE | DM2T_NORELUPDAT),
			     dmf_svcb->svcb_lock_list, log_id, &jsx->jsx_dberr );
	    if (status != E_DB_OK)
	    {
		dmfWriteMsg(&jsx->jsx_dberr, 0, 0);
		break;
	    }
	}
    }

    do
    {
	/*
	** Check that all the required snapshots exist
	*/
	status = rfp_check_snapshots_exist( rfp, jsx, dcb, &d->dmckp_num_locs );
	if ( status != E_DB_OK )
	    break;

	/*
	** Call the checkpoint starting routine
	*/
        status = dmckp_begin_restore_tbl( d, dcb, &error );
	if ( status != E_DB_OK )
	{
	    SETDBERR(&jsx->jsx_dberr, 0, error);
	    break;
	}


	/*
	** Construct filename for checkpoint.
	** Notice that if the ckp is run by online mode, we trust dmp sequence.
	** Otherwise, we must use jnl sequence. The sequence number decision is
	** made by prepare() and is passed in to us through the rfp
	*/
	ckp_sequence = rfp->ckp_seq;
	filename.name[0] = 'c';
	filename.name[1] = (ckp_sequence / 1000) + '0';
	filename.name[2] = ((ckp_sequence / 100) % 10) + '0';
	filename.name[3] = ((ckp_sequence / 10) % 10) + '0';
	filename.name[4] = (ckp_sequence % 10) + '0';
	filename.name[8] = '.';
	filename.name[9] = 'c';
	filename.name[10] = 'k';
	filename.name[11] = 'p';

	/*
	** Restore the file from the checkpoint, scan through each location
	** restoring files as needed
	*/
	loc_sequence = 0;
	for (i = 0; i < ext->ext_count; i++)
	{
	    /*
	    ** Skip non data locations, not include alias locations
	    ** as want to bring down the underlying file.
	    */
	    if ( ext->ext_entry[i].flags &
	       (DCB_JOURNAL | DCB_CHECKPOINT | DCB_DUMP | DCB_WORK | DCB_AWORK))
		   continue;

	    loc_sequence++;

	    /*
	    ** Skip locations which are not needed
	    */
	    if ( BTtest( i, (char *)lm->locm_r_allow ) == FALSE )
	        continue;

	    /*
	    ** Setup the constant part of the DMCKP_CB
	    */

	    filename.name[5] = ( loc_sequence / 100) + '0';
            filename.name[6] = ((loc_sequence / 10) % 10) + '0';
            filename.name[7] = ( loc_sequence % 10) + '0';

            /*
	    ** Restore the database location, so setup the DMCKP_CB info which
	    ** are the same for all device types
	    */
	    *(ext->ext_entry[i].physical.name + ext->ext_entry[i].phys_length) = '\0';
	    d->dmckp_di_path   = ext->ext_entry[i].physical.name;
	    d->dmckp_di_l_path = ext->ext_entry[i].phys_length;
	    d->dmckp_num_files_at_loc = files_at_loc_count[ i ];

	    /*
	    ** If relocating, change location names
	    ** FIX ME
	    */
	    if ((jsx->jsx_status1 & JSX_RELOCATE) &&
		(jsx->jsx_status1 & JSX_NLOC_LIST))
	    {
		if (do_relocate = reloc_this_loc(jsx,
				&ext->ext_entry[i].logical, &new_locname))
		{
		    status = find_dcb_ext(jsx, dcb, new_locname,
					DCB_DATA, &newloc);
		    if (status != E_DB_OK)
		    {
			break;
			/* FIX ME */
		    }
		}
	    }
	    else
		do_relocate = FALSE;

	    /*
	    ** Call dmckp routine before restoring files at this location
	    */
	    status = dmckp_begin_restore_tbls_at_loc( d, dcb, &error );
	    if ( status != E_DB_OK )
	    {
		SETDBERR(&jsx->jsx_dberr, 0, error);
                break;
	    }

	    for ( totq = hcb->tblhcb_htotq.q_next;
		  totq != &hcb->tblhcb_htotq;
		  totq = tblcb->tblcb_totq.q_next
		  )
	    {
		tblcb = (RFP_TBLCB *)((char *)totq - 
				CL_OFFSETOF(RFP_TBLCB, tblcb_totq));

		/*
		** If for some reason recovery is turned off on this table
		** then skip it
		*/
		if ((tblcb->tblcb_table_status & RFP_RECOVER_TABLE) == 0)
		    continue;

		/* For partitioned tables, get file info for each partition */
		if (tblcb->tblcb_table_status & RFP_IS_PARTITIONED)
		    continue;

		master_tcb = NULL;
		master_id.db_tab_base = tblcb->tblcb_table_id.db_tab_base;
		master_id.db_tab_index = 0;
		if (tblcb->tblcb_table_status & RFP_PARTITION)
		{
		    status = dm2t_fix_tcb (dcb->dcb_id, &master_id, &tran_id,
			       dmf_svcb->svcb_lock_list, log_id, 
				   DM2T_RECOVERY, /* this means don't open file */
				   dcb, &master_tcb, &jsx->jsx_dberr);
		}

		status = dm2t_lock_table (dcb, &master_id,
					  DM2T_X,
                                          dmf_svcb->svcb_lock_list,
				  	  (i4)0,
				 	  &grant_mode,
					  (LK_LKID *)&lock_id,
					  &jsx->jsx_dberr);
		if (status != E_DB_OK)
		{
		    dmfWriteMsg(&jsx->jsx_dberr, 0, 0);
		    return (status);
		}
		status = dm2t_fix_tcb (dcb->dcb_id,
			       	       &tblcb->tblcb_table_id,
			       	       &tran_id,
			       	       lock_id,
			               (i4)0,
		   	               DM2T_RECOVERY,
			               dcb,
		 	               &tcb,
			               &jsx->jsx_dberr);
		if (status != E_DB_OK)
		{
		    dmfWriteMsg(&jsx->jsx_dberr, 0, 0);
		    return (status);
		}

		tbio = &tcb->tcb_table_io;

		for ( j = 0; j <tbio->tbio_loc_count; j++)
		{
		    /*
		    ** See if the table has a file on this location
		    */
		    if (tbio->tbio_location_array[j].loc_config_id != i)
		        continue;

		    l = &tbio->tbio_location_array[j];

		    /*
		    ** If RELOCATE, we changed this to new location prior
		    ** to restoring the previous table.
		    ** Now we have to change it back to the old location
		    ** prior to deleting the old table
		    */
		    d->dmckp_di_path   = ext->ext_entry[i].physical.name;
		    d->dmckp_di_l_path = ext->ext_entry[i].phys_length;

		    /*
	            ** Setup the filename for the DMCKP_CB block
		    */
		    d->dmckp_di_file   = l->loc_fcb->fcb_filename.name;
		    d->dmckp_di_l_file = l->loc_fcb->fcb_namelength;

		    /*
		    ** Delete the file
		    */
		    d->dmckp_tab_name = tblcb->tblcb_table_name.db_tab_name;
		    d->dmckp_l_tab_name = DB_TAB_MAXNAME;

		    status = dmckp_delete_file( d, &error );
		    if ( status != E_DB_OK )
	            {
		        /*
			** Check if ok to continue
			*/
			SETDBERR(&jsx->jsx_dberr, 0, error);

			if ( rfp_error( rfp, jsx, tblcb, (DMVE_CB *)0,
			    RFP_FAILED_TABLE, status ) != E_DB_OK )
			{
			    /*
			    ** FIXME, maybe need an error code here
			    */
			    status = E_DB_ERROR;
			    break;

	                }
		    }

		    /*
		    ** If RELOCATE, change location names FIX ME
		    */
		    if (do_relocate)
		    {
			/* Make sure the file is deleted from the old loc */
			status = DIdelete(&di_del, d->dmckp_di_path,
					d->dmckp_di_l_path,
					d->dmckp_di_file,
					d->dmckp_di_l_file,
					&sys_err);

			if (status != OK)
			{
			    if (status == DI_FILENOTFOUND)
			    {
				status = E_DB_OK;
			    }
			    else
			    {
				if (rfp_error(rfp,jsx,tblcb, (DMVE_CB *)0,
				    RFP_FAILED_TABLE, status) != E_DB_OK)
				{
				    status = E_DB_ERROR;
				    break;
				}
			    }
			}
			d->dmckp_di_path   = newloc->physical.name;
			d->dmckp_di_l_path = newloc->phys_length;
		    }

		    /*
		    ** Restore the file
		    */
		    d->dmckp_tab_name = tblcb->tblcb_table_name.db_tab_name;
		    d->dmckp_l_tab_name = DB_TAB_MAXNAME;
		    d->dmckp_raw_start = ext->ext_entry[i].raw_start;
		    d->dmckp_raw_blocks = ext->ext_entry[i].raw_blocks;
		    d->dmckp_raw_total_blocks = ext->ext_entry[i].raw_total_blocks;

		    status = dmckp_restore_file( d, dcb, &error );

		    if ( status != E_DB_OK )
	            {
			SETDBERR(&jsx->jsx_dberr, 0, error);
		        /*
			** Check if ok to continue
			*/
			if ( rfp_error( rfp, jsx, tblcb, (DMVE_CB *)0,
				RFP_FAILED_TABLE, status ) != E_DB_OK)
			{
			    /*
			    ** FIXME, maybe need an error code here
			    */
			    status = E_DB_ERROR;
			    break;

	                }
		    }


		}

		status = dm2t_unfix_tcb( &tcb, (DM2T_PURGE | DM2T_NORELUPDAT),
			           	 dmf_svcb->svcb_lock_list, log_id, 
					 &jsx->jsx_dberr );
		/*
		** Error restoring a file
		*/
	        if ( status != E_DB_OK )
		    break;

		if (master_tcb)
		{
		    status = dm2t_unfix_tcb( &master_tcb, (DM2T_PURGE | DM2T_NORELUPDAT),
				     dmf_svcb->svcb_lock_list, log_id, &jsx->jsx_dberr );
		    if (status != E_DB_OK)
		    {
			dmfWriteMsg(&jsx->jsx_dberr, 0, 0);
			break;
		    }
		}
	    }
	    /*
	    ** Error restoring a file
	    */
	    if ( status != E_DB_OK )
		break;

	    /*
	    ** Finished restoring files at this location
	    */
	    status = dmckp_end_restore_tbls_at_loc( d, dcb, &error );
	    if ( status != E_DB_OK )
	    {
		SETDBERR(&jsx->jsx_dberr, 0, error);
                break;
	    }

        }

	/*
	** Error restoring a file
	*/
	if ( status != E_DB_OK )
	    break;


	/*
	** Call the checkpoint end routine
	*/
        status = dmckp_end_restore_tbl( d, dcb, &error );
	if ( status != E_DB_OK )
	{
	    SETDBERR(&jsx->jsx_dberr, 0, error);
	    break;
	}

    } while (FALSE);

    if (master_tcb)
    {
	DB_STATUS	local_status;

	local_status = dm2t_unfix_tcb( &master_tcb, 
		    (DM2T_PURGE | DM2T_NORELUPDAT),
		     dmf_svcb->svcb_lock_list, log_id, &local_dberr );
    }

    status = dmf_rfp_commit_transaction(&tran_id, log_id, lock_id,
                                        DMXE_ROTRAN, &stamp, &jsx->jsx_dberr);
    if ( status != E_DB_OK)
    {
	dmfWriteMsg(&jsx->jsx_dberr, 0, 0);
	return (status);
    }

    return( status );

}

/*{
** Name: rfp_check_snapshots_exist
**
** Description:
**
**
** Inputs:
**	rfp
**	jsx				Pointer to journal support context.
**	dcb				Pointer to DCB for database.
**
** Outputs:
**	number_locs			Number of locations which will
**					be recovered.
**      err_code                        Reason for error code.
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**
** History:
**	14-sep-1994 (andyw)
**          Partial Backup/Recovery Project:
**              Created.
**	16-nov-1994 (andyw)
**	    Altered handling of filename within the dmckp structure
**	    Also passed path name upto length into the dmckp structure
**
**	    Copy path and filename into di variables for use later
**	    with %D type operations within the open checkpoint definition
**	    file cktmpl.def
**      21-july-1995 (thaju02)
**          bug #52556 - rolldb from tape device, returns message
**          '...beginning restore of tape...of 0 locations...'
**          now count number of locations recovering from.
*/
static DB_STATUS
rfp_check_snapshots_exist(
DMF_RFP             *rfp,
DMF_JSX             *jsx,
DMP_DCB             *dcb,
i4		    *number_locs)
{
    CHECK_PARAMS	check_params;
    DMP_EXT             *ext = dcb->dcb_ext;
    i4		loc_sequence = 0;
    i4		ckp_sequence;
    DM_FILE		filename;
    RFP_LOC_MASK	*lm  = rfp->rfp_loc_mask_ptr;
    i4		i;
    STATUS		status;
    CL_ERR_DESC         sys_err;
    DMCKP_CB		dmckp;
    i4			error;

    CLRDBERR(&jsx->jsx_dberr);

    /*
    ** If user specified a device then do not do any special checking.
    ** FIXME: In the future maybe usefull to add a OpenCheckpoint
    ** option to do this bit.
    */
    if ( jsx->jsx_status & JSX_DEVICE )
    {
        /*
        ** bug #52556, 21-july-1995 (thaju02) rolldb from tape,
        ** previously specified 0 locations, now count locations.
        */
        for (i = 0; i < ext->ext_count; i++)
        {
            /*
            ** Skip journal, dump, checkpoint, work, and aliased locations.
            */
            if (ext->ext_entry[i].flags &
                    (DCB_JOURNAL | DCB_CHECKPOINT | DCB_ALIAS |
                               DCB_DUMP | DCB_WORK | DCB_AWORK))
                continue;

            /*
            ** If not database level recovery then check if this location
            ** is of interest
            */
            if ( (jsx->jsx_status1 & JSX1_RECOVER_DB ) == 0 )
            {
                if ( BTtest( i, (char *)lm->locm_r_allow ) == FALSE )
                    continue;
            }

            ++*number_locs;
        }
        return( E_DB_OK );
    }

    /*
    ** Initialise the DMCKP_CB structure
    */
    status = dmckp_init( &dmckp, DMCKP_RESTORE, &error );
    if ( status != E_DB_OK )
    {
	SETDBERR(&jsx->jsx_dberr, 0, error);
        return( status );
    }

    /*
    ** setup the filename
    */
    ckp_sequence = rfp->ckp_seq;
    loc_sequence++;

    filename.name[0] = 'c';
    filename.name[1] = (ckp_sequence / 1000) + '0';
    filename.name[2] = ((ckp_sequence / 100) % 10) + '0';
    filename.name[3] = ((ckp_sequence / 10) % 10) + '0';
    filename.name[4] = (ckp_sequence % 10) + '0';
    filename.name[5] = (loc_sequence / 100) + '0';
    filename.name[6] = ((loc_sequence / 10) % 10) + '0';
    filename.name[7] = (loc_sequence % 10) + '0';
    filename.name[8] = '.';
    filename.name[9] = 'c';
    filename.name[10] = 'k';
    filename.name[11] = 'p';

    /*
    ** Need to cut the physical name to the size of phys_length
    ** due to the rules of ~%t
    */
    *(dcb->dcb_ckp->physical.name + dcb->dcb_ckp->phys_length) = '\0';

    /*
    ** Setup the CK information which will remain constant across the
    ** the restore of each location. Initialize the other fields...
    */
    dmckp.dmckp_ckp_file      = filename.name;
    dmckp.dmckp_ckp_l_file    = sizeof(filename);
    dmckp.dmckp_ckp_path      = dcb->dcb_ckp->physical.name;
    dmckp.dmckp_ckp_l_path    = dcb->dcb_ckp->phys_length;

    /*
    ** Checkpoint from DISK!
    */
    dmckp.dmckp_device_type   = DMCKP_DISK_FILE;

    do
    {
	/*
	** Scan locations skipping journal, dump, checkpoint, work
        ** and aliased locations making sure that the snapshot file
        ** exists
	*/
	loc_sequence = 0;
	for (i = 0; i < ext->ext_count; i++)
	{
	    /*
	    ** Skip journal, dump, checkpoint, work, and aliased locations.
	    */
	    if (ext->ext_entry[i].flags &
		    (DCB_JOURNAL | DCB_CHECKPOINT | DCB_ALIAS |
			       DCB_DUMP | DCB_WORK | DCB_AWORK))
		continue;

	    loc_sequence++;

	    /*
	    ** If not database level recovery then check if this location
	    ** is of interest
	    */
            if ( (jsx->jsx_status1 & JSX1_RECOVER_DB ) == 0 )
	    {
	        if ( BTtest( i, (char *)lm->locm_r_allow ) == FALSE )
	            continue;
            }

	    ++*number_locs;

	    filename.name[5] = (loc_sequence / 100) + '0';
	    filename.name[6] = ((loc_sequence / 10) % 10) + '0';
	    filename.name[7] = (loc_sequence % 10) + '0';

            status = dmckp_check_snapshot( &dmckp, &error );
	    if ( status != E_DB_OK )
	    {
		uleFormat( NULL, E_DM1318_RFP_NO_CKP_FILE, &sys_err,
			    ULE_LOG, NULL, NULL, 0, NULL, &error, 3,
		            sizeof(dcb->dcb_name), dcb->dcb_name.db_db_name,
		            sizeof(check_params.fname), &check_params.fname,
		            dcb->dcb_ckp->phys_length,
	                    &dcb->dcb_ckp->physical);
		/*
		** do not return error here in this release since
		** this location may have been added after the
		** checkpoint was taken. This will be taken care
		** of in the next release with info in the CNF
		** file marking what locations have been check-
		** pointed. (sandyh)
		*/
	    }

	    status = E_DB_OK ;

	}

    } while ( FALSE );

    return( status );
}

/*{
** Name: rfp_tbl_post_ckp_processing - .
**
** Description:
**
** Inputs:
**	rfp				Rollforward context.
**	jsx				Pointer to Journal Support info.
**      dcb				Pointer to DCB for this database.
**
** Outputs:
**      err_code                        Reason for error return status.
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
**	14-sep-1994 (andyw)
**          Partial Backup/Recovery Project:
**		Created
*/
static DB_STATUS
rfp_tbl_post_ckp_processing(
DMF_RFP		    *rfp,
DMF_JSX		    *jsx,
DMP_DCB		    *dcb)
{
    RFP_QUEUE		*totq;
    RFP_TBLCB           *tblcb = 0;
    RFP_TBLHCB          *hcb = rfp->rfp_tblhcb_ptr;
    DB_STATUS		status = E_DB_OK;
    i4			error;

    CLRDBERR(&jsx->jsx_dberr);

    /*
    ** Scan though the list of tables being recovered and close their
    ** TCB's.  Will not be used again.
    */
    for ( totq = hcb->tblhcb_htotq.q_next;
          totq != &hcb->tblhcb_htotq;
          totq = tblcb->tblcb_totq.q_next
          )
    {
	tblcb = (RFP_TBLCB *)((char *)totq - CL_OFFSETOF(RFP_TBLCB, tblcb_totq));

	if ( tblcb->tblcb_tcb_ptr == (DMP_TCB *) 0 )
	    continue;

	status = dm2t_unfix_tcb( &tblcb->tblcb_tcb_ptr,
			         ( DM2T_PURGE | DM2T_NORELUPDAT ),
			           dmf_svcb->svcb_lock_list, (i4)0, &jsx->jsx_dberr );
	if ( status != E_DB_OK )
	{
	    /*
	    ** FIXME need to determine error
	    */
	}


    }

    return( status );
}

/*{
** Name: rfp_tbl_filter_recs - Check if the log record is of interest.
**
** Description:
**
** Inputs:
**	rfp			Rollforward context.
**	filter_cb		Holds all information from log record needed
**				to determine if record is of interest.
**
** Outputs:
**      None.
**
**	Returns:
**	    TRUE	- Record is of interest
**	    FALSE	- Record is very boring.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	14-sep-1994 (andyw)
**          Partial Backup/Recovery Project:
**		Created
**      23-jan-1995 (stial01)
**          BUG 66401: rfp_tbl_filter_recs() not testing status correctly
*/
static bool
rfp_tbl_filter_recs(
DMF_RFP		*rfp,
DMF_JSX		*jsx,
RFP_FILTER_CB	*filter_cb,
RFP_TBLCB       **tblcb )
{
    bool 	apply = FALSE;

    /*
    ** If table level rollforward then check if table is of interest using
    ** the table id passed in.
    */
    if ( jsx->jsx_status & JSX_TBL_LIST )
    {
	apply = rfp_locate_tblcb( rfp, tblcb, filter_cb->tab_id);

	/*
	** If table found make sure it is still valid
	*/
	if ( apply )
	{
	    apply = ( (*tblcb)->tblcb_table_status & RFP_RECOVER_TABLE );
	}
    }

    return( apply );
}

/*{
** Name: rfp_init - Initialise the rfp fields used by all forms of
**		    rollforward.
**
** Description:
**
** Inputs:
**	rfp			Rollforward context.
**
** Outputs:
**      None.
**
**	Returns:
**	    None.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	14-sep-1994 (andyw)
**          Partial Backup/Recovery Project:
**		Created
**	5-march-1997 (angusm)
**	    Initialise extra fields to save existing data locations (49057)
*/
static VOID
rfp_init( 
DMF_RFP		*rfp,
DMF_JSX		*jsx)
{
    DMF_RFP	*r = rfp;

    rfp->rfp_jsx = jsx;
    r->rfp_status = 0;
    r->open_db    = 0;
    r->rfp_locmap = 0;

    r->rfp_stats_blk.rfp_total_dump_recs  = 0;
    r->rfp_stats_blk.rfp_applied_dump_recs= 0;
    r->rfp_stats_blk.rfp_total_recs   	= 0;
    r->rfp_stats_blk.rfp_applied_recs 	= 0;
    r->rfp_stats_blk.rfp_bt 		= 0;
    r->rfp_stats_blk.rfp_et 		= 0;
    r->rfp_stats_blk.rfp_assoc 		= 0;
    r->rfp_stats_blk.rfp_alloc 		= 0;
    r->rfp_stats_blk.rfp_extend 	= 0;
    r->rfp_stats_blk.rfp_ovfl 		= 0;
    r->rfp_stats_blk.rfp_nofull 	= 0;
    r->rfp_stats_blk.rfp_fmap 		= 0;
    r->rfp_stats_blk.rfp_put 		= 0;
    r->rfp_stats_blk.rfp_del 		= 0;
    r->rfp_stats_blk.rfp_btput 		= 0;
    r->rfp_stats_blk.rfp_btdel 		= 0;
    r->rfp_stats_blk.rfp_btsplit 	= 0;
    r->rfp_stats_blk.rfp_btovfl 	= 0;
    r->rfp_stats_blk.rfp_btfree 	= 0;
    r->rfp_stats_blk.rfp_btupdovfl 	= 0;
    r->rfp_stats_blk.rfp_rtput 		= 0;
    r->rfp_stats_blk.rfp_rtdel 		= 0;
    r->rfp_stats_blk.rfp_rtrep	 	= 0;
    r->rfp_stats_blk.rfp_dealloc 	= 0;
    r->rfp_stats_blk.rfp_ai		= 0;
    r->rfp_stats_blk.rfp_bi 		= 0;
    r->rfp_stats_blk.rfp_rep 		= 0;
    r->rfp_stats_blk.rfp_create         = 0;
    r->rfp_stats_blk.rfp_crverify 	= 0;
    r->rfp_stats_blk.rfp_destroy 	= 0;
    r->rfp_stats_blk.rfp_index 		= 0;
    r->rfp_stats_blk.rfp_modify 	= 0;
    r->rfp_stats_blk.rfp_relocate 	= 0;
    r->rfp_stats_blk.rfp_alter 		= 0;
    r->rfp_stats_blk.rfp_fcreate 	= 0;
    r->rfp_stats_blk.rfp_frename 	= 0;
    r->rfp_stats_blk.rfp_dmu 		= 0;
    r->rfp_stats_blk.rfp_location 	= 0;
    r->rfp_stats_blk.rfp_extalter 	= 0;
    r->rfp_stats_blk.rfp_sm0closepurge  = 0;
    r->rfp_stats_blk.rfp_sm1rename	= 0;
    r->rfp_stats_blk.rfp_sm2config 	= 0;
    r->rfp_stats_blk.rfp_other 		= 0;
    r->rfp_db_loc_count		        = 0;
    r->rfp_db_ext		        = 0;
    r->rfp_redo_err			= FALSE;

    /* The SVCB was initialized as a non-server svcb, but this is
    ** rollforward, it generates names sort-of like a server of ID 0.
    */
    dmf_svcb->svcb_tfname_id = 0;

    return;
}

/*{
** Name: rfp_dump_stats -  Dump the stats
**
** Description:
**
** Inputs:
**	rfp			Rollforward context.
**
** Outputs:
**      None.
**
**	Returns:
**	    None.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	14-sep-1994 (andyw)
**          Partial Backup/Recovery Project:
**		Created
*/
static VOID
rfp_dump_stats(
DMF_RFP		    *rfp,
DMF_JSX		    *jsx )
{

    char    		*line_buffer = rfp->rfp_line_buffer;
    RFP_STATS_BLK	*s = &rfp->rfp_stats_blk;

    if (jsx->jsx_status1 & JSX1_STATISTICS)
    {
	TRformat( dmf_put_line, 0, line_buffer, RFP_MAX_LINE_BUFFER,
		  "Dumping statistics.\n" );

	TRformat( dmf_put_line, 0, line_buffer, RFP_MAX_LINE_BUFFER,
		  "Total dump records    : %d", s->rfp_total_dump_recs );
	TRformat( dmf_put_line, 0, line_buffer, RFP_MAX_LINE_BUFFER,
		  "Applied dump records  : %d", s->rfp_applied_dump_recs );
	TRformat( dmf_put_line, 0, line_buffer, RFP_MAX_LINE_BUFFER,
		  "Total journal records : %d", s->rfp_total_recs );
	TRformat( dmf_put_line, 0, line_buffer, RFP_MAX_LINE_BUFFER,
		  "Applied records       : %d", s->rfp_applied_recs );
	TRformat( dmf_put_line, 0, line_buffer, RFP_MAX_LINE_BUFFER,
		  "Begin transaction     : %d", s->rfp_bt );
	TRformat( dmf_put_line, 0, line_buffer, RFP_MAX_LINE_BUFFER,
		  "End   transaction     : %d", s->rfp_et );
	TRformat( dmf_put_line, 0, line_buffer, RFP_MAX_LINE_BUFFER,
		  "assoc                 : %d", s->rfp_assoc );
	TRformat( dmf_put_line, 0, line_buffer, RFP_MAX_LINE_BUFFER,
		  "alloc                 : %d", s->rfp_alloc );
	TRformat( dmf_put_line, 0, line_buffer, RFP_MAX_LINE_BUFFER,
		  "extend                : %d", s->rfp_extend );
	TRformat( dmf_put_line, 0, line_buffer, RFP_MAX_LINE_BUFFER,
		  "ovfl                  : %d", s->rfp_ovfl );
	TRformat( dmf_put_line, 0, line_buffer, RFP_MAX_LINE_BUFFER,
		  "nofull                : %d", s->rfp_nofull );
	TRformat( dmf_put_line, 0, line_buffer, RFP_MAX_LINE_BUFFER,
		  "fmap                  : %d", s->rfp_fmap );
	TRformat( dmf_put_line, 0, line_buffer, RFP_MAX_LINE_BUFFER,
		  "put                   : %d", s->rfp_put );
	TRformat( dmf_put_line, 0, line_buffer, RFP_MAX_LINE_BUFFER,
		  "del                   : %d", s->rfp_del );
    }

    if (jsx->jsx_status & JSX_TRACE)
    {
	TRdisplay("Dumping statistics...\n");

	TRdisplay("Total dump records    : %d\n", s->rfp_total_dump_recs );
	TRdisplay("Applied dump records  : %d\n", s->rfp_applied_dump_recs );
	TRdisplay("Total journal records : %d\n", s->rfp_total_recs );
	TRdisplay("Total applied records : %d\n", s->rfp_applied_recs );
	TRdisplay("Begin transaction     : %d\n", s->rfp_bt );
	TRdisplay("End   transaction     : %d\n", s->rfp_et );
	TRdisplay("alloc                 : %d\n", s->rfp_alloc );
	TRdisplay("assoc                 : %d\n", s->rfp_assoc );
	TRdisplay("extend                : %d\n", s->rfp_extend );
	TRdisplay("nofull                : %d\n", s->rfp_nofull );
	TRdisplay("fmap                  : %d\n", s->rfp_fmap );
	TRdisplay("put                   : %d\n", s->rfp_put );
	TRdisplay("del                   : %d\n", s->rfp_del );
    }
}

/*{
** Name: rfp_filter_log_record - Filter a log record
**
** Description:
**	This routine filter the log record accordingly.
**
** Inputs:
**	rfp				Pointer to rollforward context.
**	jsx
**      dmve				Recovery control block.
**      record                          Pointer to log record.
**
** Outputs:
**      err_code                        Reason for error return status.
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
**	14-sep-1994 (andyw)
**          Partial Backup/Recovery Project:
**		Created.
**      23-jan-1995 (stial01)
**          BUG 66401: don't ignore begin/end trans records
**              Test for tbl_id for other log recs that must be processed.
**	1-Apr-2004 (schka24)
**	    Invent raw data record.
**      23-Feb-2009 (hanal04) Bug 121652
**          Added DM0LUFMAP case to deal with FMAP updates needed for an
**          extend operation where a new FMAP will not reside in the last
**          FMAP or new FMAP itself.
**	11-May-2009 (kschendel) b122041
**	    Fix improper bool computed with bare &, rather than with != 0.
*/
static bool
rfp_filter_log_record(
DMF_RFP             *rfp,
DMF_JSX		    *jsx,
DM0L_HEADER	    *record)
{
    DMF_RFP	    *r = rfp;
    RFP_FILTER_CB   filter_cb;
    RFP_TBLCB       **tblcb = &r->rfp_cur_tblcb;
    bool	    apply = FALSE;
    char	    *line_buffer = r->rfp_line_buffer;
    DB_TAB_ID	    *dmu_tab = NULL;
    DB_TAB_ID	    *ddl_tab = NULL;
    i4		    *newloc = NULL;
    i4		    error;

    CLRDBERR(&jsx->jsx_dberr);

    /*
    ** BUG 67411: If relocating a table, we need to adjust the location
    **            information in the jnl/dump records
    */
    if (( jsx->jsx_status1 & JSX1_RECOVER_DB ) == 0
	&& (jsx->jsx_status1 & JSX_RELOCATE)
	&& (jsx->jsx_status1 & JSX_NLOC_LIST))
    {
	newloc = (i4 *)rfp->rfp_locmap->misc_data;
    }

    switch (record->type)
    {
    case DM0LBT:

	apply = TRUE;
	break;

    case DM0LET:

	apply = TRUE;
	break;

    case DM0LRAWDATA:
	/* Always "apply" raw data since we don't know how it will be
	** used.  It's only memory...
	*/
	apply = TRUE;
	break;

    case DM0LASSOC:
	{
	    DM0L_ASSOC   *rec = (DM0L_ASSOC *)record;

	    dmu_tab = &rec->ass_tbl_id;
	    if (newloc)
	    {
		rec->ass_lcnf_loc_id = newloc[rec->ass_lcnf_loc_id];
		rec->ass_ocnf_loc_id = newloc[rec->ass_ocnf_loc_id];
		rec->ass_ncnf_loc_id = newloc[rec->ass_ncnf_loc_id];
	    }
	    break;
	}

    case DM0LDISASSOC:
	{
	    DM0L_DISASSOC  *rec = (DM0L_DISASSOC *)record;

	    dmu_tab = &rec->dis_tbl_id;
	    if (newloc)
	    {
		rec->dis_cnf_loc_id = newloc[rec->dis_cnf_loc_id];
	    }
	    break;
	}

    case DM0LALLOC:
	{
	    DM0L_ALLOC    *rec = (DM0L_ALLOC *)record;

	    dmu_tab = &rec->all_tblid;
	    if (newloc)
	    {
		rec->all_fhdr_cnf_loc_id = newloc[rec->all_fhdr_cnf_loc_id];
		rec->all_fmap_cnf_loc_id = newloc[rec->all_fmap_cnf_loc_id];
		rec->all_free_cnf_loc_id = newloc[rec->all_free_cnf_loc_id];
	    }
	    break;
	}

    case DM0LEXTEND:
	{
	    DM0L_EXTEND   *rec = (DM0L_EXTEND *)record;

	    dmu_tab = &rec->ext_tblid;
	    if (newloc)
	    {
		rec->ext_fhdr_cnf_loc_id = newloc[rec->ext_fhdr_cnf_loc_id];
		rec->ext_fmap_cnf_loc_id = newloc[rec->ext_fmap_cnf_loc_id];
	    }
	    break;
	}

    case DM0LOVFL:
	{
	    DM0L_OVFL    *rec = (DM0L_OVFL *)record;

	    dmu_tab = &rec->ovf_tbl_id;
	    if (newloc)
	    {
		rec->ovf_cnf_loc_id = newloc[rec->ovf_cnf_loc_id];
		rec->ovf_ovfl_cnf_loc_id = newloc[rec->ovf_ovfl_cnf_loc_id];
	    }
	    break;
	}

    case DM0LNOFULL:
	{
	    DM0L_NOFULL   *rec = (DM0L_NOFULL *)record;

	    dmu_tab = &rec->nofull_tbl_id;
	    if (newloc)
	    {
		rec->nofull_cnf_loc_id = newloc[rec->nofull_cnf_loc_id];
	    }
	    break;
	}

    case DM0LFMAP:
	{
	    DM0L_FMAP   *rec = (DM0L_FMAP *)record;

	    dmu_tab = &rec->fmap_tblid;
	    if (newloc)
	    {
		rec->fmap_fhdr_cnf_loc_id = newloc[rec->fmap_fhdr_cnf_loc_id];
		rec->fmap_fmap_cnf_loc_id = newloc[rec->fmap_fmap_cnf_loc_id];
	    }
	    break;
	}

    case DM0LUFMAP:
        {
            DM0L_UFMAP   *rec = (DM0L_UFMAP *)record;

            dmu_tab = &rec->fmap_tblid;
            if (newloc)
            {
                rec->fmap_fhdr_cnf_loc_id = newloc[rec->fmap_fhdr_cnf_loc_id];
                rec->fmap_fmap_cnf_loc_id = newloc[rec->fmap_fmap_cnf_loc_id];
            }
            break;
        }

    case DM0LPUT:
	{
	    DM0L_PUT    *rec = (DM0L_PUT *)record;

	    dmu_tab = &rec->put_tbl_id;
	    if (newloc)
	    {
		rec->put_cnf_loc_id = newloc[rec->put_cnf_loc_id];
	    }
	    break;
	}

    case DM0LDEL:
	{
	    DM0L_DEL    *rec = (DM0L_DEL *)record;

	    dmu_tab = &rec->del_tbl_id;
	    if (newloc)
	    {
		rec->del_cnf_loc_id = newloc[rec->del_cnf_loc_id];
	    }
	    break;
	}

    case DM0LBTPUT:
	{
	    DM0L_BTPUT  *rec = (DM0L_BTPUT *)record;

	    dmu_tab = &rec->btp_tbl_id;
	    if (newloc)
	    {
		rec->btp_cnf_loc_id = newloc[rec->btp_cnf_loc_id];
	    }
	    break;
	}

    case DM0LBTDEL:
	{
	    DM0L_BTDEL  *rec = (DM0L_BTDEL *)record;

	    dmu_tab = &rec->btd_tbl_id;
	    if (newloc)
	    {
		rec->btd_cnf_loc_id = newloc[rec->btd_cnf_loc_id];
	    }
	    break;
	}

    case DM0LBTSPLIT:
	{
	    DM0L_BTSPLIT *rec = (DM0L_BTSPLIT *)record;

	    dmu_tab = &rec->spl_tbl_id;
	    if (newloc)
	    {
		rec->spl_cur_loc_id = newloc[rec->spl_cur_loc_id];
		rec->spl_sib_loc_id = newloc[rec->spl_sib_loc_id];
		rec->spl_dat_loc_id = newloc[rec->spl_dat_loc_id];
	    }
            break;
	}

    case DM0LBTOVFL:
	{
	    DM0L_BTOVFL *rec = (DM0L_BTOVFL *)record;

	    dmu_tab = &rec->bto_tbl_id;
	    if (newloc)
	    {
		rec->bto_leaf_loc_id = newloc[rec->bto_leaf_loc_id];
		rec->bto_ovfl_loc_id = newloc[rec->bto_ovfl_loc_id];
            }
            break;
	}

    case DM0LBTFREE:
	{
	    DM0L_BTFREE *rec = (DM0L_BTFREE *)record;

	    dmu_tab = &rec->btf_tbl_id;
	    if (newloc)
	    {
		rec->btf_ovfl_loc_id = newloc[rec->btf_ovfl_loc_id];
		rec->btf_prev_loc_id = newloc[rec->btf_prev_loc_id];
	    }
            break;
	}

    case DM0LBTUPDOVFL:
	{
	    DM0L_BTUPDOVFL *rec = (DM0L_BTUPDOVFL *)record;

	    dmu_tab = &rec->btu_tbl_id;
	    if (newloc)
	    {
		rec->btu_cnf_loc_id = newloc[rec->btu_cnf_loc_id];
	    }
            break;
	}

    case DM0LRTDEL:
	{
	    DM0L_RTDEL  *rec = (DM0L_RTDEL *)record;

	    dmu_tab = &rec->rtd_tbl_id;
	    if (newloc)
	    {
		rec->rtd_cnf_loc_id = newloc[rec->rtd_cnf_loc_id];
	    }
	    break;
	}

    case DM0LRTPUT:
	{
	    DM0L_RTPUT  *rec = (DM0L_RTPUT *)record;

	    dmu_tab = &rec->rtp_tbl_id;
	    if (newloc)
	    {
		rec->rtp_cnf_loc_id = newloc[rec->rtp_cnf_loc_id];
	    }
	    break;
	}

    case DM0LRTREP:
	{
	    DM0L_RTREP  *rec = (DM0L_RTREP *)record;

	    dmu_tab = &rec->rtr_tbl_id;
	    if (newloc)
	    {
		rec->rtr_cnf_loc_id = newloc[rec->rtr_cnf_loc_id];
	    }
	    break;
	}


    case DM0LDEALLOC:
	{
	    DM0L_DEALLOC    *rec = (DM0L_DEALLOC *)record;

	    dmu_tab = &rec->dall_tblid;
	    if (newloc)
	    {
		rec->dall_fhdr_cnf_loc_id = newloc[rec->dall_fhdr_cnf_loc_id];
		rec->dall_fmap_cnf_loc_id = newloc[rec->dall_fmap_cnf_loc_id];
		rec->dall_free_cnf_loc_id = newloc[rec->dall_free_cnf_loc_id];
	    }

            break;
	}

    case DM0LAI:
	{
	    DM0L_AI         *rec = (DM0L_AI *)record;

	    dmu_tab = &rec->ai_tbl_id;
	    if (newloc)
	    {
		rec->ai_loc_id = newloc[rec->ai_loc_id];
	    }
            break;
	}

    case DM0LBI: /* Only dump processing gets here */
	{
	    DM0L_BI         *rec = (DM0L_BI *)record;

	    dmu_tab = &rec->bi_tbl_id;
	    if (newloc)
	    {
		rec->bi_loc_id = newloc[rec->bi_loc_id];
	    }
	    break;
	}

    case DM0LREP:
	{
	    DM0L_REP       *rec = (DM0L_REP *)record;

	    dmu_tab = &rec->rep_tbl_id;
	    if (newloc)
	    {
		rec->rep_ocnf_loc_id = newloc[rec->rep_ocnf_loc_id];
		rec->rep_ncnf_loc_id = newloc[rec->rep_ncnf_loc_id];
	    }
	    break;
	}

    case DM0LCREATE:

	ddl_tab = &(((DM0L_CREATE *)record)->duc_tbl_id);
	break;

    case DM0LCRVERIFY:

	ddl_tab = &(((DM0L_CRVERIFY *)record)->ducv_tbl_id);
	break;

    case DM0LDESTROY:

	ddl_tab = &(((DM0L_DESTROY *)record)->dud_tbl_id);
	break;

    case DM0LINDEX:

	ddl_tab = &(((DM0L_INDEX *)record)->dui_tbl_id);
	break;

    case DM0LMODIFY:

	ddl_tab = &(((DM0L_MODIFY *)record)->dum_tbl_id);
	break;

    case DM0LRELOCATE:

	ddl_tab = &(((DM0L_RELOCATE *)record)->dur_tbl_id);
	break;

    case DM0LALTER:

	ddl_tab = &(((DM0L_ALTER *)record)->dua_tbl_id);
	break;

    case DM0LFCREATE:

	ddl_tab = &(((DM0L_FCREATE *)record)->fc_tbl_id);
	break;

    case DM0LFRENAME:

	ddl_tab = &(((DM0L_FRENAME *)record)->fr_tbl_id);
	break;

    case DM0LDMU:

	ddl_tab = &(((DM0L_DMU *)record)->dmu_tabid);
	break;

    case DM0LLOCATION:
    case DM0LDELLOCATION:
    case DM0LEXTALTER:
    case DM0LSM0CLOSEPURGE:
    case DM0LSM1RENAME:
    case DM0LSM2CONFIG:

	/* These log records are always applied for database recovery
	** and never applied if table recovery
	*/
	apply = ( jsx->jsx_status1 & JSX1_RECOVER_DB ) != 0;
	break;

    case DM0LP1:
    case DM0LBMINI:
    case DM0LEMINI:
    case DM0LSAVEPOINT:
    case DM0LABORTSAVE:
    case DM0LBTINFO:
    case DM0LJNLSWITCH:

	/*
	** Log records which require no rollforward processing
	*/
        break;

    default:
	/*
	** Unexpected log record.  Log an error, but continue processing.
	** If the log record was some new record type unneeded for rollforward
	** (but journaled anyway) and was not added to this list then no
	** harm done.  If the record actually needs rollforward work, but
	** whoever added the new log record went braindead and forgot to add
	** rollforward support for it, then this error is bad and will likely
	** lead to an inconsistent database.
	*/
	TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
	    "apply_j_rec: Unknown log record type: %d\n", record->type);
	uleFormat(NULL, E_DM1308_RFP_BAD_RECORD, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
	    (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
    }

    if (dmu_tab == NULL && ddl_tab == NULL)
	return (apply);

    if (( jsx->jsx_status1 & JSX1_RECOVER_DB ) == 0)
    {
	/* TABLE rollforward: Skip all DDL type log records */
	if (ddl_tab)
	    return (FALSE);

	if ( jsx->jsx_status & JSX_TBL_LIST )
	{
	    filter_cb.tab_id = dmu_tab;
	    apply = rfp_tbl_filter_recs( rfp, jsx, &filter_cb, tblcb );
	}
    }
    else
    {
	apply = TRUE;
    }

    if (apply && (jsx->jsx_status2 & JSX2_INVALID_TBL_LIST))
    {
	RFP_QUEUE	*invq;
	RFP_TBLHCB      *hcb = rfp->rfp_tblhcb_ptr;
	RFP_TBLCB	*itblcb;
	DB_TAB_ID	*tab;

	tab = dmu_tab ? dmu_tab : ddl_tab;

	for ( invq = hcb->tblhcb_hinvq.q_next;
	      invq != &hcb->tblhcb_hinvq;
	      invq = itblcb->tblcb_invq.q_next)
	{

	    itblcb = (RFP_TBLCB *)((char *)invq - 
				CL_OFFSETOF(RFP_TBLCB, tblcb_invq));

	    if (itblcb->tblcb_table_id.db_tab_base != tab->db_tab_base
		|| itblcb->tblcb_table_id.db_tab_index != tab->db_tab_index)
	    {
		continue;
	    }
		
	    if (itblcb->tblcb_table_status & RFP_IGNORED_DMP_JNL)
	    {
		apply = FALSE;
	    }
	
	    break; /* Always */
	}
    }

    return (apply);
}

/*{
** Name: rfp_read_tables - Get information on each table with a database
**
** Description:
**	This function will read all tables within the database and
**	allocate the tcb relating to each table. All tables
**	will be marked as RFP_TABLE_OK except where the table
**	is one of the following (system catalog, gateway, view)
**	or if the table is a secondary_index and the INDEX bit is set
**	or if the table is a blob and the BLOB bit is set
**
**
** Inputs:
**	rfp
**	jsx				Pointer to journal support context.
**	dcb				Pointer to DCB for database.
**
** Outputs:
**      err_code                        Reason for error code.
**
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	    E_DB_INFO			If failed to find information on
**					a table.
**	Exceptions:
**	    none
**
** Side Effects:
**
** History:
**	17-nov-1994 (andyw)
**	    Partial Backup/Recovery Project:
**		Created.
**      16-jan-1995 (stial01)
**          BUG 66581: new status bits to indicate view,gateway tables
**	 4-jul-96 (nick)
**	    Initialise rcb_k_mode for the iirelation RCB.
**	16-jan-1997 (nanpr01)
**	    Duplicate messages are being displayed for errors . #80068.
**	30-may-1997 (nanpr01)
**	    There is no need to logically open the core catalog.
**      28-feb-2000 (gupsh01)
**          Enabled check for owner along with the table name
**          so that the table names in -table option can also
**          be specified as owner.tablenames .
*/
static DB_STATUS
rfp_read_tables(
DMF_RFP             *rfp,
DMF_JSX             *jsx,
DMP_DCB             *dcb)
{
    DB_STATUS		status;
    DB_TRAN_ID		tran_id;
    DB_OWN_NAME		username;
    DM_TID          	tid;
    DMP_RCB		*iirel_rcb = 0;
    DM2R_KEY_DESC	qual_list[1];
    DMP_RELATION	reltup;
    RFP_TBLCB		*tblcb;
    i4		i;
    i4		dbdb_flag = 0;
    i4			error;

    CLRDBERR(&jsx->jsx_dberr);

    /* Begin a transaction */
    dmf_rfp_dcb = dcb;

    {
        char        *cp = (char *)&username;

        IDname(&cp);
        MEmove(STlength(cp), cp, ' ', sizeof(username), username.db_own_name);
    }

    dbdb_flag = DM2D_NOLOGGING;
    if ((jsx->jsx_status & JSX_WAITDB) == 0)    /*  b66533  */
	dbdb_flag |= DM2D_NOWAIT;

    status = rfp_open_database (rfp, jsx, dcb, dbdb_flag);

    if (status != E_DB_OK)
    {
        return (status);
    }

    /*
    ** Allocate the RCB for iirelation
    */
    status = dm2r_rcb_allocate(
                     dcb->dcb_rel_tcb_ptr, (DMP_RCB *)0,
                     &tran_id, dmf_svcb->svcb_lock_list,
                     (i4)0, &iirel_rcb, &jsx->jsx_dberr );
    if ( status != E_DB_OK )
    {
	dmfWriteMsg(&jsx->jsx_dberr, 0, 0);
 	return (status);
    }

    iirel_rcb->rcb_lk_mode = RCB_K_IS;
    iirel_rcb->rcb_k_mode = RCB_K_IS;
    iirel_rcb->rcb_access_mode = RCB_A_READ;

    status = dm2r_position (iirel_rcb, DM2R_ALL, qual_list,
                            (i4)1,
                            &tid, &jsx->jsx_dberr);
    if (status != E_DB_OK)
    {
        /*
        ** Error whilst positioning a record
        */
	dmfWriteMsg(&jsx->jsx_dberr, 0, 0);
        return (status);
    }

    /*
    ** Now loop through the table
    */
    for (;;)
    {
	status = dm2r_get (iirel_rcb, &tid, DM2R_GETNEXT,
                           (char *)&reltup, &jsx->jsx_dberr);

	if (status != E_DB_OK)
	{
	    if (jsx->jsx_dberr.err_code == E_DM0055_NONEXT)
	    {
		status = E_DB_OK;
		CLRDBERR(&jsx->jsx_dberr);
		break;
	    }

	    dmfWriteMsg(&jsx->jsx_dberr, 0, 0);
	    break;
	}

	/*
	** Now allocate a table control block
	*/
	status = rfp_allocate_tblcb(rfp, &tblcb);
        if ( status != E_DB_OK )
        {
	    dmfWriteMsg(&jsx->jsx_dberr, 0, 0);
	    break;
        }

	tblcb->tblcb_table_name = reltup.relid;
	tblcb->tblcb_table_owner = reltup.relowner;
	tblcb->tblcb_table_id = reltup.reltid;
	STRUCT_ASSIGN_MACRO(reltup.relstamp12, tblcb->tblcb_stamp12);
	tblcb->tblcb_moddate = reltup.relmoddate;

	if (reltup.relstat & TCB_CATALOG)
	    tblcb->tblcb_table_status |= RFP_CATALOG;
	if (reltup.relstat & TCB_VIEW)
	    tblcb->tblcb_table_status |= RFP_VIEW;
	if (reltup.relstat & TCB_GATEWAY)
	    tblcb->tblcb_table_status |= RFP_GATEWAY;
	if (reltup.relstat2 & TCB2_PARTITION)
	    tblcb->tblcb_table_status |= RFP_PARTITION;
	if (reltup.relstat & TCB_IS_PARTITIONED)
	    tblcb->tblcb_table_status |= RFP_IS_PARTITIONED;
	if (reltup.relstat2 & TCB2_TBL_RECOVERY_DISALLOWED)
	    tblcb->tblcb_table_status |= RFP_RECOV_DISALLOWED;

	/** FIXME check privileges ?, table is journalled etc... */

        /* Check for user specified table in list */
        for ( i = (jsx->jsx_tbl_cnt-1); i >= 0; i--)
        {
            if ((MEcmp (tblcb->tblcb_table_name.db_tab_name,
                        &jsx->jsx_tbl_list[i].tbl_name.db_tab_name,
			DB_TAB_MAXNAME)) == 0)
            {
                /*
                ** Table found, check if this is a system catalog
                */
                if (STbcompare((char *)&jsx->jsx_tbl_list[i].tbl_owner.db_own_name,
                                0,"", 0, 0) != 0)
                {
                        if ((MEcmp (tblcb->tblcb_table_owner.db_own_name,
                                &jsx->jsx_tbl_list[i].tbl_owner.db_own_name
                                ,DB_OWN_MAXNAME)) == 0)
                        {
                                tblcb->tblcb_table_status |= RFP_USER_SPECIFIED;
                                tblcb->tblcb_table_status |= RFP_RECOVER_TABLE;
                        }
                }
                else
                {
                        tblcb->tblcb_table_status |= RFP_USER_SPECIFIED;
                        tblcb->tblcb_table_status |= RFP_RECOVER_TABLE;
                }
            }
        }

	/*
	** If everything is OK, add to list
	*/
	rfp_add_tblcb (rfp, tblcb);

    }

    /*
    ** Deallocate the RCB structure
    */
    if ( iirel_rcb )
    {
        status = dm2r_release_rcb( &iirel_rcb, &jsx->jsx_dberr );
        if ( status != E_DB_OK )
        {
	    dmfWriteMsg(&jsx->jsx_dberr, 0, 0);
	    return (status);
        }
    }

    /*
    ** Before we exit check if the database was opened within this
    ** function, if so close db.
    */
    status = rfp_close_database (rfp, dcb, dbdb_flag);

    if (status != E_DB_OK)
    {
	dmfWriteMsg(&jsx->jsx_dberr, 0, 0);
	return (status);
    }

    return( status );
}

/*{
** Name:
**
** Description: rfp_open_database.
**
**
** Inputs:
**      rfp
**      jsx                             Pointer to journal support context.
**      dcb                             Pointer to DCB for database.
**	flags				flags to open db with
**
** Outputs:
**      rfp                             Rollforward context.
**      err_code                        Reason for error code.
**      Returns:
**          E_DB_OK
**          E_DB_ERROR
**      Exceptions:
**          none
**
** Side Effects:
**
** History:
**	23-nov-1994 (andyw)
**	    Created.
**	 3-jul-96 (nick)
**	    Error opening database wasn't returned as an error.  Note
**	    that the FIXMEs in this routine don't need attention yet
**	    as you can't actually get to the code ( online rollforward isn't
**	    yet supported ).
*/
DB_STATUS
rfp_open_database (
DMF_RFP		*rfp,
DMF_JSX		*jsx,
DMP_DCB		*dcb,
i4		flags)
{
    DB_STATUS		status;
    i4             db_lockmode;
    i4             db_access_mode;
    i4			error;

    CLRDBERR(&jsx->jsx_dberr);


    /*
    ** Check if database is opened before calling this function
    */
    if (rfp->open_db & DB_OPENED)
    {
        uleFormat(&jsx->jsx_dberr, E_DM1300_RFP_OPEN_DB, (CL_ERR_DESC *)NULL, ULE_LOG, 
		    NULL, (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	return (E_DB_ERROR);
    }


    /*
    ** Determine what mode to lock and open the database
    */
    if ( jsx->jsx_status1 & JSX1_ONLINE )
    {
        /*
        ** FIXME, things i know need to be done
        ** a) Aquire the dm2u_ckp_lock to prevent an online checkpoint
        ** MAYBE I CAN JUST ACT LIKE ANOTHER SERVER AND CONNECT
        ** TO THE BUFFER MANAGER
        */


        /*
        ** Online recovery opens the database in the following
        ** mode
        ** a) IS mode.
        ** b) Ignore cache protocol, otherwise cannot open the db
        **    as some other process has it open and not sharing the
        **    cache.
        **    Note: We are relying on the upper layers issuing SQL
        **          to flush out old pages and updating the system
        **          catalogs.
        */
        db_lockmode  = DM2D_IS;
        db_access_mode = DM2D_A_READ;

        /*
        ** FIXME need to determine what to do here
        */
        return( E_DB_ERROR );
    }
    else
    {
        /*
        ** Offline recovery opens the database in the following
        ** mode
        ** a) X mode
        ** b) Write mode
        ** c) Pass ordinary open flags.
        */
        db_lockmode = DM2D_X;
        db_access_mode = DM2D_A_WRITE;
    }

    /*
    ** If the user specified do not wait then pass on flag
    */
    if ((jsx->jsx_status & JSX_WAITDB) == 0)
        flags |= DM2D_NOWAIT ;

    /*
    ** Open the database
    */
    status = dm2d_open_db( dcb, db_access_mode, db_lockmode,
                           dmf_svcb->svcb_lock_list,
                           flags, &jsx->jsx_dberr);
    if (status != E_DB_OK)
    {
        if (jsx->jsx_dberr.err_code == E_DM004C_LOCK_RESOURCE_BUSY)
        {
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM1315_RFP_DB_BUSY);
            /*
            ** FIXME what does it mean to return this
            */
            return (E_DB_INFO);
        }

        if (jsx->jsx_dberr.err_code == E_DM0100_DB_INCONSISTENT)
        {
            /*
            ** Report error message describing that table
            ** level recovery is not allowed on an inconsistent
            ** database
            */
            dmfWriteMsg(NULL, E_DM1357_RFP_INCONSIST_DB_NO_TBLRECV, 0);
            return (E_DB_INFO);
        }

        uleFormat(&jsx->jsx_dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL, &error, 0);
	SETDBERR(&jsx->jsx_dberr, 0, E_DM1300_RFP_OPEN_DB);
	return (status);
    }

    rfp->open_db |= DB_OPENED;
    return (E_DB_OK);
}

/*{
** Name:
**
** Description: rfp_close_database.
**
**
** Inputs:
**      rfp
**      jsx                             Pointer to journal support context.
**      dcb                             Pointer to DCB for database.
**	flags				as per previous open_database
**
** Outputs:
**      rfp                             Rollforward context.
**      err_code                        Reason for error code.
**      Returns:
**          E_DB_OK
**          E_DB_ERROR
**      Exceptions:
**          none
**
** Side Effects:
**
** History:
**      23-nov-1994 (andyw)
**          Created.
*/
DB_STATUS
rfp_close_database (
DMF_RFP         *rfp,
DMP_DCB         *dcb,
i4		flags)
{

    DMF_JSX	*jsx = rfp->rfp_jsx;
    DB_STATUS	status;
    i4		error;

    CLRDBERR(&jsx->jsx_dberr);

    if (rfp->open_db & DB_OPENED)
    {
        status = dm2d_close_db (dcb, dmf_svcb->svcb_lock_list, flags, &jsx->jsx_dberr);
 
        if (status != E_DB_OK)
        {
	    dmfWriteMsg(&jsx->jsx_dberr, 0, 0);
            return (status);
        }
        rfp->open_db &= ~(DB_OPENED | DB_LOCKED);
    }
    return (E_DB_OK);
}


/*{
** Name: rfp_get_table_info - Determine partitions and indexes for tables 
**                              being recovered
**
** Description:
**      For all user specified tables, this function will determine if 
**      there are any associated secondary indexes that need to be recovered.
**      Secondary indexes will be marked RFP_INDEX_REQUIRED.       
**      They will also be marked RFP_RECOVER_TABLE unless -nosecondary_index 
**      is specified.
**
**
** Inputs:
**	rfp
**	jsx				Pointer to journal support context.
**	dcb				Pointer to DCB for database.
**
** Outputs:
**      err_code                        Reason for error code.
**
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	    E_DB_INFO			If failed to find information on
**					a table.
**	Exceptions:
**	    none
**
** Side Effects:
**
** History:
**      23-jan-1995 (stial01)
**          BUG 66577: 
**		Created.
**	14-Jan-2004 (jenjo02)
**	    Check db_tab_index > 0 for index, not simply != 0.
*/
static DB_STATUS
rfp_get_table_info(
DMF_RFP             *rfp,
DMF_JSX             *jsx,
DMP_DCB             *dcb)
{
    RFP_QUEUE		*totq;
    RFP_QUEUE		*totq2;
    RFP_TBLCB           *tblcb, *tblcb2;
    RFP_TBLHCB          *hcb = rfp->rfp_tblhcb_ptr;

    CLRDBERR(&jsx->jsx_dberr);

    /*
    ** Loop through all the user supplied tables gathering more info
    */
    for ( totq = hcb->tblhcb_htotq.q_next;
          totq != &hcb->tblhcb_htotq;
          totq = tblcb->tblcb_totq.q_next
          )
    {
	tblcb = (RFP_TBLCB *)((char *)totq - CL_OFFSETOF(RFP_TBLCB, tblcb_totq));

	/* Skip table if not user specified */
        if ( (tblcb->tblcb_table_status & RFP_USER_SPECIFIED) == 0)
	    continue;

	/* Skip table if it is an index */
	if (tblcb->tblcb_table_id.db_tab_index > 0)
	    continue;

	/* Skip table if it is a partition */
	if (tblcb->tblcb_table_id.db_tab_index < 0)
	    continue;

	/* 
	** Loop through all tables looking for indexes on current  
	** user specified table.
	*/
	for ( totq2 = hcb->tblhcb_htotq.q_next;
	      totq2 != &hcb->tblhcb_htotq;
	      totq2 = tblcb2->tblcb_totq.q_next
	      )
	{
	    tblcb2 = (RFP_TBLCB *)((char *)totq2 - 
				CL_OFFSETOF(RFP_TBLCB, tblcb_totq));

	    if (tblcb2->tblcb_table_id.db_tab_base !=
		   tblcb->tblcb_table_id.db_tab_base
		 || tblcb2 == tblcb)
		continue;

	    if (tblcb2->tblcb_table_id.db_tab_index > 0)
	    {
		tblcb2->tblcb_table_status |= RFP_INDEX_REQUIRED;

		/* Unless -noseconary_index specified, recover the index */
		if (( jsx->jsx_status1 & JSX1_NOSEC_IND ) == 0)	
		    tblcb2->tblcb_table_status |= RFP_RECOVER_TABLE;
	    }
	    else if (tblcb2->tblcb_table_id.db_tab_index < 0)
	    {
		tblcb2->tblcb_table_status |= RFP_PART_REQUIRED;
		tblcb2->tblcb_table_status |= RFP_RECOVER_TABLE;
	    }
	}
    }

    return (E_DB_OK);

}


/*{
** Name: rfp_get_blob_info - Determine blob tables for tables being recovered 
**
** Description:
**      For all user specified tables, this function will determine if 
**      there are any associated blob tables that need to be recovered.
**      Blob tables will be marked RFP_BLOB_REQUIRED.       
**      They will also be marked RFP_RECOVER_TABLE unless -noblobs specified.
**
**
** Inputs:
**	rfp
**	jsx				Pointer to journal support context.
**	dcb				Pointer to DCB for database.
**
** Outputs:
**      err_code                        Reason for error code.
**
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	    E_DB_INFO			If failed to find information on
**					a table.
**	Exceptions:
**	    none
**
** Side Effects:
**
** History:
**      23-jan-1995 (stial01)
**          BUG 66577: 
**		Created.
**	22-feb-1995 (carly01)
**	    66868 - table-level rollforwardb generates the incorrect
**	    error message: E_DM0054_NONEXISTENT_TABLE when the table 
**	    specification is correct and the rollforwarddb executes correctly.
**	    The access of iiextended_relation for purposes of obtaining blobs,
**	    etc ... info, fails when iiextended_relation is not iilinked.
**	10-jul-96 (nick)
**	    Lock flag for wait had the opposite setting to that required.
**	    Remove magic number and unused variable. 
**	    Fix error handling.
**	16-jan-1997 (nanpr01)
**	    Duplicate messages are being displayed for errors . #80068.
*/
static DB_STATUS
rfp_get_blob_info(
DMF_RFP             *rfp,
DMF_JSX             *jsx,
DMP_DCB             *dcb)
{
    RFP_QUEUE		*totq;
    RFP_TBLCB           *tblcb;
    RFP_TBLHCB          *hcb = rfp->rfp_tblhcb_ptr;
    DB_TAB_ID           iiextended_rel;
    DB_TAB_ID           blob_tabid;
    DMP_RCB		*etab_rcb = 0;
    DM2R_KEY_DESC	key_desc[1];
    DMP_ETAB_CATALOG	etab_rec;
    DM_TID		tid;
    i4		local_error;
    DB_STATUS		local_status;
    DB_STATUS		status;
    i4             dbdb_flag = 0;
    DB_TRAN_ID          tran_id;
    DB_TAB_TIMESTAMP    stamp;
    RFP_TBLCB           *blob_tblcb;
    i4			error;
    DB_ERROR		local_dberr;

    CLRDBERR(&jsx->jsx_dberr);

    /* Begin a transaction */
    dmf_rfp_dcb = dcb;

    dbdb_flag = DM2D_NOLOGGING;
    if ((jsx->jsx_status & JSX_WAITDB) == 0)
        dbdb_flag |= DM2D_NOWAIT;

    status = rfp_open_database (rfp, jsx, dcb, dbdb_flag);

    if (status != E_DB_OK)
    {
	return (status);
    }

    do
    {
	/* Open iiextended_relation catalog */
	iiextended_rel.db_tab_base = DM_B_ETAB_TAB_ID;
	iiextended_rel.db_tab_index = DM_I_ETAB_TAB_ID;

	status = dm2t_open(dcb, &iiextended_rel, DM2T_IX, DM2T_UDIRECT,
			DM2T_A_READ, 0, 20, 0, 0, dmf_svcb->svcb_lock_list,
			0, 0, 0, &tran_id, &stamp, &etab_rcb, (DML_SCB *)0,
			&jsx->jsx_dberr);

	if (status != E_DB_OK)
	{
	    /*
	    ** If iiextended_relation doesn't exist, there are no blobs in
	    ** the database.
	    */
	    if (jsx->jsx_dberr.err_code == E_DM0054_NONEXISTENT_TABLE)
	    {
		status = E_DB_OK;
		CLRDBERR(&jsx->jsx_dberr);
	    }
	    else
	    {
		/* this is an error we are interested in */
		dmfWriteMsg(&jsx->jsx_dberr, 0, 0);
	    }
	    break;
	}

	/* 
	** Loop through all the user supplied tables gathering more info
	*/
	for ( totq = hcb->tblhcb_htotq.q_next;
	      totq != &hcb->tblhcb_htotq;
	      totq = tblcb->tblcb_totq.q_next
	      )
	{
	    tblcb = (RFP_TBLCB *)((char *)totq - 
				CL_OFFSETOF(RFP_TBLCB, tblcb_totq));

	    /* Skip table if not user specified */
	    if ((tblcb->tblcb_table_status & RFP_USER_SPECIFIED ) == 0)
		continue;

	    /* Build the key for positioning the table. */
	    key_desc[0].attr_operator = DM2R_EQ;
	    key_desc[0].attr_number = DM_1_ETAB_KEY;
	    key_desc[0].attr_value = (char *)&tblcb->tblcb_table_id.db_tab_base;

	    status = dm2r_position(etab_rcb, DM2R_QUAL, key_desc,
		(i4)1, (DM_TID *)0, &jsx->jsx_dberr);
	    if (status != E_DB_OK)
		break;

	    /*
	    **  For each index record for this base table update the 
	    ** corresponding TBLCB entry as RFP_BLOB_REQUIRED.
	    */
	    for (;;)
	    {
		/*
		** Get the next indexes record.
		** This read loop is expected to be terminated by a NONEXT 
		** return from this get call.
		*/
		status = dm2r_get(etab_rcb, &tid, DM2R_GETNEXT,
			(char *)&etab_rec, &jsx->jsx_dberr);
		if (status != E_DB_OK)
		{
		    if (jsx->jsx_dberr.err_code == E_DM0055_NONEXT)
		    {
			status = E_DB_OK;
			CLRDBERR(&jsx->jsx_dberr);
		    }
		    break;
		}

		/*
		** Make sure it's one that we want.
		*/
		if (etab_rec.etab_base != tblcb->tblcb_table_id.db_tab_base)
		    continue;

		blob_tabid.db_tab_base = etab_rec.etab_extension;
		blob_tabid.db_tab_index = 0;

		if (rfp_locate_tblcb( rfp, &blob_tblcb, &blob_tabid))
		{
		    blob_tblcb->tblcb_table_status |= RFP_BLOB_REQUIRED;

		    /* Unless -noblobs specified, recover the blob */
		    if (( jsx->jsx_status1 & JSX1_NOBLOBS ) == 0)
			blob_tblcb->tblcb_table_status |= RFP_RECOVER_TABLE;
		}
		else
		{
		    /* FIX ME */
		}
	    }
	}
    } while (FALSE);


    /*
    ** Close iiextended_relation table
    */
    if (etab_rcb)
    {
	local_status = dm2t_close(etab_rcb, 0, &local_dberr);
	if (local_status != E_DB_OK)
	{
	    dmfWriteMsg(&local_dberr, 0, 0);
	    if (local_status > status)
	    {
		status = local_status;
		jsx->jsx_dberr = local_dberr;
	    }
	}
    }

    /*
    ** Before we exit check if the database was opened within this
    ** function, if so close db
    */
    local_dberr = jsx->jsx_dberr;

    local_status = rfp_close_database (rfp, dcb, dbdb_flag);

    if (local_status != E_DB_OK)
    {
	dmfWriteMsg(&jsx->jsx_dberr, 0, 0);
	if (local_status > status)
	    status = local_status;
    }
    else
        jsx->jsx_dberr = local_dberr;

    return (status);
}

/*{
** Name: rfp_wait_free - wait for a free resource
**
**
** Description:  Waits until a device is free to be used.
**
** Inputs:
**	jsx			Pointer
**
** Outputs:
**	err_code		Pointer to variable to return error.
**
**	Returns:
**	E_DB_OK
**	E_DB_ERROR
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	20-may-1996 (hanch04)
**	    Added Parallel Checkpoint support
**		Created.
**	02-Mar-1999 (kosma01)
**	    Restoring a database using multiple devices (#m) on 
**	    platforms built with xCL_086_SETPGRP_0_ARGS can cause
**	    a loop in rfp_wait_free(), (sep test lar97).
**	    When xCL_086_ is defined rfp_wait_free is not watching
**	    the pid of the parent of the "real work" child, but of 
**	    the "real work" child's grandparent. When the grandparent
**	    and "real work" child die at the same time. Ingres's 
**	    CSsigchld() handler may reap the "real work" child and 
**	    not the pid that rfp_wait_free() is watching. If this 
**	    happens enough (#m times), then rfp_wait_free() is stuck
**	    in a loop. I changed rfp_wait_free() to really wait() on
**	    any still active pid, if no other devices are available.
**	10-nov-1999 (somsa01)
**	    Use CK_is_alive() instead of PCis_alive(). Also, on NT
**	    PID is defined as an unsigned int. Therefore, typecast
**	    dev_pid to an i4 before testing it. Also, print out the
**	    child's error condition, if necessary.
*/
DB_STATUS
rfp_wait_free(
    DMF_JSX     *jsx,
    DMCKP_CB    *d)
{
    DB_STATUS	status;
    STATUS	waitstatus;
    i4     i, j;
    i4     env_value;
    char        *string;
    i4          value = 30000; /* Default sleep time 30 seconds */
    i4		error;

    CLRDBERR(&jsx->jsx_dberr);
 
    NMgtAt("II_MAX_CKPDB_LOOPS", &string);
    if (string                &&
        *string               &&
        (!CVal(string, &env_value)))
    {
        /* user override of default */
        value = env_value * 1000;
    }
    for (;;)
    {
        for (i = 0; i < jsx->jsx_dev_cnt; i++)
        {
	    if ( (i4)jsx->jsx_device_list[i].dev_pid > 0 )
            {
                status = CK_is_alive( jsx->jsx_device_list[i].dev_pid );
                if ( !status )
	        {
		    u_i4	errnum;
		    i4		os_ret, errcode;
		    STATUS	retval;
		    char	*command = NULL;

                    status = CK_exist_error(&jsx->jsx_device_list[i].dev_pid,
					    &retval, &errnum, &os_ret,
					    &command);
                    if ( !status )
		    {
			CL_ERR_DESC	err_desc;

			SETCLERR(&err_desc, 0, ER_system);
			err_desc.errnum = errnum;
			if (retval == CK_COMMAND_ERROR)
			{
			    err_desc.moreinfo[0].size = sizeof(os_ret);
			    err_desc.moreinfo[0].data._i4 = os_ret;

			    /*
			    ** Let's make sure that we do not overflow what
			    ** we have left to work with in the moreinfo
			    ** structure. Thus, since we have
			    ** (CLE_INFO_ITEMS-1) unions left to use, the
			    ** max would be CLE_INFO_MAX*(CLE_INFO_ITEMS-1).
			    */
			    err_desc.moreinfo[1].size = min(STlength(command),
				CLE_INFO_MAX*(CLE_INFO_ITEMS-1));
			    STlcopy(command, err_desc.moreinfo[1].data.string,
				    err_desc.moreinfo[1].size);
			}
			uleFormat( NULL, retval, &err_desc, ULE_LOG, NULL, NULL,
				    0, NULL, &errcode, 0 );
			MEfree(command);

			SETDBERR(&jsx->jsx_dberr, 0, E_DM1307_RFP_RESTORE);
    		    	return( E_DB_ERROR );
		    }
		    if (command)
			MEfree(command);
	    	    jsx->jsx_device_list[i].dev_pid = -1; 
		    d->dmckp_ckp_path     = jsx->jsx_device_list[i].dev_name;
		    d->dmckp_ckp_l_path   = jsx->jsx_device_list[i].l_device;
                    d->dmckp_ckp_path_pid = &jsx->jsx_device_list[i].dev_pid;
                    d->dmckp_ckp_path_status = &jsx->jsx_device_list[i].status;
    		    return( E_DB_OK );
	        }
		else
		{
		    j = i;
		}
	    }
	    else
	    {
		    d->dmckp_ckp_path    = jsx->jsx_device_list[i].dev_name;
		    d->dmckp_ckp_l_path  = jsx->jsx_device_list[i].l_device;
                    d->dmckp_ckp_path_pid = &jsx->jsx_device_list[i].dev_pid;
                    d->dmckp_ckp_path_status = &jsx->jsx_device_list[i].status;
    		    return( E_DB_OK );
	    }
		
        }
	waitstatus = PCwait(jsx->jsx_device_list[j].dev_pid);
    }
}

/*{
** Name: rfp_wait_all - wait for all free resources
**
**
** Description:  Waits until all devices are done.
**
** Inputs:
**	jsx			Pointer
**
** Outputs:
**	err_code		Pointer to variable to return error.
**
**	Returns:
**	E_DB_OK
**	E_DB_ERROR
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	20-may-1996 (hanch04)
**	    Added Parallel Checkpoint support
**		Created.
**	14-nov-1997 (canor01)
**	    If the child process goes away between the call to PCis_alive
**	    and the call to PCwait, PCwait may return a misleading error.
**	    Change to get return status from CK error log file if PCwait
**	    returns any error.
**	10-nov-1999 (somsa01)
**	    Use CK_is_alive() instead of PCis_alive(). Also, on NT
**	    PID is defined as an unsigned int. Therefore, typecast
**	    dev_pid to an i4 before testing it. Also, print out the
**	    child's error condition, if necessary.
*/
DB_STATUS
rfp_wait_all(
    DMF_JSX     *jsx)
{
    STATUS		status;
    DB_STATUS		ret_val = E_DB_OK;
    STATUS		waitstatus = OK;
    i4             i;

    CLRDBERR(&jsx->jsx_dberr);

        for (i = 0; i < jsx->jsx_dev_cnt; i++)
        {
            if ( (i4)jsx->jsx_device_list[i].dev_pid > 0 )
            {
                status = CK_is_alive( jsx->jsx_device_list[i].dev_pid );
                if ( status )
                    waitstatus = PCwait( jsx->jsx_device_list[i].dev_pid );
                if ( !status || waitstatus )
                {
		    u_i4	errnum;
		    i4		os_ret, errcode;
		    STATUS	retval;
		    char	*command = NULL;

                    status = CK_exist_error(&jsx->jsx_device_list[i].dev_pid,
					    &retval, &errnum, &os_ret,
					    &command);
                    /*
                    **  If an error exists, CK_exist_error returns OK.
                    */
                    if ( status == OK )
                    {
			CL_ERR_DESC	err_desc;

			SETCLERR(&err_desc, 0, ER_system);
			err_desc.errnum = errnum;
			if (retval == CK_COMMAND_ERROR)
			{
			    err_desc.moreinfo[0].size = sizeof(os_ret);
			    err_desc.moreinfo[0].data._i4 = os_ret;

			    /*
			    ** Let's make sure that we do not overflow what
			    ** we have left to work with in the moreinfo
			    ** structure. Thus, since we have
			    ** (CLE_INFO_ITEMS-1) unions left to use, the
			    ** max would be CLE_INFO_MAX*(CLE_INFO_ITEMS-1).
			    */
			    err_desc.moreinfo[1].size = min(STlength(command),
				CLE_INFO_MAX*(CLE_INFO_ITEMS-1));
			    STlcopy(command, err_desc.moreinfo[1].data.string,
				    err_desc.moreinfo[1].size);
			}
			uleFormat( NULL, retval, &err_desc, ULE_LOG, NULL, NULL,
				    0, NULL, &errcode, 0 );

			SETDBERR(&jsx->jsx_dberr, 0, E_DM1307_RFP_RESTORE);
                        ret_val = E_DB_ERROR;
                    }

		    if (command)
			MEfree(command);
                }
            }
        }
    return( ret_val );
}

static VOID
rfp_xn_diag(
DMF_JSX             *jsx,
DMF_RFP             *rfp,
DMVE_CB             *dmve)
{
    char	line_buffer[ER_MAX_LEN];
    i4		error;
    i4		count;
    RFP_TX	*tx;
    i4		tx_cnt = 0;
    char	date[32];

    /* Print any open transactions */
    for (tx = rfp->rfp_tx; tx != NULL; tx = tx->tx_next)
    {
	if ((tx->tx_status & RFP_TX_COMPLETE) == 0)
	{
	    TMstamp_str((TM_STAMP *)&(tx->tx_bt.bt_time), date);
	    TRformat(dmf_diag_put_line, 0, line_buffer, sizeof(line_buffer),
		"Open Transaction 0x%x%x with BT time %s for database %~t redo %d undo %d\n",
		tx->tx_bt.bt_header.tran_id.db_high_tran,
		tx->tx_bt.bt_header.tran_id.db_low_tran,
		date,
		sizeof(DB_DB_NAME), 
		rfp->rfp_dmve->dmve_dcb_ptr->dcb_name.db_db_name,
		tx->tx_process_cnt, tx->tx_undo_cnt);
	    tx_cnt++;
	}
    }

    if (tx_cnt)
    {
	TRformat(dmf_diag_put_line, 0, line_buffer, sizeof(line_buffer),
	    "Open Transaction count %d, database %~t is inconsistent.", 
	    tx_cnt,
	    sizeof(DB_DB_NAME), 
	    rfp->rfp_dmve->dmve_dcb_ptr->dcb_name.db_db_name);
    }

}

static i4
dmf_diag_put_line(
PTR		arg,
i4		length,
char		*buffer)
{
    i4	count;
    i4  error;

    /* put this info to terminal - and also to errlog.log */
    SIwrite(length, buffer, &count, stdout);
    SIwrite(1, "\n", &count, stdout);
    SIflush(stdout);

    uleFormat(NULL, E_DM1371_RFP_INFO, (CL_ERR_DESC *)NULL, ULE_LOG,
	NULL, (char *)NULL, (i4)0, (i4 *)NULL, &error, 1, length, buffer);
    return 0; /* Make compiler happy */
}


/*{
** Name: ex_handler	- DMF exception handler.
**
** Description:
**	Exception handler for rollforward journal processing.
**
**	An error message including the exception type and PC will be written
**	to the log file.
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
**	EXDECLARE causes us to return execution to dmfrcp routine.
**
** History:
**      10-feb-2003 (stial01)
**          Created.
*/
static STATUS
ex_handler(
EX_ARGS	    *ex_args)
{
    i4		err_code;

    if (ex_args->exarg_num == EX_DMF_FATAL)
    {
	err_code = E_DM904A_FATAL_EXCEPTION;
    }
    else if (ex_args->exarg_num == EXINTR)
    {
	dmf_diag_put_line(0, sizeof("Exiting due to interrupt...")-1,
			"Exiting due to interrupt...");

	return (EXDECLARE);
    }
    else
    {
	err_code = E_DM9049_UNKNOWN_EXCEPTION;
    }

    /*
    ** Report the exception.
    ** Note: the fourth argument to ulx_exception must be FALSE so
    ** that it does not try to call scs_avformat() since there is no
    ** session in the ATP.
    */
    (VOID) ulx_exception( ex_args, DB_DMF_ID, err_code, FALSE );

    /*
    ** Returning with EXDECLARE will cause execution to return to the
    ** top of the place where the handler was declared.
    */
    return (EXDECLARE);
}


/* 
** Name: rfp_prescan_btime_qual - Prescan journal files to determine
**	 		      transactions which satisfy the -b qualification.
**
** Description:
**      This routine will perform a prescan of the journal files, looking
**      for all xactions in which BT occurred before or on the -b specified,
**      and its corresponding ET occurred after the -b specified. 
**      The result is a chain of xactions which span the -b (BT before and
**      corresponding ET after). 
**
** Inputs:
**
** Outputs:
**      Returns:
**	Exceptions:
**
** Side Effects:
**
** History:
**	17-Dec-2003 (thaju02)
**	    Created.
**	28-nov-2005 (thaju02)
**	    Do not remove xaction from list if et is equal to -b specified;
**	    match auditdb behavior. (B115565)
**	30-Sep-2004 (schka24)
**	    Must pass pointer-to-pointer to close routine.
**	
*/
DB_STATUS
rfp_prescan_btime_qual(
DMF_JSX		*jsx,
DMF_RFP		*rfp,
DMVE_CB		*dmve)
{
    DM0J_CTX            *jnl;
    DM0L_HEADER         *record;
    i4                  i;
    i4                  l_record;
    i4                  record_interval;
    i4                  record_time;
    i4                  record_count;
    i4                  byte_interval;
    i4                  byte_time;
    i4                  byte_count;
    DB_STATUS           status = E_DB_OK;
    char                line_buffer[132];
    bool                lsn_check = FALSE;
    DM_FILE             jnl_filename;
    DMCKP_CB            *d = &rfp->rfp_dmckp;
    char              *env;
    bool                no_jnl = FALSE;
    bool		done = FALSE;
    DB_TAB_TIMESTAMP    *timestamp;
    u_char              *tmp_dot;
    TM_STAMP		conv_stamp;
    char                tmp_time[TM_SIZE_STAMP];
    i4			error;
    DB_ERROR		local_dberr;

    CLRDBERR(&jsx->jsx_dberr);

    if (jsx->jsx_status & JSX_TRACE)
	TRdisplay("%@ RFP: PRESCAN for -b qualification\n");

    for (rfp->jnl_history_number =  rfp->jnl_first;
	 rfp->jnl_history_number <= rfp->jnl_last;
	 rfp->jnl_history_number++)
    {
	for (i =  rfp->rfp_jnl_history[rfp->jnl_history_number].ckp_f_jnl;
	     i <= rfp->rfp_jnl_history[rfp->jnl_history_number].ckp_l_jnl;
	     i++)
	{
	    if (i == 0)
	    {
		/* No journaling in this history, go to next history. */
                break;
	    }
	    else
		TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
		    "%@ RFP: Preprocessing journal file sequence %d.\n", i);

            /*
            ** Open journal file.
            */
            dm0j_filename( DM0J_MERGE, i, -1, &jnl_filename, (DB_TAB_ID *)NULL );
            d->dmckp_jnl_file   = jnl_filename.name;
            d->dmckp_jnl_l_file = sizeof( jnl_filename );
	    status = dm0j_open(DM0J_MERGE,
			(char *)&dmve->dmve_dcb_ptr->dcb_jnl->physical,
			dmve->dmve_dcb_ptr->dcb_jnl->phys_length,
			&dmve->dmve_dcb_ptr->dcb_name,
			rfp->jnl_block_size, i, 0, 0, DM0J_M_READ,
			-1, DM0J_FORWARD, (DB_TAB_ID *)0, &jnl, &jsx->jsx_dberr);
	    if (status != E_DB_OK)
	    {
		if (jsx->jsx_dberr.err_code == E_DM9034_BAD_JNL_NOT_FOUND)
		{
		    uleFormat(&jsx->jsx_dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL,
				&error, 0);
		    TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
		    "%@ RFP: Journal file sequence %d not found -- skipped.\n",
		    i );
		    status = E_DB_OK;
		    CLRDBERR(&jsx->jsx_dberr);
		    continue;
		}
		else
		{
		    uleFormat(&jsx->jsx_dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL,
				&error, 0);
		    SETDBERR(&jsx->jsx_dberr, 0, E_DM1305_RFP_JNL_OPEN);
		    break;
		}
	    }

	    /*  Loop through the records of this journal file. */
	    for (;;)
	    {
		status = dm0j_read(jnl, (PTR *)&record, &l_record, &jsx->jsx_dberr);

		if (status != E_DB_OK)
		{
		    if (jsx->jsx_dberr.err_code == E_DM0055_NONEXT)
		    {
			status = E_DB_OK;
			CLRDBERR(&jsx->jsx_dberr);
			break;
		    }
		    uleFormat(&jsx->jsx_dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL,
				&error, 0);
		    SETDBERR(&jsx->jsx_dberr, 0, E_DM1305_RFP_JNL_OPEN);
		    break;
		}

		if (record->type == DM0LBT)
		{
		    timestamp = &((DM0L_BT *)record)->bt_time;
		    TMstamp_str((TM_STAMP *) timestamp, tmp_time);
		    tmp_dot = (u_char *)STindex(tmp_time, ".", 0);
		    *tmp_dot = EOS;
		    TMstr_stamp(tmp_time, &conv_stamp);

		    if (TMcmp_stamp(&conv_stamp, (TM_STAMP *) &jsx->jsx_bgn_time) > 0)
		    {
			/*
			** We have read beyond the -b specified,
			** no more prescanning needed, since all
			** BT/ET from here on in should qualify.
			*/

			if (jsx->jsx_status & JSX_TRACE)
			    TRdisplay("%@ RFP: PRESCAN - tx %x%x BT record \
timestamp is beyond the -b specified\n",
					record->tran_id.db_high_tran,
					record->tran_id.db_low_tran);

			done = TRUE;
			status = E_DB_OK;
			break;
		    }
		    else
		    {
			RFP_BQTX	*tx;
			RFP_BQTX	**txq;

		        /* Add tx to qualifying xaction list */
			status = dm0m_allocate(sizeof(RFP_BQTX), 0, RFPBQTX_CB,
					RFPBQT_ASCII_ID, (char *)rfp,
					(DM_OBJECT **)&tx, &jsx->jsx_dberr);
			if (status != E_DB_OK)
			{
			    /* need err handling here */
			}
	
			STRUCT_ASSIGN_MACRO(record->tran_id, tx->tx_tran_id);
    
			if (jsx->jsx_status & JSX_TRACE)
			    TRdisplay("%@ RFP: PRESCAN - adding tx %x%x to \
qualifying xaction list\n", 
				record->tran_id.db_high_tran,
				record->tran_id.db_low_tran);
			
			/* Put BQTX on -b qualifying transaction queue */
			tx->tx_next = rfp->rfp_bqtx;
			tx->tx_prev = (RFP_BQTX *)NULL;
			if (rfp->rfp_bqtx != NULL)
			    rfp->rfp_bqtx->tx_prev = tx;
			rfp->rfp_bqtx = tx;
	
			/* Put BQTX on hash bucket queue */
			txq = &rfp->rfp_bqtxh[record->tran_id.db_low_tran & (RFP_TXH_CNT - 1)];	
			tx->tx_hash_next = *txq;
			tx->tx_hash_prev = (RFP_BQTX *)NULL;
			if (*txq)
			    (*txq)->tx_hash_prev = tx;
			*txq = tx;
			rfp->rfp_bqtx_count++;
		    }
		}
		else if (record->type == DM0LET)
		{
		    RFP_BQTX	*tx;
		    i4		cmp = 0;

		    timestamp = &((DM0L_ET *)record)->et_time;
		    TMstamp_str((TM_STAMP *) timestamp, tmp_time);
		    tmp_dot = (u_char *)STindex(tmp_time, ".", 0);
		    *tmp_dot = EOS;
		    TMstr_stamp(tmp_time, &conv_stamp);

		    /* 
		    ** If ET timestamp is less than -b specified
		    ** remove the xaction from the qualifying list.
		    */
		    cmp = TMcmp_stamp(&conv_stamp, 
					(TM_STAMP *)&jsx->jsx_bgn_time);
		    if (cmp < 0)
		    {
        		for (tx = rfp->rfp_bqtxh[record->tran_id.db_low_tran & 
					(RFP_TXH_CNT - 1)]; 
			      tx != NULL; tx = tx->tx_hash_next)
			{
			    if ((tx->tx_tran_id.db_high_tran == 
					    record->tran_id.db_high_tran) &&
				(tx->tx_tran_id.db_low_tran  == 
					    record->tran_id.db_low_tran))
			    {
				remove_qualtx(rfp, tx);
				if (jsx->jsx_status & JSX_TRACE)
				    TRdisplay("%@ RFP: PRESCAN - tx %x%x does \
not qualify; remove from xaction list\n", 
					record->tran_id.db_high_tran,
					record->tran_id.db_low_tran);
				break;
			    }					
			}
		    }
		    else if (cmp > 0)
		    {
			if (jsx->jsx_status & JSX_TRACE)
			    TRdisplay("%@ RFP: PRESCAN tx %x%x ET record \
timestamp is  beyond the -b specified\n",
					record->tran_id.db_high_tran,
					record->tran_id.db_low_tran);
			done = TRUE;
			status = E_DB_OK;
			break;
		    }
		}
	    }

	    /*  Close the current journal file, nulls out jnl. */
	    if (jnl)
	    {
		(VOID) dm0j_close(&jnl, &local_dberr);
	    }
	    if ((status != E_DB_OK) || done)
		return (status);
	}

        if ( status != E_DB_OK )
            return( status );
    }

    return (status);
}


/*
** Name: remove_qualtx - remove tx context from -b qualifying tx list
**
** Description:
**	 This routine is responsible for removing a tx from the -b
**	 qualifying tx list when the ET is less than or equal to 
**	 the -b specified. 
**
** Inputs:
**	rfp - pointer to rfp control block
**	tx - tx context to remove from list
**
** Outputs:
**	None.
**
** Side Effects:
**
** History:
**      17-Dec-2003 (thaju02)
**          Created.
**	30-Sep-2004 (schka24)
**	    deallocate wants pointer-to-pointer.
**
*/
static VOID
remove_qualtx(
DMF_RFP		*rfp,
RFP_BQTX	*tx)
{
    if (tx->tx_next != NULL)
        tx->tx_next->tx_prev = tx->tx_prev;

    if (tx->tx_prev != NULL)
        tx->tx_prev->tx_next = tx->tx_next;
    else
        rfp->rfp_bqtx = tx->tx_next;

    /*
    ** Remove from the transaction's hash bucket queue.
    */
    if (tx->tx_hash_next != NULL)
        tx->tx_hash_next->tx_hash_prev = tx->tx_hash_prev;

    if (tx->tx_hash_prev != NULL)
        tx->tx_hash_prev->tx_hash_next = tx->tx_hash_next;
    else
        rfp->rfp_bqtxh[tx->tx_tran_id.db_low_tran & (RFP_TXH_CNT - 1)] =
                                                            tx->tx_hash_next;

    rfp->rfp_bqtx_count--;

    dm0m_deallocate((DM_OBJECT **)&tx);
}


DB_STATUS
prescan_records(
DMF_RFP		*rfp,
DMVE_CB         *dmve,
PTR		log_rec)
{
    DMF_JSX		*jsx = rfp->rfp_jsx;
    DM0L_HEADER		*header = (DM0L_HEADER *)log_rec;
    DB_STATUS		status = E_DB_OK;
    RFP_OCTX            *oc, *octx = 0;
    DB_TAB_ID           *tabid = 0;
    DM2U_OSRC_CB	*osrc_entry, *osrc = 0;
    DMP_MISC		*osrc_misc_cb;
    DMP_RNL_ONLINE	*rnl;
    i4			error;

    CLRDBERR(&jsx->jsx_dberr);

    for(;;)
    {
	if (header->type == DM0LBSF)
	{
	    /* if DM0LBSF, create rfp_octx block */
	    status = create_om_context(rfp, dmve, log_rec);
	    break;
	}

	if (!rfp->rfp_octx_count)
	    break;

	/* locate octx */
	if (header->type == DM0LRNLLSN)
	{
	    DM0L_RNL_LSN    *record = (DM0L_RNL_LSN *)log_rec;
	    PTR		    rnl_lsn = ((PTR)log_rec + 
						sizeof(DM0L_RNL_LSN));
	    i4		    copy_size = record->rl_lsn_cnt * 
						sizeof(DMP_RNL_LSN);
	    LG_LSN	    *lsn = &record->rl_bsf_lsn;

	    tabid = &(record->rl_tbl_id); 

	    for ( oc = rfp->rfp_octx; oc; oc = oc->octx_next)
	    {
		if ( LSN_EQ(lsn, &oc->octx_bsf_lsn) &&
		     (oc->octx_bsf_tbl_id.db_tab_base ==
				tabid->db_tab_base) ) 
		{
		    octx = oc;
		    break;
		}
	    }
	    
	    if (!octx)
	    {
		/* FIX ME - set *err_code */
		status = E_DB_ERROR;
		break;
	    }

	    for ( osrc_entry = (DM2U_OSRC_CB *)octx->octx_osrc_next; 
		  osrc_entry; 
		  osrc_entry = osrc_entry->osrc_next )
	    {
		if ( !MEcmp(&osrc_entry->osrc_tabid, tabid, sizeof(DB_TAB_ID)) )
		{
		    osrc = osrc_entry;
		    break;
		}
	    }

	    if (osrc == NULL)
	    {
		/* allocate otb and hang off of octx's otb list */
		status = dm0m_allocate(sizeof(DMP_MISC) + sizeof(DM2U_OSRC_CB),
				DM0M_ZERO, 
				MISC_CB, MISC_ASCII_ID, 
				(char *)NULL, (DM_OBJECT **)&osrc_misc_cb, 
				&jsx->jsx_dberr);
		osrc = (DM2U_OSRC_CB *)(osrc_misc_cb + 1);
		osrc_misc_cb->misc_data = (PTR)osrc;
		osrc->osrc_misc_cb = osrc_misc_cb;
		STRUCT_ASSIGN_MACRO(*tabid, osrc->osrc_tabid);
	
		/* put osrc block on list */
		osrc->osrc_next = (DM2U_OSRC_CB *)octx->octx_osrc_next;
		octx->octx_osrc_next = (PTR)osrc;		
	    }

	    rnl = &(osrc->osrc_rnl_online);

	    /* alloc space to hold rnl lsn array */	
	    if (rnl->rnl_xlsn == NULL)
	    {
		/* allocate memory to put the rnl_lsn */
		status = dm0m_allocate(sizeof(DMP_MISC) +
                        	(record->rl_lsn_totcnt * sizeof(DMP_RNL_LSN)), 
				DM0M_ZERO, MISC_CB, MISC_ASCII_ID, 
				(char *)NULL, (DM_OBJECT **)&rnl->rnl_xlsn,
				&jsx->jsx_dberr);
		if (status != E_DB_OK)
		{
		    uleFormat(&jsx->jsx_dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, 
				NULL, &error, 0);
		    SETDBERR(&jsx->jsx_dberr, 0, E_DM1304_RFP_MEM_ALLOC);
		    break;
		}
		rnl->rnl_lsn = (DMP_RNL_LSN *)((char *)
				rnl->rnl_xlsn + sizeof(DMP_MISC));
		rnl->rnl_xlsn->misc_data = (char*)rnl->rnl_lsn;
		rnl->rnl_lsn_cnt = record->rl_lsn_totcnt;
	    }

	    /* copy the rnl_lsn array into the octx */
	    MEcopy(rnl_lsn, copy_size, 
		(char *)(rnl->rnl_lsn + rnl->rnl_read_cnt));
	    rnl->rnl_read_cnt += record->rl_lsn_cnt;
	}
	break;
    }  /* end of for(;;) */ 

    if (status)
    {
	/* FIX ME - report something */
    }
    return(status);
} 
 

DB_STATUS
create_om_context(
DMF_RFP		*rfp,
DMVE_CB         *dmve,
DM0L_BSF	*log_rec)
{
    DMF_JSX		*jsx = rfp->rfp_jsx;
    DMP_DCB             *dcb = dmve->dmve_dcb_ptr;
    RFP_OCTX            *octx;
    RFP_SIDEFILE	*sf;
    DB_STATUS           status = E_DB_OK;
    i4			error;

    CLRDBERR(&jsx->jsx_dberr);

    for (;;)
    {
        status = dm0m_allocate(sizeof(RFP_OCTX), DM0M_ZERO, 
			RFP_OCTX_CB, RFP_OCTX_ASCII_ID, 
			(char *)rfp, (DM_OBJECT **)&octx,
                        &jsx->jsx_dberr);
        if (status != E_DB_OK)
        {
	    uleFormat(&jsx->jsx_dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL, &error, 0);
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM1304_RFP_MEM_ALLOC);
	    break;
        }

	/* initialize octx control block */
	STRUCT_ASSIGN_MACRO(log_rec->bsf_header.tran_id,
			octx->octx_bsf_tran_id);
	STRUCT_ASSIGN_MACRO(log_rec->bsf_header.lsn, octx->octx_bsf_lsn);
	STRUCT_ASSIGN_MACRO(log_rec->bsf_tbl_id, octx->octx_bsf_tbl_id);
	STRUCT_ASSIGN_MACRO(log_rec->bsf_name, octx->octx_bsf_name);

        /* Add octx control block to queue */
        octx->octx_next = rfp->rfp_octx;
        octx->octx_prev = (RFP_OCTX *)NULL;
	if (rfp->rfp_octx != NULL)
	    rfp->rfp_octx->octx_prev = octx;
	rfp->rfp_octx = octx;

	rfp->rfp_octx_count++;

        break;
    }

    /*
    ** Error Cleanup.
    */
    if (status)
    {
	if (octx)
            dm0m_deallocate((DM_OBJECT **)&octx);
    }

    return(status);
}


static DB_STATUS
rfp_check_lsn_waiter(
DMF_RFP         *rfp,
DM0L_HEADER	*log_rec)
{
    DMF_JSX		*jsx = rfp->rfp_jsx;
    RFP_LSN_WAIT	*cand, *waiter = 0;
    RFP_OCTX		*octx = 0;
    DB_STATUS		status = E_DB_OK;
    DB_TAB_ID		*tabid = 0;
    DM2U_MXCB		*m;
    DMP_TCB		*t;
    DB_ERROR		error;

    CLRDBERR(&jsx->jsx_dberr);
    
    do
    {
	/* 
	** compare log record's lsn to list of lsn's we are waiting on
	** if found, continue with sort/load 
	*/
	for (cand = rfp->rfp_lsn_wait; cand != NULL; cand = cand->wait_next)
	{
	    if (LSN_EQ(&log_rec->lsn, &cand->wait_rnl_lsn->rnl_lsn))
	    {
		waiter = cand;
		break;
	    }
	}

	/* there are no waiters or not waiting on this log rec's lsn */ 
	if (!waiter)
	    break;

	/* waiting on this lsn, attempt to continue with load/sort */
	octx = waiter->wait_octx;    
	m = (DM2U_MXCB *)octx->octx_mxcb;
	t = m->mx_resume_spcb->spcb_input_rcb->rcb_tcb_ptr;

	if (rfp->rfp_jsx->jsx_status & JSX_TRACE)
	    TRdisplay("%@ rfp_check_lsn_waiter: %~t restart loader \
pg %d now at lsn=(%x,%x)\n",
		    sizeof(DB_TAB_NAME),
		    t->tcb_rel.relid.db_tab_name,
		    waiter->wait_rnl_lsn->rnl_page,
		    waiter->wait_rnl_lsn->rnl_lsn.lsn_high,
		    waiter->wait_rnl_lsn->rnl_lsn.lsn_low);

	/* take this waiter off of the queue */
	if (waiter->wait_next != NULL)
	    waiter->wait_next->wait_prev = waiter->wait_prev;
	if (waiter->wait_prev != NULL)
	    waiter->wait_prev->wait_next = waiter->wait_next;
	else
	    rfp->rfp_lsn_wait = waiter->wait_next;
	rfp->rfp_lsn_wait_cnt--;
	dm0m_deallocate((DM_OBJECT **)&waiter);

	status = dm2u_load_table((DM2U_MXCB *)octx->octx_mxcb, &error);
	if ( status )
	    jsx->jsx_dberr = error;

	if ((status == E_DB_INFO) && 
	    (jsx->jsx_dberr.err_code == E_DM9CB1_RNL_LSN_MISMATCH))
	{
	    DB_STATUS	local_status;
	    
	    /* allocate new page/lsn waiter block */
	    /* add to rfp_lsn_wait list */
	    local_status = rfp_add_lsn_waiter(rfp, octx);
	    if (local_status)
		status = local_status;
	    else
		status = E_DB_OK; /* continue applying journals */
	}
    } while(FALSE);
	
    if (status == E_DB_ERROR)
    {
	/* FIX ME - report error */
    }
    return(status);
}

static DB_STATUS
rfp_add_lsn_waiter(
DMF_RFP		*rfp,
RFP_OCTX	*octx)
{
    DMF_JSX		*jsx = rfp->rfp_jsx;
    RFP_LSN_WAIT	*waiter = 0;
    DB_STATUS		status = E_DB_OK;
    DM2U_MXCB		*m = (DM2U_MXCB *)octx->octx_mxcb;
    DM2U_SPCB		*sp = m->mx_resume_spcb;
    DMP_RNL_ONLINE	*rnl = sp->spcb_input_rcb->rcb_rnl_online;
    DMP_TCB		*t = sp->spcb_input_rcb->rcb_tcb_ptr;
    i4			error;

    CLRDBERR(&jsx->jsx_dberr);

    /* allocate & initialize rfp_lsn_wait block */
    status = dm0m_allocate(sizeof(RFP_LSN_WAIT), 0, RFP_LSN_WAIT_CB,
			RFP_LSN_WAIT_ASCII_ID, (char *)rfp,
			(DM_OBJECT **)&waiter, &jsx->jsx_dberr);
    if (status != E_DB_OK)
    {
	uleFormat(&jsx->jsx_dberr, 0, NULL, ULE_LOG, NULL, NULL, 0,
				NULL, &error, 0);
	SETDBERR(&jsx->jsx_dberr, 0, E_DM1304_RFP_MEM_ALLOC);
	return(E_DB_ERROR);
    }

    if (rfp->rfp_jsx->jsx_status & JSX_TRACE)
	TRdisplay("%@ rfp_add_lsn_waiter: %~t wait for \
pg %d, rnl_lsn=(%x,%x)\n",
		sizeof(DB_TAB_NAME),
		t->tcb_rel.relid.db_tab_name,
		rnl->rnl_lsn_wait->rnl_page,
		rnl->rnl_lsn_wait->rnl_lsn.lsn_high,
		rnl->rnl_lsn_wait->rnl_lsn.lsn_low);

    STRUCT_ASSIGN_MACRO(t->tcb_rel.reltid, waiter->wait_tabid);
    waiter->wait_rnl_lsn = rnl->rnl_lsn_wait;
    waiter->wait_octx = octx;

    /* add waiter to rfp page/lsn wait queue */
    waiter->wait_next = rfp->rfp_lsn_wait;
    waiter->wait_prev = (RFP_LSN_WAIT *)NULL;
    if (rfp->rfp_lsn_wait != NULL)
	rfp->rfp_lsn_wait->wait_prev = waiter;
    rfp->rfp_lsn_wait = waiter;	
    rfp->rfp_lsn_wait_cnt++;

    return(E_DB_OK);
}

static DB_STATUS
rfp_osrc_close(
DMF_RFP		*rfp,
RFP_OCTX	*octx)
{
    DMF_JSX		*jsx = rfp->rfp_jsx;
    DM2U_OSRC_CB	*osrc = (DM2U_OSRC_CB *)octx->octx_osrc_next;
    DB_STATUS		status = E_DB_OK;
    DMP_RCB		*master_rcb = (DMP_RCB *)NULL;
    DMP_MISC		*osrc_misc_cb;
    i4			error;

    CLRDBERR(&jsx->jsx_dberr);

    if (osrc->osrc_rnl_rcb != osrc->osrc_master_rnl_rcb)
	master_rcb = osrc->osrc_master_rnl_rcb;

    for (; osrc && (status == E_DB_OK); osrc = osrc->osrc_next)
    {
	status = dm2t_close(osrc->osrc_rnl_rcb, DM2T_PURGE, &jsx->jsx_dberr);
	if (status)
	{
	    dmfWriteMsg(&jsx->jsx_dberr, 0, 0);
	    break;
	}
    }

    if (master_rcb)
    {
	status = dm2t_close(master_rcb, DM2T_PURGE, &jsx->jsx_dberr);
	if (status)
	    dmfWriteMsg(&jsx->jsx_dberr, 0, 0);
    }

    return(status);
}

static DB_STATUS
rfp_prompt(
DMF_RFP             *rfp,
DMF_JSX             *jsx,
i4		    *rfp_action)
{
    STATUS	ret_val;
    CL_ERR_DESC	sys_err;
    char	line_buffer[132];
    char	buffer[256];
    i4		amount_read;
    DB_STATUS   status = E_DB_OK;

    *rfp_action = RFP_ROLLBACK;

    for (;;)
    {
	MEfill(sizeof(buffer), '\0', buffer);
	TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
    "\nEnter ROLLBACK_TRANSACTIONS, CONTINUE_IGNORE_TABLE or CONTINUE_IGNORE_ERROR : \n");

	if (ret_val = TRrequest(buffer, sizeof(buffer), &amount_read,
				&sys_err, NULL))
	{
	    status = E_DB_ERROR;
	    break;
	}

	CVlower(buffer);

	if (STncmp( &buffer[0], "rollback_transactions", 21) == 0)
	{
	    *rfp_action = RFP_ROLLBACK;
	}
	else if (STncmp( &buffer[0], "continue_ignore_table", 21) == 0)
	{
	    *rfp_action = RFP_CONTINUE_IGNORE_TBL; 
	}
	else if (STncmp( &buffer[0], "continue_ignore_error", 21) == 0)
	{
	    *rfp_action = RFP_CONTINUE_IGNORE_ERR;
	}
	else	    /* what ? */
	{
	    continue;
	}

	TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
	    "\nYou have specified '%s'. Is this correct Y/N? \n", buffer);

	MEfill(sizeof(buffer), '\0', buffer);
	if (ret_val = TRrequest(buffer, sizeof(buffer), &amount_read,
				&sys_err, NULL))
	{
	    status = E_DB_ERROR;
	    break;
	}

	CVlower(buffer);
	if (buffer[0] == 'y')
	    break;

	*rfp_action = RFP_ROLLBACK; 
	continue; /* reprompt */
    }

    return(status);
}

static DB_STATUS
rfp_er_handler(
DMF_RFP             *rfp,
DMF_JSX             *jsx)
{
    DB_STATUS   new_status;
    DB_STATUS   dbstatus;
    STATUS      status;
    CL_ERR_DESC	sys_err;
    char	line_buffer[132];
    char	buffer[256];
    i4		amount_read;
    char	save_char;

    if ((jsx->jsx_status2 & JSX2_ON_ERR_PROMPT) == 0)
    {
	if (jsx->jsx_status1 & JSX1_IGNORE_FLAG)
	{
	    jsx->jsx_ignored_errors = TRUE;
	    return (E_DB_OK);
	}
	else
	{
	    return (E_DB_ERROR);
	}
    }

    /* Here if JSX2_ON_ERR_PROMPT */
    for (;;)
    {
	TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
	    "Enter IGNORE_ERROR or PROCESS_ERROR: \n");

	MEfill(sizeof(buffer), '\0', buffer);
	if (status = TRrequest(buffer, sizeof(buffer), &amount_read,
				&sys_err, NULL))
	    break;

	CVlower(buffer);

	if (STncmp( &buffer[0], "process_error", 13) &&
	    STncmp( &buffer[0], "ignore_error", 12) )
	{
	    /* what ? */
	    continue;
	}

	save_char = buffer[0];
	TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
	    "\nYou have specified '%s'. Is this correct Y/N? \n", buffer);

	MEfill(sizeof(buffer), '\0', buffer);
	if (status = TRrequest(buffer, sizeof(buffer), &amount_read,
				&sys_err, NULL))
	    break;

	CVlower(buffer);
	if (buffer[0] == 'y')
	{
	    if (save_char == 'i')
		new_status = E_DB_OK;
	    else
		new_status = E_DB_ERROR;
	    break;
	}

	continue; /* reprompt */
    }

    return(new_status);
}


/*{
** Name: rfp_read_incr_journals_db  - Read and apply journal records.
**
** Description:
**      This routine looks for a new (incremental) journal file and if
**      one exists applies the journal records. Upon successful
**      completion, the new incremental journal file will be added to
**      the jnl history for the last checkpoint.
**
** Inputs:
**      rfp                             Rollforward context.
**      dmve                            Recovery control block.
**
** Outputs:
**      new_jnl_cnt			Number of new journals processed
**      err_code                        Reason for error return status.
**      Returns:
**          E_DB_OK
**          E_DB_ERROR
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      12-apr-2007 (stial0)
**          Created (from rfp_read_journal_db)
*/
static DB_STATUS
rfp_read_incr_journals_db(
DMF_JSX             *jsx,
DMF_RFP             *rfp,
DMVE_CB             *dmve,
i4		    *new_jnl_cnt)
{
    DMP_DCB             *dcb = dmve->dmve_dcb_ptr;
    DM0J_CTX		*jnl;
    DM0J_CTX		*jnl2;
    DM0L_HEADER		*record;
    i4		i;
    i4		l_record;
    i4             record_interval;
    i4             record_time;
    i4             record_count;
    i4             byte_interval;
    i4             byte_time;
    i4             byte_count;
    DB_STATUS		status;
    char		    line_buffer[132];
    bool		lsn_check = FALSE;
    DM_FILE             jnl_filename;
    DMCKP_CB            *d = &rfp->rfp_dmckp;
    char              *env;
    i4			jnl_read = 0;
    i4			jnl_done = 0;
    LG_LSN		last_rec_read_lsn;
    i4			save_rfp_status;
    DM0C_CNF		*cnf;
    i4			local_error;
    i4			last_jnl = 0;
    i4			incr_jnl = 0;
    i4			blk_seq;
    i4			error;
    DB_ERROR		local_dberr;

    CLRDBERR(&jsx->jsx_dberr);

    last_jnl = rfp->jnl_fil_seq;
    incr_jnl = last_jnl + 1;

    TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
	"%@ RFP: Searching for incremental jnl %d \n", incr_jnl);

    /*
    ** Initialise the DMCKP_CB structure, used during the OPENjournal stuff.
    */
    status = dmckp_init( d, DMCKP_RESTORE, &error );
    if ( status != E_DB_OK )
    {
	SETDBERR(&jsx->jsx_dberr, 0, error);
        return( status );
    }

    /*
    ** Setup the DMCKP stuff which will remain constant
    */
    d->dmckp_jnl_phase   = DMCKP_JNL_DO;
    d->dmckp_device_type = DMCKP_DISK_FILE;
    d->dmckp_jnl_path    = rfp->cur_jnl_loc.physical.name;
    d->dmckp_jnl_l_path  = rfp->cur_jnl_loc.phys_length;

    /*
    ** FIXME, test that this does not SEGV ever...
    */
    d->dmckp_first_jnl_file = incr_jnl;
    d->dmckp_last_jnl_file  = incr_jnl;


    /*
    ** Call the OpenJournal begin routine
    */
    status = dmckp_begin_journal_phase( d, &error );
    if ( status != E_DB_OK )
    {
	SETDBERR(&jsx->jsx_dberr, 0, error);
        return( status );
    }

    /*
    ** Check to see if the env. variables are set.  If not set, initialize
    ** to zero so they won't be used.
    */
    NMgtAt(ERx("II_RFP_JNL_RECORD_INTERVAL"), &env);
    if (env && *env)
        CVal(env, &record_interval);
    else
        record_interval = 0;
 
    NMgtAt(ERx("II_RFP_JNL_BYTE_INTERVAL"), &env);
    if (env && *env)
        CVal(env, &byte_interval);
    else
        byte_interval = 0;

    last_rec_read_lsn.lsn_high = last_rec_read_lsn.lsn_low = 0;

    if (jsx->jsx_status & JSX_BTIME)
    {
	dmve->dmve_flags |= DMVE_ROLLDB_BOPT;
	status = rfp_prescan_btime_qual(jsx, rfp, dmve);
    }

    /*
    ** Restore open xacts 
    ** - not necessary for the very first -incremental +j
    ** - when searching for the first new journal for this -incremental +j
    */
    if (rfp->rfp_jnl_history[rfp->jnl_last].ckp_f_jnl != 0 && *new_jnl_cnt == 0)
    {
	status = restore_open_xacts(jsx, rfp, dcb, dmve, last_jnl);
    } 

    /* For now process one new jnl at a time */
    for (i = incr_jnl; i <= (last_jnl+1); i++, incr_jnl++)
    {
	/*
	** Determine next journal file name for the OpenJournal
	*/
	dm0j_filename( DM0J_MERGE, i, -1, &jnl_filename, (DB_TAB_ID *)NULL );

	d->dmckp_jnl_file   = jnl_filename.name;
	d->dmckp_jnl_l_file = sizeof( jnl_filename );

	status = dmckp_work_journal_phase( d, &error );
	if ( status != E_DB_OK )
	{
	    SETDBERR(&jsx->jsx_dberr, 0, error);
	    break;
	}


	/*  Open the next journal file. */

	status = dm0j_open(DM0J_MERGE,
		    (char *)&dmve->dmve_dcb_ptr->dcb_jnl->physical,
		    dmve->dmve_dcb_ptr->dcb_jnl->phys_length,
		    &dmve->dmve_dcb_ptr->dcb_name,
		    rfp->jnl_block_size, i, 0, 0, DM0J_M_READ,
		    -1, DM0J_FORWARD, (DB_TAB_ID *)0, &jnl, &jsx->jsx_dberr);
	if (status != E_DB_OK)
	{
	    if (jsx->jsx_dberr.err_code == E_DM9034_BAD_JNL_NOT_FOUND)
	    {
		TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
		"%@ RFP: Journal file sequence %d not found -- skipped.\n",
		i );

		/* try next journal...we're trying to be fault tolerant.
		** This is a highly controversial decision, and may get
		** changed.
		** *err_code = E_DM1305_RFP_JNL_OPEN;
		** break;
		*/
		status = E_DB_OK;
		CLRDBERR(&jsx->jsx_dberr);
		continue;
	    }
	    else
	    {
		uleFormat(&jsx->jsx_dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL,
			    &error, 0);
		SETDBERR(&jsx->jsx_dberr, 0, E_DM1305_RFP_JNL_OPEN);
		break;
	    }
	}

	/* Open journal again to get correct number of blocks */
	status = dm0j_open(0,
			(char *)&dcb->dcb_jnl->physical,
			dcb->dcb_jnl->phys_length,
			&dcb->dcb_name,
			rfp->jnl_block_size, i, 0, DM0J_EXTREME,
			DM0J_M_READ, -1, DM0J_BACKWARD,
			(DB_TAB_ID *)0, &jnl2, &jsx->jsx_dberr);

	if (status == E_DB_OK)
	{
	    blk_seq = jnl2->dm0j_sequence - 1;
	    status = dm0j_close(&jnl2, &jsx->jsx_dberr);
	}
	else
	{
	    (VOID) dm0j_close(&jnl, &local_dberr);
	    jnl = 0;
	    status = E_DB_OK;
	    CLRDBERR(&jsx->jsx_dberr);
	    break;
	}

	if (rfp->rfp_status & RFP_NOROLLBACK)
	{
	    status = validate_incr_jnl(jsx, rfp, dcb, dmve, i);
	    if (status)
	    {
		(VOID) dm0j_close(&jnl, &local_dberr);
		jnl = 0;
		status = E_DB_OK;
		CLRDBERR(&jsx->jsx_dberr);
		break;
	    }
	}

	/* initialize variables */
	record_time  = 1;
	byte_time    = 1;
	record_count = 0;
	byte_count   = 0;

	TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
	    "%@ RFP: Start processing journal file %d blkseq %d \n", i, blk_seq);
	*new_jnl_cnt += 1;

	/*  Loop through the records of this journal file. */
	for ( ; ; )
	{
	    /*	Read the next record from this journal file. */

	    status = dm0j_read(jnl, (PTR *)&record, &l_record, &jsx->jsx_dberr);
	    if (status != E_DB_OK)
	    {
		if (jsx->jsx_dberr.err_code == E_DM0055_NONEXT)
		{
		    status = E_DB_OK;
		    CLRDBERR(&jsx->jsx_dberr);
		    break;
		}
		uleFormat(&jsx->jsx_dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL,
			    &error, 0);
		SETDBERR(&jsx->jsx_dberr, 0, E_DM1305_RFP_JNL_OPEN);
		if (last_rec_read_lsn.lsn_low || last_rec_read_lsn.lsn_high)
		{
		    rfp->rfp_redo_err = TRUE;
		    rfp->rfp_status |= RFP_NO_CLRLOGGING;
		    STRUCT_ASSIGN_MACRO(last_rec_read_lsn,
					    rfp->rfp_redo_end_lsn);
		    rfp->rfp_redo_end_lsn.lsn_low++;
		    if (rfp->rfp_redo_end_lsn.lsn_low == 0)
			rfp->rfp_redo_end_lsn.lsn_high += 1;
		}
		break;
	    }
	    else
		STRUCT_ASSIGN_MACRO(record->lsn, last_rec_read_lsn);

	    /*
	    ** Increase count of records processed
	    */
	    rfp->rfp_stats_blk.rfp_total_recs++;

	    /*
	    ** Perform one-time check that start lsn not less than
	    ** the lsn of the first journal record.
	    */
	    if ( (!lsn_check) && (jsx->jsx_status1 & JSX_START_LSN) )
	    {
		if (LSN_LT(&jsx->jsx_start_lsn, &record->lsn))
		{
		    char	    error_buffer[ER_MAX_LEN];
		    i4	    error_length;
		    i4	    count;

		    uleFormat(NULL, E_DM1053_JSP_INV_START_LSN,
			(CL_ERR_DESC *)NULL, ULE_LOOKUP, (DB_SQLSTATE *)NULL,
			error_buffer, sizeof(error_buffer),
			&error_length, &error, 5,
			0, jsx->jsx_start_lsn.lsn_high,
			0, jsx->jsx_start_lsn.lsn_low,
			0, record->lsn.lsn_high,
			0, record->lsn.lsn_low,
			0, i);

		    error_buffer[error_length] = EOS;
		    SIwrite(error_length, error_buffer, &count, stdout);
		    SIwrite(1, "\n", &count, stdout);
		    SIflush(stdout);

		    SETDBERR(&jsx->jsx_dberr, 0, E_DM1306_RFP_APPLY_RECORD);
		    status = E_DB_ERROR;
		    break;
		}

		lsn_check = TRUE;
	    }

	    /*
	    ** Check counters to see if we should output a message.
	    ** Note that the purpose of this code is so we can see that
	    ** processing is happening.  In that regard, we want to count
	    ** all records read, not just the ones that qualify with the
	    ** the -b, -e, -start_lsn, and -end_lsn flags.
	    */
	    if (record_interval)
	    {
		record_count++;
		if (record_count / (record_interval * record_time))
		{
		    TRformat(dmf_put_line, 0, line_buffer,
			    sizeof(line_buffer),
			    "%@ RFP: Read journal record %d.\n",
			    record_count);
		    if (jsx->jsx_status & JSX_TRACE)
		       TRdisplay("%@ RFP: Read journal record %d.\n",
			    record_count);
		    record_time++;
		}
	    }
	    if (byte_interval)
	    {
		byte_count+= l_record;
		if (byte_count / (byte_interval * byte_time))
		{
		    TRformat(dmf_put_line, 0, line_buffer,
			    sizeof(line_buffer),
			    "%@ RFP: Read %d bytes of journal file.\n",
			    byte_count);
		    if (jsx->jsx_status & JSX_TRACE)
		       TRdisplay("%@ RFP: Read %d bytes of journal file.\n",
			    byte_count);
		    byte_time++;
		}
	    }

	    /*
	    ** Qualify the record for -e, -b, -start_lsn & -end_lsn flags.
	    ** These status bits are used just below.
	    */
	    if ( (jsx->jsx_status & (JSX_BTIME | JSX_ETIME)) ||
		 (jsx->jsx_status1 & (JSX_START_LSN | JSX_END_LSN)) )
	    {
		rfp->rfp_status &= ~RFP_STRADDLE_BTIME;
		qualify_record(jsx, rfp, record);
	    }

	    /*
	    ** If we before the begin time or start lsn, skip this record.
	    */
	    if ((rfp->rfp_status & (RFP_BEFORE_BTIME | RFP_BEFORE_BLSN)) &&
		    !(rfp->rfp_status & RFP_STRADDLE_BTIME))
		continue;

	    /*
	    ** If we are past the specified -e time then halt processing.
	    **
	    ** If we are exactly at the specified -e time then process
	    ** this log record, but turn off the EXACT flag so that we
	    ** will halt processing before the next record.
	    */
	    if (rfp->rfp_status & RFP_AFTER_ETIME)
	    {
		if (rfp->rfp_status & RFP_EXACT_ETIME)
		    rfp->rfp_status &= ~RFP_EXACT_ETIME;
		else
		    break;
	    }

	    /*
	    ** If we match start or end lsn exactly, process this record,
	    ** we'll stop next time thru.
	    ** -start_lsn may equal -end_lsn, and both may
	    ** exactly match the user input.
	    */
	    if (rfp->rfp_status & RFP_EXACT_LSN)
		rfp->rfp_status &= ~RFP_EXACT_LSN;
	    else if (rfp->rfp_status & RFP_AFTER_ELSN)
		break;

	    rfp->rfp_cur_tblcb = 0;

	    if (jsx->jsx_status2 & JSX2_INVALID_TBL_LIST)
	    {
		if (( rfp_filter_log_record( rfp, jsx, record )) == 0)
		{
		    /*
		    ** Not interested so skip this record
		    */
		    continue;
		}
	    }

	    if (jsx->jsx_status & JSX_TRACE)
		dmd_log(FALSE, (PTR)record, rfp->jnl_block_size);

	    jnl_read++;
	    if (status == E_DB_OK)
	    {
		status = process_record(rfp, dmve, (PTR)record);
	    }
	    jnl_done++;

	    if (status)
	    {
		/* Check if ok to continue */
		dmfWriteMsg(&jsx->jsx_dberr, 0, 0);
		SETDBERR(&jsx->jsx_dberr, 0, E_DM1306_RFP_APPLY_RECORD);
		status = rfp_error(rfp, rfp->rfp_jsx, rfp->rfp_cur_tblcb,
			    dmve, RFP_FAILED_TABLE, status);
	    }

	    if (status == E_DB_OK)
	    {
		CLRDBERR(&jsx->jsx_dberr);
		continue;
	    }

	    /* Save lsn at time of failure */
	    rfp->rfp_redo_err = TRUE;
	    rfp->rfp_status |= RFP_NO_CLRLOGGING;
	    STRUCT_ASSIGN_MACRO(record->lsn, rfp->rfp_redo_end_lsn);
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM1306_RFP_APPLY_RECORD);
	    break;
	}

	/*  Close the current journal file. */

	if (jnl)
	{
	    (VOID) dm0j_close(&jnl, &local_dberr);
	    jnl = 0;

	    /* End of journal.  Output final counts */
	    if (record_interval)
	    {
		TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
			    "%@ RFP: Read total of %d journal records.\n",
			    record_count);
		if (jsx->jsx_status & JSX_TRACE)
		    TRdisplay("%@ RFP: Read total of %d journal records.\n",
			    record_count);
	    }
	    if (byte_interval)
	    {
		TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
		    "%@ RFP: Read total of %d bytes of journal file.\n",
		    byte_count);
		if (jsx->jsx_status & JSX_TRACE)
		    TRdisplay(
		    "%@ RFP: Read total of %d bytes of journal file.\n",
		    byte_count);
	    }
	}

	if (status != E_DB_OK)
	    return (status);

	if (incr_jnl)
	{
	    /* Update the config, Add incr jnl to jnl history */
	    if (jsx->jsx_status & JSX_TRACE)
		TRdisplay("%@ RFP: Add incremental jnl %d blk %d to jnl history\n", 
			i, blk_seq);
	    status = config_handler(jsx, OPEN_CONFIG, dcb, &cnf);
	    if (status == E_DB_OK)
	    {
		if (!cnf->cnf_jnl->jnl_history[rfp->jnl_last].ckp_f_jnl)
		    cnf->cnf_jnl->jnl_history[rfp->jnl_last].ckp_f_jnl = i;

		cnf->cnf_jnl->jnl_history[rfp->jnl_last].ckp_l_jnl = i;
		cnf->cnf_jnl->jnl_fil_seq = i;
		cnf->cnf_jnl->jnl_blk_seq = blk_seq;

		/* Update rfp_jnl_history in case -rollback specified */
		/* so rollback can start from new incremental jnl */
		rfp->rfp_jnl_history[rfp->jnl_last].ckp_f_jnl = 
			cnf->cnf_jnl->jnl_history[rfp->jnl_last].ckp_f_jnl;
		rfp->rfp_jnl_history[rfp->jnl_last].ckp_l_jnl =
			cnf->cnf_jnl->jnl_history[rfp->jnl_last].ckp_l_jnl;
		rfp->jnl_fil_seq = i;
		rfp->jnl_blk_seq = blk_seq;

		status = config_handler(jsx, UPDATE_CONFIG, dcb, &cnf);
		status = config_handler(jsx, CLOSEKEEP_CONFIG, dcb, &cnf);
	    }

	    if (status != E_DB_OK)
	    {
		dmfWriteMsg(&jsx->jsx_dberr, 0, 0);
		/* continue and try to rollback open transactions and close db */
		CLRDBERR(&jsx->jsx_dberr);
	    }

	    status = save_open_xacts(jsx, rfp, dcb, dmve, incr_jnl);
	}

	/*
	** If we are past the specified -e time or -end_lsn, then halt
	** processing.
	*/
	if ((rfp->rfp_status & (RFP_AFTER_ETIME | RFP_AFTER_ELSN)))
	    return (E_DB_OK);
    }

    if (status != E_DB_OK)
	return (status);

    /*
    ** Call the OpenJournal end routine
    */
    status = dmckp_end_journal_phase( d, &error );
    if ( status != E_DB_OK )
    {
	SETDBERR(&jsx->jsx_dberr, 0, error);
        return( status );
    }

    return (E_DB_OK);
}


/*
** Name: save_open_xacts	- Save open transaction(s) context
**
** Description:
**      This routine saves open transaction(s) context.
**      It writes DM0LBTINFO and DM0LRAW log records.
**      It is called at the end of an incremental rollforwarddb.
**
** Inputs:
**	jsx		    - JSP context
**	rfp		    - Rollforward context
**	dcb		    - Open database control block
**
** Outputs:
**	err_code	    - reason for error status
**
** Returns:
**	E_DB_OK
**	E_DB_ERROR
**
** History:
**      12-apr-2007 (stial01)
**	    Created.
*/
static DB_STATUS
save_open_xacts(
DMF_JSX		*jsx,
DMF_RFP		*rfp,
DMP_DCB		*dcb,
DMVE_CB		*dmve,
i4		last_jnl)
{
    DM0J_CTX		*btjnl = NULL;
    RFP_TX		*tx;
    char		line_buffer[132];
    i4			local_error;
    DB_STATUS		status;
    i4			btjnl_sequence;
    i4			i;
    PTR			cp_align;
    char		rawbuf[sizeof (DM0L_RAWDATA) + MAX_RAWDATA_SIZE + sizeof(ALIGN_RESTRICT)];
    PTR			cp_align2;
    DM0L_RAWDATA	*rawdata;
    i4			cur_size;
    i4			size_left;
    PTR			cur_posn;
    PTR			startrawdata;
    i4			tx_cnt = 0;
    DM0L_BTINFO		btinfo;
    i4			sequence;
    DM0L_HEADER		*record;
    i4			l_record;
    DM0L_HEADER		last_hdr;
    i4			blk_cnt = 1;
    i4			error;
    DB_ERROR		local_dberr;

    CLRDBERR(&jsx->jsx_dberr);

    if (jsx->jsx_status & JSX_TRACE)
	TRdisplay("%@ Saving incremental transaction context for jnl %d.\n",
		last_jnl);

    rawdata = (DM0L_RAWDATA *)ME_ALIGN_MACRO(rawbuf, sizeof(ALIGN_RESTRICT));

    for (;;)
    {
	/* Incremental rollforward fails if online modify spans journals */
	if (rfp->rfp_octx)
	{
	    TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
		    "Online modify cannot span incremental journals\n");
	    status = E_DB_ERROR;
	    break;
	}

	status = dm0j_delete(DM0J_RFP_INCR, 
	    (char *)&dmve->dmve_dcb_ptr->dcb_jnl->physical,
	    dmve->dmve_dcb_ptr->dcb_jnl->phys_length, last_jnl,
	    (DB_TAB_ID *)NULL, &jsx->jsx_dberr);
	if (status != E_DB_OK)
	{
	    /*
	    ** dm0j_delete only fails if the file might still be there
	    ** dm0j_delete returns OK if file not found
	    */
	    dmfWriteMsg(NULL, E_DM1110_CPP_DELETE_JNL, 0);
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM1112_CPP_JNLDMPCKP_NODEL);
	    return (E_DB_ERROR);
	}

	status = dm0j_create(DM0J_RFP_INCR, 
			(char *)&dmve->dmve_dcb_ptr->dcb_jnl->physical,
			dmve->dmve_dcb_ptr->dcb_jnl->phys_length,
			last_jnl,
			rfp->jnl_block_size,
			blk_cnt, 
			0, (DB_TAB_ID *)NULL, &jsx->jsx_dberr);


	if ( status == E_DB_OK )
	    status = dm0j_open(DM0J_RFP_INCR,
		    (char *)&dmve->dmve_dcb_ptr->dcb_jnl->physical,
		    dmve->dmve_dcb_ptr->dcb_jnl->phys_length,
		    &dmve->dmve_dcb_ptr->dcb_name,
		    rfp->jnl_block_size, last_jnl, 0, 1, DM0J_M_WRITE,
		    -1, DM0J_FORWARD, (DB_TAB_ID *)NULL, &btjnl, &jsx->jsx_dberr);

	if (status != E_DB_OK)
	    break;

	/* Write BTINFO at end of last jnl file */
	for (tx = rfp->rfp_tx; tx != NULL; tx = tx->tx_next, tx_cnt++)
	{
	    TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
		    "Save transaction context for 0x%x%x.\n",
		    tx->tx_bt.bt_header.tran_id.db_high_tran,
		    tx->tx_bt.bt_header.tran_id.db_low_tran);

	    /* check for unexpected transaction status */
	    if (tx->tx_status != 0)
	    {
		/* expect tx_status not zero only when undoing journals */
		status = E_DB_ERROR;
		break;
	    }

	    STRUCT_ASSIGN_MACRO(tx->tx_bt.bt_header, btinfo.bti_header);
	    btinfo.bti_header.length = sizeof(DM0L_BTINFO);
	    btinfo.bti_header.type = DM0LBTINFO;
	    STRUCT_ASSIGN_MACRO(tx->tx_bt, btinfo.bti_bt);
	    btinfo.bti_rawcnt = 0;

	    /* Indicate in DM0LBTINFO if this tx has RAWDATA */
	    for (i = 0; i < DM_RAWD_TYPES && status == E_DB_OK; ++i)
	    {
		if (tx->tx_rawdata[i] != NULL)
		    btinfo.bti_rawcnt++;
	    }

	    status = dm0j_write((DM0J_CTX *)btjnl, (PTR)&btinfo,
		*(i4 *)&btinfo, &jsx->jsx_dberr);

	    if (status)
		break;
	}

	if (status)
	    break;

	if (rfp->rfp_tx)
	{
	    status = dm0j_update(btjnl, &btjnl_sequence, &jsx->jsx_dberr);
	    if (status)
	    {
		TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
		    "%@ Update incremental transaction context failed %d %d\n",
		    status, jsx->jsx_dberr.err_code);
		break;
	    }
	}

	status = dm0j_close(&btjnl, &jsx->jsx_dberr);
	if (status)
	    break;
	
	TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
		"%@ Context saved for %d transaction(s).\n", tx_cnt);

	return (E_DB_OK);
    }

    /* error handling */
    dmfWriteMsg(&jsx->jsx_dberr, 0, 0);

    TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
	"%@ Save transaction context failed %d %d\n",  status, jsx->jsx_dberr.err_code);
    dmfWriteMsg(NULL, E_DM1374_RFP_INCR_TXN_SAVE, 0);

    if (btjnl)
	status = dm0j_close(&btjnl, &local_dberr);

    SETDBERR(&jsx->jsx_dberr, 0, E_DM1376_RFP_INCREMENTAL_FAIL);

    return (E_DB_ERROR);

}


/*
** Name: restore_open_xacts	- Restore open transaction(s) context
**
** Description:
**      This routine restore open transaction(s) context.
**      It processes DM0LBTINFO and DM0LRAW log records.
**      It is called at the beginning of an incremental rollforwarddb.
**
** Inputs:
**	jsx		    - JSP context
**	rfp		    - Rollforward context
**	dcb		    - Open database control block
**      last_jnl	    - last jnl file processed
**
** Outputs:
**	err_code	    - reason for error status
**
** Returns:
**	E_DB_OK
**	E_DB_ERROR
**
** History:
**      12-apr-2007 (stial01)
**	    Created.
*/
static DB_STATUS
restore_open_xacts(
DMF_JSX		*jsx,
DMF_RFP		*rfp,
DMP_DCB		*dcb,
DMVE_CB		*dmve,
i4		last_jnl)
{
    DM0J_CTX		*btjnl = NULL;
    char		line_buffer[132];
    i4			local_error;
    DB_STATUS		status;
    i4			btjnl_sequence;
    DM0L_HEADER		*record;
    i4			l_record;
    RFP_TX		*tx;
    i4			tx_cnt = 0;
    char		error_buffer[ER_MAX_LEN];
    i4			error_length;
    i4			btinfo_done = FALSE;
    i4			need_raw_cnt = 0;
    DM0L_RAWDATA	*rawdata;
    i4			error;
    DB_ERROR		local_dberr;

    CLRDBERR(&jsx->jsx_dberr);

    if (jsx->jsx_status & JSX_TRACE)
	TRdisplay("%@ Restoring incremental transaction context for jnl %d.\n",
		last_jnl);

    for (;;)
    {
	status = dm0j_open(DM0J_RFP_INCR,
		    (char *)&dmve->dmve_dcb_ptr->dcb_jnl->physical,
		    dmve->dmve_dcb_ptr->dcb_jnl->phys_length,
		    &dmve->dmve_dcb_ptr->dcb_name,
		    rfp->jnl_block_size, last_jnl, 0, DM0J_EXTREME,
		    DM0J_M_READ, -1, DM0J_BACKWARD,
		    (DB_TAB_ID *)0, &btjnl, &jsx->jsx_dberr);

	if (status != E_DB_OK)
	    break;

	for ( ; status == E_DB_OK; )
	{
	    /*	Read backwards this journal file. */
	    status = dm0j_read(btjnl, (PTR *)&record, &l_record, &jsx->jsx_dberr);
	    if (status != E_DB_OK)
	    {
		if (jsx->jsx_dberr.err_code == E_DM0055_NONEXT && need_raw_cnt == 0)
		{
		    status = E_DB_OK;
		    CLRDBERR(&jsx->jsx_dberr);
		    break;
		}
		if (need_raw_cnt)
		    SETDBERR(&jsx->jsx_dberr, 0, E_DM1323_RFP_BAD_RAWDATA);
		break;
	    }

	    if (record->type == DM0LBTINFO)
	    {
		DM0L_BTINFO	*bti = (DM0L_BTINFO *)record;

		TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
		    "Restore transaction context for 0x%x%x.\n",
		    record->tran_id.db_high_tran, record->tran_id.db_low_tran);

		status = add_tx(rfp, &bti->bti_bt); 
		tx_cnt++;

		if (bti->bti_rawcnt)
		    need_raw_cnt += bti->bti_rawcnt;
	    }
	    else if (need_raw_cnt)
	    {
		if (record->type != DM0LRAWDATA)
		    continue;

		lookup_tx(rfp, &record->tran_id, &tx);
		if (!tx)
		    continue;

		rawdata = (DM0L_RAWDATA *)record;
		status = reverse_gather_rawdata(jsx, tx, rawdata);
		if (rawdata->rawd_offset == 0)
		    need_raw_cnt--;
	    }
	    else
	    {
		break;
	    }

	    if (status)
		break;
	}

	if (status)
	    break;

	status = dm0j_close(&btjnl, &jsx->jsx_dberr);
	if (status)
	    break;

	TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
	    "%@ Context restored for %d transaction(s)\n", tx_cnt);
	
	return (E_DB_OK);
    }

    /* error handling */
    dmfWriteMsg(&jsx->jsx_dberr, 0, 0);
    TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
	"%@ Restore transaction context failed %d %d\n", status, jsx->jsx_dberr.err_code);

    dmfWriteMsg(NULL, E_DM1375_RFP_INCR_TXN_RESTORE, 0);

    if (btjnl)
	status = dm0j_close(&btjnl, &local_dberr);

    SETDBERR(&jsx->jsx_dberr, 0, E_DM1376_RFP_INCREMENTAL_FAIL);

    return (E_DB_ERROR);

}


/*{
** Name: reverse_gather_rawdata - Gather rawdata during backward jnl scan
**
** Description:
**	Some of the data needed for operations like repartitioning MODIFY
**	is of indefinite length, and doesn't fit into the MODIFY log
**	record.  So, these structures are logged as RAWDATA log records,
**	preceding the MODIFY that needs them.  Multiple RAWDATA records
**	can be used to log a very large data structure; each RAWDATA
**	has the total size as well as the offset of the current piece.
**	This routine is used to gather up RAWDATA stuff as it arrives.
**
**	Each RAWDATA is stamped with a "type", which allows a variety of
**	objects to be saved up (a partition definition and a ppchar array,
**	for instance.)  Only one of each can currently be held, although
**	there's support in the log record for multiple instances.
**	For now, if we see the start of an object of type X, and we're
**	already holding an X, we'll assume that the old X is no longer
**	needed;  it's thrown away and a new X is started.
**
**	Since there might be multiple concurrent transactions recorded
**	in the journals, each one writing RAWDATA records, the TX info
**	block is used to point to and track the raw data.
**
** Inputs:
**	tx			Transaction info block
**	record			Pointer to RAWDATA log record.
**	err_code		An output
**
** Outputs:
**	Returns E_DB_xxx status
**	err_code		Specific failure reason returned here
**
** Side effects:
**	raw data info is built up, pointed to by tx info
**
** History:
**     12-apr-2007 (stial01)
**	    Created from gather_rawdata()
*/

static DB_STATUS
reverse_gather_rawdata(DMF_JSX *jsx, RFP_TX *tx, DM0L_RAWDATA *record)
{

    DB_STATUS status;			/* The usual call status */
    i4 ix;				/* Rawdata type index */
    i4 toss_error;
    i4	error;

    CLRDBERR(&jsx->jsx_dberr);

    ix = record->rawd_type;

    /* If this is the LAST piece allocate a holding area for the raw data */
    if (record->rawd_offset == record->rawd_total_size - record->rawd_size)
    {
	status = dm0m_allocate(sizeof(DMP_MISC) + record->rawd_total_size,
			0, (i4) MISC_CB, (i4) MISC_ASCII_ID,
			(char *) tx,
			(DM_OBJECT **) &tx->tx_rawdata[ix],
			&jsx->jsx_dberr);
	if (DB_FAILURE_MACRO(status))
	    return (status);
	tx->tx_rawdata_size[ix] = record->rawd_total_size;
	tx->tx_rawdata[ix]->misc_data =
		(char *) ((PTR) tx->tx_rawdata[ix] + sizeof(DMP_MISC));
    }
    /* Copy in this piece */
    if (record->rawd_offset + record->rawd_size > tx->tx_rawdata_size[ix])
    {
	uleFormat(NULL, E_DM1323_RFP_BAD_RAWDATA, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		NULL, (i4)0, NULL, &toss_error, 4,
		0, ix, 0, record->rawd_offset, 0, record->rawd_size,
		0, tx->tx_rawdata_size[ix]);
	/* Return a parameter-less error code for caller */
	SETDBERR(&jsx->jsx_dberr, 0, E_DM1306_RFP_APPLY_RECORD);
	return (E_DB_ERROR);
    }
    MEcopy( (PTR)record + sizeof(DM0L_RAWDATA),
		record->rawd_size,
		tx->tx_rawdata[ix]->misc_data + record->rawd_offset);
    return (E_DB_OK);

} /* reverse_gather_rawdata */


/*
** Name: validate_incr_jnl - Validate incremental jnl file
**
** Description:
**      This routine verifies that the jnl file contains a DM0LJNLSWITCH record
**      A DM0LJNLSWITCH log record is written to the jnl when the archiver
**      performs a journal switch to a new journal file.
**
** Inputs:
**	jsx		    - JSP context
**	rfp		    - Rollforward context
**	dcb		    - Open database control block
**      last_jnl	    - last jnl file processed
**
** Outputs:
**	err_code	    - reason for error status
**
** Returns:
**	E_DB_OK
**	E_DB_ERROR
**
** History:
**      04-mar-2008 (stial01)
**	    Created.
*/
static DB_STATUS
validate_incr_jnl(
DMF_JSX		*jsx,
DMF_RFP		*rfp,
DMP_DCB		*dcb,
DMVE_CB		*dmve,
i4		jnl_seq)
{
    DM0J_CTX		*ljnl = NULL;
    i4			error_length;
    char		error_buffer[ER_MAX_LEN];
    i4			local_error;
    DB_STATUS		status;
    DM0L_HEADER		*record;
    i4			l_record;
    i4			jcnt = 0;
    i4			count;
    i4			error;
    DB_ERROR		local_dberr;

    CLRDBERR(&jsx->jsx_dberr);

    TRformat(dmf_put_line, 0, error_buffer, sizeof(error_buffer),
	"%@ RFP: Validating incremental journal %d \n", jnl_seq);

    for (;;)
    {
	status = dm0j_open(0,
		    (char *)&dcb->dcb_jnl->physical,
		    dcb->dcb_jnl->phys_length,
		    &dcb->dcb_name,
		    rfp->jnl_block_size, jnl_seq, 0, DM0J_EXTREME,
		    DM0J_M_READ, -1, DM0J_BACKWARD,
		    (DB_TAB_ID *)0, &ljnl, &jsx->jsx_dberr);

	if (status != E_DB_OK)
	    break;

	for ( ; status == E_DB_OK; )
	{
	    /*	Read backwards this journal file. */
	    status = dm0j_read(ljnl, (PTR *)&record, &l_record, &jsx->jsx_dberr);
	    if (status)
		break;
	
	    jcnt++;
	    if (record->type == DM0LBTINFO)
		continue;

	    if (record->type == DM0LJNLSWITCH)
		status = E_DB_OK;
	    else
		status = E_DB_ERROR;

	    break;
	}

	if (status)
	    break;

	status = dm0j_close(&ljnl, &jsx->jsx_dberr);
	if (status)
	    break;

	TRformat(dmf_put_line, 0, error_buffer, sizeof(error_buffer),
	    "%@ RFP: Incremental Journal %d is valid.\n", jnl_seq);
	return (E_DB_OK);
    }

    TRdisplay("DM0LJNLSWITCH log record not found jcnt=%d\n", jcnt);

    /* Write error to errlog */
    uleFormat(NULL, E_DM1377_RFP_INCR_JNL_WARNING, (CL_ERR_DESC *)NULL, ULE_LOG,
		NULL, error_buffer, ER_MAX_LEN, &error_length, &error, 2,
		sizeof(dcb->dcb_name), dcb->dcb_name.db_db_name, 
		0, jnl_seq);

    /* Now write it to the console */
    error_buffer[error_length] = EOS;
    SIwrite(1, "\n", &count, stdout);
    SIwrite(error_length, error_buffer, &count, stdout);
    SIwrite(1, "\n", &count, stdout);
    SIwrite(1, "\n", &count, stdout);

    if (ljnl)
	status = dm0j_close(&ljnl, &local_dberr);

    /* See if we should ignore this error */
    status = rfp_er_handler(rfp, jsx); 

    if (status)
	SETDBERR(&jsx->jsx_dberr, 0, E_DM1376_RFP_INCREMENTAL_FAIL);

    return (status);

}


/*
** Name: rfp_jnlswitch - Perform journal switch after incremental rollforwarddb 
**
** Description:
**      This routine does a journal switch.
**
** Inputs:
**	jsx		    - JSP context
**	rfp		    - Rollforward context
**      dmve
**
** Outputs:
**	err_code	    - reason for error status
**
** Returns:
**	E_DB_OK
**	E_DB_ERROR
**
** History:
**      11-mar-2008 (stial01)
**	    Created.
*/
static DB_STATUS
rfp_jnlswitch(
DMF_JSX             *jsx,
DMP_DCB		    *dcb)
{
    char		line_buffer[132];
    DB_STATUS		status = E_DB_OK;
    DB_STATUS		local_status;
    i4			db_open = FALSE;
    i4			error;

    CLRDBERR(&jsx->jsx_dberr);

    TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
	"%@ RFP: Switching to the Next Journal for Database '%~t'.\n",
	sizeof(dcb->dcb_name), &dcb->dcb_name);

    for (  ; ; )
    {
	status = dm0l_opendb(dmf_svcb->svcb_lctx_ptr->lctx_lgid,
	    DM0L_JOURNAL, &dcb->dcb_name, &dcb->dcb_db_owner, dcb->dcb_id,
	    &dcb->dcb_location.physical, dcb->dcb_location.phys_length,
	    &dcb->dcb_log_id, (LG_LSN *)0, &jsx->jsx_dberr);
	if (status)
	{
	    dmfWriteMsg(&jsx->jsx_dberr, 0, 0);
	    break;
	}

	if (jsx->jsx_status & JSX_TRACE)
	{
	    TRdisplay("%@ Database %~t (dbid %d) added to the Logging System\n",
		sizeof(DB_DB_NAME), dcb->dcb_name.db_db_name, dcb->dcb_id);
	    TRdisplay("%4* with internal id 0x%x\n", dcb->dcb_log_id);
	}

	status = dmf_jnlswitch(jsx, dcb);
	if (status)
	    dmfWriteMsg(&jsx->jsx_dberr, 0, 0);

	if (jsx->jsx_status & JSX_TRACE)
	{
	    TRdisplay("%@ Removing Database %~t (dbid %d) - Log DBID 0x%x\n",
		sizeof(DB_DB_NAME), dcb->dcb_name.db_db_name, dcb->dcb_id,
		dcb->dcb_log_id);
	}

	local_status = dm0l_closedb(dcb->dcb_log_id, &jsx->jsx_dberr);
	dcb->dcb_log_id = 0;
	if (local_status)
	{
	    dmfWriteMsg(&jsx->jsx_dberr, 0, 0);
	    status = E_DB_ERROR;
	}

	break; /* ALWAYS */
    }

    return (status);
}


/*
** Name: dmf_jnlswitch - Common code for jnl switching
**
** Description:
**      This routine does a journal switch
**
** Inputs:
**	jsx		    - JSP context
**      dcb
**
** Outputs:
**	err_code	    - reason for error status
**
** Returns:
**	E_DB_OK
**	E_DB_ERROR
**
** History:
**      11-mar-2008 (stial01)
**	    Created.
*/
static DB_STATUS
dmf_jnlswitch(
DMF_JSX	*jsx,
DMP_DCB	*dcb)
{
    DB_TRAN_ID		tran_id;
    i4			tx_id;
    DB_STATUS		status = E_DB_OK;
    STATUS		cl_status;
    CL_ERR_DESC		sys_err;
    i4	event;
    i4	event_mask;
    i4			error;

    CLRDBERR(&jsx->jsx_dberr);

    /* Start a transaction to use for LGevent waits. */

    for ( ; ; )
    {
	cl_status = LGbegin(
	    LG_NOPROTECT, dcb->dcb_log_id, &tran_id, &tx_id,
	    sizeof(dcb->dcb_db_owner), dcb->dcb_db_owner.db_own_name, 
	    (DB_DIS_TRAN_ID*)NULL, &sys_err);

	if (cl_status != OK)
	{
	    uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	    uleFormat(NULL, E_DM900C_BAD_LOG_BEGIN, &sys_err, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM1409_JNLSWITCH);
	    status = E_DB_ERROR;
	    break;
	}

	/* Signal Journal Switch Point. */
	cl_status = LGalter(LG_A_SJSWITCH,
		(PTR)&dcb->dcb_log_id, sizeof(dcb->dcb_log_id), &sys_err);

	if (cl_status != OK)
	{
	    uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	    uleFormat(NULL, E_DM900B_BAD_LOG_ALTER, &sys_err, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, &error,
		1, 0, LG_A_SJSWITCH);
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM1409_JNLSWITCH);
	    status = E_DB_ERROR;
	    break;
	}

	if (jsx->jsx_status & JSX_TRACE)
	    TRdisplay("%@ JSP: Wait for Event LG_E_ARCHIVE_DONE to Occur\n");

	event_mask = (LG_E_ARCHIVE_DONE | LG_E_START_ARCHIVER);

	cl_status = LGevent(0, tx_id, event_mask, &event, &sys_err);

	if (cl_status != OK)
	{
	    uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	    uleFormat(NULL, E_DM900F_BAD_LOG_EVENT, &sys_err, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, &error,
		1, 0, LG_E_ARCHIVE_DONE);
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM1409_JNLSWITCH);
	    status = E_DB_ERROR;
	    break;
	}
	if (event & (LG_E_START_ARCHIVER))
	{
	    uleFormat(NULL, E_DM1144_CPP_DEAD_ARCHIVER, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM1145_CPP_DEAD_ARCHIVER);
	    break;
	}

	if (jsx->jsx_status & JSX_TRACE)
	    TRdisplay("%@ JSP: LG_E_ARCHIVE_DONE occurred.\n");

	cl_status = LGend(tx_id, 0, &sys_err);
	if (cl_status != OK)
	{
	    uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	    uleFormat(NULL, E_DM900E_BAD_LOG_END, &sys_err, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM1409_JNLSWITCH);
	    break;
	}
    
	break; /* ALWAYS */ 
    }

    return (status);
}


/*
** Name: rfp_sync_last_lsn - Synchronize Last LSN, incremental journals LSNs
**
** Description:
**      After incremental rollforwarddb, make sure Last LSN in log file
**      header is > LSN's in the incremental journals.
**
** Inputs:
**	rfp				Rollforward context.
**	jsx				Pointer to Journal Support info.
**      dcb				Pointer to DCB for this database.
**
** Outputs:
**	err_code	    - reason for error status
**
** Returns:
**	E_DB_OK
**	E_DB_ERROR
**
** History:
**      27-jun-2008 (stial01)
**	    Created.
*/
static DB_STATUS
rfp_sync_last_lsn(
DMF_RFP		    *rfp,
DMF_JSX             *jsx,
DMP_DCB		    *dcb)
{
    DB_STATUS		status = E_DB_OK;
    DM0J_CTX		*ljnl = NULL;
    DM0L_HEADER		*record;
    i4			l_record;
    LG_LSN		incr_lsn;
    LG_LSN              tmseclsn;
    STATUS              cl_status;
    CL_ERR_DESC		sys_err;
    i4			length;
    LG_HEADER           header;
    char		error_buffer[ER_MAX_LEN];
    i4			error_length;
    i4			error;
    DB_ERROR		local_dberr;

    CLRDBERR(&jsx->jsx_dberr);

    TRdisplay("Sync Last LSN with incremental journals\n");

    incr_lsn.lsn_low = 0;
    incr_lsn.lsn_high = 0;

    /*
    ** If no new journals processed, must open last incremental
    ** journal to get the lsn of the last journal record
    ** (do this anyway to make sure this code works)
    */
    if (rfp->jnl_fil_seq)
    {
	status = dm0j_open(0,
		    (char *)&dcb->dcb_jnl->physical,
		    dcb->dcb_jnl->phys_length,
		    &dcb->dcb_name,
		    rfp->jnl_block_size, rfp->jnl_fil_seq, 0, DM0J_EXTREME,
		    DM0J_M_READ, -1, DM0J_BACKWARD,
		    (DB_TAB_ID *)0, &ljnl, &jsx->jsx_dberr);

	if (status != E_DB_OK)
	{
	    uleFormat(&jsx->jsx_dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL, &error, 0);
	    /* already logged */
	    CLRDBERR(&jsx->jsx_dberr);
	    dmfWriteMsg(NULL, E_DM1305_RFP_JNL_OPEN, 0);
	}
    }

    for ( ; ljnl ; )
    {
	/* Read backwards last journal file */
	status = dm0j_read(ljnl, (PTR *)&record, &l_record, &jsx->jsx_dberr);
	if (status)
	    break;
    
	if (record->lsn.lsn_high != 0)
	{
	    if (LSN_GT(&record->lsn, &incr_lsn))
		STRUCT_ASSIGN_MACRO(record->lsn, incr_lsn);
	    break;
	}
    }

    if (ljnl)
	(VOID) dm0j_close(&ljnl, &local_dberr);

    /* If we can't determine last incremental journal lsn, use TMsecs */
    if (incr_lsn.lsn_high == 0)
    {
	tmseclsn.lsn_high = TMsecs();
	tmseclsn.lsn_low = 0;
	STRUCT_ASSIGN_MACRO(tmseclsn, incr_lsn);
    }

    status = dm0l_opendb(dmf_svcb->svcb_lctx_ptr->lctx_lgid,
	DM0L_JOURNAL | DM0L_SPECIAL, &dcb->dcb_name, &dcb->dcb_db_owner, dcb->dcb_id,
	&dcb->dcb_location.physical, dcb->dcb_location.phys_length,
	&dcb->dcb_log_id, &incr_lsn, &jsx->jsx_dberr);

    if (status)
	dmfWriteMsg(&jsx->jsx_dberr, 0, 0);
    else
	status = dm0l_closedb(dcb->dcb_log_id, &jsx->jsx_dberr);

    dcb->dcb_log_id = 0;

    /* LGshow to verify Last LSN > incr_lsn */
    cl_status = LGshow(LG_A_HEADER, (PTR)&header, sizeof(header), &length, 
	&sys_err);
    if (cl_status != E_DB_OK)
    {
	uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
	    (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	uleFormat(NULL, E_DM9017_BAD_LOG_SHOW, &sys_err, ULE_LOG, NULL,
	    (char *)NULL, (i4)0, (i4 *)NULL, &error, 1,
	    0, LG_A_HEADER);
	header.lgh_last_lsn.lsn_high = 0;
	header.lgh_last_lsn.lsn_low = 0;
	status = E_DB_ERROR;
    }
    else if (LSN_GT(&incr_lsn, &header.lgh_last_lsn))
	status = E_DB_ERROR;
    else
	status = E_DB_OK;

    if (status)
    {
	i4	count;

	uleFormat(NULL, E_DM1378_RFP_INCR_LAST_LSN, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		error_buffer, sizeof(error_buffer), &error_length, &error, 5,
		sizeof(DB_DB_NAME), dcb->dcb_name.db_db_name,
		0, incr_lsn.lsn_high,
		0, incr_lsn.lsn_low,
		0, header.lgh_last_lsn.lsn_high,
		0, header.lgh_last_lsn.lsn_low);
	error_buffer[error_length] = EOS;
	SIwrite(1, "\n", &count, stdout);
	SIwrite(error_length, error_buffer, &count, stdout);
	SIwrite(1, "\n", &count, stdout);
	SIflush(stdout);
    }

    return (status);
}
