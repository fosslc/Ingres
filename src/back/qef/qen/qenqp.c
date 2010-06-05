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
**  Name: QEN_QP.C - query plan QEP node
**
**  Description:
**      This node allows for recursive execution of a QP. This node
**  reads tuples from another QP into the current QP. 
**
**          qen_qp   - query plan node
**
**
**  History:    $Log-for RCS$
**      9-sep-86 (daved)
**          written
**	02-feb-88 (puree)
**	    add reset flag to qen_qp function arguments.
**	11-feb-88 (puree)
**	    fix bug associated with reset (qea_raaggf calls).
**	30-mar-88 (puree)
**	    reset the reset flag after each call to qea_.
**	03-aug-88 (puree)
**	    fix error in ult_check_macro call.
**	26-aug-88 (puree)
**	    fix qen_qp.  The current action id (node_ahd_ptr) must be reset
**	    to null when the reset flag is set.
**	11-oct-88 (puree)
**	    fix qen_qp.  Return E_QE0015_NO_MORE_ROWS for get and aggregate
**	    functions.
**	13-jun-89 (paul)
**	    Correct bug 6346. Do not reset local variables in a DB procedure
**	    after a COMMIT or ROLLBACK action. Only need to change the
**	    parameters to qeq_validate in this module.
**      04-jan-90 (davebf)
**          pass reset to actions after first one when this function called
**	    with reset.  Uses qen_status.node_u.join.node_inner_status (which has
**	    no other use in this function) to save reset status until
**	    all actions are completed -- or error occurs.
**	13-jun-90 (davebf)
**	    insure rcb->qef_rowcount is 0 when returning "no_more_rows"
**	13-jun-90 (davebf)
**	    execute subsequent actions when one action fails by
**	    constant qualification (in qeq_validate).  this cancels
**	    11-oct-88 above.
**	11-mar-92 (jhahn)
**	    Added support for QEF interrupts.
**	    Added call to CSswitch.
**	08-dec-92 (anitap)
**	    Included <qsf.h> and <psfparse.h> for CREATE SCHEMA.
**      14-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
**	7-nov-95 (inkdo01)
**	    Changes to replace QEN_ADF structure instances by pointers in
**	    QEN_QP.
**	23-feb-04 (inkdo01)
**	    Changed qef_rcb->error to dsh->dsh_error for || thread safety
**	    (and others).
**	28-Dec-2005 (kschendel)
**	    Use DSH references whenever possible instead of QEF RCB
**	    references.  (This simplifies the code, eliminating many
**	    RCB references, and reflects the new primacy of the DSH
**	    thanks to the parallel query changes.)
**      18-Mar-2010 (hanal04) Bug 123443
**          EXCH threads will not be created if QE71 is set. Skip
**          parallel union processing if we are not going to create
**          the parallel threads.
[@history_template@]...
**/


/*{
** Name: QEN_QP	- query plan node
**
** Description:
**      This node allows for recursive execution of a QP. This node
**  reads tuples from another QP into the current QP. 
**  The tuples returned by this node are not in any particular order.
**  That is, this node should be treated as heap or heapsort if the
**  query compiler can guarantee that the reading of the actions in the
**  associated QP will return rows in sorted order.
**
**  This node will initialize each action and read rows from it until done.
**
** Inputs:
**      node                            the QP node data structure
**      rcb                             request block
**
** Outputs:
**      rcb
**	    .error.err_code		one of the following
**					E_QE0000_OK
**					E_QE0015_NO_MORE_ROWS
**					E_QE0019_NON_INTERNAL_FAIL
**					E_QE0002_INTERNAL_ERROR
**					
**	Returns:
**	    E_DB_{OK, WARN, ERROR, FATAL}
**	Exceptions:
**	    none
**
** Side Effects:
**	    All values in the appropriate row buffers have
**	been set to that which will produce the joined row.
**
** History:
**      09-sep-86 (daved)
**          written
**	12-mar-87 (daved)
**	    add a qualification to this node.
**	02-feb-88 (puree)
**	    add reset flag to the function arguments.
**	11-feb-88 (puree)
**	    fix bug associated with reset and first time call to 
**	    qea_raggf.
**	30-mar-88 (puree)
**	    reset the reset flag after each call to qea_.
**	03-aug-88 (puree)
**	    fix error in ult_check_macro call.
**	26-aug-88 (puree)
**	    The current action id (node_ahd_ptr) must be reset
**	    to null when the reset flag is set.
**	11-oct-88 (puree)
**	    The following actions must return E_QE0015_NO_MORE_ROWS:
**		QEA_GET
**		QEA_SAGG
**		QEA_RAGGF
**		QEA_AGGF
**	    if it cannot position in qeq_validation.
**	28-sept-89 (eric)
**	    Incremented qen_status->node_rcount to keep track of how many
**	    rows this node returns. This will be used to fix the Sybase bug.
**	31-oct-89 (nancy)
**	    Bug 8538 where insert as select aggregate where 1=0 will AV
**	    in qeq_validate -- qef_output is 0.  This is because the 
**	    constant qual will fail, but the aggregate cx is executed anyway.
**	    Initialize qef_output to qef_data before calling qeq_validate.
**      04-jan-90 (davebf)
**          pass reset to actions after first one when this function called
**	    with reset.  Uses qen_status.node_u.join.node_inner_status (which has
**	    no other use in this function) to save reset status until
**	    all actions are completed -- or error occurs.
**	13-jun-90 (davebf)
**	    insure dsh->dsh_qef_rowcount is 0 when returning "no_more_rows"
**	13-jun-90 (davebf)
**	    execute subsequent actions when one action fails by
**	    constant qualification (in qeq_validate).  this cancels
**	    11-oct-88 above.
**	11-mar-92 (jhahn)
**	    Added support for QEF interrupts.
**	    Added call to CSswitch.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	7-nov-95 (inkdo01)
**	    Changes to replace QEN_ADF structure instances by pointers in
**	    QEN_QP.
**	28-jun-96 (kch)
**	    Initialize rcb->qef_output to qef_data before the call to
**	    qeq_validate() regardless of whether qef_output is null. This
**	    change fixes bug 77348.
**	26-jan-01 (inkdo01)
**	    Changes to support hash aggregation (QEA_HAGGF).
**	22-mar-01 (inkdo01)
**	    Add a couple of more HAGGF checks.
**	23-apr-01 (inkdo01)
**	    Close hash agg files after bad status.
**	20-nov-03 (inkdo01)
**	    Add support for "open" call to allow parallel query initialization.
**	29-dec-03 (inkdo01)
**	    DSH is now parameter, "function" replaces "reset".
**	27-feb-04 (inkdo01)
**	    Changed dsh_hashagg from array of QEN_HASH_AGGREGATE to array
**	    of ptrs (same for QEN_STATUS array).
**	9-mar-04 (inkdo01)
**	    Enable parallel union execution by qen_qp.
**	17-mar-04 (toumi01)
**	    Add support for "close" call to allow parallel query termination.
**	1-apr-04 (inkdo01)
**	    Save dsh_qef_rowcount around qen_qp execution.
**	8-apr-04 (inkdo01)
**	    Added dsh parm to QEF_CHECK_FOR_INTERRUPT macro.
**	11-june-04 (inkdo01)
**	    Fix assignment of actions to || threads.
**	24-june-04 (inkdo01)
**	    Add QEA_SAGG to row returning actions - fixes looong time 
**	    bug in qe90 processing.
**	28-july-04 (inkdo01)
**	    Add logic to handle FUNC_CLOSE under || union.
**	04-Nov-2004 (jenjo02)
**	    Only CSswitch on root DSH or if not OS-threaded.
**	16-Feb-2006 (kschendel)
**	    Return after close, don't keep going.
**	6-sep-2006 (dougi)
**	    Multiple SEjoins may address the same QP node, so MID_OPEN 
**	    test should return immediately even when QEN1_NODE_OPEN is on.
**      21-Nov-2006 (huazh01)
**          reset the table each time we start fetching rows from
**          the QEN_QP node, this allows qen_orig() to make a position
**          call before it starts fetching rows from the table.
**          This fixes bug 117169.
**      13-Feb-2007 (kschendel)
**          Replace CSswitch with better cancel check.
**	4-Jun-2009 (kschendel) b122118
**	    Use slightly faster/more convenient dsh test for qe90, and
**	    remove pointless error returns from cost begin/end.  Minor
**	    streamlining.  Close hashagg files as part of FUNC_CLOSE,
**	    since nobody else will do it.
**	17-Nov-2009 (kschendel) SIR 122890
**	    Load now done by qea-put.
**	11-May-2010 (kschendel)
**	    NODEACT flag now guarantees that there's really a QP underneath.
*/
DB_STATUS
qen_qp(
QEN_NODE	*node,
QEF_RCB		*rcb,
QEE_DSH		*dsh,
i4		function)
{
    DB_STATUS		status	= E_DB_OK;
    QEF_CB		*qef_cb = dsh->dsh_qefcb;
    QEF_AHD		*act;
    QEF_AHD		*cur_act;
    QEE_XADDRS		*node_xaddrs = dsh->dsh_xaddrs[node->qen_num];
    QEN_STATUS		*qen_status = node_xaddrs->qex_status;
    ADE_EXCB		*jqual_excb;
    PTR			old_output;
    QEF_DATA		qef_data;
    bool		reset = ((function & FUNC_RESET) != 0);
    bool		first_time = FALSE;
    i4			rowno;
    i4			rowlen;
    i4			saved_rowcount = dsh->dsh_qef_rowcount;
    i4	    val1;
    i4	    val2;
    i4			i;
    TIMERSTAT	    timerstat;

    /* Do open processing, if required. Only if this is the root node
    ** of the query tree do we continue executing the function. */
    if ((function & TOP_OPEN &&
	!(qen_status->node_status_flags & QEN1_NODE_OPEN)) || 
	function & MID_OPEN) 
    {
	qen_status->node_status_flags |= QEN1_NODE_OPEN;
	if (function & MID_OPEN)
	    return(E_DB_OK);
	function &= ~TOP_OPEN;
    }
    /* Do close processing, if required. */
    /* Note that hash agg action needs to be closed specifically,
    ** to clean up temp files.  Other actions (e.g. RFAGG) use the
    ** dsh "temp table" list for cleaning up.  If the action ran to
    ** completion, we probably cleaned up already, but if the query
    ** stopped early (first N or existence-check or interrupt), not.
    ** FIXME really should call qea-haggf with FUNC_CLOSE in case it
    ** wants to do other stuff too, but no big deal.
    */
    if (function & FUNC_CLOSE)
    {
	if (qen_status->node_status_flags & QEN8_NODE_CLOSED)
	    return (E_DB_OK);
	if ((node->node_qen.qen_qp.qp_flag & QP_PARALLEL_UNION) &&
            !(ult_check_macro(&qef_cb->qef_trace, QEF_TRACE_EXCH_SERIAL_71, &val1, &val2)))
	{
	    act = node->node_qen.qen_qp.qp_act;
	    for (i = 1; i < dsh->dsh_threadno; i++)
		act = act->ahd_next;
				/* find our action header */
	    if (act != NULL && act->ahd_flags & QEA_NODEACT)
	    {
		node = act->qhd_obj.qhd_qep.ahd_qep;
		status = (*node->qen_func)(node, rcb, dsh, FUNC_CLOSE);
		if (status != E_DB_OK)
		    return(status);
	    }
	    /* Close hashagg too if that is the action. */
	    if (act->ahd_atype == QEA_HAGGF)
	    {
		QEN_HASH_AGGREGATE	*haptr = 
			dsh->dsh_hashagg[act->qhd_obj.qhd_qep.u1.s2.ahd_agcbix];

		(void) qea_haggf_closeall(haptr);
	    }
	}
	else for (act = node->node_qen.qen_qp.qp_act; act != NULL;
			act = act->ahd_next)
	{
	    if (act->ahd_flags & QEA_NODEACT)
	    {
		node = act->qhd_obj.qhd_qep.ahd_qep;
		status = (*node->qen_func)(node, rcb, dsh, FUNC_CLOSE);
		if (status != E_DB_OK)
		    return(status);
	    }
	    /* Close hashagg too if that is the action. */
	    if (act->ahd_atype == QEA_HAGGF)
	    {
		QEN_HASH_AGGREGATE	*haptr = 
		    dsh->dsh_hashagg[act->qhd_obj.qhd_qep.u1.s2.ahd_agcbix];

		(void) qea_haggf_closeall(haptr);
	    }
	}
	qen_status->node_status_flags &= ~QEN1_NODE_OPEN;
	qen_status->node_status_flags |= QEN8_NODE_CLOSED;
	return(E_DB_OK);
    }


    /* Check for cancel, context switch if not MT */
    CScancelCheck(dsh->dsh_sid);
    if (QEF_CHECK_FOR_INTERRUPT(qef_cb, dsh) == E_DB_ERROR)
	return (E_DB_ERROR);

    /* If the trace point qe90 is turned on then gather cpu and dio stats */
    if (dsh->dsh_qp_stats)
    {
	qen_bcost_begin(dsh, &timerstat, qen_status);
    }


    cur_act	    = dsh->dsh_act_ptr;
    rowno	    = node->qen_row;
    rowlen	    = dsh->dsh_qp_ptr->qp_row_len[rowno];
    qef_data.dt_next = (QEF_DATA*) NULL;
    qef_data.dt_size = rowlen;
    qef_data.dt_data = dsh->dsh_row[rowno];
    jqual_excb = node_xaddrs->qex_jqual;


#ifdef xDEBUG
    (VOID) qe2_chk_qp(dsh);
#endif
    if (reset)
    {
	act = (QEF_AHD *)NULL;
	qen_status->node_status = QEN0_INITIAL;
	/* indicate reset status for subsequent calls */
	qen_status->node_u.node_join.node_inner_status = QEN10_ROWS_AVAILABLE;
    }
    else
    {
	act = (QEF_AHD*) qen_status->node_ahd_ptr;
    }
    dsh->dsh_act_ptr = act;

    /* process fetch and aggregate actions */
    if (act && 
	(act->ahd_atype == QEA_SAGG || 
	 act->ahd_atype == QEA_GET || 
	 act->ahd_atype == QEA_RAGGF ||
	 act->ahd_atype == QEA_HAGGF))
    {
	for (;;)
	{
	    if (qen_status->node_status == QEN4_NO_MORE_ROWS)
	    {
		dsh->dsh_error.err_code = E_QE0015_NO_MORE_ROWS;
		status = E_DB_WARN;
		dsh->dsh_qef_rowcount = 0;   /* be sure not left over */
		/* reset the node status for the next call */
		qen_status->node_status = QEN0_INITIAL;
		break;
	    }
	    /* ask for one row */
	    dsh->dsh_qef_rowcount = 1;
	    old_output = (PTR) dsh->dsh_qef_output;
	    dsh->dsh_qef_output = &qef_data;
	    if (act->ahd_atype == QEA_GET)
		status = qea_fetch(act, rcb, dsh, function);
	    else if (act->ahd_atype == QEA_HAGGF)
		status = qea_haggf(act, rcb, dsh, function, first_time);
	    else
		status = qea_raggf(act, rcb, dsh, function, first_time);

	    function &= ~(TOP_OPEN | FUNC_RESET);
	    reset = FALSE;	/* reset the reset flag */
	    dsh->dsh_qef_output = (QEF_DATA *) old_output;
	    if (status)
	    {
		if (act->ahd_atype == QEA_HAGGF)
		{
		    QEN_HASH_AGGREGATE	*haptr = 
			dsh->dsh_hashagg[act->qhd_obj.qhd_qep.u1.s2.ahd_agcbix];

		    if (haptr->hshag_openfiles != (QEN_HASH_FILE *) NULL)
			(void) qea_haggf_closeall(haptr);
					/* close the overflow files */
		}
		if (dsh->dsh_error.err_code == E_QE0015_NO_MORE_ROWS &&
		    dsh->dsh_qef_rowcount == 1)
		{
		    qen_status->node_status = QEN4_NO_MORE_ROWS;
		    status = E_DB_OK;
		    dsh->dsh_error.err_code = E_QE0000_OK;
		}
		else
		    break;
	    }
	    /* qualify tuple */
	    status = qen_execute_cx(dsh, jqual_excb);
	    if (status != E_DB_OK)
		return (status);
	    /* Break out if the row qualifies */
	    if (jqual_excb == NULL || jqual_excb->excb_value == ADE_TRUE)
		break;
	}
	/* if no error or not an error on which we can continue the
	** next action, then exit.
	*/
	if (!status || (status && dsh->dsh_error.err_code != E_QE0015_NO_MORE_ROWS))
	    goto exit;
    }
    

    /* process each action */
    for(;;)
    {
	/* First check for parallel union - if so, find action header
	** this thread should execute. If we're at QE0015, quit loop
	** and show end-of-file from this thread. */
	if ((node->node_qen.qen_qp.qp_flag & QP_PARALLEL_UNION) &&
            !(ult_check_macro(&qef_cb->qef_trace, QEF_TRACE_EXCH_SERIAL_71, &val1, &val2)))
	{
	    if (status == E_DB_WARN &&
		dsh->dsh_error.err_code == E_QE0015_NO_MORE_ROWS)
		break;			/* this thread is done */

	    for (i = 1, act = node->node_qen.qen_qp.qp_act;
			i < dsh->dsh_threadno; i++)
		act = act->ahd_next;	/* find this thread's act. hdr */
	    qen_status->node_ahd_ptr = dsh->dsh_act_ptr = act;
	}
	else if (!act)
	    act = node->node_qen.qen_qp.qp_act;
					/* if no current action, get one */
	else
	{
	    act = act->ahd_next;	/* otherwise just get next */
	    if(qen_status->node_u.node_join.node_inner_status == QEN10_ROWS_AVAILABLE)
		{
		/* if we were originally called with reset, pass reset
		** to this (if any) action */
		reset = TRUE;
		function |= FUNC_RESET;
		}
	}

	if (!act)
	{
	    /* end of actions:  set off reset status */
	    qen_status->node_u.node_join.node_inner_status = QEN0_INITIAL;
	    break;
	}

	old_output = (PTR) dsh->dsh_qef_output;
	dsh->dsh_qef_output = &qef_data;
	status = qeq_validate(rcb, dsh, act, function, (bool)TRUE);

        /* bug: 117169, enable FUNC_RESET so that qen_orig() can
        ** make a qen_position() call. 
        */
        if (act->ahd_atype == QEA_GET && 
            act->qhd_obj.qhd_qep.ahd_qep &&
            act->qhd_obj.qhd_qep.ahd_qep->qen_type == QE_ORIG)
            function |= FUNC_RESET; 

	dsh->dsh_qef_output = (QEF_DATA *) old_output;
	if (status)
	{
	    if (dsh->dsh_error.err_code == E_QE0015_NO_MORE_ROWS)   
	    {
		if(act->ahd_next != (QEF_AHD *)NULL
		   ||
		   (act->ahd_atype != QEA_GET &&
		    act->ahd_atype != QEA_SAGG &&
		    act->ahd_atype != QEA_HAGGF &&
		    act->ahd_atype != QEA_RAGGF &&
		    act->ahd_atype != QEA_AGGF
		   )
	          )  
		{
		    dsh->dsh_error.err_code = E_QE0000_OK;
		    status = E_DB_OK;
		    continue;
		}
	    }
	    goto exit;
	}
	first_time = TRUE;
	act = dsh->dsh_act_ptr;	/* dsh_act_ptr is now set by qeq_validate */

	switch (act->ahd_atype)
	{
	    case QEA_DMU:
		status = qea_dmu(act, rcb, dsh);
		function &= ~(TOP_OPEN | FUNC_RESET);
		if (status)
		    goto exit;
		break;
	    case QEA_SAGG:
		/* ask for one row */
		dsh->dsh_qef_rowcount = 1;
		old_output = (PTR) dsh->dsh_qef_output;
		dsh->dsh_qef_output = &qef_data;
		status = qea_sagg(act, rcb, dsh, function);
		function &= ~(TOP_OPEN | FUNC_RESET);
		dsh->dsh_qef_output = (QEF_DATA *) old_output;
		if (status != E_DB_OK)
		    goto exit;

		/* if there are no more actions, then
		** since a simple agg can only return one row, mark
		** next attempt as a failure.
		*/
		if (act->ahd_next != (QEF_AHD*) NULL)
		{
		    break;
		}
		qen_status->node_status = QEN4_NO_MORE_ROWS;

		/* qualify tuple */
		status = qen_execute_cx(dsh, jqual_excb);
		if (status != E_DB_OK)
		    return (status);
		/* handle the condition where the qualification failed */
		if (jqual_excb != NULL && jqual_excb->excb_value != ADE_TRUE)
		{
		    dsh->dsh_error.err_code = E_QE0015_NO_MORE_ROWS;
		    status = E_DB_WARN;
		}
		goto exit;
	    case QEA_PROJ:
		status = qea_proj(act, rcb, dsh, function);
		function &= ~(TOP_OPEN | FUNC_RESET);
		if (status)
		    goto exit;
		break;
	    case QEA_AGGF:
		status = qea_aggf(act, rcb, dsh, function);
		function &= ~(TOP_OPEN | FUNC_RESET);
		if (status)
		    goto exit;
		break;
	    case QEA_GET:
	    case QEA_HAGGF:
	    case QEA_RAGGF:
		for (;;)
		{
		    if (qen_status->node_status == QEN4_NO_MORE_ROWS)
		    {
			dsh->dsh_error.err_code = E_QE0015_NO_MORE_ROWS;
			status = E_DB_WARN;
			dsh->dsh_qef_rowcount = 0;   /* be sure not left over */
			/* reset the node status for the next call */
			qen_status->node_status = QEN0_INITIAL;
			break;
		    }
		    /* ask for one row */
		    dsh->dsh_qef_rowcount = 1;
		    old_output = (PTR) dsh->dsh_qef_output;
		    dsh->dsh_qef_output = &qef_data;
		    if (act->ahd_atype == QEA_GET)
			status = qea_fetch(act, rcb, dsh, function);
		    else if (act->ahd_atype == QEA_HAGGF)
			status = qea_haggf(act, rcb, dsh, function, first_time);
		    else
			status = qea_raggf(act, rcb, dsh, function, first_time);

		    function &= ~(TOP_OPEN | FUNC_RESET);
		    reset = FALSE;
		    dsh->dsh_qef_output = (QEF_DATA *) old_output;
		    if (status)
		    {
			if (act->ahd_atype == QEA_HAGGF)
	 		{
			    QEN_HASH_AGGREGATE	*haptr = 
			dsh->dsh_hashagg[act->qhd_obj.qhd_qep.u1.s2.ahd_agcbix];

			    if (haptr->hshag_openfiles != (QEN_HASH_FILE *) NULL)
				(void) qea_haggf_closeall(haptr);
					/* close the overflow files */
			}
			if (dsh->dsh_error.err_code == E_QE0015_NO_MORE_ROWS &&
			    dsh->dsh_qef_rowcount == 1)
			{
			    qen_status->node_status = QEN4_NO_MORE_ROWS;
			    status = E_DB_OK;
			    dsh->dsh_error.err_code = E_QE0000_OK;
			}
			else
			    break;
		    }
		    first_time = FALSE;
		    /* qualify tuple */
		    status = qen_execute_cx(dsh, jqual_excb);
		    if (status != E_DB_OK)
			return (status);
		    if (jqual_excb == NULL
		      || jqual_excb->excb_value == ADE_TRUE)
			break;
		    /* else keep looping */
		}
		if (!status || 
		    (status && dsh->dsh_error.err_code != E_QE0015_NO_MORE_ROWS))
		    goto exit;
		break;
	    case QEA_APP:
	    case QEA_LOAD:
		status = qea_append(act, rcb, dsh, function, DSH_CT_INITIAL );
		function &= ~(TOP_OPEN | FUNC_RESET);
		if (status)
		    goto exit;
		break;
	    case QEA_DEL:
		status = qea_delete(act, rcb, dsh, function, DSH_CT_INITIAL );
		function &= ~(TOP_OPEN | FUNC_RESET);
		if (status)
		    goto exit;
		break;
	    case QEA_UPD:
		status = qea_replace(act, rcb, dsh, function, DSH_CT_INITIAL );
		function &= ~(TOP_OPEN | FUNC_RESET);
		if (status)
		    goto exit;
		break;
	    default:
		status = E_DB_ERROR;
		goto exit;
	}
	function &= ~(TOP_OPEN | FUNC_RESET);
	reset = FALSE;
    }
exit:
    /* if we are returning a row, add function attributes and print it */
    if (status == E_DB_OK && act && 
	(act->ahd_atype == QEA_GET ||
	 act->ahd_atype == QEA_SAGG ||
	 act->ahd_atype == QEA_HAGGF ||
	 act->ahd_atype == QEA_RAGGF)
    )
    {
	status = qen_execute_cx(dsh, node_xaddrs->qex_fatts);
	if (status != E_DB_OK)
	    return (status);

	/* Increment the count of rows that this node has returned */
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

    }
    /* restore current action */
    /* on error, reset the actions */
    if (status)
    {
	act = (QEF_AHD *)NULL;
	qen_status->node_u.node_join.node_inner_status = QEN0_INITIAL;  /* cancel reset status */
    }
    qen_status->node_ahd_ptr = act;
    /* restore value that existed when we entered this routine */
    dsh->dsh_act_ptr = cur_act;
    dsh->dsh_qef_rowcount = saved_rowcount;
#ifdef xDEBUG
    (VOID) qe2_chk_qp(dsh);
#endif

    if (dsh->dsh_qp_stats)
    {
	qen_ecost_end(dsh, &timerstat, qen_status);
    }

    return (status);    
}
