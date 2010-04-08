/******************************************************************************
**
**Copyright (c) 2004 Ingres Corporation
**
******************************************************************************/

#include <wsf.h>
#include <erwsf.h>
#include <wssapp.h>
#include <actsession.h>
#include <dds.h>

/*
**
**  Name: wmo_app.c - Web Monitoring Object
**
**  Description:
**	that file is composed of macro server function about application management
**
**  History:    
**	5-Feb-98 (marol01)
**	    created
**      12-Jan-1999 (fanra01)
**          Cleaned compiler warnings on unix.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      30-Oct-2001 (fanra01)
**          Bug 106215
**          Changed profile messages with authorization and permission
**          messages.
**/

GLOBALREF char* privileged_user;

GSTATUS
WMOTypeAction (
	char* action,
	u_i4 *type);

/*
** Name: WMOApplication() - insert, update, delete, select and retrieve applications
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
**      12-Jan-1999 (fanra01)
**          Cleaned compiler warnings on unix.
**      30-Oct-2001 (fanra01)
**          Replace ambiguous profile error message with authorization
**          and permission messages.
**          Also return the error status in all cases for the C_API client.
*/
GSTATUS
WMOApplication (
    ACT_PSESSION act_session,
    char* params[],
    bool* print,
    PTR*  user_info)
{
    GSTATUS        err = GSTAT_OK;
    u_i4            action;
    bool            accessible = TRUE;

    *print = TRUE;
    if (*user_info == NULL)
    {
        G_ASSERT (params[0] == NULL || params[0][0] == EOS, E_WS0059_WSF_BAD_PARAM);

        if (act_session->user_session == NULL)
        {
            *print = FALSE;
            err = DDFStatusAlloc( E_WS0101_WSS_UNAUTHORIZED_USE );
            return(err);
        }

        err = WMOTypeAction(params[0], &action);

        if (err == GSTAT_OK&& act_session->user_session->userid != SYSTEM_ID)
        {
            bool    profile;
            err = DDSCheckUserFlag (
                    act_session->user_session->userid,
                    WSF_USR_ADM,
                    &profile);

            G_ASSERT(profile == FALSE &&
                     (action == WSS_ACTION_TYPE_INS ||
                      action == WSS_ACTION_TYPE_UPD ||
                      action == WSS_ACTION_TYPE_DEL ||
                      (act_session->clientType & C_API)),
                      E_WS0102_WSS_INSUFFICIENT_PERM);
        }
    }
    else
        action = WSS_ACTION_TYPE_SEL;

    if (err == GSTAT_OK)
        if (action == WSS_ACTION_TYPE_SEL)
        {
            u_i4            *pid = NULL;

            err = WSSAppBrowse(
                        user_info,
                        &pid,
                        &params[2]);

            if (err != GSTAT_OK || *user_info == NULL)
            {
                *print = FALSE;
            }
            else
            {
                err = GAlloc((PTR*)&params[1], NUM_MAX_INT_STR, FALSE);
                if (err == GSTAT_OK)
                    WSF_CVNA(err, *pid, params[1]);
            }
            MEfree((PTR) pid);
        }
        else
        {
            u_i4        id;

            switch(action)
            {
            case WSS_ACTION_TYPE_DEL:
            case WSS_ACTION_TYPE_RET:
                G_ASSERT (params[1] == NULL || CVan(params[1], (i4*)&id) != OK, E_WS0059_WSF_BAD_PARAM);
                if (action == WSS_ACTION_TYPE_DEL)
                    err = WSSAppDelete(id);
                else
                {
                    err = WSSAppRetrieve(id, &params[2]);
                }
                break;
            case WSS_ACTION_TYPE_INS:
            case WSS_ACTION_TYPE_UPD:
                {
                    G_ASSERT (params[2] == NULL || params[2][0] == EOS, E_WS0059_WSF_BAD_PARAM);
                    if (action == WSS_ACTION_TYPE_UPD)
                    {
                        G_ASSERT (params[1] == NULL || CVan(params[1], (i4*)&id) != OK, E_WS0059_WSF_BAD_PARAM);
                        err = WSSAppUpdate(id, params[2]);
                    }
                    else
                    {
                        err = WSSAppCreate(&id, params[2]);
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
