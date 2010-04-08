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
**  Name: wmo_profile.c - Web Monitoring Object
**
**  Description:
**	that file is composed of macro server function about profile management
**
**  History:    
**	5-Feb-98 (marol01)
**	    created
**      07-Sep-1998 (fanra01)
**          Enclosed MECOPY_VAR_MACRO within braces.
**          Fixup compiler warnings on unix.
**      25-Sep-98 (fanra01)
**          Fixed typo in submission.
**      25-Jan-1999 (fanra01)
**          Update monitor and timeout parameter positons to 7 & 8
**          respectively.
**          Change all nat and longnat to i4.
**      30-Oct-2001 (fanra01)
**          Bug 106215
**          Changed profile messages with authorization and permission
**          messages.
**      17-Jun-2002 (fanra01)
**          Bug 108078
**          Add a range check on the database user id passed for insert or
**          Update to prevent use of invalid datatbase user id.
**	22-Feb-2004 (hanje04)
**	    BUG 110006
**	    Should NOT be able to create profiles with -ive or zero timeouts.
**/

GSTATUS
WMOTypeAction (
	char* action,
	u_i4  *type);

/*
** Name: WMOProfile() - insert, update, delete, select and retrieve users
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
**      25-Jan-1999 (fanra01)
**          Update monitor and timeout parameter positons to 7 & 8
**          respectively.  Change initialisation of pdbuser, pflag, ptimeout
**          since memory is allocated.  Also add free of memory after use.
**      30-Oct-2001 (fanra01)
**          Replace ambiguous profile error message with authorization
**          and permission messages.
**      17-Jun-2002 (fanra01)
**          Add a range check on the database user id passed for insert or
**          Update.
*/
GSTATUS
WMOProfile (
    ACT_PSESSION    act_session,
    char*           params[],
    bool*           print,
    PTR*            user_info)
{
    GSTATUS     err = GSTAT_OK;
    u_i4        action;
    i4          dbuser = 0;
    i4          flag = 0;
    i4          timeout = 0;
    /*
    ** parameters returned from get or retrieve have memory allocated
    */
    u_i4        *pdbuser = NULL;
    i4          *pflag = NULL;
    i4          *ptimeout = NULL;

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

        if (action != WSS_ACTION_TYPE_SEL &&
        action != WSS_ACTION_TYPE_RET)
        {
        err = DDSCheckUserFlag (
                    act_session->user_session->userid,
                    WSF_USR_SEC,
                    &profile);

        if (profile == FALSE)
        {
            err = DDFStatusAlloc( E_WS0102_WSS_INSUFFICIENT_PERM );
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
        u_i4     *id = NULL;

        if (*user_info == NULL)
        err = GAlloc((PTR*)&params[1], NUM_MAX_INT_STR, FALSE);

        if (err == GSTAT_OK)
        err = WSSSelectProfile(
                       user_info,
                       &id,
                       &params[2],
                       &pdbuser,
                       &pflag,
                       &ptimeout);


        if (err == GSTAT_OK && *user_info != NULL)
        {
            flag    = *pflag;
            timeout = *ptimeout;
            dbuser  = *pdbuser;
            WSF_CVNA(err, *id, params[1]);
            MEfree((PTR) id);
            MEfree((PTR) pdbuser);
            MEfree((PTR) pflag);
            MEfree((PTR) ptimeout);
        }
        else
        *print = FALSE;
    }
    else
    {
        i4         id;
        switch(action)
        {
        case WSS_ACTION_TYPE_DEL:
        case WSS_ACTION_TYPE_RET:
        G_ASSERT (params[1] == NULL || CVan(params[1], &id) != OK, E_WS0059_WSF_BAD_PARAM);
        if (action == WSS_ACTION_TYPE_DEL)
            err = WSSDeleteProfile (id);
        else
        {
            err = WSSRetrieveProfile (
                          id,
                          &params[2],
                          &pdbuser,
                          &pflag,
                          &ptimeout);

            if (err == GSTAT_OK)
            {
                flag    = *pflag;
                timeout = *ptimeout;
                dbuser  = *pdbuser;
                MEfree((PTR) pdbuser);
                MEfree((PTR) pflag);
                MEfree((PTR) ptimeout);
            }
        }
        break;
        case WSS_ACTION_TYPE_INS:
        case WSS_ACTION_TYPE_UPD:
        {
            u_i4         tmp = 0;
            char*        comment = NULL;

            G_ASSERT (params[2] == NULL || params[2][0] == EOS, E_WS0059_WSF_BAD_PARAM);
            G_ASSERT (params[3] == NULL || CVan(params[3], &dbuser) != OK, E_WS0059_WSF_BAD_PARAM);
            /*
            ** Attempt to get the name of the db user.  Passing a NULL for the
            ** target performs a range check on Id and ensures that id is
            ** in use.
            */
            if ((err = DDSGetDBUserName( dbuser, NULL )) != GSTAT_OK)
            {
                return(err);
            }

            if (CVal( params[8], &timeout ) != OK)
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

            if (params[4] != NULL && STcompare(params[4], WSF_CHECKED) == 0)
            flag += WSF_USR_ADM;

            if (params[5] != NULL && STcompare(params[5], WSF_CHECKED) == 0)
            flag += WSF_USR_SEC;

            if (params[6] != NULL && STcompare(params[6], WSF_CHECKED) == 0)
            flag += WSF_USR_UNIT;

            if (params[7] != NULL && STcompare(params[7], WSF_CHECKED) == 0)
            flag += WSF_USR_MONIT;

            if (action == WSS_ACTION_TYPE_UPD)
            {
            G_ASSERT (params[1] == NULL || CVan(params[1], &id) != OK, E_WS0059_WSF_BAD_PARAM);
            err = WSSUpdateProfile (id, params[2], dbuser, flag, timeout);
            }
            else
            {
            err = WSSCreateProfile (params[2], dbuser, flag, timeout, (u_i4 *)&id);
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
    if (err == GSTAT_OK && 
    (action == WSS_ACTION_TYPE_SEL ||
     action == WSS_ACTION_TYPE_RET))
    {
    u_i4  length = STlength(WSF_CHECKED) + 1;
    err = GAlloc((PTR*)&params[3], NUM_MAX_INT_STR, FALSE);
    if (err == GSTAT_OK)
        WSF_CVNA(err, dbuser, params[3]);

    if (err == GSTAT_OK)
        err = GAlloc((PTR*)&params[8], NUM_MAX_INT_STR, FALSE);

    if (err == GSTAT_OK)
        WSF_CVLA(err, timeout, params[8]);

    if (err == GSTAT_OK)
        err = GAlloc((PTR*)&params[4], length, FALSE);

    if (err == GSTAT_OK && flag & WSF_USR_ADM)
    {
        MECOPY_VAR_MACRO(WSF_CHECKED, length, params[4]);
    }
    else
    {
        params[4][0] = EOS;
    }

    if (err == GSTAT_OK)
        err = GAlloc((PTR*)&params[5], length, FALSE);

    if (err == GSTAT_OK && flag & WSF_USR_SEC)
    {
        MECOPY_VAR_MACRO(WSF_CHECKED, length, params[5]);
    }
    else
    {
        params[5][0] = EOS;
    }

    if (err == GSTAT_OK)
        err = GAlloc((PTR*)&params[6], length, FALSE);

    if (err == GSTAT_OK && flag & WSF_USR_UNIT)
    {
        MECOPY_VAR_MACRO(WSF_CHECKED, length, params[6]);
    }
    else
    {
        params[6][0] = EOS;
    }

    if (err == GSTAT_OK)
        err = GAlloc((PTR*)&params[7], length, FALSE);

    if (err == GSTAT_OK && flag & WSF_USR_MONIT)
    {
        MECOPY_VAR_MACRO(WSF_CHECKED, length, params[7]);
    }
    else
    {
        params[7][0] = EOS;
    }
    }
    return(err);
}	
