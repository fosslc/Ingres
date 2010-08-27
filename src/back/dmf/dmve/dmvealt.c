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
#include    <dm0c.h>
#include    <dm1b.h>
#include    <dm0l.h>

/**
**
**  Name: DMVEALTER.C - The backout of an alter table operation.
**
**  Description:
**	This file contains the routine for the backout of an alter table
**	operation.  There is no work required for roll-forward as the
**	actual updates to the system catalogs are logged separately.
**
**          dmve_alter - The recovery of an alter table operation.
**
**
**  History:    
**      22-jun-87 (rogerk)
**          Created new for Jupiter.
**	07-july-1992 (jnash)
**	    Add DMF function prototyping 
**	3-oct-1992 (bryanp)
**	    Re-arrange includes to share transaction ID type definition.
**	29-nov-1992 (jnash)
**	    Reduced logging project.  Add CLR handling.
**	14-dec-1992 (rogerk)
**	    Include dm1b.h before dm0l.h for prototype definitions.
**	30-feb-1993 (rmuth)
**	    Prototyping DI, need to include di.h
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
**	15-may-1993 (rmuth)
**	    Readonly table support: dm2t_open the table for 
**	    DM2T_A_OPEN_NOACESS mode so that we can purge both READONLY
**	    and NOREADONLY tables.
**	24-may-1993 (bryanp)
**	    Include <pc.h> before <lk.h> to pick up PID definition.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (rogerk)
**	    Include respective CL headers for all cl calls.
**	23-aug-1993 (rogerk)
**	  - Broke main body of function into dmv_remodify and dmv_unmodify.
**	  - Added rollforward handling.
**	18-oct-1993 (rogerk)
**	    Added dmve_unreserve_space call to release logfile space
**	    reserved for logforces in the purge tcb call.
**	15-apr-1994 (chiku)
**	    Bug56702: return logfull indication.
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
**	25-Nov-2008 (jonj)
**	    SIR 120874: dm0l_? functions converted to DB_ERROR *
**	28-Nov-2008 (jonj)
**	    SIR 120874: dm2t_? functions converted to DB_ERROR *
**	21-Jul-2010 (stial01) (SIR 121123 Long Ids)
**          Remove table name,owner from log records.
**/

static DB_STATUS dmv_realter(
	DMVE_CB         *dmve,
	DM0L_ALTER      *log_rec);


static DB_STATUS dmv_unalter(
	DMVE_CB         *dmve,
	DM0L_ALTER      *log_rec);



/*{
** Name: dmve_alter - UNDO recovery of an alter table opration.
**
** Description:
**	This function purges the TCB for the altered table during recovery of
**	an alter table function.  All changes to the system catalogs to perform
**	the alter are logged and recovered individually.
**
** Inputs:
**	dmve_cb
**	    .dmve_log_rec	    The alter table operation log record.
**	    .dmve_action	    Should only be DMVE_UNDO.
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
**	22-jun-87 (rogerk)
**          Created for Jupiter.
**	6-Oct-88 (Edhsu)
**	    Fix error mapping bug
**	29-nov-1992 (jnash)
**	    Reduced logging project.  Add CLR handling and half-hearted
**	    attempt at partial location recovery.
**	15-mar-1993 (jnash)
**	    Check dmve->dmve_logging to determine if logging required.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	15-may-1993 (rmuth)
**	    Readonly table support: dm2t_open the table for 
**	    DM2T_A_OPEN_NOACESS mode so that we can purge both READONLY
**	    and NOREADONLY tables.
**	23-aug-1993 (rogerk)
**	  - Broke main body of function into dmv_remodify and dmv_unmodify.
*/
DB_STATUS
dmve_alter(
DMVE_CB		*dmve_cb)
{
    DMVE_CB		*dmve = dmve_cb;
    DM0L_ALTER		*log_rec = (DM0L_ALTER *)dmve_cb->dmve_log_rec;
    i4		recovery_action;
    i4		error;
    DB_STATUS		status = E_DB_OK;

    CLRDBERR(&dmve->dmve_error);
    DMVE_CLEAR_TABINFO_MACRO(dmve);
	
    for (;;)
    {
	if (log_rec->dua_header.length > sizeof(DM0L_ALTER) || 
	    log_rec->dua_header.type != DM0LALTER ||
	    dmve->dmve_action == DMVE_REDO)
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
	if (log_rec->dua_header.flags & DM0L_CLR)
	    recovery_action = DMVE_UNDO;

        switch (recovery_action)
	{
	case DMVE_DO:
	    status = dmv_realter(dmve, log_rec);
	    break;

	case DMVE_UNDO:
	    status = dmv_unalter(dmve, log_rec);
	    break;
	}

	if (status != E_DB_OK)
	    break;

	return(E_DB_OK);
    }

    /*
    ** Error handling
    */

    uleFormat(&dmve->dmve_error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
	(char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
    SETDBERR(&dmve->dmve_error, 0, E_DM961B_DMVE_ALTER);

    return(E_DB_ERROR);
}

/*{
** Name: dmv_realter	- REDO Recovery of a table alter operation
**
** Description:
**
**	This routine is used during REDO recovery of an alter of a table.  
**
**	The updates of the alter operation are redon by re-applying the
**	system catalog updates via recovery of their respective log records.
**
**	This routine purges the table TCB.
**
** Inputs:
**	dmve			Recovery context block.
**	log_rec			Alter log record.
**
** Outputs:
**	err_code		The reason for error status.
**
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
**	    Routine created from dmve_alter code.
*/
static DB_STATUS
dmv_realter(
DMVE_CB         *dmve,
DM0L_ALTER 	*log_rec)
{
    LG_LSN		*log_lsn = &log_rec->dua_header.lsn; 
    LG_LSN		lsn;
    i4		dm0l_flags;
    DB_STATUS		status = E_DB_OK;
    i4			*err_code = &dmve->dmve_error.err_code;

    CLRDBERR(&dmve->dmve_error);

    for (;;)
    {
	/*
	** Purge the TCB so it can be re-built in its altered state.
	*/
	status = dm2t_purge_tcb(dmve->dmve_dcb_ptr, &log_rec->dua_tbl_id,
			(DM2T_PURGE | DM2T_NORELUPDAT), dmve->dmve_log_id, 
			dmve->dmve_lk_id, &dmve->dmve_tran_id, 
			dmve->dmve_db_lockmode, &dmve->dmve_error);
	if (status != E_DB_OK)
	    break;

	return(E_DB_OK);
    }

    uleFormat(&dmve->dmve_error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
	(char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
    SETDBERR(&dmve->dmve_error, 0, E_DM9688_REDO_ALTER);

    return(E_DB_ERROR);
}

/*{
** Name: dmv_unalter	- UNDO Recovery of a table alter operation
**
** Description:
**
**	This routine is used during UNDO recovery of an alter of a table.  
**
**	The updates of the alter operation are rolled back by undoing the
**	system catalog updates via recovery of their respective log records.
**
**	This routine purges the table TCB (and notifies other servers of
**	the structure change by bumping the cache lock).
**
** Inputs:
**	dmve			Recovery context block.
**	log_rec			Alter log record.
**
** Outputs:
**	err_code		The reason for error status.
**
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
**	    Routine created from dmve_alter code.
**	18-oct-1993 (rogerk)
**	    Added dmve_unreserve_space call to release logfile space
**	    reserved for logforces in the purge tcb call.
**	15-apr-1994 (chiku)
**	    Bug56702: return logfull indication.
*/
static DB_STATUS
dmv_unalter(
DMVE_CB         *dmve,
DM0L_ALTER 	*log_rec)
{
    LG_LSN		*log_lsn = &log_rec->dua_header.lsn; 
    LG_LSN		lsn;
    i4		dm0l_flags;
    DB_STATUS		status = E_DB_OK;
    i4			local_err;

    CLRDBERR(&dmve->dmve_error);

    for (;;)
    {
	/*
	** Alter was aborted.  Purge the table descriptor in case
	** someone (presumably this transaction) has done a build_tcb 
	** on the table in its altered state.
	*/
	status = dm2t_purge_tcb(dmve->dmve_dcb_ptr, &log_rec->dua_tbl_id,
			(DM2T_PURGE | DM2T_NORELUPDAT), dmve->dmve_log_id, 
			dmve->dmve_lk_id, &dmve->dmve_tran_id, 
			dmve->dmve_db_lockmode, &dmve->dmve_error);
	if (status != E_DB_OK)
	    break;

	/*
	** Write the CLR if necessary.
	*/
	if ((dmve->dmve_logging) &&
	    ((log_rec->dua_header.flags & DM0L_CLR) == 0))
	{
	    dm0l_flags = (log_rec->dua_header.flags | DM0L_CLR);

	    status = dm0l_alter(dmve->dmve_log_id, dm0l_flags, 
				&log_rec->dua_tbl_id, &log_rec->dua_name, 
				&log_rec->dua_owner, log_rec->dua_count,
				log_rec->dua_action, log_lsn, &lsn, 
				&dmve->dmve_error);
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

	return(E_DB_OK);
    }

    uleFormat(&dmve->dmve_error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
	(char *)NULL, (i4)0, (i4 *)NULL, &local_err, 0);

    return(E_DB_ERROR);
}
