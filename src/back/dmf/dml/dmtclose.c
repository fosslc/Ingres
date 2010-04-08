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
#include    <dmf.h>
#include    <ulf.h>
#include    <dmtcb.h>
#include    <dmrcb.h>
#include    <dmccb.h>
#include    <dmxcb.h>
#include    <lg.h>
#include    <lk.h>
#include    <dm.h>
#include    <dmp.h>
#include    <dml.h>
#include    <dm2t.h>
#include    <dmftrace.h>

/**
** Name: DMTCLOSE.C - Implements the DMF close table operation.
**
** Description:
**      This file contains the functions necessary to close a table.
**      This file defines:
**
**      dmt_close - Close a table.
**
** History:
**      01-sep-85 (jennifer)
**          Created for Jupiter.
**	17-dec-1985 (derek)
**	    Completed code for dmt_close().
**      10-oct-1991 (jnash) merged 14-jun-1991 (Derek)
**          Added performance profiling support.
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
**      13-jun-97 (dilma04)
**         Add check for the new DMT_CB flag - DMT_CONSTRAINT.
**	10-nov-1998 (somsa01)
**	    If the table we are closing contains blob columns, make sure
**	    we close any extension tables which are open as well.
**      27-mar-2000 (stial01)
**	    (Bug #94114)
**          b101047: Do not reset SCB_BLOB_OPTIM if DMT_BLOB_OPTIM is specified
**      21-apr-1999 (hanch04)
**          Replace STindex with STchr
**      28-apr-1999 (hanch04)
**          Replaced STlcopy with STncpy
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	12-Apr-2008 (kschendel)
**	    Need to include adf.h now.
**	20-Oct-2008 (jonj)
**	    SIR 120874 Modified to use new DB_ERROR based uleFormat 
**	28-Nov-2008 (jonj)
**	    SIR 120874: dm2t_? functions converted to DB_ERROR *
**/

/*{
** Name:  dmt_close - Close a table.
**
**  INTERNAL DMF call format:  status = dmt_close(&dmt_cb);
**
**  EXTERNAL call format:      status = dmf_call(DMT_CLOSE,&dmt_cb);
**
** Description:
**      This function closes a table that was previously opened.
**      All internal control information is released including the
**      record access identifier needed to access the table.
**
** Inputs:
**      dmt_cb
**          .type                           Must be set to DMT_TABLE_CB.
**          .length                         Must be at least
**                                          sizeof(DMT_TABLE_CB) bytes.
**          .dmt_flags_mask                 Of no consequence, ignored.
**          .dmt_record_access_id           Identifies open table to close.
**
** Outputs:
**      dmt_cb
**          .error.err_code                 One of the following error numbers.
**                                          E_DM0000_OK                
**                                          E_DM000B_BAD_CB_LENGTH
**                                          E_DM000C_BAD_CB_TYPE
**                                          E_DM001A_BAD_FLAG
**                                          E_DM002B_BAD_RECORD_ID
**                                          E_DM0042_DEADLOCK
**                                          E_DM004A_INTERNAL_ERROR
**                                          E_DM004B_LOCK_QUOTA_EXCEEDED
**					    E_DM004D_LOCK_TIMER_EXPIRED
**                                          E_DM0055_NONEXT
**                                          E_DM0064_USER_ABORT
**					    E_DM0090_ERROR_CLOSING_TABLE
**                                          E_DM0100_DB_INCONSISTENT
**                                          E_DM010C_TRAN_ABORTED
**                                          E_DM0112_RESOURCE_QUOTA_EXCEEDED
**
**      Returns:
**         E_DB_OK                          Function completed normally.
**         E_DB_WARN                        Function completed normally with a 
**                                          termination status which is in
**                                          dmt_cb.err_code.
**         E_DB_ERROR                       Function completed abnormally 
**                                          with a 
**                                          termination status which is in
**                                          dmt_cb.err_code.
**         E_DB_FATAL                       Function completed with a fatal
**                                          error which must be handled
**                                          immediately.  The fatal status is in
**                                          dmt_cb.err_code.
** History:
**      01-sep-1985 (jennifer)
**	    Created new for jupiter.
**	17-dec-1985 (derek)
**	    Completed code.
**	10-oct-1991 (jnash) merged 14-jun-1991 (Derek)
**	    Added performance profiling support.
**      13-jun-97 (dilma04)
**         Add check for the new DMT_CB flag - DMT_CONSTRAINT.
**	10-nov-1998 (somsa01)
**	    If the table we are closing contains blob columns, make sure
**	    we close any extension tables which are open as well.
**	    (Bug #94114)
**	14-oct-99 (stephenb)
**	    re-set the SCB_BLOB_OPTIM flag here.
**	10-May-2004 (schka24)
**	    Remove blob-optim flags from scb and dmtcb.
**	    Name-decode trick for closing blob etabs didn't work for
**	    session temp etabs, use new parent-ID in etab TCB.
**	8-Aug-2005 (schka24)
**	    Ignore session-temp flag now left over from open.
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Removed silly validation of dmt_flags_mask,
**	    which aren't used to close a table and are of no import.
**	03-Mar-2010 (jonj)
**	    SIR 121619 MVCC: Blob support: Consistently reference tcb_extended
**	    instead of relstat2 & TCB2_EXTENSION.
*/
DB_STATUS
dmt_close(
DMT_CB  *dmt_cb)
{
    DMT_CB	    *dmt = dmt_cb;
    DMP_RCB	    *rcb;
    i4	    error;
    DB_STATUS	    status;
    bool            internal_error = 0;

    CLRDBERR(&dmt->error);
   
    status = E_DB_ERROR;

    rcb = (DMP_RCB *)dmt->dmt_record_access_id;
    if (dm0m_check((DM_OBJECT *)rcb, (i4)RCB_CB) == E_DB_OK)
    {
	DML_SCB	       	*scb = rcb->rcb_xcb_ptr->xcb_scb_ptr;
	if (rcb->rcb_state & RCB_OPEN)
	{
	    /*  Dequeue the RCB from the XCB. */

	    rcb->rcb_xq_next->rcb_q_prev = rcb->rcb_xq_prev;
	    rcb->rcb_xq_prev->rcb_q_next = rcb->rcb_xq_next;

#ifdef xDEBUG
	    if (DMZ_SES_MACRO(21) || DMZ_SES_MACRO(20))
		dmd_rstat(rcb);
#endif

	    /*
	    ** First, see if this table has open extension tables.
	    ** If it does, then close them first.
	    */
	    if (rcb->rcb_tcb_ptr->tcb_rel.relstat2 & TCB2_HAS_EXTENSIONS)
	    {
		DML_XCB	*next;

		/*
		** Scan the queue of open RCB's.
		*/
		for (next = (DML_XCB*) rcb->rcb_xcb_ptr->xcb_rq_next;
		    next != (DML_XCB*) &rcb->rcb_xcb_ptr->xcb_rq_next;
		    next = next->xcb_q_next)
		{
		    DMP_RCB	*next_rcb;

		    /*
		    ** Calculate the RCB starting address.
		    */
		    next_rcb = (DMP_RCB *)(
		    (char *)next - ((char *)&((DMP_RCB*)0)->rcb_xq_next));

		    /*
		    ** If this table is an extension table, see if its
		    ** base table is the table we are closing. If it is,
		    ** close the extension table as well.
		    */
		    if ( next_rcb->rcb_tcb_ptr->tcb_extended &&
		         next_rcb->rcb_et_parent_id ==
			  rcb->rcb_tcb_ptr->tcb_rel.reltid.db_tab_base)
		    {
			DMP_TCB	*etab_tcb = next_rcb->rcb_tcb_ptr;

			/* Dequeue the RCB from the XCB. */
			next_rcb->rcb_xq_next->rcb_q_prev =
				    next_rcb->rcb_xq_prev;
			next_rcb->rcb_xq_prev->rcb_q_next =
				    next_rcb->rcb_xq_next;

			status = dm2t_close(next_rcb, (i4)0, &dmt->error);
			if (status != E_DB_OK)
			    internal_error = 1;
		    }
		    if (internal_error)
			break;
		}
	    }

	    /*  Call level 2 to close the RCB. */

	    if (!internal_error)
	    {
		status = dm2t_close(rcb, (i4)0, &dmt->error);
		if (status == E_DB_OK)
		    return (status);
	    }
	}
	else
	    SETDBERR(&dmt->error, 0, E_DM002B_BAD_RECORD_ID);
    }
    else
	SETDBERR(&dmt->error, 0, E_DM002B_BAD_RECORD_ID);

    if (dmt->error.err_code > E_DM_INTERNAL)
    {
	STATUS	    local_error;

	uleFormat(&dmt->error, 0, NULL, ULE_LOG, NULL, 
		(char * )NULL, 0L, (i4 *)NULL, &local_error, 0);
	SETDBERR(&dmt->error, 0, E_DM0090_ERROR_CLOSING_TABLE);
    }
    return (status);
}
