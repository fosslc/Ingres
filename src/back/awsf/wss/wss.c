/******************************************************************************
**
**Copyright (c) 2004 Ingres Corporation
**
******************************************************************************/

#include <wsf.h>
#include <wss.h>
#include <wps.h>
#include <ddfcom.h>
#include <dds.h>
#include <wsspars.h>
#include <erwsf.h>
#include <wsmvar.h>
#include <wpsfile.h>
#include <wssprofile.h>
#include <wssapp.h>
#include <wsslogin.h>
#include <servermeth.h>

# include <asct.h>

/*
**
**  Name: wss.c - Web Security Services
**
**  Description:
**		Openning Session Control
**
**  History:    
**	5-Feb-98 (marol01)
**	    created
**      11-Sep-1998 (fanra01)
**          Fixup compiler warnings on unix.
**      02-Nov-98 (fanra01)
**          Add remote address and remote host.
**      19-Nov-1998 (fanra01)
**          Remove memory leak.
**      20-Nov-1998 (fanra01)
**          Add case flag for ice user names.
**      15-Dec-1998 (fanra01)
**          Add attempt to read cookie variable ii_cookie if there is no
**          session information.  This is to enable the ability to execute
**          a procedure from within a 2.5 session.
**      07-Jan-1999 (fanra01)
**          Add authtype for user creation.
**      04-Mar-1999 (fanra01)
**          Add trace messages.
**      24-Mar-1999 (fanra01)
**          Change trace message to use string description.
**      12-Apr-1999 (fanra01)
**          Prevent the autoregistered user from being an empty string.
**      13-May-1999 (fanra01)
**          Add user number as HTML variable for the session.
**      03-Feb-2000 (fanra01)
**          Bug 100360
**          Updated the WSSDocAccessibility to treat rights as bit fields.
**          Ensure that READ and EXECUTE are exclusive.
**      28-Apr-2000 (fanra01)
**          Bug 100312
**          Add SSL support.  Add secure flag and scheme string to
**          WSSOpenSession for use with WSMRequestActSession.
**          secure and scheme should come from the client as part of the
**          request.
**      05-May-2000 (fanra01)
**          Bug 101346
**          Modify the unit test within the function to test document
**          accessibility.  When testing flags other than execute the
**          general function is used which test to see if the reference count
**          indicates specific document access for the user.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      11-Oct-2000 (fanra01)
**          Sir 103096
**          Add a call to retrieve the file extension for the page.  Used
**          for the generation of the content type.
**      20-Nov-00 (fanra01)
**          Bug 103275
**          Handle content type when no session is associated.
**      20-Mar-2001 (fanra01)
**          Correct function spelling WCSDispatchName.
**      23-Mar-2001 (fanra01)
**          Bug 104321
**          Use root location if no unit or location specified.
**          Add additional tests for uninitialised parameters from the client.
**      17-Jul-2001 (fanra01)
**          Bug 105254
**          Add handling of content type for public registered pages.
**      01-Oct-2001 (fanra01)
**          Bug 105919
**          Test explicitly for .htm extension when returning the mime
**          extension to ensure text/html is returned.
**      30-Oct-2001 (fanra01)
**          Bug 106216
**          Add information text, if it exists, to the error message buffer.
**	13-May-2005 (kodse01)
**	    replace %ld with %d for old nat and long nat variables.
**/

GLOBALDEF char*		privileged_user = NULL;
GLOBALDEF bool          ignore_user_case = FALSE;

GSTATUS WSSMemGetProfile(char* name, WSS_PPROFILE *profile);

/*
** Name: WSSPerformCookie() - Generate a new cookie
**
** Description:
**
** Inputs:
**	u_i4	: userid, 
**	char*	: user
**
** Outputs:
**	char*	: cookie
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
WSSPerformCookie (
	char* *cookie, 
	u_i4 userid, 
	char*	user) 
{
	GSTATUS err = GSTAT_OK;
  i4 ltime;
    
	ltime = TMsecs();
	if (userid == SYSTEM_ID && user != NULL)
	{
		err = GAlloc(cookie, NUM_MAX_INT_STR + STlength(user) + 5, FALSE);
		if (err == GSTAT_OK)
			STprintf(*cookie, "ICE%s_%d", user, ltime );
	}
	else
	{
		err = GAlloc(cookie, (2*NUM_MAX_INT_STR) + 5, FALSE);
		if (err == GSTAT_OK)
			sprintf(*cookie, "ICE%d_%d", userid, ltime );
	}
return(err);
}

/*
** Name: WSSCloseSession() - Close an active session
**
** Description:
**
** Inputs:
**	ACT_PSESSION *act_session
**	bool		 force
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
**      24-Mar-1999 (fanra01)
**          Change trace message to use string description.
*/
GSTATUS 
WSSCloseSession (
    ACT_PSESSION *act_session,
    bool         force)
{
    GSTATUS err = GSTAT_OK;
    USR_PSESSION session = (*act_session)->user_session;
    (*act_session)->user_session = NULL;

    asct_trace( ASCT_LOG )( ASCT_TRACE_PARAMS,
        "Close session=%s force=%s type=%w",
        ((session) ? session->name : ""), ((force == TRUE) ? "TRUE" : "FALSE"),
        "NORMAL,CONNECT,DISCONNECT,AUTO,DOWNLOAD", (*act_session)->type );

    if (session != NULL)
        err = WSMReleaseUsrSession(session);

    if (force == TRUE ||
        (session != NULL && (*act_session)->type == WSM_DISCONNECT))
    {
        CLEAR_STATUS(WSMDeleteUsrSession (&session));
    }

    CLEAR_STATUS(WSMReleaseActSession (act_session));
    return(err);
}

/*
** Name: WSSCreateUsrSession() - Create an user session
**
** Description:
**
** Inputs:
**	ACT_PSESSION *act_session
**	char*	user
**	char*	password
**
** Outputs:
**	USR_PSESSION *usr_session
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
WSSCreateUsrSession (
    ACT_PSESSION    act_session,
    char*           user,
    char*           password,
    USR_PSESSION    *usr_session)
{
    GSTATUS err = GSTAT_OK;
    u_i4   userid = SYSTEM_ID;

    G_ASSERT(user == NULL, E_DF0044_DDS_BAD_USER);
    if (err == GSTAT_OK)
    {
        if (ignore_user_case == TRUE)
        {
            CVlower (user);
        }
        if (privileged_user != NULL &&
            STbcompare(privileged_user, 0, user, 0, ignore_user_case) == 0)
        {
            CL_ERR_DESC sys_err;
            if (GCusrpwd(user, password, &sys_err) != OK)
                err = DDFStatusAlloc(E_DF0043_DDS_BAD_PASSWORD);
        }
        else
        {
            if (password == NULL)
                err = DDFStatusAlloc(E_DF0043_DDS_BAD_PASSWORD);
            else
                err = DDSCheckUser(user, password, &userid);
        }
    }

    if (err == GSTAT_OK)
    {
        char* new_cookie = NULL;
        err = WSSPerformCookie(
                                            &new_cookie,
                                            userid,
                                            user);
        if (err == GSTAT_OK)
            err = WSMCreateUsrSession(
                                new_cookie,
                                userid,
                                user,
                                password,
                                usr_session);
        MEfree((PTR)new_cookie);
    }
    return(err);
}

/*
** Name: WSSAutoDeclare() - Create a new user through a profile
**
** Description:
**
** Inputs:
**	ACT_PSESSION *act_session
**	char*	user
**	char*	password
**
** Outputs:
**	USR_PSESSION *usr_session
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
**      07-Jan-1999 (fanra01)
**          Add authtype. Default to repository authentication if not defined.
**      12-Apr-1999 (fanra01)
**          Prevent the autoregistered user from being an empty string.
*/
GSTATUS
WSSAutoDeclare (
    ACT_PSESSION    act_session,
    char*           user,
    char*           password)
{
    GSTATUS         err = GSTAT_OK;
    char*           name = NULL;
    WSS_PPROFILE    profile;
    u_i4           id;
    char*           auth;
    i4         authtype = DDS_REPOSITORY_AUTH;

    G_ASSERT( (user == NULL) || (*user == EOS),
        E_WS0096_WSS_INVALID_USER_NAME );

    if (ignore_user_case == TRUE)
    {
        CVlower (user);
    }
    err = WSMGetVariable( act_session, HVAR_PROFILE, STlength(HVAR_PROFILE),
        &name, WSM_ACTIVE);

    if (name != NULL &&
        name[0] != EOS)
        err = WSSMemGetProfile(name, &profile);

    if (err == GSTAT_OK)
    {
        err = WSMGetVariable( act_session, HVAR_AUTHTYPE,
            STlength(HVAR_AUTHTYPE), &auth, WSM_ACTIVE);
        if ((err == GSTAT_OK) && auth && *auth)
        {
            if (STbcompare (auth, 0, STR_OS_AUTH, 0, TRUE) == 0)
            {
                authtype = DDS_OS_USRPWD_AUTH;
            }
        }
    }
    if (err == GSTAT_OK && profile != NULL)
    {
        WSS_ID_LIST    *dbs = profile->dbs;
        WSS_ID_LIST    *roles = profile->roles;
        err = DDSCreateUser (
                user,
                password,
                profile->dbuser_id,
                NULL,
                profile->flags,
                profile->timeout,
                authtype,
                &id);

        while (err == GSTAT_OK && dbs != NULL)
        {
            err = DDSCreateAssUserDB (id, dbs->id);
            dbs = dbs->next;
        }
        while (err == GSTAT_OK && roles != NULL)
        {
            err = DDSCreateAssUserRole (id, roles->id);
            roles = roles->next;
        }
        asct_trace( ASCT_LOG )( ASCT_TRACE_PARAMS,
            "Declare status=%8x user=%s profile=%s authtype=%s",
            ((err == NULL) ? 0 : err->number), user, name,
            ((authtype == 0) ? "ICE" : "OS") );
    }
    return(err);
}

/*
** Name: WSSErrorURL() - Open an active session
**
** Description:
**
** Inputs:
**	char*	query
**	char*	host
**	char*	cookie
**	char*	variable
**	char*	auth_type
**	char*	gateway
**
** Outputs:
**	ACT_PSESSION *act_session
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
WSSErrorURL (
	ACT_PSESSION act_session,
	char*		 url,
	STATUS		 number) 
{
	GSTATUS				err = GSTAT_OK;
	char buf[NUM_MAX_INT_STR];
	char *msgError;
	char szMsg[ERROR_BLOCK];

	MEfree((PTR)act_session->query);
	act_session->query = STalloc(url);
	CVna(number, buf);

	err = WSMGetVariable (
						act_session, 
						HVAR_ERROR_MSG, 
						STlength(HVAR_ERROR_MSG), 
						&msgError, 
						WSM_SYSTEM | WSM_ACTIVE | WSM_USER);

	if (msgError == NULL || msgError[0] == EOS)
	{
		CL_ERR_DESC clError;
		i4 nMsgLen;	
			
		ERslookup (
					number, 
					NULL, 
					0, 
					NULL, 
					szMsg, 
					ERROR_BLOCK,
					-1, 
					&nMsgLen, 
					&clError, 
					0, 
					NULL);
		szMsg[nMsgLen] = EOS;
		msgError = szMsg;
	}

	if (err == GSTAT_OK)
		err = WSMAddVariable(
						act_session,
						HVAR_STATUS_NUMBER,
						STlength(HVAR_STATUS_NUMBER), 
						buf,
						STlength(buf),
						WSM_ACTIVE);

	if (err == GSTAT_OK)
		err = WSMAddVariable(
						act_session,
						HVAR_STATUS_TEXT,
						STlength(HVAR_STATUS_TEXT), 
						msgError,
						STlength(msgError),
						WSM_ACTIVE);
return(err);
}

/*
** Name: WSSOpenSession() - Open an active session
**
** Description:
**
** Inputs:
**	char*	query
**	char*	host
**	char*	cookie
**	char*	variable
**	char*	auth_type
**	char*	gateway
**
** Outputs:
**	ACT_PSESSION *act_session
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
**      02-Nov-98 (fanra01)
**          Add remote address and remote host.
**          Add check to enter auto declare only if error is not invalid
**          password.
**      13-May-1999 (fanra01)
**          Add user number as HTML variable for the session.
**      28-Apr-2000 (fanra01)
**          Add secure flag and scheme string for use in WSMRequestActSession.
**      23-Mar-2001 (fanra01)
**          Add tests of parameters from client before using them.  If the
**          client is run with incorrect or no parameters these cause an
**          exception.
*/
GSTATUS
WSSOpenSession (
    u_i4   clientType,
    char    *query,
    char    *secure,
    char    *scheme,
    char    *host,
    char    *port,
    char    *variable,
    char    *auth_type,
    char    *gateway,
    char    *agent,
    char    *rmt_addr,
    char    *rmt_host,
    USR_PSESSION usr_session,
    ACT_PSESSION *session)
{
    GSTATUS         err=GSTAT_OK;
    ACT_PSESSION    act_session= NULL;
    char*           action;
    u_i4           userid = SYSTEM_ID;
    char*           user = NULL;
    char*           password = NULL;

    err = WSMRequestActSession(
        clientType,
        query,
        secure,
        scheme,
        host,
        port,
        usr_session,
        &act_session);

    if (err == GSTAT_OK)
    {
        err = ParseHTMLVariables( act_session, variable);

        if (err == GSTAT_OK)
        {
            err = WSMGetVariable(
                act_session,
                HVAR_ACTION,
                STlength(HVAR_ACTION),
                &action,
                WSM_ACTIVE);
        }

        if (err == GSTAT_OK && action != NULL && action[0] != EOS)
        {
            if (STcompare(action, CONNECT_STR) == 0)
                act_session->type = WSM_CONNECT;
            else if (STcompare(action, DISCONNECT_STR) == 0)
                act_session->type = WSM_DISCONNECT;
            else if (STcompare(action, AUTO_DECLARE_STR) == 0)
                act_session->type = WSM_AUTO_DECL;
            else if (STcompare(action, DOWNLOAD_STR) == 0)
                act_session->type = WSM_DOWNLOAD;
        }

        if (err == GSTAT_OK)
        {
            if (act_session->type == WSM_AUTO_DECL ||
                act_session->type == WSM_CONNECT)
            {
                if (auth_type != NULL && auth_type[0] != EOS)
                    err = AllocUserPass(auth_type, &user, &password);

                if (err == GSTAT_OK && user == NULL)
                {
                    char* tmp;

                    err = WSMGetVariable(
                        act_session,
                        HVAR_USERID,
                        STlength(HVAR_USERID),
                        &tmp,
                        WSM_ACTIVE);

                    if (err == GSTAT_OK && tmp != NULL)
                    {
                        err =  G_ST_ALLOC(user, tmp);
                        if (err == GSTAT_OK)
                            err = WSMDelVariable(
                                act_session,
                                HVAR_USERID,
                                STlength(HVAR_USERID),
                                WSM_ACTIVE);

                        if (err == GSTAT_OK)
                            err = WSMGetVariable(
                                act_session,
                                HVAR_PASSWORD,
                                STlength(HVAR_PASSWORD),
                                &tmp,
                                WSM_ACTIVE);

                        if (err == GSTAT_OK)
                            err =  G_ST_ALLOC(password, tmp);

                        if (err == GSTAT_OK)
                            err = WSMDelVariable(
                                act_session,
                                HVAR_PASSWORD,
                                STlength(HVAR_PASSWORD),
                                WSM_ACTIVE);
                    }
                }
            }
            else
            {
                if (act_session->type != WSM_DISCONNECT)
                {
                    if ((act_session->clientType & CONNECTED) &&
                        (usr_session == NULL))
                    {
                        err = DDFStatusAlloc( E_WS0077_WSS_TIMEOUT );
                    }
                    else
                    {
                        /*
                        ** no previous session, attempt to read a cookie
                        ** variable
                        */
                        if (usr_session == NULL)
                        {
                            char*   cookie = NULL;

                            if ((err = WSMGetVariable(
                                act_session,
                                HVAR_COOKIE,
                                STlength(HVAR_COOKIE),
                                &cookie,
                                WSM_ACTIVE)) == GSTAT_OK)
                            {
                                if (cookie && *cookie)
                                {
                                    err = WSMRequestUsrSession(cookie,
                                        &usr_session);
                                    if (err == GSTAT_OK)
                                    {
                                        act_session->user_session=usr_session;
                                    }
                                }
                            }
                        }
                    }
                }
                else
                {
                    asct_trace( ASCT_LOG )( ASCT_TRACE_PARAMS,
                        "Disconnect status=%8x session=%s",
                        ((err == NULL) ? 0 : err->number),
                        ((act_session->user_session) ?
                            act_session->user_session->name : "ICE2.0") );
                }
            }
        }
        if (err == GSTAT_OK && act_session->type == WSM_AUTO_DECL)
        {
            err = WSSAutoDeclare(
                act_session,
                user,
                password);
            act_session->type = WSM_CONNECT;
        }

        if (err == GSTAT_OK)
        {
            if (act_session->type == WSM_CONNECT)
            {
                if (usr_session != NULL)
                {
                    usr_session->requested_counter--;
                    /* remove the request made by the current active_session */
                    CLEAR_STATUS( WSMDeleteUsrSession(&usr_session) );
                    act_session->user_session = NULL;
                }

                err = WSSCreateUsrSession (
                    act_session,
                    user,
                    password,
                    &usr_session);
                if (err == GSTAT_OK)
                {
                    char usrnum[20]; /* ULONG_MAX is 20 digits */

                    act_session->user_session = usr_session;
                    if (usr_session != NULL)
                    {
                        CVna( usr_session->userid, usrnum );
                        err = WSMAddVariable(
                            act_session,
                            HVAR_USERID,
                            STlength(HVAR_USERID),
                            user,
                            STlength(user),
                            WSM_USER);
                        err = WSMAddVariable(
                            act_session,
                            HVAR_USERNUM,
                            STlength(HVAR_USERNUM),
                            usrnum,
                            STlength(usrnum),
                            WSM_USER);
                    }
                }
                asct_trace( ASCT_LOG )( ASCT_TRACE_PARAMS,
                    "Connect status=%8x user=%s session=%s",
                    ((err == NULL) ? 0 : err->number), user,
                    ((act_session->user_session) ?
                        act_session->user_session->name : "ICE2.0") );
                MEfree((PTR)user);
                MEfree((PTR)password);
            }
        }
    }

    if (act_session != NULL)
    {
        /*
        ** Set the variable for ii_cookie for use in ice variable pages
        ** executed from within a 2.5 session.
        */
        if (act_session->user_session != NULL)
        {
            CLEAR_STATUS(WSMAddVariable(
                act_session,
                HVAR_COOKIE,
                STlength(HVAR_COOKIE),
                act_session->user_session->name,
                STlength(act_session->user_session->name),
                WSM_ACTIVE));
        }
        if (gateway != NULL)
        {
            CLEAR_STATUS(WSMAddVariable(
                act_session,
                HVAR_HTTP_API_EXT,
                STlength(HVAR_HTTP_API_EXT),
                gateway,
                STlength(gateway),
                WSM_ACTIVE));
        }
        if (agent != NULL)
        {
            CLEAR_STATUS(WSMAddVariable(
                act_session,
                HVAR_HTTP_AGENT,
                STlength(HVAR_HTTP_AGENT),
                agent,
                STlength(agent),
                WSM_ACTIVE));
        }
        if (rmt_addr != NULL)
        {
            CLEAR_STATUS(WSMAddVariable(
                act_session,
                HVAR_HTTP_RMT_ADDR,
                STlength(HVAR_HTTP_RMT_ADDR),
                rmt_addr,
                STlength(rmt_addr),
                WSM_ACTIVE));
        }
        if (rmt_host != NULL)
        {
            CLEAR_STATUS(WSMAddVariable(
                act_session,
                HVAR_HTTP_RMT_HOST,
                STlength(HVAR_HTTP_RMT_HOST),
                rmt_host,
                STlength(rmt_host),
                WSM_ACTIVE));
        }
        asct_trace( ASCT_LOG )( ASCT_TRACE_PARAMS,
            "Connect session=%s access type=%s addr=%s client=%s",
            ((act_session->user_session) ?
                act_session->user_session->name : "ICE2.0"), gateway,
            ((rmt_addr) ? rmt_addr : ""), agent );
    }

    if (err != GSTAT_OK && err->number != E_DF0043_DDS_BAD_PASSWORD &&
        act_session != NULL)
    {
        char*    url = NULL;

        CLEAR_STATUS(WSMGetVariable(
            act_session,
            HVAR_ERROR_URL,
            STlength(HVAR_ERROR_URL),
            &url,
            WSM_ACTIVE));
        if (url != NULL)
        {
            CLEAR_STATUS(WSSErrorURL(act_session, url, err->number));
            DDFStatusFree(TRC_INFORMATION, &err);
        }
    }

    if (err != GSTAT_OK)
        CLEAR_STATUS( WSSCloseSession(&act_session, TRUE) );

    *session = act_session;
    return(err);
}

/*
** Name: WSSUnitAccessibility() - Verify if a unit is accessible
**
** Description:
**
** Inputs:
**	u_i4	user id
**	u_i4	unit id
**	i4	right
**
** Outputs:
**	bool	*result
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
WSSUnitAccessibility (
	USR_PSESSION	usr_session,
	u_i4	unit_id,
	i4	right,
	bool	*result)
{
	GSTATUS		err = GSTAT_OK;

	if (usr_session != NULL)
	{
		*result = (usr_session->userid == SYSTEM_ID || unit_id == SYSTEM_ID);

		if (*result == FALSE)
			err = WCSUnitIsOwner(unit_id, usr_session->userid, result);

		if (err == GSTAT_OK && *result == FALSE)
		{
			char resource[WSF_STR_RESOURCE];
			STprintf(resource, "p%d", unit_id);
			err = DDSCheckRight(usr_session->userid, resource, right, result);
		}
	}
	else
		*result = FALSE;
return(err);
}

/*
** Name: WSSGeneralUnitAccess
**
** Description:
**
** Inputs:
**    usr_session   pointer to session context
**    unit_id       unit to check
**    right         permission to check for
**
** Outputs:
**    result        permission mask test
**
** Returns:
**    GSTATUS    :    GSTAT_OK
**
** Exceptions:
**    None
**
** Side Effects:
**    None
**
** History:
**      05-May-2000 (fanra01)
**          Created.
*/
GSTATUS
WSSGeneralUnitAccess(
    USR_PSESSION    usr_session,
    u_i4    unit_id,
    i4      right,
    bool    *result)
{
    GSTATUS err = GSTAT_OK;
    i4      refs = 0;
    bool    access;

    if (usr_session != NULL)
    {
        *result = (usr_session->userid == SYSTEM_ID || unit_id == SYSTEM_ID);

        if (*result == FALSE)
            err = WCSUnitIsOwner(unit_id, usr_session->userid, result);

        if (err == GSTAT_OK && *result == FALSE)
        {
            char resource[WSF_STR_RESOURCE];
            STprintf(resource, "p%d", unit_id);
            err = DDSCheckRightRef(usr_session->userid, resource, right, &refs,
                &access);
            /*
            ** return granted if the right is granted and the reference
            ** count is not set for specific use.
            */
            *result = (access && (refs > 0)) ? FALSE : TRUE;
        }
    }
    else
        *result = FALSE;
    return(err);
}

/*
** Name: WSSDocAccessibility() - Verify if a document is accessible
**
** Description:
**
** Inputs:
**	u_i4	user id
**	u_i4	document id
**	i4	right
**
** Outputs:
**	bool	*result
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
**      03-Feb-2000 (fanra01)
**          Bug 100360
**          Modified the test on rights from value comparison to bit
**          comparison.
**      05-May-2000 (fanra01)
**          Add call to test for general access  if specifying flag other
**          than execute.
*/
GSTATUS
WSSDocAccessibility (
    USR_PSESSION    usr_session,
    u_i4           id,
    i4         right,
    bool            *result)
{
    GSTATUS err = GSTAT_OK;
    char    resource[WSF_STR_RESOURCE];

    *result = (id == SYSTEM_ID);

    if (*result == FALSE)
    {
        err = WCSDocCheckFlag (id, WCS_PUBLIC, result);
    }
    if (err == GSTAT_OK && *result == FALSE && usr_session != NULL)
    {
        if (usr_session->userid == SYSTEM_ID)
            *result = TRUE;
        else
        {
            if (err == GSTAT_OK && *result == FALSE)
                err = WCSDocIsOwner(id, usr_session->userid, result);

            if ((err == GSTAT_OK) && (*result == FALSE) &&
                ((right & WSF_READ_RIGHT) || (right & WSF_EXE_RIGHT)))
            {
                u_i4        unit_id;

                err = WCSDocGetUnit (id, &unit_id);
                if (err == GSTAT_OK)
                {
                    if (right & WSF_EXE_RIGHT)
                    {
                        err = WSSUnitAccessibility( usr_session, unit_id, right,
                            result );
                    }
                    else
                    {
                        /*
                        ** testing if user already has general access
                        */
                        err = WSSGeneralUnitAccess( usr_session, unit_id, right,
                            result );
                    }
                }
            }

            if (err == GSTAT_OK && *result == FALSE)
            {
                STprintf(resource, "d%d", id);
                err = DDSCheckRight( usr_session->userid, resource, right,
                    result);
            }
        }
    }
    return(err);
}

/*
** Name: WSSCheckDoc() - Check if the document can be accessed
**
** Description:
**
** Inputs:
**	ACT_PSESSION active session
**
** Outputs:
**	WPS_FILE	**cache
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
**      20-Mar-2001 (fanra01)
**          Correct function spelling WCSDispatchName.
**      23-Mar-2001 (fanra01)
**          If no unit and no location is specified attempt to use the
**          named root location.
*/
GSTATUS 
WSSCheckDoc (
    ACT_PSESSION    act_session,
    char*           page,
    WPS_FILE        **cache,
    u_i4            authorization)
{
    GSTATUS err = GSTAT_OK;
    char    *unit = NULL;
    char    *location = NULL;
    char    *name = NULL;
    char    *suffix = NULL;
    u_i4    length = STlength(page) + 1;
    char*   query;

    *cache = NULL;

    err = G_ME_REQ_MEM(0, query, char, length);
    if (err == GSTAT_OK)
    {
        MECOPY_CONST_MACRO(page, length, query);
        err = WCSDispatchName(
                        query,
                        &unit,
                        &location,
                        &name,
                        &suffix);

        if (err == GSTAT_OK &&
            (unit == NULL || unit[0] == EOS))
        {
            char *def_unit = NULL;
            err = WSMGetVariable (
                        act_session,
                        HVAR_UNIT,
                        STlength(HVAR_UNIT),
                        &def_unit,
                        WSM_SYSTEM | WSM_ACTIVE | WSM_USER);
            if (def_unit != NULL)
                unit = def_unit;
        }

        if (err == GSTAT_OK)
        {
            if (name != NULL && name[0] != EOS)
            {
                bool        status;
                u_i4        unit_id;
                bool        access;

                err = WCSGetUnitId(unit, &unit_id);
                if (err == GSTAT_OK)
                {
                    /*
                    ** if no unit and no location is specified use the named
                    ** root location
                    */
                    err = WPSRequest(
                        (act_session->user_session) ?
                            act_session->user_session->name:
                            NULL,
                        act_session->scheme,
                        act_session->host,
                        act_session->port,
                        (unit_id == 0 && location == NULL) ? "/" : location ,
                        unit_id,
                        name,
                        (suffix == NULL) ? STR_HTML_EXT : suffix,
                        cache,
                        &status);

                    if (err == GSTAT_OK)
                    {
                        err = WSSDocAccessibility(
                            act_session->user_session,
                            (*cache)->file->doc_id,
                            authorization,
                            &access);

                        if (err == GSTAT_OK)
                                                {
                            if (access == TRUE)
                            {
                                if (status == WCS_ACT_LOAD)
                                {
                                    bool is_external = TRUE;
                                    if ((*cache)->file->doc_id != SYSTEM_ID)
                                        err = WCSDocCheckFlag ((*cache)->file->doc_id, WCS_EXTERNAL, &is_external);

                                    if (is_external == FALSE)
                                        err = WCSExtractDocument ((*cache)->file);

                                    if (err == GSTAT_OK)
                                        CLEAR_STATUS( WCSLoaded((*cache)->file, TRUE) );
                                }
                            }
                            else
                            {
                                if (status == WCS_ACT_LOAD)
                                    CLEAR_STATUS( WCSLoaded((*cache)->file, FALSE) );
                                err = DDFStatusAlloc( E_WS0065_WSF_UNAUTH_ACCESS );
                            }
                        }
                    }
                }
            }
        }
        MEfree((PTR)query);
    }

    if (err != GSTAT_OK && *cache != NULL)
    {
        CLEAR_STATUS(WPSRelease(cache));
    }
    return(err);
}

/*
** Name: WSSProcessMacro() - Execute a macro command
**
** Description:
**
** Inputs:
**	ACT_PSESSION active session
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
**      03-Feb-2000 (fanra01)
**          Check page returned is valid.
**      11-Oct-2000 (fanra01)
**          Add call to WCSRetrieveDocument to set the extension for the
**          content type.
**      20-Nov-00 (fanra01)
**          Handle content type when no session is associated.  Previous
**          WCSRetrieveDocument returns E_WS0073 when a public HTTP document
**          is passed in.
**      17-Jul-2001 (fanra01)
**          Update the determination of text mimetype from the internal
**          registered suffix if it exists.  Otherwise use the extension.
**      01-Oct-2001 (fanra01)
**          Files used in unsecured 2.0 mode and named using a .htm extension
**          are returning incorrect mime types.  Using the extension to build
**          mime type was added for basic XML handling.  A more detailed
**          handling of mime type will require a change to the repository
**          schema.
**          Add explicit test of the htm extension and return html.
*/
GSTATUS 
WSSProcessMacro (
    ACT_PSESSION act_session,
    WPS_PBUFFER     buffer,
    char*                 page)
{
    GSTATUS        err = GSTAT_OK;
    WPS_FILE    *cache = NULL;

    err = WSSCheckDoc(act_session, page, &cache, WSF_EXE_RIGHT);
    if ((err == GSTAT_OK) && (cache != NULL))
    {
        asct_trace( ASCT_QUERY )( ASCT_TRACE_PARAMS,
            "Macro Execute session=%s page=%s",
            ((act_session->user_session) ?
                act_session->user_session->name : "ICE2.0"),
            (page ? page : "") );

        err = WPSPerformMacro( act_session, buffer, cache);
        if (err == GSTAT_OK)
        {
            /*
            **  If there is file information to use, try to get the mime type
            **  from the internal suffix of registered documents.  Otherwise
            **  set mime type from the file extension.
            */
            if ((act_session != NULL) && (cache->file != NULL))
            {
                if (cache->file->doc_id > 0)
                {
                    err = WCSRetrieveDocument( cache->file->doc_id, NULL,
                        NULL, &act_session->mimeext, NULL, NULL, NULL,
                        NULL, NULL);
                }
                else
                {
                    /*
                    ** if we have file information we'll use it otherwise
                    ** the mimeext field will stay null and get whatever
                    ** the default compiled value is.
                    */
                    if (cache->file->info != NULL)
                    {
                        char dev[MAX_LOC +1];
                        char path[MAX_LOC +1];
                        char prefix[MAX_LOC +1];
                        char suffix[MAX_LOC +1];
                        char vers[MAX_LOC +1];

                        if (LOdetail( &cache->file->loc, dev, path, prefix,
                            suffix, vers ) == OK)
                        {
                            /*
                            ** The addition of building mime type from the
                            ** file extension to provide basic XML handling
                            ** fell foul of the shortened extensions for
                            ** html files.
                            ** This causes an invalid mime type to be returned.
                            ** Detect htm extensions and return the html
                            ** mime type.
                            ** The correct treatment of this problem requires
                            ** a repository schema update.
                            */
                            if (STbcompare( suffix, 3, STR_HTML_EXT, 3, TRUE )
                                == 0)
                            {
                                act_session->mimeext = STalloc( STR_HTML_EXT );
                            }
                            else
                            {
                                act_session->mimeext = STalloc( suffix );
                            }
                        }
                    }
                }
            }
        }
        CLEAR_STATUS(WPSRelease(&cache));
    }
    return(err);
}

/*
** Name: WSSDownload() - Download a document
**
** Description:
**
** Inputs:
**	USR_PSESSION usr_session
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
**      18-Nov-1998 (fanra01)
**          Add MEfree for read buffer.
*/
GSTATUS
WSSDownload (
    ACT_PSESSION    act_session,
    char*           page,
    WPS_PBUFFER     result)
{
    GSTATUS     err = GSTAT_OK;
    WPS_FILE    *cache;
    char*       header = NULL;

    err = WSSCheckDoc(act_session, page, &cache, WSF_READ_RIGHT);
    if (err == GSTAT_OK)
    {
        result->page_type = WPS_DOWNLOAD_BLOCK;

        err = WCSOpen (cache->file, "r", SI_BIN, 0);
        if (err == GSTAT_OK)
        {
            char *tmp;

            err = G_ME_REQ_MEM(0, tmp, char, WSF_READING_BLOCK);
            if (err == GSTAT_OK)
            {
                i4 read;
                err = WCSRead(cache->file, WSF_READING_BLOCK, &read, tmp);
                if (err == GSTAT_OK)
                {
                    char*    category = NULL;
                    char* ext = NULL;
                    err = GuessFileType (tmp, read, &category, &ext);

                    while (err == GSTAT_OK && read > 0)
                    {
                        err = WPSBlockAppend(result, tmp, read);
                        if (err == GSTAT_OK)
                            err = WCSRead(cache->file, WSF_READING_BLOCK,
                                &read, tmp);
                    }

                    if (err == GSTAT_OK)
                    {
                        err = G_ME_REQ_MEM( 0, header, char,
                            STlength(BIN_CONTENT) + ((category) ?
                                STlength(category) :
                                STlength(DEFAULT_FILE_CAT)) +
                                    ((ext) ? STlength(ext) :
                                        STlength(DEFAULT_FILE_EXT)) + 1);
                        if (err == GSTAT_OK)
                            STprintf(header,
                                BIN_CONTENT,
                                (category) ? category : DEFAULT_FILE_CAT,
                                (ext) ? ext : DEFAULT_FILE_EXT);
                    }
                }
                MEfree (tmp);
            }
            CLEAR_STATUS( WCSClose(cache->file) );
        }

        if (err == GSTAT_OK && header != NULL)
            err = WPSHeaderAppend ( result, header, STlength(header));
        CLEAR_STATUS(WPSRelease(&cache));
    }

    if (err != GSTAT_OK)
    {
        char*    url = NULL;
        CLEAR_STATUS(WSMGetVariable( act_session, HVAR_ERROR_URL,
            STlength(HVAR_ERROR_URL), &url, WSM_ACTIVE));
        if (url != NULL)
        {
            CLEAR_STATUS(WSSErrorURL(act_session, url, err->number));
            DDFStatusFree(TRC_INFORMATION, &err);
            err = WSSSetBuffer(act_session, result, act_session->query);
        }
    }
    MEfree((PTR)header);
    return(err);
}

/*
** Name: WSSCAPIBuffer() - Execute a request
**
** Description:
**
** Inputs:
**	ACT_PSESSION active session
**
** Outputs:
**	WPS_PBUFFER		*buffer
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
**      18-Nov-1998 (fanra01)
**          Add MEfree for memory leak.
**      27-Nov-1998 (fanra01)
**          Replace null value literal with a string.  Adjustment in code for
**          sizeof nullval.  Fixes incorrect buffer positioning.
**      30-Oct-2001 (fanra01)
**          Append the error information text, if it exists, to the error
**          message buffer.
*/
static char* nullval = { "00" };

#define		SET_STR_PARAMS(X, Y)	\
		if (Y != NULL) STprintf(X, "%s%*d%s", X, STR_LENGTH_SIZE, STlength(Y), Y);\
		else STcat(X, nullval)

#define		SET_NUM_PARAMS(X, Y) STprintf(X, "%s%*d", X, NUM_MAX_INT_STR, Y)

GSTATUS 
WSSCAPIBuffer (
    ACT_PSESSION    session,
    char*           function,
    WPS_PBUFFER     buffer)
{
    GSTATUS             err = GSTAT_OK;
    PSERVER_FUNCTION    func;
    char *dll_name = function;
    char *fct_name = function;

    if (fct_name != NULL && fct_name[0] != EOS)
    {
        u_i4   numberOfCols;
        char*   *tab = NULL;
        char    *in_out = NULL;
        char*   var = NULL;
        char*   data = NULL;
        u_i4   out_attr = 0;
        u_i4   length;
        u_i4   i;
        u_i4   bufLength = 2 * NUM_MAX_INT_STR;
        char*   user_error;
        char*   buf = NULL;

        while (fct_name[0] != EOS && fct_name[0] != '.')
            fct_name++;
        if (fct_name[0] == '.')
        {
            fct_name[0] = EOS;
            fct_name++;
        }
        else
        {
            fct_name = dll_name;
            dll_name = NULL;
        }

        err = WSFGetServerFunction(dll_name, fct_name, &func);
        if (err == GSTAT_OK)
        {
            asct_trace( ASCT_QUERY )( ASCT_TRACE_PARAMS,
                "C_API Execute session=%s library=%s function=%s",
                ((session->user_session) ?
                    session->user_session->name : "ICE2.0"),
                    (dll_name ? dll_name : "server"), fct_name );

            numberOfCols = func->params_size;

            err = G_ME_REQ_MEM( 0, tab, char*, numberOfCols);
            if (err == GSTAT_OK)
                err = G_ME_REQ_MEM( 0, in_out, char, numberOfCols);

            if (err == GSTAT_OK)
            {
                char *out_name = NULL;

                for (i = 0; err == GSTAT_OK && i < numberOfCols; i++)
                {
                    var = func->params[i];
                    in_out[i] = ' ';
                    err = GAlloc(&out_name, STlength(var) + 5, FALSE);
                    if (err == GSTAT_OK)
                    {
                        STprintf(out_name, "out_%s", var);
                        err = WSMGetVariable(
                            session,
                            out_name,
                            STlength(out_name),
                            &data,
                            WSM_ACTIVE);
                        if (err == GSTAT_OK && data != NULL)
                        {
                            in_out[i] = 'O';
                            bufLength += (STR_LENGTH_SIZE + STlength(var));
                            out_attr++;
                        }
                    }

                    if (err == GSTAT_OK)
                    {
                        err = WSMGetVariable(
                            session,
                            var,
                            STlength(var),
                            &data,
                            WSM_ACTIVE);
                        if (err == GSTAT_OK && data != NULL)
                        {
                            length = STlength(data) + 1;
                            err = G_ME_REQ_MEM( 0, tab[i], char, length);
                                if (err == GSTAT_OK)
                                    MECOPY_VAR_MACRO(data, length, tab[i]);
                        }
                    }
                }

                if (err == GSTAT_OK)
                {
                    PTR user_info = NULL;
                    bool print = FALSE;
                    typedef GSTATUS    ( *SVRFCT )( ACT_PSESSION, char**, bool*, PTR*);
                    typedef char*    ( *USRFCT )( char**, bool*, PTR*);

                    SVRFCT svrfct;
                    USRFCT usrfct;

                    err = GAlloc((PTR*)&buf, bufLength + 1, FALSE);
                    if (err == GSTAT_OK)
                    {
                        buf[0] = EOS;
                        SET_NUM_PARAMS(buf, 0);
                        SET_NUM_PARAMS(buf, out_attr);
                        for (i = 0; i < numberOfCols; i++)
                            if (in_out[i] == 'O')
                                SET_STR_PARAMS(buf, func->params[i]);
                        err = WPSBlockAppend (buffer, buf, bufLength);
                    }

                    if (dll_name == NULL)
                        svrfct = (SVRFCT) func->fct;
                    else
                        usrfct = (USRFCT) func->fct;

                    do
                    {
                        if (dll_name == NULL)
                            err = svrfct(session, tab, &print, &user_info);
                        else
                        {
                            user_error = usrfct(tab, &print, &user_info);
                            if (user_error != NULL)
                            {
                                err=DDFStatusAlloc (E_WS0063_WSF_USER_FCT_ERR);
                                DDFStatusInfo(err, user_error);
                            }
                        }

                        if (err == GSTAT_OK && print == TRUE)
                        {
                            bufLength = 0;
                            for (i = 0; i < numberOfCols; i++)
                            {
                                if (in_out[i] == 'O')
                                {
                                    if (tab[i] != NULL)
                                        bufLength += (STR_LENGTH_SIZE + STlength(tab[i]));
                                    else
                                        bufLength += STlength (nullval);
                                }
                            }

                            err = GAlloc((PTR*)&buf, bufLength + 1, FALSE);
                            if (err == GSTAT_OK)
                            {
                                buf[0] = EOS;
                                for (i = 0; i < numberOfCols; i++)
                                    if (in_out[i] == 'O')
                                        SET_STR_PARAMS(buf, tab[i]);
                                err = WPSBlockAppend (buffer, buf, bufLength);
                            }
                        }
                    }
                    while (err == GSTAT_OK && user_info != NULL);
                }
                for (i = 0; i < numberOfCols; i++)
                    if (tab[i] != NULL)
                        MEfree((PTR)tab[i]);
                MEfree((PTR)tab);
                MEfree((PTR)in_out);
            }
            if (buf != NULL)
            {
                MEfree(buf);
            }
        }
    }
    else
    {
        char buf[NUM_MAX_INT_STR + 1];
        STprintf(buf, "%*d", NUM_MAX_INT_STR, 0);
        err = WPSBlockAppend (buffer, buf, NUM_MAX_INT_STR);
    }

    if (err != GSTAT_OK)
    {
        char buf[ERROR_BLOCK + NUM_MAX_INT_STR + STR_LENGTH_SIZE + 1];
        char szMsg[ERROR_BLOCK];
        CL_ERR_DESC clError;
        i4  nMsgLen = 0;
        i4  infolen = 0;

        WPS_BUFFER_EMPTY_BLOCK(buffer);
        STprintf(buf, "%*d", NUM_MAX_INT_STR, err->number);

        ERslookup (
            err->number,
            NULL,
            0,
            NULL,
            szMsg,
            ERROR_BLOCK,
            -1,
            &nMsgLen,
            &clError,
            0,
            NULL);
        if (nMsgLen > 0)
        {
            /*
            ** if there is additional information take its length.
            */
            if ((err->info != NULL) && (*err->info != 0))
            {
                infolen = STlength( err->info );
            }
            /*
            ** if the total message length will fit in the buffer including
            ** a space append additional info and bump the length.
            */
            if ((nMsgLen + infolen + 1) < ERROR_BLOCK)
            {
                if (infolen > 0)
                {
                    STcat( szMsg, STR_SPACE );
                    STcat( szMsg, err->info );
                    nMsgLen += (infolen + 1);
                }
            }
            szMsg[nMsgLen] = EOS;
            SET_STR_PARAMS(buf, szMsg);
        }
        CLEAR_STATUS(WPSBlockAppend (
                            buffer,
                            buf,
                            nMsgLen + NUM_MAX_INT_STR + STR_LENGTH_SIZE));
        DDFStatusFree(TRC_INFORMATION, &err);
    }
    return(err);
}

/*
** Name: WSSSetBuffer() - Execute a request
**
** Description:
**
** Inputs:
**    ACT_PSESSION active session
**
** Outputs:
**    WPS_PBUFFER        *buffer
**
** Returns:
**    GSTATUS    :    GSTAT_OK
**
** Exceptions:
**    None
**
** Side Effects:
**    None
**
** History:
*/
GSTATUS
WSSSetBuffer (
    ACT_PSESSION    session,
    WPS_PBUFFER        buffer,
    char*                    page)
{
    GSTATUS        err = GSTAT_OK;

    if (session->type == WSM_DOWNLOAD)
        err = WSSDownload (session, page, buffer);
    else if (session->clientType & C_API)
        err = WSSCAPIBuffer(session, page, buffer);
    else if (session->clientType & HTML_API)
        if (page != NULL && page[0] != EOS)
            err = WSSProcessMacro (session, buffer, page);
        else
            err = WPSPerformVariable(session, buffer);

    return(err);
}

/*
** Name: WSSIgnoreUserCase
**
** Description:
**      Function returns the state of the ignore_user_case flag.
**
** Inputs:
**      None.
**
** Outputs:
**      None.
**
** Returns:
**      True    Ignore case
**      False   Case sensitive
**
** History:
**      19-Nov-1998 (fanra01)
**          Created.
**
*/
bool
WSSIgnoreUserCase ()
{
    return (ignore_user_case);
}
