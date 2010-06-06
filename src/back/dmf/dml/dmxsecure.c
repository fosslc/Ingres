/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <me.h>
#include    <tr.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <adf.h>
#include    <pc.h>
#include    <ulf.h>
#include    <dmf.h>
#include    <dmxcb.h>
#include    <dmrcb.h>
#include    <dmccb.h>
#include    <dmtcb.h>
#include    <lg.h>
#include    <lk.h>
#include    <dm.h>
#include    <dml.h>
#include    <dmp.h>
#include    <dmpp.h>
#include    <dm1b.h>
#include    <dm0l.h>
#include    <dm2t.h>
#include    <dmxe.h>
#include    <dm2rep.h>
#include    <dmftrace.h>
#include    <dmd.h>

/**
** Name: DMXSECURE.C - Functions used to Prepare to Commit a Transaction.
**
** Description:
**      This file contains the functions necessary to prepare to commit a 
**	transaction.
**      This file defines:
**    
**      dmx_secure()	-  Routine to perform the prepare to commit 
**			   transaction operation.
**
** History:
**	20-May-1988 (ac)
**	    created.
**	17-jan-1990 (rogerk)
**	    Added check for transaction which has not written a BT record.
**	8-jul-1992 (walt)
**	    Prototyping DMF.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**		Include <lg.h> to pick up logging system typedefs
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (bryanp)
**	    Include <lk.h> to pick up locking system type definitions.
**	    Include <pc.h> before <lk.h> to pick up PID definition
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      29-nov-2005 (horda03) Bug 48659/INGSRV 2716
**          A TX can now be started with a non-journaled BT record. This
**          leaves the XCB_DELAYBT flag set but sets the XCB_NONJNL_BT
**          flag. Need to write a journaled BT record if the update
**          will be journaled.
**	12-Apr-2008 (kschendel)
**	    Need to include adf.h now.
**	20-Oct-2008 (jonj)
**	    SIR 120874 Modified to use new DB_ERROR based uleFormat 
**	21-Nov-2008 (jonj)
**	    SIR 120874: dmxe_? functions converted to DB_ERROR *
**	28-Nov-2008 (jonj)
**	    SIR 120874: dm2t_? functions converted to DB_ERROR *
**	02-Dec-2008 (jonj)
**	    SIR 120874: dm2rep_? functions converted to DB_ERROR *
**/
/*
** globals
*/
GLOBALREF	DMC_REP		*Dmc_rep;

/*{
** Name: dmx_secure - Prepare to commit a transaction.
**
**  INTERNAL DMF call format:      status = dmx_secure(&dmx_cb);
**
**   EXTERNAL call format:          status = dmf_call(DMX_SECURE,
**					     &dmx_cb);
**
** Description:
**   The dmx_secure function prepares to commit a distributed slave 
**   transaction. The distributed transaction ID is required to associate
**   with the local transaction. This distributed transaction ID will then
**   be carried around in the local DBMS for re-association and recovery of
**   the local transaction.
**
** Inputs:
**     dmx_cb 
**         .type                           Must be set to DMX_TRANSACTION_CB.
**         .length                         Must be at least sizeof(DMX_CB).
**         .dmx_tran_id.                   Must be a valid transaction 
**                                         identifier returned from the 
**                                         begin transaction operation.
**         .dmx_dis_tran_id                Transaction identifier of the 
**					   distributed transaction.
** Output:
**      dmx_cb 
**          .error.err_code                 One of the following error numbers.
**                                          E_DM0000_OK                
**                                          E_DM000B_BAD_CB_LENGTH
**                                          E_DM000C_BAD_CB_TYPE
**                                          E_DM001A_BAD_FLAG
**                                          E_DM003B_BAD_TRAN_ID
**                                          E_DM004A_INTERNAL_ERROR
**                                          E_DM004B_LOCK_QUOTA_EXCEEDED
**                                          E_DM0061_TRAN_NOT_IN_PROGRESS
**                                          E_DM0063_TRAN_TABLE_OPEN
**                                          E_DM0064_USER_ABORT
**                                          E_DM0065_USER_INTR
**					    E_DM009A_ERROR_SECURE_TRAN
**                                          E_DM0100_DB_INCONSISTENT
**                                          E_DM010C_TRAN_ABORTED
**                                          E_DM0112_RESOURCE_QUOTA_EXCEEDED
**					    E_DM0119_TRAN_ID_NOT_UNIQUE
**                     
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
**	12-Jan-1989 (ac)
**	    Completed code;
**	17-jan-1990 (rogerk)
**	    Added check for transaction which has not written a BT record.
**	    If this is the case, then force a BT record before putting the
**	    xact in WILLING_COMMIT state.
**      25-sep-1992 (nandak)
**          Copy the entire distributed transaction id.
**	10-may-1999 (thaju02)
**	    Close dd_input_queue table. (B96878)
**	18-Apr-2002 (jenjo02)
**	    Fix for B96878 still may leave replicator pages fixed
**	    in cache leading to DM9C01/DM9301. Before putting
**	    txn in willing commit, process the replicator input
**	    queue and then close dd_input_queue table.
**	27-jul-2006 (kibro01) bug 114168
**	    Tell dm2rep_qman that this is a genuine attempt to perform qman,
**	    and not a hijacked open_db.
**	31-Mar-2005 (inifa01) Bug 110795 INGREP 147
**          The Replicator server hangs during 2PC due to block on dd_distrib_queue
**          lock request. 
**          FIX: Repserver is triggering distribution when in 2PC b/cos dd_input_queue
**          will be opened here, which will lead to calling dm2rep_qman() if the 
**          transaction queue is full.  Fix was to distinguish b/w user initiated
**          2PC and repserver 2PC.  
**	29-Jun-2007 (kibro01) b118566
**          If rep_txq_size is 0 there will be no Dmc_rep set, since there
**          is no replication transaction queue at all for asynchronous
**          distribution, so always do the distribution synchronously and
**          don't check the Dmc_rep list or semaphore.
**	20-May-2010 (thaju02) Bug 123427
**	    Pass xcb->xcb_lk_id to dm2rep_qman().
*/

DB_STATUS
dmx_secure(
DMX_CB    *dmx_cb)
{
    DML_SCB             *scb;
    DMX_CB		*dmx = dmx_cb;
    DML_XCB		*xcb;
    DB_STATUS		status = E_DB_ERROR;
    i4		error,local_error;
    DMP_RCB		*rcb;
    CL_ERR_DESC		sys_err;
    STATUS		cl_stat;
    i4			qnext;
    DMC_REPQ		*repq = (DMC_REPQ *)(Dmc_rep + 1);
    i4			rep_iq_lock;
    i4			rep_maxlocks;
    DB_TAB_TIMESTAMP	timestamp;
    LK_EVENT		lk_event;

    CLRDBERR(&dmx->error);

    for (;;)
    {
	xcb = (DML_XCB *)dmx->dmx_tran_id;
	if (dm0m_check((DM_OBJECT *)xcb, (i4)XCB_CB) != E_DB_OK)
	{
	    SETDBERR(&dmx->error, 0, E_DM003B_BAD_TRAN_ID);
	    break;
	}

	if (dmx->dmx_dis_tran_id.db_dis_tran_id_type == 0)
	{
	    SETDBERR(&dmx->error, 0, E_DM002A_BAD_PARAMETER);
	    break;
	}
	if (xcb->xcb_state & (XCB_STMTABORT | XCB_TRANABORT))
	{
	    SETDBERR(&dmx->error, 0, E_DM0064_USER_ABORT);
	    break;
	}
	if (xcb->xcb_state & XCB_WILLING_COMMIT)
	{
	    SETDBERR(&dmx->error, 0, E_DM0132_ILLEGAL_STMT);
	    break;
	}
        scb = xcb->xcb_scb_ptr;

	/*  Clear user interrupt state in SCB. */

	scb->scb_ui_state &= ~SCB_USER_INTR;

	/* Clear the force abort state of the SCB. */

	if (xcb->xcb_state & XCB_FORCE_ABORT)
	    scb->scb_ui_state &= ~SCB_FORCE_ABORT;

	/*
	** If the transaction has written no Begin Transaction record,
	** then force one to the log file now.
	*/
	if (xcb->xcb_flags & XCB_DELAYBT)
	{
	    status = dmxe_writebt(xcb, TRUE, &dmx->error);
	    if (status != E_DB_OK)
	    {
		xcb->xcb_state |= XCB_TRANABORT;
		break;
	    }
	}

	/*
	** If the database is replicated, the input queue must
	** be processed before putting the transaction in a
	** willing commit state.
	**
	** All of this transaction's tables, including those
	** associated with replication, must be closed and
	** their pages tossed from the cache.
	**
	** This is the same processing as that for a 
	** "normal" commit.
	*/
	if ((xcb->xcb_odcb_ptr->odcb_dcb_ptr->dcb_status & DCB_S_REPLICATE) &&
	    (DMZ_SES_MACRO(32) == 0 || dmd_reptrace() == FALSE) &&
	    ((xcb->xcb_rep_input_q == NULL) || 
		(xcb->xcb_rep_input_q == (DMP_RCB *)-1)))
	{
	    if (dmf_svcb->svcb_rep_iqlock == DMC_C_ROW)
		rep_iq_lock = DM2T_RIX;
	    else if (dmf_svcb->svcb_rep_iqlock == DMC_C_TABLE)
		rep_iq_lock = DM2T_X;
	    else
		rep_iq_lock = DM2T_IX;
 
	    /* allow a minimum maxlocks value of 50 ... */
	    if (dmf_svcb->svcb_rep_dtmaxlock > 50)
		rep_maxlocks = dmf_svcb->svcb_rep_dtmaxlock;
	    /* ...but default to 100 */
	    else if (dmf_svcb->svcb_lk_maxlocks > 100)
		rep_maxlocks = dmf_svcb->svcb_lk_maxlocks;
	    else
		rep_maxlocks = 100;
	    
	    status = dm2t_open(xcb->xcb_odcb_ptr->odcb_dcb_ptr, 
		&xcb->xcb_odcb_ptr->odcb_dcb_ptr->rep_input_q, rep_iq_lock,
                DM2T_UDIRECT, DM2T_A_WRITE, (i4)0, rep_maxlocks,
                (i4)0, xcb->xcb_log_id, xcb->xcb_lk_id, (i4)0, 
		(i4)0, (i4)0, &(xcb->xcb_tran_id), &timestamp, 
		&(xcb->xcb_rep_input_q), (DML_SCB *)0, &dmx->error);
	    if (status != E_DB_OK)
	    {
		xcb->xcb_state |= XCB_TRANABORT;
		break;
	    }
	}
        /*
        ** clean up replication input queue RCB
        */
        if (xcb->xcb_rep_input_q)
        {
	    HRSYSTIME		curtime;
	    bool	list_ok = TRUE;
	    i4		semaphore_status = -1;

	    (VOID)TMhrnow(&curtime);

	    /* If rep_txq_size=0, Dmc_rep will be NULL and we have to
	    ** do the distribution synchronously now (kibro01) b118566
	    */
	    if (Dmc_rep == NULL)
		list_ok = FALSE;

	    /*
	    ** if we had the input queue open, then we need to distribute
	    */
	    if (list_ok)
	    {
	        semaphore_status = CSp_semaphore(TRUE, &Dmc_rep->rep_sem);
		if (semaphore_status == E_DB_OK)
		{
		    qnext = (Dmc_rep->queue_start == Dmc_rep->queue_end &&
			repq[Dmc_rep->queue_start].active == 0 &&
			repq[Dmc_rep->queue_start].tx_id == 0) ?
			  Dmc_rep->queue_end :
			  (Dmc_rep->queue_end + 1) % Dmc_rep->seg_size;
		    if (qnext == Dmc_rep->queue_start && 
			Dmc_rep->queue_start != Dmc_rep->queue_end)
		    {
			list_ok = FALSE;
		    }
		} else
		{
	            TRdisplay("%@ dmxsecure() CSp_semaphore failed %d\n",
                        semaphore_status);

		    /* Error getting the semaphore, can't rely on the list */
		    list_ok = FALSE;
		}
	    }
	    if (!list_ok)
	    {
		/* queue is full, print warning and distribute manually */
		if (semaphore_status == E_DB_OK)
			CSv_semaphore(&Dmc_rep->rep_sem);

		/* If we have a queue and it's full, log that fact */
		if (Dmc_rep)
		{
            	    uleFormat(NULL, W_DM9561_REP_TXQ_FULL, NULL, ULE_LOG, NULL, 
		        (char *)NULL, (i4)0, (i4 *)NULL, &local_error, 0);
		}

		status = dm2rep_qman(xcb->xcb_odcb_ptr->odcb_dcb_ptr,
		    xcb->xcb_rep_remote_tx ? xcb->xcb_rep_remote_tx :
			(i4)xcb->xcb_tran_id.db_low_tran,
		    &curtime, xcb->xcb_rep_input_q, xcb->xcb_lk_id, 
		    &dmx->error, FALSE);
		if (status != E_DB_OK)
		{
		    if (dmx->error.err_code > E_DM004A_INTERNAL_ERROR)
		    {
		        uleFormat( &dmx->error, 0, NULL, ULE_LOG , NULL, 
				    (char * )NULL, 0L, (i4 *)NULL, 
				    &local_error, 0);
			SETDBERR(&dmx->error, 0, E_DM957F_REP_DISTRIB_ERROR);
		    }
		    xcb->xcb_state |= XCB_TRANABORT;
		    break;
		}
	    }
	    else
	    {
	        /* we have mutex and an entry we can use, so fill out entry */
	        /* we assume we are only accessing one db in this tx */
	        STRUCT_ASSIGN_MACRO(xcb->xcb_odcb_ptr->odcb_dcb_ptr->dcb_name,
		    repq[qnext].dbname);
	        /*
	        ** use remote TX if it's set (by the replication server)
	        */
	        if (xcb->xcb_rep_remote_tx)
		    repq[qnext].tx_id = xcb->xcb_rep_remote_tx;
	        else
	            repq[qnext].tx_id = (i4)xcb->xcb_tran_id.db_low_tran;
	        repq[qnext].active = FALSE;
	        MEcopy((char *)&curtime, sizeof(HRSYSTIME), 
		       (char *)&repq[qnext].trans_time);
	        Dmc_rep->queue_end = qnext;
	        CSv_semaphore(&Dmc_rep->rep_sem);
	        /*
	        ** signal the queue management thread(s)
	        */
		lk_event.type_high = REP_READQ;
		lk_event.type_low = 0;
		lk_event.value = REP_READQ_VAL;
		
	        cl_stat = LKevent(LK_E_CLR | LK_E_CROSS_PROCESS, xcb->xcb_lk_id,
		    &lk_event, &sys_err);
                if (cl_stat != OK)
                {
                    uleFormat(NULL, cl_stat, &sys_err, ULE_LOG, NULL,
                        (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
                    uleFormat(&dmx->error, E_DM904B_BAD_LOCK_EVENT, &sys_err, ULE_LOG, 
			NULL, (char *)NULL, (i4)0, (i4 *)NULL, &error, 3,
                        0, LK_E_CLR, 0, REP_READQ, 0, xcb->xcb_lk_id);
                    status = E_DB_ERROR;
                    break;
                }
	    }
	    /*
	    ** user updates to the input queue will cause xcb_rep_input_q to
	    ** be set to -1, the table will already be closed by the user in
	    ** this case
	    */
	    if (xcb->xcb_rep_input_q != (DMP_RCB *)-1)
	    {
                status = dm2t_close(xcb->xcb_rep_input_q, (i4)0, &dmx->error);

		if (status != E_DB_OK)
		{
		    if (dmx->error.err_code != E_DM004A_INTERNAL_ERROR)
		    {
			uleFormat( &dmx->error, 0, NULL, ULE_LOG , NULL,
			    (char * )NULL, 0L, (i4 *)NULL,
			    &local_error, 0);
			SETDBERR(&dmx->error, 0, E_DM009A_ERROR_SECURE_TRAN);
		    }
		    xcb->xcb_state |= XCB_TRANABORT;
		    return (status);
		}
		xcb->xcb_rep_input_q = (DMP_RCB*)NULL;
	    }
        }

	/*  Close all open tables and destroy all open temporary tables.    */

	while (xcb->xcb_rq_next != (DMP_RCB*) &xcb->xcb_rq_next)
	{
	    /*	Get next RCB. */

	    rcb = (DMP_RCB *)((char *)xcb->xcb_rq_next -
		(char *)&(((DMP_RCB*)0)->rcb_xq_next));

	    /*	Remove from the XCB. */

	    rcb->rcb_xq_next->rcb_q_prev = rcb->rcb_xq_prev;
	    rcb->rcb_xq_prev->rcb_q_next = rcb->rcb_xq_next;

	    /*	Deallocate the RCB. */

	    status = dm2t_close(rcb, (i4)0, &dmx->error);
	    if (status != E_DB_OK)
	    {
		if (dmx->error.err_code != E_DM004A_INTERNAL_ERROR)
		{
		    uleFormat( &dmx->error, 0, NULL, ULE_LOG , NULL, 
			(char * )NULL, 0L, (i4 *)NULL, 
			&local_error, 0);
		    SETDBERR(&dmx->error, 0, E_DM009A_ERROR_SECURE_TRAN);
		}
		xcb->xcb_state |= XCB_TRANABORT;
		return (status);
	    }
	}

	/*  Now prepare to commit the transaction. */

	STRUCT_ASSIGN_MACRO(dmx->dmx_dis_tran_id, xcb->xcb_dis_tran_id);

	status = dmxe_secure(&xcb->xcb_tran_id, &xcb->xcb_dis_tran_id,
				xcb->xcb_log_id, xcb->xcb_lk_id, &dmx->error);
	if (status != E_DB_OK)
	{
	    if (dmx->error.err_code > E_DM_INTERNAL)
	    {
		uleFormat(&dmx->error, 0, NULL, ULE_LOG, NULL, 
			    (char * )NULL, (i4)0, (i4 *)NULL, &error, 0);
		SETDBERR(&dmx->error, 0, E_DM009A_ERROR_SECURE_TRAN);
	    }

	    xcb->xcb_state |= XCB_TRANABORT;
	    return (E_DB_ERROR);
	}

	/* Mark the state of the transaction in xcb to WILLING COMMIT. */

	xcb->xcb_x_type |= XCB_DISTRIBUTED;
	xcb->xcb_state |= XCB_WILLING_COMMIT;
	return (E_DB_OK);
    }
    return (status);
}
