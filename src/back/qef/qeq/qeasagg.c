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
**  Name: QEASAGG.C - compute simple aggregates
**
**  Description:
**      The routines in this file compute simple, scalar aggregates.
**  The basic algorithm is:
**	1) initialize the result relation
**	2) read tuples from the QEP
**	3) compute the aggregate for each tuple
**	4) move the result to its output location. 
**
**
**          qea_sagg	 - compute a simple aggregate
**
**
**  History:    $Log-for RCS$
**      27-aug-86 (daved)
**          written
**	02-feb-88 (puree)
**	    add reset flag to qea_sagg function arguments.
**	30-mar-89 (paul)
**	    Add resource limit checking.
**	08-dec-92 (anitap)
**	    Included <qsf.h> and <psfparse.h> for CREATE SCHEMA.
**      13-sep-93 (smc)
**          Added <cs.h> for CS_SID.
**	3-sep-93 (rickh)
**	    Flag the referential constraint state machine whether any
**	    rows qualified or not.  This is to support ALTER TABLE ADD
**	    REFERENTIAL CONSTRAINT.
**	14-jul-95 (ramra01)
**	    Aggregate Otimization for Count(*) only. This is a forerunner
**	    for the bit flags in OPF to indicate aggf optimzations for   
**	    count(*), min, max (Any or all of them) in a query with
**	    restrictions.
**	21-aug-95 (ramra01)
**	    Extending aggf optimization for all simple aggregates as long
**	    as the node below is a ORIG node.
**      01-nov-95 (ramra01)
**          count(*) optimize bypass adf calls only when it is the only
**          simple aggregate.
**	15-mar-96 (pchang)
**	    Fixed regression to bug 73095.  An earlier fix was broken by changes
**	    made to enhance the processing of simple aggregate in DMF. 
**	7-nov-95 (inkdo01)
**	    Changes to replace QEN_ADF structure instances by pointers in
**	    QEF_AHD structures.
**	28-aug-1998 (somsa01 for ocoro02)
**	    Bug 88218: Only set dmr_cb->dmr_count to 0 if node->qen_type
**	    = QE_ORIGAGG. Otherwise dmr_cb isn't initialised and causes a
**	    SEGV. Make sure in future dmr_cb is initialised if used outside
**	    the scope of QE_ORIGAGG.
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
** Name: QEA_SAGG	- compute simple aggregate
**
** Description:
**      The routines in this file compute simple, scalar aggregates.
**  The basic algorithm is:
**	1) initialize the result relation
**	2) read tuples from the QEP
**	3) compute the aggregate for each tuple
**	4) move the result to its output location. 
**
**	Simple aggregate results are stored in a row buffer. The row buffer
**	used is in the qen_output field of the ahd_current QEN_ADF struct.
**        
**  The new implementation has a choice of two paths for aggregate processing.
**  If the involved aggregates are based on a simple table without a solo any
**  aggregate,they will invoke a special node called qen_origagg. 
**  The basic algorithm for this is:
**      1) initialize the result relation
**      2) call the node qen_origagg
**	3) move the result to its output location.
**
** Inputs:
**      action				QEA_SAGG action item
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
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	3-sep-93 (rickh)
**	    Flag the referential constraint state machine whether any
**	    rows qualified or not.  This is to support ALTER TABLE ADD
**	    REFERENTIAL CONSTRAINT.
**      14-jul-95 (ramra01)
**          Aggregate Otimization for Count(*) only. This is a forerunner
**          for the bit flags in OPF to indicate aggf optimzations for
**          count(*), min, max (Any or all of them) in a query with
**          restrictions.
**      21-aug-95 (ramra01)
**          Simple aggregate optimization extended to all functions 
**	    as long as it is against a base table.
**      01-nov-95 (ramra01)
**          count(*) optimize bypass adf calls only when it is the only
**          simple aggregate.
**	7-nov-95 (inkdo01)
**	    Changes to replace QEN_ADF structure instances by pointers in
**	    QEF_AHD structures.
**	19-feb-96 (inkdo01)
**	    Minor fix to optimize logic to avoid reference to ADF error
**	    buffer unless actually in optimize logic (fix for 74283).
**	15-mar-96 (pchang)
**	    Fixed regression to bug 73095.  An earlier fix was broken by changes
**	    made to enhance the processing of simple aggregate.  Specifically, 
**	    processing of simple aggregates is now carried out in DMF via
**	    qen_origagg() but the qef_remnull flag was not set after we 
**	    returned from the call to DMF.
**	29-oct-97 (inkdo01)
**	    Added code to support AHD_NOROWS flag (indicating no possible 
**	    rows from underlying query).
**	6-nov-97 (inkdo01)
**	    Add execution of ahd_constant code. SINIT/FINIT code must be 
**	    executed even if no rows are processed (because of ahd_constant).
**	28-aug-1998 (somsa01 for ocoro02)
**	    Bug 88218: Only set dmr_cb->dmr_count to 0 if node->qen_type
**	    = QE_ORIGAGG. Otherwise dmr_cb isn't initialised and causes a
**	    SEGV. Make sure in future dmr_cb is initialised if used outside
**	    the scope of QE_ORIGAGG.
**	2-jan-01 (inkdo01)
**	    Count optimization data ptr now addresses simple i4, not ADF_AG_STRUCT,
**	    with aggregate code improvements.
**	21-nov-03 (inkdo01)
**	    Add open_flag parm to qen_func calls for parallel query support.
**	29-dec-03 (inkdo01)
**	    DSH is now parameter, "function" replaces "reset".
**	16-feb-04 (inkdo01)
**	    Fix long term latent bug that produces bogus ADF errors.
**	28-feb-04 (inkdo01)
**	    Change qef_rcb parm to dsh in qeq_chk_resources call.
**	26-aug-04 (inkdo01)
**	    Support global base array for sagg result.
**	4-Jun-2009 (kschendel) b122118
**	    Minor dead code removal.
**	2-Dec-2010 (kschendel) SIR 124685
**	    Warning, prototype fixes.
*/
DB_STATUS
qea_sagg(
QEF_AHD		    *action,
QEF_RCB		    *qef_rcb,
QEE_DSH		    *dsh,
i4		    function )
{
    QEN_NODE	  *node = action->qhd_obj.qhd_qep.ahd_qep;
    bool	  reset = ((function & FUNC_RESET) != 0);
    DB_STATUS	  status;
    QEF_CB	  *qef_cb = dsh->dsh_qefcb;
    ADF_CB	  *adfcb = dsh->dsh_adf_cb;
    PTR		  *cbs	= dsh->dsh_cbs;
    QEN_ADF	  *qen_adf, *qen_adfconst;
    ADE_EXCB	  *ade_excb, *ade_excbconst;
    i4		  rowCount = 0;
    i4		  open_flag = (reset) ? (TOP_OPEN | function) : function;

    PTR           data = NULL;
    DMR_CB        *dmr_cb; 
    ADF_ERROR	  *adf_errcb;
    bool	  origagg = FALSE;
    i4  	  instr_cnt = 0;

    /* Check if resource limits are exceeded by this query */
    if (action->qhd_obj.qhd_qep.ahd_estimates && (qef_cb->qef_fl_dbpriv &
	(DBPR_QROW_LIMIT | DBPR_QDIO_LIMIT | DBPR_QCPU_LIMIT |
	 DBPR_QPAGE_LIMIT | DBPR_QCOST_LIMIT))
	)
    {
	if (qeq_chk_resource(dsh, action) != E_DB_OK)
	    return (E_DB_ERROR);
    }

    /* initialize the aggregate result */
    /* if there is no QEP, this will also create the result value */
    qen_adf		= action->qhd_obj.qhd_qep.ahd_current;
    ade_excb		= (ADE_EXCB*) cbs[qen_adf->qen_pos];	
    ade_excb->excb_seg	= ADE_SINIT;


    if (qen_adf->qen_uoutput > -1)
    {
	if (dsh->dsh_qp_ptr->qp_status & QEQP_GLOBAL_BASEARRAY)
	    dsh->dsh_row[qen_adf->qen_uoutput] = dsh->dsh_qef_output->dt_data;
	else
	    ade_excb->excb_bases[ADE_ZBASE + qen_adf->qen_uoutput] =
				dsh->dsh_qef_output->dt_data;
    }

    status = ade_execute_cx(adfcb, ade_excb);
    if (status != E_DB_OK)
    {
	if ((status = qef_adf_error(&adfcb->adf_errcb, status, 
	    qef_cb, &dsh->dsh_error)) != E_DB_OK)
	    return (status);
    }
    dsh->dsh_qef_remnull |= ade_excb->excb_nullskip;

    /* check for constant code */
    /* if there is some & it returns FALSE, skip to FINIT execution */
    qen_adfconst	= action->qhd_obj.qhd_qep.ahd_constant; 
    if (qen_adfconst)
    {
	ade_excbconst    = (ADE_EXCB*) cbs[qen_adfconst->qen_pos];
	ade_excbconst->excb_seg = ADE_SMAIN;
 
	/* process the constant expression */
	status = ade_execute_cx(adfcb, ade_excbconst);
	if (status != E_DB_OK)
	{
	    if ((status = qef_adf_error(&adfcb->adf_errcb,
			status, qef_cb, &dsh->dsh_error)) != E_DB_OK)
	     return (status);
	}
	/* handle the condition where the qualification failed */
	if (ade_excbconst->excb_value != ADE_TRUE)
	    goto finitit;
    }

    ade_countstar_loc( adfcb, ade_excb, &data, &instr_cnt);
    ade_excb->excb_seg	= ADE_SMAIN;
    /* get the rows */
    if (node)
    {
	origagg = (node->qen_type == QE_ORIGAGG);
	if (origagg)
        {
            dmr_cb = (DMR_CB*) cbs[node->node_qen.qen_orig.orig_get];
	    dmr_cb->dmr_agg_excb = (PTR) ade_excb; 
	    dmr_cb->dmr_agg_flags = 0;
	    dmr_cb->dmr_qef_adf_cb = (PTR) adfcb;
	    if (action->qhd_obj.qhd_qep.ahd_qepflag & AHD_CSTAROPT)
            {
	        dmr_cb->dmr_agg_flags |= DMR_AGLIM_COUNT;
	    }
	    if (node->node_qen.qen_orig.orig_flag & ORIG_MAXOPT)
	    {
	        dmr_cb->dmr_agg_flags |= DMR_AGLIM_MAX;
	    }
	    if (node->node_qen.qen_orig.orig_flag & ORIG_MINOPT)
	    {
	        dmr_cb->dmr_agg_flags |= DMR_AGLIM_MIN;
	    }
        }
	for (;;)
	{
	    /* fetch rows or call spl node for aggf optim */
	    if (action->qhd_obj.qhd_qep.ahd_qepflag & AHD_NOROWS)
	    {
		/* Negative syllogism assures no rows. */
		status = E_DB_WARN;
		dsh->dsh_error.err_code = E_QE0015_NO_MORE_ROWS;
		if (origagg)
		    dmr_cb->dmr_count = 0;	/* just to be safe */
	    }
	    else
		status = (*node->qen_func)(node, qef_rcb, dsh, open_flag);
	    open_flag &= ~(TOP_OPEN | FUNC_RESET);

	    if (status != E_DB_OK)
	    {
		if (!origagg)	/* first, handle errors for non-ORIGAGG */
		{
	            if (dsh->dsh_error.err_code == E_QE0015_NO_MORE_ROWS)
			break; 	/* end of scan, exit read loop */
		    else return(status);
		}

	        if (dsh->dsh_error.err_code == E_QE0015_NO_MORE_ROWS)
		{
		    if (dmr_cb->dmr_agg_flags & DMR_AGLIM_COUNT && data)
		    {
                        *((i4 *)data) = dmr_cb->dmr_count;
		    }
                    rowCount = dmr_cb->dmr_count;
		    dsh->dsh_qef_remnull |= ade_excb->excb_nullskip;
		    break;
		}
                if (dsh->dsh_error.err_code == E_DM0167_AGG_ADE_EXCEPTION ||
            	    dsh->dsh_error.err_code == E_DM0166_AGG_ADE_FAILED)
                {
		    adf_errcb = (ADF_ERROR *)&adfcb->adf_errcb;
		    if ((adf_errcb->ad_errclass == ADF_INTERNAL_ERROR)  ||
			(adf_errcb->ad_errclass == ADF_USER_ERROR) ||
			(adf_errcb->ad_errclass == ADF_EXTERNAL_ERROR) )
		    {
                	if ((status = qef_adf_error(&adfcb->adf_errcb, status,
				qef_cb, &dsh->dsh_error)) != E_DB_OK)
			  return (status);
		    }
                }              
		    
		return (status);
	    }
            /* From the comment below this applies only for the select  */  
            /* any constraint and should not apply to agg optim         */

	    rowCount++;

	    if (data && instr_cnt == 1)
            {
                *((i4 *)data) += 1;
	    }
            else
            { 
		/* process the aggregate */
		status = ade_execute_cx(adfcb, ade_excb);
		if (status != E_DB_OK)
		{
		    if ((status = qef_adf_error(&adfcb->adf_errcb, 
				status, qef_cb, &dsh->dsh_error)) != E_DB_OK)
			return (status);
		}
            }
	    dsh->dsh_qef_remnull |= ade_excb->excb_nullskip;
	    /* if solo any aggregate, stop */
	    if (action->qhd_obj.qhd_qep.ahd_qepflag & AHD_ANY)
		break;
	}
	
    }
    else
    {
	/* process the aggregate */
	status = ade_execute_cx(adfcb, ade_excb);
	if (status != E_DB_OK)
	{
	    if ((status = qef_adf_error(&adfcb->adf_errcb, 
		status, qef_cb, &dsh->dsh_error)) != E_DB_OK)
		return (status);
	}
	dsh->dsh_qef_remnull |= ade_excb->excb_nullskip;
    }
    /* move the result to the result location */

finitit:		/* come from ahd_constant = FALSE */
    ade_excb->excb_seg	= ADE_SFINIT;
    status = ade_execute_cx(adfcb, ade_excb);
    if (status != E_DB_OK)
    {
	if ((status = qef_adf_error(&adfcb->adf_errcb, status, 
		qef_cb, &dsh->dsh_error)) != E_DB_OK)
	    return (status);
    }

    /*
    ** The ALTER TABLE ADD CONSTRAINT state machine for RERERENTIAL CONSTRAINTS
    ** cooks up a SELECT ANY( 1 ) statement.  Subsequent states in that
    ** machine need to know whether the SELECT returned 1 or 0 rows.  The
    ** following code flags that piece of information in the session control
    ** block.
    */

    if ( qef_cb->qef_callbackStatus & QEF_EXECUTE_SELECT_STMT )
    {
	if ( rowCount > 0 )
	{   qef_cb->qef_callbackStatus |= QEF_SELECT_RETURNED_ROWS; }
	else {  qef_cb->qef_callbackStatus &= ~( QEF_SELECT_RETURNED_ROWS ); }
    }

    return (E_DB_OK);
}
