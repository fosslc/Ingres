/******************************************************************************
**
**Copyright (c) 2004 Ingres Corporation
**
******************************************************************************/

#include <wsf.h>
#include <dds.h>
#include <wsslogin.h>
#include <erwsf.h>
#include <servermeth.h>

/*
**
**  Name: wmo_dbuser.c - Web Monitoring Object
**
**  Description:
**	that file is composed of macro server function about database user management
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
**      25-Jul-2002 (fanra01)
**          Bug 108382
**          Add check of dbuser id prior to retrieval/deletion.
**/

GSTATUS
WMOTypeAction (
	char* action,
	u_i4 *type);

/*
** Name: WMODBUser() - insert, update, delete, select and retrieve database users
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
**          Add check of provided db user id prior to use.
*/
GSTATUS
WMODBUser (
    ACT_PSESSION act_session,
    char* params[],
    bool* print,
    PTR*  user_info)
{
    GSTATUS        err = GSTAT_OK;
    u_i4        action;

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

    if (err == GSTAT_OK)
        if (action == WSS_ACTION_TYPE_SEL)
        {
            u_i4    *id = NULL;
            if (*user_info == NULL)
            {
                err = DDSOpenDBUser(user_info);
                if (err == GSTAT_OK)
                    err = GAlloc((PTR*)&params[1], NUM_MAX_INT_STR, FALSE);
            }

            if (err == GSTAT_OK)
            {
                err = DDSGetDBUser(
                        user_info,
                        &id,
                        &params[2],
                        &params[3],
                        &params[4],
                        &params[6]);


                if (err != GSTAT_OK || *user_info == NULL)
                {
                    CLEAR_STATUS (DDSCloseDB(user_info));
                    *print = FALSE;
                }
                else
                {
                    WSF_CVNA(err, *id, params[1]);
                    MEfree((PTR) id);
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
                if ((err = DDSGetDBUserName( id, NULL )) != GSTAT_OK)
                {
                    return(err);
                }
                if (action == WSS_ACTION_TYPE_DEL)
                    err = DDSDeleteDBUser (id);
                else
                {
                    err = DDSRetrieveDBUser (id, &params[2], &params[3], &params[4], &params[6]);
                }
                break;
            case WSS_ACTION_TYPE_INS:
            case WSS_ACTION_TYPE_UPD:
                {
                char*        comment = NULL;

                G_ASSERT (params[2] == NULL || params[2][0] == EOS, E_WS0059_WSF_BAD_PARAM);
                G_ASSERT (params[3] == NULL || params[3][0] == EOS, E_WS0059_WSF_BAD_PARAM);
                G_ASSERT (params[4] == NULL, E_WS0059_WSF_BAD_PARAM);
                G_ASSERT (params[5] == NULL, E_WS0059_WSF_BAD_PARAM);

                if (STcompare(params[4], params[5]) != 0)
                    err = DDFStatusAlloc(E_WS0060_WSS_BAD_PASSWORD);
                else
                {
                    if (params[6] != NULL)
                        comment = params[6];
                    else
                        comment = "";

                    if (action == WSS_ACTION_TYPE_UPD)
                    {
                        G_ASSERT (params[1] == NULL || CVan(params[1], (i4*)&id) != OK, E_WS0059_WSF_BAD_PARAM);
                        err = DDSUpdateDBUser (id, params[2], params[3], params[4], comment);
                    }
                    else
                    {
                        err = DDSCreateDBUser (params[2], params[3], params[4], comment, &id);
                        if (err == GSTAT_OK)
                        {
                            err = GAlloc((PTR*)&params[1], NUM_MAX_INT_STR, FALSE);
                            if (err == GSTAT_OK)
                                WSF_CVNA(err, id, params[1]);
                        }
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
