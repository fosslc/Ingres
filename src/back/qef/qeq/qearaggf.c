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
**  Name: QEARAGGF.C - return aggregate function row
**
**  Description:
**      This action returns the aggregate result tuple instead of
**  appending it to a file.
**
**          qea_raggf - return aggregate function row
**
**
**  History:    $Log-for RCS$
**      12-mar-87 (daved)
**          written
**	02-feb-88 (puree)
**	    add reset flag to qea_raggf function arguments.
**      19-feb-92 (anitap)
**          added the line qef_rcb->qef_rowcount = 0 to fix bug 40557.
**	08-dec-92 (anitap)
**	    Included <qsf.h> and <psfparse.h> for CREATE SCHEMA.
**      13-sep-93 (smc)
**          Added <cs.h> for CS_SID.
**	7-nov-95 (inkdo01)
**	    Changes to replace QEN_ADF structure instances by pointers in
**	    QEF_AHD structures.
**	23-feb-04 (inkdo01)
**	    Changed qef_rcb->error to dsh->dsh_error for || thread safety
**	    (and others).
**	28-Dec-2005 (kschendel)
**	    Use DSH references whenever possible instead of QEF RCB
**	    references.  (This simplifies the code, eliminating many
**	    RCB references, and reflects the new primacy of the DSH
**	    thanks to the parallel query changes.)
[@history_template@]...
**/


/*{
** Name: QEA_RAGGF	- compute aggregate function
**
** Description:
**      The following algorithm is used to produce the aggregate result
**  tuple. NOTE- this node only performs no-project aggregates.
**
**	1) If a QEP exists for the aggregate, get a tuple
**	    (it was positioned in qea_validate).
**	2) Init the result tuple
**	3) For each tuple in the QEP with the same by-list values, accumulate
**	    the aggregate result.
**	4) When the by-list changes, materialize the result tuple, put it into
**	the result relation, materialize the new by-list values.
**
** Inputs:
**      action                          the QEA_RAGGF action
**	qef_rcb
**	reset				The reset flag if result has to be 
**					re-initialized
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
**      11-mar-87 (daved)
**          written
**	02-feb-88 (puree)
**	    add reset flag to the function arguments.  Use this flag to
**	    reinitialize the aggregate result.
**	11-feb-88 (puree)
**	    fixed bug associated with reset and first_time.
**      19-feb-92 (anitap)
**          fixed bug associated with 0 rows for the first time from the node 
**	    below. bug 40557.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	7-nov-95 (inkdo01)
**	    Changes to replace QEN_ADF structure instances by pointers in
**	    QEF_AHD structures.
**	21-nov-03 (inkdo01)
**	    Add open_flag parm to qen_func calls for parallel query support.
**	29-dec-03 (inkdo01)
**	    DSH is now parameter, "function" replaces "reset".
**	26-aug-04 (inkdo01)
**	    Add global base array support for raggf results.
[@history_template@]...
*/
DB_STATUS
qea_raggf(
QEF_AHD		    *action,
QEF_RCB		    *qef_rcb,
QEE_DSH		    *dsh,
i4		    function,
bool		    first_time )
{
    QEN_NODE	*node = action->qhd_obj.qhd_qep.ahd_qep;
    DB_STATUS	status;
    DB_STATUS	ret_val = E_DB_OK;
    QEF_CB	*qef_cb = dsh->dsh_qefcb;
    PTR		*cbs	= dsh->dsh_cbs;
    ADF_CB	*adfcb = dsh->dsh_adf_cb;
    QEN_ADF	*qen_adf;
    ADE_EXCB	*ade_excb;
    bool	reset = ((function & FUNC_RESET) != 0);
    i4		open_flag = (reset) ? (TOP_OPEN | function) : function;

    dsh->dsh_error.err_code = E_QE0000_OK;
    dsh->dsh_qef_rowcount = 0;

    if (node && (first_time | reset))
    {
	/* get first record */
	status = (*node->qen_func)(node, qef_rcb, dsh, open_flag);
	open_flag = function &= ~(TOP_OPEN | FUNC_RESET);
	if (status != E_DB_OK)
	{
	    /* the error.err_code field in dsh should already be
	    ** filled in.
	    */
	    dsh->dsh_qef_rowcount = 0;
	    return (status);
	}
    }
    /* initialize the aggregate result */
    /* if the aggregate is of the form x=sum(1 by 5), where there are no
    ** var nodes, the aggregate is computed here.
    */
    qen_adf		= action->qhd_obj.qhd_qep.ahd_current;
    ade_excb		= (ADE_EXCB*) cbs[qen_adf->qen_pos];	
    ade_excb->excb_seg = ADE_SINIT;
    if (qen_adf->qen_uoutput > -1)
     if (dsh->dsh_qp_ptr->qp_status & QEQP_GLOBAL_BASEARRAY)
	dsh->dsh_row[qen_adf->qen_uoutput] = dsh->dsh_qef_output->dt_data;
     else ade_excb->excb_bases[ADE_ZBASE + qen_adf->qen_uoutput] =
		dsh->dsh_qef_output->dt_data;
    status = ade_execute_cx(adfcb, ade_excb);
    if (status != E_DB_OK)
    {
	if ((status = qef_adf_error(&adfcb->adf_errcb, 
	    status, qef_cb, &dsh->dsh_error)) != E_DB_OK)
	    return (status);
    }
    dsh->dsh_qef_remnull |= ade_excb->excb_nullskip;


    /* compute the result tuples */
    if (node)
    {
	for (;;)
	{
	    /* compute the aggregate */
	    ade_excb->excb_seg = ADE_SMAIN;
	    status = ade_execute_cx(adfcb, ade_excb);
	    if (status != E_DB_OK)
	    {
		if ((status = qef_adf_error(&adfcb->adf_errcb, 
		    status, qef_cb, &dsh->dsh_error)) != E_DB_OK)
		    return (status);
	    }
	    dsh->dsh_qef_remnull |= ade_excb->excb_nullskip;
	    /* if the qualification returned false, finalize this
	    ** result and return.
	    */
	    if (ade_excb->excb_value != ADE_TRUE)
	    {
		/* compute the result row */
		ade_excb->excb_seg = ADE_SFINIT;
		status = ade_execute_cx(adfcb, ade_excb);
		if (status != E_DB_OK)
		{
		    if ((status = qef_adf_error(&adfcb->adf_errcb, 
			status, qef_cb, &dsh->dsh_error)) != E_DB_OK)
			return (status);
		}
		dsh->dsh_qef_rowcount = 1;
		dsh->dsh_error.err_code = E_QE0000_OK;
		return (E_DB_OK);
	    }
	    /* fetch rows */
	    status = (*node->qen_func)(node, qef_rcb, dsh, NO_FUNC);
	    if (status != E_DB_OK)
	    {
		/* the error.err_code field in dsh should already be
		** filled in.
		*/
		if (dsh->dsh_error.err_code == E_QE0015_NO_MORE_ROWS)
		{
		    ret_val = E_DB_WARN;
		    break;
		}
		return (status);
	    }
	}
    }
    else
    {
	/* compute the aggregate */
	ade_excb->excb_seg = ADE_SMAIN;
	status = ade_execute_cx(adfcb, ade_excb);
	if (status != E_DB_OK)
	{
	    if ((status = qef_adf_error(&adfcb->adf_errcb, 
		status, qef_cb, &dsh->dsh_error)) != E_DB_OK)
		return (status);
	}
	dsh->dsh_qef_remnull |= ade_excb->excb_nullskip;
	ret_val = E_DB_WARN;
    }

    /* compute the final result row */
    ade_excb->excb_seg = ADE_SFINIT;
    status = ade_execute_cx(adfcb, ade_excb);
    if (status != E_DB_OK)
    {
	if ((status = qef_adf_error(&adfcb->adf_errcb, 
	    status, qef_cb, &dsh->dsh_error)) != E_DB_OK)
	    return (status);
    }
    dsh->dsh_qef_rowcount = 1;
    return (ret_val);
}
