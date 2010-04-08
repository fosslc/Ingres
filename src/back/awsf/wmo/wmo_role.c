/******************************************************************************
**
**Copyright (c) 2004 Ingres Corporation
**
******************************************************************************/

#include <wsf.h>
#include <dds.h>
#include <erwsf.h>
#include <servermeth.h>
#include <wssprofile.h>
/*
**
**  Name: wmo_role.c - Web Monitoring Object
**
**  Description:
**	that file is composed of macro server function about role management
**
**  History:    
**	5-Feb-98 (marol01)
**	    created
**      08-Sep-1998 (fanra01)
**          Fixup compiler warnings on unix.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      30-Oct-2001 (fanra01)
**          Bug 106215
**          Changed profile messages with authorization and permission
**          messages.
**      25-Jul-2002 (fanra01)
**          Bug 108382
**          Add validation for role id and user id.
**/

GSTATUS
WMOTypeAction (
	char* action,
	u_i4 *type);

/*
** Name: WMORole() - insert, update, delete, select and retrieve a role
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
**      30-Oct-2001 (fanra01)
**          Replace ambiguous profile error message with authorization
**          and permission messages.
**          Also return the error status in all cases for the C_API client.
**      25-Jul-2002 (fanra01)
**          Check the requested role id is valid for a retrieve/delete.
*/
GSTATUS
WMORole (
	ACT_PSESSION act_session, 
	char* params[],
	bool* print,
	PTR*  user_info)
{
    GSTATUS		err = GSTAT_OK;
    u_i4		action;

    *print = TRUE;
    if (*user_info == NULL)
    {
	G_ASSERT (params[0] == NULL || params[0][0] == EOS, E_WS0059_WSF_BAD_PARAM);

	if (err == GSTAT_OK)
	    err = WMOTypeAction(params[0], &action);

	if (act_session->user_session == NULL)
	{
	    *print = FALSE;
            err = DDFStatusAlloc( E_WS0101_WSS_UNAUTHORIZED_USE );
	    return(err);
	}

	if (act_session->user_session->userid != SYSTEM_ID &&
	    (action == WSS_ACTION_TYPE_INS ||
	     action == WSS_ACTION_TYPE_UPD ||
	     action == WSS_ACTION_TYPE_DEL ||
             (act_session->clientType & C_API)))
	{
	    bool	profile;
	    err = DDSCheckUserFlag (
				    act_session->user_session->userid, 
				    WSF_USR_SEC, 
				    &profile);

	    if (profile == FALSE)
	    {
		err = DDFStatusAlloc ( E_WS0102_WSS_INSUFFICIENT_PERM );
		*print = FALSE;
	    }
	}
    }
    else
	action = WSS_ACTION_TYPE_SEL;
	
    if (err == GSTAT_OK)
	if (action == WSS_ACTION_TYPE_SEL)
	{
	    u_i4	*id = NULL;
	    if (*user_info == NULL)
	    {
		err = DDSOpenRole(user_info);
		if (err == GSTAT_OK)
		    err = GAlloc((PTR*)&params[1], NUM_MAX_INT_STR, FALSE);
	    }

	    if (err == GSTAT_OK)
	    {
		err = DDSGetRole(
				 user_info,
				 &id, 
				 &params[2], 
				 &params[3]);


		if (err != GSTAT_OK || *user_info == NULL)
		{
		    CLEAR_STATUS (DDSCloseDB(user_info));
		    *print = FALSE;
		}
		else
		{
		    WSF_CVNA(err, *id, params[1]);
		    MEfree((PTR)id);
		}
	    }
	}
	else 
	{
	    i4		id;
	    switch(action)
	    {
	    case WSS_ACTION_TYPE_DEL:
	    case WSS_ACTION_TYPE_RET:
		G_ASSERT (params[1] == NULL || CVan(params[1], &id) != OK, E_WS0059_WSF_BAD_PARAM);
                if ((err = DDSGetRoleName( id, NULL )) != GSTAT_OK)
                {
                    return(err);
                }
		if (action == WSS_ACTION_TYPE_DEL)
		{
		    err = WSSProfilesUseRole(id);
		    if (err == GSTAT_OK)
			err = DDSDeleteRole (id);
		}
		else
		    err = DDSRetrieveRole (id, &params[2], &params[3]);
		break;
	    case WSS_ACTION_TYPE_INS:
	    case WSS_ACTION_TYPE_UPD:
		{
		    char*		comment = NULL;

		    G_ASSERT (params[2] == NULL || params[2][0] == EOS, E_WS0059_WSF_BAD_PARAM);

		    if (params[3] != NULL)
			comment = params[3];
		    else
			comment = "";

		    if (action == WSS_ACTION_TYPE_UPD)
		    {
			G_ASSERT (params[1] == NULL || CVan(params[1], &id) != OK, E_WS0059_WSF_BAD_PARAM);
			err = DDSUpdateRole (id, params[2], comment);
		    } 
		    else 
		    {
			err = DDSCreateRole (params[2], comment, (u_i4*)&id);
			if (err == GSTAT_OK)
			{
			    err = GAlloc((PTR*)&params[1], NUM_MAX_INT_STR, FALSE);
			    if (err == GSTAT_OK)
				WSF_CVNA(err, id, params[1]);
			}
		    }
		}
	    break;
	    default: 
		err = DDFStatusAlloc(E_WS0061_WPS_BAD_SVR_ACTION);
	    }
	}
    return(err);
}

/*
** Name: WMOUserRole() - insert, delete, select User/Role association
**
** Description:
**	About select, that function list roles for a given user.
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
**          Also return the error status in all cases for the C_API client.
**      25-Jul-2002 (fanra01)
**          Test the requested user id is valid for a retrieve/delete.
**          Test the requested role id is valid for a retrieve/delete.
*/
GSTATUS
WMOUserRole (
    ACT_PSESSION act_session,
    char* params[],
    bool* print,
    PTR*  user_info)
{
    GSTATUS        err = GSTAT_OK;
    u_i4        action;
    i4        user_id;

    *print = TRUE;

    if (*user_info == NULL)
    {
        G_ASSERT (params[0] == NULL || params[0][0] == EOS, E_WS0059_WSF_BAD_PARAM);

        if (err == GSTAT_OK)
            err = WMOTypeAction(params[0], &action);

        if (act_session->user_session == NULL)
        {
            *print = FALSE;
            err = DDFStatusAlloc( E_WS0101_WSS_UNAUTHORIZED_USE );
            return(err);
        }

        if (act_session->user_session->userid != SYSTEM_ID)
        {
            bool    profile;
            err = DDSCheckUserFlag (
                    act_session->user_session->userid,
                    WSF_USR_SEC,
                    &profile);

            if (profile == FALSE)
            {
                G_ASSERT(action == WSS_ACTION_TYPE_INS ||
                         action == WSS_ACTION_TYPE_UPD ||
                         action == WSS_ACTION_TYPE_DEL ||
                         (act_session->clientType & C_API),
                         E_WS0102_WSS_INSUFFICIENT_PERM);

                *print = FALSE;
                return(GSTAT_OK);
            }
        }
    }
    else
        action = WSS_ACTION_TYPE_SEL;

    G_ASSERT (params[1] == NULL || CVan(params[1], &user_id) != OK, E_WS0059_WSF_BAD_PARAM);
    err = DDSGetUserName( user_id, NULL);
    if (err == GSTAT_OK)
        if (action == WSS_ACTION_TYPE_SEL)
        {
            u_i4        *role_id = NULL;

            if (*user_info == NULL)
            {
                err = DDSOpenUserRole(user_info);
                if (err == GSTAT_OK)
                    err = GAlloc((PTR*)&params[2], NUM_MAX_INT_STR, FALSE);
            }

            if (err == GSTAT_OK)
            {
                err = DDSGetUserRole(
                        user_info,
                        user_id,
                        &role_id,
                        &params[3]);

                if (err != GSTAT_OK || *user_info == NULL)
                {
                    CLEAR_STATUS (DDSCloseUserRole(user_info));
                    *print = FALSE;
                }
                else
                {
                    WSF_CVNA(err, *role_id, params[2]);
                    MEfree((PTR)role_id);
                }
            }
        }
        else
        {
            i4        role_id;

            G_ASSERT (params[2] == NULL || CVan(params[2], &role_id) != OK, E_WS0059_WSF_BAD_PARAM);
            if ((err = DDSGetRoleName( role_id, NULL )) != GSTAT_OK)
            {
                return(err);
            }
            switch(action)
            {
            case WSS_ACTION_TYPE_DEL:
                err = DDSDeleteAssUserRole (user_id, role_id);
                break;
            case WSS_ACTION_TYPE_INS:
                err = DDSCreateAssUserRole (user_id, role_id);
                if (err == GSTAT_OK)
                    err = DDSGetRoleName(role_id, &params[3]);
                break;
            default:
                err = DDFStatusAlloc(E_WS0061_WPS_BAD_SVR_ACTION);
            }
        }
    return(err);
}

/*
** Name: WMOProfileRole() - insert, delete, select User/Role association
**
** Description:
**	About select, that function list roles for a given user.
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
**          Also return the error status in all cases for the C_API client.
*/
GSTATUS
WMOProfileRole (
    ACT_PSESSION act_session,
    char* params[],
    bool* print,
    PTR*  user_info)
{
    GSTATUS        err = GSTAT_OK;
    u_i4        action;
    i4        user_id;

    *print = TRUE;

    if (*user_info == NULL)
    {
    G_ASSERT (params[0] == NULL || params[0][0] == EOS, E_WS0059_WSF_BAD_PARAM);

    if (err == GSTAT_OK)
        err = WMOTypeAction(params[0], &action);

    if (act_session->user_session == NULL)
    {
        *print = FALSE;
        err = DDFStatusAlloc( E_WS0101_WSS_UNAUTHORIZED_USE );
        return(err);
    }

    if (act_session->user_session->userid != SYSTEM_ID)
    {
        bool    profile;
        err = DDSCheckUserFlag (
                    act_session->user_session->userid,
                    WSF_USR_SEC,
                    &profile);

        if (profile == FALSE)
        {
        G_ASSERT(action == WSS_ACTION_TYPE_INS ||
             action == WSS_ACTION_TYPE_UPD ||
             action == WSS_ACTION_TYPE_DEL ||
             (act_session->clientType & C_API),
             E_WS0102_WSS_INSUFFICIENT_PERM);

        *print = FALSE;
        return(GSTAT_OK);
        }
    }
    }
    else
    action = WSS_ACTION_TYPE_SEL;

    G_ASSERT (params[1] == NULL || CVan(params[1], &user_id) != OK, E_WS0059_WSF_BAD_PARAM);

    if (err == GSTAT_OK)
    if (action == WSS_ACTION_TYPE_SEL)
    {
        u_i4        *role_id = NULL;

        if (*user_info == NULL)
        err = GAlloc((PTR*)&params[2], NUM_MAX_INT_STR, FALSE);

        if (err == GSTAT_OK)
        err = WSSSelectProfileRole(
                       user_info,
                       user_id,
                       &role_id,
                       &params[3]);

        if (err != GSTAT_OK || *user_info == NULL)
        *print = FALSE;
        else
        {
        WSF_CVNA(err, *role_id, params[2]);
        MEfree((PTR)role_id);
        }
    }
    else
    {
        u_i4        role_id;

        G_ASSERT (params[2] == NULL || CVan(params[2], (i4*)&role_id) != OK, E_WS0059_WSF_BAD_PARAM);

        switch(action)
        {
        case WSS_ACTION_TYPE_DEL:
        err = WSSDeleteProfileRole (user_id, role_id);
        break;
        case WSS_ACTION_TYPE_INS:
        err = WSSCreateProfileRole (user_id, role_id);
        if (err == GSTAT_OK)
            err = DDSGetRoleName(role_id, &params[3]);
        break;
        default:
        err = DDFStatusAlloc(E_WS0061_WPS_BAD_SVR_ACTION);
        }
    }
    return(err);
}
