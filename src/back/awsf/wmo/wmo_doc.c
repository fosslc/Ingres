/******************************************************************************
**
**Copyright (c) 2004 Ingres Corporation
**
******************************************************************************/

#include <wcs.h>
#include <servermeth.h>
#include <wss.h>
#include <dds.h>
#include <wsf.h>
#include <erwsf.h>
#include <wpsfile.h>

/*
**
**  Name: wmo_doc.c - Web Monitoring Object
**
**  Description:
**	that file is composed of macro server function about document management
**
**  History:    
**	5-Feb-98 (marol01)
**	    created
**      07-Sep-1998 (fanra01)
**          Enclosed MECOPY_VAR_MACRO within braces.
**          Fixup compiler warnings on unix.
**      28-Apr-2000 (fanra01)
**          Bug 100312
**          Add SSL support.  Add additional scheme parameter to WPSrequest.
**      05-May-2000 (fanra01)
**          Bug 101346
**          Set unit right for user when a user is assigned a document right.
**          Set the refs flag for unit so that all permission is not deleted
**          until all documents are revoked.
**      03-Aug-2000 (fanra01)
**          Bug 100527
**          Making a copied-out business unit downloadable from a web page
**          adds a temporary file to the available documents in the business
**          unit.  Add checks to return only documents held in the repository.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      30-Oct-2001 (fanra01)
**          Bug 106215
**          Changed profile messages with authorization and permission
**          messages.
**      09-Jul-2002 (fanra01)
**          Bug 108207
**          Add validation of document id to document/role association.
**/

GLOBALREF char* privileged_user;

GSTATUS
WMOTypeAction (
	char* action,
	u_i4 *type);

/*
** Name: WMODocument() - insert, update, delete, select and retrieve documents
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
**      28-Apr-2000 (fanra01)
**          Add additional scheme parameter to WPSrequest.
**      30-Oct-2001 (fanra01)
**          Replace ambiguous profile error message with authorization
**          and permission messages.
*/
GSTATUS
WMODocument (
    ACT_PSESSION act_session,
    char* params[],
    bool* print,
    PTR*  user_info)
{
    GSTATUS        err = GSTAT_OK;
    u_i4            action;
    u_i4            unit_id = 0;
    u_i4            ext_loc = 0;
    i4        flag = 0;
    u_i4            owner = 0;
    u_i4            *pid = NULL;
    u_i4            *punit_id = NULL;
    i4        *pflag = NULL;
    u_i4            *powner = NULL;
    u_i4            *pext_loc = NULL;
    char            *ext_file;
    char            *ext_suffix;
    bool            accessible = TRUE;
    WPS_FILE    *cache = NULL;
    bool        status;

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

    if (err == GSTAT_OK)
        err = WMOTypeAction(params[0], &action);
    }
    else
    action = WSS_ACTION_TYPE_SEL;

    if (err == GSTAT_OK)
    if (action == WSS_ACTION_TYPE_SEL)
    {
        if (*user_info == NULL)
        {
        err = WCSOpenDocument(user_info);
        if (err == GSTAT_OK)
            err = GAlloc((PTR*)&params[1], NUM_MAX_INT_STR, FALSE);
        }

        if (err == GSTAT_OK)
        {
        err = WCSGetDocument(
                     user_info,
                     &pid,
                     &punit_id,
                     &params[4],
                     &params[5],
                     (u_i4**)&pflag,
                     &powner,
                     &pext_loc,
                     &params[12],
                     &params[13]);

        if (err != GSTAT_OK || *user_info == NULL)
        {
            CLEAR_STATUS (WCSCloseDocument(user_info));
            *print = FALSE;
        }
        else
        {
            err = WSSDocAccessibility(act_session->user_session, *pid, WSF_READ_RIGHT, &accessible);
            if (err == GSTAT_OK && accessible == TRUE)
            {
            WSF_CVNA(err, *pid, params[1]);
            unit_id = *punit_id;
            flag = *pflag;
            ext_loc= *pext_loc;
            }
            else
            *print = FALSE;
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
        if (action == WSS_ACTION_TYPE_DEL)
        {
            err = WSSDocAccessibility(act_session->user_session, id, WSF_DEL_RIGHT, &accessible);

            if (err == GSTAT_OK)
            if (accessible == TRUE)
                err = WCSDeleteDocument (id);
            else
                err = DDFStatusAlloc( E_WS0065_WSF_UNAUTH_ACCESS );
        }
        else
        {
            err = WSSDocAccessibility(act_session->user_session, id, WSF_READ_RIGHT, &accessible);
            if (err == GSTAT_OK && accessible == TRUE)
            {
            err = WCSRetrieveDocument (
                           id,
                           &punit_id,
                           &params[4],
                           &params[5],
                           (u_i4**)&pflag,
                           &powner,
                           &pext_loc,
                           &params[12],
                           &params[13]);

            if (err == GSTAT_OK && punit_id != NULL)
            {
                unit_id = *punit_id;
                flag = *pflag;
                ext_loc= *pext_loc;
                owner = *powner;

                if (err == GSTAT_OK)
                {
                err = WPSRequest(
                         act_session->user_session->name,
                         act_session->scheme,
                         act_session->host,
                         act_session->port,
                         NULL,
                         unit_id,
                         params[4],
                         params[5],
                         &cache,
                         &status);

                if (err == GSTAT_OK)
                {
                    if (status == WCS_ACT_LOAD)
                    {
                    if ((flag & WCS_EXTERNAL) == FALSE)
                        err = WCSExtractDocument (cache->file);

                    if (err == GSTAT_OK)
                        CLEAR_STATUS(WCSLoaded(cache->file, TRUE))
                        else
                            CLEAR_STATUS(WCSLoaded(cache->file, FALSE))
                        }

                    if (err == GSTAT_OK)
                    {
                    char *tmp = (cache->url == NULL) ? WSF_UNDEFINED : cache->url;
                    u_i4 length = STlength(tmp) + 1;
                    err = GAlloc((PTR*)&params[10], length, FALSE);
                    if (err == GSTAT_OK)
                        MECOPY_VAR_MACRO(tmp, length, params[10]);
                    }
                    CLEAR_STATUS(WPSRelease(&cache));
                }
                }
            }
            }
            else
            err = DDFStatusAlloc( E_WS0065_WSF_UNAUTH_ACCESS );
        }
        break;
        case WSS_ACTION_TYPE_INS:
        case WSS_ACTION_TYPE_UPD:
        {
            char *suffix;

            G_ASSERT (params[2] == NULL || CVan(params[2], (i4*)&unit_id) != OK, E_WS0059_WSF_BAD_PARAM);
            G_ASSERT (params[4] == NULL || params[4][0] == EOS, E_WS0059_WSF_BAD_PARAM);

            if (params[5] != NULL)
            suffix = params[5];
            else
            suffix = "";

            if (params[6] != NULL && STcompare(params[6], WSF_CHECKED) == 0)
            flag |= WCS_PUBLIC;

            if (params[11] != NULL && CVan(params[11], (i4*)&ext_loc) == OK && ext_loc != 0)
            {
            flag |= WCS_EXTERNAL;
            ext_file = params[12];
            ext_suffix = params[13];
            }
            else
            {
            ext_file = params[10];
            ext_suffix = NULL;
            if (params[7] != NULL && STcompare(params[7], WSF_CHECKED) == 0)
                flag |= WCS_PRE_CACHE;
            else if (params[8] != NULL && STcompare(params[8], WSF_CHECKED) == 0)
                flag |= WCS_PERMANENT_CACHE;
            else
                flag |= WCS_SESSION_CACHE;
            }

            if (action == WSS_ACTION_TYPE_UPD)
            {
            bool transfer = FALSE;
            bool page;
            WCS_FILE *file = NULL;

            G_ASSERT (params[1] == NULL || CVan(params[1], (i4*)&id) != OK, E_WS0059_WSF_BAD_PARAM);

            err = WCSDocCheckFlag (id, WSF_PAGE, &page);

            if (page == TRUE)
                flag |= WSF_PAGE;
            else
                flag |= WSF_FACET;

            if (err == GSTAT_OK &&
                params[15] != NULL &&
                STcompare(params[15], WSF_CHECKED) == 0)
            {    /* transfer external file in the repository */
                bool    is_external;
                char*    loc_name = NULL;

                err = WCSDocCheckFlag(id, WCS_EXTERNAL, &is_external);
                if (err == GSTAT_OK)
                {
                if (is_external == TRUE)
                {
                    flag &= (WCS_EXTERNAL ^ flag);
                    flag |= WCS_SESSION_CACHE;
                    err = WCSRetrieveLocation(
                                  ext_loc,
                                  &loc_name,
                                  NULL,
                                  NULL,
                                  NULL,
                                  NULL);

                    if (err == GSTAT_OK)
                    err = WCSRequest(
                             0,
                             loc_name,
                             unit_id,
                             params[4],
                             params[5],
                             &file,
                             &status);

                    if (err == GSTAT_OK &&
                    status == WCS_ACT_LOAD)
                    err = WCSLoaded(file, TRUE);

                    if (err == GSTAT_OK)
                    {
                    MEfree((PTR)ext_file);
                    ext_file = file->info->path;
                    params[11][0] = EOS;
                    transfer = TRUE;
                    }
                }
                else
                    err = DDFStatusAlloc(E_WS0082_WSS_INTERNAL);
                }
            }
            if (err == GSTAT_OK)
                err = WSSDocAccessibility(act_session->user_session, id, WSF_UPD_RIGHT, &accessible);

            if (err == GSTAT_OK)
                if (accessible == TRUE)
                err = WCSUpdateDocument (id, unit_id, params[4], suffix, flag, act_session->user_session->userid, ext_loc, ext_file, ext_suffix);
                else
                err = DDFStatusAlloc( E_WS0065_WSF_UNAUTH_ACCESS );

            if (transfer == TRUE)
            {
                ext_file = NULL;
                if (err == GSTAT_OK)
                CLEAR_STATUS(WCSDelete(file));
                CLEAR_STATUS(WCSRelease (&file));
            }
            }
            else
            {
            G_ASSERT (ext_file == NULL || ext_file[0] == EOS, E_WS0059_WSF_BAD_PARAM);

            if (params[16] != NULL &&
                STbcompare(
                       params[16],
                       STlength(params[16]),
                       WSF_FACET_STR,
                       STlength(WSF_FACET_STR),
                       TRUE) == 0)
                flag |= WSF_FACET;
            else
                flag |= WSF_PAGE;

            err = WSSUnitAccessibility(act_session->user_session, unit_id, WSF_INS_RIGHT, &accessible);
            if (err == GSTAT_OK)
                if (accessible == TRUE)
                {
                err = WCSCreateDocument(&id, unit_id, params[4], suffix, flag, act_session->user_session->userid, ext_loc, ext_file, ext_suffix);
                if (err == GSTAT_OK)
                {
                    err = GAlloc((PTR*)&params[1], NUM_MAX_INT_STR, FALSE);
                    if (err == GSTAT_OK)
                    WSF_CVNA(err, id, params[1]);
                }
                }
                else
                err = DDFStatusAlloc( E_WS0065_WSF_UNAUTH_ACCESS );
            }
        }
        break;
        default:
        err = DDFStatusAlloc(E_WS0061_WPS_BAD_SVR_ACTION);
        }
    }
    if (err == GSTAT_OK &&
    *print == TRUE &&
    (action == WSS_ACTION_TYPE_SEL ||
     action == WSS_ACTION_TYPE_RET ||
     action == WSS_ACTION_TYPE_UPD))
    {
    u_i4 length = STlength (WSF_CHECKED) + 1;

    err = GAlloc((PTR*)&params[2], NUM_MAX_INT_STR, FALSE);
    if (err == GSTAT_OK)
        WSF_CVNA(err, unit_id, params[2]);

    if (err == GSTAT_OK && unit_id != 0)
        err = WCSGetUnitName(unit_id, &params[3]);

    err = GAlloc((PTR*)&params[6], length, FALSE);
    if (err == GSTAT_OK && flag & WCS_PUBLIC)
    {
        MECOPY_VAR_MACRO(WSF_CHECKED, length, params[6]);
    }
    else
    {
        params[6][0] = EOS;
    }

    err = GAlloc((PTR*)&params[7], length, FALSE);
    if (err == GSTAT_OK && flag & WCS_PRE_CACHE)
    {
        MECOPY_VAR_MACRO(WSF_CHECKED, length, params[7]);
    }
    else
    {
        params[7][0] = EOS;
    }

    err = GAlloc((PTR*)&params[8], length, FALSE);
    if (err == GSTAT_OK && flag & WCS_PERMANENT_CACHE)
    {
        MECOPY_VAR_MACRO(WSF_CHECKED, length, params[8]);
    }
    else
    {
        params[8][0] = EOS;
    }

    err = GAlloc((PTR*)&params[9], length, FALSE);
    if (err == GSTAT_OK && flag & WCS_SESSION_CACHE)
    {
        MECOPY_VAR_MACRO(WSF_CHECKED, length, params[9]);
    }
    else
    {
        params[9][0] = EOS;
    }

    err = GAlloc((PTR*)&params[11], length, FALSE);
    if (err == GSTAT_OK && flag & WCS_EXTERNAL)
        WSF_CVNA(err, ext_loc, params[11])
        else
            params[11][0] = EOS;

    err = GAlloc((PTR*)&params[16], WSF_MAX_TYPE_STR, FALSE);
    if ((err == GSTAT_OK) &&
            ((flag & (WSF_PAGE|WSF_TEMP)) == WSF_PAGE))
    {
        MECOPY_VAR_MACRO(WSF_PAGE_STR, STlength(WSF_PAGE_STR)+1, params[16]);
    }
    else if ((err == GSTAT_OK) &&
            ((flag & (WSF_FACET|WSF_TEMP)) == WSF_FACET))
    {
        MECOPY_VAR_MACRO(WSF_FACET_STR, STlength(WSF_FACET_STR)+1, params[16]);
    }
        else
        {
            params[16][0] = EOS;
        }

    if (err == GSTAT_OK)
        if (owner != SYSTEM_ID)
        err = DDSGetUserName(owner, &params[14]);
        else
        {
        length = STlength(privileged_user) + 1;
        err = GAlloc((PTR*)&params[14], length, FALSE);
        if (err == GSTAT_OK)
        {
            MECOPY_VAR_MACRO(privileged_user, length, params[14]);
        }
        }
    }
    MEfree((PTR) pid);
    MEfree((PTR) pflag);
    MEfree((PTR) punit_id);
    MEfree((PTR) powner);
    MEfree((PTR) pext_loc);
    return(err);
}

/*
** Name: WMODocRole() - insert, delete and retrieve document/role associations
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
**      09-Jul-2002 (fanra01)
**          Add validation of document id.
*/
GSTATUS
WMODocRole (
	ACT_PSESSION act_session, 
	char* params[],
	bool* print,
	PTR*  user_info)
{
	GSTATUS		err = GSTAT_OK;
	u_i4		action;
	u_i4		doc_id;
	u_i4		role_id;
	i4	rights;
	char		resource[WSF_STR_RESOURCE];

	*print = TRUE;

	G_ASSERT (params[1] == NULL || CVan(params[1], (i4*)&doc_id) != OK, E_WS0059_WSF_BAD_PARAM);	
        if ((err = WCSvaliddoc( doc_id, NULL )) != GSTAT_OK)
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
			bool	profile;
			bool	owner;
			err = DDSCheckUserFlag (act_session->user_session->userid, WSF_USR_SEC, &profile);
			if (err == GSTAT_OK)
				err = WCSDocIsOwner(doc_id, act_session->user_session->userid, &owner);

			if (err == GSTAT_OK)
				G_ASSERT (profile == FALSE && owner == FALSE, E_WS0102_WSS_INSUFFICIENT_PERM);
		}

		G_ASSERT (params[0] == NULL || params[0][0] == EOS, E_WS0059_WSF_BAD_PARAM);

		if (err == GSTAT_OK)
			err = WMOTypeAction(params[0], &action);
	}
	else
		action = WSS_ACTION_TYPE_RET;

	STprintf(resource, "d%d", doc_id);

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
				if (err == GSTAT_OK)
					err = GAlloc((PTR*)&params[7], length, FALSE);
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

					if (err == GSTAT_OK && CHECK_RIGHT(rights, WSF_READ_RIGHT))
					{
						MECOPY_VAR_MACRO(WSF_CHECKED, length, params[4]);
					}
					else
					{
						params[4][0] = EOS;
					}

					if (err == GSTAT_OK && CHECK_RIGHT(rights, WSF_UPD_RIGHT))
					{
						MECOPY_VAR_MACRO(WSF_CHECKED, length, params[5]);
					}
					else
					{
						params[5][0] = EOS;
					}

					if (err == GSTAT_OK && CHECK_RIGHT(rights, WSF_DEL_RIGHT))
					{
						MECOPY_VAR_MACRO(WSF_CHECKED, length, params[6]);
					}
					else
					{
						params[6][0] = EOS;
					}

					if (err == GSTAT_OK && CHECK_RIGHT(rights, WSF_EXE_RIGHT))
					{
						MECOPY_VAR_MACRO(WSF_CHECKED, length, params[7]);
					}
					else
					{
						params[7][0] = EOS;
					}
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
					err = DDSAssignRoleRight (role_id, resource, WSF_READ_RIGHT);
				else
					err = DDSDeleteRoleRight (role_id, resource, WSF_READ_RIGHT);

				if (params[5] != NULL && STcompare(params[5], WSF_CHECKED) == 0)
					err = DDSAssignRoleRight (role_id, resource, WSF_UPD_RIGHT);
				else
					err = DDSDeleteRoleRight (role_id, resource, WSF_UPD_RIGHT);

				if (params[6] != NULL && STcompare(params[6], WSF_CHECKED) == 0)
					err = DDSAssignRoleRight (role_id, resource, WSF_DEL_RIGHT);
				else
					err = DDSDeleteRoleRight (role_id, resource, WSF_DEL_RIGHT);

				if (params[7] != NULL && STcompare(params[7], WSF_CHECKED) == 0)
					err = DDSAssignRoleRight (role_id, resource, WSF_EXE_RIGHT);
				else
					err = DDSDeleteRoleRight (role_id, resource, WSF_EXE_RIGHT);
				break;
			default: 
				err = DDFStatusAlloc(E_WS0061_WPS_BAD_SVR_ACTION);
			}
		}
return(err);
}

/*
** Name: WMODocUser() - insert, delete and retrieve document/user associations
**
** Description:
**
** Inputs:
**    ACT_PSESSION: active session
**    char*[]     : list of parameters
**
** Outputs:
**    bool*       : TRUE is there is something to print
**    PTR*        : cursor pointer (if NULL means no more information)
**
** Returns:
**    GSTATUS     :    GSTAT_OK
**
** Exceptions:
**    None
**
** Side Effects:
**    None
**
** History:
**      05-May-2000 (fanra01)
**          Add additional resource string for the adding association for the
**          the unit.  Add additional parameters to the assign and delete
**          functions.
**      30-Oct-2001 (fanra01)
**          Replace ambiguous profile error message with authorization
**          and permission messages.
*/
GSTATUS
WMODocUser (
    ACT_PSESSION act_session,
    char* params[],
    bool* print,
    PTR*  user_info)
{
    GSTATUS err = GSTAT_OK;
    u_i4    action;
    u_i4    doc_id;
    u_i4    user_id;
    i4      rights;
    char    resource[WSF_STR_RESOURCE];
    char    resource1[WSF_STR_RESOURCE];
    u_i4    unit_id;

    *print = TRUE;

    G_ASSERT (params[1] == NULL || CVan(params[1], (i4*)&doc_id) != OK, E_WS0059_WSF_BAD_PARAM);

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
            bool    profile;
            bool    owner;
            err = DDSCheckUserFlag (act_session->user_session->userid, WSF_USR_SEC, &profile);
            if (err == GSTAT_OK)
                err = WCSDocIsOwner(doc_id, act_session->user_session->userid, &owner);

            if (err == GSTAT_OK)
                G_ASSERT (profile == FALSE && owner == FALSE, E_WS0102_WSS_INSUFFICIENT_PERM);
        }

        G_ASSERT (params[0] == NULL || params[0][0] == EOS, E_WS0059_WSF_BAD_PARAM);

        if (err == GSTAT_OK)
            err = WMOTypeAction(params[0], &action);
    }
    else
        action = WSS_ACTION_TYPE_RET;

    STprintf(resource, "d%d", doc_id);
    if ((err = WCSDocGetUnit( doc_id, &unit_id )) == GSTAT_OK)
    {
        STprintf(resource1,"p%d", unit_id);
    }

    if (err == GSTAT_OK)
    {
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
                if (err == GSTAT_OK)
                    err = GAlloc((PTR*)&params[7], length, FALSE);
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

                    if (err == GSTAT_OK && CHECK_RIGHT(rights, WSF_READ_RIGHT))
                    {
                        MECOPY_VAR_MACRO(WSF_CHECKED, length, params[4]);
                    }
                    else
                    {
                        params[4][0] = EOS;
                    }

                    if (err == GSTAT_OK && CHECK_RIGHT(rights, WSF_UPD_RIGHT))
                    {
                        MECOPY_VAR_MACRO(WSF_CHECKED, length, params[5]);
                    }
                    else
                    {
                        params[5][0] = EOS;
                    }

                    if (err == GSTAT_OK && CHECK_RIGHT(rights, WSF_DEL_RIGHT))
                    {
                        MECOPY_VAR_MACRO(WSF_CHECKED, length, params[6]);
                    }
                    else
                    {
                        params[6][0] = EOS;
                    }

                    if (err == GSTAT_OK && CHECK_RIGHT(rights, WSF_EXE_RIGHT))
                    {
                        MECOPY_VAR_MACRO(WSF_CHECKED, length, params[7]);
                    }
                    else
                    {
                        params[7][0] = EOS;
                    }
                }
            }
        }
        else
        {
            bool created = FALSE;
            bool deleted = FALSE;

            G_ASSERT (params[2] == NULL || CVan(params[2], (i4*)&user_id) != OK, E_WS0059_WSF_BAD_PARAM);

            switch(action)
            {
            case WSS_ACTION_TYPE_UPD:
                if (params[4] != NULL && STcompare(params[4], WSF_CHECKED) == 0)
                {
                    created = FALSE;
                    if ((err = DDSAssignUserRight( user_id, resource,
                        WSF_READ_RIGHT, 0, &created)) == GSTAT_OK)
                    {
                        /*
                        ** Set the refs flag to update reference only if the
                        ** previous assign indicates a creation.
                        ** That way we don't increment the count for updates.
                        */
                        err =  DDSAssignUserRight( user_id, resource1,
                            WSF_READ_RIGHT, created, &created );
                    }
                }
                else
                {
                    deleted = FALSE;
                    if ((err =  DDSDeleteUserRight( user_id, resource,
                        WSF_READ_RIGHT, 0, &deleted )) == GSTAT_OK)
                    {
                        /*
                        ** Set the refs flag to update reference only if the
                        ** previous assign indicates a creation.
                        ** That way we don't decrement the count for updates.
                        */
                        err = DDSDeleteUserRight(user_id, resource1,
                            WSF_READ_RIGHT, deleted, &deleted );
                    }
                }
                if (params[5] != NULL && STcompare(params[5], WSF_CHECKED) == 0)
                {
                    created = FALSE;
                    if ((err = DDSAssignUserRight( user_id, resource,
                        WSF_UPD_RIGHT, 0, &created )) == GSTAT_OK)
                    {
                        /*
                        ** Set the refs flag to update reference only if the
                        ** previous assign indicates a creation.
                        ** That way we don't increment the count for updates.
                        */
                        err =  DDSAssignUserRight( user_id, resource1,
                            WSF_UPD_RIGHT, created, &created );
                    }
                }
                else
                {
                    deleted = FALSE;
                    if ((err =  DDSDeleteUserRight( user_id, resource,
                        WSF_UPD_RIGHT, 0, &deleted )) == GSTAT_OK)
                    {
                        /*
                        ** Set the refs flag to update reference only if the
                        ** previous assign indicates a creation.
                        ** That way we don't decrement the count for updates.
                        */
                        err = DDSDeleteUserRight( user_id, resource1,
                            WSF_UPD_RIGHT, deleted, &deleted );
                    }
                }

                if (params[6] != NULL && STcompare(params[6], WSF_CHECKED) == 0)
                {
                    created = FALSE;
                    if ((err = DDSAssignUserRight( user_id, resource,
                        WSF_DEL_RIGHT, 0, &created )) == GSTAT_OK)
                    {
                        /*
                        ** Set the refs flag to update reference only if the
                        ** previous assign indicates a creation.
                        ** That way we don't increment the count for updates.
                        */
                        err =  DDSAssignUserRight( user_id, resource1,
                            WSF_DEL_RIGHT, created, &created );
                    }
                }
                else
                {
                    deleted = FALSE;
                    if ((err =  DDSDeleteUserRight( user_id, resource,
                        WSF_DEL_RIGHT, 0, &deleted )) == GSTAT_OK)
                    {
                        /*
                        ** Set the refs flag to update reference only if the
                        ** previous assign indicates a creation.
                        ** That way we don't decrement the count for updates.
                        */
                        err = DDSDeleteUserRight( user_id, resource1,
                            WSF_DEL_RIGHT, deleted, &deleted );
                    }
                }

                if (params[7] != NULL && STcompare(params[7], WSF_CHECKED) == 0)
                {
                    created = FALSE;
                    if ((err = DDSAssignUserRight( user_id, resource,
                        WSF_EXE_RIGHT, 0, &created )) == GSTAT_OK)
                    {
                        /*
                        ** Set the refs flag to update reference only if the
                        ** previous assign indicates a creation.
                        ** That way we don't increment the count for updates.
                        */
                        err =  DDSAssignUserRight( user_id, resource1,
                            WSF_EXE_RIGHT, created, &created );
                    }
                }
                else
                {
                    deleted = FALSE;
                    if ((err =  DDSDeleteUserRight( user_id, resource,
                        WSF_EXE_RIGHT, 0, &deleted )) == GSTAT_OK)
                    {
                        /*
                        ** Set the refs flag to update reference only if the
                        ** previous assign indicates a creation.
                        ** That way we don't decrement the count for updates.
                        */
                        err = DDSDeleteUserRight( user_id, resource1,
                            WSF_EXE_RIGHT, deleted, &deleted );
                    }
                }
                break;
            default:
                err = DDFStatusAlloc(E_WS0061_WPS_BAD_SVR_ACTION);
            }
        }
    }
    return(err);
}
