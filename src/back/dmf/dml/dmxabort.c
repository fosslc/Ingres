/*
**Copyright (c) 2004, 2010 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <cm.h>
#include    <me.h>
#include    <st.h>
#include    <tr.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <adf.h>
#include    <pc.h>
#include    <usererror.h>
#include    <dmf.h>
#include    <ulf.h>
#include    <lg.h>
#include    <lk.h>
#include    <dmrcb.h>
#include    <dmxcb.h>
#include    <dmccb.h>
#include    <dmtcb.h>
#include    <dm.h>
#include    <dml.h>
#include    <dmp.h>
#include    <dm2t.h>
#include    <dm0m.h>
#include    <dmxe.h>
#include    <dmftrace.h>
#include    <dm0p.h>
#include    <scf.h>

/**
** Name: DMXABORT.C - Functions used to abort a Transaction.
**
** Description:
**      This file contains the functions necessary to abort a transaction.
**      This file defines:
**    
**      dmx_abort()        -  Routine to perform the abort 
**                            transaction operation.
**
** History:
**      01-sep-85 (jennifer)
**          Created for Jupiter.      
**	21-jul-86 (Derek)
**	    Upgrade for full logging support.
**	30-sep-1988 (rogerk)
**	    Added capability to treat transactions as read-only until first
**	    write operation is performed.  If DELAYBT flag is still on in the
**	    XCB at abort time, then treat the transaction as a read-only
**	    transaction.
**	30-Jan-89 (ac)
**	    Added 2PC support.
**	17-jan-1990 (rogerk)
**	    Added capability to not log savepoints until first write operation
**	    is performed.
**	 4-feb-1991 (rogerk)
**	    Added support for Set Nologging.  If abort is called on a Nologging
**	    transaction then report and return an error.
**	25-feb-1991 (rogerk)
**	    Changed Set Nologging abort error from an ERUSF error number to
**	    a DMF error number to follow error number conventions.
**	25-mar-1991 (rogerk)
**	    Changed the way nologging transactions are aborted.  Now flag is
**	    passed down to DMXE_BEGIN which will cause the database to be
**	    marked inconsistent.
**	13-may-1991 (rogerk)
**	    Check for database inconsistency errors being returned from
**	    dmxe_abort call.  We should remove our DMF transaction
**	    data structures before returning the error to ensure that
**	    the session can be removed from the server without getting
**	    open-transaction errors.
**	19-aug-1991 (rogerk)
**	    Bug # 38403
**	    Fix error cleaning up after failed Set Nologging transaction.
**	    Don't attempt abort processing for Nologging session on an
**	    Abort to Savepoint request.
**	17-oct-1991 (rogerk)
**	    Great Merge of DMF : Move 6.4 changes into 6.5.  In this case,
**	    the 6.4 version of this file was moved directly to 6.5 and the
**	    6.5 changes were merged back in:
**
**	    14-jun-1991 (Derek)
**		Added performance profiling support.
**	16-jan-1992 (bryanp)
**	    Temporary table enhancements: drop open temp tables during abort.
**	8-jul-1992 (walt)
**	    Prototyping DMF.
**	3-oct-1992 (bryanp)
**	    Re-arrange includes to share transaction ID type definition.
**	26-oct-1992 (rogerk)
**	    Reduced Logging Project:  Checked for errors during abort-to-svpt
**	    operations.  If we fail during an attempt to rollback to a savepoint
**	    we now mark the transaction as requiring a full rollback.  Otherwise
**	    the database is left in an unknown state.
**	15-mar-1993 (rogerk)
**	    Reduced Logging - Phase IV:
**	    Changed Savepoints to be tracked by LSN rather than Log Address.
**	    Removed old xcb_abt_sp_addr needed with BI recovery.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**		Pass a NULL DCB pointer to dmxe_abort.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (bryanp)
**	    Disallow abort to savepoint if the transaction is already prepared.
**	    Include <lk.h> to pick up locking system type definitions.
**	    Include <pc.h> before <lk.h> to pick up PID definition
**	20-sep-1993 (andys)
**	    When we force abort, print an informative message to the errlog.log
**	    This is bug 49861 in 6.4.
**	18-oct-1993 (rogerk)
**	    When a temporary table is open by more than rcb in this transaction,
**	    only pass the close-destroy request on the last close of the temp
**	    table.  This change was necessitated by new dm2t_release_tcb logic
**	    which waits for busy tcb's rather than pending a destroy action.
**	31-jan-1994 (bryanp) B58558, B58561
**	    Check return code from dm2t_destroy_temp_tcb.
**	    Check return code from LGshow.
**      04-apr-1995 (dilma04)
**        (Re)Initialize scb_isolation_level.
**	30-may-96 (stephenb)
**	    Check for and clean up (close table for) replication input queue
**	    RCB.
**	05-nov-1996 (somsa01)
**	    In dmx_abort(), if a temporary (etab) table is being deleted, take
**	    its dmt_cb off of the scb list so that it will not try to be
**	    deleted again in dmpe_free_temps().
**      12-feb-97 (dilma04)
**          Set Lockmode Cleanup:
**          Replaced scb_isolation_level with scb_tran_iso.
**	27-Feb-1997 (jenjo02)
**	    Extended <set transaction statement> to support <transaction 
**	    access mode> (READ ONLY | READ WRITE) per SQL-92.
**	    Added SET SESSION <access mode>
**	    Reset scb_tran_am.
**	3-jun-97 (nanpr01)
**	    Fix bug 78352 : When transactions are rolled back, temporary
**	    tables that were updated does not get dropped as documented.
** 	17-jun-1997 (wonst02)
** 	    Changes for readonly database.
**	25-Aug-1997 (jenjo02)
**	    Added log_id parm to dm2t_destroy_temp_tcb() calls.
**      12-nov-97 (stial01)
**          dmx_abort() Reset scb_tran_iso = DMC_C_SYSTEM;
**	11-Aug-1998 (jenjo02)
**	    Clear rcb_tup_adds, rcb_page_adds to cancel effects
**	    of this txn on a table.
**	29-dec-1998 (stial01)
**          dmx_abort() if taking DMT_CB off scb->scb_lo_next queue,
**          call dm0m_deallocate().
**	15-Dec-1999 (jenjo02)
**	    Support for SHARED log transactions, multiple txns
**	    scheduled to be aborted (dmx_show()).
**      18-jan-2000 (gupsh01)
**          Added setting DMXE_NONJNLTAB in order to identify transactions
**          which alter nonjournaled tables of a journaled database.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	08-Nov-2000 (jenjo02)
**	    Deleted xcb_tq_next|prev, xccb_tq_next|prev.
**	    To find session temp tables which need to be destroyed,
**	    examine XCCB list on ODCB.
**	10-sep-2001 (somsa01)
**	    Corrected previous cross-integration by changing nat's
**	    and longnat's to i4's.
**	12-Oct-2001 (devjo01)
**	    In dmx_abort, fixed casting of some pointers to ule_format,
**	    and more importantly corrected one call to dm0m_deallocate
**	    which was being passed a xccb, where *xccb was intended.
**	05-Nov-2001 (hanje04)
**	    Backed out changes related to bug 103567 becuase all global
**	    temp tables are being removed by rollback.
**	05-Mar-2002 (jenjo02)
**	    Call dms_end_tran() for Sequence Generators.
**	08-apr-2002 (somsa01)
**	    Added back definition of "odcb", as we use it for other reasons.
**	08-Jul-2002 (jenjo02)
**	    dmx_show() Pass transaction's log id as well as tran_id. (b108208)
**      22-oct-2002 (stial01)
**          Added dmx_xa_abort()
**      18-feb-2004 (stial01)
**          Fixed incorrect casting of length arguments.
**      01-jun-2005 (stial01)
**          dmx_xa_abort() get odcb from dmx_db_id, dmx_tran_id may not be init.
**	26-Jun-2006 (jonj)
**	    Deleted dmx_xa_abort.
**      29-nov-2005 (horda03) Bug 48659/INGSRV 2716
**          Only need to journal an ET record for an aborted TX if there was a
**          journaled BT record logged.
**      26-jan-2006 (horda03) Bug 48659/INGSRV 2716 Additional
**          A non-journalled DB will the XCB_DELAYBT flag set when it has updated
**          a table (i.e. it is not a ROTRAN), in this circumstance the XCB_NONJNL_BT
**          flag will also be set.
**      16-oct-2006 (stial01)
**          Free locator context.
**      09-feb-2007 (stial01)
**          Moved locator context from DML_XCB to DML_SCB
**	12-Apr-2008 (kschendel)
**	    Need to include adf.h now.
**	20-Oct-2008 (jonj)
**	    SIR 120874 Modified to use new DB_ERROR based uleFormat 
**	21-Nov-2008 (jonj)
**	    SIR 120874: dmxe_? functions converted to DB_ERROR *
**	28-Nov-2008 (jonj)
**	    SIR 120874: dm2t_? functions converted to DB_ERROR *
**/

static	DB_STATUS   empty_temporary_tables();


/*{
** Name: dmx_abort - Aborts a transaction.
**
**  INTERNAL DMF call format:      status = dmx_abort(&dmx_cb);
**
**  EXTERNAL call format:          status = dmf_call(DMX_ABORT,&dmx_cb);
**
** Description:
**    The dmx_abort function handles the aborting of a transaction.
**    This includes aborting to a savepoint previously defined by a 
**    dmx_savepoint operation.  This function causes all internal control
**    information associated with this transaction to be released.  This
**    will automatically close all the opens of tables associated with this 
**    transaction.
**
**    If DMX_ABORT is called on an update NOLOGGING transaction, then the
**    transaction context will be removed, the database will be marked
**    inconsistent, and an error will be returned.  Since no logging was 
**    performed, no recovery is done.  The error returned will 
**    be E_DM0146_SETLG_XCT_ABORT.
**
**    If a rollback to savepoint is requested on a NOLOGGING transaction, then
**    no abort will be done, and an error will be returned.  The database will
**    not be marked inconsistent at that time nor will the transaction context
**    be removed.  It is expected that the DMF client will, upon failure to
**    rollback to the savepoint, request a transaction rollback.  The database
**    will then be marked inconsistent following the rules stated above.
**
**	Any temporary table which is open for update by this transaction is
**	automatically dropped. This is because operations against temporary
**	tables are not logged, and so they can't be backed out, and since we
**	don't want to allow the users to access any potentially corrupt
**	temporary tables, we drop the table. 
**
** Inputs:
**      dmx_cb 
**          .type                           Must be set to DMX_TRANSACTION_CB.
**          .length                         Must be at least sizeof(DMX_CB).
**          .dmx_tran_id.                   Must be a valid transaction 
**                                          identifier returned from the 
**                                          begin transaction operation.
**          .dmx_option                     DMX_SP_ABORT if abort to savepoint
**					    DMX_XA_END_FAIL if aborting XA txn.
**          .dmx_savepoint_name             Must be zero or if dmx_option 
**                                          is set to DMX_SP_ABORT 
**                                          a valid savepoint name 
**                                          returned from a transaction
**                                          savepoint operation.
**          
**
** Output:
**      dmx_cb 
**          .error.err_code                 One of the following error numbers.
**                                          E_DM0000_OK                
**                                          E_DM000B_BAD_CB_LENGTH
**                                          E_DM000C_BAD_CB_TYPE
**                                          E_DM001A_BAD_FLAG
**                                          E_DM002C_BAD_SAVEPOINT_NAME
**                                          E_DM003B_BAD_TRAN_ID
**                                          E_DM004A_INTERNAL_ERROR
**                                          E_DM004B_LOCK_QUOTA_EXCEEDED
**                                          E_DM0065_USER_INTR
**                                          E_DM0064_USER_ABORT
**					    E_DM0096_ERROR_ABORTING_TRAN
**                                          E_DM0102_NONEXISTENT_SP
**                                          E_DM0100_DB_INCONSISTENT
**                                          E_DM010C_TRAN_ABORTED
**                                          E_DM0112_RESOURCE_QUOTA_EXCEEDED
**                                          E_DM0146_SETLG_XCT_ABORT
**                     
**
**      Returns:
**          E_DB_OK                         Function completed normally. 
**          E_DB_WARN                       An abort to savepoint request was
**					    overriden by FORCE_ABORT and
**					    the complete transaction recovered.
**          E_DB_ERROR                      Function completed abnormally with 
**                                          a termination status which is in
**                                          dmx_cb.error.err_code.
**          E_DB_FATAL                      Function completed with a fatal
**                                          error which must be handled
**                                          immediately.  The fatal status is in
**                                          dmx_cb.error.err_code.
** History:
**      01-sep-85 (jennifer)
**          Created for Jupiter.
**	21-jul-1986 (Derek)
**	    Filled in the code.
**	30-sep-1988 (rogerk)
**	    Added capability to treat transactions as read-only until first
**	    write operation is performed.  If DELAYBT flag is still on in the
**	    XCB at abort time, then treat the transaction as a read-only
**	    transaction.
**	30-Jan-89 (ac)
**	    Added 2PC support.
**	17-jan-1990 (rogerk)
**	    Added capability to not log savepoints until first write operation
**	    is performed.  If abort to savepoint and there has not been a
**	    savepoint record written, then treat the operation as a read-only
**	    abort.
**	 4-feb-1991 (rogerk)
**	    Added support for Set Nologging.  If abort is called on a Nologging
**	    transaction then report and return an error.
**	25-mar-1991 (rogerk)
**	    Changed the way nologging transactions are aborted.  Now flag is
**	    passed down to DMXE_BEGIN which will cause the database to be
**	    marked inconsistent.
**	13-may-1991 (rogerk)
**	    Check for database inconsistency errors being returned from
**	    dmxe_abort call.  In this case, we will have released the
**	    transaction context from the system even though we cannot
**	    continue the rollback.  We should remove our DMF transaction
**	    data structures before returning the error to ensure that
**	    the session can be removed from the server without getting
**	    open-transaction errors.
**	19-aug-1991 (rogerk)
**	    Bug # 38403
**	    Fix error cleaning up after failed Set Nologging transaction.
**	    Don't attempt abort processing for Nologging session on an
**	    Abort to Savepoint request.  Return an error, but don't mark
**	    the database inconsistent.  When the caller sees the abort
**	    error, it will probably escalate the abort to the transaction
**	    level, at which time the database will be marked inconsistent.
**	    Marking the database inconsistent during savepoint processing
**	    would cause us to release the LG context without cleaning up
**	    the XCB.  This would leave us in a state where we were unable
**	    to clean up the session and close the database properly.
**	17-oct-1991 (rogerk)
**	    Great Merge of DMF : Move 6.4 changes into 6.5.  In this case,
**	    the 6.4 version of this file was moved directly to 6.5 and the
**	    6.5 changes were merged back in:
**
**	    14-jun-1991 (Derek)
**		Added performance profiling support.
**	16-jan-1992 (bryanp)
**	    Temporary table enhancements: when closing the open tables, if
**	    a table is a temporary table which was opened for update access,
**	    then pass DM2T_DESTROY to dm2t_close to cause the table to be
**	    dropped. Call empty_temporary_tables() to handle other actions.
**	26-oct-1992 (rogerk)
**	    Reduced Logging Project:  Checked for errors during abort-to-svpt
**	    operations.  If we fail during an attempt to rollback to a savepoint
**	    we now mark the transaction as requiring a full rollback.  Otherwise
**	    the database is left in an unknown state.
**	15-mar-1993 (rogerk)
**	    Reduced Logging - Phase IV:
**	    Changed Savepoints to be tracked by LSN rather than Log Address.
**	    Removed old xcb_abt_sp_addr needed with BI recovery.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**		Pass a NULL DCB pointer to dmxe_abort.
**	26-jul-1993 (bryanp)
**	    Disallow abort to savepoint if the transaction is already prepared.
**	20-sep-1993 (andys)
**	    When we force abort, print an informative message to the errlog.log
**	    This is bug 49861 in 6.4.
**	18-oct-1993 (rogerk)
**	    When a temporary table is open by more than rcb in this transaction,
**	    only pass the close-destroy request on the last close of the temp
**	    table.  This change was necessitated by new dm2t_release_tcb logic
**	    which waits for busy tcb's rather than pending a destroy action.
**	31-jan-1994 (bryanp) B58558, B58561
**	    Check return code from dm2t_destroy_temp_tcb.
**	7-jul-94 (robf)
**          Purge RCB on close if write transaction and peripheral table,
**	    this is to work around a problem where the peripheral information
**	    in the TCB could become out of sync on an ABORT causing subsequent
**	    attempts to reference a non-existent extension table. 
**      04-apr-1995 (dilma04)
**          (Re)Initialize scb_isolation_level.
**	30-may-96 (stephenb)
**	    Check for and clean up (close table for) replication input queue
**	    RCB.
**	05-nov-1996 (somsa01)
**	    If a temporary (etab) table is being deleted, take its dmt_cb off
**	    of the scb list so that dmpe_free_temps() will not try to delete
**	    it again.
**      12-feb-97 (dilma04)
**          Set Lockmode Cleanup:
**          Replaced scb_isolation_level with scb_tran_iso.
**	27-Feb-1997 (jenjo02)
**	    Extended <set transaction statement> to support <transaction 
**	    access mode> (READ ONLY | READ WRITE) per SQL-92.
**	    Added SET SESSION <access mode>
**	    Reset scb_tran_am.
**	13-mar-96 (stephenb)
**	    remove input queue RCB from the XCB after aborting
**	3-jun-97 (nanpr01)
**	    Fix bug 78352 : When transactions are rolled back, temporary
**	    tables that were updated does not get dropped as documented.
** 	17-jun-1997 (wonst02)
** 	    Changes for readonly database.
**	30-jul-97 (nanpr01)
**	    Rollback to savepoint shouldnot cause the temporary table to be
**	    deleted if the temporary table was not updated between savepoint
**	    and now. Also, here, if the temp table rcb is in xcb_rq,
**	    it used to get dropped on last reference is not required.
**      12-nov-97 (stial01)
**          dmx_abort() Reset scb_tran_iso = DMC_C_SYSTEM;
**	27-mar-98 (stephenb)
**	    Only clean up the xcb_rep_input_q pointer if we are aborting
**	    the whole transaction, not just rolling back to a savepoint.
**	11-Aug-1998 (jenjo02)
**	    Clear rcb_tup_adds, rcb_page_adds to cancel effects
**	    of this txn on a table.
**      31-Aug-98 (wanya01)
**          Bug 77435: IPM session detail display shows "Performing Force
**          Abort Processing" as activity for session which is executing a
**          normal rollback. Take out change 415103 which fix bug 62550 in
**          dmxe.c. Add scf_call in dmx_abort and set force abort message
**          only when abort transaction due to reach force abort limit. 
**      18-jan-2000 (gupsh01)
**          Added setting DMXE_NONJNLTAB in order to identify transactions
**          which alter nonjournaled tables of a journaled database.
**	07-Mar-2000 (jenjo02)
**	    If rollback to savepoint requested (from qet_abort()) but the
**	    transaction has been selected for FORCE_ABORT, abort the 
**	    entire transaction and return E_DB_WARN that this has happened.
**      22-May-2000 (islsa01)
**          Bug 90402 : calling a function dm0p_check_logfull to see if
**          logfile is full. If the logfile is full and the event is force
**          abort, retain the existing error message. If the  logfile is not
**          full and the event is force abort, then report a new error message
**          called E_DM011F_TRAN_ABORTED.
**	08-Nov-2000 (jenjo02)
**	    Deleted xcb_tq_next|prev, xccb_tq_next|prev.
**	    To find session temp tables which need to be destroyed,
**	    examine XCCB list on ODCB.
**	21-Dec-2000 (thaju02)
**	    (hanje04) X-Integ of change 451613
**	    In abort to savepoint, if gtt was created after savepoint
**	    then gtt must be destroyed. (B103567)
**      14-Sep-2001 (thaju02)
**          Ammend the above change to prevent all gtts from being dropped
**          when a rollback is issued. (B105755)
**	26-Mar-2002 (thaju02)
**	    Backed out previous changes for bugs 103567 and 105755, caused
**	    regression.
**	12-Oct-2001 (devjo01)
**	    Correct casting of some pointers to ule_format, and more 
**	    importantly correct one call to dm0m_deallocate which
**	    was being passed a xccb, where *xccb was intended.
**	05-Nov-2001 (hanje04)
**	    Removed changed related to bug 103567 because all global 
**	    temp table are being removed by rollback.
**	06-Mar-2003 (jenjo02)
**	    Destroy sequence references only if aborting entire txn.
**	14-Mar-2003 (jenjo02)
**	    Set RCB_ABORTING in each RCB. This alters locking
**	    semantics in dm0p during recovery (B109842).
**	10-Mar-2004 (schka24)
**	    When dropping a session temp due to rollback, make sure
**	    that the table definition is cleared out of RDF.
**	10-May-2004 (schka24)
**	    Clean up blob data and blob temps.
**	11-Jun-2004 (schka24)
**	    Use temp destroy utility;  mutex the odcb cq list.
**	22-Jul-2004 (jenjo02)
**	    After full abort, log/lock contexts are gone; clear
**	    xcb_log_id, xcb_lk_id for any subsequent functions
**	    (like dmt_destroy_temp).
**	11-Nov-2005 (jenjo02)
**	    Removed dm0p_check_logfull(). Transactions are
**	    force-aborted to keep the log from becoming full,
**	    hence dm0p_check_logfull() isn't the right check
**	    to make. When the txn is marked XCB_FORCE_ABORT,
**	    it really is; do DM9059 to show the txn that has
**	    been thwarted.
**      26-jan-2006 (horda03) Bug 48659/INGSRV 2716 Additional
**          Check that the TX has the XCB_DELAYBT flag set, but not the 
**          XCB_NONJNL_BT flag when deciding if the TX is Read Only
**          (DMXE_ROTRAN) or not.
**	11-Sep-2006 (jonj)
**	    Removed check for XCB_SVPT_DELAY.
**	30-Oct-2006 (jonj)
**	    dmx_option may have bits other than simply DMX_SP_ABORT.
**	8-Aug-2007 (kibro01) b118915 
**	    Add in flag to allow lock release to take place after the
**	    abort, so that transaction temporary tables can keep their
**	    lock until they are destroyed.
**	9-Oct-2007 (kibro01 on behalf of kschendel) b119277
**	    Don't set delay-release flag (b118915) if willing commit, since
**	    that essentially implies delayed release already.
**      16-Jun-2009 (hanal04) Bug 122117
**          Call LKalter() to clear down the LLB_FORCEABORT flag if set.
**          We may need to wait for locks as part of the abort processing.
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Free xcb_lctx_ptr, xcb_jctx_ptr memory
**	    if allocated.
**	23-Mar-2010 (kschendel) SIR 123448
**	    XCB's XCCB list now has to be mutex protected.
**	13-Apr-2010 (kschendel) SIR 123485
**	    Force LOB query-end upon abort.
**	23-Sep-2010 (jonj) B124486
**	    dmxe_abort will return E_DM9509_DMXE_PASS_ABORT
**	    if rollback failed. If it does, don't
**	    release the lock list; the RCP needs it.
*/
DB_STATUS
dmx_abort(
DMX_CB    *dmx_cb)
{
    DMX_CB		*dmx = dmx_cb;
    DMP_DCB             *dcb;
    DML_SCB             *scb;
    DML_XCB		*xcb;
    DML_SPCB		*spcb;
    DML_SPCB		*end;
    DMT_CB		*dmt_cb;
    DMT_CB              *next_dmt_cb;
    DMP_RCB		*rcb;
    DB_STATUS		return_status = E_DB_OK, status;
    i4		abort = DMXE_DELAY_UNLOCK;
    i4		savepoint_id = 0;
    i4		error,local_error;
    i4		close_flags;
    DML_XCCB            *xccb, *next_xccb;
    bool		nologging_abort_error = FALSE;
    bool		db_inconsist_error = FALSE;
    char                buffer[80];
    SCF_CB              scf_cb;
    DML_ODCB		*odcb;
    CL_ERR_DESC		sys_err;

    CLRDBERR(&dmx->error);

    for (;;)
    {
	/*  Check parameters. */

	xcb = (DML_XCB *)dmx->dmx_tran_id;
	if (dm0m_check((DM_OBJECT *)xcb, (i4)XCB_CB) != E_DB_OK)
	{
	    SETDBERR(&dmx->error, 0, E_DM003B_BAD_TRAN_ID);
	    break;
	}

        scb = xcb->xcb_scb_ptr;
	odcb = xcb->xcb_odcb_ptr;

        /* Now that the session has acknowledged any force-abort, clear the
        ** LK flag used to bring force-abort to the session's attention.
        ** Abort may need to wait for locks as part of abort processing.
        */
        status = LKalter(LK_A_NOFORCEABORT, scb->scb_lock_list, &sys_err);
        if(status != E_DB_OK)
        {
            break;
        }

	/*
	** Check for NOLOGGING and remember nologging state for later.
	** If a NOLOGGING transaction is aborted, we will log and
	** return an error after completing abort processing.
	**
	** Only do this if update operations have been performed
	** (db is not readonly, xact is not RONLY, and DELAY_BT is not set).
	*/
	if ((xcb->xcb_flags & XCB_NOLOGGING) &&
	    (odcb->odcb_dcb_ptr->dcb_access_mode != DMC_A_READ) &&
	    ((xcb->xcb_x_type & XCB_RONLY) == 0) &&
	    ((xcb->xcb_flags & XCB_DELAYBT) == 0))
	{
	    nologging_abort_error = TRUE;
	    uleFormat(NULL, E_DM9051_ABORT_NOLOGGING, (CL_ERR_DESC *)NULL, ULE_LOG,
		(DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, (i4 *)NULL, 
		&local_error, 1,
		sizeof(odcb->odcb_dcb_ptr->dcb_name.db_db_name),
		odcb->odcb_dcb_ptr->dcb_name.db_db_name);
	}

	/* session/txn marked by for FORCE ABORT? */
	if ( xcb->xcb_state & XCB_FORCE_ABORT ||
	     scb->scb_ui_state & SCB_FORCE_ABORT )
	{
	    /*
	    ** Ensure XCB_FORCE_ABORT state is in the XCB.
	    ** Depending on which facility "caught" the FORCE_ABORT
	    ** condition first, only SCB_FORCE_ABORT may be
	    ** on in scb_ui_state (dmxCheckForInterrupt may not
	    ** have been called by DMF to set XCB_FORCE_ABORT).
	    ** This lets the remainder of this code rely on
	    ** XCB_FORCE_ABORT.
	    */
	    xcb->xcb_state |= XCB_FORCE_ABORT;

	    /*
	    ** log some information about the transaction we are aborting
	    */
	    uleFormat(NULL, E_DM9059_TRAN_FORCE_ABORT, (CL_ERR_DESC *)NULL, 
	      ULE_LOG, NULL, (char *)NULL, (i4)0, 
	      (i4 *)NULL, &local_error, 3,
	      sizeof(odcb->odcb_dcb_ptr->dcb_name.db_db_name),
	      odcb->odcb_dcb_ptr->dcb_name.db_db_name,
	      0, xcb->xcb_tran_id.db_high_tran,
	      0, xcb->xcb_tran_id.db_low_tran);

	    scf_cb.scf_length = sizeof(scf_cb);
	    scf_cb.scf_type = SCF_CB_TYPE;
	    scf_cb.scf_ascii_id = SCF_ASCII_ID;
	    scf_cb.scf_facility = DB_DMF_ID;
	    scf_cb.scf_session = DB_NOSESSION;
	    scf_cb.scf_ptr_union.scf_buffer = (PTR) buffer;
	    scf_cb.scf_nbr_union.scf_atype = SCS_A_MAJOR;

            STprintf(buffer, "Performing Force Abort Processing");
            scf_cb.scf_len_union.scf_blength = STlength(buffer);
            (VOID) scf_call(SCS_SET_ACTIVITY, &scf_cb);

	    /*
	    ** qet_abort() is unaware of the force abort state and
	    ** if its recovery action is to rollback the current
	    ** statement, it's called dmx_abort with an abort-to-savepoint.
	    **
	    ** FORCE_ABORT always aborts the entire transaction, so
	    ** if abort-to-savepoint, override it and abort the
	    ** complete transaction. Return E_DB_WARN to let the
	    ** caller (qet_abort()) know that we've done this.
	    **
	    ** If we just do the abort-to-savepoint, we'll be called
	    ** back soon to abort the entire transaction anyway for
	    ** FORCE_ABORT (by scsqncr) and upgrading the abort now 
	    ** saves writing an ABORTSAVE log record (which isn't the 
	    ** best thing to be doing in a LOGFULL state) then re-reading 
	    ** the transaction's log records just to find the corresponding
	    ** SAVEPOINT record from which transaction recovery would
	    ** begin.
	    */
	    if ( dmx->dmx_option & DMX_SP_ABORT )
	    {
		dmx->dmx_option = 0;
		return_status = E_DB_WARN;
	    }
	}

	/*  Check the savepoint id. */

	if ( dmx->dmx_option & DMX_SP_ABORT )
	{
	    if (xcb->xcb_state & XCB_WILLING_COMMIT)
	    {
		SETDBERR(&dmx->error, 0, E_DM0132_ILLEGAL_STMT);
		break;
	    }

	    /* If state is marked as transaction abort, cannot abort
            ** to savepoint, must abort transaction.
            */
	    if (xcb->xcb_state & XCB_ABORT)
	    {
		if (xcb->xcb_state & XCB_FORCE_ABORT)
		{
		    SETDBERR(&dmx->error, 0, E_DM010C_TRAN_ABORTED);
		    break;
		}
		else if (xcb->xcb_state & XCB_TRANABORT)
		{
		    SETDBERR(&dmx->error, 0, E_DM0064_USER_ABORT);
		    break;
		}
	    }

	    /*	Lookup the savepoint. */

	    for (end = (DML_SPCB*) &xcb->xcb_sq_next, spcb = xcb->xcb_sq_prev;
		spcb != end;
		spcb = spcb->spcb_q_prev)
	    {
		if (MEcmp((char *)&spcb->spcb_name, 
                    (char *)&dmx->dmx_savepoint_name,
                    sizeof(spcb->spcb_name)) == 0)
		{
		    break;
		}
	    }

	    if (spcb == end)
	    {
		SETDBERR(&dmx->error, 0, E_DM0102_NONEXISTENT_SP);
		break;
	    }

	    savepoint_id = spcb->spcb_id;
	}
        /*
        ** clean up replication input queue RCB. We do this only if we opened
	** it durig the relevant savepoint window (the entitr TX if none).
        */
        if (xcb->xcb_rep_input_q) /* input queue now open */
        {
	    if ( !(dmx->dmx_option & DMX_SP_ABORT) || 
		(spcb && (spcb->spcb_flags & SPCB_REP_IQ_OPEN) == 0))
	    {
		/* input queue not open at start of savepoint window */
		if (xcb->xcb_rep_input_q != (DMP_RCB *)-1)
		{
		    /* not opened directly by user */
		    xcb->xcb_rep_input_q->rcb_tup_adds  = 0;
		    xcb->xcb_rep_input_q->rcb_page_adds = 0;
		    xcb->xcb_rep_input_q->rcb_state |= RCB_ABORTING;
                    status = dm2t_close(xcb->xcb_rep_input_q, (i4)0, 
			&dmx->error);
            	    if (status != E_DB_OK)
                	break;
		}
	    	xcb->xcb_rep_input_q = 0;
	    }
        }

	/*  Clear user interrupt state in SCB. */

	xcb->xcb_scb_ptr->scb_ui_state &= ~SCB_USER_INTR;

	/*
	** If this transaction is being FORCE ABORTed,
	** clear the force abort state of the session. 
	*/
	if ( xcb->xcb_state & XCB_FORCE_ABORT )
	    xcb->xcb_scb_ptr->scb_ui_state &= ~SCB_FORCE_ABORT;
	
        if (savepoint_id == 0 && (xcb->xcb_flags & XCB_USER_TRAN) != 0)
        {
	    /*
            ** forget any transaction-duration isolation level
	    ** and access mode:
	    */
            scb->scb_tran_iso = DMC_C_SYSTEM;
	    scb->scb_tran_am = 0;
        }

	/* Cleanup any Sequence Generator references if txn abort */
	if ( savepoint_id == 0 && (xcb->xcb_seq || xcb->xcb_cseq) )
	    status = dms_end_tran(DMS_TRAN_ABORT, xcb, &dmx->error);

	/* BLOB query is ended one way or the other now.  Don't however
	** drop holding temps, if this is a rollback inside a DBP the
	** outermost query might still have valid holding temps around.
	** Do this before forcible table close so that we can close any
	** opens attached to the BQCB, nicely.
	** Unclear what should be done about error here, just ignore it.
	*/
	(void) dmpe_query_end(TRUE, FALSE, &dmx->error);

	/* Clean up any dangling blob memory */
	while (xcb->xcb_pcb_list != NULL)
	    dmpe_deallocate(xcb->xcb_pcb_list);

	/*  
	** Close all open tables and destroy all open temporary tables.
	** If the transaction is in the WILLING COMMIT state, all the
	** tables should have been closed at the WILLING COMMIT time.
	*/

	while (xcb->xcb_rq_next != (DMP_RCB*) &xcb->xcb_rq_next)
	{
	    /*	Get next RCB. */

	    rcb = (DMP_RCB *)((char *)xcb->xcb_rq_next -
		    (char *)&(((DMP_RCB*)0)->rcb_xq_next));

	    /*	Remove from the XCB. */

	    rcb->rcb_xq_next->rcb_q_prev = rcb->rcb_xq_prev;
	    rcb->rcb_xq_prev->rcb_q_next = rcb->rcb_xq_next;

	    /* Clear tuple and page counts */
	    rcb->rcb_tup_adds  = 0;
	    rcb->rcb_page_adds = 0;

	    /* Notify RCB users of abort */
	    rcb->rcb_state |= RCB_ABORTING;

	    /*
	    ** If the table has extensions which may have changed 
	    ** then we must purge since peripheral information may have
	    ** been invalidated by the abort. We only do this if TCB open
	    ** count is one or less to prevent a purge if in use, this does
	    ** leave a window where another session could see the
	    ** invalid peripheral information until the purge but catches
	    ** the common case of the current user being the only one
	    ** accessing the table. Long term the handling of the tcb_et_list
	    ** and syncronization on error should be looked at.
	    */
	    if(rcb->rcb_access_mode == RCB_A_WRITE &&
	       (rcb->rcb_tcb_ptr->tcb_rel.relstat2 & TCB2_HAS_EXTENSIONS) &&
		rcb->rcb_tcb_ptr->tcb_open_count <= 1)
	    {
		close_flags = DM2T_PURGE;
	    }
	    else
		close_flags = 0;

	    /*	Deallocate the RCB. */

	    status = dm2t_close(rcb, close_flags, &dmx->error);
	    if (status != E_DB_OK)
	    {
		if ((dmx->error.err_code != E_DM004B_LOCK_QUOTA_EXCEEDED) &&
			(dmx->error.err_code != E_DM0042_DEADLOCK) &&
			(dmx->error.err_code != E_DM004D_LOCK_TIMER_EXPIRED) &&
			(dmx->error.err_code != E_DM004A_INTERNAL_ERROR))
		{
		    uleFormat( &dmx->error, 0, NULL, ULE_LOG , NULL, 
				    (char * )NULL, 0L, (i4 *)NULL, 
				    &local_error, 0);
		    SETDBERR(&dmx->error, 0, E_DM0096_ERROR_ABORTING_TRAN);
		}
		return (E_DB_FATAL);
	    }
	}
	/* One more swipe at the BQCB's now that all tables are closed.
	** This call definitively deletes BQCB's.
	** (dmpe-query-end perhaps should have been named "dmpe-query-end-
	** as-much-as-you-can-for-now"!  The call above is there to neatly
	** close out loads and logical-key-generators.  This call, after
	** forcible table close, guarantees deletion of the BQCB's.)
	*/
	(void) dmpe_query_end(TRUE, FALSE, &dmx->error);

	/*
	** Get abort flags.
	**
	** If XCB_DELAYBT state is still turned on, then there was no
	** write operation done during this transaction, so it can be
	** treated as a readonly transaction.
        ** But a TX can still have the XCB_DELAYBT set for a an UPDATE
        ** TX if a non-journalled BT record has been written.
	*/
	if ( xcb->xcb_x_type & XCB_RONLY || 
	     odcb->odcb_dcb_ptr->dcb_access_mode == DMC_A_READ ||
	    (xcb->xcb_flags & (XCB_DELAYBT|XCB_NONJNL_BT)) == XCB_DELAYBT )
	{
	    abort |= DMXE_ROTRAN;
	}
        /* Only need to journal the ET record, if there is a BT journaled
        ** record.
        */
	if ( (xcb->xcb_odcb_ptr->odcb_dcb_ptr->dcb_status & DCB_S_JOURNAL) &&
             !(xcb->xcb_flags & XCB_NONJNL_BT))
	    abort |= DMXE_JOURNAL;
	if (xcb->xcb_state & XCB_DISCONNECT_WILLING_COMMIT)
	{
	    /* Don't delay unlock if willing-commit txn, since we can't
	    ** unilaterally abort such transactions anyway and the lock
	    ** list stays around (kibro01) b119277
	    */
	    abort &= ~DMXE_DELAY_UNLOCK;
	    abort |= DMXE_WILLING_COMMIT;
	}
    	if (xcb->xcb_flags & XCB_NONJNLTAB)
	    abort |= DMXE_NONJNLTAB;
	/*
	** If transaction is marked as NOLOGGING, then pass the DMXE_NOLOGGING
	** flag which will cause the database to be marked inconsistent.
	** (Unless no real work was done (DMXE_ROTRAN) - in which case the
	** transaction will just be removed without causing any inconsistency)
	**
	** If this is an abort-to-savepoint request, then pass in the
	** ROTRAN flag to indicate not really to do any abort processing,
	** and to avoid marking the database inconsistent at this point.
	** We will return an error back to the caller and if the caller
	** decides to terminate the transaction, we will mark the database
	** inconsistent at that point.
	*/
	if (xcb->xcb_flags & XCB_NOLOGGING)
	{
	    abort |= DMXE_NOLOGGING;
	    if (savepoint_id)
		abort |= DMXE_ROTRAN;
	}
    
	/*
	** Call the physical layer to undo any database changes.
	*/
	status = dmxe_abort(odcb, (DMP_DCB *)0, &xcb->xcb_tran_id,
	    xcb->xcb_log_id, xcb->xcb_lk_id, abort,
	    savepoint_id, &spcb->spcb_lsn, &dmx->error); 
	if (status != E_DB_OK)
	{
	    if ( dmx->error.err_code != E_DM004B_LOCK_QUOTA_EXCEEDED &&
	    	 dmx->error.err_code != E_DM0096_ERROR_ABORTING_TRAN &&
	         dmx->error.err_code != E_DM004D_LOCK_TIMER_EXPIRED &&
	         dmx->error.err_code != E_DM0100_DB_INCONSISTENT &&
	         dmx->error.err_code != E_DM010C_TRAN_ABORTED &&
		 dmx->error.err_code != E_DM004A_INTERNAL_ERROR &&
		 dmx->error.err_code != E_DM9509_DMXE_PASS_ABORT )
	    {
		uleFormat( &dmx->error, 0, NULL, ULE_LOG , NULL, 
		    (char * )NULL, 0L, (i4 *)NULL, 
		    &local_error, 0);
		SETDBERR(&dmx->error, 0, E_DM0096_ERROR_ABORTING_TRAN);
	    }

	    /*
	    ** If we failed during an abort-to-savepoint operation then
	    ** the database is in an unknown and inconsistent state
	    ** (since an unknown amount of the transaction may have been
	    ** rolled back).  The only way to bring the database to a
	    ** consistent state is to complete the rollback.
	    **
	    ** We mark the transaction as requiring a full rollback.
	    */
	    if ( dmx->dmx_option & DMX_SP_ABORT )
	    {
		xcb->xcb_state |= XCB_TRANABORT;
	    }

	    /*
	    ** If pass-abort, do NOT release lock list; it's needed
	    ** by the RCP to recover the transaction.
	    */
	    if ( dmx->error.err_code == E_DM9509_DMXE_PASS_ABORT )
	        abort &= ~DMXE_DELAY_UNLOCK;
	    else
	    {
		/*
		** Break to bottom to return the abort error unless DB_INCONSISTENT
		** is returned.  In this case, the transaction context will have
		** been successfully removed even though the abort could not
		** be properly processed (since the database is inconsistent).
		** So we fall through to complete processing of the XCB cleanup.
		** This will allow us to remove this session later without getting
		** "transaction in progress" errors.
		*/
		if (dmx->error.err_code != E_DM0100_DB_INCONSISTENT)
		    break;

		db_inconsist_error = TRUE;
		abort &= ~DMXE_DELAY_UNLOCK; /* Locklist already gone if error */

		/* Transaction context is gone, ignore savepoints */
		dmx->dmx_option &= ~DMX_SP_ABORT;
		savepoint_id = 0;
	    }
	}

	/*
	** Clean up the internal data structures associated with this
	** transaction.  If abort to savepoint, only need to clean up
	** the structures that refer to operations since the savepoint.
	*/

	/*
	**  Flush the savepoint list of savepoints newer then the aborted
	**  savepoint.
	*/

	while (xcb->xcb_sq_prev != (DML_SPCB*)&xcb->xcb_sq_next)
	{
	    /*	Get previous SPCB. */

	    spcb = xcb->xcb_sq_prev;

	    /*	Check for end of list. */

	    if (spcb->spcb_id <= savepoint_id)
		break;

	    /*	Remove SPCB from XCB queue. */

	    spcb->spcb_q_next->spcb_q_prev = spcb->spcb_q_prev;
	    spcb->spcb_q_prev->spcb_q_next = spcb->spcb_q_next;

	    /*	Deallocate the SPCB. */

	    dm0m_deallocate((DM_OBJECT **)&spcb);
	}

	/* 
	**  Flush the transaction's pending operation list 
	**  for items newer then the savepoint that was 
	**  just aborted.
	**  Don't bother with the xcb_cq_mutex here.  It's for
	**  protecting the main session thread's xccb list when
	**  children put stuff on it.  At this point, the children
	**  had better be done doing "stuff" or the abort won't be right.
        */

	for ( xccb = xcb->xcb_cq_next;
	      xccb != (DML_XCCB *)&xcb->xcb_cq_next;
	      xccb = next_xccb )
	{
	    next_xccb = xccb->xccb_q_next;

	    /* 
            ** Only remove the xccb if it was created after
            ** the savepoint. 
	    */

	    if (xccb->xccb_sp_id < savepoint_id)
		continue; 		

	    /*	Remove from queue. */

	    xccb->xccb_q_prev->xccb_q_next = xccb->xccb_q_next;
	    xccb->xccb_q_next->xccb_q_prev = xccb->xccb_q_prev;

	    /* Must back out the temporary table creates. */
	    if (xccb->xccb_operation == XCCB_T_DESTROY)
	    {
		status = dmt_destroy_temp(scb, xccb, &dmx->error);
		if (status != E_DB_OK && 
			dmx->error.err_code != E_DM0054_NONEXISTENT_TABLE)
	        {
		    uleFormat( &dmx->error, 0, (CL_ERR_DESC *)NULL, ULE_LOG , NULL, 
			(char * )NULL, 0L, (i4 *)NULL, &local_error, 0);
		}
	    }
    
	    /*	Deallocate. */

	    dm0m_deallocate((DM_OBJECT **)&xccb);
	}

	/*
	** Destroy any session temp tables that were updated
	** after this savepoint.
	**
	** See code in dm2t/dm2r which is reponsible for setting
	** the current savepoint in the XCCB when a session
	** temp table is updated.
	*/
	dcb = odcb->odcb_dcb_ptr;

	dm0s_mlock(&odcb->odcb_cq_mutex);
	for ( xccb = odcb->odcb_cq_next;
	      xccb != (DML_XCCB*)&odcb->odcb_cq_next;
	      xccb = next_xccb )
	{
	    next_xccb = xccb->xccb_q_next;

	    if ( xccb->xccb_operation == XCCB_T_DESTROY &&
		 xccb->xccb_sp_id >= savepoint_id )
	    {

		/*  Remove from ODCB's list */
		xccb->xccb_q_prev->xccb_q_next = xccb->xccb_q_next;
		xccb->xccb_q_next->xccb_q_prev = xccb->xccb_q_prev;
		dm0s_munlock(&odcb->odcb_cq_mutex);
     
		status = dmt_destroy_temp(scb, xccb, &dmx->error);
		if (status != E_DB_OK && 
			dmx->error.err_code != E_DM0054_NONEXISTENT_TABLE)
	        {
		    uleFormat( &dmx->error, 0, (CL_ERR_DESC *)NULL, ULE_LOG , NULL, 
			(char * )NULL, 0L, (i4 *)NULL, &local_error, 0);
		}

		/*	Deallocate. */
		dm0m_deallocate((DM_OBJECT **)&xccb);
		/* Re-lock list and keep going */
		dm0s_mlock(&odcb->odcb_cq_mutex);
	    }
	}
	dm0s_munlock(&odcb->odcb_cq_mutex);
    
	/*  Deallocate the XCB if this an abort. */

	if (savepoint_id == 0)
	{
	    i4 stat;
	    CL_ERR_DESC sys_err;
	    /* Drop the locks now (delayed from earlier) */

	    if (abort & DMXE_DELAY_UNLOCK)
	    {
		/* Drop the locks now (delayed from earlier)
		** If not delaying, don't release!  Might be willing
		** commit txn, or inconsistent-db cleanup (kibro01) b119277
		*/
                stat = LKrelease(LK_ALL, xcb->xcb_lk_id, (LK_LKID *)0,
                                (LK_LOCK_KEY *)0, (LK_VALUE *)0, &sys_err);


	        /* Log any failure, but this is already an abort, so do the
	        ** same as in dmxe.c where we FALL THROUGH TO RETURN OK 
	        */
                if (stat != OK)
                {
                    uleFormat(NULL, stat, &sys_err, ULE_LOG, NULL, 
		    		NULL, 0, NULL, &error, 0);
                    uleFormat(NULL, E_DM901B_BAD_LOCK_RELEASE, &sys_err, ULE_LOG, NULL,
                        (char*)NULL, (i4)0, (i4*)NULL, &error, 1, 0, xcb->xcb_lk_id);
                    uleFormat(NULL, E_DM9503_DMXE_ABORT, &sys_err, ULE_LOG, NULL,
                                NULL, 0, NULL, &error, 0);
	        }
	    }
	    xcb->xcb_log_id = xcb->xcb_lk_id = 0;

	    /*  Remove XCB from SCB queue. */

	    xcb->xcb_q_next->xcb_q_prev = xcb->xcb_q_prev;
	    xcb->xcb_q_prev->xcb_q_next = xcb->xcb_q_next;
	    scb->scb_x_ref_count--;

#ifdef xDEBUG
	    /*
	    ** Report statistics collected in transaction control block.
	    */
	    if (DMZ_SES_MACRO(20))
		dmd_txstat(xcb, 2);
#endif

	    /* Deallocate lctx, jctx, if allocated */
	    if ( xcb->xcb_lctx_ptr )
	        dm0m_deallocate(&xcb->xcb_lctx_ptr);
	    if ( xcb->xcb_jctx_ptr )
		(void) dm0j_close(&xcb->xcb_jctx_ptr, &dmx->error);

	    dm0s_mrelease(&xcb->xcb_cq_mutex);
	    dm0m_deallocate((DM_OBJECT **)&xcb);

	    /* Destroy caller's transaction context */
	    dmx->dmx_tran_id = NULL;
	}
	else
	{
	    /*  Turn off abort and interrupt flags in XCB. */

	    xcb->xcb_state &= ~(XCB_STMTABORT | XCB_USER_INTR);
	}

	/*
	** Return with error code if have aborted NOLOGGING transaction.
	** Error code depends on abort request:
	**	Savepoint Abort
	**	Transaction Abort
	*/
	if (nologging_abort_error)
	{
	    if (savepoint_id)
		SETDBERR(&dmx->error, 0, E_DM0149_SETLG_SVPT_ABORT);
	    else
		SETDBERR(&dmx->error, 0, E_DM0146_SETLG_XCT_ABORT);
	    break;
	}

	/*
	** Return with database inconsistent error if did not do the
	** abort because the database was inconsistent.
	*/
	if (db_inconsist_error)
	{
	    SETDBERR(&dmx->error, 0, E_DM0100_DB_INCONSISTENT);
	    break;
	}

	return (return_status);
    }

    /* Break to here on any error */
    return (E_DB_ERROR);
}
/*{
** Name: dmxCheckForInterrupt - Checks for external interrupts
**
** Description:
**	Checks if an external interrupt has been received or
**	tranasaction has been selected for FORCE_ABORT
**
** Inputs:
**	xcb				Transaction's XCB
**		xcb_scb_ptr		scb_ui_state indicates
**					type of interrupt 
**					received, if any.
**
** Outputs:
**      error				Reason for error return status.
**					E_DM0065_USER_INTR if
**						external interrupt.
**					E_DM010C_TRAN_ABORTED if
**						FORCE_ABORTED.
**	xcb_state			XCB_FORCE_ABORT if
**					verified by logging.
**					XCB_USER_INTR if some other
**					external interrupt.
**	scb_ui_state			interrupt bits are reset.
**
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
**	11-Nov-2005 (jenjo02)
**	    Created to clean up interrupt handling hodgepodge.
**	    Incorporates and replaces "dmx_show()" functionality.
**	    Also handle other types of interrupts.
**	11-Sep-2006 (jonj)
**	    Reset scb_ui_state right away. If interrupt already
**	    caught or txn is aborting, ignore the interrupt.
**	    Check explicitly for force-abort, don't assume.
*/
DB_STATUS
dmxCheckForInterrupt(
DML_XCB		*xcb,
i4		*error)
{
    LG_TRANSACTION	tr;
    i4			length;
    CL_ERR_DESC		sys_err;	    
    STATUS		s;
    DB_STATUS		status;
    DML_SCB		*scb;
    i4			ui_state;

    *error = 0;

    /* Sanely check the transaction's SCB for interrupt */
    if ( xcb && (scb = xcb->xcb_scb_ptr) && 
		(ui_state = scb->scb_ui_state) )
    {
	/* Reset scb_ui_state right away */
	scb->scb_ui_state &= 
		~(SCB_USER_INTR | SCB_CTHREAD_INTR | 
		  SCB_FORCE_ABORT | SCB_RCB_FABORT);

	/* If already caught or aborting, ignore */
	if ( !(xcb->xcb_state & (XCB_USER_INTR | XCB_ABORT)) )
	{
	    /* Check for external (non-FORCE_ABORT) first */
	    if ( ui_state & (SCB_USER_INTR | SCB_CTHREAD_INTR) )
	    {
		xcb->xcb_state |= XCB_USER_INTR;
		*error = E_DM0065_USER_INTR;
	    }
	    else if ( ui_state & SCB_FORCE_ABORT )
	    {
		/* This is the tranid we're interested in */
		tr.tr_eid = xcb->xcb_tran_id;
		tr.tr_id = xcb->xcb_log_id;

		/* Verify with LG that we're being force-aborted */
		s = LGshow(LG_S_FORCE_ABORT, (PTR)&tr, sizeof(tr), 
				    &length, &sys_err);
		if ( s )
		{
		    uleFormat( NULL, s, &sys_err, ULE_LOG , NULL, 
				(char * )NULL, 0L, (i4 *)NULL, error, 0);
		    uleFormat(NULL, E_DM9017_BAD_LOG_SHOW, &sys_err, ULE_LOG, NULL, 
				(char *)NULL, (i4)0, (i4 *)NULL, error, 1, 
				0, LG_S_FORCE_ABORT);
		    *error = E_DM004A_INTERNAL_ERROR;
		}
		/* if length returned, txn is in abort state, tr_status has why */
		else if ( length )
		{
		    /* Force-abort (logfull) ? */
		    if ( tr.tr_status & TR_FORCE_ABORT )
		    {
			DB_ERROR	dberror;
			/* Commit on logfull? */
			if ( !(scb->scb_sess_mask & SCB_LOGFULL_COMMIT) ||
			    (status = dmx_logfull_commit(xcb, &dberror)) )
			{
			    /* Mark the transaction with the force abort status. */
			    *error = E_DM010C_TRAN_ABORTED;
			    xcb->xcb_state |= XCB_FORCE_ABORT;
			}
		    }
		    else if ( tr.tr_status & TR_ABORT )
		    {
			/* Not force-abort, some other generic kind */
			*error = E_DM0064_USER_ABORT;
			xcb->xcb_state |= XCB_TRANABORT;
		    }
		}
		else
		{
		    /* 
		    ** This txn is not marked for abort.
		    */
		}
	    }
	}
    }
    return( (*error) ? E_DB_ERROR : E_DB_OK );
}
