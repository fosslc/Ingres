/*
** Copyright (c) 2010 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <st.h>
#include    <cv.h>
#include    <ex.h>
#include    <sr.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <me.h>
#include    <ulf.h>
#include    <cut.h>
#include    <ulm.h>
#include    <adf.h>
#include    <ade.h>
#include    <qsf.h>
#include    <dmf.h>
#include    <dmtcb.h>
#include    <dmrcb.h>
#include    <dmucb.h>
#include    <qefmain.h>
#include    <qefkon.h>
#include    <qefrcb.h>
#include    <qefcb.h>
#include    <qefnode.h>
#include    <qefqeu.h>
#include    <psfparse.h>	
#include    <qefact.h>
#include    <qefqp.h>
#include    <qefdsh.h>
#include    <qefexch.h>
#include    <qefscb.h>
#include    <qeuqcb.h>
#include    <rqf.h>
#include    <dudbms.h>
#include    <tpf.h>
#include    <qefqry.h>
#include    <qefcat.h>
#include    <sxf.h>
#include    <qefprotos.h>

/**
**
**  Name: QEEHASH.C - Hash Join/Aggregation Reservation
**
**  Description:
**
**	This file supplies routines for doing pre-execution reservation
**	of hash joins and hash aggregations.  Basically, what we do here
**	is decide which joins and aggregations get what share of
**	memory.  There is only a finite amount of memory available,
**	defined by the session limit qef_hash_mem, so for complex
**	queries this can be a difficult optimization problem.
**
**	A naive implementation would simply count up all the hash
**	operations (joins and aggs), and give each one 1/N'th of
**	the limit.  Indeed, this was how things worked for a long
**	time.  The desire to do better is driven mostly by the need
**	to run very many truly concurrent sessions, all running
**	queries with several joins or aggs, against massive tables.
**
**	Since the algorithms involved are intricate, and will probably
**	change a lot as improvements are developed, I've split this
**	off from qee.c which is big enough as it is.
**
**	There are a few driving reasons for not simply giving each
**	hash op 1/N'th of the session limit (N = total hash ops):
**
**	- Hash ops in separate actions do not run concurrently.
**
**	- A hash join reads all of its build side before reading from
**	  its probe side (see below).  Therefore the two input sub-
**	  plans do not run concurrently, and can share memory.
**
**	- Hash joins (and to a lesser degree, hash aggs) are sized
**	  according to the optimizer build input estimate.  Puny
**	  hash joins don't need the entire 1/N slice, and it would be
**	  nice to allow large joins to reserve the excess.
**
**	(Note: hash joins can pre-probe the probe side, but they take
**	care to only do this when the probe side is a bare orig and not
**	a more complex sub-plan.)
**
**	This isn't entirely about giving hash ops as much memory as
**	possible.  Hash join performance isn't linear in memory anyway,
**	it's more of a step function.  Adding more memory to a spilling
**	hash join doesn't make it faster, until you can add enough to
**	make it NOT spill.  What's just as important is that by being
**	careful with reservations, we can run a given workload of a
**	given complexity with a lower qef_hash_mem session limit,
**	which in turn allows more simultaneous users to run in a
**	given sorthash pool size (and ultimately in a given amount
**	of system RAM).
**
**	As a side note, sorting and sort-hold reservation for
**	merge joins would benefit from a similar treatment, although
**	probably not as much since these operations tend to slough
**	off onto DMF at the faintest sign of memory pressure.
**	Since the trend is to avoid merge joins with their attendant
**	sorts anyway, I've made no attempt to go beyond simple 1/N
**	slicing for sort-hold's.  They are not mentioned further here.
**
**	External entry points are:
**
**	qee_hash_reserve - Perform hashop reservations under one action
**
**  History:
**	26-Sep-2007 (kschendel) SIR 122512
**	    Extract qee hash reservation into its own file.
**	15-Oct-2009 (kschendel) SIR 122512
**	    VMS needs cs.h or it doesn't compile.
**	14-May-2010 (kschendel) b123565
**	    qen-nthreads is now zero except under an exchange, make
**	    the necessary minor adjustments here.
*/

/* Reservation state information structure.
** This info is passed up and down the query plan, and it's easier
** to keep the state in one struct rather than passing a bunch of
** separate variables.
*/
typedef struct _HASHRES_STATE
{
    /* Action-or-node, typically parent or top-level.  Only one of
    ** the two will be filled in, with the other NULL.  It could be
    ** a union but no big difference.
    */
    QEF_AHD	*act;		/* Top-level or parent action header */
    QEN_NODE	*node;		/* Top-level or parent plan node */

    /* Concurrency tracking, for both counts and sizes.  Count and size
    ** are mutually exclusive in the sense that if a reserved-size is
    ** known, we don't have to count it.  If a reserved-size is not known,
    ** we have to count it to slice the limit N ways.
    ** Also, we have to track concurrency prior to returning rows separately
    ** from concurrency upon delivering rows, since that is the whole
    ** reason for this business.  A hash join is empty before it receives
    ** any rows from the build side;  and its build input vanishes before
    ** the join delivers rows to its caller.
    ** The "building" size/number reflect the state of things before any
    ** rows are sent to the parent (or received from the child).
    ** The "running" size/number reflect the state of things while rows
    ** are being sent to the parent (or are coming from the child).
    */
    SIZE_TYPE	building_size;	/* Concurrent, reserved total bytes */
    SIZE_TYPE	running_size;	/* same, delivering rows */
    i4		building_num;	/* Concurrent, non-reserved total hashops */
    i4		running_num;	/* concurrency once delivering rows */
} HASHRES_STATE;

/* Completion states returned by qee_hash_doinit. */

typedef enum completion_state_enum {
    DOINIT_DID_SOME = 0,	/* We reserved some hash ops but not all */
    DOINIT_DID_ALL = 1,		/* All hash ops were reserved */
    DOINIT_DID_NONE = 2		/* None were reserved, none fit */
} COMPLETION_STATE;


static void qee_hash_add(
	QEE_DSH *dsh,
	QEF_AHD *act,
	QEN_NODE *node,
	HASHRES_STATE *top_state,
	HASHRES_STATE *adder_state,
	bool stop_at_sorts);

static void qee_hash_concurrency(
	QEE_DSH *dsh,
	QEF_AHD *act,
	QEN_NODE *node,
	HASHRES_STATE *top_state,
	HASHRES_STATE *parent_state);

static void qee_hash_findexch(
	QEN_NODE *node,
	HASHRES_STATE **cur_statep);

static DB_STATUS qee_hash_doinit(
	QEF_RCB *rcb,
	QEE_DSH *dsh,
	QEF_AHD *act,
	SIZE_TYPE session_limit,
	COMPLETION_STATE *completion_state,
	bool always_reserve,
	ULM_RCB *ulm);


/* Table of primes used to select number of hash-join partitions.
** Primes are used to help reduce or eliminate interactions with
** hash table-partitioning, and should give good distributions.
** Don't need every prime here...
*/

static READONLY i4 primes[] = {
        7, 11, 13, 17, 19, 23, 29, 31,
        37, 43, 47, 53, 59, 67, 73, 79,
	83, 89, 97, 107, 113, 127, 137, 149,
	157, 167, 173, 181, 191, 199, 211, 223,
	233, 241, 251, 263, 277, 283, 293, 307,
	313, 331, 347, 359, 367, 373, 383, 397,
	419, 433, 443, 457, 467, 479, 491, 509,
	521, 541, 563, 587, 613, 641, 661, 683,
	709, 733, 757, 787, 809, 839, 859, 887,
	911, 937, 967, 991, 1009, 1031, 1061, 1109,
        0};

GLOBALREF QEF_S_CB	*Qef_s_cb;


/*
** Name: qee_hash_reserve -- Hash join/aggregation reservation driver
**
** Description:
**
**	This routine runs the process of reserving memory for all of
**	the hash operations (joins/aggs) that fall underneath one
**	"main" query plan action.  A main action in this case is one
**	that is not a sub-action of another (ie underneath a QP node).
**	Main actions are executed serially and run to completion before
**	moving on to the next action.  (There is a minor exception here
**	for DBproc FOR-loops, but we can deal with that with some
**	fiddling.)
**
**	The overall algorithm looks like this:
**
**	1. Allocate an array of HASHRES_STATE entries, one for the main
**	action and one for each concurrently executable parallel child
**	thread within that action.  Obviously the latter only exist when
**	parallel query is turned on.  Basically we have one entry for each
**	sub-plan within the action that can potentially execute concurrently
**	with the other sub-plans.
**
**	2. For a parallel plan, walk the action QP and fill in the header
**	array with the top of the parallel sub-plans.  (Meaning, the node
**	under an EXCH for a 1:1 or 1:n pipelined EXCH, or the individual
**	action headers for the actions under a parallel-union EXCH.)
**
**	3. Perform the concurrency calculation for each sub-plan.  At
**	this stage, we're only computing concurrent counts, as all the
**	known-reserved sizes are zero.
**
**	4. For a parallel plan, add each sub-plan's counts to all the
**	other sub-plans.  This allows for the plan parallelism.  We
**	assume that all threads run concurrently with no dependencies;
**	this is an unnecessarily pessimistic assumption, but I don't
**	care to get into child thread flow analysis at this time.
**
**	5. Now we have a worst-case concurrency count for each hash operation.
**	Do hash-join and hash-agg reservation for the entire action, but ONLY
**	complete the reservation for hash-ops that can be planned to fit
**	within their 1/N slice.  ("Fit" using the optimizer estimate, and
**	1/N now referring to concurrency at that particular hashop.)
**
**	6. Repeat steps 3 and 4, which will this time carry along the
**	concurrent sizes as well as counts.  When complete, the remaining
**	un-reserved hash ops will be assigned a new concurrency number
**	reflecting only other un-reserved hashops, as well as a
**	concurrent-size reflecting the "small" reserved hashops.
**
**	7. Do another reservation pass over the action.  This time we
**	will be reserving (session_limit - concurrent size) / N for
**	the remaining hash ops, for a new N reflecting only other unreserved
**	hash ops.  This is an "always reserve" pass, so any remaining
**	hash ops get reserved whether they fit in the slice or not.
**
**	This is a lot of whirling around!  but it does result in a much
**	more accurate picture of completed vs concurrently-running vs
**	not-yet-started hash operations at each point.  It also allows
**	small operations to reserve their moieties first, potentially
**	leaving more for the big grinders.
**
**	It should be noted that plans with exactly one hash join or one
**	hash aggregation for the entire plan, skip all of this and call
**	the low level reserver directly.
**
**	Caution: not all of the DSH stuff (like dsh_qefcb etc) is set
**	up at this point.  This is called during DSH initialization.
**
** Inputs:
**	rcb			QEF_RCB query request block pointer
**	dsh			Query's data segment header
**	act			Action header, we'll reserve all hash ops
**				underneath this action
**	ulm			A ULM request block for the DSH stream for
**				use in allocating runtime control blocks
**	temp_ulm		A ULM request block for temporary memory
**				allocation, if needed.  (This stream belongs
**				to qee-cract and vanishes when it is done.)
**
** Outputs:
**	Returns E_DB_OK or error code;  if error, rcb->error.err_code
**	is supposed to be set.
**
** Side Effects:
**	Hash join / hash agg memory reservations decided upon and
**	initial control structures allocated from DSH stream.
**
** History:
**	26-Sep-2007 (kschendel)
**	    Written.  We're looking at some 300 truly simultaneous
**	    queries per server and the old way was too wasteful and
**	    unpredictable.
**
*/

/* Most plans are serial or only have a few parallel thread starting
** points.  Keep header state on the stack unless there is a lot
** of it.
*/
#define HSTATE_HEADER_MAX 8

DB_STATUS
qee_hash_reserve(QEF_RCB *rcb, QEE_DSH *dsh,
	QEF_AHD *act, ULM_RCB *ulm, ULM_RCB *temp_ulm)
{
    COMPLETION_STATE completion_status;	/* DOINIT_DID_xxx status */
    DB_STATUS status;
    HASHRES_STATE *top_state;
    HASHRES_STATE *header_state;	/* Usually &header_array[0] */
    HASHRES_STATE header_array[HSTATE_HEADER_MAX];
    i4 num_headers;			/* Entries in header_state */
    QEF_QP_CB *qp;			/* Overall plan header */
    SIZE_TYPE session_limit;		/* overall reservation limit */

    qp = dsh->dsh_qp_ptr;
    session_limit = rcb->qef_cb->qef_h_max;

    /* We might be in a FOR-loop.  Or, this might be a GET that is
    ** driving the FOR-loop.  All of the driving queries run at the
    ** same time, so we have to assume the deepest possible nesting.
    ** An inside-the-loop query has the exact nesting level available.
    **
    ** Unfortunately opc hasn't found it convenient to compute and store
    ** the max nesting for any given FOR-loop.  But then, people don't
    ** usually write fancy nested FOR-loops in db procedures.
    **
    ** In the normal non-FOR case, act_for_depth is zero.
    */
    if (act->qhd_obj.qhd_qep.ahd_qepflag & AHD_FORGET)
	session_limit = session_limit / qp->qp_for_depth;
    else
	session_limit = session_limit / (act->qhd_obj.qhd_qep.ahd_for_depth + 1);

    header_state = &header_array[0];
    /* Note that this is a QP-wide count, was too inconvenient to get just
    ** a per-action count.  Usually aren't that many exch's around anyway.
    */
    num_headers = qp->qp_pqhead_cnt + 1;
    if (num_headers > HSTATE_HEADER_MAX)
    {
	/* Sigh, giantly complex parallel query, allocate headers from the
	** temp memory stream (caller will close stream).
	*/
	temp_ulm->ulm_psize = num_headers * sizeof(HASHRES_STATE);
	status = qec_malloc(temp_ulm);
	if (status != E_DB_OK)
	{
	    rcb->error.err_code = temp_ulm->ulm_error.err_code;
	    return (status);
	}
	header_state = (HASHRES_STATE *) temp_ulm->ulm_pptr;
    }
    MEfill(num_headers * sizeof(HASHRES_STATE), 0, (PTR) header_state);

    /* Zero'th entry in header array is the main action itself */
    header_state[0].act = act;
    header_state[0].node = NULL;
    if (num_headers > 1)
    {
	/* For parallel plan, walk the QP under this action and find all
	** the child-thread starting points (under EXCH's).  Record these
	** in the header_state array.  Some of the exch's may turn out to
	** not have anything hashly-interesting underneath, hence the
	** recalc of num-headers.
	*/
	top_state = &header_state[1];
	qee_hash_findexch(act->qhd_obj.qhd_qep.ahd_qep, &top_state);
	num_headers = top_state - &header_state[0];
    }

    /* Now walk each sub-plan to figure out the concurrency level of
    ** each one in isolation.  At this stage we're only carrying around
    ** counts, all the sizes will be zero.
    */
    for (top_state = &header_state[0];
	 top_state <= &header_state[num_headers-1];
	 ++ top_state)
    {
	HASHRES_STATE cur_state;

	cur_state.act = NULL; cur_state.node = NULL;
	cur_state.building_num = 0;
	cur_state.building_size = 0;
	cur_state.running_num = 0;
	cur_state.running_size = 0;
	qee_hash_concurrency(dsh, top_state->act, top_state->node,
		top_state, &cur_state);
    }

    /* If there was more than one sub-plan, we have to add 'em all
    ** together, since they all run independently.  No way to know what
    ** point any particular thread is in.  We're adding worst-cases here.
    */
    if (num_headers > 1)
    {
	HASHRES_STATE cur_state;
	i4 tot_num;
	SIZE_TYPE tot_size;

	tot_num = 0; tot_size = 0;
	for (top_state = &header_state[num_headers-1];
	     top_state >= &header_state[0];
	     -- top_state)
	{
	    tot_num += top_state->running_num;
	    tot_size += top_state->running_size;
	}
	for (top_state = &header_state[num_headers-1];
	     top_state >= &header_state[0];
	     -- top_state)
	{
	    cur_state.running_num = tot_num - top_state->running_num;
	    cur_state.running_size = tot_size - top_state->running_size;
	    qee_hash_add(dsh, top_state->act, top_state->node,
			top_state, &cur_state, FALSE);
	}
    }

    /* Run a trial reservation across the entire action.  This will
    ** reserve any hash joins/aggs that fit into their proposed slice.
    */
    completion_status = DOINIT_DID_NONE;
    status = qee_hash_doinit(rcb, dsh, act,
			session_limit, &completion_status, FALSE, ulm);
    if (status != E_DB_OK)
	return (status);

    /* If that trial got everything, we're done! */
    if (completion_status == DOINIT_DID_ALL)
	return (E_DB_OK);

    if (completion_status == DOINIT_DID_SOME)
    {
	/* Do another concurrency calculation pass.  This time, any
	** hash op already reserved won't appear in the concur_num
	** values, but WILL appear in concur_size.  Since they
	** were the smaller ones, the hope is that more memory will
	** be available for the larger hash operations.
	** This bit is pretty much identical to the first pass above.
	**
	** (If the doinit trial pass didn't reserve anything, nothing
	** would change, so don't bother if did-none.)
	*/
	for (top_state = &header_state[0];
	     top_state <= &header_state[num_headers-1];
	     ++ top_state)
	{
	    HASHRES_STATE cur_state;

	    top_state->running_num = 0;	/* start over */
	    top_state->running_size = 0;
	    cur_state.act = NULL; cur_state.node = NULL;
	    cur_state.running_num = 0;
	    cur_state.running_size = 0;
	    cur_state.building_num = 0;
	    cur_state.building_size = 0;
	    qee_hash_concurrency(dsh, top_state->act, top_state->node,
		    top_state, &cur_state);
	}

	/* If there was more than one sub-plan, we have to add 'em all
	** together, since they all run independently.  No way to know what
	** point any particular thread is in.  We're adding worst-cases here.
	*/
	if (num_headers > 1)
	{
	    HASHRES_STATE cur_state;
	    i4 tot_num;
	    SIZE_TYPE tot_size;

	    tot_num = 0; tot_size = 0;
	    for (top_state = &header_state[num_headers-1];
		 top_state >= &header_state[0];
		 -- top_state)
	    {
		tot_num += top_state->running_num;
		tot_size += top_state->running_size;
	    }
	    for (top_state = &header_state[num_headers-1];
		 top_state >= &header_state[0];
		 -- top_state)
	    {
		cur_state.running_num = tot_num - top_state->running_num;
		cur_state.running_size = tot_size - top_state->running_size;
		qee_hash_add(dsh, top_state->act, top_state->node,
			    top_state, &cur_state, FALSE);
	    }
	}
    }

    /* Now reserve whatever remains, unconditionally */
    completion_status = DOINIT_DID_NONE;
    status = qee_hash_doinit(rcb, dsh, act,
			session_limit, &completion_status, TRUE, ulm);

    return (status);
} /* qee_hash_reserve */

/*
** Name: qee_hash_add -- Add concurrency to plan fragment
**
** Description:
**
**	There are several situations where a hash operation's concurrency
**	is affected by something that goes on elsewhere in the plan.
**	The most obvious example is a parallel plan, where the concurrency
**	for any one hash op has to be increased by the total worst-case
**	concurrency in all of the other independent child threads.
**	We need a way to add a concurrency value all the way through a
**	query plan fragment, and that is what this routine does.
**
**	A "fragment" is defined to stop at an EXCH, since the driver
**	separates out parallel children into their own individually
**	handled sub-plans.
**
**	The caller supplies a HASHRES_STATE variable containing the
**	concurrency counts/sizes to add, and a pointer to the top of
**	the plan fragment to affect.  The caller can also indicate whether
**	the adding should stop at sort nodes or not.  (Usually, not.)
**	The stop-at-sort feature is to cater to merge joins which usually
**	have a sort high in the left input;  since the sort drains its
**	input before returning rows, the right side of the merge join
**	isn't concurrent with anything below the sort.
**
** Inputs:
**	dsh			Data segment header
**	act or node		Top action or node for fragment to add to
**				(Only one given, the other will be NULL)
**	top_state		Ptr to subplan header state, for tracking
**				the maximum concurrency number for the
**				entire subplan.
**	adder_state
**	    .running_num	Value to add to all concurrency numbers
**	    .running_size	Value to add to all concurrency sizes
**	stop_at_sort		TRUE to stop at sort/tsort, FALSE to do
**				the entire plan fragment unconditionally.
**
** Outputs:
**	None
**
** Side Effects:
**	May update top_state if a new most-concurrent hash op is found.
**
** History:
**	27-Sep-2007 (kschendel)
**	    Written.
*/

static void
qee_hash_add(QEE_DSH *dsh, QEF_AHD *act, QEN_NODE *node,
	HASHRES_STATE *top_state, HASHRES_STATE *adder_state,
	bool stop_at_sort)
{
    QEN_HASH_AGGREGATE *hag;		/* Hash aggregate info */
    QEN_HASH_BASE *hbase;		/* Hash join info */
    QEN_NODE *inner, *outer;		/* Join children */

    /* If we were fed an action, add to it if it's hash agg, otherwise
    ** just hop over it and process the query fragment below it.
    ** Note that the caller will verify that the action is a QEP-type
    ** action, none of your goofy IF's and such!
    */
    if (act != NULL)
    {
	if (act->ahd_atype == QEA_HAGGF)
	{
	    hag = dsh->dsh_hashagg[act->qhd_obj.qhd_qep.u1.s2.ahd_agcbix];
	    if (hag->hshag_estmem == 0)
	    {
		hag->hshag_concur_num += adder_state->running_num;
		hag->hshag_concur_size += adder_state->running_size;
		if (hag->hshag_concur_num > top_state->running_num)
		    top_state->running_num = hag->hshag_concur_num;
		if (hag->hshag_concur_size > top_state->running_size)
		    top_state->running_size = hag->hshag_concur_size;
	    }
	}
	node = act->qhd_obj.qhd_qep.ahd_qep;
    }

    /* Dispatch on node, process query fragment recursively */
    inner = NULL;
    switch (node->qen_type)
    {
	case QE_HJOIN:
	    hbase = dsh->dsh_hash[node->node_qen.qen_hjoin.hjn_hash];
	    if (hbase->hsh_msize == 0)
	    {
		hbase->hsh_concur_num += adder_state->running_num;
		hbase->hsh_concur_size += adder_state->running_size;
		if (hbase->hsh_concur_num > top_state->running_num)
		    top_state->running_num = hbase->hsh_concur_num;
		if (hbase->hsh_concur_size > top_state->running_size)
		    top_state->running_size = hbase->hsh_concur_size;
	    }
	    outer = node->node_qen.qen_hjoin.hjn_out;
	    inner = node->node_qen.qen_hjoin.hjn_inner;
	    break;

	case QE_TJOIN:
	    outer = node->node_qen.qen_tjoin.tjoin_out;
	    break;

	case QE_KJOIN:
	    outer = node->node_qen.qen_kjoin.kjoin_out;
	    break;

	case QE_CPJOIN:
	case QE_FSMJOIN:
	case QE_ISJOIN:
	    outer = node->node_qen.qen_sjoin.sjn_out;
	    inner = node->node_qen.qen_sjoin.sjn_inner;
	    break;

	case QE_SORT:
	    if (stop_at_sort)
		return;
	    outer = node->node_qen.qen_sort.sort_out;
	    break;

	case QE_TSORT:
	    if (stop_at_sort)
		return;
	    outer = node->node_qen.qen_tsort.tsort_out;
	    break;

	case QE_SEJOIN:
	    outer = node->node_qen.qen_sejoin.sejn_out;
	    inner = node->node_qen.qen_sejoin.sejn_inner;
	    break;

	case QE_QP:
	    /* QP is a special case, apply add to all hashop-containing
	    ** actions below the QP
	    */
	    act = node->node_qen.qen_qp.qp_act;
	    while (act != NULL)
	    {
		if (act->ahd_flags & QEA_NODEACT
		  && act->qhd_obj.qhd_qep.ahd_qepflag & AHD_CONTAINS_HASHOP)
		{
		    qee_hash_add(dsh, act, NULL,
				top_state, adder_state, stop_at_sort);
		}
		act = act->ahd_next;
	    }
	    return;

	default:
	    /* Orig, exch, tproc -- don't go below these. */
	    return;
    } /* switch */

    /* Recursively process outer, and inner if there is one */
    if (inner != NULL)
	qee_hash_add(dsh, NULL, inner, top_state, adder_state, stop_at_sort);

    qee_hash_add(dsh, NULL, outer, top_state, adder_state, stop_at_sort);
    return;

} /* qee_hash_add */

/*
** Name: qee_hash_concurrency -- Compute hash operation potential concurrency
**
** Description:
**
**	This routine walks a sub-plan, looking for hash join or hash
**	aggregation operations.  It computes how many other hash ops
**	are potentially active at that point, and how much total memory
**	they have reserved.  Here, "active" means "may have allocated
**	buffer space".
**
**	The key to this whole mess is that a hash join reads its left
**	(build) input to completion, BEFORE reading any of its right
**	input.  (Assuming that the right input is not an orig;  and if
**	it IS an orig, it's irrelevant as far as hash reservations go.)
**	Also, the hash join cannot emit any rows until it has started
**	reading from the right (probe) input.  The final point is that when
**	a hash join completes, it closes its sorthash memory stream and
**	frees up all of the query-runtime space that it asked for.
**
**	So, consider these query plans (assume NOT PC-joins):
**
**	    hj#1
**          /  \
**	 hj#2  hj#3
**
**	The maximum concurrency here is 2, not 3.  First, join#1 asks
**	for (all) rows from join #2.  As #2 is supplying rows, #1 will
**	allocate space, so both are "active".  Only when #2 is finished
**	and deallocates is #3 asked for its first row.
**
**	        hj#1
**             /
**          hj#2
**         /
**      hj#3
**
**	Curiously, the maximum concurrency here is ALSO 2, not 3.
**	#1 asks for rows from #2.  #1 does not allocate any space until
**	it receives a row from #2.  #2 in turn asks for rows from #3,
**	so 2 and 3 are concurrently active.  Only when #3 completes will
**	#2 ask for rows from its probe side and send the results on to
**	#1, making #1 and #2 concurrently active (but #3 is gone).
**
**	hj#1
**	   \
**	   hj#2
**            \
**	      hj#3
**
**	This time the maximum concurrency is indeed 3, as #1 asks #2
**	for a row, #2 in turn asks #3, and a joining row flows through
**	all 3 before emerging from the top.
**
**	Getting this right is hard.  Two examples are presented without
**	running commentary;  if you run them through the standard
**	hash-join rules commented in-line (below) you'll see why
**	we need the building vs running business.
**
**	      hj                   hj
**             \                    \
**              hj                   hj (*)
**             /                    /
**            hj                   hj (A)
**             \                  /
**              hj               hj
**               \                \
**                hj               hj
**                                  \
**                                   hj
**
**	The concurrency number for the left plan is 5 everywhere.
**	For the right plan, it's 5 everywhere except the starred join,
**	which is concurrency 3!  (The bottom 3 joins are gone before
**	join (A) can feed the first build row into join (*), so (*)
**	would see just itself and (A), except that the topmost one is
**	waiting for a probe as well.)
**
**	Sorts also create a point at which all of the input is consumed
**	before any of the output.  The rules for other joins can be
**	tricky, and the exact details are commented in-line.
**
**	Only a single serially-executed plan is walked.  If the plan
**	is a parallel plan, each child-thread sub-plan is done independently.
**	The driver will add things up later, if needed.
**
**	One detail ignored in the hand-waving above is that we want
**	to treat already-reserved hashops as if they aren't there, sort of.
**	They are to contribute to concur_size, but NOT to concur_num.
**	The comments will use the word THIS to mean specifically:
**	- num = zero, if the current hash-op has been reserved already;
**	- num = qen_nthreads, if the current hash-op is NOT reserved yet
**	  (note that qen_nthreads is typically 0/1 except under a 1:N
**	  non-union EXCH, which in turn is always to be found over
**	  a partition compatible plan fragment.)
**	- size = this hashop's reservation (may be zero if not done yet)
**
**	As another commentary shorthand, the notation [A,B] will
**	mean that we pass A in building_num and building_size, and
**	pass B in running_num and running_size.
**
** Inputs:
**	dsh			Data segment header
**	act or node		Action or node to examine (only one filled in,
**				the other will be NULL)
**	top_state		Ptr to subplan header state, for tracking
**				the maximum concurrency number for the
**				entire subplan.
**	parent_state
**	    .act or .node	Next-higher-up hash join or hash agg, if
**				there is a relevant one.
**	    .building_num	Number of concurrent hashops above this node
**				that are "live" (will have allocated space)
**				even before this node returns rows.
**				(Only counting non-reserved hashops.)
**	    .building_size	Total reservation of reserved hashops above
**				this node that are "live" even before this
**				node returns rows.
**	    .running_num	Concurrent hashops (non-reserved) that this
**				node will see once it starts delivering rows.
**	    .running_size	Total concurrent reservation that this node
**				will see once it starts delivering rows.
**
** Outputs:
**	parent_state
**	    .building_num	Worst-case concurrent hashops at or below
**				this node, before any rows are delivered.
**	    .building_size	Worse-case concurrent reserved size at or
**				below this node, before rows are delivered.
**	    .running_num	Worst-case concurrent hashops at or below
**				this node as rows are streaming to the parent.
**	    .running_size	Worst-case concurrent reserved size at or
**				below this node as rows are being returned.
**
** Side Effects:
**	will update running_xxx in top_state if a new highest concurrency
**	level or size is discovered
**
** History:
**	26-Sep-2007 (kschendel)
**	    Invent tricky new algorithm for slicing the session limit
**	    for hash joins / hash aggs.
**	11-Sep-2009 (kschendel) SIR 122513
**	    Turn on the nested-PC-join and PC-agg stuff.
*/

static void
qee_hash_concurrency(QEE_DSH *dsh, QEF_AHD *act, QEN_NODE *node,
	HASHRES_STATE *top_state, HASHRES_STATE *parent_state)
{
    bool part_compat;			/* PC-join or PC-agg */
    bool stop_at_sorts;
    HASHRES_STATE l_state, r_state; 	/* State to/from children */
    i4 this;				/* THIS hashop, see intro */
    i4 n;
    QEN_HASH_AGGREGATE *hag;		/* Hash aggregate info */
    QEN_HASH_BASE *hbase;		/* Hash join info */
    QEN_NODE *inner, *outer;		/* Join children */
    SIZE_TYPE s;
    SIZE_TYPE this_size;		/* THIS hashop's reserved size */

    /* If we were fed an action, process it if it's hash agg, otherwise
    ** just hop over it and process the query fragment below it.
    ** Note that the caller will verify that the action is a QEP-type
    ** action, none of your goofy IF's and such!
    */
    if (act != NULL)
    {
	if (act->ahd_atype == QEA_HAGGF)
	{
	    /* Calculations for a hash agg depends on whether it's partition
	    ** compatible or not.  Ordinary hash agg:
	    ** - process QP, passing [parent.building, parent.building + THIS],
	    **   returning L;
	    ** - hashagg concurrency is max of hashagg receiving rows and
	    **   hashagg sending rows to parent, which is max of:
	    **     parent.building + L.running + THIS, parent.running + THIS
	    ** - return.building is max(L.building, L.running + THIS)
	    ** - return.running is THIS
	    **
	    ** For partition compatible, it's different, as the hashagg
	    ** child subtree is still running when hashagg starts returning
	    ** rows.  So the procedure is:
	    ** - process QP, passing [parent.building, 
	    **     max(parent.building, parent.running) + THIS],
	    **   returning L;
	    ** - hashagg concurrency is child sending rows and us sending
	    **   groups, plus worst case from parent:
	    **     max(parent.building, parent.running) + L.running + THIS
	    ** - return.building is L.building
	    ** - return.running is L.running + THIS
	    */
	    hag = dsh->dsh_hashagg[act->qhd_obj.qhd_qep.u1.s2.ahd_agcbix];
	    part_compat = (act->qhd_obj.qhd_qep.ahd_qepflag & AHD_PART_SEPARATE) != 0;
	    l_state.act = act;
	    l_state.node = NULL;
	    this = 1;
	    this_size = hag->hshag_estmem;
	    if (this_size != 0)
		this = 0;
	    l_state.building_num = parent_state->building_num;
	    l_state.building_size = parent_state->building_size;
	    l_state.running_num = parent_state->building_num + this;
	    l_state.running_size = parent_state->building_size + this_size;
	    if (part_compat)
	    {
		/* PC-agg, parent can be in row receipt while child is
		** still sending rows.  Tell child of worst case.
		*/
		if (parent_state->running_num > parent_state->building_num)
		    l_state.running_num = parent_state->running_num + this;
		if (parent_state->running_size > parent_state->building_size)
		    l_state.running_size = parent_state->running_size + this_size;
	    }
	    qee_hash_concurrency(dsh, NULL, act->qhd_obj.qhd_qep.ahd_qep,
			top_state, &l_state);
	    if (part_compat)
	    {
		/* our concurrency is us, plus child running, plus worst
		** case in parent.
		*/
		n = parent_state->running_num + l_state.running_num + this;
		if (parent_state->building_num > parent_state->running_num)
		    n = parent_state->building_num + l_state.running_num + this;
		s = parent_state->running_size + l_state.running_size + this_size;
		if (parent_state->building_size > parent_state->running_size)
		    s = parent_state->building_size + l_state.running_size + this_size;
	    }
	    else
	    {
		/* Non-PC-agg, our concurrency is worst case of getting rows
		** vs sending rows.
		*/
		n = parent_state->building_num + l_state.running_num + this;
		if (parent_state->running_num + this > n)
		    n = parent_state->running_num + this;
		s = parent_state->building_size + l_state.running_size + this_size;
		if (parent_state->running_size + this_size > s)
		    s = parent_state->running_size + this_size;
	    }
	    hag->hshag_concur_size = s;
	    hag->hshag_concur_num = n;
	    if (part_compat)
	    {
		/* PC aggregation, children running while rows coming out */
		parent_state->building_num = l_state.building_num;
		parent_state->building_size = l_state.building_size;
		parent_state->running_num = l_state.running_num + this;
		parent_state->running_size = l_state.running_size + this_size;
	    }
	    else
	    {
		/* non-partition-compatible agg, children are drained */
		parent_state->running_num = this;
		parent_state->running_size = this_size;
		/* Before any rows returned, take worse of child setup,
		** child returning rows to us.
		*/
		parent_state->building_num = l_state.running_num + this;
		if (l_state.building_num > parent_state->building_num)
		    parent_state->building_num = l_state.building_num;
		parent_state->building_size = l_state.running_size + this_size;
		if (l_state.building_size > parent_state->building_size)
		    parent_state->building_size = l_state.building_size;
	    }
	    /* Check for sub-plan worst case, then return */
	    if (hag->hshag_concur_num > top_state->running_num)
		top_state->running_num = hag->hshag_concur_num;
	    if (hag->hshag_concur_size > top_state->running_size)
		top_state->running_size = hag->hshag_concur_size;
	    return;
	}
	/* here if not haggf, just skip down to the qep and analyse it */
	node = act->qhd_obj.qhd_qep.ahd_qep;
    }
    /* Must have been a node, dispatch by node type. */
    part_compat = (node->qen_flags & (QEN_PART_SEPARATE | QEN_PART_NESTED)) != 0;
    switch (node->qen_type)
    {
	case QE_HJOIN:
	    /* Interesting one first.
	    ** The partition compatible and non-partition-compatible
	    ** cases are entirely different.
	    ** For the non-PC-join case, the build side runs, loads this
	    ** join, then we run the probe side and return rows.
	    ** - call build with [parent.building, parent.building + THIS]
	    **   returning L
	    ** - call probe with [parent.building+THIS, parent.running+THIS]
	    **   returning R
	    ** - this node concurrency is worst case of build delivering
	    **   rows, probe getting started, and probe delivering rows.
	    **   (Notice that we don't count build getting started because
	    **   this node isn't live at that point.)  So, it's max of:
	    **     parent.building + L.running + THIS,
	    **     parent.building + R.building + THIS,
	    **     parent.running + R.running + THIS.
	    ** - Returned concurrency is:
	    **    [max(L.building, L.running+THIS, R.building+THIS),
	    **     R.running + THIS]
	    **   i.e. concurrency before this node returns rows is the
	    **   worst of the three situations L building, L sending build
	    **   rows, and R building;  and this node is active for the
	    **   latter two.  Concurrency while running is just R running
	    **   plus this node.
	    **
	    ** The PC-join case is rather uglier because the build side is
	    ** not drained before we start on the probe side, and both
	    ** build and probe are still running as we return rows.
	    ** - call build with [parent.building,
	    **      max(parent.building + THIS, parent.running + THIS)]
	    **   returning L
	    ** - call probe with [parent.building + THIS,
	    **      max(parent.building + THIS, parent.running + THIS)]
	    **   returning R
	    ** - Add max(L.building, L.running) to nodes on the R side;
	    **   add max(R.building, R.running) to nodes on the L side;
	    **   this reflects the fact that both sides are active
	    **   simultaneously when rows are flowing.
	    ** - this node concurrency is the sum of the worst cases
	    **   in all directions:
	    **     max(parent.building, parent.running)
	    **     + max(L.building, L.running) + max(R.building, R.running)
	    **     + THIS
	    **   which probably could be refined a bit but it's not worth it.
	    ** - Returned concurrency is:
	    **    [max(L.building, L.running+THIS, R.building+THIS),
	    **     max(L.building,L.running) + max(R.building,R.running)+THIS]
	    */
	    hbase = dsh->dsh_hash[node->node_qen.qen_hjoin.hjn_hash];
	    this = node->qen_nthreads;
	    if (this == 0)
		this = 1;		/* Not under an exch, use 1 */
	    this_size = hbase->hsh_msize * this;
	    if (hbase->hsh_msize != 0)
		this = 0;		/* Don't count if already reserved */
	    if (part_compat)
	    {
		/* Partition compatible join style */
		l_state.act = NULL;
		l_state.node = node;
		l_state.building_num = parent_state->building_num;
		l_state.building_size = parent_state->building_size;
		l_state.running_num = parent_state->running_num;
		if (parent_state->building_num > parent_state->running_num)
		    l_state.running_num = parent_state->building_num;
		l_state.running_num += this;
		l_state.running_size = parent_state->running_size;
		if (parent_state->building_size > parent_state->running_size)
		    l_state.running_size = parent_state->building_size;
		l_state.running_size += this_size;
		STRUCT_ASSIGN_MACRO(l_state, r_state);
		r_state.building_num += this;
		r_state.building_size += this_size;
		qee_hash_concurrency(dsh, NULL,
			node->node_qen.qen_hjoin.hjn_out,
			top_state, &l_state);
		qee_hash_concurrency(dsh, NULL,
			node->node_qen.qen_hjoin.hjn_inner,
			top_state, &r_state);
		/* Calculate returned "building" value before doing max's
		** in L, R.
		*/
		n = l_state.building_num;
		if (l_state.running_num + this > n)
		    n = l_state.running_num + this;
		if (r_state.building_num + this  > n)
		    n = r_state.building_num + this;
		s = l_state.building_size;
		if (l_state.running_size + this_size > s)
		    s = l_state.running_size + this_size;
		if (r_state.building_size + this_size  > s)
		    s = r_state.building_size + this_size;
		/* We need max(building,running) in various places, make
		** "running" be the max for adding.
		*/
		if (l_state.building_num > l_state.running_num)
		    l_state.running_num = l_state.building_num;
		if (l_state.building_size > l_state.running_size)
		    l_state.running_size = l_state.building_size;
		if (r_state.building_num > r_state.running_num)
		    r_state.running_num = r_state.building_num;
		if (r_state.building_size > r_state.running_size)
		    r_state.running_size = r_state.building_size;
		if (parent_state->building_num > parent_state->running_num)
		    parent_state->running_num = parent_state->building_num;
		if (parent_state->building_size > parent_state->running_size)
		    parent_state->running_size = parent_state->building_size;
		qee_hash_add(dsh, NULL,
			node->node_qen.qen_hjoin.hjn_out,
			top_state, &r_state, FALSE);
		qee_hash_add(dsh, NULL,
			node->node_qen.qen_hjoin.hjn_inner,
			top_state, &l_state, FALSE);
		hbase->hsh_concur_num = parent_state->running_num
			+ l_state.running_num + r_state.running_num + this;
		hbase->hsh_concur_size = parent_state->running_size
			+ l_state.running_size + r_state.running_size + this_size;
		parent_state->building_num = n;
		parent_state->building_size = s;
		parent_state->running_num =
			l_state.running_num + r_state.running_num + this;
		parent_state->running_size =
			l_state.running_size + r_state.running_size + this_size;
	    }
	    else
	    {
		/* Standard join style that drains build first */
		hbase->hsh_parent_act = parent_state->act;
		hbase->hsh_parent_node = parent_state->node;
		/* Parent isn't running yet as build side runs */
		l_state.act = NULL;
		l_state.node = node;
		l_state.building_num = parent_state->building_num;
		l_state.building_size = parent_state->building_size;
		l_state.running_num = l_state.building_num + this;
		l_state.running_size = l_state.building_size + this_size;
		qee_hash_concurrency(dsh, NULL,
			node->node_qen.qen_hjoin.hjn_out,
			top_state, &l_state);
		/* Probe child sees parent, ourselves running */
		r_state.act = NULL;
		r_state.node = node;
		r_state.building_num = parent_state->building_num + this;
		r_state.building_size = parent_state->building_size + this_size;
		r_state.running_num = parent_state->running_num + this;
		r_state.running_size = parent_state->running_size + this_size;
		qee_hash_concurrency(dsh, NULL,
			node->node_qen.qen_hjoin.hjn_inner,
			top_state, &r_state);
		/* Our concurrency is max of L running, R building, R running
		** L building doesn't count for us although we'll pass it up.
		*/
		n = parent_state->building_num + l_state.running_num;
		if (parent_state->building_num + r_state.building_num > n)
		    n = parent_state->building_num + r_state.building_num;
		if (parent_state->running_num + r_state.running_num > n)
		    n = parent_state->running_num + r_state.running_num;
		n += this;
		s = parent_state->building_size + l_state.running_size;
		if (parent_state->building_size + r_state.building_size > s)
		    s = parent_state->building_size + r_state.building_size;
		if (parent_state->running_size + r_state.running_size > s)
		    s = parent_state->running_size + r_state.running_size;
		s += this_size;
		hbase->hsh_concur_size = s;
		hbase->hsh_concur_num = n;
		/* Returned concurrency is [worst-case building, running] */
		n = l_state.building_num;
		if (l_state.running_num + this > n)
		    n = l_state.running_num + this;
		if (r_state.building_num + this > n)
		    n = r_state.building_num + this;
		parent_state->building_num = n;
		s = l_state.building_size;
		if (l_state.running_size + this_size > s)
		    s = l_state.running_size + this_size;
		if (r_state.building_size + this_size > s)
		    s = r_state.building_size + this_size;
		parent_state->building_size = s;
		parent_state->running_num = r_state.running_num + this;
		parent_state->running_size = r_state.running_size + this_size;
	    }
	    /* Check for new subplan maxima, then done */
	    if (hbase->hsh_concur_num > top_state->running_num)
		top_state->running_num = hbase->hsh_concur_num;
	    if (hbase->hsh_concur_size > top_state->running_size)
		top_state->running_size = hbase->hsh_concur_size;
	    return;

	case QE_KJOIN:
	    /* Right side is an orig, just move down the left */
	    qee_hash_concurrency(dsh, NULL,
			node->node_qen.qen_kjoin.kjoin_out,
			top_state, parent_state);
	    return;

	case QE_TJOIN:
	    /* Right side is an orig, just move down the left */
	    qee_hash_concurrency(dsh, NULL,
			node->node_qen.qen_tjoin.tjoin_out,
			top_state, parent_state);
	    return;

	case QE_SEJOIN:
	    outer = node->node_qen.qen_sejoin.sejn_out;
	    inner = node->node_qen.qen_sejoin.sejn_inner;
	    goto nonhash_join;	/* Just below */

	case QE_FSMJOIN:
	case QE_ISJOIN:
	case QE_CPJOIN:
	    outer = node->node_qen.qen_sjoin.sjn_out;
	    inner = node->node_qen.qen_sjoin.sjn_inner;

	  nonhash_join:

	    /* All of the non-hash non-key joins come here.  They
	    ** work pretty much like the PC-join hash-join, because
	    ** both inputs run concurrently.  Thus:
	    ** - call left side with [parent.building,parent.running] giving L
	    ** - call right side with [parent.building,parent.running] giving R
	    ** - take max(building,running) for both L and R
	    ** - Add R-max to left side, add L-max to right input
	    **   (see below)
	    ** - returned concurrency is L-max + R-max
	    **
	    ** The only issue is whether it's necessary to propagate the
	    ** "add" through sorts.  Sort nodes usually consume their
	    ** entire input before returning rows, thus isolating the
	    ** input from what's going up higher up.  However!  The
	    ** right sides of all but the FSM-join are resettable,
	    ** meaning that we'll run the entire fragment again.
	    ** Also, FSM-joins can be partition compatible, which
	    ** implies that the inputs don't actually complete even
	    ** under sorts.
	    ** The upshot is that the "add" can stop at sorts for
	    ** LEFT inputs of SE/IS/CP-joins, and for either input
	    ** of non-partition-compatible FSM-joins.  Otherwise
	    ** we have to blow the add all the way down.
	    */
	    l_state.act = NULL;
	    l_state.node = NULL;
	    l_state.building_num = parent_state->building_num;
	    l_state.building_size = parent_state->building_size;
	    l_state.running_num = parent_state->running_num;
	    l_state.running_size = parent_state->running_size;
	    STRUCT_ASSIGN_MACRO(l_state, r_state);
	    qee_hash_concurrency(dsh, NULL, outer,
			top_state, &l_state);
	    qee_hash_concurrency(dsh, NULL, inner,
			top_state, &r_state);
	    /* We need max(building,running) in various places, make
	    ** "running" be the max for adding.
	    */
	    if (l_state.building_num > l_state.running_num)
		l_state.running_num = l_state.building_num;
	    if (l_state.building_size > l_state.running_size)
		l_state.running_size = l_state.building_size;
	    if (r_state.building_num > r_state.running_num)
		r_state.running_num = r_state.building_num;
	    if (r_state.building_size > r_state.running_size)
		r_state.running_size = r_state.building_size;
	    stop_at_sorts = TRUE;
	    if (node->qen_type == QE_FSMJOIN && part_compat)
		stop_at_sorts = FALSE;
	    qee_hash_add(dsh, NULL, outer, top_state, &r_state, stop_at_sorts);
	    stop_at_sorts = FALSE;
	    if (node->qen_type == QE_FSMJOIN && !part_compat)
		stop_at_sorts = TRUE;
	    qee_hash_add(dsh, NULL, inner, top_state, &l_state, stop_at_sorts);
	    parent_state->building_num = l_state.running_num + r_state.running_num;
	    parent_state->building_size = l_state.building_size + r_state.building_size;
	    parent_state->running_num = parent_state->building_num;
	    parent_state->running_size = parent_state->building_size;
	    return;

	case QE_SORT:
	case QE_TSORT:
	    /* Sorts eat all of their input before emitting any output.
	    ** So, we can pass the parent state to the sort input, but
	    ** then return a running concurrency level of zero.
	    ** That's if the sort is not a partition compatible sort.
	    ** If it is, the child is NOT finished when the sort spits
	    ** out some rows.  PC sorts look like pass-thrus.
	    */
	    if (node->qen_type == QE_SORT)
		outer = node->node_qen.qen_sort.sort_out;
	    else
		outer = node->node_qen.qen_tsort.tsort_out;
	    if (part_compat)
	    {
		qee_hash_concurrency(dsh, NULL, outer,
			top_state, parent_state);
		return;
	    }
	    qee_hash_concurrency(dsh, NULL, outer,
			top_state, parent_state);
	    /* Sorts reflect zero hashop concurrency up the tree */
	    parent_state->running_num = 0;
	    parent_state->running_size = 0;
	    return;

	case QE_QP:
	    /* QP nodes are funky because their input is a list of
	    ** actions, not a child node.  Run thru the QP actions,
	    ** and for any that contain hash-ops, pass the parent state.
	    ** Treatment of the firstrow stuff is slightly tricky.
	    ** If the leading actions are SAGG's which compute scalars
	    ** (and don't return rows up the QP), pass [parent.building,
	    ** parent.building] down into the SAGG.
	    ** If the first non-SAGG action is a hash agg, pass parent
	    ** as-is [building,running].
	    ** After a hash agg, or before the first non-SAGG non-HAGGF
	    ** action, pass max(parent.building,parent.running) for both
	    ** building and running since we don't know if the parent has
	    ** seen any rows yet.
	    ** Return a hash-concurrency level of the max of all the
	    ** values returned from the actions.
	    */
	    /* Track max in r-state, use l-state for child calls */
	    r_state.building_num = 0;
	    r_state.building_size = 0;
	    r_state.running_num = 0;
	    r_state.running_size = 0;
	    parent_state->act = NULL;  parent_state->node = NULL;
	    act = node->node_qen.qen_qp.qp_act;
	    /* Process any non-row-returning saggs first */
	    while (act != NULL && act->ahd_atype == QEA_SAGG)
	    {
		if (act->ahd_flags & QEA_NODEACT
		  && act->qhd_obj.qhd_qep.ahd_qepflag & AHD_CONTAINS_HASHOP)
		{
		    STRUCT_ASSIGN_MACRO(*parent_state, l_state);
		    l_state.running_num = l_state.building_num;
		    l_state.running_size = l_state.building_size;
		    qee_hash_concurrency(dsh, act, NULL,
				top_state, &l_state);
		    /* Return worst case of building,running from child */
		    if (l_state.running_num > l_state.building_num)
			l_state.building_num = l_state.running_num;
		    if (l_state.running_size > l_state.building_size)
			l_state.building_size = l_state.running_size;
		    /* Return worst case of child, worst-so-far, leaving
		    ** running state zero (no rows thru QP yet)
		    */
		    if (l_state.building_num > r_state.building_num)
			r_state.building_num = l_state.building_num;
		    if (l_state.building_size > r_state.building_size)
			r_state.building_size = l_state.building_size;
		}
		act = act->ahd_next;
	    }
	    /* If next thing is a hash agg, pass full parent state */
	    if (act != NULL && act->ahd_atype == QEA_HAGGF)
	    {
		STRUCT_ASSIGN_MACRO(*parent_state, l_state);
		qee_hash_concurrency(dsh, act, NULL,
				top_state, &l_state);
		/* Worst case of child and worst-so-far */
		if (l_state.building_num > r_state.building_num)
		    r_state.building_num = l_state.building_num;
		if (l_state.building_size > r_state.building_size)
		    r_state.building_size = l_state.building_size;
		if (l_state.running_num > r_state.running_num)
		    r_state.running_num = l_state.running_num;
		if (l_state.running_size > r_state.running_size)
		    r_state.running_size = l_state.running_size;
		act = act->ahd_next;
	    }
	    if (act != NULL)
	    {
		/* For whatever is left assume parent is now in worst of
		** building or running state - we might have fed rows, or not.
		*/
		if (parent_state->building_num > parent_state->running_num)
		    parent_state->running_num = parent_state->building_num;
		if (parent_state->building_size > parent_state->running_size)
		    parent_state->running_size = parent_state->building_size;
		parent_state->building_num = parent_state->running_num;
		parent_state->building_size = parent_state->running_size;
	    }
	    while (act != NULL)
	    {
		if (act->ahd_flags & QEA_NODEACT
		  && act->qhd_obj.qhd_qep.ahd_qepflag & AHD_CONTAINS_HASHOP)
		{
		    STRUCT_ASSIGN_MACRO(*parent_state, l_state);
		    qee_hash_concurrency(dsh, act, NULL,
				top_state, &l_state);
		if (l_state.building_num > r_state.building_num)
		    r_state.building_num = l_state.building_num;
		if (l_state.building_size > r_state.building_size)
		    r_state.building_size = l_state.building_size;
		if (l_state.running_num > r_state.running_num)
		    r_state.running_num = l_state.running_num;
		if (l_state.running_size > r_state.running_size)
		    r_state.running_size = l_state.running_size;
		}
		act = act->ahd_next;
	    }
	    /* Pass worst-case back up to QP node's caller */
	    parent_state->building_num = r_state.building_num;
	    parent_state->building_size = r_state.building_size;
	    parent_state->running_num = r_state.running_num;
	    parent_state->running_size = r_state.running_size;
	    return;

	default:
	    /* Orig's, exch's, tproc's - stop there.  We stop at exch because
	    ** independent child threads are evaluated separately.
	    ** Pass back zero.
	    */
	    parent_state->building_num = parent_state->running_num = 0;
	    parent_state->building_size = parent_state->running_size = 0;
	    return;
    } /* switch */

    /* notreached */
    return;
} /* qee_hash_concurrency */

/*
** Name: qee_hash_findexch -- Find child thread starting points
**
** Description:
**
**	At the start of the reservations process on a parallel plan,
**	this routine recursively locates all of the child thread
**	starting points;  in other words, all of the immediate EXCH
**	node children.  This is fairly routine query-plan digging,
**	with the most interesting point being that for a union-style
**	EXCH over a QP node, the relevant "starting points" are in
**	fact the QP child actions, NOT the QP node itself.
**
** Inputs:
**	node			Pointer to current query plan node
**	cur_statep		HASHRES_STATE ** points to next header
**				state entry to fill out when a child
**				starting point seen.
**
** Outputs:
**	None
**
** History:
**	26-Sep-2007 (kschendel)
**	    Written.
*/

static void
qee_hash_findexch( QEN_NODE *node, HASHRES_STATE **cur_statep)
{
    QEN_NODE *outer, *inner;

    inner = NULL;
    switch (node->qen_type)
    {
	case QE_EXCHANGE:
	    outer = node->node_qen.qen_exch.exch_out;
	    if (outer->qen_type == QE_ORIG || outer->qen_type == QE_ORIGAGG)
		return;
	    /* This is a thread-starter, check for union style */
	    if ((node->node_qen.qen_exch.exch_flag & EXCH_UNION) == 0)
	    {
		/* Ordinary 1:1 or 1:N exch */
		(*cur_statep)->node = outer;
		++ (*cur_statep);
		break;			/* Analyze the child */
	    }
	    {
		QEF_AHD *act;

		/* For union style EXCH, QP-node is under exch, all of
		** the QP actions are thread starters.  Also, analyze
		** below each action for nested parallelism.
		*/
		act = outer->node_qen.qen_qp.qp_act;
		while (act != NULL)
		{
		    if (act->ahd_flags & QEA_NODEACT
		      && act->qhd_obj.qhd_qep.ahd_qepflag & AHD_CONTAINS_HASHOP)
		    {
			(*cur_statep)->act = act;
			++ (*cur_statep);
		    }
		    if (act->ahd_flags & QEA_NODEACT)
			qee_hash_findexch(act->qhd_obj.qhd_qep.ahd_qep, cur_statep);
		    act = act->ahd_next;
		}
	    }
	    /* No need to go lower, we just did */
	    return;

	case QE_TJOIN:
	    outer = node->node_qen.qen_tjoin.tjoin_out;
	    break;

	case QE_KJOIN:
	    outer = node->node_qen.qen_kjoin.kjoin_out;
	    break;

	case QE_CPJOIN:
	case QE_FSMJOIN:
	case QE_ISJOIN:
	    outer = node->node_qen.qen_sjoin.sjn_out;
	    inner = node->node_qen.qen_sjoin.sjn_inner;
	    break;

	case QE_SORT:
	    outer = node->node_qen.qen_sort.sort_out;
	    break;

	case QE_TSORT:
	    outer = node->node_qen.qen_tsort.tsort_out;
	    break;

	case QE_HJOIN:
	    outer = node->node_qen.qen_hjoin.hjn_out;
	    inner = node->node_qen.qen_hjoin.hjn_inner;
	    break;

	case QE_SEJOIN:
	    outer = node->node_qen.qen_sejoin.sejn_out;
	    inner = node->node_qen.qen_sejoin.sejn_inner;
	    break;

	case QE_QP:
	{
	    QEF_AHD *act;

	    /* Analyze the serially executed actions for parallelism */
	    act = node->node_qen.qen_qp.qp_act;
	    while (act != NULL)
	    {
		if (act->ahd_flags & QEA_NODEACT)
		    qee_hash_findexch(act->qhd_obj.qhd_qep.ahd_qep, cur_statep);
		act = act->ahd_next;
	    }
	    /* No need to go lower, we just did */
	    return;
	}

	default:
	    /* orig or tproc, probably */
	    return;
    } /* switch */

    if (inner != NULL)
	qee_hash_findexch(inner, cur_statep);

    qee_hash_findexch(outer, cur_statep);
    return;

} /* qee_hash_findexch */

/*
** Name: qee_hash_doinit - Perform hash op reservation and init.
**
** Description:
**	After a concurrency pass has assigned concurrency numbers and
**	sizes to all of the hash operations in the action being
**	inited, qee_hash_doinit is called to run the actual
**	hash join or hash aggregation initers action-wide.
**	This might be a trial reservation, which only reserves
**	those hash joins/aggs that fit in their assigned slice;
**	or it might be a "do everything" reservation.
**
**	We'll tell the caller whether we successfully inited all of
**	the hash ops, some of them, or none of them.
**
**	This init pass doesn't need to proceed child-thread-wise like
**	the concurrency pass does.  We can make life a little easier
**	and use the action's postfix list, like qee_cract does.
**
** Inputs:
**	rcb			QEF_RCB query request block
**	dsh			Data segment header being inited
**	act			Action header to operate on
**	session_limit		The hash session limit to apply
**	completion_status	Initialize to DOINIT_DID_NONE
**	always_reserve		TRUE to init unconditionally,
**				FALSE to only init hashops that fit
**	ulm			ULM_RCB for the DSH memory stream
**
** Outputs:
**	completion_status	Returned as DOINIT_DID_ALL, SOME, or NONE.
**				Relevant only to a trial pass so that the
**				caller knows what needs to happen next.
**	Returns E_DB_OK or error
**
** History:
**	28-Sep-2007 (kschendel)
**	    New hash operation reservation framework.
**	8-feb-2008 (kschendel)
**	    Fix nasty bug that failed to report "did some" if a nested
**	    hashagg init didn't do ALL.  If all hash joins allocated but
**	    the hash aggs didn't, we erroneously returned ALL to the caller,
**	    which returned to qee without allocating the hash aggs.
**	    The result was a segv in qee-fetch.
*/

static DB_STATUS
qee_hash_doinit(QEF_RCB *rcb, QEE_DSH *dsh, QEF_AHD *act,
	SIZE_TYPE session_limit, COMPLETION_STATE *completion_status,
	bool always_reserve, ULM_RCB *ulm)
{
    bool skipped;		/* TRUE if trial pass, skipped one or more */
    DB_STATUS status;
    QEN_NODE *node;

    skipped = FALSE;
    if (act->ahd_atype == QEA_HAGGF)
    {
	/* Reserve/init hash agg action */
	status = qee_hashaggInit(rcb, dsh, &act->qhd_obj.qhd_qep,
			session_limit, always_reserve, ulm);
	if (status == E_DB_OK)
	    *completion_status = DOINIT_DID_SOME;
	else if (status == E_DB_INFO)
	    skipped = TRUE;
	else
	    return (status);	/* error */
    }
    node = act->qhd_obj.qhd_qep.ahd_postfix;
    while (node != NULL)
    {
	if (node->qen_type == QE_HJOIN)
	{
	    /* Reserve/init hash join node */
	    status = qee_hashInit(rcb, dsh, node, session_limit,
				always_reserve, ulm);
	    if (status == E_DB_OK)
		*completion_status = DOINIT_DID_SOME;
	    else if (status == E_DB_INFO)
		skipped = TRUE;
	    else
		return (status);	/* error */
	}
	else if (node->qen_type == QE_QP)
	{
	    COMPLETION_STATE qp_completion;

	    /* For a QP, we have to recursively process its actions.
	    ** We can limit ourselves to only those containing hashops.
	    */
	    act = node->node_qen.qen_qp.qp_act;
	    while (act != NULL)
	    {
		if (act->ahd_flags & QEA_NODEACT
		  && act->qhd_obj.qhd_qep.ahd_qepflag & AHD_CONTAINS_HASHOP)
		{
		    qp_completion = DOINIT_DID_NONE;
		    status = qee_hash_doinit(rcb, dsh, act, session_limit,
				&qp_completion, always_reserve, ulm);
		    if (status != E_DB_OK)
			return (status);
		    /* Incorporate action completion into our tracking */
		    if (qp_completion != DOINIT_DID_NONE)
			*completion_status = DOINIT_DID_SOME;
		    if (qp_completion != DOINIT_DID_ALL)
			skipped = TRUE;
		}
		act = act->ahd_next;
	    }
	}
	/* All other node types are ignored */
	node = node->qen_postfix;
    } /* while */
    /* If we did some, and didn't skip anything, we did all. */
    if (*completion_status == DOINIT_DID_SOME && !skipped)
	*completion_status = DOINIT_DID_ALL;

    return (E_DB_OK);
} /* qee_hash_doinit */

/*{
** Name: qee_hashInit	- Initialize base structure for hash join.
**
** Description:
**	Persistent hash join info is initialized here. More transient
**	stuff (which varies from execution to execution) is initialized
**	in qee_fetch.
**
**	The most ad-hoc task here is allocating memory.  Hash joining
**	runs fastest if we can keep from spilling any of the build
**	partitions.  Once we spill, it's reasonable to assume that all
**	the partitions will spill more or less evenly, and the next
**	best is to try to fit one partition's worth in memory.
**	That is hard to figure, so we basically use what is available
**	in that case.  The calculations are quite arbitrary.
**
**	Caution: not all of the DSH stuff (like dsh_qefcb etc) is set
**	up at this point.  This is called during DSH initialization.
**
** Inputs:
**	qef_rcb			QEF request block
**	dsh			QEF data segment structure
**	hnode			The hash join query plan node
**	session_limit		Max memory available to session at this point
**	always_reserve		TRUE to do reservation regardless of size;
**				if FALSE, only reserve if optimizer estimate
**				can be fit into the "availmax" slice.
**	ulm			A ulm_rcb to use for allocating initial
**				control structures, should point to the
**				DSH stream.
**
** Outputs:
**	qef_rcb
**	    .error.err_code
**	Returns:
**	    E_DB_OK if successful;
**	    E_DB_INFO if trial reservation didn't fit (always-reserve false);
**	    warn or error if a real problem
**
** Side Effects:
**	    [@description_or_none@]
**
** History:
**      3-mar-99 (inkdo01)
**	    Written for hash join.
**	20-dec-99 (inkdo01)
**	    Changes for table driven memory computation and more accurate
**	    partition, subpartition count determination.
**	jan-00 (inkdo01)
**	    Changes for cartprod logic.
**	7-feb-00 (inkdo01)
**	    Added flag for CO joins.
**	22-aug-01 (inkdo01)
**	    prepare HASH_CHECKNULL flag.
**	3-oct-01 (inkdo01)
**	    Fix key cols to look like a single BYTE(n) col (if no nullables).
**	13-jan-03 (inkdo01)
**	    Add 2nd hash workrow for enhanced CP algorithm.
**	17-jan-03 (inkdo01)
**	    Fix SEGV when build/probe source row size is bigger than proposed
**	    buffer size (bug 109481).
**	5-sep-04 (inkdo01)
**	    Add hsh_brptr/prptr to save row array addrs across resets for 
**	    global base array support.
**	14-dec-04 (inkdo01)
**	    Replace unused cmp_flags by cmp_collID (for collations).
**	6-Jun-2005 (schka24)
**	    Refine memory usage calculations.
**	31-Aug-2005 (schka24)
**	    Use large bit-vector even for small joins.
**	7-Dec-2005 (kschendel)
**	    Compare-list is built into the node now.
**	22-Nov-2006 (kschendel)
**	    Compute the key size in bytes for the join.
**	6-Mar-2007 (kschendel)
**	    Allow independent read and write (partition) buffer sizes, and
**	    take the preferred size from the config.
**	20-Jun-2007 (kschendel)
**	    Raise the big-join clipping limit a little more, now that
**	    the sorthash pool can exceed 2 Gb.  This clipping limit
**	    really should be a config item, I guess.
**	21-Aug-2007 (jgramling)
**	    Add config parameters for hash join allocation fudge factors:
**	    qef_hashjoin_min for minimum allocation regardless of opf
**	    estimate, _max for upper limit on allocation, _factor for
**	    opf estimate multiplier.
**	23-Aug-2007 (kschendel)
**	    Fix stupid in (unremarked) recent change to allow 2 Mb floor,
**	    it forgot to check whether current allowance was > 2 Mb!
**	25-Sep-2007 (kschendel)
**	    Redo the memory allocation -- again -- because the old way
**	    was comparing apples and oranges in hopes that they were close
**	    enough.  Which they sort of were, but with a bit more work we
**	    can get a much more accurate allocation while still obeying
**	    the hash session limit (qef_hash_mem).  The old way tended to
**	    under-allocate hash joins, causing unnecessary spillage.
**	28-Sep-2007 (kschendel)
**	    Incorporate into new hash reservation framework.
**	7-May-2008 (kschendel)
**	    Re-type all row pointers as qef-hash-dup, which they are.
**	23-Jun-2008 (kschendel)
**	    Include bvsize in hash pointer table, hashjoin now uses bitvector
**	    space for hash table if possible.  Drop factor of 1.1 from
**	    hash pointer table size estimate.
**	    Decide whether row-compression is feasible and set flag.
**	24-Jul-2008 (kschendel)
**	    Re-order one of the availmin/max tests.  With very tight
**	    settings, such as when debugging, we could end up with
**	    min > max.  Nothing bad seems to happen, but it's wrong.
**	24-Oct-2008 (kschendel)
**	    Fix blow-up in the very unlikely case that the initial
**	    partition count and write-buffer sizing loop exits while still
**	    wanting to raise the buffer size.
**	8-May-2009 (kschendel)
**	    Heuristic that raised availmin was hurting very bushy plans
**	    with lots of hash-join concurrency and small joins at the
**	    bottom.  (Such plans often come out of greedy enumeration.)
**	    Memory was being wasted on tiny joins.  Change the heuristic
**	    to only operate if the concurrency level is low, because
**	    otherwise there are too many chances for the optimizer to
**	    guess wrong.  FIXME:  what's really wanted is some kind of
**	    "bmem quality" indicator from the optimizer.  Sometimes it
**	    can be pretty sure of its numbers, and sometimes they are
**	    just wild guesses.
**	28-Jun-2010 (smeke01) b123969
**	    Added diagnostic tracepoint QE72 which facilitates the setting
**	    of partition size and number of partitions respectively for hash
**	    join.
*/
DB_STATUS
qee_hashInit(
	QEF_RCB		*qef_rcb,
	QEE_DSH		*dsh,
	QEN_NODE	*hnode,
	SIZE_TYPE	session_limit,
	bool		always_reserve,
	ULM_RCB		*ulm)
{
    QEF_QP_CB	*qp = dsh->dsh_qp_ptr;
    QEF_CB	*qefcb = qef_rcb->qef_cb;
    QEN_HJOIN	*hjoin = &hnode->node_qen.qen_hjoin;
    QEN_HASH_BASE	*hbase = dsh->dsh_hash[hjoin->hjn_hash];
    i4		rowsz, buildmem;
    i4		htsize, bvsize;
    i4		availmax, availmin;	/* Allocation limits */
    i4		ix, nparts;		/* primes-index limits */
    i4		needmem;
    i4		browsz, prowsz;
    u_i4	wbufsize, rbufsize, wbufmin;
    DB_STATUS	status;
    DB_CMP_LIST	*lastcmp;
    bool	shrink;			/* Termination enforcer */
    i4		tp_csize, tp_pcount; /* used by trace point QE72 */

    /* If we already did this hash join, just return. */
    if (hbase->hsh_msize != 0)
	return (E_DB_OK);

    buildmem = hjoin->hjn_bmem;
    rbufsize = Qef_s_cb->qef_hash_rbsize;
    wbufsize = Qef_s_cb->qef_hash_wbsize;

    /* Compute the available slice given the session limit, and the
    ** concurrency levels at this hash join.
    ** Sometimes the two-pass estimation process can result in a bogus
    ** reduced estimate.  Consider a large hash join with a small hash
    ** join on one side and another large one on the other.  The
    ** concurrency number is 2 regardless of whether we reserve the
    ** small one during the trial pass or not.  Since the large join
    ** must necessarily be able to fit into 1/2 total memory, don't
    ** allow the concurrently reserved memory to artificially make it
    ** look worse.
    */
    ix = hbase->hsh_concur_num;
    if (ix == 0)
	ix = hnode->qen_nthreads;  /* Might not be set if qee direct call */
    if (ix == 0)
	ix = 1;			/* not under parallel query */
    availmax = (session_limit - hbase->hsh_concur_size) / ix;
    if (hbase->hsh_orig_slice == 0)
	hbase->hsh_orig_slice = availmax;
    else if (availmax < hbase->hsh_orig_slice)
	availmax = hbase->hsh_orig_slice;

    /* Set min, max slice limits for adjustment.  On the high side,
    ** limit the join to qef_hashjoin_max.  On the low side, give it
    ** at least qef_hashjoin_min;  and if the available slice is
    ** large enough (meaning > 4Mb), give it at least 2 Mb.  The intent
    ** is to allow a join to run OK even if the bmem estimate is way low.
    */
    availmin = Qef_s_cb->qef_hashjoin_min;
    if (availmin < 7*wbufsize + 50000)
	availmin = 7*wbufsize + 50000;		/* Sanity check minimum */

    if (availmax > Qef_s_cb->qef_hashjoin_max)
	availmax = Qef_s_cb->qef_hashjoin_max;
    if (availmax > 4 * 1024 * 1024 && availmin < 2 * 1024 * 1024
      && hbase->hsh_concur_num <= 2)
	availmin = 2 * 1024 * 1024;
    if (availmax < availmin)
	availmax = availmin;

    /* The target memory range for this join is anywhere between
    ** availmin and availmax.
    */

    browsz = qp->qp_row_len[hjoin->hjn_brow];
    /* OPC should be aligning these, but just make sure */
    hbase->hsh_browsz = browsz = DB_ALIGNTO_MACRO(browsz,QEF_HASH_DUP_ALIGN);
    prowsz = qp->qp_row_len[hjoin->hjn_prow];
    hbase->hsh_prowsz = prowsz = DB_ALIGNTO_MACRO(prowsz,QEF_HASH_DUP_ALIGN);
    rowsz = prowsz;
    if (browsz > rowsz)
	rowsz = browsz;		/* rowsz := the larger of the two */

    /* Make sure either kind of row fits in either buffer.  Make sure
    ** it's page aligned (should be, but make sure).
    ** Note that we may well fool with the wbufsize, so maintain a
    ** minimum below which we can't shrink it.
    */
    while (rbufsize < rowsz)
	rbufsize = rbufsize + HASH_PGSIZE;
    rbufsize = DB_ALIGNTO_MACRO(rbufsize, HASH_PGSIZE);
    while (wbufsize < rowsz)
	wbufsize = wbufsize + HASH_PGSIZE;
    wbufsize = DB_ALIGNTO_MACRO(wbufsize, HASH_PGSIZE);
    wbufmin = DB_ALIGNTO_MACRO(rowsz, HASH_PGSIZE);

    /* Inflate estimate by 20% just because. */
    if (buildmem < MAXI4 / 2)
	buildmem = buildmem + buildmem / 5;	/* Add 20% */
    else
	buildmem = MAXI4 / 2;		/* Avoid absurd sizes */

    if (dsh->dsh_hash_debug)
    {
	TRdisplay("qee_hashInit: node %d %s reserve: avail max=%d min=%d bmem=%d\n",
		hnode->qen_num, always_reserve ? "always" : "trial",
		availmax, availmin, buildmem);
	TRdisplay("qee_hashInit: node %d concur_num=%d, concur_size=%lu\n",
		hnode->qen_num, hbase->hsh_concur_num, hbase->hsh_concur_size);
    }

    /* There is additional memory overhead beyond buildmem, so if
    ** it's already too big, might as well clip buildmem now so that
    ** the main loop doesn't search a whole range of uselessly large
    ** sizes and counts.
    */
    if (buildmem > availmax)
    {
	if (!always_reserve)
	    return (E_DB_INFO);
	buildmem = availmax;
    }

    /* If availmax is way bigger than buildmem, shrink it so that
    ** the join doesn't grab more memory than needed.  This is mainly
    ** an issue with large-ish buildmems and a large availmin..max
    ** range;  not so much with small buildmems.
    ** A reasonable sounding heuristic is:  if buildmem is between
    ** availmin and max, and is at least 3 meg less than availmax,
    ** reduce availmax to half the distance between it and buildmem.
    ** Better heuristics are always welcome...
    ** (Don't clip availmax too closely to buildmem, since buildmem
    ** might be low, and we need some room for the discrete steps
    ** created by the hash bucketing.)
    ** (Don't clip availmax when buildmem is below availmin, since
    ** we'll probably pick a decent starting point anyway.)
    */
    if (buildmem > availmin && buildmem < availmax - 3*1024*1024)
	availmax = (buildmem + availmax) / 2;

    /* A pure heuristic:  if buildmem is small, less than availmin, and
    ** if the default wbufsize is large, 128K or more, pin the initial
    ** wbufsize to 128K (or 64K for truly tiny estimates).
    ** The reason for this is that 256K times the smallest P (which is 7)
    ** will typically fit in availmax and computes out to more than
    ** availmin (usually);  so tiny dozen-row joins end up allocating
    ** 256K partition buffers that don't spill.  Going with 128K is
    ** a bit less wasteful, but still writes fairly well if the
    ** buildmem estimate is way off low.
    */
    if (buildmem < availmin && wbufsize >= 128 * 1024)
    {
	wbufsize = 128 * 1024;
	if (buildmem <= 2048)
	    wbufsize = 64 * 1024;
	if (wbufsize < wbufmin)
	    wbufsize = wbufmin;
    }

    /* The basic memory equation is:
    ** M = 1.1B*Sp + P*(W+BV+4Sf+So) + R
    ** where B = build-rows estimate
    ** P = number of partitions
    ** W = partition size (which is also the write-buffer size)
    ** BV = bit-vector size  (which is a weak function of P)
    ** R = read-buffer size
    ** Sp = sizeof(PTR), Sf = sizeof(file overhead),
    ** So = sizeof(partition overhead)
    **
    ** and also P*W >= B*Sb (Sb = sizeof(build row))
    **
    ** Unfortunately we have two variables to play with and one
    ** equation, plus only certain discrete values of P and W are
    ** allowed, plus the configuration may indicate a preferred W!
    **
    ** So, we proceed thusly:
    ** - choose W as the configuration write-buffer size and an
    **   initial P such that P is the first prime-table entry
    **   with P >= (B*Sb) / W  for the optimizer estimated B.
    ** - compute M as above.
    ** - if M > availmax:  reduce W if it's large, else reduce the
    **   maximum P and repeat.
    ** - if M < availmin:  raise W if it's small, else raise the
    **   minimum P and repeat.
    */

    for (;;)
    {
	ix = 0;
	while (primes[ix] * wbufsize < buildmem && primes[ix] != 0)
	    ix++;
	if (primes[ix] != 0 || wbufsize >= 10 * 1024 * 1024)
	    break;
	/* Initial W is small, or buildmem estimate is very large;
	** raise W so that the initial memory estimate is not needlessly
	** clipped on the low side by a small W.  (e.g. for W=16K,
	** the largest primes entry of about 1000+ would mean that
	** no join would use more than 16 meg unless we adjusted here.)
	** (Note: the test for wbufsize >= 10 meg above is merely to
	** guarantee that the loop exits in all cases.)
	*/
	if (wbufsize == HASH_PGSIZE)
	    wbufsize += HASH_PGSIZE;
	else
	    wbufsize += 2*HASH_PGSIZE;	/* prefer 16K increments */
    }
    nparts = primes[ix];
    /* Extremely unlikely that wbufsize upper limit caused the loop to
    ** exit, but if it did, make sure we have a valid nparts.
    */
    if (nparts == 0)
	nparts = primes[--ix];

    /* Now loop adjusting wbufsize or nparts up or down until we fit
    ** into the range availmin to availmax.
    */

    shrink = FALSE;
    for (;;)
    {
	/* Bitfiltering uses bvsize*8 (bit number) - 1 as an extract,
	** choose things so that bvsize*8-1 is prime.  Just because.
	** Also choose multiples of 8 so that word-clearing tricks can
	** be used to clear the bits initially.
	*/
	bvsize = 513 * 8;
	if (nparts < 400)
	    bvsize = 1532 * 8;		/* Bigger bitfilters... */
	if (nparts < 150)
	    bvsize = 2582 * 8;
	if (nparts < 75)
	    bvsize = 3588 * 8;		/* ... for fewer partitions */
	if (nparts < 50)
	    bvsize = 4353 * 8;
	if (nparts < 20)
	    bvsize = 5123 * 8;
	/* Worst case for hash pointer table is when the join spills and
	** we use all partition bufs + bitvector space for hash space.
	** There's no way of predicting which side (build or probe) will
	** be actually used for the hash table, but the optimizer thinks
	** it's the build side, go along with that for now.
	*/
	htsize = (nparts * (wbufsize+bvsize)) / browsz;
	needmem = (htsize+1) * sizeof(PTR)
		+ nparts * (wbufsize + bvsize + sizeof(QEN_HASH_PART)
				+ 4 * (sizeof(QEN_HASH_FILE) + sizeof(SR_IO)) )
		+ sizeof(QEN_HASH_LINK)
		+ rbufsize;

	if (dsh->dsh_hash_debug)
	{
	    TRdisplay("qee_hashInit: node %d trying W=%d, P=%d, BV=%d, HT=%d, M=%d\n",
		hnode->qen_num, wbufsize, nparts, bvsize, htsize, needmem);
	}

	if (needmem <= availmax && needmem >= availmin)
	    break;
	if (needmem > availmax)
	{
	    /* We need to shrink something if possible.  Prefer shrinking
	    ** the number of partitions, since wbufsize may have been chosen
	    ** to make spill I/O work best.
	    ** If we have to back off the partition buffer size, do it
	    ** in larger steps if the size is big.  Stop trimming at 16K;
	    ** avoid the 8K size unless config specifies it initially.
	    ** Make sure our step-function doesn't violate the min size.
	    */
	    shrink = TRUE;
	    if (ix > 0)
	    {
		nparts = primes[--ix];
		continue;
	    }
	    if (wbufsize == wbufmin)
		break;			/* Already at minimums, too bad */
	    /* Slightly dirty code here sort of assumes PGSIZE is 8K */
	    if (wbufsize > 512*1024)
		wbufsize -= 64*1024;
	    else if (wbufsize > 128*1024)
		wbufsize -= 32*1024;
	    else if (wbufsize > 16*1024 + HASH_PGSIZE)
		wbufsize -= 16*1024;
	    else if (wbufsize > 2*HASH_PGSIZE)
		wbufsize -= HASH_PGSIZE;
	    if (wbufsize < wbufmin)
		wbufsize = wbufmin;
	    /* Might be able to use more partitions with this new size,
	    ** reset to a new nparts guess, it can be counted down again
	    ** as needed.
	    */
	    ix = 0;
	    while (primes[ix] * wbufsize < buildmem && primes[ix] != 0)
		ix++;
	    if (primes[ix] == 0)
		--ix;			/* Running off the end is unlikely! */
	    nparts = primes[ix];
	    /* Fall thru to continue the looping */
	}
	else
	{
	    if (shrink)
		break;			/* Ensure termination of loop */
	    /* Must have had memory set too small, which implies a
	    ** tiny optimizer buildmem estimate and smallish config
	    ** partition buffer size.  First get the buffer size up to
	    ** at least 32K, and then start raising the number of
	    ** partitions.  With the current primes table running up
	    ** to >1000, this procedure works for any hashjoin min
	    ** below about 64 meg... !
	    */
	    if (wbufsize < 32*1024)
		wbufsize += HASH_PGSIZE;
	    else if (primes[ix+1] == 0)
		break;			/* Zowie.  Giant hashjoin min. */
	    else
		nparts = primes[++ix];
	}
    } /* for */

    /* If this was a trial reservation, determine whether or not
    ** this one counts as "optimizer estimate fits in the slice".
    ** If not, return before we do any permanent damage.
    */
    if (!always_reserve && nparts*wbufsize < buildmem)
	return (E_DB_INFO);

    /* Store all the numbers we computed. */

    hbase->hsh_pcount = nparts;
    hbase->hsh_csize = wbufsize;
    hbase->hsh_rbsize = rbufsize;
    hbase->hsh_bvsize = bvsize;
    hbase->hsh_msize = needmem;

    /* Diagnostic trace point QE72 - set hash parameters pn size and pn count */
    if (ult_check_macro(&qefcb->qef_trace, QEF_TRACE_PARAMS_72, &tp_csize, &tp_pcount))
    {
	/* 
	** Overwrite the calculated values only if we have
	** at least minimal values to work with
	*/
	TRdisplay("%@ %^ Trace point QE72: csize was %d, pcount was %d;", hbase->hsh_csize, hbase->hsh_pcount);
	if (tp_csize >= browsz && tp_csize >= hbase->hsh_prowsz && tp_pcount > 1) 
	{
	    hbase->hsh_csize = tp_csize;
	    hbase->hsh_pcount = tp_pcount;
	}
	TRdisplay(" csize now %d, pcount now %d\n", hbase->hsh_csize, hbase->hsh_pcount);
    }

#if 0
/* Note! disable this code until page-compression on work files is actually
** implemented.  (RLE compression may suffice, anyway.)  VMS doesn't support
** non-power-of-2 page sizes at present, and outside of page compression,
** there is no reason to want a larger underlying page size.
** A DIread of 1 256K page or 32 8K pages does the same system call.
*/
    /* Because optional page-compression likes larger pages, try to select
    ** a larger page size.   The page size has to be a multiple of
    ** HASH_PGSIZE and a common factor of both read and write sizes.
    ** The read and write sizes are both a multiple of HASH_PGSIZE
    ** thanks to the forced alignment, so if we compute the
    ** greatest common factor of the two sizes, we'll get the
    ** desired underlying page size for I/O.
    */
**    {
**	i4 a, b, t;
**
**	a = wbufsize;
**	b = rbufsize;
**	/* Avoid a useless division by making a >= b */
**	if (b > a)
**	{
**	    a = b;			/* ie rbufsize */
**	    b = wbufsize;
**	}
**	do
**	{
**	    t = b;			/* usual GCF by division */
**	    b = a % b;
**	    a = t;
**	} while (b != 0);
**	if (a > DI_MAX_PAGESIZE)
**	    a = DI_MAX_PAGESIZE;
**	hbase->hsh_pagesize = a;	/* Set work-file page size */
**    }
#endif
    hbase->hsh_pagesize = HASH_PGSIZE;	/* For now, hard-wired */

    hbase->hsh_brptr = (QEF_HASH_DUP *) dsh->dsh_row[hjoin->hjn_brow];
    hbase->hsh_prptr = (QEF_HASH_DUP *) dsh->dsh_row[hjoin->hjn_prow];
    hbase->hsh_dmhcb = dsh->dsh_cbs[hjoin->hjn_dmhcb];
    hbase->hsh_kcount = hjoin->hjn_hkcount;
    hbase->hsh_streamid = (PTR) NULL;
    hbase->hsh_rbptr = (PTR) NULL;
    hbase->hsh_maxreclvl = 0;
    hbase->hsh_bwritek = 0;
    hbase->hsh_pwritek = 0;
    hbase->hsh_memparts = 0;
    hbase->hsh_rpcount = 0;
    hbase->hsh_bvdcnt = 0;
    hbase->hsh_flags = 0;
    hbase->hsh_nodep = hnode;
    hbase->hsh_count1 = 0;
    hbase->hsh_count2 = 0;
    hbase->hsh_count3 = 0;
    hbase->hsh_count4 = 0;

    /* If outer join, set flags. */
    if (hjoin->hjn_oj)
    switch (hjoin->hjn_oj->oj_jntype)
    {
     case DB_LEFT_JOIN:
	hbase->hsh_flags |= HASH_WANT_LEFT;
	break;

     case DB_RIGHT_JOIN:
	hbase->hsh_flags |= HASH_WANT_RIGHT;
	break;

     case DB_FULL_JOIN:
	hbase->hsh_flags |= (HASH_WANT_LEFT | HASH_WANT_RIGHT);
	break;

     default: break;
    }

    /* Translate new nullable-key flag to old */
    if (hjoin->hjn_nullablekey)
	hbase->hsh_flags |= HASH_CHECKNULL;

    /* If (CO) join and no jqual, we can toss away build rows with
    ** duplicate hash key values.
    */
    if (hjoin->hjn_kuniq
      && hnode->node_qen.qen_hjoin.hjn_jqual == NULL)
	hbase->hsh_flags |= HASH_CO_KEYONLY;

    /* Figure out the size of the keys-to-hash, which we can get from
    ** the key compare list.
    ** Size = (lastOffset + lastLen) - firstOffset
    */
    lastcmp = &hjoin->hjn_cmplist[hjoin->hjn_hkcount-1];
    hbase->hsh_keysz = lastcmp->cmp_offset + lastcmp->cmp_length
		- hjoin->hjn_cmplist[0].cmp_offset;

    /* Decide whether RLL compression should be attempted on either
    ** the build or probe side.  Since row sizes are kept in multiples
    ** of 8 (QEF_HASH_DUP_ALIGN), unless it's likely that we will shrink
    ** the row by at least 8, don't bother.  Use the qef_hash_cmp_threshold
    ** parameter to decide.  (qec assures that it's at least 16.)
    ** FIXME a better determination could probably be done in OPC by
    ** counting columns, widths, and data types, but until some
    ** sensible heuristic presents itself, go with this way.
    */

    if (browsz - hbase->hsh_keysz - QEF_HASH_DUP_SIZE >= Qef_s_cb->qef_hash_cmp_threshold)
	hbase->hsh_flags |= HASH_BUILD_RLL;
    if (prowsz - hbase->hsh_keysz - QEF_HASH_DUP_SIZE >= Qef_s_cb->qef_hash_cmp_threshold)
	hbase->hsh_flags |= HASH_PROBE_RLL;

    /* Allocate array of level 0 partition descriptors, plus the
    ** level 0 recursion link structure and the buffer for assembling 
    ** rows which split across partition blocks.
    **
    ** Note: this stuff is allocated in the DSH stream, not the new 
    ** stream. If recursive partitioning is required, subsequent LINK and
    ** PARTITION structures will be allocated in the new stream, as well
    ** as the partition buffers and hash bucket array.
    */
    ulm->ulm_psize = hbase->hsh_pcount * sizeof (QEN_HASH_PART) +
		sizeof (QEN_HASH_LINK) + rowsz + rowsz;
    status = qec_malloc(ulm);
    if (status != E_DB_OK)
    {
	qef_rcb->error.err_code = ulm->ulm_error.err_code;
	return (status);
    }
    hbase->hsh_workrow = (QEF_HASH_DUP *) ulm->ulm_pptr;
    hbase->hsh_workrow2 = (QEF_HASH_DUP *)&((char *)ulm->ulm_pptr)[rowsz];
    hbase->hsh_currlink = (QEN_HASH_LINK *)&((char *)ulm->ulm_pptr)[2*rowsz];
    hbase->hsh_toplink = hbase->hsh_currlink;
    hbase->hsh_toplink->hsl_partition = 
	(QEN_HASH_PART *)&((char *)hbase->hsh_toplink)[sizeof(QEN_HASH_LINK)];
				/* partition array follows link struct */

    return(E_DB_OK);
}


/*{
** Name: qee_hashaggInit	- Initialize base structure for hash aggregate.
**
** Description:
**	Persistent hash aggregation info is initialized here. More transient
**	stuff (which varies from execution to execution) is initialized
**	in qea_haggf upon the first execution of the action.
**	
** Inputs:
**	qef_rcb			QEF request block
**	dsh			QEF data segment structure
**	qepp			Ptr to action specific action header structure
**	session_limit		Session limit to apply here
**	always_reserve		TRUE to do reservation regardless of size;
**				if FALSE, only reserve if optimizer estimate
**				can be fit into the "availmax" slice.
**	ulm			Ptr to memory mgmt control block
**
** Outputs:
**	qef_rcb
**	    .error.err_code
**	Returns:
**	    E_DB_OK if successful;
**	    E_DB_INFO if trial reservation didn't fit (always-reserve false);
**	    warn or error if a real problem
**
** Side Effects:
**	    [@description_or_none@]
**
** History:
**      26-jan-01 (inkdo01)
**	    Written for hash aggregation.
**	9-feb-01 (inkdo01)
**	    Minor changes to sync with qefdsh.h and qefact.h changes.
**	29-mar-01 (inkdo01)
**	    Various additions to support hash table overflow handling.
**	2-oct-01 (inkdo01)
**	    Minor adjustments to chunk size to allow for pointer chain.
**	4-oct-01 (inkdo01)
**	    Turn keylist into a single aggregate BYTE coplumn for ease
**	    of comparison.
**	31-may-02 (inkdo01)
**	    Change minimum value of hshag_chsize to be hshag_pbsize, so 
**	    overflow routine recurses at the right time.
**	31-aug-04 (inkdo01)
**	    Changes to support global base arrays.
**	14-dec-04 (inkdo01)
**	    Replace unused cmp_flags by cmp_collID (for collations).
**	5-May-2005 (schka24)
**	    Use true pointer size, not align-restrict, for chunk calculations
**	    (align-restrict need not be as big as a pointer).
**	6-Jun-2005 (schka24)
**	    Rework memory allocation estimates to give more memory to
**	    large aggregations.
**	30-Jun-2005 (schka24)
**	    Clip giant hash agg estimate sizes.
**	18-Oct-2005 (hweho01)
**	    Modified the chunk_size assignment, so it will be rounded up  
**          to the sizeof(ALIGN_RESTRICT).
**	7-Dec-2005 (kschendel)
**	    Hash-agg action header now points to (concatenated) key
**	    compare entry, don't need to build it here.
**	27-Jun-2006 (kschendel)
**	    Adjust number of partitions down if dictated by low available mem.
**	29-Aug-2006 (kschendel)
**	    Clipping memory usage at 32M is a bad idea, try something else.
**	9-Jan-2007 (kschendel)
**	    Make sure that the partition buffer can hold an overflow row.
**	6-Mar-2007 (kschendel)
**	    Separate read and write buffer sizes, and take from config.
**	    Lots of wild ad-hoc whirling to try to guess at the best balance
**	    between number of partitions (higher is good because it reduces
**	    the chances of recursive overflow), and the write-buffer size
**	    (which the user may have set for good platform-related reasons),
**	    and still have it all fit into allowed memory.
**	17-Aug-2007 (kschendel)
**	    Confusion between max groups in memory and hash table size,
**	    untangle.
**	21-Sep-2007 (kschendel)
**	    Clip the hash pointer table a little harder for small
**	    result estimates.
**	28-Sep-2007 (kschendel)
**	    Incorporate into new hash-reservation framework.
**	12-Jun-2008 (kschendel)
**	    Fiddle with the number-of-chunks heuristic for small
**	    optimizer estimates and large available-memory slices.
**	7-Jul-2008 (kschendel)
**	    Allocate compression work row, and set the compression-allowed
**	    flag if compression might be useful.
**	24-Oct-2008 (kschendel)
**	    Fix blow-up in the very unlikely case that the initial
**	    partition count and write-buffer sizing loop exits while still
**	    wanting to raise the buffer size.
**	29-Oct-2008 (kschendel)
**	    Ensure that chunk space is at least as large as a write buffer.
*/
DB_STATUS
qee_hashaggInit(
        QEF_RCB		*qef_rcb,
        QEE_DSH		*dsh,
        QEF_QEP		*qepp,
	SIZE_TYPE	session_limit,
	bool		always_reserve,
	ULM_RCB		*ulm)

{
    QEN_HASH_AGGREGATE *haggp = dsh->dsh_hashagg[qepp->u1.s2.ahd_agcbix];
    QEF_QP_CB	*qp = dsh->dsh_qp_ptr;
    QEF_CB	*qefcb = qef_rcb->qef_cb;
    DB_CMP_LIST	*lastcmp;
    i4		orowsz;
    i4		i, ix, htsize;
    i4		aggest, available;
    i4		chunks, chunk_size, rows_per_chunk;
    i4		availchunks, availbufs, availmin;
    i4		htclip;
    i4		needmem, nparts;
    u_i4	rbufsize, wbufsize, wbufmin;	/* Buffer sizes */
    DB_STATUS	status;
    bool	globalbase;
    bool	shrink, shrinksize;

    /* If we already did this one, just return, must be second pass
    ** of a fancy reservation.
    */
    if (haggp->hshag_estmem != 0)
	return (E_DB_OK);

    /* Set buffer sizes (agg. work, hash, overflow). */
    globalbase = (dsh->dsh_qp_ptr->qp_status & QEQP_GLOBAL_BASEARRAY) != 0;
    ix = (globalbase) ? qepp->u1.s2.ahd_agwbuf :
	qepp->ahd_current->qen_base[
		qepp->u1.s2.ahd_agwbuf-ADE_ZBASE].qen_index;
    haggp->hshag_wbufsz = dsh->dsh_qp_ptr->qp_row_len[ix];
    ix = (globalbase) ? qepp->u1.s2.ahd_aghbuf :
	qepp->ahd_by_comp->qen_base[
		qepp->u1.s2.ahd_aghbuf-ADE_ZBASE].qen_index;
    haggp->hshag_hbufsz = dsh->dsh_qp_ptr->qp_row_len[ix];
    ix = (globalbase) ? qepp->u1.s2.ahd_agobuf :
	qepp->u1.s2.ahd_agoflow->qen_base[
		qepp->u1.s2.ahd_agobuf-ADE_ZBASE].qen_index;
    orowsz = dsh->dsh_qp_ptr->qp_row_len[ix];
    /* Must be aligned to HASH-DUP row length multiple (opc should have done
    ** that already), and additionally ensure that it's pointer aligned.
    */
    orowsz = DB_ALIGNTO_MACRO(orowsz, QEF_HASH_DUP_ALIGN);
    orowsz = DB_ALIGN_MACRO(orowsz);
    haggp->hshag_obufsz = orowsz;

    aggest = qepp->u1.s2.ahd_aggest;
    wbufsize = Qef_s_cb->qef_hash_wbsize;
    rbufsize = Qef_s_cb->qef_hash_rbsize;

    /* The read code in qeahaggf expects to do ONE read to complete any
    ** given row, so we need to make sure that the partition size (which
    ** equals the read buffer size) can hold a row.  Otherwise a row might
    ** cross three buffers, which we can't allow.
    ** Apply the same minimum to the write buffers.
    */
    wbufmin = DB_ALIGNTO_MACRO(orowsz, HASH_PGSIZE);
    if (rbufsize < wbufmin)
	rbufsize = wbufmin;
    if (wbufsize < wbufmin)
	wbufsize = wbufmin;

    /* Compute available, taking off a read buffer and a couple hundred
    ** K just for a safety margin.
    */
    i = haggp->hshag_concur_num;
    if (i == 0)
	i = 1;			/* May not be set if direct call from qee */
    available = (session_limit - haggp->hshag_concur_size) / i;
    /* Don't permit the slice to be artificially reduced, see hash join */
    if (haggp->hshag_orig_slice == 0)
	haggp->hshag_orig_slice = available;
    else if (available < haggp->hshag_orig_slice)
	available = haggp->hshag_orig_slice;

    if (dsh->dsh_hash_debug)
    {
	TRdisplay("qee_hashaggInit: %s reserve: session_limit=%lu, avail=%d\n",
		always_reserve ? "always" : "trial", session_limit, available);
	TRdisplay("qee_hashaggInit: aggest=%d, concur_num=%d, concur_size=%lu\n",
		aggest, haggp->hshag_concur_num, haggp->hshag_concur_size);
    }

    available = available - rbufsize - 200000;
    if (available < 256 * 1024)
	available = 256 * 1024;		/* Defend against goofy configs */

    /* Allow for 100 rows per accumulation chunk, unless the output row
    ** is stupidly large.  There's also a link pointer at the front, so
    ** make sure that the size is suitably padded and aligned.
    */
    rows_per_chunk = 100;		/* actually groups per chunk */
    if (haggp->hshag_wbufsz > 32 * 1024)
	rows_per_chunk = 10;
    chunk_size = rows_per_chunk * haggp->hshag_wbufsz + sizeof(PTR);
					/* for the ptr chain thru chunks */
    chunk_size = DB_ALIGN_MACRO(chunk_size);

    /* We'll assign 2/3 of the available space to aggregation chunks,
    ** and 1/3 to overflow buffers.  This is a fairly arbitrary division,
    ** but a reasonable enough one.
    **
    ** The chunk area is chunks plus a hash table, one entry per
    ** result group (inflated by 1.1 for hash dispersion).  So:
    **
    ** M = C * Nc + RPC * Nc * 1.1 * Sptr
    **
    ** M = available chunk memory (2/3 of total available)
    ** C = chunk size, Nc = number of chunks
    ** RPC = rows per chunk, Sptr = sizeof(PTR)
    ** Solving for Nc we get:
    ** Nc = M / (C + RPC * 1.1 * Sptr)
    **
    ** and if this is at least 5 (our bare minimum), and if the resulting
    ** Nc * RPC (total output space) is at least the optimizer estimate,
    ** this reservation "fits".  At least, so far.
    ** (Test against a fudged-upwards estimate, since it's very hard to
    ** get good output estimates from group-by's, and easy to guess low.)
    */
    availchunks = (2 * available) / 3;
    availbufs = available - availchunks;
    chunks = availchunks / (chunk_size + (rows_per_chunk * 11 * sizeof(PTR))/10);
    ++ chunks;				/* For rounding up */
    if (!always_reserve && (chunks < 5 || chunks * rows_per_chunk < 2*aggest))
	return (E_DB_INFO);
    if (chunks < 5)
	chunks = 5;

    /* For very tiny estimates, reduce the number of chunks so that we
    ** don't waste memory.  The optimizer estimates aren't so hot sometimes,
    ** so it's best to be pretty generous.  If our chunk allowance is
    ** more than 20X the optimizer estimate, and more than 3X the write
    ** buffer size, set the chunk allowance to 3X write buffers.
    **
    ** The theory here is that aggregations tend to be either small or
    ** large.  Being just the right in-between size is rare.  So, if the
    ** agg result turns out to be much larger than the optimizer estimate,
    ** it's liable to spill no matter how many chunks we have.  In that
    ** case, giving memory to buffering wins.  *But*, having too little
    ** chunk space leads quickly to wasteful recursion, since we then
    ** have to chop a spilled partition very small to fit.  Going for
    ** three times the requested partition buffer size seems like a
    ** decent compromise.
    ** Don't clip to fewer than 20 chunks here.
    */
    if (chunks > 20
      && chunks * rows_per_chunk > 20 * aggest
      && chunks * chunk_size > 3 * wbufsize)
    {
	chunks = (3 * wbufsize + chunk_size-1) / chunk_size;
	/* Maintain at least 20X the optimizer estimate... */
	if (chunks * rows_per_chunk < 20 * aggest)
	    chunks = (20 * aggest + rows_per_chunk-1) / rows_per_chunk;
	if (chunks < 20)
	    chunks = 20;
    }
    htsize = (chunks * rows_per_chunk * 11) / 10;
    /* Unlike the chunks, the hash ptr table is allocated all at once.
    ** For large available spaces, we could end up with some pretty
    ** monstrous hash ptr tables.  Clip the hash ptr table to 10 meg,
    ** or even smaller if the optimizer estimate is small.
    */
    htclip = 10 * 1024 * 1024;
    if (aggest < 25000)
	htclip = 4 * 1024 * 1024;	/* More off-the-wall numbers */
    if (aggest < 1500)
	htclip = 1 * 1024 * 1024;
    if (htsize * sizeof(PTR) > htclip)
    {
	/* Give the overage to the chunk area */
	chunks += (htsize * sizeof(PTR) - htclip) / chunk_size;
	htsize = htclip / sizeof(PTR);
    }

    /* Having estimated the aggregation chunk work space, now figure out
    ** if there is enough room to reasonably well accomodate overflow,
    ** should that happen.  Recalculate available overflow space since
    ** we may not have eaten up all of the chunk space available.
    ** Unlike hash join, hash aggregation has no configured minimum
    ** size;  for plenty of space, fit things into the range 1/2M .. M.
    */
    availbufs = available - htsize * sizeof(PTR) - chunks * chunk_size;
    availmin = availbufs;
    if (availmin > 6 * 1024 * 1024)
	availmin = availmin / 2;

    for (;;)
    {
	ix = 0;
	while (primes[ix] * wbufsize < availmin && primes[ix] != 0)
	    ix++;
	if (primes[ix] != 0 || wbufsize >= 1 * 1024 * 1024)
	    break;
	/* Initial buffer size is small, or available is very large;
	** raise size so that the initial memory estimate is not needlessly
	** clipped on the low side by a small size.  (e.g. for W=16K,
	** the largest primes entry of about 1000+ would mean that
	** we couldn't use more than 16 meg of overflow-buffer memory.)
	** (Note: the test for wbufsize >= 1 meg above is merely to
	** guarantee that the loop exits in all cases.)
	*/
	if (wbufsize == HASH_PGSIZE)
	    wbufsize += HASH_PGSIZE;
	else
	    wbufsize += 2*HASH_PGSIZE;	/* prefer 16K increments */
    }
    nparts = primes[ix];
    /* Extremely unlikely that wbufsize upper limit caused the loop to
    ** exit, but if it did, make sure we have a valid nparts.
    */
    if (nparts == 0)
	nparts = primes[--ix];

    /* Now loop adjusting wbufsize or nparts up or down until we fit
    ** into availbufs.
    */

    shrink = FALSE;
    shrinksize = FALSE;
    for (;;)
    {
	needmem = nparts * (wbufsize + sizeof(QEN_HASH_FILE) + sizeof(SR_IO));

	if (dsh->dsh_hash_debug)
	{
	    TRdisplay("qee_hashaggInit: avbuf=%d trying W=%d, P=%d, M=%d\n",
		availbufs, wbufsize, nparts, needmem);
	}

	if (needmem <= availbufs && needmem >= availmin)
	    break;
	if (needmem > availbufs)
	{
	    /* We need to shrink something if possible.  Prefer shrinking
	    ** the number of partitions, since wbufsize may have been chosen
	    ** to make spill I/O work best.
	    ** If we have to back off the partition buffer size, do it
	    ** in larger steps if the size is big.  Stop trimming at 16K;
	    ** avoid the 8K size unless config specifies it initially.
	    ** Make sure our step-function doesn't violate the min size.
	    */
	    shrink = TRUE;
	    if (ix > 0)
	    {
		nparts = primes[--ix];
		continue;
	    }
	    if (wbufsize == wbufmin)
		break;			/* Already at minimums, too bad */
	    /* Slightly dirty code here sort of assumes PGSIZE is 8K */
	    shrinksize = TRUE;
	    if (wbufsize > 512*1024)
		wbufsize -= 64*1024;
	    else if (wbufsize > 128*1024)
		wbufsize -= 32*1024;
	    else if (wbufsize > 16*1024 + HASH_PGSIZE)
		wbufsize -= 16*1024;
	    else if (wbufsize > 2*HASH_PGSIZE)
		wbufsize -= HASH_PGSIZE;
	    if (wbufsize < wbufmin)
		wbufsize = wbufmin;
	    /* Might be able to use more partitions with this new size,
	    ** reset to a new nparts guess, it can be counted down again
	    ** as needed.
	    */
	    ix = 0;
	    while (primes[ix] * wbufsize < availmin && primes[ix] != 0)
		ix++;
	    if (primes[ix] == 0)
		--ix;			/* Running off the end is unlikely! */
	    nparts = primes[ix];
	    /* Fall thru to continue the looping */
	}
	else
	{
	    if (shrink)
		break;			/* Ensure loop termination */
	    /* This guess was too small, try to make it larger.  (unless
	    ** we already tried to make it smaller!  which is the shrink
	    ** termination test above.)
	    ** First get the buffer size up to at least 64K, and then
	    ** start raising the number of partitions.  With the current
	    ** primes table running up to >1000, this procedure gives the
	    ** hash aggregation up to some 32 meg of overflow buffering.
	    */
	    if (wbufsize < 64*1024)
		wbufsize += HASH_PGSIZE;
	    else if (primes[ix+1] == 0)
		break;			/* Zowie.  Giant spaces */
	    else
		nparts = primes[++ix];
	}
    } /* for */

    /* If overflow buffering didn't fit, and a trial reservation,
    ** tell caller that it didn't work out.
    */
    if (!always_reserve && (shrinksize || needmem > availbufs) )
	return (E_DB_INFO);

    /* After we're done fooling around with the write-buffer size, make sure
    ** that the total chunk space is at least that large.  The reason for
    ** doing this is that we have to be able to process at least one
    ** NON-spilled buffer's worth of input rows without re-spilling, in
    ** the worst case of entirely distinct rows.  (This equates to the
    ** load2p1 phase at runtime.)  Because of RLL compression, this
    ** adjustment doesn't solve all situations, but it does prevent issues
    ** for the relatively common case of short rows without compression.
    **
    ** This isn't expected, and would happen only for aggs with tiny
    ** (and wrong!) estimates, so don't bother rebalancing everything.
    */
    if (chunks * chunk_size < wbufsize)
	chunks = (wbufsize + chunk_size-1) / chunk_size;  /* rounding up */

    /* Store the various numbers we computed. */
    haggp->hshag_rbsize = rbufsize;
    haggp->hshag_pbsize = wbufsize;
    haggp->hshag_pcount = nparts;
    haggp->hshag_chmax = chunks;
    haggp->hshag_chsize = chunk_size;
    haggp->hshag_grpmax = rows_per_chunk * chunks;
    haggp->hshag_htsize = htsize;

#if 0
/* Note! disable this code until page-compression on work files is actually
** implemented.  (RLL compression may suffice, anyway.)  VMS doesn't support
** non-power-of-2 page sizes at present, and outside of page compression,
** there is no reason to want a larger underlying page size.
** A DIread of 1 256K page or 32 8K pages does the same system call.
*/
    /* Because optional page-compression likes larger pages, try to select
    ** a larger page size.   The page size has to be a multiple of
    ** HASH_PGSIZE and a common factor of both read and write sizes.
    ** The read and write sizes are both a multiple of HASH_PGSIZE
    ** thanks to the forced alignment, so if we compute the
    ** greatest common factor of the two sizes, we'll get the
    ** desired underlying page size for I/O.
    */
**    {
**	i4 a, b, t;
**
**	a = wbufsize;
**	b = rbufsize;
**	/* Avoid a useless division by making a >= b */
**	if (b > a)
**	{
**	    a = b;			/* ie rbsize */
**	    b = wbufsize;
**	}
**	do
**	{
**	    t = b;			/* usual GCF by division */
**	    b = a % b;
**	    a = t;
**	} while (b != 0);
**	if (a > DI_MAX_PAGESIZE)
**	    a = DI_MAX_PAGESIZE;
**	haggp->hshag_pagesize = a;	/* Set work-file page size */
**    }
#endif
    haggp->hshag_pagesize = HASH_PGSIZE;  /* Set work-file page size */

    /* Re-calculate estimated memory for this hash aggregation, mostly
    ** for stats, but also to help out large-operation reallocation in the
    ** case when this allocation was "small".
    */
    haggp->hshag_estmem = htsize * sizeof(PTR)
		+ chunk_size * chunks
		+ orowsz * 3
		+ wbufsize * nparts
		+ nparts * (sizeof(QEN_HASH_FILE) + sizeof(SR_IO))
		+ rbufsize;

    /* Most of the fields in the hash aggregation structure are simply 
    ** set to 0. However, the "by expression" attribute descriptors are
    ** used to generate sort key descriptors and the hash table size 
    ** and chunk sizes are also set. */

    haggp->hshag_htable = NULL;
    haggp->hshag_streamid = NULL;
    haggp->hshag_1stchunk = NULL;
    haggp->hshag_toplink = haggp->hshag_currlink = (QEN_HASH_LINK *) NULL;
    haggp->hshag_openfiles = haggp->hshag_freefile = (QEN_HASH_FILE *) NULL;
    haggp->hshag_lastrow = (QEF_HASH_DUP *) NULL;
    haggp->hshag_rbptr = (PTR) NULL;
    haggp->hshag_dmhcb = dsh->dsh_cbs[qepp->u1.s2.ahd_agdmhcb];
    haggp->hshag_halloc = 0;
    haggp->hshag_challoc = 0;
    haggp->hshag_rcount = 0;
    haggp->hshag_gcount = 0;
    haggp->hshag_ocount = 0;
    haggp->hshag_fcount = 0;
    haggp->hshag_curr_bucket = -1;
    haggp->hshag_flags = 0;

    /* Just one concatenated key, put compare-list entry ptr where
    ** hashagg code can find it easily.
    ** The key size is a bit redundant, but keep it here anyway so that
    ** the actual aggregation code doesn't need to realize that
    ** there is always only one keylist entry.
    */
    haggp->hshag_keycount = 1;
    haggp->hshag_keylist = qepp->u1.s2.ahd_aghcmp;
    haggp->hshag_keysz = haggp->hshag_keylist->cmp_length;

    /* Allocate array of level 0 partition descriptors, plus the
    ** level 0 recursion link structure and the buffers for assembling 
    ** rows which split across partition blocks.
    **
    ** Note: this stuff is allocated in the DSH stream, not the new 
    ** stream. If recursive partitioning is required, subsequent LINK and
    ** PARTITION structures will be allocated in the new stream, as well
    ** as the chunks, partition buffers and hash bucket array.
    **
    ** The work rows come first, and they should be size aligned such that
    ** what follows (the top hash link structure) is aligned.
    */
    ulm->ulm_psize = haggp->hshag_pcount * sizeof (QEN_HASH_PART) +
		sizeof (QEN_HASH_LINK) + orowsz + orowsz;
    status = qec_malloc(ulm);
    if (status != E_DB_OK)
    {
	qef_rcb->error.err_code = ulm->ulm_error.err_code;
	return (status);
    }
    haggp->hshag_workrow = ulm->ulm_pptr;
    haggp->hshag_workrow2 = (char *) haggp->hshag_workrow + orowsz;
    haggp->hshag_currlink = (QEN_HASH_LINK *) ((char *) haggp->hshag_workrow2 + orowsz);
    haggp->hshag_toplink = haggp->hshag_currlink;
    haggp->hshag_toplink->hsl_partition = 
	(QEN_HASH_PART *)&((char *)haggp->hshag_toplink)[sizeof(QEN_HASH_LINK)];
				/* partition array follows link struct */

    /* Decide whether RLL compression should be attempted on spilled
    ** aggregation input rows.  Unlike hash join, hash agg only compresses
    ** for spill, not in memory, and it includes the keys.  Like hash join,
    ** the row length is stored as a multiple of 8 (QEF_HASH_DUP_ALIGN);
    ** so if it's not likely we can shrink the overflow row by at least 8,
    ** don't bother.  Check against the compression threshold parameter.
    ** FIXME a better determination could probably be done in OPC by
    ** counting columns, widths, and data types, but until some
    ** sensible heuristic presents itself, go with this way.
    */

    if (orowsz - QEF_HASH_DUP_SIZE >= Qef_s_cb->qef_hash_cmp_threshold)
	haggp->hshag_flags |= HASHAG_RLL;

    return(E_DB_OK);
}

