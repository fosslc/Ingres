/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <di.h>
#include    <pc.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <lg.h>
#include    <lk.h>
#include    <adf.h>
#include    <ulf.h>
#include    <dmf.h>
#include    <dm.h>
#include    <dmp.h>
#include    <dmve.h>
#include    <dmpp.h>
#include    <dm0c.h>
#include    <dm1b.h>
#include    <dm0l.h>
#include    <dmtcb.h>
#include    <dm2t.h>

/**
**
**  Name: DMVESM0.C - The recovery of a sysmod load-completed operation
**
**  Description:
**	This file contains the routine for the recovery of a sysmod load
**	completed operation.
**
**          dmve_sm0_closepurge - Recovery of a sysmod load-completed operation
**
**  History:
**	25-sep-1991 (mikem) integrated following change: 16-jul-1991 (bryanp)
**	    B38527: Added this file to add rollforward support for sysmod.
**	7-july-92 (jnash)
**	    Add DMF function prototyping.
**	3-oct-1992 (bryanp)
**	    Re-arrange includes to share transaction ID type definition.
**	14-dec-1992 (rogerk)
**	    Include dm1b.h before dm0l.h for prototype definitions.
**	24-may-1993 (bryanp)
**	    Include <pc.h> before <lk.h> to pick up PID definition.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      20-may-95 (hanch04)
**          Added include dmtcb.h for ndmfapi
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      14-jul-2003
**          Added include adf.h
**	14-Nov-2008 (jonj)
**	    SIR 120874 Change function prototypes to pass DB_ERROR *dberr
**	    instead of i4 *err_code, use new form uleFormat.
**	28-Nov-2008 (jonj)
**	    SIR 120874: dm2t_? functions converted to DB_ERROR *
**	21-Jul-2010 (stial01) (SIR 121123 Long Ids)
**          Remove table name,owner from log records.
**/

/*{
** Name: dmve_sm0_closepurge - Recovery of a sysmod load-completed operation
**
** Description:
**	This routine handles the log record which is written just after we
**	complete the loading of iirtemp. During normal processing, we
**	close iirtemp with a special PURGE operation code to force all the
**	iirtemp pages to be forced to disk, then we perform a magic rename
**	operation to swap iirtemp's physical file with the core catalog
**	we're modifying. Finally, we destroy iirtemp, which (since we renamed
**	the files) actually destroys the old core catalog.
**
**	No special operations are needed for this log record during UNDO and
**	REDO. During rollforward, we use this log record to signal that the
**	load of iirtemp has been completed, and we force all the iirtemp
**	pages to disk and purge the TCB.
**
** Inputs:
**	dmve_cb
**	    .dmve_log_rec	    The rename file log record.
**	    .dmve_action	    Should be DMVE_UNDO, DMVE_REDO or DMVE_DO.
**	    .dmve_dcb_ptr	    Pointer to DCB.
**
** Outputs:
**	dmve_cb
**	    .dmve_error.err_code    The reason for error status.
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	25-sep-1991 (mikem) integrated following change: 16-jul-1991 (bryanp)
**	    B38527: Added rollforward support for sysmod.
*/
DB_STATUS
dmve_sm0_closepurge(
DMVE_CB		*dmve_cb)
{
    DMVE_CB		*dmve = dmve_cb;
    DB_STATUS		status = E_DB_OK;
    i4		error = E_DB_OK;
    DM0L_SM0_CLOSEPURGE	*log_rec = (DM0L_SM0_CLOSEPURGE*)dmve_cb->dmve_log_rec;
    DB_TAB_ID           *tbl_id;

    CLRDBERR(&dmve->dmve_error);
    DMVE_CLEAR_TABINFO_MACRO(dmve);
	
    for (;;)
    {
	if (log_rec->sm0_header.length != sizeof(DM0L_SM0_CLOSEPURGE) || 
	    log_rec->sm0_header.type != DM0LSM0CLOSEPURGE)
	{
	    uleFormat(NULL, E_DM9601_DMVE_BAD_PARAMETER, 0, ULE_LOG, NULL, 0, 0, 0, 
			&error, 0);
	    break;
	}

	tbl_id = &log_rec->sm0_tbl_id;

	switch (dmve->dmve_action)
	{
	case DMVE_UNDO:
	    /* No UNDO action is needed for this log record */
	    break;
	case DMVE_REDO:
	    /* Don't ever redo sysmod operation. */
	    break;
	case DMVE_DO:
						
	    status = dm2t_purge_tcb(dmve->dmve_dcb_ptr, tbl_id,
					DM2T_PURGE, dmve->dmve_log_id,
					dmve->dmve_lk_id, &dmve->dmve_tran_id,
					dmve->dmve_db_lockmode, &dmve->dmve_error);
	    break;    
	} /* end switch. */
	if (status != E_DB_OK)
	{
	    uleFormat(&dmve->dmve_error, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL, &error, 0);
	    break;
	}
	return(E_DB_OK);

    } /* end for. */
    if (dmve->dmve_error.err_code > E_DM_INTERNAL)
    {
	uleFormat(&dmve->dmve_error, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL, &error, 0);
	SETDBERR(&dmve->dmve_error, 0, E_DM9629_DMVE_SM0CLOSEPURGE);
    }
    return(status);
}
