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
**  Name:   QEARDEL.C		- Delete cursor support
**
**  Description:
**	Routines supporting cursor delete execution
**
**	    qea_rdelete		- delete the current row for the cursor
**
**  History:
**	17-apr-89 (paul)
**	    Moved from qeq.c and integrated into qeq_query action processing
**	22-sep-89 (paul)
**	    Added support to use TIDs as parameters to procedures called
**	    from rules.
**	29-jun-90 (davebf)
**	    Handle rows which were accessed by TID only
**	15-jun-92 (davebf)
**	    Sybil Merge
**      08-dec-92 (anitap)
**          Included <qsf.h> and <psfparse.h> for CREATE SCHEMA.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**       5-Nov-1993 (fred)
**          Add support for removing large object temporaries...
**	13-jan-94 (rickh)
**	    Added some casts to some MEcopy calls.
**       7-Apr-1994 (fred)
**          Commented out large object removal pending a better way...
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
**	28-Dec-2005 (kschendel)
**	    Use DSH references whenever possible instead of QEF RCB
**	    references.  (This simplifies the code, eliminating many
**	    RCB references, and reflects the new primacy of the DSH
**	    thanks to the parallel query changes.)
**/

/*{
** Name: QEA_RDELETE		- delete the current cursor
**
**  External QEF call:	    status = qef_call(QEQ_DELETE, &qef_rcb);
**
** Description:
**      The current row in the named cursor (QP) is
**  deleted and sent to DMF 
**
** Inputs:
**	action			Delete current row of cursor action.
**      qef_rcb
**	assoc_dsh		DSH for QP for cursor to be deleted.
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
**	17-apr-89 (paul)
**	    Processing cursor deletes integrated into qeq_query. Delete cursor
**	    logic moved from qeq.c to this file (qearupd.c). This routine is
**	    a simplified version of what was previously called qeq_delete.
**	22-sep-89 (paul)
**	    Added support for TIDs as parameters to procedures called from rules.
**	29-jun-90 (davebf)
**	    Handle rows which were accessed by TID only  (bug 31113)
**	11-nov-92 (barbara)
**	    Use open cursor's dsh to check for position.  Copy table and
**	    id from open cursor's dsh to delete's dsh.
**	01-may-93 (barbara)
**	    For delete cursor, owner is now in a separate field (qeq_d9_delown)
**	    from tablename.  Copy this to dsh.
**       7-Apr-1994 (fred)
**          Commented out large object removal pending a better way...
**	29-dec-03 (inkdo01)
**	    DSH is now parameter, "function" replaces "reset".
**	5-feb-04 (inkdo01)
**	    Add support to RDEL from partitioned tables.
**	12-mar-04 (inkdo01)
**	    Fix to handle bigtids on byte-swapped machines.
**	19-mar-04 (inkdo01)
**	    Bigtids and rule processing.
**	12-may-04 (inkdo01)
**	    Call qeq_part_open unconditionally - it'll determine whether
**	    partition cbs are already prepared.
**	8-Jul-2004 (schka24)
**	    Still some tid byte-ordering confusion on x86, make tid handling
**	    more portable.
**	28-Feb-2005 (schka24)
**	    Obey the 4-byte-tidp flag; open partitions against the cursor,
**	    not the RDEL action.
**	28-Dec-2005 (kschendel)
**	    Make sure we pass back the error code if a partition open fails.
**	20-june-06 (dougi)
**	    Add support for BEFORE triggers (not that they're much use for
**	    DELETE).
**	1-Nov-2006 (kschendel)
**	    Make sure we have the right dmr cb pointer after a BEFORE rule.
**	7-mar-2007 (dougi)
**	    Slight change to address the DMR_CB (broken by BEFORE changes).
**	8-Sep-2008 (kibro01) b120693
**	    Remove unused action parameter from qeq_part_open
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: If E_DM0029_ROW_UPDATE_CONFLICT returned,
**	    invalidate the QP and retry.
**	2-Jul-2010 (kschendel) b124004
**	    Fix before-rule if in-place delete, wasn't setting partition into
**	    the TID.  (Not sure if OPC will compile things in-place if there
**	    is a before rule, but let's get it right just in case.)
*/
DB_STATUS
qea_rdelete(
QEF_AHD		    *act,
QEF_RCB		    *qef_rcb,
QEE_DSH		    *dsh,
QEE_DSH		    *assoc_dsh,
i4		    function,
i4		    state )
{
    i4		err;
    DB_STATUS		status	= E_DB_OK;
    DMR_CB		*dmr_cb, *pdmr_cb;
    PTR			*assoc_cbs = assoc_dsh->dsh_cbs;
    PTR			output, save_addr;
    char		*tidinput;  /* location of tid */
    DB_TID8		bigtid;
    i4			odmr_cb_ix;
    u_i2		partno = 0;

    if (dsh->dsh_qefcb->qef_c1_distrib & DB_3_DDB_SESS)    /* distributed? */
    {
	/* make sure we have a positioned record */
	if (assoc_dsh->dsh_positioned == FALSE)
	{
	    qef_error(E_QE0021_NO_ROW, 0L, E_DB_ERROR, &err,
			  &dsh->dsh_error, 0);
	    return (E_DB_WARN);
	}

	/* the delete call will unposition us */
	assoc_dsh->dsh_positioned = FALSE;

	/* get tablename and cursor id from assoc dsh */

	dsh->dsh_qp_ptr->qp_ddq_cb.qeq_d7_deltable
		= assoc_dsh->dsh_qp_ptr->qp_ddq_cb.qeq_d7_deltable;

	dsh->dsh_qp_ptr->qp_ddq_cb.qeq_d9_delown
		= assoc_dsh->dsh_qp_ptr->qp_ddq_cb.qeq_d9_delown;

	STRUCT_ASSIGN_MACRO(assoc_dsh->dsh_ddb_cb->qee_d5_local_qid,
				    dsh->dsh_ddb_cb->qee_d5_local_qid);

	status = qeq_c2_delete(qef_rcb, dsh);
	return(status);
    }

    /* LDB processing */

    for (;;)
    {
	/* A cursor delete affects only a single row. If we are called back  */
	/* after rules processing simply return. */
	if (state == DSH_CT_CONTINUE)
	    break;

	/* Address the DMR_CB for performing the DELETE. */
	odmr_cb_ix = act->qhd_obj.qhd_qep.ahd_odmr_cb;
	dmr_cb = (DMR_CB*) assoc_cbs[odmr_cb_ix];
	if (state == DSH_CT_CONTINUE3)
	{
	    /* After a BEFORE rule, retrieve DMR CB, tid */
	    MEcopy((PTR) dsh->dsh_row[act->qhd_obj.qhd_qep.u1.s1.ahd_b_tid],
			sizeof(DB_TID8), (PTR) &bigtid);
	    if (act->qhd_obj.qhd_qep.ahd_tidoffset != -1
	      && act->qhd_obj.qhd_qep.ahd_part_def)
	    {
		partno = bigtid.tid.tid_partno;
		dmr_cb = (DMR_CB *)assoc_cbs[odmr_cb_ix + partno + 1];
	    }
	}
	else
	{
	    /* Normal path */
	    dsh->dsh_qef_remnull = 0;
	    dsh->dsh_qef_rowcount = 0;

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
	        
	    /* Check that the query plan has delete privilege */
	    if ((assoc_dsh->dsh_qp_ptr->qp_status & QEQP_DELETE) == 0)
	    {
		/* no permission */
		dsh->dsh_error.err_code = E_QE0009_NO_PERMISSION;
		status = E_DB_ERROR;
		break;
    	    }

	    /* The delete operation will leave the cursor unpositioned */
	    assoc_dsh->dsh_positioned = FALSE;

	    /* Use fetch row for holding update row if need be */
	    output = assoc_dsh->dsh_row[assoc_dsh->dsh_qp_ptr->qp_fetch_ahd->qhd_obj.qhd_qep.ahd_ruprow];

	    /* Delete the row. */
	    /* If the cursor is positioned (ahd_tidoffset == -1), delete the
	    ** row at the current position. If the cursor is not positioned
	    ** (e.g., the current row has been found via a secondary index),
	    ** delete the row by TID.
	    */

	    if (act->qhd_obj.qhd_qep.ahd_tidoffset == -1)
	    {
		if (act->qhd_obj.qhd_qep.ahd_part_def != NULL)
		{
		    i4 orig_node = act->qhd_obj.qhd_qep.ahd_ruporig;
		    if (orig_node == -1)
		    {
			TRdisplay("%@ qea_rdel: missing ahd_ruporig\n");
			dsh->dsh_error.err_code = E_QE0002_INTERNAL_ERROR;
			status = E_DB_ERROR;
			break;
		    }
		    partno = assoc_dsh->dsh_xaddrs[orig_node]->
						qex_status->node_ppart_num;
		    dmr_cb = (DMR_CB *) assoc_cbs[odmr_cb_ix + partno + 1];
		}
		dmr_cb->dmr_flags_mask = DMR_CURRENT_POS;
	    }
	    else
	    {
		/* Must do the delete by TID */
		tidinput = (char *)assoc_dsh->dsh_row[act->
			qhd_obj.qhd_qep.ahd_tidrow] + act->
			qhd_obj.qhd_qep.ahd_tidoffset;
		if (act->qhd_obj.qhd_qep.ahd_qepflag & AHD_4BYTE_TIDP)
		{
		    I4ASSIGN_MACRO(*tidinput, bigtid.tid_i4.tid);
		    bigtid.tid_i4.tpf = 0;
		}
		else
		{
		    I8ASSIGN_MACRO(*tidinput, bigtid);
		}
		partno = bigtid.tid.tid_partno;

		/* Check for partitioning and set up DMR_CB. */
		if (act->qhd_obj.qhd_qep.ahd_part_def)
		{
		    status = qeq_part_open(qef_rcb, assoc_dsh, NULL, 0, 
			odmr_cb_ix,
			act->qhd_obj.qhd_qep.ahd_dmtix, partno);
			
		    if (status != E_DB_OK)
		    {
			dsh->dsh_error.err_code = assoc_dsh->dsh_error.err_code;
			return(status);
		    }
		    pdmr_cb = (DMR_CB *)assoc_cbs[odmr_cb_ix + partno + 1];
		    STRUCT_ASSIGN_MACRO(dmr_cb->dmr_data, pdmr_cb->dmr_data);
		    dmr_cb = pdmr_cb;
		}
		dmr_cb->dmr_flags_mask = DMR_BY_TID;
		dmr_cb->dmr_tid = bigtid.tid_i4.tid;

		if (act->qhd_obj.qhd_qep.ahd_after_act != NULL ||
		    act->qhd_obj.qhd_qep.ahd_before_act != NULL)
	    	{
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
	    }

	    /* Save old TID from DMR_CB used to fetch this row */
	    bigtid.tid_i4.tpf = 0;		/* Clean out unused stuff */
	    bigtid.tid_i4.tid = dmr_cb->dmr_tid;
	    bigtid.tid.tid_partno = partno;

	    /* If we have rules, save a copy of the row before updating */
	    if (act->qhd_obj.qhd_qep.ahd_after_act != NULL ||
		act->qhd_obj.qhd_qep.ahd_before_act != NULL)
	    {
		/* Don't save ahd_a_row as we'll never access it DELETE */
		MEcopy(output, act->qhd_obj.qhd_qep.ahd_repsize,
		    dsh->dsh_row[act->qhd_obj.qhd_qep.u1.s1.ahd_b_row]);
	    }

	    /* Look for before rules to apply */
	    if (act->qhd_obj.qhd_qep.ahd_before_act != NULL)
	    {
		/*
		** Place the TID in a known location so it can be used
		** as a parameter to the procedure fired by the rule.
		*/
		MEcopy ((PTR)&bigtid, ( u_i2 ) sizeof(DB_TID8),
		    (PTR) dsh->dsh_row[act->qhd_obj.qhd_qep.u1.s1.ahd_b_tid]);  
	        dsh->dsh_ctx_act[dsh->dsh_depth_act].dsh_ct_ptr = act;
	        dsh->dsh_ctx_act[dsh->dsh_depth_act].dsh_ct_status =
		    DSH_CT_CONTINUE3;
		dsh->dsh_depth_act++;
		dsh->dsh_act_ptr = act->qhd_obj.qhd_qep.ahd_before_act;
		dsh->dsh_error.err_code = E_QE0120_RULE_ACTION_LIST;
		return (E_DB_ERROR);
	    }
	}   /* end of !DSH_CT_CONTINUE3 */

	state = DSH_CT_INITIAL;			/* signal resumption after
						** BEFORE trigger */

	status = dmf_call(DMR_DELETE, dmr_cb);
	if (status != E_DB_OK)
	{
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
	else
	{
       	    dsh->dsh_qef_rowcount = 1;
	    /* Look for after rules to apply */
	    if (act->qhd_obj.qhd_qep.ahd_after_act != NULL)
	    {
		/*
		** Place the TID in a known location so it can be used
		** as a parameter to the procedure fired by the rule.
		*/
		MEcopy ((PTR)&bigtid, ( u_i2 ) sizeof(DB_TID8),
		    ( PTR ) dsh->dsh_row[act->qhd_obj.qhd_qep.u1.s1.ahd_b_tid]);   

	        dsh->dsh_ctx_act[dsh->dsh_depth_act].dsh_ct_ptr = act;
	        dsh->dsh_ctx_act[dsh->dsh_depth_act].dsh_ct_status =
		    DSH_CT_CONTINUE;
		dsh->dsh_depth_act++;
		dsh->dsh_act_ptr = act->qhd_obj.qhd_qep.ahd_after_act;
		dsh->dsh_error.err_code = E_QE0120_RULE_ACTION_LIST;
		status = E_DB_ERROR;
	    
	    }
	}

	break;
    }
    return (status);
}		
