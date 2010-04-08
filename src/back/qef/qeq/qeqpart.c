/*
** Copyright (c) 2004, Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <bt.h>
#include    <cs.h>
#include    <st.h>
#include    <pc.h>
#include    <lk.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <ulf.h>
#include    <ulm.h>
#include    <adf.h>
#include    <ade.h>
#include    <dmf.h>
#include    <dmrcb.h>
#include    <dmtcb.h>
#include    <qefmain.h>
#include    <qsf.h>
#include    <scf.h>
#include    <qefrcb.h>
#include    <qefcb.h>
#include    <qefnode.h>
#include    <psfparse.h>
#include    <qefact.h>
#include    <qefqp.h>
#include    <qefdsh.h>
#include    <qefscb.h>

#include    <dmmcb.h>
#include    <dmucb.h>
#include    <ex.h>
#include    <tm.h>

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
**  Name: QEQPART.C - functions for table partitioning support.
**
**  Description:
**	The functions in this file provide support for QEF management 
**	of table partitions.
**
**	qeq_join_pqeof - Join node reached EOF on outer, check results
**	qeq_join_pqreset - Join node is RESET, reset part-qual runtime state
**	qeq_join_pquals - Evaluate list of possible qualifications at a
**		join node
**	qeq_part_open - allocates/formats/opens DMR_CB/DMT_CB for 
**		a table partition
**	qeq_pqual_eval - Evaluate a qualification, build result bitmap
**	qeq_pqual_init - Do CX setup and init for a qualification
**
**	Routines are listed alphabetically to make them easier to find.
**
**  History:
**	22-jan-04 (inkdo01)
**	    Written for table partitioning.
**	23-feb-04 (inkdo01)
**	    Changed qef_rcb->error to dsh->dsh_error for || thread safety.
**	24-Jul-2007 (kschendel) SIR 122513
**	    Significant additions for generalized partition qualification.
**	9-Jan-2008 (kschendel) SIR 122513
**	    A few misc changes to include K-joins and T-joins.
**/


/*
** Name: qeq_join_pqeof - Reached outer EOF at a join node
**
** Description:
**
**	This routine is called when a join node that is doing
**	partition qualification reaches EOF on its outer.
**
**	At the moment, the only thing that we need to do is take a
**	look at the local-result bitmap(s).  Since all of the
**	completed node-result bitmaps are AND'ed, if ours ended
**	up sufficiently small, we might as well turn off join-time
**	evaluation for nodes underneath us;  they can't make the
**	restriction enough better to be worth it.
**
**	While various definitions of "sufficiently small" come to
**	mind, for starters I'll use "zero or one".
**
** Inputs:
**	dsh			Data segment header for thread
**	pqual			First (of N) QEN_PART_QUAL ptr for node
**
** Outputs:
**	None
**
** History:
**	27-Jul-2007 (kschendel) SIR 122513
**	    Written.
*/

void
qeq_join_pqeof(QEE_DSH *dsh, QEN_PART_QUAL *pqual)
{
    i4 ppcount;				/* Phys partitions that can join */
    QEE_PART_QUAL *rqual;		/* Runtime state */

    do
    {
	rqual = &dsh->dsh_part_qual[pqual->part_pqual_ix];
	if (rqual->qee_qualifying)
	{
	    ppcount = BTcount(rqual->qee_lresult, rqual->qee_qdef->nphys_parts);
	    if (ppcount <= 1)
	    {
		QEE_PART_QUAL *rq;

		/* Start at orig and turn off all the qualifying except us. */
		rq = &dsh->dsh_part_qual[rqual->qee_orig_pqual->part_pqual_ix];
		rq = rq->qee_qnext;
		while (rq != NULL)
		{
		    if (rq != rqual)
			rq->qee_qualifying = FALSE;
		    rq = rq->qee_qnext;
		}
	    }
	}
	pqual = pqual->part_qnext;
    } while (pqual != NULL);

} /* qeq_join_pqeof */

/*
** Name: qeq_join_pqreset - Reset qualification at a join node
**
** Description:
**
**	When a join node (hash or FSM) is reset, we need to puts its
**	runtime state into "start over".  While this is very simple,
**	it might as well be a routine so that it can be called from
**	various places.
**
** Inputs:
**	dsh			Data segment header for thread
**	pqual			First (of N) QEN_PART_QUAL ptr for node
**
** Outputs:
**	None
**
** History:
**	27-Jul-2007 (kschendel) SIR 122513
**	    Written.
*/

void
qeq_join_pqreset(QEE_DSH *dsh, QEN_PART_QUAL *pqual)
{
    QEE_PART_QUAL *rqual;		/* Runtime state */

    do
    {
	rqual = &dsh->dsh_part_qual[pqual->part_pqual_ix];
	/* Allow qualifying again unless turned off by orig */
	rqual->qee_qualifying = rqual->qee_enabled;
	MEfill(rqual->qee_mapbytes, 0, rqual->qee_lresult); /* Reset to 0 */
	pqual = pqual->part_qnext;
    } while (pqual != NULL);

} /* qeq_join_pqreset */

/*
** Name: qeq_join_pquals - Evaluate possible partition qualifications
**				at a join node
**
** Description:
**
**	This routine is called at a joining node, hash join or FSM join(*),
**	if the node can potentially supply partition qualification to
**	some orig on its inner side.  qeq_join_pquals is called when
**	an outer row is presented;  all active partition qualifications
**	are evaluated given that outer row's value.
**
**	(*) For an FSM-join, the qualification is actually done as
**	part of the tsort under the FSM outer, as the tsort is the
**	actual row accumulation point.
**
**	The query compiler arranges the evaluation directives so as to
**	end up OR'ing the resulting physical-partition bitmap into
**	the "local result" bitmap for this node.  Eventually, after
**	all of the outer rows are presented, the local-result bitmap
**	is AND'ed into the corresponding orig node's partition bitmap
**	(that part is done elsewhere).
**
**	Running the partition qualification directives is not free,
**	and if the outer input is large, it's quite likely that we
**	will end up not restricting the result at all.  (i.e. it will
**	become all-ones.)  We check for that situation and turn on
**	the flag saying "don't bother any more" if it occurs.
**
**	TODO: as long as there are no OJ's in the way between this
**	node and the orig, we could test this row's resultant bitmap
**	against the orig's constant map.  If this row's map isn't a
**	subset, in theory it can be tossed preemptively, because the
**	row(s) which it would join to are being excluded by the
**	orig constant qual.  This is a possibly nifty idea, but will
**	require that the eval process be broken apart a bit (to expose
**	the row's map before OR'ing it to local-result).  Don't have
**	the time to analyze that fully ...
**
** Inputs:
**	dsh			Data segment header for thread
**	pqual			First (of N) QEN_PART_QUAL ptr for node
**
** Outputs:
**	Returns E_DB_xxx OK or error status
**
** Side Effects:
**	Updates local-result bitmap, or qualifying bool in runtime data.
**
** History:
**	27-Jul-2007 (kschendel) SIR 122513
**	    Written.
*/

DB_STATUS
qeq_join_pquals(QEE_DSH *dsh, QEN_PART_QUAL *pqual)
{

    DB_STATUS status;
    QEE_PART_QUAL *rqual;		/* Execution-specific runtime data */

    do
    {
	rqual = &dsh->dsh_part_qual[pqual->part_pqual_ix];
	if (! rqual->qee_qualifying)
	{
	    pqual = pqual->part_qnext;	/* Skip useless one, do next */
	    continue;
	}
	/* Run this qual against this outer row */
	status = qeq_pqual_eval(pqual->part_join_eval, rqual, dsh);
	if (status != E_DB_OK)
	    return (status);
	/* See if local-result is now all ones, if so it's useless.
	** FIXME a BTallones would be better, add later.
	*/
	if (BTcount(rqual->qee_lresult, rqual->qee_qdef->nphys_parts) == rqual->qee_qdef->nphys_parts)
	    rqual->qee_qualifying = FALSE;

	pqual = pqual->part_qnext;
    } while (pqual != NULL);

    return (E_DB_OK);
} /* qeq_join_pquals */

/*{
** Name: QEQ_PART_OPEN	- Open a partition of a partitioned table
**
** Description:
**      Allocate/init DMR_CB and DMT_CB, then open DMT_CB for table
**	partition.  If the partition is already open, we do nothing
**	and return success.
**
** Inputs:
**	qef_rcb - rcb for the current query
**	dsh - dsh for the current query
**
** Outputs:
**
**	Returns:
**	    success or error status.
**	Exceptions:
**	    none
**
** Side Effects:
**	    The partition gets opened.
**
** History:
**      22-jan-04 (inkdo01)
**	    Written for partitioned table support.
**	10-mar-04 (inkdo01)
**	    Copy dmr_attr_desc from master DMR_CB to partition DMR_CB.
**	26-mar-04 (inkdo01)
**	    Use dsh_ahdvalid for locating tables - not all action hdrs
**	    have a valid chain.
**	6-may-04 (inkdo01)
**	    New code to avoid re-allocating DMT/DMR_CBs for repeat qs
**	    with DSHs containing already alloc'ed cbs.
**	19-Jul-2004 (schka24)
**	    Do vl lookup after we decide that open is needed.
**	28-Jul-2004 (jenjo02)
**	    Tweaked the function to also open an instance of a
**	    non-partitioned table on behalf of a Child thread.
**	30-Jul-2004 (jenjo02)
**	    Use dsh_dmt_id rather than qef_dmt_id transaction context.
**	22-Feb-2008 (kschendel) SIR 122513
**	    Ensure that mdata in dmrcb is nulled out (helps out qeaput).
**	    Use valid entry backpointer in master dmrcb to find the valid
**	    entry;  remove scan of dsh_ahdvalid.
**	8-Sep-2008 (kibro01) b120693
**	    Rework fix for b120693 avoiding need for extra flag (involving
**	    simply removing the original ahd_flags check (since action may
**	    be NULL, and allowing the lower check on (qp->qp_status &
**	    QEQP_DEFERRED) to take effect.  action is no longer used, so
**	    I have removed it.
**	4-Jul-2009 (kschendel) b122118
**	    Use open-and-link utility.  Also, this is a real open, not a
**	    validation open, so remove the retry with LK_N.
**	20-Nov-2009 (kschendel) SIR 122890
**	    Add needs-Xlock support for upcoming insert/select via LOAD.
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Tell DMF if table being opened is for a cursor.
**	26-Feb-2010 (jonj)
**	    Use DSH pointer as cursorid to dmt_open.
*/
DB_STATUS
qeq_part_open(
	QEF_RCB	    *qef_rcb, 
	QEE_DSH	    *dsh, 
	PTR	    rowptr,
	i4	    rowsize,
	i4	    dmrix,
	i4	    dmtix,
	u_i2	    partno)
{
    PTR		*cbs = dsh->dsh_cbs;
    DMT_CB	*dmt_cb;
    DMR_CB	*dmr_cb, *masterdmr_cb;
    QEF_VALID	*vl;
    DB_STATUS	status = E_DB_OK;
    QEF_QP_CB	*qp = dsh->dsh_qp_ptr;
    DMT_CHAR_ENTRY  dmt_char_entry[2];
    ULM_RCB	ulm;

    /*
    ** If dmtix == -1, this is a request to open an
    ** instance of a table for a Child thread (called
    ** from qen_orig() when dmr_cb->dmr_access_id has
    ** been set to NULL by qen_dsh().
    **
    ** In this case, look up the passed dmrix in the
    ** query's list of validate tables, then open
    ** an instance of the table, supplying dmr_access_id
    ** and linking the table into the DSH's list of
    ** tables opened by this Child thread.
    **
    ** It's presumed that since the table must have already
    ** been opened earlier, the corresponding DMT_CB has
    ** be initialized to all the proper locking, blah-blah,
    ** values, so we just open it.
    */
    if ( dmtix == -1 )
    {
	dmr_cb = (DMR_CB *)cbs[dmrix];
	/* Find dmt-cb via valid list entry used to open dmr-cb */
	vl = dmr_cb->dmr_qef_valid;
	if (vl == (QEF_VALID *) NULL)
	{
	    /* Gotta return some sort of error code */
	    dsh->dsh_qp_status |= DSH_QP_OBSOLETE;
	    dsh->dsh_error.err_code = E_QE0023_INVALID_QUERY;
	    return(E_DB_ERROR);	/* big trouble if no QEF_VALID */
	}
	dmt_cb = (DMT_CB*)cbs[vl->vl_dmf_cb];
	/* Fall through to open the table... */
    }
    else
    {
	/* Regular "open a partition" call */
	if ( (dmt_cb = (DMT_CB *)cbs[dmtix + partno + 1]) != (DMT_CB *) NULL)
	{
	    if (dmt_cb->dmt_record_access_id)
		return(E_DB_OK);	/* already open - nothing to do */
	    dmr_cb = (DMR_CB *)dsh->dsh_cbs[dmrix + partno + 1];
	}
	else
	{
	    /* Not already allocated, set up for memory allocation. */
	    STRUCT_ASSIGN_MACRO(Qef_s_cb->qef_d_ulmcb, ulm);
	    ulm.ulm_streamid_p = &dsh->dsh_streamid;
	    
	    ulm.ulm_psize = sizeof(DMT_CB) + sizeof(DMR_CB) + sizeof(DB_LOC_NAME);
	    status = qec_malloc(&ulm);
	    if (status != E_DB_OK)
	    {
		dsh->dsh_error.err_code = ulm.ulm_error.err_code;
		return (status);
	    }
	    dmt_cb 	= (DMT_CB*) ((char *) ulm.ulm_pptr); 
	    dsh->dsh_cbs[dmtix+partno+1] = (PTR) dmt_cb;
	    dmr_cb = (DMR_CB *) &ulm.ulm_pptr[sizeof(DMT_CB) + sizeof(DB_LOC_NAME)];
	    dsh->dsh_cbs[dmrix+partno+1] = (PTR) dmr_cb;

	    /* Address the single location entry. */
	    dmt_cb->dmt_location.data_address = &ulm.ulm_pptr[sizeof(DMT_CB)];
	}

	masterdmr_cb = (DMR_CB *) dsh->dsh_cbs[dmrix-1];
			    /* load master from stowed address */

	/* Locate QEF_VALID structure for this table (for partition 
	** db_tab_id values and later DMT_CB manipulation). */

	vl = masterdmr_cb->dmr_qef_valid;
	if (vl == (QEF_VALID *) NULL)
	{
	    dsh->dsh_qp_status |= DSH_QP_OBSOLETE;
	    dsh->dsh_error.err_code = E_QE0023_INVALID_QUERY;
	    return(E_DB_ERROR);	/* big trouble if no QEF_VALID */
	}

	/* Initialize and open. */
	dmt_cb->q_next = (PTR)NULL;
	dmt_cb->q_prev = (PTR)NULL;
	dmt_cb->length 	= sizeof (DMT_CB);
	dmt_cb->type = DMT_TABLE_CB;
	dmt_cb->owner = (PTR)DB_QEF_ID;
	dmt_cb->ascii_id = DMT_ASCII_ID;
	dmt_cb->dmt_db_id = qef_rcb->qef_db_id;
	dmt_cb->dmt_record_access_id = (PTR)NULL;
	/* Copy table ID or (if partitioned not the master
	** table) partition ID. */
	STRUCT_ASSIGN_MACRO(vl->vl_part_tab_id[partno], dmt_cb->dmt_id);
	dmt_cb->dmt_flags_mask = 0;
	MEfill(sizeof(DB_OWN_NAME), (u_char)' ',
	    (PTR)&dmt_cb->dmt_owner);
	
	dmt_cb->dmt_char_array.data_address = 0;
	dmt_cb->dmt_char_array.data_in_size = 0;
	dmt_cb->dmt_mustlock = vl->vl_mustlock;

	/* Initialize the DMR_CB */
	dmr_cb->q_next = (PTR)NULL;
	dmr_cb->q_prev = (PTR)NULL;
	dmr_cb->length = sizeof (DMR_CB);
	dmr_cb->type = (i2)DMR_RECORD_CB;
	dmr_cb->owner = (PTR)DB_QEF_ID;
	dmr_cb->ascii_id = DMR_ASCII_ID;
	dmr_cb->dmr_access_id = (PTR)NULL;

	/* Copy key info from master. */
	STRUCT_ASSIGN_MACRO(masterdmr_cb->dmr_attr_desc, 
				    dmr_cb->dmr_attr_desc);

	/* Now set up/open the DMT_CB. */
	dmt_cb->dmt_db_id = qef_rcb->qef_db_id;
	dmt_cb->dmt_sequence = dsh->dsh_stmt_no;
	dmt_cb->dmt_mustlock = vl->vl_mustlock;

	if (qef_rcb->qef_qacc == QEF_READONLY)
	    dmt_cb->dmt_access_mode = DMT_A_READ;
	else
	    dmt_cb->dmt_access_mode = vl->vl_rw;

	if ((qp->qp_status & QEQP_DEFERRED) != 0)
	    dmt_cb->dmt_update_mode = DMT_U_DEFERRED;
	else
	    dmt_cb->dmt_update_mode = DMT_U_DIRECT;

	/* default for lock granularity setting purposes */
	switch (dmt_cb->dmt_access_mode)
	{
	    case DMT_A_READ:
	    case DMT_A_RONLY:
		dmt_cb->dmt_lock_mode = DMT_IS;
		break;

	    case DMT_A_WRITE:
		dmt_cb->dmt_lock_mode = DMT_IX;
		/* see qeq-vopen in qeqvalid.c for explanation */
		if (vl->vl_flags & QEF_NEED_XLOCK)
		    dmt_cb->dmt_lock_mode = DMT_X;
		break;
	}

	/*
	** If executing a procedure that supports an integrity constraint, 
	** set DMT_CONSTRAINTS flag in the DMT_CB.
	*/
	if (qef_rcb->qef_intstate & QEF_CONSTR_PROC)
	    dmt_cb->dmt_flags_mask |= DMT_CONSTRAINT;

	if (vl->vl_flags & QEF_SESS_TEMP)
	    dmt_cb->dmt_flags_mask |= DMT_SESSION_TEMP;

	/* If opening for a cursor, tell DMF */
	if ( dsh->dsh_qp_status & DSH_CURSOR_OPEN )
	{
	    dmt_cb->dmt_flags_mask |= DMT_CURSOR;
	    dmt_cb->dmt_cursid = (PTR)dsh;
	}
    }

    /* This thread's transaction context */
    dmt_cb->dmt_tran_id	= dsh->dsh_dmt_id;

    dmt_char_entry[0].char_id = DMT_C_EST_PAGES;
    dmt_char_entry[0].char_value = vl->vl_est_pages;
    dmt_char_entry[1].char_id = DMT_C_TOTAL_PAGES;
    dmt_char_entry[1].char_value = vl->vl_total_pages;
    dmt_cb->dmt_char_array.data_in_size = 2 * sizeof(DMR_CHAR_ENTRY);
    dmt_cb->dmt_char_array.data_address = (PTR)dmt_char_entry;

    /* clear the qualification function */	
    dmr_cb->dmr_q_fcn = 0;
    dmr_cb->dmr_q_arg = 0;

    /* assign the output row and its size */
    dmr_cb->dmr_data.data_address = rowptr;
    dmr_cb->dmr_data.data_in_size = rowsize;
    dmr_cb->dmr_mdata = NULL;

    /* Open the table */

    status = qen_openAndLink(dmt_cb, dsh);

    /* Reset dmt_char_array for future calls */
    dmt_cb->dmt_char_array.data_address = 0;
    dmt_cb->dmt_char_array.data_in_size = 0;

    /*
    ** Turn off row update bit, since later calls may check for
    ** (in)valid flags_mask
    */
    if (status != E_DB_OK)
    {
	if (dmt_cb->error.err_code == E_DM0054_NONEXISTENT_TABLE ||
	    dmt_cb->error.err_code == E_DM0114_FILE_NOT_FOUND)
	{
	    dsh->dsh_qp_status |= DSH_QP_OBSOLETE;
	    dsh->dsh_error.err_code = E_QE0023_INVALID_QUERY;
	    status = E_DB_WARN;
	}
	else if (dmt_cb->error.err_code 
	      != E_DM0129_SECURITY_CANT_UPDATE)
	    dsh->dsh_error.err_code = dmt_cb->error.err_code;
	return(status);
    }

    /* Consistency check due to common AV */
    if (cbs[vl->vl_dmr_cb] == NULL)
    {
	i4		err;
	char		reason[100];

	status = E_DB_ERROR;
	STprintf(reason, "vl_dmr_cb = %d, cbs[%d] = NULL, qeqp_deferred=%d",
	     vl->vl_dmr_cb, vl->vl_dmr_cb, (qp->qp_status & QEQP_DEFERRED));
	qef_error(E_QE0105_CORRUPTED_CB, 0L, status, &err,
	      &dsh->dsh_error, 2,
	      sizeof("qeq_validate")-1, "qeq_validate",
	      STlength(reason), reason);
	dsh->dsh_error.err_code = E_QE0025_USER_ERROR;
	return(status);
    }
    /* Set the table record access id into the DMR_CB. */
    dmr_cb->dmr_access_id = dmt_cb->dmt_record_access_id;

    return(status);
}

/*
** Name: qeq_pqual_eval - Evaluate a qualification
**
** Description:
**
**	This routine executes a partition qualification for a table,
**	by interpreting a supplied QEN_PQ_EVAL array.  The result of
**	this evaluation is a physical-partition bitmap of qualifying
**	partitions, which is left in some DSH row as directed by
**	the evaluation itself.  Most likely, a constant (orig-like)
**	qual will AND itself to the constant-result bitmap; while
**	a join-time qual will OR itself to the local-result bitmap,
**	with further propagation down to the orig happening by
**	hand elsewhere.
**
**	The qualification is set up as a mini-program, or series
**	of directives.  The most interesting is the VALUE directive,
**	which inspects one or more values set up by the header CX in a
**	value row.  We run a whichpart against the value according to
**	the partitioning dimension's value-distribution scheme.
**	Next, the predicate testing-operator is used to decide
**	which partition bits to set:  just the partition holding the
**	value;  from there to the end (>=);  from the start to that
**	partition (<=);  or all but that partition (<>).  So as to
**	deal with IN-lists more efficiently, multiple values are
**	allowed;  each one is tested and the matching partition bit(s)
**	are set.  (I.e. the multiple-value case is an implicit OR.)
**
**	Dimensions with multiple columns in the distribution scheme
**	work exactly the same way.  The CX has to set up all the
**	column values in the value row, but that's the query compiler's
**	problem, not ours.
**
**	The initial result of VALUE bit-setting is a logical partition
**	bitmap.  In the case of a multi-dimension partitioning, that
**	logical partition bitmap is converted to a physical partition
**	bitmap.  This is part of the VALUE operation, not a separate
**	step.  By always working with physical partition bitmaps, we
**	can deal with multiple dimensions in one qualification (although
**	not in one VALUE operation, obviously).
**
**	Other EVAL directives perform bitmap zeroing and bitmap AND,
**	OR, and NOT operations.
**
**	The first EVAL directive is always a header giving the size
**	of the eval array, in bytes and instructions.  The CX that
**	generates all the necessary values is in the header as well.
**	(Conceptually, a CX per VALUE directive would be clearer, but
**	the CX is kind of heavy-weight, so it's better to have one
**	that sets up all the values end-to-end in one giant row.)
**
** Inputs:
**	pqe			QEN_PQ_EVAL * evaluation directives
**	rqual			QEE_PART_QUAL * pointer to runtime info
**				for qualification at this qp node
**	dsh			The data segment header
**
** Outputs:
**	E_DB_xxx OK or error status;
**	CX evaluation errors do the usual qef-adf-error thing.
**
** Side Effects:
**	Physical-partition bitmaps generated per the eval directives.
**
** History:
**	26-Jul-2007 (kschendel) SIR 122513
**	    Generalized partition qualification, expanded from dougi's
**	    original.
*/

DB_STATUS
qeq_pqual_eval(QEN_PQ_EVAL *pqe,
    QEE_PART_QUAL *rqual, QEE_DSH *dsh)
{
    ADF_CB *adfcb = dsh->dsh_adf_cb;
    bool multi_dim;			/* Single or multi dimension? */
    DB_STATUS status;			/* The usual status thing */
    i4 instrs_left;
    i4 map_bytes;			/* Bitmap size in bytes */
    i4 totparts;			/* Total physical partitions */
    PTR *dshrow = dsh->dsh_row;		/* We'll need this a lot */
    PTR result_row;
    PTR value_row;

    totparts = rqual->qee_qdef->nphys_parts;  /* Total physical partitions */
    map_bytes = rqual->qee_mapbytes;	/* PP bitmap sizes in bytes */
    multi_dim = (rqual->qee_qdef->ndims != 1);

    /* Run the CX in the header that does all the setup work.
    ** (For joins, there might not be a CX, if we happen to have all
    ** the right columns available in the right data types.)
    */
    if (pqe->un.hdr.pqe_cx != NULL)
    {
	ADE_EXCB *excb;

	excb = (ADE_EXCB *) dsh->dsh_cbs[pqe->un.hdr.pqe_cx->qen_pos];
	status = qen_execute_cx(dsh, excb);
	if (status != E_DB_OK)
	    return (status);
    }
    value_row = dshrow[pqe->un.hdr.pqe_value_row];

    /* Pick up number of instructions from the header, which is always
    ** first.  (Trust opc, don't check!)
    */
    instrs_left = pqe->un.hdr.pqe_ninstrs;
    while (--instrs_left > 0)
    {
	pqe = (QEN_PQ_EVAL *) ((char *)pqe + pqe->pqe_size_bytes);
	result_row = dshrow[pqe->pqe_result_row];
	switch (pqe->pqe_eval_op)
	{
	case PQE_EVAL_ZERO:
	    /* Rare, but occasionally used */
	    MEfill(map_bytes, 0, result_row);
	    break;

	case PQE_EVAL_ANDMAP:
	    BTand(totparts, dshrow[pqe->un.andor.pqe_source_row], result_row);
	    break;

	case PQE_EVAL_ORMAP:
	    BTor(totparts, dshrow[pqe->un.andor.pqe_source_row], result_row);
	    break;

	case PQE_EVAL_NOTMAP:
	    /* Unused, but in case we get here, result is "don't know" */
	    MEfill(map_bytes, -1, result_row);
	    break;

	case PQE_EVAL_VALUE:
	    {
		DB_PART_BREAKS *bp;	/* Break table entry for range stuff */
		DB_PART_DIM *dimp;	/* Scheme dimension pointer */
		i4 i, j;
		PTR lpp_map;		/* Logical partition map */
		PTR valuep;		/* Value in source row */
		u_i2 partno;		/* Logical partition returned */

		/* This is the interesting one that does the work */

		/* If multi-dimensional, generate the (logical) partition
		** bits into a work map.  If single dimension, logical
		** is physical, and we can operate directly into the
		** result row map.
		*/
		if (multi_dim)
		    lpp_map = rqual->qee_work1;
		else
		    lpp_map = result_row;
		MEfill(map_bytes, 0, lpp_map);
		dimp = &rqual->qee_qdef->dimension[pqe->un.value.pqe_dim];

		/* Iterate over the generated values calling the
		** appropriate adt_whichpart_nnn function to get partition
		** number(s).  Set bitmap bits per the relop instruction.
		*/
		i = pqe->un.value.pqe_nvalues;
		valuep = value_row + pqe->un.value.pqe_start_offset;
		for (;;)
		{
		    switch (dimp->distrule)
		    {
		    case DBDS_RULE_HASH:
			status = adt_whichpart_hash(adfcb, dimp, valuep, &partno);
			break;
		    case DBDS_RULE_LIST:
			status = adt_whichpart_list(adfcb, dimp, valuep, &partno);
			break;
		    case DBDS_RULE_RANGE:
			status = adt_whichpart_range(adfcb, dimp, valuep, &partno);
			break;
		    }
		    if (status != E_DB_OK)
		    {
			status = qef_adf_error(&adfcb->adf_errcb,
					status, dsh->dsh_qefcb,
					&dsh->dsh_error);
			if (status != E_DB_OK)
			    return (status);
		    }
		    /* Now set bits per relop.  For LE or GE, note that
		    ** we set or clear bits for the break entry partition
		    ** up to (LE) or beyond (GE) the break for partno.
		    ** Don't make the mistake of starting at partno and
		    ** going up or down from there!  It's the break table
		    ** that matters.
		    ** Example:  range partition scheme defined as:
		    **      >10,  >20, <=10.  Break table is:
		    **     <=10 (p2), > 10 (p0), > 20 (p1).
		    ** A LE operation on value 15 turns on bits 0 and 2.
		    ** (whichpart returns p0, we start at the beginning of
		    ** the break table and light bits until we come to the
		    ** entry with that partition, which is the >10 entry.)
		    **
		    ** Also keep in mind that partition sequence #'s are
		    ** one origin, not zero origin.
		    */
		    switch (pqe->un.value.pqe_relop)
		    {
		    case PQE_RELOP_EQ:
			BTset((i4)partno-1, lpp_map);
			break;
		    case PQE_RELOP_NE:
			/* NOTE: opc will only compile NE for LIST-type
			** dimensions;  they are the only ones where we can
			** potentially exclude an entire partition based on
			** only one value.  This loop rigmarole avoids
			** eliminating a partition that holds multiple values
			** (which would include the DEFAULT partition.)
			*/
			MEfill(map_bytes, -1, lpp_map);
			if (dimp->distrule != DBDS_RULE_LIST)
			    break;
			j = 0;
			for (bp = &dimp->part_breaks[dimp->nbreaks-1];
			     bp >= &dimp->part_breaks[0];
			     --bp)
			{
			    if (bp->partseq == partno)
			    {
				if (bp->oper == DBDS_OP_DFLT)
				    ++j;	/* Can't "NE" the default */
				if (++j > 1)
				    break;
			    }
			}
			/* Can only assume NE if only one break value maps
			** to the partition we located.
			*/
			if (j == 1)
			    BTclear((i4)partno-1, lpp_map);
			break;
		    case PQE_RELOP_LE:
			for (bp = &dimp->part_breaks[0];
			     bp < &dimp->part_breaks[dimp->nbreaks];
			     ++bp)
			{
			    BTset(bp->partseq-1, lpp_map);
			    if (partno == bp->partseq)
				break;
			}
			break;
		    case PQE_RELOP_GE:
			for (bp = &dimp->part_breaks[dimp->nbreaks-1];
			     bp >= &dimp->part_breaks[0];
			     --bp)
			{
			    BTset(bp->partseq-1, lpp_map);
			    if (partno == bp->partseq)
				break;
			}
			break;
		    }
		    if (--i == 0)
			break;		/* for-loop exit */
		    /* Move on to next value */
		    valuep += pqe->un.value.pqe_value_len;
		}

		/* If multi-dimensional, convert LP to PP by extracting
		** each logical partition bit, and laboriously turning on
		** the physical partition bits that correspond to that
		** logical partition.  (For a pretty picture of LP-to-PP
		** correspondence, see qenorig.c.)
		*/
		if (multi_dim)
		{
		    MEfill(map_bytes, 0, result_row);
		    i = -1;
		    while ( (i = BTnext(i, lpp_map, dimp->nparts)) != -1)
		    {
			/* Set bits for logical partition i */
			partno = i * dimp->stride;
			do
			{
			    /* Set "stride" bits, then skip the gap */
			    for (j = dimp->stride; j > 0; --j)
			    {
				BTset(partno, result_row);
				++partno;
			    }
			    partno = partno + (dimp->nparts-1) * dimp->stride;
			} while (partno < totparts);
		    }
		}
		break;			/* this case */
	    }
	default:
	    TRdisplay("%@ qeq_pqual_eval: invalid eval op %d\n", pqe->pqe_eval_op);
	    return (E_DB_ERROR);
	} /* endswitch */
    } /* directive loop */

    return (E_DB_OK);

} /* qeq_pqual_eval */

/*
** Name: qeq_pqual_init - Initialize part-qual control blocks
**
** Description:
**
**	This routine is called one (or more) times at query
**	setup time, to initialize any of the QEN_PART_QUAL
**	structs attached to a node.
**
**	The CX that generates up the predicate evaluation values
**	has to be set up, which means either generating an ADE_EXCB
**	control block for it, or filling in user repeat-query
**	parameter pointers for CX bases.  (The first happens once
**	when the query is first allocated in qee, the second happens
**	at the start of each execution if it's a repeated query.)
**
**	Also, at qee query setup time, the runtime QEE_PART_QUAL
**	structures have to be initialized.  Since they are initially
**	zero-filled, we can just look at the qee_qdef pointer to
**	see if init is needed.  (qee_qdef is necessarily non-NULL
**	for a properly initialized QEE_PART_QUAL.)
**
** Inputs:
**	dsh			QEE_DSH * data segment header
**	pqual			QEN_PART_QUAL * pointer to first of a list
**				of partition qual structs for a plan node
**				(If null, return without doing anything)
**	cx_init			CX initialization routine;
**				either qee_ade or qeq_ade.
**
** Outputs:
**	Returns E_DB_OK or E_DB_xxx error code
**	(propagated from CX-init routine, which takes care of
**	filling in any error codes into the proper place.)
**
** History:
**	25-Jul-2007 (kschendel) SIR 122513
**	    Write for generalized partition qualification.
*/

DB_STATUS
qeq_pqual_init(QEE_DSH *dsh, QEN_PART_QUAL *pqual,
	DB_STATUS (*cx_init)(QEE_DSH *, QEN_ADF *) )
{

    DB_STATUS status;
    i4 i;
    QEE_PART_QUAL *rqual;		/* Pointer to runtime info */
    QEE_PART_QUAL *orig_rqual;		/* Rqual block for related ORIG */
    QEN_PART_QUAL *orig_qual;		/* Related orig's qual struct */
    QEN_PQ_EVAL *pqe;			/* Traverse evaluation array */

    while (pqual != NULL)
    {
	/* Let's start by setting up the CX's in the evaluation arrays. */
	pqe = pqual->part_const_eval;
	if (pqe != NULL)
	{
	    status = (*cx_init)(dsh, pqe->un.hdr.pqe_cx);
	}

	/* Repeat for joining qualification CX.  Usually there is just
	** one or the other;  K/T-joins might have both.
	*/
	pqe = pqual->part_join_eval;
	if (pqe != NULL)
	{
	    status = (*cx_init)(dsh, pqe->un.hdr.pqe_cx);
	}

	/* See if the runtime QEE_PART_QUAL control block needs to be
	** initialized.  Most of the row pointers are convenience items
	** copied from the related orig node.  (of course, this might BE
	** the related orig!  doesn't matter...)
	*/
	rqual = &dsh->dsh_part_qual[pqual->part_pqual_ix];
	rqual->qee_qualifying = TRUE;	/* Always init these */
	rqual->qee_enabled = TRUE;	/* in case it's the orig */
	if (rqual->qee_qdef == NULL)
	{
	    QEN_NODE *node;

	    node = pqual->part_orig;
	    /* Might be a real ORIG, or a K or T join... */
	    if (node->qen_type == QE_KJOIN)
		orig_qual = node->node_qen.qen_kjoin.kjoin_pqual;
	    else if (node->qen_type == QE_TJOIN)
		orig_qual = node->node_qen.qen_tjoin.tjoin_pqual;
	    else
		orig_qual = node->node_qen.qen_orig.orig_pqual;
	    rqual->qee_orig_pqual = orig_qual;
	    orig_rqual = &dsh->dsh_part_qual[orig_qual->part_pqual_ix];
	    rqual->qee_qdef = orig_qual->part_qdef;
	    rqual->qee_mapbytes = (rqual->qee_qdef->nphys_parts + 7) / 8;
	    /* If rqual isn't the orig's, link it into the list.  The
	    ** order is irrelevant as long as the orig's rqual stays in front.
	    */
	    if (rqual != orig_rqual)
	    {
		rqual->qee_qnext = orig_rqual->qee_qnext;
		orig_rqual->qee_qnext = rqual;
	    }
	    /* Some of these aren't really optional, but be safe. */
	    if (orig_qual->part_constmap_ix >= 0)
		rqual->qee_constmap = dsh->dsh_row[orig_qual->part_constmap_ix];
	    if (pqual->part_lresult_ix >= 0)
		rqual->qee_lresult = dsh->dsh_row[pqual->part_lresult_ix];
	    if (orig_qual->part_work1_ix >= 0)
		rqual->qee_work1 = dsh->dsh_row[orig_qual->part_work1_ix];
	    /* Orig may have already decide to turn join-time qual off... */
	    rqual->qee_qualifying = rqual->qee_enabled = orig_rqual->qee_enabled;
	}

	/* Hash and FSM-joins (actually tsorts) can qualify for multiple
	** tables underneath, so keep whirling until we do all of the
	** quals for this node.
	*/
	pqual = pqual->part_qnext;
    }

    return (E_DB_OK);
} /* qeq_pqual_init */

