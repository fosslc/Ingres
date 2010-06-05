/*
** Copyright (c) 1986, 2005 Ingres Corporation
**
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <adf.h>
#include    <ade.h>
#include    <ulf.h>
#include    <ulm.h>
#include    <scf.h>
#include    <dmf.h>
#include    <dmtcb.h>
#include    <dmrcb.h>
#include    <qefmain.h>
#include    <qsf.h>
#include    <qefrcb.h>
#include    <qefscb.h>
#include    <qefcb.h>
#include    <qefnode.h>
#include    <psfparse.h>
#include    <qefact.h>
#include    <qefqp.h>
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
#include    <me.h>
#include    <bt.h>

/**
**
**  Name: QENORIG.C - originate a tuple from a relation
**
**  Description:
**
**          qen_orig - originate tuples from relations
**
**
**  History:    $Log-for RCS$
**      11-jul-86 (daved)
**          written
**	02-feb-88 (puree)
**	    add reset flag to qen_orig function arguments.
**	19-feb-88 (puree)
**	    make qen_position a global symbol.
**	03-aug-88 (puree)
**	    fix error in ult_check_macro call.
**	19-oct-88 (puree)
**	    modify qen_orig to return E_QE0015_NO_MORE_ROWS if the table
**	    has not been positioned.
**	02-dec-88 (puree)
**	    fix qen_position to reset the count of keys to the actual number
**	    found in the key list.
**	13-apr-90 (nancy)
**	    Bug 20408.  Determine if this is a key by tid.  If so, position
**	    in qen_orig() and/or qen_reset().  Fix made both places.
**      30-apr-90 (eric)
**          added support for qe90, a trace point that keeps track of
**          the cpu, dio, and et that each QEN node uses.
**	18-dec-90 (kwatts)
**	    Smart Disk integration: Added code to check for a Smart Disk program
**	    and if appropriate point at it from the rcb and set position type
**	    to SDSCAN.
**	14-mar-91 (davebf)
**	    Detect adf error occuring in qen_dmr_cx() (b34682).
**      07-aug-91 (stevet)
**          Fix B37995: Added new parameter to qeq_ksort call to indicate 
**          finding the MIN of the MAX keys and MAX of the MIN keys is 
**	    not required when calling from qen_reset.
**      09-sep-91 (stevet)
**          Fix B39611: This bug is a side effect of bug fix B37995.
**          dmr_position is being called twice. Added code to skip qen_reset
**          if call with qen_status = QEN0_INITIAL because reset is already
**          done in qeq_validate.
**	11-mar-92 (jhahn)
**	    Added support for QEF interrupts.
**	    Added call to CSswitch.
**	08-dec-92 (anitap)
**	    Included <qsf.h> and <psfparse.h> for CREATE SCHEMA.
**	18-dec-1992 (kwatts)
**	    Smart Disk enhancements. More info to prepare for the DMF
**	    position call.
**	17-may-93 (nancy)
**	    Modified qen_ecost_end() and qen_bcost_begin() call to CSstatistics
**	    for session stats rather than server stat.
**	17-may-93 (nancy)
**	    Bug 51853. Modified qen_orig() to detect a qualification failing
**	    when this is a tid qualification ANDed with something else.
**      20-jul-1993 (pearl)
**          Bug 45912.  In qen_reset(), change call to qeq_ksort to pass
**          dsh instead of dsh_key.
**      14-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
**      21-aug-1995 (ramra01)
**          Added new node for Simple aggregate handling with a base table
**          only
**	7-nov-95 (inkdo01)
**	    Changes to replace QEN_ADF structure instances by
**	    pointer in QEN_ORIG.
**      16-jul-96 (sarjo01)
**          Bug 76243: qen_origagg(): copy ADF warning counters from RCB
**	    use by DMF to the one passed in from QEF
**	30-jul-96 (nick)
**	    Remove direct calls to DMF as this is a no-no.
**	    Pick ADI_WARN up from the DMR_CB result area.
**      07-aug-96 (wilan01)
**          include <me.h> to ensure that the MEcopy macro is included 
**      07-aug-96 (wilan01)
**          include <me.h> to ensure that the MEcopy macro is included 
**	02-mar-97 (hanal04)
**	    Picking ADI_WARN up from the DMR_CB result area (re: 30-jul-96)
**          causes SIGSEGV when we are not performing aggregates because in
**          these cases rcb->qef_adf_cb is null. (Bug# 81349).
**      14-may-97 (dilma04)
**          Cursor Stability Project:
**          Set DMR_SORT flag if getting tuples to append them to a sorter.
**      15-may-2000 (stial01)
**          Remove Smart Disk code
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	23-feb-04 (inkdo01)
**	    Changed qef_rcb->error to dsh->dsh_error for || thread safety
**	    (and others).
**	20-Jul-2005 (schka24)
**	    Overhaul partition calculations to fix bugs in grouped plus
**	    qualified access, and to better allow physical partitions to
**	    be distributed among threads.
**	04-oct-2005 (abbjo03/schka24)
**	    In qen_orig(), if qen_position() returns something other than no
**	    more rows, return it as an error. 
**	14-Dec-2005 (kschendel)
**	    Move ADF CB pointer to dsh.
**	    b81349 (above) was actually caused by using rcb instead of
**	    qef cb (or now, dsh) for getting at the adf cb.
**	13-Jan-2006 (kschendel)
**	    Revise calls to evaluate CX's.
**	16-Jun-2006 (kschendel)
**	    Part-info stuff moved around for multidim partition qualification,
**	    make needed changes here.
**	27-Jul-2007 (kschendel) SIR 122513
**	    Partition qualification revised again, this time to include
**	    restrictions produced by joins above us in the plan.
**	11-Apr-2008 (kschendel)
**	    Use utility for setting up DMF qualification.
**	4-Jun-2009 (kschendel) b122118
**	    Use slightly faster/more convenient dsh test for qe90, and
**	    remove pointless error returns from cost begin/end.  Minor
**	    streamlining.
**	11-May-2010 (kschendel) b123565
**	    Rename dsh-root to dsh-parent.
**/

/* Local static procedure declarations */

static DB_STATUS qen_reset(
	QEE_DSH *dsh,
	QEN_NODE *node,
	QEN_STATUS *qen_status );

/* Advance to next physical partition of PC-join group */
static i4 advance_group(QEN_STATUS *qen_status,
	QEN_PART_INFO *partp,
	PTR groupmap);

/* Advance to start of next PC-join group */
static bool advance_next_group(QEN_STATUS *qen_status,
	QEN_PART_INFO *partp);

/* Set up grouped access physical partition map for thread */
static void setup_groupmap(QEN_STATUS *qen_status,
	QEN_PART_INFO *partp,
	PTR qpmap,
	PTR groupmap);

/*{
** Name: QEN_BCOST_BEGIN	- Begin the cost calculation for a QEN_NODE
**
** Description:
[@comment_line@]...
**
** Inputs:
[@PARAM_DESCR@]...
**
** Outputs:
[@PARAM_DESCR@]...
**
**	Returns:
**	    {@return_description@}
**	Exceptions:
**	    [@description_or_none@]
**
** Side Effects:
**	    [@description_or_none@]
**
** History:
**      20-jan-90 (eric)
**          created
**	18-may-93 (nancy)
**	    Change parameter to CSstatistics to get session stats.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	1-Jul-2008 (kschendel) b122118
**	    Remove pointless error return, this is just an informational
**	    tracing aid.
[@history_template@]...
*/
void
qen_bcost_begin(
QEE_DSH	    *dsh,
TIMERSTAT   *timerstat,
QEN_STATUS  *qenstatus )
{
    SYSTIME stime;

    if (CSstatistics(timerstat, (i4) 0) != OK)
    {
	return;
    }

    TMnow(&stime);
    timerstat->stat_wc = stime.TM_secs;

    if (qenstatus != NULL)
    {
	qenstatus->node_nbcost += 1;
    }

    return;
}

/*{
** Name: QEN_ECOST_END	- End the cost calculation for a QEN_NODE
**
** Description:
{@comment_line@}...
**
** Inputs:
[@PARAM_DESCR@]...
**
** Outputs:
[@PARAM_DESCR@]...
**
**	Returns:
**	    {@return_description@}
**	Exceptions:
**	    [@description_or_none@]
**
** Side Effects:
**	    [@description_or_none@]
**
** History:
**      20-jan-90 (eric)
**          created
**	18-may-93 (nancy)
**	    Change parameter to CSstatistics to get session stats.
**	1-Jul-2008 (kschendel) b122118
**	    Remove pointless error return, this is just an informational
**	    tracing aid.
[@history_template@]...
*/
void
qen_ecost_end(
QEE_DSH	    *dsh,
TIMERSTAT   *btimerstat,
QEN_STATUS  *qenstatus )
{
    TIMERSTAT	    etimerstat;
    SYSTIME	    systime;

    if (CSstatistics(&etimerstat, (i4) 0) != OK)
    {
	return;
    }

    TMnow(&systime);
    etimerstat.stat_wc = systime.TM_secs;

    if (qenstatus == NULL)
    {
	btimerstat->stat_cpu = etimerstat.stat_cpu - btimerstat->stat_cpu;
	btimerstat->stat_dio = etimerstat.stat_dio - btimerstat->stat_dio;
	btimerstat->stat_wc = etimerstat.stat_wc - btimerstat->stat_wc;
    }
    else
    {
	qenstatus->node_cpu += (etimerstat.stat_cpu - btimerstat->stat_cpu);
	qenstatus->node_dio += (etimerstat.stat_dio - btimerstat->stat_dio);
	qenstatus->node_wc += (etimerstat.stat_wc - btimerstat->stat_wc);
	qenstatus->node_necost += 1;
    }

    return;
}

/*** Name: QEN_ORIG	- originate tuples from relations
**
** Description:
**      Tuples are read, restricted and projected from the
**  relation into a row buffer. 
**
**  Thus, this node will make use of keys but will always sequentially
**  scan the relation (for each key).
**
**  The relation being accessed has already been opened and the first
**  key has been positioned.   (False for partitioned tables.)
**
**	In the presence of partitions (and possibly threads), there are
**	some fairly complex decisions to make so that the right threads
**	read the proper partitions.  There are fundamentally two cases,
**	PC joining or non-PC-joining.
**
**	PC (partition compatible) joining means that the tables to be joined
**	are both partitioned in some compatible way on the joining column(s),
**	such that instead of one big join we can do N mini-joins.  In this
**	case, the orig has to produce the appropriate bits of the source
**	table, separated by end-of-group indicators.  The partition group
**	count (part_gcount) is the number of LOGICAL partitions (of the
**	joining dimension, part_pcdim) that we are to read before
**	returning an end-of-group indicator to the parent join.  Since it's
**	necessary to feed the parent join the entire (series of) logical
**	partitions, we have to hand partitions to threads based on logical
**	partitioning.
**
**	Part_gcount in the PC-join case is always 1 unless both tables
**	are HASH partitioned.  Due to the way hash partitioning works,
**	when gcount > 1, the group is NOT consecutive logical partitions.
**	Instead, we read each (nparts/gcount)'th logical partition.
**	For instance, given a 12-partition (hash) table and a part_gcount
**	of 3, the first group is logical partitions 0, 4, 8;  the second
**	group is LP's 1, 5, 9;  the third group is 2, 6, 10;  and the
**	last group is 3, 7, 11.
**
**	Note that in the PC-join case, the join and both orig's will execute
**	in the same thread -- i.e. EXCH nodes are not currently placed below
**	PC-joining nodes.  This simplifies the task of deciding which logical
**	and ultimately physical partitions to select.
**
**	In the non-PC-joining case, we're simply reading some or all
**	partitions.  The partitions to be read can be divvied up among the
**	available threads in whatever manner we like.  The partitions to read
**	might be indicated by a bit-map showing which (physical) partitions
**	qualified.
**
**	It's possible to have both partition qualification and PC joining.
**	For that case, we need to first decide which logical partitions should
**	be returned, and then apply the physical partition bit-map.
**
** Inputs:
**      node                            the orig node data structure
**	rcb				request block
**	    .qef_cb.qef_dsh		the data segment header
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
**	    none
**
** History:
**      11-jul-86 (daved)
**          written
**      30-jul-86 (daved)
**          Add support for keys
**	02-feb-88 (puree)
**	    Add reset flag to the function arguments.
**	03-aug-88 (puree)
**	    Fix error in ult_check_macro call.
**	09-sep-88 (puree)
**	    If specified, also return tid in the action's row buffer.
**	    This is to support delete/replace cursor by tid from the
**	    secondary indices.
**	19-oct-88 (puree)
**	    If a table is not positioned, it's the same as no more
**	    rows condition.  The table not position can only happen
**	    (other than an internal bug) when the qualification cannot
**	    be met when the table is opened.  So it is fair to say that
**	    there are no more rows.
**	28-sep-89 (eric)
**	    Incremented qen_status->node_rcount to keep track of how many
**	    rows this node returns. This will be used to fix the Sybase bug.
**	13-apr-90 (nancy)
**	    Bug 20408.  Determine if this is a key by tid.  If so, position
**	    here. 
**	14-mar-91 (davebf)
**	    Detect adf error occuring in qen_dmr_cx() (b34682).
**      09-sep-91 (stevet)
**          Fix B39611: This bug is a side effect of bug fix B37995.
**          dmr_position is being called twice. Added code to skip qen_reset
**          if call with qen_status = QEN0_INITIAL because reset is already
**          done in qeq_validate.
**	11-mar-92 (jhahn)
**	    Added support for QEF interrupts.
**	    Added call to CSswitch.
**	17-may-1993 (nancy)
**	    Bug 51853.  A tid qualification ANDed with another qual returns
**          a row even if the qual fails.  Handle condition where the cx
**	    does not return TRUE and this is a key_by_tid -- break out of 
**  	    loop and set error code to E_QE0015_NO_MORE_ROWS;
**      19-jun-95 (inkdo01)
**          Collapsed orig_tkey/ukey nat's into orig_flag flag word.
**	7-nov-95 (inkdo01)
**	    Changes to replace QEN_ADF structure instances by
**	    pointer in QEN_ORIG.
**	24-apr-96 (inkdo01)
**	    Very minor fix for SEGV in "bugtest".
**	12-feb-97 (inkdo01)
**	    Changes to support MS Access OR transformed in-memory constant
**	    tables. Entirely new stream of logic to read these guys
**	    (thankfully, very simple since they're in memory already).
**      14-may-97 (dilma04)
**          Cursor Stability Project:
**          Set DMR_SORT flag if getting tuples to append them to a sorter. 
**	21-jul-98 (inkdo01)
**	    Added support for topsort-less descending sorts.
**	12-nov-98 (inkdo01)
**	    Added support of base table index only access (Btree).
**	20-nov-03 (inkdo01)
**	    Add support for "open" call to allow parallel query initialization.
**	29-dec-03 (inkdo01)
**	    DSH is now parameter, "function" replaces "reset".
**	5-jan-04 (inkdo01)
**	    Added logic to process arrays of partition descriptors.
**	3-feb-04 (inkdo01)
**	    Changed TIDs to TID8s for table partitioning. Added logic to
**	    process individual partitions of partitioned table.
**	25-feb-04 (inkdo01)
**	    Replace qef_cb by dsh in dmr_q_arg for thread safety.
**	27-feb-04 (inkdo01)
**	    Changed dsh ptr to array of QEN_STATUS structs to ptr to array
**	    of ptrs.
**	9-mar-04 (inkdo01)
**	    Add logic to split partitions over multiple threads.
**	11-mar-04 (inkdo01)
**	    Coupla fixes to end-of-partition code for keyed scans of
**	    partitioned tables.
**	12-mar-04 (inkdo01)
**	    Fix to support bigtids on byte-swapped machines.
**	17-mar-04 (toumi01)
**	    Add support for "close" call to allow parallel query termination.
**	19-mar-04 (inkdo01)
**	    Change TID manipulation for bigtids on byte-swapped machines.
**	1-apr-04 (inkdo01)
**	    Use QEN16_ORIG_FIRST flag to issue reset call before 1st row.
**	    Other logic changes to guide through 1st row retrieval in the
**	    presence of || query logic.
**	6-apr-04 (inkdo01)
**	    Minor tweak to start up orig nodes inside cursors.
**	8-apr-04 (inkdo01)
**	    Added dsh parm to QEF_CHECK_FOR_INTERRUPT macro.
**	12-apr-04 (inkdo01)
**	    Support DMR_UNFIX call to liberate pages in buffer after 
**	    scan completion.
**	13-apr-04 (inkdo01)
**	    Generalized 1:n partition logic - for partition compatible joins.
**	27-apr-04 (inkdo01)
**	    Add support for PC joins with partition qualification.
**	10-may-04 (inkdo01)
**	    Fix bad BTcount's for partition processing.
**	19-may-04 (inkdo01)
**	    Couple of || query/partitioning fixes. Also added some xDEBUGed 
**	    displays to help with || query debugging.
**	1-Jun-2004 (schka24)
**	    Init DSH qen-adf ptr sooner to cover all cases.
**	8-june-04 (inkdo01)
**	    More tinkering to make partitioning/threading logic work.
**	22-june-04 (inkdo01)
**	    Add logic to allocate and format DSH key structures, if needed.
**	24-june-04 (inkdo01)
**	    Fix bug in reset processing (due to || changes in qeqvalid.c).
**	8-Jul-2004 (schka24)
**	    Still some tid byte-ordering confusion on x86, make tid handling
**	    more portable.
**	20-july-04 (inkdo01)
**	    Add logic to send resets through partition positioning code
**	    if not because of end of previous partition group.
**	28-Jul-2004 (jenjo02)
**	    If OPEN on Child DSH and table has not been opened for
** 	    the Child thread, call qeq_part_open() to instantiate
**	    the table.
**	30-Jul-2004 (jenjo02)
**	    During OPEN, if keyed QP and keys haven't been set up yet,
**	    call qen_keyprep to make a thread-safe copy of the
**	    keys.
**	04-Nov-2004 (jenjo02)
**	    Only CSswitch on root DSH or if not OS-threaded.
**	22-dec-04 (toumi01)
**	    For in-memory constant ORIG nodes the dmr_cb is really a
**	    (QEE_MEM_CONSTTAB*) and if we use it as a normal dmr_cb for
**	    node open we'll trash some memory. If memtab is set we can
**	    skip the parallel query node open logic. Fixes bug 113675.
**	5-jan-05 (inkdo01)
**	    Fix bug when partition count isn't even multiple of threads.
**	18-Apr-2005 (thaju02)
**	    If qen_position() fails due to deadlock (DM0042), goto exit.
**	    (B114338)
**	24-May-2005 (schka24)
**	    Fix logic that determines whether thread has no partitions
**	    to work on.
**	18-Jul-2005 (schka24)
**	    Physical-partition num in qen-status moved, adjust here.
**	20-Jul-2005 (schka24)
**	    Overhaul partition calculations to fix bugs in grouped plus
**	    qualified access, and to better allow physical partitions to
**	    be distributed among threads.
**	13-Dec-2005 (kschendel)
**	    Potential for tid qualification against a partitioned table to
**	    not work, fix.  (qual-by-tid has to run here, not in DMF.)
**	16-Feb-2006 (kschendel)
**	    Return after close, don't keep going.
**	27-dec-2006 (kibro01) b117403
**	    If using unique key but not the tid, it is still valid to say there
**	    are no more rows if the current row fails qualification (because
**	    there is a tid-based qualification, but not equality and therefore
**	    not used as a key).
**	13-Feb-2007 (kschendel)
**	    Replace CSswitch with better cancel check.
**	27-Jul-2007 (kschendel) SIR 122513
**	    Rework partition qualification again, include join quals.
**	23-Aug-2007 (kibro01) b118720
**	    If we received a user error from a function below, mark in the
**	    dsh that the error has already been reported so we don't get
**	    the worrying DM008A and QE007A errors later on in the log.
**	16-Apr-2008 (kschendel)
**	    No more qen-dmr-cx, minor fixes here.
**	1-Jul-2008 (kschendel) SIR 122513
**	    Try to streamline the most common get-next-row case.
**	8-Sep-2008 (kibro01) b120693
**	    Remove unused action parameter from qeq_part_open
**	17-Dec-2008 (kibro01) b121372
**	    If performing an IN list on tids, don't advance along the OR
**	    list before processing the first one.
**	9-Jan-2009 (kibro01) b121483
**	    Another bug with IN-list on tids involving another check which
**	    also restricts the tid but doesn't match the first of the IN-list.
**	4-Jun-2009 (kschendel) b122118
**	    DMF memory problems caused by QEF not closing partitions.
**	    Close partitions here after we're done with them.
**	4-Sep-2009 (smeke01) b122399
**	    Ensure that the correct qen_position() code is called for the 
**	    first row when the partition-grouping code is invoked.
**	14-Oct-2009 (kschendel) SIR 122513
**	    Include a new bool introduced by kibro01 into the setup after
**	    fast-path decides to drop back to standard orig.
*/

DB_STATUS
qen_orig(
QEN_NODE           *node,
QEF_RCB            *rcb,
QEE_DSH		   *dsh,
i4		    function )
{
    QEF_CB	    *qef_cb;
    PTR		    *cbs = dsh->dsh_cbs;
    DMR_CB	    *dmr_cb = (DMR_CB*) cbs[node->node_qen.qen_orig.orig_get];
    DMT_CB	    *dmt_cb;
    QEN_PART_INFO   *partp;
    QEE_MEM_CONSTTAB	*cnsttab_p;
    QEN_TKEY	    *qen_tkey;

    char	    *tidoutput;
    DB_TID8	    bigtid;
    QEE_XADDRS	    *node_xaddrs = dsh->dsh_xaddrs[node->qen_num];
    QEN_STATUS	    *qen_status = node_xaddrs->qex_status;
    DB_STATUS	    status;
    
    bool	    reset;
    bool	    key_by_tid, positioned_by_tid;
    bool	    memtab;
    bool	    readback;
    bool	    bcost;
    i4		    ppartno;
    i4		    pcount;
    i4		    offset = node->node_qen.qen_orig.orig_tid;
    i4		    threadno;
    i4	    val1;
    i4	    val2;
    i4		    i;
    i4		    node_orig_flag;
    PTR		    qpmap;		/* Qualified partition bit-map */
    i4		    totparts;		/* Length of same in bits */
    PTR		    groupmap;		/* Thread's group partition bit-map */
    TIMERSTAT	    timerstat;


#ifdef xDEBUG
    /* (VOID) qe2_chk_qp(dsh); */
#endif

    /* Attempt fast-path for simple get next row, before doing a bunch of
    ** extra setup and testing.  If this doesn't work out we'll jump off to
    ** the normal dmr-get error path.
    ** Fast-path can only be attempted if:
    ** - there's no caller supplied function request
    ** - it's not a memory constant table
    ** - QP stats (ie qe90) are not requested
    ** - it's not read-backwards or position-by-tid
    ** - there is no TID qualification (ie any qualification is being done
    **   in DMF and it's already set up)
    ** - the node isn't flagged UKEY (a single unique key, only one row
    **   to get anyway, no point in fast-next)
    **
    ** It is however permitted to be a partitioned table, to return
    ** computed tids into the output row, and to compute function attrs.
    */

    if (qen_status->node_status == QEN_ORIG_FASTNEXT)
    {
	if (function != 0)
	{
	    /* oops, can't get away with fast-next, reset to "executing" */
	    qen_status->node_status = QEN1_EXECUTED;
	    /* Fall thru to mainline */
	}
	else
	{
	    /* Attempt a quick get-next using the dmr flags from the
	    ** prior (successful) get.
	    */
	    if (QEF_CHECK_FOR_INTERRUPT(dsh->dsh_qefcb, dsh) == E_DB_ERROR)
		return (E_DB_ERROR);
	    dsh->dsh_error.err_code = E_QE0000_OK;
	    dmr_cb->error.err_code = E_DM0000_OK;
	    /* inplace cursor update can change the flags in the dmrcb, use
	    ** the saved ones from when we decided to go into fast-next mode.
	    */
	    dmr_cb->dmr_flags_mask = qen_status->node_u.node_orig.node_dmr_flags;
	    status = dmf_call(DMR_GET, dmr_cb);

	    if (status != E_DB_OK)
	    {
		/* oops... perhaps we reached end of table or partition, or
		** something else went wrong.  In any case, flag so that we
		** fall thru all the bits that follow until we get to the
		** regular get-next error handler.
		** By the way, the absurdity of this setup just goes to show
		** what a lava flow the mainline orig has turned into...
		*/
		qen_status->node_status = QEN1_EXECUTED;
		reset = FALSE;
		bcost = FALSE;
		partp = node->node_qen.qen_orig.orig_part;
		qef_cb = dsh->dsh_qefcb;
		node_orig_flag = node->node_qen.qen_orig.orig_flag;
		memtab = FALSE;
		readback = FALSE;
		if (partp != NULL)
		{
		    totparts = partp->part_totparts;
		    qpmap = groupmap = NULL;
		    if (partp->part_groupmap_ix >= 0)
			groupmap = dsh->dsh_row[partp->part_groupmap_ix];
		    if (node_xaddrs->qex_rqual != NULL)
			qpmap = node_xaddrs->qex_rqual->qee_lresult;
		} /* partition variables setup */
		/* qen_tkey may not have a valid address. be careful when using */
		if (node->node_qen.qen_orig.orig_pkeys)
		    qen_tkey = (QEN_TKEY*) cbs[node->node_qen.qen_orig.orig_keys];
		else
		    qen_tkey = NULL;
		key_by_tid = FALSE;
		positioned_by_tid = FALSE;
		/* FIXME get the orig mainline unclogged enough to make this
		** goto into a status or function flag with fall-thru!  and
		** get rid of most or all of the above poop!
		*/
		goto dmr_get_error;
	    }

	    /* If the tid is needed for subsequent qualification or 
	    ** for delete/replace cursor by tid, generate it now.
	    */
	    if (offset > -1)
	    {
		tidoutput = (char *) dmr_cb->dmr_data.data_address + offset;
		bigtid.tid_i4.tid = dmr_cb->dmr_tid;
		bigtid.tid_i4.tpf = 0;
		if (node->node_qen.qen_orig.orig_part)
		    bigtid.tid.tid_partno = qen_status->node_ppart_num;
		I8ASSIGN_MACRO(bigtid,*tidoutput);
	    }
	    /* generate function attrs if needed */
	    if (node_xaddrs->qex_fatts)
	    {
		status = qen_execute_cx(dsh, node_xaddrs->qex_fatts);
		if (status != E_DB_OK)
		    return (status);
	    }
	    return (E_DB_OK);
	}
    }

    /* Ok, fast-path didn't work out, set up more variables first */

    status = E_DB_OK;
    reset = (function & FUNC_RESET) != 0;
    bcost = FALSE;
    partp = node->node_qen.qen_orig.orig_part;
    qef_cb = dsh->dsh_qefcb;

    /* If this is a MID-OPEN, don't do anything, but remember that
    ** all the "open" stuff still has to run.  The next call, the one
    ** that really wants a row, will probably show "reset" but this
    ** way makes sure.
    ** We want to delay the actual open until the higher level of the
    ** plan has a chance to run -- there might be partition qualification
    ** being done for us.
    ** (top-open expects a row back, so that one has to go through.
    ** We're at the top of the subplan in that case anyway.)
    */
    if (function & MID_OPEN && (function & TOP_OPEN) == 0)
    {
	qen_status->node_status_flags |= QEN_ORIG_NEEDS_OPEN;
	return (E_DB_OK);
    }

    /* Set up flags */

    node_orig_flag = node->node_qen.qen_orig.orig_flag;
    memtab = ((node_orig_flag & ORIG_MCNSTTAB) != 0);
    readback = ((node_orig_flag & ORIG_READBACK) != 0);

    /* Additional variable setup if partitioned */

    if (partp != NULL)
    {
	totparts = partp->part_totparts;
	qpmap = groupmap = NULL;
	if (partp->part_groupmap_ix >= 0)
	    groupmap = dsh->dsh_row[partp->part_groupmap_ix];
	if (node_xaddrs->qex_rqual != NULL)
	    qpmap = node_xaddrs->qex_rqual->qee_lresult;
    } /* partition variables setup */

    /* Do open processing, if required.
    ** Only open if we haven't already, or if resetting (and not resetting
    ** after an end-of-group, which may cause an upper join node to do
    ** a reset to prepare itself for the next group.)
    */
    if (!memtab && (
	qen_status->node_status_flags & QEN_ORIG_NEEDS_OPEN ||
	( function & TOP_OPEN && 
	 (qen_status->node_status_flags & QEN1_NODE_OPEN) == 0) ||
	( function & FUNC_RESET &&
	 ((qen_status->node_status_flags & QEN64_ORIG_NORESET) == 0)) ) )
    {
	/* Indicate that we've opened the node, and haven't done the
	** first real fetch.  Also set "noreset" since sometimes
	** callers like to feed in multiple resets, or an open and
	** a reset, and there's no need to reinitialize twice in a row.
	** (note this is a "set", not an OR)
	*/
	qen_status->node_status_flags = (QEN1_NODE_OPEN |
				QEN16_ORIG_FIRST | QEN64_ORIG_NORESET);
	/* Clear mostly out of paranoia in case unpartitioned */
	qen_status->node_u.node_orig.node_groupsleft = 0;
	qen_status->node_u.node_orig.node_partsleft = 0;

	/* For partitioned tables, the following logic must choose 
	** the first partition to process. There are numerous orthogonal
	** variables that determine how this is done:
	**  1. is there a partition qualification bit map or not?
	**  2. is this under a grouped partition compatible join or not?
	**  3. is this single or multi-threaded?
	** In addition to setting the first partition for processing by
	** this thread, the code sets various values that guide later
	** logic to select subsequent partitions for processing.
	*/
	ppartno = -1;
	if (partp != (QEN_PART_INFO *) NULL)
	{
	    i4 lpartno;
	    i4 parts_per_thread;
	    QEE_PART_QUAL   *rqual = node_xaddrs->qex_rqual;

	    /* First select no. of partitions to be read (logical
	    ** partitions if grouped access, otherwise physical partitions).
	    ** If we're doing partition qualification against this table,
	    ** here is where we apply any restrictions computed by joins
	    ** higher up in the query plan.
	    ** Note qpmap == rqual->qee_lresult.
	    */
	    if (rqual != NULL)
	    {
		QEE_PART_QUAL *jq;
		MEcopy(rqual->qee_constmap, rqual->qee_mapbytes, qpmap);
		jq = rqual->qee_qnext;
		while (jq != NULL)
		{
		    if (jq->qee_qualifying)
			BTand(totparts, jq->qee_lresult, qpmap);
		    jq = jq->qee_qnext;
		}
	    }
	    threadno = dsh->dsh_threadno;
	    if (threadno == 0) threadno = 1;	/* non-threaded */
	    if (partp->part_gcount >= 1)
		pcount = partp->part_pcdim->nparts;
	    else if (qpmap == NULL)
		pcount = totparts;
	    else
		pcount = BTcount(qpmap, totparts);

	    /* Split partitions over threads - if more threads than
	    ** partitions, signal quick exit from the leftovers.
	    ** We split rounded up.  For example, say pcount is 6,
	    ** 5 threads.  We can a) give each thread 1 and the last
	    ** thread an extra, or b) give 3 threads 2 partitions and
	    ** kiss off two threads.  The slowest thread is the same
	    ** either way, so we'll opt to keep it balanced.
	    ** The way we do it, parts-per-thread * threads >= pcount.
	    ** Also, all but the last thread will get parts-per-thread to do.
	    ** If grouping, we hand out groups-per-thread.
	    */
	    i = partp->part_gcount;	/* lparts-per-group... */
	    if (i <= 0) i = 1;		/* or 1 if ungrouped */
	    parts_per_thread = ((pcount/i) + partp->part_threads - 1) / 
				partp->part_threads;	/* rounded up */

	    lpartno = (threadno - 1) * parts_per_thread * i;

	    /* Check for more threads than needed. */
	    if (lpartno >= pcount)
		lpartno = pcount = -1;		/* ppartno still -1 */

	    if (pcount > 0)	/* thread has something to do */
	    {
		if (partp->part_gcount <= 0)
		{
		    /* This is physical partitions - lpartno is really
		    ** starting physical partition no.
		    */
		    ppartno = lpartno;
		    lpartno = -1;
		    qen_status->node_u.node_orig.node_partsleft = parts_per_thread;

		    /* If a qualification bitmap is around, skip to the
		    ** start bit for this thread.  Do this the brute force
		    ** way, by moving past bits assigned to lower-numbered
		    ** threads.  (in effect recalculating ppartno.)
		    ** Note need to loop one extra time because the first
		    ** time around gets the bit-next started up.
		    */
		    if (qpmap != NULL)
		    {
			ppartno = -1;
			for (i = (threadno-1)*parts_per_thread; i >= 0; --i)
			{
			    ppartno = BTnext(ppartno, qpmap, totparts);
			    if (ppartno < 0)
				break;
			}
		    }
		}
		else
		{
		    /* If grouping, compute ppartno from lpartno.
		    ** Note that parts_per_thread here is really groups.
		    ** Threads read consecutive groups, which start at
		    ** consecutive lpartno positions, so we need to adjust
		    ** the computed lpartno to remove the extra factor
		    ** of lp's-per-group introduced above.
		    ** Note that both qualified and grouped partitioning
		    ** logic may lead to some threads being wasted on empty
		    ** partitions. If the qualification partition dimension
		    ** and the grouping partition dimension are the same, 
		    ** qualification must be on the join column and we might
		    ** be able to assume the other table(s) are also 
		    ** qualified. Then threads could be assigned only 
		    ** qualified partitions. But slightly more careful 
		    ** optimization code is required before that can be 
		    ** risked. */

		    lpartno = lpartno / partp->part_gcount;
		    ppartno = lpartno * partp->part_pcdim->stride;
		    qen_status->node_u.node_orig.node_lpart_num = lpartno;
		    qen_status->node_u.node_orig.node_gstart = lpartno;
		    qen_status->node_u.node_orig.node_grouppl = 
						partp->part_gcount;
		    /* Physical parts per logical partition: */
		    qen_status->node_u.node_orig.node_partsleft = 
				totparts / partp->part_pcdim->nparts;
		    qen_status->node_u.node_orig.node_groupsleft = parts_per_thread;
		    if (groupmap != NULL)
			setup_groupmap(qen_status, partp, qpmap, groupmap);

		    /* When we get around to reading, either this call or
		    ** later on, we need to check qualification and open
		    ** the partition.
		    */
		    qen_status->node_status_flags |= QEN128_PGROUP_SETUP;
		} /* if group */
	    } /* if any parts for thread */

	    if (ppartno >= 0 && partp->part_gcount <= 0)
	    {
		/* Prepare DMR_CB/DMT_CB for 1st partition,
		** but only if not grouping.  (Grouping will do
		** this at the next read, after checking for an
		** qualified-empty group as needed.)
		*/
#ifdef xDEBUG
    TRdisplay("QEN_ORIG1: start at node: %d  pp: %d  lp: %d by thread: %d\n",
	node->qen_num, ppartno, lpartno, dsh->dsh_threadno);
#endif
		status = qeq_part_open(rcb,
		    dsh, dsh->dsh_row[node->qen_row], 
		    dsh->dsh_qp_ptr->qp_row_len[node->qen_row],
		    partp->part_dmrix, partp->part_dmtix, ppartno);
		if (status != E_DB_OK)
		    return(status);
		qen_status->node_pcount++;
		cbs[node->node_qen.qen_orig.orig_get] = 
				cbs[partp->part_dmrix + ppartno + 1];
				/* save for subsequent calls */
		dmr_cb = (DMR_CB *)cbs[node->node_qen.qen_orig.orig_get];
	    }

	    /* Save results into node status for next call. */
	    qen_status->node_ppart_num = ppartno;
	    /* end of partitioned-specific OPEN preparation */
	}
	else if ( dmr_cb->dmr_access_id == NULL && dsh != dsh->dsh_parent )
	{
	    /*
	    ** Then this is a Child thread that needs a table
	    ** instantiated because two threads cannot share
	    ** the same DMR_CB which is associated with a
	    ** single transaction context. Each Child thread
	    ** has its own transaction context and must 
	    ** therefore have its own DMR_CB instance.
	    **
	    ** Passing '-1' as dmtix to qeq_part_open
	    ** tells it this is what we're doing.
	    */
	    status = qeq_part_open(rcb,
			dsh, dsh->dsh_row[node->qen_row], 
			dsh->dsh_qp_ptr->qp_row_len[node->qen_row],
			node->node_qen.qen_orig.orig_get, (i4)-1, (u_i2)0);
	    if ( status != E_DB_OK )
		return(status);
	}

	/*
	** If keyed ORIG, call qen_keyprep() if not already done.
	** If serial or 1:1 parallel plan, the keys were prepared
	** by qeq_validate and "node_keys" will not be NULL;
	** if a Child thread, qen_dsh zero-filled QEN_STATUS
	** when making a copy of the DSH, hence "node_keys"
	** will be NULL.
	*/
	if ( node->node_qen.qen_orig.orig_pkeys != NULL &&
	     qen_status->node_u.node_orig.node_keys == NULL )
	{
	    status = qen_keyprep(dsh, node);
	    if (status != E_DB_OK)
		return(status);
	}

	/* Set this DMR_CB for DMF qualification if appropriate */
	qen_set_qualfunc(dmr_cb, node, dsh);

	/* See if we might be able to ask for fast-path get-next's.
	** Note that one of the conditions (not memtab) is already
	** satisfied if we're here...
	** A couple conditions (key-by-tid and position on tid) will
	** be checked later.
	*/
	if (!dsh->dsh_qp_stats && !readback
	  && !(node_orig_flag & ORIG_TKEY
		  && node->node_qen.qen_orig.orig_qual != NULL)
	  && (node_orig_flag & ORIG_UKEY) == 0
	  && node->qen_prow == NULL)
	{
	    qen_status->node_status_flags |= QEN_ORIG_FASTNEXT_OK;
	}

	function &= ~TOP_OPEN;
    }

    /* Do close processing, if required.  Not much to see here. */
    if (function & FUNC_CLOSE)
    {
	/* Hard-set the flags to all off except closed */
	qen_status->node_status_flags = QEN8_NODE_CLOSED;
	return(E_DB_OK);
    }

    /* If End-Of-Group call, terminate the current group.
    ** We want to feed the caller either QE00A5 or QE0015, depending
    ** on whether there will be anything more to read.  (If we have a
    ** qualified PC-join, and our side is now finished, we can potentially
    ** tell the caller to stop now and avoid further work by returning
    ** QE0015 eof instead of QE00A5 end-of-group.)
    ** For non-qualified groups, just see if there are more to do.
    ** For qualified groups we need to creep to the end of this group
    ** to keep the group bit-map updated.
    */
    if (function & FUNC_EOGROUP)
    {
	/* if inappropriate context, ignore.  (Maybe should set EOF?) */
	if (partp == NULL || partp->part_gcount <= 0)
	    return (E_DB_OK);

#ifdef xDEBUG
	TRdisplay("QEN_ORIG5: eog-func node: %d  pp: %d  lp: %d thread: %d gleft: %d rcount: %d\n",
	    node->qen_num, qen_status->node_ppart_num,
	    qen_status->node_u.node_orig.node_lpart_num,
	    dsh->dsh_threadno,
	    qen_status->node_u.node_orig.node_groupsleft,
	    qen_status->node_rcount);
#endif
	if (qen_status->node_u.node_orig.node_groupsleft <= 0)
	{
	    /* We already hit EOF.  Stay there. */
	    goto eof_exit;
	}
	/* If we're already sitting at an end-of-group, we have to
	** advance the node position to the start of the next group.
	** This way, successive EOG requests pitch out successive groups.
	*/
	if (qen_status->node_status_flags & QEN32_PGROUP_DONE)
	{
	    if (!advance_next_group(qen_status, partp))
		goto eof_exit;		/* We ran out of groups */
	}
	/* Now we can pitch out the rest of the current group. */
	if (groupmap == NULL)
	{
	    /* Would this have been the last group?  If so, just take
	    ** the orig to EOF now.
	    */
	    if (qen_status->node_u.node_orig.node_groupsleft <= 1)
		goto eof_exit;
	}
	else
	{
	    /* Finish out this group to update the groupmap */
	    while (advance_group(qen_status, partp, groupmap) != -1)
		;  /* empty */
	    if (BTcount(groupmap, totparts) == 0)
		goto eof_exit;
	}
	qen_status->node_status_flags |= (QEN32_PGROUP_DONE | QEN64_ORIG_NORESET);
	/* Please note that groups-left does NOT get counted off
	** here!  It gets counted off when we return to orig after
	** we return end-of-group here.
	*/
	dsh->dsh_error.err_code = E_QE00A5_END_OF_PARTITION;
	return (E_DB_WARN);
    }

    /* If last read returned end-of-group, it's now time to set up for
    ** the next group.  Count off the one we did and see if there
    ** are more to do.  (There ought to be, else the last call here
    ** should have returned EOF instead of end-of-group, but be safe.)
    */
    if (qen_status->node_status_flags & QEN32_PGROUP_DONE)
    {
	qen_status->node_status_flags &= ~QEN32_PGROUP_DONE;
	if (!advance_next_group(qen_status, partp))
	    /* Looks like EOF */
	    goto eof_exit;
	/* Ask for group qualification check, partition opening, below */
	qen_status->node_status_flags |= QEN128_PGROUP_SETUP;
    }

    /* If we're at the start of a new partition group, we need to check
    ** it out.  Make sure that there is some qualified partition in the
    ** group, and if there is, open the first physical partition.
    ** The open is deferred to this point because there might not be any
    ** qualifying partitions -- in which case we return either end-of-group
    ** if there are more groups) or end-of-file.
    */
    if (qen_status->node_status_flags & QEN128_PGROUP_SETUP)
    {
	qen_status->node_status_flags &= ~QEN128_PGROUP_SETUP;
	ppartno = qen_status->node_ppart_num;
	if (groupmap != NULL)
	{
	    /* Need to see if this physical partition is in our map.
	    ** If it's not, see if there are ANY more pp's to do.
	    ** If not, return eof;  if yes, "advance" to move to the
	    ** next qualifying PP in this group;  if none, return
	    ** end-of-group.
	    */
	    if (! BTtest(ppartno, groupmap))
	    {
		ppartno = advance_group(qen_status, partp, groupmap);
		if (ppartno == -1)
		{
		    /* Got either end-of-group or end-of-file, see which */
		    if (BTcount(groupmap, totparts) == 0)
			goto eof_exit;
		    qen_status->node_status_flags |=
				(QEN32_PGROUP_DONE | QEN64_ORIG_NORESET);
		    dsh->dsh_error.err_code = E_QE00A5_END_OF_PARTITION;
		    return (E_DB_WARN);
		}
	    }
	}
	/* Open the partition.  (Really should make this bit a routine.) */
#ifdef xDEBUG
    TRdisplay("QEN_ORIG4: pgroup setup node: %d  pp: %d  lp: %d gleft: %d by thread: %d\n",
	node->qen_num, ppartno, qen_status->node_u.node_orig.node_lpart_num,
	qen_status->node_u.node_orig.node_groupsleft, dsh->dsh_threadno);
#endif
	status = qeq_part_open(rcb,
			dsh, dsh->dsh_row[node->qen_row], 
			dsh->dsh_qp_ptr->qp_row_len[node->qen_row],
			partp->part_dmrix, partp->part_dmtix, ppartno);
	if (status != E_DB_OK)
	    return(status);
	qen_status->node_pcount++;
	cbs[node->node_qen.qen_orig.orig_get] =
				cbs[partp->part_dmrix + ppartno + 1];
	dmr_cb = (DMR_CB *)cbs[node->node_qen.qen_orig.orig_get];
	/* Set DMF qualification into the DMR_CB if needed */
	qen_set_qualfunc(dmr_cb, node, dsh);
	/* Make sure we go thru the qen-tkey reset poop a little ways below.
	** This either positions or sets "reset".
	*/
	qen_status->node_status = QEN4_NO_MORE_ROWS;
    } /* if pgroup-setup */

    /* qen_tkey may not have a valid address. be carefull when using */
    if (node->node_qen.qen_orig.orig_pkeys)
	qen_tkey = (QEN_TKEY*) cbs[node->node_qen.qen_orig.orig_keys];
    else
	qen_tkey = 0;    

    /* If we get this far, the "noreset" i.e. no-partition-reposition
    ** flag has done its job, turn it off.
    */
    qen_status->node_status_flags &= ~QEN64_ORIG_NORESET;
    if (qen_status->node_status_flags & QEN16_ORIG_FIRST)
	reset = TRUE;		/* force reset for 1st row retrieved */


    /* Check for cancel, context switch if not MT, but only for in-
    ** memory tables.
    ** DMR_GET, below, will duplicate this CSswitch, so let it handle it
    */
    if (memtab)
	CScancelCheck(dsh->dsh_sid);
    if (QEF_CHECK_FOR_INTERRUPT(qef_cb, dsh) == E_DB_ERROR)
	return (E_DB_ERROR);

    /* If the trace point qe90 is turned on then gather cpu and dio stats */
    if (dsh->dsh_qp_stats)
    {
	qen_bcost_begin(dsh, &timerstat, qen_status);
	bcost = TRUE;
    }

    dsh->dsh_error.err_code = E_QE0000_OK;
    if (partp && qen_status->node_ppart_num < 0)
    {
	dsh->dsh_error.err_code = E_QE0015_NO_MORE_ROWS;
	qen_status->node_status = QEN4_NO_MORE_ROWS;
	status = E_DB_WARN;
	goto exit;
    }

    /* FIXME get this stupid test out of the mainline!  Ideally
    ** opc would generate a key-by-tid flag in the node.
    */
    if (node->node_qen.qen_orig.orig_pkeys != NULL &&
	node->node_qen.qen_orig.orig_pkeys->key_kor != NULL &&
	node->node_qen.qen_orig.orig_pkeys->key_kor->kor_keys != NULL &&
	node->node_qen.qen_orig.orig_pkeys->key_kor->kor_keys->kand_attno == 0
       )
    {
	key_by_tid = TRUE;
	qen_status->node_status_flags &= ~QEN_ORIG_FASTNEXT_OK;
    }
    else
    {
	key_by_tid = FALSE;
    }
    /* This flag is needed because of the horrendous mess that is
    ** positioning.  key-by-tid needs to reposition on each get, as
    ** "next" would be wrong;  so it's handled with UKEY below.
    ** Except, key-by-tid also needs to do the orig-first reset stuff
    ** since we might have an IN-list of tids and reset needs to
    ** reset it.  So it ends up doing the first position in the
    ** orig-first code, and we have to remember to not reposition the
    ** first time when we get to position:.
    ** What a mess.  One improvement might be to redefine UKEY as
    ** specifically "reposition on each orig call", and unify the
    ** orig-first handling.
    ** An even better fix would be to clean up the positioning madness
    ** entirely, so that it's done in some predictable manner.
    */
    positioned_by_tid = FALSE;

    if (qen_status->node_status == QEN4_NO_MORE_ROWS)
    {
	qen_status->node_status = QEN1_EXECUTED;
	if (qen_tkey)
	{
	    qen_tkey->qen_nkey = qen_status->node_u.node_orig.node_keys;
	    if (!(qen_status->node_status_flags & QEN16_ORIG_FIRST))
		goto position;
	}
	reset = TRUE;
    }

    /* 
    ** Only do reset when qen_status->node_status != QEN0_INITIAL, that's 
    ** because if status is QEN0_INITIAL, qeq_validate already reset
    ** this node. The exception is if this is also a partitioned table,
    ** in which case, qeq_validate reset the master table, not this
    ** partition.
    ** Don't run this for single-unique-key orig's (UKEY).
    */  
loop_reset:
    if (reset && (qen_status->node_status != QEN0_INITIAL || 
		qen_tkey == (QEN_TKEY *) NULL ||
		(qen_status->node_status_flags & QEN16_ORIG_FIRST) &&
		!(node_orig_flag & ORIG_UKEY)))
    {
	if (qen_tkey && qen_status->node_status_flags & QEN16_ORIG_FIRST)
	{
	    /* 1st qen_position call is now in qen_orig, not qeq_validate.
	    ** This was done for || query processing since the DSH to be
	    ** used may not exist before the qen_orig call. */
	    status = qen_position(qen_tkey, dmr_cb, dsh,
				node->node_qen.qen_orig.orig_ordcnt, readback);
	    /* get next key */

	    /* If using tids, note that we've positioned now, so we shouldn't
	    ** advance again until we've used this row (kibro01) b121372
	    */
	    if (key_by_tid)
		positioned_by_tid = TRUE;

	    if (dmr_cb->dmr_flags_mask == DMR_PREV) 
				qen_status->node_access = QEN_READ_PREV;
	     else qen_status->node_access = 0;
					/* descending sort logic */
	    if (status != E_DB_OK)
	    {
		if (dsh->dsh_error.err_code != E_QE0015_NO_MORE_ROWS
		    && dsh->dsh_error.err_code != E_DM0055_NONEXT)
		    return(status);
		
		dsh->dsh_error.err_code = E_QE0015_NO_MORE_ROWS;
		qen_status->node_status = QEN4_NO_MORE_ROWS;
		status = E_DB_WARN;
	    }
	}
	else status = qen_reset(dsh, node, qen_status);
	qen_status->node_status_flags &= ~QEN16_ORIG_FIRST;
	if (status != E_DB_OK)
	{
#ifdef xDEBUG
	    /* (VOID) qe2_chk_qp(dsh); */
#endif
	    goto exit;
	}
    }

    /* if no more rows, return */
    if (qen_status->node_status == QEN4_NO_MORE_ROWS)
    {
	dsh->dsh_error.err_code = E_QE0015_NO_MORE_ROWS;
#ifdef xDEBUG
	/* (VOID) qe2_chk_qp(dsh); */
#endif

	if (bcost)
	{
	    qen_ecost_end(dsh, &timerstat, qen_status);
	}

	return (E_DB_WARN);
    }

    /* position if necessary. TIDS and some keys are unique and
    ** need a position before each get.  UKEY didn't run the orig-first
    ** code at all so make sure the flag is off.
    */
    if ((node_orig_flag & ORIG_UKEY) || key_by_tid)
    {
	qen_status->node_status_flags &= ~QEN16_ORIG_FIRST;
			/* UKEY queries make it here with the flag on */
	goto position;
    }

    /* In-memory constant table logic goes here. It replaces the whole 
    ** "for" loop which follows. It loops, copying rows from the table
    ** to the output buffer until the count of scanned rows matches the 
    ** number of rows in the table. If there is orig_qual ADF code (not
    ** too likely, but you never know!), rows are processed until one
    ** passes the test. */
    if (memtab)
    {
	PTR	recptr;
	ADE_EXCB *qual_excb = node_xaddrs->qex_origqual;

	cnsttab_p = (QEE_MEM_CONSTTAB *)dmr_cb;
				/* structure ptr is stuffed in orig_get */
	for (;;)		/* loop for escaping from */
	{
	    if (cnsttab_p->qer_scancount == cnsttab_p->qer_rowcount)
	    {
		/* No more rows. */
		dsh->dsh_error.err_code = E_QE0015_NO_MORE_ROWS;
		qen_status->node_status = QEN4_NO_MORE_ROWS;
		status = E_DB_WARN;
		goto exit;
	    }
	    recptr = (PTR)(cnsttab_p->qer_tab_p + cnsttab_p->qer_rowsize *
			cnsttab_p->qer_scancount++);
				/* compute next record addr & incr count */
	    MEcopy((char *)recptr, cnsttab_p->qer_rowsize,
			(char *)dsh->dsh_row[node->qen_row]);
				/* copy it to output buffer */
	    /* qualify the tuple, if there's orig_qual code (shouldn't usually
	    ** be any orig_qual - but you never know with EQclasses!) */
	    if (qual_excb)
	    {
		status = qen_execute_cx(dsh, qual_excb);
		if (status != E_DB_OK)
		{
		    goto exit;
		}
	    }
	    if (qual_excb == NULL || qual_excb->excb_value == ADE_TRUE) break;
	}		/* end of "read" loop */
	status = E_DB_OK;	/* got a "row" - now run with it */
	goto exit;
    }	/* end of memory table processing */

    dmr_cb->dmr_flags_mask = DMR_NEXT;
    if (readback && (!qen_tkey || (qen_status->node_access & QEN_READ_PREV)))
     dmr_cb->dmr_flags_mask = DMR_PREV;
				/* read backwards for descending sort if no
				** key qual or if positioned with range pred */
    if (dsh->dsh_qp_status & DSH_SORT)
        dmr_cb->dmr_flags_mask |= DMR_SORT;

    for (;;)
    {
	dmr_cb->error.err_code = E_DM0000_OK;

	/* Set "index pages only" flag, if warranted. */
	if ((node_orig_flag & ORIG_IXONLY) &&
		!(dmr_cb->dmr_flags_mask & DMR_BY_TID))
	    dmr_cb->dmr_flags_mask |= DMR_PKEY_PROJECTION;
	/* get the positioned tuple */
	status = dmf_call(DMR_GET, dmr_cb);

	if (status == E_DB_OK)
	{

	    /* If the tid is needed for subsequent qualification or 
	    ** for delete/replace cursor by tid,
	    ** move it into the designated row buffer. Action header
	    ** for delete/replace has ahd_tidrow/tidoffset fields that 
	    ** address tid in ORIG output buffer. Need to
	    ** verify that OPF generates 8-byte TID values for
	    ** comparison here. */
	    if (offset > -1)
	    {
		tidoutput = (char *) dmr_cb->dmr_data.data_address + offset;
		bigtid.tid_i4.tid = dmr_cb->dmr_tid;
		bigtid.tid_i4.tpf = 0;
		if (node->node_qen.qen_orig.orig_part)
		    bigtid.tid.tid_partno = qen_status->node_ppart_num;
		I8ASSIGN_MACRO(bigtid,*tidoutput);

		/* Hand qualify if tid is part of the qualification.
		** (Normally we let DMF do the row qualification.)
		*/
		if (node_orig_flag & ORIG_TKEY
		  && node->node_qen.qen_orig.orig_qual != NULL)
		{
		    status = qen_execute_cx(dsh, node_xaddrs->qex_origqual);
		    if (status != E_DB_OK)
		    {
			break;
		    }
		    /* handle the condition where the qualification failed */
		    if (node_xaddrs->qex_origqual->excb_value != ADE_TRUE)
		    {
			/* try next row, unless reposition on every row.
			** For key-by-tid, just try the next tid if any.
			** For ukey (single unique key), it's over.
			*/
			if (key_by_tid)
			    /* Might have an IN-list of tids! */
			    goto position;
			else if ((node_orig_flag & ORIG_UKEY) == 0)
			    continue; /* Common case */
 		        else
			{
			    dsh->dsh_error.err_code = E_QE0015_NO_MORE_ROWS;
			    qen_status->node_status = QEN4_NO_MORE_ROWS;
	                    status = E_DB_WARN;
	                    break;
			}
		    }
		}
	    }
	    /* return the good row */
	    /* If fast get-next might be allowed, set for next time in */
	    if (qen_status->node_status_flags & QEN_ORIG_FASTNEXT_OK
	      && (dmr_cb->dmr_flags_mask & DMR_BY_TID) == 0)
	    {
		qen_status->node_status = QEN_ORIG_FASTNEXT;
		qen_status->node_u.node_orig.node_dmr_flags = dmr_cb->dmr_flags_mask;
	    }
	    goto exit;
	}
	else	/* DMR_GET failed */
	{
    dmr_get_error:

	    /* if not an expected error, return it. Else, fall through
	    ** to position to next key.
	    */
	    if (!(dmr_cb->error.err_code == E_DM0055_NONEXT ||
		  dmr_cb->error.err_code == E_DM0074_NOT_POSITIONED ||
		/* any tids in this node are user specified */
		    dmr_cb->error.err_code == E_DM003C_BAD_TID ||
		    dmr_cb->error.err_code == E_DM0044_DELETED_TID))
	    {
		/* This might include DM0023 which gets translated to
		** QE0025 which means msg already issued, no SC0206 please.
		** Ain't this fun?
		*/
		dsh->dsh_error.err_code = dmr_cb->error.err_code;
		if (dmr_cb->error.err_code == E_DM0023_USER_ERROR
		  && dsh->dsh_qp_ptr->qp_oerrorno_offset != -1)
		{
		    i4 *iierrorno;

		    /* DMF handled a row-qualifier user error, but we still
		    ** need to set iierrornumber while we have the info.
		    */
		    iierrorno = (i4 *) (
		(char *) (dsh->dsh_row[dsh->dsh_qp_ptr->qp_rerrorno_row])
			 + dsh->dsh_qp_ptr->qp_oerrorno_offset);
		    *iierrorno = dmr_cb->error.err_data;
		}
		break;
	    }
	    /* else, we are still OK */
	    status = E_DB_OK;
	}

	/* position to new key */
position:
	if (qen_tkey)
	{
	    /* If we've already got a tid row, don't advance yet */
	    if (positioned_by_tid)
	    {
		positioned_by_tid = FALSE;
		status = E_DB_OK;
	    } else
	    {
	        status = qen_position(qen_tkey, dmr_cb, dsh,
				node->node_qen.qen_orig.orig_ordcnt, readback);
	    }
	    if (status > E_DB_WARN)
		break;
	    /* get next key */
	    if (dmr_cb->dmr_flags_mask == DMR_PREV) 
				qen_status->node_access = QEN_READ_PREV;
	     else qen_status->node_access = 0;
					/* descending sort logic */
	    if (status == E_DB_OK)
	    {
		continue;
	    }
	}
	/* EOF on this table/partition. Free the pages held in buffer. */
	status = dmf_call(DMR_UNFIX, dmr_cb);
	if (status != E_DB_OK)
	    goto errexit;
#ifdef xDEBUG
    if (partp)
	TRdisplay("QEN_ORIG2: eof node: %d  pp: %d  lp: %d thread: %d rcount: %d\n",
	    node->qen_num, qen_status->node_ppart_num,
	    qen_status->node_u.node_orig.node_lpart_num,
	    dsh->dsh_threadno, qen_status->node_rcount);
#endif
	/* If it's a partition, close it as well, so that we don't
	** pile up a brazillion uninteresting open partitions, which
	** would stay open and unreclaimable in DMF.
	** Note that we don't have to get this exactly right, query
	** cleanup will get anything we miss, we just want to help DMF.
	** Only close partitions opened for read, don't want to get into
	** trouble with cursor updating and such.
	*/
	if (partp != NULL)
	{
	    dmt_cb = (DMT_CB *) cbs[partp->part_dmtix + qen_status->node_ppart_num + 1];
	    if (dmt_cb->dmt_access_mode == DMT_A_READ)
	    {
		(void) qen_closeAndUnlink(dmt_cb, dsh);
		dmr_cb->dmr_access_id = NULL;
	    }
	}

	/* we have no new keys, we are at EOF */
	dsh->dsh_error.err_code = E_QE0015_NO_MORE_ROWS;
	qen_status->node_status = QEN4_NO_MORE_ROWS;
	status = E_DB_WARN;
	break;
    }

    /* if row is still good, return it */
exit:
    if (status == E_DB_OK)
    {
	/* set node status after execution */
	if (qen_status->node_status == QEN0_INITIAL)
	    qen_status->node_status = QEN1_EXECUTED;
	
	if (node_xaddrs->qex_fatts)
	{
	    status = qen_execute_cx(dsh, node_xaddrs->qex_fatts);
	    if (status != E_DB_OK)
		return (status);
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
	    (void) qen_print_row(node, rcb, dsh);
	}
	if (bcost)
	{
	    qen_ecost_end(dsh, &timerstat, qen_status);
	}
	return (E_DB_OK);
    }

#ifdef xDEBUG
    /* (VOID) qe2_chk_qp(dsh); */
#endif

    /* Following is logic to determine next partition of partitioned 
    ** table to process. Non-grouped, non-qualified table access simply
    ** increments physical partition number. But partition grouping and
    ** qualification introduce complications.
    **
    ** Qualified, non-grouped access is pretty simple, we ask for the
    ** next partition number from the qualification bit-map.  We stop
    ** when we've reached our partition quota (in node_partsleft) or
    ** run off the end of the bit-map (can happen in last thread.)
    **
    ** Grouped access is slightly trickier.  We "advance" to the next
    ** physical partition in the group (letting advance_group deal with
    ** qualifications and such).  If there is nothing more in the group,
    ** we return QE00A5 (if there are more groups and more qualified
    ** partitions), or QE0015 (if we're at the very end).
    **
    ** Unless we decide that we've reached the end, we open the newly
    ** selected physical partition and loop back to read the next row.
    */
  
    if (status == E_DB_WARN && partp != NULL
      && qen_status->node_status == QEN4_NO_MORE_ROWS)
    {
	ppartno = qen_status->node_ppart_num;
	if (partp->part_gcount <= 0)
	{
	    /* Non-grouped access.  Just count off another PP and move to
	    ** the next one, if there is one.
	    ** "partsleft" might be over-estimated in the last thread,
	    ** so make sure we don't run off the end of partitions.
	    */
	    if (--qen_status->node_u.node_orig.node_partsleft <= 0)
		goto eof_exit;
	    if (qpmap == NULL)
	    {
		if (++ppartno >= totparts)
		    goto eof_exit;	/* Ran off the end */
	    }
	    else
	    {
		ppartno = BTnext(ppartno, qpmap, totparts);
		if (ppartno == -1 || ppartno >= totparts)
		    goto eof_exit;	/* No more qualifying PP's */
	    }
	}
	else
	{
	    /* Grouped access.  Advance to the next PP for group, letting
	    ** advance-group take care of the bookkeeping.
	    */
	    ppartno = advance_group(qen_status, partp, groupmap);
	    if (ppartno == -1)
	    {
		/* See if eof or end-of-group. */
		if (qen_status->node_u.node_orig.node_groupsleft <= 1
		  || (groupmap != NULL && BTcount(groupmap, totparts) == 0) )
		    goto eof_exit;

		/* Please note that groups-left does NOT get counted off
		** here!  It gets counted off when we return to orig after
		** we return end-of-group here.
		*/
		qen_status->node_status_flags |=
				(QEN32_PGROUP_DONE | QEN64_ORIG_NORESET);
		dsh->dsh_error.err_code = E_QE00A5_END_OF_PARTITION;
		status = E_DB_WARN;
		goto errexit;		/* ecost and return */
	    }
	}
	dsh->dsh_error.err_code = E_QE0000_OK;	/* Undo EOF indication */
	qen_status->node_ppart_num = ppartno;
	/* Open up the partition and whirl back to reset/read from it.
	** This should probably be made a routine...
	*/
#ifdef xDEBUG
    TRdisplay("QEN_ORIG3: continue node: %d  pp: %d  lp: %d by thread: %d\n",
	node->qen_num, ppartno, qen_status->node_u.node_orig.node_lpart_num, dsh->dsh_threadno);
#endif
	status = qeq_part_open(rcb,
		dsh, dsh->dsh_row[node->qen_row], 
		dsh->dsh_qp_ptr->qp_row_len[node->qen_row],
		partp->part_dmrix, partp->part_dmtix, ppartno);
	if (status != E_DB_OK)
	    return(status);
	qen_status->node_pcount++;
	cbs[node->node_qen.qen_orig.orig_get] =
				cbs[partp->part_dmrix + ppartno + 1];
	dmr_cb = (DMR_CB *)cbs[node->node_qen.qen_orig.orig_get];
	/* Set up DMF qualification in this dmrcb if needed */
	qen_set_qualfunc(dmr_cb, node, dsh);
	qen_status->node_status = QEN1_EXECUTED;
	if (qen_tkey)
	{
	    qen_tkey->qen_nkey = qen_status->node_u.node_orig.node_keys;
	    goto position;		/* reset key position */
	}
	reset = TRUE;
	goto loop_reset;
    } /* partition advance */

errexit:
    if (bcost)
    {
	qen_ecost_end(dsh, &timerstat, qen_status);
    }

    if (dsh->dsh_error.err_code == E_QE00A5_END_OF_PARTITION)
	qen_status->node_status_flags |= QEN64_ORIG_NORESET;

    return (status);


/* Convenience return point for returning EOF */
eof_exit:
    qen_status->node_u.node_orig.node_groupsleft = 0;
    qen_status->node_u.node_orig.node_partsleft = 0;
    qen_status->node_u.node_orig.node_lpart_num = -1;
    qen_status->node_ppart_num = -1;
    qen_status->node_status = QEN4_NO_MORE_ROWS;
    dsh->dsh_error.err_code = E_QE0015_NO_MORE_ROWS;
    if (bcost)
	qen_ecost_end(dsh, &timerstat, qen_status);
    return (E_DB_WARN);

} /* qen_orig */

/*{
** Name: QEN_POSITION	- position dmr_cb for orig node
**
** Description:
**      This routine uses the next key in the key list to position
**  the dmr_cb for row retrieval. 
**
** Inputs:
**      qen_tkey                        key list
**      dmr_cb                          dmr control block for position
**      dsh                             runtime environment structure
**
** Outputs:
**      dsh
**	    .error.err_code		error status
**					E_QE0000_OK
**					E_QE0015_NO_MORE_ROWS
**					dmf errors
**	Returns:
**	    E_DB_{OK,WARN,ERROR}
**	Exceptions:
**	    none
**
** Side Effects:
**	    The dmr_cb is positioned
**
** History:
**      19-may-87 (daved)
**          written
**	02-dec-88 (puree)
**	    when position by key, the count of keys in dmr_cb which was set
**	    in qee_creact may be incorrect since qeq_ksort may have reordered
**	    the key. Reset it here to the actual length of the key list.
**	    (BUG 4143).
**	21-jul-98 (inkdo01)
**	    Added support for topsort-less descending sorts.
**	13-feb-04 (inkdo01)
**	    Support 8-byte TID values in key structure.
**	18-mar-04 (inkdo01)
**	    Support 8-byte TIDs in byte swap compatible fashion.
**	26-Mar-09 (kibro01) b121835
**	    Protect against null kattr pointer since we can now have inequality
**	    under an OR node (which previously was just equality combos).
[@history_template@]...
*/
DB_STATUS
qen_position(
QEN_TKEY           *qen_tkey,
DMR_CB             *dmr_cb,
QEE_DSH            *dsh,
i4		   ordcount,
bool		   readback)
{
    DMR_ATTR_ENTRY  *attr, *attr1;
    DMR_ATTR_ENTRY  **attr_array;
    QEF_KATT	    *qef_katt;
    QEN_NOR	    *qen_nor;
    QEN_NKEY	    *keys;
    char	    *output;
    char	    *input;
    DB_STATUS	    status = E_DB_OK;
    i8		    bigtid;
    i4	    i;
    i4		    attcount = 0;
    i4		    lastatt = -1;
    bool	    alleqs;

#ifdef xDEBUG
    /* (VOID) qe2_chk_qp(dsh); */
#endif
    qen_nor = qen_tkey->qen_nkey;
    /* if no more keys */
    if (qen_nor == 0)
    {
	dsh->dsh_error.err_code = E_QE0015_NO_MORE_ROWS;
	status = E_DB_WARN;
	goto exit;
    }
    /* if tid lookup, set the tid in the dmr_cb.*/
    if (!qen_nor->qen_key->nkey_att->attr_attno)
    {
	dmr_cb->dmr_flags_mask = DMR_BY_TID;
	/* set the tid */
	output = (char *) &bigtid;
	input =  (char *) dsh->dsh_key 
			+ qen_nor->qen_key->nkey_att->attr_value;
	    
	qen_tkey->qen_nkey = qen_nor->qen_next;
	/* copy the tid into an i8 then cast to i4. This 
	** won't currently handle partitioned tables. */
	output[0] = input[0];
	output[1] = input[1];
	output[2] = input[2];
	output[3] = input[3];
	output[4] = input[4];
	output[5] = input[5];
	output[6] = input[6];
	output[7] = input[7];
	dmr_cb->dmr_tid = (i4)bigtid;
	/* try to get this tid */
	goto exit;
    }
    /* get key. */
    for (;;)
    {
	alleqs = TRUE;
	qen_nor = qen_tkey->qen_nkey;
	/* if no more keys */
	if (qen_nor == 0)
	{
	    dsh->dsh_error.err_code = E_QE0015_NO_MORE_ROWS;
	    status = E_DB_WARN;
	    break;
	}
	/* set keys */
	dmr_cb->dmr_position_type = DMR_QUAL;
	attr_array = (DMR_ATTR_ENTRY**) dmr_cb->dmr_attr_desc.ptr_address;
	/* use the top of the key list, the first element */
	for (keys = qen_nor->qen_key, i = 0;
	    keys; 
	    keys = keys->nkey_next, i++)
	{
	    qef_katt = keys->nkey_att;
	    /* During merging of opc_meqrange and opc_srange we can have
	    ** overestimated required key operations, so skip blank ones
	    */
	    if (!qef_katt)
	    {
		i--;
		continue;
	    }
	    attr = attr_array[i];
	    attr->attr_number = qef_katt->attr_attno;
	    attr->attr_operator = qef_katt->attr_operator;
	    if (qef_katt->attr_operator != DMR_OP_EQ) alleqs = FALSE;
	    attr->attr_value = (char *) dsh->dsh_key + qef_katt->attr_value;

	    /* There are 2 circumstances involving backwards reading which
	    ** require a DMR_OP_EQ to be changed to a pair of GTE/LTE's.
	    ** Both are required because DMF needs the positioning structure
	    ** to end in a range check if it is to do a ENDQUAL. The first
	    ** follows immediately and occurs when there are fewer columns
	    ** in the positioning structure than order by (desc) columns AND
	    ** the last attribute is DMR_OP_EQ. The second is when the 
	    ** attribute in the key structure corresponding to the last 
	    ** order by (desc) column is DMR_OP_EQ, but some earlier 
	    ** attribute has a range test. */
	    if (readback && lastatt != attr->attr_number)
	    {	/* track key column count for readback processing */
		attcount++;
		lastatt = attr->attr_number;
		if (attcount == ordcount && !alleqs &&
				attr->attr_operator == DMR_OP_EQ)
		{
		    /* Add one more attribute entry and make it and previous
		    ** into GTE/LTE pair on same value. */
		    attr1 = attr_array[i++];
		    attr1->attr_number = attr->attr_number;
		    attr1->attr_value = attr->attr_value;
		    attr1->attr_operator = DMR_OP_LTE;
		    attr->attr_operator = DMR_OP_GTE;
	    	}
	    }
	}

	/* This is the second condition under which a DMR_OP_EQ is changed
	** to a range test. */
	if (readback && attcount < ordcount && 
				attr->attr_operator == DMR_OP_EQ)
	{
	    /* Add one more attribute entry and make it and previous
	    ** into GTE/LTE pair on same value. */
	    attr1 = attr_array[i++];
	    attr1->attr_number = attr->attr_number;
	    attr1->attr_value = attr->attr_value;
	    attr1->attr_operator = DMR_OP_LTE;
	    attr->attr_operator = DMR_OP_GTE;
	    alleqs = FALSE;
	}

	/* reset ptr_in_count to the actual count of key (BUG 4143) */
	dmr_cb->dmr_attr_desc.ptr_in_count = i;

	qen_tkey->qen_nkey = qen_tkey->qen_nkey->qen_next;
	if (readback && !alleqs) dmr_cb->dmr_position_type = DMR_ENDQUAL;
					/* descending sort on a range probe
					** requires position at end of row set */
	dmr_cb->dmr_flags_mask = 0;
	status = dmf_call(DMR_POSITION, dmr_cb);
	if (status == E_DB_OK)
	{
	    dmr_cb->dmr_flags_mask = DMR_NEXT;
	    if (readback && !alleqs) dmr_cb->dmr_flags_mask = DMR_PREV;
	    break;
	}
	if (dmr_cb->error.err_code != E_DM0055_NONEXT)
	{
	    if (dmr_cb->error.err_code == E_DM0074_NOT_POSITIONED)
		dsh->dsh_error.err_code = E_QE0110_NO_ROWS_QUALIFIED;
	    else
		dsh->dsh_error.err_code = dmr_cb->error.err_code;
	    break;
	}
	/* else, try the next KOR */
    }
exit:
#ifdef xDEBUG
    /* (VOID) qe2_chk_qp(dsh); */
#endif
    return (status);
}


/*{
** Name: QEN_RESET	- Reposition a table for qen_orig node
**
** Description:
**
** Inputs:
**
** Outputs:
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
** History:
**      02-feb-88 (puree)
**          written to support repositioning for subselect.
**	13-apr-90 (nancy)
**	    Bug 20408.  Determine if this is a key by tid.  If so, position
**	    here. 
**	18-dec-90 (kwatts)
**	    Smart Disk integration: Added code to check for a Smart Disk program
**	    and if appropriate point at it from the rcb and set position type
**	    to SDSCAN. Also took opportunity to introduce a QEN_ORIG pointer and
**    	    tidy up the code.
**      7-aug-91 (stevet)
**          Fix B37995: Added new parameter to qeq_ksort call to indicate
**          finding the MIN of the MAX keys and MAX of the MIN keys is
**          not required when calling from qen_reset.
**	18-dec-1992 (kwatts)
**	    Smart Disk enhancement. There is now more for QEF to do in terms
**	    of setting Smart Disk info, including generating pointers for
**	    bind information if appropriate.
**      20-jul-1993 (pearl)
**          Bug 45912.  In qen_reset(), change call to qeq_ksort to pass
**          dsh instead of dsh_key.
**      19-jun-95 (inkdo01)
**          Collapsed orig_tkey/ukey nat's into orig_flag flag word.
**	21-jul-98 (inkdo01)
**	    Add support of topsort-less descending sorts (in qeq_ksort call).
[@history_template@]...
*/
static DB_STATUS
qen_reset(
QEE_DSH		   *dsh,
QEN_NODE           *node,
QEN_STATUS	   *qen_status )
{
    DB_STATUS	    status = E_DB_OK;
    PTR		    *cbs = dsh->dsh_cbs;    /* control blocks for DMU, ADF */
    DMR_CB	    *dmr_cb;
    QEN_TKEY	    *tkptr;
    QEN_NKEY	    *keys;
    QEF_KATT	    *qef_katt;
    QEN_ORIG	    *orig_node;
    DMR_ATTR_ENTRY  *attr;
    DMR_ATTR_ENTRY  **attr_array;
    i4		    i;
    bool	    key_by_tid;
    bool	    readback;
    bool	    alleqs;

#ifdef xDEBUG
    /* (VOID) qe2_chk_qp(dsh); */
#endif

    orig_node = &node->node_qen.qen_orig;
    readback = (orig_node->orig_flag & ORIG_READBACK);
    qen_status->node_access = 0;
    if (orig_node->orig_flag & ORIG_MCNSTTAB)
    {
	QEE_MEM_CONSTTAB   *cnsttab_p = 
			(QEE_MEM_CONSTTAB *)cbs[orig_node->orig_get];
	status = E_DB_OK;
	qen_status->node_status = QEN0_INITIAL;
	qen_status->node_rcount = 0;
	cnsttab_p->qer_scancount = 0;
	return(status);
    }

    if (orig_node->orig_pkeys != NULL &&
	orig_node->orig_pkeys->key_kor != NULL &&
	orig_node->orig_pkeys->key_kor->kor_keys != NULL &&
	orig_node->orig_pkeys->key_kor->kor_keys->kand_attno == 0
       )
    {
	key_by_tid = TRUE;
    }
    else
    {
	key_by_tid = FALSE;
    }

    do
    {
	dmr_cb = (DMR_CB*) cbs[orig_node->orig_get];

	/* position to the first record */

	if (orig_node->orig_pkeys == 0)
	{
	     if (!readback) 
		dmr_cb->dmr_position_type = DMR_ALL;
	     else dmr_cb->dmr_position_type = DMR_LAST;
	}
	else
	{
	    /* sort the keys and determine which ones to use */
	    tkptr = (QEN_TKEY*)cbs[orig_node->orig_keys];
	    if ((status = qeq_ksort(tkptr, orig_node->orig_pkeys,
		 dsh, (bool)FALSE, readback)) != E_DB_OK)
	    {
#ifdef xDEBUG
		/* (VOID) qe2_chk_qp(dsh); */
#endif
		return(status);
	    }
	    /* don't position on tids or unique keys because
	    ** these are handled in QEN_ORIG
	    */
	    if  ((orig_node->orig_flag & ORIG_UKEY) ||
		    (key_by_tid == TRUE)
		)
	    {
		/* don't call dmr_position */
		break;
	    }
	    /* set keys */
	    dmr_cb->dmr_position_type = DMR_QUAL;
	    attr_array = (DMR_ATTR_ENTRY**) 
		dmr_cb->dmr_attr_desc.ptr_address;
	    alleqs = TRUE;

	    /* use the top of the key list, the first element */
	    for (keys = tkptr->qen_nkey->qen_key, i = 0;
		keys; 
		keys = keys->nkey_next, i++)
	    {
		qef_katt = keys->nkey_att;
		attr = attr_array[i];
		attr->attr_number = qef_katt->attr_attno;
		attr->attr_operator = qef_katt->attr_operator;
		if (qef_katt->attr_operator != DMR_OP_EQ) alleqs = FALSE;
		attr->attr_value = (char *) dsh->dsh_key + qef_katt->attr_value;
	    }
	    /* advance to next key */
	    tkptr->qen_nkey = tkptr->qen_nkey->qen_next;
	    if (readback && !alleqs) dmr_cb->dmr_position_type = DMR_ENDQUAL;
	}
	dmr_cb->dmr_flags_mask = 0;
	status = dmf_call(DMR_POSITION, dmr_cb);
	if (status != E_DB_OK)
	{
	    if (dmr_cb->error.err_code == E_DM0055_NONEXT)
	    {
		dsh->dsh_qef_rowcount = 0;
		dsh->dsh_error.err_code = E_QE0015_NO_MORE_ROWS;
		qen_status->node_status = 
				QEN4_NO_MORE_ROWS;
		status = E_DB_WARN;
		break;
	    }
	    else
	    {
		dsh->dsh_error.err_code = dmr_cb->error.err_code;
	    }
	    break;
	}
	qen_status->node_status = QEN0_INITIAL;
    } while (FALSE);

    /* Set read direction for possible descending sort. */
    if (readback && (orig_node->orig_pkeys == 0 || !alleqs))
	qen_status->node_access = QEN_READ_PREV;
#ifdef xDEBUG
    /* (VOID) qe2_chk_qp(dsh); */
#endif
    return (status);
}


/*{
** Name: QEN_KEYPREP	Prepare DSH key areas for || queries
**
** Description:	ORIG nodes executing under 1:n exchange nodes must
**	have distinct DSH key areas allocated/formatted. 
**
** Inputs:
**
** Outputs:
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
** History:
**	22-june-04 (inkdo01)
**	    Written for || query processing.
**	01-Jul-2004 (jenjo02)
**	    Use qef_d_ulmcb (DSH memory) rather than
**	    qef_s_ulmcb (Sort memory) to allocate memory.
**	30-Jul-2004 (jenjo02)
**	    After qeq_ksort-ing the keys, set a pointer to
** 	    the first key in QEN_STATUS node_keys so this
**	    gets called only once per query.
**	20-Sep-2004 (jenjo02)
**	    Externalized function, now also called from
**	    qeq_validate some times.
*/
DB_STATUS
qen_keyprep(
	QEE_DSH	*dsh,
	QEN_NODE *node)

{
    QEN_STATUS	*qen_status = dsh->dsh_xaddrs[node->qen_num]->qex_status;
    QEF_KEY	*qef_key;
    QEF_KOR	*qef_kor;
    QEF_KAND	*qef_kand;
    QEF_KATT	*qef_katt;
    QEN_TKEY	*tkptr;
    QEN_NOR	*orkptr;
    QEN_NKEY	*nkptr;
    QEN_NOR	**lastnor;
    QEN_NKEY	**lastnkey;
    ULM_RCB	ulm;
    DB_STATUS	status;
    i4		key_len;
    i4		orcount, i;
    bool	readback = (node->node_qen.qen_orig.orig_flag & ORIG_READBACK);

    /* Take memory from DSH's stream */
    STRUCT_ASSIGN_MACRO(Qef_s_cb->qef_d_ulmcb, ulm);
    ulm.ulm_streamid = dsh->dsh_streamid;
    qef_key = node->node_qen.qen_orig.orig_pkeys;

    /* allocate key storage structures */
    /* first allocate the list header QEN_TKEY */
    ulm.ulm_psize = sizeof (QEN_TKEY);
    key_len  = qef_key->key_width;
    if (status = qec_malloc(&ulm))
    {
	dsh->dsh_error.err_code = ulm.ulm_error.err_code;
	return (status);
    }
    tkptr = (QEN_TKEY *) ulm.ulm_pptr;
    i = node->node_qen.qen_orig.orig_keys;
    dsh->dsh_cbs[i] = (PTR) tkptr;

    tkptr->qen_nkey = (QEN_NOR *)NULL;
    tkptr->qen_keys = (QEN_NOR *)NULL;
    lastnor = (QEN_NOR **) &tkptr->qen_keys;

    /* allocate QEN_NOR for QEF_KORs */
    for (qef_kor = qef_key->key_kor, orcount = 0; qef_kor;
	 qef_kor = qef_kor->kor_next, orcount++)
    {
	ulm.ulm_psize = sizeof (QEN_NOR);
	if (status = qec_malloc(&ulm))
	{
	    dsh->dsh_error.err_code = ulm.ulm_error.err_code;
	    return (status);
	}
	orkptr = (QEN_NOR *) ulm.ulm_pptr;
	orkptr->qen_next = (QEN_NOR *)NULL;
	orkptr->qen_key	= (QEN_NKEY *)NULL;
	orkptr->qen_kor = qef_kor;
	dsh->dsh_kor_nor[qef_kor->kor_id] = orkptr;		
	/* append to the list in QEN_TKEY */
	*lastnor = orkptr;
	lastnor = (QEN_NOR **) &orkptr->qen_next;
	lastnkey = (QEN_NKEY **)&orkptr->qen_key;

	/* allocate QEN_NKEY for QEF_KAND */
	for (qef_kand = qef_kor->kor_keys; qef_kand;
	     qef_kand = qef_kand->kand_next)
	{
	    ulm.ulm_psize = sizeof (QEN_NKEY);
	    if (status = qec_malloc(&ulm))
	    {
		dsh->dsh_error.err_code = ulm.ulm_error.err_code;
		return (status);
	    }
	    nkptr = (QEN_NKEY *) ulm.ulm_pptr;
	    nkptr->nkey_att = (QEF_KATT *)NULL;
	    nkptr->nkey_next = (QEN_NKEY *)NULL;

	    /* append it to the list in QEN_NOR */
	    *lastnkey = nkptr;
	    lastnkey = (QEN_NKEY **) &nkptr->nkey_next;
	}
    }

    /* Finally, call qeq_ksort to allocate and fill the DSH key
    ** structures for this node/thread pair. */

    status = qeq_ksort(tkptr, qef_key, dsh, TRUE, readback);

    if ( status == E_DB_OK )
	qen_status->node_u.node_orig.node_keys = tkptr->qen_nkey;

    return(status);
}

/*** Name: QEN_ORIGAGG  - Simple Aggregate Optimization spl node
**
** Description:
**      The simple aggregate handling has potential to perform better
**  when dealing with a simple table. Unlike the data flow in the ORIG_NODE
**  the computed results are returned back in the allocated buffer and the
**  decision on the data handling and computation of the results are handled
**  in DMF. By passing the ADF EXCB block with the compile instructions, all
**  the types could be handled with DMF using specialized modes based on the
**  AGLIM flags set in the dmr_cb.
**
**  On return from DMF, the status will be checked in the SAGG node to 
**  if there was an error. The error could be in the ADF code and that
**  has to be handled for ADF error processing. On examination of the return
**  error codes,it has been found that with aggregate handling, all ADF 
**  errors will cause termination of the query. The current implementation
**  will only work in this mode unlike the ORIG node that will allow the
**  node to fetch more rows.
**
**  The qualification is passed onto to DMF for use in restricting the rows
**
**  This node is only invoked under the following conditions
**	1) Called from a SAGG action
**      2) It is an ORIG node below the SAGG action
**	3) The bit AHD_ANY in ahd_qepflag is not on
**         (solo any aggregate)
**
**  The relation being accessed has already been opened and the first
**  key has been positioned.
**
**
** Inputs:
**      node                            the orig node data structure
**      rcb                             request block
**          .qef_cb.qef_dsh             the data segment header
**
** Outputs:
**      rcb
**          .error.err_code             one of the following
**                                      E_QE0000_OK
**                                      E_QE0015_NO_MORE_ROWS
**                                      E_QE0019_NON_INTERNAL_FAIL
**                                      E_QE0002_INTERNAL_ERROR
**
**      Returns:
**          E_DB_{OK, WARN, ERROR, FATAL}
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
**
** History:
**      21-aug-95 (ramra01)
**          written based on the qen_orig node 
**      16-jul-96 (sarjo01)
**          Bug 76243:  copy ADF warning counters from RCB
**          used by DMF to the one passed in from QEF
**	30-jul-96 (nick)
**	    Remove direct calls to DMF - this is illegal.  Get the result
**	    from the DMR_CB. #76243, #78024
**	11-Feb-1997 (nanpr01)
**	    Bug 80636 : Causes random segv because in dmf the adf_cb is not
**	    initialized. QEF ADF_CB is properly initialized and the dmf
**	    ADF_CB should be initialized from that.
**	02-mar-97 (hanal04)
**	    Picking ADI_WARN up from the DMR_CB result area (re: 30-jul-96)
**          causes SIGSEGV when we are not performing aggregates because in
**          these cases rcb->qef_adf_cb is null. (Bug# 81349).
**	27-jul-98 (nicph02)
**          Error status of the dmf_call was not passsed back to dsh->dsh_error
**          (bug 88649)
**	20-nov-03 (inkdo01)
**	    Add support for "open" call to allow parallel query initialization.
**	8-apr-04 (inkdo01)
**	    Added dsh parm to QEF_CHECK_FOR_INTERRUPT macro.
**	04-Nov-2004 (jenjo02)
**	    Only CSswitch on root DSH or if not OS-threaded.
**	13-Feb-2007 (kschendel)
**	    Replace CSswitch with better cancel check.
*/

DB_STATUS
qen_origagg(
QEN_NODE           *node,
QEF_RCB            *rcb,
QEE_DSH		   *dsh,
i4		    function )
{
    QEF_CB	    *qef_cb = dsh->dsh_qefcb;
    DMR_CB	    *dmr_cb = (DMR_CB*) dsh->dsh_cbs[node->node_qen.qen_orig.orig_get];
    QEN_STATUS	    *qen_status = dsh->dsh_xaddrs[node->qen_num]->qex_status;
    DB_STATUS	    status = E_DB_OK;
    bool	    reset = ((function & FUNC_RESET) != 0);
    TIMERSTAT	    timerstat;

#ifdef xDEBUG
    /* (VOID) qe2_chk_qp(dsh); */
#endif

    /* Do open processing, if required. Only if this is the root node
    ** of the query tree do we continue executing the function. */
    if ((function & TOP_OPEN || function & MID_OPEN) && 
	!(qen_status->node_status_flags & QEN1_NODE_OPEN))
    {
	qen_status->node_status_flags |= QEN1_NODE_OPEN;
	dsh->dsh_adf_cb->adf_errcb.ad_errclass = 0;
	dsh->dsh_adf_cb->adf_errcb.ad_usererr = 0;
	if (function & MID_OPEN)
	    return(E_DB_OK);
	function &= ~TOP_OPEN;
    }

    /* Do close processing, if required. */
    if (function & FUNC_CLOSE)
    {
	/* Nothing to see here, move along... */
	qen_status->node_status_flags &= ~QEN1_NODE_OPEN;
	qen_status->node_status_flags |= QEN8_NODE_CLOSED;
	return(E_DB_OK);
    }


    /* Check for cancel, context switch if not MT */
    CScancelCheck(dsh->dsh_sid);
    if (QEF_CHECK_FOR_INTERRUPT(qef_cb, dsh) == E_DB_ERROR)
	return (E_DB_ERROR);

    dsh->dsh_error.err_code = E_QE0000_OK;

    /* If the trace point qe90 is turned on then gather cpu and dio stats */
    if (dsh->dsh_qp_stats)
    {
	qen_bcost_begin(dsh, &timerstat, qen_status);
    }
 
    /* 
    ** Only do reset when qen_status->node_status != QEN0_INITIAL, that's 
    ** because if status is QEN0_INITIAL, qeq_validate already reset
    ** this node. 
    */  
    if (reset && qen_status->node_status != QEN0_INITIAL)
    {
	status = qen_reset(dsh, node, qen_status);
	if (status != E_DB_OK)
	{
#ifdef xDEBUG
	    /* (VOID) qe2_chk_qp(dsh); */
#endif

	    if (dsh->dsh_qp_stats)
	    {
		qen_ecost_end(dsh, &timerstat, qen_status);
	    }

	    return(status);
	}
    }

    /* if no more rows, return */
    if (qen_status->node_status == QEN4_NO_MORE_ROWS)
    {
	dsh->dsh_error.err_code = E_QE0015_NO_MORE_ROWS;
#ifdef xDEBUG
	/* (VOID) qe2_chk_qp(dsh); */
#endif

	if (dsh->dsh_qp_stats)
	{
	    qen_ecost_end(dsh, &timerstat, qen_status);
	}

	return (E_DB_WARN);
    }

    dmr_cb->dmr_flags_mask = DMR_NEXT;
    /* Roll warnings into ADF-CB used by qef for warning check */
    dmr_cb->dmr_qef_adf_cb = (PTR) dsh->dsh_qefcb->qef_adf_cb;
    
    /* ensure we go to qen_dmr_cx() with 0 err_code */
    dsh->dsh_error.err_code = E_QE0000_OK;  

    dmr_cb->dmr_res_data = NULL;
    /* get the positioned tuple */
    status = dmf_call(DMR_AGGREGATE, dmr_cb);
    if (status != E_DB_OK)
    {
        dsh->dsh_error.err_code = dmr_cb->error.err_code;
	if (dmr_cb->error.err_code == E_DM0023_USER_ERROR
	  && dsh->dsh_qp_ptr->qp_oerrorno_offset != -1)
	{
	    i4 *iierrorno;

	    /* DMF handled a row-qualifier user error, but we still need
	    ** to set iierrornumber while we have the info.
	    */
	    iierrorno = (i4 *) (
		(char *) (dsh->dsh_row[dsh->dsh_qp_ptr->qp_rerrorno_row])
			 + dsh->dsh_qp_ptr->qp_oerrorno_offset);
	    *iierrorno = dmr_cb->error.err_data;
	}
    }

    if (dmr_cb->dmr_res_data)
    {
	MEcopy(dmr_cb->dmr_res_data, sizeof(ADI_WARN),
		&dsh->dsh_adf_cb->adf_warncb);
    }

    qen_status->node_rcount = dmr_cb->dmr_count;

    if ((status != E_DB_OK) || (dsh->dsh_error.err_code != E_QE0000_OK))
    {
#ifdef xDEBUG
        /* (VOID) qe2_chk_qp(dsh); */
#endif
        if (dmr_cb->error.err_code == E_DM0074_NOT_POSITIONED)    
            goto exit;
	if (dsh->dsh_qp_stats)
        {
            qen_ecost_end(dsh, &timerstat, qen_status);
        }
 
        return(status);
    }


#ifdef xDEBUG
    /* (VOID) qe2_chk_qp(dsh); */
#endif

exit:
    /* we have no new keys, we are at EOF */
    dsh->dsh_error.err_code = E_QE0015_NO_MORE_ROWS;
    qen_status->node_status = QEN4_NO_MORE_ROWS;
    status = E_DB_WARN;

    if (dsh->dsh_qp_stats)
    {
	qen_ecost_end(dsh, &timerstat, qen_status);
    }

    return (status);
}

/* Name: setup_groupmap - Set up grouped physical partition bitmap
**
** Description:
**
**	An orig under a partition compatible (PC) join needs to emit
**	partition rows in groups, with an END-OF-GROUP marker status
**	in between each group.  When combined with partition qualification,
**	things can get tricky.  Not only might some groups be empty,
**	but what looks like an end-of-group might in fact be end-of-file
**	(if no more groups contain qualified partitions).
**
**	It's worth going to some extra effort to properly distinguish
**	EOG and EOF, since at EOF the upper levels of the execution tree
**	might well be able to avoid doing expensive sort or hash-build
**	operations on the other input(s).  To that end, when grouped
**	partition access is also qualified, we'll build a bitmap of
**	physical partitions that qualify and also are to be read by
**	this thread.  That makes checking for EOF relatively easy,
**	as long as bits are turned off when partitions are read.  (When
**	the bitmap becomes empty, it's EOF.)
**
**	This routine sets up the initial physical partition group bitmap
**	for this thread.  Upon entry, the node status info has been
**	initialized for the first logical partition of the first group
**	to be read.  We'll build a bitmap of physical partitions that
**	the thread will read, and then simply AND that with the qualification
**	bitmap.
**
**	Each logical partition consists of <stride> consecutive physical
**	partitions, followed by a gap of <nparts*stride> PP's.  This recurs
**	until we fall off the end of the table.  Each group consists
**	of <part_gcount> non-consecutive logical partitions, starting at
**	logical partition <part_gstart> and incrementing by
**	<nparts / part_gcount> to reach the next LP in the group.
**	The thread does <part_groupsleft> groups unless we advance
**	the starting point past the dimension's nparts first.
**
**	To help visualize the logical vs physical stuff, here's a picture.
**	(The term "logical partition" is slightly ambiguous, in that
**	it might mean one subpartition of a higher logical partition, or
**	it might mean all subpartitions in that position of all higher
**	logical partitions.  In the orig code, we always mean the latter.)
**
**	Consider a 3-dimensional partitioning, with 2, 3, and 4 partitions
**	per dimension.  Thus, there are 2 * 3 * 4 = 24 physical partitions.
**	Let's call the outermost dimension's partitions A and B;
**	the middle dimension is 1 2 and 3;  the inner dimension is a b c d.
**
**	For the outermost dimension, nparts = 2, stride = 3*4 = 12, and there
**	are totparts/nparts = 12 PP's per logical partition:
**	    A A A A A A A A A A A A B B B B B B B B B B B B
**
**	For the middle dimension, nparts = 3, stride = 4, and there are
**	24 / 3 = 8 PP's per logical partition:
**	    1 1 1 1 2 2 2 2 3 3 3 3 1 1 1 1 2 2 2 2 3 3 3 3
**
**	and for the innermost dimension, nparts = 4, stride = 1, and there
**	are 24/4 = 6 PP's per logical partition:
**	    a b c d a b c d a b c d a b c d a b c d a b c d
**
**	Compare the second logical partition of each dimension (B, 2, and b):
**	    . . . . . . . . . . . . B B B B B B B B B B B B
**	    . . . . 2 2 2 2 . . . . . . . . 2 2 2 2 . . . .
**	    . b . . . b . . . b . . . b . . . b . . . b . .
**
** Inputs:
**	ns		QEN_STATUS * state info for this node and thread
**	partp		QEN_PART_INFO * partition info for orig
**	qpmap		Pointer to qualification bitmap (expressed in
**			physical partitions), might be NULL
**	groupmap	Pointer to groupmap to be set up
**
** Outputs:
**	groupmap set up as described above.
**
** Returns:
**	nothing
**
** History:
**	22-Jul-2005 (schka24)
**	    Written.
**	27-Jul-2007 (kschendel) SIR 122513
**	    Minor adjustments for new groupmap location.
*/

static void
setup_groupmap(QEN_STATUS *ns, QEN_PART_INFO *partp, PTR qpmap, PTR groupmap)
{
    i4 gstart;			/* Group starting logical partition */
    i4 lpartno;			/* Current logical partition number */
    i4 lpstride;		/* Group stride, ie nparts / gcount */
    i4 i, j, k;
    i4 nparts;			/* Number of LP's in PC-join dimension */
    i4 ppartno;			/* Current physical partition position */
    i4 stride;			/* PP's per LP */
    i4 totparts;		/* Size of bitmap in bits */

    totparts = partp->part_totparts;
    MEfill((totparts + BITSPERBYTE - 1) / BITSPERBYTE, 0, groupmap);
    gstart = ns->node_u.node_orig.node_gstart;
    stride = partp->part_pcdim->stride;
    nparts = partp->part_pcdim->nparts;
    lpstride = nparts / partp->part_gcount;
    /* Each thread is assigned consecutive groups.
    ** For each group that this thread should do...
    */
    for (i = ns->node_u.node_orig.node_groupsleft; i > 0; --i)
    {
	lpartno = gstart;
	/* For each LP making up this group... */
	for (j = partp->part_gcount; j > 0; --j)
	{
	    /* Set bits for this logical partition */
	    ppartno = lpartno * stride;
	    do
	    {
		for (k = stride; k > 0; --k)
		{
		    BTset(ppartno, groupmap);
		    ++ppartno;
		}
		ppartno = ppartno + (nparts - 1) * stride;
	    } while (ppartno < totparts);
	    lpartno += lpstride;
	    if (lpartno >= nparts)
		break;
	}
	/* Advance to next group, stop if we fall off the end */
	if (++gstart >= nparts)
	    break;
    }
    /* Now apply qualifications */
    if (qpmap != NULL)
	BTand(totparts, qpmap, groupmap);

} /* setup_groupmap */

/* Name: advance_group - Advance to next physical partition of PC-join group
**
** Description:
**
**	This routine advances to the next physical partition of a
**	PC-join group, which is <part_gcount> logical partitions
**	starting at <node_gstart>.
**
**	When called, we've just finished dealing with physical partition
**	<node_ppart_num>, which may or may not be a real qualified partition.
**	In any case, we clear the group-partition bit for the PP just
**	finished, and count it off.  The next physical partition for the
**	current logical partition that has a group-map bit set is
**	then calculated.  If there aren't any, the logical partition
**	is counted off, and we repeat with the next logical partition
**	of the group.  If end-of-group is reached without finding a
**	qualified partition, -1 is returned.
**
**	Upon return, all the node_xxx things are updated to reflect the
**	newly current physical and logical partition.
**
**	Note that we could almost certainly do the calculations without
**	keeping track of logical partitions.  I'm leaving the logical
**	partition stuff in for now, since it might help debugging.
**
**	Programmer's note:  I tried writing the loop without the goto,
**	and it's just simply way more obvious and easy to read this way.
**	The first time into the loop, we need to advance the current
**	PP, but after moving to the next logical partition we need to
**	test the LP's first PP *without* advancing.  Yes, you can write
**	a goto-less loop by duplicating the test-and-advance the first
**	time in, but it's stupid and ugly.
**
** Inputs:
**	ns		QEN_STATUS * pointer to state for node and thread
**	partp		QEN_PART_INFO * pointer to orig partitioning info
**	groupmap	Pointer to qualified-group bitmap (might be null)
**			(Qualifying PP's for this thread)
**
** Outputs:
**	Returns new physical partition number, or -1 if end-of-group.
**	Group-map and counters in QEN_STATUS are updated.
**
** History:
**	22-Jul-2005 (schka24)
**	    Written.
**	27-Jul-2007 (kschendel)
**	    Adjust call sequence for new groupmap location.
*/

static i4
advance_group(QEN_STATUS *ns, QEN_PART_INFO *partp, PTR groupmap)
{
    i4 nparts;			/* Logical partitions in dimension */
    i4 ppartno;
    i4 stride;

    ppartno = ns->node_ppart_num;
    stride = partp->part_pcdim->stride;
    nparts = partp->part_pcdim->nparts;
    if (groupmap != NULL)
	BTclear(ppartno, groupmap);	/* We did this one */

    /* Loop to find next qualified PP in current LP */
    while (-- ns->node_u.node_orig.node_partsleft > 0)
    {
	/* More PP's in this logical partition, compute the next one. */
	++ppartno;
	if ((ppartno % stride) == 0)
	    ppartno += stride * (nparts - 1);	/* Mind the gap */
	/* Loop back here if we have to advance a logical partition */
test_this:
	if (groupmap == NULL || BTtest(ppartno, groupmap) )
	{
	    ns->node_ppart_num = ppartno;
	    return (ppartno);
	}
    }
    /* Reached end of this logical partition, move to next.  The
    ** next LP isn't consecutive, we instead skip (nparts/gcount)
    ** LP's to get to the next one.
    */
    if (-- ns->node_u.node_orig.node_grouppl <= 0)
	return (-1);
    ns->node_u.node_orig.node_lpart_num += (nparts / partp->part_gcount);
    ns->node_u.node_orig.node_partsleft = partp->part_totparts / nparts;
    ppartno = ns->node_u.node_orig.node_lpart_num * stride;
    /* Test this PP to see if it's qualified, move through the LP
    ** if necessary.
    */
    goto test_this;

} /* advance_group */

/* Name: advance_next_group - Advance to start of next PC-join group
**
** Description:
**	This routine advances the node position to the start of
**	the next PC-join group.  TRUE is returned if it worked,
**	and FALSE is returned if there is no next group.
**
**	Threads are assigned consecutive PC-join groups, so all
**	we have to do is reset the current logical partition to
**	one past where we started the last group;  and then
**	set up the physical partition position, and various left-to-do
**	counters.
**
**	We do NOT check that the position returned is a qualified
**	physical partition.  It's up to the caller to do that.
**
** Inputs:
**	ns		QEN_STATUS * pointer to state for node and thread
**	partp		QEN_PART_INFO * pointer to orig partitioning info
**
** Outputs:
**	Returns TRUE normally, FALSE if reached EOF.
**
** History:
**	17-Aug-2005 (schka24)
**	    Written.
*/

static bool
advance_next_group(QEN_STATUS *ns, QEN_PART_INFO *partp)
{
    i4 lpartno;
    i4 nparts;			/* # logical partitions in group dimension */

    if (--ns->node_u.node_orig.node_groupsleft <= 0)
    {
	/* Looks like EOF */
	return (FALSE);
    }
    nparts = partp->part_pcdim->nparts;
    /* Set to first logical part of next consecutive group */
    lpartno = ++ ns->node_u.node_orig.node_gstart;
    if (lpartno > nparts)
	return (FALSE);
    ns->node_u.node_orig.node_lpart_num = lpartno;
    ns->node_u.node_orig.node_grouppl = partp->part_gcount;
    /* Set to first physical partition of that logical partition */
    ns->node_ppart_num = lpartno * partp->part_pcdim->stride;
    /* Physical partitions per logical partition: */
    ns->node_u.node_orig.node_partsleft = partp->part_totparts / nparts;

    return (TRUE);
}
