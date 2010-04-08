/******************************************************************************
**
**Copyright (c) 2004 Ingres Corporation
**
******************************************************************************/

#include <usrsession.h>
#include <wsfvar.h>
#include <erwsf.h>
#include <dds.h>
#include <wpsfile.h>
#include <wpsmo.h>
#include <wsmmo.h>
#include <tmtz.h>

/*
**
**  Name: userSession.c - User Session management
**
**  Description:
**
**  History:    
**	5-Feb-98 (marol01)
**	    created
**      03-Jul-98 (fanra01)
**          Add instance creation and removal of user session for ima.
**      11-Sep-1998 (fanra01)
**          Corrected case for usrsession to match piccolo.
**      15-Dec-1998 (fanra01)
**          Return user info based upon ingres dbuser as read from config.dat.
**      17-Aug-1999 (fanra01)
**          Bug 98429
**          Add the function WSMGetCookieExpireStr to convert the timeout
**          time to GMT and into the cookie string format.
**      22-Sep-1999 (fanra01)
**          Bug 98862
**          Removed the 'expire=' string as this was causing instant expiry
**          as it is duplicated in the returned HTTP header.
**      25-Jan-2000 (fanra01)
**          Bug 100132
**          Create the WSMGetCookieMaxAge function to return the session
**          timeout in seconds as a string.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      16-Mar-2001 (fanra01)
**          Bug 104298
**          Add initialization and use of parameters for multirow result
**          processing.
**      04-Oct-2001 (fanra01)
**          Bug 105964
**          Return an error if requesting an invalid session or a session
**          that no longer exists.
<<<< THEIR VERSION
**      14-Mar-2003 (fanra01)
**          Bug 109934
**          Move free of cursor name to within conditional.
====
**      29-Jul-2002 (fanra01)
**          Bug
**          Multiple session creation requests could return a duplicate hash
**          entry.
>>>> YOUR VERSION
**/

GLOBALDEF DDFHASHTABLE	usr_sessions;
static USR_PSESSION			oldest = NULL; 
static USR_PSESSION			youngest = NULL; 
static USR_PCURSOR			cursors = NULL;
static USR_PTRANS				transactions = NULL;
GLOBALDEF SEMAPHORE			usrSessSemaphore;
GLOBALDEF SEMAPHORE			transSemaphore;
GLOBALDEF SEMAPHORE			curSemaphore;
GLOBALDEF i4				system_timeout = 300;
GLOBALDEF i4				db_timeout = 300;
GLOBALDEF i4        minrowset =  MIN_ROW_SET;
GLOBALDEF i4        maxsetsize = MAX_SET_SIZE;

/*
** Name: WSMRemoveTimeout() - Remove the session from the timeout list
**
** Description:
**
** Inputs:
**	USR_PSESSION *usr_session
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
WSMRemoveTimeout(
	USR_PSESSION usr_session) 
{
	GSTATUS err = GSTAT_OK;

	if (usr_session == oldest)
		oldest = usr_session->timeout_next;

	if (usr_session == youngest)
		youngest = usr_session->timeout_previous;

	if (usr_session->timeout_previous != NULL)
		usr_session->timeout_previous->timeout_next = usr_session->timeout_next;

	if (usr_session->timeout_next != NULL)
		usr_session->timeout_next->timeout_previous = usr_session->timeout_previous;

	usr_session->timeout_previous = NULL;
	usr_session->timeout_next = NULL;
	usr_session->timeout_end = 0;
return(err);
}

/*
** Name: WSMSetTimeout() - Set the timeout system
**
** Description:
**
** Inputs:
**	USR_PSESSION *usr_session
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
WSMSetTimeout(
	USR_PSESSION usr_session) 
{
	GSTATUS err = GSTAT_OK;
	
	i4 user_timeout;
	if (usr_session->userid == SYSTEM_ID)
		user_timeout = system_timeout;
	else		
		err = DDSGetUserTimeout(usr_session->userid, &user_timeout);

	if (err == GSTAT_OK)
	{
		err = WSMRemoveTimeout(usr_session);
		if (err == GSTAT_OK)
		{
			usr_session->timeout_end = TMsecs() + user_timeout;
			if (youngest == NULL)
			{
				youngest = usr_session;
				oldest = usr_session;
			}
			else
			{
				usr_session->timeout_previous = youngest;
				youngest->timeout_next = usr_session;
				youngest = usr_session;
			}
		}
	}
return(err);
}

/*
** Name: WSMRemoveUsrSession() - remove a session from the hashtable and the timeout list.
**
** Description:
**
** Inputs:
**	USR_PSESSION*	: user session 
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
**      03-Jun-1999 (fanra01)
**          Add removal of user session variable semaphore.
*/
GSTATUS 
WSMRemoveUsrSession ( 
    USR_PSESSION    session)
{
    GSTATUS err = GSTAT_OK;
    USR_PSESSION tmp;

    CLEAR_STATUS(WPSCleanFilesTimeout (session->name));

    CLEAR_STATUS(DDFdelhash(
                        usr_sessions,
                        session->name,
                        STlength(session->name),
                        (PTR*) &tmp));

    CLEAR_STATUS(WSMRemoveTimeout(session));

    if (session->requested_counter <= 0)
    {
        while (session->transactions != NULL)
            CLEAR_STATUS( WSMRollback(session->transactions, FALSE) );

        if (session->name != NULL)
            usr_sess_detach(session->name);

        if (session->userid != SYSTEM_ID)
            CLEAR_STATUS(DDSReleaseUser(session->userid));

        err = DDFSemOpen(&session->usrvarsem, TRUE);
        if (err == GSTAT_OK)
        {
            CLEAR_STATUS(WSFCleanVariable(session->vars));
            CLEAR_STATUS(DDFrmhash(&session->vars));
            CLEAR_STATUS(DDFSemClose(&session->usrvarsem));
        }
        CLEAR_STATUS(DDFSemDestroy(&session->usrvarsem));
        MEfreetag(session->mem_tag);
        MEfree((PTR)session);
    }
return(err);
}

/*
** Name: WSMCleanUserTimeout() - Clean the timeout system
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
WSMCleanUserTimeout() 
{
	GSTATUS err = GSTAT_OK;
	USR_PSESSION tmp = oldest;
	USR_PSESSION next;
	i4 limit = TMsecs();

	err = DDFSemOpen(&usrSessSemaphore, TRUE);
	if (err == GSTAT_OK)
	{
		while (tmp != NULL &&
		   tmp->timeout_end < limit)
		{
			next = tmp->timeout_next; 
			CLEAR_STATUS(WSMRemoveUsrSession(tmp));
			tmp = next;
		}
		CLEAR_STATUS(DDFSemClose(&usrSessSemaphore));
	}
return(err);
}

/*
** Name: WSMReleaseUsrSession() - Release user session
**
** Description:
**
** Inputs:
**	USR_PSESSION *usr_session
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
**      03-Jul-98 (fanra01)
**          Enable removal of user session instance for ima.
*/
GSTATUS
WSMReleaseUsrSession(
	USR_PSESSION usr_session) 
{
	GSTATUS err = GSTAT_OK;
	
	if (usr_session == NULL)
		return(GSTAT_OK);

	err = DDFSemOpen(&usrSessSemaphore, TRUE);
	if (err == GSTAT_OK)
	{
		if (usr_session->requested_counter > 0)
			usr_session->requested_counter--;

		if (usr_session->timeout_end == 0)
		{
			if (usr_session->requested_counter == 0)
				CLEAR_STATUS(WSMRemoveUsrSession(usr_session));
		}
		else
			err = WSMSetTimeout(usr_session);

		CLEAR_STATUS(DDFSemClose(&usrSessSemaphore));
	}
return(err);
}


/*
** Name: WSMRequestUsrSession() - Retrieve a user session from the cookie
**
** Description:
**
** Inputs:
**	char*	: cookie
**
** Outputs:
**	USR_PSESSION*	: user session (NULL if doesn't exist)
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
**      04-Oct-2001 (fanra01)
**          Add a test for an invalid session or a session that is no longer
**          in the hash table and return an error.
*/
GSTATUS 
WSMRequestUsrSession (
    char         *cookie,
    USR_PSESSION *session)
{
    GSTATUS err = GSTAT_OK;
    u_i4    i = 0;
    USR_PSESSION tmp = NULL;

    if (cookie != NULL && cookie[0] != EOS)
    {
        err = DDFSemOpen(&usrSessSemaphore, TRUE);
        if (err == GSTAT_OK)
        {
            err = DDFgethash(
                    usr_sessions,
                    cookie,
                    STlength(cookie),
                    (PTR*) &tmp);
            /*
            ** If the object is not in the hash table, raise and error.
            */
            if (tmp == NULL)
            {
                err = DDFStatusAlloc( E_WS0011_WSS_NO_OPEN_SESSION );
            }
            if (tmp != NULL &&
                tmp->timeout_end < TMsecs())
                tmp = NULL;

            if (tmp != NULL)
                tmp->requested_counter++;

            CLEAR_STATUS(DDFSemClose(&usrSessSemaphore));
        }
    }

    *session = tmp;
    return(err);
}

/*
** Name: WSMCreateUsrSession() - create a user new session
**
** Description:
**
** Inputs:
**	char*	: name
**	u_i4	: userid
**	char*	: user
**	char*	: password
**
** Outputs:
**	USR_PSESSION*	: user session (NULL if doesn't exist)
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
**          Enable creation of user session instance for ima.
**      03-Jun-1999 (fanra01)
**          Add semaphore for protecting user session variables.
**      29-Jul-2002 (fanra01)
**          If a duplicate hash entry is detected during session creation it
**          could be a duplicate session from a frame it should be safe to
**          ignore the error and continue.
*/
GSTATUS 
WSMCreateUsrSession (
    char            *name,
    u_i4           userid,
    char            *user,
    char            *password,
    USR_PSESSION    *session)
{
    GSTATUS         err = GSTAT_OK;
    u_i4            i = 0;
    u_i4            length = 0;
    WSF_PVAR        var = NULL;
    USR_PSESSION    tmp = NULL;

    G_ASSERT((userid == SYSTEM_ID) && (user == NULL || password == NULL),
        E_WS0010_WPS_UNDEF_USERPASS);

    err = G_ME_REQ_MEM(0, tmp, USR_SESSION, 1);
    if (err == GSTAT_OK)
    {
        tmp->transactions = NULL;
        tmp->mem_tag = MEgettag();
        tmp->requested_counter = 0;

        length = STlength(name) + 1;
        err = G_ME_REQ_MEM(tmp->mem_tag, tmp->name, char, length);
        if (err == GSTAT_OK)
        {
            MECOPY_VAR_MACRO(name, length, tmp->name);
            if (userid == SYSTEM_ID)
            {
                length = STlength(user) + 1;
                err = G_ME_REQ_MEM(tmp->mem_tag, tmp->user, char, length);
                if (err == GSTAT_OK)
                {
                    MECOPY_VAR_MACRO(user, length, tmp->user);
                    length = STlength(password) + 1;
                    err = G_ME_REQ_MEM(tmp->mem_tag, tmp->password, char, length);
                    if (err == GSTAT_OK)
                        MECOPY_VAR_MACRO(password, length, tmp->password);
                }
                tmp->userid = SYSTEM_ID;
            }
            else
            {
                tmp->userid = userid;
                tmp->user = NULL;
                tmp->password = NULL;
            }
        }

        if (err == GSTAT_OK)
        {
            err = DDFSemOpen(&usrSessSemaphore, TRUE);
            if (err == GSTAT_OK)
            {
                err = WSMSetTimeout(tmp);
                if (err == GSTAT_OK)
		{
                    err = DDFputhash(
                            usr_sessions,
                            tmp->name,
                            STlength(tmp->name),
                            (PTR) tmp);

                   if ((err != GSTAT_OK) && 
                       (err->number == E_DF0002_HASH_ITEM_ALR_EXIST))
                   {
                       DDFStatusFree(TRC_EVERYTIME, &err);
                   }
		}
                CLEAR_STATUS(DDFSemClose(&usrSessSemaphore));
            }
        }
        if ((err == GSTAT_OK) &&
            ((err = DDFSemCreate(&tmp->usrvarsem, tmp->name)) == GSTAT_OK))
        {
            err = DDFmkhash(20, &tmp->vars);
        }
        if (err == GSTAT_OK)
        {    /* IMA Stuff */
            STATUS status;
            if ((status = usr_sess_attach(tmp->name, tmp)) != OK)
                err = DDFStatusAlloc(status);
        }

        if (err == GSTAT_OK && userid != SYSTEM_ID)
            err = DDSRequestUser(userid);

        if (err == GSTAT_OK)
            *session = tmp;
        else
            CLEAR_STATUS( WSMDeleteUsrSession (&tmp));
    }
return(err);
}

/*
** Name: WSMDeleteUsrSession() - clean a user session
**
** Description:
**
** Inputs:
**	USR_PSESSION*	: user session 
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
WSMDeleteUsrSession ( 
	USR_PSESSION *session) 
{
	GSTATUS err = GSTAT_OK;

	if (*session != NULL)
	{
		err = DDFSemOpen(&usrSessSemaphore, TRUE);
		if (err == GSTAT_OK)
		{
			(*session)->timeout_end = 0;
			if ((*session)->requested_counter == 0)
					CLEAR_STATUS(WSMRemoveUsrSession(*session));
			CLEAR_STATUS(DDFSemClose(&usrSessSemaphore));
		}
	}
	*session = NULL;
return(err);
}

/*
** Name: WSMGetUserName() - Return the user who owns the session
**
** Description:
**
** Inputs:
**	USR_PSESSION*	: user session 
**
** Outputs:
**	char**			: user name
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
WSMGetUserName(
	USR_PSESSION session, 
	char*	*user_name)
{
	GSTATUS err = GSTAT_OK;

	if (session->userid == SYSTEM_ID)
	{
		u_i4	length;
		length = STlength(session->user) + 1;
		err = GAlloc((PTR*)user_name, length, FALSE);
		if (err == GSTAT_OK) 
			MECOPY_VAR_MACRO(session->user, length, *user_name);
	}
	else
		err = DDSGetUserName(session->userid, user_name);
return(err);
}

/*
** Name: WSMBrowseUsers() - Open cursor to list user sessions
**
** Description:
**
** Inputs:
**	PTR*			: cursor
**
** Outputs:
**	char**			: session key
**	char**			: user owner
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
WSMBrowseUsers(
	PTR		*cursor,
	USR_PSESSION *session)
{
	GSTATUS err = GSTAT_OK;

	if (*cursor == NULL)
	{
		err = DDFSemOpen(&usrSessSemaphore, FALSE);
		if (err == GSTAT_OK)
		{
			*cursor = (PTR) oldest;
			*session = oldest;
		}
	}
	else
	{
		*session = (USR_PSESSION) *cursor;
		*session = (*session)->timeout_next;
		*cursor = (PTR) *session;
	}

	if (*session == NULL)
		CLEAR_STATUS(DDFSemClose(&usrSessSemaphore));
return(err);
}

/*
** Name: WSMCreateCursor() - 
**
** Description:
**
** Inputs:
**	USR_PTRANS	transaction
**	char*		cursor name
**
** Outputs:
**	USR_PCURSOR	*cursor
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
**      16-Mar-2001 (fanra01)
**          Add parameters for handlding mutilple row result sets.
*/
GSTATUS 
WSMCreateCursor(
    USR_PTRANS  transaction,
    char        *name,
    char        *statement,
    USR_PCURSOR *cursor)
{
    GSTATUS err = GSTAT_OK;

    *cursor = NULL;

    err = DDFSemOpen(&curSemaphore, TRUE);
    if (err == GSTAT_OK)
    {
        err = G_ME_REQ_MEM(0, (*cursor), USR_CURSOR, 1);
        if (err == GSTAT_OK)
        {
            if (name != NULL &&
                name[0] != EOS)
            {
                u_i4 len = STlength(name) + 1;
                err = G_ME_REQ_MEM(0, (*cursor)->name, char, len);
                if (err == GSTAT_OK)
                {
                    MECOPY_VAR_MACRO(name, len, (*cursor)->name);
                    usr_curs_attach (*cursor);
                    if (transaction->cursors != NULL)
                        transaction->cursors->previous = (*cursor);
                    (*cursor)->next = transaction->cursors;
                    transaction->cursors = (*cursor);
                    (*cursor)->cursors_next = cursors;
                    if (cursors != NULL)
                        cursors->cursors_previous = (*cursor);
                    cursors = (*cursor);
                }
            }
            (*cursor)->transaction = transaction;
        }
        CLEAR_STATUS(DDFSemClose(&curSemaphore));
    }
    if (err == GSTAT_OK)
    {
        err = DBPrepare(transaction->session->driver)(
            &(*cursor)->query,
            statement,
            transaction->rowset,
            transaction->setsize );
        if (err == GSTAT_OK)
            err = DBExecute(transaction->session->driver)(
                                &(transaction->session->session),
                                &(*cursor)->query);
    }
    return(err);
}

/*
** Name: WSMGetCursor() - 
**
** Description:
**
** Inputs:
**	USR_PTRANS	transaction
**	char*		cursor name
**
** Outputs:
**	USR_PCURSOR	*cursor
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
WSMGetCursor(	
	USR_PTRANS	transaction,
	char		*name,
	USR_PCURSOR	*cursor) 
{
	GSTATUS err = GSTAT_OK;
	USR_PCURSOR tmp = NULL;

	*cursor = NULL;

	if (transaction == NULL ||
		transaction->cursors == NULL)
		return(GSTAT_OK);

	err = DDFSemOpen(&curSemaphore, FALSE);
	if (err == GSTAT_OK)
	{
		tmp = transaction->cursors;
		while (tmp != NULL) 
		{
			if (STcompare(tmp->name, name) == 0)
				break;
			tmp = tmp->next;
		}
		CLEAR_STATUS(DDFSemClose(&curSemaphore));
	}
	if (tmp != NULL)
		*cursor = tmp;
return(err);
}

/*
** Name: WSMBrowseCursors() - 
**
** Description:
**
** Inputs:
**	USR_PSESSION*	: user session 
**	char*			: transaction name
**
** Outputs:
**	USR_PTRANS : transaction
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
WSMBrowseCursors(	
	PTR		*cursor,
	USR_PCURSOR	*object) 
{
	GSTATUS err = GSTAT_OK;

	if (*cursor == NULL)
	{
		err = DDFSemOpen(&curSemaphore, FALSE);
		if (err == GSTAT_OK)
		{
			*cursor = (PTR) cursors;
			*object = cursors;
		}
	}
	else
	{
		*object = (USR_PCURSOR) *cursor;
		*object = (*object)->cursors_next;
		*cursor = (PTR) *object;
	}

	if (*object == NULL)
		CLEAR_STATUS(DDFSemClose(&curSemaphore));
return(err);
}

/*
** Name: WSMDestroyCursor() - 
**
** Description:
**
** Inputs:
**	USR_PTRANS	transaction
**
** Outputs:
**	u_i4		*count
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
**      14-Mar-2003 (fanra01)
**          Moved free of cursor name to within check name conditional.
**          This should remove spurious name free.
*/
GSTATUS
WSMDestroyCursor(
	USR_PCURSOR	cursor,
	u_i4		*count,
	bool	   checkAdd) 
{
	GSTATUS err = GSTAT_OK;

	if (checkAdd == TRUE)
	{
		USR_PCURSOR tmp = cursors;
		while (tmp != NULL && tmp != cursor)
			tmp = tmp->next;
		if (tmp == NULL)
			err = DDFStatusAlloc (E_DF0063_INCORRECT_KEY);
	}

	if (err == GSTAT_OK && cursor != NULL)
	{
		err = DDFSemOpen(&curSemaphore, FALSE);
		if (err == GSTAT_OK)
		{
			err = DBClose(cursor->transaction->session->driver)(&cursor->query, count);
			CLEAR_STATUS(DBRelease(cursor->transaction->session->driver)(&cursor->query));

			if (cursor->name != NULL)
			{
	      usr_curs_detach (cursor);
				if (cursor->transaction->cursors == cursor)
					cursor->transaction->cursors = cursor->next;
				else
					cursor->previous->next = cursor->next;
				if (cursor->next != NULL)
					cursor->next->previous = cursor->previous;

				if (cursors == cursor)
					cursors = cursor->cursors_next;
				else
					cursor->cursors_previous->cursors_next = cursor->cursors_next;
				if (cursor->cursors_next != NULL)
					cursor->cursors_next->cursors_previous = cursor->cursors_previous;
                MEfree((PTR)cursor->name);
                cursor->name = NULL;
			}
			MEfree((PTR)cursor);
			CLEAR_STATUS(DDFSemClose(&curSemaphore));
		}
	}
return(err);
}

/*
** Name: WSMRetrieveTransaction() - 
**
** Description:
**
** Inputs:
**	USR_PSESSION*	: user session 
**	char*			: transaction name
**
** Outputs:
**	USR_PTRANS : transaction
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
WSMRetrieveTransaction(	
	USR_PSESSION session, 
	char *name, 
	USR_PTRANS *transaction) 
{
	GSTATUS err = GSTAT_OK;
	USR_PTRANS tmp = NULL;

	*transaction = NULL;

	if (session == NULL)
		return(GSTAT_OK);

	tmp = session->transactions;
	while (tmp != NULL) 
	{
		if (STcompare(tmp->trname, name) == 0)
			break;
		tmp = tmp->next;
	}

	if (tmp != NULL) 
		*transaction = tmp;
return(err);
}

/*
** Name: WSMCreateTransaction() - 
**
** Description:
**
** Inputs:
**	USR_PSESSION	usr_session
**	u_i4			database Type
**	char*			database name
**	char*			transation name
**
** Outputs:
**	USR_PTRANS transaction
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
**      15-Dec-1998 (fanra01)
**          Add lookup of default dbuser from config.dat and return info.
**      16-Mar-2001 (fanra01)
**          Add initialization of the rowset parameters on a per transcaction
**          basis.
*/
GSTATUS 
WSMCreateTransaction(
    USR_PSESSION    usr_session,
    u_i4           dbType,
    char*           name,
    char*           trname,
    char*           user,
    char*           password,
    USR_PTRANS*     transaction)
{
    GSTATUS err = GSTAT_OK;
    bool    dbconnection = FALSE;
    char*   dbname = name;
    char*   dbuser = NULL;
    char*   dbpassword = NULL;
    char*   allocPassword = NULL;
    i4 dbflag;

    *transaction = NULL;

    if (usr_session == NULL)
        trname = NULL;

    err = DDFSemOpen(&transSemaphore, FALSE);
    if (err == GSTAT_OK)
    {
        if (trname != NULL && trname[0] != EOS)
            err = WSMRetrieveTransaction(usr_session, trname, transaction);

        if (*transaction == NULL)
        {
            if (user != NULL)
            {
                err = DDSGetDBUserPassword(user, &dbuser, &allocPassword);
                dbpassword = allocPassword;
            }
            else if (usr_session != NULL)
            {
                if (usr_session->userid == SYSTEM_ID)
                {
                    dbuser = usr_session->user;
                    dbpassword = usr_session->password;
                }
                else
                {
                    err = DDSGetDBForUser(
                                            usr_session->userid,
                                            name,
                                            &dbname,
                                            &dbuser,
                                            &allocPassword,
                                            &dbflag);
                    dbpassword = allocPassword;
                }
            }
            else
            {
                char  szConfig [CONF_STR_SIZE];
                STprintf (szConfig, CONF_DEFAULT_USERID, PMhost());
                PMget( szConfig, &user);
                if (user != NULL)
                {
                    char *alias = NULL;

                    if ((err = DDSGetDBUserInfo(&user, &alias, &allocPassword))
                        == GSTAT_OK)
                    {
                        dbuser = user;
                        dbpassword = allocPassword;
                    }
                }
            }

            G_ASSERT(dbuser == NULL, E_WS0087_WSM_UNDEF_DB_USER);

            if (password != NULL)
                dbpassword = password;

            if (err == GSTAT_OK)
            {
                char *name = NULL;
                char *node = NULL;
                char *svrclass = NULL;
                u_i4 i = 0;

                while (err == GSTAT_OK &&
                       dbname != NULL && 
                       dbname[i] != EOS) 
                {
                    if (dbname[i] == ':' && dbname[i+1] == VARIABLE_MARK)
                    {
                        err = G_ME_REQ_MEM(0, node, char, i + 1);
                        if (err == GSTAT_OK)
                            MECOPY_VAR_MACRO(dbname, i, node);
                        node[i] = EOS;
                        dbname += i + 2;
                        i = 0;
                    }
                    else if (dbname[i] == '/')
                    {
                        err = G_ME_REQ_MEM(0, name, char, i + 1);
                        if (err == GSTAT_OK)
                          MECOPY_VAR_MACRO(dbname, i, name);
                        name[i] = EOS;
                        dbname += i + 1;
                        i = 0;
                    }
                    i++;
                }

                if (name == NULL)
                {
                    err = G_ME_REQ_MEM(0, name, char, i + 1);
                    if (err == GSTAT_OK)
                        MECOPY_VAR_MACRO(dbname, i, name);
                    name[i] = EOS;
                }
                else
                {
                    err = G_ME_REQ_MEM(0, svrclass, char, i + 1);
                    if (err == GSTAT_OK)
                        MECOPY_VAR_MACRO(dbname, i, svrclass);
                    svrclass[i] = EOS;
                }

                err = G_ME_REQ_MEM(0, (*transaction), USR_TRANS, 1);
                if (err == GSTAT_OK)
                {
                    (*transaction)->rowset = minrowset;
                    (*transaction)->setsize= maxsetsize;
                    err = DBCachedConnect (
                                dbType,
                                node,
                                name,
                                svrclass,
                                NULL,
                                (dbuser) ? dbuser : user,
                                (dbpassword) ? dbpassword : password,
                                db_timeout,
                                &(*transaction)->session);

                    if (err == GSTAT_OK &&
                        trname != NULL &&
                        trname[0] != EOS)
                    {
                        u_i4 len = STlength(trname) + 1;
                        err = G_ME_REQ_MEM(0, (*transaction)->trname, char, len);
                        if (err == GSTAT_OK)
                        {
                            MECOPY_VAR_MACRO(trname, len, (*transaction)->trname);
                            (*transaction)->usr_session = usr_session;
                               usr_trans_attach (*transaction);

                            if (usr_session->transactions != NULL)
                                usr_session->transactions->previous = (*transaction);
                            (*transaction)->next = usr_session->transactions;
                            usr_session->transactions = (*transaction);

                            (*transaction)->trans_next = transactions;
                            if (transactions != NULL)
                                transactions->trans_previous = (*transaction);
                            transactions = (*transaction);
                        }
                    }
                    else
                        (*transaction)->trname = NULL;
                }
                MEfree((PTR)node);
                MEfree((PTR)name);
                MEfree((PTR)svrclass);
            }
        }
        CLEAR_STATUS(DDFSemClose(&transSemaphore));
    }
    MEfree((PTR)allocPassword);
    return(err);
}

/*
** Name: WSMGetTransaction() - 
**
** Description:
**
** Inputs:
**	USR_PSESSION*	: user session 
**	char*			: transaction name
**
** Outputs:
**	USR_PTRANS : transaction
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
WSMGetTransaction(	
	USR_PSESSION session, 
	char *name, 
	USR_PTRANS *transaction) 
{
	GSTATUS err = GSTAT_OK;
	err = DDFSemOpen(&transSemaphore, FALSE);
	if (err == GSTAT_OK)
	{
		err = WSMRetrieveTransaction(session, name, transaction);
		CLEAR_STATUS(DDFSemClose(&transSemaphore));
	}
return(err);
}

/*
** Name: WSMBrowseTransaction() - 
**
** Description:
**
** Inputs:
**	USR_PSESSION*	: user session 
**	char*			: transaction name
**
** Outputs:
**	USR_PTRANS : transaction
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
WSMBrowseTransaction(	
	PTR		*cursor,
	USR_PTRANS *trans)
{
	GSTATUS err = GSTAT_OK;

	if (*cursor == NULL)
	{
		err = DDFSemOpen(&transSemaphore, FALSE);
		if (err == GSTAT_OK)
		{
			*cursor = (PTR) transactions;
			*trans = transactions;
		}
	}
	else
	{
		*trans = (USR_PTRANS) *cursor;
		*trans = (*trans)->trans_next;
		*cursor = (PTR) *trans;
	}

	if (*trans == NULL)
		CLEAR_STATUS(DDFSemClose(&transSemaphore));
return(err);
}

/*
** Name: WSMDestroyTransaction() - 
**
** Description:
**
** Inputs:
**	USR_PSESSION usr_session
**	USR_PTRANS transaction
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
WSMDestroyTransaction(
	USR_PTRANS transaction) 
{
	GSTATUS err = GSTAT_OK;
	CLEAR_STATUS( DBCachedDisconnect(&transaction->session) );
	if (transaction->trname != NULL)
	{
		usr_trans_detach (transaction);
		if (transaction == transaction->usr_session->transactions)
			transaction->usr_session->transactions = transaction->next;
		else
			transaction->previous->next = transaction->next;
		if (transaction->next != NULL)
			transaction->next->previous = transaction->previous;

		if (transaction == transactions)
			transactions = transaction->trans_next;
		else
			transaction->trans_previous->trans_next = transaction->trans_next;

		if (transaction->trans_next != NULL)
			transaction->trans_next->trans_previous = transaction->trans_previous;
        MEfree((PTR)transaction->trname);
	}
	MEfree((PTR)transaction);
return(err);
}

/*
** Name: WSMCommit() - 
**
** Description:
**
** Inputs:
**	USR_PSESSION usr_session
**	USR_PTRANS transaction
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
WSMCommit(
	USR_PTRANS transaction,
	bool	   checkAdd) 
{
	GSTATUS err = GSTAT_OK;

	err = DDFSemOpen(&transSemaphore, FALSE);
	if (err == GSTAT_OK)
	{
		if (checkAdd == TRUE)
		{
			USR_PTRANS tmp = transactions;
			while (tmp != NULL && tmp != transaction)
				tmp = tmp->next;
			if (tmp == NULL)
				err = DDFStatusAlloc (E_DF0063_INCORRECT_KEY);
		}
	
		if (err == GSTAT_OK)
		{
			while (transaction->cursors != NULL)
				CLEAR_STATUS(WSMDestroyCursor(transaction->cursors, NULL, FALSE));

			err = DBCommit(transaction->session->driver)(&transaction->session->session);
			if (err == GSTAT_OK)
				CLEAR_STATUS( WSMDestroyTransaction(transaction) );
		}
		CLEAR_STATUS(DDFSemClose(&transSemaphore));
	}
return(err);
}

/*
** Name: WSMRollback() - 
**
** Description:
**
** Inputs:
**	USR_PSESSION usr_session
**	USR_PTRANS transaction
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
WSMRollback(
	USR_PTRANS transaction,
	bool	   checkAdd) 
{
	GSTATUS err = GSTAT_OK;

	err = DDFSemOpen(&transSemaphore, FALSE);
	if (err == GSTAT_OK)
	{
		if (checkAdd == TRUE)
		{
			USR_PTRANS tmp = transactions;
			while (tmp != NULL && tmp != transaction)
				tmp = tmp->next;
			if (tmp == NULL)
				err = DDFStatusAlloc (E_DF0063_INCORRECT_KEY);
		}
	
		if (err == GSTAT_OK)
		{
			while (transaction->cursors != NULL)
				CLEAR_STATUS(WSMDestroyCursor(transaction->cursors, NULL, FALSE));

			err = DBRollback(transaction->session->driver)(&transaction->session->session);
			if (err == GSTAT_OK)
				CLEAR_STATUS( WSMDestroyTransaction(transaction) );
		}
		CLEAR_STATUS(DDFSemClose(&transSemaphore));
	}
return(err);
}

/*
** Name: WSMGetCookieExpireStr
**
** Description:
**      Get the expire string to be appended to the cookie path.
**      Determines how long the cookie will be valid in the browser.
**      After this time the cookie is no longer sent back to the HTTP server.
**
** Inputs:
**    USR_PSESSION  usr_session
**
** Outputs:
**      expire      expiration time string
**
** Returns:
**      GSTAT_OK    success
**
** Exceptions:
**    None
**
** Side Effects:
**    None
**
** History:
**      17-Aug-1999 (fanra01)
**          Created.
**      22-Sep-1999 (fanra01)
**          Bug 98862
**          Removed the 'expire=' from the string as this was conflicting
**          with the header setup.
*/
GSTATUS
WSMGetCookieExpireStr( USR_PSESSION usrsess, PTR expire )
{
    GSTATUS status = GSTAT_OK;
    STATUS  stat = OK;
    SYSTIME systime;
    PTR     tzcb = NULL;
    long    timesecs = 0;
    i4      timecheck = 0;
    i4      tzoff = 0;
    struct  TMhuman th;
    char*   lsyear;

    /*
    ** Set structure to get timeout time
    */
    systime.TM_secs = usrsess->timeout_end;
    timesecs = (long)systime.TM_secs;
    if( timesecs > MAXI4)
        timecheck = MAXI4;
    else if( timesecs < MINI4)
        timecheck = MINI4;
    else
        timecheck = (i4) timesecs;

    if ((stat = TMtz_init( &tzcb )) == OK)
    {

        tzoff = TMtz_search( tzcb, TM_TIMETYPE_LOCAL, timecheck );

        /*
        ** Adjust for GMT
        */
        systime.TM_secs -= tzoff;

        TMbreak( &systime, &th );

        /*
        ** Least significant year digits
        */
        lsyear = th.year;
        CMnext( lsyear );
        CMnext( lsyear );

        STprintf( expire, "%s, %s-%s-%s %s:%s:%s GMT",
            th.wday, th.day, th.month, lsyear, th.hour, th.mins, th.sec );
    }
    else
    {
        status = DDFStatusAlloc(stat);
    }
    return(status);
}

/*
** Name: WSMGetCookieMaxAge
**
** Description:
**      Return the session timeout seconds as a string for inclusion in the
**      cookie.
**
** Inputs:
**    USR_PSESSION  usr_session
**
** Outputs:
**      expire      expiration seconds string
**
** Returns:
**      GSTAT_OK    success
**
** Exceptions:
**    None
**
** Side Effects:
**    None
**
** History:
**      25-Jan-2000 (fanra01)
**          Created.
*/
GSTATUS
WSMGetCookieMaxAge( USR_PSESSION usrsess, PTR expire )
{
    GSTATUS err = GSTAT_OK;
    i4 user_timeout = system_timeout;

    if (usrsess->userid != SYSTEM_ID)
    {
        err = DDSGetUserTimeout(usrsess->userid, &user_timeout);
        if (err != GSTAT_OK)
        {
            user_timeout = 0;
        }
    }
    CVna( user_timeout, expire );
    return(err);
}
