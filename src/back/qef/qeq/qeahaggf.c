/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <sr.h>
#include    <st.h>
#include    <me.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <hshcrc.h>
#include    <ddb.h>
#include    <ulf.h>
#include    <ulm.h>
#include    <adf.h>
#include    <ade.h>
#include    <scf.h>
#include    <dmf.h>
#include    <dmhcb.h>
#include    <dmrcb.h>
#include    <dmtcb.h>
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
#include    <qefqeu.h>

#include    <dmmcb.h>
#include    <dmucb.h>
#include    <ex.h>
#include    <tm.h>

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
**  Name: QEAHAGGF.C - return aggregate function row (using hash
**	aggregation)
**
**  Description:
**      This action returns the aggregate result tuple instead of
**  appending it to a file. It also does the aggregation using hash
**  techniques, rather than requiring the source data to be presorted 
** on the grouping expressions
**
**          qea_haggf - return aggregate function row (hash aggregation)
**
**
**  History:    $Log-for RCS$
**      26-jan-01 (inkdo01)
**          Written (for hash aggregation project).
**	24-sep-01 (devjo01)
**	    Modify space requirements calculation to allow for 64 bit
**	    pointers. (b105879).
**	01-oct-2001 (devjo01)
**	    Rename QEN_HASH_DUP to QEF_HASH_DUP.
**	05-oct-2001 (somsa01)
**	    Found one more spot where QEN_HASH_DUP had to be renamed
**	    to QEF_HASH_DUP.
**	11-oct-2001 (devjo01)
**	    Add "static" to forward declaration of local static funcs.
**	23-feb-04 (inkdo01)
**	    Changed qef_rcb->error to dsh->dsh_error for || thread safety
**	    (and others).
**	5-May-2005 (schka24)
**	    Use sizeof(PTR) for chunk link ptr size, not align-restrict
**	    (which could conceivably be something smaller than a pointer).
**	    Hash values must be unsigned (they can be -infinity).
**	    Kill a few more incorrect u_i2 casts on MEcopy sizes.
**	7-Jun-2005 (schka24)
**	    Break out memory allocator into a routine; apply recursion
**	    level entry/exit fixes from hash join.
**	    Include inkdo01 changes to delete temp files as they complete.
**	13-Oct-2005 (hweho01)
**	    Modified the chunkoff setting, so it will be rounded up to  
**          the sizeof(ALIGN_RESTRICT), avoid seg V on HP 32-bit build.
**          Since the pointer is 4-bytes, the float type number must be  
**          aligned at 8 byte boundary.
**	28-Dec-2005 (kschendel)
**	    Use DSH references whenever possible instead of QEF RCB
**	    references.  (This simplifies the code, eliminating many
**	    RCB references, and reflects the new primacy of the DSH
**	    thanks to the parallel query changes.)
**	7-Jul-2008 (kschendel) SIR 122512
**	    Make partition read and write buffer sizes independent.
**	    Allow work files to have a page size that is a multiple of
**	    HASH_PGSIZE;  this helps out the optional page compression.
**	    Rename htmax -> grpmax since that is what it really is.
**	    Hash join spill files have been unified, reflect changes here
**	    even though hash agg doesn't use two-sided spill.
**	    Use simple RLL compression on rows being spilled, to reduce
**	    spill size for giant aggregations.  Tidy up code a little.
[@history_template@]...
**/


/**
**
**  Name: HSHAG_STATE - local state structure to reduce parameter passing
**
**  Description:
**      This structure contains various variables local to the hash
**	aggregation functions. It is allocated in the locals of qea_haggf
**	passed to the other local functions. It reduces the parameter
**	passing overhead to these functions.
**
[@history_template@]...
**/

typedef struct _HSHAG_STATE {
    QEN_ADF	*curradf;
    ADE_EXCB	*hashexcb;
    ADE_EXCB	*currexcb;
    ADE_EXCB	*oflwexcb;	/* INIT materializes overflow so that it
				** can be spilled; MAIN aggregates from
				** an overflow row (as opposed to an input
				** row as currexcb's CX does).
				*/
    ADE_EXCB	*accum_excb;	/* EXCB for accumulating results in insert;
				** either currexcb (load1) or oflwexcb (load2)
				*/
    PTR		*hbufpp;
    PTR		*wbufpp;
    PTR		*obufpp;
    PTR		*owbufpp;
    PTR		*accum_wbufpp;	/* Where to stash work buffer for accum */
    PTR		*accum_obufpp;	/* Where to store overflow buffer for accum */
    QEF_HASH_DUP *hashbufp;
    PTR		curchunk;
    PTR		prevchunk;
    PTR		oflwbufp;
    i4		chunkoff;
    i4		chunk_count;
    i4		currgcount;
} HSHAG_STATE;

GLOBALREF QEF_S_CB *Qef_s_cb;

static DB_STATUS
qea_haggf_init(
QEE_DSH		*dsh,
QEN_HASH_AGGREGATE *haptr,
QEF_QEP		*qp);

static DB_STATUS
qea_haggf_load1(
QEF_RCB		*qef_rcb,
QEE_DSH		*dsh,
QEF_AHD		*action,
QEN_HASH_AGGREGATE *haptr,
HSHAG_STATE	*hstp,
i4		function);

static DB_STATUS
qea_haggf_load2(
QEE_DSH		*dsh,
QEF_AHD		*action,
QEN_HASH_AGGREGATE *haptr,
HSHAG_STATE	*hstp);

static DB_STATUS
qea_haggf_insert(
QEE_DSH		*dsh,
QEN_HASH_AGGREGATE *haptr,
HSHAG_STATE	*hstp);

static DB_STATUS
qea_haggf_overflow(
QEE_DSH		*dsh,
QEN_HASH_AGGREGATE *haptr,
HSHAG_STATE	*hstp,
u_i4		hashno);

static DB_STATUS
qea_haggf_recurse(
QEE_DSH		*dsh,
QEN_HASH_AGGREGATE *haptr);

static DB_STATUS
qea_haggf_write(
QEE_DSH		*dsh,
QEN_HASH_AGGREGATE *haptr,
QEN_HASH_FILE	*hfptr,
PTR		buffer);

static DB_STATUS
qea_haggf_read(
QEN_HASH_AGGREGATE *haptr,
QEN_HASH_FILE	*hfptr,
PTR		buffer);

static DB_STATUS
qea_haggf_open(
QEE_DSH		*dsh,
QEN_HASH_AGGREGATE *haptr,
QEN_HASH_PART	*hpptr);
 
static DB_STATUS
qea_haggf_close(
QEN_HASH_AGGREGATE *haptr,
QEN_HASH_FILE	**hfptr,
i4		flag);
/* qea_haggf_close flag settings */
#define CLOSE_RELEASE	1	
#define CLOSE_DESTROY	2


static DB_STATUS
qea_haggf_flush(
QEE_DSH		*dsh,
QEN_HASH_AGGREGATE *haptr);

static DB_STATUS qea_haggf_palloc(
	QEF_CB *qef_cb,
	QEN_HASH_AGGREGATE *haptr,
	SIZE_TYPE psize,
	void *pptr,
	DB_ERROR *dsh_error,
	char *what);

static void qea_haggf_cleanup(
	QEE_DSH *dsh,
	QEN_HASH_AGGREGATE *haptr);
 
static void haggf_stat_dump(
	QEE_DSH *dsh,
	QEF_AHD *act,
	QEN_HASH_AGGREGATE *haptr);

/*{
** Name: QEA_HAGGF	- compute aggregate function
**
** Description:
**      The following algorithm is used to produce the aggregate result
**  tuple. It does so on unordered input and produces no-project aggregates.
**
**	1) Allocate memory for hash table and structures.
**	2) Read each tuple in turn from the source QEP.
**	3) Hash the by expressions and search the hash table for a matching
**	    row.
**	4) If a match is found, accumulate the aggregate result into it, else
**	    init new aggregate row and add to hash table.
**	4.5) If rows with distinct group expression values exceed storage
**	    available, remaining non-matching rows are partitioned into
**	    buffers that may eventually be written to disk.
**	5) When input rows are done, read rows out of hash table, performing
**	    final aggregation on each and returning to caller.
**	5.5) If some rows were written to disk, hash table is reinitialized
**	    and overflow rows are read from disk and loaded into hash table. 
**	    Steps 3 thru 5 are repeated for overflow rows, with the 
**	    possibility that enough overflow rows may even cause recursive
**	    partitioning.
**
** Inputs:
**      action                          the QEA_HAGGF action
**	qef_rcb
**	function			FUNC_RESET or zero
**	first_time			TRUE if first time call, seems
**					redundant with reset?
**
** Outputs:
**      dsh
**	    .error.err_code		one of the following
**				E_QE0000_OK
**				E_QE000D_NO_MEMORY_LEFT
**				E_QE0017_BAD_CB
**				E_QE0018_BAD_PARAM_IN_CB
**				E_QE0019_NON_INTERNAL_FAIL
**				E_QE0002_INTERNAL_ERROR
**				E_QE0015_NO_MORE_ROWS
**	Returns:
**	    E_DB_{OK, WARN, ERROR, FATAL}
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      26-jan-01 (inkdo01)
**          Written (for hash aggregation project).
**	23-feb-01 (inkdo01)
**	    Add overflow support.
**	2-oct-01 (inkdo01)
**	    Changes to assure chunk link leaves chunk 8-byte aligned.
**	29-dec-03 (inkdo01)
**	    DSH is now parameter, "function" replaces "reset".
**	27-feb-04 (inkdo01)
**	    Changed dsh_hashagg from array of QEN_HASH_AGGREGATE to array
**	    of ptrs.
**	31-aug-04 (inkdo01)
**	    Changes to support global base arrays.
**	20-Sep-2004 (jenjo02)
**	    Reset hshag_dmhcb; with child threads, dsh_cbs
**	    aren't what they used to be when qee initialized them.
**	10-Apr-2007 (kschendel) SIR 122513
**	    Pass reset to QP underneath when doing PC-aggregation, in order
**	    to support PC-agg of PC-join.
*/
DB_STATUS
qea_haggf(
QEF_AHD		    *action,
QEF_RCB		    *qef_rcb,
QEE_DSH		    *dsh,
i4		    function,
bool		    first_time )
{
    QEN_NODE	*node = action->qhd_obj.qhd_qep.ahd_qep;
    DB_STATUS	status = E_DB_OK;
    ADF_CB	*adfcb = dsh->dsh_adf_cb;
    PTR		*cbs = dsh->dsh_cbs;
    HSHAG_STATE hastate;
    QEF_HASH_DUP *workbufp;
    QEN_HASH_AGGREGATE	*haptr;
    i4		i;
    bool	reset = ((function & FUNC_RESET) != 0);
    bool	globalbase = (dsh->dsh_qp_ptr->qp_status & QEQP_GLOBAL_BASEARRAY) != 0;

    /* Haggf is always under a QP which eats the top-open function.
    ** Regenerate top-open for our subplan.
    */
    if (first_time && reset)
	function |= TOP_OPEN;

    /* Init a bunch of stuff. */
    dsh->dsh_error.err_code = E_QE0000_OK;
    dsh->dsh_qef_rowcount = 0;
    hastate.curradf = action->qhd_obj.qhd_qep.ahd_current;
    hastate.currexcb = (ADE_EXCB *)cbs[hastate.curradf->qen_pos];
    hastate.wbufpp = globalbase ?
	&(dsh->dsh_row[action->qhd_obj.qhd_qep.u1.s2.ahd_agwbuf]) :
	&(hastate.currexcb->excb_bases[action->qhd_obj.qhd_qep.u1.s2.ahd_agwbuf]);
    haptr = dsh->dsh_hashagg[action->qhd_obj.qhd_qep.u1.s2.ahd_agcbix];
    haptr->hshag_dmhcb = cbs[action->qhd_obj.qhd_qep.u1.s2.ahd_agdmhcb];

    /* This outer loop is only needed for partition-compatible aggs.
    ** When EOG is returned from below, the aggregation is flushed out;
    ** then, "first-time" is turned (back) on so that we re-init the agg
    ** for the next group.
    */
    for (;;)
    {
	if (first_time || reset)
	{
	    /* Init. hash table, etc. */
	    status = qea_haggf_init(dsh, haptr, &action->qhd_obj.qhd_qep);
	    if (status != E_DB_OK)
	    {
		/* the error.err_code field in dsh should already be
		** filled in.
		*/
		dsh->dsh_qef_rowcount = 0;
		return (status);
	    }
	}

	/* Loop over input rows and aggregate into hash table. */
	if (haptr->hshag_flags & HASHAG_LOAD1)
	{
	    hastate.curchunk = haptr->hshag_1stchunk;
	    hastate.prevchunk = NULL;
	    hastate.chunk_count = 1;
	    hastate.chunkoff = (sizeof(PTR) + DB_ALIGN_SZ - 1) &
			       (~(DB_ALIGN_SZ - 1));
	    status = qea_haggf_load1(qef_rcb, dsh, action,
				    haptr, &hastate, function);
	    if (status != E_DB_OK)
	    {
		if (haptr->hshag_flags & HASHAG_EOG)
		{
		    haggf_stat_dump(dsh, action, haptr);
		    function = FUNC_RESET;  /* Reset QP underneath us */
		    continue;		/* Empty partition, cycle around */
		}
		return(status);
	    }
	}

	/* Big for-loop to force continued overflow processing when hash table
	** empties from current phase. Performs RETURN as long as there are 
	** completed rows remaining, then calls load2 to build next set of 
	** result rows from overflow (if any). */
	for ( ; ; )
	{
	    if (haptr->hshag_flags & HASHAG_RETURN)
	    {
		/* Locate next work row to produce result from. */
		workbufp = NULL;
		if (haptr->hshag_lastrow != NULL)
		    workbufp = haptr->hshag_lastrow->hash_next;
		while (workbufp == NULL)
		{
		    i = ++(haptr->hshag_curr_bucket);
		    if (i >= haptr->hshag_htsize)
			break;
		    workbufp = (QEF_HASH_DUP *)haptr->hshag_htable[i];
		}
    
		if (workbufp == NULL)
		{
		    if (!(haptr->hshag_flags & HASHAG_OVFLW) ||
			haptr->hshag_flags & HASHAG_DONE)
		    {
			/* We're all done! First close/destroy any open files.
			** Then, for end-of-group, exit the for-loop to
			** reset;  for regular EOF, return no-more-rows.
			*/
			haggf_stat_dump(dsh, action, haptr);
			status = qea_haggf_closeall(haptr);
			if (status != E_DB_OK)
			    return(status);
			if (haptr->hshag_flags & HASHAG_EOG)
			    break;
			qea_haggf_cleanup(dsh, haptr);
			dsh->dsh_error.err_code = E_QE0015_NO_MORE_ROWS;
			return(E_DB_WARN);
		    }
    
		    /* (More) overflow processing yet to be done. Reset flag 
		    ** and hash table search vars and loop. */
		    haptr->hshag_flags &= ~HASHAG_RETURN;
		    haptr->hshag_curr_bucket = -1;
		    haptr->hshag_lastrow = (QEF_HASH_DUP *) NULL;
		    continue;
		}

		haptr->hshag_lastrow = workbufp;
    
		/* Initialize the aggregate result for next output row. */
		if (hastate.curradf->qen_uoutput > -1)
		{
		    if (globalbase)
			dsh->dsh_row[hastate.curradf->qen_uoutput] =
				    dsh->dsh_qef_output->dt_data;
		    else
			hastate.currexcb->excb_bases[ADE_ZBASE + 
					hastate.curradf->qen_uoutput] =
				dsh->dsh_qef_output->dt_data;
		}
		*(hastate.wbufpp) = (PTR)workbufp;
    
		/* Locate next work row, compute final result and return. */
		hastate.currexcb->excb_seg = ADE_SFINIT;
		status = ade_execute_cx(adfcb, hastate.currexcb);
		if (status != E_DB_OK)
		{
		    if ((status = qef_adf_error(&adfcb->adf_errcb, 
			status, dsh->dsh_qefcb, &dsh->dsh_error)) != E_DB_OK)
			return (status);
		}
		dsh->dsh_qef_remnull |= hastate.currexcb->excb_nullskip;
		dsh->dsh_qef_rowcount = 1;
		return (E_DB_OK);
	    }
    
	    if (haptr->hshag_flags & (HASHAG_LOAD2P1 | HASHAG_LOAD2P2))
	    {
		/* Loop over overflow rows and aggregate into hash table. */
		hastate.curchunk = haptr->hshag_1stchunk;
		hastate.prevchunk = NULL;
		hastate.chunk_count = 1;
		hastate.chunkoff = (sizeof(PTR) + DB_ALIGN_SZ - 1) &
				   (~(DB_ALIGN_SZ - 1));
		status = qea_haggf_load2(dsh, action, haptr, &hastate);
		if (status != E_DB_OK)
		    return(status);
    
		if (haptr->hshag_flags & HASHAG_DONE)
		{
		    /* We're all done! First close/destroy any open files.
		    ** Then, for end-of-group, exit the for-loop to
		    ** reset;  for regular EOF, return no-more-rows.
		    */
		    haggf_stat_dump(dsh, action, haptr);
		    status = qea_haggf_closeall(haptr);
		    if (status != E_DB_OK)
			return(status);
		    if (haptr->hshag_flags & HASHAG_EOG)
			break;
		    qea_haggf_cleanup(dsh, haptr);
		    dsh->dsh_error.err_code = E_QE0015_NO_MORE_ROWS;
		    return(E_DB_WARN);
		}
	    }
	}	/* end inner for */
	/* We only get here if end-of-group is reached for a partition
	** compatible hash aggregation.  The entire previous group has
	** been flushed out now, so reset the aggregation and move on
	** for more input.
	*/
	function = FUNC_RESET;		/* Reset QP underneath us */
	first_time = TRUE;
	/* back around for an init... */
    } /* end outer for */
}


/*{
** Name: QEA_HAGGF_INIT	- allocate/format hash memory 
**
** Description:
**      This function allocates (if necessary) memory for the hash table
**	and first chunk for aggregate accumulation. It then inits all hash
**	table entries to NULL in preparation for the aggregation operation.
**
** Inputs:
**	dsh
**	haptr			Ptr to dynamic hash aggregation structure
**	hbufsz			Size of hash work buffer
**	obufsz			Size of overflow work buffer
**
** Outputs:
**      dsh
**	    .error.err_code		one of the following
**				E_QE0000_OK
**				E_QE000D_NO_MEMORY_LEFT
**	Returns:
**	    E_DB_{OK, WARN, ERROR, FATAL}
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      26-jan-01 (inkdo01)
**          Written (for hash aggregation project).
**	24-sep-01 (devjo01)
**	    Modify space requirements calculation to allow for 64 bit
**	    pointers. (b105879).
**	2-oct-01 (inkdo01)
**	    Changes to assure chunk link leaves chunk 8-byte aligned.
**	4-oct-01 (inkdo01)
**	    Rearranged memory to avoid ugly alignment code from above.
**	22-july-03 (inkdo01)
**	    Set dmh_tran_id. Used to be set in qee.c, but that was too early
**	    and the tran ID could change before query exec (bug 110603).
**	30-Jul-2004 (jenjo02)
**	    Use dsh_dmt_id rather than qef_dmt_id transaction context.
**	 3-nov-06 (hayke02)
**	    Initialise the hash partition table entries and hshag_flags. This
**	    prevents E_DM9713/EDM9712/E_QE0018 and incorrect results when a DBP
**	    uses multiple hash aggregations, and therefore reuse of the same
**	    memory for QEN_HASH_AGGREGATE and QEN_HASH_PART structures. This
**	    change fixes bug 117034.
[@history_template@]...
*/
static DB_STATUS
qea_haggf_init(
QEE_DSH		*dsh,
QEN_HASH_AGGREGATE *haptr,
QEF_QEP		*qp)

{
    QEF_CB	*qef_cb = dsh->dsh_qefcb;
    DB_STATUS	status = E_DB_OK;
    DMH_CB	*dmhcb = (DMH_CB *)haptr->hshag_dmhcb;
    QEN_HASH_LINK *hlink;
    i4		i;
    i4		hbufsz = haptr->hshag_hbufsz;
    i4		obufsz = haptr->hshag_obufsz;

    if (dsh->dsh_hash_debug)
    {
	TRdisplay("%@ hashagg init t%d htsize %d chunksize %d chmax %d\n",
		dsh->dsh_threadno,haptr->hshag_htsize,
		haptr->hshag_chsize, haptr->hshag_chmax);
	TRdisplay("t%d hashagg write size %d  read size %d  pcount %d, aggest %d\n",
		dsh->dsh_threadno,haptr->hshag_pbsize, haptr->hshag_rbsize,
		haptr->hshag_pcount, qp->u1.s2.ahd_aggest);
	TRdisplay("t%d hashagg wbufsz %d  hbufsz %d  obufsz %d  keysz %d\n",
		dsh->dsh_threadno, haptr->hshag_wbufsz, haptr->hshag_hbufsz,
		haptr->hshag_obufsz, haptr->hshag_keysz);
    }
    /* See if hash table needs allocation - if so, stream also needs
    ** to be opened. */
    if (haptr->hshag_htable == NULL)
    {
	/* now allocate hash table - single ULM call
	** also allocates initial chunk of aggregation memory, plus a 
	** buffer for running the hash computation and a buffer to hold
	** an overflow row (if it spans 2 blocks) and space to assure the
	** chunk linkage leaves chunks aligned on 8 byte boundaries.
	** FIXME!  I think that the second obufsz is bogus since we
	** allocate the workrows at qee time. (kschendel).  One obufsz
	** is for the hash-key area (bycompare result).
	*/
	status = qea_haggf_palloc(qef_cb, haptr,
			sizeof(PTR) * haptr->hshag_htsize
			+ haptr->hshag_chsize
			+ obufsz + obufsz
			+ (sizeof(PTR) + DB_ALIGN_SZ -1) & (~(DB_ALIGN_SZ -1)),
			&haptr->hshag_1stchunk,
			&dsh->dsh_error, "ht_ptrs+1stchunk");
	if (status != E_DB_OK)
	    return (status);
	haptr->hshag_htable = (PTR *)(haptr->hshag_1stchunk +
				(obufsz + obufsz + haptr->hshag_chsize));
					/* hash table follows 1stchunk and 
					** work buffers */
	haptr->hshag_challoc = 1;	/* for stats */
	((PTR **)haptr->hshag_1stchunk)[0] = NULL;
    }
    else
    {
	/* If hash table existed, open files might exist too; make sure
	** they're all closed and tossed.
	*/
	status = qea_haggf_closeall(haptr);
	if (status != E_DB_OK)
	    return (status);
    }

    /* Init. hash buckets to NULL. */
    for (i = 0; i < haptr->hshag_htsize; i++)
			haptr->hshag_htable[i] = NULL;

    /* In case this is a reset or a new partition group, clean out
    ** the top-level partition array.
    */
    hlink = haptr->hshag_currlink = haptr->hshag_toplink;
    for (i = 0; i < haptr->hshag_pcount; i++)
    {
	QEN_HASH_PART   *hpart = &hlink->hsl_partition[i];

	/* Reset stuff in existing partition array. */
	hpart->hsp_file = (QEN_HASH_FILE *) NULL;
	hpart->hsp_bstarts_at = 0;
	hpart->hsp_offset = 0;
	hpart->hsp_brcount = 0;
	hpart->hsp_bbytes = 0;
	hpart->hsp_flags  = 0;
    }
    hlink->hsl_flags = 0;
    hlink->hsl_pno = -1;

    haptr->hshag_rcount = 0;		/* reset for this group */
    haptr->hshag_gcount = 0;		/* Stats are per-group */
    haptr->hshag_ocount = 0;
    haptr->hshag_maxreclvl = 0;
    /* Clear all flags (such as EOG) except permanent ones, then set
    ** back to load1 phase.
    */
    haptr->hshag_flags &= HASHAG_KEEP_OVER_RESET;
    haptr->hshag_flags |= HASHAG_LOAD1;	/* indicate load phase */

    dmhcb->dmh_tran_id = dsh->dsh_dmt_id;

    return(E_DB_OK);
}


/*{
** Name: QEA_HAGGF_LOAD1 - phase I (from action header source node) load
**	of aggregate rows
**
** Description:
**      This function reads rows from the QEA_HAGGF source node and attempts
**	to insert/aggregate them into the hash table.
**
** Inputs:
**	qef_rcb
**	action			Ptr to hash aggregate action header
**	haptr			Ptr to dynamic hash aggregation structure
**	hstp			Ptr to hash aggregation state structure (to
**				reduce paramaters passed)
**	function		Reset/open function for below
**
** Outputs:
**	Sets EOG in hshag_flags if end-of-group from below
**      qef_rcb
**	    .error.err_code		one of the following
**				E_QE0000_OK
**				E_QE000D_NO_MEMORY_LEFT
**	Returns:
**	    E_DB_{OK, WARN, ERROR, FATAL}
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      26-mar-01 (inkdo01)
**          Split from qea_haggf as part of overflow processing.
**	21-nov-03 (inkdo01)
**	    Add open_flag parm to qea_func calls for parallel query support.
**	8-jan-04 (inkdo01)
**	    Fix parms to qea_func to conform to 29-dec changes.
**	31-aug-04 (inkdo01)
**	    Changes to support global base arrays.
**	27-Nov-2006 (kschendel) SIR 122512
**	    Use hash-join hasher and call by hand rather than from the CX.
**	    This is an attempt to avoid resonances with hash partitioning.
**	24-Jun-2008 (kschendel) SIR 122512
**	    Comment update re non-use of row length and compression.
[@history_template@]...
*/
static DB_STATUS
qea_haggf_load1(
QEF_RCB		*qef_rcb,
QEE_DSH		*dsh,
QEF_AHD		*action,
QEN_HASH_AGGREGATE *haptr,
HSHAG_STATE	*hstp,
i4		function)

{
    QEN_NODE	*node = action->qhd_obj.qhd_qep.ahd_qep;
    DB_STATUS	status = E_DB_OK;
    ADF_CB	*adfcb = dsh->dsh_adf_cb;
    PTR		*cbs = dsh->dsh_cbs;
    PTR		toss_ptr;
    QEF_HASH_DUP *rowptr;

    /* Init. various variables and enter read loop. */

    hstp->hashexcb = (ADE_EXCB *)cbs[action->qhd_obj.qhd_qep.ahd_by_comp->qen_pos];
    rowptr = hstp->hashbufp = (QEF_HASH_DUP *)(hstp->curchunk + haptr->hshag_chsize);

    /* Cause the "bycompare" CX to materialize hash keys into hashbufp */

    hstp->hbufpp = (dsh->dsh_qp_ptr->qp_status & QEQP_GLOBAL_BASEARRAY) ?
	&(dsh->dsh_row[action->qhd_obj.qhd_qep.u1.s2.ahd_aghbuf]) :
	&(hstp->hashexcb->excb_bases[action->qhd_obj.qhd_qep.u1.s2.ahd_aghbuf]);
    *(hstp->hbufpp) = (PTR)rowptr;

    /* Set up overflow-row materializer and row-address pointer too.  If
    ** load1 spills, the overflower will set a result address into *obufpp
    ** and the INIT segment of the overflow CX will materialize spill there.
    */

    hstp->oflwexcb = (ADE_EXCB *)cbs[action->qhd_obj.qhd_qep.u1.s2.ahd_agoflow->qen_pos];
    hstp->obufpp = (dsh->dsh_qp_ptr->qp_status & QEQP_GLOBAL_BASEARRAY) ?
	&(dsh->dsh_row[action->qhd_obj.qhd_qep.u1.s2.ahd_agobuf]) :
	&(hstp->oflwexcb->excb_bases[action->qhd_obj.qhd_qep.u1.s2.ahd_agobuf]);
    hstp->currgcount = 0;

    /* Inserts from load1 use currexcb */
    hstp->accum_excb = hstp->currexcb;
    hstp->accum_wbufpp = hstp->wbufpp;
    hstp->accum_obufpp = &toss_ptr;	/* Not needed for ahd_current */

    for (;;)
    {
	/* fetch rows */
	status = (*node->qen_func)(node, qef_rcb, dsh, function);
	function &= ~(TOP_OPEN | FUNC_RESET);
	if (status != E_DB_OK)
	{
	    /* the error.err_code field in dsh should already be
	    ** filled in.
	    ** For EOF, or end-of-group and expecting it, break to finish
	    ** up.  If end-of-group and not expecting it (should not
	    ** happen, defensive code), just loop for more.  If any other
	    ** error, return error status.
	    */
	    if (dsh->dsh_error.err_code == E_QE0015_NO_MORE_ROWS
	       || (dsh->dsh_error.err_code == E_QE00A5_END_OF_PARTITION
		  && action->qhd_obj.qhd_qep.ahd_qepflag & AHD_PART_SEPARATE) )
		break;
	    if (dsh->dsh_error.err_code == E_QE00A5_END_OF_PARTITION)
		continue;
	    return (status);
	}
	haptr->hshag_rcount++;

	/* Search for matching row in hash table. Project hash number,
	** then look for it in hash table.
	** It's essential to clear the work buffer since we treat the
	** hash key as one big byte key for comparison purposes.
	** Without the clearing we'll have problems with varchars
	** and nulls.  (Nulls only move the null-indicator into the
	** key area, via MECOPYN.  Varchars are TRIMmed into the key area.)
	*/
	MEfill(haptr->hshag_keysz, 0, (PTR)&rowptr->hash_key[0]);
					/* init hash stuff buffer */
	hstp->hashexcb->excb_seg = ADE_SMAIN;
	status = ade_execute_cx(adfcb, hstp->hashexcb);
	if (status != E_DB_OK)
	{
	    if ((status = qef_adf_error(&adfcb->adf_errcb, 
		    status, dsh->dsh_qefcb, &dsh->dsh_error)) != E_DB_OK)
	      return (status);
	}
	/* and now for the actual hash number... */
	qen_hash_value(rowptr, haptr->hshag_keysz);
	rowptr->hash_compressed = 0;

	/* Note! the row length thingie in the row header isn't used for
	** most of hash agg.  It's only used to overflow the row onto disk,
	** and get the row back off of disk.
	*/

	/* Insert (or aggregate) into hash table. */
	status = qea_haggf_insert(dsh, haptr, hstp);
	if (status != E_DB_OK)
	    return(status);
    }

    /* Here when source node returned EOF or end-of-partition.
    ** Always light the end-of-group flag if appropriate, caller uses that
    ** to know that there is ultimately more input.
    ** If there were no rows, return E_DB_WARN with the error still in the
    ** DSH, so that if it's an empty agg we return EOF to the caller.
    ** Otherwise, set flags for processing what we have so far.
    */
    if (dsh->dsh_error.err_code == E_QE00A5_END_OF_PARTITION)
	haptr->hshag_flags |= HASHAG_EOG;

    status = E_DB_WARN;
    if (haptr->hshag_rcount > 0)
    {
	haptr->hshag_flags &= ~HASHAG_LOAD1;
	haptr->hshag_flags |= HASHAG_RETURN;
	haptr->hshag_curr_bucket = -1;
	haptr->hshag_lastrow = NULL;
	dsh->dsh_error.err_code = E_QE0000_OK;
	status = E_DB_OK;
    }

    if (dsh->dsh_hash_debug)
    {
	i4 i;
	u_i4 nb;
	QEN_HASH_PART *hpptr;
	TRdisplay("%@ hashagg thread %d LOAD1 loaded %d rows, %d chunks\n",
		dsh->dsh_threadno, haptr->hshag_rcount, hstp->chunk_count);
	for (i=0; i<haptr->hshag_pcount; i++)
	{
	    hpptr = &haptr->hshag_currlink->hsl_partition[i];
	    nb = 0;
	    if (hpptr->hsp_file)
		nb = hpptr->hsp_file->hsf_filesize;
	    TRdisplay("t%d Partition[%d]: %d rows, offset %d, %u blocks, flags %v\n",
		dsh->dsh_threadno, i, hpptr->hsp_brcount, hpptr->hsp_offset, nb,
		"BSPILL,PSPILL,BINHASH,PINHASH,BFLUSHED,PFLUSHED,BALLSAME,PALLSAME,DONE",hpptr->hsp_flags);
	}
    }

    return(status);
}


/*{
** Name: QEA_HAGGF_LOAD2 - phase II (from overflow structures) load of aggregate
**	rows
**
** Description:
**      This function reads rows from the hash aggregate overflow structures and
**	attempts to insert/aggregate them into the hash table (note that large
**	numbers of distinct grouping column sets may lead to recursive overflow 
**	handling).
**
**	There are two sub-phases of LOAD2:  LOAD2P1 attempts to find overflow
**	partitions that were so small that they did not spill to disk.
**	These can be aggregated immediately.  LOAD2P2 selects a spilled
**	overflow partition and reads/aggregates it, repeating until
**	all spilled partitions are read.
**
**	Note that just because a buffer is unspilled (LOAD2P1) does not
**	imply that it can be aggregated in one pass.  QEE will attempt to
**	make sure that aggregation chunk space is larger than a buffer,
**	but under RLL compression even that might not suffice.  So, the
**	P1 phase has to be able to deal with rows that can't be aggregated
**	and are put back into the (same) buffer.  Fortunately, these rows
**	can't aggregate with anything but what's in that buffer.
**
** Inputs:
**	dsh
**	action			Ptr to hash aggregate action header
**	haptr			Ptr to dynamic hash aggregation structure
**	hstp			Ptr to hash aggregation state structure (to
**				reduce paramaters passed)
**
** Outputs:
**      dsh
**	    .error.err_code		one of the following
**				E_QE0000_OK
**				E_QE000D_NO_MEMORY_LEFT
**	Returns:
**	    E_DB_{OK, WARN, ERROR, FATAL}
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      26-mar-01 (inkdo01)
**          Written for hash aggregate overflow processing.
**	31-aug-04 (inkdo01)
**	    Changes to support global base arrays.
**	17-may-05 (inkdo01)
**	    Get rid of files and free space, as soon as we're done
**	    with them.
**	7-Jul-2008 (kschendel) SIR 122512
**	    Spilled rows may be compressed, update row readers.
**	29-Oct-2008 (kschendel) SIR 122512
**	    Under RLL compression, there may be more rows in an unspilled
**	    buffer (load2p1) than can fit into chunk space.  Rearrange l2p1
**	    a little bit so that an unspilled buffer can be reprocessed if
**	    it can't be fully processed in one pass.
*/
static DB_STATUS
qea_haggf_load2(
QEE_DSH		*dsh,
QEF_AHD		*action,
QEN_HASH_AGGREGATE *haptr,
HSHAG_STATE	*hstp)

{
    DB_STATUS	status = E_DB_OK;
    QEN_HASH_LINK *hlink = haptr->hshag_currlink;
    QEN_HASH_PART *hpptr;
    QEN_HASH_PART *prev_hpptr;
    PTR		obptr;
    i4		i, j;
    i4		rowsz;

    /* The "load2" phase loads overflowed partitions into the hash table
    ** (performing aggregation as they are loaded) in stages. After each
    ** stage, the aggregated results are returned to the caller. The
    ** first stage (or set of stages) loads partitions that spilled to a
    ** write buffer, but didn't spill to disk.  The remaining stage(s) process
    ** partitions that did overflow to disk. Any spilled partition that
    ** still has too many distinct values of the grouping columns will
    ** recursively partition until the rows can be successfully aggregated.
    **
    ** A "rule" in processing the partitions is that if the hash table 
    ** already contains data from another partition processed in the same
    ** stage, no partition will be processed that contains more rows than
    ** will fit the hash table under the assumption that ALL those rows
    ** have distinct grouping column values. This is a very pessimistic 
    ** assumption, but little is gained by reducing the "qea_haggf_load2"
    ** calls, and the assumption will not result in any more partition file
    ** I/O. At a later date, techniques such as bit maps may be used to
    ** estimate the number of distinct groups in a partition file, thus
    ** enabling this rule to be loosened somewhat. */

    /* Init. local and global variables, set hash table to "empty". */
    hstp->oflwexcb = (ADE_EXCB *)dsh->dsh_cbs[action->qhd_obj.qhd_qep.u1.s2.ahd_agoflow->qen_pos];
    hstp->obufpp = (dsh->dsh_qp_ptr->qp_status & QEQP_GLOBAL_BASEARRAY) ?
	&(dsh->dsh_row[action->qhd_obj.qhd_qep.u1.s2.ahd_agobuf]) :
	&(hstp->oflwexcb->excb_bases[action->qhd_obj.qhd_qep.u1.s2.ahd_agobuf]);
    hstp->owbufpp = (dsh->dsh_qp_ptr->qp_status & QEQP_GLOBAL_BASEARRAY) ?
	&(dsh->dsh_row[action->qhd_obj.qhd_qep.u1.s2.ahd_agowbuf]) :
	&(hstp->oflwexcb->excb_bases[action->qhd_obj.qhd_qep.u1.s2.ahd_agowbuf]);

    /* Inserts from load2 use oflwexcb since we're accumulating overflow */
    hstp->accum_excb = hstp->oflwexcb;
    hstp->accum_wbufpp = hstp->owbufpp;
    hstp->accum_obufpp = hstp->obufpp;

    for (i = 0; i < haptr->hshag_htsize; i++)
	haptr->hshag_htable[i] = NULL;
    hstp->currgcount = 0;

    /* First step is to loop over partitions, looking for non-empty,
    ** non-spilled ones to load first. */
    if (haptr->hshag_flags & HASHAG_LOAD2P1)
    {
	i4 rows_in_buf;

	for (i = 0; i < haptr->hshag_pcount; i++)
	{
	    hpptr = &hlink->hsl_partition[i];
	    if (hpptr->hsp_bptr == NULL || 
		    hpptr->hsp_brcount == 0 ||
		    hpptr->hsp_flags & (HSP_DONE | HSP_BSPILL))
		continue;		/* parition must have rows and be in mem. */

	    /* test "rule" from above.  This is doubly pessimistic here, as
	    ** it would probably be good enough to guarantee that currgcount
	    ** isn't beyond grpmax (i.e. if we filled aggregation memory,
	    ** in the worst case only rows from the final buffer would
	    ** remain to be reprocessed).  As pointed out above, though,
	    ** there is no particular benefit to reducing load2 calls,
	    ** and the pessimistic test is the simplest to understand.
	    */
	    if (hstp->currgcount > 0 && 
		hstp->currgcount + hpptr->hsp_brcount > haptr->hshag_grpmax)
		continue;		/* "rule" from above */

	    if (dsh->dsh_hash_debug)
	    {
		TRdisplay("%@ hashagg t%d L2P1 nonspilled pno %d, %d rows, %d inmem\n",
			dsh->dsh_threadno, i, hpptr->hsp_brcount, hstp->currgcount);
	    }
	    /* This partition will fit into hash table. Loop over buffer
	    ** and insert rows.  Under RLL compression, we might not be able
	    ** to process all rows from the buffer, so first reset the buffer
	    ** to look empty (so that insert/overflow can put-back the extra).
	    ** (Note that this putback can only occur if this one buffer is
	    ** being processed, and it holds more rows than chunk space
	    ** does, ie brcount > grpmax.  The "rule" above prevents any other
	    ** case of putback.)
	    */
	    obptr = hpptr->hsp_bptr;
	    rows_in_buf = hpptr->hsp_brcount;
	    hpptr->hsp_brcount = 0;	/* reset in case of overflow */
	    hpptr->hsp_offset = 0;
	    /* Slightly kludgy use of this flag -- tells overflow that we're
	    ** doing a "putback" instead of an initial overflow.
	    */
	    hpptr->hsp_flags = HSP_DONE;
	    for (j = 0; j < rows_in_buf; j++)
	    {
		hstp->hashbufp = (QEF_HASH_DUP *)obptr;
		hstp->oflwbufp = obptr;
		status = qea_haggf_insert(dsh, haptr, hstp);
		if (status != E_DB_OK)
		    return(status);
		obptr += QEFH_GET_ROW_LENGTH_MACRO((QEF_HASH_DUP *)obptr);
					/* next row in buffer */
	    }
	    /* If we did any "putback" into the same buffer, just loop to
	    ** reprocess what's left in it.
	    */
	    if (hpptr->hsp_brcount > 0)
	    {
		hpptr->hsp_flags &= ~HSP_DONE;  /* it's not actually done */
		--i;			/* Redo outer loop on same partition */
		continue;
	    }
	    /* Normal case is that buffer got emptied, move to next */
	    status = qea_haggf_close(haptr, &hpptr->hsp_file,
		CLOSE_DESTROY);		/* discard the file */
	}

	/* If any data was processed from in-memory buffers, return 
	** corresponding aggregate rows now. This avoids confusing the 
	** in-memory logic with the spilled partitions logic. */
	if (hstp->currgcount > 0)
	{
	    haptr->hshag_flags |= HASHAG_RETURN;
	    return(E_DB_OK);
	}

	/* Nothing left in memory buffers - flush buffers for any open
	** files and continue with phase 2 of overflowed data. */
	haptr->hshag_flags &= ~HASHAG_LOAD2P1;
	status = qea_haggf_flush(dsh, haptr);
	if (status != E_DB_OK)
	    return(status);
	haptr->hshag_flags |= HASHAG_LOAD2P2;
    }

    /* Phase 2 of overflow handling - loop in search of offloaded partitions
    ** and process them now. */
    {
	i4	leftover, offset, j;
	QEF_HASH_DUP *rowptr;

	for (i = 0; i < haptr->hshag_pcount; i++)
	{
	    hpptr = &hlink->hsl_partition[i];
	    /* Process an overflowed partition, but only if we haven't picked
	    ** one yet; or if we've started loading partitions, only if
	    ** this one will fit.
	    */
	    if (!(hpptr->hsp_flags & HSP_BSPILL) ||
		(hstp->currgcount > 0 && 
		 hstp->currgcount + hpptr->hsp_brcount > haptr->hshag_grpmax))
		continue;		/* process overflowed partitions, but
					** only if there's room */
	    if (dsh->dsh_hash_debug)
	    {
		TRdisplay("%@ hashagg t:%d L2P2 l:%d loading pno %d, %d rows, %d inmem\n",
			dsh->dsh_threadno, hlink->hsl_level, i,
			hpptr->hsp_brcount, hstp->currgcount);
	    }
	    /* reset spill file to its start, always block-zero for hashagg. */
	    hpptr->hsp_file->hsf_currbn = 0;

	    offset = haptr->hshag_rbsize;  /* init to trigger 1st block read */

	    for (j = 0; j < hpptr->hsp_brcount; j++)
	    {
		rowptr = (QEF_HASH_DUP *)((char *)haptr->hshag_rbptr + offset);
		leftover = haptr->hshag_rbsize - offset;
		if (leftover >= QEF_HASH_DUP_SIZE
		  && leftover >= QEFH_GET_ROW_LENGTH_MACRO(rowptr))
		{
		    /* Entire row is in buffer, just point to it */
		    offset += QEFH_GET_ROW_LENGTH_MACRO(rowptr);
		}
		else
		{
		    /* Row is split, or entirely in next buffer */
		    if (leftover > 0)
		    {
			obptr = haptr->hshag_workrow;
			MEcopy((PTR) rowptr, leftover, obptr);
			rowptr = (QEF_HASH_DUP *) obptr;
			obptr += leftover;
		    }
		    status = qea_haggf_read(haptr, hpptr->hsp_file,
				haptr->hshag_rbptr);
		    if (status != E_DB_OK)
			return(status);
		    if (leftover <= 0)
		    {
			/* New row is entirely in the buffer now */
			rowptr = (QEF_HASH_DUP *) haptr->hshag_rbptr;
			offset = QEFH_GET_ROW_LENGTH_MACRO(rowptr);
		    }
		    else
		    {
			/* Copy the remainder, doing the header first if
			** necessary (so that the row length is available)
			*/
			offset = 0;
			if (leftover < QEF_HASH_DUP_SIZE)
			{
			    MEcopy(haptr->hshag_rbptr,
				QEF_HASH_DUP_SIZE-leftover, obptr);
			    offset = QEF_HASH_DUP_SIZE-leftover;
			    obptr += offset;
			    leftover = QEF_HASH_DUP_SIZE;
			}
			rowsz = QEFH_GET_ROW_LENGTH_MACRO(rowptr);
			MEcopy(haptr->hshag_rbptr + offset,
				rowsz - leftover, obptr);
			offset += (rowsz - leftover);
		    }
		}
		hstp->oflwbufp = (PTR) rowptr;

		/* Finally have a row - now aggregate/insert into hash table. */
		hstp->hashbufp = (QEF_HASH_DUP *)hstp->oflwbufp;
						/* set up hash buff ptr, too */
		status = qea_haggf_insert(dsh, haptr, hstp);
		if (status != E_DB_OK)
		    return(status);
	    }	/* end of "rows from partition i" loop */
	
	    hpptr->hsp_flags = HSP_DONE;	/* even if we recursed, 
						** this partition is done */
	    status = qea_haggf_close(haptr, &hpptr->hsp_file,
		CLOSE_DESTROY);			/* discard the file */

	    if (haptr->hshag_flags & HASHAG_RECURSING)
	    {
		/* This partition caused recursion. Set flags to return agg
		** rows already built, but then do phase 1 processing of 
		** recursed partition. */
		haptr->hshag_flags &= ~(HASHAG_RECURSING | HASHAG_LOAD2P2);
		haptr->hshag_flags |= (HASHAG_LOAD2P1 | HASHAG_RETURN);
		return(E_DB_OK);
	    }
	}	/* end of partition loop */
    }

    if (hstp->currgcount > 0)
	haptr->hshag_flags |= HASHAG_RETURN;	/* show agg rows to return */
    else if (hlink->hsl_level > 0)
    {
	/* No more rows from this recursion level, but back up to
	** previous level for possibly more overflow. LOAD2P2 
	** causes us to pick up where we left off in previous 
	** level. */
	if (dsh->dsh_hash_debug)
	{
	    TRdisplay("%@ Exiting hashagg recursion level %d, thread %d\n",
			hlink->hsl_level, dsh->dsh_threadno);
	}
	/* Copy back partition buffers from lower level, it might have
	** allocated new ones if upper level partitioning was uneven
	*/
	prev_hpptr = &hlink->hsl_prev->hsl_partition[haptr->hshag_pcount-1];
	for (hpptr = &hlink->hsl_partition[haptr->hshag_pcount-1];
	     hpptr >= &hlink->hsl_partition[0];
	    --hpptr, --prev_hpptr)
	{
	    /* Ignore the partition file, hash agg destroys them as it
	    ** goes, unlike hash join.  So it should already be gone.
	    */
	    prev_hpptr->hsp_bptr = hpptr->hsp_bptr;
	}
	haptr->hshag_currlink = hlink->hsl_prev;
	haptr->hshag_flags |= HASHAG_LOAD2P2;
    }
    else
    {
	/* Looks like we're done.  Keep the end-of-group flag only, if it's
	** on, and the perm flags, and light the done flag.
	*/
	haptr->hshag_flags &= (HASHAG_EOG|HASHAG_KEEP_OVER_RESET);
	haptr->hshag_flags |= HASHAG_DONE;
    }

    return(E_DB_OK);
}


/*{
** Name: QEA_HAGGF_INSERT - locates/inserts rows in hash table
**
** Description:
**	This function processes rows from either the QP underneath,
**	or from overflow (spill to disk).  It attempts to locate a matching
**	row in the hash table.  If a match is found, the input row is
**	accumulated into the aggregate.  If no match, we attempt to
**	add a new output group into the hash table with the input
**	row's keys.  If no space is available for insertion, the row
**	is spilled off to a disk partition buffer.
**
**	If the incoming row is from load1, ie from the query plan, the
**	"bycompare" CX has been run which copies the hash keys into the
**	hash-work row (aghbuf);  the rest of the input row is in some row
**	produced by the QP, and while the ahd_current CX knows where that
**	is, this code doesn't (nor does it need to).  If the incoming
**	row is from load2, ie overflow, the row has been materialized
**	using the ahd_agoflow CX and the address of the possibly
**	compressed row is in both hashbufp and oflwbufp.
**
** Inputs:
**	dsh
**	haptr			Ptr to dynamic hash aggregation structure
**	hstp			Ptr to hash aggregation state structure (to
**				reduce paramaters passed)
**
** Outputs:
**      dsh
**	    .error.err_code		one of the following
**				E_QE0000_OK
**				E_QE000D_NO_MEMORY_LEFT
**	Returns:
**	    E_DB_{OK, WARN, ERROR, FATAL}
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      26-mar-01 (inkdo01)
**          Split from qea_haggf as part of overflow processing.
**	2-oct-01 (inkdo01)
**	    Changes to assure chunk link leaves chunk 8-byte aligned.
**	7-Jul-2008 (kschendel) SIR 122512
**	    Uncompress rows as they come out of spill buffers.
**	    Tidy up the main loops a little.
[@history_template@]...
*/
static DB_STATUS
qea_haggf_insert(
QEE_DSH		*dsh,
QEN_HASH_AGGREGATE *haptr,
HSHAG_STATE	*hstp)

{
    DB_STATUS	status = E_DB_OK;
    ADF_CB	*adfcb = dsh->dsh_adf_cb;
    QEF_HASH_DUP *workbufp, *prevbufp;
    ADE_EXCB	*aggexcb;
    QEF_HASH_DUP *rowptr;
    i4		hbucket, cmpres;
    u_i4	hashno;

    /* Set up locals, then process row. */

    rowptr = hstp->hashbufp;
    hashno = rowptr->hash_number;
    hbucket = hashno % haptr->hshag_htsize;

    /* if input row is compressed, decompress it into a work area. */

    if (rowptr->hash_compressed)
    {
	rowptr = qen_hash_unrll(0, rowptr, haptr->hshag_workrow2);

	/* Note that the original row is still in oflwbufp, which might
	** be the read buffer, or might be workrow.  We'll only see
	** compressed when reading spill, which is always in overflow
	** format already.
	*/
    }

    /* Search is 2 loops - first looks for hashno on chain and
    ** (with luck) usually finds it. If 2 consecutive entries have 
    ** same hashno, second loop uses hash keylist to compare 
    ** actual by-list entries. If neither finds a match, new 
    ** entry is added to chain. */
    adfcb->adf_errcb.ad_errcode = 0;
    cmpres = -1;
    prevbufp = NULL;
    workbufp = (QEF_HASH_DUP *) haptr->hshag_htable[hbucket];
    while (workbufp != NULL && hashno < workbufp->hash_number)
    {
	prevbufp = workbufp;
	workbufp = workbufp->hash_next;
    }
    while (workbufp != NULL && hashno == workbufp->hash_number)
    {
	/* Inner search loop - does adt_sortcmp's on by exprs. */
	cmpres = adt_sortcmp(adfcb, haptr->hshag_keylist,
		haptr->hshag_keycount, &(rowptr->hash_key[0]),
		&(workbufp->hash_key[0]), 0);
	if (adfcb->adf_errcb.ad_errcode != 0)
	{
	    /* Return comparison error. */
	    dsh->dsh_error.err_code = adfcb->adf_errcb.ad_errcode;
	    return(E_DB_ERROR);
	}
	if (cmpres <= 0)
	    break;
	prevbufp = workbufp;
	workbufp = workbufp->hash_next;
    }

    /* If no match was found, must execute init code to 
    ** prepare new aggregate work row. */
    if (cmpres != 0)
    {
	if (hstp->chunkoff+haptr->hshag_wbufsz <= haptr->hshag_chsize)
	{
	    workbufp = (QEF_HASH_DUP *)(hstp->curchunk + hstp->chunkoff);
	    hstp->chunkoff += haptr->hshag_wbufsz;
	}
	else
	{
	    /* Allocate next chunk and continue. */
	    if (*((PTR *)hstp->curchunk) != NULL)
	    {
		/* Chunkset already allocated - get next. */
		hstp->prevchunk = hstp->curchunk;
		hstp->curchunk = *((PTR *)hstp->curchunk);
		hstp->chunk_count++;
	    }
	    else if (hstp->chunk_count < haptr->hshag_chmax)
	    {
		/* Allocate another chunk. */
		hstp->prevchunk = hstp->curchunk;
		status = qea_haggf_palloc(dsh->dsh_qefcb, haptr,
				haptr->hshag_chsize,
				&hstp->curchunk,
				&dsh->dsh_error, "chunk");
		if (status != E_DB_OK)
		    return(E_DB_ERROR);
		++ hstp->chunk_count;
		++ haptr->hshag_challoc;	/* Just for stats */

		*((PTR *)hstp->prevchunk) = hstp->curchunk;
		*((PTR *)hstp->curchunk) = NULL;
	    }
	    else
	    {
		/* Overflow - if we're in LOAD2P2, we're in recursion
		** and must see if a new recursion must be started. 
		** In any event, row is sent to qea_haggf_overflow for
		** overflow processing. */
		if (haptr->hshag_flags & HASHAG_LOAD2P2 &&
		    !(haptr->hshag_flags & HASHAG_RECURSING))
		{
		    status = qea_haggf_recurse(dsh, haptr);
		    if (status != E_DB_OK)
			return(status);
		}

		status = qea_haggf_overflow(dsh, haptr, 
				hstp, hashno);
		return(status);
	    }

	    hstp->chunkoff = (sizeof(PTR) + DB_ALIGN_SZ - 1) &
			     (~(DB_ALIGN_SZ - 1));
	    workbufp = (QEF_HASH_DUP *) (hstp->curchunk + hstp->chunkoff);
	    hstp->chunkoff += haptr->hshag_wbufsz;
	}
	/* Copy hash stuff to front end, then exec init code for
	** new row. */
	MEcopy((PTR)rowptr, haptr->hshag_hbufsz, (PTR)workbufp);
	*(hstp->wbufpp) = (PTR) workbufp;
	hstp->currexcb->excb_seg = ADE_SINIT;
	status = ade_execute_cx(adfcb, hstp->currexcb);
	if (status != E_DB_OK)
	{
	    if ((status = qef_adf_error(&adfcb->adf_errcb, 
    		status, dsh->dsh_qefcb, &dsh->dsh_error)) != E_DB_OK)
    	    return (status);
	}
	workbufp->hash_number = hashno;
	if (prevbufp == NULL)
	{
	    workbufp->hash_next = 
		(QEF_HASH_DUP *)haptr->hshag_htable[hbucket];
	    haptr->hshag_htable[hbucket] = (PTR)workbufp;
	}
	else
	{
	    workbufp->hash_next = prevbufp->hash_next;
	    prevbufp->hash_next = workbufp;
	}
	/* Other row-header fields (compressed, row length, etc) are not
	** used for output group entries.
	*/
	haptr->hshag_gcount++;
	hstp->currgcount++;
    }

    /* Found matching row, accumulate the aggregate. */
    aggexcb = hstp->accum_excb;		/* ahd_current or ahd_agoflow */
    *(hstp->accum_wbufpp) = (PTR) workbufp;
    *(hstp->accum_obufpp) = (PTR) rowptr;
    aggexcb->excb_seg = ADE_SMAIN;
    status = ade_execute_cx(adfcb, aggexcb);
    if (status != E_DB_OK)
    {
	if ((status = qef_adf_error(&adfcb->adf_errcb, 
	    status, dsh->dsh_qefcb, &dsh->dsh_error)) != E_DB_OK)
	    return (status);
    }
    dsh->dsh_qef_remnull |= aggexcb->excb_nullskip;

    return(E_DB_OK);
}


/*{
** Name: QEA_HAGGF_OVERFLOW - adds overflow row to hash aggregate overflow 
**	structure
**
** Description:
**      This function processes overflow rows. Hash number determines partition
**	to which row is assigned. Row is added to buffer, possibly requiring 
**	write of previous buffer contents to disk file.
**
**	The incoming row might be in hashagg input format, or in overflow
**	spill format, depending on whether we are in recursion or not.
**	If we're writing the top level of spillage, the rows are in
**	input form in some mystery input row, and the overflow CX must be run
**	to materialize the row into some known place and form.  If we're
**	below the top level, this must be recursion, and the row is already
**	in overflow form.  In the latter case, the oflwbufp pointer
**	points to the uncompressed copy of the row as read back from higher
**	level spill.
**
**	One special case occurs when LOAD2P1 is processing a buffer that
**	didn't spill to disk, and the buffer has more rows than fit into
**	the aggregation chunk area.  (This can only happen if RLL-compressing,
**	qee prevents this otherwise.)  This special case is indicated by
**	the partition being marked DONE (a bit kludgy).  When this happens,
**	all we need to do is move the row down lower in the same buffer.
**
** Inputs:
**	dsh
**	haptr			Ptr to dynamic hash aggregation structure
**	hstp			Ptr to hash aggregation state structure (to
**				reduce paramaters passed)
**	hashno			Hash number of current row
**
** Outputs:
**      dsh
**	    .error.err_code		one of the following
**				E_QE0000_OK
**				E_QE000D_NO_MEMORY_LEFT
**	Returns:
**	    E_DB_{OK, WARN, ERROR, FATAL}
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      26-mar-01 (inkdo01)
**          Written for hash aggregate overflow processing.
**	20-apr-01 (inkdo01)
**	    Fix to handle recursion - overflow row is already materialized
**	    in a recursion.
**	7-Jun-2005 (schka24)
**	    Typo fix (- instead of = getting open status).
**	9-Jan-2007 (kschendel) SIR 122512
**	    Rehash when overflowing into a recursion (level>0), ie the
**	    input rows are coming from overflow.  This vastly speeds up
**	    aggregations that overflow significantly.
**	7-Jul-2008 (kschendel) SIR 122512
**	    Changes to attempt simple RLL compression when spilling rows.
**	30-Oct-2008 (kschendel) SIR 122512
**	    Deal with load2p1 putback into the same buffer.
*/
static DB_STATUS
qea_haggf_overflow(
QEE_DSH		*dsh,
QEN_HASH_AGGREGATE *haptr,
HSHAG_STATE	*hstp,
u_i4		hashno)

{
    DB_STATUS	status = E_DB_OK;
    QEF_CB	*qef_cb = dsh->dsh_qefcb;
    ADF_CB	*adfcb = dsh->dsh_adf_cb;
    QEN_HASH_LINK *hlink = haptr->hshag_currlink;
    QEN_HASH_PART *hpptr;
    QEF_HASH_DUP *rowptr;
    PTR		obptr;
    i4		pno, rowsz, chunk;
    
    /* Determine partition number, allocate (if necessary) partition
    ** buffer, add row to buffer and (if necessary) write block to 
    ** file.
    ** The partition number is formed from the hash number, and at
    ** the top level (when overflowing from the QP input), we'll just
    ** use that.  If we're in recursion, overflowing from an overflow
    ** partition, it's a bad idea to re-use the same hash or we'll just
    ** dump everything back where it came from.  Instead, use a
    ** hash on the original hash number, salted with the recursion
    ** level to produce different hashing at different levels.
    */

    ++ haptr->hshag_ocount;
    haptr->hshag_flags |= (HASHAG_OVFLW | HASHAG_LOAD2P1);
						/* indicate overflow */
    if (hlink->hsl_level > 0)
    {
	u_i4 newkey[2];

	newkey[0] = hashno;
	newkey[1] = hlink->hsl_level;
	hashno = (u_i4) -1;
	HSH_CRC32((char *)&newkey[0], sizeof(newkey), &hashno);
    }
    pno = hashno % haptr->hshag_pcount;		/* partition no. */
    hpptr = &hlink->hsl_partition[pno];

    if (hpptr->hsp_bptr == NULL)
    {
	/* Allocate buffer for partition. */
	status = qea_haggf_palloc(qef_cb, haptr,
			haptr->hshag_pbsize,
			&hpptr->hsp_bptr,
			&dsh->dsh_error, "part_buf");
	if (status != E_DB_OK)
	    return (status);

	haptr->hshag_toplink->hsl_partition[pno].hsp_bptr =
			hpptr->hsp_bptr;	/* set in top level, too */
	hpptr->hsp_offset = 0;
    }
    else if (hpptr->hsp_flags & HSP_DONE)
    {
	/* Special indicator that we're to put-back this row into
	** the same buffer, at the current offset.  LOAD2P1 is running
	** and not all the rows in the buffer fit into the output,
	** so we just save up the spill and reprocess it later.
	** This is very unusual and can only happen if RLL-compressing.
	*/
	rowptr = (QEF_HASH_DUP *) hstp->oflwbufp;
	rowsz = QEFH_GET_ROW_LENGTH_MACRO(rowptr);
	obptr = (PTR)(hpptr->hsp_bptr + hpptr->hsp_offset);
	/* No check for buffer overflow, since it didn't spill the
	** first time, and we necessarily aggregate away at least some
	** of the (initial) rows from this buffer.
	*/
	MEcopy((PTR) rowptr, rowsz, obptr);
	hpptr->hsp_brcount++;
	hpptr->hsp_offset += rowsz;
	return (E_DB_OK);
    }

    if (hlink->hsl_level == 0)
    {
	/* If this is a fresh row, materialize the overflow row form,
	** and then optionally try to compress the row.  Do this right
	** into the output buffer if it's certain that there is
	** enough space.
	*/
	rowsz = haptr->hshag_obufsz;
	obptr = (PTR)(hpptr->hsp_bptr + hpptr->hsp_offset);
	if (hpptr->hsp_offset + rowsz > haptr->hshag_pbsize)
	    obptr = haptr->hshag_workrow;

	/* Move hash number, keys to head of buffer. */
	MEcopy((PTR)hstp->hashbufp, haptr->hshag_hbufsz, obptr);
	*(hstp->obufpp) = obptr;

	/* Materialize rest of overflow row into buffer. */
	hstp->oflwexcb->excb_seg = ADE_SINIT;
	status = ade_execute_cx(adfcb, hstp->oflwexcb);
	if (status != E_DB_OK)
	{
	    if ((status = qef_adf_error(&adfcb->adf_errcb, 
		status, qef_cb, &dsh->dsh_error)) != E_DB_OK)
		return (status);
	}

	rowptr = (QEF_HASH_DUP *) obptr;
	rowptr->hash_compressed = 0;
	QEFH_SET_ROW_LENGTH_MACRO(rowptr,rowsz);

	/* If we're allowed to try compression, do so */
	if (haptr->hshag_flags & HASHAG_RLL)
	{
	    rowptr = qen_hash_rll(0, rowptr, haptr->hshag_workrow2);
	    if (rowptr->hash_compressed)
	    {
		/* If compressed, the row is now in workrow2.  See if it
		** might fit into the buffer now, and if so, move it there.
		** (otherwise leave it in workrow2 for splitting.)
		*/
		rowsz = QEFH_GET_ROW_LENGTH_MACRO(rowptr);
		if (hpptr->hsp_offset + rowsz <= haptr->hshag_pbsize)
		{
		    /* Move compressed row to buffer */
		    obptr = (PTR)(hpptr->hsp_bptr + hpptr->hsp_offset);
		    MEcopy((PTR) rowptr, rowsz, obptr);
		}
		else
		{
		    obptr = (PTR) rowptr;	/* Split from here */
		}
	    }
	}
    }
    else
    {
	/* In recursion, row was just read from spill and decompressed.
	** oflwbufp points to the original compressed version.  Just use the
	** original possibly compressed row and move it straight to the
	** output buffer (or the workrow buffer if we need to split.)
	*/
	rowptr = (QEF_HASH_DUP *) hstp->oflwbufp;
	rowsz = QEFH_GET_ROW_LENGTH_MACRO(rowptr);
	obptr = (PTR)(hpptr->hsp_bptr + hpptr->hsp_offset);
	if (hpptr->hsp_offset + rowsz > haptr->hshag_pbsize)
	    obptr = haptr->hshag_workrow;
	/* Move the row, unless it's already where it belongs (workrow) */
	if ((PTR) rowptr != obptr)
	    MEcopy((PTR) rowptr, rowsz, obptr);
    }

    /* Update offset, row counts, etc. and leave (unless we need
    ** to write a block. */
    hpptr->hsp_brcount++;
    if (obptr != haptr->hshag_workrow && obptr != haptr->hshag_workrow2)
    {
	hpptr->hsp_offset += rowsz;
	return(E_DB_OK);
    }

    /* If the output row is in a work area, we must need splitting.
    ** Copy the bit that fits into the buffer (if any), write the block
    ** (after possibly opening a new file), and copy rest of row to
    ** refreshed buffer. */

    chunk = haptr->hshag_pbsize - hpptr->hsp_offset;
    if (chunk > 0)
    {
	MEcopy(obptr, chunk, hpptr->hsp_bptr + hpptr->hsp_offset);
	obptr += chunk;
    }
    else
	chunk = 0;		/* just make sure... */

    if (!(hpptr->hsp_flags & HSP_BSPILL))
    {
	/* Open (and possibly allocate) a file before write. */
	status = qea_haggf_open(dsh, haptr, hpptr);
	if (status != E_DB_OK)
	    return(status);
	hpptr->hsp_flags |= HSP_BSPILL;
    }
    status = qea_haggf_write(dsh, haptr, hpptr->hsp_file,
			hpptr->hsp_bptr);
    if (status != E_DB_OK)
	return(status);
    MEcopy(obptr, rowsz - chunk, hpptr->hsp_bptr);
    hpptr->hsp_offset = rowsz - chunk;
    return(E_DB_OK);
}


/*{
** Name: QEA_HAGGF_RECURSE - prepares next overflow recursion level
**
** Description:
**      This function sets up QEN_HASH_LINK structure for next level of
**	overflow partitioning in the event that a partition from the current
**	level has too many distinct groups to fit the hash table. A new
**	link structure is allocated and formatted unless one has already
**	been prepared at this level (more than one partition required 
**	recursion).
**
** Inputs:
**	dsh
**	haptr			Ptr to dynamic hash aggregation structure
**
** Outputs:
**      dsh
**	    .error.err_code		one of the following
**				E_QE0000_OK
**				E_QE000D_NO_MEMORY_LEFT
**	Returns:
**	    E_DB_{OK, WARN, ERROR, FATAL}
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      11-apr-01 (inkdo01)
**          Written for hash aggregate overflow processing.
**	24-Jun-2008 (kschendel) SIR 122512
**	    recursion level counter in partition struct removed, fix here.
[@history_template@]...
*/
static DB_STATUS
qea_haggf_recurse(
QEE_DSH		*dsh,
QEN_HASH_AGGREGATE *haptr)

{
    DB_STATUS	status = E_DB_OK;
    QEF_CB	*qef_cb = dsh->dsh_qefcb;
    QEN_HASH_LINK *hlink = haptr->hshag_currlink;
    QEN_HASH_LINK *prevlink;
    QEN_HASH_PART *hpptr;
    i4		i;


    /* First check if there is already a link structure at this recursion
    ** level. If so, tidy it up and prepare it for the new recursed
    ** partition. If not, allocate a new link structure and corresponding
    ** partition structures and copy partition buffers from top level (buffers
    ** are only used for load of one recursion level at a time - and top level
    ** partition structures have union of all allocated buffers). */

    if (dsh->dsh_hash_debug)
    {
	TRdisplay("%@ hashagg recursing to level %d, thread %d\n",
		hlink->hsl_level+1, dsh->dsh_threadno);
    }
    if (hlink->hsl_next == (QEN_HASH_LINK *) NULL)
    {
	/* First visit to this recursion level. Allocate/format link
	** structure and corresponding partition descriptor array. */
	status = qea_haggf_palloc(qef_cb, haptr,
			sizeof(QEN_HASH_LINK)
			+ haptr->hshag_pcount * sizeof(QEN_HASH_PART),
			&hlink->hsl_next,
			&dsh->dsh_error, "link+part_descs");
	if (status != E_DB_OK)
	    return (status);

	prevlink = hlink;
	hlink = hlink->hsl_next;	/* hook 'em together */
	hlink->hsl_prev = prevlink;
	hlink->hsl_next = (QEN_HASH_LINK *) NULL;
	hlink->hsl_level = prevlink->hsl_level + 1;
	hlink->hsl_partition = (QEN_HASH_PART *)&((char *)hlink)
						[sizeof(QEN_HASH_LINK)];
	for (i = 0; i < haptr->hshag_pcount; i++)
	{
	    QEN_HASH_PART   *hpart = &hlink->hsl_partition[i];

	    hpart->hsp_bptr = prevlink->hsl_partition[i].hsp_bptr;
	    hpart->hsp_file = (QEN_HASH_FILE *) NULL;
	    hpart->hsp_bstarts_at = 0;
	    hpart->hsp_bbytes = 0;
	    hpart->hsp_offset = 0;
	    hpart->hsp_brcount = 0;
	    hpart->hsp_flags  = 0;
	}
	if (hlink->hsl_level > haptr->hshag_maxreclvl)
	    haptr->hshag_maxreclvl = hlink->hsl_level;  /* for stats */
    }
    else
    {
	prevlink = hlink;
	hlink = hlink->hsl_next;	/* new link level */
	for (i = 0; i < haptr->hshag_pcount; i++)
	{
	    QEN_HASH_PART   *hpart = &hlink->hsl_partition[i];

	    /* Reset stuff in existing partition array. */
	    hpart->hsp_bptr = prevlink->hsl_partition[i].hsp_bptr;
	    hpart->hsp_file = (QEN_HASH_FILE *) NULL;
	    hpart->hsp_bstarts_at = 0;
	    hpart->hsp_bbytes = 0;
	    hpart->hsp_offset = 0;
	    hpart->hsp_brcount = 0;
	    hpart->hsp_flags  = 0;
	}
    }


    haptr->hshag_currlink = hlink;
    haptr->hshag_flags |= HASHAG_RECURSING;

    return(E_DB_OK);
}


/*{
** Name: QEA_HAGGF_WRITE	- write a block to a partition file
**
** Description:
**	This function writes blocks to partition files. 
**
** Inputs:
**	dsh		- Thread data segment header
**	haptr		- ptr to base hash structure
**	hfptr		- ptr to file descriptor
**	buffer		- ptr to buffer being written
**
** Outputs:
**
** Side Effects:
**	Advances file's currbn by blocks written.
**
** History:
**	28-mar-01 (inkdo01)
**	    Cloned from qenhashutl.c
**	15-May-2008 (kschendel) SIR 122512
**	    Move current block number to file structure.
**
*/
 
static DB_STATUS
qea_haggf_write(
QEE_DSH		*dsh,
QEN_HASH_AGGREGATE *haptr,
QEN_HASH_FILE	*hfptr,
PTR		buffer)

{

    DMH_CB	*dmhcb;
    DB_STATUS	status = E_DB_OK;
    QEF_CB	*qcb = dsh->dsh_qefcb;

    /* If we're writing a block, we'll need to read it back sooner
    ** or later. So allocate read buffer here. */
    if (haptr->hshag_rbptr == NULL)
    {

	/* Partition read buffer has not yet been allocated. Do
	** it now. */
	status = qea_haggf_palloc(qcb, haptr,
				haptr->hshag_rbsize,
				&haptr->hshag_rbptr,
				&dsh->dsh_error, "io_buf");
	if (status != E_DB_OK)
	    return (status);
    }

    /* Load up DMH_CB and issue the call. */
    dmhcb = (DMH_CB *)haptr->hshag_dmhcb;
    dmhcb->dmh_locidxp = &hfptr->hsf_locidx;
    dmhcb->dmh_file_context = hfptr->hsf_file_context;
    dmhcb->dmh_rbn = hfptr->hsf_currbn;
    dmhcb->dmh_buffer = buffer;
    dmhcb->dmh_func = DMH_WRITE;
    dmhcb->dmh_flags_mask = 0;
    dmhcb->dmh_blksize = haptr->hshag_pagesize;
    dmhcb->dmh_nblocks = haptr->hshag_pbsize / haptr->hshag_pagesize;
    hfptr->hsf_currbn += dmhcb->dmh_nblocks;
    if (hfptr->hsf_currbn > hfptr->hsf_filesize)
	hfptr->hsf_filesize = hfptr->hsf_currbn;

    status = dmh_write(dmhcb);
    return(status);

}


/*{
** Name: QEA_HAGGF_OPEN 	- determines if open file is available
**				or creates (and opens) a new one
**
** Description:
**	Open a spill file for writing.
**
**	If the partition has no spill file, get one from a free chain,
**	or if none, ask DMF for a new spill file.
**
**	Hash agg only spills one-sided, unlike hash join, so there should
**	not already be a spill file.
**
**	There was originally a call to open for reading, but it
**	turns out that neither DMF nor the low level SR code needed
**	it.  Reading just requires proper positioning.
**
**	General note about the haggf file utilities:  they are
**	essentially identical to the hash join file utilities, with
**	enough difference to make merging the two difficult.
**	(Not only do we access stuff in a QEN_HASH_AGGREGATE instead of
**	a QEN_HASH_BASE, but we use hash-agg memory allocator, etc.)
**	Until someone applies the effort needed to merge these utilities,
**	changes/fixes done here probably need done in qenhashutl.c as
**	well, and vice versa.
**
** Inputs:
**	dsh		- Thread data segment header
**	haptr		- ptr to base hash structure
**	hpptr		- ptr to partition descriptor
**
** Outputs:
**
** Side Effects:
**
**
** History:
**	28-mar-01 (inkdo01)
**	    Cloned from qenhashutl.c
**	12-Jun-2008 (kschendel) SIR 122512
**	    Open now implies write, not needed for read.
**
*/
 
static DB_STATUS
qea_haggf_open(
QEE_DSH		*dsh,
QEN_HASH_AGGREGATE *haptr,
QEN_HASH_PART	*hpptr)

{

    DMH_CB	*dmhcb;
    QEN_HASH_FILE  *fptr;
    DB_STATUS	status = E_DB_OK;

    fptr = hpptr->hsp_file;

    /* There shouldn't already be a file open, but if there is, just
    ** reset it back to zero.  Maybe should trdisplay this...
    */
    if (fptr != NULL)
    {
	fptr->hsf_currbn = fptr->hsf_filesize = 0;
	return (E_DB_OK);
    }

    /* Check free list and reuse existing file, if possible. 
    ** Otherwise, allocate a QEN_HASH_FILE + file context block
    ** and create the file, too. */
    if (haptr->hshag_freefile != NULL)
    {
	fptr = haptr->hshag_freefile;
	hpptr->hsp_file = fptr;
	haptr->hshag_freefile = fptr->hsf_nextf;
	fptr->hsf_filesize = 0;
	fptr->hsf_currbn = 0;	/* Reset to start */
	return(E_DB_OK);
    }
    else
    {
	QEF_CB		*qcb = dsh->dsh_qefcb;

	/* No existing file, but maybe there's a context block
	** hanging around that we can re-use.
	*/
	if (haptr->hshag_availfiles != NULL)
	{
	    fptr = haptr->hshag_availfiles;
	    haptr->hshag_availfiles = fptr->hsf_nextf;
	    /* fall thru to reinit the block */
	}
	else
	{
	    /* Allocate memory for the file context (and containing
	    ** QEN_HASH_FILE block). */
	    status = qea_haggf_palloc(qcb, haptr,
				sizeof(QEN_HASH_FILE) + sizeof(SR_IO),
				&fptr,
				&dsh->dsh_error, "hash_file");
	    if (status != E_DB_OK)
		return (status);
	}
	hpptr->hsp_file = fptr;

	fptr->hsf_nextf = NULL;
	fptr->hsf_nexto = haptr->hshag_openfiles;
	haptr->hshag_openfiles = fptr;		/* link to open list */
	fptr->hsf_filesize = 0;
	fptr->hsf_currbn = 0;
	fptr->hsf_file_context = (PTR)&((char *)fptr)
					[sizeof(QEN_HASH_FILE)];
    }
    /* Load up DMH_CB and issue the call. */
    dmhcb = (DMH_CB *)haptr->hshag_dmhcb;

    dmhcb->dmh_file_context = fptr->hsf_file_context;
    dmhcb->dmh_locidxp = &fptr->hsf_locidx;
    dmhcb->dmh_blksize = haptr->hshag_pagesize;
    dmhcb->dmh_func = DMH_OPEN;
    dmhcb->dmh_flags_mask = DMH_CREATE;

    status = dmh_open(dmhcb);
    return(status);

}


/*{
** Name: QEA_HAGGF_READ 	- read a block from a partition file
**
** Description:
**	This function reads blocks from partition files.  The amount read
**	is always hshag_rbsize from the hashagg control block.
**
**	If the file hasn't written as much as requested (probably because the
**	read and write buffer sizes are different), this routine will shorten
**	the DMH read request to keep DMH happy;  BUT it will still advance
**	the file's RBN as if the entire buffer had been read.  Hash agg
**	does not depend on this, but it matches hash join's behavior.
**
** Inputs:
**	haptr		- ptr to base hash structure
**	hfptr		- ptr to file descriptor
**	buffer		- ptr to buffer being written
**
** Outputs:
**
** Side Effects:
**	Updates currbn by the number of pages requested.
**
**
** History:
**	28-mar-01 (inkdo01)
**	    Cloned from qenhashutl.c
**	4-Mar-2007 (kschendel) SIR 122512
**	    Allow multi-page reads.
**	15-May-2008 (kschendel) SIR 122512
**	    Move current block number to file structure.
**
*/
 
static DB_STATUS
qea_haggf_read(
QEN_HASH_AGGREGATE *haptr,
QEN_HASH_FILE	*hfptr,
PTR		buffer)

{

    DMH_CB	*dmhcb;
    DB_STATUS	status = E_DB_OK;

    /* Load up DMH_CB and issue the call. */
    dmhcb = (DMH_CB *)haptr->hshag_dmhcb;
    dmhcb->dmh_locidxp = &hfptr->hsf_locidx;
    dmhcb->dmh_file_context = hfptr->hsf_file_context;
    dmhcb->dmh_rbn = hfptr->hsf_currbn;
    dmhcb->dmh_buffer = buffer;
    dmhcb->dmh_func = DMH_READ;
    dmhcb->dmh_flags_mask = 0;
    dmhcb->dmh_blksize = haptr->hshag_pagesize;
    dmhcb->dmh_nblocks = haptr->hshag_rbsize / haptr->hshag_pagesize;
    hfptr->hsf_currbn += dmhcb->dmh_nblocks;
    if (hfptr->hsf_currbn > hfptr->hsf_filesize)
	dmhcb->dmh_nblocks -= (hfptr->hsf_currbn - hfptr->hsf_filesize);

    status = dmh_read(dmhcb);
    if (status != E_DB_OK) 
{
status = status;
}
    return(status);

}


/*{
** Name: QEA_HAGGF_FLUSH	- write out contents of partition buffers
**				and close files
**
** Description:
**	This function writes remaining buffer of all disk resident 
**	partition files.
**
** Inputs:
**	dsh		- Thread data segment header
**	haptr		- ptr to base hash structure
**
** Outputs:
**
** Side Effects:
**
**
** History:
**	28-mar-01 (inkdo01)
**	    Cloned from qenhashutl.c
**	14-May-2008 (kschendel) SIR 122512
**	    Remove do-nothing close call.
**
*/
 
static DB_STATUS
qea_haggf_flush(
QEE_DSH		*dsh,
QEN_HASH_AGGREGATE *haptr)

{

    QEN_HASH_LINK  *hlink = haptr->hshag_currlink;
    QEN_HASH_PART  *hpptr;
    DMH_CB	*dmhcb;
    QEN_HASH_FILE  *fptr;
    i4		i;
    DB_STATUS	status = E_DB_OK;
    

    /* Search partition array for "spilled" partitions. Write last
    ** block for each. */

    for (i = 0; i < haptr->hshag_pcount; i++)
    {
	hpptr = &hlink->hsl_partition[i];
	if (!(hpptr->hsp_flags & HSP_BSPILL) ||
		(hpptr->hsp_flags & HSP_BFLUSHED)) continue;

	/* There is always data in the buffer of a spilled partition.
	** The flag isn't set (and a write isn't done) until there's
	** enough data to partially fill the next block. */
	fptr = hpptr->hsp_file;
	status = qea_haggf_write(dsh, haptr, fptr, hpptr->hsp_bptr);
	if (status != E_DB_OK) return(status);

	hpptr->hsp_flags |= HSP_BFLUSHED;
	hpptr->hsp_offset = 0;
	fptr->hsf_currbn = 0;
    }

    return(E_DB_OK);

}


/*{
** Name: QEA_HAGGF_CLOSE	- closes an existing partition file and
**				places it on free chain
**
** Description:
**	This function closes an existing partition file and, if file
**	isn't immediately being reread (to load it into the hash table
**	or to repartition it), places its context block on the free
**	chain for possible reuse. 
**
** Inputs:
**	haptr		- ptr to base hash structure
**	hfptr		- ptr to partition descriptor ptr
**	flag		- flag indicating whether to "release" file
**
** Outputs:
**
** Side Effects:
**
**
** History:
**	28-mar-01 (inkdo01)
**	    Cloned from qenhashutl.c
**	17-may-05 (inkdo01)
**	    Add code to drop destroyed files from open chain.
**	10-Jan-2006 (kschendel) SIR 122512
**	    Pull over context block re-use from qenhashutl.
*/
 
static DB_STATUS
qea_haggf_close(
QEN_HASH_AGGREGATE *haptr,
QEN_HASH_FILE	**hfptr,
i4		flag)

{

    DMH_CB	*dmhcb;
    QEN_HASH_FILE  *fptr = *hfptr, **wfptr;


    /* SR close is not required to flip from output to input. This 
    ** function does nothing, if no flags are input. It moves the
    ** file context for the still-open file to the free chain if
    ** the CLOSE_RELEASE flag is on. And it calls DMF to destroy it 
    ** if the CLOSE_DESTROY flag is on. */

    if (!(flag & (CLOSE_RELEASE | CLOSE_DESTROY)) || fptr == NULL)
	return(E_DB_OK);

    (*hfptr) = (QEN_HASH_FILE *) NULL;
    if (flag & CLOSE_RELEASE)
    {
	/* Add file context to free list. */
	fptr->hsf_nextf = haptr->hshag_freefile;
	haptr->hshag_freefile = fptr;
	return(E_DB_OK);
    }

    /* Must be CLOSE_DESTROY case. */
    /* Load up DMH_CB and issue the call. */
    dmhcb = (DMH_CB *)haptr->hshag_dmhcb;
    dmhcb->dmh_locidxp = &fptr->hsf_locidx;
    dmhcb->dmh_file_context = fptr->hsf_file_context;
    dmhcb->dmh_func = DMH_CLOSE;
    dmhcb->dmh_flags_mask = DMH_DESTROY;
    /* Search for file on open chain and remove it. */
    for (wfptr = &haptr->hshag_openfiles; (*wfptr) && (*wfptr) != fptr;
	    wfptr = &(*wfptr)->hsf_nexto);
    if ((*wfptr))
	(*wfptr) = fptr->hsf_nexto;

    /* Add file block to re-use list */
    fptr->hsf_nextf = haptr->hshag_availfiles;
    haptr->hshag_availfiles = fptr;

    return(dmh_close(dmhcb));


}

/*{
** Name: QEA_HAGGF_CLOSEALL	- closes and destroys all files on hash
**				control block's file chain
**
** Description:
**	This function loops over the hshag_openfiles chain, issuing 
**	qea_haggf_close calls with the DESTROY option to free them all.
**	It is called at the end of hash aggregation processing.
**
** Inputs:
**	haptr		- ptr to base hash structure
**
** Outputs:
**
** Side Effects:
**
**
** History:
**	28-mar-01 (inkdo01)
**	    Cloned from qenhashutl.c
**
*/
 
DB_STATUS
qea_haggf_closeall(
QEN_HASH_AGGREGATE *haptr)

{
    QEN_HASH_FILE	*hfptr, *savefptr;
    DB_STATUS		status;


    /* Simply loop over file list, qea_haggf_close'ing each entry. */
    for (hfptr = savefptr = haptr->hshag_openfiles; hfptr; 
				hfptr = savefptr = savefptr->hsf_nexto)
    {
	status = qea_haggf_close(haptr, &hfptr, CLOSE_DESTROY);
	if (status != E_DB_OK) return(status);
    }
    return(E_DB_OK);
}


/*
** Name: qea_haggf_palloc
**
** Description:
**	Allocate memory from the hash stream, making sure that we don't
**	exceed the session limit.  If the hash stream isn't open, we'll
**	open it.
**
** Inputs:
**	qefcb			QEF session control block
**	haptr			Hash aggregation control base
**	psize			Size of request in bytes
**	pptr			Put allocated block address here
**	dsh_error		Where to put error code if error
**	what			Short name for request for tracing
**
** Outputs:
**	Returns E_DB_OK or error status
**	If error, dsh_error->err_code filled in.
**
** History:
**	7-Jun-2005 (schka24)
**	    Copy from qen_hash_palloc.
**	24-Oct-2005 (schka24)
**	    Delay if no memory resources are available (from Datallegro).
**	25-Sep-2007 (kschendel) SIR 122512
**	    Allow somewhat more than qef_hash_mem before preemptively
**	    announcing out of memory.  qee now figures hash joins and
**	    hash aggregations a bit more closely, and for a query to
**	    exceed qef_hash_mem probably implies a complex query and a
**	    too-small qef_hash_mem setting.
**	    (later) actually, remove the test altogether.
*/

#ifdef xDEBUG
GLOBALREF i4 hash_most_ever;
#endif

static DB_STATUS
qea_haggf_palloc(QEF_CB *qefcb, QEN_HASH_AGGREGATE *haptr,
	SIZE_TYPE psize, void *pptr,
	DB_ERROR *dsh_error, char *what)

{
    DB_STATUS status;		/* Usual status thing */
    i4 naptime;			/* Milliseconds delayed so far */
    SIZE_TYPE sess_max;		/* Session limit (qef_hash_mem sort-of) */
    ULM_RCB ulm;		/* ULM request block */

#ifdef xDEBUG
    if (qefcb->qef_h_used + psize > hash_most_ever)
    {
	if (qefcb->qef_h_used + psize > qefcb->qef_h_max*10/11)
	    TRdisplay("%@ Most ever was %d, new used=%d, this req=%d\n",
			hash_most_ever, qefcb->qef_h_used+psize, psize);
	hash_most_ever = qefcb->qef_h_used+psize;
    }
#endif

    /* Use the sorthash pool */
    STRUCT_ASSIGN_MACRO(Qef_s_cb->qef_s_ulmcb, ulm);

    /* If stream not allocated, do that now */
    if (haptr->hshag_streamid == NULL)
    {
	ulm.ulm_streamid_p = &haptr->hshag_streamid;
	naptime = 0;
	for (;;)
	{
	    status = ulm_openstream(&ulm);
	    if (status == E_DB_OK || naptime > Qef_s_cb->qef_max_mem_nap)
		break;
	    CSms_thread_nap(500);		/* Half a second */
	    naptime += 500;
	}
	if (status != E_DB_OK)
	{
	    TRdisplay("%@ QEF sorthash pool: hashagg %p open of %s (%u) failed, pool full, used is %u\n",
			haptr, what, psize, qefcb->qef_h_used);
	    if (ulm.ulm_error.err_code == E_UL0005_NOMEM)
		dsh_error->err_code = E_QE000D_NO_MEMORY_LEFT;
	    else
		dsh_error->err_code = ulm.ulm_error.err_code;
	    return (E_DB_ERROR);
	}
	if (naptime > 0)
	{
	    TRdisplay("%@ QEF sorthash pool: hashagg %p open of %s (%u) delayed %d ms, pool full, used is %d\n",
		haptr, what, psize, naptime, qefcb->qef_h_used);
	}
    }

    ulm.ulm_streamid = haptr->hshag_streamid;
    ulm.ulm_psize = psize;
    naptime = 0;
    for (;;)
    {
	status = ulm_palloc(&ulm);
	if (status == E_DB_OK || naptime > Qef_s_cb->qef_max_mem_nap)
	    break;
	CSms_thread_nap(500);		/* Half a second */
	naptime += 500;
    }
    if (status != E_DB_OK)
    {
	TRdisplay("%@ QEF sorthash pool: hashagg %p alloc of %s (%u) failed, pool full, used is %u\n",
		haptr, what, psize, qefcb->qef_h_used);
	if (ulm.ulm_error.err_code == E_UL0005_NOMEM)
	    dsh_error->err_code = E_QE000D_NO_MEMORY_LEFT;
	else
	    dsh_error->err_code = ulm.ulm_error.err_code;
	return (E_DB_ERROR);
    }
    if (naptime > 0)
    {
	TRdisplay("%@ QEF sorthash pool: hashagg %p alloc of %s (%u) delayed %d ms, pool full, used is %d\n",
		haptr, what, psize, naptime, qefcb->qef_h_used);
    }
    haptr->hshag_halloc += psize;
    CSadjust_counter(&qefcb->qef_h_used, psize);	/* thread-safe */
    *(void **) pptr = ulm.ulm_pptr;
    /* FIXME pass dsh instead of qcb but fix later */
    if ( ((QEE_DSH *)(qefcb->qef_dsh))->dsh_hash_debug)
    {
	TRdisplay("%@ hashagg %p alloc %u %s at %p, new halloc %u used %u\n",
		haptr, psize, what, ulm.ulm_pptr, haptr->hshag_halloc, qefcb->qef_h_used);
    }

    return (E_DB_OK);

} /* qea_haggf_palloc */

/*
** Name: qea_haggf_cleanup -- Clean up finished hash aggregation
**
** Description:
**	In order to release sorthash pool memory for nodes higher up
**	in the query tree, hash aggregation releases its memory resources
**	as soon as it returns a "no more rows" to its caller.
**	Being an action, hash aggregation is often at the top where it
**	doesn't matter;  but if it's under a QP node down in the query
**	plan, this can help the memory situation significantly.
**
**	The hash agg memory stream is released.  We don't do a whole
**	lot of cleanup on the QEN_HASH_AGGREGATE control block, just
**	enough to make sure any next call opens a new stream.  The
**	caller has already closed/deleted hash files (it has to do
**	that separately in case it's end-of-group rather than end-
**	of-rows).
**
** Inputs:
**	dsh			Thread's data segment header
**	haptr			Hashagg runtime control block
**
** Outputs:
**	None
**
** History:
**	30-Sep-2007 (kschendel)
**	    Written so that hash agg cleans itself up at eof.
*/

static void
qea_haggf_cleanup(QEE_DSH *dsh, QEN_HASH_AGGREGATE *haptr)
{
    ULM_RCB ulm;

    if (haptr->hshag_streamid != NULL)
    {
	STRUCT_ASSIGN_MACRO(Qef_s_cb->qef_s_ulmcb, ulm);
	ulm.ulm_streamid_p = &haptr->hshag_streamid;
	if (ulm_closestream(&ulm) == E_DB_OK)
	{
	    /* ULM has nullified hshag_streamid.  Adjust hash used
	    ** for the session.
	    */
	    CSadjust_counter(&dsh->dsh_qefcb->qef_h_used, -haptr->hshag_halloc);
	    haptr->hshag_halloc = 0;
	}
	haptr->hshag_htable = NULL;	/* Tells init to reopen things */
    }
} /* qea_haggf_cleanup */

/*
** Name: haggf_stat_dump
**
** Description:
**	This routine dumps hashagg stats after it's done (or has reached
**	end-of-group) when QE93 is set.  Analogous to hash join's output.
**
** Inputs:
**	dsh			Thread's data segment header
**	act			Pointer to haggf action
**	haptr			Hashagg runtime control block
**
** Outputs:
**	None
**
** History:
**	17-Aug-2007 (kschendel)
**	    Write to expose what is going on inside hash agg
**	21-Sep-2007 (kschendel)
**	    Show actual chunks, not just max chunks
**	25-Sep-2007 (kschendel)
**	    More fiddling with the output
*/

static void
haggf_stat_dump(QEE_DSH *dsh, QEF_AHD *act, QEN_HASH_AGGREGATE *haptr)
{

    QEF_CB *qefcb = dsh->dsh_qefcb;
    QEF_RCB *rcb = qefcb->qef_rcb;
    char *cbuf = qefcb->qef_trfmt;
    i4 cbufsize = qefcb->qef_trsize;
    i4 rows_per_chunk;
    i4 val1, val2;

    if (! ult_check_macro(&qefcb->qef_trace, 93, &val1, &val2))
	return;

    STprintf(cbuf, "Hash agg summary for thread %d\n", dsh->dsh_threadno);
    qec_tprintf(rcb, cbufsize, cbuf);
    STprintf(cbuf, "Memory estimated (worst-case): %lu; allocated: %d\n",
	haptr->hshag_estmem, haptr->hshag_halloc);
    qec_tprintf(rcb, cbufsize, cbuf);
    STprintf(cbuf, "Chunk size, max count: %d, %d;  holding %d groups max\n",
	haptr->hshag_chsize, haptr->hshag_chmax, haptr->hshag_grpmax);
    qec_tprintf(rcb, cbufsize, cbuf);
    STprintf(cbuf, "Partitions: %d;  write, read buf sizes: %d,%d @ %dK\n",
	haptr->hshag_pcount, haptr->hshag_pbsize, haptr->hshag_rbsize,
	haptr->hshag_pagesize / 1024);
    qec_tprintf(rcb, cbufsize, cbuf);
    STprintf(cbuf, "Hash-ptr entries: %d (%d kb);  ovf row size: %d\n",
	haptr->hshag_htsize, (haptr->hshag_htsize * sizeof(PTR)) / 1024,
	haptr->hshag_obufsz);
    qec_tprintf(rcb, cbufsize, cbuf);
    rows_per_chunk = haptr->hshag_grpmax / haptr->hshag_chmax;
    STprintf(cbuf, "Chunks actually allocated: %d;  holding %d groups\n",
	haptr->hshag_challoc, haptr->hshag_challoc * rows_per_chunk);
    qec_tprintf(rcb, cbufsize, cbuf);
    STprintf(cbuf, "Rows in: %d;  Output: estimate: %d,  actual: %d\n",
	haptr->hshag_rcount, act->qhd_obj.qhd_qep.u1.s2.ahd_aggest,
	haptr->hshag_gcount);
    qec_tprintf(rcb, cbufsize, cbuf);
    STprintf(cbuf, "Overflowed rows: %d;  max recursion level: %d\n\n",
	haptr->hshag_ocount, haptr->hshag_maxreclvl);
    qec_tprintf(rcb, cbufsize, cbuf);

} /* haggf_stat_dump */
