/*
**Copyright (c) 2010 Ingres Corporation
*/

#include    <compat.h>
#include    <bt.h>
#include    <gl.h>
#include    <cs.h>
#include    <me.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <adf.h>
#include    <ade.h>
#include    <ulf.h>
#include    <ulm.h>
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
#include    <tm.h>
#include    <ex.h>
#include    <dmmcb.h>
#include    <dmucb.h>

#include    <dudbms.h>
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
**  Name: QENTJOIN.C - tid join
**
**  Description:
**
**          qen_tjoin - tid join
**
**
**  History:    $Log-for RCS$
**	01-oct-90 (davebf)
**	    adapted from qen_tjoin with OJ support added
**	11-mar-92 (jhahn)
**	    Added support for QEF interrupts.
**	    Added call to CSswitch.
**	01-apr-92 (rickh)
**	    De-supported right joins inside kjoins.  We will support this
**	    only when DMF supports a "physical scan" of a table that returns
**	    tuples in TID order.
**	08-dec-92 (anitap)
**	    Included <qsf.h> and <psfparse.h> for CREATE SCHEMA.
**	13-apr-93 (rickh)
**	    Changed a number of checks after applying the JQUAL to look
**	    for =ADE_TRUE not !=ADE_TRUE.
**	7-may-93 (rickh)
**	    If we get a deleted TID error, that's OK.
**	25-jun-93 (rickh)
**	    Cloning error. This routine was looking for the TID JOIN
**	    OQUAL CX at the place in the outer join structure where
**	    the KEY JOIN OQUAL CX lives.
**      14-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
**      01-nov-95 (ramra01)
**          Prior to calling qen_u21_execute_cx confirm the presence of
**          a adf control block to execute
**	7-nov-95 (inkdo01)
**	    Changes to replace QEN_ADF and QEN_OJINFO structure instances by
**	    pointers in QEN_TJOIN.
**	05-feb-97 (kch)
**	    In qen_tjoin(), we now increment the counter of the rows that the
**	    node has returned. This change fixes bug 78836.
**      14-may-97 (dilma04)
**          Cursor Stability Project:
**          Set DMR_SORT flag if getting tuples to append them to a sorter.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	23-feb-04 (inkdo01)
**	    Changed qef_rcb->error to dsh->dsh_error for || thread safety.
**	15-Jul-2004 (schka24)
**	    Clean out remnants of never implemented right-join.
**	28-Dec-2005 (kschendel)
**	    Use DSH references whenever possible instead of QEF RCB
**	    references.  (This simplifies the code, eliminating many
**	    RCB references, and reflects the new primacy of the DSH
**	    thanks to the parallel query changes.)
**	30-Jun-2009 (kschendel) SIR 122138
**	    Missing include of me.h, sparc build failed.
**	11-May-2010 (kschendel) b123565
**	    Rename dsh-root to dsh-parent.
**/

/*	static functions	*/



/*{
** Name: QEN_TJOIN	- tid join
**
** Description:
**
** History:
**	01-oct-90 (davebf)
**	    adapted from qen_tjoin with OJ support added
**	11-mar-92 (jhahn)
**	    Added support for QEF interrupts.
**	    Added call to CSswitch.
**	01-apr-92 (rickh)
**	    De-supported right joins inside kjoins.  We will support this
**	    only when DMF supports a "physical scan" of a table that returns
**	    tuples in TID order.
**	13-apr-93 (rickh)
**	    Changed a number of checks after applying the JQUAL to look
**	    for =ADE_TRUE not !=ADE_TRUE.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      01-nov-95 (ramra01)
**          Prior to calling qen_u21_execute_cx confirm the presence of
**          a adf control block to execute
**	7-nov-95 (inkdo01)
**	    Changes to replace QEN_ADF and QEN_OJINFO structure instances by
**	    pointers in QEN_TJOIN.
**	05-feb-97 (kch)
**	    We now increment the counter of the rows that the node has
**	    returned, qen_status->node_rcount. This change fixes bug 78836.
**      14-may-97 (dilma04)
**          Cursor Stability Project:
**          Set DMR_SORT flag if getting tuples to append them to a sorter.
**	2-jan-03 (inkdo01)
**	    Removed noadf_mask logic in favour of QEN_ADF * tests to fix
**	    concurrency bug.
**	20-nov-03 (inkdo01)
**	    Add support for "open" call to allow parallel query initialization.
**	29-dec-03 (inkdo01)
**	    DSH is now parameter, "function" replaces "reset".
**	2-jan-04 (inkdo01)
**	    Added end-of-partition-grouping logic.
**	3-feb-04 (inkdo01)
**	    Copy TID8 instead of TID4 for table partitioning and handle
**	    Tjoins from global indexes to partitioned tables.
**	27-feb-04 (inkdo01)
**	    Changed dsh ptr to array of QEN_STATUS structs to ptr to array
**	    of ptrs.
**	17-mar-04 (toumi01)
**	    Add support for "close" call to allow parallel query termination.
**	19-mar-04 (inkdo01)
**	    Changes to handle big TIDs on byte-swapped machines.
**	5-apr-04 (inkdo01)
**	    One more change for byte-swapped big TID handling.
**	8-apr-04 (inkdo01)
**	    Added dsh parm to QEF_CHECK_FOR_INTERRUPT macro.
**	5-may-04 (inkdo01)
**	    Change tidp support for partitioned tables (composed like
**	    {i2, i2, i4}, rather than {i8}). Leaves other TID changes for
**	    byte swapping asis.
**	28-Jun-2004 (schka24)
**	    Partno for materialized inner tid was being taken from un-inited
**	    thing in status -- use tid that we fetched with instead.
**	19-Jul-2004 (schka24)
**	    Fix test for partition-open for repeated query.
**	10-Aug-2004 (jenjo02)
**	    Open tables for Child threads.
**	04-Nov-2004 (jenjo02)
**	    Only CSswitch on root DSH or if not OS-threaded.
**	3-may-05 (inkdo01)
**	    Add support for TJOIN_4BYTE_TIDP flag (fixes 114437).
**	5-may-05 (toumi01)
**	    For outer join queries with an IS NULL constraint on one of
**	    the equijoined columns, treat the result of ADE_COMPARE where
**	    xcxcb.excb_value == FALSE but xcxcb.excb_cmp == ADE_BOTHNULL
**	    as "success" for a JQUAL test, since it is quite ligit that
**	    ADE_COMPARE vs. ADE_BYCOMPARE will be generated, as e.g. when
**	    some members of the equivalence class are not nullable (hence
**	    ope_nulljoin is FALSE). For more info: issue 14110170-1
**	    problem INGSRV3289 bug 114467.
**	10-may-05 (toumi01)
**	    Based on work on issue 14138885 it turns out that the above
**	    change to test for ADE_BOTHNULL is incorrect and casts too
**	    broad a net for tuples. Instead a change is being made to
**	    opctuple.c to choose ADE_COMPARE vs. ADE_BYCOMPARE based on
**	    the nullability of the argument tuples, not the theoretical
**	    nullability of the entire equivalence class, which can be
**	    turned off by inclusion of a not-nullable tuple that is not
**	    involved in the immediate comparison.
**	18-Jul-2005 (schka24)
**	    Set current partition number in node status in case we're an
**	    originator for an in-place update (RUP).
**	13-Jan-2006 (kschendel)
**	    Revise CX execution calls.
**	25-Feb-2006 (kschendel)
**	    Always return after closing a tjoin node.
**	13-Feb-2007 (kschendel)
**	    Replace CSswitch with better cancel check.
**	9-Jan-2008 (kschendel) SIR 122513
**	    Implement partition qualification on the inner.
**	8-Sep-2008 (kibro01) b120693
**	    Remove unused action parameter from qeq_part_open
**	4-Jun-2009 (kschendel) b122118
**	    Use slightly faster/more convenient dsh test for qe90, and
**	    remove pointless error returns from cost begin/end.  Minor
**	    streamlining.
*/

DB_STATUS
qen_tjoin(
QEN_NODE           *node,
QEF_RCB            *rcb,
QEE_DSH		   *dsh,
i4		    function)
{
    QEF_CB	    *qef_cb = dsh->dsh_qefcb;
    PTR		    *cbs = dsh->dsh_cbs;
    QEE_XADDRS	    *node_xaddrs = dsh->dsh_xaddrs[node->qen_num];
    QEN_STATUS	    *qen_status = node_xaddrs->qex_status;
    ADE_EXCB	    *ade_excb;
    DMR_CB	    *dmr_cb;
    DMT_CB	    *dmt_cb;
    QEN_NODE	    *out_node = node->node_qen.qen_tjoin.tjoin_out;
    QEN_OJINFO	    *ojoin = node->node_qen.qen_tjoin.tjoin_oj;
    QEN_PART_INFO   *partp = node->node_qen.qen_tjoin.tjoin_part;
    QEN_PART_QUAL   *pqual;		/* Partition pruning info */
    QEE_PART_QUAL   *rqual = node_xaddrs->qex_rqual;
    PTR		    ktconst_map;	/* PP bitmap of constant and higher
					** join qualifying partitions */
    DB_TID8	    *output;
    char	    *input;
    DB_TID8	    bigtid;
    u_i2	    partno = 0;
    i4		    tid4;
    i4		    totparts;		/* Number of inner phys partitions */
    DB_STATUS	    status = E_DB_OK;
    i4	    offset;
    bool	    reset = FALSE;
    i4		    out_func = NO_FUNC;
    i4	    val1;
    i4	    val2;
    TIMERSTAT	    timerstat;

    if (function != 0)
    {
	if (function & FUNC_RESET)
	    reset = TRUE;

	/* Do open processing, if required. Only if this is the root node
	** of the query tree do we continue executing the function. */
	if ((function & TOP_OPEN || function & MID_OPEN) && 
	    !(qen_status->node_status_flags & QEN1_NODE_OPEN))
	{
	    /* FIXME! this ojoin test belongs in opc! */
	    if (ojoin != NULL)
	    {
		/* Only left joins supported.  A right t-join would require
		** something equivalent to a bitmap with potentially one bit
		** for every row in the inner table.
		*/
		if (ojoin->oj_jntype != DB_LEFT_JOIN)
		{
		    dsh->dsh_error.err_code = E_QE0002_INTERNAL_ERROR;
		    status = E_DB_ERROR;
		    goto errexit;           
		}
	    }
	    status = (*out_node->qen_func)(out_node, rcb, dsh, MID_OPEN);
	    qen_status->node_status_flags |= QEN1_NODE_OPEN;
	    if (function & MID_OPEN)
		return(E_DB_OK);
	    function &= ~TOP_OPEN;
	}
	/* Do close processing, if required. */
	if (function & FUNC_CLOSE)
	{
	    if (!(qen_status->node_status_flags & QEN8_NODE_CLOSED))
	    {
		/* Pass close to (left) child node */
		status = (*out_node->qen_func)(out_node, rcb, dsh, FUNC_CLOSE);
		qen_status->node_status_flags = 
		    (qen_status->node_status_flags & ~QEN1_NODE_OPEN) | QEN8_NODE_CLOSED;
	    }
	    return(E_DB_OK);
	}
    } /* function != 0 */

#ifdef xDEBUG
    (VOID) qe2_chk_qp(dsh);
#endif

    /* If the trace point qe90 is turned on then gather cpu and dio stats */
    if (dsh->dsh_qp_stats)
    {
	qen_bcost_begin(dsh, &timerstat, qen_status);
    }

    /* Check for cancel, context switch if not MT */
    CScancelCheck(dsh->dsh_sid);
    if (QEF_CHECK_FOR_INTERRUPT(qef_cb, dsh) == E_DB_ERROR)
	return (E_DB_ERROR);

    dsh->dsh_error.err_code = E_QE0000_OK;
    dmr_cb = (DMR_CB *) cbs[node->node_qen.qen_tjoin.tjoin_get];

    if (partp != NULL)
    {
	pqual = node->node_qen.qen_tjoin.tjoin_pqual;
	totparts = partp->part_totparts;
	ktconst_map = dsh->dsh_row[partp->part_ktconst_ix];
    }

    /*
    ** If forced reset, reset the node status.  This makes it go through
    ** the "first time" routine.
    ** There isn't really any "first time" for T-joins, but we'll pass
    ** the reset on down.
    */
    if (reset)
    {
	qen_status->node_status = QEN0_INITIAL;
	out_func = FUNC_RESET;
    }

    if(qen_status->node_status == QEN0_INITIAL)
    {
	if (partp != NULL && rqual != NULL)
	{
	    /* For a partitioned inner, combine any qualification bitmaps
	    ** that were constructed by higher-up joins, and include the
	    ** bitmap for constant qualifications.  None of this changes
	    ** as we fetch outer rows.
	    */
	    QEE_PART_QUAL *jq;
	    MEcopy(rqual->qee_constmap, rqual->qee_mapbytes, ktconst_map);
	    jq = rqual->qee_qnext;
	    while (jq != NULL)
	    {
		if (jq->qee_qualifying)
		    BTand(totparts, jq->qee_lresult, ktconst_map);
		jq = jq->qee_qnext;
	    }
	    /* Check for no possible partitions qualifying, if so and
	    ** it's an inner join, don't even bother reading the outer.
	    */
	    if (ojoin == NULL && BTcount(ktconst_map, totparts) == 0)
	    {
		qen_status->node_status = QEN1_EXECUTED; /* show not first*/
		dsh->dsh_error.err_code = E_QE0015_NO_MORE_ROWS;
		status = E_DB_WARN;
		goto errexit;
	    }
	}
	qen_status->node_status = QEN1_EXECUTED;  /* node now running */
    }


newouter:
    status = (*out_node->qen_func)(out_node, rcb, dsh, out_func);
    out_func = NO_FUNC;
    if (status != E_DB_OK)
    {
	if (dsh->dsh_error.err_code == E_QE0015_NO_MORE_ROWS)
	{
	    status = E_DB_WARN;
	    goto errexit;
	}
	if (dsh->dsh_error.err_code == E_QE00A5_END_OF_PARTITION &&
	    !(node->qen_flags & QEN_PART_SEPARATE))
	{
	    /* End of rows from partitioning group. Start reading 
	    ** next group or return 00A5 up to caller. */
	    out_func = FUNC_RESET;
	    goto newouter;
	}
	goto errexit;  /* else return error */ 
    }


    /* is the tid null */
    ade_excb = node_xaddrs->qex_prequal;
    if (ade_excb != NULL)
    {
	if (status = qen_execute_cx(dsh, ade_excb) )
	    goto errexit;   /* if ade error, return error */
    
	if (ade_excb->excb_value == ADE_TRUE)
	{
	    /* Tid is null, no inner can match - check for outer join */
	    if(ojoin != NULL)   /* if left join requested */
	    {
		if (status = qen_execute_cx(dsh, node_xaddrs->qex_lnull))
		    goto errexit;   /* if ade error, return error */
		goto postqual;	/* to return left join */
	    }

	    goto newouter;   /* else ignore outer */
	}
    }

    /* tid is not null */

    /* Now set the tid into the DMF control block for inner lookup */
    input = (char *) (dsh->dsh_row[node->node_qen.qen_tjoin.tjoin_orow]
			    + node->node_qen.qen_tjoin.tjoin_ooffset);

    /* Check for partitioned table and process DB_TID8 from global ix.
    ** Note: incoming tid from outer need not be aligned.
    */
    if (node->node_qen.qen_tjoin.tjoin_flag & TJOIN_4BYTE_TIDP)
    {
	I4ASSIGN_MACRO(*input, tid4);
    }
    else 
    {
	I8ASSIGN_MACRO(*input, bigtid);	/* Align properly */
	tid4 = bigtid.tid_i4.tid;
    }
    
    if (partp == (QEN_PART_INFO *) NULL)
    {
	partno = 0;

	if ( dmr_cb->dmr_access_id == NULL && dsh != dsh->dsh_parent )
	{
	    /* Call qeq_part_open() to open Child's DMR_CB. */
	    status = qeq_part_open(rcb, dsh,
		dsh->dsh_row[node->qen_row], 
		dsh->dsh_qp_ptr->qp_row_len[node->qen_row],
		node->node_qen.qen_tjoin.tjoin_get, (i4)-1, (u_i2)0);
	    if (status != E_DB_OK)
		return(status);
	}
    }
    else
    {
	partno = bigtid.tid.tid_partno;

	/* If we have partition qualification, run any join-time qualification
	** against this outer row, and then see if the partition number in
	** the tid qualifies.  If not, we can skip this outer.
	*/
	if (rqual != NULL)
	{
	    PTR qualmap;

	    qualmap = ktconst_map;
	    if (pqual->part_join_eval != NULL)
	    {
		/* eval puts answer in the lresult map */
		qualmap = rqual->qee_lresult;
		status = qeq_pqual_eval(pqual->part_join_eval, rqual, dsh);
		if (status != E_DB_OK)
		    goto errexit;
		BTand(totparts, ktconst_map, qualmap);
	    }
	    if (! BTtest(partno, qualmap))
	    {
		/* Partition excluded, no row - check for outer join */
		if(ojoin != NULL)
		{
		    if ( status = qen_execute_cx(dsh, node_xaddrs->qex_lnull) )
			goto errexit;   /* if ade error, return error */
		    goto postqual;	/* to return left join */
		}
		goto newouter;  /* No left join or not qualified, next outer */
	    }
	}
	dmr_cb = (DMR_CB *)cbs[partp->part_dmrix+partno+1];
	dmt_cb = (DMT_CB *) cbs[partp->part_dmtix + partno + 1];
	if (dmr_cb == NULL || dmt_cb == NULL || dmt_cb->dmt_record_access_id == NULL)
	{
	    /* Call qeq_part_open() to init/open DMR_CB. */
	    status = qeq_part_open(rcb, dsh,
		dsh->dsh_row[node->qen_row], 
		dsh->dsh_qp_ptr->qp_row_len[node->qen_row],
		partp->part_dmrix, partp->part_dmtix, partno);
	    if (status != E_DB_OK)
		return(status);
	    qen_status->node_pcount++;
	    dmr_cb = (DMR_CB *)cbs[partp->part_dmrix+partno+1];
	}
	cbs[node->node_qen.qen_tjoin.tjoin_get] = (PTR) dmr_cb;
    }
    dmr_cb->dmr_tid = tid4;
    qen_status->node_ppart_num = partno;	/* In case anyone (RUP) cares */

    dmr_cb->dmr_flags_mask = DMR_BY_TID;
    if (dsh->dsh_qp_status & DSH_SORT)
        dmr_cb->dmr_flags_mask |= DMR_SORT;

    /* Get the inner row-this should usually work the first time */
    if ((status = dmf_call(DMR_GET, dmr_cb)) != E_DB_OK)
    {
	if ( ( dmr_cb->error.err_code == E_DM003C_BAD_TID ) ||
	     ( dmr_cb->error.err_code == E_DM0044_DELETED_TID ) )
	{
	    /* tid get fails, no row - check for outer join */
	    if(ojoin != NULL)
	    {
		if ( status = qen_execute_cx(dsh, node_xaddrs->qex_lnull) )
		    goto errexit;   /* if ade error, return error */
		goto postqual;	/* to return left join */
	    }
	    goto newouter;   /* No left join or not qualified, next outer */
	}
	else
	{
	    dsh->dsh_error.err_code = dmr_cb->error.err_code;
	    goto errexit;
	}
    }

    /* we have an inner tuple */

    /* store inner tid, if requested - always as tid8.
    ** tjoin node compiler assures that ioffset is aligned.
    ** partno, tid4 set as appropriate from outer tid4 or tid8, above.
    */
    if(node->node_qen.qen_tjoin.tjoin_ioffset >= 0)
    {
	output = (DB_TID8 *) (dsh->dsh_row[node->node_qen.qen_tjoin.tjoin_irow]
			    + node->node_qen.qen_tjoin.tjoin_ioffset);
	output->tid.tid_partno = partno;
	output->tid.tid_page_ext = 0;
	output->tid.tid_row_ext = 0;
	output->tid_i4.tid = tid4;
    }

    /* execute OQUAL */
    ade_excb = node_xaddrs->qex_onqual;
    if (ade_excb != NULL)
    {
	status = qen_execute_cx(dsh, ade_excb);
	if (status != E_DB_OK)
	    goto errexit;
    }
    if (ade_excb == NULL || ade_excb->excb_value == ADE_TRUE)
    {
	/* no OQUAL or OQUAL succeeds, execute IJMAT if outer join */
	status = qen_execute_cx(dsh, node_xaddrs->qex_eqmat);
	if (status != E_DB_OK)
	    goto errexit;   /* if ade error, return error */
    }  /* end of OQUAL true */
    else  /* OQUAL false */
    {
	/* It must be an outer join for oqual to return false.
	** Null out the inner columns.
	*/
	if ( status = qen_execute_cx(dsh, node_xaddrs->qex_lnull) )
	    goto errexit;   /* if ade error, return error */
    }  /* end of OQUAL false */

postqual:

    /* Now post-qualify the result tuple */
    ade_excb = node_xaddrs->qex_jqual;
    if (ade_excb != NULL)
    {
	status = qen_execute_cx(dsh, ade_excb);
	if (status != E_DB_OK)
	    goto errexit;
	if (ade_excb->excb_value != ADE_TRUE)
	    goto newouter;	/* post-qual failed, get a new inner */
    }

    status = qen_execute_cx(dsh, node_xaddrs->qex_fatts);
    if (status != E_DB_OK)
	goto errexit;       

    /* Increment the count of rows that this node has returned */
    qen_status->node_rcount++;

    /* print tracing information DO NOT xDEBUG THIS */
    if (node->qen_prow != NULL &&
	(ult_check_macro(&qef_cb->qef_trace, 100+node->qen_num,
			&val1, &val2) ||
	ult_check_macro(&qef_cb->qef_trace, 99,
			&val1, &val2)))
    {
	(void) qen_print_row(node, rcb, dsh);
    }

#ifdef xDEBUG
    (VOID) qe2_chk_qp(dsh);
#endif

errexit:

    if (dsh->dsh_qp_stats)
    {
	qen_ecost_end(dsh, &timerstat, qen_status);
    }

    return (status);
}
