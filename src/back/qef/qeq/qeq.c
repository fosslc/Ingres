/* Copyright (c) 1986, 2010 Ingres Corporation
**
**
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <me.h>
#include    <st.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <ulf.h>
#include    <ulm.h>
#include    <adf.h>
#include    <ade.h>
#include    <dmf.h>
#include    <dmtcb.h>
#include    <dmrcb.h>
#include    <dmscb.h>
#include    <qefmain.h>
#include    <qsf.h>
#include    <qefrcb.h>
#include    <qefcb.h>
#include    <qefnode.h>
#include    <psfparse.h>
#include    <qefact.h>
#include    <qefqp.h>
#include    <qefdsh.h>
#include    <qefqeu.h>
#include    <scf.h>

#include    <dmmcb.h>
#include    <dmucb.h>
#include    <ex.h>
#include    <tm.h>

#include    <dudbms.h>
#include    <qeuqcb.h>
#include    <qefscb.h>
#include    <rqf.h>
#include    <tpf.h>
#include    <qefkon.h>
#include    <qefqry.h>
#include    <qefcat.h>
#include    <sxf.h>
#include    <qefprotos.h>

#include    <ercl.h>

/**
**
**  Name: QEQ.C - query processing routines
**
**  Description:
**      The routines in this module provide the entry points into
**  query processing in QEF. 
**
**          qeq_open	- open a cursor
**          qeq_close	- close a cursor
**          qeq_fetch	- get the next row from the cursor
**          qeq_query	- a set query is executed
**	    qeq_dsh	- find the current dsh struct
**	    qeq_cleanup	- clean up after a query execution
**	    qeq_restore - restore the context on return from SCF after parsing
**			  the E.I. text while processing a QEA_EXEC_IMM type of 
**			  action.
**
**
**  History: 
**      11-jun-86 (daved)    
**          written
**	22-dec-86 (daved)
**          don't return NO_MORE_ROWS on validation unless action is 
**	    fetch.
**	17-aug-87 (puree)
**	    modified qeq_error not to call dmt_close when aborting.
**	14-oct-87 (puree)
**	    changed reference to E_DM0055 to E_QE0015 in qeq_delete.
**	14-oct-87 (puree)
**	    modified error handling in qeq_open, qeq_fetch.
**	16-oct-87 (puree)
**	    modified qeq_close and qeq_query to destroy temporary tables
**	    at the end of queries and closing of cursors.
**	22-oct-87 (puree)
**	    modified qeq_dsh for query constant support.
**	27-oct-87 (puree)
**	    use qef_cb->qef_adf_cb->adf_constants in qeq_dhs.
**	09-dec-87 (puree)
**	    modified qeq_fetch to check for the existence of dsh_act_ptr.
**	    This is done to fix bug 1460.
**	06-jan-88 (puree)
**	    modified qeq_error to set qp_status flag of the erroneous DSH
**	    to QEQP_OBSOLETE instead of turning off the QEQP_RPT flag.
**	01-feb-88 (puree)
**	    modified qeq_error to always call qee_cleanup.
**	02-feb-88 (puree)
**	    added reset flag to qeq_validate and all qea_xxxx call sequence.
**	11-mar-88 (puree)
**	    remove DSH_QUEUE structure. DMT_CB's can be linked together
**	    using the standard jupiter control block header.
**	10-may-88 (puree)
**	    Set obsolescence flag in DSH instead of QP.  QP is read-only.
**	19-may-88 (puree)
**	    modified qeq_close, qeq_query, qeq_error, qeq_endret  to clear
**	    dmt_record_access_id when a table is closed.
**	02-aug-88 (puree)
**	    general cleanup by implementing qeq_cleanup to replace qeq_error.
**	    Error handling with regard to transaction and cursor states is
**	    done in qeq_cleanup.  Also modify qeq_query for error handling
**	    semantics in DB procedures.
**	11-aug-88 (puree)
**	    Clean up lint errors.
**	22-aug-88 (puree)
**	    Modify QEQ_QUERY to call qef_error if qeq_validate fails.
**	25-aug-88 (puree)
**	    Modify qeq_query for bug concerning the count of open query
**	    during commit and rollback actions (for DB procedure).
**	29-aug-88 (puree)
**	    Modify qeq_query to implement built-in variables for DB
**	    procedures.
**	    Implement delete cursor and update cursor by tid if the cursor
**	    has been positioned on a secondary index.
**	07-sep-88 (carl)
**	    Modified qeq_cleanup for Titan
**	12-sep-88 (puree)
**	    Modify qeq_fetch to reset the singleton bit in the query plan 
**	    for cursor fetch from constant function to work right.  This 
**	    is a temporary fix since the QP should be a read only object.
**	05-oct-88 (puree)
**	    Modify transaction handling for invalid query.  If a table in
**	    the topmost action is invalid, we will just terminate the 
**	    current query without executing it.  If a table in other actions
**	    is invalid, the current query will aborted.  Terminating and
**	    aborting a query take their usual semantics.
**  
**	    In this module, the qeq_cleanup is modified to set status code
**	    to E_DB_ERROR if the caller status is E_DB_WARN and the error
**	    code is E_QE0023_INVALID_QUERY so that SCF will properly retry
**	    the query.
**	10-oct-88 (carl)
**	    Modified qeq_query and qeq_endret for Titan
**	12-oct-88 (puree)
**	    If a table validation fails in any of cursor operations (except
**	    the open cursor), abort the cursor.
**	24-oct-88 (puree)
**	    Avoid AV in qeq_cleanup when DSH is not present.
**	10-nov-88 (puree)
**	    Modify qeq_query not to change rowcount for if and message
**	    statements.
**	23-nov-88 (puree)
**	    Modify qeq_query not to continue executing DB procedure if the
**	    validation fails in commit and rollback actions (BUG 3917)
**	15-jan-89 (paul)
**	    Modify qeq_query() to support rule action lists and the new
**	    CALLPROC and EMESSAGE actions for rules support.
**	05-feb-89 (carl)
**	    Modified cursor routines for Titan
**	23-feb-89 (paul)
**	    Added support to cursor routines to properly manage qef_cb->qef_dsh
**	    as required for rules processing. The cursor routines must set
**	    this field to NULL upon completing their operations.
**	23-mar-89 (paul)
**	    Corrected problem with invoking rules on a set update involving
**	    multiple rows. Also added support for an error callback if
**	    procedure creation fails for a nested procedure call.
**	    Corrected problem allocating multiple DSH structures.
**	10-apr-89 (paul)
**	    Corrected problem with marking DSH's in use during cursor
**	    handling.
**	17-apr-89 (paul)
**	    Corrected unix compiler warnings.
**	17-apr-89 (paul)
**	    Converted code to use qef_rcb->qef_rule_depth to track
**	    current rule nesting level replacing incorrect usage
**	    of qef_stat for this function in prior versions.
**	23-may-89 (carl)
**	    Fixed qeq_query to initialize QES_03CTL_READING_TUPLES for SELECT 
**	    queries to prevent premature DDL_CONCURRENCY commit
**	13-jun-89 (paul)
**	    Correct bug 6346. Do not reset local variables in a DB
**	    procedure after a COMMIT or ROLLBACK action.
**	03-aug-89 (nancy) -- fix bug #7339 in qeq_query where execution of
**	    a dbproc will end prematurely if a select evaluates to false.
**	    (as in select...where 1=0 followed by more stmts.)
**	10-aug-89 (neil)
**	    Update bug fix for 7339.  One remaining case left described later.
**	15-aug-89 (neil)
**	    Term 2: Fix rowcounts on return from nested procedures.
**	07-sep-89 (neil)
**	    Alerters: Added recognition of EVENT actions.
**	26-sep-89 (neil)
**	    Support for SET NORULES.
**	07-oct-89 (carl)
**	    Modified qeq_query to call qel_u3_lock_objbase to avoid STAR 
**	    deadlocks
**	14-nov-89 (carl)
**	    Added qet_t9_ok_w_ldbs call to qeq_query for 2PC
**	29-nov-89 (fred)
**	    Added support for peripheral objects.  This amounts to noticing the
**	    INCOMPLETE/INFO status returned by qea_fetch() and passing it along.
**	21-dec-89 (neil)
**	    Check for NULL dsh in qeq_cleanup.
**	10-jan-90 (neil)
**	    Cleaned up some DSH handling for rules.
**	1-feb-90 (nancy)
**	    Fixed bug 7514 in qeq_query.  See comments below.
**	22-mar-90 (nancy)
**	    Fixed bug 20755.  If qeq_validate returns NO_MORE_ROWS, set
**	    iirowcount accordingly.
**	16-apr-90 (neil)
**	    Fixed rules/iirowcount problem in qeq_query (B21195).
**	    Fixed OPEN CURSOR problem in qeq_open (B21257).
**	20-apr-90 (davebf)
**	    Fixed bug 8875 enabling iterated update of given row inside
**	    a dbp.  See comments in qeq_query.
**	09-may-90 (neil)
**	    Cursor performance I: Allow the pre-fetch of multiple rows.
**	05-jun-90 (nancy)
**	    Bug 21652- qeq_fetch(), make call to qef_error() when cursor
**	    fetch fails. (lock-timeout did not get mapped to user error)
**	21-jul-90 (carl)
**	    Changed qeq_query() to process trace points
**	06-aug-90 (mikem)
**	    Bug #32320 - failure of qeq_validate from qeq_query would not
**	    back out transaction.  The fix is to make sure normal rule
**	    error processing happens after a deadlock encountered in the
**	    qeq_validate.
**	14-aug-90 (carl)
**	    Moved DDB logic from qeq_open to qeq_c4_open
**	16-sep-90 (carl)
**	    changed qeq_cleanup to abort for STAR if error
**	13-oct-90 (carl)
**	    modified qeq_query to:
**	    1) hide E_DB_WARN (which is NOT an error condition) from 
**	       qeq_cleanup to prevent aborting a good retrieval
**	    2) test for trace point 51
**	20-nov-90 (davebf)
**	    Indicate in dsh when this is a cursor query  (cursor opened).
**	10-dec-90 (neil)
**	    TM EXECUTE DBP: Fix rowcounts on return from dbps + trace point.
**	    Alerters: Added recognition of EVENT actions.
**	04-feb-91 (neil)
**	    qeq_query: Trace DBP return
**	10-jun-91 (fpang)
**	    If aborting in qeq_cleanup(), drop temp tables first.
**	    Fixes B37196 and B36824.
**	18-jun-91 (davebf)
**	    Avoid closing all cursors when dbp not found in cache.
**	02-jul-91 (nancy)
**	    bug 36834, modified qeq_chk_resource to add QEA_LOAD action so
**	    that resource control can be applied to create table as select.
**	15-jul-91 (stevet)
**	    Bug #36065.  Add error E_QE0238_NO_MSTRAN to QEQ_DSH if there
**	    is no MST when trying to open new query and another query is 
**          already active.
**      15-jul-91 (stevet)
**          Bug #37995.  Added act_reset flag to qeq_query to determine if 
**          actions should be executed with reset.  Without resetting, DBP 
**          with WHILE LOOP can fail because not all nodes within a query plan
**          were executed on second and subsequent loops.
**	07-aug-91 (mikem)
**	    bug #32999
**	    Changed qeq_query to include logical key return values as part 
**	    saved and restored context manipulated as part of a procedure
**	    call, so that logical keys allocated as part of rule firing will
**	    not be returned to the user. 
**	12-feb-92 (nancy)
**	    bug 38565.  Return E_QE0301_TBL_SIZE_CHANGE from qeq_validate()
**	    indicating that qep needs recompilation.  Since this is not an
**	    error, include this status in all routines below that do special
**	    handling for E_QE0023_QEP_INVALID.  The additional check in 
**	    qeq_query() is the actual bug fix, modifications to other routines
**	    are for completeness.
**	11-mar-92 (jhahn)
**	    Added support for QEF interrupts in dbprocs.
**	15-jun-92 (davebf)
**	    Sybil Merge
**	04-sep-92 (fpang)
**	    In qeq_query(), properly set DSH_IN_USE for distributed cases.
**	01-oct-1992 (fpang)
**	    In qeq_endret() and qeq_cleanup(), save qef_count around 
**	    qeq_d6_tmp_act() call.
**	23-oct-92 (fpang)
**	    In qeq_fetch(), unset DSH_IN_USE bit after distributed fetch.
**	    In qeq_query(), add QEA_D9_UPD to QEA_RUP case.
**	09-nov-92 (jhahn)
**	    Added handling for byrefs.
**	19-nov-92 (rickh)
**	    Add CREATE_TABLE and CREATE_INTEGRITY actions.
**	14-dec-92 (jhahn)
**	    Cleaned up handling of byrefs. Cleaned up nested handling of
**	    return_status.
**      14-dec-92 (anitap)
**          Added #include <psfparse.h> and changed the order of <qsf.h> for
**          CREATE SCHEMA.
**	15-dec-92 (anitap)
**	    Added calls to two new routines qea_cschema() and qea_execimm()
**  	    for CREATE SCHEMA project. Also added support for QEA_EXEC_IMM type
**	    of headers for both CREATE SCHEMA and FIPS constraints project. 
**	20-dec-92 (anitap)
**	    Added enhancements to qeq_query(). Added new routine qeq_restore().
**      07-Jan-1993 (fred)
**          Altered qeq_fetch() to correctly handle cursor fetches of
**          large object columns.
**	08-jan-93 (anitap)
**	    Bug fixes for CREATE SCHEMA in qeq_query().
**	12-feb-93 (jhahn)
**          Added support for statement level rules. (FIPS)
**      3-mar-93 (rickh)
**          Fixes to execute immediate processing for CREATE_INTEGRITY.
**	24-mar-93 (jhahn)
**	    Various fixes for support of statement level rules. (FIPS)
**	31-mar-93 (anitap)
**	    Added fixes for QEA_CREATE_INTEGRITY and QEA_CREATE_VIEW for
**	    "SET AUTOCOMMIT ON". qef_open_count was not being set to 0
**	    because the mode was being lost.
**	02-apr-93 (jhahn)
**	    Made set input procedures called from rules bump qef_rule_depth.
**	01-may-93 (jhahn)
**	    Undid above change. Instead added new action QEA_INVOKE_RULE.
**	06-may-93 (anitap)
**          Added new routine qeq_updrow().
**	9-jun-93 (rickh)
**	    Added 2 new arguments to qeu_dbu call.
**	16-jun-93 (jhahn)
**	    Added 'fix' so that we don't rollback the transaction when
**	    an unpositioned cursor update is attempted.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      07-jul-93 (anitap)
**          Call qeq_restore() to restore context on return from SCF for
**          QEA_CALLPROC also.
**          Enhance comments. Check status for QEA_CREATE_INTEGRITY and
**          QEA_CRETE_VIEW before returning to SCF to parse the E.I. qtext.
**          Check if dsh is null to avoid AV in E.I. code.
**	    Do not restore qef_rowcount (to avoid 0 rows message).
**	26-jul-1993 (jhahn)
**	    Moved coercing of returned byref procedure parameters from the
**	    back end to the front end.
**	29-jul-93 (rickh)
**	    At cleanup time, don't try to release objects anchored in the
**	    DSH unless there is a DSH.
**	12-aug-93 (swm)
**	    Cast first parameter of CSaltr_session() to CS_SID to match
**	    revised CL interface specification. Moved <cs.h> for CS_SID.
**	09-aug-93 (anitap)
**	    Added new routine qeq_setup() so that it can be used for FIPS
**	    constraints project also.
**	    Initialized qef_rowcount to get rid of spurious message "0 rows".
**	26-aug-93 (rickh)
**	    NOT NULL integrites may follow the SELECT action compiled out of
**	    a CREATE TABLE AS SELECT statement.  If this happens, don't clobber
**	    the rowcount meticulously accumulated by the SELECT.
**	    Do not restore qef_rowcount (to avoid 0 rows message).
**	01-sep-93 (jhahn)
**	    Added support for multiple query plans for set input procedures.
**      15-Oct-1993 (fred)
**          Added large object support for STAR.
**	26-oct-93 (rickh)
**	    Changed variable name to reflect its use by both ALTER TABLE
**	    ADD CHECK CONSTRAINT and ALTER TABLE ADD REFERENTIAL CONSTRAINT.
**	    Also added code to NOT restore the qef_output pointer if we're
**	    performing a SELECT ANY to verify an ALTER TABLE ADD CONSTRAINT.
**	08-oct-93 (swm)
**	    Bug #56448
**	    Altered qec_tprintf calls to conform to new portable interface.
**	    Each call is replaced with a call to STprintf to deal with the
**	    variable number and size of arguments issue, then the resulting
**	    formatted buffer is passed to qec_tprintf. This works because
**	    STprintf is a varargs function in the CL. Unfortunately,
**	    qec_tprintf cannot become a varargs function as this practice is
**	    outlawed in main line code.
**	13-oct-93 (swm)
**	    Bug #56448
**	    Altered uld_prnode calls to conform to new portable interface.
**	    It is no longer a "pseudo-varargs" function. It cannot become a
**	    varargs function as this practice is outlawed in main line code.
**	    Instead it takes a formatted buffer as its only parameter.
**	    Each call which previously had additional arguments has an
**	    STprintf embedded to format the buffer to pass to uld_prnode.
**	    This works because STprintf is a varargs function in the CL.
**	31-jan-94 (jhahn)
**	    Fixed support for resource lists.
**	03-feb-94 (rickh)
**	    Re-cycle temporary tables at cleanup time.
**	15-feb-94 (swm)
**	    Bug #59611
**	    Replace STprintf calls with calls to TRformat which checks
**	    for buffer overrun. Added qeq_tr_format to deal with TRformat
**	    semantics: null-terminate formatted strings and retain trainling
**	    newline characters.
**      15-feb-1994 (stevet)
**          Removed call to adu_free_objects() since BLOB stored in
**          temp tables may still needed.  Moved adu_free_objects() to
**          qeq_cleanup.  (B59267)
**	17-mar-1994 (anitap)
**	    Fix for bug 59929. Error during SELECT of ALTER TABLE resulted
**	    in the whole transaction to be rolled back. This was because the
**	    number of open cursors (qef_cb->qef_open_count) was not being
**	    decremented. Look at qeq_query() for more details. 
**	21-mar-94 (rickh)
**	    Look up DSH's by QSF object handle, not query plan name. This
**	    fixes bug 60540.
**	18-mar-94 (swm)
**	    Bug #59883
**	    Remove format buffers declared as automatic data on the stack with
**	    references to per session buffer allocated from ulm memory at
**	    QEF session startup time.
**      15-Apr-1994 (fred)
**          Altered call to adu_free_objects() in qeq_cleanup().
**	08-apr-1994 (pearl)
**	    Bug 56748.  Save the mode in QEF_RCB.qef_mode before 
**	    returning from database procedures.
**	19-apr-94 (davebf)
**	    Reset first_act when unwinding with no error from rule fired
**	    proc (b42514, b41108)
**	19-apr-94 (davebf)
**	    Reset first_act when unwinding with no error from rule fired
**	    proc (b42514, b41108)
**	18-may-94 (anitap)
**	    Do not rollback the whole transaction when an error occurs in a
**	    nested dbproc (b63465). Changes in qeq_query().
**      23-Jan-95 (liibi01)
**          Bug #66436, get rid of the bonus message, also clean up the
**          comments.
**	19-jan-96 (pchang)
**	    Reset qef_remnull flag in qeq_open().  This is necessary because
**	    following changes made to fix bug 73095, the setting of this flag 
**	    now triggers a correct warning SS01003_NULL_ELIM_IN_SETFUNC in the
**	    SQLSTATE variable when a cursor is OPEN.  Prior to this it was 
**	    referenced only when a cursor is FETCHed. 
**	30-sep-96 (nick)
**	    Don't silently rollback transactions just because we can't find a
**	    QP. #78523
**	17-oct-96 (nick)
**	    Tidy stuff found whilst debugging here.
**	28-oct-96 (somsa01)
**	    In qeq_cleanup(), moved adu_free_objects() from where it was to
**	    after qeq_closeTempTables(). This is because, after
**	    qeq_release_dsh_resources() was run, there were situations where
**	    adu_free_objects() would be using the dsh which pointed to an
**	    invalid location of memory (or perhaps pointing to an area of
**	    memory which was being used).
**	30-oct-96 (nick)
**	    Nested procedure invocation initiated via execute immediate would
**	    silently rollback the transaction as a side effect of decrementing
**	    qef_open_count too many times ; this happened if a commit was 
**	    issued somewhere within a nested procedure. #79139
**	26-nov-96 (somsa01)
**	    Backed out change made in qeq_cleanup() from change #427841; it
**	    is not needed by BLOBs, and it also breaks the creation of star
**	    databases.
**	27-Feb-1997 (jenjo02)
**	    Extended <set transaction statement> to support <transaction access
**          mode> (READ ONLY | READ WRITE) per SQL-92.
**	    When starting a single statement txn with autocommit on, use query
**	    access mode passed by caller instead of forcing to QEF_UPDATE.
**	20-march-1997(angusm)
**	    Change to qeq_query when resuming after error in dbproc.(b80759)
**      13-jun-97 (dilma04)
**          Added support for a bit flag in QEF_RCB indicating that QEF is 
**          processing an action associated with an integrity constraint.
**	21-july-1997(angusm)
**	    Change to qeq_open() for bug 82733: cursor containing aggregate
**	    with empty sub-select returns NOTFOUND instead of zero count.
**	07-Aug-1997 (jenjo02)
**	    Changed *dsh to **dsh in qee_cleanup(), qee_destroy() prototypes.
**	    Pointer will be set to null if DSH is deallocated.
**	22-Aug-1997 (wonst02)
**	    Added parentheses-- '==' has higher precedence than bitwise '&':
**	    ( (integrityDetails->qci_flags & QCI_SAVE_ROWCOUNT) == 0 ), not:
**	    ( integrityDetails->qci_flags & QCI_SAVE_ROWCOUNT == 0 )
**	19-Nov-1997 (jenjo02)
**	    Added qeq_unwind_dsh_resources() function to release resources for
**	    all DSH's in the stack.
**	26-nov-1997 (somsa01)
**	    In qeq_query(), in the case of multiple user rules defined to an
**	    SQL operation, if the first rule fired fails, then the subsequent
**	    rules are not fired (since the action list containing these rules
**	    is "invalidated"). However, if one of these multiple rules is an
**	    internal rule (generated, for example, by an ALTER TABLE ADD
**	    REFERENTIAL CONSTRAINT), these rules are part of another action
**	    list. Therefore, if iierrorno has been set, "invalidate" this
**	    rules list as well. (Bug #86734)
**	10-dec-1997 (somsa01)
**	    Ammended above change for bug #86734. "iierrorno" can be null.
**	1-apr-1998 (hanch04)
**	    Calling SCF every time has horendous performance implications,
**	    re-wrote routine to call CS directly with qef_session.
**	28-may-1998 (shust01)
**	    Initialized act to NULL in qeq_query().  Problem only when NULL
**	    dsh pointer. act was being checked, but was never initialized.
**	    Caused qeq_validate() to be called (when it shouldn't have been)
**	    and quickly SEGV when act had garbage in it. bug 91068.
**      26-Jun-1998 (hanal04)
**          X-integ fix for bug 89898. When the first statement in a
**          dbproc is to execute another DBproc load the default values
**          saved during compilation into the DSH if values are not already
**          present.
**	11-nov-1998 (somsa01)
**	    In qeq_endret() and qeq_close(), we need to make sure that the
**	    continuation flag is reset.  (Bug #94114)
**	22-May-2000 (dacri01)
**	    qeq_nccalls() : Check if the action header for the qep is null
**	    before calling qeq_nccalls(). (B93384)
**      22-nov-2000 (stial01)
**          qeq_query Check for table open error from qeq_validate (B103150)
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	10-Jan-2001 (jenjo02)
**	    Supply session's SID to QSF in qsf_sid.
**	    Replaced qef_session() function with GET_QEF_CB macro.
**	18-Jan-2001 (jenjo02)
**	    Subvert calls to qeu_secaudit(), qeu_secaudit_detail()
**	    if not C2SECURE.
**      22-Mar-2001 (stial01)
**          qeq_cleanup: call adu_free_objects with sequence number 0 (b104317)
**	30-jul-01 (devjo01)
**	    Make sure qp was set before testing qp_status for QEQP_ROWPROC.
**	    (b105373/INGSRV1508).
**      11-nov-2002 (stial01)
**          Do not start a transaction for select without from list (b109101)
**      26-nov-2002 (stial01)
**          Remove TRdisplay added on 11-nov-2002 (b109101)
**	21-Jun-2004 (inifa01) bug 110957 INGSRV 2529    
**	    Query with nested procedure calls fails with E_DM0055_NONEXT,
**	    E_DM9B02_PUT_DMPE_ERROR, E_DM904D_ERROR_PUTTING_RECORD and 
**	    E_DM008B_ERROR_PUTTING_RECORD.
**	    FIX: Don't call dmpe_free_temps() to clear all BLOB temps
**	    if called on nested procedure call. 
**	30-dec-03 (inkdo01)
**	    Add "dsh" and replace "reset" with "function" in many calls to 
**	    qea_ action processors and qen_ node processors.
**      06-jan-2004 (stial01)
**          Dont free blob temps if error is E_QE0023_INVALID_QUERY (B111552)
**      30-mar-2004 (stial01 for inkdo01)
**	    Fixed error handling for create table.
**	20-Jul-2004 (gorvi01)
**	    Fixed bug#112688-wrong error message when trying to update a read
**	    only database. Update operation of create table in a read only
**	    database gives wrong error message of E_QE0018. Fix given in
**	    QEA_CREATE_TABLE to break if correct error message is recognised of
**	    transaction access conflict.
**	    Updating operation now gives correct error message of E_US138F.
**      14-dec-2004 (stial01)
**          Dont free blob temps if error is E_QE0301_TBL_SIZE_CHANGE (B113623)
**      10-Mar-2005 (zhahu02) INGSRV3012/b113294
**          Updated qeq_query for error rasing with rule.
**       5-Jul-2005 (hanal04) Bug 114790 INGSRV3345
**          Removed above change for bug 113294. It caused a regression
**          in RUN_ALL RULES tests and was not required in the first place.
**          RAISE ERROR should be used to ensure errors in DBPs fired by
**          rules hit the errlog.log
**	13-Dec-2005 (kschendel)
**	    Remaining inlined QEN_ADF structs changed to pointers, fix here.
**	28-Dec-2005 (kschendel)
**	    Use DSH references whenever possible instead of QEF RCB
**	    references.  (This simplifies the code, eliminating many
**	    RCB references, and reflects the new primacy of the DSH
**	    thanks to the parallel query changes.)
**	16-Jan-2006 (kschendel)
**	    Access qen-status thru xaddrs.
**      27-May-2008 (gefei01)
**          qeq_dsh(): Allocate dsh for TPROC.
**      08-Sep-2008 (gefei01)
**          Support table procedure.
**      24-Feb-2009 (gefei01)
**          Fix bug 121711:: Make table procedure dsh allocation an exception
**          for multiple statement transaction.
**      27-Feb-2009 (gefei01)
**          Support cursor select for table procedure.
**      01-Apr-2009 (gefei01)
**          Bug 121870: qeq_cleanup does qet_commit to abort to the last
**          internal savepoint even when open cursor count is non zero.
**	18-Jun-2010 (kschendel) b123775
**	    Various changes in resource validation data structures, mostly
**	    so that table procs get validated the right way.
*/


/*	static	functions	*/

static PTR
qeq_lqen(
PTR	p );

static PTR
qeq_rqen(
PTR	p );

static i4
qeq_tr_callback(
	char		*nl,
	i4		length,
	char		*buffer
);

static VOID
qeq_prqen(
PTR	p,
PTR	control );

static void
qeq_restore(
        QEF_RCB         *qeq_rcb,
        QEF_RCB         *qef_rcb,
        QEE_DSH         *dsh,
        i4              *mode,
        QEF_AHD         **act,
        i4         *rowcount,
	QEF_QP_CB	**qeq_qp,
        PTR             **cbs,
        i4              **iirowcount);

static void
qeq_setup(
QEF_CB		*qef_cb,
QEE_DSH		**dsh,
QEF_QP_CB	**qp,
QEF_AHD		**first_act,
QEF_AHD		**act,
PTR		**cbs,
i4		**iirowcount,
i4		**iierrorno);

/*{
** Name: QEQ_OPEN	- open a cursor
**
**  External QEF call:	    status = qef_call(QEQ_OPEN, &qef_rcb);
**
** Description:
**      The named cursor is opened. This entails finding the correct
**	QP and initializing it for execution. If no transaction exists when
**	this routine is called, a single statement transaction is created
**	on behalf of the user. No other cursors can be opened (or set
**	queries executed) during a single statement transaction. 
**
** Inputs:
**      qef_rcb
**	    .qef_eflag		designate error handling semantics
**				for user errors.
**		QEF_INTERNAL	return error code.
**		QEF_EXTERNAL	send message to user.
**	    .qef_cb		session control block
**	    .qef_d_id		dmf session id
**	    .qef_db_id		database id
**	    .qef_qp		cursor id (DB_CURSOR_ID)
**	    .qef_qso_handle	internal QP id. Used instead of qef_qp if not
**				zero.
**	    .qef_param		input parameters
**	    .qef_qacc		open command modifier
**		    QEF_READONLY
**		    QEF_UPDATE
**
** Outputs:
**      qef_rcb
**	    .error.err_code	one of the following
**				E_QE0000_OK
**				E_QE0017_BAD_CB
**				E_QE0018_BAD_PARAM_IN_CB
**				E_QE0019_NON_INTERNAL_FAIL
**				E_QE0002_INTERNAL_ERROR
**				E_QE0004_NO_TRANSACTION
**				E_QE000D_NO_MEMORY_LEFT
**				E_QE000E_ACTIVE_COUNT_EXCEEDED
**				E_QE000F_OUT_OF_OTHER_RESOURCES
**				E_QE000C_CURSOR_ALREADY_OPENED
**				E_QE0024_TRANSACTION_ABORTED
**				E_QE0034_LOCK_QUOTA_EXCEEDED
**				E_QE0035_LOCK_RESOURCE_BUSY
**				E_QE0036_LOCK_TIMER_EXPIRED
**				E_QE002A_DEADLOCK
**	Returns:
**	    E_DB_{OK,WARN,ERROR,FATAL}
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	11-JUN-86 (daved)
**          written
**	22-may-87 (daved)
**	    mark open cursor as not positioned.
**	14-oct-87 (puree)
**	    make sure that qef_error is called in case of error from
**	    qeq_validate.
**	02-feb-88 (puree)
**	    added reset flag to qeq_validate and all qea_xxxx call sequence.
**	07-jun-88 (puree)
**	    move deferred update cursor flag from QP into action header.
**	12-oct-88 (puree)
**	    If a table validation fails during cursor open, abort the open
**	    cursor statement but not the transaction.
**	23-feb-89 (paul)
**	    Clean up qef_cb state before returning.
**	10-apr-89 (paul)
**	    DSH must be marked as not in use when returning from a cursor
**	    operation.
**	26-may-89 (paul)
**	    Deleted unused actions out of routines.
**	16-apr-90 (neil)
**	    Fixed OPEN CURSOR problem where 2 cursor for DEFERRED UPDATE gave
**	    the wrong error because they did not set the err_code. (B21257)
**	05-feb-89 (carl)
**	    Modified for Titan
**	14-aug-90 (carl)
**	    Moved DDB logic to qeq_c4_open
**	20-nov-90 (davebf)
**	    Indicate in dsh when this is a cursor query  (cursor opened).
**	12-feb-92 (nancy)
**	    Add new return from qeq_validate(), E_QE0301_TBL_SIZE_CHANGE to
**	    same handling as E_QE0023. (bug 38565)
**	01-oct-1992 (fpang)
**	    Save qef_count around qeq_d6_tmp_act() call.
**      12-feb-93 (jhahn)
**          Added support for statement level rules. (FIPS)
**	19-jan-96 (pchang)
**	    Reset qef_remnull flag.  This is necessary because following 
**	    changes made to fix bug 73095, the setting of this flag now 
**	    triggers a correct warning SS01003_NULL_ELIM_IN_SETFUNC in the
**	    SQLSTATE variable when a cursor is OPEN.  Prior to this it was 
**	    referenced only when a cursor is FETCHed. 
**	27-Feb-1997 (jenjo02)
**	    Extended <set transaction statement> to support 
**	    <transaction access mode> (READ ONLY | READ WRITE) per SQL-92.
**	    When starting a single statement txn with autocommit on, use query
**	    access mode passed by caller instead of forcing to QEF_UPDATE.
**	21-july-1997(angusm)
**	    Change to qeq_open() for bug 82733: cursor containing aggregate
**	    with empty sub-select returns NOTFOUND instead of zero count.
**	21-Jun-2004 (inifa01) bug 110957 INGSRV 2529
**	    Query with nested procedure calls fails with E_DM0055_NONEXT,
**	    E_DM9B02_PUT_DMPE_ERROR, E_DM904D_ERROR_PUTTING_RECORD and
**	    E_DM008B_ERROR_PUTTING_RECORD.
**	    FIX: Due to c449556 adu_free_objects()->dmpe_free_temps() was 
**	    always called with pop_segno1 == 0, which means all temp etabs on 
**	    scb_lo_next queue will be freed.  This fix modifies qeq_cleanup() 
**	    so that dmpe_free_temps() is not called when cleaning up from a 
**	    nested procedure query.  The BLOB temps will be freed when the
**	    outermost procedure call completes.
**	30-dec-03 (inkdo01)
**	    Add "dsh" and replace "reset" with "function" in many calls to 
**	    qea_ action processors and qen_ node processors.
**	23-feb-04 (inkdo01)
**	    Add calls to copy from DSH to RCB for || query thread safety.
**      27-Feb-2009 (gefei01)
**          Support cursor select for table procedure.
**	14-May-2010 (kschendel) b123565
**	    Split validation into two parts, fix here.
**	21-Jun-2010 (kschendel) b123775
**	    Slight changes to table proc re-entry.
*/
DB_STATUS
qeq_open(
QEF_RCB		    *qef_rcb )
{
    i4		err;
    DB_STATUS		status	= E_DB_OK;
    QEF_CB		*qef_cb = qef_rcb->qef_cb;
    QEE_DSH		*dsh;
    QEF_AHD		*act;

    qef_cb->qef_rcb = qef_rcb;
    qef_rcb->qef_remnull = 0;

    /* Search for the DSH of the specified QP.  If this is a reentry
    ** after table procedure QP recreation, the QEF RCB has the tproc
    ** info in it instead of the cursor-open info, needs a tiny bit of
    ** special handling.
    */
    if (qef_rcb->qef_intstate & QEF_DBPROC_QP)
    {
	dsh = (QEE_DSH *) qef_cb->qef_dsh;
	if (dsh == NULL)
	{
	    i4 local_err;
	    qef_error(E_QE0002_INTERNAL_ERROR, 0, status, &err,
		&qef_rcb->error, 0);
	    return (E_DB_ERROR);
	}
	*qef_rcb = *(dsh->dsh_saved_rcb);
	qef_rcb->qef_intstate &= ~QEF_DBPROC_QP;  /* Make sure */
    }
    else
    {
	status = qeq_dsh(qef_rcb, -1, &dsh, QEQDSH_NORMAL, -1);
	if (status != E_DB_OK)
	    return (status);
    }

    /* Start a single statement transaction if necessary */

    if (qef_cb->qef_stat == QEF_NOTRAN)
    {
	qef_rcb->qef_modifier = QEF_SSTRAN;
	if (qef_cb->qef_auto == QEF_ON)
	    qef_rcb->qef_flag = qef_rcb->qef_qacc;
	else
	    qef_rcb->qef_flag = QEF_UPDATE;
	/* qet_begin will log an error message */
	if (status = qet_begin(qef_cb))
	    return (qeq_cleanup(qef_rcb, status, TRUE));
    }

    dsh->dsh_stmt_no = qef_cb->qef_stmt++;	/* set statement # in DSH */
    dsh->dsh_qp_status |= DSH_CURSOR_OPEN;  /* indicate this is cursor query */
    dsh->dsh_positioned = FALSE;
    qeq_rcbtodsh(qef_rcb, dsh);

    if (qef_cb->qef_c1_distrib & DB_3_DDB_SESS)
    {
	/* Process DDB action list */
	status = qeq_c4_open(qef_rcb, dsh);
    }
    else
    {
	/* Process LDB action list */
	for (act = dsh->dsh_qp_ptr->qp_ahd; 
	     act != (QEF_AHD *)NULL; act = act->ahd_next)
	{
	    /* check for deferred cursor, only one at a time is allowed */
	    if (act->ahd_flags & QEA_DEF)
	    {
		dsh->dsh_qp_status |= DSH_DEFERRED_CURSOR;
		if (qef_cb->qef_defr_curs)
		{
		    /* B21257 - set error code to get to user */
		    qef_rcb->error.err_code = E_QE0026_DEFERRED_EXISTS;
		    status = E_DB_ERROR;
		    break;
		}
	    }

	    /* validate the action */
	    status = qeq_topact_validate(qef_rcb, dsh, act, TRUE);
	    if (status == E_DB_OK)
		status = qeq_subplan_init(qef_rcb, dsh, act, NULL, NULL);
	    qeq_dshtorcb(qef_rcb, dsh);
	    if (status != E_DB_OK)
	    {
		if (qef_rcb->error.err_code == E_QE0015_NO_MORE_ROWS &&
		    act->ahd_atype != QEA_GET)
		{
		    qef_rcb->error.err_code = E_QE0000_OK;
		    status = E_DB_OK;
		    if ((dsh->dsh_qp_status & DSH_CURSOR_OPEN) &&
			(act->ahd_next != NULL) &&
			((act->ahd_next)->ahd_atype == QEA_GET))
			continue;
		    act = act->ahd_next;
		    continue;
		}
		break;
	    }

	    switch (act->ahd_atype)
	    {
		case QEA_DMU:
		    status = qea_dmu(act, qef_rcb, dsh);
		    break;
		case QEA_SAGG:
		    status = qea_sagg(act, qef_rcb, dsh, (i4) FUNC_RESET);
		    break;

		case QEA_PROJ:
		    status = qea_proj(act, qef_rcb, dsh, (i4) FUNC_RESET);
		    break;
		case QEA_AGGF:
		    status = qea_aggf(act, qef_rcb, dsh, (i4) FUNC_RESET);
		    break;

		case QEA_GET:
		    /* we are opened when we hit the first get action
		    ** the open command does everything upto retrieving the
		    ** first record */
		    break;
		default:
		    status = E_DB_ERROR;
		    break;
	    }  /*end of switch on act->ahd_atype */

	    /* get the next action, except when an error occurs */
	    qeq_dshtorcb(qef_rcb, dsh);
	    if (status != E_DB_OK)
		break;

	    /* preserve XACT status as of this action end */
	    qef_cb->qef_ostat = qef_cb->qef_stat;  

	}	/* end of action list */
    }	/* end of if DDB */

    /* If tproc QP needs to be recompiled,
     * return to the sequencer to do it.
     */
    if (status != E_DB_OK && qef_rcb->error.err_code == E_QE030F_LOAD_TPROC_QP)
        return status;

    /* cleaning up */

    if (DB_FAILURE_MACRO(status) ||
	(status == E_DB_WARN && 
	 (qef_rcb->error.err_code == E_QE0023_INVALID_QUERY ||
	  qef_rcb->error.err_code == E_QE0301_TBL_SIZE_CHANGE)))
    {
	qef_error(qef_rcb->error.err_code, 0L, status, &err, 
		    &qef_rcb->error, 0);
	status = qeq_cleanup(qef_rcb, status, TRUE);
    }
    else
    {
	qef_cb->qef_open_count++;	    /* number of open query/cursors */
	if (dsh->dsh_qp_status & DSH_DEFERRED_CURSOR)
	    qef_cb->qef_defr_curs = TRUE;   /* mark a deferred cursor opened */
	dsh->dsh_qp_status &= ~DSH_IN_USE;
    }
    /* We follow the convention the qef_cb->qef_dsh == NULL when QEF is	    */
    /* being invoked with a user request. If this field is not NULL, QEF    */
    /* assumes that it is being called back to execute a nested procedure   */
    /* invocation. */
    qef_cb->qef_dsh = NULL;
    return (status);
}


/*{
** Name: QEQ_CLOSE	- close a cursor
**
**  External QEF call:	    status = qef_call(QEQ_CLOSE, &qef_rcb);
**
** Description:
**      A cursor is closed. If this cursor was in a single statement
**	transaction, the transaction is committed.  If it is in a 
**	multi-statement transaction, an internal savepoint is declared.
**
** Inputs:
**      qef_rcb             
**	    .qef_eflag		designate error handling semantics
**				for user errors.
**		QEF_INTERNAL	return error code.
**		QEF_EXTERNAL	send message to user.
**	    .qef_cb		session control block
**	    .qef_qp		name of query plan
**	    .qef_qso_handle	internal QP id. Used instead of qef_qp if not
**				zero.
**
** Outputs:
**      qef_rcb
**	    .error.err_code	one of the following
**				E_QE0000_OK
**				E_QE0017_BAD_CB
**				E_QE0018_BAD_PARAM_IN_CB
**				E_QE0019_NON_INTERNAL_FAIL
**				E_QE0002_INTERNAL_ERROR
**				E_QE0008_CURSOR_NOT_OPENED
**				E_QE002A_DEADLOCK
**	Returns:
**	    E_DB_{OK,WARN,ERROR,FATAL}
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	11-JUN-86 (daved)
**          written
**	16-oct-87 (puree)
**	    destroy temporary tables when closing a cursor.
**	11-mar-88 (puree)
**	    remove DSH_QUEUE structure. DMT_CB's can be linked together
**	    using the standard jupiter control block header.
**	19-may-88 (puree)
**	    clear the dmt_record_access_id so we know that this table is
**	    closed.
**	05-feb-89 (carl)
**	    Modified for Titan
**	23-feb-89 (paul)
**	    Clean up qef_cb state before returning.
**	12-feb-93 (jhahn)
**	    Added support for statement level rules. (FIPS)
**	11-nov-1998 (somsa01)
**	    We need to make sure that the continuation flag is reset.
**	    (Bug #94114)
**	24-Aug-2007 (kibro01) b114735 b119002
**	    Use the open count to determine if we have anything to do.
[@history_line@]...
*/
DB_STATUS
qeq_close(
QEF_RCB		    *qef_rcb )
{
    DB_STATUS		status, sav_status = E_DB_OK;
    DB_ERROR		sav_error;
    QEF_CB		*qef_cb = qef_rcb->qef_cb;
    QEE_DSH		*dsh;

    qef_cb->qef_rcb = qef_rcb;

    if (qef_cb->qef_open_count == 0)
    {
	/* For STAR servers, we have sometimes (b114735) already closed
	** the cursor at the end of the return of data, but equally
	** sometimes (b119002) we sometimes leave it open and leave locks.
	** We can determine this only by checking the open count, so
	** if that is 0 we have already closed the cursor, so return OK
	*/
	return (E_DB_OK);
    }

    /* Search for the DSH of the specified QP */
    if (status = qeq_dsh(qef_rcb, 1, &dsh, QEQDSH_NORMAL, -1))
	return (status);

    /* This cursor is closed, update number of open query/cursors. */
    qef_cb->qef_open_count--;
    
    /*
    ** We need to make sure that the continuation flag is reset to zero.
    */
    if ((dsh->dsh_act_ptr) && (dsh->dsh_act_ptr->ahd_atype == QEA_GET) &&
	(dsh->dsh_act_ptr->qhd_obj.qhd_qep.ahd_current))
    {
	QEN_ADF  *qen_adf = dsh->dsh_act_ptr->qhd_obj.qhd_qep.ahd_current;
	ADE_EXCB *ade_excb = (ADE_EXCB*) dsh->dsh_cbs[qen_adf->qen_pos];

	ade_excb->excb_continuation = 0;
    }

    /* Handle deferred cursor flag.  Needed here in case qeq_cleanup calls
    ** qet_savepoint rather than qet_commit/qet_abort. 
    ** Only one deferred cursor a time allowed in a session.  So if this one 
    ** is a deferred cursor, mark no more deferred cursor in the session cb */

    if (dsh->dsh_qp_status & DSH_DEFERRED_CURSOR)
	    qef_cb->qef_defr_curs = FALSE;

    if (qef_cb->qef_c1_distrib & DB_3_DDB_SESS)
    {
	status = qeq_c1_close(qef_rcb, dsh);
	if (status)
	{
	    sav_status = status;
	    STRUCT_ASSIGN_MACRO(qef_rcb->error, sav_error);
	}

	/* clean up the cursor execution environment */
	status = qeq_cleanup(qef_rcb, E_DB_OK, TRUE);

	if (sav_status)
	{
	    status = sav_status;
	    STRUCT_ASSIGN_MACRO(sav_error, qef_rcb->error);
	}
    }
    else
    {
	/* clean up the cursor execution environment */
	status = qeq_cleanup(qef_rcb, E_DB_OK, TRUE);
    }

    /* preserve XACT status as of this statement end */
    qef_cb->qef_ostat = qef_cb->qef_stat;  

    /* We follow the convention the qef_cb->qef_dsh == NULL when QEF is	    */
    /* being invoked with a user request. If this field is not NULL, QEF    */
    /* assumes that it is being called back to execute a nested procedure   */
    /* invocation. */
    qef_cb->qef_dsh = NULL;
    return (status);
}

/*
** {
** Name: QEQ_FETCH	- fetch the next row(s) from the cursor
**
** External QEF call:		status = qef_call(QEQ_FETCH, &qef_rcb);
**
** Description:
**      The next row(s) is(are) returned to the user. The cursor must have been
**	earlier opened with a QEQ_OPEN command.
**
** Inputs:
**      qef_rcb
**	    .qef_eflag		designate error handling semantics
**				for user errors.
**		QEF_INTERNAL	return error code.
**		QEF_EXTERNAL	send message to user.
**	    .qef_cb		session control block
**	    .qef_qp		name of query plan
**	    .qef_qso_handle	internal QP id. Used instead of qef_qp if not
**				zero.
**	    .qef_rowcount	number of rows to read (may be > 1)
**		...		Also indicates the number of output buffers
**				provided in . . .
**	    .qef_output		output buffer
**
** Outputs:
**      qef_rcb
**	    .qef_remnull	SET if an aggregate computation found a NULL val
**	    .qef_rowcount	number of output rows
**	    .qef_targcount	number of output rows
**	    .qef_count		Number of input buffers that were used.
**				As described in qefrcb.h, this may be different
**				than the number of rows.
**	    .error.err_code	one of the following
**				E_QE0000_OK
**				E_QE0017_BAD_CB
**				E_QE0018_BAD_PARAM_IN_CB
**				E_QE0019_NON_INTERNAL_FAIL
**				E_QE0002_INTERNAL_ERROR
**				E_QE0008_CURSOR_NOT_OPENED
**				E_QE0021_NO_ROW
**				E_QE0015_NO_MORE_ROWS
**				E_QE0024_TRANSACTION_ABORTED
**				E_QE0034_LOCK_QUOTA_EXCEEDED
**				E_QE0035_LOCK_RESOURCE_BUSY
**				E_QE0036_LOCK_TIMER_EXPIRED
**				E_QE002A_DEADLOCK
**
**                              E_AD0002_INCOMPLETE -- in case of the
**                                fetch of a large object that won't fit.
**	Returns:
**	    E_DB_{OK,INFO,WARN,ERROR,FATAL}
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	11-JUN-86 (daved)
**          Written
**	14-oct-87 (puree)
**	    On exit, do not call qef_error since all the called routines
**	    have processed the errors.
**	09-dec-87 (puree)
**	    Fixed bug 1460.  Check if dsh_act_ptr is set (by qeq_validate).
**	    If not, return E_QE0015_NO_MORE_ROWS.
**	02-feb-88 (puree)
**	    Added reset flag to qeq_validate and all qea_xxxx call sequence.
**	12-sep-88 (puree)
**	    Reset singleton bit in the query plan for cursor fetch from
**	    constant function to work right.  This is a temporary fix since
**	    the QP should be a read-only object for QEF.
**	12-oct-88 (puree)
**	    If a table validation fails during cursor fetch, abort the
**	    transaction.
**	05-feb-89 (carl)
**	    Modified for Titan
**	23-feb-89 (paul)
**	    Clean up qef_cb state before returning.
**	10-apr-89 (paul)
**	    DSH must be set to not in use before returning from cursor
**	    operation.
**	26-may-89 (paul)
**	    Deleted unused actions out of routines.
**	29-nov-89 (fred)
**	    Added support for peripheral objects.  This amounts to noticing the
**	    INCOMPLETE/INFO status returned by qea_fetch() and passing it along.
**	09-may-90 (neil)
**	    Cursor performance I: Allow the pre-fetch of multiple rows.
**	    Accumulate row counts over multiple actions.
**	05-jun-90 (nancy)
**	    bug 21652 -- call qef_error() when cursor fetch fails so that
**	    proper error mapping can get done. (lock timeout was not getting
**	    mapped)
**	04-feb-91 (neil)
**	    Removed extra E_DB_OK that was left over from 6.3->6.5 merge.
**	23-oct-92 (fpang)
**	    Unset DSH_IN_USE bit after distributed fetch.
**      07-Jan-1993 (fred)
**          Modified to pass the E_DB_INFO/E_AD0002_INCOMPLETE status
**          on when that is returned from qea_fetch().  This is often
**          the case when fetching from a large object cursor.
**	12-feb-93 (jhahn)
**	    Added support for statement level rules. (FIPS)
**      15-Oct-1993 (fred)
**          Added large object support for STAR to parallel that for
**          local databases done above.
**	30-dec-03 (inkdo01)
**	    Add "dsh" and replace "reset" with "function" in many calls to 
**	    qea_ action processors and qen_ node processors.
**	23-feb-04 (inkdo01)
**	    Add calls to copy from DSH to RCB for || query thread safety.
**	24-mar-04 (inkdo01)
**	    Add processing of DSH_DONE_1STFETCH flag to guide reset flag
**	    in cursor processing.
**	16-july-2007 (dougi)
**	    Don't set DONE_1STFETCH if we scrolled to BEFORE.
**	14-May-2010 (kschendel) b123565
**	    Split validation into two parts, fix here.
*/
DB_STATUS
qeq_fetch(
QEF_RCB		*qef_rcb )
{
    i4		err;
    DB_STATUS		status	= E_DB_OK;
    QEF_CB		*qef_cb = qef_rcb->qef_cb;
    QEE_DSH		*dsh;
    QEF_AHD		*act;
    i4		ask_rowcount = qef_rcb->qef_rowcount;	/* # to fetch */
    i4		tot_rowcount;				/* # fetched */

    qef_cb->qef_rcb = qef_rcb;
    qef_rcb->qef_remnull = 0;
    qef_rcb->qef_count = 0;

    /* Search for the DSH of the specified QP */

    if (status = qeq_dsh(qef_rcb, 1, &dsh, QEQDSH_NORMAL, -1))
	return (status);

    /* assume that this fetch will leave us unpositioned
    ** we will mark that we are positioned after a successful fetch.
    */
    dsh->dsh_positioned = FALSE;

    if (qef_cb->qef_c1_distrib & DB_3_DDB_SESS)	    /* DDB Processing? */
    {
	dsh->dsh_qp_ptr->qp_status &= ~QEQP_SINGLETON;

	status = qeq_c3_fetch(qef_rcb, dsh);
	dsh->dsh_qp_status &= ~DSH_IN_USE;
	if ((status == E_DB_OK)
	    ||
	    ((status == E_DB_INFO) &&
	         (qef_rcb->error.err_code
		          == E_AD0002_INCOMPLETE)))
	{
	    dsh->dsh_positioned = TRUE;
	    return (status);
	}

	/* If error other than NO_MORE_ROWS, abort the cursor.  If 
	** NO_MORE_ROWS, return.
	*/
	if (qef_rcb->error.err_code == E_QE0015_NO_MORE_ROWS)
	    return(status);
	else
	    return(qeq_cleanup(qef_rcb, (DB_STATUS) status, (bool) TRUE));
    }

    /* LDB processing */
    /* make sure we have a fetch action type */

    if (dsh->dsh_act_ptr == (QEF_AHD *)NULL)
    {
	qef_rcb->error.err_code = E_QE0015_NO_MORE_ROWS;
	dsh->dsh_qp_status &= ~DSH_IN_USE;
	return (E_DB_WARN);
    }

    if (dsh->dsh_act_ptr->ahd_atype != QEA_GET)
    {
	/* no legal row here */
	qef_error(E_QE0021_NO_ROW, 0L, E_DB_ERROR, &err, &qef_rcb->error, 0);
	dsh->dsh_qp_status &= ~DSH_IN_USE;
	return (E_DB_ERROR);
    }

    /* TEMPORARY FIX.
    ** Singleton bit in the QP is meaningless for a cursor.  In fact, it
    ** leads to an error.  Cursor fetches behave just like scanning through 
    ** a sequential file, you don't get an EOF if you get a record.  The 
    ** reverse is also true.  So, we turn off the singleton bit here for 
    ** SCF-QEF and application programs to be in sync. */

    dsh->dsh_qp_ptr->qp_status &= ~QEQP_SINGLETON;

    /* now fetch the tuple */
    qeq_rcbtodsh(qef_rcb, dsh);
    if (!(dsh->dsh_qp_status & DSH_DONE_1STFETCH))
	qef_rcb->qef_curspos = 0;	    /* init cursor position */
    status = qea_fetch(dsh->dsh_act_ptr, qef_rcb, dsh, 
	(dsh->dsh_qp_status & DSH_DONE_1STFETCH) ? (i4) NO_FUNC : 
					(i4) NO_FUNC | FUNC_RESET);
    if (qef_rcb->qef_curspos > 0)
	dsh->dsh_qp_status |= DSH_DONE_1STFETCH;
    qeq_dshtorcb(qef_rcb, dsh);
    if ((status == E_DB_OK)
	    ||
	((status == E_DB_INFO) &&
			(qef_rcb->error.err_code
			    == E_AD0002_INCOMPLETE)))
    {
	/*
	**	If the query worked -- which can be either because it is
	**	completely done, or because it is partially done and
	**	needs to be called back -- situation is the same in
	**	either case...
	**
	** Mark the DSH as available for the DSh search
	** routine. When the fetch next returns, we must find
	** this DSH.
	*/

	/*
	** NOTE THAT THIS IS A POTENTIAL BUG WHEN A SINGLE SESSION
	** CAN OPEN A QUERY PLAN (SHARED) MORE THAN ONCE.  IN THAT
	** CASE, THE DSH SEARCH ROUTINE CANNOT DISTINGUISH ONE
	** INVOCATION OF A QUERY FROM ANOTHER.
	**
	** (fred)
	*/

	dsh->dsh_positioned = TRUE;

	/*
    	** We follow the convention the qef_cb->qef_dsh == NULL when QEF is   
	** being invoked with a user request. If this field is not NULL, QEF   
	** assumes that it is being called back to execute a nested procedure   
	** invocation.
    	*/

	qef_cb->qef_dsh = NULL;

	/*
    	** Mark the DSH as available for the DSh search routine. When the
	** fetch next returns, we must find this DSH.
    	*/

	dsh->dsh_qp_status &= ~DSH_IN_USE;
	return (status);
    }

    /* If error other than NO_MORE_ROWS, abort the cursor.  If 
    ** NO_MORE_ROWS, fall through to process the next action.
    */
    if (qef_rcb->error.err_code != E_QE0015_NO_MORE_ROWS)
    {
	qef_error(qef_rcb->error.err_code, 0L, status, &err,
	    	  &qef_rcb->error, 0);
	return (qeq_cleanup(qef_rcb, status, TRUE));
    }

    /*
    ** Need to fetch more rows?  Keep track of the number gotten so far and
    ** make sure not to exceed the original requested row count.
    */
    ask_rowcount -= qef_rcb->qef_rowcount;	/* Decrease by # so far */
    tot_rowcount = qef_rcb->qef_rowcount;	/* Got this # so far */

    /* Start with the next action, process actions until we get a row */

    act = dsh->dsh_act_ptr->ahd_next;
    while (act != (QEF_AHD *)NULL)
    {
	status = qeq_topact_validate(qef_rcb, dsh, act, TRUE);
	if (status == E_DB_OK)
	    status = qeq_subplan_init(qef_rcb, dsh, act, NULL, NULL);
	qeq_dshtorcb(qef_rcb, dsh);
	if (status != E_DB_OK)		
	{
	    if (qef_rcb->error.err_code == E_QE0015_NO_MORE_ROWS &&
	        act->ahd_atype != QEA_GET)
	    {
		qef_rcb->error.err_code = E_QE0000_OK;
		status = E_DB_OK;
		act = act->ahd_next;
		continue;
	    }
	    if (status == E_DB_WARN)
	        status = E_DB_ERROR;
	    break;
	}

	qef_rcb->qef_rowcount = ask_rowcount;	/* # we have left to get >= 0 */
	switch (act->ahd_atype)
	{
	    case QEA_DMU:
		status = qea_dmu(act, qef_rcb, dsh);
		break;

	    case QEA_SAGG:
		status = qea_sagg(act, qef_rcb, dsh, (i4) NO_FUNC);
		break;

	    case QEA_PROJ:
		status = qea_proj(act, qef_rcb, dsh, (i4) NO_FUNC);
		break;

	    case QEA_AGGF:
		status = qea_aggf(act, qef_rcb, dsh, (i4) NO_FUNC);
		break;

	    case QEA_GET:
		/* Get more - zap error in case ask_rowcount is down to 0 */
		qef_rcb->error.err_code = E_QE0000_OK;
		status = qea_fetch(act, qef_rcb, dsh, (i4) NO_FUNC);
		if (status == E_DB_OK)
		{
		    dsh->dsh_positioned = TRUE;
		    /* Mark the DSH as available for the DSh search	    */
		    /* routine. When the fetch next returns,		    */
		    /* we must find this DSH. */
		    dsh->dsh_qp_status &= ~DSH_IN_USE;
		    /* Increase # rows just gotten with previous total */
		    dsh->dsh_qef_rowcount += tot_rowcount;
		    return (E_DB_OK);
		}
		else if (status == E_DB_WARN)
		{
		    /* Going to continue with more actions - track totals */
		    ask_rowcount -= dsh->dsh_qef_rowcount;
		    tot_rowcount += dsh->dsh_qef_rowcount;
		}
		break;

	    default:
		status = E_DB_ERROR;
		break;
	}
	qeq_dshtorcb(qef_rcb, dsh);
	if (DB_FAILURE_MACRO(status))
	    break;

	/* get the next action */
	act = act->ahd_next;

	/* preserve XACT status as of this action end */
	qef_cb->qef_ostat = qef_cb->qef_stat;  

    }

    dsh->dsh_qp_status &= ~DSH_IN_USE;
    if (DB_FAILURE_MACRO(status))
    {
	qef_error(qef_rcb->error.err_code, 0L, status, &err,
		  &qef_rcb->error, 0);
	status = qeq_cleanup(qef_rcb, status, TRUE);
    }
    else				/* Total # of rows collected so far */
    {
	qef_rcb->qef_rowcount = tot_rowcount;
    }

    /* We follow the convention the qef_cb->qef_dsh == NULL when QEF is	    */
    /* being invoked with a user request. If this field is not NULL, QEF    */
    /* assumes that it is being called back to execute a nested procedure   */
    /* invocation. */
    qef_cb->qef_dsh = NULL;
    return (status);
}

/*
** {
** Name: QEQ_QUERY	- a set query is executed
**
** Synopsis:
**	DB_STATUS	status;
**	QEF_RCB		*qef_rcb;
**	    status = qef_call(QEQ_QUERY, &qef_rcb);
**
** Description:
**	A set query statement or a database procedure whose QP is specified
**	in qef_rcb.qef_qso_handle or qef_rcb.qef_qp will be executed.  QEF
**	first obtains the QP from QSF.  If the QP is not found, the error
**	E_QE0014_NO_QUERY_PLAN will be returned.  If the QP is found, it 
**	will be validated by comparing the timestamps of the tables in the
**	QP with the timestamps in the database.  If any of the timestamp 
**	is different, the QP is considered invalid and the error code 
**	E_QE0023_INVALID_QUERY will be returned.
**
**	If the QP is not opened, it will be opened the first time the call
**	is made.  If the query is a RETRIEVE, the caller can repeat the
**	call many times until all rows have been retrieved.  At this time,
**	the QP will be closed (and released).  For all other queries, the QP
**	is closed when the execution is completed.
**
**	If there is no current transaction, a single statement transaction
**	is created on behalf of the user.  Only one query/cursor can be 
**	opened during such a transaction.
**
**	If a CALLPROC action is executed and the QP for the CALLPROC cannot
**	be found, E_QE0119_LOAD_QP will be returned indicating that the
**	procedure whose name is currently found in the QEF_RCB must be
**	loaded into QSF. The current execution environment is retained when
**	this error is returned. qeq_query() expects to be called back with
**	the execution environment and the RCB intact.
**
**	A similar situation may occur when performing select/retrieve queries
**	which involve peripheral datatypes.  In these cases, QEF may be unable
**	to fit the entire data value in the space provided.  In these cases,
**	this routine will return with a status of E_DB_INFO, and an error code
**	of E_AD0002_INCOMPLETE, which tells the caller that he/she/it should
**	dispose of the current buffer, and recall this routine with the
**	execution environment and RCB intact.  The mechanism used for this
**	activity is built atop the mechanism used for the aforementioned
**	CALLPROC processing.  The DSH of the query will be saved in
**	qef_cb->qef_dsh, and the internal state vector will have the
**	QEF_IN_PERIPH_MASK bit set.  On callback, this routine will note the bit
**	set in the vector, and reset appropriately.  In the event that some
**	error occurs and further processing is either impossible or undesirable,
**	this routine must be called back with the QEF_CLEAN_RSRC bit set.
**	
**	This routine may be called with different modes - with time more 
**	more special-purpose statements will be collapsed into action lists to
**	be processed by qeq_query.  Some modes cause small modifications to
**	the generic behavior (eg. cursor updates must not decrement the "open
**	count" when leaving).  Since nested procedure invocation (and rules)
**	may leave this module to have their plans recompiled the value of the
**	original request mode must be stacked together the rest of the QEF_RCB.
**	
** Inputs:
**      qef_rcb
**	    .qef_eflag		designate error handling semantics
**				for user errors.
**		QEF_INTERNAL	return error code.
**		QEF_EXTERNAL	send message to user.
**	    .qef_cb		session control block
**	    .qef_d_id		dmf session id
**	    .qef_db_id		database id
** 	    .qef_qp		name of query plan (DB_CURSOR_ID type)
**	    .qef_qso_handle	internal QP id. Used instead of qef_qp if not
**				zero.
**	    .qef_param		base of parameters for update
**	    .qef_rowcount	number of output rows (retrieve only)
**	    .qef_output		base of output row buffer (retrieve only)
**	    .qef_qacc		open command modifier
**		    QEF_READONLY   treat query as read-only
**		    QEF_UPDATE	    query can be for update
**	mode			query mode for action list:
**		    QEQ_QUERY	    generic query processing
**		    QEQ_DELETE	    delete cursor action list
**		    QEQ_REPLACE	    update cursor action list
**				this mode may be restored from a previously
**				stacked RCB that was used for a nested procedure
**				invocation.
**
** Outputs:
**      qef_rcb
**	    .qef_remnull	SET if an aggregate computation found a NULL val
**	    .qef_rowcount	number of output rows or rows affected
**	    .qef_targcount	number of output rows or attempted updates.
**	    .qef_count		Number of input buffers that were used.
**				As described in qefrcb.h, this may be different
**				than the number of rows.
**	    .error.err_code	one of the following
**				E_QE0000_OK
**				E_QE0017_BAD_CB
**				E_QE0018_BAD_PARAM_IN_CB
**				E_QE0019_NON_INTERNAL_FAIL
**				E_QE0002_INTERNAL_ERROR
**				E_QE0014_NO_QUERY_PLAN
**				E_QE0009_NO_PERMISSION
**				E_QE0012_DUPLICATE_KEY
**				E_QE0010_DUPLIATE_ROW
**				E_QE0011_AMBIGUOUS_REPLACE
**				E_QE0013_INTEGRITY_FAILED
**				E_QE0004_NO_TRANSACTION
**				E_QE000D_NO_MEMORY_LEFT
**				E_QE000E_ACTIVE_COUNT_EXCEEDED
**				E_QE000F_OUT_OF_OTHER_RESOURCES
**				E_QE0015_NO_MORE_ROWS
**				E_QE0024_TRANSACTION_ABORTED
**				E_QE0034_LOCK_QUOTA_EXCEEDED
**				E_QE0035_LOCK_RESOURCE_BUSY
**				E_QE0036_LOCK_TIMER_EXPIRED
**				E_QE002A_DEADLOCK
**				E_QE0119_LOAD_QP
**				E_QE0208_EXCEED_MAX_CALL_DEPTH
**				E_QE0209_BAD_RULE_STMT
**				E_QE0301_TBL_SIZE_CHANGE
**	Returns:
**	    E_DB_{OK,WARN,ERROR,FATAL}
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	11-JUN-86 (daved)
**          written
**	16-oct-87 (puree)
**	    destroy temporary tables at the end of a set query.
**	11-jan-88 (puree)
**	    added debug code to trap uninitialize error code from called
**	    routine.
**	02-feb-88 (puree)
**	    added reset flag to qeq_validate and all qea_xxxx call sequence.
**	11-mar-88 (puree)
**	    remove DSH_QUEUE structure. DMT_CB's can be linked together
**	    using the standard jupiter control block header.
**	09-may-88 (puree)
**	    many changes related to DB procedures.
**	19-may-88 (puree)
**	    clear the dmt_record_access_id so we know that this table is
**	    closed.
**	02-aug-88 (puree)
**	    modify the action execution loop to handle error condition
**	    in DB procedure (the execution of a DB procedure must continue
**	    atfer an error is handled).
**	22-aug-88 (puree)
**	    must call qef_error if qeq_validate fails when the query is 
**	    initially opened.
**	25-aug-88 (puree)
**	    After commit and rollback action, the count of open queries must
**	    be set to 1 to indicate that the current query is opened.  This
**	    is the case of executing a DB procedure which contains commit
**	    and/or rollback statement.
**	29-aug-88 (puree)
**	    Implement iirowcount and iierrornumber for DB procedures.
**	10-oct-88 (carl)
**	    Changes for Titan
**	10-oct-88 (puree)
**	    Added proper casting for iirowcount and iierrorno.
**	10-nov-88 (puree)
**	    do not change qef_rowcount in IF and MESSAGE statements.
**	23-nov-88 (puree)
**	    Modify qeq_query not to continue executing DB procedure if the
**	    validation fails in commit and rollback actions (BUG 3917)
**	23-dec-88 (carl)
**	    More hanges for Titan: moved end-of-query handling here for
**	    destroying temporary tables
**	15-jan-89 (paul)
**	    Added support to execute a rule action list by suspending the
**	    current action list and continuing with the rule action list.
**	    On completion the original action is restarted. Also added support
**	    for stacking the current QP execution environment and calling
**	    another QP via a nested procedure call.
**	    Added support for the QEA_EMESSAGE action.
**	3-mar-89 (paul)
**	    Match rules processing to SCF and PSF changes. Use qef_intstate
**	    in QEF_RCB to determine if a LOAD_QP or INVALID_QP request
**	    during query processing has failed.
**	23-mar-89 (paul)
**	    Corrected problem with invoking rules on a set update involving
**	    multiple rows. Also added support for an error callback if
**	    procedure creation fails for a nested procedure call.
**	04-apr-89 (teg)
**	    Added QEF Internal Procedure support.
**	11-apr-89 (neil)
**	    Return error for COMMIT/ROLLBACK in rule processing - E_QE0209.
**	17-apr-89 (paul)
**	    Corrected unix compiler warnings.
**	17-apr-89 (paul)
**	    Converted code to use qef_rcb->qef_rule_depth to track
**	    current rule nesting level replacing incorrect usage
**	    of qef_stat for this function in prior versions.
**	23-may-89 (carl)
**	    Fixed to initialize QES_03CTL_READING_TUPLES for SELECT queries
**	    to prevent premature DDL_CONCURRENCY commit
**	31-may-89 (neil)
**	    On call back from SAVED_RSRC re-enters at last parent action,
**	    which is QEA_CALLPROC.  Also fixed updating of context depth.
**	13-jun-89 (paul)
**	    Correct bug 6346. Do not reset local variables in a DB
**	    procedure after a COMMIT or ROLLBACK action.
**	3-aug-89  (nancy) -- Fix bug 7339 where a select that will always
**	    evaluate to false ends the execution of a dbproc prematurely.
**	10-aug-89 (neil)
**	    Update bug fix for 7339.  One remaining case that still fails is
**	    when a UNION SELECT is issued where one of the SELECT clauses
**	    has a constant qualification that yields FALSE.  This will fail
**	    and drop out with 0 (or fewer than expected) rows (when outside
**	    of a DBP).  The fix for 7339 will automatically solve the case
**	    inside a DBP.
**	15-aug-89 (neil)
**	    Term 2: Fix rowcounts/status on return from nested procedures.
**	07-sep-89 (neil)
**	    Alerters: Added recognition of EVENT actions.
**	26-sep-89 (neil)
**	    Support for SET NORULES.
**	07-oct-89 (carl)
**	    Modified qeq_query to call qel_u3_lock_objbase to avoid STAR 
**	    deadlocks
**	14-nov-89 (carl)
**	    Added qet_t9_ok_w_ldbs call for 2PC
**	29-nov-89 (fred)
**	    Added support for peripheral objects.  This amounts to noticing the
**	    INCOMPLETE/INFO status returned by qea_fetch() and passing it along.
**	10-jan-90 (neil)
**	    Cleaned up some DSH handling for rules:
**	    1. Unless the QEF_SAVED_RSRC flag is set never stack the DSH.
**	    2. Never reference through a saved DSH pointer after it is freed.
**	19-jan-90 (sandyh)
**	    When error condition from qeq_validate is E_QE0023_INVALID_QUERY
**	    and it is an execution of a procedure, set status = E_DB_ERROR and
**	    log error E_QE0115_TABLES_MODIFIED. Also, set qef_dbp_status upon
**	    procedure invocation to insure local variables are not corrupted
**	    in user code by procedures w/o a return action (bug 8502).
**	01-feb-90 (nancy)
**	    Fixed bug 7514 where an error condition in a dbproc "corrupted"
**	    local variables.  Set init_action to FALSE when calling 
**	    qeq_validate after error in dbproc.  This prevents re-initializa-
**	    tion of dbproc vars (i.e. corruption).
**	16-apr-90 (neil)
**	    Fixed rules/iirowcount problem where a failed rule fired from
**	    within a DBP did not update the iirowcount/iierrorno values of
**	    the firing DBP. (B21195)
**	20-apr-90 (davebf)
**	    Fixed bug 8875 enabling iterated update of given row inside
**	    a dbp.  This is a partial, not the ultimate fix.  It fixes
**	    the case where the iterated loop is not nested.  In the case
**	    where there is a loop within a loop (including nested procs)
**	    the inner loops will still be vulnerable to the error.
**	21-jul-90 (carl)
**	    Changed to process trace points
**	06-aug-90 (mikem)
**	    Bug #32320 - failure of qeq_validate from qeq_query would not
**	    back out transaction.  The fix is to make sure normal rule
**	    error processing happens after a deadlock encountered in the
**	    qeq_validate.
**	13-oct-90 (carl)
**	    1) hide E_DB_WARN (which is NOT an error condition) from 
**	       qeq_cleanup to prevent aborting a good retrieval
**	    2) test for trace point 51
**	10-dec-90 (neil)
**	    TM EXECUTE DBP: Fix rowcounts on return from dbps + trace point.
**	    Alerters: Added recognition of EVENT actions.
**	04-feb-91 (neil)
**	    Trace DBP return statements
**      15-jul-91 (stevet)
**          Bug #37995.  Added act_reset flag to qeq_query to determine if
**          actions should be executed with reset.  Without resetting, DBP
**          with WHILE LOOP can fail because not all nodes within a query plan
**          were executed on second and subsequent loops.
**      07-aug-91 (mikem)
**          bug #32999
**          Change qeq_query to include logical key return values as part
**          saved and restored context manipulated as part of a procedure
**          call.  This change will insure that logical keys allocated as
**          part of procedure calls caused by recursive rule firing will not
**          overwrite the logical key return value which should be returned
**          to the user (ie. only the value which resulted from a user issued
**          insert statement into a table with a system maintained logical
**          key.)
**	12-feb-92 (nancy)
**	    Bug 38565.  Check for new return status/error from qeq_validate(),
**	    E_QE0301_TBL_SIZE_CHANGE and return this error so that scf will
**	    NOT increment retry counter.  This allows queries that do a large
**	    insert to proceed to end when insert fires a rule in which the
**	    dbp references a table changing in size.
**	11-mar-92 (jhahn)
**	    Added support for QEF interrupts in dbprocs.
**	04-sep-92 (fpang)
**	    Set DSH_IN_USE for distributed cases.
**	11-sep-92 (andre)
**	    ensure that a transaction is in progress before calling qeq_dsh() 
**	    since it does constant expression evaluation and one of such
**	    expressions can be resolve_table() processing of which may involve 
**	    opening and reading catalogs.  As a result, if qeq_dsh()
**	    returns an error, we will invoke qeq_cleanup() to ensure proper 
**	    transaction semantics
**	24-oct-92 (fpang)
**	    Add QEA_D9_UPD to QEA_RUP case.
**	09-nov-92 (jhahn)
**	    Added handling for byrefs.
**	14-dec-92 (jhahn)
**	    Cleaned up handling of byrefs. Cleaned up nested handling of
**	    return_status.
**	15-dec-92 (anitap)
**	    Added support for QEA_EXEC_IMM type of actions for CREATE SCHEMA. 
**	    The query text needs to be passed back to SCF so that it can be 
**	    parsed. Also the query text for rules and procedures of constraints
**	    will also need to be passed back to SCF to be parsed so that the
**	    the procedures and rules can be created in order without any 
**	    backward references.
**	20-dec-92 (anitap)
**	    Added enhancements for CREATE SCHEMA project to create implicit 
**	    schemas. Also added new routine qeq_restore().
**	28-dec-92 (andre)
**	    added support for QEF_CREATE_VIEW action.
**	08-jan-93 (anitap)
**	    check for qef_intstate and do not restore the qp handle if we
**	    are dealing with QEA_EXEC_IMM type of action. Check for qef_intstate
**	    to reset at the end of the routine.
**      12-feb-93 (jhahn)
**          Added support for statement level rules. (FIPS)
**      28-feb-93 (andre)
**          rather than process QEA_CREATE_VIEW action here, call qea_cview()
**	    to handle it.  That function will handle creation of rule(s) and a
**	    dbproc to dynamically enforce CHECK OPTION for local SQL views when
**	    necessary.
**      03-mar-93 (rickh)
**          Fixes to execute immediate processing for CREATE_INTEGRITY.
**      04-mar-93 (anitap)
**          Changed function names qea_execimm() and qea_cschema() to
**          qea_xcimm() and qea_schema() to conform to standards.
**	14-mar-93 (andre)
**	    added support for QEA_DROP_INTEGRITY action
**	24-mar-93 (jhahn)
**	    Various fixes for support of statement level rules. (FIPS)
**	31-mar-93 (anitap)
**	    Added fixes for QEA_CREATE_INTEGRITY and QEA_CREATE_VIEW for
**	    "SET AUTOCOMMIT ON". qef_open_count was not being set to 0
**	    because the mode was being lost.
**	02-apr-93 (jhahn)
**	    Made set input procedures called from rules bump qef_rule_depth.
**	01-may-93 (jhahn)
**	    Undid above change. Instead added new action QEA_INVOKE_RULE.
**	9-jun-93 (rickh)
**	    Added 2 new arguments to qeu_dbu call.
**	15-june-93 (robf)
**	    For CREATE_TABLE action propagate error status from 
**	    qeu_cb to qef_rcb (symptom: spurious bad control block QEF errors)
**	16-jun-93 (jhahn)
**	    Added 'fix' so that we don't rollback the transaction when
**	    an unpositioned cursor update is attempted.
**      07-jul-93 (anitap)
**          Call qeq_restore() to restore context on return from SCF for
**          QEA_CALLPROC also.
**          Enhanced comments. Check status for QEA_CREATE_INTEGRITY and
**          QEA_CEATE_VIEW before returning to SCF to parse the E.I. qtext.
**          Check if dsh is null to avoid AV in E.I. code.
**	    Do not restore qef_rowcount (to avoid 0 rows diagnostic).	
**	26-jul-1993 (jhahn)
**	    Moved coercing of returned byref procedure parameters from the
**	    back end to the front end.
**	09-aug-93 (anitap)
**	    Added calls to qeq_setup() for SELECT ANY(1) statement of
**	    QEA_CREATE_INTEGRITY action of ALTER TABLE ADD CONSTRAINT 
**	    statement. 
**	    Special check needed after QEA_GET action to continue with
**	    creation of rules, procedures, index.	
**	26-aug-93 (rickh)
**	    NOT NULL integrites may follow the SELECT action compiled out of
**	    a CREATE TABLE AS SELECT statement.  If this happens, don't clobber
**	    the rowcount meticulously accumulated by the SELECT.
**	1-sep-93 (rickh)
**	    Added logic to clean up if errors occur in the SELECT ANY( 1 )
**	    statement generated by a ALTER TABLE ADD REFERENTIAL CONSTRAINT
**	    query.
**      15-Oct-1993 (fred)
**          Fixed STAR case to observe the complete/incomplete
**          protocol mentioned (way) above.
**	5-oct-93 (eric)
**	    Removed call to qeq_iterate as the dmt statement number is
**	    maintained using resource lists.
**	26-oct-93 (rickh)
**	    Changed variable name to reflect its use by both ALTER TABLE
**	    ADD CHECK CONSTRAINT and ALTER TABLE ADD REFERENTIAL CONSTRAINT.
**      27-jan-1994 (stevet)
**          Removed call to adu_free_objects() since BLOB stored in
**          temp tables may still needed.  Moved adu_free_objects() to
**          qeq_cleanup. (B59267)
**	17-mar-1994 (anitap)
**	    Fix for bug 59929. Error during SELECT of ALTER TABLE resulted
**	    in the whole transaction to be rolled back. This was because the
**	    number of open cursors (qef_cb->qef_open_count) was not being
**	    decremented. 
**	08-apr-1994 (pearl)
**	    Bug 56748.  Save the mode in QEF_RCB.qef_mode before 
**	    returning from database procedures.
**	19-apr-94 (davebf)
**	    Reset first_act when unwinding with no error from rule fired
**	    proc (b42514, b41108)
**	18-may-94 (anitap)
**	    Do not rollback the whole transaction when an error occurs in a
**	    nested dbproc (b63465). Changes in qeq_query().
**	24-april-96 (angusm)
**	    If in callback with CLEAN_RSRC flag set, and pointer to
**	    DSH is null, set QE0025 and call qeq_cleanup() directly,
**	    instead of waiting to go down into qea_callproc(). (bug 75401)
**	30-sep-96 (nick)
**	    DON'T call qeq_cleanup() if we need to go back out to SCF to
**	    reload a QP etc.  If we do, we silently abort transactions. #78523
**	30-oct-96 (nick)
**	    Another case where transactions were rolled back silently ; 
**	    nested database procedures with an commit or rollback would
**	    result in qef_open_count becoming negative.  This could also
**	    cause a segmentation violation with a bogus DSH pointer in some
**	    instances.
**	23-jul-96 (ramra01)
**	    Support for ALTER TABLE add drop col thru QEA_CREATE_TABLE
**	27-Feb-1997 (jenjo02)
**	    Extended <set transaction statement> to support <transaction access
**          mode> (READ ONLY | READ WRITE) per SQL-92.
**	    When starting a single statement txn with autocommit on, use query
**	    access mode passed by caller instead of forcing to QEF_UPDATE.
**	20-march-1997(angusm)
**	    Change to qeq_query when resuming after error in dbproc.(b80759)
**	    need to ensure that 'first_act' is set.
**	17-apr-1997 (nanpr01)
**	    Change the stupid code of setting variable to zero.
**      13-jun-97 (dilma04)
**          When invoking a DB procedure, check if the procedure supports a 
**          constraint and set the bit flag in the QEF_RCB accordingly.
**	26-nov-1997 (somsa01)
**	    In the case of multiple user rules defined to an SQL operation,
**	    if the first rule fired fails, then the subsequent rules are not
**	    fired (since the action list containing these rules is
**	    "invalidated"). However, if one of these multiple rules is an
**	    internal rule (generated, for example, by an ALTER TABLE ADD
**	    REFERENTIAL CONSTRAINT), these rules are part of another action
**	    list. Therefore, if iierrorno has been set, "invalidate" this
**	    rules list as well. (Bug #86734)
**	10-dec-1997 (somsa01)
**	    Amended above change to bug #86734. "iierrorno" can be null.
**	28-may-1998 (shust01)
**	    Initialized act to NULL in qeq_query().  Problem only when NULL
**	    dsh pointer. act was being checked, but was never initialized.
**	    Caused qeq_validate() to be called (when it shouldn't have been)
**	    and quickly SEGV when act had garbage in it. bug 91068.
**      26-Jun-1998 (hanal04)
**          X-integ fix for bug 89898. When the first statement in a
**          dbproc is to execute another DBproc load the default values
**          saved during compilation into the DSH if values are not already
**          present. E.g. 'with null' variables do not have a default of
**          null because the null-bit is ADF_NVL_BIT (1). This value
**          must be placed in the DSH.
**	14-sep-2000 (hayke02)
**	    Check for QEF_GCA1_INVPROC (indicating a DBP executed from a
**	    libq front end e.g. ESQLC) and set qef_open_count and
**	    qef_context_cnt to 1 rather than 0. This prevents subsequent
**	    DBP failures because of the fix for bug 102123. This change
**	    fixes bug 102320.
**	18-sep-98 (stephenb)
**	    When running distributed queries, if we return without clearing
**	    the DSH, we need to note the fact so that we ca re-use it when we
**	    are called again in the same query context. Otherwhise we try
**	    to build it again, and end up locking the QP in QSF twice for the
**	    same query. This causes QSF memory leaks since the object is never
**	    destroyed because of a positive lock count.
**	29-oct-98 (inkdo01)
**	    Numerous changes for row-producing procedures, including support
**	    for QEA_FOR and QEA_RETROW action headers.
**	31-mar-00 (inkdo01)
**	    Tweak to for-loop setup.
**	18-june-01 (inkdo01)
**	    Use DSH_FORINPROC flag to avoid reset of qef_rowcount by callprocs.
**	30-jul-01 (devjo01)
**	    Make sure qp is set before testing qp_status for QEQP_ROWPROC.
**	    (b105373/INGSRV1508).
**      26-Jun-2001 (hanal04) Bug 87296 INGSRV 203
**	    (hanje04) X-Integ of change 451680
**          Cross integration of ocoro02's change 433657.
**          Bug 87296: Unwinding a multi-statement procedure results in
**          E_SC0216/E_SC0206 when part of the procedure fails due to a lock
**          timeout. Added mapping for E_DM004D with qef_error to ensure
**          DMF error doesn't escape to places which don't expect it.
**	11-Dec-2003 (wanfr01) Bug 110793 INGSTR 63
**	    The QES_05Q_DSH flag assumes a valid DSH and should be reset 
**	    when the DSH is destroyed.
**	30-dec-03 (inkdo01)
**	    Add "dsh" and replace "reset" with "function" in many calls to 
**	    qea_ action processors and qen_ node processors.
**      19-feb-04 (hayke02)
**          Modify the fix for 63465 to deal with a zero qef_context_cnt
**          (indicating that we are not in a nested DBP) - decrement
**          qef_open_count, rather than set it to zero, before the call to
**          qeq_cleanup(). This prevents E_CL1037 errors when multiple cursors
**          are open and a transaction is rolled back. This change fixes
**          problem INGSRV 2692, bug 111734.
**	23-feb-04 (inkdo01)
**	    Add calls to copy from DSH to RCB for || query thread safety.
**	23-Mar-2004 (schka24)
**	    qea_load needs RESET flag to pass downwards.
**	6-Apr-2004 (schka24)
**	    IF was cycling loop without dsh->rcb; fix up a couple
**	    rowcount suppressions to use the dsh.
**	21-apr-04 (inkdo01)
**	    Suppress dshcopy for qea_schema (it's called from qeu, too).
**	6-May-2004 (schka24)
**	    I/we missed one more rowcount-suppress place, drop constraint.
**	26-aug-04 (inkdo01)
**	    Add global base array support for "return" results.
**	16-Sep-2004 (schka24)
**	    Blind try for rowcount suppress for create schema (works for me).
**	20-sep-2004 (thaju02)
**	    Execute row-producing procedure fails with SIGSEGV followed by
**	    dbms server crash. Set dsh->dsh_qef_rowcount, rather than 
**	    qef_rcb->qef_rowcount. (B113090)
**	16-dec-04 (inkdo01)
**	    Inited a collID.
**      23-mar-2005 (hanal04) Bug 113556 INGSRV3074
**          For RPPs restore the rowcount from the DSH to ensure we
**          return the correct value for use in the determination of buffer
**          sizes in scc_send and later use in GCA.
**	13-Aug-2005 (schka24)
**	    RPP's need to return both row and buffer count for blobs.
**	 9-apr-08 (hayke02)
**	    Only apply the fix for bug 89898 to BYREF parameters (parm_flags
**	    & QEF_IS_BYREF). This prevents non-BYREF parameters containing 0
**	    or '' being set to NULL or garbage from parm_dbv.db_data. This
**	    change fixes bug 115857.
**    17-Apr-2008 (kschendel)
**        Comment update only re errors in dbp's.
**    08-Sep-2008 (gefei01)
**          Implemeted table procedure.
**	17-Nov-2009 (kschendel) SIR 122890
**	    Subsume load into qea-append.
**	15-Jan-2010 (jonj)
**	    In QEA_CALLPROC, check ahd_flags & QEF_CP_CONSTRAINT; not all
**	    procedures that are constraints have parameters.
**	15-Mar-2010 (troal01)
**	    Propagate the E_QE5424_INVALID_SRID error to user.
**	19-Mar-2010 (gupsh01) SIR 123444
**	    Added support for rename table/columns.
**	14-May-2010 (kschendel) b123565
**	    Validation split into two parts now, fix here.
**	21-May-2010 (kschendel) b123775
**	    If tproc validation / load interrupts an action that is itself
**	    a FOR-GET, e.g. in a rowproc, make sure that the continuation is
**	    done properly.
**	    Fix a little bad indentation.
**	24-Jun-2010 (kschendel) b123775
**	    Tproc validation redone, always happens on first action just like
**	    tables, reflect here.  Make sure that "clean-resources" callback
**	    from sequencer is properly handled when the failure was due to a
**	    tproc (as opposed to a explicit or rule-called dbp).
*/

DB_STATUS
qeq_query(
QEF_RCB		*qef_rcb,
i4		mode )
{
    i4		err;
    DB_STATUS		status	= E_DB_OK;
    QEF_CB		*qef_cb = qef_rcb->qef_cb;
    QEE_DSH		*dsh;
    QEF_QP_CB		*qp = (QEF_QP_CB *)NULL;
    QEF_AHD		*act = (QEF_AHD *)NULL;
    QEF_AHD		*first_act = (QEF_AHD *)NULL;
    QEF_AHD		*saved_act = (QEF_AHD *)NULL;
    PTR			*cbs;
    QEN_ADF		*qen_adf;
    ADE_EXCB		*ade_excb;
    i4		rowcount = qef_rcb->qef_rowcount;
    i4			*iirowcount, *iierrorno;
    i4			open_count;
    i4			act_state = DSH_CT_INITIAL;
    bool		prevreset, act_reset = TRUE;
    bool		errorDuringAddConstraintSelect = FALSE;
			/* set to true if an error occurred during the
			** the SELECT statement that supports an ALTER TABLE
			** ADD REFERENTIAL CONSTRAINT */
    QEE_DSH		*tmp_dsh;
    QES_DDB_SES		*dds_p;
    QES_QRY_SES		*qss_p;
    bool		ddb_b, val_qp_51, log_qry_55, log_err_59;
    i4		i4_1, i4_2;
    QEF_RCB		qeq_rcb;
    DB_TEXT_STRING      *text;
    PST_INFO            *qef_info;
    DB_SCHEMA_NAME	*qea_schname;
    i4			flag = EXPLICIT_SCHEMA;
    i4			func;
    QEUQ_CB		*qeuq_cb = (QEUQ_CB *)NULL;
    DB_ERRTYPE		err_save = -1;
    QEF_CREATE_INTEGRITY_STATEMENT	*integrityDetails;
    char		*cbuf = qef_cb->qef_trfmt;
    i4			cbufsize = qef_cb->qef_trsize;
    bool		prevfor = FALSE;
    /* b89898 */
    i4                  i,k;
    QEF_CP_PARAM        *cp_param;
    char                *ch;

    /* MUST first initialize boolean flags */

    if (qef_cb->qef_c1_distrib & DB_3_DDB_SESS)
    {
	ddb_b = TRUE;
	dds_p = & qef_cb->qef_c2_ddb_ses;
	qss_p = & qef_cb->qef_c2_ddb_ses.qes_d8_union.u2_qry_ses;
	val_qp_51 = FALSE,
	log_qry_55 = FALSE,
	log_err_59 = FALSE;

	if (ult_check_macro(& qef_cb->qef_trace, QEF_TRACE_DDB_VAL_QP_51,
	    & i4_1, & i4_2))
		val_qp_51 = TRUE;
  
	if (ult_check_macro(& qef_cb->qef_trace, QEF_TRACE_DDB_LOG_QRY_55,
	    & i4_1, & i4_2))
		log_qry_55 = TRUE;
  
	if (ult_check_macro(& qef_cb->qef_trace, QEF_TRACE_DDB_LOG_ERR_59,
	    & i4_1, & i4_2))
	    log_err_59 = TRUE;
    }
    else
	ddb_b = FALSE;


    /* 
    ** If this is a call back after we have requested a procedure
    ** re-creation and there qeq currently has resources allocated, we must
    ** de-allocate resources rather than execute a QP. This will occur only
    ** on nested procedure calls. 
    **				OR
    ** If this is a call back after we have requested a QEA_EXEC_IMM
    ** action to be parsed, qeq has resources allocated, we must
    ** de-allocate resources before executing the next QEA_EXEC_IMM
    ** action header. This will occur only for the DDL statements specified
    ** in CREATE SCHEMA statement and for CREATE TABLE with REFERENCES
    ** syntax for now.
    */		
    if (((qef_rcb->qef_intstate & QEF_DBPROC_QP) != 0) ||
	((qef_rcb->qef_intstate & QEF_EXEIMM_PROC) != 0))
    {
	/* 
	** At this point we restore the query plan that requested the
	** construction of a QP for a nested procedure. Since the nested
	** procedure could not be constructed, we consider the procedure
	** as having failed. At this point restore the QP that attempted
	** to call the failed nested procedure and continue execution at
	** the CALLPROC statement that failed. The CALLPROC action will
	** check to see if the procedure load has failed. If so, it will
	** generate a fatal error and continue with normal error recovery.
	**			OR
	** At this point we restore the query plan that requested the
        ** parsing of the query text for QEA_EXEC_IMM action header. At
        ** this point restore the QP that attempted to get the query text
        ** parsed and failed. The QEA_EXEC_IMM action will check to see if
        ** parsing of the query text has failed. If so, it will generate a
        ** fatal error and continue with normal error recovery.
	**			OR
	** If the sequencer is unable to recreate a requested QP, it may
	** be calling back so that QEF can clean up the in-progress query.
	** If there is no DSH, do the cleanup by hand here.  Otherwise
	** fall through, but with an error status so that query execution
	** aborts and unwinds in the normal manner.
	*/

	dsh = (QEE_DSH *) qef_cb->qef_dsh;
	if ((qef_rcb->qef_intstate & QEF_CLEAN_RSRC) != 0 || dsh == NULL)
	{
	    qef_rcb->error.err_code = E_QE0025_USER_ERROR;
	    if (dsh == NULL)
	    {
		status = qeq_cleanup(qef_rcb, E_DB_OK, TRUE);
		qef_rcb->qef_intstate &= ~(QEF_CLEAN_RSRC | QEF_DBPROC_QP | QEF_EXEIMM_PROC);
		return (status);
	    }
	}

	/* 
	** We saved the execution state of the old QP before trying to
	** create a new QP. At this point we restore the old QP's state.
	*/

	if ((qef_rcb->qef_intstate & QEF_DBPROC_QP) != 0)
	{
	    qeq_rcb = *(dsh->dsh_saved_rcb);

	    /* we do not want to restore the qp handle if we are dealing
	    ** with QEA_EXEC_IMM type of action.
	    */ 
	    qef_rcb->qef_qso_handle = qeq_rcb.qef_qso_handle;
	}
	else
	    qeq_rcb = *(dsh->dsh_exeimm_rcb);

	qef_rcb->qef_rowcount= qeq_rcb.qef_rowcount;

	qeq_restore(&qeq_rcb, qef_rcb, dsh, &mode, &act, &rowcount, &qp,
		    &cbs, &iirowcount);

	if (qp->qp_oerrorno_offset == -1)
	    iierrorno = (i4 *)NULL;
	else
	    iierrorno = (i4 *)(dsh->dsh_row[qp->qp_rerrorno_row] + 
			   qp->qp_oerrorno_offset);
	/* If resume action is null, start at the beginning.  (This
	** ought to be the normal case for tprocs.)
	** If resume action was a FOR-GET, and we're resuming from
	** a dbproc load, it pretty much has to be a tproc, and
	** also has to be the first execution of that GET (else we
	** wouldn't have been revalidating it.)  Turn on the prevfor
	** flag to show later code that this is the first time thru
	** this FOR-GET driven for loop.
	** (As of 123775 this really shouldn't happen because tprocs
	** are validated and recompiled at the start of the QP, which
	** would be the FOR itself if not earlier, but leave this in case.)
	*/
	first_act = qp->qp_ahd;
	if (act == NULL)
	    act = first_act;
	else if (qef_rcb->qef_intstate & QEF_DBPROC_QP
	  && act->ahd_atype == QEA_GET
	  && act->qhd_obj.qhd_qep.ahd_qepflag & AHD_FORGET)
	    prevfor = TRUE;
    }
    else
    {
	qef_rcb->qef_remnull = 0;
	qef_rcb->qef_count = 0;

	/* 
	** for distributed queries, check that we didn't save the DSH from
	** the last pass, if we try to build it again the QSF QP lock count
	** will be incrimented again, causing us to leave it locked in memory
	** for ever (since it is only decremented once at the end of the query).
	** And anyway, this is an un-necesary step
	*/
	if ((qef_cb->qef_c1_distrib & DB_3_DDB_SESS) &&
	    (qss_p->qes_q1_qry_status & QES_05Q_DSH) && qef_cb->qef_dsh)
	{
	    dsh = (QEE_DSH *) qef_cb->qef_dsh;
	}
	else
	{
	    /* Search for the DSH of the specified QP */
	    status = qeq_dsh(qef_rcb, 0, &dsh, QEQDSH_NORMAL, -1);
	    if (DB_FAILURE_MACRO(status))
	    {
		if ((qef_rcb->error.err_code == E_QE0023_INVALID_QUERY) ||
		    (qef_rcb->error.err_code == E_QE0014_NO_QUERY_PLAN) ||
		    (qef_rcb->error.err_code == E_QE0301_TBL_SIZE_CHANGE))
		{
		    return(status);
		}
		else
		{
		    return (qeq_cleanup(qef_rcb, status, TRUE));    
		}
	    }
	}

	/* Make sure to clear the DSH stack for new queries */
	dsh->dsh_stack = NULL;
	dsh->dsh_exeimm = NULL;
	/* Init row-producing procedure stuff in case it's an RPP */
	dsh->dsh_rpp_rowmax = qef_rcb->qef_rowcount;
	dsh->dsh_rpp_rows = 0;
	dsh->dsh_rpp_bufs = 0;
    
	qp = dsh->dsh_qp_ptr;

	/*
	** Start a transaction if necessary. If we are in autocommit mode
	** or using QUEL semantics, a single statement transaction will be
	** declared if there is no current transaction. With the extensions for
	** rules processing, a single statement transaction implies that the
	** transaction will be committed or aborted before this call to QEF
	** completes. However, the transaction may include several statements
	** due to rules processing.
	**
	** If autocommit is on, start the transaction as UPDATE (READ WRITE)
	** or READ ONLY as specified by the caller in qef_qacc. If autocommit
	** is off, the txn may be escalated to a MSTRAN and we have no way
	** of knowing what it will do, so we must start it UPDATE.
	** 
	** Check for special case of select with no from list in which case
	** we don't need to start a transaction.
	** 	NOTE special case for resolve_table() function (ADI_RESTAB_OP)
	**	select resolve_table('tableowner', 'tablename')
	**	is what gets sent for 'help tableowner.tablename'
	**	This looks like a constant function, but requires catalog info.
	**	In opc, we set the QEQP_NEED_TRANSACTION if we see ADI_RESTAB_OP
	*/
	if (qef_cb->qef_stat == QEF_NOTRAN &&
			((qp->qp_status & QEQP_SINGLETON) == 0 ||
			(qp->qp_status & QEQP_NEED_TRANSACTION) ||
			qp->qp_cnt_resources != 0 ||
			qp->qp_qmode != QEQP_01QM_RETRIEVE))
	{
	    qef_rcb->qef_modifier = QEF_SSTRAN;
	    if (qef_cb->qef_auto == QEF_ON)
		qef_rcb->qef_flag = qef_rcb->qef_qacc;
	    else
		qef_rcb->qef_flag = QEF_UPDATE;

	    if (status = qet_begin(qef_cb))
		return (qeq_cleanup(qef_rcb, status, TRUE));
		/* the error was already reported/logged by qet_begin */
	}

	/* set up local variables from QP */
	if (qp->qp_orowcnt_offset == -1)
	    iirowcount = (i4 *)NULL;
	else
	    iirowcount = (i4 *)(dsh->dsh_row[qp->qp_rrowcnt_row] + 
			    qp->qp_orowcnt_offset);

	if (qp->qp_oerrorno_offset == -1)
	    iierrorno = (i4 *)NULL;
	else
	    iierrorno = (i4 *)(dsh->dsh_row[qp->qp_rerrorno_row] + 
			    qp->qp_oerrorno_offset);

	/* Substitute parameters for DB procedure */

	if (qp->qp_qmode == QEQP_EXEDBP)
	{
	    qef_rcb->qef_dbp_status = 0L;   /* reset return value - bug 8502 */
	    if (qp->qp_ndbp_params != 0)
	    {
		qeq_rcbtodsh(qef_rcb, dsh);
		status = qee_dbparam(qef_rcb, dsh,
				     (QEE_DSH *)NULL,
				     (QEF_CP_PARAM *)NULL, -1, FALSE);
		qeq_dshtorcb(qef_rcb, dsh);
		if (DB_FAILURE_MACRO(status))
		    return (qeq_cleanup(qef_rcb, status, TRUE));
	    }
	}

	first_act = qp->qp_ahd;

	if ((act = dsh->dsh_act_ptr) == (QEF_AHD *)NULL && mode == QEQ_QUERY)
	{	/* the query/cursor has not been opened */
	    qef_cb->qef_open_count++;	    /* number of open query/cursors */
	    dsh->dsh_stmt_no = qef_cb->qef_stmt++; /* update statement number */
	}
	/* 
	** If this is the continuation of a GET action, there is no need to
	** validate the action.  It has already been validated when the
	** action was originally executed.  Notice: It also implies that the
	** QP is not for a DB procedure.
	*/
	if (act && act->ahd_atype == QEA_GET)
	{ 
	    /* LDB processing */
	    qeq_rcbtodsh(qef_rcb, dsh);
	    status = qea_fetch(act, qef_rcb, dsh, (i4) NO_FUNC);
	    qeq_dshtorcb(qef_rcb, dsh);

	    /* If the request is satisfied, return now even if there are more 
	    ** actions to go (like a retrieve loop). The caller will call again.
	    */
	    if (    (status == E_DB_OK)
		|| ((status == E_DB_INFO) &&
			(qef_rcb->error.err_code == E_AD0002_INCOMPLETE))
	       )
	    {
		/*
		**  If the query worked -- which can be either because it is
		**  completely done, or because it is partially done and
		**  needs to be called back -- situation is the same in
		**  either case...
		**
		** Mark the DSH as available for the DSh search
		** routine. When the fetch next returns, we must find
		** this DSH.
		*/

		/*
		** NOTE THAT THIS IS A POTENTIAL BUG WHEN A SINGLE SESSION
		** CAN OPEN A QUERY PLAN (SHARED) MORE THAN ONCE.  IN THAT
		** CASE, THE DSH SEARCH ROUTINE CANNOT DISTINGUISH ONE
		** INVOCATION OF A QUERY FROM ANOTHER.
		**
		** (fred)
		*/
		dsh->dsh_qp_status &= ~DSH_IN_USE;

		return (status);
	    }
	    /* If error other than NO_MORE_ROWS, terminate the query.  If 
	    ** NO_MORE_ROWS, fall through to process the next action.
	    */
	    if (qef_rcb->error.err_code != E_QE0015_NO_MORE_ROWS)
	    {
		qef_error(qef_rcb->error.err_code, 0L, status, &err,
		      &qef_rcb->error, 0);
		qef_cb->qef_open_count--;
		return (qeq_cleanup(qef_rcb, status, TRUE));
	    }
	}
	else if (act && (act->ahd_atype == QEA_D2_GET))
	{
	    /* DDB processing */
	    if (qss_p->qes_q1_qry_status & QES_02Q_EOD)
	    {
		/* handle end-of-data and more actions condition */

		if (act->ahd_next == (QEF_AHD *) NULL)	/* more actions ? */
		{
		    /* end of query plan: destroy DSH and return no-more-rows */

		    dds_p->qes_d7_ses_state = QES_0ST_NONE;	/* reset */
		    qef_rcb->qef_rowcount = 0;
		    qss_p->qes_q1_qry_status &= ~QES_05Q_DSH;

		    /* must return E_QE0015_NO_MORE_ROWS to signal exhaustion 
		    ** of data */

		    qef_rcb->error.err_code = E_QE0015_NO_MORE_ROWS;
		    status = E_DB_WARN;

		    /* fall thru to destroy the DSH */
		}
		else
		{
		    /* more actions to execute, use hold-and-execute strategy:
		    ** set the HOLD state and count, continue to execute until
		    ** end of query at which time no-more-rows is returned */

		    qss_p->qes_q1_qry_status |= QES_04Q_HOLD;
		    qss_p->qes_q4_hold_cnt = qef_rcb->qef_rowcount;
		    
		    /* must set code to execute next action below */

		    qef_rcb->error.err_code = E_QE0015_NO_MORE_ROWS;
		}
	    }
	    else
	    {	    
		/* execute GET action */

		status = qeq_d2_get_act(qef_rcb, act);

		if ((status == E_DB_OK)
		    || ((status == E_DB_INFO) &&
			(qef_rcb->error.err_code == E_AD0002_INCOMPLETE)))
		{
		    dsh->dsh_qp_status &= ~DSH_IN_USE;
		    return(status);	/* return data and save the DSH */
		}
		else 
		{
		    if (status == E_DB_WARN 
			&& 
			qef_rcb->error.err_code == E_QE0015_NO_MORE_ROWS)
		    {
			if (act->ahd_next != NULL)
			{
			    /* more actions to execute, use hold-and-
			    ** execute strategy: set the HOLD state 
			    ** and count, continue to execute until
			    ** end of query at which time no-more-rows 
			    ** is returned */

			    qss_p->qes_q1_qry_status |= QES_04Q_HOLD;
			    qss_p->qes_q4_hold_cnt = qef_rcb->qef_rowcount;
		    
			    /* no-more-rows code set to fall thru 
			    ** to execute next action */
			}
		    }
		    else
			qed_u10_trap();	/* trap the error */
		}
	    }

	    /* If error other than NO_MORE_ROWS, terminate the query.  If 
	    ** NO_MORE_ROWS, fall through to process the next action.
	    */
	    if (qef_rcb->error.err_code != E_QE0015_NO_MORE_ROWS)
	    {
		qef_error(qef_rcb->error.err_code, 0L, status, &err,
		      &qef_rcb->error, 0);
		qef_cb->qef_open_count--;
		return (qeq_cleanup(qef_rcb, status, TRUE));
	    }
	}				    
	else if (act && act->ahd_atype == QEA_D10_REGPROC)
	{
	    /* The requtest to execute a registered procedure has already been
	    ** sent to the LDBMS, and a TDESC has been returned.  Now set
	    ** up to call RQF again for a the byref parameters.
	    **
	    ** NOTE: if there is no byref parameter passing, then QEF still 
	    **	     needs to call RQF to read the final GCA_RESPONSE block 
	    **	     which is part of the 6.5 IPROC protocol.
	    */
	    if (qss_p->qes_q1_qry_status & QES_02Q_EOD)
	    {
		/* end of query plan: destroy DSH and return no-more-rows  --
		** this case is not expected, but may be possible with large
		** TDESCs (occurs when there are lots of byref parameters).
		*/

		dds_p->qes_d7_ses_state = QES_0ST_NONE;	/* reset */
		qef_rcb->qef_rowcount = 0;

		/* must return E_QE0015_NO_MORE_ROWS to signal exhaustion 
		** of data 
		*/
		qef_rcb->error.err_code = E_QE0015_NO_MORE_ROWS;
		status = E_DB_WARN;
		/* fall thru to destroy the DSH */
	    }
	    else
	    {	    
		/* continue executing REGPROC action */

		status = qeq_d12_regproc(qef_rcb, act);

		if (status == E_DB_OK)
		{
		    dsh->dsh_qp_status &= ~DSH_IN_USE;
		    return(E_DB_OK);	/* return data and save the DSH */
		}
		else 
		{
		    if (status == E_DB_WARN 
			&& 
			qef_rcb->error.err_code == E_QE0015_NO_MORE_ROWS)
		    {
			if (act->ahd_next != NULL)
			{
			    /* more actions to execute, use hold-and-
			    ** execute strategy: set the HOLD state 
			    ** and count, continue to execute until
			    ** end of query at which time no-more-rows 
			    ** is returned */

			    qss_p->qes_q1_qry_status |= QES_04Q_HOLD;
			    qss_p->qes_q4_hold_cnt = qef_rcb->qef_rowcount;
		    
			    /* no-more-rows code set to fall thru 
			    ** to execute next action */
			}
		    }
		    else
			qed_u10_trap();	/* trap the error */
		}
	    }

	    /* If error other than NO_MORE_ROWS, terminate the query.  If 
	    ** NO_MORE_ROWS, fall through to process the next action.
	    */
	    if (qef_rcb->error.err_code != E_QE0015_NO_MORE_ROWS)
	    {
		qef_error(qef_rcb->error.err_code, 0L, status, &err,
		      &qef_rcb->error, 0);
		qef_cb->qef_open_count--;
		return (qeq_cleanup(qef_rcb, status, TRUE));
	    }
	}

	/* Get an action.  If no action executed yet, get the first one
	** from QP, otherwise get the next action */

	cbs = dsh->dsh_cbs;	/* ptr to control block pointers array */
	if (!act)
	{
	    act = first_act;
	    if (ddb_b)
	    {
		if (val_qp_51)
		{
		    /* validate that query plan has right number of actions */

		    int	    act_cnt;
		    QEF_AHD	    *act_p; 

		    for (act_cnt = 0, act_p = first_act; 
			act_p != (QEF_AHD *) NULL;
			++act_cnt)
			act_p = act_p->ahd_next;

		    if (act_cnt != qp->qp_ahd_cnt)
		    {
			status = qed_u2_set_interr(
					E_QE1002_BAD_ACTION_COUNT, 
				    & qef_rcb->error);
			return(qeq_cleanup(qef_rcb, (DB_STATUS) status, 
				(bool) TRUE));
		    }
		}
		/* action count ok */

		if (log_qry_55)
		    qeq_p10_prt_qp(qef_rcb, dsh);

		if (dsh->dsh_qp_ptr->qp_qmode == QEQP_01QM_RETRIEVE)
		    dds_p->qes_d9_ctl_info |= QES_03CTL_READING_TUPLES;
						/* must set for qef_call */
		else if (dsh->dsh_qp_ptr->qp_qmode == QEQP_03QM_RETINTO)
		{
		    /* lock to synchronize */

		    status = qel_u3_lock_objbase(qef_rcb);
		    if (status)
			return(qeq_cleanup(qef_rcb, (DB_STATUS) status, 
			        (bool) TRUE));
		}
		else
		    dds_p->qes_d9_ctl_info &= ~QES_03CTL_READING_TUPLES;

		qss_p->qes_q1_qry_status = QES_00Q_NIL; 
						/* initialize query status */
		qss_p->qes_q5_ldb_rowcnt = 0;	/* initialize LDB return count
						*/

		/* call TPF to validate the LDBs in a write query */

		if (dsh->dsh_qp_ptr->qp_qmode != QEQP_01QM_RETRIEVE)
		{
		    bool	ok;


		    status = qet_t9_ok_w_ldbs(dsh, qef_rcb, & ok);
		    if (status)
			return(qeq_cleanup(qef_rcb, (DB_STATUS) status, 
			        (bool) TRUE));
		    else
		    {
			if (! ok)
			{
			    status = qed_u2_set_interr(
				    E_QE0990_2PC_BAD_LDB_MIX,
				    & qef_rcb->error);
			    return(qeq_cleanup(qef_rcb, (DB_STATUS) status, 
					(bool) TRUE));
			}	
		    }
		}
	    }
	}
	else
	    act = act->ahd_next;

	/* Reset the rule and context nesting depth on new queries */

	qef_rcb->qef_rule_depth = 0;
	if ((qef_rcb->qef_stmt_type & QEF_GCA1_INVPROC) == 0)
	    qef_rcb->qef_context_cnt = 0;
	else
	    qef_rcb->qef_context_cnt = 1;
    }

    /*
    ** For all actions.
    */
    while (act != (QEF_AHD *)NULL)
    {
	qeq_rcbtodsh(qef_rcb, dsh);
	status = E_DB_OK;
	if (qef_rcb->qef_intstate & QEF_CLEAN_RSRC)
	    status = E_DB_ERROR;

	if (status == E_DB_OK && ddb_b)
	{
	    /* Validate the action.  This checks legal action types for
	    ** distributed.  Must modify when more action types become legal
	    ** for distributed */
	    status = qeq_d0_val_act(qef_rcb, act);
	    if (status)
	    {
		if (log_err_59)
		    qeq_d22_err_qp(qef_rcb, dsh);
		qed_u10_trap();
	    }
	}

	/* 
	** Validate (and initialize) the action if this is the first
	** invocation of this action. If we are continuing action execution
	** after processing a rule action list, do not validate.
	** act_state is constrained to be DSH_CT_INITIAL only on the first
	** call to the action. If we are executing a for (select) loop in
	** a procedure, we only validate the select GET on entry to the
	** loop (so as not to reposition the query for each loop iteration).
	*/
	else if (status == E_DB_OK && act_state == DSH_CT_INITIAL &&
	    (prevfor || act->ahd_atype != QEA_GET || 
		!(act->qhd_obj.qhd_qep.ahd_qepflag & AHD_FORGET)) )
	{
	    status = qeq_topact_validate(qef_rcb, dsh, act, TRUE);
	    if (status == E_DB_OK)
		status = qeq_subplan_init(qef_rcb, dsh, act, NULL, NULL);
	    if (status != E_DB_OK)
	    {
		/*
		** If no more rows (because of constant qual that evaluated to
		** FALSE) and the action isn't GET or is GET and we're inside
		** a DBP then continue with next action.  If the no more rows
		** and GET and we're not in a DBP then stop the loop - we know
		** the query will fail.  Also handle invalid queries & other
		** errors reported in qeq_validate.
		**
		** Note: that this does not handle UNION SELECT cases outside
		** of DBP's where one of the SELECT qualifications is a constant
		** qualification that yields FALSE.
		*/

		qeq_dshtorcb(qef_rcb, dsh);
		if (qef_rcb->error.err_code == E_QE0119_LOAD_QP ||
		    qef_rcb->error.err_code == E_QE030F_LOAD_TPROC_QP)
		{
		    /* The QP must be compiled, return to SCF */
		    return (status);
		}

		if (qef_rcb->error.err_code == E_QE1004_TABLE_NOT_OPENED)
		{
		    /*
		    ** B103150
		    ** validate succeeded but table could not be opened 
		    ** with the required lockmode because of lock timeout
		    ** In this case we want the action to fail, not the validate
		    ** so that we will abort the procedure and continue processing
		    ** the next action. (B103150)
		    */
		    qef_rcb->error.err_code = E_DM004D_LOCK_TIMER_EXPIRED;
		}

		else if (qef_rcb->error.err_code == E_QE0015_NO_MORE_ROWS)
		{
		    if (act->ahd_atype != QEA_GET || qp->qp_qmode == QEQP_EXEDBP)
		    {
			qef_rcb->error.err_code = E_QE0000_OK;
			status = E_DB_OK;
			act = act->ahd_next;
			prevfor = FALSE;
			if (iirowcount != (i4 *)NULL)
			    *iirowcount = (i4) (qef_rcb->qef_rowcount);
			continue;
		    }
		    else	/* Action type must be GET and we're not in a DBP */
		    {
			/* Leave warning status and error code alone & stop loop */
			break;
		    }
		}
		else if ((qef_rcb->error.err_code == E_QE0023_INVALID_QUERY) ||
			 (qef_rcb->error.err_code == E_QE0301_TBL_SIZE_CHANGE))
		{
		    /* decrement the count of open query/cursor so that
		    ** qeq_cleanup can do the right thing */
		    if (qef_cb->qef_open_count > 0)
			--qef_cb->qef_open_count;
		    if ((tmp_dsh = dsh->dsh_stack) != NULL)
		    {
			qef_rcb->qef_context_cnt--;
			qef_rcb->qef_intstate |= QEF_DBPROC_QP;
		    }
		    status = qeq_cleanup(qef_rcb, status, TRUE);
		    qef_cb->qef_dsh = (PTR)tmp_dsh;
		    return (status);
		}
		else	/* Some other error - abort QP */
		{
		    qef_error(qef_rcb->error.err_code, 0L, status, &err,
			      &qef_rcb->error, 0);

		    /* We must do cleanup for any errors that happen during 
		    ** initialization.  This includes backing out transactions
		    ** in the case of deadlock.  So drop through to error
		    ** handling following the switch below. (bug #32320)
		    */
		}
	    }
	} /* if validate needed */

	/* The following effectively resets the row count for every action. */
	/* If we are processing a rule action list, we want to remember	    */
	/* the current row count. Since the rule action lists are	    */
	/* one level deep and cannot use DML statements, we just skip	    */
	/* resetting the row count on rule actions. */
	/*
	** We also don't want to reset the rowcount if the compiler told
	** us to remember the rowcount over the creation of a NOT NULL
	** integrity.  For more detail, see the CREATE_INTEGRITY case below.
	** And if this is a row producing procedure, qef_rowcount indicates
	** how much space is left in the fetch buffer.
	*/

	if ( act->ahd_atype == QEA_CREATE_INTEGRITY )
	{  integrityDetails = &act->qhd_obj.qhd_createIntegrity; }

	if (dsh->dsh_depth_act == 0 && act_state == DSH_CT_INITIAL &&
	    act->ahd_atype != QEA_INVOKE_RULE &&
	    ( act->ahd_atype != QEA_CREATE_INTEGRITY ||
	      ( (integrityDetails->qci_flags & QCI_SAVE_ROWCOUNT) == 0 )) 
	)
		dsh->dsh_qef_rowcount = rowcount;

	/* Don't allow for-loop get's to restart query on each iteration. */
	prevreset = act_reset;
	if (!prevfor && act->ahd_atype == QEA_GET &&
		act->qhd_obj.qhd_qep.ahd_qepflag & AHD_FORGET)
	{
	    act_reset = FALSE;
	}
	prevfor = FALSE;		/* reset the FOR flag */
	func = NO_FUNC;
	if (act_reset)
	    func = FUNC_RESET;		/* Compute oft-used function */

	if (status == E_DB_OK)
	{
	    /* initialization was successful */

	    QED_DDL_INFO    sav_ddl;
	    bool	    dshcopy = TRUE;

	    switch (act->ahd_atype)
	    {
	    case QEA_DMU:
		status = qea_dmu(act, qef_rcb, dsh);
		break;

	    case QEA_APP:
	    case QEA_LOAD:
		status = qea_append(act, qef_rcb, dsh, func, act_state);
		break;

	    case QEA_SAGG:
		status = qea_sagg(act, qef_rcb, dsh, func);
		break;

	    case QEA_UPD:
		status = qea_replace(act, qef_rcb, dsh, func, act_state);
		break;

	    case QEA_RUP:
	    case QEA_D9_UPD:
		status = qea_rupdate(act, qef_rcb, dsh, dsh->dsh_aqp_dsh,
			func, act_state);
		break;

	    case QEA_RDEL:
		status = qea_rdelete(act, qef_rcb, dsh, dsh->dsh_aqp_dsh,
			func, act_state);
		if ((status == E_DB_OK) && (ddb_b))
		{
		    dsh->dsh_qp_status &= ~DSH_IN_USE;
		    return (status);
		}
		break;
		
	    case QEA_DEL:
		status = qea_delete(act, qef_rcb, dsh,
			func, act_state);
		break;

	    case QEA_FOR:
		prevfor = TRUE; 
		dsh->dsh_qp_status |= DSH_FORINPROC;
		break;

	    case QEA_GET:
		status = qea_fetch(act, qef_rcb, dsh, func);
		act_reset = prevreset;
		qeq_dshtorcb(qef_rcb, dsh);
		dshcopy = FALSE;
		if (	((status == E_DB_OK)
			    || ((status == E_DB_INFO) &&
				    (qef_rcb->error.err_code
					== E_AD0002_INCOMPLETE)))
		     && qp->qp_qmode != QEQP_EXEDBP)
		{
		    /*
		    **	If the query worked -- which can be either because it is
		    **	completely done, or because it is partially done and
		    **	needs to be called back -- situation is the same in
		    **	either case...
		    **
		    ** Mark the DSH as available for the DSh search
		    ** routine. When the fetch next returns, we must find
		    ** this DSH.
		    */

		    /*
		    ** NOTE THAT THIS IS A POTENTIAL BUG WHEN A SINGLE SESSION
		    ** CAN OPEN A QUERY PLAN (SHARED) MORE THAN ONCE.  IN THAT
		    ** CASE, THE DSH SEARCH ROUTINE CANNOT DISTINGUISH ONE
		    ** INVOCATION OF A QUERY FROM ANOTHER.
		    **
		    ** (fred)
		    **
		    ** POTENTIAL BUGS:
                    **      - in recursive procedures.
                    **      - in repeat cursors.	
		    */
		    dsh->dsh_qp_status &= ~DSH_IN_USE;
		    return (status);
		}
		break;

	    case QEA_RETROW:
		status = qea_retrow(act, qef_rcb, dsh, func);
		qeq_dshtorcb(qef_rcb, dsh);
		dshcopy = FALSE;
		if ((status == E_DB_INFO &&
			qef_rcb->error.err_code == E_AD0002_INCOMPLETE)
		  || dsh->dsh_rpp_rows >= dsh->dsh_rpp_rowmax)
		{
		    /*
		    **	If the query worked -- which can be either because it is
		    **	completely done, or because it is partially done and
		    **	needs to be called back -- situation is the same in
		    **	either case...
		    **
		    ** Mark the DSH as available for the DSh search
		    ** routine. When the fetch next returns, we must find
		    ** this DSH.
		    */

		    /*
		    ** NOTE THAT THIS IS A POTENTIAL BUG WHEN A SINGLE SESSION
		    ** CAN OPEN A QUERY PLAN (SHARED) MORE THAN ONCE.  IN THAT
		    ** CASE, THE DSH SEARCH ROUTINE CANNOT DISTINGUISH ONE
		    ** INVOCATION OF A QUERY FROM ANOTHER.
		    **
		    ** (fred)
		    **
		    ** POTENTIAL BUGS:
                    **      - in recursive procedures.
                    **      - in repeat cursors.	
		    */
		    dsh->dsh_qp_status &= ~DSH_IN_USE;

                    /* If this was row-producing procedure restore the
                    ** rowcount from the DSH. 
                    */
                    if (qp != NULL && (qp->qp_status & QEQP_ROWPROC))
                    {
			qef_rcb->qef_count = dsh->dsh_rpp_bufs;
			qef_rcb->qef_rowcount = qef_rcb->qef_targcount =
					dsh->dsh_rpp_rows;
                    }

		    return (status);
		}
		break;

	    case QEA_PROJ:
		status = qea_proj(act, qef_rcb, dsh, func);
		break;

	    case QEA_AGGF:
		status = qea_aggf(act, qef_rcb, dsh, func);
		break;

	    case QEA_PUPD:
		status = qea_preplace(act, qef_rcb, dsh, func, act_state);
		break;

	    case QEA_PDEL:
		status = qea_pdelete(act, qef_rcb, dsh, func, act_state);
		break;

	    case QEA_COMMIT:
		status = qea_commit(act, qef_rcb, dsh, NO_FUNC, act_state);
		break;

	    case QEA_ROLLBACK:
		status = qea_rollback(act, qef_rcb, dsh, NO_FUNC, act_state);
		break;

	    case QEA_IF:
		/* Evaluate the condition expression */
		qen_adf = act->qhd_obj.qhd_if.ahd_condition;
		if (qen_adf != NULL)
		{
		    ade_excb = (ADE_EXCB*) cbs[qen_adf->qen_pos];
		    status = qen_execute_cx(dsh, ade_excb);
		    if (status != E_DB_OK)
			break;
		    if (ade_excb->excb_value == ADE_TRUE)
			act = act->qhd_obj.qhd_if.ahd_true;
		    else
			act = act->qhd_obj.qhd_if.ahd_false;

		    if (qef_rcb->qef_rule_depth <= 0)  /* if not in rule proc */
		    {
   			if (saved_act == (QEF_AHD *)NULL)
   			    saved_act = act;  
			/* else we've already saved a QEA_IF branch, 
			** assume this is an inner loop */
		    }


		    /* if there are more actions, execute them */
		    if (act != NULL)
		    {
			qeq_dshtorcb(qef_rcb, dsh);
		        continue;
		    }
		}
		break;

	    case QEA_RETURN:
	      {
		i4	t1 = 0, t2 = 0;	/* Dummy trace values */
		i4	was_return;
		QEE_DSH	*caller_dsh = dsh->dsh_stack;

                /* need to restore the mode because this routine will be
                ** exited to allocate memory to return the byref value.
                */
		qef_rcb->qef_mode = mode;

		/* Copy OUT, INOUT parm values back to caller. */
		if (caller_dsh && caller_dsh->dsh_act_ptr &&
			caller_dsh->dsh_act_ptr->ahd_atype == QEA_CALLPROC)
		    status = qee_return_dbparam(qef_rcb, dsh, caller_dsh,
		      caller_dsh->dsh_act_ptr->qhd_obj.qhd_callproc.ahd_params,
		      caller_dsh->dsh_act_ptr->qhd_obj.qhd_callproc.ahd_pcount);

		/* PSF confirms all procedures end with a RETURN */
		qeq_dshtorcb(qef_rcb, dsh);
		dshcopy = FALSE;
		qef_rcb->qef_rowcount = -1;

		status = qee_update_byref_params(qef_rcb, dsh);
		if (status)
		{
		    if (qef_rcb->error.err_code ==
			E_QE0306_ALLOCATE_BYREF_TUPLE)
		    {
			STRUCT_ASSIGN_MACRO(*qef_rcb, *(dsh->dsh_saved_rcb));
			dsh->dsh_act_ptr = act;
			return(status);
		    }
		    else
		    {
			break;
		    }
		}
		qen_adf = act->qhd_obj.qhd_return.ahd_rtn;
		if (qen_adf != NULL)	/* Check if there is a return value */
		{			 /* Yes, */
		    ade_excb = (ADE_EXCB*) cbs[qen_adf->qen_pos];
		    if (qp->qp_status & QEQP_GLOBAL_BASEARRAY)
			dsh->dsh_row[qen_adf->qen_uoutput] = 
					    (PTR)(&qef_rcb->qef_dbp_status);
		    else ade_excb->excb_bases[ADE_ZBASE + qen_adf->qen_uoutput] = 
					    (PTR)(&qef_rcb->qef_dbp_status);
		    status = qen_execute_cx(dsh, ade_excb);
		    was_return = 1;
		}
		else
		{			/* No user-specified return value */
		    qef_rcb->qef_dbp_status = 0L;
		    was_return = 0;
		}
		if (ult_check_macro(&qef_cb->qef_trace, QEF_T_RETURN, &t1, &t2))
		{
		    TRformat(qeq_tr_callback, (i4 *)0,
				cbuf, cbufsize,
				was_return ? "RETURN VALUE = %d" : "RETURN",
				qef_rcb->qef_dbp_status);
        	    qec_tprintf(qef_rcb, cbufsize, cbuf);
		}
	      }
	      break;

	    case QEA_MESSAGE:
		status = qea_message(act, qef_rcb);
		break;

	    case QEA_EMESSAGE:
		status = qea_emessage(act, qef_rcb);
		break;

	    case QEA_IPROC:	/* internal qef procedure */
	    {
		status = qea_iproc(act, qef_rcb);
		break;
	    }
	    case QEA_CALLPROC:
		qef_rcb->qef_mode = mode;
		/* Make sure RESET flag is on when call a procedure */
		act_reset = TRUE;
		/* Not fired from rule: reset rowcount for normal restoration */
		if (!(dsh->dsh_qp_status & DSH_FORINPROC)
			 && dsh->dsh_depth_act == 0)
		{
		    dsh->dsh_qef_rowcount = -1;
		    qef_rcb->qef_rowcount = -1;
		}
		status = qea_callproc(act, qef_rcb, dsh,
			NO_FUNC, act_state);
		qeq_dshtorcb(qef_rcb, dsh);
		dshcopy = FALSE;
		if (status == E_DB_OK)
		{
		    if (dsh == (QEE_DSH *)qef_cb->qef_dsh)
			break; /* procedure call was no-op */
		    /* We have successfully called a new QP.		    */
                    /* Check if the procedure supports a constraint and     */
                    /* set the bit flag in the QEF_RCB accordingly.         */
		    if ( act->qhd_obj.qhd_callproc.ahd_flags & QEF_CP_CONSTRAINT )
                        qef_rcb->qef_intstate |= QEF_CONSTR_PROC;
                    else 
                        qef_rcb->qef_intstate &= ~QEF_CONSTR_PROC;

		    /* Set up the local variables required for processing   */
		    /* and continue with the first action of the new QP. */
	            qeq_setup(qef_cb, &dsh, &qp, &first_act, &act, &cbs, 
				&iirowcount, &iierrorno);
                    /* b89898 - act is now 1st action of procedure */
                    /* If the 1st action is to call another dbproc */
                    /* load initialised parameter values into DSH  */
                    if((act != NULL) &&
                       (act->ahd_atype == QEA_CALLPROC) &&
                       (act->qhd_obj.qhd_callproc.ahd_params))
                      {
                       cp_param = act->qhd_obj.qhd_callproc.ahd_params;
                       for(i=0;i<act->qhd_obj.qhd_callproc.ahd_pcount;i++)
                         {
                          ch = (char *)dsh->dsh_row[cp_param->ahd_parm_row]
                                  + cp_param->ahd_parm_offset;
			  if (!(cp_param->ahd_parm.parm_flags & QEF_IS_BYREF))
			    break;
                          /* Double check that the DSH has not had values */
                          /* assigned from invocation of caller. Not very */
                          /* tidy but unavoidable.                        */
                          for(k=0;k<cp_param->ahd_parm.parm_dbv.db_length;k++)
                            {
                             if(*ch != '\0')
                               break;
                             ch++;
                             if((k==(cp_param->ahd_parm.parm_dbv.db_length-1))&&
                                (cp_param->ahd_parm.parm_dbv.db_data))
                               {
                                MEcopy((char *)cp_param->ahd_parm.parm_dbv.db_data,
                                    cp_param->ahd_parm.parm_dbv.db_length,
                                    (char *)dsh->dsh_row[cp_param->ahd_parm_row]
                                            + cp_param->ahd_parm_offset);
                               }
                            }
                          cp_param++;
                         }
                      }
		    continue;
		}
		else if (qef_rcb->error.err_code == E_QE0119_LOAD_QP ||
                         qef_rcb->error.err_code == E_QE030F_LOAD_TPROC_QP)
		{
		    /* The QP must be compiled, return to SCF */
		    return (status);
		}
		else if (qef_rcb->error.err_code == E_QE0125_RULES_INHIBIT)
		{
		    /*
		    ** By setting act to NULL we behave as though the qual
		    ** that is supposed to fire the rule was not true.
		    */
		    status = E_DB_OK;
		    act = NULL;
		}
		break;
	    		
	    case QEA_PROC_INSERT:	/* append row for deferred proc call */
	    {
		status = qea_prc_insert(act, dsh, NO_FUNC, act_state);
		break;
	    }

	    case QEA_CLOSETEMP:	/* close temporary table after deferred
				** proc call */
	    {
		status = qea_closetemp(act, dsh, NO_FUNC, act_state);
		break;
	    }

	    case QEA_EVRAISE:
	    case QEA_EVREGISTER:
	    case QEA_EVDEREG:
		status = qea_event(act, qef_rcb, dsh);
		break;

	    case QEA_CREATE_SCHEMA:
		qea_schname = &act->qhd_obj.qhd_schema.qef_schname;
                status = qea_schema(qef_rcb, qeuq_cb, qef_cb, qea_schname, flag);
		dshcopy = FALSE;
		dsh->dsh_qef_rowcount = -1;
		qef_rcb->qef_rowcount = -1;
                break;

            case QEA_EXEC_IMM:
                /* need to restore the mode beause this routine will be
                ** exited to parse the text of QEA_EXEC_IMM actions.
                */
               qef_rcb->qef_mode = mode;

               text = act->qhd_obj.qhd_execImm.ahd_text;
               qef_info = act->qhd_obj.qhd_execImm.ahd_info;

               status = qea_xcimm(act, qef_rcb, dsh, text, qef_info);

	       dsh->dsh_qef_rowcount = -1;

               /* In case of CREATE TABLE/ALTER TABLE/CREATE VIEW in CREATE 
	       ** SCHEMA, we want to continue with the next action i.e., 
               ** QEA_CREATE_TABLE, QEA_CREATE_INTEGRITY or QEA_CREATE_VIEW
	       ** after returning.
	       */	
		qeq_dshtorcb(qef_rcb, dsh);
		dshcopy = FALSE;
               if (status == E_DB_OK)
               {
		  if (qef_rcb->error.err_code != E_QE0303_PARSE_EXECIMM_QTEXT)
		  {
		     /* This check will not be needed once the GRANT statement
                     ** is also routed like DML statement through the server.
                     */

                     if (qef_info->pst_basemode == PSQ_GRANT)
                        break;
	
	             qeq_setup(qef_cb, &dsh, &qp, &first_act, &act, &cbs, 
				&iirowcount, &iierrorno);

	             /* We have set up the context for the new QP, continue
	             ** with the first action of the new QP
	             */	
                     continue;
		  }
   	          else 
		  {
	             /* The qtext for the action needs to be parsed. */
                     /* return to SCF */
		     status = E_DB_ERROR;
		     return(status); 
	 	  }
		}	

               break;

	    /* beginning of DDL statements */

	    case QEA_CREATE_TABLE:
		{
		i4	caller;
		DB_TAB_ID table_id;

		/* This is the standalone create table.  CREATE AS SELECT
		** uses a DMU action header because in that case, the
		** create has to play nice with the rest of the statement.
		*/

		QEU_CB *qeu_cb =
			(QEU_CB *) act->qhd_obj.qhd_DDLstatement.ahd_qeuCB;
			
		caller = (qeu_cb->qeu_d_op ==
			  DMU_ALTER_TABLE ? QEF_EXTERNAL : QEF_INTERNAL);

		status = qeu_dbu( qef_rcb->qef_cb,
			( QEU_CB * ) act->qhd_obj.qhd_DDLstatement.ahd_qeuCB,
			1, &table_id,
			&qef_rcb->qef_rowcount, &qef_rcb->error, caller );

		dshcopy = FALSE;
                qef_rcb->qef_rowcount = -1; /* this eliminates the spurious
                                            ** "(0 rows)" diagnostic. */

		/* This bit is rather bogus, but maybe we'll use it someday.
		** If there are other create table actions (there aren't),
		** they might want to know the new table ID.
		** This would be the master ID if partitioning.
		*/
		if (act->ahd_ID != QEA_UNDEFINED_OBJECT_ID)
		{
		    STRUCT_ASSIGN_MACRO(table_id,
			*(DB_TAB_ID *) &dsh->dsh_row[act->ahd_ID]);
		}

                if (  qef_rcb->error.err_code  == E_QE0025_USER_ERROR )
                {
                    qef_rcb->error.err_code = E_QE0122_ALREADY_REPORTED;
                    break;
                }
		
		if (  qef_rcb->error.err_code  == E_DM006A_TRAN_ACCESS_CONFLICT )
			break;
		if (  qef_rcb->error.err_code  == E_QE5424_INVALID_SRID )
			break;

		qef_rcb->error.err_code=qeu_cb->error.err_code;

		}
		break;
	    case QEA_RENAME_STATEMENT:
	    {
		i4	caller;
		DB_TAB_ID table_id;

		QEU_CB *qeu_cb =
			(QEU_CB *) act->qhd_obj.qhd_DDLstatement.ahd_qeuCB;

    		caller = (qeu_cb->qeu_d_op ==
    			  DMU_ALTER_TABLE ? QEF_EXTERNAL : QEF_INTERNAL);
	
		/* Execute the rename operation now */
    		status = qeu_dbu( qef_rcb->qef_cb,
    			( QEU_CB * ) act->qhd_obj.qhd_rename.qrnm_ahd_qeuCB,
    			1, &table_id,
    			&qef_rcb->qef_rowcount, &qef_rcb->error, caller );
    
    		dshcopy = FALSE;
		dsh->dsh_qef_rowcount = -1;
                qef_rcb->qef_rowcount = -1; /* this eliminates the spurious
                                            ** "(0 rows)" diagnostic. */
    
		if (  qef_rcb->error.err_code  == E_QE0025_USER_ERROR )
		{
		    qef_rcb->error.err_code = E_QE0122_ALREADY_REPORTED;
		    break;
		}
    		
		if (  qef_rcb->error.err_code  == E_DM006A_TRAN_ACCESS_CONFLICT )
		    break;
    
		qef_rcb->error.err_code=qeu_cb->error.err_code;

    	        /* Now process the dependent objects (grants)
     		** on the table, if we were successful. 
		*/
		if (status == E_DB_OK)
		{
		    status = qea_renameExecute( act, qef_rcb, dsh );
		    qeq_dshtorcb(qef_rcb, dsh);
		}
	    }
	    break;

	    case QEA_CREATE_INTEGRITY:
                /* need to restore the mode beause this routine may be
                ** exited to parse the text of QEA_EXEC_IMM actions.
                */
                qef_rcb->qef_mode = mode;
		status = qea_createIntegrity( act, qef_rcb, dsh );
		qeq_dshtorcb(qef_rcb, dsh);
		dshcopy = FALSE;

		/*
		** Normally, we want to set qef_rowcount to -1 in order to
		** suppress the spurious "(0 rows)" diagnostic.  However,
		** it may be that we are executing a CREATE TABLE AS SELECT
		** statement.  In this case, NOT NULL integrities may follow
		** the SELECT action.  We will want to hold in
		** our craw the rowcount meticulously accumulated by the
		** SELECT so we can cough it up at statement end.
		*/
		integrityDetails = &act->qhd_obj.qhd_createIntegrity;
		if ( !( integrityDetails->qci_flags & QCI_SAVE_ROWCOUNT ) )
		{   qef_rcb->qef_rowcount = -1; }

	        if (status == E_DB_OK) 
		{
		   if (qef_rcb->error.err_code ==
			 E_QE0303_PARSE_EXECIMM_QTEXT)
                   {
                      status = E_DB_ERROR;
                      return(status);
                   }
		   /* In case of SELECT ANY(1) statement, we want to set up the
		   ** the context for the new QP and continue with 
		   ** the first action of the new QP (similar to 
		   ** QEA_CREATE_TABLE/QEA_CREATE_VIEW in CREATE SCHEMA).
		   */
		   else if (qef_rcb->qef_cb->qef_callbackStatus &
				QEF_EXECUTE_SELECT_STMT)
		   { 
		      qeq_setup(qef_cb, &dsh, &qp, &first_act, &act, &cbs, 
				&iirowcount, &iierrorno);
		      
		      /* We have set up the context for the new QP, 
	              ** continue with the first action of the new QP
	              */
                      continue;
		   }
		}
		break;

	    case QEA_CREATE_VIEW:
	    {
                /* need to restore the mode beause this routine may be
                ** exited to parse the text of QEA_EXEC_IMM actions.
                */
                qef_rcb->qef_mode = mode;
		status = qea_cview(act, qef_rcb, dsh);
		qeq_dshtorcb(qef_rcb, dsh);
		dshcopy = FALSE;

		qef_rcb->qef_rowcount = -1; /* this eliminates the spurious
					    ** "(0 rows)" diagnostic. */
		if (status == E_DB_OK && 
			qef_rcb->error.err_code == E_QE0303_PARSE_EXECIMM_QTEXT)
                {     
                   /* EXECUTE IMMDIATE magic */
                   status = E_DB_ERROR;
                   return(status);
                }

		break;
	    }

	    case QEA_DROP_INTEGRITY:
		status = qea_d_integ(act, qef_rcb);

		dsh->dsh_qef_rowcount = -1; /* this eliminates the spurious
					    ** "(0 rows)" diagnostic. */
		break;

	    /* currently unsupported DDL statements */

	    case QEA_CREATE_RULE:
	    case QEA_CREATE_PROCEDURE:
	    case QEA_MODIFY_TABLE:
		dshcopy = FALSE;
		qef_rcb->error.err_code = E_QE1996_BAD_ACTION;
		status = E_DB_ERROR;
		qef_error( qef_rcb->error.err_code, 0L, status, &err,
			&qef_rcb->error, 0);
		break;

	    /* beginning of distributed actions */

	    case QEA_D1_QRY:
	    case QEA_D5_DEF:
	    case QEA_D8_CRE:
		dshcopy = FALSE;
		dsh->dsh_act_ptr = act;		/* promote to current action */

		if (dsh->dsh_qp_ptr->qp_status & QEQP_RPT)	
		    status = qeq_d7_rpt_act(qef_rcb, & act);
		else
		    status = qeq_d1_qry_act(qef_rcb, & act);

		if (status == E_DB_OK)
		{
		    dsh->dsh_qp_status &= ~DSH_IN_USE;
		    if (dds_p->qes_d7_ses_state == QES_1ST_READ)
		    {
			/* indicate the DSH is already built before returning */
			qss_p->qes_q1_qry_status |= QES_05Q_DSH;
			return(E_DB_OK);	/* return here to save DSH */
		    }
		}
		else if (status == E_DB_WARN 
			 &&
			 (qef_rcb->error.err_code == E_QE0015_NO_MORE_ROWS))
		{
		    /* handle end-of-data and more actions condition */

		    if (act->ahd_next != NULL)
		    {
			/* more actions to execute, use hold-and-
			** execute strategy: set the HOLD state 
			** and count, continue to execute until
			** end of query at which time no-more-rows 
			** is returned */

			qss_p->qes_q1_qry_status |= QES_04Q_HOLD;
			qss_p->qes_q4_hold_cnt = qef_rcb->qef_rowcount;

			/* set clean status to continue to execute actions */

			status = E_DB_OK;
			break;
		    }

		    /* otherwise fall thru with E_DB_WARN to destroy the DSH */
		}
		else
		{
		    if (log_err_59)
			qeq_d22_err_qp(qef_rcb, dsh);
		    qed_u10_trap();
		}
		/* fall thru to destroy the DSH for: 1) error or 2) query 
		** completion */

		dsh->dsh_act_ptr = NULL;
		dds_p->qes_d7_ses_state = QES_0ST_NONE;
						/* reset session state */
		break;

	    case QEA_D3_XFR:

		dshcopy = FALSE;
		dsh->dsh_act_ptr = act;		/* promote to current dsh */
		status = qeq_d5_xfr_act(qef_rcb, act);
		if (status)
		{
		    if (log_err_59)
			qeq_d22_err_qp(qef_rcb, dsh);
		    qed_u10_trap();
		}
		break;

	    case QEA_D4_LNK:			/* CREATE LINK action for a
						** CREATE TABLE as SUBSELECT
						** or RETRIEVE INTO statement */
		dshcopy = FALSE;
		qss_p->qes_q1_qry_status |= QES_03Q_PHASE2;
						/* enter phase 2 */
		STRUCT_ASSIGN_MACRO(qef_rcb->qef_r3_ddb_req.qer_d7_ddl_info, 
			sav_ddl);
						/* save */
		STRUCT_ASSIGN_MACRO(
		    *act->qhd_obj.qhd_d4_lnk.qeq_l1_lnk_p,
		    qef_rcb->qef_r3_ddb_req.qer_d7_ddl_info);
						/* link information */
		status = qel_c0_cre_lnk(qef_rcb);
		if (status)
		{
		    if (log_err_59)
			qeq_d22_err_qp(qef_rcb, dsh);
		    qed_u10_trap();
		}
		STRUCT_ASSIGN_MACRO(sav_ddl, 
		    qef_rcb->qef_r3_ddb_req.qer_d7_ddl_info);
						/* restore */
		break;

	    case QEA_D6_AGG:

		dshcopy = FALSE;
		status = qeq_d10_agg_act(qef_rcb, act);
		if (status)
		{
		    if (log_err_59)
			qeq_d22_err_qp(qef_rcb, dsh);
		    qed_u10_trap();
		}
		break;

            case QEA_D7_OPN:
		dsh->dsh_act_ptr = act;		/* must keep track */

		dshcopy = FALSE;
                status = qeq_d8_def_act(qef_rcb, act);
		if (status)
		{
		    if (log_err_59)
			qeq_d22_err_qp(qef_rcb, dsh);
		    qed_u10_trap();
		}
                break;

            case QEA_D10_REGPROC:	/* Execute Registered procedure */
		dsh->dsh_act_ptr = act;		/* must keep track */
		dshcopy = FALSE;
		status = qeq_d11_regproc(qef_rcb, act);
		if (status == E_DB_OK)
		{
		    dsh->dsh_qp_status &= ~DSH_IN_USE;
		    if (dds_p->qes_d7_ses_state == QES_1ST_READ)
		    {
			/* indicate the DSH is already built before returning */
			qss_p->qes_q1_qry_status |= QES_05Q_DSH;
			return(E_DB_OK);	/* return here to save DSH */
		    }
		}
		break;

	    case QEA_INVOKE_RULE:
		dshcopy = FALSE;
		if (iierrorno != (i4 *)NULL)
		{
		    if (*iierrorno)
		    {
			/*
			** A previous action failed. Therefore, do not run
			** the rule, as there is no point to doing so.
			*/
			qef_rcb->error.err_code = *iierrorno;
			status = E_DB_ERROR;
			break;
		    }
		}
		if (act_state == DSH_CT_INITIAL)
		{
		    dsh->dsh_ctx_act[dsh->dsh_depth_act].dsh_ct_ptr = act;
		    dsh->dsh_ctx_act[dsh->dsh_depth_act].dsh_ct_status =
			DSH_CT_CONTINUE;
		    dsh->dsh_depth_act++;
		    dsh->dsh_act_ptr =
			act->qhd_obj.qhd_invoke_rule.ahd_rule_action_list;
		    qef_rcb->error.err_code = E_QE0120_RULE_ACTION_LIST;
		    status = E_DB_ERROR;
		}
		else
		{
		    status = E_DB_OK;
		}
		break;

	    default:
		status = E_DB_ERROR;
		break;
	    }   /* end of switch */

	    if (dshcopy)
		qeq_dshtorcb(qef_rcb, dsh);
	}

	errorDuringAddConstraintSelect = FALSE;

	if (DB_SUCCESS_MACRO(status))
	{
	    if (iierrorno != (i4 *)NULL)
		*iierrorno = (i4)0;
	}
	else if (qef_rcb->qef_cb->qef_callbackStatus & QEF_EXECUTE_SELECT_STMT)
	{   
	    /*
	    ** If an error occurred while executing the SELECT statement
	    ** supporting an ALTER TABLE ADD REFERENTIAL CONSTRAINT,
	    ** then skip the following logic.  We will unwind below.
	    */
	    errorDuringAddConstraintSelect = TRUE; 
	}
	else 
	{
	    /* If the error return is E_QE0120_RULE_ACTION_LIST, the last */
	    /* action has requested the execution of another action list.   */
	    /* Set the action PC and continue */
	    if (qef_rcb->error.err_code == E_QE0120_RULE_ACTION_LIST)
	    {
		qef_rcb->qef_rule_depth++;
		act = dsh->dsh_act_ptr;
		act_state = DSH_CT_INITIAL;
		act_reset = TRUE;
		continue;
	    }
	    err_save = qef_rcb->error.err_code;
	    if (qef_rcb->error.err_code != E_QE0121_USER_RAISE_ERROR)
	    {
		if (qef_rcb->error.err_code != E_QE0122_ALREADY_REPORTED)
		    qef_error(qef_rcb->error.err_code, 0L, status, &err,
			&qef_rcb->error, 0);
		else
		    qef_rcb->error.err_code = E_QE0025_USER_ERROR;
	    }
	    
	    while (qef_rcb->qef_rule_depth > 0)
	    {
		/* Since we are unwinding, set RESET flag to FALSE. */
		act_reset = FALSE;
		/* 
		** We are currently processing rules. The semantics of
		** rules require that the current statement be aborted
		** including all rules processing. Thus, we completely
		** unwind the DSH stack at this time and then fall through
		** to normal outer block processing.
		*/
		if (dsh->dsh_depth_act > 0)
		{
		    /* 
		    ** Abort the rule action list by restoring the old
		    ** pointer. Since the error condition still exists, the
		    ** action triggering this rule will also be aborted as
		    ** we fall through the bottom of the rule abort code.
		    */
		    dsh->dsh_depth_act--;
		    act = dsh->dsh_ctx_act[dsh->dsh_depth_act].dsh_ct_ptr;
		    act_state =
			dsh->dsh_ctx_act[dsh->dsh_depth_act].dsh_ct_status;
		    qef_rcb->qef_rule_depth--;
		}
		else
		{
		    /*
		    ** B21195: save error number before popping stack, so that
		    ** if rule was invoked from within a DBP we can set the
		    ** parent diagnostic variables upon return. (neil)
		    */
		    i4		iierror_save;

		    /* We must be executing in a DB procedure called from a */
		    /* rule action list. Abort the execution of this	    */
		    /* procedure. */

		    /* Save error number that got us here */
		    if (iierrorno != (i4 *)NULL && *iierrorno)
			iierror_save = *iierrorno;
		    else
			iierror_save = (i4)(qef_rcb->error.err_code);
		    tmp_dsh = dsh->dsh_stack;
		    qef_cb->qef_open_count--;
		    qeq_cleanup(qef_rcb, status, TRUE);
		    dsh = tmp_dsh;
		    qef_cb->qef_dsh = (PTR) dsh;
		    qef_rcb->qef_context_cnt--;
		    qef_rcb->qef_qso_handle = dsh->dsh_saved_rcb->qef_qso_handle;
                    qef_rcb->qef_rowcount= dsh->dsh_saved_rcb->qef_rowcount;

		    qeq_restore(dsh->dsh_saved_rcb, qef_rcb, dsh, &mode,
				&act, &rowcount, &qp, &cbs, &iirowcount);

		    first_act = qp->qp_ahd;

		    if (qp->qp_oerrorno_offset == -1)
		    	iierrorno = (i4 *)NULL;
		    else
		    	iierrorno = (i4 *)(dsh->dsh_row[qp->qp_rerrorno_row] + 
			    qp->qp_oerrorno_offset);
		    /*
		    ** Bug fix: This code is only entered to recover from
		    ** errors from under a rule.  Always make sure to reset
		    ** local diagnostics and if we were fired from within
		    ** a DBP then reset local iivars. (neil)
		    */
		    qef_rcb->qef_rowcount = -1;		/* Default for error */
		    if (iirowcount != (i4 *)NULL)
			*iirowcount = (i4)(qef_rcb->qef_rowcount);
		    if (iierrorno != (i4 *)NULL)
			*iierrorno = iierror_save;	/* Saved before "pop" */
		}
	    }
	    
	    if (qp->qp_qmode != QEQP_EXEDBP)
	    {
		if (qef_rcb->error.err_code == E_QE0121_USER_RAISE_ERROR)
		{
		    /* User has already reported error and aborted	    */
		    /* transaction. At this time report back "success".	    */
		    status = E_DB_OK;
		}
		break;
	    }
	    
	    /* error in a DB procedure, abort up to the beginning of the
	    ** procedure if there are no other cursors opened.  We
	    ** guarantee that by decrementing the qef_open_count.  If the
	    ** count becomes zero, qeq_cleanup will abort to the last internal 
	    ** savepoint. Notice that we also tell qeq_cleanup not to release
	    ** the DSH so we can continue with the next action */
	    open_count = qef_cb->qef_open_count;

	    /* Fix for bug 63465. Error in nested procedure should not
	    ** rollback the whole transaction.
	    */	
            if (qef_cb->qef_open_count > 0)
            {
                if (qef_rcb->qef_context_cnt > 0)
                    qef_cb->qef_open_count = 0;
                else
                    qef_cb->qef_open_count--;
            }

	    /* User interrupt should abort a procedure */
	    if (qef_cb->qef_user_interrupt_handled)
	        break;

	    status = qeq_cleanup(qef_rcb, status, FALSE);
	    qef_cb->qef_open_count = open_count;

	    /* If there are more actions to execute, restart the
	    ** transaction and reopen the tables.
	    **
	    ** ** FIXME this is not really correct in the face of
	    ** multi-action queries.  Although nothing terribly bad
	    ** happens, iierrornumber is lost, and there may be other
	    ** more-or-less-subtle ill effects.  E.g. consider:
	    **   select count(*) into :i from foo where int4(str)=0
	    ** and a non-numeric str.  The query is a SAGG and a GET.
	    ** The SAGG action will error out, but this code will restart
	    ** at the following GET, which will succeed (with an immediate
	    ** EOF, but no error), wiping iierrornumber.
	    **
	    ** Not sure how this would be fixed outside of a user-query
	    ** id number of some sort so that we could restart at the
	    ** next query, rather than the next action.
	    */
	    if ((act->ahd_next != (QEF_AHD *)NULL)
		&& (act->ahd_next->ahd_atype != QEA_RETURN))
	    {
		/* restart the transaction if necessary */
		if (qef_cb->qef_stat == QEF_NOTRAN)
		{
		    if (status = qet_begin(qef_cb))
			break;
		}
		/* set if not set: bug 80759 */
		if (first_act == (QEF_AHD *)NULL)
			first_act = qp->qp_ahd;
			
		/* reopen and validate the tables */
		status = qeq_topact_validate(qef_rcb, dsh, first_act, FALSE);
		if (status != E_DB_OK)
		{
		    qef_rcb->error.err_code = dsh->dsh_error.err_code;
		    if (qef_rcb->error.err_code == E_QE1004_TABLE_NOT_OPENED) 
		    {
			/*
			** B103150
			** validate succeeded but table could not be opened 
			** with the required lockmode because of lock timeout
			*/
			qef_rcb->error.err_code = E_QE0000_OK;
			status = E_DB_OK;       /* continue with next action */
		    }
		    else if (qef_rcb->error.err_code == E_QE0015_NO_MORE_ROWS)
		    {
			qef_rcb->error.err_code = E_QE0000_OK;
			status = E_DB_OK;	/* continue with next action */
		    }
		    else
		    {
			if ((qef_rcb->error.err_code == E_QE0023_INVALID_QUERY)
			    ||
			  (qef_rcb->error.err_code == E_QE0301_TBL_SIZE_CHANGE))
			{
			    qef_rcb->error.err_code = E_QE0115_TABLES_MODIFIED;
			    status = E_DB_ERROR;
			    qef_error(qef_rcb->error.err_code, 0L, status, &err,
				&qef_rcb->error, 0);
			}
                        else if (qef_rcb->error.err_code ==
                                E_DM004D_LOCK_TIMER_EXPIRED)
                        {
                            qef_rcb->error.err_code = 
					E_QE0036_LOCK_TIMER_EXPIRED;
                            status = E_DB_ERROR;
                            qef_error(qef_rcb->error.err_code, 0L, status, &err,
                            &qef_rcb->error, 0);
                        }
			break;
		    }
		}
	    }
	}

	if (iirowcount != (i4 *)NULL)
	    *iirowcount = (i4) (qef_rcb->qef_rowcount);

	act_state = DSH_CT_INITIAL;

	/* On completion of an action list check for the special conditions */
	/*	Rule Action List Completed				    */
	/*	Nested Procedure Completed				    */
	/*	Execute Immediate Action List Completed			    */
	/* Note that this logic allows a rule action list to be	    */
	/* terminated by a QEA_RETURN action. In practice, this situation   */
	/* will never arise. */

	/*
	** We also want to fall into this block if an error occurred while
	** executing the SELECT statement that supports an ALTER TABLE ADD
	** REFERENTIAL CONSTRAINT
	*/

	if (act == NULL || 
	    act->ahd_atype == QEA_RETURN ||	
	    act->ahd_next == NULL ||
	    errorDuringAddConstraintSelect == TRUE
	   )
	{
	    /* Normal unwinding, set RESET flag to FALSE. */
	    act_reset = FALSE;

	    if (dsh->dsh_depth_act == 0)
	    {
		/* We are ending the invocation of a procedure/execution of */
     	  	/* of execute immediate action. If this is a nested 	    */
		/* procedure call, restore the old QP's context.   */
		if (dsh->dsh_stack != (QEE_DSH *)NULL)
		{
		    tmp_dsh = dsh->dsh_stack;
		    if (qef_cb->qef_open_count > 0)
		    	qef_cb->qef_open_count--;
		    qeq_cleanup(qef_rcb, status, TRUE);
		    dsh = tmp_dsh;
		    qef_cb->qef_dsh = (PTR) dsh;
		    qef_rcb->qef_context_cnt--;
		    qef_rcb->qef_qso_handle = dsh->dsh_saved_rcb->qef_qso_handle;
		    qef_rcb->qef_rowcount = dsh->dsh_saved_rcb->qef_rowcount;

		    qeq_restore(dsh->dsh_saved_rcb, qef_rcb, dsh, &mode, &act,
				&rowcount, &qp, &cbs, &iirowcount);

		    first_act = qp->qp_ahd;

		    first_act = qp->qp_ahd;

		    /*
		    ** Popping out of nested procedure (that was not fired via
		    ** a rule - depth_act = 0) may require nested status
		    ** retrieval to support the INTO clause. 
		    */
		    if (act != NULL
			&& act->ahd_atype == QEA_CALLPROC
			&& act->qhd_obj.qhd_callproc.ahd_return_status
			.ret_dbv.db_datatype != DB_NODT)
		    {
			DB_DATA_VALUE src_dbv, dest_dbv;
			
			/* At this point we need to copy the return status
			** into the caller's variable pointed to by
			** ahd_return_status
			*/
			STRUCT_ASSIGN_MACRO(act->qhd_obj.qhd_callproc
					    .ahd_return_status.ret_dbv,
					    dest_dbv);
			dest_dbv.db_data = (PTR)
			    (dsh->dsh_row[act->qhd_obj.qhd_callproc
			      .ahd_return_status.ret_rowno]
			     + act->qhd_obj.qhd_callproc
			     .ahd_return_status.ret_offset);
			src_dbv.db_datatype = DB_INT_TYPE;
			src_dbv.db_prec = 0;
			src_dbv.db_length = sizeof(qef_rcb->qef_dbp_status);
			src_dbv.db_data = (PTR) &qef_rcb->qef_dbp_status;
			src_dbv.db_collID = -1;
			status = adc_cvinto(dsh->dsh_adf_cb, 
					    &src_dbv, &dest_dbv);
			if (status != E_DB_OK
			    && (status =
				qef_adf_error(&dsh->dsh_adf_cb->adf_errcb, 
				  status, qef_cb, &qef_rcb->error)) != E_DB_OK)
			{
			    break;
			}
		    }
		    if (qp->qp_oerrorno_offset == -1)
		    	iierrorno = (i4 *)NULL;
		    else
		    	iierrorno = (i4 *)(dsh->dsh_row[qp->qp_rerrorno_row] + 
			    qp->qp_oerrorno_offset);

		    /* Check if this was a rule triggered CALLPROC, in
		    ** which case, we may need to execute a CX to return
		    ** OUT/INOUT parms to the triggering statement. */
		    if (dsh->dsh_depth_act > 0 && 
			act != NULL && act->ahd_atype == QEA_CALLPROC &&
			(act->qhd_obj.qhd_callproc.ahd_flags & QEF_CP_BEFORE) &&
			(qen_adf = act->qhd_obj.qhd_callproc.ahd_procp_return))
		    {
			ade_excb = (ADE_EXCB*) cbs[qen_adf->qen_pos];
			status = qen_execute_cx(dsh, ade_excb);
		    }
		}
		/* We are ending the invocation of a QEA_CREATE_TABLE/
		** QEA_CREATE_INTEGRITY/QEA_CREATE_VIEW action in CREATE
		** SCHEMA. Restore the old QP's context so that we can
		** continue with the next QEA_EXEC_IMM action.
		*/
		else if (dsh->dsh_exeimm != (QEE_DSH *)NULL)
                {
                   tmp_dsh = dsh->dsh_exeimm;

		   /* We have two scenarios here:
		   **
		   ** 1) We do not want to decrement the number of
		   **    open cursors if we are doing a CREATE SCHEMA
		   **    statement because CREATE SCHEMA is treated as 
		   **    one unit. Thus, we will decrement at the end
		   **    of the routine. Otherwise, if an error occurs
		   **    during the middle of CREATE SCHEMA, the whole
		   **    statement is not rolled back. But only the
		   **    the offending statements in C.S. block are 
		   **    rolled back and the rest of the objects in
		   **    C.S. block are created.
		   **
		   ** 2) But when we get an error during ALTER TABLE
		   **    (to be specific during the SELECT for ALTER TABLE),
		   **	 we do not want to rollback the whole transaction,
		   **	 but only rollback the offending statement. For
		   **	 example: after creating and inserting into a
		   **	 table, if we get an error while doing an alter
		   **	 table, we do not want to rollback the effects of
		   **	 creating and inserting into the table, but only the
		   **	 the alter table statement.
		   **
		   ** FIXME: This special code will not be needed once the
		   **        the cursor savepoint code is integrated. This
		   **	     is because QEF will lay an internal savepoint
		   **	     previous to any update or DDL statement and
		   **	     and error during any of the update/DDL statement
		   **	     will rollback the effects of only that particular
		   **	     statement.
		   **
		   ** Fix for bug 59929.	
		   */	

		   if ( errorDuringAddConstraintSelect == TRUE )
		      qef_cb->qef_open_count--;

                   qeq_cleanup(qef_rcb, status, TRUE);
                   dsh = tmp_dsh;
                   qef_cb->qef_dsh = (PTR)dsh;
                   qeq_rcb = *(dsh->dsh_exeimm_rcb);
		   qef_rcb->qef_qso_handle = qeq_rcb.qef_qso_handle;

	           /* we want to restore PST_INFO context */
                   qef_rcb->qef_info = qeq_rcb.qef_info;
                   
		   qeq_restore(&qeq_rcb, qef_rcb, dsh, &mode, &act,
                      &rowcount, &qp, &cbs, &iirowcount);

		   /*
		   ** This is the secret wink which tells the execute
		   ** immediate recovery logic that an error occurred.
		   */

		   if ( errorDuringAddConstraintSelect == TRUE )
		   {  qef_rcb->qef_intstate |= QEF_EXEIMM_CLEAN; }
                }
		else
		{
		    /* Ending the outermost procedure (QP) */
		    break;
		}
	    }
	    if ( act == NULL || act->ahd_next == NULL ||
		 errorDuringAddConstraintSelect == TRUE
	       )
	    {
		if (dsh->dsh_depth_act > 0)
		{
		    /* Ending a rule action list, restore previous action */
		    dsh->dsh_depth_act--;
		    act = dsh->dsh_ctx_act[dsh->dsh_depth_act].dsh_ct_ptr;
		    act_state = 
			    dsh->dsh_ctx_act[dsh->dsh_depth_act].dsh_ct_status;
		    qef_rcb->qef_rule_depth--;
		    continue;
		}
		/* in case of SELECT ANY(1) of QEA_CREATE_INTEGRITY for
		** ALTER TABLE ADD CONSTRAINT statement, we need to continue 
		** with the QEA_CREATE_INTEGRITY action to create the 4 rules,
	 	** 3 procedures and 1 index.
		** FIXME: This check will not be needed once the creation
		** of rules, procedures and index are routed like DML
		** statements through the server.
		*/
	        else if (qef_rcb->qef_cb->qef_callbackStatus &
				QEF_EXECUTE_SELECT_STMT)
		{
		        continue;
		}

		break;
	    }
	} 
	act = act->ahd_next;
	act_reset = TRUE;

	/* preserve XACT status as of this action end */
	qef_cb->qef_ostat = qef_cb->qef_stat;  

    }


    /* decrement the count of open query/cursor so that
    ** qeq_cleanup can do the right thing */
    if (qef_cb->qef_open_count > 0 && mode == QEQ_QUERY)
	--qef_cb->qef_open_count;

    /* check for qef_rcb->qef_intstate and reset accordingly */
    /* On the first pass, this bit is not reset as we return to SCF
    ** from the WHILE loop.
    */

    if ((qef_rcb->qef_intstate & QEF_EXEIMM_PROC) != 0)
    {
       qef_rcb->qef_intstate &= ~QEF_EXEIMM_PROC;
    }
    else
    {
       qef_rcb->qef_intstate &= ~QEF_DBPROC_QP;
    }

    /* If this was row-producing procedure, send back QE0015.
    ** Also send back rows, output bufs used.
    */
    if (qp != NULL && (qp->qp_status & QEQP_ROWPROC))
    {
	status = E_DB_WARN;
	qef_rcb->error.err_code = E_QE0015_NO_MORE_ROWS;
	qef_rcb->qef_count = dsh->dsh_rpp_bufs;
	qef_rcb->qef_rowcount = qef_rcb->qef_targcount = dsh->dsh_rpp_rows;
    }

    if (ddb_b)
    {
	/* end of query, deal with hold-and-execute situation if necessary */

	if (qss_p->qes_q5_ldb_rowcnt > 0)
	{
	    qef_rcb->qef_rowcount = qss_p->qes_q5_ldb_rowcnt;
	    qef_rcb->qef_targcount = qss_p->qes_q5_ldb_rowcnt;
	}

	if (! qef_cb->qef_abort 
	    &&
	    qss_p->qes_q1_qry_status & QES_04Q_HOLD)
	{
	    qef_rcb->qef_rowcount = qss_p->qes_q4_hold_cnt;
	    qef_rcb->error.err_code = E_QE0015_NO_MORE_ROWS;
	    status = E_DB_WARN;		/* return end of retrieval */
	}
    }

    if (err_save == E_QE0021_NO_ROW)
    {
	/* This is to fix a problem in the FIPS test suits Where a
	** transaction is being rolled back when an unpositioned update is
	** attempted. Since in this case we haven't done an update yet we
	** don't need to rollback the statement. This can be taken out when
	** we do a more general fix of error handling when cursors are open.
	** i.e. not roll back the transaction.
	*/
	qeq_cleanup(qef_rcb, E_DB_OK, TRUE);
	return (status);
    }
    else
    {
	return (qeq_cleanup(qef_rcb, status, TRUE));
    }
}

/*
**  Name: QEQ_QP_APROPOS   -
**
**  Description:
**          returns TRUE iff query plan containing specified QEF_VALID struct.
**          is appropriate for table with the specified size.
**          parameter of the specified size.
**
**  Inputs:
**      vl                      QEF_VALIDstruct of query plan to check
**      page_count              current pagecount of table.
**	care_if_smaller		TRUE iff we should check to see if the table
**				is significantly smaller than the one the
**				query was compiled with
**
**  Output:
**      none
**
**      Returns:
**          returns TRUE iff query plan containing specified QEF_VALID struct.
**          is appropriate for tabl with the specified size.
**          parameter of the specified size.
**      Exceptions:
**          none
**
**  History:
**      20-feb-93 (jhahn)
**          written (based on code from qeq_valid).
**	01-sep-93 (jhahn)
**	    Added support for multiple query plans for set input procedures.
**	18-jul-96 (inkdo01)
**	    Minor change for user-coded "set of" parms.
*/
bool
qeq_qp_apropos(
QEF_VALID       *vl,
i4		page_count,
bool		care_if_smaller)
{
    /*
    ** The semantics of the size_sensitive field in the valid
    ** list is defined below:
    **
    **   If the size_sensitive is FALSE, the query plan is
    **   totally independent of actual number of pages in the
    **   relation.
    **
    **   If the size_sensitive is TRUE, the estimated number
    **   of affected pages in the query plan must be adjusted by
    **   the proportion of the actual page count (from DMF) to
    **   the estimated page count (in the query plan).
    **
    **   If the difference of the adjusted value and the estimated
    **   number (secified in the query plan) is greater than a
    **   certain level (2 times + 5), the query plan is invalid.
    */
    i4 fudge;

    if (vl == NULL) return(TRUE);	/* this can happen with user-
					** induced "set of" parms -
					** so just return when it does */
    if ((!vl->vl_size_sensitive) || (vl->vl_est_pages == 0))
        return (TRUE);

    /* if the scaled estimated pages is more than
    ** 2 times the estimate + 5, cause a recompile with new
    ** RDF cache */

    fudge = (vl->vl_total_pages/vl->vl_est_pages);
    if (fudge >= (MAXI4/5))
	return (TRUE); /* Otherwise would overflow. */
    fudge = fudge*5;
    return (((page_count-fudge)/2 <= vl->vl_total_pages) &&
	    ((!care_if_smaller) ||
	     page_count >= (vl->vl_total_pages - fudge) / 2));
}

/*
**  Name: QEQ_DSH   - find/create the current DSH
**
**  Description:
**	The session control block (qef_cb) is searched for the current DSH 
**	structure.  If it doesn't exist, and the must_find flag is set to
**	create one, QEE routines will be called to create it.
**
**  Inputs:
**	qef_rcb
**	    .qef_eflag		designate error handling semantics
**				for user errors.
**		QEF_INTERNAL	return error code.
**		QEF_EXTERNAL	send message to user.
**	    .qef_cb		session control block
**	    .qef_qso_handle	QSF id of QP
**	    .qef_qp		QSF name of QP - used if qef_qso_handle is null.
**	must_find		Search qualifier
**	       -1		error if the dsh already exists.
**		0		create one if the dsh is not found.
**				Or, if tproc, create one without looking.
**		1		error if the dsh is not found. 
**	qptype			One of QEQDSH_NORMAL, QEQDSH_IN_PROGRESS, or
**				QEQDSH_TPROC.  The in-progress call is for
**				normal callprocs or execute immediates, and
**				alters xaction checking slightly.  The tproc
**				call asks to flag the DSH as being a table
**				proc DSH, and alters the DSH search slightly.
**	page_count		number of pages in set input table parameter.
**				-1 if this is not for a set input procedure.
**
**  Output:
**	dsh			set to the current dsh
**	qef_rcb
**	    .error.err_code	one of the following
**				E_QE0000_OK
**				E_QE0008_CURSOR_NOT_OPENED
**				E_QE0014_NO_QUERY_PLAN
**				E_QE000E_ACTIVE_COUNT_EXCEEDED
**				E_QE000F_OUT_OF_OTHER_RESOURCES
**				E_QE0004_NO_TRANSACTION
**	
**
**	Returns:
**	    E_DB_{OK,ERROR}
**	Exceptions:
**	    none
**  Side Effects:
**	qef_cb->qef_dsh is set to the current dsh
**
**  History:
**	18-jul-86 (daved)
**	    written
**	22-oct-87 (puree)
**	    Each time a dsh is located, copy the pointer to ADF query
**	    constant control block from the dsh into the session ADF_CB.
**	27-oct-87 (puree)
**	    use qef_cb->qef_adf_cb->adf_constants.
**	20-feb-89 (paul)
**	    Added support for rule action lists. A DSH may be loaded
**	    by a nested procedure call. Under these conditions it is
**	    not an error to initiate a new DSH when we are in a SST.
**	23-mar-89 (paul)
**	    Corrected problem allocating multiple DSH's for a single QP.
**	10-jan-90 (neil)
**	    Cleaned up some DSH handling for rules not to release the DSH if
**	    we're in progress (in the middle of nested procedure/rule
**	    processing) and suppress error handling for caller.
**	18-jun-91 (davebf)
**	    Avoid closing all cursors when dbp not found in cache. b37470.
**      15-jul-91 (stevet)
**          Bug #36065.  Add error E_QE0238_NO_MSTRAN to QEQ_DSH if there
**          is no MST when trying to open new query and another query is
**          already active.
**	12-feb-92 (nancy)
**	    Add new return from qeq_validate(), E_QE0301_TBL_SIZE_CHANGE to
**	    same handling as E_QE0023. (bug 38565)
**	12-feb-93 (jhahn)
**          Added support for statement level rules. (FIPS)
**	01-sep-93 (jhahn)
**	    Added support for multiple query plans for set input procedures.
**	21-mar-94 (rickh)
**	    Look up DSH's by QSF object handle, not query plan name. This
**	    fixes bug 60540.
**      20-may-94 (eric)
**          Corrected fix for bug 60540 by moving the qsf call to look up the
**          qp from qee_fetch to qeq_dsh. Qeq_dsh now has total responsibility
**          to do this, which means that the qp must be passed to qee_fetch.
**	4-mar-97 (inkdo01)
**	    Detect attempt to invoke SET OF proc from row level rule (with
**	    scalar parameter list).
**      13-Mar-99 (islsa01)
**          Fix for bug #96010. This change sets up the size_sensitive
**          field to false for the query plan of a session temporary table 
**          to be totally independent of actual number of pages in the
**          relation.
**	22-sep-03 (inkdo01)
**	    Removed TRdisplay code for tp qe128.
**      27-May-2008 (gefei01)
**          Allocate dsh for TPROC.
**      24-Feb-2009 (gefei01)
**          Fix bug 121711:: Make table procedure dsh allocation an exception
**          for multiple statement transaction.
**	18-Jun-2010 (kschendel) b123775
**	    Combine is-tproc and in-progress parameters.
**	    Handle QE030B from execute procedure properly (eg embedded
**	    execute procedure, which runs the query with the dbp QP directly
**	    rather than executing a callproc.)
**	    This routine is called a lot, do some minor optimizing.
*/
DB_STATUS
qeq_dsh(
	QEF_RCB		*qef_rcb,
	i4		must_find,
	QEE_DSH		**pdsh,
	QEQ_DSH_QPTYPE	qptype,
	i4         page_count
 	)
{
    PTR		handle;
    i4	err;
    DB_STATUS	stat;
    DB_STATUS	status   = E_DB_OK;
    QEF_CB	*qef_cb = qef_rcb->qef_cb;
    bool        is_set_input = page_count >= 0;
    QSF_RCB	qsf_rcb;
    QEE_DSH	*dsh;
    QEF_QP_CB	*qp;

    *pdsh = dsh = NULL;
    handle = qef_rcb->qef_qso_handle;
    if (handle != NULL && qptype != QEQDSH_TPROC)
    {
	/* Try to locate the DSH in the session open-DSH list. */
	for (dsh = (QEE_DSH *)qef_cb->qef_odsh; dsh; dsh = dsh->dsh_next)
	{
	    if (dsh->dsh_qp_handle == handle &&
		(dsh->dsh_qp_status & DSH_IN_USE) == 0)
		break;		/* found an active DSH */
	}
	if (dsh != NULL && must_find >= 0 && !is_set_input)
	{
	    /* Fast-path return especially for cursor FETCH */
	    dsh->dsh_adf_cb->adf_constants = dsh->dsh_qconst_cb;
	    qef_cb->qef_dsh = (PTR) dsh;
	    dsh->dsh_qp_status |= DSH_IN_USE;
	    *pdsh = dsh;
	    return (E_DB_OK);
	}
    }
    if (dsh == NULL)
    {
	/* DSH not in the list, or caller doesn't know the handle.
	** We'll need to lock the QP by handle, or look it up by ID.
	*/

	qsf_rcb.qsf_next = (QSF_RCB *)NULL;
	qsf_rcb.qsf_prev = (QSF_RCB *)NULL;
	qsf_rcb.qsf_length = sizeof(QSF_RCB);
	qsf_rcb.qsf_type = QSFRB_CB;
	qsf_rcb.qsf_owner = (PTR)DB_QEF_ID;
	qsf_rcb.qsf_ascii_id = QSFRB_ASCII_ID;
	qsf_rcb.qsf_sid = qef_rcb->qef_cb->qef_ses_id;
	qsf_rcb.qsf_lk_state = QSO_SHLOCK;

	if (handle != NULL && qptype != QEQDSH_TPROC)
	{
	    qsf_rcb.qsf_obj_id.qso_handle = qef_rcb->qef_qso_handle;
	    status = qsf_call(QSO_LOCK, &qsf_rcb);
	    if (status)
	    {
		qef_rcb->error.err_code = E_QE0019_NON_INTERNAL_FAILURE;
		return(status);
	    }
	    qp = (QEF_QP_CB *) qsf_rcb.qsf_root;
	}
	else
	{
	    /* Obtain and lock the QP from QSF */
	    qsf_rcb.qsf_obj_id.qso_type = QSO_QP_OBJ;
	    qsf_rcb.qsf_obj_id.qso_lname = sizeof (DB_CURSOR_ID);
	    MEcopy((PTR)&qef_rcb->qef_qp, sizeof (DB_CURSOR_ID), 
		(PTR)qsf_rcb.qsf_obj_id.qso_name);
	    qsf_rcb.qsf_obj_id.qso_handle = NULL;
	    do
	    {
		status = qsf_call(QSO_GETNEXT, &qsf_rcb);
		if (status)
		{
		    if (qsf_rcb.qsf_error.err_code == E_QS0019_UNKNOWN_OBJ)
			qef_rcb->error.err_code = E_QE0023_INVALID_QUERY;
		    else
			qef_rcb->error.err_code = E_QE0014_NO_QUERY_PLAN;
		    return(status);
		}
		qp = (QEF_QP_CB *) qsf_rcb.qsf_root;
		handle = qsf_rcb.qsf_obj_id.qso_handle;
		/* For a session temporary table, 
		** size_sensitive field is set to false */
		if ( qp->qp_setInput != NULL &&
		     (qp->qp_setInput->vl_flags & QEF_SESS_TEMP)
		   )
		     qp->qp_setInput->vl_size_sensitive = FALSE;
	    }
	    while (is_set_input &&
		   !qeq_qp_apropos(qp->qp_setInput, page_count, TRUE));

	    /* For sharable QP.  Sometimes the QP sent to QEF is destroyed by
	    ** another session.  So check for a NULL root.  If so, release the
	    ** lock and tell SCF that we have got an invalid QP */
	    if (qp == (QEF_QP_CB *) NULL)
	    {
		qsf_call(QSO_UNLOCK, &qsf_rcb);
		qef_rcb->error.err_code = E_QE0023_INVALID_QUERY;
		status = E_DB_ERROR;
		return(status);
	    } 

	    /* When the dsh is a table procedure DSH, don't bother
	    ** looking for other session DSH's using this table proc.
	    ** Just create a new DSH for the table proc.
	    ** (We're being called either from validation in qeqvalid,
	    ** or from tproc node open in qentproc.)
	    **
	    ** The reason is that tprocs are like tables in that one
	    ** can have multiple independent open-references to the
	    ** same tproc.  Each one must have its own DSH, we don't want
	    ** to find a DSH that's for some other reference.
	    */
	    if (must_find != 0 || qptype != QEQDSH_TPROC)
	    {
		/* Not TPROC, now that we know the QP handle, look for the
		** DSH on the session list to see if we already have it.
		*/
		for (dsh = (QEE_DSH *)qef_cb->qef_odsh; dsh; dsh = dsh->dsh_next)
		{
		    if (dsh->dsh_qp_handle == handle &&
			(dsh->dsh_qp_status & DSH_IN_USE) == 0)
			break;		/* found an active DSH */
		}
	    }
	}
    }

    /* This may seem an odd place to detect this, but ... If page_count  < 0
    ** but qp->qp_setInput is non-null, this is an attempt to invoke an
    ** already compiled SET OF procedure from a row level rule (with scalar 
    ** parameter list). This is the only place in which all the pieces are
    ** available for detection of the condition. BTW, attempts to explicitly
    ** call a SET OF proc with a scalar parm list (instead of a global temp
    ** table) get caught in OPF (opc). Only rules have to be checked here. */

    if (qp->qp_setInput != NULL && !is_set_input)
    {
	qef_cb->qef_dsh = (PTR)NULL;
	err = E_QE030B_RULE_PROC_MISMATCH;
	status = E_DB_ERROR;
    }
    /* continue if we found a dsh and the search qualifier is to find one */
    else if (dsh != NULL && must_find >= 0)
    {
	qef_cb->qef_dsh = (PTR) dsh;
	status = E_DB_OK;
    }
    /* error if we found one and the qualifier is not to find one */
    else if (dsh != NULL && must_find < 0)
    {
	qef_cb->qef_dsh = (PTR)NULL;
	err = E_QE000C_CURSOR_ALREADY_OPENED;
	status = E_DB_ERROR;
    }
    /* also error if the DSH is not found when it is expected to be found */
    else if (dsh == NULL && must_find > 0)
    {
	qef_cb->qef_dsh = (PTR)NULL;
	err = E_QE0008_CURSOR_NOT_OPENED;
	status = E_DB_ERROR;
    }
    /* The DSH is not found and the qualifier is to create one */
    else
    {
	do
	{
	    /* allocate a new DSH */
	    status = qee_fetch(qef_rcb, qp, &dsh, page_count, &qsf_rcb,
			(qptype == QEQDSH_TPROC) );
	    err = qef_rcb->error.err_code;
	    if (status != E_DB_OK)
		break;
	    /* replace cursor QPs are OK. Don't check for deferred or
	    ** transactions. Also applies to delete cursor QPs.
	    */
	    if (dsh->dsh_qp_ptr->qp_ahd->ahd_atype == QEA_RUP ||
		dsh->dsh_qp_ptr->qp_ahd->ahd_atype == QEA_RDEL)
		break;

	    /* error if we want to create a new DSH (open another query).
	    ** and another query plan is active and we don't have an MST.
	    ** Don't check this if DSH is for a DB proc or the query is
	    ** in progress for some reason.
	    */
	    if (qef_cb->qef_open_count > 0 &&
                qef_cb->qef_stat != QEF_MSTRAN &&
		qptype == QEQDSH_NORMAL)
	    {
		err = E_QE0238_NO_MSTRAN;
		status = E_DB_ERROR;
		break;
	    }	    
	} while (FALSE);

	if (status != E_DB_OK)
	{
	    /*
	    ** If in nested context make sure to clear qef_dsh, but do not call
	    ** qee_cleanup with a NULL DSH as that will zap all session DSH's.
	    */
	    if (qptype == QEQDSH_IN_PROGRESS || dsh == NULL)
	    {
		qef_cb->qef_dsh = (PTR)NULL;
	    }
	    else
	    {
		stat = qee_cleanup(qef_rcb, &dsh);
		if (stat > status)
		{
		    status = stat;
		    err = qef_rcb->error.err_code;
		}
	    }
	}
    }

    if (status != E_DB_OK)
    {
	i4 local_error;
	if (qptype != QEQDSH_IN_PROGRESS &&	/* Handled by caller */
	    (err != E_QE0023_INVALID_QUERY) &&  
            (err != E_QE0301_TBL_SIZE_CHANGE))  /* not DBP recompile needed */
	{
	    if (err == E_QE030B_RULE_PROC_MISMATCH)
	    {
		char *pn;
		/* This message has a parameter, need to supply it. */
		pn = qef_rcb->qef_dbpname.qso_n_id.db_cur_name;
		qef_error(err, 0, status, &local_error, &qef_rcb->error,1,
			qec_trimwhite(DB_DBP_MAXNAME,pn), pn);
	    }
	    else
	    {
		qef_error(err, 0L, status, &local_error,
		      &qef_rcb->error, 0);
	    }
	    err = E_QE0122_ALREADY_REPORTED;
	}
	qef_rcb->error.err_code = err;
    }
    else
    {
	dsh->dsh_adf_cb->adf_constants = dsh->dsh_qconst_cb;
	dsh->dsh_qp_status |= DSH_IN_USE;
	qef_rcb->error.err_code = E_QE0000_OK;
	*pdsh = dsh;
    }
    return (status);
}


/*{
** Name: QEQ_LQEN	- Return the left of a QEN_NODE tree.
**
** Description:
**      This routine is used as a part of the tree printing routines. This 
**      routine returns the pointer to the left child of the given QEN_NODE  
[@comment_line@]...
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
**      31-JAN-90 (eric)
**          written
**	21-dec-99 (inkdo01)
**	    Add support for hash joins.
**	5-sep-03 (inkdo01)
**	    Add support for hash aggregation.
**	8-jan-04 (toumi01)
**	    Add support for exchange nodes.
[@history_template@]...
*/
static PTR
qeq_lqen(
PTR	p )
{
    QEN_NODE	*node = (QEN_NODE *) p;
    QEN_NODE	*ret_node = NULL;
    QEF_AHD	*ahd;

    switch (node->qen_type)
    {
     case QE_TJOIN:
	ret_node = node->node_qen.qen_tjoin.tjoin_out;
	break;

     case QE_KJOIN:
	ret_node = node->node_qen.qen_kjoin.kjoin_out;
	break;

     case QE_HJOIN:
	ret_node = node->node_qen.qen_hjoin.hjn_out;
	break;

     case QE_EXCHANGE:
	ret_node = node->node_qen.qen_exch.exch_out;
	break;

     case QE_ISJOIN:
     case QE_FSMJOIN:
     case QE_CPJOIN:
	ret_node = node->node_qen.qen_sjoin.sjn_out;
	break;

     case QE_SEJOIN:
	ret_node = node->node_qen.qen_sejoin.sejn_out;
	break;

     case QE_SORT:
	ret_node = node->node_qen.qen_sort.sort_out;
	break;

     case QE_TSORT:
	ret_node = node->node_qen.qen_tsort.tsort_out;
	break;

     case QE_QP:
	/* go find the first action that has a QEN_NODE tree below this 
	** QE_QP node
	*/
	for (ahd = node->node_qen.qen_qp.qp_act;
		ahd != NULL && ret_node == NULL;
		ahd = ahd->ahd_next
	    )
	{
	    switch (ahd->ahd_atype)
	    {
	     case QEA_DMU:
	     case QEA_UTL:
	     case QEA_RUP:
	     case QEA_COMMIT:
	     case QEA_ROLLBACK:
	     case QEA_RETURN:
	     case QEA_MESSAGE:
	     default:
		ret_node = NULL;
		break;

	     case QEA_APP:
	     case QEA_SAGG:
	     case QEA_PROJ:
	     case QEA_AGGF:
	     case QEA_UPD:
	     case QEA_DEL:
	     case QEA_GET:
	     case QEA_HAGGF:
	     case QEA_RAGGF:
	     case QEA_LOAD:
	     case QEA_PUPD:
	     case QEA_PDEL:
		/* This will make the first action be the left */
		ret_node = ahd->qhd_obj.qhd_qep.ahd_qep;
		break;

	     case QEA_IF:
		/* we can handle if statements yet */
		ret_node = NULL;
		break;
	    }
	}
	break;

     default:
	ret_node = NULL;
	break;
    }

    return((PTR) ret_node);
}

/*{
** Name: QEQ_RQEN	- Return the RIGHT of a QEN_NODE tree.
**
** Description:
**      This routine is used as a part of the tree printing routines. This 
**      routine returns the pointer to the RIGHT child of the given QEN_NODE  
[@comment_line@]...
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
**      31-JAN-90 (eric)
**          written
**	21-dec-99 (inkdo01)
**	    Add support for hash joins.
**	5-sep-03 (inkdo01)
**	    Add support for hash aggregation.
[@history_template@]...
*/
static PTR
qeq_rqen(
PTR	p )
{
    QEN_NODE	*node = (QEN_NODE *) p;
    QEN_NODE	*ret_node = NULL;
    QEN_NODE	*node1, *node2;
    QEF_AHD	*ahd;

    switch (node->qen_type)
    {
     case QE_ISJOIN:
     case QE_FSMJOIN:
     case QE_CPJOIN:
	ret_node = node->node_qen.qen_sjoin.sjn_inner;  
	break;

     case QE_HJOIN:
	ret_node = node->node_qen.qen_hjoin.hjn_inner;  
	break;

     case QE_SEJOIN:
	ret_node = node->node_qen.qen_sejoin.sejn_inner;
	break;

     case QE_QP:
	/* go find the second action that has a QEN_NODE tree below this 
	** QE_QP node
	*/
	node1 = node2 = (QEN_NODE *) NULL;
	for (ahd = node->node_qen.qen_qp.qp_act;
		ahd != NULL && node2 == NULL;
		ahd = ahd->ahd_next
	    )
	{
	    switch (ahd->ahd_atype)
	    {
	     case QEA_DMU:
	     case QEA_UTL:
	     case QEA_RUP:
	     case QEA_COMMIT:
	     case QEA_ROLLBACK:
	     case QEA_RETURN:
	     case QEA_MESSAGE:
	     default:
		break;

	     case QEA_APP:
	     case QEA_SAGG:
	     case QEA_PROJ:
	     case QEA_AGGF:
	     case QEA_UPD:
	     case QEA_DEL:
	     case QEA_GET:
	     case QEA_HAGGF:
	     case QEA_RAGGF:
	     case QEA_LOAD:
	     case QEA_PUPD:
	     case QEA_PDEL:
		/* This will make the second action be the right */
		if (node1 == NULL)
		{
		    node1 = ahd->qhd_obj.qhd_qep.ahd_qep;
		}
		else
		{
		    node2 = ahd->qhd_obj.qhd_qep.ahd_qep;
		}
		break;

	     case QEA_IF:
		/* we can handle if statements yet */
		break;
	    }
	}
	ret_node = node2;
	break;

     default:
	ret_node = NULL;
	break;
    }

    return((PTR) ret_node);
}

/*{
** Name: qeq_tr_callback
**
** Description:
**      This function is meant to be called by TRformat. It keeps trailing
**	'\n's discarded by TRformat and inserts a '\0' after the formatted
**	string (since TRformat does not null-terminate the formatted buffer).
**	
** Inputs:
**	nl		if non-zero, add trailing newline
**	length		length of formatted buffer
**	buffer		formatted buffer from TRformat
**
** Outputs:
**	buffer		processed format buffer
**	Returns:
**	    None
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	16-feb-94 (swm)
**	    Added.
*/
static i4
qeq_tr_callback(nl, length, buffer)
char		*nl;
i4		length;
char		*buffer;
{
	char *p = buffer + length;	/* end of buffer pointer */

	/* nl is used as a flag, it is not really a pointer */
	if (nl != NULL)
		*p++ = '\n';
	*p = '\0';
	return (0);
}

/*{
** Name: QEQ_PRQEN	- Print an QEN_NODE struct for tree printing.
**
** Description:
{@comment_line@}...
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
**      28-oct-86 (eric)
**          written
**      20-may-94 (eric)
**          Corrected estimation of stat collection cost. Cost of collection
**          of nodes below is twice that of the current node because of
**          the way the the OS gets CPU time.
**      21-aug-95 (ramra01)
**          simple aggregate optimization cost output
**	21-dec-99 (inkdo01)
**	    Add support for hash join.
**	8-jan-04 (toumi01)
**	    Add support for exchange nodes.
**	27-feb-04 (inkdo01)
**	    Changed dsh_qen_status from ptr to array of QEN_STATUSs to 
**	    ptr to arrays of ptrs.
**	28-Sep-2004 (jenjo02)
**	    Added print of node number, fix duplicate
**	    printing of "at", print node_pcount whenever it's
**	    non-zero, parenthetically show child count on
**	    EXCH nodes.
**	1-Jul-2008 (kschendel)
**	    Fix up CPU overhead calculation, it's sufficiently inaccurate
**	    that negatives are easily printed.  Also use %u rather than %d.
[@history_template@]...
*/
static VOID
qeq_prqen(
PTR	p,
PTR	control )
{
    QEN_NODE	    *node = (QEN_NODE *) p;
    QEF_CB	    *qef_cb;
    QEE_DSH	    *dsh;
    QEN_STATUS	    *qen_status;
    i4		    node_ncost_calls;
    f8		    cost_cpu;
    char	    trbuf[ULD_PRNODE_LINE_WIDTH + 1];	/* TRformat buffer */
    CS_SID	    sid;

    CSget_sid(&sid);
    qef_cb = GET_QEF_CB(sid);

    dsh = (QEE_DSH *)qef_cb->qef_dsh;
    qen_status = dsh->dsh_xaddrs[node->qen_num]->qex_status;

    uld_prnode(control, "----------");

    /* Print the node number, useful for debugging */
    TRformat(uld_tr_callback, (i4 *)0,
	trbuf, sizeof(trbuf), "|%2d|",
	node->qen_num);
    uld_prnode(control, trbuf);

    switch (node->qen_type)
    {
     case QE_TJOIN:
	uld_prnode(control, "TJOIN     ");
	break;

     case QE_KJOIN:
	uld_prnode(control, "KJOIN     ");
	break;

     case QE_HJOIN:
	uld_prnode(control, "HJOIN    ");
	break;

     case QE_FSMJOIN:
	uld_prnode(control, "FSMJOIN   ");
	break;

     case QE_CPJOIN:
	uld_prnode(control, "CPJOIN   ");
	break;

     case QE_ORIG:
	uld_prnode(control, "ORIG      ");
	break;

     case QE_ORIGAGG:
	uld_prnode(control, "ORIGAGG   ");
	break;

     case QE_TPROC:
	uld_prnode(control, "TPROC     ");
	break;

     case QE_EXCHANGE:
	TRformat(uld_tr_callback, (i4 *)0,
	    trbuf, sizeof(trbuf), "EXCH(%d)",
	    node->node_qen.qen_exch.exch_ccount);
	uld_prnode(control, trbuf);
	break;

     case QE_ISJOIN:
	uld_prnode(control, "ISJOIN    ");
	break;

     case QE_SEJOIN:
	uld_prnode(control, "SEJOIN    ");
	break;

     case QE_SORT:
	uld_prnode(control, "SORT      ");
	break;

     case QE_TSORT:
	uld_prnode(control, "TSORT     ");
	break;

     case QE_QP:
	uld_prnode(control, "QP	       ");
	break;

     default:
	uld_prnode(control, "BAD NODE  ");
	break;
    }

    if (qen_status->node_nbcost != qen_status->node_necost)
    {
	uld_prnode(control, "Error:    ");
	uld_prnode(control, "Unmatched ");
	uld_prnode(control, "Cost Calls");
	uld_prnode(control, "          ");
	uld_prnode(control, "          ");
	uld_prnode(control, "          ");
	uld_prnode(control, "          ");
	uld_prnode(control, "          ");
	uld_prnode(control, "          ");
	uld_prnode(control, "          ");
    }
    else
    {
	/* get the cumulative cost call numbers */
	qeq_nccalls(dsh, node, &node_ncost_calls);
	cost_cpu = node_ncost_calls * dsh->dsh_cpuoverhead;
	if (cost_cpu >= qen_status->node_cpu)
	    cost_cpu = 0.0;
	else
	    cost_cpu = qen_status->node_cpu - cost_cpu;

	if (node->qen_type == QE_SORT)
	{
/* FIXME */
	    /* sort nodes (as opposed to tsort nodes) do not currently calculate
	    ** their node_rcount (number of tuples returned) correctly. This
	    ** is because the ISJOIN or FSMJOIN (the sort nodes parent) 
	    ** does the reading out
	    ** of the sort file directly, hence 
	    ** the sort node never gets a chance
	    ** to do this. Anyway, we'll print out blank lines here, so as not
	    ** to alarm people.
	    */
	    uld_prnode(control, "          ");
	    uld_prnode(control, "          ");
	}
	else
	{
	    TRformat(uld_tr_callback, (i4 *)0,
		trbuf, sizeof(trbuf), "at%9u",
		qen_status->node_rcount);
	    uld_prnode(control, trbuf);
	    TRformat(uld_tr_callback, (i4 *)0,
		trbuf, sizeof(trbuf), "et%9u", 
		node->qen_est_tuples);
	    uld_prnode(control, trbuf);
	    if ( qen_status->node_pcount > 0 )
	    {
		TRformat(uld_tr_callback, (i4 *)0,
		    trbuf, sizeof(trbuf), "ap%9u",
		    qen_status->node_pcount);
		uld_prnode(control, trbuf);
	    }
	}

	uld_prnode(control, "          ");
	TRformat(uld_tr_callback, (i4 *)0,
		trbuf, sizeof(trbuf), "ad%9u",
		(u_i4) (qen_status->node_dio));
	uld_prnode(control, trbuf);
	TRformat(uld_tr_callback, (i4 *)0,
		trbuf, sizeof(trbuf), "ed%9u",
		node->qen_dio_estimate);
	uld_prnode(control, trbuf);

	uld_prnode(control, "          ");
	TRformat(uld_tr_callback, (i4 *)0,
		trbuf, sizeof(trbuf), "ac%9u",
		(u_i4) (cost_cpu));
	uld_prnode(control, trbuf);
	TRformat(uld_tr_callback, (i4 *)0,
		trbuf, sizeof(trbuf), "ec%9u",
		node->qen_cpu_estimate);
	uld_prnode(control, trbuf);
	uld_prnode(control, "          ");
	TRformat(uld_tr_callback, (i4 *)0,
		trbuf, sizeof(trbuf), "et%9u",
		(u_i4) qen_status->node_wc);
	uld_prnode(control, trbuf);
    }
    uld_prnode(control, "----------");
}

/*{
** Name: QEQ_NCALLS	- Count the number of qen cost calls in a qen tree
**
** Description:
**	Count the total bcost calls below and including the input node.
**	This is for figuring the overhead in a QE90 display:  if some
**	node X uses N cpu-seconds between start and end, and the subplan
**	below it calls bcost B times, there's B+1 units of overhead.
**	(Overhead is computed at query start as the time for one B+E pair,
**	as bcost and ecost do pretty much the same thing.)
**
** Inputs:
[@PARAM_DESCR@]...
**
** Outputs:
[@PARAM_DESCR@]...
**
**	Returns:
**	    {@return_description@}
**	Exceptions:
**	    [@description_or_none@]
**
** Side Effects:
**	    [@description_or_none@]
**
** History:
**	21-dec-99 (inkdo01)
**	    Add support for hash joins.
**	8-jan-04 (toumi01)
**	    Add support for exchange nodes.
**	27-feb-04 (inkdo01)
**	    Changed dsh_qen_status from ptr to array of QEN_STATUSs to 
**	    ptr to arrays of ptrs.
**	1-Jul-2008 (kschendel) b122118
**	    This routine forgot to count anything!!!  fix.
[@history_template@]...
*/
VOID
qeq_nccalls(
QEE_DSH		*dsh,
QEN_NODE	*node,
i4		*ncost_calls )
{
    QEN_STATUS	    *qen_status;
    QEF_AHD	    *ahd;
    i4		    ahd_calls;
    i4		    outer_calls = 0;
    i4		    inner_calls = 0;

    *ncost_calls = 0;

    qen_status = dsh->dsh_xaddrs[node->qen_num]->qex_status;
    if (qen_status == NULL)
	return;
    *ncost_calls += qen_status->node_nbcost;

    switch (node->qen_type)
    {
     case QE_TJOIN:
	qeq_nccalls(dsh, node->node_qen.qen_tjoin.tjoin_out, &outer_calls);
	break;

     case QE_KJOIN:
	qeq_nccalls(dsh, node->node_qen.qen_kjoin.kjoin_out, &outer_calls);
	break;

     case QE_HJOIN:
	qeq_nccalls(dsh, node->node_qen.qen_hjoin.hjn_out, &outer_calls);
	qeq_nccalls(dsh, node->node_qen.qen_hjoin.hjn_inner, &inner_calls);
	break;

     case QE_ORIG:
     case QE_ORIGAGG:
	break;

     case QE_TPROC:
	break;

     case QE_EXCHANGE:
	qeq_nccalls(dsh, node->node_qen.qen_exch.exch_out, &outer_calls);
	break;

     case QE_ISJOIN:
     case QE_FSMJOIN:
     case QE_CPJOIN:
	qeq_nccalls(dsh, node->node_qen.qen_sjoin.sjn_out, &outer_calls);
	qeq_nccalls(dsh, node->node_qen.qen_sjoin.sjn_inner, &inner_calls);
	break;

     case QE_SEJOIN:
	qeq_nccalls(dsh, node->node_qen.qen_sejoin.sejn_out, &outer_calls);
	qeq_nccalls(dsh, node->node_qen.qen_sejoin.sejn_inner, &inner_calls);
	break;

     case QE_SORT:
	qeq_nccalls(dsh, node->node_qen.qen_sort.sort_out, &outer_calls);
	break;

     case QE_TSORT:
	qeq_nccalls(dsh, node->node_qen.qen_tsort.tsort_out, &outer_calls);
	break;

     case QE_QP:
	for (ahd = node->node_qen.qen_qp.qp_act;
		ahd != NULL;
		ahd = ahd->ahd_next
	    )
	if (ahd->qhd_obj.qhd_qep.ahd_qep)
	{
	    qeq_nccalls(dsh, ahd->qhd_obj.qhd_qep.ahd_qep, &ahd_calls);
	    outer_calls += ahd_calls;
	}
	break;

     default:
	/* error is reported elsewhere */
	return;
    }

    *ncost_calls += outer_calls + inner_calls;
}

/*
**  Name: QEQ_UNWIND_DSH_RESOURCES   - Release dsh resources
**				       for all stacked DSH's.
**
**  Description:
**	Marks the dmt_cb's as closed in the starting dsh's list of resources
**	and all DSH's above it in the stack.
**  Inputs:
**	dsh	pointer to starting dsh for which resources are to be released.
**  Output:
**      none
**
**  History:
**	19-Nov-1997 (jenjo02)
**	    Invented.
*/
VOID
qeq_unwind_dsh_resources(
QEE_DSH	    *dsh
)
{
    QEE_DSH	    *pdsh = dsh;

    while (pdsh != (QEE_DSH*)NULL)
    {
	qeq_release_dsh_resources(pdsh);
	pdsh = pdsh->dsh_stack;
    }
}

/*
**  Name: QEQ_RELEASE_DSH_RESOURCES   - Release dsh resources
**
**  Description:
**	Called after all tables and table procedures for a DSH's QP are
**	closed.  All DMT-CB's are cleaned to make sure they appear
**	closed (probably unnecessary, but whatever).  More importantly.
**	the DSH side of the QP resource list is cleaned up so that
**	all the resources are marked as not-opened, not-validated.
**
**  Inputs:
**	dsh	pointer to dsh for which resources are to be released.
**  Output:
**      none
**
**  History:
**	31-jan-94 (jhahn)
**	    Created (from qeq_validate)
**	18-Jun-2010 (kschendel) b123775
**	    DSH side of resource list reworked, reflect changes here.
**	    No more qee-valid list, and tprocs have info to clean up.
*/
VOID
qeq_release_dsh_resources(
QEE_DSH	    *dsh
)
{
    QEF_QP_CB       *qp = dsh->dsh_qp_ptr;
    DMT_CB	    *dmt_cb;
    QEE_RESOURCE    *resource;
    QEF_RESOURCE    *qpres;

    for(dmt_cb = dsh->dsh_open_dmtcbs;
	dmt_cb != (DMT_CB *)NULL;
	dmt_cb = (DMT_CB *) dmt_cb->q_next)
    {
	dmt_cb->dmt_record_access_id = (PTR)NULL;
    }
    dsh->dsh_open_dmtcbs = (DMT_CB *)NULL;

    /* for each resource */
    for (qpres = qp->qp_resources; qpres != NULL; qpres = qpres->qr_next)
    {
	resource = &dsh->dsh_resources[qpres->qr_id_resource];

	if (qpres->qr_type == QEQR_TABLE)
	{
	    resource->qer_resource.qer_tbl.qer_validated = FALSE;
	}
	else if (qpres->qr_type == QEQR_PROCEDURE)
	{
	    if (resource->qer_resource.qer_proc.qer_proc_dsh != NULL)
		qeq_release_dsh_resources(resource->qer_resource.qer_proc.qer_proc_dsh);
	    /* Don't null out qer-proc-dsh, might just be a commit and
	    ** the DSH is still there, just needs validated.
	    ** However we can clear used, either this isn't a tproc-using
	    ** action or the dsh has indeed been released.
	    */
	    resource->qer_resource.qer_proc.qer_procdsh_used = FALSE;
	    resource->qer_resource.qer_proc.qer_procdsh_valid = FALSE;
	}
    }
}

/*
** Name: qeq_close_dsh_nodes -- FUNC_CLOSE all QP nodes for a DSH
**
** Description:
**
**	This routine calls the CLOSE function for a query plan, by
**	walking through the QP actions and calling CLOSE on the
**	topmost node of any node-based action.  Normally this is
**	done at query cleanup time.  It's also done as a safeguard
**	against leaking memory when a dsh is about to be destroyed.
**	(This means that redundant CLOSE's can be done, which should
**	be innocuous.)
**
**
** Inputs:
**	dsh		The thread data segment header
**
** Outputs:
**	none
**
** History:
**	16-Feb-2006 (kschendel)
**	    Extract from qeq-cleanup.
**	11-May-2010 (kschendel)
**	    OPC now only sets NODEACT when there's really something there.
**      21-Sep-2010 (horda03) b124315
**          Check all the actions. The ahd_next list may "miss" actions
**          due to flow changes introduced by IF statements.
*/

void
qeq_close_dsh_nodes(QEE_DSH *dsh)
{

    QEF_AHD	    *action;
    QEN_NODE	    *node;
    QEF_RCB	    *rcb = dsh->dsh_qefcb->qef_rcb;

    for (action = dsh->dsh_qp_ptr->qp_ahd; 
	 action != NULL;
	 action = action->ahd_list)
    {
	if ( action->ahd_flags & QEA_NODEACT )
	{
	    node = action->qhd_obj.qhd_qep.ahd_qep;
	    (void)(*node->qen_func)(node, rcb, dsh, FUNC_CLOSE);
	}
    }
} /* qeq_close_dsh_nodes */

/*
** Name: qeq_cleanup	- cleanup a session's execution environment
**
** Description:
**	This routine cleans up the session's execution environment by:
**	    - if c_status is SUCCESS, 
**		    - close all open tables (so we can savepoint if MST).
**		    - destroy all temporary tables.
**		    - commit/savepoint the transaction.
**		    - handle deferred cursor flag in the session CB as 
**		      appropriate
**		    - mark all tables closed in the DSH.
**	    - if c_status is FAILURE
**		    - abort the transaction as appropriate.
**		    - handle deferred cursor flag in the session CB as 
**		      appropriate
**		    - marks all tables closed in the DSH.
**	    - issue a call to qee_cleanup to release/destroy the DSH
**	      if the DSH release flag is set.
**
**	If an error occurs in this routine, qef_error will be called.
**  
** Inputs:
**	qef_rcb			QEF request block
**	c_status		current status of calling routine
**	release			TRUE if DSH is to be released/destroyed
**
** Outputs:
**	qef_rcb
**	    .error.err_code
**	Returns:
**	    E_DB_{OK, WARN, ERROR, FATAL}
**	Exceptions:
**	    none
** Side effects:
**	none
** History:
**	20-jul-86 (daved)
**	    written
**	17-aug-87 (puree)
**	    if aborting a transaction, do not call dmt_close.  DMF will
**	    close all opend tables and delete all temp tables.
**	06-jan-88 (puree)
**	    set qp_status flag of the erroneous DSH to QEQP_OBSOLETE so
**	    it will be destroyed by qee_cleanup.
**	01-feb-88 (puree)
**	    always call qee_cleanup.
**	11-mar-88 (puree)
**	    remove DSH_QUEUE structure. DMT_CB's can be linked together
**	    using the standard jupiter control block header.
**	10-may-88 (puree)
**	    set DSH_QP_OBSOLETE in dsh_qp_status rather than in qp_status.  QP
**	    is read-only to QEF.
**	19-may-88 (puree)
**	    clear the dmt_record_access_id so we know that this table is
**	    closed.
**	01-jun-88 (puree)
**	    do not set DSH_QP_OBSOLETE in the dsh. This is for DB procedure
**	    support.  Now qeq_validate is the only one who decides if a QP
**	    is obsolete and should be destroyed or not.  User's error should
**	    not be the reason to destroy a QP.
**	15-sep-88 (puree)
**	    Fix bug breaking off double for loops.
**	05-oct-88 (puree)
**	    Modify transaction handling for invalid query.  If the caller's
**	    status is E_DB_WARNING and the error code is E_QE0023_INVALID_QUERY,
**	    return E_DB_ERROR.
**	23-mar-89 (paul)
**	    Corrected problem allocating multiple DSH's for a single QP.
**	    Upgraded transaction backout to handle rule cases.
**	21-dec-89 (neil)
**	    Check for NULL dsh in when cleaning up.  This routine expects there
**	    to be NULL dsh's (and checks for that).  Some references to the dsh
**	    were not checking for NULL.
**	16-sep-90 (carl)
**	    changed to abort if error
**	10-jan-90 (neil)
**	    Cleaned up some DSH handling for rules - turn off DSH_IN_USE bit
**	    only if doing a "release".
**	10-jun-91 (fpang)
**	    If aborting in qeq_cleanup(), drop temp tables first.
**	    Fixes B37196 and B36824.
**	12-feb-92 (nancy)
**	    Add new return from qeq_validate(), E_QE0301_TBL_SIZE_CHANGE to
**	    same handling as E_QE0023.  (bug 38565)
**	06-oct-92 (bonobo)
**	    Fixed duplicate definition of sav_rowcount.
**	06-jan-93 (teresa)
**	    Fix AV where it assumed DSH is available.  In fact it may not be,
**	    as in the case where qee_fetch encounters an error building the dsh.
**	24-mar-93 (jhahn)
**	    Various fixes for support of statement level rules. (FIPS)
**	29-jul-93 (rickh)
**	    At cleanup time, don't try to release objects anchored in the
**	    DSH unless there is a DSH.
**	31-jan-94 (jhahn)
**	    Fixed support for resource lists.
**	03-feb-94 (rickh)
**	    Re-cycle temporary tables at cleanup time.
**      15-feb-1994 (stevet)
**          Add call to adu_free_objects() to clean temp tables used
**          for handling BLOB. (B59267)
**      15-Apr-1994 (fred)
**          Altered call to adu_free_objects().  This routine now also
**          takes a statement number parameter, which is used to
**          delete temporaries from this statement only.
**	20-may-94 (eric)
**	    Corrected the way that we collect qe90 stats: used a greater
**	    sampling for stat collection overhead, and corrected for the
**	    that the OS only reports half of the cost to collect cpu stats.
**	28-oct-96 (somsa01)
**	    Moved adu_free_objects() from where it was to after
**	    qeq_closeTempTables(). This is because, after
**	    qeq_release_dsh_resources() was run, there were situations where
**	    adu_free_objects() would be using the dsh which pointed to an
**	    invalid location of memory (or perhaps pointing to an area of
**	    memory which was being used).
**	26-nov-96 (somsa01)
**	    Backed out change from change #427841; it is not needed by BLOBs,
**	    and it breaks the creation of star databases.
**	13-nov-97 (inkdo01)
**	    Dropped one "\n" and resurrected qe91.
**	19-Nov-1997 (jenjo02)
**	    Added qeq_unwind_dsh_resources() function to release resources for
**	    all DSH's in the stack.
**	18-mar-02 (inkdo01)
**	    Close any open sequences.
**	15-mar-04 (inkdo01)
**	    Turned into static function to cleanup each individual DSH (part
**	    of || query changes).
**	8-apr-04 (toumi01)
**	    Implement Doug's change to skip tables closed in orig.
**	15-Jul-2004 (jenjo02)
**	    Redid as external function. Kids are responsible for
**	    cleaning up their DSHs, so this function will only
** 	    be called on the "root" DSH.
**	01-Oct-2004 (jenjo02)
**	    Must FUNC_CLOSE all DSHs, follow dsh_next.
**	5-May-2005 (hanal04) Bug 113214 INGSRV2999
**          adu_free_object() will lead to dmt_delete_temp() calls which
**          reference the XCB. The XCB may be deallcoated during a DMX_ABORT
**          so cleanup the TX BLOB temps before calling DMX_ABORT.
**      20-Jul-2005 (hanal04 ) Bug 114885 INGSRV3355
**          Adjust the change for bug 113214 INGSRV2999. Calling
**          adu_free_objects() when we are the Star server causes us to 
**          SIGSEGV.
**	14-Feb-2006 (kschendel)
**	    Always close the QP nodes, not just if parallel plan.
**	    Only close the current QP, not all of 'em;  e.g. don't close
**	    caller DBP or query invoking a rule DBP!
**      01-Apr-2009 (gefei01)
**          Bug 121870: qeq_cleanup does qet_commit to abort to the last
**          internal savepoint even when open cursor count is non zero.
**	13-Apr-2010 (kschendel) SIR 123485
**	    Call new query-end routine in DMF to shut down blob processing.
**	    Statement abort was broken because we put the savepoint name in a
**	    local, then promptly exited the block containing the local!
**	    Who knows why that has ever worked.  Caused E_US087A.
**	18-Jun-2010 (kschendel) b123775
**	    Tproc DSH cleanup is now sent through here, so skip anything
**	    that tproc cleanup shouldn't do:  in particular, commit/abort.
**	06-sep-2010 (maspa05) SIR 124363
**	    Trace point sc925 - log long-running queries to errlog.log
**	07-sep-2010 (maspa05) SIR 124363
**	    Log to DBMS log instead of erlog.log.
**          Also added a 'ceiling' value to SC925.
**	08-sep-2010 (maspa05) SIR 124363
**	    Add Session ID to SC925 output
*/
DB_STATUS
qeq_cleanup(
QEF_RCB	    *qef_rcb,
DB_STATUS   c_status,
bool	    release )
{
    DB_ERROR	    error;
    QEF_CB	    *qef_cb = qef_rcb->qef_cb;
    QEE_DSH	    *dsh = (QEE_DSH *)qef_cb->qef_dsh;
    DMT_CB	    *dmt_cb;
    DB_STATUS	    status;
    i4	    err;
    TIMERSTAT	    tstat;
    QEF_AHD	    *ahd;
    QEF_SEQUENCE    *qseqp;
    bool            isTproc = FALSE;
    i4	    val1 = 0;
    i4	    val2 = 0;
    bool	    ddb_b;
    char	    *cbuf = qef_rcb->qef_cb->qef_trfmt;
    i4		    cbufsize = qef_rcb->qef_cb->qef_trsize;
    i4		    c_err = qef_rcb->error.err_code;
    i4              open_count = 0;

    if (qef_rcb->qef_cb->qef_c1_distrib & DB_3_DDB_SESS)
	ddb_b = TRUE;
    else
	ddb_b = FALSE;
    
    /* Save the original error code */
    STRUCT_ASSIGN_MACRO(qef_rcb->error, error);
    status = E_DB_OK;

    if (dsh != NULL)
    {
	qeq_close_dsh_nodes(dsh);

	isTproc = (dsh->dsh_qp_status & DSH_TPROC_DSH) != 0;
    }

    do
    {
	if (dsh != (QEE_DSH *)NULL &&	/* avoid AV if no DSH */
	   (dsh->dsh_qp_status & DSH_CPUSTATS_OFF) != 0)
	{
	    i4	    csret;
	    i4	    csoff;

	    csoff = 0;
	    csret = CSaltr_session((CS_SID) 0, CS_AS_CPUSTATS, ( PTR ) &csoff);

	    if (csret != OK && csret != E_CS001F_CPUSTATS_WASSET)
	    {
		qef_rcb->error.err_code = csret;
		return(E_DB_ERROR);
	    }
	}

	/* close all internal temporary tables */

	status = qeq_closeTempTables( dsh );
	if ( status != E_DB_OK )
	    return( status );

	if (DB_SUCCESS_MACRO(c_status))
	{
	    if ( ( !ddb_b ) && ( dsh != ( QEE_DSH * ) NULL ) )
	    {
		/* trace point sc925 - long-running queries */    
 		i4    lqry_thresh,lqry_ceil;
 
 	        ult_trace_longqry(&lqry_thresh,&lqry_ceil);
 	        if (lqry_thresh != 0 )
 		{
 		    i4 lqry_time;
 
 		    qen_bcost_begin(dsh, &tstat, NULL);
 		    lqry_time=tstat.stat_wc - dsh->dsh_twc;

 		    if ((lqry_time > lqry_thresh) && 
			((lqry_ceil == 0) || (lqry_time <lqry_ceil)))
		    {
		    /* get username,database and query text for error message */
 		        i4 error,qlen,erbuflen=0;
 		        DB_DB_NAME  dbname;
 		        DB_OWN_NAME dbuser;
 		        SCF_SCI         info[4];
 		        SCF_CB          scf_cb;
 		        DB_STATUS	    l_status;
			char        erbuf[ER_MAX_LEN];
			char        *qbuf,noqry[]="<query details unavailable>";
 
 		        info[0].sci_code = SCI_USERNAME;
 		        info[0].sci_length = sizeof(dbuser);
 		        info[0].sci_aresult = (char *) &dbuser;
 		        info[0].sci_rlength = 0;
 		        info[1].sci_code = SCI_DBNAME;
 		        info[1].sci_length = sizeof(dbname);
 		        info[1].sci_aresult = (char *) &dbname;
 		        info[1].sci_rlength = 0;
			info[2].sci_code = SCI_QBUF;
			info[2].sci_length = sizeof(qbuf);
			info[2].sci_aresult = (char *) &qbuf;
			info[2].sci_rlength = 0;
			info[3].sci_code = SCI_QLEN;
			info[3].sci_length = sizeof(qlen);
			info[3].sci_aresult = (char *) &qlen;
			info[3].sci_rlength = 0;
 		        scf_cb.scf_length = sizeof(SCF_CB);
 		        scf_cb.scf_type = SCF_CB_TYPE;
 		        scf_cb.scf_ascii_id = SCF_ASCII_ID;
 		        scf_cb.scf_facility = DB_QEF_ID;
 		        scf_cb.scf_session = DB_NOSESSION;
 		        scf_cb.scf_len_union.scf_ilength = 4;
 		        scf_cb.scf_ptr_union.scf_sci = (SCI_LIST *) &info[0];
 		        l_status = scf_call(SCU_INFORMATION, &scf_cb);
 		        if (l_status != E_DB_OK)
 		        {
 		            TRdisplay("qeq_cleanup:sc925 SCU_INFO call failed with status=%x\n",
 				    l_status);
 		            STcopy("<unknown>",dbname.db_db_name);
 		            STcopy("<unknown>",dbuser.db_own_name);
			    qlen=STlength(noqry);
			    qbuf=noqry;
 		        }
 
                        if (lqry_ceil == 0)
			{
 		          ule_format(I_QE3000_LONGRUNNING_QUERY, 
 				(CL_SYS_ERR *)0, ULE_LOOKUP, NULL,
 			        erbuf, ER_MAX_LEN,&erbuflen, &error, 4,
 				sizeof(lqry_time),&lqry_time,
 			        sizeof(lqry_thresh), &lqry_thresh,
 				DB_OWN_MAXNAME,dbuser.db_own_name,
 				DB_DB_MAXNAME,dbname.db_db_name);
			}
			else
			{
 		          ule_format(I_QE3001_LONGRUNNING_QUERY, 
 				(CL_SYS_ERR *)0, ULE_LOOKUP, NULL,
 			        erbuf, ER_MAX_LEN,&erbuflen, &error, 5,
 				sizeof(lqry_time),&lqry_time,
 			        sizeof(lqry_thresh), &lqry_thresh,
 			        sizeof(lqry_ceil), &lqry_ceil,
 				DB_OWN_MAXNAME,dbuser.db_own_name,
 				DB_DB_MAXNAME,dbname.db_db_name);
			}
			TRdisplay("[%x]%t\n",qef_cb->qef_ses_id,erbuflen,erbuf);
			TRdisplay("Query: %t\n",qlen,qbuf);

		    }
 
 		}
 
		/* if trace flag qe91 is set */
		if (ult_check_macro(&qef_cb->qef_trace, 91, &val1, &val2))
		{
		    qen_bcost_begin(dsh, &tstat, NULL);

		    dsh->dsh_tcpu = tstat.stat_cpu - dsh->dsh_tcpu;;
		    dsh->dsh_tdio = tstat.stat_dio - dsh->dsh_tdio;
		    dsh->dsh_twc = tstat.stat_wc - dsh->dsh_twc;

		    /* do not let TRformat see leading '\n' */
		    *cbuf = '\n';
		    TRformat(qeq_tr_callback, (i4 *)1,
				(cbuf + 1), cbufsize - 1,
				"Query Plan execution stats:\n");
        	    qec_tprintf(qef_rcb, cbufsize, cbuf);
		    *cbuf = '\n';
		    TRformat(qeq_tr_callback, (i4 *)1,
				(cbuf + 1), cbufsize - 1,
				" cpu: %d, dio: %d, et: %d\n",
				dsh->dsh_tcpu, dsh->dsh_tdio, dsh->dsh_twc);
        	    qec_tprintf(qef_rcb, cbufsize, cbuf);
		}

		/* if trace flag qe90 is set */
		if (ult_check_macro(&qef_cb->qef_trace, 90, &val1, &val2))
		{
		    /* then print out the actual and estimated CPU, DIO, and tuple
		    ** count stats for each QEN_NODE. The print out is in tree
		    ** form, so we use uld_prtree (naturally). Also, we do this
		    ** for each action that has a QEN_NODE tree.
		    */
		    for (ahd = dsh->dsh_qp_ptr->qp_ahd; 
				ahd != (QEF_AHD *) NULL; 
				ahd = ahd->ahd_next
			)
		    {
			switch (ahd->ahd_atype)
			{
			case QEA_APP:
			case QEA_SAGG:
			case QEA_PROJ:
			case QEA_AGGF:
			case QEA_UPD:
			case QEA_DEL:
			case QEA_GET:
			case QEA_RUP:
			case QEA_RAGGF:
			case QEA_LOAD:
			case QEA_PUPD:
			case QEA_PDEL:
			case QEA_RDEL:
			    if (ahd->qhd_obj.qhd_qep.ahd_qep != NULL)
			    {
				uld_prtree((PTR) ahd->qhd_obj.qhd_qep.ahd_qep,
				    		qeq_prqen, qeq_lqen, qeq_rqen, 
							    10, 13, DB_QEF_ID);
			    }
			    break;

			default:
			    break;
			}
		    }
		}

		/* Close all sequences. */
		for (qseqp = dsh->dsh_qp_ptr->qp_sequences; qseqp != NULL; 
							qseqp = qseqp->qs_qpnext)
		{
		    DMS_CB	*dms_cb;

		    dms_cb = (DMS_CB *)dsh->dsh_cbs[qseqp->qs_cbnum];
		    status = dmf_call(DMS_CLOSE_SEQ, dms_cb);
		    if (status)
		    {
			qef_error(dms_cb->error.err_code, 0L, status, &err,
			    &qef_rcb->error, 0);
		    }
		}
		/* Before nailing all open tables, tell DMF that the query
		** is coming to an end.  Indicate whether the query ended
		** OK or not, and (unless an outer level of the query is
		** still in progress) drop all LOB holding temps.
		** Note that indicating query-end is a valid thing to do
		** even if the query isn't really ending, because all tables
		** are going to be closed.  If the query gets going again,
		** DMF can rebuild its LOB context and other needed things.
		** The only really critical thing to hang on to is in-flight
		** LOB's in holding temps.
		*/
		if (!isTproc && qef_cb->qef_open_count == 0)
		{
		    DMR_CB dmrcb;
		    dmrcb.type = DMR_RECORD_CB;
		    dmrcb.length = sizeof(DMR_CB);
		    dmrcb.dmr_flags_mask = 0;
		    if (c_status == E_DB_ERROR)
			dmrcb.dmr_flags_mask |= DMR_QEND_ERROR;
		    if (dsh->dsh_stack == NULL
		      && !(c_status == E_DB_WARN && 
			   (c_err == E_QE0023_INVALID_QUERY || 
			    c_err == E_QE0301_TBL_SIZE_CHANGE)))
		    {
			dmrcb.dmr_flags_mask |= DMR_QEND_FREE_TEMPS;
		    }
		    status = dmf_call(DMPE_QUERY_END, &dmrcb);
		    if (status != E_DB_OK)
		    {
			qef_error(dmrcb.error.err_code, 0L, status,
				  &err, &qef_rcb->error, 0);
			if (status > c_status)
			{
			    STRUCT_ASSIGN_MACRO(qef_rcb->error, error);
			    c_status = status;
			}
		    }
		}

		/* close all open tables.  also destroy all temp tables. */

		for (dmt_cb = dsh->dsh_open_dmtcbs; dmt_cb; 
		        dmt_cb = (DMT_CB *)(dmt_cb->q_next))
		{
		    if (dmt_cb->dmt_record_access_id == (PTR) NULL)
			continue;

		    /* close the table */
		    status = dmf_call(DMT_CLOSE, dmt_cb);
		    if (status)
		    {
			qef_error(dmt_cb->error.err_code, 0L, status, &err,
			    &qef_rcb->error, 0);
			if (status > c_status)
			{
			    STRUCT_ASSIGN_MACRO(qef_rcb->error, error);
			    c_status = status;
			    break;	    /* break off to error handling section */
			}
		    }

		    /* mark the table closed */
		    dmt_cb->dmt_record_access_id = (PTR)NULL;

		    /* destroy if it's a temporary table */
		    if (TEMP_TABLE_MACRO(dmt_cb))
		    {
			dmt_cb->dmt_flags_mask = 0;
			status = dmf_call(DMT_DELETE_TEMP, dmt_cb);
			if (status)
			{
			    if (dmt_cb->error.err_code == 
				E_DM005D_TABLE_ACCESS_CONFLICT)
			    {
				status = E_DB_OK; /* Must be in use by calling
						  ** query */
			    }
			    else
			    {
				qef_error(dmt_cb->error.err_code, 0L, status,
					  &err, &qef_rcb->error, 0);
				if (status > c_status)
				{
				    STRUCT_ASSIGN_MACRO(qef_rcb->error, error);
				    c_status = status;
				    /* break off to error handling section */
				    break;  
				}
			    }
			}
		    }
		}

		/* Mark DSH's resource list, nothing is open */
		qeq_release_dsh_resources(dsh);

	    } /* if dsh */

	    if (status)
		break;

	    /* Commit/savepoint the transaction
	    **  -  For a SST, commit the transaction 
	    **  -  For a MST, declare an internal savepoint if no open cursor.
	    ** But never commit or savepoint if this is a tproc DSH.
	    */
	    status = E_DB_OK;

	    if (qef_cb->qef_stat == QEF_SSTRAN && qef_cb->qef_open_count == 0
	      && ! isTproc)
	    {
		status = qet_commit(qef_cb);
	    }
	    else if (qef_cb->qef_stat == QEF_MSTRAN && 
		     qef_cb->qef_open_count == 0 && 
		     !isTproc &&
		     !ddb_b)			/* no distributed internal sp */
	    {
		qef_rcb->qef_spoint = &Qef_s_cb->qef_sp_savepoint;
		status = qet_savepoint(qef_cb);
	    }
	    if (DB_FAILURE_MACRO(status) && 
		qef_rcb->error.err_code != E_QE0025_USER_ERROR)
	    {
		qef_error(qef_rcb->error.err_code, 0L, status, &err,
		    &qef_rcb->error, 0);
		if (status > c_status)
		{
		    STRUCT_ASSIGN_MACRO(qef_rcb->error, error);
		    c_status = status;
		    break;
		}
	    }
	}
    } while (FALSE);

    if (DB_FAILURE_MACRO(c_status))	/* check again in case the error comes
					** from the above operation */
    {
	/* 
	** If we are inside a rule action list, do not abort the
	** transaction. The transaction will be aborted when the outermost
	** query is aborted. This forces the transaction to be backed out
	** only when aborting the outermost query. If we tried to abort
	** while in rule processing we would always have to abort the
	** entire transaction since there will always be more than one
	** query open (the original query and the rule action).
	*/
	if (qef_cb->qef_stat != QEF_NOTRAN && qef_rcb->qef_rule_depth == 0
	  && !isTproc)
	{
	    if (ddb_b)		/* distributed? */
	    {
		qef_rcb->qef_spoint = NULL;
	        /* Drop temp tables */
	        if (dsh && (dsh->dsh_qp_ptr->qp_ddq_cb.qeq_d3_elt_cnt > 0) )
	        {
	            i4	  sav_rowcount;
		    i4	  sav_count;
		    DB_STATUS sav_status;
		    DB_ERROR  sav_error;

	            /* Star must save the state and drop all temporary tables */

	            sav_status = status;
	            STRUCT_ASSIGN_MACRO(qef_rcb->error, sav_error);
	            sav_rowcount = qef_rcb->qef_rowcount;
		    sav_count = qef_rcb->qef_count;
						/* save output row count */
	            status = qeq_d6_tmp_act(qef_rcb, dsh);
	            if (status)
	            {
		        /* save new error state */
    
		        sav_status = status;
		        STRUCT_ASSIGN_MACRO(qef_rcb->error, sav_error);
	            }

						/* restore output row count */
	            qef_rcb->qef_rowcount = sav_rowcount;	
	            qef_rcb->qef_count = sav_count;	
	            dsh->dsh_qp_ptr->qp_ddq_cb.qeq_d3_elt_cnt = 0;
	        }
	    }

	    /* 
	    ** If the transaction is MST and there is no open cursor, abort to
	    ** the last internal savepoint.  For other transaction type, or if
	    ** there is an open cursor, abort the entire transaction.
	    */
	    else if ((qef_cb->qef_stat != QEF_MSTRAN) || 
		(qef_cb->qef_open_count != 0))
	    {
		qef_rcb->qef_spoint = NULL;
	    }
	    else /* MST with no open cursors only */
	    {
		qef_rcb->qef_spoint = &Qef_s_cb->qef_sp_savepoint;
	    }
	    /* Could in theory let the abort do this from inside DMF, but
	    ** it's cleaner to indicate end-of-query explicitly.
	    */
	    if (qef_cb->qef_open_count == 0 && ( !ddb_b ) && ( dsh != NULL ) )
	    {
		DMR_CB dmrcb;
		dmrcb.type = DMR_RECORD_CB;
		dmrcb.length = sizeof(DMR_CB);
		dmrcb.dmr_flags_mask = DMR_QEND_ERROR;
		if (dsh->dsh_stack == NULL)
		{
		    dmrcb.dmr_flags_mask |= DMR_QEND_FREE_TEMPS;
		}
		status = dmf_call(DMPE_QUERY_END, &dmrcb);
		if (status != E_DB_OK)
		{
		    qef_error(dmrcb.error.err_code, 0L, status,
			      &err, &qef_rcb->error, 0);
		    if (status > c_status)
		    {
			STRUCT_ASSIGN_MACRO(qef_rcb->error, error);
			c_status = status;
		    }
		}
	    }
	    status = qet_abort(qef_cb);
	    if (DB_FAILURE_MACRO(status) && 
		qef_rcb->error.err_code != E_QE0025_USER_ERROR)
	    {
		qef_error(qef_rcb->error.err_code, 0L, status, &err,
		    &qef_rcb->error, 0);
		STRUCT_ASSIGN_MACRO(qef_rcb->error, error);
		c_status = status;
	    }
	}

	if (!ddb_b && dsh != (QEE_DSH *)NULL)
	{
	    /* Mark all tables closed */
	    qeq_unwind_dsh_resources(dsh);
	}
    }

    /* call qee_cleanup to release/destroy the DSH, if required */
    if (release)
    {
	/* Set the DSH to no longer in use (confirm that DSH is not NULL) */
	if (dsh != (QEE_DSH *)NULL)
	    dsh->dsh_qp_status &= ~DSH_IN_USE;

	status = qee_cleanup(qef_rcb, &dsh);
	if (DB_FAILURE_MACRO(status))
	{
	    qef_error(qef_rcb->error.err_code, 0L, status, &err,
		      &qef_rcb->error, 0);
	    if (status > c_status)
	    {
		STRUCT_ASSIGN_MACRO(qef_rcb->error, error);
		c_status = status;
	    }
	}
    }
    STRUCT_ASSIGN_MACRO(error, qef_rcb->error);
    if (c_status == E_DB_WARN && 
        (error.err_code == E_QE0023_INVALID_QUERY ||
	 error.err_code == E_QE0301_TBL_SIZE_CHANGE))
	c_status = E_DB_ERROR;

    return (c_status);
}

/*{
** Name: qeq_closeTempTables - close internally generated temp tables
**
**  Internal QEF call:	    status = qeq_closeTempTables( qef_rcb );
**
** Description:
**	This routine closes all the internally generated temporary tables
**	in a query plan.
**
**
**
** Inputs:
**      dsh
**
** Outputs:
**				zero.
**	Returns:
**	    E_DB_{OK,WARN,ERROR,FATAL}
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	03-feb-94 (rickh)
**	    Created.
**
*/
DB_STATUS
qeq_closeTempTables(
	QEE_DSH		*dsh
)
{
    DB_STATUS	    status = E_DB_OK;
    QEF_QP_CB	    *qp;
    i4		    numberOfTempTables;
    QEE_TEMP_TABLE  **tempTables;
    i4		    i;

    /* sanity checks. only release temp tables that exist */

    if ( dsh == ( QEE_DSH * ) NULL )	{ return( status ); }

    qp = dsh->dsh_qp_ptr;
    numberOfTempTables = qp->qp_tempTableCount;
    tempTables = dsh->dsh_tempTables;
    if (   ( numberOfTempTables <= 0 )
	|| ( tempTables == ( QEE_TEMP_TABLE **) NULL ) )  { return( status ); }

    /*
    ** Now loop through the internal temporary tables, destroying them as
    ** we go.  If a table has already been destroyed, it doesn't hurt to
    ** call this routine again.
    */

    for ( i = 0; i < numberOfTempTables; i++ )
    {
	status = qen_destroyTempTable( dsh, i );
	if ( status != E_DB_OK )	{ return( status ); }
    }

    return( status );
}

/*{
** Name: QEQ_ENDRET	- a set query is ended prematurely
**
**  External QEF call:	    status = qef_call(QEQ_ENDRET, &qef_rcb);
**
** Description:
**	This routine should be called while the application program
**  is in a retrieve loop. This routine interrupts the execution of the
**  retrieve, closes any tables, and adjusts the transaction state
**  as appropriate.
**
**  If the QP is not opened, it is an error.
**
** Inputs:
**      qef_rcb
**	    .qef_eflag		designate error handling semantics
**				for user errors.
**		QEF_INTERNAL	return error code.
**		QEF_EXTERNAL	send message to user.
**	    .qef_cb		session control block
**	    .qef_d_id		dmf session id
**	    .qef_db_id		database id
**	    .qef_qp		name of query plan
**	    .qef_qso_handle	internal QP id. Used instead of qef_qp if not
**				zero.
**
** Outputs:
**      qef_rcb
**	    .error.err_code	one of the following
**				E_QE0000_OK
**				E_QE0017_BAD_CB
**				E_QE0018_BAD_PARAM_IN_CB
**				E_QE0019_NON_INTERNAL_FAIL
**				E_QE0002_INTERNAL_ERROR
**				E_QE0014_NO_QUERY_PLAN
**				E_QE0004_NO_TRANSACTION
**				E_QE000E_ACTIVE_COUNT_EXCEEDED
**				E_QE000F_OUT_OF_OTHER_RESOURCES
**				E_QE0024_TRANSACTION_ABORTED
**				E_QE0034_LOCK_QUOTA_EXCEEDED
**				E_QE0035_LOCK_RESOURCE_BUSY
**				E_QE0036_LOCK_TIMER_EXPIRED
**				E_QE002A_DEADLOCK
**	Returns:
**	    E_DB_{OK,WARN,ERROR,FATAL}
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	13-apr-87 (daved)
**          written
**	11-mar-88 (puree)
**	    remove DSH_QUEUE structure. DMT_CB's can be linked together
**	    using the standard jupiter control block header.
**	19-may-88 (puree)
**	    clear the dmt_record_access_id so we know that this table is
**	    closed.
**	20-feb-89 (paul)
**	    Updated parameters to qeq_dsh call for rules processing.
**	10-oct-88 (carl)
**	    Modified for Titan
**	01-oct-1992 (fpang)
**	    Save qef_count around qeq_d6_tmp_act() call.
**      12-feb-93 (jhahn)
**          Added support for statement level rules. (FIPS)
**	11-nov-1998 (somsa01)
**	    We need to make sure that the continuation flag is reset.
**	    (Bug #94114)
**      18-Aug-2010 (maspa05) b124108
**          If we call qeq_dsh to retrieve the same DSH that we already have
**          in qef_dsh then we will have locked the corresponding QP twice
**          therefore unlock it so that we don't accumulate locks and 
**          consequently never free the QP.
*/
DB_STATUS
qeq_endret(
QEF_RCB		*qef_rcb )
{
    DB_STATUS		status, sav_status = E_DB_OK;
    DB_ERROR		sav_error;
    QEF_CB		*qef_cb = qef_rcb->qef_cb;
    QEE_DSH		*dsh;

    qef_cb->qef_rcb = qef_rcb;

    /* search the list of active DSHs for the specified one */

    if (status = qeq_dsh(qef_rcb, 1, &dsh, QEQDSH_NORMAL, -1))
	return (status);    /* won't go very far without DSH */

    /* if we just retrieved the same dsh that we already have then
     * unlock it (we'll have locked it twice) */

    if (dsh == (QEE_DSH *)qef_cb->qef_dsh)
    {
        QSF_RCB	qsf_rcb;

        qsf_rcb.qsf_next = (QSF_RCB *)NULL;
        qsf_rcb.qsf_prev = (QSF_RCB *)NULL;
        qsf_rcb.qsf_length = sizeof(QSF_RCB);
        qsf_rcb.qsf_type = QSFRB_CB;
        qsf_rcb.qsf_lk_state = QSO_SHLOCK;
        qsf_rcb.qsf_owner = (PTR)DB_QEF_ID;
        qsf_rcb.qsf_ascii_id = QSFRB_ASCII_ID;
        qsf_rcb.qsf_sid = qef_rcb->qef_cb->qef_ses_id;
        qsf_rcb.qsf_obj_id.qso_handle = qef_rcb->qef_qso_handle;
	qsf_rcb.qsf_obj_id.qso_type = QSO_QP_OBJ;
	qsf_rcb.qsf_obj_id.qso_lname = sizeof (DB_CURSOR_ID);
	MEcopy((PTR)&qef_rcb->qef_qp, sizeof (DB_CURSOR_ID), 
	    (PTR)qsf_rcb.qsf_obj_id.qso_name);

	/* can't unlock without a handle */
	if (!qsf_rcb.qsf_obj_id.qso_handle)
	{
            status=qsf_call(QSO_GETHANDLE, &qsf_rcb);   
	    if (status)
	    {
	       qef_rcb->error.err_code = qsf_rcb.qsf_error.err_code;
	       return (status);
	    }
	    /* gethandle adds another lock! so unlock for that */
            status=qsf_call(QSO_UNLOCK, &qsf_rcb);   
	    if (status)
	    {
	       qef_rcb->error.err_code = qsf_rcb.qsf_error.err_code;
	       return (status);
	    }
	}

	status=qsf_call(QSO_UNLOCK, &qsf_rcb); 
	if (status)
	{
	    qef_rcb->error.err_code = qsf_rcb.qsf_error.err_code;
	    return (status);
	}
    }

    /* update count of open query/cursor */
    qef_cb->qef_open_count--;

    /* Handle deferred cursor flag.  Needed here in case qeq_cleanup calls
    ** qet_savepoint rather than qet_commit/qet_abort. 
    ** Only one deferred cursor a time allowed in a session.  So if this one 
    ** is a deferred cursor, mark no more deferred cursor in the session cb */

    if (dsh->dsh_qp_status & DSH_DEFERRED_CURSOR)
	    qef_cb->qef_defr_curs = FALSE;

    if (qef_cb->qef_c1_distrib & DB_3_DDB_SESS)
    {
	status = qeq_d3_end_act(qef_rcb);

	if (dsh->dsh_qp_ptr->qp_ddq_cb.qeq_d3_elt_cnt > 0)
	{
	    i4	sav_rowcount;
	    i4	sav_count;

	    /* Star must save the state and drop all temporary tables */

	    sav_status = status;
	    STRUCT_ASSIGN_MACRO(qef_rcb->error, sav_error);
	    sav_rowcount = qef_rcb->qef_rowcount;
	    sav_count = qef_rcb->qef_count;
						/* save output row count */
	    status = qeq_d6_tmp_act(qef_rcb, dsh);
	    if (status)
	    {
		/* save new error state */

		sav_status = status;
		STRUCT_ASSIGN_MACRO(qef_rcb->error, sav_error);
	    }

	    qef_rcb->qef_rowcount = sav_rowcount;	
	    qef_rcb->qef_count = sav_count;	
						/* restore output row count */
	}
	else
	{
	    /* save state */
	    sav_status = status;
	    STRUCT_ASSIGN_MACRO(qef_rcb->error, sav_error);
	}

	/* must present clean state to qeq_cleanup */

	status = E_DB_OK;
	qef_rcb->error.err_code = E_QE0000_OK;
    }

    /*
    ** We need to make sure that the continuation flag is reset to zero.
    */
    if ((dsh->dsh_act_ptr) && (dsh->dsh_act_ptr->ahd_atype == QEA_GET) &&
	(dsh->dsh_act_ptr->qhd_obj.qhd_qep.ahd_current))
    {
	QEN_ADF  *qen_adf = dsh->dsh_act_ptr->qhd_obj.qhd_qep.ahd_current;
	ADE_EXCB *ade_excb = (ADE_EXCB*) dsh->dsh_cbs[qen_adf->qen_pos];

	ade_excb->excb_continuation = 0;
    }

    status = qeq_cleanup(qef_rcb, status, TRUE);
    if (sav_status)
    {
	STRUCT_ASSIGN_MACRO(sav_error, qef_rcb->error);
	return(sav_status);
    }

    return (status);
}   

/*{
** Name: QEQ_CHK_RESOURCE   - check resource limits for query execution
**
** Description:
**	This procedure checks the query limits in the action header against
**	the limits specified in the session's QEF_CB. The QEF_CB limits
**	are the limits granted to the user for the current session.
**
**	Presently, this procedure only checks limits in query processing
**	actions. That is, those containing a QEF_QEP structure.
**	The actions for which checking is done are:
**
**	    QEA_SAGG
**	    QEA_PROJ
**	    QEA_AGGF
**	    QEA_APP
**	    QEA_UPD
**	    QEA_DEL
**	    QEA_GET
**	    QEA_PUPD
**	    QEA_PDEL
**
**	The rules for application of resource checking are:
**
**	    Resource checking applies only to the first QEP action of a
**	    statement. ahd_estimates == 1 if this is the first node and
**	    ahd_estimates == 0 if this is other than the first node.
**	    To improve performance the caller of this routine is expected to
**	    check this condition.
**
**	    Resource checking only applies if the values in the QEF_CB are
**	    non-zero. To improve performance the caller of this routine
**	    is expected to check this condition. The field, qef_fl_dbpriv,
**	    also contains flags indicating which resource limits are set.
**
**	    The returned row estimate (ahd_row_estimate) is only set by OPC
**	    if this statement results in rows being returned to the user.
**	    Thus, insert, update, delete and select as statements are not
**	    constrained by this limit.
**
** Inputs:
**	qef_rcb			QEF_RCB for this query
**	action			Action to be checked
**
** Outputs:
**	qef_rcb.error.err_code
**				E_QE0202_RESOURCE_CHCK_INVALID
**				E_QE0203_DIO_LIMIT_EXCEEDED
**				E_QE0204_ROW_LIMIT_EXCEEDED
**				E_QE0205_CPU_LIMIT_EXCEEDED
**				E_QE0206_PAGE_LIMIT_EXCEEDED
**				E_QE0207_COST_LIMIT_EXCEEDED
**		
**	Returns:
**	    E_DB_{OK,ERROR}
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	30-MAR-89 (paul)
**	    First written to support query resource control.
**	02-jul-91 (nancy)
**	    bug 36834 -- add QEA_LOAD to action types checked so that resource
**	    control can be applied to create table as select.
**	12-aug-93 (robf)
**	    Add resource auditing, write audit records whenever a resource
**	    limit is hit.
**	21-mar-96 (inkdo01)
**	    Minor corrections to qef_error parameter lists.
**	28-feb-04 (inkdo01)
**	    Change parms to DSH from RCB (for thread safety).
**	09-apr-10 (smeke01) b123554
**	    Corrected typo for MAXPAGE that had adh_cost_estimate instead of
**	    ahd_page_estimate.
*/
DB_STATUS
qeq_chk_resource(
QEE_DSH		*dsh,
QEF_AHD		*action )
{
    DB_STATUS		status = E_DB_OK;
    QEF_CB		*qef_cb = dsh->dsh_qefcb;
    i4		err;
        
    /* Check that the action is valid.					    */
    /* If it is, apply the resource checks */
    switch (action->ahd_atype)
    {
      case QEA_GET:
	if (qef_cb->qef_fl_dbpriv & DBPR_QROW_LIMIT &&
	    qef_cb->qef_row_limit < action->qhd_obj.qhd_qep.ahd_row_estimate)
	{
	    if ( Qef_s_cb->qef_state & QEF_S_C2SECURE )
	    {
		status =qeu_secaudit_detail( 
			    FALSE,
			    qef_cb->qef_ses_id,
			    "ROW_LIMIT",
			    NULL,
			    9,
			    SXF_E_RESOURCE,
			    I_SX2731_QUERY_ROW_LIMIT,
			    SXF_A_FAIL|SXF_A_EXECUTE,
			    &dsh->dsh_error,
			    NULL,
			    0,
			    qef_cb->qef_row_limit);
		if (status != E_DB_OK)
		{
		    break;
		}
	    }
	    status = E_DB_ERROR;
	    qef_error(E_QE0204_ROW_LIMIT_EXCEEDED, 0L, status, &err,
		      &dsh->dsh_error, 2,
		      sizeof(action->qhd_obj.qhd_qep.ahd_row_estimate),
		      &action->qhd_obj.qhd_qep.ahd_row_estimate,
		      sizeof(qef_cb->qef_row_limit),
		      &qef_cb->qef_row_limit);
	    dsh->dsh_error.err_code = E_QE0122_ALREADY_REPORTED;
	    break;
	}
      /* Fall through to process remaining query limits */
      
      case QEA_APP:
      case QEA_SAGG:
      case QEA_PROJ:
      case QEA_AGGF:
      case QEA_UPD:
      case QEA_DEL:
      case QEA_PDEL:
      case QEA_PUPD:
      case QEA_LOAD:
	if (qef_cb->qef_fl_dbpriv & DBPR_QDIO_LIMIT &&
	    qef_cb->qef_dio_limit < action->qhd_obj.qhd_qep.ahd_dio_estimate)
	{
	    if ( Qef_s_cb->qef_state & QEF_S_C2SECURE )
	    {
		status =qeu_secaudit_detail( 
			    FALSE,
			    qef_cb->qef_ses_id,
			    "DIO_LIMIT",
			    NULL,
			    9,
			    SXF_E_RESOURCE,
			    I_SX2733_QUERY_DIO_LIMIT,
			    SXF_A_FAIL|SXF_A_EXECUTE,
			    &dsh->dsh_error,
			    NULL,
			    0,
			    qef_cb->qef_dio_limit);
		if (status != E_DB_OK)
		{
		    break;
		}
	    }
	    status = E_DB_ERROR;
	    qef_error(E_QE0203_DIO_LIMIT_EXCEEDED, 0L, status, &err,
		      &dsh->dsh_error, 2,
		      sizeof(action->qhd_obj.qhd_qep.ahd_dio_estimate),
		      &action->qhd_obj.qhd_qep.ahd_dio_estimate,
		      sizeof(qef_cb->qef_dio_limit),
		      &qef_cb->qef_dio_limit);
	    dsh->dsh_error.err_code = E_QE0122_ALREADY_REPORTED;
	    break;
	}
	if (qef_cb->qef_fl_dbpriv & DBPR_QCOST_LIMIT &&
	    qef_cb->qef_cost_limit < action->qhd_obj.qhd_qep.ahd_cost_estimate)
	{
	    if ( Qef_s_cb->qef_state & QEF_S_C2SECURE )
	    {
		status =qeu_secaudit_detail( 
			    FALSE,
			    qef_cb->qef_ses_id,
			    "COST_LIMIT",
			    NULL,
			    10,
			    SXF_E_RESOURCE,
			    I_SX2735_QUERY_COST_LIMIT,
			    SXF_A_FAIL|SXF_A_EXECUTE,
			    &dsh->dsh_error,
			    NULL,
			    0,
			    qef_cb->qef_cost_limit);
		if (status != E_DB_OK)
		{
		    break;
		}
	    }
	    status = E_DB_ERROR;
	    qef_error(E_QE0207_COST_LIMIT_EXCEEDED, 0L, status, &err,
		      &dsh->dsh_error,2,
		      sizeof(action->qhd_obj.qhd_qep.ahd_cost_estimate),
		      &action->qhd_obj.qhd_qep.ahd_cost_estimate,
		      sizeof(qef_cb->qef_cost_limit),
		      &qef_cb->qef_cost_limit);
	    dsh->dsh_error.err_code = E_QE0122_ALREADY_REPORTED;
	    break;
	}
	if (qef_cb->qef_fl_dbpriv & DBPR_QCPU_LIMIT &&
	    qef_cb->qef_cpu_limit < action->qhd_obj.qhd_qep.ahd_cpu_estimate)
	{
	    if ( Qef_s_cb->qef_state & QEF_S_C2SECURE )
	    {
		status =qeu_secaudit_detail( 
			    FALSE,
			    qef_cb->qef_ses_id,
			    "CPU_LIMIT",
			    NULL,
			    9,
			    SXF_E_RESOURCE,
			    I_SX2732_QUERY_CPU_LIMIT,
			    SXF_A_FAIL|SXF_A_EXECUTE,
			    &dsh->dsh_error,
			    NULL,
			    0,
			    qef_cb->qef_cpu_limit);
		if (status != E_DB_OK)
		{
		    break;
		}
	    }
	    status = E_DB_ERROR;
	    qef_error(E_QE0205_CPU_LIMIT_EXCEEDED, 0L, status, &err,
		      &dsh->dsh_error,2,
		      sizeof(action->qhd_obj.qhd_qep.ahd_cpu_estimate),
		      &action->qhd_obj.qhd_qep.ahd_cpu_estimate,
		      sizeof(qef_cb->qef_cpu_limit),
		      &qef_cb->qef_cpu_limit);
	    dsh->dsh_error.err_code = E_QE0122_ALREADY_REPORTED;
	    break;
	}
	if (qef_cb->qef_fl_dbpriv & DBPR_QPAGE_LIMIT &&
	    qef_cb->qef_page_limit < action->qhd_obj.qhd_qep.ahd_page_estimate)
	{
	    if ( Qef_s_cb->qef_state & QEF_S_C2SECURE )
	    {
		status =qeu_secaudit_detail( 
			    FALSE,
			    qef_cb->qef_ses_id,
			    "PAGE_LIMIT",
			    NULL,
			    10,
			    SXF_E_RESOURCE,
			    I_SX2734_QUERY_PAGE_LIMIT,
			    SXF_A_FAIL|SXF_A_EXECUTE,
			    &dsh->dsh_error,
			    NULL,
			    0,
			    qef_cb->qef_page_limit);
		if (status != E_DB_OK)
		{
		    break;
		}
	    }
	    status = E_DB_ERROR;
	    qef_error(E_QE0206_PAGE_LIMIT_EXCEEDED, 0L, status, &err,
		      &dsh->dsh_error,2,
		      sizeof(action->qhd_obj.qhd_qep.ahd_page_estimate),
		      &action->qhd_obj.qhd_qep.ahd_page_estimate,
		      sizeof(qef_cb->qef_page_limit),
		      &qef_cb->qef_page_limit);
	    dsh->dsh_error.err_code = E_QE0122_ALREADY_REPORTED;
	    break;
	}
	break;

      default:
	status = E_DB_ERROR;
	dsh->dsh_error.err_code = E_QE0202_RESOURCE_CHCK_INVALID;
    }
    return (status);
}

/*{
** Name: QEQ_RESTORE            - Restore old QP's context
**
** Descripton:
**      For QEA_EXEC_IMM action, we might return with a QEA_DMU
**      action. After the QEA_DMU action has been processed, we need to
**      continue with the next QEA_EXEC_IMM action of the main QP. For this, we 
**      need to restore the old QP's context.
**
** Inputs:
**      qeq_rcb               QEF request control block
**         .qef_pcount         number of set of input parameters
**         .qef_usr_param      ptr to an array of pointers to QEF_USR_PARAM
**         .qef_qp             name of query plan
**         .qef_targcount      number of rows touched in query
**         .qef_output         where to put result information
**         .qef_count          number of output buffers filled
**         .qef_val_logkey     valid logical key
**      qef_rcb                output QEF request control bock
**      dsh                    current dsh
**         .dsh_act_ptr        pointer to current action
**         .dsh_qp_ptr         pointer to corresponding QP
**         .cbs                address of array of pointers to various control
**                             blocks
**      qp                   will be set to point to the current QP
**      cbs                  will be set to point to address of array of
**                           pointers to various control blocks
**      act                  will be set to next action to execute
**      rowcount             will be set to number of rows desired
**
** Outputs:
**      qef_rcb
**          .error.err_code
**      Returns:
**         E_DB_{OK, WARN,ERRR, FATAL}
**      Exceptions:
**          [@description_or_none@]
**
** Side Effects:
**          [@description_or_none]
**
** History:
**     21-dec-92 (anitap)
**         Need to restore the old QP's context on return from the processing
**         of a QEA_EXEC_IMM action.
**     08-jan-93 (anitap)
**	   Changed qeq_restore() to be static VOID.	
**      12-feb-92 (jhahn)
**	   removed saves of unused fields.
**	15-mar-93 (anitap)
**	    Removed unused field.
**	07-jul-93 (anitap)
**          Added new argument QEF_QP_CB and removed argument iierrorno.
**	    Do not restore qef_rowcount (to avoid 0 rows diagnostic).
**	27-oct-93 (rickh)
**	    Added code to NOT restore the qef_output pointer if we're
**	    performing a SELECT ANY to verify an ALTER TABLE ADD CONSTRAINT.
**      13-jun-97 (dilma04)
**          Added code to restore QEF_CONSTR_PROC bit flag. 
**	13-Aug-2005 (schka24)
**	    Restore next-output position, which row-producing procedures use
**	    to track the output buffer position.
**	9-Nov-2009 (kschendel) b122851
**	    Restore query constant block pointer in ADF-CB, so that it
**	    matches the current DSH.
*/
static void
qeq_restore( 
	QEF_RCB         *qeq_rcb,
	QEF_RCB         *qef_rcb,
	QEE_DSH         *dsh,
	i4             *mode,
	QEF_AHD         **act,
	i4         *rowcount,
	QEF_QP_CB	**qeq_qp,
	PTR             **cbs,
	i4              **iirowcount)
{
     QEF_CB		*qef_cb = qeq_rcb->qef_cb;

     qef_rcb->qef_pcount = qeq_rcb->qef_pcount;
     qef_rcb->qef_usr_param = qeq_rcb->qef_usr_param;
     qef_rcb->qef_qp = qeq_rcb->qef_qp;
     if (qeq_rcb->qef_intstate & QEF_CONSTR_PROC)
         qef_rcb->qef_intstate |= QEF_CONSTR_PROC;
     else
         qef_rcb->qef_intstate &= ~QEF_CONSTR_PROC;

     /* qef_dbp_status is left alone */
     qef_rcb->qef_targcount = qeq_rcb->qef_targcount;

     /*
     ** DON'T restore the output-buffer-pointer if we're executing the
     ** SELECT ANY statement that verifies an ALTER TABLE ADD CONSTRAINT.
     ** The parent ALTER TABLE statement didn't have a valid output
     ** buffer.  However, SCF thoughtfully cooked up an output buffer
     ** for the SELECT.
     */

    if ( !( qef_cb->qef_callbackStatus & QEF_EXECUTE_SELECT_STMT ) )
    {
	qef_rcb->qef_output = qeq_rcb->qef_output;
	qef_rcb->qef_nextout = qeq_rcb->qef_nextout;
    }

     /* qef_rcb->qef_count = qeq_rcb->qef_count; */
     qef_rcb->qef_val_logkey = qeq_rcb->qef_val_logkey;
     STRUCT_ASSIGN_MACRO(qeq_rcb->qef_logkey,
                qef_rcb->qef_logkey);
     *mode= qeq_rcb->qef_mode;


     /* restore next action to execute and qeq_query() context. */

     *act = dsh->dsh_act_ptr;
     *rowcount = qef_rcb->qef_rowcount;
     *qeq_qp = dsh->dsh_qp_ptr;
     *cbs = dsh->dsh_cbs;

    /* Associate ADF-CB query constant block with current DSH's
    ** query constant block (if any, might be null).
    */
    dsh->dsh_adf_cb->adf_constants = dsh->dsh_qconst_cb;

     /* set up local variables from QP */
     if ((*qeq_qp)->qp_orowcnt_offset == -1)
        *iirowcount = (i4 *)NULL;
     else
        *iirowcount = (i4 *)(dsh->dsh_row[(*qeq_qp)->qp_rrowcnt_row] +
                   (*qeq_qp)->qp_orowcnt_offset);
}

/*{
** Name: QEQ_SETUP - Set up the variables for continuation of the E.I. 
**			action.
**			
** Description:
**	After we return from parsing the E.I. query text, we want to 
**	continue with new QP. We need to set up the new QP context.
**
**	This routine will be called for QEA_CREATE_TABLE/QEA_CREATE_VIEW
**	actions in CREATE SCHEMA, SELECT ANY(1) statement in Alter Table Add 
**	Constraint statement. 
**
**	This routine will also be called for QEA_CALLPROC action.
**
** Inputs:
**	qef_rcb		QEF Request Control Block
**	qef_info	PST_INFO pointer
**	status		status of E.I. execution
**	dsh		DSH for new QP
**	qp		pointer to new QP
**	first_act	action header of new QP
**	act		action header of new QP
**	cbs		Pointer to array of control blocks
**	iirowcount      final row count for each query 
**	iierrorno 
**
** Outputs:
**	none.
**
** 	Returns
**		none
**
** 	Exceptions:
**		none
**
** Side Effects:
**	none
**
** History:
**	06-aug-93 (anitap)
**	    Written for CREATE SCHEMA and FIPS CONSTRAINTS projects.
*/
static void
qeq_setup(
QEF_CB		*qef_cb,
QEE_DSH		**dsh,
QEF_QP_CB	**qp,
QEF_AHD		**first_act,
QEF_AHD		**act,
PTR		**cbs,
i4		**iirowcount,
i4		**iierrorno)
{
 
    /* We have the QP for the QEA_EXEC_IMM action. Set up the */
    /* local variables required for processing and continue   */
    /* with the first action of the new QP                    */

    *dsh = (QEE_DSH *)qef_cb->qef_dsh;
    *qp = (*dsh)->dsh_qp_ptr;
    *first_act = (*qp)->qp_ahd;
    *act = (*qp)->qp_ahd;
    *cbs = (*dsh)->dsh_cbs;
   
    /* set up local variables from QP */
    if ((*qp)->qp_orowcnt_offset == -1)
       *iirowcount = (i4 *)NULL;
    else
       *iirowcount = (i4 *)((*dsh)->dsh_row[(*qp)->qp_rrowcnt_row] +
                            (*qp)->qp_orowcnt_offset);

    if ((*qp)->qp_oerrorno_offset == -1)
       *iierrorno = (i4 *)NULL;
    else
       *iierrorno = (i4 *)((*dsh)->dsh_row[(*qp)->qp_rerrorno_row] +
                            (*qp)->qp_oerrorno_offset);
 }

/*{
** Name: QEQ_UPDROW - set update_rowcount to changed or qualified
**
** External QEF call:   status = qef_call(QEQ_UPD_ROWCNT, &qef_rcb);
**
** Description:
**      With this set command the user can get the value for the actual number
**  of rows that were updated or the number of rows that qualified for the
**  update.
**
** Inputs:
**      qef_rcb
**          .qef_cb            session control block
**          .qef_upd_rowcnt    value for the number of rows that should be
**                             returned to the user
**              QEF_ROWCHGD
**              QEF_ROWQLFD
**          .qef_eflag          designate error handling semantics
**                               for user errors.
**              QEF_INTERNAL    return error code.
**              QEF_EXTERNAL    send message to user.
**
** Outputs:
**      qef_rcb
**          .error.err_code     One of the following:
**                              E_QE0000_OK
**
**      Returns:
**          E_DB_OK
**      Exceptions:
**         none
**
** Side Effects:
**          none
**
** History:
**      28-apr-93 (anitap)
**          Written.
*/
DB_STATUS
qeq_updrow(
QEF_CB          *qef_cb)
{

      qef_cb->qef_upd_rowcnt = qef_cb->qef_rcb->qef_upd_rowcnt;
      qef_cb->qef_rcb->error.err_code = E_QE0000_OK;
      return (E_DB_OK);
}

/*{
** Name: QEQ_DSHTORCB	- copies items from QEE_DSH to QEF_RCB
**
** Description:
**      Copies various modifiable fields from the DSH back to the RCB
**	where they are expected by callers of qeq.c. This is part of
**	the thread safety logic for parallel query execution.
**
** Inputs:
**      qef_rcb		QEF_RCB pointer
**	dsh		QEE_DSH pointer
**
** Outputs:
**      qef_rcb		updated by various elements from dsh
**
**      Returns:
**          E_DB_OK
**      Exceptions:
**         none
**
** Side Effects:
**          none
**
** History:
**      23-feb-04 (inkdo01)
**	    Written to aid thread safety of || query execution.
*/
VOID
qeq_dshtorcb(
QEF_RCB		*qef_rcb,
QEE_DSH		*dsh)
{

    qef_rcb->qef_output = dsh->dsh_qef_output;
    qef_rcb->qef_count = dsh->dsh_qef_count;
    qef_rcb->qef_rowcount = dsh->dsh_qef_rowcount;
    qef_rcb->qef_targcount = dsh->dsh_qef_targcount;
    qef_rcb->qef_remnull = dsh->dsh_qef_remnull;
    qef_rcb->qef_nextout = dsh->dsh_qef_nextout;

    STRUCT_ASSIGN_MACRO(dsh->dsh_error, qef_rcb->error);
}


/*{
** Name: QEQ_RCBTODSH	- copies items from QEF_RCB to QEE_DSH
**
** Description:
**      Copies various modifiable fields from the RCB to the DSH
**	where they may be modified by different threads executing 
**	portions of a query plan in parallel. This is part of
**	the thread safety logic for parallel query execution.
**
** Inputs:
**      qef_rcb		QEF_RCB pointer
**	dsh		QEE_DSH pointer
**
** Outputs:
**      dsh		updated by various elements from rcb
**
**      Returns:
**          E_DB_OK
**      Exceptions:
**         none
**
** Side Effects:
**          none
**
** History:
**      23-feb-04 (inkdo01)
**	    Written to aid thread safety of || query execution.
*/
VOID
qeq_rcbtodsh(
QEF_RCB		*qef_rcb,
QEE_DSH		*dsh)
{

    dsh->dsh_qef_output = qef_rcb->qef_output;
    dsh->dsh_qef_count = qef_rcb->qef_count;
    dsh->dsh_qef_rowcount = qef_rcb->qef_rowcount;
    dsh->dsh_qef_targcount = qef_rcb->qef_targcount;
    dsh->dsh_qef_remnull = qef_rcb->qef_remnull;
    dsh->dsh_qef_nextout = qef_rcb->qef_nextout;

    STRUCT_ASSIGN_MACRO(qef_rcb->error, dsh->dsh_error);
}
