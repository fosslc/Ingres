/*
** Copyright (c) 1986, 2008 Ingres Corporation
**
NO_OPTIM = dr6_us5
*/

#include    <compat.h>
#include    <gl.h>
#include    <pc.h>
#include    <st.h>
#include    <me.h>
#include    <cs.h>
#include    <jf.h>
#include    <ck.h>
#include    <cv.h>
#include    <di.h>
#include    <id.h>
#include    <lo.h>
#include    <nm.h>
#include    <si.h>
#include    <tm.h>
#include    <tr.h>
#include    <er.h>
#include    <sl.h>
#include    <bt.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <lk.h>
#include    <cx.h>
#include    <lg.h>
#include    <csp.h>
#include    <lgdef.h>
#include    <lgkdef.h>
#include    <duf.h>
#include    <dudbms.h>
#include    <ulf.h>
#include    <adf.h>
#include    <dmf.h>
#include    <dm.h>
#include    <dmp.h>
#include    <dm2d.h>
#include    <dmtcb.h>
#include    <dm2t.h>
#include    <dm2r.h>
#include    <dm0c.h>
#include    <dm0j.h>
#include    <sxf.h>
#include    <dma.h>
#include    <dm0d.h>
#include    <dmfjsp.h>
#include    <dmxe.h>
#include    <dmpp.h>
#include    <dm1b.h>
#include    <dm0l.h>
#include    <dm0m.h>
#include    <dm0p.h>
#include    <dm0s.h>
#include    <dmfrcp.h>
#include    <dmftrace.h>
#include    <dmckp.h>
#include    <dmfcpp.h>
#include    <dm0pbmcb.h>
#include    <dm0llctx.h>
#include    <dmccb.h>
#include    <dmfcsp.h>

/**
**
**  Name: DMFCPP.C - DMF Check Point Processor.
**
**  Description:
**	Function prototypes are defined in DMFJSP.H.
**
**      This file contains the routines that implement the
**	DMF checkpoint processor.
**
**          dmfcpp - Check point processor.
**
**	  Internal routines:
**
**	    check_point	- Write the actual check point.
**	    update_catalogs - Change journal state of catalogs.
**	    delete_files - Delete any unnecessary files.
**	    cluster_merge - Merge old journal files for cluster installation.
**
**  History:
**      30-nov-86 (derek)
**	    Created new for Jupiter.
**	30-sep-88 (rogerk)
**	    Changed dmxe_begin call to use new arguments.
**	24-oct-88 (greg)
**	    LOingpath() return status must be checked.  LOingpath
**	    should only fail if the installation is hosed but we
**	    don't want to AV anyway.
**	16-dec-1988 (rogerk)
**	    Fix Unix compile warnings.
**	21-jan-1989 (EdHsu)
**	    Modify code for online backup feature support.
**	08-aug-1989 (greg)
**	    Fix up dump location error messages.  Use LO_{DMP, DATA, CKP, JNL}
**	19-sep-1989 (walt)
**	    Fixed problem where the lock key for LK_CKP_CLUSTER
**	    was incompletely initialized.  Changed MEcopy to move 16 bytes
**	    rather than only 12.  The uninitialized bytes caused the lock
**	    name to nolonger match the "same" name taken out on another node.
**	09-oct-1989 (walt)
**	    Fixed problem where the lock key for LK_CKP_CLUSTER
**	    was incompletely initialized.  Changed MEcopy to move 16 bytes
**	    rather than only 12.  The uninitialized bytes caused the lock
**	    name to nolonger match the "same" name taken out on another node.
**	13-oct-1989 (sandyh)
**	    fixed history array handling for jnl & dump info when array
**	    reaches CKP_HISTORY # of entries. This used to cause the area
**	    to extend into the node table and beyond which eventually
**	    made the config file unusable and rendered the database
**	    inaccessible. bug 8352.
**	16-oct-1989 (rogerk)
**	    Store correct checkpoint sequence number in the online dump
**	    structure and history. Use the journal checkpoint sequence
**	    number rather than keeping an independent count.
**	16-oct-1989 (rogerk)
**	    Create dump directory even if not online checkpoint.  The config
**	    file is copied to the dump directory by the archiver if the
**	    database is journaled, regardless of whether it was last
**	    checkpointed in an online fashion.
**	23-oct-1989 (rogerk)
**	    When build LK_CKP_DB lock, only copy 12 bytes of dbname to lock
**	    key as that is all that will fit.
**	23-oct-1989 (rogerk)
**	    When running online backup, don't call delete_files to purge all
**	    temporary tables/files from database directories.  Created new
**	    routine - delete_alias_files - which can be used for online backup
**	    which only deletes checkpoint alias files.
**	28-nov-1989 (mikem)
**	    Close database when there is a failure of online checkpoint, due
**	    to there already being another online checkpoint of the same db in
**	    progress.  If the db is not closed then the LG rundown code will
**	    mistakenly cleanup up the running online checkpoint, rather than
**	    the failed one.
**	 1-dec-1989 (rogerk)
**	    If checkpoint stall flag is on, then stall for one minute before
**	    and after the backup phase.  This allows online checkpoint tests
**	    to stress the system more.
**	 5-dec-1989 (rogerk)
**	    At start of online backup (while DB is stalled), copy the config
**	    file to the DUMP directory.  This gives a consistent copy of the
**	    config file that can be used by rollforward.  This prevents us
**	    from having to back out any updates made to the config file during
**	    the backup.
**	08-dec-1989 (mikem)
**	    Force "online" ckpdb to fail if the archiver dies anytime before
**	    the LG_E_FBACKUP event has been recieved.  This means we don't have
**	    to deal with any weird synchronization problems with a new archiver
**	    coming up after the previous one has failed and getting into some
**	    sort of state where the ckpdb "hangs".  This is done by forcing the
**	    ckp to wait on the event and also the LG_START_ARCHIVER
**	    event which is signaled when an archiver dies.
**	13-dec-1989 (rogerk)
**	    Put back in the dm0l_drain call to wait for the database purge to
**	    complete.  This was commented out during online-checkpoint
**	    development.
**	15-Dec-1989 (ac)
**	    Fixed the cluster online checkpoint haning problem : CKPDB is 
**	    holding CKP_CLUSTER lock and waiting for an ongoing transaction 
**	    to finish but the transaction is waiting for the CKP_CLUSTER lock.
**	26-dec-1989 (greg)
**	    Split path and filename in TRformat messages so Unix "looks right".
**	02-jan-1990 (mikem)
**	    Added another event to wait on during online checkpoint 
**	    (LG_E_BACKUP_FAILURE).  This event is signaled if the ACP encounters
**	    an error while processing the database we are online checkpointing.
**	    This event is per database rather than system wide.
**	10-jan-1990 (rogerk)
**	    Added Online Backup fixes.  Changed some of the Online Backup
**	    algorithms.  Added the cluster_merge routine.
**
**	    Changed algorithms for how to get correct journal records written
**	    to the old/new journal files.  If Online Checkpoint of a journaled
**	    database, then don't create the new journal history entry or
**	    increment the journal file sequence number here - it will be done
**	    in the archiver when the SBACKUP record is encountered.  Signal
**	    a CP and a PURGE event after writing the SBACKUP record to ensure
**	    that the archiver will purge the database up to the SBACKUP record
**	    during the checkpoint.  Do not complete checkpoint processing
**	    until the PURGE has compeleted.  This way we know that the old
**	    journal records have been flushed to the old journal files.
**
**	    If running in a cluster installation, we have to signal a CP
**	    and a PURGE before writing the SBACKUP record in order to merge
**	    the old journal files.  After the PURGE has completed, we can
**	    do the journal merge.  Added the cluster_merge routine to do this.
**
**	    Change initial setting of the jnl_fil_seq value in the config
**	    file.  It is initialized to 1 - which is the next journal file to
**	    use.  When a checkpoint is executed, the jnl_fil_seq will be
**	    incremented (in dmfcpp if offline or non-journaled datbase - in
**	    archiver if online and journaled) if the current journal file #
**	    was used by the previous checkpoint.  If it wasn't (database was
**	    not journaled) then we don't increment the value - we just use
**	    the current jnl file sequence.
**
**	    Changed code to delete/truncate old dump files to work even if
**	    checkpoint is an offline checkpoint - we can still delete/truncate
**	    files for old online checkpoints.  Fixed bug where we referenced
**	    out-of-bounds entries in the jnl history if this is the first
**	    checkpoint on the database.
**	11-jan-1990 (rogerk)
**	    Change Cluster Online Checkpoint to not run if other nodes on the
**	    cluster have the database open.  This is because we currently
**	    have no method to force the other nodes to flush journal updates
**	    from the log file to the journal files before executing the
**	    checkpoint.
**
**	    We can allow non-journaled databases to be open by other nodes as
**	    there is no journal work to do.
**
**	    We can allow other nodes to open the database after the checkpoint
**	    has started as long as they don't execute any updates.  But they
**	    must completely close the db in order to flush journal updates
**	    before we can start the checkpoint.
**	15-jan-1990 (rogerk)
**	    Fix trace message in cluster-busy case.
**	    Added log address parameter to dm0l_sbackup/dm0l_ebackup calls so
**	    that these can be output in trace information.
**	18-jan-1990 (rogerk)
**	    Zero out the last-dumped address in the config file when we start
**	    an online backup.
**	23-apr-1990 (walt)
**	    Copy the config file to the dump area for all checkpoints, not
**	    just Online Backup.  Then rollforward can use this copy of the
**	    config file rather than being forced to initially restore the
**	    default location checkpoint to find out about the locations.  If
**	    rollforward doesn't need to restore the default location first,
**	    the checkpoints can be restored "concurrently".
**	14-may-1990 (rogerk)
**	    Fixed up some error handling problems in dmfcpp, create_ckpjnldmp.
**	    Changed STRUCT_ASSIGN_MACRO's of different typed objects in
**	    create_ckpjnldmp to MEcopy's.
**	21-may-90 (bryanp)
**	    Added DM0C_COPY flag to UPDATE_CONFIG code in config_handler to
**	    keep an up-to-date backup of the config file for recovery.
**	    Set DSC_DUMP_DIR_EXISTS in the config file after creating the
**	    dump directory.
**	24-sep-1990 (rogerk)
**	    Merged 6.3 changes into 6.5.
**	31-oct-1990 (bryanp)
**	    Bug #34016:
**	    Now that we make a config file dump for offline as well as online
**	    checkpoints, config file dump deletion must be performed as part of
**	    checkpoint deletion, not as part of dump file deletion, otherwise
**	    we erroneously don't delete the config file dumps for offline
**	    checkpoints. Changed file_maint appropriately. 
**	20-nov-1990 (bryanp)
**	    When clearing the cluster journal file history, clear cnode_la as
**	    well. This helps to prevent bugs having to do with misbehaved
**	    log file addresses.
**	19-nov-1990 (bryanp)
**	    Don't close config file twice if check_point() fails.
**	28-nov-1990 (rachael)
**	    Bug 34069:
**	    The relstat of every table was being set (in update_catalogs) 
**   	    to ~TCB_JOURNAL rather than ~TCB_JOURNAL and TCB_JON when 
**	    using the -j flag. 
**	25-feb-1991 (rogerk)
**	    Turn off disable_journal status if journaling state has been 
**	    toggled (by +j or -j).  This was done as part of the Archiver 
**	    Stability project changes.
**	25-mar-1991 (bryanp)
**	    Initialize dcb_type to DCB_CB so that xDEBUG checks on the control
**	    blocks don't reject it as invalid.
**	22-apr-1991 (rogerk)
**	    Disallow online checkpoint while database is open in Set Nologging
**	    state.  Online checkpoint will not be valid while transactions
**	    are running with logging disabled.
**	 3-may-1991 (rogerk)
**	    Backout change to clear the cnode_la field in the cluster journal
**	    history after a checkpoint is performed.  Clearing this loses
**	    the knowledge of how far this node's log file has been archived.
**      15-oct-1991 (mikem)
**          Eliminated 2 sun compiler warnings "warning: statement not reached",
**	    altering the way 2 loops were coded.
**	19-may-1992 (ananth)
**	    Increasing IIrelation tuple size project.
**	    Remove references to the following obsoleted fields in the
**	    following structures. cnf->cnf_rel, cnf->cnf_att, 
**	    cnf->cnf_relx, cnf->cnf_idx. 
**	7-july-1992 (ananth)
**	    Prototyping DMF.
**	4-aug-1992 (bryanp)
**	    Prototyping LG and LK.
**	19-aug-92 (ananth)
**	    Rewrite backup to simplify and modularize the online backup code. 
**	    No functional change.
**	04-nov-92 (jrb)
**	    Renamed DCB_SORT to DCB_WORK.
**	14-dec-1992 (bryanp)
**	    Fix argument mismatch problems in config_handler calls.
**	14-dec-1992 (rogerk)
**	    Include dm1b.h before dm0l.h for prototype definitions.
**      11-jan-1993 (mikem)
**          Lint directed changes.  Added a return(E_DB_ERR) to bottom of
**          routine as a bit of defensive programing, the code should never
**          get there; but if it ever did previously a random return would
**          result.
**	04-feb-93 (jnash)
**	    - In dmfcpp(), append to end of ckpdb.dbg file rather than 
**	      create new one each time (verbose option only).
**	    - In online_signal_cp(), fix problem where LGevent 
**	      2PURGE_COMPLETE was not included in event list.
**	    - In online_setup(), fix problem where error, not warning, 
**	      returned when database is in use by other member of the 
**	      cluster.
**	    - In online_setup(), change order of LGevent(LG_E_SBACKUP) 
**	      and LKrequest(DBU) lock.
**	    - In online_write_ebackup(), include a wait for ECP event.
**	15-mar-1993 (rogerk)
**	    Reduced Logging - Phase IV:
**	    Removed dmxe_abort output parameter used to return the log address
**		of the abortsave log record.
**	    Added direction parameter to dm0j_open routine.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**		Altered dm0j_merge calling sequence in order to allow auditdb
**		    to call dm0j_merge.
**		Cast some parameters to quiet compiler complaints following
**		    prototyping of LK, CK, JF, SR.
**		Fix argument passing to dmxe_abort so that DCB can be passed.
**	24-may-1993 (rogerk)
**	    Temporary fix for journaling of system catalog updates.  Need to
**	    set the rcb_usertab_jnl flag in update_catalogs so the system
**	    catalog updates will be journaled.
**	21-jun-1993 (rogerk)
**	    Fix prototype warnings.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (bryanp)
**	    Removed an extra (duplicated) MEcopy call from online_setup().
**      26-jul-1993 (rogerk)
**	    Changed the way journal and dump windows are handled.  They are
**	    now tracked by real Log Addresses rather than the Consistency
**	    points that bound them.
**	20-sep-1993 (andys)
**	    Fix arguments to ule_format call.
**      20-sep-1993 (jnash)
**	    ALTERDB -delete_oldest_ckp.
**	     - Move file_maint() to dmfjsp.c, rename to jsp_file_maint().
**	     - Create truncate_jnl_dmp() from bit of file_maint() that
**	       is not needed by alterdb.  Code restructured accordingly.
**	    Fix up messy TRformat of checkpoint information.
**	1-oct-93 (robf)
**	    Replace dmxe calls by wrappers
**	22-nov-1993 (jnash)
**	    B56095.  Update jnl_first_jnl_seq after successful checkpoint 
**	    of a journaled database.  This is used to catch operator errors 
**	    performing rolldb #c from a point where the database was not
**	    journaled.
**	31-jan-1994 (jnash)
**	  - Release config file lock prior to signaling E_BACKUP.
**	    This was causing online checkpoints to hang when the lock was
**	    needed (for example by dm2u_get_tabid) in order to complete an 
**	    open transaction.
**	  - Fix jnl_first_jnl_seq maintance problems.
**	18-feb-94 (stephenb)
**	    Remove failure cases in calls to dma_write_audit() we audit these
**	    elsewhere.
**	9-mar-94 (stephenb)
**	    Update dma_write_audit() calls to current prototype.
**      14-mar-94 (stephenb)
**          Add new parameter to dma_write_audit() to match current prototype.
**	18-apr-1994 (jnash)
**	    B58026.  JFopen() in truncate_jnl_dump() was failing after
**	    the journal file block size had been changed with alterdb 
**	    -jnl_block_size, because the block size stored in the config 
**	    file didn't match the block size in the journal file header.  
**	    This happened only after an alterdb -disable_journaling 
**	    (ckpdb -j cleans up old journals).  The change has us not
**	    call truncate_jnl_dump() if ckpdb +j performed.
**	21-apr-1994 (rogerk)
**	    Changed dm2r_get calls to explicitly request DM2R_GETNEXT mode
**	    rather than implicitly expect it by passing a zero get mode.
**	20-may-1994 (rmuth)
**	    Bug 63533 - Display information about the stall phase of online
**	    checkpoint.
**	20-jun-1994 (jnash)
**	    Add DM1318 to simulate CKsave error.
**	13-sep-1994 (wolf) 
**	    #include dmftrace.h to resolve DMZ_ASY_MACRO reference
**      31-Nov-1994 (liibi01)
**          Bug 59771
**          Cross integration from 6.4. The timestamp held in the checkpoint
**          History for Journal section of the configuration file is incorrect
**          for online checkpoints. Changed method of timestamping the check-
**          point journal history - we now use the time the Sbackup record was
**          written for online checkpoints.
**      07-jan-1995 (alison)
**          Partial Backup/Recovery Project:
**          changed CK..() calls to dmckp_..() calls.
**      07-Jan-1995 (nanpr01)
**	    Bug #66028
**          reset the bit TABLE_RECOVERY_DISALLOWED after a successful 
**	    checkpoint. 
**	29-jan-1995 (rudtr01)
**	    Fixed bug 66571 by modifying cpp_prepare_save_tbl() to 
**	    separate the filenames by a comma instead of a blank.
**      18-Jan-1995 (nanpr01)
**	    Bug #66439
**          reset the bit TABLE_RECOVERY_DISALLOWED for only those tables 
**	    in the partial checkpoint list. 
**	29-jan-1995 (rudtr01)
**	    Fixed bug 66571 by modifying cpp_prepare_save_tbl() to 
**	    separate the filenames by a comma instead of a blank.
**	31-jan-1995 (shero03)
**	    Bug #66545
**	    make sure the TABLE_RECOVERY_DISALLOWED bit is reset
**	    even if the journaling is not being changed.
**      24-jan-1995 (stial01)
**          BUG 66473: online_write_sbackup() call dm0l_sbackup with  
**          flags = DM0L_TBL_CKPT for table checkpoint.
**          ckp_mode in config is now bit pattern.
**          BUG 66482,66519: if offline table checkpoint and just locking the
**          database, we need to open database to get table info.
**          moved call to cpp_prepare_save_tbl() from check_point() up inside
**          dmfcpp().
**          BUG 66403: cpp_prepare_tbl_ckpt() apply proper case to table names
**      06-feb-1995 (stial01)
**          dmfcpp() init cnf = 0, in case user error causes us to break.
**          cpp_ckpt_invalid_tbl() distinguish views/gateways from core tables.
**          Change cpp_display_... routines as rfp_display routines were 
**          changed.
**      07-Feb-1995 (liibi01)
**          Bug 48500, if check point sequence number is greater than 9999,
**          we loop back to 1.
**	22-feb-1995 (shero03)
**	    Bug #66987
**	    Support an index into jsp_file_maint
**      08-mar-1995 (stial01)
**          Bug 67327
**          Automatically checkpoint secondary indexes and blobs for table.
**          For now, -nosecondary_index and -noblobs is not allowed for
**          table checkpoint. Since rollforward doesn't know what tables  
**          are in a particular checkpoint, it is best to let it assume
**          that the indexes and blobs are always there.
**      23-mar-1995 (stial01)
**          cpp_add_tblcb() Don't fix prev pointer of htotalq unless there
**          is an element on the htotalq. Otherwise we trash 
**          hcb->tblhcb_hash_array[3].
**          cpp_prepare_save_tbl() fixed calculation of offset of next
**          filename in the cpp_fnames_at_loc buffer.
**      20-Apr-1995 (nick)
**          Added a lock of type LK_CKP_TXN for use in detecting LG/LK deadlock
**          in online checkpoints. #67888  
**	28-Apr-1995 (smiba01)
**	    Added dr6_ues to NO_OPTIM MING directives.
**      20-may-95 (hanch04)
**          Added include dmtcb.h for ndmfapi
** 	01-jun-1995 (wonst02) 
**	    Bug 68886, 68866
** 	    Added cpp_save_chkpt_tbl_list() to save the list of tables 
**	    checkpointed.
**	12-jun-1995 (thaju02)
**	    Bug #67327: Modified cpp_save_chkpt_tbl_list, such that indexes
**	    and blobs which are associated to the table(s) that are specified
**	    for table level checkpoint are added to the table level
**	    checkpoint list file.  Note: if -nosecondary_index and/or
**	    -noblobs options are added to table level checkpoint another
**	    check of CPP_CKPT_TABLE MUST be added to cpp_save_chkpt_tbl_list.
**	29-jun-1995 (thaju02)
**	    Modified internal routine: online_setup, due to addition
**	    of new ckpdb parameter '-max_stall_time=xx'.
**      13-july-1995 (thaju02)
**          Added call to LKcreate_list in online_setup, in order to
**          create new lock list without LK_NOINTERRUPT, for '-timeout'
**          argument. '-max_stall_time' changed to '-timeout'.
**      11-sep-1995 (thaju02)
**	    Database level checkpoint to create a file list of all
**	    tables in database at time of checkpoint.
**	18-oct-1995 (thaju02)
**	    Bug #71996 - was unable to ckpdb +j repeatedly.
**      19-oct-1995 (stial01)
**          cpp_allocate_tblcb() relation tuple buffer allocated incorrectly.
**	27-Nov-1995 (mckba01)
**	    BUG 71090.  Modify routine check_point(), so that alias
**	    locations are not automatically skipped when taking a
**	    checkpoint of selected tables (-table=<tabname>
**	    command line option.)
**	09-dev-95 (hanch04)
**	    Fix comment.
**	 6-feb-1996 (nick)
**	    Rewrite cpp_save_chkpt_tbl_list so that it uses the new suite
**	    of functions to manipulate the table list file.  This has the 
**	    primary advantage of making 'ckpdb' work again.
**	13-feb-1996 (canor01)
**	    Disallow online checkpoint if online checkpoint is disabled.
**	19-mar-96 (nick)
**	    Move LK_CKP_TXN to close small timing window.
**	28-may-96 (nick)
**	    Enforce purge of journals when checkpointing a journal disabled
**	    database.
**	13-jun-96 (nick)
**	    Add prototypes for functions which were being implicity declared ;
**	    picked up by the HP compiler.
**	17-jun-96 (nick)
**	    Rewrite update_catalogs().  Tidy some code and add additional
**	    error handling to ensure some locks aren't left orphaned. 
**	    #76958 and #77307
**	 4-jul-96 (nick)
**	    Ensure we lock iirelation with shared mode rather than exclusive.
**	10-jul-96 (nick)
**	    More error handling fixes.
**	24-oct-96 (nick)
**	    LINT stuff.
**	15-oct-96 (mcgem01)
**	    E_DM1115_CPP_II_CHECKPOINT, E_DM1117_CPP_II_DATABASE and 
**	    E_DM1116_CPP_II_JOURNAL now take SystemProductName as a parameter.
**	26-Oct-1996 (jenjo02)
**	    Added semaphore name to dm0s_minit() calls.
**      06-nov-96 (hanch04)
**          Added dm0p_alloc_pgarray for partial table with VPS
**	13-Dec-1996 (jenjo02)
**	    Removed LK_CKP_TXN lock. Logging will now suspend newly-starting
**	    update transactions if online checkpoint is pending, and waits until all
**	    current update protected transactions in the database are complete
**	    before starting the checkpoint.
**	16-jun-1997 (hanch04)
**	    Bug 81939, allocate block lm and with zero fill
**	    locm_r_allow is a byte flag and must start off set to zero.
**      15-aug-97 (stephenb)
**          add DML_SCB parameter to dm2t_open() call.
**	25-Aug-1997 (jenjo02)
**	    Added log_id parm to dm2t_unfix_tcb() calls.
**	13-Nov-1997 (jenjo02)
**	    Added -s option. If indicated, newly starting online 
**	    read-write log txns will be stalled while ckpdb is stalled.
**      08-Dec-1997 (hanch04)
**          Use new la_block field when block number is needed instead
**          of calculating it from the offset
**          Keep track of block number and offset into the current block.
**          for support of logs > 2 gig
**	12-jan-1998 (popri01)
**	    X-integ john saroka's 1.2/01 change #430380 in dmfcpp routine
**	    to solve a problem that has re-appeared under 2.0:
**	    ckpdb fails after journaling has been disabled.
**	23-jun-1998(angusm)
**	    bug 82736: change interface to dmckp_init()
**      22-Feb-1999 (bonro01)
**          Replace references to svcb_log_id with
**          dmf_svcb->svcb_lctx_ptr->lctx_lgid
**	15-Mar-1999 (jenjo02)
**	    Rewrote LK_CKP_TXN scheme, again.
**	30-mar-1999 (hanch04)
**	    Added support for raw locations
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
**	10-may-1999 (walro03)
**	    Remove obsolete version string dr6_ues, dr6_uv1.
**	10-nov-1999 (somsa01)
**	    Changed PCis_alive to CK_is_alive.
**	30-Nov-1999 (jenjo02)
**	    Added DB_DIS_TRAN_ID parm to LKcreate_list() prototype.
**	15-Dec-1999 (jenjo02)
**	    Added DB_DIS_TRAN_ID parm to LGbegin() prototype.
**	    Removed DB_DIS_TRAN_ID parm from LKcreate_list() prototype.
**      28-feb-2000 (gupsh01)
**          Enabled check for owner along with the table name
**          so that the table names in -table option can also
**          be specified as owner.tablename .
**      14-jul-2000 (stial01)
**         update_catalogs() rcb_usertab_jnl initialization for b102072.
**	18-Jan-2001 (jenjo02)
**	    Subvert calls to dma_write_audit() if not C2SECURE.
**	25-Mar-2001 (jenjo02)
**	    Productized RAW locations.
**	30-may-2001 (devjo01)
**	    Replace LGcluster() with CXcluster_enabled. s103715.
**	    Add dm0p.h to provide prototype for dm0p_alloc_pgarray.
**      28-Jun-2001 (hweho01)
**          Added the missing argument num_parms to ule_format() call
**          in update_catalogs() routine.
**	14-June-2001 (bolke01)
**	    Added new function cpp_wait_for_purge - created from online_wait_for_purge
**	    Added function trace_lgshow to allow checking of the Archiver's outstanding
**	    work.  Check if dmfrcp is alive and in proces of closing database.
**	    (104984)
**	26-May-2002 (hanje04)
**	    Replaced longnats brought in by X-Integ with i4's.
**	14-aug-2003 (jenjo02)
**	    dm2t_unfix_tcb() "tcb" parm now pointer-to-pointer,
**	    obliterated by unfix to prevent multiple unfixes
**	    by the same transaction and corruption of 
**	    tcb_ref_count.
**	04-sep-2003 (abbjo03)
**	    Remove a few leftover nats hidden by xDEBUG.
**      18-feb-2004 (stial01)
**          Fixed incorrect casting of length arguments.
**	09-Sep-2004 (hanje04)
**	    BUG 112987
**	    If I_DM116D is returned by dmckp_init() the error message printed
**	    out should state that the template file pointed to by II_CKTMPL_FILE
**	    is not owned by the installation owner which may now not be ingres.
**	30-Sep-2004 (schka24)
**	    Fix a nervous-making compiler warning.
**      01-sep-2004 (stial01)
**	    Support cluster online checkpoint
**      30-sep-2004 (stial01)
**          online cluster checkpoint: signal consistency point on remote node.
**          Fix non-cluster build related problem.
**      29-Oct-2004 (stial01)
**         Restore previous stall message to eliminate sep test diffs.
**      03-dec-2004 (stial01)
**          online cluster checkpoint: signal archive cycle and wait for 
**          purge to complete on each node, not just the local node.
**      07-dec-2004 (stial01)
**          LK_CKP_TXN: key change to include db_id
**          LK_CKP_TXN: key change remove node id, LKrequest LK_LOCAL instead
**      22-Dec-2004 (garni05) Bug #113524 Prob#INGSRV3057
**          modified 'if(jsx->jsx_status & JSX_JON)' condition so that after
**          'ckpdb +j dbname', is_journalled value in iitables should switch
**          from'C' to 'Y' for a journalled table and from 'N' to 'Y' for
**          foreign/Index keys.Two sub if conditions having similar code have
**          been merged into one.
**	07-Jan-2005 (hanje04)
**	    BUG 112987
**	    LOgetowner is not defined on Windows so wrap code checking 
**	    ownership of cktmpl.def in CK_OWNNOTING as it is in dmckp.c
**	29-Mar-2005 (hanje04)
**          BUG 112987
**          Checkpoint template should be owned by EUID of server as
**          ownership of ckptmpl.def can be "changed" with missuse of II_CONFIG
**	15-Apr-2005 (hanje04)
**	    BUG 112987
**	    Use IDename instead of IDgeteuser to get effective user of process.
**      01-apr-2005 (stial01)
**         upd1_jnl_dmp() if cluster create new jnl history entry in config
**         Also, more online cluster checkpoint error handling
**      07-apr-2005 (stial01)
**          Fix queue handling
**	22-Apr-2005 (fanch01)
**	   Reorder csp.h include to follow lg.h for LG_LSN type.
**	25-may-2005 (devjo01)
**	   Changed references to CSP message fields to reflect csp.h changes.
**      06-jul-2005 (stial01)
**         update_catalogs() make sure db is opened (and added to logging 
**         system) with the correct journaling status (b114744)
**      07-jul-2005 (stial01)
**         update_catalogs() fixed initialization of rcb_journal (B114771)
**      08-aug-2005 (stial01)
**         cpp_wait_for_purge() Start a transaction for LGevent waits (b115008)
**      11-aug-2005 (stial01)
**         Add dcb_status to CKP_CSP_INIT msg so that remote node adds the
**         db to logging system with correct journaling status (b115034)
**      26-Oct-2005 (hanal04) Bug 115455 INGSRV3473
**          Backout the above change. It incorrectly turns journalling on for 
**          all tables. It also does not make a difference to the results for
**          the script for bug 113524.
**      19-oct-06 (kibro01) bug 116436
**          Change jsp_file_maint parameter to be sequence number rather than
**          index into the jnl file array, and add a parameter to specify
**          whether to fully delete (partial deletion leaves journal and
**          checkpoint's logical existence)
**      03-nov-2006 (horda03) Bug 116231
**          Merge journals if the DB is marked DSC_DISABLE_JOURNAL.
**	19-mar-07 (kibro01) b117830
**	    Pass the lock list pointer into online_setup and online_resume so
**	    that when ckpdb -timeout is being used, and a different lock list
**	    is generated, the change can be reflected elsewhere - otherwise
**	    errors E_DM1131 and E_DM110B are raised when using ckpdb -timeout.
**      23-oct-2007 (stial01)
**          Support table level CPP/RFP for partitioned tables (b119315)
**	11-Apr-2008 (kschendel)
**	    dm2r position call updated, fix here.
**	30-May-2008 (jonj)
**	    LSNs are 2 u_i4's; display them in hex rather than
**	    signed decimal.
**	11-Nov-2008 (jonj)
**	    SIR 120874 Change function prototypes to pass DB_ERROR *dberr
**	    instead of i4 *err_code, use new form uleFormat.
**	18-Nov-2008 (jonj)
**	    dm0d_?, dm0j_?, dm0c_?, dm2d_? functions converted to DB_ERROR *
**	21-Nov-2008 (jonj)
**	    SIR 120874: dm0m_? functions converted to DB_ERROR *
**	25-Nov-2008 (jonj)
**	    SIR 120874: dm0l_? functions converted to DB_ERROR *
**	26-Nov-2008 (jonj)
**	    SIR 120874: dm2t_?, dm2r_? functions converted to DB_ERROR *
**	05-Dec-2008 (jonj)
**	    SIR 120874: dma_?, dm0p_? functions converted to DB_ERROR *
**	21-Apr-2009 (drivi01)
**	    Cleanup warnings in effort to port to Visual Studio 2008 
**	    compiler.
*/

/*
**  External variables
*/

/*
**  Static functions.
*/

static	DB_STATUS   online_setup(
    DMF_JSX	*jsx,
    LK_LLID     *ckp_lock_list);

static	DB_STATUS   online_signal_cp(
    DMF_JSX	    *jsx);

static	DB_STATUS   online_wait_for_purge(
    DMF_JSX	    *jsx);

static	DB_STATUS   online_write_sbackup(
    DMF_JSX	    *jsx);

static	DB_STATUS   online_resume(
    DMF_JSX	    *jsx,
    LK_LLID     *ckp_lock_list);

static	DB_STATUS   online_write_ebackup(
    DMF_JSX	    *jsx);

static	DB_STATUS   online_dbackup(
    DMF_JSX	    *jsx);

static	DB_STATUS   online_finish(
    DMF_JSX	    *jsx,
    DMP_DCB	    *dcb);

static	DB_STATUS   check_point(
    DMF_CPP     *cpp,
    DMF_JSX	*jsx,
    DMP_DCB	*dcb,
    DM0C_CNF    **config,
    i4	ckp_sequence);

static DB_STATUS    upd1_jnl_dmp(
    DMF_JSX	*journal_context,
    DMP_DCB	*dcb,
    DM0C_CNF    **config);

static DB_STATUS    upd2_jnl_dmp(
    DMF_JSX	*journal_context,
    DMP_DCB	*dcb,
    DM0C_CNF    **config,
    DB_TAB_TIMESTAMP  *sb_time);

static DB_STATUS    update_catalogs(
    DMF_JSX         *jsx,
    DMP_DCB	    *dcb,
    DM0C_CNF	    **config,
    i4	 	    *abort_xact_lgid);

static DB_STATUS    delete_files(
    DMP_LOC_ENTRY   *location,
    char	    *filename,
    i4	    l_filename,
    CL_ERR_DESC	    *sys_err);

static DB_STATUS delete_alias_files(
    DMP_LOC_ENTRY   *location,
    char	    *filename,
    i4	    l_filename,
    CL_ERR_DESC	    *sys_err);

static DB_STATUS config_handler(
    i4     operation,
    DMP_DCB	*dcb,
    DM0C_CNF	**config,
    DMF_JSX	*jsx);

static DB_STATUS create_ckpjnldmp(
    DMF_JSX	*jsx,
    DMP_DCB	*dcb,
    i4     ckp_seq,
    bool        jflag,
    bool	dflag);

static DB_STATUS cluster_merge(
    DMF_JSX     *jsx,
    DMP_DCB	*dcb);

static DB_STATUS truncate_jnl_dmp(
    DMF_JSX	*jsx,
    DMP_DCB	*dcb,
    DM0C_CNF	**config);

static DB_STATUS online_display_stall_message(
    DMF_JSX	*jsx);

static DB_STATUS cpp_prepare_tbl_ckpt(
    DMF_CPP         *cpp,
    DMF_JSX         *jsx,
    DMP_DCB         *dcb);

static DB_STATUS cpp_ckpt_invalid_tbl(
    DMF_CPP         *cpp,
    DMF_JSX         *jsx,
    DMP_DCB         *dcb);

static DB_STATUS cpp_allocate_tblcb(
    DMF_JSX             *jsx,
    DMF_CPP             *cpp,
    CPP_TBLCB           **tablecb);

static VOID cpp_add_tblcb(
    DMF_CPP             *cpp,
    CPP_TBLCB           *tblcb );

static bool cpp_locate_tblcb(
    DMF_CPP             *cpp,
    CPP_TBLCB           **tblcb,
    DB_TAB_ID           *tabid);

static VOID cpp_display_no_ckpt_tables(
    DMF_CPP             *cpp,
    DMF_JSX             *jsx );

static VOID cpp_display_ckpt_tables(
    DMF_CPP             *cpp,
    DMF_JSX             *jsx );

static VOID cpp_display_inv_tables(
    DMF_CPP             *cpp,
    DMF_JSX             *jsx );

static DB_STATUS cpp_read_tables(
    DMF_CPP             *cpp,
    DMF_JSX             *jsx,
    DMP_DCB             *dcb);

static	DB_STATUS   cpp_prepare_save_tbl(
    DMF_CPP     *cpp,
    DMF_JSX	*jsx,
    DMP_DCB	*dcb);

static DB_STATUS cpp_save_chkpt_tbl_list(
    DMF_CPP         	*cpp,
    DMF_JSX	    	*jsx,
    DMP_DCB	    	*dcb,
    i4		ckpseq);

static DB_STATUS cpp_get_table_info(
    DMF_CPP             *cpp,
    DMF_JSX             *jsx,
    DMP_DCB             *dcb);

static DB_STATUS cpp_get_blob_info(
    DMF_CPP             *cpp,
    DMF_JSX             *jsx,
    DMP_DCB             *dcb);

static DB_STATUS cpp_wait_free(
    DMF_JSX     	*jsx,
    DMCKP_CB    	*d);

static	DB_STATUS   cpp_wait_for_purge(
    DMF_JSX	    *jsx,
    DMP_DCB	    *dcb);

static DB_STATUS cpp_wait_all(
    DMF_JSX     	*jsx);

#ifdef xDEBUG
static VOID trace_lgshow (
    i4                  flag,
    i4                  cnt,
    DMF_JSX             *jsx,
    DMP_DCB             *dcb);
#endif

static STATUS online_cxmsg_send(
    i4			phase,
    DMF_JSX		*jsx,
    DMP_DCB             *dcb);

static DB_STATUS
lk_ckp_db(
    DMF_JSX		*jsx,
    DMP_DCB         *dcb);

static VOID
    ckp_txn_lock_key(
    DMF_JSX		*jsx, 
    LK_LOCK_KEY		*lockxkey);

static DB_STATUS GetClusterStatusLock(
	DMF_JSX		*jsx);

static DB_STATUS ReleaseClusterStatusLock(
	DMF_JSX		*jsx);

#define	    OPEN_CONFIG		1
#define	    UPDATE_CONFIG	2
#define	    CLOSE_CONFIG	3
#define	    CLOSEKEEP_CONFIG	4
#define	    UNKEEP_CONFIG	5
#define	    DUMP_CONFIG		6

/* Timing Constants - can be used in any function
** Each constant is either an initial counter value
** or a maximum cycles value that determines when
** a periodical action occurs
*/
/* max cycles to loop before issuing a messgae */
#define CPP_MSG_MAX_DELAY	30
/* cycles to loop before issuing first messgae */
#define CPP_MSG_1_DELAY		5
/* Initial value of retry counter */
#define CPP_MSG_INIT		CPP_MSG_MAX_DELAY - CPP_MSG_1_DELAY 

/*{
** Name: dmfcpp	- Check Point Processor.
**
** Description:
**      This routine manages the generation of a checkpoint. 
**
** Inputs:
**      context                         Pointer to DMF_CONTEXT
**	dcb				Pointer to DCB for the database.
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
**	21-jan-1989 (Edhsu)
**	    Modify code to support online backup.
**	23-oct-1989 (rogerk)
**	    When build LK_CKP_DB lock, only copy 12 bytes of dbname to lock
**	    key as that is all that will fit.
**	28-nov-1989 (mikem)
**	    Close database when there is a failure of online checkpoint, due
**	    to there already being another online checkpoint of the same db in
**	    progress.  If the db is not closed then the LG rundown code will
**	    mistakenly cleanup up the running online checkpoint, rather than
**	    the failed one.
**	 5-dec-1989 (rogerk)
**	    At start of online backup (while DB is stalled), copy the config
**	    file to the DUMP directory.  This gives a consistent copy of the
**	    config file that can be used by rollforward.  This prevents us
**	    from having to back out any updates made to the config file during
**	    the backup.
**	13-dec-1989 (rogerk)
**	    Put back in the dm0l_drain call to wait for the database purge to
**	    complete.  This was commented out during online-checkpoint
**	    development.
**	15-Dec-1989 (ac)
**	    Fixed the cluster online checkpoint haning problem : CKPDB is 
**	    holding CKP_CLUSTER lock and waiting for an ongoing transaction 
**	    to finish but the transaction is waiting for the CKP_CLUSTER lock.
**	    Have CKPDB requested CKP_CLUSTER lock after starting the
**	    online checkpoint.
**	02-jan-1990 (mikem)
**	    Added another event to wait on during online checkpoint 
**	    (LG_E_BACKUP_FAILURE).  This event is signaled if the ACP encounters
**	    an error while processing the database we are online checkpointing.
**	    This event is per database rather than system wide.
**	10-jan-1990 (rogerk)
**	    Changed algorithms for how to get correct journal records written
**	    to the old/new journal files.  If Online Checkpoint of a journaled
**	    database, then don't create the new journal history entry or
**	    increment the journal file sequence number here - it will be done
**	    in the archiver when the SBACKUP record is encountered.  Signal
**	    a CP and a PURGE event after writing the SBACKUP record to ensure
**	    that the archiver will purge the database up to the SBACKUP record
**	    during the checkpoint.  Do not complete checkpoint processing
**	    until the PURGE has compeleted.  This way we know that the old
**	    journal records have been flushed to the old journal files.
**
**	    If running in a cluster installation, we have to signal a CP
**	    and a PURGE before writing the SBACKUP record in order to merge
**	    the old journal files.  After the PURGE has completed, we can
**	    do the journal merge.  Added the cluster_merge routine to do this.
**
**	    Change initial setting of the jnl_fil_seq value in the config
**	    file.  It is initialized to 1 - which is the next journal file to
**	    use.  When a checkpoint is executed, the jnl_fil_seq will be
**	    incremented (in dmfcpp if offline or non-journaled datbase - in
**	    archiver if online and journaled) if the current journal file #
**	    was used by the previous checkpoint.  If it wasn't (database was
**	    not journaled) then we don't increment the value - we just use
**	    the current jnl file sequence.
**	12-jan-1990 (rogerk)
**	    Beautified cluster busy error message - added tab before line that
**	    was out of adjustment with others.  Added logging of sbackup and
**	    ebackup log record addresses.
**	23-apr-1990 (walt)
**	    Copy the config file to the dump area for all checkpoints, not
**	    just Online Backup.  Then rollforward can use this copy of the
**	    config file rather than being forced to initially restore the
**	    default location checkpoint to find out about the locations.  If
**	    rollforward doesn't need to restore the default location first,
**	    the checkpoints can be restored "concurrently".
**	14-may-1990 (rogerk)
**	    Added error parameter count to ule_format calls which were missing
**	    it.  Made sure that error code is set before breaking with
**	    error condition.  Change cleanup code at the bottom to not
**	    overwrite the error condition value.
**	21-may-90 (bryanp)	    
**	    Set DSC_DUMP_DIR_EXISTS in the config file after creating the
**	    dump directory.
**	22-apr-91 (rogerk)
**	    Disallow online backup while database is being acted upon with
**	    Set Nologging flag.  Online backup algorithms don't work if
**	    updates are not being logged.
**	19-aug-92 (ananth)
**	    Rewrite backup to simplify and modularize the online backup code. 
**	    No functional change.
**	04-feb-93 (jnash)
**	    - Append to end of existing ckpdb.dbg files rather than creating
**	      new one each time. 
**	15-mar-1993 (rogerk)
**	    Reduced Logging - Phase IV:
**	    Removed db_id argument from online_setup, online_write_sbackup
**		and online_write_ebackup routines.  This db_id used to be
**		used to hold the logging system internal DBID for the database
**		being backed up.  It is no longer needed as the external
**		database id is now written correctly in the sbackup and ebackup
**		log records.
**	    Removed requirement to signal a Consistency Point in order to
**		request the archiver to flush records to the journal/dump files.
**	    Moved signal to start archiver from the online_signal_cp routine
**		to the online_wait_for_purge routine.
**	    Moved the Consistency Point request which is needed before we
**		can start the database backup to after the point when database
**		activity is resumed.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**      20-sep-1993 (jnash)
**          Alterdb delete_oldest_ckp  
**	     - Split file_maint() into truncate_jnl_dmp() and jsp_file_maint(),
**	       the latter moved to dmfjsp and also used by alterdb.
**	    Cleanup a few TRdisplay() and TRformat() messages.
**	31-jan-1994 (jnash)
**	    Pass pointer to pointer to config file to online_setup().
**	18-feb-94 (stephenb)
**	    Remove failure case in call to dma_write_audit() we audit this 
**	    elsewhere. Move call to a place where we can ensure successfull 
**	    completion.
**	18-apr-1994 (jnash)
**	    B58026.  JFopen() in truncate_jnl_dump() was failing after
**	    the journal file block size had been changed with alterdb 
**	    -jnl_block_size, because the block size stored in the config 
**	    file didn't match the block size in the journal file header.  
**	    This happened only after an alterdb -disable_journaling 
**	    (ckpdb -j cleans up old journals).  The change has us not
**	    call truncate_jnl_dump() if ckpdb +j performed.
**      07-jan-1995 (alison)
**          Partial checkpoint.
**      24-jan-1995 (stial01)
**          BUG 66482,66519: if offline table checkpoint and just locking the
**          database, we need to open database to get table info.
**	22-feb-1995 (shero03)
**	    Bug #66987
**	    Support an index into jsp_file_maint
**	11-sep-1995 (thaju02)
**	    Database level checkpoint to create a file list of all
**	    tables in database at time of checkpoint.
**      18-oct-1995 (thaju02)
**          Bug #71996 - was unable to ckpdb +j repeatedly.
**	27-Nov-1995 (mckba01)
**	    BUG 71090, don't skip alias locations when -table=<tabname>
**	    option has been specified on the command line.
**	21-May-1996 (hanch04)
**	    Added cpp_wait_free, cpp_wait_all for multi-device checkpoint
**	28-may-96 (nick)
**	    Enforce a purge of journal records from the logfile if we are
**	    attempting to checkpoint a journal disabled database ; if we 
**	    don't do this we can end up with the discarded journals in the
**	    new journal set if the archiver hasn't discarded them already.
**	24-jun-96 (nick)
**	    Add some additional error handling ; the DBU lock was remaining
**	    after a failed online checkpoint.
**	12-jan-98 (popri01)
**	    X-integ of john saroka's 1.2/01 change #430380:
**	    Bug 80621: on some platforms, dm0l_drain() may fail with a non-
**	    zero dsc_open_count detected in the .cnf file. This can occur
**	    if the database was set to -disable_journaling prior to ckpdb
**	    being run. This appears to be a timing problem, wherein dmfrcp
**	    sets the open count, and then dmfjsp reads it before dmfrcp
**	    can set it back to zero. The fix was to retry the operation
**	    if the error occurs, giving dmfrcp a chance to reset the count.
**	27-may-98 (thaju02)
**	    For online checkpoint, avoid calling update_catalogs().
**	    We should not be modifying catalogs, since database is opened
**	    "no lock"; resulted in stale catalog pages being written out
**	    to disk. (B91000)
**	15-Mar-1999 (jenjo02)
**	    Rewrote LK_CKP_TXN scheme, again.
**	15-jul-1999 (nanpr01)
**	    Implementation of SIR 92299. Enabaling journalling on a
**	    online checkpoint.
**	    Back out changes for 91000. Now we use cache locking for core
**	    catalogs. Consequently we can update core catalogs without
**	    database lock because they do physical locking and write
**	    on unfix and changes can be visible by server using cache locks.
**	14-June-2001 (bolke01)
**	    Following a disable_journalling request.
** 	    The Archiver is checked for activity, the recovery server is 
**	    checked to be alive, so we wait indefinitly for it. 
**	    Comfort messages are issued if TRACE (#x) is used.
**	    (104984)
**	26-jul-2002 (thaju02)
**	    Race condition can occur between update catalogs transaction
**	    which is being pass aborted and a drop table user transaction.
**	    Must hold LK_CKP_DB lock until dmfrcp has completed pass 
**	    abort processing for the update catalogs transaction.
**	    (B108377)
**	10-Oct-2006 (wanfr01)
**	    Bug 116841
**	    Fix db_id so checkpoint waits for a LK_CKP_TXN lock
**      03-nov-2006 (horda03) Bug 116231
**          Need to merge journal files if the DB was journalled but
**          journalling has been disabled by alterdb.
**	21-Dec-2007 (jonj)
**	    Add GetClusterStatusLock(), ReleaseClusterStatusLock()
**	    to prevent concurrent DB checkpoints from multiple nodes
**	    in a cluster. Tidy up LK_CKP_CLUSTER lock forms.
**	30-may-2008 (joea)
**	    dm0d_merge now takes only two arguments.
*/
DB_STATUS
dmfcpp(
DMF_JSX	    *jsx,
DMP_DCB	    *dcb)
{
    DMF_CPP             cpp;
    DM0C_CNF		*cnf = 0;
    i4		sequence;
    CL_ERR_DESC		k;
    DB_STATUS		status, local_status;
    STATUS		cl_status;
    i4             save_ckp_seq = 0;
    bool                journal_flag = FALSE;
    bool		dump_flag = FALSE;
    CL_ERR_DESC		sys_err;
    i4             error;
    i4		flag;
    i4		open_flags;
    i4		open_access;
    i4		open_lockmode;
    char		line_buffer[132];
    DB_TAB_TIMESTAMP    sbackup_time;
    bool		dbu_locked = FALSE;
    bool		cluster_locked = FALSE;
    bool		txn_locked = FALSE;
    i4			abort_xact_lgid = 0;
    LK_LLID		ckp_lock_list;
    LK_LKID		lockxid;
    bool		ClusterStatusLocked = FALSE;
    DB_ERROR		local_dberr;
    i4			*err_code = &jsx->jsx_dberr.err_code;

    CLRDBERR(&jsx->jsx_dberr);

    /*
    ** 15-Mar-1999 (jenjo02) Rearchitect online checkpoint stall scheme
    **
    **	o LG_A_SBACKUP functionality has been extended such that, for
    **	  each active transaction in the database, a SHARE LK_CKP_TXN
    **	  lock is affixed to the transaction's lock list.
    **  o CKPDB then requests an EXCL LK_CKP_TXN lock, stalling until
    **	  the database has quiesced (all active txns have completed).
    **  o Transactions which attempt to become active in the database
    **	  while CKPDB is preparing for the checkpoint will have their
    **	  LGreserve/LGwrite requests rejected with a status of 
    **	  LG_CKPDB_STALL and are expected to acquire a SHARE LK_CKP_TXN
    **	  lock, stalling until CKPDB release its EXCL lock, and then
    **	  retrying the LGreserve/LGwrite operation.
    **
    **	  See also lgalter, lgreserve, lgwrite, and dm0l.
    */

    if (jsx->jsx_status & JSX_TRACE)
    {
#ifdef VMS
	    flag = TR_F_OPEN;
#else
	    flag = TR_F_APPEND;
#endif
	TRset_file(flag, "ckpdb.dbg", 9, &k);
    }

    /*	Prepare for the checkpoint. */
    STRUCT_ASSIGN_MACRO(dcb->dcb_name, jsx->jsx_db_name);
    STRUCT_ASSIGN_MACRO(dcb->dcb_db_owner, jsx->jsx_db_owner);

    TRformat( dmf_put_line, 0, line_buffer, sizeof(line_buffer),
	      "%@ CPP: Preparing to checkpoint database: %~t\n",
	      sizeof(dcb->dcb_name), &dcb->dcb_name);

    if (jsx->jsx_status & JSX_DRAINDB)
	TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
	    "%@ CPP: Preparing to drain the log file: %~t\n",
	    sizeof(dcb->dcb_name), &dcb->dcb_name);

    if (jsx->jsx_status & JSX_TRACE)
	TRdisplay("%@ CPP: Preparing to checkpoint database: %~t\n",
		sizeof(dcb->dcb_name), &dcb->dcb_name);

    /*
    ** Open/Lock the database and check for database-in-use.
    ** If Online Checkpoint, then open the database without locking it.
    */
    if (jsx->jsx_status & JSX_CKPXL)
    {
	/* Offline Checkpoint */
	if (jsx->jsx_status & JSX_TRACE)
	    TRdisplay("%@ CPP: Option Offline Backup Selected.\n");

	open_access = DM2D_A_WRITE;
	open_lockmode = DM2D_X;
	open_flags = DM2D_JLK;
	         
	if ((jsx->jsx_status & JSX_WAITDB) == 0)
	    open_flags |= DM2D_NOWAIT;
    }
    else
    {
	/* Online Checkpoint */
	if (jsx->jsx_status & JSX_TRACE)
	    TRdisplay("%@ CPP: Option Online Backup Selected.\n");

	open_access = DM2D_A_READ;
	open_lockmode = DM2D_N;

	/*
	** If Cluster, just lock, don't open DB until
	** we successfully get the Cluster Status lock.
	*/
	if ( CXcluster_enabled() )
	    open_flags = DM2D_JLK;
	else
	    open_flags = DM2D_NLK;
    }

    status = dm2d_open_db(dcb, open_access, open_lockmode,
	dmf_svcb->svcb_lock_list, open_flags, &jsx->jsx_dberr);

    /*
    ** Check for dm2d_open_db failure.
    ** Check explicitly for Database-in-use error.
    */
    if (status != E_DB_OK)
    {
	if (jsx->jsx_dberr.err_code == E_DM004C_LOCK_RESOURCE_BUSY)
	{
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM110D_CPP_DB_BUSY);
	    return (E_DB_INFO);
	}
	uleFormat(&jsx->jsx_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
	    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	SETDBERR(&jsx->jsx_dberr, 0, E_DM1108_CPP_OPEN_DB);
	return (E_DB_ERROR);
    }

    /* Use server lock list, unless jsx->jsx_stall_time != 0 */
    jsx->jsx_lock_list = dmf_svcb->svcb_lock_list;
    jsx->jsx_log_id = dcb->dcb_log_id;
    jsx->jsx_db_id = dcb->dcb_id; 
    ckp_lock_list = jsx->jsx_lock_list;

    for (;;)
    {
	if ( !(jsx->jsx_status & JSX_CKPXL) && CXcluster_enabled() )
	{
	    /*
	    ** Database is not locally busy.
	    **
	    ** Get cluster-wide status lock LK_X; if already
	    ** held for this DB, then a ckpdb is already
	    ** running.
	    **
	    ** We'll hold the status lock until this ckpdb
	    ** completes.
	    */
	    status = GetClusterStatusLock(jsx);

	    if ( status )
	    {
		if (jsx->jsx_dberr.err_code == E_DM004C_LOCK_RESOURCE_BUSY)
		{
		    status = dm2d_close_db(dcb, 
				dmf_svcb->svcb_lock_list, 
				open_flags, &jsx->jsx_dberr);

		    if ( status != E_DB_OK )
		    {
			uleFormat(&jsx->jsx_dberr, 0, NULL, ULE_LOG, NULL, 
			    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
		    }

		    SETDBERR(&jsx->jsx_dberr, 0, E_DM110D_CPP_DB_BUSY);
		    return (E_DB_INFO);
		}
		SETDBERR(&jsx->jsx_dberr, 0, E_DM1108_CPP_OPEN_DB);
		break;
	    }
	    ClusterStatusLocked = TRUE;

	    /* Now really open the DB */
	    status = dm2d_open_db(dcb, open_access, open_lockmode,
		    dmf_svcb->svcb_lock_list, DM2D_NLK, &jsx->jsx_dberr);
	    if (status != E_DB_OK)
		break;
	    jsx->jsx_log_id = dcb->dcb_log_id;
	}


	/* If offline checkpoint db is not open, just locked */
	if (jsx->jsx_status & JSX_CKPXL)
	{
	    status = dm2d_open_db(dcb, DM2D_A_READ, DM2D_N,
			dmf_svcb->svcb_lock_list, DM2D_NLK, &jsx->jsx_dberr);
	    if (status != E_DB_OK)
		break;
	    jsx->jsx_log_id = dcb->dcb_log_id;
	}

	if (!(jsx->jsx_status & JSX_CKPXL))
	{
	    /* Online Checkpoint */
	    if (dcb->dcb_status & DCB_S_NOBACKUP)
	    {
		/* online checkpoint has been disabled */
		SETDBERR(&jsx->jsx_dberr, 0, E_DM116C_CPP_NO_ONLINE_CKP);
		break;
	    }
	}

	MEfill( sizeof(DMF_CPP), '\0', (char *)&cpp);

	/* Get table info for all tables being checkpointed */
	status = cpp_prepare_tbl_ckpt(&cpp, jsx, dcb);
	if (status != E_DB_OK)
	    break;

	/*
	** If partial backup... get table information
	*/
	if ((jsx->jsx_status1 & JSX1_CKPT_DB) == 0)
	{
	    /*
	    ** Find out 'interesting locations and make list of
	    ** filenames at each 'interesting' location
	    */
	    status = cpp_prepare_save_tbl(&cpp, jsx, dcb);
	    if (status != E_DB_OK)
		break;
	}


#ifdef xDEBUG
	if (jsx->jsx_status & JSX_TRACE)
	    trace_lgshow (LG_S_DATABASE,__LINE__,jsx,dcb) ;
#endif

	/*  Open and read the configuration file. */

	status = config_handler(OPEN_CONFIG, dcb, &cnf, jsx);
	if (status != E_DB_OK)
	    break;

	/*
	** If the database is journal disabled and neither +j or -j
	** specified, disallow the checkpoint.
	*/
	if ((cnf->cnf_dsc->dsc_status & DSC_DISABLE_JOURNAL) &&
	    ((jsx->jsx_status & (JSX_JON | JSX_JOFF)) == 0))
	{
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM1157_CPP_DISABLE_JNL);
	    break;
	}

	/* If offline checkpoint, close db, keep db lock */    
	/* bolke01 - do not close here  - do it after drain*/

	if ((jsx->jsx_status & JSX_CKPXL) &&
	    (!(cnf->cnf_dsc->dsc_status & (DSC_JOURNAL | DSC_DISABLE_JOURNAL))))
	{  
            status = config_handler(CLOSE_CONFIG, dcb, &cnf, jsx);
            if (status != E_DB_OK)
                break;   
	    status = dm2d_close_db(dcb, dmf_svcb->svcb_lock_list,
			DM2D_NLK, &jsx->jsx_dberr);
	    if (status != E_DB_OK)
	        break;
	    status = config_handler(OPEN_CONFIG, dcb, &cnf, jsx);
	    if (status != E_DB_OK)
	        break;
	}

	/*
	** Start Online Backup protocols.
	**
	** Signal the start of Online Backup to the Logging System and wait for
	** the Logging System to indicate that the Backup can proceed. Backup
	** can proceed when the database has been quiesed. ie. there are no
	** active transactions.
	**
	** Get EXCL LK_CKP_TXN lock to prevent transactions from becoming
	** active while preparing for the backup.
	**
	** Get Cluster Checkpoint Lock to block out WRITE access to the DB on
	** other nodes of the cluster.
	**
	** Get DBU lock to prevent database reorganization during the backup.
	*/

	if ((jsx->jsx_status & JSX_CKPXL) == 0)
	{    
	    /*
	    ** Check for Set Nologging use on the database.  Disallow online
	    ** checkpoint while database is being updated without logging.  This
	    ** is because online checkpoint algorithms depend on extracting
	    ** database change records from the log file in order to backout
	    ** any updates that occur while the backup is in progress.
	    **
	    ** Note that we are protected from a new NOLOGGING transaction
	    ** starting during the online backup by having the set nologging
	    ** code also check for online backup status.
	    */
	    if (cnf->cnf_dsc->dsc_status & DSC_NOLOGGING)
	    {
		SETDBERR(&jsx->jsx_dberr, 0, E_DM1156_CPP_NOLOGGING);
		break;
	    }

	    if ((cnf->cnf_dsc->dsc_dbaccess & DU_OPERATIVE) == 0)
	    {
		SETDBERR(&jsx->jsx_dberr, 0, E_DM000F_BAD_DB_ACCESS_MODE);
		break;
	    }

	    status = config_handler(CLOSE_CONFIG, dcb, &cnf, jsx);
	    if (status != E_DB_OK)
		break;

	    if (CXcluster_enabled())
	    {
		/* CXmsg_send, and wait for all nodes to finish */
		status = online_cxmsg_send(CKP_CSP_INIT, jsx, dcb);
		if (status != E_DB_OK)
		    break;

		/* CXmsg_send, and wait for all nodes to finish */
		status = online_cxmsg_send(CKP_CSP_STALL, jsx, dcb);
	    }
	    else
	    {
		status = online_setup(jsx, &ckp_lock_list);
	    }

	    if (status == E_DB_INFO) 
		return(E_DB_INFO);

	    if (status != E_DB_OK) 
		break;

	    status = lk_ckp_db(jsx, dcb);
	    if (status != E_DB_OK)
		break;

	    txn_locked = TRUE;
	    dbu_locked = TRUE;
	    if (CXcluster_enabled())
		cluster_locked = TRUE;

 	    /*
	    ** Reopen the config file, it was closed before online_setup()  
	    ** so that transactions could access it if necessary in order
	    ** to complete.
	    */
	    status = config_handler(OPEN_CONFIG, dcb, &cnf, jsx);
	    if (status != E_DB_OK)
		break;
	}

	save_ckp_seq = cnf->cnf_jnl->jnl_ckp_seq;
	journal_flag = cnf->cnf_dsc->dsc_status & DSC_JOURNAL;
	dump_flag = cnf->cnf_dsc->dsc_status & DSC_DUMP;
	
	/*
	** If this is offline checkpoint, then we must drain all the current
	** log records to the journal files.  We must do this so we can then
	** create new journal files for the new checkpoint.  The drain should
	** have already started - and may already be done.  The log records
	** are drained when the database is closed.  Since this operation
	** requires an exclusive lock, the database must currently be closed.
	** We will wait here until the drain is complete.
	**
	** If this is online checkpoint, then the database may be open and
	** not in purge state.  In this case, we don't wait for a drain to
	** complete.  Online checkpoint must be able to keep track of which
	** log records go to which journal files.
	**
	** Wait for all journal records to be purged if there are journal
	** records present after the database has been disable_journaled
	** as well ; failure to do this will allow restoration of journaling
	** and subsequent purging of these records to the new journals.
	*/

	if ((jsx->jsx_status & JSX_CKPXL) &&
	    (cnf->cnf_dsc->dsc_status & (DSC_JOURNAL | DSC_DISABLE_JOURNAL)))
	{
	    i4         retries = CPP_MSG_INIT ; 

	    TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
		"%@ CPP: Archiver must drain log records from log file\n");
	    TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
		"%27* : to journal file.  If Archiver is not running\n");
	    TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
		"%27* : this program will not continue.  Exit the program\n");
	    TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
	      "%27* : and have the appropriate person restart the archiver.\n");

	    /* ensure config file is closed */
	    status = config_handler(CLOSE_CONFIG, dcb, &cnf, jsx);
	    if (status != E_DB_OK)
		break;

	    /* ensure Archiver does its job for us */
	    status = cpp_wait_for_purge(jsx, dcb);
	    if  (status != E_DB_OK)
		    break;

	    /* Now close database - needed it open for longer to
	    ** enable the LGshow command to function correctly 
	    */

	    status = dm2d_close_db(dcb, dmf_svcb->svcb_lock_list,
					 DM2D_NLK, &jsx->jsx_dberr);
	    if (status != E_DB_OK)
		break;

	    /* ensure config file is closed */
	    status = config_handler(CLOSE_CONFIG, dcb, &cnf, jsx);
	    if (status != E_DB_OK)
		break;

	    /*
	    ** Wait a few moments before checking the database/system states.
	    ** in the config. Then hang arround until we get the required 
	    ** result or an error occurs that needs handling 
	    ** Initial message after CPP_MSG_DELAY seconds thereafter 
	    ** CPP_MSG_MAX_DELAY seconds
	    ** Monitor the state of the Recovery Server when the CPP_MSG_MAX_DELAY
	    ** Note: if the recovery server is dead we will just abandon the ckpdb
	    ** time is reached.
	    */

	    while ( status == E_DB_OK )
	    {
		status = dm0l_drain(dcb, &jsx->jsx_dberr);

		if ( status == E_DB_INFO )
		{
		    /* test then re-initialize the retries counter if MAX_DELAY is reached */
		    if ( retries++ >= CPP_MSG_MAX_DELAY ) 
	    	    {  
			LGD	*lgd = (LGD *)LGK_base.lgk_lgd_ptr;
		        LPB	*master_lpb;

			retries = 1;

    	    	        if (jsx->jsx_status & JSX_TRACE)
			 TRdisplay("%@ CPP: Recovery Server has not yet closed database: %~t complete.\n",
				    sizeof(dcb->dcb_name), &dcb->dcb_name);
		        if (lgd->lgd_master_lpb)
		        {
			    master_lpb = (LPB *)LGK_PTR_FROM_OFFSET(lgd->lgd_master_lpb);
			    if (PCis_alive(master_lpb->lpb_pid) == FALSE)
			    {
				/* RCP Process is dead!!!! */
				SETDBERR(&jsx->jsx_dberr, 0, E_DM1170_NO_RECOVERY_SERVER );
				status = E_DB_ERROR;
				break;
			    }
		        }
		        else
		        {
			    /* RCP Process never started */
			    SETDBERR(&jsx->jsx_dberr, 0, E_DM1170_NO_RECOVERY_SERVER );
			    status = E_DB_ERROR;
			    break;
		        }
		    }
		    PCsleep(1000);
		    status = E_DB_OK;
		}
		else
		    break;
	    }

	    /* if there was any problem with the archiver we will now 
	    ** reach this spot with good reason 
	    ** Note: if the recovery server is dead we will have abandoned the ckpdb 
	    */
	    if (status == E_DB_INFO)
	    {  
		TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
		"%27* : Archiver may not have completed drain of journal records, \n");
		TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
		"%27* : or database has active sessions.\n");
		SETDBERR(&jsx->jsx_dberr, 0, E_DM1113_CPP_NOARCHIVER);
		break;
	    }

	    if (status == E_DB_OK)
		status = config_handler(OPEN_CONFIG, dcb, &cnf,  jsx);

	    if (status != E_DB_OK)
		break;

    	    if (jsx->jsx_status & JSX_TRACE)
		TRdisplay("%@ CPP: Journal flush for database: %~t complete.\n",
			sizeof(dcb->dcb_name), &dcb->dcb_name);
	}

	
	if (jsx->jsx_status & JSX_DRAINDB)
	{
	    /*  Close the configuration file. */

	    status = config_handler(CLOSE_CONFIG, dcb, &cnf, jsx);
	    if (status != E_DB_OK)
		break;

	    /*  Close the database. */

	    status = dm2d_close_db(
		dcb, dmf_svcb->svcb_lock_list, open_flags, &jsx->jsx_dberr);

	    if (status != E_DB_OK)
	    {
		uleFormat(&jsx->jsx_dberr, 0, NULL, ULE_LOG, NULL, 
		    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
		SETDBERR(&jsx->jsx_dberr, 0, E_DM110A_CPP_CLOSE_DB);
		break;
	    }

	    TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
		"%@ CPP: Completed drain of log file for database: %~t\n",
		sizeof(DB_DB_NAME), &dcb->dcb_name);

	    return (E_DB_OK);
	}

	/* Close the config file but keep the extent info in the dcb. */

	status = config_handler(CLOSEKEEP_CONFIG, dcb, &cnf, jsx);
	if (status != E_DB_OK)
	    break;

	/*  Create ckp and jnl directories if needed. */

	if (jsx->jsx_status & JSX_TRACE)
	    TRdisplay("%@ CPP: Creating ckp, jnl, dmp directories if needed\n");

	status = create_ckpjnldmp(
	    jsx, dcb, save_ckp_seq, journal_flag, dump_flag);
	if (status != E_DB_OK)
	    break;

	/* Now deallocate the extent info. */

	status = config_handler(UNKEEP_CONFIG, dcb, &cnf, jsx);
	if (status != E_DB_OK)
	    break;

	status = config_handler(OPEN_CONFIG, dcb, &cnf, jsx);
	if (status != E_DB_OK)
	    break;

	/*
	** If this is a cluster installation, then we have to do a merge 
	** of the old journal files.
	*/

	if ((cnf->cnf_dsc->dsc_status & (DSC_JOURNAL|DSC_DISABLE_JOURNAL)) && 
            (cnf->cnf_jnl->jnl_count) && (cnf->cnf_jnl->jnl_fil_seq) && 
	    (CXcluster_enabled()))
	{
            /*
	    ** Close the config file to allow outside tasks to gain access
	    ** to it while we wait for purge processing.  The archiver will
	    ** need access to it as will the cluster_merge below.
            */
            status = config_handler(CLOSE_CONFIG, dcb, &cnf, jsx);
            if (status != E_DB_OK)
                break;

	    /* 
	    ** Merge the journal files
	    ** 
	    ** If this is an online checkpoint, we first have to guarantee
	    ** that the log records have been purged to the journal files.
	    ** We already did this online_wait_for_purge on each node
	    ** in online_cx_phase (CKP_CSP_STALL)
	    */
            status = cluster_merge(jsx, dcb);
            if (status != E_DB_OK)
                break;

            status = config_handler(OPEN_CONFIG, dcb, &cnf, jsx);
            if (status != E_DB_OK)
                break;
	}

	/*
	** Mark configuration file as checkpoint in progress.  Also mark as
	** requiring a roll forward so that a restored checkpoint of a
	** journaled database can't be processed immediately.
	** Thirdly, mark that a dump directory has been created so that
	** we will start keeping an up to date backup of the config file.
	*/

	cnf->cnf_dsc->dsc_status |= DSC_CKP_INPROGRESS;
	if (cnf->cnf_dsc->dsc_status & DSC_JOURNAL)
	    cnf->cnf_dsc->dsc_status |= DSC_ROLL_FORWARD;
	cnf->cnf_dsc->dsc_status |= DSC_DUMP_DIR_EXISTS;

	if (jsx->jsx_status & JSX_TRACE)
	    TRdisplay("%@ CPP: Create jnl and dmp entry for ACP\n");

	/*  
	** Must create the ckp entry for ACP for online backup case.  When ACP
	** completes and ckpdb receives the FBAKCUP signal, then we must reopen
	** the config file to mark the ckp to be valid.
	**
	** The upd1_jnl_dmp routine will write the config file.
	*/

	status = upd1_jnl_dmp(jsx, dcb, &cnf);
	if (status != E_DB_OK)
	    break;

	/*
	** Copy the CONFIG file to the DUMP directory.  This config file
	** will be used rather than the backup version as we know this
	** version is consistent and we are not sure what state the backup
	** version is in since updates may be in progress (and we don't do
	** physical logging on the config file to restore with).
	**
	** 4/23/90 (walt) - do this for any checkpoint so we can use this
	** config file during rollforward, eliminating the need there to
	** first read the default location checkpoint to find out about
	** the database's locations.  If rollforward doesn't need to read
	** the default location first, the checkpoints can be restored
	** "concurrently".
	*/

	status = config_handler(DUMP_CONFIG, dcb, &cnf, jsx);
	if (status != E_DB_OK)
	{
	    dmfWriteMsg(&jsx->jsx_dberr, 0, 0);
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM110B_CPP_FAILED);
	    break;
	}

	/* 
	** Online backup: Write a start backup record to the log to mark
	** the start of required dump processing and signal a consistency 
	** to ensure that all needed data has been forced to the database
	** before we start the backup.
	**
	** Close the config file to allow outside tasks (archiver, infodb)
	** to proceed while we wait for the consistency point to complete.
	*/

	if ((jsx->jsx_status & JSX_CKPXL) == 0)
	{
	    status = config_handler(CLOSE_CONFIG, dcb, &cnf, jsx);
	    if (status != E_DB_OK)
		break;

	    if (cluster_locked)
	    {
	        /* CXmsg_send, and wait for all nodes to finish */
	        status = online_cxmsg_send(CKP_CSP_SBACKUP, jsx, dcb);
	    }
	    else
	    {
		status = online_write_sbackup(jsx);
	    }
	    if	(status != E_DB_OK)
		    break;
            /* liibi01 Bug 59771 
              ** we now use the time the SBACKUP record was written 
              ** for online checkpoints.
              */
            TMget_stamp((TM_STAMP *) &sbackup_time);

	    /* Now we can resume stalled transactions */
	    /* and release the EXCL LK_CKP_TXN lock.   */

	    if (cluster_locked)
		status = online_cxmsg_send(CKP_CSP_RESUME, jsx, dcb);
	    else
		status = online_resume(jsx, &ckp_lock_list);

	    txn_locked = FALSE;

	    if	(status != E_DB_OK)
		break;

	    /*
	    ** Signal a consistency point to force the database we are about
	    ** to backup to a consistent state on disk.
	    */
	    if (cluster_locked)
	        status = online_cxmsg_send(CKP_CSP_CP, jsx, dcb);
	    else
		status = online_signal_cp(jsx);
	    
	    if (status != E_DB_OK)
		break;

	    /* Reopen the config file. */

	    status = config_handler(OPEN_CONFIG, dcb, &cnf, jsx);
	    if (status != E_DB_OK)
		break;
	}

	/*  Perform the checkpoint.  Will also close the config file. */

	if (jsx->jsx_status & JSX_TRACE)
	    TRdisplay("%@ CPP: Check Point Processing\n");

	sequence = cnf->cnf_jnl->jnl_ckp_seq;
	
	/*
	** Make backup copy of database.
	** Note that check_point will write and close the config file
	** before copying the database.  Upon return, the config file
	** will be closed.
	*/

	status = check_point(&cpp, jsx, dcb, &cnf, sequence);    
	if (status != E_DB_OK)
	    break;

	/*
	** If this is Online Backup, then we must now write the End Backup
	** record.  This completes the backup window for which the archiver
	** must move records to the dump file.
	**
	** After writing the End Backup record, we wait till the archiver
	** completes moving records to the dump file.
	*/
	if ((jsx->jsx_status & JSX_CKPXL) == 0)
	{
	    if (cluster_locked)
	    {
	        /* CXmsg_send, and wait for all nodes to finish */
	        status = online_cxmsg_send(CKP_CSP_EBACKUP, jsx, dcb);
	    }
	    else
	    {
		status = online_write_ebackup(jsx);
		if (status != E_DB_OK)
		    break;

		status = online_wait_for_purge(jsx);
	    }

	    if  (status != E_DB_OK)
		break;
	}

	status = config_handler(OPEN_CONFIG, dcb, &cnf, jsx);
	if (status != E_DB_OK)
	    break;

	if (cluster_locked && (jsx->jsx_status & JSX_CKPXL) == 0)
	{
	    /* Perform the Merge on the dump files */
	    status = dm0d_merge(cnf, &jsx->jsx_dberr);

	    if (status != E_DB_OK)
		break;
	}


	/* We're done. Mark the checkpoint valid. */

	status = upd2_jnl_dmp(jsx, dcb, &cnf, &sbackup_time);
	if (status != E_DB_OK)
	    break;

	/*
	** 	We now have a valid checkpoint so
	**	Record we are checkpointing this database,
	**	checkpoint can be regarded as selecting from database
	*/
	if ( dmf_svcb->svcb_status & SVCB_C2SECURE )
	    local_status = dma_write_audit( SXF_E_DATABASE,
		    SXF_A_SELECT | SXF_A_SUCCESS,
		    dcb->dcb_name.db_db_name, /* Object name (database) */
		    sizeof(dcb->dcb_name.db_db_name), /* Object name (database) */
		    &dcb->dcb_db_owner, /* Object owner */
		    I_SX270B_CKPDB,     /*  Message */
		    TRUE,		    /* Force */
		    &local_dberr, NULL);

	/*
	** Note that : update_catalog only can change the core catalogs
	** in dmfjsp since only the core catalogs use cache locking.
	** This change backs out the fix for bug 91000.
	*/

	status = update_catalogs(jsx, dcb, &cnf, &abort_xact_lgid);
	if (status != E_DB_OK)
	   break;

	/*  
	** Perform truncation of last journal file set.
	** Don't bother if we are about to destroy old journal and dump files.
	*/
	if (((jsx->jsx_status & JSX_DESTROY) == 0) &&
	     (jsx->jsx_status & JSX_JON) == 0)
	{
	    status = truncate_jnl_dmp(jsx, dcb, &cnf);
	    if (status != E_DB_OK)
		break;
	}
	else if (jsx->jsx_status & JSX_DESTROY)
	{
	    /*  
	    ** JSX_DESTROY.  Do the destroy, update config file.
	    */
	    status = jsp_file_maint(jsx, dcb, &cnf, 0, TRUE);
	    if (status != E_DB_OK)
		break;

	    status = config_handler(UPDATE_CONFIG, dcb, &cnf, jsx);
	    if (status != E_DB_OK)
		break;
	}

	status = config_handler(CLOSE_CONFIG, dcb, &cnf, jsx);
	if (status != E_DB_OK)
	    break;

	/* 
	** Inform the logging system that the checkpoint is complete.
	** Release all remaining locks.
	*/

	if ((jsx->jsx_status & JSX_CKPXL) == 0)
	{
	    if (cluster_locked)
	    {
	        /* CXmsg_send, and wait for all nodes to finish */
	        status = online_cxmsg_send(CKP_CSP_DBACKUP, jsx, dcb);
                if (status == E_DB_OK)
                    status = online_cxmsg_send(CKP_CSP_STATUS, jsx, dcb);
		if ( status == E_DB_OK )
		{
		    status = ReleaseClusterStatusLock(jsx);
		    ClusterStatusLocked = FALSE;
		}
	    }
	    else
	    {
		status = online_dbackup(jsx);
	    }
	    if	(status != E_DB_OK)
		break;

	    /* Release DBU operation lock */
	    status = online_finish(jsx, dcb);
	    if (status != E_DB_OK)
		break;
	}

	/*  Close the database. */

	status = dm2d_close_db(
	    dcb, dmf_svcb->svcb_lock_list, open_flags, &jsx->jsx_dberr);

	if (status != E_DB_OK)
	{
	    uleFormat(&jsx->jsx_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM110A_CPP_CLOSE_DB);
	    return (E_DB_ERROR);
	}

	/*	Checkpoint complete. */

	if (jsx->jsx_status & JSX_VERBOSE)
	    TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
		"%@ CPP: Completed checkpoint of database: %~t\n",
		sizeof(DB_DB_NAME), &dcb->dcb_name);

	if (jsx->jsx_status & JSX_TRACE)
	    TRdisplay("%@ CPP: Completed checkpoint of database: %~t\n",
		sizeof(DB_DB_NAME), &dcb->dcb_name);

	if (jsx->jsx_status & JSX_TRACE)
	    TRset_file(TR_F_CLOSE, 0, 0, &k);

	/*  Return. */

	return (E_DB_OK);
    }

    /*	Error recovery. */

    {
	i4  	    local_err_code;
	/* Preserve jsx_dberr information */
	DB_ERROR    save_dberr = jsx->jsx_dberr;

	if (jsx->jsx_ckp_phase != CKP_CSP_NONE)
	{
	    CSP_CX_MSG		err_msg;

	    err_msg.csp_msg_action = CSP_MSG_7_CKP;
	    err_msg.csp_msg_node = jsx->jsx_node;
	    err_msg.ckp.dbid = jsx->jsx_db_id;
	    err_msg.ckp.phase = CKP_CSP_ABORT;
	    err_msg.ckp.ckp_node = jsx->jsx_ckp_node;
	    err_msg.ckp.ckp_var.abort_phase = jsx->jsx_ckp_phase;
	    TRdisplay("Last cluster phase %d sending ABORT msg \n", 
	    	jsx->jsx_ckp_phase);
	    cl_status = CXmsg_send( CSP_CHANNEL, NULL, NULL, (PTR)&err_msg);
	}

	/*  Close any open configuration file. */

	if (cnf)
	{
	    cnf->cnf_dsc->dsc_status &= ~(DSC_CKP_INPROGRESS|DSC_ROLL_FORWARD);
	    config_handler(CLOSE_CONFIG, dcb, &cnf, jsx);
	}

	if (dbu_locked)
	{
	    if (abort_xact_lgid)
  	    {
		/* 
		** if update_catalogs() transaction failure encountered,
		** and pass abort occurs - we can not release the
		** DBU lock until the pass abort has completed. 
		*/
		i4		locstat = 0;
		i4		length = 0;
		LG_TRANSACTION	tr;
		CL_ERR_DESC	locsyserr;
		MEcopy((char *)&abort_xact_lgid, sizeof(abort_xact_lgid), 
				(char *)&tr);
		while (1)
		{
		    locstat = LGshow(LG_S_ATRANSACTION, (PTR)&tr, sizeof(tr),
				&length, &locsyserr);
	    	    if ((locstat != OK) || (length == 0))
		    {
			if (locstat)
			    uleFormat(NULL, locstat, &locsyserr, ULE_LOG, NULL,
				(char *)NULL, 0L, (i4 *)NULL, &local_err_code, 0);
			break;
		    }
		    if (tr.tr_status & TR_PASS_ABORT)
		    	PCsleep(5000);
		    else
			break;
		}
	    }
	    cl_status = LKrelease(0, dmf_svcb->svcb_lock_list, &jsx->jsx_lockid,
		(LK_LOCK_KEY *)0, (LK_VALUE * )0, &sys_err);
	    if (cl_status != OK)
	    {
		uleFormat(NULL, E_DM901B_BAD_LOCK_RELEASE, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, &local_err_code, 0);
	    }
	}

	if ( ClusterStatusLocked )
	    (VOID)ReleaseClusterStatusLock(jsx);

#ifdef FIXME
/*
	if (cluster_locked)
	{
	    cl_status = LKrelease(0, dmf_svcb->svcb_lock_list,&jsx->jsx_lockcid,
		(LK_LOCK_KEY *)0, (LK_VALUE * )0, &sys_err);
	    if (cl_status != OK)
	    {
		uleFormat(NULL, E_DM901B_BAD_LOCK_RELEASE, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, &local_err_code, 0);
	    }
	}
*/
#endif

	if (txn_locked)
	{
	    cl_status = LKrelease(0, ckp_lock_list, &lockxid,
		(LK_LOCK_KEY *)0, (LK_VALUE * )0, &sys_err);
	    if (cl_status != OK)
	    {
		uleFormat(NULL, E_DM901B_BAD_LOCK_RELEASE, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, &local_err_code, 0);
	    }
	}

	/*	Close the database. */

	(VOID)dm2d_close_db(
	    dcb, dmf_svcb->svcb_lock_list, open_flags, &local_dberr);

        /*	Return with error. */

	/* Restore the "real" error */
	jsx->jsx_dberr = save_dberr;

	if (jsx->jsx_dberr.err_code)
	    dmfWriteMsg(&jsx->jsx_dberr, 0, 0);

	SETDBERR(&jsx->jsx_dberr, 0, E_DM110B_CPP_FAILED);
	return (E_DB_ERROR);
    }
}

/*{
** Name: online_setup
**
** Description:
**	This routine does the initial setup for online checkpoint.  This
**	includes error checking, stalling any new transactions, waiting for
**	active transactions to complete and getting the cluster checkpoint and
**	DBU operation locks.
**
** Inputs:
**      jsx				Pointer to checkpoint context.
**	dcb				Pointer to DCB for the database.
**
** Outputs:
**      err_code                        Reason for error return status.
**	Returns:
**	    E_DB_OK
**	    E_DB_INFO			If database busy (cluster case)
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    The config file is closed.
**
** History:
**      19-aug-1992 (Ananth)
**          Created for backup rewrite.
**	04-feb-93 (jnash)
**	    - Return E_DB_INFO in cluster busy case, not error.
**	    - Change order of DBU lock and LGevent(LG_E_SBACKUP) operations.
**	      The order of these events was reversed in the 19-aug-1992 
**	      change, for unknown reasons.  If done prior to the LGevent,
**	      is appears that a deadlock can occur when the lock is
**	      granted here and a subsequent transaction needs it to  
**	      complete a currently active transaction. 
**	15-mar-1993 (rogerk)
**	    Reduced Logging - Phase IV:
**	    Removed db_id argument from online_setup, online_write_sbackup
**		and online_write_ebackup routines.  This db_id used to be
**		used to hold the logging system internal DBID for the database
**		being backed up.  It is no longer needed as the external
**		database id is now written correctly in the sbackup and ebackup
**		log records.
**	    Removed LGshow to obtain the internal database id.
**	26-jul-1993 (bryanp)
**	    Removed an extra (duplicated) MEcopy call from online_setup().
**	31-jan-1994 (jnash)
**	    Close config file thereby releasing config file lock 
**	    prior to signaling E_BACKUP.  This was causing online checkpoints 
**	    to hang when the lock was needed in order to complete an open 
**	    transaction.  Change config param from pointer to pointer-to-
**	    pointer".
**	20-may-1994 (rmuth)
**	    Bug 63533 - Display information about the stall phase of online
**	    checkpoint.
**      29-jun-1995 (thaju02)
**          Modified due to addition of new ckpdb parameter
**	    '-max_stall_time=xx'.  
**      13-july-1995 (thaju02)
**          Added call to LKcreate_list, in order to create new lock list
**          without LK_NOINTERRUPT, for '-timeout' argument. '-max_stall
**          _time' changed to '-timeout'.
**	19-mar-96 (nick)
**	    Move place where we take LK_CKP_TXN to close small window
**	    where the deadlock it was designed to prevent could still occur.
**	24-jun-96 (nick)
**	    Additional error handling code.
**	13-Dec-1996 (jenjo02)
**	    Removed LK_CKP_TXN lock. Logging will now suspend newly-starting
**	    update transactions if online checkpoint is pending, and waits until all
**	    current update, protected transactions in the database are complete
**	    before starting the checkpoint.
**	13-Nov-1997 (jenjo02)
**	    Added -s option. If indicated, newly starting online 
**	    read-write log txns will be stalled while ckpdb is stalled.
**	    This is indicated to the logging system via LG_A_SBACKUP_STALL
**	    instead of the usual LG_A_SBACKUP call to LGalter().
**	15-Mar-1999 (jenjo02)
**	    Rewrote LK_CKP_TXN scheme, again.
**	26-Apr-2000 (thaju02)
**	    Added LGalter(LG_A_LBACKUP), to avoid race condition with
**	    txn becoming active. (B101303)
**	07-June-2000 (thaju02)
**	    Added another call to LGalter(LG_A_LBACKUP) for when ckpdb
**	    is initiated with the timeout option. If the checkpoint 
**	    times out, call LGalter(LG_A_LBACKUP) such that all
**	    transactions becoming active, that are suspended in 
**	    LGreserve() are resumed. (101773)
**	 
*/
static DB_STATUS
online_setup(
DMF_JSX	    *jsx,
LK_LLID     *ckp_lock_list)
{
    DB_STATUS		status;
    STATUS		cl_status;
    CL_ERR_DESC		sys_err;
    i4		event;
    LK_LOCK_KEY 	lockkey;
    LK_LOCK_KEY 	lockxkey;
    i4		length;
    i4             local_error;
    i4		event_mask;
    i4             node_id;
    i4		node_mask;
    char		node_buf[512];
    char		line_buffer[132];
    bool		ckptxn_locked = FALSE;
    bool		have_log = FALSE;
    i4			tx_id;
    DB_TRAN_ID		tran_id;
    i4			*err_code = &jsx->jsx_dberr.err_code;

    CLRDBERR(&jsx->jsx_dberr);

    /*
    ** 15-Mar-1999 (jenjo02) Rearchitect online checkpoint stall scheme
    **
    **	o LG_A_SBACKUP functionality has been extended such that, for
    **	  each active transaction in the database, a SHARE LK_CKP_TXN
    **	  lock is affixed to the transaction's lock list.
    **  o CKPDB then requests an EXCL LK_CKP_TXN lock, stalling until
    **	  the database has quiesced (all active txns have completed).
    **  o Transactions which attempt to become active in the database
    **	  while CKPDB is preparing for the checkpoint will have their
    **	  LGreserve/LGwrite requests rejected with a status of 
    **	  LG_CKPDB_STALL and are expected to acquire a SHARE LK_CKP_TXN
    **	  lock, stalling until CKPDB release its EXCL lock, and then
    **	  retrying the LGreserve/LGwrite operation.
    **
    **	  See also lgalter, lgreserve, lgwrite, and dm0l.
    */

    for (;;)
    {
	/*
	** Display any information about Open transactions
	** For cluster online checkpoint - information about open transactions
        ** on remote nodes will be displayed in the nodes rcp log.
	*/
	status = online_display_stall_message( jsx );
	if ( status != E_DB_OK )
	    break;

	/* Start a log transaction for wait events. */

	cl_status = LGbegin(
	    LG_NOPROTECT, 
	    jsx->jsx_log_id, 
	    (DB_TRAN_ID *)&tran_id, 
	    &tx_id, 
	    sizeof(DB_OWN_NAME), jsx->jsx_db_owner.db_own_name,
	    (DB_DIS_TRAN_ID*)NULL,
	    &sys_err);

	if (cl_status != OK)
	{
	    uleFormat(NULL, E_DM900C_BAD_LOG_BEGIN, &sys_err, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM1125_CPP_LGBGN_ERROR);
	    status = E_DB_ERROR;
	    break;
	}

	have_log = TRUE;

	/*
	** Mark the database in Online Backup State.
	**
	** For each active txn in the database, LGalter() will request
	** a SHARE CKP_TXN lock on behalf of that transaction. If there
	** are none, we'll be granted the EXCL lock, below. If any 
	** txns attemp to become active between now and the time we make 
	** the EXCL lock request, they'll acquire a SHARE CKP_TXN lock 
	** when their log request is rejected with a status of LG_CKPDB_STALL.
	*/
	if (jsx->jsx_status & JSX_TRACE)
	{
	    TRdisplay("%@ CPP: Register Database (%d) in Backup State\n",
		jsx->jsx_log_id);
	}
    
	cl_status = LGalter(LG_A_SBACKUP,
	    (PTR)&jsx->jsx_log_id,  sizeof(jsx->jsx_log_id), &sys_err);

	if (cl_status != OK)
	{
	    uleFormat(NULL, E_DM900B_BAD_LOG_ALTER, &sys_err, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, err_code,
		1, 0, LG_A_SBACKUP);

	    SETDBERR(&jsx->jsx_dberr, 0, E_DM1124_CPP_SBACKUP);
	    status = E_DB_ERROR;
	    break;
	}

	/*
	** Acquire EXCL CKP_TXN lock.
	**
	** If the LGalter(LG_A_SBACKUP) call above found active
	** transactions in this database, they now hold SHARE locks,
	** and this LK request will stall waiting for those txns
	** to complete and release their SHARE locks.
	**
	** We'll hold on to this EXCL lock until it's ok to unstall
	** the stalled txns, i.e., when online_resume() is called.
	*/
	if (jsx->jsx_status & JSX_TRACE)
	    TRdisplay("%@ CPP: Need Transaction Lock\n");

	/* Prepare LK_CKP_TXN lock key */
	ckp_txn_lock_key(jsx, &lockxkey);

	/*
	** If stall wait time specified, create a new lock list
	** (This is not necessary for online_setup called from online_cx_phase
	** on a remote cluster node, a new lock list was already created)
	*/
	if (jsx->jsx_lock_list == dmf_svcb->svcb_lock_list
	    && (jsx->jsx_status1 & JSX1_STALL_TIME))
	{
	    cl_status = LKcreate_list((LK_ASSIGN | LK_NONPROTECT),
			(LK_LLID)0, (LK_UNIQUE *)0, ckp_lock_list,
			0, &sys_err);
	    if (cl_status)
	    {
		uleFormat(NULL, E_DM901A_BAD_LOCK_CREATE, &sys_err,
		    ULE_LOG, NULL, (char *)NULL, 0L, (i4 *)NULL,
		    err_code, 0);
		SETDBERR(&jsx->jsx_dberr, 0, E_DM1131_CPP_LK_FAIL);
		status = E_DB_ERROR;
		break;
	    }
	} else
	    *ckp_lock_list = dmf_svcb->svcb_lock_list;

	cl_status = LKrequest(LK_PHYSICAL | LK_LOCAL, *ckp_lock_list,
		&lockxkey, LK_X, (LK_VALUE * )0, &jsx->jsx_lockxid,
		jsx->jsx_stall_time, &sys_err);

	if (cl_status)
	{
	    if ((jsx->jsx_status1 & JSX1_STALL_TIME) &&
	       (cl_status == LK_TIMEOUT))
	    {
		SETDBERR(&jsx->jsx_dberr, 0, E_DM004D_LOCK_TIMER_EXPIRED);
	    }
	    else
	    {
		uleFormat(NULL, E_DM901C_BAD_LOCK_REQUEST, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
		SETDBERR(&jsx->jsx_dberr, 0, E_DM1131_CPP_LK_FAIL);
	    }
	    status = E_DB_ERROR;
	    cl_status = LGalter(LG_A_LBACKUP, (PTR)&jsx->jsx_log_id,
            			sizeof(jsx->jsx_log_id), &sys_err);
	    break;
	}

	ckptxn_locked = TRUE;

	cl_status = LGalter(LG_A_LBACKUP, (PTR)&jsx->jsx_log_id,
            			sizeof(jsx->jsx_log_id), &sys_err);
 
	if (cl_status != OK)
	{
	    uleFormat(NULL, E_DM900B_BAD_LOG_ALTER, &sys_err, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, err_code,
		1, 0, LG_A_LBACKUP);
 
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM1124_CPP_SBACKUP);
	    status = E_DB_ERROR;
	    break;
	}

	if (jsx->jsx_status & JSX_TRACE)
	    TRdisplay("%@ CPP: Transaction Lock Granted.\n");

	/*
	** Wait for all active update transactions to complete.  This is
	** signaled by the assertion of the LG_E_SBACKUP event.
	**
	** As we've been granted the EXCL LK_CKP_TXN lock, all active
	** transactions have completed, else we wouldn't be here.
	*/
	if (jsx->jsx_status & JSX_TRACE)
	    TRdisplay("%@ CPP: Wait for Event LG_E_SBACKUP to Occur\n");

	event_mask = (LG_E_SBACKUP | LG_E_START_ARCHIVER | LG_E_BACKUP_FAILURE);

	cl_status = LGevent(0, tx_id, event_mask, &event, &sys_err);
	if ((cl_status != OK) || 
	    (event & (LG_E_START_ARCHIVER | LG_E_BACKUP_FAILURE)))
	{
	    if (cl_status != OK)
	    {
		uleFormat(NULL, E_DM900F_BAD_LOG_EVENT, &sys_err, ULE_LOG, NULL, 
		    (char *)NULL, (i4)0, (i4 *)NULL, err_code,
		    1, 0, event_mask);
		SETDBERR(&jsx->jsx_dberr, 0, E_DM1150_CPP_BACKUP_FAILURE);
	    }
	    else if (event & LG_E_START_ARCHIVER)
	    {
		uleFormat(NULL, E_DM1144_CPP_DEAD_ARCHIVER, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
		SETDBERR(&jsx->jsx_dberr, 0, E_DM1145_CPP_DEAD_ARCHIVER);
	    }
	    else if (event & LG_E_BACKUP_FAILURE)
	    {
		uleFormat(NULL, E_DM1149_CPP_BACKUP_FAILURE, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 1, 
		    sizeof(DB_DB_NAME), jsx->jsx_db_name.db_db_name);
		SETDBERR(&jsx->jsx_dberr, 0, E_DM1150_CPP_BACKUP_FAILURE);
	    }

	    status = E_DB_ERROR;
	    break;
	}

	if (jsx->jsx_status & JSX_TRACE)
	    TRdisplay("%@ CPP: Event LG_E_SBACKUP Occurred\n");
	    
	cl_status = LGend(tx_id, 0, &sys_err);
	have_log = FALSE;
	if (cl_status != OK)
	{
	    uleFormat(NULL, E_DM900E_BAD_LOG_END, &sys_err, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM1127_CPP_LGEND_ERROR);
	    status = E_DB_ERROR;
	    break;
	}

	/*
	** Display that the stall has finished
	*/
	TRformat( dmf_put_line, 0, line_buffer, sizeof(line_buffer),
	    "%@ CPP: Finished stall of database\n" );

	return (E_DB_OK);
    }

    /*
    ** Error handling.
    */
    if (have_log)
    {
	cl_status = LGend(tx_id, 0, &sys_err);
	if (cl_status != OK)
	{
	    uleFormat(NULL, E_DM900E_BAD_LOG_END, &sys_err, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM1127_CPP_LGEND_ERROR);
	    status = E_DB_ERROR;
	}
    }

    if (ckptxn_locked)
    {
	cl_status = LKrelease(((jsx->jsx_status1 & JSX1_STALL_TIME) ?
				LK_ALL : 0),
			  *ckp_lock_list, &jsx->jsx_lockxid,
			  (LK_LOCK_KEY *)0, (LK_VALUE * )0, &sys_err);
	if (cl_status != E_DB_OK)
	{
	    uleFormat(NULL, E_DM901B_BAD_LOCK_RELEASE, &sys_err, ULE_LOG, 
		NULL, (char *)NULL, (i4)0, (i4 *)NULL, &local_error, 0);
	    status = E_DB_ERROR;
	}
    }

    return (status);
}

/*{
** Name: online_signal_cp
**
** Description:
**	Signal for a consistency point and wait for the archiver to start
**	purging the log records.
**
** Inputs:
**      jsx				Pointer to checkpoint context.
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
**      19-aug-1992 (Ananth)
**          Created for backup rewrite.
**	4-feb-1993 (jnash)
**	    Add 2PURGE_COMPLETE to LGevent list.
**	15-mar-1993 (rogerk)
**	    Reduced Logging - Phase IV:
**	    Removed archiver signal from this routine.  We no longer signal
**		the purge of dump records until we write the EBACKUP record.
*/
static DB_STATUS
online_signal_cp(
DMF_JSX	    *jsx)
{
    STATUS		cl_status;
    CL_ERR_DESC		sys_err;
    i4			tx_id;
    DB_TRAN_ID		tran_id;
    i4		event;
    i4		item;
    i4			*err_code = &jsx->jsx_dberr.err_code;

    CLRDBERR(&jsx->jsx_dberr);

    for(;;)
    {
	/* Start a transaction to use for LGevent waits. */

	cl_status = LGbegin(LG_NOPROTECT, jsx->jsx_log_id,
	    (DB_TRAN_ID *)&tran_id, &tx_id, 
	    sizeof(DB_OWN_NAME), jsx->jsx_db_owner.db_own_name,
	    (DB_DIS_TRAN_ID*)NULL, &sys_err);

	if (cl_status != OK)
	{
	    uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	    uleFormat(NULL, E_DM900C_BAD_LOG_BEGIN, &sys_err, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM1151_CPP_CPNEEDED);
	    break;
	}

	/* Signal Consistency Point. */

	item = 1;
	cl_status = LGalter(LG_A_CPNEEDED, (PTR)&item, sizeof(item), &sys_err);
	if (cl_status != OK)
	{
	    uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	    uleFormat(NULL, E_DM900B_BAD_LOG_ALTER, &sys_err, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, err_code,
		1, 0, LG_A_CPNEEDED);
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM1151_CPP_CPNEEDED);
	    break;
	}

	if (jsx->jsx_status & JSX_TRACE)
	    TRdisplay("%@ CPP: Wait Event LG_E_ECPDONE to Occur\n");

	cl_status = LGevent(0, tx_id, LG_E_ECPDONE, &event, &sys_err);

	if (cl_status != OK)
	{
	    uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	    uleFormat(NULL, E_DM900F_BAD_LOG_EVENT, &sys_err, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, err_code,
		1, 0, LG_E_ECPDONE);
	    break;
	}

	cl_status = LGend(tx_id, 0, &sys_err);
	if (cl_status != OK)
	{
	    uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	    uleFormat(NULL, E_DM900E_BAD_LOG_END, &sys_err, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM1151_CPP_CPNEEDED);
	    break;
	}

	return(E_DB_OK);
    }
    return(E_DB_ERROR);
}

/*{
** Name: online_wait_for_purge
**
** Description:
**	Wait for archiver to complete purging log records.
**
** Inputs:
**      jsx				Pointer to checkpoint context.
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
**      19-aug-1992 (Ananth)
**          Created for backup rewrite.
**	15-mar-1993 (rogerk)
**	    Reduced Logging - Phase IV:
**	    Rewrote with new backup protocols.  Removed ONLINE_PURGE from the
**		Logging System and the EBACKUP/FBACKUP lg events.  We now
**		use a normal LG_A_ARCHIVE signal to start the archiver and
**		just loop showing the database waiting for the dump records
**		to be processed.  Changed to check for checkpoint errors by
**		looking at the database status returned from LGshow rather
**		than using an LGevent value.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**      26-jul-1993 (rogerk)
**	    Changed the way journal and dump windows are handled.  They are
**	    now tracked by real Log Addresses rather than the Consistency
**	    points that bound them.
**	20-sep-1993 (andys)
**	     Fix arguments to ule_format call.
**	22-Feb-2008 (kibro01) b119804
**	    The purge has finished if the dump has got far enough and there
**	    is no journalling at all (e.g. no journalled tables)
*/
static DB_STATUS
online_wait_for_purge(
DMF_JSX	    *jsx)
{
    STATUS		cl_status;
    CL_ERR_DESC		sys_err;
    LG_LA		purge_la;
    LG_LA		dbdump_la;
    LG_LA		dbjnl_la;
    LG_DATABASE		database;
    i4			tx_id;
    DB_TRAN_ID		tran_id;
    i4		event;
    i4		event_mask;
    i4		length;
    i4             local_error;
    bool		have_transaction = FALSE;
    bool		purge_complete = FALSE;
    char		line_buffer[132];
    i4			*err_code = &jsx->jsx_dberr.err_code;

    CLRDBERR(&jsx->jsx_dberr);

    for (;;)
    {
	/* Start a transaction to use for LGevent waits. */
	cl_status = LGbegin(
	    LG_NOPROTECT, jsx->jsx_log_id, 
	    (DB_TRAN_ID *)&tran_id, &tx_id, 
	    sizeof(DB_OWN_NAME), jsx->jsx_db_owner.db_own_name,
	    (DB_DIS_TRAN_ID*)NULL, &sys_err);

	if (cl_status != OK)
	{
	    uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	    uleFormat(NULL, E_DM900C_BAD_LOG_BEGIN, &sys_err, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM1125_CPP_LGBGN_ERROR);
	    break;
	}
	have_transaction = TRUE;


	/*
	** Show the database to check its DUMP and JOURNAL windows.
	** Record the end of the DUMP window as the point to which we must
	** complete archiver processing before we regard the purge as complete.
	*/
	database.db_id = (LG_DBID) jsx->jsx_log_id;
	cl_status = LGshow(LG_S_DATABASE, (PTR)&database, sizeof(database),
	    &length, &sys_err);
	if (cl_status != E_DB_OK)
	{
	    uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	    uleFormat(NULL, E_DM9017_BAD_LOG_SHOW, &sys_err, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, err_code, 1,
		0, LG_S_DATABASE);
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM1152_CPP_ONLINE_PURGE);
	    break;
	}
	purge_la.la_sequence = database.db_l_dmp_la.la_sequence;
	purge_la.la_block    = database.db_l_dmp_la.la_block;
	purge_la.la_offset   = database.db_l_dmp_la.la_offset;

	if (jsx->jsx_status & JSX_TRACE)
	    TRdisplay("%@ CPP: Wait for Archiver to purge %~t\n",
		sizeof(DB_DB_NAME), jsx->jsx_db_name.db_db_name);

	/*
	** Start Purge-Wait cycle:
	**
	**	Show the database and check the DUMP and Journal windows.
	**	Until the database has been completely processed up to the
	**	    purge address (recorded above), then signal an archive
	**	    cycle and wait for archive processing to complete.
	*/
	for (;;)
	{
	    /*
	    ** Show the database to get its current Dump/Journal windows.
	    */
	    database.db_id = (LG_DBID) jsx->jsx_log_id;
	    cl_status = LGshow(LG_S_DATABASE, (PTR)&database, sizeof(database),
		&length, &sys_err);
	    if (cl_status != E_DB_OK)
	    {
		uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL, 
		    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
		uleFormat(NULL, E_DM9017_BAD_LOG_SHOW, &sys_err, ULE_LOG, NULL, 
		    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 1,
		    0, LG_S_DATABASE);
		SETDBERR(&jsx->jsx_dberr, 0, E_DM1152_CPP_ONLINE_PURGE);
		break;
	    }

	    dbdump_la.la_sequence = database.db_f_dmp_la.la_sequence;
	    dbdump_la.la_block    = database.db_f_dmp_la.la_block;
	    dbdump_la.la_offset   = database.db_f_dmp_la.la_offset;

	    dbjnl_la.la_sequence = database.db_f_jnl_la.la_sequence;
	    dbjnl_la.la_block    = database.db_f_jnl_la.la_block;
	    dbjnl_la.la_offset   = database.db_f_jnl_la.la_offset;

	    TRdisplay("%@ CPP: Purge window seq %d block %d offset %d \n",
		purge_la.la_sequence, purge_la.la_block, purge_la.la_offset);

	    TRdisplay("%@ CPP: Dump window seq %d block %d offset %d \n",
		dbdump_la.la_sequence, dbdump_la.la_block, dbdump_la.la_offset);

	    TRdisplay("%@ CPP: Jnl  window seq %d block %d offset %d \n",
		dbjnl_la.la_sequence, dbjnl_la.la_block, dbjnl_la.la_offset);

	    /*
	    ** Check for backup errors recorded on the database by
	    ** outside processes (the archiver).
	    */
	    if (database.db_status & DB_CKPDB_ERROR)
	    {
		uleFormat(NULL, E_DM1149_CPP_BACKUP_FAILURE, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 1, 
		    sizeof(DB_DB_NAME), jsx->jsx_db_name.db_db_name);
		SETDBERR(&jsx->jsx_dberr, 0, E_DM1152_CPP_ONLINE_PURGE);
		break;
	    }

	    /*
	    ** If the database dump window indicates there is no dump/journal
	    ** work to process, then the purge wait is complete.
	    */
	    if ((database.db_f_dmp_la.la_sequence == 0) &&
		(database.db_f_dmp_la.la_block == 0) &&
		(database.db_l_dmp_la.la_sequence == 0) &&
		(database.db_l_dmp_la.la_block == 0) &&
		(database.db_f_jnl_la.la_sequence == 0) &&
		(database.db_f_jnl_la.la_block == 0) &&
		(database.db_l_jnl_la.la_sequence == 0) &&
		(database.db_l_jnl_la.la_block == 0))
	    {
		purge_complete = TRUE;
		break;
	    }

	    /*
	    ** If the database has no more dump/journal work to do previous
	    ** to the purge address, then the purge wait is complete.
	    ** Also complete if the dump has got far enough and there is
	    ** no journalling left to do at all (kibro01) b119804.
	    */
	    if (LGA_GT(&dbdump_la, &purge_la) &&
		( LGA_GT(&dbjnl_la, &purge_la) ||
		  (dbjnl_la.la_sequence == 0 && dbjnl_la.la_block == 0)
		)
	       )
	    {
		purge_complete = TRUE;
		break;
	    }

	    /*
	    ** Check the state of the archiver.  If it is dead then we have
	    ** to fail.  If its alive but not running then give it a nudge.
	    */
	    event_mask = (LG_E_ARCHIVE | LG_E_START_ARCHIVER);

	    cl_status = LGevent(LG_NOWAIT, tx_id, event_mask, &event, &sys_err);
	    if (cl_status != OK)
	    {
		uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL, 
		    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
		uleFormat(NULL, E_DM900F_BAD_LOG_EVENT, &sys_err, ULE_LOG, NULL, 
		    (char *)NULL, (i4)0, (i4 *)NULL, err_code,
		    1, 0, event_mask);
		SETDBERR(&jsx->jsx_dberr, 0, E_DM1152_CPP_ONLINE_PURGE);
		break;
	    }
	    else if (event & LG_E_START_ARCHIVER)
	    {
		uleFormat(NULL, E_DM1144_CPP_DEAD_ARCHIVER, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
		SETDBERR(&jsx->jsx_dberr, 0, E_DM1145_CPP_DEAD_ARCHIVER);
		break;
	    }

	    /*
	    ** If there is still archiver work to do, but the archiver is
	    ** is not processing our database, then signal an archive event.
	    */
	    if ( ! (event & LG_E_ARCHIVE))
	    {
		cl_status = LGalter(LG_A_ARCHIVE, (PTR)0, 0, &sys_err);
		if (cl_status != OK)
		{
		    uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL, 
			(char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
		    uleFormat(NULL, E_DM900B_BAD_LOG_ALTER, &sys_err, ULE_LOG, NULL, 
			(char *)NULL, (i4)0, (i4 *)NULL, err_code,
			1, 0, LG_A_ARCHIVE);
		    SETDBERR(&jsx->jsx_dberr, 0, E_DM1152_CPP_ONLINE_PURGE);
		    break;
		}
	    }

	    /*
	    ** Wait a few seconds before rechecking the database/system states.
	    */
	    PCsleep(5000);
	}

	/*
	** If we broke out of the above loop because of an error, then fall
	** through to error handling code below.
	*/
	if ( ! purge_complete)
	    break;

	if (jsx->jsx_status & JSX_TRACE)
	    TRdisplay("%@ CPP: Online Purge has completed for %~t\n",
		sizeof(DB_DB_NAME), jsx->jsx_db_name.db_db_name);

	cl_status = LGend(tx_id, 0, &sys_err);
	if (cl_status != OK)
	{
	    uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	    uleFormat(NULL, E_DM900E_BAD_LOG_END, &sys_err, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM1127_CPP_LGEND_ERROR);
	    break;
	}
	return(E_DB_OK);
    }

    if (have_transaction)
    {
	cl_status = LGend(tx_id, 0, &sys_err);
	if (cl_status != OK)
	{
	    uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, &local_error, 0);
	    uleFormat(NULL, E_DM900E_BAD_LOG_END, &sys_err, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, &local_error, 0);
	}
    }

    uleFormat(&jsx->jsx_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
	(char *)NULL, (i4)0, (i4 *)NULL, &local_error, 0);
    SETDBERR(&jsx->jsx_dberr, 0, E_DM1150_CPP_BACKUP_FAILURE);
    return(E_DB_ERROR);
}

/*{
** Name: cpp_wait_for_purge
**
** Description:
**	Wait for archiver to complete purging log records.
**
** Inputs:
**      jsx				Pointer to checkpoint context.
**	dcb				Pointer to DCB for the database.
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
**      19-aug-1992 (Ananth)
**          Created for Drain purge timing error.
*/
static DB_STATUS
cpp_wait_for_purge(
DMF_JSX	    *jsx,
DMP_DCB	    *dcb)
{
    STATUS		status = E_DB_OK;
    STATUS		cl_status;
    CL_ERR_DESC		sys_err;
    bool		have_transaction = FALSE;
    LG_LA		purge_la;
    LG_LA		dbdump_la;
    LG_DATABASE		database;
    i4		event;
    i4		event_mask;
    i4		length;
    i4             local_error;
    bool		purge_complete = FALSE;
    i4			tx_id;
    DB_TRAN_ID		tran_id;
    i4			*err_code = &jsx->jsx_dberr.err_code;

    CLRDBERR(&jsx->jsx_dberr);

    for (;;)
    {
	if (jsx->jsx_status & JSX_TRACE)
	{
	    char line_buffer[132];
	    TRformat( dmf_put_line, 0, line_buffer, sizeof(line_buffer),
	      "%@ CPP: cpp_wait_for_purge:OFFLINE\n" );
	}

	/*
	** Show the database to check its DUMP and JOURNAL windows.
	** Record the end of the DUMP window as the point to which we must
	** complete archiver processing before we regard the purge as complete.
	*/
	database.db_id = (LG_DBID) dcb->dcb_log_id;
	cl_status = LGshow(LG_S_DATABASE, (PTR)&database, sizeof(database),
	    &length, &sys_err);
	if (cl_status != E_DB_OK)
	{
	    uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	    uleFormat(NULL, E_DM9017_BAD_LOG_SHOW, &sys_err, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, err_code, 1,
		0, LG_S_DATABASE);
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM116F_CPP_DRN_PURGE);
	    break;
	}
	purge_la.la_sequence = database.db_l_dmp_la.la_sequence;
	purge_la.la_offset   = database.db_l_dmp_la.la_offset;

	if (jsx->jsx_status & JSX_TRACE)
	    TRdisplay("%@ CPP:OFFLINE Wait for Archiver to purge database %~t\n",
		sizeof(dcb->dcb_name.db_db_name), dcb->dcb_name.db_db_name);

	/*
	** Start Purge-Wait cycle:
	**
	**	Show the database and check the DUMP and Journal windows.
	**	Until the database has been completely processed up to the
	**	    purge address (recorded above), then signal an archive
	**	    cycle and wait for archive processing to complete.
	*/
	for (;;)
	{
	    if (jsx->jsx_status & JSX_TRACE)
	    {
		char line_buffer[132];
		TRformat( dmf_put_line, 0, line_buffer, sizeof(line_buffer),
                  "%@ CPP: cpp_wait_for_purge:INIT\n" );
	    }
	    /*
	    ** Show the database to get its current Dump/Journal windows.
	    */
	    database.db_id = (LG_DBID) dcb->dcb_log_id;
	    cl_status = LGshow(LG_S_DATABASE, (PTR)&database, sizeof(database),
		&length, &sys_err);
	    if (cl_status != E_DB_OK)
	    {
		uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL, 
		    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
		uleFormat(NULL, E_DM9017_BAD_LOG_SHOW, &sys_err, ULE_LOG, NULL, 
		    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 1,
		    0, LG_S_DATABASE);
		SETDBERR(&jsx->jsx_dberr, 0, E_DM116F_CPP_DRN_PURGE);
		break;
	    }

	    dbdump_la.la_sequence = database.db_f_dmp_la.la_sequence;
	    dbdump_la.la_offset   = database.db_f_dmp_la.la_offset;

	    /*
	    ** Check for backup errors recorded on the database by
	    ** outside processes (the archiver).
	    */
	    if (database.db_status & DB_CKPDB_ERROR)
	    {
		uleFormat(NULL, E_DM1149_CPP_BACKUP_FAILURE, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 1, 
		    sizeof(DB_DB_NAME), dcb->dcb_name);
		SETDBERR(&jsx->jsx_dberr, 0, E_DM116F_CPP_DRN_PURGE);
		break;
	    }

	    /*
	    ** If the database dump window indicates there is no dump work
	    ** to process, then the purge wait is complete.
	    */
	    if ((database.db_f_dmp_la.la_sequence == 0) &&
		(database.db_f_dmp_la.la_offset  == 0) &&
		(database.db_l_dmp_la.la_sequence == 0) &&
		(database.db_l_dmp_la.la_offset  == 0))
	    {
		purge_complete = TRUE;
		break;
	    }

	    /*
	    ** If the database has no more dump/journal work to do previous
	    ** to the purge address, then the purge wait is complete.
	    */
	    if (LGA_GT(&dbdump_la, &purge_la))
	    {
		purge_complete = TRUE;
		break;
	    }

	    /*
	    ** Check the state of the archiver.  If it is dead then we have
	    ** to fail.  If its alive but not running then give it a nudge.
	    */
	    event_mask = (LG_E_ARCHIVE | LG_E_START_ARCHIVER);

	    /* Start a transaction to use for LGevent waits. */
	    cl_status = LGbegin(LG_NOPROTECT, dcb->dcb_log_id,
			(DB_TRAN_ID *)&tran_id, &tx_id,
			sizeof(DB_OWN_NAME), jsx->jsx_db_owner.db_own_name,
			(DB_DIS_TRAN_ID*)NULL, &sys_err);
	    if (cl_status != OK)
	    {
		uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL, 
		    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
		uleFormat(NULL, E_DM900C_BAD_LOG_BEGIN, &sys_err, ULE_LOG, NULL, 
		    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
		SETDBERR(&jsx->jsx_dberr, 0, E_DM1125_CPP_LGBGN_ERROR);
		break;
	    }
	    have_transaction = TRUE;

	    cl_status = LGevent(LG_NOWAIT, tx_id, event_mask, &event, &sys_err);
	    if (cl_status != OK)
	    {
		uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL, 
		    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
		uleFormat(NULL, E_DM900F_BAD_LOG_EVENT, &sys_err, ULE_LOG, NULL, 
		    (char *)NULL, (i4)0, (i4 *)NULL, err_code,
		    1, 0, event_mask);
		SETDBERR(&jsx->jsx_dberr, 0, E_DM116F_CPP_DRN_PURGE);
		break;
	    }
	    else if (event & LG_E_START_ARCHIVER)
	    {
		uleFormat(NULL, E_DM1144_CPP_DEAD_ARCHIVER, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
		SETDBERR(&jsx->jsx_dberr, 0, E_DM1145_CPP_DEAD_ARCHIVER);
		break;
	    }

	    /*
	    ** If there is still archiver work to do, but the archiver is
	    ** not processing our database, then signal an archive event.
	    */
	    if ( ! (event & LG_E_ARCHIVE))
	    {
		cl_status = LGalter(LG_A_ARCHIVE, (PTR)0, 0, &sys_err);
		if (cl_status != OK)
		{
		    uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL, 
			(char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
		    uleFormat(NULL, E_DM900B_BAD_LOG_ALTER, &sys_err, ULE_LOG, NULL, 
			(char *)NULL, (i4)0, (i4 *)NULL, err_code,
			1, 0, LG_A_ARCHIVE);
		    SETDBERR(&jsx->jsx_dberr, 0, E_DM116F_CPP_DRN_PURGE);
		    break;
		}
	    }

	    /*
	    ** Wait a few seconds before rechecking the database/system states.
	    */
	    PCsleep(5000);
	}

	if (have_transaction)
	{
	    cl_status = LGend(tx_id, 0, &sys_err);
	    if (cl_status != OK)
	    {
		uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL, 
		    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
		uleFormat(NULL, E_DM900E_BAD_LOG_END, &sys_err, ULE_LOG, NULL, 
		    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
		SETDBERR(&jsx->jsx_dberr, 0, E_DM1127_CPP_LGEND_ERROR);
		break;
	    }
	}

	/*
	** If we broke out of the above loop because of an error, then fall
	** through to error handling code below.
	*/
	if ( ! purge_complete)
	    break;

	if (jsx->jsx_status & JSX_TRACE)
	    TRdisplay("%@ CPP:OFFLINE Purge has Completed for %~t\n",
		sizeof(dcb->dcb_name.db_db_name), dcb->dcb_name.db_db_name);

	return(E_DB_OK);
    }

    if (jsx->jsx_status & JSX_TRACE)
	TRdisplay("%@ CPP:OFFLINE Purge has Failed.\n");

    uleFormat(&jsx->jsx_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
	(char *)NULL, (i4)0, (i4 *)NULL, &local_error, 0);
    if (jsx->jsx_status & JSX_TRACE)
	TRdisplay("%@ CPP:OFFLINE Purge has Failed (%d).\n",jsx->jsx_dberr.err_code);
    SETDBERR(&jsx->jsx_dberr, 0, E_DM1150_CPP_BACKUP_FAILURE);

    return(E_DB_ERROR);
}

/*{
** Name: online_write_sbackup
**
** Description:
** This routine does the following.
**
**	- Write the start backup log record.  This marks the
**	  spot in the log file where the backup begins.  All
**	  updates between this and the end backup record must
**	  be logged in the dump file.
**
**	  When the archiver encounters the start backup log record
**	  it will create a new journal history entry and a new
**	  journal file will be created for new updates.
**
** Inputs:
**      jsx				Pointer to checkpoint context.
**      log_id
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
**      19-aug-1992 (Ananth)
**          Created for backup rewrite.
**	15-mar-1993 (rogerk)
**	    Reduced Logging - Phase IV:
**	    Removed db_id argument from online_setup, online_write_sbackup
**		and online_write_ebackup routines.  This db_id used to be
**		used to hold the logging system internal DBID for the database
**		being backed up.  It is no longer needed as the external
**		database id is now written correctly in the sbackup and ebackup
**		log records.
**	    Removed interaction with archiver from this routine.  We no longer
**		signal a Consistency Point or request a Database Purge here.
**		A consistency Point is still needed before the checkpoint can
**		proceed and will be requested by code higher up.  The Purge
**		is no longer needed at the start of the backup and will be
**		requested after the EBACKUP log record is written.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	26-jul-1993 (bryanp)
**	    Break to the end on error rather than returning from the middle.
**	    This eliminates a "statement not reached" warning.
**	26-jul-1993 (rogerk)
**	    Took out unused flag parameter to dm0l_sbackup.
**      24-jan-1995 (stial01)
**          BUG 66473: call dm0l_sbackup with flags=DM0L_TBL_CKPT if table ckpt.
*/
static DB_STATUS
online_write_sbackup(
DMF_JSX	    *jsx)
{
    DB_STATUS		status;
    LG_LSN		lsn;
    i4             flags;

    CLRDBERR(&jsx->jsx_dberr);

    for (;;)
    {
	/* Initialize flags for SBACKUP record BUG 66473 */
	if ((jsx->jsx_status1 & JSX1_CKPT_DB) == 0)
	    flags = DM0L_TBL_CKPT;
	else
	    flags = 0;

	/* Write the Start Backup record. */
	status = dm0l_sbackup(flags, jsx->jsx_log_id, &jsx->jsx_db_owner, 
		&lsn, &jsx->jsx_dberr);

	if (status != E_DB_OK)
	{
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM1126_CPP_DM0LSBACKUP);
	    break;
	}

	if (jsx->jsx_status & JSX_TRACE)
	{
	    TRdisplay(
	       "%@ CPP: Wrote Start Backup Log Record at Log Seq Num <%x,%x>\n",
		    lsn.lsn_high, lsn.lsn_low);
	}

	return (E_DB_OK);
    }
    return (E_DB_ERROR);
}

/*{
** Name: online_resume
**
** Description:
**	Signal the logging system to allow stalled transaction to resume.
**
** Inputs:
**      jsx				Pointer to checkpoint context.
**	dcb				Pointer to DCB for the database.
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
**      19-aug-1992 (Ananth)
**          Created for backup rewrite.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	15-Mar-1999 (jenjo02)
**	    Release EXCL LK_CKP_TXN lock.
*/
static DB_STATUS
online_resume(
DMF_JSX	    *jsx,
LK_LLID     *ckp_lock_list)
{
    STATUS		cl_status;
    CL_ERR_DESC		sys_err;
    LK_LOCK_KEY		lockxkey;
    i4			*err_code = &jsx->jsx_dberr.err_code;

    CLRDBERR(&jsx->jsx_dberr);

    if (jsx->jsx_status & JSX_TRACE)
	TRdisplay("%@ CPP: Requesting LG to resume stalled xacts for %~t\n",
	    sizeof(DB_DB_NAME), jsx->jsx_db_name.db_db_name);

    cl_status = LGalter(
	LG_A_RESUME, (PTR)&jsx->jsx_log_id, sizeof(jsx->jsx_log_id), &sys_err);

    if (cl_status != OK)
    {
	uleFormat(NULL, E_DM900B_BAD_LOG_ALTER, &sys_err, ULE_LOG, NULL, 
	    (char *)NULL, (i4)0, (i4 *)NULL, err_code,
	    1, 0, LG_A_RESUME);
	SETDBERR(&jsx->jsx_dberr, 0, E_DM1128_CPP_RESUME);
	return (E_DB_ERROR);
    }

    /*
    **
    ** Prepare LK_CKP_TXN lock key
    **
    ** We MUST use LK_LOCK_KEY to release the lock here, since 
    ** if this is online checkpoint on remote cluster node,
    ** jsx_lockxid is not saved between ONLINE_SETUP and ONLINE_RESUME phases
    */
    ckp_txn_lock_key(jsx, &lockxkey);

    /* Release the LK_X lock on LK_CKP_TXN */
    cl_status = LKrelease(0, *ckp_lock_list, (LK_LKID *)0,
	&lockxkey, (LK_VALUE * )0, &sys_err);
    if (cl_status != OK)
    {
	uleFormat(NULL, E_DM901B_BAD_LOCK_RELEASE, &sys_err, ULE_LOG, NULL,
	    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	SETDBERR(&jsx->jsx_dberr, 0, E_DM1128_CPP_RESUME);
	return(E_DB_ERROR);
    }

    return (E_DB_OK);
}

/*{
** Name: online_write_ebackup
**
** Description:
**	Wait for the archiver to purge log records upto the Start Backup record.
**	Write an End Backup record.
**	Wait for the archiver to complete moving records to the dump file.
**
** Inputs:
**      jsx				Pointer to checkpoint context.
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
**      19-aug-1992 (Ananth)
**          Created for backup rewrite.
**	04-feb-93 (jnash)
**	    Include LGevent() call to wait for CP to complete prior to 
**	    signaling EBACKUP.  This code appers to have gotten lost in 
**	    the 19-aug changes.
**	15-mar-1993 (rogerk)
**	    Reduced Logging - Phase IV:
**	    Removed db_id argument from online_setup, online_write_sbackup
**		and online_write_ebackup routines.  This db_id used to be
**		used to hold the logging system internal DBID for the database
**		being backed up.  It is no longer needed as the external
**		database id is now written correctly in the sbackup and ebackup
**		log records.
**	    Removed the purge_wait done before logging the ebackup record.
**		We need only wait for the archiver to finish all work up-to and
**		including the ebackup log record.
**	    Removed the EBACKUP signal and FBACKUP event wait.  The caller
**		now waits for archiver processing with a normal
**		online_wait_for_purge call.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
*/
static DB_STATUS
online_write_ebackup(
DMF_JSX	    *jsx)
{
    DB_STATUS		status;
    LG_LSN		lsn;

    CLRDBERR(&jsx->jsx_dberr);

    for (;;)
    {
	/*
	** Write End Backup record.  All updates to the database made
	** between the writing of the Begin and End Backup records must
	** be logged to the DUMP file.
	*/

	status = dm0l_ebackup(jsx->jsx_log_id, &jsx->jsx_db_owner, 
			&lsn, &jsx->jsx_dberr);
	if (status != E_DB_OK)
	{
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM1129_CPP_DM0LEBACKUP);
	    break;
	}

	if (jsx->jsx_status & JSX_TRACE)
	{
	    TRdisplay(
		"%@ CPP: Wrote End Backup Log Record at Log Seq Num <%x,%x>\n",
		lsn.lsn_high, lsn.lsn_low);
	}

	return(E_DB_OK);
    }
    return(E_DB_ERROR);
}

/*{
** Name: online_dbackup
**
** Description:
**	Inform the logging system that the backup is complete. The logging
**	system can appropriately update its data structures.
**
** Inputs:
**      jsx				Pointer to checkpoint context.
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
**      19-aug-1992 (Ananth)
**          Created for backup rewrite.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	24-jun-96 (nick)
**	    Remove unused variables and correct type mismatches.
*/
static DB_STATUS
online_dbackup(
DMF_JSX	    *jsx)
{
    STATUS		cl_status;
    CL_ERR_DESC		sys_err;
    i4			*err_code = &jsx->jsx_dberr.err_code;

    CLRDBERR(&jsx->jsx_dberr);

    /* Signal the logging system that the backup is complete. */

    cl_status = LGalter(
	LG_A_DBACKUP, (PTR)&jsx->jsx_log_id, sizeof(jsx->jsx_log_id), &sys_err);

    if (cl_status != OK)
    {
	uleFormat(NULL, E_DM900B_BAD_LOG_ALTER, &sys_err, ULE_LOG, NULL, 
	    (char *)NULL, (i4)0, (i4 *)NULL, err_code,
	    1, 0, LG_A_DBACKUP);
	SETDBERR(&jsx->jsx_dberr, 0, E_DM1139_CPP_DBACKUP);
	return (E_DB_ERROR);
    }


    if (jsx->jsx_status & JSX_TRACE)
	TRdisplay("%@ CPP: Successfully Notify LG via LG_A_DBACKUP\n");

    return (E_DB_OK);
}

/*{
** Name: online_finish
**
** Description:
**	Release the cluster checkpoint lock and the DBU operation lock.
**
** Inputs:
**      jsx				Pointer to checkpoint context.
**	dcb				Pointer to DCB for the database.
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
**      19-aug-1992 (Ananth)
**          Created for backup rewrite.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	24-jun-96 (nick)
**	    Remove unused variables and correct type mismatches.
*/
static DB_STATUS
online_finish(
DMF_JSX	    *jsx,
DMP_DCB	    *dcb)
{
    STATUS		cl_status;
    CL_ERR_DESC		sys_err;
    i4			*err_code = &jsx->jsx_dberr.err_code;

    CLRDBERR(&jsx->jsx_dberr);

    /* Release the DBU operation lock. */

    cl_status = LKrelease(0, dmf_svcb->svcb_lock_list, &jsx->jsx_lockid,
	    (LK_LOCK_KEY *)0, (LK_VALUE * )0, &sys_err); 
    if (cl_status != OK)
    {
	uleFormat(NULL, E_DM901B_BAD_LOCK_RELEASE, &sys_err, ULE_LOG, NULL,
	    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	SETDBERR(&jsx->jsx_dberr, 0, E_DM1131_CPP_LK_FAIL);
	return (E_DB_ERROR);
    }

    if (jsx->jsx_status & JSX_TRACE)
	TRdisplay("%@ CPP: Successfully Released DBU lock\n");

    return (E_DB_OK);
}

/*{
** Name: check_point	- Check point the database.
**
** Description:
**      This routine manages the actual generation of the checkpoints
**	files for each location.  Locations that are aliases are detected
**	and only generated once.
**
** Inputs:
**      cpp                             Pointer to checkpoint context
**	jsx				Pointer to Journal support info.
**      dcb				Pointer to DCB for this database.
**	config				Address of the caller's config structure
**	ckp_sequence			Checkpoint set sequence.
**
** Outputs:
**	config				Caller's config is cleared when we close
**					the config file.
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
**	 1-feb-1989 (EdHsu)
**	    Modify to support online backup.
**	 4-may-1989 (Sandyh)
**	    Cleaned up start message for UNIX syntax. bug 4630
**	23-oct-1989 (rogerk)
**	    Added new routine to delete only alias files.  Use this instead
**	    of old 'delete_files' routine when using online backup.
**	 1-dec-1989 (rogerk)
**	    If checkpoint stall flag is on, then stall for one minute before
**	    and after the backup phase.  This allows online checkpoint tests
**	    to stress the system more.
**	19-nov-1990 (bryanp)
**	    We are called with the config file open and we close it in this
**	    routine. When we close it, we must update our CALLER'S config
**	    pointer as well as our local pointer so that our caller knows that
**	    the config file is no longer open (otherwise he might erroneously
**	    try to close it again, resulting in spurious errors).
**	04-nov-92 (jrb)
**	    Renamed DCB_SORT to DCB_WORK and added DCB_AWORK.
**      11-dec-1991 (bentley, pholman)
**          ICL DRS 6000's require NO_OPTIM to prevent the complier failing
**	    during code generation: compiler error: "no table entry for op =
**          Fatal error in /usr/ccs/lib/cg,  Status 06.
**	21-dec-1992 (robf)
**	    Altering journaling is security relevent, so, make sure we 
**	    audit changes to the journaling status of database/tables.
**	21-jun-1993 (rogerk)
**	    Fix prototype warnings.
**	20-sep-1993 (jnash)
**	    Fix up TRformat() and TRdisplay() of checkpoint information.
**	20-jun-1994 (jnash)
**	    Add DM1318 to simulate CKsave error.
**      07-jan-1995 (alison)
**          Partial checkpoint.
**      26-jan-1995 (stial01)
**          BUG 66482,66519: moved call to cpp_prepare_save_tbl() to dmfcpp().
**      11-sep-1995 (thaju02)
**	    Database level checkpoint to create a file list of all
**	    tables in database at time of checkpoint.
**	23-june-1998(angusm)
**	    bug 82736: change interface to dmckp_init(), add direction.
**	    check that checkpoint template owned by user 'ingres'
**	21-oct-1999 (hayek02)
**	    17-Aug-1998 (wanya01)
**	    Bug 39563: update configuration file in dump directory with
**	    alias information.
*/
static	DB_STATUS
check_point(
DMF_CPP     *cpp,
DMF_JSX	    *jsx,
DMP_DCB	    *dcb,
DM0C_CNF    **config,
i4	    ckp_sequence)
{
    DMP_EXT		*ext = dcb->dcb_ext;
    DM0C_CNF		*cnf = *config;
    i4             i;
    i4		cpp_loc = 0;
    i4		sequence = 0;
    DB_STATUS		status;
    CL_ERR_DESC		sys_err;
    DI_IO		di_file;
    DM_FILE		filename;
    DM_FILE		alias_file;
    char		line_buffer[132];
    DB_STATUS		(*delete_routine)();
    DMCKP_CB            *d = &cpp->cpp_dmckp;
    CPP_LOC_MASK        *lm  = cpp->cpp_loc_mask_ptr;
    i4			dev_in_use = 0;
    PTR			tptr;
    char		tbuf[2*MAX_LOC];
    char		username[DB_MAXNAME+1];
    char		*cp = &username[0];
    i4			*err_code = &jsx->jsx_dberr.err_code;

    CLRDBERR(&jsx->jsx_dberr);

    /*
    **	Loop through the list of locations deleting any alias files that might
    **	be present.
    */

    for (i = 0; i < ext->ext_count; i++)
    {
	/*  Skip journal, dump, work, and checkpoint locations. */

	if (ext->ext_entry[i].flags & (DCB_RAW | DCB_JOURNAL | DCB_DUMP | 
				DCB_CHECKPOINT | DCB_WORK | DCB_AWORK))
	    continue;

	/*  Turn off any leftover alias flags. */

	cnf->cnf_ext[i].ext_location.flags &= ~DCB_ALIAS;

	/*
	** Delete junk or leftover alias files in the directory.
	** If online checkpoint, then we can't delete junk from the directory
	** as it may be junk that is being used.
	*/
	if (jsx->jsx_status & JSX_CKPXL)
	    delete_routine = delete_files;
	else
	    delete_routine = delete_alias_files;

	status = DIlistfile(&di_file, ext->ext_entry[i].physical.name,
	    ext->ext_entry[i].phys_length, delete_routine, 
	    (PTR) &ext->ext_entry[i], &sys_err);

	if (status == DI_ENDFILE)
	    continue;

	{
	    char	message[256];
	    i4	l_message;
	    i4  	local_err_code;
	    i4	count;

	    status = uleFormat(NULL, status, &sys_err, ULE_LOOKUP, NULL, message,
		sizeof(message), &l_message, &local_err_code, 0);
	    if (status == E_DB_OK)
	    {
		SIwrite(l_message, message, &count, stderr);
		SIwrite(1, "\n", &count, stderr);
	    }
	    status = uleFormat(NULL, E_DM1114_CPP_LOCATION, NULL, ULE_LOOKUP, NULL, 
		message, sizeof(message), &l_message, &local_err_code, 2,
		sizeof(ext->ext_entry[i].logical), &ext->ext_entry[i].logical,
		ext->ext_entry[i].phys_length, &ext->ext_entry[i].physical);
	    if (status == E_DB_OK)
	    {
		SIwrite(l_message, message, &count, stderr);
		SIwrite(1, "\n", &count, stderr);
	    }
	}
	SETDBERR(&jsx->jsx_dberr, 0, E_DM1100_CPP_BAD_LOCATION);
	return (E_DB_ERROR);
    }

    /*	Build alias file name. */

    alias_file = *(DM_FILE *)"zzzz0000.ali";
    alias_file.name[4] = ((ckp_sequence / 1000) % 10) + '0';
    alias_file.name[5] = ((ckp_sequence / 100) % 10) + '0';
    alias_file.name[6] = ((ckp_sequence / 10) % 10) + '0';
    alias_file.name[7] = (ckp_sequence % 10) + '0';

    /*	Now create new alias files to detect location aliasing. */

    for (i = 0; i < ext->ext_count; i++)
    {
	/*  Skip journal, dump, work and checkpoint locations. */

	if (ext->ext_entry[i].flags & ( DCB_RAW | DCB_JOURNAL | DCB_CHECKPOINT | 
				       DCB_DUMP | DCB_WORK | DCB_AWORK))
	    continue;

	/*  Create alias files in locations. */

	status = DIcreate(&di_file, (char *)&ext->ext_entry[i].physical,
	    ext->ext_entry[i].phys_length, (char *)&alias_file, 
	    sizeof(alias_file), DM_PG_SIZE, &sys_err);
	if (status == OK)
	{
	    cpp_loc++;
	    continue;
	}
	if (status == DI_EXISTS)
	{
	    ext->ext_entry[i].flags |= DCB_ALIAS;
	    cnf->cnf_ext[i].ext_location.flags |= DCB_ALIAS;
	    continue;
	}

	uleFormat(NULL, E_DM9002_BAD_FILE_CREATE, &sys_err, 
                   ULE_LOG, NULL, 0, 0, 0, err_code, 4,
	    sizeof(dcb->dcb_name), &dcb->dcb_name,
	    4, "None",
	    ext->ext_entry[i].phys_length, &ext->ext_entry[i].physical,
	    sizeof(alias_file), &alias_file);
	SETDBERR(&jsx->jsx_dberr, 0, E_DM1100_CPP_BAD_LOCATION);
	return (E_DB_ERROR);
    }

    /*	Update configuration file with alias information. */

    status = config_handler(UPDATE_CONFIG, dcb, &cnf, jsx);

    if (status != E_DB_OK)
	return (status);

    status = config_handler(DUMP_CONFIG, dcb, &cnf, jsx);

    /* Close the config file but keep the extent info in the dcb. */

    if (status == E_DB_OK)
        status = config_handler(CLOSEKEEP_CONFIG, dcb, &cnf, jsx);
    if (status != E_DB_OK)
	return (status);

    /*
    ** Now that the config file is closed, propagate this information up to
    ** our caller so that he knows that the config file is closed. The
    ** config_handler(CLOSEKEEP_CONFIG) call will have set our local cnf
    ** variable to 0 to indicate that it closed the config file, and we return
    ** the favor to our caller.
    */

    *config = (DM0C_CNF *)0;

    filename.name[0] = 'c';
    filename.name[1] = (ckp_sequence / 1000) + '0';
    filename.name[2] = ((ckp_sequence / 100) % 10) + '0';
    filename.name[3] = ((ckp_sequence / 10) % 10) + '0';
    filename.name[4] = (ckp_sequence % 10) + '0';
    filename.name[8] = '.';
    filename.name[9] = 'c';
    filename.name[10] = 'k';
    filename.name[11] = 'p';

    /* Initialize the DMCKP_CB structure */
    status = dmckp_init(d, DMCKP_SAVE, err_code);
    if (status != E_DB_OK)
    {
	SETDBERR(&jsx->jsx_dberr, 0, *err_code);

	if (jsx->jsx_dberr.err_code == I_DM116D_CPP_NOT_DEF_TEMPLATE)
	{
	    i4	l; 
    	    char defowner[DB_MAXNAME+1]; /* effective user of process */
	    char *def_cp = &defowner[0];
	    
	    MEfill(sizeof(tbuf), '\0', tbuf);
	    MEfill(sizeof(username), '\0', username);
	    LOtos(&(d->dmckp_cktmpl_loc), &tptr);
            l = STlength(tptr);
	    MEcopy(tptr, l, tbuf);
	    IDname(&cp);

#ifdef CK_OWNNOTING
	    /* effective user of process */
	    (VOID)IDename(&def_cp);
#else
	    STcopy(def_cp, "ingres");
#endif

	    uleFormat(&jsx->jsx_dberr, 0, NULL, ULE_LOG, NULL, 
		    NULL, 0, NULL, err_code, 3,
		    sizeof(dcb->dcb_name), &dcb->dcb_name,
		    STlength(username), &username,
		    l, &tbuf);

	    TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
		"%@ CPP: Template file %t not owned by user %s\n",
		l, tbuf, defowner);
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM1121_CPP_BEGIN_ERROR);
        }

	return (status);
    }
    /*
    ** Setup the CK information which will remain constant across the
    ** checkpoint of each location. Initialize the other fields...
    */
    d->dmckp_ckp_file      = filename.name;
    d->dmckp_ckp_l_file    = sizeof(filename);
  
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
        d->dmckp_ckp_path    = dcb->dcb_ckp->physical.name;
        d->dmckp_ckp_l_path  = dcb->dcb_ckp->phys_length;
        d->dmckp_device_type = DMCKP_DISK_FILE;
        d->dmckp_dev_cnt     = jsx->jsx_dev_cnt;
    }

    /*
    **	Start checkpointing each location.  Start with the root location.
    */
    if (jsx->jsx_status1 & JSX1_CKPT_DB)
    {
	d->dmckp_op_type       = DMCKP_SAVE_DB;
	d->dmckp_num_locs = cpp_loc; 
	status = dmckp_begin_save_db(d, dcb, err_code);
	if ( status )
	    SETDBERR(&jsx->jsx_dberr, 0, *err_code);
    }
    else
    {
	d->dmckp_op_type       = DMCKP_SAVE_FILE;

	/* 
	** Init dmckp_num_files: total # files being processed
	** Init dmckp_num_locs: total # locations we are interested in
	*/
	d->dmckp_num_files = 0;
	d->dmckp_num_locs = 0;
	for (i = 0; i < ext->ext_count; i++)
	{
	    if ( (jsx->jsx_status1 & JSX1_CKPT_DB) == 0)
	    {
		if ( BTtest( i, (char *)lm->locm_r_allow ) == FALSE )
		    continue;
		d->dmckp_num_locs++;
		d->dmckp_num_files += cpp->cpp_fcnt_at_loc[i];
	    }
	}
	if (status == E_DB_OK)
	{
	    status = dmckp_begin_save_tbl(d, dcb, err_code);
	    if ( status )
		SETDBERR(&jsx->jsx_dberr, 0, *err_code);
	}
    }

    if (status != OK)
    {
	uleFormat( &jsx->jsx_dberr, 0, &sys_err,
			ULE_LOG, NULL, NULL, 0, NULL, err_code, 0);
	if (jsx->jsx_dberr.err_code != E_DM1155_CPP_TMPL_MISSING)
	{
	    uleFormat(NULL, E_DM1104_CPP_CKP_ERROR, &sys_err, 
			ULE_LOG, NULL, NULL, 0, NULL, err_code, 0);
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM1121_CPP_BEGIN_ERROR);
	}
	return (E_DB_ERROR);
    }

    /*
    ** If checkpoint stall flag is on, then stall here for 1 minute.
    ** This allows online checkpoint tests to stress the system more.
    */
    if (jsx->jsx_status & JSX_CKP_STALL)
	PCsleep(60000);

    for (i = 0; i < ext->ext_count; i++)
    {

	/*  
	**  Skip Journal, Dump and Work, locations.
	**  Also skip Alias locations if checkpointing
	**  whole database.
	*/

	if ((jsx->jsx_status1 & JSX1_CKPT_DB) && 
	    (ext->ext_entry[i].flags & (DCB_JOURNAL| DCB_CHECKPOINT | 
					DCB_ALIAS | DCB_DUMP | 
					DCB_WORK | DCB_AWORK)))
	    continue;

	/*
	**  Don't Skip Alias locations if only checkpointing a table.
	*/

	else if (ext->ext_entry[i].flags & (DCB_JOURNAL| DCB_CHECKPOINT | 
			       		    DCB_DUMP | DCB_WORK | DCB_AWORK))
	    continue;
 
	/*  Construct filename for checkpoint. */

	sequence++;

	/*
	** If table backup, skip locations which are not needed 
	*/
	if ( (jsx->jsx_status1 & JSX1_CKPT_DB) == 0)
	{
	    if ( BTtest( i, (char *)lm->locm_r_allow ) == FALSE )
		continue;
	}

	filename.name[5] = (sequence / 100) + '0';
	filename.name[6] = ((sequence / 10) % 10) + '0';
	filename.name[7] = (sequence % 10) + '0';

	if (jsx->jsx_status & JSX_TRACE)
	{
	    TRdisplay("%@ CPP: Start checkpoint of location: %~t:\n",
		sizeof(ext->ext_entry[i].logical), &ext->ext_entry[i].logical);

            TRdisplay("%4* path = '%t'\n%4* file = '%~t'\n",
                dcb->dcb_ckp->phys_length, &dcb->dcb_ckp->physical,
                sizeof(filename), &filename);
	}

	/*
	**  Get a free device 
	*/
	status = cpp_wait_free(jsx, d);
	if (status != OK)
	{
	    uleFormat(NULL, E_DM1104_CPP_CKP_ERROR, &sys_err,
		       ULE_LOG, NULL, NULL, 0, NULL, err_code, 0);
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM1101_CPP_WRITE_ERROR);
	    return (E_DB_ERROR);
	}
	if (jsx->jsx_status & JSX_DEVICE)
	{
	    TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
		"%@ CPP: Start checkpoint of location: %~t to tape:\n",
		sizeof(ext->ext_entry[i].logical), &ext->ext_entry[i].logical);

            TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
                "%4* device = '%t'\n%4* file = '%~t'\n",
		d->dmckp_ckp_l_path, d->dmckp_ckp_path,
                sizeof(filename), &filename);
	}
	else
	{
	    d->dmckp_ckp_path     = dcb->dcb_ckp->physical.name;
	    d->dmckp_ckp_l_path   = dcb->dcb_ckp->phys_length;
	    TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
		"%@ CPP: Start checkpoint of location: %~t to disk:\n",
		sizeof(ext->ext_entry[i].logical), &ext->ext_entry[i].logical);

            TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
                "%4* path = '%t'\n%4* file = '%~t'\n",
                dcb->dcb_ckp->phys_length, &dcb->dcb_ckp->physical,
                sizeof(filename), &filename);
	}

	/*
	** Set up the directory path to the data location
	*/
	*(ext->ext_entry[i].physical.name + ext->ext_entry[i].phys_length) =
 '\0';
	d->dmckp_di_path   = ext->ext_entry[i].physical.name;
	d->dmckp_di_l_path = ext->ext_entry[i].phys_length;


	d->dmckp_raw_start = ext->ext_entry[i].raw_start;
	d->dmckp_raw_blocks = ext->ext_entry[i].raw_blocks;
	d->dmckp_raw_total_blocks = ext->ext_entry[i].raw_total_blocks;

	/* Call the checkpoint routine */
	if (jsx->jsx_status1 & JSX1_CKPT_DB)
	{
	    status = dmckp_pre_save_loc(d, dcb, err_code);
	    if (status != OK)
	    {	
		SETDBERR(&jsx->jsx_dberr, 0, *err_code);

		uleFormat(NULL, E_DM1104_CPP_CKP_ERROR, &sys_err,
			   ULE_LOG, NULL, NULL, 0, NULL, err_code, 0);
		SETDBERR(&jsx->jsx_dberr, 0, E_DM1101_CPP_WRITE_ERROR);
		return (E_DB_ERROR);
	    }
	    status = dmckp_save_loc(d, dcb, err_code);
	}
	else
	{
	    d->dmckp_num_files_at_loc = cpp->cpp_fcnt_at_loc[i];
	    d->dmckp_di_file = cpp->cpp_fnames_at_loc[i]->cpp_fnms_str;
	    d->dmckp_di_l_file = cpp->cpp_fcnt_at_loc[i] * (sizeof(DM_FILE) +1);
	    status = dmckp_pre_save_file(d, dcb, err_code);
	    if (status != OK)
	    {
		SETDBERR(&jsx->jsx_dberr, 0, *err_code);

		uleFormat(NULL, E_DM1104_CPP_CKP_ERROR, &sys_err,
			   ULE_LOG, NULL, NULL, 0, NULL, err_code, 0);
		SETDBERR(&jsx->jsx_dberr, 0, E_DM1101_CPP_WRITE_ERROR);
		return (E_DB_ERROR);
	    }
	    status = dmckp_save_file(d, dcb, err_code);
	    if ( status )
		SETDBERR(&jsx->jsx_dberr, 0, *err_code);
	    /* FIX ME ok to deallocate CPP_FNMS for this loc */
	}

#ifdef xDEBUG
	/*
	** DM1318 simulates CKsave error.
	*/
	if (DMZ_ASY_MACRO(18))
	{
	    /* sys_err is random but no matter for this test */
	    status = CK_ENDFILE;
	}
#endif

	if (status != OK)
	{
	    uleFormat(NULL, E_DM1104_CPP_CKP_ERROR, &sys_err, 
		       ULE_LOG, NULL, NULL, 0, NULL, err_code, 0);
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM1101_CPP_WRITE_ERROR);
	    return (E_DB_ERROR);
	}
    }

    /*
    ** If using multiple devices, wait for last device to finish.
    */

    status = cpp_wait_all(jsx);
    if (status != OK)
    {
	uleFormat(NULL, E_DM1104_CPP_CKP_ERROR, &sys_err, 
		   ULE_LOG, NULL, NULL, 0, NULL, err_code, 0);
	SETDBERR(&jsx->jsx_dberr, 0, E_DM1101_CPP_WRITE_ERROR);
	return (E_DB_ERROR);
    }
    /*
    ** If checkpoint stall flag is on, then stall here for 1 minute.
    ** This allows online checkpoint tests to stress the system more.
    */
    if (jsx->jsx_status & JSX_CKP_STALL)
	PCsleep(60000);

    if (jsx->jsx_status1 & JSX1_CKPT_DB)
    {
	status = dmckp_end_save_db(d, dcb, err_code);
	if ( status )
	    SETDBERR(&jsx->jsx_dberr, 0, *err_code);

        if (status == OK)
        {
            status = cpp_save_chkpt_tbl_list(cpp, jsx, dcb, ckp_sequence);
        }

    }
    else
    {
	status = dmckp_end_save_tbl(d, dcb, err_code);
	if ( status )
	    SETDBERR(&jsx->jsx_dberr, 0, *err_code);

	if (status == OK)
	{
	    status = cpp_save_chkpt_tbl_list(cpp, jsx, dcb, ckp_sequence);
	} 
    }

    if (status != OK)
    {
	uleFormat(NULL, E_DM1104_CPP_CKP_ERROR, &sys_err, 
			ULE_LOG, NULL, NULL, 0, NULL, err_code, 0);
	SETDBERR(&jsx->jsx_dberr, 0, E_DM1122_CPP_END_ERROR);
	return (E_DB_ERROR);
    }

    /*	Delete any alias file created during the checkpoint process. */

    for (i = 0; i < ext->ext_count; i++)
    {
	/*  Skip journal, dump,work and checkpoint locations. */

	if (ext->ext_entry[i].flags & (DCB_RAW | DCB_JOURNAL | DCB_CHECKPOINT | 
				       DCB_DUMP | DCB_WORK | DCB_AWORK))
	    continue;

	status = DIdelete(&di_file, (char *)&ext->ext_entry[i].physical,
	    ext->ext_entry[i].phys_length, (char *)&alias_file, 
	    sizeof(alias_file), &sys_err);
	if (status == OK || status == DI_FILENOTFOUND)
	    continue;

	uleFormat(NULL, E_DM9003_BAD_FILE_DELETE, &sys_err, 
                   ULE_LOG, NULL, NULL, 0, NULL, err_code, 4,
	    sizeof(dcb->dcb_name), &dcb->dcb_name,
	    4, "None",
	    ext->ext_entry[i].phys_length, &ext->ext_entry[i].physical,
	    sizeof(filename), &filename);
	SETDBERR(&jsx->jsx_dberr, 0, E_DM1100_CPP_BAD_LOCATION);
	return (E_DB_ERROR);
    }

    /*	Release the extent information. */

    status = config_handler(UNKEEP_CONFIG, dcb, &cnf, jsx);
    if (status != E_DB_OK)
	return (status);

    return (E_DB_OK);
}

/*{
** Name: upd1_jnl_dmp	- Update table journaling/dumping status.
**
** Description:
**      This routine updates the configuration file maintained history
**	of checkpoints.
**
** Inputs:
**      journal_context			Pointer to DMF_JSX.
**	dcb				Pointer to DCB.
**	cnf				Pointer to CNF pointer.
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
**	13-oct-1989 (Sandyh)
**	    Fixed jnl_history & dmp_history indexing, bug 8352.
**	16-oct-1989 (rogerk)
**	    Store correct checkpoint sequence number in the online dump
**	    structure and history. Use the journal checkpoint sequence
**	    number rather than keeping an independent count.
**	10-jan-1990 (rogerk)
**	    Changed algorithms for how to get correct journal records written
**	    to the old/new journal files.  If Online Checkpoint of a journaled
**	    database, then don't create the new journal history entry or
**	    increment the journal file sequence number here - it will be done
**	    in the archiver when the SBACKUP record is encountered.  Signal
**	    a CP and a PURGE event after writing the SBACKUP record to ensure
**	    that the archiver will purge the database up to the SBACKUP record
**	    during the checkpoint.  Do not complete checkpoint processing
**	    until the PURGE has compeleted.  This way we know that the old
**	    journal records have been flushed to the old journal files.
**
**	    Change initial setting of the jnl_fil_seq value in the config
**	    file.  It is initialized to 1 - which is the next journal file to
**	    use.  When a checkpoint is executed, the jnl_fil_seq will be
**	    incremented (in dmfcpp if offline or non-journaled datbase - in
**	    archiver if online and journaled) if the current journal file #
**	    was used by the previous checkpoint.  If it wasn't (database was
**	    not journaled) then we don't increment the value - we just use
**	    the current jnl file sequence.
**	18-jan-1990 (rogerk)
**	    Zero out the last-dumped address when starting an online backup.
**	    This ensures that we won't get messed up by a garbage value, or
**	    a value from another node on the cluster, that will cause us not
**	    to save any dump information.
**	20-nov-1990 (bryanp)
**	    When resetting the cluster node info, clear the cnode_la field to
**	    zero. Recently, a customer ran into a situation in which their
**	    log file addresses decreased. Detection of the problem was hampered
**	    because the cluster node info history reported a high log address.
**	    Resettting the log address to zero ensures that an accurate value
**	    will be available in this field (i.e., that code won't be confused
**	    by a 'stale' value).
**	 3-may-1991 (rogerk)
**	    Backout change to clear the cnode_la field in the cluster journal
**	    history after a checkpoint is performed.  Clearing this loses
**	    the knowledge of how far this node's log file has been archived.
**      24-jan-1995 (stial01)
**          BUG 66473: indicate in ckp_mode if checkpoint is table checkpoint.
**      07-Feb-1995 (liibi01)
**          Bug 48500, if checkpoint sequence is greater than 9999, loop back
**          to 1.
*/
static DB_STATUS
upd1_jnl_dmp(
DMF_JSX	    *journal_context,
DMP_DCB	    *dcb,
DM0C_CNF    **config)
{
    DMF_JSX		*jsx = journal_context;
    DM0C_CNF            *cnf = *config;
    DB_STATUS		status;
    i4		j;

    CLRDBERR(&jsx->jsx_dberr);

    /*
    ** If this is the first checkpoint on the database, then initialize the
    ** journal structures.
    */
    if (cnf->cnf_jnl->jnl_count == 0)
    {
	cnf->cnf_jnl->jnl_ckp_seq = 1;
	cnf->cnf_jnl->jnl_fil_seq = 1;
	cnf->cnf_jnl->jnl_blk_seq = 0;
	cnf->cnf_jnl->jnl_update = 0;
    }
    else
    {
       if (++cnf->cnf_jnl->jnl_ckp_seq > 9999)
                cnf->cnf_jnl->jnl_ckp_seq = 1;

    }

    /*
    ** Ensure that the journal file sequence number is at least 1.  Pre 6.3
    ** checkpoints may have left jnl_count updated, but not jnl_fil_seq.
    */
    if (cnf->cnf_jnl->jnl_fil_seq <= 0)
	cnf->cnf_jnl->jnl_fil_seq = 1;

    /*
    ** If this is OFFLINE checkpoint or a non-journaled database, then create a
    ** journal history entry for the new checkpoint.  Also update the journal
    ** file sequence number so we will create a new journal file the next time
    ** we archive journaled records.
    **
    ** If this an ONLINE checkpoint, then we can't create a new journal
    ** history entry yet as it may mess up ongoing archiver processing
    ** of journaled updates.  The new journal history entry will be created
    ** by the archiver during online backup processing when it is finished
    ** processing all journal records that occured before the checkpoint.   
    **
    ** If this is a cluster online checkpoint ckpdb will create the new journal
    ** history (instead of the archiver)
    */
    if ((jsx->jsx_status & JSX_CKPXL) || CXcluster_enabled () ||
	(cnf->cnf_dsc->dsc_status & DSC_JOURNAL) == 0)
    {
	/*
	** Increment the count of entries in the journal history array.
	** If we exceed the limit, toss the oldest entry by copying all
	** entries up one level - leaving the last entry free.
	*/
	if (++cnf->cnf_jnl->jnl_count > CKP_HISTORY)
	{
	    MEcopy((char *)&cnf->cnf_jnl->jnl_history[1],
		(CKP_HISTORY - 1) * sizeof(struct _JNL_CKP),
		(char *)&cnf->cnf_jnl->jnl_history[0]); 
	    cnf->cnf_jnl->jnl_count = CKP_HISTORY;
	}

	/*
	** Initialize the new journal history entry.  Leave the associated
	** journal file numbers (ckp_f_jnl/ckp_l_jnl) zero until we actually
	** create a journal file in the archiver.
	*/
	j = cnf->cnf_jnl->jnl_count - 1;
	cnf->cnf_jnl->jnl_history[j].ckp_sequence = cnf->cnf_jnl->jnl_ckp_seq;
	cnf->cnf_jnl->jnl_history[j].ckp_f_jnl = 0;
	cnf->cnf_jnl->jnl_history[j].ckp_l_jnl = 0;
	cnf->cnf_jnl->jnl_history[j].ckp_jnl_valid = 0;
	if (jsx->jsx_status & JSX_CKPXL)
	    cnf->cnf_jnl->jnl_history[j].ckp_mode = CKP_OFFLINE;
	else
	    cnf->cnf_jnl->jnl_history[j].ckp_mode = CKP_ONLINE;

	/* BUG 66473, indicate if this is table checkpoint */
	if ((jsx->jsx_status1 & JSX1_CKPT_DB) == 0)
	    cnf->cnf_jnl->jnl_history[j].ckp_mode |= CKP_TABLE;

	TMget_stamp((TM_STAMP *) &cnf->cnf_jnl->jnl_history[j].ckp_date);

	/*
	** This new checkpoint requires a new journal file.  
	** Check first to see if the previous checkpoint actually created a
	** journal file with the current sequence number.  If it didn't, (or if
	** there is no previous checkpoint) then we can just reuse that
	** sequence number.
	**
	** Note that we bump the file sequence number even if the database is
	** not journaled - but only if the previous checkpoint created a
	** journal file.  If this database was not journaled before this
	** checkpoint, then there should have been no journal file created.
	*/
	if ((j != 0) && (cnf->cnf_jnl->jnl_history[j - 1].ckp_f_jnl != 0))
	{
	    cnf->cnf_jnl->jnl_fil_seq++;
	    cnf->cnf_jnl->jnl_blk_seq = 0;
	}

	/*
	** If this is a cluster installation, then reset the journal/dump
	** node information. 
	**
	** The journaled log address value (cnode_la) is still needed
	** however.  This records the place in this node's log file to
	** which we have completed archiving log records for the database.
	** The next time archiving is needed from this node, we will start
	** processing at that spot.
	*/
	if (CXcluster_enabled())
	{
	    for (j = 0; j < DM_CNODE_MAX; j++)
	    {
		cnf->cnf_jnl->jnl_node_info[j].cnode_fil_seq = 0;
		cnf->cnf_jnl->jnl_node_info[j].cnode_blk_seq = 0;

		cnf->cnf_dmp->dmp_node_info[j].cnode_fil_seq = 0;
		cnf->cnf_dmp->dmp_node_info[j].cnode_blk_seq = 0;
	    }
	}
    }

    /*
    ** If this is an Online Checkpoint, then update the DUMP history
    ** information.  Create a new entry in the dump history array, and
    ** record that the checkpoint requires dump processing.
    */
    if ((jsx->jsx_status & JSX_CKPXL) == 0)
    {
	/* 
	** Update the dump information.
	** If this is the first entry, then initialize the values.
	*/
	cnf->cnf_dsc->dsc_status |= DSC_DUMP;
	if (cnf->cnf_dmp->dmp_count == 0)
	{
	    cnf->cnf_dmp->dmp_fil_seq = 0;
	    cnf->cnf_dmp->dmp_update = 0;
	}

	cnf->cnf_dmp->dmp_fil_seq++;
	cnf->cnf_dmp->dmp_blk_seq = 0;
	cnf->cnf_dmp->dmp_ckp_seq = cnf->cnf_jnl->jnl_ckp_seq;

	/*
	** Zero out the address of the last dumped record - as we are
	** starting a new dump.  We don't want cluster installations
	** to get messed up by using conflicting values of dump addresses
	** from two different log files.
	*/
	cnf->cnf_dmp->dmp_la.la_sequence = 0;
	cnf->cnf_dmp->dmp_la.la_block    = 0;
	cnf->cnf_dmp->dmp_la.la_offset   = 0;

	/*
	** Increment the count of entries in the dump history array.
	** If we exceed the limit, toss the oldest entry by copying all
	** entries up one level - leaving the last entry free.
	*/
	if (++cnf->cnf_dmp->dmp_count > CKP_HISTORY)
	{
	    MEcopy((char *)&cnf->cnf_dmp->dmp_history[1],
		(CKP_HISTORY - 1) * sizeof(struct _DMP_CKP),
		(char *)&cnf->cnf_dmp->dmp_history[0]); 
	    cnf->cnf_dmp->dmp_count = CKP_HISTORY;
	}

	/*
	** Initialize the new dump history entry.
	*/
	j = cnf->cnf_dmp->dmp_count - 1;
	cnf->cnf_dmp->dmp_history[j].ckp_mode = CKP_ONLINE;

	/* BUG 66473, indicate if this is table checkpoint */
	if ((jsx->jsx_status1 & JSX1_CKPT_DB) == 0)
	    cnf->cnf_dmp->dmp_history[j].ckp_mode |= CKP_TABLE;

	cnf->cnf_dmp->dmp_history[j].ckp_sequence = cnf->cnf_jnl->jnl_ckp_seq;
	cnf->cnf_dmp->dmp_history[j].ckp_f_dmp = 0;
	cnf->cnf_dmp->dmp_history[j].ckp_l_dmp = 0;
	cnf->cnf_dmp->dmp_history[j].ckp_dmp_valid = 0;
	TMget_stamp((TM_STAMP *) &cnf->cnf_dmp->dmp_history[j].ckp_date);
    }

    /*  Update configuration file. */

    status = config_handler(UPDATE_CONFIG, dcb, config, jsx);
    if (status != E_DB_OK)
    {
	dmfWriteMsg(&jsx->jsx_dberr, 0, 0);
	SETDBERR(&jsx->jsx_dberr, 0, E_DM1140_CPP_UPD1_JNLDMP);
	return (E_DB_ERROR);
    }

    return (E_DB_OK);
}

/*{
** Name: upd2_jnl_dmp	- Update table journaling/dumping status.
**
** Description:
**      This routine updates the configuration file maintained history
**	of checkpoints.
**
** Inputs:
**      journal_context			Pointer to DMF_JSX.
**	dcb				Pointer to DCB.
**	cnf				Pointer to CNF pointer.
**      sb_time                         Pointer to DB_TAB_TIMESTAMP
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
**      01-feb-1989 (EdHsu)
**          Created for Terminator.
**	13-oct-1989 (Sandyh)
**	    Fixed indexing into history arrays, bug 8352.
**	25-feb-1991 (rogerk)
**	    Turn off disable_journal status if journaling state has
**	    been toggled.  This was done as part of the Archiver Stability
**	    project changes.
**	22-nov-1993 (jnash)
**	    B56095.  Update jnl_first_jnl_seq after a checkpoint that enables 
**	    journaling, reset it if journaling disabled.   This is used 
**	    to catch operator errors attempting to perform rolldb #c from a 
**	    point where the database was not journaled.
**	31-jan-1994 (jnash)
**	    Fix jnl_first_jnl_seq maintenance problems.
**	2-mar-1995 (shero03)
**	    Bug #67104
**	    Don't turn journalling on/off for the databaaase
**	    if this is a partial checkpoint.
*/
static DB_STATUS
upd2_jnl_dmp(
DMF_JSX	    *journal_context,
DMP_DCB	    *dcb,
DM0C_CNF    **config,
DB_TAB_TIMESTAMP  *sb_time)

{
    DMF_JSX		*jsx = journal_context;
    DM0C_CNF            *cnf = *config;
    DB_STATUS		status;
    i4		n;

    CLRDBERR(&jsx->jsx_dberr);

    /*  Now update the DM0C_JNL entry in the configuration file. */
	
    /*	Change the database journal status bits. */
    /*  B67104 - only if this is a whole DB checkpoint */

    if (jsx->jsx_status1 & JSX1_CKPT_DB)
    {
        if (jsx->jsx_status & JSX_JON)
	{
	    cnf->cnf_dsc->dsc_status |= DSC_JOURNAL;
	    cnf->cnf_dsc->dsc_status &= ~DSC_DISABLE_JOURNAL;
	}
	if (jsx->jsx_status & JSX_JOFF)
	{
	    cnf->cnf_dsc->dsc_status &= ~DSC_JOURNAL;
	    cnf->cnf_dsc->dsc_status &= ~DSC_DISABLE_JOURNAL;
	}
    }

    /*
    ** jnl_first_jnl_seq is used to track the first checkpoint associated
    ** with a database on which journaling is enabled (or reenabled) and is 
    ** used by rollforwarddb (#c) +j to detect checkpoints from which a normal 
    ** rollfoward cannot be legally performed (one where the database was 
    ** not journaled at the time of the checkpoint).
    */
    if (cnf->cnf_dsc->dsc_cnf_version >= CNF_V4)
    {
	if (cnf->cnf_dsc->dsc_status & DSC_JOURNAL) 
	{
	    /*
	    ** ckpdb -d or ckpdb +j uses the last checkpoint sequence number.
	    */
	    if ((jsx->jsx_status & JSX_DESTROY) ||
	        (cnf->cnf_jnl->jnl_first_jnl_seq == 0) )
	    {
		cnf->cnf_jnl->jnl_first_jnl_seq = 
		 cnf->cnf_jnl->jnl_history[cnf->cnf_jnl->jnl_count-1].ckp_sequence;
	    }

	    /*
	    ** Check if obsolete because the oldest fell off the end
	    ** of the history array.
	    */
	    if (cnf->cnf_jnl->jnl_first_jnl_seq < 
		 cnf->cnf_jnl->jnl_history[0].ckp_sequence)
	    {
		cnf->cnf_jnl->jnl_first_jnl_seq = 
		    cnf->cnf_jnl->jnl_history[0].ckp_sequence;
	    }
	}

	/*
	** For ckpdb -j, or ckpdb on non-journaled db, there is
	** no valid checkpoint from which a rolldb +j may occur.
	*/
	if ((cnf->cnf_dsc->dsc_status & DSC_JOURNAL) == 0)
	{
	    cnf->cnf_jnl->jnl_first_jnl_seq = 0;
	}
    }

    if ((jsx->jsx_status & JSX_CKPXL) == 0)
    {
	n = cnf->cnf_dmp->dmp_count - 1;

	cnf->cnf_dmp->dmp_history[n].ckp_dmp_valid = 1;
        STRUCT_ASSIGN_MACRO(*sb_time, cnf->cnf_dmp->dmp_history[n].ckp_date);
    }

    n = cnf->cnf_jnl->jnl_count - 1;

    cnf->cnf_jnl->jnl_history[n].ckp_jnl_valid = 1;
    /*
    ** Timestamp to place in the checkpoint journal history is now
    ** if the checkpoint was offline else the time the SBACKUP record
    ** was written for online checkpoints.
    */
    if (jsx->jsx_status & JSX_CKPXL)
    {
        TMget_stamp((TM_STAMP *) &cnf->cnf_jnl->jnl_history[n].ckp_date);
    }
    else
    {
        STRUCT_ASSIGN_MACRO(*sb_time, cnf->cnf_jnl->jnl_history[n].ckp_date);
    }
 
    /*  Update configuration file. */

    status = config_handler(UPDATE_CONFIG, dcb, config, jsx);
    if (status != E_DB_OK)
    {
	dmfWriteMsg(&jsx->jsx_dberr, 0, 0);
	SETDBERR(&jsx->jsx_dberr, 0, E_DM1135_CPP_UPD2_JNLDMP);
	return (E_DB_ERROR);
    }

    return (E_DB_OK);
}

/*{
** Name: update_catalogs	- Update catalog journal status.
**
** Description:
**      This routine turns the journal status bit's on or off in
**	the relation table for each table that needs it.
**
** Inputs:
**      jsx                             Journal support context.
**      dcb                             Pointer to DCB.
**	cnf				Pointer to cnf pointer.
**
** Outputs:
**      err_code                        Reason for error return.
**	Returns:
**	    E_DB_OK
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
**	30-sep-88 (rogerk)
**	    Changed dmxe_begin call to use new arguments.
**	14-apr-1989 (EdHsu)
**	    Online backup cluster support
**	28-nov-1990 (rachael)
**	    Bug 34069:
**	    The relstat of every table was being set (in update_catalogs) 
**   	    to ~TCB_JOURNAL rather than ~TCB_JOURNAL and TCB_JON when 
**	    using the -j flag. 
**	21-dec-1992 (robf)
**	    Altering journaling is security relevent, so, make sure  we 
**	    audit changes to the journaling status of database/tables.
**	15-mar-1993 (rogerk)
**	    Reduced Logging - Phase IV:
**	    Removed dmxe_abort output parameter used to return the log address
**		of the abortsave log record.
**	26-apr-1993 (bryanp)
**	    Fix argument passing to dmxe_abort so that DCB can be passed.
**	24-may-1993 (rogerk)
**	    Temporary fix for journaling of system catalog updates.  Need to
**	    set the rcb_usertab_jnl flag to indicate the the system
**	    catalog updates should be journaled.
**      06-Jan-1995 (nanpr01)
**	    Bug #66028
**          reset the bit TABLE_RECOVERY_DISALLOWED after a successful 
**	    checkpoint. 
**      18-Jan-1995 (nanpr01)
**	    Bug #66439
**          reset the bit TABLE_RECOVERY_DISALLOWED for only those tables 
**	    in the partial checkpoint list. 
**	31-jan-1995 (shero03)
**	    Bug #66545
**	    make sure the TABLE_RECOVERY_DISALLOWED bit is reset
**	    even if the journaling is not being changed.
**	17-jun-96 (nick)
**	    (Substantial) rewrite ; tidied up the logic and fixed problem
**	    where error handling got screwed ( and returned ok on error ).  
**	    We also weren't returning a valid DM0C_CNF pointer on failure
**	    either which caused ACCVIOs later on.  An additional problem
**	    was that we security audited the reset of the recovery disallowed
**	    flag as either switching journaling on or off.
**	03-jan-1997 (nanpr01)
**	    Use the #define's rather than the hard-coded values.
**	24-jun-1998 (nanpr01)
**	    rcb_usertab_jnl must be set correctly based on the user table
**	    journal status. Otherwise rollforwarddb will fail.
**	01-oct-1998 (nanpr01)
**	    Added the new parameter in dm2r_replace call for update performance
**	    enhancement.
**      28-feb-2000 (gupsh01)
**          Enabled check for owner along with the table name
**          so that the table names in -table option can also
**          be specified as owner.tablename .
**      14-jul-2000 (stial01)
**         update_catalogs() rcb_usertab_jnl initialization for b102072.
**	26-jul-2002 (thaju02)
**	    Race condition can occur between update catalogs transaction
**	    which is being pass aborted and a drop table user transaction.
**	    If we are aborting xaction, save xaction lgid in case pass 
**	    abort occurs. (B108377)
**      22-Dec-2004 (garni05) Bug #113524 Prob#INGSRV3057
**          modified 'if(jsx->jsx_status & JSX_JON)' condition so that after
**          'ckpdb +j dbname', is_journalled value in iitables should switch
**          from'C' to 'Y' for a journalled table and from 'N' to 'Y' for
**          foreign/Index keys.Two sub if conditions having similar code have
**          been merged into one.
**      26-Oct-2005 (hanal04) Bug 115455 INGSRV3473
**          Backout the above change. It incorrectly turns journalling on for
**          all tables. It also does not make a difference to the results for
**          the script for bug 113524.
*/
static DB_STATUS
update_catalogs(
DMF_JSX             *jsx,
DMP_DCB		    *dcb,
DM0C_CNF	    **config,
i4		    *abort_xact_lgid)
{
    DMP_RCB		*rcb = 0;
    DM0C_CNF		*cnf = *config;
    i4		status;
    DB_TRAN_ID		tran_id;
    i4		tran_llid;
    i4		tran_lgid = 0;
    i4		dmxe_flag = DMXE_JOURNAL;
    DM_TID		tid;
    DMP_RELATION	relation;
    DB_TAB_TIMESTAMP	timestamp,ctime;
    DB_TAB_ID		table_id;
    char		line_buffer[132];
    DB_STATUS	        local_status;
    i4		local_err;
    SXF_ACCESS          access;
    i4 	        msgid;
    i4		i;
    bool 	        found;
    bool		db_open = FALSE;
# define TUP_JON	0x01
# define TUP_JOFF	0x02
# define TUP_REC_ALLOW	0x04
    u_i2 	        upd_tup;
    LG_DATABASE		database;
    STATUS		cl_status;
    CL_ERR_DESC		sys_err;
    i4			length;
    i4			dsc_status;
    DB_ERROR		local_dberr;
    i4			*err_code = &jsx->jsx_dberr.err_code;

    CLRDBERR(&jsx->jsx_dberr);

    /*	Check whether catalogs need updating. */

    if ( jsx->jsx_status & (JSX_JON | JSX_JOFF) &&
	 dmf_svcb->svcb_status & SVCB_C2SECURE )
    {
	/*
	**    Record we are enabling/disabling journaling for this database.
	*/
	access = (SXF_A_ALTER|SXF_A_SUCCESS);

	if (jsx->jsx_status & JSX_JON)
	    msgid = I_SX2712_CKPDB_ENABLE_JNL;
	else
	    msgid = I_SX2713_CKPDB_DISABLE_JNL;

	status = dma_write_audit( SXF_E_DATABASE,
		access,
		dcb->dcb_name.db_db_name, /* Object name (database) */
		sizeof(dcb->dcb_name.db_db_name), /* Object name (database) */
		&dcb->dcb_db_owner, /* Object owner */
		msgid,
		FALSE,		    /* not-Force */
		&jsx->jsx_dberr, NULL);

	if (status != E_DB_OK)
	    return(status);
    }

    /* initialize table_id to be the that of the relation table */

    table_id.db_tab_base = DM_B_RELATION_TAB_ID;
    table_id.db_tab_index = DM_I_RELATION_TAB_ID;

 
    for (;;)
    {
	dsc_status = cnf->cnf_dsc->dsc_status;

	/*  Close the configuration file. */

	status = config_handler(CLOSE_CONFIG, dcb, &cnf, jsx);
	if (status != E_DB_OK)
	    break;

	if (jsx->jsx_status & JSX_VERBOSE)
	    TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
		"%@ CPP: Update catalogs.\n");
 
	/*  Open the database. */

	for (i = 0; i < 20; i++)
	{

	    status = dm2d_open_db(dcb, DM2D_A_WRITE, DM2D_X, 
			    dmf_svcb->svcb_lock_list, DM2D_NLK, &jsx->jsx_dberr);
	    if (status != E_DB_OK)
		break;

	    db_open = TRUE;
	    jsx->jsx_log_id = dcb->dcb_log_id;

	    if (((jsx->jsx_status1 & JSX1_CKPT_DB) == 0) ||
		((jsx->jsx_status & JSX_CKPXL) == 0) ||
		( (jsx->jsx_status & (JSX_JON | JSX_JOFF)) == 0))
		break;

	    /*
	    ** Make sure db is opened (and added to logging system)
	    ** with the correct journaling status (b114744)
	    */
	    database.db_id = (LG_DBID) dcb->dcb_log_id;
	    cl_status = LGshow(LG_S_DATABASE, (PTR)&database, sizeof(database),
		    &length, &sys_err);
	    if (cl_status != E_DB_OK)
	    {
		uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL, 
		    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
		uleFormat(NULL, E_DM9017_BAD_LOG_SHOW, &sys_err, ULE_LOG, NULL, 
		    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 1,
		    0, LG_S_DATABASE);
	    }

	    if (cl_status != E_DB_OK ||
		((dsc_status & DSC_JOURNAL) && (database.db_status & DB_JNL)) ||
		(!(dsc_status & DSC_JOURNAL) && !(database.db_status & DB_JNL)))
		break;

	    (VOID)dm2d_close_db(dcb, dmf_svcb->svcb_lock_list, 
			DM2D_NLK, &local_dberr);
	    db_open = FALSE;

	    PCsleep(1000);

	    if (jsx->jsx_status & JSX_VERBOSE)
		TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
		"%@ CPP: Database close pending CNF jnl %d LG jnl %d\n",
		    (dsc_status & DSC_JOURNAL) ? 1 : 0,
		    (database.db_status & DB_JNL) ? 1 : 0);
	}

	if (!db_open)
	    break;

	/*	Begin a transaction. */

	if (jsx->jsx_status & JSX_JOFF)
	    dmxe_flag = 0;

	status = dmf_jsp_begin_xact(DMXE_WRITE, dmxe_flag, dcb, dcb->dcb_log_id,
	    &dcb->dcb_db_owner, dmf_svcb->svcb_lock_list, &tran_lgid,
	    &tran_id, &tran_llid, &jsx->jsx_dberr);
	if (status != E_DB_OK)
	    break;

	/*	Open the relation table. */

	status = dm2t_open(dcb, &table_id, DM2T_IX, DM2T_UDIRECT, DM2T_A_WRITE,
	    0, 20, 0, tran_lgid, tran_llid, 0, 0, DM2T_X, &tran_id,
	    &timestamp, &rcb, (DML_SCB *)0, &jsx->jsx_dberr);
	if (status != E_DB_OK)
	    break;
	
	/*
	** Journal the iirelation updates.
	*/
	if (is_journal(rcb) || (dsc_status & DSC_JOURNAL))
	    rcb->rcb_journal = RCB_JOURNAL;
	else
	    rcb->rcb_journal = 0;

	/*	Position for a full table scan. */

	status = dm2r_position(rcb, DM2R_ALL, 0, 0, 0, &jsx->jsx_dberr);
	if (status != E_DB_OK)
	    break;

	/*	Examine each record returned. */

	for (;;)
	{
	    /*  Get the next record. */

	    status = dm2r_get(rcb, &tid, DM2R_GETNEXT, (char *)&relation, 
			      &jsx->jsx_dberr);
	    if (status != E_DB_OK)
	    {
		if (jsx->jsx_dberr.err_code == E_DM0055_NONEXT)
		{
		    status = E_DB_OK;
		    CLRDBERR(&jsx->jsx_dberr);
		}
		break;
	    }

	    upd_tup = 0;

	    /*  
	    ** For partial checkpoints, we are only interested in specified
	    ** tables and need go no further with this record if it wasn't.
	    */
	    if ((jsx->jsx_status1 & JSX1_CKPT_DB) == 0)
	    {
		found = FALSE;
		/* Check for user specified table in list */
		for (i = (jsx->jsx_tbl_cnt - 1); i >= 0; i--)
		{
	    	   /*
	    	   ** FIX ME : right now in all other places it just checks
	    	   **          for the table name for match. However I think
	    	   **          it should be both tablename & owner name.
	    	   */
		   if ((MEcmp(&relation.relid,
			&jsx->jsx_tbl_list[i].tbl_name.db_tab_name,
			DB_MAXNAME)) == 0)
		    {
               if (STbcompare((char *)&jsx->jsx_tbl_list[i].tbl_owner.db_own_name,
                                         0,"", 0, 0) != 0)
                    {
                        if ((MEcmp (&relation.relowner.db_own_name,
                                &jsx->jsx_tbl_list[i].tbl_owner.db_own_name
                                ,DB_MAXNAME)) == 0)
                        {
                                found = TRUE;
                        }
                    }
                    else
                    {
                        found = TRUE;
                    }
		  }
                }
		if (!found)
		    continue;
	    }

	    /*
	    ** When we get here, we have a database level checkpoint or
	    ** a specified table from a table level checkpoint.
	    */
		
	    /* Add for partial recovery - reset this bit     */
	    if (relation.relstat2 & TCB2_TBL_RECOVERY_DISALLOWED)
	    {
	    	relation.relstat2 &= ~TCB2_TBL_RECOVERY_DISALLOWED;
		upd_tup |= TUP_REC_ALLOW;
	    }

	    /*	Check for turning journaling on. */

	    if ((jsx->jsx_status & JSX_JON) ||
		((dcb->dcb_status & DCB_S_JOURNAL) &&
		 (jsx->jsx_status1 & JSX1_CKPT_DB)))
	    {
		if (relation.relstat & TCB_CATALOG)
		{
		    if ((relation.relstat & TCB_JOURNAL) == 0) 
		    {
			/* Catalog and not journaled */
			relation.relstat &= ~TCB_JON;
			relation.relstat |= TCB_JOURNAL;
			upd_tup |= TUP_JON;
		    }
		}
		else if (relation.relstat & TCB_JON) 
		{
		    /* 
		    ** Not a catalog but has 
		    ** 'enable journaling at next checkpoint' set
		    */
		    relation.relstat &= ~TCB_JON;
		    relation.relstat |= TCB_JOURNAL;
		    upd_tup |= TUP_JON;
		}
	    }
	    else if (jsx->jsx_status & JSX_JOFF)
	    {
		/* Bug 34069: if -j has been specified, set relstat to JON if
		** a table is marked as journaled
		**
		** When disabling journaling on the database, all tables
		** currently marked as journaled are changed to 'enable
		** journaling at next journaled checkpoint'
	        */

		if (relation.relstat & TCB_JOURNAL) 
		{
		    relation.relstat &= ~TCB_JOURNAL;
		    relation.relstat |= TCB_JON;
		    upd_tup |= TUP_JOFF;
		}
	    }

	    if (!upd_tup)
		continue;

	    /* Journal depending on the underlying table's journalling status */
	    if ((relation.relstat & TCB_JOURNAL) || (upd_tup & TUP_JON))
		rcb->rcb_usertab_jnl = TRUE;
	    else
		rcb->rcb_usertab_jnl = FALSE;

	    /*  Now replace the record. */

	    status = dm2r_replace(rcb, 0, DM2R_BYPOSITION, (char *)&relation, 
				  (char *)NULL, &jsx->jsx_dberr);

	    /*
	    **	Changing the journaling state of a table is a security event.
	    **  Make sure we audit it.
	    */

	    if ( dmf_svcb->svcb_status & SVCB_C2SECURE )
	    {
		msgid = 0;
		if (upd_tup & TUP_JON)
		    msgid = I_SX2715_CKPDB_TABLE_JNLON;
		else if (upd_tup & TUP_JOFF)
		    msgid = I_SX2716_CKPDB_TABLE_JNLOFF;

		if (msgid)
		{
		    access = SXF_A_ALTER;
		    if (status)
			access |= SXF_A_FAIL;
		    else
			access |= SXF_A_SUCCESS;

		    local_status = dma_write_audit( 
			SXF_E_TABLE,
			access,
			(char*)&relation.relid,	/* Object name (table) */
			sizeof(relation.relid),	/* Object name (table) */
			&relation.relowner,		/* Object owner */
			msgid,
			FALSE,		    /* not-Force since may be many */
			&local_dberr,
			NULL);

		    if (local_status > status)
		    {
			status = local_status;
			jsx->jsx_dberr = local_dberr;
		    }
		}
	    }
	    if (status != E_DB_OK)
		break;
	}

	/*  Check for some failure in the update loop. */

	if (status != E_DB_OK)
	    break;

	/*	Close the table. */
	status = dm2t_close(rcb, DM2T_NOPURGE, &jsx->jsx_dberr);
	if (status != E_DB_OK)
	    break;
	rcb = 0;

	/*	Commit the transaction. */

	status = dmf_jsp_commit_xact(&tran_id, tran_lgid, tran_llid, dmxe_flag,
	    &ctime, &jsx->jsx_dberr);
	if (status != E_DB_OK)
	    break;
	tran_lgid = 0;

	status = dm2d_close_db(
	    dcb, dmf_svcb->svcb_lock_list, DM2D_NLK, &jsx->jsx_dberr);
	if (status != E_DB_OK)
	    break;

	db_open = FALSE;

	if (jsx->jsx_status & JSX_VERBOSE)
	{
	    if (jsx->jsx_status & JSX_JON)
		TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
		    "%@ CPP: Journaling now enabled.\n");
	    else if (jsx->jsx_status & JSX_JOFF)
		TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
		    "%@ CPP: Journaling now disabled.\n");
	}

	/*
	** Reopen and read the configuration file to ensure 
	** extent and journal information is available.
	*/

	status = config_handler(OPEN_CONFIG, dcb, &cnf, jsx);
	if (status != E_DB_OK)
	    break;

	*config = cnf;

	return (E_DB_OK);
    }
    /* Handle the aborted transaction case. */


    /*	Log the error message from the call. */

    uleFormat(&jsx->jsx_dberr, 0, NULL, ULE_LOG, NULL, 
    		NULL, 0, NULL, err_code, 0);
    dmfWriteMsg(NULL, E_DM1107_CPP_CATALOG_ERROR, 0);

    if (cnf == 0)
    {
	/*
	** Reopen and read the configuration file to ensure 
	** extent and journal information is available.
	*/

	local_status = config_handler(OPEN_CONFIG, dcb, &cnf, jsx);
	if (local_status != E_DB_OK)
	{
	    uleFormat(&jsx->jsx_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, &local_err, 0);
	    cnf = 0;
	}
    }

    if (db_open)
    {
	if (rcb)
	{
	    local_status = dm2t_close(rcb, DM2T_NOPURGE, &local_dberr);
	    if (local_status)
		uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
		    (char *)NULL, (i4)0, (i4 *)NULL, &local_err, 0);
	}

	if (tran_lgid)
	{
	    *abort_xact_lgid = tran_lgid;
	    local_status = dmf_jsp_abort_xact((DML_ODCB *)0, dcb, &tran_id,
				tran_lgid, tran_llid,
				dmxe_flag, 0, 0, &local_dberr);
	    if (local_status)
		uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
		    (char *)NULL, (i4)0, (i4 *)NULL, &local_err, 0);
	}

	(VOID)dm2d_close_db(dcb, dmf_svcb->svcb_lock_list, 
			DM2D_NLK, &local_dberr);
    }

    /* Set final error, last */
    SETDBERR(&jsx->jsx_dberr, 0, E_DM110F_CPP_FAIL_NO_CHANGE);

    *config = cnf;
    return (status);
}

/*{
** Name: delete_files	- Delete all non-database files in a directory.
**
** Description:
**      This routine is called to delete all alias file and all temporary
**	table files in a directory.  This routine will only keep the following
**	file names:
**		aaaaaaaa.cnf			    Configuration file.
**		aaaaaaab.tnn - pppopppp.tnn	    Database tables.
**
**	The following files will be deleted:
**
**		ppppaaaa.tnn - pppppppp.tnn	    Temporary tables.
**		*.m*				    Modify temps.
**		*.d*				    Deleted files.
**		*.ali				    Alias files.
**		*.sm*				    Sysmod temps.
**		Any file with wrong name	    User created junk.
**
**	This routine can only be used for non-online backup, as temporary
**	tables cannot be deleted during online backup.
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
**	20-sep-1993 (andys)
**	     Fix arguments to ule_format call.
*/
static DB_STATUS
delete_files(
DMP_LOC_ENTRY	    *location,
char		    *filename,
i4		    l_filename,
CL_ERR_DESC	    *sys_err)
{
    DB_STATUS	    status;
    i4  	    err_code;
    DI_IO	    di_file;
    char	    line_buffer[132];
        
    if (l_filename == sizeof(DM_FILE) &&
	filename[8] == '.' &&
	MEcmp("aaaaaaaa", filename, 8) <= 0 &&
	MEcmp("pppopppp", filename, 8) >= 0 &&
	(MEcmp("t", &filename[9], 1) == 0 ||
	    MEcmp("aaaaaaaa.cnf", filename, 12) == 0))
    {
	return (OK);
    }

    TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
	"%@ CPP: Deleting non-database file: path = '%t', file = '%t'\n",
	location->phys_length, &location->physical,
	l_filename, filename);

    status = DIdelete(&di_file, (char *)&location->physical,
	    location->phys_length, filename, l_filename,
	    sys_err);

    if (status == OK)
	return (status);

    uleFormat(NULL, E_DM9003_BAD_FILE_DELETE, sys_err, ULE_LOG, NULL,
	(char *)NULL, (i4)0, (i4 *)NULL, &err_code, 4,
	4, "None", 4, "None",
	location->phys_length, &location->physical,
	l_filename, filename);

    return (DI_ENDFILE);
}

/*{
** Name: delete_alias_files - Delete all alias files in a directory.
**
** Description:
**      This routine is called to delete all alias filesin a directory.
**
**	The following files will be deleted:
**
**		*.ali				    Alias files.
**
**	This routine is used rather than the 'delete_files' routine during
**	online backup.
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
**      23-oct-1989 (rogerk)
**          Created for Terminator.
*/
static DB_STATUS
delete_alias_files(
DMP_LOC_ENTRY	    *location,
char		    *filename,
i4		    l_filename,
CL_ERR_DESC	    *sys_err)
{
    DB_STATUS	    status;
    i4  	    error;
    DI_IO	    di_file;
    char	    line_buffer[132];

    /*
    ** If file does not end in ".ali", then don't delete it.
    */
    if ((l_filename != sizeof(DM_FILE)) || (filename[8] != '.') ||
	(MEcmp(&filename[9], "ali", 3) != 0))
    {
	return (OK);
    }

    TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
	"%@ CPP: Deleting non-database file: path = '%t', file = '%t'\n",
	location->phys_length, &location->physical,
	l_filename, filename);

    status = DIdelete(&di_file, (char *)&location->physical,
	    location->phys_length, filename, l_filename,
	    sys_err);

    if (status == OK)
	return (status);

    uleFormat(NULL, E_DM9003_BAD_FILE_DELETE, sys_err, ULE_LOG, NULL,
	(char *)NULL, (i4)0, (i4 *)NULL, &error, 4,
	4, "None", 4, "None",
	location->phys_length, &location->physical,
	l_filename, filename);

    return (DI_ENDFILE);
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
**      dcb                             Pointer to DCB for database.
**	cnf				Pointer to CNF pointer.
**
** Outputs:
*	err_code			Reason for error return.
**	Returns:
**	    E_DB_OK
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
**	01-feb-1989 (EdHsu)
**	    Updated for online backup.
**	 5-dec-1989 (rogerk)
**	    Added DUMP_CONFIG option to copy config file to dump directory
**	    at start of online backup.
**	21-may-1990 (bryanp)
**	    Changed UPDATE_CONFIG to copy the config file to the dump
**	    directory (as aaaaaaaa.cnf, not Cnnnnnnn.DMP) after significant
**	    updates have been performed to the file.
**	19-aug_1992 (ananth)
**	    Moved trace messages into this routine as part of backup rewrite.
**      11-jan-1993 (mikem)
**          Lint directed changes.  Added a return(E_DB_ERR) to bottom of
**          routine as a bit of defensive programing, the code should never
**          get there; but if it ever did previously a random return would
**          result.
**      02-jan-2007 (smeke01) b114251
**          Prevent possibility of sigsegv (bug 114251) when #x option used
**          by making sure cnf pointer is used before cnf object is 
**          de-allocated, and not after. 
*/
static DB_STATUS
config_handler(
i4             operation,
DMP_DCB		    *dcb,
DM0C_CNF	    **config,
DMF_JSX		    *jsx)
{
    DM0C_CNF		*cnf = *config;
    i4		i;
    DB_STATUS		status;
    char		line_buffer[132];
    DB_ERROR		local_dberr;
    i4			*err_code = &jsx->jsx_dberr.err_code;

    CLRDBERR(&jsx->jsx_dberr);

    switch (operation)
    {
    case OPEN_CONFIG:

	/*  Open the configuration file. */

	*config = 0;
	status = dm0c_open(dcb, 0, dmf_svcb->svcb_lock_list, config, &jsx->jsx_dberr);
	if (status == E_DB_OK)
	{
	    cnf = *config;

	    /*  Allocate and initialize the EXT. */

	    status = dm0m_allocate((i4)(sizeof(DMP_EXT) +
		cnf->cnf_dsc->dsc_ext_count * sizeof(DMP_LOC_ENTRY)),
		0, (i4)EXT_CB,	(i4)EXT_ASCII_ID, (char *)dcb,
		(DM_OBJECT **)&dcb->dcb_ext, &jsx->jsx_dberr);
	    if (status != E_DB_OK)
	    {
		uleFormat(&jsx->jsx_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
		    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
		SETDBERR(&jsx->jsx_dberr, 0, E_DM1109_CPP_MEM_ALLOC);
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
	
	    /*	Capture journal status of the database - catalog updating. */

	    if (cnf->cnf_dsc->dsc_status & DSC_JOURNAL)
		dcb->dcb_status |= DCB_S_JOURNAL;

	    if (jsx->jsx_status & JSX_TRACE)
	    {
		TRdisplay("%@ CPP: Open Config File\n");
		dump_cnf(cnf);
	    }

	    return (E_DB_OK);
	}

	if (*config)
	    dm0c_close(*config, 0, &local_dberr);
	*config = 0;
	uleFormat(&jsx->jsx_dberr, 0, NULL, ULE_LOG, NULL, 
			NULL, 0, NULL, err_code, 0);
	SETDBERR(&jsx->jsx_dberr, 0, E_DM1105_CPP_CNF_OPEN);
	return (E_DB_ERROR);

    case CLOSEKEEP_CONFIG:

	if (jsx->jsx_status & JSX_TRACE)
	{
	    TRdisplay("%@ CPP: Close (Keep) Config File\n");
	    dump_cnf(cnf);
	}

	status = dm0c_close(cnf, 0, &jsx->jsx_dberr);

	if (status == E_DB_OK)
	{
	    *config = 0;

	    return (E_DB_OK);
	}
	uleFormat(&jsx->jsx_dberr, 0, NULL, ULE_LOG, NULL, 
			NULL, 0, NULL, err_code, 0);
	SETDBERR(&jsx->jsx_dberr, 0, E_DM110C_CPP_CNF_CLOSE);
	return (E_DB_ERROR);

    case UNKEEP_CONFIG:

	if (dcb->dcb_ext)
	{
	    dm0m_deallocate((DM_OBJECT **)&dcb->dcb_ext);
	}
	dcb->dcb_root = 0;
	dcb->dcb_dmp = 0;
	dcb->dcb_jnl = 0;
	dcb->dcb_ckp = 0;
	return (E_DB_OK);
	    
    case CLOSE_CONFIG:

	if (*config == 0)
	    return (E_DB_OK);

	if (jsx->jsx_status & JSX_TRACE)
	{
	    TRdisplay("%@ CPP: Close Config File\n");
	    dump_cnf(cnf);
	}

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

	uleFormat(&jsx->jsx_dberr, 0, NULL, ULE_LOG, NULL, 
			NULL, 0, NULL, err_code, 0);
	SETDBERR(&jsx->jsx_dberr, 0, E_DM110C_CPP_CNF_CLOSE);
	return (E_DB_ERROR);

    case UPDATE_CONFIG:

	/*  
	** Write configuration file to disk. Also, copy it to the dump
	** directory so that we have an up-to-date backup in case of disaster
	*/

	status = dm0c_close(
	    cnf, DM0C_UPDATE | DM0C_LEAVE_OPEN | DM0C_COPY, &jsx->jsx_dberr);

	if (status == E_DB_OK)
	{   
	    if (jsx->jsx_status & JSX_TRACE)
	    {
		TRdisplay("%@ CPP: Updated Config File\n");
		dump_cnf(cnf);
	    }
	    return (status);
	}

	uleFormat(&jsx->jsx_dberr, 0, NULL, ULE_LOG, NULL, 
			NULL, 0, NULL, err_code, 0);
	SETDBERR(&jsx->jsx_dberr, 0, E_DM110C_CPP_CNF_CLOSE);
	return (E_DB_ERROR);

    case DUMP_CONFIG:
	/*
	** Copy configuration file to the dump directory for use during
	** rollforward from online backup.
	*/
	status = dm0c_close(cnf, DM0C_COPY | DM0C_DUMP | DM0C_LEAVE_OPEN, 
	    &jsx->jsx_dberr);

	if (status == E_DB_OK)
	{
	    if (jsx->jsx_status & JSX_TRACE)
		TRdisplay("%@ CPP: Copied CONFIG file to DUMP location\n");

	    return (status);
	}

	uleFormat(&jsx->jsx_dberr, 0, NULL, ULE_LOG, NULL, 
			NULL, 0, NULL, err_code, 0);
	SETDBERR(&jsx->jsx_dberr, 0, E_DM1148_CPP_CNF_DUMP);
	return (E_DB_ERROR);
    }
    return (E_DB_ERROR);
}

/*{
** Name: create_ckpjnldmp   - Create journal,dump and/or checkpoint directories.
**
** Description:
**      Create journal, dump or checkpoint directories if they are needed
**	and they don't exist.
**
** Inputs:
**      jsx                             Pointer to JSX control block.
**	dcb				Pointer to DCB.
**      ckp_seq                         Checkpoint sequence number.
**	jflag				Flag indicating if db journaled.
**	dflag				Flag indicating if db dumped.
**
** Outputs:
**      err_code                        Reason for error return code.
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
**      20-apr-1987 (Derek)
**          Recreated for Jupiter.
**	30-sep-88 (rogerk)
**	    Changed dmxe_begin call to use new arguments.
**	26-jan-1989 (EdHsu)
**	    Added code to support online backup.
**	14-apr-1989 (EdHsu)
**	    Online backup cluster support
**	04-may-1989 (Sandyh)
**	    Fixed bugs 5442 & 5445 having to do with 24 char location names.
**	16-oct-1989 (rogerk)
**	    Create dump directory even if not online checkpoint.  The config
**	    file is copied to the dump directory by the archiver if the
**	    database is journaled, regardless of whether it was last
**	    checkpointed in an online fashion.
**	14-may-1990 (rogerk)
**	    Added new error return code (9036) so that sensible error code
**	    could be returned to the caller.  Changed error calls that returned
**	    with no error code.  Added parameter count to ule_format calls
**	    which were missing it.  Changed STRUCT_ASSIGN_MACRO's of different
**	    typed objects to MEcopy's.
**	25-mar-1991 (bryanp)
**	    Initialize dcb_type to DCB_CB so that xDEBUG checks on the control
**	    blocks don't reject it as invalid.
**	15-oct-1991 (mikem)
**	    Eliminated sun compiler warning "warning: statement not reached"
**	    by modifying the way "for" loop was coded.
*/
static DB_STATUS
create_ckpjnldmp(
DMF_JSX             *jsx,
DMP_DCB		    *dcb,
i4             ckp_seq,
bool                jflag,
bool		    dflag)
{
    DMP_DCB		dbdb_dcb;
    DMP_DCB		*d = &dbdb_dcb;
    DMP_RCB		*rcb;
    i4		i;
    CL_ERR_DESC		sys_err;
    DM_TID		tid;
    DB_TRAN_ID		tran_id;
    i4		log_id;
    i4		lock_id;
    DB_TAB_TIMESTAMP	stamp;
    i4  		error;
    DB_STATUS		status;
    DB_STATUS		close_status;
    DM2R_KEY_DESC	key_list[1];
    DB_TAB_ID		iilocation;
    struct
    {
    	i2	    count;
	char	    logical[sizeof(dcb->dcb_location.logical)];
    }	logical_text;
    DU_LOCATIONS    	jnl_location;
    DU_LOCATIONS	dmp_location;
    DU_LOCATIONS	ckp_location;
    JFIO	    jfio;
    LOCATION	    loc;
    char	    *cp;
    char	    line_buffer[132];
    DB_ERROR		local_dberr;
    i4			*err_code = &jsx->jsx_dberr.err_code;

    CLRDBERR(&jsx->jsx_dberr);

    /* default initialization */

    iilocation.db_tab_base  = DM_B_LOCATIONS_TAB_ID;
    iilocation.db_tab_index = DM_I_LOCATIONS_TAB_ID;

    /*	Check to see if any work is required. */

    if ((jflag) && (dflag) && (ckp_seq))
    {
	return (E_DB_OK);
    }

    /*	Fill the in DCB for the database database. */

    MEfill(sizeof(*d), 0, (char *)d);
    MEmove(sizeof(DU_DBDBNAME) - 1, DU_DBDBNAME, ' ', 
           sizeof(d->dcb_name), (char *)&d->dcb_name);
    MEmove(sizeof(DU_DBA_DBDB) - 1, DU_DBA_DBDB, ' ', 
           sizeof(d->dcb_db_owner), (char *)&d->dcb_db_owner);
    d->dcb_type = DCB_CB;
    d->dcb_access_mode = DCB_A_READ;
    d->dcb_served = DCB_MULTIPLE;
    d->dcb_db_type = DCB_PUBLIC;
    d->dcb_log_id = dmf_svcb->svcb_lctx_ptr->lctx_lgid;
    dm0s_minit(&d->dcb_mutex, "DCB mutex");
    MEmove(8, "$default", ' ', sizeof(d->dcb_location.logical), 
           (PTR)&d->dcb_location.logical);
    
    status = LOingpath("II_DATABASE", "iidbdb", LO_DB, &loc);
    if (!status)
	LOtos(&loc, &cp);

    if (status || *cp == EOS)
    {
	uleFormat(NULL, E_DM1117_CPP_II_DATABASE, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
	    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 1,
	    STlength(SystemProductName), SystemProductName);
	SETDBERR(&jsx->jsx_dberr, 0, E_DM9036_JNLCKP_CREATE_ERROR);
        return (E_DB_ERROR);
    }

    MEmove(STlength(cp), cp, ' ', sizeof(d->dcb_location.physical),
	(PTR)&d->dcb_location.physical);
    d->dcb_location.phys_length = STlength(cp);
    
    /*	Open the database database. First see if it is the database 
    **  database that we are checkpointing.
    */

    
    status = MEcmp((char *)&dcb->dcb_name, (char *)&d->dcb_name, 
	sizeof(d->dcb_name));
    if (status == 0)
    {
	d=dcb;
	status = dm2d_open_db(d, DM2D_A_WRITE, DM2D_X, 
                              dmf_svcb->svcb_lock_list, DM2D_NLK,
				    &jsx->jsx_dberr); 
    }
    else
    {
	status = dm2d_open_db(d, DM2D_A_READ, DM2D_IS, 
                              dmf_svcb->svcb_lock_list, 0, &jsx->jsx_dberr); 
    }

    if (status == E_DB_OK)
	jsx->jsx_log_id = dcb->dcb_log_id;
    else
    {
	SETDBERR(&jsx->jsx_dberr, 0, E_DM1009_JSP_DBDB_OPEN);
	return (E_DB_ERROR);
    }

    status = dmf_jsp_begin_xact(DMXE_READ, 0, d, d->dcb_log_id, &dcb->dcb_db_owner,
	dmf_svcb->svcb_lock_list, &log_id, &tran_id, &lock_id, &jsx->jsx_dberr);

    for(;;)
    {
	if (status != E_DB_OK)
	    break;

	/*  Open the database table. */

	status = dm2t_open(d, &iilocation, DM2T_IS, DM2T_UDIRECT, DM2T_A_READ,
	    0, 20, 0, 0, dmf_svcb->svcb_lock_list, 0, 0, 0, &tran_id,
	    &stamp, &rcb, (DML_SCB *)0, &jsx->jsx_dberr);

	if (status != E_DB_OK)
	{
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM1015_JSP_OPEN_IILOC);
	    break;
	}

	/*  Lookup journal location. */

	for (i = 0; dcb->dcb_jnl->logical.db_loc_name[i] != ' '; i++)
	    ;
	i > DB_MAXNAME ? i = DB_MAXNAME : i;
	logical_text.count = i;
	MEcopy((PTR)&dcb->dcb_jnl->logical, 
		sizeof(logical_text.logical), (PTR)logical_text.logical);
	key_list[0].attr_number = DM_1_LOCATIONS_KEY;
	key_list[0].attr_value = (char *)&logical_text;
	key_list[0].attr_operator = DM2R_EQ;
    
	status = dm2r_position(rcb, DM2R_QUAL, key_list, 1,
                               &tid, &jsx->jsx_dberr);

	if (status == E_DB_OK)
	    status = dm2r_get(rcb, &tid, DM2R_GETNEXT, 
				(char *)&jnl_location, &jsx->jsx_dberr);
	if (status != E_DB_OK)
	    break;

	/*  Lookup dump location. */

	for (i = 0; dcb->dcb_dmp->logical.db_loc_name[i] != ' '; i++)
	    ;
	i > DB_MAXNAME ? i = DB_MAXNAME : i;
	logical_text.count = i;
	MEcopy((PTR)&dcb->dcb_dmp->logical, 
		sizeof(logical_text.logical), (PTR)logical_text.logical);
	key_list[0].attr_number = DM_1_LOCATIONS_KEY;
	key_list[0].attr_value = (char *)&logical_text;
	key_list[0].attr_operator = DM2R_EQ;
   
	status = dm2r_position(rcb, DM2R_QUAL, key_list, 1,
                               &tid, &jsx->jsx_dberr);

	if (status == E_DB_OK)
	    status = dm2r_get(rcb, &tid, DM2R_GETNEXT, 
				(char *)&dmp_location, &jsx->jsx_dberr);
	if (status != E_DB_OK)
	    break;

	/*  Lookup checkpoint location. */

	for (i = 0; dcb->dcb_ckp->logical.db_loc_name[i] != ' '; i++)
	    ;
	i > DB_MAXNAME ? i = DB_MAXNAME : i;
	logical_text.count = i;
	MEcopy((PTR)&dcb->dcb_ckp->logical, 
		sizeof(logical_text.logical), (PTR)logical_text.logical);
	key_list[0].attr_number = DM_1_LOCATIONS_KEY;
	key_list[0].attr_value = (char *)&logical_text;
	key_list[0].attr_operator = DM2R_EQ;
	status = dm2r_position(rcb, DM2R_QUAL, key_list, 1,
                               &tid, &jsx->jsx_dberr);

	if (status == E_DB_OK)
	    status = dm2r_get(rcb, &tid, DM2R_GETNEXT, 
				(char *)&ckp_location, &jsx->jsx_dberr);
	break;
    }

    /*  Close the location table. */

    if (rcb)
    {
	close_status = dm2t_close(rcb, 0, &local_dberr);
	if (close_status != E_DB_OK && status == E_DB_OK)
	{
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM1013_JSP_CLOSE_IILOC);
	    status = close_status;
	}
	rcb = 0;
    }

    if (log_id)
    {
	status = dmf_jsp_commit_xact(&tran_id, log_id, lock_id, DMXE_ROTRAN,
	    &stamp, &jsx->jsx_dberr);
    }

    /*	Close the database. */

    if (d == dcb)
	close_status = dm2d_close_db(
	    d, dmf_svcb->svcb_lock_list, DM2D_NLK, &jsx->jsx_dberr);
    else	
	close_status = dm2d_close_db(
	    d, dmf_svcb->svcb_lock_list, 0, &jsx->jsx_dberr);
    if (close_status != E_DB_OK && status == E_DB_OK)
	status = close_status;

    if (status != E_DB_OK)
    {
	return (E_DB_ERROR);
    }

    /*	Now create journal, dump and/or checkpoint directories. */

    if (ckp_seq == 0)
    {
	char	    temp_loc[DB_AREA_MAX + 4];

	MEmove(ckp_location.du_l_area, ckp_location.du_area,
	    0, sizeof(temp_loc), temp_loc);

	status = LOingpath(temp_loc, 0, LO_CKP, &loc);

        if (!status)
	    LOtos(&loc, &cp);

	if (status || *cp == EOS)
	{
	    uleFormat(NULL, E_DM1115_CPP_II_CHECKPOINT, (CL_ERR_DESC *)NULL, ULE_LOG, 
		NULL, (char *)NULL, (i4)0, (i4 *)NULL, err_code, 1,
		STlength(SystemProductName), SystemProductName);
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM9036_JNLCKP_CREATE_ERROR);
	    return (E_DB_ERROR);
	}

	for (i = 0; i < sizeof(DB_DB_NAME); i++)
	    if (dcb->dcb_name.db_db_name[i] == ' ')
		break;
	if (jsx->jsx_status & JSX_VERBOSE)
	    TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
		"%@ CPP: Create checkpoint directory: %~t at %t\n",
		sizeof(dcb->dcb_name), &dcb->dcb_name,
		STlength(cp), cp);
	status = CKdircreate(cp, STlength(cp), dcb->dcb_name.db_db_name,
				i, &sys_err);
	if (status != OK && status != CK_EXISTS)
	{
	    uleFormat(NULL, status, &sys_err, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM9040_BAD_CKP_DIRCREATE);
	    return (E_DB_ERROR);
	}
    }

    if (jsx->jsx_status & JSX_JON)
    {
	char	    temp_loc[DB_AREA_MAX];

	MEmove(jnl_location.du_l_area, jnl_location.du_area,
	    0, sizeof(temp_loc), temp_loc);

	status = LOingpath(temp_loc, 0, LO_JNL, &loc);

        if (!status)
	    LOtos(&loc, &cp);

	if (status || *cp == EOS)
	{
	    uleFormat(NULL, E_DM1116_CPP_II_JOURNAL, (CL_ERR_DESC *)NULL, ULE_LOG, 
		NULL, (char *)NULL, (i4)0, (i4 *)NULL, err_code, 1,
		STlength(SystemProductName), SystemProductName);
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM9036_JNLCKP_CREATE_ERROR);
	    return (E_DB_ERROR);
	}

	for (i = 0; i < sizeof(DB_DB_NAME); i++)
	{
	    if (dcb->dcb_name.db_db_name[i] == ' ')
		break;
	}
	status = JFdircreate(&jfio, cp, STlength(cp), 
				dcb->dcb_name.db_db_name,
			        i, &sys_err);
	if (status != OK && status != JF_EXISTS)
	{
	    uleFormat(NULL, E_DM9033_BAD_JNL_DIRCREATE, &sys_err, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, err_code, 1,
		STlength(cp), cp);
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM9036_JNLCKP_CREATE_ERROR);
	    return (E_DB_ERROR);
	}
    }

    /*
    ** Create dump directory if not already there.
    **
    ** If this is an online checkpoint, then online dump information will
    ** be written here to be used to restore from the online checkpoint.
    ** If this is not an online checkpoint, the dump directory will be used
    ** by the archiver to store copies of the config file to protect against
    ** corrupted config files in the database directory.
    */
    {
	char	    temp_loc[DB_AREA_MAX];

	MEmove(dmp_location.du_l_area, dmp_location.du_area,
	    0, sizeof(temp_loc), temp_loc);

	status = LOingpath(temp_loc, 0, LO_DMP, &loc);

        if (!status)
	    LOtos(&loc, &cp);

	if (status || *cp == EOS)
	{
	    uleFormat(NULL, E_DM1133_CPP_II_DUMP, NULL, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM9036_JNLCKP_CREATE_ERROR);
	    return (E_DB_ERROR);
	}

	for (i = 0; i < sizeof(DB_DB_NAME); i++)
	{
	    if (dcb->dcb_name.db_db_name[i] == ' ')
		break;
	}
	status = JFdircreate(&jfio, cp, STlength(cp),
				dcb->dcb_name.db_db_name,
	                        i, &sys_err);
	if (status != OK && status != JF_EXISTS)
	{
	    uleFormat(NULL, E_DM935C_BAD_DMP_DIRCREATE, (CL_ERR_DESC *)NULL, ULE_LOG, 
		NULL, (char *)NULL, (i4)0, (i4 *)NULL, err_code, 1,
		STlength(cp), cp);
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM9036_JNLCKP_CREATE_ERROR);
	    return (E_DB_ERROR);
	}
    }

    return (E_DB_OK);
}
	
/*{
** Name: cluster_merge - Merge old journal files before doing new checkpoint.
**
** Description:
**	Used for cluster installations.  This routine does a merge of all the
**	journal files for all nodes into a single inclusive journal file.
**
**	This routine must be run after all journal updates have been flushed
**	to disk on all nodes of the cluster.  This adds some complications
**	when using Online Checkpoint.
**
**	If Online Checkpoint, then this routine must be called while the
**	system is in its stall state, but before the new checkpoint sequence
**	has been created (before the SBACKUP record is written).
**
**	Before this routine is called, we must signal a Consistency Point, wait
**	for it to complete, and then signal a purge on the database to flush
**	all log records to the journal files up to the new Consistency Point.
**	We know that this will get all required journal records since any new
**	journal records to be written will go to new journal files to be
**	associated with the new checkpoint.
**
** Inputs:
**      jsx                             Pointer to JSX control block.
**	dcb				Pointer to DCB.
**
** Outputs:
**      err_code                        Reason for error return code.
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
**      5-jan-1990 (rogerk)
**          Created for Terminator.
**	19-aug-1992 (ananth)
**	    Took the code that signalled a consistency point and waited for
**	    the purge out of this routine. This is now done in the mainline
**	    code. 
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**		Altered dm0j_merge calling sequence in order to allow auditdb
**		    to call dm0j_merge.
*/
static DB_STATUS
cluster_merge(
DMF_JSX         *jsx,
DMP_DCB		*dcb)
{
    DB_STATUS	    status;
    DM0C_CNF	    *cnf;
    i4			*err_code = &jsx->jsx_dberr.err_code;

    CLRDBERR(&jsx->jsx_dberr);

    TRdisplay(ERx("%@ CPP %d: Begin cluster merge of node journal files\n"),
		__LINE__);

    status = config_handler(OPEN_CONFIG, dcb, &cnf,  jsx);

    /*
    ** Perform the Merge on the journal files.
    */
    if (status == E_DB_OK)
    {
	status = dm0j_merge(cnf, (DMP_DCB *)0, (PTR)0,
			    (DB_STATUS (*)())0, &jsx->jsx_dberr);
    }

    TRdisplay(ERx("%@ CPP %d: Finish cluster merge of node journal files\n"),
		__LINE__);

    if (status == E_DB_OK)
	status = config_handler(UPDATE_CONFIG, dcb, &cnf, jsx);
    if (status == E_DB_OK)
	status = config_handler(CLOSE_CONFIG, dcb, &cnf, jsx);
    if (status != E_DB_OK)
    {
	uleFormat(&jsx->jsx_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
	    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
    }

    if (status == E_DB_OK)
	return (E_DB_OK);

    SETDBERR(&jsx->jsx_dberr, 0, E_DM1153_CPP_CLUSTER_MERGE);
    return (E_DB_ERROR);
}

/*{
** Name: truncate_jnl_dmp    - Truncate journal and dump files
**
** Description:
** 	Truncate the last set of journal and dump files to reclaim empty 
**	space.  
**
**	Journal and dump files are created with preallocated space.
**	If possible, we want to give some of it back.
**
** Inputs:
**	jsx				Journal support context.
**	dcb				Pointer to DCB.
**	config				Pointer to pointer to config file.
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
**	20-sep-1993 (jnash)
**	    Extracted from file_maint() and made into separate () as
**	    part of alterdb -delete_oldest_ckp changes.
**	15-jan-1999 (nanpr01)
**	    Pass point to a pointer in dm0j_close & dm0d_close routines.
*/
static DB_STATUS
truncate_jnl_dmp(
DMF_JSX             *jsx,
DMP_DCB		    *dcb,
DM0C_CNF	    **config)
{
    DM0C_CNF            *cnf = *config;
    DM0J_CTX		*jnl;
    DM0D_CTX		*dmp;
    DB_STATUS		status;
    i4		i;
    i4		j;
    i4			*err_code = &jsx->jsx_dberr.err_code;

    CLRDBERR(&jsx->jsx_dberr);

    /*
    ** Truncate all journal files belonging to the previous checkpoint.
    ** Make sure there is a previous checkpoint.	
    */
    while (cnf->cnf_jnl->jnl_count > 1)
    {
	/* Truncate journal files pertaining to previous checkpoint */

	i = cnf->cnf_jnl->jnl_count - 2;
	if ((j = cnf->cnf_jnl->jnl_history[i].ckp_f_jnl) == 0)
	    break;

	/*
	** Cycle through all journal files for this checkpoint. 
	*/
	for (; j <= cnf->cnf_jnl->jnl_history[i].ckp_l_jnl; j++)
	{
	    status = dm0j_open(DM0J_MERGE, (char *)&dcb->dcb_jnl->physical,
		dcb->dcb_jnl->phys_length, &dcb->dcb_name,
		cnf->cnf_jnl->jnl_bksz, j, 0, 0, DM0J_M_WRITE, -1,
		DM0J_FORWARD, (DB_TAB_ID *)0, &jnl, &jsx->jsx_dberr);
	    if (status != E_DB_OK)
	    {
		/* If no journal files exist, no problem */

		if (jsx->jsx_dberr.err_code == E_DM9034_BAD_JNL_NOT_FOUND)
		{
		    CLRDBERR(&jsx->jsx_dberr);
		    return(E_DB_OK);
		}
		else
		{
		    dmfWriteMsg(&jsx->jsx_dberr, E_DM1118_CPP_JNL_OPEN, 0);
		    return(E_DB_ERROR);
		}
	    }
	    status = dm0j_truncate(jnl, &jsx->jsx_dberr);
	    if (status != E_DB_OK)
	    {
		dmfWriteMsg(&jsx->jsx_dberr, E_DM1119_CPP_JNL_TRUNCATE, 0);
		return(E_DB_ERROR);
	    }
	    (VOID) dm0j_close(&jnl, &jsx->jsx_dberr);
	    if (status != E_DB_OK)
	    {
		dmfWriteMsg(&jsx->jsx_dberr, E_DM1120_CPP_JNL_CLOSE, 0);
		return(E_DB_ERROR);
	    }
	    jnl = 0;
	}

	break;
    }

    /*
    ** Truncate all dump files belonging to the previous checkpoint.
    ** Make sure there is a previous online checkpoint.
    **
    ** If the current checkpoint is an Offline Checkpoint, then truncate
    ** the most recent dump entry - which must belong to a previous
    ** checkpoint.
    */
    i = cnf->cnf_dmp->dmp_count - 2;
    if (jsx->jsx_status & JSX_CKPXL)
	i++;

    if ((i >= 0) && ((j = cnf->cnf_dmp->dmp_history[i].ckp_f_dmp) != 0))
    {
	/*
	** Truncate dump files pertaining to previous checkpoint.
	*/

	for (; j <= cnf->cnf_dmp->dmp_history[i].ckp_l_dmp; j++)
	{
	    status = dm0d_open(DM0D_MERGE, (char *)&dcb->dcb_dmp->physical,
		dcb->dcb_dmp->phys_length, &dcb->dcb_name,
		cnf->cnf_dmp->dmp_bksz, j, 0, 0, DM0D_M_WRITE, -1,
		DM0D_FORWARD, &dmp, &jsx->jsx_dberr);
	    if (status != E_DB_OK)
	    {
		/* If no dump files exist, no problem */

		if (jsx->jsx_dberr.err_code == E_DM9352_BAD_DMP_NOT_FOUND)
		{
		    return(E_DB_OK);
		}
		else
		{
		    dmfWriteMsg(&jsx->jsx_dberr, E_DM1141_CPP_DMP_OPEN, 0);
		    return(E_DB_ERROR);
		}
	    }
	    status = dm0d_truncate(dmp, &jsx->jsx_dberr);
	    if (status != E_DB_OK)
	    {
		dmfWriteMsg(&jsx->jsx_dberr, E_DM1142_CPP_DMP_TRUNCATE, 0);
		return(E_DB_ERROR);
	    }
	    (VOID) dm0d_close(&dmp, &jsx->jsx_dberr);
	    if (status != E_DB_OK)
	    {
		dmfWriteMsg(&jsx->jsx_dberr, E_DM1143_CPP_DMP_CLOSE, 0);
		return(E_DB_ERROR);
	    }
	}
    }

    return(E_DB_OK);
}

/*{
** Name: online_display_stall_message - Display a stall message.
**
** Description:
**	Routine displays information on transactions that may cause us to 
**	stall.  Note that the LGalter call is after this message so they may
**	have committed in this very tint window, unlikely but true. 
**	Still the information is dead usefull for users to determine what
**	is happening.
**
** Inputs:
**	jsx				Journal support context.
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
**	20-may-1994 (rmuth)
**	    Created, bug 63533 - Display usefull information about the
**	    stall.
*/
static DB_STATUS
online_display_stall_message(
DMF_JSX	    *jsx)
{
    LG_DATABASE		database;
    LG_TRANSACTION      tr;
    LG_HEADER		header;
    STATUS		cl_status;
    CL_ERR_DESC		sys_err;
    char		line_buffer[132];
    i4		show_index = 0;
    i4		length;
    i4		err_arg;
    DB_STATUS		status = E_DB_OK;
    i4			*err_code = &jsx->jsx_dberr.err_code;

    CLRDBERR(&jsx->jsx_dberr);

    do 
    {
	/*
	** Find out information on the database to determine if there 
	** are currently any open transactions
	*/
	database.db_id = (LG_DBID) jsx->jsx_log_id;
	cl_status = LGshow(LG_S_DATABASE, (PTR)&database, sizeof(database),
	    &length, &sys_err);
	if (cl_status != E_DB_OK)
	{
	    uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM9017_BAD_LOG_SHOW);
	    err_arg = LG_S_DATABASE;
	    status = E_DB_ERROR;
	    break;
	}

	/*
	** Display the stall message
	*/
	TRformat( dmf_put_line, 0, line_buffer, sizeof(line_buffer),
		"%@ CPP: Preparing stall of database, active xact cnt: %d",
		database.db_stat.trans );

	/*
	** Only verbose mode displays information on transactions.
	*/
	if ((jsx->jsx_status & JSX_VERBOSE) == 0 )
	{
	    status = E_DB_OK;
	    break;
	}

	/*
	** If active transactions then we will stall so display more 
	** info on these transactions
	*/
	if ( database.db_stat.trans )
	{
	    TRformat( dmf_put_line, 0, line_buffer, sizeof(line_buffer),
		      "\nList of active transactions...\n" );

	    /*
	    ** Get the log header so can display the transactions in same
	    ** format as logstat.
	    */
            cl_status = LGshow( LG_A_HEADER, (PTR)&header, sizeof(header),
                                &length, &sys_err);
            if (cl_status != OK )
	    {
		uleFormat( NULL, cl_status, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
		SETDBERR(&jsx->jsx_dberr, 0, E_DM9017_BAD_LOG_SHOW);
		err_arg = LG_A_HEADER;
		status = E_DB_ERROR;
		break;
	    }
 
	    for (;;)
	    {
		cl_status = LGshow(LG_N_TRANSACTION, (PTR)&tr, sizeof(tr),
				    &show_index, &sys_err);
		if (cl_status != OK)
		{
		    uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
			(char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
		    SETDBERR(&jsx->jsx_dberr, 0, E_DM9017_BAD_LOG_SHOW);
		    err_arg = LG_N_TRANSACTION;
		    status = E_DB_ERROR;
		    break;
		}

		/*
		** If we have exhausted the transaction list, then we 
	        ** are finished.
		*/
		if (show_index == 0)
		    break;

		/*
		** Check is interested in this transaction, i.e. is it active,
		** protect and for this database. If not then skip it...
		*/
		if (( tr.tr_db_id != database.db_id ) || 
		   ( (tr.tr_status & ( TR_PROTECT | TR_ACTIVE )) == 0 ))
		    continue;

		/*
	        ** Display transaction the tranid, session id, process id.
		*/
	        TRformat( dmf_put_line, 0, line_buffer, sizeof(line_buffer),
	  	          "    Username: %~t  Tran_id: %x%x\n",
			  tr.tr_l_user_name, tr.tr_user_name,
			  tr.tr_eid.db_high_tran, tr.tr_eid.db_low_tran );
	        TRformat( dmf_put_line, 0, line_buffer, sizeof(line_buffer),
	  		  "    Process: %x  Session: %p\n\n",
			  tr.tr_pr_id, tr.tr_sess_id );
           

		
	    }

	    if ( status != E_DB_OK )
		break;

	}

    } while (FALSE);

    if (status != E_DB_OK)
    {
        /* If got LGshow error, then pass the show error parameter */
        if (jsx->jsx_dberr.err_code == E_DM9017_BAD_LOG_SHOW)
        {
            uleFormat(&jsx->jsx_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
                (char *)NULL, (i4)0, (i4 *)NULL, err_code, 1,
                0, err_arg);
        }
        else
        {
            uleFormat(&jsx->jsx_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
                (char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
        }

	SETDBERR(&jsx->jsx_dberr, 0, E_DM942D_RCP_ONLINE_CONTEXT);
        return (E_DB_ERROR);
    }

    return (E_DB_OK);
}

/*{
** Name:
**
** Description: cpp_prepare_tbl_ckpt.
**
**
** Inputs:
**      cpp                             Pointer to checkpoint context	
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
**      07-jan-1995 (alison)
**          Partial Backup/Recovery Project:
**              Created.
**      26-jan-1995 (alison)
**          BUG 66403: apply proper case to table names
**      08-mar-1995 (stial01)
**          BUG 67327: automatically checkpoint indexes and blobs.
**      11-sep-1995 (thaju02)
**	    Database level checkpoint to create a file list of all
**	    tables in database at time of checkpoint.
**	10-jul-96 (nick)
**	    Error handling changes.
**      28-feb-2000 (gupsh01)
**          Enabled check for owner along with the table name
**          so that the table names in -table option can also
**          be specified as owner.tablename .
**	15-Aug-2005 (jenjo02)
**	    Implement multiple DMF LongTerm memory pools and
**	    ShortTerm session memory.
**	    Rename TBL_HCB to TBL_HCB_CB.
*/
static DB_STATUS
cpp_prepare_tbl_ckpt(
DMF_CPP		    *cpp,
DMF_JSX		    *jsx,
DMP_DCB		    *dcb)
{
    DB_STATUS		status;
    DMP_EXT             *ext;
    CPP_TBLHCB  	*tblhcb;
    i4		i;
    CPP_LOC_MASK	*lm =0;
    i4		size;
    char                tmp_name[DB_MAXNAME];
    i4			*err_code = &jsx->jsx_dberr.err_code;

    CLRDBERR(&jsx->jsx_dberr);

    cpp->cpp_jsx = jsx;

    /*
    ** Apply proper case to objects needing it.
    ** Adjustment performed in place, assuming that CKPDB 
    ** operates on only one database.
    */
    if (jsx->jsx_status & JSX_TBL_LIST) 
    {
	for (i = 0; i < jsx->jsx_tbl_cnt; i++)
	{
	    jsp_set_case(jsx, 
		    jsx->jsx_tbl_list[i].tbl_delim ? 
			jsx->jsx_delim_case : jsx->jsx_reg_case,
		    DB_MAXNAME, (char *)&jsx->jsx_tbl_list[i].tbl_name, 
		    tmp_name);

	    MEcopy(tmp_name, DB_MAXNAME, 
		    (char *)&jsx->jsx_tbl_list[i].tbl_name);

          if (STbcompare((char *)&jsx->jsx_tbl_list[i].tbl_owner.db_own_name,
                                         0,"", 0, 0) != 0)
            {
                jsp_set_case(jsx,
                        jsx->jsx_tbl_list[i].tbl_delim ?
                        jsx->jsx_delim_case : jsx->jsx_reg_case,
                        DB_MAXNAME, (char *)&jsx->jsx_tbl_list[i].tbl_owner.db_own_name,
                        tmp_name);

                MEcopy(tmp_name, DB_MAXNAME,
                        (char *)&jsx->jsx_tbl_list[i].tbl_owner.db_own_name);
            }
	}
    }

    do
    {
	/*
	** Allocate the hash bucket array for the tables
	*/
        /* (canor01) don't zero, since it is explicitly initialized below */
        status = dm0m_allocate(
	    sizeof(CPP_TBLHCB) + ( 63 * sizeof(TBL_HASH_ENTRY)),
            (i4)0, (i4)TBL_HCB_CB,
            (i4)CPP_TBLHCB_ASCII_ID, (char *)cpp,
            (DM_OBJECT **)&cpp->cpp_tblhcb_ptr, &jsx->jsx_dberr);
        if ( status != E_DB_OK )
	    break;

	/*
	** Initailize the hash bucket array
        */
        tblhcb = cpp->cpp_tblhcb_ptr;
	tblhcb->tblhcb_htotq.q_next = &tblhcb->tblhcb_htotq;
	tblhcb->tblhcb_htotq.q_prev = &tblhcb->tblhcb_htotq;
	tblhcb->tblhcb_hinvq.q_next = &tblhcb->tblhcb_hinvq;
	tblhcb->tblhcb_hinvq.q_prev = &tblhcb->tblhcb_hinvq;

        tblhcb->tblhcb_no_hash_buckets = 63;
        for (i = 0; i < tblhcb->tblhcb_no_hash_buckets; i++)
        {
            tblhcb->tblhcb_hash_array[i].tblhcb_headq_next = (CPP_TBLCB*)
                &tblhcb->tblhcb_hash_array[i].tblhcb_headq_next;
            tblhcb->tblhcb_hash_array[i].tblhcb_headq_prev = (CPP_TBLCB*)
                &tblhcb->tblhcb_hash_array[i].tblhcb_headq_next;
        }

	/*
	** Read tables and their respective tcb control blocks
	** before any check processing against whether the table
	** is valid or not!
	*/
	status = cpp_read_tables (cpp, jsx, dcb);
	if (status != E_DB_OK)
	{
	    dmfWriteMsg(&jsx->jsx_dberr, 0, 0);
	    return (status);
	}

        if (jsx->jsx_status1 & JSX1_CKPT_DB)
	    return(status);   

	/*
	** Determine which partitions,indexes need to be checkpointed 
	** for consistency:
	**
	**   Set CPP_PART_REQUIRED for all partitions on tables selected.
	**   Set CPP_INDEX_REQUIRED for all indexes on tables selected.
	** 
	**   If -nosecondary_index NOT specified,
	**     Set CPP_CKPT_TABLE for all indexes on tables selected.
	*/
	status = cpp_get_table_info (cpp, jsx, dcb);
	if (status != E_DB_OK)
	    break;

	/*
	** Determine which blobs need to be checkpointed for consistency
	**
	** Set CPP_BLOB_REQUIRED for all blob tables for tables selected.
	**
	** If -noblobs NOT specified,
	**    Set CPP_CKPT_TABLE for all blobs on tables selected.
	*/
	status = cpp_get_blob_info (cpp, jsx, dcb);
	if (status != E_DB_OK)
	    break;

	/*
	** table level checkpoint requires that the tables specified by
	** the user are valid
	*/
	status = cpp_ckpt_invalid_tbl (cpp, jsx, dcb);

	if (status != E_DB_OK)
	{
	    /*
	    ** Unable to open invalid tables which has been specified
	    ** by the user, therefore abort procedure
	    */
            return (status);
        }

	/*
	** Allocate the location mask structure, holds information
	** on what locations are interesting. Currently this is only
	** set whilst scanning list of tables to determine what
	** checkpoint files are around. In the future could set by
	** a -location switch to ckpdb.
	*/
        ext = dcb->dcb_ext;
        size = sizeof(i4) *
        (ext->ext_count+BITS_IN(i4)-1)/BITS_IN(i4);

	status = dm0m_allocate((i4)sizeof(CPP_LOC_MASK) + size,
				(i4)DM0M_ZERO, (i4)CPP_LOCM_CB,
				(i4)CPP_LOCM_ASCII_ID,
				(char *)cpp,
				(DM_OBJECT **)&lm, &jsx->jsx_dberr);
	if ( status != E_DB_OK )
	    break;

	lm->locm_bits = size * BITSPERBYTE;
	lm->locm_r_allow  = (i4 *)((char *)lm + sizeof(CPP_LOC_MASK));

	cpp->cpp_loc_mask_ptr = lm;

	/* FIX ME should we lock the tables we are checkpointing here ? */

	/*
	** Display information gathered sofar
	*/
	cpp_display_ckpt_tables( cpp, jsx );
	cpp_display_no_ckpt_tables( cpp, jsx );
	cpp_display_inv_tables( cpp, jsx );

    } while (FALSE);

    return( status );
}

/*{
** Name: cpp_read_tables - Get information on each table with a database
**
** Description:
**	This function will read all tables within the database and
**	allocate the tcb relating to each table. All tables
**	will be marked as CPP_TABLE_OK except where the table
**	is one of the following (system catalog, gateway, view)
**	or if the table is a secondary_index and the INDEX bit is set
**	or if the table is a blob and the BLOB bit is set
**
**
** Inputs:
**      cpp                             Pointer to checkpoint context.	
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
**      07-jan-1995 (alison)
**          Partial Backup/Recovery Project:
**		Created.
**      06-jan-1995 (stial01) 
**	    set bits for gateway/view/system catalog
**	 4-jul-96 (nick)
**	    Initialise rcb_k_mode on iirelation.
**	06-nov-96 (hanch04)
**	    Added dm0p_alloc_pgarray for partial table with VPS
**	30-may-1997 (nanpr01)
**	    There is no need to logically open the core catalog tables.
**	    It is already open physically when database was opened.
**      28-feb-2000 (gupsh01)
**          Enabled check for owner along with the table name
**          so that the table names in -table option can also
**          be specified as owner.tablename .
*/
static DB_STATUS
cpp_read_tables(
DMF_CPP             *cpp,
DMF_JSX             *jsx,
DMP_DCB             *dcb)
{
    DB_STATUS		status, loc_status = E_DB_OK;
    DB_TRAN_ID		tran_id;
    DB_OWN_NAME		username;
    DM_TID          	tid;
    DMP_RCB		*iirel_rcb = 0;
    DM2R_KEY_DESC	qual_list[1];
    DMP_RELATION	reltup;
    CPP_TBLCB		*tblcb;
    i4		i;
    i4			*err_code = &jsx->jsx_dberr.err_code;

    CLRDBERR(&jsx->jsx_dberr);
	     
    /* Begin a transaction */
    {
        char        *cp = (char *)&username;

        IDname(&cp);
        MEmove(STlength(cp), cp, ' ', sizeof(username), username.db_own_name);
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
	status = cpp_allocate_tblcb(jsx, cpp, &tblcb);
        if ( status != E_DB_OK )
        {
	    dmfWriteMsg(&jsx->jsx_dberr, 0, 0);
	    break;
        }

	tblcb->tblcb_table_name = reltup.relid;
	tblcb->tblcb_table_owner = reltup.relowner;
	tblcb->tblcb_table_id = reltup.reltid;

	if (reltup.relstat & TCB_CATALOG)
	    tblcb->tblcb_table_status |= CPP_CATALOG;
	if (reltup.relstat & TCB_VIEW)
	    tblcb->tblcb_table_status |= CPP_VIEW;
	if (reltup.relstat & TCB_GATEWAY)
	    tblcb->tblcb_table_status |= CPP_GATEWAY;
	if (reltup.relstat2 & TCB2_PARTITION)
	    tblcb->tblcb_table_status |= CPP_PARTITION;
	if (reltup.relstat & TCB_IS_PARTITIONED)
	    tblcb->tblcb_table_status |= CPP_IS_PARTITIONED;
	if ( !(jsx->jsx_status1 & JSX1_CKPT_DB) &&
	    ((reltup.relstat2 & TCB2_TBL_RECOVERY_DISALLOWED) ||
	    !(reltup.relstat & TCB_JOURNAL)) )
	    tblcb->tblcb_table_status |= CPP_TBL_CKP_DISALLOWED;

        /* Check for user specified table in list */
        for ( i = (jsx->jsx_tbl_cnt-1); i >= 0; i--)
        {
            if ((MEcmp (&tblcb->tblcb_table_name,
                        &jsx->jsx_tbl_list[i].tbl_name.db_tab_name,
			DB_MAXNAME)) == 0)
            {
                /*
                ** Table found, check if this is a system catalog
		*/
                if (STbcompare((char *)&jsx->jsx_tbl_list[i].tbl_owner.db_own_name,
                                       0,"", 0, 0) != 0)
                {
                /* Check for owner name */
                        if((MEcmp (tblcb->tblcb_table_owner.db_own_name,
                                &jsx->jsx_tbl_list[i].tbl_owner.db_own_name
                                                       ,DB_MAXNAME)) == 0)
                        {
                                tblcb->tblcb_table_status |= CPP_USER_SPECIFIED;
                                tblcb->tblcb_table_status |= CPP_CKPT_TABLE;
                        }
                }
                else
                {
                        tblcb->tblcb_table_status |= CPP_USER_SPECIFIED;
                        tblcb->tblcb_table_status |= CPP_CKPT_TABLE;
                }
		/*
		** Make sure a buffer cache was allocated for the pagesize
		*/
		if (!(dm0p_has_buffers(reltup.relpgsize)))
		{
		    loc_status = dm0p_alloc_pgarray(reltup.relpgsize, &jsx->jsx_dberr);
		    if (loc_status != E_DB_OK)
			break;
		}

            }
        }
	if (loc_status != E_DB_OK)
	{
	    dmfWriteMsg(&jsx->jsx_dberr, 0, 0);
	    status = loc_status;
	    break;
	}

	/*
	** If everything is OK, add to list
	*/
	cpp_add_tblcb (cpp, tblcb);

    }

    /*
    ** Deallocate the RCB structure
    */
    if ( iirel_rcb )
    {
        loc_status = dm2r_release_rcb( &iirel_rcb, &jsx->jsx_dberr );
        if ( loc_status != E_DB_OK )
        {
	    dmfWriteMsg(&jsx->jsx_dberr, 0, 0);
	    return (loc_status);
        }
    }
    return( status );
}

/*{
** Name: cpp_allocate_tblcb - Allocate a new tbl control block.
**
** Description:
**
**
** Inputs:
**      cpp	
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
**      07-jan-1995 (alison)
**	    Partial Backup/Recovery Project:
**		Created.
**      19-oct-1995 (stial01)
**          relation tuple buffer allocated incorrectly
**      15-jun-1999 (hweho01)
**          Corrected argument mismatch in ule_format() call,  
**          the first one should be an integer, not a pointer. 
*/
static DB_STATUS
cpp_allocate_tblcb(
DMF_JSX		    *jsx,
DMF_CPP             *cpp,
CPP_TBLCB	    **tablecb)
{
    CPP_TBLCB	    *tblcb;
    DB_STATUS	    status;
    i4			*err_code = &jsx->jsx_dberr.err_code;

    CLRDBERR(&jsx->jsx_dberr);

    status = dm0m_allocate((i4)sizeof( CPP_TBLCB ),
            (i4)DM0M_ZERO, (i4)TBL_CB, (i4)CPP_TBLCB_ASCII_ID,
            (char *)cpp, (DM_OBJECT **)&tblcb, &jsx->jsx_dberr);
    if (status != E_DB_OK)
    {
	dmfWriteMsg(&jsx->jsx_dberr, 0, 0);
        uleFormat(&jsx->jsx_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,(char *)NULL,
                (i4)0, (i4*)NULL, err_code, 0);
        return( E_DB_ERROR );
    }

    tblcb->tblcb_table_status = 0;

    tblcb->tblcb_q_next      =  (CPP_TBLCB *)&tblcb->tblcb_q_next;
    tblcb->tblcb_q_prev      =  (CPP_TBLCB *)&tblcb->tblcb_q_next;
    tblcb->tblcb_totq.q_next = &tblcb->tblcb_totq;
    tblcb->tblcb_totq.q_prev = &tblcb->tblcb_totq;
    tblcb->tblcb_invq.q_next = &tblcb->tblcb_invq;
    tblcb->tblcb_invq.q_prev = &tblcb->tblcb_invq;

    *tablecb = tblcb;

    return( E_DB_OK );

}

/*{
** Name: cpp_add_tblcb - Add tblcb to relevent structures
**
** Description:
**
**
** Inputs:
**      cpp	
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
**      07-jan-1995 (alison)
**	    Partial Backup/Recovery Project:
**		Created.
**      23-mar-1995 (stial01)
**          Don't fix prev pointer of htotalq unless there
**          is an element on the htotalq. Otherwise we trash 
**          hcb->tblhcb_hash_array[3].
*/
static VOID
cpp_add_tblcb(
DMF_CPP             *cpp,
CPP_TBLCB	    *tblcb )
{
    CPP_TBLHCB	        *hcb = cpp->cpp_tblhcb_ptr;
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
    tblcb->tblcb_q_prev = (CPP_TBLCB *)&h->tblhcb_headq_next;

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
** Name: cpp_locate_tblcb - Locate a tblcb via table_id.
**
** Description:
**
**
** Inputs:
**      cpp	
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
**      07-jan-1995 (alison)
**	    Partial Backup/Recovery Project:
**		Created.
*/
static bool
cpp_locate_tblcb(
DMF_CPP             *cpp,
CPP_TBLCB	    **tblcb,
DB_TAB_ID	    *tabid)
{
    CPP_TBLHCB	        *hcb = cpp->cpp_tblhcb_ptr;
    TBL_HASH_ENTRY	*h;
    CPP_TBLCB		*t;


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
	  t != (CPP_TBLCB*)&h->tblhcb_headq_next;
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
** Name: cpp_display_ckpt_tables
**
** Description:
**
**
** Inputs:
**      cpp                             Pointer to checkpoint support context
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
**      07-jan-1995 (alison)
**	    Partial Backup/Recovery Project:
**		Created.
*/
static VOID
cpp_display_ckpt_tables(
DMF_CPP             *cpp,
DMF_JSX             *jsx )
{
    char		line_buffer[132];
    CPP_QUEUE		*totq;
    CPP_TBLCB           *tblcb = 0;
    CPP_TBLHCB          *hcb = cpp->cpp_tblhcb_ptr;
    bool		none = TRUE;

    TRformat( dmf_put_line, 0, line_buffer, sizeof(line_buffer),
                    "The following tables will be checkpointed : ");

    for ( totq = hcb->tblhcb_htotq.q_next;
          totq != &hcb->tblhcb_htotq;
          totq = tblcb->tblcb_totq.q_next
          )
    {
	tblcb = (CPP_TBLCB *)((char *)totq - CL_OFFSETOF(CPP_TBLCB, tblcb_totq));

        /*
	** If not table will not be checkpointed, skip it
        */
        if ((tblcb->tblcb_table_status & CPP_CKPT_TABLE ) == 0)
            continue;

	TRformat( dmf_put_line, 0, line_buffer, sizeof(line_buffer),
		       " %~t\n", DB_MAXNAME, &tblcb->tblcb_table_name );

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
** Name: cpp_display_no_ckpt_tables
**
** Description:
**
**
** Inputs:
**      cpp                             Pointer to checkpoint context.
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
**      07-jan-1995 (alison)
**	    Partial Backup/Recovery Project:
**		Created.
*/
static VOID
cpp_display_no_ckpt_tables(
DMF_CPP             *cpp,
DMF_JSX             *jsx )
{
    char		line_buffer[132];
    CPP_QUEUE		*totq;
    CPP_TBLCB           *tblcb = 0;
    CPP_TBLHCB          *hcb = cpp->cpp_tblhcb_ptr;
    bool		none = TRUE;

    TRformat( dmf_put_line, 0, line_buffer, sizeof(line_buffer),
 "Additional tables required to maintain physical consistency of the database :"
	     );

    for ( totq = hcb->tblhcb_htotq.q_next;
          totq != &hcb->tblhcb_htotq;
          totq = tblcb->tblcb_totq.q_next
          )
    {
	tblcb = (CPP_TBLCB *)((char *)totq - CL_OFFSETOF(CPP_TBLCB, tblcb_totq));

        /*
	** If not table will not be checkpointed, skip it
        */
        if (tblcb->tblcb_table_status & CPP_CKPT_TABLE)
            continue;

	/*
	** Skip catalogs, views, gateway and partition tables
	** (tables we can't table checkpoint)
	*/
	if (tblcb->tblcb_table_status & 
	    (CPP_CATALOG | CPP_VIEW | CPP_GATEWAY | CPP_PARTITION))
	    continue;

	none = FALSE;

	TRformat( dmf_put_line, 0, line_buffer, sizeof(line_buffer),
		       " %~t\n", DB_MAXNAME, &tblcb->tblcb_table_name );

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
** Name: cpp_display_inv_tables
**
** Description:
**
**
** Inputs:
**      cpp                             Pointer to checkpoint context.	
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
**      07-jan-1995 (alison)
**	    Partial Backup/Recovery Project:
**		Created.
*/
static VOID
cpp_display_inv_tables(
DMF_CPP             *cpp,
DMF_JSX             *jsx )
{
    char		line_buffer[132];
    CPP_QUEUE		*invq;
    CPP_TBLCB           *tblcb = 0;
    CPP_TBLHCB          *hcb = cpp->cpp_tblhcb_ptr;
    bool		none = TRUE;

    TRformat( dmf_put_line, 0, line_buffer, sizeof(line_buffer),
                    "Tables found to be invalid : ");

    for ( invq = hcb->tblhcb_hinvq.q_next;
	  invq != &hcb->tblhcb_hinvq;
	  invq = tblcb->tblcb_invq.q_next)
    {

	tblcb = (CPP_TBLCB *)((char *)invq - 
			    CL_OFFSETOF(CPP_TBLCB, tblcb_invq));

        /*
	** If not an invalid table then skip it
        */
        if ((tblcb->tblcb_table_status & CPP_INVALID_TABLE ) == 0)
            continue;

	none = FALSE;

	TRformat( dmf_put_line, 0, line_buffer, sizeof(line_buffer),
		       "    %~t - err_code: %d\n",
		       DB_MAXNAME, &tblcb->tblcb_table_name,
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

/*
** Name:
**
** Description: cpp_ckpt_invalid_tbl - Returns true if checkpointing an
**			                invalid table
**
**
** Inputs:
**	jsx				Pointer to journal support context.
**
** Outputs:
**      cpp                             Checkpoint context.
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
**      07-jan-1995 (alison)
**	    Partial Backup/Recovery Project:
**	 	Created.
**	30-jun-98 (thaju02)
**	    Disallow tbl lvl ckpdb, if table recovery disallowed or
**	    table is not journaled. (B91356)
**      28-feb-2000 (gupsh01)
**          Enabled check for owner along with the table name
**          so that the table names in -table option can also
**          be specified as owner.tablename .
*/
static DB_STATUS
cpp_ckpt_invalid_tbl(
DMF_CPP		    *cpp,
DMF_JSX		    *jsx,
DMP_DCB		    *dcb)
{
    CPP_QUEUE		*totq;
    CPP_TBLCB           *tblcb = 0;
    CPP_TBLHCB          *hcb = cpp->cpp_tblhcb_ptr;
    i4		i;
    bool 		table_found = FALSE;
    i4			*err_code = &jsx->jsx_dberr.err_code;

    CLRDBERR(&jsx->jsx_dberr);

    /*
    ** Loop through all the user supplied tables gathering more info
    */
    for ( totq = hcb->tblhcb_htotq.q_next;
          totq != &hcb->tblhcb_htotq;
          totq = tblcb->tblcb_totq.q_next
          )
    {
	tblcb = (CPP_TBLCB *)((char *)totq - CL_OFFSETOF(CPP_TBLCB, tblcb_totq));

	if ((tblcb->tblcb_table_status & CPP_USER_SPECIFIED) == 0)
	    continue;

	if (tblcb->tblcb_table_status & CPP_CATALOG)
	{
            /*
            ** Presently unable to checkpoint core DBMS catalogs,
	    ** so display and log error message.
            */
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM1161_CPP_NO_CORE_TBL_CKPT);
	    return (E_DB_ERROR);
	}

	if (tblcb->tblcb_table_status & CPP_TBL_CKP_DISALLOWED)
	{
	    /* table ckp disallowed or table is not journaled */
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM116E_CPP_TBL_CKP_DISALLOWED);
	    return(E_DB_ERROR);
	}

	if (tblcb->tblcb_table_status &
		(CPP_CATALOG | CPP_VIEW | CPP_GATEWAY | CPP_PARTITION))
        {
	    /* Table cannot be explicitly checkpointed */
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM1162_CPP_USR_INVALID_TBL);
	    return (E_DB_ERROR);
	}
    }

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
	    tblcb = (CPP_TBLCB *)((char *)totq - 
				CL_OFFSETOF(CPP_TBLCB, tblcb_totq));

	    if ((MEcmp (&tblcb->tblcb_table_name,
			&jsx->jsx_tbl_list[i].tbl_name.db_tab_name,
			DB_MAXNAME)) == 0)
            {
                if (STbcompare((char *)&jsx->jsx_tbl_list[i].tbl_owner.db_own_name,
                                       0,"", 0, 0) != 0)
                {
                /* Check for owner name */
                        if((MEcmp (tblcb->tblcb_table_owner.db_own_name,
                                &jsx->jsx_tbl_list[i].tbl_owner.db_own_name
                                                       ,DB_MAXNAME)) == 0)
                        {
                                table_found = TRUE;
                        }
                }
                else
                {
                        table_found = TRUE;
                }
	    }
        }
	if (!table_found)
	{
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM1162_CPP_USR_INVALID_TBL);
	    return (E_DB_ERROR);
	}
    }
    return (E_DB_OK);
}

/*{
** Name: cpp_prepare_save_tbl   Prepare to save subset of tables in db 
**
** Description:
**      This routine determines which files (if any) at this location
**      are interesting for partial backup.
**
** Inputs:
**      cpp                             Pointer to checkpoint context
**	jsx				Pointer to Journal support info.
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
**      07-jan-1995 (alison)
**	    Partial Backup/Recovery Project:
**	 	Created.
**	29-jan-1995 (rudtr01)
**	    Fixed bug 66571 by modifying cpp_prepare_save_tbl() to 
**	    separate the filenames by a comma instead of a blank.
**      23-mar-1995 (stial01)
**          fixed calculation of offset of next filename in 
**          the cpp_fnames_at_loc buffer.
**	16-jun-1997 (nanpr01)
**	    Yet another hole. Referencing memory after the memory was 
**	    deallocated.
*/
static DB_STATUS
cpp_prepare_save_tbl(
DMF_CPP     *cpp,
DMF_JSX	    *jsx,
DMP_DCB	    *dcb)
{
    CPP_QUEUE		*totq;
    CPP_TBLCB           *tblcb = 0;
    CPP_TBLHCB          *hcb = cpp->cpp_tblhcb_ptr;
    CPP_LOC_MASK        *lm  = cpp->cpp_loc_mask_ptr;
    i4             i;
    DMP_LOCATION        *l;
    DMP_EXT             *ext = dcb->dcb_ext;
    DB_STATUS           status, close_status;
    DMP_TCB             *tcb;
    DMP_TCB		*master_tcb = NULL;
    DB_TAB_ID		master_id;
    DMP_TABLE_IO        *tbio;
    char                *cp;
    DB_TRAN_ID          tran_id;
    i4             lock_id;
    i4             log_id;
    DB_TAB_TIMESTAMP    ctime;
    DB_OWN_NAME         username;
    i4             files_at_loc_count[ DM_LOC_MAX ];
    i4             size;
    CPP_FNMS            *cpp_fnms = 0;
    i4			local_error;
    DB_STATUS		local_status;
    DB_ERROR		local_dberr;
    i4			*err_code = &jsx->jsx_dberr.err_code;

    CLRDBERR(&jsx->jsx_dberr);

    /*
    ** Initialize count of files at each location
    */
    for ( i = 0; i < ext->ext_count; i++)
    {
	cpp->cpp_fcnt_at_loc[i] = 0;
	files_at_loc_count[i] = 0;
    }

    cp = (char *)&username;
    IDname(&cp);
    MEmove(STlength(cp), cp, ' ', sizeof(username), username.db_own_name);

    do 
    {
	/* Begin a transaction */
	status = dmf_jsp_begin_xact(DMXE_READ, 0, dcb, dcb->dcb_log_id,
		&username, dmf_svcb->svcb_lock_list, &log_id, &tran_id,  
		&lock_id, &jsx->jsx_dberr);

	if (status != E_DB_OK)
	    break;

	/*
	** Scan through the list of tables to determine the following info
	** - What locations are of interest
	** - Total number of files that will be processed
	** - Total number of files at each location
	**
	*/
	for ( totq = hcb->tblhcb_htotq.q_next;
	      totq != &hcb->tblhcb_htotq;
	      totq = tblcb->tblcb_totq.q_next
	      )
	{
	    tblcb = (CPP_TBLCB *)((char *)totq - 
				CL_OFFSETOF(CPP_TBLCB, tblcb_totq));

	    /*
	    ** If for some reason recovery is turned off on this table
	    ** then skip it
	    */
	    if ((tblcb->tblcb_table_status & CPP_CKPT_TABLE) == 0)
		continue;

	    /* For partitioned tables, get file info for each partition */
	    if (tblcb->tblcb_table_status & CPP_IS_PARTITIONED)
		continue;

	    master_tcb = NULL;
	    if (tblcb->tblcb_table_status & CPP_PARTITION)
	    {
		master_id.db_tab_base = tblcb->tblcb_table_id.db_tab_base;
		master_id.db_tab_index = 0;
		status = dm2t_fix_tcb (dcb->dcb_id, &master_id, &tran_id,
			   dmf_svcb->svcb_lock_list, log_id, 
		   	       DM2T_RECOVERY, /* this means don't open file */
			       dcb, &master_tcb, &jsx->jsx_dberr);
	    }

	    if ( status == E_DB_OK )
		status = dm2t_fix_tcb (dcb->dcb_id,
			       &tblcb->tblcb_table_id,
			       &tran_id,
			       dmf_svcb->svcb_lock_list,
			       log_id,
		   	       DM2T_RECOVERY, /* this means don't open file */
			       dcb,
		 	       &tcb,
			       &jsx->jsx_dberr);

	    if (status != E_DB_OK)
	    {
		dmfWriteMsg(&jsx->jsx_dberr, 0, 0);
		break;
	    }

	    tbio = &tcb->tcb_table_io;

	    /*
	    ** Mark the corresponding bits for this table used,
	    ** all locations are of interest.
	    */
	    for ( i = 0; i < tbio->tbio_loc_count; i++)
	    {
		l = &tbio->tbio_location_array[i];

		/*
		** Indicate that this location is of interest
		*/
		if ( BTtest( l->loc_config_id, (char *)lm->locm_r_allow ) 
						== FALSE)
		{
		    BTset( l->loc_config_id, (char *)lm->locm_r_allow );
		}

		/*
		** Increase the count of files that will be processed at this
		** location
		*/
		cpp->cpp_fcnt_at_loc[ l->loc_config_id ]++;
	    }

	    status = dm2t_unfix_tcb( &tcb, (DM2T_PURGE | DM2T_NORELUPDAT),
		           	 dmf_svcb->svcb_lock_list, log_id, &jsx->jsx_dberr );
	    if (status != E_DB_OK)
	    {
		dmfWriteMsg(&jsx->jsx_dberr, 0, 0);
		break;
	    }

	    if (master_tcb)
	    {
		status = dm2t_unfix_tcb(&master_tcb, 
			    (DM2T_PURGE | DM2T_NORELUPDAT),
			     dmf_svcb->svcb_lock_list, log_id, &jsx->jsx_dberr );

		if (status != E_DB_OK)
		{
		    dmfWriteMsg(&jsx->jsx_dberr, 0, 0);
		    break;
		}
	    }
	}   

	if (status != E_DB_OK)
	    break;

	/*
	** Now that we know the total number of tables being checkpointed,
	** allocate space to hold the list of tablenames
	** FIX ME
	** Note list of tablenames currently not printed by dmckp
	*/

	/*
	** Now that we know the total number of files at each location,
	** allocate space to hold the list of filenames at each location.
	*/
	for ( i = 0; i < ext->ext_count; i++)
	{
	    if (cpp->cpp_fcnt_at_loc[i])
	    {
		/* size = file count at this loc * (filename len + 1 blank) */
		size = cpp->cpp_fcnt_at_loc[i] * ((sizeof (DM_FILE)) + 1);

		/* (canor01) don't zero, since it is explicitly initialized below */
		status = dm0m_allocate((i4)sizeof(CPP_FNMS) + size,
			(i4)0,
			(i4)CPP_FNMS_CB,
			(i4)CPP_FNMS_ASCII_ID,
			(char *)cpp,
			(DM_OBJECT **)&cpp_fnms,
			&jsx->jsx_dberr);
		if (status != E_DB_OK)
		{
		    dmfWriteMsg(&jsx->jsx_dberr, 0, 0);
		    break;
		}

		cpp->cpp_fnames_at_loc[i] = cpp_fnms;
		cpp_fnms->cpp_fnms_str = 
			(char *)((char *)cpp_fnms + sizeof(CPP_FNMS));
	    }
	}

	if (status != E_DB_OK)
	    break;

	/*
	** If no error so far....
	** Scan through the list of tables to build a list of filenames to be
	** saved at each location.
	** Also make a list of tables being checkpointed FIX ME
	*/
	for ( totq = hcb->tblhcb_htotq.q_next;
	      totq != &hcb->tblhcb_htotq;
	      totq = tblcb->tblcb_totq.q_next
	      )
	{
	    tblcb = (CPP_TBLCB *)((char *)totq - 
				    CL_OFFSETOF(CPP_TBLCB, tblcb_totq));

	    /*
	    ** If for some reason recovery is turned off on this table
	    ** then skip it
	    */
	    if ((tblcb->tblcb_table_status & CPP_CKPT_TABLE) == 0)
		continue;

	    /* For partitioned tables, get file info for each partition */
	    if (tblcb->tblcb_table_status & CPP_IS_PARTITIONED)
		continue;

	    master_tcb = NULL;
	    if (tblcb->tblcb_table_status & CPP_PARTITION)
	    {
		master_id.db_tab_base = tblcb->tblcb_table_id.db_tab_base;
		master_id.db_tab_index = 0;
		status = dm2t_fix_tcb (dcb->dcb_id, &master_id, &tran_id,
			   dmf_svcb->svcb_lock_list, log_id, 
		   	       DM2T_RECOVERY, /* this means don't open file */
			       dcb, &master_tcb, &jsx->jsx_dberr);
	    }

	    if (status == E_DB_OK)
		status = dm2t_fix_tcb (dcb->dcb_id,
			       &tblcb->tblcb_table_id,
			       &tran_id,
			       dmf_svcb->svcb_lock_list,
			       log_id,
		   	       DM2T_RECOVERY, /* this means don't open file */
			       dcb,
		 	       &tcb,
			       &jsx->jsx_dberr);

	    if (status != E_DB_OK)
	    {
		dmfWriteMsg(&jsx->jsx_dberr, 0, 0);
		break;
	    }

	    tbio = &tcb->tcb_table_io;

	    /*
	    ** Look at each area this table is located in
	    */
	    for ( i = 0; i < tbio->tbio_loc_count; i++)
	    {
		l = &tbio->tbio_location_array[i];

		/* Add the filename for this table to the list of files
		** we are interested in at this location
		*/
		cp = cpp->cpp_fnames_at_loc[ l->loc_config_id]->cpp_fnms_str;
		if (files_at_loc_count[ l->loc_config_id ])
		{
		    /* not the first file, skip the filenames and blanks */
		    cp += ((sizeof (DM_FILE)) + 1) *
		    	files_at_loc_count[l->loc_config_id]; 
		}

		MEmove(l->loc_fcb->fcb_namelength,
			l->loc_fcb->fcb_filename.name,
#ifdef VMS
		   ',', 
#else 
		   ' ',
#endif
			sizeof(DM_FILE) + 1,
			cp);

		/* 
		** Increment number of file names for this loc
		*/
		files_at_loc_count[ l->loc_config_id ]++;
	    }

	    status = dm2t_unfix_tcb( &tcb, (DM2T_PURGE | DM2T_NORELUPDAT),
		           	 dmf_svcb->svcb_lock_list, log_id, &jsx->jsx_dberr );
	    if (status != E_DB_OK)
	    {
		dmfWriteMsg(&jsx->jsx_dberr, 0, 0);
		break;
	    }

	    if (master_tcb)
	    {
		status = dm2t_unfix_tcb( &master_tcb, 
			    (DM2T_PURGE | DM2T_NORELUPDAT),
			     dmf_svcb->svcb_lock_list, log_id, &jsx->jsx_dberr );

		if (status != E_DB_OK)
		{
		    dmfWriteMsg(&jsx->jsx_dberr, 0, 0);
		    break;
		}
	    }
	}
    } while (FALSE);

    if (master_tcb)
    {
	local_status = dm2t_unfix_tcb( &master_tcb, 
		    (DM2T_PURGE | DM2T_NORELUPDAT),
		     dmf_svcb->svcb_lock_list, log_id, &local_dberr );
    }

#ifdef VMS
    /*
    ** For each location, fix the list of filenames to remove the trailing
    ** comma left there by the loop that built it just above.       
    */
    for ( i = 0; i < ext->ext_count; i++)
    {
	if (cpp->cpp_fcnt_at_loc[i])
	{
	    cp = cpp->cpp_fnames_at_loc[ i ]->cpp_fnms_str;
	    *(cp + (cpp->cpp_fcnt_at_loc[i] * (sizeof(DM_FILE) + 1)) - 1) = ' ';
	}
    }
#endif

    if (log_id)
    {
	close_status = dmf_jsp_commit_xact(&tran_id, log_id, lock_id,
			DMXE_ROTRAN, &ctime, &local_dberr);
	if (close_status != E_DB_OK && status == E_DB_OK)
	{
	    jsx->jsx_dberr = local_dberr;
	    status = close_status;
	}
    }

    return (status);
}

/*{
** Name: cpp_get_table_info - Determine partitions and indexes for tables 
**				being checkpointed
**
** Description:
**      For all user specified tables, this function will determine if 
**      there are any associated secondary indexes that need to be checkpointed.
**      Secondary indexes will be marked CPP_INDEX_REQUIRED.       
**      They will also be marked CPP_CKPT_TABLE unless -nosecondary_index 
**      is specified.
**
**
** Inputs:
**	cpp
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
**      08-mar-1995 (stial01)
**          BUG 67327: 
**		Created.
**	14-Jan-2004 (jenjo02)
**	    Check db_tab_index > 0 for index, not simply != 0.
*/
static DB_STATUS
cpp_get_table_info(
DMF_CPP             *cpp,
DMF_JSX             *jsx,
DMP_DCB             *dcb)
{
    CPP_QUEUE		*totq;
    CPP_QUEUE		*totq2;
    CPP_TBLCB           *tblcb, *tblcb2;
    CPP_TBLHCB          *hcb = cpp->cpp_tblhcb_ptr;

    /*
    ** Loop through all the user supplied tables gathering more info
    */
    for ( totq = hcb->tblhcb_htotq.q_next;
          totq != &hcb->tblhcb_htotq;
          totq = tblcb->tblcb_totq.q_next
          )
    {
	tblcb = (CPP_TBLCB *)((char *)totq - CL_OFFSETOF(CPP_TBLCB, tblcb_totq));

	/* Skip table if not user specified */
        if ( (tblcb->tblcb_table_status & CPP_USER_SPECIFIED) == 0)
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
	    tblcb2 = (CPP_TBLCB *)((char *)totq2 - 
				CL_OFFSETOF(CPP_TBLCB, tblcb_totq));

	    if (tblcb2->tblcb_table_id.db_tab_base !=
		   tblcb->tblcb_table_id.db_tab_base
		 || tblcb2 == tblcb)
		continue;

	    if (tblcb2->tblcb_table_id.db_tab_index > 0)
	    {
		tblcb2->tblcb_table_status |= CPP_INDEX_REQUIRED;

		/* Unless -noseconary_index specified, recover the index */
		if (( jsx->jsx_status1 & JSX1_NOSEC_IND ) == 0)	
		    tblcb2->tblcb_table_status |= CPP_CKPT_TABLE;
	    }
	    else if (tblcb2->tblcb_table_id.db_tab_index < 0)
	    {
		tblcb2->tblcb_table_status |= CPP_PART_REQUIRED;
		tblcb2->tblcb_table_status |= CPP_CKPT_TABLE;
	    }
	}
    }

    return (E_DB_OK);

}


/*{
** Name: cpp_get_blob_info - Determine blob tables for tables 
**				being checkpointed 
**
** Description:
**      For all user specified tables, this function will determine if 
**      there are any associated blob tables that need to be recovered.
**      Blob tables will be marked CPP_BLOB_REQUIRED.       
**      They will also be marked CPP_CKPT_TABLE unless -noblobs specified.
**
**
** Inputs:
**	cpp
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
**      08-mar-1995 (stial01)
**          BUG 67327: 
**		Created.
**	10-jul-96 (nick)
**	    Remove unused variables and magic number.
*/
static DB_STATUS
cpp_get_blob_info(
DMF_CPP             *cpp,
DMF_JSX             *jsx,
DMP_DCB             *dcb)
{
    CPP_QUEUE		*totq;
    CPP_TBLCB           *tblcb;
    CPP_TBLHCB          *hcb = cpp->cpp_tblhcb_ptr;
    DB_TAB_ID           iiextended_rel;
    DB_TAB_ID           blob_tabid;
    DMP_RCB		*etab_rcb = 0;
    DM2R_KEY_DESC	key_desc[1];
    DMP_ETAB_CATALOG	etab_rec;
    DM_TID		tid;
    i4		local_error;
    DB_STATUS		local_status;
    DB_STATUS		status;
    DB_TRAN_ID          tran_id;
    DB_TAB_TIMESTAMP    stamp;
    CPP_TBLCB           *blob_tblcb;
    DB_OWN_NAME		username;
    DB_ERROR		local_dberr;
    i4			*err_code = &jsx->jsx_dberr.err_code;

    CLRDBERR(&jsx->jsx_dberr);

    /* Begin a transaction */
    {
        char        *cp = (char *)&username;

        IDname(&cp);
        MEmove(STlength(cp), cp, ' ', sizeof(username), username.db_own_name);
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
	    if (jsx->jsx_dberr.err_code == E_DM0054_NONEXISTENT_TABLE)
	    {
		status = E_DB_OK;
		CLRDBERR(&jsx->jsx_dberr);
	    }
	    else
	    {
		/* error during the opening of iiextended_relation */
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
	    tblcb = (CPP_TBLCB *)((char *)totq - 
				CL_OFFSETOF(CPP_TBLCB, tblcb_totq));

	    /* Skip table if not user specified */
	    if ((tblcb->tblcb_table_status & CPP_USER_SPECIFIED ) == 0)
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
	    ** corresponding TBLCB entry as CPP_BLOB_REQUIRED.
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

		if (cpp_locate_tblcb( cpp, &blob_tblcb, &blob_tabid))
		{
		    blob_tblcb->tblcb_table_status |= CPP_BLOB_REQUIRED;

		    /* Unless -noblobs specified, recover the blob */
		    if (( jsx->jsx_status1 & JSX1_NOBLOBS ) == 0)
			blob_tblcb->tblcb_table_status |= CPP_CKPT_TABLE;
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
		jsx->jsx_dberr = local_dberr;
		status = local_status;
	    }
	}
    }

    return (status);
}

/*{
** Name: cpp_save_chkpt_tbl_list - Save a list of the tables being checkpointed
**
** Description:
**      For table-level checkpoints, save the list of user-specified tables
** 	in a special file. The file is named like the checkpoint files:
** 	Cssss000.LST, where ssss is the sequence number of the checkpoint.
** 	Unlike checkpoint files, there is only one .LST file per checkpoint,
** 	not one for each location; hence the 000 in the file name.
**
** Inputs:
**      cpp                             Pointer to checkpoint context
**	jsx				Pointer to Journal support info.
**      dcb				Pointer to DCB for this database.
** 	ckpseq				Checkpoint sequence number.
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
**
** History:
** 	01-jun-1995 (wonst02) 
** 	    Bug 68886, 68866
** 	    Added to save the list of tables checkpointed.
**      12-jun-1995 (thaju02)
**          Bug #67327: Modified cpp_save_chkpt_tbl_list, such that indexes
**          and blobs which are associated to the table(s) that are specified
**          for table level checkpoint are added to the table level
**          checkpoint list file.  Note: if -nosecondary_index and/or
**          -noblobs options are added to table level checkpoint another
**          check of CPP_CKPT_TABLE MUST be added to cpp_save_chkpt_tbl_list.
**	11-sep-1995 (thaju02)
**	    Database level checkpoint to create a file list of all
**	    tables in database at time of checkpoint.
**	 6-feb-1996 (nick)
**	    Rewrite to fix #74453.
**      29-sep-2008 (ashco01) Bug: 120558
**          Ensure that the end-of-list marker is written even if this is 
**          the last entry within the current block.
*/
static DB_STATUS
cpp_save_chkpt_tbl_list(
DMF_CPP         *cpp,
DMF_JSX	    	*jsx,
DMP_DCB	    	*dcb,
i4		ckpseq)
{
    CPP_QUEUE		*totq;
    CPP_TBLCB           *tblcb;
    CPP_TBLHCB          *hcb = cpp->cpp_tblhcb_ptr;
    DB_TAB_NAME		*table_list;
    TBLLST_CTX		tblctx;
    DB_STATUS       	status = E_DB_OK;
    DB_STATUS       	local_status;
    i4             i;
    i4		local_err;
    i4			*err_code = &jsx->jsx_dberr.err_code;

    CLRDBERR(&jsx->jsx_dberr);

    do
    {
    	tblctx.seq = ckpseq;
	tblctx.blkno = -1;
    	tblctx.blksz = TBLLST_BKZ;
    	tblctx.phys_length = dcb->dcb_dmp->phys_length;
    	STRUCT_ASSIGN_MACRO(dcb->dcb_dmp->physical, tblctx.physical);

	while (status == E_DB_OK)
	{
    	    status = tbllst_create(jsx, &tblctx);

	    if (status == E_DB_OK)
		break;

    	    if (status == E_DB_WARN)
	    {
		/* file exists so delete and try again */
		status = tbllst_delete(jsx, &tblctx);
	    }
	}
	if (status != E_DB_OK)
	{
	    uleFormat(&jsx->jsx_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
	    	(char * )0, 0L, (i4 *)NULL, &local_err, 0);
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM1165_CPP_CHKPT_LST_CREATE);
	    break;
	}
	status = tbllst_open(jsx, &tblctx);
	if (status != E_DB_OK)
	{
	    uleFormat(&jsx->jsx_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
	    	(char * )0, 0L, (i4 *)NULL, &local_err, 0);
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM1166_CPP_CHKPT_LST_OPEN);
	    break;
	}

	table_list = (DB_TAB_NAME *)&tblctx.tbuf;

	/*
	** Loop through all the user-supplied tables 
	*/
	i = 0;
	for ( totq = hcb->tblhcb_htotq.q_next;
	      totq != &hcb->tblhcb_htotq;
	      totq = tblcb->tblcb_totq.q_next
	      )
	{
	    tblcb = (CPP_TBLCB *)((char *)totq - 
				CL_OFFSETOF(CPP_TBLCB, tblcb_totq));

	    if (jsx->jsx_status1 & JSX1_CKPT_DB)
	    {
		/* 
		** Db level ckp 
		**	Skip system catalogs, gateway tables, views, partitions
		*/
		if (tblcb->tblcb_table_status &
			(CPP_CATALOG | CPP_VIEW | CPP_GATEWAY | CPP_PARTITION))
		    continue;
	    }
	    else 
	    {
		/* 
		** Table level ckp 
		**	Only save if a user specified table or an index/blob
		**	associated with it
		*/
		if ((tblcb->tblcb_table_status &
			(CPP_USER_SPECIFIED | CPP_INDEX_REQUIRED |
			  CPP_BLOB_REQUIRED | CPP_PART_REQUIRED)) == 0)
		    continue;

	    }

	    MEcopy (&tblcb->tblcb_table_name, DB_MAXNAME, 
		&table_list[i].db_tab_name);

	    if (++i == TBLLST_MAX_TAB)
	    {
		status = tbllst_write(jsx, &tblctx);
		if (status != E_DB_OK)
		{
		    uleFormat(&jsx->jsx_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
			(char * )0, 0L, (i4 *)NULL, &local_err, 0);
		    SETDBERR(&jsx->jsx_dberr, 0, E_DM1168_CPP_CHKPT_LST_WRITE);
	    	    break;
		}
		i = 0;
	    }
	}

	if (status != E_DB_OK)
	    break;

    	/* Mark the end of the list */
    	if (i > 0)
	{
	    if (i < (TBLLST_MAX_TAB ))
    	    	table_list[i].db_tab_name[0] = '\0';

	    status = tbllst_write(jsx, &tblctx);
	    if (status != E_DB_OK)
	    {
		uleFormat(&jsx->jsx_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
		    (char * )0, 0L, (i4 *)NULL, &local_err, 0);
		SETDBERR(&jsx->jsx_dberr, 0, E_DM1168_CPP_CHKPT_LST_WRITE);
	    	break;
	    }
	}
    } while (FALSE);
    
    if (tblctx.blkno != -1)
    {
        DB_ERROR	save_dberr = jsx->jsx_dberr;

	local_status = tbllst_close(jsx, &tblctx);
	if (local_status != E_DB_OK)
	{
	    uleFormat(&jsx->jsx_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
		(char * )0, 0L, (i4 *)NULL, &local_err, 0);
	    if (status != E_DB_OK)
	    {
		uleFormat(&save_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG,
		    NULL, (char * )0, 0L, (i4 *)NULL, &local_err, 1,
		    sizeof(tblctx.filename), tblctx.filename.name);
	    }
	    status = E_DB_ERROR;
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM1169_CPP_CHKPT_LST_CLOSE);
	}
	else
	    jsx->jsx_dberr = save_dberr;
    }

    if (status != E_DB_OK)
    {
    	uleFormat(&jsx->jsx_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, (char * )0, 
	    0L, (i4 *)NULL, &local_err, 1, 
	    sizeof(tblctx.filename), tblctx.filename.name);
	SETDBERR(&jsx->jsx_dberr, 0, E_DM116A_CPP_CHKPT_LST_ERROR);
    }
    return(status);
}

/*{
** Name: cpp_wait_free - wait for a free resource
**
**
** Description: Waits until a device is free to be used.
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
**      02-Mar-1999 (kosma01)
**          Checkpointing a database using multiple devices (#m) on
**          platforms built with xCL_086_SETPGRP_0_ARGS can cause
**          a loop in cpp_wait_free(), (sep test lar97).
**          When xCL_086_ is defined cpp_wait_free is not watching
**          the pid of the parent of the "real work" child, but of 
**          the "real work" child's grandparent. When the grandparent
**          and "real work" child die at the same time. Ingres's 
**          CSsigchld() handler may reap the "real work" child and 
**          not the pid that cpp_wait_free() is watching. If this 
**          happens enough (#m times), then cpp_wait_free() is stuck
**          in a loop. I changed cpp_wait_free() to really wait() on
**          any still active pid, if no other devices are available.
**	10-nov-1999 (somsa01)
**	    Use CK_is_alive() instead of PCis_alive(). Also, on NT
**	    PID is defined as an unsigned int. Therefore, typecast
**	    dev_pid to an i4 before testing it. Also, print out the
**	    child's error condition, if necessary.
*/
DB_STATUS
cpp_wait_free(
    DMF_JSX     *jsx,
    DMCKP_CB    *d)
{
    DB_STATUS		status;
    i4             i, j;
    i4     env_value;
    char        *string;
    i4          value = 30000; /* Default sleep time 30 seconds */
    STATUS	waitstatus;
    i4			*err_code = &jsx->jsx_dberr.err_code;

    CLRDBERR(&jsx->jsx_dberr);

    NMgtAt("II_MAX_CKPDB_LOOPS", &string);
    if (string                &&
	*string               &&
	(!CVal(string, &env_value)))
    {
	/* user override of default */
	value = env_value ;
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
			uleFormat( NULL, retval, &err_desc, ULE_LOG, NULL, 
				    NULL, 0, NULL, &errcode, 0 );
			MEfree(command);

			SETDBERR(&jsx->jsx_dberr, 0, E_DM1160_CPP_SAVE);
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
** Name: cpp_wait_all - wait for all free resources
**
**
** Description:  Waits until all devices have finished.
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
**      14-nov-1997 (canor01)
**          If the child process goes away between the call to PCis_alive
**          and the call to PCwait, PCwait may return a misleading error.
**          Change to get return status from CK error log file if PCwait
**          returns any error.
**	10-nov-1999 (somsa01)
**	    Use CK_is_alive() instead of PCis_alive(). Also, on NT
**	    PID is defined as an unsigned int. Therefore, typecast
**	    dev_pid to an i4 before testing it. Also, print out the
**	    child's error condition, if necessary.
*/
DB_STATUS
cpp_wait_all(
    DMF_JSX     *jsx)
{
    DB_STATUS		status;
    DB_STATUS		ret_val = E_DB_OK;
    STATUS		waitstatus = OK;
    i4             i;
    i4			*err_code = &jsx->jsx_dberr.err_code;

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
                    if ( status == E_DB_OK )
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
			uleFormat( NULL, retval, &err_desc, ULE_LOG, NULL, 
				    NULL, 0, NULL, &errcode, 0 );

			SETDBERR(&jsx->jsx_dberr, 0, E_DM1160_CPP_SAVE);
        	        ret_val = E_DB_ERROR;
                    }

		    if (command)
			MEfree(command);
                }
            }
        }
    return( ret_val );
}

#ifdef xDEBUG
/*{
** Name: trace_lgshow -  trace the sequence,offset from LGshow

**
**
** Description:  trace the sequence,offset from LGshow.
**
** Inputs:
**      flag                            LGshow context identifier.
**      identifier                      local calling routine context counter.
**      jsx                             Pointer to checkpoint context.
**      dcb                             Pointer to DCB for the database.
**
** Outputs:
**      Display only
**
** Side Effects:
**          none
**
** History:
**      Created 12-July-2001 ( bolke01)
**              Check the internal sequence of the database during Archiver activity
**              If not zero activity is present and some error may have occured or the
**              Archiver has not yet drained the log file
*/
VOID
trace_lgshow (
i4          flag,
i4          cnt,
DMF_JSX     *jsx,
DMP_DCB     *dcb)
{
LG_DATABASE database ;
int             length = 0 ;
STATUS          cl_status;
CL_ERR_DESC      sys_err;
i4         *err_code , err_value;
char            line_buffer[132];

/*
** Show the database to get its current Dump/Journal windows.
*/ 
   if (jsx->jsx_status & JSX_TRACE)
   {
        TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
            "%@ CPP: Preparing trace_lgshow %~t\n",
            sizeof(dcb->dcb_name), &dcb->dcb_name);

      database.db_id = (LG_DBID) dcb->dcb_log_id;

      cl_status = LGshow(LG_S_DATABASE, (PTR)&database, sizeof(database),
      &length, &sys_err);

      if (cl_status != E_DB_OK)
      {
        err_code = &err_value ;
        *err_code = E_DM1113_CPP_NOARCHIVER;
        uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
           (char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
        uleFormat(NULL, E_DM9017_BAD_LOG_SHOW, &sys_err, ULE_LOG, NULL,
            (char *)NULL, (i4)0, (i4 *)NULL, err_code, 1,
            0, LG_S_DATABASE);
      }
	TRformat( dmf_put_line, 0, line_buffer, sizeof(line_buffer),
		       "cl_status=%d:dmfcpp: dbId: %d:sequence=%d,offset=%d.\n",
			cl_status,
          		database.db_id,
          		database.db_f_dmp_la.la_sequence,
          		database.db_f_dmp_la.la_offset);

   }
}
#endif

DB_STATUS
online_cx_phase(i4 phase, DMF_JSX *jsx)
{
    DB_STATUS		status = E_DB_OK;
    i4			err_code;
    i4                  loc_status;
    CSP_CX_MSG		err_msg;
    LK_LOCK_KEY		lockckey;
    STATUS		cl_status;
    CL_ERR_DESC		sys_err;

    CLRDBERR(&jsx->jsx_dberr);

    switch (phase)
    {
	case CKP_CSP_STALL:
	    status = online_setup(jsx, &jsx->jsx_lock_list);
	    /*
	    ** Signal an archive cycle and wait for the purge to complete
	    ** because after each node has finished the STALL, the local
	    ** node is going to merge the journal files.
	    */
	    if (status == E_DB_OK)
		status = online_wait_for_purge(jsx);
	    break;

	case CKP_CSP_SBACKUP:
	    status = online_write_sbackup(jsx);
	    break;

	case CKP_CSP_RESUME:
	    status = online_resume(jsx, &jsx->jsx_lock_list);
	    break;

	case CKP_CSP_CP:
	    status = online_signal_cp(jsx);
	    break;

	case CKP_CSP_EBACKUP:
	    status = online_write_ebackup(jsx);
	    if (status == E_DB_OK)
		status = online_wait_for_purge(jsx);
	    break;

	case CKP_CSP_DBACKUP:
	    /* Inform LG system we are done with backup */
	    status = online_dbackup(jsx);
	    break;

	default:
	    TRdisplay(ERx("%@ CPP %d: Invalid phase %d\n"), 
			__LINE__, phase);
	    return (E_DB_ERROR);
	    break;
    }

    if (jsx->jsx_ckp_crash == phase)
    {
	TRdisplay(ERx("%@ CPP %d %d::%x: Phase %d testing error handling\n"), 
			__LINE__, jsx->jsx_ckp_node, jsx->jsx_db_id,
			phase), 
	SETDBERR(&jsx->jsx_dberr, 0, E_DM1102_CPP_FAILED);
	status = E_DB_ERROR;
    }

    if (status != E_DB_OK)
    {
	uleFormat(&jsx->jsx_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG,
		NULL, (char * )0, (i4)0, (i4 *)NULL, &err_code, 0);
	/* Note caller will send CSP_MSG_7_CKP (CKP_CSP_STATUS) message */
	/* And release locks */
    }

    return (status);

}

static STATUS
online_cxmsg_send(
i4		phase,
DMF_JSX		*jsx,
DMP_DCB         *dcb)
{
    DB_STATUS		status = E_DB_OK;
    DB_STATUS		ckpdb_status = E_DB_OK;
    STATUS		cl_status;
    LK_LOCK_KEY		lockckey;
    CL_ERR_DESC		sys_err;
    LK_VALUE		lock_value;
    CSP_CX_MSG		ckp_msg;
    CSP_CX_MSG          err_msg;
    char                buf[sizeof(DB_DB_NAME) + sizeof(DB_OWN_NAME) + 
                            sizeof(DMP_LOC_ENTRY) + 32]; 
    char		keydispbuf[128];
    i4			*err_code = &jsx->jsx_dberr.err_code;

    CLRDBERR(&jsx->jsx_dberr);

    do
    {
        /* Send CSP_MSG_7_CKP message */
	jsx->jsx_db_id = dcb->dcb_id;
        ckp_msg.csp_msg_action = CSP_MSG_7_CKP;
        ckp_msg.csp_msg_node = jsx->jsx_node;
        ckp_msg.ckp.dbid = jsx->jsx_db_id;
        ckp_msg.ckp.ckp_node = jsx->jsx_ckp_node;
        ckp_msg.ckp.phase = phase;

	/* ckpdb db "-x_csp_crash=N" */
	if ((jsx->jsx_status & JSX_TRACE) && phase == jsx->jsx_ckp_crash)
	    ckp_msg.ckp.flags = CKP_CRASH;
	else
	    ckp_msg.ckp.flags = 0;

        if (phase == CKP_CSP_INIT)
        {
	    char *dst = buf;

            MEcopy(dcb->dcb_name.db_db_name, sizeof(DB_DB_NAME), dst);
	    dst += sizeof(DB_DB_NAME);
            MEcopy(dcb->dcb_db_owner.db_own_name, sizeof(DB_OWN_NAME), dst);
	    dst += sizeof(DB_OWN_NAME);
            MEcopy((PTR)&dcb->dcb_location, sizeof(DMP_LOC_ENTRY), dst);
	    dst += sizeof(DMP_LOC_ENTRY);
	    MEcopy((PTR)&dcb->dcb_status, sizeof(dcb->dcb_status), dst);
	    dst += sizeof(dcb->dcb_status);
            CVla((i4)jsx->jsx_pid, dst);
            cl_status = CXmsg_stow( &ckp_msg.ckp.ckp_var.init_chit, 
                        buf, sizeof(buf));
        }
        else if (phase == CKP_CSP_SBACKUP)
        {
            /* Indicate if this is table level checkpoint */
            if ((jsx->jsx_status1 & JSX1_CKPT_DB) == 0)
                ckp_msg.ckp.ckp_var.sbackup_flag = 1;
            else
                ckp_msg.ckp.ckp_var.sbackup_flag = 0;
        }
        else
        {
            ckp_msg.ckp.ckp_var.misc = 0;
        }

	jsx->jsx_ckp_phase = phase;
        cl_status = CXmsg_send( CSP_CHANNEL, NULL, NULL, (PTR)&ckp_msg);
	TRdisplay("%@ CPP %d %d::%x: CXmsg_send phase %d status %d\n", 
			__LINE__, jsx->jsx_ckp_node,jsx->jsx_db_id, 
			phase, cl_status);
        if (cl_status != OK)
            break;

        /* CKP_CSP_INIT processing done */
        if (phase == CKP_CSP_INIT)
            break;

        /* CKP_CSP_STATUS processing done */
        if (phase == CKP_CSP_STATUS)
            break;

	/*
	** Even if this node got an error
	** WAIT for other nodes to finish this phase before ABORTING
	** Concurrent ckpdb and ABORT processing in the csp can lead to errors
	*/
        ckpdb_status = online_cx_phase(phase, jsx);

        /* Wait for all nodes to finish (Request LK_CKP_CLUSTER phase lock) */
        lockckey.lk_type = LK_CKP_CLUSTER;
        lockckey.lk_key1 = jsx->jsx_ckp_node;
        lockckey.lk_key2 = jsx->jsx_db_id;
        lockckey.lk_key3 = phase;
        lockckey.lk_key4 = 0;
        lockckey.lk_key5 = 0;
        lockckey.lk_key6 = 0;
        cl_status = LKrequest(LK_PHYSICAL, jsx->jsx_lock_list,
                &lockckey, LK_X, (LK_VALUE * )0, (LK_LKID *)0, 0L, &sys_err);

        if (cl_status != OK)
        {
	    TRdisplay("%@ CPP %d %d::%x: CXmsg_send phase %d LK status %x\n"
			"\t Key %s\n",
			    __LINE__, jsx->jsx_ckp_node, jsx->jsx_db_id, 
			    phase, cl_status,
			    LKkey_to_string(&lockckey, keydispbuf));
            uleFormat(NULL, E_DM901C_BAD_LOCK_REQUEST, &sys_err, ULE_LOG, NULL,
                (char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
            status = E_DB_ERROR;
            break;
        }

	/* Check ckpdb status AFTER remote nodes have finished */
	if (ckpdb_status != E_DB_OK)
	{
	    status = E_DB_ERROR;
	    break;
	}

        /* Check value in LK_CKP_CLUSTER CKP_CSP_STATUS lock */
        lock_value.lk_low = 0;
        lock_value.lk_high = 0;

        lockckey.lk_key3 = CKP_CSP_STATUS;
	/* Exclude ckpdb node from key for CKP_CSP_STATUS phase locks */
        lockckey.lk_key1 = 0;

        cl_status = LKrequest(LK_PHYSICAL, jsx->jsx_lock_list,
            &lockckey, LK_S, &lock_value, (LK_LKID *)0, 0L, &sys_err);

        if (cl_status != OK)
        {
	    TRdisplay("%@ CPP %d %d::%x: CXmsg_send phase %d LK status %x\n"
			"\t Key %s\n",
		    __LINE__, jsx->jsx_ckp_node, jsx->jsx_db_id, 
		    phase,
		    cl_status,
		    LKkey_to_string(&lockckey, keydispbuf));
            uleFormat(NULL, E_DM901C_BAD_LOCK_REQUEST, &sys_err, ULE_LOG, NULL,
                (char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
            status = E_DB_ERROR;
            break;
        }

	TRdisplay("%@ CPP %d %d::%x: After phase %d LK_CKP_CLUSTER CKP_CSP_STATUS (%x,%x)\n",
		__LINE__, jsx->jsx_ckp_node, jsx->jsx_db_id, 
		phase, 
		lock_value.lk_high, lock_value.lk_low);

        if (lock_value.lk_low != jsx->jsx_pid || lock_value.lk_high != 0)
        {
	    TRdisplay("%@ CPP %d %d::%x: LK_CKP_CLUSTER value (%x,%x) should be (%x,%x)\n"
			"\t Key %s\n",
		__LINE__, jsx->jsx_ckp_node, jsx->jsx_db_id, 
		lock_value.lk_high,lock_value.lk_low, 
		0, jsx->jsx_pid,
		LKkey_to_string(&lockckey, keydispbuf));
            status = E_DB_ERROR;
            break;
        }
    
	cl_status = LKrelease(0, jsx->jsx_lock_list,
	    (LK_LKID *)0, (LK_LOCK_KEY *)&lockckey, (LK_VALUE *)0, &sys_err);

	if (cl_status != OK)
	{
	    TRdisplay("%@ CPP %d %d::%x: CXmsg_send phase %d LK status %x\n"
			"\t Key %s\n",
		__LINE__, jsx->jsx_ckp_node, jsx->jsx_db_id, 
		phase, cl_status,
		LKkey_to_string(&lockckey, keydispbuf));
	    uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	    uleFormat(NULL, E_DM901B_BAD_LOCK_RELEASE, &sys_err, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, err_code, 1, 0, 
		dmf_svcb->svcb_lock_list);
	    status = E_DB_ERROR;
	    break;
	}

    } while (FALSE);

    /* If any error, notify all nodes */
    if (status != E_DB_OK || cl_status != OK)
    {
        err_msg.csp_msg_action = CSP_MSG_7_CKP;
        err_msg.csp_msg_node = jsx->jsx_node;
        err_msg.ckp.dbid = jsx->jsx_db_id;
        err_msg.ckp.phase = CKP_CSP_ABORT;
        err_msg.ckp.ckp_node = jsx->jsx_ckp_node;
        err_msg.ckp.ckp_var.abort_phase = phase;
	TRdisplay("%@ CPP %d %d::%x: After phase %d sending ABORT msg \n", 
		    __LINE__, jsx->jsx_ckp_node, jsx->jsx_db_id, 
		    phase);
        cl_status = CXmsg_send( CSP_CHANNEL, NULL, NULL, (PTR)&err_msg);
	/* Reset jsx_ckp_phase so CKP_CSP_NONE msg is sent once */
	jsx->jsx_ckp_phase = CKP_CSP_NONE;
	SETDBERR(&jsx->jsx_dberr, 0, E_DM1131_CPP_LK_FAIL);
        return (E_DB_ERROR);
    }

    return (E_DB_OK);

}

/*{
** Name: GetClusterStatusLock -  Set Cluster DB STATUS lock
**
**
** Description:  To prevent concurrent checkpoints on a database
**		 from different nodes in a cluster, request a
**		 CKP_CSP_STATUS-type LK_CKP_CLUSTER lock.
**	
**		 The lock value contains the PID of the running
**		 checkpoint process.
**
**		 Note that the JSX_WAITDB flag as applied to
**		 DB access within a node also applies to 
**		 cluster-wide access; we'll wait for cluster-access
**		 if requested.
**
**		 If no-wait and the lock is held by another node,
**		 return E_DM004C_LOCK_RESOURCE_BUSY error code.
**
** Inputs:
**      jsx                             Pointer to checkpoint context.
**
** Outputs:
**      none
**
** Side Effects:
**          Prohibits concurrent checkpoint of the same DB from
**	    multiple nodes.
**
** History:
**	21-Dec-2007 (jonj)
**	    Created.
*/
static DB_STATUS
GetClusterStatusLock(
DMF_JSX		*jsx)
{
    CL_ERR_DESC	sys_err;
    DB_STATUS	status = E_DB_OK;
    STATUS	lk_status;
    LK_LOCK_KEY	lockckey;
    LK_VALUE	lock_value;
    LK_LKID	lockid;
    i4		lock_flags;
    i4		lock_mode;
    i4			*err_code = &jsx->jsx_dberr.err_code;

    CLRDBERR(&jsx->jsx_dberr);

    lockckey.lk_type = LK_CKP_CLUSTER;
    lockckey.lk_key1 = 0;
    lockckey.lk_key2 = jsx->jsx_db_id;
    lockckey.lk_key3 = CKP_CSP_STATUS;
    lockckey.lk_key4 = 0;
    lockckey.lk_key5 = 0;
    lockckey.lk_key6 = 0;

    lockid.lk_unique = 0;
    lockid.lk_common = 0;

    lock_flags = LK_PHYSICAL | LK_INTERRUPTABLE;

    lock_mode = LK_X;

    if ( !(jsx->jsx_status & JSX_WAITDB) )
	lock_flags |= LK_NOWAIT;

    TRdisplay(ERx("%@ CPP %d %d::%x: %w(%d) LKrequest LK_X LK_CKP_CLUSTER (%w)\n"),
	    __LINE__, jsx->jsx_ckp_node, jsx->jsx_db_id,
	    CKP_PHASE_NAME, CKP_CSP_INIT, CKP_CSP_INIT,
	    CKP_PHASE_NAME, CKP_CSP_STATUS);

    lk_status = LKrequest(lock_flags,
		    dmf_svcb->svcb_lock_list,
		    &lockckey, lock_mode, (LK_VALUE * )0, &lockid,
		    0L, &sys_err);

    if ( lk_status == OK )
    {
	/*
	** Convert LK_CKP_CLUSTER CKP_CSP_STATUS lock
	** LK_X->LK_S to store LK_VALUE
	** (high value has 0, low value has ckpdb pid)
	*/
	lock_value.lk_high = 0;
	lock_value.lk_low = jsx->jsx_pid;

	lock_mode = LK_S;

	TRdisplay(ERx("%@ CPP %d %d::%x: %w(%d) LKconvert LK_S CKP_CSP_CLUSTER (%x,%x)\n"), 
		__LINE__, jsx->jsx_ckp_node, jsx->jsx_db_id,
		CKP_PHASE_NAME, CKP_CSP_INIT,
		lock_value.lk_high, lock_value.lk_low);

	lk_status = LKrequest(LK_PHYSICAL | LK_CONVERT | LK_NODEADLOCK, 
	    dmf_svcb->svcb_lock_list, (LK_LOCK_KEY *)0, lock_mode, 
	    &lock_value, &lockid, (i4)0, &sys_err);
    }
    else if ( lk_status == LK_BUSY )
    {
	lk_status = OK;
	SETDBERR(&jsx->jsx_dberr, 0, E_DM004C_LOCK_RESOURCE_BUSY);
	status = E_DB_ERROR;
    }

    if ( lk_status != OK )
    {
	uleFormat(NULL, lk_status, &sys_err, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	uleFormat(NULL, E_DM901C_BAD_LOCK_REQUEST, &sys_err, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, err_code, 2,
		0, lock_mode, 0, dmf_svcb->svcb_lock_list);
	status = E_DB_ERROR;
    }

    return(status);
}


/*{
** Name: ReleaseClusterStatusLock -  Release Cluster DB STATUS lock
**
**
** Description:  Release the cluster STATUS lock when the ckpdb
**		 completes.
**
** Inputs:
**      jsx                             Pointer to checkpoint context.
**
** Outputs:
**      none
**
** Side Effects:
**      none
**
** History:
**	21-Dec-2007 (jonj)
**	    Created.
*/
static DB_STATUS
ReleaseClusterStatusLock(
DMF_JSX		*jsx)
{
    CL_ERR_DESC	sys_err;
    DB_STATUS	status = E_DB_OK;
    STATUS	lk_status;
    LK_LOCK_KEY	lockckey;
    i4			*err_code = &jsx->jsx_dberr.err_code;

    CLRDBERR(&jsx->jsx_dberr);

    lockckey.lk_type = LK_CKP_CLUSTER;
    lockckey.lk_key1 = 0;
    lockckey.lk_key2 = jsx->jsx_db_id;
    lockckey.lk_key3 = CKP_CSP_STATUS;
    lockckey.lk_key4 = 0;
    lockckey.lk_key5 = 0;
    lockckey.lk_key6 = 0;

    TRdisplay(ERx("%@ CPP %d %d::%x: %w(%d) LKrelease LK_CKP_CLUSTER (%w)\n"),
	    __LINE__, jsx->jsx_ckp_node, jsx->jsx_db_id,
	    CKP_PHASE_NAME, CKP_CSP_INIT, CKP_CSP_INIT,
	    CKP_PHASE_NAME, CKP_CSP_STATUS);

    lk_status = LKrelease(0, dmf_svcb->svcb_lock_list,
		(LK_LKID*)0, &lockckey, (LK_VALUE*)0,
		&sys_err);

    if (lk_status != OK)
    {
	uleFormat(NULL, lk_status, &sys_err, ULE_LOG, NULL,
	    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	uleFormat(NULL, E_DM901B_BAD_LOCK_RELEASE, &sys_err, ULE_LOG,
	    NULL, (char *)NULL, (i4)0, (i4 *)NULL, err_code,
	    1, 0, dmf_svcb->svcb_lock_list);
	status = E_DB_ERROR;
    }

    return(status);
}


static DB_STATUS
lk_ckp_db(
DMF_JSX		*jsx,
DMP_DCB         *dcb)
{
    DB_STATUS		status = E_DB_OK;
    STATUS		cl_status;
    LK_LOCK_KEY		lockckey;
    CL_ERR_DESC		sys_err;
    LK_LOCK_KEY         lockkey;
    i4			*err_code = &jsx->jsx_dberr.err_code;

    CLRDBERR(&jsx->jsx_dberr);
     
    /*
    ** Get DBU lock to prevent database reorganization during the backup.
    */
    lockkey.lk_type = LK_CKP_DB; 
    lockkey.lk_key1 = dcb->dcb_id;
    MEcopy((PTR)&dcb->dcb_db_owner, 8, (PTR)&lockkey.lk_key2);
    MEcopy((PTR)&dcb->dcb_name, 12, (PTR)&lockkey.lk_key4);

    cl_status = LKrequest(LK_PHYSICAL, jsx->jsx_lock_list, &lockkey,
	    LK_X, (LK_VALUE * )0, &jsx->jsx_lockid, 0L, &sys_err); 
    if (cl_status != OK)
    {
	uleFormat(NULL, E_DM901C_BAD_LOCK_REQUEST, &sys_err, ULE_LOG, NULL, 
	    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	SETDBERR(&jsx->jsx_dberr, 0, E_DM1131_CPP_LK_FAIL);
	return (E_DB_ERROR);
    }

    if (jsx->jsx_status & JSX_TRACE)
	TRdisplay("%@ CPP: Successfully Lock Out All DBU operations\n");
    return (E_DB_OK);
}

static VOID
ckp_txn_lock_key(
DMF_JSX		*jsx, 
LK_LOCK_KEY	*lockxkey)
{

    /*
    ** Prepare the lock key as:
    **
    **	type:   LK_CKP_TXN
    **    key1-5: 1st 20 bytes of database name
    **    key6:   dbid
    **
    **	(see also conforming code in lgalter, dm0l, lkshow, dmfcsp)
    */
    lockxkey->lk_type = LK_CKP_TXN;
    MEcopy((char *)jsx->jsx_db_name.db_db_name, 20, (char *)&lockxkey->lk_key1);
    lockxkey->lk_key6 = jsx->jsx_db_id;

    if (jsx->jsx_status & JSX_TRACE)
	TRdisplay("%@ CPP: LK_CKP_TXN %~t %d\n",
		20, (char*)(&lockxkey->lk_key1), lockxkey->lk_key6);
}
