/*
**Copyright (c) 2010 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <me.h>
#include    <pc.h>
#include    <st.h>
#include    <bt.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <cs.h>
#include    <lk.h>
#include    <scf.h>
#include    <ulf.h>
#include    <ulm.h>
#include    <adf.h>
#include    <ade.h>
#include    <dmf.h>
#include    <tm.h>
#include    <sxf.h>
#include    <dmtcb.h>
#include    <dmrcb.h>
#include    <dmscb.h>
#include    <dmacb.h>
#include    <qefmain.h>
#include    <qsf.h>
#include    <qefrcb.h>
#include    <qefcb.h>
#include    <qefnode.h>
#include    <psfparse.h>
#include    <qefact.h>
#include    <qefqp.h>
#include    <qefdsh.h>
#include    <qefscb.h>
#include    <er.h>

#include    <dmmcb.h>
#include    <dmucb.h>
#include    <ex.h>

#include    <dudbms.h>
#include    <qefqeu.h>
#include    <qeuqcb.h>
#include    <rqf.h>
#include    <tpf.h>
#include    <qefkon.h>
#include    <qefqry.h>
#include    <qefcat.h>
#include    <qefprotos.h>
#include    <rdf.h>
/**
**
**  Name: QEQVALID.C - query validation routines
**
**  Description:
**      The routines in this file perform all query validation
**  semantics. This includes checking timestamps on relations
**  and the use of pattern matching in keys.
**
**  As a byproduct, the relations are opened and keys are produced. 
**
**	    qeq_topact_validate - Open a "top" action (one NOT under a QP
**			   node), validate the entire QP if necessary.
**	    qeq_subplan_init - (Re-)Initialize the subplan under an action,
**			   (or rarely, under a specific node).  Re-runs
**			   virgin CX segments, resets keying, etc.
**	    qeq_ksort	 - key sorting and placement
**	    qeq_ade	 - initialize ADE_EXCBs.
**	    qeq_ade_base - initialize ADE_EXCB pointer array.
**          qeq_audit     - Audit access to table and views, also alarms
**	    qeq_vopen	 - Open a table instance (a valid struct)
**
**
**  History:
**      1-jul-86 (daved)    
**          written
**	19-aug-87 (puree)
**	    modified qeq_validate for repositioning a table.
**	09-dec-87 (puree)
**	    modified qeq_validate not to return upon error.
**      10-dec-87 (puree)
**          Converted all ulm_palloc to qec_palloc
**	06-jan-88 (puree)
**	    Modified qeq_validate to set QEQP_OBSOLETE flag in an invalid DSH.
**	10-feb-88 (puree)
**	    Convert ulm_openstream to qec_mopen, rename qec_palloc to
**	    qec_malloc.
**	11-feb-88 (puree)
**	    Modify qeq_ade to initialize VLT dsh using ADE_SVIRGIN CX.
**	19-feb-88 (puree)
**	    Modify qeq_validate to follow the key list in positioning
**	    the first tuple for a multiple ranges keys.
**	22-feb-88 (puree)
**	    Modify qeq_validate to validate and set table lock modes based 
**	    on the estimated nunber of pages touched.
**	11-mar-88 (puree)
**	    remove DSH_QUEUE structure. DMT_CB's can be linked together
**	    using the standard jupiter control block header.
**	10-may-88 (puree)
**	    Modify qeq_validate to set obsolesence flag in DSH, not in QP.
**	08-aug-88 (puree)
**	    Modify qeq_validate to recognize E_DM0114_FILE_NOT_FOUND.
**	11-aug-88 (puree)
**	    Clean up lint errors.
**	22-aug-88 (puree)
**	    Modify qeq_validate to initialize ade_excb for qp_qual CX for
**	    QP nodes.
**	02-sep-88 (puree)
**	    Modify qeq_ade for new VLUP/VLT scratch space management.
**	    Rename fields in DSH and QEN_ADF.
**	05-oct-88 (puree)
**	    Modify transaction handling for invalid query.  If a table in
**	    the topmost action is invalid, we will just terminate the 
**	    current query without executing it.  If a table in other actions
**	    is invalid, the current query will aborted.  Terminating and
**	    aborting a query take their usual semantics.
**
**	    In this module, qeq_validate is modified to return E_DB_WARN
**	    if the action is the topmost one. Else, it returns E_DB_ERROR.
**	14-oct-88 (puree)
**	    Modify qeq_validate to handle E_DM0055_NONEXT during positioning.
**	02-dec-88 (puree)
**	    Fix qeq_ksort to break off from the comparing loop if the two
**	    OR lists do not have the same attribute.
**	19-feb-89 (paul)
**	    Enhanced for rules processing. qeq_ade extended to initialize 
**	    parameter arrays for CALLPROC statements.
**	04-apr-89 (jrb)
**	    Properly initialized db_prec field for decimal project.
**	18-apr-89 (paul)
**	    Modify to support cursor update and delete as regular query plans.
**	13-jun-89 (paul)
**	    Fix bug 6346; Add a new parameter to qeq_validate to indicate
**	    whether only the tables in the valid list should be opened or
**	    the tables should be opened and the action initialized.
**      07-jul-89 (jennifer)
**          Added routine to audit direct updates to secure system catalogs,
**          to audit the access of tables and views, to indicate which 
**          alarm events happened, and to re-qualify access to all 
**          views (This forces a view MAC check in DMF each time
**          a query is run).
**	07-aug-89 (neil)
**	    Add consistency check to avoid AV when a cb gets trashed.
**	23-Mar-90 (anton)
**	    Add CSswitch call to prevent stored procedure loops from
**	    hanging UNIX servers.
**	13-apr-90 (nancy)
**	    Bug 20408.  Determine if this node does a key_by_tid.  If so, don't
**	    position here since it is done in qen_orig().
**	18-dec-90 (kwatts)
**	    Smart Disk integration: Added code to check for a Smart Disk program
**	    and if appropriate point at it from the rcb and set position type
**	    to SDSCAN.
**	06-mar-91 (davebf)
**	    Protect against a retry loop due to a max-i4 vl_est_pages.
**	16-apr-91 (davebf)
**	    Enabled vl_size_sensitive test since OPC now sets it
**	25-jun-91 (stevet)
**	    Fix B38161: Return error E_QE0018_BAD_PARAM_IN_CB if parameter 
**          is not found when expected.
**	07-aug-91 (stevet)
**	    Fix B37995: Added new parameter to qeq_ksort to indicate whether
**	    finding the MIN of the MAX keys and MAX of the MIN keys is required.
**	07-aug-91 (davebf)
**	    Extended scope of vl_size_sensitive test to avoid scaling
**	    estimate for lock granularity purposes when not size sensitive
**	    (b39056)
**      09-sep-91 (mikem)
**          bug #39632
**          Changed the logic of qeq_validate() to take advantage of the new
**          returns added to DMT_OPEN.  DMT_OPEN now returns # of tuples and #
**          of pages of a table following an open.  The new logic now uses this
**          information rather than an extra DMT_SHOW call to get the same
**          information.
**
**          Prior to this change QEF, while validating DBP QEPs, would first
**          call DMT_SHOW for this information and subsequently call DMT_OPEN.
**          This extra call was expensive on platforms like the RS6000 or
**          DECSTATION where cross process semaphores must be implemented as
**          system calls.  This change allows QEF (qeq_validate()) to eliminate
**          the extra DMT_SHOW call.
**	15-oct-91 (bryanp)
**	    Initialize dmt_char_array in the dmt_show_cb.
**      12-dec-91 (davebf)
**          Avoid executing ahd_constant in qeq_validate when QEA_AGGF (b41134)
**	15-jan-92 (rickh)
**	    In qeq_validate, don't re-initialize node status when chasing
**	    the postfix list at record position time.
**	12-feb-1992 (nancy)
**	    Bug 39225 -- Add QEA_IF and QEA_RETURN to statement types that
**	    need build-key initialization so that local variables can be
**	    initialized. Modify logic so key-build is done first.
**	12-feb-1992 (nancy)
**	    Bug 38565 -- Add new error/return status for table size change -
**	    E_QE0301_TBL_SIZE_CHANGE - so that scf will NOT increment retry
**	    counter.
**	11-mar-92 (jhahn)
**	    Added support for QEF interrupts.
**      08-dec-92 (anitap)
**          Included header files qsf.h and psfparse.h
**	04_dec_92 (kwatts)
**	    Created a procedure qeq_ade_base to do the base to address
**	    conversion part of qeq_ade. Did this so that qeq_validate could
**	    call qeq_ade_prts to convert a table of bases to a table of
**	    pointers outside qeq_ade. Part of Smart Disk project.
**	10-mar-1993 (markg)
**	    Modified code in qeq_audit() to not flush the audit log each time
**	    a security alarm is triggered.
**	24-mar-93 (jhahn)
**	    Various fixes for support of statement level rules. (FIPS)
**	18-may-93 (nancy)
**	    Bug 48032.  Only do size_sensitive check if init_action is TRUE.
**	    This avoids QE0115 error in a dbp, which cannot be retried.  
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	20-jul-1993 (pearl)
**	    Bug 45912.    On the 2nd invocation of a db proc, the QEN_NOR's are
**          sorted, and may be in a different order than it is assumed to be
**          in on the 1st invocation, when the structures are newly created.
**          Change the traversal of the QEN_NOR structures when ordering the
**          keys within the ranges in qeq_ksort().  Get the pointer to the
**          QEN_NOR structure from the dsh_kor_nor array.  Change the parameter
**          from dsh_key to pointer to dsh strucutre so we can get at the
**          array.  Change call to qeq_ksort()  to pass dsh instead
**          of dsh_key.
**      5-oct-93 (eric)
**          Added support for resource lists, and valid structs on their own
**          actions.
**	31-jan-94 (jhahn)
**	    Fixed support for resource lists.
**	22-feb-94 (jhahn)
**	    Another fix for support of resource lists.
**      19-dec-94 (sarjo01) from 18-feb-94 (markm)
**          Added "mustlock" flag that ensures we don't do any dirty reads
**          when the query is an update (58681).
**      01-nov-95 (ramra01)
**          noadf_mask preparation for qef optimization, reduced calls to
**          qen_u21_execute_cx. Cleanup compiler warnings.
**	7-nov-95 (inkdo01)
**	    Changes to replace QEN_ADF, QEN_OJINFO structure instances in
**	    various QEN_NODE and QEF_AHD structures.
**      06-feb-96 (johnst)
**          Bug 72202
**          Changed qee_ade_base() arguments to match the interface of
**          qeq_ade_base() from which it was originally cloned off,
**          restoring the two {qen_base, num_bases} arguments instead of passing
**          the higher level struct QEN_ADF, as (eric) defined it.
**          The Smart Disk code in qeq_validate() needs to call both
**          qee_ade_base() and qeq_ade_base() passing in the actual address
**          arrays and number of bases used for the Smart Disk case.
**          This enables ICL SCAFS to work again.
**      16-may-96 (ramra01)
**          Bug 76552
**	    Size sensitive option needs to refresh rdf cache if plan fails.
**	10-jul-96 (nick)
**	    Don't set LK_X on core catlogs just 'cos they don't have
**	    timestamps set - #77614.  Tidy some variable usage.
**	18-jul-96 (nick)
**	    If an action's resource is open but hasn't yet been used, then
**	    ensure we update the statement number. #76670
**	 4-oct-96 (nick)
**	    Tidy some stuff.
**	17-oct-96 (nick)
**	    Tidy some more stuff.
**	20-dec-96 (kch)
**	    In qeq_validate(), we now only calculate the qe90 statistic
**	    collection overhead once. This change fixes bug 78122.
**      13-jun-97 (dilma04)
**          In qeq_vopen(), set DMT_CONSTRAINT flag in the DMT_CB, if 
**          executing procedure that supports integrity constraint.
**	11-Aug-1997 (jenjo02)
**	    Changed ulm_streamid from (PTR) to (PTR*) so that ulm
**	    can destroy those handles when the memory is freed.
**	19-Aug-1997 (jenjo02)
**	    Distinguish between private and shared ULM streams.
**	    Piggyback ulm_palloc() calls with ulm_openstream() where appropriate.
**	07-Nov-1997 (jenjo02)
**	    Restored assignment of ulm_streamid to dsh_streamid in qeq_validate().
**      18-nov-97 (stial01)
**          qeq_vopen() QEF may specify DMT_C_EST_PAGES.
**      02-feb-98 (stial01)
**          Back out 18-nov-97 change.
**	16-jul-98 (shust01)
**	    SEGV possible due to initializing qep_node at time local variable
**	    is definied.  It points 64 bytes into action structure, but action
**	    is a variable sized structure, and there are times it is only 64
**	    bytes in size (action = QEA_INVOKE_RULE, for example).
**	    qep_node is initialized later on if it is needed.
**      07-jul-98 (sarjo01)
**          Bug 88784: Be sure to bump sequence number in dmt_cb each time
**          a table resource is opened with qeq_vopen(). A looping db proc
**          that updates the same row can produce ambiguous update error
**          if it thinks the same instance of a statement updates a record
**          twice.
**	28-oct-1998 (somsa01)
**	    In qeq_vopen(), in the case of session temporary tables, its
**	    locks will reside on session lock list.  (Bug #94059)
**	12-nov-1998 (somsa01)
**	    In qeq_vopen(), refined check for a Global Session Temporary
**	    Table.  (Bug #94059)
**	03-Dec-1998 (jenjo02)
**	    Repaired page count change calculation.
**	08-dec-1998 (somsa01)
**	    In qeq_valid(), if we find an unused open valid struct, don't
**	    just replace qer_inuse_vl with it as qer_inuse_vl may be set.
**	    Instead, add the open valid struct to the chain of qer_inuse_vl,
**	    making it the first in the chain.  (Bug #79195)
**      17-may-1999 (stial01)
**          qeq_vopen() QEF will defer lock escalation to DMF.
[@history_template@]...
**	17-mar-1999 (somsa01)
**	    Added include of pc.h and lk.h to resolve references to locking
**	    ids.
**      15-may-2000 (stial01)
**          Remove Smart Disk code
**      22-nov-2000 (stial01)
**          Check for lock timer expired during table validation (B103150)
**	18-Jan-2001 (jenjo02)
**	    Subvert calls to qeu_secaudit(), qeu_secaudit_detail()
**	    if not C2SECURE.
**	2-jan-03 (inkdo01)
**	    Removed use of noadf_mask (added in Nov. 1995) to prevent
**	    concurrency errors.
**	8-jan-04 (toumi01)
**	    Add support for exchange nodes.
**	23-feb-04 (inkdo01)
**	    Changed qef_rcb->error to dsh->dsh_error for || thread safety
**	    (and others).
**	16-Jul-2004 (schka24)
**	    Remove qeq_iterate, never called.
**	16-dec-04 (inkdo01)
**	    Add various collID's.
**    25-Oct-2005 (hanje04)
**        Add prototype for qeq_vopen() to prevent compiler
**        errors with GCC 4.0 due to conflict with implicit declaration.
**	28-Dec-2005 (kschendel)
**	    Use DSH references whenever possible instead of QEF RCB
**	    references.  (This simplifies the code, eliminating many
**	    RCB references, and reflects the new primacy of the DSH
**	    thanks to the parallel query changes.)
**	25-Jul-2007 (kschendel) SIR 122513
**	    Generalized partition qualification changes;  remove query-
**	    setup-time qualification to qeqpart.c.
**    08-Sep-2008 (gefei01)
**        Support table precedure.
**    03-Dec-2008 (gefei01)
**        Fixed SD 132599: qeq_validate() checks for QEF_CLEAN_RSRC
**        for table procedure. 
**    17-Dec-2008 (gefei01)
**        Fixed bug 121399: qeq_validate() saves QP ID for each
**        table procedure.
**    12-May-2009 (gefei01)
**        Bug 122053: Do table procedure timestamp validation
**        in the containing query. Modified qeq_validate(). Added
**        qeq_load_tproc_qp(), qeq_validate_tproc(), qeq_get_tproc_tab()
**        and qeq_destroy_tproc_qp().
**      01-Dec-2009 (horda03) B103150
**          In qeq_vopen, instruct DMT_OPEN not to wait for busy locks.
**	13-May-2010 (kschendel) b123565
**	    Don't pass action to vopen, not needed.
**	    Split validation into two pieces.  One is called to open tables
**	    for a "top action" (ie an action that's not under a QP node).
**	    The other walks a sub-plan starting at an action or a node and
**	    does initialization (VIRGIN segments and such).  The sub-plan
**	    initialization has to run in the proper child thread context
**	    when parallel query is in use, so it needs to be a separate piece.
**	17-Jun-2010 (kschendel) b123775
**	    More work on validation:  locate and validate tprocs during
**	    resource validation, not action open.  To decide if a table
**	    valid-list entry is open, look at the DMT_CB instead of
**	    recording the valid entry on (yet another) list.
**	29-Jun-2010 (kschendel)
**	    Comment updates only, no code change.
**/


/*
** Forward Declarations.
*/

static void qeq_ade_base(
	QEE_DSH		*dsh,
	QEN_BASE	*qen_base,
	PTR		*excb_base,
	i4		num_bases,
	PTR		*param );

static
DB_STATUS
qeq_audit(
QEF_RCB      *qef_rcb,
QEE_DSH	     *dsh,
QEF_AUD      *aud,
DB_STATUS   *status );

static DB_STATUS qeq_validate_qp(
	QEF_RCB		*qef_rcb,
	QEE_DSH		*dsh,
	bool		size_check);

static DB_STATUS
qeq_vopen(
	  QEF_RCB     *qef_rcb,
	  QEE_DSH     *dsh,
          QEF_VALID   *vl);

static DB_STATUS qeq_load_tproc_dsh(
	QEF_RCB		*qef_rcb,
        QEF_RESOURCE	*qpresource,
        QEE_DSH		*dsh,
	QEE_DSH		**tproc_dsh);

GLOBALREF QEF_S_CB		*Qef_s_cb;

/*{
** Name: qeq_topact_validate - Open/Validate a "top" action
**
** Description:
**	This routine is called to prepare a "top" action.  Top actions
**	are always run in the main session thread (*) and are never actions
**	found in a QP tree under a QP node.
**
**	If it's the first action of the query, the resource list is
**	used to validate the query plan as a whole.  If some table
**	(or DBproc) is newer than the QP timestamp, the entire query
**	plan is invalid, and a warning plus specific error status is
**	returned.  The caller might attempt a recompile / retry of
**	the entire query.
**
**	Each top action (first or not) of a query has a "valid" list,
**	which is a list of tables that need to be opened for that action.
**	We'll open the table in the appropriate lockmode.  Note that for
**	a parallel query, child threads may have to open the table
**	again in the child context;  it would be slicker to tie table-
**	opens to the specific threads, but that's not how it works at
**	the moment.
**
**	(*) A top action may be opened and executed in a child thread of a
**	parallel plan, if and only if it's an action inside a table procedure.
**	In that situation, all we're doing is opening tables;  QP validation
**	never takes place in a child thread.
**
** Inputs:
**      qef_rcb			QEF query request control block
**	dsh			DSH for the (main session) thread.
**	action			A "top" action, possibly with a resource or
**				valid list.
**	size_check		TRUE to do table size check if we're
**				validating the QP (first action)
**				FALSE if just reopening tables (e.g. for
**				DBproc continuation after commit/rollback)
**
** Outputs:
**	qef_rcb
**	    .error.err_code	one of the following
**				E_QE0000_OK
**				E_QE0017_BAD_CB
**				E_QE0018_BAD_PARAM_IN_CB
**				E_QE0019_NON_INTERNAL_FAIL
**				E_QE0002_INTERNAL_ERROR
**				E_QE0023_INVALID_QUERY
**				E_QE0105_CORRUPTED_CB - new generic CB error.
**	Returns:
**	    E_DB_{OK, WARN, ERROR, FATAL}
**	Exceptions:
**	    none
**
** Side Effects:
**	    relations are opened
**
** History:
**	1-july-86 (daved)
**          written
**	14-feb-87 (daved)
**	    mark invalid queries as non-repeat so that cleanup will happen
**	    properly.
**	19-aug-87 (puree)
**	    reset dsh_qen_status->node_status when repositioning a table in
**	    an orig node.
**	09-dec-87 (puree)
**	    return immediately on warning or error.  also zero out the
**	    action pointer in dsh in the beginning.  This is necessary
**	    for qeq_fetch to recognize that there is no row qualified
**	    on the specified constant expression.
**	06-jan-88 (puree)
**	    If a QP is found to be invalid, set the QEQP_OBSOLETE in the
**	    DSH qp_status flag so that qee_clean up will destroy the DSH
**	    in case of a repeat query plan.
**	01-feb-88 (puree)
**	    Reset dmt_db_id every time we validate.  This is a temporary
**	    fix to test shared query plan.  Resetting db id should be done
**	    in qee_fetch.
**	02-feb-88 (puree)
**	    add reset flag to the function arguments.
**	19-feb-88 (puree)
**	    Modify qeq_validate to follow the key list in positioning
**	    the first tuple for a multiple ranges keys.
**	22-feb-88 (puree)
**	    Implement table validation and set lock mode based on the
**	    estimated page count.
**	11-mar-88 (puree)
**	    remove DSH_QUEUE structure. DMT_CB's can be linked together
**	    using the standard jupiter control block header.
**	10-may-88 (puree)
**	    Mark dsh_qp_status for obsolesence, not qp_status.  QP is a
**	    read-only object to QEF.
**	08-aug-88 (puree)
**	    Watch for E_DM0114_FILE_NOT_FOUND from DMT_SHOW and DMT_OPEN.
**	    This error makes the query invalid.
**	22-aug-88 (puree)
**	    QP node may have query paramters for its qualification CX.
**	    The ade_excb must be initialized here.
**	19-sep-88 (puree)
**	    Fix bug setting DSH_QP_OBSOLETE flag.  The flag must be set in
**	    the DSH, not in QP as it was before the fix.
**	05-oct-88 (puree)
**	    Implement new transaction handling for invalid query.  If a table
**	    in the topmost action of a query is found invalid, return the
**	    error E_QE0023_INVALID_QUERY with status E_DB_WARN.  If the
**	    action is not the topmost one, return E_QE0023_INVALID_QUERY
**	    with status E_DB_ERROR.  This allows the caller to terminate or
**	    abort the current query as appropriate.
**	14-oct-88 (puree)
**	    Return E_DB_OK if DMF returns E_DM0055_NONEXT.  Many QEP nodes
**	    still need to be executed.  During DMR_GETs we will receive
**	    the required E_DM0055...
**	20-oct-88 (puree)
**	    Fix for size_sensitive semantics.
**      15-feb-89 (davebf)
**          Temporary disable of tuple count mismatch error after DMT_SHOW
**	18-apr-89 (paul)
**	    Modify to support cursor update and delete as regular query plans.
**	13-jun-89 (paul)
**	    Fix bug 6346; Add a new parameter to qeq_validate to indicate
**	    whether only the tables in the valid list should be opened or
**	    the tables should be opened and the action initialized.
**      07-jul-89 (jennifer)
**          Added routine to audit direct updates to secure system catalogs,
**          to audit the access of tables and views, to indicate which
**          alarm events happened, and to re-qualify access to all
**          views (This forces a view MAC check in DMF each time
**          a query is run).
**          Must audit success and failure.
**	07-aug-89 (neil)
**	    Add consistency check to avoid AV when cbs[vl_dmr_cb] gets trashed.
**	23-Mar-90 (anton)
**	    Add CSswitch call to prevent stored procedure loops from
**	    hanging UNIX servers.
**	13-apr-90 (nancy)
**	    Bug 20408.  Determine if this node does a key_by_tid.  If so, don't
**	    position here since it is done in qen_orig().
**	28-nov-90 (teresa)
**	    bug 5446.  Invalidate QP is size estimate is too far off from reality.
**	04-jan-91 (davebf)
**	    Init dmt_cb_shw.dmt_char_array
**	04-jan-91 (davebf)
**	    Don't destroy QP of QUEL shared QP when recompiled due to
**	    pattern matching characters in string parameter.
**	18-dec-90 (kwatts)
**	    Smart Disk integration: Added code to check for a Smart Disk program
**	    and if appropriate point at it from the rcb and set position type
**	    to SDSCAN. Also took opportunity to introduce a QEN_ORIG pointer and
**	    tidy up the code.
**	06-mar-91 (davebf)
**	    Protect against a retry loop due to a max-i4 vl_est_pages.
**	16-apr-91 (davebf)
**	    Enabled vl_size_sensitive test since OPC now sets it
**      25-jun-91 (stevet)
**          Fix B38161: Return error E_QE0018_BAD_PARAM_IN_CB if parameter
**          is not found when expected.
**	07-aug-91 (davebf)
**	    Extended scope of vl_size_sensitive test to avoid scaling
**	    estimate for lock granularity purposes when not size sensitive
**	    (b39056)
**      09-sep-91 (mikem)
**          bug #39632
**          Changed the logic of qeq_validate() to take advantage of the new
**          returns added to DMT_OPEN.  DMT_OPEN now returns # of tuples and #
**          of pages of a table following an open.  The new logic now uses this
**          information rather than an extra DMT_SHOW call to get the same
**          information.
**
**          Prior to this change QEF, while validating DBP QEPs, would first
**          call DMT_SHOW for this information and subsequently call DMT_OPEN.
**          This extra call was expensive on platforms like the RS6000 or
**          DECSTATION where cross process semaphores must be implemented as
**          system calls.  This change allows QEF (qeq_validate()) to eliminate
**          the extra DMT_SHOW call.
**	15-oct-91 (bryanp)
**	    Initialize dmt_char_array in the dmt_show_cb.
**      12-dec-91 (davebf)
**          Avoid executing ahd_constant in qeq_validate when QEA_AGGF (b41134)
**	14-jan-1992 (nancy)
**	    Bug #39225 -- local variables in dbp's do not get initialized in
**	    IF and RETURN statements.  Add types QEA_IF and QEA_RETURN to
**	    section for initializing cx's to build keys. (need opc part also)
**	14-jan-1992 (nancy)
**	    Bug 38565 -- Add new error/return status for table size change -
**	    E_QE0301_TBL_SIZE_CHANGE - so that scf will NOT increment retry
**	    counter.  This allows a large insert to proceed when rules are
**	    fired which reference the table which is growing.
**	15-jan-92 (rickh)
**	    In qeq_validate, don't re-initialize node status when chasing
**	    the postfix list at record position time.
**	11-mar-92 (jhahn)
**	    Added support for QEF interrupts.
**	24-mar-93 (jhahn)
**	    Various fixes for support of statement level rules. (FIPS)
**	18-may-93 (nancy)
**	    Bug 48032.  Only do size_sensitive check if init_action is TRUE.
**	    If init_action is FALSE then we are inside a database procedure
**	    and we don't need to know if the qep needs recompiling.  This
**	    avoids QE0301 error for table size change and thus avoids QE0115
**	    error which is fatal dbp error.
**	5-jul-93 (robf)
**	    Reworked ORANGE code, security auditing.
**      20-jul-1993 (pearl)
**          Bug 45912.    On the 2nd invocation of a db proc, the QEN_NOR's are
**          sorted, and may be in a different order than it is assumed to be
**          in on the 1st invocation, when the structures are newly created.
**          Change the traversal of the QEN_NOR structures when ordering the
**          keys within the ranges in qeq_ksort().  Get the pointer to the
**          QEN_NOR structure from the dsh_kor_nor array.  Change the parameter
**          from dsh_key to pointer to dsh strucutre so we can get at the
**          array.  Change call to qeq_ksort()  to pass dsh instead
**	    lists.
**	15-sep-93 (robf)
**	    Pass DMT_UPD_ROW_SECLABEL option to DMF when row security
**	    label attribute is being updated.
**	    Rework security auditing to get audit info from the action
**	    rather than the qp header. This allows action-specific auditing.
**          of dsh_key.
**	5-oct-93 (eric
**	    Added support for resource lists. This changes the way that we
**	    validate query plans. Now we validate resource lists, not valid
**	    lists.
**      7-jan-94 (robf)
**          Rework secure changes after mainline integration.
**	31-jan-94 (jhahn)
**	    Fixed support for resource lists.  Moved some code into the
**	    procedure "qeq_release_dsh_resources".
**	22-feb-94 (jhahn)
**	    Another fix for support of resource lists.
**      19-jun-95 (inkdo01)
**          Collapse orig_tkey/ukey nat's into single orig_flag flag word.
**      21-aug-95 (ramra01)
**          setup for ORIGAGG
**      01-nov-95 (ramra01)
**          noadf_mask preparation for qef optimization, reduced calls to
**          qen_u21_execute_cx.
**	7-nov-95 (inkdo01)
**	    Changes to replace QEN_ADF, QEN_OJINFO structure instances in
**	    various QEN_NODE and QEF_AHD structures.
**	05-Dec-95 (prida01)
**	    Add qe90 tracing for open file. This is a dio and costs in the query
**	    it should be noted in the qe90 tracing. (bug 72937)
**      06-feb-96 (johnst)
**          Bug 72202
**          Changed qee_ade_base() arguments to match the interface of
**          qeq_ade_base() from which it was originally cloned off,
**          restoring the two {qen_base, num_bases} arguments instead of passing
**          the higher level struct QEN_ADF, as (eric) defined it.
**          The Smart Disk code in qeq_validate() needs to call both
**          qee_ade_base() and qeq_ade_base() passing in the actual address
**          arrays and number of bases used for the Smart Disk case.
**          This enables ICL SCAFS to work again.
**          Removed (ramra01) comment around qee_ade_base() call as it is now
**          fixed.
**      16-may-96 (ramra01)
**          Bug 76552
**          Size sensitive option needs to refresh rdf cache if plan fails.
**	10-jul-96 (nick)
**	    Remove unused variable.
**	18-jul-96 (nick)
**	    We weren't updating the statement number on resources which are
**	    open but not yet in use ( i.e. they are sitting on the free_vl
**	    list ).  This breaks the deferred update semantics in DMF. #76670
**	    Also, the addition of qe90 tracing on 5-dec-95 above broke
**	    the error handling in this routine as it trashed status ; I've
**	    fixed this.
**	20-dec-96 (kch)
**	    We now only calculate the qe90 statistic collection overhead once.
**	    This change fixes bug 78122.
**	12-feb-97 (inkdo01)
**	    Changes for MS Access OR transform - such QEF_VALIDs skip most
**	    validation (since corr. table is in memory). Just have to plug
**	    addr of QEE_MEM_CONSTTAB struct in dmr_cb slot of dsh_cbs.
**	6-nov-97 (inkdo01)
**	    Avoid executing ahd_constant for QEA_SAGG (75803).
**	07-Nov-1997 (jenjo02)
**	    Restored assignment of ulm_streamid to dsh_streamid in qeq_validate().
**	13-nov-97 (inkdo01)
**	    Improved (?) prida fix for bug 72937. Grab stats for DMR_POSITION
**	    logic only.
**	16-jul-98 (shust01)
**	    SEGV possible due to initializing qep_node at time local variable
**	    is definied.  It points 64 bytes into action structure, but action
**	    is a variable sized structure, and there are times it is only 64
**	    bytes in size (action = QEA_INVOKE_RULE, for example).
**	    qep_node is initialized later on if it is needed.
**	27-jul-98 (hayke02)
**	    Switch off simple aggregate optimization for the max() function
**	    (orig_flag &= ~ORIG_MAXOPT) if we fail to position on the key
**	    value(s) (i.e. the call to qen_position() returns with status
**	    E_DB_WARN and the err_code is E_QE0015_NO_MORE_ROWS). This
**	    change fixes bug 91022.
**	21-jul-98 (inkdo01)
**	    Added support for topsort-less descending sorts.
**	02-sep-1998 (nanpr01)
**	    We must invalidate the RDF cache if the timestamp doesnot match.
**	    Actually QE0023 has been returned in many other cases where really
**	    an RDF invalidation is not required. This bug was noticed when
**	    I disabled RDF cache refresh for every DEFQRY and DEFCURS in
**	    pstshwtab.c.
**	03-Dec-1998 (jenjo02)
**	    Repaired page count change calculation.
**	08-dec-1998 (somsa01)
**	    If we find an unused open valid struct, don't just replace
**	    qer_inuse_vl with it as qer_inuse_vl may be set. Instead,
**	    add the open valid struct to the chain of qer_inuse_vl, making
**	    it the first in the chain.  (Bug #79195)
**	27-jan-2000 (hayke02)
**	    We now set AD_1RSVD_EXEDBP in the ADI_WARN block before
**	    the call to ade_execute_cx() to create the keys, if we are
**	    executing a DBP. This prevents adc_ixopt() being called,
**	    which in turn prevents incorrect results when a DBP
**	    parameter is used in multiple predicates. This change
**	    fixes bug 99239.
**	26-jul-00 (inkdo01)
**	    Only increment statement number if query plan requires deferred
**	    processing (refinement to fix for 76670).
**	30-jan-01 (inkdo01)
**	    Changes for hash aggregation (support QEA_HAGGF action).
**	20-apr-01 (inkdo01)
**	    Add ahd_agoflow ADF code to QEA_HAGGF initialization.
**	18-mar-02 (inkdo01)
**	    Add open call for sequences.
**	2-jan-03 (inkdo01)
**	    Removed use of noadf_mask (added in Nov. 1995) to prevent
**	    concurrency errors.
**	6-jan-03 (inkdo01)
**	    Fix to above fix to eliminate possible SEGV's (what a slob!).
**	8-jan-03 (inkdo01)
**	    Minor fix to sequence logic to open 'em even if no resources
**	    (could be a constant query).
**	06-Feb-2003 (jenjo02)
**	    If DMS_OPEN_SEQ fails, set err_code in qef_rcb.
**	12-feb-03 (inkdo01)
**	    Reset dms_db_id so repeat queries work ok.
**	29-dec-03 (inkdo01)
**	    DSH is now parameter, "function" replaces "reset".
**	2-feb-04 (inkdo01)
**	    Add preparation of qualified partitions bit map for partitioned
**	    tables.
**	25-feb-04 (inkdo01)
**	    Assign DSH addr to dmr_q_arg instead of QEF_CB for thread safety.
**	27-feb-04 (inkdo01)
**	    Changed dsh_qen_status from ptr to array of QEN_STATUSs to
**	    ptr to arrays of ptrs.
**	11-mar-04 (inkdo01)
**	    Save key structure in qen_status for partitioned table keyed
**	    retrievals.
**	26-mar-04 (inkdo01)
**	    Save top action's ahd_valid ptr in DSH for part. opening.
**	1-apr-04 (inkdo01)
**	    Avoid dmf_call's from ORIG nodes unless it's for ORIGAGG (they
**	    interfere with || query logic).
**	8-apr-04 (inkdo01)
**	    Add dsh to QEF_CHECK_FOR_INTERRUPT macro.
**	12-may-04 (inkdo01)
**	    Support added slot in cbs array for master DMR_CB of
**	    partitioned table.
**	14-may-04 (inkdo01)
**	    Fix above change for action header defined DMR_CB's.
**	8-Jun-2004 (schka24)
**	    Move above to vopen so that it happens for all vl opens.
**	11-june-04 (inkdo01)
**	    Change to use local dsh rather than global (eliminates
**	    SEGV in || processing).
**	15-july-04 (inkdo01)
**	    Make same change for first dsh_act_ptr ref (not sure why I
**	    didn't make this change before).
**	30-Jul-2004 (jenjo02)
**	    Use dsh_dmt_id rather than qef_dmt_id transaction context.
**	26-aug-04 (inkdo01)
**	    Enable global base arrays.
**	20-Sep-2004 (jenjo02)
**	    If ORIG keys haven't been made yet (like in a SQ),
**	    call qen_keyprep() to make them.
**	04-Nov-2004 (jenjo02)
**	    Only CSswitch on root DSH or if not OS-threaded.
**	13-Dec-2005 (kschendel)
**	    Remaining inline QEN_ADF changed to pointers, fix here.
**	    Don't ask for DMF qual unless there is really a qualification cx.
**	16-Jan-2006 (kschendel)
**	    Qen-status now addressable from XADDRS structure.
**	31-May-2006 (kschendel)
**	    Remove a little origagg dead code.
**	16-Jun-2006 (kschendel)
**	    Multi-dimensional partition qualification.
**	12-jan-06 (kibro01) b117485
**	    When a table open fails the error is in dmt_cb, since that
**	    was what was passed into dmf_call
**	12-apr-2007 (dougi)
**	    Init ahd_rskcurr CX for KEYSET processing.
**	13-Feb-2007 (kschendel)
**	    Replace CSswitch with better CScancelCheck.
**	17-dec-2007 (dougi)
**	    Support parameterized first/offset "n".
**	29-jan-2008 (dougi)
**	    Check for 0-valued first/offset "n", too.
**	11-Apr-2008 (kschendel)
**	    Call utility for setting DMR qual stuff.
**	4-Sep-2008 (kibro01) b120693
**	    Save ahd_flags for partition opening, same as ahdvalid.
**	8-Sep-2008 (kibro01) b120693
**	    Reworked fix for b120693 avoiding need for extra flag.
**      08-Sep-2008 (gefei01)
**          Check for table procedure and load qp.
**      19-nov-2008 (huazh01)
**          Back out the fix to b99239, which can't be reproduced
**          after 2.x code line. This allows the 'ad_1rsvd_cnt'
**          field to be removed from ADI_WARN structure. (b121246).
**      03-Dec-2008 (gefei01)
**          Fixed SD 132599: qeq_validate() checks for QEF_CLEAN_RSRC
**          for table procedure.
**      17-Dec-2008 (gefei01)
**          Fixed bug 121399: Save QP ID for each table procedure.
**	4-Jun-2009 (kschendel) b122118
**	    Use slightly faster/more convenient dsh test for qe90, and
**	    remove pointless error returns from cost begin/end.
**	    Streamline qeq-ade initialization calls.
**	    Don't broadcast RDF invalidation because of out-of-date
**	    QP stamp, no reason to think that other servers need it.
**	    Fix the qe90 cpu overhead calculator to be self calibrating;
**	    the old loop count of 2000 was way too small to evaluate an
**	    overhead on modern machines.  Require at least 2 cpu-seconds.
**	23-Jul-2009 (kschendel) b122118
**	    Fix two "if (oj == NULL);" that somehow crept in.
**	13-Aug-2009 (kschendel) b122118
**	    One more dangling semicolon found by inspection.  I must have
**	    had a really bad juju day.
**	14-Oct-2009 (kschendel) b121882
**	    Set-qualfunc has to happen *after* the qeq_ade calls (kjoin).
**	    Otherwise, a reset will leave the CX segment as virgin,
**	    not main, and DMF row qualification will run the wrong code.
**	    This was fixed (differently) by wanfr01, but the structural
**	    changes for kjoin partition pruning (122513) undid that fix.
**	    This fix is sharper, anyway.
**	    Also: don't clear orig node stats at reset, is very confusing.
**	17-Nov-2009 (kschendel) SIR 122890
**	    Combine load with append.  Add needs-X-lock support.
**	13-May-2010 (kschendel) b123565
**	    Split validate into two pieces.  This piece is for opening tables
**	    at the "top" action of a query.
**	20-May-2010 (kschendel) b123775
**	    Restore the setting of dsh-act-ptr for tprocs, that's the
**	    query resume point if the tproc needs loaded.  My bad for
**	    taking it out.
**	18-Jun-2010 (kschendel) b123775
**	    Instead of using yet another list to see if a valid entry was
**	    processed, just check the DMT_CB.  Take tproc handling out of
**	    here, do in the QP validation routine which is where it
**	    really belongs anyway.  (plus doing it here causes confusion
**	    as to which resource list is being used.)
*/

DB_STATUS
qeq_topact_validate(
QEF_RCB		*qef_rcb,
QEE_DSH		*dsh,
QEF_AHD		*action,
bool		size_check)
{
    QEF_VALID	    *vl;
    DB_STATUS	    status = E_DB_OK;
    QEF_CB	    *qef_cb = dsh->dsh_qefcb;
    PTR		    *cbs = dsh->dsh_cbs;		/* control blocks for
							** DMU, ADF
							*/
    DMR_CB	    *dmr_cb;
    DMT_CB	    *dmt_cb;
    QEF_QP_CB       *qp = dsh->dsh_qp_ptr;
    i4		    i;
    QEE_RESOURCE    *resource;
    QEF_RESOURCE    *qefresource;
    DMR_CHAR_ENTRY  dmr_char_entry;

    /* This call will prevent a server which implements CSswitch from
       getting into a loop if a user has a looping stored procedure. */

    CScancelCheck(dsh->dsh_sid);

    if (QEF_CHECK_FOR_INTERRUPT(qef_cb, dsh) == E_DB_ERROR)
	return (E_DB_ERROR); /* so user can abort a looping procedure */

    /* clean up action pointer, it will be set if qeq_validate completed */

    dsh->dsh_act_ptr = (QEF_AHD *)NULL;

    /* validate relations */
    /* Give DMT a new statement number, so that it can properly maintain
    ** the deferred update semantics (but only if deferred processing
    ** is required).
    */
    if (qp->qp_status & QEQP_DEFERRED)
	dsh->dsh_stmt_no = qef_cb->qef_stmt++;

    if (action == qp->qp_ahd && (dsh->dsh_qp_status & DSH_TPROC_DSH) == 0)
    {
	/* If the first action of a query, validate the query plan against
	** the resource list of tables and tprocs needed.  This will
	** check timestamps and sizes, and will return a not-OK status if
	** the QP is out of date with respect to the resources needed.
	**
	** This is NOT done for table procedures;  tprocs are validated
	** from the parent query plan.  By the time we actually get into
	** the tproc it should be known valid.
	*/
	status = qeq_validate_qp(qef_rcb, dsh, size_check);
	/* if status != E_DB_OK we drop thru, below */
    }

    /* Open tables for this action */
    for (vl = action->ahd_valid;
	    status == E_DB_OK && vl != NULL;
	    vl = vl->vl_next
	)
    {
	if (vl->vl_flags & QEF_MCNSTTBL) continue;

	/* Must be a table, and not an in-memory table either.
	** Open the table unless we opened it for validation purposes.
	*/
	dmt_cb = (DMT_CB *) cbs[vl->vl_dmf_cb];
	if (dmt_cb->dmt_record_access_id == NULL)
	{
	    /*
	    ** Open (another) DMF reference to the table
	    */
	    status = qeq_vopen(qef_rcb, dsh, vl);
	    if (status != E_DB_OK)
		break;
	}
	/* If the statement number isn't right (e.g. the table was
	** opened for validation at a prior step and the qp uses
	** deferred semantics), tell DMF the right stmt no.
	*/
	if (dmt_cb->dmt_sequence != dsh->dsh_stmt_no)
	{
	    /* set up to pass it to DMF */
	    dmr_char_entry.char_id = DMR_SEQUENCE_NUMBER;
	    dmr_char_entry.char_value = dsh->dsh_stmt_no;
	    dmt_cb->dmt_sequence = dsh->dsh_stmt_no;

	    dmr_cb = (DMR_CB *) cbs[vl->vl_dmr_cb];
	    dmr_cb->dmr_char_array.data_in_size =
					sizeof(DMR_CHAR_ENTRY);
	    dmr_cb->dmr_char_array.data_address =
					(PTR)&dmr_char_entry;

	    status = dmf_call(DMR_ALTER, dmr_cb);
	    if (DB_FAILURE_MACRO(status))
	    {
		dsh->dsh_error.err_code = dmr_cb->error.err_code;
		break;
	    }
	}

	/*
	** Make sure the table was locked with the correct lock mode
	**
	** If the table open failed with lock timeout while
	** validating ALL query plan table resources, we performed the
	** table validation with readlock=nolock which requests a table
	** control lock.
	**
	** The current action requires this table resource so we must
	** now try to lock the table with the correct lock mode.
	** Try to open a new cursor with the correct lock mode.
	**
	** We could alter the lock mode... (the way it is done in scsraat)
	** but this won't necessarily get the table lock mode we would
	** have gotten by calling open (which uses session lockmode info)
	**
	** Close the cursor opened with readlock nolock AFTER we request
	** the lock we need, so that the control lock is held while
	** the table lock is requested.
	*/
	if (dmt_cb->dmt_lock_mode == DMT_N)
	{
	    PTR		nolock_access_id;
	    PTR		new_access_id;

	    nolock_access_id = dmt_cb->dmt_record_access_id;
	    dmt_cb->dmt_record_access_id = (PTR)0;

	    /* default for lock granularity setting purposes */
	    switch (dmt_cb->dmt_access_mode)
	    {
		case DMT_A_READ:
		case DMT_A_RONLY:
		    dmt_cb->dmt_lock_mode = DMT_IS;
		    break;

		case DMT_A_WRITE:
		    /* See qeq-vopen... */
		    if (vl->vl_flags & QEF_NEED_XLOCK)
			dmt_cb->dmt_lock_mode = DMT_X;
		    dmt_cb->dmt_lock_mode = DMT_IX;
		    break;
	    }
	    status = dmf_call(DMT_OPEN, dmt_cb);
	    if (DB_FAILURE_MACRO(status))
	    {
		if (dmt_cb->error.err_code == E_DM004D_LOCK_TIMER_EXPIRED)
		    dsh->dsh_error.err_code = E_QE1004_TABLE_NOT_OPENED;
		else
		    dsh->dsh_error.err_code = dmt_cb->error.err_code;
		dmt_cb->dmt_record_access_id = nolock_access_id;
		break;
	    }
	    else
	    {
		new_access_id = dmt_cb->dmt_record_access_id;
		dmt_cb->dmt_record_access_id = nolock_access_id;
		status = dmf_call(DMT_CLOSE, dmt_cb);
		dmt_cb->dmt_record_access_id = new_access_id;
		/* set the table record access ids into the DMR_CBs */
		((DMR_CB*) cbs[vl->vl_dmr_cb])->dmr_access_id =
					dmt_cb->dmt_record_access_id;

		if (status)
		{
		    i4 err;
		    qef_error(dmt_cb->error.err_code, 0L, status, &err,
			&dsh->dsh_error, 0);
		    break;
		}
	    }
	}

    }   /* end of the validation loop */

    if (status != E_DB_OK)
    {
	if ((dsh->dsh_error.err_code == E_QE0023_INVALID_QUERY ||
	     dsh->dsh_error.err_code == E_QE0301_TBL_SIZE_CHANGE) &&
	    dsh->dsh_qp_ptr->qp_ahd != action)
	{
	    status = E_DB_ERROR;
	}
	return (status);
    }

    /*
    ** Now we must handle any security audit requirements.
    */
    if  ( (action->ahd_audit != NULL) &&
	  (Qef_s_cb->qef_state & QEF_S_C2SECURE)
	)
    {
	QEF_AUD     *aud;
	QEF_ART     *art;
	i4          i;

	aud = (QEF_AUD *)action->ahd_audit;
	status = qeq_audit(qef_rcb, dsh, aud, &status);
	if (status != E_DB_OK)
	    return (status);
    }
    /*
    ** Check if query plan needs QUERY_SYSCAT privilege, if so
    ** make sure the session has it, and invalidate the query plan
    ** if not.
    */
    if (action->ahd_flags & QEA_NEED_QRY_SYSCAT)
    {
	/*
	** Query needs QUERY_SYSCAT, so verify session has it
	*/
	if (!(qef_cb->qef_fl_dbpriv & DBPR_QUERY_SYSCAT))
	{
	    /* Session missing priv, so invalidate */
	    status = E_DB_WARN;
	    dsh->dsh_qp_status |= DSH_QP_OBSOLETE;
	    dsh->dsh_error.err_code = E_QE0023_INVALID_QUERY;
	    return (status);
	}
    }

    /* Don't set dsh-act-ptr, subplan init does that after an action passes
    ** the "constant" test.
    */
    dsh->dsh_error.err_code = E_QE0000_OK;
    return (E_DB_OK);

} /* qeq_topact_validate */

/*
** Name: qeq_subplan_init - Initialize or reset a query (sub)plan
**
** Description:
**	This routine is the second half of the original qeq_validate.
**
**	A QP or part of a QP is to be executed, or reset.  We walk
**	the query plan sub-tree and do initialization / reset things:
**	- some final ADE_EXCB setup is done;
**	- some action CX's (keys, constants) are executed;
**	- VIRGIN segments in CX's are run, if they exist;
**	- partition pruning / qualification is set up, and constant
**	  qualifications are executed;
**	- orig keys are set up
**	- orig positioning is done (ORIGAGG only)
**
**	All of these things are done in the context of the thread that
**	will actually execute this part of the plan.  To that end, we'll
**	stop walking the tree at an EXCH node;  the exch's child thread(s)
**	will take care of things underneath.  A sub-plan might belong
**	to multiple threads (if under a 1:N exch), which is fine;  each
**	thread will run its subplan-init on the sub-plan, using data
**	structures private to that thread.
**
**	The caller should pass either an action (the usual case), or a
**	start-node (probably an EXCH), but never both.  The "node"
**	argument should always be NULL, except for recursive calls
**	from subplan-init itself.
**
**	The plan walk does not go below a QP node.  It could, but there is
**	no real benefit to doing so.  The QP node executor will complete
**	the subplan-init for the actions under the QP (selecting the proper
**	action for this thread, in the parallel union case).
**
** Inputs:
**	qef_rcb			QEF request control block for query
**	dsh			This thread's DSH
**	action			Action to start initializing at, or ...
**	start_node		QP node to start initializing at (most
**				likely an EXCH)
**	node			Node to process when moving recursively down
**				the tree;  outside callers ALWAYS pass NULL.
**
** Outputs:
**	Returns E_DB_OK or error status
**	Can return E_DB_WARN and QE0015 (no more rows), but *only* if called
**	from an action -- never happens when started in mid-QP.  (resettable
**	EXCH child depends on this.)
**
** History:
**	13-May-2010 (kschendel)
**	    Split from original qeq-validate while trying to make
**	    complex parallel queries work.
**	19-May-2010 (kschendel) b123759
**	    Move a couple variable declarations closer to their use point.
**	06-sep-2010 (maspa05) SIR 124363
**	    Trace point sc925 - log long-running queries
**	07-sep-2010 (maspa05) SIR 124363
**	    ult_trace_longqry now takes two arguments and returns a bool
**	10-Sep-2010 (kschendel) b124341
**	    Replace SEjoin's kcompare with cvmat.
*/

DB_STATUS
qeq_subplan_init(QEF_RCB *qef_rcb, QEE_DSH *dsh,
	QEF_AHD *act, QEN_NODE *start_node, QEN_NODE *node)
{
    ADE_EXCB	*ade_excb;
    ADF_CB	*adfcb = dsh->dsh_adf_cb;
    DB_STATUS	status = E_DB_OK;
    DMR_CB	*dmr_cb;
    i4		dmr_index;
    i4		i;
    i4		val1, val2;
    PTR		*cbs = dsh->dsh_cbs;
    QEF_CB	*qef_cb = dsh->dsh_qefcb;
    QEF_QP_CB	*qp = dsh->dsh_qp_ptr;
    QEN_ADF	*qen_adf;
    QEN_NODE	*inner, *outer;
    QEN_OJINFO	*oj;
    QEN_PART_QUAL  *pqual;

    if (qef_rcb->qef_pcount && qef_rcb->qef_param)
        dsh->dsh_param	= qef_rcb->qef_param->dt_data;
    dsh->dsh_result = (PTR) dsh->dsh_qef_output;
    dsh->dsh_error.err_code = E_QE0000_OK;

    /* run the timer initialization stuff if we don't have the overheads
    ** calculated yet for this query and DSH.
    */
    if (! dsh->dsh_stats_inited)
    {
	i4 cson;
	STATUS csret;
	TIMERSTAT init_tstat;
	i4 lqry_thresh;
 
	/* check whether sc925 is set */
 	ult_trace_longqry(&lqry_thresh,&val2);

	/* initialize the queries begining cpu, dio, and wall clock numbers */
 	if (ult_check_macro(&qef_cb->qef_trace, 91, &val1, &val2) ||
	    lqry_thresh != 0)
	{

	    cson = 1;
	    csret = CSaltr_session((CS_SID) 0, CS_AS_CPUSTATS,
			( PTR ) &cson);

	    if (csret != OK && csret != E_CS001F_CPUSTATS_WASSET)
	    {
		dsh->dsh_error.err_code = csret;
		return(E_DB_ERROR);
	    }

	    qen_bcost_begin(dsh, &init_tstat, NULL);

	    dsh->dsh_tcpu = init_tstat.stat_cpu;
	    dsh->dsh_tdio = init_tstat.stat_dio;
	    dsh->dsh_twc = init_tstat.stat_wc;
	}

	/* initialize cpu and dio overhead numbers if needed */
	if (dsh->dsh_qp_stats)
	{
	    cson = 1;
	    csret = CSaltr_session((CS_SID) 0, CS_AS_CPUSTATS,
			( PTR ) &cson);

	    if (csret != OK && csret != E_CS001F_CPUSTATS_WASSET)
	    {
		dsh->dsh_error.err_code = csret;
		return(E_DB_ERROR);
	    }

	    dsh->dsh_diooverhead = 0.0;	/* Silly to even have this */
	    if (dsh->dsh_cpuoverhead == 0.0)
	    {
		qen_bcost_begin(dsh, &init_tstat, NULL);

		/* need enough of these to get at least a couple of cpu-seconds,
		** else it's a waste of effort (and source of error).
		** cpu time is x10 so look for at least 2000 ms.
		** FIXME! CSstatistics, and hence bcost, measures in different
		** units on different platforms!  and unix platforms currently
		** manage to deliberately lose resolution below 10 ms!  this
		** area could use some attention...
		*/
#ifdef NT_GENERIC
#define QEQ_OVERHEAD_NEEDED 2000
#else
#define QEQ_OVERHEAD_NEEDED 200
#endif

		for (i = 1; i < 2000000; i++)
		{
		    TIMERSTAT loop_tstat;
		    qen_bcost_begin(dsh, &loop_tstat, NULL);
		    if (loop_tstat.stat_cpu - init_tstat.stat_cpu >= QEQ_OVERHEAD_NEEDED)
			break;
		}

		qen_ecost_end(dsh, &init_tstat, NULL);

		/* N bcost calls is N/2 b/e pairs;  just use a tiny value if
		** we can't generate enough overhead in 1 million pairs.
		*/
		if (init_tstat.stat_cpu >= QEQ_OVERHEAD_NEEDED)
		    dsh->dsh_cpuoverhead = (f4) init_tstat.stat_cpu / (f4)(i/2);
		if (dsh->dsh_cpuoverhead == 0.0)
		    dsh->dsh_cpuoverhead = 1.0e-10;
	    }
	}

	dsh->dsh_stats_inited = TRUE;
    } /* if stats not inited */

    /* further (re)initialize the ADE_EXCBs, run virgin segments */

    if (act != NULL)
    {
	/*
	** Fix B38161: Return E_QE0018_BAD_PARAM_IN_CB if qen_pcx_idx is
	**             positive but qef_pcount is 0.
	*/
	if (act->ahd_mkey &&
		    act->ahd_mkey->qen_pcx_idx && !qef_rcb->qef_pcount)
	{
	    dsh->dsh_error.err_code = E_QE0018_BAD_PARAM_IN_CB;
	    return (E_DB_ERROR);
	}

	/* Cursor- (remote-) update has no QP under it, but does have
	** CX's to initialize
	*/
	if (act->ahd_atype == QEA_RUP)
	{
	    status = qeq_ade(dsh, act->qhd_obj.qhd_qep.ahd_current);
	    if (status)
		return (status);

	    status = qeq_ade(dsh, act->qhd_obj.qhd_qep.ahd_constant);
	    return (status);

	}
	/* Other actions with no QP need no further effort */
	if ((act->ahd_atype <= QEA_DMU || act->ahd_atype >= QEA_UTL) &&
		(act->ahd_atype != QEA_RAGGF) &&
		(act->ahd_atype != QEA_HAGGF) &&
		(act->ahd_atype != QEA_LOAD) &&
		(act->ahd_atype != QEA_PUPD) &&
		(act->ahd_atype != QEA_RETURN) &&
		(act->ahd_atype != QEA_IF) &&
		(act->ahd_atype != QEA_PDEL)
	    )
	{
	    /* Set the current action in the DSH */
	    dsh->dsh_act_ptr = act;
	    return (E_DB_OK);
	}

	/* Copy first "n" and offset "n" values for SELECTs or CTAS. */
	if ((act->ahd_atype == QEA_GET || act->ahd_atype == QEA_APP ||
	     act->ahd_atype == QEA_LOAD)
	  && act->qhd_obj.qhd_qep.ahd_firstncb >= 0)
	{
	    QEA_FIRSTN	    *firstncbp;
	    PTR		    valp;
	    bool	    offval = FALSE, firval = FALSE;

	    firstncbp = (QEA_FIRSTN *)cbs[act->qhd_obj.qhd_qep.ahd_firstncb];
	    firstncbp->get_count = 0;

	    firstncbp->get_offsetn = act->qhd_obj.qhd_qep.ahd_offsetn;
	    if (act->qhd_obj.qhd_qep.ahd_qepflag & AHD_PARM_OFFSETN)
	    {
		if (qp->qp_status & QEQP_ISDBP)
		{
		    /* Procedure parameter or local variable - must be either
		    ** 2-byte integer or 4-byte integer. */

		    valp = dsh->dsh_row[act->qhd_obj.qhd_qep.ahd_offsetn];
		    if (qp->qp_row_len[act->qhd_obj.qhd_qep.ahd_offsetn]
							== 2)
			firstncbp->get_offsetn = *(i2 *)valp;
		    else firstncbp->get_offsetn = *(i4 *)valp;
		}
		else
		{
		    /* Repeat query parm - load value from param array. */
		    valp = dsh->dsh_param[act->qhd_obj.qhd_qep.ahd_offsetn];
		    if (qp->qp_params[act->qhd_obj.qhd_qep.ahd_offsetn-1]->
							db_length == 2)
			firstncbp->get_offsetn = *(i2 *)valp;
		    else firstncbp->get_offsetn = *(i4 *)valp;
		}
		offval = TRUE;
	    }

	    firstncbp->get_firstn = act->qhd_obj.qhd_qep.ahd_firstn;
	    if (act->qhd_obj.qhd_qep.ahd_qepflag & AHD_PARM_FIRSTN)
	    {
		if (qp->qp_status & QEQP_ISDBP)
		{
		    /* Procedure parameter or local variable - must be either
		    ** 2-byte integer or 4-byte integer. */

		    valp = dsh->dsh_row[act->qhd_obj.qhd_qep.ahd_firstn];
		    if (qp->qp_row_len[act->qhd_obj.qhd_qep.ahd_firstn] == 2)
			firstncbp->get_firstn = *(i2 *)valp;
		    else firstncbp->get_firstn = *(i4 *)valp;
		}
		else
		{
		    /* Repeat query parm - load value from param array. */
		    valp = dsh->dsh_param[act->qhd_obj.qhd_qep.ahd_firstn];
		    if (qp->qp_params[act->qhd_obj.qhd_qep.ahd_firstn-1]
							->db_length == 2)
			firstncbp->get_firstn = *(i2 *)valp;
		    else firstncbp->get_firstn = *(i4 *)valp;
		}
		firval = TRUE;
	    }

	    /* Validate "first n"/"offset n" values. */
	    if (firval && firstncbp->get_firstn <= 0)
	    {
		dsh->dsh_error.err_code = E_QE00A8_BAD_FIRSTN;
		status = E_DB_ERROR;
		return(status);
	    }
	    else if (offval && firstncbp->get_offsetn < 0)
	    {
		dsh->dsh_error.err_code = E_QE00A9_BAD_OFFSETN;
		status = E_DB_ERROR;
		return(status);
	    }
	}

	if (act->ahd_atype == QEA_AGGF ||
	    act->ahd_atype == QEA_RAGGF ||
	    act->ahd_atype == QEA_HAGGF)
	{
	    status = qeq_ade(dsh, act->qhd_obj.qhd_qep.ahd_by_comp);
	    if (status)
		return (status);
	}

	if (act->ahd_atype == QEA_HAGGF)
	{
	    status = qeq_ade(dsh, act->qhd_obj.qhd_qep.u1.s2.ahd_agoflow);
	    if (status)
		return (status);
	}

	status = qeq_ade(dsh, act->ahd_mkey);
	if (status)
	    return (status);

	/* create the keys */
	qen_adf	= act->ahd_mkey;
	if (qen_adf != NULL)
	{
	    ade_excb    = (ADE_EXCB*) cbs[qen_adf->qen_pos];
	    ade_excb->excb_seg = ADE_SMAIN;
	    status = ade_execute_cx(adfcb, ade_excb);
	    if (status != E_DB_OK)
	    {
		if ((status = qef_adf_error(&adfcb->adf_errcb,
			      status, qef_cb, &dsh->dsh_error)) != E_DB_OK)
		    return (status);
	    }
	    /* handle the condition where the qualification failed */
	    if (ade_excb->excb_value != ADE_TRUE)
	    {
		/* if qp->qp_status has bits set for QEQP_RPT
		** and QEQP_SHAREABLE and not set for QEQP_SQL_QRY,
		** we don't destroy the query plan */
		if(!(dsh->dsh_qp_ptr->qp_status & QEQP_SHAREABLE) ||
		   (dsh->dsh_qp_ptr->qp_status & QEQP_SQL_QRY)    ||
		   !(dsh->dsh_qp_ptr->qp_status & QEQP_RPT)
		  )
		    dsh->dsh_qp_status |= DSH_QP_OBSOLETE; /* destroy plan */
		/* else don't destroy plan */

		/* force recompile */
		dsh->dsh_error.err_code = E_QE0023_INVALID_QUERY;
		if (dsh->dsh_qp_ptr->qp_ahd == act)
		    status = E_DB_WARN;
		else
		    status = E_DB_ERROR;
		return (status);
	    }
	}

	if ((act->ahd_atype == QEA_RETURN) ||
	    (act->ahd_atype == QEA_IF))
	{
	    /* Note that although this is an "ok" return, dsh-act-ptr is
	    ** not set for these two actions ... not sure if that is
	    ** deliberate or irrelevant (kschendel).
	    */
	    return (status);
	}

	status = qeq_ade(dsh, act->qhd_obj.qhd_qep.ahd_current);
	if (status)
	    return (status);

	status = qeq_ade(dsh, act->qhd_obj.qhd_qep.ahd_constant);
	if (status)
	    return (status);

	if (act->qhd_obj.qhd_qep.ahd_qepflag & AHD_KEYSET)
	{
	    status = qeq_ade(dsh, act->qhd_obj.qhd_qep.ahd_scroll->ahd_rskcurr);
	    if (status)
		return (status);
	}

	/* check constant qualification
	** except in AGGF & SAGG which will evaluate this CX
	** in the action interpreter.
	** AGGF is special because it may finish a projection if the
	** constant qual says "false".
	** SAGGF is special because it runs the "current" CX even if
	** the constant qual says "false", and because it has a special
	** hack for ALTER TABLE ADD CONSTRAINT.
	*/

	if (act->ahd_atype != QEA_AGGF && act->ahd_atype != QEA_SAGG)
	{
	    qen_adf = act->qhd_obj.qhd_qep.ahd_constant;
	    if (qen_adf)
	    {
		ade_excb = (ADE_EXCB*) cbs[qen_adf->qen_pos];
		ade_excb->excb_seg = ADE_SMAIN;

		/* process the constant expression */
		status = ade_execute_cx(adfcb, ade_excb);
		if (status != E_DB_OK)
		{
		    status = qef_adf_error(&adfcb->adf_errcb, status,
					qef_cb, &dsh->dsh_error);
		    if (status != E_DB_OK)
			return (status);
		}
		/* handle the condition where the qualification failed.
		** Aggregates still have to run init and finalization.
		*/
		if (ade_excb->excb_value != ADE_TRUE)
		{
		    dsh->dsh_qef_rowcount = 0;
		    dsh->dsh_error.err_code = E_QE0015_NO_MORE_ROWS;
		    if (act->ahd_atype == QEA_RAGGF ||
			act->ahd_atype == QEA_HAGGF)
		    {
			/* initialize the aggregate result */
			qen_adf = act->qhd_obj.qhd_qep.ahd_current;
			ade_excb = (ADE_EXCB*) cbs[qen_adf->qen_pos];
			ade_excb->excb_seg = ADE_SINIT;
			if (qen_adf->qen_uoutput > -1)
			{
			    if (qp->qp_status & QEQP_GLOBAL_BASEARRAY)
				dsh->dsh_row[qen_adf->qen_uoutput] = dsh->dsh_qef_output->dt_data;
			    else
				ade_excb->excb_bases[ADE_ZBASE + qen_adf->qen_uoutput] =
				    dsh->dsh_qef_output->dt_data;
			}

			status = ade_execute_cx(adfcb, ade_excb);
			if (status != E_DB_OK)
			{
			    status = qef_adf_error(&adfcb->adf_errcb, status,
					qef_cb, &dsh->dsh_error);
			    if (status != E_DB_OK)
				return (status);
			}
			dsh->dsh_qef_remnull |= ade_excb->excb_nullskip;

			/* finalize the aggregate result now */
			ade_excb->excb_seg = ADE_SFINIT;
			status = ade_execute_cx(adfcb, ade_excb);
			if (status != E_DB_OK)
			{
			    status = qef_adf_error(&adfcb->adf_errcb, status,
					qef_cb, &dsh->dsh_error);
			    if (status != E_DB_OK)
				return (status);
			}
		    }
		    return (E_DB_WARN);
		}
	    }
	}
	/* Made it thru constant qual, set current DSH action now.
	** If this is done any sooner, cursor fetches that fail the constant
	** test will still retrieve rows!  the constant test is in the OPEN,
	** and there's no other mechanism to tell FETCH to not do anything.
	*/
	dsh->dsh_act_ptr = act;

	/* Looks like an action with a QP under it, start at the top. */

	node = act->qhd_obj.qhd_qep.ahd_qep;
	start_node = NULL;
    }

    /* An outside caller starting in the middle of the QP tree (ie, EXCH)
    ** will pass non-NULL start-node and NULL node and action.
    */
    if (start_node != NULL && node == NULL)
	node = start_node;

    if (node == NULL)
    {
	return (E_DB_OK);
    }

    /* Walk the QP starting at "node" and run CX inits, virgin segments.
    ** Don't walk through EXCH nodes because the EXCH and below need
    ** to be inited in the EXCH child thread(s) context.
    ** Although we could walk through QP nodes and do the QP actions,
    ** it's not really necessary, let qenqp init its sub-actions
    ** in the traditional and historical manner.  (which has the
    ** added benefit of assuring that only the proper action of a
    ** parallel union is inited by this thread, without any extra work
    ** needed here.)
    */

    /* Big loop to walk the tree */
    do
    {
	status = qeq_ade(dsh, node->qen_fatts);
	if (status)
	    return (status);

	status = qeq_ade(dsh, node->qen_prow);
	if (status)
	    return (status);

	dmr_index = -1;
	pqual = NULL;
	oj = NULL;
	switch (node->qen_type)
	{
	case QE_EXCHANGE:
	    /* Unless this exchange is the starting node, stop here.  The
	    ** exch's child thread will take it from this exch on down.
	    */
	    if (node != start_node)
		return (E_DB_OK);

	    /* Nothing to do for the exch itself, but init on down the tree */
	    outer = node->node_qen.qen_exch.exch_out;
	    inner = NULL;
	    break;

	case QE_TJOIN:
	    outer = node->node_qen.qen_tjoin.tjoin_out;
	    inner = NULL;
	    dmr_index = node->node_qen.qen_tjoin.tjoin_get;
	    status = qeq_ade(dsh, node->node_qen.qen_tjoin.tjoin_jqual);
	    if (status)
		return (status);

	    status = qeq_ade(dsh, node->node_qen.qen_tjoin.tjoin_isnull);
	    if (status)
		return (status);

	    pqual = node->node_qen.qen_tjoin.tjoin_pqual;
	    oj = node->node_qen.qen_tjoin.tjoin_oj;
	    break;

	case QE_KJOIN:
	    outer = node->node_qen.qen_kjoin.kjoin_out;
	    inner = NULL;
	    dmr_index = node->node_qen.qen_kjoin.kjoin_get;
	    dmr_cb = (DMR_CB*) cbs[dmr_index];
	    status = qeq_ade(dsh, node->node_qen.qen_kjoin.kjoin_kqual);
	    if (status)
		return (status);

	    status = qeq_ade(dsh, node->node_qen.qen_kjoin.kjoin_jqual);
	    if (status)
		return (status);

	    status = qeq_ade(dsh, node->node_qen.qen_kjoin.kjoin_iqual);
	    if (status)
		return (status);

	    /* *Note* !! don't set iqual for DMF until AFTER we
	    ** run the qeq-ade setup on it.  Qeq-ade sets the CX
	    ** segment to virgin.  We need it left at main, which
	    ** is one of the things set-qualfunc does.  (DMF has
	    ** no idea what it's calling and can't set the CX
	    ** segment, unlike QEF calls which do set the segment.)
	    */
	    qen_set_qualfunc(dmr_cb, node, dsh);

	    status = qeq_ade(dsh, node->node_qen.qen_kjoin.kjoin_kcompare);
	    if (status)
		return (status);

	    status = qeq_ade(dsh, node->node_qen.qen_kjoin.kjoin_key);
	    if (status)
		return (status);

	    pqual = node->node_qen.qen_kjoin.kjoin_pqual;
	    oj = node->node_qen.qen_kjoin.kjoin_oj;
	    break;

	case QE_CPJOIN:
	case QE_FSMJOIN:
	case QE_ISJOIN:
	    outer = node->node_qen.qen_sjoin.sjn_out;
	    inner = node->node_qen.qen_sjoin.sjn_inner;
	    status = qeq_ade(dsh, node->node_qen.qen_sjoin.sjn_joinkey);
	    if (status)
		return (status);

	    status = qeq_ade(dsh, node->node_qen.qen_sjoin.sjn_okcompare);
	    if (status)
		return (status);

	    status = qeq_ade(dsh, node->node_qen.qen_sjoin.sjn_okmat);
	    if (status)
		return (status);

	    status = qeq_ade(dsh, node->node_qen.qen_sjoin.sjn_itmat);
	    if (status)
		return (status);

	    status = qeq_ade(dsh, node->node_qen.qen_sjoin.sjn_jqual);
	    if (status)
		return (status);

	    oj = node->node_qen.qen_sjoin.sjn_oj;
	    break;

	case QE_SORT:
	    outer = node->node_qen.qen_sort.sort_out;
	    inner = NULL;
	    dmr_index = node->node_qen.qen_sort.sort_load;
	    status = qeq_ade(dsh, node->node_qen.qen_sort.sort_mat);
	    if (status)
		return (status);
	    break;

	case QE_TSORT:
	    outer = node->node_qen.qen_tsort.tsort_out;
	    inner = NULL;
	    dmr_index = node->node_qen.qen_tsort.tsort_load;
	    status = qeq_ade(dsh, node->node_qen.qen_tsort.tsort_mat);
	    if (status)
		return (status);
	    pqual = node->node_qen.qen_tsort.tsort_pqual;
	    break;

	case QE_HJOIN:
	    outer = node->node_qen.qen_hjoin.hjn_out;
	    inner = node->node_qen.qen_hjoin.hjn_inner;
	    status = qeq_ade(dsh, node->node_qen.qen_hjoin.hjn_btmat);
	    if (status)
		return (status);

	    status = qeq_ade(dsh, node->node_qen.qen_hjoin.hjn_ptmat);
	    if (status)
		return (status);

	    status = qeq_ade(dsh, node->node_qen.qen_hjoin.hjn_jqual);
	    if (status)
		return (status);

	    pqual = node->node_qen.qen_hjoin.hjn_pqual;
	    oj = node->node_qen.qen_hjoin.hjn_oj;
	    break;

	case QE_SEJOIN:
	    outer = node->node_qen.qen_sejoin.sejn_out;
	    inner = node->node_qen.qen_sejoin.sejn_inner;
	    status = qeq_ade(dsh, node->node_qen.qen_sejoin.sejn_oqual);
	    if (status)
		return (status);

	    status = qeq_ade(dsh, node->node_qen.qen_sejoin.sejn_kqual);
	    if (status)
		return (status);

	    status = qeq_ade(dsh, node->node_qen.qen_sejoin.sejn_okmat);
	    if (status)
		return (status);

	    status = qeq_ade(dsh, node->node_qen.qen_sejoin.sejn_itmat);
	    if (status)
		return (status);

	    status = qeq_ade(dsh, node->node_qen.qen_sejoin.sejn_cvmat);
	    if (status)
		return (status);

	    status = qeq_ade(dsh, node->node_qen.qen_sejoin.sejn_ccompare);
	    if (status)
		return (status);

	    break;

	case QE_QP:
	    status = qeq_ade(dsh, node->node_qen.qen_qp.qp_qual);
	    if (status)
		return (status);

	    /* As noted above, don't dive into the QP actions.  Let qenqp
	    ** do it, which will also select the correct action of a
	    ** parallel union automatically.
	    */
	    inner = outer = NULL;
	    break;

	case QE_TPROC:
	    inner = outer = NULL;
	    status = qeq_ade(dsh, node->node_qen.qen_tproc.tproc_qual);
	    if (status)
		return (status);
	    status = qeq_ade(dsh, node->node_qen.qen_tproc.tproc_parambuild);
	    if (status)
		return (status);
	    break;

	case QE_ORIG:
	case QE_ORIGAGG:
	{
	    QEN_ORIG *orig_node;
	    QEN_STATUS *ns;
	    QEN_TKEY *tkptr;
	    bool bcost = FALSE;
	    TIMERSTAT open_tstat;

	    inner = outer = NULL;
	    orig_node = &node->node_qen.qen_orig;
	    dmr_index = orig_node->orig_get;
	    dmr_cb = (DMR_CB*) cbs[dmr_index];
	    status = qeq_ade(dsh, orig_node->orig_qual);
	    if (status)
		return (status);

	    pqual = orig_node->orig_pqual;

	    /* Init status, prior to possible break (for MS Access
	    ** OR transform) */
	    ns = dsh->dsh_xaddrs[node->qen_num]->qex_status;
	    ns->node_status = QEN0_INITIAL;

	    if (orig_node->orig_flag & ORIG_MCNSTTAB)
	    {
		dmr_index = -1;
		if (orig_node->orig_qual)
		{	/* kludge to fill in buffer addr */
		    ADE_EXCB	*ade_excb;
		    ade_excb = (ADE_EXCB *)cbs[orig_node->orig_qual->qen_pos];
		    ade_excb->excb_bases[orig_node->orig_qual->qen_input + ADE_ZBASE]
					    = dsh->dsh_row[node->qen_row];
			/* use ORIG node row buffer for qual code */
		}
		break;	/* rest is DMR fiddling */
	    }

	    /* Setup qualification in DMF if appropriate */
	    /* *Note* !! don't set qual for DMF until AFTER we
	    ** run the qeq-ade setup on it.  Qeq-ade sets the CX
	    ** segment to virgin.  We need it left at main, which
	    ** is one of the things set-qualfunc does.  (DMF has
	    ** no idea what it's calling and can't set the CX
	    ** segment, unlike QEF calls which do set the segment.)
	    */
	    qen_set_qualfunc(dmr_cb, node, dsh);

	    /*
	    ** If qe90, prepare node stats for DMR_POSITION (72937)
	    ** (only if origagg)
	    */
	    if (node->qen_type == QE_ORIGAGG
	      && dsh->dsh_qp_stats)
	    {
		qen_bcost_begin(dsh, &open_tstat, ns);
		bcost = TRUE;
	    }

	    /* position the cursor to the first record */
	    /* But only if it's an ORIGAGG!  which is never done for a
	    ** partitioned table, or under an exchange.  So the necessary
	    ** table open has been done.
	    ** FIXME origagg positioning should be removed to qenorig, just
	    ** like ordinary orig positioning has been.
	    */
	    if (orig_node->orig_pkeys == 0)
	    {
		dmr_cb->dmr_position_type = DMR_ALL;

		dmr_cb->dmr_flags_mask = 0;
		if (orig_node->orig_flag & ORIG_READBACK)
		    dmr_cb->dmr_position_type = DMR_LAST;
		if (node->qen_type == QE_ORIGAGG)
		{	/* only position non-partitioned tables */
		    status = dmf_call(DMR_POSITION, dmr_cb);
		    if (status != E_DB_OK)
		    {
			if (dmr_cb->error.err_code == E_DM0055_NONEXT)
			{
			    status = E_DB_OK;
			}
			else
			{
			    dsh->dsh_error.err_code = dmr_cb->error.err_code;
			    return (status);
			}
		    }
		}
	    }
	    else
	    {
		bool	readback = (orig_node->orig_flag & ORIG_READBACK) != 0;
		bool	key_by_tid;

		/* sort the keys and determine which ones to use */
		tkptr = (QEN_TKEY*) cbs[orig_node->orig_keys];
		/*
		** If orig_keys haven't been constructed yet,
		** call qen_keyprep() to do it.
		*/
		if ( tkptr == (QEN_TKEY*)NULL )
		{
		    if ( status = qen_keyprep(dsh, node) )
			return(status);
		}
		else
		{
		    status = qeq_ksort(tkptr, orig_node->orig_pkeys,
			     dsh, (bool)TRUE, readback);
		    if (status != E_DB_OK)
			return(status);
		    ns->node_u.node_orig.node_keys = tkptr->qen_nkey;
				/* save for partitioned tables */
		}

		/* don't position on tids or unique keys because
		** these are handled in QEN_ORIG
		*/
		if (orig_node->orig_pkeys != NULL &&
		    orig_node->orig_pkeys->key_kor != NULL &&
		    orig_node->orig_pkeys->key_kor->kor_keys != NULL &&
		    orig_node->orig_pkeys->key_kor->kor_keys->kand_attno == 0
		    )
		{
		    key_by_tid = TRUE;
		}
		else
		{
		    key_by_tid = FALSE;
		}
		if (!(orig_node->orig_flag & ORIG_UKEY) &&
		   (key_by_tid == FALSE) &&
		    node->qen_type == QE_ORIGAGG)
		{
		    dsh->dsh_qef_rowcount = 0;
		    status = qen_position(tkptr, dmr_cb, dsh,
			orig_node->orig_ordcnt, readback);
		    if (status != E_DB_OK)
		    {
			if (dsh->dsh_error.err_code ==
					E_QE0015_NO_MORE_ROWS)
			{
			    orig_node->orig_flag &= ~ORIG_MAXOPT;
			    status = E_DB_OK;
			}
			else
			    return (status);
		    }
		    if (dmr_cb->dmr_flags_mask == DMR_PREV)
			ns->node_access = QEN_READ_PREV;
					/* descending sort logic */
		}
	    }
	    /*
	    ** If qe90, update node stats with cost of position (72937)
	    */
	    if (bcost)
	    {
		qen_ecost_end(dsh, &open_tstat, ns);
	    }

	} /* end of orig case block */

	default:
	    /* huh? */
	    inner = outer = NULL;
	    break;

	} /* node-type switch */

	if (oj != NULL)
	{
	    /* Process standard outer-join CX's */
	    status = qeq_ade(dsh, oj->oj_oqual);
	    if (status)
		return (status);

	    status = qeq_ade(dsh, oj->oj_equal);
	    if (status)
		return (status);

	    status = qeq_ade(dsh, oj->oj_rnull);
	    if (status)
		return (status);

	    status = qeq_ade(dsh, oj->oj_lnull);
	    if (status)
		return (status);
	}

	/* If this node has partition qualification, complete the
	** CX initialization;  and, if there are constant qualifications
	** (ie it's an orig-like node), execute them.
	*/
	if (pqual != NULL)
	{
	    QEE_PART_QUAL *rqual, *jq;

	    status = qeq_pqual_init(dsh, pqual, qeq_ade);
	    if (status != E_DB_OK)
		return (status);
	    rqual = &dsh->dsh_part_qual[pqual->part_pqual_ix];
	    if (pqual->part_node_type == QE_ORIG
	      || pqual->part_node_type == QE_KJOIN
	      || pqual->part_node_type == QE_TJOIN)
	    {
		/* Orig-like node, start out assuming all partitions
		** whether there's a qual to eval or not.
		*/
		MEfill(rqual->qee_mapbytes, -1, rqual->qee_constmap);
	    }
	    if (pqual->part_const_eval != NULL)
	    {
		/* evaluate, AND with constant map */
		status = qeq_pqual_eval(pqual->part_const_eval, rqual, dsh);
		if (status != E_DB_OK)
		    return (status);
		/* As an optimization, if the constant map restricted
		** the table to just one partition, don't bother with
		** any join-time stuff;  it's hard to beat 1 partition,
		** and total exclusion is unlikely for most queries.
		** FIXME? maybe "small" should work too rather than
		** just "one"?  need some experience here first.
		*/
		if (rqual->qee_qnext != NULL
		  && BTcount(rqual->qee_constmap,rqual->qee_qdef->nphys_parts) <= 1)
		{
		    do
		    {
			jq = rqual->qee_qnext;
			jq->qee_qualifying = FALSE;
			jq->qee_enabled = FALSE;
		    } while (jq->qee_qnext != NULL);
		}
	    }
	}

	/* reset the row pointer in the DMR_CB for safety. */
	if (dmr_index >= 0)
	{
	    /* set index for open cb */
	    dmr_cb = (DMR_CB*) cbs[dmr_index];
	    /* assign the output row and its size */
	    dmr_cb->dmr_data.data_address = dsh->dsh_row[node->qen_row];
	    dmr_cb->dmr_data.data_in_size = node->qen_rsz;
	}

	/* Recurse on inner, loop on outer or only.  This isn't a big deal,
	** just reduces the recursion depth a bit.
	*/
	if (outer == NULL)
	{
	    outer = inner;
	    inner = NULL;
	}
	if (inner != NULL)
	{
	    status = qeq_subplan_init(qef_rcb, dsh, NULL, NULL, inner);
	    if (status != E_DB_OK)
		return (status);
	}
	node = outer;
    } while (outer != NULL);

    dsh->dsh_error.err_code = E_QE0000_OK;
    return (status);
}

/*
** Name: qeq_validate_qp -- Validate a query plan
**
** Description:
**
**	This routine is called for the first action of a query plan.
**	It goes through the plan resource list and validates items
**	(tables and DB procedures) used by the plan, to ensure that
**	the plan is valid and no tables have changed definition
**	since the QP was compiled.
**
**	The table validation step picks up the first valid list
**	entry, opens it, and checks the timestamp on the table vs
**	the query plan (ie resource list entry).  The table is
**	left open to hold a control lock during execution.  If a
**	table validation fails, the QP is marked obsolete, and the
**	table definition is turfed out of RDF since RDF probably
**	has an obsolete version of the definition.
**
**	The DB procedure validation step applies strictly to table
**	procedures;  other uses of DB procs are handled separately.
**	Procedure validation involves finding the tproc QP, finding
**	or creating a DSH for it, and recursively validating the
**	tproc QP's resource list.  The QP is left locked and the
**	DSH left around;  the first tproc usage will grab the DSH
**	and use it.
**
**	In addition to validating, if the QP uses any sequences,
**	they are all opened here.
**
** Inputs:
**	qef_rcb			QEF request control block
**	dsh			(main) thread's data segment header
**	size_check		TRUE to run a table size check in addition
**				to the timestamp validation.
**				FALSE if no size check wanted (for situations
**				like DBproc continuation after commit or
**				rollback, when a validate error is a hard
**				error with no retry available).
**
** Outputs:
**	Returns E_DB_OK or error status; error info will be in dsh_error.
**
** History:
**	13-May-2010 (kschendel) b123565
**	    Split from qeq-validate whilst trying to figure out the
**	    parallel query validation fiasco.
**	18-Jun-2010 (kschendel) b123755
**	    More work on validation:  simplify table validation tracking,
**	    do tproc validation here instead of at action table-open time
**	    (so that the resource lists don't get mixed up).
**	    Rename to better reflect what the routine does.
**	21-jul-2010 (stephenb) b124107
**	    We need to verify that the iisequnece record with the correct
**	    sequence ID according to the DSH exists since this may not
**	    be checked in the parser if the query gets at the sequence through
**	    devious means. It is possible that the sequence has been dropped,
**	    and another one with the same name created, and we can get to here
**	    without noticing. We need to reject the plan if that happens.
**	    Re-created sequences with have a new sequence ID.
*/

static DB_STATUS
qeq_validate_qp(QEF_RCB *qef_rcb, QEE_DSH *dsh, bool size_check)
{
    DB_STATUS	status, status1;
    DMT_CB	*dmt_cb;
    i4		i;
    PTR		*cbs = dsh->dsh_cbs;
    RDF_CB	rdf_cb;
    QEE_RESOURCE  *resource;
    QEF_RESOURCE  *qpresource;
    QEF_CB	*qef_cb = dsh->dsh_qefcb;
    QEF_QP_CB	*qp = dsh->dsh_qp_ptr;
    QEF_VALID	*vl;
    ULM_RCB	ulm;

    /* set up to allocate memory out of the dsh's stream */
    STRUCT_ASSIGN_MACRO(Qef_s_cb->qef_d_ulmcb, ulm);
    ulm.ulm_streamid_p = &dsh->dsh_streamid;

    /* for each resource */
    status = E_DB_OK;
    for (qpresource = qp->qp_resources;
	 qpresource != NULL;
	 qpresource = qpresource->qr_next
	)
    {
	resource = &dsh->dsh_resources[qpresource->qr_id_resource];

	/* if this is a resource that needs validating */

	/* First check for in-memory table (MS Access OR transform) */
	if (qpresource->qr_type == QEQR_TABLE &&
	    qpresource->qr_resource.qr_tbl.qr_tbl_type == QEQR_MCNSTTBL)
	{
	    i4	tab_index;
	    QEF_MEM_CONSTTAB	*qef_con_p;
	    QEE_MEM_CONSTTAB	*qee_con_p;

	    tab_index = qpresource->qr_resource.qr_tbl.qr_lastvalid->vl_dmr_cb;
	    qef_con_p = qpresource->qr_resource.qr_tbl.qr_cnsttab_p;
	    qee_con_p = resource->qer_resource.qer_tbl.qer_cnsttab_p;
	    /* Init. MEM_CONSTTAB and stick addr in dsh_cbs */
	    qee_con_p->qer_tab_p = qef_con_p->qr_tab_p;
	    qee_con_p->qer_rowcount = qef_con_p->qr_rowcount;
	    qee_con_p->qer_rowsize = qef_con_p->qr_rowsize;
	    qee_con_p->qer_scancount = 0;
	    cbs[tab_index] = (PTR)qee_con_p;
			/* VERY IMPORTANT - stick QEE_MEM_CONSTTAB
			** ptr here for qen_orig to find later */
	    continue;	/* done, skip to next resource */
	}

	if (qpresource->qr_type == QEQR_TABLE &&
	     qpresource->qr_resource.qr_tbl.qr_tbl_type == QEQR_REGTBL &&
	     ! resource->qer_resource.qer_tbl.qer_validated
	    )
	{
	    /* Ordinary table, snag a valid list entry and use it to
	    ** open the table so that we can check its timestamp.
	    */
	    vl = qpresource->qr_resource.qr_tbl.qr_valid;

	    status = qeq_vopen(qef_rcb, dsh, vl);
	    dmt_cb = (DMT_CB *) cbs[vl->vl_dmf_cb];

	    if (status != E_DB_OK)
		break;

	    /* this is the real validation step for tables */
	    if (qpresource->qr_resource.qr_tbl.qr_timestamp.db_tab_high_time !=
			dmt_cb->dmt_timestamp.db_tab_high_time ||
		qpresource->qr_resource.qr_tbl.qr_timestamp.db_tab_low_time !=
			dmt_cb->dmt_timestamp.db_tab_low_time)
	    {
		/* relation out of date */
		dsh->dsh_qp_status |= DSH_QP_OBSOLETE;
		dsh->dsh_error.err_code = E_QE0023_INVALID_QUERY;
		status = E_DB_WARN;

		qeu_rdfcb_init((PTR) &rdf_cb, qef_cb);
		/* There's no need to imagine that this QP exists
		** in other servers, so keep this invalidation local.
		*/
		rdf_cb.rdf_rb.rdr_fcb = NULL;

		rdf_cb.rdf_rb.rdr_tabid.db_tab_base =
				   dmt_cb->dmt_id.db_tab_base;
		rdf_cb.rdf_rb.rdr_tabid.db_tab_index =
				   dmt_cb->dmt_id.db_tab_index;
		status1 = rdf_call(RDF_INVALIDATE, (PTR) &rdf_cb);
		if (DB_FAILURE_MACRO(status1))
		{
		    dsh->dsh_error.err_code = rdf_cb.rdf_error.err_code;
		    return(status1);
		}
		break;
	    }

	    /*
	    ** The semantics of the size_sensitive field in the valid
	    ** list is defined below:
	    **
	    **   If the size_sensitive is FALSE, the query plan is
	    **   totally independent of actual number of pages in the
	    **   relation
	    **
	    **   If the size_sensitive is TRUE, the the estimated number
	    **   of affected pages in the query plan must be adjusted by
	    **   the proportion of the actual page count (from DMF) to
	    **   the estimated page count (in the query plan).
	    **
	    **   If the difference of the adjusted value and the
	    **   estimated number (specified in the query plan) is
	    **   greater than a certain level (2 times + 5), the
	    **   query plan is invalid.
	    */

	    /* Skip all this if the number of pages hasn't changed */
	    if (vl->vl_size_sensitive && size_check &&
		vl->vl_total_pages != dmt_cb->dmt_page_count)
	    {
		i4	adj_affected_pages, adj_est_pages;
		i4	growth_factor;

		/*
		** scale estimated pages by growth factor, if any,
		** using integer arithmetic...
		*/
		growth_factor = ((dmt_cb->dmt_page_count -
				  vl->vl_total_pages) * 100)
				 / vl->vl_total_pages;

		/* Account for growth or shrinkage */

		adj_affected_pages = vl->vl_est_pages +
			((vl->vl_est_pages * growth_factor) / 100);

		/* if the scaled estimated pages is more than
		** 2 times the original estimate + 5, cause a recompile
		** with new RDF cache */

		/*
		** calculate comparand so that overflow becoming
		** negative can be detected
		*/
		adj_est_pages = (2 * vl->vl_est_pages) + 5;

		if (adj_affected_pages > adj_est_pages)
		{
		    dsh->dsh_qp_status |= DSH_QP_OBSOLETE;
		    dsh->dsh_error.err_code = E_QE0301_TBL_SIZE_CHANGE;
		    status = E_DB_WARN;

		    qeu_rdfcb_init((PTR) &rdf_cb, qef_cb);
		    /* Local invalidation only */
		    rdf_cb.rdf_rb.rdr_fcb = NULL;
		    rdf_cb.rdf_rb.rdr_tabid.db_tab_base =
				   dmt_cb->dmt_id.db_tab_base;
		    rdf_cb.rdf_rb.rdr_tabid.db_tab_index =
				   dmt_cb->dmt_id.db_tab_index;
		    status1 = rdf_call(RDF_INVALIDATE, (PTR) &rdf_cb);
		    if (DB_FAILURE_MACRO(status1))
		    {
			dsh->dsh_error.err_code = rdf_cb.rdf_error.err_code;
			return(status1);
		    }

		    break;
		}
	    }   /* end if size_sensitive */
	} /* if regular table */
	else if (qpresource->qr_type == QEQR_PROCEDURE
	  && ! resource->qer_resource.qer_proc.qer_procdsh_valid)
	{
	    QEE_DSH *tproc_dsh;

	    /* Validate a table procedure.
	    ** Load the QP and create a DSH for that QP.  Then, validate
	    ** the tproc's QP in the usual manner.  If something goes wrong,
	    ** drop the DSH, unlock the QP if it was there, and return
	    ** so that the caller can recreate the tproc's QP.
	    */
	    qef_rcb->qef_intstate &= ~QEF_DBPROC_QP;
	    tproc_dsh = resource->qer_resource.qer_proc.qer_proc_dsh;
	    *(dsh->dsh_saved_rcb) = *qef_rcb;
	    if (tproc_dsh == NULL)
	    {
		status = qeq_load_tproc_dsh(qef_rcb, qpresource, dsh, &tproc_dsh);
	    }
	    else
	    {
		/* Switch a few rcb things to the tproc context */
		qef_rcb->qef_dbpname = qpresource->qr_resource.qr_proc.qr_dbpalias;
		qef_rcb->qef_qp = resource->qer_resource.qer_proc.qer_proc_cursid;
		qef_rcb->qef_qso_handle = tproc_dsh->dsh_qp_handle;
	    }
	    if (status == E_DB_OK)
	    {
		/* Set the proper statement number, then validate */
		tproc_dsh->dsh_stmt_no = dsh->dsh_stmt_no;
		status = qeq_validate_qp(qef_rcb, tproc_dsh, size_check);
		if (status != E_DB_OK)
		{
		    /* Drop the unusable tproc DSH and its QP */
		    tproc_dsh->dsh_qp_status |= DSH_QP_OBSOLETE;
		    (void) qee_destroy(qef_cb, qef_rcb, &tproc_dsh);
		    resource->qer_resource.qer_proc.qer_proc_dsh = NULL;
		}
	    }
	    if (status != E_DB_OK)
	    {
		/* Not there, or was invalid, ask SCF to recompile.
		** qef_qp in the qef_rcb has the tproc QSF ID and qef_dbpname
		** in the qsf_rcb has the full alias for recompiling.
		** These are set by load-tproc-dsh;  in the case of
		** nested tprocs, the most deeply nested tproc's ID is
		** the one left in the qef RCB (which is the one that failed).
		**
		** Sequencer expects null qso-handle for DBproc recreate,
		** else it doesn't do all the right things.
		*/
		qef_rcb->qef_intstate |= QEF_DBPROC_QP;
		qef_rcb->qef_qso_handle = NULL;
		dsh->dsh_error.err_code = E_QE030F_LOAD_TPROC_QP;
		break;
	    }
	    /* The tproc looks OK, hang on to the DSH, someone can use it */
	    resource->qer_resource.qer_proc.qer_proc_dsh = tproc_dsh;
	    resource->qer_resource.qer_proc.qer_procdsh_used = FALSE;
	    resource->qer_resource.qer_proc.qer_procdsh_valid = TRUE;
	    /* Restore qef-rcb to what it looked like before switching to the
	    ** tproc dsh for validating.
	    */
	    *qef_rcb = *(dsh->dsh_saved_rcb);
	}
    }
    /* For each sequence (if any) - open with DMF call. */
    if (status == E_DB_OK)
    {
	DMS_CB		*dms_cb;
	QEF_SEQUENCE	*qseqp;
	DB_IISEQUENCE	seqtuple;

	for (qseqp = qp->qp_sequences; qseqp; qseqp = qseqp->qs_qpnext)
	{
	    /*
	    ** make sure iisequence record exists for DSH sequence ID. We need
	    ** to do this here because it may not have been checked already, and
	    ** it is possible to drop and re-create a sequence with the same name
	    ** which invalidates the sequence ID in the DSH.
	    */
	    qeu_rdfcb_init((PTR) &rdf_cb, qef_cb);
	    rdf_cb.rdf_rb.rdr_types_mask = RDR_BY_NAME;
	    rdf_cb.rdf_rb.rdr_2types_mask = RDR2_SEQUENCE;
	    MEmove(sizeof(DB_NAME), (PTR) &qseqp->qs_seqname, ' ',
		sizeof(DB_NAME), (PTR) &rdf_cb.rdf_rb.rdr_name.rdr_seqname);
	    STRUCT_ASSIGN_MACRO(qseqp->qs_owner, rdf_cb.rdf_rb.rdr_owner);
	    rdf_cb.rdf_rb.rdr_update_op = RDR_OPEN;
	    rdf_cb.rdf_rb.rdr_qtuple_count = 1;
	    rdf_cb.rdf_rb.rdr_qrytuple = (PTR)&seqtuple;
	    status = rdf_call(RDF_GETINFO, (PTR) &rdf_cb);
	    if (status != E_DB_OK)
	    {
		if (rdf_cb.rdf_error.err_code == E_RD0013_NO_TUPLE_FOUND)
		{
		    /* no iisequence record, must have been dropped */
		    dsh->dsh_qp_status |= DSH_QP_OBSOLETE;
		    dsh->dsh_error.err_code = E_QE0023_INVALID_QUERY;
		    return(E_DB_WARN);
		}
		else
		{
		    dsh->dsh_error.err_code = rdf_cb.rdf_error.err_code;
		    return(status);
		}
	    }
	    else if (seqtuple.dbs_uniqueid.db_tab_base != qseqp->qs_id.db_tab_base ||
		    seqtuple.dbs_uniqueid.db_tab_index != qseqp->qs_id.db_tab_index)
	    {
		/*
		** found record but it has the wrong sequence ID,
		** must have been re-created
		*/
		dsh->dsh_qp_status |= DSH_QP_OBSOLETE;
		dsh->dsh_error.err_code = E_QE0023_INVALID_QUERY;
		return(E_DB_WARN);
	    }

	    dms_cb = (DMS_CB *)dsh->dsh_cbs[qseqp->qs_cbnum];
	    dms_cb->dms_tran_id = dsh->dsh_dmt_id;
	    dms_cb->dms_db_id = qef_rcb->qef_db_id;
	    status = dmf_call(DMS_OPEN_SEQ, dms_cb);
	    if (status != E_DB_OK)
	    {
		dsh->dsh_error.err_code = dms_cb->error.err_code;
		return(status);
	    }
	}
    }

    return (status);
} /* qeq_validate_qp */

/*{
** Name: QEQ_VOPEN	- Open a valid list entry.
**
** Description:
**      This handles opening a valid struct by setting up for and calling
**	DMT_OPEN. 
**
** Inputs:
**  qef_rcb -
**	The rcb for the current query
**  action -
**	the current action being executed
**  dsh -
**	the dsh for the current query
**  vl -
**	the valid struct to open
**
** Outputs:
**
**	Returns:
**	    success or error status.
**	Exceptions:
**	    none
**
** Side Effects:
**	    The table gets opened.
**
** History:
**      5-oct-93 (eric)
**          created by extracting out of qeq_validate.
**      19-dec-94 (sarjo01) from 18-feb-94 (markm)
**          Added "mustlock" flag that ensures we don't do any dirty reads
**          when the query is an update (58681).
**	10-jul-96 (nick)
**	    Change method of determining 'temporary tables' ; some core
**	    catalogs can have zero timestamps and this was causing exclusive 
**	    locks to be taken on them. #77614
**	    Change temporary_table variable from i4  to bool. 
**      13-jun-97 (dilma04)
**          Set DMT_CONSTRAINT flag in the DMT_CB, if executing procedure 
**          that supports integrity constraint.
**	28-oct-1998 (somsa01)
**	    In the case of session temporary tables, its locks will reside on
**	    session lock list.  (Bug #94059)
**	12-nov-1998 (somsa01)
**	    Refined check for a Global Session Temporary Table.  (Bug #94059)
**	01-dec-2000 (thaju02)
**	    Addition of DMT_FORCE_NOLOCK, to prevent dmt_open/dmt_set_lock_value
**	    from overriding the DMT_N lock mode. (cont. B103150)
**	2-jan-04 (inkdo01)
**	    Change qeq_vopen() to external function.
**	22-jan-04 (inkdo01)
**	    Change qeq_vopen() back to static function.
**	8-Jun-2004 (schka24)
**	    Move setting of partition-master dmrcb slot to here.
**	30-Jul-2004 (jenjo02)
**	    Use dsh_dmt_id rather than qef_dmt_id transaction context.
**	29-Oct-2004 (jenjo02)
**	    Temp tables are readily identified by vl_flags & QEF_SESS_TEMP;
**	    remove other unreliable tests, specifically db_tab_base <=
**	    DM_SCONCUR_MAX which is unfortunately true for temp tables
**	    now that db_tab_base is an i4 instead of a u_i4.
**	2-Mar-2005 (schka24)
**	    It turns out that the original test that Jon replaced (above) was
**	    broken.  Unfortunately, that's what made temp tables work.
**	    In particular, we must not assume that gtt's always get opened
**	    in DIRECT mode;  otherwise, insert/select queries like
**	    insert into gtt select * from gtt
**	    never terminate, as the select keeps seeing the rows just
**	    inserted.
**	20-Nov-2009 (kschendel) SIR 122890
**	    Add needs-xlock support for upcoming insert/select via load.
**      01-Dec-2009 (horda03) B103150
**          Don't wait for a BUSY table lock.
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Tell DMF if table being opened is for a cursor.
**	26-Feb-2010 (jonj)
**	    Use DSH pointer as cursorid to dmt_open.
**	13-May-2010 (kschendel) b123565
**	    Delete "action" parameter, not real meaningful.
**	15-Jul-2010 (jonj)
**	    Reset DMT_FORCE_NOLOCK now that dmt_open no
**	    longer hoses dmt_flags_mask bits.
*/
static DB_STATUS
qeq_vopen(
	QEF_RCB	    *qef_rcb, 
	QEE_DSH	    *dsh, 
	QEF_VALID   *vl)
{
    PTR		*cbs = dsh->dsh_cbs;
    DMT_CB	*dmt_cb;
    DMR_CB	*dmrcb;
    DB_STATUS	status = E_DB_OK;
    QEF_QP_CB	*qp = dsh->dsh_qp_ptr;
    DMT_CHAR_ENTRY  dmt_char_entry[2];

    for (;;)
    {
	/* 
	** If there is no control block, or the table is already opened,
	** skip validation for this entry
	*/

	dmt_cb = (DMT_CB *) cbs[vl->vl_dmf_cb];
	if (dmt_cb == (DMT_CB *)NULL)
	    break;

	if (dmt_cb->dmt_record_access_id != (PTR)NULL)
	    break;   /* table is already opened */

	/* For partitioned tables, copy master DMR_CB addr to 
	** vl_dmr_cb location in array.  (Unless master slot is
	** already set, in which case this must be a REPEATED re-use.)
	*/
	if (vl->vl_partition_cnt > 1)
	    if (dsh->dsh_cbs[vl->vl_dmr_cb - 1] != NULL)
		dsh->dsh_cbs[vl->vl_dmr_cb] = dsh->dsh_cbs[vl->vl_dmr_cb - 1];
	    else
		dsh->dsh_cbs[vl->vl_dmr_cb - 1] = dsh->dsh_cbs[vl->vl_dmr_cb];

	if (vl->vl_flags & QEF_SET_INPUT)
	{
	    STRUCT_ASSIGN_MACRO(* (DB_TAB_ID *)
				(dsh->dsh_row[vl->vl_tab_id_index]),
				dmt_cb->dmt_id);
	}

	dmt_cb->dmt_tran_id	= dsh->dsh_dmt_id;
	dmt_cb->dmt_db_id = qef_rcb->qef_db_id;
	dmt_cb->dmt_sequence = dsh->dsh_stmt_no;
        dmt_cb->dmt_mustlock = vl->vl_mustlock;
	dmt_cb->dmt_flags_mask = 0;

	if (qef_rcb->qef_qacc == QEF_READONLY)
	    dmt_cb->dmt_access_mode = DMT_A_READ;
	else
	    dmt_cb->dmt_access_mode = vl->vl_rw;

	if ((qp->qp_status & QEQP_DEFERRED) != 0)
	    dmt_cb->dmt_update_mode = DMT_U_DEFERRED;
	else
	    dmt_cb->dmt_update_mode = DMT_U_DIRECT;

	if ( vl->vl_flags & QEF_SESS_TEMP )
	{
	    /* Lock mode is kinda irrelevant with session temps.
	    ** We do however want to pass the session-temp flag so that
	    ** any gtt locks live on the SCB lock list, not the XCB.
	    */
	    dmt_cb->dmt_lock_mode = DMT_X;
	    dmt_cb->dmt_flags_mask |= DMT_SESSION_TEMP;
	}
	else
	{
	    /* Use the QEP's estimated number of pages to set lock mode.
	    ** Page level locking will be used unless the query plan 
	    ** estimates that more than 10 pages will be used, in which
	    ** case table level locking will be used.  Table level locking
	    ** is always used with temporary tables.
	    **
	    ** This is a change from the previous strategy where DMT_SHOW 
	    ** was used to retrieve the actual page count of the relation 
	    ** and then the lockmode choice was based on the estimated page
	    ** count scaled by the amount the table had grown since the 
	    ** query plan had been built.  The problem with this is that 
	    ** making an extra DMT_SHOW for every query in a compiled DBP 
	    ** (like tp1) is significant overhead (especially on some 
	    ** machines like the RS6000 and decstation where cross process 
	    ** semaphores are very expensive).
	    **
	    ** Note that we need X table locks for LOAD.  For a load in
	    ** a CTAS, this doesn't matter, but for an INSERT load it does;
	    ** DMF doesn't do the normal heap locking when loading, and if
	    ** we don't ask for X locks, one can concurrently APPEND during
	    ** the load which will barf everything out.
	    ** (DMF insists on the X lock as a check, but besides that...)
	    */

			
	    /* default for lock granularity setting purposes */
	    switch (dmt_cb->dmt_access_mode)
	    {
		case DMT_A_READ:
		case DMT_A_RONLY:
		    dmt_cb->dmt_lock_mode = DMT_IS;
		    break;

		case DMT_A_WRITE:
		    if (vl->vl_flags & QEF_NEED_XLOCK)
			dmt_cb->dmt_lock_mode = DMT_X;
		    dmt_cb->dmt_lock_mode = DMT_IX;
		    break;
	    }

	    /*
	    ** Qef should not escalate to table locking if  vl_est_pages > 10.
	    ** Qef does not know the correct maxlock setting for this table.
	    ** 
	    ** QEF should also never escalate when a lower isolation level
	    ** has been specified.
	    ** QEF does not know the isolation level.
	    **
	    ** QEF could do a DMT_SHOW to find out these settings,
	    ** but to avoid an extra expensive DMT_SHOW, QEF will pass
	    ** the est_pages and total_pages values to dmf.
	    */
	    dmt_char_entry[0].char_id = DMT_C_EST_PAGES;
	    dmt_char_entry[0].char_value = vl->vl_est_pages;
	    dmt_char_entry[1].char_id = DMT_C_TOTAL_PAGES;
	    dmt_char_entry[1].char_value = vl->vl_total_pages;
	    dmt_cb->dmt_char_array.data_in_size = 2 * sizeof(DMR_CHAR_ENTRY);
	    dmt_cb->dmt_char_array.data_address = (PTR)dmt_char_entry;
	}	

        /*
        ** If executing a procedure that supports an integrity constraint, 
        ** set DMT_CONSTRAINTS flag in the DMT_CB.
        */
        if (qef_rcb->qef_intstate & QEF_CONSTR_PROC)
        {
            dmt_cb->dmt_flags_mask |= DMT_CONSTRAINT;
        }

	/* If opening for a cursor, tell DMF */
	if ( dsh->dsh_qp_status & DSH_CURSOR_OPEN )
	{
	    dmt_cb->dmt_flags_mask |= DMT_CURSOR;
	    dmt_cb->dmt_cursid = (PTR)dsh;
	}

	/* open and check time stamp for each non-temp table.
	** Don't wait if the lock cannot be granted.
	*/
        dmt_cb->dmt_flags_mask |= DMT_NO_LOCK_WAIT;

	status = qen_openAndLink(dmt_cb, dsh);

        /* Reset DMT_NO_LOCK_WAIT for future calls. */
        dmt_cb->dmt_flags_mask &= ~DMT_NO_LOCK_WAIT;

	if (status && dmt_cb->error.err_code == E_DM004D_LOCK_TIMER_EXPIRED)
	{
	    /* Try to do validate with lockmode DMT_N (B103150) */
	    dmt_cb->dmt_flags_mask |= DMT_FORCE_NOLOCK;
	    dmt_cb->dmt_lock_mode = DMT_N;
	    status = qen_openAndLink(dmt_cb, dsh);
	    /* Reset DMT_FORCE_NOLOCK for future calls. */
	    dmt_cb->dmt_flags_mask &= ~DMT_FORCE_NOLOCK;
	}

	/* Reset dmt_char_array for future calls */
	dmt_cb->dmt_char_array.data_address = 0;
	dmt_cb->dmt_char_array.data_in_size = 0;

        /*
        ** Turn off row update bit, since later calls may check for
        ** (in)valid flags_mask
        */
	if (status != E_DB_OK)
	{
	    if (dmt_cb->error.err_code == E_DM0054_NONEXISTENT_TABLE ||
		dmt_cb->error.err_code == E_DM0114_FILE_NOT_FOUND)
	    {
		dsh->dsh_qp_status |= DSH_QP_OBSOLETE;
		dsh->dsh_error.err_code = E_QE0023_INVALID_QUERY;
		status = E_DB_WARN;
	    }
	    else if (dmt_cb->error.err_code 
		      != E_DM0129_SECURITY_CANT_UPDATE)
	    {
		dsh->dsh_error.err_code = dmt_cb->error.err_code;
	    }
	    break;
	}

	/* Consistency check due to common AV */
	dmrcb = (DMR_CB *) cbs[vl->vl_dmr_cb];
	if (dmrcb == NULL)
	{
	    i4		err;
	    char		reason[100];

	    status = E_DB_ERROR;
	    STprintf(reason, "vl_dmr_cb = %d, cbs[%d] = NULL",
		     vl->vl_dmr_cb, vl->vl_dmr_cb);
	    qef_error(E_QE0105_CORRUPTED_CB, 0L, status, &err,
		      &dsh->dsh_error, 2,
		      sizeof("qeq_vopen")-1, "qeq_vopen",
		      STlength(reason), reason);
	    dsh->dsh_error.err_code = E_QE0025_USER_ERROR;
	    break;
	}
	/* set the table record access ids into the DMR_CBs.
	** Also store the valid list into the dmr-cb in case this is a
	** partitioned master open, helps qeq-part-open find the valid entry.
	*/
	dmrcb->dmr_access_id = dmt_cb->dmt_record_access_id;
	dmrcb->dmr_qef_valid = vl;
    }

    return(status);
}

/*{
** Name: QEQ_KSORT_MERGECOMP	- compare key items
**
** Description:
**	Used within qen_ksort_merge to compare two OR clauses within
**	a keyed index search.
**
** Inputs:
**
**	item1	\
**	item2	- Items to be compared - return >0 for item1>item2
**	dsh_key		dsh_key from function above
**	dsh		dsh from function above (for retrieval of data)
**
** History
**
**	31-Mar-2009 (kibro01) b121918
**	    Implemented.
**      31-Mar-2010 (maspa05) b123446
**          Return k1_next and k2_next to indicate whether there are
**          further uncompared attributes in either k1 or k2 
**          respectively. A return of 0 from this function means
**          the OR clauses sort in the same order it does NOT imply
**          they are duplicates as previously assumed.
*/

static i4
qen_ksort_mergecomp(QEN_NOR *item1, QEN_NOR *item2, PTR dsh_key, QEE_DSH *dsh,
		bool *k1_next,bool *k2_next)
{
    QEN_NKEY	    *key1;
    QEN_NKEY        *key2;
    DB_DATA_VALUE   adc_dv1;
    DB_DATA_VALUE   adc_dv2;
    i4		    result;
    i4		    status;

    key1 = item1->qen_key;
    key2 = item2->qen_key;

    result = -1;		
    do
    {
	/* compare the two OR lists - need to compare all elements
        ** if we are to eliminate equal keys */

	if (key1->nkey_att == NULL ||
	  key1->nkey_att->attr_attno != key2->nkey_att->attr_attno)
	    break;

	adc_dv1.db_datatype = key1->nkey_att->attr_type;
	adc_dv1.db_prec     = key1->nkey_att->attr_prec;
	adc_dv1.db_length   = key1->nkey_att->attr_length;
	adc_dv1.db_data	= (char *) dsh_key + 
				    key1->nkey_att->attr_value;
	adc_dv1.db_collID   = key1->nkey_att->attr_collID;

	adc_dv2.db_datatype = key2->nkey_att->attr_type;
	adc_dv2.db_prec     = key2->nkey_att->attr_prec;
	adc_dv2.db_length   = key2->nkey_att->attr_length;
	adc_dv2.db_data	= (char *) dsh_key + 
				key2->nkey_att->attr_value;
	adc_dv2.db_collID   = key2->nkey_att->attr_collID;
	status = adc_compare(dsh->dsh_adf_cb, &adc_dv1, 
	    &adc_dv2, &result);
	if (status != E_DB_OK)
	{
	    status = qef_adf_error(&dsh->dsh_adf_cb->adf_errcb, status, 
		dsh->dsh_qefcb, &dsh->dsh_error);
	    if (status == E_DB_ERROR)
		return (status);
	    /* else, status has been changed to OK and we continue
	    ** processing.
	    */
	}
	key1 = key1->nkey_next; /* look at multi-attribute keys
				    ** until the attributes are not
				    ** equal */
	key2 = key2->nkey_next;
    } while ( key1 && key2 && (result == 0));

    if (key1)
	    *k1_next=TRUE;
    else
	    *k1_next=FALSE;

    if (key2)
	    *k2_next=TRUE;
    else
	    *k2_next=FALSE;

    return result;
}

/*{
** Name: QEQ_KSORT_MERGE	- use a sort-merge to sort OR clauses
**
** Description:
**	Use a standard sort-merge technique to sort the keyed OR clauses
**	since in principle there could be hundreds of them.  This is 
**	especially true since when the OR combinations are being evaluated
**	in opc_meqrange the specific connections between values (e.g.
**	WHERE (A=:1 AND B=:2) OR (A=:3 AND B=:4) does not preserve the
**	connection between A=:1 and B=:2, so you end up with 4 OR clauses
**	(A=:1 AND B=:2), (A=:1 AND B=:4), (A=:3 AND B=:2), (A=:4 AND B=:4).
**	In that instant a better sort technique is vital, but it would be
**	of use anyway if there were index conditions such as 
**	A in (:1,:2,:3,...) AND B in (:100,:101,...) where a large number
**	of combinations would occur.
**
** Inputs:
**
**	list		Incoming list of OR clauses to be sorted
**	listlen		How many items are in the list
**	dupstart	Pointer to start of duplicates list to be removed
**	dupend		Pointer to keep track of end up duplicates list
**	desc		Is the sort descending?
**	dsh_key		dsh_key from function above
**	dsh		dsh from function above (for retrieval of data)
**
** History
**
**	31-Mar-2009 (kibro01) b121918
**	    Implemented.
**	17-Apr-2009 (kibro01) b121952
**	    Ensure whole of left list is preserved if all of the right list
**	    are found to be duplicates to the first element of the left list.
**      31-Mar-2010 (maspa05) b123446
**          Properly determine duplicates from qen_ksort_mergecomp - only if
**          all elements of left or right have been successfully compared -
**          result=0 is not of itself indicator of a duplicate, merely
**          that the clauses sort in the same order. 
*/

static QEN_NOR *
qen_ksort_merge(QEN_NOR *list, i4 listlen, QEN_NOR **dupstart, QEN_NOR **dupend,
		i4 desc, PTR dsh_key, QEE_DSH *dsh)
{
    QEN_NOR *leftlist, *rightlist, *endlist;
    i4 result;
    i4 leftlistlen, rightlistlen;
    i4 x;
    bool left_has_next,right_has_next;

    if (listlen >= 2)
    {
	leftlistlen = listlen / 2;
	leftlist = list;
	rightlist = list;

	for (x = 0; x < leftlistlen; x++)
	{
	    endlist = rightlist;
	    rightlist = rightlist->qen_next;
	}

	rightlistlen = listlen - leftlistlen;
	endlist->qen_next = (QEN_NOR*)NULL;
	if (leftlistlen > 1)
	    leftlist = qen_ksort_merge(leftlist, leftlistlen,
			 dupstart, dupend, desc, dsh_key, dsh);
	if (rightlistlen > 1)
	    rightlist = qen_ksort_merge(rightlist, rightlistlen,
			 dupstart, dupend, desc, dsh_key, dsh);

	endlist = list = NULL;
	while (leftlist && rightlist)
	{
	    result = qen_ksort_mergecomp(leftlist, rightlist, dsh_key, dsh,
			    &left_has_next, &right_has_next);

	    if ((result == 0) && (!(left_has_next && right_has_next)))
	    {
		    /* left and right ORs are the same from the pov of
		     * sorting - can one be removed as a duplicate? 
		     *
		     * if both have a next key then we didn't compare all
		     * keys and it's not a duplicate. Otherwise if A has a
		     * next and B not then B is a subset of A and we can
		     * treat A as a duplicate. If neither has a next key
		     * then it's an exact duplicate and we remove the
		     * one on the right (arbitrary but see below). 
		     */

                if (!(right_has_next))
		{
		    /* remove right OR as duplicate */
    	            if (*dupstart)
    	            {
    		        (*dupend)->qen_next = rightlist;
    		        *dupend = rightlist;
    	            } else
    	            {
    		        *dupstart = *dupend = rightlist;
    	            }
    		    rightlist = rightlist->qen_next;
		} else if (!(left_has_next))
		{
		    /* remove left OR as duplicate */
    	            if (*dupstart)
    	            {
    		        (*dupend)->qen_next = leftlist;
    		        *dupend = leftlist;
    	            } else
    	            {
    		        *dupstart = *dupend = leftlist;
    	            }
    		    leftlist = leftlist->qen_next;
		} 
	    } else if ( (result < 0 && !desc) || (result > 0 && desc) )
	    {
		if (list)
		{
		    endlist->qen_next = leftlist;
		    endlist = leftlist;
		} else
		{
		    endlist = list = leftlist;
		}
		leftlist = leftlist->qen_next;
	    } else
	    {
		if (list)
		{
		    endlist->qen_next = rightlist;
		    endlist = rightlist;
		} else
		{
		    endlist = list = rightlist;
		}
		rightlist = rightlist->qen_next;
	    }
	}
	/* list isn't set if there were precisely two items in the list
	** and they were identical */
	if (!list)
	{
		/* since we removed the right one as a duplicate (see above)
		 * make the list the left list */
		list = endlist = leftlist;
		leftlist = NULL; /* To avoid circular list reference */
		/* But use the whole of the left list, not just the first
		** element (kibro01) b121952
		*/
		while (endlist->qen_next)
			endlist = endlist->qen_next;
	}
	endlist->qen_next = NULL;

	/* Add any remaining values */
	if (leftlist)
	    endlist->qen_next = leftlist;
	if (rightlist)
	    endlist->qen_next = rightlist;
    }
    return list;
}


/*{
** Name: QEQ_KSORT	- key sorting and placement
**
** Description:
**      At this point, the user's parameters (dsh_param) and constants have
**  been converted to their key formats and reside in dsh_key. 
**  The QEF_KEY structures point at these keys. What we want to do is 
**  order the keys for maximum effectiveness. This requires that we
**  find the MIN of the MAX keys and the MAX of the MIN keys. Furthermore,
**  we need to sort the KOR clauses.
**
**  The first step is to order the keys within the ranges.
**  The second step is to order the ranges.
**  The third step is to set the current key pointer in qen_tkey to the
**  first. The first key is set in qeq_validate.
**
** Inputs:
**      qen_tkey                        top of actual keys pointer
**	qef_key				top of formal keys pointer
**	dsh_key				buffer containing keys
**	dsh				Data segment header for thread 
**
** Outputs:
**      qen_tkey			top of actual keys pointer
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    the elements pointed to by qen_tkey are filled in.
**
** History:
**      29-jul-86 (daved)
**          written
**	02-dec-88 (puree)
**	    break off from comparing OR keys loop if the keys are not the
**	    same attributes.
**	04-apr-89 (jrb)
**	    Initialized db_prec properly.
**      7-aug-91 (stevet)
**          Fix B37995: Added new parameter to qeq_ksort to indicate whether
**          finding the MIN of the MAX keys and MAX of the MIN keys is required.
**      20-jul-1993 (pearl)
**          Bug 45912.    On the 2nd invocation of a db proc, the QEN_NOR's are
**          sorted, and may be in a different order than it is assumed to be
**          in on the 1st invocation, when the structures are newly created.
**          Change the traversal of the QEN_NOR structures when ordering the
**          keys within the ranges in qeq_ksort().  Get the pointer to the
**          QEN_NOR structure from the dsh_kor_nor array.  Change the parameter
**          from dsh_key to pointer to dsh strucutre so we can get at the
**          array.  Change call to qeq_ksort()  to pass dsh instead
**          of dsh_key.
**	21-jul-98 (inkdo01)
**	    Support topsort-less descending sort by sorting ORs descending.
**	26-Mar-09 (kibro01) b121835
**	    Protect against null kattr pointer since we can now have inequality
**	    under an OR node (which previously was just equality combos).
**	31-Mar-09 (kibro01) b121918
**	    Use a sort-merge technique to sort keyed OR clauses rather than
**	    repeatedly searching for the maximum value to take from the list.
**	    Also fix a bug where the descending key would simply sort 
**	    randomly.
[@history_template@]...
*/
DB_STATUS
qeq_ksort(
QEN_TKEY           *qen_tkey,
QEF_KEY            *qef_key,
QEE_DSH            *dsh,
bool               keysort,
bool		   desc )
{
    ADF_CB	    *adf_cb;
    DB_STATUS	    status;
    QEF_KOR	    *qef_kor;
    QEF_KATT	    *qef_katt;
    QEF_KAND	    *qef_kand;
    QEN_NOR	    *qen_nor;
    QEN_NOR	    *qen_max;
    QEN_NOR	    *qen_1dup;	    /* duplicate values */
    QEN_NOR	    *qen_2dup;	    /* duplicate values */
    QEN_NOR	    **qen_prev;	    /* points to next list ptr */
    QEN_NOR	    **max_prev;	    /* pointer to next list ptr */
    QEN_NKEY	    *qen_nkey;
    PTR             dsh_key;
    DB_DATA_VALUE   adc_dv1;
    DB_DATA_VALUE   adc_dv2;
    i4		    result;
    i4		    listlen;

    if (qef_key == 0)
	return (E_DB_OK);

    adf_cb = dsh->dsh_adf_cb;
    dsh_key = dsh->dsh_key;
    /* 
    ** If keysort is TRUE, than order the keys within the ranges, if FALSE then
    ** just order the ranges.
    */
    if(keysort)
    {
	/* as we traverse the qef_key structure down the list of ranges (KOR)
	** nodes
	*/
    
	for 
	(
	    qef_kor = qef_key->key_kor;
	    qef_kor; 
	    qef_kor = qef_kor->kor_next
	)
	{

	    /* bug 45912: find the right QEN_NOR structure */
	    qen_nor = dsh->dsh_kor_nor[qef_kor->kor_id];
	    /* double check */
	    if (!qen_nor || qen_nor->qen_kor != qef_kor)
	    {
	    	dsh->dsh_error.err_code = E_QE0085_ERROR_SORTING;
		return (E_DB_ERROR);
	    }

	    /* find the min, max, or equality key for each attribute in the
	    ** range 
	    */
	    for
	    (
		qef_kand = qef_kor->kor_keys, qen_nkey = qen_nor->qen_key;
		qef_kand;
		qef_kand = qef_kand->kand_next, qen_nkey = qen_nkey->nkey_next
	    )    
	    {
		/* assume the first value is the desired key */
		qef_katt = qef_kand->kand_keys;
		qen_nkey->nkey_att = qef_katt;
		if (!qef_katt)
			continue;

		/* if this is a min or max list and there are more
		** elements in it, find the max or min.
		** Note that we know this is a min or max list if there are
		** multiple elements in it because we do not group equality
		** keys in this manner. Equality keys can be grouped in
		** separate ranges (KOR lists).
		*/
		for 
		(
		    qef_katt = qef_katt->attr_next;
		    qef_katt;
		    qef_katt = qef_katt->attr_next
		)
		{
		    /* set new value */
		    adc_dv1.db_datatype = qef_katt->attr_type;
		    adc_dv1.db_prec     = qef_katt->attr_prec;
		    adc_dv1.db_length   = qef_katt->attr_length;
		    adc_dv1.db_data	    = (char *) dsh_key + qef_katt->attr_value;
		    adc_dv1.db_collID   = qef_katt->attr_collID;

		    /* set current max */
		    adc_dv2.db_datatype = qef_katt->attr_type;
		    adc_dv2.db_prec     = qef_katt->attr_prec;
		    adc_dv2.db_length   = qef_katt->attr_length;
		    adc_dv2.db_data	    = (char *) dsh_key + 
					qen_nkey->nkey_att->attr_value;
		    adc_dv2.db_collID   = qef_katt->attr_collID;
		    status = adc_compare(adf_cb, &adc_dv1, 
			&adc_dv2, &result);
		    if (status != E_DB_OK)
		    {
			status = qef_adf_error(&adf_cb->adf_errcb,
			     status, dsh->dsh_qefcb, &dsh->dsh_error);
			if (status == E_DB_ERROR)
			    return (status);
			/* else, status has been changed to OK and we continue
			** processing.
			*/
		    }
		    if 
		    (
			/* a new max */
			(
			    result < 0 
			    && 
			    qef_kand->kand_type == QEK_MIN 
			)
			||
			(
			    result > 0
			    &&
			    qef_kand->kand_type == QEK_MAX
			)
		    )
		    qen_nkey->nkey_att = qef_katt;
		}
	    }
	}
    }
    /* put this OR list into the proper order off of qen_tkey. Proper
    ** order is defined as increasing order. This can be changed
    ** in the future (ie when we can perform merge sorts in decreasing
    ** order
    */
    /* for each OR element, find the max of the remaining elements. Put this
    ** element at the front of the queue. This will have the effect of
    ** sorting the keys in increasing order.
    ** This is currently only performed for single attribute keys. more
    ** precisely, only the first key is considered.
    */

    qen_1dup		= NULL;
    qen_2dup		= NULL;
    qen_tkey->qen_nkey	= NULL;

    listlen = 0;
    qen_nor = qen_tkey->qen_keys;
    while (qen_nor)
    {
	qen_nor = qen_nor->qen_next;
	listlen++;
    }

    qen_tkey->qen_nkey = qen_ksort_merge(qen_tkey->qen_keys, listlen,
				&qen_1dup, &qen_2dup, desc, dsh_key, dsh);

    /* reset the list */
    /* next key pointer is first key in list */
    qen_tkey->qen_keys	    = qen_tkey->qen_nkey;

    /* attach the duplicate KOR nodes, they will be reused later 
    ** note that the next key pointer will not point to the beginning of the
    ** list.
    */
    if (qen_1dup)
    {
	qen_2dup->qen_next = qen_tkey->qen_keys;
	qen_tkey->qen_keys = qen_1dup;
    }
    return (E_DB_OK);
}

/*{
** Name: QEQ_ADE_BASE		- initialize a set of BASE pointers
**
** Description:
**  A given address array is initialised by reading the given qen_base array 
**  and generating addresses into the address array.
**
** Inputs:
**      dsh                             current data segment header
**      qen_base			array of bases
**	excb_base			pointers to bases
**	num_bases			Number of bases to process	
**	param				user parameter array
**
** Outputs:
**	excb_base			array updated with pointers to rows
**	    none
**
** Side Effects:
**	    None.
**
** History:
**      03-dec-1992 (kwatts)
**          Broken out of qeq_ade code. qeq_ade now calls this to perform the
**	    lowest level of QEN_BASE conversion. 
**          virgin segment.
**      20-may-94 (eric)
**          Split this routine into two parts: qee_ade_base (called by qee_ade)
**          and qeq_ade_base (continued to be called by qeq_ade). All static
**          CX base initializations go into qee_ade_base() and all query
**          startup specific initializations go into qeq_ade_base(). This is
**          to improve performance.
**	26-aug-04 (inkdo01)
**	    Support global base array in parameter address resolution.
**	19-Apr-2007 (kschendel) b122118
**	    Clean up historical C strangenesses.
**	    Delete obsolete adf-ag stuff.
[@history_line@]...
[@history_template@]...
*/
static void
qeq_ade_base(
QEE_DSH            *dsh,
QEN_BASE	   *qen_base,
PTR		   *excb_base,
i4		   num_bases,
PTR		   *param )
{
    DB_STATUS	    	status = E_DB_OK;
    i4			i;

    /* Convert each base to the correct address */

    for (i = 0; i < num_bases; i++, qen_base++, excb_base++)
    {
	switch (qen_base->qen_array)
	{
	    case QEN_PARM:
		if (dsh->dsh_qp_ptr->qp_status & QEQP_GLOBAL_BASEARRAY)
		    dsh->dsh_row[qen_base->qen_parm_row] =
			param[qen_base->qen_index];
		else *excb_base = param[qen_base->qen_index];
		break;

	    case QEN_PLIST:
		*excb_base = (PTR) dsh->dsh_usr_params[qen_base->qen_index];
		break;

	    case QEN_OUT:
	    case QEN_VTEMP:
	    case QEN_DMF_ALLOC:
		*excb_base = 0;
		break;

	    case QEN_ROW:
	    case QEN_KEY:
		break;
	}
    }
    return;
}

/*{
** Name: QEQ_ADE	- initialize the ADE_EXCB, run VIRGIN code
**
** Description:
**	This is a possibly repeated query startup initialization.
**	It does these things:
**	- sets up ADE_EXCB bases that depend on user parameters, which
**	  obviously can change from execution to execution.  If
**	  ADE scratch space for VLT/VLUP is needed, it's (re)allocated here.
**
**	- runs the VIRGIN code segment of the CX, if there is one.
**
**	- resets the CX selector to MAIN.  While this is in general just
**	  a convenience, in a couple situations it's essential.  One
**	  example is the GET action with no QP (e.g. an assignment stmt
**	  in a DBproc, or a GET to return a SAGG result).  These GETs
**	  fiddle the CX select to be FINIT, so that if called back without
**	  reset, the GET can indicate EOF.  (There is no place for
**	  actions to store runtime status, unlike nodes.)  The reset
**	  ends up here where we reset the CX selector to allow the
**	  GET to be executed again.
**
** Inputs:
**      dsh                             current data segment header
**      qen_adf                         ade_excb initializer
**
** Outputs:
**      dsh
**	    .dsh_error.err_code		errors in allocating space
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    The ade_excbs are initialized in the DSH
**
** History:
**      1-aug-86 (daved)
**          written 
**      10-dec-87 (puree)
**          Converted all ulm_palloc to qec_palloc
**	10-feb-88 (puree)
**	    Convert ulm_openstream to qec_mopen.  Convert qec_palloc to
**	    qec_malloc.
**	11-feb-88 (puree)
**	    Using ADE_SVIRGIN CX to initialize VLT dsh's.
**	02-sep-88 (puree)
**	    New architecture for VLUP/VLT scratch space handling.  Each
**	    ADE_CX needs its own space.
**	24-mar-89 (jrb)
**	    Cleaned up adf_ag_struct initialization; added db_prec.
**      5-dec-89 (eric)
**          make constant expressions go faster by putting them into the
**          virgin segment.
**	04_dec_1992 (kwatts)
**	    Call new routine qeq_ade_base to create the table of pointers
**	    from bases.
**      20-May-94 (eric)
**          We now only execute this routine if we need to do query startup
**          specific initializations, or if there is a Virgin segment to
**          execute. All static (non-query startup specific issues)
**          initializations are now in qee_ade().
**      11-Aug-2003 (huazh01)
**          Initialize the memory returned by qen_mopen() to zero before we
**          execute the VIRGIN instructions. 
**          This fixes bug 110196. INGSRV2253.
**	26-aug-04 (inkdo01)
**	    Add support for global base array.
**	11-march-2008 (dougi)
**	    Fix broken ade_excb->excb_bases[ADE_GLOBALBASE] in parallel plans.
**	4-Jun-2009 (kschendel) b122118
**	    param arg was always dsh_param, get rid of it so that we can
**	    share call sequence with qee_ade.  (Will be useful for the
**	    extended partition qualification stuff, later on.)
**	21-May-2010 (kschendel) b122775
**	    Leave CX segment at main.  This is called from reset and it's
**	    reasonable to expect CX segment selectors to be reset too.
**	    In the case of GET, it's essential, since QP-less GET fiddles
**	    the CX segment to "FINIT" to indicate EOF (there is no other
**	    place for an action to keep state, unlike nodes).
*/
DB_STATUS
qeq_ade(
QEE_DSH            *dsh,
QEN_ADF            *qen_adf)
{
    ADE_EXCB			*ade_excb;
    QEN_BASE			*qen_base;
    PTR				*excb_bases;
    PTR				*bases;
    QEE_VLT			*vlt;
    GLOBALREF QEF_S_CB		*Qef_s_cb;
    i4				length;
    DB_STATUS			status = E_DB_OK;
    ULM_RCB			ulm;

    if (qen_adf == NULL)
	return (E_DB_OK);	/* No CX */

    ade_excb = (ADE_EXCB*) dsh->dsh_cbs[qen_adf->qen_pos];
    ade_excb->excb_seg = ADE_SMAIN;	/* Reset to MAIN */
    if ( (qen_adf->qen_mask & (QEN_SVIRGIN | QEN_HAS_PARAMS)) == 0 )
    {
	return (E_DB_OK);
    }

    excb_bases  = &ade_excb->excb_bases[ADE_ZBASE];
    qen_base = qen_adf->qen_base;
    if (dsh->dsh_qp_ptr->qp_status & QEQP_GLOBAL_BASEARRAY)
    {
	bases = dsh->dsh_row;
	ade_excb->excb_bases[ADE_GLOBALBASE] = (PTR)bases; /* reset in case broken
							** by parallel plan */
    }
    else bases = ade_excb->excb_bases;

    /* initialize the temps */
    if ((qen_adf->qen_mask & QEN_HAS_PARAMS) != 0)
    {
	qeq_ade_base(dsh, qen_base, excb_bases, qen_adf->qen_sz_base,
		dsh->dsh_param);
    }

    /* create the VLUP/VLT scratch space if reqiured */

    if (qen_adf->qen_pcx_idx > 0)
    {
	/* get the length of space for VLUP/VLT */
	status = ade_vlt_space(dsh->dsh_adf_cb, ade_excb->excb_cx,
	    ade_excb->excb_nbases, bases, &length);

	if (status)
	{
	    if ((status = qef_adf_error(&dsh->dsh_adf_cb->adf_errcb, 
		status, dsh->dsh_qefcb, &dsh->dsh_error)) != E_DB_OK)
		return (status);
	}

	if (length > 0)
	{
	    vlt  = &dsh->dsh_vlt[(qen_adf->qen_pcx_idx) - 1];

	    if (vlt->vlt_size < length)
	    {
		STRUCT_ASSIGN_MACRO(Qef_s_cb->qef_d_ulmcb, ulm);

		/* if the space exists but is not big enough, release it */
		ulm.ulm_streamid_p = &vlt->vlt_streamid;

		if (vlt->vlt_streamid != (PTR)NULL)
		{
		    status = ulm_closestream(&ulm);
		    if (status != E_DB_OK)
		    {
			dsh->dsh_error.err_code = ulm.ulm_error.err_code;
			return (status);
		    }
		    /* ULM has nullified vlt_streamid */
		}

		/* Open the stream and allocate the new space in one action */
		ulm.ulm_flags = ULM_PRIVATE_STREAM | ULM_OPEN_AND_PALLOC;
		ulm.ulm_psize = ulm.ulm_blocksize = length;

		status = qec_mopen(&ulm);
		if (status != E_DB_OK)
		{
		    dsh->dsh_error.err_code = ulm.ulm_error.err_code;
		    return (status);
		}
		/* ULM has stored streamid in vlt_streamid */

		/* store information in the descriptor */
		vlt->vlt_ptr = ulm.ulm_pptr;
		vlt->vlt_size  = length;

		/* b110196, provide clean working area */ 
		MEfill(length, 0, vlt->vlt_ptr); 
	    }

	    /* set the base address */
	    ade_excb->excb_bases[ADE_VLTBASE] = vlt->vlt_ptr;
	}
	else
	{
	    ade_excb->excb_bases[ADE_VLTBASE] = (PTR)NULL;
	}
    }
    else
    {
	ade_excb->excb_bases[ADE_VLTBASE] = (PTR)NULL;
    }

    if ((qen_adf->qen_mask & QEN_SVIRGIN) != 0)
    {
        /* Now execute the SVIRGIN segment of the CX */
	ade_excb->excb_seg = ADE_SVIRGIN;
	status = ade_execute_cx(dsh->dsh_adf_cb, ade_excb);
	if (status != E_DB_OK)
	{
	    if ((status = qef_adf_error(&dsh->dsh_adf_cb->adf_errcb,
			  status, dsh->dsh_qefcb, &dsh->dsh_error)) != E_DB_OK)
		return (status);
	}
	ade_excb->excb_seg = ADE_SMAIN;		/* Reset to MAIN */
    }

    return (status);
}

/*{
** Name: QEQ_AUDIT	- Audit table, view and alarm events.
**
** Description:
**      When any query is run the table and view access must
**      be audited.  Also if alarm events are associated 
**      with the query these must be audited.
**
** Inputs:
**      qef_rcb                 Pointer to qef control block.
**      aud                     Pointer to audit information.
**      status                  Status of qeu_audit call to determine
**                              success or failure auditing.
**	    
**
** Outputs:
**	qef_rcb
**	    .error.err_code	Any error returned from
**                              SCF or DMF unless status
**                              indicates failure, then original
**                              error remains.
**	Returns:
**	    E_DB_{OK, WARN, ERROR, FATAL}
**	    NB 'status' and error code will refer to the _last_ error
**	       if multiple errors are found.
**	Exceptions:
**	    none
**
** Side Effects:
**
** History:
**	07-jul-89 (jennifer)
**          written
**	19-nov-1992 (pholman)
**	    Modified to use qeu_secaudit rather than the obsolete qeu_audit
**	10-mar-1993 (markg)
**	    Modified call to qeu_secaudit so that audit records don't get 
**	    flushed every time a security alarm is triggered.
**	06-jul-93 (robf)
**	   Merge auditing access to secure catalogs here, and make generic
**	   so we don't need to list all the possible secure catalogs (which
**	   tended to get out of date)
**	24-nov-93 (robf)
**         Fire alarms by group and role as well as user/public
*/
static
DB_STATUS
qeq_audit(
QEF_RCB      *qef_rcb,
QEE_DSH	     *dsh,
QEF_AUD      *aud,
DB_STATUS   *status )
{
    QEF_CB	    *qef_cb = dsh->dsh_qefcb;
    DB_STATUS       local_status;
    QEF_ART         *art;
    QEF_APR         *apr;
    DB_ERROR	    e_error;
    i4              i;
    i4		    j;
    i4		    eventtype;
    i4	    msgid;

    for (; aud; aud = aud->qef_next)
    {
	/* 
        ** Go through the list of audit blocks and perform the audits.
        */

	/*  Process Audit's. */

	for (i=0, art=aud->qef_art; 
	     i< aud->qef_cart && Qef_s_cb->qef_state & QEF_S_C2SECURE;
	     i++)
	{
	    if (art[i].qef_tabtype & QEFART_VIEW)
	    {
		eventtype = SXF_E_VIEW;    
		msgid     = I_SX2027_VIEW_ACCESS;
	    }
	    else
	    {
		/* Default is table access */
		eventtype = SXF_E_TABLE;
		msgid     = I_SX2026_TABLE_ACCESS;
	    }
	    local_status = qeu_secaudit(FALSE, qef_cb->qef_ses_id, 
	      		    art[i].qef_tabname.db_tab_name,
			    &art[i].qef_ownname, sizeof(DB_TAB_NAME),
			    eventtype, msgid, 
			    (SXF_A_SUCCESS | art[i].qef_mode), &e_error);

	    if (local_status != E_DB_OK)
	    {
		if (*status == E_DB_OK)
		{
		    dsh->dsh_error = e_error;
		    *status = local_status;
		}
	    }
	    /*
	    ** If updating a secure catalog log a security  event
	    */
	    if((art[i].qef_tabtype&QEFART_SECURE)  && 
		art[i].qef_mode!=SXF_A_SELECT)
	    {
		     local_status = qeu_secaudit(TRUE, qef_cb->qef_ses_id, 
	      		   art[i].qef_tabname.db_tab_name,
			   &art[i].qef_ownname, sizeof(DB_TAB_NAME),
	      		   SXF_E_SECURITY,
	      		   I_SX201C_SEC_CAT_UPDATE, 
			   (SXF_A_SUCCESS | art[i].qef_mode), &e_error);
		    if (local_status != E_DB_OK)
		    {
			if (*status == E_DB_OK)
			{
			    dsh->dsh_error = e_error;
			    *status = local_status;
			}
		    }
	    }
	}

	/*  Process Alarm's. */
	for (j=0, apr = aud->qef_apr; j < aud->qef_capr; j++)
	{
	    /*
	    ** Find the ART for this AUD to get table info.
	    */
	    for (i = 0; i < aud->qef_cart; i++)
	    {
		if (apr[j].qef_tabid.db_tab_base ==
		    art[i].qef_tabid.db_tab_base
			&&
		    apr[j].qef_tabid.db_tab_index ==
		    art[i].qef_tabid.db_tab_index)
		{
		    /*
		    ** Got the ART info, so check if firing the alarm
		    */
		    if (!(apr[j].qef_user.db_own_name[0] == ' ' ||  ( 
			/* USER */
			apr[j].qef_gtype==QEFA_USER &&
			!MEcmp( ( PTR ) &apr[j].qef_user,
			       ( PTR ) &qef_cb->qef_user,
			       ( u_i2 ) sizeof(qef_cb->qef_user ) )  
		     ) || (
			/* GROUP */
			apr[j].qef_gtype==QEFA_GROUP &&
			!MEcmp( ( PTR ) &apr[j].qef_user,
			       ( PTR ) qef_cb->qef_group,
			       ( u_i2 ) sizeof(apr[i].qef_user ) )  
		     ) || (
			/* ROLE */
			apr[j].qef_gtype==QEFA_APLID &&
			!MEcmp( ( PTR ) &apr[j].qef_user,
			       ( PTR ) qef_cb->qef_aplid,
			       ( u_i2 ) sizeof(apr[i].qef_user ) )  
		     )))
		    {
			/*
			** Not firing the alarm so go to the next
			*/
			break;
		    }
		    /*
		    ** Fire the alarm
		    */
		    /*
		    ** Write audit record for alarm.
		    */
		    if ( Qef_s_cb->qef_state & QEF_S_C2SECURE )
		    {
			local_status = qeu_secaudit(FALSE, qef_cb->qef_ses_id, 
				art[i].qef_tabname.db_tab_name,
				&art[i].qef_ownname, sizeof(DB_TAB_NAME),
				SXF_E_ALARM, I_SX2028_ALARM_EVENT, 
				(SXF_A_SUCCESS | art[i].qef_mode), &e_error); 

			if (local_status != E_DB_OK)
			{
			    if (*status == E_DB_OK)
			    {
				dsh->dsh_error = e_error;
				*status = local_status;
			    }
			}
		    }
		    /*
		    ** Raise dbevents if necessary
		    */
		    if(apr[j].qef_flags & QEFA_F_EVENT)
		    {
		        char evtxt[DB_EVDATA_MAX+1];
			/*
			** If no event text add some
			*/
			if(!apr[j].qef_ev_l_text)
			{
			   STprintf(evtxt,ERx("Security alarm raised on table %.*s.%.*s on success when %s by %.*s"),
				qec_trimwhite(sizeof(DB_OWN_NAME),
			        	(char*)&art[i].qef_ownname), 
			        (char*)&art[i].qef_ownname, 
	      		    	qec_trimwhite(sizeof(art[i].qef_tabname.db_tab_name),
	      		    		(char*)&art[i].qef_tabname.db_tab_name),
	      		    	&art[i].qef_tabname.db_tab_name,
				art[i].qef_mode==SXF_A_UPDATE?"update":
				art[i].qef_mode==SXF_A_INSERT?"insert":
				art[i].qef_mode==SXF_A_DELETE?"delete":
				art[i].qef_mode==SXF_A_SELECT?"select":
							      "access",
				qec_trimwhite(sizeof(DB_OWN_NAME),
			        	(char*)&qef_cb->qef_user), 
					(char*)&qef_cb->qef_user
				);
			   
			    qef_rcb->qef_ev_l_text=STlength(evtxt);
			    qef_rcb->qef_evtext=evtxt;
			}
			else
			{
			    qef_rcb->qef_ev_l_text=apr[j].qef_ev_l_text;
			    qef_rcb->qef_evtext=apr[j].qef_evtext;
			}
			qef_rcb->qef_evname= &apr[j].qef_evname;
			qef_rcb->qef_evowner= &apr[j].qef_evowner;
			local_status=qeu_evraise(qef_cb,qef_rcb);
			if(local_status!=E_DB_OK)
			{
				*status=E_DB_SEVERE;
			        dsh->dsh_error.err_code = E_QE028D_ALARM_DBEVENT_ERR;
				break;
			}
		    }
		    break;
		}
	    }
	}
    } /* end for all aud structures. */
    return (*status);
}


/*{
** Name: qeq_load_tproc_dsh	- Load table procedure QP and DSH
**
** Description:
**	This routine is part of table procedure validation.  The
**	tproc named by the given (calling-)QP resource is located
**	in QSF, and a DSH is created or located for that query plan.
**
**	Either way, the QEF RCB is updated with the QSF name and
**	alias of the table proc.  This way, if the QP isn't there,
**	or if something goes wrong with validation later on, the
**	sequencer will know what table proc needs to be recompiled.
**
** Inputs:
**	qef_rcb		- Pointer to qef request block
**	qpresource	- (Calling-)QP resource for table procedure
**	dsh		- DSH for (calling-)QP
**	tproc_dsh	- an output
**
** Outputs:
**	tproc_dsh	- Filled in with DSH for tproc QP if found
**	The DB cursor ID of the DSH side of the QP resource is filled in
**	with the QSF tproc object name, if it's found.
**
** Returns:
**	E_DB_OK, or E_DB_ERROR if QP not there or can't make a DSH.
**
**	Note that if error, there's no need to worry about the specific
**	error code in dsh_error, caller will change it anyway to "load
**	tproc" for the sequencer return-back.
**
** Exceptions:
**	None.
**
** History
**	12-May-2009 (gefei01)
**	    Creation.
**	18-Jun-2010 (kschendel) b123775
**	    Rework so that the DSH is created as well as the QP located.
*/

static DB_STATUS
qeq_load_tproc_dsh(
	QEF_RCB		*qef_rcb,
        QEF_RESOURCE	*qpresource,
        QEE_DSH		*dsh,
	QEE_DSH		**tproc_dsh)
{
    DB_STATUS	status = E_DB_OK;
    QEE_RESOURCE *res;		/* DSH resource for table proc */
    QSF_RCB	qsf;		/* QSF request block */
    QSO_NAME	dbpalias;	/* Alias name of the table proc */

    /* Look for the table procedure QP object in QSF */
    qsf.qsf_type = QSFRB_CB;
    qsf.qsf_ascii_id = QSFRB_ASCII_ID;
    qsf.qsf_length = sizeof(QSF_RCB);
    qsf.qsf_owner = (PTR)DB_QEF_ID;
    qsf.qsf_sid = qef_rcb->qef_cb->qef_ses_id;
    qsf.qsf_feobj_id.qso_type = QSO_ALIAS_OBJ;
    qsf.qsf_feobj_id.qso_lname = sizeof(qsf.qsf_feobj_id.qso_name);
    dbpalias = qpresource->qr_resource.qr_proc.qr_dbpalias;
    /* Clean out the cursor-ID bits that we don't know and are zero */
    dbpalias.qso_n_id.db_cursor_id[0] = 0;
    dbpalias.qso_n_id.db_cursor_id[1] = 0;
    MEcopy((PTR) &dbpalias, sizeof(QSO_NAME), (PTR)qsf.qsf_feobj_id.qso_name);
    qsf.qsf_lk_state = QSO_SHLOCK;	/* Ask for shared lock on QP */

    /* Before making the QSF call, fill in the tproc name info into the
    ** QEF RCB as well ... just in case things don't work out, here or
    ** during validation.
    */
    qef_rcb->qef_dbpId.db_tab_base = 0;	/* not to confuse with set-input */
    qef_rcb->qef_dbpId.db_tab_index = 0;
    qef_rcb->qef_qp = dbpalias.qso_n_id;
    qef_rcb->qef_qso_handle = NULL;	/* See below */
    qef_rcb->qef_dbpname = qpresource->qr_resource.qr_proc.qr_dbpalias;

    /* Now try for the QP in QSF */
    status = qsf_call(QSO_JUST_TRANS, &qsf);
    if (status != E_DB_OK || qsf.qsf_t_or_d != QSO_WASTRANSLATED)
	return (E_DB_ERROR);

    /* OK so far, remember the translated name in the DSH resource */
    res = &dsh->dsh_resources[qpresource->qr_id_resource];
    MEcopy((PTR) qsf.qsf_obj_id.qso_name, sizeof(DB_CURSOR_ID),
	(PTR) &res->qer_resource.qer_proc.qer_proc_cursid);
    qef_rcb->qef_qso_handle = qsf.qsf_obj_id.qso_handle;

    /* Get a DSH for this table proc QP.  Specifying a tproc in the call
    ** tells qeq-dsh to not look for an existing DSH active for the query,
    ** but instead got a new one.  This call basically can't fail unless
    ** there's no memory for a DSH.
    */
    qef_rcb->qef_qp = res->qer_resource.qer_proc.qer_proc_cursid;
    status = qeq_dsh(qef_rcb, 0, tproc_dsh, QEQDSH_TPROC, -1);
    /* qeq-dsh shlocks the QP just like we did above, release one of
    ** the locks so we don't over-count.  Wait till now to do it so
    ** that the QP is pinned between the trans and the DSH creation.
    */
    (void) qsf_call(QSO_UNLOCK, &qsf);

    return (status);
} /* qeq_load_tproc_dsh */

