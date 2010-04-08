/******************************************************************************
**
**Copyright (c) 2004 Ingres Corporation
**
******************************************************************************/

#include <wcs.h>
#include <servermeth.h>
#include <dds.h>
#include <wsf.h>
#include <wcslocation.h>
#include <erwsf.h>

/*
LEVEL1_OPTIM = hp2_us5
*/

/*
**
**  Name: wmo_location.c - Web Monitoring Object
**
**  Description:
**	that file is composed of macro server function about location management
**
**  History:    
**	5-Feb-98 (marol01)
**	    created
**      07-Sep-1998 (fanra01)
**          Enclosed MECOPY_VAR_MACRO within braces.
**          Fixup compiler warnings on unix.
**      25-Feb-2000 (chika01)
**          Bug 100605 
**          Updates WMOLocation() to verify directory before 
**          insert or update as an ICE location.
**      25-Jul-2000 (fanra01)
**          Bug 102168
**          Modified previous change to include verification of http locations
**          during creation.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      23-Mar-2001 (fanra01)
**          Bug 104321
**          Allow the use of '/' as the root directory.
**      30-Oct-2001 (fanra01)
**          Bug 106215
**          Changed profile messages with authorization and permission
**          messages.
**	08-Nov-2002 (somsa01)
**	    For hp2_us5, downgraded the optimization level to 1. Otherwise,
**	    in WMOLocation(), when creating a location with VDBA,
**	    the output of WCSGetRootPath() does not get passed into
**	    STprintf().
**  04-feb-2003 (fanra01)
**      Bug 109070
**      Corrected admin user right test to permit select from an
**      HTTP client and to prevent access from an API client when
**      the admin user right is not granted.
**      27-Oct-2003 (hanch04)
**	    Added wcslocation.h for function prototypes.
**/

GSTATUS
WMOTypeAction (
	char* action,
	u_i4 *type);

/*
** Name: WMOLocation() - insert, update, delete, select and retrieve locations
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
**      25-Feb-2000 (chika01)
**          Check the location to see if it exist.  If not, generate error 
**          message, 'Directory doesn't exist'.
**      23-Mar-2001 (fanra01)
**          Treat root virtual directory differently, as the root
**          url separator is appended later.
**      30-Oct-2001 (fanra01)
**          Replace ambiguous profile error message with authorization
**          and permission messages.
**          Also return the error status in all cases for the C_API client.
**      04-Feb-2003 (fanra01)
**          Add parenthesis to ensure that user right test is correct for
**          HTTP and API clients.
*/
GSTATUS
WMOLocation (
    ACT_PSESSION act_session,
    char* params[],
    bool* print,
    PTR*  user_info)
{
    GSTATUS err = GSTAT_OK;
    u_i4    action;
    i4      type = 0;
    i4      *ptype = &type;

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
            err = DDSCheckUserFlag( act_session->user_session->userid,
                WSF_USR_ADM, &profile );

            /*
            ** Guarentee that only select actions are permitted from and HTTP
            ** client if admin rights are not granted - prevents errors 
            ** appearing on web pages.
            ** Always return error to client api if admin right not granted.
            */
            G_ASSERT(profile == FALSE &&
                ((action == WSS_ACTION_TYPE_INS ||
                action == WSS_ACTION_TYPE_UPD ||
                action == WSS_ACTION_TYPE_DEL) ||
                (act_session->clientType & C_API)),
                E_WS0102_WSS_INSUFFICIENT_PERM);
        }
    }
    else
        action = WSS_ACTION_TYPE_SEL;

    if (err == GSTAT_OK)
    {
        if (action == WSS_ACTION_TYPE_SEL)
        {
            u_i4    *id = NULL;

            if (*user_info == NULL)
            {
                err = WCSOpenLocation(user_info);
                if (err == GSTAT_OK)
                    err = GAlloc((PTR*)&params[1], NUM_MAX_INT_STR, FALSE);
            }

            if (err == GSTAT_OK)
            {
                err = WCSGetLocation(
                         user_info,
                         &id,
                         &params[2],
                         (u_i4**)&ptype,
                         NULL,
                         &params[3],
                         &params[4]);

                if (err != GSTAT_OK || *user_info == NULL)
                {
                    CLEAR_STATUS (WCSCloseLocation(user_info));
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
            u_i4    id;
            char    path[MAX_LOC+1];

            switch(action)
            {
                case WSS_ACTION_TYPE_DEL:
                case WSS_ACTION_TYPE_RET:
                    G_ASSERT (params[1] == NULL ||
                        CVan(params[1], (i4*)&id) != OK,
                        E_WS0059_WSF_BAD_PARAM);
                    if (action == WSS_ACTION_TYPE_DEL)
                        err = WCSDeleteLocation (id);
                    else
                    {
                        err = WCSRetrieveLocation (
                            id,
                            &params[2],
                            (u_i4**)&ptype,
                            NULL,
                            &params[3],
                            &params[4]);
                        type = *ptype;
                    }
                    break;
                case WSS_ACTION_TYPE_INS:
                case WSS_ACTION_TYPE_UPD:
                {
                    char*   extensions;

                    G_ASSERT (params[2] == NULL || params[2][0] == EOS,
                        E_WS0059_WSF_BAD_PARAM);
                    G_ASSERT (params[3] == NULL, E_WS0059_WSF_BAD_PARAM);

                    extensions = (params[4] != NULL) ? params[4] : "" ;

                    if (params[5] != NULL &&
                        STcompare(params[5], WSF_CHECKED) == 0)
                    {
                        type = WCS_HTTP_LOCATION;
                        /*
                        ** if the path string specifies the root directory
                        ** don't append it to the phsical path.
                        */
                        STprintf( path, "%s%c%s", WCSGetRootPath(),
                            CHAR_PATH_SEPARATOR,
                            (STcompare( params[3], "/" ) == 0) ? "" : params[3]);
                    }
                    else if (params[6] != NULL &&
                        STcompare(params[6], WSF_CHECKED) == 0)
                    {
                        type = WCS_II_LOCATION;
                        STcopy( params[3], path );
                    }
                    else
                        err = DDFStatusAlloc( E_WS0059_WSF_BAD_PARAM );

                    if (err == GSTAT_OK)
                    {
                        STATUS   status = OK;
                        LOCATION loc;

                        LOfroms( PATH, path, &loc );
                        status = LOexist ( &loc );
                        G_ASSERT (status != OK, E_WS0035_INST_DIR_NOEXIST);
                    }

                    if (params[7] != NULL &&
                        STcompare(params[7], WSF_CHECKED) == 0)
                        type |= WCS_PUBLIC;

                    if (err == GSTAT_OK)
                    {
                        if (action == WSS_ACTION_TYPE_UPD)
                        {
                            G_ASSERT (params[1] == NULL ||
                                CVan(params[1], (i4*)&id) != OK,
                                E_WS0059_WSF_BAD_PARAM);
                            err = WCSUpdateLocation( id, params[2], type,
                                params[3], params[4] );
                        }
                        else
                        {
                            err = WCSCreateLocation( &id, params[2], type,
                                params[3], params[4] );
                            if (err == GSTAT_OK)
                            {
                                err = GAlloc( (PTR*)&params[1], NUM_MAX_INT_STR,
                                    FALSE );
                            if (err == GSTAT_OK)
                                WSF_CVNA( err, id, params[1] );
                            }
                        }
                    }
                    break;
                }
                default:
                    err = DDFStatusAlloc(E_WS0061_WPS_BAD_SVR_ACTION);
            }
        }
    }
    if (err == GSTAT_OK && (action == WSS_ACTION_TYPE_SEL ||
        action == WSS_ACTION_TYPE_RET))
    {
        u_i4 length = STlength( WSF_CHECKED ) + 1;

        err = GAlloc( (PTR*)&params[5], length, FALSE );
        if (err == GSTAT_OK && (type & WCS_HTTP_LOCATION))
        {
            MECOPY_VAR_MACRO(WSF_CHECKED, length, params[5]);
        }
        else
        {
            params[5][0] = EOS;
        }

        if (err == GSTAT_OK)
            err = GAlloc( (PTR*)&params[6], length, FALSE );

        if (err == GSTAT_OK && (type & WCS_II_LOCATION))
        {
            MECOPY_VAR_MACRO(WSF_CHECKED, length, params[6]);
        }
        else
        {
            params[6][0] = EOS;
        }

        if (err == GSTAT_OK)
            err = GAlloc( (PTR*)&params[7], length, FALSE );

        if (err == GSTAT_OK && (type & WCS_PUBLIC))
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
