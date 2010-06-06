/*
**Copyright (c) 2010 Ingres Corporation
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
#include    <dmxcb.h>
#include    <qefmain.h>
#include    <qsf.h>
#include    <qefrcb.h>
#include    <qefcb.h>
#include    <qefscb.h>

#include    <ade.h>
#include    <dudbms.h>
#include    <dmmcb.h>
#include    <dmucb.h>
#include    <dmtcb.h>
#include    <dmrcb.h>
#include    <ex.h>
#include    <qefnode.h>
#include    <psfparse.h>
#include    <qefact.h>
#include    <qefqp.h>
#include    <qefdsh.h>
#include    <qefqeu.h>
#include    <qeuqcb.h>
#include    <rqf.h>
#include    <tpf.h>
#include    <qefkon.h>
#include    <qefqry.h>
#include    <qefcat.h>
#include    <tm.h>
#include    <sxf.h>
#include    <qefprotos.h>
/**
**
**  Name: QET.C - This file provides the transaction management routines
**
**  Description:
**      The routines in this file allow the caller to maintain control
**  over the state of user defined transactions. We must first understand
**  that some transactions are initiated by the user (ie begin transaction)
**  and other transactions are initiated by QEF (ie open cursor when no
**  transaction is in progress). There are also externally defined savepoints
**  and internally defined savepoints (to allow user interrupts). 
**
**  External savepoints are those specified by the user in order to allow
**  rollback of sets up updates without releasing the entire transaction.
**
**  Internal savepoints are used by the DBMS for error recovery protocols.
**  Internal savepoints are indicated by a savepoint name which is preceeded
**  by a "$" character.  The only currently used internal savepoint is the
**  QEF per-statement savepoint - defined by QEF_SP_SAVEPOINT.  It is used
**  by qef to partition the transaction work done by different statements
**  within multi-statement transactions.
**
**  The use of per-statement savepoints is affected by the session's ON_ERROR
**  setting: ROLLBACK STATEMENT or ROLLBACK TRANSACTION (the default setting
**  for Ingres sessions is 'rollback statement').
**
**  When a session runs in default ON_ERROR ROLLBACK STATEMENT mode, qef
**  will lay down internal QEF_SP_SAVEPOINT savepoints after each statment
**  of an MST.  This marks the spot to which the dbms should roll-back the
**  transaction should an error occur on the next statement.
**
**  When a session runs in ON_ERROR ROLLBACK TRANSACTION mode, qef will not
**  lay down internal QEF_SP_SAVEPOINT savepoints.  A statement error which
**  requires rollback processing will abort then entire transaction rather
**  than attempting to rollback a single statement.  Any request to write
**  a per-statement savepoints will be silently ignored by the qet_savepoint
**  routine.  Any request to abort to a per-statement savepoint will instead
**  cause the entire transaction to be aborted.
**
**  If new dbms query processing protocols require the use of internal
**  savepoints in non user-error situations (for example: as a way to do
**  automatic retry on some special operations), then those operations should
**  use a different internal savepoint than the per-statement QEF_SP_SAVEPOINT
**  so that these routines will not ignore it when using ON_ERROR ROLLBACK
**  TRANSACTION mode.
**
**	    qet_autocommit - auto commit on | off
**          qet_abort    - abort to savepoint
**          qet_begin    - begin a transaction
**          qet_commit   - commit and end transaction
**          qet_savepoint- set a savepoint
**          qet_secure - Secure a transaction to end the first phase of a 
**			 distributed transaction.
**          qet_resume- Resume a disconnected slave transaction.
**
**  History:
**      30-apr-86 (daved)    
**          written
**	26-aug-87 (puree)
**	    modify qet_abort.
**	09-sep-87 (puree)
**	    modify qet_savepoint.
**	14-oct-87 (puree)
**	    let qet_abort handle all errors due to declaring an internal
**	    savepoint in qet_savepoint.
**	11-nov-87 (puree)
**	    made errors from dms_abort fatal errors as returned by DMF.
**	22-dec-87 (puree)
**	    modified qet_savepoint to issue user's message if trying to
**	    declare a savepoint while autocommit is on.
**	22-dec-87 (puree)
**	    fix qet_abort and qet_commit to report error from qee_cleanup.
**	20-jan-88 (puree)
**	    remove qet_escalate.  All calls to qet_escalate have been
**	    converted to in-line code.
**	28-jan-88 (puree)
**	    modified qet_abort and qet_commit to ensure that qee_cleanup
**	    is called in any event.
**	19-feb-88 (puree)
**	    modify qet_abort not to complain about no transaction during
**	    a rollback statement.
**	28-aug-88 (carl)
**	    modified for Titan
**	25-dec-88 (carl)
**	    more changes for Titan
**	08-Mar-89 (ac)
**	    Added 2PC support.
**	22-may-89 (neil)
**	    Return E_DB_ERROR from all local routinesnes if in wrong xact state.
**	20-may-90 (carl)
**	    fixed abort bug in qet_abort
**	25-feb-91 (rogerk)
**	    Added support for settable error handling on statement errors.
**	    When qef_stm_error specifies QEF_XACT_ROLLBACK, then bypass
**	    internal savepoints and abort the entire transaction on an error.
**	    When qef_stm_error specifies QEF_SVPT_ROLLBACK, then bypass
**	    internal savepoints and abort to the user defined savepoint.
**	13-aug-91 (davebf)
**	    reset QEF_NOTRAN if aborting after failure to secure (b37778)
**	15-jun-92 (davebf)
**	    Sybil Merge
**	08-dec-92 (anitap)
**	    Added #include <qsf.h> for CREATE SCHEMA.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	08-Nov-1993 (fpang)
**	    In qet_commit(), the wrong parameter was passed to qef_error() 
**	    when a distributed commit failed. 
**	    Fixes B56673, (Sometimes Star server SEGV's if commit fails).
**      04-apr-1995 (dilma04) 
**          Set dmx_option to DMX_USER_TRAN unless QEF_ITRAN is set.
**	07-Mar-2000 (jenjo02)
**	    DMX_ABORT may override abort-to-savepoint if transaction is
**	    being FORCE_ABORTED. E_DB_WARN signifies this.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	10-Jan-2001 (jenjo02)
**	    Deleted qef_dmf_id, use qef_sess_id instead.
**      22-oct-2002 (stial01)
**          Added qet_xa_abort
**	30-mar-04 (toumi01)
**	    move qefdsh.h below qefact.h for QEF_VALID definition
**      01-jun-05 (stial01)
**          qet_xa_abort() Init dmx_db_id
**	26-May-2010 (kschendel) b123814
**	    Call QSF at commit time to flush the uncommitted-objects list.
**	    Delete some 20 year old comment headers with no code.
**/


/*{
** Name: QET_ABORT      - abort to savepoint
**
** External QEF call:   status = qef_call(QET_ABORT, &qef_rcb);
**
** Description:
**      A transaction is aborted to a savepoint or to the
** beginning of the transaction (at which point the transaction is
** closed). It is, of course, an error for a transaction to not
** be in progress or for the savepoint not to exist. 
**
** Inputs:
**      qef_rcb
**	    .qef_cb		session control block
**		.qef_abort	if true, always abort transaction (not to
**				savepoint).
**	    .qef_eflag		designate error handling semantis
**				for user errors.
**		QEF_INTERNAL	return error code.
**		QEF_EXTERNAL	send message to user.
**          .qef_spoint         name of user defined savepoint
**                              A null string or zero value 
**                              indicates abort to beginning of 
**                              transaction and then end
**                              transaction.
**	    .qef_modifier	type of transaction we should abort
**		QEF_MSTRAN
**		QEF_SSTRAN
**
** Outputs:
**      qef_rcb
**          .error.err_code         One of the following:
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
**      29-apr-86 (daved)
**          written
**	21-jul-87 (daved)
**	    if qef_abort is set, ignore savepoint requests and
**	    always abort transaction.
**	25-aug-87 (puree)
**	    if aborting to user's savepoint failed, do not abort
**	    the transaction, just log the error message.
**	11-nov-87 (puree)
**	    made errors from dms_abort fatal errors as returned by DMF.
**	22-dec-87 (puree)
**	    when abort to savepoint fails, check for QET_ROLLBACK as well
**	    as QET_ABORT to determine if it is a user's savepoint or not.
**	22-dec-87 (puree)
**	    report error from qee_clean up since qee_xxxx will never report
**	    any error.
**	29-jan-88 (puree)
**	    always calll qee_cleanup to make sure that all resources are
**	    released.
**	19-feb-88 (puree)
**	    cannot complain about no transaction during a rollback statement.
**	28-aug-88 (carl)
**	    modified for Titan 
**	25-dec-88 (carl)
**	    more changes for Titan
**	22-may-89 (neil)
**	    Return E_DB_ERROR if in wrong xact state.
**	25-feb-91 (rogerk)
**	    Added support for settable error handling on statement errors.
**	    When qef_stm_error specifies QEF_XACT_ROLLBACK, then bypass
**	    internal savepoints and abort the entire transaction on an error.
**	    Also, don't lay down internal savepoints following abort calls.
**	    When qef_stm_error specifies QEF_SVPT_ROLLBACK, then abort to
**	    user defined savepoint rather than internal one.
**	18-apr-91 (fpang)
**	    If qet_t1_abort()  returns with E_DB_FATAL, return E_DB_FATAL
**	    not E_DB_ERROR.. Something is probably seriously wrong in rqf/tpf.
**	31-jan-95 (forky01)
**	    When user specifies ROLLBACK to SAVEPOINT and the savepoint name
**	    specified does not exist, then abort the rollback so that cursors
**	    remain open and the system state doesn't change to think there
**	    is no current number of transactions active.  This is to fix
**	    bugs 66552 & 41092.
**	29-dec-1997 (nanpr01)
**	    Hitting cntrl-C in a nologging transaction is not marking
**	    the database inconsistent.
**	18-may-1998 (nanpr01)
**	    Willing Commit transactions are getting aborted instead of 
**	    being in Orphaned state. This is due to  fix of bug 87609.
**	07-Mar-2000 (jenjo02)
**	    DMX_ABORT may override abort-to-savepoint if transaction is
**	    being FORCE_ABORTED. E_DB_WARN signifies this.
**	20-Apr-2004 (jenjo02)
**	    If transaction context is gone after abort,
**	    set QEF_NOTRAN.
**      18-Feb-2005 (hanal04) Bug 112376 INGSRV2837
**          Flush QSF cache objects ownerd by this session to prevent
**          invalid cache objects being left in QSF cache after the abort.
**	18-Jul-2006 (kschendel)
**	    Keith's fix in qet_commit has to be done here too:
**	    Decrement qef_open_count rather than set to zero if we are in a
**	    nested DBP (qef_context_cnt > 1). This prevents DSH cleanup -
**	    qee_cleanup() called from qeu_etran() - and failure of
**	    subsequent DBP calls. This change fixes bug 102123.
**	22-Apr-2010 (kschendel) SIR 123485
**	    Use padded copy of standard savepoint name in the QEF SCB.
*/
DB_STATUS
qet_abort(
QEF_CB         *qef_cb)
{
    i4     err;
    bool	save_point  =  FALSE;
    bool	internal_savepoint  =  FALSE;
    DB_STATUS   status, local_status;
    DMX_CB      dmx_cb;             /* dmf transaction control block */
    DB_SP_NAME	*sp_name;
    bool	ddb_b;
    QSF_RCB     qsf_rb;

 
    if (qef_cb->qef_c1_distrib & DB_3_DDB_SESS)
	ddb_b = TRUE;
    else
	ddb_b = FALSE;

    /*
    ** make sure that this is a legal QEF call.  Note "ROLLBACK WORK"
    ** does not issue an error message if there is no transaction.  However,
    ** "ROLLBACK" to savepoint will issue an invalid savepoint error.
    */ 

    if (qef_cb->qef_stat == QEF_NOTRAN ||
	    (qef_cb->qef_operation == QET_ABORT &&
		    qef_cb->qef_stat != qef_cb->qef_rcb->qef_modifier))
    {
	if (qef_cb->qef_rcb->qef_operation == QET_ABORT)
	{
	    qef_error(E_QE0004_NO_TRANSACTION, 0L, E_DB_ERROR, &err,
		&qef_cb->qef_rcb->error, 0);
	    return (E_DB_ERROR);
	}
	
	if (qef_cb->qef_operation == QET_ROLLBACK &&
	    qef_cb->qef_rcb->qef_spoint)
	{
	    qef_error(E_QE0016_INVALID_SAVEPOINT, 0L, E_DB_ERROR, &err,
		    &qef_cb->qef_rcb->error, 1, 
		    sizeof(*qef_cb->qef_rcb->qef_spoint), 
		    qef_cb->qef_rcb->qef_spoint);
	    return (E_DB_ERROR);
	}
	return (E_DB_OK);
    }

    /* initialize QSF_RCB */
    qsf_rb.qsf_type = QSFRB_CB;
    qsf_rb.qsf_ascii_id = QSFRB_ASCII_ID;
    qsf_rb.qsf_length = sizeof(QSF_RCB);
    qsf_rb.qsf_owner = (PTR) DB_QEF_ID;
    qsf_rb.qsf_sid = qef_cb->qef_ses_id;

    /* Flush QSF object from QSF that are owner by this session */
    local_status = qsf_call(QSF_SES_FLUSH_LRU, &qsf_rb);
    if(local_status)
    {
        TRdisplay("%@ qet_abort(): Unable to flush all QSF cache objects for sid(%d).\n", qef_cb->qef_ses_id);
    }

    save_point = (qef_cb->qef_rcb->qef_spoint && qef_cb->qef_abort == FALSE);

    if (ddb_b)
    {
	status = qet_t1_abort(qef_cb);
	if (status)
	{
	    if (qef_cb->qef_rcb->error.err_code == E_QE0016_INVALID_SAVEPOINT)
	    {
		if (qef_cb->qef_operation == QET_ROLLBACK &&
		    qef_cb->qef_rcb->qef_spoint)
		    qef_error(E_QE0016_INVALID_SAVEPOINT, 0L, E_DB_ERROR, &err,
			  &qef_cb->qef_rcb->error, 1,
			  sizeof(*qef_cb->qef_rcb->qef_spoint), 
			  qef_cb->qef_rcb->qef_spoint);
	    }

	    /*
	    ** If status was E_DB_FATAL, return it.
	    ** Something bad might have happened, or TPF may be
	    ** trying to crash the server.
	    */

	    if (status == E_DB_FATAL)
	    {
		return (status);
	    }
	    else
	    {
	        return (E_DB_ERROR);
	    }
	}
    }
    else
    {

	/* initialize dmx */

	dmx_cb.type     = DMX_TRANSACTION_CB;
	dmx_cb.length   = sizeof(dmx_cb);
	dmx_cb.dmx_tran_id = qef_cb->qef_dmt_id;
	dmx_cb.dmx_session_id = (char *) qef_cb->qef_rcb->qef_sess_id;

	/*
	** Check if an abort-to-savepoint has been requested.  Bypass 
	** abort-to-savepoint requests if the session has been marked to require
	** transaction-abort.
	**
	** Also check for internal-savepoint rules:
	**
	**     If qef_stm_error specifies transaction-abort, then ignore sp
	**		abort requests with internal savepoint names.
	**     If qef_stm_error specifies user-savepoint-abort, then substitute
	**		the defined savepoint name if this is an internal sp
	**		abort request.
	*/
	if (save_point)
	{
	    sp_name = qef_cb->qef_rcb->qef_spoint;
	    if (MEcmp((PTR) sp_name->db_sp_name,
			  (PTR) QEF_SP_SAVEPOINT, QEF_PS_SZ) == 0)
	    {
		internal_savepoint = TRUE;
	    }
	}

	if (internal_savepoint && (qef_cb->qef_stm_error == QEF_XACT_ROLLBACK))
	{
	    save_point = FALSE;
	}
	else if (internal_savepoint 
		&& (qef_cb->qef_stm_error == QEF_SVPT_ROLLBACK))
	{
	    sp_name = &qef_cb->qef_savepoint;
	}

	/*
	** Format DMF control block for abort request.
	** If abort-to-savepoint request, fill in the savepoint name.
	*/
	if (save_point == TRUE)
	{
	    dmx_cb.dmx_option = DMX_SP_ABORT;
	    STRUCT_ASSIGN_MACRO(*sp_name, dmx_cb.dmx_savepoint_name);
	}
	else
	{
	    dmx_cb.dmx_option = 0;
	}

	/* make the call and process status */

	status = dmf_call(DMX_ABORT, &dmx_cb);

	/* If transaction context is gone, further abort attempts are futile */
	if ( dmx_cb.dmx_tran_id == NULL )
	    qef_cb->qef_stat = QEF_NOTRAN;

	if ( status == E_DB_WARN )
	{
	    /* Abort-to-savepoint promoted to transaction abort */
	    save_point = FALSE;
	    status = E_DB_OK;
	}
	else if (status != E_DB_OK)
	{
	    qef_error(dmx_cb.error.err_code, 0L, status, &err,
		&qef_cb->qef_rcb->error, 0);

	    if ( save_point && !( internal_savepoint ) && 
		 ( dmx_cb.error.err_code == E_DM0102_NONEXISTENT_SP ))
	    {
		return (status);
	    }
	    
	    /* if we abort to an internal savepoint and fail, abort the xact */
	    /* We better not return E_DM0132_ILLEGAL_STMT from dmxabort.c    
	    ** other than WILLING_COMMIT transaction. The problem is that
	    ** we need to find that this transaction is distributed or not
	    ** in qef which we cannot because of the encapsulation.
	    ** We also cannot check the operation code of QET_ABORT or 
	    ** QET_ROLLBACK for bug 87609 which does not make the database
	    ** inconsistent.
	    */
	    if ( save_point &&  qef_cb->qef_stat != QEF_NOTRAN &&
		 dmx_cb.error.err_code != E_DM0132_ILLEGAL_STMT )
	    {
		qef_cb->qef_stat = QEF_NOTRAN;
		dmx_cb.dmx_option = 0;
		if ((local_status = dmf_call(DMX_ABORT, &dmx_cb)) != E_DB_OK)
		{
		    if (dmx_cb.error.err_code != E_DM0146_SETLG_XCT_ABORT)
		    {
		        qef_error(dmx_cb.error.err_code, 0L, local_status, &err,
				&qef_cb->qef_rcb->error, 0);
		        return(E_DB_FATAL);
		    }
		}
	    }
	}
	else                      
	{
	    /*
	    ** After aborting to a user savepoint, lay down a new internal
	    ** savepoint to which to recover should the next update query fail.
	    **
	    ** Only do this if:
	    **     - we are in an MST.
	    **     - the session's statement error handling mode is to abort 
	    **       to the last internal savepoint.
	    **     - there are no open cursors (can't issue svpt command with
	    **       open cursors).
	    */
	    if (save_point &&
		qef_cb->qef_stat == QEF_MSTRAN &&
		qef_cb->qef_open_count == 0 &&
		qef_cb->qef_stm_error == QEF_STMT_ROLLBACK &&
		(local_status = MEcmp((PTR) QEF_SP_SAVEPOINT,
		      (PTR) qef_cb->qef_rcb->qef_spoint->db_sp_name,
			QEF_PS_SZ)) != 0)
	    {
		qef_cb->qef_rcb->qef_spoint = &Qef_s_cb->qef_sp_savepoint;   /* write one */
		status = qet_savepoint(qef_cb);

		if (DB_FAILURE_MACRO(status) && 
		    qef_cb->qef_rcb->error.err_code != E_QE0025_USER_ERROR)
		{
		    qef_error(qef_cb->qef_rcb->error.err_code, 0L, status, &err,
			&qef_cb->qef_rcb->error, 0);
		}
	    }
	}
    }

    /* set state back to quiescent state */

    if ((qef_cb->qef_rcb->qef_context_cnt > 1) && (qef_cb->qef_open_count > 1))
	qef_cb->qef_open_count--;
    else
	qef_cb->qef_open_count = 0; /* no cursors opened */
    qef_cb->qef_defr_curs = FALSE;  /* no deferred cursor */
    qef_cb->qef_abort = FALSE;
    if (save_point == FALSE)
	qef_cb->qef_stat = QEF_NOTRAN;

    return (status);
}


/*{
** Name: QET_AUTOCOMMIT - set autocommit on or off
**
** External QEF call:   status = qef_call(QET_AUTOCOMMIT, &qef_rcb);
**
** Description:
**	The transaction semantics for SQL are changed with this command
**  so that, if set, SQL will behave like SSTRAN (commit after each query).
**  It is illegal to use this command inside a transaction.
**
** Inputs:
**      qef_rcb
**	    .qef_cb		session control block
**	    .qef_auto		auto delete on end of query	
**		QEF_ON
**		QEF_OFF
**	    .qef_eflag		designate error handling semantis
**				for user errors.
**		QEF_INTERNAL	return error code.
**		QEF_EXTERNAL	send message to user.
**
** Outputs:
**      qef_rcb
**          .error.err_code         One of the following:
**                                  E_QE0000_OK
**                                  E_QE0017_BAD_CB
**                                  E_QE0018_BAD_PARAM_IN_CB
**                                  2176 - no autocommit if in a transaction
**
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
**      3-feb-87(daved)
**          written
**	22-may-89 (neil)
**	    Return E_DB_ERROR if in wrong xact state.
*/
DB_STATUS
qet_autocommit(
QEF_CB         *qef_cb)
{
    i4	err;

    /* make sure that this is a legal QEF call */
    if (qef_cb->qef_stat != QEF_NOTRAN)
    {
        qef_error(2176L, 0L, E_DB_ERROR, &err,
            &qef_cb->qef_rcb->error, 0);
        return (E_DB_ERROR);
    }
    qef_cb->qef_auto = qef_cb->qef_rcb->qef_auto;
    qef_cb->qef_rcb->error.err_code = E_QE0000_OK;
    return (E_DB_OK);
}

/*{
** Name: QET_BEGIN      - begin transaction
**
** External QEF call:   status = qef_call(QET_BEGIN, &qef_rcb);
**
** Description:
**      A transaction is begun. The user will now be able to
** execute more than one concurrent statement. 
**
** Inputs:
**      qef_rcb
**	    .qef_cb		session control block
**	    .qef_eflag		designate error handling semantis
**				for user errors.
**		QEF_INTERNAL	return error code.
**		QEF_EXTERNAL	send message to user.
**          .qef_modifier   
**		    QEF_ITRAN	internal transaction (internal use)
**                  QEF_SSTRAN  single stmt transaction (internal use)
**                  QEF_MSTRAN	multi stmt transaction
**          .qef_sess_id
**          .qef_db_id
**          .qef_flag
**                  QEF_READONLY
**                  0
**                  
**
** Outputs:
**      qef_rcb
**          .error.err_code         One of the following:
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
**      29-apr-86 (daved)
**          written
**	28-aug-88 (carl)
**	    modified for Titan
**	25-dec-88 (carl)
**	    more changes for Titan
**	22-may-89 (neil)
**	    Return E_DB_ERROR if in wrong xact state.
**	25-feb-91 (rogerk)
**	    Added support for settable error handling on statement errors.
**	    When qef_stm_error specifies QEF_XACT_ROLLBACK, then don't
**	    lay down internal savepoints in MST's.
**      04-apr-1995 (dilma04) 
**          Set dmx_option to DMX_USER_TRAN unless QEF_ITRAN is set.
**	30-Jul-2004 (jenjo02)
**	    Copy qef_dmt_id to dsh_dmt_id.
**	03-Jul-2006 (jonj)
**	    Handle QET_XA_START, too.
*/
DB_STATUS
qet_begin(
QEF_CB     *qef_cb)
{
    i4     err;
    DB_STATUS   status;
    DMX_CB      dmx_cb;             /* dmf transaction control block */
    bool	ddb_b;

    if ( qef_cb->qef_operation != QET_XA_START &&
         qef_cb->qef_c1_distrib & DB_3_DDB_SESS )
    {
	ddb_b = TRUE;
    }
    else
	ddb_b = FALSE;
    
    /* make sure that this is a legal QEF call
    ** No current transaction.
    */
    if (qef_cb->qef_stat != QEF_NOTRAN)
    {
        qef_error(E_QE0006_TRANSACTION_EXISTS, 0L, E_DB_ERROR, &err,
            &qef_cb->qef_rcb->error, 0);
        return (E_DB_ERROR);
    }

    if (ddb_b)
    {
	status = qet_t2_begin(qef_cb);
        if (status)
	    return (status);
    }
    else
    {
    
	/* initialize dmx */
	dmx_cb.type	= DMX_TRANSACTION_CB;
	dmx_cb.length = sizeof(dmx_cb);

	if (qef_cb->qef_rcb->qef_flag == QEF_READONLY)
	    dmx_cb.dmx_flags_mask = DMX_READ_ONLY;    
	else
	    dmx_cb.dmx_flags_mask = 0;

	dmx_cb.dmx_session_id = (char *) qef_cb->qef_rcb->qef_sess_id;
	dmx_cb.dmx_db_id = qef_cb->qef_rcb->qef_db_id;

	if ( qef_cb->qef_operation == QET_XA_START )
	{
	    dmx_cb.dmx_option = DMX_USER_TRAN;
	    if ( qef_cb->qef_rcb->qef_modifier == QET_XA_START_JOIN )
		dmx_cb.dmx_option |= DMX_XA_START_JOIN;
	    qef_cb->qef_rcb->qef_modifier = QEF_MSTRAN;

	    STRUCT_ASSIGN_MACRO(qef_cb->qef_dis_tran_id, dmx_cb.dmx_dis_tran_id);

	    status = dmf_call(DMX_XA_START, &dmx_cb);
	}
	else
	{
	    dmx_cb.dmx_option =  
	      (qef_cb->qef_rcb->qef_modifier == QEF_ITRAN) ? 0 : DMX_USER_TRAN; 

	    /* make the call and process status */
	    status = dmf_call(DMX_BEGIN, &dmx_cb);
	}

	if (status != E_DB_OK)
	{
	    qef_error(dmx_cb.error.err_code, 0L, status, &err,
		&qef_cb->qef_rcb->error, 0);
	    return (status);
	}
	qef_cb->qef_dmt_id = dmx_cb.dmx_tran_id;
	STRUCT_ASSIGN_MACRO(dmx_cb.dmx_phys_tran_id, qef_cb->qef_tran_id);

	/* Copy the txn context to the root DSH */
	if ( qef_cb->qef_dsh )
	    ((QEE_DSH*)(qef_cb->qef_dsh))->dsh_dmt_id = qef_cb->qef_dmt_id;
    }

    qef_cb->qef_stmt = 1;	    /* renumber statements for new tran */
    qef_cb->qef_defr_curs = FALSE;  /* no deferred cursor yet */
    /* 
    ** convert single statement transaction to multi statement if
    ** autocommit is off.
    */
    if (qef_cb->qef_auto == QEF_OFF && 
	    qef_cb->qef_rcb->qef_modifier == QEF_SSTRAN)
	qef_cb->qef_stat = QEF_MSTRAN;
    else
	qef_cb->qef_stat = qef_cb->qef_rcb->qef_modifier;

    /*
    ** If beginning a Multi-Statement transaction, then declare an internal
    ** QEF savepoint so we can abort back to here on statement errors without
    ** actually ending the transaction.
    **
    ** Only do this if the statement error handling mode is to abort to the
    ** last internal savepoint.
    */

    if (!ddb_b &&
        (qef_cb->qef_stat == QEF_MSTRAN) &&
	(qef_cb->qef_stm_error == QEF_STMT_ROLLBACK))
    {
	MEmove(QEF_PS_SZ, (PTR) QEF_SP_SAVEPOINT, (char) ' ', 
	    sizeof (dmx_cb.dmx_savepoint_name), 
	    (PTR) &dmx_cb.dmx_savepoint_name);
	status = dmf_call(DMX_SAVEPOINT, &dmx_cb);
	if (status != E_DB_OK)
	{	
	    qef_error(dmx_cb.error.err_code, 0L, status, &err,
		&qef_cb->qef_rcb->error, 0);
	    return (status);
	}
    }
	
    qef_cb->qef_rcb->error.err_code = E_QE0000_OK;
    return (E_DB_OK);
}

/*{
** Name: QET_COMMIT     - commit the current transaction
**
** External QEF call:   status = qef_call(QET_COMMIT, &qef_rcb);
**
** Description:
**      All changes are now committed to the database.
** All open cursors are closed. 
**
** Inputs:
**      qef_rcb
**	    .qef_eflag		designate error handling semantis
**				for user errors.
**		QEF_INTERNAL	return error code.
**		QEF_EXTERNAL	send message to user.
**	    .qef_cb		session control block
**
** Outputs:
**      qef_rcb
**          .error.err_code         One of the following:
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
**      29-apr-86 (daved)
**          written
**	11-nov-87 (puree)
**	    made errors from dms_abort fatal errors as returned by DMF.
**	22-dec-87 (puree)
**	    report errors from qee_clean up since qee_xxx will never
**	    report any error.
**	29-jan-88 (puree)
**	    always calll qee_cleanup to make sure that all resources are
**	    released.
**	28-aug-88 (carl)
**	    modified for Titan
**	25-dec-88 (carl)
**	    more changes for Titan
**	22-may-89 (neil)
**	    Return E_DB_ERROR if in wrong xact state.
**	18-apr-91 (fpang)
**	    If qet_t3_commit()  returns with E_DB_FATAL, don't try
**	    to abort. Something is probably seriously wrong in rqf/tpf
**	    so we don't really want to call them.
**	08-Nov-1993 (fpang)
**	    The wrong parameter, 'local_status' was being passed to qef_error() 
**	    in the case where distributed commit fails. The correct parameter
**	    is 'status' for the first call, and 'abt_status' for the second.
**	    Fixes B56673, (Sometimes Star server SEGV's if commit fails).
**	 1-Aug-2000 (hayke02)
**	    Decrement qef_open_count rather than set to zero if we are in a
**	    nested DBP (qef_context_cnt > 1). This prevents DSH cleanup -
**	    qee_cleanup() called from qeu_etran() - and failure of
**	    subsequent DBP calls. This change fixes bug 102123.
**	03-Jul-2006 (jonj)
**	    Handle QET_XA_END, too.
**	26-May-2010 (kschendel) b123814
**	    Call QSF with "session commit" so that it can dump its internal
**	    uncommitted-session-objects list.  (More or less the opposite of
**	    the FLUSH call done by qef rollback.)
*/
DB_STATUS
qet_commit(
QEF_CB     *qef_cb)
{
    i4     err;
    DB_STATUS   status, local_status;
    DMX_CB      dmx_cb;		/* dmf transaction control block */
    QSF_RCB     qsf_rb;		/* QSF request block */

    /* 
    ** make sure that this is a legal QEF call
    ** error if:
    **	    state	operation   result
    **	    NOTRAN	COMMIT	    error
    **	    NOTRAN	others	    return OK
    **	    != MST	COMMIT	    error
    */
    if 
    (
	qef_cb->qef_stat == QEF_NOTRAN 
	|| 
	(
	    qef_cb->qef_operation == QET_COMMIT
	    &&
	    qef_cb->qef_stat != QEF_MSTRAN
	)
    )
    {
	if (qef_cb->qef_rcb->qef_operation == QET_COMMIT)
	{
	    qef_error(E_QE0004_NO_TRANSACTION, 0L, E_DB_ERROR, &err,
		&qef_cb->qef_rcb->error, 0);
	    return (E_DB_ERROR);
	}
	qef_cb->qef_rcb->error.err_code = E_QE0000_OK;
	return (E_DB_OK);
    }

    /* Tell QSF that the transaction is committing, no need to remember
    ** potentially uncommitted objects any more.  Don't do this if an
    ** internal transaction is being committed, though;  the QSF object
    ** for a DB procedure is created during the initial parse, which might
    ** be using an internal transaction;  the actual DBP (including
    ** catalog entries) happens during the "real" transaction, but in
    ** the meantime the sequencer has committed the internal transaction...
    */
    if (qef_cb->qef_stat != QEF_ITRAN)
    {
	qsf_rb.qsf_type = QSFRB_CB;
	qsf_rb.qsf_ascii_id = QSFRB_ASCII_ID;
	qsf_rb.qsf_length = sizeof(QSF_RCB);
	qsf_rb.qsf_owner = (PTR) DB_QEF_ID;
	qsf_rb.qsf_sid = qef_cb->qef_ses_id;
	local_status = qsf_call(QSO_SES_COMMIT, &qsf_rb);
	if(local_status)
	{
	    TRdisplay("%@ qet_commit(): QSF-commit failed for sid(%ld).\n", qef_cb->qef_ses_id);
	    /* but keep going anyway */
	}
    }

    /* DMF closes all cursors on commit. All queries have ended.
    ** Set state back to quiescent state */

    if ((qef_cb->qef_rcb->qef_context_cnt > 1) && (qef_cb->qef_open_count > 1))
	qef_cb->qef_open_count--;
    else
	qef_cb->qef_open_count = 0; /* no cursors opened */
    qef_cb->qef_defr_curs = FALSE;  /* no deferred cursor */
    qef_cb->qef_abort = FALSE;
    qef_cb->qef_stat = QEF_NOTRAN;
    qef_cb->qef_stmt = 1;	    /* renumber statements for new tran */

    if ( qef_cb->qef_operation != QET_XA_END &&
         qef_cb->qef_c1_distrib & DB_3_DDB_SESS )
    {
	status = qet_t3_commit(qef_cb);
	if (status)
	{
	    DB_STATUS	abt_status;

	    /* Don't try to commit if E_DB_FATAL.
	    ** Bad things may have happened, or tpf may have been 
	    ** trying to crash the server.
	    */

	    if (status == E_DB_FATAL)
	    {
	        qef_error(qef_cb->qef_rcb->error.err_code, 0L, status, 
			  &err, &qef_cb->qef_rcb->error, 0);
		return (status);
	    }

	    abt_status = qet_t1_abort(qef_cb);
	    if (abt_status)
	    {
		qef_error(qef_cb->qef_rcb->error.err_code, 0L, abt_status,
			  &err, &qef_cb->qef_rcb->error, 0);
		return(abt_status);	/* cannot commit; cannot abort */
	    }
	    return(status);		/* cannot commit; aborted */
	}
    }
    else
    {

	/* initialize dmx */

	dmx_cb.type = DMX_TRANSACTION_CB;
	dmx_cb.length = sizeof(dmx_cb);
	dmx_cb.dmx_tran_id = qef_cb->qef_dmt_id;
	dmx_cb.dmx_session_id = (char *) qef_cb->qef_rcb->qef_sess_id;
	dmx_cb.dmx_db_id = qef_cb->qef_rcb->qef_db_id;
	dmx_cb.dmx_option = 0;

	if ( qef_cb->qef_operation == QET_XA_END )
	{
	    dmx_cb.dmx_option = DMX_XA_END_SUCCESS;
	    if ( qef_cb->qef_rcb->qef_modifier == QET_XA_END_FAIL )
		dmx_cb.dmx_option = DMX_XA_END_FAIL;

	    STRUCT_ASSIGN_MACRO(qef_cb->qef_dis_tran_id, dmx_cb.dmx_dis_tran_id);

	    /* make the call and process status */
	    status = dmf_call(DMX_XA_END, &dmx_cb);
	    if ( status )
	    {
		qef_error(dmx_cb.error.err_code, 0L, status, &err,
			    &qef_cb->qef_rcb->error, 0);
		return(status);
	    }
	}
	else
	{
	    /* make the call and process status */

	    status = dmf_call(DMX_COMMIT, &dmx_cb);
	    if (status != E_DB_OK)
	    {
		/* if we could not commit, abort */

		qef_error(dmx_cb.error.err_code, 0L, status, &err,
			    &qef_cb->qef_rcb->error, 0);
		dmx_cb.dmx_option = 0;
		local_status = dmf_call(DMX_ABORT, &dmx_cb);
		if (local_status != E_DB_OK)
		{
		    qef_error(dmx_cb.error.err_code, 0L, local_status, &err,
				    &qef_cb->qef_rcb->error, 0);
		    return(local_status);   /* cannot commit; cannot abort */
		}
		return (status);	    /* cannot commit; aborted */
	    }
	    STRUCT_ASSIGN_MACRO(dmx_cb.dmx_commit_time, 
				qef_cb->qef_rcb->qef_comstamp);
	}
    }

    qef_cb->qef_rcb->error.err_code = E_QE0000_OK;
    return (E_DB_OK);
}

/*{
** Name: QET_SAVEPOINT  - set a savepoint
**
** External QEF call:   status = qef_call(QET_SAVEPOINT, &qef_rcb);
**
** Description:
**      A savepoint is set in the current transaction. 
**
** Inputs:
**      qef_rcb
**	    .qef_cb		session control block
**	    .qef_eflag		designate error handling semantis
**				for user errors.
**		QEF_INTERNAL	return error code.
**		QEF_EXTERNAL	send message to user.
**          .qef_spoint         name of user defined savepoint
**	    .qef_flag		DB_QUEL or DB_SQL
**
** Outputs:
**      qef_rcb
**          .error.err_code         One of the following:
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
**      29-apr-86 (daved)
**          written
**	09-sep-87 (puree)
**	    do not abort transaction declaring a bad savepoint.
**	14-oct-87 (puree)
**	    let qet_abort handle errors declaring an internal savepoint.
**	22-dec-87 (puree)
**	    issue error message if declaring a savepoint while 
**	    autocommit is on.
**	28-aug-88 (carl)
**	    modified for Titan
**	25-dec-88 (carl)
**	    more changes for Titan
**	22-may-89 (neil)
**	    Return E_DB_ERROR if in wrong xact state.
**	25-feb-91 (rogerk)
**	    Added support for settable error handling on statement errors.
**	    When qef_stm_error specifies QEF_XACT_ROLLBACK, then don't
**	    lay down internal savepoints in MST's.  Ignore any requests
**	    to issue an internal savepoint.
*/
DB_STATUS
qet_savepoint(
QEF_CB         *qef_cb)
{
    i4     err;
    DB_STATUS   status;
    DMX_CB      dmx_cb;             /* dmf transaction control block */
    DB_ERROR	saved_errblk;
    bool	ddb_b;

    if (qef_cb->qef_c1_distrib & DB_3_DDB_SESS)
	ddb_b = TRUE;
    else
	ddb_b = FALSE;

    saved_errblk.err_code = E_QE0000_OK;
    saved_errblk.err_data = E_QE0000_OK;
    /* 
    ** Make sure that this is a legal QEF call.
    ** Must specify savepoint.
    */
    if (qef_cb->qef_rcb->qef_spoint == 0)
    {
        qef_error(E_QE0018_BAD_PARAM_IN_CB, 0L, E_DB_ERROR, &err,
            &qef_cb->qef_rcb->error, 0);
        return (E_DB_ERROR);
    }

    /* Must be in a transaction */

    if (qef_cb->qef_stat == QEF_NOTRAN ||
	(qef_cb->qef_operation == QET_SAVEPOINT 
		&& qef_cb->qef_stat != QEF_MSTRAN))
    {
	if (qef_cb->qef_rcb->qef_flag == DB_QUEL)
	{
	    qef_error(E_QE0004_NO_TRANSACTION, 0L, E_DB_ERROR, &err,
		&qef_cb->qef_rcb->error, 0);
	    return (E_DB_ERROR);
	}
	else 
	if (qef_cb->qef_auto == QEF_ON)
	{
	    /* savepoint not allowed when autocommit is on */
	    qef_error(2178L, 0L, E_DB_ERROR, &err, &qef_cb->qef_rcb->error, 0);
	    return (E_DB_ERROR);
	}
	else	/* begin a transaction */
	{
	    if (status = qet_begin(qef_cb))
	    {
		return (status);
	    }
	}
    }

    /* Cannot set a savepoint if there are open cursors */

    if (qef_cb->qef_open_count > 0)
    {
        qef_error(E_QE0020_OPEN_CURSOR, 0L, E_DB_ERROR, &err,
            &qef_cb->qef_rcb->error, 0);
        return (E_DB_ERROR);
    }

    if (ddb_b)
    {
	status = qet_t4_savept(qef_cb);
	if (status)
	    return(status);

	qef_cb->qef_rcb->error.err_code = E_QE0000_OK;
	return (E_DB_OK);	/* finished with distributed processing */
    }

    /*
    ** If called to lay down an internal QEF savepoint, and our current
    ** statement error handling mode does not specify to abort to internal
    ** savepoints on errors, then just ignore this request.  If an abort
    ** is actually required, we will abort back to the level indicated
    ** by the statement error handling mode (transaction-abort is the
    ** only currently supported alternative).
    */
    if ((qef_cb->qef_stm_error != QEF_STMT_ROLLBACK) &&
        (MEcmp((PTR) QEF_SP_SAVEPOINT,
               (PTR) qef_cb->qef_rcb->qef_spoint->db_sp_name,
               QEF_PS_SZ) == 0))
    {
	qef_cb->qef_rcb->error.err_code = E_QE0000_OK;
	return (E_DB_OK);
    }

    /* Initialize dmx */

    dmx_cb.type = DMX_TRANSACTION_CB;
    dmx_cb.length = sizeof(dmx_cb);
    dmx_cb.dmx_tran_id = qef_cb->qef_dmt_id;
    dmx_cb.dmx_session_id = (char *) qef_cb->qef_rcb->qef_sess_id;
    STRUCT_ASSIGN_MACRO(*qef_cb->qef_rcb->qef_spoint, 
	    dmx_cb.dmx_savepoint_name);

    /* Call DMF to declare a savepoint. */

    status = dmf_call(DMX_SAVEPOINT, &dmx_cb);
    if (status != E_DB_OK)
    {	
	qef_error(dmx_cb.error.err_code, 0L, status, &err,
			    &qef_cb->qef_rcb->error, 0);
	if (qef_cb->qef_operation != QET_SAVEPOINT)
	{   /* Error in declaring an internal savepoint, abort to the 
	    ** last internal savepoint */
	    STRUCT_ASSIGN_MACRO(qef_cb->qef_rcb->error, saved_errblk);
	    if (qet_abort(qef_cb) == E_DB_OK)
		STRUCT_ASSIGN_MACRO(saved_errblk, qef_cb->qef_rcb->error);
	}
	return (status);
    }

    /*
    ** If declaring a user's savepoint, also declare an internal savepoint.
    ** Only do this if the session's statement error mode is to abort to
    ** the last internal savepoint.
    */

    if ((qef_cb->qef_operation == QET_SAVEPOINT) &&
	(qef_cb->qef_stm_error == QEF_STMT_ROLLBACK))
    {
	MEmove(QEF_PS_SZ, (PTR) QEF_SP_SAVEPOINT, (char) ' ', 
	    sizeof (dmx_cb.dmx_savepoint_name), 
	    (PTR) &dmx_cb.dmx_savepoint_name);
	status = dmf_call(DMX_SAVEPOINT, &dmx_cb);
	if (status != E_DB_OK)
	{   /* Error during declaring an internal savepoint, abort to the
	    ** last internal savepoint */
	    qef_error(dmx_cb.error.err_code, 0L, status, &err,
				&qef_cb->qef_rcb->error, 0);
	    STRUCT_ASSIGN_MACRO(qef_cb->qef_rcb->error, saved_errblk);
	    if (qet_abort(qef_cb) == E_DB_OK)
		STRUCT_ASSIGN_MACRO(saved_errblk, qef_cb->qef_rcb->error);
	    return (status);
	}
    }

    qef_cb->qef_rcb->error.err_code = E_QE0000_OK;
    return (E_DB_OK);
}

/*{
** Name: QET_SECURE - Secure a transaction to end the first phase of a 
**		      distributed transaction.
**
** External QEF call:   status = qef_call(QET_SECURE, &qef_rcb);
**
** Description:
**      All changes are now committed to the database.
**      All open cursors are closed. 
**
** Inputs:
**      qef_rcb
**	    .qef_cb		session control block
**		.qef_dis_tran_id    Distributed transaction ID of the local
**				    transaction.
**
** Outputs:
**      qef_rcb
**          .error.err_code         One of the following:
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
**	8-Mar-89 (ac)
**	    created.
**	22-may-89 (neil)
**	    Return E_DB_ERROR if in wrong xact state.
**	13-aug-91 (davebf)
**	    reset QEF_NOTRAN if aborting after failure to secure (b37778)
**      25-sept-92 (nandak)
**          Copy the entire distributed transaction id.
*/
DB_STATUS
qet_secure(
QEF_CB     *qef_cb)
{
    i4     err;
    DB_STATUS   status, local_status;
    DMX_CB      dmx_cb;             /* dmf transaction control block */

    /* 
    ** make sure that this is a legal QEF call
    ** error if:
    **	    state	operation   result
    **	    NOTRAN	SECURE	    error
    **	    NOTRAN	others	    return OK
    **	    != MST	SECURE	    error
    */
    if 
    (
	qef_cb->qef_stat == QEF_NOTRAN 
	|| 
	(
	    qef_cb->qef_operation == QET_SECURE
	    &&
	    qef_cb->qef_stat != QEF_MSTRAN
	)
    )
    {
	if (qef_cb->qef_rcb->qef_operation == QET_SECURE)
	{
	    qef_error(E_QE0004_NO_TRANSACTION, 0L, E_DB_ERROR, &err,
		&qef_cb->qef_rcb->error, 0);
	    return (E_DB_ERROR);
	}
	qef_cb->qef_rcb->error.err_code = E_QE0000_OK;
	return (E_DB_OK);
    }

    /* DMF closes all cursors on secure. All queries have ended.
    ** Set state back to quiescent state */

    if ((qef_cb->qef_rcb->qef_context_cnt > 1) && (qef_cb->qef_open_count > 1))
	qef_cb->qef_open_count--;
    else
	qef_cb->qef_open_count = 0; /* no cursors opened */
    qef_cb->qef_defr_curs = FALSE;  /* no deferred cursor */

    /* initialize dmx */

    dmx_cb.type = DMX_TRANSACTION_CB;
    dmx_cb.length = sizeof(dmx_cb);
    dmx_cb.dmx_tran_id = qef_cb->qef_dmt_id;
    dmx_cb.dmx_session_id = (char *) qef_cb->qef_rcb->qef_sess_id;
    STRUCT_ASSIGN_MACRO(qef_cb->qef_dis_tran_id, dmx_cb.dmx_dis_tran_id);

    /* make the call and process status */

    status = dmf_call(DMX_SECURE, &dmx_cb);
    if (status != E_DB_OK)
    {
	/* if we could not secure, abort */

        qef_error(dmx_cb.error.err_code, 0L, status, &err,
		&qef_cb->qef_rcb->error, 0);
	qef_cb->qef_abort = FALSE;
	qef_cb->qef_stat = QEF_NOTRAN;
	dmx_cb.dmx_option = 0;
	local_status = dmf_call(DMX_ABORT, &dmx_cb);
	if (local_status != E_DB_OK)
	{
	    qef_error(dmx_cb.error.err_code, 0L, local_status, &err,
					&qef_cb->qef_rcb->error, 0);
	    return(local_status);   /* cannot secure; cannot abort */
	}
	return (status);	    /* cannot secure; aborted */
    }
    qef_cb->qef_rcb->error.err_code = E_QE0000_OK;
    return (E_DB_OK);
}

/*{
** Name: QET_RESUME - Resume a disconnected slave transaction.
**
** External QEF call:   status = qef_call(QET_RESUME, &qef_rcb);
**
** Description:
**      A transaction is resumed. The user will now be able to
**	commit or abort the disconnected transactin.
**
** Inputs:
**      qef_rcb
**	    .qef_cb		session control block
**		.qef_dis_tran_id    Distributed transaction ID of the local
**				    transaction to resume.
**          .qef_modifier   
**		    QEF_ITRAN	internal transaction (internal use)
**                  QEF_SSTRAN  single stmt transaction (internal use)
**                  QEF_MSTRAN	multi stmt transaction
**          .qef_sess_id
**          .qef_db_id
**                  
**
** Outputs:
**      qef_rcb
**          .error.err_code         One of the following:
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
**	8-Mar-89 (ac)
**	    created.
**	22-may-89 (neil)
**	    Return E_DB_ERROR if in wrong xact state.
**      25-sept-92 (nandak)
**          Copy the entire distributed transaction id.
**	30-Jul-2004 (jenjo02)
**	    Copy qef_dmt_id to dsh_dmt_id.
*/
DB_STATUS
qet_resume(
QEF_CB     *qef_cb)
{
    i4     err;
    DB_STATUS   status;
    DMX_CB      dmx_cb;             /* dmf transaction control block */

    
    /* make sure that this is a legal QEF call
    ** No current transaction.
    */
    if (qef_cb->qef_stat != QEF_NOTRAN)
    {
        qef_error(E_QE0006_TRANSACTION_EXISTS, 0L, E_DB_ERROR, &err,
            &qef_cb->qef_rcb->error, 0);
        return (E_DB_ERROR);
    }
    
    /* initialize dmx */
    dmx_cb.type	= DMX_TRANSACTION_CB;
    dmx_cb.length = sizeof(dmx_cb);

    dmx_cb.dmx_session_id = (char *) qef_cb->qef_rcb->qef_sess_id;
    dmx_cb.dmx_db_id = qef_cb->qef_rcb->qef_db_id;
    STRUCT_ASSIGN_MACRO(qef_cb->qef_dis_tran_id, dmx_cb.dmx_dis_tran_id);

    /* make the call and process status */

    status = dmf_call(DMX_RESUME, &dmx_cb);

    if (status != E_DB_OK)
    {
        qef_error(dmx_cb.error.err_code, 0L, status, &err,
            &qef_cb->qef_rcb->error, 0);
	return (status);
    }

    qef_cb->qef_dmt_id = dmx_cb.dmx_tran_id;
    STRUCT_ASSIGN_MACRO(dmx_cb.dmx_phys_tran_id, qef_cb->qef_tran_id);

    /* Copy the txn context to the root DSH */
    if ( qef_cb->qef_dsh )
	((QEE_DSH*)(qef_cb->qef_dsh))->dsh_dmt_id = qef_cb->qef_dmt_id;
	
    qef_cb->qef_stmt = 1;	    /* renumber statements for new tran */
    qef_cb->qef_defr_curs = FALSE;  /* no deferred cursor yet */
    /* 
    ** convert single statement transaction to multi statement if
    ** autocommit is off.
    */
    if (qef_cb->qef_auto == QEF_OFF && 
	    qef_cb->qef_rcb->qef_modifier == QEF_SSTRAN)
	qef_cb->qef_stat = QEF_MSTRAN;
    else
	qef_cb->qef_stat = qef_cb->qef_rcb->qef_modifier;

    qef_cb->qef_rcb->error.err_code = E_QE0000_OK;
    return (E_DB_OK);
}

/*{
** Name: QET_XA_START - Start a distributed transaction.
**
** External QEF call:   status = qef_call(QET_XA_START, &qef_rcb);
**
** Description:
**	Starts or joins an XA distributed transaction.
**
** Inputs:
**      qef_rcb
**	    .qef_modifier	QET_XA_START_JOIN if joining an existing
**				global transaction.
**	    .qef_db_id		Database identifier.
**	    .qef_cb		session control block
**		.qef_dis_tran_id    Distributed transaction ID of the
**				    transaction branch.
**
** Outputs:
**      qef_rcb
**          .error.err_code         As from qet_begin.
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
**	26-Jun-2006 (jonj) 
**	    Created.
*/
DB_STATUS
qet_xa_start(
QEF_CB     *qef_cb)
{
    /* With qef_operation == QET_XA_START... */
    return( qet_begin(qef_cb) );
}

/*{
** Name: QET_XA_END - End a distributed transaction branch.
**
** External QEF call:   status = qef_call(QET_XA_END, &qef_rcb);
**
** Description:
**	Ends this session's work on an XA distributed transaction.
**
** Inputs:
**      qef_rcb
**	    .qef_modifier	QET_XA_END_FAIL to abort the transaction.
**	    .qef_db_id		Database identifier.
**	    .qef_cb		session control block
**		.qef_dis_tran_id    Distributed transaction ID of the
**				    transaction branch.
**
** Outputs:
**      qef_rcb
**          .error.err_code         As from qet_commit.
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
**	26-Jun-2006 (jonj) 
**	    Created.
*/
DB_STATUS
qet_xa_end(
QEF_CB     *qef_cb)
{
    /* With qef_operation == QET_XA_END... */
    return( qet_commit(qef_cb) );
}


/*{
** Name: QET_XA_PREPARE - Prepare a disassociated distributed transaction.
**
** External QEF call:   status = qef_call(QET_XA_PREPARE, &qef_rcb);
**
** Description:
**      Prepares a disassociated transaction branch, Phase 1 of
**	2-phase commit.
**
** Inputs:
**      qef_rcb
**	    .qef_db_id		Database identifier.
**	    .qef_cb		session control block
**		.qef_dis_tran_id    Distributed transaction ID of the
**				    transaction branch.
**
** Outputs:
**      qef_rcb
**          .error.err_code         One of the following:
**                                  E_QE0000_OK
**                                  E_QE0017_BAD_CB
**                                  E_QE0018_BAD_PARAM_IN_CB
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
**          none
**
** History: 
**	26-Jun-2006 (jonj) 
**	    Created.
*/
DB_STATUS
qet_xa_prepare(
QEF_CB     *qef_cb)
{
    i4     	err;
    DB_STATUS   status;
    DMX_CB      dmx_cb;             /* dmf transaction control block */

    /* initialize dmx 
    ** all we have and need are the session id, db_id, and
    ** distributed tran id.
    */
    dmx_cb.type = DMX_TRANSACTION_CB;
    dmx_cb.length = sizeof(dmx_cb);
    dmx_cb.dmx_session_id = (char *) qef_cb->qef_rcb->qef_sess_id;
    dmx_cb.dmx_db_id = qef_cb->qef_rcb->qef_db_id;
    dmx_cb.dmx_option = 0;
    STRUCT_ASSIGN_MACRO(qef_cb->qef_dis_tran_id, dmx_cb.dmx_dis_tran_id);

    /* make the call and process status */
    status = dmf_call(DMX_XA_PREPARE, &dmx_cb);

    if (status != E_DB_OK)
    {
        qef_error(dmx_cb.error.err_code, 0L, status, &err,
            &qef_cb->qef_rcb->error, 0);
    }
    return (status);
}


/*{
** Name: QET_XA_COMMIT - Commit a disassociated distributed transaction.
**
** External QEF call:   status = qef_call(QET_XA_COMMIT, &qef_rcb);
**
** Description:
**      Commits a disassociated transaction branch, Phase 2 of
**	2-phase commit.
**
** Inputs:
**      qef_rcb
**	    .qef_db_id		Database identifier.
**	    .qef_cb		session control block
**		.qef_dis_tran_id    Distributed transaction ID of the
**				    transaction branch.
**
** Outputs:
**      qef_rcb
**          .error.err_code         One of the following:
**                                  E_QE0000_OK
**                                  E_QE0017_BAD_CB
**                                  E_QE0018_BAD_PARAM_IN_CB
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
**          none
**
** History:
**	26-Jun-2006 (jonj)
**	    Created.
*/
DB_STATUS
qet_xa_commit(
QEF_CB     *qef_cb)
{
    i4     	err;
    DB_STATUS   status;
    DMX_CB      dmx_cb;             /* dmf transaction control block */

    /* initialize dmx 
    ** all we have and need are the session id, db_id, and
    ** distributed tran id.
    */
    dmx_cb.type = DMX_TRANSACTION_CB;
    dmx_cb.length = sizeof(dmx_cb);
    dmx_cb.dmx_session_id = (char *) qef_cb->qef_rcb->qef_sess_id;
    dmx_cb.dmx_db_id = qef_cb->qef_rcb->qef_db_id;
    dmx_cb.dmx_option = 0;
    STRUCT_ASSIGN_MACRO(qef_cb->qef_dis_tran_id, dmx_cb.dmx_dis_tran_id);

    if ( qef_cb->qef_rcb->qef_modifier == QET_XA_COMMIT_1PC )
	dmx_cb.dmx_option = DMX_XA_COMMIT_1PC;

    /* make the call and process status */
    status = dmf_call(DMX_XA_COMMIT, &dmx_cb);

    if (status != E_DB_OK)
    {
        qef_error(dmx_cb.error.err_code, 0L, status, &err,
            &qef_cb->qef_rcb->error, 0);
    }
    return (status);
}


/*{
** Name: QET_XA_ROLLBACK - Rollback a distributed transaction branch.
**
** External QEF call:   status = qef_call(QET_XA_ROLLBACK, &qef_rcb);
**
** Description:
**      Rolls back a transaction branch.
**
** Inputs:
**      qef_rcb
**	    .qef_db_id		Database identifier.
**	    .qef_cb		session control block
**		.qef_dis_tran_id    Distributed transaction ID of the
**				    transaction branch.
**
** Outputs:
**      qef_rcb
**          .error.err_code         One of the following:
**                                  E_QE0000_OK
**                                  E_QE0017_BAD_CB
**                                  E_QE0018_BAD_PARAM_IN_CB
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
**          none
**
** History:
**	26-Jun-2006 (jonj)
**	    Created.
*/
DB_STATUS
qet_xa_rollback(
QEF_CB     *qef_cb)
{
    i4     	err;
    DB_STATUS   status;
    DMX_CB      dmx_cb;             /* dmf transaction control block */

    /* initialize dmx 
    ** all we have and need are the session id, db_id, and
    ** distributed tran id.
    */
    dmx_cb.type = DMX_TRANSACTION_CB;
    dmx_cb.length = sizeof(dmx_cb);
    dmx_cb.dmx_session_id = (char *) qef_cb->qef_rcb->qef_sess_id;
    dmx_cb.dmx_db_id = qef_cb->qef_rcb->qef_db_id;
    STRUCT_ASSIGN_MACRO(qef_cb->qef_dis_tran_id, dmx_cb.dmx_dis_tran_id);

    /* make the call and process status */
    status = dmf_call(DMX_XA_ROLLBACK, &dmx_cb);

    if (status != E_DB_OK)
    {
        qef_error(dmx_cb.error.err_code, 0L, status, &err,
            &qef_cb->qef_rcb->error, 0);
    }
    return (status);
}

/*{
** Name: QET_CONNECT      - Connect to transaction context
**
** Description:
**	Creates a shared transaction context for a
**	parallel query thread.
**
** Inputs:
**	qef_rcb			QEF request block
**        qef_cb
**	    .qef_dmt_id		The query session's DMF transaction
**				for which a thread-specific context
**				will be created.
**
** Outputs:
**	tran_id			The thread's shared transaction context.
**      error		        One of the following:
**                                  E_QE0000_OK
**                                  E_QE0004_NO_TRANSACTION
**      Returns:
**          E_DB_OK                 
**          E_DB_ERROR              caller error
**      Exceptions:
**          none
**
** Side Effects:
**	    A new shared log/lock context is created
**	    for the query session.
**
** History:
**	14-Jun-2004 (jenjo02)
**	    Created for || threading.
*/
DB_STATUS
qet_connect(
QEF_RCB    *qef_rcb,
PTR	   *tran_id,
DB_ERROR   *error)
{
    DB_STATUS   status;
    QEF_CB	*qef_cb = qef_rcb->qef_cb;
    DMX_CB      dmx_cb;             /* dmf transaction control block */
    
    /* Make sure there is a transaction to connect to */
    if ( qef_cb->qef_stat == QEF_NOTRAN || qef_cb->qef_dmt_id == NULL ||
	 tran_id == NULL )
    {
	error->err_code = E_QE0004_NO_TRANSACTION;
	status = E_DB_ERROR;
    }
    else
    {
	*tran_id = NULL;

	/* initialize dmx */
	dmx_cb.type	= DMX_TRANSACTION_CB;
	dmx_cb.length = sizeof(dmx_cb);

	dmx_cb.dmx_flags_mask = 0;
        dmx_cb.dmx_option = DMX_CONNECT; 

	/* This is the transaction to connect to */
	dmx_cb.dmx_tran_id = qef_cb->qef_dmt_id;

	/* This is the connecting thread */
	CSget_sid((CS_SID*)&dmx_cb.dmx_session_id);

	/* This is the database */
	dmx_cb.dmx_db_id = qef_rcb->qef_db_id;


	/* make the call and process status */
	status = dmf_call(DMX_BEGIN, &dmx_cb);
	STRUCT_ASSIGN_MACRO(dmx_cb.error, *error);
	
	if ( status == E_DB_OK )
	{
	    /* Return the thread's shared txn context */
	    *tran_id = dmx_cb.dmx_tran_id;
	}
    }

    return (status);
}

/*{
** Name: QET_DISCONNECT     - Disconnect thread's transaction context.
**
** Description:
**	Disconnects a thread from a shared transaction
**	context.
**
** Inputs:
**	qef_rcb			QEF request block
**        qef_cb		Query session's read-only QEF_CB.
**	tran_id			The thread's shared transaction
**				context.
**
** Outputs:
**	tran_id			Set to NULL
**      error		        One of the following:
**                                  E_QE0000_OK
**                                  E_QE0004_NO_TRANSACTION
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
**	14-Jun-2004 (jenjo02)
**	    Created for || threading.
**	01-Oct-2004 (jenjo02)
**	    Don't include check of qef_stat, qef_dmt_id when
**	    determining if a txn is in progress. The
**	    QEF_CB is exposed to all threads and those
**	    variables may get changed such that it may
**	    appear there is no transaction in progress
**	    when in fact there is.
**	09-Dec-2004 (jenjo02)
**	    DMF won't complete the COMMIT if txn is flagged
**	    aborted; in that case, DMX_ABORT instead.
*/
DB_STATUS
qet_disconnect(
QEF_RCB    *qef_rcb,
PTR	   *tran_id,
DB_ERROR   *error)
{
    DB_STATUS   status;
    DMX_CB      dmx_cb;             /* dmf transaction control block */
    
    /* Make sure there is a transaction to disconnect from */
    if ( !tran_id || *tran_id == NULL )
    {
	error->err_code = E_QE0004_NO_TRANSACTION;
	status = E_DB_ERROR;
    }
    else
    {
	/* initialize dmx */

	dmx_cb.type = DMX_TRANSACTION_CB;
	dmx_cb.length = sizeof(dmx_cb);
	dmx_cb.dmx_option = 0;

	/* This is the shared txn context to disconnect */
	dmx_cb.dmx_tran_id = *tran_id;

	/* This is the connected thread */
	CSget_sid((CS_SID*)&dmx_cb.dmx_session_id);

	/*
	** Since this is a shared transaction, we'll merely
	** detach from it.
	**
	** If can't commit (DMF txn is marked "abort", .e.g)
	** the abort instead.
	*/
	if ( status = dmf_call(DMX_COMMIT, &dmx_cb) )
	    status = dmf_call(DMX_ABORT, &dmx_cb);

	STRUCT_ASSIGN_MACRO(dmx_cb.error, *error);

	*tran_id = NULL;
    }

    return (status);
}
