/*
**Copyright (c) 2010 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <st.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <ulf.h>
#include    <ulm.h>
#include    <uld.h>
#include    <adf.h>
#include    <ade.h>
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
#include    <qefqp.h>
#include    <qefscb.h>
#include    <qefdsh.h>
#include    <qefqeu.h>
#include    <me.h>
#include    <tm.h>
#include    <ex.h>
#include    <dmmcb.h>
#include    <dmucb.h>

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
**  Name: qentproc.c - table procedure node
**
**  Description:
**      This node drives the execution of a QP associated with a
**  table procedure. This node reads rows from the table procedure
**  and return them to the caller.
**
**          qen_tproc   - table procedure node
**
**
**  History:    $Log-for RCS$
**      24-Apr-08 (gefei01)
**          Creation.
**      08-Sep-08 (gefei01)
**          Implemeted table procedure.
**      11-Nov-08 (gefei01)
**          Fixed bug 121402. qen_tproc_exec_act():
**          Fixed infinite loop of simple aggregate in tproc.
**      17-Dec-08(gefei01)
**          Bug 121399: qen_tproc() uses the right QP ID for
**          table procedure dsh allocation.
**      12-Jan-09 (gefei01)
**          Fixed SD 132476. qen_tproc() clears qso_handle
**          to force the retrieval of tproc QP.
**      05-Mar-09 (gefei01)
**         Add qen_tproc_close() and close open tables.
**     31-Mar-09 (gefei01)
**         Bug 121870: Call qet_commit to abort to the last internal
**         savepoint when open cursor count is not zero.
**	13-May-2009 (kschendel) b122041
**	    Compiler warning fixes, need uld.h.
**      20-Oct-2009 (gefei01)
**          Fixed bug 122719. Don't return QEN4_NO_MORE_ROWS
**          for aggregation in singleton SELECT.
**	21-May-2010 (kschendel) b122755
**	    Fix action execution order to do the right thing and return
**	    the right answers, especially with nested FOR loops.
**	    Fix some nonstandard indentation.
**	18-Jun-2010 (kschendel) b122755
**	    More changes, to tproc validation this time.  They are now
**	    validated when the calling QP is validated, so by the time
**	    execution reaches the tproc node the tproc QP is known valid.
**/

/* static function */
static DB_STATUS qen_tproc_exec_act(QEF_RCB   *qef_rcb,
                                    QEN_NODE  *node,
                                    QEE_DSH   *caller_dsh,
                                    QEE_DSH   *dsh);

static DB_STATUS
qen_tproc_close(
QEN_NODE *node,
QEF_RCB  *qef_rcb, 
QEE_DSH  *caller_dsh);


/*{
** Name: qen_tproc	- Table procedure 
**
** Description:
**     This node allows for the execution of a QP associated with
** a table procedure. This node reads rows from the table procedure
** and return them to the caller by processing the actions in the QP.
** This node reads one row at a time and returns it to the caller.
**
** Inputs:
**     node                            the tproc node data structure
**     rcb                             request block
**     dsh                             caller data segment header
**     function                        execution mode
**
** Outputs:
**     rcb
**	   .error.err_code		one of the following
**					E_QE0000_OK
**					E_QE0015_NO_MORE_ROWS
**	                                E_QE0310_INVALID_TPROC_ACT
**				
**     Returns:
**	   E_DB_{OK, WARN, ERROR, FATAL}
**     Exceptions:
**	   E_QE0310_INVALID_TPROC_ACT
**
** Side Effects:
**     None.
**
** History:
**     24-Apr-2008 (gefei01)
**         Creation.
**     08-Sep-2008 (gefei01)
**         Implemented table procedure node.
**     17-Dec-08 (gefei01)
**          Bug 121399: qen_tproc() uses the right QP ID for
**          table procedure dsh allocation.
**      12-Jan-09 (gefei01)
**          Fixed SD 132476. Clear qso_handle to force the
**          retrieval of tproc QP for tproc DSH.
**	4-Jun-2009 (kschendel) b122118
**	    Use slightly faster/more convenient dsh test for qe90, and
**	    remove pointless error returns from cost begin/end.  Minor
**	    streamlining.
**	16-May-2010 (kschendel) b123565
**	    Don't set qef-dsh, might be parallel query, and not needed.
**	    The qef-cb needs to point to the main thread DSH.
**	19-May-2010 (kschendel) b123775
**	    Fix up action tracking.  If tprocs are to execute like rowprocs,
**	    they shouldn't restart at the beginning every time.
**	    Add a cancel check.
**	18-Jun-2010 (kschendel) b123775
**	    Use calling DSH's statement number since in effect we're all
**	    the same statement.  QP validation will open one tproc DSH,
**	    use it if it's unused, else create another DSH.
**/

DB_STATUS
qen_tproc(
QEN_NODE	*node,
QEF_RCB		*rcb,
QEE_DSH		*dsh,
i4		function)
{
    QEE_DSH	*tproc_dsh = (QEE_DSH *)NULL;
    QEN_STATUS	*qen_status = dsh->dsh_xaddrs[node->qen_num]->qex_status;
    QEF_CB	*qef_cb = dsh->dsh_qefcb;
    PTR		*cbs = dsh->dsh_cbs;
    QEN_TPROC	*tproc_node = &node->node_qen.qen_tproc;
    DB_STATUS	status = E_DB_OK;
    TIMERSTAT	timerstat;
    i4		val1 = 0, val2 = 0;

    /* Initialize dsh for tproc at the first execution */
    if (function & (TOP_OPEN | MID_OPEN) )
    {
	if ((qen_status->node_status_flags & QEN1_NODE_OPEN) == 0)
	{
	    QEE_PROC_RESOURCE *res;

	    /* See if validation DSH is available */
	    res = &dsh->dsh_resources[tproc_node->tproc_resix].qer_resource.qer_proc;
	    if (! res->qer_procdsh_used)
	    {
		tproc_dsh = res->qer_proc_dsh;
		res->qer_procdsh_used = TRUE;
	    }
	    else
	    {
		QEF_RCB		tproc_rcb;

		tproc_rcb = *rcb;

		/* Get tproc QP cursor ID for tproc dsh allocation */
		tproc_rcb.qef_qp = res->qer_proc_cursid;

		/* Setting qef_qso_handle to 0 will trigger
		* the use of qef_qp to get the tproc Qp from QSF,
		* instead of searching for an existing DSH.
		*/
		tproc_rcb.qef_qso_handle = NULL;

		/* Allocate tproc dsh.  Since the tproc was already validated,
		** this really shouldn't fail with "proc not found".
		** The combination of must_find == 0 with a table
		** proc QP tells qeq-dsh to get another DSH, don't reuse
		** one already in use by this query.
		*/
		status = qeq_dsh(&tproc_rcb, 0, &tproc_dsh, QEQDSH_TPROC, -1);
		if (status != E_DB_OK)
		    return status;
	    }

	    tproc_dsh->dsh_stmt_no = dsh->dsh_stmt_no;
	    qen_status->node_ahd_ptr = tproc_dsh->dsh_qp_ptr->qp_ahd;
	    qen_status->node_u.node_tproc.node_tproc_dsh = tproc_dsh;
	    qen_status->node_status_flags |= QEN1_NODE_OPEN;
	}

	if (function & MID_OPEN)
	    return E_DB_OK;
    }
    else if (function & FUNC_CLOSE)
    {
      /* Clean up dsh at CLOSE */
	if (qen_status->node_status_flags & QEN1_NODE_OPEN)
	    status = qen_tproc_close(node, rcb, dsh);
	qen_status->node_status_flags &= ~QEN1_NODE_OPEN;
 
	return status;
    }

    /* Check for cancel */
    CScancelCheck(dsh->dsh_sid);
    if (QEF_CHECK_FOR_INTERRUPT(qef_cb, dsh) == E_DB_ERROR)
	return (E_DB_ERROR);

    /* If the trace point qe90 is turned on then gather cpu and dio stats */
    if (dsh->dsh_qp_stats)
    {
	qen_bcost_begin(dsh, &timerstat, qen_status);
    }

    tproc_dsh = qen_status->node_u.node_tproc.node_tproc_dsh;

    if (function & FUNC_RESET) 
    {
	ADE_EXCB *ade_excb;
	QEN_ADF *qen_adf;

	/* Reset says start over with the first action */
	qen_status->node_ahd_ptr = tproc_dsh->dsh_qp_ptr->qp_ahd;

	/* Only evaluate tproc actual params at RESET time.
	** In any situation where they may change, such as when joining
	** to another table (or tproc), OPF will always compile a
	** CPJOIN, which will reset the (inner) tproc for each new
	** outer with potentially different params passed.
	*/
	qen_adf = node->node_qen.qen_tproc.tproc_parambuild;
	if (qen_adf != NULL)
	{
	    /* compute the actual parameters for tproc.
	    ** Note that this happens in the context of the caller DSH.
	    */
	    ade_excb = (ADE_EXCB *)cbs[qen_adf->qen_pos];
	    status = qen_execute_cx(dsh, ade_excb);
	    if (status != E_DB_OK)
		return status;
	}
        if (tproc_dsh->dsh_qp_ptr->qp_ndbp_params)
	{
            tproc_dsh->dsh_qp_status &= ~DSH_DBPARMS_DONE;

            /* Initialize tproc parameters */
            status = qee_dbparam(rcb, tproc_dsh, dsh,
                                 tproc_node->tproc_params,
 			         tproc_node->tproc_pcount,
                                 TRUE);

            if (DB_FAILURE_MACRO(status))
            {
                STRUCT_ASSIGN_MACRO(tproc_dsh->dsh_error, dsh->dsh_error);
                return status; 
            }
        }
    }

    /* Process the actions for this tproc */
    status = qen_tproc_exec_act(rcb, node, dsh, tproc_dsh);

    if (dsh->dsh_qp_stats)
    {
	qen_ecost_end(dsh, &timerstat, qen_status);
    }

    if (DB_FAILURE_MACRO(status))
        return status;

    if (status == E_DB_OK)
    {
	qen_status->node_rcount++;

	/* print tracing information DO NOT xDEBUG THIS */
	if (node->qen_prow != NULL &&
	    (ult_check_macro(&qef_cb->qef_trace, 100+node->qen_num,
				&val1, &val2) ||
		ult_check_macro(&qef_cb->qef_trace, 99,
				&val1, &val2)
	    )
	)
	{
	    (void) qen_print_row(node, rcb, dsh);
	}

        return status;
    }
    else
    {
        status = E_DB_WARN;
        dsh->dsh_error.err_code = E_QE0015_NO_MORE_ROWS;

        return status;
    }
}


/*{
** Name: qen_tproc_exec_act     - Processing the actions for a table procedure
**
** Description:
**     This routine processes the actions under a table procedure and returns
** when the row buffer is filled with a qualified row, or no more row, or
** exception occurs.
**
**	The most recently executed action is in node_ahd_ptr in the
**	QEN_STATUS.  Presumably, it's either the start of the tproc,
**	or whatever followed a RETURN ROW action.  Pick up starting
**	at the specified action, and keep going for more.  (By the way,
**	the relevant QEN_STATUS is the one for the tproc node, which is
**	accessed in the *caller* context, not the tproc context.)
**
**	The tproc actions are in effect top level actions, and as such
**	they are reset on every trip through the tproc.  A key exception
**	is the query running a FOR-loop, which must not be repositioned
**	on each execution.  As it happens, the final statement of a
**	FOR-loop (quite possibly a RETURN ROW, but need not be) points
**	back to the GET, not the FOR.  A little ascii artwork:
**
**	... FOR -> GET (flagged FOR-GET) -> IF rowcount>0
**		   ^				FALSE -> (loop exit)
**		   |				TRUE  -> act -> act --V
**		   |						      |
**		   ^--------------------------------------------------<
**
**	The act's after the IF TRUE are the FOR-loop body;  the last action
**	of the FOR-loop links back to the GET.  Nested FOR-loops work the
**	same way, with the inner-loop IF-test branching FALSE back to the
**	GET running the outer loop.
**
**	What all this means is that if we see a FOR action, then a FOR-GET
**	tagged GET action, it's the first time executing that GET and it
**	should be reset.  If a FOR-GET tagged GET is seen *without* an
**	immediately preceding FOR, it's *not* the first time, and that
**	get should not be reset.
**
**	Note that we'll still reset all the other loop body actions every
**	trip through the loop.  This is of no account for some actions (IF),
**	and is absolutely essential for loop-body GETs (which are probably
**	assignments! and if not reset, won't execute.)
**
**	If there is any other glop needed to run a FOR's GET, it will show
**	up before the FOR.  One slightly tricky one is SAGG.  The loop
**	    for select count(*) from ...
**	ends up as SAGG->FOR->GET, and the GET may not have any QP under
**	it.  Since there's no place to store state for an action with no
**	QP, GET had to be fixed up so that it would return EOF after the
**	first call even when there's no QP under the GET.  (This had not
**	been true in Ingres before now.)  Using tproc node status fails
**	because there could be nested FOR's.
**
**	Tprocs can't execute DML and Ingres doesn't have select triggers,
**	so we don't need to deal with "action state" in the same way as
**	ordinary query execution does.  A tproc will never divert to
**	rule execution.
**
**	Since tprocs run as a standard QP node, there's no need to shuffle
**	status back and forth into the qef_rcb like the qeq top level does.
**	Keep any error status in the DSH like any other QP node handler.
**
** Inputs:
**     qef_rcb                          request block
**     node                             the tproc node data structure
**     caller_dsh                       caller data segment header
**     dsh                              tproc dsh
**
** Outputs:
**     dsh
**         .dsh_error.err_code          one of the following
**                                      E_QE0000_OK
**                                      E_QE0015_NO_MORE_ROWS
**                                      E_QE0310_INVALID_TPROC_ACT
**
**     Returns:
**         E_DB_{OK, WARN, ERROR, FATAL}
**
**     Exceptions:
**         E_QE0310_INVALID_TPROC_ACT
**
** Side Effects:
**          None.
**
** History:
**     08-Sep-08 (gefei01)
**         Creation.
**     08-Sep-08 (gefei01)
**         Implemented table procedure.
**      11-Nov-08 (gefei01)
**         Bug 121402: Fixed infinite loop of simple aggregate in tproc.
**      20-Oct-2009 (gefei01)
**         Fixed bug 122719. Don't return QEN4_NO_MORE_ROWS
**         for aggregation in singleton SELECT.
**	14-May-2010 (kschendel) b123565
**	    Validate is now split into two pieces, call each one.
**	    Pass DSH to ret-row, else it will pick up the wrong one during
**	    parallel query.
**	20-May-2010 (kschendel) b123775
**	    Rework action state tracking so that the proper actions are
**	    executed in the proper order.  Fix iirowcount setting.
**	    GET now behaves better, so 121402 sagg fix no longer needed
**	    (which is good, because it didn't work if there were nested
**	    FOR-loops and the sagg driven one was the outer.)
*/

static DB_STATUS
qen_tproc_exec_act(
QEF_RCB   *qef_rcb,
QEN_NODE  *node,
QEE_DSH   *caller_dsh,
QEE_DSH   *dsh)
{
    QEF_AHD	*act;
    QEN_ADF	*qen_adf;
    ADE_EXCB	*ade_excb1;
    PTR		*cbs = dsh->dsh_cbs;
    QEF_QP_CB	*qp = dsh->dsh_qp_ptr;
    QEE_XADDRS	*caller_xaddrs = caller_dsh->dsh_xaddrs[node->qen_num];
    ADE_EXCB	*qual_excb = caller_xaddrs->qex_tprocqual;
    QEN_STATUS	*tproc_status = caller_xaddrs->qex_status;
    QEF_DATA	qef_data;
    DB_STATUS	status = E_DB_OK;
    DB_ERROR	err_blk;
    i4		*iirowcount;
    i4		toss_rowcount;
    i4		func;
    i4		rowno;
    i4		rowlen;
    i4		err;
    char	*errstr;
    bool	prevfor;

    /* set up local variables from QP */
    if (qp->qp_orowcnt_offset == -1)
        iirowcount = &toss_rowcount;	/* Simplify the code */
    else
        iirowcount = (i4 *)(dsh->dsh_row[qp->qp_rrowcnt_row] +
                            qp->qp_orowcnt_offset);

    act = tproc_status->node_ahd_ptr;
    prevfor = FALSE;
    while (act != (QEF_AHD *)NULL)
    {
	func = NO_FUNC;
	/* Reset, reopen, reinit each action, but not if it's a
	** recurring execution of a FOR-GET - see intro.
	*/
	if (prevfor || act->ahd_atype != QEA_GET
	  || (act->qhd_obj.qhd_qep.ahd_qepflag & AHD_FORGET) == 0)
	{
	    /* Note that this "validate" is a pure table-open, validation
	    ** is already done at this point by the parent QP.
	    */
	    status = qeq_topact_validate(qef_rcb, dsh, act, FALSE);
	    if (status == E_DB_OK)
	    {
		status = qeq_subplan_init(qef_rcb, dsh, act, NULL, NULL);
		if (status == E_DB_WARN && dsh->dsh_error.err_code == E_QE0015_NO_MORE_ROWS)
		{
		    /* ahd_constant CX said FALSE, don't run the action,
		    ** just skip on to the next one.
		    */
		    status = E_DB_OK;
		    dsh->dsh_error.err_code = E_QE0000_OK;
		    *iirowcount = 0;
		    act = act->ahd_next;
		    continue;
		}
	    }

	    if (status != E_DB_OK)
	    {
		if (status == E_DB_ERROR)
		    qef_error(dsh->dsh_error.err_code, 0L, status, &err,
			&dsh->dsh_error, 0);
		break;
	    }
	    func = FUNC_RESET;
        }

	switch (act->ahd_atype)
	{
	    case QEA_AGGF:
		status = qea_aggf(act, qef_rcb, dsh, func);
		break;

	    case QEA_FOR:
		prevfor = TRUE;
		break;

	    case QEA_GET:
		/* One side effect of the way we traverse actions is that
		** any GET that isn't a FOR GET is effectively a singleton
		** select.  We run it, get a row, and reset it next time
		** around.
		*/
		status = qea_fetch(act, qef_rcb, dsh, func);
		if (status == E_DB_ERROR)
		    break;
		if (dsh->dsh_qef_rowcount > 0
		  && status == E_DB_WARN && dsh->dsh_error.err_code == E_QE0015_NO_MORE_ROWS)
		{
		    /* Just flush no-more-rows status, fetch will trick it
		    ** out to enforce singleton retrievals from non-FOR-get
		    ** queries in DBP's.
		    */
		    status = E_DB_OK;
		    dsh->dsh_error.err_code = E_QE0000_OK;
		}
		*iirowcount = dsh->dsh_qef_rowcount;

		if (act->qhd_obj.qhd_qep.ahd_qepflag & AHD_FORGET)
		{
		    prevfor = FALSE;	/* We've seen the FOR-GET now */
		}

		break; 

	    case QEA_IF:
		qen_adf = act->qhd_obj.qhd_if.ahd_condition;
		if (qen_adf != NULL)
		{
		    ade_excb1 = (ADE_EXCB *)cbs[qen_adf->qen_pos];
		    status = qen_execute_cx(dsh, ade_excb1);
		    if (status != E_DB_OK)
		      break;

		    if (ade_excb1->excb_value == ADE_TRUE)
			act = act->qhd_obj.qhd_if.ahd_true;
		    else
			act = act->qhd_obj.qhd_if.ahd_false;
		}

		if (act != NULL)
		{
		    continue;	/* Instead of following next link */
		}
		break;

	    case QEA_PROJ:
		status = qea_proj(act, qef_rcb, dsh, func);
		break;

	    case QEA_RETROW:
		rowno = node->qen_row;
		rowlen = caller_dsh->dsh_qp_ptr->qp_row_len[rowno];
		qef_data.dt_next = (QEF_DATA*) NULL;
		qef_data.dt_size = rowlen;
		qef_data.dt_data = caller_dsh->dsh_row[rowno];

		dsh->dsh_qef_output = &qef_data;
		dsh->dsh_qef_nextout = NULL;

		status = qea_retrow(act, qef_rcb, dsh, func);
		if (status != E_DB_OK)
		    break;

		/* predicate qualification */
		if (qual_excb != NULL)
		{
		    status = qen_execute_cx(caller_dsh, qual_excb);
		    if (status != E_DB_OK)
			break;
		}
		break;

	    case QEA_RETURN:
		tproc_status->node_ahd_ptr = NULL;
		*iirowcount = 0;
		dsh->dsh_error.err_code = E_QE0015_NO_MORE_ROWS;
		return E_DB_WARN;

	    case QEA_SAGG:
		status = qea_sagg(act, qef_rcb, dsh, func);
		break;

	    default:
		status = E_DB_ERROR;
		errstr = uld_qef_actstr(act->ahd_atype);

		qef_error(E_QE0310_INVALID_TPROC_ACT, 0L, E_DB_FATAL,
			    &err, &err_blk, 1, STlength(errstr), errstr);
		caller_dsh->dsh_error.err_code = E_QE0122_ALREADY_REPORTED;
		break;
	}
	/* Only GET returns a row, maybe.  Everything else doesn't.
	** RETROW is a special case, if the row was accepted by
	** qualification, we'll set iirowcount to 1 below.  If the
	** row was rejected by the TPROC node qual, clear iirowcount.
	** (This way, someone could code an IF test after the RETURN ROW
	** to see if the row satisfied other qualifications;  I can't
	** imagine what use that would have, but maybe others can!)
	*/
	if (act->ahd_atype != QEA_GET)
	{
	    *iirowcount = 0;
	}


	if (DB_FAILURE_MACRO(status))
	{
	    tproc_status->node_ahd_ptr = NULL;
	    return status;
	}

	if (act->ahd_atype == QEA_RETROW &&     /* QEA_RETROW */
           (!qual_excb ||                      /* no predicate */
            qual_excb->excb_value == ADE_TRUE))/* predicate evaluated to TRUE */
	{
	    /* remember the action for next row */
	    tproc_status->node_ahd_ptr = act->ahd_next;
	    *iirowcount = 1;
	    return E_DB_OK;
	}

	act = act->ahd_next;
    } /* Action loop */

    /* The only way out of the action loop is to run off the end of
    ** the tproc, which shouldn't happen, but if it does, just sit at
    ** the end and return no-more-rows.
    */
    tproc_status->node_ahd_ptr = NULL;
    *iirowcount = 0;
    dsh->dsh_error.err_code = E_QE0015_NO_MORE_ROWS;
    return E_DB_WARN;
}


/*
** Name: qen_tproc_close     - Close table procedure resources
**
** Description:
**     This routine closes all open tables for the table procedure
**     and releases table procedure DSH.
**
** Inputs:
**     qef_rcb                          request block
**     node                             the tproc node data structure
**     caller_dsh                       caller data segment header
**
** Outputs:
**     None.
**
**     Returns:
**         E_DB_{OK, WARN, ERROR, FATAL}
**
**     Exceptions:
**         None.
**
** Side Effects:
**         None.
**
** History:
**     05-Mar-09 (gefei01)
**         Creation.
**     31-Mar-09 (gefei01)
**         Bug 121870: Call qet_commit to abort to the last internal
**         savepoint when open cursor count is not zero.
**	19-May-2010 (kschendel) b123775
**	    Drop any temp tables generated by the tproc.
**	18-Jun-2010 (kschendel) b123775
**	    Call qeq-cleanup now instead of trying to do it by hand.
**	    That will do all the right things including releasing the
**	    tproc DSH.  
*/

static DB_STATUS
qen_tproc_close(
QEN_NODE *node,
QEF_RCB  *qef_rcb, 
QEE_DSH  *caller_dsh)
{
    PTR		save_dsh;
    QEE_DSH     *tproc_dsh;
    QEE_PROC_RESOURCE *res;
    QEF_CB	*qefcb = caller_dsh->dsh_qefcb;
    QEN_STATUS  *qen_status;
    DB_STATUS    status;

    /* Find the tproc dsh */
    qen_status = caller_dsh->dsh_xaddrs[node->qen_num]->qex_status;
    tproc_dsh = qen_status->node_u.node_tproc.node_tproc_dsh;

    if (!tproc_dsh)
        return E_DB_ERROR;

    /* Probably unnecessary, but clean up the DSH ptr in the status.
    ** Also, if this was the DSH used for validating this tproc in the
    ** calling QP, clean up the resource entry.
    */
    qen_status->node_u.node_tproc.node_tproc_dsh = NULL;
    res = &caller_dsh->dsh_resources[node->node_qen.qen_tproc.tproc_resix].qer_resource.qer_proc;
    if (tproc_dsh == res->qer_proc_dsh)
    {
	res->qer_proc_dsh = NULL;
	res->qer_procdsh_used = FALSE;
	res->qer_procdsh_valid = FALSE;
    }

    /* Now cleanup and release the tproc dsh itself */
    save_dsh = qefcb->qef_dsh;
    qefcb->qef_dsh = (PTR) tproc_dsh;	/* Where qeq-cleanup wants it */
    status = qeq_cleanup(qef_rcb, E_DB_OK, TRUE);
    qefcb->qef_dsh = save_dsh;
    /* It's unclear what exactly to do with a failure of cleanup, ignore
    ** it for now, too easy to screw up unwinding.
    */

    return E_DB_OK;
}
