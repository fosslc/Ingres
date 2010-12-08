/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <me.h>
#include    <st.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <cs.h>
#include    <scf.h>
#include    <ulf.h>
#include    <ulm.h>
#include    <adf.h>
#include    <dmf.h>
#include    <dmtcb.h>
#include    <dmrcb.h>
#include    <qefmain.h>
#include    <qefkon.h>
#include    <qsf.h>
#include    <qefrcb.h>
#include    <qefcb.h>
#include    <qefnode.h>
#include    <psfparse.h>
#include    <qefact.h>
#include    <qefqp.h>
#include    <qefdsh.h>
#include    <qefscb.h>
#include    <tpf.h>

#include    <ade.h>
#include    <dudbms.h>
#include    <dmmcb.h>
#include    <dmucb.h>
#include    <ex.h>
#include    <qefqeu.h>
#include    <qeuqcb.h>
#include    <rqf.h>
#include    <qefqry.h>
#include    <qefcat.h>
#include    <tm.h>
#include    <sxf.h>
#include    <qefprotos.h>


/**
**
**  Name: QETTPF.C - Titan transaction management routines
**
**  Description:
**      The majority of the routines in this file have been adapted from QET.C.
**
**          qet_t1_abort	- abort to savepoint or current transaction
**          qet_t2_begin	- begin a DX
**          qet_t3_commit	- commit a DX
**          qet_t4_savept	- declare a savepoint within a transaction
**          qet_t5_register	- register a read/update operation with TPF 
**          qet_t6_ddl_cc	- set DDL concurrency ON | OFF
**	    qet_t7_p1_recover	- call TPF to determine if recovery is necessary
**	    qet_t8_p2_recover	- call TPF to recover all DDBs in iidbdb
**	    qet_t9_ok_w_ldbs	- call TPF to validate the LDBs of a write query
**	    qet_t10_is_ddb_open	- call TPF to see if a given DDB is open
**	    qet_t20_recover	- call TPF to recover all DDBs in iidbdb
**
**  History:    $Log-for RCS$
**	17-oct-88 (carl)
**	    adapted from QET.C
**	20-feb-89 (carl)
**	    removed qet_t5_set_auto,  qet_t7_eoq and qet_t8_rollback;  
**	    renamed qet_t6_intent to qet_t5_register
**	18-may-89 (carl)
**	    added qet_t6_ddl_cc to manage DDL concurrency 
**	29-jul-89 (carl)
**	    fixed qet_t1_abort and qet_t3_commit to reinitialize the session's
**	    state
**	07-oct-89 (carl)
**	    removed from qet_t1_abort check for transaction state 
**	11-nov-89 (carl)
**	    added qet_t20_recover and qet_t9_ok_w_ldbs
**	28-jan-90 (carl)
**	    added qet_t7_p1_recover and qet_t8_p2_recover
**	02-jun-90 (carl)
**	    changed qet_t9_ok_w_ldbs to test qeq_q3_ctl_info for 
**	    QEQ_002_USER_UPDATE instead of qeq_q3_read_b which is obsolete
**	28-aug-90 (carl)
**	    changed all occurrences of tpf_call to qed_u17_tpf_call 
**	12-sep-90 (carl)
**	    changed qet_t9_ok_w_ldbs() to use qec_m1_mopen(), qec_m2_mclose()
**	    and qec_m3_malloc() 
**	07-oct-90 (carl)
**	    changed remaining occurrences of tpf_call to qed_u17_tpf_call 
**	28-feb-91 (fpang)
**	    Propagate tpr_error.err_code in qet_t3_commit() in the case
**	    of failures.
**	    Fixes B32157 and B32148.
**	18-jun-92 (davebf)
**	    Updated for Sybil - removed calls to qecmem.c routines
**	08-dec-92 (anitap)
**	    Added #include <qsf.h> and <psfparse.h> for CREATE SCHEMA.
**	14-apr-94 (peters)
**	    Changed the return value of qet_t1_abort when the call to 
**	    qed_u17_tpf_call(TPF_ABORT, ...) fails.  Fixes bug 60634.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	2-Dec-2010 (kschendel) SIR 124685
**	    Warning, prototype fixes.
**/



/*{
** Name: QET_T1_ABORT - abort to savepoint if there is one or else transaction
**
** Description:
**      A transaction is aborted to a savepoint or to the beginning of the 
**  transaction (at which point the transaction is closed).  TPF will be 
**  called to perform the appropriate operation on each LDB in the transaction.
**  It is assumed that the caller has verified that this is a valid operation.
**
** Inputs:
**	qef_cb			session control block
**	    .qef_abort		abort transaction if TRUE, abort to
**				savepoint if FALSE
** Outputs:
**      qef_cb
**          .qef_rcb
**		.error.err_code	    One of the following:
**                                  E_QE0000_OK
**                                  E_QE0017_BAD_CB
**                                  E_QE0018_BAD_PARAM_IN_CB
**                                  E_QE0019_NON_INTERNAL_FAIL
**                                  E_QE0002_INTERNAL_ERROR
**                                  E_QE0004_NO_TRANSACTION
**                                  E_QE0005_NO_SAVEPOINT
**				    E_QE0032_BAD_SAVEPOINT_NAME
**      Returns:
**          E_DB_OK                 
**          E_DB_ERROR              caller error
**          E_DB_FATAL              internal error
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**	17-oct-88 (carl)
**	    adapted from qet_abort 
**	29-jul-89 (carl)
**	    fixed to reinitialize the session's state
**	07-oct-89 (carl)
**	    removed from qet_t1_abort check for transaction state 
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	14-apr-94 (peters)
**	    Changed the return value of qet_t1_abort when the call to 
**	    qed_u17_tpf_call(TPF_ABORT, ...) fails.  Until now, E_DB_FATAL 
**	    was always passed up, now the return value of qed_u17_tpf_call()
**	    is passed up.  This may be E_DB_ERROR and will not cause the
**	    star server to crash.  Fixes bug 60634.
*/


DB_STATUS
qet_t1_abort(
QEF_CB         *qef_cb)
{
    bool	savept_b = FALSE,
		abort_b = TRUE;		    /* assume aborting transaction */
    DB_STATUS   status;
    QES_DDB_SES	*dds_p = & qef_cb->qef_c2_ddb_ses;
    QES_QRY_SES	*qss_p = & dds_p->qes_d8_union.u2_qry_ses;
    TPR_CB	tpr_cb,
		*tpr_p = & tpr_cb;


    dds_p->qes_d7_ses_state = QES_0ST_NONE;	/* must reinitialize */

    /* caller may have already set qef_cb->qef_stat to QEF_NOTRAN, 
    ** so don't check */

    savept_b = (qef_cb->qef_rcb->qef_spoint && qef_cb->qef_abort == FALSE);

    MEfill(sizeof(tpr_cb), '\0', (PTR) & tpr_cb);
    tpr_p->tpr_session = qef_cb->qef_c2_ddb_ses.qes_d2_tps_p;
    tpr_p->tpr_rqf = qef_cb->qef_c2_ddb_ses.qes_d3_rqs_p;
    tpr_p->tpr_lang_type = qss_p->qes_q2_lang;

    if (savept_b)				/* save point ? */
    {
	/* abort to user-defined savepoint */

	tpr_p->tpr_save_name = qef_cb->qef_rcb->qef_spoint;
	status = qed_u17_tpf_call(TPF_SP_ABORT, tpr_p, qef_cb->qef_rcb);
	if (status)
	{
	    if (tpr_p->tpr_error.err_code == E_TP0012_SP_NOT_EXIST)
		qef_cb->qef_rcb->error.err_code = E_QE0016_INVALID_SAVEPOINT;
	    return(status);
	}
	abort_b = FALSE;			/* done, no transaction abort */
    }

    if (abort_b)				/* aborting transaction ? */
    {
	qef_cb->qef_stat = QEF_NOTRAN;		/* preset */

	status = qed_u17_tpf_call(TPF_ABORT, tpr_p, qef_cb->qef_rcb);
	if (status)
	{
	    /* cannot abort with TPF, return error */
	    return(status);
	}
    }

    /* set state back to quiescent state */

    qef_cb->qef_open_count = 0;			/* no cursors opened */
    qef_cb->qef_defr_curs = FALSE;		/* no deferred cursor */
    qef_cb->qef_abort = FALSE;

    if (savept_b == FALSE)
	qef_cb->qef_stat = QEF_NOTRAN;

    return(status);
}


/*{
** Name: QET_T2_BEGIN - begin transaction
**
** Description:
**      TPF will be called to begin a transaction.  It is assumed that the 
**  caller has verified that this is a valid operation.
**
** Inputs:
**      qef_rcb
**	    .qef_cb		session control block
**	    .qef_eflag		designate error handling semantis
**				for user errors.
**		QEF_INTERNAL	return error code.
**		QEF_EXTERNAL	send message to user.
** Outputs:
**      qef_cb
**          .qef_rcb
**		.error.err_code	    One of the following:
**                                  E_QE0000_OK
**                                  E_QE0017_BAD_CB
**                                  E_QE0018_BAD_PARAM_IN_CB
**                                  E_QE0019_NON_INTERNAL_FAIL
**                                  E_QE0002_INTERNAL_ERROR
**                                  E_QE0006_TRANSACTION_EXISTS
**      Returns:
**          E_DB_OK                 
**          E_DB_ERROR              caller error
**          E_DB_FATAL              internal error
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**	17-oct-88 (carl)
**	    adapted from qet_begin
*/


DB_STATUS
qet_t2_begin(
QEF_CB     *qef_cb)
{
    DB_STATUS   status;
    TPR_CB	tpr_cb,
		*tpr_p = & tpr_cb;


    /* caller may have already set qef_cb->qef_stat, so don't check */

    MEfill(sizeof(tpr_cb), '\0', (PTR) & tpr_cb);
    tpr_p->tpr_session = qef_cb->qef_c2_ddb_ses.qes_d2_tps_p;
    tpr_p->tpr_rqf = qef_cb->qef_c2_ddb_ses.qes_d3_rqs_p;
    tpr_p->tpr_lang_type = DB_NOLANG;

    status = qed_u17_tpf_call(TPF_BEGIN_DX, tpr_p, qef_cb->qef_rcb);
    if (status)
    {
	return(status);
    }

    MEfill(sizeof(DB_TRAN_ID), ' ', (PTR) & qef_cb->qef_tran_id);
    qef_cb->qef_stmt = 1;			/* renumber statements for new 
						** xact */
    qef_cb->qef_defr_curs = FALSE;		/* no deferred cursor yet */
    return (E_DB_OK);
}


/*{
** Name: QET_T3_COMMIT - commit or end a DX
**
** Description:
**      TPF will be called to commit all LDBs and hence the transaction.  It
**  is assumed that the caller has verified that this is a valid operation.
**
** Inputs:
**	qef_cb		session control block
**	    .qef_rcb
**	    .qef_eflag		designate error handling semantis
**				for user errors.
**		QEF_INTERNAL	return error code.
**		QEF_EXTERNAL	send message to user.
** Outputs:
**	qef_cb
**	    .qef_rcb
**		.error.err_code	    One of the following:
**                                  E_QE0000_OK
**                                  E_QE0017_BAD_CB
**                                  E_QE0018_BAD_PARAM_IN_CB
**                                  E_QE0019_NON_INTERNAL_FAIL
**                                  E_QE0002_INTERNAL_ERROR
**                                  E_QE0004_NO_TRANSACTION
**      Returns:
**          E_DB_OK                 
**          E_DB_ERROR              caller error
**          E_DB_FATAL              internal error
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**	17-oct-88 (carl)
**	    adapted from qet_commit
**	29-jul-89 (carl)
**	    fixed to reinitialize the session's state
**	28-feb-91 (fpang)
**	    Propagate tpr_error.err_code in qet_t3_commit() in the case
**	    of failures.
**	    Fixes B32157 and B32148.
*/


DB_STATUS
qet_t3_commit(
QEF_CB     *qef_cb)
{
    DB_STATUS   status;
    QES_DDB_SES	*dds_p = & qef_cb->qef_c2_ddb_ses;
    QES_QRY_SES	*qss_p = & dds_p->qes_d8_union.u2_qry_ses;
    TPR_CB	tpr_cb,
		*tpr_p = & tpr_cb;


    dds_p->qes_d7_ses_state = QES_0ST_NONE;	/* must reinitialize */

    /* caller may have already set qef_cb->qef_stat to QEF_NOTRAN, 
    ** so don't check */

    qef_cb->qef_open_count = 0;		    /* no cursors opened */
    qef_cb->qef_defr_curs = FALSE;	    /* no deferred cursor */
    qef_cb->qef_abort = FALSE;
    qef_cb->qef_stat = QEF_NOTRAN;
    qef_cb->qef_stmt = 1;		    /* renumber statements for new 
					    ** transaction */
    MEfill(sizeof(tpr_cb), '\0', (PTR) & tpr_cb);
    tpr_p->tpr_session = qef_cb->qef_c2_ddb_ses.qes_d2_tps_p;
    tpr_p->tpr_rqf = qef_cb->qef_c2_ddb_ses.qes_d3_rqs_p;
    tpr_p->tpr_lang_type = qss_p->qes_q2_lang;

    status = qed_u17_tpf_call(TPF_COMMIT, tpr_p, qef_cb->qef_rcb);
    if (status)
    {
	qef_cb->qef_rcb->error.err_code = tpr_p->tpr_error.err_code;
	return(status);
    }
    qef_cb->qef_rcb->qef_comstamp.db_tab_high_time = 0;
    qef_cb->qef_rcb->qef_comstamp.db_tab_low_time = 0;
    return (E_DB_OK);
}


/*{
** Name: QET_T4_SAVEPT - set a savepoint
**
** Description:
**      Set a savepoint in current transaction. 
**
** Inputs:
**	qef_cb			session control block
**	    .qef_rcb
**	    .qef_eflag		designate error handling semantis
**				for user errors.
**		QEF_INTERNAL	return error code.
**		QEF_EXTERNAL	send message to user.
**          .qef_spoint         name of user defined savepoint
**	    .qef_flag		DB_QUEL or DB_SQL
** Outputs:
**	qef_cb			    session control block
**	    .qef_rcb
**		.error.err_code	    One of the following:
**                                  E_QE0000_OK
**                                  E_QE0017_BAD_CB
**                                  E_QE0018_BAD_PARAM_IN_CB
**                                  E_QE0019_NON_INTERNAL_FAIL
**                                  E_QE0002_INTERNAL_ERROR
**                                  E_QE0004_NO_TRANSACTION
**                                  E_QE0016_INVALID_SAVEPOINT
**                                  E_QE0020_OPEN_CURSOR
**				    E_QE0032_BAD_SAVEPOINT_NAME
**      Returns:
**          E_DB_OK                 
**          E_DB_ERROR              caller error
**          E_DB_FATAL              internal error
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**	17-oct-88 (carl)
**	    adapted from qet_savepoint
**	29-jan-1993 (fpang)
**	    Check qef_auto for QEF_ON or QEF OFF instead of non zero.
*/


DB_STATUS
qet_t4_savept(
QEF_CB         *qef_cb)
{
    i4	err;
    DB_STATUS   status;
    TPR_CB	tpr_cb,
		*tpr_p = & tpr_cb;


    if (qef_cb->qef_rcb->qef_spoint == 0)
    {
        qef_error(E_QE0018_BAD_PARAM_IN_CB, 0L, E_DB_WARN, &err,
		  &qef_cb->qef_rcb->error, 0);
        return (E_DB_WARN);
    }

    /* Must be in a transaction */

    if (qef_cb->qef_stat == QEF_NOTRAN)
    {
	if (qef_cb->qef_rcb->qef_flag == DB_QUEL)
	{
	    qef_error(E_QE0004_NO_TRANSACTION, 0L, E_DB_WARN, &err,
		      &qef_cb->qef_rcb->error, 0);
	    return (E_DB_WARN);
	}
	else if (qef_cb->qef_auto == QEF_ON)
	{
	    /* savepoint not allowed when autocommit is on */
	    qef_error(2178L, 0L, E_DB_WARN, &err, &qef_cb->qef_rcb->error, 0);
	    return (E_DB_WARN);
	}
    }

    /* Cannot set a savepoint if there are open cursors */

    if (qef_cb->qef_open_count > 0)
    {
        qef_error(E_QE0020_OPEN_CURSOR, 0L, E_DB_WARN, &err,
		  &qef_cb->qef_rcb->error, 0);
        return (E_DB_WARN);
    }

    MEfill(sizeof(tpr_cb), '\0', (PTR) & tpr_cb);
    tpr_p->tpr_session = qef_cb->qef_c2_ddb_ses.qes_d2_tps_p;
    tpr_p->tpr_rqf = qef_cb->qef_c2_ddb_ses.qes_d3_rqs_p;
    tpr_p->tpr_save_name = qef_cb->qef_rcb->qef_spoint;

    if (qef_cb->qef_rcb->qef_flag == DB_QUEL)
	tpr_p->tpr_lang_type = DB_QUEL;
    else if (qef_cb->qef_rcb->qef_flag == DB_SQL)
	tpr_p->tpr_lang_type = DB_SQL;
    else
	tpr_p->tpr_lang_type = DB_NOLANG;

    status = qed_u17_tpf_call(TPF_SAVEPOINT, tpr_p, qef_cb->qef_rcb);

    return(status);
}


/*{
** Name: QET_T5_REGISTER - Inform TPF prior to executing a series of 
**			   retrievals or updates on an LDB
**
** Description:
**	This routine is used to inform TPF once before executing a series
**  of queries that are either read-only or updates on an LDB.  Its purpose
**  is to eliminate TPF interaction for each such query.
**
** Inputs:
**	    v_qer_p			QEF_RCB
**              .qef_cb			QEF_CB
**	    i_ldb_p			ptr to DD_LDB_DESC for LDB
**	    i_lang			DB_SQL or DB_QUEL
**	    i_mode			QEK_2TPF_READ or QEK_3TPF_UPDATE
**					
** Outputs:
**	none
**          Returns:
**		E_DB_OK                 
**		E_DB_ERROR              caller error
**		E_DB_FATAL              internal error
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**	21-oct-88 (carl)
**          written
*/


DB_STATUS
qet_t5_register(
QEF_RCB         *v_qer_p,
DD_LDB_DESC	*i_ldb_p,
DB_LANG		i_lang,
i4		i_mode)
{
    DB_STATUS	    status;
    QES_DDB_SES	    *dds_p = & v_qer_p->qef_cb->qef_c2_ddb_ses;
    TPR_CB	    tpr_cb,
		    *tpr_p = & tpr_cb;
    i4		    tpf_opcode;


    if (! (i_lang == DB_SQL || i_lang == DB_QUEL))
    {
	status = qed_u1_gen_interr(& v_qer_p->error);
	return(status);
    }

    MEfill(sizeof(tpr_cb), '\0', (PTR) & tpr_cb);
    tpr_p->tpr_session = dds_p->qes_d2_tps_p;	/* TPF session cb ptr */
    tpr_p->tpr_rqf = dds_p->qes_d3_rqs_p;	/* RQF session cb ptr */
    tpr_p->tpr_site = i_ldb_p;
    tpr_p->tpr_lang_type = i_lang;

    switch (i_mode)
    {
    case QEK_2TPF_READ:
    case QEK_4TPF_1PC:
	tpf_opcode = TPF_READ_DML;
	break;
    case QEK_3TPF_UPDATE:
	tpf_opcode = TPF_WRITE_DML;
	break;
    default:
	status = qed_u1_gen_interr(& v_qer_p->error);
	return(status);
    }

    status = qed_u17_tpf_call(tpf_opcode, tpr_p, v_qer_p);

    return(status);
}


/*{
** Name: QET_T6_DDL_CC - set DDL concurrency on or off
**
** External QEF call:   status = qef_call(QED_CONCURRENCY, & qef_rcb);
**
** Description:
**	It is illegal to use this command inside a transaction.
**
** Inputs:
**      v_qer_p
**	    .qef_cb		session control block
**	    .qef_r3_ddb_req
**		.qer_d12_ddl_concur_on_b
**				TRUE or FALSE
**	    .qef_eflag		designate error handling semantis
**				for user errors.
**		QEF_INTERNAL	return error code.
**		QEF_EXTERNAL	send message to user.
**
** Outputs:
**      v_qer_p
**                                  2176 - no autocommit if in a transaction
**
**      Returns:
**          E_DB_OK                 
**          E_DB_ERROR              caller error
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**	18-may-89 (carl)
**	    added qet_t6_ddl_cc to manage DDL concurrency 
**	29-jan-1993 (fpang)
**	    Compare qer_d12_ddl_concur_on_b to QEF_ON or QEF_OFF
**	    instead of testing for non-zero.
*/


DB_STATUS
qet_t6_ddl_cc(
QEF_RCB         *v_qer_p)
{
    DB_STATUS	    status= E_DB_OK;
    QEF_CB	    *qef_cb = v_qer_p->qef_cb;
    QEF_DDB_REQ	    *ddr_p = & v_qer_p->qef_r3_ddb_req;
    QES_DDB_SES	    *dds_p = & v_qer_p->qef_cb->qef_c2_ddb_ses;
    TPR_CB	    tpr_cb,
		    *tpr_p = & tpr_cb;


    /* make sure that this is a legal QEF call */

    if (qef_cb->qef_stat != QEF_NOTRAN)
    {
        status = qed_u2_set_interr(
		    E_QE0962_DDL_CC_ON_XACTION, & v_qer_p->error);
        return (E_DB_WARN);			/* a warning */
    }

    MEfill(sizeof(tpr_cb), '\0', (PTR) & tpr_cb);
    tpr_p->tpr_session = dds_p->qes_d2_tps_p;	/* TPF session cb ptr */
    tpr_p->tpr_rqf = dds_p->qes_d3_rqs_p;	/* RQF session cb ptr */

    if (ddr_p->qer_d12_ddl_concur_on_b == QEF_ON)
    {
	dds_p->qes_d9_ctl_info |= QES_01CTL_DDL_CONCURRENCY_ON;
	status = qed_u17_tpf_call(TPF_1T_DDL_CC, tpr_p, v_qer_p);
    }
    else
    {
	dds_p->qes_d9_ctl_info &= ~QES_01CTL_DDL_CONCURRENCY_ON;
	status = qed_u17_tpf_call(TPF_0F_DDL_CC, tpr_p, v_qer_p);
    }

    return(status);
}



/*{
** Name: qet_t7_p1_recover - call TPF to find out if recovery is necessary
**
** External QEF call:   status = qef_call(QED_P1_RECOVER, & qef_rcb);
**
** Description:
**	This is called immediately after a server starts up to find out
**	if recovery is necessary.
**
**	TPF is called to do the hard work.
**
** Inputs:
**      v_qer_p			ptr to QEF_RCB
**	    .qef_cb		session control block
**		.qef_c2_ddb_sess
**		    .qes_d2_tps_p
**				TPF's session CB ptr
**		    .qes_d3_rqs_p
**				RQF's session CB ptr
**	    .qef_r3_ddb_req		QEF_DDB_REQ
**		.qer_d2_ldb_info_p	DD_LDB_DESC ptr
**		    .dd_l1_ingres_b	FALSE
**		    .dd_l2_node_name    blanks for default
**		    .dd_l3_ldb_name	"iidbdb"
**		    .dd_l4_dbms_name    INGRES
**		    .dd_l5_dbms_code    DD_1DBMS_INGRES (1)
**		    .dd_l6_ldb_id	DD_UNASSIGNED_IS_0
**
** Outputs:
**      v_qer_p				ptr to QEF_RCB
**	    .qef_r3_ddb_req		QEF_DDB_REQ
**		.qer_d13_ctl_info	QEF_04DD_ODX_FOUND ON/OFF
**
**      Returns:
**          DB_STATUS
**
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**	28-jan-90 (carl)
**	    created
*/


DB_STATUS
qet_t7_p1_recover(
QEF_RCB         *v_qer_p)
{
    DB_STATUS	    status= E_DB_OK;
    QEF_DDB_REQ	    *ddr_p = & v_qer_p->qef_r3_ddb_req;
    QES_DDB_SES	    *dds_p = & v_qer_p->qef_cb->qef_c2_ddb_ses;
    DD_LDB_DESC	    *iidbdb_p =  & v_qer_p->qef_r3_ddb_req.qer_d2_ldb_info_p->
			dd_i1_ldb_desc;
    TPR_CB	    tpr_cb,
		    *tpr_p = & tpr_cb;


    ddr_p->qer_d13_ctl_info &= ~QEF_04DD_RECOVERY;  /* assume NO recovery */

    MEfill(sizeof(tpr_cb), '\0', (PTR) & tpr_cb);
    tpr_p->tpr_session = dds_p->qes_d2_tps_p;	/* TPF session CB ptr */
    tpr_p->tpr_rqf = dds_p->qes_d3_rqs_p;	/* RQF session CB ptr */
    tpr_p->tpr_site = iidbdb_p;
						/* iidbdb desc */
    status = qed_u17_tpf_call(TPF_P1_RECOVER, tpr_p, v_qer_p);
    if (status == E_DB_OK)
    {
	if (tpr_p->tpr_need_recovery)
	    ddr_p->qer_d13_ctl_info |= QEF_04DD_RECOVERY;	
						/* yes, recovery */
    }
    return(status);
}


/*{
** Name: qet_t8_p2_recover - call TPF to recover all DDBs in the installation
**
** External QEF call:   status = qef_call(QED_P2_RECOVER, & qef_rcb);
**
** Description:
**	This is called immediately after a server starts up to recover all
**	the DDBs in the current installation as found out by QED_P1_RECOVER.
**
**	TPF is called to do the hard work.
**
** Inputs:
**      v_qer_p			ptr to QEF_RCB
**	    .qef_cb		session control block
**		.qef_c2_ddb_sess
**		    .qes_d2_tps_p
**				TPF's session CB ptr
**		    .qes_d3_rqs_p
**				RQF's session CB ptr
**	    .qef_r3_ddb_req		QEF_DDB_REQ
**		.qer_d2_ldb_info_p	DD_LDB_DESC ptr
**		    .dd_l1_ingres_b	FALSE
**		    .dd_l2_node_name    blanks for default
**		    .dd_l3_ldb_name	"iidbdb"
**		    .dd_l4_dbms_name    INGRES
**		    .dd_l5_dbms_code    DD_1DBMS_INGRES (1)
**		    .dd_l6_ldb_id	DD_UNASSIGNED_IS_0
**
** Outputs:
**      none
**
**      Returns:
**          DB_STATUS
**
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**	11-nov-89 (carl)
**	    created
*/


DB_STATUS
qet_t8_p2_recover(
QEF_RCB         *v_qer_p)
{
    DB_STATUS	    status= E_DB_OK;
    QES_DDB_SES	    *dds_p = & v_qer_p->qef_cb->qef_c2_ddb_ses;
    DD_LDB_DESC	    *iidbdb_p =  & v_qer_p->qef_r3_ddb_req.qer_d2_ldb_info_p->
			dd_i1_ldb_desc;
    TPR_CB	    tpr_cb,
		    *tpr_p = & tpr_cb;


    MEfill(sizeof(tpr_cb), '\0', (PTR) & tpr_cb);
    tpr_p->tpr_session = dds_p->qes_d2_tps_p;	/* TPF session CB ptr */
    tpr_p->tpr_rqf = dds_p->qes_d3_rqs_p;	/* RQF session CB ptr */
    tpr_p->tpr_site = iidbdb_p;
						/* iidbdb desc */
    status = qed_u17_tpf_call(TPF_P2_RECOVER, tpr_p, v_qer_p);

    return(status);
}


/*{
** Name: qet_t9_ok_w_ldbs - request TPF to validate the ldbs of a write query
**
** Description:
**      This routine traverses a query plan, builds a list of pointers to 
**	updating LDBs (their descriptors) in a temporary stream, passes it
**	to TPF to verify whether they are compatible for the current DX,
**	closes the stream before execution begins.
**
** Inputs:
**	i_dsh_p			    ptr to QEE_DSH
**	v_qer_p			    ptr to QEF_RCB
**
** Outputs:
**	o1_ok_p			    TRUE if OK, FALSE otherwise
**
**	v_qer_p			    ptr to QEF_RCB
**          .error		    One of the following
**		.err_code
**
**      Returns:
**          E_DB_OK
**          E_DB_ERROR              error
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**	14-nov-89 (carl)
**          written
**	02-jun-90 (carl)
**	    changed to test qeq_q3_ctl_info for QEQ_002_USER_UPDATE instead
**	    of qeq_q3_read_b which is obsolete
**	12-sep-90 (carl)
**	    changed to use qec_m1_mopen(), qec_m2_mclose() and qec_m3_malloc() 
**	18-jun-92 (davebf)
**	    Updated for Sybil - removed calls to qecmem.c routines
*/


DB_STATUS
qet_t9_ok_w_ldbs(
QEE_DSH		*i_dsh_p,
QEF_RCB		*v_qer_p,
bool		*o1_ok_p)
{
    DB_STATUS	    status_0 = E_DB_OK,
		    status_t = E_DB_OK,
		    status_u = E_DB_OK;		
    DB_ERROR	    ulm_err,
		    tpf_err;
    QES_DDB_SES	    *dds_p = & v_qer_p->qef_cb->qef_c2_ddb_ses;
    TPR_CB	    tpr_cb,
		    *tpr_p = & tpr_cb;
    QEF_QP_CB	    *qp_p = i_dsh_p->dsh_qp_ptr;
    QEF_AHD	    *act_p = qp_p->qp_ahd;
    QEQ_D1_QRY	    *subqry_p;
    ULM_RCB	    ulm;
    i4		    w_ldbcnt;
    TPR_W_LDB	    *wldb1_p = (TPR_W_LDB *) NULL,
		    *wldb2_p = (TPR_W_LDB *) NULL;


    *o1_ok_p = TRUE;	/* assume */

    if (qp_p->qp_qmode == QEQP_01QM_RETRIEVE)
	return(E_DB_OK);	    /* read-only query */

    /* assume that there are update sites within this query plan */

    /* allocate stream to build list of LDB ptrs; note that this 
    ** stream must be closed upon return from this routine */

    STRUCT_ASSIGN_MACRO(Qef_s_cb->qef_s_ulmcb, ulm);
    ulm.ulm_blocksize = sizeof(TPR_W_LDB);  /* allocation size */

    status_u = qec_mopen(&ulm);
    if (status_u)
    {
	v_qer_p->error.err_code = ulm.ulm_error.err_code;
	return(status_u);
    }

    /* traverse the action list to build an LDB ptr list */

    MEfill(sizeof(tpr_cb), '\0', (PTR) & tpr_cb);
    tpr_p->tpr_session = dds_p->qes_d2_tps_p;	/* TPF session CB ptr */
    tpr_p->tpr_rqf = dds_p->qes_d3_rqs_p;	/* RQF session CB ptr */
    tpr_cb.tpr_15_w_ldb_p = (TPR_W_LDB *) NULL;
    w_ldbcnt = 0;
    act_p = qp_p->qp_ahd; 
    while (act_p != (QEF_AHD *) NULL && status_u == E_DB_OK)
    {
	if (act_p->ahd_atype == QEA_D1_QRY)
	{
	    subqry_p = & act_p->qhd_obj.qhd_d1_qry;

	    if (subqry_p->qeq_q3_ctl_info & QEQ_002_USER_UPDATE)
	    {
		/* an update site */

		w_ldbcnt++;

		ulm.ulm_psize = sizeof(TPR_W_LDB);
		status_u = qec_malloc(&ulm);
		if (status_u)
		{
		    STRUCT_ASSIGN_MACRO(ulm.ulm_error, ulm_err);
		    goto dismantle_9;
		}
		else
		{
		    /* allocation ok */

		    wldb2_p = (TPR_W_LDB *) ulm.ulm_pptr;

		    if (wldb1_p == (TPR_W_LDB *) NULL)
		    {
			/* first in list */

			tpr_cb.tpr_15_w_ldb_p = 
			    wldb1_p = wldb2_p;  
			wldb2_p->tpr_1_prev_p = 
			    wldb2_p->tpr_2_next_p = (TPR_W_LDB *) NULL;
		    }
		    else
		    {
			/* append to list */

			wldb1_p->tpr_2_next_p = wldb2_p;  
			wldb2_p->tpr_1_prev_p = wldb1_p;
			wldb2_p->tpr_2_next_p = (TPR_W_LDB *) NULL;
		    }

		    wldb2_p->tpr_3_ldb_p = subqry_p->qeq_q5_ldb_p;
		}
  	    }
	}
	act_p = act_p->ahd_next;	/* advance */
    }
    /* call TPF if any update sites */

    if (tpr_cb.tpr_15_w_ldb_p != (TPR_W_LDB *) NULL)
    {
	status_t = qed_u17_tpf_call(TPF_OK_W_LDBS, & tpr_cb, v_qer_p);
	if (status_t)
	    STRUCT_ASSIGN_MACRO(tpr_cb.tpr_error, tpf_err);
	else
	    *o1_ok_p = tpr_cb.tpr_14_w_ldbs_ok;
    }

dismantle_9:

    /* must always close stream */

    status_0 = ulm_closestream(&ulm);

    if (status_u)
    {
	/* return previous ulm error */

	STRUCT_ASSIGN_MACRO(ulm_err, v_qer_p->error);
	return(status_u);
    }

    if (status_t)
    {
	/* return tpf error */

	STRUCT_ASSIGN_MACRO(tpf_err, v_qer_p->error);
	return(status_t);
    }

    if (status_0)
    {
	/* return close-stream ulm error */

	STRUCT_ASSIGN_MACRO(ulm.ulm_error, v_qer_p->error);
    }
    
    return(status_0);
}


/*{
** Name: qet_t10_is_ddb_open - call TPF to see if a DDB is open
**
** External QEF call:   status = qef_call(QED_IS_DDB_OPEN, & qef_rcb);
**
** Description:
**	This is called after QED_P1_RECOVER is processed and TPF has created
**	a search DDB list.
**
** Inputs:
**      v_qer_p			ptr to QEF_RCB
**	    .qef_cb		session control block
**		.qef_c2_ddb_sess
**		    .qes_d2_tps_p
**				TPF's session CB ptr
**		    .qes_d3_rqs_p
**				RQF's session CB ptr
**	    .qef_r3_ddb_req		QEF_DDB_REQ
**              .qef_r3_ddb_req		QEF_DDB_REQ
**		    .qer_d1_ddb_p	DD_DDB_DESC ptr
**			.dd_d1_ddb_name	DDB name
**
** Outputs:
**      v_qer_p			ptr to QEF_RCB
**	    .qef_r3_ddb_req		QEF_DDB_REQ
**              .qef_r3_ddb_req		QEF_DDB_REQ
**		    .qer_d13_ctl_info
**					QEF_04DD_DDB_OPEN ON if DDB is open,
**					OFF if DDB is under recovery
**
**      Returns:
**          DB_STATUS
**
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**	11-nov-89 (carl)
**	    created
*/


DB_STATUS
qet_t10_is_ddb_open(
QEF_RCB         *v_qer_p)
{
    DB_STATUS	    status= E_DB_OK;
    QEF_DDB_REQ	    *ddr_p = & v_qer_p->qef_r3_ddb_req;
    QES_DDB_SES	    *dds_p = & v_qer_p->qef_cb->qef_c2_ddb_ses;
    TPR_CB	    tpr_cb,
		    *tpr_p = & tpr_cb;

    ddr_p->qer_d13_ctl_info &= ~QEF_05DD_DDB_OPEN;   /* assume NOT open */

    MEfill(sizeof(tpr_cb), '\0', (PTR) & tpr_cb);
    tpr_p->tpr_session = dds_p->qes_d2_tps_p;	/* TPF session CB ptr */
    tpr_p->tpr_rqf = dds_p->qes_d3_rqs_p;	/* RQF session CB ptr */
    tpr_p->tpr_16_ddb_p = v_qer_p->qef_r3_ddb_req.qer_d1_ddb_p;;

    status = qed_u17_tpf_call(TPF_IS_DDB_OPEN, tpr_p, v_qer_p);
    if (status == E_DB_OK)
    {
	if (tpr_p->tpr_17_ddb_open == TRUE)    /* DDB NOT in recovery */
	    ddr_p->qer_d13_ctl_info |= QEF_05DD_DDB_OPEN;   
    }
    return(status);
}

/*{
** Name: qet_t11_ok_w_ldb - request TPF to validate the ldb for a write query
**
** Description:
**	(See above comment.)
**
** Inputs:
**	i1_ldb_p		    ptr to LDB descritpor
**	v_qer_p			    ptr to QEF_RCB
**
** Outputs:
**	o1_ok_p			    TRUE if OK, FALSE otherwise
**
**	v_qer_p			    ptr to QEF_RCB
**          .error
**		.err_code
**      Returns:
**          E_DB_OK
**          E_DB_ERROR              error
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**	15-mar-90 (carl)
**          written
*/


DB_STATUS
qet_t11_ok_w_ldb(
DD_LDB_DESC	*i1_ldb_p,
QEF_RCB		*v_qer_p,
bool		*o1_ok_p)
{
    DB_STATUS	    status = E_DB_OK;
    QES_DDB_SES	    *dds_p = & v_qer_p->qef_cb->qef_c2_ddb_ses;
    TPR_CB	    tpr_cb;
    TPR_W_LDB	    wldb,
		    *wldb_p = & wldb;


    MEfill(sizeof(tpr_cb), '\0', (PTR) & tpr_cb);
    tpr_cb.tpr_session = dds_p->qes_d2_tps_p;	/* TPF session CB ptr */
    tpr_cb.tpr_rqf = dds_p->qes_d3_rqs_p;	/* RQF session CB ptr */
    tpr_cb.tpr_15_w_ldb_p = wldb_p;

    wldb_p->tpr_3_ldb_p = i1_ldb_p;
    wldb_p->tpr_1_prev_p = 
	wldb_p->tpr_2_next_p = (TPR_W_LDB *) NULL;

    status = qed_u17_tpf_call(TPF_OK_W_LDBS, & tpr_cb, v_qer_p);
    if (status == E_DB_OK)
	*o1_ok_p = tpr_cb.tpr_14_w_ldbs_ok;
    return(status);
}


/*{
** Name: qet_t20_recover - call TPF to recover all DDBs in the installation
**
** External QEF call:   status = qef_call(QED_RECOVER, & qef_rcb);
**
** Description:
**	This is called immediately after a server starts up to attempt to
**	recover all the DDBs in the current installation as defined by
**	iistar_cdbs in iidbdb.
**
**	TPF is called to do the hard work.
**
** Inputs:
**      v_qer_p			ptr to QEF_RCB
**	    .qef_cb		session control block
**		.qef_c2_ddb_sess
**		    .qes_d2_tps_p
**				TPF's session CB ptr
**		    .qes_d3_rqs_p
**				RQF's session CB ptr
**	    .qef_r3_ddb_req		QEF_DDB_REQ
**		.qer_d2_ldb_info_p	DD_LDB_DESC ptr
**		    .dd_l1_ingres_b	FALSE
**		    .dd_l2_node_name    blanks for default
**		    .dd_l3_ldb_name	"iidbdb"
**		    .dd_l4_dbms_name    INGRES
**		    .dd_l5_dbms_code    DD_1DBMS_INGRES (1)
**		    .dd_l6_ldb_id	DD_UNASSIGNED_IS_0
**
** Outputs:
**      none
**
**      Returns:
**          DB_STATUS
**
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**	11-nov-89 (carl)
**	    created
*/


DB_STATUS
qet_t20_recover(
QEF_RCB         *v_qer_p)
{
    DB_STATUS	    status= E_DB_OK;
    QES_DDB_SES	    *dds_p = & v_qer_p->qef_cb->qef_c2_ddb_ses;
    DD_LDB_DESC	    *iidbdb_p =  & v_qer_p->qef_r3_ddb_req.qer_d2_ldb_info_p->
			dd_i1_ldb_desc;
    TPR_CB	    tpr_cb,
		    *tpr_p = & tpr_cb;


    MEfill(sizeof(tpr_cb), '\0', (PTR) & tpr_cb);
    tpr_p->tpr_session = dds_p->qes_d2_tps_p;	/* TPF session CB ptr */
    tpr_p->tpr_rqf = dds_p->qes_d3_rqs_p;	/* RQF session CB ptr */
    tpr_p->tpr_site = iidbdb_p;
						/* iidbdb desc */
    status = qed_u17_tpf_call(TPF_RECOVER, tpr_p, v_qer_p);

    return(status);
}

