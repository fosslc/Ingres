/* 
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <me.h>
#include    <st.h>
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
**  Name: QENISJOIN.C - merge of inner stream sorted, outer not
**
**  Description:
**
**
**  History:
**      01-dec-89 (davebf)
**          written  (adapted from qenjoin.c 6.3)
**	11-mar-92 (jhahn)
**	    Moved calls to CSswitch to top of qen_join. (see 16-jul-91)
**	    CSswitch is now called at the top of every qen node.
**	29-may-92 (rickh)
**	    Rewrote to fix right joins.
**	12-aug-92 (rickh)
**	    Converted tid hold file to a temporary table to facilitate
**	    sorting.
**	08-dec-92 (anitap)
**	    Included <qsf.h> and <psfparse.h> for CREATE SCHEMA.
**	17-may-93 (rickh)
**	    Rewind inner hold file to beginning if get outer key that's
**	    less than previous outer.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      14-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
**	19-oct-93 (rickh)
**	    Removed unget logic. Since every tuple from the inner stream
**	    is forced into the hold file immediately after it's read, there's
**	    no need for an unget buffer.
**	30-dec-93 (rickh)
**	    Upon reset, set node status to QEN0_INITIAL.  This is done in 6.4.
**	    Re-initializing on reset makes subselects work.
**      01-nov-95 (ramra01)
**          Prior to calling qen_u21_execute_cx confirm the presence of
**          a adf control block to execute
**	7-nov-95 (inkdo01)
**	    Changes to replace QEN_ADF and QEN_OJINFO structure instances by
**	    pointers in QEN_SJOIN.
**	24-feb-97 (pchang)
**	    Fixed a flaw in the reset logic that prevented the dumping of the
**	    hold file and the subsequent allocation of new sort vector arrays
**	    when the node was to be reset.  The error had resulted in SIGBUS
**	    caused by memory overrun in the sort vector array.  (B79874)
**	12-jun-97 (hayke02)
**	    Fixed erroneous logic when dealing with NULLs in the inner and
**	    outer streams. If a NULL is encountered in the outer stream we
**	    now get a new outer, rather than a new inner. The converse
**	    applies for a NULL in the inner stream. The logic for a NULL in
**	    both the inner and outer streams was correct. This change fixes
**	    bug 82800.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	23-feb-04 (inkdo01)
**	    Changed qef_rcb->error to dsh->dsh_error for || thread safety.
**	28-Dec-2005 (kschendel)
**	    Use DSH references whenever possible instead of QEF RCB
**	    references.  (This simplifies the code, eliminating many
**	    RCB references, and reflects the new primacy of the DSH
**	    thanks to the parallel query changes.)
**	11-Nov-2009 (kiria01) SIR 121883
**	    Scalar sub-query - Pick up on cardinality errors if requested.
**/

/*
** Here's the algorithm for joining tuple streams when the driving stream
** is mostly sorted on the join key and the re-scannable stream is completely
** sorted:
**
**	This algorithm assumes the following sort order for a multi-attribute
**	key:
**
**	a,a
**	a,b
**	...
**	a,
**	b,a
**	b,b
**	...
**	b,
**	...
**	z,a
**	z,b
**	...
**	z,z
**	z,
**	 ,a
**	 ,b
**	...
**	 ,z
**	 ,
**	EOF
**
**	That is, within an attribute, real values come first, then nulls.
**
**
**INNER_STREAM_EXHAUSTED means inner stream at EOF and hold file not positioned
**
**begin isjoin
**
**    readOuter = TRUE
**    readInner = TRUE
**
**    outerHasJoined = TRUE
**    lookForRightJoins = FALSE
**
**    begin for
**
**	// read outer //
**
**	if readOuter == TRUE
**
**	    if outerHasJoined == FALSE
**	        outerHasJoined = TRUE
**		execute LNULL
**		if jqual passes
**		    emit left join
**		endif
**	    endif
**
**	    outerHasJoined = FALSE
**
**	    readOuter = FALSE
**	    read an outer tuple if not already at EOF
**	    if outer at EOF
**		outerHasJoined = TRUE
**
**		if rjoin
**		    lookForRightJoins = TRUE
**		    reset innerHoldFile to beginning of file
**		    reset tidHoldFile to beginning of file
**		    readInner = TRUE
**		    continue
**		else
**		    done
**		endif
**	    endif
**
**	    if not first outer read
**
**	        if new key is same as old and rescanTIDsaved == TRUE
**		    reposition innerHoldFile to rescanStartPoint
**		    readInner = TRUE
**		    continue;
**		else 
**		    rescanTIDsaved = FALSE
**		endif new same as old
**
**		if new key is less than old
**		    reposition innerHoldFile to beginning
**		    readInner = TRUE
**		    continue
**		endif new key is less than old
**
**	    endif not first outer read
**	endif readOuter==TRUE
**
**	// read inner //
**
**	if readInner is TRUE
**
**	    readInner = FALSE
**
**	    if not INNER_STREAM_EXHAUSTED
**
**	        if hold file positioned
**		    read from hold file
**		    if hold file scanned to end
**		        readInner = TRUE
**		        continue
**		    endif hold file scanned to end
**
**	        else hold file not positioned
**
**		    read from inner stream (this may involve refetching ungotten
**		    tuple)
**		    if not at EOF and lookForRightJoins == FALSE
**			append inner to hold file if from inner stream
**		    endif
**
**	        endif hold file positioned or not
**
**	    endif inner stream is not exhausted
**	endif readInner is TRUE
**
**
**	// emit right joins //
**
**	if lookForRightJoins == TRUE
**	    if INNER_STREAM_EXHAUSTED
**		done
**	    endif
**
**	    readOuter = FALSE
**	    readInner = TRUE
**	    if this tid is not in the tidHoldFile
**		execute RNULL
**		if jqual succeeds
**		    emit right join
**		endif
**	    endif
**	    continue
**	endif
**
**	// set joinResult //
**
**	if INNER_STREAM_EXHAUSTED or the inner tuple buffer is empty
**	    joinResult = outerLessThanInner
**	else if outer at EOF
**	    joinResult = outerGreaterThanInner
**	else
**	    joinResult = key comparison of outer and inner
**
**	    if both outer and inner keys are null
**	        joinResult = outerGreaterThanInner
**	    else if outer key is null
**	        joinResult = outerGreaterThanInner
**	    else if inner key is null
**	        joinResult = outerLessThanInner
**	    endif
**	endif setting of joinResult
**
**	// emit joins based on joinResult //
**
**	if joinResult == outerLessThanInner
**	    readOuter = TRUE
**	    readInner = FALSE
**	    continue
**	endif outer key less than inner key
**
**	if joinResult == outerGreaterThanInner
**	    readOuter = FALSE
**	    readInner = TRUE
**	    continue
**	endif outer greater than inner
**
**	// else inner key = outer key //
**
**	if this is the first inner to match
**	    save tid as rescanStart
**	    rescanTIDsaved = TRUE
**	endif
**
**	readInner = TRUE
**	readOuter = FALSE
**
**	execute OQUAL
**	if oqual succeeds
**	    outerHasJoined = TRUE
**	    add its tid to tidHoldFile
**	    execute IJMAT
**	    execute jqual
**	    if jqual succeeds
**		emit inner join
**	    endif
**	endif
**
**    end for
**end isjoin
**
**	From time to time you will wonder "What makes the right join logic
**	work?"  It's quite simple: after being snarfed up from the inner
**	relation, the inner tuples are ALWAYS buffered in a hold file when
**	we're doing a right join.  The TIDs we insert into the TIDholdfile
**	are from this buffering file and are therefore GUARANTEED to be
**	in ascending order (since the buffer file is a heap).   This is
**	what makes it possible to simply sort the TIDholdfile, remove
**	duplicates, and then just read the inner stream side-by-side with
**	the TIDholdfile to look for right joins.
*/

/*	static functions	*/

static DB_STATUS
repositionForRightJoins(
QEN_NODE	*node,
QEF_RCB		*qef_rcb,
QEE_DSH		*dsh,
bool		*in_reset );

static DB_STATUS
rewindInnerStream(
QEN_NODE	*node,
QEF_RCB		*qef_rcb,
QEE_DSH		*dsh,
bool		*in_reset );

static DB_STATUS
backupInnerStream(
QEN_NODE	*node,
QEE_DSH		*dsh,
bool		*in_reset );

/*{
** Name: QEN_ISJOIN	 - interpreter for inner stream sorted, not outer
**
** Description:
**
**  redo this
**
**
**
**
** Inputs:
**      node                            the join node data structure
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
**	    All values in the appropriate row buffers have
**	    been set to that which will produce the joined row.
**
**
** History:
**      01-dec-89 (davebf)
**          written   (adapted from qen_join  6.3)
**	    To help QEF determine what to do when a null in encountered in
**	    a multi-attribute join, ADF will return 3 more values in
**	    xcxcb.excb_cmp, ADE_1ISNULL, ADE_2ISNULL, and ADE_BOTHNULL.
**          QEF logic has been adjusted to handle these 3 new values.
**	11-mar-92 (jhahn)
**	    Added support for QEF interrupts.
**	    Added call to CSswitch.
**	25-mar-92 (rickh)
**	    Upgraded tid hold file to use the same logic as the hold file
**	    for whole tuples returned from the inner child.  For free,
**	    we get dumping to disk when necessary.
**	29-may-92 (rickh)
**	    Rewrote to fix right joins.
**	12-aug-92 (rickh)
**	    Converted tid hold file to a temporary table to facilitate
**	    sorting.
**	12-oct-92 (rickh)
**	    State transition from INITIAL to EXECUTED wasn't occurring
**	    for right joins when the driving stream was empty and the
**	    re-scannable stream wasn't a sort file.
**	19-oct-93 (rickh)
**	    Removed unget logic. Since every tuple from the inner stream
**	    is forced into the hold file immediately after it's read, there's
**	    no need for an unget buffer.
**	30-dec-93 (rickh)
**	    Upon reset, set node status to QEN0_INITIAL.  This is done in 6.4.
**	    Re-initializing on reset makes subselects work.  On reset,
**	    also reset node_u.node_join.node_outer_status.
**      01-nov-95 (ramra01)
**          Prior to calling qen_u21_execute_cx confirm the presence of
**          a adf control block to execute
**	7-nov-95 (inkdo01)
**	    Changes to replace QEN_ADF and QEN_OJINFO structure instances by
**	    pointers in QEN_SJOIN.
**	24-feb-97 (pchang)
**	    Fixed a flaw in the reset logic that prevented the dumping of the
**	    hold file and the subsequent allocation of new sort vector arrays
**	    when the node was to be reset.  The error had resulted in SIGBUS
**	    caused by memory overrun in the sort vector array.  (B79874)
**	12-jun-97 (hayke02)
**	    Fixed erroneous logic when dealing with NULLs in the inner and
**	    outer streams. If a NULL is encountered in the outer stream
**	    (join_result == ADE_1ISNULL) we now get a new outer, rather than
**	    a new inner. The converse applies for a NULL in the inner stream
**	    (join_result == ADE_2ISNULL). The logic for a NULL in both the
**	    inner and outer streams was correct. This change fixes bug 82800.
**	2-feb-00 (inkdo01)
**	    Increment row count (for qe90).
**	2-jan-03 (inkdo01)
**	    Removed noadf_mask logic in favour of QEN_ADF * tests to fix
**	    concurrency bug.
**	21-mar-03 (wanfr01)
**	    Bug 109168, INGSRV2010
**	    Added code to rescan inner stream if a NULL is encountered.
**	20-nov-03 (inkdo01)
**	    Add support for "open" call to allow parallel query initialization.
**	29-dec-03 (inkdo01)
**	    DSH is now parameter, "function" replaces "reset".
**	2-jan-04 (inkdo01)
**	    Added end-of-partition-grouping logic.
**	27-feb-04 (inkdo01)
**	    Changed dsh ptr to arrays of structs (SHD, HOLD, STATUS) to
**	    ptrs to arrays of ptrs.
**	17-mar-04 (toumi01)
**	    Add support for "close" call to allow parallel query termination.
**	8-apr-04 (inkdo01)
**	    Added dsh parm to QEF_CHECK_FOR_INTERRUPT macro.
**	19-apr-04 (inkdo01)
**	    Minor fix to partition group handling.
**	04-Nov-2004 (jenjo02)
**	    Only CSswitch on root DSH or if not OS-threaded.
**	12-Jan-2006 (kschendel)
**	    Update calls to execute CX's.
**	16-Feb-2006 (kschendel)
**	    Return after close, don't keep going.
**	13-Feb-2007 (kschendel)
**	    Replace CSswitch with better cancel check.
**	4-Jun-2009 (kschendel) b122118
**	    Use slightly faster/more convenient dsh test for qe90, and
**	    remove pointless error returns from cost begin/end.  Minor
**	    streamlining.
**	11-Nov-2009 (kiria01) SIR 121883
**	    Scalar sub-query - Pick up on cardinality errors if requested.
**	    It may be that in practice the selection count cannot be tripped
**	    in a PSM as tests so far suggest that we switch to FSM when an
**	    outer join is encountered on a join that would otherwise be PSM.
*/


	/*
	** When there's no hold file for inner tuples, we know we're done
	** scanning and rescanning the inner stream if the inner node
	** has reached EOF and doesn't need to be reset.
	**
	** When there is an inner hold file, we know we're done rescanning
	** if the above conditions obtain plus the hold file hasn't been
	** positioned for rescanning.
	**
	** So this is what it means to say the inner stream is exhausted:
	*/

#define	INNER_STREAM_EXHAUSTED ( \
	(  qen_status->node_u.node_join.node_inner_status == QEN11_INNER_ENDS && \
	   in_reset == FALSE && \
	   ( !qen_hold || \
	      ( qen_hold && \
	        qen_hold->hold_status != HFILE3_POSITIONED \
	      ) \
	   ) \
	) )


DB_STATUS
qen_isjoin(
QEN_NODE	*node,
QEF_RCB		*qef_rcb,
QEE_DSH		*dsh,
i4		function )
{
    QEF_CB	    *qef_cb = dsh->dsh_qefcb;
    DMR_CB	    *dmrcb;
    QEN_NODE	    *out_node = node->node_qen.qen_sjoin.sjn_out;
    QEN_NODE	    *in_node = node->node_qen.qen_sjoin.sjn_inner;
    QEE_XADDRS	    *node_xaddrs = dsh->dsh_xaddrs[node->qen_num];
    QEN_STATUS	    *qen_status = node_xaddrs->qex_status;
    ADE_EXCB	    *ade_excb;
    ADE_EXCB	    *jqual_excb;	/* Post-join restriction */
    QEN_HOLD	    *qen_hold = (QEN_HOLD *)NULL;
    QEN_SHD         *qen_shd = (QEN_SHD *)NULL;
    QEN_TEMP_TABLE  *tidTempTable = (QEN_TEMP_TABLE *)NULL;
    DB_STATUS	    status = E_DB_OK;
    bool	    reset = FALSE;
    bool	    out_reset = FALSE;
    bool	    in_reset = FALSE;
    bool            ojoin = (node->node_qen.qen_sjoin.sjn_oj != NULL);
    bool            ljoin = FALSE;
    bool            rjoin = FALSE;
    i4		    new_to_old;
    i4		    join_result;
    i4	    val1;
    i4	    val2;
    u_i4	    tid;
    TIMERSTAT	    timerstat;
    bool potential_card_violation = FALSE;

#ifdef xDEBUG
    (VOID) qe2_chk_qp(dsh);
#endif

    if (function != 0)
    {
	if (function & FUNC_RESET)
	{
	    reset = in_reset = out_reset = TRUE;
	}

	/* Do open processing, if required. Only if this is the root node
	** of the query tree do we continue executing the function. */
	if ((function & TOP_OPEN || function & MID_OPEN) &&
	    !(qen_status->node_status_flags & QEN1_NODE_OPEN))
	{
	    status = (*out_node->qen_func)(out_node, qef_rcb, dsh, MID_OPEN);
	    status = (*in_node->qen_func)(in_node, qef_rcb, dsh, MID_OPEN);
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
		status = (*out_node->qen_func)(out_node, qef_rcb, dsh, FUNC_CLOSE);
		status = (*in_node->qen_func)(in_node, qef_rcb, dsh, FUNC_CLOSE);
		qen_status->node_status_flags = 
		    (qen_status->node_status_flags & ~QEN1_NODE_OPEN) | QEN8_NODE_CLOSED;
	    }
	    return(E_DB_OK);
	}
    }

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

    if(node->node_qen.qen_sjoin.sjn_hfile >= 0)
    {
	qen_hold = dsh->dsh_hold[node->node_qen.qen_sjoin.sjn_hfile];
	qen_shd = dsh->dsh_shd[dsh->dsh_qp_ptr->qp_sort_cnt +
	                        node->node_qen.qen_sjoin.sjn_hfile];
    }

    /* if there's a tid temporary table */

    if( ojoin && node->node_qen.qen_sjoin.sjn_oj->
		 oj_innerJoinedTIDs )
    {
	tidTempTable = node->node_qen.qen_sjoin.sjn_oj->oj_innerJoinedTIDs;
    }

    jqual_excb = node_xaddrs->qex_jqual;

    if ( ojoin ) switch(node->node_qen.qen_sjoin.sjn_oj->oj_jntype)
    {
       case DB_LEFT_JOIN:
         ljoin = TRUE;
         break;
       case DB_RIGHT_JOIN:
         rjoin = TRUE;
         break;
       case DB_FULL_JOIN:
         ljoin = TRUE;
         rjoin = TRUE;
         break;
       default: break;
    }

    /* If the node is to be reset, dump the hold files and reset the
    ** inner/outer nodes */

loop_reset:
    if (reset)
    {
	if ( qen_status->node_status != QEN0_INITIAL )
	{
	    if ( qen_hold && ( in_node->qen_type != QE_SORT ) )
	    {
		/* reset in memory or dump dmf tuple hold file if it has
		** been created */
		status = qen_u9_dump_hold(qen_hold, dsh, qen_shd);
		if(status) goto errexit;
		qen_hold->hold_buffer_status = HFILE6_BUF_EMPTY;
	    }

	    if ( tidTempTable )
	    {
		/* dump tid temp table if it was created */
		status = qen_dumpTidTempTable( tidTempTable, node, dsh);
		if(status) goto errexit;  
	    }
	}

	qen_status->node_status = QEN0_INITIAL; /* reset = reintialize */
	qen_status->node_u.node_join.node_inner_status = QEN0_INITIAL;
	qen_status->node_u.node_join.node_outer_status = QEN0_INITIAL;

	qen_status->node_access = (	QEN_READ_NEXT_OUTER |
					QEN_READ_NEXT_INNER |
					QEN_OUTER_HAS_JOINED	);  
    } /* end if reset */

    if (qen_status->node_status == QEN0_INITIAL)  
    {
	if(qen_hold)
	{
	    qen_hold->hold_status = HFILE0_NOFILE;  /* default */
	    qen_hold->hold_medium = HMED_IN_MEMORY;  /* default */
	    /* in case we build a hold file
	    ** tell qen_u1_append to calculate its size in memory
	    ** or go to DMF hold */
	    qen_shd->shd_tup_cnt = -1;
	}

	if( tidTempTable )
	{
	    status = qen_initTidTempTable( tidTempTable, node, dsh);
	    if(status) goto errexit;
	}

	qen_status->node_access = 0;  

	if(rjoin)
	{
	    /* consistency check */
	    if( (!qen_hold) || (!tidTempTable) )
	    {
		/* rjoin and no hold files */
		dsh->dsh_error.err_code = E_QE0002_INTERNAL_ERROR;
		status = E_DB_ERROR;
		goto errexit;
	    }

	    /* force DMF hold, if we build one */
	    /* this is because we can't have a mixture of DMF tids and
	    ** memory tids on tidfile */
	    qen_hold->hold_medium = HMED_ON_DMF;

	    /* create and open hold file */
	    status = qen_u101_open_dmf(qen_hold, dsh);
	    if(status) goto errexit;  

	}

	qen_status->node_access = (	QEN_READ_NEXT_OUTER |
					QEN_READ_NEXT_INNER |
					QEN_OUTER_HAS_JOINED	);  
    }

    for (;;)   /* The loop */
    {
	status = E_DB_OK;

	/*********************************************************
	**	
	**	LOGIC TO READ FROM THE OUTER TUPLE STREAM
	**
	**
	**
	*********************************************************/

	if( qen_status->node_access & QEN_READ_NEXT_OUTER )
	{

	    /*
	    ** If the previous outer tuple did not inner join with
	    ** any inner tuples, then it may be an outer join.  Return
	    ** it along with nulls for the right side if it passes
	    ** the WHERE clause.
	    */

	    if ( ljoin && !( qen_status->node_access & QEN_OUTER_HAS_JOINED ) )
	    {
		/*
		** Set the "outer has joined" flag so that if we emit
		** a left join, we won't come back into this conditional
		** the next time through this fmsjoin node.
		*/
		qen_status->node_access |= QEN_OUTER_HAS_JOINED;

		/* now execute oj_lnull */
		status = qen_execute_cx(dsh, node_xaddrs->qex_lnull);
		if (status != E_DB_OK)
		    goto errexit;   /* if ade error, return error */

		/* Apply post-join restriction if any */
		if (jqual_excb == NULL)
		    break;		/* Emit the left join */
		else
		{
		    status = qen_execute_cx(dsh, jqual_excb);
		    if (status != E_DB_OK)
			goto errexit;   /* if ade error, return error */
		    if (jqual_excb->excb_value == ADE_TRUE)
			break;	/* emit a left join */
		}
	    }	/* endif previous outer did not join */

	    qen_status->node_access &= ~(	QEN_READ_NEXT_OUTER |
						QEN_OUTER_HAS_JOINED	);

	    /* get a new outer */

	newouter:
	    if ( qen_status->node_u.node_join.node_outer_status == QEN8_OUTER_EOF )
	    {
		/*
		** Consistency check.  Should never be asked to
		** get more outer tuples when we're already at EOF.
		*/
		dsh->dsh_error.err_code = E_QE0002_INTERNAL_ERROR;
		status = E_DB_ERROR;
		goto errexit;
	    }
	    else	/* this is where we actually read the outer stream! */
	    {
	        status = (*out_node->qen_func)(out_node, qef_rcb, dsh,
				(out_reset) ? FUNC_RESET : NO_FUNC);
	    }
	    out_reset = FALSE;

	    /* a little error handling.  check for end of outer stream */
	    if (status != E_DB_OK)
	    {
		if (dsh->dsh_error.err_code == E_QE0015_NO_MORE_ROWS ||
		    dsh->dsh_error.err_code == E_QE00A5_END_OF_PARTITION &&
		    node->qen_flags & QEN_PART_SEPARATE)
                {

		    qen_status->node_access |= QEN_OUTER_HAS_JOINED;
                    qen_status->node_u.node_join.node_outer_status = QEN8_OUTER_EOF;

		    if (dsh->dsh_error.err_code == E_QE00A5_END_OF_PARTITION)
			qen_status->node_status_flags |= QEN2_OPART_END;
					/* if just EOPartition, flag it */

		    /*
		    ** If we must look for right joins, rescan the inner.
		    ** Otherwise, we're done.
		    */
		    if ( rjoin )
		    {
			status = repositionForRightJoins( node, qef_rcb,
			     dsh, &in_reset );
		        if(status != E_DB_OK) break;   /* to error */
		        qen_status->node_access |=
			 ( QEN_READ_NEXT_INNER | QEN_LOOKING_FOR_RIGHT_JOINS );

			if(qen_status->node_status == QEN0_INITIAL)
			{
			    qen_status->node_status = QEN1_EXECUTED;
			}

		        continue; /* to get a new inner */
		    }
		    else
                    {
			/*
			** This is one of the two ways that we can exit this node
			** for good.
			*/

		        qen_status->node_status = QEN4_NO_MORE_ROWS;
                        break;    /* done */
                    }    
		}
		else if (dsh->dsh_error.err_code == E_QE00A5_END_OF_PARTITION)
		{
		    /* No more rows in partitioning group - read from 
		    ** next group. */
		    out_reset = TRUE;
		    goto newouter;
		}
		else	/* non EOF error */
		{
		    break;  /* return error */
		}
	    }

            if(qen_status->node_status == QEN0_INITIAL)
            {
		qen_status->node_status = QEN1_EXECUTED;  /* init done */

		if(in_node->qen_type  == QE_SORT)
		{

		    /* consistency check */
		    if(!qen_hold)
		    {
			/* inner sort and no hold file */
			dsh->dsh_error.err_code = E_QE0002_INTERNAL_ERROR;
			status = E_DB_ERROR;
			goto errexit;
		    }

		    status = qen_u32_dosort(in_node, 
					    qef_rcb, 
					    dsh, 
					    qen_status, 
					    qen_hold,
					    qen_shd, 
					    (in_reset) ? FUNC_RESET : NO_FUNC);

		    if(status) goto errexit;
		    in_reset = FALSE;
		}

		/* now materialize the first join key */
		status = qen_execute_cx(dsh, node_xaddrs->qex_okmat);
		if (status != E_DB_OK)
		    goto errexit;   /* if ade error, return error */
	    }

	    /* If not the first time */
	    else
	    {
		/* compare the old outer key to the new one. */
		new_to_old = ADE_1EQ2;
		ade_excb = node_xaddrs->qex_kcompare;
		if (ade_excb != NULL)
		{
		    status = qen_execute_cx(dsh, ade_excb);
		    if (status != E_DB_OK)
			goto errexit;   /* if ade error, return error */
		    new_to_old = ade_excb->excb_cmp;
		}

		/* Materialize the new outer key if the old and the 
		** new outer keys are not equal */
		if (new_to_old != NEW_EQ_OLD)
		{
		    status = qen_execute_cx(dsh, node_xaddrs->qex_okmat);
		    if (status != E_DB_OK)
			goto errexit;   /* if ade error, return error */
		}
		else if ((node->qen_flags & QEN_CARD_MASK) == QEN_CARD_01L)
		{
		    /* Right outer - note cardinality */
		    potential_card_violation = (new_to_old == ADE_1EQ2);
		}

		/*
		** If the outer key hasn't changed and it matched some
		** inners, we must rescan them to look for joins.
		** Otherwise, dump any inner tuples the old key matched.
		*/

		if ( ( new_to_old == NEW_EQ_OLD ) &&
		     ( qen_status->node_access & QEN_RESCAN_MARKED ) )
		{
		    status = backupInnerStream( node, dsh, &in_reset );
		    if(status != E_DB_OK) break;   /* to error */
		    qen_status->node_access |= QEN_READ_NEXT_INNER;
		    continue;
		}
		else
		{
		    qen_status->node_access &= ~QEN_RESCAN_MARKED;
		}

		/*
		** If new outer key is less than old one, we must back
		** up inner stream to the beginning.  Also added code for NULL
		** handling.
		*/

	        if (( new_to_old == NEW_LT_OLD ) || ( new_to_old == ADE_1ISNULL) || 
		    ( new_to_old == ADE_2ISNULL))
		{
		    status = rewindInnerStream( node, qef_rcb, dsh, &in_reset );
		    if(status != E_DB_OK) break;   /* to error */
		    qen_status->node_access |= QEN_READ_NEXT_INNER;
		    continue;
		}


	    }	/* end first or subsequent times */
	}  /* end if read_outer  */

	/*********************************************************
	**	
	**	LOGIC TO READ FROM THE INNER TUPLE STREAM
	**
	**
	*********************************************************/

	if( qen_status->node_access & QEN_READ_NEXT_INNER )
	{
            qen_status->node_access &= ~QEN_READ_NEXT_INNER;

	    if ( !INNER_STREAM_EXHAUSTED )
	    {
	        /* Read from hold/sort file if it exists and is positioned */
	        if(qen_hold &&
	           qen_hold->hold_status == HFILE3_POSITIONED)
	        {
		    if ( (status = qen_u4_read_positioned(qen_hold, dsh,
	    			           qen_shd)) != E_DB_OK)
		    {
		        if (dsh->dsh_error.err_code == E_QE0015_NO_MORE_ROWS)
		        {
			    /* Hold file ends, must read from inner node */
			    qen_hold->hold_buffer_status = HFILE6_BUF_EMPTY;
			    qen_hold->hold_status = HFILE2_AT_EOF;
			    dsh->dsh_error.err_code = 0;
			    qen_status->node_access |= QEN_READ_NEXT_INNER;
			    continue;	/* to read a new inner */
		        }
		        else	/* other, presumably fatal error */
		        {
			    break;
		        }
		    }   /* end if hold end */

		    if(in_node->qen_type == QE_SORT)  /* if hold from sort */
		    {
		        /* Materialize the inner tuple from sort's row buffer
		           into my row buffer. */
			status = qen_execute_cx(dsh, node_xaddrs->qex_itmat);
			if (status != E_DB_OK)
			    goto errexit;   /* if ade error, return error */
		    } /* end if hold from sort */

		    qen_hold->hold_buffer_status = HFILE7_FROM_HOLD;
	        }	/* end if positioned */
	        /* if not EOF on stream */
	        else if (qen_status->node_u.node_join.node_inner_status != QEN11_INNER_ENDS)
	        {
	newinner:
		    status = (*in_node->qen_func)(in_node, qef_rcb, dsh,
				(in_reset) ? FUNC_RESET : NO_FUNC);
		    in_reset = FALSE;
		    if (status != E_DB_OK)
		    {
			if (dsh->dsh_error.err_code == E_QE0015_NO_MORE_ROWS ||
			    dsh->dsh_error.err_code == E_QE00A5_END_OF_PARTITION &&
			    node->qen_flags & QEN_PART_SEPARATE)
			{
			    if ( qen_hold )
			    {
			        qen_hold->hold_buffer_status =
					HFILE6_BUF_EMPTY;
			    }

			    if (dsh->dsh_error.err_code == E_QE00A5_END_OF_PARTITION)
				qen_status->node_status_flags |= QEN4_IPART_END;
					/* if just EOPartition, flag it */

			    /* mark EOF on stream */
			    qen_status->node_u.node_join.node_inner_status = QEN11_INNER_ENDS;
			    if((!qen_hold) || 
			        qen_hold->hold_status == HFILE2_AT_EOF ||
			        qen_hold->hold_status == HFILE0_NOFILE)
			    {
			        qen_status->node_u.node_join.node_hold_stream = 
			        	    QEN5_HOLD_STREAM_EOF;
			    }
			}
			else if (dsh->dsh_error.err_code == E_QE00A5_END_OF_PARTITION)
			{
			    /* No more rows in partitioning group - read from 
			    ** next group. */
			    in_reset = TRUE;
			    goto newinner;
			}
			else	/* other, fatal error */
			{
			    break;	/* want to return */
			}
		    }
		    else
		    {
		        /* Materialize the inner tuple into the row buffer. */
			status = qen_execute_cx(dsh, node_xaddrs->qex_itmat);
			if (status != E_DB_OK)
			    goto errexit;   /* if ade error, return error */
		        qen_hold->hold_buffer_status = HFILE8_FROM_INNER;
		        if( qen_hold &&
		             !( qen_status->node_access & QEN_LOOKING_FOR_RIGHT_JOINS))
		        {
		            /* append to hold */
	                    status = qen_u1_append(qen_hold, qen_shd, dsh); 
		            if(status) break;  /* to return error */
		        }
		    }	/* endif at EOF or not */
	        }   /* end of read from hold/inner */
	    }	/* end if inner stream not exhausted */
	}    /* end if read_inner */

	/***************************************************************
	**
	**	LOOK FOR RIGHT JOINS
	**
	**
	***************************************************************/

	if ( qen_status->node_access & QEN_LOOKING_FOR_RIGHT_JOINS )
	{
	    /*
	    ** If we've finished rescanning the inner stream, then
	    ** we're done.
	    */

	    /*
	    ** This is one of the two ways that we can exit this node
	    ** for good.
	    */

	    if ( INNER_STREAM_EXHAUSTED )
	    {
	        status = E_DB_WARN;
		dsh->dsh_error.err_code = E_QE0015_NO_MORE_ROWS;
		qen_status->node_status = QEN4_NO_MORE_ROWS;
		break;
	    }

	    qen_status->node_access &= ~QEN_READ_NEXT_OUTER;
	    qen_status->node_access |= QEN_READ_NEXT_INNER;

	    /*
	    ** If we get all the way to the end of the following
	    ** conditional, then the current inner tid is not in the
	    ** temporary table of inner tids.  That is, this inner tuple
	    ** never inner joined with ANY outer tuple.  So it's
	    ** a candidate for being a right join.
	    */

	    if(qen_hold->hold_status != HFILE2_AT_EOF)
	    {
	        if ( !( qen_status->node_access & QEN_TID_FILE_EOF ) )
	        {
	            if ( status = qen_u8_current_tid(qen_hold, qen_shd,
				&tid) )
			goto errexit;

	            status = qen_readTidTempTable( tidTempTable, 
					   node,
					   dsh,
					   tid );

		    /* tid inner joined.  get new row from hold */
		    if ( status == E_DB_OK ) continue;

		    if ( status == E_DB_ERROR )
		    {
		        if ( dsh->dsh_error.err_code != E_QE0015_NO_MORE_ROWS )
			    goto errexit;
		    }
	        }
	    }

	    /* Generate right-join tuple with RNULL */
	    status = qen_execute_cx(dsh, node_xaddrs->qex_rnull);
	    if (status != E_DB_OK)
		goto errexit;   /* if ade error, return error */

	    /* Apply post-join restriction if any */
	    if (jqual_excb == NULL)
		break;		/* Emit the right join */
	    else
	    {
		status = qen_execute_cx(dsh, jqual_excb);
		if (status != E_DB_OK)
		    goto errexit;   /* if ade error, return error */
		if (jqual_excb->excb_value == ADE_TRUE)
		    break;	/* emit a right join */
	    }

	    continue;	/* evaluate next inner tuple for right joins */
	}	/* endif looking for right joins */

	/***************************************************************
	**
	**	COMPARE THE INNER AND OUTER JOIN KEYS
	**
	**
	***************************************************************/


	if ( INNER_STREAM_EXHAUSTED ||
	     ( qen_hold && qen_hold->hold_buffer_status == HFILE6_BUF_EMPTY ) )
	{
	    join_result = OUTER_LT_INNER;
	}

	else if(qen_status->node_u.node_join.node_outer_status == QEN8_OUTER_EOF)
	{
	    join_result = OUTER_GT_INNER;
	}

	else	/* we have an inner and outer.  join them on the join key. */
	{
	    join_result = ADE_1EQ2;
	    ade_excb = node_xaddrs->qex_joinkey;
	    if (ade_excb != NULL)
	    {
		status = qen_execute_cx(dsh, ade_excb);
		if (status != E_DB_OK)
		    goto errexit;   /* if ade error, return error */
		join_result = ade_excb->excb_cmp;
	    }

	    if (join_result == ADE_BOTHNULL)
	    {  
	        join_result = OUTER_GT_INNER;
	    }

	    else if (join_result == ADE_1ISNULL)
	    {
	        join_result = OUTER_LT_INNER;
	    }

	    else if (join_result == ADE_2ISNULL)
	    {
	        join_result = OUTER_GT_INNER;
	    }
	}	/* endif we have inner and outer */

	/***************************************************************
	**
	**	OUTER AND INNER KEYS NOW JOINED.  PERFORM OTHER
	**	QUALIFICATIONS NOW.  EMIT JOINS WHERE APPROPRIATE.
	**
	***************************************************************/


	if (join_result == OUTER_LT_INNER)
	{                           
	    qen_status->node_access |= QEN_READ_NEXT_OUTER;
	    qen_status->node_access &= ~QEN_READ_NEXT_INNER;
	    continue;	/* get next outer */
	}

	if ( join_result == OUTER_GT_INNER )
	{
	    qen_status->node_access &= ~QEN_READ_NEXT_OUTER;
	    qen_status->node_access |= QEN_READ_NEXT_INNER;
	    continue;	/* get next inner */
	}    /* endif outer greater than inner */
		
	/* We come to this point when kqual returns OUTER_EQ_INNER */ 

	if ( join_result != OUTER_EQ_INNER )
	{
	    /* consistency check */
	    dsh->dsh_error.err_code = E_QE0002_INTERNAL_ERROR;
	    status = E_DB_ERROR;
	    goto errexit;           
	}	/* end consistency check */

	/* If this is the first inner that matches the current outer
	** key, save the hold file TID so we can reposition it later.
	*/
	if ( !( qen_status->node_access & QEN_RESCAN_MARKED ) )
	{
	    if ( qen_hold )
	    {
	        if ( qen_u5_save_position(qen_hold, qen_shd) )
		    goto errexit;
	    }
	    qen_status->node_access |= QEN_RESCAN_MARKED;
	}
	else if ((node->qen_flags & QEN_CARD_MASK) == QEN_CARD_01R &&
		(qen_status->node_access & QEN_OUTER_HAS_JOINED) != 0)
	{
	    /* Left outer - note cardinality */
	    potential_card_violation = TRUE;
	}

	qen_status->node_access &= ~QEN_READ_NEXT_OUTER;
	qen_status->node_access |= QEN_READ_NEXT_INNER;

	/* execute OQUAL */
	ade_excb = node_xaddrs->qex_onqual;
	status = qen_execute_cx(dsh, ade_excb);
	if (status != E_DB_OK)
	    goto errexit;   /* if ade error, return error */
	if (ade_excb == NULL || ade_excb->excb_value == ADE_TRUE)
	{
	    /* OQUAL succeeds.  Remember that a join occurred. */
	    qen_status->node_access |= QEN_OUTER_HAS_JOINED;  

	    if(rjoin)
	    {
		/* append the current inner holdfile tid to tidfile */
		if (  status = qen_u8_current_tid(qen_hold, qen_shd, &tid) )
		    goto errexit;
		if(  status = qen_appendTidTempTable( tidTempTable,
			node, dsh, tid) )
		    goto errexit;
	    }

	    status = qen_execute_cx(dsh, node_xaddrs->qex_eqmat);
	    if (status != E_DB_OK)
		goto errexit;   /* if ade error, return error */
            if ( (status = qen_execute_cx(dsh, jqual_excb)) )
		goto errexit;   /* if ade error, return error */
	    if ( jqual_excb == NULL || jqual_excb->excb_value == ADE_TRUE )
	    {
		/* JQUAL succeeds */
		if(node->node_qen.qen_sjoin.sjn_kuniq)  /* if kuniq */
		{
		    /* make next entry read new outer bit not new inner */
		    qen_status->node_access |= QEN_READ_NEXT_OUTER;
		    qen_status->node_access &= ~QEN_READ_NEXT_INNER;
		}	/* endif key unique */
		if (potential_card_violation)
		{
		    /* We only want to act on seltype violation after
		    ** qualification and that is now. */
		    qen_status->node_status = QEN7_FAILED;
		    dsh->dsh_error.err_code = E_QE004C_NOT_ZEROONE_ROWS;
		    status = E_DB_ERROR;
		    goto errexit;
		}

		break;  /* emit inner join */
	    }
	}  /* end of OQUAL true */

	/* OQUAL or JQUAL failed.  Get next inner. */

	continue;

    }	/* end of get loop */

    /********************************************************************
    **
    **	CLEANUP.  MATERIALIZE FUNCTION ATTRIBUTES.  ERROR EXIT WHEN
    **	APPROPRIATE.
    **
    ********************************************************************/


    if (status == E_DB_OK)
    {
	status = qen_execute_cx(dsh, node_xaddrs->qex_fatts);
	if (status != E_DB_OK)
	    goto errexit;   

	/* Increment row count (for qe90) */
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
	    if (status == E_DB_OK)
	    {
		status = qen_print_row(node, qef_rcb, dsh);
		if (status != E_DB_OK)
		{
		    goto errexit;
		}
	    }
	}

#ifdef xDEBUG
	(VOID) qe2_chk_qp(dsh);
#endif 
    }
    /*
    ** release sort memory and hold file if we're exiting this routine
    ** for good.  why is this here and not under errexit?  i'll tell you
    ** why:  end of query cleanup will release this memory anyway;  we're
    ** just being good citizens and releasing memory as soon as we can
    ** to help out other qen nodes that might be struggling for space.
    */
    else
    {
	if(in_node->qen_type == QE_SORT)  /* if sort child */
	/* release the memory now if in memory */
	{
	    qen_u31_release_mem(qen_hold, dsh,
	    	  dsh->dsh_shd[in_node->node_qen.qen_sort.sort_shd] );
	}
	else
	if(qen_hold)  /* if hold file */
	{
	    /* release our hold file, if in memory */
	    qen_u31_release_mem(qen_hold, dsh, qen_shd );
	}
    }


errexit:

    if ((dsh->dsh_error.err_code == E_QE0015_NO_MORE_ROWS ||
	dsh->dsh_error.err_code == E_QE00A5_END_OF_PARTITION) &&
	(qen_status->node_status_flags & 
			(QEN2_OPART_END | QEN4_IPART_END)))
    {
	/* Restart using next partitioning group. */
	out_reset = in_reset = reset = TRUE;
	qen_status->node_status_flags &=
			~(QEN2_OPART_END | QEN4_IPART_END);
	goto loop_reset;
    }

    if (dsh->dsh_qp_stats)
    {
	qen_ecost_end(dsh, &timerstat, qen_status);
    }

    return (status);	    
}

/*
** Name: repositionForRightJoins - reposition inner stream to find right joins
**
** Description:
**
**	This routine repositions the inner hold file and the tid temp table
**	to beginning of file so that the two files can be read against
**	one another and right joins identified.
**
** Inputs:
**	node -
**	    Current qen node.
**	qef_rcb -
**	    The request control block that we get most of our query common 
**	    info from, like cbs, the dsh, dsh rows, etc.
**	in_reset -
**	    Whether or not to reset the inner node when sorting it.
**
**
** Outputs:
**	status words for the hold files are updated
**	in_reset -
**	    Whether or not to reset the inner node when sorting it.
**
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**
** History:
**      01-jun-92 (rickh)
**          created
**	17-may-93 (rickh)
**	    Moved most of the guts of this routine to rewindInnerStream.
**	27-feb-04 (inkdo01)
**	    Changed dsh ptr to arrays of structs (SHD, HOLD, STATUS) to
**	    ptrs to arrays of ptrs.
[@history_template@]...
*/
static DB_STATUS
repositionForRightJoins(
QEN_NODE	*node,
QEF_RCB		*qef_rcb,
QEE_DSH		*dsh,
bool		*in_reset )
{
    QEN_HOLD	    *qen_hold = (QEN_HOLD *)NULL;
    QEN_SHD         *qen_shd = (QEN_SHD *)NULL;
    QEN_TEMP_TABLE  *tidTempTable =
			node->node_qen.qen_sjoin.sjn_oj->oj_innerJoinedTIDs;
    DB_STATUS	    status = E_DB_OK;

    if(node->node_qen.qen_sjoin.sjn_hfile >= 0)
    {
	qen_hold = dsh->dsh_hold[node->node_qen.qen_sjoin.sjn_hfile];
	qen_shd = dsh->dsh_shd[dsh->dsh_qp_ptr->qp_sort_cnt +
	                        node->node_qen.qen_sjoin.sjn_hfile];
    }

    for ( ; ; )		/* the usual procedure block to break out of */
    {
	status = rewindInnerStream( node, qef_rcb, dsh, in_reset );
	if(status != E_DB_OK)	{ break; }	/* to error */

	/* reposition tid hold file to start of file, too */

	status = qen_rewindTidTempTable( tidTempTable, node, dsh);
	if(status != E_DB_OK)	{ break; }	/* to error */
	break;

    }	/* end of the usual and customary procedure block */

    return( status );
}

/*
** Name: rewindInnerStream - reposition inner stream to beginning
**
** Description:
**
**	This routine repositions the inner hold file to its beginning.
**
** Inputs:
**	node -
**	    Current qen node.
**	qef_rcb -
**	    The request control block that we get most of our query common 
**	    info from, like cbs, the dsh, dsh rows, etc.
**	in_reset -
**	    Whether or not to reset the inner node when sorting it.
**
**
** Outputs:
**	status words for the hold files are updated
**	in_reset -
**	    Whether or not to reset the inner node when sorting it.
**
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**
** History:
**	17-may-93 (rickh)
**          created by carving out the guts of repositionForRightJoins
**	    and adding an unget call.
**	19-oct-93 (rickh)
**	    Removed unget logic.
**	27-feb-04 (inkdo01)
**	    Changed dsh ptr to arrays of structs (SHD, HOLD, STATUS) to
**	    ptrs to arrays of ptrs.
[@history_template@]...
*/
static DB_STATUS
rewindInnerStream(
QEN_NODE	*node,
QEF_RCB		*qef_rcb,
QEE_DSH		*dsh,
bool		*in_reset )
{
    QEN_STATUS	    *qen_status = dsh->dsh_xaddrs[node->qen_num]->qex_status;
    QEN_NODE	    *in_node = node->node_qen.qen_sjoin.sjn_inner;
    QEN_HOLD	    *qen_hold = (QEN_HOLD *)NULL;
    QEN_SHD         *qen_shd = (QEN_SHD *)NULL;
    DB_STATUS	    status = E_DB_OK;

    if(node->node_qen.qen_sjoin.sjn_hfile >= 0)
    {
	qen_hold = dsh->dsh_hold[node->node_qen.qen_sjoin.sjn_hfile];
	qen_shd = dsh->dsh_shd[dsh->dsh_qp_ptr->qp_sort_cnt +
	                        node->node_qen.qen_sjoin.sjn_hfile];
    }

    for ( ; ; )		/* the usual procedure block to break out of */
    {
	if( in_node->qen_type == QE_SORT &&
	    qen_status->node_status == QEN0_INITIAL )  /* if outer was empty */
	{
	    status = qen_u32_dosort(in_node, 
				    qef_rcb, 
				    dsh, 
				    qen_status, 
				    qen_hold,
				    qen_shd, 
				    (*in_reset) ? FUNC_RESET : NO_FUNC);

	    if(status) break;
	    *in_reset = FALSE;
	    qen_status->node_status = QEN1_EXECUTED;  /* init done */
	}
	qen_status->node_access &= ~QEN_RESCAN_MARKED;

	if ( qen_hold )
	{
	    qen_hold->hold_status = HFILE2_AT_EOF;
	    /* reposition to read first tuple */
	    if (  status = qen_u3_position_begin(qen_hold, dsh, qen_shd) )
	        break;
	}

	break;
    }	/* end of the usual and customary procedure block */

    return( status );
}

/*
** Name: backupInnerStream - backup inner stream to the saved tid
**
** Description:
**
**	This routine repositions the inner stream at the tid of the first
**	tuple which matched the last outer.
**
**
**
** Inputs:
**	node -
**	    Current qen node.
**	dsh - Thread's data segment header
**
**
** Outputs:
**	status words for the hold files are updated
**	in_reset -
**	    Whether or not to reset the inner node when sorting it.
**
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**
** History:
**      01-jun-92 (rickh)
**          created
**	27-feb-04 (inkdo01)
**	    Changed dsh ptr to arrays of structs (SHD, HOLD, STATUS) to
**	    ptrs to arrays of ptrs.
[@history_template@]...
*/
static DB_STATUS
backupInnerStream(
QEN_NODE	*node,
QEE_DSH		*dsh,
bool		*in_reset )
{
    QEN_STATUS	    *qen_status = dsh->dsh_xaddrs[node->qen_num]->qex_status;
    QEN_HOLD	    *qen_hold = (QEN_HOLD *)NULL;
    QEN_SHD         *qen_shd = (QEN_SHD *)NULL;
    DB_STATUS	    status = E_DB_OK;

    if(node->node_qen.qen_sjoin.sjn_hfile >= 0)
    {
	qen_hold = dsh->dsh_hold[node->node_qen.qen_sjoin.sjn_hfile];
	qen_shd = dsh->dsh_shd[dsh->dsh_qp_ptr->qp_sort_cnt +
	                        node->node_qen.qen_sjoin.sjn_hfile];
    }

    for ( ; ; )		/* the usual procedure block to break out of */
    {
	if(!qen_hold )
	    *in_reset = TRUE;   /* force reset on inner */
	else
	{
            status = qen_u2_position_tid(qen_hold, dsh, qen_shd);
            if(status != E_DB_OK) break;   /* to error */
	}

        /* repositions set off hold_stream_eof */
        qen_status->node_u.node_join.node_hold_stream = 0;
        qen_status->node_access |= QEN_READ_NEXT_INNER;

	break;
    }	/* end of the usual and customary procedure block */

    return( status );
}
