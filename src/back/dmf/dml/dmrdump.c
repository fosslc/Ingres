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
#include    <lg.h>
#include    <lk.h>
#include    <dm.h>
#include    <dmp.h>
#include    <dmrcb.h>
#include    <dmccb.h>
#include    <dmxcb.h>
#include    <dmtcb.h>
#include    <dml.h>
#include    <dm0m.h>
#include    <dm2r.h>
#include    <dm1s.h>
#include    <sr.h>
#include    <dmse.h>
#include    <dm1b.h>
#include    <dm2umct.h>
#include    <dm2rlct.h>

/**
** Name: DMRDUMP.C - Implements the DMF dump data record operation.
**
** Description:
**      This file contains the functions necessary to implement the 
**	dump data table operation which allows a temporary file
**      to be have its logical end of file reset to zero.
**
**      This file defines:
**
**      dmr_dump_data - Reset the logical end of file of temporary.
**
** History:
**	24-sep-1986 (jennifer)
**	    Created for Jupiter.
**	 8-jul-1992 (walt0
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
**      31-Jul-2002 (hanal04) Bug 107132 INGSRV 1692
**          Resolve SIGSEGV in dmr_get() when qen_tsort() performs
**          a DMF (DMSE) sort.
**	12-Apr-2008 (kschendel)
**	    Move adf.h for dmp.h.
**	20-Oct-2008 (jonj)
**	    SIR 120874 Modified to use new DB_ERROR based uleFormat 
**	25-Nov-2008 (jonj)
**	    SIR 120874: dmse_? functions converted to DB_ERROR *
**	03-Dec-2008 (jonj)
**	    SIR 120874: dm1s_? functions converted to DB_ERROR *
**	25-Aug-2009 (kschendel) 121804
**	    Need dm0m.h to satisfy gcc 4.3.
**	03-Nov-2010 (jonj) SIR 124685 Prototype Cleanup
**	    Include dml.h to get prototype for dmr_dump_data().
**/

/*{
** Name:  dmr_dump_data - Reset end of file for temporary.
**
**  INTERNAL DMF call format:      status = dmr_dump_data(&dmr_cb);
**
**  EXTERNAL call format:          status = dmf_call(DMR_DUMP_DATA, &dmr_cb);
**
**	Dump Data is called to reset the end of file pointer on a
**      temporary file to zero.
**
** Inputs:
**      dmr_cb
**          .type                           Must be set to DMR_RECORD_CB.
**          .length                         Must be at least
**                                          sizeof(DMR_RECORD_CB) bytes.
**          .dmr_flags_mask                 Must be zero.
**          .dmr_access_id                  Record access identifer returned
**                                          from DMT_CREATE_TEMP that identifies
**					    the table to load.
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
**                                          E_DM004A_INTERNAL_ERROR
**                                          E_DM0065_USER_INTR
**                                          E_DM0064_USER_ABORT
**                                          E_DM009F_ILLEGAL_OPERATION
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
**         E_DB_FATAL			    Function completed with a fatal
**                                          error which must be handled
**                                          immediately.  The fatal status is in
**                                          dmr_cb.err_code.
** History:
**	14-apr-1986 (derek)
**	    Created for Jupiter.
**      31-Jul-2002 (hanal04) Bug 107132 INGSRV 1692
**          Accept the DMR_SORT_NOCOPY flag and end the DMSE sort when
**          specified.
**	27-Jul-2004 (schka24)
**	    Don't need NOCOPY flag, look at RCB state instead.
**	03-Mar-2010 (jonj)
**	    SIR 121619 MVCC, blob support:
**	    Set rcb_dmr_opcode here; dmpe bypasses dmf_call,
**	    which used to set it.
*/
DB_STATUS
dmr_dump_data(
DMR_CB  *dmr_cb)
{
    DMR_CB		*dmr = dmr_cb;
    DMP_RCB		*rcb;
    DB_STATUS		status;
    i4			error;
    DMP_TCB		*tcb;
    DM2R_L_CONTEXT	*lct;
    DB_STATUS		dmse_status = E_DB_OK;

    CLRDBERR(&dmr->error);

    status = E_DB_ERROR;
    if (dmr->dmr_flags_mask == 0)
    {
	rcb = (DMP_RCB *)dmr->dmr_access_id;

	if (dm0m_check((DM_OBJECT *)rcb, (i4)RCB_CB) == E_DB_OK)
	{
	    rcb->rcb_dmr_opcode = DMR_DUMP_DATA;

	    /* If RCB is in LSTART state, we were getting rows out of
	    ** a loaded sorter directly.  (or perhaps still putting rows.)
	    ** Close out the sorter.
	    */
	    if (rcb->rcb_state & RCB_LSTART)
	    {
                tcb = rcb->rcb_tcb_ptr;
		lct = (DM2R_L_CONTEXT *)tcb->tcb_lct_ptr;

		/* There should be an LCT but be paranoid */
		if (lct != NULL)
		{
		    dmse_status = dmse_end(lct->lct_srt, &dmr->error);
                
		    if(dmse_status == E_DB_OK)
		    {
			/* Deallocate load context */
			if (lct->lct_mct.mct_buffer != (PTR)0)
			{
			    dm0m_deallocate((DM_OBJECT **)&lct->lct_mct.mct_buffer);
			}
			dm0m_deallocate((DM_OBJECT **)&tcb->tcb_lct_ptr);
			tcb->tcb_lct_ptr = 0;
		    }
		    else
		    {
			if (dmr->error.err_code > E_DM_INTERNAL)
			{
			    DB_STATUS       local_error;
 
			    uleFormat( &dmr->error, 0, NULL, ULE_LOG , NULL,
				    (char * )NULL, 0L, (i4 *)NULL,
				    &local_error, 0);
			    SETDBERR(&dmr->error, 0, E_DM0108_ERROR_DUMPING_DATA);
			}
			return (status);
		    }
		}
		rcb->rcb_state &= ~RCB_LSTART;
	    }

	    if (rcb->rcb_tcb_ptr->tcb_temporary == TCB_TEMPORARY &&
		rcb->rcb_tcb_ptr->tcb_rel.relspec == TCB_HEAP)
	    {
		status = dm1s_deallocate(rcb, &dmr->error);	
		if (status == E_DB_OK)
		    return (status);
	    }
	    else
		SETDBERR(&dmr->error, 0, E_DM009F_ILLEGAL_OPERATION);
	}
	else
	    SETDBERR(&dmr->error, 0, E_DM002B_BAD_RECORD_ID);
    }
    else
	SETDBERR(&dmr->error, 0, E_DM001A_BAD_FLAG);

    if (dmr->error.err_code > E_DM_INTERNAL)
    {
	DB_STATUS	local_error;

	uleFormat( &dmr->error, 0, NULL, ULE_LOG , NULL, 
	    (char * )NULL, 0L, (i4 *)NULL, 
	    &local_error, 0);
	SETDBERR(&dmr->error, 0, E_DM0108_ERROR_DUMPING_DATA);
    }
    return (status);
}
