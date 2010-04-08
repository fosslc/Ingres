/******************************************************************************
**
**Copyright (c) 2004 Ingres Corporation
**
******************************************************************************/

#include <actsession.h>
#include <wsfvar.h>
#include <erwsf.h>
#include <wpsfile.h>
#include <wsmmo.h>
/*
**
**  Name: wsmvar.c - Web Session Manager Variables
**
**  Description:
**		permit access to different variable levels enabled by the system
**
**  History:    
**	5-Feb-98 (marol01)
**	    created
**      11-Sep-1998 (fanra01)
**          Corrected case of actsession to match piccolo.
**      28-Apr-2000 (fanra01)
**          Bug 100312
**          Add SSL support.  Add secure flag and scheme string to
**          WSMRequestActSession.  Secure flag in session structure can be 0
**          default port or 1 request on secure port.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	27-Aug-2003 (fanra01)    
**      Bug 110747
**      Memory associated with a tag need not be freed independently.
**/

static ACT_PSESSION active_sessions;
GLOBALDEF SEMAPHORE actSessSemaphore;

static WSF_VAR actDefaultVars[] =
{
	{HVAR_ROWCOUNT, "0" },
	{HVAR_STATUS_NUMBER, NULL_ERROR_STR },
	{HVAR_STATUS_TEXT, "" },
	{HVAR_STATUS_INFO, "" }
};

/*
** Name: WSMReleaseActSession() - Clean an active session
**
** Description:
**
** Inputs:
**	ACT_PSESSION* : active session
**
** Outputs:
**	ACT_PSESSION* : active session
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
**      03-Jul-98 (fanra01)
**          Rename wts_sess_detach to act_sess_detach.
**      26-Aug-2003 (fanra01)
**          Removed explicit memory frees which are part of tagged memory
**          free.
*/
GSTATUS 
WSMReleaseActSession(
	ACT_PSESSION	*act_session) 
{
	GSTATUS err = GSTAT_OK;
	ACT_PSESSION session = *act_session;
	if (session != NULL)
	{
		*act_session = NULL;
		if (session->name != NULL)
			act_sess_detach(session->name);

		err = DDFSemOpen(&actSessSemaphore, TRUE);
		if (err == GSTAT_OK)
		{
			if (session->previous != NULL)
				session->previous->next = session->next;
			else
				active_sessions = session->next;

			if (session->next != NULL)
				session->next->previous = session->previous;
			CLEAR_STATUS(DDFSemClose(&actSessSemaphore));
		}

		CLEAR_STATUS( WSMCleanUserTimeout() );
		CLEAR_STATUS( WPSCleanFilesTimeout(NULL) );
		CLEAR_STATUS( DBCleanTimeout() );

		CLEAR_STATUS(WSFCleanVariable(session->vars));
		CLEAR_STATUS (DDFrmhash(&session->vars));
		MEfreetag(session->mem_tag);

		MEfree((PTR)session);
	}
return(err);
}

/*
** Name: WSMRequestActSession() - create an active session
**
** Description:
**
** Inputs:
**	char*		: query text (name of the html file)
**	char*		: host (http server which requests that file)
**
** Outputs:
**	ACT_PSESSION* : active session
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
**      03-Jul-98 (fanra01)
**          Rename wts_sess_attach to act_sess_attach.
**      28-Apr-2000 (fanra01)
**          Add secure flag and scheme string to WSMRequestActSession.
**          Also use i4 instead of nats.
*/
GSTATUS
WSMRequestActSession(
    u_i4            clientType,
    char*           query,
    char*           secure,
    char*           scheme,
    char*           host,
    char*           port,
    USR_PSESSION    user_session,
    ACT_PSESSION    *act_session)
{
    GSTATUS err = GSTAT_OK;
    WSF_PVAR    var = NULL;
    ACT_PSESSION session = NULL;
    u_i4 length;
    u_i4 i;

    err = G_ME_REQ_MEM(0, session, ACT_SESSION, 1);
    if (err == GSTAT_OK)
    {
        session->mem_tag = MEgettag();

        if (err == GSTAT_OK)
        {
            err = G_ME_REQ_MEM(session->mem_tag, session->name, char, NUM_MAX_ADD_STR);
            if (err == GSTAT_OK)
                CVptrax( (PTR)session, session->name );
        }

        if (err == GSTAT_OK)
        {
            if (query != NULL)
            {
                length = STlength(query) + 1;
                err = G_ME_REQ_MEM(session->mem_tag, session->query, char, length);
                if (err == GSTAT_OK)
                    MECOPY_VAR_MACRO(query, length, session->query);
            }
            else
            {
                err = G_ME_REQ_MEM(session->mem_tag, session->query, char, 1);
                if (err == GSTAT_OK)
                    session->query[0] = EOS;
            }
        }

        if (err == GSTAT_OK)
        {
            session->secure = 0;
            /*
            ** if the flag is set convert its value to an integer and store
            ** in session structure.  Default is 0. 1 means secure.
            */
            if (secure != NULL && *secure != 0)
            {
                if (CVan( secure, &session->secure ) != OK)
                    session->secure = 0;
            }
        }

        if (err == GSTAT_OK)
        {
            if (scheme != NULL)
            {
                length = STlength( scheme ) + 1;
                err = G_ME_REQ_MEM( session->mem_tag, session->scheme, char,
                    length );
                if (err == GSTAT_OK)
                    MECOPY_VAR_MACRO( scheme, length, session->scheme );
            }
            else
            {
                /*
                ** if scheme is not passed as part of request use default
                */
                length = sizeof(STR_DEFAULT_SCHEME) + 1;
                err = G_ME_REQ_MEM( session->mem_tag, session->scheme, char,
                    length );
                if (err == GSTAT_OK)
                {
                    STcopy( STR_DEFAULT_SCHEME, session->scheme );
                }
            }
        }

        if (err == GSTAT_OK)
        {
            if (host != NULL)
            {
                length = STlength(host) + 1;
                err = G_ME_REQ_MEM(session->mem_tag, session->host, char, length);
                if (err == GSTAT_OK)
                    MECOPY_VAR_MACRO(host, length, session->host);
            }
            else
                session->host = NULL;
        }
        if (err == GSTAT_OK)
        {
            if (port != NULL)
            {
                length = STlength(port) + 1;
                err = G_ME_REQ_MEM(session->mem_tag, session->port, char, length);
                if (err == GSTAT_OK)
                    MECOPY_VAR_MACRO(port, length, session->port);
            }
            else
                session->port = NULL;
        }
        if (err == GSTAT_OK)
        {
            session->vars = NULL;
            err = DDFmkhash(20, &session->vars);
            if (err == GSTAT_OK)
            {
                for (i=0;
                     err == GSTAT_OK &&
                     i<(sizeof(actDefaultVars)/sizeof(WSF_VAR));
                     i++)
                    {
                        err = WSFCreateVariable(
                                session->mem_tag,
                                actDefaultVars[i].name,
                                STlength(actDefaultVars[i].name),
                                actDefaultVars[i].value,
                                (actDefaultVars[i].value == NULL) ?
                                    0 :
                                    STlength(actDefaultVars[i].value),
                                &var);

                        if (err == GSTAT_OK)
                            err = DDFputhash(
                                    session->vars,
                                    var->name,
                                    HASH_ALL,
                                    (PTR) var);
                    }
                if (err != GSTAT_OK)
                {
                    CLEAR_STATUS(WSFCleanVariable(session->vars));
                    CLEAR_STATUS(DDFrmhash(&session->vars));
                }
            }
        }

        if (err == GSTAT_OK)
        {
            session->type = WSM_NORMAL;
            session->error_counter = 0;
            session->user_session = user_session;
            session->clientType = clientType;
        }

        if (err == GSTAT_OK)
        {    /* IMA Stuff */
            STATUS status;
            if ((status = act_sess_attach(session->name, session)) != OK)
                err = DDFStatusAlloc(status);
        }

        if (err == GSTAT_OK)
        {
            err = DDFSemOpen(&actSessSemaphore, TRUE);
            if (err == GSTAT_OK)
            {
                session->next = active_sessions;
                if (active_sessions != NULL)
                    active_sessions->previous = session;
                active_sessions = session;
                CLEAR_STATUS(DDFSemClose(&actSessSemaphore));
            }
        }
    }

    if (err != GSTAT_OK)
    {
        CLEAR_STATUS(WSMReleaseActSession(&session));
    }
    else
        *act_session = session;
    return(err);
}

/*
** Name: WSMGetActiveSession() - fetch
**
** Description:
**
** Inputs:
**	PTR*		: cursor
**
** Outputs:
**	char**		: session key
**	char**		: attached user session
**	char**		: html file
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
WSMBrowseActiveSession(
	PTR		*cursor,
	ACT_PSESSION *session)
{
	GSTATUS err = GSTAT_OK;

	if (*cursor == NULL)
	{
		err = DDFSemOpen(&actSessSemaphore, FALSE);
		if (err == GSTAT_OK)
		{
			*cursor = (PTR) active_sessions;
			*session = active_sessions;
		}
	}
	else
	{
		*session = (ACT_PSESSION) *cursor;
		*session = (*session)->next;
		*cursor = (PTR) *session;
	}

	if (*session == NULL)
		CLEAR_STATUS(DDFSemClose(&actSessSemaphore));
return(err);
}
