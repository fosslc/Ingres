/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <er.h>
#include    <gl.h>
#include    <dl.h>
#include    <sl.h>
#include    <st.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ulf.h>
#include    <ulm.h>
#include    <adf.h>
#include    <cs.h>
#include    <scf.h>

#include    <iifas.h>
#include    <ursm.h>

/**
**
**  Name: URSAPPL.C - URS Application Services
**
**  Description:
**      This file contains the service routines for the Applications
**      defined to the URSM of the Frontier Application Server. Routines
** 	are included to get application information (from the repository),
** 	load the appropriate DLL, start the application, build and add to
** 	the application method's parameter list, call the application's
** 	method(s), release the parameter list, shut down the application,
** 	and free the DLL.
**
**	urs_load_dll		Load the application DLL and get function addresses
**	urs_free_dll		Free the application DLL
** 	urs_shut_appl		Shut down the application
**
**  History:
**      31-Dec-1997 wonst02
**          Original User Request Services Manager.
** 	28-Jan-1998 wonst02
** 	    Added urs_load_dll(), urs_free_dll() for OpenRoad.
** 	21-Mar-1998 wonst02
** 	    Move some functions to the Interface layer.
**	10-Aug-1998 wonst02
**	    Fix some comments.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/*
** Global and static variables
*/
static char *Urs_AS_names[] = {
    "AS_Initiate",
    "AS_ExecuteMethod",
    "AS_Terminate",
    "AS_MEfree",
    NULL};


/*{
** Name: urs_get_appl - Get the requested application by its identifier
**
** Description:
** 		Lookup the appropriate application by its ID number.
**
** Inputs:
**      ursm				The URS Manager Control Block
** 	intfc				The interface control block
**
** Outputs:
** 		
**	Returns:
** 		A ptr to an application block or NULL, if not found.
**
**	Exceptions:
**
**
** Side Effects:
**	    none
**
** History:
**      05-jun-1998 wonst02
**          Original
*/
FAS_APPL *
urs_get_appl(URS_MGR_CB	*ursm,
	     i4		appl_id)
{
	FAS_APPL	*appl;

/*	This should be a hash table lookup */
	for (appl = ursm->ursm_srvr->fas_srvr_appl;
	     appl; 
	     appl = appl->fas_appl_next)
	{
	    if (appl->fas_appl_id == appl_id)
	     	break;
	}

	return appl;
}

/*{
** Name: urs_load_dll	Load the application DLL and get its function addresses
**
** Description:
**      Load the application DLL and find its functions: AS_Initiate,
** 	AS_ExecuteMethod, AS_Terminate, etc.
**
** Inputs:
**      ursm				The URS Manager Control Block
**
** Outputs:
**
**	Returns:
** 	    E_DB_OK				DLL loaded OK
** 	    E_DB_ERROR			Error loading DLL
** 							(ursb_error field has more info)
**
**	Exceptions:
**
** Side Effects:
**	    none
**
** History:
**      27-Jan-1998 wonst02
**          Added original.
** 	21-Mar-1998 wonst02
** 	    Move some functions to the Interface layer.
*/
DB_STATUS
urs_load_dll(URS_MGR_CB	*ursm,
             FAS_APPL	*appl,
			 URSB		*ursb)
{
	CL_ERR_DESC	err ZERO_FILL;
	i4			method;
	char		appl_name[sizeof appl->fas_appl_name + 1];
	char		*name = appl->fas_appl_name.db_name;
	char		*nameend;
	STATUS		s = OK;
	DB_STATUS	status = E_DB_OK;

	/* load the application DLL */
	nameend = STindex(name, ERx(" "), sizeof appl->fas_appl_name.db_name);
	if (nameend == NULL)
		nameend = name + sizeof appl->fas_appl_name.db_name;
	STlcopy(name, appl_name, nameend - name);
    	if ( (s = DLprepare_loc((char *)NULL, appl_name,
			    Urs_AS_names, &appl->fas_appl_loc, 0,
			    &appl->fas_appl_handle, &err)) != OK )
    	{
	    urs_error(E_UR0372_DLL_LOAD, URS_INTERR, 2,
	    	      sizeof(s), &s,
		      STlength(appl->fas_appl_loc.string), 
		      appl->fas_appl_loc.string);
	    SET_URSB_ERROR(ursb, E_UR0100_APPL_STARTUP, E_DB_ERROR)
	    return E_DB_ERROR;
	}

	if (appl->fas_appl_type != FAS_APPL_OPENROAD)
	{
	    return E_DB_OK;
	}

	/*
	** Get the addresses of OpenRoad application entry points
	*/
	for (method = 0; Urs_AS_names[method] != NULL; method++)
	{
    	    if (s = DLbind(appl->fas_appl_handle, Urs_AS_names[method],
			&appl->fas_appl_addr[method], &err) != OK)
    	    {
		/* function isn't there -- cleanup dll */
	    	urs_error(E_UR0373_DLL_BIND, URS_INTERR, 3,
	    	    	  sizeof(s), &s,
			  STlength(Urs_AS_names[method]), Urs_AS_names[method],
			  STlength(appl->fas_appl_loc.string), 
			  appl->fas_appl_loc.string);
		(void)urs_free_dll(ursm, appl, ursb);
	    	SET_URSB_ERROR(ursb, E_UR0100_APPL_STARTUP, E_DB_ERROR)
		return E_DB_ERROR;
    	    }
	}
	return status;
}

/*{
** Name: urs_free_dll	Unload the application DLL
**
** Description:
**      Unload the application DLL.
**
** Inputs:
**      ursm				The URS Manager Control Block
**
** Outputs:
**
**	Returns:
** 	    E_DB_OK				Unloaded DLL OK
** 	    E_DB_WARN			The dll handle is NULL
** 	    E_DB_ERROR			Error (ursb_error field has more info)
**
**	Exceptions:
**
** Side Effects:
**	Sets appl->fas_appl_handle = NULL.
**
** History:
**      28-Jan-1998 wonst02
**          Added original.
*/
DB_STATUS
urs_free_dll(URS_MGR_CB	*ursm,
	     	 FAS_APPL	*appl,
			 URSB		*ursb)
{
	STATUS		s;
	CL_ERR_DESC	err ZERO_FILL;
	DB_STATUS	status = E_DB_OK;

	if (appl->fas_appl_handle == NULL)
	    return E_DB_WARN;

	s = DLunload( appl->fas_appl_handle, &err );
	appl->fas_appl_handle = NULL;
	if (s != OK)
	{
	    status = E_DB_ERROR;
	    SET_URSB_ERROR(ursb, E_UR0105_APPL_SHUTDOWN, status)
	    urs_error(E_UR0371_DLL_UNLOAD, URS_INTERR, 2,
	    		  sizeof(s), &s,
				  sizeof appl->fas_appl_handle, &appl->fas_appl_handle);
	}
	return status;
}

/*{
** Name: urs_shut_appl	Shut down the application
**
** Description:
**      Stop the Application.
**
** Inputs:
**      ursm			The URS Manager Control Block
** 	appl			Address of FAS application block
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
**      31-Dec-1997 wonst02
**          Original User Request Services Manager.
** 	21-Mar-1998 wonst02
** 	    Add the missing Interface layer.
*/
DB_STATUS
urs_shut_appl(URS_MGR_CB	*ursm,
		      FAS_APPL		*appl,
			  URSB			*ursb)
{
	DB_STATUS	status = E_DB_OK;

	status = urs_free_dll(ursm, appl, ursb);
	if (status != E_DB_OK)
	{
	    status = E_DB_ERROR;
	    SET_URSB_ERROR(ursb, E_UR0105_APPL_SHUTDOWN, status)
	    urs_error(E_UR0105_APPL_SHUTDOWN, URS_INTERR, 1,
	    	      sizeof(status), &status);
	}

	return status;
}
