/*
** Copyright (c) 1987, 2004 Ingres Corporation
**
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <cv.h>
#include    <cx.h>
#include    <er.h>
#include    <jf.h>
#include    <di.h>
#include    <tr.h>
#include    <me.h>
#include    <nm.h>
#include    <pc.h>
#include    <st.h>
#include    <tm.h>
#include    <sl.h>
#include    <lo.h>
#include    <pm.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <lg.h>
#include    <lk.h>
#include    <ulf.h>
#include    <adf.h>
#include    <dmf.h>
#include    <dm.h>
#include    <dm0m.h>
#include    <dmpp.h>
#include    <dm0llctx.h>
#include    <dmp.h>
#include    <dm0c.h>
#include    <dm1b.h>
#include    <dm0l.h>
#include    <dm0j.h>
#include    <dm0d.h>
#include    <dmfacp.h>
#include    <dmftrace.h>
#include    <dmd.h>

/**
**
**  Name: DMFARCHIVE.C - Common archiver routines.
** 
**  Description: 
**	Function prototypes are defined in DMFACP.H.  
** 
**      This module contains the common routines used to archive the
**	journaled log records from the log file to the a journal file for
**	each database.
**
**     External routines
**
**	    dma_alloc_dbcb	-   Allocate DBCB control block.
**	    dma_archive	 	-   Copy from log to journal and dump.
**	    dma_complete 	-   Clean up prior to shutdown
**	    dma_eoc      	-   Clean up after archive cycle.
**	    dma_jobtodo		-
**	    dma_prepare  	-   Prepare for archiving
**	    dma_soc      	-   Perform start of cycle prep 
**	    dma_soc      	-   Perform archive copy to journal
**	    dma_offline_context -   Figure out which DB's to process from log.
**
**     Internal routines
**
**	    acp_test_record	-
**	    check_start_backup	-
**	    check_end_backup	-
**	    copy_record_to_dump	- 
**	    init_dump    	-  Init dump file 
**	    init_journal 	-  Init journal file 
**	    log_2_dump		-
**	    new_jnl_entry	-
**	    skip_cp		-
**	    next_journal	-
**
**  History:
**      28-jul-1987 (Derek)
**          Created.
**	15-Jun-1988 (EdHsu)
**	    Add changes for fast commit.  Use BCP records instead of CP records.
**	21-jan-1989 (Edhsu)
**	    Add code to support online backup feature of Terminator.
**	31-Aug-1989 (ac)
**	    Fixed Cluster ONLINE BACKUP problems.
**	11-sep-1989 (rogerk)
**	    Added lx_id parameter to dm0l_opendb call.
**	 2-oct-1989 (rogerk)
**	    Added DM0LFSWAP log record.
**	13-nov-1989 (rogerk)
**	    Fixed online backup bugs.  Added ebackup log address to the backup
**	    context structure.  This is used to tell whether log records fall
**	    outside the backup window rather than using dbcb_done since
**	    dbcb_done may be set even when we are not finished with backup
**	    processing.  Also added log address parameters to check_start_backup
**	    and check_end_backup routines so they can distiguish between the
**	    current start/end backup records and ones left in the log file
**	    from a previous backup on the same database.
**	01-dec-1989 (mikem)
**	    Fixed 2 problems.  Made sure that empty dump files were truncated
**	    (as opposed to being written out uninitialized and sometimes very
**	    large, ie. 7 meg).  Also make sure that duump file history of 
**	    obsoleted checkpoints is written correctly, so that a subsequent
**	    "ckpdb -d" can correctly clean them out.
**	 1-dec-1989 (rogerk)
**	    In copy_dump, skip records for databases that we know have no
**	    records to move to the DUMP files.  These db's may not have been
**	    initialized by init_dump and we cannot rely on information in the
**	    control blocks.
**	12-dec-1989 (rogerk)
**	    Changed setting of dbcb_done flag.  This flag indicates that the
**	    archiver work required for a checkpoint has completed and the
**	    checkpoint process can be notified that the checkpoint was
**	    successful.  We cannot set this flag until ALL the work is done.
**	    We should delay setting this flag until all processing in COPY_DUMP
**	    is complete and the dump file has been closed in dma_b_p3 (or we
**	    can go ahead and set dbcb_done if we see there is no work to do
**	    for the checkpoint).  Use the dbcb_copydone flag to indicate that
**	    the copy is complete.
**	12-Dec-1989 (ac)
**	    In acp_s_j_p2(), don't create a new journal file if the current
**	    journal file is created for the current checkpoint.
**	 8-jan-1990 (rogerk)
**	    Made major changes for Online Backup bug fixes - changed some of
**	    the algorithms for online backup:
**		- Consolidated some of the backup code into common routines -
**		  created new_jnl_entry and dma_p1_dump_rec routines.
**		- Added backup error status to dbcb - if ob_clean finds db
**		  in error state it should signal a checkpoint-error.
**		- Changed checkpoint protocol to make checkpoint process wait
**		  until all backup work in the acp is complete to report that
**		  the checkpoint has completed.  This ensures that we will not
**		  have to deal with multiple backups within the same archive
**		  window.
**		- Added dbcb_backup_complete field.  This is set when all
**		  work for online backup has completed - all dump records
**		  have been written, and the dump files have been flushed and
**		  closed.  The archiver should not signal FBACKUP until
**		  backup_complete is true.
**		- Added Online_purge operation as an option to the PURGE 
**		  operation.  This tells the archiver to purge all the journal
**		  records for the given database, even though the database is
**		  still open.  It is assumed/enforced that when the archiver 
**		  gets to the end of the current archive window, that there will
**		  no unresolved transactions left.  This Online-Purge operation
**		  will be signaled in Online Backup to purge all old journal
**		  records to the old journal files before completing the backup.
**		- Took out acp_s_d_p2 routine - we should never have to switch
**		  dump files in the middle of backup processing.
**		- Moved creation of the new entry in the journal history from
**		  the checkpoint process to the archiver for journaled dbs.
**		  This prevents us from mistakenly putting old journal info
**		  into journal files associated with the new checkpoint.
**		- Added NEW_JNL_NEEDED flag in the dbcb to make sure we add
**		  the new journal history entry - even if we find no journal
**		  work to do in the current archive window.  If there are no
**		  journal records to process then the archiver will skip the
**		  copy phase and acp_s_j_p2 will never be called.
**		- Added dbcb_sbackup field to keep track of the address of
**		  the correct sbackup record associated with the current backup.
**		  Don't process sbackup records unless they belong to an
**		  ongoing online backup call.
**		- Don't process dump work in the CSP or during PURGE operations.
**		- Changed how we keep track of the last-dumped address.  Don't
**		  use CP addresses as we don't always do DUMP work between
**		  CP's.  Instead keep track of actual addresses of records
**		  written to the dump file.
**	    Added more trace information - added log address parms to
**	    dm0l_sbackup/dm0l_ebackup for better trace information.
**	    Fixed archiver bug - only call dm0l_archive if CSP process.
**	15-jan-1990 (rogerk)
**	    Added more trace information for Online Backup.
**	17-jan-1990 (rogerk)
**	    In dma_p2, don't process SBACKUP records for databases that 
**	    require no journal processing in the current archive interval.
**	    Call LGend to clean up transaction context if dm0l_bt call fails.
**	    This is no longer done in dm0l_bt.
**	26-apr-1990 (mikem)
**	    bug #21427
**	    In check_start_backup() LGshow() was being called with a "length" 
**	    uninitialized.  
**	    In some cases at least on the sun4 this value sometimes was a large 
**	    negative number which would result in an AV.  The fix was to set
**	    "length = 0" before the LGshow loop.
**	30-apr-1990 (sandyh)
**	    bug #21370
**	    In read_cp() use only oldest transaction that is in ACTIVE,PROTECT
**	    state; otherwise the lxb_cp_lga value will contain a possibly bad
**	    cp address which will cause the scan to proceed to the EOF causing
**	    the ACP to crash.
**	5-jun-1990 (bryanp)
**	    Explicitly clear the DCB portion of the DBCB when allocating it.
**	9-jul-1990 (bryanp)
**	    New debugging subroutine "dump_mismatched_et" for problem solving.
**	20-jul-1990 (bryanp)
**	    A customer somehow managed to get a log file to contain lower
**	    log addresses (perhaps by re-initializing it with an erroneous
**	    system time). The result was that current log file addresses
**	    compared less than log file addresses stored in the config file for
**	    the database, and journalling was not performed. Added a number of
**	    sanity checks and internal consistency checks to the archiver to
**	    enable it to detect and report such an error. Also found several
**	    other minor, unrelated bugs and fixed them: dbcb_et_cnt was twice
**	    as large as it should be, the tx pointer was being used after it
**	    was deallocated, and the acb_tx_count was going negative. Finally,
**	    added a number of comments in the code about ways to debug problems
**	    of this sort.
**	9-aug-1990 (bryanp)
**	    Correct the test for 'already archived' in dma_p2 copy phase.
**	18-sep-1990 (bryanp)
**	    Fix mismatched et in dma_p1 -- don't check DB jnl window (#33392)
**      25-feb-1991 (rogerk)
**	    Added changes for Archiver Stability project.
**	    Added acb_error_code, acb_err_text, acb_err_database fields to
**	    the archiver control block.  These fields are formatted whenever
**	    an acp-fatal error is encountered.  The error code and message
**	    written from the acp_exit_handler to give final indication of
**	    the archiver shutdown type.
**	    Made changes to much error handling code in DMFACP.C and in
**	    DMFARCHIVE.C to format the above fields for shutdown processing.
**	    Added more detailed error information when memory allocation
**	    errors are encountered.
**	    Added acp_test_record routine and ACP test log records to
**	    simulate archiver tests.
**	17-mar-1991 (bryanp)
**	    B36228. We weren't writing DM0L_LPS records when we should. Instead
**	    of keeping a node-wide current_lps value in the ACB, keep a value
**	    for each database in the DBCB.
**	25-mar-1991 (rogerk)
**	    Moved check for test log records (acp_test_record calls) to be
**	    after the check for if the database has already been processed
**	    in the current consistency point window.  This prevents the
**	    archiver from tripping the same trace log record more than once.
**	    Added check for test log records in dma_p1 to ensure that the
**	    the COMPLETE_WORK test log record is processed.
**	    Added phase argument to acp_test_record to allow it to be
**	    called from dma_p1 and dma_p2.
**	28-mar-1991 (bryanp)
**	    Fixed db log address sanity check in dma_p2 for cluster archiving.
**	8-may-1991 (bryanp)
**	    Copy DM0LBPEXTEND to dump file for online checkpoint (B37428)
**	16-jul-1991 (bryanp)
**	    B38527: add cases for DM0LOLDMODIFY and DM0LOLDINDEX and
**	    DM0LSM0CLOSEPURGE..
**	9-aug-1991 (bryanp)
**	    Fix parameter to E_DM900E and to E_DM9016.
**	19-aug-1991 (rogerk)
**	    Set process type in the svcb.  This was done as part of the
**	    bugfix for B39017.  This allows low-level DMF code to determine
**	    the context in which it is running (Server, RCP, ACP, utility).
**	16-sep-1991 (walt)
**	    In update_backup_context, nolonger include stalled online backups
**	    in the list of databases needing backup processing.  The backup
**	    hasn't started yet, and doing processing on it can lead to errors
**	    like E_DM9829_ARCH_BAD_LOG_TYPE where normal log records are
**	    flagged as "illegal" during the (not yet started) backup.
**	17-oct-1991 (rogerk)
**	    Great Merge of DMF : Move 6.4 changes into 6.5.  In this case,
**	    the 6.4 version of this file was moved directly to 6.5.  The
**	    changes made for the file-extend project were then merged into
**	    this version of the file.
**
**	    14-jun-1991 (Derek)
**		Update on-line backup support for new set of log records that
**		need to be processed given the allocation project changes.
**	4-mar-1992 (bryanp)
**	    B41286: Handle "phantoms" properly.
**	14-may-1992 (bryanp)
**	    B38112: Handle DB_INCONST ET's specially.
**	8-july-1992 (ananth)
**	    Prototyping DMF.
**	29-sep-1992 (bryanp)
**	    Prototyping LG and LK.
**	29-sep-1992 (nandak)
**	    Use one common transaction ID type.
**      2-oct-1992 (ed)
**          Use DB_MAXNAME to replace hard coded numbers
**          - also created defines to replace hard coded character strings
**          dependent on DB_MAXNAME
**	17-nov-1992 (bryanp)
**	    Log more information for E_DM9806. Also, log LG/LK status return
**	    code when an LG/LK call fails.
**	    Use LGidnum_from_id to extract the numeric portion of an LG handle.
**	03-dec-1992 (jnash)
**	    Reduced logging project.  Insert log record lsn into the log
**	    prior to writing it to journal.
**	14-dec-1992 (rogerk)
**	    Include dm1b.h before dm0l.h for prototype definitions.
**	21-dec-1992 (jnash)
**	    Rewritten for the Reduced Logging Project.  
**	     - Page oriented recovery mechanisms require that database pages
**	       be "moved forward" to their state at the time of the crash,
**	       including pages affected by transaction rollback.  This
**	       is also the case for ROLLFORWARD, hence we now need to
**	       archive aborted transactions.  This allows us to make 
**	       archiving a single pass operation, significantly simplifying 
**	       program design.   
**	     - The archiver no longer tracks transactions -- just databases.
**	     - All archive events now cause the same internal action.
**	     - Dump file writes occur at the same time as journal writes.
**	     - Archiving is now done from the CP prior to the last archived
**	       location until the end of file.  The archiver is no 
**	       longer concerned with the location of the last protected 
**	       transaction.
**	18-jan-1993 (rogerk)
**	    Added II_DMF_TRACE handling.
**	27-jan-1993 (jnash)
**	    Online backup fixes.  Databases undergoing online backup must
**	    be distinguised from journaled databases.  The new archiver
**	    was not making this distinction.  Add DBCB_JNL.
**	10-feb-1993 (jnash)
**	    Add new param to dmd_log().
**	15-mar-1993 (jnash)
**	    Reduced Logging - Phase IV:
**	    Use external database id in places where internal id previously 
**		used.  Remove checks for "bad database id" which now make
**		no sense.  
**	    Eliminate use of internal database id as an index into DBCB 
**		infomation.  Instead, build hash queue based on external 
**		database id.  Remove all references to LGidnum_from_id.
**		Add lookup_dbcb() and dalloc_dbcbs().
**	    Removed special tran_id, db_id fields from ET and BACKUP records.
**	    Removed unused acp_s_d_p2 routine.
**	    Renamed LG_A_ARCHIVE signal to LG_A_ARCHIVE_DONE.
**	    Fixed journal-disabled support.
**	    Added direction parameter to dm0j_open routine.
**	22-mar-1993 (rogerk)
**	    Added check for hitting EOF in validate_cp.  During normal
**	    archive processing it is OK to hit EOF - this is treated as
**	    end of the archive cycle.  Changed validate_cp to not error
**	    if EOF is hit, but to return a warning so that dma_copy could
**	    the proceed.  It is expected that dma_copy will hit the EOF
**	    as well and terminate the archive cycle.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**		Added dma_offline_context. This is called by the CSP when it
**		    performs archiving on behalf of another node.
**		Log forcing is now done by LSN, not by LG_LA.
**		Don't stop archiving after 4 CP's if this is the CSP.
**		Don't need to call LG_A_ARCHIVE_DONE if this is the CSP.
**		Pass correct transaction handle to dm0l_read.
**		Correct LK parameter passing problems.
**		Use the appropriate LGA log address comparison macros.
**		Respond to lint suggestions.
**	24-may-1993 (bryanp)
**	    Include <pc.h> before <lk.h> to pick up PID definition.
**	24-may-1993 (andys)
**	    Pass appropriate logfile blocksize to dmd_log and dmd_format_log
**	    rather than using hardwired 4096.
**	24-may-1993 (jnash)
** 	   Preallocate space only in the first journal file created after
**	   a ckpdb.
**	21-jun-1993 (rogerk)
**	    Add BCP record type to the list of records that the archiver
**	    looks at even though they are not journaled records.  BCP's
**	    must be handled so we can track the CP address to which to
**	    inform the logging system we have archived records.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (bryanp)
**	    We no longer log a closedb log record, so we no longer need the
**		'lx_id' argument to dm0l_opendb() and dm0l_closedb().
**	26-jul-1993 (rogerk)
**	  - Changed the way journal and dump windows are handled.  They are
**	    now tracked by real Log Addresses rather than the Consistency
**	    points that bound them.  Consolidated the system journal and
**	    dump windows into a single archive window.
**	  - Changed tracking of CP addresses during archiving.  We now note 
**	    CP addresses each time we see an ECP record (rather than the BCP),
**	    keeping track of the address of the last valid CP we encounter.
**	    At the end of the archive cycle we tell the Logging System the
**	    address of the last CP we saw.  If no CP's were passed, we now
**	    can return a zero log address and LG knows what to do.
**	  - Merged dma_archive and dma_copy into one routine.
**	  - Changed the way the archiver periodically stops to mark its
**	    progress so far.  Rather than stopping each 4 CP's, the archiver
**	    now determines its stopping point through use of the acb_max_cycle
**	    field.  This field can be altered through the archiver_refresh
**	    PM variable.  The stopping point is now pre-determined using this
**	    information in the context building phase.
**	  - Added acb_last_la which tracks the last log record processed
**	    by the archiver in this cycle.
**	  - Added optimization to bypass any work on databases whose current
**	    journal and dump windows are zero.
**	  - Took out ifdef'd code that writes a Consistency Point at the 
**	    end of offline archiving.
**	  - Added "archiver_refresh" PM variable which directs the archiver to
**	    complete archive cycles after some maximum percentage of the log
**	    file is processed.
**	  - Changed trace messages written by the archiver.
**	  - removed validate_cp routine.
**	  - Include respective CL headers for all cl calls.
**	20-sep-1993 (bryanp)
**	    Changes in support of 6.5 cluster archiving:
**		Fill in acb_s_lxid, acb_start_addr, acb_stop_addr in csp call to
**		    dma_prepare.
**		In offline context, pass reasonable journal window addresses
**		    when allocating dbcb.
**		In offline context, check for existing dbcb before allocating
**		    new one.
**		After archiving a DB on a node, mark the DB fully closed.
**	18-oct-1993 (jnash)
**	    Add log address information to E_DM9845_ARCH_WINDOW, add new
**	    E_DM9846_ARCH_WINDOW error.
**	18-oct-1993 (rmuth)
**	    CL prototype, add <pm.h>
**	28-mar-1994 (bryanp) B59975
**	    Remove node_id from the dma_prepare interface. Figure node_id out
**		by using the lctx_node_id.
**	25-apr-1994 (bryanp) B60709, B60230
**	    Show the log header when preparing to perform cluster archiving.
**	    During cluster node failure recovery the log file may have been
**		changed (due to CLR writing, moving forward of the BOF, etc.),
**		so the lctx_header in the lctx may be out of date.
**	05-feb-95 (carly01) b66422
**		prevent SEGV when writing to unopened journal.
**      24-jan-1995 (stial01) 
**          BUG 66473: flag in header of SBACKUP record now indicates if 
**              checkpoint is TABLE checkpoint.
**      12-may-1995 (thaju02)
**          Bug #68644 - rollforwarddb was applying journal records
**          for non-journaled tables.  Corrected bug such that
**          the log record is copied to the journal file only
**          when the database is journaled and the table in
**          which the log record pertains to is a journaled table.
**	    Modified procedure dma_archive.
**	06-jun-1995 (thaju02)
**	    Back out of change made for bug#68644, as per nick@camel
**	    ("The Ingres database was traditionally afflicted by the
**	    fact that in a journaled database, the system catalogs
**	    are always journaled but user tables may not be
**	    yet operations on non-journaled objects which altered
**	    catalogs usually resulted in these system catalog
**	    changes not being journaled either.
**	    This caused many inconsistencies where only parts or
**	    nothing of a particular transaction was journaled when
**	    the changes being made to the catalogs should
**	    have been.
**	    ...a deliberate warning / information message being
**	    written to warn that a non-journaled table is being dropped 
**	    during rollforward...
**	    The reason for this is that a traditional hack to
**	    retrieve a non-journaled table which had been dropped
**	    in error was to perform a rollforward - doing this
**	    with the default +j to apply journals would now drop
**	    it again and it was decided after much discussion in
**	    '93 ( Jim Nash, RogerK  and others ) that this
**	    message would be useful.  Failure to journal the system
**	    catalogs causes corruption...")
**      06-mar-1996 (stial01)
**          Variable Page Size project: len field in log records is now i4:
**          dma_archive() change call to dm0j_write()
**          log_2_dump() fix call to dm0d_write()
**      08-Dec-1997 (hanch04)
**          Use new la_block field when block number is needed instead
**          of calculating it from the offset
**          Keep track of block number and offset into the current block.
**          for support of logs > 2 gig
**	22-Feb-1999 (bonro01)
**	    dm0l_deallocate() was modified to remove the explicit assignment
**	    of dmf_svcb->svcb_lctx_ptr to NULL, so a check must be added
**	    in dma_complete() to determine if the LCTX we are deallocating
**	    is the same one referenced in dmf_svcb->svcb_lctx_ptr.
**	21-apr-1999 (hanch04)
**	    Replace STindex with STchr
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
**      28-apr-1999 (hanch04)
**          Replaced STlcopy with STncpy
**	30-Nov-1999 (jenjo02)
**	    Added DB_DIS_TRAN_ID parm to LKcreate_list() prototype.
**	15-Dec-1999 (jenjo02)
**	    Added DB_DIS_TRAN_ID parm to LGbegin() prototype.
**	    Removed DB_DIS_TRAN_ID parm from LKcreate_list() prototype.
**	30-may-2001 (devjo01)
**	    Add lgclustr.h for CXcluster_enabled() macro. s103715.
**      01-sep-2004 (stial01)
**	    Support cluster online checkpoint
**	02-dec-2004 (thaju02)
**	    For dma_xxx_sidefile routines, TRdisplay only if verbose.
**      03-dec-2004 (stial01)
**          init_journal() bump jnl_fil_seq if it is same as ckp_f_jnl
**	10-jan-2005 (thaju02)
**	    Online modify of partitioned table. In dma_filter_sidefile,
**	    write log record to sidefile if db_tab_index < 0. 
**      01-apr-2005 (stial01)
**          init_journal() removed 03-dec-2004 changes
**          next_journal() if cluster return... ckpdb adds new jnl history entry
**      22-Mar-2006 (stial01)
**          dma_ebackup() If jnl switch requested after archive cycle started,
**          signal switch done anyway, so jsp (alterdb) can retry (b115385)
**      31-mar-2006 (stial01)
**          dma_ebackup() Moved JSWITCH processing to LG_archive_complete
**      10-dec-2007 (stial01)
**          Support archiving from RCP startup.
**      04-mar-2008 (stial01)
**          Write DM0LJNLSWITCH log record for incremental rollforwarddb
**      27-jun-2008 (stial01)
**          Added param to dm0l_opendb call.
**	10-Nov-2008 (jonj)
**	    SIR 120874 Change function prototypes to pass DB_ERROR *dberr
**	    instead of i4 *err_code, use new form uleFormat.
**      16-nov-2008 (stial01)
**          Redefined name constants without trailing blanks.
**	18-Nov-2008 (jonj)
**	    dm0d_?, dm0j_?, dm0c_? functions converted to DB_ERROR *
**	21-Nov-2008 (jonj)
**	    SIR 120874: dm0m_? functions converted to DB_ERROR *
**	25-Nov-2008 (jonj)
**	    SIR 120874: dm0l_? functions converted to DB_ERROR *
**	19-Aug-2009 (kschendel) 121804
**	    Need cx.h for proper CX declarations (gcc 4.3).
**      01-apr-2010 (stial01)
**          Changes for Long IDs, db_buffer holds (dbname, owner.. )
**/

/*
[@forward_type_references@]
[@forward_function_references@]
[@#defines_of_other_constants@]
[@type_definitions@]
[@global_variable_definitions@]
*/

/*
**  Definition of static variables and forward static functions.
*/

static	VOID	    acp_test_record(
    ACB		*acb,
    DM0L_HEADER	*record,
    i4		phase);

static i4 check_end_backup(
    ACB			*acb,
    i4		dbid,
    LG_LA		*la);

static DB_STATUS    copy_record_to_dump(
    ACB			*acb,
    DBCB		*db,
    LG_LA		*la,
    LG_RECORD		*header,
    DM0L_HEADER		*record);

static VOID dalloc_dbcbs(
    ACB	    	*acb);

static DB_STATUS mark_fully_closed( ACB *acb, DBCB *dbcb);

static DB_STATUS dma_ebackup(
    ACB			*acb);

static  DB_STATUS   error_translate(
    i4	    	    new_error,
    DB_ERROR	    *dberr);

static DB_STATUS init_dump(
    ACB	    		*acb,
    DBCB	    	*db,
    i4	  	fil_seq,
    i4	 	blk_seq);

static DB_STATUS init_journal(
    ACB	    		*acb,
    DBCB	    	*db);


static	DB_STATUS   next_journal(
    ACB		*acb,
    DBCB	*db,
    LG_RECORD	*header,
    DM0L_HEADER	*record);

static DB_STATUS log_2_dump(
    ACB	        *acb,
    DBCB	*db,
    LG_RECORD   *header,
    DM0L_HEADER *record,
    LG_LA	*la);

static VOID lookup_dbcb(
    ACB		*a,
    i4	db_id,
    DBCB	**ret_dbcb);

static	DB_STATUS   new_jnl_entry(
    ACB		*acb,
    DMP_DCB	*dcb,
    DM0L_SBACKUP    *record);

static DB_STATUS write_jnl_eof(
    ACB		*acb,
    DBCB	*db,
    DM0J_CTX	*openjnl,
    DM0C_CNF	*config,
    i4		jnl_seq,
    i4		blk_seq);

static DB_STATUS dma_alloc_sidefile(
    ACB		*a,
    DBCB	*dbcb,
    DM0L_BSF	*log_rec);

static DB_STATUS dma_write_sidefile(
    ACB		*a,
    DBCB	*dbcb,
    DM0L_HEADER	*record,
    SIDEFILE	*sf);

static DB_STATUS dma_close_sidefile(
    ACB		*a,
    DBCB	*dbcb,
    DM0L_ESF	*log_rec);

static DB_STATUS dma_filter_sidefile(
    ACB		*a,
    DBCB	*dbcb,
    DM0L_HEADER *record,
    SIDEFILE	**ret_sf);


/*{
** Name: dma_prepare	- Prepare to archive using only the log file.
**
** Description:
**      This routine is used to initialize archiver data structures
**	when the process starts up.
**
**	The following archiver knobs are also looked up:
**
**	archiver_refresh : indicates how often during an archive cycle to
**		update the database config files and logging and logging
**		system to record the amount of archive work completed so far.
**
**		It can be set if archive windows tend to be large (window 
**		size is determined by the system variables: cp_percent and 
**		archiver_interval) so that log file space can be periodically 
**		released as it is made available rather than waiting for the 
**		full archive cycle to complete.  This also lessens the chance
**		of rearchiving large amount of logfile unnecessarily after
**		a system crash.
**
**		It is actually implemented by terminating the current cycle
**		so that the normal end-of-cycle update can be done.  It is 
**		expected that if archive work still remains then a new archive
**		cycle will be immediately started.
**
**		Expressed in a percentage of the Log File size, it defaults
**		to 10%.  It can be set to zero (or 100) to turn off this 
**		behavior and always archive to the log file EOF whenever an 
**		archive cycle is signaled.
**
**
** Inputs:
**	flag				Specifies the archiver personality:
**					    DMA_ARCHIVER -- this is the local
**						archiver.
**					    DMA_CSP -- this is the CSP process
**						performing archiving for
**						another node following a node
**						or system crash.
**                                          DMA_RCP -- this is the RCP process
**                                              archiving in server startup
**                                              following server crash.
**                                              (Archive BEFORE redo recovery)
**	lctx				Logfile context. If this is the local
**					    archiver, this parameter may be
**					    zero, in which case the local
**					    logfile context is used. If this
**					    is the CSP, then this should be
**					    the logfile context for the node
**					    to be archived.
**
** Outputs:
**      acb                             Pointer to returned ACB.
**      err_code                        Reason for error status.
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
**      24-sep-1986 (Derek)
**          Created.
**      24-Jan-1989 (ac)
**          Added arguments to LGbegin().
**	21-jan-1989 (EdHsu)
**	    Add code to support online backup for Terminator
**	11-sep-1989 (rogerk)
**	    Added lx_id parameter to dm0l_opendb call.
**	 8-jan-1990 (rogerk)
**	    Initialize sbackup field in dbcb.
**	5-jun-1990 (bryanp)
**	    Explicitly clear the DCB portion of the DBCB when allocating it.
**	    This allows users of the DCB to test for uninitialized fields by
**	    comparing them with 0.
**	25-feb-1991 (rogerk)
**	    Add changes for Archiver Stability project.
**	    Initialize new exit error information fields.
**	    As databases are opened for CSP archive processing, check the
**	    database journaling status for JOURNAL_DISABLE.  Journal
**	    processing should be bypassed on those databases.
**	17-mar-1991 (bryanp)
**	    B36228. Remove acb_current_lps (replaced by dbcb_current_lps).
**	    Initialize dbcb_current_lps when allocating DBCB.
**	19-aug-1991 (rogerk)
**	    Set process type in the svcb.  This was done as part of the
**	    bugfix for B39017.  This allows low-level DMF code to determine
**	    the context in which it is running (Server, RCP, ACP, utility).
**	    The process type is set only if running as the ACP (not when
**	    the CSP is doing archive work).
**	28-dec-1992 (jnash)
**	    Reduced logging project.  Changes correlating with
**	    new archiver design.  Eliminate LPS fields and log records; we 
**	    now merge clustered journals using system-wide LSN's.  No longer 
**	    have the CSP read cp and perform log scan in this routine.
**	18-jan-1993 (rogerk)
**	    Added II_DMF_TRACE handling.
**	26-apr-1993 (bryanp)
**	    6.5 Cluster support:
**		Added lctx parameter so that the caller can specify a
**		    particular logfile to work on. If no logfile is specified,
**		    the archiver works on the local logfile.
**		Documented this routine's parameters.
**	26-jul-1993 (bryanp)
**	    We no longer log a closedb log record, so we no longer need the
**		'lx_id' argument to dm0l_opendb() and dm0l_closedb().
**	26-jul-1993 (rogerk)
**	  - Added archiver_refresh PM variable which directs the archiver to
**	    complete archive cycles after some maximum percentage of the log
**	    file is processed.
**	  - Changed acb_verbose to use DM1313 to be different than rcp verbose.
**	20-sep-1993 (bryanp)
**	    Set acb_s_lxid if this is being invoked by the CSP doing remote log
**		archiving.
**	28-mar-1994 (bryanp) B59975
**	    Remove node_id from the dma_prepare interface. Set acb_node_id to
**		lctx_node_id instead. In a non-cluster environment, lctx_node_id
**		is zero, set to such by dm0l_allocate. In a cluster
**		environment, lctx_node_id is the local node id if we are the
**		local archiver, and is the remote log's node id if we are a csp
**		doing node-failure archiving or restart archiving.
**	25-apr-1994 (bryanp) B60709, B60230
**	    Show the log header when preparing to perform cluster archiving.
**	    During cluster node failure recovery the log file may have been
**		changed (due to CLR writing, moving forward of the BOF, etc.),
**		so the lctx_header in the lctx may be out of date.
**	3-Dec-2004 (schka24)
**	    Don't allow archiver_refresh of zero, causes looping.
**	15-Aug-2005 (jenjo02)
**	    Implement multiple DMF LongTerm memory pools and
**	    ShortTerm session memory.
**	    Call dm0m_init() to set it up.
**	    
[@history_template@]...
*/
DB_STATUS
dma_prepare(
i4	    flag,
ACB	    **acb,
DMP_LCTX    *lctx,
DB_ERROR    *dberr)
{
    static DM_SVCB  svcb ZERO_FILL;
    ACB		    *a;
    DB_OWN_NAME	    user_name;
    DB_TRAN_ID	    tran_id;
    char	    *pmvalue;
    i4	    i;
    f8		    float_val;
    DB_STATUS	    status;
    STATUS	    cl_status;
    CL_ERR_DESC	    sys_err;
    i4		length;
    LG_HDR_ALTER_SHOW   show_hdr;
    i4		    error;
    DB_DB_NAME	archiver_db;

    CLRDBERR(dberr);


    if (flag == DMA_ARCHIVER)
    {
        /*	Initialize the DMF SERVER CONTROL BLOCK. */

	dmf_svcb = &svcb;
	svcb.svcb_status = (SVCB_SINGLEUSER | SVCB_ACTIVE | SVCB_ACP);

	/* Init DMF memory manager */
	dm0m_init();

	/*	Initialize the logging system for this process. */

	status = dm0l_allocate(DM0L_ARCHIVER, 0, 0, 0, 0, (DMP_LCTX **)0,
				dberr);
	if (status != E_DB_OK)
	    return (error_translate(E_DM9812_ARCH_OPEN_LOG, dberr));
    }
    if (lctx == 0)
	lctx = dmf_svcb->svcb_lctx_ptr;

    if (flag == DMA_CSP)
    {
	/* Ensure that our log header information is up to date */

	show_hdr.lg_hdr_lg_id = lctx->lctx_lgid;
	status = LGshow(LG_A_NODELOG_HEADER, (PTR)&show_hdr, 
			    sizeof(show_hdr), &length, &sys_err);
	if (status)
	{
	    uleFormat(NULL, status, &sys_err, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	    uleFormat(NULL, E_DM9017_BAD_LOG_SHOW, &sys_err, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &error, 1,
		0, LG_A_HEADER);
	    SETDBERR(dberr, 0, E_DM9810_ARCH_LOG_BEGIN);
	    return (error_translate(E_DM9812_ARCH_OPEN_LOG, dberr));
	}
	lctx->lctx_lg_header = show_hdr.lg_hdr_lg_header;
    }

    /*
    **  Set DMF trace flags for this utility.
    */
    {
	char		*trace = NULL;

	NMgtAt("II_DMF_TRACE", &trace);
	if ((trace != NULL) && (*trace != EOS))
	{
	    char    *comma;
	    i4 trace_point;

	    do
	    {
		if (comma = STchr(trace, ','))
		    *comma = EOS;
		if (CVal(trace, &trace_point))
		    break;
		trace = &comma[1];
		if (trace_point < 0 ||
		    trace_point >= DMZ_CLASSES * DMZ_POINTS)
		{
		    continue;
		}

		TRdisplay("%@ DMA_PREPARE: Set trace flag %d\n", trace_point);
		DMZ_SET_MACRO(dmf_svcb->svcb_trace, trace_point);
	    } while (comma);
	}
    }

    /*	Allocate the ACB. */
    status = dm0m_allocate(
	sizeof(ACB) + (ACB_DB_HASH + 1) * sizeof(DBCB *),
	0, ACB_CB, ACB_ASCII_ID, 
	0, (DM_OBJECT **)acb, dberr);

    if (status != E_DB_OK)
	return (error_translate(E_DM9812_ARCH_OPEN_LOG, dberr));
    a = *acb;

    /*	Initialize the ACB. */

    a->acb_dbh = (DBCB**)((char *)a + sizeof(ACB));
    a->acb_db_state.stq_next = (ACP_QUEUE *)&a->acb_db_state.stq_next;
    a->acb_db_state.stq_prev = (ACP_QUEUE *)&a->acb_db_state.stq_next;
    a->acb_c_dbcb = 0;
    a->acb_lctx = lctx;
    a->acb_status = 0;
    a->acb_test_flag = 0;
    a->acb_verbose = FALSE;
    a->acb_sfq_next = (SIDEFILE *) &a->acb_sfq_next;
    a->acb_sfq_prev = (SIDEFILE *) &a->acb_sfq_next;
    a->acb_sfq_count = 0;
    CLRDBERR(&a->acb_dberr);

    for (i = 0; i <= ACB_DB_HASH; i++)
	a->acb_dbh[i] = 0;

    if (DMZ_ASY_MACRO(13))
	a->acb_verbose = TRUE;

    /*
    ** Initialize ACP termination information - set exit error code to
    ** UNKNOWN error in case we exit without setting it to anything.
    ** Zero-fill the error database name - if this holds non-zero data
    ** at exit time then we assume that it holds the name of the database
    ** on which an error was encountered.
    */
    a->acb_error_code = E_DM985F_ACP_UNKNOWN_EXIT;
    a->acb_errltext = 0;
    MEfill(sizeof(a->acb_errdb), '\0', (char *)&a->acb_errdb);

    /*
    ** Lookup ARCHIVER tuning knobs.  These are explained above.
    */

    /*
    ** Archiver refresh value: convert from logfile percentage to # log blocks.
    */
    a->acb_max_cycle_size = lctx->lctx_lg_header.lgh_count / 10;
    cl_status = PMget("ii.$.rcp.log.archiver_refresh", &pmvalue);
    if ((cl_status == OK) && (pmvalue))
    {
	cl_status = CVaf(pmvalue, '.', &float_val);
	if ((cl_status == OK) && (float_val > 0) && (float_val <= 100))
	{
	    a->acb_max_cycle_size = 
			lctx->lctx_lg_header.lgh_count * float_val / 100;
	    if (a->acb_max_cycle_size == 0)
		a->acb_max_cycle_size = 1;

	    TRdisplay("%@ DMA_PREPARE: Set archive refresh to %d blocks.\n",
		a->acb_max_cycle_size);
	}
	else
	{
	    TRdisplay("%@ ACP: Illegal archiver_refresh value.\n",
		a->acb_max_cycle_size);
	}
    }

    /* init total active backup database */
    a->acb_bkdb_cnt = 0;
    if (CXcluster_enabled())
	a->acb_status |= ACB_CLUSTER;
    a->acb_node_id = lctx->lctx_node_id;
    if (flag == DMA_CSP)
	a->acb_status |= (ACB_CLUSTER | ACB_CSP);
    else if (flag == DMA_RCP)
	a->acb_status |= ACB_RCP;

    if (flag == DMA_ARCHIVER)
    {
	/*	Begin the Archiver database. */
	STmove(DB_ARCHIVER_INFO, ' ', sizeof(DB_DB_NAME), (PTR)&archiver_db);
	status = dm0l_opendb(
	    lctx->lctx_lgid, 
	    DM0L_NOTDB,
	    &archiver_db,
	    (DB_OWN_NAME *)DB_INGRES_NAME,
	    (i4)0,
	    (DM_PATH *)"None", 
	    4, 
	    &a->acb_db_id, 
	    (LG_LSN *)0,
	    dberr);

	if (status != E_DB_OK)
	    return (error_translate(E_DM9812_ARCH_OPEN_LOG, dberr));

	/*	Start an archiver transaction so that we can read the log. */

	STmove((PTR)DB_ARCHIVER_INFO, ' ', sizeof(DB_OWN_NAME), (PTR) &user_name);
	status = LGbegin(LG_NOPROTECT, a->acb_db_id, &tran_id, &a->acb_s_lxid,
	         sizeof(DB_OWN_NAME), &user_name.db_own_name[0], 
		 (DB_DIS_TRAN_ID*)NULL, &sys_err);
	if (status != OK)
	{
	    uleFormat(NULL, status, &sys_err, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	    uleFormat(NULL, E_DM900C_BAD_LOG_BEGIN, &sys_err, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	    SETDBERR(dberr, 0, E_DM9810_ARCH_LOG_BEGIN);
	    return (error_translate(E_DM9812_ARCH_OPEN_LOG, dberr));
	}
	    
	/*	Get transaction lock list for any locks we might need. */

	status = LKcreate_list(LK_ASSIGN | LK_NONPROTECT, 0, 0, 
				(LK_LLID *)&a->acb_lk_id, 0, &sys_err);
	if (status != OK)
	{
	    uleFormat(NULL, status, &sys_err, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	    uleFormat(NULL, E_DM901A_BAD_LOCK_CREATE, &sys_err, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	    SETDBERR(dberr, 0, E_DM9814_ARCH_LOCK_CREATE);
	    return (error_translate(E_DM9812_ARCH_OPEN_LOG, dberr));
	}

	return (E_DB_OK);
    }
    else
    {
	a->acb_s_lxid = lctx->lctx_lxid;

	/*	Get transaction lock list for any locks we might need. */

	status = LKcreate_list(LK_ASSIGN | LK_NONPROTECT | LK_MASTER, 0, 0, 
		    (LK_LLID *)&a->acb_lk_id, 0, &sys_err);
	if (status != OK)
	{
	    uleFormat(NULL, status, &sys_err, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	    uleFormat(NULL, E_DM901A_BAD_LOCK_CREATE, &sys_err, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	    SETDBERR(dberr, 0, E_DM9814_ARCH_LOCK_CREATE);
	    return (error_translate(E_DM9812_ARCH_OPEN_LOG, dberr));
	}
    }

    return (E_DB_OK);
}

/*{
** Name: dma_complete	- Complete archive processing.
**
** Description:
**      This routine is called when a SHUTDOWN event is received, and
**	is responsible for cleaning up data structures and detaching from 
**	the logging system.
**
** Inputs:
**      acb                             Pointer to ACB.
**
** Outputs:
**      err_code                        Reason for error status.
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
**      24-sep-1986 (Derek)
**          Created.
**	22-Sept-1988 (Edhsu)
**	    Add ecp after bcp
**	21-jan-1989 (EdHsu)
**	    Add code to support online backup for Terminator.
**	 8-jan-1990 (rogerk)
**	    Check backup_complete field before signaling FBACKUP.
**	9-aug-1991 (bryanp)
**	    Fix parameter to E_DM900E and to E_DM9016.
**	08-jan-1993 (jnash)
**	    Modified for use with new archiver design.
**	22-feb-1993 (jnash)
**	    Call dalloc_dbcbs() to deallocate dbcbs.
**	15-mar-1993 (rogerk)
**	    Reduced Logging - Phase IV:
**	    Changed ECP to point back to the BCP via the LSN rather than a
**		log address.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	26-jul-1993 (rogerk)
**	    Took out ifdef'd code that writes a Consistency Point at the 
**	    end of offline archiving.
**	15-jan-1999 (nanpr01)
**	    Pass pointer to pointer to dma_complete function.
**	22-Feb-1999 (bonro01)
**	    dm0l_deallocate() was modified to remove the explicit assignment
**	    of dmf_svcb->svcb_lctx_ptr to NULL, so a check must be added
**	    in dma_complete() to determine if the LCTX we are deallocating
**	    is the same one referenced in dmf_svcb->svcb_lctx_ptr.
[@history_template@]...
*/
DB_STATUS
dma_complete(
ACB	**acb,
DB_ERROR *dberr)
{
    ACB			*a = *acb;
    DB_STATUS		status;
    STATUS		cl_status;
    CL_ERR_DESC		sys_err;
    i4			error;

    CLRDBERR(dberr);

    /*
    ** Deallocate DBCBs.
    */
    dalloc_dbcbs(*acb);

    if ((a->acb_status & (ACB_CSP | ACB_RCP)) == 0)
    {
	cl_status = LGend(a->acb_s_lxid, 0, &sys_err);
	if (cl_status != OK)
	{
	    uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	    uleFormat(NULL, E_DM900E_BAD_LOG_END, &sys_err, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &error, 1,
		0, a->acb_s_lxid);
	    SETDBERR(dberr, 0, E_DM9811_ARCH_LOG_END);
	    return (error_translate(E_DM9813_ARCH_CLOSE_LOG, dberr));
	}

	cl_status = LGremove(a->acb_db_id, &sys_err);
	if (cl_status != OK)
	{
	    uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	    uleFormat(NULL, E_DM9016_BAD_LOG_REMOVE, &sys_err, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &error, 1,
		0, a->acb_db_id);
	    SETDBERR(dberr, 0, E_DM980F_ARCH_LOG_REMOVE);
	    return (error_translate(E_DM9813_ARCH_CLOSE_LOG, dberr));
	}

	/*
	** Deallocate the LCTX and clear the svcb_lctx_ptr if it is the
	** same one being cleared.  When this routine is called by
	** csp_archive(), this pointer should be different.
	*/
	{
	    bool clear_lctx_ptr = (dmf_svcb->svcb_lctx_ptr == a->acb_lctx);
	    status = dm0l_deallocate(&a->acb_lctx, dberr);
	    if(clear_lctx_ptr)
		dmf_svcb->svcb_lctx_ptr = a->acb_lctx; /* synchronize fields */
	    if (status != E_DB_OK)
		return (error_translate(E_DM9813_ARCH_CLOSE_LOG, dberr));
	}
    }

    if (cl_status = LKrelease(LK_ALL, a->acb_lk_id, 0, 0, 0, &sys_err))
    {
	uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
	    (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	uleFormat(NULL, E_DM901B_BAD_LOCK_RELEASE, &sys_err, ULE_LOG, NULL, 
	    (char *)NULL, (i4)0, (i4 *)NULL, &error, 1, 
	    sizeof(i4), (i4)LK_A_CSPID);
	SETDBERR(dberr, 0, E_DM980F_ARCH_LOG_REMOVE);
	return (error_translate(E_DM9813_ARCH_CLOSE_LOG, dberr));
    }
    dm0m_deallocate((DM_OBJECT **)acb);
    return (E_DB_OK);
}

/*{
** Name: dma_online_context	- Get information on DB's to process
**
** Description:
**      This routine collects a new set of journaled databases for the ACP
**	from the logging system and builds the appropriate context.
**
**	At the end of this routine, the following fields will be
**	initialized in the ACB:
**
**	    a->acb_bof_addr	: Current Log File BOF.
**	    a->acb_eof_addr	: Current Log File EOF.
**	    a->acb_cp_addr	: Most recent logging system Consistency Point.
**	    a->acb_start_addr	: Address at which to start archiving.
**	    a->acb_stop_addr	: Address to which to archive.
**
**	The Start and Stop addresses are initially built using the Logging
**	System's current Archive Window.  The Start address may be adjusted
**	in dma_soc after reviewing the open databases' Config Files.
**
**	If the archive window spans more than 10% of the log file size,
**	the Stop address will be set back to make sure that we flush
**	journal records and update logging system journal information at
**	least every 10% of the log file.
**
** Inputs:
**      acb                             Pointer to ACB.
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
**	07-jan-1993 (rogerk & jnash)
**	    Written for Reduced logging project.
**	27-jan-1993 (jnash)
**	    Online backup fixes.	
**	22-feb-1993 (jnash)
**	    Access DBCB as a hash queue hashed on the external db id.
**	    Fix online backup problems.
**	15-mar-1993 (rogerk)
**	    Reduced Logging - Phase IV:
**	    Fixed to not update minimum starting log address with database
**		journal and dump values unless they are non-zero.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	26-jul-1993 (rogerk)
**	  - Changed the way journal and dump windows are handled.  They are
**	    now tracked by real Log Addresses rather than the Consistency
**	    points that bound them.  Consolidated the system journal and
**	    dump windows into a single archive window.  We now request the
**	    system archive window from LG and use that for the archiver start
**	    and stop addresses.
**	  - Changed tracking of last CP address.  This value is now initialized
**	    to zero rather than the begin address of the archive window.
**	  - Added optimization to bypass any work on databases whose current
**	    journal and dump windows are zero.
**	  - Changed trace messages and error handling in this routine.
**	18-oct-1993 (jnash)
**	    Add log address information to E_DM9845_ARCH_WINDOW, add new
**	    E_DM9846_ARCH_WINDOW error.
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Call LGalter(LG_A_ARCHIVE_JFIB) when 
**	    starting archive of a MVCC database.
*/
DB_STATUS
dma_online_context(
ACB	*acb)
{
    ACB			*a = acb;
    ACP_QUEUE		*st;
    DBCB		*dbcb;
    DB_DB_NAME		*dbname;
    DB_OWN_NAME		*dbowner;
    DM_PATH		*dbpath;
    i4			*i4_ptr;
    i4			path_len;
    LG_HEADER		header;
    LG_DATABASE		db;
    LG_LA		lga;
    LG_LSN		nlsn;
    LG_LA		min_address;
    i4		lgshow_index;
    i4		db_count = 0;
    i4		length;
    bool		lgerror = FALSE;
    CL_ERR_DESC		sys_err;
    STATUS		cl_status;
    DB_STATUS		status = E_DB_OK;
    i4			error;

    CLRDBERR(&a->acb_dberr);

    for (;;)
    {
	TRdisplay("%@ ACP DMA_ONLINE_CONTEXT: Building Archiver context.\n");

	/*
	** Find the Current BOF, this is the earliest point from which we 
	** may have to begin archiving.  We may find later (upon examination
	** of journaled db's) that we can start archiving at a much later
	** log address.
	*/
	cl_status = LGshow(LG_A_HEADER, (PTR)&header, sizeof(header), &length, 
	    &sys_err);
	if (cl_status != E_DB_OK)
	{
	    uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	    uleFormat(NULL, E_DM9017_BAD_LOG_SHOW, &sys_err, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &error, 1,
		0, LG_A_HEADER);
	    status = E_DB_ERROR;
	    lgerror = TRUE;
	    break;
	}

	STRUCT_ASSIGN_MACRO(header.lgh_begin, a->acb_bof_addr);
	STRUCT_ASSIGN_MACRO(header.lgh_cp, a->acb_cp_addr);
	STRUCT_ASSIGN_MACRO(header.lgh_count, a->acb_blk_count);
	STRUCT_ASSIGN_MACRO(header.lgh_size, a->acb_blk_size);

	/*	
	** Get the log file eof, the point to which we shall archive.
	*/
	cl_status = LGshow(LG_A_EOF, (PTR)&lga, sizeof(lga), &length, &sys_err);
	if (cl_status != E_DB_OK)
	{
	    uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	    uleFormat(NULL, E_DM9017_BAD_LOG_SHOW, &sys_err, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &error, 1, 
		0, LG_A_EOF);
	    status = E_DB_ERROR;
	    lgerror = TRUE;
	    break;
	}

	STRUCT_ASSIGN_MACRO(lga, a->acb_eof_addr);

	if (a->acb_verbose)
	{
	    TRdisplay("\tLog File BOF (%d,%d,%d) .. EOF (%d,%d,%d)\n",
		a->acb_bof_addr.la_sequence,
		a->acb_bof_addr.la_block,
		a->acb_bof_addr.la_offset,
		a->acb_eof_addr.la_sequence,
		a->acb_eof_addr.la_block,
		a->acb_eof_addr.la_offset);
	    TRdisplay("\tLog File CP  (%d,%d,%d)\n",
		a->acb_cp_addr.la_sequence,
		a->acb_cp_addr.la_block,
		a->acb_cp_addr.la_offset);
	}

	/*
	** Get the current Archive window from the Logging System.  This
	** gives us the start and stop log addresses for the archive cycle.
	*/
	cl_status = LGshow(LG_S_ACP_START, 
				(PTR)&lga, sizeof(lga), &length, &sys_err);
	if (cl_status != E_DB_OK)
	{
	    uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	    uleFormat(NULL, E_DM9017_BAD_LOG_SHOW, &sys_err, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &error, 1,
		0, LG_S_ACP_START);
	    status = E_DB_ERROR;
	    lgerror = TRUE;
	    break;
	}
	STRUCT_ASSIGN_MACRO(lga, a->acb_start_addr);

	cl_status = LGshow(LG_S_ACP_END,
				(PTR)&lga, sizeof(lga), &length, &sys_err);
	if (cl_status != E_DB_OK)
	{
	    uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	    uleFormat(NULL, E_DM9017_BAD_LOG_SHOW, &sys_err, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &error, 1,
		0, LG_S_ACP_END);
	    status = E_DB_ERROR;
	    lgerror = TRUE;
	    break;
	}
	STRUCT_ASSIGN_MACRO(lga, a->acb_stop_addr);

	TRdisplay("\tArchive Window (%d,%d,%d)..(%d,%d,%d)\n",
	    a->acb_start_addr.la_sequence,
	    a->acb_start_addr.la_block,
	    a->acb_start_addr.la_offset,
	    a->acb_stop_addr.la_sequence,
	    a->acb_stop_addr.la_block,
	    a->acb_stop_addr.la_offset);

	/*
	** Show the CP address previous to the Archive Window so we can
	** trace its value.
	*/
	cl_status = LGshow(LG_S_ACP_CP,
				(PTR)&lga, sizeof(lga), &length, &sys_err);
	if (cl_status != E_DB_OK)
	{
	    uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	    uleFormat(NULL, E_DM9017_BAD_LOG_SHOW, &sys_err, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &error, 1,
		0, LG_S_ACP_CP);
	    status = E_DB_ERROR;
	    lgerror = TRUE;
	    break;
	}

	TRdisplay("\tArchive Window Previous CP (%d,%d,%d)\n",
	    lga.la_sequence,
	    lga.la_block,
	    lga.la_offset);

	/*
	** Force the log file out to the current EOF to make sure that we
	** can LGread up to the eof address we just stored away.
	*/
	cl_status = LGforce(LG_LAST, a->acb_s_lxid, 0, &nlsn, &sys_err);
	if (cl_status != OK)
	{
	    uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	    uleFormat(NULL, E_DM9010_BAD_LOG_FORCE, &sys_err, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &error, 1,
		0, a->acb_s_lxid);
	    lgerror = TRUE;
	    status = E_DB_ERROR;
	    break;
	}


	/*
	** Get a list of open databases from the logging system and find
	** those that are journaled or undergoing online backup.  These
	** may require processing by the archiver.
	*/
	lgshow_index = 0;
	while (status == E_DB_OK)
	{
	    cl_status = LGshow(LG_N_DATABASE, (PTR)&db, sizeof(db), 
			    &lgshow_index, &sys_err);
	    if (cl_status != OK)
	    {
		uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
		uleFormat(NULL, E_DM9017_BAD_LOG_SHOW, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, &error, 1,
		    0, LG_N_DATABASE);
		lgerror = TRUE;
		status = E_DB_ERROR;
		break;
	    }

	    /*
	    ** If we have exhausted the database list, then we are finished.
	    */
	    if (lgshow_index == 0)
		break;

	    /*
	    ** Skip databases which are neither journaled nor undergoing
	    ** online backup.  Databases which are listed as DB_STALL or
	    ** DB_FBACKUP have either not started or have completed backup
	    ** processing and can be skipped even if they say DB_BACKUP.
	    */
	    if (( ! (db.db_status & DB_JNL)) &&
		(( ! (db.db_status & DB_BACKUP)) || 
		    (db.db_status & DB_FBACKUP)))
	    {
		continue;
	    }

	    /*
	    ** Skip databases which have no journal or dump records to
	    ** process in this archive window.
	    */
	    if ((db.db_f_jnl_la.la_block == 0) && 
		(db.db_l_jnl_la.la_block == 0) &&
		(db.db_f_dmp_la.la_block == 0) &&
		(db.db_l_dmp_la.la_block == 0) &&
		((db.db_status & DB_JSWITCH) == 0) )
	    {
		TRdisplay("\tDatabase %~t\n", sizeof(DB_DB_NAME), db.db_buffer);
		TRdisplay("\t    Journal Window: (%d,%d,%d)..(%d,%d,%d)\n",
		    db.db_f_jnl_la.la_sequence,
		    db.db_f_jnl_la.la_block,
		    db.db_f_jnl_la.la_offset,
		    db.db_l_jnl_la.la_sequence,
		    db.db_l_jnl_la.la_block,
		    db.db_l_jnl_la.la_offset);
		continue;
	    }

	    /*
	    ** If journaled MVCC database with something to archive,
	    ** or we need to do a JSWITCH, notify LG that its time
	    ** to check and possibly advance its LG_JFIB to the
	    ** next journal file sequence.
	    */
	    if ( db.db_status & DB_MVCC && db.db_status & DB_JNL &&
	         (db.db_f_jnl_la.la_block != 0 || db.db_status & DB_JSWITCH) )
	    {
	        cl_status = LGalter(LG_A_ARCHIVE_JFIB, (PTR)&db,
					sizeof(db), &sys_err);
		if ( cl_status != OK )
		{
		    uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
			(char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
		    uleFormat(NULL, E_DM900B_BAD_LOG_ALTER, &sys_err, ULE_LOG, NULL,
			(char *)NULL, (i4)0, (i4 *)NULL, &error, 1,
			0, LG_A_ARCHIVE_JFIB);
		    lgerror = TRUE;
		    status = E_DB_ERROR;
		    break;
		}
	    }

	    /*
	    ** Extract DB information out of info buffer where it has
	    ** been packed into the db_buffer field.  Sanity check the
	    ** size of the buffer to make sure it actually holds what
	    ** we expect it to.
	    */
	    i4_ptr = (i4 *) &db.db_buffer[DB_DB_MAXNAME + DB_OWN_MAXNAME + 4];
	    I4ASSIGN_MACRO(*i4_ptr, path_len);
	    if (db.db_l_buffer < (DB_DB_MAXNAME + DB_OWN_MAXNAME + path_len + 8))
	    {
		TRdisplay("LGshow DB buffer size mismatch (%d vs %d)\n",
		    (DB_DB_MAXNAME + DB_OWN_MAXNAME + path_len + 8), db.db_l_buffer);
		SETDBERR(&a->acb_dberr, 0, E_DM9842_ARCH_SOC);
		lgerror = TRUE;
		status = E_DB_ERROR;
		break;
	    }

	    dbname = (DB_DB_NAME *) &db.db_buffer[0];
	    dbowner = (DB_OWN_NAME *) &db.db_buffer[DB_DB_MAXNAME];
	    dbpath = (DM_PATH *) &db.db_buffer[DB_DB_MAXNAME + DB_OWN_MAXNAME + 8];

	    status = dma_alloc_dbcb(a, db.db_database_id, db.db_id,
		dbname, dbowner, dbpath, path_len, 
		(LG_LA *)&db.db_f_jnl_la, (LG_LA *)&db.db_l_jnl_la,
		(LG_LA *)&db.db_f_dmp_la, (LG_LA *)&db.db_l_dmp_la,
		(LG_LA *)&db.db_sbackup, &dbcb);
	    if (status != E_DB_OK)
		break;
	    db_count++;

	    /* If archiving a journaled MVCC database... */
	    if ( db.db_status & DB_MVCC && db.db_status & DB_JNL &&
	         db.db_f_jnl_la.la_block != 0 )
	    {
		/* This is needed in dm0c_close */
	        dbcb->dbcb_dcb.dcb_status |= DCB_S_MVCC;

		dbcb->dbcb_status |= DBCB_MVCC;

		/* Prevents carving up "too big" archives */
		a->acb_status |= ACB_MVCC;

		TRdisplay("\t    Database is MVCC.\n");
	    }

	    /*
	    ** Set important database status information.
	    */
	    if (db.db_status & DB_PURGE)
	    {
		TRdisplay("\t    Database is being Purged.\n");
		dbcb->dbcb_status |= DBCB_PURGE;
	    }

	    if (db.db_status & DB_JNL)
	    {
		TRdisplay("\t    Database is Journaled.\n");
		dbcb->dbcb_status |= DBCB_JNL;
	    }

	    if (db.db_status & DB_JSWITCH)
	    {
		TRdisplay("\t    Database is Journal switching.\n");
		dbcb->dbcb_status |= DBCB_JSWITCH;
	    }

	    if ((db.db_status & DB_BACKUP) &&   
		( ! (db.db_status & DB_FBACKUP)))
	    {
		a->acb_bkdb_cnt++;
		dbcb->dbcb_status |= DBCB_BACKUP;
		TRdisplay("\t    Database is undergoing Online Backup:\n");
		TRdisplay("\t\tDump Window: (%d,%d,%d)..(%d,%d,%d)\n",
		    dbcb->dbcb_first_dmp_la.la_sequence,
		    dbcb->dbcb_first_dmp_la.la_block,
		    dbcb->dbcb_first_dmp_la.la_offset,
		    dbcb->dbcb_last_dmp_la.la_sequence,
		    dbcb->dbcb_last_dmp_la.la_block,
		    dbcb->dbcb_last_dmp_la.la_offset);
		TRdisplay("\t\tSBackup    Address: (%d,%d,%d)\n",
		    dbcb->dbcb_sbackup.la_sequence,
		    dbcb->dbcb_sbackup.la_block,
		    dbcb->dbcb_sbackup.la_offset);
	    }

	    /*
	    ** Consistency Check:  Verify that this database's journal and
	    ** dump windows are spanned by the current archiver window.
	    ** If not, report an error and reset the window back to include
	    ** this database's dump and journal information.
	    */
	    if ((dbcb->dbcb_first_jnl_la.la_sequence) &&
		(LGA_LT(&dbcb->dbcb_first_jnl_la, &a->acb_start_addr)))
	    {
		TRdisplay(
		   "%@ ACP DMA_ONLINE_CONTEXT: Journal/archive window inconsistency.\n");
		uleFormat(NULL, E_DM9845_ARCH_WINDOW, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, &error, 6,
		    0, dbcb->dbcb_first_jnl_la.la_sequence,
		    0, dbcb->dbcb_first_jnl_la.la_block,
		    0, dbcb->dbcb_first_jnl_la.la_offset,
		    0, a->acb_start_addr.la_sequence,
		    0, a->acb_start_addr.la_block,
		    0, a->acb_start_addr.la_offset);
		STRUCT_ASSIGN_MACRO(dbcb->dbcb_first_jnl_la, a->acb_start_addr);
	    }

	    if ((dbcb->dbcb_first_dmp_la.la_sequence) &&
		(LGA_LT(&dbcb->dbcb_first_dmp_la, &a->acb_start_addr)))
	    {
		TRdisplay(
		   "%@ ACP DMA_ONLINE_CONTEXT: Dump/archive window inconsistency.\n");
		uleFormat(NULL, E_DM9845_ARCH_WINDOW, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, &error, 6,
		    0, dbcb->dbcb_first_dmp_la.la_sequence,
		    0, dbcb->dbcb_first_dmp_la.la_block,
		    0, dbcb->dbcb_first_dmp_la.la_offset,
		    0, a->acb_start_addr.la_sequence,
		    0, a->acb_start_addr.la_block,
		    0, a->acb_start_addr.la_offset);
		STRUCT_ASSIGN_MACRO(dbcb->dbcb_first_dmp_la, a->acb_start_addr);
	    }
	}

	/*
	** If we broke out of the above LGshow loop with an error then
	** fall down to error handling.
	*/
	if (status != E_DB_OK)
	    break;

	/*
	** Consistency Check:  Make sure that the archiver window falls inside
	** the current Log File legal address range.  Otherwise terrible errors
	** will result.
	*/
	if ((a->acb_start_addr.la_sequence) &&
	    ((LGA_LT(&a->acb_start_addr, &a->acb_bof_addr)) ||
	     (LGA_GT(&a->acb_start_addr, &a->acb_eof_addr))))
	{
	    TRdisplay(
	       "%@ ACP DMA_ONLINE_CONTEXT: Log file/archive window inconsistency.\n");
	    uleFormat(NULL, E_DM9846_ARCH_WINDOW, &sys_err, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &error, 9,
		0, a->acb_start_addr.la_sequence,
		0, a->acb_start_addr.la_block,
		0, a->acb_start_addr.la_offset,
		0, a->acb_bof_addr.la_sequence,
		0, a->acb_bof_addr.la_block,
		0, a->acb_bof_addr.la_offset,
		0, a->acb_eof_addr.la_sequence,
		0, a->acb_eof_addr.la_block,
		0, a->acb_eof_addr.la_offset);

	    status = E_DB_ERROR;
	    lgerror = TRUE;
	    break;
	}

	/*
	** Initialize dynamic last record and CP log addresses.
	** These will be set by the main archive loop and at the end
	** of the archive cycle will be the log addresses of the last
	** log record processed and last ECP record seen, respectively.
	*/
	a->acb_last_la.la_sequence = 0;   
	a->acb_last_la.la_block    = 0;   
	a->acb_last_la.la_offset   = 0;   
	a->acb_last_cp.la_sequence = 0;   
	a->acb_last_cp.la_block    = 0;   
	a->acb_last_cp.la_offset   = 0;   

	TRdisplay("\t%d Databases to process.\n", db_count);
	TRdisplay("%@ ACP DMA_ONLINE_CONTEXT: Archiver context complete.\n");

	return (E_DB_OK);
    }

    /*
    ** Error handling.
    */

    /*
    ** If an error occurred in the LGshow calls above, fill in exit error 
    ** information for archiver exit handler.  For LG errors, the err_code
    ** value has not been filled in.
    */
    if (lgerror)
    {
	a->acb_error_code = E_DM9855_ACP_LG_ACCESS_EXIT;
	uleFormat(NULL, a->acb_error_code, (CL_ERR_DESC *)NULL, ULE_LOOKUP, 
	    (DB_SQLSTATE *)NULL, a->acb_errtext, sizeof(a->acb_errtext), 
	    &a->acb_errltext, &error, 0);
    }
    else
    {
	uleFormat(&a->acb_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
	    (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
    }

    SETDBERR(&a->acb_dberr, 0, E_DM9842_ARCH_SOC);
    return (E_DB_ERROR);
}

/*{
** Name: dma_offline_context	- Get information on DB's to process
**
** Description:
**      This routine collects a new set of journaled databases for the ACP
**	by scanning the log file which is to be archived. Then it builds
**	the appropriate context.
**
**	A future optimization would be to change this routine so that instead
**	of scanning the logfile to figure out which databases require processing
**	we would ask the multi-node logging system this question. The multi-
**	node logging system should know the answer, because by the time we call
**	this routine we've just finished recovering this node's logfile, and
**	during the recovery process we figured out which databases required
**	archive processing.
**
** Inputs:
**      acb                             Pointer to ACB.
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
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**		Wrote this new routine. If this turns into a performance
**		    nightmare, we might wish to revisit this routine.
**		Pass correct transaction handle to dm0l_read.
**	26-jul-1993 (rogerk)
**	  - Changed the way journal and dump windows are handled.  They are
**	    now tracked by real Log Addresses rather than the Consistency
**	    points that bound them.  Consolidated the system journal and
**	    dump windows into a single archive window.  Scan through the
**	    database contexts built here and init the archive start and
**	    stop addresses using the union of all database windows.
**	  - Changed tracking of last CP address.  This value is now initialized
**	    to zero rather than the begin address of the archive window.
**	20-sep-1993 (bryanp)
**	    Set acb_start_addr to bof, acb_stop_addr to eof.
**	    Pass reasonable journal window addresses when allocating dbcb.
**	    Check for existing dbcb before allocating new one.
**	04-Feb-2010 (jonj)
**	    SIR 121619 MVCC: Add bufid parameter to dm0l_read().
*/
DB_STATUS
dma_offline_context(
ACB	    *acb)
{
    ACB			*a = acb;
    DMP_LCTX		*lctx = acb->acb_lctx;
    ACP_QUEUE		*st;
    DBCB		*dbcb;
    i4		log_recs_read = 0;
    i4		db_count;
    i4		dbid;
    LG_LA		zero_la;
    LG_LA		la;
    LG_LA		start_rec_la;
    DM0L_HEADER		*record;
    DM0L_BCP		*bcp;
    DM0L_OPENDB		*odb;
    LG_LA		min_address;
    LG_LA		max_address;
    i4		i;
    DB_STATUS		status = E_DB_OK;
    i4			error;

    zero_la.la_sequence = 0;
    zero_la.la_block    = 0;
    zero_la.la_offset   = 0;

    for (;;)
    {
	TRdisplay("%@ ACP DMA_OFFLINE_CONTEXT: Building Archiver context.\n");

	/*
	** Find the Current BOF, this is the earliest point from which we 
	** may have to begin archiving.  We may find later (upon examination
	** of journaled db's) that we can start archiving at a much later
	** log address.
	*/
	STRUCT_ASSIGN_MACRO(lctx->lctx_lg_header.lgh_begin, a->acb_bof_addr);
	STRUCT_ASSIGN_MACRO(lctx->lctx_lg_header.lgh_cp, a->acb_cp_addr);
	STRUCT_ASSIGN_MACRO(lctx->lctx_lg_header.lgh_end, a->acb_eof_addr);

	TRdisplay("\tLog File BOF (%d,%d,%d) .. EOF (%d,%d,%d)\n",
	    a->acb_bof_addr.la_sequence,
	    a->acb_bof_addr.la_block,
	    a->acb_bof_addr.la_offset,
	    a->acb_eof_addr.la_sequence,
	    a->acb_eof_addr.la_block,
	    a->acb_eof_addr.la_offset);
	TRdisplay("\tLog File  CP (%d,%d,%d)\n",
	    a->acb_cp_addr.la_sequence,
	    a->acb_cp_addr.la_block,
	    a->acb_cp_addr.la_offset);

	STRUCT_ASSIGN_MACRO(a->acb_bof_addr, a->acb_start_addr);
	STRUCT_ASSIGN_MACRO(a->acb_eof_addr, a->acb_stop_addr);

	TRdisplay("\tArchive Window (%d,%d,%d)..(%d,%d,%d)\n",
	    a->acb_start_addr.la_sequence,
	    a->acb_start_addr.la_block,
	    a->acb_start_addr.la_offset,
	    a->acb_stop_addr.la_sequence,
	    a->acb_stop_addr.la_block,
	    a->acb_stop_addr.la_offset);

	/*	
	** Position to the beginning of the BCP. 
	*/
	status = dm0l_position(DM0L_FLGA, a->acb_s_lxid,
				&a->acb_start_addr, a->acb_lctx, &a->acb_dberr);
	if (status != E_DB_OK)
	{
	    /*
	    ** Fill in exit error information for archiver exit handler.
	    */
	    a->acb_error_code = E_DM9855_ACP_LG_ACCESS_EXIT;
	    uleFormat(NULL, a->acb_error_code, (CL_ERR_DESC *)NULL, ULE_LOOKUP,
		(DB_SQLSTATE *)NULL, a->acb_errtext, sizeof(a->acb_errtext),
		&a->acb_errltext, &error, 0);

	    /* break instead of return? Need new messages, anyhow */

	    break;
	}

	STRUCT_ASSIGN_MACRO(a->acb_start_addr, la); /* "prime the pump" */

	/*
	** Now scan to EOF looking for any OPENDB or CP records from which we
	** can extract information about databases that are journaled. These
	** may require processing by the archiver.
	**
	** A few notes about the following code:
	**
	**   - We have to check for an existing dbcb before
	**     allocating a new one when a db is found referenced in a
	**     CP or OPENDB record.  It would be bad to have multiple dbcbs
	**     for the same database.  Nor can we overwrite an already
	**     allocated dbcb with new jnl addresses.
	**
	**   - The log addresses being passed to the dma_alloc_dbcb
	**     routine are interesting.  The dump addresses should use the
	**     zero_la (which is a zero log address) since no dump processing
	**     will be done in offline archiving, but the journal start and
	**     end addresses need to use a real log address.
	**
	**     In the OPENDB case, the log address of the opendb log record
	**     should be passed.
	**
	**     In the BCP case, the log address of the BCP should be used.
        **
	**   - We err on the side of safety with the database end journal
	**     window values.  In online_context the LGshows for the databases
	**     returns the last journal record written for each database.
	**     In this routine, the last journal record address is set to
	**     an address which is probably greater than the last jnl rec.
	**
	**     We could keep track of journal records for each db as
	**     we do this scan (when a jnl rec is found, look up its dbcb and
	**     set the current record la into the dbcb_last_jnl_la field).
	**
	**     Instead, we set each db's last_jnl_la field to be the current EOF
	**     (by passing the eof address to the dma_alloc_dbcb call).
	**     The latter is easier while the former may make it possible for
	**     us to pick an archive_window_end value below that prevents 
	**     scanning to the EOF in the archive phase.
	**     
	*/

	for (;;)
	{
	    /*  Read next record from the log file. */

	    start_rec_la = la;
	    status = dm0l_read(a->acb_lctx, a->acb_s_lxid,
				&record, &la, (i4*)NULL, &a->acb_dberr);
	    if (status != E_DB_OK)
	    {
		if (a->acb_dberr.err_code == E_DM9238_LOG_READ_EOF)
		{
		    TRdisplay("%@ ACP OFF CXT: Hit EOF at <%d,%d,%d>.\n",
			la.la_sequence, la.la_block, la.la_offset);
		    status = E_DB_OK;
		    CLRDBERR(&a->acb_dberr);
		    break;
		}

		/*
		** Fill in exit error information for archiver exit handler.
		*/
		a->acb_error_code = E_DM9855_ACP_LG_ACCESS_EXIT;
		uleFormat(NULL, a->acb_error_code, (CL_ERR_DESC *)NULL, ULE_LOOKUP,
		    (DB_SQLSTATE *)NULL, a->acb_errtext, sizeof(a->acb_errtext),
		    &a->acb_errltext, &error, 0);

		break;
	    }

	    log_recs_read++;
	    if ((log_recs_read % 1000) == 0)
		TRdisplay("\t%d Log Records processed.\n", log_recs_read);

	    /* 
	    ** Check for OPEND or Consistency Point records.
	    **
	    ** OPENDB records are used in offline archive cycles to build
	    ** datbase context.
	    */
	    if (record->type != DM0LBCP && record->type != DM0LOPENDB)
	    {
		continue;
	    }

	    /*
	    ** If this is the start of a new CP,
	    ** update our database information.
	    */
	    if (record->type == DM0LBCP)
	    {
		/*
		** Address of the start of the log record saved above.
		*/
		la = start_rec_la;
		if (a->acb_verbose)
		{
		    TRdisplay("%@ ACP OFF CXT: found BCP at <%d,%d,%d>\n",
			la.la_sequence, la.la_block, la.la_offset);
		}
		bcp = (DM0L_BCP *) record;
		if (bcp->bcp_type == CP_DB)
		{
		    for (i = 0; i < bcp->bcp_count; i++)
		    {
			dbid = bcp->bcp_subtype.type_db[i].db_ext_dbid;

			/*
			** Skip non-database entries used for LG bookkeeping.
			*/
			if (bcp->bcp_subtype.type_db[i].db_status & DB_NOTDB)
			    continue;

			if (bcp->bcp_subtype.type_db[i].db_status & DB_JOURNAL)
			{
			    /*
			    ** Lookup the id in the list of dbcb's handled in
			    ** this archive cycle.
			    */
			    lookup_dbcb(a, dbid, &dbcb);

			    if (dbcb == 0)
			    {
				if (a->acb_verbose)
				{
				    TRdisplay("\t    %~t ID: %x\n",
					sizeof (DB_DB_NAME),
					&bcp->bcp_subtype.type_db[i].db_name,
					dbid); 
				}
				status = dma_alloc_dbcb(a, dbid, 0,
					&bcp->bcp_subtype.type_db[i].db_name,
					(DB_OWN_NAME*)
					&bcp->bcp_subtype.type_db[i].db_owner,
					(DM_PATH *)
					&bcp->bcp_subtype.type_db[i].db_root,
					bcp->bcp_subtype.type_db[i].db_l_root,
					&start_rec_la, &a->acb_stop_addr,
					&zero_la, &zero_la, &zero_la, &dbcb);

				if (status)
				{
				    /* do something. */
				    break;
				}

				dbcb->dbcb_status |= DBCB_JNL;
			    }
			}
		    }
		    if (status)
			break;
		}
	    }

	    if (record->type == DM0LOPENDB)
	    {
		odb = (DM0L_OPENDB *)record;

		if (odb->o_isjournaled)
		{
		    TRdisplay("%@ ACP OFF CXT: found OPENDB at <%d,%d,%d>\n",
			la.la_sequence, la.la_block, la.la_offset);

		    /*
		    ** Lookup the id in the list of dbcb's handled in this
		    ** archive cycle.
		    */
		    lookup_dbcb(a, record->database_id, &dbcb);

		    if (dbcb == 0)
		    {
			/*
			** Build a DBCB for this database.
			*/
			status = dma_alloc_dbcb(a, record->database_id,
			    0, &odb->o_dbname, 
			    &odb->o_dbowner, &odb->o_root, odb->o_l_root,
			    &la, &a->acb_stop_addr,
			    &zero_la, &zero_la, &zero_la,
			    &dbcb);
			if (status != E_DB_OK)
			{
			    /* log errors ... break? */
			    break;
			}

			dbcb->dbcb_status |= DBCB_JNL;
		    }
		}
	    }
	}

	if (status)
	    break;

	TRdisplay("%@ ACP OFF CXT: Read %d records\n",
		    log_recs_read);

	/*
	** Determine the Archive Start and Stop addresses.
	**
	** Cycle through the database contexts and find the smallest valued
	** non-zero log address from which journal processing must be done.
	** and use that as the Archiver Start address.  Use the highest
	** Use the highest valued end log address for the Archver Stop address.
	**
	** A zero log address indicates there is no work to do for that db.
	*/
	db_count = 0;
	STRUCT_ASSIGN_MACRO(zero_la, min_address);
	STRUCT_ASSIGN_MACRO(zero_la, max_address);

	for (st = acb->acb_db_state.stq_next;
	     st != (ACP_QUEUE *)&acb->acb_db_state.stq_next;
	     st = dbcb->dbcb_state.stq_next)
	{
	    dbcb = (DBCB *)((char *)st - CL_OFFSETOF(DBCB, dbcb_state));

	    db_count++;

	    if ((min_address.la_sequence == 0) ||
		((dbcb->dbcb_first_jnl_la.la_sequence != 0) &&
		 (LGA_LT(&dbcb->dbcb_first_jnl_la, &min_address))))
	    {
		STRUCT_ASSIGN_MACRO(dbcb->dbcb_first_jnl_la, min_address);
	    }

	    if (LGA_GT(&dbcb->dbcb_last_jnl_la, &max_address))
		STRUCT_ASSIGN_MACRO(dbcb->dbcb_last_jnl_la, max_address);
	}

	/*
	** Consistency Check: If db's found then min/max should make sense.
	**
	**     The min_address should not have been moved back before the BOF.
	**     The max_address should be non-zero.
	**     The max_address should not be less than the min_address.
	*/
	if (db_count != 0)
	{
	    if ((LGA_LT(&min_address, &a->acb_bof_addr)) ||
		(max_address.la_sequence == 0) ||
		(LGA_GT(&min_address, &max_address)))
	    {
		TRdisplay("ACP Window Inconsistency: archive full log file.\n");
		STRUCT_ASSIGN_MACRO(a->acb_bof_addr, a->acb_start_addr);
		STRUCT_ASSIGN_MACRO(a->acb_eof_addr, a->acb_stop_addr);
	    }
	}

	/*
	** Update the archive window addresses to the minimum and maximum
	** log address which we found that journal processing was needed.
	*/
	STRUCT_ASSIGN_MACRO(min_address, a->acb_start_addr);
	STRUCT_ASSIGN_MACRO(max_address, a->acb_stop_addr);

	TRdisplay("\tArchive Window (%d,%d,%d)..(%d,%d,%d)\n",
	    a->acb_start_addr.la_sequence,
	    a->acb_start_addr.la_block,
	    a->acb_start_addr.la_offset,
	    a->acb_stop_addr.la_sequence,
	    a->acb_stop_addr.la_block,
	    a->acb_stop_addr.la_offset);

	/*
	** Initialize dynamic last record and CP log addresses.
	** These will be set by the main archive loop and at the end
	** of the archive cycle will be the log addresses of the last
	** log record processed and last ECP record seen, respectively.
	*/
	a->acb_last_la.la_sequence = 0;   
	a->acb_last_la.la_block    = 0;   
	a->acb_last_la.la_offset   = 0;   
	a->acb_last_cp.la_sequence = 0;   
	a->acb_last_cp.la_block    = 0;   
	a->acb_last_cp.la_offset   = 0;   

	TRdisplay("%@ ACP DMA_OFFLINE_CONTEXT: Archiver context complete.\n");
	TRdisplay("\t%d Databases to process.\n", db_count);

	return (E_DB_OK);
    }

    /*
    ** Error handling.
    */

    uleFormat(&a->acb_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
	(char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
    SETDBERR(&a->acb_dberr, 0, E_DM9842_ARCH_SOC);
    return (E_DB_ERROR);
}

/*{
** Name: dma_archive	- The main loop of the archiver.
**
** Description:
**      This routine drives the main algorithm of the archiver.
**
**	It reads the log file between the acb_start_addr and acb_stop_addr
**	processing journal and dump records as it goes.
**
**	The following types of log records trigger some sort of processing
**	here by the archiver:
**
**	    JOURNAL records: 
**		Journal records are copied to their respective database's 
**		current journal file as long as journaling is still enabled
**		on the database and the log record falls after the database
**		previous journaled address (dbcb_prev_jnl_la).
**
**	    DUMP records: 
**		Dump records are copied to their respective database's
**		current dump file to provide for online backup processing.
**		This is done only during when the database is actively
**		undergoing online backup and when the log record falls inside
**		the valid SBACKUP and EBACKUP window and after the database
**		previous dump address (dbcb_prev_dmp_la).
**
**	    DM0LECP records:
**		The archiver tracks Consistency Points while it scans the log
**		file so that it can report to the Logging System the latest
**		CP to which the Log File is archived.  The Logging System uses
**		this information to determine where to move the Log File BOF.
**
**	    DM0LSBACKUP, DM0LEBACKUP records:
**		Used to track database online backup states.
**
**	    TEST records:
**		Journal records marked DM0LTEST are used to test various
**		journal error conditions and trigger a acp_test_record call.
**
**
**	At the start of the archive cycle, the following fields should be
**	initialized in the ACB:
**
**	    a->acb_bof_addr	: Current Log File BOF.
**	    a->acb_eof_addr	: Current Log File EOF.
**	    a->acb_cp_addr	: Most recent logging system Consistency Point.
**	    a->acb_start_addr	: Address at which to start archiving.
**	    a->acb_stop_addr	: Address to which to archive.
**
**	At the end of the archive cycle, the following fields will be set
**	in the ACB:
**
**	    a->acb_last_cp 	: Initially zero, will be set to the log
**				  address of the last valid CP encountered
**				  in the log file scan.
**	    a->acb_last_la	: Initially zero, will be set to the log
**				  address of the last log record processed.
**
**	Hitting the log file EOF will terminate the archive scan normally
**	and will not be treated as an error.
**
** Inputs:
**	acb				Archiver context
**
** Outputs:
**      err_code                        Reason for error status.
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	none
**
** History:
**      24-sep-1986 (Derek)
**          Created.
**	21-jan-1989 (EdHsu)
**	    Add code to support online backup for Terminator
**	 8-jan-1990 (rogerk)
**	    Don't do backup work in CSP.
**	28-dec-1992 (jnash & rogerk)
**	    Rewritten for the reduced logging project.  Merged bits of old
**	    dma_p1 and dma_p2.
**	27-jan-1993 (jnash)
**	    Online backup fixes.  Reposition the log file to the start of
**	    BCP record if one is found during copy phase.  This allows
**	    the calling routine to handle this CP properly.
**	15-mar-1993 (rogerk & jnash)
**	    Reduced Logging - Phase IV:
**	    Use external database id in places where internal id previously 
**		used.  Remove checks for "bad database id" which now make
**		no sense.  
**	    Changed to now extract the database_id, transaction_id and
**		LSN from the record header (dm0l_header) rather than from
**		the LGrecord information in the LCTX.
**	    Took out code that used to copy the database and transaction ids to
**		the record header when copying records to the journal files.
**		These fields are now set correctly at the time the records
**		written to the log file.
**	    Removed special tran_id, db_id fields from ET log records.
**	    Removed special db_id field from SBACKUP and EBACKUP records.
**	    Fixed disable_journal support. Skip records on journal-disabled dbs.
**	    Changed to use DM0L_DUMP flag as a way of recognizing records which
**		should be copied to the dump files.
**	    Removed assumptions that ECP,BCP,OPENDB,CLOSEDB are journaled.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**		The CSP process, when performing archiving, does not stop
**		    at 4 consistency points; it continues until it reaches
**		    the end of the log file.
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**		Pass correct transaction handle to dm0l_read.
**		Respond to lint suggestions by deleting unused variables.
**	24-may-1993 (andys)
**	    Pass appropriate logfile blocksize to dmd_log and dmd_format_log
**	    rather than using hardwired 4096.
**	21-jun-1993 (rogerk)
**	    Add BCP record type to the list of records that the archiver
**	    looks at even though they are not journaled records.  BCP's
**	    must be handled so we can track the CP address to which to
**	    inform the logging system we have archived records.
**	26-jul-1993 (rogerk)
**	  - Changed the way journal and archive windows are handled.  Took
**	    out archivers fascination with CP's.  We no longer stop every
**	    Consistency Point to determine if we are at a valid CP.
**	  - Merged dma_archive and dma_copy into this one routine.
**	  - We now note CP addresses each time we see an ECP record, keeping
**	    track of the address of the last valid CP we encounter.
**	  - We now process until we get to the acp_stop_addr rather than after
**	    four consistency points.  The stop address will be calculated
**	    in the init phase and will often be previous to the EOF.
**	  - We now keep track of the last log record processed (acp_last_la)
**	    and use this value to record the address to which we've archived.
**	  - Moved checks around so that DUMP processing is not bypassed on
**	    journal disabled databases.
**	12-may-1995 (thaju02)   
**	    Bug #68644 - rollforwarddb was applying journal records
**	    for non-journaled tables.  Corrected bug such that
**	    the log record is copied to the journal file only
**	    when the database is journaled and the table in 
**	    which the log record pertains to is a journaled table.
**      06-jun-1995 (thaju02)
**	    Back out of change made for bug#68644, causes possible 
**	    corruption of system catalogs. 
**      06-mar-1996 (stial01)
**          Variable Page Size project: length field in log records is now 
**          i4, so change call to dm0j_write()
**	16-jul-2004 (thaju02)
**	    Online Modify: during sort/load from btree, disassociated
**	    data page may be freed; no entry in rnl lsn array for page.
** 	    If sidefile processing is unable to locate record (position_to_row
**	    fails with E_DM0055, we need to verify that page was deallocated.
**	    Write dm0l_dealloc log records to sidefile. (B112658)
**	04-Feb-2010 (jonj)
**	    SIR 121619 MVCC: Add bufid parameter to dm0l_read().
[@history_template@]...
*/
DB_STATUS
dma_archive(
ACB	    *acb)
{
    ACB			*a = acb;
    DBCB		*db;
    LG_RECORD		*header;
    LG_LA		la;
    LG_LA		zero_la;
    DM0L_HEADER		*record;
    DM0L_OPENDB    	*odb;
    DB_STATUS		status;
    i4			db_id;
    i4			error;
    i4			log_recs_read;
    i4			log_recs_written;
    i4			backup_status;
    char		jnlerr_db[DB_DB_MAXNAME+1];
    char		jnlerr_dir[DB_AREA_MAX+1];
    SIDEFILE		*sf = 0;

    CLRDBERR(&a->acb_dberr);

    /*
    ** Initialize the value which shows the point to which we have successfully
    ** archived.  Even if we find no records to read, we can still indicate
    ** to the Logging System that we have prcessed all required records up
    ** to this point.
    */
    STRUCT_ASSIGN_MACRO(a->acb_start_addr, a->acb_last_la);

    /*
    ** If there is no archive window, then there is no work to do.
    */
    if (a->acb_start_addr.la_sequence == 0)
    {
	TRdisplay("%@ ACP DMA_ARCHIVE: No archive processing needed.\n");
	return (E_DB_OK);
    }


    TRdisplay("%@ ACP DMA_ARCHIVE: Copy log records to Journal files.\n");

    /*	
    ** Position to the start of the archive window.
    */
    status = dm0l_position(DM0L_FLGA, a->acb_s_lxid,
			    &a->acb_start_addr, a->acb_lctx, &a->acb_dberr);
    if (status != E_DB_OK)
    {
	/*
	** Fill in exit error information for archiver exit handler.
	*/
	a->acb_error_code = E_DM9855_ACP_LG_ACCESS_EXIT;
	uleFormat(NULL, a->acb_error_code, (CL_ERR_DESC *)NULL, ULE_LOOKUP,
	    (DB_SQLSTATE *)NULL, a->acb_errtext, sizeof(a->acb_errtext),
	    &a->acb_errltext, &error, 0);

	return (error_translate(E_DM9843_ARCH_COPY, &a->acb_dberr));
    }

    /*
    ** Main loop of copy phase. 
    **
    ** Read through the Log File until we reach the EOF or the specified 
    ** stopping address.  ACB information of interest:
    **
    **     a->acb_bof_addr	: Current Log File BOF.
    **     a->acb_eof_addr	: Current Log File EOF.
    **     a->acb_cp_addr	: Most recent logging system Consistency Point.
    **
    **     a->acb_start_addr	: Address at which to start archiving.
    **     a->acb_stop_addr	: Address to which to archive.
    **     a->acb_last_cp 	: Initially zero, will be set to the log
    **				  address of the last valid CP encountered
    **				  in the log file scan.
    **     a->acb_last_la	: Initially set to the archiver start address,
    **				  will be set to the address of the last log
    **				  record processed.
    */
    log_recs_read = 0;
    log_recs_written = 0;
    STRUCT_ASSIGN_MACRO(a->acb_start_addr, la);
    db = (DBCB*)NULL;
    for(;;)
    {
	/*  Read next record from the log file. */

	status = dm0l_read(a->acb_lctx, a->acb_s_lxid, &record, &la, 
				(i4*)NULL, &a->acb_dberr);
	if (status != E_DB_OK)
	{
	    if (a->acb_dberr.err_code == E_DM9238_LOG_READ_EOF)
	    {
		TRdisplay("%@ ACP COPY: Hit end of Log File at <%d,%d,%d>.\n",
		    la.la_sequence, la.la_block, la.la_offset);
		status = E_DB_OK;
		CLRDBERR(&a->acb_dberr);
		break;
	    }

	    /*
	    ** Fill in exit error information for archiver exit handler.
	    */
	    a->acb_error_code = E_DM9855_ACP_LG_ACCESS_EXIT;
	    uleFormat(NULL, a->acb_error_code, (CL_ERR_DESC *)NULL, ULE_LOOKUP,
		(DB_SQLSTATE *)NULL, a->acb_errtext, sizeof(a->acb_errtext),
		&a->acb_errltext, &error, 0);
	    break;
	}

	/*
	** Check if we have reached the requested log address.
        */
	if (LGA_GT(&la, &a->acb_stop_addr))
        {
	    if (a->acb_verbose)
	    {
		TRdisplay("%@ ACP COPY: Hit Stop Address at: <%d,%d,%d>.\n",
		    la.la_sequence, la.la_block, la.la_offset);
	    }

	    break;
        }

	log_recs_read++;
	if ((log_recs_read % 1000) == 0)
	    TRdisplay("\t%d Log Records processed.\n", log_recs_read);

	STRUCT_ASSIGN_MACRO(la, a->acb_last_la);
	header = (LG_RECORD*) (((DMP_LCTX*)a->acb_lctx)->lctx_header);

	/* 
	** Check for journaled or dump records.
	**
	** Process ECP, SBACKUP, EBACKUP, and OPENDB records even though 
	** they are not journaled - these records require special archiver 
	** processing.
	**
	** In the current archiver design, SBACKUP records cause us to 
	** stop writing to the current journal and switch to new one.  
	** This could perhaps be better handled in dmfcpp...
	**
	** EBACKUP logs tells us that this online backup cycle is
	** complete, and define the dump processing end log address.
	**
	** ECP records are used to track Consistency Points in the log file.
	** At the end of the archive cycle we need to inform the logging
	** system of the highest valued CP address to which we have archived.
	*/
	if (((record->flags & (DM0L_JOURNAL | DM0L_DUMP)) == 0) && 
	    (record->type != DM0LSBACKUP) && (record->type != DM0LEBACKUP) &&
	    (record->type != DM0LOPENDB) && (record->type != DM0LECP))
	{
	    continue;
	}

	/*
	** During CLUSTER recovery archiving, Online Backups are ignored.
	** During RCP recovery archiving, Online Backups are ignored.
	*/
	if ((a->acb_status & (ACB_CSP | ACB_RCP)) && 
	    ((record->type == DM0LSBACKUP) || (record->type == DM0LEBACKUP)))
	{
	    continue;
	}

	/*
	** OPENDB log records are ignored UNLESS we are doing cluster recovery
	** archiving.  Otherwise the database context is extracted from the
	** Logging System at the start of the archive cycle.
	*/
	if ((record->type == DM0LOPENDB) && ( ! (a->acb_status & ACB_CSP)))
	    continue;

	/*
 	** Track consistency point records as we archive.  At the end of
 	** the archive cycle we need to report the last valid CP that we
 	** have encountered.
 	**
 	** Since we must only report those BCP's which are VALID, we actually
 	** don't record the CP address until we see an ECP record.
 	*/
 	if (record->type == DM0LECP)
 	{
	    DM0L_ECP	*ecp = (DM0L_ECP *)record;

 	    if (LGA_GT(&ecp->ecp_begin_la, &a->acb_bof_addr))
 		STRUCT_ASSIGN_MACRO(ecp->ecp_begin_la, a->acb_last_cp);

	    continue;
	}

	/*
	** Check if we are simulating a bad record.
	*/
	if (a->acb_test_flag & ACB_T_BADREC)
	{
	    TRdisplay("%@ ACP COPY: Simulating Journal Protocol Error\n");

	    /*
	    ** Fill in exit error information for archiver exit handler.
	    */
	    a->acb_error_code = E_DM985A_ACP_JNL_PROTOCOL_EXIT;
	    uleFormat(NULL, a->acb_error_code, (CL_ERR_DESC *)NULL, ULE_LOOKUP,
		(DB_SQLSTATE *)NULL, a->acb_errtext, sizeof(a->acb_errtext),
		&a->acb_errltext, &error, 0);
	    SETDBERR(&a->acb_dberr, 0, E_DM9840_ARCH_BAD_DBID);
	    break;
	}

	/*
	** Lookup the id in the list of dbcb's handled in this archive cycle.
	*/
	db_id = record->database_id;
	lookup_dbcb(a, db_id, &db);

	/*
	** If normal ACP archiving and the database is not known, then we are
	** not archiving it and the record can be ignored.
	*/
	if (db == 0)
	{
	    if ((a->acb_status & ACB_CSP) == 0)
		continue;

	    /*
	    ** If archiving from the CSP, then add the unknown database to the
	    ** archive context.  The record must be an OPENDB since the db
	    ** was not listed in the BCP (and an opendb must preceed all other
	    ** log records).
	    **
	    ** 26-apr-1993 (bryanp)
	    **	I've left this code in because it compiles. However, I don't
	    **	think it works and I don't know what we should do with it. The
	    **	current CSP never encounters this situation because
	    **	dma_offline_context pre-computes the list of all databases
	    **	we'll encounter.
	    */

	    if (record->type != DM0LOPENDB)
	    {
		/*
		** We assume that the first log record we find for a
		** database is an OPENDB log record.  If this is not
		** the case something is wrong.
		** XXXX new error XXX 
		*/
		TRdisplay(
	"%@ CSP DMA_COPY: Internal error, DBID %d not previously cataloged\n",
		    db_id);

		TRdisplay("  (LA=(%d,%d,%d),LEN=%d,XID=%x%x,ID=%x\n",
		    la.la_sequence, la.la_block, la.la_offset,
		    header->lgr_length, record->tran_id.db_high_tran, 
		    record->tran_id.db_low_tran, record->database_id);
		dmd_log(FALSE, (PTR)record, a->acb_lctx->lctx_bksz);
		    
		/*
		** Fill in exit error information for archiver exit handler.
		*/
		a->acb_error_code = E_DM985A_ACP_JNL_PROTOCOL_EXIT;
		uleFormat(NULL, a->acb_error_code, (CL_ERR_DESC *)NULL, ULE_LOOKUP,
		    (DB_SQLSTATE *)NULL, a->acb_errtext, 
		    sizeof(a->acb_errtext), &a->acb_errltext, 
		    &error, 0);
		SETDBERR(&a->acb_dberr, 0, E_DM9840_ARCH_BAD_DBID);
		break;
	    }

	    /*
	    ** Build a DBCB for this database.
	    */
	    odb = (DM0L_OPENDB *)record;
	    zero_la.la_sequence = 0;
	    zero_la.la_block    = 0;
	    zero_la.la_offset   = 0;

	    status = dma_alloc_dbcb(a, db_id, 0, &odb->o_dbname, 
		&odb->o_dbowner, &odb->o_root, odb->o_l_root,
		&zero_la, &zero_la, &zero_la, &zero_la, &zero_la, &db);
	    if (status != E_DB_OK)
		break;
	}

	if (record->type == DM0LBSF)
	{
	    status = dma_alloc_sidefile(a, db, (DM0L_BSF*)record);
	    /*
	    if (status != E_DB_OK)
		break;
	    */
	}
	else if (record->type == DM0LESF)
	{
	    status = dma_close_sidefile(a, db, (DM0L_ESF*)record);
	    /*
	    if (status != E_DB_OK)
		break;
	    */
	}
	else if (record->type == DM0LPUT || record->type == DM0LDEL ||
		record->type == DM0LREP || record->type == DM0LDEALLOC)
	{
	    if (a->acb_sfq_count)
	    {
		status = dma_filter_sidefile(a, db, record, &sf);
		if (sf && status == E_DB_OK)
		{
		    status = dma_write_sidefile(a, db, record, sf);

		    if (status != E_DB_OK)
			status = E_DB_OK;
		}
	    }
	}

	/*
	** If executing Online Backup, then check for EBACKUP log records.  
	** These cause us to mark the database as having finished Online Backup.
	**
	** Backup log records were bypassed above in the Cluster Process (CSP).
	** Do not do Online Backup processing if the Cluster Process (CSP).
	*/
	if ((record->type == DM0LEBACKUP) && (db->dbcb_status & DBCB_BACKUP))
	{
	    TRdisplay(
	  	"%@ ACP COPY: End Backup Record for DBID %x at <%d,%d,%d>.\n",
		db_id, la.la_sequence,
		la.la_block,
		la.la_offset);

	    /*
	    ** If dbid does not appear on the bkdb queue or does
	    ** not match the SBACKUP address, ignore.
	    */
	    if (! check_end_backup(a, db_id, &la))
	    {
		TRdisplay("%@ ACP DMA_COPY: CHECK_END_BACKUP is INVALID\n");
		continue;
	    }

	    /*
	    ** Mark EBACKUP found and record its log address so the
	    ** archiver can tell whether records fall within the
	    ** BEGIN/END Backup window.
	    */
	    db->dbcb_ebackup = la; 
	    db->dbcb_backup_complete = TRUE;        
	}

	/*
	** When a Start Backup record is encountered, we need to switch
	** journal files since all journal records following this point
	** should be associated with the new checkpoint, whereas the
	** previous records were associated with the previous checkpoint.
	*/
	if (record->type == DM0LSBACKUP)
	{
	    /*
	    ** If the database is not undergoing online backup, then this is
	    ** either an old backup log record or the backup was cancelled.
	    */
	    if ( ! (db->dbcb_status & DBCB_BACKUP))
	    {
		if (a->acb_verbose)
		{
		    TRdisplay("ACP DMA_COPY: SBACKUP record ignored.\n");
		    TRdisplay(
		      "    Current la: <%d,%d,%d>, SBACKUP la: <%d,%d,%d>\n",
			la.la_sequence,
			la.la_block,
			la.la_offset,
			db->dbcb_sbackup.la_sequence, 
			db->dbcb_sbackup.la_block,
			db->dbcb_sbackup.la_offset);
		}
		continue;
	    }

	    TRdisplay("%@ ACP COPY: Hit SBACKUP Log Record at: <%d,%d,%d>.\n",
		la.la_sequence, la.la_block, la.la_offset);

	    /*
	    ** Non-journaled databases do not require this journal switch work.
	    */
	    if ( ! (db->dbcb_status & DBCB_JNL))
		continue;

	    /*
	    ** Ensure that the SBACKUP record is the correct one - rather
	    ** than a leftover from a failed checkpoint.  
	    */
	    backup_status = check_start_backup(a, db_id, &la);
	    if (backup_status == BKDB_INVALID)
	    {
		/* 
		** Database not found in internal context in logging 
		** system, this is probably a leftover from a prior failure.
		** Log the event and continue.
		*/
		TRdisplay("ACP DMA_COPY: SBACKUP for database %~t ignored.\n",
		    sizeof(db->dbcb_dcb.dcb_name), &db->dbcb_dcb.dcb_name);
		continue;
	    }
	    else if (backup_status == BKDB_EXIST)
	    {
		/*
		** The logging system knows about this database but we do 
		** not have a record of it in the BKDBCB.  This is not good.
		*/
		TRdisplay("ACP DMA_COPY: SBACKUP for database %~t not found internally.\n",
		    sizeof(db->dbcb_dcb.dcb_name), &db->dbcb_dcb.dcb_name);
		status = E_DB_ERROR;
		SETDBERR(&a->acb_dberr, 0, E_DM9843_ARCH_COPY);
		break;
	    }

	    /*
	    ** Switch journal files.
	    */
	    status = next_journal(a, db, header, record);
	    if (status != E_DB_OK)
		break;
	}

	/*
        ** Check for Archiver test log records.
	**
	** The test log records are used to drive test actions such as
	** crashing the archiver or simulating disk-full or other error
	** conditions.  Test log records are not copied to the journal file.
	*/
	if (record->type == DM0LTEST)
	{
	    acp_test_record(a, record, 1);
	    continue;
	}

	/*
	** JOURNAL RECORD PROCESSING.
	**
	** Copy record to journal file if required.  
	*/
	if (record->flags & DM0L_JOURNAL) 
	{
	    /*
	    ** If the database is marked as journal disabled, then skip 
	    ** the log record.
	    */
	    if (db->dbcb_status & DBCB_JNL_DISABLE)
		continue;
		/*b66422 prevent SEGV when erroneously writing to journal
		 *(dbcb_dm0j = NULL).
		 */

	    if ( ! ( db->dbcb_status & DBCB_JNL ))
		continue ;
	    if ( db->dbcb_dm0j == NULL )
		continue ;
	    /*
	    ** If we have already processed this record earlier, skip it.
	    */
	    if (LGA_LTE(&la, &db->dbcb_prev_jnl_la))
		continue;

	    /*  
	    ** Add the record to journal for the database. 
	    */
	    status = dm0j_write((DM0J_CTX *)db->dbcb_dm0j, (PTR)record,
		*(i4 *)record, &a->acb_dberr);

	    /*
	    ** Archiver test point - if archiver test flag specifies to simulate
	    ** a disk full error, then act as though the dm0j_write call failed.
	    */
	    if (a->acb_test_flag & ACB_T_DISKFULL)
	    {
		TRdisplay("%@ ACP COPY: Simulating Disk Full Error\n");
		status = E_DB_ERROR;
		SETDBERR(&a->acb_dberr, 0, E_DM9284_DM0J_WRITE_ERROR);
	    }

	    if (status != E_DB_OK)
	    {
		/*
		** Fill in exit error information for archiver exit handler.
		*/
		a->acb_error_code = E_DM9852_ACP_JNL_WRITE_EXIT;
		STRUCT_ASSIGN_MACRO(db->dbcb_dcb.dcb_name, a->acb_errdb);
		STncpy( jnlerr_db, db->dbcb_dcb.dcb_name.db_db_name,DB_DB_MAXNAME);
		jnlerr_db[ DB_DB_MAXNAME ] = '\0';
		STtrmwhite(jnlerr_db);
		STncpy( jnlerr_dir, db->dbcb_j_path.name, DB_AREA_MAX);
		jnlerr_dir[DB_AREA_MAX] = '\0';
		STtrmwhite(jnlerr_dir);

		uleFormat(NULL, a->acb_error_code, (CL_ERR_DESC *)NULL, ULE_LOOKUP,
		    (DB_SQLSTATE *)NULL, a->acb_errtext, sizeof(a->acb_errtext),
		    &a->acb_errltext, &error, 3, 
		    0, jnlerr_db, 0, jnlerr_dir, 0, ERx("write to"));
		break;
	    }

	    log_recs_written++;
	    db->dbcb_jrec_cnt++;
	    db->dbcb_jnl_la = la;
	}

	/* 
	** BACKUP RECORD PROCESSING
	**
	** Copy dump records to the dump file if necessary.
	*/
	if ((record->flags & DM0L_DUMP) && (db->dbcb_status & DBCB_BACKUP)) 
	{
	    /*
	    ** The record has to fall after between the SBACKUP and EBACKUP
	    ** log records and after the previous dumped address.
	    ** (We use dbcb_backup_complete to qualify with the EBACKUP)
	    */
	    if ((LGA_GT(&la, &db->dbcb_sbackup)) &&
		(LGA_GT(&la, &db->dbcb_prev_dmp_la)) &&
		(! db->dbcb_backup_complete ) )    
	    {
		/*
		** Copy log record to the dump file.
		*/
		status = copy_record_to_dump(a, db, &la, header, record);
		if (status != E_DB_OK)
		    break;
	    }
	}
    }

    if (status != E_DB_OK)
    {
	acb->acb_status |= ACB_BAD;
	return (error_translate(E_DM9843_ARCH_COPY, &a->acb_dberr));
    }

    TRdisplay("%@ ACP ARCHIVE: Read %d records, wrote %d records.\n",
		log_recs_read, log_recs_written);

    return (E_DB_OK);
}

/*{
** Name: dma_soc  - Perform archive start of cycle processing.
**
** Called at the beginning of an archive cycle, this routine uses 
** previously established dbcb context to open the config files, journal 
** and dump files (if necessary).  
**			   
** Description:
**      This routine reads config file information and prepares for the copy 
**	phase of archive processing.  This includes:
**	 - Build online backup context.
**	 - For each database 
**	     - determine from config file information where the last
**	       archive cycle left off.
**	     - determine journal disabled status. 
**	     - Copy required info from config file to dbcb or bkdbcb.
**
**	Opening/creating journal and dump files is deferred until the
**	first write to those files.  DBCB_JNL_FILE and DBCB_DUMP_FILE 
**	status bits reflect if files are open, these fields being
**	initialized to zero when CBs are created.
**
** Inputs:
**	acb				ACB 
**
** Outputs:
**      err_code                        Reason for error status.
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
**      22-dec-1992 (jnash)
**          Created for the reduced logging project.
**      22-feb-1993 (jnash)
**          Search dbcb hash queue.
**	15-mar-1993 (rogerk)
**	    Reduced Logging - Phase IV:
**	    Fixed use of dump file sequence and block sequence numbers
**		(dbcb_d_filseq/dbcb_d_blkseq) that were being mixed up.
**	    Fixed handling of journal-disabled dbs.  Turn off journal status
**		in dbcb to cause journal file cleanup to be skipped.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**		Respond to lint suggestions by deleting unused variables.
**	26-jul-1993 (rogerk)
**	  - Changed the way journal and dump windows are handled.  They are
**	    now tracked by real Log Addresses rather than the Consistency
**	    points that bound them.  Consolidated the system journal and
**	    dump windows into a single archive window.
**	  - Changed trace messages written by the routine.
**	  - Removed initializing of the acb_last_cp value here.  It is now
**	    set to zero when the context is built.
**	  - Use the acb_eof_addr value rather than acb_stop_addr when checking
**	    that log addresses are legal.
**	  - Set previous journal address info on journal disabled databases
**	    up to the stop address to avoid when possible even having to
**	    read the section of the log file which contains database records.
**	  - Use new acb_max_cycle to pre-determine the stopping address if
**	    we are to stop previous to the end of the archive window.
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Don't carve up MVCC archives to keep JFIB
**	    in sync with the actual journals.
[@history_template@]...
*/
DB_STATUS
dma_soc(
ACB	    *acb)
{
    ACB		    *a = acb;
    ACP_QUEUE	    *st;
    DBCB	    *db; 
    DM0C_EXT        *ext;
    DM0C_CNF	    *config;
    DB_STATUS	    status;
    LG_LA	    min_address;
    u_i4	    log_blocks;
    u_i4	    interval;
    i4	    i;
    i4	    error;

    CLRDBERR(&a->acb_dberr);

    TRdisplay("%@ ACP START CYCLE: Check database config information.\n");

    /*
    ** Update dbcb with config file info for each database.
    */
    for (st = acb->acb_db_state.stq_next;
	 st != (ACP_QUEUE *)&acb->acb_db_state.stq_next;
	 st = db->dbcb_state.stq_next)
    {
	db = (DBCB *)((char *)st - CL_OFFSETOF(DBCB, dbcb_state));

	/*
	** Open the config file for this db, fill in last
	** journaled log address, etc.
	*/
	status = dm0c_open(&db->dbcb_dcb, 0, a->acb_lk_id, &config, &a->acb_dberr);
	if (status != E_DB_OK)
	{
	    /*
	    ** Error opening database config file - database cannot be
	    ** processed.  Fill in exit error information for archiver
	    ** exit handler.
	    */
	    a->acb_error_code = E_DM9856_ACP_DB_ACCESS_EXIT;
	    STRUCT_ASSIGN_MACRO(db->dbcb_dcb.dcb_name, a->acb_errdb);
	    uleFormat(NULL, a->acb_error_code, (CL_ERR_DESC *)NULL, ULE_LOOKUP,
		(DB_SQLSTATE *)NULL, a->acb_errtext, 
		sizeof(a->acb_errtext), &a->acb_errltext, 
		&error, 1, DB_DB_MAXNAME, db->dbcb_dcb.dcb_name.db_db_name);

	    break;
	}

	/* 
	** Record journal-disabled status and continue, but do not 
	** destroy context (if online backup is being done, we still need
	** the database context). 
	**
	** Turn off the DBCB_JNL status to avoid doing any journal processing
	** on the database and set the previous journal address up to the
	** archiver stop address so that the recalculation of the start address
	** below will not be limited by this database's journal window.
	*/
	if (config->cnf_dsc->dsc_status & DSC_DISABLE_JOURNAL)
	{
	    db->dbcb_status |= DBCB_JNL_DISABLE;
	    db->dbcb_status &= ~DBCB_JNL;
	    STRUCT_ASSIGN_MACRO(a->acb_stop_addr, db->dbcb_prev_jnl_la);

	    TRdisplay("\t    Database %~t is journal_disabled and will not\n",
		sizeof(db->dbcb_dcb.dcb_name), &db->dbcb_dcb.dcb_name);
	    TRdisplay("\t    be processed in this archive cycle.\n");

	    uleFormat(NULL, E_DM983D_ARCH_JNL_DISABLE, (CL_ERR_DESC *)NULL,
		ULE_LOG, NULL, (char *)NULL, (i4)0,
		(i4 *)NULL, &error, 1,
		sizeof(db->dbcb_dcb.dcb_name.db_db_name),
		db->dbcb_dcb.dcb_name.db_db_name);

	    status = dm0c_close(config, 0, &a->acb_dberr);
	    if (status != E_DB_OK)
	    {
		/*
		** Fill in exit error info for archiver exit handler.
		*/
		a->acb_error_code = E_DM9856_ACP_DB_ACCESS_EXIT;
		STRUCT_ASSIGN_MACRO(db->dbcb_dcb.dcb_name, 
		    a->acb_errdb);
		uleFormat(NULL, a->acb_error_code, (CL_ERR_DESC *)NULL, 
		    ULE_LOOKUP, (DB_SQLSTATE *)NULL, a->acb_errtext, 
		    sizeof(a->acb_errtext),
		    &a->acb_errltext, &error, 1,
		    DB_DB_MAXNAME, db->dbcb_dcb.dcb_name.db_db_name);

		return (error_translate(E_DM9842_ARCH_SOC, &a->acb_dberr));
	    }
	    continue;
	}

	/*
	** Get journal information out of the config file.
	*/
	db->dbcb_prev_jnl_la = config->cnf_jnl->jnl_la;
	db->dbcb_j_filseq = config->cnf_jnl->jnl_fil_seq;
	db->dbcb_j_blkseq = config->cnf_jnl->jnl_blk_seq;
	if (a->acb_status & ACB_CLUSTER)
	{
	    db->dbcb_prev_jnl_la = 
		config->cnf_jnl->jnl_node_info[a->acb_node_id].cnode_la;
	    db->dbcb_j_filseq = 
		config->cnf_jnl->jnl_node_info[a->acb_node_id].cnode_fil_seq;
	    db->dbcb_j_blkseq = 
		config->cnf_jnl->jnl_node_info[a->acb_node_id].cnode_blk_seq;
	}

	/* Find the journal location in the extent array */
	for (i = 0; i < config->cnf_dsc->dsc_ext_count; i++)
	{
	    ext = &config->cnf_ext[i];
	    if (ext->ext_location.flags & DCB_JOURNAL)
		break;
	}
	db->dbcb_j_path = ext->ext_location.physical;
	db->dbcb_lj_path = ext->ext_location.phys_length;


	TRdisplay("\tDatabase %~t previously archived to (%d,%d,%d)\n",
	    sizeof(db->dbcb_dcb.dcb_name), &db->dbcb_dcb.dcb_name,
	    db->dbcb_prev_jnl_la.la_sequence,
	    db->dbcb_prev_jnl_la.la_block,
	    db->dbcb_prev_jnl_la.la_offset);
	if (a->acb_verbose)
	{
	    TRdisplay("\t    Journal Location %t\n",
		db->dbcb_lj_path, &db->dbcb_j_path);
	    TRdisplay("\t    Journal File Number %d, Block Number %d.\n",
		db->dbcb_j_filseq, db->dbcb_j_blkseq);
	}

	/*
	** db->dbcb_prev_jnl_la has just been read from the config 
	** file and is supposed to contain the highest log address 
	** which has been archived for this database. Since the 
	** validity of this address is CRITICAL to the correct 
	** operation of ACP algorithms, we sanity-check this
	** address by demanding that it be less than or equal 
	** to the current log file EOF -- if it exceeds the 
	** current log file EOF than something is SERIOUSLY wrong 
	** (for example, the log file has been re-initialized
	** with lower log file addresses -- don't laugh, it's 
	** happened before).
	*/
	if (LGA_GT(&db->dbcb_prev_jnl_la, &a->acb_eof_addr))
	{
	    TRdisplay( "%@ DMA_SOC: Database %~t out of range (<%d,%d,%d>).\n",
		sizeof(db->dbcb_dcb.dcb_name), &db->dbcb_dcb.dcb_name,
		db->dbcb_prev_jnl_la.la_sequence,
		db->dbcb_prev_jnl_la.la_block,
		db->dbcb_prev_jnl_la.la_offset);
	    if (config != 0)
		status = dm0c_close(config, 0, &a->acb_dberr);

	    /*
	    ** Fill in exit error information for archiver exit handler.
	    */
	    a->acb_error_code = E_DM9857_ACP_JNL_RECORD_EXIT;
	    STRUCT_ASSIGN_MACRO(db->dbcb_dcb.dcb_name, a->acb_errdb);
	    uleFormat(NULL, a->acb_error_code, (CL_ERR_DESC *)NULL, ULE_LOOKUP,
		(DB_SQLSTATE *)NULL, a->acb_errtext, 
		sizeof(a->acb_errtext), &a->acb_errltext, 
		&error, 1, DB_DB_MAXNAME, db->dbcb_dcb.dcb_name.db_db_name);

	    SETDBERR(&a->acb_dberr, 0, E_DM9838_ARCH_DB_BAD_LA);
	    return (error_translate(E_DM9842_ARCH_SOC, &a->acb_dberr));
	}

	/*
	** If doing online backup, fill in required dbcb info.
	** We do not open the dump file until the first write is detected.
	*/
	if (db->dbcb_status & DBCB_BACKUP)
	{
	    if (a->acb_status & ACB_CLUSTER)
	    {
		db->dbcb_prev_dmp_la = 
		    config->cnf_dmp->dmp_node_info[a->acb_node_id].cnode_la;
		db->dbcb_d_filseq = 
		    config->cnf_dmp->dmp_node_info[a->acb_node_id].cnode_fil_seq;
		db->dbcb_d_blkseq = 
		    config->cnf_dmp->dmp_node_info[a->acb_node_id].cnode_blk_seq;
	    }
	    else
	    {
		db->dbcb_prev_dmp_la = config->cnf_dmp->dmp_la; 
		db->dbcb_d_blkseq = config->cnf_dmp->dmp_blk_seq;
		db->dbcb_d_filseq = config->cnf_dmp->dmp_fil_seq;
	    }

	    if (a->acb_verbose)
	    {
		TRdisplay("\t    Online Backup completed to (%d,%d,%d)\n",
		    db->dbcb_prev_dmp_la.la_sequence,
		    db->dbcb_prev_dmp_la.la_block,
		    db->dbcb_prev_dmp_la.la_offset);
		TRdisplay("\t    Dump File %d, Block Number %d.\n",
		    db->dbcb_d_filseq, db->dbcb_d_blkseq);
	    }
	}

	/* Close the configuation file */
	status = dm0c_close(config, 0, &a->acb_dberr);
	if (status != E_DB_OK)
	{
	    /*
	    ** Fill in exit error information for archiver exit handler.
	    */
	    a->acb_error_code = E_DM9858_ACP_ONLINE_BACKUP_EXIT;
	    STRUCT_ASSIGN_MACRO(db->dbcb_dcb.dcb_name, a->acb_errdb);
	    uleFormat(NULL, a->acb_error_code, (CL_ERR_DESC *)NULL, ULE_LOOKUP,
		(DB_SQLSTATE *)NULL, a->acb_errtext, 
		sizeof(a->acb_errtext), &a->acb_errltext, &error, 1,
		DB_DB_MAXNAME, db->dbcb_dcb.dcb_name.db_db_name);

	    return (error_translate(E_DM9842_ARCH_SOC, &a->acb_dberr));
	}       	

	/*
	** Open and possibly create journal file.
	**
	** We do this in a separate call, with separate open and
	** close of the config file, to accomodate a future development 
	** where we would like to open the journal file upon first reference,
	** and eliminate empty journal files.  When done, corresponding
	** changes must be made to dmfcpp.c and the new_next_entry()
	** routine.
	*/
	if (db->dbcb_status & DBCB_JNL)
	{
	    status = init_journal(a, db);
	    if (status != E_DB_OK)
		return (error_translate(E_DM9842_ARCH_SOC, &a->acb_dberr)); 
	}

    }		/* of all DB entries */

    /*
    ** Cycle through the archive context and find the smallest valued
    ** log address to which journal and dump processing has already been
    ** done.  We may be able to move our starting address up to this point.
    **
    ** We will only be able to adjust the starting address if the system
    ** archive window includes records that have already been archived.
    ** This is normally true only after a system restart when the archive
    ** window may be initialized to span the entire log file.
    **
    ** Note that zero log addresses are important here.  A previous dump
    ** or log address of ZERO indicates that no previous work has been done
    ** on the database an ALL current records must be processed.
    **
    ** Note also that if the archive window is non-zero, but we find no
    ** databases to process (the archive window may be non-zero due to
    ** dump records being written after the ebackup is processed or due
    ** to journal disabled databases) then this check will move the start
    ** address all the way forward to the stop address, leaving very little
    ** work in the dma_archive phase.  We cannot close the archive window
    ** completely because we must LGalter the system at the end of the
    ** archive cycle to notify LG that the system archive window can be moved
    ** forward or closed.
    */
    STRUCT_ASSIGN_MACRO(a->acb_stop_addr, min_address);

    for (st = a->acb_db_state.stq_next;
	 st != (ACP_QUEUE *)&a->acb_db_state.stq_next;
	 st = db->dbcb_state.stq_next)
    {
        db = (DBCB *)((char *)st - CL_OFFSETOF(DBCB, dbcb_state));

	if (LGA_LT(&db->dbcb_prev_jnl_la, &min_address))
	    STRUCT_ASSIGN_MACRO(db->dbcb_prev_jnl_la, min_address);

	if ((db->dbcb_status & DBCB_BACKUP) &&
	    (LGA_LT(&db->dbcb_prev_dmp_la, &min_address)))
	{
	    STRUCT_ASSIGN_MACRO(db->dbcb_prev_dmp_la, min_address);
	}
    }

    /*
    ** Update the archive starting address to the minimum log address
    ** which we found that journal and dump processing was needed if we
    ** found that the starting address can be moved forward.
    */
    if (LGA_GT(&min_address, &a->acb_start_addr))
    {
	STRUCT_ASSIGN_MACRO(min_address, a->acb_start_addr);
	TRdisplay("\tArchiver Starting Address moved to (%d,%d,%d)\n",
	    a->acb_start_addr.la_sequence,
	    a->acb_start_addr.la_block,
	    a->acb_start_addr.la_offset);
	TRdisplay("\t    after checking previously completed archive work.\n");
    }

    /*
    ** If the current archive window spans more than 10% of the log
    ** file then set the stopping address to exactly 10% of the log
    ** file.  The current large window will be completed in sections
    ** so that we can update the Logging System and Config File
    ** information more frequently.
    **
    ** (The 10% is actually configurable using a PM variable and is 
    ** stored in acb_max_cycle_size).
    **
    ** When archiving in the CSP we must process the whole thing in one go.
    ** When archiving in the RCP we must process all log records in one cycle.
    ** When archiving any MVCC databases, we can't carve it up as it
    ** screws up the JFIB journal file addresses.
    */
    if ((a->acb_status & (ACB_CSP | ACB_RCP | ACB_MVCC)) == 0)
    {
	if (a->acb_stop_addr.la_sequence > a->acb_start_addr.la_sequence)
	{
	    interval = a->acb_blk_count - 
		    (a->acb_start_addr.la_block - a->acb_stop_addr.la_block);
	}
	else
	{
	    interval = a->acb_stop_addr.la_block - a->acb_start_addr.la_block;
	}

	if (interval > a->acb_max_cycle_size)
	{
	    /*
	    ** Adjust stopping point using max_cycle_size.  Convert down
	    ** to number of blocks to avoid overflow on calculations.
	    */
	    log_blocks = a->acb_start_addr.la_block;
	    log_blocks += a->acb_max_cycle_size;

	    a->acb_stop_addr.la_sequence = a->acb_start_addr.la_sequence;
	    if (log_blocks > a->acb_blk_count)
	    {
		a->acb_stop_addr.la_sequence += 1;
		log_blocks -= a->acb_blk_count;
	    }
	    a->acb_stop_addr.la_block  = log_blocks;
	    a->acb_stop_addr.la_offset = 0;

	    TRdisplay("\tArchiver Stop Address adjusted to (%d,%d,%d)\n",
		a->acb_stop_addr.la_sequence,
		a->acb_stop_addr.la_block,
		a->acb_stop_addr.la_offset);
	}
    }

    TRdisplay("%@ ACP Initialization Complete:\n");
    return(E_DB_OK);
}

/*{
** Name: dma_eoc - 	Perform archiver end of cycle processing.
**
** Description:
**	This routine is called at the end of an archive cycle to:
**	 - update the journal file sequence numbers, 
**	 - write a journal eof record, 
**	 - update the configuration file, 
**	 - deallocate internal control blocks, 
**	 - notify the logging system of the address to which archiving has
**	   completed (and the address of the last CP record we passed).
**
**	Data structures will be rebuilt if necessary at the beginning of 
**	the next cycle.
**
**	If the database control block for a backup db is marked DBCB_ERROR
**	then an error was detected in online backup processing.  Signal this
**	to the Checkpoint process so it can abort the checkpoint.
**
**	At the end of the archive cycle, the following fields will be set
**	in the ACB:
**
**	    a->acb_last_cp 	: Initially zero, will be set to the log
**				  address of the last valid CP encountered
**				  in the log file scan.
**	    a->acb_last_la	: Initially zero, will be set to the log
**				  address of the last log record processed.
**
**
** Inputs:
**      acb                             The ACB.
**
** Outputs:
**      err_code                        The reason for error return status.
**	Returns:
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	29-dec-1992 (jnash)
**	    Created from old dma_p3() for the reduced logging project.
**	22-feb-1993 (jnash)
**	    Search dbcb hash queue.
**	15-mar-1993 (rogerk)
**	    Reduced Logging - Phase IV:
**	    Renamed LG_A_ARCHIVE signal to LG_A_ARCHIVE_DONE.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**		When the CSP is performing archiving, it does not notify
**		    the logging system about the logfile processing it has
**		    performed. The CSP uses other techniques (in dmfcsp.c)
**		    to record that the archiving has been performed.
**		Correct the parameters for error message DM900B.
**	26-jul-1993 (rogerk)
**	  - Changed the way journal and dump windows are handled.  They are
**	    now tracked by real Log Addresses rather than the Consistency
**	    points that bound them.  Consolidated the system journal and
**	    dump windows into a single archive window.
**	  - Added acb_last_la which tracks the last log record processed
**	    by the archiver in this cycle.
**	  - Changed arguments to LG_A_ARCHIVE_COMPLETE to now pass in the
**	    acb_last_la and acb_last_cp values.
**	  - Changed trace messages written by the routine.
**	15-jan-1999 (nanpr01)
**	    Pass point to a pointer in dm0j_close, dm0d_close routines.
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Tell dm0c_close explicitly when the journal
**	    info is changing (DM0C_UPDATE_JNL).
*/
DB_STATUS
dma_eoc(
ACB	    *acb)
{
    ACB			*a = acb;
    ACP_QUEUE		*st;
    DBCB		*dbcb;
    DM0C_CNF		*config;
    LG_LA		log_address[2];
    i4		jnl_sequence = 0;
    i4		dmp_sequence = 0;
    i4		error;
    char		jnlerr_db[DB_DB_MAXNAME+1];
    char		jnlerr_dir[DB_AREA_MAX+1];
    CL_ERR_DESC		sys_err;
    DB_STATUS		status;
    STATUS		cl_status;
    i4			dm0cFlags;

    CLRDBERR(&a->acb_dberr);
    
    TRdisplay("%@ ACP DMA_EOC: End phase for Journal Processing.\n");
    TRdisplay("\tLog File archived to Log Address: (%d,%d,%d)\n",
	a->acb_last_la.la_sequence,
	a->acb_last_la.la_block,
	a->acb_last_la.la_offset);
    TRdisplay("\tLast Consistency Point Address found: (%d,%d,%d)\n",
	a->acb_last_cp.la_sequence,
	a->acb_last_cp.la_block,
	a->acb_last_cp.la_offset);

    /*
    ** a->acb_last_la is the point at which archiving stopped.
    ** a->acb_last_cp is the last CP previous to the stop address.
    ** 
    ** Each database updated must have its config file updated to show
    ** the spot to which processing was done.
    **
    ** The Logging System must be signaled to indicate that archive
    ** processing is complete up to the acb_last_la address and up to
    ** the acb_last_cp Consistency Point.
    */

    /*
    ** Update journal and config file information for databases processed in
    ** this archive cycle.
    */
    for (st = a->acb_db_state.stq_next;
	 st != (ACP_QUEUE *)&a->acb_db_state.stq_next;
	 st = dbcb->dbcb_state.stq_next)
    {
        dbcb = (DBCB *)((char *)st - CL_OFFSETOF(DBCB, dbcb_state));

	TRdisplay("\tDatabase %~t: %d journal records processed.\n",
	    sizeof(dbcb->dbcb_dcb.dcb_name), &dbcb->dbcb_dcb.dcb_name,
	    dbcb->dbcb_jrec_cnt);
	if (dbcb->dbcb_status & DBCB_BACKUP)
	{
	    TRdisplay("\tDatabase %~t: %d dump records processed.\n",
		sizeof(dbcb->dbcb_dcb.dcb_name), &dbcb->dbcb_dcb.dcb_name,
		dbcb->dbcb_drec_cnt);
	}

	/*  
	** If journal work was done for this database, then update the 
	** journal file, saving new block sequence number for the config file.
	*/
	if (dbcb->dbcb_jrec_cnt > 0)
	{
	    status = dm0j_update((DM0J_CTX *)dbcb->dbcb_dm0j, &jnl_sequence, 
		&a->acb_dberr);
	    if (status != E_DB_OK)
	    {
		/*
		** Fill in exit error information for archiver exit handler.
		*/
		a->acb_error_code = E_DM9854_ACP_JNL_ACCESS_EXIT;
		STRUCT_ASSIGN_MACRO(dbcb->dbcb_dcb.dcb_name, a->acb_errdb);
		STncpy( jnlerr_db, dbcb->dbcb_dcb.dcb_name.db_db_name,
		    DB_DB_MAXNAME);
		jnlerr_db[DB_DB_MAXNAME] = '\0';
		STtrmwhite(jnlerr_db);
		STncpy( jnlerr_dir, dbcb->dbcb_j_path.name, DB_AREA_MAX);
		jnlerr_dir[DB_AREA_MAX] = '\0';
		STtrmwhite(jnlerr_dir);
		uleFormat(NULL, a->acb_error_code, (CL_ERR_DESC *)NULL, ULE_LOOKUP,
		    (DB_SQLSTATE *)NULL, a->acb_errtext, sizeof(a->acb_errtext),
		    &a->acb_errltext, &error, 2, 0, jnlerr_db, 0, jnlerr_dir);
		
		return(error_translate(E_DM9844_ARCH_EOC, &a->acb_dberr));
	    }
	}

	/*
	** Close the current journal file which was opened at the start of
	** the archive cycle (even if no records were copied.
	*/
	if (dbcb->dbcb_status & DBCB_JNL)
	{
	    status = dm0j_close((DM0J_CTX **)&dbcb->dbcb_dm0j, &a->acb_dberr);
	    if (status != E_DB_OK)
	    {
		/*
		** Fill in exit error information for archiver exit handler.
		*/
		a->acb_error_code = E_DM9854_ACP_JNL_ACCESS_EXIT;
		STRUCT_ASSIGN_MACRO(dbcb->dbcb_dcb.dcb_name, a->acb_errdb);
		STncpy( jnlerr_db, dbcb->dbcb_dcb.dcb_name.db_db_name,
		    DB_DB_MAXNAME);
		jnlerr_db[ DB_DB_MAXNAME ] = '\0';
		STtrmwhite(jnlerr_db);
		STncpy( jnlerr_dir, dbcb->dbcb_j_path.name, DB_AREA_MAX);
		jnlerr_dir[ DB_AREA_MAX ] = '\0';
		STtrmwhite(jnlerr_dir);
		uleFormat(NULL, a->acb_error_code, (CL_ERR_DESC *)NULL, ULE_LOOKUP,
		    (DB_SQLSTATE *)NULL, a->acb_errtext, sizeof(a->acb_errtext),
		    &a->acb_errltext, &error, 2, 0, jnlerr_db, 0, jnlerr_dir);

		return (error_translate(E_DM9844_ARCH_EOC, &a->acb_dberr));
	    }
	}

	/*
	** Update and close the current dump file.
	*/
	if (dbcb->dbcb_drec_cnt > 0)
	{
	    status = dm0d_update((DM0D_CTX *)dbcb->dbcb_dm0d, &dmp_sequence, 
		&a->acb_dberr);
	    if (status != E_DB_OK)
	    {
		/*
		** Fill in exit error information for archiver exit handler.
		*/
	   	a->acb_error_code = E_DM9858_ACP_ONLINE_BACKUP_EXIT;
		STRUCT_ASSIGN_MACRO(dbcb->dbcb_dcb.dcb_name, a->acb_errdb);
		uleFormat(NULL, a->acb_error_code, (CL_ERR_DESC *)NULL, ULE_LOOKUP,
		    (DB_SQLSTATE *)NULL, a->acb_errtext, sizeof(a->acb_errtext),
		    &a->acb_errltext, &error, 1,
		    DB_DB_MAXNAME, dbcb->dbcb_dcb.dcb_name.db_db_name);

		return (error_translate(E_DM9844_ARCH_EOC, &a->acb_dberr));
	    }

	    status = dm0d_close((DM0D_CTX **)&dbcb->dbcb_dm0d, &a->acb_dberr);
	    if (status != E_DB_OK)
	    {
		/*
		** Fill in exit error information for archiver exit handler.
		*/
		a->acb_error_code = E_DM9858_ACP_ONLINE_BACKUP_EXIT;
		STRUCT_ASSIGN_MACRO(dbcb->dbcb_dcb.dcb_name, a->acb_errdb);
		uleFormat(NULL, a->acb_error_code, (CL_ERR_DESC *)NULL, ULE_LOOKUP, 
		    (DB_SQLSTATE *)NULL, a->acb_errtext, 
		    sizeof(a->acb_errtext), &a->acb_errltext, &error, 1,
		DB_DB_MAXNAME, dbcb->dbcb_dcb.dcb_name.db_db_name);

		return (error_translate(E_DM9844_ARCH_EOC, &a->acb_dberr));
	    }
	}

	/*
	** Skip config file updates if no journal or dump work was done 
	** for the database.
	*/
	if ((dbcb->dbcb_jrec_cnt <= 0) && 
	    (dbcb->dbcb_drec_cnt <= 0))
	    continue;

	if ((dbcb->dbcb_status & DBCB_JNL) && (dbcb->dbcb_jrec_cnt > 0))
	{
	    TRdisplay("\t    Journaling Complete to (%d,%d,%d)\n",
		a->acb_last_la.la_sequence,
		a->acb_last_la.la_block,
		a->acb_last_la.la_offset);
	    TRdisplay("\t    Old Jnl block sequence %d, New %d\n",
		 dbcb->dbcb_j_blkseq, jnl_sequence);
	}

	if ((dbcb->dbcb_status & DBCB_BACKUP) && (dbcb->dbcb_drec_cnt > 0))
	{
	    TRdisplay("\t    New online backup address <%d,%d,%d>.\n",
		dbcb->dbcb_dmp_la.la_sequence,
		dbcb->dbcb_dmp_la.la_block,
		dbcb->dbcb_dmp_la.la_offset);
	    TRdisplay("\t    Old dump block sequence %d, New %d\n",
		 dbcb->dbcb_d_blkseq, dmp_sequence);
	}

	/*
	** Update config file to show how far journal processing has 
	** progressed.
	*/
	status = dm0c_open(&dbcb->dbcb_dcb, 0, a->acb_lk_id,
	    &config, &a->acb_dberr);
	if (status != E_DB_OK)
	{
	    /*
	    ** Fill in exit error information for archiver exit handler.
	    */
	    a->acb_error_code = E_DM9856_ACP_DB_ACCESS_EXIT;
	    STRUCT_ASSIGN_MACRO(dbcb->dbcb_dcb.dcb_name, a->acb_errdb);
	    uleFormat(NULL, a->acb_error_code, (CL_ERR_DESC *)NULL, ULE_LOOKUP,
		(DB_SQLSTATE *)NULL, a->acb_errtext, sizeof(a->acb_errtext),
		&a->acb_errltext, &error, 1,
		DB_DB_MAXNAME, dbcb->dbcb_dcb.dcb_name.db_db_name);

	    return (error_translate(E_DM9844_ARCH_EOC, &a->acb_dberr));
	}

	/* Set the flags for dm0c_close */
	dm0cFlags = DM0C_UPDATE | DM0C_COPY;

	/*  
	** Set new config information. 
	*/
	if ( (dbcb->dbcb_status & DBCB_JNL) && 
	     (dbcb->dbcb_jrec_cnt > 0) )
	{
	    /* Tell dm0c_close the JNL information is updated */
	    dm0cFlags |= DM0C_UPDATE_JNL;

	    if (a->acb_status & ACB_CLUSTER)
	    {
		i4 	nid = a->acb_node_id;

		config->cnf_jnl->jnl_node_info[nid].cnode_blk_seq = 
		    jnl_sequence;
		config->cnf_jnl->jnl_node_info[nid].cnode_la = 
		    a->acb_last_la;
	    }
	    else
	    {
		config->cnf_jnl->jnl_blk_seq = jnl_sequence;  
		config->cnf_jnl->jnl_la = a->acb_last_la;
	    }
	}

	if ( (dbcb->dbcb_status & DBCB_BACKUP) && 
	     (dbcb->dbcb_drec_cnt > 0) )
	{
	    if (a->acb_status & ACB_CLUSTER)
	    {
	        i4      nid = a->acb_node_id;
		
		config->cnf_dmp->dmp_node_info[nid].cnode_blk_seq = 
		    dmp_sequence;
		config->cnf_dmp->dmp_node_info[nid].cnode_la = 
		    dbcb->dbcb_dmp_la;
	    }
	    else
	    {
		config->cnf_dmp->dmp_blk_seq = dmp_sequence;
		config->cnf_dmp->dmp_la = dbcb->dbcb_dmp_la;
	    }
	}


	/*
	** Update and close the config file.
	*/
	status = dm0c_close(config, dm0cFlags, &a->acb_dberr);
	if (status != E_DB_OK)
	{
	    /*
	    ** Fill in exit error information for archiver exit handler.
	    */
	    a->acb_error_code = E_DM9856_ACP_DB_ACCESS_EXIT;
	    STRUCT_ASSIGN_MACRO(dbcb->dbcb_dcb.dcb_name, a->acb_errdb);
	    uleFormat(NULL, a->acb_error_code, (CL_ERR_DESC *)NULL, ULE_LOOKUP,
		(DB_SQLSTATE *)NULL, a->acb_errtext, sizeof(a->acb_errtext),
		&a->acb_errltext, &error, 1,
		DB_DB_MAXNAME, dbcb->dbcb_dcb.dcb_name.db_db_name);

	    return (error_translate(E_DM9844_ARCH_EOC, &a->acb_dberr));
	}
    }

    /*
    ** Do any necessary Online Backup post-processing (signal completion
    ** of any backups that finished or interrupt any that failed).
    */
    status = dma_ebackup(a);
    if (status != E_DB_OK)
	return (error_translate(E_DM9844_ARCH_EOC, &a->acb_dberr));

    /*
    ** Inform the Logging System of the completion of this archive cycle.
    ** The logging system will update all databases to show that they
    ** have now been processed to this log address.
    **
    ** If the oldest CP interval in the log file has no active transactions,
    ** the logging system will move the log file BOF forward.
    */
    if ((a->acb_status & (ACB_CSP | ACB_RCP)) == 0)
    {
	if (a->acb_verbose)
	    TRdisplay("\tInform Logging System of Archive Cycle completion.\n");

	log_address[0].la_sequence = a->acb_last_la.la_sequence;
	log_address[0].la_block    = a->acb_last_la.la_block;
	log_address[0].la_offset   = a->acb_last_la.la_offset;
	log_address[1].la_sequence = a->acb_last_cp.la_sequence;
	log_address[1].la_block    = a->acb_last_cp.la_block;
	log_address[1].la_offset   = a->acb_last_cp.la_offset;

	cl_status = LGalter(LG_A_ARCHIVE_DONE, (PTR)log_address,
				sizeof(log_address), &sys_err);
	if (cl_status != OK)
	{
	    uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	    uleFormat(NULL, E_DM900B_BAD_LOG_ALTER, &sys_err, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &error, 1,
		0, LG_A_ARCHIVE_DONE);

	    /*
	    ** Fill in exit error information for archiver exit handler.
	    */
	    a->acb_error_code = E_DM9855_ACP_LG_ACCESS_EXIT;
	    uleFormat(NULL, a->acb_error_code, (CL_ERR_DESC *)NULL, ULE_LOOKUP,
		(DB_SQLSTATE *)NULL, a->acb_errtext, sizeof(a->acb_errtext),
		&a->acb_errltext, &error, 0);

	    SETDBERR(&a->acb_dberr, 0, E_DM980D_ARCH_LOG_ALTER);
	    return (error_translate(E_DM9844_ARCH_EOC, &a->acb_dberr));
	}

	/*	
	** Force log header to disk. 
	** This will force out the new BOF if it has been moved forward by this
	** archive cycle.
	*/
	cl_status = LGforce(LG_HDR, a->acb_s_lxid, 0, 0, &sys_err);
	if (cl_status != OK)
	{
	    uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	    uleFormat(NULL, E_DM9010_BAD_LOG_FORCE, &sys_err, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, &error, 0);

	    /*
	    ** Fill in exit error information for archiver exit handler.
	    */
	    a->acb_error_code = E_DM9855_ACP_LG_ACCESS_EXIT;
	    uleFormat(NULL, a->acb_error_code, (CL_ERR_DESC *)NULL, ULE_LOOKUP,
		(DB_SQLSTATE *)NULL, a->acb_errtext, sizeof(a->acb_errtext),
		&a->acb_errltext, &error, 0);

	    SETDBERR(&a->acb_dberr, 0, E_DM980B_ARCH_LOG_FORCE);
	    return (error_translate(E_DM9844_ARCH_EOC, &a->acb_dberr));
	}
    }

    /*
    ** Deallocate DBCB control blocks.
    */
    dalloc_dbcbs(acb);

    return (E_DB_OK);
}

/*{
** Name: dma_alloc_dbcb		- Build a new DBCB
**
** Description:
**      This routine builds a DBCB from passed-in record information and 
**	links it onto the ACB list.
**
** Inputs:
**	db_id				Database id of db to add
**	lg_db_id			Internal LG database id.
**      db_name				Pointer to db name
**      db_owner			Pointer to db owner
**      db_root				Pointer to db location
**      db_l_root			Length of location
**
** Outputs:
**	dbcb				Pointer to returned dbcb
**      err_code                        Reason for error status.
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
**      22-dec-1992 (jnash)
**          Created from older versions for the reduced logging project.
**      22-feb-1993 (jnash & rogerk)
**	    The db_id param is now the external logging system id,
**	    and cannot be used as an offset into a static array.
**	    Instead, we hashdance in the old family style.
**	    Added lg_db_id param for the internal db id.
**	15-mar-1993 (rogerk)
**	    Reduced Logging - Phase IV:
**	    Fixed to set the dbcb dump addresses with the proper dump address
**		parameters instead of the journal address parameters.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	26-jul-1993 (rogerk)
**	    Changed trace messages.
**	30-Apr-2003 (bonro01)
**	    Fix for bug caused by change 462432. Parameters to function
**	    dm0m_info() were changed to SIZE_TYPE so the parms in this
**	    routine needed to change as well.  The parms are passed to
**	    ule_format() for output, but the message can only handle 32-bit
**	    values so the variables are cast to (i4) for compatibility on
**	    64-bit platforms.  This message will eventually have to change
**	    to support 64-bit length variables.
[@history_template@]...
*/
DB_STATUS
dma_alloc_dbcb(
ACB	    	*acb,
i4		db_id,
i4		lg_db_id,
DB_DB_NAME	*db_name,
DB_OWN_NAME	*db_owner,
DM_PATH		*db_root,
i4		db_l_root,
LG_LA		*f_jnl_addr,
LG_LA		*l_jnl_addr,
LG_LA		*f_dmp_addr,
LG_LA		*l_dmp_addr,
LG_LA		*sbackup_addr,
DBCB		**dbcb)
{
    ACB		*a = acb;
    DBCB	*db;
    DBCB	**dbq;
    i4 	error;
    DB_STATUS	status;

    CLRDBERR(&a->acb_dberr);

    /*  Allocate and initialize a DBCB. */

    status = dm0m_allocate(sizeof(DBCB), 0, DBCB_CB, DBCB_ASCII_ID, 
	(char *)a, (DM_OBJECT **)dbcb, &a->acb_dberr);
    if (status != E_DB_OK)
    {
	SIZE_TYPE  total_memory;
	SIZE_TYPE  alloc_memory;
	SIZE_TYPE  free_memory;

	/*
	** Would be nice to build a dm0m_info call to return the amount
	** of memory which has been allocated so far.  This could be
	** logged here along with the amount of memory required.
	*/
	dm0m_info(&total_memory, &alloc_memory, &free_memory);
	uleFormat(NULL, E_DM9427_DM0M_MEMORY_ALLOCATE, (CL_ERR_DESC *)NULL, ULE_LOG,
	    (DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, (i4 *)NULL, &error,
	    4, 0, sizeof(DBCB), 0, (i4)total_memory, 0, (i4)alloc_memory,
	    0, (i4)free_memory);

	/*
	** Fill in exit error information for archiver exit handler.
	*/
	a->acb_error_code = E_DM9853_ACP_RESOURCE_EXIT;
	STRUCT_ASSIGN_MACRO(*db_name, a->acb_errdb);
	uleFormat(NULL, a->acb_error_code, (CL_ERR_DESC *)NULL, ULE_LOOKUP,
	    (DB_SQLSTATE *)NULL, a->acb_errtext, sizeof(a->acb_errtext),
	    &a->acb_errltext, &error, 2,
	    0, ERx("memory"), 0, ERx("resources"));

	return (status);
    }

    db = *dbcb;
    db->dbcb_ext_db_id = db_id;
    db->dbcb_int_db_id = lg_db_id;
    db->dbcb_refcnt = 1;
    db->dbcb_jrec_cnt = 0;
    db->dbcb_drec_cnt = 0;
    db->dbcb_status = 0;
    db->dbcb_j_blkseq = 0;
    db->dbcb_j_filseq = 0;
    db->dbcb_dm0j = 0;
    db->dbcb_prev_jnl_la.la_sequence = 0;
    db->dbcb_prev_jnl_la.la_block    = 0;
    db->dbcb_prev_jnl_la.la_offset   = 0;
    db->dbcb_jnl_la.la_sequence = 0;
    db->dbcb_jnl_la.la_block    = 0;
    db->dbcb_jnl_la.la_offset   = 0;

    db->dbcb_first_jnl_la = *f_jnl_addr;
    db->dbcb_last_jnl_la = *l_jnl_addr;
    db->dbcb_first_dmp_la = *f_dmp_addr;
    db->dbcb_last_dmp_la = *l_dmp_addr;
    db->dbcb_sbackup = *sbackup_addr;

    MEfill(sizeof(db->dbcb_dcb), 0, (char *)&db->dbcb_dcb);
    STRUCT_ASSIGN_MACRO(*db_name, db->dbcb_dcb.dcb_name);
    STRUCT_ASSIGN_MACRO(*db_owner, db->dbcb_dcb.dcb_db_owner);
    db->dbcb_dcb.dcb_location.phys_length = db_l_root;
    MEcopy((char *)db_root, sizeof(DM_PATH), 
	(char *)&db->dbcb_dcb.dcb_location.physical);
    db->dbcb_dcb.dcb_status = 0;
    
    /*
    ** Initialize online backup information.
    */
    db->dbcb_backup_complete = FALSE;
    db->dbcb_d_blkseq = 0;
    db->dbcb_d_filseq = 0;
    db->dbcb_dm0d = 0;
    db->dbcb_prev_dmp_la.la_sequence = 0;
    db->dbcb_prev_dmp_la.la_block    = 0;
    db->dbcb_prev_dmp_la.la_offset   = 0;
    db->dbcb_dmp_la.la_sequence = 0;
    db->dbcb_dmp_la.la_block    = 0;
    db->dbcb_dmp_la.la_offset   = 0;
    db->dbcb_ebackup.la_sequence = 0;
    db->dbcb_ebackup.la_block    = 0;
    db->dbcb_ebackup.la_offset   = 0;


    TRdisplay("\tDatabase %~t ID: %x\n", sizeof(DB_DB_NAME), db_name, db_id);
    if (a->acb_verbose)
	TRdisplay("\t    ROOT: %t\n", db_l_root, db_root);
    TRdisplay("\t    Journal Window: (%d,%d,%d)..(%d,%d,%d)\n",
	db->dbcb_first_jnl_la.la_sequence,
	db->dbcb_first_jnl_la.la_block,
	db->dbcb_first_jnl_la.la_offset,
	db->dbcb_last_jnl_la.la_sequence,
	db->dbcb_last_jnl_la.la_block,
	db->dbcb_last_jnl_la.la_offset);

    /*
    ** Put DBCB on hash bucket queue.
    */
    dbq = &acb->acb_dbh[db_id % ACB_DB_HASH];
    db->dbcb_next = *dbq;
    db->dbcb_prev = (DBCB *) dbq;
    if (*dbq)
        (*dbq)->dbcb_prev = db;
    *dbq = db;

    /*
    ** Insert DBCB on active queue.
    */
    db->dbcb_state.stq_next = acb->acb_db_state.stq_next;
    db->dbcb_state.stq_prev = (ACP_QUEUE *)&acb->acb_db_state.stq_next;
    acb->acb_db_state.stq_next->stq_prev = &db->dbcb_state;
    acb->acb_db_state.stq_next = &db->dbcb_state;

    acb->acb_c_dbcb++;

    return(E_DB_OK);
}

/*{
** Name: lookup_dbcb		- Find a dbcb in the hash queue.
**
** Description:
**      This routine finds a given DBCB from the passed in external
**	database id, or returns NULL if non exists.
**
** Inputs:
**	acb				ACB control block
**      db_id				The external db id
**
** Outputs:
**	dbcb				Pointer to returned dbcb
**					  or NULL if not found
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      22-feb-1993 (jnash)
**          Create for reduced logging project.
[@history_template@]...
*/
static VOID
lookup_dbcb(
ACB	    *a, 
i4	    db_id, 
DBCB	    **ret_dbcb)
{
    ACP_QUEUE   *st;
    DBCB     	*dbcb;

    *ret_dbcb = NULL;

    for (dbcb = a->acb_dbh[db_id % ACB_DB_HASH];
	 dbcb != (DBCB *) NULL;
	 dbcb = dbcb->dbcb_next)
    {
        if (dbcb->dbcb_ext_db_id == db_id)
        {
            *ret_dbcb = dbcb;
            return;
        }
    }

    return;
}

/*{
** Name: dalloc_dbcbs		- Deallocate all DBCBs.
**
** Description:
**      This routine removes all DBCBs from the ACB hash queue.
**
**	If this is the CSP performing cluster archiving, then we now go and
**	mark the database as being not open on this node.
**
** Inputs:
**	dbcb				The element to remove
**
** Outputs:
**      VOID
**
** Side Effects:
**	    none
**
** History:
**      22-feb-1993 (jnash)
**          Created for the reduced logging project.
**	20-sep-1993 (bryanp)
**	    Following completion of cluster archiving for a database on a node,
**		mark that database as no longer open on this node.
[@history_template@]...
*/
static VOID
dalloc_dbcbs(
ACB	    	*acb)
{
    ACB		    *a = acb;
    ACP_QUEUE	    *st;
    DBCB	    *dbcb;
    i4	    	    error;

    /*
    ** Deallocate DBCB control blocks.
    */
    st = a->acb_db_state.stq_next;
    while (st != (ACP_QUEUE *) &a->acb_db_state.stq_next)
    {
        dbcb = (DBCB *)((char *)st - CL_OFFSETOF(DBCB, dbcb_state));

	if ((a->acb_status & ACB_CSP) != 0)
	{
	    if (mark_fully_closed(a, dbcb))
	    {
		uleFormat(&a->acb_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
			    (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);

		/* duh... then what? keep on going? abort? dunno ... */
	    }
	}
	st = dbcb->dbcb_state.stq_next;

	/*
	** Remove DBCB from the active state queue.
	*/
	dbcb->dbcb_state.stq_next->stq_prev = dbcb->dbcb_state.stq_prev;
	dbcb->dbcb_state.stq_prev->stq_next = dbcb->dbcb_state.stq_next;

	/*
	** Remove it from its hash bucket queue.
	*/
	if (dbcb->dbcb_next)
	    dbcb->dbcb_next->dbcb_prev = dbcb->dbcb_prev;
	dbcb->dbcb_prev->dbcb_next = dbcb->dbcb_next;

	/*
	** Deallocate the control block.
	*/
	dm0m_deallocate((DM_OBJECT **)&dbcb);

	a->acb_c_dbcb--;
    }
}

/*
** Name: mark_fully_closed	    - mark the database as closed by this node
**
** Description:
**	The database is marked as fully closed on this node.
**
**	This routine exists because cluster archiving following a machine or
**	system failure has some very special requirements, since the archiving
**	in cluster restart cases is performed by the CSP whereas in non-cluster
**	scenarios archiving is always performed by the node's archiver (after
**	it is restarted following recovery).
**
**	Essentially, in non-cluster situations, the RCP starts up, performs
**	recovery, discovers that some databases require archiving, adds them
**	to the local logging system, and then leaves them active in the logging
**	system. The local archiver is then started, discovers that the databases
**	need archiving, archives them, and tells the logging system is has done
**	so.
**
**	In the cluster situation, however, this technique does not work, because
**	the CSP is recovering the log files for other nodes, but it cannot
**	simply start the archiver on that other node to perform the archiving.
**	Instead, the CSP must perform the archiving itself. Once it has
**	completed the archiving, there is no longer any reason to leave the
**	database marked as open and active in the logging system, since it is
**	neither. Therefore, the CSP archiving routines call this routine to
**	mark the database as closed and remove it from the logging system.
**
** History:
**	20-sep-1993 (bryanp)
**	    Created to solve some cluster archiving problems.
*/
static DB_STATUS
mark_fully_closed( ACB *acb, DBCB *dbcb)
{
    ACB		    *a = acb;
    DMP_DCB	    local_dcb;
    DM0C_CNF	    *cnf = 0;
    DB_STATUS	    status = E_DB_OK;
    STATUS	    cl_status;
    LG_DATABASE	    db;
    CL_ERR_DESC	    sys_err;
    i4	    open_mask;
    i4	    length;
    i4		    error;

    CLRDBERR(&a->acb_dberr);

    for (;;)
    {
	/*
	** Format a local DCB to use for the config file update since we
	** will not have always succeeded in opening the database when we
	** get to here.
	*/
	MEfill(sizeof(local_dcb), 0, (char *)&local_dcb);
	MEmove(sizeof(dbcb->dbcb_dcb.dcb_name), (PTR)&dbcb->dbcb_dcb.dcb_name,
		' ',
		sizeof(local_dcb.dcb_name), (PTR)&local_dcb.dcb_name);
	MEmove(sizeof(dbcb->dbcb_dcb.dcb_owner), (PTR)&dbcb->dbcb_dcb.dcb_owner,
		' ',
		sizeof(local_dcb.dcb_db_owner), (PTR)&local_dcb.dcb_db_owner);
	MEmove(dbcb->dbcb_dcb.dcb_location.phys_length,
		(PTR)&dbcb->dbcb_dcb.dcb_location.physical, ' ',
		sizeof(local_dcb.dcb_location.physical), 
		(PTR)&local_dcb.dcb_location.physical);
	local_dcb.dcb_location.phys_length =
				    dbcb->dbcb_dcb.dcb_location.phys_length;
	local_dcb.dcb_access_mode = DCB_A_WRITE;
	local_dcb.dcb_served = DCB_MULTIPLE;
	local_dcb.dcb_db_type = DCB_PUBLIC;
	local_dcb.dcb_status = 0;
	    
	TRdisplay("%@ DMA_COMPLETE: Mark DB %~t closed on node %d\n",
		    sizeof(DB_DB_NAME), (PTR)&local_dcb.dcb_name,
		    a->acb_lctx->lctx_node_id);

	/*
	** Open the config file
	*/
	status = dm0c_open(&local_dcb, DM0C_PARTIAL, dmf_svcb->svcb_lock_list, 
			    &cnf, &a->acb_dberr);
	if (status != E_DB_OK)
	    break;

	/*
	** Mark the database closed now that archiving is complete.
	*/
	open_mask = 1 << a->acb_lctx->lctx_node_id;
	cnf->cnf_dsc->dsc_open_count &= ~open_mask;

	status = dm0c_close(cnf, DM0C_UPDATE, &a->acb_dberr);
	if (status != E_DB_OK)
	    break;

	/*
	** See if the database is known to the logging system. If so, indicate
	** that the database is now closed.
	*/
	if (dbcb->dbcb_int_db_id == 0)
	{
	    db.db_database_id = dbcb->dbcb_ext_db_id;
	    cl_status = LGshow(LG_S_DBID, (PTR)&db, sizeof(db),
				    &length, &sys_err);
	    if (cl_status != OK)
	    {
		uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
			(char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
		uleFormat(NULL, E_DM9017_BAD_LOG_SHOW, &sys_err, ULE_LOG, NULL,
			(char *)NULL, (i4)0, (i4 *)NULL, &error, 1,
			0, LG_S_DBID);
		SETDBERR(&a->acb_dberr, 0, E_DM9844_ARCH_EOC);
		break;
	    }
	    if (length != 0)
		dbcb->dbcb_int_db_id = db.db_id;
	}

	if (dbcb->dbcb_int_db_id != 0)
	{
	    if (cl_status = LGalter(LG_A_CLOSEDB, (PTR)&dbcb->dbcb_int_db_id,
			sizeof(dbcb->dbcb_int_db_id), &sys_err))
	    {
		uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
			(char *)NULL, 0L, (i4 *)NULL, &error, 0);
		uleFormat(NULL, E_DM900B_BAD_LOG_ALTER, &sys_err, ULE_LOG, NULL,
			(char *)NULL, 0L, (i4 *)NULL, &error, 1,
			0, LG_A_CLOSEDB);
		SETDBERR(&a->acb_dberr, 0, E_DM9844_ARCH_EOC);
		break;
	    }
	}

	return (E_DB_OK);
    }

    uleFormat(&a->acb_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
	(char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
    SETDBERR(&a->acb_dberr, 0, E_DM9844_ARCH_EOC);
    return (E_DB_ERROR);
}

/*{
** Name: init_journal - initialize jounral file.
**
** Description:
**	This routine opens the database JOURNAL file in preparation to
**	move records from the log file.  If the file does not exist, or 
**	if the current file is too large, then a new file is created.
**
** Inputs:
**      acb                             The ACB.
**	db				Backup database control block.
**
** Outputs:
**      err_code                        The reason for error return status.
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
**	28-dec-1992 (jnash)
**	    Created from a bit of old dma_p2 the reduced logging project.
**	    Although this routine could be inline in dma_copy at 
**	    present, it was created to make the change to "create
**	    journal file at first reference" easier.
**	15-mar-1993 (rogerk)
**	    Added direction parameter to dm0j_open routine.
**	24-may-1993 (jnash)
** 	   Preallocate space only in the first journal file created after
**	   a ckpdb.
[@history_template@]...
*/
static DB_STATUS
init_journal(
ACB	    *acb,
DBCB	    *db)
{
    ACB			*a = acb;
    DB_STATUS		status;
    i4		error;
    i4		i;
    i4		fil_seq;
    i4		blk_seq;
    i4		blk_cnt;
    i4		cur_entry;
    char		jnlerr_db[DB_DB_MAXNAME+1];
    char		jnlerr_dir[DB_AREA_MAX+1];
    DM0C_EXT    	*ext;
    DM0C_CNF		*config;
    DB_ERROR		local_dberr;
    i4			dm0cFlags = 0;

    CLRDBERR(&a->acb_dberr);

    /*
    ** Open the config file for this db, fill in last
    ** journaled log address, last file and block number.
    */
    status = dm0c_open(&db->dbcb_dcb, 0, a->acb_lk_id, &config, &a->acb_dberr);
    if (status != E_DB_OK)
    {
	/*
	** Error opening database config file - database cannot be
	** processed.  Fill in exit error information for archiver
	** exit handler.
	*/
	a->acb_error_code = E_DM9856_ACP_DB_ACCESS_EXIT;
	STRUCT_ASSIGN_MACRO(db->dbcb_dcb.dcb_name, a->acb_errdb);
	uleFormat(NULL, a->acb_error_code, (CL_ERR_DESC *)NULL, ULE_LOOKUP,
	    (DB_SQLSTATE *)NULL, a->acb_errtext, 
	    sizeof(a->acb_errtext), &a->acb_errltext, 
	    &error, 1, DB_DB_MAXNAME, db->dbcb_dcb.dcb_name.db_db_name);

	return (error_translate(E_DM9835_ARCH_NEW_JNL_ERROR, &a->acb_dberr));
    }

    /*
    ** Get the current journal file sequence and block number.
    **
    ** If this is a cluster installation, get this information 
    ** out of the cluster node info array - as the block 
    ** number an jnl address will be different for each node.  
    ** Note that if no journal file has yet been created for 
    ** this database, then the fil_seq in the cluster case will 
    ** be zero.  We should not find a #0 jnl file and
    ** should fall through below to create a journal file 
    ** with the proper sequence number (jnl_fil_seq) and will 
    ** update the cnode_fil_seq at that time.
    */
    if (a->acb_status & ACB_CLUSTER)
    {
	i4	nid = a->acb_node_id;

	fil_seq = config->cnf_jnl->jnl_node_info[nid].cnode_fil_seq;
	blk_seq = config->cnf_jnl->jnl_node_info[nid].cnode_blk_seq;
    }
    else
    {
	fil_seq = config->cnf_jnl->jnl_fil_seq;
	blk_seq = config->cnf_jnl->jnl_blk_seq;
    }

    /*  Find journal location. */

    for (i = 0; i < config->cnf_dsc->dsc_ext_count; i++)
    {
	ext = &config->cnf_ext[i];
	if (ext->ext_location.flags & DCB_JOURNAL)
	    break;
    }

    db->dbcb_j_path = ext->ext_location.physical;
    db->dbcb_lj_path = ext->ext_location.phys_length;

    /*  Open journal file for this DBCB. */
    /* If Journal Switch is requested, then open a new journal */

    if ((blk_seq < config->cnf_jnl->jnl_maxcnt) &&
    	((db->dbcb_status & DBCB_JSWITCH) == 0) )
    {
	status = dm0j_open(0, (char *)&db->dbcb_j_path, db->dbcb_lj_path, 
			&db->dbcb_dcb.dcb_name, config->cnf_jnl->jnl_bksz,
			fil_seq, config->cnf_jnl->jnl_ckp_seq,
			blk_seq + 1, DM0J_M_WRITE, a->acb_node_id,
			DM0J_FORWARD, (DB_TAB_ID *)0, 
			(DM0J_CTX **)&db->dbcb_dm0j, &a->acb_dberr);

	if (status == E_DB_OK && fil_seq <= 0)
	{
	    /*  Internal sanity check violation -- corrupt config file. */
	    TRdisplay(
		"%@ ACP INIT_JOURNAL: Can't open journal file %d\n", fil_seq);
	    status = E_DB_ERROR;
	    SETDBERR(&a->acb_dberr, 0, E_DM983A_ARCH_OPEN_JNL_ZERO);
	}
    }
    else 
    {
	/*
	** If the current journal file is full, then we need to create
	** a new journal file.  Treat this case the same as getting
	** FILE_NOT_FOUND from the above dm0j_open call.
	**
	** Increment the journal sequence number to create a new file.
	*/
#ifdef xACP_TRACE
	TRdisplay(
	   "init_journal: Journal file sequence %d at location %~t is full.\n",
	    config->cnf_jnl->jnl_fil_seq, db->dbcb_lj_path, &db->dbcb_j_path);
#endif

	write_jnl_eof(acb, db, (DM0J_CTX *)0, config, fil_seq, blk_seq);

	status = E_DB_ERROR;
	SETDBERR(&a->acb_dberr, 0, E_DM9034_BAD_JNL_NOT_FOUND);

	/* Increment to next journal file */
	config->cnf_jnl->jnl_fil_seq++;
    }

    if (status != E_DB_OK)
    {
	if (a->acb_dberr.err_code != E_DM9034_BAD_JNL_NOT_FOUND)	
	{
	    /*
	    ** Fill in exit error information for archiver exit handler.
	    */
	    a->acb_error_code = E_DM9854_ACP_JNL_ACCESS_EXIT;
	    STRUCT_ASSIGN_MACRO(db->dbcb_dcb.dcb_name, a->acb_errdb);

	    STncpy( jnlerr_db, db->dbcb_dcb.dcb_name.db_db_name, DB_DB_MAXNAME);
	    jnlerr_db[ DB_DB_MAXNAME ] = '\0';
	    STtrmwhite(jnlerr_db);
	    STncpy(jnlerr_dir, db->dbcb_j_path.name, DB_AREA_MAX);
	    jnlerr_dir[ DB_AREA_MAX ] = '\0';
	    STtrmwhite(jnlerr_dir);
	    uleFormat(NULL, a->acb_error_code, (CL_ERR_DESC *)NULL, ULE_LOOKUP,
		(DB_SQLSTATE *)NULL, a->acb_errtext, sizeof(a->acb_errtext),
		&a->acb_errltext, &error, 2,
		0, jnlerr_db, 0, jnlerr_dir);

	    status = dm0c_close(config, 0, &local_dberr);

	    return (error_translate(E_DM9835_ARCH_NEW_JNL_ERROR, &a->acb_dberr));
	}

	/*
	** Update the journal history to show the new file.
	**
	** For the non-cluster case, include the new journal file in the
	** journal history - set the last-journal number to the new file
	** and set the first-journal number if it is currently zero.
	**
	** If this is a cluster installation, then we only update the
	** journal history if this is the first journal file for this
	** database (for the current checkpoint).  Otherwise we only
	** update the cluster node info to show the new journal file.
	** The journal history shows the current journal file sequence
	** number for MERGED journal files.  It will be updated by DM0J
	** whenever a merge is executed.
	*/
	i = config->cnf_jnl->jnl_count - 1;

	if ((a->acb_status & ACB_CLUSTER) == 0)
	{
	    config->cnf_jnl->jnl_blk_seq = 0;
	    config->cnf_jnl->jnl_history[i].ckp_l_jnl = 
		config->cnf_jnl->jnl_fil_seq;
	    if (config->cnf_jnl->jnl_history[i].ckp_f_jnl == 0)
	    {
		config->cnf_jnl->jnl_history[i].ckp_f_jnl = 
		    config->cnf_jnl->jnl_fil_seq;
	    }
	}
	else
	{
	    config->cnf_jnl->jnl_node_info[a->acb_node_id].cnode_blk_seq =0;
	    config->cnf_jnl->jnl_node_info[a->acb_node_id].cnode_fil_seq = 
		config->cnf_jnl->jnl_fil_seq;

	    if (config->cnf_jnl->jnl_history[i].ckp_f_jnl == 0)
	    {
		config->cnf_jnl->jnl_blk_seq = 0;
		config->cnf_jnl->jnl_history[i].ckp_l_jnl = 
			config->cnf_jnl->jnl_fil_seq;
		config->cnf_jnl->jnl_history[i].ckp_f_jnl = 
			config->cnf_jnl->jnl_fil_seq;
	    }
	}

	TRdisplay(
	    "%@ ACP INIT_JOURNAL: Create journal file sequence number %d at location %t.\n", 
	    config->cnf_jnl->jnl_fil_seq, db->dbcb_lj_path, &db->dbcb_j_path);
	    

	if (config->cnf_jnl->jnl_fil_seq > 0)	    
	{
	    /*
	    ** Preallocate the first journal file with jnl_blkcnt number of
	    ** blocks.  Journals beyond than the first are not preallocated 
	    ** because of the performance hit and lack of predictability.
	    */
	    cur_entry = config->cnf_jnl->jnl_count - 1;
	    if (config->cnf_jnl->jnl_history[cur_entry].ckp_f_jnl == 
		config->cnf_jnl->jnl_history[cur_entry].ckp_l_jnl)
		   blk_cnt = config->cnf_jnl->jnl_blkcnt;
	    else
		blk_cnt = 0;

	    status = dm0j_create(0, (char *)&db->dbcb_j_path, 
			db->dbcb_lj_path,
			config->cnf_jnl->jnl_fil_seq,
			config->cnf_jnl->jnl_bksz,
			blk_cnt, 
			a->acb_node_id, (DB_TAB_ID *)0, &a->acb_dberr);
	}
	else
	{
	    /*  Sanity check violation -- config file is corrupt */

	    TRdisplay("%@ ACP INIT_JOURNAL: Can't create journal file %d.\n",
			config->cnf_jnl->jnl_fil_seq);
	    status = E_DB_ERROR;
	    SETDBERR(&a->acb_dberr, 0, E_DM9839_ARCH_JNL_SEQ_ZERO);
	}

	/*  Handle error creating journal file. */

	if (status != OK)
	{
	    /*
	    ** Fill in exit error information for archiver exit handler.
	    ** On dm0j_create error, report out-of-disk-space.
	    */
	    STncpy( jnlerr_db, db->dbcb_dcb.dcb_name.db_db_name, DB_DB_MAXNAME);
	    jnlerr_db[ DB_DB_MAXNAME ] = '\0';
	    STtrmwhite(jnlerr_db);
	    STncpy( jnlerr_dir, db->dbcb_j_path.name, DB_AREA_MAX);
	    jnlerr_dir[ DB_AREA_MAX] = '\0';
	    STtrmwhite(jnlerr_dir);
	    STRUCT_ASSIGN_MACRO(db->dbcb_dcb.dcb_name, a->acb_errdb);
	    if (a->acb_dberr.err_code == E_DM9839_ARCH_JNL_SEQ_ZERO)
	    {
		a->acb_error_code = E_DM9854_ACP_JNL_ACCESS_EXIT;
		uleFormat(NULL, a->acb_error_code, (CL_ERR_DESC *)NULL, ULE_LOOKUP,
		    (DB_SQLSTATE *)NULL, a->acb_errtext,
		    sizeof(a->acb_errtext), &a->acb_errltext,
		    &error, 2, 0, jnlerr_db, 0, jnlerr_dir);
	    }
	    else
	    {
		a->acb_error_code = E_DM9852_ACP_JNL_WRITE_EXIT;
		uleFormat(NULL, a->acb_error_code, (CL_ERR_DESC *)NULL, ULE_LOOKUP,
		    (DB_SQLSTATE *)NULL, a->acb_errtext,
		    sizeof(a->acb_errtext), &a->acb_errltext,
		    &error, 3,  0, jnlerr_db, 0, jnlerr_dir, 0,
		    ERx("create"));
	    }

	    status = dm0c_close(config, 0, &local_dberr);

	    return (error_translate(E_DM9835_ARCH_NEW_JNL_ERROR, &a->acb_dberr));
	}

	/*
	** The test for ACB_CLUSTER isn't really necessary here -- the
	** algorithm guarantees that the file sequence number in the node
	** info is identical to cnf_jnl->jnl_fil_seq and that the blk_seq
	** is 0, but this way we can use the same code as above.
	*/
	if (a->acb_status & ACB_CLUSTER)
	{
	    i4	nid = a->acb_node_id;

	    fil_seq = config->cnf_jnl->jnl_node_info[nid].cnode_fil_seq;
	    blk_seq = config->cnf_jnl->jnl_node_info[nid].cnode_blk_seq;
	}
	else
	{
	    fil_seq = config->cnf_jnl->jnl_fil_seq;
	    blk_seq = config->cnf_jnl->jnl_blk_seq;
	}

	status = dm0j_open(0, (char *)&db->dbcb_j_path, db->dbcb_lj_path, 
	    &db->dbcb_dcb.dcb_name, config->cnf_jnl->jnl_bksz,
	    fil_seq, config->cnf_jnl->jnl_ckp_seq,
	    blk_seq + 1, DM0J_M_WRITE, a->acb_node_id, 
	    DM0J_FORWARD, 
	    (DB_TAB_ID *)0, (DM0J_CTX **)&db->dbcb_dm0j, &a->acb_dberr);
	if (status != E_DB_OK)
	{
	    TRdisplay("%@ ACP INIT_JOURNAL: Error opening file sequence number %d at location %t.\n", 
		fil_seq, db->dbcb_lj_path, &db->dbcb_j_path);

	    status = dm0c_close(config, 0, &local_dberr);

	    /*
	    ** Fill in exit error information for archiver exit handler.
	    */
	    a->acb_error_code = E_DM9854_ACP_JNL_ACCESS_EXIT;
	    STRUCT_ASSIGN_MACRO(db->dbcb_dcb.dcb_name, a->acb_errdb);
	    STncpy(jnlerr_db, db->dbcb_dcb.dcb_name.db_db_name, DB_DB_MAXNAME);
	    jnlerr_db[ DB_DB_MAXNAME ] = '\0';
	    STtrmwhite(jnlerr_db);
	    STncpy(jnlerr_dir, db->dbcb_j_path.name, DB_AREA_MAX);
	    jnlerr_dir[ DB_AREA_MAX ] = '\0';
	    STtrmwhite(jnlerr_dir);
	    uleFormat(NULL, a->acb_error_code, (CL_ERR_DESC *)NULL, ULE_LOOKUP,
		(DB_SQLSTATE *)NULL, a->acb_errtext, sizeof(a->acb_errtext),
		&a->acb_errltext, &error, 2,
		0, jnlerr_db, 0, jnlerr_dir);

	    return (error_translate(E_DM9835_ARCH_NEW_JNL_ERROR, &a->acb_dberr));
	}

	/* Update config with new journal info */
	dm0cFlags = DM0C_UPDATE | DM0C_UPDATE_JNL;

    } /* end if error opening journal file. */

    /*  Update and close the configuration file. */

    status = dm0c_close(config, dm0cFlags, &a->acb_dberr);
    if (status != E_DB_OK)
    {
	/*
	** Fill in exit error information for archiver exit handler.
	*/
	a->acb_error_code = E_DM9856_ACP_DB_ACCESS_EXIT;
	STRUCT_ASSIGN_MACRO(db->dbcb_dcb.dcb_name, a->acb_errdb);
	uleFormat(NULL, a->acb_error_code, (CL_ERR_DESC *)NULL, ULE_LOOKUP, 
	    (DB_SQLSTATE *)NULL, a->acb_errtext, sizeof(a->acb_errtext), 
	    &a->acb_errltext, &error, 1, DB_DB_MAXNAME, 
	    db->dbcb_dcb.dcb_name.db_db_name);

	return (error_translate(E_DM9835_ARCH_NEW_JNL_ERROR, &a->acb_dberr));
    }

    /*
    **  Remember starting block sequence. 
    */
    db->dbcb_j_blkseq = blk_seq;

    return(E_DB_OK);
}

/*{
** Name: error_translate	- Translate from one DMF error to another.
**
** Description:
**      This routine will translate from one DMF error with parameters to 
**	another DMF error.  The previous DMF error is logged.
**
** Inputs:
**      new_error			    New error number.
**	old_error			    Old error number.
**
** Outputs:
**      err_code                            Location to place new error number.
**	Returns:
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      29-sep-1986 (Derek)
**          Created.
**      25-feb-1991 (rogerk)
**          Changed routine to log the error message to the Ingres Error Log
**          as well as the ACP Log.  Use ule_format to write to both
**          files rather than TRdisplay.  Side effect of this is that the
**          ACP Log error messages will no longer be preceded with the
**          string "ACP ERROR->".
[@history_template@]...
*/
static DB_STATUS
error_translate(
i4	    new_error,
DB_ERROR    *dberr)
{
    i4		error;

    uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, (DB_SQLSTATE *)NULL,
        (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);

    /*
    ** Reset error code to new trace-back error.
    **
    ** Retain original error source information.
    */
    dberr->err_code = new_error;

    return (E_DB_ERROR);
}

/*{
** Name: init_dump - initialize dump file for dump processing.
**
** Description:
**	This routine is called to open the database DUMP file in preparation 
**	to move records from the journal file as part of Online Checkpoint.
**
**	If the dump file does not exist, or if the current dump file is
**	too large, then a new file is created.
**
** Inputs:
**      acb                             The ACB.
**	db				Backup database control block.
**	fil_seq				Dump file sequence number.
**	blk_seq				Dump file block number.
**
** Outputs:
**      err_code                        The reason for error return status.
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
**      6-jan-1989 (Edhsu)
**	    Created for Terminator
**	25-feb-1991 (rogerk)
**	    Added support for Archiver Stability project.  Fill in exit
**	    error information whan an ACP-Fatal error is encountered.
**	05-jan-1992 (jnash)
**	    Reduced logging project.  Dump file now opened upon first
**	    write, requiring an open/update/close of the config file here.  
**	    Config record no longer passed as param.
[@history_template@]...
*/
static DB_STATUS
init_dump(
ACB	    *acb,
DBCB	    *db,
i4	    fil_seq,
i4	    blk_seq)
{
    ACB		*a = acb;
    i4	i;
    DB_STATUS	status;
    i4		error;
    char	jnlerr_db[DB_DB_MAXNAME+1];
    char	jnlerr_dir[DB_AREA_MAX+1];
    DM0C_CNF	*config;
    DB_ERROR	local_dberr;

    CLRDBERR(&a->acb_dberr);

    /*
    ** Open the config file.
    */
    status = dm0c_open(&db->dbcb_dcb, 0, a->acb_lk_id, &config, &a->acb_dberr);
    if (status != E_DB_OK)
    {
	/*
	** Error opening database config file - database cannot be
	** processed.  Fill in exit error information for archiver
	** exit handler.
	*/
	a->acb_error_code = E_DM9856_ACP_DB_ACCESS_EXIT;
	STRUCT_ASSIGN_MACRO(db->dbcb_dcb.dcb_name, a->acb_errdb);
	uleFormat(NULL, a->acb_error_code, (CL_ERR_DESC *)NULL, ULE_LOOKUP,
	    (DB_SQLSTATE *)NULL, a->acb_errtext, 
	    sizeof(a->acb_errtext), &a->acb_errltext, 
	    &error, 1, DB_DB_MAXNAME, db->dbcb_dcb.dcb_name.db_db_name);

	return (error_translate(E_DM9825_ARCH_INIT_DUMP, &a->acb_dberr));
    }

    /*  Find dump location. */

    for (i = 0; i < config->cnf_dsc->dsc_ext_count; i++)
    {
	if (config->cnf_ext[i].ext_location.flags & DCB_DUMP)
	    break;
    }

    db->dbcb_d_path = config->cnf_ext[i].ext_location.physical;
    db->dbcb_ld_path = config->cnf_ext[i].ext_location.phys_length;

    /*
    ** Get the current dump file sequence and block number.
    **
    ** If this is a cluster installation, get this information 
    ** out of the cluster node info array - as the block 
    ** number an dmp address will be different for each node.  
    ** Note that if no dump file has yet been created for 
    ** this database, then the fil_seq in the cluster case will 
    ** be zero.  We should not find a #0 dmp file and
    ** should fall through below to create a dump file 
    ** with the proper sequence number (dmp_fil_seq) and will 
    ** update the cnode_fil_seq at that time.
    */
    if (a->acb_status & ACB_CLUSTER)
    {
	i4	nid = a->acb_node_id;

	fil_seq = config->cnf_dmp->dmp_node_info[nid].cnode_fil_seq;
	blk_seq = config->cnf_dmp->dmp_node_info[nid].cnode_blk_seq;
    }
    else
    {
	fil_seq = config->cnf_dmp->dmp_fil_seq;
	blk_seq = config->cnf_dmp->dmp_blk_seq;
    }

    /*  Open dump file for this DBCB. */

    if (blk_seq < config->cnf_dmp->dmp_maxcnt)
    {
	status = dm0d_open(0, (char *)&db->dbcb_d_path, db->dbcb_ld_path, 
	    &db->dbcb_dcb.dcb_name, config->cnf_dmp->dmp_bksz,
	    fil_seq, config->cnf_dmp->dmp_ckp_seq,
	    blk_seq + 1, DM0D_M_WRITE, a->acb_node_id, DM0D_FORWARD,
	    (DM0D_CTX **)&db->dbcb_dm0d, &a->acb_dberr);
    }
    else 
    {
	/*
	** Need a new dump, old one full, same as not found. 
	** Increment the dump file sequence to create a new dump file.
	*/
	TRdisplay("%@ ACP INIT_DUMP: Dump file sequence number %d has reached maximum size - %d blocks.\n",
	    config->cnf_dmp->dmp_fil_seq, blk_seq);
	config->cnf_dmp->dmp_fil_seq++;
	status = E_DB_ERROR;
	SETDBERR(&a->acb_dberr, 0, E_DM9352_BAD_DMP_NOT_FOUND);
    }

    if (status != E_DB_OK)
    {
	if (a->acb_dberr.err_code != E_DM9352_BAD_DMP_NOT_FOUND)	
	{
	    if (config != 0)
		status = dm0c_close(config, 0, &local_dberr);

	    /*
	    ** Fill in exit error information for archiver exit handler.
	    */
	    a->acb_error_code = E_DM9858_ACP_ONLINE_BACKUP_EXIT;
	    STRUCT_ASSIGN_MACRO(db->dbcb_dcb.dcb_name, a->acb_errdb);
	    uleFormat(NULL, a->acb_error_code, (CL_ERR_DESC *)NULL, ULE_LOOKUP, 
		(DB_SQLSTATE *)NULL, a->acb_errtext, sizeof(a->acb_errtext), 
		&a->acb_errltext, &error, 1,
		DB_DB_MAXNAME, db->dbcb_dcb.dcb_name.db_db_name);

	    return (error_translate(E_DM9825_ARCH_INIT_DUMP, &a->acb_dberr));
	}

	/*
	** Create the dump file.
	**
	** Update the history for this checkpoint. 
	** If it is the initial checkpoint, set both first and last.
	**
	** If this is a cluster installation, then we only update the
	** dump history if this is the first dump file for this
	** database (for the current checkpoint).  Otherwise we only
	** update the cluster node info to show the new dump file.
	** The journal history shows the current dump file sequence
	** number for MERGED journal files.  It will be updated by DM0D
	** whenever a merge is executed.
	*/
	i = config->cnf_dmp->dmp_count - 1;

	if ((a->acb_status & ACB_CLUSTER) == 0)
	{
	    config->cnf_dmp->dmp_blk_seq = 0;
	    config->cnf_dmp->dmp_history[i].ckp_l_dmp =
		config->cnf_dmp->dmp_fil_seq;
	    if (config->cnf_dmp->dmp_history[i].ckp_f_dmp == 0)
	    {
		config->cnf_dmp->dmp_history[i].ckp_f_dmp = 
			config->cnf_dmp->dmp_fil_seq;
	    }
	}
	else
	{
	    config->cnf_dmp->dmp_node_info[a->acb_node_id].cnode_blk_seq =0;
	    config->cnf_dmp->dmp_node_info[a->acb_node_id].cnode_fil_seq = 
		config->cnf_dmp->dmp_fil_seq;

	    if (config->cnf_dmp->dmp_history[i].ckp_f_dmp == 0)
	    {
		config->cnf_dmp->dmp_blk_seq = 0;
		config->cnf_dmp->dmp_history[i].ckp_l_dmp = 
			config->cnf_dmp->dmp_fil_seq;
		config->cnf_dmp->dmp_history[i].ckp_f_dmp = 
			config->cnf_dmp->dmp_fil_seq;
	    }
	}

	TRdisplay("%@ ACP INIT_DUMP: Create dump file sequence number %d at location %t.\n", 
	    config->cnf_dmp->dmp_fil_seq, db->dbcb_ld_path, &db->dbcb_d_path);
	
	status = dm0d_create(0,(char *)&db->dbcb_d_path, db->dbcb_ld_path,
	    config->cnf_dmp->dmp_fil_seq,
	    config->cnf_dmp->dmp_bksz, config->cnf_dmp->dmp_blkcnt, 
	    a->acb_node_id, &a->acb_dberr);

	/*  Handle error creating journal file. */

	if (status != OK)
	{
	    if (config != 0)
		status = dm0c_close(config, 0, &local_dberr);

	    /*
	    ** Fill in exit error information for archiver exit handler.
	    */
	    a->acb_error_code = E_DM985C_ACP_DMP_WRITE_EXIT;
	    STRUCT_ASSIGN_MACRO(db->dbcb_dcb.dcb_name, a->acb_errdb);
	    STncpy( jnlerr_db, db->dbcb_dcb.dcb_name.db_db_name, DB_DB_MAXNAME);
	    jnlerr_db[ DB_DB_MAXNAME ] = '\0';
	    STtrmwhite(jnlerr_db);
	    STncpy( jnlerr_dir, db->dbcb_d_path.name, DB_AREA_MAX);
	    jnlerr_dir[ DB_AREA_MAX ] = '\0';
	    STtrmwhite(jnlerr_dir);
	    uleFormat(NULL, a->acb_error_code, (CL_ERR_DESC *)NULL, ULE_LOOKUP, 
		(DB_SQLSTATE *)NULL, a->acb_errtext, sizeof(a->acb_errtext), 
		&a->acb_errltext, &error, 3,
		0, jnlerr_db, 0, jnlerr_dir, 0, ERx("create"));

	    return (error_translate(E_DM9825_ARCH_INIT_DUMP, &a->acb_dberr));
	}

	/*
	** The test for ACB_CLUSTER isn't really necessary here -- the
	** algorithm guarantees that the file sequence number in the node
	** info is identical to cnf_dmp->dmp_fil_seq and that the blk_seq
	** is 0, but this way we can use the same code as above.
	*/
	if (a->acb_status & ACB_CLUSTER)
	{
	    i4	nid = a->acb_node_id;

	    fil_seq = config->cnf_dmp->dmp_node_info[nid].cnode_fil_seq;
	    blk_seq = config->cnf_dmp->dmp_node_info[nid].cnode_blk_seq;
	}
	else
	{
	    fil_seq = config->cnf_dmp->dmp_fil_seq;
	    blk_seq = config->cnf_dmp->dmp_blk_seq;
        }

	status = dm0d_open(0, (char *)&db->dbcb_d_path, db->dbcb_ld_path, 
	    &db->dbcb_dcb.dcb_name, config->cnf_dmp->dmp_bksz,
	    fil_seq, config->cnf_dmp->dmp_ckp_seq,
	    blk_seq + 1, DM0D_M_WRITE, a->acb_node_id, DM0D_FORWARD,
	    (DM0D_CTX **)&db->dbcb_dm0d, &a->acb_dberr);
	if (status != E_DB_OK)
	{
	    if (config != 0)
		status = dm0c_close(config, 0, &local_dberr);
	    TRdisplay("%@ ACP INIT_DUMP: Error opening file sequence number %d at location %t.\n", 
		fil_seq, db->dbcb_ld_path, &db->dbcb_d_path);

	    /*
	    ** Fill in exit error information for archiver exit handler.
	    */
	    a->acb_error_code = E_DM9858_ACP_ONLINE_BACKUP_EXIT;
	    STRUCT_ASSIGN_MACRO(db->dbcb_dcb.dcb_name, a->acb_errdb);
	    uleFormat(NULL, a->acb_error_code, (CL_ERR_DESC *)NULL, ULE_LOOKUP, 
		(DB_SQLSTATE *)NULL, a->acb_errtext, sizeof(a->acb_errtext), 
		&a->acb_errltext, &error, 1,
		DB_DB_MAXNAME, db->dbcb_dcb.dcb_name.db_db_name);

	    return (error_translate(E_DM9825_ARCH_INIT_DUMP, &a->acb_dberr));
	}

    } /* end if error opening journal file. */

    /*  Remember starting block sequence and that file has been opened. */

    db->dbcb_d_blkseq = blk_seq;
    db->dbcb_status |= DBCB_DUMP_FILE_OPEN;

    status = dm0c_close(config, DM0C_UPDATE, &a->acb_dberr);
    if (status != E_DB_OK)
    {
	/*
	** Fill in exit error information for archiver exit handler.
	*/
	a->acb_error_code = E_DM9858_ACP_ONLINE_BACKUP_EXIT;
	STRUCT_ASSIGN_MACRO(db->dbcb_dcb.dcb_name, a->acb_errdb);
	uleFormat(NULL, a->acb_error_code, (CL_ERR_DESC *)NULL, ULE_LOOKUP, 
	    (DB_SQLSTATE *)NULL, a->acb_errtext, sizeof(a->acb_errtext), 
	    &a->acb_errltext, &error, 1,
	    DB_DB_MAXNAME, db->dbcb_dcb.dcb_name.db_db_name);

	return (error_translate(E_DM9825_ARCH_INIT_DUMP, &a->acb_dberr));
    }

    return (E_DB_OK);
}   

/*{
** Name: copy_record_to_dump - Copy log record to dump file.
**
** Description:
**	This routine copies a log record to the dump file during online backup.
**
** Inputs:
**      acb                             Pointer to the ACB.
**      la				Pointer to log address of record
**	db				Pointer to the associated BKDBCB
**	header				Pointer to the record header
**	record				Pointer to the log record
**
** Outputs:
**      err_code                        The reason for error return status.
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
**	28-dec-1992 (jnash)
**	    Made into new function for the reduced logging project.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
*/
static DB_STATUS
copy_record_to_dump(
ACB		*acb,
DBCB		*db,
LG_LA		*la,
LG_RECORD	*header,
DM0L_HEADER	*record)
{
    ACB		    *a = acb;
    DB_STATUS	    status;
    CL_ERR_DESC	    sys_err;
    i4	    	    error;

    CLRDBERR(&a->acb_dberr);

    switch(record->type)
    {
	case   DM0LBI:

	    /*
	    ** Copy the log record to the dump file.
	    ** Update the address of the last dumped record and 
	    ** internal counters.
	    */
	    status = log_2_dump(a, db, header, record, la);
	    if (status != E_DB_OK)
		return (status);

	    db->dbcb_dmp_la = *la;
            db->dbcb_drec_cnt++;

	    break;

	/* 
	** These operations should not occur during an online checkpoint.
	*/
	case   DM0LCREATE:
	case   DM0LDESTROY:
	case   DM0LINDEX:
	case   DM0LMODIFY:
	case   DM0LRELOCATE:
	case   DM0LFCREATE:
	case   DM0LFRENAME:
	case   DM0LLOAD:
	case   DM0LSM0CLOSEPURGE:
	case   DM0LSM1RENAME:
	case   DM0LSM2CONFIG:

	    /*
	    ** Fill in exit error information for archiver exit handler.
	    */
	    a->acb_error_code = E_DM9858_ACP_ONLINE_BACKUP_EXIT;
	    STRUCT_ASSIGN_MACRO(db->dbcb_dcb.dcb_name, a->acb_errdb);
	    uleFormat(NULL, a->acb_error_code, (CL_ERR_DESC *)NULL, ULE_LOOKUP, 
		(DB_SQLSTATE *)NULL, a->acb_errtext, sizeof(a->acb_errtext),
		&a->acb_errltext, &error, 1,
		DB_DB_MAXNAME, db->dbcb_dcb.dcb_name.db_db_name);

	    SETDBERR(&a->acb_dberr, 0, E_DM9829_ARCH_BAD_LOG_TYPE);
	    return (error_translate(E_DM9826_ARCH_COPY_DUMP, &a->acb_dberr));

	default:
	    break;
    }

    return (E_DB_OK);
}

/*{
** Name: log_2_dump - write record to the Dump file.
**
** Description:
**	This routine is called to write Before Image and File Extend
**	records to the DUMP file, to be used in rollforward processing.
**
** Inputs:
**      acb                             The ACB.
**	db				The backup database control block.
**	header				The log record header.
**	record				The record to dump.
**	la				The log address of the record.
**
** Outputs:
**      err_code                        The reason for error return status.
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
**      6-jan-1989 (Edhsu)
**	    Created for Terminator
**      6-Oct-1989 (ac)
**	    Fixed the potential problem that log records are not moved to
**	    dmp files.
**	 8-jan-1990 (rogerk)
**	    Moved check for already-dumped records to copy_dump.
**	25-feb-1991 (rogerk)
**	    Added support for Archiver Stability project.  Fill in exit
**	    error information whan an ACP-Fatal error is encountered.
**	05-jan-1993 (jnash)
**	    Reduced logging project.  Open/create dump file if not
**	    already done. 
**	10-feb-1993 (jnash)
**	    Add FALSE param to dmd_log().
**	15-mar-1993 (rogerk)
**	    Reduced Logging - Phase IV:
**	    Took out code that used to copy the database and transaction ids to
**		the record header when copying records to the dump files.
**		These fields are now set correctly at the time the records
**		written to the log file.
**	    Fixed use of dump file sequence and block sequence numbers
**		(dbcb_d_filseq/dbcb_d_blkseq) that were being mixed up.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	24-may-1993 (andys)
**	    Pass appropriate logfile blocksize to dmd_log and dmd_format_log
**	    rather than using hardwired 4096.
**      06-mar-1996 (stial01)
**          Variable Page Size project: length field in log records is now
**          i4, so change call to dm0d_write()
*/
static DB_STATUS
log_2_dump(
ACB	    *acb,
DBCB	    *db,
LG_RECORD   *header,
DM0L_HEADER *record,
LG_LA	    *la)
{
    ACB		*a = acb;
    DB_STATUS	status;
    LG_LA	*tran_id;
    i4		error;
    char	jnlerr_db[DB_DB_MAXNAME+1];
    char	jnlerr_dir[DB_AREA_MAX+1];

    CLRDBERR(&a->acb_dberr);

#ifdef ACP_TRACE
    TRdisplay("%@ ACP LOG2_DUMP: Writing Log Record for DB %~t to Dump file.\n",
	sizeof(db->dbcb_dcb.dcb_name), &db->dbcb_dcb.dcb_name);
    TRdisplay("  (LA=(%d,%d,%d),PREV_LA=(%d,%d,%d),LEN=%d,XID=%x%x,ID=%x\n",
	la->la_sequence, la->la_block, la->la_offset,
	header->lgr_prev.la_sequence,
	header->lgr_prev.la_block,
	header->lgr_prev.la_offset,
	header->lgr_length, record->tran_id.db_high_tran, 
	record->tran_id.db_low_tran, record->database_id);
    dmd_log(FALSE, record, a->acb_lctx->lctx_bksz);
#endif

    /*
    ** If dump file not yet open, create and open it now.  
    */
    if (!(db->dbcb_status & DBCB_DUMP_FILE_OPEN))
    {
	status = init_dump(a, db, db->dbcb_d_filseq, db->dbcb_d_blkseq);
	if (status != E_DB_OK)
	{
	    TRdisplay("ACP LOG_2_DUMP: INIT_DUMP failed\n");
	    a->acb_error_code = E_DM985C_ACP_DMP_WRITE_EXIT;
	    STRUCT_ASSIGN_MACRO(db->dbcb_dcb.dcb_name, a->acb_errdb);
	    STncpy( jnlerr_db, db->dbcb_dcb.dcb_name.db_db_name, DB_DB_MAXNAME);
	    jnlerr_db[ DB_DB_MAXNAME ] = '\0';
	    STtrmwhite(jnlerr_db);
	    STncpy( jnlerr_dir, db->dbcb_d_path.name, DB_AREA_MAX);
	    jnlerr_dir[ DB_AREA_MAX ] = '\0';
	    STtrmwhite(jnlerr_dir);
	    uleFormat(NULL, a->acb_error_code, (CL_ERR_DESC *)NULL, ULE_LOOKUP, 
		(DB_SQLSTATE *)NULL, a->acb_errtext, sizeof(a->acb_errtext), 
		&a->acb_errltext, &error, 3,
		0, jnlerr_db, 0, jnlerr_dir, 0, ERx("create"));

	    return (error_translate(E_DM9827_ARCH_LOG2DUMP, &a->acb_dberr));
	}
    }

    status = dm0d_write(
       (DM0D_CTX *)db->dbcb_dm0d, (PTR)record, *(i4 *)record, &a->acb_dberr);

    /*
    ** Archiver test point - if archiver test flag specifies to simulate
    ** a disk full error, then act as though the dm0d_write call failed.
    */
    if (a->acb_test_flag & ACB_T_BACKUP_ERROR)
    {
	TRdisplay(
	    "%@ ACP LOG_2_DUMP: Simulating Disk Full Error on DMP device.\n");
	status = E_DB_ERROR;
	SETDBERR(&a->acb_dberr, 0, E_DM9362_DM0D_WRITE_ERROR);
    }

    if (status != E_DB_OK)
    {
	/*
	** Fill in exit error information for archiver exit handler.
	*/
	a->acb_error_code = E_DM985C_ACP_DMP_WRITE_EXIT;
	STRUCT_ASSIGN_MACRO(db->dbcb_dcb.dcb_name, a->acb_errdb);
	STncpy( jnlerr_db, db->dbcb_dcb.dcb_name.db_db_name, DB_DB_MAXNAME);
	jnlerr_db[ DB_DB_MAXNAME ] = '\0';
	STtrmwhite(jnlerr_db);
	STncpy( jnlerr_dir, db->dbcb_d_path.name, DB_AREA_MAX);
	jnlerr_dir[ DB_AREA_MAX ] = '\0';
	STtrmwhite(jnlerr_dir);
	uleFormat(NULL, a->acb_error_code, (CL_ERR_DESC *)NULL, ULE_LOOKUP, 
	    (DB_SQLSTATE *)NULL, a->acb_errtext, sizeof(a->acb_errtext), 
	    &a->acb_errltext, &error, 3,
	    0, jnlerr_db, 0, jnlerr_dir, 0, ERx("write to"));

	return (error_translate(E_DM9827_ARCH_LOG2DUMP, &a->acb_dberr));
    }
    return (E_DB_OK);

}

/*{
** Name: check_start_backup - Check Start Backup record for Online Checkpoint.
**
** Description:	
**	This routine is called when an SBACKUP record is encountered in the
**	scan phase - either in archive processing (dma_p1) or online backup
**	processing (dma_online_backup).
**
**	It checks to see if this is a valid SBACKUP record - that is that
**	the database it belongs to is undergoing Online Backup and the
**	log address matches the current backup.
**
**	This prevents us from processing records from already-completed
**	checkpoints, or from old failed checkpoints.
**
**	The current backup database list is searched, and if this database
**	is found we compare the SBACKUP log address with the one stored
**	in the dbcb.  This address is obtained from LGshow and is an address
**	which we know is LESS than the actual Start Backup record (it will
**	most likely not be equal to it).  If the passed in log address is
**	less than the dbcb's address then this is an SBACKUP record for
**	a previous backup of this database.
**
**	If the database is not in the current backup list, then we do an
**	LGshow to check the same information.
**
**	If the SBACKUP record proves to be invalid - the database is no
**	being backed up or the log address doesn't match - then we return
**	BKDB_INVALID.
**
**	If the SBACKUP record is valid and the database is already in the
**	backup database list, then we return BKDB_FOUND.
**
**	If the SBACKUP record is valid but the database is not in the backup
**	database list, then we return BKDB_EXIST.  The caller should probably
**	call update_backup_context to include this database in the backup list.
**
** Inputs:
**	acb				pointer to ACB structure
**      dbid				External database identifier.
**	la				log address of SBACKUP record.
**
** Outputs:
**	Returns:
**	    BKDB_INVALID		invalid db to backup
**	    BKDB_FOUND			valid db in ACP context
**	    BKDB_EXIST			valid db, need LG to update
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      6-jan-1989 (Edhsu)
**	    Created for Terminator
**	13-nov-1989 (rogerk)
**	    Added log address parameter.  Caller should pass in address of
**	    SBACKUP record to check.  This routine should make sure that the
**	    record does not belong to a previous checkpoint on the same db.
**	    Use database.db_id to verify correct database rather than
**	    unitialized id variable.
**	 8-jan-1990 (rogerk)
**	    If find new SBACKUP record while already processing an online
**	    backup, then deallocate the current context so a new one can
**	    be allocated.
**	26-apr-1990 (mikem)
**	    bug #21427
**	    LGshow() was being called with a "length" uninitialized.  In some
**	    cases at least on the sun4 this value sometimes was a large 
**	    negative number which would result in an AV.  The fix was to set
**	    "length = 0" before the LGshow loop.
**	25-feb-1991 (rogerk)
**	    Added support for Archiver Stability project.  Fill in exit
**	    error information whan an ACP-Fatal error is encountered.
**	15-mar-1993 (jnash & rogerk)
**	    Reduced Logging - Phase IV:
**	    Access dbcb via hash queue.  Changed to use lookup_dbcb.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
[@history_template@]...
*/
i4
check_start_backup(
ACB		*acb,
i4		dbid,
LG_LA		*la)
{
    ACB			*a = acb;
    DBCB		*db;
    DB_STATUS		status;
    LG_DATABASE		database;
    CL_ERR_DESC		sys_err;
    i4			error;
    i4		length;
    i4		id;

    CLRDBERR(&a->acb_dberr);

    /*
    **	Check if this is a valid db
    */
    lookup_dbcb(a, dbid, &db);
    if (db)
    {
	/*
	** Check if this database is known in the backup list and if this
	** SBACKUP record corresponds to the latest backup (If it occurs
	** previous to the SBACKUP record address held in the backup context
	** then it is to a previous backup and should be ignored).
	**
	** If we find a new SBACKUP record for a database which is already
	** undergoing dump processing (bk_cnt is non-zero), then deallocate
	** the current backup control block and reformat a new one for the
	** new backup context.  This situation could occur if an online
	** backup is interrupted during dump processing and a new backup
	** started immediately - before the ACP finishes processing the
	** old backup.
	*/
	if ((la->la_sequence > db->dbcb_sbackup.la_sequence) ||
	     (la->la_sequence == db->dbcb_sbackup.la_sequence &&
	      la->la_block >= db->dbcb_sbackup.la_block) ||
	     (la->la_sequence == db->dbcb_sbackup.la_sequence &&
	      la->la_block == db->dbcb_sbackup.la_block &&
	      la->la_offset >= db->dbcb_sbackup.la_offset))
	{
	    if (db->dbcb_drec_cnt > 0)
	    {
		TRdisplay("%@ ACP CHECK_START_BACKUP: Start Backup record foundfor database %~t.\n",
		    sizeof(db->dbcb_dcb.dcb_name), &db->dbcb_dcb.dcb_name);
		TRdisplay("%@ ACP CHECK_START_BACKUP:   Database is already undergoing Online Backup processing.\n");
		TRdisplay("%@ ACP CHECK_START_BACKUP:   The old backup will be cancelled and a new one started.\n");

		/* XXXXX NOT DONE */
	    }
		
	    return (BKDB_FOUND);
	}
    }

    /*
    **	Check with LG if this is a newly added db
    */

    /*
    ** XXXX This could be done a lot more efficiently by looking up the
    ** internal id of the database (with LG_S_DBID) and then requesting
    ** information on that particular db.
    **
    ** On the other hand, I'm not really sure under what circumstances
    ** this routine can be called.  Can we really process records for a
    ** database that we did not know was open when we started the archive?
    ** I don't think so.  It may have started a backup after we started,
    ** but we would not encounter the SBACKUP unless we knew it was in
    ** backup when we called online_context.
    */

    /* length must be 0 at the beginning of the loop, else a garbage value
    ** gets passed into lg, which can result in a crash in the logging system.
    */
    length = 0;

    while (1)
    {
	status = LGshow(LG_N_DATABASE, (PTR)&database, sizeof(database),
	    &length, &sys_err);

	if (status != E_DB_OK)
	{
	    uleFormat(NULL, E_DM9017_BAD_LOG_SHOW, &sys_err, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, &error, 0);

	    /*
	    ** Fill in exit error information for archiver exit handler.
	    */
	    a->acb_error_code = E_DM9858_ACP_ONLINE_BACKUP_EXIT;
	    uleFormat(NULL, a->acb_error_code, (CL_ERR_DESC *)NULL, ULE_LOOKUP, 
		(DB_SQLSTATE *)NULL, a->acb_errtext, sizeof(a->acb_errtext), 
		&a->acb_errltext, &error, 1, 0, ERx("DB-UKNOWN"));

	    SETDBERR(&a->acb_dberr, 0, E_DM9809_ARCH_LOG_SHOW);
	    return (error_translate(E_DM981F_ARCH_CK_SBACKUP, &a->acb_dberr));
	}
	if (length == 0)
	    return (BKDB_INVALID);

	if ((database.db_status & DB_BACKUP) == 0 ||
	    ((database.db_status & DB_BACKUP) &&
	     (database.db_status & DB_FBACKUP)))
	    continue;

	/*
	** Check if this is the correct database and if this SBACKUP record
	** corresponds to the current Backup in progress on this database.
	** (The SBACKUP record for the current backup must occur after the
	** SBACKUP record address held in the backup context).
	*/
	if ((dbid == database.db_database_id) &&
	    ((la->la_sequence > database.db_sbackup.la_sequence) ||
	     (la->la_sequence == database.db_sbackup.la_sequence &&
	      la->la_block >= database.db_sbackup.la_block) ||
	     (la->la_sequence == database.db_sbackup.la_sequence &&
	      la->la_block == database.db_sbackup.la_block &&
	      la->la_offset >= database.db_sbackup.la_offset)))
	{
	    /*
	    ** This is the correct database and SBACKUP record.  Break to
	    ** return BKDB_EXIST which will cause us to put this database
	    ** and backup context into the backup list.
	    */
	    break;
	}
    }

    return (BKDB_EXIST);
}

/*{
** Name: check_end_backup - Check End Backup record for Online Checkpoint.
**
** Description:
**	This routine is called when an End Backup record is encountered
**	during Online Backup processing.
**
**	It checks whether this EBACKUP record is the End Backup for a current
**	Online Checkpoint operation.
**
**	In order for it to be valid, the database must be known in the backup
**	database list and the log address of the EBACKUP record must be greater
**	than the SBACKUP address.
**
** Inputs:
**      acb                             The ACB.
**	dbid				Database Identifier
**	la				Log address of EBACKUP record.
**
** Outputs:
**	Returns:
**	    1				valid db to stop backup
**	    0				invalid db
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**       6-jan-1989 (Edhsu)
**	    Created for Terminator
**	13-nov-1989 (rogerk)
**	    Added log address parameter.  Caller should pass in address of
**	    EBACKUP record to check.  This routine should make sure that the
**	    record does not belong to a previous checkpoint on the same db.
**	15-mar-1993 (jnash & rogerk)
**	    Reduced Logging - Phase IV:
**	    Access dbcb via hash queue.  Changed to use lookup_dbcb.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
[@history_line@]...
[@history_template@]...
*/
static i4
check_end_backup(
ACB		*acb,
i4		dbid,
LG_LA		*la)
{
    ACB		*a = acb;
    DBCB	*db;

    /*
    **	Check if this is a valid db
    */
    lookup_dbcb(a, dbid, &db);
    if (db)
    {
	/*
	** Check if this database is known in the backup list and if this
	** EBACKUP record corresponds to the latest backup (it must occur
	** after the SBACKUP record address held in the backup context).
	*/
	if ((la->la_sequence > db->dbcb_sbackup.la_sequence) ||
	     (la->la_sequence == db->dbcb_sbackup.la_sequence &&
	      la->la_block > db->dbcb_sbackup.la_block) ||
	     (la->la_sequence == db->dbcb_sbackup.la_sequence &&
	      la->la_block == db->dbcb_sbackup.la_block &&
	      la->la_offset > db->dbcb_sbackup.la_offset))
	{
	    return (1);
	}
    }

    return (0);
}

/*{
** Name: dma_ebackup - Do end-of-cycle processing for online backup databases.
**
** Description:
**
** Inputs:
**
** Outputs:
**
** Side Effects:
**
** History:
**	18-jan-1993 (rogerk)
**	     Written for Reduced Logging Project.
**	22-feb-1993 (jnash)
**	     Access dbcb via hash queue.
*/
static DB_STATUS
dma_ebackup(
ACB	    *acb)
{
    ACB		*a = acb;
    ACP_QUEUE	*st;
    DBCB	*dbcb;
    STATUS	cl_status;
    CL_ERR_DESC	sys_err;
    i4		error;

    CLRDBERR(&a->acb_dberr);

    for (st = a->acb_db_state.stq_next;
	 st != (ACP_QUEUE *)&a->acb_db_state.stq_next;
	 st = dbcb->dbcb_state.stq_next)
    {
        dbcb = (DBCB *)((char *)st - CL_OFFSETOF(DBCB, dbcb_state));

	/* 
	** Check if journal switch done
	*/
	if (dbcb->dbcb_status & DBCB_JSWITCH)
	{
	    /*
	    ** Moved JSWITCH code to LG_archive_complete
	    ** JSWITCH requested BEFORE dma_online_context was done
	    ** JSWITCH requested AFTER dma_online_context was NOT done
	    ** 
	    ** No need for archiver to distinguish if JSWITCH was done or not.
	    ** JSP alterdb -next_jnl_file needs to verify if JSWITCH completed.
	    */
	    dbcb->dbcb_status &= ~DBCB_JSWITCH;
	}

	/*
	** If database is not undergoing Online Backup, the skip it.
	*/
	if ( ! (dbcb->dbcb_status & DBCB_BACKUP))
	    continue;

	/*
	** Check if the backup has failed - if it has, then signal the
	** failure to the Checkpoint process.
	*/
	if (dbcb->dbcb_status & DBCB_BCK_ERROR)
	{
	    uleFormat(NULL, E_DM9833_ARCH_BACKUP_FAILURE, &sys_err, ULE_LOG, 
		NULL, (char *)NULL, (i4)0, (i4 *)NULL, &error, 1, 
		sizeof(DB_DB_NAME), dbcb->dbcb_dcb.dcb_name);

	    cl_status = LGalter(LG_A_CKPERROR, (PTR)&dbcb->dbcb_int_db_id, 
		sizeof(dbcb->dbcb_int_db_id), &sys_err);
	    if (cl_status != E_DB_OK)
	    {
		uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
		uleFormat(NULL, E_DM900B_BAD_LOG_ALTER, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL,
		    &error, 1, 0, LG_A_CKPERROR);

		/*
		** Fill in exit error information for archiver exit handler.
		*/
		a->acb_error_code = E_DM9858_ACP_ONLINE_BACKUP_EXIT;
		STRUCT_ASSIGN_MACRO(dbcb->dbcb_dcb.dcb_name, a->acb_errdb);
		uleFormat(NULL, a->acb_error_code, (CL_ERR_DESC *)NULL, ULE_LOOKUP, 
		    (DB_SQLSTATE *)NULL, a->acb_errtext,
		    sizeof(a->acb_errtext), &a->acb_errltext, &error, 1,
		    DB_DB_MAXNAME, dbcb->dbcb_dcb.dcb_name.db_db_name);

		SETDBERR(&a->acb_dberr, 0, E_DM980D_ARCH_LOG_ALTER);
		return (error_translate(E_DM9844_ARCH_EOC, &a->acb_dberr));
	    }
	}
	else if (dbcb->dbcb_backup_complete)
	{
	    /*
	    ** If done with online backup, tell the logging system.
	    */
	    cl_status = LGalter(LG_A_FBACKUP, (PTR)&dbcb->dbcb_int_db_id, 
		sizeof(dbcb->dbcb_int_db_id), &sys_err);
	    if (cl_status != E_DB_OK)
	    {
		uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
		uleFormat(NULL, E_DM900B_BAD_LOG_ALTER, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL,
		    &error, 1, 0, LG_A_FBACKUP);

		SETDBERR(&a->acb_dberr, 0, E_DM980D_ARCH_LOG_ALTER);
		return (error_translate(E_DM9844_ARCH_EOC, &a->acb_dberr));

	    }

	    /*
	    ** Reset dbcb_backup_complete so not redone.
	    */
	    dbcb->dbcb_backup_complete = 0;
	}
    }

    return (E_DB_OK);
}

/*{
** Name: next_journal - 	Cycle to next journal file 
**
** Description:
**	This routine is called when a Start Backup record for a database which
**	is undergoing Online Backup is encountered during the copy phase of
**	journal processing.
**
**	The Start Backup record marks the place at which journal records begin
**	to be associated with the new checkpoint rather than the old one.
**
**	In this routine we close the old journal file and create a new journal
**	entry in the journal history.  We flag that a new journal needs to 
**	be created if another journal write is required for this database.
**
**	XX Old note, not deleted for documentation XX 
**	The Online Backup processing does not currently depend on this code
**	being executed - if it can guarantee that no archive window will
**	include journal records from before and after the Online Checkpoint.
**	This is done by signaling a CP and a PURGE operation just after writing
**	the Start Backup record.  The archiver is called to purge all records
**	up to the Start Backup record.  When the SBACKUP is encountered, the
**	old journal files will be closed, and the new entry created.  If no
**	journal processing is required, and the copy phase is skipped (or
**	terminated before getting to the SBACKUP) then this routine is not
**	called and this work is done in phase 3.
**
**	The online backup algorithms were designed to allow journal records
**	from before and after the checkpoint to be processed in the same
**	archive cycle.  However, there currently exists a problem with updating
**	the last journaled address.  When we close the old journal file we
**	update the config file to show that the journal records have been
**	flushed up to the archive stopping point.  This presents a problem that
**	must be resolved if we want to relax the current restrictions.
**	XXX End of old note XXX
**
** Inputs:
**      acb                             The ACB.
**	db				The journal db control block.
**	header				Log record header.
**	record				The SBACKUP record.
**
** Outputs:
**	err_code			Reason for error.
**	Returns:
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      25-Feb-1989 (Edhsu)
**	    Created for Terminator
**	31-Aug-1989 (ac)
**	    Fixed Cluster ONLINE BACKUP problems.
**	16-nov-1989 (rogerk)
**	    Took out code that reused the old journal file if it was empty.
**	    This would cause us to get bad JFwrite errors and effectively
**	    hose the installation.
**	12-Dec-1989 (ac)
**	    Don't create a new journal file if the current journal file is
**	    created for the current checkpoint.
**	14-dec-1989 (sandyh)
**	    Trace creation of journal files.
**	 8-jan-1990 (rogerk)
**	    Call new_jnl_entry to do the journal entry allocation.
**	    Took out code to reuse empty journal files as it seemed to not
**	    work.  We now make sure we do not mistakenly create emtpy journal
**	    files for the old checkpoint by signaling an Online Purge after
**	    starting the checkpoint.
**	17-jan-1990
**	    Call LGend to clean up transaction context if dm0l_bt call fails.
**	    This is no longer done in dm0l_bt.
**	24-jul-1990 (bryanp)
**	    Internal sanity check on config file journal sequence number before
**	    creating journal file.
**	25-feb-1991 (rogerk)
**	    Added support for Archiver Stability project.  Fill in exit
**	    error information whan an ACP-Fatal error is encountered.
**	27-dec-1992 (jnash)
**	    Reduced logging project.  Change routine name from acp_s_j_p2()
**	    to next_journal(), add changes to support new archiver design.
**	15-mar-1993 (rogerk)
**	    Added direction parameter to dm0j_open routine.
**	26-jul-1993 (rogerk)
**	    Changed to update config file to show that the database has been
**	    archived up to the last log record processed (acb_last_la) rather
**	    than to the stopping point (acb_stop_addr).  The former is more
**	    accurate and divorces the routine from weird dependencies about
**	    when the current archive cycle will be completed.
**      24-jan-1995 (stial01)
**          BUG 66473: pass log record to new_jnl_entry() 
*/
static DB_STATUS
next_journal(
ACB		*acb,
DBCB		*db,
LG_RECORD	*header,
DM0L_HEADER	*record)
{
    ACB			*a = acb;
    DB_STATUS		status;
    CL_ERR_DESC		sys_err;
    DM0C_CNF		*config, *cnf;
    i4		cur_entry;
    i4		new_seq;
    i4		fil_seq;
    i4		blk_seq;
    i4			error;
    char		jnlerr_db[DB_DB_MAXNAME+1];
    char		jnlerr_dir[DB_AREA_MAX+1];
    bool		jnl_empty;

    CLRDBERR(&a->acb_dberr);

    if (a->acb_status & ACB_CLUSTER)
        return (E_DB_OK);

    /*
    ** We have encountered an SBACKUP record.  This marks the spot at
    ** which we begin writing journal records for the new checkpoint rather
    ** than the old one.
    **
    ** We will close the current journal file (which belongs to the previous
    ** checkpoint) and open a new journal file.  We will also update the
    ** config file to show the new journal sequence for this checkpoint.
    */

    TRdisplay("%@ ACP NEXT_JOURNAL: Switching Journal file for database %~t as part of Online Backup Processing.\n",
	sizeof(db->dbcb_dcb.dcb_name), &db->dbcb_dcb.dcb_name);

    /*
    ** Add a new entry to the journal history array.
    ** Record in the dbcb that we have done this so it won't be done again.
    */
    status = new_jnl_entry(a, &db->dbcb_dcb,
			(DM0L_SBACKUP *)record); /* BUG 66473 */
    if (DB_FAILURE_MACRO(status))
	return (error_translate(E_DM9831_ARCH_JWRITE, &a->acb_dberr));

    /*
    ** If new_jnl_entry returns that the entry had already been created
    ** by a previous pass of the archiver, then just return without
    ** closing any journal files.
    ** 
    ** XXX Still correct?  What about failures between this point and
    ** the creation of the journal file, below?
    ** XXX
    */
/*
    if (status == E_DB_INFO)
    {
	return (E_DB_OK);
    }
*/

    /*
    ** Need to flush and close the old journal file.
    ** Do dm0j_update to get the last block sequence written.
    */
    
    write_jnl_eof(acb, db, (DM0J_CTX *)db->dbcb_dm0j, (DM0C_CNF *)0, 0, 0);
    status = dm0j_update((DM0J_CTX *)db->dbcb_dm0j, &new_seq, &a->acb_dberr);
    if (status != E_DB_OK)
    {
	/*
	** Fill in exit error information for archiver exit handler.
	*/
	a->acb_error_code = E_DM9858_ACP_ONLINE_BACKUP_EXIT;
	STRUCT_ASSIGN_MACRO(db->dbcb_dcb.dcb_name, a->acb_errdb);
	uleFormat(NULL, a->acb_error_code, (CL_ERR_DESC *)NULL, ULE_LOOKUP, 
	    (DB_SQLSTATE *)NULL, a->acb_errtext, sizeof(a->acb_errtext), 
	    &a->acb_errltext, &error, 1,
	    DB_DB_MAXNAME, db->dbcb_dcb.dcb_name.db_db_name);

	return (error_translate(E_DM9831_ARCH_JWRITE, &a->acb_dberr));
    }

#ifdef NOTUSED
    /*
    ** It is possible to make a check here to see if the just-closed journal
    ** file is empty - and if it is, to scavange it away from the old checkpoint
    ** and use it for the current checkpoints.
    **
    ** This is not done here since currently we always create a journal file
    ** for each checkpoint of a journaled database - even if there are no
    ** records to put in it.  We even check for this condition in checkpoint
    ** and create an empty journal file if none exists.
    **
    **** THIS IS NOT TRUE IN A CLUSTER SITUATION!!! In a cluster, we do NOT
    **** pre-create individual cluster node journal files. If a particular node
    **** on the cluster performs no update activity on the database, it will
    **** NOT get a journal file created.
    **
    ** If this requirement were relaxed, we may want to restore this check.
    ** If no journal records have been written for a database (and no journal
    ** file creaetd), it is likely that Online Checkpoint will cause a new
    ** empty journal file to be created for the old checkpoint before getting
    ** to this routine to create a journal file for the new checkpoint.  This
    ** is because if there are journal records to write at the start of the
    ** copy phase which includes this SBACKUP record, then we create a new
    ** journal file for the current checkpoint (which is the old checkpoint
    ** since we have not yet excecuted this routine) even though the updates
    ** were logged after the SBACKUP record.
    **
    ** To scavange empty journal files, we would need to do the following:
    **
    **	   - Check the sequence number returned by dm0j_flush to see if the
    **	     file is empty.
    **
    **	   - Decrement the journal sequence number as it was incremented in
    **	     in new_jnl_entry.
    **
    **	   - Update the previous entry in the journal history to not include
    **	     the empty journal file.
    **
    **	   - Update the current entry in the journal history to include the
    **	     empty journal file.
    */
    jnl_empty = (/* check block sequence returned from dm0j_update */ FALSE);

    /*
    ** If the old journal file is empty, then leave it open and update
    ** the previous journal history to not include it.
    */
    if (jnl_empty)
    {
	/*
	** Open the config file to update the journal history.
	*/
	status = dm0c_open(&db->dbcb_dcb, 0, a->acb_lk_id, &config, &a->acb_dberr);
	if (status != E_DB_OK)
	{
	    /*
	    ** Fill in exit error information for archiver exit handler.
	    */
	    a->acb_error_code = E_DM9858_ACP_ONLINE_BACKUP_EXIT;
	    STRUCT_ASSIGN_MACRO(db->dbcb_dcb.dcb_name, a->acb_errdb);
	    uleFormat(NULL, a->acb_error_code, (CL_ERR_DESC *)NULL, ULE_LOOKUP, 
		(DB_SQLSTATE *)NULL, a->acb_errtext, sizeof(a->acb_errtext), 
		&a->acb_errltext, &error, 1,
		DB_DB_MAXNAME, db->dbcb_dcb.dcb_name.db_db_name);

	    return (error_translate(E_DM9831_ARCH_JWRITE, &a->acb_dberr));
	}
	cnf = config;
	cur_entry = cnf->cnf_jnl->jnl_count - 1;

	/* Decrement the file sequence to restore its previous value. */
	cnf->cnf_jnl->jnl_fil_seq--;

	if (cur_entry > 0)
	{
	    TRdisplay(
		"%@ ACP NEXT_JOURNAL: Prev Jnl Entry F_JNL %d L_JNL %d Now %d\n",
		cnf->cnf_jnl->jnl_history[cur_entry - 1].ckp_f_jnl,
		cnf->cnf_jnl->jnl_history[cur_entry - 1].ckp_l_jnl,
		cnf->cnf_jnl->jnl_fil_seq - 1);

	    /*
	    ** Decrement the previous journal entry sequence count.
	    ** If this leaves no journal files, then reset counts to zero.
	    */
	    cnf->cnf_jnl->jnl_history[cur_entry - 1].ckp_l_jnl--;
	    if (cnf->cnf_jnl->jnl_history[cur_entry - 1].ckp_l_jnl <
		    cnf->cnf_jnl->jnl_history[cur_entry - 1].ckp_f_jnl)
	    {
		cnf->cnf_jnl->jnl_history[cur_entry - 1].ckp_f_jnl = 0;
		cnf->cnf_jnl->jnl_history[cur_entry - 1].ckp_l_jnl = 0;
	    }
	}

	/*
	** Update the current journal history entry.
	*/
	cnf->cnf_jnl->jnl_history[cur_entry].ckp_f_jnl =
	    cnf->cnf_jnl->jnl_fil_seq;
	cnf->cnf_jnl->jnl_history[cur_entry].ckp_l_jnl =
	    cnf->cnf_jnl->jnl_fil_seq;

	status = dm0c_close(config, DM0C_UPDATE | DM0C_UPDATE_JNL, &a->acb_dberr);
	if (status != E_DB_OK)
	{
	    /*
	    ** Fill in exit error information for archiver exit handler.
	    */
	    a->acb_error_code = E_DM9858_ACP_ONLINE_BACKUP_EXIT;
	    STRUCT_ASSIGN_MACRO(db->dbcb_dcb.dcb_name, a->acb_errdb);
	    uleFormat(NULL, a->acb_error_code, (CL_ERR_DESC *)NULL, ULE_LOOKUP, 
		(DB_SQLSTATE *)NULL, a->acb_errtext, sizeof(a->acb_errtext), 
		&a->acb_errltext, &error, 1,
		DB_DB_MAXNAME, db->dbcb_dcb.dcb_name.db_db_name);

	    return (error_translate(E_DM9831_ARCH_JWRITE, &a->acb_dberr));
	}

	return (E_DB_OK);
    }
#endif /* NOTUSED */

    /*
    ** Close the current journal file.
    */
    status = dm0j_close((DM0J_CTX **)&db->dbcb_dm0j, &a->acb_dberr);
    if (status != E_DB_OK)
    {
	/*
	** Fill in exit error information for archiver exit handler.
	*/
	a->acb_error_code = E_DM9858_ACP_ONLINE_BACKUP_EXIT;
	STRUCT_ASSIGN_MACRO(db->dbcb_dcb.dcb_name, a->acb_errdb);
	uleFormat(NULL, a->acb_error_code, (CL_ERR_DESC *)NULL, ULE_LOOKUP, 
	    (DB_SQLSTATE *)NULL, a->acb_errtext, sizeof(a->acb_errtext), 
	    &a->acb_errltext, &error, 1,
	    DB_DB_MAXNAME, db->dbcb_dcb.dcb_name.db_db_name);

	return (error_translate(E_DM9831_ARCH_JWRITE, &a->acb_dberr));
    }

    TRdisplay("%@ ACP NEXT_JOURNAL: Database: %~t Jnl record cnt: %d\n",
	sizeof(db->dbcb_dcb.dcb_name), &db->dbcb_dcb.dcb_name,
	db->dbcb_jrec_cnt);
    TRdisplay("\tOld Jnl block sequence: %d, new: %d\n", 
	db->dbcb_j_blkseq, new_seq);

    /* 
    ** Open config file to update journal history
    */
    status = dm0c_open(&db->dbcb_dcb, 0, a->acb_lk_id, &config, &a->acb_dberr);
    if (status != E_DB_OK)
    {
	/*
	** Fill in exit error information for archiver exit handler.
	*/
	a->acb_error_code = E_DM9858_ACP_ONLINE_BACKUP_EXIT;
	STRUCT_ASSIGN_MACRO(db->dbcb_dcb.dcb_name, a->acb_errdb);
	uleFormat(NULL, a->acb_error_code, (CL_ERR_DESC *)NULL, ULE_LOOKUP, 
	    (DB_SQLSTATE *)NULL, a->acb_errtext, sizeof(a->acb_errtext), 
	    &a->acb_errltext, &error, 1,
	    DB_DB_MAXNAME, db->dbcb_dcb.dcb_name.db_db_name);

	return (error_translate(E_DM9831_ARCH_JWRITE, &a->acb_dberr));
    }

    cnf = config;

    /*
    ** Create the new journal file.
    */
    fil_seq = cnf->cnf_jnl->jnl_fil_seq;
    blk_seq = cnf->cnf_jnl->jnl_blk_seq;
    cur_entry = cnf->cnf_jnl->jnl_count - 1;

    TRdisplay("%@ ACP NEXT_JOURNAL: Create journal file sequence number %d at location %t.\n", 
	fil_seq, db->dbcb_lj_path, &db->dbcb_j_path);

    if (cnf->cnf_jnl->jnl_fil_seq > 0)
    {
	status = dm0j_create(0, (char *)&db->dbcb_j_path, db->dbcb_lj_path,
		    cnf->cnf_jnl->jnl_fil_seq, cnf->cnf_jnl->jnl_bksz,
		    cnf->cnf_jnl->jnl_blkcnt, a->acb_node_id, 
		    (DB_TAB_ID *)NULL, &a->acb_dberr);
    }
    else
    {
	/*  Sanity check violation -- config file is corrupt */

	TRdisplay("%@ ACP NEXT_JOURNAL: Can't create journal file zero.\n");
	status = E_DB_ERROR;
	SETDBERR(&a->acb_dberr, 0, E_DM9839_ARCH_JNL_SEQ_ZERO);
    }
    if (status != OK)
    {
	/*
	** Fill in exit error information for archiver exit handler.
	** On dm0j_create error, report out-of-disk-space.
	*/
	STRUCT_ASSIGN_MACRO(db->dbcb_dcb.dcb_name, a->acb_errdb);
	if (a->acb_dberr.err_code != E_DM9839_ARCH_JNL_SEQ_ZERO)
	{
	    STncpy( jnlerr_db, db->dbcb_dcb.dcb_name.db_db_name, DB_DB_MAXNAME);
	    jnlerr_db[ DB_DB_MAXNAME ] = '\0';
	    STtrmwhite(jnlerr_db);
	    STncpy( jnlerr_dir, db->dbcb_j_path.name, DB_AREA_MAX);
	    jnlerr_dir[ DB_AREA_MAX ] = '\0';
	    STtrmwhite(jnlerr_dir);

	    a->acb_error_code = E_DM9852_ACP_JNL_WRITE_EXIT;
	    uleFormat(NULL, a->acb_error_code, (CL_ERR_DESC *)NULL, ULE_LOOKUP,
		NULL, a->acb_errtext, sizeof(a->acb_errtext),
		&a->acb_errltext, &error, 3,
		0, jnlerr_db, 0, jnlerr_dir, 0, ERx("create"));
	}
	else
	{
	    a->acb_error_code = E_DM9858_ACP_ONLINE_BACKUP_EXIT;
	    uleFormat(NULL, a->acb_error_code, (CL_ERR_DESC *)NULL, ULE_LOOKUP, 
		NULL, a->acb_errtext, sizeof(a->acb_errtext), 
		&a->acb_errltext, &error, 1,
		DB_DB_MAXNAME, db->dbcb_dcb.dcb_name.db_db_name);
	}

	return (error_translate(E_DM9831_ARCH_JWRITE, &a->acb_dberr));
    }

    status = dm0j_open(0, (char *)&db->dbcb_j_path, db->dbcb_lj_path, 
	&db->dbcb_dcb.dcb_name, cnf->cnf_jnl->jnl_bksz, fil_seq, 
	cnf->cnf_jnl->jnl_ckp_seq, blk_seq + 1, DM0J_M_WRITE, 
	a->acb_node_id, DM0J_FORWARD, 
	(DB_TAB_ID *)NULL, (DM0J_CTX **)&db->dbcb_dm0j, &a->acb_dberr);
    if (status != E_DB_OK)
    {
	TRdisplay("%@ ACP NEXT_JOURNAL: Error opening file sequence number %d at location %t.\n",
	    fil_seq, db->dbcb_lj_path, &db->dbcb_j_path);

	/*
	** Fill in exit error information for archiver exit handler.
	*/
	a->acb_error_code = E_DM9858_ACP_ONLINE_BACKUP_EXIT;
	STRUCT_ASSIGN_MACRO(db->dbcb_dcb.dcb_name, a->acb_errdb);
	uleFormat(NULL, a->acb_error_code, (CL_ERR_DESC *)NULL, ULE_LOOKUP, 
	    NULL, a->acb_errtext, sizeof(a->acb_errtext), 
	    &a->acb_errltext, &error, 1,
	    DB_DB_MAXNAME, db->dbcb_dcb.dcb_name.db_db_name);

	return (error_translate(E_DM9831_ARCH_JWRITE, &a->acb_dberr));
    }

    /*
    ** Update the current journal history entry to show the new journal file.
    ** If cluster installation, set the journal node information.
    */
    cnf->cnf_jnl->jnl_history[cur_entry].ckp_f_jnl = fil_seq;
    cnf->cnf_jnl->jnl_history[cur_entry].ckp_l_jnl = fil_seq;
    if (CXcluster_enabled())
	cnf->cnf_jnl->jnl_node_info[a->acb_node_id].cnode_fil_seq = fil_seq;

    /*
    ** Update the current jnl address as we are closing the current journal
    ** file.  Normally, this is done at the end of the consistency point rather
    ** than here in the middle.  However, since we are creating a new
    ** journal file, we need to remember which log records we have
    ** successfully written so we will not reprocess them if we crash.
    */
    cnf->cnf_jnl->jnl_la = a->acb_last_la;

    status = dm0c_close(config, DM0C_UPDATE | DM0C_UPDATE_JNL, &a->acb_dberr);
    if (status != E_DB_OK)
    {
	/*
	** Fill in exit error information for archiver exit handler.
	*/
	a->acb_error_code = E_DM9858_ACP_ONLINE_BACKUP_EXIT;
	STRUCT_ASSIGN_MACRO(db->dbcb_dcb.dcb_name, a->acb_errdb);
	uleFormat(NULL, a->acb_error_code, (CL_ERR_DESC *)NULL, ULE_LOOKUP, 
	    (DB_SQLSTATE *)NULL, a->acb_errtext, sizeof(a->acb_errtext), 
	    &a->acb_errltext, &error, 1,
	    DB_DB_MAXNAME, db->dbcb_dcb.dcb_name.db_db_name);

	return (error_translate(E_DM9831_ARCH_JWRITE, &a->acb_dberr));
    }
 
    /*  
    ** Remember starting block sequence.
    */
    db->dbcb_j_blkseq = 0;

    return (E_DB_OK);
}

/*{
** Name: new_jnl_entry - create new journal history entry.
**
** Description:
**	This routine creates a new entry in the config journal history
**	so new journal records will go to a new journal file.
**
**	It is called processing when an SBACKUP record is encountered.  
**	Before the SBACKUP record, journal records continue to be 
**	written to the old journal files.  After the SBACKUP
**	record, updates are logged to new journal files.
**
**	The config file is checked first to see if the journal entry has
**	already been added for this checkpoint - to make sure that the
**	Start Backup record is not processed twice for the same checkpoint.
**	(This happens sometimes when the SBACKUP record is encountered
**	in a normal archive action before the backup is complete).  If
**	the current journal entry is for the current checkpoint sequence
**	then just return E_DB_INFO.
**
**	The routine only updates the config file information - it does not
**	actually create a new journal file.
**
** Inputs:
**      acb                             The ACB.
**	dcb				Database control block.
**      record                          SBACKUP record
**
** Outputs:
**	Returns:
**	    E_DB_OK
**	    E_DB_INFO			Journal entry was already added.
**	    E_DB_ERROR			Error adding journal entry.
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	 5-jan-1990 (rogerk)
**	    Created for Terminator.
**	25-feb-1991 (rogerk)
**	    Added support for Archiver Stability project.  Fill in exit
**	    error information whan an ACP-Fatal error is encountered.
**      24-jan-1995 (stial01)
**          BUG 66473: check flags in log record to see if table checkpoint.
*/
static DB_STATUS
new_jnl_entry(
ACB		*acb,
DMP_DCB		*dcb,
DM0L_SBACKUP    *record)
{
    ACB			*a = acb;
    DB_STATUS		status;
    DM0C_CNF		*config, *cnf;
    i4		cur_entry;
    i4			error;
    i4			j;

    /*
    ** Open the config file for config updates.
    */
    status = dm0c_open(dcb, DM0C_PARTIAL, a->acb_lk_id, &config, &a->acb_dberr);
    if (status != E_DB_OK)
    {
	/*
	** Fill in exit error information for archiver exit handler.
	*/
	a->acb_error_code = E_DM9856_ACP_DB_ACCESS_EXIT;
	STRUCT_ASSIGN_MACRO(dcb->dcb_name, a->acb_errdb);
	uleFormat(NULL, a->acb_error_code, (CL_ERR_DESC *)NULL, ULE_LOOKUP, 
	    (DB_SQLSTATE *)NULL, a->acb_errtext, sizeof(a->acb_errtext), 
	    &a->acb_errltext, &error, 1, DB_DB_MAXNAME, dcb->dcb_name.db_db_name);

	return (error_translate(E_DM9835_ARCH_NEW_JNL_ERROR, &a->acb_dberr));
    }
    cnf = config;

    /*
    ** Check if the entry has already been added for this checkpoint.	
    */
    cur_entry = cnf->cnf_jnl->jnl_count - 1;
    if ((cur_entry >= 0) &&
	(cnf->cnf_jnl->jnl_history[cur_entry].ckp_sequence ==
						    cnf->cnf_jnl->jnl_ckp_seq))
    {
	status = dm0c_close(config, 0, &a->acb_dberr);
	if (status != E_DB_OK)
	{
	    /*
	    ** Fill in exit error information for archiver exit handler.
	    */
	    a->acb_error_code = E_DM9856_ACP_DB_ACCESS_EXIT;
	    STRUCT_ASSIGN_MACRO(dcb->dcb_name, a->acb_errdb);
	    uleFormat(NULL, a->acb_error_code, (CL_ERR_DESC *)NULL, ULE_LOOKUP, 
		(DB_SQLSTATE *)NULL, a->acb_errtext, sizeof(a->acb_errtext), 
		&a->acb_errltext, &error, 1, 
		DB_DB_MAXNAME, dcb->dcb_name.db_db_name);

	    return (error_translate(E_DM9835_ARCH_NEW_JNL_ERROR, &a->acb_dberr));
	}
	return (E_DB_INFO);
    }

    /*
    ** Allocate a new journal history entry and initialize it.
    ** There is no need to create a new journal file at this point.
    ** By bumping the sequence number we guarantee that the next time
    ** we need a journal file, we will create it.
    */

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
    cur_entry = cnf->cnf_jnl->jnl_count - 1;

    /*
    ** Format the new journal history entry.
    */
    cnf->cnf_jnl->jnl_history[cur_entry].ckp_sequence = 
						cnf->cnf_jnl->jnl_ckp_seq;
    cnf->cnf_jnl->jnl_history[cur_entry].ckp_f_jnl = 0;
    cnf->cnf_jnl->jnl_history[cur_entry].ckp_l_jnl = 0;
    cnf->cnf_jnl->jnl_history[cur_entry].ckp_jnl_valid = 0;
    cnf->cnf_jnl->jnl_history[cur_entry].ckp_mode = CKP_ONLINE;
    if (record->sbackup_header.flags & DM0L_TBL_CKPT)
	cnf->cnf_jnl->jnl_history[cur_entry].ckp_mode |= CKP_TABLE;
    TMget_stamp((TM_STAMP *)&cnf->cnf_jnl->jnl_history[cur_entry].ckp_date);

    /*
    ** Get journal file sequence number for this checkpoint.  Check
    ** first to see if the previous checkpoint used a journal file.
    ** If there is no previous checkpoint, or if the previous one did
    ** not create a journal file, then we do not actually have to
    ** increment the journal file count - we can just reuse the current
    ** sequence number.
    */
    if ((cur_entry != 0) && 
	(cnf->cnf_jnl->jnl_history[cur_entry - 1].ckp_f_jnl != 0))
    {
	cnf->cnf_jnl->jnl_fil_seq++;
	cnf->cnf_jnl->jnl_blk_seq = 0;
    }

    /*
    ** Ensure that the journal file sequence number is at least 1.
    */
    if (cnf->cnf_jnl->jnl_fil_seq <= 0)
	cnf->cnf_jnl->jnl_fil_seq = 1;

    TRdisplay("%@ ACP NEW_JNL_ENTRY: Create new journal entry in Config File - JNL_COUNT %d, FIL_SEQ %d\n", 
	cnf->cnf_jnl->jnl_count, cnf->cnf_jnl->jnl_fil_seq);

    /*
    ** Update the cluster node information.
    */
    if (CXcluster_enabled())
    {
	for (j = 0; j < DM_CNODE_MAX; j++)
	{
	    cnf->cnf_jnl->jnl_node_info[j].cnode_fil_seq = 0;
	    cnf->cnf_jnl->jnl_node_info[j].cnode_blk_seq = 0;
	}
    }

    /*  Update and close the configuration file. */

    status = dm0c_close(config, DM0C_UPDATE | DM0C_UPDATE_JNL, &a->acb_dberr);
    if (status != E_DB_OK)
    {
	/*
	** Fill in exit error information for archiver exit handler.
	*/
	a->acb_error_code = E_DM9856_ACP_DB_ACCESS_EXIT;
	STRUCT_ASSIGN_MACRO(dcb->dcb_name, a->acb_errdb);
	uleFormat(NULL, a->acb_error_code, (CL_ERR_DESC *)NULL, ULE_LOOKUP, 
	    (DB_SQLSTATE *)NULL, a->acb_errtext, sizeof(a->acb_errtext), 
	    &a->acb_errltext, &error, 1, DB_DB_MAXNAME, dcb->dcb_name.db_db_name);

	return (error_translate(E_DM9835_ARCH_NEW_JNL_ERROR, &a->acb_dberr));
    }

    return (E_DB_OK);
}

/*
** Name: acp_test_record	- handle archiver test record
**
** Description:
**	This routine processes DM0LTEST records.
**
**	These records are used to drive test actions in the archiver.
**
**	Most test log records are ignored unless the TEST_ENABLE log
**	record has been previously encountered by the archiver.  This
**	allows tests to avoid permanently corrupting the log file by
**	executing using the test log records as follows:
**
**	    1) Write TEST_ENABLE test log record.
**	    2) Write COMPLETE_WORK test log record.
**	    3) Signal Consistency Point.
**	    4) Write desired test record (eg. ACCESS_VIOLATE).
**
**	    Eventually, the archiver will process the three test log records
**	    written.  Because we wrote a COMPLETE_WORK record followed by
**	    a Consistency Point, the archiver will finish off a phase of 
**	    archiving when it gets to the Consistency Point - updating all 
**	    databases that it is processing to show they have been journaled 
**	    up to that CP window.  It will then start a new archive phase and 
**	    trip the access violation - the intent of our test.
**
**	    When the archiver is restarted, it will NOT reprocess the
**	    TEST_ENABLE log record, since the database has already been
**	    processed up to the CP window signalled in step (3).  So when
**	    the ACCESS_VIOLATE test record is encountered again, the archiver
**	    will ignore it.
**
**	The following test operations are supported:
**
**	    TST101_ACP_TEST_ENABLE:	Enable archiver tests.
**	    TST102_ACP_DISKFULL:	Simulate disk full on next jnl write.
**	    TST103_ACP_BAD_JNL_REC:	Simulate bad journal record.
**	    TST104_ACP_ACCESS_VIOLATE:	Access violate.
**	    TST105_ACP_COMPLETE_WORK:	Break up archive work at the next CP.
**	    TST106_ACP_BACKUP_ERROR:	Simulate disk full in online backup.
**	    TST107_ACP_MEMORY_FAILURE:	Simulate out-of-memory.
**	    TST109_ACP_TEST_DISABLE:	Disable archiver tests.
**
** Inputs:
**	acb		- the main ACP control block
**	record		- log record
**	phase		- archiver phase : 1 or 2
**
** Outputs:
**	none
**
** Returns:
**	none
**
** History:
**	25-feb-1991 (rogerk)
**	    Added for Archiver Stability project tests.
**	25-mar-1991 (rogerk)
**	    Added phase flag to allow some tests to be tripped in phase 1
**	    of the archiver.
*/
static	VOID
acp_test_record(
ACB		*acb,
DM0L_HEADER	*record,
i4		phase)
{
    ACB		    *a = acb;
    DM0L_TEST	    *tst = (DM0L_TEST *)record;

    /*
    ** Process test records which are meaningful to phase 1 of the archiver.
    */
    if (phase == 1)
    {
	if (tst->tst_number == TST105_ACP_COMPLETE_WORK)
	    a->acb_test_flag |= ACB_T_COMPLETE_WORK;
	return;
    }

    /*
    ** Check for Test-enable status or log record.
    */
    if ((tst->tst_number == TST101_ACP_TEST_ENABLE) &&
	((a->acb_status & ACB_CSP) == 0))
    {
	a->acb_test_flag |= ACB_T_ENABLE;
	return;
    }

    if ((a->acb_test_flag & ACB_T_ENABLE) == 0)
	return;

    /*
    ** Process phase 2 test records.
    */
    switch (tst->tst_number)
    {
	case TST102_ACP_DISKFULL:
	    a->acb_test_flag |= ACB_T_DISKFULL;
	    break;

	case TST103_ACP_BAD_JNL_REC:
	    a->acb_test_flag |= ACB_T_BADREC;
	    break;

	case TST104_ACP_ACCESS_VIOLATE:
	    /*
	    ** Access violation test.
	    ** THIS WILL ACCESS VIOLATE.
	    */
	    TRdisplay("%@ ACP : Simulating Access Violation Error\n");
	    a = NULL;
	    a->acb_test_flag |= ACB_T_ENABLE;
	    break;

	case TST106_ACP_BACKUP_ERROR:
	    a->acb_test_flag |= ACB_T_BACKUP_ERROR;
	    break;

	case TST107_ACP_MEMORY_FAILURE:
	    a->acb_test_flag |= ACB_T_MEM_ALLOC;
	    break;

	case TST109_ACP_TEST_DISABLE:
	    a->acb_test_flag = 0;
	    break;
    }
}


/*{
** Name: dma_alloc_sidefile		- Build a new SIDEFILE
**
** Description:
**      This routine builds a SIDEFILE from passed-in record information and 
**	links it onto the DBCB.
**
** Inputs:
**	db_id				Database id of db to add
**	lg_db_id			Internal LG database id.
**      db_name				Pointer to db name
**      db_owner			Pointer to db owner
**      db_root				Pointer to db location
**      db_l_root			Length of location
**
** Outputs:
**	dbcb				Pointer to returned dbcb
**      err_code                        Reason for error status.
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
**      19-nov-2003 (stial01)
**          Created.
**      22-sep-2006 (wonca01) BUG 116694
**          Incorrect error string arguments were being passed
**          ule_format() for SIDEFILE related errors.
[@history_template@]...
*/
static DB_STATUS
dma_alloc_sidefile(
ACB		*a,
DBCB		*dbcb,
DM0L_BSF	*log_rec)
{
    DBCB	*db = dbcb;
    i4 		error;
    SIDEFILE	*sf;
    DB_STATUS	status;
    DM0C_CNF    *config;

    CLRDBERR(&a->acb_dberr);

    /* FIX ME journaling must be enabled for db or there might not be jnl dir */

    /*  Allocate and initialize a sidefile. */

    status = dm0m_allocate(sizeof(SIDEFILE), 0, SIDEFILE_CB, SIDEFILE_ASCII_ID, 
	(char *)a, (DM_OBJECT **)&sf, &a->acb_dberr);
    if (status != E_DB_OK)
    {
	SIZE_TYPE  total_memory;
	SIZE_TYPE  alloc_memory;
	SIZE_TYPE  free_memory;

	/*
	** Would be nice to build a dm0m_info call to return the amount
	** of memory which has been allocated so far.  This could be
	** logged here along with the amount of memory required.
	*/
	dm0m_info(&total_memory, &alloc_memory, &free_memory);
	uleFormat(NULL, E_DM9427_DM0M_MEMORY_ALLOCATE, (CL_ERR_DESC *)NULL, ULE_LOG,
	    (DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, (i4 *)NULL, &error,
	    4, 0, sizeof(SIDEFILE), 0, (i4)total_memory, 0, (i4)alloc_memory,
	    0, (i4)free_memory);

	/*
	** Fill in exit error information for archiver exit handler.
	*/
	a->acb_error_code = E_DM9853_ACP_RESOURCE_EXIT;
	STRUCT_ASSIGN_MACRO(db->dbcb_dcb.dcb_name, a->acb_errdb);
	uleFormat(NULL, a->acb_error_code, (CL_ERR_DESC *)NULL, ULE_LOOKUP,
	    (DB_SQLSTATE *)NULL, a->acb_errtext, sizeof(a->acb_errtext),
	    &a->acb_errltext, &error, 2,
	    0, ERx("memory"), 0, ERx("resources"));

	return (status);
    }

    /* FIX ME why do we have to open config to get jnl_bksz */
    status = dm0c_open(&db->dbcb_dcb, 0, a->acb_lk_id, &config, &a->acb_dberr);
    sf->sf_bksz = config->cnf_jnl->jnl_bksz;
    status = dm0c_close(config, 0, &a->acb_dberr);

    /* Create Sidefile */
    status = dm0j_create(0, (char *)&db->dbcb_j_path, 
		db->dbcb_lj_path, 0, sf->sf_bksz, 0,
		a->acb_node_id, &log_rec->bsf_tbl_id, &a->acb_dberr);

    if (status != E_DB_OK)
    {
	dm0m_deallocate((DM_OBJECT **)&sf);

	TRdisplay(
	    "%@ ACP DMA_SIDEFILE: %~t, %~t Create failed (%d)\n", 
		sizeof(DB_OWN_NAME), &log_rec->bsf_owner,
		sizeof(DB_TAB_NAME), &log_rec->bsf_name, a->acb_dberr.err_code); 
	uleFormat(NULL, E_DM9CA8_SIDEFILE_CREATE, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &error, 1,
		sizeof(DB_TAB_NAME), &log_rec->bsf_name); 
	return (E_DB_ERROR);
    }

    /* Open Sidefile */
    status = dm0j_open(0, (char *)&db->dbcb_j_path, 
		db->dbcb_lj_path, &db->dbcb_dcb.dcb_name, sf->sf_bksz,
		0, 0, 1, DM0J_M_WRITE, a->acb_node_id,
		DM0J_FORWARD, &log_rec->bsf_tbl_id, 
		(DM0J_CTX **)&sf->sf_dm0j, &a->acb_dberr);

    if (status != E_DB_OK)
    {
	dm0m_deallocate((DM_OBJECT **)&sf);

	TRdisplay(
	    "%@ ACP DMA_SIDEFILE: %~t, %~t Open failed (%d)\n", 
		sizeof(DB_OWN_NAME), &log_rec->bsf_owner,
		sizeof(DB_TAB_NAME), &log_rec->bsf_name, a->acb_dberr.err_code);
        uleFormat(NULL, E_DM9CA9_SIDEFILE_OPEN, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
                (char *)NULL, (i4)0, (i4 *)NULL, &error, 1,
                sizeof(DB_TAB_NAME), &log_rec->bsf_name);
	return (E_DB_ERROR);
    }

    /* Initialize SIDEFILE */
    STRUCT_ASSIGN_MACRO(*log_rec, sf->sf_bsf);
    sf->sf_error = 0;
    sf->sf_status = E_DB_OK;
    sf->sf_rec_cnt = 0;
    sf->sf_flags = (SIDEFILE_CREATE | SIDEFILE_OPEN);
    
    if (a->acb_verbose)
    {
	TRdisplay(
	    "%@ ACP DMA_SIDEFILE: %~t, %~t OPEN (%d,%d)\n", 
		sizeof(DB_OWN_NAME), &log_rec->bsf_owner,
		sizeof(DB_TAB_NAME), &log_rec->bsf_name,
		log_rec->bsf_tbl_id.db_tab_base, 
		log_rec->bsf_tbl_id.db_tab_index);
	TRdisplay("\tDatabase %~t\n",
	    sizeof(db->dbcb_dcb.dcb_name), &db->dbcb_dcb.dcb_name);
	TRdisplay("\t    Journal Window: (%d,%d,%d)..(%d,%d,%d)\n",
	    db->dbcb_first_jnl_la.la_sequence,
	    db->dbcb_first_jnl_la.la_block,
	    db->dbcb_first_jnl_la.la_offset,
	    db->dbcb_last_jnl_la.la_sequence,
	    db->dbcb_last_jnl_la.la_block,
	    db->dbcb_last_jnl_la.la_offset);
    }

    /* Add sf control block to queue */
    sf->sf_next = a->acb_sfq_next;
    sf->sf_prev = (SIDEFILE *)&a->acb_sfq_next;
    a->acb_sfq_next->sf_prev = sf;
    a->acb_sfq_next = sf;
    a->acb_sfq_count++;
    
#ifdef xxx
    /*
    ** Put SIDEFILE on hash bucket queue.
    */
    dbq = &acb->acb_dbh[db_id % ACB_DB_HASH];
    db->dbcb_next = *dbq;
    db->dbcb_prev = (DBCB *) dbq;
    if (*dbq)
        (*dbq)->dbcb_prev = db;
    *dbq = db;

    /*
    ** Insert SIDEFILE on active queue.
    */
    db->dbcb_state.stq_next = acb->acb_db_state.stq_next;
    db->dbcb_state.stq_prev = (ACP_QUEUE *)&acb->acb_db_state.stq_next;
    acb->acb_db_state.stq_next->stq_prev = &db->dbcb_state;
    acb->acb_db_state.stq_next = &db->dbcb_state;

    acb->acb_c_dbcb++;
#endif

    return(E_DB_OK);
}

static DB_STATUS
dma_write_sidefile(
ACB		*a,
DBCB		*dbcb,
DM0L_HEADER 	*record,
SIDEFILE	*sf)
{
    DBCB	*db = dbcb;
    DB_STATUS	status;
    i4		error;

    CLRDBERR(&a->acb_dberr);


    /* FIX ME if Sidefile write fails - only online modify needs
    ** to fail
    ** Ignore all subsequent writes to this SF
    ** Erase sidefile?? online modify will get error reading it ??
    */
    if (sf->sf_status)
	return(E_DB_ERROR);
    /*
    ** Add the record to journal for the database. 
    */
    status = dm0j_write((DM0J_CTX *)sf->sf_dm0j, (PTR)record,
	*(i4 *)record, &a->acb_dberr);

    if (status != E_DB_OK)
    {
	TRdisplay(
	    "%@ ACP DMA_SIDEFILE: %~t, %~t Write failed (%d)\n", 
		sizeof(DB_OWN_NAME), &sf->sf_bsf.bsf_owner,
		sizeof(DB_TAB_NAME), &sf->sf_bsf.bsf_name, a->acb_dberr.err_code);
	sf->sf_status = status;
	sf->sf_error = a->acb_dberr.err_code;
    }
    else
    {
	sf->sf_flags |= SIDEFILE_HAS_RECORDS;
	sf->sf_rec_cnt++;
	if (a->acb_verbose)
	    TRdisplay(
	        "%@ ACP DMA_SIDEFILE: %~t, %~t WRITE type %d\n", 
		sizeof(DB_OWN_NAME), &sf->sf_bsf.bsf_owner,
		sizeof(DB_TAB_NAME), &sf->sf_bsf.bsf_name, record->type);
    }
    return (E_DB_OK);
}

static DB_STATUS
dma_close_sidefile(
ACB		*a,
DBCB		*dbcb,
DM0L_ESF	*log_rec)
{
    DBCB	*db = dbcb;
    i4 		error;
    SIDEFILE	*sf;
    SIDEFILE	*sfcand;
    DB_STATUS	status = E_DB_OK;
    i4		jnl_sequence;
    i4		local_error;
    DB_STATUS   local_status;
    DM0L_HEADER *header = (DM0L_HEADER *)log_rec;
    DB_ERROR	local_dberr;

    CLRDBERR(&a->acb_dberr);

    /* locate sf control block in queue */
    for (sfcand = a->acb_sfq_next; 
	 sfcand != (SIDEFILE *)&a->acb_sfq_next;
	 sfcand = sfcand->sf_next)
    {
        if ( (((DM0L_HEADER *)log_rec)->database_id == 
				sfcand->sf_bsf.bsf_header.database_id) &&
             (log_rec->esf_tbl_id.db_tab_base == 
				sfcand->sf_bsf.bsf_tbl_id.db_tab_base) &&
             (log_rec->esf_tbl_id.db_tab_index == 
				sfcand->sf_bsf.bsf_tbl_id.db_tab_index) )
        {
            sf = sfcand;
	    break;
        }
    }
    if (!sf)
    {
	uleFormat(NULL, E_DM9CAA_ARCH_SFCB_NOTFOUND, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &error, 2,
		sizeof(u_i4), log_rec->esf_tbl_id.db_tab_base,
		sizeof(u_i4), log_rec->esf_tbl_id.db_tab_index);
	uleFormat(NULL, E_DM9CAC_SIDEFILE_CLOSE, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &error, 1,
		sizeof(DB_TAB_NAME), log_rec->esf_name);
	return (E_DB_ERROR); 
    }

    if (sf->sf_status != E_DB_OK)
    {
	uleFormat(NULL, sf->sf_error, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &sf->sf_error, 0);
	uleFormat(NULL, E_DM9CAB_ARCH_BAD_SIDEFILE, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &error, 3,
		sizeof(u_i4), sf->sf_bsf.bsf_tbl_id.db_tab_base,
		sizeof(u_i4), sf->sf_bsf.bsf_tbl_id.db_tab_index,
		sizeof(i4), sf->sf_flags);
	uleFormat(NULL, E_DM9CAC_SIDEFILE_CLOSE, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &error, 1,
		sizeof(DB_TAB_NAME), &log_rec->esf_name);
	return (sf->sf_status);
    }

    if (sf->sf_rec_cnt && (sf->sf_flags & SIDEFILE_OPEN) && sf->sf_dm0j)
    {
	status = dm0j_update((DM0J_CTX *)sf->sf_dm0j, &jnl_sequence, 
	    &a->acb_dberr);
    }

    if ((sf->sf_flags & SIDEFILE_OPEN) && sf->sf_dm0j)
    {
	local_status = dm0j_close((DM0J_CTX **)&sf->sf_dm0j, &local_dberr);
	if (local_status != E_DB_OK)
	{
	    uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		(char * )NULL, 0L, (i4 *)NULL, &local_error, 0);
	    uleFormat(NULL, E_DM9CAC_SIDEFILE_CLOSE, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &error, 1,
		sizeof(DB_TAB_NAME), log_rec->esf_name);
	    status = local_status;
	}
    }

    if (status != E_DB_OK)
	TRdisplay(
	    "%@ ACP DMA_SIDEFILE: %~t, %~t Close failed (%d)\n", 
		sizeof(DB_OWN_NAME), &sf->sf_bsf.bsf_owner,
		sizeof(DB_TAB_NAME), &sf->sf_bsf.bsf_name, a->acb_dberr.err_code);
    else if (a->acb_verbose)
	TRdisplay(
	    "%@ ACP DMA_SIDEFILE: %~t, %~t CLOSE (%d records)\n", 
		sizeof(DB_OWN_NAME), &sf->sf_bsf.bsf_owner,
		sizeof(DB_TAB_NAME), &sf->sf_bsf.bsf_name, sf->sf_rec_cnt);

    /* Remove sf control block from queue */
    sf->sf_prev->sf_next = sf->sf_next;
    sf->sf_next->sf_prev = sf->sf_prev;
    a->acb_sfq_count--;
	
    /* deallocate sf control block */
    dm0m_deallocate((DM_OBJECT **)&sf);

    return (status);
}

static DB_STATUS
dma_filter_sidefile(
ACB		*a,
DBCB		*dbcb,
DM0L_HEADER 	*record,
SIDEFILE	**ret_sf)
{
    DBCB	*db = dbcb;
    DB_TAB_ID   *tabid;
    bool	sf_write;
    SIDEFILE	*sfcand;
    SIDEFILE	*sf = 0;
    i4		error;

    CLRDBERR(&a->acb_dberr);

    /*
    ** Given DM0LPUT, DM0LDEL, DM0LREP
    ** See if tabid in log record matches tabid for Sidefile
    ** If not, ignore this record
    */
    switch (record->type)
    {
	case DM0LPUT: tabid = &((DM0L_PUT *)record)->put_tbl_id; break;
	case DM0LDEL: tabid = &((DM0L_DEL *)record)->del_tbl_id; break;
	case DM0LREP: tabid = &((DM0L_REP *)record)->rep_tbl_id; break;
	case DM0LDEALLOC: tabid = &((DM0L_DEALLOC *)record)->dall_tblid; break;
    }

    /* locate sf control block in queue */
    for (sfcand = a->acb_sfq_next; 
	 sfcand != (SIDEFILE *)&a->acb_sfq_next;
	 sfcand = sfcand->sf_next)
    {
        if ( (record->database_id == sfcand->sf_bsf.bsf_header.database_id) &&
	     (tabid->db_tab_base == sfcand->sf_bsf.bsf_tbl_id.db_tab_base) &&
	     ( (tabid->db_tab_index == 
			sfcand->sf_bsf.bsf_tbl_id.db_tab_index) ||
	       (tabid->db_tab_index < 0) ) ) 
	{
	    *ret_sf = sf = sfcand;
	    break;
	}
    }

    if (!sf)
	return(E_DB_ERROR);

    if (sf->sf_status != E_DB_OK)
    {
        uleFormat(NULL, sf->sf_error, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
                (char *)NULL, (i4)0, (i4 *)NULL, &sf->sf_error, 0);
        uleFormat(NULL, E_DM9CAB_ARCH_BAD_SIDEFILE, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
                (char *)NULL, (i4)0, (i4 *)NULL, &error, 3,
                sizeof(u_i4), tabid->db_tab_base, sizeof(u_i4), tabid->db_tab_index,
		sizeof(i4), sf->sf_flags);
	uleFormat(NULL, E_DM9CAD_ARCH_FILTER_SIDEFILE, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	return (sf->sf_status);
    }

    if ((sf->sf_flags & SIDEFILE_OPEN) == 0)
    {
        uleFormat(NULL, E_DM9CAB_ARCH_BAD_SIDEFILE, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
                (char *)NULL, (i4)0, (i4 *)NULL, &error, 3,
                sizeof(u_i4), tabid->db_tab_base, sizeof(u_i4), tabid->db_tab_index,
                sizeof(i4), sf->sf_flags);
	uleFormat(NULL, E_DM9CAD_ARCH_FILTER_SIDEFILE, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	return (E_DB_ERROR);
    }

    if (a->acb_verbose)
	TRdisplay(
	    "%@ ACP DMA_SIDEFILE: %~t, %~t FILTER (%d,%d) sf (%d,%d)\n", 
		sizeof(DB_OWN_NAME), &sf->sf_bsf.bsf_owner,
		sizeof(DB_TAB_NAME), &sf->sf_bsf.bsf_name, 
		tabid->db_tab_base, tabid->db_tab_index,
		sf->sf_bsf.bsf_tbl_id.db_tab_base, 
		sf->sf_bsf.bsf_tbl_id.db_tab_index);

    return (E_DB_OK);
}


/*{
** Name:  write_jnl_eof	- Write DM0LJNLSWITCH record to journal.
**
** Description:
**      This routine writes a DM0LJNLSWITCH record to the current journal
**      before the next journal is opened.
**      Currently the only purpose of this journal record is so that 
**      incremental rollforwarddb can verify incremental journal files.
**
** Inputs:
**      acb                             Pointer to ACB.
**      db                              Database context    
**      openjnl                         Open jnl context (if current jnl open)
**      config                          (if current jnl not open)
**      jnl_seq                         (if current jnl not open)
**      blk_seq                         (if current jnl not open)
**
** Outputs:
**      err_code                        Reason for error status.
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
**      04-mar-2008 (stial01)
**          Created (for incremental rollforwarddb)
*/
static DB_STATUS
write_jnl_eof(
ACB	    *acb,
DBCB	    *db,
DM0J_CTX    *openjnl,
DM0C_CNF    *config,
i4	    jnl_seq,
i4	    blk_seq)

{
    ACB			*a = acb;
    DM0J_CTX		*jnl = NULL;
    char		line_buffer[132];
    DB_STATUS		status;
    i4			i;
    DM0L_HEADER		*record;
    i4			l_record;
    i4			jnl_sequence; 
    DM0L_JNL_SWITCH	rec;
    DB_ERROR		local_dberr;

    if (jnl_seq < 1)
	return (E_DB_OK);

    /*
    ** Need to flush and close the old journal file.
    ** Do dm0j_update to get the last block sequence written.
    ** First write DM0LJNLSWITCH log record for incremental rollforwarddb
    ** tran_id, lsn are ingored for this log record
    */
    MEfill(sizeof(rec), 0, (char *)&rec);
    rec.js_header.length = sizeof(DM0L_JNL_SWITCH);
    rec.js_header.type = DM0LJNLSWITCH;
    rec.js_header.database_id = db->dbcb_ext_db_id; 
    TMget_stamp((TM_STAMP *)&rec.js_time);
    if (openjnl)
    {
	status = dm0j_write(openjnl, (PTR)&rec, *(i4 *)&rec, &local_dberr);
	TRdisplay("%@ Write eof for database %~t to journal %d,%d jnlctx %x\n", 
	    sizeof(DB_DB_NAME), &db->dbcb_dcb.dcb_name, jnl_seq,
	    openjnl->dm0j_sequence, openjnl);
	return (E_DB_OK);
    }

    for (;;)
    {
	status = dm0j_open(0, (char *)&db->dbcb_j_path, db->dbcb_lj_path, 
			&db->dbcb_dcb.dcb_name, config->cnf_jnl->jnl_bksz,
			jnl_seq, config->cnf_jnl->jnl_ckp_seq,
			blk_seq + 1, DM0J_M_WRITE, a->acb_node_id,
			DM0J_FORWARD, (DB_TAB_ID *)0, 
			&jnl, &local_dberr);

	if (status != E_DB_OK)
	    break;
	    

	status = dm0j_write(jnl, (PTR)&rec, *(i4 *)&rec, &local_dberr);
	if (status)
	    break;
	TRdisplay("%@ Write eof for database %~t to journal %d,%d jnlctx %x\n", 
	    sizeof(DB_DB_NAME), &db->dbcb_dcb.dcb_name, jnl_seq,
	    jnl->dm0j_sequence, openjnl);

	status = dm0j_update(jnl, &jnl_sequence, &local_dberr);
	if (status)
	    break;

	status = dm0j_close(&jnl, &local_dberr);
	if (status)
	    break;
	
	return (E_DB_OK);
    }

    TRdisplay("%@ ERROR Writing eof for database %~t to jnl %d, jnlctx %x error %d\n", 
	sizeof(DB_DB_NAME), &db->dbcb_dcb.dcb_name, jnl_seq, openjnl, local_dberr.err_code);

    /* Error handling */
    if (jnl)
	status = dm0j_close(&jnl, &local_dberr);

    return (E_DB_ERROR);
}
