/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <st.h>
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
#include    <dmhcb.h>
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
**  Name: QENHJOIN.C - hash table(s) used to join matching rows
**
**  Description:
**      The join keys of the outer source are used to build hash table(s)
**	probed by rows of inner source to determine matching rows.
**
**          qen_hjoin - hash join
**
**
**  History:    
**      1-mar-99 (inkdo01)
**	    Cloned from qenkjoin.
**	23-feb-04 (inkdo01)
**	    Changed qef_rcb->error to dsh->dsh_error for || thread safety.
**	2-Dec-2010 (kschendel) SIR 124685
**	    Warning, prototype fixes.
**/

/* Static variables, functions, etc. */

static VOID 
hjoin_stat_dump(QEF_RCB	*rcb,
	QEE_DSH		*dsh,
	QEN_NODE	*node,
	QEN_STATUS	*qen_status,
	QEN_HASH_BASE	*hbase);

static DB_STATUS
qen_hjoin_reset(QEF_RCB	*rcb,
	QEE_DSH		*dsh,
	QEN_HASH_BASE	*hbase,
	bool		clearbufptrs);

static void qen_hjoin_cleanup(
	QEF_RCB		*rcb,
	QEE_DSH		*dsh,
	QEN_HASH_BASE	*hbase);


/*{
** Name: QEN_HJOIN	- hash join
**
** Description:
**      The join keys of the outer source are used to build hash table(s)
**	probed by rows of inner source to determine matching rows.
**
**	Hash join proceeds in several phases:
**
**	Hash build:  read the build (outer) source and partition the
**	rows by hash key.  After the outer is entirely read, we attempt
**	to build a hash table in memory.  This may succeed, partially
**	succeed (some of the build spilled), or fail (all of the build
**	side spilled with no room to put any one partition).
**
**	Probe1:  the probe (inner) side is read and we decide whether
**	the partition it hashes to is in the hash table or not.  If not,
**	the row is buffered and possibly spilled.  If yes, we match the
**	row against the hash table to see if it joins.
**
**	P1P2: this interphase handles EOF on the probe side.  Left-join
**	rows are drained from the hash table (if any).  If any rows from
**	either side were spilled to disk, the previous hash table is
**	flushed and a new one set up.
**
**	Probe2:  this phase reads rows from the spilled "probe" partition(s)
**	corresponding to the current hash table, and joins them against
**	the "build" rows in the hash table.  (I quote "probe" and "build"
**	here, because during phase 2 role-reversal can occur such that
**	the "build" rows actually came from the inner, and "probe" rows
**	came from the outer.  Care is needed.)  After reading all the
**	relevant rows, if there are more spilled partitions around, we
**	flush the old hash table, make a new one, and repeat probe2
**	until the join is complete.
**
**	Outer joins are handled by tracking an indicator for each row
**	that shows whether the row ever joined.  (Obvious cases, such as
**	a probe that doesn't match any build, can be handled without the
**	indicator).  The indicator flag is in the row header.
**	When the build side is outer, after we're done with a hash table
**	we run through the rows looking for zero indicators;  these are
**	OJ rows.
**
**	When the probe side is outer, it's a bit more complicated.  In the
**	normal case there's no need to track an indicator flag, because
**	as soon as we're done probing with a row (and have no more matches)
**	we can use a simple NOJOIN flag to determine whether the probe row
**	is an OJ row.   We still maintain the indicator flag for the probe
**	side, though, because if we have to cart-prod a partition the
**	cart-prod logic runs the nested loops with the build side as the
**	outer-loop, and probe rows get re-read.
**
**	(Note: with the indicator flag in the row header, it might be
**	simpler to use it for probes in all cases.  The above comment,
**	and code, was written when the indicator was a trailing byte,
**	which was a bit more complicated to access and maintain.)
**
** Inputs:
**      node                            the hash join node data structure
**	rcb				request block
**	    .qef_cb.qef_dsh		the data segment header
**
** Outputs:
**      rcb
**	    .error.err_code		one of the following
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
**	1-mar-99 (inkdo01)
**	    Cloned from qenkjoin.
**	jan-00 (inkdo01)
**	    Changes for outer joins, cart prods.
**	1-may-01 (inkdo01)
**	    Removed MEfills that temporarily cleaned hash key areas in
**	   preparation for moving var-length strings.
**	7-sep-01 (inkdo01)
**	    Changed NOMATCH to NOJOIN to better reflect its purpose and
**	    added better OJ logic for right joins in probe1 phase.
**	29-oct-01 (inkdo01)
**	    Fix hsh_jcount computation.
**	14-mar-02 (inkdo01)
**	    Tidy up hash structures when join is re-opened (e.g. because
**	    it's under a QP node).
**	9-aug-02 (inkdo01)
**	    Handle OJ candidates that arise from phase II when probe row
**	    matches, but is rejected by OQUAL code (fix bug 108229).
**	2-jan-03 (inkdo01)
**	    Removed noadf_mask logic in favour of QEN_ADF * tests to fix
**	    concurrency bug.
**	22-july-03 (inkdo01)
**	    Init dmh_tran_id on first (or reset) call. This used to be done
**	    in qee.c, but that is too soon (tran ID may change before query
**	    execution). Fixes bug 110603.
**	20-nov-03 (inkdo01)
**	    Add support for "open" call to allow parallel query initialization.
**	29-dec-03 (inkdo01)
**	    DSH is now parameter, "function" replaces "reset".
**	2-jan-04 (inkdo01)
**	    Added end-of-partition-grouping logic.
**	27-feb-04 (inkdo01)
**	    Changed dsh_hash from array of QEN_HASH_BASE to array of ptrs.
**	17-mar-04 (toumi01)
**	    Add support for "close" call to allow parallel query termination.
**	8-apr-04 (inkdo01)
**	    Added dsh parm to QEF_CHECK_FOR_INTERRUPT macro.
**	19-apr-04 (inkdo01)
**	    Minor fix to partition group handling.
**	28-apr-04 (inkdo01)
**	    Add support for "end of partition group" call.
**	9-june-04 (inkdo01)
**	    Heuristic to read one inner row first, so we don't read
**	    all outer rows only to determine there are no inners to
**	    join to.
**	30-Jul-2004 (jenjo02)
**	    Use dsh_dmt_id rather than qef_dmt_id transaction context.
**	16-aug-04 (toumi01)
**	    free hash memory if we are done with it to ease pressure on qef
**	    memory, especially if we are running queries in parallel
**	28-aug-04 (inkdo01)
**	    Changes to use global base array to avoid excessive moving
**	    of build and probe rows.
**	20-Sep-2004 (jenjo02)
**	    Reset hsh_dmhcb on OPEN; with child threads, dsh_cbs
**	    aren't what they used to be when qee initialized them.
**	14-oct-04 (inkdo01)
**	    Tidy up some build/probe row logic - added more fields to xDEBUG
**	    TRdisplay.
**	02-Nov-2004 (jenjo02)
**	    After freeing hash stream, clear hsh_size lest we
**	    keep accumulating it and later think we've blown through
**	    qef_h_max.
**	04-Nov-2004 (jenjo02)
**	    Only CSswitch on root DSH or if not OS-threaded.
**	21-feb-05 (inkdo01)
**	    Fix assignment of build/probe row indicators for outer join
**	    detection, broken by earlier base array changes (fixes 113918).
**	6-May-2005 (schka24)
**	    Make hash values unsigned.  Fix a few improper (i2) casts on
**	    MEcopy sizes (a relic of yore, no longer correct).
**	17-Aug-2005 (schka24)
**	    Teach join to stop if PC-joining and one side gets a real EOF,
**	    and it's not an outer join.  (since orig now goes to substantial
**	    lengths to return a true EOF indication with partition qualified
**	    PC-joins, we might as well take advantage!)
**	    Mess with debugging displays a little to help me out with
**	    PC-joining.
**	10-Nov-2005 (schka24)
**	    Rename partitioning filter flag for clarity.
**	    Don't copy bogus data around in probe1 phase unless probe1
**	    actually got something.
**	    Fix wrong-answer bug with OJ and role reversal by eliminating
**	    the use of the indicator byte on the probe side, and repurposing
**	    the NOJOIN flag to mean "probe row hasn't joined yet".
**	    The prevpr fix from 6-Apr-05 was getting confused during role
**	    reversal, and this way is easier anyway.
**	    Simplify the probe-outer logic a little by moving some code
**	    around.
**	14-Dec-2005 (kschendel)
**	    Eliminate rcb from most utility routine calls (DSH suffices).
**	12-Jan-2006 (kschendel)
**	    Update qen-execute-cx calls.
**	    Smack around build, probe loops a bit more, mainly for cosmetics.
**	16-Feb-2006 (kschendel)
**	    Return after close, don't keep going.
**	10-oct-2006 (dougi)
**	    Assure that buildptr, probeptr start off right.
**	22-Nov-2006 (kschendel) SIR 122512
**	    Do the hashing ourselves, with a different hash algorithm,
**	    to prevent resonances with table hash-partitioning.
**	13-Feb-2007 (kschendel)
**	    Replace CSswitch with better cancel check.
**	10-Apr-2007 (kschendel) SIR 122513
**	    Allow nested PC-join.
**	1-Aug-2007 (kschendel) SIR 122513
**	    Changes for join-time partition qualification; get read-inner
**	    flag from QP.
**	21-Dec-2007 (kibro01) b119634
**	    If we don't have an inner row, don't even try to perform post-
**	    matching when this is a right join since it cannot match.  
**	    This prevents a "where not exists" query erroneously identifying
**	    a dummy null row as output.
**	24-Jul-2008 (kschendel)
**	    Revert above, real problems were in probe2 and cartprod.
**	    Add quickie version of Datallegro fix to not copy garbage
**	    after probe2, if not in global basearray mode and probe2
**	    returned an outer join.
**	7-May-2008 (kschendel) SIR 122512
**	    Move OJ indicator into hash-join header.
**	    Re-type all row pointers as qen-hash-dup, which they are.
**	18-Jun-2008 (kschendel) SIR 122512
**	    Enable optional rll-compression of non-key columns in build and
**	    probe rows, to reduce memory overhead and spillage.
**	30-Jun-2008 (kschendel) SIR 122512
**	    Code bumming to speed up the hot path thru hash join slightly.
**	    Use slightly faster/more convenient dsh test for qe90, and
**	    remove pointless error returns from cost begin/end.
**	24-Mar-2009 (kschendel) b122040
**	    Add a cancel check after post-join qualification, if very
**	    many rows have failed jqual.  We might be spinning in a giant
**	    probe2 cart-prod, and if no rows ever qualify, the query can't
**	    be cancelled.
**	4-Jun-2009 (kschendel) b122118
**	    Use slightly faster/more convenient dsh test for qe90, and
**	    remove pointless error returns from cost begin/end.  Hot path
**	    streamlining.  Remove unused link level rowcounts.
**	27-Oct-2009 (kiria01) SIR 121883
**	    Scalar sub-query - Pick up on cardinality errors if requested.
**	13-Nov-2009 (kiria01+schka01) SIR 121883
**	    Rearranged the card checks to be based purely on the indicator
**	    flags and thereby also covering cart-prod.
**	17-Jan-2010 (kiria01) SIR 121883
**	    Ensure card test applied for inner joins too if needed relying
**	    on the correct orientation of the QEN node.
*/

/* Max number of joined rows that we reject due to post-join qualification,
** before doing a cancel check.  See usage for more info.
** The number is chosen to be about a second's worth of rows generated
** and rejected, using a simple post-join qual, on 2008-ish vintage hardware.
** The value is not critical in the slightest.  Grossly too small will
** waste a little CPU, too large will impact cancel responsiveness.
*/

#define ROWS_TILL_CANCELCHECK 1000000



DB_STATUS
qen_hjoin(
QEN_NODE           *node,
QEF_RCB            *rcb,
QEE_DSH		   *dsh,
i4		    function )
{
    ADE_EXCB	    *ade_excb;
    QEF_CB	    *qef_cb = dsh->dsh_qefcb;
    QEE_XADDRS	    *node_xaddrs = dsh->dsh_xaddrs[node->qen_num];
    QEN_STATUS	    *qen_status = node_xaddrs->qex_status;
    QEN_NODE	    *out_node = node->node_qen.qen_hjoin.hjn_out;
    QEN_NODE	    *in_node = node->node_qen.qen_hjoin.hjn_inner;
    QEN_HASH_BASE   *hashptr = dsh->dsh_hash[node->
					node_qen.qen_hjoin.hjn_hash];
    QEF_HASH_DUP    *buildptr = (QEF_HASH_DUP *) hashptr->hsh_brptr;
    QEF_HASH_DUP    *probeptr = (QEF_HASH_DUP *) hashptr->hsh_prptr;
    PTR		    *browpp, *prowpp;
    DB_STATUS	    status = E_DB_OK;
    DB_STATUS	    locstat;
    i4	    	    val1;
    i4	    	    val2;
    i4		    browsz = hashptr->hsh_browsz;
    i4		    prowsz = hashptr->hsh_prowsz;
    i4		    in_func = NO_FUNC;
    i4		    nojoin_count;
    QEN_OJMASK	    ojmask = 0;
    bool	    reset = FALSE;
    bool	    filterflag;
    bool	    checkinner;
    bool	    global_basearray;
    TIMERSTAT	    timerstat;
    bool	    potential_card_violation = FALSE;
    bool	    card_testing =
		    (node->qen_flags & QEN_CARD_MASK) == QEN_CARD_01R ||
		    (node->qen_flags & QEN_CARD_MASK) == QEN_CARD_01L;
/*     (VOID) qe2_chk_qp(dsh); */

    if (function != 0)
    {
	if (dsh->dsh_hash_debug)
	{
	    TRdisplay("%@ HJOIN node %d t%d got func 0x%x (%v)\n",
		node->qen_num, dsh->dsh_threadno, function,
		"MID_OPEN,TOP_OPEN,RESET,ALLOC,NEXTROW,CLOSE,EOGROUP",function);
	}

	if (function & FUNC_RESET)
	    reset = TRUE;

	/* Do open processing, if required. Only if this is the root node
	** of the query tree do we continue executing the function. */
	if ((function & TOP_OPEN || function & MID_OPEN) && 
	    !(qen_status->node_status_flags & QEN1_NODE_OPEN))
	{
	    /* Ensure correct DMH_CB */
	    hashptr->hsh_dmhcb = dsh->dsh_cbs[node->node_qen.qen_hjoin.hjn_dmhcb];
	    buildptr = hashptr->hsh_brptr = (QEF_HASH_DUP *)
		dsh->dsh_row[node->node_qen.qen_hjoin.hjn_brow];
	    probeptr = hashptr->hsh_prptr = (QEF_HASH_DUP *)
		dsh->dsh_row[node->node_qen.qen_hjoin.hjn_prow];
	    if (dsh->dsh_hash_debug)
	    {
		TRdisplay("%@ HJOIN open node %d thread %d hbase %p\n        msize %u bvsize %d browsz %d prowsz %d\n        pcount %d csize %u rbsize %u\n",
			node->qen_num, dsh->dsh_threadno,
			hashptr, hashptr->hsh_msize, hashptr->hsh_bvsize,
			browsz, prowsz,
			hashptr->hsh_pcount, hashptr->hsh_csize, hashptr->hsh_rbsize);
	    }
	    status = (*out_node->qen_func)(out_node, rcb, dsh, MID_OPEN);
	    status = (*in_node->qen_func)(in_node, rcb, dsh, MID_OPEN);
	    qen_status->node_status_flags = QEN1_NODE_OPEN;
	    if (function & MID_OPEN)
		return(E_DB_OK);
	    function &= ~TOP_OPEN;
	}
	/* Do close processing, if required. */
	if (function & FUNC_CLOSE)
	{
	    if (!(qen_status->node_status_flags & QEN8_NODE_CLOSED))
	    {
		status = (*out_node->qen_func)(out_node, rcb, dsh, FUNC_CLOSE);
		status = (*in_node->qen_func)(in_node, rcb, dsh, FUNC_CLOSE);
		qen_hjoin_cleanup(rcb, dsh, hashptr);
		qen_status->node_status_flags = 
		    (qen_status->node_status_flags & ~QEN1_NODE_OPEN) | QEN8_NODE_CLOSED;
	    }
	    return(E_DB_OK);
	}
	if (function & FUNC_EOGROUP)
	{
	    /* Handle flush-current-partition-group request.
	    ** If we're at an initial state, just pass the EOG down so that
	    ** the children skip the upcoming group and go on to the next.
	    **
	    ** If we're processing, flush out the join in progress; don't
	    ** EOG the build side since we've read it to completion already;
	    ** EOG the probe side only if we haven't finished reading them,
	    ** i.e. only if we're still in P1.
	    */
	    if (qen_status->node_status == QEN0_INITIAL)
	    {
		status = (*out_node->qen_func)(out_node, rcb, dsh, FUNC_EOGROUP);
		if (status != E_DB_OK && dsh->dsh_error.err_code == E_QE0015_NO_MORE_ROWS)
		    qen_status->node_status_flags |= QEN256_OPART_EOF;
		status = (*in_node->qen_func)(in_node, rcb, dsh, FUNC_EOGROUP);
		if (status != E_DB_OK && dsh->dsh_error.err_code == E_QE0015_NO_MORE_ROWS)
		    qen_status->node_status_flags |= QEN512_IPART_EOF;
	    }
	    else
	    {
		if (qen_status->node_u.node_join.node_inner_status == QEN13_PROBE_P1)
		{
		    status = (*in_node->qen_func)(in_node, rcb, dsh, FUNC_EOGROUP);
		    if (status != E_DB_OK && dsh->dsh_error.err_code == E_QE0015_NO_MORE_ROWS)
			qen_status->node_status_flags |= QEN512_IPART_EOF;
		}
		qen_status->node_status = QEN0_INITIAL;
		(void) qen_hjoin_reset(rcb, dsh, hashptr, FALSE);
	    }
	    /* Normally we return end-of-partition to an end-of-group request,
	    ** but if either side has reached true EOF and there's no corresponding
	    ** outer join, we can return EOF to the caller.  This gives the
	    ** caller the opportunity to stop early as well.
	    */
	    dsh->dsh_error.err_code = E_QE00A5_END_OF_PARTITION;
	    if ( ((qen_status->node_status_flags & QEN256_OPART_EOF)
		    && ((hashptr->hsh_flags & HASH_WANT_RIGHT) == 0))
		  || ((qen_status->node_status_flags & QEN512_IPART_EOF)
		    && ((hashptr->hsh_flags & HASH_WANT_LEFT) == 0)) )
	    {
		dsh->dsh_error.err_code = E_QE0015_NO_MORE_ROWS;
	    }
	    return(E_DB_WARN);
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
    status = E_DB_OK;
    checkinner = FALSE;		/* May turn on if reset or initial state */

    /* Reset buildptr, probeptr to those of previous call if we're
    ** using global base arrays. */
    global_basearray = (dsh->dsh_qp_ptr->qp_status & QEQP_GLOBAL_BASEARRAY) != 0;
    if (global_basearray)
    {
	browpp = (PTR *) &dsh->dsh_row[node->node_qen.qen_hjoin.hjn_brow];
	prowpp = (PTR *) &dsh->dsh_row[node->node_qen.qen_hjoin.hjn_prow];
	buildptr = (QEF_HASH_DUP *) (*browpp);
	probeptr = (QEF_HASH_DUP *) (*prowpp);
    }

    /*
    ** If forced reset, reset the node status.  This makes it go through
    ** the "first time" routine. Must also reset lots of stuff in the 
    ** hash structures. This can happen if the hash join is opened more
    ** than once in a plan execution - as when it sits under a QP node
    ** or for each grouping of a partition compatible join.
    ** We also come back here to run the next PC-join after an end-of-group,
    ** when this join is at the top (ie not passing EOG's further up).
    */
loop_reset:
    if (reset)
    {
	qen_status->node_status = QEN0_INITIAL;
	status = qen_hjoin_reset(rcb, dsh, hashptr, FALSE);
	if (status != E_DB_OK)
	    goto errexit;
	if (node->node_qen.qen_hjoin.hjn_pqual != NULL)
	    qeq_join_pqreset(dsh, node->node_qen.qen_hjoin.hjn_pqual);
	in_func = FUNC_RESET;
    }

    /* If the join is now in an initial state, load the build side.
    ** We also do the read-inner heuristic if requested.
    */
    if(qen_status->node_status == QEN0_INITIAL)
    {
	ADE_EXCB	*bmat_excb;
	bool		try_compress;
	DMH_CB		*dmhcb = (DMH_CB *)hashptr->hsh_dmhcb;
	i4		out_func = NO_FUNC;

	dmhcb->dmh_tran_id = dsh->dsh_dmt_id;
	qen_status->node_access = 0;
	if (reset)
	    out_func = FUNC_RESET;
	probeptr = hashptr->hsh_prptr;
	buildptr = hashptr->hsh_brptr;
				/* make sure it is correct */
	/* Turn off group-end flags, might be start of a new group */
	qen_status->node_status_flags &=
			~(QEN2_OPART_END | QEN4_IPART_END);

	checkinner = node->node_qen.qen_hjoin.hjn_innercheck;
	if (checkinner)
	{
	    /* Heuristic that checks inner for any rows before we read
	    ** from outer. This enables us to avoid reading the outer
	    ** source and loading hash table/spooling overflow to disk
	    ** when there are no inner rows to process anyway. 
	    **
	    ** It only handles non-left joins for now, but left join
	    ** support will be added in the future. */

	    status = (*in_node->qen_func)(in_node, rcb, dsh, in_func);
	    in_func = NO_FUNC;

	    if (status != E_DB_OK)
	    {
		if (dsh->dsh_error.err_code == E_QE0015_NO_MORE_ROWS ||
		    (dsh->dsh_error.err_code == E_QE00A5_END_OF_PARTITION &&
		     node->qen_flags & QEN_PART_SEPARATE))
		{
		    if (dsh->dsh_hash_debug)
		    {
			TRdisplay("hjoin: probe-trial node: %d error: %s thread: %d\n        rcount: %u brcount: %ld prcount: %ld jcount: %u\n",
				node->qen_num,
				(dsh->dsh_error.err_code == E_QE0015_NO_MORE_ROWS) ?
				"EOF" : "EOG",
				dsh->dsh_threadno, qen_status->node_rcount,
				hashptr->hsh_brcount, hashptr->hsh_prcount,
				hashptr->hsh_jcount);
		    }
		    /* If partition compatible join, set flags, force end of
		    ** group on build source and go to error exit to loop
		    ** on next group. */
		    if (dsh->dsh_error.err_code == E_QE0015_NO_MORE_ROWS)
			qen_status->node_status_flags |= QEN512_IPART_EOF;
		    else
		    {
			qen_status->node_status_flags |= QEN4_IPART_END;
			status = (*out_node->qen_func)(out_node, rcb, dsh, 
					    FUNC_EOGROUP);
			if (status != E_DB_OK)
			{
			    if (dsh->dsh_error.err_code == E_QE00A5_END_OF_PARTITION)
				qen_status->node_status_flags |= QEN2_OPART_END;
			    else if (dsh->dsh_error.err_code == E_QE0015_NO_MORE_ROWS)
				qen_status->node_status_flags |= QEN256_OPART_EOF;
			}
		    }

		    qen_status->node_u.node_join.node_inner_status = 
							    QEN11_INNER_ENDS;
		    /* If QE0015, this exits whole join. */
		}
		goto errexit;  /* handle eof, eog, or error */
	    }
	}

	/* Now load up the build side. */

	/* The build side is "outer" in the xaddrs struct */
	bmat_excb = node_xaddrs->qex_otmat;
	try_compress = (hashptr->hsh_flags & HASH_BUILD_RLL) != 0;
	qen_status->node_status = QEN1_EXECUTED;
	for ( ; ; )	/* big loop to drive load of build source */
	{
    newouter:
	    status = (*out_node->qen_func)(out_node, rcb, dsh, out_func);
	    out_func = NO_FUNC;
	    if (status != E_DB_OK)
		break;			/* Go figure it out */

	    /* Materialize the row into buffer, preparing the hash
	    ** key at the front.  (coerce/trim/prepare the hash key as
	    ** needed.  Don't need to worry about NULL since nulls
	    ** don't join, and we pick up on that later on.)
	    */
	    status = qen_execute_cx(dsh, bmat_excb);
	    if (status != E_DB_OK)
		goto errexit;	/* if ADF error, we're dead */
	    /* and now for the actual hash value */
	    qen_hash_value(buildptr, hashptr->hsh_keysz);

	    buildptr->hash_oj_ind = 0;	/* init oj join indicator */
	    buildptr->hash_compressed = 0;  /* Not (yet) compressed */
	    QEFH_SET_ROW_LENGTH_MACRO(buildptr, browsz);

	    /* Got row from outer source - send it to hash table build.
	    ** Also, apply row to table partition qualifications if
	    ** we're doing that.
	    */
	    if (node->node_qen.qen_hjoin.hjn_pqual != NULL)
	    {
		status = qeq_join_pquals(dsh, node->node_qen.qen_hjoin.hjn_pqual);
		if (status != E_DB_OK) goto errexit;
	    }
	    hashptr->hsh_brcount++;		/* incr build row count */
	    filterflag = FALSE;
	    status = qen_hash_partition(dsh, hashptr, buildptr,
				&filterflag, try_compress);
	    if (status != E_DB_OK) goto errexit;
	}

	/* Analyze error exit out of above loop */
	if (dsh->dsh_error.err_code == E_QE0015_NO_MORE_ROWS ||
	    (dsh->dsh_error.err_code == E_QE00A5_END_OF_PARTITION &&
	     node->qen_flags & QEN_PART_SEPARATE))
	{
	    if (dsh->dsh_hash_debug)
	    {
		TRdisplay("hjoin: build node: %d error: %s thread %d\n        rcount: %u brcount: %ld prcount: %ld jcount: %u\n",
			node->qen_num,
			(dsh->dsh_error.err_code == E_QE0015_NO_MORE_ROWS) ?
			"EOF" : "EOG",
			dsh->dsh_threadno, qen_status->node_rcount,
			hashptr->hsh_brcount, hashptr->hsh_prcount,
			hashptr->hsh_jcount);
	    }
	    /* EOF or EOG is the expected "error" */
	    if (dsh->dsh_error.err_code == E_QE00A5_END_OF_PARTITION)
		qen_status->node_status_flags |= QEN2_OPART_END;
	    else
		qen_status->node_status_flags |= QEN256_OPART_EOF;

	    /* Wrap up join partition qual stuff if we're doing it */
	    if (node->node_qen.qen_hjoin.hjn_pqual != NULL)
	    {
		qeq_join_pqeof(dsh, node->node_qen.qen_hjoin.hjn_pqual);
	    }

	    qen_status->node_u.node_join.node_outer_status = QEN8_OUTER_EOF;
	    qen_status->node_u.node_join.node_inner_status = QEN13_PROBE_P1;
	    if (hashptr->hsh_brcount == 0)
	    {
		if(!(hashptr->hsh_flags & HASH_WANT_RIGHT))
		{
		    /* No build rows and no right/full join.
		    ** Send EOG to probe source if PC join, then quit.
		    */
		    if (node->qen_flags & QEN_PART_SEPARATE)
		    {
			status = (*in_node->qen_func)(in_node, rcb, dsh, 
				    FUNC_EOGROUP);
			if (status != E_DB_OK)
			{
			    if (dsh->dsh_error.err_code == E_QE00A5_END_OF_PARTITION)
				qen_status->node_status_flags |= QEN4_IPART_END;
			    else if (dsh->dsh_error.err_code == E_QE0015_NO_MORE_ROWS)
				qen_status->node_status_flags |= QEN512_IPART_EOF;
			}
		    }
		    goto errexit;
		}	/* no build rows & no right/full join */
		else 
		{
		    /* Have to process probes if right/full join */
		    /* Fall out of if's to process probes */
		    dsh->dsh_error.err_code = E_QE0000_OK;
		    hashptr->hsh_flags |= HASH_NO_BUILDROWS;
		}
	    }
	    else
	    {
		/* Got build rows, this is the normal continuation.
		** Reset QEF's error code before doing hash build, then
		** build first (or only) hash table.
		*/
		dsh->dsh_error.err_code = E_QE0000_OK;
		status = qen_hash_build(dsh, hashptr);
		/* If we couldn't build a hash table, indicate that we
		** are partitioning probes in Phase1.  This is a nano-
		** optimization so that probe1 doesn't have to set reverse
		** for every row going thru into the partitioner.
		*/
		if (hashptr->hsh_flags & HASH_NOHASHT)
		    hashptr->hsh_reverse = TRUE;
		if (status != E_DB_OK) goto errexit;
	    }
	}
	else if (dsh->dsh_error.err_code == E_QE00A5_END_OF_PARTITION)
	{
	    /* No more rows in partitioning group - read from 
	    ** next group.  bmat_excb still set up.
	    ** This shouldn't happen, is an error by lower level of QP.
	    */
	    out_func = FUNC_RESET;
	    goto newouter;
	}
	else
	{
	    goto errexit;  /* else return error */ 
	}
	/* Before entering probe1 loop, set up ojmask to avoid needing to
	** recompute it on every row.  Since probe1 reads the probe/inner
	** side, set it for right OJ's.
	*/
	status = E_DB_OK;		/* Just make sure ... */
	hashptr->hsh_pojmask = 0;
	if (hashptr->hsh_flags & HASH_WANT_RIGHT)
	    hashptr->hsh_pojmask = OJ_HAVE_RIGHT;
    } /* initial setup */



    nojoin_count = 0;
    for ( ; ; )		/* Biiiig loop to cover whole join process */
    {
	QEF_HASH_DUP *orig_buildptr, *orig_probeptr;

	/* Build (outer) source has been passed and turned into one or more
	** partitions. One of these is in a memory resident hash table. 
	** Phase 1 of the probe operation reads the rows of the probe (inner)
	** source. If the row targets the memory resident (and hashed) 
	** partition, an attempt is made to join it now. Otherwise, it is 
	** written to a probe partition on disk matching the build partition 
	** it hashes to. */

	if (qen_status->node_u.node_join.node_inner_status == QEN13_PROBE_P1)
	{
	    ADE_EXCB	*pmat_excb;
	    bool	gotone;

	    /* The probe materialize CX is "inner" */
	    pmat_excb = node_xaddrs->qex_itmat;
	    for ( ; ; )	/* big loop until a match is made, or eof/eog */
	    {

		/* Don't need another inner if last trip thru said that
		** there are multiple build rows with the same key, try
		** for more matches with the same probe.  Normal case is
		** hash-probe-match is NOT set and we get another probe.
		*/
		if ((hashptr->hsh_flags & HASH_PROBE_MATCH) == 0)
		{
		    if (!checkinner)
		    {
			/* checkinner means the 1st probe row has been read and 
			** is in buffers waiting for processing. */
			status = (*in_node->qen_func)(in_node, rcb, dsh, in_func);
			in_func = NO_FUNC;
			if (status != E_DB_OK)
			    break;		/* Go figure it out */
		    }

		    checkinner = FALSE;	/* read all the rest */
		    /* Materialize the row into buffer (preparing hash key). */
		    status = qen_execute_cx(dsh, pmat_excb);
		    if (status != E_DB_OK)
			goto errexit;	/* if ADF error, we're dead */
		    /* and now for the actual hash value */
		    qen_hash_value(probeptr, hashptr->hsh_keysz);

		    hashptr->hsh_flags |= HASH_PROBE_NOJOIN;
		    probeptr->hash_oj_ind = 0; /* init oj join indicator */
		    probeptr->hash_compressed = 0;  /* Not (yet) compressed */
		    QEFH_SET_ROW_LENGTH_MACRO(probeptr, prowsz);

		    /* This is not executed because it depends on the setting
		    ** of the null indicator for the hash result, which doesn't
		    ** currently happen (but it'd be nice if it did!).
		    ** if (probe-row-shows-null-key && hashptr->hsh_flags & HASH_WANT_RIGHT)
		    ** {
			** Hash key is null - automatic rjoin candidate. **
			** ojmask |= OJ_HAVE_RIGHT;
			** gotone = TRUE;
			** break;
		    ** } */

		    hashptr->hsh_prcount++;		/* incr probe row count */
		}
		orig_probeptr = probeptr;

		if (hashptr->hsh_flags & HASH_NO_BUILDROWS) 
		{
		    /* no build rows - and right/full join. */
		    ojmask |= OJ_HAVE_RIGHT;
		    break;
		}
		else 
		{
		    /* Use probe (inner) row to probe build.
		    ** If the probe row joins here, it never gets compressed,
		    ** but the matching build row (if any) might be.
		    */
		    status = qen_hash_probe1(dsh, hashptr, &buildptr, 
				probeptr, &ojmask, &gotone);
		    if (status != E_DB_OK) goto errexit;

		    if (gotone) 
		    {
			/* buildptr is bogus if right-OJ (the only kind
			** that we can see during P1), so null it out.
			** Nothing should be referencing it, best we
			** find out if something does.
			*/
			if (ojmask != 0)
			{
			    buildptr = NULL;	/* Must be right-OJ */
			}
			else
			{
			    hashptr->hsh_jcount++;
			    orig_buildptr = buildptr;
			    if (buildptr->hash_compressed)
			    {
				buildptr = qen_hash_unrll(hashptr->hsh_keysz,
					buildptr,
					(PTR) hashptr->hsh_brptr);
			    }
			    if (global_basearray)
			    {
				/* With global base array, just update row ptr array. */
				*browpp = (PTR) buildptr;
			    }
			    else if (buildptr != hashptr->hsh_brptr)
			    {
				/* Without global base array, copy to row buffer. */
				MEcopy((PTR) buildptr, browsz, hashptr->hsh_brptr);
			    }
			}
			break;	/* Proceed with join */
		    }
		    /* !gotone, no match, try another */
		}
	    } /* P1 loop */

	    if (status != E_DB_OK)
	    {
		/* If we fell out with nonzero error status, must have
		** been EOF, EOG, or bad problem, see which
		*/
		if (dsh->dsh_error.err_code == E_QE0015_NO_MORE_ROWS ||
		    (dsh->dsh_error.err_code == E_QE00A5_END_OF_PARTITION &&
		     node->qen_flags & QEN_PART_SEPARATE))
		{
		    if (dsh->dsh_hash_debug)
		    {
			TRdisplay("hjoin: probe node: %d error: %s thread %d\n        rcount: %u brcount: %ld prcount: %ld jcount: %u\n",
				node->qen_num,
				(dsh->dsh_error.err_code == E_QE0015_NO_MORE_ROWS) ?
				"EOF" : "EOG", dsh->dsh_threadno,
				qen_status->node_rcount, hashptr->hsh_brcount,
				hashptr->hsh_prcount, hashptr->hsh_jcount);
		    }
		    if (dsh->dsh_error.err_code == E_QE00A5_END_OF_PARTITION)
			qen_status->node_status_flags |= QEN4_IPART_END;
		    else
			qen_status->node_status_flags |= QEN512_IPART_EOF;

		    qen_status->node_u.node_join.node_inner_status = 
				QEN14_PROBE_P2;
		    /* "End" any in-hash or useless partitions.
		    ** Indicate that all probes are read, and
		    ** flush them out.
		    */
		    hashptr->hsh_flags |= HASH_BETWEEN_P1P2;
		    if (hashptr->hsh_prcount == 0
		      && (hashptr->hsh_brcount == 0 || 
			    !(hashptr->hsh_flags & HASH_WANT_LEFT)) )
			goto errexit;	
				/* no inner rows & no outers or
				** no left/full join => finished */
		    if (hashptr->hsh_flags & HASH_NO_BUILDROWS) 
			goto errexit;	/* right joins are done in
					** p1 calls for NO_BUILD */
		    if (hashptr->hsh_flags & HASH_WANT_LEFT && 
			!(hashptr->hsh_flags & HASH_NOHASHT))
		    {
			/* rescan hash table for unjoined outers. */
			hashptr->hsh_currbucket = -1;
			hashptr->hsh_flags |= HASH_BUILD_OJS;
		    }
		    else hashptr->hsh_flags |= HASH_NOHASHT;
				/* tell P2 to start another hasht */
		    dsh->dsh_error.err_code = 0;
				/* reset error for P2 */
		}
		else
		    goto errexit;  /* else return error */ 
	    } /* if status != ok */

	}   /* End of if probe phase 1 */

	/* Phase 2 probe occurs if the build source required partitioning
	** onto disk because it was too large to fit into a single 
	** memory resident hash table. This results in the probe source
	** also being partitioned onto disk. Phase 1 does the partitioning,
	** as well as the matching of probe rows which have joins in the
	** initial build source hash table, if there is one.
	** Phase 2 reloads the remaining build source partitions into
	** a hash table, and hashes the rows of the corresponding probe
	** partitions into them, returning any joins found in the process. 
	** Phase 2 repeats building hash tables until everything has been
	** processed.
	**
	** Phase 2 also returns any right join candidates (probe rows
	** which didn't find a match) and left join candidates (leftover
	** build rows from current hash table), as required by the join 
	** type. */

	if (qen_status->node_u.node_join.node_inner_status == QEN14_PROBE_P2)
	{
	    /* If we're in P2 now, continue with previously matched row
	    ** (if PROBE_MATCH set), or fetch a fresh probe and match it up.
	    ** no for-loop required - each call should return a row.
	    */
	    status = qen_hash_probe2(dsh, hashptr, &buildptr, &probeptr, 
					&ojmask);
	    if (status != E_DB_OK) goto errexit;
	    /* Row bits are who knows where, so move to proper place.
	    ** Or, with global base array, just update row ptrs.
	    ** To complicate things, either build or probe ptr might be
	    ** undefined if we got an outer-join.
	    ** Build side is valid if not a right join.
	    ** Probe side is valid if not a left join.
	    */
	    if ((ojmask & OJ_HAVE_RIGHT) == 0)
	    {
		orig_buildptr = buildptr;
		if (buildptr->hash_compressed)
		{
		    buildptr = qen_hash_unrll(hashptr->hsh_keysz,
				buildptr, (PTR) hashptr->hsh_brptr);
		}
		if (global_basearray)
		    *browpp = (PTR) buildptr;
		else if (buildptr != hashptr->hsh_brptr)
		    MEcopy((PTR) buildptr, browsz, hashptr->hsh_brptr);
	    }
	    if ((ojmask & OJ_HAVE_LEFT) == 0)
	    {
		orig_probeptr = probeptr;
		if (probeptr->hash_compressed)
		{
		    probeptr = qen_hash_unrll(hashptr->hsh_keysz,
				probeptr, (PTR) hashptr->hsh_prptr);
		}
		if (global_basearray)
		{
		    *prowpp = (PTR) probeptr;
		}
		else if (probeptr != hashptr->hsh_prptr)
		{
		    /* It would be nice to set probeptr to prptr to re-use
		    ** this copied probe row, but if the same probe matches
		    ** again next call, we'll lose track of the original.
		    ** This doesn't matter *UNLESS* it's a cart-prod with
		    ** probe-side outer join!  and it's not worth testing for.
		    ** These days, global-basearray rules anyway.
		    */
		    MEcopy((PTR) probeptr, prowsz, hashptr->hsh_prptr);
		}
	    }
	    if (ojmask == 0)
		hashptr->hsh_jcount++;
	}

	/* We really ought to have a row now. Do the rest of the join
	** stuff (ADF exprs, etc.) and either stay in big loop, or break
	** to return to caller. */

	/* Check for outer join rows returned. If left join, project
	** right source nulls. If right join, project left source
	** nulls. Otherwise execute remaining "on" clause. */
	if (hashptr->hsh_flags & (HASH_WANT_LEFT | HASH_WANT_RIGHT) ||
	    card_testing)
	{
	    if (ojmask == 0)
	    {
		/* Don't know about row yet, run the oqual.
		** (if no oqual, it's equivalent to TRUE, ie inner join).
		*/
 
		ade_excb = node_xaddrs->qex_onqual;
		if (status = qen_execute_cx(dsh, ade_excb))
		    goto errexit;
		if (ade_excb != NULL && ade_excb->excb_value != ADE_TRUE) 
		{
		    /* If row might have more matches in hash table,
		    ** just loop since it might join one of them.
		    ** (and if it doesn't after all, the prober will
		    ** announce outer-join if appropriate.)
		    **
		    ** Note: this continue, and the one immediately below,
		    ** could potentially head for the inner loop cancel-
		    ** check instead, but it doesn't take that long to run
		    ** through the hash table even if they're all identical.
		    */
		    if (hashptr->hsh_flags & HASH_PROBE_MATCH)
			continue;

		    /* Failed oqual and no more chances, if probe never
		    ** joined, then we can pronounce outer join.
		    */
		    if (!hashptr->hsh_reverse
		      && (hashptr->hsh_flags & (HASH_WANT_RIGHT | HASH_PROBE_NOJOIN))
			    == (HASH_WANT_RIGHT | HASH_PROBE_NOJOIN) )
			ojmask |= OJ_HAVE_RIGHT;
		    else if (hashptr->hsh_reverse
		      && (hashptr->hsh_flags & (HASH_WANT_LEFT | HASH_PROBE_NOJOIN))
			    == (HASH_WANT_LEFT | HASH_PROBE_NOJOIN) )
			ojmask |= OJ_HAVE_LEFT;
		    else
			continue;
		}
	    }
	    if (ojmask & OJ_HAVE_LEFT)
	    {
		if (status = qen_execute_cx(dsh, node_xaddrs->qex_lnull))
		    goto errexit;
	    }
	    else if (ojmask & OJ_HAVE_RIGHT)
	    {
		if (status = qen_execute_cx(dsh, node_xaddrs->qex_rnull))
		    goto errexit;
	    }
	    else 
	    {
		/* build and probe inner-joined */
		if (status = qen_execute_cx(dsh, node_xaddrs->qex_eqmat))
		    goto errexit;

		if ((node->qen_flags & QEN_CARD_MASK) == QEN_CARD_01L &&
				orig_probeptr->hash_oj_ind == 1 ||
		    (node->qen_flags & QEN_CARD_MASK) == QEN_CARD_01R &&
				orig_buildptr->hash_oj_ind == 1)
		    potential_card_violation = TRUE;
		else
		    potential_card_violation = FALSE;
		/* Set OJ indicators in hash table (to eliminate these guys as 
		** outer join candidates).  Note that we set the bits in
		** the original row, not a copied or un-compressed copy!
		** Normally we only need the OJ flag on the build side, so
		** this ensures that we light the flag on the row in the
		** hash table and not an uncompression copy.
		**
		** (The special case of cart-prod which can return rows in
		** workrow or workrow2 is OK;  cart-prod is driven by its
		** outer side, and for an inner side OJ cart-prod has
		** flag writeback machinery for dealing with it.  See
		** the COPY-WORK-OJ business in cart-prod.)
		*/
		orig_buildptr->hash_oj_ind = 1;
		orig_probeptr->hash_oj_ind = 1;  /* In case cart-proding */
		hashptr->hsh_flags &= ~HASH_PROBE_NOJOIN;
	    }
	} /* if outer join */

	/* Post-qualify the row (e.g. for inequalities), toss the joined
	** row and keep looking if post-qual fails.
	*/
	if (!(ade_excb = node_xaddrs->qex_jqual) ||
	    !(status = qen_execute_cx(dsh, ade_excb)) &&
		ade_excb->excb_value == ADE_TRUE)
	{
	    if (potential_card_violation)
	    {
		/* We only want to act on seltype violation after
		** qualification and that is now. */
		qen_status->node_status = QEN7_FAILED;
		dsh->dsh_error.err_code = E_QE004C_NOT_ZEROONE_ROWS;
		status = E_DB_ERROR;
		goto errexit;
	    }
	    break;	/* got a match and can return it */
	}
	else if (ade_excb && status)
	{
	    goto errexit;
	}

	/* Here if the row didn't qualify.  Just continue the loop attempting
	** to produce more joins, but first, do a cancel check if we've
	** generated and discarded "too many" rows.  The need for this
	** cancel check only comes about in probe2, if the join keys are
	** heavily duplicated (e.g. a partition cart-prod), and the post-
	** join qualification eliminates many rows.  (The user query
	** that provoked this change joined two 125,000 row inputs,
	** all with identical join keys, and the post-join qualification
	** rejected all 15 billion joined rows!)
	**
	** In probe1, the probe row fetch will do any needed cancel
	** checking, but it's not worth making the extra test here.
	*/
	if (++nojoin_count >= ROWS_TILL_CANCELCHECK)
	{
	    CScancelCheck(dsh->dsh_sid);
	    if (QEF_CHECK_FOR_INTERRUPT(qef_cb, dsh) == E_DB_ERROR)
		return (E_DB_ERROR);
	    nojoin_count = 1;
	}
    }	/* end of loop which finds joins */

    /* We get here with a successful join. Final tidying before it's
    ** returned to caller. First though, if existence only join, force
    ** read of next probe row. */
    if (node->node_qen.qen_hjoin.hjn_kuniq)
	hashptr->hsh_flags &= ~HASH_PROBE_MATCH;

    /* Compute function attrs if any needed */

    status = qen_execute_cx(dsh, node_xaddrs->qex_fatts);

errexit:

    if (status != E_DB_OK)
    {
	locstat = qen_hash_closeall(hashptr);
	if (ult_check_macro(&qef_cb->qef_trace, 93, &val1, &val2))
	    hjoin_stat_dump(rcb, dsh, node, qen_status, hashptr);

	if ((dsh->dsh_error.err_code == E_QE0015_NO_MORE_ROWS ||
	    dsh->dsh_error.err_code == E_QE00A5_END_OF_PARTITION) &&
	    (qen_status->node_status_flags & 
			(QEN2_OPART_END | QEN4_IPART_END)))
	{
	    /* One side got end-of-group, but before moving on, see if
	    ** we reached true EOF on some input (and don't need outer
	    ** join on the other side).  If that's the case, the
	    ** join is over.
	    ** "if (! (join over)) ... "
	    */
	    if (! ( ((qen_status->node_status_flags & QEN256_OPART_EOF)
		    && ((hashptr->hsh_flags & HASH_WANT_RIGHT) == 0))
		  || ((qen_status->node_status_flags & QEN512_IPART_EOF)
		    && ((hashptr->hsh_flags & HASH_WANT_LEFT) == 0)) ) )
	    {

		if (dsh->dsh_hash_debug)
		{
		    TRdisplay("hjoin: reached end-of-group node: %d thread %d\n        rcount: %u brcount: %ld prcount: %ld jcount: %u\n",
			    node->qen_num, dsh->dsh_threadno,
			    qen_status->node_rcount, hashptr->hsh_brcount,
			    hashptr->hsh_prcount, hashptr->hsh_jcount);
		}

		/* End of partitioning group, not yet end of join */
		qen_status->node_status_flags &=
			    ~(QEN2_OPART_END | QEN4_IPART_END);
		if ((node->qen_flags & QEN_PART_NESTED) == 0)
		{
		    /* Top level PC-join, just keep going on next group */
		    reset = TRUE;
		    goto loop_reset;
		}
		/* Feed end-of-group indication to caller.
		** Reset to clean everything up, and set initial state so
		** that end-of-group handler knows what to do.
		*/
		qen_status->node_status = QEN0_INITIAL;
		status = qen_hjoin_reset(rcb, dsh, hashptr, FALSE);
		if (status == E_DB_OK)
		    dsh->dsh_error.err_code = E_QE00A5_END_OF_PARTITION;
	    }
	    else
	    {
		/* We want to stop the join, set things for cleanup below */
		dsh->dsh_error.err_code = E_QE0015_NO_MORE_ROWS;
	    }
	    status = E_DB_WARN;
	}

	/*
	** free hash memory if we are done with it to ease pressure on qef
	** memory, especially if we are running queries in parallel
	*/
	if (status == E_DB_WARN && dsh->dsh_error.err_code == E_QE0015_NO_MORE_ROWS)
	{
	    qen_hjoin_cleanup(rcb, dsh, hashptr);
	}
    } /* if status notok */

    if (dsh->dsh_qp_stats)
    {
	/* QE90 accumulation for this call. */
	qen_ecost_end(dsh, &timerstat, qen_status);
    }

    /* Increment the count of rows that this node has returned */
    if (status == E_DB_OK)
    {
	qen_status->node_rcount++;
	/* Check for / print QE99/QE100 tracing stuff */
	if (node->qen_prow != NULL &&
	    (ult_check_macro(&qef_cb->qef_trace, 100+node->qen_num,
				&val1, &val2) ||
		ult_check_macro(&qef_cb->qef_trace, 99,
				&val1, &val2) ) )
	{
	    (void) qen_print_row(node, rcb, dsh);
	}
    }

    return (status);
}


/*{
** Name: HJOIN_STAT_DUMP	- dumps some hash join stats per qe93
**
** Description:
**      Implements tracepoint qe93 by dumping selected information
**	concerning a hash join. 
**
** Inputs:
**      node                            the hash join node data structure
**	rcb				request block
**	    .qef_cb.qef_dsh		the data segment header
**	hbase				the hash join control structure
**
** Outputs:
**      none
**					
**	Returns:
**	    nothing
**	Exceptions:
**	    none
**
** History:
**	jan-00 (inkdo01)
**	    Written.
**	25-oct-01 (inkdo01)
**	    Added "known" row count to display.
**	22-Jun-2005 (schka24)
**	    Add more stuff to stats output.
**	6-Mar-2007 (kschendel)
**	    Add read buffer size.
**	24-Sep-2007 (kschendel) SIR 122512
**	    Fiddle with the output some more.
**      12-May-2010 (kschendel) Bug 123720
**          Take qef_trsem for valid parallel-query output.
*/
static VOID 
hjoin_stat_dump(QEF_RCB	*rcb,
	QEE_DSH		*dsh,
	QEN_NODE	*node,
	QEN_STATUS	*qen_status,
	QEN_HASH_BASE	*hbase)

{
    QEF_CB	    *qef_cb = dsh->dsh_qefcb;
    char	    *cbuf = qef_cb->qef_trfmt;
    i4		    cbufsize = qef_cb->qef_trsize;


    CSp_semaphore(1, &qef_cb->qef_trsem);
    STprintf(cbuf, "Hash join summary for node %d thread %d\n",
		node->qen_num, dsh->dsh_threadno);
    qec_tprintf(rcb, cbufsize, cbuf);
    STprintf(cbuf, "bmem size hint: %d (%d rows);  build,probe rowsz: %d,%d\n",
	node->node_qen.qen_hjoin.hjn_bmem,
	node->node_qen.qen_hjoin.hjn_bmem / hbase->hsh_browsz,
	hbase->hsh_browsz, hbase->hsh_prowsz);
    qec_tprintf(rcb, cbufsize, cbuf);
    STprintf(cbuf, "Memory estimated (worst-case): %d; allocated: %d\n",
	hbase->hsh_msize, hbase->hsh_size);
    qec_tprintf(rcb, cbufsize, cbuf);
    STprintf(cbuf, "Partitions: %d;  write, read sizes: %d,%d @ %dK\n",
	hbase->hsh_pcount, hbase->hsh_csize, hbase->hsh_rbsize,
	hbase->hsh_pagesize / 1024);
    qec_tprintf(rcb, cbufsize, cbuf);
    /* We allocate an extra slot for some unknown reason, but the hash divide
    ** doesn't use it.  Hence the +1 discrepancy here.
    */
    STprintf(cbuf, "Hash-ptr entries: %d (%d kb)\n",
	hbase->hsh_htsize, ((hbase->hsh_htsize+1) * sizeof(PTR))/1024);
    qec_tprintf(rcb, cbufsize, cbuf);
    STprintf(cbuf, "Build row count: %ld, probe row count: %ld, result row count: %u\n",
	hbase->hsh_brcount, hbase->hsh_prcount, qen_status->node_rcount);
    qec_tprintf(rcb, cbufsize, cbuf);
    STprintf(cbuf, "Joined row count: %u, file count: %d, cartprod count: %d\n",
	hbase->hsh_jcount, hbase->hsh_fcount, hbase->hsh_ccount);
    qec_tprintf(rcb, cbufsize, cbuf);
    STprintf(cbuf, "Max. recursion level: %d, filter discards: %u\n",
	hbase->hsh_maxreclvl, hbase->hsh_bvdcnt);
    qec_tprintf(rcb, cbufsize, cbuf);
    STprintf(cbuf, "Build kb spilled: %u, probe kb spilled: %u\n",
	hbase->hsh_bwritek, hbase->hsh_pwritek);
    qec_tprintf(rcb, cbufsize, cbuf);
    STprintf(cbuf, "Repartitions: %d, unspilled ht-build partitions: %d\n\n",
	hbase->hsh_rpcount, hbase->hsh_memparts);
    qec_tprintf(rcb, cbufsize, cbuf);
    CSv_semaphore(&qef_cb->qef_trsem);

}


/*{
** Name: QEN_HJOIN_RESET	- reset hash structures upon recall
**
** Description:
**      When hash join is opened multiple times in a plan execution
**	(as when it is under a QP node), must reset various ptrs, counters,
**	flags for it to execute properly. We avoid reallocation of memory, 
**	though.
**
** Inputs:
**	rcb				request block
**	    .qef_cb.qef_dsh		the data segment header
**	hbase				the hash join control structure
**	clearbufptrs			TRUE if hash stream will be released,
**					clear out pointers.
**
** Outputs:
**      none
**					
**	Returns:
**	    nothing
**	Exceptions:
**	    none
**
** History:
**	14-mar-02 (inkdo01)
**	    Added "known" row count to display.
**	30-Jul-2004 (jenjo02)
**	    Use dsh_dmt_id rather than qef_dmt_id transaction context.
**	30-aug-04 (inkdo01)
**	    Add flag to clear ptrs after closestream.
**	6-apr-05 (inkdo01)
**	    Init. hsh_prevpr.
**	13-Jun-2005 (schka24)
**	    Got rid of first-build flag.
**	14-Nov-2005 (schka24)
**	    Remove prevpr, fixed differently.
**	11-Jun-2008 (kschendel) SIR 122512
**	    Reflect minor data structure changes here.
**	25-Nov-2008 (kibro01) b121244
**	    Clean up rbptr, which is allocated from the hsh_streamid stream.
*/
static DB_STATUS
qen_hjoin_reset(QEF_RCB	*rcb,
	QEE_DSH		*dsh,
	QEN_HASH_BASE	*hbase,
	bool		clearbufptrs)

{
    DMH_CB	    *dmhcb;
    QEN_HASH_LINK   *hlnk;
    QEN_HASH_PART   *hpart;

    DB_STATUS	    status = E_DB_OK;
    u_i4	    j;


    /* Tidy up the hash structure. */
    if (clearbufptrs)
    {
	hbase->hsh_htable = NULL;
	hbase->hsh_extra = NULL;
	hbase->hsh_extra_count = 0;
	hbase->hsh_rbptr = NULL;
    }
    /* (don't clean file pointers, closeall will) */
    dsh->dsh_row[hbase->hsh_nodep->node_qen.qen_hjoin.hjn_brow] = (PTR) hbase->hsh_brptr;
				/* reset default buffer ptrs */
    dsh->dsh_row[hbase->hsh_nodep->node_qen.qen_hjoin.hjn_prow] = (PTR) hbase->hsh_prptr;

    hbase->hsh_cartprod = NULL;
    hbase->hsh_firstbr = NULL;
    hbase->hsh_nextbr = NULL;
    hbase->hsh_min_browsz = hbase->hsh_browsz;
    hbase->hsh_min_prowsz = hbase->hsh_prowsz;
    hbase->hsh_jcount = 0;
    hbase->hsh_ccount = 0;
    hbase->hsh_fcount = 0;
    hbase->hsh_maxreclvl = 0;
    hbase->hsh_bvdcnt = 0;
    hbase->hsh_bwritek = 0;
    hbase->hsh_pwritek = 0;
    hbase->hsh_memparts = 0;
    hbase->hsh_rpcount = 0;
    hbase->hsh_currpart = -1;
    hbase->hsh_brcount = 0;
    hbase->hsh_prcount = 0;
    hbase->hsh_reverse = FALSE;
    hbase->hsh_flags &= HASH_KEEP_OVER_RESET;
				/* drop all runtime state flags */
    hbase->hsh_currows_left = 0;
    hbase->hsh_currbucket = 0;
    hbase->hsh_count1 = 0;
    hbase->hsh_count2 = 0;
    hbase->hsh_count3 = 0;
    hbase->hsh_count4 = 0;
    dmhcb = (DMH_CB *)hbase->hsh_dmhcb;	/* load DMH_CB for later */

    /* Loop to init LINK structures. */
    for (hlnk = hbase->hsh_toplink; hlnk; hlnk = hlnk->hsl_next)
    {
	hlnk->hsl_pno = -1;
	hlnk->hsl_flags = 0;

	/* And re-init the partition descriptors. */
	for (j = 0; j < hbase->hsh_pcount; j++)
	{
	    hpart = &hlnk->hsl_partition[j];

	    hpart->hsp_file = (QEN_HASH_FILE *) NULL;
	    hpart->hsp_bbytes = 0;
	    hpart->hsp_pbytes = 0;
	    hpart->hsp_bstarts_at = 0;
	    hpart->hsp_pstarts_at = 0;
	    hpart->hsp_buf_page = 0;
	    hpart->hsp_offset = 0;
	    hpart->hsp_brcount = 0;
	    hpart->hsp_prcount = 0;
	    hpart->hsp_flags  = (HSP_BALLSAME | HSP_PALLSAME);
	    if (clearbufptrs)
	    {
		hpart->hsp_bptr = NULL;
		hpart->hsp_bvptr = NULL;
	    }
	}
	/* No point in cleaning lower levels if releasing the memory
	** stream, they'll all just disappear
	*/
	if (clearbufptrs)
	    break;
    }
    /* The topmost one sticks around, clean it if releasing memory */
    if (clearbufptrs && hbase->hsh_toplink != NULL)
	hbase->hsh_toplink->hsl_next = NULL;

    /* Cleanup the files (cleans hbase file lists too). */
    if (dmhcb && dmhcb->dmh_locidxp)
    {
	status = qen_hash_closeall(hbase);
	if (clearbufptrs)
	    hbase->hsh_availfiles = NULL;
    }
    if (status == E_DB_OK)
    {
	/* Tidy up some stuff in the hash join's DMH_CB. */
	dmhcb->dmh_file_context = (PTR) NULL;
	dmhcb->dmh_buffer = (PTR) NULL;
	dmhcb->dmh_locidxp = (i2 *) NULL;
	dmhcb->dmh_rbn = 0;
	dmhcb->dmh_func = 0;
	dmhcb->dmh_flags_mask = 0;
	dmhcb->dmh_db_id = rcb->qef_db_id;
	dmhcb->dmh_tran_id = dsh->dsh_dmt_id;
    }
    return(status);

}

/* Name: qen_hjoin_cleanup - clean up completed hash join
**
** Description:
**
**	After a hash join is entirely completed, and won't be needed
**	any more, this routine is called to clean it up.  We'll reset
**	the hash join (which deletes all the temp files as well as
**	cleaning up data structures), and free the hash memory stream.
**
** Inputs:
**	qef_rcb			QEF request control block
**	dsh			dsh for this (possibly child) execution
**	hbase			Hash join control block
**
** Outputs:
**	None
**
** History:
**	27-May-2005, schka24
**	    Written.
*/

static void
qen_hjoin_cleanup(QEF_RCB *qef_rcb,
	QEE_DSH *dsh, QEN_HASH_BASE *hbase)

{
    DB_STATUS	status;
    QEF_CB	*qefcb;
    ULM_RCB	ulm;


    qefcb = dsh->dsh_qefcb;
    status = qen_hjoin_reset(qef_rcb, dsh, hbase, TRUE);
    /* Don't care if it fails */
    if (hbase->hsh_streamid != (PTR)NULL)
    {

	STRUCT_ASSIGN_MACRO(Qef_s_cb->qef_s_ulmcb, ulm);

	if (dsh->dsh_hash_debug)
	{
	    TRdisplay("%@ qen_hjoin_cleanup node: %d, hsh_size %u used %lu, memleft %ld sizepool %ld\n",
                hbase->hsh_nodep->qen_num, hbase->hsh_size, qefcb->qef_h_used,
		*ulm.ulm_memleft, ulm.ulm_sizepool);
	}

	ulm.ulm_streamid_p = &hbase->hsh_streamid;
	if (ulm_closestream(&ulm) == E_DB_OK)
	{
	    /* ULM has nullified hsh_streamid */
	    /* Adjust hash used for the session. */
	    CSadjust_counter(&qefcb->qef_h_used, -hbase->hsh_size);
	    hbase->hsh_size = 0;
	}
#ifdef xDEBUG
	TRdisplay("%@ qen_hjoin_cleanup: ulm_closestream %p, hsh_size %u used %lu, memleft %ld sizepool %ld\n",
                hbase, hbase->hsh_size, qefcb->qef_h_used, *ulm.ulm_memleft, ulm.ulm_sizepool);
#endif
    }
} /* qen_hjoin_cleanup */

/*
** Name: qen_hash_rll -- Attempt RLL compression of hash join row
**
** Description:
**	In order to minimize memory and disk usage, hash join may choose
**	to try to compress the non-key portions of the input (build and
**	probe) rows.  As long as we leave the key columns alone, this
**	can be done in a reasonably non-intrusive manner:  compress when
**	a row is entered into the partition buffers, uncompress when
**	a join is produced.  While there will probably be a small CPU
**	penalty on joins that are in-memory anyway, the potential win
**	for huge spilling joins is very large.
**
**	The compression method used is fairly primitive, because we
**	need to be quick, and because we're looking at a relatively small
**	amount of data.  Therefore block-oriented compressors (LZO and
**	friends) are out.  Huffman and arithmetic coding compressors are
**	too slow.  We'll use a simple run-length coding optimized for
**	short runs of zeros and somewhat longer runs of spaces.
**
**	The control byte meanings are (control bytes are hex):
**
**	00 .. f4: next 1..245 bytes following are to be taken literally.
**	f5: stop
**	f6 <len-3>: run of 3..258 UCS2 spaces (ie 0x0020)
**	f7 <len-3>: run of 3..258 ASCII spaces.
**	f8 <len-9>: run of 9..264 zero bytes.
**	f9 .. ff: run of 2 .. 8 zero bytes.
**
**	(The unicode-space compression is not currently implemented, until
**	we get OPC to help out by indicating that there are unicode columns
**	that are worth fiddling with.)
**
**	The "stop" code is needed because the length stored in the row
**	isn't exact, it's divided by 8 to save some bits.
**
**	Originally I was going to have a code for "run of a given byte",
**	but that sort of thing is relatively rare in real life data
**	seen to date.  Since it's harder to detect, I've eliminated it.
**
**	The caller supplies a work area of length uncompressed-row-length
**	to compress into.  If compression is unavailing, the row is left
**	in place and the compression flag is left off in the header.
**	(I.e. rll-compression will never make the row longer.)
**	If we succeed, the compressed row address is returned instead.
**
**	The caller should also ensure that compression is reasonable to
**	attempt, i.e. that the non-key part is long enough to expect
**	a savings of at least QEF_HASH_DUP_ALIGN bytes.
**
**	Note that the rest of hash-join is ignorant of the compression
**	method.  Also, the compressed data is entirely transient and is
**	never stored in persistent table data.  Thus, this compression
**	method can be replaced at any time with a better one.
**
**	Hash aggregation uses this same algorithm, although it doesn't need
**	to skip the hash keys.  Hash agg only uses compression to reduce
**	overflow size, never in memory, so we might as well compress it all.
**
** Inputs:
**	skip			Number of leading bytes left uncompressed
**				(for hash join this is the key)
**	rowptr			Uncompressed input row
**	where			Where to construct compressed result row
**
** Outputs:
**	Returns: new rowptr (either original, or "where") pointing to
**	possibly compressed row
**
** History:
**	24-Jun-2008 (kschendel) SIR 122512
**	    Written.
**	7-Jul-2008 (kschendel)
**	    Minor change to call sequence so that hash agg can use it.
*/

QEF_HASH_DUP *
qen_hash_rll(i4 skip, QEF_HASH_DUP *rowptr, PTR where)
{
    i4 rowsz, aligned_rowsz;
    i4 run_length;
    u_char ch;
    u_char ctl;
    u_char *nonrun_start;
    u_char *input_end;
    u_char *putter;
    u_char *stop;
    u_char *taker;
    u_char *taker_giveup;
    u_char *t;

    /* Start after header, keys.
    ** Note that we assume a to-be-compressed length of at least 2!
    ** (The caller isn't supposed to ask for an RLL attempt unless the
    ** area to compress is reasonably long.)
    */

    taker = (u_char *) rowptr + QEF_HASH_DUP_SIZE + skip;
    putter = (u_char *) where + QEF_HASH_DUP_SIZE + skip;
    rowsz = QEFH_GET_ROW_LENGTH_MACRO(rowptr);
    input_end = (u_char *) rowptr + rowsz;

    /* Since row length granularity is QEF_HASH_DUP_ALIGN (ie 8), we have to
    ** shrink the row by at least that much to win.  (The initial row size
    ** is guaranteed to be a multiple of the alignment, thanks to opc.)
    */
    stop = (u_char *)where + rowsz - QEF_HASH_DUP_ALIGN;

    do
    {
	/* taker+1 < input_end at this point.  I.e. there is the possibility
	** of a run.
	*/

	nonrun_start = taker;

	/* The output string has room for (stop-putter) bytes, and we
	** will need at least one control byte if no run is detected.
	** Give up if the taker reaches that length without detecting
	** a run.  Example:  output has room for 4 bytes before
	** declaring no-compression.  Input remaining is: a b 0 0;
	** The giveup-point is the third byte, since even though the
	** last two zeros form a run, 0x01 a b 0xf9 isn't any shorter
	** than the original a b 0 0.
	*/
	taker_giveup = taker + (stop - putter) - 1;
	if (taker_giveup >= input_end)
	    taker_giveup = input_end-1;
	run_length = 0;
	/* This loop scans for a run.  First we look for zero or blank,
	** because they're all we care about.  If we find one, look for
	** more.
	*/
	do
	{
	    ch = *taker;
	    if (ch != ' ' && ch != '\0')
	    {
		++taker;
	    }
	    else
	    {
		/* Try for a run */
		t = taker + 1;
		while (t < input_end && *t == ch)
		{
		    ++t;
		}
		run_length = t - taker;
		if (run_length >= 2 && (ch == '\0' || run_length >= 3))
		    break;
		/* Future note: try for unicode space here, since it
		** consists of 0 and ' ' -- we'll get this far regardless
		** of byte ordering.  Only try if unicode in the data.
		*/
		run_length = 0;
		taker = t;
	    }
	} while (taker < taker_giveup);
	/* Either we found a run, or reached the giveup point, or reached
	** the actual end of input.  If we're not giving up, there may or
	** may not be bytes to output literally (before any run).
	** If there's a run, taker points to the first character of the
	** run, and ch is the run character.
	*/
	if (taker > nonrun_start)
	{
	    i4 available, lit_length, i;
	    u_char *lit_end;

	    /* There's stuff to attempt to output literally.  If there is
	    ** no run found, we reached the end or the giveup point.
	    ** Either way, calculate whether there's room for the literal
	    ** string in the output before we spend time moving bytes.
	    */

	    if (run_length == 0)
	    {
		/* Try to fit all the rest */
		lit_end = input_end;
		available = stop - putter;
	    }
	    else
	    {
		lit_end = taker;
		/* will need at least one byte for the run */
		available = stop - putter - 1;
	    }
	    lit_length = lit_end - nonrun_start;
	    available -= lit_length;
	    while (available > 0 && lit_length > 0)
	    {
		/* Account for the control bytes needed, one every 254 */
		i = lit_length;
		if (i > 245)
		    i = 245;
		lit_length -= i;
		--available;
	    }
	    if (available <= 0)
		return (rowptr);	/* No compression available */
	    /* Move bytes from nonrun_start to lit_end */
	    taker = nonrun_start;
	    lit_length = lit_end - nonrun_start;
	    do
	    {
		i = lit_length;
		if (i > 245)
		    i = 245;
		*putter++ = (u_char) (i-1);
		MEcopy(taker, i, putter);
		taker += i;
		putter += i;
		lit_length -= i;
	    } while (lit_length > 0);
	} /* if literal to be output */
	if (run_length > 0)
	{
	    if (ch == ' ')
	    {
		/* Spaces are recorded as 0xf7:<length-3> with a max
		** length of 258 spaces per control-byte pair.
		** Minimum run length is 3.
		*/
		do
		{
		    if (putter+2 >= stop)
			return (rowptr);	/* No compression */
		    run_length = run_length - 3;
		    *putter++ = (u_char) 0xf7;
		    ctl = (run_length > 255) ? 255 : (u_char) run_length;
		    *putter++ = ctl;
		    run_length = run_length - ctl;
		    taker = taker + ctl + 3;
		} while (run_length >= 3);
	    }
	    /* else if unicode -- not implemented */
	    else
	    {
		/* Must be zero.  Zero runs are recorded as:
		** 0xf9 thru 0xff for runs of 2 to 8 zero bytes;
		** 0xf8:<length-9> for runs of 9 to 264 zeros.
		** Minimum run length is 2.
		*/
		do
		{
		    if (run_length <= 8)
		    {
			if (putter+1 >= stop)
			    return (rowptr);
			*putter++ = (u_char) (0xf9 + run_length - 2);
			taker = taker + run_length;
			run_length = 0;
			break;
		    }
		    if (putter+2 >= stop)
			return (rowptr);
		    run_length = run_length - 9;
		    *putter++ = (u_char) 0xf8;
		    ctl = (run_length > 255) ? 255 : (u_char) run_length;
		    *putter++ = ctl;
		    run_length = run_length - ctl;
		    taker = taker + ctl + 9;
		} while (run_length >= 2);
	    }
	}
	/* If there's leftover that could not be encoded,
	** "taker" simply isn't advanced past that leftover.
	** Include it as part of the next literal string.
	*/
    } while (taker+1 < input_end);
 
    /* One last thing to check, if there's a lone byte at the very end
    ** we have to construct a literal sequence to include it.
    */
    if (taker+1 == input_end)
    {
	if (putter+2 >= stop)
	    return (rowptr);
	*putter++ = 0;		/* ie one byte */
	*putter++ = *taker++;
    }

    /* Assert: taker should be at input-end exactly. */
    if (taker != input_end)
    {
	TRdisplay("%@ hashjoin row rll error, taker long by %d\n",
		taker - input_end);
	return (rowptr);
    }

    /* If we make it out of the loop, the row apparently got compressed.
    ** Copy the header and keys (which we avoided so far in case compression
    ** doesn't happen), mark the row compressed, update the row length,
    ** and return the compressed row.
    **
    ** putter points at the end of the compressed row, + 1.
    */
    if (putter >= stop)
	return (rowptr);	/* Double check that we're OK */

    MEcopy((PTR) rowptr, QEF_HASH_DUP_SIZE + skip, where);
    rowptr = (QEF_HASH_DUP *) where;
    rowptr->hash_compressed = 1;
    rowsz = putter - (u_char *) where;
    aligned_rowsz = DB_ALIGNTO_MACRO(rowsz, QEF_HASH_DUP_ALIGN);
    /* if we didn't happen to stop at a multiple of QEF_HASH_DUP_ALIGN
    ** exactly, output a stop code so that uncompression doesn't go
    ** too far.  This is a consequence of trying to save some bits in
    ** the header by storing row length / 8 instead of row length.
    */
    if (aligned_rowsz > rowsz)
	*putter = 0xf5;
    QEFH_SET_ROW_LENGTH_MACRO(rowptr, aligned_rowsz);
    return (rowptr);

} /* qen_hash_rll */

/*
** Name: qen_hash_unrll -- Un-compress a compressed hashjoin row
**
** Description:
**	If a hash-join row header shows that it's compressed, this
**	routine is called to decompress the non-key portion.  The
**	caller must verify that the row is compressed, and must supply
**	someplace to decompress the row into.  Typically this will be
**	the normal build-input or probe-input DSH row area;  since
**	compressed rows are always coming out of hash join partition
**	buffers or similar structures, those DSH rows are safe to use.
**
**	The compressed row consists of control bytes and data.
**	The control byte meanings are (control bytes are hex):
**
**	00 .. f4: next 1..245 bytes following are to be taken literally.
**	f5: stop
**	f6 <len-3>: run of 3..258 occurrences of UCS2 space (0x0020)
**	f7 <len-3>: run of 3..258 ASCII spaces.
**	f8 <len-9>: run of 9..264 zero bytes.
**	f9 .. ff: run of 2 .. 8 zero bytes.
**
** Inputs:
**	skip			Number of leading bytes not compressed
**				(for hash join this is the key)
**	rowptr			Compressed input row
**	where			Where to construct uncompressed result row
**
** Outputs:
**	Returns: new rowptr pointing to uncompressed row in "where"
**
** History:
**	24-Jun-2008 (kschendel) SIR 122512
**	    Written.
**	7-Jul-2008 (kschendel)
**	    Minor change to call sequence so that hash agg can use it.
*/

QEF_HASH_DUP *
qen_hash_unrll(i4 skip, QEF_HASH_DUP *rowptr, PTR where)
{
    i4 i;
    i4 rowsz;
    u_char control;
    u_char *input_end;
    u_char *putter;
    u_char *taker;

    rowsz = QEFH_GET_ROW_LENGTH_MACRO(rowptr);
    MEcopy((PTR) rowptr, QEF_HASH_DUP_SIZE + skip, where);
    putter = (u_char *) where + QEF_HASH_DUP_SIZE + skip;
    taker = (u_char *) rowptr + QEF_HASH_DUP_SIZE + skip;
    input_end = (u_char *) rowptr + rowsz;

    /* An interesting question here is whether to move data by hand,
    ** or use MEcopy/MEfill.  I suspect that by-hand is the right answer,
    ** but it would be interesting to experiment.
    */
    do
    {
	control = *taker++;
	if (control <= 0xf4)
	{
	    /* <control+1> bytes taken literally */
	    i = control + 1;
	    do
		*putter++ = *taker++;
	    while (--i > 0);
	}
	else if (control >= 0xf9)
	{
	    /* 2..8 zeros */
	    i = control - 0xf9 + 2;
	    do
	    {
		*putter++ = 0;
	    } while (--i > 0);
	}
	else if (control == 0xf8)
	{
	    /* <next byte>+9 zeros */
	    i = *taker++ + 9;
	    do
	    {
		*putter++ = 0;
	    } while (--i > 0);
	}
	else if (control == 0xf7)
	{
	    /* <next byte>+3 spaces */
	    i = *taker++ + 3;
	    do
	    {
		*putter++ = ' ';
	    } while (--i > 0);
	}
	else if (control == 0xf6)
	{
	    u_i2 value = U_BLANK;

	    /* <next byte>+3 of 0x0020.  Use I2ASSIGN_MACRO to avoid
	    ** byte-ordering issues.
	    */
	    i = *taker++ + 3;
	    do
	    {
		I2ASSIGN_MACRO(value, *(u_i2 *)putter);
		putter += sizeof(u_i2);
	    } while (--i > 0);
	}
	else /* if (control == 0xf5) */
	{
	    /* Stop code */
	    break;
	}
    } while (taker < input_end);

    /* Fix up the new header and return the uncompressed row */
    rowsz = putter - (u_char *) where;
    rowptr = (QEF_HASH_DUP *) where;
    rowptr->hash_compressed = 0;
    /* Note, shouldn't need alignment on rowsz here since it had better
    ** be one of the original aligned build or probe row lengths.
    */
    QEFH_SET_ROW_LENGTH_MACRO(rowptr, rowsz);
    return (rowptr);

} /* qen_hash_unrll */
