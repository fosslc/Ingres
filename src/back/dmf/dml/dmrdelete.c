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
#include    <dmacb.h>
#include    <lg.h>
#include    <lk.h>
#include    <dm.h>
#include    <dml.h>
#include    <dmp.h>
#include    <dmxe.h>
#include    <dm2r.h>
#include    <dmrthreads.h>
#include    <dmftrace.h>

/**
** Name: DMRDELETE.C - Implements the DMF delete record operation.
**
** Description:
**      This file contains the functions necessary to delete a record.
**      This file defines:
**
**      dmr_delete - Delete a record.
**
** History:
**      01-sep-85 (jennifer)
**          Created for Jupiter.
**	17-dec-1985 (derek)
**	    Completed code from dmr_delete().
**	30-sep-1988 (rogerk)
**	    Added capability to delay writing Begin Transaction record until
**	    first write operation.  If DELAYBT flag is set in the XCB, then
**	    call dmxe to write the BT record.
**	6-June-1989 (ac)
**	    Added 2PC support.
**	17-jan-1990 (rogerk)
**	    Add support for not writing savepoint records in readonly xacts.
**	5-may-1992 (bryanp)
**	    Conditionalize BT logging on rcb_logging for temporary tables.
**	8-jul-1992 (walt)
**	    Prototyping DMF.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**		Include <lg.h> to pick up logging system type definitions
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (bryanp)
**	    Include <lk.h> to pick up locking system type definitions.
**	    Include <pc.h> before <lk.h> to pick up PID definition
**      31-jan-1994 (markm)
**          Added check for XCB_NOLOGGING before deciding whether to call
**          dmxe_writebt(), otherwise XCB_DELAYBT is not getting reset and
**          the transaction is considered readonly at rollback time (57857).
**	31-jan-1994 (bryanp) B58416
**	    Handle failure in both dm2r_delete and dm2r_unfix_pages.
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          Unfix all pages before leaving DMF if row locking
**      14-may-97 (dilma04)
**          Cursor Stability Project:
**          If isolation level is CS or RR, set RCB_CSRR_LOCK locking mode 
**          for the time of dm2r_delete() call.
**      21-may-97 (stial01)
**          Row locking: No more LK_PH_PAGE locks, so page(s) can stay fixed.
**	3-jun-97 (nanpr01)
**	    Fix bug 78352 : When transactions are rolled back, temporary
**	    tables that were updated does not get dropped as documented.
**	31-Mar-1998 (hanal04)
**	    b89589 - Maintain cursor stability even when we are deleting
**	    by tid. Failure to do so leads to physical locks being held
**	    for the duration of the tx when we should be taking logical locks.
**	15-oct-1998 (nanpr01)
**	    Get rid of the extra comparison. Also donot call the function
**	    put_temporary_in_xcb_tq unless it is a temporary table.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	08-Nov-2000 (jenjo02)
**	    Deleted put_temporary_in_xcb_tq() function. This 
**	    functionality is now handled when RCB is closed.
**	12-Apr-2008 (kschendel)
**	    Need to include adf.h now.
**	20-Oct-2008 (jonj)
**	    SIR 120874 Modified to use new DB_ERROR based uleFormat 
**	21-Nov-2008 (jonj)
**	    SIR 120874: dmxe_? functions converted to DB_ERROR *
**	26-Nov-2008 (jonj)
**	    SIR 120874: dm2r_? functions converted to DB_ERROR *
*/

/*{
** Name:  dmr_delete - Delete a record.
**
**  INTERNAL DMF call format:      status = dmr_delete(&dmr_cb);
**
**  EXTERNAL call format:          status = dmf_call(DMR_DELETE, &dmr_cb);
**
** Description:
**      This function deletes a record from a table.  All secondary indices
**      are automatically updated.  The function can be directed to delete the
**      current record, or a specific record identified by a tuple identifier.
**      This implies that the record must be read to set current position
**      or to determine its tuple identifier, prior to calling delete.
**
** Inputs:
**      dmr_cb
**          .type                           Must be set to DMR_RECORD_CB.
**          .length                         Must be at least
**                                          sizeof(DMR_RECORD_CB) bytes.
**          .dmr_flags_mask                 Must DMR_CURRENT_POS  or 
**                                          DMR_BY_TID.
**          .dmr_access_id                  Record access identifer returned
**                                          from DMR_OPEN that identifies a
**                                          table.
**          .dmr_tid                        If dmr_flags_mask = DMR_BY_TID use
**                                          this as tuple identifier of record
**                                          to delete.
** Outputs:
**      dmr_cb
**          .error.err_code                 One of the following error numbers.
**                                          E_DM0000_OK                
**                                          E_DM000B_BAD_CB_LENGTH
**                                          E_DM000C_BAD_CB_TYPE
**                                          E_DM001A_BAD_FLAG
**                                          E_DM002B_BAD_RECORD_ID
**                                          E_DM003C_BAD_TID
**                                          E_DM0042_DEADLOCK
**                                          E_DM0044_DELETED_TID
**					    E_DM0047_CHANGED_TUPLE
**                                          E_DM004A_INTERNAL_ERROR
**                                          E_DM004B_LOCK_QUOTA_EXCEEDED
**					    E_DM004D_LOCK_TIMER_EXPIRED
**                                          E_DM0055_NONEXT
**                                          E_DM0065_USER_INTR
**                                          E_DM0064_USER_ABORT
**                                          E_DM0073_RECORD_ACCESS_CONFLICT
**                                          E_DM0074_NOT_POSITIONED
**					    E_DM008D_ERROR_DELETING_RECORD
**                                          E_DM0100_DB_INCONSISTENT
**                                          E_DM010C_TRAN_ABORTED
**
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
**	    Added code to support cluster online backup support.
**	17-jan-1990 (rogerk)
**	    Add support for not writing savepoint records in readonly xacts.
**	    Pass xcb argument to dmxe_writebt routine.
**	5-may-1992 (bryanp)
**	    Conditionalize BT logging on rcb_logging for temporary tables.
**      31-jan-1994 (markm)
**          Added check for XCB_NOLOGGING before deciding whether to call
**          dmxe_writebt(), otherwise XCB_DELAYBT is not getting reset and
**          the transaction is considered readonly at rollback time (57857).
**	31-jan-1994 (bryanp) B58416
**	    Handle failure in both dm2r_delete and dm2r_unfix_pages.
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          Unfix all pages before leaving DMF if row locking.
**      14-may-97 (dilma04)
**          Cursor Stability Project:
**          If isolation level is CS or RR, set RCB_CSRR_LOCK locking mode 
**          for the time of dm2r_delete() call.
**      21-may-97 (stial01)
**          Row locking: No more LK_PH_PAGE locks, so page(s) can stay fixed.
**	3-jun-97 (nanpr01)
**	    Fix bug 78352 : When transactions are rolled back, temporary
**	    tables that were updated does not get dropped as documented.
**	31-Mar-1998 (hanal04)
**	    b89589 - Maintain cursor stability even when we are deleting
**	    by tid. Failure to do so leads to physical locks being held
**	    for the duration of the tx when we should be taking logical locks.
**	15-oct-1998 (nanpr01)
**	    Get rid of the extra comparison. Also donot call the function
**	    put_temporary_in_xcb_tq unless it is a temporary table.
**  	1-Jul-2000 (wanfr01)
**	    Bug 101521, INGSRV 1226 - Pass DM2R_RAAT flag to dm2r_delete
**	    if this query came from RAAT
**	22-Jan-2004 (jenjo02)
**	  Added call to dmrThrOp() to parallelize index updates.
**	12-Feb-2004 (schka24)
**	    Preventive check for partition master, don't allow.
**	01-Apr-2004 (jenjo02)
**	    If !DMZ_ASY_MACRO(20), don't parallelize SI updates.
**	11-Nov-2005 (jenjo02)
**	    Replaced dmx_show() with the more robust 
**	    dmxCheckForInterrupt() to standardize external
**	    interrupt handling.
**      29-nov-2005 (horda03) Bug 48659/INGSRV 2716
**          A TX can now be started with a non-journaled BT record. This
**          leaves the XCB_DELAYBT flag set but sets the XCB_NONJNL_BT
**          flag. Need to write a journaled BT record if the update
**          will be journaled.
**	11-Sep-2006 (jonj)
**	    Don't dmxCheckForInterrupt if extended table as txn is
**	    likely in a recursive call and not at an atomic
**	    point in execution as required for LOGFULL_COMMIT.
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: No CSRR_LOCK when crow_locking().
**	03-Mar-2010 (jonj)
**	    SIR 121619 MVCC, blob support:
**	    Set rcb_dmr_opcode here; dmpe bypasses dmf_call,
**	    which used to set it.
*/
DB_STATUS
dmr_delete(
DMR_CB   *dmr_cb)
{
    DMR_CB		*dmr = dmr_cb;
    DMP_RCB		*rcb;
    DMP_TCB		*t;
    DML_XCB		*xcb;
    i4		flag;
    DB_STATUS		status, local_status;
    i4		error, local_error;
    DB_ERROR		local_dberr;

    CLRDBERR(&dmr->error);

    for (status = E_DB_ERROR;;)
    {
	rcb = (DMP_RCB *)dmr->dmr_access_id;
	if (dm0m_check((DM_OBJECT *)rcb, (i4)RCB_CB) == E_DB_OK)
	{
	    rcb->rcb_dmr_opcode = DMR_DELETE;

	    t = rcb->rcb_tcb_ptr;
	    if (t->tcb_rel.relstat & TCB_IS_PARTITIONED)
	    {
		uleFormat(&dmr->error, E_DM0022_BAD_MASTER_OP, NULL, ULE_LOG, NULL,
			NULL, 0, NULL, &local_error, 3,
			0, "dmrdelete",
			sizeof(DB_OWN_NAME),t->tcb_rel.relowner.db_own_name,
			sizeof(DB_TAB_NAME),t->tcb_rel.relid.db_tab_name);
		break;
	    }

	    /*
	    ** If there are indexes to be updated in parallel or
	    ** the base operation is to be done in parallel,
	    ** execute DELETE in parallel.
	    */
	    if ( DMZ_ASY_MACRO(20) &&
	         !(dmr->dmr_flags_mask & DMR_AGENT_CALLBACK) )
	    {
		if ( (t->tcb_update_idx && !(dmr->dmr_flags_mask & DMR_NO_SIAGENTS))
			|| dmr->dmr_flags_mask & DMR_AGENT )
		{
		    status = dmrThrOp(DMR_DELETE, dmr);
		    if ( dmr->dmr_flags_mask & DMR_AGENT )
			return(status);
		    /* Not threading the base DELETE, fall through... */
		}
	    }

	    if (dmr->dmr_flags_mask & DMR_CURRENT_POS)
		flag = DM2R_BYPOSITION;
	    else if (dmr->dmr_flags_mask & DMR_BY_TID)
		flag = DM2R_BYTID;
	    else
	    {
		SETDBERR(&dmr->error, 0, E_DM001A_BAD_FLAG);
		break;
	    }
	    if (dmr->dmr_flags_mask & DMR_RAAT) 
		flag |= DM2R_RAAT;

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
                /* b89589 */
		if ( !crow_locking(rcb) &&
		       (rcb->rcb_iso_level == RCB_CURSOR_STABILITY ||
			rcb->rcb_iso_level == RCB_REPEATABLE_READ) )
                {
                    rcb->rcb_state |= RCB_CSRR_LOCK; 
                }

		status = dm2r_delete(rcb, (DM_TID *)&dmr->dmr_tid, 
                             flag, &dmr->error);

		if (t->tcb_rel.relstat & TCB_CONCUR)
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

                rcb->rcb_state &= ~RCB_CSRR_LOCK;

		if (status == E_DB_OK)
		    return (status);
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
    else if (dmr->error.err_code > E_DM_INTERNAL)
    {
	uleFormat(&dmr->error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
	    (char * )NULL, (i4)0, (i4 *)NULL, &error, 0);
	SETDBERR(&dmr->error, 0, E_DM008D_ERROR_DELETING_RECORD);
    }

    /* In parallel or not, must call dmrThrError on error */
    return(dmrThrError(dmr, rcb, status));
}
