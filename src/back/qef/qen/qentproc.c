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
**/

/* static function */
static DB_STATUS qen_tproc_exec_act(QEF_AHD   *act,
                                    QEF_RCB   *qef_rcb,
                                    QEN_NODE  *node,
                                    QEE_DSH   *caller_dsh,
                                    QEE_DSH   *dsh);

static DB_STATUS
qen_tproc_close(
QEN_NODE *node,
QEF_RCB  *qef_rcb, 
QEE_DSH  *top_dsh);


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
**/

DB_STATUS
qen_tproc(
QEN_NODE	*node,
QEF_RCB		*rcb,
QEE_DSH		*dsh,
i4		function)
{
    QEE_DSH       *tproc_dsh = (QEE_DSH *)NULL;
    QEN_STATUS    *qen_status = dsh->dsh_xaddrs[node->qen_num]->qex_status;
    QEF_CB        *qef_cb = dsh->dsh_qefcb;
    QEN_ADF       *qen_adf = node->node_qen.qen_tproc.tproc_parambuild;
    ADE_EXCB      *ade_excb;
    PTR           *cbs = dsh->dsh_cbs;
    QEN_TPROC      tproc_node = node->node_qen.qen_tproc;
    QEF_AHD       *first_act;
    DB_STATUS      status = E_DB_OK;
    TIMERSTAT	    timerstat;
    i4		   val1 = 0, val2 = 0;

    /* Initialize dsh for tproc at the first execution */
    if (function & (TOP_OPEN | MID_OPEN))
    {
	DB_CURSOR_ID	tproc_qp_id;
	QEF_RCB		tproc_rcb;

	tproc_rcb = *rcb;

	/* Get tproc QP cursor ID for tproc dsh allocation */
	MEcopy((PTR)tproc_node.tproc_qpidptr,
             sizeof(DB_CURSOR_ID),
             (PTR)&tproc_rcb.qef_qp);

	/* Setting qef_qso_handle to 0 will trigger
	* the use of qef_qp to get the tproc Qp instead of
	* the top QP.
	*/
	tproc_rcb.qef_qso_handle = 0;

	/* Allocate tproc dsh */
	status = qeq_dsh(&tproc_rcb, 0, &tproc_dsh, (bool)FALSE,
		(i4)-1, (bool)TRUE);
	if (status != E_DB_OK)
	    return status;

	qen_status->node_u.node_tproc.node_tproc_dsh = tproc_dsh;

	tproc_dsh->dsh_act_ptr = tproc_dsh->dsh_qp_ptr->qp_ahd;

	if (function & MID_OPEN)
	    return status;
    }
    else if (function & FUNC_CLOSE)
    {
      /* Clean up dsh at CLOSE */
      status = qen_tproc_close(node, rcb, dsh);
 
      return status;
    }

    /* If the trace point qe90 is turned on then gather cpu and dio stats */
    if (dsh->dsh_qp_stats)
    {
	qen_bcost_begin(dsh, &timerstat, qen_status);
    }

    if (qen_adf != NULL)
    {
      /* compute the actual parameters for tproc */
      ade_excb = (ADE_EXCB *)cbs[qen_adf->qen_pos];
      ade_excb->excb_seg = ADE_SMAIN;
      status = ade_execute_cx(dsh->dsh_adf_cb, ade_excb);
      if (status != E_DB_OK)
      {
        status = qef_adf_error(&dsh->dsh_adf_cb->adf_errcb, status,
                               qef_cb, &dsh->dsh_error);
        if (status != E_DB_OK)
          return status;
      } 
    }

    qen_status = dsh->dsh_xaddrs[node->qen_num]->qex_status;
    tproc_dsh = qen_status->node_u.node_tproc.node_tproc_dsh;

    if (function & FUNC_RESET) 
    {
        tproc_dsh->dsh_act_ptr = tproc_dsh->dsh_qp_ptr->qp_ahd;

        if (tproc_dsh->dsh_qp_ptr->qp_ndbp_params)
	{
            tproc_dsh->dsh_qp_status &= ~DSH_DBPARMS_DONE;

            /* Initialize tproc parameters */
            status = qee_dbparam(rcb, tproc_dsh, dsh,
                                 tproc_node.tproc_params,
 			         tproc_node.tproc_pcount,
                                 TRUE);

            if (DB_FAILURE_MACRO(status))
            {
                STRUCT_ASSIGN_MACRO(tproc_dsh->dsh_error, dsh->dsh_error);
                qef_cb->qef_open_count--;
                return status; 
            }
        }
	/* ***** FIXME not good enough, need to pass reset to actions.
	** and needs to start over at the first action.
	** and non-reset really ought to take up where we left off from
	** the most recent return-row rather than always starting over.
	** tproc in general doesn't traverse actions quite right,
	** it mostly works, but not always.
	*/
    }

    /* Process the actions for this tproc */
    first_act = tproc_dsh->dsh_act_ptr;

    status = qen_tproc_exec_act(first_act, rcb, node, dsh, tproc_dsh);

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
** Inputs:
**     act                              action header
**     qef_rcb                          request block
**     node                             the tproc node data structure
**     caller_dsh                       caller data segment header
**     dsh                              tproc dsh
**
** Outputs:
**     qef_rcb
**         .error.err_code              one of the following
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
*/
static DB_STATUS
qen_tproc_exec_act(
QEF_AHD   *act,
QEF_RCB   *qef_rcb,
QEN_NODE  *node,
QEE_DSH   *caller_dsh,
QEE_DSH   *dsh)
{
    QEN_ADF     *qen_adf;
    ADE_EXCB    *ade_excb1;
    PTR         *cbs = dsh->dsh_cbs;
    QEF_QP_CB   *qp = dsh->dsh_qp_ptr;
    QEE_XADDRS  *node_xaddrs = caller_dsh->dsh_xaddrs[node->qen_num];
    ADE_EXCB    *ade_excb2 = node_xaddrs->qex_tprocqual;
    QEN_STATUS  *qen_status = caller_dsh->dsh_xaddrs[node->qen_num]->qex_status;
    QEF_DATA     qef_data;
    DB_STATUS    status = E_DB_OK;
    DB_ERROR     err_blk;
    i4           act_state = DSH_CT_INITIAL;
    i4          *iirowcount;
    i4           rowno;
    i4           rowlen;
    i4           err;
    char        *errstr;
    bool         prevreset, act_reset = TRUE;
    bool         prevfor = FALSE;
    bool         is_sag = FALSE;

    /* set up local variables from QP */
    if (qp->qp_orowcnt_offset == -1)
        iirowcount = (i4 *)NULL;
    else
        iirowcount = (i4 *)(dsh->dsh_row[qp->qp_rrowcnt_row] +
                            qp->qp_orowcnt_offset);

    while (act != (QEF_AHD *)NULL)
    {
        qeq_rcbtodsh(qef_rcb, dsh);

        /*
         * Validate (and initialize) the action if this is the first
         * invocation of this action. act_state is constrained to be
         * DSH_CT_INITIAL only on the first call to the action. If we
         * are executing a for (select) loop in a procedure, we only 
         * validate the select GET on entry to the loop (so as not to
         * reposition the query for each iteration).
	 * FIXME this is still goofy, revalidates on each returned row
	 * because tproc starts over from the beginning each call.
	 * Teach tproc to track its state better!
         */
        if (act_state == DSH_CT_INITIAL &&
            (prevfor || act->ahd_atype != QEA_GET ||
             !(act->qhd_obj.qhd_qep.ahd_qepflag & AHD_FORGET)) )
	{
	    status = qeq_topact_validate(qef_rcb, dsh, act, TRUE);
	    if (status == E_DB_OK)
		status = qeq_subplan_init(qef_rcb, dsh, act, NULL, NULL);

	    if (status != E_DB_OK)
	    {
		qeq_dshtorcb(qef_rcb, dsh);
		qef_error(qef_rcb->error.err_code, 0L, status, &err,
			&qef_rcb->error, 0);
		break;
	    }
        }

        /* Don't allow for-loop get's to restart query on each iteration */
        prevreset = act_reset;

        if (!prevfor && act->ahd_atype == QEA_GET &&
            act->qhd_obj.qhd_qep.ahd_qepflag & AHD_FORGET)
        {
           act_reset = FALSE;
           status = E_DB_OK;
        }
        prevfor = FALSE;

        if (!(act_state == DSH_CT_INITIAL && status))
        {
            switch (act->ahd_atype)
	    {
	        case QEA_AGGF:
		    status = qea_aggf(act, qef_rcb, dsh, (i4)NO_FUNC);
		    break;
                case QEA_FOR:
                    prevfor = TRUE;
                    break;

                case QEA_GET:
		    if (qen_status->node_status == QEN4_NO_MORE_ROWS)
		    {
		        caller_dsh->dsh_error.err_code = E_QE0015_NO_MORE_ROWS;
                        status = E_DB_WARN;
                        qen_status->node_status = QEN0_INITIAL;

                        return status;
                    }

                    status = qea_fetch(act, qef_rcb, dsh,
                                       act_reset? FUNC_RESET : NO_FUNC);
                    act_reset = prevreset;
                    qeq_dshtorcb(qef_rcb, dsh);

                    if (is_sag && prevfor)
                       qen_status->node_status = QEN4_NO_MORE_ROWS;

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
                        qeq_dshtorcb(qef_rcb, dsh); 
                        continue;
                    }
                    break;

	        case QEA_PROJ:
		    status = qea_proj(act, qef_rcb, dsh, (i4)NO_FUNC);
                    break;

                case QEA_RETROW:
                     rowno = node->qen_row;
                     rowlen = caller_dsh->dsh_qp_ptr->qp_row_len[rowno];
                     qef_data.dt_next = (QEF_DATA*) NULL;
                     qef_data.dt_size = rowlen;
                     qef_data.dt_data = caller_dsh->dsh_row[rowno];

                     dsh->dsh_qef_output = &qef_data;

                     status = qea_retrow(act, qef_rcb, dsh,
                                         (act_reset) ? FUNC_RESET : NO_FUNC);

                     if (status != E_DB_OK)
		       goto errexit;

                     /* predicate qualification */
                     if (ade_excb2 != NULL)
                     {
                         if (status = qen_execute_cx(caller_dsh, ade_excb2))
                             goto errexit;

                     }

                     qeq_dshtorcb(qef_rcb, dsh);

                     if (status != E_DB_OK)
		       goto errexit;

                     break;

                case QEA_RETURN:
                     return E_DB_WARN;

                case QEA_SAGG:
 		     is_sag = TRUE;

                     status = qea_sagg(act, qef_rcb, dsh,
                                       (act_reset) ? FUNC_RESET : NO_FUNC);
                     break;

                default:
                    status = E_DB_ERROR;
                    errstr = uld_qef_actstr(act->ahd_atype);

                    qef_error(E_QE0310_INVALID_TPROC_ACT, 0L, E_DB_FATAL,
                                &err, &err_blk, 1, STlength(errstr), errstr);
                    caller_dsh->dsh_error.err_code = E_QE0122_ALREADY_REPORTED;
                    break;
            }
       }

       if (DB_FAILURE_MACRO(status))
           return status;

       if (iirowcount != (i4 *)NULL)
           *iirowcount = (i4) (qef_rcb->qef_rowcount);

       act_state = DSH_CT_INITIAL;
       act_reset = TRUE;


       if (act->ahd_atype == QEA_RETROW &&     /* QEA_RETROW */
           (!ade_excb2 ||                      /* no predicate */
            ade_excb2->excb_value == ADE_TRUE))/* predicate evaluated to TRUE */
       {
          /* remember the action for next row */
          dsh->dsh_act_ptr = act->ahd_next;
          break; 
       }

       act = act->ahd_next;
  }

 errexit:

  return status;
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
*/

static DB_STATUS
qen_tproc_close(
QEN_NODE *node,
QEF_RCB  *qef_rcb, 
QEE_DSH  *top_dsh)
{
    QEE_DSH     *tproc_dsh;
    QEN_STATUS  *qen_status;
    DMT_CB      *dmt_cb;
    DB_STATUS    status;
    i4           open_count;
    i4           err;

    /* Find the tproc dsh */
    qen_status = top_dsh->dsh_xaddrs[node->qen_num]->qex_status;
    tproc_dsh = qen_status->node_u.node_tproc.node_tproc_dsh;

    if (!tproc_dsh)
        return E_DB_ERROR;

    /* Set DSH_TPROC_DSH to the top dsh so qeq_cleanup
     * knows it is a tproc query and calls qet_commit
     * regardless of the open cursor count.
     */
    if (!(top_dsh->dsh_qp_status & DSH_TPROC_DSH)) 
        top_dsh->dsh_qp_status |= DSH_TPROC_DSH;

    /* Close all the tables opened by this tproc */
    for (dmt_cb = tproc_dsh->dsh_open_dmtcbs; dmt_cb;
         dmt_cb = (DMT_CB *)(dmt_cb->q_next))
    {
        if (dmt_cb->dmt_record_access_id == (PTR) NULL)
            continue;

	/* Could use closeAndUnlink, but release-resources will nuke the
	** entire list anyway, don't bother  (it also cleans record-access-id)
	*/
        status = dmf_call(DMT_CLOSE, dmt_cb);
        if (status)
        {
    	    qef_error(dmt_cb->error.err_code, 0L, status, &err,
	    	      &qef_rcb->error, 0);

            return status;
        }
	/* Possible FIXME?  what about deleting temporary tables? */
    }

    /* release tproc dsh */
    qeq_release_dsh_resources(tproc_dsh);

    return E_DB_OK;
}
