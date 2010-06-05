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
**  Name: QEARETROW.C - return result rows from a dbproc.
**
**  Description:
**      The return row statement returns a result row image from local
**	variables/expressions computed in the database procedure.
**
**          qea_retrow - build result rows from dbproc.
**
**
**  History:    $Log-for RCS$
**      28-oct-98 (inkdo01)
**          written
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
** Name: QEA_RETROW - build a result row image for a database procedure
**
** Description:
**	This function is called to build a result row in a database 
**	procedure. Since the corresponding action can be contained in 
**	a loop (for, while or repeat), multiple rows may ultimately
**	be returned from one call to the procedure - one per call to 
**	this function. There may even be multiple return row action
**	headers in the same query plan, though they must all build
**	identically formatted result rows. 
**
**	Output buffer position tracking is a little different from the
**	usual GET tracking, partly because RETROW's can be interspersed
**	with various other actions in the row-producing DBP, and those
**	other actions may change the usual rowcount variables.
**	RETROW has three variables in the DSH that are used to track
**	the current buffer position.  "dsh_rpp_rowmax" is the number of
**	rows allows to be returned, as passed in from SCF;  this is
**	set upon (re)entry to the rowproc, although not when re-entering
**	QEF after a dependent rule/DBP re-parse/re-load.
**	When dsh_rowmax is set, the other two counters, dsh_rpp_rows and
**	dsh_rpp_bufs, are cleared.  "dsh_rpp_rows" counts rows emitted by
**	RETROW actions, and "dsh_rpp_bufs" counts output buffers used.
**	(The two are usually the same, but when blobs are around,
**	a single row might take up multiple output buffers.)
**	The top level of QEF (qeq_query) copies dsh_rpp_rows and
**	dsh_rpp_bufs back out to qef_rowcount and qef_count respectively,
**	just where SCF wants them.
**
**	The current output buffer could be computed from dsh_rpp_bufs,
**	but it's easier to track the next buffer to use in dsh_qef_nextout.
**	If dsh_qef_nextout isn't set yet, it must be the first row emitted
**	since SCF gave us a fresh set of output buffers, and we'll use
**	dsh_qef_output (i.e. qef_rcb->qef_output) for the first time.
**
**	Note that if the rowproc is interrupted back to SCF to load a
**	rule DBP, or for any reason other than "output buffer full",
**	SCF is careful to not mess with the output buffers.
**	There can only be one top-level row-producing procedure, so
**	there's no danger of anything else messing with the output
**	in progress.
**
**	NOTE: this function was cloned from qea_fetch. The differences in 
**	which "return row" actions are executed demanded a separation of
**	logic from qea_fetch.
**
**	Interestingly enough, the separation of rows emitted vs buffers
**	used ends up being rather strained, because SCF forgets to make the
**	distinction upon input!  We can make it work by checking the next-
**	buffer link rather than counting buffers.
**
** Inputs:
**      action				fetch action item
**      qef_rcb                         the qef request block
**	function			reset flags, not used
**
** Outputs:
**      dsh
**	    .dsh_rpp_rows		actual number of rows returned
**	    .dsh_rpp_bufs		number of buffers filled
**	    .dsh_qef_nextout		next output to use
**	    .dsh_error.err_code		one of the following
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
**      28-oct-98 (inkdo01)
**          written
**	29-dec-03 (inkdo01)
**	    Changed reset parm to function.
**	28-feb-04 (inkdo01)
**	    Change qef_rcb parm to dsh in qeq_chk_resources call.
**	2-apr-04 (inkdo01)
**	    Fix to DSH changes (not updating output buffer list).
**	26-aug-04 (inkdo01)
**	    Add global base array support for return row results.
**	13-Aug-2005 (schka24)
**	    Rearrange output buffer tracking variables, revise comments
**	    to match current reality.
**	1-May-2006 (kschendel)
**	    Set blob base properly when doing global base arrays.
**	16-May-2010 (kschendel) b123565
**	    Caller must pass DSH to make parallel query + table procs
**	    work properly.
*/
DB_STATUS
qea_retrow(
QEF_AHD		*action,
QEF_RCB		*qef_rcb,
QEE_DSH		*dsh,
i4		function )
{
    QEF_CB	*qef_cb = qef_rcb->qef_cb;
    QEF_DATA	*output;
    i4 		bufs_used;
    DB_STATUS	status = E_DB_OK;
    QEN_ADF	*qen_adf;
    ADE_EXCB	*ade_excb;

#ifdef xDEBUG
    (VOID) qe2_chk_qp(dsh);
#endif

    /* Check if resource limits are exceeded by this query */
    if (action->qhd_obj.qhd_qep.ahd_estimates && (qef_cb->qef_fl_dbpriv &
        (DBPR_QROW_LIMIT | DBPR_QDIO_LIMIT | DBPR_QCPU_LIMIT |
        DBPR_QPAGE_LIMIT | DBPR_QCOST_LIMIT))
        )
    {
        if (qeq_chk_resource(dsh, action) != E_DB_OK)
	    return (E_DB_ERROR);
    }

    /* initialize the output buffer */
    qen_adf = action->qhd_obj.qhd_qep.ahd_current;
    ade_excb = (ADE_EXCB*) dsh->dsh_cbs[qen_adf->qen_pos];	

    /* Next output buffer to use is "nextout" unless it's not set yet. */
    output = dsh->dsh_qef_nextout;
    if (output == (QEF_DATA *)NULL)
	output = dsh->dsh_qef_output;

    bufs_used = 0;
    
    /* Materialize the row.  Usually just once around the FOR loop,
    ** unless something odd happens with blobs.
    */
    for (;;)
    {
	if (ade_excb->excb_seg == ADE_SFINIT)
	{
	    dsh->dsh_error.err_code = E_QE0015_NO_MORE_ROWS;
	    ade_excb->excb_seg = ADE_SMAIN;
	    status = E_DB_WARN;
	}
	else
	{
	    /* materialize row into output buffer.
	    ** Aim output row (uoutput) at caller supplied output buffer.
	    ** If there are blobs, tell adf that it's the blob area.
	    */
	    ade_excb->excb_seg = ADE_SMAIN;
	    ade_excb->excb_limited_base = ADE_NOBASE;
	    if (qen_adf->qen_uoutput >= 0)
	    {
		if (qen_adf->qen_mask & QEN_HAS_PERIPH_OPND)
		    ade_excb->excb_limited_base = qen_adf->qen_uoutput;
		if (dsh->dsh_qp_ptr->qp_status & QEQP_GLOBAL_BASEARRAY)
		{
		    dsh->dsh_row[qen_adf->qen_uoutput] = output->dt_data;
		}
		else
		{
		    /* old-fashioned way, uoutput is local base, bias it
		    ** for blobs, set output pointer where old way wants it
		    */
		    if (qen_adf->qen_mask & QEN_HAS_PERIPH_OPND)
			ade_excb->excb_limited_base += ADE_ZBASE;
		    ade_excb->excb_bases[ADE_ZBASE+qen_adf->qen_uoutput] =
							    output->dt_data;
		}
		ade_excb->excb_size = output->dt_size;
	    }

	    /* process the tuples */

	    status = ade_execute_cx(dsh->dsh_adf_cb, ade_excb);
	    if (status != E_DB_OK)
	    {
#ifdef xDEBUG
		(VOID) qe2_chk_qp(dsh);
#endif
		if ((status == E_DB_INFO) &&
			    (dsh->dsh_adf_cb->adf_errcb.ad_errcode ==
					E_AD0002_INCOMPLETE))
		{
		    /* Caught in mid-BLOB. */
		    dsh->dsh_error.err_code = E_AD0002_INCOMPLETE;
		    ++ bufs_used;
		    if (qen_adf->qen_uoutput >= 0)
		    {
			output->dt_size = ade_excb->excb_size;
			ade_excb->excb_limited_base = ADE_NOBASE;
			if (output->dt_next != NULL)
			{
			    output = output->dt_next;
			    /* There's room left, loop back and output more */
			    continue;
			}
		    }
		    break;
		}
		else if ((status = qef_adf_error(
			    &dsh->dsh_adf_cb->adf_errcb, status,
			    qef_cb, &dsh->dsh_error)) != E_DB_OK)
		{
		    break;
		}
	    }
	    bufs_used += 1;

	    if (qen_adf->qen_uoutput >= 0)
	    {
		output->dt_size = ade_excb->excb_size;
		ade_excb->excb_limited_base = ADE_NOBASE;
	    }
	}
	break;
    }

    /* Don't leave bogus error indication if we managed to fit it all */
    if (status == E_DB_OK)
	dsh->dsh_error.err_code = 0;

    /* Update counts, buffer addresses. */
    ++ dsh->dsh_rpp_rows;			/* Always just one row */
    dsh->dsh_rpp_bufs += bufs_used;
    dsh->dsh_qef_nextout = output->dt_next;
#ifdef xDEBUG
    (VOID) qe2_chk_qp(dsh);
#endif
    return (status);
}
