/*
** Copyright (c)  2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <di.h>
#include    <cs.h>
#include    <me.h>
#include    <pc.h>
#include    <tm.h>
#include    <st.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <tr.h>
#include    <ulf.h>
#include    <adf.h>
#include    <lg.h>
#include    <lgdef.h>
#include    <cx.h>
#include    <lk.h>
#include    <lkdef.h>
#include    <scf.h>
#include    <dm.h>
#include    <dmf.h>
#include    <dmccb.h>
#include    <dmrcb.h>
#include    <dmtcb.h>
#include    <dmxcb.h>
#include    <dmucb.h>
#include    <dml.h>
#include    <dmp.h>
#include    <dmpp.h>
#include    <dmxe.h>
#include    <dmve.h>
#include    <dm0p.h>
#include    <dm0c.h>
#include    <dm1b.h>
#include    <dm0llctx.h>
#include    <dm0l.h>
#include    <dm0m.h>
#include    <dm2t.h>
#include    <dm2d.h>
#include    <dm2f.h>
#include    <dm2r.h>
#include    <dm0s.h>
#include    <dmfgw.h>
#include    <dmd.h>
#include    <dmftrace.h>

/**
**
**  Name: DMXE.C - Transaction handling services.
**
**  Description:
**      This file contains the functions that manage locking and logging
**	request needed to implement transactions.
**	to begin a transaction.
**
**          dmxe_abort - Abort a transaction.
**          dmxe_begin - Begin a transaction.
**	    dmxe_commit	- Commit a transaction.
**          dmxe_savepoint - Declare a savepoint.
**	    dmxe_writebt - Write begin transaction record.
**	    dmxe_secure - Prepare to commit a transaction.
**	    dmxe_resume	- Resume a willing commit transaction.
**
**
**  History:
**      13-dec-1985 (derek)    
**          Created new for Jupiter
**	30-sep-1988 (rogerk)
**	    Added dmxe_writebt record.  Added capability to delay writing the
**	    begin transaction record in dmxe_begin until the first write
**	    operation is done.
**      30-Jan-1989 (ac)
**          Added 2PC support.
**	17-mar-1989 (EdHsu)
**	    Added cluster online backup support.
**	15-may-1989 (rogerk)
**	    Check for LG_EXCEED_LIMIT return value from LGbegin in dmxe_begin.
**	    Don't convert DMF external errors (< E_DM_INTERNAL) to generic
**	    E_DM9500_DMXE_BEGIN error number.
**	15-aug-1989 (rogerk)
**	    Added support for Non-SQL Gateway.
**	11-sep-1989 (rogerk)
**	    Added handling of DM0L_DMU records during rollback.
**	25-sep-1989 (rogerk)
**	    Fix PASS_ABORT bug causing us to get invalid pages in the buffer
**	    manager during rcp recovery.  Inform buffer manager when rcp is
**	    signaled to take over recovery.
**	 2-oct-1989 (rogerk)
**	    Added handling of DM0L_FSWAP records during rollback.
**	16-oct-1989 (rogerk)
**	    When PASS_ABORT, toss all unused TCB's from cache in case there
**	    is a DMU operation to be aborted which requires invalidating
**	    TCB's in the cache.
**	29-dec-1989 (rogerk)
**	    Added call to LGshow to dmxe_begin() to check if database is 
**	    being backed up.  If it is, then set DCB_S_BACKUP status so DMF 
**	    will know to do special logging.
**	17-jan-1990 (rogerk)
**	    Added support for delaying savepoint records for read-only
**	    transactions.  Changed arguments to dmxe_writebt routine.
**	    Moved LGend call out of error handling in dm0l_bt.  The caller
**	    of dm0l_bt is now responsible for cleaning up the transaction.
**	06-apr-1990 (fred)
**	    Added support for SCS_SET_ACTIVITY'ing progress of abort.  This is
**	    used so that users can diagnose HUNG servers as being busy doing an
**	    abort.
**	 4-feb-1991 (rogerk)
**	    Added new argument to dm0p_toss_pages call.
**	 4-feb-1990 (rogerk)
**	    Added support for NOLOGGING in dmxe_writebt.  Don't write 
**	    log records in an XCB_NOLOGGING transaction.
**	25-feb-1991 (rogerk)
**	    Added DM0L_TEST log record type.  It is ignored in UNDO.
**	    Added as part of Archiver Stability project.
**	22-mar-1991 (bryanp)
**	    Fix B36630: created dm2d_check_db_backup_status() and
**	    dm2r_set_rcb_btbi_info() and added calls to them in dmxe_begin and
**	    dmxe_writebt().
**	25-mar-1991 (rogerk)
**	    Changed abort handling of Set Nologging transactions.  These
**	    will now raise an error which will cause us to mark the database
**	    inconsistent.
**	 6-may-1991 (rogerk)
**	    Fix two settings of err_code to use "=" instead of "==".  These
**	    were in the abort code when database inconsistent was encountered.
**	    This was causing us to not propogate the DB_INCONSISTENT state back
**	    to the caller correctly.
**	6-jun-1991 (rachael,bryanp)
**	    Added error e_dm9c07_out_of_locklists for error message project.
**	    This error will print in the errlog.log prior to the return of
**	    e_dm004b.
**	16-jul-1991 (bryanp)
**	    B38527: add support for DM0L_OLD_MODIFY & DM0L_OLD_INDEX &
**	    DM0L_SM0_CLOSEPURGE. The new SM0_CLOSEPURGE record is not currently
**	    used during UNDO processing, but I'm passing it to the DMVE layer
**	    anyway so as to be consistent with the other sysmod records, which
**	    ARE used by UNDO.
**	19-aug-1991 (rogerk)
**	    Added dmxe_check_abort routine to return the abort status of
**	    a transaction.  This was added as part of a bugfix for B39017.
**	    It allows the buffer manager to check the transaction state
**	    to avoid lock escalation while in abort processing.
**	17-oct-1991 (rogerk)
**	    Great Merge of DMF : Move 6.4 changes into 6.5.  In this case,
**	    the 6.4 version of this file was moved directly to 6.5 and the
**	    6.5 changes were merged back in:
**
**	     9-oct-1989 (Derek)
**		B1 changes: add CRDB log record.
**	    14-jun-1991 (Derek)
**		File extend project:
**		Add new file extend log record types : assoc, alloc, dealloc,
**		and extend.  Remove obsolete balloc, calloc, bpextend, and
**		bdealloc records.
**		Added new performance profiling hooks.
**		Added new on-line checkpoint protocol to reduce number of BI's
**		that need to be saved.
**		Added call to dm0p_wpass_abort to flush pages required
**		for pass-abort handling.
**      15-oct-1991 (mikem)
**          Eliminated unix compile warnings in dmxe_abort().
**	 3-nov-1991 (rogerk)
**	    Added DM0LOVFL log record to dmxe_abort, no action is taken 
**	    during UNDO.
**	21-jan-1992 (jnash)
**	    Add SES_30 tracepoint to generate pass abort within dmxe_abort.
**	14-may-1992 (bryanp)
**	    B38112: Write DM0L_DBINCONST record if DB is already inconsistent.
**	8-jul-1992 (walt)
**	    Prototyping DMF.
**      25-sep-1992 (nandak)
**          Copy the entire distributed transaction id.
**	26-oct-1992 (rogerk)
**	    Reduced Logging Project:  Added support for new recovery protocols:
**	      - Changed abort processing to follow new rollback protocols:
**		- Removed Before Image recovery.
**		- Added new log record types.
**		- Added support for CLR log records.
**		- Removed force/toss of pages during abort processing.
**	      - Added new dmxe_pass_abort routine to handle pass aborts.
**	      - Removed calls to dm0p_tran_invalidate.
**	      - Added clstatus error reporting to several routines.
**	14-dec-1992 (rogerk)
**	    Include dm1b.h before dm0l.h for prototype definitions.
**	08-feb-1993 (jnash)
**	    Pass journal status to dm0l_savepoint.  Cast ME calls to
**	    quiet compiler.  
**	10-feb-1993 (jnash)
**	    Add FALSE param to dmd_log().
**	30-feb-1993 (rmuth)
**	    Include di.h for dm0c.h
**	15-mar-1993 (rogerk & jnash)
**	    Reduced Logging - Phase IV:
**	    Renamed prev_lsn to compensated_lsn.
**	    Changed Abortsave and EndMini log records to written as CLR's.
**	        The compensated_lsn field is now used to skip backwards in
**		in the log file rather than using the log addresses in the
**		log records themselves.
**	    Removed dmxe_abort output parameter to return the Log Address of
**		the abortsave log record.
**	    Added dmve_logging field which indicates to write CLR's during
**		undo recovery.
**	    Also cast a couple of MEcopy() params to (char *) to quiet compiler.
**	    Changed LG_S_DBID show call in dmxe_resume.
**	    Removed obsolete rcb_bt_bi_logging and rcb_merge_logging fields.
**	    Deleted dm2r_set_rcb_btbi_info routine.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**		Pass correct transaction handle to dm0l_read.
**		Not all callers of dmxe_abort have an ODCB available. For
**		    example, the checkpoint process does not have an ODCB. I
**		    changed dmxe_abort to take both an ODCB AND a DCB parameter,
**		    and fixed the callers such that those that have an ODCB,
**		    pass the ODCB; those that have a DCB, pass the DCB, and
**		    dmxe_abort figures out the mess.
**	24-may-1993 (bryanp)
**	    Include <pc.h> before <lk.h> to pick up PID definition.
**	24-may-1993 (andys)
**	    Pass appropriate logfile blocksize to dmd_log and dmd_format_log
**	    rather than using hardwired 4096.
**	21-jun-1993 (bryanp)
**	    Add handling of LKrelease(LK_ALL) errors as a special case.
**	    Improve error message logging when dmxe_pass_abort fails.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (rogerk)
**	    Include respective CL headers for all cl calls.
**	23-aug-1993 (jnash)
**	    Don't journal savepoints.  Rollfoward figures out what to do 
**	    from LSN context.
**      10-sep-1993 (pearl)
**          In dmxe_abort(), add error DM0152_DB_INCONSIST_DETAIL if database
**          is inconsistent; this error will name the database in question.
**	20-sep-1993 (rogerk)
**	    Initialize dmve_rolldb_action list when dmve cb is initialized.
**	18-oct-1993 (rachael) Bug 52925
**	    An abort attempt in a set nologging transaction causes the database
**	    to be marked inconsistent.  The configuration file is opened and the
**	    inconsistency is recorded, but the logging system is not informed
**	    as is done when logging is being performed.  This means that
**	    other sessions will not be notified of the inconsistency when they
**	    write to the logging system.  The fix is to mark the LDB invalid
**	    for the database when the inconsistency is recorded in the
**	    configuration file.
**	18-oct-1993 (jnash)
**	    Issue error when pass abort performed.
**	18-oct-1993 (rogerk)
**	    Change inconsistent db error messages to not reference odcb as it
**	    may not be defined in this routine.
**	18-oct-93 (rogerk)
**	    Changed LG_A_OWNER interface in dmxe_resume so that it expects the 
**	    db_id of the caller's open database context.  We no longer scavange
**	    the opendb from the orphaned transaction, only the xact itself.
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          Added dmxe_xn_active()
**	27-Feb-1997 (jenjo02)
**	    Extended <set transaction statement> to support <transaction access mode>
**	    (READ ONLY | READ WRITE) per SQL-92.
**	    Removed call to dm0l_robt() which really isn't needed and just make
**	    the LGbegin() call here with the proper flags.
**      21-may-1997 (stial01)
**          Disallow PASS-ABORT for row locking transaction.
** 	17-jun-1997 (wonst02)
** 	    Changes for readonly database.
**      22-Aug-1997 (wonst02)
**          Added parentheses-- '==' has higher precedence than bitwise '&':
**	    (flag & DMXE_NOLOGGING) && ((flag & DMXE_ROTRAN) == 0)), not:
**	    (flag & DMXE_NOLOGGING) && (flag & DMXE_ROTRAN == 0))
**	25-Aug-1997 (jenjo02)
**	    Added log_id parm to dm0p_toss_pages(), dm0p_wpass_abort(),
**	    dm2t_reclaim_tcb() calls.
**	    Acquire the lctx_mutex after doing the log force for the transaction,
**	    instead of before it, to permit concurrent log forces.
**	11-Nov-1997 (jenjo02)
**	    Pass DM0L_DELAYBT to dm0l_bt() when writing a delayed BT.
**      08-Dec-1997 (hanch04)
**          Use new la_block field when block number is needed instead
**          of calculating it from the offset
**          Keep track of block number and offset into the current block.
**          for support of logs > 2 gig
**      31-Aug-1998 (wanya01)
**          Fix bug 77435: IPM session detail display shows "Performing Force
**          Abort Processing" as activity for session which is executing a
**          normal rollback.  Take out change 415103 which fix bug 62550.
**          Change 415103 wrongly set session activity to be "performing 
**          force abort processing" without checking the cause of abort.
**      22-Feb-1999 (bonro01)
**          Replace references to svcb_log_id with
**          dmf_svcb->svcb_lctx_ptr->lctx_lgid
**	15-Mar-1999 (jenjo02)
**	    Removed DM0L_DELAYBT references.
**      05-may-1999 (nanpr01,stial01)
**          dmxe_xn_active() Set err_code if LGshow fails
**	08-Sep-1999 (jenjo02)
**	    LKshow(LK_S_LLB_INFO) changed to LK_S_LIST. LK_S_LLB_INFO
**	    only returns info of lock list's related list, if any, causing
**	    erroneous E_DM93F5_ROW_LOCK_PROTOCOL errors because LK_LLB_INFO
**	    is garbage!
**	30-Nov-1999 (jenjo02)
**	    Added DB_DIS_TRAN_ID* to dmxe_begin(), LKcreate_list() prototypes.
**	15-Dec-1999 (jenjo02)
**	    Added DB_DIS_TRAN_ID parm to LGbegin() prototype.
**	    Removed DB_DIS_TRAN_ID parm from LKcreate_list() prototype.
**	    Added support for SHARED transactions.
**      18-jan-2000 (gupsh01)
**          Added support for setting transaction flag when the transaction
**          changed a non-journaled table for a journaled database.
**	14-Mar-2000 (jenjo02)
**	    Removed writing of SAVEPOINT and BMINI log records. 
**	03-Aug-2000 (jenjo02)
**	    Modified dmxe_abort() to support concurrent rollback for
**	    DAR 6075960.
**	24-Aug-2000 (jenjo02)
**	    When aborting a SHARED transaction in WILLING_COMMIT state,
**	    call (new) LKalter(LK_A_REASSOC) to prepare the SHARED
**	    locklist for the RCP.
**      01-feb-2001 (stial01)
**          Changed dmxe_xn_active args (Variable Page Type SIR 102955)
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	29-Mar-2001 (jenjo02)
**	    Added TRdisplay's of begin and end of rollback to assure
**	    QA that concurrent rollback occurs.
**	30-Apr-2001 (jenjo02)
**	    Reduce log clutter; only do those TRdisplays when trace
**	    point DM903 is set.
**	30-may-2001 (devjo01)
**	    Add cx.h for CXcluster_enabled(). s103715.
**	    Add dm0m.h for prototype access.
**      19-jan-2004 (horda03) Bug 111628
**          If E_DM9238 is reported, then wait a short while and try again.
**	30-Apr-2004 (schka24)
**	    Don't write BT, ET when doing shared begin/end.
**	    Don't attempt to force pages when doing shared commit.
**      31-aug-2004 (sheco02)
**          X-integrate change 468367 to main.
**  12-Jan-2005 (fanra01)
**      Bug 106465
**      Prevent 'to go' value in abort progress message from showing
**      negative values.
**	28-Jul-2005 (schka24)
**	    Back out x-integration for bug 111502 (not shown), doesn't
**	    apply to r3.
**	26-Jun-2006 (jonj)
**	    Deprecated dmxe_xa_abort().
**      29-nov-2005 (horda03) Bug 48659/INGSRV 2716
*          If the TX has updated non-journalled tables only, then write
**          a non-journalled ET.
**          Also, for a BT we may need to reserve 2 BT records, one
**          for the JNL and non-JNL's BT records.
**      14-nov-2006 (stial01) Bug 111628
**          If E_DM9238 is reported, reset status=E_DB_OK before retry.
**      12-apr-2007 (stial01)
**          dmxe_writebt() return error for readonly databases.
**      11-may-2007 (stial01)
**          dmxe_secure() Prepare all xn branches before calling dm0l_secure
**	18-Nov-2008 (jonj)
**	    SIR 120874: dm0c_?, dm2d_? functions converted to DB_ERROR *
**	21-Nov-2008 (jonj)
**	    SIR 120874: dmxe_? functions converted to DB_ERROR *, use
**	    new form uleFormat.
**	24-Nov-2008 (jonj)
**	    SIR 120874: dm0m_? functions converted to DB_ERROR *
**	25-Nov-2008 (jonj)
**	    SIR 120874: dm0l_? functions converted to DB_ERROR *
**	28-Nov-2008 (jonj)
**	    SIR 120874: dm2t_? functions converted to DB_ERROR *
**	05-Dec-2008 (jonj)
**	    SIR 120874: dmf_gw?, dm0p_? functions converted to DB_ERROR *
**/

/*{
** Name: dmxe_abort	- Abort transaction
**
** Description:
**      This routine implements the abort to savepoint or abort to
**	transaction primitives.
**
**	Some callers of this routine happen to have an ODCB control block
**	available. If available, the ODCB is the preferred control block to
**	pass to this routine, as it contains information about session context.
**
**	If this routine must be called outside of normal session context (for
**	example by the DMFCPP process), then the ODCB may not be available. In
**	this case, callers may pass the DCB control block. dmxe_abort will
**	attempt to figure out which is which and what is what, and will use the
**	information from whatever control block is available, WITH ONE
**	EXCEPTION:
**
**	    If dmxe_abort is being called outside of session context (i.e., when
**	    only the DCB is available), then gateway session aborting will not
**	    be performed. This is because gateway session aborting requires a
**	    session context.
**
**	I believe that in all cases where dmxe_abort is called with only the DCB
**	available, gateway session aborting is not actually required, since no
**	gateway updates were performed, so this should not cause any problems.
**
**
**	What happens when a transaction abort fails?
**	============================================
**
**	In most cases, if the transaction abort fails, we attempt a pass-abort.
**	Pass-abort is a prototocl in which the transaction is passed to the RCP
**	and the RCP makes another attempt to abort the transaction. After we
**	pass the transaction to the RCP, we return to our caller as though we
**	succeeded in aborting the transaction. Meanwhile, the RCP picks up the
**	transaction and tries again to abort it. Note that during the time that
**	the RCP is making this attempt, our caller may believe that the
**	transaction has already disappeared, while the logging and locking
**	system still know that this transaction exists, and that sometimes
**	leads to peculiar results.
**
**	There is one particular error condition which does NOT lead to a pass
**	abort. At the end of transaction abort processing, after we have logged
**	the DM0L_ET(ABORT) record, and after we have successfully released the
**	transaction context from the logging system, we then call the locking
**	system with LKrelease(LK_ALL) to release the transaction's lock list and
**	all of its locks. If this call should fail, we are in a quandary: we
**	cannot pass-abort the transaction, because the transaction is gone. If
**	we try to pass-abort the transaction, the logging system will complain.
**	However, we have this lock list out there which is holding some locks
**	which cannot be released. Since transaction lock list cleanup is only
**	performed as a result of completing the processing of the associated
**	transaction, it does no good to kill this server either, since server
**	failure recovery will still not clean up the lock list. In effect, we
**	have a "lock leak": this stranded lock list will remain in the locking
**	system until the system is shut down and restarted. We deal with this
**	failure by logging the locking system errors to the error log, but then
**	we simply return OK to our caller, and decide to proceed on about our
**	business. If the stranded locks cause concurrency problems, eventually
**	the user will end up having to shutdown and restart the entire
**	installation, but until then we may as well continue processing.
**
** Inputs:
**	odcb				Address of this session's Open Database
**					    Control Block. If dmxe_abort is
**					    being called outside of session
**					    context, and no ODCB is available,
**					    this pointer may be null.
**	dcb				Address of the Database Control Block,
**					    if known. If not known, pass NULL.
**      tran_id                         The transaction id to commit.
**	log_id				The log_id to use.
**      lock_id				The lock_id to use.
**	flag				Special flags: 	DMXE_JOURNAL | 
**					DMXE_ROTRAN | DMXE_WILLING_COMMIT.
**	sp_id				The savepoint id to look for.
**	savepoint_lsn			The Log Sequence Number of the
**					savepoint to which we are aborting (if
**					we are indeed aborting to a svpt).
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
**      21-jul-1986 (Derek)
**	    Created for Jupiter.
**      30-Jan-1989 (ac)
**          Added 2PC support.
**	15-aug-1989 (rogerk)
**	    Added support for Non-SQL Gateway.  If server is connected to a
**	    gateway which requires begin/end transaction notification, then
**	    we must notify the gateway of the abort.
**	11-sep-1989 (rogerk)
**	    Added handling of DM0L_DMU records during rollback.
**	25-sep-1989 (rogerk)
**	    Fix PASS_ABORT bug causing us to get invalid pages in the buffer
**	    manager during rcp recovery.  Inform buffer manager when rcp is
**	    signaled to take over recovery.  Also need to toss all non-locked
**	    pages in the buffer manager before signaling rcp recovery.  Any
**	    page belonging to the same database may be invalidated by the RCP.
**	    Check that sp_id was passed into dmxe_abort before breaking when
**	    find a savepoint id that matches it.
**	 2-oct-1989 (rogerk)
**	    Added handling of DM0L_FSWAP records during rollback.
**	16-oct-1989 (rogerk)
**	    When PASS_ABORT, toss all unused TCB's from cache in case there
**	    is a DMU operation to be aborted which requires invalidating
**	    TCB's in the cache.
**	25-feb-1991 (rogerk)
**	    Added DM0L_TEST log record type.  It is ignored in UNDO.
**	    Added as part of Archiver Stability project.
**	25-mar-1991 (rogerk)
**	    Changed abort handling of Set Nologging transactions.  These
**	    will now raise an error which will cause us to mark the database
**	    inconsistent.
**	 6-may-1991 (rogerk)
**	    Fix two settings of err_code to use "=" instead of "==".  This
**	    was causing us to not propogate the DB_INCONSISTENT state back
**	    to the caller correctly.
**	16-jul-1991 (bryanp)
**	    B38527: add support for DM0L_OLD_MODIFY & DM0L_OLD_INDEX &
**	    DM0L_SM0_CLOSEPURGE. The new SM0_CLOSEPURGE record is not currently
**	    used during UNDO processing, but I'm passing it to the DMVE layer
**	    anyway so as to be consistent with the other sysmod records, which
**	    ARE used by UNDO.
**	19-aug-1991 (rogerk)
**	    As long as I was touching this file, I tried to modify the while
**	    loop in the cleanup of abort processing which rendered "warning:
**	    statement not reached" errors on unix.
**	17-oct-1991 (rogerk)
**	    Great Merge of DMF : Move 6.4 changes into 6.5.  In this case,
**	    the 6.4 version of this file was moved directly to 6.5 and the
**	    6.5 changes were merged back in:
**
**	     9-oct-1989 (Derek)
**		B1 changes: add CRDB log record.
**	    14-jun-1991 (Derek)
**		File extend project:
**		Add new file extend log record types : assoc, alloc, dealloc,
**		and extend.  Remove obsolete balloc, calloc, bpextend, and
**		bdealloc records.
**		Added new performance profiling hooks.
**		Added new on-line checkpoint protocol to reduce number of BI's
**		that need to be saved.
**		Added call to dm0p_wpass_abort to flush pages required
**		for pass-abort handling.
**      15-oct-1991 (mikem)
**          Eliminated unix compile warnings ("warning: statement not reached")
**          by changing the way "while (E_DB_OK)" loops were coded.
**	 3-nov-1991 (rogerk)
**	    Added DM0LOVFL log record - no action is taken during UNDO.
**      21-jan-1992 (jnash)
**          Add SES_30 tracepoint to generate pass abort.
**	14-may-1992 (bryanp)
**	    B38112: Write DM0L_DBINCONST record if DB is already inconsistent.
**	26-oct-1992 (rogerk)
**	    Reduced Logging Project:  Added support for new recovery protocols:
**	      - Removed Before Image recovery.
**	      - Added support for CLR log records.  When a CLR is encountered,
**		we skip over already completed undo work back to the CLR's
**		prev_lsn address.
**	      - Took out FORCE argument to dm0p_force_pages called at the
**		end of the abort operation.  Abort updates are now cached
**		according to normal Fast Commit protocols.
**	      - Bypass dm0p_force_pages call if the abort is only to a svpt.
**	      - Took out call to dm0p_tran_invalidate made before starting
**		abort processing.  Pages can no longer be tossed before an
**		abort since BI recovery is not used.
**	      - Added error message indicating when a Pass Abort is signaled.
**	      - Moved Force Abort handling to new dmxe_pass_abort routine.
**		We now log force abort actions.  We now call dm0p_force_pages
**		instead of dm0p_tran_invalidate to release the pages needed
**		for the pass abort.
**	      - Moved switch statement with undo calls to dmve_undo routine.
**	      - Remove fswap processing.
**	      - Add new log record types: AI, BTDEL, BTPUT, BTSPLIT, BTOVFL,
**		BTFREE, BTUPDOVFL, DISASSOC, CRVERIFY, NOFULL, FMAP.
**	15-mar-1993 (jnash & rogerk)
**	    Reduced Logging - Phase IV:
**	    Renamed prev_lsn to compensated_lsn.
**	    Changed Abortsave and EndMini log records to written as CLR's.
**	        The compensated_lsn field is now used to skip backwards in
**		in the log file rather than using the log addresses in the
**		log records themselves.
**	    Added logic to allow multiple AbortSave log records to point
**		back to a common Savepoint.
**	    Removed output parameter to return the Log Address of the
**		abortsave log record.
**	    Added dmve_logging field which indicates to write CLR's during
**		undo recovery.
**	    Changed log record format: added database_id, tran_id and LSN
**		in the log record header.  The LSN of a log record is now
**		obtained from the record itself rather than from the dmve
**		control block.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**		Pass correct transaction handle to dm0l_read.
**		Not all callers of dmxe_abort have an ODCB available. For
**		    example, the checkpoint process does not have an ODCB. I
**		    changed dmxe_abort to take both an ODCB AND a DCB parameter,
**		    and fixed the callers such that those that have an ODCB,
**		    pass the ODCB; those that have a DCB, pass the DCB, and
**		    dmxe_abort figures out the mess.
**	24-may-1993 (andys)
**	    Pass appropriate logfile blocksize to dmd_log and dmd_format_log
**	    rather than using hardwired 4096.
**	21-jun-1993 (bryanp)
**	    Add handling of LKrelease(LK_ALL) errors as a special case.
**	    Improve error message logging when dmxe_pass_abort fails.
**      10-sep-1993 (pearl)
**          In dmxe_abort(), add error DM0152_DB_INCONSIST_DETAIL if database
**          is inconsistent; this error will name the database in question.
**	20-sep-1993 (rogerk)
**	    Initialize dmve_rolldb_action list field.
**	18-oct-1993 (rachael) Bug 52925
**	    An abort attempt in a set nologging transaction causes the database
**	    to be marked inconsistent.  The configuration file is opened and the
**	    inconsistency is recorded, but the logging system is not informed
**	    as is done when logging is being performed.  This means that
**	    other sessions will not be notified of the inconsistency when they
**	    write to the logging system.  The fix is to mark the LDB invalid
**	    for the database when the inconsistency is recorded in the
**	    configuration file.
**	18-oct-1993 (jnash)
**	    Issue error when pass abort performed.
**	18-oct-1993 (rogerk)
**	    Change inconsistent db error messages to not reference odcb as it
**	    may not be defined in this routine.
**	06-Nov-1994 (Bin Li)
**	    Fix bug 62550 regarding the inconsistency between exp.scf.scs.scb
**	    _activity and exp.scf.scs.scb_act_detail if encountering force_abort
**	    while running the query without commiting. Update the activity 
**	    within the following function call if force_abort is encountered 
**	    in order for two activities in sync.
**	01-aug-1996 (nanpr01)
**	    Clearout the "performing Force Abort Message " by setting
**	    the scf_blength to zero.
** 	17-jun-1997 (wonst02)
** 	    Changes for readonly database.
**	25-Aug-1997 (jenjo02)
**	    Acquire the lctx_mutex after doing the log force for the transaction,
**	    instead of before it, to permit concurrent log forces.
**	15-Dec-1999 (jenjo02)
**	    Support for SHARED transactions.
**	    Perform rollback only when this is the last (only)
**	    connection to the (shared) transaction.
**	    Test on the state of the SHARED transaction rather than this
**	    local one.
**      18-jan-2000 (gupsh01)
**          Added support for setting transaction flag when the transaction
**          changed a non-journaled table for a journaled database.
**	14-Mar-2000 (jenjo02)
**	    SAVEPOINT and BMINI log records no longer written. They are only
**	    needed for online recovery and we can do that just fine 
**	    without them.
**	03-Aug-2000 (jenjo02)
**	    Modified dmxe_abort() to support concurrent rollback for
**	    DAR 6075960.
**	24-Aug-2000 (jenjo02)
**	    When aborting a SHARED transaction in WILLING_COMMIT state,
**	    call (new) LKalter(LK_A_REASSOC) to prepare the SHARED
**	    locklist for the RCP.
**	29-Mar-2001 (jenjo02)
**	    Added TRdisplay's of begin and end of rollback to assure
**	    QA that concurrent rollback occurs.
**	30-Apr-2001 (jenjo02)
**	    Reduce log clutter; only do those TRdisplays when trace
**	    point DM903 is set.
**      22-oct-2002 (stial01)
**          Added dmxe_xa_abort
**	17-Apr-2003 (jenjo02)
**	    When checking NOLOGGING transaction, check DMXE_ROTRAN
**	    in addition to tr_status & TR_ACTIVE. STAR 12602247,
**	    INGSRV2225, BUG 110097.
**	5-Apr-2004 (schka24)
**	    Clear out new rawdata pointers in dmve.
**	30-aug-2004 (thaju02)
**	    Initialize dmve_flags.
**  12-Jan-2005 (fanra01)
**      lgr_sequence, used in the abort progress message, is defined in
**      LG_RECORD as an i2 but the intent is to utilize the full 32-bit
**      range.  Cast value to a u_i2 during conversion for display.
**	15-Aug-2005 (jenjo02)
**	    Implement multiple DMF LongTerm memory pools and
**	    ShortTerm session memory.
**	15-Mar-2006 (jenjo02)
**	    dm0l_force the log only through the transaction's last LSN,
**	    not the entire log.
**	11-Sep-2006 (jonj)
**	    Override the supplied tran_id with the one known to the
**	    logging system. In LOGFULL_COMMIT situations, it may
**	    be different than the one known externally.
**	19-Jan-2007 (kschendel)
**	    Always make the lock list noninterruptable, even for abort
**	    to savepoint.  (See inline for why.)
**	23-Feb-2007 (kschendel) b122040
**	    "ack" any pending interrupt when we start aborting.  This isn't
**	    a real answer to the row-lock pass abort thing, but it should
**	    help some;  if we're here, we don't need to be interrupted
**	    again.
**	8-Aug-2007 (kibro01) b118915
**	    Add in flag to allow lock release to take place after the
**	    commit, so that transaction temporary tables can keep their
**	    lock until they are destroyed.
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: If rollback to savepoint and the last
**	    LSN written by the transaction is the savepoint LSN,
**	    then do nothing.
**	04-Feb-2010 (jonj)
**	    SIR 121619 MVCC: Add bufid parameter to dm0l_read().
**	23-Sep-2010 (jonj) B124486
**	    If PASS_ABORT and DMXE_DELAY_UNLOCK, return E_DB_ERROR
**	    with E_DM9509_DMXE_PASS_ABORT to keep dmx_abort from
**	    releasing the lock list - the RCP needs it to
**	    recover the transaction.
*/
DB_STATUS
dmxe_abort(
DML_ODCB	    *odcb,
DMP_DCB		    *dcb,
DB_TRAN_ID          *TranId,
i4		    log_id,
i4		    lock_id,
i4		    flag,
i4		    sp_id,
LG_LSN		    *savepoint_lsn,
DB_ERROR	    *dberr)
{
    DMP_LCTX		*lctx = (DMP_LCTX*)NULL;
    LG_RECORD		*record_header;
    DMVE_CB		dmve;
    DB_STATUS		status = E_DB_OK;
    STATUS		stat = OK;
    CL_ERR_DESC		sys_err;
    DB_TAB_TIMESTAMP    ctime;
    LG_TRANSACTION	tr;
    LG_LSN		lsn;
    LG_LSN		clr_lsn;
    i4			i;
    i4			length;
    i4			et_flags = 0;
    i4			et_found = FALSE;
    i4			lctx_locked = FALSE;
    i4			jnl_et = FALSE;
    i4			abort_complete = FALSE;
    i4			clr_jump = FALSE;
    i4			count = 0;
    char		buffer[80];
    SCF_CB		scf_cb;
    int                 temp = TRUE;
    i4                  bad_read = 0;
    i4			clr_type;
    DB_TRAN_ID		*tran_id = TranId;
    DB_ERROR		local_dberr;
    i4			*err_code = &dberr->err_code;

    CLRDBERR(dberr);

    scf_cb.scf_length = sizeof(scf_cb);
    scf_cb.scf_type = SCF_CB_TYPE;
    scf_cb.scf_ascii_id = SCF_ASCII_ID;
    scf_cb.scf_facility = DB_DMF_ID;
    scf_cb.scf_session = DB_NOSESSION;
    scf_cb.scf_ptr_union.scf_buffer = (PTR) buffer;
    scf_cb.scf_nbr_union.scf_atype = SCS_A_MINOR;

    dmf_svcb->svcb_stat.tx_abort++;

    /*
    ** If the caller passed an ODCB but not a DCB, then figure out the DCB from
    ** the ODCB:
    */
    if (odcb != 0 && dcb == 0)
	dcb = odcb->odcb_dcb_ptr;

    /*
    ** If this is an Abort-to-Savepoint and we have made no updates in
    ** the transaction then there is no work to do.
    */
    if ((flag & DMXE_ROTRAN) && (sp_id))
	return (E_DB_OK);

    /*
    ** If this server is connected to a gateway, then notify the gateway
    ** of the abort.  If the gateway abort returns an error, then just
    ** continue processing the abort, as there is not much else we can do.
    ** If the ODCB is not available, then we know that no gateway updates
    ** could have been performed, so we can skip this (in fact, we MUST skip
    ** this, since we have no session pointer available). 
    */ 

    if (dmf_svcb->svcb_gw_xacts != 0 && odcb != 0) 
    { 
	status = dmf_gwx_abort(odcb->odcb_scb_ptr, dberr); 
        if (status != E_DB_OK) 
        { 
	    uleFormat(dberr, 0, &sys_err, ULE_LOG, NULL, 
	              (char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	    status = E_DB_OK;
	}
    }

    /*
    ** Ask the Logging System for information about the (SHARED) transaction we are
    ** recovering.
    */
    MEcopy((char *)&log_id, sizeof(log_id), (char *)&tr);
    stat = LGshow(LG_S_STRANSACTION, (PTR)&tr, sizeof(tr), &length, &sys_err);
    if ((stat != OK) || (length == 0))
    {
	if (stat)
	{
	    uleFormat(NULL, stat, &sys_err, ULE_LOG, NULL,
		(char *)NULL, 0L, (i4 *)NULL, err_code, 0);
	}
	uleFormat(NULL, E_DM9017_BAD_LOG_SHOW, &sys_err, ULE_LOG, NULL,
	    (char *)NULL, 0L, (i4 *)NULL, err_code, 1, 0, LG_S_STRANSACTION);
	SETDBERR(dberr, 0, E_DM9503_DMXE_ABORT);
	return (E_DB_ERROR);
    }

    /*
    ** If rollback to savepoint and the savepoint LSN is the last LSN
    ** written by the transaction, then there's nothing to do.
    */
    if ( sp_id && LSN_EQ(savepoint_lsn, &tr.tr_last_lsn) )
        return(E_DB_OK);

    /*
    ** Reference the tran_id as known by logging; LOGFULL_COMMIT
    ** may have changed it from what's known externally.
    */
    tran_id = &tr.tr_eid;

    /*
    ** If the DMXE_WILLING_COMMIT flag is passed, the distributed
    ** transaction is in a state in which it cannot be rolled back.
    **
    ** Disconnect it and let recovery take over.
    */
    if ( flag & DMXE_WILLING_COMMIT ) 
    {
	status = disconnect_willing_commit(tran_id, lock_id, log_id, dberr);
	if (status != E_DB_OK)
	{
	    if (dberr->err_code > E_DM_INTERNAL)
	    {
		uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, (char *)NULL,
		(i4)0, (i4 *)NULL, err_code, 0);
		SETDBERR(dberr, 0, E_DM9503_DMXE_ABORT);
	    }
	}

	/*
	** If our log transaction is a handle to a SHARED log transaction,
	** end our transaction and release our connection to the shared lock list.
	** The SHARED txn will be orphaned when all sharers have thus terminated.
	*/
	if ( tr.tr_status & TR_SHARED )
	{
	    stat = LGend(log_id, 0, &sys_err);
	    if (stat != OK)
	    {
		status = E_DB_ERROR;
		uleFormat(NULL, stat, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, 0L, (i4 *)NULL, err_code, 0);
		uleFormat(NULL, E_DM900E_BAD_LOG_END, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, 0L, (i4 *)NULL, err_code, 1, 0, log_id);
		SETDBERR(dberr, 0, E_DM9503_DMXE_ABORT);
	    }
	    else
	    {
		/*
		** Now handle the transaction's SHARED lock list. 
		** LK_A_REASSOC will deallocate this transaction's 
		** handle to the shared list. When all handles have
		** likewise been released, the SHARED lock list will
		** be un-SHARED and ready for recovery.
		*/
		stat = LKalter(LK_A_REASSOC, lock_id, &sys_err);

		if (stat != OK)
		{
		    status = E_DB_ERROR;
		    uleFormat(NULL, stat, &sys_err, ULE_LOG, NULL,
			(char *)NULL, 0L, (i4 *)NULL, err_code, 0);
		    uleFormat(NULL, E_DM9019_BAD_LOCK_ALTER, &sys_err, ULE_LOG, NULL,
			(char *)NULL, 0L, (i4 *)NULL, err_code, 1, 0, lock_id);
		    SETDBERR(dberr, 0, E_DM9503_DMXE_ABORT);
		}
	    }
	}
	return (status);
    }

    /*
    ** If the transaction is being ended, make its lock list non-interruptable
    ** to prevent Front-End interrupts from messing up abort handling.
    **
    ** We used to not do this for abort to savepoint, but that turned out to
    ** be a dreadful idea.  If an interrupt arrives while DMVE is trying to
    ** get a page lock, the dmve undo operation will likely fail, which
    ** will cause dmve to fall to the floor writhing in agony and refusing
    ** to do any more.
    ** What's worse, if the transaction has any row locks up to that point,
    ** we won't be able to pass-abort it, and the entire installation will
    ** crash horribly.
    */

    stat = LKalter(LK_A_NOINTERRUPT, lock_id, &sys_err);
    if (stat != OK)
    {
	uleFormat(NULL, stat, &sys_err, ULE_LOG, NULL,
	    (char *)NULL, 0L, (i4 *)NULL, err_code, 0);
	uleFormat(NULL, E_DM9019_BAD_LOCK_ALTER, &sys_err, ULE_LOG, NULL,
	    (char *)NULL, 0L, (i4 *)NULL, err_code, 0);
	SETDBERR(dberr, 0, E_DM9503_DMXE_ABORT);
	return (E_DB_ERROR);
    }
    /* Now that the lock-list is marked non-interruptable, clear any
    ** pending interrupt too.  This ensures that no lock request during
    ** the rollback gets "interrupted" status, which would cause a
    ** hissy-fit and a pass abort at best, or a DMF029 crash at worst.
    */
    CSintr_ack();

    /*
    ** If the Logging System tells us that the database is already inconsistent
    ** then we don't do any abort processing, we just log the unsuccessful
    ** completion of our transaction.
    */
    if (tr.tr_status & TR_DBINCONST)
    {
	/*
	** If we were supposed to abort this transaction, but failed to even
	** attempt the abort because the database was already inconsistent,
	** then record this information in the logfile by writing a "special"
	** ET record indicating that the transaction should be aborted but
	** the database is inconsistent. It's important to write the ET so that
	** the outcome of the transaction can be known to programs such as the
	** Archiver which scan the logfile attempting to match up BT and ET
	** log records -- if we just throw the transaction away without writing
	** an ET, the archiver gets confused (B38112).
	*/
        uleFormat(NULL, E_DM0152_DB_INCONSIST_DETAIL, (CL_ERR_DESC *)NULL, ULE_LOG,
                (DB_SQLSTATE *)0, (char *)NULL, (i4)0, (i4 *)NULL,
                err_code, 1, sizeof (dcb->dcb_name.db_db_name),
                dcb->dcb_name.db_db_name);
	if (sp_id == 0)
	{
	    if (flag & DMXE_JOURNAL)
		et_flags |= DM0L_JOURNAL;
	    /*
	    ** Check read-only state of SHARED txn, not just this handle.
	    **
	    ** if (flag & DMXE_ROTRAN)
	    */
	    if ( tr.tr_status & TR_INACTIVE )
		et_flags = DM0L_ROTRAN;
	    if (flag & DMXE_NONJNLTAB)
		et_flags |= DM0L_NONJNLTAB;

	    status = dm0l_et(log_id, et_flags, DM0L_ABORT | DM0L_DBINCONST,
			    0, 0, &ctime, dberr);
	    if (status != E_DB_OK)
		return (status);
	}
	SETDBERR(dberr, 0, E_DM0100_DB_INCONSISTENT);
	status = E_DB_ERROR;
    }

    /*
    ** Check for an abort attempt in a Set Nologging transaction,
    ** unless using a readonly database.
    */
    if ( status == E_DB_OK && dcb->dcb_access_mode != DCB_A_READ &&
	 flag & DMXE_NOLOGGING &&
	(tr.tr_status & TR_ACTIVE || (flag & DMXE_ROTRAN) == 0) )
    {
	DM0C_CNF            *cnf;

	/*
	** Abort attempted in Set Nologging transaction.
	**
	** Since the (SHARED) transaction was active (TR_ACTIVE) we
	** have no choice but to mark the database inconsistent.  Note that
	** if we try to fault over to the RCP here, the RCP will not mark
	** the database inconsistent as it does not believe the transaction
	** is really active (no log records have been written).
	*/

	/*
	** Open the config file and record the data inconsistency.
	*/
	status = dm0c_open(dcb, 0, lock_id, &cnf, &local_dberr);
	if (status == E_DB_OK)
	{
	    cnf->cnf_dsc->dsc_status &= ~(DSC_VALID | DSC_NOLOGGING);
	    cnf->cnf_dsc->dsc_inconst_time = TMsecs();
	    cnf->cnf_dsc->dsc_inconst_code = DSC_ERR_NOLOGGING;
	    status = dm0c_close(cnf, DM0C_UPDATE, &local_dberr);
	}
	if (status != E_DB_OK)
	{
	    uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG,
		(DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, (i4 *)NULL,
		err_code, 0);
	}


	status = LGalter(LG_A_DB, (PTR) &tr.tr_db_id, sizeof(tr.tr_db_id),
			&sys_err);

	if (status != OK)
	{
	    uleFormat(NULL, status, &sys_err, ULE_LOG, NULL,
		        (char *)NULL, 0L, (i4 *)NULL, err_code, 0);
	    uleFormat(NULL, E_DM900B_BAD_LOG_ALTER, &sys_err, ULE_LOG, 
			NULL, (char *)NULL, 0L, (i4 *)NULL, err_code, 1,
			0, LG_A_DB);

	}  

        uleFormat(NULL, E_DM0152_DB_INCONSIST_DETAIL, (CL_ERR_DESC *)NULL, ULE_LOG,
                (DB_SQLSTATE *)0, (char *)NULL, (i4)0, (i4 *)NULL,
                err_code, 1, sizeof (dcb->dcb_name.db_db_name),
                dcb->dcb_name.db_db_name);

	SETDBERR(dberr, 0, E_DM0100_DB_INCONSISTENT);
	status = E_DB_ERROR;
    }

    /*
    ** Execute abort recovery - either to the specified savepoint (if sp_id)
    ** or the entire transaction.
    */
    while (status == E_DB_OK)
    {
	/*
	** If the (SHARED) transaction performed no updates, then there is nothing
	** to rollback (and any attempt at an abort will fail since the BT
	** record will never be found.
	**
	** if (flag & DMXE_ROTRAN || dcb->dcb_access_mode == DCB_A_READ)
	*/

	/* If the (SHARED) transaction never became active, nothing to rollback */
	if ( tr.tr_status & TR_INACTIVE )
	    break;

	/* 
	** If aborting the entire transaction then register the abort with
	** the Logging System.  Among other things this prevents us from
	** being selected as a Force Abort victim.
	**
	** For shared transactions, delay recovery until all other handles
	** have ended. This is signaled by a return of LG_CONTINUE
	** from LGalter().
	*/
	if ( sp_id == 0 )
	{
	    stat = LGalter(LG_A_SERVER_ABORT, (PTR)&log_id, sizeof(log_id), 
		&sys_err);
	    if ( stat != OK && stat != LG_CONTINUE )
	    {
		uleFormat(NULL, stat, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, 0L, (i4 *)NULL, err_code, 0); 
		uleFormat(NULL, E_DM900B_BAD_LOG_ALTER, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, 0L, (i4 *)NULL, err_code, 1, 0, 
		    LG_A_SERVER_ABORT);
		SETDBERR(dberr, 0, E_DM9503_DMXE_ABORT);
		return (E_DB_ERROR);
	    }

	    if ( stat == OK )
	    {
		/* Pretend we found ET so we won't try to write one */
		et_found = TRUE;
		status = E_DB_OK;
		break;
	    }
	}

	/*
	** Ensure that all log records for this transaction have been
	** forced to the log file before we begin trying to read them.
	*/
	status = dm0l_force(log_id, &tr.tr_last_lsn, 0, dberr);
	if (status != E_DB_OK)
	    break;

	/*
	** First, attempt to allocate a private LCTX for this thread, 
	** using the server-wide LCTX as the model. Using a private
	** LCTX per thread permits concurrent recovery. If the memory
	** allocation fails, our fallback is to use the server-wide
	** LCTX to perform the recovery; to do that, we must mutex
	** it to block other threads from doing the same thing (this
	** is the old, single-threaded behavior).
	**
	** Get ShortTerm memory for LCTX.
	*/

	if ( (status = 
		dm0m_allocate(sizeof(DMP_LCTX) +
			dmf_svcb->svcb_lctx_ptr->lctx_l_area,
		    DM0M_SHORTTERM, LCTX_CB, LCTX_ASCII_ID, (char *)&lctx,
		    (DM_OBJECT **)&lctx, dberr)) == E_DB_OK )
	{
	    /* This private LCTX is an exact replica of svcb_lctx */
	    lctx->lctx_area = (char *)lctx + sizeof(DMP_LCTX);
	    lctx->lctx_l_area = dmf_svcb->svcb_lctx_ptr->lctx_l_area;
	    lctx->lctx_lgid = log_id;
	    lctx->lctx_status = 0;
	    lctx->lctx_bksz = dmf_svcb->svcb_lctx_ptr->lctx_bksz;
	}
	else
	{
	    /* Can't get the memory; use the static LCTX exclusively */
	    lctx = dmf_svcb->svcb_lctx_ptr;
	    dm0s_mlock(&lctx->lctx_mutex);
	    lctx_locked = TRUE;
	}

	/* Mark beginning of rollback in II_DBMS_LOG  */
	if ( DMZ_XCT_MACRO(3) )
	    TRdisplay("%@ dmxe_abort: Begin rollback of txn (%x,%x)\n",
		tran_id->db_high_tran, tran_id->db_low_tran);

	/*
	** Position the log file to last record for this transaction.
	*/
	status = dm0l_position(DM0L_TRANS, log_id, 0, lctx, dberr);
	if (status != E_DB_OK)
	    break;

	/* Initialize the DMVE control block. */

	dmve.dmve_next = 0;
	dmve.dmve_prev = 0;
	dmve.dmve_length = sizeof(DMVE_CB);
	dmve.dmve_type = DMVECB_CB;
	dmve.dmve_owner = 0;
	dmve.dmve_ascii_id = DMVE_ASCII_ID;
	dmve.dmve_action = DMVE_UNDO;
	dmve.dmve_dcb_ptr  = dcb;
	dmve.dmve_tran_id = *tran_id;
	dmve.dmve_lk_id = lock_id;
	dmve.dmve_log_id = log_id;
	dmve.dmve_rolldb_action = NULL;
	dmve.dmve_flags = 0;
	if (odcb)
	    dmve.dmve_db_lockmode = odcb->odcb_lk_mode;
	else
	    dmve.dmve_db_lockmode = (dcb->dcb_status & DCB_S_EXCLUSIVE) != 0 ?
					ODCB_X : ODCB_IX;
	dmve.dmve_logging = TRUE;
	for (i = 0; i < DM_RAWD_TYPES; ++i)
	{
	    dmve.dmve_rawdata[i] = NULL;
	    dmve.dmve_rawdata_size[i] = 0;
	}

	/*
	** Read backwards through the stream of log records for this
	** transaction backing out each update action.
	**
	** The following types of log records are special to this routine:
	**
	**     BT         - The beginning of the transaction has been found
	**		    and the abort is complete.
	**     ET         - The transaction had already committed before
	**		    the abort request was made.  No work is required.
	**     CLR or
	**     EMINI or
        **     ABORTSAVE  - These log records (CLR is a flag not a record type)
	**		    mark actions that have already been aborted.  They
	**		    direct us to jump over the already processed
	**		    records and begin aborting new log records.
	*/
	abort_complete = FALSE;
	bad_read = 0;

	do
	{
	    /*  Read the next record. */

    	    status = dm0l_read(lctx, log_id, (DM0L_HEADER **)&dmve.dmve_log_rec,
		&dmve.dmve_lg_addr, (i4*)NULL, &dmve.dmve_error);
	    if (status != E_DB_OK)
            {
                /* If tried to read the first record, and it wasn't written
                ** to disk yet, stall for a short time and try again.
                */
                if (bad_read < 5 && (dmve.dmve_error.err_code == E_DM9238_LOG_READ_EOF))
                {
                   STprintf(buffer, "E_DM9238 reported. Retrying...");
                   uleFormat( NULL, 0, NULL, ULE_MESSAGE, 0, buffer, STlength(buffer), 
			       NULL, err_code, 0);


		   /* MUST reset status before continue */
		   status = E_DB_OK;
                   bad_read++;
                   CSsuspend( CS_TIMEOUT_MASK, 2, 0);

                   continue;
                }
		break;
            }
            if ( bad_read )
            {
               STprintf(buffer, "Delay resolved the E_DM9238");
               uleFormat( NULL, 0, NULL, ULE_MESSAGE, 0, buffer, STlength(buffer), 
			    NULL, err_code, 0);

               bad_read = 0;
            }

	    record_header = (LG_RECORD *) lctx->lctx_header;

#ifdef xDEBUG
	    if (DMZ_AM_MACRO(60))
		dmd_log(FALSE, dmve.dmve_log_rec, lctx->lctx_bksz);
#endif

	    /* Generate pass abort if test tracepoint indicates to do so */
            if (DMZ_SES_MACRO(30))
	    {
                status = E_DB_ERROR;
		SETDBERR(&dmve.dmve_error, 0, E_DM9503_DMXE_ABORT);
                break;
	    }

            /* (Bug 62550) Update the activity due to force_abort in order
	       to make exp.scf.scs.scb_activity and exp.scf.scs.scb_act_detail
	       in sync. */
            

         /* Take out change for bug 62550 since it produces bug 77435*/
/*	    if ( temp == TRUE) {
		STprintf(buffer, "Performing Force Abort Processing");
		scf_cb.scf_len_union.scf_blength = STlength(buffer);
		(VOID) scf_call(SCS_SET_ACTIVITY, &scf_cb);
		scf_cb.scf_nbr_union.scf_atype = SCS_A_MINOR;
                temp = FALSE;
            }
*/
	    /*
	    ** Every 200 records, update IIMONITOR comment line with
	    
	    ** information about abort progress.
	    */
	    if ((++count % 200) == 0)
	    {
		STprintf(buffer,
		   "%d log record%s done, %d to go: log addr %x:%x:%x.",
			count,
			((count == 1) ? "" : "s"),
			(u_i2)record_header->lgr_sequence,
			dmve.dmve_lg_addr.la_sequence,
			dmve.dmve_lg_addr.la_block,
			dmve.dmve_lg_addr.la_offset);
		scf_cb.scf_len_union.scf_blength = STlength(buffer);
		(VOID) scf_call(SCS_SET_ACTIVITY, &scf_cb);
	    }

	    /*
	    ** If the previous record processed for this transaction was a CLR
	    ** then we want to jump backwards in the transaction chain to the
	    ** record which was compensated and start recovery processing at
	    ** the log record previous to it.
	    **
	    ** We skip each log record for this transaction until we find the
	    ** one whose LSN matches the compensated LSN.
	    */

	    lsn = ((DM0L_HEADER *)dmve.dmve_log_rec)->lsn;
	    
	    if (clr_jump)
	    {
		/*
		** If we have not yet reached the LSN we are looking for, then
		** go onto the next log record.
		*/
		if (LSN_GT(&lsn, &clr_lsn))
		    continue;

		/*
		** Consistency Check:
		** If we find an LSN less than the one we are looking for, then
		** something is wrong - the record compensated by this LSN is
		** not on our transaction chain.
		*/
		if (LSN_LT(&lsn, &clr_lsn))
		{
		    /* XXXX ERROR */
		    TRdisplay("DMXE_ABORT: CLR lsn not found.\n");
		    status = E_DB_ERROR;
		    SETDBERR(&dmve.dmve_error, 0, E_DM9503_DMXE_ABORT);
		    break;
		}

		/*
		** We have found the compensated record, so turn off the
		** CLR_JUMP flag so that the next record for this transaction
		** will be processed.  Then continue to read the next log
		** record which will be the record at which to begin abort
		** processing.
		**
		** Special conditions: If the compensation record was an
		** ABORTSAVE or EMINI, neither a SAVEPOINT nor BMINI
		** record was actually written to the log, rather the 
		** transaction's last LSN at the time of the SAVEPOINT or 
		** BMINI was recorded in the ABORTSAVE or EMINI log record
		** (we're now positioned on the last-written log record
		** PRIOR to the start of the SAVEPOINT/BMINI). 
		** Fall through to process this log record.
		*/
		clr_jump = FALSE;
		if ( clr_type != DM0LABORTSAVE && clr_type != DM0LEMINI )
		    continue;
	    }

	    if ( sp_id && LSN_EQ(&lsn, savepoint_lsn) )
	    {
		/*
		** If abort-to-savepoint and this record marks the beginning
		** of the savepoint, we're done. Note that the LSN is that
		** of the transaction's last LSN at the time the savepoint
		** was established.
		*/
		abort_complete = TRUE;
	    }
	    else if (((DM0L_HEADER *)dmve.dmve_log_rec)->flags & DM0L_CLR)
	    {
		/*
		** Process Compensation Log Records.  These are logged during
		** abort processing as each undo action is taken.  If we encounter
		** a CLR during rollback processing, then it indicates that the
		** transaction being rolled back had already had some rollback
		** processing done on it (either we are reattempting the rollback
		** an abort-to-savepoint was done).
		**
		** We save the CLR's compensated_lsn pointer to skip over the 
		** already-aborted work to the next log record which has not
		** yet been aborted.
		*/
		clr_lsn = ((DM0L_HEADER *)dmve.dmve_log_rec)->compensated_lsn;
		clr_type = ((DM0L_HEADER *)dmve.dmve_log_rec)->type;
		clr_jump = TRUE;

		/*
		** If rollback to savepoint and where we're jumping to 
		** is the BT, then quit now rather than trolling through all the 
		** to-be-skipped records in between.
		**
		** We can get away with this only in a savepoint as dmx_abort
		** "knows" the proper setting of DMXE_JOURNAL; in all other
		** cases we need to back into the BTs to find out the
		** journal setting for writing the ET. A rollback to savepoint
		** may well not encounter a BT, so we trust that dmx_abort
		** set the journal flag properly.
		*/
		if ( sp_id && LSN_EQ(&clr_lsn, &tr.tr_first_lsn) )
		{
		   abort_complete = TRUE;
		   lsn = clr_lsn;
		}
	    }
	    else if ( ((DM0L_HEADER *)dmve.dmve_log_rec)->type == DM0LBT )
	    {
		/*
		** Look for other special log records which direct abort processing
		** rather than requiring a specific database recovery action.
		**
		**     ET - Halt undo processing since the transaction committed.
		** 
		**     BT - Halt undo processing since the transaction is fully
		**             aborted.
		*/
		
		/*
		** Found beginning of the transaction - Abort is complete!
		** only if this isn't the 2nd (journaled) BT record for the TX
		*/
		if (!(((DM0L_HEADER *)dmve.dmve_log_rec)->flags & DM0L_2ND_BT))
		    abort_complete = TRUE;

		/* Only journal the ET/ABORTSAVE record if there's a Journaled BT record */
		if ( !jnl_et )
		    jnl_et = (((DM0L_HEADER *)dmve.dmve_log_rec)->flags & DM0L_JOURNAL);
	    }
	    else if ( ((DM0L_HEADER *)dmve.dmve_log_rec)->type == DM0LET )
	    {
		/*
		** Found End Transaction Record.  The transaction cannot
		** be aborted because it had already been successfully
		** committed before the abort request was made.
		*/
		et_found = TRUE;
		abort_complete = TRUE;
	    }
	    /*
	    ** Process record by type.
	    */
	    else status = dmve_undo(&dmve);

	} while (status == E_DB_OK && abort_complete == FALSE);

	if (status != E_DB_OK)
	    *dberr = dmve.dmve_error;

	break;
    }

    /*
    ** If we locked the server-wide lctx mutex then release it now.
    **
    ** If we allocated a private LCTX, deallocate it now.
    */
    if (lctx_locked)
	dm0s_munlock(&lctx->lctx_mutex);
    else if ( lctx )
	dm0m_deallocate((DM_OBJECT **)&lctx);

    /* Mark ending of rollback in II_DBMS_LOG  */
    if ( DMZ_XCT_MACRO(3) && count )
	TRdisplay("%@ dmxe_abort: Ended rollback of txn (%x,%x), %d log records\n",
	    tran_id->db_high_tran, tran_id->db_low_tran, count);


    /* clear both activity and activity detail */
    scf_cb.scf_len_union.scf_blength = 0;
    (VOID) scf_call(SCS_CLR_ACTIVITY, &scf_cb);
    scf_cb.scf_nbr_union.scf_atype = SCS_A_MAJOR;
    (VOID) scf_call(SCS_CLR_ACTIVITY, &scf_cb);
    
    
    /*
    ** Complete the abort request.
    **
    ** For a full transaction abort:
    **     - Log the End Transaction log record.
    **     - Release the Transaction's locks and deallocate its lock list.
    **     - Deallocate the Transaction's LG context.
    **     
    ** For an abort to savepoint we just log the Abortsave log record.  The
    ** transaction's locks must be held until the full transaction completes.
    **
    ** If we found that the transaction had actually committed (we found its
    ** Commit log record) or this is a readonly database, then we log nothing 
    ** and just deallocate its LK and LG contexts to complete the commit.
    */

    while ( status == E_DB_OK )
    {
	if (et_found == FALSE && dcb->dcb_access_mode != DCB_A_READ)
	{
	    /*
	    ** Call the buffer manager to disown the pages updated by this
	    ** transaction if the transaction is now complete.  If the server
	    ** is Non-FastCommit then this will force all pages touched by the
	    ** transaction to the database.
	    */
	    if ( sp_id )
		/* Update the savepoint_lsn with where rollback stopped */
		*savepoint_lsn = lsn;
	    else
	    {
		status = dm0p_force_pages(lock_id, tran_id, log_id, 0,dberr);
		if (status != E_DB_OK)
		    break;
	    }

	    /*
	    ** Log the End Transaction or AbortSave log record.
	    */

	    /*
	    ** If the (SHARED) transaction never became active,
	    ** pass the ReadOnly flag so we don't write an ET.
	    **
	    ** if (flag & DMXE_ROTRAN)
	    */
	    if ( tr.tr_status & TR_INACTIVE )
		et_flags = DM0L_ROTRAN;
	    if ( (flag & DMXE_JOURNAL) && (jnl_et | sp_id))
		et_flags |= DM0L_JOURNAL;
	    if(flag & DMXE_NONJNLTAB)
		et_flags |= DM0L_NONJNLTAB;

	    if (sp_id == 0)
		status = dm0l_et(log_id, et_flags, DM0L_ABORT, 0, 0, &ctime, 
				    dberr);
	    else
		status = dm0l_abortsave(log_id, sp_id, et_flags,
				    savepoint_lsn, &lsn, dberr);
	    if (status != E_DB_OK)
		break;
	}
	else
	{
	    /*
	    ** Either this is a readonly database (nothing could have been 
	    ** updated) or the End transaction record was found, which means 
	    ** that this transaction was either committed or aborted already.  
	    ** No abort work to do - but we do need to clear the LG context 
	    ** for this transaction.
	    */
	    stat = LGend(log_id, 0, &sys_err);
	    if (stat != OK)
	    {
		uleFormat(NULL, stat, &sys_err, ULE_LOG, NULL,
		    0, 0, 0, err_code, 0);
		uleFormat(NULL, E_DM900E_BAD_LOG_END, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, 0L, (i4 *)NULL, err_code, 1, 0, log_id);
		SETDBERR(dberr, 0, E_DM9503_DMXE_ABORT);
		status = E_DB_ERROR;
		break;
	    }
	}

	/*
	** After logging the transaction completion, we can release the
	** transaction's locks (unless this was a Savepoint Abort).
	**
	** If we fail to be able to release the transaction's locks, but have
	** otherwise successfully aborted the transaction, then we log all of
	** the lock release error messages, and log that the lock release
	** problem occurred during transaction abort, but then we return OK
	** to our caller. See comment in function header for more explanation.
	*/
	if (sp_id == 0)
	{
	    /* Delay unlocking until after temporary tables are destroyed */
	    if ( flag & DMXE_DELAY_UNLOCK )
	    {
		return (E_DB_OK);
	    }

	    stat = LKrelease(LK_ALL, lock_id, (LK_LKID *)0, 
				    (LK_LOCK_KEY *)0, (LK_VALUE *)0, &sys_err);
	    if (stat != OK)
	    {
		uleFormat(NULL, stat, &sys_err, ULE_LOG, NULL, NULL, 0, NULL, err_code, 0);
		uleFormat(NULL, E_DM901B_BAD_LOCK_RELEASE, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 1, 0, lock_id);
		uleFormat(NULL, E_DM9503_DMXE_ABORT, &sys_err, ULE_LOG, NULL,
			    NULL, 0, NULL, err_code, 0);

		/* FALL THROUGH TO RETURN OK */
	    }
	    else
	    {
		/*
		** Trace the Lock Release.
		*/
		if (DMZ_SES_MACRO(1))
		    dmd_lock(0, lock_id, 1, LK_ALL, 0, 0, tran_id, 0, 0);    
	    }
	}
	else
	{
	    /* Put the lock-list back into its usual state */
	    (void) LKalter(LK_A_INTERRUPT, lock_id, &sys_err);
	}


	/*
	** Successfull Transaction Abort is complete.
	*/
	return (E_DB_OK);
    }

    /*
    ** If the abort request failed because the database was recognized to
    ** be inconsistent, then remove the transaction context.  There is not
    ** much else we can do.
    */
    if (dberr->err_code == E_DM0100_DB_INCONSISTENT)
    {
	stat = LGend(log_id, 0, &sys_err);
	if (stat != OK)
	{
	    uleFormat(NULL, stat, &sys_err, ULE_LOG, NULL,
		(char *)NULL, 0L, (i4 *)NULL, err_code, 0);
	    uleFormat(NULL, E_DM900E_BAD_LOG_END, &sys_err, ULE_LOG, NULL,
		(char *)NULL, 0L, (i4 *)NULL, err_code, 1, 0, log_id);
	}

	stat = LKrelease(LK_ALL, lock_id, (LK_LKID *)0, 
				    (LK_LOCK_KEY *)0, (LK_VALUE *)0, &sys_err);
	if (stat != OK)
	{
	    uleFormat(NULL, stat, &sys_err, ULE_LOG, NULL,
		(char *)NULL, 0L, (i4 *)NULL, err_code, 0);
	    uleFormat(NULL, E_DM901B_BAD_LOCK_RELEASE, &sys_err, ULE_LOG, NULL,
		(char *)NULL, 0L, (i4 *)NULL, err_code, 1, 0, lock_id);
	}
	return(E_DB_ERROR);
    }

    /*
    ** If the abort attempt failed at some point during recovery processing
    ** then issue a Pass Abort request to let the RCP have a shot at aborting
    ** the transaction.
    **
    ** After signaling the Pass Abort we simply return as though the abort
    ** succeeded with the following assumptions:
    **
    **   - Regardless of the outcome of the RCP's abort attempt, the data
    **     updated by this transaction will remain locked until the abort
    **     attempt is complete.  So transaction consistency is not affected
    **     returning before the abort is complete.
    **
    **   - If the abort request fails in the RCP, the database will be marked
    **     inconsistent and we will be kicked out on the next update attempt.
    **
    */
    if ((dberr->err_code != E_DM010C_TRAN_ABORTED) && (sp_id == 0))
    {
	/*
	** Log Pass Abort action as well as the reason for the abort failure.
	*/
	uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
	    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);

	uleFormat(NULL, E_DM9509_DMXE_PASS_ABORT, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
	    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 2,
	    0, tran_id->db_high_tran, 0, tran_id->db_low_tran);

	/*
	** Failure to pass the recovery processing to the RCP means we
	** are screwed.  We bring the server down in the hopes that
	** crash recovery will fix everything (if not, then the database
	** will end up marked inconsistent).
	*/
	status = dmxe_pass_abort(log_id, lock_id, tran_id,
				dcb->dcb_id, dberr);

	/*
	** If log txn is a handle to a SHARED txn, end our transaction.
	** The LG_A_PASS_ABORT call made by dmxe_pass_abort() caused
	** our handle to be disconnected from the SHARED list and
	** transformed into a regular non-PROTECT txn.
	*/
	if ( tr.tr_status & TR_SHARED )
	{
	    stat = LGend(log_id, 0, &sys_err);
	    if (stat != OK)
	    {
		status = E_DB_ERROR;
		uleFormat(NULL, stat, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, 0L, (i4 *)NULL, err_code, 0);
		uleFormat(NULL, E_DM900E_BAD_LOG_END, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, 0L, (i4 *)NULL, err_code, 1, 0, log_id);
	    }

	    stat = LKrelease(LK_ALL, lock_id, (LK_LKID *)0, 
					(LK_LOCK_KEY *)0, (LK_VALUE *)0, &sys_err);
	    if (stat != OK)
	    {
		status = E_DB_ERROR;
		uleFormat(NULL, stat, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, 0L, (i4 *)NULL, err_code, 0);
		uleFormat(NULL, E_DM901B_BAD_LOCK_RELEASE, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, 0L, (i4 *)NULL, err_code, 1, 0, lock_id);
	    }
	}

	if (status != E_DB_OK)
	{
	    uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	    uleFormat(NULL, E_DM9508_DMXE_TRAN_INFO, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, err_code, 6,
		0, log_id,
		0, lock_id,
		0, tran_id->db_high_tran,
		0, tran_id->db_low_tran,
		0, dcb->dcb_id,
		sizeof(DB_DB_NAME), dcb->dcb_name.db_db_name);
	    dmd_check(E_DMF029_DMXE_PASS_ABORT);
	}

	if ( flag & DMXE_DELAY_UNLOCK )
	{
	    /*
	    ** Tell dmx_abort to NOT release lock list,
	    ** needed by pass abort in RCP
	    */
	    SETDBERR(dberr, 0, E_DM9509_DMXE_PASS_ABORT);
	    return(E_DB_ERROR);
	}

	return (E_DB_OK);
    }

    if (dberr->err_code != E_DM0100_DB_INCONSISTENT)
    {
	uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
	    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	SETDBERR(dberr, 0, E_DM9503_DMXE_ABORT);
    }

    return(E_DB_ERROR);
}

/*{
** Name: dmxe_begin	- Begin a transaction.
**
** Description:
**      This function makes the appropriate locking and logging request
**	needed to begin a transaction.
**
** Inputs:
**      mode                            The mode of the transaction. Must be
**					DMXE_READ, DMXE_WRITE.
**      flags                           From the following:
**					DMXE_INTERNAL - local transaction
**					DMXE_JOURNAL - tran is journaled
**					DMXE_DELAYBT - don't write begin
**					    transaction record. It will be
**					    written when the first write
**					    operation is performed.
**					DMXE_CONNECT - connect to
**					    existing log/lock contexts.
**					DMXE_XA_START_XA - explicit 
**					    start XA transaction.
**					DMXE_XA_START_JOIN - explicit join
**					    to an existing XA transaction,
**					    combined with DMXE_XA_START_XA
**	dcb				dcb pointer.
**	dcb_id				The logging system database id.
**	user_name			The user name of the session.
**	related_lock			The related lock list.
**      tran_id                         Optional distributed transaction id.
**
** Outputs:
**      tran_id                         The assigned transaction_id.
**	log_id				The logging system id.
**	lock_id				The assigned lock id.
**	err_code			The reason for error status.
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
**	13-dec-1985 (derek)
**          Created new for jupiter.
**      09-jun-1986 (ac)    
**          Added dm0l_bt() and dm0l_robt() logging calls.
**      18-Aug-1987 (ac)
**	    Changed to LGbegin a transaction, create the lock list, then
**	    write the begin transaction log record.
**      30-sep-1987 (rogerk)
**	    Added flags parameter to take place of old type and journal parms.
**	    Added DELAYBT flag which specifies to not write a begin transaction
**	    log record.  The BT record will be written when (if) the first
**	    write operation is performed.
**      12-Jan-1989 (ac)
**          Added 2PC support.
**	17-mar-1989 (EdHsu)
**	    Support cluster online backup.
**	15-may-1989 (rogerk)
**	    Check for LG_EXCEED_LIMIT return value from LGbegin.
**	    Don't convert DMF external errors (< E_DM_INTERNAL) to generic
**	    E_DM9500_DMXE_BEGIN error number.
**	29-dec-1989 (rogerk)
**	    Added call to LGshow to check if database is being backed up.  If
**	    it is, then set DCB_S_BACKUP status so DMF will know to do special
**	    logging.
**	17-jan-1990 (rogerk)
**	    Moved LGend call out of dm0l_bt when there is an error.  We need
**	    to call LGend here now if dm0l_bt returns an error.
**	22-mar-1991 (bryanp)
**	    B36630: move dcb_status handling into dm2d_check_db_backup_status().
**	6-jun-1991 (rachael,bryanp)
**	    Added error e_dm9c07_out_of_locklists for error message project.
**	    This error will print in the errlog.log prior to the return of
**	    e_dm004b.
**	26-oct-1992 (rogerk)
**	    Added clstatus error reporting.
**	27-Feb-1997 (jenjo02)
**	    Extended <set transaction statement> to support <transaction access mode>
**	    (READ ONLY | READ WRITE) per SQL-92.
**	    Removed call to dm0l_robt() which really isn't needed and just make
**	    the LGbegin() call here with the proper flags.
**	15-Dec-1999 (jenjo02)
**	    Support for SHARED transactions. If connecting to a SHARED log 
**	    transaction, create a SHARED lock list.
**	29-Jan-2004 (jenjo02)
**	    Added support for DMXE_CONNECT to connect to existing
**	    log/lock contexts.
**	29-Jun-2006 (jonj)
**	    Add support for DMXE_XA_START_XA, DMXE_XA_START_JOIN
**	28-Jul-2006 (jonj)
**	    Don't ule_format duplicate/unknown txn Log errors
**	    if XA, those are simple user errors.
**	01-Aug-2006 (jonj)
**	    Handle LG_CONTINUE status from LGbegin for XA transactions.
*/
DB_STATUS
dmxe_begin(
i4             mode,
i4		    flags,
DMP_DCB		    *dcb,
i4		    dcb_id,
DB_OWN_NAME	    *user_name,
i4		    related_lock,
i4		    *log_id,
DB_TRAN_ID	    *tran_id,
i4		    *lock_id,
DB_DIS_TRAN_ID	    *dis_tran_id,
DB_ERROR	    *dberr)
{
    DB_STATUS		status;
    STATUS		cl_status;
    CL_ERR_DESC		sys_err;
    i4			begin_flag = 0;
    i4			btflag = 0;
    i4			lk_flags = 0;
    LG_LSN		lsn;
    LG_TRANSACTION	tr;
    i4                  length;
    i4			*err_code = &dberr->err_code;

    CLRDBERR(dberr);

    dmf_svcb->svcb_stat.tx_begin++;
    for (;;)
    {
	/*
	** Start a log transaction.
	**
	** READ_ONLY transactions will not log, but need a log_id in order for
	** the buffer manager to flush data pages in cache to make room for the
	** pages read by the nologging transaction. LGend for the READ_ONLY
	** transaction will be called when the transaction is committed or
	** aborted as a is done for a normal transaction.
	**
	** Transactions running with SET NOLOGGING may be either READ_ONLY
	** or READ_WRITE. READ_ONLY will begin a LG_READONLY log transaction,
	** READ_WRITE will begin a normal non-LG_READONLY log transaction
	** but will never write log records.
	*/

	if ( flags & DMXE_CONNECT )
	{
	    i4	connect_to_log_id = *log_id;

	    cl_status = LGconnect((LG_LXID)connect_to_log_id, 
				  (LG_LXID*)log_id, &sys_err);
	}
	else
	{
	    if (mode == DMXE_READ)
		begin_flag |= LG_READONLY;
	    else if (flags & DMXE_JOURNAL)
	    {
		begin_flag |= LG_JOURNAL;
		btflag |= DM0L_JOURNAL;
	    }

	    /* Pass xa_start flags to LG */
	    if ( flags & DMXE_XA_START_XA )
	    {
		/* By itself, start a new XA txn */
		begin_flag |= LG_XA_START_XA;

		/* ...or join an existing XA txn */
		if ( flags & DMXE_XA_START_JOIN )
		    begin_flag |= LG_XA_START_JOIN;
	    }

	    cl_status = LGbegin(begin_flag, dcb_id, tran_id, log_id, 
			     sizeof(DB_OWN_NAME), (char *)user_name, 
			     dis_tran_id, &sys_err);
	}
	/*
	** If log transaction is sharable, LGbegin will return
	**
	**	LG_SHARED_TXN  - new sharable transaction created,
	**			 create a SHARED lock list for it.
	**	LG_CONNECT_TXN - connection to an existing SHARED
	**			 transaction, connect to its
	**			 lock list.
	**	LG_CONTINUE    - Found reusable context for
	**		         XA branch, replete with lock
	**		         context.
	*/
	if (cl_status != OK)
	{
	    /* New SHARED transaction? */
	    if ( cl_status == LG_SHARED_TXN )
	    {
		/* Create SHARED lock list */
		lk_flags = LK_SHARED;
	    }
	    /* Connected to SHARED transaction? */
	    else if ( cl_status == LG_CONNECT_TXN )
	    {
		/* Create connecting lock list */
		lk_flags = LK_CONNECT;
	    }
	    else if ( cl_status != LG_CONTINUE )
	    {
		/* Duplicate/unknown txn aren't so bad if XA */
		if ( (flags & DMXE_XA_START_XA) == 0 ||
		    (cl_status != LG_NO_TRANSACTION &&
		     cl_status != LG_DUPLICATE_TXN) )
		{
		    uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
			(char *)NULL, 0L, (i4 *)NULL, err_code, 0);
		    (VOID)uleFormat(NULL, E_DM900C_BAD_LOG_BEGIN, &sys_err, 
			    ULE_LOG, NULL, NULL, 0, NULL,
			    err_code, 1, 0, dcb_id);
		}
		if (cl_status == LG_EXCEED_LIMIT)
		    SETDBERR(dberr, 0, E_DM0062_TRAN_QUOTA_EXCEEDED);
		else if ( cl_status == LG_NO_TRANSACTION )
		    SETDBERR(dberr, 0, E_DM0120_DIS_TRAN_UNKNOWN);
		else if ( cl_status == LG_DUPLICATE_TXN )
		    SETDBERR(dberr, 0, E_DM0119_TRAN_ID_NOT_UNIQUE);
		else
		    SETDBERR(dberr, 0, E_DM9500_DMXE_BEGIN);
		return (E_DB_ERROR);
	    }
	}

	if ( cl_status == LG_CONTINUE )
	{
	    /* Extract the lock id from the reused logging context */
	    MEcopy((char*)log_id, sizeof(*log_id), (char*)&tr);

	    cl_status = LGshow(LG_S_ATRANSACTION, (PTR)&tr, sizeof(tr), &length, &sys_err);

	    if ( cl_status || length == 0 )
	    {
		if ( cl_status )
		    uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
			(char *)NULL, 0L, (i4 *)NULL, err_code, 0);
		(VOID)uleFormat(NULL, E_DM9017_BAD_LOG_SHOW, &sys_err, 
			ULE_LOG, NULL, NULL, 0, NULL,
			err_code, 1, 0, LG_S_ATRANSACTION);
		/*
		** Release the transaction.
		*/
		(VOID)LGend(*log_id, 0, &sys_err);

		SETDBERR(dberr, 0, E_DM9500_DMXE_BEGIN);
		return (E_DB_ERROR);
	    }
	    *lock_id = tr.tr_lock_id;
	}
	else
	{
	    /*
	    ** Create (or connect to) the transaction's lock list.
	    */
	    if ( flags & DMXE_CONNECT )
	    {
		i4	connect_to_lock_id = *lock_id;

		cl_status = LKconnect((LK_LLID)connect_to_lock_id, 
				      (LK_LLID*)lock_id, &sys_err);
	    }
	    else
		cl_status = LKcreate_list(lk_flags, (i4)related_lock, 
		    (LK_UNIQUE *)tran_id, (LK_LLID *)lock_id, 0, 
		    &sys_err);

	    if (cl_status != OK)
	    {
		CL_ERR_DESC		temp_err_desc;

		/*  Log the locking error. */

		uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, 0L, (i4 *)NULL, err_code, 0);
		uleFormat(NULL, E_DM901A_BAD_LOCK_CREATE, &sys_err, ULE_LOG, NULL, 
		    (char *)NULL, 0L, (i4 *)NULL, err_code, 0);

		/*  End the transaction to clean up the LG structure. */
		(VOID)LGend(*log_id, 0, &temp_err_desc);
		if (cl_status == LK_NOLOCKS)
		{
		    uleFormat(NULL, E_DM9C07_OUT_OF_LOCKLISTS, &sys_err, ULE_LOG, NULL,
			(char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
		    SETDBERR(dberr, 0, E_DM004B_LOCK_QUOTA_EXCEEDED);
		    return(E_DB_ERROR);
		}
		SETDBERR(dberr, 0, E_DM9500_DMXE_BEGIN);
		return (E_DB_ERROR);	    
	    }

	    /*
	    ** Set the lock context identifier and initial transaction state
	    ** flags in the log context block
	    **
	    ** It's currently only needed for some XA operations,
	    ** but we'll do it for all.
	    */
	    tr.tr_id = *(LG_LXID*)log_id;
	    tr.tr_lock_id = *lock_id;
	    tr.tr_txn_state = flags;

	    cl_status = LGalter(LG_A_LOCKID, (PTR)&tr, sizeof(tr), &sys_err);
	    if ( cl_status == OK )
		cl_status = LGalter(LG_A_TXN_STATE, (PTR)&tr, sizeof(tr), &sys_err);


	    if ( cl_status )
	    {
		uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, 0L, (i4 *)NULL, err_code, 0);
		(VOID)uleFormat(NULL, E_DM900B_BAD_LOG_ALTER, &sys_err, 
			ULE_LOG, NULL, NULL, 0, NULL,
			err_code, 1, 0, LG_A_LOCKID);
		/*
		** Release the transaction.
		*/
		(VOID)LGend(*log_id, 0, &sys_err);

		/* Release the lock list for this transaction. */

		(VOID)LKrelease(LK_ALL, *lock_id, (LK_LKID *)0, 
			(LK_LOCK_KEY *)0, (LK_VALUE *)0, &sys_err);
		SETDBERR(dberr, 0, E_DM9500_DMXE_BEGIN);
		return (E_DB_ERROR);
	    }
	}


	/*
	** Write transactions may eventually need Begin
	** Transaction records.
	**
	** If the Delay_bt flag is set, then don't write the begin transaction
	** record here, wait until the first write operation is done.  This
	** will avoid writing the begin and end tran records when no
	** changes are made for a transaction.
	*/
	if (((flags & (DMXE_DELAYBT|DMXE_CONNECT)) == 0) && (mode == DMXE_WRITE))
	{
	    status = dm0l_bt(*log_id, btflag, *lock_id, dcb, user_name, 
				&lsn, dberr);
	    if (status != E_DB_OK)
	    {
		/*
		** Release the transaction.
		*/
		(VOID)LGend(*log_id, 0, &sys_err);

		/* Release the lock list for this transaction. */

		cl_status = LKrelease(LK_ALL, *lock_id, (LK_LKID *)0, 
			(LK_LOCK_KEY *)0, (LK_VALUE *)0, &sys_err);
		if (cl_status == OK)
		{
		    if (DMZ_SES_MACRO(1))
			dmd_lock(0, *lock_id, 1, LK_ALL, 0, 0, tran_id, 0, 0);
		}		
		break;
	    }

	    /*
	    ** Check if the database is in backup state.  If it is, then we
	    ** have to log physical log records for all updates, even updates
	    ** to system catalogs and btree index pages.  Mark the database in
	    ** BACKUP state so that DMF will do this.
	    */
	    status = dm2d_check_db_backup_status( dcb, dberr );
	    if (status != E_DB_OK)
		break;
	}

	return (E_DB_OK);
    }

    /*	Handle mapping the error message. */
    if (dberr->err_code > E_DM_INTERNAL)
    {
	uleFormat(dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL, err_code, 0);
	SETDBERR(dberr, 0, E_DM9500_DMXE_BEGIN);
    }
    return (E_DB_ERROR);
}

/*{
** Name: dmxe_commit	- Commit a transaction.
**
** Description:
**      This function makes the appropriate calls to the
**	the logging syste, locking system and buffer manager.
**
** Inputs:
**      tran_id                         The transaction id to commit.
**	log_id				The log_id to use.
**      lock_id				The lock_id to use.
**	flag				Special flags. 
**					DMXE_JOURNAL - journaled transaction. 
**                                      DMXE_ROTRAN - readonly transaction.
**					DMXE_WILLING_COMMIT - commit a willing
**					commit transaction.
**					DMXE_LOGFULL_COMMIT - commit the 
**					transaction, but keep log/lock
**					contexts.
**					DMXE_CONNECT - This was a shared
**					transaction that is disconnecting,
**					and it's not the last sharer.
**					Don't force pages or write ET.
**      ctime                           Commit time.
** Outputs:
**      err_code                        The reason for an error status.
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR			E_DM9500_DMXE_COMMIT
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	13-dec-1985 (derek)
**          Created new for jupiter.
**      09-jun-1986 (ac)
**          Added dm0l_et() logging call.
**      26-feb-87 (jennifer)
**          Added commit time to call.
**	08-apr-1987 (Derek)
**	    Must call dm0p_force_pages on a read-only transaction so that
**	    the cache locks held by the buffer manager can be released.
**	15-jun-87 (rogerk)
**	    Added parameter to dm0p_force_pages for fast commit. Took out
**	    call to dm0l_force as it is not needed.
**      30-Jan-1989 (ac)
**          Added 2PC support.
**	26-oct-1992 (rogerk)
**	    Added clstatus error reporting.
**      18-jan-2000 (gupsh01)
**          Added support for setting transaction flag when the transaction
**          changed a non-journaled table for a journalaed database.
**	20-Apr-2004 (jenjo02)
**	    Added support for LOGFULL_COMMIT.
**	30-Apr-2004 (schka24)
**	    Don't force pages or write ET if disconnecting shared txn.
**	03-May-2004 (jenjo02)
**	    Slight modification to Karl's last change; go ahead and
**	    call dm0l_et even for SHARED (DMXE_COMMIT) txns, LG now
**	    delays the ET until the last sharer commits.
**	22-Jul-2004 (schka24)
**	    Avoid the confusing "unlock:all" from a lock-traced session
**	    that has merely disconnected from a shared txn instead of
**	    actually committing it.
**	29-Sep-2006 (jonj)
**	    Remove bogus cast from tran_id to XCB*, trust the 
**	    BT-journaledness provided by the caller.
**	28-Jun-2007 (kibro01) b118559
**	    Add in flag to allow lock release to take place after the
**	    commit, so that transaction temporary tables can keep their
**	    lock until they are destroyed.
*/
DB_STATUS
dmxe_commit(
DB_TRAN_ID         *tran_id,
i4		    log_id,
i4		    lock_id,
i4		    flag,
DB_TAB_TIMESTAMP    *ctime,
DB_ERROR	    *dberr)
{
    DB_STATUS		status;
    STATUS		stat;
    i4		et_flags;
    CL_ERR_DESC		sys_err;
    i4			*err_code = &dberr->err_code;

    CLRDBERR(dberr);

    dmf_svcb->svcb_stat.tx_commit++;

    for (;;)
    {
	/*
	** Dont bother calling dm0l_force here go force the log file as it
	** will be called anyway if needed before writing any dirty pages.
	** The log will be forced again after the commit record is written.
	**
	** status = dm0l_force(log_id, 0, 0, dberr);
	*/

	/*
	** Release the pages touched by this transaction from the transaction
	** page list.  If not running Fast Commit, then force the dirty
	** pages to disk.
	*/
	
	/* 
	** If the transaction is in WILLING COMMIT state, the dirty pages
	** have been forced at the the WILLING COMMIT time.
	** If a simple disconnect, the transaction is still going.
	**
	** In a shared transaction, only the pages belonging to
	** this piece of the transaction (log_id) will be released
	** (dm0p_force_pages() qualifies tran_id with log_id now).
	*/
	if ((flag & (DMXE_WILLING_COMMIT|DMXE_CONNECT)) == 0)
	{
	    status = dm0p_force_pages(lock_id, tran_id, log_id, (i4)0,
		dberr);
	    if (status != E_DB_OK)
		break;
	}

	/*
	** Force the commit record.
	** This commits the transaction - when this returns successfully,
	** the transction can no longer be aborted.
	**
	** If a shared transaction, the writing of the ET is delayed
	** until the last sharer COMMITs.
	*/

	et_flags = 0;
	/* Trust that the caller is telling the BT-journaled truth */
	if ( flag & DMXE_JOURNAL )
	    et_flags |= DM0L_JOURNAL;
	if (flag & DMXE_ROTRAN)
	    et_flags = DM0L_ROTRAN;
	if (flag & DMXE_NONJNLTAB)
	    et_flags |= DM0L_NONJNLTAB;
	if ( flag & DMXE_LOGFULL_COMMIT )
	    et_flags |= DM0L_LOGFULL_COMMIT;

	status = dm0l_et(log_id, et_flags, DM0L_COMMIT, 0, 0, ctime, dberr);
	if (status != E_DB_OK)
	    break;

	/*
	** If LOGFULL_COMMIT, LG has preserved log context
	** and we want to do the same for the lock context,
	** so we're done.
	*/
	if ( flag & DMXE_LOGFULL_COMMIT )
	    return(E_DB_OK);

	/* Delay unlocking until after temporary tables have been destroyed */
	if ( flag & DMXE_DELAY_UNLOCK )
	    return (E_DB_OK);

	/*	Release the locks for this transaction. */

	stat = LKrelease(LK_ALL, lock_id, (LK_LKID *)0, 
	    (LK_LOCK_KEY *)0, (LK_VALUE *)0, &sys_err);
	if (stat == OK)
	{
	    /* No trace message if we're just disconnecting */
	    if ((flag & DMXE_CONNECT) == 0 && DMZ_SES_MACRO(1))
		dmd_lock(0, lock_id, LK_RELEASE, LK_ALL, 0, 0, tran_id, 0, 0);
	    return (E_DB_OK);
	}

	/*  Handle lock failure. */

	uleFormat(NULL, stat, &sys_err, ULE_LOG, NULL,
	    (char *)NULL, 0L, (i4 *)NULL, err_code, 0);
	uleFormat(NULL, E_DM901B_BAD_LOCK_RELEASE, &sys_err, ULE_LOG, NULL,
	    (char *)NULL, 0L, (i4 *)NULL, err_code, 1, 0, lock_id);
	SETDBERR(dberr, 0, E_DM9501_DMXE_COMMIT);
	return (E_DB_ERROR);
    }

    /*	Handle error reporting. */
    if (dberr->err_code != E_DM0100_DB_INCONSISTENT)
    {
	uleFormat(dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL, err_code, 0);
	SETDBERR(dberr, 0, E_DM9501_DMXE_COMMIT);
    }
    return (E_DB_ERROR);    
}

/*{
** Name: dmxe_savepoint	- Start a Savepoint.
**
** Description:
**      This function makes the appropriate logging request
**	needed to start a savepoint.
**
** Inputs:
**      xcb                             Pointer to the corresponding xcb.
**      spcb                            Pointer to the SPCB.
**
** Outputs:
**	err_code			The reason for error status.
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR			E_DM9500_DMXE_SAVEPOINT
**	Exceptions:
**
**
** Side Effects:
**	Sets the log addr of begin savepoint log record in spcb.
**
** History:
**	03-nov-1986 (jennifer)
**          Created new for jupiter.
**	08-feb-93 (jnash)
**	    Add flag param to dm0l_savepoint().
**	15-mar-1993 (rogerk)
**	    Reduced Logging - Phase IV:
**	    Changed savepoints to be tracked via LSN's rather than Log addrs.
**	    Removed the xcb_abt_sp_addr stuff which was used when BI's were
**		used for undo processing.
**	23-aug-1993 (jnash)
**	    Don't journal savepoints.  Rollfoward figures out what to do 
**	    from LSN context.
*/
DB_STATUS
dmxe_savepoint(
DML_XCB             *xcb,
DML_SPCB            *spcb,
DB_ERROR	    *dberr)
{
    DB_STATUS	    status;
    i4			*err_code = &dberr->err_code;

    CLRDBERR(dberr);

    dmf_svcb->svcb_stat.tx_savepoint++;
    
    status = dm0l_savepoint(xcb->xcb_log_id, spcb->spcb_id, 0, 
	&spcb->spcb_lsn, dberr);

    if (status == E_DB_OK)
	return (E_DB_OK);
    
    /*	Handle error reporting. */
    if (dberr->err_code != E_DM0100_DB_INCONSISTENT)
    {
	uleFormat(dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL, err_code, 0);
	SETDBERR(dberr, 0, E_DM9502_DMXE_SAVE);
    }
    return (E_DB_ERROR);
}

/*{
** Name: dmxe_writebt - write begin transaction record for already begun
**	    transaction.
**
** Description:
**	This routine writes a begin transaction log record for a transaction
**	already in progress.  When a normal user transaction is begun, it
**	is treated as a nologging transaction (no logging done) until the
**	first write operation is performed.  This means that we don't write
**	the begin transaction record when the transaction is started.  When
**	dmf gets called to perform the first write operation, it calls this
**	routine to write the begin transaction record.
**
**	If the transaction is marked as NOLOGGING, then we don't actually
**	want to write a begin transaction log record.  Do turn off the
**	DELAY bits as update actions have been performed.
**
** Inputs:
**	xcb	    - transaction control block.
**
** Outputs:
**	err_code			The reason for error status.
**					E_DM9504_DMXE_WRITEBT
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    None
**
** Side Effects:
**	None
**
** History:
**	30-sep-1988 (rogerk)
**	    Written.
**	17-mar-1989 (EdHsu)
**	    added cluster online backup support.
**	17-jan-1990 (rogerk)
**	    Added support for delaying savepoint records for read-only
**	    transactions.  When we write the BT record, check also if
**	    we need to write the SAVEPOINT record.  Changed the arguments
**	    to this routine to accept an XCB instead of individual xcb
**	    fields to inact this change.
**	 4-feb-1990 (rogerk)
**	    Added support for NOLOGGING.  Don't write log records in
**	    an XCB_NOLOGGING transaction.
**	22-mar-1991 (bryanp)
**	    Fix B36630: When this transaction writes its BT record, it may need
**	    to promote rcb_bt_bi_logging from 0 to 1 to trigger Btree BI's if
**	    this database is undergoing Online Backup. Call
**	    dm2d_check_db_backup_status() and dm2r_set_rcb_btbi_info() to do
**	    this.
**	15-mar-1993 (rogerk)
**	    Reduced Logging - Phase IV:
**	    Removed obsolete rcb_bt_bi_logging and rcb_merge_logging fields.
**	    Deleted dm2r_set_rcb_btbi_info routine.
**	11-Nov-1997 (jenjo02)
**	    Pass DM0L_DELAYBT to dm0l_bt() when writing a delayed BT.
**	15-Mar-1999 (jenjo02)
**	    Removed DM0L_DELAYBT references.
**	14-Mar-2000 (jenjo02)
**	    No need to write SAVEPOINT log records. Instead, set the
**	    savepoint's LSN to be that of the BT.
**	27-Aug-2004 (jenjo02)
**	    For connected transactions, duplicate the flag and LSN
**	    changes in the Parent transaction's XCB.
**	05-Sep-2006 (jonj)
**	    Pass XCB_ILOG (internal logging) bit to dm0l_bt(DM0L_ILOG).
**      29-nov-2005 (horda03) Bug 48659/INGSRV 2716
**          Added "journaled" parameter. Indicated that the BT record
**          should be journaled.
**      06-feb-2005 (horda03) Bug 48659/INGSRV 2716 Addition
**          If in a non-journaled TX, and a Non-Journaled BT record has
**          already been written, don't write a BT record.
**	11-Sep-2006 (jonj)
**	    Carry horda03's non-journaled BT stuff into Parent's XCB.
**	    If LOGFULL_COMMIT protocols, update XCB's tran_id with
**	    that now known to logging.
**	    Deprecated need for XCB_SVPT_DELAY.
*/
DB_STATUS
dmxe_writebt(
DML_XCB		    *xcb,
i4                  journaled,
DB_ERROR	    *dberr)
{
    DML_XCB		*Pxcb = xcb;
    DB_STATUS		status;
    i4			bt_flags = 0;
    LG_LSN		lsn;
    STATUS		stat;
    CL_ERR_DESC		sys_err;
    DML_ODCB		*odcb;
    DMP_DCB		*dcb;
    i4			*err_code = &dberr->err_code;

    CLRDBERR(dberr);

    /*  Check for read only database. */
    odcb = xcb->xcb_odcb_ptr;
    if (odcb && dm0m_check((DM_OBJECT *)odcb, (i4)ODCB_CB) == 0)
    {
	dcb = (DMP_DCB *)odcb->odcb_db_id;
	if (dcb && dcb->dcb_access_mode == DCB_A_READ)
	{
	    SETDBERR(dberr, 0, E_DM006A_TRAN_ACCESS_CONFLICT);
	    return (E_DB_ERROR);
	}
    }

    /*
    ** If connected to a shared transaction context, we must
    ** duplicate flag and LSN changes in the Parent's XCB
    ** to avoid losing ETs and rollbacks, e.g.
    **
    ** - Child writes the BT and does some updating but Parent's 
    **   xcb_flags & XCB_DELAYBT is still on.
    ** - Child terminates (ET is not actually written until
    **   last connected txn terminates)
    ** - Parent commits, but because its XCB xcb_flags & XCB_DELAYBT
    **   is still on, it appears that no updates have been done,
    **   so ET is not written. Likewise if Parent aborts, recovery
    **   would not be done for the same reason.
    **
    **   Note that for SHARED transactions we utilize the odcb_cq_mutex
    **   to block concurrent updates to the Parent xcb_flags.
    **   See also dmx_logfull_commit which diddles with Parent's
    **   xcb_flags.
    */
    if ( xcb->xcb_flags & XCB_SHARED_TXN )
    {
	odcb = xcb->xcb_odcb_ptr;
	Pxcb = odcb->odcb_scb_ptr->scb_x_next;
	dm0s_mlock(&odcb->odcb_cq_mutex);
    }

    /*
    ** If this is a NOLOGGING transaction, then don't actually log
    ** the begin transaction.  Just turn off the delay flag.
    */
    if (Pxcb->xcb_flags & XCB_NOLOGGING)
    {
	Pxcb->xcb_flags &= ~XCB_DELAYBT;
	xcb->xcb_flags  &= ~XCB_DELAYBT;
	if ( Pxcb != xcb )
	    dm0s_munlock(&odcb->odcb_cq_mutex);
	return (E_DB_OK);
    }

    if ( !journaled && Pxcb->xcb_flags & XCB_NONJNL_BT )
    {
        /*
        ** Already wrote a non-journaled BT record for this
        ** (shared) transaction.
        */
        xcb->xcb_flags |= XCB_NONJNL_BT;
	if ( Pxcb != xcb )
	    dm0s_munlock(&odcb->odcb_cq_mutex);
        return (E_DB_OK);
    }

    if ( journaled && !(Pxcb->xcb_flags & XCB_DELAYBT))
    {
        /*
        ** Already wrote a journaled BT record for this
        ** (shared) transaction.
        */
        xcb->xcb_flags &= ~XCB_DELAYBT;
	if ( Pxcb != xcb )
	    dm0s_munlock(&odcb->odcb_cq_mutex);
        return (E_DB_OK);
    }

    if ( journaled && Pxcb->xcb_flags & XCB_JOURNAL )
    {
	bt_flags = DM0L_JOURNAL;

        if ( Pxcb->xcb_flags & XCB_NONJNL_BT )
        {
	    /* This is the 2nd BT (a journaled one)
	    ** for this (shared) TX.
	    */
	    bt_flags |= DM0L_2ND_BT;
	}
    }
    else if ( Pxcb->xcb_flags & XCB_NONJNL_BT )
    {
        /* TX is not journaled, and we've already written a 
        ** (shared) non-journaled BT record 
        */
        xcb->xcb_flags |= XCB_NONJNL_BT;

        /*
        ** If txn will never journal, shut off the
        ** delaybt flag so we won't keep getting
        ** called.
        */
        if ( !(Pxcb->xcb_flags & XCB_JOURNAL) )
	    xcb->xcb_flags &= ~XCB_DELAYBT;
	if ( Pxcb != xcb )
	    dm0s_munlock(&odcb->odcb_cq_mutex);
        return (E_DB_OK);
    }

    /* If internal logging, let dm0l/lg know it's ok if read-only */
    if ( xcb->xcb_flags & XCB_ILOG )
	bt_flags |= DM0L_ILOG;

    if ( Pxcb != xcb )
	dm0s_munlock(&odcb->odcb_cq_mutex);

    /*
    ** Write a Begin Transaction record.
    */
    status = dm0l_bt(xcb->xcb_log_id, bt_flags, xcb->xcb_lk_id,
	xcb->xcb_odcb_ptr->odcb_dcb_ptr, 
	(DB_OWN_NAME *)xcb->xcb_username.db_own_name, &lsn, dberr);

    if (status == E_DB_OK)
    {
	if ( Pxcb != xcb )
	    dm0s_mlock(&odcb->odcb_cq_mutex);

        if (journaled)
	{
	   Pxcb->xcb_flags &= ~(XCB_DELAYBT | XCB_NONJNL_BT);
	   xcb->xcb_flags  &= ~(XCB_DELAYBT | XCB_NONJNL_BT);
	}
        else
	{
	   /* If another thread didn't write a journaled ET... */
	   if ( Pxcb->xcb_flags & XCB_DELAYBT )
	   {
	       Pxcb->xcb_flags |= XCB_NONJNL_BT;
	       xcb->xcb_flags  |= XCB_NONJNL_BT;
	       /*
	       ** If txn will never journal, shut off the
	       ** delaybt flag so we won't keep getting
	       ** called.
	       */
	       if ( !(Pxcb->xcb_flags & XCB_JOURNAL) )
	       {
		   Pxcb->xcb_flags &= ~XCB_DELAYBT;
		   xcb->xcb_flags  &= ~XCB_DELAYBT;
	       }
	   }
	   else
	       xcb->xcb_flags  &= ~(XCB_DELAYBT | XCB_NONJNL_BT);
	}

	if ( Pxcb != xcb )
	    dm0s_munlock(&odcb->odcb_cq_mutex);

	/*
	** Check if the database is in backup state.  If it is, then we
	** have to log physical log records for all updates, even updates
	** to system catalogs and btree index pages.  Mark the database in
	** BACKUP state so that DMF will do this.
	*/
	status = dm2d_check_db_backup_status( xcb->xcb_odcb_ptr->odcb_dcb_ptr,
					dberr );

	if ( status == E_DB_OK && xcb->xcb_scb_ptr->scb_sess_mask & SCB_LOGFULL_COMMIT )
	{
	    LG_TRANSACTION	tr;
	    i4			length;
	    MEcopy((char*)&xcb->xcb_log_id, sizeof(xcb->xcb_log_id), (char*)&tr);
	    stat = LGshow(LG_S_STRANSACTION, (PTR)&tr, sizeof(tr), &length, &sys_err);
	    if ((stat != OK) || (length == 0))
	    {
		if (stat)
		    uleFormat(NULL, stat, &sys_err, ULE_LOG, NULL,
			(char *)NULL, 0L, (i4 *)NULL, err_code, 0);
		uleFormat(NULL, E_DM9017_BAD_LOG_SHOW, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, 0L, (i4 *)NULL, err_code, 1, 0, LG_S_STRANSACTION);
		status = E_DB_ERROR;
	    }
	    else
		/* Update this XCB's transaction id */
		xcb->xcb_tran_id = tr.tr_eid;
	}
    }

    if ( status && dberr->err_code > E_DM_INTERNAL )
    {
	uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, (char *)NULL,
	    (i4)0, (i4 *)NULL, err_code, 0);
	SETDBERR(dberr, 0, E_DM9504_DMXE_WRITEBT);
    }

    return (status);
}

/*{
** Name: dmxe_secure 	- Prepare to commit a transaction.
**
** Description:
**      This function prepares to commit a transaction by writing 
**	prepare commit log record, writing all the locks held by
**	the transaction to the log file, forcing all the log records 
**	of the transaction to the log file and finally alter the state
**	of the transaction to be WILLING COMMIT. 
**
** Inputs:
**      tran_id                         The transaction id to prepare to commit.
**      dis_tran_id                     The distributed transaction id to 
**					prepare to commit.
**	log_id				The log_id to use.
**      lock_id				The lock_id to use.
** Outputs:
**      err_code                        The reason for an error status.
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR			E_DM9500_DMXE_COMMIT
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	12-Jan-1989 (ac)
**	    Completed code;
**	26-oct-1992 (rogerk)
**	    Added clstatus error reporting.
**      1-Oct-1997 (thaju02) bug#72386 - While there is another session
**          connected to database, aborting a two phase commit transaction
**          via lartool, results in the transaction being committed
**          instead of being aborted. dmfrcp undo's log records and writes
**          changes to disk on behalf of lartool. While there is
**          another session to database, reading stale pages in dmf cache
**          buffer, buffer is written out to disk, ultimately overwriting
**          the 'good' rolled back pages. Calling dm0p_force_pages with
**          action of 0, resulted in buffer being written to disk, yet
**          being left in the cache. Modified dm0p_force_pages() to
**          have action of DM0P_TOSS.
**	30-Apr-2002 (jenjo02)
**	    Moved dm0p_force_pages() from here to dm0l_secure().
**	    Must wait to force transaction's pages until all
**	    HANDLE transactions have prepared and the global
**	    transaction is ready to be put in WILLING COMMIT
**	    to avoid DM9C01 page-still-fixed failures.
*/
DB_STATUS
dmxe_secure(
DB_TRAN_ID         *tran_id,
DB_DIS_TRAN_ID     *dis_tran_id,
i4		    log_id,
i4		    lock_id,
DB_ERROR	    *dberr)
{
    DB_STATUS		status;
    STATUS		stat;
    CL_ERR_DESC		sys_err;
    i4		length;
    LG_TRANSACTION	tr;
    LG_CONTEXT		ctx;
    i4			*err_code = &dberr->err_code;

    CLRDBERR(dberr);

    /* Reject the secure request if in the cluster environment. */

    stat = CXcluster_enabled();
    if (stat != OK)
    {
	uleFormat(NULL, stat, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
	    (char *)NULL, 0L, (i4 *)NULL, err_code, 0);
	SETDBERR(dberr, 0, E_DM0122_CANT_SECURE_IN_CLUSTER);
	return (E_DB_ERROR);    
    }
    /* 
    ** Check if the distributed transaction has already exist in the logging
    ** system.
    */
    
    MEcopy((char *)dis_tran_id, sizeof(DB_DIS_TRAN_ID), (char *)&tr.tr_dis_id);
    tr.tr_id = log_id;
    
    stat = LGshow(LG_S_DIS_TRAN, (PTR)&tr, sizeof(tr), &length, &sys_err);
    if (stat != OK)
    {
	uleFormat(NULL, stat, &sys_err, ULE_LOG, NULL,
	    (char *)NULL, 0L, (i4 *)NULL, err_code, 0);
	uleFormat(NULL, E_DM9017_BAD_LOG_SHOW, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, 0L, (i4 *)NULL, err_code, 1, 0, 
		    LG_S_DIS_TRAN);
	SETDBERR(dberr, 0, E_DM9505_DMXE_WILLING_COMMIT);
	return (E_DB_ERROR);
    }
    if (length)
    {
	/* 
	** The distributed transaction has already associated with a local 
	** transaction. Reject the distributed transaction.
	*/

	SETDBERR(dberr, 0, E_DM0119_TRAN_ID_NOT_UNIQUE);
	return (E_DB_ERROR);
    }

    /*
    ** Alter this transaction branch to PREPARED.
    ** When all branches have done so, LGalter returns
    ** LG_CONTINUE, at which time we'll make sure that
    ** all of the global transaction's pages have been forced.
    **
    ** Notice that even after we've done this, the branches
    ** are abortable; we never enter the WILLING_COMMIT state
    ** because we retain the log/lock contexts until the
    ** final XA_COMMIT rolls around.
    */
    ctx.lg_ctx_id = tr.tr_id;

    stat = LGalter(LG_A_PREPARE, (PTR)&ctx, sizeof(ctx), &sys_err);

    if ( stat == LG_CONTINUE )
    {
	status = dm0l_secure(log_id, lock_id, tran_id, dis_tran_id, dberr);

	if (status != E_DB_OK)
	{
	    if (dberr->err_code > E_DM_INTERNAL)
	    {
		uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, (char *)NULL,
		    (i4)0, (i4 *)NULL, err_code, 0);
		SETDBERR(dberr, 0, E_DM9505_DMXE_WILLING_COMMIT);
	    }
	}
    }
    else if ( stat )
    {
	uleFormat(NULL, stat, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, 0L, (i4 *)NULL, err_code, 0);
	uleFormat(NULL, E_DM900B_BAD_LOG_ALTER, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, 0L, (i4 *)NULL, err_code, 1, 0, 
		    LG_A_PREPARE);
	SETDBERR(dberr, 0, E_DM012B_ERROR_XA_PREPARE);
    }
    else
	status = E_DB_OK;

    return (status);
}

/*{
** Name: disconnect_willing_commit - Handle the disconnection of the 
**				     FE/coordinator for the willing commit 
**				     slave transaction.
**
** Description:
**      This function handles the disconnection of the FE/coordinator for
**	the willing commit slave transaction by flushing and invalidating
**	all the dirty pages of the transaction. The transaction context
**	and all the locks are still preserved and held by the logging and
**	locking system. Because all the locks are still held by the 
**	transaction, all the changes on the database will be backed out if
**	the final status ofthe transaction is ABORT.
**
** Inputs:
**      tran_id                         The transaction id to disconnect.
**      lock_id				The lock_id to use.
**	log_id				The log_id to use.
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
**	12-Jan-1989 (ac)
**	    Completed code;
**	26-oct-1992 (rogerk)
**	    Changed dm0p_tran_invalidate call to use dm0p_force_pages.
**	    Modified Pages can no longer be indiscriminatly thrown from the
**	    cache prior to an abort as BI recovery is no longer used.
**	    Added clstatus error reporting.
*/
DB_STATUS
disconnect_willing_commit(
DB_TRAN_ID          *tran_id,
i4		    lock_id,
i4		    log_id,
DB_ERROR	    *dberr)
{
    DB_STATUS		status;
    STATUS		stat;
    CL_ERR_DESC		sys_err;
    i4			*err_code = &dberr->err_code;

    CLRDBERR(dberr);

    status = dm0p_force_pages(lock_id, tran_id, log_id, DM0P_TOSS, dberr);
    if (status != E_DB_OK)
    {
	if (dberr->err_code > E_DM_INTERNAL)
	{
	    uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, (char *)NULL,
		(i4)0, (i4 *)NULL, err_code, 0);
	    SETDBERR(dberr, 0, E_DM9506_DMXE_DISCONNECT);
	}
	return (status);
    }
    /*	Alter the state of the tranaction to WAIT FOR RE-ASSOCIATIN. */
 
    stat = LGalter(LG_A_REASSOC, (PTR)&log_id, sizeof(log_id), &sys_err);
    if (stat != OK)
    {
	uleFormat(NULL, stat, &sys_err, ULE_LOG, NULL,
	    (char *)NULL, 0L, (i4 *)NULL, err_code, 0);
	uleFormat(NULL, E_DM900B_BAD_LOG_ALTER, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, 0L, (i4 *)NULL, err_code, 1, 0, 
		    LG_A_REASSOC);
	SETDBERR(dberr, 0, E_DM9506_DMXE_DISCONNECT);
	return (E_DB_ERROR);
    }    
    return(E_DB_OK);
}

/*{
** Name: dmxe_resume	- Resume a willing commit transaction.
**
** Description:
**      This function resume a willing commit transaction by re-gaining
**	the context of the transaction in the logging system and assuming
**	all the locks held by the transaction.
**
** Inputs:
**      dis_tran_id                     The distributed transaction id to 
**					re-associated.
** Outputs:
**	dcb_log_id			The logging system database id.
**	related_lock			The related lock list.
**	log_id				The logging system id.
**      tran_id                         The assigned transaction_id.
**	lock_id				The assigned lock id.
**	err_code			The reason for error status.
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
**	1-Feb-1989 (ac)
**	    Completed code;
**	26-oct-1992 (rogerk)
**	    Added clstatus error reporting.
**	15-mar-1993 (rogerk)
**	    Reduced Logging - Phase IV:
**	    Changed LG_S_DBID to require only the external database id to be
**		passed in as a handle to find db information, rather than
**		the full dbinfo buffer.
**	08-sep-93 (swm)
**	    Changed sid type from i4 to CS_SID to match recent CL
**	    interface revision.
**	18-oct-93 (rogerk)
**	    Changed LG_A_OWNER interface so that it expects the db_id of the
**	    caller's open database context.  We no longer scavange the opendb
**	    from the orphaned transaction, only the transaction itself.  Also
**	    removed the sid parameter which was passed in the tr_lpd_id field;
**	    this value is now looked up inside LGalter.
*/
DB_STATUS
dmxe_resume(
DB_DIS_TRAN_ID	   *dis_tran_id,
DMP_DCB		   *dcb,
i4		   *related_lock,
i4		   *log_id,
DB_TRAN_ID         *tran_id,
i4		   *lock_id,
DB_ERROR	   *dberr)
{
    CL_ERR_DESC		sys_err;
    STATUS		stat;
    i4		length;
    LG_TRANSACTION	tr;
    LK_LLB_INFO		llb;
    i4		context;
    LG_DATABASE		ldb;
    i4			*err_code = &dberr->err_code;

    CLRDBERR(dberr);

    /* Get the db_id of the transaction. */

    ldb.db_database_id = dcb->dcb_id;

    stat = LGshow(LG_S_DBID, (PTR)&ldb, sizeof(ldb), &length, &sys_err);
    if (stat != OK)
    {
	uleFormat(NULL, stat, &sys_err, ULE_LOG, NULL,
	    (char *)NULL, 0L, (i4 *)NULL, err_code, 0);
	uleFormat(NULL, E_DM9017_BAD_LOG_SHOW, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, 0L, (i4 *)NULL, err_code, 1, 0, 
		    LG_S_DBID);
	SETDBERR(dberr, 0, E_DM9507_DMXE_RESUME);
	return (E_DB_ERROR);
    }
    if (length == 0)
    {
	/* 
	** The database s unknown to the logging system.
	** Reject the resume operation.
	*/

	SETDBERR(dberr, 0, E_DM0134_DIS_DB_UNKNOWN);
	return (E_DB_ERROR);
    }
    
    /* 
    ** Check if the distributed transaction has already existed in the logging
    ** system and return the information of the transaction if exist.
    */

    MEcopy((char *)dis_tran_id, sizeof(DB_DIS_TRAN_ID), (char *)&tr.tr_dis_id);
    tr.tr_db_id = ldb.db_id;

    stat = LGshow(LG_A_CONTEXT, (PTR)&tr, sizeof(tr), &length, &sys_err);
    if (stat != OK)
    {
	uleFormat(NULL, stat, &sys_err, ULE_LOG, NULL,
	    (char *)NULL, 0L, (i4 *)NULL, err_code, 0);
	uleFormat(NULL, E_DM9017_BAD_LOG_SHOW, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, 0L, (i4 *)NULL, err_code, 1, 0, 
		    LG_A_CONTEXT);
	SETDBERR(dberr, 0, E_DM9507_DMXE_RESUME);
	return (E_DB_ERROR);
    }
    if (length == 0)
    {
	/* 
	** The distributed transaction has already been aborted or committed.
	** Reject the resume operation.
	*/

	SETDBERR(dberr, 0, E_DM0120_DIS_TRAN_UNKNOWN);
	return (E_DB_ERROR);
    }
    
    if (((tr.tr_status & TR_RECOVER) == 0 && 
	    (tr.tr_status & TR_REASSOC_WAIT) == 0
        ) ||
	(tr.tr_status & TR_RESUME)
       )
    {
	/* 
	** If the transaction is not owned by the RCP and the server that 
	** owns the transaction is not aware the disconnection or if the
	** transaction has been resumed, signal the error.
	*/

	SETDBERR(dberr, 0, E_DM0121_DIS_TRAN_OWNER);
	return (E_DB_ERROR);
    }

    *log_id = tr.tr_id;
    STRUCT_ASSIGN_MACRO( (*((DB_TRAN_ID *)&(tr.tr_eid))), *tran_id);

    /* 
    ** Change the owner of the transaction to the new server.
    */
    tr.tr_lpd_id = dcb->dcb_log_id;
    tr.tr_pr_id = dmf_svcb->svcb_lctx_ptr->lctx_lgid;

    stat = LGalter(LG_A_OWNER, (PTR)&tr, sizeof(tr), &sys_err);
    if (stat != OK)
    {
	uleFormat(NULL, stat, &sys_err, ULE_LOG, NULL,
	    (char *)NULL, 0L, (i4 *)NULL, err_code, 0);
	uleFormat(NULL, E_DM900B_BAD_LOG_ALTER, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, 0L, (i4 *)NULL, err_code, 1, 0, 
		    LG_A_OWNER);
	SETDBERR(dberr, 0, E_DM9507_DMXE_RESUME);
	return (E_DB_ERROR);
    }

    /* Get the lock list of the WILLING COMMIT transaction. */

    stat = LKcreate_list((i4)LK_RECOVER, (LK_LLID)0, 
			(LK_UNIQUE *)&(tr.tr_eid), 
			(LK_LLID *)lock_id, 0,
		        &sys_err);
    if (stat != OK)
    {
	uleFormat(NULL, stat, &sys_err, ULE_LOG, NULL,
	    (char *)NULL, 0L, (i4 *)NULL, err_code, 0);
	(VOID) uleFormat(NULL, E_DM901A_BAD_LOCK_CREATE, &sys_err, ULE_LOG, NULL, 
	    (char *)NULL, 0L, (i4 *)NULL, err_code, 0);

	SETDBERR(dberr, 0, E_DM9507_DMXE_RESUME);
	return(E_DB_ERROR);
    }
    
    /* Release the current session lock list. */
    
    stat = LKrelease(LK_ALL, *related_lock, (LK_LKID *)0, (LK_LOCK_KEY *)0,
		    (LK_VALUE *)0, &sys_err);
    if (stat != OK)
    {
	uleFormat(NULL, stat, &sys_err, ULE_LOG, NULL,
	    (char *)NULL, 0L, (i4 *)NULL, err_code, 0);
	uleFormat( NULL, E_DM901B_BAD_LOCK_RELEASE, &sys_err, ULE_LOG , NULL, 
	            (char * )0, 0L, (i4 *)NULL, err_code, 1, 
		    sizeof(*related_lock), *related_lock);
	SETDBERR(dberr, 0, E_DM9507_DMXE_RESUME);
	return(E_DB_ERROR);
    }

    /* Get the related lock list of the WILLING COMMIT transaction. */

    stat = LKshow(LK_S_LLB_INFO, *lock_id, 0, 0, sizeof(llb),
		(PTR)&llb, (u_i4 *)&length, 
		(u_i4 *)&context, &sys_err);
    if (stat != OK)
    {
	uleFormat(NULL, stat, &sys_err, ULE_LOG, NULL,
	    (char *)NULL, 0L, (i4 *)NULL, err_code, 0);
	uleFormat(NULL, E_DM901D_BAD_LOCK_SHOW, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, 0L, (i4 *)NULL, err_code, 1, 0, 
		    LK_S_LLB_INFO);
	SETDBERR(dberr, 0, E_DM9507_DMXE_RESUME);
	return (E_DB_ERROR);
     }
     *related_lock = llb.llb_r_llb_id;
     return (E_DB_OK);
}

/*{
** Name: dmxe_check_abort	- Check if transaction is in ABORT state
**
** Description:
**	Returns information on whether the current transaction is in
**	active or abort state.
**
**	Calls the Logging System to query the status of the current
**	transaction.  Returns abort = TRUE if the transaction is in
**	one of the following states:
**
**		TR_RECOVER 
**		TR_FORCE_ABORT 
**		TR_SESSION_ABORT 
**		TR_SERVER_ABORT 
**		TR_PASS_ABORT 
**		TR_MAN_ABORT
**
**	If the current transaction context is unknown to the logging system, an
**	error will be logged and a warning will be returned.  The abort_state
**	return value will be FALSE.
**
** Inputs:
**	log_id				The logging system id for this session.
**					re-associated.
** Outputs:
**	abort_state			TRUE if transaction is being aborted,
**					FALSE otherwise.
**	Returns:
**	    E_DB_OK
**	    E_DB_WARN			Transaction state invalid.
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	19-aug-1991 (rogerk)
**	    Written as part of bug fix for bug #39017.  Added method for
**	    buffer manager to request transaction state to determine whether
**	    to escalate to table-level locking.  We don't want to escalate
**	    when processing transaction aborts as it can lead to deadlock.
**	26-oct-1992 (rogerk)
**	    Added clstatus error reporting.
*/
DB_STATUS
dmxe_check_abort(
i4		log_id,
bool		*abort_state)
{
    LG_TRANSACTION	tr;
    CL_ERR_DESC		sys_err;
    STATUS		status;
    i4		length;
    i4		error;

    /*
    ** Call LGshow to get transaction status.
    **
    ** LG_S_ATRANSACTION call requires the log_id to be written into
    ** the tr buffer on entry to LGshow.  On exit, the tr buffer will
    ** hold the transaction information.
    */
    MEcopy((char *)&log_id, sizeof(log_id), (char *)&tr);

    status = LGshow(LG_S_ATRANSACTION, (PTR)&tr, sizeof(tr), &length, &sys_err);
    if ((status != OK) || (length == 0))
    {
	if (status != OK)
	{
	    uleFormat(NULL, status, &sys_err, ULE_LOG, NULL,
		(char *)NULL, 0L, (i4 *)NULL, &error, 0);
	}
	uleFormat(NULL, E_DM9017_BAD_LOG_SHOW, &sys_err, ULE_LOG, NULL,
	    (char *)NULL, 0L, (i4 *)NULL, &error, 1, 0, LG_S_ATRANSACTION);
	*abort_state = FALSE;
	return (E_DB_WARN);
    }

    /*
    ** Check transaction status for one of the abort states.
    */
    *abort_state = FALSE;
    if (tr.tr_status & (TR_RECOVER | TR_FORCE_ABORT | TR_SESSION_ABORT |
		 TR_SERVER_ABORT | TR_PASS_ABORT | TR_MAN_ABORT))
    {
	*abort_state = TRUE;
    }

    return (E_DB_OK);
}

/*{
** Name: dmxe_pass_abort - Pass Abort of Transaction to Recovery Process
**
** Description:
**
** Inputs:
**	log_id				The log_id to use.
**      lock_id				The lock_id to use.
**      tran_id				The transaction Id.
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
**	none
**
** History:
**      26-oct-1992 (rogerk)
**	    Created for Reduced Logging Project.
**	    Code moved from dmxe_abort routine.
**      21-may-1997 (stial01)
**          Disallow PASS-ABORT for row locking transaction.
**	08-Sep-1999 (jenjo02)
**	    LKshow(LK_S_LLB_INFO) changed to LK_S_LIST. LK_S_LLB_INFO
**	    only returns info of lock list's related list, if any, causing
**	    erroneous E_DM93F5_ROW_LOCK_PROTOCOL errors because LK_LLB_INFO
**	    is garbage!
*/
DB_STATUS
dmxe_pass_abort(
i4		    log_id,
i4		    lock_id,
DB_TRAN_ID	    *tran_id,
i4		    db_id,
DB_ERROR	    *dberr)
{
    DB_STATUS		status = E_DB_OK;
    STATUS		stat = OK;
    CL_ERR_DESC		sys_err;
    i4             length;
    i4             context;
    LK_LLB_INFO         llb;
    i4			*err_code = &dberr->err_code;

    CLRDBERR(dberr);

    for (;;)
    {
	/*
	** Disallow PASS-ABORT of row locking transaction
	** The RCP CANNOT do PASS-ABORT of a row locking transaction.
	** That is because when the RCP inherits IX page locks, it
	** does not OWN the pages that it updates. (The RCP has its own 
	** private cache). There is no guarantee that a dbms server is 
	** not simultaneously operating on a page the RCP is trying to recover.
	*/
	stat = LKshow(LK_S_LIST, lock_id, 0, 0, sizeof(llb),
		    (PTR)&llb, (u_i4 *)&length, 
		    (u_i4 *)&context, &sys_err);
	if (stat != OK)
	{
	    uleFormat(NULL, stat, &sys_err, ULE_LOG, NULL,
		(char *)NULL, 0L, (i4 *)NULL, err_code, 0);
	    uleFormat(NULL, E_DM901D_BAD_LOCK_SHOW, &sys_err, ULE_LOG, NULL,
			(char *)NULL, 0L, (i4 *)NULL, err_code, 1, 0, 
			LK_S_LIST);
	    SETDBERR(dberr, 0, E_DM9503_DMXE_ABORT);
	    status = E_DB_ERROR;
	    break;
	}

	if (llb.llb_status & LLB_XROW)
	{
	    uleFormat(NULL, E_DM93F5_ROW_LOCK_PROTOCOL, NULL, ULE_LOG, NULL, 
			NULL, 0, NULL, err_code, 0);
	    SETDBERR(dberr, 0, E_DM9503_DMXE_ABORT);
	    status = E_DB_ERROR;
	    break;
	}

	/*
	** Log the Pass Abort action.
	*/
	/*XXXX add force abort error message */

	/* 
	** Turn off server aborting the xact status so that recovery 
	** process can take over to abort the xact.
	*/
	stat = LGalter(LG_A_OFF_SERVER_ABORT, (PTR)&log_id, sizeof(log_id), 
								    &sys_err);
	if (stat != E_DB_OK)
	{
	    uleFormat(NULL, stat, &sys_err, ULE_LOG, NULL,
		(char *)NULL, 0L, (i4 *)NULL, err_code, 0);
	    uleFormat(NULL, E_DM900B_BAD_LOG_ALTER, &sys_err, ULE_LOG, NULL,
		(char *)NULL, 0L, (i4 *)NULL, err_code, 1, 
		0, LG_A_OFF_SERVER_ABORT);
	    SETDBERR(dberr, 0, E_DM9503_DMXE_ABORT); /* XXXX */
	    status = E_DB_ERROR;
	    break;
	}

	/*
	** Inform the buffer manager that the RCP is about to begin
	** abort processing for this server.  The Buffer Manager will not
	** bring any unlocked pages into the cache through group reads or
	** read-nolock queries until recovery is complete.
	*/
	dm0p_pass_abort(FALSE);

	/*
	** Transaction was not aborted, let the recovery process try
	** to abort.  First we need to flush any pages that will be
	** used by the RCP.  Also toss them from our cache since they
	** may be changed by the RCP.
	*/
	status = dm0p_force_pages(lock_id, tran_id, log_id, 
	    (i4)DM0P_TOSS, dberr);
	if (status != E_DB_OK)
	    break;

	/*
	** Continue flushing out pages needed by the RCP.  Write out
	** all modified buffers for this database which are not protected
	** by transaction locks.  These pages may be required by the
	** RCP, but were not forced out by the tran_invalidate call
	** above.
	*/
	dm0p_wpass_abort(db_id, lock_id, log_id);

	/*
	** Need to clear out any pages in the buffer cache that may
	** be invalidated by RCP.  Any page for this database that is
	** unfixed and unmodified may have been brought into the cache
	** without a lock (through group faulting or readnolock) and may
	** have been modified in the transaction that is about to be
	** aborted.
	**
	** Note that all pages updated by the aborting transaction may not
	** be on the transaction's hash queue.  A modified page may be
	** bumped from the cache and then read back in by a read-nolock
	** session.
	*/
	dm0p_toss_pages(db_id, (i4)0, lock_id, log_id, 0);

	/*
	** Need to toss any unused TCB's from the TCB cache.  Any TCB that
	** is not currently referenced could potentially have been used
	** in a DMU operation in the transaction being aborted.  Part of
	** the recovery action for DMU operations is to purge the TCB
	** from the cache as it may no longer reflect the true state
	** of the table after the DMU is backed out.
	**
	** Any TCB which is currently referenced could not be used in
	** a DMU operation as it is locked (even in the READ-NOLOCK case)
	** by the session referencing it.
	*/
	status = E_DB_OK;
	while (status == E_DB_OK)
	{
	    /*
	    ** Loop calling reclaim_tcb until there are no more tcb's
	    ** to reclaim.
	    */
	    status = dm2t_reclaim_tcb(lock_id, log_id, dberr);

	    if ((status != E_DB_OK) && 
		(dberr->err_code != E_DM9328_DM2T_NOFREE_TCB))
	    {
		uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
		    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	    }
	}

	/*
	** The loop above is expected to have ended with a DM9328 error.
	** Anything else indicates a real error.
	*/
	if (dberr->err_code != E_DM9328_DM2T_NOFREE_TCB)
	    break;
	status = E_DB_OK;
	CLRDBERR(dberr);

	/* Let recovery process try to abort the transaction again. */
    
	stat = LGalter(LG_A_PASS_ABORT, (PTR)&log_id, sizeof(log_id), &sys_err);
	if (stat != OK)
	{
	    uleFormat(NULL, stat, &sys_err, ULE_LOG, NULL,
		(char *)NULL, 0L, (i4 *)NULL, err_code, 0);
	    uleFormat(NULL, E_DM900B_BAD_LOG_ALTER, &sys_err, ULE_LOG, NULL,
		(char *)NULL, 0L, (i4 *)NULL, err_code, 1, 0, LG_A_PASS_ABORT);
	    SETDBERR(dberr, 0, E_DM9503_DMXE_ABORT); /* XXXX */
	    status = E_DB_ERROR;
	    break;
	}

	/*
	** Inform the buffer manager that the RCP has now been signaled
	** to perform recovery.  The Buffer manager can now turn off its
	** PASS_ABORT status when it sees that recovery is no longer in
	** progress.
	*/
	dm0p_pass_abort(TRUE);

	return (E_DB_OK);
    }

    uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
	(char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
    SETDBERR(dberr, 0, E_DM9503_DMXE_ABORT); /* XXXX */

    return(E_DB_ERROR);
}

/*{
** Name: dmxe_xn_active	- Check if a transaction is active
**
** Description:
**	Calls the Logging System to query the status of a transaction
**
** Inputs:
**      tran_id                         The external low tran id
**
** Returns 
**	TRUE/FALSE
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          Created.
*/
bool
dmxe_xn_active(
u_i4		low_tran)
{
    LG_TRANSACTION	tr;
    CL_ERR_DESC		sys_err;
    STATUS		status;
    i4		length;
    i4		error;

    if (low_tran == 0)
    {
	return(FALSE);
    }

    /*
    ** Call LGshow to get transaction status.
    **
    ** LG_S_ETRANSACTION call requires the LOW transaction id to be written
    ** into the tr buffer on entry to LGshow.
    ** On exit, the tr buffer will hold the transaction information.
    */
    tr.tr_eid.db_low_tran = low_tran;

    status = LGshow(LG_S_ETRANSACTION, (PTR)&tr, sizeof(tr), &length, &sys_err);
    if (status != OK)
    {
	uleFormat(NULL, status, &sys_err, ULE_LOG, NULL,
	    (char *)NULL, 0L, (i4 *)NULL, &error, 0);
	uleFormat(NULL, E_DM9017_BAD_LOG_SHOW, &sys_err, ULE_LOG, NULL,
	    (char *)NULL, 0L, (i4 *)NULL, &error, 1, 0, LG_S_ATRANSACTION);
	return (TRUE);
    }

    /* Check if transaction known to logging system */
    if (length == 0)
	return(FALSE);
    else
	return(TRUE);
}
