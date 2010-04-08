/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <di.h>
#include    <me.h>
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


/*
**
**  Name: DMVECRDB.C - The recovery of a create database operation.
**
**  Description:
**	This file contains the routine for the UNDO recovery of a create 
**	database operation.
**
**          dmve_crdb - The recovery of a create database operation.
**
**
**  History:
**      30-jun-89 (derek)    
**          Created new for Jupiter.
**	07-july-1992 (jnash)
**	    Add DMF function prototyping 
**	3-oct-1992 (bryanp)
**	    Re-arrange includes to share transaction ID type definition.
**	30-nov-1992 (jnash)
**	    Reduced logging project. Add CLR handling.
**	14-dec-1992 (rogerk)
**	    Include dm1b.h before dm0l.h for prototype definitions.
**	15-mar-1993 (jnash & rogerk)
**	    Reduced Logging - Phase IV:
**	    Changed log record format: added database_id, tran_id and LSN
**		in the log record header.  The LSN of a log record is now
**		obtained from the record itself rather than from the dmve
**		control block.
**	    Check dmve->dmve_logging to determine if logging required.
**	    Also, case DI params to (char *) to quiet compiler.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	24-may-1993 (bryanp)
**	    Include <pc.h> before <lk.h> to pick up PID definition.
**	21-jun-1993 (rogerk)
**	    Fix prototype warnings.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (rogerk)
**	    Include respective CL headers for all cl calls.
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
**/


/*
**  Forward Reference to static Functions.
*/

typedef struct
{
	STATUS	    error;
	char	    *path_name;	
	int	    path_length;
	int	    pad;
}   DELETE_INFO;

static STATUS del_di_files(
	DELETE_INFO     *delete_info,
	char            *file_name,
	i4             file_length,
	CL_ERR_DESC      *sys_err );


/*{
** Name: dmve_crdb - UNDO recovery of a create database opration.
**
** Description:
**	This function UNDOes the create database opration using the
**	parameters stored in the log record.
**
** Inputs:
**	dmve_cb
**	    .dmve_log_rec	    The create table operation log record.
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
**	30-jun-89 (derek)
**          Created for Jupiter.
**	22-aug-91 (bonobo)
**	    Fixed an unclear reference to eliminate su4 compiler warning.
**	    "cd_root" is type DM_PATH, and the DM_PATH struct contains
**	    one element, a char array called "name".
**	30-nov-1992 (jnash)
**	    Reduced logging project.  Add CLR handling.
**	15-mar-1993 (jnash)
**	    Check dmve->dmve_logging to determine if logging required.
**	    Also, case DI params to (char *) to quiet compiler.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	21-jun-1993 (rogerk)
**	    Fix prototype warnings.
**	15-apr-1994 (chiku)
**	    Bug56702: return logfull indication.
*/
DB_STATUS
dmve_crdb(
DMVE_CB		*dmve_cb)
{
    DMVE_CB		*dmve = dmve_cb;
    DM0L_CRDB	        *log_rec = (DM0L_CRDB*)dmve_cb->dmve_log_rec;
    LG_LSN		*log_lsn = &log_rec->cd_header.lsn; 
    DM0C_CNF		*config;
    DB_STATUS		status = E_DB_OK;
    DB_STATUS		error;
    DI_IO		di_context;
    DELETE_INFO		delete_info;
    CL_ERR_DESC		sys_err;
    i4		dm0l_flags;
    i4			*err_code = &dmve->dmve_error.err_code;

    CLRDBERR(&dmve->dmve_error);

    for (;;)
    {
	/*
	** Write CLR if necessary
	*/
	if ((dmve->dmve_logging) &&
	    ((log_rec->cd_header.flags & DM0L_CLR) == 0))
	{
	    dm0l_flags = log_rec->cd_header.flags | DM0L_CLR;

	    status = dm0l_crdb(dmve->dmve_log_id, dm0l_flags, 
		log_rec->cd_dbid, &log_rec->cd_dbname, 
		log_rec->cd_access, log_rec->cd_service,
		&log_rec->cd_location, log_rec->cd_l_root,
		&log_rec->cd_root, log_lsn, &dmve->dmve_error);
	    if (status != E_DB_OK)
	    {
		/* XXXX Better error message and continue after logging. */
		TRdisplay(
		    "dmve_crdb: dm0l_crdb error, status: %d, error: %d\n", 
		    status, dmve->dmve_error.err_code);
		/*
		 * Bug56702: return logfull indication.
		 */
		dmve->dmve_logfull = dmve->dmve_error.err_code;
		return(E_DB_ERROR);
	    }
	}

	/*
	**  Prepare to delete files in data directory.
	**  Delete AAAAAAAA.CNF last so that database is semi-legal
	**  until the very end.
	*/

	delete_info.error = OK;
	delete_info.path_name = log_rec->cd_root.name;
	delete_info.path_length = log_rec->cd_l_root;
	status = DIlistfile(&di_context, delete_info.path_name,
	    delete_info.path_length, del_di_files, (PTR) &delete_info,
	    &sys_err);
	if (status == DI_DIRNOTFOUND)
	{
	    /*	Directory doesn't exist anymore. */
	    break;
	}

	/*	Delete configuration file. */

	status = DIdelete(&di_context, (char *)&log_rec->cd_root,
	    log_rec->cd_l_root, "aaaaaaaa.cnf", 12, &sys_err);
	if (status != E_DB_OK)
	{
	    /*	Directory could be empty. */

	    if (status != DI_FILENOTFOUND)
		uleFormat(NULL, E_DM9003_BAD_FILE_DELETE, &sys_err, ULE_LOG, NULL,
		    NULL, 0, NULL, err_code, 4,
		    sizeof(log_rec->cd_dbname), &log_rec->cd_dbname,
		    0, "Configuration_File",
		    log_rec->cd_l_root, &log_rec->cd_root,
		    12, "aaaaaaaa.cnf");
	}

	/*  Delete the directory. */

	status = DIdirdelete(&di_context, 
	    (char *)&log_rec->cd_root, log_rec->cd_l_root,
	    0, 0, &sys_err);
	if (status != OK)
	{
	    uleFormat(NULL, E_DM9003_BAD_FILE_DELETE, &sys_err, ULE_LOG, NULL,
		NULL, 0, NULL, err_code, 4,
		sizeof(log_rec->cd_dbname), &log_rec->cd_dbname,
		0, "",
		log_rec->cd_l_root, &log_rec->cd_root,
		sizeof(log_rec->cd_dbname), &log_rec->cd_dbname);
	}

	break;
    }

    return (E_DB_OK);
}

static STATUS
del_di_files(
DELETE_INFO	*delete_info,
char		*file_name,
i4		file_length,
CL_ERR_DESC	*sys_err)
{
    DI_IO	di_context;
    DB_STATUS	err_code;

    if (file_length == 12 && MEcmp(file_name, "aaaaaaaa.cnf", 12) == 0)
	return (delete_info->error = OK);

    delete_info->error = DIdelete(&di_context,
	delete_info->path_name, delete_info->path_length,
	file_name, file_length, sys_err);
    if (delete_info->error == OK)
	return (OK);
    uleFormat(NULL, E_DM9003_BAD_FILE_DELETE, sys_err, ULE_LOG, NULL,
	    NULL, 0, NULL, &err_code, 4,
	    0, "",
	    0, "",
	    delete_info->path_length, delete_info->path_name,
	    file_length, file_name);
    return (FAIL);
}
