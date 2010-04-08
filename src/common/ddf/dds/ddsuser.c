/******************************************************************************
**
** Copyright (c) 2004 Ingres Corporation
**
******************************************************************************/

#include <ddgexec.h>
#include <ddsmem.h>

/**
**
**  Name: ddsuser.c - Data Dictionary Security Service Facility
**
**  Description:
**		This file contains all available functions to manage users
**
**  History:    
**	5-Feb-98 (marol01)
**	    created
**	13-may-1998 (canor01)
**	    Replace GC_L_PASSWORD with DDS_PASSWORD_LENGTH.
**          Replace gcn_encrypt with gcu_encode.
**	15-may-1998 (marol01)
**	    Add user timeout and method for getting timeout property.
**	15-may-1998 (marol01)
**	    Add user timeout.
**      07-Jan-1999 (fanra01)
**          Add authtype for OS password authentication.
**      27-Jan-1999 (fanra01)
**          Changed order of authtype and id for update as the order of the
**          parameters is used in the query.
**      06-Aug-1999 (fanra01)
**          Change nat to i4, u_nat to u_i4, longnat to i4, u_longnat to u_i4
**      05-May-2000 (fanra01)
**          Bug 101346
**          Add the refs flag parameter to the assign and delete user right
**          functions in order to link the sets of rights to a single one.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	11-oct-2002 (somsa01)
**	    Changed DDGInsert(), DDGUpdate(), DDGKeySelect(), DDGExecute()
**	    and DDGDelete() such that we pass the addresses of int and float
**	    arguments.
**	28-Jul-2004 (hanje04)
**	    Replace calls to gcu_encode with DDSencode.
**/

GLOBALREF SEMAPHORE dds_semaphore;
GLOBALREF DDG_SESSION dds_session;

/*
** Name: DDSCreateUser() - User creation in the Data Dictionary and in memory
**
** Description:
**	
**
** Inputs:
**	char*		: user name
**	char*		: user password
**	u_nat		: db user id
**  longnat		: flags
**	char*		: comment
**
** Outputs:
**	u_nat*		: user id
**
** Returns:
**	GSTATUS		: 
**				  E_OG0001_OUT_OF_MEMORY
**				  GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
**      07-Jan-1999 (fanra01)
**          Add authtype function parameter and encrypt password if repository
**          authentication required.
*/

GSTATUS 
DDSCreateUser (
    char*   name,
    char*   password,
    u_i4   dbuser_id,
    char*   comment,
    i4 flags,
    i4 timeout,
    i4 authtype,
    u_i4*  id)
{
    GSTATUS err = GSTAT_OK;
    STATUS  status = OK;

    err = DDFSemOpen(&dds_semaphore, TRUE);
    if (err == GSTAT_OK)
    {
        err = DDSPerformUserId(id);
        if (err == GSTAT_OK)
        {
            char crypt[DDS_PASSWORD_LENGTH];

            crypt[0] = EOS;
            switch (authtype)
            {
                case DDS_OS_USRPWD_AUTH:
                    /*
                    ** Do nothing, password ignored.
                    ** Empty string inserted to table.
                    */
                    break;

                case DDS_REPOSITORY_AUTH:
                default:
                    status = DDSencode(name, password, crypt);
                    break;
            }
            if (status == OK)
            {
                err = DDGInsert(
                        &dds_session,
                        DDS_OBJ_USER,
                        "%d%s%s%d%d%d%s%d",
                        id,
                        name,
                        crypt,
                        &dbuser_id,
                        &flags,
                        &timeout,
                        comment,
                        &authtype);
                if (err == GSTAT_OK)
                {
                    err = DDSMemCreateUser(*id, name, crypt, flags, timeout,
                        dbuser_id, authtype);
                }
                if (err == GSTAT_OK)
                    err = DDGCommit(&dds_session);
                else
                    CLEAR_STATUS(DDGRollback(&dds_session));
            }
            else
                err = DDFStatusAlloc(E_DF0054_DDS_CRYPT);
        }
        CLEAR_STATUS(DDFSemClose(&dds_semaphore));
    }
    return(err);
}

/*
** Name: DDSUpdateUser()	- User change in the Data Dictionary and in memory
**
** Description:
**	
**
** Inputs:
**	u_nat		: user id
**	char*		: user name
**	char*		: user password
**	u_nat		: db user id
**	char*		: comment
**  longnat		: flags
**	longnat		: timeout
**
** Outputs:
**
** Returns:
**	GSTATUS		: 
**				  E_OG0001_OUT_OF_MEMORY
**				  GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
**      07-Jan-1999 (fanra01)
**          Add authtype function parameter.
**      27-Jan-1999 (fanra01)
**          Changed order of authtype and id as order matters here.
*/

GSTATUS 
DDSUpdateUser (
    u_i4   id,
    char*   name,
    char*   password,
    u_i4   dbuser_id,
    char*   comment,
    i4 flags,
    i4 timeout,
    i4 authtype)
{
    GSTATUS err = GSTAT_OK;

    err = DDFSemOpen(&dds_semaphore, TRUE);
    if (err == GSTAT_OK)
    {
        bool same = FALSE;
        char crypt[DDS_PASSWORD_LENGTH];
        i4 auth = DDS_REPOSITORY_AUTH;
        STATUS status = OK;

        crypt[0] = EOS;
        switch (authtype)
        {
            case DDS_OS_USRPWD_AUTH:
                if ((err = DDSMemGetAuthType(id, &auth)) == GSTAT_OK)
                {
                    if (authtype != auth)
                    {
                        /*
                        ** assign empty string to clear password field
                        ** in table
                        */
                        password = crypt;
                    }
                }
                break;

            case DDS_REPOSITORY_AUTH:
            default:
                if (DDSencode(name, password, crypt) != OK)
                {
                    err = DDFStatusAlloc(E_DF0054_DDS_CRYPT);
                }
                else
                {
                    password = crypt;
                    err = DDSMemVerifyUserPass(id, password, &same);
                }
                break;
        }

        if (err == GSTAT_OK)
        {
            err = DDGUpdate(
                    &dds_session,
                    DDS_OBJ_USER,
                    "%s%s%d%d%d%s%d%d",
                    name,
                    password,
                    &dbuser_id,
                    &flags,
                    &timeout,
                    comment,
                    &authtype,
                    &id);

            if (err == GSTAT_OK)
            {
                err = DDSMemUpdateUser(id, name, password, flags,
                    timeout, dbuser_id, authtype);
            }
            if (err == GSTAT_OK)
                err = DDGCommit(&dds_session);
            else
                CLEAR_STATUS(DDGRollback(&dds_session));
        }
        CLEAR_STATUS(DDFSemClose(&dds_semaphore));
    }
    return(err);
}

/*
** Name: DDSDeleteUser() - User delete in the Data Dictionary and in memory
**
** Description:
**	
**
** Inputs:
**	u_nat		: user id
**	char*		: user name
**	char*		: user password
**	u_nat		: db user id
**	char*		: comment
**
** Outputs:
**
** Returns:
**	GSTATUS		: 
**				  E_OG0001_OUT_OF_MEMORY
**				  GSTAT_OK
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
DDSDeleteUser (
	u_i4 id) 
{
	GSTATUS err = GSTAT_OK;

	err = DDFSemOpen(&dds_semaphore, TRUE);
	if (err == GSTAT_OK) 
	{
		err = DDGExecute(&dds_session, DDS_DEL_USER_RIGHT, "%d", &id);
		CLEAR_STATUS( DDGClose(&dds_session) );

		if (err == GSTAT_OK) 
		{
			err = DDGExecute(&dds_session, DDS_DEL_USER_ROLE,
					 "%d", &id);
			CLEAR_STATUS( DDGClose(&dds_session) );
		}

		if (err == GSTAT_OK) 
		{
			err = DDGExecute(&dds_session, DDS_DEL_USER_DB, "%d",
					 &id);
			CLEAR_STATUS( DDGClose(&dds_session) );
		}

		if (err == GSTAT_OK) 
			err = DDGDelete(&dds_session, DDS_OBJ_USER, "%d", &id);

		if (err == GSTAT_OK) 
			err = DDSMemDeleteUser(id);

		if (err == GSTAT_OK) 
			err = DDGCommit(&dds_session);
		else 
			CLEAR_STATUS(DDGRollback(&dds_session));

		CLEAR_STATUS(DDFSemClose(&dds_semaphore));
	}
return(err);
}

/*
** Name: DDSAssignUserRight() - define an accdds right between a user 
**                                and a resource
**
** Description:
**
**
** Inputs:
**      user        user to assign right to
**      resource    resource string for the right
**      right       permissions to remove
**      refs        flag indiates if reference count should be altered.
**
** Outputs:
**
** Returns:
**    GSTATUS:
**          E_OG0001_OUT_OF_MEMORY
**          GSTAT_OK
**
** Exceptions:
**    None
**
** Side Effects:
**    None
**
** History:
**      05-May-2000 (fanra01)
**          Add refs parameter to function and repository calls.
*/
GSTATUS
DDSAssignUserRight(
    u_i4 user,
    char* resource,
    u_i4 right,
    i4 refs,
    bool* created
 )
{
    GSTATUS err = GSTAT_OK;
    u_i4    tmp = right;
    i4      pref = refs;

    err = DDFSemOpen(&dds_semaphore, TRUE);
    if (err == GSTAT_OK)
    {
        err = DDSMemAssignUserRight(user, resource, &tmp, &pref, created);
        if (err == GSTAT_OK)
            if (*created == TRUE)
                err = DDGInsert(
                        &dds_session,
                        DDS_OBJ_USER_RIGHT,
                        "%d%s%d%d",
                        &user,
                        resource,
                        &tmp,
                        &pref);
            else
                err = DDGUpdate(
                        &dds_session,
                        DDS_OBJ_USER_RIGHT,
                        "%d%d%d%s",
                        &tmp,
                        &pref,
                        &user,
                        resource);

        if (err == GSTAT_OK)
            err = DDGCommit(&dds_session);
        else
        {
            bool deleted = FALSE;
            CLEAR_STATUS(DDGRollback(&dds_session));
            /*
            ** created and counted then count the resource when deleted
            */
            if (*created == TRUE)
            {
                pref = (refs != 0) ? refs : 0;
            }
            else
            {
                pref = 0;
            }
            CLEAR_STATUS(DDSMemDeleteUserRight(user, resource, &right, &pref,
                &deleted));
            *created = FALSE;
        }
        CLEAR_STATUS(DDFSemClose(&dds_semaphore));
    }
return(err);
}

/*
** Name: DDSDeleteUserRight() - define an accdds right between a user 
**                                and a resource
**
** Description:
**
**
** Inputs:
**      user        user to assign right to
**      resource    resource string for the right
**      right       permissions to remove
**      refs        flag indiates if reference count should be altered.
**
** Outputs:
**      None.
**
** Returns:
**    GSTATUS:
**          E_OG0001_OUT_OF_MEMORY
**          GSTAT_OK
**
** Exceptions:
**    None
**
** Side Effects:
**    None
**
** History:
**      05-May-2000 (fanra01)
**          Add refs parameter to function and repository calls.
*/
GSTATUS
DDSDeleteUserRight(
    u_i4 user,
    char* resource,
    u_i4 right,
    i4 refs,
    bool* deleted )
{
    GSTATUS err = GSTAT_OK;
    i4      tmp = right;
    i4      prefs = refs;

    err = DDFSemOpen(&dds_semaphore, TRUE);
    if (err == GSTAT_OK)
    {
        err = DDSMemDeleteUserRight(user, resource, &tmp, &prefs, deleted);
        if (err == GSTAT_OK)
            if (*deleted == TRUE)
                err = DDGDelete(
                        &dds_session,
                        DDS_OBJ_USER_RIGHT,
                        "%d%s",
                        &user,
                        resource);
            else
                err = DDGUpdate(
                        &dds_session,
                        DDS_OBJ_USER_RIGHT,
                        "%d%d%d%s",
                        &tmp,
                        &prefs,
                        &user,
                        resource);

        if (err == GSTAT_OK)
            err = DDGCommit(&dds_session);
        else
        {
            bool created = FALSE;
            CLEAR_STATUS(DDGRollback(&dds_session));
            /*
            ** deleted and counted then count the resource when reassigned.
            */
            if (*deleted == TRUE)
            {
                prefs = (refs != 0) ? refs : 0;
            }
            else
            {
                prefs = 0;
            }
            CLEAR_STATUS(DDSMemAssignUserRight(user, resource, &right, &prefs,
                &created ));
            *deleted = FALSE;
        }
        CLEAR_STATUS(DDFSemClose(&dds_semaphore));
    }
return(err);
}

/*
** Name: DDSOpenUser()	- Open a cursor on the User list
**
** Description:
**	
**
** Inputs:
**
** Outputs:
**
** Returns:
**	GSTATUS		: 
**				  GSTAT_OK
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
DDSOpenUser () 
{
	GSTATUS err = GSTAT_OK;

	err = DDFSemOpen(&dds_semaphore, TRUE);
	if (err == GSTAT_OK)
		err = DDGSelectAll(&dds_session, DDS_OBJ_USER);
return(err);
}

/*
** Name: DDSRetrieveUser()	- Select a unique User
**
** Description:
**	
**
** Inputs:
**	u_nat : User id
**
** Outputs:
**
** Returns:
**	GSTATUS		: 
**				  GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
**      07-Jan-1999 (fanra01)
**          Add authtype function parameter.
*/

GSTATUS 
DDSRetrieveUser (
    u_i4   id,
    char   **name,
    char   **password,
    u_i4   **dbuser_id,
    char   **comment,
    i4     **flags,
    i4     **timeout,
    i4     **authtype)
{
    GSTATUS err = GSTAT_OK;

    err = DDFSemOpen(&dds_semaphore, TRUE);
    if (err == GSTAT_OK)
    {
        err = DDGKeySelect(
                &dds_session,
                DDS_OBJ_USER,
                "%d%s%s%d%d%d%s%d",
                &id,
                name,
                password,
                dbuser_id,
                flags,
                timeout,
                comment,
                authtype);

        if (err == GSTAT_OK)
            err = DDGClose(&dds_session);

        if (err == GSTAT_OK)
            err = DDGCommit(&dds_session);
        else
            CLEAR_STATUS(DDGRollback(&dds_session));

        CLEAR_STATUS(DDFSemClose(&dds_semaphore));
    }
    return(err);
}

/*
** Name: DDSGetUser()	- get the current User
**
** Description:
**
** Inputs:
**
** Outputs:
**
** Returns:
**	GSTATUS		: 
**				  GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
**      07-Jan-1999 (fanra01)
**          Add authtype function parameter.
**	13-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
*/

GSTATUS 
DDSGetUser (
    PTR     *cursor,
    u_i4   **id,
    char    **name,
    char    **password,
    u_i4   **dbuser_id,
    char    **comment,
    i4 **flags,
    i4 **timeout,
    i4 **authtype)
{
    GSTATUS err = GSTAT_OK;
    bool    exist;

    err = DDGNext(&dds_session, &exist);
    if (err == GSTAT_OK && exist == TRUE)
        err = DDGProperties(
                &dds_session,
                "%d%s%s%d%d%d%s%d",
                id,
                name,
                password,
                dbuser_id,
                flags,
                timeout,
                comment,
                authtype);
    if (err != GSTAT_OK)
        exist = FALSE;
    *cursor = (char*) (SCALARP)exist;
    return(err);
}

/*
** Name: DDSCloseUser()	- close the cursor on the User list
**
** Description:
**
** Inputs:
**
** Outputs:
**
** Returns:
**	GSTATUS		: 
**				  GSTAT_OK
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
DDSCloseUser () 
{
	GSTATUS err = GSTAT_OK;

	err = DDGClose(&dds_session);

	if (err == GSTAT_OK)
		err = DDGCommit(&dds_session);
	else
		CLEAR_STATUS(DDGRollback(&dds_session));
	CLEAR_STATUS(DDFSemClose(&dds_semaphore));
return(err);
}

/*
** Name: DDSOpenUserResource()	- Open a cursor on the Resource/Role relation
**
** Description:
**	
**
** Inputs:
**
** Outputs:
**
** Returns:
**	GSTATUS		: 
**				  GSTAT_OK
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
DDSOpenUserResource (PTR *cursor, char* resource) 
{
	GSTATUS err = GSTAT_OK;
	err = DDFSemOpen(&dds_semaphore, FALSE);
	if (err == GSTAT_OK)
	{
		DDS_SCANNER *scan;
		err = G_ME_REQ_MEM(0, scan, DDS_SCANNER, 1);
		if (err == GSTAT_OK)
		{
			scan->name = resource;
			*cursor = (PTR) scan;
		}
	}
return(err);
}

/*
** Name: DDSGetUserResource()	- 
**
** Inputs:
**	PTR*		: cursor,
**
** Outputs:
**	u_nat*		: role id
**	longnat*	: rights
**
** Returns:
**	GSTATUS		: 
**				  GSTAT_OK
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
DDSGetUserResource (
	PTR		*cursor,
	u_i4	*id,
	i4	*right) 
{
	GSTATUS err = GSTAT_OK;
	DDS_SCANNER *scan = (DDS_SCANNER*) *cursor;
	err = DDSMemSelectResource (&scan->current, RIGHT_TYPE_USER, scan->name, id, right);
	if (err == GSTAT_OK && scan->current == NULL)
	{
		MEfree((PTR)scan);
		*cursor = NULL;
	}
return(err);
}

/*
** Name: DDSCloseRoleResource()	- close the cursor on the Role list
**
** Description:
**
** Inputs:
**
** Outputs:
**
** Returns:
**	GSTATUS		: 
**				  GSTAT_OK
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
DDSCloseUserResource (PTR *cursor) 
{
	CLEAR_STATUS(DDFSemClose(&dds_semaphore));
return(GSTAT_OK);
}

/*
** Name: DDSGetUserName()	- return the User name
**
** Description:
**
** Inputs:
**
** Outputs:
**
** Returns:
**	GSTATUS		: 
**				  GSTAT_OK
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
DDSGetUserName(
	u_i4	id, 
	char** name)
{
	GSTATUS err = GSTAT_OK;

	err = DDFSemOpen(&dds_semaphore, FALSE);
	if (err == GSTAT_OK) 
	{
		err = DDSMemGetUserName(id, name);
		CLEAR_STATUS(DDFSemClose(&dds_semaphore));
	}
return(err);
}

/*
** Name: DDSGetUserTimeout()	- return the User timeout
**
** Description:
**
** Inputs:
**
** Outputs:
**
** Returns:
**	GSTATUS		: 
**				  GSTAT_OK
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
DDSGetUserTimeout(
	u_i4  id, 
	i4* timeout)
{
	GSTATUS err = GSTAT_OK;

	err = DDFSemOpen(&dds_semaphore, FALSE);
	if (err == GSTAT_OK) 
	{
		err = DDSMemGetUserTimeout(id, timeout);
		CLEAR_STATUS(DDFSemClose(&dds_semaphore));
	}
return(err);
}
