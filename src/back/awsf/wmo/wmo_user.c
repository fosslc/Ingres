/******************************************************************************
**
**Copyright (c) 2004 Ingres Corporation
**
******************************************************************************/

#include <wsf.h>
#include <dds.h>
#include <erwsf.h>
#include <servermeth.h>
#include <wss.h>

/*
**
**  Name: wmo_user.c - Web Monitoring Object
**
**  Description:
**	that file is composed of macro server function about user management
**
**  History:    
**	5-Feb-98 (marol01)
**	    created
**      08-Sep-1998 (fanra01)
**          Fixup compiler warnings on unix.
**      08-Jan-1999 (fanra01)
**          Add authentication type parameter for users.  This parameter
**          determines what method to use for password authentication.
**      07-May-1999 (fanra01)
**          Add wss header for function prototypes.
**          Also add check to allow a user to change their own password.
**      09-Mar-2000 (fanra01)
**          Bug 100809
**          Make negative session timeouts invalid.
**    15-Aug-2000 (gupsh01)
**        Added new error message for creating or modifying error on profile.
**        E_WS0100_WSS_BAD_PROFILE_MOD. 
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      30-Oct-2001 (fanra01)
**          Bug 106215
**          Changed profile messages with authorization and permission
**          messages.
**      18-Jun-2002 (fanra01)
**          Bug 106925
**          Prevent currently connected user from removing themselves.
**      14-Nov-2002 (fanra01)
**          Bug 109133
**          Add dbuser id check when creating a web user.
**      04-Feb-2003 (fanra01)
**          Correct assignment of err in testing dbuser id.
**/

GSTATUS
WMOTypeAction (
	char* action,
	u_i4 *type);

/*
** Name: WMOUser() - insert, update, delete, select and retrieve users
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
**      19-Nov-1998 (fanra01)
**          Check case flag and covert user name string to lower if not
**          case sensitive.
**      07-Jan-1999 (fanra01)
**          Add authtype for user functions.
**      09-Mar-2000 (fanra01)
**          Test for negative session timeout and return
**          E_WS0097_WSF_INVALID_TIMEOUT message.
**      15-Aug-2000 (gupsh01)
**          Added new error message for creating or modifying error on profile.
**          E_WS0100_WSS_BAD_PROFILE_MOD. 
**      30-Oct-2001 (fanra01)
**          Replace ambiguous profile error message with authorization
**          and permission messages.
**      18-Jun-2002 (fanra01)
**          Add test to check if the requested deletion is for the current user.
**      14-Nov-2002 (fanra01)
**          Add test to check if the user id is valid before attempting
**          to use it for retrieve or deletes.
**          Add test to check if the database user is valid before attempting
**          to use it for inserts and updates.
**      04-Feb-2003 (fanra01)
**          Removed expression error where an equality was used instead of
**          an assignment.
*/
GSTATUS
WMOUser (
    ACT_PSESSION    act_session,
    char*           params[],
    bool*           print,
    PTR*            user_info)
{
    GSTATUS err = GSTAT_OK;
    u_i4   action;
    i4      dbuser = 0;
    i4 flag = 0;
    i4 timeout = 0;
    i4 authtype = DDS_REPOSITORY_AUTH;
    u_i4   *pdbuser = (u_i4 *)&dbuser;
    /*
    ** parameters returned from get or retrieve have memory allocated
    */
    i4 *pflag = NULL;
    i4 *ptimeout = NULL;
    i4 *pauthtype = NULL;

    *print = TRUE;

    if (*user_info == NULL)
    {
        G_ASSERT (params[0] == NULL || params[0][0] == EOS,
            E_WS0059_WSF_BAD_PARAM);

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

            if (action != WSS_ACTION_TYPE_SEL &&
                action != WSS_ACTION_TYPE_RET)
            {
                u_i4 id = 0;
                if (params[1] != NULL)
                {
                    CVan(params[1], (i4 *)&id);
                }
                if (act_session->user_session->userid != id)
                {
                    err = DDSCheckUserFlag (
                        act_session->user_session->userid,
                        WSF_USR_SEC,
                        &profile);
                }
                else
                {
                    profile = TRUE;
                }

                if (profile == FALSE)
                {
                    err = DDFStatusAlloc( E_WS0100_WSS_BAD_PROFILE_MOD );
                    *print = FALSE;
                }
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
                err = DDSOpenUser(user_info);
                if (err == GSTAT_OK)
                    err = GAlloc((PTR*)&params[1], NUM_MAX_INT_STR, FALSE);
            }

            if (err == GSTAT_OK)
            {
                err = DDSGetUser(
                    user_info,
                    &id,
                    &params[2],
                    &params[3],
                    &pdbuser,
                    &params[6],
                    &pflag,
                    &ptimeout,
                    &pauthtype);

                if (err != GSTAT_OK || *user_info == NULL)
                {
                    CLEAR_STATUS (DDSCloseUser(user_info));
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
                    G_ASSERT (params[1] == NULL || CVan(params[1],
                        (i4*)&id) != OK, E_WS0059_WSF_BAD_PARAM);
                    /*
                    ** Test vailidty of id for update delete operation.
                    */
                    if ((err = DDSGetUserName( id, NULL )) != GSTAT_OK)
                    {
                        return(err);
                    }
                    if (action == WSS_ACTION_TYPE_DEL)
                    {
                        if (act_session->user_session->userid == id)
                        {
                            err=DDFStatusAlloc( E_WS0103_WMO_DROP_CURRENT_USER );
                            *print = FALSE;
                        }
                        else
                        {
                            err = DDSDeleteUser (id);
                        }
                    }
                    else
                    {
                        if (act_session->user_session->userid != SYSTEM_ID &&
                            act_session->user_session->userid != id)
                        {
                            bool    profile;
                            err = DDSCheckUserFlag (
                                act_session->user_session->userid,
                                WSF_USR_SEC,
                                &profile);

                            if (profile == FALSE)
                            {
                                err=DDFStatusAlloc( E_WS0102_WSS_INSUFFICIENT_PERM );
                                *print = FALSE;
                            }
                        }

                        if (err == GSTAT_OK)
                            err = DDSRetrieveUser (
                                id,
                                &params[2],
                                &params[3],
                                &pdbuser,
                                &params[6],
                                &pflag,
                                &ptimeout,
                                &pauthtype);
                    }
                    break;
                case WSS_ACTION_TYPE_INS:
                case WSS_ACTION_TYPE_UPD:
                {
                    u_i4        tmp = 0;
                    char*        comment = NULL;

                    /*
                    ** Check for authentication type to know if passwords
                    ** are required.
                    */
                    if (params[12] != NULL)
                    {
                        authtype = DDS_REPOSITORY_AUTH;
                        if (STbcompare (params[12], 0, STR_OS_AUTH,
                            0, TRUE) == 0)
                        {
                            authtype = DDS_OS_USRPWD_AUTH;
                        }
                    }

                    G_ASSERT (params[2] == NULL || params[2][0] == EOS,
                        E_WS0059_WSF_BAD_PARAM);
                    /*
                    ** Passwords needed if repository authentication.
                    */
                    if (authtype == DDS_REPOSITORY_AUTH)
                    {
                        G_ASSERT (params[3] == NULL || params[3][0] == EOS,
                            E_WS0059_WSF_BAD_PARAM);
                        G_ASSERT (params[4] == NULL || params[4][0] == EOS,
                            E_WS0059_WSF_BAD_PARAM);
                    }
                    G_ASSERT (params[5] == NULL ||
                        CVan(params[5], &dbuser) != OK, E_WS0059_WSF_BAD_PARAM);
                    /*
                    ** Test vailidty of dbuser for update delete operation.
                    */
                    if ((err = DDSGetDBUserName( dbuser, NULL )) != GSTAT_OK)
                    {
                        return(err);
                    }
                    if (CVal( params[10], &timeout ) != OK)
                    {
                        return(DDFStatusAlloc( E_WS0059_WSF_BAD_PARAM ));
                    }
                    else
                    {
                        if (timeout <= 0)
                        {
                            return(DDFStatusAlloc( E_WS0097_WSF_INVALID_TIMEOUT ));
                        }
                    }

                    if (WSSIgnoreUserCase() == TRUE)
                    {
                        CVlower (params[2]);
                    }
                    if (STcompare(params[3], params[4]) != 0)
                        err = DDFStatusAlloc(E_WS0060_WSS_BAD_PASSWORD);
                    else
                    {
                        if (params[6] != NULL)
                            comment = params[6];
                        else
                            comment = "";

                        if (params[7] != NULL && STcompare(params[7],
                            WSF_CHECKED) == 0)
                            flag += WSF_USR_ADM;

                        if (params[8] != NULL && STcompare(params[8],
                            WSF_CHECKED) == 0)
                            flag += WSF_USR_SEC;

                        if (params[9] != NULL && STcompare(params[9],
                            WSF_CHECKED) == 0)
                            flag += WSF_USR_UNIT;

                        if (params[11] != NULL && STcompare(params[11],
                            WSF_CHECKED) == 0)
                            flag += WSF_USR_MONIT;

                        if (action == WSS_ACTION_TYPE_UPD)
                        {
                            G_ASSERT (params[1] == NULL ||
                                CVan(params[1], (i4 *)&id) != OK,
                                E_WS0059_WSF_BAD_PARAM);
                            err = DDSUpdateUser (id, params[2], params[3],
                                dbuser, comment, flag, timeout, authtype);
                        }
                        else
                        {
                            err = DDSCreateUser (params[2], params[3], dbuser,
                                comment, flag, timeout, authtype, &id);
                            if (err == GSTAT_OK)
                            {
                                err = GAlloc((PTR*)&params[1], NUM_MAX_INT_STR,
                                    FALSE);
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
    if (err == GSTAT_OK &&
        (action == WSS_ACTION_TYPE_SEL ||
         action == WSS_ACTION_TYPE_RET))
    {
        u_i4 length = STlength(WSF_CHECKED) + 1;

        if (pflag)
            flag = *pflag;
        if (ptimeout)
            timeout = *ptimeout;
        if (pauthtype)
            authtype = *pauthtype;

        err = GAlloc((PTR*)&params[5], NUM_MAX_INT_STR, FALSE);
        if (err == GSTAT_OK)
            WSF_CVNA(err, dbuser, params[5]);

        if (err == GSTAT_OK)
            err = GAlloc((PTR*)&params[10], NUM_MAX_INT_STR, FALSE);

        if (err == GSTAT_OK)
            WSF_CVLA(err, timeout, params[10]);

        if (params[6] == NULL)
        {
            err = GAlloc((PTR*)&params[6], 1, FALSE);
            params[6][0] = EOS;
        }

        if (err == GSTAT_OK)
            err = GAlloc((PTR*)&params[7], length, FALSE);

        if (err == GSTAT_OK && flag & WSF_USR_ADM)
        {
            MECOPY_VAR_MACRO(WSF_CHECKED, length, params[7]);
        }
        else
        {
            params[7][0] = EOS;
        }

        if (err == GSTAT_OK)
            err = GAlloc((PTR*)&params[8], length, FALSE);

        if (err == GSTAT_OK && flag & WSF_USR_SEC)
        {
            MECOPY_VAR_MACRO(WSF_CHECKED, length, params[8]);
        }
        else
        {
            params[8][0] = EOS;
        }

        if (err == GSTAT_OK)
            err = GAlloc((PTR*)&params[9], length, FALSE);

        if (err == GSTAT_OK && flag & WSF_USR_UNIT)
        {
            MECOPY_VAR_MACRO(WSF_CHECKED, length, params[9]);
        }
        else
        {
            params[9][0] = EOS;
        }

        if (err == GSTAT_OK)
            err = GAlloc((PTR*)&params[11], length, FALSE);

        if (err == GSTAT_OK && flag & WSF_USR_MONIT)
        {
            MECOPY_VAR_MACRO(WSF_CHECKED, length, params[11]);
        }
        else
        {
            params[11][0] = EOS;
        }
        /*
        ** Return text for method.
        */
        if (err == GSTAT_OK)
        {
            char*   p;
            switch (authtype)
            {
                default:
                case DDS_REPOSITORY_AUTH:
                    p = STR_ICE_AUTH;
                    break;
                case DDS_OS_USRPWD_AUTH:
                    p = STR_OS_AUTH;
                    break;
            }
            length = STlength(p) + 1;
            err = GAlloc((PTR*)&params[12], length, FALSE);
            MECOPY_VAR_MACRO(p, length, params[12]);
        }
        if (pflag != NULL)
        {
            MEfree ((PTR)pflag);
        }
        if (ptimeout != NULL)
        {
            MEfree ((PTR)ptimeout);
        }
        if (pauthtype != NULL)
        {
            MEfree ((PTR)pauthtype);
        }
    }
    return(err);
}	
