/*
**Copyright (c) 2004 Ingres Corporation
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
#include    <adp.h>
#include    <dmf.h>
#include    <dmrcb.h>
#include    <dmtcb.h>
#include    <qefmain.h>
#include    <qsf.h>
#include    <qefrcb.h>
#include    <qefcb.h>
#include    <qefnode.h>
#include    <psfparse.h>
#include    <qefact.h>
#include    <qefqp.h>
#include    <qefdsh.h>

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
#include    <sxf.h>
#include    <qefprotos.h>

/**
**
**  Name: QEADELETE.C - provides tuple delete support
**
**  Description:
**      The routines in this file deletes records to open
**  relations. 
**
**          qea_delete - delete one or more records to the open relation.
**          qea_pdelete - delete one or more records in place.
**
**
**  History:    $Log-for RCS$
**      10-jul-86 (daved)
**          written
**	02-feb-88 (puree)
**	    add reset flag to qea_delete and qea_pdelete function argument.
**	12-apr-88 (puree)
**	    modify qea_delete and qea_pdelete to handle both DM0044 and
**	    DM0047 since DMF returns DM0047 if GET and DELETE use the same
**	    record access id and DM0047 otherwise.
**	20-feb-89 (paul)
**	    Update to handle a nested action list for rule processing.
**	30-mar-89 (paul)
**	    Add resource limit checking.
**	22-sep-89 (paul)
**	    Added support for TIDs as parameters to procedures invoked
**	    by rules. This requires that the old TID be copied into a
**	    variable (in DSH_ROW) from which an ADE expression can access it.
**	08-dec-92 (anitap)
**	    Included <qsf.h> and <psfparse.h> for CREATE SCHEMA.
**	23-aug-93 (andys)
**	    If a fetch of a row in the outer side fails because of a deleted
**	    tid, then we continue on, as it is a tid we have just 
**	    deleted. [bug 43205] [was 24-aug-92 (andys)]
**	01-sep-93 (davebf)
**	    Fix bug 40262.  rowcount += i, targcount += j strategy returned
**	    wrong counts when there were actions internal to QEP
**      13-sep-93 (smc)
**          Added <cs.h> for CS_SID.
**       5-Nov-1993 (fred)
**          Added code to make the adu_free_los() call.
**       7-Apr-1994 (fred)
**          Deleted adu_free_objects code.  It is done in qeq_cleanup().
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	24-jan-01 (inkdo01)
**	    Changes to QEF_QEP structure putting some fields in union 
**	    (for hash aggregation project).
**	23-feb-04 (inkdo01)
**	    Changed qef_rcb->error to dsh->dsh_error for || thread safety
**	    (and others).
**	27-feb-04 (inkdo01)
**	    Changed dsh_qen_status from ptr to array of QEN_STATUSs to 
**	    ptr to arrays of ptrs.
**	28-Dec-2005 (kschendel)
**	    Use DSH references whenever possible instead of QEF RCB
**	    references.  (This simplifies the code, eliminating many
**	    RCB references, and reflects the new primacy of the DSH
**	    thanks to the parallel query changes.)
**	16-Jan-2006 (kschendel)
**	    Access qen-status thru xaddrs.
**	4-Jun-2009 (kschendel) b122118
**	    Minor dead code removal.
**/


/*
** {
** Name: QEA_DELETE	- delete rows from records
**
** Description:
**	Read a row from a QEP and perform the delete on it.
**	Delete is ALWAYS done by TID.
**
** Inputs:
**      action                          update action item
**				    E_QE0000_OK
**				    E_QE0017_BAD_CB
**				    E_QE0018_BAD_PARAM_IN_CB
**				    E_QE0019_NON_INTERNAL_FAIL
**				    E_QE0002_INTERNAL_ERROR
**				    E_QE0019_NON_INTERNAL_FAIL
**	state			    DSH_CT_INITIAL if this is an action call
**				    DSH_CT_CONTINUE if being called back after
**				    a rule action list is executed.
**
** Outputs:
**      qef_rcb
**	    .qef_rowcount		number of delete tuples
**	    .qef_targcount		number of attempted tuples
**	    .error.err_code		one of the following
**
**	Returns:
**	    E_DB_{OK, WARN, ERROR, FATAL}
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      10-jul-86 (daved)
**          written
**	02-feb-88 (puree)
**	    add reset flag to the function arguments.
**      12-apr-88 (puree)
**          handle both DM0044 and DM0047 the same way.
**	20-feb-89 (paul)
**	    Added support to invoke rule action lists.
**	30-mar-89 (paul)
**	    Add resource limit checking.
**	22-sep-89 (paul)
**	    Added support for using TIDs in rules.
**	28-sept-89 (eric)
**	    Add support to fix the sybase bug. This is done by returning
**	    a zerotup check flag when invalidating a query plan because
**	    it's QEN_NODE returns zero rows.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	23-aug-93 (andys)
**	    If a fetch of a row in the outer side fails because of a deleted
**	    tid, then we continue on, as it is a tid we have just 
**	    deleted. [bug 43205] [was 24-aug-92 (andys)]
**	01-sep-93 (davebf)
**	    Fix bug 40262.  rowcount += i, targcount += j strategy returned
**	    wrong counts when there were actions internal to QEP
**       5-Nov-1993 (fred)
**          Added adu_free_los() call to free large object temporaries.
**       7-Apr-1994 (fred)
**          Deleted adu_free_objects code.  It is done in qeq_cleanup().
**      29-oct-97 (inkdo01)
**          Added code to support AHD_NOROWS flag (indicating no possible
**          rows from underlying query).
**	21-nov-03 (inkdo01)
**	    Add open_flag parm to qen_func calls for parallel query support.
**	29-dec-03 (inkdo01)
**	    DSH is now parameter, "function" replaces "reset".
**	5-feb-04 (inkdo01)
**	    Add support for delete from partitioned tables.
**	28-feb-04 (inkdo01)
**	    Change qef_rcb parm to dsh in qeq_chk_resources call.
**	12-mar-04 (inkdo01)
**	    Fix to handle bigtids on byte-swapped machines.
**	12-may-04 (inkdo01)
**	    Call qeq_part_open unconditionally - it'll determine whether
**	    partition cbs are already prepared.
**	8-Jul-2004 (schka24)
**	    Still some tid byte-ordering confusion on x86, make tid handling
**	    more portable.
**	20-june-06 (dougi)
**	    Add support for BEFORE triggers (not that they're much use for
**	    DELETE).
**	09-sep-06 (toumi01) BUG 116609
**	    Compute tidinput address within loop; it moves.
**	1-Nov-2006 (kschendel)
**	    Fix up before trigger stuff when partitioned, and don't hurt
**	    the already set up dmr-tid after a before resume.
**	8-Sep-2008 (kibro01) b120693
**	    Remove unused action parameter from qeq_part_open
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: If E_DM0029_ROW_UPDATE_CONFLICT returned,
**	    invalidate the QP and retry.
[@history_template@]...
*/
DB_STATUS
qea_delete(
QEF_AHD		*action,
QEF_RCB		*qef_rcb,
QEE_DSH		*dsh,
i4		function,
i4		state )
{
    QEN_NODE	*node = action->qhd_obj.qhd_qep.ahd_qep;
    bool	reset = ((function & FUNC_RESET) != 0);
    QEF_CB	*qef_cb = dsh->dsh_qefcb;
    i4	i;
    i4	j;
    i4		open_flag = (reset) ? (TOP_OPEN | function) : function;
    DB_STATUS	status	= E_DB_OK;
    char	*tidinput;  /* location of tid */
    DB_TID8	bigtid;
    DMR_CB	*dmr_cb, *pdmr_cb;
    u_i2	partno = 0;

    if (state == DSH_CT_INITIAL)
    {
	/* Check if resource limits are exceeded by this query */
	if (action->qhd_obj.qhd_qep.ahd_estimates && (qef_cb->qef_fl_dbpriv &
	    (DBPR_QROW_LIMIT | DBPR_QDIO_LIMIT | DBPR_QCPU_LIMIT |
	    DBPR_QPAGE_LIMIT | DBPR_QCOST_LIMIT))
	    )
	{
	    if (qeq_chk_resource(dsh, action) != E_DB_OK)
		return (E_DB_ERROR);
	}

        dsh->dsh_qef_rowcount	    = 0;
	dsh->dsh_qef_targcount	    = 0;
    }
    dmr_cb = (DMR_CB*) dsh->dsh_cbs[action->qhd_obj.qhd_qep.ahd_odmr_cb];

    dmr_cb->dmr_flags_mask	    = DMR_BY_TID;
    dmr_cb->dmr_data.data_address   = 0;
    dmr_cb->dmr_data.data_in_size   = 0;

    /* set the tid */

    if (node)
    {
	i = dsh->dsh_qef_rowcount;	/* zero when dsh_ct_initial */
	j = dsh->dsh_qef_targcount;	/* zero when dsh_ct_initial */
	for (;;)
	{
	    if (state == DSH_CT_CONTINUE3)
	    {
		/* Resuming from before trigger, reset things */
		MEcopy(dsh->dsh_row[action->qhd_obj.qhd_qep.u1.s1.ahd_b_tid],
			sizeof(DB_TID8), (PTR) &bigtid);
		partno = bigtid.tid.tid_partno;

		/* Check for partitioning and set up DMR_CB. */
		if (action->qhd_obj.qhd_qep.ahd_part_def)
		{
		    dmr_cb = (DMR_CB *)dsh->dsh_cbs[action->
				qhd_obj.qhd_qep.ahd_odmr_cb+partno+1];
		}
	    }
	    else
	    {
		/* fetch rows (if there are any) */
        	if (action->qhd_obj.qhd_qep.ahd_qepflag & AHD_NOROWS)
        	{
                    /* Negative syllogism assures no rows. */
                    status = E_DB_WARN;
                    dsh->dsh_error.err_code = E_QE0015_NO_MORE_ROWS;
        	}
		 else status = (*node->qen_func)(node, qef_rcb, dsh, open_flag);
		open_flag &= ~(TOP_OPEN | FUNC_RESET);
	    
		/* If the qen_node returned no rows and we are supposed to
		** invalidate this query plan if this action doesn't get
		** any rows, then invalidate the plan.
		*/
		if (dsh->dsh_error.err_code == E_QE0015_NO_MORE_ROWS &&
		    dsh->dsh_xaddrs[node->qen_num]->qex_status->node_rcount == 0 &&
		    (action->ahd_flags & QEA_ZEROTUP_CHK) != 0)
		{
		    dsh->dsh_qp_status |= DSH_QP_OBSOLETE;
		    dsh->dsh_error.err_code = E_QE0023_INVALID_QUERY;
		    qef_rcb->qef_intstate |= QEF_ZEROTUP_CHK;
		    if (dsh->dsh_qp_ptr->qp_ahd != action)
		    {
			status = E_DB_ERROR;
		    }
		    else
		    {
			status = E_DB_WARN;
		    }
		    return(status);
		}

		if (status != E_DB_OK)
		{
		    if (dsh->dsh_error.err_code == E_QE0015_NO_MORE_ROWS)
		    {
			dsh->dsh_error.err_code = E_QE0000_OK;
			status = E_DB_OK;
		    }
		    else if (dsh->dsh_error.err_code == E_DM0047_CHANGED_TUPLE)
		    {
			continue;
		    }
		    break;
		}

		tidinput = (char *) dsh->dsh_row[action->qhd_obj.qhd_qep.ahd_tidrow] +
		    action->qhd_obj.qhd_qep.ahd_tidoffset;
		/* perform the delete */
		if (action->qhd_obj.qhd_qep.ahd_qepflag & AHD_4BYTE_TIDP)
		{
		    I4ASSIGN_MACRO(*tidinput, bigtid.tid_i4.tid);
		    bigtid.tid_i4.tpf = 0;
		}
		else
		{
		    I8ASSIGN_MACRO(*tidinput, bigtid);
		}
		dmr_cb->dmr_tid = bigtid.tid_i4.tid;
		partno = bigtid.tid.tid_partno;

		/* Check for partitioning and set up DMR_CB. */
		if (action->qhd_obj.qhd_qep.ahd_part_def)
		{
		    status = qeq_part_open(qef_rcb, dsh, NULL, 0, 
			action->qhd_obj.qhd_qep.ahd_odmr_cb,
			action->qhd_obj.qhd_qep.ahd_dmtix, partno);
			
		    if (status != E_DB_OK)
			return(status);
		    pdmr_cb = (DMR_CB *)dsh->dsh_cbs[action->
				qhd_obj.qhd_qep.ahd_odmr_cb+partno+1];

		    MEcopy((PTR)&dmr_cb->dmr_tid, sizeof(DB_TID),
				(PTR)&pdmr_cb->dmr_tid);
		    dmr_cb = pdmr_cb;
		    dmr_cb->dmr_flags_mask	    = DMR_BY_TID;
		    dmr_cb->dmr_data.data_address   = 0;
		    dmr_cb->dmr_data.data_in_size   = 0;
		}

		j++;
		/* If we may have rules, save a copy of the row before	    */
		/* deletion. */
		if (action->qhd_obj.qhd_qep.ahd_after_act != NULL ||
	    	    action->qhd_obj.qhd_qep.ahd_before_act != NULL)
		{
		    dmr_cb->dmr_data.data_address =
			dsh->dsh_row[action->qhd_obj.qhd_qep.u1.s1.ahd_b_row];
		    dmr_cb->dmr_data.data_in_size =
			dsh->dsh_qp_ptr->qp_row_len[action->
					qhd_obj.qhd_qep.u1.s1.ahd_b_row];
		    status = dmf_call(DMR_GET, dmr_cb);
		    if (status != E_DB_OK)
		    {
			dsh->dsh_error.err_code = dmr_cb->error.err_code;
			return (status);
		    }
		}

		/* Check for BEFORE triggers and do 'em. */
		if (action->qhd_obj.qhd_qep.ahd_before_act != NULL)
		{
		    /*
		    ** Place the TID in a known location so it can be used
		    ** as a parameter to the procedure fired by the rule.
		    */
		    MEcopy (&bigtid, sizeof(DB_TID8),
		      dsh->dsh_row[action->qhd_obj.qhd_qep.u1.s1.ahd_b_tid]);   

		    /* Rules apply, reset the execution state to point to the 
		    ** rule action list to execute and indicate to the caller 
		    ** that the next action is a rule action. */
		    dsh->dsh_qef_rowcount = i;
		    dsh->dsh_qef_targcount = j;
		    dsh->dsh_ctx_act[dsh->dsh_depth_act].dsh_ct_ptr = action;
		    dsh->dsh_ctx_act[dsh->dsh_depth_act].dsh_ct_status =
			DSH_CT_CONTINUE3;
		    dsh->dsh_depth_act++;
		    dsh->dsh_act_ptr = action->qhd_obj.qhd_qep.ahd_before_act;
		    dsh->dsh_error.err_code = E_QE0120_RULE_ACTION_LIST;
		    status = E_DB_ERROR;
		    return (status);
		}
	    }	/* end of !DSH_CT_CONTINUE3 */

	    state = DSH_CT_INITIAL;		/* signal resumption after
						** BEFORE trigger */

	    status = dmf_call(DMR_DELETE, dmr_cb);
	    if (status != E_DB_OK)
	    {
		if (dmr_cb->error.err_code == E_DM0044_DELETED_TID ||
		    dmr_cb->error.err_code == E_DM0047_CHANGED_TUPLE)
		{
		    status = E_DB_OK;
		    continue;
		}
		if ( dmr_cb->error.err_code == E_DM0029_ROW_UPDATE_CONFLICT )
		{
		    dsh->dsh_qp_status |= DSH_QP_OBSOLETE;
		    dsh->dsh_error.err_code = E_QE0023_INVALID_QUERY;
		    status = E_DB_WARN;
		}
		else
		    dsh->dsh_error.err_code = dmr_cb->error.err_code;
		break;
	    }
	    else
	    {
		/* successful delete */
		i++;
	    }

	    /* After a successful delete, look for rules to apply */
	    if (action->qhd_obj.qhd_qep.ahd_after_act != NULL)
	    {
		/*
		** Place the TID in a known location so it can be used
		** as a parameter to the procedure fired by the rule.
		*/
		MEcopy (&bigtid, sizeof(DB_TID8),
		    dsh->dsh_row[action->qhd_obj.qhd_qep.u1.s1.ahd_b_tid]);   

		/* Rules apply, reset the execution state to point to the   */
		/* rule action list to execute and indicate to the caller   */
		/* that the next action is a rule action. */
		dsh->dsh_qef_rowcount = i;
		dsh->dsh_qef_targcount = j;
		dsh->dsh_ctx_act[dsh->dsh_depth_act].dsh_ct_ptr = action;
		dsh->dsh_ctx_act[dsh->dsh_depth_act].dsh_ct_status =
		    DSH_CT_CONTINUE;
		dsh->dsh_depth_act++;
		dsh->dsh_act_ptr = action->qhd_obj.qhd_qep.ahd_after_act;
		dsh->dsh_error.err_code = E_QE0120_RULE_ACTION_LIST;
		status = E_DB_ERROR;
		return (status);
	    }
	}

	if (status == E_DB_OK)
	{
	    dsh->dsh_qef_rowcount  = i;
	    dsh->dsh_qef_targcount = j;
	}
    }
    return (status);	
}

/*{
** Name: QEA_PDELETE	- delete rows from records in place
**
** Description:
**	Read a row from a QEP and perform the delete on it in place rather
**	than by tid.
**
** Inputs:
**      action                          update action item
**				    E_QE0000_OK
**				    E_QE0017_BAD_CB
**				    E_QE0018_BAD_PARAM_IN_CB
**				    E_QE0019_NON_INTERNAL_FAIL
**				    E_QE0002_INTERNAL_ERROR
**				    E_QE0019_NON_INTERNAL_FAIL
**
**	state			    DSH_CT_INITIAL if this is an action call
**				    DSH_CT_CONTINUE if being called back after
**				    a rule action list is executed.
**
** Outputs:
**      qef_rcb
**	    .qef_rowcount		number of delete tuples
**	    .qef_targcount		number of attempted tuples
**	    .error.err_code		one of the following
**
**	Returns:
**	    E_DB_{OK, WARN, ERROR, FATAL}
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      10-jul-86 (daved)
**          written
**	02-feb-88 (puree)
**	    add reset flag to the function arguments.
**      12-apr-88 (puree)
**          handle both DM0044 and DM0047 the same way.
**	20-feb-89 (paul)
**	    Added support to invoke rule action lists.
**	30-mar-89 (paul)
**	    Add resource limit checking.
**	22-sep-89 (paul)
**	    Added support for using TIDs in rules.
**	28-sept-89 (eric)
**	    Add support to fix the sybase bug. This is done by returning
**	    a zerotup check flag when invalidating a query plan because
**	    it's QEN_NODE returns zero rows.
**       5-Nov-1993 (fred)
**          Added adu_free_los() call to free large object temporaries.
**       7-Apr-1994 (fred)
**          Deleted adu_free_objects code.  It is done in qeq_cleanup().
**      29-oct-97 (inkdo01)
**          Added code to support AHD_NOROWS flag (indicating no possible
**          rows from underlying query).
**	21-nov-03 (inkdo01)
**	    Add open_flag parm to qen_func calls for parallel query support.
**	5-feb-04 (inkdo01)
**	    Added support for partitioned tables.
**	28-feb-04 (inkdo01)
**	    Change qef_rcb parm to dsh in qeq_chk_resources call.
**	12-mar-04 (inkdo01)
**	    Fix to handle bigtids on byte-swapped machines.
**	8-Jul-2004 (schka24)
**	    Still some tid byte-ordering confusion on x86, make tid handling
**	    more portable.
**	28-Feb-2005 (schka24)
**	    Obey 4-byte-tidp flag.
**	20-june-06 (dougi)
**	    Add support for BEFORE triggers (not that they're much use for
**	    DELETE).
**	1-Nov-2006 (kschendel)
**	    Fix up before trigger stuff so that after triggers work too.
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: If E_DM0029_ROW_UPDATE_CONFLICT returned,
**	    invalidate the QP and retry.
[@history_template@]...
*/
DB_STATUS
qea_pdelete(
QEF_AHD		*action,
QEF_RCB		*qef_rcb,
QEE_DSH		*dsh,
i4		function,
i4		state )
{
    QEN_NODE	*node = action->qhd_obj.qhd_qep.ahd_qep;
    bool	reset = ((function & FUNC_RESET) != 0);
    i4	i;
    i4	j;
    i4		open_flag = (reset) ? (TOP_OPEN | function) : function;
    QEF_CB	*qef_cb = dsh->dsh_qefcb;
    DB_STATUS	status	= E_DB_OK;
    DMR_CB	*dmr_cb;
    i4	old_flags_mask;
    i4	old_dmr_tid;    
    char	*old_d_address;
    i4	old_d_in_size;
    char	*tidinput;
    DB_TID8	bigtid;
    
    if (state == DSH_CT_INITIAL)
    {
	/* Check if resource limits are exceeded by this query */
	if (action->qhd_obj.qhd_qep.ahd_estimates && (qef_cb->qef_fl_dbpriv &
	    (DBPR_QROW_LIMIT | DBPR_QDIO_LIMIT | DBPR_QCPU_LIMIT |
	    DBPR_QPAGE_LIMIT | DBPR_QCOST_LIMIT))
	    )
	{
	    if (qeq_chk_resource(dsh, action) != E_DB_OK)
		return (E_DB_ERROR);
	}

	dsh->dsh_qef_rowcount	    = 0;
	dsh->dsh_qef_targcount	    = 0;
    }
    dmr_cb = (DMR_CB*) dsh->dsh_cbs[action->qhd_obj.qhd_qep.ahd_odmr_cb];

    if (node)
    {
	i = dsh->dsh_qef_rowcount;	/* zero when dsh_ct_initial */
	j = dsh->dsh_qef_targcount;	/* zero when dsh_ct_initial */

	for (;;)
	{
	    if (state == DSH_CT_CONTINUE3)
	    {
		/* Resuming from before trigger, reset things */
		MEcopy(dsh->dsh_row[action->qhd_obj.qhd_qep.u1.s1.ahd_b_tid],
			sizeof(DB_TID8), (PTR) &bigtid);
		/* "Save" old stuff that is restored after the delete */
		old_flags_mask = dmr_cb->dmr_flags_mask;
		old_dmr_tid = dmr_cb->dmr_tid;
		old_d_address = dmr_cb->dmr_data.data_address;
		old_d_in_size = dmr_cb->dmr_data.data_in_size;

		if (!(dmr_cb->dmr_flags_mask & DMR_BY_TID))
		{
		    /* Set the dmr_cb values for DMR_DELETE */
		    dmr_cb->dmr_flags_mask = DMR_CURRENT_POS;
		    dmr_cb->dmr_tid = 0;
		    dmr_cb->dmr_data.data_address = 0;
		    dmr_cb->dmr_data.data_in_size = 0;
		}
	    }
	    else
	    {
		/* fetch rows (if there are any) */
        	if (action->qhd_obj.qhd_qep.ahd_qepflag & AHD_NOROWS)
        	{
                    /* Negative syllogism assures no rows. */
                    status = E_DB_WARN;
                    dsh->dsh_error.err_code = E_QE0015_NO_MORE_ROWS;
        	}
	 	else status = (*node->qen_func)(node, qef_rcb, dsh, open_flag);
		open_flag &= ~(TOP_OPEN | FUNC_RESET);
	    
		/* If the qen_node returned no rows and we are supposed to
		** invalidate this query plan if this action doesn't get
		** any rows, then invalidate the plan.
		*/
		if (dsh->dsh_error.err_code == E_QE0015_NO_MORE_ROWS &&
		  dsh->dsh_xaddrs[node->qen_num]->qex_status->node_rcount == 0 &&
		    (action->ahd_flags & QEA_ZEROTUP_CHK) != 0)
		{
		    dsh->dsh_qp_status |= DSH_QP_OBSOLETE;
		    dsh->dsh_error.err_code = E_QE0023_INVALID_QUERY;
		    qef_rcb->qef_intstate |= QEF_ZEROTUP_CHK;
		    if (dsh->dsh_qp_ptr->qp_ahd != action)
		    {
			status = E_DB_ERROR;
		    }
		    else
		    {
			status = E_DB_WARN;
		    }
		    return(status);
		}

		if (status != E_DB_OK)
		{
		    if (dsh->dsh_error.err_code == E_QE0015_NO_MORE_ROWS)
		    {
			dsh->dsh_error.err_code = E_QE0000_OK;
			status = E_DB_OK;
		    }
		    break;
		}

		j++;

		dmr_cb = (DMR_CB*) dsh->dsh_cbs[action->qhd_obj.
							qhd_qep.ahd_odmr_cb];
		/* save dmr_cb values */
		old_flags_mask = dmr_cb->dmr_flags_mask;
		old_dmr_tid = dmr_cb->dmr_tid;
		old_d_address = dmr_cb->dmr_data.data_address;
		old_d_in_size = dmr_cb->dmr_data.data_in_size;
		if (dmr_cb->dmr_flags_mask != DMR_BY_TID)
		{
		    /* set the new dmr_cb values */
		    dmr_cb->dmr_flags_mask = DMR_CURRENT_POS;
		    dmr_cb->dmr_tid = 0;
		    dmr_cb->dmr_data.data_address = 0;
		    dmr_cb->dmr_data.data_in_size = 0;
		}

		/* If we may have rules, save a copy of the row before	    */
		/* deletion. */
		if (action->qhd_obj.qhd_qep.ahd_after_act != NULL ||
		    action->qhd_obj.qhd_qep.ahd_before_act != NULL)
		{
		    /* We have the TID available so look up the row by TID. */
		    tidinput =
		    (char *)dsh->dsh_row[action->qhd_obj.qhd_qep.ahd_tidrow] +
				action->qhd_obj.qhd_qep.ahd_tidoffset;
		    if (action->qhd_obj.qhd_qep.ahd_qepflag & AHD_4BYTE_TIDP)
		    {
			I4ASSIGN_MACRO(*tidinput, bigtid.tid_i4.tid);
			bigtid.tid_i4.tpf = 0;
		    }
		    else
		    {
			I8ASSIGN_MACRO(*tidinput, bigtid);
		    }
		    dmr_cb->dmr_tid = bigtid.tid_i4.tid;
		    /* (schka24) It had better be the right partition - or if
		    ** not, partitioned had better not use PDEL...
		    */
		    dmr_cb->dmr_flags_mask = DMR_BY_TID;
		    dmr_cb->dmr_data.data_address =
			dsh->dsh_row[action->qhd_obj.qhd_qep.u1.s1.ahd_b_row];
		    dmr_cb->dmr_data.data_in_size =
			dsh->dsh_qp_ptr->qp_row_len[action->
					qhd_obj.qhd_qep.u1.s1.ahd_b_row];
		    status = dmf_call(DMR_GET, dmr_cb);
		    if (status != E_DB_OK)
		    {
			dsh->dsh_error.err_code = dmr_cb->error.err_code;
			return (status);
		    }
		    if (old_flags_mask != DMR_BY_TID)
		    {
			/* set the new dmr_cb values */
			dmr_cb->dmr_flags_mask = DMR_CURRENT_POS;
			dmr_cb->dmr_tid = 0;
			dmr_cb->dmr_data.data_address = 0;
			dmr_cb->dmr_data.data_in_size = 0;
		    }
		    else
		    {
			dmr_cb->dmr_flags_mask = old_flags_mask;
			dmr_cb->dmr_tid = old_dmr_tid;
			dmr_cb->dmr_data.data_address = old_d_address;
			dmr_cb->dmr_data.data_in_size = old_d_in_size;
		    }
		}

		/* Check for BEFORE triggers and do 'em. */
		if (action->qhd_obj.qhd_qep.ahd_before_act != NULL)
		{
		    /*
		    ** Place the TID in a known location so it can be used
		    ** as a parameter to the procedure fired by the rule.
		    */
		    MEcopy (&bigtid, sizeof(DB_TID8),
		      dsh->dsh_row[action->qhd_obj.qhd_qep.u1.s1.ahd_b_tid]);   

		    /* Must restore DMR_CB stuff to save across trigger. */
		    dmr_cb->dmr_flags_mask = old_flags_mask;
		    dmr_cb->dmr_tid = old_dmr_tid;
		    dmr_cb->dmr_data.data_address = old_d_address;
		    dmr_cb->dmr_data.data_in_size = old_d_in_size;

		    /* Rules apply, reset the execution state to point to the 
		    ** rule action list to execute and indicate to the caller 
		    ** that the next action is a rule action. */
		    dsh->dsh_qef_rowcount = i;
		    dsh->dsh_qef_targcount = j;
		    dsh->dsh_ctx_act[dsh->dsh_depth_act].dsh_ct_ptr = action;
		    dsh->dsh_ctx_act[dsh->dsh_depth_act].dsh_ct_status =
			DSH_CT_CONTINUE3;
		    dsh->dsh_depth_act++;
		    dsh->dsh_act_ptr = action->qhd_obj.qhd_qep.ahd_before_act;
		    dsh->dsh_error.err_code = E_QE0120_RULE_ACTION_LIST;
		    status = E_DB_ERROR;
		    return (status);
		}
	    }	/* end of !DSH_CT_CONTINUE3 */

	    state = DSH_CT_INITIAL;		/* signal resumption after
						** BEFORE trigger */

	    status = dmf_call(DMR_DELETE, dmr_cb);

	    /* restore the dmr_cb values. */
	    if (dmr_cb->dmr_flags_mask != DMR_BY_TID)
	    {
		dmr_cb->dmr_flags_mask = old_flags_mask;
		dmr_cb->dmr_tid = old_dmr_tid;
		dmr_cb->dmr_data.data_address = old_d_address;
		dmr_cb->dmr_data.data_in_size = old_d_in_size;
	    }

	    if (status != E_DB_OK)
	    {
		if (dmr_cb->error.err_code == E_DM0044_DELETED_TID ||
		    dmr_cb->error.err_code == E_DM0047_CHANGED_TUPLE)
		{
		    status = E_DB_OK;
		    continue;
		}
		if ( dmr_cb->error.err_code == E_DM0029_ROW_UPDATE_CONFLICT )
		{
		    dsh->dsh_qp_status |= DSH_QP_OBSOLETE;
		    dsh->dsh_error.err_code = E_QE0023_INVALID_QUERY;
		    status = E_DB_WARN;
		}
		else
		    dsh->dsh_error.err_code = dmr_cb->error.err_code;
		break;
		
	    }
	    else
	    {
		/* successful delete */
		i++;
	    }
	    /* After a successful delete, look for rules to apply */
	    if (action->qhd_obj.qhd_qep.ahd_after_act != NULL)
	    {
		/*
		** Place the TID in a known location so it can be used
		** as a parameter to the procedure fired by the rule.
		*/
		MEcopy (&bigtid, sizeof(DB_TID8),
		    dsh->dsh_row[action->qhd_obj.qhd_qep.u1.s1.ahd_b_tid]);   
		/* Rules apply, reset the execution state to point to the   */
		/* rule action list to execute and indicate to the caller   */
		/* that the next action is a rule action. */
		dsh->dsh_qef_rowcount = i;
		dsh->dsh_qef_targcount = j;
		dsh->dsh_ctx_act[dsh->dsh_depth_act].dsh_ct_ptr = action;
		dsh->dsh_ctx_act[dsh->dsh_depth_act].dsh_ct_status =
		    DSH_CT_CONTINUE;
		dsh->dsh_depth_act++;
		dsh->dsh_act_ptr = action->qhd_obj.qhd_qep.ahd_after_act;
		dsh->dsh_error.err_code = E_QE0120_RULE_ACTION_LIST;
		status = E_DB_ERROR;
		return (status);
	    }

	}

	if (status == E_DB_OK)
	{
	    dsh->dsh_qef_rowcount  = i;
	    dsh->dsh_qef_targcount = j;
	}
    }
    return (status);	
}
