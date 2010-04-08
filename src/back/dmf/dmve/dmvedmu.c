/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <pc.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <tr.h>
#include    <lg.h>
#include    <lk.h>
#include    <adf.h>
#include    <ulf.h>
#include    <dmf.h>
#include    <dm.h>
#include    <dmp.h>
#include    <dmpp.h>
#include    <dmve.h>
#include    <dm1b.h>
#include    <dm0l.h>
#include    <dm0p.h>

/**
**
**  Name: DMVEDMU.C - The recovery of a DMU operation.
**
**  Description:
**	This file contains the routine for the recovery of a DMU operation.
**
**          dmve_dmu - The recovery of a DMU operation.
**
**  History:
**      11-sep-89 (rogerk)
**          Created for Terminator.
**	25-sep-1991 (mikem) integrated following change: 20-sep-89 (rogerk)
**	    When write ABORTSAVE record, use address in dmu_sp_addr.
**	25-sep-1991 (mikem) integrated following change: 10-jan-90 (rogerk)
**	    If DMVE says the abort is being done by the CSP, then pass
**	    the DM0L_CLUSTER_REQ flag to dm0l_abortsave.
**	07-july-1992 (jnash)
**	    Add DMF function prototyping 
**	3-oct-1992 (bryanp)
**	    Re-arrange includes to share transaction ID type definition.
**	20-oct-1992 (jnash)
**	    Reduced logging project.  Write CLR's instead of abort-save
**	    log records.
**	14-dec-1992 (rogerk)
**	    Include dm1b.h before dm0l.h for prototype definitions.
**	15-mar-1993 (jnash & rogerk)
**	    Reduced Logging - Phase IV:
**	    Changed log record format: added database_id, tran_id and LSN
**		in the log record header.  The LSN of a log record is now
**		obtained from the record itself rather than from the dmve
**		control block.
**	    Check dmve->dmve_logging to determine if logging required.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**		Remove special checks for DCB_S_CSP, since DCB's built by the
**		    CSP process no longer get special handling.
**	24-may-1993 (bryanp)
**	    Include <pc.h> before <lk.h> to pick up PID definition.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (rogerk)
**	    Include respective CL headers for all cl calls.
**	23-aug-1993 (rogerk)
**	    Added support for rollforward of CLR records.
**	15-apr-1994 (chiku)
**	    Bug56702: return logfull indication.
**	25-Aug-1997 (jenjo02)
**	    Added log_id parm to dm0p_toss_pages() calls.
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
**	25-Nov-2008 (jonj)
**	    SIR 120874: dm0l_? functions converted to DB_ERROR *
**/


/*{
** Name: dmve_dmu - The recovery of a DMU opration.
**
** Description:
**	This routine is used for ROLLBACK recovery of a DMU operation.
**
**	DMU operations are not IDEMPOTENT during backout.  That means that we
**	cannot re-execute abort recovery on operations that occured after a
**	DMU operation, if the DMU operation has already been aborted.
**
**	This is because the recovery of many operations makes the assumption
**	the the basic state of the table (storage structure, number of pages,
**	file layout) is the similar to the state it was in when the operation
**	was logged.
**
**	If we abort the following transaction:
**
**	    CREATE newtab (a = i4)
**	    APPEND newtab (a = 1)
**
**	And the server doing the abort crashes after aborting the CREATE,
**	but before completing the abort, then the RCP will start over with
**	recovery on that transaction.  But the RCP cannot attempt to re-execute
**	the abort of the APPEND statement because the table 'newtab' no longer
**	exists.  (Worse cases are when the server died while in the middle of
**	aborting the CREATE - and left the catalogs in an inconsistent state.)
**
**	To solve this, we write a DM0L_DMU log record whenever we issue an
**	operation for which recovery is not idempotent.  This log record must
**	be written AFTER the operation is complete so that recovery is done
**	before the operation is recovered (while recovery can still be
**	re-executed).
**
**	When we encounter a DM0L_DMU log record during abort processing, we
**	lay down a CLR record which points to the record before the DMU 
**	record.  If we re-execute abort processing on this transaction,
**	we will encounter the CLR and jump over the already backed-out 
**	operations and begin with recovery on the DMU operation
**	itself (which by itself must be idempotent).
**
**	Since we must write a CLR record during recovery, LG must make
**	sure the is room in the log file to write one CLR (and
**	force it to disk) for all DMU operations active in the system.
**
** Inputs:
**	dmve_cb
**	    .dmve_log_rec	    The dmu operation log record.
**	    .dmve_action	    Should only be DMVE_UNDO.
**	    .dmve_dcb_ptr	    Pointer to DCB.
**	    .dmve_tran_id	    The physical transaction id.
**	    .dmve_lk_id		    The transaction lock list id.
**	    .dmve_log_id	    The logging system database id.
**	    .dmve_db_lockmode	    The lockmode of the database. Should be 
**				    DM2T_X or DM2T_S.
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
**	11-sep-89 (rogerk)
**          Created for Terminator.
**	25-sep-1991 (mikem) integrated following change: 20-sep-89 (rogerk)
**	    When write ABORTSAVE record, use address in dmu_sp_addr.
**	25-sep-1991 (mikem) integrated following change: 10-jan-90 (rogerk)
**	    If DMVE says the abort is being done by the CSP, then pass
**	    the DM0L_CLUSTER_REQ flag to dm0l_abortsave.
**	20-oct-1992 (jnash)
**	    Reduced logging project.  Write CLR's instead of abort-save
**	    log records.  
**	18-jan-1993 (rogerk)
**	    Reduced Logging Project: Removed OFFLINE_CSP flag.
**	15-mar-1993 (jnash)
**	    Check dmve->dmve_logging to determine if logging required.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**		Remove special checks for DCB_S_CSP, since DCB's built by the
**		    CSP process no longer get special handling.
**	23-aug-1993 (rogerk)
**	    Added support for rollforward of CLR records.
**	    Took out old dm0p_force_pages call (which wrote out records
**	    updated by the current transaction) and replaced it with a
**	    dm0p_toss_pages call - which tosses out all pages belonging
**	    to the dmu table.
**	15-apr-1994 (chiku)
**	    Bug56702: return logfull indication.
*/
DB_STATUS
dmve_dmu(
DMVE_CB		*dmve_cb)
{
    DMVE_CB		*dmve = dmve_cb;
    DM0L_DMU		*log_rec = (DM0L_DMU *)dmve_cb->dmve_log_rec;
    LG_LSN		*log_lsn = &log_rec->dmu_header.lsn; 
    i4		flag;
    i4		error;
    DB_STATUS		status = E_DB_OK;

    CLRDBERR(&dmve->dmve_error);
	
    for (;;)
    {
	if (log_rec->dmu_header.length > sizeof(DM0L_DMU) || 
	    log_rec->dmu_header.type != DM0LDMU)
	{
	    SETDBERR(&dmve->dmve_error, 0, E_DM9601_DMVE_BAD_PARAMETER);
	    status = E_DB_ERROR;
	    break;
	}

	/*
	** Only UNDO modes require any DMU processing.
	*/
	if ((dmve->dmve_action != DMVE_UNDO) && 
	    ((log_rec->dmu_header.flags & DM0L_CLR) == 0))
	{
	    return (E_DB_OK);
	}

	/*
	** Toss any modified pages for this table from the buffer manager
	** as we are about to rename or possible remove the files associated
	** with it.  (DMU records are usually logged in conjunction with
	** file rename/swap operations).
	*/
	dm0p_toss_pages(dmve->dmve_dcb_ptr->dcb_id, 
			log_rec->dmu_tabid.db_tab_base, 
			dmve->dmve_lk_id, dmve->dmve_log_id, (i4) TRUE);

	/*
	** Write CLR record that specifies that we have aborted
	** the transaction up to this log record.
	**
	** DMU log records are written as NONREDO operations to ensure
	** that redo cannot be attempted using the old (about to be renamed)
	** file structures.  (The nonredo status is appended in the dm0l_dmu
	** routine).
	*/
	if ((dmve->dmve_logging) &&
	    ((log_rec->dmu_header.flags & DM0L_CLR) == 0))
	{
	    status = dm0l_dmu(dmve->dmve_log_id, 
			    (log_rec->dmu_header.flags | DM0L_CLR), 
			    &log_rec->dmu_tabid, &log_rec->dmu_tblname, 
			    &log_rec->dmu_tblowner, log_rec->dmu_operation,
			    log_lsn, &dmve->dmve_error);
	    if (status != E_DB_OK) 
	    {
		/*
		 * Bug56702: returned logfull indication.
		 */
		dmve->dmve_logfull = dmve->dmve_error.err_code;
		break;
	    }
	}

	return(E_DB_OK);	
    }

    if (dmve->dmve_error.err_code > E_DM_INTERNAL)
    {
	uleFormat(&dmve->dmve_error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
	    (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	SETDBERR(&dmve->dmve_error, 0, E_DM9622_DMVE_DMU);
    }

    return(E_DB_ERROR);
}
