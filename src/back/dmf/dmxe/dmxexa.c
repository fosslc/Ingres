/*
** Copyright (c)  2006 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <me.h>
#include    <cs.h>
#include    <cx.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <adf.h>
#include    <pc.h>
#include    <lg.h>
#include    <lk.h>
#include    <scf.h>
#include    <dmf.h>
#include    <ulf.h>
#include    <dmxcb.h>
#include    <dmrcb.h>
#include    <dmccb.h>
#include    <dmtcb.h>
#include    <dmucb.h>
#include    <dm.h>
#include    <dmp.h>
#include    <dml.h>
#include    <dmxe.h>
#include    <dm1b.h>
#include    <dmpp.h>
#include    <dm0l.h>
#include    <dm0llctx.h>
#include    <dm0m.h>
#include    <dm0p.h>
#include    <dm2t.h>
#include    <dmfgw.h>
#include    <dmftrace.h>

/**
**
**  Name: DMXEXA.C - Distributed XA transaction handling services.
**
**  Description:
**      This file contains the functions that twiddle locking and logging
**	contexts needed to implement Distributed XA transactions.
**
**          dmxe_xa_disassoc	- Disassociate a thread from a 
**				  transaction branch.
**          dmxe_xa_prepare 	- Prepare a disassociated transaction
**				  branch for commit.
**	    dmxe_xa_commit	- Commit a disassociated transaction
**				  branch.
**          dmxe_xa_rollback	- Rollback a transaction branch.
**
**
**  History:
**	26-Jun-2006 (jonj)
**	    Created.
**      11-may-2007 (stial01)
**          dmxe_xa_prepare() call dm0l_secure() instead of dm0p_force_pages
**          dmxe_xa_commit() if xn not found see if RCP owns it
**          dmxe_xa_rollback() if xn not found, see if RCP owns it 
**	12-Apr-2008 (kschendel)
**	    Need to include adf.h now.
**	20-Nov-2008 (jonj)
**	    SIR 120874: Convert functions to return DB_ERROR *dberr 
**	    instead of i4 *err_code. Use new form uleFormat.
**	25-Nov-2008 (jonj)
**	    SIR 120874: dm0l_? functions converted to DB_ERROR *
**	19-Aug-2009 (kschendel) 121804
**	    Need cx.h for proper CX declarations (gcc 4.3).
**/

/*{
** Name: dmxe_xa_disassoc	- Disassociate a transaction context from
**				  a thread.
**
** Description:
**	Disassociates a log/lock context from the calling thread.
**
** Inputs:
**      DisTranId			The distributed tranid.
**	log_id				This transaction's handle to it.
**	TxnStateFlags			The state of the txn.
**
** Outputs:
**      err_code                        Reason for error return status.
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    The log/lock context is disassociated from the DMF
**	    transaction and transferred to the RCP.
**	    There it awaits prepare/commit/rollback/resume.
**
** History:
**	26-Jun-2006 (jonj)
**	    Created.
**	01-Aug-2006 (jonj)
**	    Handle LG_CONTINUE from LGalter(LG_A_XA_DISASSOC).
*/
DB_STATUS
dmxe_xa_disassoc(
DB_DIS_TRAN_ID	    *DisTranId,
i4		    log_id,
i4		    lock_id,
i4		    TxnStateFlags,
DB_ERROR	    *dberr)
{
    STATUS		stat;
    CL_ERR_DESC		sys_err;
    LG_TRANSACTION	tr;
    i4			*err_code = &dberr->err_code;

    CLRDBERR(dberr);

    /* Stow the transaction state flags in the log context */
    tr.tr_id = log_id;
    tr.tr_txn_state = TxnStateFlags;

    stat = LGalter(LG_A_TXN_STATE, (PTR)&tr, sizeof(tr), &sys_err);
    if ( stat )
    {
	uleFormat(NULL, stat, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, 0L, (i4 *)NULL, err_code, 0);
	uleFormat(NULL, E_DM900B_BAD_LOG_ALTER, &sys_err, 
		    ULE_LOG, NULL, 0, 0, 0,
		    err_code, 1, 0, LG_A_TXN_STATE);
	SETDBERR(dberr, 0, E_DM016A_ERROR_XA_DISASSOC);
    }
    /* Relinquish ownership of the log/lock context */
    else if ( stat = LGalter(LG_A_XA_DISASSOC, (PTR)&log_id, sizeof(log_id), &sys_err) )
    {
	/*
	** LG_CONTINUE indicates there are other live associations with 
	** this Branch. 
	**
	** In this case, we simply destroy our log/lock contexts; when the
	** last association disassociates, the XA Branch will be
	** orphaned and given over to the RCP's ownership.
	*/
	if ( stat == LG_CONTINUE )
	{
	    /* End our log/lock contexts */
	    stat = LGend(log_id, 0, &sys_err);
	    if ( stat != OK )
	    {
		uleFormat(NULL, stat, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, 0L, (i4 *)NULL, err_code, 0);
		uleFormat(NULL, E_DM900E_BAD_LOG_END, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, 0L, (i4 *)NULL, err_code, 1, 0, log_id);
	    }

	    stat = LKrelease(LK_ALL, lock_id, (LK_LKID *)0, 
					(LK_LOCK_KEY *)0, (LK_VALUE *)0, &sys_err);
	    if ( stat != OK )
	    {
		uleFormat(NULL, stat, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, 0L, (i4 *)NULL, err_code, 0);
		uleFormat(NULL, E_DM901B_BAD_LOCK_RELEASE, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, 0L, (i4 *)NULL, err_code, 1, 0, lock_id);
	    }
	}
	else
	{
	    uleFormat(NULL, stat, &sys_err, ULE_LOG, NULL,
			(char *)NULL, 0L, (i4 *)NULL, err_code, 0);
	    uleFormat(NULL, E_DM900B_BAD_LOG_ALTER, &sys_err, ULE_LOG, NULL,
			(char *)NULL, 0L, (i4 *)NULL, err_code, 1, 0, 
			LG_A_XA_DISASSOC);
	}

	if ( stat )
	    SETDBERR(dberr, 0, E_DM016A_ERROR_XA_DISASSOC);
    }    

    return( (dberr->err_code) ? E_DB_ERROR : E_DB_OK );
}


/*{
** Name: dmxe_xa_prepare	- Prepare a disassociated XA distributed 
**				  transaction branch.
**				  (Phase 1 of 2-phase commit)
**
** Description:
**      Prepare a disassociated XA distributed transaction branch.
**
** Inputs:
**      DisTranId			The distributed tranid to prepare.
**      dcb				DCB of its database.
**
** Outputs:
**      err_code                        Reason for error return status.
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	26-Jun-2006 (jonj)
**	    Created.
*/
DB_STATUS
dmxe_xa_prepare(
DB_DIS_TRAN_ID	    *DisTranId,
DMP_DCB		    *dcb,
DB_ERROR	    *dberr)
{
    DB_STATUS		status;
    STATUS		stat;
    CL_ERR_DESC		sys_err;
    i4			length;
    LG_TRANSACTION	tr;
    LG_CONTEXT		ctx;
    i4			*err_code = &dberr->err_code;

    CLRDBERR(dberr);

    /* Reject the secure request if in the cluster environment. */

    if ( CXcluster_enabled() )
    {
	SETDBERR(dberr, 0, E_DM0122_CANT_SECURE_IN_CLUSTER);
	return (E_DB_ERROR);    
    }

    /*
    ** Find the distributed transaction branch in the logging system.
    **
    ** To be preparable, the transaction must have been xa_end(TMSUCCESS)
    ** not already prepared, and owned by the RCP (disassociated from
    ** any user thread).
    **
    ** The LG_S_XA_PREPARE call will verify this and change the 
    ** ownership of the transaction to our process.
    **
    ** Then we call LGalter(LG_A_PREPARE), which marks the transaction as
    ** prepared, then we transfer it back to the RCP where it awaits
    ** dmxe_xa_commit.
    */
    
    MEcopy((char *)DisTranId, sizeof(DB_DIS_TRAN_ID), (char *)&tr.tr_dis_id);

    /* Identify this process and database to LGshow */
    tr.tr_pr_id = dmf_svcb->svcb_lctx_ptr->lctx_lgid;
    tr.tr_lpd_id = dcb->dcb_log_id;

    /* 
    ** LGshow will return
    **
    **		length > 0	Transaction found and can be secured.
    **		length = 0	Transaction found, but cannot be secured.
    **		length < 0	Transaction not found in logging system.
    */
    
    stat = LGshow(LG_S_XA_PREPARE, (PTR)&tr, sizeof(tr), &length, &sys_err);

    if ( stat )
    {
	uleFormat(NULL, stat, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, 0L, (i4 *)NULL, err_code, 0);
	uleFormat(NULL, E_DM9017_BAD_LOG_SHOW, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, 0L, (i4 *)NULL, err_code, 1, 0, 
		    LG_S_XA_PREPARE);
	SETDBERR(dberr, 0, E_DM012B_ERROR_XA_PREPARE);
    }
    else if ( length > 0 )
    {
	/*
	** Alter this transaction branch to PREPARED.
	** When all branches have done so, LGalter returns
	** LG_CONTINUE, at which time we'll make sure that
	** all of the global transaction's pages have been forced.
	**
	** Notice that even after we've done this, the branches
	** are abortable; we never enter the WILLING_COMMIT state
	** because we retain the log/lock contexts until the
	** final XA_COMMIT rolls around.
	*/
	ctx.lg_ctx_id = tr.tr_id;

	stat = LGalter(LG_A_PREPARE, (PTR)&ctx, sizeof(ctx), &sys_err);

	if ( stat == LG_CONTINUE )
	{
	    stat = OK;
	    status = dm0l_secure(tr.tr_id, tr.tr_lock_id, &tr.tr_eid, 
				    &tr.tr_dis_id, dberr);
	}
	else if ( stat )
	{
	    uleFormat(NULL, stat, &sys_err, ULE_LOG, NULL,
			(char *)NULL, 0L, (i4 *)NULL, err_code, 0);
	    uleFormat(NULL, E_DM900B_BAD_LOG_ALTER, &sys_err, ULE_LOG, NULL,
			(char *)NULL, 0L, (i4 *)NULL, err_code, 1, 0, 
			LG_A_PREPARE);
	    SETDBERR(dberr, 0, E_DM012B_ERROR_XA_PREPARE);
	}
	else
	    status = E_DB_OK;

	if ( stat == OK && status == E_DB_OK )
	{
	    /* Disassociate from the log/lock context */
	    if ( status = dmxe_xa_disassoc(&tr.tr_dis_id, 
					tr.tr_id, 
					tr.tr_lock_id, 
					tr.tr_txn_state, 
					dberr) )
		SETDBERR(dberr, 0, E_DM012B_ERROR_XA_PREPARE);
	}
    }
    else if ( length == 0 )
    {
	/* Found the txn, but already in a prepared state */
	SETDBERR(dberr, 0, E_DM011F_INVALID_TXN_STATE);

    }
    else
    {
	/* Distributed txn not found */
	SETDBERR(dberr, 0, E_DM0120_DIS_TRAN_UNKNOWN);
    }

    return( (dberr->err_code) ? E_DB_ERROR : E_DB_OK );
}

/*{
** Name: dmxe_xa_commit		- Commit a disassociated XA distributed
** 				  transaction branch.
**				  (Phase 2 of 2-phase commit)
**
** Description:
**      Commit a disassociated XA distributed transaction branch.
**
** Inputs:
**      DisTranId			The distributed tranid to commit.
**      dcb				DCB of its database.
**	OnePhaseCommit			TRUE if one-phase commit optimization
**					(prepare not done)
**
** Outputs:
**      err_code                        Reason for error return status.
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	26-Jun-2006 (jonj)
**	    Created.
*/
DB_STATUS
dmxe_xa_commit(
DB_DIS_TRAN_ID	    *DisTranId,
DMP_DCB		    *dcb,
bool		    OnePhaseCommit,
DB_ERROR	    *dberr)
{
    DB_STATUS		status;
    STATUS		stat;
    CL_ERR_DESC		sys_err;
    i4			length;
    LG_TRANSACTION	tr;
    DB_TAB_TIMESTAMP	ctime;
    i4			*err_code = &dberr->err_code;

    CLRDBERR(dberr);

    /*
    ** Find the distributed transaction in the logging system.
    **
    ** To be commitable, the transaction must be disassociated.
    **
    ** If OnePhaseCommit (xa_commit(TMONEPHASE)), do an
    ** optimized commit, skipping the prepare, otherwise
    ** the transaction must have been prepared.
    **
    ** The LG_S_XA_COMMIT call will verify this and change the 
    ** ownership of the transaction to our process.
    **
    ** Then we call dmxe_commit to commit the transaction, which
    ** removes its log/lock contexts from the system.
    */
    
    MEcopy((char *)DisTranId, sizeof(DB_DIS_TRAN_ID), (char *)&tr.tr_dis_id);

    /* Identify this process and database to LGshow */
    tr.tr_pr_id = dmf_svcb->svcb_lctx_ptr->lctx_lgid;
    tr.tr_lpd_id = dcb->dcb_log_id;

    /* 
    ** LGshow will return
    **
    **		length > 0	Transaction found and can be committed.
    **		length = 0	Transaction found, but is not committable.
    **		length < 0	Transaction not found in logging system.
    */
    if ( OnePhaseCommit )
	stat = LGshow(LG_S_XA_COMMIT_1PC, (PTR)&tr, sizeof(tr), &length, &sys_err);
    else
	stat = LGshow(LG_S_XA_COMMIT, (PTR)&tr, sizeof(tr), &length, &sys_err);

    if (!stat && length < 0 && !OnePhaseCommit)
    {
	/*
	** Distributed txn not found
	** If ingres terminated abnormally after PREPARE,
	** the RCP will own WILLING_COMMIT transactions
	*/
	stat = LGshow(LG_S_COMMIT_WILLING, (PTR)&tr, sizeof(tr), &length, &sys_err);
	if (stat == OK && length > 0)
	    return (E_DB_OK);
    }

    if ( stat )
    {
	uleFormat(NULL, stat, &sys_err, ULE_LOG, NULL,
		(char *)NULL, 0L, (i4 *)NULL, err_code, 0);
	uleFormat(NULL, E_DM9017_BAD_LOG_SHOW, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, 0L, (i4 *)NULL, err_code, 1, 0, 
		    LG_S_XA_COMMIT);
	SETDBERR(dberr, 0, E_DM012C_ERROR_XA_COMMIT);
    }
    else if ( length > 0 )
    {
	status = dmxe_commit(&tr.tr_eid, tr.tr_id, tr.tr_lock_id, tr.tr_txn_state, 
				&ctime, dberr);
	
	/* We expect log/lock contexts to have been obliterated now */
    }
    else if ( length == 0 )
    {
	/* Transaction was found, but in wrong state */
	SETDBERR(dberr, 0, E_DM011F_INVALID_TXN_STATE);
    }
    else
    { 
	/* Distributed txn not found */
	SETDBERR(dberr, 0, E_DM0120_DIS_TRAN_UNKNOWN);
    }

    return( (dberr->err_code) ? E_DB_ERROR : E_DB_OK );

}


/*{
** Name: dmxe_xa_rollback	- Rollback a XA distributed transaction branch
**
** Description:
**      Rollback a XA distributed transaction, associated or disassociated.
**
** Inputs:
**      DisTranId			The distributed tranid to rollback.
**      dcb				DCB of its database.
**
** Outputs:
**      err_code                        Reason for error return status.
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    If transaction branch is not disassociated (i.e., it
**	    belongs to a thread), all branches of the global
**	    transaction are forcibly aborted (xa_rollback before xa_end).
**
** History:
**	26-Jun-2006 (jonj)
**	    Created.
*/
DB_STATUS
dmxe_xa_rollback(
DB_DIS_TRAN_ID	    *DisTranId,
DMP_DCB		    *dcb,		    
DB_ERROR	    *dberr)
{
    DB_STATUS		status;
    STATUS		stat;
    CL_ERR_DESC		sys_err;
    i4			length;
    LG_TRANSACTION	tr;
    i4			*err_code = &dberr->err_code;

    CLRDBERR(dberr);

    /* Get the branch log context */
    
    MEcopy((char *)DisTranId, sizeof(DB_DIS_TRAN_ID), (char *)&tr.tr_dis_id);

    /* Identify this process and database to LGshow */
    tr.tr_pr_id = dmf_svcb->svcb_lctx_ptr->lctx_lgid;
    tr.tr_lpd_id = dcb->dcb_log_id;
    
    /* 
    ** LGshow will return
    **
    **		length > 0	Transaction found and is abortable.	
    **		length = 0	Transaction found, but already aborting.
    **		length < 0	Transaction not found in logging system;
    **				it may be a bad XA tran id, or may have
    **				already aborted and exited the logging system.
    */
    stat = LGshow(LG_S_XA_ROLLBACK, (PTR)&tr, sizeof(tr), &length, &sys_err);

    if (!stat && length < 0)
    {
	/*
	** Distributed txn not found
	** If ingres terminated abnormally after PREPARE,
	** the RCP will own WILLING_COMMIT transactions
	*/
	stat = LGshow(LG_S_ROLLBACK_WILLING, (PTR)&tr, sizeof(tr), &length, &sys_err);
	if (stat == OK && length > 0)
	    return (E_DB_OK);
    }

    if ( stat )
    {
	uleFormat(NULL, stat, &sys_err, ULE_LOG, NULL,
		(char *)NULL, 0L, (i4 *)NULL, err_code, 0);
	uleFormat(NULL, E_DM9017_BAD_LOG_SHOW, &sys_err, ULE_LOG, NULL,
		(char *)NULL, 0L, (i4 *)NULL, err_code, 1, 0, 
		LG_S_XA_ROLLBACK);
	SETDBERR(dberr, 0, E_DM012D_ERROR_XA_ROLLBACK);
    }
    else if ( length > 0 )
    {
	status = dmxe_abort((DML_ODCB*)NULL, 
				dcb, 
				&tr.tr_eid,
				tr.tr_id,
				tr.tr_lock_id,
				tr.tr_txn_state,
				(i4)0,
				(LG_LSN*)NULL,
				dberr);

	/* We expect log/lock contexts to have been obliterated now */
    }
    else if ( length == 0 )
    {
	/* Found txn, but already marked for abort */
    }
    else
    {
	/* Txn not found; it may have already been aborted */
    }

    return( (dberr->err_code) ? E_DB_ERROR : E_DB_OK );
}
