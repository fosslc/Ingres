/*
**Copyright (c) 2004 Ingres Corporation
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
#include    <scf.h>
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
#include    <qefdsh.h>
#include    <qefscb.h>
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
**  Name: QENKJOIN.C - key lookup on inner join
**
**  Description:
**      The attributes from the outer node can
**  be used to key/access the inner relation. 
**
**          qen_kjoin - key join
**
**
**  History:    $Log-for RCS$
**      8-apr-86 (daved)    
**          written
**	30-jul-86 (daved)
**	    updated once more information was known about how the system
**	    would work.
**	24-jul-87 (daved)
**	    added qen_pjoin as a special case of kjoin. pjoin has no keys,
**	    a scan is used instead.
**	02-feb-88 (puree)
**	    add reset flag to qen_kjoin function arguments.
**	03-aug-88 (puree)
**	    fix error in ult_check_macro call.
**	28-sep-89 (eric)
**	    Incremented qen_status->node_rcount to keep track of how many
**	    rows this node returns. This will be used to fix the Sybase bug.
**	29-nov-89 (davebf)
**	    modified for outer join support
**	11-mar-92 (jhahn)
**	    Added support for QEF interrupts.
**	    Added call to CSswitch.
**	01-apr-92 (rickh)
**	    De-supported right joins inside kjoins.  We will support this
**	    only when DMF supports a "physical scan" of a table that returns
**	    tuples in TID order.
**	08-dec-92 (anitap)
**	    Included <qsf.h> and <psfparse.h> for CREATE SCHEMA.
**	02-apr-93 (rickh)
**	    When an inner join condition occurs, remember it.
**      14-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
**	31-aug-93 (rickh)
**	    If an outer tuple inner joined with an inner tuple but failed
**	    the ON clause for the next inner tuple, we were emitting the
**	    outer tuple as both an inner and outer join.
**	18-oct-93 (rickh)
**	    Converted node_access to a flag array.  Added new flag to indicate
**	    when the last outer tuple successfully keyed into the inner
**	    relation.  This allows us to fix a bug:  we used to get an
**	    "unpositioned stream" error from DMF when trying to reposition a
**	    BTREE on a previous outer value which failed to probe in the first
**	    place.
**
**	    Also, moved the inner tuple materializer higher up so that the
**	    inner tuple would be available for the KQUAL and OQUAL.
**      01-nov-95 (ramra01)
**          Prior to calling qen_u21_execute_cx confirm the presence of
**          a adf control block to execute
**	7-nov-95 (inkdo01)
**	    Changes to replace QEN_ADF and QEN_OJINFO structure instances by
**	    pointers in QEN_KJOIN.
**	12-jun-96 (inkdo01)
**	    Support null detection in Rtree spatial key joins.
**	05-feb-97 (kch)
**	    In qen_kjoin(), we now increment the counter of the rows that the
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
**	14-Jul-2004 (schka24)
**	    First cut at implementing partitioned inners.  For now,
**	    there's no support for using the struct-compat flag in the
**	    partdef to narrow the partition search - we just look at all
**	    partitions.
**	28-Dec-2005 (kschendel)
**	    Use DSH references whenever possible instead of QEF RCB
**	    references.  (This simplifies the code, eliminating many
**	    RCB references, and reflects the new primacy of the DSH
**	    thanks to the parallel query changes.)
**	13-Jan-2006 (kschendel)
**	    Revise CX execution calls.
**	9-Jan-2008 (kschendel) SIR 122513
**	    Implement partition qualification on the inner, finally.
**	11-Apr-2008 (kschendel)
**	    Use utility for setting DMF qualification.
**/


/*	static functions	*/

static DB_STATUS kjoin_inner(
	QEF_RCB *rcb,
	QEE_DSH *dsh,
	QEN_NODE *node,
	QEE_XADDRS *node_xaddrs,
	bool do_position);


/*{
** Name: QEN_KJOIN	- key join
**
** Description:
**      The attributes from the outer node is used to key the inner relation. 
**
** Inputs:
**      node                            the keyed join node data structure
**	rcb				request block
**	dsh				Query fragment data segment header
**	func				Function (open, close, etc)
**
** Outputs:
**      dsh
**	    .dsh_error.err_code		one of the following
**					E_QE0000_OK
**					E_QE0015_NO_MORE_ROWS
**					E_QE00A5_END_OF_PARTITION
**					E_QE0019_NON_INTERNAL_FAIL
**					E_QE0002_INTERNAL_ERROR
**					
**	Returns:
**	    E_DB_{OK, WARN, ERROR, FATAL}
**	Exceptions:
**	    none
**
** Side Effects:
**	All values in the appropriate row buffers have been set to that
**	which will produce the joined row.
**
** History:
**	8-apr-86 (daved)
**          written
**	02-feb-88 (puree)
**	    add reset flag to the function arguments.
**	03-aug-88 (puree)
**	    fix error in ult_check_macro call.
**	28-sep-89 (eric)
**	    Incremented qen_status->node_rcount to keep track of how many
**	    rows this node returns. This will be used to fix the Sybase bug.
**	29-nov-89 (davebf)
**	    modified for outer join support
**	11-mar-92 (jhahn)
**	    Added support for QEF interrupts.
**	    Added call to CSswitch.
**	01-apr-92 (rickh)
**	    De-supported right joins inside kjoins.  We will support this
**	    only when DMF supports a "physical scan" of a table that returns
**	    tuples in TID order.
**	02-apr-93 (rickh)
**	    When an inner join condition occurs, remember it.
**	14-apr-93 (rickh)
**	    An assumption was made that if the last outer tuple created a
**	    left join condition, then its successor will too.  But this
**	    is not true if there are non-key clauses in the OQUAL.  Bogus
**	    assumption removed.  Also removed the test for new_outer, which
**	    looked really wrong.
**      08-apr-93 (smc)
**          Moved declaration of qen_kjoin() to clear compiler warnings.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	18-oct-93 (rickh)
**	    Converted node_access to a flag array.  Added new flag to indicate
**	    when the last outer tuple successfully keyed into the inner
**	    relation.  This allows us to fix a bug:  we used to get an
**	    "unpositioned stream" error from DMF when trying to reposition a
**	    BTREE on a previous outer value which failed to probe in the first
**	    place.
**
**	    Also, moved the inner tuple materializer higher up so that the
**	    inner tuple would be available for the KQUAL and OQUAL.
**
**	    Also, go get another inner if the OQUAL fails.  You can't emit
**	    the outer tuple as a LEFT JOIN until you know that it doesn't
**	    inner join with ANY inner tuple.
**      01-nov-95 (ramra01)
**          Prior to calling qen_u21_execute_cx confirm the presence of
**          a adf control block to execute
**	7-nov-95 (inkdo01)
**	    Changes to replace QEN_ADF and QEN_OJINFO structure instances by
**	    pointers in QEN_KJOIN.
**	12-jun-96 (inkdo01)
**	    Support null detection in Rtree spatial key joins.
**	05-feb-97 (kch)
**	    We now increment the counter of the rows that the node has
**	    returned, qen_status->node_rcount. We ignore the first call to
**	    this function in oder to give the exact row count. This change
**	    fixes bug 78836.
**      14-may-97 (dilma04)
**          Cursor Stability Project:
**          Set DMR_SORT flag if getting tuples to append them to a sorter.
**	28-may-97 (inkdo01)
**	    Fix logic for handling TID qualifications. If kjoin_tqual, 
**	    execute the kjoin_iqual OUTSIDE of DMF.
**	12-nov-98 (inkdo01)
**	    Added support of base table index only access (Btree).
**	20-may-99 (hayke02)
**	    We now do not call qen_u21_execute_cx(QEN_22KJN_KQUAL,&xcxcb)
**	    and only set xcxcb.excb_value TRUE if KJN22_KQUAL is set in
**	    qen_kjoin.noadf_mask. KJN22_KQUAL will be set in qeq_validate()
**	    if a NULL kjoin_kqual is found - this will now be the case
**	    for non-outer and non-spatial key joins (see opc_ijkjoin()).
**	    This change fixes bug 96840.
**      12-Oct-2001 (hanal04) Bug 105231 INGSRV 1495.
**          If DMR_GET fails with E_QE0025_USER_ERROR do not overwrite
**          dsh->dsh_error.err_code with the dmr_cb->error.err_code. The
**          will already have been handled in qef_adf_error() and
**          qef_error() will report 'alarming' E_DM008A and E_QE007C
**          errors when the underlying error is a USER error.
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
**	    Changed TIDs to TID8s.
**	27-feb-04 (inkdo01)
**	    Changed dsh ptr to array of QEN_STATUS structs to ptr to array
**	    of ptrs.
**	12-mar-04 (my daughter's birthday) (inkdo01)
**	    Fill in dmr_q_arg with this thread's dsh addr.
**	17-mar-04 (toumi01)
**	    Add support for "close" call to allow parallel query termination.
**	19-mar-04 (inkdo01)
**	    Fix TID manipulation for byte swapped machines.
**	8-apr-04 (inkdo01)
**	    Added dsh parm to QEF_CHECK_FOR_INTERRUPT macro.
**	13-Jul-2004 (schka24)
**	    Added partitioned-inner support (first cut).
**	    Cleaned out remnants of never-implemented right join code,
**	    right kjoin is just impractical IMHO.  Cleaned out other
**	    dead code (new_outer set but never tested, etc).
**	    Test/set the "this key won't probe" flag from the same place
**	    at all times (!) (node-access, not node-status).
**	10-Aug-2004 (jenjo02)
**	    KJOIN under a Child thread needs its tables opened now.
**	04-Nov-2004 (jenjo02)
**	    Only CSswitch on root DSH or if not OS-threaded.
**	13-Dec-2005 (kschendel)
**	    Double-check that there's an iqual before asking DMF to
**	    qualify against it.  (probably always an iqual, except maybe
**	    for spatial.)
**	25-Feb-2006 (kschendel)
**	    Always return after doing a FUNC_CLOSE.
**	13-Feb-2007 (kschendel)
**	    Replace CSswitch with better cancel check.
**	9-Jan-2008 (kschendel) SIR 122513
**	    Set up qualifying partition maps for the inner.
**	25-march-2008 (dougi) bug 120154
**	    Change nullind_ptr to u_char* from bool*.
**	8-Sep-2008 (kibro01) b120693
**	    Remove unused action parameter from qeq_part_open
**	4-Jun-2009 (kschendel) b122118
**	    Use slightly faster/more convenient dsh test for qe90, and
**	    remove pointless error returns from cost begin/end.  Minor
**	    streamlining.
**	11-Nov-2009 (kiria01) SIR 121883
**	    Scalar sub-query - Pick up on cardinality errors if requested.
*/

DB_STATUS
qen_kjoin(
QEN_NODE           *node,
QEF_RCB            *rcb,
QEE_DSH		   *dsh,
i4		    function )
{
    ADE_EXCB	    *ade_excb;
    ADE_EXCB	    *jqual_excb;
    QEF_CB	    *qef_cb = dsh->dsh_qefcb;
    PTR		    *cbs = dsh->dsh_cbs;
    QEE_XADDRS	    *node_xaddrs = dsh->dsh_xaddrs[node->qen_num];
    QEN_STATUS	    *qen_status = node_xaddrs->qex_status;
    DMR_CB	    *dmr_cb;
    QEN_NODE	    *out_node = node->node_qen.qen_kjoin.kjoin_out;
    QEN_OJINFO	    *ojoin = node->node_qen.qen_kjoin.kjoin_oj;
    QEN_PART_INFO   *partp;		/* Partitioning info if any */
    QEN_PART_QUAL   *pqual;		/* Partition pruning info */
    QEE_PART_QUAL   *rqual = node_xaddrs->qex_rqual;
    PTR		    ktconst_map;	/* PP bitmap of constant AND higher-
					** join qualifying partitions */
    PTR		    knokey_map;		/* Bitmap of PP's not containing key */
    DB_STATUS	    status = E_DB_OK;
    STATUS	    st;
    bool	    reset = FALSE;
    i4		    out_func = NO_FUNC;
    i4	    val1;
    i4	    val2;
    i4		    cmp_value;
    i4		    totparts;		/* Physical partitions of inner */
    i4		    mapbytes;		/* Size (bytes) of phys partition map */
    bool	    read_outer;		/* Need to / have just read an outer */
    TIMERSTAT	    timerstat;
    u_char	    *nullind_ptr;
    bool	    potential_card_violation = FALSE;


#ifdef xDEBUG
    (VOID) qe2_chk_qp(dsh);
#endif

    partp = node->node_qen.qen_kjoin.kjoin_part;
    if (partp != NULL)
    {
	totparts = partp->part_totparts;
	/* mapbytes is precomputed in the rqual but there might not be one */
	mapbytes = (totparts + 7) >> 3;
	pqual = node->node_qen.qen_kjoin.kjoin_pqual;
	/* ktconst_map is only relevant if there is partition qualification
	** around (a pqual / rqual).  knokey_map is always used.
	*/
	ktconst_map = dsh->dsh_row[partp->part_ktconst_ix];
	knokey_map = dsh->dsh_row[partp->part_knokey_ix];
    }

    if (function != 0)
    {
	if (function & FUNC_RESET)
	    reset = TRUE;

	/* Do open processing, if required. Only if this is the root node
	** of the query tree do we continue executing the function. */
	if ((function & TOP_OPEN || function & MID_OPEN) && 
	    !(qen_status->node_status_flags & QEN1_NODE_OPEN))
	{
	    /* "Open" the kjoin node. */

	    /* FIXME this ojoin test really belongs in opc */
	    if (ojoin != NULL)
	    {
		/* Only left joins supported */
		if (ojoin->oj_jntype != DB_LEFT_JOIN)
		{
		    dsh->dsh_error.err_code = E_QE0002_INTERNAL_ERROR;
		    status = E_DB_ERROR;
		    goto errexit;           
		}
	    }

	    if (partp == NULL)
	    {
		/* Prepare unpartitioned dmr_cb with proper DSH parameter.
		** For a nonpartitioned inner, set the DMR_GET qualification
		** function since we won't be farting with partition dmrcb's.
		*/

		dmr_cb = (DMR_CB *) cbs[node->node_qen.qen_kjoin.kjoin_get];

		if ( dmr_cb->dmr_access_id == NULL && dsh != dsh->dsh_root )
		{
		    /*
		    ** Then this is a Child thread that needs
		    ** its table opened.
		    */
		    status = qeq_part_open (rcb, dsh,
				dsh->dsh_row[node->qen_row],
				dsh->dsh_qp_ptr->qp_row_len[node->qen_row],
				    node->node_qen.qen_kjoin.kjoin_get,
				    (i4)-1, (u_i2)0);
		    if (status != E_DB_OK)
			return (status);
		}

		/* Set kjoin inner qualification in DMF if appropriate */
		qen_set_qualfunc(dmr_cb, node, dsh);
	    }

	    /* Pass open request to our children */
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
    jqual_excb = node_xaddrs->qex_jqual;

    /*
    ** If forced reset, reset the node status.  This makes it go through
    ** the "first time" routine.
    */
    if (reset)
    {
	qen_status->node_status = QEN0_INITIAL;
	out_func = FUNC_RESET;
    }

    /* Need a fresh outer row if first time, or if join-state bits say so */
    read_outer = FALSE;
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
	    MEcopy(rqual->qee_constmap, mapbytes, ktconst_map);
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
	qen_status->node_access = 0;
	read_outer = TRUE; 
	/* Leave initial status for below */
    }
    else if ((qen_status->node_access & (QEN31_IJ_OUTER | QEN32_OJ_OUTER)) != 0)
    {
	read_outer = TRUE;
    }

    status = E_DB_OK;

    /* Locate null indicator copied from nullable outers in spatial
    ** key joins. */
    if (node->node_qen.qen_kjoin.kjoin_nloff >= 0)
	nullind_ptr = dsh->dsh_row[node->node_qen.qen_kjoin.kjoin_key->
	    qen_output] + node->node_qen.qen_kjoin.kjoin_nloff;
    else nullind_ptr = NULL;

    if (read_outer)
    {
newouter:
	/* Get an outer, looping if we ran off the end of an outer
	** partition group.
	** (currently notimp)
	*/
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
		/* End of partitioning group - immediately start reading
		** the next or pass 00A5 up the plan tree.
		** *Note* as of Jul '04 we don't use partitioning groups
		** for kjoins, so you don't really get here.
		*/
		out_func = FUNC_RESET;
		goto newouter;
	    }
	    goto errexit;  /* else return error */ 
	}
	read_outer = TRUE;		/* Just read a new outer */

	/* if the first time through the loop (or forced reset) */
	if (qen_status->node_status == QEN0_INITIAL)
	{
	    cmp_value = ADE_NOT_TRUE;
	    qen_status->node_status = QEN1_EXECUTED; /* show not first*/
	}
	else
	{
	    /* compare to previous key value */
	    cmp_value = ADE_TRUE;
	    ade_excb = node_xaddrs->qex_kcompare;
	    if (ade_excb != NULL)
	    {
		status = qen_execute_cx(dsh, ade_excb);
		if (status != E_DB_OK)
		    goto errexit;
		cmp_value = ade_excb->excb_value;
	    }
	}
	if (cmp_value != ADE_TRUE)  /* not equal */
	{
	    /*
	    ** New key.  (or first time/reset.)
	    ** We can now forget whether the last tuple keyed
	    ** into the inner relation.
	    */
	    qen_status->node_access &= ~(QEN_THIS_KEY_WONT_PROBE | QEN_KEY_IN_SOME_PART);

	    /* now materialize and prepare the new outer key to be used
	    ** in DMR_POSITION */

	    status = qen_execute_cx(dsh, node_xaddrs->qex_keyprep);
	    if (status != E_DB_OK)
		goto errexit;   /* if ade error, return error */
    
	    /* check if this was a null outer column. 
	    */
	    if (nullind_ptr && *nullind_ptr)
		qen_status->node_access |= QEN_THIS_KEY_WONT_PROBE;

	    qen_status->node_access |= QEN_OUTER_NEW_KEY;
	    /* Reset the key-not-found partition map to all-found */
	    if (partp != NULL)
		MEfill(mapbytes, 0, knokey_map);
	}
	else
	{
	    /* 
	    ** We come here if the new outer key is the same
	    ** as the old one.
	    */

	    qen_status->node_access &= ~QEN_OUTER_NEW_KEY;
	}

	/* indicate a new outer has been read  with no LJ or IJ
	** returned yet */
	QEN_SET_JOIN_STATE( qen_status->node_access, 0 );

	/* If partitioned inner, qualify possible partitions based on
	** the available outer row values.  The qual evaluation will
	** operate directly into lresult, to which we apply the constant
	** and higher-join bits that are held in ktconst_map.
	** (Note that qualification can depend on the whole outer, not
	** just the key, so this needs done even if no-new-outer-key.)
	*/
	if (rqual != NULL)
	{
	    if (pqual->part_join_eval == NULL)
	    {
		/* Just the constant stuff -- put where expected */
		MEcopy(ktconst_map, mapbytes, rqual->qee_lresult);
	    }
	    else
	    {
		status = qeq_pqual_eval(pqual->part_join_eval, rqual, dsh);
		if (status != E_DB_OK)
		    goto errexit;
		BTand(totparts, ktconst_map, rqual->qee_lresult);
	    }
	}
	if (partp != NULL)
	{
	    qen_status->node_ppart_num = -1;	/* Gets things started */
	}

    }   /* end of if read_outer */


    for (;;)	/* to break/continue in */
    {
	/* Fetch (next) inner for this key */

	status = kjoin_inner(rcb, dsh, node, node_xaddrs, read_outer);
	potential_card_violation = !read_outer && !status;
	read_outer = FALSE;
	if (status != E_DB_OK)
	{
	    /* if end of kjoin true */
	    if (dsh->dsh_error.err_code != E_QE0015_NO_MORE_ROWS)
	    {
		/* Badness, exit */
		goto errexit;
	    }
	    /* No (more) inners for this outer */
	    if(ojoin != NULL		/* if left join requested and */
		&& ( ( qen_status->node_access & QEN_JOIN_STATE_MASK ) == 0) )
					    /* no IJ or LJ returned */
	    {
		/* execute oj_lnull, fill in nulls */
		if(status = qen_execute_cx(dsh, node_xaddrs->qex_lnull))
		    goto errexit;   /* if ade error, return error */

		/* Qualify (outer)-joined row, get new outer if fails */
		status = qen_execute_cx(dsh, jqual_excb);
		if (status != E_DB_OK) goto errexit;
		if (jqual_excb == NULL || jqual_excb->excb_value == ADE_TRUE)
		{
		    /* We'll return an (outer-joined) row.
		    ** Set state so that we get a new outer next time in.
		    */

		    QEN_SET_JOIN_STATE( qen_status->node_access,
				    QEN32_OJ_OUTER );
		    break;	/* to return left join */
		}
	    } 
	    
	    /* No oj, or jqual rejected the oj'ed row.
	    ** Time for a new outer.
	    */
	    goto newouter;
	}
	/* we have an inner tuple */

	/* If there's a TID qualifier, kjoin_iqual is executed out here,
	** not in DMF. */
	if (node->node_qen.qen_kjoin.kjoin_tqual &&
	    (ade_excb = node_xaddrs->qex_origqual) != NULL)
	{
	    if ((status = qen_execute_cx(dsh, ade_excb)))
		goto errexit;   /* if ade error, return error */
	    if (ade_excb->excb_value != ADE_TRUE)
		continue;		/* to get new inner */
	}

	/* Materialize the joined tuple (if OJ) for join, ON-clause tests */
	status = qen_execute_cx(dsh, node_xaddrs->qex_eqmat);
	if (status != E_DB_OK)
	    goto errexit;   /* if ade error, return error */

	/* execute KQUAL  if it exists */
	ade_excb = node_xaddrs->qex_joinkey;
	if (ade_excb != NULL)
	{
	    if (status = qen_execute_cx(dsh, ade_excb) )
		goto errexit;   /* if ade error, return error */
	    if (ade_excb->excb_value != ADE_TRUE)
		continue;	/* to get new inner */
	}

	/* execute OQUAL if outer join (the ON clause) */
	ade_excb = node_xaddrs->qex_onqual;
	if (ade_excb != NULL)
	{
	    status = qen_execute_cx(dsh, ade_excb);
	    if (status != E_DB_OK)
		goto errexit;   /* if ade error, return error */
	}
	if (ade_excb == NULL || ade_excb->excb_value == ADE_TRUE)
	{
	    /* No OQUAL, or OQUAL succeeds, so these rows inner-join. */

	    /* If inner is keyed unique, there's no need to look
	    ** for more inners, and we can get another outer
	    ** when we return.
	    */
	    if(node->node_qen.qen_kjoin.kjoin_kuniq)
	    {
		/* make next entry read new outer */
		QEN_SET_JOIN_STATE( qen_status->node_access,
					QEN31_IJ_OUTER );
	    }
	    else
	    {
		/* need to read more inners when we return */
		QEN_SET_JOIN_STATE( qen_status->node_access,
					QEN33_IJ_INNER );
	    }
	    /* See if joined result passes JQUAL */
	    if (jqual_excb != NULL)
	    {
		status = qen_execute_cx(dsh, jqual_excb);
		if (status != E_DB_OK)
		    goto errexit;   /* if ade error, return error */
		if (jqual_excb->excb_value != ADE_TRUE)
		{
		    /* Don't want this join.  If inner is unique we can skip
		    ** looking for more and get a new outer.  Otherwise keep
		    ** looking for joining inners.
		    */
		    if (node->node_qen.qen_kjoin.kjoin_kuniq)
			goto newouter;
		    continue;	/* to get a new inner */
		}
	    }
	    if (potential_card_violation &&
		(node->qen_flags & QEN_CARD_MASK) == QEN_CARD_01R)
	    {
		/* We only want to act on seltype violation after
		** qualification and that is now. */
		qen_status->node_status = QEN7_FAILED;
		dsh->dsh_error.err_code = E_QE004C_NOT_ZEROONE_ROWS;
		status = E_DB_ERROR;
		goto errexit;
	    }
	    break;		/* to return inner join */
	}  /* end if inner-join */

	/*
	** If OQUAL was false, this must be an outer join, and this
	** inner and outer didn't join.
	** Go get another inner tuple.  We can't emit this outer
	** tuple as a LEFT JOIN until we know that it doesn't
	** inner join with any inner tuple.
	*/

    }   /* end inner loop */

    /* Return an inner or outer joined row. */

    status = qen_execute_cx(dsh, node_xaddrs->qex_fatts);
    if (status != E_DB_OK)
	goto errexit;       

#ifdef xDEBUG
    (VOID) qe2_chk_qp(dsh);
#endif

errexit:

    if (dsh->dsh_qp_stats)
    {
	qen_ecost_end(dsh, &timerstat, qen_status);
    }

    /* Increment the count of rows that this node has returned */
    if (status == E_DB_OK)
    {
	qen_status->node_rcount++;

	/* print tracing information DO NOT xDEBUG THIS */
	if (node->qen_prow != NULL &&
	    (ult_check_macro(&qef_cb->qef_trace, 100+node->qen_num,
				&val1, &val2) ||
		ult_check_macro(&qef_cb->qef_trace, 99,
				&val1, &val2) ))
	{
	    (void) qen_print_row(node, rcb, dsh);
	}
    }

    return (status);
}

/* Name: kjoin_inner - Read inner row via key
**
** Description:
**	This routine reads an inner row.  If we're working on a
**	fresh outer, we position first;  otherwise we simply read
**	the next inner (there might be several inners that match
**	the given key).
**
**	For an unpartitioned inner, doing the above is simple enough.
**	However for a partitioned inner, we need to repeat the
**	position+read for each partition.
**
**	For partitioned inners, there are two cases:  some sort of
**	partition qualification against the inner exists, or not.
**	The no-qualification case is the simplest and most expensive,
**	as we simply probe each partition in turn, doing a key position
**	in each, and reading any rows found.
**
**	If qualification of some sort exists, whether constant or join-time
**	or both, the caller has prepared the bitmap of qualifying
**	partitions in the "local result" bitmap row.
**
**	In both partitioning cases, when moving to a new partition,
**	we test against the "known no-match partition" bitmap.  This
**	optimization can help when the key value occurs repeatedly for
**	multiple outer rows;  partitions that don't contain the key are
**	remembered for the next outer(s).  Obviously this is wasted
**	work when the outer key values are unique, but we don't know
**	that a priori.
**	
**	When the inner is partitioned, only after the final partition is
**	examined do we return a "no next" to the caller.
**
** Inputs:
**	rcb			The query's QEF_RCB request block
**	dsh			This query fragment's data segment header
**	node			Pointer to the kjoin node
**	node_xaddrs		Pointer to runtime pointers for node
**	do_position		TRUE if we just read a new outer;
**				otherwise we've positioned and should just
**				try to read more inners.
**
** Outputs:
**	dsh.dsh_error.err_code	Set to E_QE0015_NO_MORE_ROWS if no (more)
**				matching inners;  or to other error code
**				if something bad happened.
**	Returns E_DB_xxx status;  no-more-rows returns E_DB_WARN
**
** Side effects:
**	If the inner tid is needed (for a qualification, say), it will
**	be computed into the inner's row buffer at the specified offset.
**
** History:
**	15-Jul-2004 (schka24)
**	    First pass at doing partitioned inners.
**	10-Aug-2004 (jenjo02)
**	    Open Child thread's table.
**	18-Jul-2005 (schka24)
**	    node-ppart-num moved, adjust here.
**	9-Jan-2008 (kschendel) SIR 122513
**	    Overhaul partitioned inners to fit the standard framework.
**	17-Apr-2008 (kschendel)
**	    Set iierrornumber if DMF-handled row qualification ADF error.
**	8-Sep-2008 (kibro01) b120693
**	    Remove unused action parameter from qeq_part_open
**	2-Apr-2009 (wanfr01) 
**	    Bug 121882
**	    iqual offset needs to be set prior to fetching rows from DMF
**	27-Jul-2009 (thaju02) B122326
**	    Position fails with E_DM0074 on newly opened partition due to
**	    DMR_REPOSITION flag being specified. 
*/

static DB_STATUS
kjoin_inner(QEF_RCB *rcb, QEE_DSH *dsh,
	QEN_NODE *node, QEE_XADDRS *node_xaddrs,
	bool do_position)
{
    char *output;			/* Where to put the inner tid */
    DB_STATUS status;			/* The usual status thing */
    DB_TID8 bigtid;			/* Inner tid with partition */
    DMR_CB *dmr;			/* Inner's DMF record request CB */
    DMT_CB *dmt;			/* Inner's DMF table request CB */
    i4 offset;				/* Tid offset in output row */
    i4 partno;				/* Partition number */
    i4 totparts;			/* Number of physical partitions */
    PTR *cbs;				/* CB pointer base for this DSH */
    QEE_PART_QUAL *rqual = node_xaddrs->qex_rqual;
    QEN_PART_INFO *partp;		/* Partitioning info if any */
    QEN_STATUS *ns = node_xaddrs->qex_status;
    PTR knokey_map;			/* Bitmap of PP's not containing key */

    /* Skip everything if new outer (positioning) and we know that the
    ** key doesn't match anything.  (ie same key as last outer and last
    ** outer got a nomatch.)
    */
    if (do_position && ns->node_access & QEN_THIS_KEY_WONT_PROBE)
    {
	dsh->dsh_error.err_code = E_QE0015_NO_MORE_ROWS;
	return (E_DB_WARN);
    }

    cbs = dsh->dsh_cbs;
    partp = node->node_qen.qen_kjoin.kjoin_part;

    if (partp == NULL)
    {
	/* Not partitioned, set up dmrcb ptr just once here */
	partno = 0;		/* In case of building a tid */
	dmr = (DMR_CB *) cbs[node->node_qen.qen_kjoin.kjoin_get];

	if ( dmr->dmr_access_id == NULL && dsh != dsh->dsh_root )
	{
	    /*
	    ** Then this is a Child thread that needs
	    ** its table opened.
	    */
	    status = qeq_part_open (rcb, dsh,
			dsh->dsh_row[node->qen_row],
			dsh->dsh_qp_ptr->qp_row_len[node->qen_row],
			    node->node_qen.qen_kjoin.kjoin_get,
			    (i4)-1, (u_i2)0);
	    if (status != E_DB_OK)
		return (status);
	    /* Set kjoin inner qualification in DMF if appropriate */
	    qen_set_qualfunc(dmr, node, dsh);
	}
    }
    else
    {
	knokey_map = dsh->dsh_row[partp->part_knokey_ix];
	totparts = partp->part_totparts;
	partno = ns->node_ppart_num;
	/* partno should be a valid, open partition if not positioning
	** (i.e. if reading inners).  If positioning, we'll set dmr below.
	*/
	if (! do_position)
	    dmr = (DMR_CB *) cbs[partp->part_dmrix + partno + 1];
    }


    /* Now position/read an inner */
    for (;;)
    {

	status = E_DB_OK;
	if (do_position)
	{
	    /* We're positioning the inner first. */
	    /* If it's partitioned, it's time for a new partition. */
	    if (partp != NULL)
	    {
		/* Spin forward until we find the next partition to work
		** on.  A loop is needed because first we advance, and then
		** check against the key-notin-partition bitmap.
		** This loop may discover that there are no more partitions.
		*/
		for (;;)
		{
		    if (rqual != NULL)
		    {
			/* If we're doing qualification, advance to the next
			** qualified partition in the local-result map.
			*/
			partno = BTnext(partno, rqual->qee_lresult, totparts);
			if (partno < 0)
			    break;
		    }
		    else
		    {
			if (++partno >= totparts)
			    break;
		    }
		    if (ns->node_access & QEN_OUTER_NEW_KEY
		      || ! BTtest(partno, knokey_map) )
			break;
		}
		if (partno < 0 || partno >= totparts)
		{
		    /* Didn't find any more partitions to probe.
		    ** Light "tain't there" flag if all probes failed,
		    ** and then return without a row to the caller.
		    */
		    if ((ns->node_access & QEN_KEY_IN_SOME_PART) == 0)
			ns->node_access |= QEN_THIS_KEY_WONT_PROBE;
		    dsh->dsh_error.err_code = E_QE0015_NO_MORE_ROWS;
		    return (E_DB_WARN);
		}
		ns->node_ppart_num = partno;
		dmt = (DMT_CB *) cbs[partp->part_dmtix + partno + 1];
		dmr = (DMR_CB *) cbs[partp->part_dmrix + partno + 1];
		if (dmr == NULL || dmt == NULL || dmt->dmt_record_access_id == NULL)
		{
		    /* Partition ain't open yet, open it. */
		    status = qeq_part_open (rcb, dsh,
				dsh->dsh_row[node->qen_row],
				dsh->dsh_qp_ptr->qp_row_len[node->qen_row],
				partp->part_dmrix, partp->part_dmtix,
				(u_i2) partno);
		    if (status != E_DB_OK)
			return (status);
		    ++ ns->node_pcount;	/* Update stats */
		    dmr = (DMR_CB *) cbs[partp->part_dmrix + partno + 1];

		    /* Set qual function for this DMR_CB.  Let DMF do
		    ** inner qualification unless tid is involved.
		    */
		    qen_set_qualfunc(dmr, node, dsh);
		}
	    }

	    /* Ok, we can position now.  Decide how:
	    ** If this outer was a fresh key, use QUAL key positioning.
	    ** If this outer has the same key as the last outer, just
	    ** reposition to reread the same inners.
	    ** Rereading isn't allowed if qualified, partitioned inner;
	    ** the set of qualifying partitions may change from outer
	    ** to outer, and may include some partitions that weren't
	    ** probed by the first appearance of the key.  (Qualification
	    ** can depend on more than just the key!)
	    */
	    dmr->dmr_position_type = DMR_QUAL;
	    if ((ns->node_access & QEN_OUTER_NEW_KEY) == 0 && partp == NULL)
		dmr->dmr_position_type = DMR_REPOSITION;

	    dmr->dmr_flags_mask = 0;
	    status = dmf_call(DMR_POSITION, dmr);
	    /* Don't turn off do-position until we see if it worked */
	}
	if (status == E_DB_OK)
	{
	    /* No position, or the position worked, try a get.
	    ** The get might return norows, for various reasons (e.g.
	    ** qualification), but at least the key MIGHT be there.
	    */

	    do_position = FALSE;
	    ns->node_access |= QEN_KEY_IN_SOME_PART;

	    /* Set various DMF flags.  If the kjoin result will end up
	    ** in a sort, tell dmf so that it can fool with its locking
	    ** protocols.  If we can satisfy the request from a tree
	    ** index alone, light the primary-key only flag.
	    */

	    dmr->dmr_flags_mask = DMR_NEXT;
	    if (dsh->dsh_qp_status & DSH_SORT)
		dmr->dmr_flags_mask |= DMR_SORT;
	    if (node->node_qen.qen_kjoin.kjoin_ixonly)
		dmr->dmr_flags_mask |= DMR_PKEY_PROJECTION;
	    status = dmf_call(DMR_GET, dmr);
	}
	if (status == E_DB_OK)
	{
	    /* We have a row.  See if the tid needs to be calculated. */
	    if ( (offset = node->node_qen.qen_kjoin.kjoin_tid) > -1)
	    {
		bigtid.tid_i4.tpf = 0;
		bigtid.tid_i4.tid = dmr->dmr_tid;
		bigtid.tid.tid_partno = (u_i2) partno;
		output = (char *) dmr->dmr_data.data_address + offset;
		I8ASSIGN_MACRO(bigtid, *output);
	    } 
	    return (E_DB_OK);	/* Normal OK return with row */
	}

	if (dmr->error.err_code != E_DM0055_NONEXT)
	{
	    /* Non-eof error, might be DMF-handled row qualification err */
	    dsh->dsh_error.err_code = dmr->error.err_code;
	    if (dmr->error.err_code == E_DM0023_USER_ERROR
	      && dsh->dsh_qp_ptr->qp_oerrorno_offset != -1)
	    {
		i4 *iierrorno;

		/* DMF handled a row-qualifier user error, but we still need
		** to set iierrornumber while we have the info.
		*/
		iierrorno = (i4 *) (
		    (char *) (dsh->dsh_row[dsh->dsh_qp_ptr->qp_rerrorno_row])
			 + dsh->dsh_qp_ptr->qp_oerrorno_offset);
		*iierrorno = dmr->error.err_data;
	    }
	    return (status);
	}

	/* No-more-rows, either on the position, or on the get.
	** If it was on the position, note that we have a nomatch key
	** (either globally or for this partition) in case the same key
	** happens in the next outer.
	** Then either move to the next partition, or pass no-more
	** back to the caller.
	*/
	if (do_position)
	{
	    /* Nomatch on the position */
	    if (partp != NULL)
	    {
		/* Set nomatch for this partition.  Tracking partitions
		** individually ought to be a win for lots of partitions,
		** and of little moment for few partitions.
		*/
		BTset(partno, knokey_map);
	    }
	    else
	    {
		ns->node_access |= QEN_THIS_KEY_WONT_PROBE;
	    }
	}
	if (partp == NULL)
	{
	    dsh->dsh_error.err_code = E_QE0015_NO_MORE_ROWS;
	    return (E_DB_WARN);
	}

	/* Move to the next partition if there is one */
	do_position = TRUE;
    } /* for */

    /*NOTREACHED*/

} /* kjoin_inner */
