/*
**Copyright (c) 2004, 2010 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <di.h>
#include    <me.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <adf.h>
#include    <pc.h>
#include    <ulf.h>
#include    <dmf.h>
#include    <dmxcb.h>
#include    <dmccb.h>
#include    <dmtcb.h>
#include    <dmrcb.h>
#include    <dmucb.h>
#include    <lg.h>
#include    <lk.h>
#include    <dm.h>
#include    <dml.h>
#include    <dmp.h>
#include    <dmxe.h>
#include    <dm2t.h>
#include    <dm2f.h>
#include    <dm0m.h>
#include    <dmfgw.h>
#include    <dmftrace.h>
#include    <dm2rep.h>
#include    <dmpecpn.h>


/**
** Name: DMXCOMMIT.C - Functions used to commit a Transaction.
**
** Description:
**      This file contains the functions necessary to commit a transaction.
**      This file defines:
**    
**      dmx_commit()        -  Routine to perform the commit 
**                             transaction operation.
**	dmx_logfull_commit()-  Commit transaction, but keep log
**			       and locking contexts.
**
** History:
**      01-sep-85 (jennifer)
**          Created for Jupiter.      
**	25-nov-1985 (derek)
**	    Completed code fro dmx_commit().
**	30-sep-1988 (rogerk)
**	    Added capability to treat transactions as read-only until first
**	    write operation is performed.  If DELAYBT flag is still on in the
**	    XCB at commit time, then treat the transaction as a read-only
**	    transaction.
**	2-Mar-1989 (ac)
**	    Added 2PC support.
**	15-aug-1989 (rogerk)
**	    Added support for Non-SQL Gateway.
**	 4-feb-1991 (rogerk)
**	    Added support for Set Nologging.
**	17-oct-1991 (rogerk)
**	    Great Merge of DMF : Move 6.4 changes into 6.5.  In this case,
**	    the 6.4 version of this file was moved directly to 6.5 and the
**	    6.5 changes were merged back in:
**
**	    14-jun-1991 (Derek)
**		Added performance profiling support.
**	15-apr-1992 (bryanp)
**	    If error executing pending operations, log error messages.
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
**      27-dec-1994 (cohmi01)
**          For RAW files, setup additional info in DMP_LOCATION from new
**          fields in XCCB to identify location name and raw extent.
**      04-apr-1995 (dilma04) 
**          (Re)Initialize scb_isolation_level.
**	30-may-96 (stephenb)
**	    clean up (close table for) replication input queue RCB, also
**	    add code to write TX to replication transaction shared memory
**	    queue and wake queue management threads.
**	2-aug-96 (stephenb)
**	    Fixes for replication transaction queue handling in dmx_commit().
**	05-nov-1996 (somsa01)
**	    In dmx_commit(), if a temporary (etab) table is being deleted, take
**	    its dmt_cb off of the scb list so that it will not try to be
**	    deleted again in dmpe_free_temps().
**	5-feb-97 (stephenb)
**	    Add code to wake up replication queue management threads, if we
**	    are waiting for a free TXQ entry.
**      12-feb-97 (dilma04)
**          Set Lockmode Cleanup:
**          Replaced scb_isolation_level with scb_tran_iso.
**	27-Feb-1997 (jenjo02)
**	    Extended <set transaction statement> to support <transaction 
**	    access mode> (READ ONLY | READ WRITE) per SQL-92.
**	    Added SET SESSION <access mode>
**	    Reset scb_tran_am.
**	25-Aug-1997 (jenjo02)
**	    Added log_id parm to dm2t_destroy_temp_tcb() calls.
**	    Added log_id (LG_LXID) to dm2f_build_fcb(), dm2f_open_file(),
**	    dm2f_create_file(), dm2f_delete_file(), dm2f_rename(),
**	    dm2f_add_fcb() dm2f_release_fcb() prototypes so that it 
**	    can be passed on to dm2t_reclaim_tcb().
**      12-nov-97 (stial01)
**          dmx_commit() Reset scb_tran_iso = DMC_C_SYSTEM;
**	19-Nov-1997 (jenjo02)
**	    After calling dm0l_et(), zero xcb_log_id as it's no longer valid.
**	20-Jul-1998 (jenjo02)
**	    LKevent() event type and value enlarged and changed from 
**	    u_i4's to *LK_EVENT structure.
**	03-Dec-1998 (jenjo02)
**	    Pass XCB*, page size, extent start from XCCB to 
**	    dm2f_alloc_rawextent().
**	29-dec-1998 (stial01)
**          dmx_commit() if taking DMT_CB off scb->scb_lo_next queue,
**          call dm0m_deallocate().
**      19-mar-1999 (nanpr01)
**	    Deleted obsolete RAW locations code.
**      18-jan-2000 (gupsh01)
**          Added setting DMXE_NONJNLTAB in order to identify transactions
**          which alter nonjournaled tables of a journaled database.
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	05-Mar-2002 (jenjo02)
**	    Call dms_end_tran() for Sequence Generators.
**      26-jan-2006 (horda03) Bug 48659/INGSRV 2716 Additional
**          A non-journalled DB will the XCB_DELAYBT flag set when it has updated
**          a table (i.e. it is not a ROTRAN), in this circumstance the XCB_NONJNL_BT
**          flag will also be set.
**      16-oct-2006 (stial01)
**          Free locator context.
**      09-feb-2007 (stial01)
**          Moved locator context from DML_XCB to DML_SCB
**	12-Apr-2008 (kschendel)
**	    Need to include adf.h now.
**	20-Oct-2008 (jonj)
**	    SIR 120874 Modified to use new DB_ERROR based uleFormat 
**	21-Nov-2008 (jonj)
**	    SIR 120874: dmxe_? functions converted to DB_ERROR *
**	26-Nov-2008 (jonj)
**	    SIR 120874: dm2f_?, dm2t_?, dm2r_? functions converted to DB_ERROR *
**	02-Dec-2008 (jonj)
**	    SIR 120874: dm2rep_? functions converted to DB_ERROR *
**	05-Dec-2008 (jonj)
**	    SIR 120874: dmf_gw? functions converted to DB_ERROR *
**/
/*
** globals
*/
GLOBALREF	DMC_REP		*Dmc_rep;

/*{
** Name: dmx_commit - Commits a transaction.
**
**  INTERNAL DMF call format:      status = dmx_commit(&dmx_cb);
**
**  EXTERNAL call format:          status = dmf_call(DMX_COMMIT,&dmx_cb);
**
** Description:
**    The dmx_commit function handles the end of a transaction.
**    Once a transaction is committed, all internal control information
**    associated with this transaction is released.  A transaction cannot
**    be committed until all tables assocaited with the transaction are
**    closed.  After the transaction is comitted but before it releases
**    the xcb, any files pended for deletion are deleted or any temporary
**    table pended for destruction are destroyed.
**
**    If the transaction was begun with DMX_CONNECT, the assumption is
**    that this is a child thread of some sort, and the controller
**    thread still has the transaction open.  In that case we just
**    disconnect.  (The XCB_SHARED_TXN flag tells us what to do.)
**
** Inputs:
**      dmx_cb 
**          .type                           Must be set to DMX_TRANSACTION_CB.
**          .length                         Must be at least sizeof(DMX_CB).
**          .dmx_tran_id.                   Must be a valid transaction 
**                                          identifier returned from the 
**                                          begin transaction operation.
**
** Output:
**      dmx_cb 
**          .dmx_commit_time		    The commit timestamp.
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
**					    E_DM0095_ERROR_COMMITING_TRAN
**                                          E_DM0100_DB_INCONSISTENT
**                                          E_DM010C_TRAN_ABORTED
**                                          E_DM0112_RESOURCE_QUOTA_EXCEEDED
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
**      01-sep-85 (jennifer)
**          Created for Jupiter.
**	25-nov-1985 (derek)
**	    Completed code;
**	30-sep-1988 (rogerk)
**	    Added capability to treat transactions as read-only until first
**	    write operation is performed.  If DELAYBT flag is still on in the
**	    XCB at commit time, then treat the transaction as a read-only
**	    transaction.
**	2-Mar-1989 (ac)
**	    Added 2PC support.
**	15-aug-1989 (rogerk)
**	    Added support for Non-SQL Gateway.  If server is connected to a
**	    gateway which requires begin/end transaction notification, then
**	    we must notify the gateway of the end transaction.
**	 4-feb-1991 (rogerk)
**	    Added support for Set Nologging.  If transaction is marked
**	    XCB_NOLOGGING, then log the commit as a Read-only transaction.
**	17-oct-1991 (rogerk)
**	    Great Merge of DMF : Move 6.4 changes into 6.5.  In this case,
**	    the 6.4 version of this file was moved directly to 6.5 and the
**	    6.5 changes were merged back in:
**
**	    14-jun-1991 (Derek)
**		Added performance profiling support.
**	15-apr-1992 (bryanp)
**	    Log any error messages returned by execution of pending operations.
**	27-dec-1994 (cohmi01)
**	    For RAW files, setup additional info in DMP_LOCATION from new 
**	    fields in XCCB to identify location name and raw extent.
**      04-apr-1995 (dilma04) 
**          (Re)Initialize scb_isolation_level.
**	30-may-96 (stephenb)
**	    clean up (close table for) replication input queue RCB, also
**	    add code to write TX to replication transaction shared memory
**	    queue and wake queue management threads.
**	2-aug-96 (stephenb)
**	    if the xcb_rep_remote_tx has been set (by the replication server)
**	    then use this value in the replication transaction queue.
**	05-nov-1996 (somsa01)
**	    If a temporary (etab) table is being deleted, take its dmt_cb off
**	    of the scb list so that dmpe_free_temps() will not try to delete
**	    it again.
**      12-feb-97 (dilma04)
**          Set Lockmode Cleanup:
**          Replaced scb_isolation_level with scb_tran_iso.
**	27-Feb-1997 (jenjo02)
**	    Extended <set transaction statement> to support <transaction access mode>
**	    (READ ONLY | READ WRITE) per SQL-92.
**	    Added SET SESSION <access mode>
**	    Reset scb_tran_am.
**	2-jun-97 (stephenb)
**	    if xcb_rep_input_q has been set to -1, then the user opened the
**	    input queue by hand (normaly done by replicator management 
**	    applications), in this case it will be closed during normal user
**	    cleanup, it doesn't need to be closed here.
**      12-nov-97 (stial01)
**          dmx_commit() Reset scb_tran_iso = DMC_C_SYSTEM;
**	19-Nov-1997 (jenjo02)
**	    After calling dm0l_et(), zero xcb_log_id as it's no longer valid.
**	29-may-98 (stephenb)
**	    Re-design replicator transaction queue processing to avoid
**	    a resource deadlock. Rather than waiting for a free entry when the
**	    transaction queue is full, we'll just distribute the transaction
**	    ourselves.
**	03-Dec-1998 (jenjo02)
**	    Pass XCB*, page size, extent start from XCCB to 
**	    dm2f_alloc_rawextent().
**	4-dec-98 (stephenb)
**	    Replicator trans_time is now an HRSYSTIME, use TMhrnow() instead
**	    if TMsecs().
**	10-may-1999 (thaju02)
**	    For willing commit xactions, if database is replicated, open the
**	    dd_input_queue table. (B96878)
**      18-jan-2000 (gupsh01)
**          Added setting DMXE_NONJNLTAB in order to identify transactions
**          which alter nonjournaled tables of a journaled database.
**	18-Apr-2002 (jenjo02)
**	    Previous fix for B96878 incomplete. Replicator input queue
**	    must be processed at willing-commit time, not here.
**	10-Sep-2003 (inifa01) Bug 110795 INGREP 147
**	    The Replicator server hangs during 2PC due to block on
**	    dd_distrib_queue lock request. 
**	    FIX: Repserver is triggering distribution when in 2PC b/cos
**	    dd_input_queue will be opened here, which will lead to calling
**	    dm2rep_qman() if the transaction queue is full.  Fix was to
**	    distinguish b/w user initiated 2PC and repserver 2PC.  
**	30-Apr-2004 (schka24)
**	    If transaction was started with DMX_CONNECT, just do a
**	    disconnect instead of a real commit.
**	10-May-2004 (schka24)
**	    Clean up open blob operations.
**      31-aug-2004 (sheco02)
**          X-integrate change 466484 to main and replace longnat with i4.
**      21-Jun-2005 (sheco02)
**          Back out the previous X-integration because the related block of 
**	    code was moved to dmxsecure.c by jenjo02 with change 456831.
**	28-Jul-2005 (schka24)
**	    Back out x-integration for bug 111502 (not shown), doesn't
**	    apply to r3.
**	26-Jul-2006 (jonj)
**	    Add DMX_XA_END_SUCCESS support for XA integration.
**	01-Aug-2006 (jonj)
**	    Add lock_id to dmxe_xa_disassoc() prototype.
**	28-Aug-2006 (jonj)
**	    On XA_END_SUCCESS, relocate any XCCB_F_DELETE files to the
**	    Global Transaction's LXB where they await deletion when the
**	    GT finally commits.
**	25-Aug-2006 (kibro01) bug 114168
**	    Tell db2rep_qman that this is a genuine attempt to perform qman,
**	    and not a hijacked open_db call.
**	05-Sep-2006 (jonj)
**	    Check for XCB_ILOG (internal logging) in an otherwise
**	    read-only transaction.
**      26-jan-2006 (horda03) Bug 48659/INGSRV 2716 Additional
**          Check that the TX has the XCB_DELAYBT flag set, but not the
**          XCB_NONJNL_BT flag when deciding if the TX is Read Only
**          (DMXE_ROTRAN) or not.
**	29-Sep-2006 (jonj)
**	    Tweak horda03 change, above. Don't DMXE_JOURNAL ET if 
**	    only non-journaled BT was written.
**      28-Jun-2007 (kibro01) b118559
**          Add in flag to allow lock release to take place after the
**          commit, so that transaction temporary tables can keep their
**          lock until they are destroyed.
**	29-Jun-2007 (kibro01) b118566
**	    If rep_txq_size is 0 there will be no Dmc_rep set, since there
**	    is no replication transaction queue at all for asynchronous
**	    distribution, so always do the distribution synchronously and
**	    don't check the Dmc_rep list or semaphore.
**	29-Oct-2007 (kibro01) b119369
**	    Don't release locks from an XA transaction at this stage.
**	30-Oct-2007 (kibro01) b119369
**	    Do not delay unlock when disassociating from an XA transaction,
**	    since temporary tables are already destroyed, and it prevents
**	    locks ever being released.
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Deallocate lctx, jctx if allocated.
**	23-Mar-2010 (kschendel) SIR 123448
**	    Release xccb list mutex when all done.
**	13-Apr-2010 (kschendel) SIR 123485
**	    Force LOB query-end upon commit.
**	20-May-2010 (thaju02) Bug 123427
**	    Pass xcb->xcb_lk_id to dm2rep_qman().
*/
DB_STATUS
dmx_commit(
DMX_CB    *dmx_cb)
{
    DML_SCB             *scb;
    DMX_CB		*dmx = dmx_cb;
    DMT_CB		*dmt_cb;
    DMT_CB              *next_dmt_cb;
    DMP_RCB		*rcb;
    DMP_DCB		*dcb;
    DML_XCB		*xcb;
    DML_XCCB            *xccb;
    DML_SPCB            *spcb;
    DB_TAB_ID		tbl_id;
    DB_STATUS		status = E_DB_ERROR;
    i4			commit_flags;
    i4			error,local_error;
    DB_STATUS		local_status;
    DI_IO               f;
    CL_ERR_DESC		sys_err;
    DB_TAB_TIMESTAMP    ctime;
    DI_IO		di_file;
    i4			filelength;
    DMP_LOCATION        loc;
    i4             	loc_count = 1;
    DMP_LOC_ENTRY       ext;
    DMP_FCB             fcb;
    STATUS		cl_stat;
    i4			qnext;
    DMC_REPQ		*repq = (DMC_REPQ *)(Dmc_rep + 1);
    i4			rep_iq_lock;
    i4			rep_maxlocks;
    DB_TAB_TIMESTAMP	timestamp;
    LK_EVENT		lk_event;
    DB_ERROR		local_dberr;

    CLRDBERR(&dmx->error);

    for (;;)
    {
	xcb = (DML_XCB *)dmx->dmx_tran_id;
	if (dm0m_check((DM_OBJECT *)xcb, (i4)XCB_CB) != E_DB_OK)
	{
	    SETDBERR(&dmx->error, 0, E_DM003B_BAD_TRAN_ID);
	    break;
	}

	if (xcb->xcb_state & (XCB_STMTABORT | XCB_TRANABORT))
	{
	    SETDBERR(&dmx->error, 0, E_DM0064_USER_ABORT);
	    break;
	}
        scb = xcb->xcb_scb_ptr;
	dcb = xcb->xcb_odcb_ptr->odcb_dcb_ptr;

        /*
        ** clean up replication input queue RCB
	**
	** For txn in willing commit, this will already have
	** been done by dmx_secure().
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
		    /* queue is full, print warning and distribute manually */
		    if (qnext == Dmc_rep->queue_start && 
			Dmc_rep->queue_start != Dmc_rep->queue_end)
		    {
			list_ok = FALSE;
		    }
		} else
		{
	            TRdisplay("%@ dmxcommit() CSp_semaphore failed %d\n",
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
            	    uleFormat(NULL, W_DM9561_REP_TXQ_FULL, (CL_ERR_DESC *)NULL, 
			ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL, 
			&local_error, 0);
		}

		status = dm2rep_qman(dcb,
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
	        STRUCT_ASSIGN_MACRO(dcb->dcb_name,
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
                    break;
	    }
        }

	/*  Clear user interrupt state in SCB. */

	scb->scb_ui_state &= ~SCB_USER_INTR;

	/* Clear the force abort state of the SCB. */

	if (xcb->xcb_state & XCB_FORCE_ABORT)
	    scb->scb_ui_state &= ~SCB_FORCE_ABORT;
 
        if (xcb->xcb_flags & XCB_USER_TRAN)
        {
	    /*
            ** forget any transaction-duration isolation level
	    ** and access mode:
	    */
            scb->scb_tran_iso = DMC_C_SYSTEM;
	    scb->scb_tran_am = 0;
        }

	/* Clean up any Sequence Generator references */
	if ( (xcb->xcb_seq || xcb->xcb_cseq) &&
	    (status = dms_end_tran(DMS_TRAN_COMMIT, xcb, &dmx->error)) )
	{
	    break;
	}

	/* Tell LOB processing that it's over (probably happened already).
	** Don't however drop holding temps!  we might have just finished
	** couponifying LOB's (from the sequencer), or the commit might have
	** been issued from a nested DBP and LOB's might still exist in
	** the outer DBP.
	** Do this before forcible table close so that we can close any
	** opens attached to the BQCB, nicely.
	*/
	status = dmpe_query_end(FALSE, FALSE, &dmx->error);
	if (status != E_DB_OK)
	    break;

	/* Clean up any blob memory that is dangling.
	** (shouldn't be any if commit?)
	*/
	while (xcb->xcb_pcb_list != NULL)
	    dmpe_deallocate(xcb->xcb_pcb_list);

	/*  
	** Close all open tables and destroy all open temporary tables.
	** If the transaction is in the WILLING COMMIT state, all the
	** tables should have been closed at the WILLING COMMIT time.
	*/

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
		    SETDBERR(&dmx->error, 0, E_DM0095_ERROR_COMMITING_TRAN);
		}
	    
		xcb->xcb_state |= XCB_TRANABORT;
		return (status);
	    }
	}
	/* One more swipe at BQCB's now that all tables are closed.
	** This call will definitely delete all BQCB's.
	** (dmpe-query-end perhaps should have been named "dmpe-query-end-
	** as-much-as-you-can-for-now"!  The call above is there to neatly
	** close out loads and logical-key-generators.  This call, after
	** forcible table close, guarantees deletion of the BQCB's.)
	*/
	(void) dmpe_query_end(FALSE, FALSE, &dmx->error);

	/*
	** If this server is connected to a gateway, then notify the gateway
	** of the transaction commit.
	*/
	if (dmf_svcb->svcb_gw_xacts)
	{
	    status = dmf_gwx_commit(scb, &dmx->error);
	    if (status != E_DB_OK)
		break;
	}

	/*
	** Check for commit flags.
	** If XCB_DELAYBT state is still turned on, then there was no
	** journaled write operation done during this transaction, so it may be
	** treated as a readonly transaction. However, if XCB_NONJNL_BT state is 
	** turned on, then there were non-journaled writes and this is not
	** a readonly transaction.
	**
	** If the transaction is marked NOLOGGING, then indicate to the
	** logging system that it was READONLY - even though we may have
	** performed update operations.
	**
	** XCB_ILOG indicates internal logging in perhaps an otherwise
	** read-only transaction.
        **
        ** A TX can still have the XCB_DELAYBT set for a an UPDATE
        ** TX if a non-journalled BT record has been written.
	*/
	/* Delay unlocking until after temp table destruction */
	/* ...but only when this is NOT an XA transaction finally completing
	** (kibro01) b119369
	*/
	if (dmx->dmx_option & DMX_XA_END_SUCCESS)
	    commit_flags = 0;
	else
	    commit_flags = DMXE_DELAY_UNLOCK;

	if ((xcb->xcb_x_type & XCB_RONLY && !(xcb->xcb_flags & XCB_ILOG)) || 
            (xcb->xcb_flags & XCB_NOLOGGING) ||
              ((xcb->xcb_flags & (XCB_DELAYBT|XCB_NONJNL_BT)) == XCB_DELAYBT))
	    commit_flags |= DMXE_ROTRAN;
	/* If the BT was not journaled, then do not journal the ET */
	if ( dcb->dcb_status & DCB_S_JOURNAL &&
	    !(xcb->xcb_flags & XCB_NONJNL_BT) )
	    commit_flags |= DMXE_JOURNAL;
	if (xcb->xcb_state & XCB_WILLING_COMMIT)
	    commit_flags |= DMXE_WILLING_COMMIT;
	if (xcb->xcb_flags & XCB_NONJNLTAB)
	    commit_flags |= DMXE_NONJNLTAB;
	if (xcb->xcb_flags & XCB_SHARED_TXN)
	    commit_flags |= DMXE_CONNECT;
	if ( xcb->xcb_flags & XCB_NOLOGGING )
	    commit_flags |= DMXE_NOLOGGING;

	/*
	** If doing xa_end(TMSUCCESS), we throw away everything
	** about the DMF transaction, but keep and disassociate the log/lock
	** context, awaiting xa_prepare, xa_commit, xa_rollback.
	** Note that we keep information about the state of the txn
	** "commit_flags" in the log context.
	**
	** Now we also shuffle any accumulated pending file deletes
	** to the Global Transaction's LXB. If we execute the deletes
	** now, a subsequent rollback of the GT will fail to find
	** the deleted files and recovery will fail.
	*/
	if ( dmx->dmx_option & DMX_XA_END_SUCCESS )
	{
	    /* Shouldn't have other threads accessing our XCCB list at
	    ** this point, don't bother with mutex.
	    */
	    xccb = xcb->xcb_cq_next;

	    while ( xccb != (DML_XCCB*) &xcb->xcb_cq_next )
	    {
		LG_FDEL		fdel;
		DML_XCCB	*nxccb;
		/* Get pend XCCB */

		nxccb = xccb->xccb_q_next;

		if (xccb->xccb_operation == XCCB_F_DELETE)
		{
		    /* Remove from xcb list, post to GT's LXB, deallocate XCCB */
		    xccb->xccb_q_prev->xccb_q_next = xccb->xccb_q_next;
		    xccb->xccb_q_next->xccb_q_prev = xccb->xccb_q_prev;

		    fdel.fdel_id = (LG_LXID)xcb->xcb_log_id;
		    MEcopy((char*)&xccb->xccb_p_name,
				sizeof(fdel.fdel_path),
				(char*)fdel.fdel_path);
		    fdel.fdel_plen = xccb->xccb_p_len;
		    MEcopy((char*)&xccb->xccb_f_name,
				sizeof(fdel.fdel_file),
				(char*)fdel.fdel_file);
		    fdel.fdel_flen = xccb->xccb_f_len;

		    cl_stat = LGalter(LG_A_FILE_DELETE,
					(PTR)&fdel,
					sizeof(fdel),
					&sys_err);
		    if ( cl_stat )
		    {
			/* Report any errors, but keep going */
			uleFormat( NULL, cl_stat, NULL, ULE_LOG , NULL, 
				(char * )NULL, 0L, (i4 *)NULL, &error, 0);
			uleFormat( NULL, E_DM0095_ERROR_COMMITING_TRAN,
				NULL, ULE_LOG , NULL, 
				(char * )NULL, 0L, (i4 *)NULL, &error, 0);
		    }
		    dm0m_deallocate((DM_OBJECT **)&xccb);
		}
		xccb = nxccb;
	    }

	    status = dmxe_xa_disassoc(&xcb->xcb_dis_tran_id, 
				      xcb->xcb_log_id,
				      xcb->xcb_lk_id,
				      commit_flags, &dmx->error);
	}
	else
	{
	    /*  Regular old commit: commit the transaction. */
	    status = dmxe_commit(&xcb->xcb_tran_id, xcb->xcb_log_id, 
			 xcb->xcb_lk_id, commit_flags, &ctime, &dmx->error);
	    STRUCT_ASSIGN_MACRO(ctime, dmx->dmx_commit_time);
	}

	if (status != E_DB_OK)
	{
	    if ((dmx->error.err_code != E_DM004B_LOCK_QUOTA_EXCEEDED) &&
		(dmx->error.err_code != E_DM0112_RESOURCE_QUOTA_EXCEED) &&
		(dmx->error.err_code != E_DM0095_ERROR_COMMITING_TRAN) &&
		(dmx->error.err_code != E_DM0100_DB_INCONSISTENT) &&
		(dmx->error.err_code != E_DM004A_INTERNAL_ERROR))
	    {
		uleFormat( &dmx->error, 0, NULL, ULE_LOG , NULL, 
		    (char * )NULL, 0L, (i4 *)NULL, 
		    &local_error, 0);

		if ( dmx->dmx_option & DMX_XA_END_SUCCESS )
		    SETDBERR(&dmx->error, 0, E_DM012F_ERROR_XA_END);
		else
		    SETDBERR(&dmx->error, 0, E_DM0095_ERROR_COMMITING_TRAN);
	    }

	    xcb->xcb_state |= XCB_TRANABORT;
	    return (E_DB_ERROR);
	}

	STRUCT_ASSIGN_MACRO(ctime, dmx->dmx_commit_time);

	/*
	** We're now past the moment of commit. The transaction has truly
	** committed. Any errors at this point get logged, but do not cause
	** the transaction to be aborted. dmx_commit() still ends up returning
	** E_DB_OK.
	*/

	/* 
        ** See if there are any files pended for delete, or any
        ** temporary tables pended for destroy.
	** At this stage, any child threads should be disconnected,
	** and there shouldn't be any list contention; so don't mutex.
        */
	while (xcb->xcb_cq_next != (DML_XCCB*) &xcb->xcb_cq_next)
	{
	    /* Get pend XCCB */

	    xccb = xcb->xcb_cq_next;
	    xccb->xccb_q_prev->xccb_q_next = xccb->xccb_q_next;
	    xccb->xccb_q_next->xccb_q_prev = xccb->xccb_q_prev;
	    if (xccb->xccb_operation == XCCB_F_DELETE)
	    {
		MEcopy((char *)&xccb->xccb_p_name, 
		    sizeof(DM_PATH), (char *)&ext.physical);
		ext.phys_length = xccb->xccb_p_len;
		MEcopy((char *)&xccb->xccb_f_name, 
		    sizeof(DM_FILE), (char *)&fcb.fcb_filename);
		fcb.fcb_namelength = xccb->xccb_f_len;
		fcb.fcb_di_ptr = &di_file;
		fcb. fcb_last_page = 0;
		fcb.fcb_location = (DMP_LOC_ENTRY *) 0;
		fcb.fcb_state = FCB_CLOSED;
		loc.loc_ext = &ext;
		loc.loc_fcb = &fcb;
		/* Location id not important since file name is 
		** xccb. Set to zero. */
		loc.loc_id = 0;
		loc.loc_status = 0;    /* not really a valid loc */

		local_status = dm2f_delete_file(scb->scb_lock_list, 
					xcb->xcb_log_id, &loc,
					loc_count, (DM_OBJECT *)dcb, (i4)0,
					&local_dberr);
		if (local_status)
		{
		    uleFormat( &local_dberr, 0, NULL, ULE_LOG , NULL, 
			    (char * )NULL, 0L, (i4 *)NULL, &local_error, 0);
		    uleFormat( NULL, E_DM0095_ERROR_COMMITING_TRAN,
			    NULL, ULE_LOG , NULL, 
			    (char * )NULL, 0L, (i4 *)NULL, &local_error, 0);
		}
	    }
	    else if (xccb->xccb_operation == XCCB_T_DESTROY)
	    {
		DB_ERROR	local_dberror;

		local_status = dmt_destroy_temp(scb, xccb, &local_dberror);
		/*
		** It's fine if the temporary table is already gone; any other
		** error, however, should be logged.
		*/
		if (local_status != E_DB_OK &&
		    local_dberror.err_code != E_DM0054_NONEXISTENT_TABLE)
		{
		    uleFormat( &local_dberror, 0, NULL, ULE_LOG , NULL, 
			    (char * )NULL, 0L, (i4 *)NULL, &local_error, 0);
		    uleFormat( NULL, E_DM0095_ERROR_COMMITING_TRAN,
			    NULL, ULE_LOG , NULL, 
			    (char * )NULL, 0L, (i4 *)NULL, &local_error, 0);
		}

	    }
	    dm0m_deallocate((DM_OBJECT **)&xccb);
	} /* end while */
    
        /* DON'T release transaction locks for XA_END (kibro01) b119369 */
        if ( (dmx->dmx_option & DMX_XA_END_SUCCESS ) == 0)
        {

	    status = LKrelease(LK_ALL, xcb->xcb_lk_id, (LK_LKID *)0,
		(LK_LOCK_KEY *)0, (LK_VALUE *)0, &sys_err);

	    if (status != E_DB_OK)
	    {
                uleFormat(NULL, cl_stat, &sys_err, ULE_LOG, NULL,
                        (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
                uleFormat(NULL, E_DM901B_BAD_LOCK_RELEASE, &sys_err, ULE_LOG, NULL,
                        (char *)NULL, (i4)0, (i4 *)NULL, &error, 3,
                        0, LK_E_CLR, 0, REP_READQ, 0, xcb->xcb_lk_id);
		SETDBERR(&dmx->error, 0, E_DM9501_DMXE_COMMIT);
                xcb->xcb_state |= XCB_TRANABORT;
	        return (E_DB_ERROR);
	    }
	}

	/* The commit has released both the logging and locking contexts */
	xcb->xcb_log_id = xcb->xcb_lk_id = 0;

	/*
	**  Flush the savepoint list of savepoints newer than the aborted
	**  savepoint.
	*/

	while (xcb->xcb_sq_prev != (DML_SPCB*) &xcb->xcb_sq_next)
	{
	    /*	Get previous SPCB. */

	    spcb = xcb->xcb_sq_prev;

	    /*	Remove SPCB from XCB queue. */

	    spcb->spcb_q_next->spcb_q_prev = spcb->spcb_q_prev;
	    spcb->spcb_q_prev->spcb_q_next = spcb->spcb_q_next;

	    /*	Deallocate the SPCB. */

	    dm0m_deallocate((DM_OBJECT **)&spcb);
	}

	/*  Remove XCB from SCB queue. */

	xcb->xcb_q_next->xcb_q_prev = xcb->xcb_q_prev;
	xcb->xcb_q_prev->xcb_q_next = xcb->xcb_q_next;
        scb->scb_x_ref_count--;

#ifdef xDEBUG
	/*
	** Report statistics collected in transaction control block.
	*/
	if (DMZ_SES_MACRO(20))
	    dmd_txstat(xcb, 1);
#endif

	/* Deallocate lctx, jctx if allocated */
	if ( xcb->xcb_lctx_ptr )
	    dm0m_deallocate(&xcb->xcb_lctx_ptr);
	if ( xcb->xcb_jctx_ptr )
	    (void) dm0j_close(&xcb->xcb_jctx_ptr, &dmx->error);

	dm0s_mrelease(&xcb->xcb_cq_mutex);
	dm0m_deallocate((DM_OBJECT **)&xcb);

	return (E_DB_OK);
    }

    return (status);
}

/*{
** Name: dmx_logfull_commit - Commit transaction, keep log/locking
**			  	 contexts.
**
**  INTERNAL DMF call format:      status = dmx_logfull_commit(xcb, error);
**
** Description:
**
**    The dmx_logfull_commit function silently commits changes made by
**    a transaction but retains the transaction's log and
**    lock contexts.
**
**    Locks held by the transaction are retained.
**
**    This function is called during a LOGFULL event when the 
**    transaction has been selected for force-abort and the
**    transaction's session option specifies ON_LOGFULL COMMIT.
**    This may occur "mid-query" but should only be detected
**    at an atomic DMF update point, i.e. not during secondary
**    index processing or in the midst of a mini-transaction
**    or recursive blob processing.
**
** Inputs:
**      xcb 				    The DML XCB of the txn.
**
** Output:
**      xcb 				    Is reinit'd to look like
**					    the txn has just begun.
**	    xcb_tran_id			    Set to newly assigned
**					    internal tran_id from LG.
**	error				    Why it failed, if it did.
**
**      Returns:
**          E_DB_OK                         Function completed normally. 
**          E_DB_ERROR                      Function completed abnormally with 
**                                          a termination status which is in
**					    *error.
** History:
**	20-Apr-2004 (jenjo02)
**	    Created for on_logfull commit.
**	11-Jun-2004 (schka24)
**	    Lock the odcb cq list in case we have || threads at this point.
**	11-Sep-2006 (jonj)
**`	    Make work with parallel threads, shared transactions:
**	    Don't write NOTIFY message to log again if (shared) txn
**	    force-abort state has already been reset.
**	29-Sep-2006 (jonj)
**	    Don't DMXE_JOURNAL ET if only non-journaled BT was written.
*/
DB_STATUS
dmx_logfull_commit(
DML_XCB    *xcb,
DB_ERROR   *error)
{
    DML_SPCB            *spcb;
    DML_XCCB		*xccb;
    DML_ODCB		*odcb;
    DMP_RCB		*rcb;
    DML_XCB		*next_r_queue;
    DB_STATUS		status;
    i4			local_error;
    i4			length;
    i4			commit;
    CL_ERR_DESC		sys_err;
    STATUS		stat;
    DB_TAB_TIMESTAMP    ctime;
    LG_TRANSACTION	lg_tran, *tr = &lg_tran;
    LG_TRANSACTION	new_lg_tran, *new_tr = &new_lg_tran;

    CLRDBERR(error);

    /* If this (piece of a shared) txn did no writes, do nothing */
    if ( (xcb->xcb_flags & (XCB_DELAYBT | XCB_NONJNL_BT)) == XCB_DELAYBT )
	return(E_DB_OK);

    odcb = xcb->xcb_odcb_ptr;

    /* Get information about the (shared) log transaction */
    MEcopy((char *)&xcb->xcb_log_id, sizeof(xcb->xcb_log_id), (char *)tr);
    stat = LGshow(LG_S_STRANSACTION, (PTR)tr, sizeof(*tr), &length, &sys_err);
    if ((stat != OK) || (length == 0))
    {
	if (stat)
	{
	    uleFormat(NULL, stat, &sys_err, ULE_LOG, NULL,
		(char *)NULL, 0L, (i4 *)NULL, &local_error, 0);
	}
	uleFormat(error, E_DM9017_BAD_LOG_SHOW, &sys_err, ULE_LOG, NULL,
	    (char *)NULL, 0L, (i4 *)NULL, &local_error, 1, 0, LG_S_STRANSACTION);
	return (E_DB_ERROR);
    }

    /* Don't even try if distributed transaction */
    if ( tr->tr_status & (TR_DISTRIBUTED | TR_WILLING_COMMIT) )
	return(E_DB_ERROR);

    /* Must unfix all pages that are fixed */
    for ( next_r_queue = (DML_XCB*)xcb->xcb_rq_next;
	  next_r_queue != (DML_XCB*)&xcb->xcb_rq_next;
	  next_r_queue = next_r_queue->xcb_q_next )
    {
	rcb = (DMP_RCB *)((char *)next_r_queue -
		    (char *)&(((DMP_RCB*)0)->rcb_xq_next));

	if ( status = dm2r_unfix_pages(rcb, error) )
	{
	    if (error->err_code != E_DM004A_INTERNAL_ERROR)
		uleFormat( error, 0, NULL, ULE_LOG , NULL, 
				(char * )NULL, 0L, (i4 *)NULL, 
				&local_error, 0);
	    return (status);
	}
    }

    commit = DMXE_LOGFULL_COMMIT;

    if ( xcb->xcb_odcb_ptr->odcb_dcb_ptr->dcb_status & DCB_S_JOURNAL &&
	!(xcb->xcb_flags & XCB_NONJNL_BT) )
	commit |= DMXE_JOURNAL;
    if (xcb->xcb_flags & XCB_NONJNLTAB)
	commit |= DMXE_NONJNLTAB;

    /*
    **  Now commit the transaction, keeping log/locks.
    **
    **  Logging will assign a new internal transaction id
    **  to our log context (xcb_log_id) which we'll plug
    **  into our XCB.
    */
    if ( status = dmxe_commit(&xcb->xcb_tran_id, xcb->xcb_log_id, 
		 xcb->xcb_lk_id, commit, &ctime, error) )
    {
	uleFormat( error, 0, NULL, ULE_LOG , NULL, 
	    (char * )NULL, 0L, (i4 *)NULL, 
	    &local_error, 0);
    }
    else
    {
	/* Reget transaction info to update XCB with new tran id */
	MEcopy((char *)&xcb->xcb_log_id, sizeof(xcb->xcb_log_id), (char *)new_tr);
	stat = LGshow(LG_S_STRANSACTION, (PTR)new_tr, sizeof(*new_tr), &length, &sys_err);
	if ((stat != OK) || (length == 0))
	{
	    if (stat)
	    {
		uleFormat(NULL, stat, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, 0L, (i4 *)NULL, &local_error, 0);
	    }
	    uleFormat(error, E_DM9017_BAD_LOG_SHOW, &sys_err, ULE_LOG, NULL,
		(char *)NULL, 0L, (i4 *)NULL, &local_error, 1, 0, LG_S_STRANSACTION);
	    return (E_DB_ERROR);
	}

	/* Update this XCB's transaction id */
	xcb->xcb_tran_id = new_tr->tr_eid;

	/*
	** Now set the delay_bt flag so that the next update,
	** if any, will write a nw BT log record to begin
	** the new log transaction;
	*/
	xcb->xcb_flags &= ~XCB_NONJNL_BT;
	xcb->xcb_flags |= XCB_DELAYBT;

	if ( xcb->xcb_flags & XCB_SHARED_TXN )
	{
	    DML_XCB	*Pxcb = odcb->odcb_scb_ptr->scb_x_next;

	    if ( Pxcb->xcb_tran_id.db_high_tran != xcb->xcb_tran_id.db_high_tran ||
		 Pxcb->xcb_tran_id.db_low_tran  != xcb->xcb_tran_id.db_low_tran )
	    {
		Pxcb->xcb_flags &= ~XCB_NONJNL_BT;
		Pxcb->xcb_flags |= XCB_DELAYBT;
	    }
	}


	/* Nothing now to rollback to */
	xcb->xcb_sp_id = 0;

	/*  Flush (invalidate) the savepoint list */

	while (xcb->xcb_sq_prev != (DML_SPCB*) &xcb->xcb_sq_next)
	{
	    /*	Get previous SPCB. */

	    spcb = xcb->xcb_sq_prev;

	    /*	Remove SPCB from XCB queue. */

	    spcb->spcb_q_next->spcb_q_prev = spcb->spcb_q_prev;
	    spcb->spcb_q_prev->spcb_q_next = spcb->spcb_q_next;

	    /*	Deallocate the SPCB. */

	    dm0m_deallocate((DM_OBJECT **)&spcb);
	}

	/*
	** Reset savepoint ids in all of session's
	** temporary tables. If any of these tables
	** get updated by this transaction, this
	** savepoint id will get updated as well.
	*/

	dm0s_mlock(&odcb->odcb_cq_mutex);
	for ( xccb = odcb->odcb_cq_next;
	      xccb != (DML_XCCB*)&odcb->odcb_cq_next;
	      xccb = xccb->xccb_q_next )
	{
	    if ( xccb->xccb_operation == XCCB_T_DESTROY )
		xccb->xccb_sp_id = -1;
	}

	/*
	** If SHARED txn, also update the Parent's BT xcb_flags
	** if it hasn't already been done for this new transaction.
	** Do it while holding the cq_mutex.
	** See similar mutexed tomfoolery in dmxe_writebt.
	*/
	if ( xcb->xcb_flags & XCB_SHARED_TXN )
	{
	    DML_XCB	*Pxcb = odcb->odcb_scb_ptr->scb_x_next;

	    if ( Pxcb->xcb_tran_id.db_high_tran != xcb->xcb_tran_id.db_high_tran ||
		 Pxcb->xcb_tran_id.db_low_tran  != xcb->xcb_tran_id.db_low_tran )
	    {
		Pxcb->xcb_flags &= ~XCB_NONJNL_BT;
		Pxcb->xcb_flags |= XCB_DELAYBT;
	    }
	}
	dm0s_munlock(&odcb->odcb_cq_mutex);

	/* Write warning message to log if NOTIFY */
	if ( xcb->xcb_scb_ptr->scb_sess_mask & SCB_LOGFULL_NOTIFY &&
		     tr->tr_status & TR_FORCE_ABORT )
	{
	    uleFormat(NULL, E_DM00AC_LOGFULL_COMMIT, NULL, ULE_LOG , 
		NULL, (char * )NULL, 0L, (i4 *)NULL, 
		&local_error, 6,
		0, tr->tr_eid.db_high_tran,
		0, tr->tr_eid.db_low_tran,
		sizeof(odcb->odcb_dcb_ptr->dcb_name.db_db_name),
		    odcb->odcb_dcb_ptr->dcb_name.db_db_name,
		0, tr->tr_last.la_sequence,
		0, tr->tr_last.la_block,
		0, tr->tr_last.la_offset);
	}
    }

    return (status);
}
