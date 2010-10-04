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
**  Name: QENSEJOIN.C - subselect join processor
**
**/


/*{
** Name: QEN_SEJOIN	- subselect join processor for QEP nodes
**
** Description:
**	This join node is used when performing a subselect join. The
**	join algorithm is the basic nested-loop join with many special
**	handling for subselect semantics.  The outer node can be any 
**	QEN node including another SEJOIN. The inner node must always 
**	be a QEN_QP node.
**  
**	When an outer row fails to join with an inner row,  the semantic of
**	the SEJOIN may allows it to continue with the next outer tuple, or
**      return the unqualified (outer) row, or to stop processing.  Notice,
**	howver, that if the E_QE004C_NOT_ZEROONE_ROWS error is found, the
**	processing of the entire QEP must stop and the error must be reported
**      to the user. The SEJOIN semantics is specified by the following
**	execution specifications:
**
**	JUNCTION TYPES:
**	QESE1_CONT_AND
**	    The outer must be a qualified row AND must successfully join with
**	    an inner row (or satisfies the pre-join CX) before it can be 
**          returned as a qualified row.  If the current outer row is not
**	    qualified, CONTinue processing with the next outer until a
**	    qaulified row is found or there are no more rows.  Do not   
**	    return an unqualified row.
**
**	QESE2_CONT_OR
**	    The outer may either be a qualified row OR successfully join
**	    with an inner row before it can be returned as a qualified row.
**          If the outer is a qualified row, return it right away without
**	    further processing.  If the outer is not a qualified row but
**	    satisfies the pre-join CX or joins successfully with an inner
**	    row, also return it as a qualified row.  Otherwise, CONTinue
**	    with the next outer.
**
**	QESE3_RETURN_OR
**	    Like above, the outer may either be a qualified row OR 
**          successfully join with an inner row before it can be return as
**	    a qualified row.  However, if the row is not qualified, return
**	    it with an appropriate error code (NOT_ALL_ROWS or UNQUAL_ROW).
**
**	QESE4_RETURN_AND
**	    The outer must be a qualified row AND must successfully join with
**	    an inner row (or satisfies the pre-join CX) before it can be 
**          returned as a qualified row.  If the row is not qualified,
**	    return it with an appropriate error.
**
**	PRE-JOIN CX (sejn_oqual)
**	    If exists, this CX will be applied to every rows returned from
**	    the outer node.  A row that satisfies this CX will be returned
**	    as a qualified row to the upper node as if it has successfully
**	    joined with the inner.  The rows that does not qualify will 
**	    proceed to perform the actaul join operation with the inner.
**
**	JOIN CX (sejn_kqual)
**	    This is the CX that join the outer rows to the inner rows.  An
**	    outer row successfully joins to an inner row if this CX is
**	    satisfies.
**	    
**
**	SELECT TYPES:
**	QEN_CARD_ALL	
**	    An outer row must successfully join with all rows fetched from
**	    the inner node for the join to be successful (the outer row is
**	    qualified).  If the inner node is empty, the join is also 
**	    considered successful.  Notice that, for each outer row, the  
**	    inner node will always be scanned to the end.
**
**	QEN_CARD_ANY	
**	    An outer row must successfully join with an inner row.  The
**	    scanning of the inner node stops as soon as a join is found.
**
**	QEN_CARD_01R
**	    The inner node will be scanned and is expected to return exactly
**	    one row prior to applying the join CX If the inner node returns
**	    more than one row (E_QE004C_NOT_ZEROONE), the join fails.  In
**	    this case, the entire QEP must be  terminated.  If the inner 
**	    returns no row, the join also fails but may continue with the 
**	    next outer.  The join is successful only if the inner returns 
**	    exactly one row and that row successfully joins with the outer
**	    row.
**
**	The SEJOIN logic is described below:
**
**	- An outer row is fetched in the outer loop.  If the outer node is
**	  another SEJOIN node, the row returned may be an unqualified row
**        Notice that only an outer SEJOIN is capable of returning an
**	  unqualified row.  Rows returned from all other nodes are always
**	  qualified rows.
** 
**	  Special actions will be taken depending on the kind of the row
**	  and the junction type (sejn_junc).  See Table 1.
**	    
**	- The outer row,  qualified or not, is then tested with the
**	  special pre-join CX (sejn_oqual).  If passed, the row will be
**        returned as a qualified row to the upper node.  In this case,
**	  join operation is not needed.
**
**	  If the sejn_oqual does not exist or if the qualification falied,
**	  continue with the join operation.
**
**	- Prior to probing the inner node, the correlation value and the
**	  join key will be examined to make an inferal decision from the
**	  join result of the previous outer row.  See Table 2.  
**
**	  Notice that if the correlation values of the previous and of the
**	  current outer rows are not the same, the inferal decision must 
**	  NOT be made.  Furthermore, the contents of the hold file becomes
**	  obsolete.
**
**	- If the inner node must be probed, the inner rows are fetched from
**	  the hold file and/or the inner node as appropriate.  See Table 3
**	  for special handling when an end of the inner node is found.
**
**	- If the inner row is successfully fetched, it will be put into the
**	  hold file.  The join CX (sejn_kqual) is then exectued. If the result
**	  of the CX is true, the join is successful.  Table 4. describes the
**	  actions taken base on the result of the join.
**
**
**  Notes:
**
**  1)  When using the excb_cmp field of the ade_excb, the outer value is
**	compared to the inner (dt1, dt2). Old value is compared to the new
**	one (dt1, dt2).
**
**
**  2)  Note on hold_status values:	
**	    is used to keep the state of the hold file.
**
**  3)	Note on node_status:	
**	    is used to keep the status of the  previous join operation.
**
**
**  Decision Tables:
**-----------------------------------------------------------------------------
**  Table 1:  Decision to be made upon fetching an outer row:
**
**  Fetch Status    Junction Type	Action
**
**  NO_MORE_ROWS       X		Return NO_MORE_ROWS.
**					
**  NOT_ZERO_ONE       X                Return NOT_ZERO_ONE.
**
**  OK  	    CONT_OR		Return TRUE.
**                  RETURN_OR		Return TRUE.	       
**		    CONT_AND		Proceed to the next step.
**		    RETURN_AND		Proceed to the next step.
**
**  UNQUAL_ROW	    CONT_OR		Proceed to the next step.     
**		    RETURN_OR           Proceed to the next step.
**		    CONT_AND            Get the next outer.
**	            RETURN_AND          Return UNQUAL_ROW.
**-----------------------------------------------------------------------------
**  Table 2:  Decision to be made upon comparing the previous outer key
**	      to the current one:
**
**  Note 1: If the NEW:OLD is unknown, always scan the inner node.
**
**  Note 2: If there will be no inner rows qualified for the current outer,
**          the program state is the same as when NO_MORE_ROWS is found on 
**          the inner node and no inner rows have been qualified.
**
**  Previous   Join      NEW:OLD        Action
**  Result   Operator	
**
**    F         ?           <           Scan the inner
**    F		<           <           Scan the inner
**    F		=           <		Scan the inner
**    F		>           <		Scan the inner (in case null in key)
**
**    F         X           =           Will not qualified
**
**    F         ?           >           Scan the inner
**    F		<           >		Scan the inner (in case null in key)
**    F		=           >		Scan the inner
**    F		>           >           Scan the inner (in case null in key)
**
**    F         X           ?           Scan the inner
**
**    T         ?           <           Scan the inner
**    T		<           <		Scan the inner
**    T		=           <		Scan the inner
**    T		>           <		Scan the inner
**
**    T         X           =           Return OK
**
**    T         ?           >           Scan the inner
**    T		<           >		Scan the inner
**    T		=           >	        Scan the inner
**    T		>           >		Scan the inner (in case null in key)
**
**    T         X           ?           Scan the inner
**-----------------------------------------------------------------------------
**  Table 4:  Decision to be made upon NO_MORE_ROWS from the inner node.
**
**  NOTE 1: I_QUAL - TRUE if there is a previous inner row that is qualified
**          for the current outer.  This variable must be initialized to
**	    FALSE every time a new outer row is fetched.
**
**  NOTE 2: If select type is QEN_CARD_ALL, either there are no inner rows or all
**	    rows must be qualified for the scan loop to reach the end of the
**	    inner node. FALSE in I_QUAL means there are no rows and TRUE 
**	    means all rows qualify.
**
**  NOTE 3: If the select type is QEN_CARD_01R, either no rows or exactly
**          one row has been fetched from the inner node for the scan
**	    loop to reach the end.
**
**  I_QUAL  SEL_TYPE    JUNC_TYPE	Action
**	    
**    F	    ANY		CONT_OR		get the next outer
**	    		CONT_AND	get the next outer
**	    		RETURN_OR	return UNQUAL_ROW
**          		RETURN_AND	return UNQUAL_ROW
**
**    F	    ALL		X	        return OK
**
**    F	    ZERO_ONE	CONT_OR	        get the next outer
**			CONT_AND	get the next outer
**			RETURN_OR	return UNQUAL_ROW
**			RETURN_AND	return UNQUAL_ROW
**
**    T     ANY	        X		n/a
**
**    T	    ALL		X	        return OK
**
**    T	    ZERO_ONE	X	        return OK
**-----------------------------------------------------------------------------
**  Table 5:  Decision to be made upon qualifying the inner row.
**
**  KQUAL   SEL_TYPE    JUNC_TYPE       Action               
**
**    F     ANY         X               get the next inner.
**          ALL         CONT_OR		fails, get the next outer
**		        CONT_AND        fails, get the next outer
**			RETURN_OR	fails, return UNQUAL_ROW
**			RETURN_AND	fails, return UNQUAL_ROW
**          ZERO_ONE	X               get the next inner
**
**    T     ANY         X		succeeds, return OK
**          ALL         X               succeeds, get the next inner
**          ZERO_ONE    X               succeeds, get the next inner
**-----------------------------------------------------------------------------
**
** Inputs:
**      node                            the join node data structure
**	rcb				request block
**	    .qef_cb.qef_dsh		the data segment header
**	reset				TRUE if the node needs to be 
**					reinitialized.
**
** Outputs:
**      rcb
**	    .error.err_code		one of the following
**					E_QE0000_OK
**					E_QE0002_INTERNAL_ERROR
**					E_QE0015_NO_MORE_ROWS
**					E_QE00A5_END_OF_PARTITION
**					E_QE0019_NON_INTERNAL_FAIL
**					E_QE004B_NOT_ALL_ROWS
**					E_QE004C_NOT_ZEROONE_ROWS
**					E_QE004F_UNQUAL_ROW
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
** History:
**      29-jan-87 (daved)
**          written
**      27-jan-88 (puree)
**          Fixed subselect bug.  Rewrote most of it.
**	02-feb-88 (puree)
**	    Added reset flag to the function arguments.
**	11-mar-88 (puree)
**	    remove DSH_QUEUE structure. DMT_CB's can be linked together
**	    using the standard jupiter control block header.
**	03-aug-88 (puree)
**	    fix error in ult_check_macro call.
**	26-aug-88 (puree)
**	    There is no need to reset inner node current action (node_ahd_ptr).
**	    The inner node will reset it when the reset flag is set.
**	16-sep-88 (puree)
**	    Remove #defines that are already in header files.
**	04-jan-89 (puree)
**	    Rewrite it.
**	02-may-89 (puree)
**	    Because of the changes to ADF in return value and the way ADF 
**	    execute the compare CX, QENSEJOIN can no longer use the inference
**	    rule based on the new to old value of the outer key to 
**          automatically qualify or disqualify an outer row, except for the
**          equi-join cases.
**	28-sep-89 (eric)
**	    Incremented qen_status->node_rcount to keep track of how many
**	    rows this node returns. This will be used to fix the Sybase bug.
**	27-dec-90 (davebf)
**	    Modified to support shared hold files between SEJOINs which
**	    have the same inner subselect.
**	24-jan-91 (davebf)
**	    Modified for dsh_shd_cnt use
**	11-mar-92 (jhahn)
**	    Added support for QEF interrupts.
**	    Added call to CSswitch.
**	04-may-1992 (bryanp)
**	    Changed dmt_location from a DB_LOC_NAME to a DM_DATA.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      14-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
**	15-sep-93 (anitap)
**	    Added fix for bugs 45204/45687 in qen_sejoin(). This was a
**          SEjoin query with two ORed subselects so that the query plan had
**          two SEjoin nodes, one on top of the other. The lower SEjoin node
**          qualified the row and passed it up to the SEjoin node above it. But
**          the upper SEjoin node did not pass it up. This was because there
**          was no check in qen_sejoin() for subsequent times when the lower
**          node qualified the row. Added the check.
**      15-sep-93 (anitap)
**          Added fix for bugs 49336/50631/50905/50938/53335/54500. This was a 
**	    SEjoin query with an ORed subselect and constant qualification.
**	    Data dependent bug. When the prev outer did not join and the 
**	    current outer was same as prev outer, the outer row was not 
**	    passed up even though the constant qualification was satisfied.
**	15-sep-93 (anitap)
**          Added fix for bug 53658. This was a
**          SEJOIN query with ORed subselects and NOT EXISTS/EXISTS
**          clause. Current outer was not being joined, but returned
**          because out_status = OK. But the next outer row (with
**          same value for the outer key as the previous) was not
**          returned to the node above because the previous did not
**          set node_status = SUCCEEDED. Delayed check of out_status
**          = OK until after the join.
**	29-jun-95 (forky01)(wilri01)
**	    SEJOIN with ORed subselects were returning different # rows by
**	    reordering OR clauses.  Part of the 15-sep-93 fixes IF'd out the
**	    sejn_oqual for two of the four junction types.  Removed the IF
**	    test so all junctions are outer qualified.  Fixes 69359.
**	7-nov-95 (inkdo01)
**	    Changes to QP storage structures.
**	21-dec-1995 (inkdo01)
**	    Minor fix to 29-jun change to make it work more universally.
**	    Previous fix didn't work unless there was also a scalar
**	    comparsion ("column <compop> const/column") OR'ed in with the
**	    subselect compares. Fixes 73370.
**	5-dec-97 (inkdo01)
**	    Another minor fix to the handling of multiple OR'ed SEjoins.
**	    The short cut logic (for equal key values, using node_status)
**	    isn't always right in the presence of OR's.
**      13-Oct-1998 (islsa01)
**          This fix is for bug# 92452.
**          The query in this problem contained a "not in" subselect, resulting
**          in a query plan with a SE join node. SE joins "hold" files (under
**          the covers temporary tables) as part of their execution. If a commit
**          or rollback is executed between 2 executions of the SE join node as 
**          might happen if the query and the commit/rollback are all contained 
**          in a while loop in the procedure, an attempt is made to access the
**          hold file using control block no longer valid (because of the 
**          commit/rollback).
**          This flag QEQP_DBP_COMRBK is to flag query plans for database procedures
**          which contain a commit or rollback.  The SE join node processor then
**          tests the flag before attempting to access the hold file on a second or 
**          subsequent reexecution of the query, and takes evasive action.
**	31-Jan-2001 (thaju02)
**	    On a subsequent re-execution of the query, before attempting
**	    to create a new hold file, need to close/delete hold file 
**	    currently associated with dmt_cb and remove it from the 
**	    dsh_open_dmtcbs queue. Thus avoiding having multiple 
**	    instances of the same dmt_cb on the dsh_open_dmtcbs queue, 
**	    which results in a SIGSEGV in dmt_close during qeq_cleanup. 
**	    (B103845)
**	06-nov-03 (hayke02)
**	    Handle the case when we have a 'not in' subselect (SEL_ALL) that
**	    returns an excb_value (cx_value in ade_execute_cx()) of UNKNOWN,
**	    due to NULLs in the inner or outer, handled by the ADE_3CXI_SET_SKIP
**	    instruction. We should be continuing to the next inner rather than
**	    treating UNKNOWN the same as NOT_TRUE, and getting a new outer.
**	    This change fixes bug 111201, INGSRV 2577.
**	20-nov-03 (inkdo01)
**	    Add support for "open" call to allow parallel query initialization.
**	29-dec-03 (inkdo01)
**	    DSH is now parameter, "function" replaces "reset".
**	2-jan-04 (inkdo01)
**	    Added end-of-partition-grouping logic.
**	23-feb-04 (inkdo01)
**	    Changed qef_rcb->error to dsh->dsh_error for || thread safety.
**	27-feb-04 (inkdo01)
**	    Changed dsh ptr to arrays of structs (HOLD, STATUS) to
**	    ptrs to arrays of ptrs.
**	17-mar-04 (toumi01)
**	    Add support for "close" call to allow parallel query termination.
**	8-apr-04 (inkdo01)
**	    Added dsh parm to QEF_CHECK_FOR_INTERRUPT macro.
**	24-june-04 (inkdo01)
**	    Do open/close calls for inner subtree, too.
**	30-Jul-2004 (jenjo02)
**	    Use dsh_dmt_id rather than qef_dmt_id transaction context.
**	04-Nov-2004 (jenjo02)
**	    Only CSswitch on root DSH or if not OS-threaded.
**	18-jan-05 (inkdo01)
**	    Removed change of 06-nov-03. This was not a bug - just a 
**	    misunderstanding about the results of "not in" subselects 
**	    involving NULL values.
**	13-Dec-2005 (kschendel)
**	    Remaining inline QEN_ADF changed to pointers, fix here.
**	28-Dec-2005 (kschendel)
**	    Use DSH references whenever possible instead of QEF RCB
**	    references.  (This simplifies the code, eliminating many
**	    RCB references, and reflects the new primacy of the DSH
**	    thanks to the parallel query changes.)
**	13-Jan-2006 (kschendel)
**	    Revise CX evaluation calls.
**	16-Feb-2006 (kschendel)
**	    Return after close, don't keep going.
**    21-Mar-2005 (karbh01)
**       The code fix for B103845 closes the hold file and deletes the
**       hold file currently associated with dmt_cb and removes it from
**       dsh_open_dmtcbs queue without checking it is already closed. This
**       check is added.              
**	11-apr-06 (hayke02)
**	    Fix up the previous change for 113938/112183 so that we now test
**	    for a non-NULL dmt_cb->dmt_record_access_id AFTER we have assigned
**	    dmt_cb from qen_hold->hold_dmt_cb. This prevents a SEGV when
**	    attempting to read dmt_record_access_id from the uninitialised and
**	    unassigned dmt_cb. This change fixes bug 115976. Also, fix up the
**	    formatting of the previous change.
**	13-Feb-2007 (kschendel)
**	    Replace CSswitch with better cancel check.
**	4-Jun-2009 (kschendel) b122118
**	    Use slightly faster/more convenient dsh test for qe90, and
**	    remove pointless error returns from cost begin/end.  Minor
**	    streamlining.
**	27-Oct-2009 (kiria01) SIR 121883
**	    Scalar sub-query - Pick up cardinality infpor from generic QEN_NODE
**	13-Nov-2009 (kiria01) SIR 121883
**	    Updated symbol names and notes.
[@history_template@]...
**	10-Sep-2010 (kschendel) b124341
**	    Because of VLUP issues, outer key materialize and new vs old
**	    compare are all in the okmat now.  Split correlation value
**	    materialize into its own CX just to keep things straight.
*/

DB_STATUS
qen_sejoin(
QEN_NODE	*node,
QEF_RCB		*rcb,
QEE_DSH		*dsh,
i4		function)
{
    ADF_CB	    *adf_cb = dsh->dsh_adf_cb;
    QEF_CB	    *qef_cb = dsh->dsh_qefcb;
    DMR_CB	    *dmr_cb;
    DMT_CB	    *dmt_cb;
    QEN_NODE	    *out_node = node->node_qen.qen_sejoin.sejn_out;
    QEN_NODE	    *in_node = node->node_qen.qen_sejoin.sejn_inner;
    QEE_XADDRS	    *node_xaddrs = dsh->dsh_xaddrs[node->qen_num];
    QEN_STATUS	    *qen_status = node_xaddrs->qex_status;
    ADE_EXCB	    *ade_excb;
    QEN_HOLD	    *qen_hold;
    DB_STATUS	    status, out_status;
    bool	    reset = ((function & FUNC_RESET) != 0);
    bool	    out_reset = reset;
    bool	    corr_val_changed;
    bool	    i_qual;
    bool	    next_o_pass;
    bool	    i_fetched;
    i4	    	    new_to_old;
    i4		    sel_type;
    i4		    junc_type;
    i4	    val1;
    i4	    val2;
    TIMERSTAT	    timerstat;
    i4		    save_counter;

    /* Do open processing, if required. Only if this is the root node
    ** of the query tree do we continue executing the function. */
    if ((function & TOP_OPEN || function & MID_OPEN) && 
	!(qen_status->node_status_flags & QEN1_NODE_OPEN))
    {
	status = (*out_node->qen_func)(out_node, rcb, dsh, MID_OPEN);
	status = (*in_node->qen_func)(in_node, rcb, dsh, MID_OPEN);
	/* No inner node open - inner of SE join may depend on outer. */
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
	    status = (*out_node->qen_func)(out_node, rcb, dsh, FUNC_CLOSE);
	    status = (*in_node->qen_func)(in_node, rcb, dsh, FUNC_CLOSE);
	    qen_status->node_status_flags = 
		(qen_status->node_status_flags & ~QEN1_NODE_OPEN) | QEN8_NODE_CLOSED;
	}
	return(E_DB_OK);
    }


#ifdef xDEBUG
    (VOID) qe2_chk_qp(dsh);
#endif 


    /* Check for cancel, context switch if not MT */
    CScancelCheck(dsh->dsh_sid);
    if (QEF_CHECK_FOR_INTERRUPT(qef_cb, dsh) == E_DB_ERROR)
	return (E_DB_ERROR);

    dsh->dsh_error.err_code = E_QE0000_OK;
    junc_type = node->node_qen.qen_sejoin.sejn_junc;
    sel_type = node->qen_flags & QEN_CARD_MASK;
    qen_hold = dsh->dsh_hold[node->node_qen.qen_sejoin.sejn_hfile];
    dmr_cb = qen_hold->hold_dmr_cb;

    /* hold_reset_pending keeps reset flag until its sent to inner by
    ** this node or its hold-sharing partner */
    qen_hold->hold_reset_pending |= reset;  /* note the OR */

    /* If the trace point qe90 is turned on then gather cpu and dio stats */
    if (dsh->dsh_qp_stats)
    {
	qen_bcost_begin(dsh, &timerstat, qen_status);
    }

    /* If the node is to be reset, dump the hold file and reset the
    ** status of the inner/outer nodes.  Note that this is not done
    ** at this node if sejn_hcreate is FALSE */
    if (reset && node->node_qen.qen_sejoin.sejn_hcreate)
    {
	qen_hold->hold_inner_status = QEN0_INITIAL;
	if (qen_status->node_status != QEN0_INITIAL)
	{
	    qen_status->node_status = QEN0_INITIAL; /* force reinit */
            if (dsh->dsh_qp_ptr->qp_status & QEQP_DBP_COMRBK)
	    {
		/* B103845 */
	        dmt_cb = qen_hold->hold_dmt_cb;
		if (dmt_cb->dmt_record_access_id != NULL)
		{
		    status = qen_closeAndUnlink(dmt_cb, dsh);
		    if (status != E_DB_OK)
		    {
			dsh->dsh_error.err_code = dmt_cb->error.err_code;
			return(status);
		    }

		    dmt_cb->dmt_flags_mask = 0;
		    status = dmf_call(DMT_DELETE_TEMP, dmt_cb);
		    if (status)
		    {
			dsh->dsh_error.err_code = dmt_cb->error.err_code;
			return(status);
		    }
        	}
		dmt_cb->q_next = dmt_cb->q_prev = (PTR)NULL;
		qen_hold->hold_status = HFILE0_NOFILE;
	    }
            else if (qen_hold->hold_status > HFILE1_EMPTY)
	     {
		dmr_cb->dmr_flags_mask = 0;
		status = dmf_call(DMR_DUMP_DATA, dmr_cb);
		if (status != E_DB_OK)
		{
		    dsh->dsh_error.err_code = dmr_cb->error.err_code;

		    if (dsh->dsh_qp_stats)
		    {
			qen_ecost_end(dsh, &timerstat, qen_status);
		    }

		    return (status);
		}
		qen_hold->hold_status = HFILE1_EMPTY;
	    }
	}
    }


    for (;;)
    {
        /* 
	** Get an outer row. 
	*/
	save_counter = qen_hold->hold_reset_counter;	/* save current */
	out_status = (*out_node->qen_func)(out_node, rcb, dsh,
				(out_reset) ? FUNC_RESET : NO_FUNC);
	if (out_status != E_DB_OK && 
		dsh->dsh_error.err_code == E_QE00A5_END_OF_PARTITION &&
		!(node->qen_flags & QEN_PART_SEPARATE))
	{
	    /* End of rows from partitioning group. Start reading from
	    ** next group or return 00A5 up to caller. */
	    out_reset = TRUE;
	    continue;
	}
	out_reset = FALSE;
	i_qual = FALSE;	    /* no inner rows qualified for this outer yet */
	i_fetched = FALSE;  /* no inner rows fetched for this outer yet */
	next_o_pass = TRUE; /* allow next pass through the outer loop */

	/* if status on outer and not 4B/4F errors */
	if (out_status != E_DB_OK &&
	    dsh->dsh_error.err_code != E_QE004B_NOT_ALL_ROWS &&
	    dsh->dsh_error.err_code != E_QE004F_UNQUAL_ROW)
	{
	    status = out_status;
	    break;	    /* off to return the error */
	}

	/* If this is the first time this node is called, set up the hold 
	** file and a few other things */

	if (qen_status->node_status == QEN0_INITIAL)  
	{
	    /* Create the hold file if it has not been done either by   
	    ** this node or a shared hold node below */
	    if (qen_hold->hold_status == HFILE0_NOFILE)
	    {
		--dsh->dsh_shd_cnt;	/* show we don't use in mem hold */

		dmt_cb = qen_hold->hold_dmt_cb;
		dmt_cb->dmt_db_id = rcb->qef_db_id;
		dmt_cb->dmt_tran_id = dsh->dsh_dmt_id;
		dmt_cb->dmt_flags_mask = DMT_DBMS_REQUEST;
		MEmove(8, (PTR) "$default", (char) ' ', 
		    sizeof(DB_LOC_NAME),
		    (PTR) dmt_cb->dmt_location.data_address);
		dmt_cb->dmt_location.data_in_size = sizeof(DB_LOC_NAME);

		if ((status = dmf_call(DMT_CREATE_TEMP, dmt_cb)) != E_DB_OK)
		{
		    dsh->dsh_error.err_code = dmt_cb->error.err_code;
		    if (dmt_cb->error.err_code == E_DM0078_TABLE_EXISTS)
		    {
			dsh->dsh_error.err_code = E_QE0050_TEMP_TABLE_EXISTS;
			status = E_DB_SEVERE;
		    }
		    break;  /* off to return DMF error */
		}
		dmt_cb->dmt_flags_mask	= 0;
		if ((status = qen_openAndLink(dmt_cb, dsh)) != E_DB_OK)
		{
		    break;  /* off to return DMF error */
		}

		/* move the record access ids into the DMR_CBs */

		dmr_cb->dmr_access_id = dmt_cb->dmt_record_access_id;

		/* Indicate the hold file created */
		qen_hold->hold_status = HFILE1_EMPTY;
	    }
	    qen_status->node_status = QEN1_EXECUTED;

	    /* now materialize the first join key */
	    ade_excb = node_xaddrs->qex_okmat;
	    ade_excb->excb_seg = ADE_SINIT;
	    status = ade_execute_cx(adf_cb, ade_excb);
	    if (status != E_DB_OK)
	    {
		if ((status = qef_adf_error(&adf_cb->adf_errcb, 
		    status, qef_cb, &dsh->dsh_error)) != E_DB_OK)
		    break;
	    }
	    /* and any initial correlation values */
	    if (node_xaddrs->qex_cvmat != NULL)
	    {
		status = qen_execute_cx(dsh, node_xaddrs->qex_cvmat);
		if (status != E_DB_OK)
		    break;
	    }
	}
	else	/* For the subsequent times only */
	{
	    /* compare the correlation value of the new outer to previous one.*/
	    corr_val_changed = FALSE;
	    ade_excb = node_xaddrs->qex_ccompare;
	    if (ade_excb != NULL)
	    {
		status = qen_execute_cx(dsh, ade_excb);
		if (status != E_DB_OK)
		    break;
		corr_val_changed = (ade_excb->excb_cmp != ADE_1EQ2);
	    }

	    /* compare the join key of the new outer to previous one. */
	    new_to_old = ADE_1ISNULL;
	    ade_excb = node_xaddrs->qex_okmat;
	    ade_excb->excb_seg = ADE_SMAIN;
	    status = ade_execute_cx(adf_cb, ade_excb);
	    if (status != E_DB_OK)
	    {
		if ((status = qef_adf_error(&adf_cb->adf_errcb, 
		    status, qef_cb, &dsh->dsh_error)) != E_DB_OK)
		    break;
	    }
	    new_to_old = ade_excb->excb_cmp;

	    /* materialize the new join key if we have a new key or if
	    ** the correlation value changes */

	    if (new_to_old != NEW_EQ_OLD)
	    {
		/* excb still points at okmat */
		ade_excb->excb_seg = ADE_SINIT;
		status = ade_execute_cx(adf_cb, ade_excb);
		if (status != E_DB_OK)
		{
		    if ((status = qef_adf_error(&adf_cb->adf_errcb, 
			status, qef_cb, &dsh->dsh_error)) != E_DB_OK)
			break;
		}
	    }
	    if (corr_val_changed == TRUE)
	    {
		status = qen_execute_cx(dsh, node_xaddrs->qex_cvmat);
		if (status != E_DB_OK)
		    break;
	    }


	    /* added as a fix for bugs 45204/45687 */

            if (new_to_old == NEW_EQ_OLD && corr_val_changed == FALSE)
            {
               if (out_status == E_DB_OK)
               {
                  if (junc_type == QESE2_CONT_OR || 
			junc_type == QESE3_RETURN_OR)
                     break;           /* off to return OK */
                  /* else proceed to the next step */
               }
               else
               {
               /* An error is found fetching an outer row
               ** If we're here it must be E_QE004B or E_QE004F */

                  if (junc_type == QESE1_CONT_AND)
                     continue;   /* to get the next outer */
                  else if (junc_type == QESE4_RETURN_AND)
                     break;      /* off to return the unqual row */
                 /* else proceed to the next step */
               }
            }
	    
	    /* if the correlation value changes, dump the hold file and 
	    ** reset the inner node unless this hold file is shared with
	    ** a subtree sejoin and that node has dumped/reset, incrementing
	    ** hold_reset_counter */
	    if (corr_val_changed == TRUE)      
	    {
	    	if(save_counter == qen_hold->hold_reset_counter)
		{
		    if (qen_hold->hold_status > HFILE1_EMPTY)  
		    {
			dmr_cb->dmr_flags_mask = 0;
			if (dmf_call(DMR_DUMP_DATA, dmr_cb))
			{
			    dsh->dsh_error.err_code = dmr_cb->error.err_code;
			    break;  /* off to return DMF error */
			}
			qen_hold->hold_status = HFILE1_EMPTY;
		    }

		    /* reset the inner node */

		    /* increment counter to tell hold-sharing partner (if any)
		    ** that reset done for this corelation change */
		    ++qen_hold->hold_reset_counter;

		    /* force inner reset on next inner call by either partner */
		    qen_hold->hold_reset_pending = TRUE; 

		    qen_hold->hold_inner_status = QEN0_INITIAL; /* cancel eof */
		}
	    }
	    else 
	    /* The corr values are the same, make use of the results of the
	    ** previous join operation and the new to old key comparison.
	    ** See Table 2. in the module header comment.
	    */
	    {
		if (qen_status->node_status == QEN7_FAILED)
		{
		    if (new_to_old == NEW_EQ_OLD)
		    { 
			/* The current outer will find no qualified inner.
			** Determine what to do next based on the junc_type.
			*/
			/* Fix for bugs 49336/50631/50905/50938/53335/54500.
			** For QESE1_CONT_AND and QESE2_CONT_OR junc_types,
			** we return the qualified outer row if it satisfies
			** the pre-join CX (sejn_oqual) even if it does not
			** join with the inner.
			** Qualify the outer row wih the outer qualification
			** (sejn_oqual). If the outer row satisfies this CX,
			** no further processing is required. The row will be
			** returned to upper node immediately. This
			** qualification is one disjunct of which the
			** subselect join is the other disjunct.
			*/
			ade_excb = node_xaddrs->qex_prequal;
			if (ade_excb != NULL)
			{
			    status = qen_execute_cx(dsh, ade_excb);
			    if (status != E_DB_OK)
				break;
			    if (ade_excb->excb_value == ADE_TRUE)
			    {
				/* set node_status so that if next outer
				** doesn't get control break, it won't try to
				** act on QEN6/QEN7 */
				qen_status->node_status = QEN1_EXECUTED;
				status = E_DB_OK;
				break;      /* return OK */
			    }
			}
                        if (junc_type == QESE4_RETURN_AND)
                        {
                            if (sel_type == QEN_CARD_ALL)
                                dsh->dsh_error.err_code = E_QE004B_NOT_ALL_ROWS;
                            else
				dsh->dsh_error.err_code = E_QE004F_UNQUAL_ROW;
			    status = E_DB_WARN;
			    break;	/* off to return UNQUAL row */
                        }

			else if (junc_type == QESE1_CONT_AND)
			 continue;	/* to get the next outer */
			/* OR's that get this far will continue into the 
			** inner loop inspite of QEN7, because it isn't
			** a reliable measure for OR's. If all previous
			** outer's arrived already qualified, node_status
			** will still be QEN7, because inner logic has yet
			** to be executed. This fixes 87547. */
		    }
		}
		else if (qen_status->node_status == QEN6_SUCCEEDED)
		{
		    if (new_to_old == NEW_EQ_OLD)
		    {
			/* The current outer will also successfully join 
			** with the inner, so just return it now. 
			*/
			status = E_DB_OK;
			break;
		    }
		}
		/* Else proceed to scan the inner node */
	    }
	}

	/* Common path for the first or the subsequent times */


	/*
	** Special action taken upon fetching an outer row.  See Table 1.
	** in the module header comment.
	*/

        /* Moved check for out_status = OK to the section when the
        ** join fails. Fix for bug 53658. We
        ** should let the outer join with the inner so that
        ** node_status is set to SUCCEEDED if the join was successful
        ** and the next outer key if same as the previou will be
        ** considered as joined and the corresponding row returned
        ** to the node above.
        */

	if (out_status != E_DB_OK)
	{
	    /* An error is found fetching an outer row   
	    ** If we're here, it must be E_QE004B or E_QE004F */

	    if (junc_type == QESE1_CONT_AND)
		continue;   /* to get the next outer */
	    else if (junc_type == QESE4_RETURN_AND)
		break;	    /* off to return the unqual row */
	    /* else proceed to the next step */
	}

	/*
	**  Qualify the outer row with the outer qualification (sejn_oqual).
	**  If the outer row satisfies this CX, no further processing is
	**  required.  The row will be returned to upper node immediately.
	**  This qualification is one disjunct of which the sub-select join
	**  is the other disjunct.
	*/
	ade_excb = node_xaddrs->qex_prequal;
	if (ade_excb)
	{
	    status = qen_execute_cx(dsh, ade_excb);
	    if (status != E_DB_OK)
	    {
		break;  /* off to return ADF error */
	    }
	    if (ade_excb->excb_value == ADE_TRUE)
	    {
		/* set node_status so that if next outer doesn't get
		** control break, it won't try to act on QEN6/QEN7 */
		qen_status->node_status = QEN1_EXECUTED;
		status = E_DB_OK;
		break;	    /* return OK */
	    }
	}


	/* Reposition the hold file if it is not empty */
	if (qen_hold->hold_status > HFILE1_EMPTY)
	{
	    dmr_cb->dmr_position_type = DMR_ALL;
	    dmr_cb->dmr_flags_mask = 0;
	    status = dmf_call(DMR_POSITION, dmr_cb);
	    if (status != E_DB_OK)
	    {
		if (dmr_cb->error.err_code == E_DM0055_NONEXT)
		{
		    dsh->dsh_error.err_code = E_QE0002_INTERNAL_ERROR;
		    status = E_DB_FATAL;
		}
		else
		{
		    dsh->dsh_error.err_code = dmr_cb->error.err_code;
		}
		break;	    /* to return DMF error */
	    }
	    qen_hold->hold_status = HFILE3_POSITIONED;
	}

	/* The following is the inner loop to fetch and process the
	** inner rows. 
	** If there are rows in the hold file, get them first.
	*/
	for (;;)
	{				
	    /* If the hold file is full and positioned, then get the
	    ** inner tuples from there. Otherwise, we will need to 
	    ** get them from the inner QEN node.
	    */
	    if (qen_hold->hold_status == HFILE3_POSITIONED)
	    {
		dmr_cb->dmr_flags_mask = DMR_NEXT;
		if ((status = dmf_call(DMR_GET, dmr_cb)) != E_DB_OK)
		{
		    if (dmr_cb->error.err_code == E_DM0055_NONEXT)
		    {
			qen_hold->hold_status = HFILE2_AT_EOF;
			continue;   /* to get rows from the inner node */
		    }
		    else
		    {
			dsh->dsh_error.err_code = dmr_cb->error.err_code;
			break;	    /* to return DMF error */
		    }
		}
	    }
	    else
	    {
		/* No rows can be obtained from the hold file, have to get 
		** rows from the inner node now.
		*/
		if (qen_hold->hold_inner_status == QEN11_INNER_ENDS)
		{
		    dsh->dsh_error.err_code = E_QE0015_NO_MORE_ROWS;
		    status = E_DB_WARN;
		}
		else
		{
		    status = (*in_node->qen_func) (in_node, rcb, dsh,
			(qen_hold->hold_reset_pending) ? FUNC_RESET : NO_FUNC);
		    qen_hold->hold_reset_pending = FALSE;
		}
		

		if (status != E_DB_OK)
		{
		    if (dsh->dsh_error.err_code == E_QE0015_NO_MORE_ROWS)
		    {
			/* Mark inner ends */
			qen_hold->hold_inner_status = QEN11_INNER_ENDS;

			/* Special handling when the inner ends. See Table 3. */
			if (i_qual == TRUE || sel_type == QEN_CARD_ALL)
			{
			    qen_status->node_status = QEN6_SUCCEEDED;
			    status = E_DB_OK;
			    next_o_pass = FALSE;
			    break;	/* to return successful join */
			}
			else
			{
                            /*
                            ** Special actio taken upon fetching an outer row
                            ** See Table 1 in the module header
                            ** comment.
                            ** Added as fix for bug 53658.
                            ** This is part of the fix.
                            ** The rest is in the section when the
                            ** join fails. Since we delayed out_status
                            ** check till after the join, we need to
                            ** include an out_staus check for the
                            ** cases where the inner ends, i_qual is
                            ** FALSE and out_status = OK. We need to
                            ** return the outer row in this case.
                            */

			    if (out_status == E_DB_OK)
                            {
                               if (junc_type == QESE2_CONT_OR
                                  || junc_type == QESE3_RETURN_OR)
                               {
                                  qen_status->node_status =
                                        QEN7_FAILED;
                                  status = E_DB_OK;
                                  next_o_pass = FALSE;
                                  break; /* off to return OK */
                              }
                            }

			    if (junc_type == QESE1_CONT_AND ||
				junc_type == QESE2_CONT_OR)
			    {
				qen_status->node_status = QEN7_FAILED;
				status = E_DB_OK;
				break;  /* to get the new outer */
			    }
			    else
			    {
				qen_status->node_status = QEN7_FAILED;
				dsh->dsh_error.err_code = E_QE004F_UNQUAL_ROW;
				status = E_DB_WARN;
				break;	/* to return FALSE row */
			    }
			}
		    }		
		    else
		    {
			status = E_DB_ERROR;
			break;	    /* to return error */
		    }
		}

		/* Materialize the inner tuple */

		status = qen_execute_cx(dsh, node_xaddrs->qex_itmat);
		if (status != E_DB_OK)
		    break;	    /* to return ADF error */

		/* put tuple in hold file */

		dmr_cb->dmr_flags_mask = 0;
		if ((status = dmf_call(DMR_PUT, dmr_cb)) != E_DB_OK)
		{
		    dsh->dsh_error.err_code = dmr_cb->error.err_code;
		    break;	    /* to return DMF error */
		}

		/* The hold file is now not empty.  It's also at EOF */
		qen_hold->hold_status = HFILE2_AT_EOF;
	    }

	    /* A row is fetched from the inner, a special check for 
	    ** QEN_CARD_01R */
	    if (sel_type == QEN_CARD_01R && i_fetched == TRUE)
	    {
		qen_status->node_status = QEN7_FAILED;
		dsh->dsh_error.err_code = E_QE004C_NOT_ZEROONE_ROWS;
		status = E_DB_ERROR;
		break;	    /* to return NOT_ZERO_ONE error */
	    }

	    i_fetched = TRUE;	/* indicate rows returned from inner node */

	    /* Join the outer row with the inner row on the join key. */
	    ade_excb = node_xaddrs->qex_joinkey;
	    status = qen_execute_cx(dsh, ade_excb);
	    if (status != E_DB_OK)
		break;	    /* to return ADF error */

	    /*
	    ** Special process for the result of the join expression.  See
	    ** Table 4.
	    */
	    if (ade_excb == NULL || ade_excb->excb_value == ADE_TRUE)  /* succeeded */
	    {
		i_qual = TRUE;
		if (sel_type == QEN_CARD_ALL || sel_type == QEN_CARD_01R)
		{
		    continue;	/* to examine the next inner */
		}
		else
		{
		    qen_status->node_status = QEN6_SUCCEEDED;
		    status = E_DB_OK;
		    next_o_pass = FALSE;
		    break;	/* to return successful join */
		}
	    }
	    else    /* the join expression failed */
	    {
		i_qual = FALSE;

		/*
                ** Special action taken upon fetching an outer row.
                ** See Table 1 in the module header comment.
                ** Added as fix for bug 53658.
                ** Previously, we did not check if the outer joined with
                ** inner before we returned the row. If out_status = OK,
                ** we returned the row. This did not set node_status to
                ** SUCCEEDED. If the next outer key had the same value
                ** as the previous, we would not pass up the next outer
                ** row as we would see that the previous had not
                ** set node_status to SUCCEEDED (and thus did not join
                ** with the inner).
                **        Delayed the out_satus check till after the
                ** outer row is actually joined with inner. If it does
                ** not join with inner, then we check for the
                ** out_status and return the row if OK.
                */

		if (out_status == E_DB_OK)
                {
                   if (junc_type == QESE2_CONT_OR
                        || junc_type == QESE3_RETURN_OR)                 
		   {
                      next_o_pass = FALSE;
                      qen_status->node_status = QEN7_FAILED;
                      break; /* off to return OK */
                   }
                }

		if (sel_type == QEN_CARD_ALL)
		{
		    qen_status->node_status = QEN7_FAILED;
		    if (junc_type == QESE1_CONT_AND ||
			junc_type == QESE2_CONT_OR)
		    {
			status = E_DB_OK;
			break;	/* to get the new outer */
		    }
		    else
		    {
			dsh->dsh_error.err_code = E_QE004B_NOT_ALL_ROWS;
			status = E_DB_ERROR;
			break;	/* to return FALSE row */
		    }
		}
		else
		{
		    continue;	/* to examine the next inner */
		}
	    }
	}   

	if (status != E_DB_OK || next_o_pass != TRUE)
	    break;	/* break off the outer loop in case of error. */
			/* Otherwise, we will continue on the the next pass */
			/* of the outer loop. */
    }

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
	    (void) qen_print_row(node, rcb, dsh);
	}
    }

#ifdef xDEBUG
    (VOID) qe2_chk_qp(dsh);
#endif 

    if (dsh->dsh_qp_stats)
    {
	qen_ecost_end(dsh, &timerstat, qen_status);
    }

    return (status);
}
