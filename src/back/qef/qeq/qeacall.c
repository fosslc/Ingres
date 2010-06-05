/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <me.h>
#include    <pc.h>
#include    <st.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <ulf.h>
#include    <ulm.h>
#include    <adf.h>
#include    <ade.h>
#include    <cs.h>
#include    <lk.h>
#include    <tm.h>
#include    <scf.h>
#include    <sxf.h>
#include    <dmf.h>
#include    <dmacb.h>
#include    <dmtcb.h>
#include    <dmrcb.h>
#include    <qefmain.h>
#include    <qsf.h>	
#include    <qefrcb.h>
#include    <qefcb.h>
#include    <qefnode.h>
#include    <psfparse.h>
#include    <qefact.h>
#include    <qefdsh.h>
#include    <qefqp.h>
#include    <qefscb.h>

#include    <dmmcb.h>
#include    <dmucb.h>
#include    <ex.h>
#include    <tm.h>

#include    <dudbms.h>
#include    <qefqeu.h>
#include    <qeuqcb.h>
#include    <rqf.h>
#include    <tpf.h>
#include    <qefkon.h>
#include    <qefqry.h>
#include    <qefcat.h>
#include    <qefprotos.h>

static DB_STATUS openTempTable(
	QEF_RCB 	*rcb,
	QEE_DSH		*dsh,
	i4		tempTableNumber,
	bool		gttparm);

/**
**
**  Name:   QEACALL.C		- Procedure call support
**
**  Description:
**	Routines supporting procedure calls from within a QP.
**
**	    qea_callproc	- call the named procedure
**	    qea_prc_insert      - Do insert for set input procedure.
**	    qea_closetemp       - Close a temp table used for set input
**					procedure.
**
**  History:
**	20-apr-89 (paul)
**	    Moved from qeq.c.
**	09-may-89 (neil)
**	    Added rule name tracing.
**	26-may-89 (paul)
**	    Increase QEF statement #, cleanup DSH on error recovery.
**	31-may-89 (neil)
**	    Cleanup tracing, modify when context count is set.
**	23-jun-89 (neil)
**	    Extended trace for nested procedures.
**	26-sep-89 (neil)
**	    Support for SET NORULES.
**	02-jan-90 (neil)
**	    DSH initialization error handling improved.
**	03-jan-90 (ralph)
**	    Change interface to QSO_JUST_TRANS
**	10-jan-90 (neil)
**	    Improved DSH cleanup and DSH initialization errors.
**	09-feb-90 (neil)
**	    Auditing cleanup.
**	09-nov-92 (jhahn)
**	    Added handling for byrefs.
**	09-dec-1992 (pholman)
**	    C2: replace obsolete qeu_audit with qeu_secaudit.
**	14-dec-92 (jhahn)
**	    Cleaned up handling of byrefs.
**      14-dec-92 (anitap)
**          Included <psfparse.h> and changed order of <qsf.h> for CREATE SCHEMA
**	12-feb-93 (jhahn)
**	    Added support for statement level rules. (FIPS)
**	15-mar-93 (anitap)
**	    Prototyped qea_proc_insert() and qea_closetemp().
**	24-mar-93 (jhahn)
**	    Various fixes for support of statement level rules. (FIPS)
**	02-apr-93 (jhahn)
**	    Made set input procedures called from rules bump qef_rule_depth.
**	01-may-93 (jhahn)
**	    Undid above change. Instead added new action QEA_INVOKE_RULE.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	19-jul-93 (anitap)
**	    Renamed QEF_SAVED_RSRC to QEF_DBPROC_QP.
**	01-sep-93 (jhahn)
**	    Added support for multiple query plans for set input procedures.
**	08-oct-93 (swm)
**	    Bug #56448
**	    Altered qec_tprintf calls to conform to new portable interface.
**	    Each call is replaced with a call to STprintf to deal with the
**	    variable number and size of arguments issue, then the resulting
**	    formatted buffer is passed to qec_tprintf. This works because
**	    STprintf is a varargs function in the CL. Unfortunately,
**	    qec_tprintf cannot become a varargs function as this practice is
**	    outlawed in main line code.
**	18-mar-94 (swm)
**	    Bug #59883
**	    Remove format buffers declared as automatic data on the stack with
**	    references to per session buffer allocated from ulm memory at
**	    QEF session startup time.
**	18-may-94 (anitap)
**	    Bug #63465
**	    If error in nested procedure, the whole transaction was being
**	    rolled back. 
**	17-oct-96 (nick)
**	    Remove compiler warning.
**	7-nov-95 (inkdo01)
**	    Changes to replace QEN_ADF structure instances by pointers in
**	    QEF_AHD structures.
**	29-oct-1998 (somsa01)
**	    In openTempTable(), if we are opening a Global Temporary Table,
**	    turn on the DMT_SESSION_TEMP flag in dmt_flags_mask before the
**	    open.  (Bug #94059)
**	12-nov-1998 (somsa01)
**	    Added another parameter to openTempTable(), and refined the
**	    check for a Global Session Temporary Table in openTempTable().
**	    (Bug #94059)
**	17-mar-1999 (somsa01)
**	    Added include of pc.h and lk.h to resolve references to locking
**	    ids.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	10-Jan-2001 (jenjo02)
**	    Supply session's SID to QSF in qsf_sid.
**	29-dec-03 (inkdo01)
**	    DSH is now parameter, "function" replaces "reset".
**	23-feb-04 (inkdo01)
**	    Changed qef_rcb->error to dsh->dsh_error for || thread safety
**	    (and others).
**	13-Jul-2004 (schka24)
**	    Make sure error code ends up in callproc's dsh; it was getting
**	    lost in the qef-rcb in some cases.
**	28-Dec-2005 (kschendel)
**	    Use DSH references whenever possible instead of QEF RCB
**	    references.  (This simplifies the code, eliminating many
**	    RCB references, and reflects the new primacy of the DSH
**	    thanks to the parallel query changes.)
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**/

/*{
** Name: QEA_CALLPROC		- call the named procedure
**
** Description:
**	The current execution environment is saved and an
**	environment is created in which to execute the
**	named procedure.
**
**	This procedure is only called when a nested procedure is invoked.
**	This can occur the first time through, or if the procedure is not
**	found (LOAD_QP) or the plan was deemed invalid (INVALID_QUERY) then
**	the procedure is re-entered in this routine.
**
**	If rules are turned off (QEF_T_NORULES) then this procedure returns
**	immediately.
**
** Inputs:
**	action			Callproc action header
**      qef_rcb
**	call_dsh		DSH doing the callproc
**	function		unused
**	state			unused
**
** Outputs:
**      call_dsh
**	    .error.err_code	one of the following
**				E_QE0119_LOAD_QP - Load a procedure QP
**				E_QE0125_RULES_INHIBIT - Rules are turned off.
**				E_QE0000_OK
**	Returns:
**	    E_DB_{OK,WARN,ERROR,FATAL}
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	20-apr-89 (paul)
**	    Moved from qeq.c
**	09-may-89 (neil)
**	    Added rule name tracing.
**	26-may-89 (paul)
**	    Increase statement #, cleanup DSH on error recovery.
**	31-may-89 (neil)
**	    Cleanup tracing; modify when context count is set; reset "saved
**	    resource" bit if procedure is loaded.
**	23-jun-89 (neil)
**	    Extended trace for nested procedures: Indicate procedure nesting
**	    level and rule depth.
**	26-sep-89 (neil)
**	    Support for SET NORULES.
**	02-jan-90 (neil)
**	    DSH initialization error handling improved to indicate problem
**	    specifics.
**	03-jan-90 (ralph)
**	    Change interface to QSO_JUST_TRANS
**	10-jan-90 (neil)
**	    Improved DSH cleanup on DSH initialization errors.  Made sure
**	    to confirm allocation error, and to pass a FALSE "release" flag
**	    to qeq_cleanup.
**	09-feb-90 (neil)
**	    Auditing cleanup: only audit the firing of rules and only if
**	    auditing is on (for performance).
**	09-nov-92 (jhahn)
**	    Added handling for byrefs.
**	14-dec-92 (jhahn)
**	    Cleaned up handling of byrefs.
**	12-feb-93 (jhahn)
**	    Added support for statement level rules. (FIPS)
**	24-mar-93 (jhahn)
**	    Various fixes for support of statement level rules. (FIPS)
**	02-apr-93 (jhahn)
**	    Made set input procedures called from rules bump qef_rule_depth.
**	01-may-93 (jhahn)
**	    Undid above change. Instead added new action QEA_INVOKE_RULE.
**	06-jul-93 (robf)
**	    Pass security  label to qeu_secaudit
**	01-sep-93 (jhahn)
**	    Added support for multiple query plans for set input procedures.
**	 7-jan-94 (swm)
**	    Bug #58635
**	    Added PTR cast for qsf_owner which has changed type to PTR.
**	18-may-94 (anitap)
**	    Bug #63465
**	    If error in nested procedure, the whole transaction was being
**	    rolled back. 
**	7-nov-95 (inkdo01)
**	    Changes to replace QEN_ADF structure instances by pointers in
**	    QEF_AHD structures.
**	24-jul-96 (inkdo01)
**	    Added support of global temp table proc parms.
**	4-mar-97 (inkdo01)
**	    Added display of error QE030B (row rule calls SET OF proc).
**	23-may-97 (inkdo01)
**	    Change QE030B to only pass procname, since it is mapped to a 
**	    US error in qeferror, anyway.
**	17-aug-99 (thaju02)
**	    initialize dmt_show.db_tab_id.db_tab_base to 0 prior to calling
**	    dmt_show, to avoid falsely reporting E_QE0018. (b98431)
**	27-apr-01 (inkdo01)
**	    Add code to detect nested/dynamic calls to row procs.
**	15-mar-04 (inkdo01)
**	    dsh_tempTables is now an array of ptrs.
**	17-mar-04 (inkdo01)
**	    We be fixin' a bug in error handlin' when qeq_dsh() doesn't
**	    return a dsh.
**	13-may-04 (inkdo01)
**	    Preserve status across qeq_cleanup calls.
**	18-may-04 (inkdo01)
**	    Quick fix to eliminate QE0018 when QE030D (US09AF) happens.
**	13-Jul-2004 (schka24)
**	    Straighten out which dsh is used where, fix loss of error
**	    code when finding called qp leading to QE0018.
**	13-Dec-2005 (kschendel)
**	    Inlined QEN_ADF changed to pointer, fix here.
**      29-May-2008 (gefei01)
**          Prototype change for qeq_dsh().
**      22-Apr-2009 (hanal04) Bug 121970
**          In printrule tracing print the ahd_pcount not qef_pcount
**          which is set to zero if we are in a sub-procedure.
*/
DB_STATUS
qea_callproc(
QEF_AHD		    *act,
QEF_RCB		    *qef_rcb,
QEE_DSH		    *call_dsh,
i4		    function,		/* Unused */
i4		    state )		/* Unused */
{
    i4		err;
    DB_STATUS		status	= E_DB_OK, savestat;
    QEF_CB		*qef_cb = call_dsh->dsh_qefcb;
    PTR			*cbs = call_dsh->dsh_cbs;
    QEE_DSH		*proc_dsh;	/* DSH for called dbp */
    QEN_ADF		*qen_adf;
    ADE_EXCB		*ade_excb;
    QSF_RCB		qsf_rcb;
    i4		tr1 = 0, tr2 = 0;	/* Dummy trace values */
    DB_ERROR		e_error;		/* To pass to qeu_secaudit */
    i4             page_count;
    bool                is_deferred =
        		  act->qhd_obj.qhd_callproc.ahd_proc_temptable != NULL;
    bool		gttparm = 
			  act->qhd_obj.qhd_callproc.ahd_gttid.db_tab_base != 0;
    bool		need_cleanup = FALSE;
    bool		need_release;
    bool		from_rule =
	                   act->qhd_obj.qhd_callproc.ahd_rulename.db_name[0]
			       != EOS;
    QEE_TEMP_TABLE  *proc_temptable;
    char		*cbuf = qef_cb->qef_trfmt;
    i4			cbufsize = qef_cb->qef_trsize;
    i4			open_count;

    do
    {
	/*
	** This action is called back if a request to CREATE a QP for a
	** CALLPROC fails. In this case we just continue with error processing.
	** No need for another error as CLEAN_RSRC is only set if a client on
	** the outside issued an error knowing we have a stacked environment.
	*/
	if ((qef_rcb->qef_intstate & QEF_CLEAN_RSRC) != 0)
	{
	    call_dsh->dsh_error.err_code = E_QE0025_USER_ERROR;
	    qef_rcb->qef_intstate &= ~QEF_CLEAN_RSRC;
	    status = E_DB_ERROR;
	    break;
	}

	if (is_deferred)
	{
	    proc_temptable = call_dsh->dsh_tempTables[act->qhd_obj.qhd_callproc
				    .ahd_proc_temptable->ttb_tempTableIndex];
	    if (proc_temptable->tt_statusFlags & TT_EMPTY)
	     if (from_rule) break;	/* empty ttab in statement rule -
					** nothing to do at all */
	     else status = openTempTable(qef_rcb, call_dsh, act->qhd_obj.qhd_callproc
		   	.ahd_proc_temptable->ttb_tempTableIndex, gttparm);
	    else status = qen_rewindTempTable(call_dsh, act->qhd_obj.
			qhd_callproc.ahd_proc_temptable->ttb_tempTableIndex);
	    if (status != E_DB_OK)
		   break;
	}
	/* If called from a rule & SET NORULES is on, then inhibit execution */
	if (    from_rule
	     && ult_check_macro(&qef_cb->qef_trace, QEF_T_NORULES, &tr1, &tr2)
	   )
	{
	    /* Trace inhibited rule if required */
	    if (ult_check_macro(&qef_cb->qef_trace, QEF_T_RULES, &tr1, &tr2))
	    {
		char	*rn = act->qhd_obj.qhd_callproc.ahd_rulename.db_name;
		STprintf(cbuf, "PRINTRULES: Rule '%.*s' suppressed\n",
			    qec_trimwhite(DB_RULE_MAXNAME, rn), rn);
        	qec_tprintf(qef_rcb, cbufsize, cbuf);
	    }
	    call_dsh->dsh_error.err_code = E_QE0125_RULES_INHIBIT;
	    status = E_DB_ERROR;
	    break;
	} /* If rules off */

	/*
	** Security audit rule firing  - check that is first time (not
	** recreation), is a rule and, for performance, that we really
	** need to audit.
	*/
	if (   (qef_rcb->qef_intstate & QEF_DBPROC_QP) == 0
	    && (act->qhd_obj.qhd_callproc.ahd_rulename.db_name[0] != EOS)
	    && (Qef_s_cb->qef_state & QEF_S_C2SECURE)
	   )
	{
	    status = qeu_secaudit(FALSE, qef_cb->qef_ses_id,
		    act->qhd_obj.qhd_callproc.ahd_rulename.db_name,
		    (DB_OWN_NAME *)&act->qhd_obj.qhd_callproc.ahd_ruleowner,
		    sizeof(act->qhd_obj.qhd_callproc.ahd_rulename), SXF_E_RULE,
		    I_SX202F_RULE_ACCESS, SXF_A_SUCCESS | SXF_A_EXECUTE,
		    &e_error);

	    if (status != E_DB_OK)
	    {
		call_dsh->dsh_error.err_code = e_error.err_code;
		break;
	    }
	}

	/* Actually execute a CALLPROC. */
	/* We generate an actual parameter list, save the current   */
	/* execution context, stack the DSH and setup the new	    */
	/* execution context. Processing then continues with the    */
	/* new QP. */

	/* Save the current execution context as represented by	    */
	/* QEF_RCB and the current DSH. */
	STRUCT_ASSIGN_MACRO(*qef_rcb, *call_dsh->dsh_saved_rcb);
	call_dsh->dsh_act_ptr = act;

	qen_adf = act->qhd_obj.qhd_callproc.ahd_procparams;
	if (qen_adf != NULL)
	{
	    /* Compute the actual parameters for this procedure	    */
	    /* call. */
	    qef_rcb->qef_pcount = act->qhd_obj.qhd_callproc.ahd_pcount;
	    ade_excb = (ADE_EXCB *)cbs[qen_adf->qen_pos];
	    ade_excb->excb_seg = ADE_SMAIN;
	    status = ade_execute_cx(call_dsh->dsh_adf_cb, ade_excb);
	    if (status != E_DB_OK)
	    {
		status = qef_adf_error(&call_dsh->dsh_adf_cb->adf_errcb,
			status, qef_cb, &call_dsh->dsh_error);
		if (status != E_DB_OK)
		    break;
	    }
	}
	else
	{
	    /* No actual parameters */
	    qef_rcb->qef_pcount = 0;
	}

	/*
	** If tracing rules and first time through then display
	** rule/procedure information.
	*/

	if ((qef_rcb->qef_intstate & QEF_DBPROC_QP) == 0)
	{
	    if (ult_check_macro(&qef_cb->qef_trace, QEF_T_RULES, &tr1, &tr2))
	    {
		char	*rn, *pn;	/* Rule/procedure names */

		rn = act->qhd_obj.qhd_callproc.ahd_rulename.db_name;
		pn = act->qhd_obj.qhd_callproc.ahd_dbpalias.als_crsr_id.db_cur_name;
		/* Tailor trace for rules vs nested procedures */
		STprintf(cbuf, *rn == EOS ?
		     "PRINTRULES 1: Executing procedure '%.*s'\n" :
		     "PRINTRULES 1: Executing procedure '%.*s' from rule '%.*s'\n",
		     qec_trimwhite(DB_DBP_MAXNAME, pn), pn,
		     qec_trimwhite(DB_RULE_MAXNAME, rn), rn);
        	qec_tprintf(qef_rcb, cbufsize, cbuf);
		STprintf(cbuf,
		     "PRINTRULES 2: Rule/procedure depth = %d/%d, parameters passed = %d\n",
		     qef_rcb->qef_rule_depth,
		     qef_rcb->qef_context_cnt + 1, 
		     act->qhd_obj.qhd_callproc.ahd_pcount);
        	qec_tprintf(qef_rcb, cbufsize, cbuf);
	    } /* If tracing rules */
	}

	/*
	** Indicate that we have nested one more level, generate an error if
	** we are nested too deeply. Context count is actually set later.
	*/
	if (qef_cb->qef_max_stack < qef_rcb->qef_context_cnt + 1)
	{
	    status = E_DB_ERROR;
	    qef_error(E_QE0208_EXCEED_MAX_CALL_DEPTH, 0L, status, &err,
		&call_dsh->dsh_error, 1, sizeof(qef_cb->qef_max_stack),
		&qef_cb->qef_max_stack);
	    call_dsh->dsh_error.err_code = E_QE0122_ALREADY_REPORTED;
	    break;
	}

	if (gttparm) 
	{
	    STRUCT_ASSIGN_MACRO(act->qhd_obj.qhd_callproc.ahd_gttid,
		qef_rcb->qef_setInputId);	/* copy temptab ID */
	    page_count = 1;
	    is_deferred = TRUE;
	}
	else if (is_deferred)
        {
            DMT_CB      *dmt_cb = proc_temptable->tt_dmtcb;
            DMT_SHW_CB  dmt_show;
            DMT_TBL_ENTRY       dmt_tbl_entry;

            dmt_show.type = DMT_SH_CB;
            dmt_show.length = sizeof(DMT_SHW_CB);
            dmt_show.dmt_session_id = qef_cb->qef_ses_id;
            dmt_show.dmt_db_id = qef_rcb->qef_db_id;
            dmt_show.dmt_tab_id.db_tab_base = 0;
            dmt_show.dmt_flags_mask = DMT_M_TABLE | DMT_M_ACCESS_ID;
            dmt_show.dmt_char_array.data_address = NULL;
            dmt_show.dmt_table.data_address = (PTR) &dmt_tbl_entry;
            dmt_show.dmt_table.data_in_size = sizeof(DMT_TBL_ENTRY);
            dmt_show.dmt_record_access_id = dmt_cb->dmt_record_access_id;
            status = dmf_call(DMT_SHOW, &dmt_show);
            if (status != E_DB_OK)
	    {
		STRUCT_ASSIGN_MACRO(dmt_show.error, call_dsh->dsh_error);
                break;
	    }
            page_count = dmt_tbl_entry.tbl_page_count;
            STRUCT_ASSIGN_MACRO(dmt_cb->dmt_id,
                               qef_rcb->qef_setInputId);
        }
        else
        {
            page_count = -1;
            MEfill(sizeof(DB_TAB_ID), (u_char)0,
                   (PTR)&qef_rcb->qef_setInputId);
        }
        STRUCT_ASSIGN_MACRO(act->qhd_obj.qhd_callproc.ahd_procedureID,
                           qef_rcb->qef_dbpId);

	/* Get the id of the procedure to call */
	STRUCT_ASSIGN_MACRO(act->qhd_obj.qhd_callproc.ahd_dbpalias.als_crsr_id, qef_rcb->qef_qp);
	qef_rcb->qef_qso_handle = 0;
	/* Set the full name of the procedure into the RCB in case  */
	/* we have to call PSF to define the procedure. */
	MEcopy((PTR)&act->qhd_obj.qhd_callproc.ahd_dbpalias,
	       sizeof(act->qhd_obj.qhd_callproc.ahd_dbpalias),
	       (PTR)&qef_rcb->qef_dbpname);

	/* Lookup this procedure name as a QSF alias. At this time  */
	/* we do not have a valid DB_CURSOR_ID because we do not    */
	/* have the timestamp for the QP object. Ask QSF to look    */
	/* this up for us. If found, we continue executing, if not  */
	/* return to SCF to define this procedure.		    */
	qsf_rcb.qsf_type = QSFRB_CB;
	qsf_rcb.qsf_ascii_id = QSFRB_ASCII_ID;
	qsf_rcb.qsf_length = sizeof(QSF_RCB);
	qsf_rcb.qsf_owner = (PTR)DB_QEF_ID;
	qsf_rcb.qsf_sid = qef_rcb->qef_cb->qef_ses_id;

	qsf_rcb.qsf_feobj_id.qso_type = QSO_ALIAS_OBJ;
	qsf_rcb.qsf_feobj_id.qso_lname =
	    sizeof(qsf_rcb.qsf_feobj_id.qso_name);
	MEcopy((PTR)&act->qhd_obj.qhd_callproc.ahd_dbpalias,
	       sizeof(qsf_rcb.qsf_feobj_id.qso_name),
	       (PTR)qsf_rcb.qsf_feobj_id.qso_name);
        qsf_rcb.qsf_lk_state = QSO_FREE;
	status = qsf_call(QSO_JUST_TRANS, &qsf_rcb);
	if (DB_FAILURE_MACRO(status))
	{
	    /* No such procedure in QSF, ask SCF to define it */
	    /* Tell SCF to call us back even if the porcedure	    */
	    /* cannot be loaded. We will need to clean up */
	    qef_rcb->qef_intstate |= QEF_DBPROC_QP;
	    call_dsh->dsh_error.err_code = E_QE0119_LOAD_QP;
	    break;
	}
	else
	{
	    /*
	    ** The procedure was found - make sure "saved" bit isn't on for
	    ** the scope of this execution.  It may get turned back on if
	    ** the QP or DSH is found to be invalid but then we'll re-enter
	    ** here (to try again) and turn it off.
	    */
	    qef_rcb->qef_intstate &= ~QEF_DBPROC_QP;
	}

	/* Increase context count now that we've loaded the QP */
	qef_rcb->qef_context_cnt++;

	/* Procedure in QSF, load the timestamp into the QEF_RCB    */
	/* to allow normal QEF processing to continue.		    */
	MEcopy((PTR)qsf_rcb.qsf_obj_id.qso_name, sizeof(DB_CURSOR_ID),
	       (PTR)&qef_rcb->qef_qp);
	status = qeq_dsh(qef_rcb, 0 , &proc_dsh, TRUE, page_count,
                         (bool)FALSE);
	if (DB_FAILURE_MACRO(status))
	{
	    char	*rn, *pn;	/* Rule/procedure names */

	    STRUCT_ASSIGN_MACRO(qef_rcb->error, call_dsh->dsh_error);

	    if (call_dsh->dsh_error.err_code == E_QE0023_INVALID_QUERY)
	    {
		/* No such procedure in QSF, ask SCF to define it */
		/* Tell SCF to call us back even if the porcedure */
		/* cannot be loaded. We will need to clean up     */
		qef_cb->qef_dsh = (PTR) call_dsh;
		qef_rcb->qef_qp = call_dsh->dsh_saved_rcb->qef_qp;
		qef_rcb->qef_intstate |= QEF_DBPROC_QP;
		call_dsh->dsh_error.err_code = E_QE0119_LOAD_QP;
		qef_rcb->qef_context_cnt--;
		break;
	    }
	    /*
	    ** The QP DSH is invalid for some reason. Generate error and return.
	    ** If any allocation error then change to useful error message. 
	    */
	    if (   call_dsh->dsh_error.err_code == E_UL0005_NOMEM 
	        || call_dsh->dsh_error.err_code == E_QS0001_NOMEM 
	        || call_dsh->dsh_error.err_code == E_QE001E_NO_MEM 
		|| call_dsh->dsh_error.err_code == E_QE000D_NO_MEMORY_LEFT
		|| call_dsh->dsh_error.err_code == E_QE030B_RULE_PROC_MISMATCH)
	    {
		pn =
		 act->qhd_obj.qhd_callproc.ahd_dbpalias.als_crsr_id.db_cur_name;
		rn = act->qhd_obj.qhd_callproc.ahd_rulename.db_name;
		if (*rn == EOS)
		    rn = "NULL                                   ";
		if (call_dsh->dsh_error.err_code == E_QE030B_RULE_PROC_MISMATCH)
		    qef_error(E_QE030B_RULE_PROC_MISMATCH, 0L, status, &err,
		    &call_dsh->dsh_error, 1,
		    /* qec_trimwhite(DB_RULE_MAXNAME, rn), rn, */
		    qec_trimwhite(DB_DBP_MAXNAME, pn), pn);
		else qef_error(E_QE0199_CALL_ALLOC, 0L, status, &err,
		    &call_dsh->dsh_error, 3,
		    qec_trimwhite(DB_DBP_MAXNAME, pn), pn,
		    qec_trimwhite(DB_RULE_MAXNAME, rn), rn,
		    sizeof(qef_rcb->qef_context_cnt),&qef_rcb->qef_context_cnt);
		call_dsh->dsh_error.err_code = E_QE0122_ALREADY_REPORTED;
	    }

	    /*
	    ** Now clean up and restore to state before routine entry.
	    ** Pass in FALSE for release as if the DSH is NULL we do NOT want
	    ** to cause cleanup to release all DSH's for this session
	    ** (qee_cleanup).  If the DSH is not NULL it will be cleaned up at
	    ** the end of the query.
	    */
	    need_cleanup = TRUE;
	    need_release = FALSE;
	    break;
	}

	if (proc_dsh->dsh_qp_ptr->qp_status & QEQP_ROWPROC)
	{
	    char	*pn;
	    /* Row producing procs cannot be invoked by QEA_CALLPROC (which
	    ** implies either nested proc call or dynamic SQL proc call). */
	    pn = act->qhd_obj.qhd_callproc.ahd_dbpalias.als_crsr_id.db_cur_name;
	    qef_error(E_QE030D_NESTED_ROWPROCS, 0L, E_DB_ERROR, &err,
		&call_dsh->dsh_error, 1,
		qec_trimwhite(DB_DBP_MAXNAME, pn), pn);
	    status = E_DB_ERROR;
	    break;
	}
	/* Found QP and DSH, stack old context */
	proc_dsh->dsh_stack = call_dsh;

	proc_dsh->dsh_stmt_no = qef_cb->qef_stmt++;

	qef_cb->qef_dsh = (PTR) proc_dsh;

	qef_cb->qef_open_count++;

	/* Initialize procedure parameters (& user params - even if wrong) */
	if (proc_dsh->dsh_qp_ptr->qp_ndbp_params != 0 || qef_rcb->qef_pcount > 0)
	{
	    status = qee_dbparam(qef_rcb,
				 proc_dsh,
				 call_dsh,
				 act->qhd_obj.qhd_callproc.ahd_params,
				 act->qhd_obj.qhd_callproc.ahd_pcount, TRUE);
	    if (DB_FAILURE_MACRO(status))
	    {
		/* If we fail after acquiring the DSH, we need to */
		/* deallocate the DSH and recover back to the original	    */
		/* calling state. */
		STRUCT_ASSIGN_MACRO(proc_dsh->dsh_error, call_dsh->dsh_error);
		qef_cb->qef_open_count--;
		need_cleanup = TRUE;
		need_release = TRUE;
		break;
	    }
	}
	if (is_deferred)
	{
	    /* FIXME should error if it's null */
	    if (proc_dsh->dsh_qp_ptr->qp_setInput != NULL)
		STRUCT_ASSIGN_MACRO(qef_rcb->qef_setInputId,
			* (DB_TAB_ID *)(proc_dsh->dsh_row[proc_dsh->dsh_qp_ptr->
					 qp_setInput->vl_tab_id_index]));
	}
    } while (FALSE);

    if (need_cleanup)
    {
	/* error in a nested DB procedure, abort up to the beginning of the
	** procedure if there are no other cursors opened. We
	** guarantee that by decrementing the qef_open_count. If the
	** count becomes zero, qeq_cleanup will abort to the last internal
	** savepoint. Fix for bug 63465.
	*/

	savestat = status;
	open_count = qef_cb->qef_open_count;
	
	while (qef_cb->qef_open_count > 0) 
	      qef_cb->qef_open_count--;
	
	status = qeq_cleanup(qef_rcb, status, need_release);

	status = savestat;
	qef_cb->qef_open_count = open_count;

	qef_cb->qef_dsh = (PTR) call_dsh;
	qef_rcb->qef_context_cnt--;
	qef_rcb->qef_pcount = call_dsh->dsh_saved_rcb->qef_pcount;
	qef_rcb->qef_usr_param = call_dsh->dsh_saved_rcb->qef_usr_param;
	qef_rcb->qef_qp = call_dsh->dsh_saved_rcb->qef_qp;
	qef_rcb->qef_qso_handle = call_dsh->dsh_saved_rcb->qef_qso_handle;
	call_dsh->dsh_qef_rowcount = call_dsh->dsh_saved_rcb->qef_rowcount;
	call_dsh->dsh_qef_targcount = call_dsh->dsh_saved_rcb->qef_targcount;
	call_dsh->dsh_qef_output = call_dsh->dsh_saved_rcb->qef_output;
	call_dsh->dsh_qef_count = call_dsh->dsh_saved_rcb->qef_count;
    }
    return (status);
}

/*{
** Name: openTempTable		- Open a temp table used for set input
**                                  procedure.
**
** Description:
**      The current execution environment is saved and an
**      environment is created in which to execute the
**      named procedure.
**
**      This procedure is only called when a nested procedure is invoked.
**      This can occur the first time through, or if the procedure is not
**      found (LOAD_QP) or the plan was deemed invalid (INVALID_QUERY) then
**      the procedure is re-entered in this routine.
**
**      If rules are turned off (QEF_T_NORULES) then this procedure returns
**      immediately.
**
** Inputs:
**      qef_rcb
**	tempTableNumber		index to dsh temptable array for temporary
**				table to be created.
**
** Outputs:
**      Returns:
**          E_DB_{OK,WARN,ERROR,FATAL} (from table creation)
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**	18-jul-96 (inkdo01)
**	    Created.
**	29-oct-1998 (somsa01)
**	    If we are opening a Global Temporary Table, turn on the
**	    DMT_SESSION_TEMP flag in dmt_flags_mask before the open.
**	    (Bug #94059)
**	12-nov-1998 (somsa01)
**	    Added another parameter, and refined the check for a Global
**	    Session Temporary Table.  (Bug #94059)
**	15-mar-04 (inkdo01)
**	    dsh_tempTables is now an array of ptrs.
**	13-Jul-2004 (schka24)
**	    Pass in dsh so we get the right one for sure.
**	30-Jul-2004 (jenjo02)
**	    Use dsh_dmt_id rather than qef_dmt_id transaction context.
**	4-Jun-2009 (kschendel) b122118
**	    Use open-and-link utility.
*/
static DB_STATUS openTempTable(
	QEF_RCB 	*rcb,
	QEE_DSH		*dsh,
	i4		tempTableNumber,
	bool		gttparm
)
{
    QEF_CB	    *qef_cb = rcb->qef_cb;
    QEE_TEMP_TABLE  *tempTable = dsh->dsh_tempTables[ tempTableNumber ];
    DMT_CB	    *dmt = tempTable->tt_dmtcb;
    DMR_CB	    *dmr = tempTable->tt_dmrcb;
    DM_MDATA	    dm_mdata;
    DB_STATUS status = E_DB_OK;

    for (;;)	/* something to break out of */
    {
	/* create the table if it doesn't already exist */

	if ( !( tempTable->tt_statusFlags & TT_CREATED ) )
	{
	    dmt->dmt_db_id = rcb->qef_db_id;
	    dmt->dmt_tran_id = dsh->dsh_dmt_id;
	    MEmove(8, (PTR) "$default", (char) ' ', 
	            sizeof(DB_LOC_NAME),
	            (PTR) dmt->dmt_location.data_address);
	    dmt->dmt_location.data_in_size = sizeof(DB_LOC_NAME);

	    dmt->dmt_flags_mask = DMT_DBMS_REQUEST;
	    if (gttparm)
		dmt->dmt_flags_mask |= DMT_SESSION_TEMP;
	    dmr->dmr_flags_mask = 0;
	    /*
	    ** We assume that the sort key descriptor was set up
	    ** earlier, probably at QEE time.
	    */

	    if ( tempTable->tt_statusFlags & TT_PLEASE_SORT )
		dmt->dmt_flags_mask |= DMT_LOAD;

	    if ( tempTable->tt_statusFlags & TT_NO_DUPLICATES )
		dmr->dmr_flags_mask |= DMR_NODUPLICATES;

	    dmr->dmr_count = 1;
	    dmr->dmr_s_estimated_records = TT_TUPLE_COUNT_GUESS;
	    dmr->dmr_tid = 0;
	    dm_mdata.data_address = dsh->dsh_row[ tempTable->tt_tuple ];
	    dm_mdata.data_size =
		dsh->dsh_qp_ptr->qp_row_len[ tempTable->tt_tuple ];
	    dmr->dmr_mdata = &dm_mdata;

	    status = dmf_call(DMT_CREATE_TEMP, dmt);
	    if (status != E_DB_OK)
	    {
	        if (dmt->error.err_code == E_DM0078_TABLE_EXISTS)
	        {
		    dsh->dsh_error.err_code = E_QE0050_TEMP_TABLE_EXISTS;
		    status = E_DB_ERROR;
	        }
	        else
	        {
		    dsh->dsh_error.err_code = dmt->error.err_code;
	        }
	        break;
	    }

	    /* Open the temp table */
	    dmt->dmt_flags_mask = 0;
	    if (gttparm)
		dmt->dmt_flags_mask |= DMT_SESSION_TEMP;
	    dmt->dmt_sequence = dsh->dsh_stmt_no;
	    dmt->dmt_access_mode = DMT_A_WRITE;
	    dmt->dmt_lock_mode = DMT_X;
	    dmt->dmt_update_mode = DMT_U_DIRECT;
	
	    status = qen_openAndLink(dmt, dsh);
	    if (status != E_DB_OK)
	    {
	        break;
	    }

            tempTable->tt_statusFlags |= TT_CREATED;
	    dmr->dmr_access_id = dmt->dmt_record_access_id;
	}	/* end of table creation */

	break;
    }	/* end of code block */

    return( status );
}
/*{
** Name: QEA_PRC_INSERT                - Do insert for set input procedure.
**
** Description:
**      The current execution environment is saved and an
**      environment is created in which to execute the
**      named procedure.
**
**      This procedure is only called when a nested procedure is invoked.
**      This can occur the first time through, or if the procedure is not
**      found (LOAD_QP) or the plan was deemed invalid (INVALID_QUERY) then
**      the procedure is re-entered in this routine.
**
**      If rules are turned off (QEF_T_NORULES) then this procedure returns
**      immediately.
**
** Inputs:
**      action                 Delete current row of cursor action.
**      dsh
**      reset
**      state                   DSH_CT_INITIAL if this is an action call
**                              DSH_CT_CONTINUE for call back after processing
**                              a rule action list.
**
** Outputs:
**      dsh
**          .error.err_code     one of the following
**                             E_QE0119_LOAD_QP - Load a procedure QP
**                             E_QE0125_RULES_INHIBIT - Rules are turned off.
**                             E_QE0000_OK
**      Returns:
**          E_DB_{OK,WARN,ERROR,FATAL}
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      17-nov-92 (jhahn)
**          Created.
**	15-mar-93 (anitap)
**	    Prototyped.
**	15-mar-94 (jhahn)
**	    Made external.
**	06-mar-96 (nanpr01)
**	    Parameter change in qen_putIntoTempTable for passing the temp
**	    table page size.
**	13-Dec-2005 (kschendel)
**	    Inlined QEN_ADF changed to pointer, fix here.
*/
DB_STATUS
qea_prc_insert(
        QEF_AHD             *act,
	QEE_DSH		    *dsh,
        i4		    function,           /* Unused */
        i4                  state)              /* Unused */
{
    DB_STATUS          status  = E_DB_OK;
    PTR                *cbs = dsh->dsh_cbs;
    QEN_ADF            *qen_adf;
    ADE_EXCB           *ade_excb;

    qen_adf = act->qhd_obj.qhd_proc_insert.ahd_procparams;
    if (qen_adf != NULL)
    {
        /* Compute the actual parameters for this row of procedure
        ** call.
        */
        ade_excb = (ADE_EXCB *)cbs[qen_adf->qen_pos];
        ade_excb->excb_seg = ADE_SMAIN;
        status = ade_execute_cx(dsh->dsh_adf_cb, ade_excb);
        if (status != E_DB_OK)
        {
            status = qef_adf_error(&dsh->dsh_adf_cb->adf_errcb,
			    status, dsh->dsh_qefcb, &dsh->dsh_error);
            if (status != E_DB_OK)
                return (status);
        }
        return(qen_putIntoTempTable(dsh, act->qhd_obj.qhd_proc_insert.
                                     ahd_proc_temptable));
    }
    return(E_DB_OK);
}

/*{
** Name: QEA_CLOSETEMP          - Close a temp table used for set input
**                                  procedure.
**
** Description:
**      The current execution environment is saved and an
**      environment is created in which to execute the
**      named procedure.
**
**      This procedure is only called when a nested procedure is invoked.
**      This can occur the first time through, or if the procedure is not
**      found (LOAD_QP) or the plan was deemed invalid (INVALID_QUERY) then
**      the procedure is re-entered in this routine.
**
**      If rules are turned off (QEF_T_NORULES) then this procedure returns
**      immediately.
**
** Inputs:
**      action                 Delete current row of cursor action.
**      dsh
**      reset
**      state                  DSH_CT_INITIAL if this is an action call
**                             DSH_CT_CONTINUE for call back after processing
**                             a rule action list.
**
** Outputs:
**      Returns:
**          E_DB_{OK,WARN,ERROR,FATAL}
**      Exceptions:
**          none
**
** Sid Effects:
**          none
**
** History:
**      17-nov-92 (jhahn)
**          Created.
**	15-mar-93 (anitap)
**	    Prototyped.
**	24-mar-93 (jhahn)
**	    Made external.
*/
DB_STATUS
qea_closetemp(
       QEF_AHD             *act,
       QEE_DSH		   *dsh,
       i4		   function,           /* Unused */
       i4                  state)              /* Unused */
{
    return (qen_destroyTempTable(dsh, act->qhd_obj.qhd_closetemp.
                                ahd_temptable_to_close->ttb_tempTableIndex));
}
