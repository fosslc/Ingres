/******************************************************************************
**
**Copyright (c) 2004 Ingres Corporation
**
******************************************************************************/

#include <wsf.h>
#include <dds.h>
#include <erwsf.h>
#include <servermeth.h>
#include <wcs.h>
#include <wsmvar.h>
#include <htmlmeth.h>

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
**      07-Sep-1998 (fanra01)
**          Corrected case of wsmvar to match piccolo.
**          Fixup compiler warnings on unix.
**      04-Nov-98 (fanra01)
**          Add check for null value in WMOGetVariables.
**      19-Apr-1998 (chika01)
**          Add check for duplicate server variable in WMOVariables().
**      01-Aug-2000 (fanra01)
**          Bug 102229
**          Change WMOGetSvrVariables to allow update of server variables and
**          to return an error message when trying to create a duplicate
**          variable.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      04-May-2001 (fanra01)
**          Sir 103813
**          Add the supporting function for the ice_var function.
**      30-Oct-2001 (fanra01)
**          Bug 106215
**          Changed profile messages with authorization and permission
**          messages.
**/

typedef struct _tag_icevarscope
{
    char*   name;
    u_i4    value;
}VARSCOPE;

static VARSCOPE icescope[] =
{
    { STR_DECL_SERVER,  WSM_SYSTEM  },
    { STR_DECL_SESSION, WSM_USER    },
    { STR_DECL_PAGE,    WSM_ACTIVE  },
    { NULL,             0  }
};

GSTATUS
WMOTypeAction (
	char* action,
	u_i4 *type);

/*
** Name: WMOTag2String() - Convert tags to String
**
** Description:
**
** Inputs:
**	char*			: tags
**
** Outputs:
**	char*			: string
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
WMOTag2String  (
	ACT_PSESSION act_session, 
	char* params[],
	bool* print,
	PTR*  user_info)
{
	*print = TRUE;
return (WPSConvertTag(params[0], -1, &params[1], NULL, FALSE));
}

/*
** Name: WMOGetLocFiles() - Return files of a specific location
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
WMOGetLocFiles (
	ACT_PSESSION act_session, 
	char* params[],
	bool* print,
	PTR*  user_info)
{
	GSTATUS		err = GSTAT_OK;
	LO_DIR_CONTEXT *lc = NULL;
	LOCATION	loc;
	u_i4		cl_status;

	*print = FALSE;

	if (*user_info == NULL)
	{
		i4	id;
		char*		path = NULL;

		G_ASSERT (params[0] == NULL || CVan(params[0], &id) != OK, E_WS0059_WSF_BAD_PARAM);

		err = WCSRetrieveLocation (
						id, 
						NULL, 
						NULL, 
						&path, 
						NULL,
						NULL);

		if (err == GSTAT_OK)
			err = G_ME_REQ_MEM(0, lc, LO_DIR_CONTEXT, 1);

		if (err == GSTAT_OK)
		{
			char* fprefix = NULL;
			char* fsuffix = NULL;
			LOfroms(PATH, path, &loc);

			if (params[1] != NULL)
			{
				fprefix = params[1];
				for (fsuffix = fprefix; *fsuffix != EOS || *fsuffix != CHAR_FILE_SUFFIX_SEPARATOR; fsuffix++) ;
				if (*fsuffix == EOS)
					fsuffix = NULL;
			}
			if ((cl_status = LOwcard(&loc, fprefix, fsuffix, NULL, lc)) == OK)
				*user_info = (PTR) lc;
		}
		MEfree((PTR) path);	
	}
	else
	{
		lc = (LO_DIR_CONTEXT*) *user_info;
		cl_status = LOwnext( lc, &loc);
	}

	if (cl_status == OK)
	{
		char dev[LO_NM_LEN];
		char path[MAX_LOC];
		char fprefix[LO_NM_LEN];
		char fsuffix[LO_NM_LEN];
		char version[LO_NM_LEN];
		u_i4 len;	
		if (LOdetail(&loc, dev, path, fprefix, fsuffix, version) == OK)
		{
			len = STlength(fprefix) + 1;
			err = GAlloc((PTR*)&params[1], len, FALSE);
			if (err == GSTAT_OK)
			{
				MECOPY_VAR_MACRO(fprefix, len, params[1]);
				len = STlength(fsuffix) + 1;
				err = GAlloc((PTR*)&params[2], len, FALSE);
				if (err == GSTAT_OK)
				{
					MECOPY_VAR_MACRO(fsuffix, len, params[2]);
					*print = TRUE;
				}
			}
		}
	}
	else
	{
		if (cl_status == ENDFILE)
			LOwend(lc);
		else
			err = DDFStatusAlloc ( E_WS0078_WMO_DIR_LOCATION );

		MEfree((PTR)lc);
		params[1] = NULL;
		*user_info = NULL;
	}
return(err);
}

/*
** Name: WMOGetVariables() - Returns the variable list for session and/or pages
**
** Description:
**
** Inputs:
**	char*			: tags
**
** Outputs:
**	char*			: string
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
**      04-Nov-98 (fanra01)
**          Add check for null value.
*/
GSTATUS
WMOGetVariables (
    ACT_PSESSION    act_session,
    char*           params[],
    bool*           print,
    PTR*            user_info)
{
    GSTATUS         err = GSTAT_OK;
    u_i4           length;
    char*           variable = NULL;
    char*           value = NULL;
    u_i4           level = 0;
    WSM_PSCAN_VAR   scanner = NULL;

    *print = FALSE;

    if (*user_info == NULL)
    {
        if (params[0] != NULL && STcompare(params[0], WSF_CHECKED) == 0)
            level += WSM_ACTIVE;

        if (params[1] != NULL && STcompare(params[1], WSF_CHECKED) == 0)
            level += WSM_USER;

        if (params[2] != NULL && STcompare(params[2], WSF_CHECKED) == 0)
            level += WSM_SYSTEM;

        G_ASSERT (level == 0, E_WS0059_WSF_BAD_PARAM);

        err = G_ME_REQ_MEM(0, scanner, WSM_SCAN_VAR, 1);
        if (err == GSTAT_OK)
            err = WSMOpenVariable (scanner, act_session, level);
        *user_info = (PTR) scanner;
    }
    else
    {
        scanner = (WSM_PSCAN_VAR) *user_info;
    }

    if (err == GSTAT_OK)
    {
        err = WSMScanVariable (scanner, &variable, &value, &level);
        if (err != GSTAT_OK || variable != NULL)
        {
            length = STlength(variable) + 1;
            err = GAlloc((PTR*)&params[3], length, FALSE);
            if (err == GSTAT_OK)
            {
                MECOPY_VAR_MACRO(variable, length, params[3]);
            }
            else
            {
                params[3][0] = EOS;
            }

            if (err == GSTAT_OK)
            {
                length = (value != NULL) ? STlength(value) + 1 : 1;
                err = GAlloc((PTR*)&params[4], length, FALSE);
                if (err == GSTAT_OK)
                {
                    if (value != NULL)
                    {
                        MECOPY_VAR_MACRO(value, length, params[4]);
                    }
                    else
                    {
                        params[4][0] = EOS;
                    }
                }
                else
                {
                    params[4][0] = EOS;
                }
            }

            length = STlength(WSF_CHECKED) + 1;
            if (err == GSTAT_OK)
            {
                err = GAlloc((PTR*)&params[0], length, FALSE);

                if (err == GSTAT_OK && level & WSM_ACTIVE)
                {
                    MECOPY_VAR_MACRO(WSF_CHECKED, length, params[0]);
                }
                else
                {
                    params[0][0] = EOS;
                }
            }

            if (err == GSTAT_OK)
            {
                err = GAlloc((PTR*)&params[1], length, FALSE);

                if (err == GSTAT_OK && level & WSM_USER)
                {
                    MECOPY_VAR_MACRO(WSF_CHECKED, length, params[1]);
                }
                else
                {
                    params[1][0] = EOS;
                }
            }

            if (err == GSTAT_OK)
            {
                err = GAlloc((PTR*)&params[2], length, FALSE);

                if (err == GSTAT_OK && level & WSM_SYSTEM)
                {
                    MECOPY_VAR_MACRO(WSF_CHECKED, length, params[2]);
                }
                else
                {
                    params[2][0] = EOS;
                }
            }

            if (err == GSTAT_OK)
                *print = TRUE;
        }
        else
        {
            CLEAR_STATUS(WSMCloseVariable (scanner));
            MEfree((PTR) scanner);
            *user_info = NULL;
        }
    }
    return(err);
}

/*
** Name: WMOVariables()
**
** Description:
**      Retrieves or Updates a server variable value.
**
** Inputs:
**      act_session     current session control block
**      params[0]       action
**      params[1]       name of variable
**      params[2]       value of varible (on update)
**
** Outputs:
**      params[2]       value of varible (on retrieve)
**
** Returns:
**      err     GSTAT_OK                    Completed successfully
**              E_WS0059_WSF_BAD_PARAM      Input parameter is null or empty
**              E_WS0061_WPS_BAD_SVR_ACTION params[0] specfies invalid action
**
** Side Effects:
**	None
**
** History:
**      27-Nov-1998 (fanra01)
**          Created.
**      19-Apr-1998 (chika01)
**          Add check for duplicate server variable.
**      01-Aug-2000 (fanra01)
**          Moved check for duplicate variable to created ones only.
**          Add insert and delete action.
**      30-Oct-2001 (fanra01)
**          Replace ambiguous profile error message with authorization
**          and permission messages.
*/
GSTATUS
WMOGetSvrVariables (
    ACT_PSESSION    act_session,
    char*           params[],
    bool*           print,
    PTR*            user_info)
{
    GSTATUS         err = GSTAT_OK;
    u_i4            action = WSS_ACTION_TYPE_RET;
    u_i4            length;
    char*           variable = NULL;
    char*           value = NULL;
    u_i4            level = WSM_SYSTEM;

    *print = FALSE;

    /*
    ** First time in
    */
    if (*user_info == NULL)
    {
        /*
        ** if no action specified return error
        */
	G_ASSERT (params[0] == NULL || params[0][0] == EOS,
            E_WS0059_WSF_BAD_PARAM);
	if (act_session->user_session == NULL)
	{
            err = DDFStatusAlloc( E_WS0101_WSS_UNAUTHORIZED_USE );
	    return(err);
	}

	if (err == GSTAT_OK)
	    err = WMOTypeAction(params[0], &action);

        if (err == GSTAT_OK)
        {
            G_ASSERT (params[1] == NULL || *params[1] == EOS,
                E_WS0059_WSF_BAD_PARAM);

            switch (action)
            {
                case WSS_ACTION_TYPE_RET:
                    err = WSMGetVariable (act_session, params[1],
                        STlength(params[1]), &value, level);
                    if (err == GSTAT_OK)
                    {
                        length = (value != NULL) ? STlength(value) + 1 : 1;
                        err = GAlloc((PTR*)&params[2], length, FALSE);
                        if ((err == GSTAT_OK) && (value != NULL))
                        {
                            MECOPY_VAR_MACRO(value, length, params[2]);
                            *print = TRUE;
                        }
                    }
                    break;
                case WSS_ACTION_TYPE_DEL:
                    err = WSMGetVariable (act_session, params[1],
                        STlength(params[1]), &value, level);
                    if (err == GSTAT_OK)
                    {
                        if (value != NULL)
                        {
                            err = WSMDelVariable( act_session, params[1],
                                STlength(params[1]), level );
                        }
                        else
                        {
                            err = DDFStatusAlloc(E_WS0012_WSS_VAR_BAD_NAME);
                        }
                    }
                    break;

                case WSS_ACTION_TYPE_INS:
                    err = WSMGetVariable (act_session, params[1],
                        STlength(params[1]), &value, level);
                    if ((err != GSTAT_OK) || (value != NULL))
                    {
                        G_ASSERT(value != NULL, E_WS0098_WMO_EXISTING_SER_VAR);
                        break;
                    }
                    /*
                    ** Intentional fall through to next case if creating a new
                    ** variable.
                    */
                case WSS_ACTION_TYPE_UPD:
                    value =  ((params[2] == NULL) && (*params[2] == EOS)) ?
                        "\0" : params[2];
                    length = STlength(value);
                    err = WSMAddVariable (act_session, params[1],
                        STlength (params[1]), value, length, level);
                    break;

                default:
                    err = DDFStatusAlloc(E_WS0061_WPS_BAD_SVR_ACTION);
                    break;
            }
        }
    }
    else
    {
        MEfree((PTR) user_info);
        user_info = NULL;
    }
    return(err);
}

/*
** Name: WMOIceVariables()
**
** Description:
**      
**
** Inputs:
**      act_session     current session control block
**      params[0]       action
**      params[1]       scope of variable
**      params[2]       name of variable
**      params[3]       value of varible (on update)
**
** Outputs:
**      params[3]       value of varible (on retrieve)
**
** Returns:
**      err     GSTAT_OK                    Completed successfully
**              E_WS0059_WSF_BAD_PARAM      Input parameter is null or empty
**              E_WS0061_WPS_BAD_SVR_ACTION params[0] specfies invalid action
**
** Side Effects:
**	None
**
** History:
**      27-Apr-2001 (fanra01)
**          Created.
**      30-Oct-2001 (fanra01)
**          Replace ambiguous profile error message with authorization
**          and permission messages.
*/
GSTATUS
WMOIceVariables (
    ACT_PSESSION    act_session,
    char*           params[],
    bool*           print,
    PTR*            user_info)
{
    GSTATUS         err = GSTAT_OK;
    i4              i;
    u_i4            action;
    u_i4            length;
    u_i4            level;
    char*           scope = NULL;
    char*           name  = NULL;
    char*           value = NULL;
    WSM_PSCAN_VAR   scanner = NULL;

    *print = FALSE;

    /*
    ** If this is the first time through the function
    */
    if (*user_info == NULL)
    {
        /*
        ** Check that there is an authenticated session
        */
	G_ASSERT( act_session->user_session == NULL,
            E_WS0101_WSS_UNAUTHORIZED_USE );

        /*
        ** Check that an action parameter has been supplied
        */
	G_ASSERT( params[0] == NULL || params[0][0] == EOS,
            E_WS0059_WSF_BAD_PARAM );

        /*
        ** Check that an scope parameter has been supplied
        */
        G_ASSERT( params[1] == NULL || *params[1] == EOS,
            E_WS0059_WSF_BAD_PARAM );

        /*
        ** Work out what the scope value is
        */
        for(i=0; icescope[i].name != NULL; i+=1)
        {
            if (STbcompare( icescope[i].name, 0, params[1], 0, TRUE ) == 0)
            {
                level = icescope[i].value;
                break;
            }
        }

        /*
        ** Get the action parameter value.
        */
        if ((err = WMOTypeAction( params[0], &action )) == GSTAT_OK)
        {
            if (action == WSS_ACTION_TYPE_SEL)
            {
                err = G_ME_REQ_MEM(0, scanner, WSM_SCAN_VAR, 1);
                if (err == GSTAT_OK)
                {
                    *user_info = (PTR) scanner;
                    err = WSMOpenVariable( scanner, act_session, level );
                }
            }
        }
    }
    else
    {
        action = WSS_ACTION_TYPE_SEL;
        scanner = (WSM_PSCAN_VAR) *user_info;
    }
    if (err == GSTAT_OK)
    {
        switch(action)
        {
            case WSS_ACTION_TYPE_SEL:
                if (scanner != NULL)
                {
                    err = WSMScanVariable( scanner, &name, &value, &level );
                    if ((err == GSTAT_OK) && (value != NULL))
                    {
                        /*
                        ** Work out what the scope value is
                        */
                        for(i=0; icescope[i].name != NULL; i+=1)
                        {
                            if (icescope[i].value & level)
                            {
                                length = STlength(icescope[i].name) + 1;
                                err = GAlloc((PTR*)&params[1], length, FALSE);
                                if (err == GSTAT_OK)
                                {
                                    MECOPY_VAR_MACRO(icescope[i].name, length,
                                        params[1]);
                                }
                                else
                                {
                                    *params[2] = EOS;
                                }
                                break;
                            }
                        }
                        length = STlength(name) + 1;
                        err = GAlloc((PTR*)&params[2], length, FALSE);
                        if (err == GSTAT_OK)
                        {
                            MECOPY_VAR_MACRO(name, length, params[2]);
                        }
                        else
                        {
                            *params[2] = EOS;
                        }

                        if (err == GSTAT_OK)
                        {
                            length = (value != NULL) ? STlength(value) + 1 : 1;
                            err = GAlloc((PTR*)&params[3], length, FALSE);
                            if (err == GSTAT_OK)
                            {
                                if (value != NULL)
                                {
                                    MECOPY_VAR_MACRO(value, length, params[3]);
                                }
                                else
                                {
                                    *params[3] = EOS;
                                }
                            }
                        }
                        *print = TRUE;
                    }
                    else
                    {
                        CLEAR_STATUS(WSMCloseVariable( scanner ));
                        MEfree( (PTR) scanner );
                        *user_info = NULL;
                    }
                }
                break;
            case WSS_ACTION_TYPE_RET:
                err = WSMGetVariable( act_session, params[2],
                    STlength(params[2]), &value, level );
                if (err == GSTAT_OK)
                {
                    length = (value != NULL) ? STlength(value) + 1 : 1;
                    err = GAlloc((PTR*)&params[3], length, FALSE);
                    if (err == GSTAT_OK)
                    {
                        if (value != NULL)
                        {
                            MECOPY_VAR_MACRO(value, length, params[3]);
                        }
                        else
                        {
                            *params[3] = EOS;
                        }
                        *print = TRUE;
                    }
                }
                break;
            case WSS_ACTION_TYPE_DEL:
                err = WSMGetVariable( act_session, params[2],
                    STlength(params[2]), &value, level );
                if (err == GSTAT_OK)
                {
                    if (value != NULL)
                    {
                        err = WSMDelVariable( act_session, params[2],
                            STlength(params[2]), level );
                    }
                    else
                    {
                        err = DDFStatusAlloc(E_WS0012_WSS_VAR_BAD_NAME);
                    }
                }
                break;
            case WSS_ACTION_TYPE_INS:
            case WSS_ACTION_TYPE_UPD:
                value =  ((params[3] == NULL) && (*params[3] == EOS)) ?
                    "\0" : params[3];
                length = STlength(value);
                err = WSMAddVariable (act_session, params[2],
                    STlength (params[2]), value, length, level);
                break;

            default:
                err = DDFStatusAlloc(E_WS0061_WPS_BAD_SVR_ACTION);
                break;
        }
    }
    return(err);
}
