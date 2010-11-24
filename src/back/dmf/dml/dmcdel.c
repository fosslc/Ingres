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
#include    <dbms.h>
#include    <dmf.h>
#include    <ulf.h>
#include    <dmccb.h>
#include    <dmrcb.h>
#include    <dmscb.h>
#include    <dmtcb.h>
#include    <dmxcb.h>
#include    <lg.h>
#include    <lk.h>
#include    <dm.h>
#include    <dmp.h>
#include    <dm2d.h>
#include    <dml.h>
/**
** Name: DMCDEL.C - Functions used to delete a database from a server.
**
** Description:
**      This file contains the functions necessary to delete a database
**      from a server.  This file defines:
**    
**      dmc_del_db() -  Routine to perform the delete 
**                      database operation.
**
** History:
**      01-sep-1985 (jennifer)
**          Created for Jupiter.      
**	18-nov-1985 (derek)
**	    Completed code for dmc_del_db().
**	8-jul-1992 (walt)
**	    Prototyping DMF.
**
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**		Include <lg.h> to pick up logging system type definitions.
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
**	12-Apr-2008 (kschendel)
**	    Need to include adf.h now.
**	20-Oct-2008 (jonj)
**	    SIR 120874 Modified to use new DB_ERROR based uleFormat 
**	18-Nov-2008 (jonj)
**	    dm2d_? functions converted to DB_ERROR *
**	03-Nov-2010 (jonj) SIR 124685 Prototype Cleanup
**	    Include dml.h to get dmc_del_db() prototype.
**/

/*{
** Name: dmc_del_db - Deletes a database from a server.
**
**  INTERNAL DMF call format:    status = dmc_del_db(&dmc_cb);
**
**  EXTERNAL call format:        status = dmf_call(DMC_DEL_DB,&dmc_cb);
**
** Description:
**    The dmc_del_db function handles the deletion of a database from a
**    DMF server.  This operation causes all database control information
**    about the database identifier by dmc_db_id to be released and the 
**    identifier to become invalid.  This function checks that the data
**    base to be deleted is not currently in use by a session.
**
** Inputs:
**      dmc_cb 
**          .type                           Must be set to DMC_CONTROL_CB.
**          .length                         Must be at least sizeof(DMC_CB).
**          .dmc_op_type                    Must be DMC_DATABASE_OP.
**          .dmc_id                         Must be server identifier.
**          .dmc_db_id                      Must be the database identifier
**                                          returned from the add database
**                                          operation.
**
** Output:
**      dmc_cb 
**          .error.err_code                 One of the following error numbers.
**                                          E_DM0000_OK                
**                                          E_DM000B_BAD_CB_LENGTH
**                                          E_DM000C_BAD_CB_TYPE
**                                          E_DM0010_BAD_DB_ID
**                                          E_DM002D_BAD_SERVER_ID
**                                          E_DM003F_DB_OPEN
**                                          E_DM004A_INTERNAL_ERROR
**                                          E_DM0053_NONEXISTENT_DB
**                                          E_DM0065_USER_INTR
**                                          E_DM0064_USER_ABORT
**					    E_DM0085_ERROR_DELETING_DB
**                                          E_DM0112_RESOURCE_QUOTA_EXCEEDED
**                     
**      Returns:
**          E_DB_OK                         Function completed normally. 
**          E_DB_WARN                       Function completed normally with 
**                                          a termination status which is in 
**                                          dmc_cb.error.err_code.
**          E_DB_ERROR                      Function completed abnormally with 
**                                          a termination status which is in
**                                          dmc_cb.error.err_code.
**          E_DB_FATAL                      Function completed with a fatal
**                                          error which must be handled
**                                          immediately.  The fatal status is in
**                                          dmc_cb.error.err_code.
** History:
**      01-sep-1985 (jennifer)
**          Created for Jupiter.
**	18-nov-1985 (derek)
**	    Completed code for dmc_del_db().
**	25-oct-93 (vijay)
**	    Remove incorrect type cast.
**	06-oct-93 (swm)
**	    Bug #56437
**	    Eliminated i4 cast since dmc->dmc_id is a PTR.
**
*/

DB_STATUS
dmc_del_db(
DMC_CB    *dmc_cb)
{
    DMC_CB		*dmc = dmc_cb;
    i4		error, local_error;
    DB_STATUS		status;
    DMP_DCB		*dcb = (DMP_DCB *)dmc->dmc_db_id;

    CLRDBERR(&dmc->error);

    for (status = E_DB_ERROR;;)
    {
	/*  Check control block parameters. */

	if (dmc->dmc_op_type != DMC_DATABASE_OP)
	{
	    SETDBERR(&dmc->error, 0, E_DM000C_BAD_CB_TYPE);
	    break;
	}
	if (dmc->dmc_id != dmf_svcb->svcb_id)
	{
	    SETDBERR(&dmc->error, 0, E_DM002D_BAD_SERVER_ID);
	    break;
	}
	if (dm0m_check((DM_OBJECT *)dcb, DCB_CB) != E_DB_OK)
	{
	    SETDBERR(&dmc->error, 0, E_DM0010_BAD_DB_ID);
	    break;
	}

	/*  Call dm2d_del_db() to do the dirty work. */

	status = dm2d_del_db(dcb, &dmc->error);
	if (status != E_DB_OK)
	{
	    if (dmc->error.err_code > E_DM_INTERNAL)
	    {
		uleFormat( &dmc->error, 0, NULL, ULE_LOG , NULL, 
		    (char * )NULL, 0L, (i4 *)NULL, &local_error, 0);
		SETDBERR(&dmc->error, 0, E_DM0085_ERROR_DELETING_DB);
	    }
	    break;
	}

	return (E_DB_OK);
    }

    return (status);
}
