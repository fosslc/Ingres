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
#include    <dmtcb.h>
#include    <dm2t.h>
#include    <dmve.h>
#include    <dmpp.h>
#include    <dm1b.h>
#include    <dm0l.h>
#include    <dm0p.h>

/**
**
**  Name: DMVEDEST.C - The recovery of a destroy table operation.
**
**  Description:
**	This file contains the routine for the recovery of a destroy table
**	operation.
**
**          dmve_destroy - The recovery of a destroy table operation.
**
**
**  History:
**      30-jun-86 (ac)    
**          Created new for Jupiter.
**	10-apr-1992 (bryanp)
**	    B43580: When rolling forward a destroy, purge the table.
**	07-july-1992 (jnash)
**	    Add DMF function prototyping 
**	3-oct-1992 (bryanp)
**	    Re-arrange includes to share transaction ID type definition.
**	28-nov-1992 (jnash)
**	    Reduced logging project. Program restructured, add CLR handling.
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
**	24-may-1993 (bryanp)
**	    Include <pc.h> before <lk.h> to pick up PID definition.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (rogerk)
**	    Include respective CL headers for all cl calls.
**	23-aug-1993 (rogerk)
**	    Added rollforward processing for destroy record.
**	    Added dmve_redestroy routine.
**	18-oct-1993 (rmuth)
**	    CL prototype, add <dm0p.h>.
**	18-oct-1993 (rogerk)
**	    Added dmve_unreserve_space call to release logfile space
**	    reserved for logforces in the purge tcb call.
**	15-apr-1994 (chiku)
**	    Bug56702: return logfull indication.
**      20-may-95 (hanch04)
**          Added include dmtcb.h for ndmfapi
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
**	28-Nov-2008 (jonj)
**	    SIR 120874: dm2t_? functions converted to DB_ERROR *
**/

static DB_STATUS 	dmv_undestroy(
				DMVE_CB		*dmve);

static DB_STATUS 	dmv_redestroy(
				DMVE_CB		*dmve);



/*{
** Name: dmve_destroy - The recovery of a destroy table opration.
**
** Description:
**	This function re-execute the destroy table opration using the
**	parameters stored in the log record. The system table changes,
**	the file creation and the file rename operations associated
**	with this destroy table operation appear later in the journal
**	file, so these opeorations are not performed during recovery.
**	This routine has UNDO code to purge the tcb of the destroyed table.  
**      If this is a secondary index, purge the base table's tcb.
**
**	When performing rollforward of a destroy table operation, we need to
**	purge the table. We are about to destroy the physical underlying file
**	for the table (when we encounter the FRENAME operation, a few records
**	later in the journal). Before we destroy the underlying file, however,
**	we'd better ensure that there are no pages in rollforward's buffer
**	cache for this table. That is the purpose of purging the table. Before
**	we added this purge of the table during rollforward, we were susceptible
**	to bug B43580, in which we'd rolled forward some operations on a table
**	which made some pages for the table resident in the cache, then
**	rolled forward a DROP TABLE operation, which deleted the underlying
**	file.
**
**	We had failed to purge the table, however, and so the pages hung
**	around in the cache, and eventually got flushed out by normal cache
**	flushing operations. On most systems, the request to delete the file
**	while it was still open had been deferred until the last opener of the
**	file closed the file, and thus we were still able to come back later
**	and flush out the pages.
**
**	However, not all implementations of DIdelete operate this way. In
**	particular, on Unix systems where the DI layer may LRU-out file
**	descriptors (close and re-open them "under the covers"), a DIdelete on
**	an open DI file is deferred only until that DI file gets LRU-d out, at
**	which point the delete takes effect, and later reads and writes to the
**	file fail with "file not found". So if the order of operations was:
**		1) roll forward some operation on the table making a page
**		   resident.
**		2) roll forward the delete table and the FRENAME and call
**		   DIdelete on the underlying file.
**		3) LRU the file out within DI
**		4) Attempt to write the resident page out.
**	then the DIwrite call in step (4) failed with DI_FILENOTFOUND (!).
**
**	By close-purging the table here, we ensure that when the delete the file
**	upon encountering the FRENAME, the file is truly closed at that point
**	and the delete takes effect immediately.
**
** Inputs:
**	dmve_cb
**	    .dmve_log_rec	    The destroy table operation log record.
**	    .dmve_action	    Should only be DMVE_DO.
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
**	30-jun-86 (ac)
**          Created for Jupiter.
**	03-jun-88 (teg)
**	    Added logic to verify tcb exists before purgeing it.
**	10-apr-1992 (bryanp)
**	    B43580: When rolling forward a destroy, purge the table.
**	28-nov-1992 (jnash)
**	    Reduced logging project.  Program restructured, CLR
**	    handling added.
**	23-aug-1993 (rogerk)
**	    Added dmve_redestroy routine.
**	13-Jul-2007 (kibro01) b118695
**	    DM0L_DESTROY structure may be larger if the number of locations
**	    exceeds DM_MAX_LOC
**	21-Jul-2010 (stial01) (SIR 121123 Long Ids)
**          Remove table name,owner from log records.
*/
DB_STATUS
dmve_destroy(
DMVE_CB		*dmve_cb)
{
    DMVE_CB		*dmve = dmve_cb;
    DM0L_DESTROY	*log_rec = (DM0L_DESTROY *)dmve_cb->dmve_log_rec;
    DB_STATUS		status = E_DB_OK;
    i4		error = E_DB_OK;
    i4		recovery_action;
    i4		actual_size;

    CLRDBERR(&dmve->dmve_error);
    DMVE_CLEAR_TABINFO_MACRO(dmve);

    for (;;)
    {
	if (log_rec->dud_header.type != DM0LDESTROY ||
	    dmve->dmve_action == DMVE_REDO)
	{
	    SETDBERR(&dmve->dmve_error, 0, E_DM9601_DMVE_BAD_PARAMETER);
	    break;
	}

	actual_size = sizeof(DM0L_DESTROY);
	if (log_rec->dud_loc_count > DM_LOC_MAX)
	    actual_size +=
		(log_rec->dud_loc_count - DM_LOC_MAX) * sizeof(DB_LOC_NAME);

	if (log_rec->dud_header.length > sizeof(DM0L_DESTROY))
	{
	    SETDBERR(&dmve->dmve_error, 0, E_DM9601_DMVE_BAD_PARAMETER);
	    break;
	}

	/*
	** Call appropriate recovery action depending on the recovery type
	** and record flags.  CLR actions are always executed as an UNDO
	** operation.  Code within UNDO recognizes the CLR and does not
	** write another CLR log record.
	*/
	recovery_action = dmve->dmve_action;
	if (log_rec->dud_header.flags & DM0L_CLR)
	    recovery_action = DMVE_UNDO;

        switch (recovery_action)
	{
	case DMVE_DO:
	    status = dmv_redestroy(dmve);
	    break;

	case DMVE_UNDO:
	    status = dmv_undestroy(dmve);
	    break;
	}

	if (status != E_DB_OK)
	    break;

	return(E_DB_OK);
    }

    uleFormat(&dmve->dmve_error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
	(char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
    SETDBERR(&dmve->dmve_error, 0, E_DM9608_DMVE_DESTROY);

    return(E_DB_ERROR);
}

/*
** Name: dmv_redestroy	- REDO Recovery of a destroy table operation
**
** Description:
**
**	This routine is called in ROLLFORWARD to re-process a destroy table
**	operation.  Since most of the destroy work is done by redoing the
**	system catalog deletes and file operation log records, there is
**	very little to do here.
**
**	We toss any pages from the cache that belong to the table we are
**	about to destroy and purge any tcb's from the tcb cache.
**
** Inputs:
**	dmve_cb
**	    .dmve_log_rec	    The destroy table operation log record.
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
**	23-aug-1993 (rogerk)
**	    Added rollforward processing for destroy record.
*/
static DB_STATUS
dmv_redestroy(
DMVE_CB         *dmve)
{
    DM0L_DESTROY	*log_rec = (DM0L_DESTROY *)dmve->dmve_log_rec;
    DB_TAB_ID		table_id;
    DB_STATUS		status = E_DB_OK;
    i4			*err_code = &dmve->dmve_error.err_code;

    CLRDBERR(&dmve->dmve_error);

    for (;;)
    {
	/*
	** Toss any modified pages for this table from the buffer manager
	** as we are about to destroy it.
	*/
	dm0p_toss_pages(dmve->dmve_dcb_ptr->dcb_id, 
			log_rec->dud_tbl_id.db_tab_base, 
			dmve->dmve_lk_id, dmve->dmve_log_id, (i4) TRUE);

	/*
	** Purge the table descriptor for the table we are about to destroy.
	**
	** If the table is a secondary index, then the purge should be done
	** on the base table instead of the index.
	*/
	table_id.db_tab_base = log_rec->dud_tbl_id.db_tab_base;
	table_id.db_tab_index = 0;
	status = dm2t_purge_tcb(dmve->dmve_dcb_ptr, &table_id, 
			DM2T_PURGE, dmve->dmve_log_id, 
			dmve->dmve_lk_id, &dmve->dmve_tran_id, 
			dmve->dmve_db_lockmode, &dmve->dmve_error);
	if (status != E_DB_OK)
	    break;

	return (E_DB_OK);
    }

    uleFormat(&dmve->dmve_error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
	(char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
    SETDBERR(&dmve->dmve_error, 0, E_DM967E_REDO_DESTROY);

    return (E_DB_ERROR);
}

/*
** Name: dmv_undestroy	- UNDO Recovery of a destroy table operation
**
** Description:
**
**	This routine is used to perform UNDO recovery of 
**	a destroy table operation.
**
** Inputs:
**	dmve_cb
**	    .dmve_log_rec	    The destroy table operation log record.
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
**	28-nov-1992 (jnash)
**	    Restructured for reduced logging project.
**	15-mar-1993 (jnash)
**	    Check dmve->dmve_logging to determine if logging required.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	23-aug-1993 (rogerk)
**	    Took out dual tcb-purges in secondary index drops (for base
**	    and secondary).  One purge on the base table is sufficient.
**	18-oct-1993 (rogerk)
**	    Added dmve_unreserve_space call to release logfile space
**	    reserved for logforces in the purge tcb call.
**	15-apr-1994 (chiku)
**	    Bug56702: return logfull indication.
*/
static DB_STATUS
dmv_undestroy(
DMVE_CB         *dmve)
{
    DM0L_DESTROY	*log_rec = (DM0L_DESTROY *)dmve->dmve_log_rec;
    LG_LSN		*log_lsn = &log_rec->dud_header.lsn; 
    LG_LSN		lsn;
    DB_TAB_ID		table_id;
    i4		dm0l_flags;
    DB_STATUS		status = E_DB_OK;
    i4			*err_code = &dmve->dmve_error.err_code;

    CLRDBERR(&dmve->dmve_error);

    for (;;)
    {
	/*
	** Purge the table descriptor for the undestroyed table.  Next
	** time the table is accessed, we will have to get fresh info
	** from the system catalogs.
	**
	** If the table is a secondary index, then the purge should be done
	** on the base table instead of the index.
	*/
	table_id.db_tab_base = log_rec->dud_tbl_id.db_tab_base;
	table_id.db_tab_index = 0;
	status = dm2t_purge_tcb(dmve->dmve_dcb_ptr, &table_id, 
			DM2T_PURGE, dmve->dmve_log_id, 
			dmve->dmve_lk_id, &dmve->dmve_tran_id, 
			dmve->dmve_db_lockmode, &dmve->dmve_error);
	if (status != E_DB_OK)
	    break;

	/*
	** Write CLR if necessary
	*/
	if ((dmve->dmve_logging) &&
	    ((log_rec->dud_header.flags & DM0L_CLR) == 0))
	{
	    dm0l_flags = (log_rec->dud_header.flags | DM0L_CLR);

	    status = dm0l_destroy(dmve->dmve_log_id, dm0l_flags, 
		&log_rec->dud_tbl_id, &log_rec->dud_name, 
		&log_rec->dud_owner, log_rec->dud_loc_count,
		&log_rec->dud_location[0], log_lsn, &lsn, &dmve->dmve_error);
	    if (status != E_DB_OK) 
	    {
		/*
		 * Bug56702: return logfull indication.
		 */
		dmve->dmve_logfull = dmve->dmve_error.err_code;
		break;
	    }

	    /*
	    ** Release log file space allocated for the logforce done
	    ** in the purge tcb call above.
	    */
	    dmve_unreserve_space(dmve, 1);
	}

	return (E_DB_OK);
    }

    uleFormat(&dmve->dmve_error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
	(char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
    SETDBERR(&dmve->dmve_error, 0, E_DM967F_UNDO_DESTROY);

    return (E_DB_ERROR);
}
