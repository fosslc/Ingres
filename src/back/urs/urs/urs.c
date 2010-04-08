/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <lo.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ulf.h>
#include    <ulm.h>
#include    <adf.h>
#include    <cs.h>
#include    <scf.h>

#include    <ursm.h>

/**
**
**  Name: URS.C - User Request Services
**
**  Description:
**      This file contains the call interface to the User Request Services
**	Manager of the Frontier Application Server.
**
**	    URS_CALL - Call User Request Services
**
**  History:
**      03-Nov-1997 wonst02
**          Original User Request Services Manager.
** 	15-Jan-1998 wonst02
** 	    Add URS_EXEC_METHOD, URS_BUILD_PARM.
** 	04-Mar-1998 wonst02
** 	    Remove ursm parameter from urs_add_parm; save last error.
** 	05-Mar-1998 wonst02
** 	    Add URS_GET_PARM.
** 	10-Apr-1998 wonst02
** 	    Add URS_REFRESH_INTFC.
**	11-Aug-1998 (fanra01)
**	    Fixed incomplete last line.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/*
** Global variables
*/

GLOBALREF   URS_MGR_CB		*Urs_mgr_cb;     /* URS Manager control block */

/*
**  Forward and/or External function references.
*/


/*{
** Name: URS_CALL - Call User Request Services
**
** Description:
** 	Call the appropriate User Request Services routine. Handle errors.
**
** Inputs:
** 	ursb				User Request Services Block
**
** Outputs:
**	Returns:
**
**	Exceptions:
**
**
** Side Effects:
**	    none
**
** History:
** 	10-Nov-1997 wonst02
** 	    Original.
** 	14-Jan-1998 wonst02
** 	    Add URS_EXEC_METHOD, URS_BUILD_PARM.
** 	04-Mar-1998 wonst02
** 	    Remove ursm parameter from urs_add_parm; save last error.
*/
DB_STATUS
urs_call(i4		func_code,
	 URSB		*ursb,
	 DB_ERROR	*err)
{
	DB_STATUS	status;

    	status = E_DB_OK;
    	if (err)
	    err->err_code = E_DB_OK;
    	if (ursb == NULL)
	    return E_DB_SEVERE;
	ursb->ursb_error.err_code = E_DB_OK;
	ursb->ursb_error.err_data = E_DB_OK;

    	switch ( func_code )
    	{
    	    case URS_STARTUP :
	    	status = urs_startup(ursb);
	    	if (DB_FAILURE_MACRO(status))
		    (void)urs_shutdown(ursb);
	    	break;

    	    case URS_SHUTDOWN :
	    	status = urs_shutdown(ursb);
	    	break;

    	    case URS_EXEC_METHOD :
	    	status = urs_exec_method(Urs_mgr_cb, ursb);
	    	break;

    	    case URS_BEGIN_PARM :
	    	status = urs_begin_parm(Urs_mgr_cb, ursb);
	    	if (DB_FAILURE_MACRO(status))
	    	    (void)urs_release_parm(Urs_mgr_cb, ursb);
	    	break;

    	    case URS_ADD_PARM :
	    	status = urs_add_parm(ursb);
	    	break;

    	    case URS_FREE_PARM :
	    	status = urs_release_parm(Urs_mgr_cb, ursb);
	    	break;

    	    case URS_GET_TYPE :
	    	status = urs_get_type(ursb);
	    	break;

    	    case URS_REFRESH_INTFC :
	    	status = urs_refresh_intfc(Urs_mgr_cb, ursb);
	    	break;

    	    case URS_GET_METHOD :
	    	status = urs_get_method(Urs_mgr_cb, ursb);
	    	break;

    	    case URS_GET_ARG :
	    	status = urs_get_arg(Urs_mgr_cb, ursb);
	    	break;

    	    default:
	    	status = E_DB_ERROR;
	}

	/*
	** If there was an error, return it to the caller.
	*/
	if ((ursb->ursb_error.err_code != E_DB_OK) && err)
	    STRUCT_ASSIGN_MACRO(ursb->ursb_error, *err);

	return status;
}
