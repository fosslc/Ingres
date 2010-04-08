/******************************************************************************
**
**Copyright (c) 2004 Ingres Corporation
**
******************************************************************************/

#include <wsf.h>
#include <dds.h>
#include <erwsf.h>
#include <servermeth.h>
#include <usrsession.h>
#include <dbaccess.h>
#include <wpsfile.h>
#include <wcsdoc.h>

/*
**
**  Name: wmo_monitor.c - Web Monitoring Object
**
**  Description:
**	that file is composed of macro server function to monitor the system
**		cache
**		user sessions
**		active sessions
**
**  History:    
**	5-Feb-98 (marol01)
**	    created
**      07-Sep-1998 (fanra01)
**          Corrected case for usrsession to match piccolo.
**          Enclosed MECOPY_VAR_MACRO within braces.
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

GSTATUS
WMOTypeAction (
	char* action,
	u_i4 *type);

/*
** Name: WMOCheckMonitor() - 
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
*/
GSTATUS
WMOCheckMonitor (
    ACT_PSESSION act_session)
{
    GSTATUS        err = GSTAT_OK;

    if (act_session->user_session == NULL)
    {
        err = DDFStatusAlloc( E_WS0101_WSS_UNAUTHORIZED_USE );
        return(err);
    }
    if (act_session->user_session->userid != SYSTEM_ID)
    {
        bool    profile;
        err = DDSCheckUserFlag (
                    act_session->user_session->userid,
                    WSF_USR_ADM | WSF_USR_MONIT,
                    &profile);

        G_ASSERT(profile == FALSE, E_WS0102_WSS_INSUFFICIENT_PERM);
    }
    return(err);
}

/*
** Name: WMOCache() - delete, select Cached files
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
*/
GSTATUS
WMOCache (
	ACT_PSESSION act_session, 
	char* params[],
	bool* print,
	PTR*  user_info)
{
	GSTATUS		err = GSTAT_OK;
	u_i4		action;
	WPS_PFILE	file = NULL;

	*print = FALSE;
	if (*user_info == NULL)
	{
		G_ASSERT (params[0] == NULL || params[0][0] == EOS, E_WS0059_WSF_BAD_PARAM);

		if (err == GSTAT_OK)
			err = WMOTypeAction(params[0], &action);

		if (err == GSTAT_OK)
			err = WMOCheckMonitor(act_session);
		CLEAR_STATUS( WPSCleanFilesTimeout(NULL) );
	}
	else
		action = WSS_ACTION_TYPE_SEL;
	
	if (err == GSTAT_OK)
		switch(action)
		{
		case WSS_ACTION_TYPE_SEL:
			{
			err = WPSBrowseFiles(
						user_info,
						&file);

			if (err == GSTAT_OK && file != NULL)
			{
				u_i4 length;
				err = GAlloc((PTR*)&params[1], NUM_MAX_ADD_STR, FALSE);
				if (err == GSTAT_OK) 
				{
					CVptrax( (PTR)file, params[1] );

					if (file->file->doc_id == SYSTEM_ID)
					{
						length = STlength(file->file->info->name) + 1;
						err = GAlloc((PTR*)&params[2], length, FALSE);
						if (err == GSTAT_OK) 
								MECOPY_VAR_MACRO(file->file->info->name, length, params[2]);
					}
					else
					{
						char *prefix = NULL;
						char *suffix = NULL;
						err = WCSGetDocFromId (
											file->file->doc_id,
											NULL,
											&prefix,
											&suffix,
											NULL,
											NULL,
											NULL,
											NULL,
											NULL);
						if (err == GSTAT_OK) 
						{
							length = STlength(prefix) + STlength(suffix) + 2;
							err = GAlloc((PTR*)&params[2], length, FALSE);
							if (err == GSTAT_OK) 
								STprintf(params[2], "%s%c%s", prefix, CHAR_FILE_SUFFIX_SEPARATOR, suffix);
						}
						MEfree((PTR)prefix);
						MEfree((PTR)suffix);
					}
					if (err == GSTAT_OK) 
					{
						if (file->file->info->location != NULL)
							length = STlength(file->file->info->location->name) + 1;
						else
							length = 1;

						err = GAlloc((PTR*)&params[3], length, FALSE);
						if (err == GSTAT_OK) 
						{
							if (file->file->info->location != NULL)
							{
								MECOPY_VAR_MACRO(file->file->info->location->name, length, params[3]);
							}
							else
							{
								params[3][0] = EOS;
							}

							err = GAlloc((PTR*)&params[4], sizeof(file->file->info->status), FALSE);
							if (err == GSTAT_OK) 
							{
								err = GAlloc((PTR*)&params[4], NUM_MAX_INT_STR, FALSE);
								if (err == GSTAT_OK)
								{
									WSF_CVNA(err, file->file->info->status, params[4]);
									err = GAlloc((PTR*)&params[5], NUM_MAX_INT_STR, FALSE);
									if (err == GSTAT_OK)
									{
										length = STlength(WSF_CHECKED) + 1;
										WSF_CVNA(err, file->file->info->counter, params[5]);
										err = GAlloc((PTR*)&params[6], length, FALSE);
										if (err == GSTAT_OK && file->file->info->exist == TRUE)
{
											MECOPY_VAR_MACRO(WSF_CHECKED, length, params[6]);
										}
										else
{
											params[6][0] = EOS;
										}
									}
								}
							}
						}
					}
				}
				if (err == GSTAT_OK) 
				{
					length = STlength(file->key) + 1;
					err = GAlloc((PTR*)&params[7], length, FALSE);
					if (err == GSTAT_OK) 
					{
						MECOPY_VAR_MACRO(file->key, length, params[7]);
						err = GAlloc((PTR*)&params[8], NUM_MAX_INT_STR, FALSE);
						if (err == GSTAT_OK)
						{
							length = STlength(WSF_CHECKED) + 1;
							WSF_CVLA(err, 
									(file->timeout_end > 0) ?
										file->timeout_end - TMsecs():
										0, 
									params[8]);
							err = GAlloc((PTR*)&params[9], length, FALSE);
							if (err == GSTAT_OK)
							{
								if (file->used == TRUE)
{
									MECOPY_VAR_MACRO(WSF_CHECKED, length, params[9]);
								}
								else
								{
									params[9][0] = EOS;
								}
								err = GAlloc((PTR*)&params[10], NUM_MAX_INT_STR, FALSE);
								if (err == GSTAT_OK)
								{
									WSF_CVNA(err, file->count, params[10]);
									*print = TRUE;
								}
							}
						}
					}
				}
			}
			}
			break;
		case WSS_ACTION_TYPE_UPD:
			G_ASSERT (params[1] == NULL || CVaxptr(params[1], (PTR*)&file) != OK, E_WS0059_WSF_BAD_PARAM);
			err = WPSRefreshFile (file, TRUE);
			break;
		case WSS_ACTION_TYPE_DEL:
			G_ASSERT (params[1] == NULL || CVaxptr(params[1], (PTR*)&file) != OK, E_WS0059_WSF_BAD_PARAM);
			err = WPSRemoveFile (&file, TRUE);
			break;
		default: 
			err = DDFStatusAlloc(E_WS0061_WPS_BAD_SVR_ACTION);
		}
return(err);	
}	

/*
** Name: WMOUserSession() - delete, select User sessions
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
*/
GSTATUS
WMOUserSession (
	ACT_PSESSION act_session, 
	char* params[],
	bool* print,
	PTR*  user_info)
{
	GSTATUS		err = GSTAT_OK;
	u_i4		action;
	USR_PSESSION session;

	*print = FALSE;
	if (*user_info == NULL)
	{
		G_ASSERT (params[0] == NULL || params[0][0] == EOS, E_WS0059_WSF_BAD_PARAM);

		if (err == GSTAT_OK)
			err = WMOTypeAction(params[0], &action);

		if (err == GSTAT_OK)
			err = WMOCheckMonitor(act_session);

		CLEAR_STATUS( WSMCleanUserTimeout() );
	}
	else
		action = WSS_ACTION_TYPE_SEL;
	
	if (err == GSTAT_OK)
		switch(action)
		{
		case WSS_ACTION_TYPE_SEL:
			{
				err = WSMBrowseUsers(
						user_info,
						&session);

				if (err == GSTAT_OK && session != NULL)
				{
					u_i4 length = STlength(session->name) + 1;
					err = GAlloc((PTR*)&params[1], length, FALSE);
					if (err == GSTAT_OK) 
					{
						char* user = NULL;
						MECOPY_VAR_MACRO(session->name, length, params[1]);
						if (session->userid == SYSTEM_ID)
							user = session->user;
						else
							err = DDSGetUserName(session->userid, &user);
						if (err == GSTAT_OK)
						{
							length = STlength(user) + 1;
							err = GAlloc((PTR*)&params[2], length, FALSE);
							if (err == GSTAT_OK) 
							{
								MECOPY_VAR_MACRO(user, length, params[2]);
								err = GAlloc((PTR*)&params[3], NUM_MAX_INT_STR, FALSE);
								if (err == GSTAT_OK)
								{
									WSF_CVNA(err, session->requested_counter, params[3]);
									err = GAlloc((PTR*)&params[4], NUM_MAX_INT_STR, FALSE);
									if (err == GSTAT_OK)
									{
										WSF_CVLA(err, session->timeout_end - TMsecs(), params[4]);
										*print = TRUE;
									}
								}
							}
						}
					}
				}
			}
			break;
		case WSS_ACTION_TYPE_DEL:
			G_ASSERT (params[1] == NULL, E_WS0059_WSF_BAD_PARAM);
			err = WSMRequestUsrSession(params[1], &session);
			if (err == GSTAT_OK && session != NULL)
			{
				err = WSMRemoveUsrSession (session);
				CLEAR_STATUS(WSMReleaseUsrSession(session));
			}
			break;
		default: 
			err = DDFStatusAlloc(E_WS0061_WPS_BAD_SVR_ACTION);
		}
return(err);	
}	

/*
** Name: WMOActiveSession() - delete, select Active sessions
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
*/
GSTATUS
WMOActiveSession (
	ACT_PSESSION act_session, 
	char* params[],
	bool* print,
	PTR*  user_info)
{
	GSTATUS		err = GSTAT_OK;
	u_i4		action;
	ACT_PSESSION session;

	*print = FALSE;
	if (*user_info == NULL)
	{
		G_ASSERT (params[0] == NULL || params[0][0] == EOS, E_WS0059_WSF_BAD_PARAM);

		if (err == GSTAT_OK)
			err = WMOTypeAction(params[0], &action);

		if (err == GSTAT_OK)
			err = WMOCheckMonitor(act_session);
	}
	else
		action = WSS_ACTION_TYPE_SEL;
	
	if (err == GSTAT_OK)
		switch(action)
		{
		case WSS_ACTION_TYPE_SEL:
			{
				err = WSMBrowseActiveSession(
						user_info,
						&session);

				if (err == GSTAT_OK && session != NULL)
				{
					u_i4 length = STlength(session->name) + 1;
					err = GAlloc((PTR*)&params[1], length, FALSE);
					if (err == GSTAT_OK)
					{
						MECOPY_VAR_MACRO(session->name, length, params[1]);
						length = STlength(session->user_session->name) + 1;
						err = GAlloc((PTR*)&params[2], length, FALSE);
						if (err == GSTAT_OK)
						{
							MECOPY_VAR_MACRO(session->user_session->name, length, params[2]);
							length = STlength(session->query) + 1;
							err = GAlloc((PTR*)&params[3], length, FALSE);
							if (err == GSTAT_OK) 
							{
								MECOPY_VAR_MACRO(session->query, length, params[3]);
								length = STlength(session->host) + 1;
								err = GAlloc((PTR*)&params[4], length, FALSE);
								if (err == GSTAT_OK)
								{
									MECOPY_VAR_MACRO(session->host, length, params[4]);
									err = GAlloc((PTR*)&params[5], NUM_MAX_INT_STR, FALSE);
									if (err == GSTAT_OK)
									{
										WSF_CVNA(err, session->error_counter, params[5]);
										*print = TRUE;
									}
								}
							}
						}
					}
				}
			}
			break;
		default: 
			err = DDFStatusAlloc(E_WS0061_WPS_BAD_SVR_ACTION);
		}
return(err);	
}	

/*
** Name: WMOUserTransaction() - 
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
*/
GSTATUS
WMOUserTransaction (
	ACT_PSESSION act_session, 
	char* params[],
	bool* print,
	PTR*  user_info)
{
	GSTATUS		err = GSTAT_OK;
	u_i4		action;
	USR_PTRANS  trans = NULL;
	u_i4		length;

	*print = FALSE;
	if (*user_info == NULL)
	{
		G_ASSERT (params[0] == NULL || params[0][0] == EOS, E_WS0059_WSF_BAD_PARAM);

		if (err == GSTAT_OK)
			err = WMOTypeAction(params[0], &action);

		if (err == GSTAT_OK)
			err = WMOCheckMonitor(act_session);
	}
	else
		action = WSS_ACTION_TYPE_SEL;
	
	if (err == GSTAT_OK)
		switch(action)
		{
		case WSS_ACTION_TYPE_SEL:
			{
				err = WSMBrowseTransaction(
						user_info,
						&trans);
	
				if (err == GSTAT_OK && trans != NULL)
				{
					err = GAlloc((PTR*)&params[1], NUM_MAX_ADD_STR, FALSE);
					if (err == GSTAT_OK) 
					{
						CVptrax( (PTR)trans, params[1] );
						length = STlength(trans->trname) + 1;
						err = GAlloc((PTR*)&params[2], length, FALSE);
						if (err == GSTAT_OK) 
						{
							MECOPY_VAR_MACRO(trans->trname, length, params[2]);
							length = STlength(trans->usr_session->name) + 1;
							err = GAlloc((PTR*)&params[3], length, FALSE);
							if (err == GSTAT_OK) 
							{
								MECOPY_VAR_MACRO(trans->usr_session->name, length, params[3]);
								err = GAlloc((PTR*)&params[4], NUM_MAX_ADD_STR, FALSE);
								if (err == GSTAT_OK) 
								{
									CVptrax( (PTR)trans->session, params[4] );
									*print = TRUE;
								}
							}
						}
					}
				}
			}
			break;
		case WSS_ACTION_TYPE_DEL:
			G_ASSERT (params[1] == NULL || CVaxptr(params[1], (PTR*)&trans) != OK, E_WS0059_WSF_BAD_PARAM);
			err = WSMRollback(trans, TRUE);
			break;
		default: 
			err = DDFStatusAlloc(E_WS0061_WPS_BAD_SVR_ACTION);
		}
return(err);	
}	


/*
** Name: WMOUserCursor() - 
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
*/
GSTATUS
WMOUserCursor (
	ACT_PSESSION act_session, 
	char* params[],
	bool* print,
	PTR*  user_info)
{
	GSTATUS		err = GSTAT_OK;
	u_i4		action;
	USR_PCURSOR	object;

	*print = FALSE;
	if (*user_info == NULL)
	{
		G_ASSERT (params[0] == NULL || params[0][0] == EOS, E_WS0059_WSF_BAD_PARAM);

		if (err == GSTAT_OK)
			err = WMOTypeAction(params[0], &action);

		if (err == GSTAT_OK)
			err = WMOCheckMonitor(act_session);
	}
	else
		action = WSS_ACTION_TYPE_SEL;
	
	if (err == GSTAT_OK)
		switch(action)
		{
		case WSS_ACTION_TYPE_SEL:
			{
				err = WSMBrowseCursors(
						user_info,
						&object);

				if (err == GSTAT_OK && object != NULL)
				{
					u_i4 length;
					err = GAlloc((PTR*)&params[1], NUM_MAX_ADD_STR, FALSE);
					if (err == GSTAT_OK) 
					{
						CVptrax( (PTR)object, params[1] );
						length = STlength(object->name) + 1;
						err = GAlloc((PTR*)&params[2], length, FALSE);
						if (err == GSTAT_OK) 
						{
							char *statement = NULL;
							MECOPY_VAR_MACRO(object->name, length, params[2]);
							err = DBGetStatement(object->transaction->session->driver)(&object->query, &statement);
							if (err == GSTAT_OK && statement != NULL) 
							{
								length = STlength(statement) + 1;
								err = GAlloc((PTR*)&params[3], length, FALSE);
								if (err == GSTAT_OK) 
								{
									MECOPY_VAR_MACRO(statement, length, params[3]);
									err = GAlloc((PTR*)&params[4], NUM_MAX_ADD_STR, FALSE);
									if (err == GSTAT_OK) 
									{
										CVptrax( (PTR)object->transaction, params[4] );
										*print = TRUE;
									}
								}
							}
						}
					}
				}
			}
			break;
		case WSS_ACTION_TYPE_DEL:
			G_ASSERT (params[1] == NULL || CVaxptr(params[1], (PTR*)&object) != OK, E_WS0059_WSF_BAD_PARAM);
			err = WSMDestroyCursor(object, NULL, TRUE);
			break;
		default: 
			err = DDFStatusAlloc(E_WS0061_WPS_BAD_SVR_ACTION);
		}
return(err);	
}	

/*
** Name: WMOConnection() - 
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
*/
GSTATUS
WMOConnection (
	ACT_PSESSION act_session, 
	char* params[],
	bool* print,
	PTR*  user_info)
{
	GSTATUS		err = GSTAT_OK;
	u_i4		action;
	PDB_CACHED_SESSION conn;
	*print = FALSE;
	if (*user_info == NULL)
	{
		G_ASSERT (params[0] == NULL || params[0][0] == EOS, E_WS0059_WSF_BAD_PARAM);

		if (err == GSTAT_OK)
			err = WMOTypeAction(params[0], &action);

		if (err == GSTAT_OK)
			err = WMOCheckMonitor(act_session);

		CLEAR_STATUS( DBCleanTimeout() );
	}
	else
		action = WSS_ACTION_TYPE_SEL;
	
	if (err == GSTAT_OK)
		switch(action)
		{
		case WSS_ACTION_TYPE_SEL:
			{
				err = DBBrowse(
						user_info,
						&conn);

				if (err == GSTAT_OK && conn != NULL)
				{
					u_i4 length;
					err = GAlloc((PTR*)&params[1], NUM_MAX_ADD_STR, FALSE);
					if (err == GSTAT_OK) 
					{
						char *buffer;
						CVptrax( (PTR)conn, params[1] );
						err = DBDriverName(conn->driver)(&buffer);
						if (err == GSTAT_OK) 
						{
							length = STlength(buffer) + 1;
							err = GAlloc((PTR*)&params[2], length, FALSE);
							if (err == GSTAT_OK) 
							{
								MECOPY_VAR_MACRO(buffer, length, params[2]);
								length = STlength(conn->set->name) + 1;
								err = GAlloc((PTR*)&params[3], length, FALSE);
								if (err == GSTAT_OK) 
								{
									MECOPY_VAR_MACRO(conn->set->name, length, params[3]);
									err = GAlloc((PTR*)&params[4], NUM_MAX_INT_STR, FALSE);
									if (err == GSTAT_OK)
									{
										WSF_CVNA(err, conn->used, params[4]);
										err = GAlloc((PTR*)&params[5], NUM_MAX_INT_STR, FALSE);
										if (err == GSTAT_OK)
										{
											i4 timeout = (conn->timeout_end > 0) ?
																conn->timeout_end - TMsecs() :
																0;
											WSF_CVLA(err, timeout, params[5]);
											*print = TRUE;
										}
									}
								}
							}
						}
					}
				}
			}
			break;
		case WSS_ACTION_TYPE_DEL:
			G_ASSERT (params[1] == NULL || CVaxptr(params[1], (PTR*)&conn) != OK, E_WS0059_WSF_BAD_PARAM);
			err = DBRemove (conn, TRUE);
			break;
		default: 
			err = DDFStatusAlloc(E_WS0061_WPS_BAD_SVR_ACTION);
		}
return(err);	
}	
