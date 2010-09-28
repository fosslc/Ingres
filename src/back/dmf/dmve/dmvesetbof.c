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
#include    <lg.h>
#include    <lk.h>
#include    <adf.h>
#include    <ulf.h>
#include    <dmf.h>
#include    <dm.h>
#include    <dmp.h>
#include    <dmve.h>
#include    <dmpp.h>
#include    <dm1b.h>
#include    <dm0l.h>

/**
**
**  Name: DMVESETBOF.C - The recovery of a set begin log file operation.
**
**  Description:
**	This file contains the routine for the recovery of a set begin
**	of log file operation.
**
**          dmve_setbof - The recovery of a set begin log file operation.
**
**
**  History:
**      22-may-87 (Derek)    
**          Created for Jupiter.
**	6-Oct-88 (Edhsu)
**	    Fix error mapping bug
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
**      14-jul-2003
**          Added include adf.h
**	14-Nov-2008 (jonj)
**	    SIR 120874 Change function prototypes to pass DB_ERROR *dberr
**	    instead of i4 *err_code, use new form uleFormat.
**	21-Jul-2010 (stial01) (SIR 121123 Long Ids)
**          Remove table name,owner from log records.
**/

/*{
** Name: dmve_setbof - The recovery of a set begin log file operation.
**
** Description:
**	This function re-execute the create table opration using the
**	parameters stored in the log record. The system table changes,
**	the file creation and the file rename operations associated
**	with this create table operation appear later in the journal
**	file, so these opeorations are not performed during recovery.
**
** Inputs:
**	dmve_cb
**	    .dmve_log_rec	    The set begin of log file log record.
**	    .dmve_action	    Should only be DMVE_UNDO.
**	    .dmve_lg_addr	    The log address of the log record.
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
**	22-may-87 (Derek)
**          Created for Jupiter.
*/
DB_STATUS
dmve_setbof(
DMVE_CB		*dmve_cb)
{
    DMVE_CB		*dmve = dmve_cb;
    DM0L_SETBOF		*log_rec = (DM0L_SETBOF *)dmve_cb->dmve_log_rec;
    DB_STATUS		status;
    CL_ERR_DESC		sys_err;
    DB_STATUS		error;

    CLRDBERR(&dmve->dmve_error);
    DMVE_CLEAR_TABINFO_MACRO(dmve);
	
	if (log_rec->sb_header.length != sizeof(DM0L_SETBOF) || 
	    log_rec->sb_header.type != DM0LSETBOF ||
	    dmve->dmve_action != DMVE_UNDO)
	{
	    uleFormat(NULL, E_DM9601_DMVE_BAD_PARAMETER, NULL, ULE_LOG, NULL, 
	    		NULL, 0, NULL, &error, 0);
	    SETDBERR(&dmve->dmve_error, 0, E_DM9618_DMVE_SETBOF);
	    return (E_DB_ERROR);
	}

	/*  Set beginning of log file address. */

	status = LGalter(LG_A_BOF, (PTR)&log_rec->sb_oldbof,
	    sizeof(log_rec->sb_oldbof),
	    &sys_err);
	if (status == OK)
	    return (E_DB_OK);

	uleFormat(NULL, E_DM900B_BAD_LOG_ALTER, &sys_err, ULE_LOG, NULL,
	    		NULL, 0, NULL, &error, 0);
	if (dmve->dmve_error.err_code > E_DM_INTERNAL)
	{
	    uleFormat(&dmve->dmve_error, 0, NULL, ULE_LOG, NULL,
	    		NULL, 0, NULL, &error, 0);
	    SETDBERR(&dmve->dmve_error, 0, E_DM9618_DMVE_SETBOF);
	}
	return (E_DB_ERROR);
}
