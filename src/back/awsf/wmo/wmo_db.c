/******************************************************************************
**
** Copyright (c) 1998, 2004 Ingres Corporation
**
******************************************************************************/

#include <wsf.h>
#include <dds.h>
#include <erwsf.h>
#include <servermeth.h>
#include <wssprofile.h>

#ifdef i64_win
#pragma optimize("", off)
#endif

/*
**
**  Name: wmo_db.c - Web Monitoring Object
**
**  Description:
**	that file is composed of macro server function about virtual database management
**
**  History:    
**	5-Feb-98 (marol01)
**	    created
**      10-Sep-1998 (fanra01)
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
**      19-Jul-2002 (fanra01)
**          Bug 108314
**          Add validation check for the associated dbuser id.
**      23-Jul-2002 (fanra01)
**          Bug 108352
**          Add validation check for the database id provided for
**          update/delete.
**	08-jun-2004 (somsa01)
**	    Added NO_OPTIM for i64_win to prevent SEGV when creating
**	    database connections with VDBA under Security for the ICE server.
**/

GSTATUS
WMOTypeAction (
	char* action,
	u_i4 *type);

/*
** Name: WMODB() - insert, update, delete, select and retrieve virtual databases
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
**      19-Jul-2002 (fanra01)
**          Add check of dbuser id before insert/update.
**      23-Jul-2002 (fanra01)
**          Add check for the database id provided for update/delete.
*/
GSTATUS
WMODB  (
    ACT_PSESSION act_session,
    char* params[],
    bool* print,
    PTR*  user_info)
{
    GSTATUS        err = GSTAT_OK;
    u_i4        action;
    u_i4        dbuser = 0;
    i4        flag = 0;
    u_i4        *pdbuser = &dbuser;
    i4        *pflag = &flag;

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
                         action == WSS_ACTION_TYPE_DEL||
                         (act_session->clientType & C_API),
                         E_WS0102_WSS_INSUFFICIENT_PERM);

                *print = FALSE;
                return(GSTAT_OK);
            }
        }
    }
    else
        action = WSS_ACTION_TYPE_SEL;

    if (err == GSTAT_OK)
        if (action == WSS_ACTION_TYPE_SEL)
        {
            u_i4    *id = NULL;

            if (*user_info == NULL)
            {
                err = DDSOpenDB(user_info);
                if (err == GSTAT_OK)
                    err = GAlloc((PTR*)&params[1], NUM_MAX_INT_STR, FALSE);
                if (err == GSTAT_OK)
                    err = GAlloc((PTR*)&params[4], NUM_MAX_INT_STR, FALSE);
            }

            if (err == GSTAT_OK)
            {
                err = DDSGetDB(
                        user_info,
                        &id,
                        &params[2],
                        &params[3],
                        &pdbuser,
                        &pflag,
                        &params[5]);

                if (err != GSTAT_OK || *user_info == NULL)
                {
                    CLEAR_STATUS (DDSCloseDB(user_info));
                    *print = FALSE;
                }
                else
                {
                    WSF_CVNA(err, *id, params[1]);
                    MEfree((PTR) id);
                    if (err == GSTAT_OK)
                        WSF_CVNA(err, dbuser, params[4]);
                }
            }
        }
        else
        {
            u_i4        id;

            switch(action)
            {
            case WSS_ACTION_TYPE_DEL:
            case WSS_ACTION_TYPE_RET:
                G_ASSERT (params[1] == NULL || CVan(params[1], (i4*)&id) != OK, E_WS0059_WSF_BAD_PARAM);
                if ((err = DDSGetDBName( id, NULL )) != GSTAT_OK)
                {
                    return(err);
                }
                if (action == WSS_ACTION_TYPE_DEL)
                {
                    err = WSSProfilesUseDB(id);
                    if (err == GSTAT_OK)
                        err = DDSDeleteDB (id);
                }
                else
                {
                    err = DDSRetrieveDB (id, &params[2], &params[3], &pdbuser, &pflag, &params[5]);
                    if (err == GSTAT_OK)
                    {
                        err = GAlloc((PTR*)&params[4], NUM_MAX_INT_STR, FALSE);
                        if (err == GSTAT_OK)
                            WSF_CVNA(err, dbuser, params[4]);
                    }
                }
                break;
            case WSS_ACTION_TYPE_INS:
            case WSS_ACTION_TYPE_UPD:
                {
                    u_i4        tmp = 0;
                    char*        comment = NULL;

                    G_ASSERT (params[2] == NULL || params[2][0] == EOS, E_WS0059_WSF_BAD_PARAM);
                    G_ASSERT (params[3] == NULL || params[3][0] == EOS, E_WS0059_WSF_BAD_PARAM);
                    G_ASSERT (params[4] == NULL || CVan(params[4], (i4*)&dbuser) != OK, E_WS0059_WSF_BAD_PARAM);
                    if ((err = DDSGetDBUserName( dbuser, NULL )) != GSTAT_OK)
                    {
                        return( err );
                    }

                    if (params[5] != NULL)
                        comment = params[5];
                    else
                        comment = "";

                    if (action == WSS_ACTION_TYPE_UPD)
                    {
                        G_ASSERT (params[1] == NULL || CVan(params[1], (i4*)&id) != OK, E_WS0059_WSF_BAD_PARAM);
                        err = DDSUpdateDB (id, params[2], params[3], dbuser, flag, comment);
                    }
                    else
                    {
                        err = DDSCreateDB (params[2], params[3], dbuser, flag, comment, &id);
                        if (err == GSTAT_OK)
                        {
                            err = GAlloc((PTR*)&params[1], NUM_MAX_INT_STR, FALSE);
                            if (err == GSTAT_OK)
                                WSF_CVNA(err, id, params[1]);
                        }
                        if (err == GSTAT_OK)
                        {
                            err = GAlloc((PTR*)&params[4], NUM_MAX_INT_STR, FALSE);
                            if (err == GSTAT_OK)
                                WSF_CVNA(err, dbuser, params[4]);
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
** Name: WMOUserDB() - insert, delete and select User/database association
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
*/
GSTATUS
WMOUserDB (
    ACT_PSESSION act_session,
    char* params[],
    bool* print,
    PTR*  user_info)
{
    GSTATUS        err = GSTAT_OK;
    u_i4        action;
    u_i4        user_id;

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

    G_ASSERT (params[1] == NULL || CVan(params[1], (i4*)&user_id) != OK, E_WS0059_WSF_BAD_PARAM);

    if (err == GSTAT_OK)
        if (action == WSS_ACTION_TYPE_SEL)
        {
            u_i4        *db_id = NULL;

            if (*user_info == NULL)
            {
                err = DDSOpenUserDB(user_info);
                if (err == GSTAT_OK)
                    err = GAlloc((PTR*)&params[2], NUM_MAX_INT_STR, FALSE);
            }

            if (err == GSTAT_OK)
            {
                err = DDSGetUserDB(
                        user_info,
                        user_id,
                        &db_id,
                        &params[3]);

                if (err != GSTAT_OK || *user_info == NULL)
                {
                    CLEAR_STATUS (DDSCloseUserDB(user_info));
                    *print = FALSE;
                }
                else
                {
                    WSF_CVNA(err, *db_id, params[2]);
                    MEfree((PTR)db_id);
                }
            }
        }
        else
        {
            u_i4        db_id;

            G_ASSERT (params[2] == NULL || CVan(params[2], (i4*)&db_id) != OK, E_WS0059_WSF_BAD_PARAM);

            switch(action)
            {
            case WSS_ACTION_TYPE_DEL:
                err = DDSDeleteAssUserDB (user_id, db_id);
                break;
            case WSS_ACTION_TYPE_INS:
                err = DDSCreateAssUserDB (user_id, db_id);
                if (err == GSTAT_OK)
                    err = DDSGetDBName(db_id, &params[3]);
                break;
            default:
                err = DDFStatusAlloc(E_WS0061_WPS_BAD_SVR_ACTION);
            }
        }
    return(err);
}

/*
** Name: WMOProfileDB() - insert, delete and select Profile/database association
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
*/
GSTATUS
WMOProfileDB (
    ACT_PSESSION act_session,
    char* params[],
    bool* print,
    PTR*  user_info)
{
    GSTATUS        err = GSTAT_OK;
    u_i4        action;
    u_i4        user_id;

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

    G_ASSERT (params[1] == NULL || CVan(params[1], (i4*)&user_id) != OK, E_WS0059_WSF_BAD_PARAM);

    if (err == GSTAT_OK)
        if (action == WSS_ACTION_TYPE_SEL)
        {
            u_i4        *db_id = NULL;

            if (*user_info == NULL)
                err = GAlloc((PTR*)&params[2], NUM_MAX_INT_STR, FALSE);

            if (err == GSTAT_OK)
                err = WSSSelectProfileDB(
                                    user_info,
                                    user_id,
                                    &db_id,
                                    &params[3]);

            if (err != GSTAT_OK || *user_info == NULL)
                *print = FALSE;
            else
            {
                WSF_CVNA(err, *db_id, params[2]);
                MEfree((PTR)db_id);
            }
        }
        else
        {
            u_i4        db_id;

            G_ASSERT (params[2] == NULL || CVan(params[2], (i4*)&db_id) != OK, E_WS0059_WSF_BAD_PARAM);

            switch(action)
            {
            case WSS_ACTION_TYPE_DEL:
                err = WSSDeleteProfileDB (user_id, db_id);
                break;
            case WSS_ACTION_TYPE_INS:
                err = WSSCreateProfileDB (user_id, db_id);
                if (err == GSTAT_OK)
                    err = DDSGetDBName(db_id, &params[3]);
                break;
            default:
                err = DDFStatusAlloc(E_WS0061_WPS_BAD_SVR_ACTION);
            }
        }
    return(err);
}
