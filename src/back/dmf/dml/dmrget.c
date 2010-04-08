/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <me.h>
#include    <sl.h>
#include    <sr.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <pc.h>
#include    <tm.h>
#include    <adf.h>
#include    <dmf.h>
#include    <ulf.h>
#include    <sxf.h>
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
#include    <dmse.h>
#include    <dm1b.h>
#include    <dm2umct.h>
#include    <dm2r.h>
#include    <dm2rlct.h>
#include    <dm0m.h>
#include    <dma.h>
#include    <gwf.h>

/**
** Name: DMRGET.C - Implements the DMF get record operation.
**
** Description:
**      This file contains the functions necessary to get a record.
**      This file defines:
**
**      dmr_get - Get a record.
**
** History:
**      01-sep-85 (jennifer)
**          Created for Jupiter.
**	17-dec-1985 (derek)
**	    Completed code for dmr_get().
**	6-June-1989 (ac)
**	    Added 2PC support.
**	28-jul-1989 (mikem)
**	    Added logging of database, table, and owner when we get an internal
**	    error.
**	15-aug-1989 (rogerk)
**	    Added support for Non-SQL Gateway.
**	13-jan-1991 (rachael)
**	    Added checks for null pointers on rcb, xcb, tcb and scb.  This
**	    was done in response to several customers reporting access
**	    violations here causing the server to crash.  Now, the current
**	    transaction will abort.
**	11-Oct-1991 Merge65 24-sep-1990 (rogerk)
**	    Merged 6.3 changes into 6.5.
**      05-May-1992 (schang)
**          GW merge
**	 8-jul-1992 (walt)
**	    Prototyping DMF.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**		Include <lg.h> to pick up logging system type definitions.
**	28-may-1993 (robf)
**	   Secure 2.0: Reworked old ORANGE code.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (bryanp)
**	    Include <lk.h> to pick up locking system type definitions.
**	    Include <pc.h> before <lk.h> to pick up PID definition
**	23-aug-1993 (bryanp)
**	    Fix a few cut-and-paste errors in some error message parameters.
**	31-jan-1994 (bryanp) B58487
**	    Handle failures in both dm2r_get and dm2r_unfix_pages.
**      30-aug-1994 (cohmi01)
**          Add DMR_PREV support for FASTPATH rel. Error if not btree.
**	22-may-1995 (cohmi01)
**	    Add support for count-only, for aggregate optimization.
**	21-aug-1995 (cohmi01)
**	    count-only aggregate code moved to dml!dmragg.c
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          Unfix all pages before leaving DMF if row locking.
**      14-may-97 (dilma04)
**          Cursor Stability Project:
**          - upgrade isolation level from CS to RR, if DMR_SORT flag is set;
**          - if isolation level is CS or RR, set RCB_CSRR_LOCK locking mode 
**          for the time of dm2r_get() call.
**      21-may-97 (stial01)
**          Row locking: No more LK_PH_PAGE locks, so page(s) can stay fixed.
**      16-nov-98 (stial01)
**          Check for btree primary key projection
**      02-may-2000 (stial01)
**          Check for DMR_RAAT flag 
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      11-aug-2003 (chash01) 110755/INGGAT345
**          For RMS Gateway index table, add specific test to make sure
**          dmr->dmr_data.data_in_size + sizeof(DM_TID)
**          is no more than the value in table_width
**      18-feb-2004 (stial01)
**          Fixed incorrect casting of length arguments.
**	20-Oct-2008 (jonj)
**	    SIR 120874 Modified to use new DB_ERROR based uleFormat 
**	25-Nov-2008 (jonj)
**	    SIR 120874: dmse_?, dm2r_? functions converted to DB_ERROR *
**	25-Aug-2009 (kschendel) 121804
**	    Need dm0m.h to satisfy gcc 4.3.
**          
**/

/*{
** Name:  dmr_get - Get a record.
**
**  INTERNAL DMF call format:      status = dmr_get(&dmr_cb);
**
**  EXTERNAL call format:          status = dmf_call(DMR_GET,&dmr_cb);
**
** Description:
**      This function gets a record from a table.  It can either get a record
**      by tuple identifier, re-get the last record returned, or get the
**      next record that passes the qualification specified by the dmr_position
**      operation.  If no morerecords meet the qualification the caller is
**      returned a error indicating no next record.
**
**	As a special case for aggregate optimization, this function may
**	be called to obtain the count of records in the table, which prior
**	to this change required getting all records with multiple calls
**	to this function.
** 
**      Note:  When a B1 secure server is running, this interface only 
**      returns records that pass the B1 MAC assurances.  Namely, only
**      records with a security label that is dominated by the security 
**      label of the requester are returned.
**
** Inputs:
**      dmr_cb
**          .type                           Must be set to DMR_RECORD_CB.
**          .length                         Must be at least
**                                          sizeof(DMR_RECORD_CB) bytes.
**          .dmr_flags_mask                 Must be DMR_NEXT, DMR_PREV,        
**					    DMR_CURRENT_POS or DMR_BY_TID.
**          .dmr_access_id                  Record access identifer returned
**                                          from DMT_OPEN that identifies a
**                                          table.
**          .dmr_tid                        If dmr_flags_mask = DMR_BY_TID, then
**                                          field is used as a tuple identifer.
**          .dmr_data.data_address          Pointer to area to return the
**                                          requested record.
**          .dmr_data.data_in_size          Size of area for record.
**
** Outputs:
**      dmr_cb
**          .dmr_tid                        The tuple identifier of the record
**                                          being returned.
**          .dmr_data.data_address          The record is stored here.
**          .dmr_data.data_out_size         The size of the returned record.
**          .error.err_code                 One of the following error numbers.
**                                          E_DM0000_OK                
**                                          E_DM000B_BAD_CB_LENGTH
**                                          E_DM000C_BAD_CB_TYPE
**                                          E_DM000F_BAD_DB_ACCESS_MODE
**                                          E_DM0011_BAD_DB_NAME
**                                          E_DM001A_BAD_FLAG
**                                          E_DM001D_BAD_LOCATION_NAME
**                                          E_DM002B_BAD_RECORD_ID
**                                          E_DM003C_BAD_TID
**                                          E_DM0042_DEADLOCK
**                                          E_DM0044_DELETED_TID
**					    E_DM0047_UPDATED_TUPLE
**                                          E_DM004A_INTERNAL_ERROR
**                                          E_DM004B_LOCK_QUOTA_EXCEEDED
**					    E_DM004D_LOCK_TIMER_EXPIRED
**                                          E_DM0055_NONEXT
**                                          E_DM0065_USER_INTR
**                                          E_DM0064_USER_ABORT
**                                          E_DM0073_RECORD_ACCESS_CONFLICT
**                                          E_DM0074_NOT_POSITIONED
**					    E_DM008A_ERROR_GETTING_RECORD
**                                          E_DM0100_DB_INCONSISTENT
**                                          E_DM010C_TRAN_ABORTED
**                                          E_DM0112_RESOURCE_QUOTA_EXCEEDED
**					    E_DM006E_NON_BTREE_GETPREV
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
**	28-jul-1989 (mikem)
**	    Added logging of database, table, and owner when we get an internal
**	    error.
**	15-aug-1989 (rogerk)
**	    Added support for Non-SQL Gateway.  If getting record from a 
**	    gateway secondary index, then make sure that record buffer is
**	    large enough to hold a record from the base table, as that is
**	    what the gateway returns.  This is somewhat hokey, and would be
**	    better if the secondary index could actually describe the records
**	    being returned back, but...
**	15-oct-90 (linda)
**	    Integrate bug fix for gateway secondary index support:  perform
**	    sanity check on table width *after* switching tcb's.
**	11-feb-1991 (linda)
**	    Check for dmr_char->char_id == DMR_TIDJOIN, if it does then set
**	    rcb->rcb_tidjoin = RCB_TIDJOIN.  Part of gateway secondary index
**	    support.
**      22-apr-92 (schang)
**          GW merge
**	    30-apr-1991 (rickh)
**	        Removed the 11-feb-1991 tidjoin logic.  Let stand the change in
**	        where table width calculation occurs.
**	    22-jul-1991 (rickh)
**	        And now remove the table width calculation change that went in
**	        with the 11-feb-1991 tidjoin logic.
**	28-may-1993 (robf)
**	   Secure 2.0: Reworked old ORANGE code.
**	23-aug-1993 (bryanp)
**	    Fix a few cut-and-paste errors in some error message parameters.
**	31-jan-1994 (bryanp) B58487
**	    Handle failures in both dm2r_get and dm2r_unfix_pages.
**      30-aug-1994 (cohmi01)
**          Add DMR_PREV support for FASTPATH rel. Error if not btree.
**	22-may-1995 (cohmi01)
**	    Add support for count-only, for aggregate optimisation.
**	21-aug-1995 (cohmi01)
**	    count-only aggregate code moved to dml!dmragg.c
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          Unfix all pages before leaving DMF if row locking.
**      14-may-97 (dilma04)
**          Cursor Stability Project:
**          - upgrade isolation level from CS to RR, if DMR_SORT flag is set;
**          - if isolation level is CS or RR, set RCB_CSRR_LOCK locking mode 
**          for the time of dm2r_get() call.
**      21-may-97 (stial01)
**          Row locking: No more LK_PH_PAGE locks, so page(s) can stay fixed.
**	19-dec-97 (inkdo01)
**	    Changes for sorts which do NOT materialize results in temp tables.
**	    get is now directed straight to DMF sorter.
**      08-oct-98 (stial01)
**          Deallocate load context after all records read from DMF sorter.
**      09-dec-98 (stial01)
**          DMR_PKEY_PROJECTION: check for relspec BTREE not table_type which
**          may be GATEWAY
**      11-aug-2003 (chash01)
**          For RMS Gateway index table, add specific test to make sure
**          dmr->dmr_data.data_in_size + sizeof(DM_TID)
**          is no more than the value in table_width
**	12-Feb-2004 (schka24)
**	    Defend against someone doing a get on a partitioned master.
**	03-Nov-2004 (jenjo02)
**	    Relocated CSswitch from dmf_call to here; don't waste the
**	    call if Factotum thread.
**	11-Nov-2005 (jenjo02)
**	    Replaced dmx_show() with the more robust 
**	    dmxCheckForInterrupt() to standardize external
**	    interrupt handling.
**	11-Sep-2006 (jonj)
**	    Don't dmxCheckForInterrupt if extended table as txn is
**	    likely in a recursive call and not at an atomic
**	    point in execution as required for LOGFULL_COMMIT.
**	13-Feb-2007 (kschendel)
**	    Replace CSswitch with cancel check.
**	11-Apr-2008 (kschendel)
**	    Roll arithmetic exceptions into caller specified ADFCB.
**	    This is part of getting DMF qual context out of QEF.
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Don't change isolation level if crow_locking()
**	03-Mar-2010 (jonj)
**	    SIR 121619 MVCC, blob support:
**	    Set rcb_dmr_opcode here; dmpe bypasses dmf_call,
**	    which used to set it.
*/
DB_STATUS
dmr_get(
DMR_CB  *dmr_cb)
{
    DMR_CB		*dmr = dmr_cb;
    DMP_RCB		*rcb;
    DMP_TCB		*tcb;
    DML_XCB		*xcb;
    i4		flag;
    i4		table_width;
    DB_STATUS		status, local_status;
    i4		error, local_error;
    DB_ERROR		local_dberr;

    CLRDBERR(&dmr->error);

    for (status = E_DB_ERROR;;)
    {
	rcb = (DMP_RCB *)dmr->dmr_access_id;
	if (dm0m_check((DM_OBJECT *)rcb, (i4)RCB_CB) == E_DB_OK)
	{
	    if (rcb == NULL)
	    {
		uleFormat(NULL, E_DM00E0_BAD_CB_PTR, (CL_ERR_DESC *)NULL, ULE_LOG, 
		    NULL, (char *)NULL, (i4)0, (i4 *)NULL, &error, 1,
		    sizeof("record")-1, "record");
		SETDBERR(&dmr->error, 0, E_DM002A_BAD_PARAMETER);
		break;
	    }

	    rcb->rcb_dmr_opcode = DMR_GET;

	    tcb = rcb->rcb_tcb_ptr;
	    if (tcb == NULL)
	    {
		uleFormat(NULL, E_DM00E0_BAD_CB_PTR, (CL_ERR_DESC *)NULL, ULE_LOG, 
		    NULL, (char *)NULL, (i4)0, (i4 *)NULL, &error, 1,
		    sizeof("table")-1, "table");
		SETDBERR(&dmr->error, 0, E_DM002A_BAD_PARAMETER);
		break;
	    }
	    if (tcb->tcb_rel.relstat & TCB_IS_PARTITIONED)
	    {
		uleFormat(&dmr->error, E_DM0022_BAD_MASTER_OP, NULL, ULE_LOG, NULL,
			NULL, 0, NULL, &error, 3,
			0, "dmrget",
			sizeof(DB_OWN_NAME),tcb->tcb_rel.relowner.db_own_name,
			sizeof(DB_TAB_NAME),tcb->tcb_rel.relid.db_tab_name);
		break;
	    }
	    xcb = rcb->rcb_xcb_ptr;
	    if (xcb == NULL)
	    {
		uleFormat(NULL, E_DM00E0_BAD_CB_PTR, (CL_ERR_DESC *)NULL, ULE_LOG, 
		    NULL, (char *)NULL, (i4)0, (i4 *)NULL, &error, 1,
		    sizeof("transaction")-1, "transaction");
		SETDBERR(&dmr->error, 0, E_DM002A_BAD_PARAMETER);
		break;
	    }
	    if (dmr->dmr_flags_mask & DMR_NEXT)
	     if (dmr->dmr_flags_mask & DMR_SORTGET) ;
	     else flag = DM2R_GETNEXT;
	    else if (dmr->dmr_flags_mask & DMR_BY_TID)
		flag = DM2R_BYTID;
	    else if (dmr->dmr_flags_mask & DMR_CURRENT_POS)
	    {
	/* 	flag = DM2R_BYPOSITION; */
		flag = DM2R_BYTID;
		dmr->dmr_tid = rcb->rcb_currenttid.tid_i4;
	    }
            else if (dmr->dmr_flags_mask & DMR_PREV)
	    {
		flag = DM2R_GETPREV;
		if (dmr->dmr_flags_mask & DMR_RAAT)
		    flag |= DM2R_RAAT;
		if (tcb->tcb_table_type != TCB_BTREE)
		{
		    SETDBERR(&dmr->error, 0, E_DM006E_NON_BTREE_GETPREV);
	    	    break;
		}
	    }
	    else
	    {
		SETDBERR(&dmr->error, 0, E_DM001A_BAD_FLAG);
		break;
	    }

	    /* Check for btree primary key projection */
	    if (dmr->dmr_flags_mask & DMR_PKEY_PROJECTION)
	    {
		if ((tcb->tcb_rel.relspec == TCB_BTREE) &&
		    ((tcb->tcb_rel.relstat & TCB_INDEX) == 0) &&
		    (flag == DM2R_GETNEXT || flag == DM2R_GETPREV))
		    flag |= DM2R_PKEY_PROJ;
		else
		{
		    SETDBERR(&dmr->error, 0, E_DM001A_BAD_FLAG);
		    break;
		}
	    }

	    if (xcb->xcb_scb_ptr == NULL )
	    {
		uleFormat(NULL, E_DM00E0_BAD_CB_PTR, (CL_ERR_DESC *)NULL, ULE_LOG, 
		    NULL, (char *)NULL, (i4)0, (i4 *)NULL, &error, 1,
		    sizeof("session")-1, "session");
		SETDBERR(&dmr->error, 0, E_DM002A_BAD_PARAMETER);
		break;
	    }
	    
	    /* Check for external interrupts */
	    if ( xcb->xcb_scb_ptr->scb_ui_state && !tcb->tcb_extended )
		dmxCheckForInterrupt(xcb, &error);

	    if (xcb->xcb_state == 0)
	    {
		/*
		** Make sure the caller's buffer is large enough to hold
		** a record from this table.
                **
		** Note that for Gateway secondary index's, retrieves done
		** will actually return records from the base table!!!! So
		** if this is a get on a gateway 2nd index, make sure the
		** buffer is large enough.
                ** Aug-4-2003 (chash01)  The value relwid is sizeof(DM_TID) +
                ** sizeof(index column's length)., but the value in
                ** data_in_size is the size of base table columns.  This leads
                ** to serious problem (looping) in DMFCALL() if the size of
                ** base table columns is less than the value in relwid. This
                ** RMS gateway specific problem will be tested specifically.
		*/
		table_width = tcb->tcb_rel.relwid;

		if ( (dmr->dmr_data.data_address) &&
                       (
                         (tcb->tcb_rel.relgwid != GW_RMS &&
                          dmr->dmr_data.data_in_size >= table_width) ||
                           ( tcb->tcb_rel.relgwid == GW_RMS &&
		             dmr->dmr_data.data_in_size + sizeof(DM_TID)
                                >= table_width)
                       )
                   )
		{
		    dmr->dmr_data.data_out_size = table_width;

                    /* Upgrade isolation level to repeatable read if a 
                    ** cursor stability transaction is getting tuples 
                    ** to sort them for further update of this table,
		    ** but not if MVCC crow_locking().
                    */
		    if ( !crow_locking(rcb) )
		    {
			if (dmr->dmr_flags_mask & DMR_SORT &&
			    rcb->rcb_access_mode == RCB_A_WRITE &&
			    rcb->rcb_iso_level == RCB_CURSOR_STABILITY)
			{
			    rcb->rcb_iso_level = RCB_REPEATABLE_READ;
			}
     
			if (rcb->rcb_iso_level == RCB_CURSOR_STABILITY ||
			    rcb->rcb_iso_level == RCB_REPEATABLE_READ)
			{
			     rcb->rcb_state |= RCB_CSRR_LOCK;
			}
		    }

		    /*
		    ** Quick troll for external interrupts.
		    */
		    CScancelCheck(rcb->rcb_sid);

		    /* If this is a SORTGET, call DMF sorter to retrieve
		    ** next row. 
		    */

		    if (dmr->dmr_flags_mask & DMR_SORTGET)
		    {
			DM2R_L_CONTEXT	*lct;
			lct = (DM2R_L_CONTEXT *)tcb->tcb_lct_ptr;
			status = dmse_get_record(lct->lct_srt,
					&lct->lct_record, &dmr->error);

			if (status == E_DB_OK)
			{
			    MEcopy((PTR)lct->lct_record, 
				dmr->dmr_data.data_in_size, 
				(PTR)dmr->dmr_data.data_address);
			}
			else
			{
			    /* eof or error, call dmse to finish up. */
			    local_status = dmse_end(lct->lct_srt, &local_dberr);
			    if (local_status != E_DB_OK)
			    {
				dmr->error = local_dberr;
				status = local_status;
			    }

			    /* Deallocate load context */
			    if (lct->lct_mct.mct_buffer != (PTR)0)
			    {
				dm0m_deallocate((DM_OBJECT **)&lct->lct_mct.mct_buffer);
			    }

			    dm0m_deallocate((DM_OBJECT **)&tcb->tcb_lct_ptr);
			    tcb->tcb_lct_ptr = 0;
			    rcb->rcb_state &= ~RCB_LSTART;
			}

			return(status);
		    }

		    /* Regular ol' gets are handled here. */
 
		    xcb->xcb_scb_ptr->scb_qfun_errptr = &dmr->error;
		    status = dm2r_get(rcb, (DM_TID*)&dmr->dmr_tid, flag, 
			dmr->dmr_data.data_address, &dmr->error);

		    xcb->xcb_scb_ptr->scb_qfun_errptr = NULL;

		    /* If any arithmetic warnings to the RCB ADFCB during
		    ** position, roll them into the caller's ADFCB.
		    */
		    if (dmr->dmr_q_fcn != NULL && dmr->dmr_qef_adf_cb != NULL
		      && rcb->rcb_adf_cb->adf_warncb.ad_anywarn_cnt > 0)
			dmr_adfwarn_rollup((ADF_CB *)dmr->dmr_qef_adf_cb, rcb->rcb_adf_cb);

		    if ((tcb->tcb_rel.relstat & TCB_CONCUR))
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
	uleFormat( &dmr->error, 0, NULL, ULE_LOG , NULL,
	    (char * )NULL, 0L, (i4 *)NULL, 
	    &error, 0);
	uleFormat(NULL, E_DM904C_ERROR_GETTING_RECORD, NULL, ULE_LOG, NULL,
	    (char * )NULL, 0L, (i4 *)NULL, &error, 3,
	    sizeof(DB_DB_NAME), &rcb->rcb_tcb_ptr->tcb_dcb_ptr->dcb_name,
	    sizeof(DB_OWN_NAME), &rcb->rcb_tcb_ptr->tcb_rel.relowner,
	    sizeof(DB_TAB_NAME), &rcb->rcb_tcb_ptr->tcb_rel.relid
	    );
	SETDBERR(&dmr->error, 0, E_DM008A_ERROR_GETTING_RECORD);
    }

    return (status);
}
