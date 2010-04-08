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
#include    <ulf.h>
#include    <ulm.h>
#include    <adf.h>
#include    <ade.h>
#include    <adp.h>
#include    <dmf.h>
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
**  Name: QEAPUT.C - provides tuple append support
**
**  Description:
**      The routines in this file append records to open
**  relations. 
**
**          qea_append - append one or more records to the open relation.
**
**
**  History:
**      10-jul-86 (daved)
**          written
**	02-feb-88 (puree)
**	    add reset flag to qea_append function arguments.
**	29-aug-88 (puree)
**	    change duplicate handling semantics
**	15-dec-88 (puree)
**	    reset was not declared in qea_append
**	20-feb-89 (paul)
**	    Update to handle a rule action list.
**	30-mar-89 (paul)
**	    Add resource limit checking.
**	20-sep-89 (paul)
**	    Allow TIDs to be used as parameters in rules.
**      12-feb-91 (mikem)
**          Added support to qea_append() for returning logical key values 
**	    through the QEF_RCB up to SCF.  This is added so that the logical 
**	    key value can be returned to the client which caused the insert.
**	08-dec-92 (anitap)
**	    Included <qsf.h> and <psfparse.h> for CREATE SCHEMA.  
**	17-may-93 (jhahn)
**	    Added fix for support of statement level unique constraints.
**	18-may-93 (nancy)
**	    Fix bug 30646. Fix condition for reporting duplicate row error so
**	    that attempt to insert a duplicate row always returns error.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	9-jul-93 (rickh)
**	    Made some ME calls conform to the prototypes.
**      13-sep-93 (smc)
**          Added <cs.h> for CS_SID.
**       5-Nov-1993 (fred)
**          Add support for removing large object temporaries...
**       7-Apr-1994 (fred)
**          Commented this out pending figuring out a better way.
**	7-nov-95 (inkdo01)
**	    Changes to replace QEN_ADF structure instances by pointers in
**	    QEF_AHD structures.
**      15-sep-1997 (angusm)
**          Bug 71888 - prevent silently ignored failure with duplicate
**          key/row from QUEL triggering rule on insert.
[@history_template@]...
**      05-may-1999 (nanpr01,stial01)
**          Set DMR_DUP_ROLLBACK when duplicate error will result in rollback
**          This allows DMF to dupcheck in indices after base table is updated.
**	24-jan-01 (inkdo01)
**	    Changes to QEF_QEP structure putting some fields in union 
**	    (for hash aggregation project).
**	23-feb-04 (inkdo01)
**	    Changed qef_rcb->error to dsh->dsh_error for || thread safety
**	    (and others).
**	9-Dec-2005 (kschendel)
**	    Remove obsolete and dangerous i2 cast on mecopy/fill.
**	28-Dec-2005 (kschendel)
**	    Use DSH references whenever possible instead of QEF RCB
**	    references.  (This simplifies the code, eliminating many
**	    RCB references, and reflects the new primacy of the DSH
**	    thanks to the parallel query changes.)
**/

/*
**  Definition of static variables and forward static functions.
*/

static VOID
pass_logical_key(
DMR_CB	*dmr_cb,
QEF_RCB *qef_rcb );



/*
** {
** Name: QEA_APPEND	- append rows to records
**
** Description:
**      If a QEP exists, then rows are retrieved from the QEP
**  and used to append the result relation. Otherwise, just
**  materialize the output tuple and append it. 
**
**	This same routine is also used for QEA_LOAD, which appends
**	rows in "load" mode.  LOAD is used in two situations:
**	- if the query is create-as-select or an equivalent (i.e.
**	  RETRIEVE INTO or DECLARE GTT AS SELECT).
**	- if the query is INSERT-SELECT into a heap and either there
**	  are no rules on the target, or they are safe rules (where
**	  safe means that the rule doesn't reference the TID, and the
**	  called DBP doesn't reference the target table in any way.)
**	  The query compiler takes care of these checks.
**	  (*Note* as of Nov 2009 we don't do insert-select this way,
**	  yet, but it's coming.)
**
**	We don't LOAD into non-heaps because it would change the observed
**	locking semantics.  (Heaps can't be concurrently inserted into
**	anyway, but one might expect it from non-heaps.)
**
**	The rule restrictions exist because tids may not exist when
**	loading a table, and row accesses may not operate like they would
**	when row-by-row inserting.
**
** Inputs:
**      action                          append action item
**				    E_QE0000_OK
**				    E_QE0017_BAD_CB
**				    E_QE0018_BAD_PARAM_IN_CB
**				    E_QE0019_NON_INTERNAL_FAIL
**				    E_QE0002_INTERNAL_ERROR
**
**	state			    DSH_CT_INITIAL for an append action
**				    DSH_CT_CONTINUE for a call back after
**				    an after rule action list is executed.
**				    DSH_CT_CONTINUE3 for a call back after
**				    a before rule action list is executed.
**
** Outputs:
**      qef_rcb
**	    .qef_rowcount		number of output tuples
**	    .qef_targcount		number of attempted puts
**	    .error.err_code		one of the following
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
**      10-jul-86 (daved)
**          written
**	02-feb-88 (puree)
**	    add reset flag to the function arguments.
**	29-aug-88 (puree)
**	    change duplicate handling semantics
**	15-dec-88 (puree)
**	    reset was not declared.
**	20-feb-89 (paul)
**	    Added support for invoking rule action lists.
**	30-mar-89 (paul)
**	    Add resource limit checking
**	28-sept-89 (eric)
**	    Add support to fix the sybase bug. This is done by returning
**	    a zerotup check flag when invalidating a query plan because
**	    it's QEN_NODE returns zero rows.
**      12-feb-91 (mikem)
**          Added support to qea_append() for returning logical key values 
**	    through the QEF_RCB up to SCF.  This is added so that the logical 
**	    key value can be returned to the client which caused the insert.
**	17-may-93 (jhahn)
**	    Added fix for support of statement level unique constraints.
**	18-may-93 (nancy)
**	    Fix bug 30646. Fix condition for reporting duplicate row error so
**	    that attempt to insert a duplicate row always returns error.
**	01-sep-93 (davebf)
**	    Fix bug 40262.  rowcount += i, targcount += j strategy returned
**	    wrong counts when there were actions internal to QEP
**       5-Nov-1993 (fred)
**          Added deleting of large object temporaries...
**       7-Apr-1994 (fred)
**          Commented this out pending figuring out a better way.
**	7-nov-95 (inkdo01)
**	    Changes to replace QEN_ADF structure instances by pointers in
**	    QEF_AHD structures.
**	8-july-96 (inkdo01)
**	    Init row buffer prior to append processing.
**      15-sep-1997 (angusm)
**	    Bug 71888: QUEL handling of insert of duplicates to tables
**          which do not allow them is to silently ignored them - i.e.
**          single duplicate row insert will succeed with 0 rows. However
**          if there is a rule on insert, this will not be prevented
**          from firing. This hurts QUEL applications which hit tables with 
**	    rules on them for Replicator.
**	30-apr-01 (inkdo01)
**	    Support "create table/insert ... as select first n ...".
**	18-mar-02 (inkdo01)
**	    Add code to perform next/current value operations on sequences.
**	21-nov-03 (inkdo01)
**	    Add open_flag parm to qen_func calls for parallel query support.
**	29-dec-03 (inkdo01)
**	    DSH is now parameter, "function" replaces "reset".
**	21-jan-04 (inkdo01)
**	    Add support for inserting to partitioned tables.
**	30-jan-04 (inkdo01)
**	    Minor initialization fix to above change.
**	27-feb-04 (inkdo01)
**	    Changed dsh_qen_status from ptr to array of QEN_STATUSs to 
**	    ptr to arrays of ptrs.
**	28-feb-04 (inkdo01)
**	    Change qef_rcb parm to dsh in qeq_chk_resources call.
**	12-may-04 (inkdo01)
**	    Check more carefully for pre-opened partitions (for rpt q's).
**	8-Jul-2004 (schka24)
**	    Still some tid byte-ordering confusion on x86, make tid handling
**	    more portable.
**	11-may-05 (hayke02)
**	    Move 'first n' test to the top of the for(;;) loop rather than the
**	    bottom. It will now work when rules are fired (call to qea_append()
**	    for each row appended) and the non-rule case (iteration in
**	    for(;;) loop for each row appended). This change fixes bug 114193,
**	    problem INGSRV 3225.
**	16-Jan-2006 (kschendel)
**	    Access qen-status thru xaddrs.
**	19-june-06 (dougi)
**	    Add support for BEFORE triggers.
**	18-dec-2007 (dougi)
**	    Update support of offset/first "n".
**	8-Sep-2008 (kibro01) b120693
**	    Remove unused action parameter from qeq_part_open
**      01-Dec-2008 (coomi01) b121301
**          Test DSH for presence of DBproc execution.
**	17-Nov-2009 (kschendel) SIR 122890
**	    Unify put and load, in anticipation of using LOAD for insert-
**	    select eventually.
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: If E_DM0029_ROW_UPDATE_CONFLICT returned,
**	    invalidate the QP and retry.
*/
DB_STATUS
qea_append(
QEF_AHD		*action,
QEF_RCB		*qef_rcb,
QEE_DSH		*dsh,
i4		function,
i4		state )
{
    QEN_NODE	*node = action->qhd_obj.qhd_qep.ahd_qep;
    bool	reset = ((function & FUNC_RESET) != 0);
    i4		open_flag = (reset) ? (TOP_OPEN | function) : function;
    QEF_CB	*qef_cb = dsh->dsh_qefcb;
    PTR		*cbs	= dsh->dsh_cbs;
    DB_STATUS	status;
    i4		rowno   = action->qhd_obj.qhd_qep.ahd_current->qen_output;
    char	*output = dsh->dsh_row[rowno];
    DMR_CB	*dmr_cb;
    DMT_CB	*dmt_cb;
    DM_MDATA	mdata;			/* Used for LOADing */
    ADE_EXCB	*ade_excb;
    i4		row_len = dsh->dsh_qp_ptr->qp_row_len[rowno];
    i4		odmr_cb_ix = action->qhd_obj.qhd_qep.ahd_odmr_cb;
    i4		dmtix = action->qhd_obj.qhd_qep.ahd_dmtix;
    i4		dmr_func;
    QEA_FIRSTN	*firstncbp;
    i4		first_n = 0;
    i4		offset_n = 0;
    DB_TID8	bigtid;
    u_i2	partno = 0;
    bool	firstrow = TRUE, newpart = FALSE;

    /* materialize row into update buffer */
    ade_excb    = (ADE_EXCB*) cbs[action->qhd_obj.qhd_qep.ahd_current->qen_pos];	
    if (state == DSH_CT_INITIAL)
    {
	/* Check if resource limits are exceeded by this query */
	if (action->qhd_obj.qhd_qep.ahd_estimates && (qef_cb->qef_fl_dbpriv &
	    (DBPR_QROW_LIMIT | DBPR_QDIO_LIMIT | DBPR_QCPU_LIMIT |
	    DBPR_QPAGE_LIMIT | DBPR_QCOST_LIMIT))
	    )
	{
	    if (qeq_chk_resource(dsh, action) != E_DB_OK)
		return (E_DB_ERROR);
	}

        dsh->dsh_qef_rowcount	    = 0;
	dsh->dsh_qef_targcount	    = 0;
	qef_rcb->qef_val_logkey     = 0;

	/* Determine "first n" value (if any) once I figure out how. */
    }

    dmr_cb = (DMR_CB*) cbs[odmr_cb_ix];
    dmr_func = DMR_PUT;
    if (action->ahd_atype == QEA_LOAD)
    {
	dmr_func = DMR_LOAD;
	mdata.data_address = output;	/* Reset stack variable */
	mdata.data_size = row_len;
	dmr_cb->dmr_mdata = &mdata;	/* and update address in dmrcb */
    }

    /* Check for CTAS with "offset/first n". */
    if (action->qhd_obj.qhd_qep.ahd_firstncb >= 0)
    {
	firstncbp = (QEA_FIRSTN *)dsh->dsh_cbs[action->qhd_obj.
							qhd_qep.ahd_firstncb];
	offset_n = firstncbp->get_offsetn;
	first_n = firstncbp->get_firstn;
    }


    /* If not resuming after a BEFORE trigger, clear the row buffer. */
    if (state != DSH_CT_CONTINUE3)
        MEfill(row_len, (u_char)0, (PTR)output);

    /* Skip right out if after-rule resumption of single row insert.
    ** Otherwise loop once (for insert/values) or N times (for insert
    ** select).
    */
    status = E_DB_OK;
    if (state != DSH_CT_CONTINUE || node != NULL)
      for (;;)
      {
	bool row_tossed;

	if (state != DSH_CT_CONTINUE3)
	{
	    /* This first stuff only gets executed before the BEFORE 
	    ** triggers. */

	    /* Check "first n" and pretend to be end-of-scan, if warranted. */
	    if (first_n > 0 && dsh->dsh_qef_targcount >= first_n)
	    {
		status = E_DB_OK;
		dsh->dsh_error.err_code = E_QE0000_OK;
		break;
	    }
	    /* fetch rows if there's a QP underneath */
	    if (node != NULL)
	    {
		status = (*node->qen_func)(node, qef_rcb, dsh, open_flag);
		open_flag &= ~(TOP_OPEN | FUNC_RESET);

		/* Check for "offset" clause and skip accordingly. 
		** NOTE: we don't decrement for the 1st row, because the
		** last row read below is to be inserted, not skipped. */
		if (offset_n > 0)
		{
		    firstncbp->get_offsetn = 0;		/* reset to avoid 
							** doing it again */
		    for ( ; offset_n-- > 0 && status == E_DB_OK; )
			status = (*node->qen_func)(node, qef_rcb, dsh, 
								open_flag);
		}

		/* If the qen_node returned no rows and we are supposed to
		** invalidate this query plan if this action doesn't get
		** any rows, then invalidate the plan.
		*/
		if (status != E_DB_OK &&
		    dsh->dsh_error.err_code == E_QE0015_NO_MORE_ROWS &&
		    dsh->dsh_xaddrs[node->qen_num]->qex_status->node_rcount == 0 &&
		    (action->ahd_flags & QEA_ZEROTUP_CHK) != 0)
		{
		    /* If this is a global temporary table being created
		    ** as a select, then this temp table must be destroyed
		    ** before retrying the query.
		    */
		    if (qef_rcb->qef_stmt_type & QEF_DGTT_AS_SELECT)
		    {
			DMT_CB	*dmt;
			i4	error;

			dmt = (DMT_CB *)cbs[dsh->dsh_dgtt_cb_no];

			/* Make sure that the table is closed. */
			status = dmf_call(DMT_CLOSE, dmt);
			if (status)
			{
			    qef_error(dmt->error.err_code, 0L, status,
				      &error, &dsh->dsh_error, 0);
			}
			dmt->dmt_record_access_id = (PTR)NULL;

			/* Now, delete the table. */
			dmt->dmt_flags_mask = 0;
			status = dmf_call(DMT_DELETE_TEMP, dmt);
			if (status)
			{
			    qef_error(dmt->error.err_code, 0L, status,
				      &error, &dsh->dsh_error, 0);
			}
		    }
		    dsh->dsh_qp_status |= DSH_QP_OBSOLETE;
		    dsh->dsh_error.err_code = E_QE0023_INVALID_QUERY;
		    qef_rcb->qef_intstate |= QEF_ZEROTUP_CHK;
		    if (dsh->dsh_qp_ptr->qp_ahd != action)
		    {
			status = E_DB_ERROR;
		    }
		    else
		    {
			status = E_DB_WARN;
		    }
		    return(status);
		}

		if (status != E_DB_OK)
		{
		    if (dsh->dsh_error.err_code == E_QE0015_NO_MORE_ROWS)
		    {
			status = E_DB_OK;
			dsh->dsh_error.err_code = E_QE0000_OK;
			/* This is normal end-of-rows, break out of loop */
			break;
		    }
		    return (status);
		}
	    } /* if node != null */

	    /* Before running current CX, check for sequences to update. */
	    if (action->qhd_obj.qhd_qep.ahd_seq != NULL)
	    {
		status = qea_seqop(action, dsh);
		if (status != E_DB_OK)
		    return(status);
	    }

	    /* Materialize the row to insert */
	    status = qen_execute_cx(dsh, ade_excb);
	    if (status != E_DB_OK)
		return (status);

	    dsh->dsh_qef_targcount++;

	    /* After projecting the row image to insert, check for
	    ** BEFORE triggers to apply. */
	    if (action->qhd_obj.qhd_qep.ahd_before_act != NULL)
	    {
		/*
		** Place the TID in a known location so it can be used
		** as a parameter to the procedure fired by the rule.
		*/
		bigtid.tid_i4.tpf = 0;
		bigtid.tid_i4.tid = -1;
		bigtid.tid.tid_partno = 0;
		MEcopy ((PTR)&bigtid, sizeof(DB_TID8),
		    dsh->dsh_row[action->qhd_obj.qhd_qep.u1.s1.ahd_a_tid]);   
		dsh->dsh_ctx_act[dsh->dsh_depth_act].dsh_ct_ptr = action;
		dsh->dsh_ctx_act[dsh->dsh_depth_act].dsh_ct_status =
		    DSH_CT_CONTINUE3;
		dsh->dsh_depth_act++;
		dsh->dsh_act_ptr = action->qhd_obj.qhd_qep.ahd_before_act;
		dsh->dsh_error.err_code = E_QE0120_RULE_ACTION_LIST;
		status = E_DB_ERROR;
		return(status);
	    }
	}	/* end of !DSH_CT_CONTINUE3 */

	state = DSH_CT_INITIAL;		/* signal resumption after
					** BEFORE trigger */

	/* Check for partitioning and set up DMR_CB. */
	if (action->qhd_obj.qhd_qep.ahd_part_def)
	{
	    status = adt_whichpart_no(dsh->dsh_adf_cb,
		    action->qhd_obj.qhd_qep.ahd_part_def, 
		    output, &partno);
	    if (status != E_DB_OK)
		return(status);

	    if ((dmr_cb = (DMR_CB *)cbs[odmr_cb_ix + partno + 1]) == NULL
	      || (dmt_cb = (DMT_CB *)cbs[dmtix + partno + 1]) == NULL
	      || dmt_cb->dmt_record_access_id == NULL)
	    {
		status = qeq_part_open(qef_rcb, dsh,
		    NULL, 0,
		    odmr_cb_ix, dmtix,
		    partno);

		if (status != E_DB_OK)
		    return(status);

		dmr_cb = (DMR_CB *)cbs[odmr_cb_ix + partno + 1];
		newpart = TRUE;
	    }
	    else
	    {
		/* Partition might be opened at valid time, but dmrcb addrs
		** not yet set up...they'll be null if so.
		*/
		newpart = (dmr_cb->dmr_mdata == NULL)
			&& (dmr_cb->dmr_data.data_address == NULL);
	    }
	    /* Mdata is stacklocal, must reset each time */
	    dmr_cb->dmr_mdata = &mdata;	/* only relevant/needed if load */
	}
	if (firstrow || newpart)
	{
	    /* Set up the DMR_CB. */
	    dmr_cb->dmr_tid		    = 0;	/* not used */
	    dmr_cb->dmr_val_logkey          = 0;
	    dmr_cb->dmr_flags_mask	    = 0;
	    if (dmr_func == DMR_LOAD)
	    {
		/* For LOAD, only pass the EMPTY flag and a row-estimate if
		** the table is newly created.  Otherwise we want DMF to
		** use non-rebuild non-recreate if the table is really
		** nonempty (which implies it must be a heap, or opf
		** would have compiled a PUT not a LOAD).
		*/
		dmr_cb->dmr_s_estimated_records = 0;
		dmr_cb->dmr_flags_mask = DMR_TABLE_EXISTS;
		if (action->qhd_obj.qhd_qep.ahd_qepflag & AHD_LOAD_CTAS)
		{
		    dmr_cb->dmr_flags_mask |= DMR_EMPTY;
		    if (node != NULL)
			dmr_cb->dmr_s_estimated_records = node->qen_est_tuples;
		}
		dmr_cb->dmr_count = 1;
	    }
	    else
	    {
		dmr_cb->dmr_data.data_address   = output;
		dmr_cb->dmr_data.data_in_size   = row_len;
		if (action->qhd_obj.qhd_qep.ahd_duphandle != QEF_SKIP_DUP)
		    dmr_cb->dmr_flags_mask = DMR_DUP_ROLLBACK;
	    }
	    firstrow = FALSE;
	}

	/* Bug 121301 :: We are deleting temporary blobs too early 
	**               inside dbprocs. This leads to a SegV.
	**
	** Before making the dmf call, we set a bitflag, dmr_dsh_isdbp, 
	** to say whether we are executing inside a dbproc.
	*/
	dmr_cb->dmr_dsh_isdbp = ((dsh->dsh_qp_ptr->qp_status & QEQP_ISDBP) != 0);

	/* perform the append */
	row_tossed = FALSE;
	status = dmf_call(dmr_func, dmr_cb);
	if (status != E_DB_OK)
	{
	    if ((dmr_cb->error.err_code == E_DM0046_DUPLICATE_RECORD ||
		 dmr_cb->error.err_code == E_DM0045_DUPLICATE_KEY ||
		 dmr_cb->error.err_code == E_DM0048_SIDUPLICATE_KEY) &&
		action->qhd_obj.qhd_qep.ahd_duphandle == QEF_SKIP_DUP)
	    {
		/* do nothing, eat dup row, don't count it */
		status = E_DB_OK;
		row_tossed = TRUE;
	    }
	    else
	    {
		if (dmr_cb->error.err_code == E_DM0045_DUPLICATE_KEY ||
		    dmr_cb->error.err_code == E_DM0048_SIDUPLICATE_KEY ||
		    dmr_cb->error.err_code == E_DM0150_DUPLICATE_KEY_STMT ||
		    dmr_cb->error.err_code == E_DM0151_SIDUPLICATE_KEY_STMT)
		    dsh->dsh_error.err_code = E_QE0012_DUPLICATE_KEY_INSERT;
		else
		if ( dmr_cb->error.err_code == E_DM0029_ROW_UPDATE_CONFLICT )
		{
		    dsh->dsh_qp_status |= DSH_QP_OBSOLETE;
		    dsh->dsh_error.err_code = E_QE0023_INVALID_QUERY;
		    status = E_DB_WARN;
		}
		else
		    dsh->dsh_error.err_code = dmr_cb->error.err_code;
		return (status);
	    }
	}
	else
	{
	    /* successful append */
	    if (++dsh->dsh_qef_rowcount == 1)
		pass_logical_key(dmr_cb, qef_rcb);
	}
	/* After a successful insert, look for rules to apply */
	if ((action->qhd_obj.qhd_qep.ahd_after_act != NULL) &&
	  ! row_tossed)
	{
	    /*
	    ** Place the TID in a known location so it can be used
	    ** as a parameter to the procedure fired by the rule.
	    ** This is meaningless but harmless if LOADing.
	    */
	    bigtid.tid_i4.tpf = 0;
	    bigtid.tid_i4.tid = dmr_cb->dmr_tid;
	    bigtid.tid.tid_partno = partno;
	    MEcopy ((PTR)&bigtid, sizeof(DB_TID8),
		dsh->dsh_row[action->qhd_obj.qhd_qep.u1.s1.ahd_a_tid]);   

	    dsh->dsh_ctx_act[dsh->dsh_depth_act].dsh_ct_ptr = action;
	    dsh->dsh_ctx_act[dsh->dsh_depth_act].dsh_ct_status =
		    DSH_CT_CONTINUE;
	    dsh->dsh_depth_act++;
	    dsh->dsh_act_ptr = action->qhd_obj.qhd_qep.ahd_after_act;
	    dsh->dsh_error.err_code = E_QE0120_RULE_ACTION_LIST;
	    status = E_DB_ERROR;
	    return (status);
	}

	if (node == NULL)
	    break;		/* Just once if insert/values */
    } /* row insert loop and if */

    if (dmr_func != DMR_LOAD)
	return (status);

    /* For load (only), we need to tell DMF to end the load.
    ** If this is partitioned table, loop over cbs looking for open 
    ** partitions to finish.  Otherwise end the only open table.
    */
    if (action->qhd_obj.qhd_qep.ahd_part_def != (DB_PART_DEF *) NULL)
    {
	i4 i;

	for (i = action->qhd_obj.qhd_qep.ahd_part_def->nphys_parts;
	     i > 0;
	     --i)
	{
	    if ((dmr_cb = (DMR_CB *)cbs[odmr_cb_ix + i]) != (DMR_CB *) NULL
	      && (dmt_cb = (DMT_CB *)cbs[dmtix + i]) != NULL
	      && dmt_cb->dmt_record_access_id != NULL)
	    {
		dmr_cb->dmr_flags_mask = DMR_TABLE_EXISTS | DMR_ENDLOAD;
		dmr_cb->dmr_count = 0;
		status = dmf_call(DMR_LOAD, dmr_cb);
		if (status != E_DB_OK)
		    break;
	    }
	}
    }
    else
    {
	dmr_cb->dmr_flags_mask = DMR_TABLE_EXISTS | DMR_ENDLOAD;
	dmr_cb->dmr_count = 0;
	status = dmf_call(DMR_LOAD, dmr_cb);
    }
    if (status != E_DB_OK)
    {
	dsh->dsh_error.err_code = dmr_cb->error.err_code;
    }
    return (status);	
}

/*{
** Name: pass_logical_key()	- short comment
**
** Description:
**	Given the dmf control block determine if the last insert assigned
**	a logical key.  If a logical key was assigned, then set the relevent 
**	members are updated in the QEF control block.  
**
**	This could have been in-line code, but it needed to be placed in
**	3 different places, and it made the code easier to read this way.
**
** Inputs:
**	dmr_cb				control block used by the last dmf_call.
**	qef_rcb				QEF control block for current query.
**
** Outputs:
**	none.
**
**	Returns:
**	    VOID
**
** History:
**      12-feb-91 (mikem)
**          Created.
*/
static VOID
pass_logical_key(
DMR_CB	*dmr_cb,
QEF_RCB *qef_rcb )
{

    if (dmr_cb->dmr_val_logkey & (DMR_TABKEY | DMR_OBJKEY))
    {
	/* if a system maintained logical key has been
	** assigned, then pass it up through the system,
	** to be returned to the client.
	*/
	if (dmr_cb->dmr_val_logkey & DMR_TABKEY)
	    qef_rcb->qef_val_logkey = QEF_TABKEY;
	else
	    qef_rcb->qef_val_logkey = 0;

	if (dmr_cb->dmr_val_logkey & DMR_OBJKEY)
	    qef_rcb->qef_val_logkey |= QEF_OBJKEY;

	STRUCT_ASSIGN_MACRO(dmr_cb->dmr_logkey, qef_rcb->qef_logkey);
    }

    return;
}
/*[@function_definition@]...*/
