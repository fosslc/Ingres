/*
**Copyright (c) 2006 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <me.h>
#include    <cs.h>
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
#include    <dm0m.h>
#include    <dm2t.h>
#include    <dmfgw.h>
#include    <dmftrace.h>

/**
** Name: DMXXA.C - Functions used to manage Distributed XA
**		   transactions.
**
** Description:
**      This file contains the functions needed to manage
**	distributed XA transactions.

**      This file defines:
**    
**      dmx_xa_start()		-  Start work on a
**				   transaction branch.
**      dmx_xa_end()        	-  End work on a
**				   transaction branch.
**      dmx_xa_prepare()       	-  Prepare to commit 
**				   a transaction branch.
**      dmx_xa_commit()       	-  Commit a transaction branch.
**      dmx_xa_rollback()     	-  Rollback a transaction branch.
**
**	In the normal course of events, a thread:
**		o dmx_xa_start() 	to start a unit of work
**					and assume ownership of
**					its log/lock context.
**		o dmxe_xa_end(SUCCESS)	to end that unit of work
**					and disassociate itself
**					from the log/lock context.
**		While disassociated, any thread connected to the
**		context's database may prepare, commit, or 
**		rollback the unit of work.
**
** History:
**	26-Jun-2006 (jonj)
**	    Created.
**      08-jun-2007 (stial01)
**          dmx_end() If transaction is already aborting, continue
**          processing the end for this branch. (b118397)
**	12-Apr-2008 (kschendel)
**	    Need to include adf.h now.
**	20-Oct-2008 (jonj)
**	    SIR 120874 Modified to use new DB_ERROR based uleFormat 
**	21-Nov-2008 (jonj)
**	    SIR 120874: dmxe_? functions converted to DB_ERROR *
[@history_template@]...
*/

/*{
** Name: dmx_xa_start - Start work on a transaction branch.
**
**  INTERNAL DMF call format:      status = dmx_xa_start(&dmx_cb);
**
**  EXTERNAL call format:          status = dmf_call(DMX_XA_START,&dmx_cb);
**
** Description:
**	Start work on a transaction branch by either joining an existing
**	branch, or starting a new one.
**
** Inputs:
**      dmx_cb 
**          .type                           Must be set to DMX_TRANSACTION_CB.
**          .length                         Must be at least sizeof(DMX_CB).
**          .dmx_flags_mask                 Must be zero or DMX_READ_ONLY.
**          .dmx_session_id                 Must be a valid session identifier
**                                          obtained from SCF.
**          .dmx_db_id                      The database identifier of the
**                                          database in this transaction that
**                                          can be updated.
**          .dmx_option                     DMX_XA_START_JOIN if joining an existing
**					    global transaction.
**					    DMX_XA_START_RESUME if resuming a
**					    suspended branch.
**	    .dmx_dis_tran_id		    The distributed branch XID.
**					    
** Output:
**      dmx_cb 
**          .error.err_code                 One of the following error numbers.
**                                          E_DM0000_OK                
**                                          E_DM002F_BAD_SESSION_ID
**                                          E_DM003B_BAD_TRAN_ID
**                                          E_DM0061_TRAN_NOT_IN_PROGRESS
**					    E_DM0094_ERROR_BEGINNING_TRAN
**					    E_DM011F_INVALID_TXN_STATE
**					    E_DM0120_DIS_TRAN_UNKNOWN
**					    E_DM012E_ERROR_XA_START
**                     
**          .dmx_tran_id                    Identifier for this transaction.
**          .dmx_phys_tran_id               Physical transaction identifier.
**
**      Returns:
**          E_DB_OK                         Function completed normally. 
**          E_DB_WARN                       Function completed normally with 
**                                          a termination status which is in 
**                                          dmx_cb.error.err_code.
**          E_DB_ERROR                      Function completed abnormally with 
**                                          a termination status which is in
**                                          dmx_cb.error.err_code.
**          E_DB_FATAL                      Function completed with a fatal
**                                          error which must be handled
**                                          immediately.  The fatal status is in
**                                          dmx_cb.error.err_code.
** History:
**	26-Jun-2006 (jonj)
**	    Created.
*/
DB_STATUS
dmx_xa_start(
DMX_CB    *dmx)
{
    DML_SCB		*scb;
    DML_XCB		*xcb;
    DB_STATUS		status;
    STATUS		stat;
    CL_ERR_DESC		sys_err;
    LG_TRANSACTION	tr;
    i4			length;
    i4			error;

    CLRDBERR(&dmx->error);

    if ( dmx->dmx_dis_tran_id.db_dis_tran_id_type != DB_XA_DIS_TRAN_ID_TYPE )
	    SETDBERR(&dmx->error, 0, E_DM003B_BAD_TRAN_ID);
    else if ( dmx->dmx_option & DMX_XA_START_RESUME )
    {
	/*
	** Resume doesn't do much, just verifies that we're already 
	** in the matching XA transaction and returns its tran_id.
	*/
	if ( !(scb = GET_DML_SCB(dmx->dmx_session_id)) ||
	      dm0m_check((DM_OBJECT*)scb, (i4)SCB_CB) )
	{
	    SETDBERR(&dmx->error, 0, E_DM002F_BAD_SESSION_ID);
	}
	else 
	{
	    xcb = (DML_XCB*)scb->scb_x_next;

	    if ( xcb == (DML_XCB*)&scb->scb_x_next )
		SETDBERR(&dmx->error, 0, E_DM0061_TRAN_NOT_IN_PROGRESS);
	    else if ( xcb->xcb_state & (XCB_STMTABORT | XCB_TRANABORT) )
		SETDBERR(&dmx->error, 0, E_DM0064_USER_ABORT);
	    else
	    {
		/* Get information about this transaction's log context */
		MEcopy((char*)&xcb->xcb_log_id, sizeof(xcb->xcb_log_id), (char*)&tr);
		stat = LGshow(LG_S_ATRANSACTION, (PTR)&tr, sizeof(tr), &length, &sys_err);

		if ( stat || length == 0 )
		{
		    if ( stat )
		    {
			uleFormat(NULL, stat, &sys_err, ULE_LOG, NULL,
			    (char *)0, 0L, (i4 *)0, &error, 0);
		    }
		    uleFormat(NULL, E_DM9017_BAD_LOG_SHOW, &sys_err, ULE_LOG, NULL,
			(char *)NULL, 0L, (i4 *)NULL, &error, 1, 0, LG_S_ATRANSACTION);
		    SETDBERR(&dmx->error, 0, E_DM012E_ERROR_XA_START);
		}
		else
		{
		    /* Must match on supplied Distributed tran id */
		    if ( !(DB_DIS_TRAN_ID_EQL_MACRO(dmx->dmx_dis_tran_id, tr.tr_dis_id)) )
			SETDBERR(&dmx->error, 0, E_DM0120_DIS_TRAN_UNKNOWN);
		    /* ... and not be in any unexpected state */
		    else if ( tr.tr_status & (TR_ORPHAN | TR_ABORT | TR_PREPARED) )
			SETDBERR(&dmx->error, 0, E_DM011F_INVALID_TXN_STATE);
		    else
		    {
			dmx->dmx_tran_id = (char*)xcb;
			STRUCT_ASSIGN_MACRO(xcb->xcb_tran_id, dmx->dmx_phys_tran_id);
		    }
		}
	    }
	}
    }
    else
    {
	/* Start new branch or DMX_XA_START_JOIN an existing one */

	/* dmx_begin does all the work */
	dmx->dmx_option |= (DMX_XA_START_XA | DMX_USER_TRAN);

	status = dmx_begin(dmx);
    }
	
    return((dmx->error.err_code) ? E_DB_ERROR : E_DB_OK);
}

/*{
** Name: dmx_xa_end - End work on a transaction branch.
**
**  INTERNAL DMF call format:      status = dmx_xa_end(&dmx_cb);
**
**  EXTERNAL call format:          status = dmf_call(DMX_XA_END,&dmx_cb);
**
** Description:
**	Ends this thread's work on a transaction branch.
**
**		DMX_XA_END_SUCCESS: Disassociate this thread from its 
**				    log/lock context
**		DMX_XA_END_FAIL:    Abort the transaction branch.
**		DMX_XA_END_SUSPEND: Suspend this thread's unit of work.
**
** Inputs:
**      dmx_cb 
**          .type                           Must be set to DMX_TRANSACTION_CB.
**          .length                         Must be at least sizeof(DMX_CB).
**          .dmx_session_id                 Must be a valid session identifier
**                                          obtained from SCF.
**          .dmx_option                     DMX_XA_END_SUCCESS,
**					    DMX_XA_END_FAIL, or
**					    DMX_XA_END_SUSPEND.
**	    .dmx_dis_tran_id		    XID of transaction branch.
**					    
** Output:
**      dmx_cb 
**          .error.err_code                 One of the following error numbers.
**                                          E_DM0000_OK                
**                                          E_DM003B_BAD_TRAN_ID
**                                          E_DM011F_INVALID_TXN_STATE
**                                          E_DM0120_DIS_TRAN_UNKNOWN
**                                          E_DM012F_ERROR_XA_END
**
**      Returns:
**          E_DB_OK                         Function completed normally. 
**          E_DB_WARN                       Function completed normally with 
**                                          a termination status which is in 
**                                          dmx_cb.error.err_code.
**          E_DB_ERROR                      Function completed abnormally with 
**                                          a termination status which is in
**                                          dmx_cb.error.err_code.
**          E_DB_FATAL                      Function completed with a fatal
**                                          error which must be handled
**                                          immediately.  The fatal status is in
**                                          dmx_cb.error.err_code.
** History:
**	26-Jun-2006 (jonj)
**	    Created.
*/
DB_STATUS
dmx_xa_end(
DMX_CB    *dmx)
{
    STATUS		stat;
    DB_STATUS		status;
    DML_SCB		*scb;
    DML_XCB		*xcb;
    CL_ERR_DESC		sys_err;
    i4			length;
    i4			error;
    LG_TRANSACTION	tr;

    /* Handle xa_end(TMSUCCESS | TMFAIL | TMSUSPEND) */

    CLRDBERR(&dmx->error);

    /* First need to verify that this thread's dist tran id exists. */
    if ( dmx->dmx_dis_tran_id.db_dis_tran_id_type != DB_XA_DIS_TRAN_ID_TYPE )
	    SETDBERR(&dmx->error, 0, E_DM003B_BAD_TRAN_ID);
    else
    {
	if ( !(scb = GET_DML_SCB(dmx->dmx_session_id)) ||
	      dm0m_check((DM_OBJECT*)scb, (i4)SCB_CB) )
	{
	    SETDBERR(&dmx->error, 0, E_DM002F_BAD_SESSION_ID);
	}
	else 
	{
	    xcb = (DML_XCB*)scb->scb_x_next;

	    if ( xcb == (DML_XCB*)&scb->scb_x_next )
		SETDBERR(&dmx->error, 0, E_DM0061_TRAN_NOT_IN_PROGRESS);
	    /* Must match on supplied Distributed tran id */
	    else if ( !(DB_DIS_TRAN_ID_EQL_MACRO(dmx->dmx_dis_tran_id, xcb->xcb_dis_tran_id)) )
		SETDBERR(&dmx->error, 0, E_DM0120_DIS_TRAN_UNKNOWN);
	    else
	    {
		/* Get information about this transaction's log context */
		MEcopy((char*)&xcb->xcb_log_id, sizeof(xcb->xcb_log_id), (char*)&tr);

		stat = LGshow(LG_S_ATRANSACTION, (PTR)&tr, sizeof(tr), &length, &sys_err);

		if ( stat || length == 0 )
		{
		    if ( stat )
		    {
			uleFormat(NULL, stat, &sys_err, ULE_LOG, NULL,
			    (char *)NULL, 0L, (i4 *)NULL, &error, 0);
		    }
		    uleFormat(NULL, E_DM9017_BAD_LOG_SHOW, &sys_err, ULE_LOG, NULL,
			(char *)NULL, 0L, (i4 *)NULL, &error, 1, 0, LG_S_ATRANSACTION);
		    SETDBERR(&dmx->error, 0, E_DM012F_ERROR_XA_END);
		}
		else
		{
		    /* Must match on supplied Distributed tran id */
		    if ( !(DB_DIS_TRAN_ID_EQL_MACRO(dmx->dmx_dis_tran_id, tr.tr_dis_id)) )
			SETDBERR(&dmx->error, 0, E_DM0120_DIS_TRAN_UNKNOWN);
		    /* ... and not be in any unexpected state */
		    else if ( tr.tr_status & (TR_ORPHAN | TR_PREPARED) )
		    {
			SETDBERR(&dmx->error, 0, E_DM011F_INVALID_TXN_STATE);
		    }
		    else if (tr.tr_status & TR_ABORT)
		    {
			/*
			** Another handle of this trans is aborting 
			** Go ahead and clean up this handle's cb's
			*/
			status = dmx_abort(dmx);
		    }
		    else if ( dmx->dmx_option & DMX_XA_END_SUCCESS )
			status = dmx_commit(dmx);
		    else if ( dmx->dmx_option & DMX_XA_END_FAIL )
			status = dmx_abort(dmx);
		    else if ( dmx->dmx_option & DMX_XA_END_SUSPEND )
		    {
			/*
			** Without TMMIGRATE support, there's no point
			** in doing anything. This thread retains ownership
			** of the transaction branch context, and Ingres
			** allows but one transaction per session anyway,
			** so just ignore the suspend.
			*/
		    }
		}
	    }
	}
    }

    return( (dmx->error.err_code) ? E_DB_ERROR : E_DB_OK );
}

/*{
** Name: dmx_xa_prepare - Prepare to commit a transaction branch.
**
**  INTERNAL DMF call format:      status = dmx_xa_prepare(&dmx_cb);
**
**  EXTERNAL call format:          status = dmf_call(DMX_XA_PREPARE,&dmx_cb);
**
** Description:
**	Prepare a disassociated transaction branch for commit
**	(phase 1 of 2-phase commit)
**
**	The transaction branch must have been previously ended via
**	dmxe_xa_end(DMX_XA_SUCCESS).
**
**	This prepare can be done by any thread of control
**	that is associated with the same DB as the branch.
**
** Inputs:
**      dmx_cb 
**          .type                           Must be set to DMX_TRANSACTION_CB.
**          .length                         Must be at least sizeof(DMX_CB).
**          .dmx_db_id                      The database identifier of the
**                                          database in this transaction that
**                                          can be updated.
**	    .dmx_dis_tran_id		    The distributed branch to prepare
**					    
** Output:
**      dmx_cb 
**          .error.err_code                 One of the following error numbers.
**                                          E_DM0000_OK                
**					    E_DM0010_BAD_DB_ID
**
**      Returns:
**          E_DB_OK                         Function completed normally. 
**          E_DB_WARN                       Function completed normally with 
**                                          a termination status which is in 
**                                          dmx_cb.error.err_code.
**          E_DB_ERROR                      Function completed abnormally with 
**                                          a termination status which is in
**                                          dmx_cb.error.err_code.
**          E_DB_FATAL                      Function completed with a fatal
**                                          error which must be handled
**                                          immediately.  The fatal status is in
**                                          dmx_cb.error.err_code.
** History:
**	26-Jun-2006 (jonj)
**	    Created.
*/
DB_STATUS
dmx_xa_prepare(
DMX_CB    *dmx)
{
    DML_ODCB		*odcb;
    DMP_DCB		*dcb;
    i4			error;
    DB_STATUS		status;

    /*  Check parameters. */

    CLRDBERR(&dmx->error);

    odcb = (DML_ODCB *)dmx->dmx_db_id;
    if (odcb && dm0m_check((DM_OBJECT *)odcb, (i4)ODCB_CB) == 0)
    {
	dcb = (DMP_DCB *)odcb->odcb_db_id;
	status = dmxe_xa_prepare(&dmx->dmx_dis_tran_id, dcb, &dmx->error);
    }
    else
    {
	TRdisplay("%@ dmx_xa_prepare E_DM0010_BAD_DB_ID");
	SETDBERR(&dmx->error, 0, E_DM0010_BAD_DB_ID);
    }

    return( (dmx->error.err_code) ? E_DB_ERROR : E_DB_OK );
}

/*{
** Name: dmx_xa_commit - Commit a transaction branch.
**
**  INTERNAL DMF call format:      status = dmx_xa_commit(&dmx_cb);
**
**  EXTERNAL call format:          status = dmf_call(DMX_XA_COMMIT,&dmx_cb);
**
** Description:
**
**	Commit a disassociated transaction branch.
**
**	The transaction branch must have been previously ended via
**	dmxe_xa_end(DMX_XA_SUCCESS).
**
**	This commit can be done by any thread of control
**	that is associated with the same DB as the branch.
**
** Inputs:
**      dmx_cb 
**          .type                           Must be set to DMX_TRANSACTION_CB.
**          .length                         Must be at least sizeof(DMX_CB).
**          .dmx_db_id                      The database identifier of the
**                                          database in this transaction that
**                                          can be updated.
**          .dmx_option                     DMX_XA_COMMIT_1PC if committing
**					    using one-phase commit 
**					    optimization.
**					    
** Output:
**      dmx_cb 
**          .error.err_code                 One of the following error numbers.
**                                          E_DM0000_OK                
**					    E_DM0010_BAD_DB_ID
**
**      Returns:
**          E_DB_OK                         Function completed normally. 
**          E_DB_WARN                       Function completed normally with 
**                                          a termination status which is in 
**                                          dmx_cb.error.err_code.
**          E_DB_ERROR                      Function completed abnormally with 
**                                          a termination status which is in
**                                          dmx_cb.error.err_code.
**          E_DB_FATAL                      Function completed with a fatal
**                                          error which must be handled
**                                          immediately.  The fatal status is in
**                                          dmx_cb.error.err_code.
** History:
**	26-Jun-2006 (jonj)
**	    Created.
*/
DB_STATUS
dmx_xa_commit(
DMX_CB    *dmx)
{
    DML_ODCB		*odcb;
    DMP_DCB		*dcb;
    i4			error;
    DB_STATUS		status;

    /*  Check parameters. */

    CLRDBERR(&dmx->error);

    odcb = (DML_ODCB *)dmx->dmx_db_id;
    if (odcb && dm0m_check((DM_OBJECT *)odcb, (i4)ODCB_CB) == 0)
    {
	dcb = (DMP_DCB *)odcb->odcb_db_id;
	status = dmxe_xa_commit(&dmx->dmx_dis_tran_id, 
				dcb, 
				(dmx->dmx_option & DMX_XA_COMMIT_1PC)
					? TRUE
					: FALSE,
				&dmx->error);
    }
    else
    {
	TRdisplay("%@ dmx_xa_commit E_DM0010_BAD_DB_ID");
	SETDBERR(&dmx->error, 0, E_DM0010_BAD_DB_ID);
    }

    return( (dmx->error.err_code) ? E_DB_ERROR : E_DB_OK );
}

/*{
** Name: dmx_xa_rollback - Rollback an XA transaction branch
**
**  INTERNAL DMF call format:      status = dmx_xa_rollback(&dmx_cb);
**
**  EXTERNAL call format:          status = dmf_call(DMX_XA_ROLLBACK,&dmx_cb);
**
** Description:
**
**	Rollback an XA distributed transaction branch.
**
**	The rollback can be requested by any thread of control
**	that is associated with the same DB as the branch.
**
**	If the branch was not ended via dmx_xa_end(DMX_XA_SUCCESS)
**	(it's still associated with a thread), then the entire
**	global transaction will be forcibly aborted
**	(the rollback-before-end scenario).
**
**	
**
** Inputs:
**      dmx_cb 
**          .type                           Must be set to DMX_TRANSACTION_CB.
**          .length                         Must be at least sizeof(DMX_CB).
**          .dmx_tran_id.                   Distributed transaction id to
**                                          be rolled back.
**          .dmx_db_id                      The database identifier of the
**                                          database for the transaction
**                                          to be rolled back.
**
** Output:
**      dmx_cb 
**          .error.err_code                 One of the following error numbers.
**                                          E_DM0000_OK                
**                                          E_DM0010_BAD_DB_ID
**
**      Returns:
**          E_DB_OK                         Function completed normally. 
**          E_DB_ERROR                      Function completed abnormally with 
**                                          a termination status which is in
**                                          dmx_cb.error.err_code.
**          E_DB_FATAL                      Function completed with a fatal
**                                          error which must be handled
**                                          immediately.  The fatal status is in
**                                          dmx_cb.error.err_code.
** History:
**	26-Jun-2006 (jonj)
**	    Created
*/
DB_STATUS
dmx_xa_rollback(
DMX_CB    *dmx)
{
    DML_ODCB		*odcb;
    DMP_DCB		*dcb;
    i4			error;
    DB_STATUS		status;

    /*  Check parameters. */

    odcb = (DML_ODCB *)dmx->dmx_db_id;
    if (odcb && dm0m_check((DM_OBJECT *)odcb, (i4)ODCB_CB) == 0)
    {
	dcb = (DMP_DCB *)odcb->odcb_db_id;
	status = dmxe_xa_rollback(&dmx->dmx_dis_tran_id, dcb, &dmx->error);
    }
    else
    {
	TRdisplay("%@ dmx_xa_rollback E_DM0010_BAD_DB_ID");
	SETDBERR(&dmx->error, 0, E_DM0010_BAD_DB_ID);
    }

    return( (dmx->error.err_code) ? E_DB_ERROR : E_DB_OK );
}
