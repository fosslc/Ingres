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
**  Name: QEAAGGF.C - compute aggregate functions
**
**  Description:
**      The routines in this file are used to execute the QEA_AGGF
**  action item. 
**
**          qea_aggf	 - produce the aggregate result relation
**          qe_putres	 - put a tuple into the result relation
**
**
**  History:    $Log-for RCS$
**      27-aug-86 (daved)
**          written
**	02-feb-88 (puree)
**	    add reset flag to qea_aggf function arguments.
**	30-mar-89 (paul)
**	    Add resource control checking.
**	12-dec-91 (davebf)
**	    Execute ahd_constant here instead of qeq_validate (b41134)
**	08-dec-92 (anitap)
**	    Added #include <qsf.h> and <psfparse.h>.
**      13-sep-93 (smc)
**          Added <cs.h> for CS_SID.
**	7-nov-95 (inkdo01)
**	    Changes to replace QEN_ADF structure instances by pointers in
**	    QEF_AHD structures.
**      23-jul-01 (wanfr01)
**          Bug 105318, INGSRV 1501
**          Handle situation where a result row is deleted in
**          mid-aggregate processing.
**	23-feb-04 (inkdo01)
**	    Changed qef_rcb->error to dsh->dsh_error for || thread safety.
**	28-Dec-2005 (kschendel)
**	    Use DSH references whenever possible instead of QEF RCB
**	    references.  (This simplifies the code, eliminating many
**	    RCB references, and reflects the new primacy of the DSH
**	    thanks to the parallel query changes.)
**	13-Jan-2006 (kschendel)
**	    Tighten up CX execution calls a bit.
[@history_template@]...
**/

/* Local static function definitions */

static DB_STATUS qe_putres(
QEE_DSH            *dsh,
ADE_EXCB           *proj_excb,
DMR_CB             *proj_dmr,
DMR_CB             *dmr_cb,
char               *proj_tuple,
i4                 proj_len );



/*{
** Name: QEA_AGGF	- compute aggregate function
**
** Description:
**      The following algorithm is used to produce the aggregate result
**  relation. All temporary tables should exist and be open by the time 
**  this routine is called.
**
**	1) If a projected by-list table is available, position and get
**	the first tuple.
**	2) If a QEP exists for the aggregate, get the first tuple
**	(it was positioned in qea_validate).
**	3) Init the first result tuple
**	4) For each tuple in the QEP with the same by-list values, accumulate
**	the aggregate result.
**	5) When the by-list changes, materialize the result tuple, put it into
**	the result relation, materialize the new by-list values.
**	6) when there are no more tuples, materialize the last result tuple
**	and add it to the result relation.
**	7) Put any remaining projected by-list values into the result relation.
**
** Inputs:
**      action                          the QEA_AGGF action
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
**	02-feb-88 (puree)
**	    add reset flag to the function arguments.
**	28-sept-89 (eric)
**	    Add support to fix the sybase bug. This is done by returning
**	    a zerotup check flag when invalidating a query plan because
**	    it's QEN_NODE returns zero rows.
**	12-dec-91 (davebf)
**	    Execute ahd_constant here instead of qeq_validate (b41134)
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	7-nov-95 (inkdo01)
**	    Changes to replace QEN_ADF structure instances by pointers in
**	    QEF_AHD structures.
**	21-nov-03 (inkdo01)
**	    Add open_flag parm to qen_func calls for parallel query support.
**	29-dec-03 (inkdo01)
**	    DSH is now parameter, "function" replaces "reset".
**	27-feb-04 (inkdo01)
**	    Changed dsh_qen_status from ptr to array of QEN_STATUSs to 
**	    ptr to arrays of ptrs.
**	28-feb-04 (inkdo01)
**	    Change qef_rcb parm to dsh in qeq_chk_resources call.
**	13-Dec-2005 (kschendel)
**	    Can count on qen-ade-cx now.
*/
DB_STATUS
qea_aggf(
QEF_AHD		    *action,
QEF_RCB		    *qef_rcb,
QEE_DSH		    *dsh,
i4		    function )
{
    QEN_NODE	*node = action->qhd_obj.qhd_qep.ahd_qep;
    ADF_CB	*adf_cb = dsh->dsh_adf_cb;
    DB_STATUS	status;
    QEF_CB	*qef_cb = dsh->dsh_qefcb;
    PTR		*cbs	= dsh->dsh_cbs;
    QEN_ADF	*qen_adf;
    QEN_ADF	*proj_adf;
    QEN_ADF	*const_adf;
    ADE_EXCB	*ade_excb;
    ADE_EXCB	*proj_excb;
    ADE_EXCB	*const_excb;
    DMR_CB	*dmr_cb	= (DMR_CB*) cbs[action->qhd_obj.qhd_qep.ahd_odmr_cb];
    DMR_CB	*proj_dmr;
    i4		rowno;
    char	*proj_tuple;
    i4		proj_len;
    bool	reset = ((function & FUNC_RESET) != 0);

    /* Check if resource limits are exceeded by this query */
    if (action->qhd_obj.qhd_qep.ahd_estimates && (qef_cb->qef_fl_dbpriv &
	(DBPR_QROW_LIMIT | DBPR_QDIO_LIMIT | DBPR_QCPU_LIMIT |
	 DBPR_QPAGE_LIMIT | DBPR_QCOST_LIMIT))
	)
    {
	if (qeq_chk_resource(dsh, action) != E_DB_OK)
	    return (E_DB_ERROR);
    }

    /* setup the result relation for output */
    qen_adf		= action->qhd_obj.qhd_qep.ahd_current;
    rowno		= qen_adf->qen_output;
    ade_excb		= (ADE_EXCB*) cbs[qen_adf->qen_pos];	
    dmr_cb->dmr_data.data_address   = dsh->dsh_row[rowno];
    dmr_cb->dmr_data.data_in_size   = dsh->dsh_qp_ptr->qp_row_len[rowno];
    dmr_cb->dmr_flags_mask	    = 0;
    dmr_cb->dmr_tid		    = 0;                    /* not used */

    /* setup the projected by-list input */
    if (action->qhd_obj.qhd_qep.ahd_proj > -1)
    {
	proj_dmr    = (DMR_CB*) cbs[action->qhd_obj.qhd_qep.ahd_proj];
	proj_dmr->dmr_flags_mask    = 0;
	proj_dmr->dmr_tid	    = 0;                    /* not used */
	proj_adf    = action->qhd_obj.qhd_qep.ahd_by_comp;
	rowno	    = proj_adf->qen_output;
	proj_tuple  = dsh->dsh_row[rowno];
	proj_len    = dsh->dsh_qp_ptr->qp_row_len[rowno];
	proj_excb   = (ADE_EXCB*) cbs[proj_adf->qen_pos];
	proj_excb->excb_seg = ADE_SMAIN;

	/* position first projected tuple */

	proj_dmr->dmr_position_type = DMR_ALL;
	proj_dmr->dmr_attr_desc.ptr_address	= 0;
	proj_dmr->dmr_attr_desc.ptr_size	= 0;
	proj_dmr->dmr_attr_desc.ptr_in_count	= 0;
	proj_dmr->dmr_q_fcn			= 0;
	if (status = dmf_call(DMR_POSITION, proj_dmr))
	{
	    if (proj_dmr->error.err_code == E_DM0055_NONEXT)
		proj_dmr = 0;
	    else
	    {
		dsh->dsh_error.err_code = proj_dmr->error.err_code;
		return (status);
	    }
	}
	proj_dmr->dmr_flags_mask = DMR_NEXT;
    }
    else
    {
	proj_dmr    = 0;
    }

    if (node)
    {
	/* if there's a constant CX for the action, execute it */
	const_adf = action->qhd_obj.qhd_qep.ahd_constant;
	if (const_adf != NULL)
	{
	    const_excb    = (ADE_EXCB*) cbs[const_adf->qen_pos];	

	    /* process the constant expression */
	    status = qen_execute_cx(dsh, const_excb);
	    if (status != E_DB_OK)
	    {
		    return (status);
	    }
	    /* handle the condition where the qualification failed */
	    if (const_excb->excb_value != ADE_TRUE)
		goto project;	/* go do projections if any */
	}

	/* get first record */
	status = (*node->qen_func)(node, qef_rcb, dsh, 
					(i4) (function | TOP_OPEN));
				/* TOP_OPEN inits for parallel plans */

	/* If the qen_node returned no rows and we are supposed to
	** invalidate this query plan if this action doesn't get
	** any rows, then invalidate the plan.
	*/
	if (dsh->dsh_error.err_code == E_QE0015_NO_MORE_ROWS &&
		dsh->dsh_xaddrs[node->qen_num]->qex_status->node_rcount == 0 &&
		(action->ahd_flags & QEA_ZEROTUP_CHK) != 0
	    )
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
	    /* the error.err_code field in qef_rcb should already be
	    ** filled in.
	    */
	    if (dsh->dsh_error.err_code == E_QE0015_NO_MORE_ROWS)
		goto project;
	    return (status);
	}
    }
    /* initialize the aggregate result */
    /* if the aggregate is of the form x=sum(1 by 5), where there are no
    ** var nodes, the aggregate is computed here.
    */
    ade_excb->excb_seg = ADE_SINIT;
    status = ade_execute_cx(adf_cb, ade_excb);
    if (status != E_DB_OK)
    {
	if ((status = qef_adf_error(&adf_cb->adf_errcb, 
	    status, qef_cb, &dsh->dsh_error)) != E_DB_OK)
	    return (status);
    }
    dsh->dsh_qef_remnull |= ade_excb->excb_nullskip;


    /* compute the result tuples */
    if (node)
    {
	for (;;)
	{
	    /* compute the aggregate (MAIN segment) */
	    status = qen_execute_cx(dsh, ade_excb);
	    if (status != E_DB_OK)
	    {
		return (status);
	    }
	    dsh->dsh_qef_remnull |= ade_excb->excb_nullskip;
	    /* if the qualifiaction returned false, finalize this
	    ** result and prepare for the next.
	    */
	    if (ade_excb->excb_value != ADE_TRUE)
	    {
		/* compute the result row */
		ade_excb->excb_seg = ADE_SFINIT;
		status = ade_execute_cx(adf_cb, ade_excb);
		if (status != E_DB_OK)
		{
		    if ((status = qef_adf_error(&adf_cb->adf_errcb, 
			status, qef_cb, &dsh->dsh_error)) != E_DB_OK)
			return (status);
		}
		/* put the result into the result relation */
		if (status = qe_putres(dsh, proj_excb, proj_dmr, dmr_cb,
			proj_tuple, proj_len))
		    return (status);
		/* prepare for next result */
		ade_excb->excb_seg = ADE_SINIT;
		status = ade_execute_cx(adf_cb, ade_excb);
		if (status != E_DB_OK)
		{
		    if ((status = qef_adf_error(&adf_cb->adf_errcb, 
			status, qef_cb, &dsh->dsh_error)) != E_DB_OK)
			return (status);
		}
		/* compute the aggregate */
		continue;
	    }
	    /* fetch rows */
	    status = (*node->qen_func)(node, qef_rcb, dsh, NO_FUNC);
	    if (status != E_DB_OK)
	    {
		/* the error.err_code field in qef_rcb should already be
		** filled in.
		*/
		if (dsh->dsh_error.err_code == E_QE0015_NO_MORE_ROWS)
		    break;
		return (status);
	    }
	}
    }
    else
    {
	/* compute the aggregate (MAIN segment) */
	status = qen_execute_cx(dsh, ade_excb);
	if (status != E_DB_OK)
	{
	    return (status);
	}
	dsh->dsh_qef_remnull |= ade_excb->excb_nullskip;
    }

    /* send last result row to result relation */
    ade_excb->excb_seg = ADE_SFINIT;
    status = ade_execute_cx(adf_cb, ade_excb);
    if (status != E_DB_OK)
    {
	if ((status = qef_adf_error(&adf_cb->adf_errcb, 
	    status, qef_cb, &dsh->dsh_error)) != E_DB_OK)
	    return (status);
    }
    /* put the result into the result relation */
    if (status = qe_putres(dsh, proj_excb, proj_dmr, dmr_cb,
    	proj_tuple, proj_len))
	return (status);

project:
    status = E_DB_OK;
    /* put the remaining projected tuples into the result relation */
    if (proj_dmr)
    {
	char	    *res_tuple;	    /* result tuple from dmr_cb */
	i4	    res_len;	    /* length of result tuple buffer */

	/* save the old result tuple info */
	res_tuple   = dmr_cb->dmr_data.data_address;
	res_len	    = dmr_cb->dmr_data.data_in_size;

	/* change the data tuple for the dmr_put call. */
	dmr_cb->dmr_data.data_address   = proj_tuple;
	dmr_cb->dmr_data.data_in_size   = proj_len;

	for (;;)
	{
	    /* get projected row */
	    if (status = dmf_call(DMR_GET, proj_dmr))
	    {
		if (proj_dmr->error.err_code == E_DM0055_NONEXT)
		{
		    proj_dmr = 0;
		    break;
		}
		else
		{
		    dsh->dsh_error.err_code = dmr_cb->error.err_code;
		    return (status);
		}
	    }
	    /* project the result row */
	    status = qen_execute_cx(dsh, proj_excb);
	    if (status != E_DB_OK)
	    {
		return (status);
	    }
	    /* put the result row */
	    if (status = dmf_call(DMR_PUT, dmr_cb))
	    {
		dsh->dsh_error.err_code = dmr_cb->error.err_code;
		return (status);
	    }
	}
	/* restore the row buffers */
	dmr_cb->dmr_data.data_address = res_tuple;
	dmr_cb->dmr_data.data_in_size = res_len;
    }
	    
    return (E_DB_OK);
}

/*{
** Name: QE_PUTRES	- put the result tuple into the result relation
**
** Description:
**      The result tuple is placed into the result relation. If the aggregate
**  needs to display all by-list values, the projected by-list tuples need
**  to be placed into the result relation in the proper order. As long as
**  the projected tuple's by-list values are less than those found in the
**  result tuple, append the projected tuples to the result relation. 
**
** Inputs:
**      dsh
**	proj_excb			CX to produce the projected tuple
**	proj_dmr			DMR_CB to get another projected tuple
**	dmr_cb				append tuple to result relation
**	proj_tuple			the projected tuple
**	proj_len			length of buffer containing projected
**					tuple
**
** Outputs:
**      qef_cb                          session control block
**	    .qef_adf_cb.adf_errcb	adf error
**	Returns:
**	    E_DB_{OK,WARN,ERROR,FATAL,SEVERE}
**	Exceptions:
**	    none
**
** Side Effects:
**	    A new projected tuple will be read
**
** History:
**      28-aug-86 (daved)
**          written
**      23-jul-01 (wanfr01)
**          Bug 105318, INGSRV 1501
**          Handle situation where a result row is deleted in
**          mid-aggregate processing.
[@history_template@]...
*/
DB_STATUS
qe_putres(
QEE_DSH		   *dsh, 
ADE_EXCB           *proj_excb,
DMR_CB             *proj_dmr,
DMR_CB             *dmr_cb,
char               *proj_tuple,
i4                 proj_len )
{
    char	    *res_tuple;	    /* result tuple from dmr_cb */
    i4		    res_len;	    /* length of result tuple buffer */
    DB_STATUS	    status;
    ADF_CB	    *adf_cb = dsh->dsh_adf_cb;

    if (proj_dmr)
    {
	/* save the old result tuple info */
	res_tuple   = dmr_cb->dmr_data.data_address;
	res_len	    = dmr_cb->dmr_data.data_in_size;

	/* set the new output */
	dmr_cb->dmr_data.data_address	= proj_tuple;
	dmr_cb->dmr_data.data_in_size	= proj_len;
	
	/* project rows into the result relation */
	for (;;)
	{
	    /* get the next projected tuple */
	    if (status = dmf_call(DMR_GET, proj_dmr))
	    {
		if (proj_dmr->error.err_code == E_DM0055_NONEXT)
		{
		    proj_dmr = 0;
		    status = E_DB_OK;
		    break;
		}
		else
		{
		    dsh->dsh_error.err_code = dmr_cb->error.err_code;
		    return (status);
		}
	    }

	    proj_dmr->dmr_flags_mask = DMR_NEXT;

	    /* compare by-list values */
	    proj_excb->excb_seg = ADE_SINIT;
	    status = ade_execute_cx(adf_cb, proj_excb);
	    if (status != E_DB_OK)
	    {
		if ((status = qef_adf_error(&adf_cb->adf_errcb, 
		    status, dsh->dsh_qefcb, &dsh->dsh_error)) != E_DB_OK)
		    return (status);
	    }
	    if (proj_excb->excb_value == ADE_TRUE)
		break;

            /*  There is a slim chance that a row may have been deleted
            **  from the original table after the 'bylist' results were
            **  stored.  If this happens, proj_excb->exb_value will
            **  never be true, and excb_cmp will become ADE_1GT2.  Under
            **  this situation the result retrieved from the table must be
            **  reloaded to synchronize the bylist values retrieved
            **  from the table with the pre-loaded bylist values.
            */
            if (proj_excb->excb_cmp ==  ADE_1GT2)
            {
                proj_dmr->dmr_flags_mask = DMR_CURRENT_POS;
                break;
            }

	    /* project the tuple */
	    status = qen_execute_cx(dsh, proj_excb);
	    if (status != E_DB_OK)
	    {
		return (status);
	    }
	    /* append the tuple */
	    if (status = dmf_call(DMR_PUT, dmr_cb))
	    {
		dsh->dsh_error.err_code = dmr_cb->error.err_code;
		return (status);
	    }
	}

	/* restore the row buffers */
	dmr_cb->dmr_data.data_address = res_tuple;
	dmr_cb->dmr_data.data_in_size = res_len;
    }

    /* put the actual result tuple into the result relation */
    if (status = dmf_call(DMR_PUT, dmr_cb))
    {
	dsh->dsh_error.err_code = dmr_cb->error.err_code;
	return (status);
    }
    return (status);
}
