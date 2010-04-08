/*
**Copyright (c) 2004 Ingres Corporation
*/
#include    <compat.h>
#include    <gl.h>
#include    <me.h>
#include    <st.h>
#include    <sl.h>
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

#include    <ade.h>
#include    <dudbms.h>
#include    <dmmcb.h>
#include    <dmucb.h>
#include    <ex.h>
#include    <tm.h>
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
**  Name: QEUT.C - transaction utility functions for QEF
**
**  Description:
**      These are the external entry points for functions that
**	provide query-like and transaction dependent operations.
**
**	    qeu_btran	- begin an internal/user transaction as necessary.
**	    qeu_etran	- end an internal transaction
**	    qeu_atran	- abort an internal transaction.
**
**
**
**
**
**	    - There are 5 states of transaction within QEF
**	       QEF_NOTRAN   - no transaction active
**	       QEF_ITRAN    - an internal, read only transaction is active.
**			      This transaction is started by SCF (through
**			      qeu_btran with qeu_flag = QEF_READONLY) during
**			      query parsing and compilation.  The transaction
**			      will be committed through qeu_etran or qeu_atran
**			      when the query parsing and compliation is 
**			      terminated.
**	       QEF_SSTRAN   - single statement transaction.  It's started up
**			      by an explicit call (from SCF) to qet_begin, or
**			      through a qeu_btran when a qef_auto(commit) is
**			      on and qeu_flag is not QEF_READONLY.
**	       QEF_MSTRAN   - multiple statement transaction.  It's started up
**			      by an explicit call (from SCF) to qet_begin, or
**			      through a qeu_btran when a qef_auto(commit) is
**			      off and qeu_flag is not QEF_READONLY.
**	       QEF_RO_MSTRAN- an MST transaction  that temporary behaves as 
**			      read-only.  This is the case when the current
**			      transaction is an MST and SCF wants to start
**			      an read-only transaction for query parsing and
**			      compilation.  This special transaction state lets
**			      us know that savepointing is not necessary.
**
**
**
**
**  History:    $Log-for RCS$
**      30-apr-86 (daved)    
**          written
**	12-may-88 (puree)
**	    Modify qeu_etran and qeu_atran to call qee_cleanup.
**	03-aug-88 (puree)
**	    Modified the transaction handling as follows:
**	02-sep-88 (carl)
**	    modified qeu_btran, qeu_etran and qeu_atran for Titan
**	06-jun-89 (carl)
**	    modified qeu_etran and qeu_atran to precommit catalog locks
**	15-jun-92 (davebf)
**	    Sybil Merge
**	08-dec-92 (anitap)
**	    Added #include <qsf.h> and <psfparse.h> for CREATE SCHEMA.
**	31-mar-93 (anitap)
**	    Added a check in qeu_etran for open cursors in a MST.
**	2-Aug-93 (rickh)
**	    Applied the 31-Mar-93 fix to Abort Transaction.  If we encounter
**	    an error in the middle of an execute immediate statement (say
**	    supporting a CONSTRAINT), this will ensure that we return to
**	    the QEF code that cooked up the statement so that we can finish
**	    our cleanup there.
**	14-nov-95 (kch)
**	    In qeu_etran(), test to see if the current query mode,
**	    qeu_cb->qeu_qmode, is a copy (PSQ_COPY). If we are doing a copy
**	    and autocommit is on (qef_cb->qef_stat == QEF_SSTRAN) we now do
**	    not call qet_commit(). Previously, if qet_commit() was called,
**	    the TCBs would be released by dm2t_destroy_temp_tcb(), and copy
**	    map processing would fail during the redemption of the large
**	    object null indicator values. This change fixes bug 72504.
**	07-Aug-1997 (jenjo02)
**	    Changed *dsh to **dsh in qee_cleanup(), qee_destroy() prototypes.
**	    Pointer will be set to null if DSH is deallocated.
**	04-jan-2000 (rigka01) bug #99199
**	    in qeu_btran(),
**	    check qeu_flag value for an indication of a database procedure
**	    and set qeu_modifier accordingly.  the result is that a transaction
**	    is not open before the first statement of a db procedure has been
**	    executed.
**	10-Jan-2001 (jenjo02)
**	    Delted qef_dmf_id, use qef_sess_id instead.
**/



/*{
** Name: QEU_BTRAN      - begin an internal/user transaction
**
** External QEF call:   status = qef_call(QEU_BTRAN, &qeu_cb);
**
** Description:
**	If there is no current transaction, start one.  If the qeu_flag
**	is QEF_READONLY, start a readonly internal transaction.  Otherwise
**	start SST or MST, depending on the current setting of qef_auto(commit)
**      flag in the session qef_cb.
**
**	If the current transaction is MST and the qeu_flag is QEF_READONLY, 
**	convert it to MST_RO (read-only, multi-statement) transaction.
**	This allows us not to savepoint/abort when the internal transaction
**	ends or aborts (to savepoint).
**
** Inputs:
**	qeu_cb
**	    .qeu_eflag	     designate error handling semantis
**			     for user errors.
**		QEF_INTERNAL return error code.
**		QEF_EXTERNAL send message to user.
**	    .qeu_db_id       a dmf database id 
**	    .qeu_d_id        a dmf session id
**	    .qeu_flag	     transaction modifier with the values below:
**				QEF_READONLY - for a read only transaction.
**				0 otherwise.
**
** Outputs:
**      qeu_cb
**          .error.err_code         One of the following:
**                                  E_QE0000_OK
**                                  E_QE0017_BAD_CB
**                                  E_QE0019_NON_INTERNAL_FAIL
**                                  E_QE0002_INTERNAL_ERROR
**	    .qeu_tran_id	    Tran ID of transaction
**      Returns:
**          E_DB_OK                 
**          E_DB_ERROR              caller error
**          E_DB_FATAL              internal error
**      Exceptions:
**          none
**
** Side Effects:
**      none
**
** History:
**      27-may-86 (daved)
**          written
**	3-feb-87 (daved)
**	    if not autocommit, begin MST
**	03-aug-88 (puree)
**	    Modified the transaction handling as described in the 
**	    description section above.
**	24-aug-88 (puree)
**	    add QEF_RO_SSTRAN transaction state.  This is used during
**	    query translation and compilation only.
**	02-sep-88 (carl)
**	    modified to ignore internal operation for Titan
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	11-feb-99 (stephenb)
**	    Set tran id in qeu_cb so that we can pass it back to SCF.
**	04-jan-2000 (rigka01) bug #99199
**	    check qeu_flag value for an indication of a database procedure
**	    and set qeu_modifier accordingly.  the result is that a transaction
**	    is not open before the first statement of a db procedure has been
**	    executed.
*/
DB_STATUS
qeu_btran(
QEF_CB	    *qef_cb,
QEU_CB      *qeu_cb)
{
    QEF_RCB	*oqef_rcb;
    DB_STATUS	status = E_DB_OK;
    QEF_RCB	qef_rcb;

    /* ignore if distributed session */
    if (qef_cb->qef_c1_distrib & DB_3_DDB_SESS)
	return(E_DB_OK);

    oqef_rcb = qef_cb->qef_rcb;
    qef_cb->qef_rcb = &qef_rcb;
    qef_rcb.qef_eflag = qeu_cb->qeu_eflag;

    switch (qef_cb->qef_stat)
    {
	case QEF_SSTRAN:
	    if (qeu_cb->qeu_flag == QEF_READONLY)
		qef_cb->qef_stat = QEF_RO_SSTRAN;
	    status = E_DB_OK;
	    break;

	case QEF_NOTRAN:
	    /* begin a readonly internal transaction if READONLY flag is set.
	    ** otherwise set the modifier to SST.  qet_begin will escalate it 
	    ** to MST if the qef_auto(commit) is off */
	    if (qeu_cb->qeu_flag == QEF_READONLY)
		qef_rcb.qef_modifier = QEF_ITRAN;
	    else
	    /* if flag indicates dbprocedure is the source, set qef_modifier 
	    ** to same value as if flag indicated QEF_READONLY and then 
	    ** reset the flag
	    */ 	 
	    if (qeu_cb->qeu_flag == QEF_DBPROC)
	    {		
		qef_rcb.qef_modifier = QEF_ITRAN;
		qeu_cb->qeu_flag = 0;
	    }
	    else
		qef_rcb.qef_modifier = QEF_SSTRAN; 

	    qef_rcb.qef_flag        = qeu_cb->qeu_flag;
	    qef_rcb.qef_sess_id     = qeu_cb->qeu_d_id;
	    qef_rcb.qef_db_id       = qeu_cb->qeu_db_id;
	    qef_rcb.qef_cb		= qef_cb;
	    status = qet_begin(qef_cb);
	    break;

	case QEF_MSTRAN:
	    if (qeu_cb->qeu_flag == QEF_READONLY)
		qef_cb->qef_stat = QEF_RO_MSTRAN;
	    status = E_DB_OK;
	    break;

	default:
	    status = E_DB_OK;
	    break;
    }
    if (status != E_DB_OK)
	STRUCT_ASSIGN_MACRO(qef_rcb.error, qeu_cb->error);
    else
	qeu_cb->error.err_code = E_QE0000_OK;

    /* restore rcb pointer */
    qef_cb->qef_rcb = oqef_rcb;
    /* set tran id */
    STRUCT_ASSIGN_MACRO(qef_cb->qef_tran_id, qeu_cb->qeu_tran_id);

    return (status);
}


/*{
** Name: QEU_ETRAN      - end an internal transaction
**
** External QEF call:   status = qef_call(QEU_ETRAN, &qeu_cb);
**
** Description:
**	If the current transaction is QEF_ITRAN or QEF_SSTRAN, commit it.
**      If it's MST with no cursor opened, declare a savepoint. If it's 
**	a read only MST, convert it back to MST.
**
** Inputs:
**	qeu_cb
**	    .qeu_eflag	     designate error handling semantis
**			     for user errors.
**		QEF_INTERNAL return error code.
**		QEF_EXTERNAL send message to user.
**
** Outputs:
**      qeu_cb
**          .error.err_code         One of the following:
**                                  E_QE0000_OK
**                                  E_QE0017_BAD_CB
**                                  E_QE0019_NON_INTERNAL_FAIL
**                                  E_QE0002_INTERNAL_ERROR
**      Returns:
**          E_DB_OK                 
**          E_DB_ERROR              caller error
**          E_DB_FATAL              internal error
**      Exceptions:
**          none
**
** Side Effects:
**      none
**
** History:
**      27-may-86 (daved)
**          written
**	24-jun-86 (daved)
**	    don't commit internal transactions with open cursors.
**	    rely upon the caller to close all cursors.
**	12-may-88 (puree)
**	    Also call qee_cleanup to clean up the environment.
**	    The call to qee_cleanup in qet_abort has been removed.
**	03-aug-88 (puree)
**	    Modified the transaction handling as described in the 
**	    description section above.
**	02-sep-88 (carl)
**	    modified to ignore internal operation for Titan
**	06-jun-89 (carl)
**	    modified qeu_etran and qeu_atran to precommit catalog locks.
**	31-mar-93 (anitap)
**	    Modified qeu_etran() not to destory the dsh when the cursor is 
**	    open in a MST.
**	14-nov-95 (kch)
**	    Test to see if the current query mode, qeu_cb->qeu_qmode, is a
**	    copy (PSQ_COPY). If we are doing a copy and autocommit is on
**	    (qef_cb->qef_stat == QEF_SSTRAN) we now do not call qet_commit().
**	    Previously, if qet_commit() was called, the TCBs would be
**	    released by dm2t_destroy_temp_tcb(), and copy map processing
**	    would fail during the redemption of the large object null
**	    indicator values. This change fixes bug 72504.
*/
DB_STATUS
qeu_etran(
QEF_CB         *qef_cb,
QEU_CB		*qeu_cb)
{
    DB_STATUS	status = E_DB_OK;
    QEF_RCB	*oqef_rcb;
    QEF_RCB	qef_rcb;	/* temporary qef_rcb for lower levels */
    DB_SP_NAME	spoint;

    qeu_cb->error.err_code = E_QE0000_OK;   /* assume */
    oqef_rcb = qef_cb->qef_rcb;

    if (qef_cb->qef_c1_distrib & DB_3_DDB_SESS)  /* if distributed session */
    {
	/* precommit RDF's catalog locks if any */

	qef_rcb.qef_cb = qef_cb;
	status = qed_u14_pre_commit(& qef_rcb);
	if (status)
	    STRUCT_ASSIGN_MACRO(qef_rcb.error, qeu_cb->error);

	/* restore the rcb pointer */

	qef_cb->qef_rcb = oqef_rcb;
	return(status);
    }


    qef_cb->qef_rcb = &qef_rcb;
    qef_rcb.qef_cb  = qef_cb;

    switch (qef_cb->qef_stat)
    {
	case QEF_NOTRAN:
	    status = E_DB_OK;
	    break;

	case QEF_ITRAN:
	    status = qet_commit(qef_cb);
	    break;

	case QEF_RO_SSTRAN:
	    qef_cb->qef_stat = QEF_SSTRAN;
	    status = E_DB_OK;
	    break;

	case QEF_SSTRAN:
	    /* Don't do anything if there is an open cursor */
	    if (qef_cb->qef_open_count)
	    {
		status = E_DB_OK;
		break;
	    }
	    
	    /* clean up DSH structures, if any */
	    status = qee_cleanup(&qef_rcb, (QEE_DSH **)NULL);
	    if (status != E_DB_OK)
		break;
	    /* Bug 72504 */
	    if (qeu_cb->qeu_qmode != PSQ_COPY)
	    status = qet_commit(qef_cb);
	    else
	    status = E_DB_OK;
	    break;

	case QEF_RO_MSTRAN:
	    qef_cb->qef_stat = QEF_MSTRAN;
	    status = E_DB_OK;
	    break;

	case QEF_MSTRAN:
	    /* Don't do anything if there is an open cursor */
	    if (qef_cb->qef_open_count)
	    {
		status = E_DB_OK;
		break;
	    }

	    /* clean up DSH structures, if any */
	    status = qee_cleanup(&qef_rcb, (QEE_DSH **)NULL);
	    if (status != E_DB_OK)
		break;

	    qef_rcb.qef_spoint = &spoint;
	    MEmove(QEF_PS_SZ, (PTR) QEF_SP_SAVEPOINT,
	      ' ', sizeof(DB_SP_NAME), (PTR) qef_rcb.qef_spoint->db_sp_name);
	    status = qet_savepoint(qef_cb);
	    break;
    }
    if (status != E_DB_OK)
	STRUCT_ASSIGN_MACRO(qef_rcb.error, qeu_cb->error);
    else
        qeu_cb->error.err_code = E_QE0000_OK;

    /* restore rcb pointer */
    qef_cb->qef_rcb = oqef_rcb;
    return (status);
}


/*{
** Name: QEU_ATRAN      - abort an internal transaction
**
** External QEF call:   status = qef_call(QEU_ATRAN, &qeu_cb);
**
** Description:
**	This module is called from SCF, PSF and internal QEF.  If the current
**	transaction state is QEF_ITRAN, the transaction is started by SCF
**	for query parsing and compilation.  In this case, there are no updates
**	to the DB and we can simply commit (end) the transaction since commit
**	incurs less DMF overhead than abort.
**
**	If the current transaction state is QEF_SSTRAN, abort the transaction.
**	If it is QEF_MSTRAN, abort to the internal savepoint. If it's a read
**	only MST, convert it to MST.
**	
**
** Inputs:
**	qeu_cb
**	    .qeu_eflag		    designate error handling semantis
**				    for user errors.
**		QEF_INTERNAL	    return error code.
**		QEF_EXTERNAL	    send message to user.
**
** Outputs:
**      qeu_cb
**          .error.err_code         One of the following:
**                                  E_QE0000_OK
**                                  E_QE0017_BAD_CB
**                                  E_QE0019_NON_INTERNAL_FAIL
**                                  E_QE0002_INTERNAL_ERROR
**      Returns:
**          E_DB_OK                 
**          E_DB_ERROR              caller error
**          E_DB_FATAL              internal error
**      Exceptions:
**          none
**
** Side Effects:
**      none
**
** History:
**	17-jul-86 (daved)
**	    written
**	12-may-88 (puree)
**	    Also call qee_cleanup to clean up the environment.
**	    The call to qee_cleanup in qet_abort has been removed.
**	03-aug-88 (puree)
**	    Modified the transaction handling as described in the description
**	    section above.
**	06-jun-89 (carl)
**	    modified qeu_etran and qeu_atran to precommit catalog locks
**	2-Aug-93 (rickh)
**	    Applied the 31-Mar-93 fix to Abort Transaction.  If we encounter
**	    an error in the middle of an execute immediate statement (say
**	    supporting a CONSTRAINT), this will ensure that we return to
**	    the QEF code that cooked up the statement so that we can finish
**	    our cleanup there.
*/
DB_STATUS
qeu_atran(
QEF_CB         *qef_cb,
QEU_CB		*qeu_cb)
{
    DB_STATUS	status = E_DB_OK;
    QEF_RCB	*oqef_rcb;
    QEF_RCB	qef_rcb;	/* temporary qef_rcb for lower levels */
    DB_SP_NAME	spoint;


    qef_rcb.qef_cb = qef_cb;
    oqef_rcb = qef_cb->qef_rcb;		/* save the original rcb away */

    if (qef_cb->qef_c1_distrib & DB_3_DDB_SESS)  /* if distributed session */
    {
	/* precommit RDF's catalog locks if any */

	qef_rcb.qef_cb = qef_cb;
	status = qed_u14_pre_commit(& qef_rcb);
	if (status)
	    STRUCT_ASSIGN_MACRO(qef_rcb.error, qeu_cb->error);

	/* restore the rcb pointer */

	qef_cb->qef_rcb = oqef_rcb;
	return(status);
    }

    qef_cb->qef_rcb = &qef_rcb;
    qef_rcb.qef_eflag = qeu_cb->qeu_eflag;


    switch (qef_cb->qef_stat)
    {
	case QEF_NOTRAN:
	    status = E_DB_OK;
	    break;

	case QEF_ITRAN:
	    status = qet_commit(qef_cb);
	    break;

	case QEF_RO_SSTRAN:
	    qef_cb->qef_stat = QEF_SSTRAN;
	    status = E_DB_OK;
	    break;

	case QEF_SSTRAN:
	    /*
	    ** Don't do anything if there is an open cursor
	    **
	    ** If we encountered an error in the middle of an execute
	    ** immediate statement, we don't want to release the DSH.
	    ** Instead, we want to return to the QEF code that cooked up
	    ** the execute immediate statement so that it can finish
	    ** its cleanup.  After that, we can abort for certain.
	    */
	    if (qef_cb->qef_open_count)
	    {
		status = E_DB_OK;
		break;
	    }

	    /* Clean up DSH structures, if any */
	    (VOID)qee_cleanup(&qef_rcb, (QEE_DSH **)NULL);

	    /* abort the transaction */
	    qef_rcb.qef_spoint = (DB_SP_NAME *)NULL;
	    status = qet_abort(qef_cb);
	    break;

	case QEF_RO_MSTRAN:
	    qef_cb->qef_stat = QEF_MSTRAN;
	    status = E_DB_OK;
	    break;

	case QEF_MSTRAN:
	    /*
	    ** Don't do anything if there is an open cursor
	    **
	    ** If we encountered an error in the middle of an execute
	    ** immediate statement, we don't want to release the DSH.
	    ** Instead, we want to return to the QEF code that cooked up
	    ** the execute immediate statement so that it can finish
	    ** its cleanup.  After that, we can abort for certain.
	    */
	    if (qef_cb->qef_open_count)
	    {
		status = E_DB_OK;
		break;
	    }

	    /* Clean up DSH structures, if any */
	    (VOID)qee_cleanup(&qef_rcb, (QEE_DSH **)NULL);

	    /* abort to the last internal savepoint */
	    qef_rcb.qef_spoint = &spoint;
	    MEmove(QEF_PS_SZ, (PTR) QEF_SP_SAVEPOINT,
	      ' ', sizeof(DB_SP_NAME), (PTR) qef_rcb.qef_spoint->db_sp_name);
	    status = qet_abort(qef_cb);
	    break;
    }
    if (status != E_DB_OK)
	STRUCT_ASSIGN_MACRO(qef_rcb.error, qeu_cb->error);
    else
	qeu_cb->error.err_code = E_QE0000_OK;

    /* restore the rcb pointer */
    qef_cb->qef_rcb = oqef_rcb;
    return (status);
}
