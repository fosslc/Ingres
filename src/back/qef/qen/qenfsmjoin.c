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
**  Name: QENFSMJOIN.C - full sort merge join interpreter
**
**  Description:
**
**
**  History:
**      07-nov-89 (davebf)
**          written  (adapted from qenjoin.c 6.3)
**	11-mar-92 (jhahn)
**	    Moved calls to CSswitch to top of qen_join. (see 16-jul-91)
**	    CSswitch is now called at the top of every qen node.
**	27-apr-92 (rickh)
**	    Right joins just didn't work.  Also, outer joins with nulls in the
**	    join key were simply dropped on the floor.  Substantially rewrote
**	    to fix the right join problem.
**	08-dec-92 (anitap)
**	    Included <qsf.h> and <psfparse.h> for CREATE SCHEMA.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      14-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
**	30-dec-93 (rickh)
**	    Upon reset, set node status to QEN0_INITIAL.  This is done in 6.4.
**	    Re-initializing on reset makes subselects work.
**	08-oct-93 (swm)
**	    Bug #56448
**	    Altered qec_tprintf calls to conform to new portable interface.
**	    Each call is replaced with a call to STprintf to deal with the
**	    variable number and size of arguments issue, then the resulting
**	    formatted buffer is passed to qec_tprintf. This works because
**	    STprintf is a varargs function in the CL. Unfortunately,
**	    qec_tprintf cannot become a varargs function as this practice is
**	    outlawed in main line code.
**	18-mar-94 (swm)
**	    Bug #59883
**	    Remove format buffers declared as automatic data on the stack with
**	    references to per session buffer allocated from ulm memory at
**	    QEF session startup time.
**	19-oct-94	(ramra01)
**	Bug #65389
**		During an FSM JOIN involving a sub select Non equijoin condition
**	the hold buffer in memory was hard coded to hold a tup cnt of 20 which
**	could cause a SEG Violation when it goes beyond that.
**      01-nov-95 (ramra01)
**          Prior to calling qen_u21_execute_cx confirm the presence of
**          a adf control block to execute
**	7-nov-95 (inkdo01)
**	    Changes to replace QEN_ADF and QEN_OJINFO structure instances by
**	    pointers in QEN_SJOIN.
**	05-feb-97 (kch)
**	    In qen_fsmjoin(), we now increment the counter of the rows that
**	    the node has returned. This change fixes bug 78836.
**	24-feb-97 (pchang)
**	    Fixed a flaw in the reset logic that prevented the dumping of the
**	    hold file and the subsequent allocation of new sort vector arrays
**	    when the node was to be reset.  The error had resulted in SIGBUS
**	    caused by memory overrun in the sort vector array.  (B79874)
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	23-feb-04 (inkdo01)
**	    Changed qef_rcb->error to dsh->dsh_error for || thread safety.
**	9-Dec-2005 (kschendel)
**	    Fix a stray i2 cast on an mecopy.
**	28-Dec-2005 (kschendel)
**	    Use DSH references whenever possible instead of QEF RCB
**	    references.  (This simplifies the code, eliminating many
**	    RCB references, and reflects the new primacy of the DSH
**	    thanks to the parallel query changes.)
**	12-Jan-2006 (kschendel)
**	    Update CX execution calls.
**	2-Dec-2010 (kschendel) SIR 124685
**	    Warning, prototype fixes.
**/

/*	static functions	*/

static DB_STATUS
repositionInnerStream(
QEN_NODE	*node,
QEE_DSH		*dsh );

static DB_STATUS
clearHoldFiles(
QEN_NODE	*node,
QEE_DSH		*dsh );





/*{
** Name: QEN_FSMJOIN	- full sort merge interpreter       
**
** Description:
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
**	been set to that which will produce the joined row.
**
**
** Here's the full sort merge algorithm:
**
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
**begin fsmjoin
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
**		if ( inner stream exhausted )
**		or rjoin=FALSE
**		    done
**		endif
**	    endif
**
**	    if not first outer read
**
**	        if new key is same as old and innerHoldFile not empty
**		    reposition innerHoldFile to rescanStartPoint
**		    position innerJoinFlags at start of file
**		    unget inner tuple if from inner stream
**		    readInner = TRUE
**		    continue;
**	        endif new key not same as old
**
**		else if ( new key not equal to old or outer at EOF )
**		and innerHoldFile not empty
**		    if rjoin
**		        reposition innerHoldFile to rescanStartPoint
**		        position innerJoinFlags at start of file
**		        unget inner tuple if from inner stream
**
**		        lookForRightJoins = TRUE
**		        readInner = TRUE
**		        continue
**		    else not rjoin
**			clear innerHoldFile
**		    endif
**		endif new greater than old
**
**	    endif not first outer read
**	endif readOuter==TRUE
**
**	// read inner //
**
**	if readInner is TRUE
**
**	    if inner stream is not exhausted
**
**	        if rjoin and we've repositioned innerJoinFlags
**		    read from innerJoinFlags
**		    if innerJoinFlags scanned to end
**		        if lookForRightJoins
**			    clear hold file
**			    clear innerJoinFlags
**			    lookForRightJoins = FALSE
**			    readOuter = FALSE
**			    readInner = TRUE
**			    continue
**		        endif
**		    endif innerJoinFlags scanned to end
**	        endif rjoin and we're rescanning  hold files
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
**	    readOuter = FALSE
**	    readInner = TRUE
**	    if innerJoinFlags == FALSE
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
**	if inner stream exhausted or the inner tuple buffer is empty
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
**	    if rjoin
**		execute RNULL
**		if jqual passes
**		    emit right join
**		endif
**	    endif
**	    continue
**	endif outer greater than inner
**
**	// else inner key = outer key //
**
**	append inner to hold file if from inner stream
**	if this is the first inner to match
**	    save tid as rescanStart
**	endif
**
**	readInner = TRUE
**	readOuter = FALSE
**
**	execute OQUAL
**	if oqual succeeds
**	    outerHasJoined = TRUE
**	    inner_join_flags( TRUE )
**	    execute IJMAT
**	    execute jqual
**	    if jqual succeeds
**		emit inner join
**	    endif
**	else oqual failed
**	    inner_join_flags( FALSE )
**	endif
**
**    end for
**end fsmjoin
**
**
** History:
**      07-nov-89 (davebf)
**          written   (adapted from qen_join  6.3)
**
**	    To help QEF determine what to do when a null in encountered in
**	    a multi-attribute join, ADF will return 3 more values in
**	    xcxcb.excb_cmp, ADE_1ISNULL, ADE_2ISNULL, and ADE_BOTHNULL.
**          QEF logic has been adjusted to handle these 3 new values.
**	11-mar-92 (jhahn)
**	    Added support for QEF interrupts.
**	    Added call to CSswitch.
**	27-apr-92 (rickh)
**	    Right joins just didn't work.  Also, outer joins with nulls in the
**	    join key were simply dropped on the floor.  Substantially rewrote
**	    to fix the right join problem.
**	30-dec-93 (rickh)
**	    Upon reset, set node status to QEN0_INITIAL.  This is done in 6.4.
**	    Re-initializing on reset makes subselects work.
**	19-oct-94 (ramra01)
**		Sort hold in memory could cause problems if the tuple count
**		is limited to 20 (hard coded). b65389
**      01-nov-95 (ramra01)
**          Prior to calling qen_u21_execute_cx confirm the presence of
**          a adf control block to execute
**	7-nov-95 (inkdo01)
**	    Changes to replace QEN_ADF and QEN_OJINFO structure instances by
**	    pointers in QEN_SJOIN.
**	05-feb-97 (kch)
**	    We now increment the counter of the rows that the node has
**	    returned, qen_status->node_rcount. This change fixes bug 78836.
**	24-feb-97 (pchang)
**	    Fixed a flaw in the reset logic that prevented the dumping of the
**	    hold file and the subsequent allocation of new sort vector arrays
**	    when the node was to be reset.  The error had resulted in SIGBUS
**	    caused by memory overrun in the sort vector array.  (B79874)
**	2-jan-03 (inkdo01)
**	    Removed noadf_mask logic in favour of QEN_ADF * tests to fix
**	    concurrency bug.
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
**	28-apr-04 (inkdo01)
**	    Add support for "end of partition group" call.
**	04-Nov-2004 (jenjo02)
**	    Only CSswitch on root DSH or if not OS-threaded.
**	16-Feb-2006 (kschendel)
**	    Return after close, don't keep going.
**	13-Feb-2007 (kschendel)
**	    Replace CSswitch with better cancel check.
**	4-Jun-2009 (kschendel) b122118
**	    Use slightly faster/more convenient dsh test for qe90, and
**	    remove pointless error returns from cost begin/end.  Minor
**	    streamlining.  Put a wrapper on qen-sort to make it look more
**	    like a regular node iterator.
**	11-Nov-2009 (kiria01) SIR 121883
**	    Scalar sub-query - Pick up on cardinality errors if requested.
*/


#define	INNER_STREAM_EXHAUSTED ( \
	 (  qen_status->node_u.node_join.node_inner_status == QEN11_INNER_ENDS && \
	    qen_hold->hold_status != HFILE3_POSITIONED && \
         !( qen_status->node_access & QEN_RESCAN_MARKED ) ) )



DB_STATUS
qen_fsmjoin(
QEN_NODE	*node,
QEF_RCB		*qef_rcb,
QEE_DSH		*dsh,
i4		function )
{
    QEF_CB	    *qef_cb = dsh->dsh_qefcb;
    QEN_NODE	    *out_node = node->node_qen.qen_sjoin.sjn_out;
    QEN_NODE	    *in_node = node->node_qen.qen_sjoin.sjn_inner;
    QEE_XADDRS	    *node_xaddrs = dsh->dsh_xaddrs[node->qen_num];
    QEN_STATUS	    *qen_status = node_xaddrs->qex_status;
    ADE_EXCB	    *ade_excb;
    ADE_EXCB	    *jqual_excb = node_xaddrs->qex_jqual;
    QEN_HOLD	    *qen_hold;
    QEN_HOLD	    *ijFlagsHold = (QEN_HOLD *)NULL;
    QEN_SHD         *qen_shd;
    QEN_SHD         *ijFlagsShd;
    DB_STATUS	    status = E_DB_OK;
    bool	    reset = FALSE;
    bool	    out_reset = FALSE;
    bool	    in_reset = FALSE;
    bool            ojoin = (node->node_qen.qen_sjoin.sjn_oj != NULL); 
    bool            ljoin = FALSE;
    bool            rjoin = FALSE;
    bool	    innerTupleJoined;
    bool	    rematerializeInnerTuple = TRUE;
			/* During full joins, the last driving tuple may left
			** join.  This 0s all special eqcs from the
			** re-scannable stream.  The current re-scannable
			** tuple will right join.  To recover the state of
			** its special eqcs, simply re-materialize the inner
			** tuple.  That's what this variable is for.
			*/
    i4		    new_to_old;
    i4		    join_result;
    i4	    val1;
    i4	    val2;
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
	if ((function & TOP_OPEN || function & MID_OPEN) 
	    && !(qen_status->node_status_flags & QEN1_NODE_OPEN))
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
		/* Ideally we would clean up all of our own shd crap here
		** instead of making qee do it...
		*/
		status = (*out_node->qen_func)(out_node, qef_rcb, dsh, FUNC_CLOSE);
		status = (*in_node->qen_func)(in_node, qef_rcb, dsh, FUNC_CLOSE);
		qen_status->node_status_flags = 
		    (qen_status->node_status_flags & ~QEN1_NODE_OPEN) | QEN8_NODE_CLOSED;
	    }
	    return(E_DB_OK);
	}

	/* End of partition group call just gets passed down. */
	if (function & FUNC_EOGROUP)
	{
	    status = (*out_node->qen_func)(out_node, qef_rcb, dsh, FUNC_EOGROUP);
	    status = (*in_node->qen_func)(in_node, qef_rcb, dsh, FUNC_EOGROUP);
	    return(E_DB_OK);
	}
    } /* if function */

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

    qen_hold = dsh->dsh_hold[node->node_qen.qen_sjoin.sjn_hfile];
    qen_shd = dsh->dsh_shd[dsh->dsh_qp_ptr->qp_sort_cnt +
                            node->node_qen.qen_sjoin.sjn_hfile];

    if( ojoin && node->node_qen.qen_sjoin.sjn_oj->oj_ijFlagsFile >= 0 )
    {
        ijFlagsHold =
	  dsh->dsh_hold[node->node_qen.qen_sjoin.sjn_oj->oj_ijFlagsFile];
        ijFlagsShd = dsh->dsh_shd[dsh->dsh_qp_ptr->qp_sort_cnt +
                            node->node_qen.qen_sjoin.sjn_oj->oj_ijFlagsFile];
    }

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

    /* If the node is to be reset, dump the hold file and reset the
    ** inner/outer nodes */

loop_reset:
    if (reset)
    {
	if (qen_status->node_status != QEN0_INITIAL && 
	    in_node->qen_type != QE_SORT)
	{
	    /* reset in memory or dump dmf hold if it has been created */
	    status = qen_u9_dump_hold(qen_hold, dsh, qen_shd);
	    if(status) goto errexit;
	    qen_hold->hold_medium = HMED_IN_MEMORY;  /* set back to mem */
	}
	qen_hold->hold_buffer_status = HFILE6_BUF_EMPTY;

	if ( qen_status->node_status != QEN0_INITIAL && ijFlagsHold )
	{
	    /* dump tid hold file if it has been created */

	    status = qen_u9_dump_hold( ijFlagsHold, dsh, ijFlagsShd );
	    if(status) goto errexit;  
	    ijFlagsHold->hold_medium = HMED_IN_MEMORY;  /* set back to mem */
	}

	qen_status->node_status = QEN0_INITIAL;  /* reset = reintialize */
	qen_status->node_u.node_join.node_inner_status = QEN0_INITIAL;
	qen_status->node_u.node_join.node_outer_status = QEN0_INITIAL;
	qen_status->node_u.node_join.node_outer_count = 0;

	qen_status->node_access = (	QEN_READ_NEXT_OUTER |
					QEN_READ_NEXT_INNER |
					QEN_OUTER_HAS_JOINED	);  
    }

    if (qen_status->node_status == QEN0_INITIAL)  
    {
	qen_status->node_u.node_join.node_outer_status = QEN0_INITIAL;

	/* set num entries in mem_hold in case we build one */
	/* this may not be a hard number in future */
	/* qen_shd->shd_tup_cnt = 20; */

	/* by setting it to -1, the required memory will be configured to */
	/* suit the condition. if it is < 20, it will use the dmf hold mem*/
	/* ramra01 19-oct-94 */
	qen_shd->shd_tup_cnt = -1;

	if( ijFlagsHold )
	{
	    ijFlagsHold->hold_status = HFILE0_NOFILE;	/* default */
	    ijFlagsHold->hold_status2 = 0;		/* default */
	    ijFlagsHold->hold_medium = HMED_IN_MEMORY;  /* default */
	    /* in case we build a hold file
	    ** tell qen_u1_append to calculate its size in memory
	    ** or go to DMF hold */
	    ijFlagsShd->shd_tup_cnt = -1;
	}

	if(rjoin)
	{
	    /* consistency check */
	    if( !ijFlagsHold )
	    {
		/* rjoin and no hold file for inner join flags */
		dsh->dsh_error.err_code = E_QE0002_INTERNAL_ERROR;
		status = E_DB_ERROR;
		goto errexit;           
	    }
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
	    ** any inner tuples, then it's an outer join.  Return
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

		/* Execute jqual restriction, if any */
		if ( jqual_excb == NULL)
		    break;	/* emit a left join */
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
	        status = E_DB_WARN;
		dsh->dsh_error.err_code = E_QE0015_NO_MORE_ROWS;
	    }
	    else	/* this is where we actually read the outer stream! */
	    {
	        status = (*out_node->qen_func)(out_node, qef_rcb, dsh,
				(out_reset) ? FUNC_RESET : NO_FUNC);
		if (status == E_DB_OK)
		    qen_status->node_u.node_join.node_outer_count++;
	    }
	    out_reset = FALSE;

	    /* a little error handling.  check for end of outer stream */
	    if (status != E_DB_OK)
	    {
		if (dsh->dsh_error.err_code == E_QE0015_NO_MORE_ROWS ||
		    (dsh->dsh_error.err_code == E_QE00A5_END_OF_PARTITION &&
		    node->qen_flags & QEN_PART_SEPARATE))
		{
		    /* If no outer rows were read and we're doing partition
		    ** grouping, skip the next inner partition to re-sync. */
		    if (dsh->dsh_error.err_code == E_QE00A5_END_OF_PARTITION &&
			qen_status->node_u.node_join.node_outer_count <= 0)
		     if (!rjoin)
		     {
			if (node->qen_flags & QEN_PART_SEPARATE)
			{
			    status = (*in_node->qen_func)(in_node, qef_rcb, 
						dsh, FUNC_EOGROUP);
			    if (dsh->dsh_error.err_code == 
						E_QE00A5_END_OF_PARTITION)
				qen_status->node_status_flags |=
						QEN4_IPART_END;
					/* if just EOPartition, flag it */
			}
		     goto errexit;
		     }

		    qen_status->node_access |= QEN_OUTER_HAS_JOINED;
                    qen_status->node_u.node_join.node_outer_status = QEN8_OUTER_EOF;

		    if (dsh->dsh_error.err_code == E_QE00A5_END_OF_PARTITION)
			qen_status->node_status_flags |= QEN2_OPART_END;
					/* if just EOPartition, flag it */

		    /*
		    ** If ( the inner stream is exhausted and there's nothing
		    ** to rescan ) or we're not right joining,
		    ** then there are no more tuples to return.  This should
		    ** be the only way to end this fsmjoin node.
		    */
		    if ( INNER_STREAM_EXHAUSTED || rjoin == FALSE  )
                    {
		       qen_status->node_status = QEN4_NO_MORE_ROWS;
                       break;    /* done */
                    }    
                    
                    else	/* we must check some more inner tuples */
		    {
                       dsh->dsh_error.err_code = 0;   /*reset*/

		       if(qen_status->node_status == QEN0_INITIAL)  /* empty */
		       {
			    qen_status->node_status = QEN1_EXECUTED;
			    qen_hold->hold_status = HFILE0_NOFILE;
			    qen_hold->hold_status2 = 0;
			    qen_hold->hold_medium = HMED_IN_MEMORY;
			    if(in_node->qen_type == QE_SORT)
			    {
				status = qen_u32_dosort(in_node, 
							qef_rcb, 
							dsh, 
							qen_status, 
							qen_hold,
							qen_shd, 
					(in_reset) ? FUNC_RESET : NO_FUNC);
				if(status) goto errexit;
			    }
			    in_reset = FALSE;
		       }	/* endif first time through */
		    }	/* endif no more inner tuples to check */
		}
		else if (dsh->dsh_error.err_code == E_QE00A5_END_OF_PARTITION)
		{
		    /* No more rows in partitioning group - read from 
		    ** next group. */
		    out_reset = TRUE;
		    goto newouter;
		}
		else  /* return error from reading outer stream */
		{
		    break;
		}
	    }	/* end of error handling for outer stream EOF */


            if(qen_status->node_status == QEN0_INITIAL)
            {
		qen_status->node_status = QEN1_EXECUTED;  /* init done */
		qen_hold->hold_status = HFILE0_NOFILE;  /* default */
		qen_hold->hold_status2 = 0;		/* default */
		qen_hold->hold_medium = HMED_IN_MEMORY;  /* default */
		if(in_node->qen_type  == QE_SORT)
		{

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
		if ( qen_status->node_u.node_join.node_outer_status == QEN8_OUTER_EOF )
		{
		    new_to_old = NEW_GT_OLD;
		}
		else	/* outer not at EOF */
		{
		    /* compare the old outer key to the new one. */
		    new_to_old = ADE_1EQ2;
		    if ((ade_excb = node_xaddrs->qex_kcompare) != NULL)
		    {
			status = qen_execute_cx(dsh, ade_excb);
			if (status != E_DB_OK)
			    goto errexit;   /* if ade error, return error */
			new_to_old = ade_excb->excb_cmp;
		    }

		    /* Materialize the new outer key if the old and the 
		    ** new outer keys are not equal */
		    if (new_to_old != ADE_1EQ2)
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
		}	/* endif outer not at EOF */

		/*
		** If there are inner tuples to rescan, decide whether
		** to thumb through them again or dump them.
		*/


		if ( qen_status->node_access & QEN_RESCAN_MARKED )
		{
		    if ( new_to_old == ADE_1EQ2 )
		    {
		        status = repositionInnerStream( node, dsh );
		        if(status != E_DB_OK) break;   /* to error */
		        continue;
		    }

		    else	/* key has changed */
		    {
		        if ( rjoin )
		        {
		            status = repositionInnerStream( node, dsh );
		            if(status != E_DB_OK) break;   /* to error */
		            qen_status->node_access |= QEN_LOOKING_FOR_RIGHT_JOINS;
		            continue; /* to get a new inner */
		        }
		        else	/* don't have to return right joins */
		        {
		            status = clearHoldFiles( node, dsh );
		            if(status != E_DB_OK) break;   /* to error */
		        }
		    }	/* endif comparison of new and old keys */
		}	/* endif there are inner tuples to rescan */
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
		/*
		** If we're rescanning the hold files and will eventually
		** have to look for right joins, read from the hold file
		** of inner join flags.
		*/
		if ( rjoin &&
		     ( ijFlagsHold->hold_status2 & HFILE_REPOSITIONED )  )
		{
		    if (qen_u40_readInnerJoinFlag( ijFlagsHold, dsh,
	    				ijFlagsShd,
					&innerTupleJoined ) != E_DB_OK)
		    {
			/*
			** If innerJoinFlags is exhausted and we were
			** looking for right joins, then we've found
			** all the right joins for this key.  Dump the
			** hold files.
			*/
		        if (dsh->dsh_error.err_code == E_QE0015_NO_MORE_ROWS)
		        {
			    /* Hold file ends, mark this. Continue reading. */
			    ijFlagsHold->hold_buffer_status = HFILE6_BUF_EMPTY;
			    ijFlagsHold->hold_status = HFILE2_AT_EOF;
			    if ( qen_status->node_access &
				 QEN_LOOKING_FOR_RIGHT_JOINS )
			    {
				qen_status->node_access &= 
				    ~( QEN_LOOKING_FOR_RIGHT_JOINS |
				       QEN_READ_NEXT_OUTER );
				qen_status->node_access |= 
				    QEN_READ_NEXT_INNER;
				status = clearHoldFiles( node, dsh );
				if(status != E_DB_OK) break;   /* to error */
				continue;	/* get next inner */
			    }
			}
			else	/* other errors are fatal */
			{
			    break;
			}
		    }	/* endif innerJoinFlags read wasn't OK */
		}	/* endif rjoin and rescanning hold files */

	        /* Read from hold file if it is positioned */
	        if (qen_hold->hold_status == HFILE3_POSITIONED)
	        {
		    if (qen_u4_read_positioned(qen_hold, dsh,
	    			       qen_shd) != E_DB_OK)
		    {
		        if (dsh->dsh_error.err_code == E_QE0015_NO_MORE_ROWS)
		        {
			    /* Hold file ends, must read from inner node */
			    qen_hold->hold_buffer_status = HFILE6_BUF_EMPTY;
			    qen_hold->hold_status = HFILE2_AT_EOF;
			    dsh->dsh_error.err_code = 0;
			    qen_status->node_access |= QEN_READ_NEXT_INNER;
			    if (node->qen_flags & QEN_PART_SEPARATE &&
				!(qen_hold->hold_status2 &
						HFILE_LAST_PARTITION))
			    {
				qen_status->node_status_flags |=
							QEN4_IPART_END;
				dsh->dsh_error.err_code = 
						E_QE00A5_END_OF_PARTITION;
			    }
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
			rematerializeInnerTuple = FALSE;
		    } /* end if hold from sort */

		    qen_hold->hold_buffer_status = HFILE7_FROM_HOLD;
	        }	/* end if positioned */
	        /* if not EOF on stream */
	        else if (qen_status->node_u.node_join.node_inner_status != QEN11_INNER_ENDS)
	        {
		    if(qen_hold->unget_status)   /* if occupied */
                    {
                        /* put unget in row buffer */
                        MEcopy((PTR)qen_hold->unget_buffer, 
			   qen_shd->shd_width,
                           (PTR)qen_shd->shd_row);
                        qen_hold->unget_status = 0;   /* set no unget */
		        qen_hold->hold_buffer_status = HFILE8_FROM_INNER;
                    }
                    else   /* get new from stream */
                    {
		newinner:
		        status = (*in_node->qen_func)(in_node, qef_rcb, dsh,
					(in_reset) ? FUNC_RESET : NO_FUNC);
		        in_reset = FALSE;
		        if (status != E_DB_OK)
		        {
			    if (dsh->dsh_error.err_code == E_QE0015_NO_MORE_ROWS ||
				(dsh->dsh_error.err_code == E_QE00A5_END_OF_PARTITION
				&& (node->qen_flags & QEN_PART_SEPARATE)) )
			    {
			        qen_hold->hold_buffer_status = HFILE6_BUF_EMPTY;

				if (dsh->dsh_error.err_code == E_QE00A5_END_OF_PARTITION)
				    qen_status->node_status_flags |= QEN4_IPART_END;
					/* if just EOPartition, flag it */

			        /* mark EOF on stream */
			        qen_status->node_u.node_join.node_inner_status = QEN11_INNER_ENDS;
			        if(qen_hold->hold_status == HFILE2_AT_EOF ||
			           qen_hold->hold_status == HFILE0_NOFILE ||
			           qen_hold->hold_status == HFILE1_EMPTY )
			        {
			           qen_status->node_u.node_join.node_hold_stream = 
			        	    QEN5_HOLD_STREAM_EOF;
			        }
			    }
			    else if (dsh->dsh_error.err_code == E_QE00A5_END_OF_PARTITION)
			    {
				/* No more rows in partitioning group - read 
				** from next group. */
				in_reset = TRUE;
				goto newinner;
			    }
			    else
			    {
			        break;	/* other, fatal error */
			    }
		        }
			else	/* inner tuple successfully read */
			{
		            /* Materialize the inner tuple into row buffer. */
			    status = qen_execute_cx(dsh, node_xaddrs->qex_itmat);
			    if (status != E_DB_OK)
				goto errexit;
		            qen_hold->hold_buffer_status = HFILE8_FROM_INNER;
			    rematerializeInnerTuple = FALSE;
			}
                    }  /* end if unget occupied */
	        }   /* end of read from hold/inner */
	    }	/* endif inner stream not exhausted */
	}    /* end if read_inner */

	/***************************************************************
	**
	**	LOOK FOR RIGHT JOINS
	**
	**
	***************************************************************/

	if ( qen_status->node_access & QEN_LOOKING_FOR_RIGHT_JOINS )
	{
	    qen_status->node_access &= ~QEN_READ_NEXT_OUTER;
	    qen_status->node_access |= QEN_READ_NEXT_INNER;

	    if ( innerTupleJoined == FALSE )
	    {
		status = qen_execute_cx(dsh, node_xaddrs->qex_rnull);
		if (status != E_DB_OK)
		    goto errexit;   /* if ade error, return error */
		if (jqual_excb == NULL)
		    break;	/* return right join */
		else
		{
		    status = qen_execute_cx(dsh, jqual_excb);
		    if (status != E_DB_OK)
			goto errexit;   /* if ade error, return error */
		    if (jqual_excb->excb_value == ADE_TRUE)
			break;  /* to return right join */
		}
	    }	/* endif inner tuple joined with some outer */

	    continue;	/* evaluate next inner tuple for right joins */
	}	/* endif looking for right joins */

	/***************************************************************
	**
	**	COMPARE THE INNER AND OUTER JOIN KEYS
	**
	**
	***************************************************************/


	if ( INNER_STREAM_EXHAUSTED ||
	     qen_hold->hold_buffer_status == HFILE6_BUF_EMPTY )
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
	    if ((ade_excb = node_xaddrs->qex_joinkey) != NULL)
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
	        join_result = OUTER_GT_INNER;
	    }

	    else if (join_result == ADE_2ISNULL)
	    {
	        join_result = OUTER_LT_INNER;
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

	    if ( rjoin )
	    {
		/* rematerialize inner tuple if the ultimate outer tuple
		** just left joined.  rematerialization will reset the
		** special equivalence classes from the inner stream.
		*/

		if ( rematerializeInnerTuple == TRUE )
		{
		    status = qen_execute_cx(dsh, node_xaddrs->qex_itmat);
		    if (status != E_DB_OK)
			goto errexit;   /* if ade error, return error */
		    rematerializeInnerTuple = FALSE;
		}

		/* execute oj_rnull */
		status = qen_execute_cx(dsh, node_xaddrs->qex_rnull);
		if (status != E_DB_OK)
		    goto errexit;   /* if ade error, return error */

		if (jqual_excb == NULL)
		    break;	/* return right join */
		else
		{
		    status = qen_execute_cx(dsh, jqual_excb);
		    if (status != E_DB_OK)
			goto errexit;   /* if ade error, return error */
		    if (jqual_excb->excb_value == ADE_TRUE)
			break;  /* to return right join */
		}
	    }
	    continue;	/* get next inner */
	}    /* endif outer greater than inner */
		
	/* We come to this point when joinkey returns OUTER_EQ_INNER */ 

	if ( join_result != OUTER_EQ_INNER )
	{
	    /* consistency check */
	    dsh->dsh_error.err_code = E_QE0002_INTERNAL_ERROR;
	    status = E_DB_ERROR;
	    goto errexit;           
	}	/* end consistency check */

	if (qen_hold->hold_buffer_status == HFILE8_FROM_INNER)
	{
	    /* append to hold */
            status = qen_u1_append(qen_hold, qen_shd, dsh); 
	    if(status) break;  /* to return error */
	}	

	/* If this is the first inner that joins with the current
	** outer, save the hold file TID so we can reposition it later.
	*/
	if ( !( qen_status->node_access & QEN_RESCAN_MARKED ) )
	{
	    if ( qen_u5_save_position(qen_hold, qen_shd) )
		goto errexit;
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
	    /* not OJ, or OQUAL succeeds.  Remember that a join occurred. */
	    qen_status->node_access |= QEN_OUTER_HAS_JOINED;  

	    if ( rjoin )
	    {
		if ( status = qen_u41_storeInnerJoinFlag( ijFlagsHold,
		ijFlagsShd, dsh, innerTupleJoined,
		     ( i4 ) TRUE ) ) goto errexit;   /* error */
	    }

	    /* set the special eqcs to "inner join" state */
	    status = qen_execute_cx(dsh, node_xaddrs->qex_eqmat);
	    if (status != E_DB_OK)
		goto errexit;   /* if ade error, return error */

	    if (jqual_excb != NULL)
	    {
		status = qen_execute_cx(dsh, jqual_excb);
		if (status != E_DB_OK)
		    goto errexit;   /* if ade error, return error */
	    }
	    if( jqual_excb == NULL || jqual_excb->excb_value == ADE_TRUE)
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
	}
	else	/* OQUAL failed */
	{
	    if ( rjoin )
	    {
	        if ( status = qen_u41_storeInnerJoinFlag( ijFlagsHold,
	            ijFlagsShd, dsh, innerTupleJoined,
		    ( i4 ) FALSE ) )
	        goto errexit;   /* error */
	    }

	}	/* end check of OQUAL status */

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
** Name: QEN_PRINT_ROW	- This routine prints the current row for a given 
**								    QEN_NODE
**
** Description:
**      This routine prints out the current row for the given QEN_NODE. 
**      This is done with the help of ADF, who converts data into a 
**      printable form, similiar to what the terminal monitor needs to do. 
**
**	FIXME this really belongs in qenutl since it applies to any node,
**	not just fsm-joins.
**
** Inputs:
**	node -
**	    The qen_node that refers to the row.
**	qef_rcb -
**	    The request control block that we get most of our query common 
**	    info from, like cbs, the dsh, dsh rows, etc.
**
** Outputs:
**
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    causes a row to be printed to the FE (via SCC_TRACE).
**
** History:
**      17-may-89 (eric)
**          created
**	6-aug-04 (inkdo01)
**	    Drop increment of node_rcount - it's already incremented in the 
**	    node handlers and this simply doubles its value.
**	13-Dec-2005 (kschendel)
**	    Can count on qen-ade-cx now.
**      11-Mar-2010 (hanal04) Bug 123415 
**          Make sure QE99 and OP173 do not interleave output with
**          using parallel query.
**	12-May-2010 (kschendel) Bug 123720
**	    Above doesn't quite go far enough, mutex the buffer.
**          Take qef_trsem for valid parallel-query output.
*/
DB_STATUS
qen_print_row(
QEN_NODE	*node,
QEF_RCB		*qef_rcb,
QEE_DSH		*dsh )
{
    QEN_STATUS	    *qen_status = dsh->dsh_xaddrs[node->qen_num]->qex_status;
    QEN_ADF	    *qen_adf;
    DB_STATUS	    status = E_DB_OK;
    char	    *cbuf = dsh->dsh_qefcb->qef_trfmt;
    i4		    cbufsize = dsh->dsh_qefcb->qef_trsize;

    qen_adf = node->qen_prow;		/* Checked non-NULL by caller */
    if (qen_status->node_rcount > 0)
    {
	/* process the row print expression */
	status = qen_execute_cx(dsh, dsh->dsh_xaddrs[node->qen_num]->qex_prow);
	if (status != E_DB_OK)
	{
# ifdef	xDEBUG
	    (VOID) qe2_chk_qp(dsh);
# endif
	    return (status);
	
	}

	/* Print in one operation to avoid interleaved output */
	CSp_semaphore(1, &dsh->dsh_qefcb->qef_trsem);
	STprintf(cbuf, "Row %d of node %d\n%s\n\n", 
			    qen_status->node_rcount, node->qen_num,
			    dsh->dsh_row[qen_adf->qen_output]);
        qec_tprintf(qef_rcb, cbufsize, cbuf);
	CSv_semaphore(&dsh->dsh_qefcb->qef_trsem);
    }			
    return(E_DB_OK);
}


/*
** Name: repositionInnerStream - back up the hold files
**
** Description:
**
**	This routine backs up the hold files.  For the hold file of
**	inner tuples, this routine backs up to the TID that marks where
**	rescanning should begin.  For the hold file of inner join flags,
**	this routine backs up to the very beginning.
**
** Inputs:
**	node -
**	    Current qen node.
**	dsh -
**
** Outputs:
**	status words for the hold files are updated
**
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**
** History:
**      05-may-92 (rickh)
**          created
**	27-feb-04 (inkdo01)
**	    Changed dsh ptr to arrays of structs (SHD, HOLD, STATUS) to
**	    ptrs to arrays of ptrs.
[@history_template@]...
*/
static DB_STATUS
repositionInnerStream(
QEN_NODE	*node,
QEE_DSH		*dsh )
{
    QEN_STATUS	    *qen_status = dsh->dsh_xaddrs[node->qen_num]->qex_status;
    QEN_HOLD	    *qen_hold;
    QEN_HOLD	    *ijFlagsHold;
    QEN_OJINFO	    *oj = node->node_qen.qen_sjoin.sjn_oj;
    QEN_SHD         *qen_shd;
    QEN_SHD         *ijFlagsShd;
    bool            rjoin = FALSE;
    DB_STATUS	    status = E_DB_OK;

    qen_hold = dsh->dsh_hold[node->node_qen.qen_sjoin.sjn_hfile];
    qen_shd = dsh->dsh_shd[dsh->dsh_qp_ptr->qp_sort_cnt +
                            node->node_qen.qen_sjoin.sjn_hfile];

    if (oj != NULL
      && (oj->oj_jntype == DB_RIGHT_JOIN
	  || oj->oj_jntype == DB_FULL_JOIN) )
	 rjoin = TRUE;

    for ( ; ; )		/* the usual procedure block to break out of */
    {
        status = qen_u2_position_tid(qen_hold, dsh, qen_shd);
        if(status != E_DB_OK) break;   /* to error */

        if( qen_hold->hold_buffer_status == HFILE8_FROM_INNER &&
	    qen_hold->unget_status != HFILE9_OCCUPIED )
        /* if row_buffer came from inner stream */   
        {
	    /* save current row_buffer contents in unget buffer */
	    status = qen_u7_to_unget(qen_hold, qen_shd, dsh);
	    if(status != E_DB_OK) break;   /* to error */
        }
        /* repositions set off hold_stream_eof */
        qen_status->node_u.node_join.node_hold_stream = 0;
        qen_status->node_access |= QEN_READ_NEXT_INNER;

        if ( rjoin )
        {
            ijFlagsHold = dsh->dsh_hold[oj->oj_ijFlagsFile];
            ijFlagsShd = dsh->dsh_shd[dsh->dsh_qp_ptr->qp_sort_cnt +
                            oj->oj_ijFlagsFile];

	    status = qen_u3_position_begin( ijFlagsHold, dsh, ijFlagsShd );
	    if(status != E_DB_OK) break;   /* to error */

	    /*
	    ** Setting this bit tells the flags file writer that the hold
	    ** file has been filled and so state changes need to be
	    ** re-written rather than written for the first time.
	    */
	    ijFlagsHold->hold_status2 |= HFILE_REPOSITIONED;
        }

	break;
    }	/* end of the usual and customary procedure block */

    return( status );
}

/*
** Name: clearHoldFiles - Scour out hold files
**
** Description:
**
**	This routine empties both hold files:  the hold file of inner
**	tuples and the hold file of inner join flags.
**
**
** Inputs:
**	node -
**	    Current qen node.
**	dsh - Thread's data segment header
**
** Outputs:
**	status words for the hold files are updated
**
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**
** History:
**      05-may-92 (rickh)
**          created
**	27-feb-04 (inkdo01)
**	    Changed dsh ptr to arrays of structs (SHD, HOLD, STATUS) to
**	    ptrs to arrays of ptrs.
[@history_template@]...
*/
static DB_STATUS
clearHoldFiles(
QEN_NODE	*node,
QEE_DSH		*dsh)
{
    QEN_STATUS	    *qen_status = dsh->dsh_xaddrs[node->qen_num]->qex_status;
    QEN_NODE	    *in_node = node->node_qen.qen_sjoin.sjn_inner;
    QEN_HOLD	    *qen_hold;
    QEN_HOLD	    *ijFlagsHold;
    QEN_OJINFO	    *oj = node->node_qen.qen_sjoin.sjn_oj;
    QEN_SHD         *qen_shd;
    QEN_SHD         *ijFlagsShd;
    bool            rjoin = FALSE;
    DB_STATUS	    status = E_DB_OK;

    qen_hold = dsh->dsh_hold[node->node_qen.qen_sjoin.sjn_hfile];
    qen_shd = dsh->dsh_shd[dsh->dsh_qp_ptr->qp_sort_cnt +
                            node->node_qen.qen_sjoin.sjn_hfile];

    if (oj != NULL
      && (oj->oj_jntype == DB_RIGHT_JOIN
	  || oj->oj_jntype == DB_FULL_JOIN) )
	 rjoin = TRUE;

    for ( ; ; )		/* the usual procedure block to break out of */
    {
        /* if inner child not sort */
        if( in_node->qen_type != QE_SORT && 
            qen_status->node_u.node_join.node_hold_stream != QEN5_HOLD_STREAM_EOF )
	{
	    status = qen_u6_clear_hold(qen_hold, dsh, qen_shd );
	    if(status != E_DB_OK) break;   /* to error */
	}
	qen_status->node_access &= ~QEN_RESCAN_MARKED;

        if ( rjoin )
        {
            ijFlagsHold = dsh->dsh_hold[oj->oj_ijFlagsFile];
            ijFlagsShd = dsh->dsh_shd[dsh->dsh_qp_ptr->qp_sort_cnt +
                            oj->oj_ijFlagsFile];
	    status = qen_u6_clear_hold( ijFlagsHold, dsh, ijFlagsShd );
	    if(status != E_DB_OK) break;   /* to error */

	    ijFlagsHold->hold_status2 &= ~HFILE_REPOSITIONED;
        }

	break;
    }	/* end of the usual and customary procedure block */

    return( status );
}
