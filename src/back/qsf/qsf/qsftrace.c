/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <st.h>
#include    <ex.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <scf.h>
#include    <ulf.h>
#include    <ulx.h>
#include    <qsf.h>
#include    <ulm.h>
#include    <qsfint.h>
/*
[@#include@]...
*/

/**
**
**  Name: QSFTRACE.C - Holds trace point routines for QSF.
**
**  Description:
**      This file holds all trace point routines for QSF.
**
**	The externally visible routines contained in this file are:
**
**          qsf_trace() - Set or clear QSF trace points, or display info for the
**			  info request trace points.
**	    qst_trcheck() - Check a QSF trace point.
[@func_list@]...
**
**
**  History:
**      11-sep-86 (thurston)
**          Initial creation.  Dummied up for now.
**	19-may-87 (thurston)
**	    Added real code.
**	05-jul-87 (thurston)
**	    Added code to qsf_trace() to increment and decrement the # server
**	    wide QSF trace points that are set.
**	17-dec-87 (thurston)
**	    Upgraded to allow for server wide trace points and info request
**	    trace points in QSF.
**	29-jan-88 (thurston)
**	    Added info request trace points 503-505, and expanded 501 and 502.
**      25-mar-88 (thurston)
**          Added info request trace point 510, which will produce a ULM Memory
**	    Pool Map for QSF's memory pool.
**	16-may-89 (jrb)
**	    Changed scc_error to scc_trace since that's what it should be for
**	    tracing output.
**	06-nov-89 (jrb)
**	    Reapplied ule_format call changes; they were somehow lost.
**	28-aug-92 (rog)
**	    Prototype QSF.
**	30-nov-1992 (rog)
**	    Include ulf.h before qsf.h.
**	07-jul-1993 (rog)
**	    Minor code cleanup due to compiler warnings.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	10-Jan-2001 (jenjo02)
**	    qsf_session() function replaced with GET_QSF_CB macro.
**	    Deleted superfluous scf_call() declaration.
**      10-Jul-2006 (hanal04) Bug 116358
**          Added trace point QS511 to call ulm_print_pool() for the QSF pool.
**	29-Oct-2010 (jonj) SIR 120874
**	    Conform to use new uleFormat, DB_ERROR constructs.
*/


/*
**  Forward and/or External function references.
*/


/*
**  Definition of static variables and forward static functions.
*/

static  i4     dum1;               /* Dummy to give to ult_check_macro() */
static  i4	    dum2;		/* Dummy to give to ult_check_macro() */

/* Routine to process info req tp's */
static	DB_STATUS	qst_reqinfo( i4  tp );

static	STATUS		ex_handler( EX_ARGS *ex_args );


/*{
** Name: qsf_trace() - Set or clear QSF trace points, or display info for the
**		       info request trace points.
**
** Description:
**      This routines sets or clears QSF trace points. 
**
** Inputs:
**      qsf_debug_cb                    A standard debug control block.
**
** Outputs:
**      none
**
**	Returns:
**	    E_DB_OK
**	    E_DB_WARN
**	    E_DB_SEVERE
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    Sets or clears QSF trace points as specified.
**
** History:
**      11-sep-86 (thurston)
**          Initial creation.  Dummied up for now.
**      19-may-87 (thurston)
**          Put the real intestines into this routine.
**	05-jul-87 (thurston)
**	    Added code to increment and decrement the # server wide QSF trace
**	    points that are set.
**	17-dec-87 (thurston)
**	    Upgraded to allow for server wide trace points and info request
**	    trace points in QSF.
**	17-nov-1992 (rog)
**	    Added an exception handler.  Call qsf_session() to get the session
**	    CB.
[@history_template@]...
*/

DB_STATUS
qsf_trace( DB_DEBUG_CB *qsf_debug_cb )
{
    STATUS		status;
    i4             ts = qsf_debug_cb->db_trswitch;
    i4             tp = qsf_debug_cb->db_trace_point;
    QSF_CB		*scb;
    QSF_TRSTRUCT	*trstruct;
    EX_CONTEXT		ex_context;
    CS_SID		sid;

    if (EXdeclare(ex_handler, &ex_context) != OK)
    {
	EXdelete();
	return (E_DB_FATAL);
    }

    /* Something to break out of */
    do
    {
	if (tp >= QSQ_MINTRACE  &&  tp <= QSQ_MAXTRACE)
	{
	    /* Info request tracepoint */

	    status = qst_reqinfo(tp);
	    break;
	}
	else if (tp >= QSR_MINTRACE  &&  tp <= QSR_MAXTRACE)
	{
	    /* Server wide tracepoint */

	    trstruct = &Qsr_scb->qsr_trstruct;
	    tp -= (QSR_MINTRACE - 1);
	}
	else if (tp >= QSS_MINTRACE  &&  tp <= QSS_MAXTRACE)
	{
	    /* Session specific tracepoint */

	    /* Get QSF session control block */
	    /* ----------------------------- */
	    CSget_sid(&sid);
	    scb = GET_QSF_CB(sid);

	    trstruct = &scb->qss_trstruct;
	}
	else
	{
	    status = E_DB_WARN;			/* Unknown tracepoint */
	    break;
	}


	switch (ts)
	{
	  case DB_TR_NOCHANGE:
	    status = E_DB_OK;
	    break;

	  case DB_TR_ON:
	    if (ult_check_macro(&trstruct->trvect, tp, &dum1, &dum2))
	    {
		status = E_DB_OK;
	    }

	    if (ult_set_macro(&trstruct->trvect, tp, 0, 0) != E_DB_OK)
	    {
		status = E_DB_WARN;
	    }
	    else
	    {
		Qsr_scb->qsr_tracing++;
		status = E_DB_OK;
	    }
	    break;

	  case DB_TR_OFF:
	    if (!ult_check_macro(&trstruct->trvect, tp, &dum1, &dum2))
	    {
		status = E_DB_OK;
	    }

	    if (ult_clear_macro(&trstruct->trvect, tp) != E_DB_OK)
	    {
		status = E_DB_WARN;
	    }
	    else
	    {
		Qsr_scb->qsr_tracing--;
		status = E_DB_OK;
	    }
	    break;

	  default:
	    status = E_DB_ERROR;		/* Bad db_trswitch */
	    break;
	}
    } while (0);

    EXdelete();
    return(status);
}


/*{
** Name: qst_trcheck() - Check a QSF trace point.
**
** Description:
**      This routines checks a QSF trace point. 
**
** Inputs:
**      scb                             Ptr to the QSF session CB.
**					If this is NULL, this routine will
**					retrieve the QSF session CB from SCF if
**					needed for a session specific trace
**					point.
**	tp				The trace point to be checked.
**
** Outputs:
**	scb				Possibly set by this routine.
**
**	Returns:
**	    E_DB_OK
**	    E_DB_WARN
**	    E_DB_SEVERE
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      21-may-87 (thurston)
**          Initial creation.
**      17-dec-87 (thurston)
**          Upgraded to allow for server wide tracepoints in QSF.
**	30-nov-1992 (rog)
**	    Call qsf_session() to get the session info from SCF.
[@history_template@]...
*/

bool
qst_trcheck( QSF_CB *scb, i4  xtp )
{
    i4			tp = xtp;
    STATUS		status;
    QSF_TRSTRUCT	*trstruct;
    CS_SID		sid;


    if (tp >= QSR_MINTRACE  &&  tp <= QSR_MAXTRACE)
    {
	/* Server wide trace point */
	
	trstruct = &Qsr_scb->qsr_trstruct;
	tp -= (QSR_MINTRACE - 1);
    }
    else if (tp >= QSS_MINTRACE  &&  tp <= QSS_MAXTRACE)
    {
	/* Session specific trace point */
	
	/* Get QSF session control block, if don't have already */
	/* ---------------------------------------------------- */
	if (scb == NULL)
	{
	    CSget_sid(&sid);
	    scb = GET_QSF_CB(sid);
	}

	if (scb == NULL)
	    return (FALSE);

	trstruct = &scb->qss_trstruct;
    }
    else
    {
	return (FALSE);	    /* Unknown tracepoint */
    }

    
    if (ult_check_macro(&trstruct->trvect, tp, &dum1, &dum2))
	return (TRUE);
    else
	return (FALSE);
}


/**
** Name: qst_reqinfo() - Process and report info request trace points for QSF.
**
** Description:
**      This routines reports to the user (via SCC_TRACE) the information that
**	has been requested by the info request trace point specified.
**
** Inputs:
**	tp				The info request trace point.
**
** Outputs:
**	none
**
**	Returns:
**	    E_DB_OK
**	    E_DB_WARN
**	    E_DB_SEVERE
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    Sends a formatted message to user with requested info.
**
** History:
**      17-dec-87 (thurston)
**          Initial creation.
**	29-jan-88 (thurston)
**	    Added trace points 503-505, and expanded 501 and 502.
**      25-mar-88 (thurston)
**          Added code to produce memory pool map for trace point 510.
**      16-apr-92 (rog)
**          Added trace point 506 to flush the LRU queue.
**      16-nov-1992 (rog)
**          Reworked statistics trace points to include new statistics and
**	    delete old ones.  Added tp 507 to trace piece size statistics.
**	06-jan-1992 (rog)
**	    Added trace of qsr_no_destroyed.
**	30-may-03 (inkdo01)
**	    Added some stats to qs502 display. Note it runs the LRU chain
**	    without the protection of the semaphore, so counts may not
**	    be exactly right.
**	26-Sep-2003 (jenjo02)
**	    Added qsr_no_trans, qso_no_defined stats,
**	    QSF_508_LRU_DISPLAY trace point to dump the LRU queue.
**	23-Feb-2005 (schka24)
**	    Add hash spins to qs502.  Semaphore protect LRU scans.
[@history_template@]...
*/

static DB_STATUS
qst_reqinfo( i4  tp )
{
    SCF_CB		scf_cb;
    ULM_RCB		ulm_rcb;
    char		buf[1000];
    char		msg_buf[1000];
    i4		msg_length;
    i4		junk;
    i4		error;
    QSF_CB		*scb;
    QSO_OBJ_HDR		*obj;
    i4		nlru;
    CS_SID		sid;
    i4		destroyed;
    QSF_RCB	qsf_rb;


    switch (tp)
    {
      case QSF_501_MEMLEFT:
	STprintf(buf, "\
\n\t QSF memleft = %d\
\n\ttotal memory = %d\
\n\t      %% used = %6.2f", 
		    Qsr_scb->qsr_memleft,
		    Qsr_scb->qsr_memtot,
		    (100 * (1.0 - ((float)Qsr_scb->qsr_memleft /
				   (float)Qsr_scb->qsr_memtot))), '.'
		);
	break;

      case QSF_502_NUM_OBJS:
	{
	    i4	lru = 0;
	    i4	byebye = 0;
	    i4	free = 0;
	    i4	nowait = 0;
	    i4	avg_qtext, avg_qtree, avg_qp, avg_sqlda;
	    QSO_OBJ_HDR	*obj;

	    CSp_semaphore(TRUE, &Qsr_scb->qsr_psem);
	    for (obj = Qsr_scb->qsr_1st_lru; obj != (QSO_OBJ_HDR *)NULL;
			obj = obj->qso_lrnext)
	    {
		lru++;
		if (obj->qso_waiters == 0)
		    nowait++;
		if (obj->qso_lk_state == QSO_FREE)
		    free++;
		if (obj->qso_status & QSO_LRU_DESTROY)
		    byebye++;
	    }
	    CSv_semaphore(&Qsr_scb->qsr_psem);
	    STprintf(buf, "\
\n\t        # of unnamed = %d\
\n\t          # of named = %d\
\n\t     # of index objs = %d\
\n\t          # QSF objs = %d\
\n\t   Most ever unnamed = %d\
\n\t     Most ever named = %d\
\n\tMost ever index objs = %d\
\n\t     Most ever total = %d\
\n\t      # on LRU chain = %d\
\n\t       # with wait 0 = %d\
\n\t     # with QSO_FREE = %d\
\n\t   being deleted now = %d\
\n\t# alias translations = %d\
\n\t           # DEFINED = %d\
\n\t # delete-wait spins = %d",
		 Qsr_scb->qsr_no_unnamed,
		 Qsr_scb->qsr_no_named,
		 Qsr_scb->qsr_no_index,
		 Qsr_scb->qsr_nobjs,
		 Qsr_scb->qsr_mx_unnamed,
		 Qsr_scb->qsr_mx_named,
		 Qsr_scb->qsr_mx_index,
		 Qsr_scb->qsr_mxobjs,
		 lru, nowait, free, byebye,
		 Qsr_scb->qsr_no_trans,
		 Qsr_scb->qsr_no_defined,
		 Qsr_scb->qsr_hash_spins
		);
	    avg_qtext = 0;
	    if (Qsr_scb->qsr_num_size[QSO_QTEXT_OBJ] > 0)
		avg_qtext = (i4) (Qsr_scb->qsr_sum_size[QSO_QTEXT_OBJ] / (i8) Qsr_scb->qsr_num_size[QSO_QTEXT_OBJ]);
	    avg_qtree = 0;
	    if (Qsr_scb->qsr_num_size[QSO_QTREE_OBJ] > 0)
		avg_qtree = (i4) (Qsr_scb->qsr_sum_size[QSO_QTREE_OBJ] / (i8) Qsr_scb->qsr_num_size[QSO_QTREE_OBJ]);
	    avg_qp = 0;
	    if (Qsr_scb->qsr_num_size[QSO_QP_OBJ] > 0)
		avg_qp = (i4) (Qsr_scb->qsr_sum_size[QSO_QP_OBJ] / (i8) Qsr_scb->qsr_num_size[QSO_QP_OBJ]);
	    avg_sqlda = 0;
	    if (Qsr_scb->qsr_num_size[QSO_SQLDA_OBJ] > 0)
		avg_sqlda = (i4) (Qsr_scb->qsr_sum_size[QSO_SQLDA_OBJ] / (i8) Qsr_scb->qsr_num_size[QSO_SQLDA_OBJ]);
	    STprintf(&buf[STlength(buf)],"\
\n\tAverage object sizes (at destroy):\
\n\tQTEXT = %d,  QTREE = %d,  QP = %d,  SQLDA = %d",
		avg_qtext, avg_qtree, avg_qp, avg_sqlda);
	    break;
	}

      case QSF_503_NUM_BKTS:
	STprintf(buf, "\
\n\t     # QSF hash buckets = %d\
\n\t           # in use now = %d\
\n\tMost ever in any bucket = %d\
\n\t       Most in use ever = %d",
		    Qsr_scb->qsr_nbuckets,
		    Qsr_scb->qsr_bkts_used,
		    Qsr_scb->qsr_bmaxobjs,
		    Qsr_scb->qsr_mxbkts_used
		);
	break;

      case QSF_504_NUM_SESS:
	STprintf(buf, "\
\n\t    # QSF sessions = %d\
\n\tMost sessions ever = %d",
		    Qsr_scb->qsr_nsess,
		    Qsr_scb->qsr_mxsess
		);
	break;

      case QSF_505_LRU:
	CSp_semaphore(TRUE, &Qsr_scb->qsr_psem);
        obj = Qsr_scb->qsr_1st_lru;
	nlru = 0;
	while (obj != (QSO_OBJ_HDR *) NULL)
	{
	    nlru++;
	    obj = obj->qso_lrnext;
	}
	CSv_semaphore(&Qsr_scb->qsr_psem);
	STprintf(buf, "\
\n\t    # objs in LRU queue = %d\
\n\t# LRU objects destroyed = %d",
		 nlru, Qsr_scb->qsr_no_destroyed
		);
	break;

      case QSF_506_LRU_FLUSH:
	CSget_sid(&sid);
	scb = GET_QSF_CB(sid);
	/* qsf_clrmem() now returns #destroyed */
	for (nlru = 0; (destroyed = qsf_clrmem(scb)) ; nlru += destroyed) ;
	STprintf(buf, "\n\tNumber of LRU objects cleared = %d", nlru);
	break;

      case QSF_507_SIZE_STATS:
	STprintf(buf,
    "\n\tLargest QSF piece allocated = %d\n\tLargest QSF piece requested = %d",
		    Qsr_scb->qsr_mx_size, Qsr_scb->qsr_mx_rsize);
	break;

      case QSF_508_LRU_DISPLAY:
	qsd_lru_dump();
	STprintf(buf, "\nQSF's LRU Queue has been written to your log file.");
	break;

      case QSF_510_POOLMAP:
	ulm_rcb.ulm_poolid = Qsr_scb->qsr_poolid;
	ulm_rcb.ulm_facility = DB_QSF_ID;
	_VOID_ ulm_mappool(&ulm_rcb);
	STprintf(buf, "\nULM Memory Pool Map for QSF's memory pool has been \
written to your log file."
		);
	break;

      case QSF_511_ULM_PRINT:
	qsf_rb.qsf_type = QSFRB_CB;
	qsf_rb.qsf_ascii_id = QSFRB_ASCII_ID;
	qsf_rb.qsf_length = sizeof(qsf_rb);
	qsf_rb.qsf_owner = (PTR) DB_QSF_ID;
        qsf_rb.qsf_obj_id.qso_handle = (PTR) NULL;

	qsd_obq_dump(&qsf_rb);
	
	ulm_rcb.ulm_poolid = Qsr_scb->qsr_poolid;
	ulm_rcb.ulm_facility = DB_QSF_ID;
	ulm_print_pool(&ulm_rcb);
	STprintf(buf, "\nQSF Object Pool and ULM Memory Print Pool output has \
been written to your \nlog file.");
	break;

      default:
	return (E_DB_WARN);	/* Unknown trace QSF info request trace point */
    }

    _VOID_ uleFormat( NULL,
		    E_QS0500_INFO_REQ,
		    NULL,
		    (i4) ULE_LOOKUP,
		    NULL,
		    msg_buf,
		    (i4) sizeof(msg_buf),
		    &msg_length,
		    &error, 4,
		    (i4) sizeof(tp),
		    (i4 *) &tp,
		    (i4) STlength(buf),
		    (i4 *) buf
		);

    scf_cb.scf_length = sizeof(scf_cb);
    scf_cb.scf_type = SCF_CB_TYPE;
    scf_cb.scf_facility = DB_QSF_ID;
    scf_cb.scf_session = DB_NOSESSION;
    scf_cb.scf_nbr_union.scf_local_error = E_QS0500_INFO_REQ;
    scf_cb.scf_len_union.scf_blength = msg_length;
    scf_cb.scf_ptr_union.scf_buffer = msg_buf;
    _VOID_ scf_call(SCC_TRACE, &scf_cb);

    scb = NULL;
    if (Qsr_scb->qsr_tracing  &&  qst_trcheck(scb, QSF_1001_LOG_INFO_REQS))
    {
	_VOID_ uleFormat( NULL,
			(i4) 0,
			NULL,
			(i4) ULE_MESSAGE,
			NULL,
			msg_buf,
			msg_length,
			&junk,
			&error, 0
		    );
    }

    return (E_DB_OK);
}

/*
** Name: ex_handler	- Exception handler to trap errors
**
** Description:
**      This routine traps any unexpected exceptions which occur 
**      within the boundaries of a qsf_trace(), logs the error, 
**      and returns.  qsf_trace() will interpret this as a fatal error.
**
** Inputs:
**      ex_args			Arguments provided by the exception mechanism
**
** Outputs:
**      (none)
**
**	Returns:
**	    EX_DECLARE
**
**	Exceptions:
**	    (in response)
**
** Side Effects:
**	Logs an error message.
**
** History:
**	17-nov-1992 (rog)
**          Initial creation.
*/

static STATUS
ex_handler( EX_ARGS *ex_args )
{
    /*
    ** Got an unexpected exception -- log it.
    ** No semaphores are set, so no need to check them.
    */

    ulx_exception(ex_args, DB_QSF_ID, E_QS9002_UNEXPECTED_EXCEPTION, TRUE);

    return (EX_DECLARE);
}
