/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <cm.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <adf.h>
#include    <pc.h>
#include    <me.h>
#include    <tr.h>
#include    <scf.h>
#include    <dmf.h>
#include    <ulf.h>
#include    <dmxcb.h>
#include    <dmrcb.h>
#include    <dmccb.h>
#include    <dmtcb.h>
#include    <lg.h>
#include    <lk.h>
#include    <dm.h>
#include    <dmp.h>
#include    <dml.h>
#include    <dmxe.h>
#include    <dm0m.h>

/**
** Name: DMXSVPT.C - Functions used to create a transaction savepoint.
**
** Description:
**      This file contains the functions necessary to create a transaction
**      savepoint.  This file defines:
**    
**      dmx_savepoint()        -  Routine to perform the savepoint 
**                                transaction operation.
**
** History:
**      01-sep-85 (jennifer)
**          Created for Jupiter.      
**	30-sep-1988 (rogerk)
**	    Added capability to delay writing Begin Transaction record until
**	    first write operation.  If DELAYBT flag is set in the XCB, then
**	    call dmxe to write the BT record.
**	 6-Jun-1989 (ac)
**	    Added 2PC support.
**	17-jan-1990 (rogerk)
**	    Added ability to delay writing the savepoint record until the
**	    first update is performed.  This allows us to avoid log writes
**	    in read-only transactions.
**	 4-feb-1991 (rogerk)
**	    Added support for NOLOGGING.  Don't actually write savepoint
**	    records for NOLOGGING transactions.
**	11-dec-1991 (fred)
**	    Added support for large objects.  In particular, allow temporaries
**	    to remain open while savepoints are declared -- this should probably
**	    be fixed soon.
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
**	26-jul-1993 (rogerk)
**	    Include respective CL headers for all cl calls.
**	21-Jan-1997 (jenjo02)
**	    Remove DM0M_ZERO from dm0m_allocate() for SPCB, manually
**	    initialize remaining fields instead.
**	17-mar-97 (stephenb)
**	    Check for replication input queue open status.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      18-feb-2004 (stial01)
**          Fixed incorrect casting of length arguments.
**      14-dec-2004 (stial01)
**          Fixed dmx_savepoint USER_INTR error handling
**      29-nov-2005 (horda03) Bug 48659/INGSRV 2716
**          A TX can now be started with a non-journaled BT record. This
**          leaves the XCB_DELAYBT flag set but sets the XCB_NONJNL_BT
**          flag. Need to write a journaled BT record if the update
**          will be journaled.
**      30-jan-2006 (horda03) Bug 48659/INGSRV 2716 Additional
**          A non-journalled DB will the XCB_DELAYBT flag set when it has updated
**          a table (i.e. it is not a ROTRAN), in this circumstance the XCB_NONJNL_BT
**          flag will also be set.
**	12-Apr-2008 (kschendel)
**	    Need to include adf.h now.
**	20-Oct-2008 (jonj)
**	    SIR 120874 Modified to use new DB_ERROR based uleFormat 
**	21-Nov-2008 (jonj)
**	    SIR 120874: dmxe_? functions converted to DB_ERROR *
**	24-Nov-2008 (jonj)
**	    SIR 120874: dm0m_? functions converted to DB_ERROR *
**/

/* # defines */
/* typedefs */
/* forward references */
/* externs */
/* statics */


/*{
** Name: dmx_savepoint - Declares a transaction savepoint.
**
**  INTERNAL DMF call format:      status = dmx_savepoint(&dmx_cb);
**
**  EXTERNAL call format:          status = dmf_call(DMX_SAVEPOINT,&dmx_cb);
**
** Description:
**    The dmx_savepoint function handles the creation of a transaction
**    savepoint.  This causes all information required to rollback to a
**    savepoint to be recorded in the savepoint control block and
**    consists of the transaction's last written log record's LSN.
**    Note a savepoint can only be declared if no files are open.
**
** Inputs:
**      dmx_cb 
**          .type                           Must be set to DMX_TRANSACTION_CB.
**          .length                         Must be at least sizeof(DMX_CB).
**          .dmx_tran_id.                  Must be a valid transaction 
**                                          identifier returned from the 
**                                          begin transaction operation.
**          .dmx_savepoint_name             Must be a valid savepoint name.
**          
** Output:
**      dmx_cb 
**          .error.err_code                 One of the following error numbers.
**                                          E_DM0000_OK                
**                                          E_DM000B_BAD_CB_LENGTH
**                                          E_DM000C_BAD_CB_TYPE
**                                          E_DM001A_BAD_FLAG
**                                          E_DM002C_BAD_SAVEPOINT_NAME
**                                          E_DM003B_BAD_TRAN_ID
**                                          E_DM004A_INTERNAL_ERROR
**                                          E_DM0061_TRAN_NOT_IN_PROGRESS
**                                          E_DM0064_USER_ABORT
**                                          E_DM0065_USER_INTR
**					    E_DM0097_ERROR_SAVEPOINTING
**        	                            E_DM009F_ILLEGAL_OPERATION
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
**	30-sep-1988 (rogerk)
**	    Added capability to delay writing Begin Transaction record until
**	    first write operation.  If DELAYBT flag is set in the XCB, then
**	    call dmxe to write the BT record.
**	17-mar-1989 (EdHsu)
**	    Added cluster online backup support
**	17-jan-1990 (rogerk)
**	    Added ability to delay writing the savepoint record until the
**	    first update is performed.  This allows us to avoid log writes
**	    in read-only transactions.  If the DELAYBT flag is set in the
**	    XCB and this is the only savepoint, then rather than writing the
**	    savepoint record, we mark SVPT_DELAY in the xcb.  When the first
**	    update is performed, the Begin Transaction and Savepoint records
**	    will be written.
**	 4-feb-1991 (rogerk)
**	    Added support for NOLOGGING.  Don't actually write savepoint
**	    records for NOLOGGING transactions.
**	11-dec-1991 (fred)
**	    Added ability to leave temporaries open across savepoint
**	    declarations.  This is a hack to allow large objects to work,
**	    temporarily.
**	27-mar-98 (stephenb)
**	    check replication input queue and set flag.
**	20-May-2002 (jenjo02)
**	    Cannot delay savepoint if XA transaction (STAR 11918837)
**	    so that each txn participating in the global txn knows its
**	    starting LSN in the event of a statement abort.
**	11-Nov-2005 (jenjo02)
**	    Replaced dmx_show() with the more robust 
**	    dmxCheckForInterrupt() to standardize external
**	    interrupt handling.
**	26-Jul-2006 (jonj)
**	    Check XCB, not SCB, for XA transaction.
**	11-Sep-2006 (jonj)
**	    Never write a BT nor "delay" a savepoint LSN.
*/

DB_STATUS
dmx_savepoint(
DMX_CB    *dmx_cb)
{
    DML_SCB             *scb;
    DMX_CB		*dmx = dmx_cb;
    DML_XCB		*xcb;
    DB_STATUS		status;
    DB_STATUS           s;
    i4			error = 0; 
    i4			local_error = 0;
    DML_SPCB            *sp = (DML_SPCB*)NULL; 

    CLRDBERR(&dmx->error);

    for (status = E_DB_ERROR;;)
    {
	xcb = (DML_XCB *)dmx->dmx_tran_id;
	if (dm0m_check((DM_OBJECT *)xcb, (i4)XCB_CB) == E_DB_OK)
	{
	    scb = xcb->xcb_scb_ptr;
	    if (scb->scb_x_ref_count == 0)
	    {
		SETDBERR(&dmx->error, 0, E_DM0061_TRAN_NOT_IN_PROGRESS);
		break;
	    }

#ifdef	xDEBUG
	    if (xcb->xcb_rq_next != (DMP_RCB*) &xcb->xcb_rq_next)
	    {
		/* Walk the queue.  If not temporary, then error */
		DMP_RCB		*rcb;
		i4		not_init = 0;

		/*  
		** Find all open tables and return an error if they are not
		** temporary tables.
		*/

		rcb = (DMP_RCB *)((char *)xcb->xcb_rq_next -
			(char *)&(((DMP_RCB*)0)->rcb_xq_next));

		do
		{
		    if (not_init++)
		    {
			rcb = (DMP_RCB *)((char *)rcb->rcb_xq_next -
			    (char *)&(((DMP_RCB*)0)->rcb_xq_next));
		    }
		    if (!rcb->rcb_tcb_ptr->tcb_temporary)
		    {
			TRdisplay("DMX_SAVEPOINT: Unexpected open table '%~t'\n",
			    sizeof(rcb->rcb_tcb_ptr->tcb_rel.relid.db_tab_name),
				rcb->rcb_tcb_ptr->tcb_rel.relid.db_tab_name);
			SETDBERR(&dmx->error, 0, E_DM009F_ILLEGAL_OPERATION);
		    }
		    else
		    {
			TRdisplay("DMX_SAVEPOINT: Found (DMPE?) open table '%~t'\n",
			    sizeof(rcb->rcb_tcb_ptr->tcb_rel.relid.db_tab_name),
				rcb->rcb_tcb_ptr->tcb_rel.relid.db_tab_name);
		    }
		} while (rcb->rcb_xq_next != (DMP_RCB*) &xcb->xcb_rq_next);
		if (dmx->error.err_code)
		    break;
	    }
#endif	        

	    /* Check for external interrupts */
	    if ( scb->scb_ui_state )
		dmxCheckForInterrupt(xcb, &error);

	    if (xcb->xcb_state)
	    {
		if (xcb->xcb_state & XCB_USER_INTR)
		    SETDBERR(&dmx->error, 0, E_DM0065_USER_INTR);
		else if (xcb->xcb_state & XCB_FORCE_ABORT)
		    SETDBERR(&dmx->error, 0, E_DM010C_TRAN_ABORTED);
		else if (xcb->xcb_state & XCB_ABORT)
		    SETDBERR(&dmx->error, 0, E_DM0064_USER_ABORT);
		else if (xcb->xcb_state & XCB_WILLING_COMMIT)
		    SETDBERR(&dmx->error, 0, E_DM0132_ILLEGAL_STMT);
		break;
	    }

	    for (sp = xcb->xcb_sq_next;
		sp != (DML_SPCB*)& xcb->xcb_sq_next;
		sp = sp->spcb_q_next)
	    {
		if (MEcmp((char *)&sp->spcb_name,
		      (char *)&dmx->dmx_savepoint_name,		
		      sizeof(DB_SP_NAME)) == 0)
		{
		    /* Dequeue previous savepoint of the same name. */

		    sp->spcb_q_prev->spcb_q_next = sp->spcb_q_next;
		    sp->spcb_q_next->spcb_q_prev = sp->spcb_q_prev;
		    break;
		}		
	    }

	    if ( sp == (DML_SPCB*)&xcb->xcb_sq_next )
	    {
		status = dm0m_allocate((i4)sizeof(DML_SPCB), 0, 
		    (i4)SPCB_CB, (i4)SPCB_ASCII_ID, (char *)xcb, 
		    (DM_OBJECT **)&sp, &dmx->error);
		if (status != E_DB_OK)
		    break;
		sp->spcb_xcb_ptr = xcb;
		STRUCT_ASSIGN_MACRO(dmx->dmx_savepoint_name, sp->spcb_name);
		/* Initialze remaining SPCB fields */
		sp->spcb_lsn.lsn_high = sp->spcb_lsn.lsn_low = 0;

	    }

	    /*  Assign new savepoint identifier. */

	    sp->spcb_id = ++xcb->xcb_sp_id;

	    /*
	    ** Savepoint log records are never written,
	    ** the DML_SPCB just records the current last-written
	    ** LSN of the transaction.
	    **
	    ** As savepoints are only useful during online
	    ** dmxe_abort, there's no need to begin a
	    ** transaction here just for the sake of
	    ** having a record of the transaction's BT,
	    ** or "delay" the acquisition of the BT 
	    ** LSN.
	    **
	    ** dmxe_savepoint/dm0l_savepoint will 
	    ** record whatever LSN happens to be the
	    ** "last LSN" of the log transaction.
	    ** If a BT hasn't been written, then this
	    ** LSN will be 0, but we don't care.
	    **
	    ** If we end up doing a rollback to savepoint,
	    ** dmxe_abort will rollback all log records
	    ** up to that LSN. If it finds the BT first,
	    ** that's fine; a savepoint LSN of 0 means
	    ** the same as "BT".
	    **
	    ** Whenever it decides that recovery is done
	    ** for this savepoint, dmxe_abort will update
	    ** our spcb_lsn with the LSN is quit on, be
	    ** that the LSN of the BT or the LSN we
	    ** acquired here.
	    **
	    ** Note that this change is not completely
	    ** arbitrary; LOGFULL_COMMIT protocols
	    ** may leave (shared) transactions with
	    ** stale LSNs in the savepoints and this
	    ** ensures that that doesn't matter.
	    */
	    status = dmxe_savepoint(xcb, sp, &dmx->error);
	    if (status != E_DB_OK)
		break;

	    /* Queue it to the end of the xcb savepoint queue. */

	    sp->spcb_q_next = (DML_SPCB *)&xcb->xcb_sq_next;
	    sp->spcb_q_prev = (DML_SPCB *)xcb->xcb_sq_prev;
	    xcb->xcb_sq_prev->spcb_q_next = sp;
	    xcb->xcb_sq_prev = sp;
	    
	    /* Turn off user interrupt if on. */

	    scb->scb_ui_state &= ~SCB_USER_INTR;
	    xcb->xcb_state &= ~XCB_USER_INTR;
	    /* check for open replication input queue and set flag */
	    if (xcb->xcb_rep_input_q)
		sp->spcb_flags |= SPCB_REP_IQ_OPEN;
	    return (E_DB_OK);
	}    
	SETDBERR(&dmx->error, 0, E_DM003B_BAD_TRAN_ID);
	break;
    }

    /*	Error recovery handling. */

    if ( sp && sp != (DML_SPCB*)&xcb->xcb_sq_next )
	dm0m_deallocate((DM_OBJECT **)&sp);
    if (dmx->error.err_code == E_DM0100_DB_INCONSISTENT)
	xcb->xcb_state |= XCB_TRANABORT;
    else if (dmx->error.err_code > E_DM_INTERNAL)
    {
	uleFormat(&dmx->error, 0, NULL, ULE_LOG, NULL, 
	    (char * )NULL, (i4)0, (i4 *)NULL, &local_error, 0);
	SETDBERR(&dmx->error, 0, E_DM0097_ERROR_SAVEPOINTING);
    }
    return (status);
}
