/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <me.h>
#include    <st.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <ulf.h>
#include    <adf.h>
#include    <ade.h>
#include    <adp.h>
#include    <cs.h>
#include    <scf.h>
#include    <dmf.h>
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

#include    <dmmcb.h>
#include    <dmucb.h>
#include    <ex.h>
#include    <tm.h>

#include    <dudbms.h>
#include    <ulm.h>
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
**  Name:   QEARUPD.C		- Update cursor support
**
**  Description:
**	Routines supporting cursor update execution
**
**	    qea_rupdate		- update the current row for the cursor
**
**  History:
**	17-apr-89 (paul)
**	    Moved from qeq.c and integrated into qeq_query action processing
**	22-sep-89 (paul)
**	    Added support for TIDs as parameters to procedures called from 
**	    rules.
**	29-jan-90 (nancy) -- fix bug 8388 (8575) to allow cursor replace by
**	    tid.  Also set status to E_DB_OK if dmf returns duplicate and
**	    duplicate handling is set to skip dups.  This was breaking quel
**	    CTS cursor tests.
**	29-jun-90 (davebf)
**	    Handle rows which were accessed by TID only
**	15-jun-92 (davebf)
**	    Sybil Merge.
**	08-dec-92 (anitap)
**	    Included <qsf.h> and <psfparse.h> for CREATE SCHEMA.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	9-jul-93 (rickh)
**	    Made some ME calls agree with the prototypes.
**	7-nov-95 (inkdo01)
**	    Changes to replace QEN_ADF structure instances by pointers in
**	    QEF_AHD structures.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**      05-may-1999 (nanpr01,stial01)
**          Set DMR_DUP_ROLLBACK when duplicate error will result in rollback
**          This allows DMF to dupcheck in indices after base table is updated.
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	24-jan-01 (inkdo01)
**	    Changes to QEF_QEP structure putting some fields in union 
**	    (for hash aggregation project).
**	23-feb-04 (inkdo01)
**	    Changed qef_rcb->error to dsh->dsh_error for || thread safety
**	    (and others).
**	9-Dec-2005 (kschendel)
**	    Kill obsolete/wrong i2 casts on mecopy/fill.
**	28-Dec-2005 (kschendel)
**	    Use DSH references whenever possible instead of QEF RCB
**	    references.  (This simplifies the code, eliminating many
**	    RCB references, and reflects the new primacy of the DSH
**	    thanks to the parallel query changes.)
**/

/*
** {
** Name: QEA_RUPDATE		- update the current cursor
**
**  External QEF call:	    status = qef_call(QEQ_REPLACE, &qef_rcb);
**
** Description:
**      The current row in the named cursor (QP) is
**  updated and sent to DMF 
**
** Inputs:
**	action			Update current row of cursor action.
**      qef_rcb
**	assoc_dsh		DSH for QP for cursor to be updated.
**	reset
**	state			DSH_CT_INITIAL if this is an action call
**				DSH_CT_CONTINUE for call back after processing
**				a rule action list.
**
** Outputs:
**      qef_rcb
**	    .qef_remnull	SET if an aggregate computation found a NULL val
**	    .qef_rowcount	number of rows replaced
**	    .qef_targcount	number of attempted replaces 
**	    .error.err_code	one of the following
**				E_QE0000_OK
**				E_QE0017_BAD_CB
**				E_QE0018_BAD_PARAM_IN_CB
**				E_QE0019_NON_INTERNAL_FAIL
**				E_QE0002_INTERNAL_ERROR
**				E_QE0008_CURSOR_NOT_OPENED
**				E_QE0021_NO_ROW
**				E_QE0009_NO_PERMISSION
**				E_QE000A_READ_ONLY
**				E_QE0012_DUPLICATE_KEY
**				E_QE0010_DUPLIATE_ROW
**				E_QE0011_AMBIGUOUS_REPLACE
**				E_QE0013_INTEGRITY_FAILED
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
**	    catch cursor not positioned errors
**	14-oct-87 (puree)
**	    make sure that qen_error is called in case of E_DB_ERROR from
**	    qeq_validate.
**	02-feb-88 (puree)
**	    added reset flag to qeq_validate and all qea_xxxx call sequence.
**	29-aug-88 (puree)
**	    implement replace cursor by tid if the cursor was position on
**	    a secondary index table and by current position if the cursor
**	    is positioned on the base table.
**	    fix duplicate handling error.
**	12-oct-88 (puree)
**	    If a table validation fails during cursor replace, abort the
**	    cursor.
**	23-feb-89 (paul)
**	    Clean up qef_cb state before returning.
**	17-apr-89 (paul)
**	    Processing cursor updates integrated into qeq_query. Update cursor
**	    logic moved from qeq.c to this file (qearupd.c). This routine is
**	    a simplified version of what was previously called qeq_replace.
**	25-sep-89 (paul)
**	    Added support for using TIDs as parameters to procedures invoked
**	    from rules.
**	25-jan-90 (nancy) -- set status to E_DB_OK when replace cursor
**	    causes dups and duplicate handling is set to SKIP_DUPS.
**	29-jan-90 (nancy) -- fix bug 8388 (8575), use assoc dsh for replace
**	    by tid.
**	29-jun-90 (davebf)
**	    Handle rows which were accessed by TID only  (bug 31113)
**       5-Nov-1993 (fred)
**          Remove any remaining large object temporaries.
**       7-Apr-1994 (fred)
**          Commented out large object removal pending better way...
**	7-nov-95 (inkdo01)
**	    Changes to replace QEN_ADF structure instances by pointers in
**	    QEF_AHD structures.
**	29-dec-03 (inkdo01)
**	    DSH is now parameter, "function" replaces "reset".
**	5-feb-04 (inkdo01)
**	    Add support for partitioned tables.
**	12-mar-04 (inkdo01)
**	    Fix to handle bigtids on byte-swapped machines.
**	19-mar-04 (inkdo01)
**	    Fix for bigtids and rules.
**	12-may-04 (inkdo01)
**	    Call qeq_part_open unconditionally - it'll determine whether
**	    partition cbs are already prepared.
**	8-Jul-2004 (schka24)
**	    Still some tid byte-ordering confusion on x86, make tid handling
**	    more portable.
**	26-aug-04 (inkdo01)
**	    Add global base array support for update buffer.
**	24-Feb-05 (hweho01)
**	    The copying of tid needs to be handled differently if   
**          TID_SWAP is defined, so the tid value can be preserved. 
**          Star #13864021.
**	28-Feb-2005 (schka24)
**	    Rework the above fix, it turns out that the underlying problem
**	    was the 4byte-tidp flag not getting set.  Now that it's set
**	    properly, obey it here.  Also, do partition opens against the
**	    cursor query, not the RUP -- it's the one with the valid list..
**	18-Jul-2005 (schka24)
**	    Well, I fixed by-tid updates, but not true in-place updates!
**	    Use new ahd_ruporig machinery to get at the current partition
**	    number for in-place updating.
**	13-Dec-2005 (kschendel)
**	    Can count on qen-ade-cx now.
**	16-Jan-2006 (kschendel)
**	    Access qen-status thru xaddrs.
**	20-june-06 (dougi)
**	    Add support for BEFORE triggers.
**	1-Nov-2006 (kschendel)
**	    Some fixes to BEFORE triggers, make sure things are set up.
**	7-mar-2007 (dougi)
**	    Slight change to address the DMR_CB (broken by BEFORE changes).
**	8-Sep-2008 (kibro01) b120693
**	    Remove unused action parameter from qeq_part_open
**	09-Sep-2009 (thaju02) B122374
**	    If BEFORE trigger has been executed, do not use ahd_upd_colmap.
**	    ahd_upd_colmap may not reflect cols updated by the rule/proc.
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: If E_DM0029_ROW_UPDATE_CONFLICT returned,
**	    invalidate the QP and retry.
**	1-Jul-2010 (kschendel) b124004
**	    Minor changes for RUP from scrollable keyset cursors;  the
**	    fetched row is always in dsh-qef-output.
*/
DB_STATUS
qea_rupdate(
QEF_AHD		    *act,
QEF_RCB		    *qef_rcb,
QEE_DSH		    *dsh,
QEE_DSH		    *assoc_dsh,
i4		    function,
i4		    state )
{
    i4		err;
    DB_STATUS		status	= E_DB_OK;
    QEF_CB		*qef_cb = dsh->dsh_qefcb;
    ADF_CB		*adfcb = dsh->dsh_adf_cb;
    DMR_CB		*dmr_cb, *pdmr_cb;
    QEN_ADF		*qen_adf;
    ADE_EXCB		*ade_excb;
    PTR			*assoc_cbs = assoc_dsh->dsh_cbs;
    PTR			output, save_addr;
    char		*tidinput;  /* location of tid */
    DB_TID8		oldbigtid, newbigtid;
    u_i2		oldpartno = 0;
    u_i2		newpartno = 0;
    i4			odmr_cb_ix;	/* CB number of DMR_CB */
    i4			orig_node;
    
    status = E_DB_OK;
    
    for (;;)
    {
	/* A cursor update affects only a single row. If we are called back  */
	/* after rules processing simply return. */
	if (state == DSH_CT_CONTINUE)
	    break;

	/* Address the DMR_CB used to access the row.  The odmr_cb_ix is in
	** the associated (cursor) QP context.  We'll adjust for partitioning
	** a bit later.
	*/
	odmr_cb_ix = act->qhd_obj.qhd_qep.ahd_odmr_cb;
	dmr_cb = (DMR_CB*) assoc_cbs[odmr_cb_ix];
	if (status == DSH_CT_CONTINUE3)
	{
	    /* Resuming from a BEFORE trigger, reset variables */
	    MEcopy(dsh->dsh_row[act->qhd_obj.qhd_qep.u1.s1.ahd_a_tid],
			sizeof(DB_TID8), (PTR) &newbigtid);
	    MEcopy(dsh->dsh_row[act->qhd_obj.qhd_qep.u1.s1.ahd_b_tid],
			sizeof(DB_TID8), (PTR) &oldbigtid);
	    oldpartno = oldbigtid.tid.tid_partno;
	    newpartno = newbigtid.tid.tid_partno;
	    output = dsh->dsh_row[act->qhd_obj.qhd_qep.u1.s1.ahd_a_row];
	    if (act->qhd_obj.qhd_qep.ahd_part_def != NULL)
		dmr_cb = (DMR_CB *) assoc_cbs[odmr_cb_ix + oldpartno + 1];
	}
	else
	{
	    /* Normal path */
	    dsh->dsh_qef_remnull = 0;
	    dsh->dsh_qef_rowcount = 0;

	    /* Use fetch row for holding update row if need be */
	    output = assoc_dsh->dsh_row[assoc_dsh->dsh_qp_ptr->qp_fetch_ahd->qhd_obj.qhd_qep.ahd_ruprow];

	    /* make sure we have a positioned record */
	    if (assoc_dsh->dsh_positioned == FALSE)
	    {
		/* no record could be valid in this action's updatable control block.
		** Nor do we know what we will be looking at when we access the
		** update control block index number
		*/
		dsh->dsh_error.err_code = E_QE0021_NO_ROW;
		status = E_DB_ERROR;
		break;
	    }

	    /* check that this query plan has update privilege */
	    if ((assoc_dsh->dsh_qp_ptr->qp_status & QEQP_UPDATE) == 0)
	    {
		/* no permission */
		dsh->dsh_error.err_code = E_QE0009_NO_PERMISSION;
		status = E_DB_ERROR;
		break;
    	    }

	    if (qef_cb->qef_c1_distrib & DB_3_DDB_SESS)	/* distributed? */
	    {
		/* DDB processing */

		status = qeq_c5_replace(qef_rcb, dsh);
		if (status == E_DB_OK)
		    dsh->dsh_qef_rowcount = 1;
		else
		    dsh->dsh_qef_rowcount = 0;
		return(status);
	    }

	    /* LDB processing */

	    /* process the qualification expression */
	    qen_adf     = act->qhd_obj.qhd_qep.ahd_constant;
	    if (qen_adf != NULL)
	    {
		ade_excb = (ADE_EXCB*) dsh->dsh_cbs[qen_adf->qen_pos];
		if (dsh->dsh_qp_ptr->qp_status & QEQP_GLOBAL_BASEARRAY)
		    dsh->dsh_row[qen_adf->qen_uoutput] = output;
		else ade_excb->excb_bases[ADE_ZBASE + qen_adf->qen_uoutput] = output;
		status = qen_execute_cx(dsh, ade_excb);
		if (status != E_DB_OK)
		    break;

		/* handle condition where qualification failed. */
		if (ade_excb->excb_value != ADE_TRUE)
		{
		    dsh->dsh_qef_targcount = 1;
		    dsh->dsh_qef_rowcount = 0;
		    break;
		}
	    }

	    /* REPLACE THE TUPLE
	    ** if the cursor is positioned on the base table (ahd_tidoffset is
	    ** -1), replace the tuple at the current position.  Otherwise the
	    ** cursor must have been positioned via a secondary index.  Get
	    ** the tid, re-fetch the original row by tid, and replace the
	    ** base table tuple by the tid.
	    */

	    if (act->qhd_obj.qhd_qep.ahd_tidoffset == -1)
	    {
		/* If partitioned, dig the current partition number out of
		** the node status for the orig (or kjoin/tjoin) node that
		** fetched the row in the first place.  OPC has kindly put
		** the node number of same into ahd_ruporig.
		*/
		if (act->qhd_obj.qhd_qep.ahd_part_def != NULL)
		{
		    orig_node = act->qhd_obj.qhd_qep.ahd_ruporig;
		    if (orig_node == -1)
		    {
			TRdisplay("%@ qea_rupd: missing ahd_ruporig\n");
			dsh->dsh_error.err_code = E_QE0002_INTERNAL_ERROR;
			status = E_DB_ERROR;
			break;
		    }
		    oldpartno = assoc_dsh->dsh_xaddrs[orig_node]->
						qex_status->node_ppart_num;
		    dmr_cb = (DMR_CB *) assoc_cbs[odmr_cb_ix + oldpartno + 1];
		}
		dmr_cb->dmr_flags_mask = DMR_CURRENT_POS;
	    }
	    else
	    {
		/* set the tid */
		tidinput  = 
		    (char *) assoc_dsh->dsh_row[act->qhd_obj.qhd_qep.ahd_tidrow]
				 + act->qhd_obj.qhd_qep.ahd_tidoffset;

		if (act->qhd_obj.qhd_qep.ahd_qepflag & AHD_4BYTE_TIDP)
		{
		    I4ASSIGN_MACRO(*tidinput, oldbigtid.tid_i4.tid );
		    oldbigtid.tid_i4.tpf = 0;
		}
		else
		{
		    I8ASSIGN_MACRO(*tidinput, oldbigtid);
		}
		oldpartno = oldbigtid.tid.tid_partno;
		newpartno = oldpartno;
	
		/* Check for partitioning and set up DMR_CB. */
		if (act->qhd_obj.qhd_qep.ahd_part_def)
		{
		    /* Open against "get" action.  The RUP action dmtix and
		    ** flags are the same as the cursor's, thanks to OPC.
		    */
		    status = qeq_part_open(qef_rcb, assoc_dsh, NULL, 0, 
			odmr_cb_ix,
			act->qhd_obj.qhd_qep.ahd_dmtix, oldpartno);
			
		    if (status != E_DB_OK)
		    {
			dsh->dsh_error.err_code = assoc_dsh->dsh_error.err_code;
			return(status);
		    }
		    pdmr_cb = (DMR_CB *)assoc_cbs[odmr_cb_ix + oldpartno+1];

		    STRUCT_ASSIGN_MACRO(dmr_cb->dmr_data, pdmr_cb->dmr_data);
		    dmr_cb = pdmr_cb;
		}
		dmr_cb->dmr_tid = oldbigtid.tid_i4.tid;
		dmr_cb->dmr_flags_mask = DMR_BY_TID;
	
		/* get the tuple to be updated. */
		save_addr = dmr_cb->dmr_data.data_address;
		dmr_cb->dmr_data.data_address = output;
		status = dmf_call(DMR_GET, dmr_cb);
		dmr_cb->dmr_data.data_address = save_addr;
		if (status != E_DB_OK)
		{
		    dsh->dsh_error.err_code = dmr_cb->error.err_code;
		    return (status);
		}
	    }

	    /* If we have rules, save a copy of the row before updating */
	    if (act->qhd_obj.qhd_qep.ahd_after_act != NULL ||
		act->qhd_obj.qhd_qep.ahd_before_act != NULL)
	    {
		MEcopy(output, act->qhd_obj.qhd_qep.ahd_repsize,
		    dsh->dsh_row[act->qhd_obj.qhd_qep.u1.s1.ahd_b_row]);
		oldbigtid.tid_i4.tpf = 0;
		oldbigtid.tid_i4.tid = dmr_cb->dmr_tid;
		oldbigtid.tid.tid_partno = oldpartno;
	    }

	    /* process the update expression */
	    qen_adf     = act->qhd_obj.qhd_qep.ahd_current;
	    ade_excb    = (ADE_EXCB*) dsh->dsh_cbs[qen_adf->qen_pos];	
	    if (dsh->dsh_qp_ptr->qp_status & QEQP_GLOBAL_BASEARRAY)
		dsh->dsh_row[qen_adf->qen_uoutput] = output;
	    else ade_excb->excb_bases[ADE_ZBASE+qen_adf->qen_uoutput] = output;

	    status = qen_execute_cx(dsh, ade_excb);
	    if (status != E_DB_OK)
		break;

	    /* Check for partitioning and see if we changed target partition. */
	    if (act->qhd_obj.qhd_qep.ahd_part_def && 
		(act->qhd_obj.qhd_qep.ahd_qepflag & AHD_PCOLS_UPDATE))
	    {
		status = adt_whichpart_no(adfcb,
			act->qhd_obj.qhd_qep.ahd_part_def, 
			output, &newpartno);
		if (status != E_DB_OK)
		    return(status);
	    }
	    /* Process BEFORE triggers (if any). */
	    if (act->qhd_obj.qhd_qep.ahd_before_act != NULL)
	    {
		/*
		** Place the TID in a known location so it can be used
		** as a parameter to the procedure fired by the rule.
		*/
		newbigtid.tid_i4.tpf = 0;
		newbigtid.tid_i4.tid = dmr_cb->dmr_tid;
		newbigtid.tid.tid_partno = newpartno;
		MEcopy ((PTR)&newbigtid, sizeof(DB_TID8),
			dsh->dsh_row[act->qhd_obj.qhd_qep.u1.s1.ahd_a_tid]);   
		MEcopy ((PTR)&oldbigtid, sizeof(DB_TID8),
			dsh->dsh_row[act->qhd_obj.qhd_qep.u1.s1.ahd_b_tid]);   

		MEcopy(output, act->qhd_obj.qhd_qep.ahd_repsize,
			dsh->dsh_row[act->qhd_obj.qhd_qep.u1.s1.ahd_a_row]);
		dsh->dsh_ctx_act[dsh->dsh_depth_act].dsh_ct_ptr = act;
		dsh->dsh_ctx_act[dsh->dsh_depth_act].dsh_ct_status =
			DSH_CT_CONTINUE3;
		dsh->dsh_depth_act++;
		dsh->dsh_act_ptr = act->qhd_obj.qhd_qep.ahd_before_act;
		dsh->dsh_error.err_code = E_QE0120_RULE_ACTION_LIST;
		status = E_DB_ERROR;
		break;
	    }
	}    /* end of !DSH_CT_CONTINUE3 */

	state = DSH_CT_INITIAL;			/* signal resumption after
						** BEFORE trigger */

	if (act->qhd_obj.qhd_qep.ahd_qepflag & AHD_PCOLS_UPDATE &&
		oldpartno != newpartno)
	{
	    /* Partition has changed for row in partitioned table.
	    ** Delete from 1st partition and insert into next.
	    ** We have to hope that DMF can keep it all straight.
	    */
	    status = dmf_call(DMR_DELETE, dmr_cb);
	    if (status != E_DB_OK)
	    {
		if (dmr_cb->error.err_code == E_DM0055_NONEXT)
		    dsh->dsh_error.err_code = 4599;
		else
		    dsh->dsh_error.err_code = dmr_cb->error.err_code;
		break;
	    }

	    /* Get output partition DMR_CB, then do DMR_PUT.
	    ** Don't change the fetching DSH's current dmr-cb in case
	    ** we're running a true in-place cursor.
	    */
	    status = qeq_part_open(qef_rcb, assoc_dsh, NULL, 0, 
		odmr_cb_ix,
		act->qhd_obj.qhd_qep.ahd_dmtix, newpartno);
			
	    if (status != E_DB_OK)
	    {
		dsh->dsh_error.err_code = assoc_dsh->dsh_error.err_code;
		return(status);
	    }
	    dmr_cb = (DMR_CB *)assoc_cbs[odmr_cb_ix + newpartno+1];

	    dsh->dsh_qef_targcount = 1;
	    /* Final preparation of new partition DMR_CB. */
	    dmr_cb->dmr_flags_mask = 0;
	    dmr_cb->dmr_val_logkey = 0;
	    dmr_cb->dmr_data.data_address = output;
	    dmr_cb->dmr_data.data_in_size  =
				act->qhd_obj.qhd_qep.ahd_repsize;
	    if (act->qhd_obj.qhd_qep.ahd_duphandle != QEF_SKIP_DUP)
		dmr_cb->dmr_flags_mask |= DMR_DUP_ROLLBACK;
	    status = dmf_call(DMR_PUT, dmr_cb);

	}
	else
	{
	    dsh->dsh_qef_targcount = 1;
	    if ((act->qhd_obj.qhd_qep.ahd_upd_colmap) && 
		(act->qhd_obj.qhd_qep.ahd_before_act == NULL))
		dmr_cb->dmr_attset = 
			(char *)act->qhd_obj.qhd_qep.ahd_upd_colmap;
	    else
		dmr_cb->dmr_attset = (char *)0;

	    if (act->qhd_obj.qhd_qep.ahd_duphandle != QEF_SKIP_DUP)
		dmr_cb->dmr_flags_mask |= DMR_DUP_ROLLBACK;
	    status = dmf_call(DMR_REPLACE, dmr_cb);
	}
	    
	if (status == E_DB_OK || 
		((dmr_cb->error.err_code == E_DM0046_DUPLICATE_RECORD ||
		  dmr_cb->error.err_code == E_DM0045_DUPLICATE_KEY    ||
		  dmr_cb->error.err_code == E_DM0048_SIDUPLICATE_KEY) &&
		 act->qhd_obj.qhd_qep.ahd_duphandle == QEF_SKIP_DUP))
	{
	    if (status == E_DB_OK)
	    {
	    	dsh->dsh_qef_rowcount = 1;
		/* Look for rules to apply */
		if (act->qhd_obj.qhd_qep.ahd_after_act != NULL)
		{
		    /*
		    ** Place the TID in a known location so it can be used
		    ** as a parameter to the procedure fired by the rule.
		    */
		    newbigtid.tid_i4.tpf = 0;
		    newbigtid.tid_i4.tid = dmr_cb->dmr_tid;
		    newbigtid.tid.tid_partno = newpartno;
		    MEcopy ((PTR)&newbigtid, sizeof(DB_TID8),
			dsh->dsh_row[act->qhd_obj.qhd_qep.u1.s1.ahd_a_tid]);   
		    MEcopy ((PTR)&oldbigtid, sizeof(DB_TID8),
			dsh->dsh_row[act->qhd_obj.qhd_qep.u1.s1.ahd_b_tid]);   

		    MEcopy(output, act->qhd_obj.qhd_qep.ahd_repsize,
			dsh->dsh_row[act->qhd_obj.qhd_qep.u1.s1.ahd_a_row]);
		    dsh->dsh_ctx_act[dsh->dsh_depth_act].dsh_ct_ptr = act;
		    dsh->dsh_ctx_act[dsh->dsh_depth_act].dsh_ct_status =
			DSH_CT_CONTINUE;
		    dsh->dsh_depth_act++;
		    dsh->dsh_act_ptr = act->qhd_obj.qhd_qep.ahd_after_act;
		    dsh->dsh_error.err_code = E_QE0120_RULE_ACTION_LIST;
		    status = E_DB_ERROR;
		    break;
		}
	    }
	    else
	    {
		dsh->dsh_qef_rowcount = 0;
		status = E_DB_OK;
	    }	
	    break;	    
	}
	
	if (dmr_cb->error.err_code == E_DM0055_NONEXT)
	    dsh->dsh_error.err_code = 4599;
	else
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

    return (status);
}
