/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
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
#include    <dm2r.h>

/**
** Name: DMRPOS.C - Implements the DMF position record operation.
**
** Description:
**      This file contains the functions necessary to position a record
**      stream based on a criteria.
**      This file defines:
**
**      dmr_position - Position to a record.
**
** History:
**      01-sep-85 (jennifer)
**          Created for Jupiter.
**	17-dec-1985 (derek)
**	    Completed code for dmr_position().
**	6-June-1989 (ac)
**	    Added 2PC support.
**	05-dec-1992 (kwatts)
**	    Smart Disk enhancements, changes to the key_descriptor setting.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**		Include <lg.h> to pick up logging system type definitions.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (bryanp)
**	    Include <lk.h> to pick up locking system type definitions.
**	    Include <pc.h> before <lk.h> to pick up PID definition
**	31-jan-1994 (bryanp) B58493
**	    Handle failures in both dm2r_position and dm2r_unfix_pages.
**	5-may-95 (stephenb/lewda02)
**	    Add support for DMR_LAST using DM2R_LAST.
**	26-apr-1996 (wonst02)
**	    Allow Rtree QUAL types: INTERSECTS, OVERLAY, INSIDE & CONTAINS.
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          Unfix all pages before leaving DMF if row locking.
**      14-may-97 (dilma04)
**          Cursor Stability Project:
**          If isolation level is CS or RR, set RCB_CSRR_LOCK locking mode 
**          for the time of dm2r_position() call.
**      21-may-97 (stial01)
**          Row locking: No more LK_PH_PAGE locks, so page(s) can stay fixed.
**	16-oct-1999 (nanpr01)
**	    If a estimate of -1 is passed, we resort to old behavior ie donot
**	    override readahead decision. If we have row estimates, override
**	    the readahead based on the estimate.
**      15-may-2000 (stial01)
**          Remove Smart Disk code
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	11-Apr-2008 (kschendel)
**	    dm2r position call updated, fix here, set RCB fields instead.
**	20-Oct-2008 (jonj)
**	    SIR 120874 Modified to use new DB_ERROR based uleFormat 
**	26-Nov-2008 (jonj)
**	    SIR 120874: dm2r_? functions converted to DB_ERROR *
**/

/*{
** Name:  dmr_position - Position a record.
**
**  INTERNAL DMF call format:      status = dmr_position(&dmr_cb);
**
**  EXTERNAL call format:          status = dmf_call(DMR_POSITION,&dmr_cb);
**
** Description:
**
**      Note: A position must be done before a table can be read.
**     
**	The act of positioning a table is used to convey information about the
**	set of records that are to be retrieved by succeeding get operations.
**	There are two basic modes of operation for position: position for 
**	full scan of table or position based on primary key qualifications.
**	Positioning for a full scan means that all records of the table will be
**	returned.  Positioning for qualification will return records that
**	satisfy the qualification.  A qualification involves comparisons of
**	attributes that are member of the primary key.  These attributes can
**	be compared for >, >=, ==, < or <=.  The least restrictive set of
**	qualification is used if there are conflicts.  (Example: If position
**	is presented the two following qualifications: (a >= ?, b > ?) where 
**      a and b are members of the key, position would use (a >= ? ,b >= ?).
**	If an attribute is mentioned more then once the extra qualifications are
**	ignored.  The data type of the value passed is determined from the
**	attribute number.  The data type is assumed to be of the length of the
**	same length and type as the attribute it is operating on.
**
**	For each of the position operations a qualification function can be
**	given.  The qualification function will be called when a record has
**	been retrieved and it has passed the key qualification if one was
**	given.  If the qualification function returns TRUE, the record is
**	returned, otherwise the next record is processed.  This allows the
**	caller to add qualifications on non-key fields and have them processed
**	by the get operation.
**
**	The special position operation DMR_TID, allows a the position of
**	heap temporary to be returned to a previous tid value.  The next
**	fetch will return the record associated with tid, and successive fetchs
**	will see successive records.
**
**	The special position operation DMR_REPOSITION, allows the position of
**	a scan on any structure to be repositioned usiing the same key used
**	in the last position request if this request was for DMR_QUAL or
**	DMR_REPOSITION.  The same set of records is returned excpet for
**	records that have been changed or deleted.
**
** Inputs:
**      dmr_cb
**          .type                           Must be set to DMR_RECORD_CB
**          .length                         Must be at least
**                                          sizeof(DMR_RECORD_CB) bytes.
**          .dmr_flags_mask                 Must be zero.
**          .dmr_position_type              Either DMR_TID, DMR_ALL, DMR_QUAL,
**					    DMR_REPOSITION, DMR_LAST or
**					    DMR_ENDQUAL. 
**					    (See above description).
**          .dmr_access_id                  Record access identifer returned
**                                          from DMT_OPEN that identifies a
**                                          table.
**          .dmr_tid                        If dmr_position_type = DMR_TID,
**                                          field is used as a tuple identifer.
**          .dmr_attr_desc.ptr_address      If dmr_position_type = DMR_QUAL, 
**                                          then
**                                          this points to an area containing
**                                          an array of DMR_ATTR_ENTRY's.  See
**                                          below for a description of 
**                                          <dmr_attr_desc>.
**          .dmr_attr_desc.ptr_size         Length of entry of 
**                                          dmr_attr_desc.data_address
**                                          in bytes.
**          .dmr_attr_desc.ptr_in_count     Number of entries in attr array.
**          .dmr_q_fcn                      Zero or address of function used to
**                                          qualify records.
**          .dmr_q_arg                      Argument to be passed to 
**                                          dmr_qual_fcn when called.
**
**          <dmr_attr_desc> is of type DMR_ATTR_ENTRY
**              .attr_number                The attribute number that this
**                                          restriction applies to.
**              .attr_operation             A binary comparision operator
**                                          in the set: DMR_OP_EQ, DMR_OP_LTE
**                                          DMR_OP_GTE, DMR_OP_INTERSECTS,
**					    DMR_OP_OVERLAY, DMR_OP_INSIDE,
**					    DMR_OP_CONTAINS.
**              .attr_value_ptr             A pointer to the value for this 
**                                          restriction.
**
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
**                                          E_DM0047_CHANGED_TUPLE
**                                          E_DM004A_INTERNAL_ERROR
**                                          E_DM004B_LOCK_QUOTA_EXCEEDED
**					    E_DM004D_LOCK_TIMER_EXPIRED
**                                          E_DM0065_USER_INTR
**                                          E_DM0064_USER_ABORT
**					    E_DM008E_ERROR_POSITIONING
**                                          E_DM004B_LOCK_QUOTA_EXCEEDED
**                                          E_DM0100_DB_INCONSISTENT
**                                          E_DM010C_TRAN_ABORTED
**                                          E_DM0112_RESOURCE_QUOTA_EXCEEDED
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
**         E_DB_FATAL			    Function completed with a fatal
**                                          error which must be handled
**                                          immediately.  The fatal status is in
**                                          dmr_cb.err_code.
** History:
**      01-sep-1985 (jennifer)
**	    Created new for jupiter.
**	17-dec-1985 (derek)
**	    Completed code.
**	 1-dec-1988 (rogerk)
**	    Don't continue after finding bad attribute array entry.
**	17-dec-1990 (jas)
**	    Smart Disk project integration:
**	    Support DMR_SDSCAN, requesting a Smart Disk scan.  In a Smart
**	    Disk scan, the "key pointer" becomes a pointer to a
**	    (hardware-dependent) block encoding the search condition;
**	    the "key count" is the length in bytes of the encoded
**	    search condition.
**	05-dec-1992 (kwatts)
**	    Smart Disk enhancements. There is now a different structure in
**	    the DMR CB to set key_descriptor to.
**	31-jan-1994 (bryanp) B58493
**	    Handle failures in both dm2r_position and dm2r_unfix_pages.
**	5-may-95 (stephenb/lewda02)
**	    Add support for DMR_LAST using DM2R_LAST.
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          Unfix all pages before leaving DMF if row locking.
**      14-may-97 (dilma04)
**          Cursor Stability Project:
**          If isolation level is CS or RR, set RCB_CSRR_LOCK locking mode 
**          for the time of dm2r_position() call.
**      21-may-97 (stial01)
**          Row locking: No more LK_PH_PAGE locks, so page(s) can stay fixed.
**	23-jul-1998 (nanpr01)
**	    Making position code aware of the scanning direction for backward
**	    scan.
**	01-Mar-2004 (jenjo02)
**	    Position requests on Partition Masters are illegal.
**	11-Nov-2005 (jenjo02)
**	    Replaced dmx_show() with the more robust 
**	    dmxCheckForInterrupt() to standardize external
**	    interrupt handling.
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
dmr_position(
DMR_CB  *dmr_cb)
{
    DM_TID              tid;
    DMR_CB		*dmr = dmr_cb;
    DMP_RCB		*rcb;
    DMP_TCB		*t;
    DML_XCB		*xcb;
    DMR_ATTR_ENTRY	*k;
    DMR_ATTR_ENTRY	*ka;
    DMR_ATTR_ENTRY      karray[DB_MAX_COLS];
    i4		i;
    i4		flag;
    DB_STATUS		status, local_status;
    i4		error,local_error;
    char		*key_descriptor = (char *) karray;
    i4			key_count = dmr->dmr_attr_desc.ptr_in_count;
    DB_ERROR		local_dberr;

    CLRDBERR(&dmr->error);

    for (status = E_DB_ERROR;;)
    {
	rcb = (DMP_RCB *)dmr->dmr_access_id;
	if (dm0m_check((DM_OBJECT *)rcb, (i4)RCB_CB) == E_DB_OK)
	{
	    rcb->rcb_dmr_opcode = DMR_POSITION;

	    t = rcb->rcb_tcb_ptr;

	    if (t->tcb_rel.relstat & TCB_IS_PARTITIONED)
	    {
		uleFormat(&dmr->error, E_DM0022_BAD_MASTER_OP, NULL, ULE_LOG, NULL,
			NULL, 0, NULL, &error, 3,
			0, "dmrpos",
			sizeof(DB_OWN_NAME),t->tcb_rel.relowner.db_own_name,
			sizeof(DB_TAB_NAME),t->tcb_rel.relid.db_tab_name);
		break;
	    }

	    if ((dmr->dmr_position_type == DMR_QUAL) ||
	        (dmr->dmr_position_type == DMR_ENDQUAL))
	    {
	        if (dmr->dmr_position_type == DMR_QUAL) 
		    flag = DM2R_QUAL;
		else
		    flag = DM2R_ENDQUAL;
		if (dmr->dmr_attr_desc.ptr_address == 0 ||
		    dmr->dmr_attr_desc.ptr_size < sizeof(DMR_ATTR_ENTRY)||
		    dmr->dmr_attr_desc.ptr_in_count == 0 ||
		    dmr->dmr_attr_desc.ptr_in_count > DB_MAX_COLS)
		{
		    SETDBERR(&dmr->error, 0, E_DM002A_BAD_PARAMETER);
		    break;
		}
		ka = karray;
		for (i = 0; i < dmr->dmr_attr_desc.ptr_in_count; i++) 
		{
		    DMR_ATTR_ENTRY	*k = 
			((DMR_ATTR_ENTRY **)dmr->dmr_attr_desc.ptr_address)[i];

		    ka->attr_number = k->attr_number;
		    ka->attr_operator = k->attr_operator;
		    ka->attr_value = k->attr_value;
		    ka++;
		    if (k->attr_number == 0 ||
			k->attr_number > t->tcb_rel.relatts ||
			k->attr_operator == 0 ||
			k->attr_operator > DMR_OP_CONTAINS)
		    {
			SETDBERR(&dmr->error, 0, E_DM002A_BAD_PARAMETER);
			break;
		    }
		}
		if (dmr->error.err_code)
		    break;
	    }
	    else if (dmr->dmr_position_type == DMR_ALL)
		flag = DM2R_ALL;
	    else if (dmr->dmr_position_type == DMR_TID)
	    {
		flag = DM2R_BYTID;
		tid.tid_i4 = ((DM_TID *)&dmr->dmr_tid)->tid_i4;
	    }
	    else if (dmr->dmr_position_type == DMR_REPOSITION)
		flag = DM2R_REPOSITION;	
	    else if (dmr->dmr_position_type == DMR_LAST)
		flag = DM2R_LAST;
	    else
	    {
		SETDBERR(&dmr->error, 0, E_DM002A_BAD_PARAMETER);
		break;
	    }

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

	    if (xcb->xcb_state == 0)
	    {
		if ( !crow_locking(rcb) &&
		       (rcb->rcb_iso_level == RCB_CURSOR_STABILITY ||
			rcb->rcb_iso_level == RCB_REPEATABLE_READ) )
                {
                     rcb->rcb_state |= RCB_CSRR_LOCK;
                }

		/* Informed Read-Ahead */
		if ((flag & DM2R_QUAL || flag & DM2R_ENDQUAL) &&
		    (dmr->dmr_s_estimated_records > 0))
		{
		    f8   	scanfactor = 
		   dmf_svcb->svcb_scanfactor[DM_CACHE_IX(t->tcb_rel.relpgsize)];

		    /* Override the read-ahead decisions */
		    if (((t->tcb_rel.relstat & TCB_INDEX) == 0) &&
			(t->tcb_tperpage != 0) &&
			(dmr->dmr_s_estimated_records/t->tcb_tperpage < 
				0.5 * scanfactor)) 
			flag |= DM2R_NOREADAHEAD;
		    else {
			if ((t->tcb_rel.relstat & TCB_INDEX) &&
			    (t->tcb_rel.relspec == TCB_BTREE) &&
			    (t->tcb_kperleaf != 0) &&
			    (dmr->dmr_s_estimated_records/t->tcb_kperleaf < 
				0.5 * scanfactor)) 
			    flag |= DM2R_NOREADAHEAD;
			else
		    	    if ((t->tcb_rel.relstat & TCB_INDEX) &&
			        (t->tcb_tperpage != 0) &&
				(dmr->dmr_s_estimated_records/t->tcb_tperpage < 
				 0.5 * scanfactor)) 
				flag |= DM2R_NOREADAHEAD;
		    }
		}

		/* Set qualification stuff in RCB from caller DMR_CB.
		** This will persist thru all upcoming gets as well.
		*/
		rcb->rcb_f_rowaddr = dmr->dmr_q_rowaddr;
		rcb->rcb_f_qual = dmr->dmr_q_fcn;
		rcb->rcb_f_arg = dmr->dmr_q_arg;
		rcb->rcb_f_retval = dmr->dmr_q_retval;
		MEfill(sizeof(ADI_WARN), 0, &rcb->rcb_adf_cb->adf_warncb);

		xcb->xcb_scb_ptr->scb_qfun_errptr = &dmr->error;
		status = dm2r_position(rcb, flag, 
		    (DM2R_KEY_DESC *)key_descriptor, key_count, 
		    &tid, &dmr->error);

		xcb->xcb_scb_ptr->scb_qfun_errptr = NULL;

		/* If any arithmetic warnings to the RCB ADFCB during
		** position, roll them into the caller's ADFCB.
		*/
		if (dmr->dmr_q_fcn != NULL && dmr->dmr_qef_adf_cb != NULL
		  && rcb->rcb_adf_cb->adf_warncb.ad_anywarn_cnt > 0)
		    dmr_adfwarn_rollup((ADF_CB *)dmr->dmr_qef_adf_cb, rcb->rcb_adf_cb);

		if ((t->tcb_rel.relstat & TCB_CONCUR))
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
	    }
	}	    
	else
	    SETDBERR(&dmr->error, 0, E_DM002B_BAD_RECORD_ID);
	break;
    }	    

    if (dmr->error.err_code == E_DM004B_LOCK_QUOTA_EXCEEDED ||
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
	    (char * )NULL, 0L, (i4 *)NULL, &error, 0);
	SETDBERR(&dmr->error, 0, E_DM008E_ERROR_POSITIONING);
    }

    return (status);
}

/*
** Name: dmr_adfwarn_rollup -- Roll up arithmetic warn counts to caller
**
** Description:
**
**	This routine is part of the mechanism for handling arithmetic
**	exceptions in row-qualification.  Row qualifications are usually
**	some sort of compiled expression (CX), and any sort of arithmetic
**	exception in same is to be reported by QEF rather than DMF.
**
**	If there are any ADF warnings in the from-ADF-CB, roll them into
**	the to-ADF-CB.  (The from-CB is normally the RCB's ADF-CB, and
**	the to-CB is usually something belonging to QEF.)
**
** Inputs:
**	to_adfcb		ADF_CB * cb to roll warning counts into
**	from_adfcb		ADF_CB * cb to roll counts from
**
** Outputs:
**	none
**	Zeros from_adfcb's warncb area
**
** History:
**	11-Apr-2008 (kschendel)
**	    Part of getting rid of QEF signal context during DMF qual.
**
*/

void
dmr_adfwarn_rollup(ADF_CB *to_adfcb, ADF_CB *from_adfcb)
{
    if (from_adfcb->adf_warncb.ad_anywarn_cnt > 0)
    {
	/* Use adjust-counter in a feeble attempt to be thread safe.
	** Unfortunately, adx_handler doesn't use adjust-counter,
	** because it's ADF.  FIXME.
	*/
	CSadjust_counter(&to_adfcb->adf_warncb.ad_anywarn_cnt,
				from_adfcb->adf_warncb.ad_anywarn_cnt);
	if (from_adfcb->adf_warncb.ad_intdiv_cnt > 0)
	    CSadjust_counter(&to_adfcb->adf_warncb.ad_intdiv_cnt,
				from_adfcb->adf_warncb.ad_intdiv_cnt);
	if (from_adfcb->adf_warncb.ad_intovf_cnt > 0)
	    CSadjust_counter(&to_adfcb->adf_warncb.ad_intovf_cnt,
				from_adfcb->adf_warncb.ad_intovf_cnt);
	if (from_adfcb->adf_warncb.ad_fltdiv_cnt > 0)
	    CSadjust_counter(&to_adfcb->adf_warncb.ad_fltdiv_cnt,
				from_adfcb->adf_warncb.ad_fltdiv_cnt);
	if (from_adfcb->adf_warncb.ad_fltovf_cnt > 0)
	    CSadjust_counter(&to_adfcb->adf_warncb.ad_fltovf_cnt,
				from_adfcb->adf_warncb.ad_fltovf_cnt);
	if (from_adfcb->adf_warncb.ad_fltund_cnt > 0)
	    CSadjust_counter(&to_adfcb->adf_warncb.ad_fltund_cnt,
				from_adfcb->adf_warncb.ad_fltund_cnt);
	if (from_adfcb->adf_warncb.ad_mnydiv_cnt > 0)
	    CSadjust_counter(&to_adfcb->adf_warncb.ad_mnydiv_cnt,
				from_adfcb->adf_warncb.ad_mnydiv_cnt);
	if (from_adfcb->adf_warncb.ad_decovf_cnt > 0)
	    CSadjust_counter(&to_adfcb->adf_warncb.ad_decovf_cnt,
				from_adfcb->adf_warncb.ad_decovf_cnt);
	if (from_adfcb->adf_warncb.ad_decdiv_cnt > 0)
	    CSadjust_counter(&to_adfcb->adf_warncb.ad_decdiv_cnt,
				from_adfcb->adf_warncb.ad_decdiv_cnt);
    }
    MEfill(sizeof(ADI_WARN), 0, &from_adfcb->adf_warncb);

} /* dmr_adfwarn_rollup */
