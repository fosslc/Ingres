/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <me.h>
#include    <pc.h>
#include    <st.h>
#include    <cs.h>
#include    <lk.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <ulf.h>
#include    <ulm.h>
#include    <gca.h>
#include    <adf.h>
#include    <ade.h>
#include    <dmf.h>
#include    <dmrcb.h>
#include    <dmscb.h>
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
#include    <qefqeu.h>
#include    <qefscb.h>

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
**  Name: QEA_FETCH.C - fetch rows from a QEP
**
**  Description:
**      The QEP is used to produce the requested number of rows. 
**
**          qea_fetch - fetch rows from a QEP
**
**
**  History:    $Log-for RCS$
**      8-jul-86 (daved)
**          written
**	7-oct-87 (puree)
**	    set EOF after fetching from a constant function.
**	4-jan-88 (puree)
**	    fix bug in constant function in repeat query.
**	15-jan-88 (puree)
**	    implement QEQP_SIGLETON for performance improvement.
**      26-jan-88 (puree)
**          QEQP_SINGLETON applies only to the topmost action.
**	02-feb-88 (puree)
**	    add reset flag to qea_fetch function arguments.
**	08-aug-88 (puree)
**	    fix problem with output buffer
**	30-mar-89 (paul)
**	    Add resource limit checking.
**	14-dec-90 (davebf)
**	    Fix bug 7186 -- alternating repeat query failure
**	13-feb-91 (davebf)
**	    Avoid AV when ahd_current.qen_uoutput = -1 as in a DBP (b35209).
**      12-dec-1991 (fred)
**	    Added support for peripheral datatypes.
**	08-dec-92 (anitap)
**	    Added #include <qsf.h> and <psfparse.h> for CREATE SCHEMA.
**      13-sep-93 (smc)
**          Added <cs.h> for CS_SID.
**	25-oct-95 (inkdo01)
**	    Changes in ADF call preparation to refine blob processing
**	    requirements in ade_execute_cx.
**	7-nov-95 (inkdo01)
**	    Changes to replace QEN_ADF structure instances by pointers in
**	    QEF_AHD structures.
**	23-feb-04 (inkdo01)
**	    Changed qef_rcb->error to dsh->dsh_error for || thread safety
**	    (and others).
**	28-Dec-2005 (kschendel)
**	    Use DSH references whenever possible instead of QEF RCB
**	    references.  (This simplifies the code, eliminating many
**	    RCB references, and reflects the new primacy of the DSH
**	    thanks to the parallel query changes.)
**	25-jan-2007 (dougi)
**	    Overhauled to handle scrollable cursors.
**	25-apr-2007 (dougi)
**	    Changes to better support row status codes and cursor positions.
**	07-Dec-2009 (troal01)
**	    Consolidated DMU_ATTR_ENTRY, DMT_ATTR_ENTRY, and DM2T_ATTR_ENTRY
**	    to DMF_ATTR_ENTRY. This change affects this file.
**      19-Mar-2010 (horda03) B122979
**          Handle scrollable cursor temp. tables where a rows spans multiple
**          pages. Also replaced the use of 511 with the appropriate macro.
**/


GLOBALREF QEF_S_CB *Qef_s_cb;

/* Local functions. */
DB_STATUS qea_fetch_position(
QEF_AHD		*action,
QEF_RCB		*qef_rcb,
QEE_DSH		*dsh,
QEF_DATA	*output,
i4		function,
i4		*open_flag,
i4		position,
bool		fetchlast,
bool		*pre_read);

DB_STATUS qea_fetch_readqep(
QEF_AHD		*action,
QEF_RCB		*qef_rcb,
QEE_DSH		*dsh,
QEF_DATA	*output,
i4		function,
i4		*open_flag,
bool		doing_offset,
bool		*gotarow );

DB_STATUS qea_fetch_readtemp(
QEF_AHD		*action,
QEF_RCB		*qef_rcb,
QEE_DSH		*dsh,
QEF_DATA	*output,
i4		*rowstat );

static DB_STATUS qea_fetch_keycomp(
QEE_DSH		*dsh,
QEF_SCROLL	*scrollp,
PTR		buf1p,
PTR		buf2p,
i4		*result);

DB_STATUS qea_fetch_writetemp(
QEF_AHD		*action,
QEF_RCB		*qef_rcb,
QEE_DSH		*dsh,
QEF_DATA	*output);

DB_STATUS qea_seqop(
QEF_AHD		*action,
QEE_DSH		*dsh);

/*{
** Name: QEA_FETCH	- fetch rows from a QEP
**
** Description:
**      This routine is called to get a row from database table.  If the 
**	row is to be returned to the caller, qen_uoutput is set to the
**	corresponding ADE base number and the output buffer header is
**	provided in qef_rcb.qef_output (in QEF_DATA structure). If the
**	row is to be returned in a row buffer, the qen_uoutput is set
**	to -1.  In this case, the ADE base should have been set up 
**	during the action initialization (in qee_cract).
**
**	If this action is in the main QP, SCF is responsible for setting
**	up the qef_rcb.qef_output.  If it is in a sub QP, QEN_QP will be
**	responsible of setting up the correct output buffer.
**
**	As part of adding support for blob or peripheral types, an abstraction
**	change has been introduced.  QEF will now descriminate between the
**	number of rows returned and the number of buffers filled.  This allows a
**	single row to be split across a number of buffers.  The qef_rowcount
**	field represents the number of rows (actually, the number of
**	end-of-rows) returned, while the [previously unused] qef_count field
**	represents the number of buffers used.  For a more complete treatise on
**	the subject, see the comments for these fields and that of the overall
**	QEF_RCB structure in qefrcb.h.  However, it should be noted in the
**	following code that the number of rows and buffers used may be
**	different.  Currently, for queries involving "normal" types, they will
**	typically be the same.  However, for queries retrieving large objects,
**	they are likely to be different.  Examples of the relationship of the
**	two fields can be found in the aforementioned header directory.
**
**	qeafetch.c has been re-written to support scrollable cursors. The
**	logic has been unbundled to several functions to permit the main 
**	function to loop, filling the available buffers by calling other
**	functions to retrieve the result rows from either the query plan
**	or from the temp table used to spool result rows already materialized
**	from a scrollable cursor.
**
** Inputs:
**      action				fetch action item
**      qef_rcb                         the qef request block
**	    .qef_rowcount		the number of requested rows
**					    (Also, number of QEF_DATA blocks
**					    (listed below) which have been
**					    provided)
**	    .qef_output			a list of QEF_DATA blocks
**					sufficient in quantity to
**					hold the requested number of
**					tuples.
**
** Outputs:
**      qef_rcb
**	    .qef_rowcount		actual number of rows returned
**	    .qef_targcount		same as rowcount
**	    ,qef_count			number of buffers filled
**	    .qef_output			filled in row buffers
**	    .error.err_code		one of the following
**				E_QE0000_OK
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
**      8-jul-1986 (daved)
**          written
**	7-oct-87 (puree)
**	    set EOF after fetching from a constant function.
**	4-jan-88 (puree)
**	    re-init ade_excb->excb_seg to ADE_SMAIN after a constant
**	    function returns NO_MORE_ROW so the DSH can be reused in
**	    a repeat query.
**	15-jan-88 (puree)
**	    if the QEQP_SINGLETON bit is set in qp_status, return one
**	    row and set NO_MORE_ROWS status.
**      26-jan-88 (puree)
**          QEQP_SINGLETON applies only to the topmost action.  The
**	    topmost action is the action associated with node number 0.
**	02-feb-88 (puree)
**	    add reset flag to the function arguments.
**	21-jun-88 (puree)
**	    As part of DB procedure development, the topmost action can 
**	    now be identified by the QEA_NEWSTMT bit in ahd_flags in the 
**	    action header.  When the action is the topmost level, either
**	    the SINGLETON bit is set in qp_status, or when it is inside
**	    a DB procedure, only the first tuple is fetched.
**	08-aug-88 (puree)
**	    Fix problem with user's output buffer.  If qen_uoutput is set
**	    (not a -1), always use output buffer from qef_rcb.qef_output.
**	    Otherwise, use output buffer already set in the ADE base which
**	    is the case for a select into local variable in DB procedure
**	    execution.
**	30-mar-89 (paul)
**	    Add resource limit checking.
**	28-sept-89 (eric)
**	    Add support to fix the sybase bug. This is done by returning
**	    a zerotup check flag when invalidating a query plan because
**	    it's QEN_NODE returns zero rows.
**	14-dec-90 (davebf)
**	    Fix bug 7186 -- alternating repeat query failure
**	13-feb-91 (davebf)
**	    Avoid AV when ahd_current.qen_uoutput = -1 as in a DBP (b35209).
**      12-dec-1991 (fred)
**          Added support for peripheral datatypes.  This support entails
**	    noticing the E_AD0002_INCOMPLETE status returned from
**	    ade_execute_cx(), and returning a corresponding error.  This code
**	    then awaits callback, which it notices by noting that the
**	    excb_continuation field of the ade_excb is nonzero, at which point
**	    it continues processing.
**
**	    This change introduced the abstraction change between the number of
**	    rows returned and the number of buffers filled.  At the same time,
**	    the code migirated to a single point of return (with one exception
**	    that wasn't worth fixing -- the case of major early failure), and
**	    variable names were introduced to clarify the aforementioned
**	    abstraction.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      19-jun-95 (inkdo01)
**          Replaced ahd_any i4  by ahd_qepflag flag word.
**	25-oct-95 (inkdo01)
**	    Changes in ADF call preparation to refine blob processing
**	    requirements in ade_execute_cx.
**	7-nov-95 (inkdo01)
**	    Changes to replace QEN_ADF structure instances by pointers in
**	    QEF_AHD structures.
**	29-oct-97 (inkdo01)
**	    Added code to detect AHD_NOROWS (meaning query has reverse 
**	    syllogism and can't return any rows).
**	30-oct-98 (inkdo01)
**	    Extra logic for QEA_GET's inside for-loops of dbprocs.
**	03-mar-00 (inkdo01)
**	    Code to implement "select first n ...".
**	21-nov-03 (inkdo01)
**	    Add open_flag parm to qen_func calls for parallel query support.
**	29-dec-03 (inkdo01)
**	    DSH is now parameter, "function" replaces "reset".
**	27-feb-04 (inkdo01)
**	    Changed dsh_qen_status from ptr to array of QEN_STATUSs to 
**	    ptr to arrays of ptrs.
**	28-feb-04 (inkdo01)
**	    Change qef_rcb parm to dsh in qeq_chk_resources call.
**	16-Jan-2006 (kschendel)
**	    Access qen-status thru xaddrs.
**	1-May-2006 (kschendel)
**	    Set blob "limited base" properly for global basearray case.
**	25-jan-2007 (dougi)
**	    Complete reorganization to introduce scrollable cursors.
**	17-apr-2007 (dougi)
**	    Slight fix to handle calls from "execute immediate"s.
**	5-july-2007 (dougi)
**	    Slight fix to set position/status for non-scrollable cursors.
**	30-oct-2007 (dougi)
**	    Slight fix to help "fetch first n" work with RPPs.
**	14-dec-2007 (dougi)
**	    Changes to support parameterized offset/first "n".
**	21-Jan-2008 (kschendel) b122118
**	    Implement QE11 row tosser, for the 4th time (in Doug's new
**	    code framework.)
**	28-jan-2008 (dougi)
**	    Move ahd_rssplix to qp_rssplix.
**	19-march-2008 (dougi) BUG 120130
**	    Fix offset - was being executed before init of other vars.
**	16-Nov-2009 (thaju02) B122848
**	    If rowoffset, qea_fetch_readqep() should not write these 
**	    tuples to result set spool file. 
*/
DB_STATUS
qea_fetch(
QEF_AHD		*action,
QEF_RCB		*qef_rcb,
QEE_DSH		*dsh,
i4		function )
{
    bool	reset = ((function & FUNC_RESET) != 0);
    bool	node_reset = reset;
    i4	 	rowcount;
    QEF_DATA	*output	 = dsh->dsh_qef_output;
    i4	 	bufs_used;
    i4	 	buf_count;
    i4		open_flag = (reset) ? (TOP_OPEN | function) : function;
    DB_STATUS	status = E_DB_OK;
    QEF_CB	*qef_cb = dsh->dsh_qefcb;
    QEE_RSET_SPOOL *sptr = NULL;
    QEA_FIRSTN	*firstncbp = NULL;
    QEF_QP_CB	*qp = dsh->dsh_qp_ptr;
    i4		rowno, rowstat;
    i4 		qmode = qp->qp_qmode;
    i4		qp_status = qp->qp_status;
    i4		rowlim = 0;
    i4		rowoffset = 0;
    i4		rowtoss = 0;		/* QE11 tossing counter */
    i4		error;
    bool	scroll = (action->qhd_obj.qhd_qep.ahd_qepflag & AHD_SCROLL);
    bool	rowinspool, fetchlast, pre_read, skipread;
    bool	gotarow;
    bool	first, last, next, prior, absolute, relative;

#ifdef xDEBUG
    (VOID) qe2_chk_qp(dsh);
#endif

    /* Check if resource limits are exceeded by this query */
    if (action->qhd_obj.qhd_qep.ahd_estimates && (qef_cb->qef_fl_dbpriv &
        (DBPR_QROW_LIMIT | DBPR_QDIO_LIMIT | DBPR_QCPU_LIMIT |
        DBPR_QPAGE_LIMIT | DBPR_QCOST_LIMIT))
        )
    {
        if (qeq_chk_resource(dsh, action) != E_DB_OK)
	    return (E_DB_ERROR);
    }

    /* Set various row and buffer counts. */
    if (qmode == QEQP_EXEDBP)
	buf_count = 1;
    else
    {
	buf_count = dsh->dsh_qef_rowcount;
	if (!scroll && action->qhd_obj.qhd_qep.ahd_qepflag & AHD_MAIN)
	{
	    /* set trace point QE11 NN
	    ** outputs NN rows, eats the rest, but runs the QP and
	    ** resdom CX's to completion unlike first N.  Good for
	    ** timing tests when you don't want the row-delivery time
	    ** included and don't want to disturb the QP.
	    ** Ignored for scrollable cursor, too complicated.
	    ** FIXME need to not do this inside dbproc or tproc
	    */
	    rowtoss = dsh->dsh_trace_rows;
	    if (rowtoss > 0)
		++rowtoss;	/* since we post-decr to 1 then stop */
	}
    }

    /* Some "execute immediate" queries can sneak in with buf_count = -1.
    ** If there's an output buffer, we must assume there's at least 1. */
    if (!scroll && buf_count <= 0 && output && output->dt_data)
	buf_count = 1;

    if (buf_count <= 0 && !scroll)
    {
	qef_error(E_QE00A6_NO_DATA, 0L, E_DB_ERROR,
		    &error, &dsh->dsh_error, 0);
	return(E_DB_ERROR);
    }
    else if (buf_count == 0)
    {
	/* 0 rows are requested for a scrollable cursor. This means 
	** we have only to position the cursor, but we still need a
	** buffer for qea_fetch to materialize the result row. */
	ULM_RCB		ulm;
	i4		rtupsize = ((GCA_TD_DATA *)dsh->dsh_qp_ptr->
				qp_sqlda)->gca_tsize;

	STRUCT_ASSIGN_MACRO(Qef_s_cb->qef_d_ulmcb, ulm);
	ulm.ulm_streamid_p = &dsh->dsh_streamid;
	ulm.ulm_psize = rtupsize + sizeof(QEF_DATA);
	status = qec_malloc(&ulm);
	
	if (status != E_DB_OK)
	{
	    dsh->dsh_error.err_code = ulm.ulm_error.err_code;
	    return (status);
	}
	output = (QEF_DATA *)ulm.ulm_pptr;
	output->dt_next = NULL;
	output->dt_data = ulm.ulm_pptr + sizeof(QEF_DATA);
    }

    dsh->dsh_qef_rowcount = 0;
    dsh->dsh_qef_targcount = 0;
    dsh->dsh_qef_count = 0;

    qef_rcb->qef_rowstat = 0;
    rowstat = 0;			/* default status */

    rowcount = 0;
    bufs_used = 0;
    pre_read = FALSE;
    skipread = FALSE;
    first = last = next = prior = absolute = relative = FALSE;

    /* Set up first "n"/offset "n" stuff. */
    if (action->qhd_obj.qhd_qep.ahd_firstncb >= 0)
    {
	firstncbp = (QEA_FIRSTN *)dsh->dsh_cbs[action->qhd_obj.qhd_qep.
								ahd_firstncb];
	rowlim = firstncbp->get_firstn;
	rowoffset = firstncbp->get_offsetn;

	/* Process offset if there is one. */
	if (rowoffset)
	{
	    /* Loop over readqep, skipping rows as we go. */
	    firstncbp->get_offsetn = 0;		/* force skip next time */
	    for ( ; rowoffset-- && status == E_DB_OK; )
	    {
		status = qea_fetch_readqep(action, qef_rcb, dsh, output, 
					function, &open_flag, TRUE, &gotarow);
		if (status != E_DB_OK &&
		    dsh->dsh_error.err_code == E_QE0015_NO_MORE_ROWS)
		    goto err_exit;
	    }
	}
    }

    /* Do setup if this is scrollable cursor. */
    if (scroll)
    {
	fetchlast = rowinspool = skipread = FALSE;
	sptr = (QEE_RSET_SPOOL *)dsh->dsh_cbs[qp->qp_rssplix];
	/* Determine target row. */
	switch(qef_rcb->qef_fetch_anchor){
	  case GCA_ANCHOR_BEGIN:
	    rowno = qef_rcb->qef_fetch_offset;
	    if (rowno == 1)
		first = TRUE;
	    else absolute = TRUE;
	    break;
	  case GCA_ANCHOR_END:
	    if (sptr->sp_flags & SP_ALLREAD)
	    {
		rowno = sptr->sp_rowcount+qef_rcb->qef_fetch_offset+1;
	    }
	    else 
	    {
		/* Flag need to FETCH LAST first. */
		fetchlast = TRUE;
		rowno = qef_rcb->qef_fetch_offset;
	    }
	    if (qef_rcb->qef_fetch_offset == -1)
		last = TRUE;
	    else absolute = TRUE;
	    break;
	  case GCA_ANCHOR_CURRENT:
	    rowno = sptr->sp_current+qef_rcb->qef_fetch_offset;
	    if (qef_rcb->qef_fetch_offset == 0)
	    {
		/* Setting currency. */
		if (buf_count <= 0)
		{
		    skipread = TRUE;
		}
	    }
	    if (qef_rcb->qef_fetch_offset == 1)
		next = TRUE;
	    else if (qef_rcb->qef_fetch_offset == -1)
		prior = TRUE;
	    else relative = TRUE;
	    break;
	}

	/* See if request is off one end or other of result set. */
	if (rowno <= 0 && !fetchlast)
	{
	    sptr->sp_current = 0;
	    skipread = TRUE;
	}
	else if ((sptr->sp_flags & SP_ALLREAD) && rowno > sptr->sp_rowcount)
	{
	    sptr->sp_current = sptr->sp_rowcount+1;
	    skipread = TRUE;
	}

	/* Skip error checking and positioning logic if this is a "before
	** first" or "after last" request. */
	if (!skipread)
	{
	    if (((!fetchlast || sptr->sp_flags & SP_ALLREAD) && rowno < 0) || 
		(rowno-1 > sptr->sp_rowcount && (sptr->sp_flags & SP_ALLREAD)) || 
		(fetchlast && rowno > 0))
	    {
		/* Row out of range of result set. */
		qef_error(E_QE00A6_NO_DATA, 0L, E_DB_ERROR,
		    &error, &dsh->dsh_error, 0);
		return(E_DB_WARN);
	    }

	    if (rowno != sptr->sp_current+1 ||
		(sptr->sp_current == 0 && sptr->sp_rowcount > 0))
	    {
		/* If fetch orientation isn't NEXT, we must position first. */
		status = qea_fetch_position(action, qef_rcb, dsh, output,
			function, &open_flag, rowno, fetchlast, &pre_read);
		if (status != E_DB_OK)
		{
		    if (dsh->dsh_error.err_code != E_QE0015_NO_MORE_ROWS)
			return(status);
		    skipread = TRUE;	/* 0 rows in scrollable cursor */
		}
	    }
	}

	/* If all rows have been read from query plan, or if desired
	** row has already been materialized, read from spool. */
	if ((sptr->sp_flags & SP_ALLREAD) || rowno <= sptr->sp_rowcount)
	    rowinspool = TRUE;
    }
    
    /* get the rows */
    while ( (bufs_used < buf_count && !skipread && rowstat == 0)
		|| rowtoss < 0)
    {
	/* Get next row (or chunk of row) from either query plan or
	** from scrollable cursor spool file. */
	if (pre_read)
	    goto fetch_bypass;		/* read one during position */

	gotarow = FALSE;
	if (scroll && rowinspool)
	{
	    status = qea_fetch_readtemp(action, qef_rcb, dsh, output, 
					&rowstat);
	    if (status != E_DB_OK || (rowstat != 0 && 
					rowstat != GCA_ROW_UPDATED))
		break;

	    /* See if next row is also in the spool file. */
	    if (!(sptr->sp_flags & SP_ALLREAD) && 
				++rowno >= sptr->sp_rowcount)
		rowinspool = FALSE;
	}
	else 
	{
	    /* Read row from query plan, then check scrollable stuff. */
	    status = qea_fetch_readqep(action, qef_rcb, dsh, output, 
					function, &open_flag, FALSE, &gotarow);
	    if (scroll && status != E_DB_OK &&
		dsh->dsh_error.err_code == E_QE0015_NO_MORE_ROWS)
	    {
		DB_STATUS	locstat;
		DMR_CB		*dmrcb;

		/* Finish up the scrolllable result set temp table (if
		** there's one more row to write). */
		if (gotarow)
		{
		    dmrcb = sptr->sp_wdmrcb;
		    locstat = dmf_call(DMR_PUT, dmrcb);
		    if (locstat != E_DB_OK)
		    {
			dsh->dsh_error.err_code = dmrcb->error.err_code;
			return(locstat);
		    }
		}
		sptr->sp_flags = SP_ALLREAD;

		if (qef_rcb->qef_fetch_anchor == GCA_ANCHOR_CURRENT &&
		    qef_rcb->qef_fetch_offset == 1)
		    sptr->sp_current = sptr->sp_rowcount+1;
					/* FETCH NEXT from last */
	    }
	}
			

	/* Analyze results of retrieval. */
	if (status != E_DB_OK)
	{
	    if (dsh->dsh_error.err_code == E_AD0002_INCOMPLETE)
	    {
		/* We're in mid-blob, address next buffer and stay in loop.
		** Increment buffer count, but not row count since we 
		** haven't finished this row yet. */
		bufs_used++;
		if (output)
		    output = output->dt_next;
		continue;
	    }
	    /* The rest just quit loop and return. */
	    if (!gotarow || dsh->dsh_error.err_code != E_QE0015_NO_MORE_ROWS)
		break;
	}

	/* If we get here, we've filled a buffer and completed a row. */
      fetch_bypass:
	pre_read = FALSE;
	if (rowtoss > 0)
	{
	    if (--rowtoss == 0)
		rowtoss = -1;		/* Start QE11 tossing */
	}
	if (rowtoss < 0)
	    continue;			/* If tossing, just loop */
	bufs_used++;
	rowcount++;
	if (status != E_DB_OK && 
			dsh->dsh_error.err_code == E_QE0015_NO_MORE_ROWS)
	    break;			/* exit when no more rows */

	if (action->qhd_obj.qhd_qep.ahd_current->qen_uoutput >= 0 && output)
	    output = output->dt_next;

	/* Stop if this is the topmost action and t is QEQP_SINGLETON, 
	** or it is a PROCEDURE and it isn't in a FOR loop,
	** or the select predicate is ANY or query was "select first n ..." 
	** and this was the "n"th. */

	if ((!scroll && rowlim && rowlim <= ++(firstncbp->get_count) ||
	 	((((qp_status & QEQP_SINGLETON) || (qmode == QEQP_EXEDBP &&
			!(action->qhd_obj.qhd_qep.ahd_qepflag & AHD_FORGET)))
		    && 
		    (action->ahd_flags & QEA_NEWSTMT)) 
		||
		action->qhd_obj.qhd_qep.ahd_qepflag & AHD_ANY)))
	{
	    dsh->dsh_error.err_code = E_QE0015_NO_MORE_ROWS;
	    status = E_DB_WARN;
	    if (firstncbp && rowlim < firstncbp->get_count)
		rowcount = 0;		/* force end of scan - for RPPs */
	    break;	    
	}
    }	/* end of buffer loop */

err_exit:
    if (status == E_DB_WARN && dsh->dsh_error.err_code ==
					E_QE0015_NO_MORE_ROWS)
    {
	/* There's really no such thing for a scrollable cursor. 
	** The rowstat communicates to the caller that we've reached
	** the end. */
	if (scroll)
	    status = E_DB_OK;
	else 
	{
	    rowstat |= GCA_ROW_AFTER;
	    qef_rcb->qef_curspos++;		/* indicate AFTER */
	}
    }
    dsh->dsh_qef_rowcount = dsh->dsh_qef_targcount = rowcount;
    dsh->dsh_qef_count = bufs_used;
    if (rowtoss > 1)
    {
	/* If we have more to output under QE11, store where we're at */
	dsh->dsh_trace_rows = rowtoss - 1;
    }

    /* Modify rowstat as needed. */
    if (scroll)
    {
	/* Set status & current row position. */
	if (sptr->sp_rowcount == 0)
	{
	    /* 0-row result set is a P.I.T.A. */
	    if (first || prior || ((absolute || relative) && rowno <= 0))
	    {
		sptr->sp_current = 0;
		rowstat |= GCA_ROW_BEFORE;
	    }
	    else if (last || next || ((absolute || relative) && rowno > 0))
	    {
		sptr->sp_current = 1;
		rowstat |= GCA_ROW_AFTER;
	    }
	}

	if (sptr->sp_current == 0)
	    rowstat |= GCA_ROW_BEFORE;
	else if (sptr->sp_current == 1 && sptr->sp_rowcount > 0)
	    rowstat |= GCA_ROW_FIRST;

	if (sptr->sp_flags & SP_ALLREAD && sptr->sp_rowcount > 0)
	{
	    if (sptr->sp_current == sptr->sp_rowcount)
		rowstat |= GCA_ROW_LAST;
	    else if (sptr->sp_current > sptr->sp_rowcount)
		rowstat |= GCA_ROW_AFTER;
	}

	qef_rcb->qef_curspos = sptr->sp_current;

	if (status == E_DB_WARN)
	    status = E_DB_OK;
    }
    if (!scroll && qef_rcb->qef_curspos == 1 && rowcount > 0)
	rowstat |= GCA_ROW_FIRST;
    qef_rcb->qef_rowstat = rowstat;
#ifdef xDEBUG
    (VOID) qe2_chk_qp(dsh);
#endif
    return (status);
}

/*{
** Name: QEA_FETCH_POSITION - establish position in result set of 
**	scrollable cursor.
**
** Description:
**      This routine establishes position in the result set (whether in the
**	spooled result set or the original query plan) according to the
**	orientation requested by the FETCH.
**
** Inputs:
**      action				fetch action item
**      dsh				Pointer to the data segment header
**	    .dsh_cbs			array of (among other things) DMS_CB
**					control blocks used for sequence
**					operators.
**	qef_rcb				Pointer to QEF request control block
**	position			Row number in result set before which
**					we must position.
**	fetchlast			TRUE if we must fetch last row first.
** Outputs:
**      dsh
**	    .dsh_error.err_code		one of the following
**				E_QE0000_OK
**				E_QE00A6_NO_DATA
**	Returns:
**	    E_DB_{OK, WARN, ERROR, FATAL}
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      25-jan-2007 (dougi)
**	    Written for scrollable cursors.
**	17-may-2007 (dougi)
**	    Generalize TID computation to support all pagesizes.
**	28-jan-2008 (dougi)
**	    Move ahd_rssplix to qp_rssplix.
**      19-Mar-2010 (horda03) B122979
**          Handle page spanning rows. The TID of these records
**          need a different algorithm to the standard row (i.e.
**          rows that fit on a page); especially as sptr->sp_rowspp
**          is 0 and an exception occurs.
*/

DB_STATUS qea_fetch_position(
QEF_AHD		*action,
QEF_RCB		*qef_rcb,
QEE_DSH		*dsh,
QEF_DATA	*output,
i4		function,
i4		*open_flag,
i4		position,
bool		fetchlast,
bool		*pre_read)

{
    QEF_CB		*qef_cb = dsh->dsh_qefcb;
    QEE_RSET_SPOOL	*sptr = (QEE_RSET_SPOOL *)dsh->dsh_cbs[dsh->
				dsh_qp_ptr->qp_rssplix];
    DMT_CB		*dmtcb = sptr->sp_dmtcb;
    DMR_CB		*dmrcb = sptr->sp_rdmrcb;
    DB_STATUS		status;
    i4			readcount;
    i4			temptid;
    bool		gotarow;


    /* If request is for row further down result set, materialize the
    ** intervening ones from query plan. */
    if (!(sptr->sp_flags & SP_ALLREAD) && (position > sptr->sp_rowcount ||
		fetchlast))
    {
	gotarow = FALSE;
	*pre_read = TRUE;
	readcount = (fetchlast) ? MAXI4 : position - sptr->sp_rowcount;
	do {
	    status = qea_fetch_readqep(action, qef_rcb, dsh, output, 
					function, open_flag, FALSE, &gotarow);
	    if (status != E_DB_OK && 
		dsh->dsh_error.err_code != E_QE0015_NO_MORE_ROWS)
		return(status);
	} while (status == E_DB_OK && (fetchlast || --readcount > 0));

	if (status != E_DB_OK && 
		dsh->dsh_error.err_code == E_QE0015_NO_MORE_ROWS)
	    sptr->sp_flags = SP_ALLREAD;

	if ((fetchlast && position >= -1) || readcount <= 0)
	{
	    if (fetchlast && position == -1 && sptr->sp_rowcount > 0)
		status = E_DB_OK;	/* really was "fetch last ..." */
	    return(status);
	}

	/* If "fetchlast" and position < -1, it was a "absolute -n"
	** request and we must re-read the row from the spool. */
	if (position < -1)
	    position += sptr->sp_rowcount+1;
	else
	{
	    /* Otherwise we were asked to read a row from outside
	    ** of the result set. That is a "no data" condition. */
	    dsh->dsh_error.err_code = E_QE00A6_NO_DATA;
	    return(E_DB_WARN);
	}
    }

    /* Request was for row that has already been read from query plan.
    ** Position in temp table and read it from there. */
    if (position > sptr->sp_rowcount || position < 0)
    {
	/* Again, we were asked to read a row beyond the result set. */
	dsh->dsh_error.err_code = E_QE00A6_NO_DATA;
	return(E_DB_WARN);
    }

    /* Compose target TID and read the row. For the prototype, we assume
    ** 16K pages. Later, the page sizes may differ, but we may also not
    ** have to worry about fmap/fhdr pages. */
    if (position <= sptr->sp_rowspp)
    {
	temptid = position-1;
    }
    else if (!sptr->sp_rowspp)
    {
       /* Page spanning row. So the normal rules don't apply. Here 1 row
       ** will use (scroll->ahd_rsrsize / scroll->ahd_rspsize) + 1) pages.
       ** 
       ** The TID of the first record is (DB_TIDEOF + 1), there after the
       ** TID can be calculated (taking into account the fmap pages). 
       */
       if (position == 1)
       {
          temptid = (DB_TIDEOF + 1);
       }
       else
       {
          QEF_SCROLL *scroll = action->qhd_obj.qhd_qep.ahd_scroll;

          temptid = (position-1) * ((scroll->ahd_rsrsize / scroll->ahd_rspsize) + 1) + 1;

          temptid += temptid/sptr->sp_fmapcount + 2;

          temptid *= (DB_TIDEOF + 1);
       }
    }
    else
    {
	/* Factor in fmap/fhdr pages, compute page no, shift it over
	** and add in line no. */
	temptid = (position-1) / sptr->sp_rowspp;
	temptid += temptid/sptr->sp_fmapcount + 2; /* factor in fhdr/fmaps */
	temptid *= (DB_TIDEOF + 1);		/* shift to page no location */
	temptid += (position-1) % sptr->sp_rowspp;
					/* add in line no. */
    }

    dmrcb->dmr_tid = temptid;
    dmrcb->dmr_flags_mask = 0;
    if (position == 1)
	dmrcb->dmr_position_type = DMR_ALL;
    else dmrcb->dmr_position_type = DMR_TID;
    dmrcb->dmr_access_id = dmtcb->dmt_record_access_id;
    dmrcb->dmr_data.data_address = output->dt_data;
    status = dmf_call(DMR_POSITION, dmrcb);

    if (status != E_DB_OK)
	dsh->dsh_error.err_code = dmrcb->error.err_code;
    else sptr->sp_current = position-1;	/* set to previous, because we'll
					** then "fetch next" */
    dmrcb->dmr_flags_mask = DMR_NEXT;
    *pre_read = FALSE;

    return(status);

}

/*{
** Name: QEA_FETCH_READTEMP - read a row from the result set spool file
**
** Description:
**      This routine reads the next row in the result set spool file. This
**	happens when a scrollable cursor moves backwards through an already
**	(partially) materialized result set.
**
** Inputs:
**      action				fetch action item
**      dsh				Pointer to the data segment header
**	    .dsh_cbs			array of (among other things) DMS_CB
**					control blocks used for sequence
**					operators.
** Outputs:
**      dsh
**	    .dsh_error.err_code		one of the following
**				E_QE0000_OK
**				E_QE00A7_ROW_DELETED
**	Returns:
**	    E_DB_{OK, WARN, ERROR, FATAL}
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      25-jan-2007 (dougi)
**	    Written for scrollable cursors.
**	5-apr-2007 (dougi)
**	    Add code for keyset cursors.
**	31-may-2007 (dougi)
**	    Return base table TID dmr_cb in qp_upd_cb for keyset cursors.
**	4-june-2007 (dougi)
**	    Fix annoying endian issues with TID copying.
**	18-july-2007 (dougi)
**	    Add logic for checksums on KEYSET rows.
**	1-oct-2007 (dougi)
**	    Fix buffer size for checksum operation & stick base row TID in
**	    sp_rowbuff for posn'd upd/del.
**	28-jan-2008 (dougi)
**	    Move ahd_rssplix to qp_rssplix.
**	2-oct-2008 (dougi)
**	    Return TIDs from updateable KEYSET cursors.
*/

DB_STATUS qea_fetch_readtemp(
QEF_AHD		*action,
QEF_RCB		*qef_rcb,
QEE_DSH		*dsh,
QEF_DATA	*output,
i4		*rowstat)

{
    QEF_CB		*qef_cb = dsh->dsh_qefcb;
    QEE_RSET_SPOOL	*sptr = (QEE_RSET_SPOOL *)dsh->dsh_cbs[dsh->
				dsh_qp_ptr->qp_rssplix];
    DMR_CB		*dmrcb = sptr->sp_rdmrcb;
    DB_STATUS		status;
    i4			error;


    /* Not too much to do here. Just set up DMF DMR_NEXT call and
    ** do it. */
    if (sptr->sp_current >= sptr->sp_rowcount)
    {
	/* End-of-file. */
	status = E_DB_WARN;
	dsh->dsh_error.err_code = E_QE0015_NO_MORE_ROWS;
	return(status);
    }

    /* dmrcb->dmr_access_id = dmtcb->dmt_record_access_id; */
    dmrcb->dmr_flags_mask = DMR_NEXT;
    dmrcb->dmr_position_type = DMR_ALL;
    dmrcb->dmr_data.data_address = output->dt_data;
    status = dmf_call(DMR_GET, dmrcb);
    if (status != E_DB_OK)
	return;

    /* Check for KEYSET cursor and perform 2nd step of reading base table
    ** by TID, verifying it to be same row and projecting result row. */
    if (action->qhd_obj.qhd_qep.ahd_qepflag & AHD_KEYSET)
    {
	DB_TID8	bigtid;
	DMR_CB	*tiddmr, *origdmr;
	char	*intid, *outtid, *to;
	QEF_SCROLL *scrollp = action->qhd_obj.qhd_qep.ahd_scroll;
	char	*btrowp = dsh->dsh_row[scrollp->ahd_rsbtrow];
	QEN_ADF	*baseproj = scrollp->ahd_rskcurr;
	ADF_CB	*adf_cb = dsh->dsh_adf_cb;
	ADE_EXCB *baseexcb = (ADE_EXCB *)dsh->dsh_cbs[baseproj->qen_pos];
	i4	result, oldcsum, checksum;
	DB_DATA_VALUE	rowdv, checkdv;
	bool	nomatch = FALSE;

	/* Extract TID from keyset row just read. */
	outtid = (PTR)&bigtid;
	intid = output->dt_data;
	I8ASSIGN_MACRO(*intid, bigtid);
	I8ASSIGN_MACRO(*intid, *(sptr->sp_rowbuff)); /* save for posn'd ops */
	intid += DB_TID8_LENGTH;		/* skip to checksum */
	I4ASSIGN_MACRO(*intid, oldcsum);
	
	/* If this is a partitioned table, extra logic goes here to 
	** locate the correct DMR_CB to clone. */

	/* Non-partitioned table. */
	origdmr = (DMR_CB *)dsh->dsh_cbs[action->qhd_obj.qhd_qep.ahd_odmr_cb];
					/* get base table DMR_CB to clone */
	tiddmr = (DMR_CB *)dsh->dsh_cbs[action->qhd_obj.qhd_qep.ahd_scroll->
							ahd_rsbtdmrix];
					/* target DMR ptr */
	MEcopy((PTR)origdmr, sizeof(DMR_CB), (PTR)tiddmr);
	tiddmr->dmr_flags_mask = DMR_BY_TID;
	tiddmr->dmr_tid = bigtid.tid_i4.tid;
	tiddmr->dmr_data.data_address = btrowp;
	dsh->dsh_cbs[dsh->dsh_qp_ptr->qp_upd_cb] = (PTR)tiddmr;
					/* save addr for pos'd upd/del */

	status = dmf_call(DMR_GET, tiddmr);
	if (status != E_DB_OK)
	{
	    /* If row not found, return "no match". Otherwise, 
	    ** something's broke. */
	    if (tiddmr->error.err_code == E_DM003C_BAD_TID ||
		tiddmr->error.err_code == E_DM0044_DELETED_TID)
	    {
		/* qef_error(E_QE00A7_ROW_DELETED, 0L, E_DB_WARN,
		    &error, &dsh->dsh_error, 0); */
		*rowstat = GCA_ROW_DELETED;
		sptr->sp_current++;
		return(E_DB_OK);
	    }
	    dsh->dsh_error.err_code = tiddmr->error.err_code;
	    return(status);
	}

	/* Got a base table row - compare remaining keyset columns
	** to verify that it is the same row. */
	status = qea_fetch_keycomp(dsh, scrollp, output->dt_data, 
				btrowp, &result);
	if (status != E_DB_OK)
	    return(status);

	nomatch = (result != 0);

	if (nomatch)
	{
	    /* Row has been changed and is no longer considered part
	    ** of the result set. Return appropriate conditions. */
	    /* qef_error(E_QE00A7_ROW_DELETED, 0L, E_DB_WARN,
		    &error, &dsh->dsh_error, 0): */
	    tiddmr->dmr_tid = -1;		/* prevent pos'd upd/del */
	    *rowstat = GCA_ROW_DELETED;
	    sptr->sp_current++;
	    return(E_DB_OK);
	}

	/* Run CX to re-materialize real result set row. */
	if (dsh->dsh_qp_ptr->qp_status & QEQP_GLOBAL_BASEARRAY)
	    dsh->dsh_row[baseproj->qen_uoutput] = output->dt_data;
	else baseexcb->excb_bases[ADE_ZBASE + baseproj->qen_uoutput] =
			output->dt_data;	/* set result row addr */

	baseexcb->excb_seg = ADE_SMAIN;
	status = ade_execute_cx(adf_cb, baseexcb);
	if (status != E_DB_OK)
	{
	    status = qef_adf_error(&adf_cb->adf_errcb, status, qef_cb, 
					&dsh->dsh_error);
	}

	/* Stick TID where positioned update/delete expect to see it. */
	if (scrollp->ahd_tidrow > 0)
	{
	    to = (PTR)(dsh->dsh_row[action->qhd_obj.qhd_qep.ahd_tidrow]
				+ action->qhd_obj.qhd_qep.ahd_tidoffset);
	    I8ASSIGN_MACRO(bigtid, *to);
	}

	/* Re-compute checksum so we can flag updated rows. */
	checkdv.db_datatype = DB_INT_TYPE;
	checkdv.db_length = 4;
	checkdv.db_data = (PTR)&checksum;
	rowdv.db_datatype = DB_BYTE_TYPE;
	rowdv.db_length = dsh->dsh_qp_ptr->qp_row_len[scrollp->ahd_rsbtrow];
	rowdv.db_data = output->dt_data;

	status = adu_checksum(adf_cb, &rowdv, &checkdv);
	if (status != E_DB_OK)
	    return(status);

	/* If new checksum is different from old, row has been updated. */
	if (oldcsum != checksum)
	    *rowstat |= GCA_ROW_UPDATED;
    }	/* end of KEYSET logic */

    if (status == E_DB_OK)
	sptr->sp_current++;
    return(status);

}

/*{
** Name: QEA_FETCH_KEYCOMP - compare keyset values with base table row
**
** Description:
**      This routine compares the keyset values in the current spool file
**	row with those in the corresponding base table row. If they match,
**	the base table row is projected to the result set.
**
** Inputs:
**      dsh				Pointer to the data segment header
**	    .dsh_cbs			array of (among other things) DMS_CB
**					control blocks used for sequence
**					operators.
**	scrollp				Ptr to scrollable cursor descriptor
**	buf1p				Ptr to keyset buffer
**	buf2p				Ptr to base table row buffer
** Outputs:
**      dsh
**	    .dsh_error.err_code		one of the following
**				E_QE0000_OK
**				E_QE0017_BAD_CB
**	result				Ptr to compare result (0 if equal)
**	Returns:
**	    E_DB_{OK, WARN, ERROR, FATAL}
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      9-apr-2007 (dougi)
**	    Written to handle keyset scrollable cursors.
*/

static DB_STATUS qea_fetch_keycomp(
QEE_DSH		*dsh,
QEF_SCROLL	*scrollp,
PTR		buf1p,
PTR		buf2p,
i4		*result)

{
    DB_DATA_VALUE adcval1, adcval2;
    ADF_CB	*adf_cb = dsh->dsh_adf_cb;
    DMF_ATTR_ENTRY *attp = scrollp->ahd_rskattr;
    DB_STATUS	status;
    i4		i, off1, *off2;
    i8		bigbuf[250];		/* 2000 byte aligned buffer */


    /* Simply loop over the keyset descriptors, comparing keyset value
    ** with corresponding base table value. 
    **
    ** NOTE: skip TID & checksum at front of keyset row buffer. */

    for (i = 0, off1 = DB_TID8_LENGTH + sizeof(i4), 
		off2 = scrollp->ahd_rskoff2; 
		    i < scrollp->ahd_rskscnt; i++, off2++, attp++)
    {
	/* Set up adcval1/2 and issue call to adc. */
	adcval1.db_datatype = adcval2.db_datatype = attp->attr_type;
	adcval1.db_length   = adcval2.db_length   = attp->attr_size;
	adcval1.db_prec     = adcval2.db_prec     = attp->attr_precision;
	adcval1.db_collID   = adcval2.db_collID   = attp->attr_collID;
	adcval1.db_data = buf1p + off1;
	adcval2.db_data = buf2p + *off2;
	if (DB_ALIGN_MACRO(*off2) != *off2)
	{
	    /* Comparand isn't aligned - copy to aligned buffer. */
	    MEcopy(adcval2.db_data, (adcval2.db_length > 2000) ? 2000 :
			adcval2.db_length, (char *)&bigbuf[0]);
	    adcval2.db_data = (char *)&bigbuf[0];
	}

	status = adc_compare(adf_cb, &adcval1, &adcval2, result);
	if (status != E_DB_OK)
	{
	    status = qef_adf_error(&adf_cb->adf_errcb, status,
		dsh->dsh_qefcb, &dsh->dsh_error);
	    if (status != E_DB_OK)
		return(status);
	}
	if (*result)
	    break;		/* leave loop if not equal */

	off1 += attp->attr_size;
    }

    return(E_DB_OK);
}

/*{
** Name: QEA_FETCH_READQEP - read a row from the query plan
**
** Description:
**      This routine materializes the next result row using ahd_qep (if there),
**	then projects it into the result row buffer. This code has been 
**	extracted from the main function to separate it from the buffer
**	loading logic to make things easier to implement scrollable cursors.
**
** Inputs:
**      action				fetch action item
**      dsh				Pointer to the data segment header
**	    .dsh_cbs			array of (among other things) DMS_CB
**					control blocks used for sequence
**					operators.
** Outputs:
**      dsh
**	    .dsh_error.err_code		one of the following
**				E_QE0000_OK
**				E_QE0017_BAD_CB
**	Returns:
**	    E_DB_{OK, WARN, ERROR, FATAL}
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      25-jan-2007 (dougi)
**	    Cloned from main function for scrollable cursors.
**	31-may-2007 (dougi)
**	    Return action's ahd_odmr_cb in qp_upd_cb for keyset cursors.
**	19-sep-2007 (dougi)
**	    Tiny change to fix problem with scrollable constant cursors.
**	28-jan-2008 (dougi)
**	    Move ahd_rssplix to qp_rssplix.
**	16-Nov-2009 (thaju02) B122848
**	    Added doing_offset param.
**	    If doing_offset, do not write tuple to result set spool file.
*/

DB_STATUS qea_fetch_readqep(
QEF_AHD		*action,
QEF_RCB		*qef_rcb,
QEE_DSH		*dsh,
QEF_DATA	*output,
i4		function,
i4		*open_flag,
bool		doing_offset,
bool		*gotarow)

{
    QEN_NODE	*node = action->qhd_obj.qhd_qep.ahd_qep;
    QEN_ADF	*qen_adf = action->qhd_obj.qhd_qep.ahd_current;
    ADE_EXCB	*ade_excb = (ADE_EXCB *)dsh->dsh_cbs[qen_adf->qen_pos];
    ADF_CB	*adf_cb = dsh->dsh_adf_cb;
    QEF_CB	*qef_cb = dsh->dsh_qefcb;
    QEE_RSET_SPOOL *rsptr;
    i4		qp_status = dsh->dsh_qp_ptr->qp_status;
    i4		rowlim = 0;
    DB_STATUS	status = E_DB_OK;
    bool	scroll;
    bool	keyset;

    if (action->qhd_obj.qhd_qep.ahd_qepflag & AHD_SCROLL)
	scroll = TRUE;
    else scroll = FALSE;
    if (action->qhd_obj.qhd_qep.ahd_qepflag & AHD_KEYSET)
	keyset = TRUE;
    else keyset = FALSE;

    /* Set up first "n"/offset "n" stuff. */
    if (action->qhd_obj.qhd_qep.ahd_firstncb >= 0)
    {
	rowlim = ((QEA_FIRSTN *)dsh->dsh_cbs[action->qhd_obj.qhd_qep.
						ahd_firstncb])->get_firstn;
    }

    if (scroll)
    {
	rsptr = (QEE_RSET_SPOOL *)dsh->dsh_cbs[dsh->dsh_qp_ptr->qp_rssplix];
	if (rowlim && rsptr->sp_rowcount >= rowlim)
	{
	    /* If "first n" and we've read the n'th row in a 
	    ** scrollable cursor, we've reached the end. */
	    dsh->dsh_error.err_code = E_QE0015_NO_MORE_ROWS;
	    rsptr->sp_flags |= SP_ALLREAD;
	    return(E_DB_WARN);
	}
    }
    else rsptr = (QEE_RSET_SPOOL *) NULL;

    *gotarow = FALSE;

    if (node)
    {
	ade_excb->excb_seg = ADE_SMAIN;

	/*
	** If this is not a blob continuation, then get rows from below.
	** If, however, this is a continuation, then skip getting rows from
	** below, and simply move on to recall ade_execute_cx().
	**
	** NOTE: scrollable cursors do not permit blobs, so rows from
	** scrollable cursors will always execute the following code.
	** The "scroll" test is superfluous, but serves to emphasize
	** that blob processing isn't an issue for them.
	*/
	if (!ade_excb->excb_continuation || scroll)
	{
	    /* fetch rows (if there are some to be fetched) */
	    if (action->qhd_obj.qhd_qep.ahd_qepflag & AHD_NOROWS)
	    {
		status = E_DB_WARN;
		dsh->dsh_error.err_code = E_QE0015_NO_MORE_ROWS;
	    }
	     else status = (*node->qen_func)(node, qef_rcb, dsh, *open_flag);
	    *open_flag = function &= ~(TOP_OPEN | FUNC_RESET);

	    /*
	    ** If the qen_node returned no rows and we are supposed to
	    ** invalidate this query plan if this action doesn't get
	    ** any rows, then invalidate the plan.
	    */
	    if (dsh->dsh_error.err_code == E_QE0015_NO_MORE_ROWS &&
		dsh->dsh_xaddrs[node->qen_num]->qex_status->node_rcount == 0 &&
			(action->ahd_flags & QEA_ZEROTUP_CHK) != 0
		)
	    {
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
	    }

	    if (keyset && action->qhd_obj.qhd_qep.ahd_odmr_cb >= 0)
		dsh->dsh_cbs[dsh->dsh_qp_ptr->qp_upd_cb] =
		    dsh->dsh_cbs[action->qhd_obj.qhd_qep.ahd_odmr_cb];

	    if (status != E_DB_OK)
	    {
		/* 
		** the error.err_code field in qef_rcb should already be
		** filled in.
		*/
		return(status);
	    }

	    /* Having fetched a fresh row, check for sequences to update. */
	    if (action->qhd_obj.qhd_qep.ahd_seq != NULL)
	    {
		status = qea_seqop(action, dsh);
		if (status != E_DB_OK)
		    return(status);
	    }
	}

	/* Project row into output buffer. Aim output row (uoutput) 
	** at caller supplied output buffer. If there are blobs, tell 
	** adf that it's the blob area.
	*/
	ade_excb->excb_limited_base = ADE_NOBASE;
	if (qen_adf->qen_uoutput >= 0)
	{
	    if (qen_adf->qen_mask & QEN_HAS_PERIPH_OPND)
		ade_excb->excb_limited_base = qen_adf->qen_uoutput;
	    if (qp_status & QEQP_GLOBAL_BASEARRAY)
	    {
		dsh->dsh_row[qen_adf->qen_uoutput] = output->dt_data;
	    }
	    else
	    {
		/* old-fashioned way, uoutput is local base, bias it
		** for blobs, set output pointer where old way wants it
		*/
		if (qen_adf->qen_mask & QEN_HAS_PERIPH_OPND)
			ade_excb->excb_limited_base += ADE_ZBASE;
		ade_excb->excb_bases[ADE_ZBASE+qen_adf->qen_uoutput] =
							    output->dt_data;
	    }
	    ade_excb->excb_size = output->dt_size;
	}
	    
	/* process the tuples */
	status = ade_execute_cx(adf_cb, ade_excb);
	if (status != E_DB_OK)
	{
#ifdef xDEBUG
	    (VOID) qe2_chk_qp(dsh);
#endif
	    if ((status == E_DB_INFO) &&
			(adf_cb->adf_errcb.ad_errcode == E_AD0002_INCOMPLETE))
	    {
		dsh->dsh_error.err_code = E_AD0002_INCOMPLETE;
		if (qen_adf->qen_uoutput >= 0)
		{
		    output->dt_size = ade_excb->excb_size;
		    ade_excb->excb_limited_base = ADE_NOBASE;
		}
		return(status);
		/*
		** At this point, we've filled in part of a tuple, but not
		** all of it.  Return to main loop to permit continued
		** filling of buffers.
		*/
	    }
	    else if ((status = qef_adf_error(&adf_cb->adf_errcb,
			status, qef_cb, &dsh->dsh_error)) != E_DB_OK)
	    {
		return(status);
	    }
	}

	*gotarow = TRUE;
	if (!scroll)
	    qef_rcb->qef_curspos++;	/* increment non-scroll curs */

	if (qen_adf->qen_uoutput >= 0)
	{
	    output->dt_size = ade_excb->excb_size;
	    ade_excb->excb_limited_base = ADE_NOBASE;
	}

	if (scroll && !doing_offset)
	{
	    status = qea_fetch_writetemp(action, qef_rcb, dsh, output);
	    if (status == E_DB_OK)
		rsptr->sp_current = rsptr->sp_rowcount;
	}
    }	/* end of "if (node)" */
    else
    {
	/* Constant query - no data to read, just execute ahd_current. */
	if (ade_excb->excb_seg == ADE_SFINIT)
	{
	    /* We've already projected the constant row. */
	    dsh->dsh_error.err_code = E_QE0015_NO_MORE_ROWS;
	    ade_excb->excb_seg = ADE_SMAIN;
	    return(E_DB_WARN);
	}
	else
	{
	    /* materialize row into output buffer, same setup as above */
	    ade_excb->excb_seg = ADE_SMAIN;
	    ade_excb->excb_limited_base = ADE_NOBASE;
	    if (qen_adf->qen_uoutput >= 0)
	    {
		if (qen_adf->qen_mask & QEN_HAS_PERIPH_OPND)
			ade_excb->excb_limited_base = qen_adf->qen_uoutput;
		if (qp_status & QEQP_GLOBAL_BASEARRAY)
		{
		    dsh->dsh_row[qen_adf->qen_uoutput] = output->dt_data;
		}
		else
		{
		    /* old-fashioned way, uoutput is local base, bias it
		    ** for blobs, set output pointer where old way wants it
		    */
		    if (qen_adf->qen_mask & QEN_HAS_PERIPH_OPND)
			    ade_excb->excb_limited_base += ADE_ZBASE;
		    ade_excb->excb_bases[ADE_ZBASE+qen_adf->qen_uoutput] =
							    output->dt_data;
		}
		ade_excb->excb_size = output->dt_size;
	    }

	    /* Before running current CX, check for sequences to update. */
	    if (action->qhd_obj.qhd_qep.ahd_seq != NULL)
	    {
		status = qea_seqop(action, dsh);
		if (status != E_DB_OK)
		    return(status);
	    }

	    /* process the tuples */

	    status = ade_execute_cx(adf_cb, ade_excb);
	    if (status != E_DB_OK)
	    {
#ifdef xDEBUG
		(VOID) qe2_chk_qp(dsh);
#endif
		if ((status == E_DB_INFO) &&
		    adf_cb->adf_errcb.ad_errcode == E_AD0002_INCOMPLETE)
		{
		    dsh->dsh_error.err_code = E_AD0002_INCOMPLETE;
		    if (qen_adf->qen_uoutput >= 0)
		    {
			output->dt_size = ade_excb->excb_size;
			ade_excb->excb_limited_base = ADE_NOBASE;
		    }
		    return(status);
		}
		else if ((status = qef_adf_error(
				    &adf_cb->adf_errcb, status,
				    qef_cb, &dsh->dsh_error)
				    ) != E_DB_OK)
		{
		    return(status);
		}
	    }

	    *gotarow = TRUE;
	    qef_rcb->qef_curspos = 1;

	    /* indicate end of file */
	    if (!(dsh->dsh_qp_status & DSH_CURSOR_OPEN))
						    /* if not cursor */
	    {
		/* eof will be returned with rowcount = 1 */
		dsh->dsh_error.err_code = E_QE0015_NO_MORE_ROWS;
		ade_excb->excb_seg = ADE_SMAIN;
		status = E_DB_WARN;
	    }
	    else
	    {
		/* set up to allow return for eof */
		ade_excb->excb_seg = ADE_SFINIT;
	    }
	    if (qen_adf->qen_uoutput >= 0)
	    {
		output->dt_size = ade_excb->excb_size;
		    ade_excb->excb_limited_base = ADE_NOBASE;
	    }
	}
	if (scroll && !doing_offset)
	{
	    status = qea_fetch_writetemp(action, qef_rcb, dsh, output);
	    if (status == E_DB_OK)
		rsptr->sp_current = 1;
	}
    }

    return(status);
}

/*{
** Name: QEA_FETCH_WRITETEMP - write a row to the result set spool file
**
** Description:
**      This routine writes a row to the end of the result set spool file.
**	This happens after each row is materialized from the original query
**	plan for a scrollable cursor in the event that a FETCH attempts
**	to read a row already materialized.
**
** Inputs:
**      action				fetch action item
**      dsh				Pointer to the data segment header
**	    .dsh_cbs			array of (among other things) DMS_CB
**					control blocks used for sequence
**					operators.
** Outputs:
**      dsh
**	    .dsh_error.err_code		one of the following
**				E_QE0000_OK
**				E_QE0017_BAD_CB
**	Returns:
**	    E_DB_{OK, WARN, ERROR, FATAL}
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      25-jan-2007 (dougi)
**	    Written for scrollable cursors.
**	4-apr-2007 (dougi)
**	    Add KEYSET support.
**	17-may-2007 (dougi)
**	    Add multiple temp table pagesize support.
**	4-june-2007 (dougi)
**	    Fix annoying endian issues with TID copying.
**	18-july-2007 (dougi)
**	    Add logic for checksums on KEYSET rows.
**	7-aug-2007 (dougi)
**	    Fix pagesize/fmap settings.
**	1-oct-2007 (dougi)
**	    Fix buffer size for checksum operation.
**	15-oct-2007 (dougi)
**	    Arghh! Rows per temp table page cannot exceed 511 because of 
**	    9-bit lineid.
**	28-jan-2008 (dougi)
**	    Move ahd_rssplix to qp_rssplix.
**	8-Jun-2009 (kibro01) b122171
**	    Use multiple columns in temp table if result size is greater than
**	    adu_maxstring.
*/

DB_STATUS qea_fetch_writetemp(
QEF_AHD		*action,
QEF_RCB		*qef_rcb,
QEE_DSH		*dsh,
QEF_DATA	*output)

{
    QEF_CB		*qef_cb = dsh->dsh_qefcb;
    QEE_RSET_SPOOL	*sptr = (QEE_RSET_SPOOL *)dsh->dsh_cbs[dsh->
				dsh_qp_ptr->qp_rssplix];
    DMT_CB		*dmtcb = sptr->sp_dmtcb;
    DMR_CB		*dmrcb = sptr->sp_wdmrcb;
    DMT_CHAR_ENTRY	char_array[2];
    DB_STATUS		status;


    /* See if the temp table needs to be created/opened first. */
    if (sptr->sp_rowcount == 0)
    {
        DMT_SHW_CB  	dmt_show;
        DMT_TBL_ENTRY   dmt_tbl_entry;
	i4		psize;

	/* Initialize the DMT_CB for the spool table */
	dmtcb->dmt_flags_mask = DMT_DBMS_REQUEST;
	dmtcb->dmt_db_id = dsh->dsh_qefcb->qef_rcb->qef_db_id;
	dmtcb->dmt_tran_id = dsh->dsh_dmt_id;
	MEmove(8, (PTR) "$default", (char) ' ', 
	    sizeof(DB_LOC_NAME),
	    (PTR) dmtcb->dmt_location.data_address);
	dmtcb->dmt_location.data_in_size = sizeof(DB_LOC_NAME);


	/* Initialize table attribute descriptors. */
	dmtcb->dmt_attr_array.ptr_address = 
		(PTR)action->qhd_obj.qhd_qep.ahd_scroll->ahd_rsattr;
	dmtcb->dmt_attr_array.ptr_in_count = 
		action->qhd_obj.qhd_qep.ahd_scroll->ahd_rsnattr;
	dmtcb->dmt_attr_array.ptr_size = sizeof (DMF_ATTR_ENTRY);

	/* No sorting of these guys. */
	dmtcb->dmt_key_array.ptr_address = (PTR) NULL;
	dmtcb->dmt_key_array.ptr_in_count = 0;
	dmtcb->dmt_key_array.ptr_size = sizeof (DMT_KEY_ENTRY);

	/*  Pass the page size. */
	char_array[0].char_id = DMT_C_PAGE_SIZE;
	char_array[0].char_value = psize = 
			action->qhd_obj.qhd_qep.ahd_scroll->ahd_rspsize;
	char_array[1].char_id = DMT_C_DUPLICATES;
	char_array[1].char_value = DMT_C_ON; /* duplicate rows allowed */
	dmtcb->dmt_char_array.data_address = (PTR) &char_array;
	dmtcb->dmt_char_array.data_in_size = 2 * sizeof(DMT_CHAR_ENTRY);

	/* Set FMAP page interval - these numbers were obtained from
	** a DMF design doc. If the space management scheme changes,
	** this logic may be rendered faulty. */
	if (psize <= 2048)
	    sptr->sp_fmapcount = 16000;
	else if (psize <= 4096)
	    sptr->sp_fmapcount = 32160;
	else if (psize <= 8192)
	    sptr->sp_fmapcount = 64928;
	else if (psize <= 16384)
	    sptr->sp_fmapcount = 130464;
	else if (psize <= 32768)
	    sptr->sp_fmapcount = 261536;
	else sptr->sp_fmapcount = 523680;

	/* Create the spool table. */
	status = dmf_call(DMT_CREATE_TEMP, dmtcb);
	if (status != E_DB_OK)
	{
	    if (dmtcb->error.err_code == E_DM0078_TABLE_EXISTS)
	    {
		dsh->dsh_error.err_code = E_QE0050_TEMP_TABLE_EXISTS;
		status = E_DB_ERROR;
	    }
	    else
	    {
		dsh->dsh_error.err_code = dmtcb->error.err_code;
	    }
	    return(status);
	}

	/* Open the spool table. */
	dmtcb->dmt_flags_mask = 0;
	dmtcb->dmt_sequence = dsh->dsh_stmt_no;
	dmtcb->dmt_access_mode = DMT_A_WRITE;
	dmtcb->dmt_lock_mode = DMT_X;
	dmtcb->dmt_update_mode = DMT_U_DIRECT;
	dmtcb->dmt_char_array.data_address = 0;
	dmtcb->dmt_char_array.data_in_size = 0;
	
	status = dmf_call(DMT_OPEN, dmtcb);
	if (status != E_DB_OK)
	{
	    dsh->dsh_error.err_code = dmtcb->error.err_code;
	    return(status);
	}
	/* Move the control block into a list of open table in the
	** data segment header (DSH).
	*/
	if (dsh->dsh_open_dmtcbs == (DMT_CB *)NULL)
	{
	    dmtcb->q_next = dmtcb->q_prev = (PTR)NULL;
	    dsh->dsh_open_dmtcbs = dmtcb;	    
	}
	else
	{
	    dsh->dsh_open_dmtcbs->q_prev = (PTR)dmtcb;
	    dmtcb->q_next = (PTR)(dsh->dsh_open_dmtcbs);
	    dmtcb->q_prev = (PTR)NULL;
	    dsh->dsh_open_dmtcbs = dmtcb;
	}

	/* Do DMT_SHOW to pick up no. of rows per page. */
	dmt_show.type = DMT_SH_CB;
	dmt_show.length = sizeof(DMT_SHW_CB);
	dmt_show.dmt_session_id = qef_cb->qef_ses_id;
	dmt_show.dmt_db_id = qef_rcb->qef_db_id;
	dmt_show.dmt_tab_id.db_tab_base = 0;
	dmt_show.dmt_flags_mask = DMT_M_TABLE | DMT_M_ACCESS_ID;
	dmt_show.dmt_char_array.data_address = NULL;
	dmt_show.dmt_table.data_address = (PTR) &dmt_tbl_entry;
	dmt_show.dmt_table.data_in_size = sizeof(DMT_TBL_ENTRY);
	dmt_show.dmt_record_access_id = dmtcb->dmt_record_access_id;
	status = dmf_call(DMT_SHOW, &dmt_show);
	if (status != E_DB_OK)
	{
	    dsh->dsh_error.err_code = dmt_show.error.err_code;
	    return(status);
	}
	sptr->sp_rowspp = dmt_tbl_entry.tbl_tperpage;
	if (sptr->sp_rowspp > DB_TIDEOF)
	    sptr->sp_rowspp = DB_TIDEOF;	/* max of DB_TIDEOF rows (line
						** ID capacity) */

	dmrcb->dmr_flags_mask = DMR_NOSORT;	/* prep for 1st row */
    }

    /* Finish prepping the DMR_CB & issue LOAD call. */
    dmrcb->dmr_access_id = dmtcb->dmt_record_access_id;
    dmrcb->dmr_count = 1;
    dmrcb->dmr_data.data_address = output->dt_data;

    /* Check for KEYSET cursor and only copy KEYSET columns to temp. */
    if (action->qhd_obj.qhd_qep.ahd_qepflag & AHD_KEYSET)
    {
	ADF_CB	*adf_cb = dsh->dsh_adf_cb;
	DMF_ATTR_ENTRY	*attp;
	QEF_SCROLL *scrollp = action->qhd_obj.qhd_qep.ahd_scroll;
	i4	*off1, checksum;
	i4	i, off2;
	DB_TID8	bigtid;
	PTR	from, to;
	DB_DATA_VALUE	checkdv, rowdv;

	/* TID is either in special buffer or ORIG node's DMR. */
	bigtid.tid_i4.tpf = 0;		/* clear partition stuff */
	if (scrollp->ahd_tidrow > 0)
	{
	    from = (PTR)(dsh->dsh_row[scrollp->ahd_tidrow]
					+ scrollp->ahd_tidoffset);
	    to = (PTR)(dsh->dsh_row[action->qhd_obj.qhd_qep.ahd_tidrow]
				+ action->qhd_obj.qhd_qep.ahd_tidoffset);
	    I8ASSIGN_MACRO(*from, bigtid);
	    I8ASSIGN_MACRO(*from, *to);		/* where posnd ops expect it */
	}
	else 
	{
	    DMR_CB	*origdmr;

	    origdmr = (DMR_CB *)dsh->dsh_cbs[action->qhd_obj.qhd_qep.
								ahd_odmr_cb];
	    bigtid.tid_i4.tid = origdmr->dmr_tid;
	}

	/* Build 8 byte TID. */
	I8ASSIGN_MACRO(bigtid, *(sptr->sp_rowbuff));
					/* copy TID to head of row */
	attp = action->qhd_obj.qhd_qep.ahd_scroll->ahd_rskattr;
	off1 = action->qhd_obj.qhd_qep.ahd_scroll->ahd_rskoff1;
	off2 = DB_TID8_LENGTH+sizeof(i4);

	for (i = 0; i < action->qhd_obj.qhd_qep.ahd_scroll->ahd_rskscnt; 
			i++, attp++, off1++)
	{
	    from = (PTR)(output->dt_data + *off1);
	    to = (PTR)(sptr->sp_rowbuff + off2);
	    MEcopy(from, attp->attr_size, to);
	    off2 += attp->attr_size;
	}
	dmrcb->dmr_data.data_address = sptr->sp_rowbuff;

	/* Set up and call checksum function. */
	checkdv.db_datatype = DB_INT_TYPE;
	checkdv.db_length = 4;
	checkdv.db_data = (PTR)&checksum;
	rowdv.db_datatype = DB_BYTE_TYPE;
	rowdv.db_length = dsh->dsh_qp_ptr->qp_row_len[scrollp->ahd_rsbtrow];
	rowdv.db_data = output->dt_data;

	status = adu_checksum(adf_cb, &rowdv, &checkdv);
	if (status != E_DB_OK)
	    return(status);
	MEcopy((PTR)&checksum, sizeof(i4), 
			(PTR)(sptr->sp_rowbuff + DB_TID8_LENGTH));
					/* copy checksum into buffer */
    }

    /* On with the show. */
    status = dmf_call(DMR_PUT, dmrcb);
    if (status != E_DB_OK)
    {
	dsh->dsh_error.err_code = dmrcb->error.err_code;
	return(status);
    }

    sptr->sp_rowcount++;
    return(E_DB_OK);
}

/*{
** Name: QEA_SEQOP	- perform sequence operators
**
** Description:
**      This routine loops over the sequences referenced by the current
**	action header and performs the required operations (next and/or current).
**
** Inputs:
**      action				fetch action item
**      dsh				Rpointer to the data segment header
**	    .dsh_cbs			array of (among other things) DMS_CB
**					control blocks used for sequence
**					operators.
** Outputs:
**      dsh
**	    .dsh_error.err_code		one of the following
**				E_QE0000_OK
**				E_QE0017_BAD_CB
**	Returns:
**	    E_DB_{OK, WARN, ERROR, FATAL}
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      18-mar-02 (inkdo01)
**	    Written for sequence support.
**	28-apr-03 (inkdo01)
**	    Fix error call for "end-of-sequence".
**	27-Feb-2005 (schka24)
**	    First next on an invalid sequence can error out here if
**	    repeated query or concurrent DDL before sequence is opened;
**	    deal with the error here.
*/
DB_STATUS
qea_seqop(
QEF_AHD		*action,
QEE_DSH		*dsh)
{
    DB_STATUS	status;
    DMS_CB	*dms_cb;
    DMS_SEQ_ENTRY *dms_seq;
    QEF_SEQ_LINK *slink;
    QEF_SEQUENCE *qseqp;
    i4		error;

    /* Simply loop over LINKs associated with current header and issue
    ** corresponding calls to DMF. */

    status = E_DB_OK;
    for (slink = action->qhd_obj.qhd_qep.ahd_seq; slink != NULL;
				slink = slink->qs_ahdnext)
    {
	qseqp = slink->qs_qsptr;
	dms_cb = (DMS_CB *)dsh->dsh_cbs[qseqp->qs_cbnum];
	if (qseqp->qs_flag & QS_NEXTVAL)
	    dms_cb->dms_flags_mask = DMS_NEXT_VALUE;
	else dms_cb->dms_flags_mask = DMS_CURR_VALUE;

	status = dmf_call(DMS_NEXT_SEQ, dms_cb);
	if (status != E_DB_OK)
	{
	    if (dms_cb->error.err_code == E_DM9D00_IISEQUENCE_NOT_FOUND)
	    {
		/* If the sequence wasn't found, some DDL must have
		** intervened, dropping or redefining the sequence since
		** the query plan was generated.  The sequence open doesn't
		** validate that the sequence exists, although it does lock
		** out sequence DDL from that point forward.
		** Mark the query plan invalid (in case it's a repeated
		** query), and return.
		*/
		dsh->dsh_qp_status |= DSH_QP_OBSOLETE;
		dsh->dsh_error.err_code = E_QE0023_INVALID_QUERY;
		if (dsh->dsh_qp_ptr->qp_ahd != action)
		{
		    return (E_DB_ERROR);
		}
		else
		{
		    return (E_DB_WARN);
		}
	    }
	    else if ( dms_cb->error.err_code == E_DM9D01_SEQUENCE_EXCEEDED )
	    {
		char	*snm;

		/* If sequence limit exceeded with no "cycle" option, 
		** SQLSTATE value of '2200H' is issued. In any event
		** the DMF code is transformed by qef_error(). */

		dms_seq = (DMS_SEQ_ENTRY*)dms_cb->dms_seq_array.data_address;
		snm = (char *)&dms_seq->seq_name;
		qef_error(dms_cb->error.err_code, 0L, status, 
		    &error, &dsh->dsh_error, 1,
		    qec_trimwhite(DB_MAXNAME, snm), snm);
	    }
	    else
		qef_error(dms_cb->error.err_code, 0L, status, 
		    &error, &dsh->dsh_error, 0);
	}
    }

    return(status);
}
