/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <pc.h>
#include    <tm.h>
#include    <dmf.h>
#include    <ulf.h>
#include    <sxf.h>
#include    <adf.h>
#include    <ade.h>
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
#include    <dm2r.h>
#include    <dm2a.h>
#include    <dma.h>

/**
** Name: DMRAGG.C - DMF Aggregate Processor.                 
**
** Description:
**      This file contains the top-level routine for processing aggregates
**	within DMF for added performance, instead of having QEF get each
**	record and performing the aggregation.
**      This file defines:
**
**      dmr_aggregate - Perform Aggregation, reading any necessary data.
**
** History:
**      21-aug-1995 (cohmi01)
**	    Create for Aggregate Performance project.
**	30-jul-96 (nick)
**	    Include <dm2a.h>, <adf.h> and <ade.h>
**	    Return the ADI_WARN control block in the DMR_CB. #76423, 78024.
**	    Tidy up.
**	12-Feb-1997 (nanpr01)
**	    Pass the QEF_ADF_CB for proper initialization. Bug #80636
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	11-Nov-2005 (jenjo02)
**	    Replaced dmx_show() with the more robust 
**	    dmxCheckForInterrupt() to standardize external
**	    interrupt handling.
**	12-Apr-2008 (kschendel)
**	    CX's executed in DMF context now, update dm2a call.
**	20-Oct-2008 (jonj)
**	    SIR 120874 Modified to use new DB_ERROR based uleFormat 
**	01-Dec-2008 (jonj)
**	    SIR 120874: dm2a_? functions converted to DB_ERROR *
**	03-Mar-2010 (jonj)
**	    SIR 121619 MVCC, blob support:
**	    Set rcb_dmr_opcode here; dmpe bypasses dmf_call,
**	    which used to set it.
**/

/*{
** Name:  dmr_aggregate - Perform Aggregation, reading any necessary data.
**
**  INTERNAL DMF call format:      status = dmr_aggregate(&dmr_cb);
**
**  EXTERNAL call format:          status = dmf_call(DMR_AGGREGATE,&dmr_cb);
**
** Description:
**
** Inputs:
**      dmr_cb
**          .type                           Must be set to DMR_RECORD_CB.
**          .length                         Must be at least
**                                          sizeof(DMR_RECORD_CB) bytes.
**          .dmr_flags_mask                 
**          .dmr_access_id                  Record access identifer returned
**                                          from DMT_OPEN that identifies a
**                                          table.
**          .dmr_data.data_address          Pointer to area to return the
**                                          requested record.
**          .dmr_data.data_in_size          Size of area for record.
**          .dmr_agg_flags                  Bits indicating what types
**                                          of aggregates are present in
**                                          a DMR_AGGREGATE request. If no
**                                          bits are on, any combination
**                                          of aggs may be present, else
**                                          just the ones indicated are.
**                                          One of the following: 
**						DMR_AGLIM_COUNT
**						DMR_AGLIM_MAX         
**						DMR_AGLIM_MIN        
**    	    .dmr_agg_excb                   For DMR_AGGREGATE - ptr to ADE_EXCB
**					    to use when processing aggregates.
**	    .dmr_qef_adf_cb		    Ptr of qef adf_cb for proper 
**					    initialization.
**
**
** Outputs:
**      dmr_cb
**          .dmr_tid                   	    For COUNT requests only, the
**					    count of items requested is placed
**					    here, as a 'tid_i4'.
**          .dmr_data.data_address          The record is stored here.
**          .dmr_data.data_out_size         The size of the returned record.
**          .error.err_code                 One of the following error numbers.
**                                          E_DM0000_OK                
**                                          E_DM000B_BAD_CB_LENGTH
**                                          E_DM000C_BAD_CB_TYPE
**                                          E_DM000F_BAD_DB_ACCESS_MODE
**                                          E_DM0011_BAD_DB_NAME
**                                          E_DM001A_BAD_FLAG
**                                          E_DM0042_DEADLOCK
**                                          E_DM004A_INTERNAL_ERROR
**                                          E_DM004B_LOCK_QUOTA_EXCEEDED
**					    E_DM004D_LOCK_TIMER_EXPIRED
**                                          E_DM0055_NONEXT
**                                          E_DM0065_USER_INTR
**                                          E_DM0064_USER_ABORT
**                                          E_DM0074_NOT_POSITIONED
**					    E_DM008A_ERROR_GETTING_RECORD
**                                          E_DM0100_DB_INCONSISTENT
**                                          E_DM010C_TRAN_ABORTED
**                                          E_DM0112_RESOURCE_QUOTA_EXCEEDED
**
**      Returns:
**         E_DB_WARN                        Function completed normally with a 
**                                          termination status which is in
**                                          dmr_cb.err_code.
**					    To comply with QEF requirements,
**					    E_DB_WARN, with a E_DM0055_NONEXT
**					    termination status is a normal
**					    return.
**         E_DB_ERROR                       Function completed abnormally 
**                                          with a 
**                                          termination status which is in
**                                          dmr_cb.err_code.
**         E_DB_FATAL                       Function completed with a fatal
**                                          error which must be handled
**                                          immediately.  The fatal status is in
**                                          dmr_cb.err_code.
*/
DB_STATUS
dmr_aggregate(
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

    CLRDBERR(&dmr->error);

    status = E_DB_OK;
    do
    {
	rcb = (DMP_RCB *)dmr->dmr_access_id;
	if (dm0m_check((DM_OBJECT *)rcb, (i4)RCB_CB) != E_DB_OK)
	{
	    SETDBERR(&dmr->error, 0, E_DM002B_BAD_RECORD_ID);
	    break;
	}

	rcb->rcb_dmr_opcode = DMR_AGGREGATE;

	tcb = rcb->rcb_tcb_ptr;
	if (dm0m_check((DM_OBJECT *)tcb, (i4)TCB_CB) != E_DB_OK)
	{
	    uleFormat(NULL, E_DM00E0_BAD_CB_PTR, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
	    	(char *)NULL, (i4)0, (i4 *)NULL, &error, 1,
	    	sizeof("table")-1, "table");
	    SETDBERR(&dmr->error, 0, E_DM002A_BAD_PARAMETER);
	    break;
	}
	xcb = rcb->rcb_xcb_ptr;
	if (xcb == NULL)
	{
	    uleFormat(NULL, E_DM00E0_BAD_CB_PTR, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
	    	(char *)NULL, (i4)0, (i4 *)NULL, &error, 1,
		sizeof("transaction")-1, "transaction");
	    SETDBERR(&dmr->error, 0, E_DM002A_BAD_PARAMETER);
	    break;
	}

	if (xcb->xcb_scb_ptr == NULL )
	{
	    uleFormat(NULL, E_DM00E0_BAD_CB_PTR, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
	    	(char *)NULL, (i4)0, (i4 *)NULL, &error, 1,
		    sizeof("session")-1, "session");
	    SETDBERR(&dmr->error, 0, E_DM002A_BAD_PARAMETER);
	    break;
	}
	    

	/* Check for external interrupts */
	if ( xcb->xcb_scb_ptr->scb_ui_state )
	    dmxCheckForInterrupt(xcb, &error);

	if (xcb->xcb_state)
	{
	    if (xcb->xcb_state & XCB_USER_INTR)
		SETDBERR(&dmr->error, 0, E_DM0065_USER_INTR);
	    else if (xcb->xcb_state & XCB_FORCE_ABORT)
		SETDBERR(&dmr->error, 0, E_DM010C_TRAN_ABORTED);
	    else if (xcb->xcb_state & XCB_ABORT)
		SETDBERR(&dmr->error, 0, E_DM0064_USER_ABORT);
	    else if (xcb->xcb_state & XCB_WILLING_COMMIT)
		SETDBERR(&dmr->error, 0, E_DM0132_ILLEGAL_STMT);
	    break;
	}
	/* Although DMR_AGGREGATE does not return a 'record',
	** The buffer passed in will be used to read records during
	** the aggregation process, and will be looked at by the
	** ade_execute() requests made by DMF, so ensure that it is valid.
	*/

	if ((dmr->dmr_data.data_address == 0) ||
	    (dmr->dmr_data.data_in_size < tcb->tcb_rel.relwid))
	{
	    SETDBERR(&dmr->error, 0, E_DM002A_BAD_PARAMETER);
	    break;
	}

	dmr->dmr_data.data_out_size = 0;	/* no record is returned. */
	dmr->dmr_res_data = (PTR)&rcb->rcb_adf_cb->adf_warncb;

	xcb->xcb_scb_ptr->scb_qfun_errptr = &dmr->error;
    	status = dm2a_simpagg(rcb, dmr->dmr_agg_flags, 
	    (ADE_EXCB *)dmr->dmr_agg_excb,
	    (ADF_CB *)dmr->dmr_qef_adf_cb,
	    dmr->dmr_data.data_address, &dmr_cb->dmr_count, &dmr->error);

	xcb->xcb_scb_ptr->scb_qfun_errptr = NULL;

	if (status == E_DB_OK)
	    return (E_DB_OK);

    } while (FALSE);	    

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
