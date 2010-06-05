/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <tr.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <adf.h>
#include    <pc.h>
#include    <dmf.h>
#include    <ulf.h>
#include    <dmrcb.h>
#include    <dmccb.h>
#include    <dmxcb.h>
#include    <dmtcb.h>
#include    <lg.h>
#include    <lk.h>
#include    <dm.h>
#include    <dml.h>
#include    <dmp.h>
#include    <dmpp.h>
#include    <dm1b.h>
#include    <dm0l.h>
#include    <dm2r.h>
#include    <dmrthreads.h>
#include    <dmftrace.h>

/**
** Name: DMRPUT.C - Implements the DMF put record operation.
**
** Description:
**      This file contains the functions necessary to put a record.
**      This file defines:
**
**      dmr_put - Put a record.
**
** History:
**      01-sep-85 (jennifer)
**          Created for Jupiter.
**	17-dec-1985 (derek)
**	    Completed code for dmr_put().
**	30-sep-1988 (rogerk)
**	    Added capability to delay writing Begin Transaction record until
**	    first write operation.  If DELAYBT flag is set in the XCB, then
**	    call dmxe to write the BT record.
**	17-mar-1989 (EdHsu)
**	    Added cluster online backup support.
**	6-June-1989 (ac)
**	    Added 2PC support.
**      28-jul-1989 (mikem)
**          Added logging of database, table, and owner when we get an internal
**          error.
**	17-jan-1990 (rogerk)
**	    Add support for not writing savepoint records in readonly xacts.
**	12-feb-1991 (mikem)
**	    Added support to dmr_put() for returning logical key values through
**	    the DMR_CB up to QEF.  This is added so that the logical key value
**	    can be returned to the client which caused the insert.
**	5-may-1992 (bryanp)
**	    Conditionalize BT logging on rcb_logging for temporary tables.
**	8-jul-1992 (walt)
**	    Prototyping DMF.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**		Include <lg.h> to pick up logging system type definitions.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (bryanp)
**	    Include <lk.h> to pick up locking system type definitions.
**	    Include <pc.h> before <lk.h> to pick up PID definition
**      31-jan-1994 (markm)
**          Added check for XCB_NOLOGGING before deciding whether to call
**          dmxe_writebt(), otherwise XCB_DELAYBT is not getting reset and
**          the transaction is considered readonly at rollback time (57857).
**	31-jan-1994 (bryanp) B58494
**	    Handle failures in both dm2r_put and dm2r_unfix_pages.
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          Unfix all pages before leaving DMF if row locking.
**      21-may-97 (stial01)
**          Row locking: No more LK_PH_PAGE locks, so page(s) can stay fixed.
**	3-jun-97 (nanpr01)
**	    Fix bug 78352 : When transactions are rolled back, temporary
**	    tables that were updated does not get dropped as documented.
**	15-oct-1998 (nanpr01)
**	    Get rid of the extra comparison. Also donot call the function 
**	    put_temporary_in_xcb_tq unless it is a temporary table.
**      05-may-1999 (nanpr01,stial01)
**          Check for DMR_DUP_ROLLBACK flag (dup error will result in rollback)
**          This allows DMF to dupcheck in indices after base table is updated.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	08-Nov-2000 (jenjo02)
**	    Deleted put_temporary_in_xcb_tq() function. This 
**	    functionality is now handled when RCB is closed.
**      14-jan-2004 (stial01)
**          Attempted to fix allocate error handling for blob etabs
**          (more fixes still needed in dmpe_put, so that dmpe_add_extension
**          gets called before dmpe_nextchain_fixup).
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
**	25-Nov-2008 (jonj)
**	    SIR 120874: dm0l_? functions converted to DB_ERROR *
**	26-Nov-2008 (jonj)
**	    SIR 120874: dm2r_? functions converted to DB_ERROR *
**      01-Dec-2008 (coomi01) b121301
**          Copy the ISDBP context bit from the DMR_CB to the RCB
*/

/*{
** Name:  dmr_put - Put a record.
**
**  INTERNAL DMF call format:      status = dmr_put(&dmr_cb);
**
**  EXTERNAL call format:          status = dmf_call(DMR_PUT, &dmr_cb);
**
** Description:
**      This function adds a record to a table.  As a result of the 
**      function a tuple is added to the table and a tuple identifier is
**      returned  to indicate where the tuple resides in the table.
**      An error for duplicate tuple or duplicate key is returned if
**      it cannot be added.  All secondary indices are automatically
**      updated.
**
** Inputs:
**      dmr_cb
**          .type                           Must be set to DMR_RECORD_CB.
**          .length                         Must be at least
**                                          sizeof(DMR_RECORD_CB) bytes.
**          .dmr_flags_mask                 Not used.
**          .dmr_access_id                  Record access identifer returned
**                                          from DMT_OPEN that identifies a
**                                          table.
**          .dmr_data.data_address          Pointer to area containing the 
**                                          record to put into the table.
**          .dmr_data.data_in_size          Size of area containing the record.
**
** Outputs:
**      dmr_cb
**          .dmr_tid                        The tuple identifier for the record
**                                          that was inserted.
**          .error.err_code                 One of the following error numbers.
**                                          E_DM0000_OK                
**                                          E_DM000B_BAD_CB_LENGTH
**                                          E_DM000C_BAD_CB_TYPE
**                                          E_DM001A_BAD_FLAG
**                                          E_DM002B_BAD_RECORD_ID
**                                          E_DM003C_BAD_TID
**                                          E_DM0042_DEADLOCK
**                                          E_DM0044_DELETED_TID
**                                          E_DM0045_DUPLICATE_KEY
**                                          E_DM0046_DUPLICATE_RECORD
**					    E_DM0047_UPDATED_TUPLE
**                                          E_DM004A_INTERNAL_ERROR
**                                          E_DM004B_LOCK_QUOTA_EXCEEDED
**					    E_DM004D_LOCK_TIMER_EXPIRED
**                                          E_DM0055_NONEXT
**                                          E_DM0065_USER_INTR
**                                          E_DM0064_USER_ABORT
**                                          E_DM0073_RECORD_ACCESS_CONFLICT
**                                          E_DM0074_NOT_POSITIONED
**					    E_DM008B_ERROR_PUTTING_RECORD
**                                          E_DM0100_DB_INCONSISTENT
**                                          E_DM010C_TRAN_ABORTED
**                                          E_DM0112_RESOURCE_QUOTA_EXCEEDED
**      Returns:
**         E_DB_OK                          Function completed normally.
**         E_DB_WARN                        Function completed normally with a 
**                                          termination status which is in
**                                          dmr_cb.err_code.
**         E_DB_ERROR                       Function completed abnormally 
**                                          with a 
**                                          termination status which is in
**                                          dmr_cb.err_code.
**         E_DB_FATAL                       Function completed with a fatal
**                                          error which must be handled
**                                          immediately.  The fatal status is in
**                                          dmr_cb.err_code.
** History:
**      01-sep-85 (jennifer)
**	    Created new for jupiter.
**	17-dec-1985 (derek)
**	    Completed code.
**	30-sep-1988 (rogerk)
**	    Added capability to delay writing Begin Transaction record until
**	    first write operation.  If DELAYBT flag is set in the XCB, then
**	    call dmxe to write the BT record.
**	17-mar-1989 (EdHsu)
**	    Added cluster online backup support.
**      28-jul-1989 (mikem)
**          Added logging of database, table, and owner when we get an internal
**          error.
**	17-jan-1990 (rogerk)
**	    Add support for not writing savepoint records in readonly xacts.
**	    Pass xcb argument to dmxe_writebt routine.
**	12-feb-1991 (mikem)
**	    Added support to dmr_put() for returning logical key values through
**	    the DMR_CB up to QEF.  This is added so that the logical key value
**	    can be returned to the client which caused the insert.
**	5-may-1992 (bryanp)
**	    Conditionalize BT logging on rcb_logging for temporary tables.
**	10-aug-93 (robf)
**	    Add support for DMR_MINI_XACT. Do a BM/EM around the PUT.
**      31-jan-1994 (markm)
**          Added check for XCB_NOLOGGING before deciding whether to call
**          dmxe_writebt(), otherwise XCB_DELAYBT is not getting reset and
**          the transaction is considered readonly at rollback time (57857).
**	31-jan-1994 (bryanp) B58494
**	    Handle failures in both dm2r_put and dm2r_unfix_pages.
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          Unfix all pages before leaving DMF if row locking.
**      21-may-97 (stial01)
**          Row locking: No more LK_PH_PAGE locks, so page(s) can stay fixed.
**	3-jun-97 (nanpr01)
**	    Fix bug 78352 : When transactions are rolled back, temporary
**	    tables that were updated does not get dropped as documented.
**	15-oct-1998 (nanpr01)
**	    Get rid of the extra comparison. Also donot call the function 
**	    put_temporary_in_xcb_tq unless it is a temporary table.
**    1-Jul-2000 (wanfr01)
**        Bug 101521, INGSRV 1226 - Pass DM2R_RAAT flag to dm2r_put
**        if this query came from RAAT
**	22-Jan-2004 (jenjo02)
**	  Added call to dmrThrOp() to parallelize index updates.
**	12-Feb-2004 (schka24)
**	    Defend against puts to a partitioned master.
**	01-Apr-2004 (jenjo02)
**	    If !DMZ_ASY_MACRO(20), don't parallelize SI updates.
**	11-Nov-2005 (jenjo02)
**	    Replaced dmx_show() with the more robust 
**	    dmxCheckForInterrupt() to standardize external
**	    interrupt handling.
**	16-Mar-2006 (wanfr01)
**	    Bug 115652
**	    dmr_put must return E_DM0112 if dm2r_put returns E_DM9247
**	    to avoid blob failures.
**	11-Sep-2006 (jonj)
**	    Don't dmxCheckForInterrupt if extended table as txn is
**	    likely in a recursive call and not at an atomic
**	    point in execution as required for LOGFULL_COMMIT.
**	03-Mar-2010 (jonj)
**	    SIR 121619 MVCC, blob support:
**	    Set rcb_dmr_opcode here; dmpe bypasses dmf_call,
**	    which used to set it.
*/
DB_STATUS
dmr_put(
DMR_CB  *dmr_cb)
{
    DMR_CB		*dmr = dmr_cb;
    DMP_RCB		*rcb, *ir;
    DML_XCB		*xcb;
    i4		flag;
    DB_STATUS		status, local_status;
    i4		error, local_error;
    bool		need_mini = FALSE;
    LG_LSN		bm_lsn;
    LG_LSN		lsn;
    DMP_TCB		*t;
    DB_ERROR		local_dberr;

    CLRDBERR(&dmr->error);

    for (status = E_DB_ERROR;;)
    {
	rcb = (DMP_RCB *)dmr->dmr_access_id;
	if (dm0m_check((DM_OBJECT *)rcb, (i4)RCB_CB) == E_DB_OK)
	{
	    rcb->rcb_dmr_opcode = DMR_PUT;

	    t = rcb->rcb_tcb_ptr;
	    if (t->tcb_rel.relstat & TCB_IS_PARTITIONED)
	    {
		uleFormat(&dmr->error, E_DM0022_BAD_MASTER_OP, NULL, ULE_LOG, NULL,
			NULL, 0, NULL, &error, 3,
			0, "dmrput",
			sizeof(DB_OWN_NAME),t->tcb_rel.relowner.db_own_name,
			sizeof(DB_TAB_NAME),t->tcb_rel.relid.db_tab_name);
		break;
	    }

	    /*
	    ** If there are indexes to be updated in parallel or
	    ** the base operation is to be done in parallel,
	    ** execute PUT in parallel.
	    */
	    if ( DMZ_ASY_MACRO(20) &&
	         !(dmr->dmr_flags_mask & DMR_AGENT_CALLBACK) )
	    {
		if ( (t->tcb_update_idx && !(dmr->dmr_flags_mask & DMR_NO_SIAGENTS))
			|| dmr->dmr_flags_mask & DMR_AGENT )
		{
		    status = dmrThrOp(DMR_PUT, dmr);
		    if ( dmr->dmr_flags_mask & DMR_AGENT )
			return(status);
		    /* Not threading the base PUT, fall through... */
		}
	    }

	    flag = DM2R_NODUPLICATES;
	    if (t->tcb_rel.relstat & TCB_DUPLICATES)
		flag = DM2R_DUPLICATES;

	    if (dmr->dmr_flags_mask & DMR_RAAT)
		flag |= DM2R_RAAT;   

	    /*
	    ** DMR_DUP_ROLLBACK is set in QEF in cases where we know a 
	    ** duplicate error condition will result in a rollback, in which
	    ** case DMF can dupcheck in indices after updating the base table.
	    ** 
	    ** If DMR_DUP_ROLLBACK is not set we must do all duplicate checking
	    ** before updating the base table. That is we must assume the
	    ** worst case, that duplicate errors will be ignored by 
	    ** QEF (or RAAT).
	    */
	    if ((dmr->dmr_flags_mask & DMR_DUP_ROLLBACK) == 0)
		flag |= DM2R_DUP_CONTINUE;
	    
	    xcb = rcb->rcb_xcb_ptr;
	    if ( dm0m_check((DM_OBJECT *)xcb, (i4)XCB_CB) )
	    {
		uleFormat(NULL, E_DM00E0_BAD_CB_PTR, (CL_ERR_DESC *)NULL, ULE_LOG, 
		    NULL, (char *)NULL, (i4)0, (i4 *)NULL, &error, 1,
		    sizeof("transaction")-1, "transaction");
		SETDBERR(&dmr->error, 0, E_DM002A_BAD_PARAMETER);
		break;
	    }
	    if ( dm0m_check((DM_OBJECT *)xcb->xcb_scb_ptr, (i4)SCB_CB) )
	    {
		uleFormat(NULL, E_DM00E0_BAD_CB_PTR, (CL_ERR_DESC *)NULL, ULE_LOG, 
		    NULL, (char *)NULL, (i4)0, (i4 *)NULL, &error, 1,
		    sizeof("session")-1, "session");
		SETDBERR(&dmr->error, 0, E_DM002A_BAD_PARAMETER);
		break;
	    }

	    /* Check for external interrupts */
	    if ( xcb->xcb_scb_ptr->scb_ui_state && !t->tcb_extended )
		dmxCheckForInterrupt(xcb, &error);

	    /*
	    ** If this is the first write operation for this transaction,
	    ** then we need to write the begin transaction record.
            **
            ** We don't call dmxe_writebt() if we have temporarily
            ** turned off logging (we turn off logging to prevent
            ** log records from being written when we process
            ** temporary tables during the execution of complex
            ** queries).  However if logging has been disabled by
            ** the "set nologging" statement (XCB_NOLOGGING) we
            ** still need to call dmxe_writebt() to ensure that
            ** XCB_DELAYBT will be reset. (57857)
	    */
	    if ( xcb->xcb_state == 0 &&
                 (xcb->xcb_flags & XCB_DELAYBT) &&
                 ((xcb->xcb_flags & XCB_NOLOGGING) ||
                  (rcb->rcb_logging != 0)) )
	    {
                i4 journal_bt = is_journal(rcb);

		status = dmxe_writebt(xcb, journal_bt, &dmr->error);
		if (status != E_DB_OK)
		{
		    xcb->xcb_state |= XCB_TRANABORT;
		    break;
		}
	    }

	    if (xcb->xcb_state == 0)
	    {
/* CRYPT_FIXME need to fix logical/physical width split and alttbl issues.
** It used to be so simple: 
**		if (dmr->dmr_data.data_address  &&
**		    dmr->dmr_data.data_in_size 
**			>= rcb->rcb_tcb_ptr->tcb_rel.relwid)
*/
		if (dmr->dmr_data.data_address  &&
		   (dmr->dmr_data.data_in_size 
			>= rcb->rcb_tcb_ptr->tcb_rel.relwid
			||
		    dmr->dmr_data.data_in_size 
			>= rcb->rcb_tcb_ptr->tcb_rel.reldatawid))
		{
		    /*
		    ** If operation is requested inside a mini-xact, do this
		    */
		    if ((dmr->dmr_flags_mask & DMR_MINI_XACT) &&
			!(xcb->xcb_flags & XCB_NOLOGGING))
		    {
			need_mini=TRUE;
			
			/*
			** Start a mini-xact
			*/
			if ((status = dm0l_bm(rcb, &bm_lsn, &dmr->error)) != E_DB_OK)
			    break;
		    }

		    /* 
		    ** Read the In-proc-body flag from the DSH context, 
		    ** and save it in the rcb control block.
		    */
		    rcb->rcb_dsh_isdbp = dmr_cb->dmr_dsh_isdbp;

		    status = dm2r_put(rcb, flag, dmr->dmr_data.data_address, 
				      &dmr->error);

		    if ( t->tcb_rel.relstat & TCB_CONCUR )
		    {
			local_status = dm2r_unfix_pages(rcb, &local_dberr);
			if (local_status != E_DB_OK)
			{
			    if (status == E_DB_OK)
			    {
				status = local_status;
				dmr->error = local_dberr;
			    }
			    else
			    {
				uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL,
					ULE_LOG, NULL, (char * )NULL, (i4)0,
					(i4 *)NULL, &local_error, 0);
			    }
			}
		    }
		    if (status == E_DB_OK)
		    {
			dmr->dmr_tid = rcb->rcb_currenttid.tid_i4;
			if (rcb->rcb_val_logkey & (RCB_TABKEY | RCB_OBJKEY))
			{
			    /* if a system maintained logical key has been 
			    ** assigned, then pass it up through the system,
			    ** to be returned to the client.
			    */
			    if (rcb->rcb_val_logkey & RCB_TABKEY)
				dmr->dmr_val_logkey = DMR_TABKEY;
			    else
				dmr->dmr_val_logkey = 0;

			    if (rcb->rcb_val_logkey & RCB_OBJKEY)
				dmr->dmr_val_logkey |= DMR_OBJKEY;

			    STRUCT_ASSIGN_MACRO(rcb->rcb_logkey, 
						dmr->dmr_logkey);
			}

		        if (need_mini)
		        {
			    /*
			    ** Finish mini-xact
			    */
			    if ((status = dm0l_em(rcb, &bm_lsn, &lsn, &dmr->error)) 
					!= E_DB_OK)
				break;
			}
			return (status);
		    }
		}
		else
		    SETDBERR(&dmr->error, 0, E_DM002A_BAD_PARAMETER);
	    }
	    else
	    {
		if (xcb->xcb_state & XCB_USER_INTR)
		    SETDBERR(&dmr->error, 0, E_DM0065_USER_INTR);
		else if (xcb->xcb_state & XCB_FORCE_ABORT)
		    SETDBERR(&dmr->error, 0, E_DM010C_TRAN_ABORTED);
		else if (xcb->xcb_state & XCB_ABORT)
		    SETDBERR(&dmr->error, 0, E_DM0064_USER_ABORT);
		else if (xcb->xcb_state & XCB_WILLING_COMMIT)
		    SETDBERR(&dmr->error, 0, E_DM0132_ILLEGAL_STMT);
		status = E_DB_ERROR;
	    }
	}	    
	else
	    SETDBERR(&dmr->error, 0, E_DM002B_BAD_RECORD_ID);
	break;
    }	    

    if (dmr->error.err_code == E_DM0022_BAD_MASTER_OP ||
	dmr->error.err_code == E_DM004B_LOCK_QUOTA_EXCEEDED ||
	dmr->error.err_code == E_DM0112_RESOURCE_QUOTA_EXCEED)
    {
	rcb->rcb_xcb_ptr->xcb_state |= XCB_STMTABORT;
    }
    else if (dmr->error.err_code == E_DM0042_DEADLOCK ||
	dmr->error.err_code == E_DM004A_INTERNAL_ERROR ||
	dmr->error.err_code == E_DM0100_DB_INCONSISTENT)
    {
	rcb->rcb_xcb_ptr->xcb_state |= XCB_TRANABORT;
    }
    else if (dmr->error.err_code == E_DM010C_TRAN_ABORTED)
    {
	rcb->rcb_xcb_ptr->xcb_state |= XCB_FORCE_ABORT;
    }
    else if (dmr->error.err_code == E_DM0065_USER_INTR)
    {
	rcb->rcb_xcb_ptr->xcb_state |= XCB_USER_INTR;
	rcb->rcb_state &= ~RCB_POSITIONED;
	*(rcb->rcb_uiptr) &= ~SCB_USER_INTR;
    }
    else if (rcb->rcb_tcb_ptr->tcb_extended &&
	(dmr->error.err_code == E_DM9245_DM1S_ALLOCATE || 
	 dmr->error.err_code == E_DM925E_DM1B_ALLOCATE))
    {
	dmr->error.err_code = E_DM0138_TOO_MANY_PAGES;
    }
    else if (dmr->error.err_code > E_DM_INTERNAL)
    {
	uleFormat(&dmr->error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
	    (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	uleFormat(NULL, E_DM904D_ERROR_PUTTING_RECORD, NULL, ULE_LOG, NULL,
	    (char * )NULL, 0L, (i4 *)NULL, &error, 3,
	    sizeof(DB_DB_NAME), &rcb->rcb_tcb_ptr->tcb_dcb_ptr->dcb_name,       
	    sizeof(DB_OWN_NAME),&rcb->rcb_tcb_ptr->tcb_rel.relowner,
	    sizeof(DB_TAB_NAME),&rcb->rcb_tcb_ptr->tcb_rel.relid
	    );
        if (dmr->error.err_code == E_DM9247_DM1H_ALLOCATE) 
	    SETDBERR(&dmr->error, 0, E_DM0112_RESOURCE_QUOTA_EXCEED);
	else
	    SETDBERR(&dmr->error, 0, E_DM008B_ERROR_PUTTING_RECORD);
    }

    /* In parallel or not, must call dmrThrError on error */
    return(dmrThrError(dmr, rcb, status));
}
