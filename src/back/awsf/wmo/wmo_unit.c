/******************************************************************************
**
**Copyright (c) 2004 Ingres Corporation
**
******************************************************************************/

#include <wcs.h>
#include <wcsdoc.h>
#include <wcsunit.h>
#include <servermeth.h>
#include <dds.h>
#include <wsf.h>
#include <wss.h>
#include <erwsf.h>

/*
**
**  Name: wmo_unit.c - Web Monitoring Object
**
**  Description:
**	that file is composed of macro server function about business unit management
**
**  History:    
**	5-Feb-98 (marol01)
**	    created
**      24-Feb-1999 (fanra01)
**          Update call to WPSGetRelativePath to pass pointer to WCS_LOCATION
**          structure.
**      13-Apr-1999 (fanra01)
**          Correct pointer to WCS_LOCATION.
**      28-Apr-1999 (fanra01)
**          Allow copy out filename and location to be overridden. If only
**          unit is provided the unit location and a temporary filename are
**          used.
**          Allow copy in unit location to be overridden.  If no location is
**          specified the name from the copy out file is used.
**          Add check for existance of the directory path during page creation.
**          If the directory does not exist one is created.
**      04-Feb-2000 (fanra01)
**          Bug 100361
**          Add conversion of unit owner id to unit owner name and return
**          the name to the caller.
**      18-Feb-2000 (chika01)
**          WMOUnitCopy(): Modified the url path so that the file that is 
**          copied from a unit can be opened.
**      28-Apr-2000 (fanra01)
**          Bug 100312
**          Add SSL support.  Add additional scheme parameter to WPSrequest.
**      05-May-2000 (fanra01)
**          Bug 101346
**          Update WMOUnitUser calls to DDSAssignUserRight and
**          DDSDeleteUserRight to provide reference count and return the
**          creation or deletion flag.
**      03-Aug-2000 (fanra01)
**          Bug 100527
**          Make the copied-out file available from HTTP and ICE locations.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      31-Aug-2001 (fanra01)
**          Bug 104028
**          A duplicate filename error from a unit unload into an ice location
**          is ignored because the filename is already registered, possibly
**          from the first unload.
**      30-Oct-2001 (fanra01)
**          Bug 106215
**          Changed profile messages with authorization and permission
**          messages.
**      19-Jun-2002 (fanra01)
**          Bug 108073
**          Update unit copy so that if no output filename is provided the
**          temporary one is used.
**          Note that the temporary file is deleted once the session ends.
**      09-Jul-2002 (fanra01)
**          Bug 102222
**          Add validation check to unit id when associating a role and a
**          business unit.
**      25-Jul-2002 (fanra01)
**          Bug 108382
**          Add validation tests of the unit id prior to use.
**      07-Nov-2002 (fanra01)
**          Bug 102195
**          Test for valid location before continuing with action.
**/

GLOBALREF char* privileged_user;

GSTATUS
WMOTypeAction (
	char* action,
	u_i4 *type);

/*
** Name: WMOUnit() - insert, update, delete, select and retrieve a unit
**
** Description:
**
** Inputs:
**	ACT_PSESSION	: active session
**	char*[]			: list of parameters
**
** Outputs:
**	bool*			: TRUE is there is something to print
**	PTR*			: cursor pointer (if NULL means no more information)
**
** Returns:
**	GSTATUS	:	GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
**      04-Feb-2000 (fanra01)
**          Convert the user id to the user name.  Use the privileged_user
**          string for system id.  Use curuser variable to reduce setup of
**          act_session->user_session->userid.
**          Also changed i4  declarations to i4.
**      30-Oct-2001 (fanra01)
**          Replace ambiguous profile error message with authorization
**          and permission messages.
**      25-Jul-2002 (fanra01)
**          Test the requested unit id is valid for a retrieve/delete.
*/
GSTATUS
WMOUnit( ACT_PSESSION act_session, char* params[], bool* print, PTR* user_info )
{
    GSTATUS err = GSTAT_OK;
    u_i4    action;
    u_i4    owner = 0;
    u_i4    *powner = NULL;
    bool    accessible = TRUE;

    *print = TRUE;
    if (*user_info == NULL)
    {
        G_ASSERT(params[0] == NULL || params[0][0] == EOS,
            E_WS0059_WSF_BAD_PARAM);

        if (act_session->user_session == NULL)
        {
            *print = FALSE;
            err = DDFStatusAlloc( E_WS0101_WSS_UNAUTHORIZED_USE );
            return(err);
        }

        err = WMOTypeAction( params[0], &action );
    }
    else
    {
        action = WSS_ACTION_TYPE_SEL;
    }

    if (err == GSTAT_OK)
    {
        if (action == WSS_ACTION_TYPE_SEL)
        {
            u_i4    *id = NULL;

            if (*user_info == NULL)
            {
                if ((err = WCSOpenUnit( user_info )) == GSTAT_OK)
                {
                    err = GAlloc( (PTR*)&params[1], NUM_MAX_INT_STR, FALSE );
                }
            }
            if (err == GSTAT_OK)
            {
                if (((err = WCSGetUnit( user_info, &id, &params[2], &powner ))
                     != GSTAT_OK) || *user_info == NULL)
                {
                    CLEAR_STATUS (WCSCloseUnit( user_info ));
                    *print = FALSE;
                }
                else
                {
                    err = WSSUnitAccessibility ( act_session->user_session,
                        *id, WSF_READ_RIGHT, &accessible );
                    if (err == GSTAT_OK && accessible == TRUE)
                    {
                        WSF_CVNA( err, *id, params[1] );
                        if(err == GSTAT_OK)
                        {
                            if (*powner != SYSTEM_ID)
                            {
                                err = DDSGetUserName( *powner, &params[3] );
                            }
                            else
                            {
                                params[3] = STalloc( privileged_user );
                            }
                        }
                    }
                    else
                    {
                        *print = FALSE;
                    }
                    MEfree( (PTR) id );
                }
            }
        }
        else
        {
            u_i4    id;
            u_i4    dbuser = 0;
            i4      flag = 0;
            i4      curuser = act_session->user_session->userid;

            switch(action)
            {
                case WSS_ACTION_TYPE_DEL:
                case WSS_ACTION_TYPE_RET:
                    G_ASSERT(params[1] == NULL || CVan(params[1],
                        (i4*)&id) != OK, E_WS0059_WSF_BAD_PARAM);
                    if ((err = WCSvalidunit( id, NULL )) != GSTAT_OK)
                    {
                        return(err);
                    }
                    if (action == WSS_ACTION_TYPE_DEL)
                    {
                        if (curuser != SYSTEM_ID)
                        {
                            err = WCSUnitIsOwner( id, curuser, &accessible );
                        }
                        if (err == GSTAT_OK)
                        {
                            if (accessible == TRUE)
                                err = WCSDeleteUnit( id );
                            else
                                err = DDFStatusAlloc( E_WS0065_WSF_UNAUTH_ACCESS );
                        }
                    }
                    else
                    {
                        err = WSSUnitAccessibility( act_session->user_session,
                            id, WSF_READ_RIGHT, &accessible );
                        if (err == GSTAT_OK && accessible == TRUE)
                        {
                            err = WCSRetrieveUnit( id, &params[2], &powner );
                            if(err == GSTAT_OK)
                            {
                                if (*powner != SYSTEM_ID)
                                {
                                    err = DDSGetUserName( *powner, &params[3] );
                                }
                                else
                                {
                                    params[3] = STalloc( privileged_user );
                                }
                            }
                        }
                    }
                    break;

                case WSS_ACTION_TYPE_INS:
                case WSS_ACTION_TYPE_UPD:
                {
                    u_i4    tmp = 0;
                    char*   comment = NULL;

                    G_ASSERT(params[2] == NULL || params[2][0] == EOS,
                        E_WS0059_WSF_BAD_PARAM);

                    if (action == WSS_ACTION_TYPE_UPD)
                    {
                        G_ASSERT(params[1] == NULL ||
                            CVan(params[1], (i4*)&id) != OK,
                            E_WS0059_WSF_BAD_PARAM);
                        if (curuser != SYSTEM_ID)
                        {
                            err = WCSUnitIsOwner( id, curuser, &accessible );
                        }

                        if (err == GSTAT_OK)
                        {
                            if (accessible == TRUE)
                            {
                                err = WCSUpdateUnit( id, params[2], curuser );
                            }
                            else
                            {
                                err = DDFStatusAlloc( E_WS0065_WSF_UNAUTH_ACCESS );
                            }
                        }
                    }
                    else
                    {
                        if (curuser != SYSTEM_ID)
                        {
                            err = DDSCheckUserFlag( curuser, WSF_USR_UNIT,
                                &accessible );
                        }
                        if (err == GSTAT_OK)
                        {
                            if (accessible == TRUE)
                            {
                                err = WCSCreateUnit(&id, params[2], curuser );
                                if (err == GSTAT_OK)
                                {
                                    err = GAlloc( (PTR*)&params[1],
                                        NUM_MAX_INT_STR, FALSE );
                                    if (err == GSTAT_OK)
                                    {
                                        WSF_CVNA( err, id, params[1] );
                                    }
                                }
                            }
                            else
                                err = DDFStatusAlloc( E_WS0065_WSF_UNAUTH_ACCESS );
                        }
                    }
                }
                break;

                default:
                    err = DDFStatusAlloc( E_WS0061_WPS_BAD_SVR_ACTION );
            }
        }
        if(powner != NULL)
        {
            MEfree( (PTR)powner );
        }
    }
    return(err);
}

/*
** Name: WMOUnitLoc() - insert, delete, select Unit/location associations
**
** Description:
**	Allow to insert or delete a relation between a unit and a location
**	Allow to list unit locations
**
** Inputs:
**	ACT_PSESSION	: active session
**	char*[]			: list of parameters
**
** Outputs:
**	bool*			: TRUE is there is something to print
**	PTR*			: cursor pointer (if NULL means no more information)
**
** Returns:
**	GSTATUS	:	GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
**      30-Oct-2001 (fanra01)
**          Replace ambiguous profile error message with authorization
**          and permission messages.
*/
GSTATUS
WMOUnitLoc (
    ACT_PSESSION act_session,
    char* params[],
    bool* print,
    PTR*  user_info)
{
    GSTATUS        err = GSTAT_OK;
    u_i4        action;
    u_i4        unit;
    u_i4        location;
    bool        accessible = TRUE;

    *print = TRUE;

    G_ASSERT (params[1] == NULL || CVan(params[1], (i4*)&unit) != OK, E_WS0059_WSF_BAD_PARAM);

    if (*user_info == NULL)
    {
        G_ASSERT (params[0] == NULL || params[0][0] == EOS, E_WS0059_WSF_BAD_PARAM);

        if (act_session->user_session == NULL)
        {
            *print = FALSE;
                        err = DDFStatusAlloc( E_WS0101_WSS_UNAUTHORIZED_USE );
            return(err);
        }

        if (err == GSTAT_OK)
            err = WMOTypeAction(params[0], &action);
    }
    else
        action = WSS_ACTION_TYPE_SEL;

    if (err == GSTAT_OK)
        if (action == WSS_ACTION_TYPE_SEL)
        {
            u_i4        *db_id = NULL;

            if (*user_info == NULL)
            {
                err = WCSOpenUnitLoc(user_info);
                if (err == GSTAT_OK)
                    err = GAlloc((PTR*)&params[2], NUM_MAX_INT_STR, FALSE);
            }

            err = WCSGetUnitLoc(
                    user_info,
                    unit,
                    &location);
            if (err != GSTAT_OK || *user_info == NULL)
            {
                CLEAR_STATUS (WCSCloseUnitLoc(user_info));
                *print = FALSE;
            }
            else
            {
                err = WSSUnitAccessibility (act_session->user_session, unit, WSF_READ_RIGHT, &accessible);
                if (err == GSTAT_OK && accessible == TRUE)
                {
                    WSF_CVNA(err, location, params[2]);
                }
                else
                    *print = FALSE;
            }
        }
        else
        {
            G_ASSERT (params[2] == NULL || CVan(params[2],
                (i4*)&location) != OK, E_WS0059_WSF_BAD_PARAM);
            /*
            ** Test location id and return error if not valid
            */
            if ((err = WCSGrabLoc( location , NULL )) != GSTAT_OK)
            {
                return(err);
            }
            if (act_session->user_session->userid != SYSTEM_ID)
                err = WCSUnitIsOwner(unit, act_session->user_session->userid, &accessible);

            if (err == GSTAT_OK)
                if(accessible == FALSE)
                    err = DDFStatusAlloc( E_WS0065_WSF_UNAUTH_ACCESS );
                else
                    switch(action)
                    {
                    case WSS_ACTION_TYPE_DEL:
                        err = WCSUnitLocDel(unit, location);
                        break;
                    case WSS_ACTION_TYPE_INS:
                        err = WCSUnitLocAdd(unit, location);
                        break;
                    default:
                        err = DDFStatusAlloc(E_WS0061_WPS_BAD_SVR_ACTION);
                    }
        }
    if (err == GSTAT_OK && *print == TRUE)
    {
        WCS_LOCATION *loc;
        u_i4 length;

        err = WCSGrabLoc(location, &loc);
        length = STlength(loc->name) + 1;
        if (err == GSTAT_OK)
        {
            err = GAlloc((PTR*)&params[3], length, FALSE);
            if (err == GSTAT_OK)
                MECOPY_VAR_MACRO(loc->name, length, params[3]);
        }
    }
    return(err);
}

/*
** Name: WMOUnitRole() - update and retrieve unit/role associations
**
** Description:
**	Allow to update right given to a role
**	Allow to list every role which has right on a specfic unit
**
** Inputs:
**	ACT_PSESSION	: active session
**	char*[]			: list of parameters
**
** Outputs:
**	bool*			: TRUE is there is something to print
**	PTR*			: cursor pointer (if NULL means no more information)
**
** Returns:
**	GSTATUS	:	GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
**      Enclosed MECOPY_VAR_MACRO within braces.
**      30-Oct-2001 (fanra01)
**          Replace ambiguous profile error message with authorization
**          and permission messages.
**      09-Jul-2002 (fanra01)
**          Add validation check on unit id prior to use.
*/
GSTATUS
WMOUnitRole (
	ACT_PSESSION act_session, 
	char* params[],
	bool* print,
	PTR*  user_info)
{
    GSTATUS		err = GSTAT_OK;
    u_i4		action;
    u_i4		unit_id;
    u_i4		role_id;
    i4		rights;
    char		resource[WSF_STR_RESOURCE];

    *print = TRUE;

    G_ASSERT (params[1] == NULL || CVan(params[1], (i4*)&unit_id) != OK, E_WS0059_WSF_BAD_PARAM);	
    if (( err = WCSvalidunit( unit_id, NULL )) != GSTAT_OK)
    {
        return(err);
    }

    if (*user_info == NULL)
    {
	if (act_session->user_session == NULL)
	{
	    *print = FALSE;
            err = DDFStatusAlloc( E_WS0101_WSS_UNAUTHORIZED_USE );
	    return(err);
	}

	if (act_session->user_session->userid != SYSTEM_ID)
	{
	    bool	owner;
	    err = WCSUnitIsOwner(unit_id, act_session->user_session->userid, &owner);
	    if (err == GSTAT_OK)
		G_ASSERT (owner == FALSE, E_WS0102_WSS_INSUFFICIENT_PERM);
	}

	G_ASSERT (params[0] == NULL || params[0][0] == EOS, E_WS0059_WSF_BAD_PARAM);

	if (err == GSTAT_OK)
	    err = WMOTypeAction(params[0], &action);
    }
    else
	action = WSS_ACTION_TYPE_RET;

    STprintf(resource, "p%d", unit_id);

    if (err == GSTAT_OK)
	if (action == WSS_ACTION_TYPE_RET)
	{
	    if (*user_info == NULL)
	    {
		u_i4 length = STlength (WSF_CHECKED) + 1;
		err = DDSOpenRoleResource(user_info, resource);
		if (err == GSTAT_OK)
		    err = GAlloc((PTR*)&params[2], NUM_MAX_INT_STR, FALSE);
		if (err == GSTAT_OK)
		    err = GAlloc((PTR*)&params[4], length, FALSE);
		if (err == GSTAT_OK)
		    err = GAlloc((PTR*)&params[5], length, FALSE);
		if (err == GSTAT_OK)
		    err = GAlloc((PTR*)&params[6], length, FALSE);
	    }

	    if (err == GSTAT_OK)
	    {
		err = DDSGetRoleResource(
					 user_info,
					 &role_id, 
					 &rights);

		if (err != GSTAT_OK || *user_info == NULL)
		{
		    CLEAR_STATUS (DDSCloseRoleResource(user_info));
		    *print = FALSE;
		}
		else
		{
		    u_i4 length = STlength (WSF_CHECKED) + 1;

		    WSF_CVNA(err, role_id, params[2]);

		    if (err == GSTAT_OK)
			err = DDSGetRoleName(role_id, &params[3]);

		    if (err == GSTAT_OK && CHECK_RIGHT(rights, WSF_EXE_RIGHT))
		    {			
			MECOPY_VAR_MACRO(WSF_CHECKED, length, params[4]);
		    }		  
		    else
			params[4][0] = EOS;

		    if (err == GSTAT_OK && CHECK_RIGHT(rights, WSF_READ_RIGHT))
		    {			
			MECOPY_VAR_MACRO(WSF_CHECKED, length, params[5]);
		    }		    
		    else
			params[5][0] = EOS;

		    if (err == GSTAT_OK && CHECK_RIGHT(rights, WSF_INS_RIGHT))
		    {
			MECOPY_VAR_MACRO(WSF_CHECKED, length, params[6]);
		    }		    
		    else
			params[6][0] = EOS;
		}
	    }
	}
	else
	{
	    G_ASSERT (params[2] == NULL || CVan(params[2], (i4*)&role_id) != OK, E_WS0059_WSF_BAD_PARAM);

	    switch(action)
	    {
	    case WSS_ACTION_TYPE_UPD:
		if (params[4] != NULL && STcompare(params[4], WSF_CHECKED) == 0)
		    err = DDSAssignRoleRight (role_id, resource, WSF_EXE_RIGHT);
		else
		    err = DDSDeleteRoleRight (role_id, resource, WSF_EXE_RIGHT);

		if (params[5] != NULL && STcompare(params[5], WSF_CHECKED) == 0)
		    err = DDSAssignRoleRight (role_id, resource, WSF_READ_RIGHT);
		else
		    err = DDSDeleteRoleRight (role_id, resource, WSF_READ_RIGHT);

		if (params[6] != NULL && STcompare(params[6], WSF_CHECKED) == 0)
		    err = DDSAssignRoleRight (role_id, resource, WSF_INS_RIGHT);
		else
		    err = DDSDeleteRoleRight (role_id, resource, WSF_INS_RIGHT);
		break;
	    default: 
		err = DDFStatusAlloc(E_WS0061_WPS_BAD_SVR_ACTION);
	    }
	}
    return(err);
}

/*
** Name: WMOUnitUser() - update and retrieve unit/user associations
**
** Description:
**	Allow to update right given to a user
**	Allow to list every user which has right on a specfic unit
**
** Inputs:
**	ACT_PSESSION	: active session
**	char*[]			: list of parameters
**
** Outputs:
**	bool*			: TRUE is there is something to print
**	PTR*			: cursor pointer (if NULL means no more information)
**
** Returns:
**	GSTATUS	:	GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
**      05-May-2000 (fanra01)
**      Add reference counts and complete flag to DDSAssignUserRight and
**      DDSDeleteUserRight.
**      30-Oct-2001 (fanra01)
**          Replace ambiguous profile error message with authorization
**          and permission messages.
**      25-Jul-2002 (fanra01)
**          Test the requested unit id is valid.
*/
GSTATUS
WMOUnitUser (
	ACT_PSESSION act_session, 
	char* params[],
	bool* print,
	PTR*  user_info)
{
    GSTATUS		err = GSTAT_OK;
    u_i4		action;
    u_i4		unit_id;
    u_i4		user_id;
    i4		rights;
    char		resource[WSF_STR_RESOURCE];

    *print = TRUE;

    G_ASSERT (params[1] == NULL || CVan(params[1], (i4*)&unit_id) != OK, E_WS0059_WSF_BAD_PARAM);	
    if ((err = WCSvalidunit( unit_id, NULL )) != GSTAT_OK)
    {
        return(err);
    }
    if (*user_info == NULL)
    {
	if (act_session->user_session == NULL)
	{
	    *print = FALSE;
            err = DDFStatusAlloc( E_WS0101_WSS_UNAUTHORIZED_USE );
	    return(err);
	}

	if (act_session->user_session->userid != SYSTEM_ID)
	{
	    bool	owner;
	    err = WCSUnitIsOwner(unit_id, act_session->user_session->userid, &owner);
	    if (err == GSTAT_OK)
		G_ASSERT (owner == FALSE, E_WS0102_WSS_INSUFFICIENT_PERM);
	}

	G_ASSERT (params[0] == NULL || params[0][0] == EOS, E_WS0059_WSF_BAD_PARAM);

	if (err == GSTAT_OK)
	    err = WMOTypeAction(params[0], &action);
    }
    else
	action = WSS_ACTION_TYPE_RET;

    STprintf(resource, "p%d", unit_id);

    if (err == GSTAT_OK)
	if (action == WSS_ACTION_TYPE_RET)
	{
	    if (*user_info == NULL)
	    {
		u_i4 length = STlength (WSF_CHECKED) + 1;
		err = DDSOpenUserResource(user_info, resource);
		if (err == GSTAT_OK)
		    err = GAlloc((PTR*)&params[2], NUM_MAX_INT_STR, FALSE);
		if (err == GSTAT_OK)
		    err = GAlloc((PTR*)&params[4], length, FALSE);
		if (err == GSTAT_OK)
		    err = GAlloc((PTR*)&params[5], length, FALSE);
		if (err == GSTAT_OK)
		    err = GAlloc((PTR*)&params[6], length, FALSE);
	    }

	    if (err == GSTAT_OK)
	    {
		err = DDSGetUserResource(
					 user_info,
					 &user_id, 
					 &rights);

		if (err != GSTAT_OK || *user_info == NULL)
		{
		    CLEAR_STATUS (DDSCloseUserResource(user_info));
		    *print = FALSE;
		}
		else
		{
		    u_i4 length = STlength (WSF_CHECKED) + 1;

		    WSF_CVNA(err, user_id, params[2]);
		    if (err == GSTAT_OK)
			err = DDSGetUserName(user_id, &params[3]);
		    if (err == GSTAT_OK && CHECK_RIGHT(rights, WSF_EXE_RIGHT))
		    {			
			MECOPY_VAR_MACRO(WSF_CHECKED, length, params[4]);
		    }		    
		    else
			params[4][0] = EOS;
		    if (err == GSTAT_OK && CHECK_RIGHT(rights, WSF_READ_RIGHT))
		    {			
			MECOPY_VAR_MACRO(WSF_CHECKED, length, params[5]);
		    }		    
		    else
			params[5][0] = EOS;
		    if (err == GSTAT_OK && CHECK_RIGHT(rights, WSF_INS_RIGHT))
		    {			
			MECOPY_VAR_MACRO(WSF_CHECKED, length, params[6]);
		    }		    
		    else
			params[6][0] = EOS;
		}
	    }
	}
	else
	{
            bool created;
            bool deleted;
	    G_ASSERT (params[2] == NULL || CVan(params[2], (i4*)&user_id) != OK, E_WS0059_WSF_BAD_PARAM);

	    switch(action)
	    {
	    case WSS_ACTION_TYPE_UPD:
		if (params[4] != NULL && STcompare(params[4], WSF_CHECKED) == 0)
                {
                    created = FALSE;
		    err = DDSAssignUserRight( user_id, resource,
                        WSF_EXE_RIGHT, 0, &created);
                }
		else
                {
                    deleted = FALSE;
		    err = DDSDeleteUserRight( user_id, resource,
                        WSF_EXE_RIGHT, 0, &deleted);
                }
		if (params[5] != NULL && STcompare(params[5], WSF_CHECKED) == 0)
                {
                    created = FALSE;
		    err = DDSAssignUserRight( user_id, resource,
                        WSF_READ_RIGHT, 0, &created);
                }
		else
                {
                    deleted = FALSE;
		    err = DDSDeleteUserRight( user_id, resource,
                        WSF_READ_RIGHT, 0, &deleted);
                }
		if (params[6] != NULL && STcompare(params[6], WSF_CHECKED) == 0)
                {
                    created = FALSE;
		    err = DDSAssignUserRight( user_id, resource,
                        WSF_INS_RIGHT, 0, &created);
                }
		else
                {
                    deleted = FALSE;
		    err = DDSDeleteUserRight( user_id, resource,
                        WSF_INS_RIGHT, 0, &created);
                }
		break;
	    default: 
		err = DDFStatusAlloc(E_WS0061_WPS_BAD_SVR_ACTION);
	    }
	}
    return(err);
}

/*
** Name: UnitCopyForADocOUT() - Put a document in to the file
**
** Description:
**
** Inputs:
**
** Outputs:
**
** Returns:
**	GSTATUS	:	GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
*/
GSTATUS
UnitCopyForADocOUT (
	WCS_PFILE	output,
	u_i4			id,
	u_i4			unit_id,
	char*			prefix,
	char*			suffix,
	i4		flag,
	u_i4			ext_loc,
	char*			ext_file,
	char*			ext_suffix)
{
    GSTATUS	err = GSTAT_OK;
    WCS_FILE *file = NULL;
    bool		 status;
    char*		 loc_name = NULL;
    i4	 file_size;
    i4  written;
    char		 tmp[WSF_READING_BLOCK];

    err = WCSRequest(
		     0,
		     NULL, 
		     unit_id, 
		     prefix, 
		     suffix, 
		     &file, 
		     &status);

    if (err == GSTAT_OK)
    {
	if (status == WCS_ACT_LOAD)
	{
	    if ((flag & WCS_EXTERNAL) == FALSE)
		err = WCSExtractDocument (file);
																
	    if (err == GSTAT_OK)
		CLEAR_STATUS(WCSLoaded(file, TRUE))
		    else
			CLEAR_STATUS(WCSLoaded(file, FALSE))
		    }

	if (err == GSTAT_OK)
	{
	    if ((flag & WCS_EXTERNAL) != FALSE)
		err = WCSRetrieveLocation(
					  ext_loc, 
					  &loc_name,
					  NULL, NULL, NULL, NULL);
						
	    if (err == GSTAT_OK)
		err = WCSLength (file, &file_size);

	    if (err == GSTAT_OK)
	    {
		if ((flag & WCS_EXTERNAL) == FALSE)
		    STprintf(tmp, "%2d%s%2d%s%*d000000%*d", 
			     STlength(prefix), prefix,
			     STlength(suffix), suffix,
			     NUM_MAX_INT_STR, flag,
			     NUM_MAX_INT_STR, file_size);
		else
		    STprintf(tmp, "%2d%s%2d%s%*d%2d%s%2d%s%2d%s%*d",
			     STlength(prefix), prefix,
			     STlength(suffix), suffix,
			     NUM_MAX_INT_STR, flag,
			     STlength(loc_name), loc_name,
			     STlength(ext_file), ext_file,
			     STlength(ext_suffix), ext_suffix,
			     NUM_MAX_INT_STR, file_size);
		err = WCSWrite(
			       STlength(tmp), 
			       tmp, 
			       &written, 
			       output);
	    }
	    if (err == GSTAT_OK && file_size > 0)
	    {
		err = WCSOpen (file, "r", SI_BIN, 0);
		if (err == GSTAT_OK)
		{
		    i4 read;
		    do 
		    {
			err = WCSRead(file, WSF_READING_BLOCK, &read, tmp);
			if (err == GSTAT_OK &&
			    read > 0)
			    err = WCSWrite(read, tmp, &written, output);
		    }
		    while (err == GSTAT_OK && read > 0);
		    CLEAR_STATUS( WCSClose(file) );
		}
	    }
	}
	CLEAR_STATUS(WCSRelease(&file));
    }
    MEfree((PTR)loc_name);
    return(err);
}

/*
** Name: ReadItem() - Get an item (length + string) from the file
**
** Description:
**
** Inputs:
**
** Outputs:
**
** Returns:
**	GSTATUS	:	GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
**	    change WCS file functions by SI file functions for input file
*/

GSTATUS
ReadItem (
	FILE *input,
	char*	*item,
	i4 *read)
{
	GSTATUS	err = GSTAT_OK;
	char	str[3];
	u_i4	length;
	STATUS status;

	status = SIread(input,2,read,str); 
	switch (status)
	{
		case OK:
			str[*read] = EOS;
			if (CVan(str, (i4*)&length) == OK && length > 0)
			{
				err = GAlloc((PTR*)item, length+1, FALSE);
				if (err == GSTAT_OK)
				{
					if (SIread(input,
										 length,
										 read,
										 *item) != OK)
						err = DDFStatusAlloc(E_WS0067_WCS_CANNOT_READ_FILE);
					else
						(*item)[*read] = EOS;
				}
			}
			else
			{
				err = GAlloc((PTR*)item, 1, FALSE);
				if (err == GSTAT_OK)
					(*item)[0] = EOS;
			}
			break;
		case ENDFILE:
			*read = 0;
			break;
		default:
			err = DDFStatusAlloc(E_WS0067_WCS_CANNOT_READ_FILE);
	}
return(err);
}

/*
** Name: UnitCopyCreateFile() - Copy in and out of a business unit
**
** Description:
**
** Inputs:
**	ACT_PSESSION	: active session
**	char*[]			: list of parameters
**
** Outputs:
**	bool*			: TRUE is there is something to print
**	PTR*			: cursor pointer (if NULL means no more information)
**
** Returns:
**	GSTATUS	:	GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
**      28-Apr-1999 (fanra01)
**          Add check of path as unit files may have subdirectories added.
**          Create the directory if it does not already exist.
*/
GSTATUS
UnitCopyCreateFile  (
    FILE        *input,
    WCS_PFILE   file,
    i4     file_size)
{
    GSTATUS err = GSTAT_OK;
    i4 read;
    STATUS  status;
    char    dev[MAX_LOC];
    char    path[MAX_LOC];
    char    fprefix[MAX_LOC];
    char    fsuffix[MAX_LOC];
    char    version[MAX_LOC];

    /*
    ** Check the existance of the path for the file.  As subdirectories
    ** may be used in the phyical location.
    */
    status = LOdetail( &file->loc, dev, path, fprefix, fsuffix, version);
    if (status == OK)
    {
        LOCATION    pathloc;
        char        pathstr[MAX_LOC+1];

        pathstr[0] = EOS;
        LOfroms( PATH, pathstr, &pathloc );
        LOcompose( dev, path, NULL, NULL, NULL, &pathloc );
        if ((status = LOexist( &pathloc )) != OK)
        {
            if ((status = LOcreate( &pathloc )) != OK)
            {
                err = DDFStatusAlloc(status);
            }
        }
    }
    if ((err == GSTAT_OK) &&
        ((err = WCSOpen (file, "w", SI_BIN, 0)) == GSTAT_OK))
    {
        char tmp[WSF_READING_BLOCK];
        while (err == GSTAT_OK && file_size > 0)
        {
            read = (file_size > WSF_READING_BLOCK) ?
                                WSF_READING_BLOCK : file_size;
            status = SIread(input, read, &read, tmp);
            switch (status)
            {
                case OK:
                    file_size -= read;
                    err = WCSWrite(read, tmp, &read, file);
                    break;
                case ENDFILE:
                    err = DDFStatusAlloc(E_WS0086_WSF_COPY_FORMAT);
                    break;
                default:
                    err = DDFStatusAlloc(E_WS0067_WCS_CANNOT_READ_FILE);
            }
        }
        CLEAR_STATUS( WCSClose(file) );
    }
    return(err);
}

/*
** Name: WMOUnitCopy() - Copy in and out of a business unit
**
** Description:
**
** Inputs:
**	ACT_PSESSION	: active session
**	char*[]			: list of parameters
**
** Outputs:
**	bool*			: TRUE is there is something to print
**	PTR*			: cursor pointer (if NULL means no more information)
**
** Returns:
**	GSTATUS	:	GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
**	8-Sep-98 (marol01)
**	    change the file path receive to copy into is the physical one.
**			So, delete WCSDisptachName.
**	8-Sep-98 (marol01)
**	    change WCS file functions by SI file functions for input file
**      24-Feb-1999 (fanra01)
**          Update call to WPSGetRelativePath to pass pointer to WCS_LOCATION
**          structure.
**      13-Apr-1999 (fanra01)
**          Correct pointer to WCS_LOCATION.
**      27-Apr-1999 (fanra01)
**          Add ability to set the output filename and a public output
**          location.
**          If none is specified a temporary file in the unit location is
**          still used.
**          Modified the returned filespec to reflect the type of location.
**      18-Feb-2000 (chika01)
**          Modified the url path so that the file that is copied from a 
**          unit can be opened.  Also, copy the file into memory space so 
**          that it is available to download.
**      28-Apr-2000 (fanra01)
**          Add additional scheme parameter to WPSrequest.
**      03-Aug-2000 (fanra01)
**          Change copy out of a business unit to accept filename only if
**          supplied via the C API.  Also attempt to get the specfied location
**          from the list of ICE locations for the unit then from the list
**          of public locations for the unit.
**          Use an ice extension if no extension is supplied as part of the
**          filename.
**          If the target location for the extracted file is an ICE location
**          register the file for access via the web.
**      31-Aug-2001 (fanra01)
**          When a business unit is unloaded into an ice location the file
**          is registered temporarily for the purpose of downloading.
**          If the unload is run more than once the file already exists and
**          a duplicate entry error is returned.
**      30-Oct-2001 (fanra01)
**          Replace ambiguous profile error message with authorization
**          and permission messages.
**      19-Jun-2002 (fanra01)
**          Allow use of the generated temporary filename if one isn't
**          specified.
*/
GSTATUS
WMOUnitCopy  (
    ACT_PSESSION act_session,
    char* params[],
    bool* print,
    PTR*  user_info)
{
    GSTATUS err = GSTAT_OK;
    bool    accessible = TRUE;
    u_i4    unit_id;
    char    buffer[WSF_READING_BLOCK];
    bool    status;

    *print = FALSE;
    if (*user_info == NULL)
    {
        G_ASSERT (params[0] == NULL || params[0][0] == EOS,
            E_WS0059_WSF_BAD_PARAM);

        if (act_session->user_session == NULL)
        {
            *print = FALSE;
            err = DDFStatusAlloc( E_WS0101_WSS_UNAUTHORIZED_USE );
            return(err);
        }

        G_ASSERT (params[1] == NULL || CVan(params[1], (i4*)&unit_id) != OK,
            E_WS0059_WSF_BAD_PARAM);

        if (act_session->user_session->userid != SYSTEM_ID)
            err = WCSUnitIsOwner(unit_id, act_session->user_session->userid,
                &accessible);

        if (err == GSTAT_OK)
            G_ASSERT(accessible == FALSE, E_WS0065_WSF_UNAUTH_ACCESS );
    }

    if (err == GSTAT_OK)
    {
        u_i4        *pid = NULL, *punit = NULL, *pext_loc = NULL;
        u_i4        *pflag = NULL;
        char        *pname = NULL, *psuffix = NULL;
        char        *pext_file = NULL, *pext_suffix = NULL;
        char        *loc_name = NULL;

        if (STcompare(params[0], WSF_SVR_ACT_OUT) == 0)
        {
            WPS_PFILE   output = NULL;
            WCS_FILE_INFO*   info = NULL;
            u_i4        *powner = NULL;
            i4          written;
            char*       outloc = NULL;
            char*       outname = NULL;
            char*       outext  = NULL;
            i4          outunit = unit_id;

            if (params[2] != NULL && *params[2] != EOS)
            {
                /*
                ** if invoked from C API separate into filename and extenstion
                */
                if (act_session->clientType & C_API)
                {
                    outname = STalloc( params[2] );
                    if ((outext = STrindex( outname, ".", 0)) != NULL)
                    {
                        *outext = EOS;
                        CMnext( outext );
                    }
                }
            }
            if (params[3] != NULL && *params[3] != EOS)
            {
                WCS_LOCATION    *loc = NULL;

                outloc = params[3];

                /*
                ** Check ICE locations of the unit for specified targret
                */
                err = WCSUnitLocFindFromName(WCS_II_LOCATION, outloc, unit_id,
                    &loc);
                if(err != GSTAT_OK)
                {
                    outunit = SYSTEM_ID;
                    /*
                    ** Not an ICE location check public locations for the
                    ** unit
                    */
                    err = WCSUnitLocFindFromName(WCS_HTTP_LOCATION, outloc,
                        outunit, &loc);
                }
            }

            if ((err = WPSRequest(
                act_session->user_session->name,
                act_session->scheme,
                act_session->host,
                act_session->port,
                outloc,
                outunit,
                outname,
                (outext == NULL) ? WSF_ICE_SUFFIX : outext,
                &output,
                &status)) == GSTAT_OK)
            {
                info = output->file->info;
                /*
                ** if request from HTML page and no name specified
                ** setup filename and extension from temporary
                */
                if ((act_session->clientType & HTML_API) &&
                    (outname == NULL))
                {
                    outname = STalloc( info->name );
                    if ((outext = STrindex( outname, ".", 0)) != NULL)
                    {
                        *outext = EOS;
                        CMnext( outext );
                    }
                }
                CLEAR_STATUS(WCSLoaded(output->file, TRUE));
            }
            if (err == GSTAT_OK)
            {
                err = WCSOpen(output->file, "w", SI_BIN, 0);

                if (err == GSTAT_OK)
                {
                    err = WCSWrite(
                           STlength(COPY_BU_STRING),
                           COPY_BU_STRING,
                           &written,
                           output->file);

                    if (err == GSTAT_OK)
                    {
                        PTR browser = NULL;
                        err = WCSOpenDocument(&browser);
                        if (err == GSTAT_OK)
                        {
                            do
                            {
                                err = WCSGetDocument(&browser,
                                    &pid,
                                    &punit,
                                    &pname,
                                    &psuffix,
                                    &pflag,
                                    NULL,
                                    &pext_loc,
                                    &pext_file,
                                    &pext_suffix);
                            if ((err == GSTAT_OK) && (browser != NULL) &&
                                (*punit == unit_id) &&
                                ((*pflag & WSF_TEMP) == 0))
                                err = UnitCopyForADocOUT(
                                    output->file,
                                    *pid,
                                    unit_id,
                                    pname,
                                    psuffix,
                                    *pflag,
                                    *pext_loc,
                                    pext_file,
                                    pext_suffix);
                            }
                            while (err == GSTAT_OK && browser != NULL);
                            CLEAR_STATUS (WCSCloseDocument(&browser));
                        }
                    }
                    if (err == GSTAT_OK)
                    {
                        char* path;
                        err = WPSGetRelativePath(info->location, &path);
                        if (err == GSTAT_OK)
                        {
                            i4 pathlen;     /* path length + dir separator */
                            i4 filelen;     /* filename length + EOS */

                            if (info->location->type & WCS_HTTP_LOCATION)
                            {
                                pathlen=STlength( path ) + 1;
                                filelen=STlength( info->name )+1;
                            }
                            else
                            {
                                pathlen = 0;
                                filelen=STlength( info->name )+1;
                            }

                            err = GAlloc((PTR*)&params[2], pathlen + filelen,
                                FALSE);
                            if (err == GSTAT_OK)
                            {
                                if (info->location->type & WCS_HTTP_LOCATION)
                                {
                                    STprintf( params[2], "%s%c%s", path,
                                        CHAR_URL_SEPARATOR,
                                        output->file->info->name);
                                }
                                else
                                {
                                    STcopy( info->name, params[2] );
                                    /*
                                    ** if outname is null here then no name
                                    ** was supplied.  Use the generated one
                                    */
                                    if (outname == NULL)
                                    {
                                        outname = STalloc( info->name );
                                    }
                                }
                            }
                        				
                            /*
                            ** If ICE location register the generated file
                            ** as a temporary document of the unit
                            */
                            if ((err == GSTAT_OK) &&
                                ((info->location->type & WCS_HTTP_LOCATION) !=
                                    WCS_HTTP_LOCATION))
                            {   
                                u_i4 pid;                            
                                err = WCSPerformDocumentId(&pid);

                                if (err == GSTAT_OK)
                                {
                                    err = WCSDocAdd(
                                        pid,
                                        unit_id,
                                        outname,
                                        (outext == NULL) ? WSF_ICE_SUFFIX : outext,
                                        WCS_EXTERNAL | WSF_PAGE | WSF_TEMP,
                                        act_session->user_session->userid,
                                        info->location->id,
                                        outname,
                                        (outext == NULL) ? WSF_ICE_SUFFIX : outext);
                                    if ((err != GSTAT_OK) &&
                                        (err->number == E_DF0002_HASH_ITEM_ALR_EXIST))
                                    {
                                        /*
                                        ** The filename is already registered.
                                        */
                                        DDFStatusFree( TRC_INFORMATION, &err );
                                    }
                                }
                            }
                        
                        }
                    }
                    CLEAR_STATUS(WCSClose(output->file));
                }
                /*
                ** If the output file is a temporary file it will be removed
                ** when the session is disconnected or after a time out
                ** if the session times out.
                ** A named file will persist.
                */
		CLEAR_STATUS(WPSRelease(&output));
                MEfree((PTR)powner);
                MEfree( outname );
            }
        }
        else if (STcompare(params[0], WSF_SVR_ACT_IN) == 0)
        {
            WCS_PFILE   input = NULL;
            char*       location = NULL;
            i4          read;
            i4          flag;
            i4          file_size;

            G_ASSERT (params[2] == NULL || params[2][0] == EOS, E_WS0059_WSF_BAD_PARAM);

            if (err == GSTAT_OK)
            {
                WCS_PFILE     file = NULL;
                WCS_LOCATION    *location = NULL;
                FILE *fp = NULL;
                LOCATION loc;
                STATUS clstatus;

                LOfroms(PATH & FILENAME, params[2], &loc);
                if (SIfopen (&loc, "r", SI_BIN, 0, &fp) != OK)
                    err = DDFStatusAlloc(E_WS0009_WPS_CANNOT_OPEN_FILE);
                else
                {
                    clstatus = SIread(fp,
                            STlength(COPY_BU_STRING),
                            &read,
                            buffer);
                    switch (clstatus)
                    {
                        case OK:
                            buffer[read] = EOS;
                            if (STcompare(buffer, COPY_BU_STRING) != 0)
                                err = DDFStatusAlloc(E_WS0086_WSF_COPY_FORMAT);
                            break;
                        case ENDFILE:
                            err = DDFStatusAlloc(E_WS0086_WSF_COPY_FORMAT);
                            break;
                        default:
                            err = DDFStatusAlloc(E_WS0067_WCS_CANNOT_READ_FILE);
                    }

                    while (err == GSTAT_OK)
                    {
                        err = ReadItem(fp, &pname, &read);
                        if (err == GSTAT_OK)
                        {
                            if (read == 0 || pname == NULL || pname[0] == EOS)
                            break;
                            err = ReadItem(fp, &psuffix, &read);
                        }

                        if (err == GSTAT_OK)
                        {
                            clstatus = SIread(fp,
                                    NUM_MAX_INT_STR,
                                    &read,
                                    buffer);
                            switch (clstatus)
                            {
                                case OK:
                                    buffer[read] = EOS;
                                    CVal(buffer, &flag);
                                    break;
                                case ENDFILE:
                                    err = DDFStatusAlloc(E_WS0086_WSF_COPY_FORMAT);
                                    break;
                                default:
                                    err = DDFStatusAlloc(E_WS0067_WCS_CANNOT_READ_FILE);
                            }
                        }

                        if (err == GSTAT_OK)
                            err = ReadItem(fp, &loc_name, &read);
                        if (err == GSTAT_OK)
                            err = ReadItem(fp, &pext_file, &read);
                        if (err == GSTAT_OK)
                            err = ReadItem(fp, &pext_suffix, &read);
                        if (err == GSTAT_OK)
                        {
                            clstatus = SIread(fp,
                                    NUM_MAX_INT_STR,
                                    &read,
                                    buffer);
                            switch (clstatus)
                            {
                                case OK:
                                    buffer[read] = EOS;
                                    CVal(buffer, &file_size);
                                    break;
                                case ENDFILE:
                                    err = DDFStatusAlloc(E_WS0086_WSF_COPY_FORMAT);
                                    break;
                                default:
                                    err = DDFStatusAlloc(E_WS0067_WCS_CANNOT_READ_FILE);
                            }
                        }

                        if (err == GSTAT_OK)
                        {
                            u_i4 id;
                            if ((flag & WCS_EXTERNAL) == FALSE)
                            {
                                err = WCSRequest(
                                     0,
                                     NULL,
                                     unit_id,
                                     NULL,
                                     WSF_ICE_SUFFIX,
                                     &file,
                                     &status);
                                if (err == GSTAT_OK)
                                {
                                    err = UnitCopyCreateFile(fp, file, file_size);
                                    if (err == GSTAT_OK)
                                        err = WCSCreateDocument(
                                            &id,
                                            unit_id,
                                            pname,
                                            psuffix,
                                            flag,
                                            act_session->user_session->userid,
                                            SYSTEM_ID,
                                            file->info->path,
                                            NULL);
                                    CLEAR_STATUS(WCSRelease(&file));
                                }
                            }
                            else
                            {
                                /*
                                ** If the unit location is specified attempt
                                ** to get the unit location from the list
                                ** associated with the unit_id.
                                ** If the unit location is not specified try
                                ** and use the location name read from file.
                                */
                                if (err == GSTAT_OK && location == NULL)
                                {
                                    char*   unitloc;

                                    unitloc = (params[3] == NULL) ? loc_name :
                                        params[3];
                                    err = WCSFindLocation(
                                          0,
                                          unit_id,
                                          unitloc,
                                          psuffix,
                                          &location);
                                }
                                if (err == GSTAT_OK)
                                {
                                    if (location == NULL)
                                    err = DDFStatusAlloc(E_WS0046_LOCATION_UNAVAIL);
                                    else
                                    {
                                        err = WCSCreateDocument(
                                            &id,
                                            unit_id,
                                            pname,
                                            psuffix,
                                            flag,
                                            act_session->user_session->userid,
                                            location->id,
                                            pext_file,
                                            pext_suffix);
                                        if (err == GSTAT_OK)
                                        {
                                            err = WCSRequest(
                                                0,
                                                location->name,
                                                unit_id,
                                                pname,
                                                psuffix,
                                                &file,
                                                &status);
                                        }
                                        if (err == GSTAT_OK)
                                        {
                                            err = UnitCopyCreateFile(fp, file, file_size);
                                            CLEAR_STATUS(WCSRelease(&file));
                                        }
                                    }
                                }
                            }
                        }
                    }
                    SIclose(fp);
                }
            }
        }
        else
            err = DDFStatusAlloc(E_WS0061_WPS_BAD_SVR_ACTION);

        if (err == GSTAT_OK)
            *print = TRUE;

        MEfree((PTR)pid);
        MEfree((PTR)punit);
        MEfree((PTR)pext_loc);
        MEfree((PTR)pflag);
        MEfree((PTR)pname);
        MEfree((PTR)psuffix);
        MEfree((PTR)pext_file);
        MEfree((PTR)pext_suffix);
    }
    return(err);
}
