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

static DB_STATUS
qea_retryFailedUpdates(
	QEF_AHD		*action,
	QEF_RCB		*qef_rcb,
	QEE_DSH		*dsh,
	DMR_CB		*dmr_cb,
	i4		rowno,
	i4		state);
/**
**
**  Name: QEAREPLACE.C - provides tuple replace support
**
**  Description:
**      The routines in this file replaces records to open
**  relations. 
**
**          qea_replace - replace one or more records to the open relation.
**          qea_preplace - replace one or more records in place.
**
**
**  History:
**      10-jul-86 (daved)
**          written
**	02-feb-88 (puree)
**	    add reset flag to qea_replace and qea_preplace function arguments.
**	29-aug-88 (puree)
**	    change duplicate handling semantics
**	20-feb-89 (paul)
**	    Update to handle rule action lists.
**	30-mar-89 (paul)
**	    Add resource limit checking.
**	20-sep-89 (paul)
**	    Add support for TIDs as procedure parameters in rules.
**	16-nov-89 (paul)
**	    Fix bug 8732. non-positioned replaces did not always fire
**	    rules correctly because a copy of the old row was never held.
**	08-dec-92 (anitap)
**	    Included <qsf.h> and <psfparse.h> for CREATE SCHEMA.
**	12-feb-93 (jhahn)
**	    Added support for statement level unique constraints. (FIPS)
**	02-apr-93 (jhahn)
**	    Various fixes for support of statement level unique constraints.
**	06-may-93 (anitap)
**          Added support for "SET UPDATE_ROWCOUNT" statement.
**	    Fixed compiler warnings in qea_retryFailedUpdates(), qea_replace()
**	    and qea_preplace().
**	17-may-93 (jhahn)
**	    More fixes for support of statement level unique constraints.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	01-sep-93 (davebf)
**	    Fix bug 40262.  rowcount += i, targcount += j strategy returned
**	    wrong counts when there were actions internal to QEP
**      13-sep-93 (smc)
**          Added <cs.h> for CS_SID.
**	18-oct-93 (anitap)
**	    Added support for "SET UPDATE_ROWCOUNT" statement in qea_replace().
**       5-Nov-1993 (fred)
**          Remove large object temporaries...
**       7-Apr-1994 (fred)
**          Commented out large object removal pending better way...
**	26-june-1997(angusm)
**	    Pass down flags w.r. cross-table update (77040, 80750, 79623)
**	7-nov-95 (inkdo01)
**	    Changes to replace QEN_ADF structure instances by pointers in
**	    QEF_AHD structures.
**      05-may-1999 (nanpr01,stial01)
**          Set DMR_DUP_ROLLBACK when duplicate error will result in rollback
**          This allows DMF to dupcheck in indices after base table is updated.
**	24-jan-01 (inkdo01)
**	    Changes to QEF_QEP structure putting some fields in union 
**	    (for hash aggregation project).
**      12-may-2003 (stial01)
**          Fixed test for (dmr_flags_mask & DMR_BY_TID), since other flags
**          may be set. (b110226).
**	23-feb-04 (inkdo01)
**	    Changed qef_rcb->error to dsh->dsh_error for || thread safety
**	    (and others).
**	28-Dec-2005 (kschendel)
**	    Use DSH references whenever possible instead of QEF RCB
**	    references.  (This simplifies the code, eliminating many
**	    RCB references, and reflects the new primacy of the DSH
**	    thanks to the parallel query changes.)
**	16-Jan-2006 (kschendel)
**	    Access qen-status thru xaddrs.
**	4-Jun-2009 (kschendel) b122118
**	    Minor dead code removal.
[@history_template@]...
**/


/*
** {
** Name: QEA_REPLACE	- replace rows to records
**
** Description:
**	Read a row from a QEP and perform the replace on it.
**
** Inputs:
**      action                          update action item
**				    E_QE0000_OK
**				    E_QE0017_BAD_CB
**				    E_QE0018_BAD_PARAM_IN_CB
**				    E_QE0002_INTERNAL_ERROR
**				    E_QE0019_NON_INTERNAL_FAIL
**				    E_QE0012_DUPLICATE_KEY
**				    E_QE0010_DUPLICATE_ROW
**				    E_QE0011_AMBIGUOUS_REPLACE
**
**	state			    DSH_CT_INITIAL for a replace action
**				    DSH_CT_CONTINUE if being called back after
**				    a rule action list is executed.
**				    DSH_CT_CONTINUE2 if being called back after
**				    a rule action list is executed while
**				    retrying an update for a statement level
**				    unique constraint.
**
** Outputs:
**      qef_rcb
**	    .qef_rowcount		number of replaced tuples
**	    .qef_targcount		number of attempted updates
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
**	29-aug-88 (puree)
**	    change duplicate handling semantics
**	20-feb-89 (paul)
**	    Added support for invoking rule action lists.
**	30-mar-89 (paul)
**	    Add resource limit checking
**	28-sept-89 (eric)
**	    Add support to fix the sybase bug. This is done by returning
**	    a zerotup check flag when invalidating a query plan because
**	    it's QEN_NODE returns zero rows.
**	16-nov-89 (paul)
**	    Fix bug 8732. non-positioned replaces did not always fire
**	    rules correctly because a copy of the old row was never held.
**	12-feb-93 (jhahn)
**	    Added support for statement level unique constraints. (FIPS)
**	02-apr-93 (jhahn)
**	    Various fixes for support of statement level unique constraints.
**	11-may-93 (anitap)
**	    Fixed compiler warning.
**	17-may-93 (jhahn)
**	    More fixes for support of statement level unique constraints.
**	01-sep-93 (davebf)
**	    Fix bug 40262.  rowcount += i, targcount += j strategy returned
**	    wrong counts when there were actions internal to QEP
**	18-oct-93 (anitap)
**          Added support for "SET UPDATE_ROWCOUNT" statement.
**       5-Nov-1993 (fred)
**          Remove large object temporaries as necessary.
**	28-jan-94 (anitap)
**	    Corrected typo. Changed '&' to '&&' in comparison.
**       7-Apr-1994 (fred)
**          Commented out large object removal pending better way...
**	26-june-1997(angusm)
**	    Pass down flags w.r. cross-table update (77040, 80750, 79623)
**	7-nov-95 (inkdo01)
**	    Changes to replace QEN_ADF structure instances by pointers in
**	    QEF_AHD structures.
**      06-mar-1996 (nanpr01)
**          Changed the parameter qen_putIntoTempTable for passing the
**	    temporary table page size.
**      29-oct-97 (inkdo01)
**          Added code to support AHD_NOROWS flag (indicating no possible
**          rows from underlying query).
**	18-mar-02 (inkdo01)
**	    Added code to perform next/current value operations on sequences.
**	21-nov-03 (inkdo01)
**	    Add open_flag parm to qen_func calls for parallel query support.
**	29-dec-03 (inkdo01)
**	    DSH is now parameter, "function" replaces "reset".
**	5-feb-04 (inkdo01)
**	    Add support to update partitioned tables.
**	27-feb-04 (inkdo01)
**	    Changed dsh_qen_status from ptr to array of QEN_STATUSs to 
**	    ptr to arrays of ptrs.
**	28-feb-04 (inkdo01)
**	    Change qef_rcb parm to dsh in qeq_chk_resources call.
**	11-mar-04 (inkdo01)
**	    Fix TID mucking code to support byte-swapped 32-bit machines.
**	19-mar-04 (inkdo01)
**	    More bigtid stuff - for rules processing.
**	12-may-04 (inkdo01)
**	    Call qeq_part_open unconditionally - it'll determine whether
**	    partition cbs are already prepared.
**	12-may-04 (inkdo01)
**	    Fix computation of new tid (8 bytes) for rules processing.
**	8-Jul-2004 (schka24)
**	    Still some tid byte-ordering confusion on x86, make tid handling
**	    more portable.  Fix partition changing with statement level
**	    unique processing.
**	20-june-06 (dougi)
**	    Add support for BEFORE triggers.
**	09-sep-06 (toumi01) BUG 116609
**	    Compute tidinput address within loop; it moves.
**	1-Nov-2007 (kibro01) b119003
**	    Added E_DM9581_REP_SHADOW_DUPLICATE as possible error
**	3-Mar-2008 (kibro01) b119725
**	    Don't overwrite the tid if we've broken out for a BEFORE rule.
**	8-Sep-2008 (kibro01) b120693
**	    Remove unused action parameter from qeq_part_open
**	09-Sep-2009 (thaju02) B122374
**	    If BEFORE trigger has been executed, do not use ahd_upd_colmap.
**	    ahd_upd_colmap may not reflect cols updated by the rule/proc. 
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: If E_DM0029_ROW_UPDATE_CONFLICT returned,
**	    invalidate the QP and retry.
[@history_template@]...
*/
DB_STATUS
qea_replace(
QEF_AHD		*action,
QEF_RCB		*qef_rcb,
QEE_DSH		*dsh,
i4		function,
i4		state )
{
    QEN_NODE	*node = action->qhd_obj.qhd_qep.ahd_qep;
    bool	reset = ((function & FUNC_RESET) != 0);
    i4	i, j;
    i4		open_flag = (reset) ? (TOP_OPEN | function) : function;
    QEF_CB	*qef_cb = dsh->dsh_qefcb;
    PTR		*cbs	= dsh->dsh_cbs;
    DB_STATUS	status	= E_DB_OK;
    QEN_ADF	*qen_adf = action->qhd_obj.qhd_qep.ahd_current;
    i4		rowno   = qen_adf->qen_output;
    i4		b_row	= action->qhd_obj.qhd_qep.u1.s1.ahd_b_row;
    char	*output = dsh->dsh_row[rowno];
    char	*tidinput;  /* location of tid */
    DB_TID8	oldbigtid, newbigtid;
    DMR_CB	*dmr_cb, *pdmr_cb;
    ADE_EXCB	*ade_excb;
    u_i2	oldpartno = 0;
    u_i2	newpartno = 0;
    bool	old_deleted;

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

	if (action->ahd_flags & QEF_STATEMENT_UNIQUE)
	{
	    status = qen_initTempTable(dsh,
				       action->qhd_obj.qhd_qep
				       .u1.s1.ahd_failed_a_temptable
				       ->ttb_tempTableIndex);
	    if (status != E_DB_OK)
		return (status);
	    if (b_row>=0)
	    {
		status = qen_initTempTable(dsh, 
					   action->qhd_obj.qhd_qep
					   .u1.s1.ahd_failed_b_temptable
					   ->ttb_tempTableIndex);
		if (status != E_DB_OK)
		    return (status);
	    }
	}
        dsh->dsh_qef_rowcount	    = 0;
	dsh->dsh_qef_targcount	    = 0;
    }
    dmr_cb = (DMR_CB*) cbs[action->qhd_obj.qhd_qep.ahd_odmr_cb];
    /* materialize row into update buffer */
    ade_excb    = (ADE_EXCB*) cbs[qen_adf->qen_pos];	

    dmr_cb->dmr_flags_mask	    = DMR_BY_TID;

    if (state == DSH_CT_INITIAL)
    {
	/* dmr_tid holds the current tid if we've broken out to execute
	** a rule.  (kibro01) b119725
	*/
	dmr_cb->dmr_tid		    = 0;
    }
    dmr_cb->dmr_data.data_address   = output;
    dmr_cb->dmr_data.data_in_size   = dsh->dsh_qp_ptr->qp_row_len[rowno];

    if (state == DSH_CT_CONTINUE2)
    {
	return (qea_retryFailedUpdates(action, qef_rcb, dsh, dmr_cb, rowno,
				       state));
    }

    if (node)
    {
	i = dsh->dsh_qef_rowcount;	/* if dsh_ct_initial, zero */
	j = dsh->dsh_qef_targcount;	/* if dsh_ct_initial. zero */
	for (;;)
	{
	    /* Skip all this stuff if resuming after a BEFORE trigger. */
	    if (state != DSH_CT_CONTINUE3)
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
		    dsh->dsh_xaddrs[node->qen_num]->
					qex_status->node_rcount == 0 &&
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
			break;
		    }
		    return (status);
		}

		/* set the tid */
		tidinput = (char *) dsh->dsh_row[action->qhd_obj.qhd_qep.ahd_tidrow] +
			action->qhd_obj.qhd_qep.ahd_tidoffset;
		if (action->qhd_obj.qhd_qep.ahd_qepflag & AHD_4BYTE_TIDP)
		{
		    I4ASSIGN_MACRO(*tidinput, oldbigtid.tid_i4.tid);
		    oldbigtid.tid_i4.tpf = 0;
		}
		else
		{
		    I8ASSIGN_MACRO(*tidinput, oldbigtid);
		}
		oldpartno = oldbigtid.tid.tid_partno;
		dmr_cb->dmr_tid = oldbigtid.tid_i4.tid;
		newpartno = oldpartno;	/* Assume not changing partitions */

		/* Check for partitioning and set up DMR_CB. */
		if (action->qhd_obj.qhd_qep.ahd_part_def)
		{
		    status = qeq_part_open(qef_rcb, dsh, NULL, 0, 
			action->qhd_obj.qhd_qep.ahd_odmr_cb,
			action->qhd_obj.qhd_qep.ahd_dmtix, oldpartno);
			
		    if (status != E_DB_OK)
			return(status);
		    pdmr_cb = (DMR_CB *)dsh->dsh_cbs[action->
			qhd_obj.qhd_qep.ahd_odmr_cb+oldpartno+1];
		    MEcopy((PTR)&dmr_cb->dmr_tid, sizeof(DB_TID),
				(PTR)&pdmr_cb->dmr_tid);
		    dmr_cb = pdmr_cb;
		    dmr_cb->dmr_flags_mask = DMR_BY_TID;
		    dmr_cb->dmr_data.data_address   = output;
		    dmr_cb->dmr_data.data_in_size   = 
				dsh->dsh_qp_ptr->qp_row_len[rowno];
		}

		j++;
		/* get the tuple to be updated. */
		old_deleted = FALSE;
		status = dmf_call(DMR_GET, dmr_cb);
		if (status != E_DB_OK)
		{
		    dsh->dsh_error.err_code = dmr_cb->error.err_code;
		    return (status);
		}

		/* Copy the tuple to be updated.
		** For rule processing we may need both the before and after
		** images of the row.
		*/
		if (b_row != -1)
		{
		    MEcopy(output, action->qhd_obj.qhd_qep.ahd_repsize,
			dsh->dsh_row[b_row]);
		}

		/* Before running current CX, check for sequences to update. */
		if (action->qhd_obj.qhd_qep.ahd_seq != NULL)
		{
		    status = qea_seqop(action, dsh);
		    if (status != E_DB_OK)
		    return(status);
		}

		/* update the tuple */
		status = qen_execute_cx(dsh, ade_excb);
		if (status != E_DB_OK)
		    return (status);

		if (action->qhd_obj.qhd_qep.ahd_before_act != NULL)
		{
		    /*
		    ** Place the TID in a known location so it can be used
		    ** as a parameter to the procedure fired by the rule.
		    */
                    newbigtid.tid_i4.tpf = 0;
                    newbigtid.tid_i4.tid = -1;
                    newbigtid.tid.tid_partno = 0;
		    MEcopy ((PTR)&newbigtid, sizeof(DB_TID8),
			dsh->dsh_row[action->qhd_obj.qhd_qep.u1.s1.ahd_a_tid]);
		    MEcopy ((PTR)&oldbigtid, sizeof(DB_TID8),
			dsh->dsh_row[action->qhd_obj.qhd_qep.u1.s1.ahd_b_tid]); 
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
	
	    /* Check for partitioning and see if we changed target partition. */
	    if (action->qhd_obj.qhd_qep.ahd_part_def && 
		(action->qhd_obj.qhd_qep.ahd_qepflag & AHD_PCOLS_UPDATE))
	    {
		status = adt_whichpart_no(dsh->dsh_adf_cb,
			action->qhd_obj.qhd_qep.ahd_part_def, 
			output, &newpartno);
		if (status != E_DB_OK)
		    return(status);
	    }

	    if (action->qhd_obj.qhd_qep.ahd_qepflag & AHD_PCOLS_UPDATE &&
		oldpartno != newpartno)
	    {
		/* Partition has changed for row in partitioned table.
		** Delete from 1st partition and insert into next. */
		status = dmf_call(DMR_DELETE, dmr_cb);
		if (status != E_DB_OK)
		{
		    if (dmr_cb->error.err_code == E_DM0055_NONEXT)
			dsh->dsh_error.err_code = 4599;
		    else
			dsh->dsh_error.err_code = dmr_cb->error.err_code;
		    break;
		}
		old_deleted = TRUE;	/* Remember that old row is gone */

		/* Get output partition DMR_CB, then do DMR_PUT. */
		status = qeq_part_open(qef_rcb, dsh, NULL, 0, 
		    action->qhd_obj.qhd_qep.ahd_odmr_cb,
		    action->qhd_obj.qhd_qep.ahd_dmtix, newpartno);
			
		if (status != E_DB_OK)
		    return(status);
		pdmr_cb = (DMR_CB *)dsh->dsh_cbs[action->
			qhd_obj.qhd_qep.ahd_odmr_cb+newpartno+1];

		/* Final preparation of new partition DMR_CB. */
		dmr_cb = pdmr_cb;
		dmr_cb->dmr_flags_mask = 0;
		dmr_cb->dmr_tid = 0;
		dmr_cb->dmr_val_logkey = 0;
		dmr_cb->dmr_data.data_address = output;
		dmr_cb->dmr_data.data_in_size  =
				action->qhd_obj.qhd_qep.ahd_repsize;
		if (action->qhd_obj.qhd_qep.ahd_duphandle != QEF_SKIP_DUP)
		    dmr_cb->dmr_flags_mask |= DMR_DUP_ROLLBACK;
		status = dmf_call(DMR_PUT, dmr_cb);
	    }
	    else
	    {
		/* put the updated tuple back into the relation */
		if (action->ahd_flags & QEA_XTABLE_UPDATE)
		    dmr_cb->dmr_flags_mask |= DMR_XTABLE;
		if ((action->qhd_obj.qhd_qep.ahd_upd_colmap) &&
		    (action->qhd_obj.qhd_qep.ahd_before_act == NULL))
		    dmr_cb->dmr_attset = 
				(char *)action->qhd_obj.qhd_qep.ahd_upd_colmap;
		else
		    dmr_cb->dmr_attset = (char *)0;
		if (action->qhd_obj.qhd_qep.ahd_duphandle != QEF_SKIP_DUP)
		    dmr_cb->dmr_flags_mask |= DMR_DUP_ROLLBACK;
		status = dmf_call(DMR_REPLACE, dmr_cb);
	    }

	    /* Construct new TID for rules processing. Even if row hasn't 
	    ** switched partitions, it may have moved in the partition. */
	    newbigtid.tid_i4.tid = dmr_cb->dmr_tid;
	    newbigtid.tid_i4.tpf = 0;
	    newbigtid.tid.tid_partno = newpartno;

	    /* Check status of DMR_REPLACE/PUT. */
	    if (status != E_DB_OK)
	    {
		if (action->ahd_flags & QEF_STATEMENT_UNIQUE &&
		    (dmr_cb->error.err_code == E_DM0150_DUPLICATE_KEY_STMT ||
		     dmr_cb->error.err_code == E_DM0151_SIDUPLICATE_KEY_STMT))
		{
		    char *a_tuple_ptr = dsh->dsh_row[action->qhd_obj.qhd_qep
					  .u1.s1.ahd_failed_a_temptable->ttb_tuple];
		    MEcopy(output, action->qhd_obj.qhd_qep.ahd_repsize,
			   (PTR) a_tuple_ptr);
		    if (b_row>=0)
			MEcopy(&newbigtid, sizeof(DB_TID8), (PTR) (a_tuple_ptr 
				      + action->qhd_obj.qhd_qep.ahd_repsize));
		    status = qen_putIntoTempTable(dsh,
						  action->qhd_obj.qhd_qep
						  .u1.s1.ahd_failed_a_temptable);
		    if (status != E_DB_OK)
			return (status);
		    if (b_row>=0)
		    {
			char *b_tuple_ptr = dsh->dsh_row[b_row];
			
			MEcopy(&oldbigtid, sizeof(DB_TID8), (PTR) (b_tuple_ptr 
				      + action->qhd_obj.qhd_qep.ahd_repsize));
			status = qen_putIntoTempTable(dsh,
						      action->qhd_obj.qhd_qep
						      .u1.s1.ahd_failed_b_temptable);
			if (status != E_DB_OK)
			    return (status);
		    }
		    /* Delete old row unless we did for partition changing */
		    if (! old_deleted)
		    {
			status = dmf_call(DMR_DELETE, dmr_cb);
			if (status != E_DB_OK)
			{
			    if (dmr_cb->error.err_code == E_DM0044_DELETED_TID
			     || dmr_cb->error.err_code == E_DM0047_CHANGED_TUPLE)
			    {
				status = E_DB_OK;
			    }
			    else
			    {
				dsh->dsh_error.err_code =
				    dmr_cb->error.err_code;
				return (status);
			    }
			}
		    } /* ! old_deleted */
		}
		else if ((dmr_cb->error.err_code == E_DM0046_DUPLICATE_RECORD ||
			  dmr_cb->error.err_code == E_DM9581_REP_SHADOW_DUPLICATE ||
			  dmr_cb->error.err_code == E_DM0045_DUPLICATE_KEY ||
			  dmr_cb->error.err_code == E_DM0048_SIDUPLICATE_KEY) &&
			 action->qhd_obj.qhd_qep.ahd_duphandle == QEF_SKIP_DUP)
		{
		    /* do nothing */
		    status = E_DB_OK;
		}
		else
		{
		    if (dmr_cb->error.err_code == E_DM0045_DUPLICATE_KEY ||
		        dmr_cb->error.err_code == E_DM0048_SIDUPLICATE_KEY)
			dsh->dsh_error.err_code = E_QE004D_DUPLICATE_KEY_UPDATE;
		    else
		    if ( dmr_cb->error.err_code == E_DM0029_ROW_UPDATE_CONFLICT )
		    {
			dsh->dsh_qp_status |= DSH_QP_OBSOLETE;
			dsh->dsh_error.err_code = E_QE0023_INVALID_QUERY;
			status = E_DB_WARN;
		    }
		    else
			dsh->dsh_error.err_code = dmr_cb->error.err_code;
		    return (status);
		}

	    }
	    else
	    {
		/* Only increment the rowcount if "SET UPDATE_ROWCOUNT TO
                ** CHANGED" and no error from DMF or if "SET UPDATE_ROWCOUNT TO
                ** QUALIFIED."
                */
                if ((qef_cb->qef_upd_rowcnt == QEF_ROWCHGD &&
                   dmr_cb->error.err_code != E_DM0154_DUPLICATE_ROW_NOTUPD) ||
                   (qef_cb->qef_upd_rowcnt == QEF_ROWQLFD))
		{
		/* successful replace */
		i++;

		/* After a successful update, look for rules to apply */
		if (action->qhd_obj.qhd_qep.ahd_after_act != NULL)
		{
		    /*
		    ** Place the TID in a known location so it can be used
		    ** as a parameter to the procedure fired by the rule.
		    */
		    MEcopy ((PTR)&newbigtid, sizeof(DB_TID8),
			dsh->dsh_row[action->qhd_obj.qhd_qep.u1.s1.ahd_a_tid]);   
		    MEcopy ((PTR)&oldbigtid, sizeof(DB_TID8),
			dsh->dsh_row[action->qhd_obj.qhd_qep.u1.s1.ahd_b_tid]);   

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
	    }
	}
	dsh->dsh_qef_rowcount = i;
	dsh->dsh_qef_targcount = j;
    }
    if ((action->ahd_flags & QEF_STATEMENT_UNIQUE) &&
	! (dsh->dsh_tempTables[action->qhd_obj.qhd_qep
			       .u1.s1.ahd_failed_a_temptable
			       ->ttb_tempTableIndex]->tt_statusFlags & TT_EMPTY))
	status = qea_retryFailedUpdates(action, qef_rcb, dsh, dmr_cb, rowno,
					state);
    return (status);	
}

/*
** {
** Name: QEA_PREPLACE	- replace rows to records in place.
**
** Description:
**	Read a row from a QEP and perform the replace on it in place at the 
**	current position. This differs from QEA_REPLACE() since it positions
**	by tid.
**
** Inputs:
**      action                          update action item
**				    E_QE0000_OK
**				    E_QE0017_BAD_CB
**				    E_QE0018_BAD_PARAM_IN_CB
**				    E_QE0002_INTERNAL_ERROR
**				    E_QE0019_NON_INTERNAL_FAIL
**				    E_QE0012_DUPLICATE_KEY
**				    E_QE0010_DUPLICATE_ROW
**				    E_QE0011_AMBIGUOUS_REPLACE
**
**	state			    DSH_CT_INITIAL for a replace action
**				    DSH_CT_CONTINUE if being called back after
**				    a rule action list is executed.
**				    DSH_CT_CONTINUE2 if being called back after
**				    a rule action list is executed while
**				    retrying an update for a statement level
**				    unique constraint.
**
** Outputs:
**      qef_rcb
**	    .qef_rowcount		number of replaced tuples
**	    .qef_targcount		number of attempted updates
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
**	29-aug-88 (puree)
**	    change duplicate handling semantics
**	20-feb-89 (paul)
**	    Added support for invoking rule action lists.
**	30-mar-89 (paul)
**	    Add resource limit checking.
**	28-sept-89 (eric)
**	    Add support to fix the sybase bug. This is done by returning
**	    a gzerotup check flag when invalidating a query plan because
**	    it's QEN_NODE returns zero rows.
**	12-feb-93 (jhahn)
**	    Added support for statement level unique constraints. (FIPS)
**	02-apr-93 (jhahn)
**	    Various fixes for support of statement level unique constraints.
**      06-may-93 (anitap)
**          Added support for "SET UPDATE_ROWCOUNT" statement.
**	    Fixed compiler warning.
**	17-may-93 (jhahn)
**	    More fixes for support of statement level unique constraints.
**	9-jul-93 (rickh)
**	    Made a MEcopy call conform to its prototype.
**	01-sep-93 (davebf)
**	    Fix bug 40262.  rowcount += i, targcount += j strategy returned
**	    wrong counts when there were actions internal to QEP
**       5-Nov-1993 (fred)
**          Remove large object temporaries as necessary...
**	28-jan-94 (anitap)
**	    Corrected typo. Changed '&' to '&&' in comparison.
**       7-Apr-1994 (fred)
**          Commented out large object removal pending better way...
**      06-mar-1996 (nanpr01)
**          Changed the parameter qen_putIntoTempTable for passing the
**	    temporary table page size.
**      29-oct-97 (inkdo01)
**          Added code to support AHD_NOROWS flag (indicating no possible
**          rows from underlying query).
**	18-mar-02 (inkdo01)
**	    Added code to perform next/current value operations on sequences.
**	21-nov-03 (inkdo01)
**	    Add open_flag parm to qen_func calls for parallel query support.
**	29-dec-03 (inkdo01)
**	    DSH is now parameter, "function" replaces "reset".
**	5-feb-04 (inkdo01)
**	    Add support to update partitioned tables.
**	27-feb-04 (inkdo01)
**	    Changed dsh_qen_status from ptr to array of QEN_STATUSs to 
**	    ptr to arrays of ptrs.
**	28-feb-04 (inkdo01)
**	    Change qef_rcb parm to dsh in qeq_chk_resources call.
**	11-mar-04 (inkdo01)
**	    Fix TID mucking code to support byte-swapped 32-bit machines.
**	19-mar-04 (inkdo01)
**	    More bigtid stuff - for rules processing.
**	12-may-04 (inkdo01)
**	    Call qeq_part_open unconditionally - it'll determine whether
**	    partition cbs are already prepared.
**	12-may-04 (inkdo01)
**	    Fix computation of new tid (8 bytes) for rules processing.
**	8-Jul-2004 (schka24)
**	    Still some tid byte-ordering confusion on x86, make tid handling
**	    more portable.
**	20-june-06 (dougi)
**	    Add support for BEFORE triggers.
**	09-sep-06 (toumi01) BUG 116609
**	    Compute tidinput address within loop; it moves.
**	8-Sep-2008 (kibro01) b120693
**	    Remove unused action parameter from qeq_part_open
**      01-Dec-2008 (coomi01) b121301
**          Test DSH for presence of DBproc execution.
**	09-Sep-2009 (thaju02) B122374
**	    If BEFORE trigger has been executed, do not use ahd_upd_colmap.
**	    ahd_upd_colmap may not reflect cols updated by the rule/proc.
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: If E_DM0029_ROW_UPDATE_CONFLICT returned,
**	    invalidate the QP and retry.
[@history_template@]...
*/
DB_STATUS
qea_preplace(
QEF_AHD		*action,
QEF_RCB		*qef_rcb,
QEE_DSH		*dsh,
i4		function,
i4		state )
{
    QEN_NODE	*node = action->qhd_obj.qhd_qep.ahd_qep;
    bool	reset = ((function & FUNC_RESET) != 0);
    i4	i, j;
    i4		open_flag = (reset) ? (TOP_OPEN | function) : function;
    QEF_CB	*qef_cb = dsh->dsh_qefcb;
    PTR		*cbs	= dsh->dsh_cbs;
    DB_STATUS	status	= E_DB_OK;
    QEN_ADF	*qen_adf = action->qhd_obj.qhd_qep.ahd_current;
    i4		rowno   = qen_adf->qen_output;
    char	*output = dsh->dsh_row[rowno];
    char	*tidinput;
    DMR_CB	*dmr_cb, *pdmr_cb, *olddmr_cb;
    ADE_EXCB	*ade_excb;
    i4		reprow	= action->qhd_obj.qhd_qep.ahd_reprow;
    i4		b_row	= action->qhd_obj.qhd_qep.u1.s1.ahd_b_row;
    i4	old_flags_mask;
    DB_TID8	oldbigtid, newbigtid;
    char	*old_d_address;
    i4	old_d_in_size;
    u_i2	oldpartno = 0;
    u_i2	newpartno = 0;
    bool	old_deleted;
    
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

	if (action->ahd_flags & QEF_STATEMENT_UNIQUE)
	{
	    status = qen_initTempTable(dsh,
				       action->qhd_obj.qhd_qep
				       .u1.s1.ahd_failed_a_temptable
				       ->ttb_tempTableIndex);
	    if (status != E_DB_OK)
		return (status);
	    if (b_row>=0)
	    {
		status = qen_initTempTable(dsh,
					   action->qhd_obj.qhd_qep
					   .u1.s1.ahd_failed_b_temptable
					   ->ttb_tempTableIndex);
		if (status != E_DB_OK)
		    return (status);
	    }
	}
	dsh->dsh_qef_rowcount	    = 0;
	dsh->dsh_qef_targcount	    = 0;
    }
    dmr_cb = (DMR_CB*) cbs[action->qhd_obj.qhd_qep.ahd_odmr_cb];
    olddmr_cb = dmr_cb;
    /* materialize row into update buffer */
    ade_excb    = (ADE_EXCB*) cbs[qen_adf->qen_pos];	

    /* Bug 121301 :: We are deleting temporary blobs too early 
    **               inside dbprocs. This leads to a SegV.
    **
    ** Before making the dmf call, we set a bitflag, dmr_dsh_isdbp, 
    ** to say whether we are executing inside a dbproc.
    */
    dmr_cb->dmr_dsh_isdbp = (dsh->dsh_qp_ptr->qp_status & QEQP_ISDBP) > 0 ? 1 : 0;


    if (state == DSH_CT_CONTINUE2)
    {
	return (qea_retryFailedUpdates(action, qef_rcb, dsh, dmr_cb, rowno,
				       state));
    }
    if (node)
    {
	i = dsh->dsh_qef_rowcount;	/* if dsh_ct_initial, zero */
	j = dsh->dsh_qef_targcount;	/* if dsh_ct_initial. zero */
	for (;;)
	{
	    /* Skip all this stuff if resuming after a BEFORE trigger. */
	    if (state != DSH_CT_CONTINUE3)
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
		    dsh->dsh_xaddrs[node->qen_num]->
					qex_status->node_rcount == 0 &&
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
			break;
		    }
		    return (status);
		}

		j++;
 		/* Set the TID. */
		dmr_cb = (DMR_CB*) cbs[action->qhd_obj.qhd_qep.ahd_odmr_cb];
		olddmr_cb = dmr_cb;
		tidinput = (char *) dsh->dsh_row[action->qhd_obj.qhd_qep.ahd_tidrow]
			+ action->qhd_obj.qhd_qep.ahd_tidoffset;
		if (action->qhd_obj.qhd_qep.ahd_qepflag & AHD_4BYTE_TIDP)
		{
		    I4ASSIGN_MACRO(*tidinput, oldbigtid.tid_i4.tid);
		    oldbigtid.tid_i4.tpf = 0;
		}
		else
		{
		    I8ASSIGN_MACRO(*tidinput, oldbigtid);
		}
		dmr_cb->dmr_tid = oldbigtid.tid_i4.tid;
		oldpartno = oldbigtid.tid.tid_partno;
		newpartno = oldpartno;	/* Assume no partition change */

		/* Copy the tuple to be updated.
		** For rule processing we may need both the before and after 
		** images of the row. If the normal update processing will not 
		** make a copy and there are rules to apply, make a copy anyway.
		*/
		if (reprow >= 0 && reprow != rowno)
		{
		    MEcopy(dsh->dsh_row[reprow], 
			    action->qhd_obj.qhd_qep.ahd_repsize, output);
		}
		if (b_row >= 0 && b_row != reprow)
		{
		    MEcopy(output, action->qhd_obj.qhd_qep.ahd_repsize,
			dsh->dsh_row[b_row]);
		}
		/* Before running current CX, check for sequences to update. */
		if (action->qhd_obj.qhd_qep.ahd_seq != NULL)
		{
		    status = qea_seqop(action, dsh);
		    if (status != E_DB_OK)
			return(status);
		}

		/* update the tuple */
		status = qen_execute_cx(dsh, ade_excb);
		if (status != E_DB_OK)
		    return (status);

		if (action->qhd_obj.qhd_qep.ahd_before_act != NULL)
		{
		    /*
		    ** Place the TID in a known location so it can be used
		    ** as a parameter to the procedure fired by the rule.
		    */
                    newbigtid.tid_i4.tpf = 0;
                    newbigtid.tid_i4.tid = -1;
                    newbigtid.tid.tid_partno = 0;
		    MEcopy ((PTR)&newbigtid, sizeof(DB_TID8),
			dsh->dsh_row[action->qhd_obj.qhd_qep.u1.s1.ahd_a_tid]);
		    MEcopy ((PTR)&oldbigtid, sizeof(DB_TID8),
			dsh->dsh_row[action->qhd_obj.qhd_qep.u1.s1.ahd_b_tid]); 
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
	
	    /* save dmr_cb values */
	    if ((dmr_cb->dmr_flags_mask & DMR_BY_TID) == 0)
	    {
		old_flags_mask = dmr_cb->dmr_flags_mask;
		old_d_address = dmr_cb->dmr_data.data_address;
		old_d_in_size = dmr_cb->dmr_data.data_in_size;

		/* set the new dmr_cb values */
		dmr_cb->dmr_flags_mask = DMR_CURRENT_POS;
		dmr_cb->dmr_tid = 0;
		dmr_cb->dmr_data.data_address = output;
		dmr_cb->dmr_data.data_in_size = dsh->dsh_qp_ptr->qp_row_len[rowno];
	    }

	    /* put the updated tuple back into the relation */

	    /* Check for partitioning and see if we changed target partition. */
	    if (action->qhd_obj.qhd_qep.ahd_part_def && 
		(action->qhd_obj.qhd_qep.ahd_qepflag & AHD_PCOLS_UPDATE))
	    {
		status = adt_whichpart_no(dsh->dsh_adf_cb,
			action->qhd_obj.qhd_qep.ahd_part_def, 
			output, &newpartno);
		if (status != E_DB_OK)
		    return(status);
	    }

	    old_deleted = FALSE;
	    if (action->qhd_obj.qhd_qep.ahd_qepflag & AHD_PCOLS_UPDATE &&
		oldpartno != newpartno)
	    {
		/* Partition has changed for row in partitioned table.
		** Delete from 1st partition and insert into next. */
		status = dmf_call(DMR_DELETE, dmr_cb);
		if (status != E_DB_OK)
		{
		    if (dmr_cb->error.err_code == E_DM0055_NONEXT)
			dsh->dsh_error.err_code = 4599;
		    else
			dsh->dsh_error.err_code = dmr_cb->error.err_code;
		    break;
		}
		old_deleted = TRUE;

		/* Get output partition DMR_CB, then do DMR_PUT. */
		status = qeq_part_open(qef_rcb, dsh, NULL, 0, 
		    action->qhd_obj.qhd_qep.ahd_odmr_cb,
		    action->qhd_obj.qhd_qep.ahd_dmtix, newpartno);
			
		if (status != E_DB_OK)
		    return(status);
		pdmr_cb = (DMR_CB *)dsh->dsh_cbs[action->
			qhd_obj.qhd_qep.ahd_odmr_cb+newpartno+1];

		/* Final preparation of new partition DMR_CB. */
		dmr_cb = pdmr_cb;
		dmr_cb->dmr_flags_mask = 0;
		dmr_cb->dmr_tid = 0;
		dmr_cb->dmr_val_logkey = 0;
		dmr_cb->dmr_data.data_address = output;
		dmr_cb->dmr_data.data_in_size  =
				action->qhd_obj.qhd_qep.ahd_repsize;
		if (action->qhd_obj.qhd_qep.ahd_duphandle != QEF_SKIP_DUP)
		    dmr_cb->dmr_flags_mask |= DMR_DUP_ROLLBACK;
		status = dmf_call(DMR_PUT, dmr_cb);
	    }
	    else
	    {
		if (action->ahd_flags & QEA_XTABLE_UPDATE)
		    dmr_cb->dmr_flags_mask |= DMR_XTABLE;
		if ((action->qhd_obj.qhd_qep.ahd_upd_colmap) && 
		    (action->qhd_obj.qhd_qep.ahd_before_act == NULL))
		    dmr_cb->dmr_attset = 
				(char *)action->qhd_obj.qhd_qep.ahd_upd_colmap;
		else
		    dmr_cb->dmr_attset = (char *)0;
		if (action->qhd_obj.qhd_qep.ahd_duphandle != QEF_SKIP_DUP)
		    dmr_cb->dmr_flags_mask |= DMR_DUP_ROLLBACK;
		/* (schka24) let's hope the DMR_CB is for the right
		** partition...??
		*/
		status = dmf_call(DMR_REPLACE, dmr_cb);
	    }

	    /* Construct new TID for rules processing. */
	    newbigtid.tid_i4.tpf = 0;	/* Clear junk */
	    newbigtid.tid_i4.tid = dmr_cb->dmr_tid;
	    newbigtid.tid.tid_partno = newpartno;

	    if (status != E_DB_OK)
	    {
		if (action->ahd_flags & QEF_STATEMENT_UNIQUE &&
		    (dmr_cb->error.err_code == E_DM0150_DUPLICATE_KEY_STMT ||
		     dmr_cb->error.err_code == E_DM0151_SIDUPLICATE_KEY_STMT))
		{
		    char *a_tuple_ptr = dsh->dsh_row[action->qhd_obj.qhd_qep
					  .u1.s1.ahd_failed_a_temptable->ttb_tuple];
		    MEcopy(output,
			   action->qhd_obj.qhd_qep.ahd_repsize,
			   (PTR) a_tuple_ptr);

		    if (b_row>=0)
			MEcopy((PTR) &newbigtid, sizeof(DB_TID8),
			       (PTR) (a_tuple_ptr 
				      + action->qhd_obj.qhd_qep.ahd_repsize));
		    status = qen_putIntoTempTable(dsh,
						  action->qhd_obj.qhd_qep
						  .u1.s1.ahd_failed_a_temptable);
		    if (status != E_DB_OK)
			return status;
		    if (b_row>=0)
		    {
			char *b_tuple_ptr = dsh->dsh_row[b_row];
			
			MEcopy( ( PTR ) &oldbigtid, ( u_i2 ) sizeof(DB_TID8),
			       (PTR) (b_tuple_ptr 
				      + action->qhd_obj.qhd_qep.ahd_repsize));
			status = qen_putIntoTempTable(dsh,
						      action->qhd_obj.qhd_qep
						      .u1.s1.ahd_failed_b_temptable);
			if (status != E_DB_OK)
			    return (status);
		    }

		    /* Delete the old row unless we did already */
		    if (! old_deleted)
		    {
			status = dmf_call(DMR_DELETE, dmr_cb);
			if (status != E_DB_OK)
			{
			    if (dmr_cb->error.err_code == E_DM0044_DELETED_TID
			     || dmr_cb->error.err_code == E_DM0047_CHANGED_TUPLE)
			    {
				status = E_DB_OK;
			    }
			    else
			    {
				dsh->dsh_error.err_code =
				    dmr_cb->error.err_code;
				return (status);
			    }
			}
		    }
		}
		else if ((dmr_cb->error.err_code == E_DM0046_DUPLICATE_RECORD ||
			  dmr_cb->error.err_code == E_DM9581_REP_SHADOW_DUPLICATE ||
			  dmr_cb->error.err_code == E_DM0045_DUPLICATE_KEY ||
			  dmr_cb->error.err_code == E_DM0048_SIDUPLICATE_KEY) &&
			 action->qhd_obj.qhd_qep.ahd_duphandle == QEF_SKIP_DUP)
		{
		    /* do nothing */
		    status = E_DB_OK;
		}
		else
		{
		    if (dmr_cb->error.err_code == E_DM0045_DUPLICATE_KEY ||
		        dmr_cb->error.err_code == E_DM0048_SIDUPLICATE_KEY)
			dsh->dsh_error.err_code = E_QE004D_DUPLICATE_KEY_UPDATE;
		    else
		    if ( dmr_cb->error.err_code == E_DM0029_ROW_UPDATE_CONFLICT )
		    {
			dsh->dsh_qp_status |= DSH_QP_OBSOLETE;
			dsh->dsh_error.err_code = E_QE0023_INVALID_QUERY;
			status = E_DB_WARN;
		    }
		    else
			dsh->dsh_error.err_code = dmr_cb->error.err_code;
		}
	    
	    }
	    else
	    {
		/* Only increment the rowcount if "SET UPDATE_ROWCOUNT TO
                ** CHANGED" and no error from DMF or if "SET UPDATE_ROWCOUNT TO
                ** QUALIFIED."
                */
                if ((qef_cb->qef_upd_rowcnt == QEF_ROWCHGD &&
                   dmr_cb->error.err_code != E_DM0154_DUPLICATE_ROW_NOTUPD) ||
                   (qef_cb->qef_upd_rowcnt == QEF_ROWQLFD))
		{

		    /* successful replace */
		    i++;

		    /* After a successful update, look for rules to apply */
		    if (action->qhd_obj.qhd_qep.ahd_after_act != NULL)
		    {
			/*
			** Place the TID in a known location so it can be used
			** as a parameter to the procedure fired by the rule.
			*/
			MEcopy ((PTR)&newbigtid, sizeof(DB_TID8),
			    dsh->dsh_row[action->
					qhd_obj.qhd_qep.u1.s1.ahd_a_tid]);   
			MEcopy ((PTR)&oldbigtid, sizeof(DB_TID8),
			    dsh->dsh_row[action->
					qhd_obj.qhd_qep.u1.s1.ahd_b_tid]);   

			dsh->dsh_qef_rowcount = i;
			dsh->dsh_qef_targcount = j;
			dsh->dsh_ctx_act[dsh->dsh_depth_act].dsh_ct_ptr = 
			    action;
			dsh->dsh_ctx_act[dsh->dsh_depth_act].dsh_ct_status =
			    DSH_CT_CONTINUE;
			dsh->dsh_depth_act++;
			dsh->dsh_act_ptr = action->
					qhd_obj.qhd_qep.ahd_after_act;
			dsh->dsh_error.err_code = E_QE0120_RULE_ACTION_LIST;
			status = E_DB_ERROR;
		    }
		}
	    }

	    /* restore the dmr_cb values. */
	    if ((dmr_cb->dmr_flags_mask & DMR_BY_TID) == 0)
	    {
		olddmr_cb->dmr_flags_mask = old_flags_mask;
		olddmr_cb->dmr_tid = oldbigtid.tid_i4.tid;
		olddmr_cb->dmr_data.data_address = old_d_address;
		olddmr_cb->dmr_data.data_in_size = old_d_in_size;
	    }

	    /* If we are now requesting the execution of a rule action list */
	    /* do not update the row count (already done). */
	    if (status != E_DB_OK)
		return (status);
	}
	dsh->dsh_qef_rowcount = i;
	dsh->dsh_qef_targcount = j;
    }
    if ((action->ahd_flags & QEF_STATEMENT_UNIQUE) &&
	! (dsh->dsh_tempTables[action->qhd_obj.qhd_qep
			       .u1.s1.ahd_failed_a_temptable
			       ->ttb_tempTableIndex]->tt_statusFlags & TT_EMPTY))
	status = qea_retryFailedUpdates(action, qef_rcb, dsh, dmr_cb, rowno,
					state);
    return (status);	
}


/*{
** Name: QEA_RETRY_FAILED_UPDATES	- retry for statement level unique
**
** Description:
**	Attemps to reinsert a set of tuples into the table beign updated.
**
** Inputs:
**      action                          update action item
**	qef_rcb
**	dmr_cb				cb for table to update.
**	ade_excb			describes cx for replace.
**	state			    DSH_CT_INITIAL for a replace action
**				    DSH_CT_CONTINUE if being called back after
**				    a rule action list is executed.
**				    DSH_CT_CONTINUE2 if being called back after
**				    a rule action list is executed while
**				    retrying an update for a statement level
**				    unique constraint.
**
** Outputs:
**      qef_rcb
**	    .qef_rowcount		number of replaced tuples
**	    .qef_targcount		number of attempted updates
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
**	12-feb-93 (jhahn)
**          written
**	02-apr-93 (jhahn)
**	    Various fixes for support of statement level unique constraints.
**	02-apr-93 (jhahn)
**	    Set status to E_DB_ERROR for rule execution.
**	11-may-93 (anitap)
**	    Fixed compiler warning.
**	17-may-93 (jhahn)
**	    More fixes for support of statement level unique constraints.
**	5-feb-04 (inkdo01)
**	    Add support to update partitioned tables.
**	19-mar-04 (inkdo01)
**	    More bigtid stuff - for rules processing.
**	12-may-04 (inkdo01)
**	    Call qeq_part_open unconditionally - it'll determine whether
**	    partition cbs are already prepared.
**	8-Jul-2004 (schka24)
**	    Revise tid8 handling, make sure we don't introduce junk into
**	    tid partno if non-partitioned.
**	8-Sep-2008 (kibro01) b120693
**	    Remove unused action parameter from qeq_part_open
[@history_template@]...
*/
static DB_STATUS
qea_retryFailedUpdates(
	QEF_AHD		*action,
	QEF_RCB		*qef_rcb,
	QEE_DSH		*dsh,
	DMR_CB		*dmr_cb,
	i4		rowno,
	i4 		state)
{
    i4	i;
    DMR_CB	*pdmr_cb, *olddmr_cb = dmr_cb;
    char	*output = dsh->dsh_row[rowno];
    DB_STATUS	status	= E_DB_OK;
    i4	old_flags_mask, pold_flags_mask;
    char	*old_d_address, *pold_d_address;
    i4	old_d_in_size, pold_d_in_size;
    char	*a_tuple_ptr = dsh->dsh_row[action->qhd_obj.qhd_qep.
			       u1.s1.ahd_failed_a_temptable->ttb_tuple];
    i4		b_row	= action->qhd_obj.qhd_qep.u1.s1.ahd_b_row;
    DB_TID8	newbigtid;
    u_i2	newpartno = 0;

    old_flags_mask = dmr_cb->dmr_flags_mask;
    old_d_address = dmr_cb->dmr_data.data_address;
    old_d_in_size = dmr_cb->dmr_data.data_in_size;

    dmr_cb->dmr_flags_mask	    = 0;
    dmr_cb->dmr_tid		    = 0;                    /* not used */
    dmr_cb->dmr_data.data_address   = output;
    dmr_cb->dmr_data.data_in_size   = dsh->dsh_qp_ptr->qp_row_len[rowno];
    dmr_cb->dmr_val_logkey          = 0;

    if (state != DSH_CT_CONTINUE2)
    {
	status = qen_rewindTempTable(dsh,
				     action->qhd_obj.qhd_qep
				     .u1.s1.ahd_failed_a_temptable
				     ->ttb_tempTableIndex);
	if (status != E_DB_OK)
	    return(status);
	if (b_row >= 0)
	{
	    status = qen_rewindTempTable(dsh,
					 action->qhd_obj.qhd_qep
					 .u1.s1.ahd_failed_b_temptable
					 ->ttb_tempTableIndex);
	    if (status != E_DB_OK)
		return(status);
	}
    }
    i = 0;
    for (;;)
    {
	status = qen_getFromTempTable(dsh,
				      action->qhd_obj.qhd_qep
				      .u1.s1.ahd_failed_a_temptable
				      ->ttb_tempTableIndex);
	if (status != E_DB_OK)
	{
	    if (dsh->dsh_error.err_code == E_QE0015_NO_MORE_ROWS)
	    {
		status = qen_destroyTempTable(dsh,
					      action->qhd_obj.qhd_qep
					      .u1.s1.ahd_failed_a_temptable
					      ->ttb_tempTableIndex);
		if (status != E_DB_OK)
		    break;
		if (b_row >= 0)
		    status = qen_destroyTempTable(dsh,
						  action->qhd_obj.qhd_qep
						  .u1.s1.ahd_failed_b_temptable
						  ->ttb_tempTableIndex);
	    }
	    break;
	}
	MEcopy((PTR) a_tuple_ptr,
	       action->qhd_obj.qhd_qep.ahd_repsize, (PTR) output);

	/* If partitioned table, determine target partition for new row. */
	if (action->qhd_obj.qhd_qep.ahd_part_def)
	{
	    status = adt_whichpart_no(dsh->dsh_adf_cb,
			action->qhd_obj.qhd_qep.ahd_part_def, 
			output, &newpartno);
	    if (status != E_DB_OK)
		return(status);

	    /* Get output partition DMR_CB, then do DMR_PUT. */
	    status = qeq_part_open(qef_rcb, dsh, NULL, 0, 
		action->qhd_obj.qhd_qep.ahd_odmr_cb,
		action->qhd_obj.qhd_qep.ahd_dmtix, newpartno);
			
	    if (status != E_DB_OK)
		return(status);
	    pdmr_cb = (DMR_CB *)dsh->dsh_cbs[action->
			qhd_obj.qhd_qep.ahd_odmr_cb+newpartno+1];

	    /* Final preparation of new partition DMR_CB. */
	    pold_flags_mask = pdmr_cb->dmr_flags_mask;
	    pold_d_address = pdmr_cb->dmr_data.data_address;
	    pold_d_in_size = pdmr_cb->dmr_data.data_in_size;
	    dmr_cb = pdmr_cb;
	    dmr_cb->dmr_flags_mask = 0;
	    dmr_cb->dmr_tid = 0;
	    dmr_cb->dmr_val_logkey = 0;
	    dmr_cb->dmr_data.data_address = output;
	    dmr_cb->dmr_data.data_in_size  =
				action->qhd_obj.qhd_qep.ahd_repsize;
	}   
	status = dmf_call(DMR_PUT, dmr_cb);

	if (action->qhd_obj.qhd_qep.ahd_part_def)
	{
	    /* Restore DMR_CB settings of table partition. */
	    pdmr_cb->dmr_flags_mask = pold_flags_mask;
	    pdmr_cb->dmr_data.data_address = pold_d_address;
	    pdmr_cb->dmr_data.data_in_size = pold_d_in_size;
	}

	if (status != E_DB_OK)
	{
	    if (dmr_cb->error.err_code == E_DM0045_DUPLICATE_KEY ||
		dmr_cb->error.err_code == E_DM0048_SIDUPLICATE_KEY ||
		dmr_cb->error.err_code == E_DM0150_DUPLICATE_KEY_STMT ||
		dmr_cb->error.err_code == E_DM0151_SIDUPLICATE_KEY_STMT)
		dsh->dsh_error.err_code = E_QE004D_DUPLICATE_KEY_UPDATE;
	    else
		dsh->dsh_error.err_code = dmr_cb->error.err_code;
	    break;
	}
	i++;
	if (action->qhd_obj.qhd_qep.ahd_after_act != NULL)
	{

	    /*
	    ** Place the TID in a known location so it can be used
	    ** as a parameter to the procedure fired by the rule.
	    */
	    newbigtid.tid_i4.tpf = 0;	/* No junk */
	    newbigtid.tid_i4.tid = dmr_cb->dmr_tid;
	    newbigtid.tid.tid_partno = newpartno;
	    MEcopy ((PTR)&newbigtid, sizeof(DB_TID8),
		    dsh->dsh_row[action->qhd_obj.qhd_qep.u1.s1.ahd_a_tid]);   
	    if (b_row >= 0)
	    {
		MEcopy((PTR) (a_tuple_ptr +
			      action->qhd_obj.qhd_qep.ahd_repsize),
		       sizeof(DB_TID8),
		       dsh->dsh_row[action->qhd_obj.qhd_qep.u1.s1.ahd_b_tid]);
		status = qen_getFromTempTable(dsh,
					      action->qhd_obj.qhd_qep
					      .u1.s1.ahd_failed_b_temptable
					      ->ttb_tempTableIndex);
		if (status != E_DB_OK)
		    break;
		if (MEcmp(dsh->dsh_row[b_row] +
			  action->qhd_obj.qhd_qep.ahd_repsize,
			  a_tuple_ptr + action->qhd_obj.qhd_qep.ahd_repsize,
			  sizeof(DB_TID8)) != 0)
		{
		    dsh->dsh_error.err_code = E_QE001D_FAIL; /*fixme*/
		    status = E_DB_ERROR;
		    break;
		}
	    }
	    dsh->dsh_ctx_act[dsh->dsh_depth_act].dsh_ct_ptr = action;
	    dsh->dsh_ctx_act[dsh->dsh_depth_act].dsh_ct_status =
		DSH_CT_CONTINUE2;
	    dsh->dsh_depth_act++;
	    dsh->dsh_act_ptr = action->qhd_obj.qhd_qep.ahd_after_act;
	    dsh->dsh_error.err_code = E_QE0120_RULE_ACTION_LIST;
	    status = E_DB_ERROR;
	    break;
	}

    }
    olddmr_cb->dmr_flags_mask = old_flags_mask;
    olddmr_cb->dmr_data.data_address = old_d_address;
    olddmr_cb->dmr_data.data_in_size = old_d_in_size;
    dsh->dsh_qef_rowcount += i;
    return(status);
}
