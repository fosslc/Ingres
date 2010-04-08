/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <ulf.h>
#include    <ulm.h>
#include    <adf.h>
#include    <ade.h>
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
#include    <qefqeu.h>

#include    <dmmcb.h>
#include    <dmucb.h>
#include    <ex.h>
#include    <tm.h>

#include    <dudbms.h>
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
**  Name: QEAPROJ.C - project the by-list values into the sort relation.
**
**  Description:
**      The QEP is read. The tuples read contain the by-list values for
**  an aggregate function. The by-list values are then projected into the
**  sort file.
**
**          QEA_PROJ	 - project by-list values
**
**
**  History:    $Log-for RCS$
**      27-aug-86 (daved)
**          written
**	16-mar-87 (rogerk) Changed DMR_LOAD call to use dmr_mdata for multi-row
**	    interface.
**	02-feb-88 (puree)
**	    add reset flag to qea_proj function arguments.
**	30-mar-89 (paul)
**	    Add resource control checking.
**	08-dec-92 (anitap)
**	    Added #include <qsf.h> and <psfparse.h> for CREATE SCHEMA.
**      13-sep-93 (smc)
**          Added <cs.h> for CS_SID.
**	17-may-95 (ramra01)
**	    Check for flags_mask DMR_EMPTY | DMR_TABLE_EXISTS which is
**	    set by qea_dmu in CREATE_TEMP for dbprocs when in a loop
**	    to indicate that these are reusable temp tables - b68464
**	7-nov-95 (inkdo01)
**	    Changes to replace QEN_ADF structure instances by pointers in
**	    QEF_AHD structures.
**	7-nov-1998 (nanpr01)
**	    Pass the DMR_HEAPSORT flag since additional consistency checks 
**	    were added in dmrload to detect if QEF is passing SORT flags
**	    when not needed.
**	23-feb-04 (inkdo01)
**	    Changed qef_rcb->error to dsh->dsh_error for || thread safety.
**	28-Dec-2005 (kschendel)
**	    Use DSH references whenever possible instead of QEF RCB
**	    references.  (This simplifies the code, eliminating many
**	    RCB references, and reflects the new primacy of the DSH
**	    thanks to the parallel query changes.)
[@history_template@]...
**/


/*{
** Name: QEA_PROJ	- project by-list values
**
** Description:
**      The QEP is read. The tuples read contain the by-list values for
**  an aggregate function. The by-list values are then projected into the
**  sort file.
**
** Inputs:
**      action				QEA_PROJ action item
**
** Outputs:
**      qef_rcb
**	    .error.err_code		one of the following
**				E_QE0000_OK
**				E_QE0017_BAD_CB
**				E_QE0018_BAD_PARAM_IN_CB
**				E_QE0019_NON_INTERNAL_FAIL
**				E_QE0002_INTERNAL_ERROR
**				E_QE0015_NO_MORE_ROWS
**	Returns:
**	    E_DB_{OK, WARN, ERROR, FATAL}
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      27-aug-86 (daved)
**          written
**	16-mar-87 (rogerk) Changed DMR_LOAD call to use dmr_mdata for multi-row
**	    interface.
**	28-sep-87 (rogerk)
**	    Zero dmr_s_estimated_records before call to dmr_load.
**	02-nov-87 (puree)
**	    set dmr_s_estimated_records from qen_est_tuples.
**	02-feb-88 (puree)
**	    add reset flag to the function arguments.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      16-may-95 (ramra01)
**	    fix for reuse of temp tables by setting the flag DMR_EMPTY
**          This is to support reusability in the case of dbproc which
**          are in a while loop. B68464
**	7-nov-95 (inkdo01)
**	    Changes to replace QEN_ADF structure instances by pointers in
**	    QEF_AHD structures.
**	21-nov-03 (inkdo01)
**	    Add open_flag parm to qen_func calls for parallel query support.
**	29-dec-03 (inkdo01)
**	    DSH is now parameter, "function" replaces "reset".
**	28-feb-04 (inkdo01)
**	    Change qef_rcb parm to dsh in qeq_chk_resources call.
**	4-Jun-2009 (kschendel) b122118
**	    Minor dead code removal.
[@history_template@]...
*/
DB_STATUS
qea_proj(
QEF_AHD		    *action,
QEF_RCB		    *qef_rcb,
QEE_DSH		    *dsh,
i4		    function )
{
    QEN_NODE	*node = action->qhd_obj.qhd_qep.ahd_qep;
    bool	reset = ((function & FUNC_RESET) != 0);
    i4		open_flag = (reset) ? (TOP_OPEN | function) : function;
    DB_STATUS	status;
    QEF_CB	*qef_cb = dsh->dsh_qefcb;
    PTR		*cbs	= dsh->dsh_cbs;
    QEN_ADF	*qen_adf;
    ADE_EXCB	*ade_excb;
    DMR_CB	*dmr_cb	= (DMR_CB*) cbs[action->qhd_obj.qhd_qep.ahd_odmr_cb];
    DM_MDATA	dm_mdata;
    i4		rowno;
    i4          flag = DMR_TABLE_EXISTS | DMR_EMPTY;


    /* Check if resource limits are exceeded by this query */
    if (action->qhd_obj.qhd_qep.ahd_estimates && (qef_cb->qef_fl_dbpriv &
	(DBPR_QROW_LIMIT | DBPR_QDIO_LIMIT | DBPR_QCPU_LIMIT |
	 DBPR_QPAGE_LIMIT | DBPR_QCOST_LIMIT))
	)
    {
	if (qeq_chk_resource(dsh, action) != E_DB_OK)
	    return (E_DB_ERROR);
    }

    qen_adf		= action->qhd_obj.qhd_qep.ahd_current;
    rowno		= qen_adf->qen_output;
    ade_excb		= (ADE_EXCB*) cbs[qen_adf->qen_pos];	
    ade_excb->excb_seg	= ADE_SMAIN;
    dmr_cb->dmr_count	= 1;
    dmr_cb->dmr_mdata	= &dm_mdata;
    dmr_cb->dmr_mdata->data_address = dsh->dsh_row[rowno];
    dmr_cb->dmr_mdata->data_size    = dsh->dsh_qp_ptr->qp_row_len[rowno];

    if (dmr_cb->dmr_flags_mask == flag)
        dmr_cb->dmr_flags_mask	    |= (DMR_NODUPLICATES | DMR_HEAPSORT);
    else
        dmr_cb->dmr_flags_mask      = (DMR_NODUPLICATES | DMR_HEAPSORT);

    dmr_cb->dmr_s_estimated_records = node->qen_est_tuples;
    dmr_cb->dmr_tid		    = 0;                    /* not used */

    for (;;)
    {
	/* fetch rows */
	status = (*node->qen_func)(node, qef_rcb, dsh, open_flag);
	open_flag &= ~(TOP_OPEN | FUNC_RESET);
	if (status != E_DB_OK)
	{
	    /* the error.err_code field in dsh should already be
	    ** filled in.
	    */
	    if (dsh->dsh_error.err_code == E_QE0015_NO_MORE_ROWS)
		break;
	    return (status);
	}
	
	/* project the by-list */
	status = ade_execute_cx(dsh->dsh_adf_cb, ade_excb);
	if (status != E_DB_OK)
	{
	    if ((status = qef_adf_error(&dsh->dsh_adf_cb->adf_errcb, 
		status, qef_cb, &dsh->dsh_error)) != E_DB_OK)
		return (status);
	}
	/* append the by-list values to the temp sort table */
	status = dmf_call(DMR_LOAD, dmr_cb);
	if (status != E_DB_OK)
	{
	    {
		dsh->dsh_error.err_code = dmr_cb->error.err_code;
		return (status);
	    }
	}

    }
    /* Tell DMF that there are no more rows */
    if ((dmr_cb->dmr_flags_mask & DMR_TABLE_EXISTS) &&
            (dmr_cb->dmr_flags_mask & DMR_EMPTY))
            dmr_cb->dmr_flags_mask          &= ~(DMR_EMPTY | DMR_NODUPLICATES);
    else 
       dmr_cb->dmr_flags_mask = DMR_HEAPSORT;

    dmr_cb->dmr_flags_mask |= DMR_ENDLOAD;
    status = dmf_call(DMR_LOAD, dmr_cb);
    if (status != E_DB_OK)
    {
	{
	    dsh->dsh_error.err_code = dmr_cb->error.err_code;
	    return (status);
	}
    }
    return (E_DB_OK);
}
