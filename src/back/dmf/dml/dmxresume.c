/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <st.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <adf.h>
#include    <pc.h>
#include    <scf.h>
#include    <ulf.h>
#include    <dmf.h>
#include    <dmxcb.h>
#include    <dmccb.h>
#include    <dmtcb.h>
#include    <dmrcb.h>
#include    <lg.h>
#include    <lk.h>
#include    <dm.h>
#include    <dml.h>
#include    <dmp.h>
#include    <dmxe.h>
#include    <dm0m.h>

/**
** Name: DMXRESUME.C - Functions used to resume a Transaction.
**
** Description:
**      This file contains the functions necessary to resume commit or 
**	resume abort a slave transaction.
**      This file defines:
**    
**      dmx_resume()        -  Routine to perform the resume commit/abort 
**                             transaction operation.
**
** History:
**      18-May-88 (ac)
**          Created for Jupiter.
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
**	31-jan-1994 (bryanp) B58563
**	    Format scf_error.err_code if scf_call fails.
**	3-jun-97 (nanpr01)
**	    Fix bug 78352 : When transactions are rolled back, temporary
**	    tables that were updated does not get dropped as documented.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	08-Nov-2000 (jenjo02)
**	    Deleted xcb_tq_next|prev, xccb_tq_next|prev.
**	08-Jan-2001 (jenjo02)
**	    Get DML_SCB* from ODCB instead of SCU_INFORMATION call.
**	05-Mar-2002 (jenjo02)
**	    Init xcb_seq, xcb_cseq for Sequence Generators.
**	12-Apr-2008 (kschendel)
**	    Need to include adf.h now.
**	20-Oct-2008 (jonj)
**	    SIR 120874 Modified to use new DB_ERROR based uleFormat 
**	21-Nov-2008 (jonj)
**	    SIR 120874: dmxe_? functions converted to DB_ERROR *
**	24-Nov-2008 (jonj)
**	    SIR 120874: dm0m_? functions converted to DB_ERROR *
**/

/*{
** Name: dmx_resume - Resume a transaction.
**
**  INTERNAL DMF call format:      status = dmx_resume(&dmx_cb);
**
**  EXTERNAL call format:          status = dmf_call(DMX_RESUME,&dmx_cb);
**
** Description:
**    The dmx_resume function handles the resume of a slave transaction
**    that is in the WILLING COMMIT state waiting for the final commit/abort
**    status from the coordinator.
**
** Inputs:
**      dmx_cb 
**          .type                           Must be set to DMX_TRANSACTION_CB.
**          .length                         Must be at least sizeof(DMX_CB).
**          .dmx_session_id                 Must be a valid session identifier
**                                          obtained from SCF.
**          .dmx_db_id                      The database identifier of the
**                                          database in this transaction that
**                                          can be updated.
**	    .dmx_dis_tran_id		    Distributed transaction identifier.
** Output:
**      dmx_cb 
**          .dmx_tran_id                    Identifier for this transaction.
**          .dmx_phys_tran_id               Original physical transaction 
**					    identifier that is associated with.
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
**					    E_DM0099_ERROR_RESUME_TRAN
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
**      18-May-88 (ac)
**          Created for Jupiter.
**      25-sep-1992 (nandak)
**          Copy the entire distributed transaction id.
**	31-jan-1994 (bryanp) B58563
**	    Format scf_error.err_code if scf_call fails.
**	21-Jan-1997 (jenjo02)
**	    Remove DM0M_ZERO from dm0m_allocate() for XCB, manually
**	    initialize remaining fields instead.
**	3-jun-97 (nanpr01)
**	    Fix bug 78352 : When transactions are rolled back, temporary
**	    tables that were updated does not get dropped as documented.
**	6-Feb-2004 (schka24)
**	    Get rid of DMU count.
**	13-May-2004 (schka24)
**	    Init PCB list.
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Init MVCC-related XCB pointers.
**	23-Mar-2010 (kschendel) SIR 123448
**	    XCB's XCCB list now has to be mutex protected, init mutex here.
*/

DB_STATUS
dmx_resume(
DMX_CB    *dmx_cb)
{
    char		sem_name[10+16+10+1];	/* last +10 is slop */
    DMX_CB		*dmx = dmx_cb;
    DML_SCB		*scb;
    DML_ODCB		*odcb;
    DML_XCB		*x;
    DML_XCB		*xcb = (DML_XCB *)0;
    i4		btflags = 0;
    i4		mode;
    i4		error,local_error;
    DB_STATUS		status;

    CLRDBERR(&dmx->error);

    status = E_DB_ERROR;
    for (;;)
    {
	mode = DMXE_WRITE;

	if ( (odcb = (DML_ODCB *)dmx->dmx_db_id) &&
	     dm0m_check((DM_OBJECT *)odcb, (i4)ODCB_CB) == 0 )
	{
	    if ( (scb = odcb->odcb_scb_ptr) &&
	         dm0m_check((DM_OBJECT *)scb, (i4)SCB_CB) == 0 )
	    {
		if (scb->scb_x_ref_count == 0)
		{
		    /* Clear user interrupt state in SCB. */
		    scb->scb_ui_state &= ~SCB_USER_INTR;
		    status = dm0m_allocate((i4)sizeof(DML_XCB), 0, 
			(i4)XCB_CB, (i4)XCB_ASCII_ID, (char *)scb, 
			(DM_OBJECT **)&xcb, &dmx->error);
		    if (status == E_DB_OK)
		    {
			if (odcb->odcb_dcb_ptr->dcb_status & DCB_S_JOURNAL)
			    btflags |= DMXE_JOURNAL;
			x = xcb;

			status = dmxe_resume(&dmx->dmx_dis_tran_id,
			    odcb->odcb_dcb_ptr,
			    &scb->scb_lock_list, &x->xcb_log_id, 
			    &x->xcb_tran_id, &x->xcb_lk_id, &dmx->error);
		    }

		    if (status == E_DB_OK)
		    {
			dm0s_minit(&x->xcb_cq_mutex,
				STprintf(sem_name, "XCB cq %p", x));
			x->xcb_x_type = XCB_UPDATE | XCB_DISTRIBUTED;
			STRUCT_ASSIGN_MACRO(dmx->dmx_dis_tran_id, x->xcb_dis_tran_id);
			x->xcb_state = XCB_WILLING_COMMIT;
			x->xcb_flags = 0;
			if (btflags & DMXE_JOURNAL)
			    x->xcb_flags |= XCB_JOURNAL;
			x->xcb_q_next = scb->scb_x_next;
			x->xcb_q_prev = (DML_XCB*)&scb->scb_x_next;
			scb->scb_x_next->xcb_q_prev = xcb;
			scb->scb_x_next = x;
			x->xcb_scb_ptr = scb;
			x->xcb_rq_next = (DMP_RCB*) &x->xcb_rq_next;
			x->xcb_rq_prev = (DMP_RCB*) &x->xcb_rq_next;
			x->xcb_sq_next = (DML_SPCB*) &x->xcb_sq_next;
			x->xcb_sq_prev = (DML_SPCB*) &x->xcb_sq_next;
			x->xcb_cq_next = (DML_XCCB*) &x->xcb_cq_next;
			x->xcb_cq_prev = (DML_XCCB*) &x->xcb_cq_next;
			x->xcb_odcb_ptr = odcb;
			dmx->dmx_tran_id = (char *)x;
			STRUCT_ASSIGN_MACRO(x->xcb_tran_id, dmx->dmx_phys_tran_id);
			scb->scb_x_ref_count++;
			STRUCT_ASSIGN_MACRO(scb->scb_user, x->xcb_username);

			/* Initialize remaining XCB fields */
			x->xcb_sp_id = 0;
			x->xcb_rep_seq = 0;
			x->xcb_rep_input_q = 0;
			x->xcb_rep_remote_tx = 0;
			x->xcb_s_open = 0;
			x->xcb_s_fix = 0;
			x->xcb_s_get = 0;
			x->xcb_s_replace = 0;
			x->xcb_s_delete = 0;
			x->xcb_s_insert = 0;
			x->xcb_s_cpu = 0;
			x->xcb_s_dio = 0;
			x->xcb_seq  = (DML_SEQ*)NULL;
			x->xcb_cseq = (DML_CSEQ*)NULL;
			x->xcb_pcb_list = NULL;
			x->xcb_crib_ptr = NULL;
			x->xcb_lctx_ptr = NULL;
			x->xcb_jctx_ptr = NULL;

			return (E_DB_OK);
		    }
		    if (xcb != (DML_XCB *)0)
			dm0m_deallocate((DM_OBJECT **)&xcb);
		}
		else
		{
		    status = E_DB_ERROR;
		    SETDBERR(&dmx->error, 0, E_DM0060_TRAN_IN_PROGRESS);
		}
	    }
	    else
	    {
		status = E_DB_ERROR;
		SETDBERR(&dmx->error, 0, E_DM002F_BAD_SESSION_ID);
	    }
	}
	else
	{
	    status = E_DB_ERROR;
	    SETDBERR(&dmx->error, 0, E_DM0010_BAD_DB_ID);
	}

	break;
    }
    
    if (dmx->error.err_code > E_DM_INTERNAL)
    {
	uleFormat( &dmx->error, 0, NULL, ULE_LOG , NULL, 
	    (char * )NULL, 0L, (i4 *)NULL, 
	    &local_error, 0);
	SETDBERR(&dmx->error, 0, E_DM0099_ERROR_RESUME_TRAN);
    }

    return (status);
}
