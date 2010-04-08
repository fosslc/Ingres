/******************************************************************************
**
** Copyright (c) 2004 Ingres Corporation
**
******************************************************************************/

#include <ddsmem.h>
#include <ddfhash.h>
#include <erddf.h>
#include <gcu.h>

/**
**
**  Name: ddsmeml.c - Data Dictionary Security Service Facility
**
**  Description:
**		This file contains all functions to manage dds information in memory
**
**  History:    
**	5-Feb-98 (marol01)
**	    created
**  28-May-98 (fanra01)
**      Add timeout parameters to methods.  Add methods for retrieving
**      properties.
**  21-jul-1998 (marol01)
**      add alias property to dbuser structure (this alias has to be unique in 
**			the system rather than the name won't.
**  14-Dec-1998 (fanra01)
**      Add the function DDSMemGetDBUserInfo to return user information based
**      upon the the db user name.
**  07-May-1999 (fanra01)
**      Add gcu header for function prototype.
**  04-Jan-2000 (fanra01)
**      Bug 99923
**      Clear pointers once memory freed in DDSMemTerminate.
**  05-May-2000 (fanra01)
**      Bug 101346
**      Add refs to the rights structure and initialize it based upon input
**      parameter to DDSMemAssignUserRight and DDSMemDeleteUserRight.
**      Update DDSMemAssignUserRight and DDSMemDeleteUserRight to take extra
**      reference count parameter.
**  20-Jul-2000 (fanra01)
**      Bug 102137
**      Remove user rights from memory before removing the user so that when
**      the document is removed no attempt is made to free rights for a
**      non-existent user.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      10-Jan-2001 (fanra01)
**          Bug 103681
**          Corrected dependency checks of web user and database connections
**          during the removal of a dbuser alias from the repository.
**      22-Jun-2001 (fanra01)
**          Bug 105154
**          Made pointers to structures maintained in the hash table
**          homogenous.  The difference was causing occasional failure to match
**          permissions as the rights field was potentially uninitialized.
**      04-Oct-2001 (fanra01)
**          Bug 105961
**          Add range test for valid database user id before use when creating
**          or modifying associations.
**      29-Oct-2001 (fanra01)
**          Bug 106173
**          Ensure consistent treatment of 1 based array indexing for
**          repository items.  Exception occurs when array bounds are
**          exceeded when 0 based array indexing is assumed.
**      18-Jun-2002 (fanra01)
**          Bug 108066
**          Add range check to role id when creating or deleting an association
**          with a user.
**      08-Jul-2002 (fanra01)
**          Bug 108201
**          Add range check to database id when attempting to remove a
**          database association.
**      23-Jul-2002 (fanra01)
**          Bug 108352
**          Adjust range tests to exclude 0 from valid range.
**	27-Jul-2004 (hanje04)
**	   Replace gcu_encode with DDSencode.
**/

typedef struct __DDS_ID_LIST 
{
	u_i4				 id;
	struct __DDS_ID_LIST *previous;
	struct __DDS_ID_LIST *next;
} DDS_ID_LIST;

/*
** Name: DDS_RIGHT
**
** Description:
**      Structure describes access permissions for a named resource.
**
** History:
**      05-May-2000 (fanra01)
**          Add refs field for holding a reference counter for a resource.
*/
typedef struct __DDS_RIGHT
{
    char                *resource;
    u_i4                right;
    i4                  refs;
    struct __DDS_RIGHT  *previous;
    struct __DDS_RIGHT  *next;

    u_i4                type;
    u_i4                id;
    struct __DDS_RIGHT  *pre_global;
    struct __DDS_RIGHT  *next_global;
} DDS_RIGHT;

typedef struct __DDS_DBUSER 
{
	u_i4	id;
	char	*alias;
	char	*name;
	char	*password;
} DDS_DBUSER;

typedef struct __DDS_DB 
{
	u_i4		id;
	char		*name;
	char		*dbname;
	DDS_DBUSER	*dbuser;
	i4		flags;
} DDS_DB;

typedef struct __DDS_ROLE 
{
	u_i4					id;
	char					*name;

	DDS_RIGHT				*rights;
	DDS_ID_LIST			*roles;		/* inherited role */
} DDS_ROLE;

typedef struct __DDS_USER
{
    u_i4       id;
    char        *name;
    char        *password;
    i4     flags;
    i4     timeout;
    i4     authtype;
    DDS_DBUSER  *dbuser;
    DDS_ID_LIST *dbs;

    u_i2        user_tag;
    u_i4       requested;
    DDS_ID_LIST *roles;
    DDS_RIGHT   *rights;
} DDS_USER;

#define	DDS_RIGHT_NAME	ERx("%d_%s")

static DDFHASHTABLE		user_name = NULL;
static DDFHASHTABLE		rights = NULL;
static DDS_USER				**users_id = NULL;
static DDS_DBUSER			**dbusers_id = NULL;
static DDS_ROLE				**roles_id = NULL;
static DDS_DB					**dbs_id = NULL;
static u_i4					dds_dbuser_max = 0;
static u_i4					dds_user_max = 0;
static u_i4					dds_role_max = 0;
static u_i4					dds_db_max = 0;
static u_i2						dds_memory_tag;
struct __DDS_RIGHT		*global_rights = NULL;
/*
** Name: DDSMemInitialize()	- Initialize DDS Memory
**
** Description:
**	Allocate all information needs by the system
**
** Inputs:
**	u_i4	: Maximum user id. Determine the hash table size
**	u_i4	: Maximum role id. Determine the hash table size
**
** Outputs:
**
** Returns:
**	GSTATUS	: 
**			  GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
**      29-Oct-2001 (fanra01)
**          Adjust memory allocations to include unused 0th element in the
**          array.
*/
GSTATUS 
DDSMemInitialize (
	u_i4	concurent_rights, 
	u_i4	max_user) 
{
	GSTATUS err = GSTAT_OK;

	dds_memory_tag = MEgettag();

	err = DDFmkhash(max_user, &user_name);
	if (err == GSTAT_OK)
		err = DDFmkhash(concurent_rights, &rights);

	if (err == GSTAT_OK) 
	{
		err = G_ME_REQ_MEM(
				dds_memory_tag, 
				dbusers_id, 
				DDS_DBUSER*, 
				(DDS_DEFAULT_DBUSER +1));

		dds_dbuser_max = DDS_DEFAULT_DBUSER;
	}
	if (err == GSTAT_OK)
	{
		err = G_ME_REQ_MEM(
				dds_memory_tag, 
				users_id, 
				DDS_USER*, 
				(DDS_DEFAULT_USER + 1));

		dds_user_max = DDS_DEFAULT_USER;
	}
	if (err == GSTAT_OK)
	{
		err = G_ME_REQ_MEM(
				dds_memory_tag, 
				roles_id, 
				DDS_ROLE*, 
				(DDS_DEFAULT_ROLE + 1));

		dds_role_max = DDS_DEFAULT_ROLE;
	}
	if (err == GSTAT_OK)
	{
		err = G_ME_REQ_MEM(
				dds_memory_tag, 
				dbs_id, 
				DDS_DB*, 
				(DDS_DEFAULT_DB + 1));

		dds_db_max = DDS_DEFAULT_DB;
	}

	if (err != GSTAT_OK)
		DDSMemTerminate();
return(err);
}

/*
** Name: DDSMemTerminate()	- Clean DDS Memory
**
** Description:
**	Allocate all information needs by the system
**
** Inputs:
**	u_i4	: Maximum user id. Determine the hash table size
**	u_i4	: Maximum role id. Determine the hash table size
**
** Outputs:
**
** Returns:
**	GSTATUS	: 
**			  GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
**      02-Mar-1999 (fanra01)
**          Check validity of name before attempting to free it.
**      04-Jan-2000 (fanra01)
**          Bug 99923r
**          Clear pointers after freeing.
*/
GSTATUS
DDSMemTerminate ()
{
    GSTATUS err = GSTAT_OK;
    DDFHASHSCAN sc;
    DDS_RIGHT    *object;
    u_i4 i = 0;

    G_ASSERT(users_id == NULL, E_DF0042_DDS_MEM_NOT_INIT);

    if (users_id != NULL)
    {
        u_i4 i;
        for (i = 0; i <= dds_user_max; i++)
            if (users_id[i] != NULL)
                CLEAR_STATUS(DDSMemDeleteUser(i));
    }

    DDF_INIT_SCAN(sc, rights);
    while (DDF_SCAN(sc, object) == TRUE && object != NULL)
    {
        if (object->resource != NULL)
        {
            CLEAR_STATUS(DDFdelhash(rights, object->resource, HASH_ALL,
                (PTR*) &object));
        }
        if (object != NULL)
        {
            MEfree(object->resource);
            object->resource = NULL;
            MEfree((PTR)object);
            object = NULL;
        }
    }

    CLEAR_STATUS(DDFrmhash(&user_name));
    CLEAR_STATUS(DDFrmhash(&rights));

    for (i = 0; i <= dds_dbuser_max; i++)
        if (users_id[i] != NULL)
            MEfreetag(users_id[i]->user_tag);

    MEfreetag(dds_memory_tag);
    return(err);
}


/*
** Name: DDSMemCheckUser()	- Check access and find the corresponding id
**
** Description:
**	
**
** Inputs:
**	char*		: user name
**	char*		: user password. 
**						If the password == NULL, the system doesn't check it.
**
** Outputs:
**	u_i4		: user id 
**
** Returns:
**	GSTATUS		: 
**				  E_OI0032_DDS_BAD_PASSWORD
**				  E_OI0033_DDS_BAD_USER
**				  GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
**      08-Jan-1999 (fanra01)
**          Add OS password authentication and moved  gcu_encode from
**          DDSCheckUser.
**	27-Jul-2004 (hanje04)
**	   Replace gcu_encode with DDSencode.
*/

GSTATUS 
DDSMemCheckUser (
    char *name,
    char *password,
    u_i4 *user_id)
{
    GSTATUS err = GSTAT_OK;
    DDS_USER *user;
    char crypt[DDS_PASSWORD_LENGTH];

    G_ASSERT(users_id == NULL, E_DF0042_DDS_MEM_NOT_INIT);

    err = DDFgethash(user_name, name, HASH_ALL, (PTR*) &user);
    if (err == GSTAT_OK && user != NULL)
    {
        switch (user->authtype)
        {
            case DDS_REPOSITORY_AUTH:
		if (DDSencode(name, password, crypt) == OK)
                {
                    if (password == NULL ||
                        STcompare(user->password, crypt) == 0)
                    {
                        *user_id = user->id;
                    }
                    else
                        err = DDFStatusAlloc (E_DF0043_DDS_BAD_PASSWORD);
                }
                else
                {
                    err = DDFStatusAlloc(E_DF0054_DDS_CRYPT);
                }
                break;

            case DDS_OS_USRPWD_AUTH:
                {
                    CL_ERR_DESC syserr;

                    CL_CLEAR_ERR (&syserr);
                    if (GCusrpwd (name, password, &syserr) == OK)
                    {
                        *user_id = user->id;
                    }
                    else
                        err = DDFStatusAlloc (E_DF0043_DDS_BAD_PASSWORD);
                }
                break;
        }
    }
    else
        err = DDFStatusAlloc (E_DF0044_DDS_BAD_USER);

    return(err);
}

/*
** Name: DDSMemVerifyUserPass()	- Compare user password
**
** Description:
**	
**
** Inputs:
**	u_i4		: user id 
**	char*		: user password. 
**
** Outputs:
**	bool		: result
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
DDSMemVerifyUserPass (
	u_i4 id,
	char *password,
	bool *result) 
{
	GSTATUS err = GSTAT_OK;

	G_ASSERT(users_id == NULL, E_DF0042_DDS_MEM_NOT_INIT);
	G_ASSERT(id < 0 || 
			 id > dds_user_max ||
			 users_id[id] == NULL, 
			 E_DF0045_DDS_UNKNOWN_USER);

	*result = (STcompare(users_id[id]->password, password) == 0);
return(err);
}

/*
** Name: DDSMemVerifyDBUserPass()	- Compare dbuser password
**
** Description:
**	
**
** Inputs:
**	u_i4		: user id 
**	char*		: user password. 
**
** Outputs:
**	bool		: result
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
DDSMemVerifyDBUserPass (
	u_i4 id,
	char *password,
	bool *result) 
{
	GSTATUS err = GSTAT_OK;

	G_ASSERT(dbusers_id == NULL, E_DF0042_DDS_MEM_NOT_INIT);
	G_ASSERT(id < 0 || 
			 id > dds_dbuser_max ||
			 dbusers_id[id] == NULL, 
			 E_DF0051_DDS_UNKNOWN_DBUSER);

	*result = (STcompare(dbusers_id[id]->password, password) == 0);
return(err);
}

/*
** Name: DDSMemCheckUserFlag()	- Match parameter flag with user's flags
**
** Description:
**
** Inputs:
**	u_i4		: user id
**  i4		: flag
**
** Outputs:
**	bool		: result of the comparison.
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
DDSMemCheckUserFlag (
	u_i4 user_id,
	i4 flag,
	bool *result)
{
	G_ASSERT(users_id == NULL, E_DF0042_DDS_MEM_NOT_INIT);
	G_ASSERT(user_id < 0 || 
			 user_id > dds_user_max ||
			 users_id[user_id] == NULL, 
			 E_DF0045_DDS_UNKNOWN_USER);

	*result = (users_id[user_id]->flags & flag) ? TRUE : FALSE;
return(GSTAT_OK);
}

/*
** Name: DDSMemInitCurrentRight()	- Initialize and allocate needed 
**									  memory for defining a current 
**									  right (internal function)
**
** Description:
**
** Inputs:
**	u_i4		: user id
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
DDSMemInitCurrentRight(
    DDS_USER *user,
    DDS_RIGHT *right )
{
    GSTATUS     err = GSTAT_OK;
    DDS_RIGHT   *tmp_right = NULL;
    u_i4        length = 0;
    char*       tmp = NULL;

    err = G_ME_REQ_MEM(user->user_tag, tmp, char,
        STlength(DDS_RIGHT_NAME) +
        NUM_MAX_INT_STR +
        STlength(right->resource) + 1);
    if (err == GSTAT_OK)
    {
        STprintf(tmp, DDS_RIGHT_NAME, user->id, right->resource);
        length = STlength(tmp) + 1;
        err = DDFgethash(rights, tmp, HASH_ALL, (PTR*) &tmp_right);
        if (err == GSTAT_OK && tmp_right == NULL)
        {
            err = G_ME_REQ_MEM(
                user->user_tag,
                tmp_right,
                DDS_RIGHT,
                1);

            if (err == GSTAT_OK)
            {
                err = G_ME_REQ_MEM(
                    user->user_tag,
                    tmp_right->resource,
                    char,
                    length);

                if (err == GSTAT_OK)
                {
                    MECOPY_VAR_MACRO(tmp, length, tmp_right->resource);
                    err = DDFputhash(
                        rights,
                        tmp_right->resource,
                        HASH_ALL,
                        (PTR) tmp_right);
                }
            }
        }
        MEfree((PTR)tmp);
    }
    if (err == GSTAT_OK)
        tmp_right->right |= right->right;
    return(err);
}

/*
** Name: DDSMemRequestUser() - Compile user information 
**							   for a late CheckRight call
**
** Description:
**
** Inputs:
**	u_i4		: user id
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
DDSMemRequestUser( u_i4 user_id )
{
    GSTATUS     err     = GSTAT_OK;
    DDS_USER    *user   = NULL;
    DDS_RIGHT   *right  = NULL;
    DDS_ID_LIST *role   = NULL;
    u_i4        length  = 0;

    G_ASSERT(users_id == NULL, E_DF0042_DDS_MEM_NOT_INIT);
    G_ASSERT(user_id < 0 ||
             user_id > dds_user_max ||
             users_id[user_id] == NULL,
             E_DF0045_DDS_UNKNOWN_USER);

    user = users_id[user_id];
    if (user->requested == 0)
    {
        user->user_tag = MEgettag();
        /* scan user rights */
        right = user->rights;
        while (err == GSTAT_OK && right != NULL)
        {
            err = DDSMemInitCurrentRight(user, right);
            right = right->next;
        }

        /* scan role rights */
        role = user->roles;
        while (role != NULL)
        {
            right = roles_id[role->id]->rights;
            while (err == GSTAT_OK && right != NULL)
            {
                err = DDSMemInitCurrentRight(user, right);
                right = right->next;
            }
            role = role->next;
        }
    }
    user->requested++;
    return(err);
}

/*
** Name: DDSMemCheckRight()	- Check access right 
**
** Description:
**	
**
** Inputs:
**	u_i4		: user id
**	char*		: resource name
**	u_i4		: right
**
** Outputs:
**	bool		: result
**
** Returns:
**	GSTATUS		: 
**				  E_OI0043_DDS_UNAUTH_ACCESS
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
DDSMemCheckRight(
	u_i4	user,
	char*	resource, 
	i4 right,
	bool	*result) 
{
	GSTATUS err = GSTAT_OK;
	DDS_RIGHT *tmp_right = NULL;
	char*	tmp = NULL;

	G_ASSERT(users_id == NULL, E_DF0042_DDS_MEM_NOT_INIT);
	G_ASSERT(user < 0 || 
			 user > dds_user_max ||
			 users_id[user] == NULL, 
			 E_DF0045_DDS_UNKNOWN_USER);

	err = G_ME_REQ_MEM(0, tmp, char, 
										 STlength(DDS_RIGHT_NAME) + 
										 NUM_MAX_INT_STR + 
										 STlength(resource) + 1);
	if (err == GSTAT_OK)
	{
		STprintf(tmp, DDS_RIGHT_NAME, user, resource);
		err = DDFgethash(rights, tmp, HASH_ALL, (PTR*) &tmp_right);
		*result = (err == GSTAT_OK && 
					   tmp_right != NULL && 
						 (tmp_right->right & right));
		MEfree((PTR)tmp);
	}
return(err);
}

/*
** Name: DDSMemCheckRightRef
**
** Description:
**      Check the the right and return the refs count.
**
** Inputs:
**      user id
**      resource name
**      right
**
** Outputs:
**      result
**      refs
**
** Returns:
**      GSTATUS:
**          E_OI0043_DDS_UNAUTH_ACCESS
**          GSTAT_OK
**
** Exceptions:
**      None
**
** Side Effects:
**      None
**
** History:
**      05-May-2000 (fanra01)
**          Created.
*/
GSTATUS 
DDSMemCheckRightRef(
    u_i4    user,
    char*   resource,
    i4      right,
    i4*     refs,
    bool    *result )
{
    GSTATUS err = GSTAT_OK;
    DDS_RIGHT *tmp_right = NULL;
    char*    tmp = NULL;

    G_ASSERT(users_id == NULL, E_DF0042_DDS_MEM_NOT_INIT);
    G_ASSERT(user < 0 ||
             user > dds_user_max ||
             users_id[user] == NULL,
             E_DF0045_DDS_UNKNOWN_USER);

    err = G_ME_REQ_MEM(0, tmp, char,
        STlength(DDS_RIGHT_NAME) +
        NUM_MAX_INT_STR +
        STlength(resource) + 1);

    if (err == GSTAT_OK)
    {
        STprintf(tmp, DDS_RIGHT_NAME, user, resource);
        err = DDFgethash(rights, tmp, HASH_ALL, (PTR*) &tmp_right);
        *result = (err == GSTAT_OK &&
                       tmp_right != NULL &&
                         (tmp_right->right & right));
        if (refs)
        {
            *refs = (tmp_right != NULL) ? tmp_right->refs : 0;
        }
        MEfree((PTR)tmp);
    }
    return(err);
}

/*
** Name: DDSMemUnInitCurrentRight()	- UnInitialize a current 
**									  right (internal function)
**
** Description:
**
** Inputs:
**	u_i4		: user id
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
DDSMemUnInitCurrentRight(
    DDS_USER *user,
    DDS_RIGHT *right )
{
    GSTATUS     err = GSTAT_OK;
    DDS_RIGHT   *tmp_right = NULL;
    char*       tmp = NULL;

    err = G_ME_REQ_MEM(user->user_tag, tmp, char,
        STlength(DDS_RIGHT_NAME) +
        NUM_MAX_INT_STR +
        STlength(right->resource) + 1);
    if (err == GSTAT_OK)
    {
        STprintf(tmp, DDS_RIGHT_NAME, user->id, right->resource);
        err = DDFgethash( rights, tmp, HASH_ALL, (PTR*) &tmp_right);
        if (err == GSTAT_OK && tmp_right != NULL)
        {
            tmp_right->right &= (right->right ^ tmp_right->right);
            if (right->right == 0)
            {
                err = DDFdelhash( rights, tmp, HASH_ALL, (PTR*) &tmp_right);
            }
        }
        MEfree((PTR)tmp);
    }
    return(err);
}

/*
** Name: DDSMemPurgeRight()	- Internal function
**
** Description:
**	
**
** Inputs:
**	DDS_RIGHT *tmp_right
**
** Outputs:
**
** Returns:
**	GSTATUS	: 
**			  GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
**      20-Jul-2000 (fanra01)
**          If the user has been deleted the entry is null.
**          Test that the right for a user id is not null before using it.
*/

GSTATUS 
DDSMemPurgeRight (
    DDS_RIGHT *tmp_right)
{
    GSTATUS         err = GSTAT_OK;
    DDS_ID_LIST     *role    = NULL;
    u_i4            i;

    if (tmp_right->next != NULL)
        tmp_right->next->previous = tmp_right->previous;

    if (tmp_right->previous == NULL)
    {
        if (tmp_right->type == RIGHT_TYPE_USER)
        {
            if (users_id[tmp_right->id] != NULL)
            {
                users_id[tmp_right->id]->rights = tmp_right->next;
            }
        }
        else
        {
            roles_id[tmp_right->id]->rights = tmp_right->next;
        }
    }
    else
        tmp_right->previous->next = tmp_right->next;

    if (tmp_right->next_global != NULL)
        tmp_right->next_global->pre_global = tmp_right->pre_global;

    if (tmp_right->pre_global == NULL)
        global_rights = tmp_right->next_global;
    else
        tmp_right->pre_global->next_global = tmp_right->next_global;

    if (tmp_right->type == RIGHT_TYPE_USER)
    {
        if ((users_id[tmp_right->id] != NULL) &&
            (users_id[tmp_right->id]->requested > 0))
        {
            err = DDSMemUnInitCurrentRight (users_id[tmp_right->id], tmp_right);
        }
    }
    else
        for (i = 0; i <= dds_user_max; i++)
            if (users_id[i] != NULL &&
                users_id[i]->requested > 0)
            {
                role = users_id[i]->roles;
                while (role != NULL)
                {
                    if (role->id == tmp_right->id)
                        err = DDSMemUnInitCurrentRight (users_id[i], tmp_right);
                    role = role->next;
                }
            }

    MEfree((PTR)tmp_right);
    return(err);
}

/*
** Name: DDSMemReleaseUser()	- Release a user previously requested
**
** Description:
**	
**
** Inputs:
**	u_i4		: user id
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
DDSMemReleaseUser(
    u_i4 user_id )
{
    GSTATUS     err = GSTAT_OK;
    DDS_USER    *user = NULL;
    DDS_RIGHT   *right = NULL;
    DDS_ID_LIST *role = NULL;
    DDS_RIGHT   *tmp_right = NULL;
    u_i4        length = 0;

    G_ASSERT(users_id == NULL, E_DF0042_DDS_MEM_NOT_INIT);
    G_ASSERT(user_id < 0 ||
             user_id > dds_user_max ||
             users_id[user_id] == NULL,
             E_DF0045_DDS_UNKNOWN_USER);

    user = users_id[user_id];
    user->requested--;

    if (user->requested == 0)
    {
        /* scan user rights */
        right = user->rights;
        while (err == GSTAT_OK && right != NULL)
        {
            err = DDSMemUnInitCurrentRight (user, right);
            right = right->next;
        }

        /* scan role rights */
        role = user->roles;
        while (role != NULL)
        {
            right = roles_id[role->id]->rights;
            while (err == GSTAT_OK && right != NULL)
            {
                err = DDSMemUnInitCurrentRight (user, right);
                right = right->next;
            }
            role = role->next;
        }
        MEfreetag(user->user_tag);
    }
    return(err);
}

/*
** Name: DDSPerformDBUserId()	- find the first available id
**
** Description:
**	
**
** Inputs:
**
** Outputs:
**	u_i4*		: available id
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
DDSPerformDBUserId (
	u_i4 *id) 
{
	GSTATUS err = GSTAT_OK;
	u_i4	i = 1;

	for (i = 1; i <= dds_dbuser_max && dbusers_id[i] != NULL; i++) ;

	*id = i;
return(err);
}

/*
** Name: DDSMemCreateDBUser()	- Create a new database user
**
** Description:
**	
**
** Inputs:
**	u_i4	: user id
**	char*	: name
**	char*	: password
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
DDSMemCreateDBUser (
	u_i4 id, 
	char *alias,
	char *name, 
	char *password) 
{
	GSTATUS		err		= GSTAT_OK;
	DDS_DBUSER	*user	= NULL;
	u_i4		i;

	G_ASSERT(users_id == NULL, E_DF0042_DDS_MEM_NOT_INIT);
	G_ASSERT(id < 0, E_DF0053_DDS_INV_ID);
	G_ASSERT(id <= dds_dbuser_max &&
			 dbusers_id[id] != NULL, 
			 E_DF0052_DDS_EXISTING_DBUSER);

	for (i = 0; i <= dds_dbuser_max; i++)
		G_ASSERT(dbusers_id[i] != NULL &&
				 STcompare(dbusers_id[i]->alias, alias) == 0, 
				 E_DF0052_DDS_EXISTING_DBUSER);
		
	if (id > dds_dbuser_max)
	{
		DDS_DBUSER* *tmp = dbusers_id;
		err = G_ME_REQ_MEM(
					dds_memory_tag, 
					dbusers_id, 
					DDS_DBUSER*, 
					id + DDS_DEFAULT_DBUSER);

		if (err == GSTAT_OK && tmp != NULL) 
			MECOPY_CONST_MACRO(
					tmp, 
					sizeof(DDS_DBUSER*) * (dds_dbuser_max + 1),
					dbusers_id);

		if (err == GSTAT_OK)
		{
			MEfree((PTR)tmp);
			dds_dbuser_max = id + DDS_DEFAULT_DBUSER;
		}
		else
			dbusers_id = tmp;
	}

	if (err == GSTAT_OK) 
	{
		err = G_ME_REQ_MEM(dds_memory_tag, user, DDS_DBUSER, 1);
		if (err == GSTAT_OK) 
		{
			u_i4 length;
			length = STlength(alias)+1;
			err = G_ME_REQ_MEM(dds_memory_tag, user->alias, char, length);
			if (err == GSTAT_OK) 
			{
				MECOPY_CONST_MACRO(alias, length, user->alias);
				length = STlength(name)+1;
				err = G_ME_REQ_MEM(dds_memory_tag, user->name, char, length);
				if (err == GSTAT_OK) 
				{
					MECOPY_CONST_MACRO(name, length, user->name);
					length = STlength(password)+1;

					err = G_ME_REQ_MEM(
										dds_memory_tag, 
										user->password, 
										char, 
										length);

					if (err == GSTAT_OK)
						MECOPY_CONST_MACRO(password, 
										length, 
										user->password);
				}
			}
			user->id = id;
		}
	}
	if (err == GSTAT_OK) 
	{
		dbusers_id[id] = user;
	}
	else
	{
		if (user != NULL)
		{
			MEfree((PTR)user->alias);
			MEfree((PTR)user->name);
			MEfree((PTR)user->password);
			MEfree((PTR)user);
		}
	}
return(err);
}

/*
** Name: DDSMemUpdateDBUser()	- update a database user
**
** Description:
**	
**
** Inputs:
**	u_i4	: user id
**	char*	: name
**	char*	: password
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
DDSMemUpdateDBUser (
	u_i4 id, 
	char *alias,
	char *name, 
	char *password) 
{
	GSTATUS err = GSTAT_OK;
	char *tmp_alias;
	char *tmp_name;
	char *tmp_password;
	u_i4 length;
	u_i4 i;
	G_ASSERT(dbusers_id == NULL, E_DF0042_DDS_MEM_NOT_INIT);
	G_ASSERT(id < 0 || 
			 id > dds_dbuser_max ||
			 dbusers_id[id] == NULL, 
			 E_DF0051_DDS_UNKNOWN_DBUSER);

	for (i = 0; i <= dds_dbuser_max; i++)
		G_ASSERT(i != id && dbusers_id[i] != NULL &&
				 STcompare(dbusers_id[i]->alias, alias) == 0, 
				 E_DF0052_DDS_EXISTING_DBUSER);

	tmp_alias = dbusers_id[id]->alias;
	tmp_name = dbusers_id[id]->name;
	tmp_password = dbusers_id[id]->password;
	dbusers_id[id]->alias = NULL;
	dbusers_id[id]->name = NULL;
	dbusers_id[id]->password = NULL;

	length = STlength(alias)+1;
	err = G_ME_REQ_MEM(dds_memory_tag, dbusers_id[id]->alias, char, length);
	if (err == GSTAT_OK) 
	{
		MECOPY_CONST_MACRO(alias, length, dbusers_id[id]->alias);
		length = STlength(name)+1;
		err = G_ME_REQ_MEM(dds_memory_tag, dbusers_id[id]->name, char, length);
		if (err == GSTAT_OK) 
		{
			MECOPY_CONST_MACRO(name, length, dbusers_id[id]->name);
			length = STlength(password)+1;
			err = G_ME_REQ_MEM(
										dds_memory_tag, 
										dbusers_id[id]->password, 
										char, 
										length);

			if (err == GSTAT_OK)
				MECOPY_CONST_MACRO(password, length, dbusers_id[id]->password);
		}
	}

	if (err == GSTAT_OK)
	{
		MEfree((PTR)tmp_alias);
		MEfree((PTR)tmp_name);
		MEfree((PTR)tmp_password);
	}
	else 
	{
		MEfree((PTR)dbusers_id[id]->alias);
		MEfree((PTR)dbusers_id[id]->password);
		MEfree((PTR)dbusers_id[id]->name);
		dbusers_id[id]->password = tmp_password;
		dbusers_id[id]->name = tmp_name;
		dbusers_id[id]->alias = tmp_alias;
	}
return(err);
}

/*
** Name: DDSMemDeleteDBUser()	- delete a database user from memory
**
** Description:
**	
**
** Inputs:
**	u_i4	: user id
**	char*	: name
**	char*	: password
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
**      10-Jan-2001 (fanra01)
**          Corrected index counter as the attributes array start from 1,
**          which allows removal of dbuser when a connection or an ice user
**          still references it.
*/
GSTATUS
DDSMemDeleteDBUser( u_i4 id )
{
    GSTATUS err = GSTAT_OK;
    u_i4 i;
    DDS_DBUSER *dbuser;

    G_ASSERT(dbusers_id == NULL, E_DF0042_DDS_MEM_NOT_INIT);
    G_ASSERT(id < 0 ||
             id > dds_dbuser_max ||
             dbusers_id[id] == NULL,
             E_DF0051_DDS_UNKNOWN_DBUSER);

    dbuser = dbusers_id[id];

    for (i = 1; err == GSTAT_OK && i <= dds_user_max; i++)
        if (users_id[i] != NULL && users_id[i]->dbuser == dbuser)
            err = DDFStatusAlloc(E_DF0055_DDS_DBUSER_USED);

    for (i = 1; err == GSTAT_OK && i <= dds_db_max; i++)
        if (dbs_id[i] != NULL && dbs_id[i]->dbuser == dbuser)
            err = DDFStatusAlloc(E_DF0055_DDS_DBUSER_USED);

    if (err == GSTAT_OK)
    {
        MEfree((PTR)dbusers_id[id]->password);
        MEfree((PTR)dbusers_id[id]->name);
        MEfree((PTR)dbusers_id[id]->alias);
        MEfree((PTR)dbusers_id[id]);
        dbusers_id[id] = NULL;
    }
    return(err);
}

/*
** Name: DDSMemGetDBForUser()	- retrieve a database user and 
**							  password from a database name
**
** Description:
**	
**
** Inputs:
**	u_i4		: user id
**	char*		: db name
**
** Outputs:
**	char*		: physical database name
**	char*		: database user name
**	char*		: database user password
**	i4		: flags
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
DDSMemGetDBForUser(
	u_i4	user_id,
	char	*name,
	char	**dbname, 
	char	**user, 
	char	**password, 
	i4 *flags)
{
	GSTATUS err = GSTAT_OK;
	DDS_ID_LIST *dbs;
	DDS_DB *db;
	*user = NULL;
	*password = NULL;

	G_ASSERT(users_id == NULL, E_DF0042_DDS_MEM_NOT_INIT);
	G_ASSERT(user_id < 0 || 
			 user_id > dds_user_max ||
			 users_id[user_id] == NULL, 
			 E_DF0045_DDS_UNKNOWN_USER);

	dbs = users_id[user_id]->dbs;
	*user = NULL;
	*dbname = NULL;
	*password = NULL;
	*flags = 0;

	while (dbs != NULL) 
	{
		db = dbs_id[dbs->id];
		if (STcompare(db->name, name) == 0) 
		{
			*dbname = db->dbname;
			*user = db->dbuser->name;
			*password = db->dbuser->password;
			*flags = db->flags;
			break;
		}
		dbs = dbs->next;
	}

	if (*dbname == NULL && users_id[user_id]->dbuser != NULL)
	{
		*dbname = name;
		*user = users_id[user_id]->dbuser->name;
		*password = users_id[user_id]->dbuser->password;
	}
	
	if (*dbname == NULL)
	{
		*dbname = name;
		*user = users_id[user_id]->name;
		*password = users_id[user_id]->password;
	}
return(err);
}

/*
** Name: DDSPerformUserId()	- find the first available id
**
** Description:
**	
**
** Inputs:
**
** Outputs:
**	u_i4*		: available id
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
DDSPerformUserId (
	u_i4 *id) 
{
	GSTATUS err = GSTAT_OK;
	u_i4	i = 1;

	for (i = 1; i <= dds_user_max && users_id[i] != NULL; i++) ;

	*id = i;
return(err);
}

/*
** Name: DDSMemCreateUser()	- Create a new user into the memory 
**
** Description:
**	
**
** Inputs:
**	u_i4	: user id
**	char*	: name
**	char*	: password
**	i4	: flags
**	i4	: timeout
**	u_i4	: database user id
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
DDSMemCreateUser (
    u_i4 id,
    char *name,
    char *password,
    i4 flags,
    i4 timeout,
    u_i4 dbuser_id,
        i4 authtype)
{
    GSTATUS err = GSTAT_OK;
    DDS_USER *user = NULL;

    G_ASSERT(users_id == NULL, E_DF0042_DDS_MEM_NOT_INIT);
    G_ASSERT(id < 0, E_DF0053_DDS_INV_ID);
    G_ASSERT(id <= dds_user_max &&
             users_id[id] != NULL,
             E_DF0049_DDS_EXISTING_USER);

    err = DDFgethash(user_name, name, HASH_ALL, (PTR*) &user);
    if (err == GSTAT_OK)
        G_ASSERT(user != NULL, E_DF0049_DDS_EXISTING_USER);

    if (id > dds_user_max)
    {
        DDS_USER* *tmp = users_id;
        err = G_ME_REQ_MEM(
                    dds_memory_tag,
                    users_id,
                    DDS_USER*,
                    id + DDS_DEFAULT_USER );

        if (err == GSTAT_OK && tmp != NULL)
            MECOPY_CONST_MACRO(tmp, sizeof(DDS_USER*)*(dds_user_max+1), users_id);
        if (err == GSTAT_OK)
        {
            MEfree((PTR)tmp);
            dds_user_max = id + DDS_DEFAULT_USER;
        }
        else
            users_id = tmp;
    }

    if (err == GSTAT_OK)
    {
        err = G_ME_REQ_MEM(dds_memory_tag, user, DDS_USER, 1);
        if (err == GSTAT_OK)
        {
            u_i4 length;
            length = STlength(name)+1;
            err = G_ME_REQ_MEM(dds_memory_tag, user->name, char, length);
            if (err == GSTAT_OK)
            {
                MECOPY_CONST_MACRO(name, length, user->name);
                length = STlength(password)+1;
                err = G_ME_REQ_MEM(dds_memory_tag,user->password,char,length);
                if (err == GSTAT_OK)
                {
                    MECOPY_CONST_MACRO(password, length, user->password);
                    user->id = id;
                    user->flags = flags;
                    user->timeout = timeout;
                                        user->authtype = authtype;
                    user->dbuser = dbusers_id[dbuser_id];
                    err = DDFputhash(user_name, user->name, HASH_ALL, (PTR)user);
                }
            }
        }
    }
    if (err == GSTAT_OK)
    {
        users_id[id] = user;
    }
    else
    {
        if (user != NULL)
        {
            MEfree((PTR)user->name);
            MEfree((PTR)user->password);
        }
        MEfree((PTR)user);
    }
    return(err);
}

/*
** Name: DDSMemUpdateUser()	- Update a user from the memory 
**
** Description:
**	
**
** Inputs:
**	u_i4	: user id
**	char*	: name
**	char*	: password
**	i4	: flags
**	i4	: timeout
**	u_i4	: database user id
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
**          Add authtype function parameter.  Also tidied restore on failure.
*/

GSTATUS 
DDSMemUpdateUser (
    u_i4   id,
    char    *name,
    char    *password,
    i4 flags,
    i4 timeout,
    u_i4   dbuser_id,
    i4 authtype)
{
    GSTATUS err = GSTAT_OK;
    DDS_USER *user;

    G_ASSERT(users_id == NULL, E_DF0042_DDS_MEM_NOT_INIT);
    G_ASSERT(id < 0 ||
             id > dds_user_max ||
             users_id[id] == NULL,
             E_DF0045_DDS_UNKNOWN_USER);

    err = DDFgethash(user_name, name, HASH_ALL, (PTR*) &user);
    if (err == GSTAT_OK)
        G_ASSERT(user != NULL && user->id != id, E_DF0049_DDS_EXISTING_USER);

    err = DDFdelhash(user_name, users_id[id]->name, HASH_ALL, (PTR*) &user);
    if (err == GSTAT_OK && user != NULL)
    {
        DDS_USER    tmp_user;
        char        *tmp_name;
        char        *tmp_password;
        u_i4       length;

        /*
        ** Save original values
        */
        MECOPY_CONST_MACRO (user, sizeof(DDS_USER), &tmp_user);

        tmp_name = user->name;
        tmp_password = user->password;
        user->name = NULL;
        user->password = NULL;

        length = STlength(name)+1;
        err = G_ME_REQ_MEM(dds_memory_tag, user->name, char, length);
        if (err == GSTAT_OK)
        {
            MECOPY_CONST_MACRO(name, length, user->name);
            length = STlength(password)+1;
            err = G_ME_REQ_MEM(dds_memory_tag, user->password, char, length);
            if (err == GSTAT_OK)
                MECOPY_CONST_MACRO(password, length, user->password);
        }
        if (err == GSTAT_OK)
        {
            user->dbuser = dbusers_id[dbuser_id];
            user->flags = flags;
            user->timeout = timeout;
            user->authtype = authtype;
            MEfree((PTR)tmp_name);
            MEfree((PTR)tmp_password);
        }
        else
        {
            MEfree((PTR)user->password);
            MEfree((PTR)user->name);
            /*
            ** Restore the original values
            */
            MECOPY_CONST_MACRO (&tmp_user, sizeof(DDS_USER), user);
        }
        err = DDFputhash(user_name, user->name, HASH_ALL, (PTR) user);
    }
return(err);
}

/*
** Name: DDSMemDeleteUser()	- Delete a user from the memory
**
** Description:
**	
**
** Inputs:
**	u_i4	: user id
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
**      20-Jul-2000 (fanra01)
**          Remove user rights from memory before removing the user.
*/
GSTATUS 
DDSMemDeleteUser (
    u_i4 id)
{
    GSTATUS     err = GSTAT_OK;
    DDS_USER    *user;
    DDS_RIGHT   *tmp_right = NULL;

    G_ASSERT(users_id == NULL, E_DF0042_DDS_MEM_NOT_INIT);
    G_ASSERT(users_id[id] == NULL, E_DF0045_DDS_UNKNOWN_USER);

    while ((tmp_right = users_id[id]->rights) != NULL)
    {
        err = DDSMemPurgeRight( tmp_right );
    }

    err = DDFdelhash(user_name, users_id[id]->name, HASH_ALL, (PTR*) &user);
    if (err == GSTAT_OK && user != NULL)
    {
        if (user->requested > 0)
        {
            user->requested = 0;
            DDSMemReleaseUser(id);
        }
        MEfree((PTR)user->name);
        MEfree((PTR)user->password);
        MEfree((PTR)users_id[id]);
    }
    users_id[id] = NULL;
    return(err);
}

/*
** Name: DDSMemAssignUserRight
**
** Description:
**      Assigns a right for a resource to a user.
**
** Inputs:
**	user        user to assign permission
**	resource    name of the resource
**	right       the permission to give user
**      refs        update reference count flag
**
** Outputs:
**      right       updated permission flags
**      refs        value of the reference counter
**      created     indicator for new user right.
**
** Returns:
**	GSTATUS	: 
**	    GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
**      05-May-2000 (fanra01)
**          Add the use and update of the refs variable which enables
**          the update of reference count for the right.
*/
GSTATUS
DDSMemAssignUserRight(
    u_i4 user,
    char *resource,
    i4   *right,
    i4   *refs,
    bool *created)
{
    GSTATUS err = GSTAT_OK;
    DDS_RIGHT *tmp_right = NULL;

    G_ASSERT(users_id == NULL, E_DF0042_DDS_MEM_NOT_INIT);
    G_ASSERT(user < 0 ||
             user > dds_user_max ||
             users_id[user] == NULL,
             E_DF0045_DDS_UNKNOWN_USER);

    tmp_right = users_id[user]->rights;
    while (tmp_right != NULL) {
        if (STcompare(tmp_right->resource, resource) == 0)
            break;
        tmp_right =tmp_right->next;
    }

    if (tmp_right == NULL)
    {
        err = G_ME_REQ_MEM(dds_memory_tag, tmp_right, DDS_RIGHT, 1);
        if (err == GSTAT_OK)
        {
            err = G_ST_ALLOC(tmp_right->resource, resource);
            if (err == GSTAT_OK)
            {
                /*
                ** default value of refs is 0.
                */
                tmp_right->refs = 0;
                tmp_right->previous = NULL;
                tmp_right->next = users_id[user]->rights;
                if (users_id[user]->rights != NULL)
                    users_id[user]->rights->previous = tmp_right;
                users_id[user]->rights = tmp_right;

                tmp_right->type = RIGHT_TYPE_USER;
                tmp_right->id = user;
                tmp_right->next_global = global_rights;
                if (global_rights != NULL)
                    global_rights->pre_global = tmp_right;
                global_rights = tmp_right;
            }
        }
        *created = TRUE;
    }
    else
        *created = FALSE;

    if (err == GSTAT_OK)
    {
        /*
        ** only update the refs counter if the caller has enabled it.
        */
        if (refs)
        {
            if (*refs != 0)
            {
                tmp_right->refs += 1;
            }
            /*
            ** Return reference count for table update.
            */
            *refs = tmp_right->refs;
        }
        tmp_right->right |= (*right);
        *right = tmp_right->right;
    }
    else
    {
        if (refs)
        {
            *refs = 0;
        }
        *right = 0;
    }
    if (err == GSTAT_OK && users_id[user]->requested > 0)
        err = DDSMemInitCurrentRight(users_id[user], tmp_right);
    return(err);
}

/*
** Name: DDSMemDeleteUserRight
**
** Description:
**	Removes a right for a resource from a user.  When the user has no
**      rights the entry is removed from memory and database.
**
** Inputs:
**	user        User to remove right from
**	resource    Resource name of object
**	right       Permissions to remove
**      refs        Reference counter flag
**
** Outputs:
**      right       Remaining rights after removal
**      refs        Remaining references
**      deleted     If entry remove
**
** Returns:
**	GSTATUS	: 
**	    GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
**      05-May-2000 (fanra01)
**          Add the use and update of the refs variable which enables
**          the update of reference count for the right.
**
*/
GSTATUS
DDSMemDeleteUserRight (
    u_i4  user,
    char  *resource,
    i4    *right,
    i4    *refs,
    bool  *deleted)
{
    GSTATUS err = GSTAT_OK;
    DDS_RIGHT *tmp_right = NULL;

    G_ASSERT(users_id == NULL, E_DF0042_DDS_MEM_NOT_INIT);
    G_ASSERT(user < 0 ||
             user > dds_user_max ||
             users_id[user] == NULL,
             E_DF0045_DDS_UNKNOWN_USER);

    tmp_right = users_id[user]->rights;
    while (tmp_right != NULL) {
        if (STcompare(tmp_right->resource, resource) == 0)
            break;
        tmp_right = tmp_right->next;
    }

    if (tmp_right != NULL)
    {
        /*
        ** only update the refs counter if the caller has enabled it.
        */
        if (refs)
        {
            /*
            ** If reference count should be modified and the actual count
            ** is not already zero then decrement the actual count.
            */
            if ((*refs != 0) && (tmp_right->refs != 0))
            {
                tmp_right->refs -= 1;
            }
            /*
            ** If the reference count is zero it is a general access right
            ** and the right should be removed.
            */
            if (tmp_right->refs == 0)
            {
                tmp_right->right &= ((*right) ^ tmp_right->right);
            }
            /*
            ** Return reference count for update
            */
            *refs = tmp_right->refs;
        }
        else
        {
            if (tmp_right->refs == 0)
            {
                tmp_right->right &= ((*right) ^ tmp_right->right);
            }
        }
        *right = tmp_right->right;
        /*
        ** only delete the rights which are no longer refereneced.
        */
        *deleted = ((tmp_right->right == 0) && (tmp_right->refs == 0));

        if (*deleted == TRUE)
            err = DDSMemPurgeRight(tmp_right);
        else
            err = DDSMemUnInitCurrentRight (users_id[user], tmp_right);
    }
    else
        *deleted = FALSE;
    return(err);
}
/*
** Name: DDSPerformRoleId()	- find the first available id
**
** Description:
**	
**
** Inputs:
**
** Outputs:
**	u_i4*		: available id
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
DDSPerformRoleId (
	u_i4 *id) 
{
	GSTATUS err = GSTAT_OK;
	u_i4	i = 1;

	for (i = 1; i <= dds_role_max && roles_id[i] != NULL; i++) ;

	*id = i;
return(err);
}

/*
** Name: DDSMemCreateRole()	- Create a new role in memory
**
** Description:
**	
**
** Inputs:
**	u_i4	: role id
**	char*	: name
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
DDSMemCreateRole (
	u_i4 id, 
	char *name) 
{
	GSTATUS err = GSTAT_OK;
	u_i4	i;

	G_ASSERT(roles_id == NULL, E_DF0042_DDS_MEM_NOT_INIT);
	G_ASSERT(id < 0, E_DF0053_DDS_INV_ID);
	G_ASSERT(id <= dds_role_max &&
			 roles_id[id] != NULL, 
			 E_DF0050_DDS_EXISTING_ROLE);

	for (i = 0; i <= dds_role_max; i++)
		G_ASSERT(roles_id[i] != NULL &&
				 STcompare(roles_id[i]->name, name) == 0, 
				 E_DF0050_DDS_EXISTING_ROLE);

	if (id > dds_role_max)
	{
		DDS_ROLE* *tmp = roles_id;
		err = G_ME_REQ_MEM(
					dds_memory_tag, 
					roles_id, 
					DDS_ROLE*, 
					id + DDS_DEFAULT_ROLE);

		if (err == GSTAT_OK && tmp != NULL) 
			MECOPY_CONST_MACRO(tmp, sizeof(DDS_ROLE*)*(dds_role_max + 1), roles_id);
		if (err == GSTAT_OK)
		{
			MEfree((PTR)tmp);
			dds_role_max = id + DDS_DEFAULT_ROLE;
		}
		else
			roles_id = tmp;
	}

	err = G_ME_REQ_MEM(dds_memory_tag, roles_id[id], DDS_ROLE, 1);
	if (err == GSTAT_OK) 
	{
		u_i4 length;
		length = STlength(name)+1;
		err = G_ME_REQ_MEM(dds_memory_tag, roles_id[id]->name, char, length);
		if (err == GSTAT_OK) 
		{
			MECOPY_CONST_MACRO(name, length, roles_id[id]->name);
			roles_id[id]->id = id;
		}
	}
	if (err != GSTAT_OK) 
	{
		if (roles_id[id])
			MEfree(roles_id[id]->name);
		MEfree((PTR)roles_id[id]);
		roles_id[id] = NULL;
	}
return(err);
}

/*
** Name: DDSMemUpdateRole()	- Create a new role in memory
**
** Description:
**	
**
** Inputs:
**	u_i4	: role id
**	char*	: name
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
DDSMemUpdateRole (
	u_i4 id, 
	char *name) 
{
	GSTATUS err = GSTAT_OK;
	u_i4	 i;

	G_ASSERT(roles_id == NULL, E_DF0042_DDS_MEM_NOT_INIT);
	G_ASSERT(id < 0 || 
			 id > dds_role_max ||
			 roles_id[id] == NULL, 
			 E_DF0046_DDS_UNKNOWN_ROLE);

	for (i = 0; i <= dds_role_max; i++)
		G_ASSERT(i != id && roles_id[i] != NULL &&
				 STcompare(roles_id[i]->name, name) == 0, 
				 E_DF0050_DDS_EXISTING_ROLE);

	if (err == GSTAT_OK && roles_id[id])
	{
		char *tmp = roles_id[id]->name;
		u_i4 length;

		length = STlength(name)+1;
		err = G_ME_REQ_MEM(dds_memory_tag, roles_id[id]->name, char, length);
		if (err == GSTAT_OK) 
			MECOPY_CONST_MACRO(name, length, roles_id[id]->name);

		if (err == GSTAT_OK) 
			MEfree(tmp);
		else
		{
			MEfree(roles_id[id]->name);
			roles_id[id]->name = tmp;
		}
	}
return(err);
}

/*
** Name: DDSMemDeleteRole()	- delete role from memory
**
** Description:
**	
**
** Inputs:
**	u_i4	: role id
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
DDSMemDeleteRole (
	u_i4 id) 
{
	GSTATUS err = GSTAT_OK;
	DDS_ID_LIST	*id_list = NULL;
	DDS_ID_LIST	*tmp = NULL;
	u_i4		i = 0;

	G_ASSERT(roles_id == NULL, E_DF0042_DDS_MEM_NOT_INIT);
	G_ASSERT(id < 0 || 
			 id > dds_role_max ||
			 roles_id[id] == NULL, 
			 E_DF0046_DDS_UNKNOWN_ROLE);

	while (err == GSTAT_OK && roles_id[id]->rights != NULL)
		err = DDSMemPurgeRight(roles_id[id]->rights);

	for (i = 0; err == GSTAT_OK && i <= dds_user_max; i++)
		if (users_id[i] != NULL)
		{
			id_list = users_id[i]->roles;
			while (err == GSTAT_OK && id_list != NULL)
			{
				tmp = id_list;
				id_list = id_list->next;
				if (tmp->id == id)
					err = DDFStatusAlloc( E_DF0056_DDS_ROLE_USED );
			}
		}
	
	if (err == GSTAT_OK)
	{
		if (roles_id[id] != NULL)
		{
			MEfree(roles_id[id]->name);
			MEfree((PTR)roles_id[id]);
			roles_id[id] = NULL;
		}
	}
return(err);
}

/*
** Name: DDSMemAssignRoleRight()	- declare an access right
**
** Description:
**	
**
** Inputs:
**	u_i4	: role id
**	char*	: resource name
**	u_i4	: right
**
** Outputs:
**
** Returns:
**	GSTATUS	: 
**			  GSTAT_OK
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
DDSMemAssignRoleRight (
	u_i4 role, 
	char* resource, 
	i4 *right,
	bool	*created) 
{
	GSTATUS err = GSTAT_OK;
	DDS_RIGHT *tmp_right = NULL;

	G_ASSERT(roles_id == NULL, E_DF0042_DDS_MEM_NOT_INIT);
	G_ASSERT(role < 0 || 
			 role > dds_role_max ||
			 roles_id[role] == NULL, 
			 E_DF0046_DDS_UNKNOWN_ROLE);

	tmp_right = roles_id[role]->rights;
	while (tmp_right != NULL) {
		if (STcompare(tmp_right->resource, resource) == 0)
			break;
		tmp_right =tmp_right->next;
	}

	if (tmp_right == NULL)
	{
		err = G_ME_REQ_MEM(dds_memory_tag, tmp_right, DDS_RIGHT, 1);
		if (err == GSTAT_OK) 
		{
			err = G_ST_ALLOC(tmp_right->resource, resource);
			if (err == GSTAT_OK)
			{
				tmp_right->previous = NULL;
				tmp_right->next = roles_id[role]->rights;
				if (roles_id[role]->rights != NULL)
					roles_id[role]->rights->previous = tmp_right;
				roles_id[role]->rights = tmp_right;

				tmp_right->type = RIGHT_TYPE_ROLE;
				tmp_right->id = role;
				tmp_right->next_global = global_rights;
				if (global_rights != NULL)
					global_rights->pre_global = tmp_right;
				global_rights = tmp_right;
			}
		}
		*created = TRUE;
	}
	else
		*created = FALSE;

	if (err == GSTAT_OK) 
	{
		tmp_right->right |= (*right);
		*right = tmp_right->right;
	}
	else
		*right = 0;

	if (err == GSTAT_OK)
	{
		u_i4 i;
		DDS_ID_LIST *tmp;

		for (i = 0; i <= dds_user_max; i++)
			if (users_id[i] != NULL &&
				users_id[i]->requested > 0)
			{
				tmp = users_id[i]->roles;
				while (err == GSTAT_OK && tmp != NULL) 
				{
					if (tmp->id == role)
						err = DDSMemInitCurrentRight(users_id[i], tmp_right);
					tmp = tmp->next;
				}
			}
	}
return(err);
}

/*
** Name: DDSMemDeleteRoleRight()	- Delete a new access right
**
** Description:
**	
**
** Inputs:
**	u_i4	: role id
**	char*	: resource name
**	u_i4	: right
**
** Outputs:
**
** Returns:
**	GSTATUS	: 
**			  GSTAT_OK
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
DDSMemDeleteRoleRight (
	u_i4 role, 
	char* resource, 
	i4 *right,
	bool	*deleted) 
{
	GSTATUS err = GSTAT_OK;
	DDS_RIGHT *tmp_right = NULL;

	G_ASSERT(roles_id == NULL, E_DF0042_DDS_MEM_NOT_INIT);
	G_ASSERT(role < 0 || 
			 role > dds_role_max ||
			 roles_id[role] == NULL, 
			 E_DF0046_DDS_UNKNOWN_ROLE);

	tmp_right = roles_id[role]->rights;
	while (tmp_right != NULL) {
		if (STcompare(tmp_right->resource, resource) == 0)
			break;
		tmp_right =tmp_right->next;
	}

	if (tmp_right != NULL) 
	{
		tmp_right->right &= ((*right) ^ tmp_right->right);
		*right = tmp_right->right;
		*deleted = (tmp_right->right == 0);

		if (*deleted == TRUE)
			err = DDSMemPurgeRight(tmp_right);
		else
		{
			u_i4 i;
			DDS_ID_LIST *tmp;

			for (i = 0; i <= dds_user_max; i++)
				if (users_id[i] != NULL &&
					users_id[i]->requested > 0)
				{
					tmp = users_id[i]->roles;
					while (err == GSTAT_OK && tmp != NULL) 
					{
						if (tmp->id == role)
							err = DDSMemUnInitCurrentRight(users_id[i], tmp_right);
						tmp = tmp->next;
					}
				}
		}
	}
	else
		*deleted = FALSE;
return(err);
}

/*
** Name: DDSMemCreateAssUserRole()	- create a relation between user and role
**
** Description:
**	
**
** Inputs:
**	u_i4		: user id
**	u_i4		: role id
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
DDSMemCreateAssUserRole (
    u_i4 user_id,
    u_i4 role_id)
{
    GSTATUS err = GSTAT_OK;
    DDS_ID_LIST *id_list;

    G_ASSERT(users_id == NULL || roles_id == NULL, E_DF0042_DDS_MEM_NOT_INIT);
    G_ASSERT(user_id < 0 ||
             user_id > dds_user_max ||
             users_id[user_id] == NULL,
             E_DF0045_DDS_UNKNOWN_USER);

    G_ASSERT(role_id < 0 ||
             role_id > dds_role_max ||
             roles_id[role_id] == NULL,
             E_DF0046_DDS_UNKNOWN_ROLE);

    err = G_ME_REQ_MEM(dds_memory_tag, id_list, DDS_ID_LIST, 1);
    if (err == GSTAT_OK)
    {
        id_list->id = role_id;
        id_list->next = users_id[user_id]->roles;
        if (users_id[user_id]->roles != NULL)
            users_id[user_id]->roles->previous = id_list;
        users_id[user_id]->roles = id_list;

        if (users_id[user_id]->requested > 0)
        {
            DDS_RIGHT *right = roles_id[role_id]->rights;
            while (err == GSTAT_OK && right != NULL)
            {
                err = DDSMemInitCurrentRight(users_id[user_id], right);
                right = right->next;
            }
        }
    }
    return(err);
}

/*
** Name: DDSMemCheckAssUserRole()	- Check if the user/role relation exists
**
** Description:
**	
**
** Inputs:
**	u_i4		: user id
**	u_i4		: role id
**
** Outputs:
**
** Returns:
**	GSTATUS		: 
**				  E_DF0060_DDS_EXIST_USER_ROLE
**				  GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
**      18-Jun-2002 (fanra01)
**          Add range check to role ids.
*/
GSTATUS
DDSMemCheckAssUserRole (
    u_i4 user_id,
    u_i4 role_id)
{
    GSTATUS err = GSTAT_OK;
    DDS_ID_LIST *id_list;

    G_ASSERT(users_id == NULL || roles_id == NULL, E_DF0042_DDS_MEM_NOT_INIT);
    G_ASSERT(user_id < 0 ||
             user_id > dds_user_max ||
             users_id[user_id] == NULL,
             E_DF0045_DDS_UNKNOWN_USER);

    G_ASSERT(role_id < 0 ||
             role_id > dds_role_max ||
             roles_id[role_id] == NULL,
             E_DF0046_DDS_UNKNOWN_ROLE);

    id_list = users_id[user_id]->roles;

    if (id_list != NULL)
    {
        while (err == GSTAT_OK && id_list != NULL)
            if (id_list->id == role_id)
                err = DDFStatusAlloc( E_DF0060_DDS_EXIST_USER_ROLE );
            else
                id_list = id_list->next;
    }
    return(err);
}

/*
** Name: DDSMemSelectAssUserRole()	- select user/role relation
**
** Description:
**	
**
** Inputs:
**	u_i4		: user id
**
** Outputs:
**	u_i4*		: role id
**  char**		: role name
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
DDSMemSelectAssUserRole (
	PTR		*cursor,
	u_i4	user_id, 
	u_i4	**role_id,
	char	**role_name) 
{
	GSTATUS err = GSTAT_OK;
	DDS_ID_LIST *id_list;	
	u_i4 length;

	if (*cursor == NULL)
	{
		G_ASSERT(users_id == NULL || roles_id == NULL, E_DF0042_DDS_MEM_NOT_INIT);
		G_ASSERT(user_id < 0 || 
			 user_id > dds_user_max ||
			 users_id[user_id] == NULL, 
			 E_DF0045_DDS_UNKNOWN_USER);

		id_list = users_id[user_id]->roles;
	}
	else
	{
		id_list = (DDS_ID_LIST *) *cursor;
		id_list = id_list->next;
	}

	if (id_list != NULL)
	{
		err = GAlloc((PTR*)role_id, sizeof(id_list->id), FALSE);
		if (err == GSTAT_OK)
		{
			**role_id = id_list->id;
			length = STlength(roles_id[id_list->id]->name) + 1;
			GAlloc((PTR*)role_name, length, FALSE);
			if (err == GSTAT_OK)
				MECOPY_CONST_MACRO(roles_id[id_list->id]->name, length, *role_name);
		}
		
	}

	*cursor = (char*) id_list;
return(err);
}

/*
** Name: DDSMemDeleteAssUserRole()	- delete a relation between user and role
**
** Description:
**	
**
** Inputs:
**	u_i4		: user id
**	u_i4		: role id
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
**      18-Jun-2002 (fanra01)
**          Add range check to role ids.
*/
GSTATUS
DDSMemDeleteAssUserRole (
    u_i4 user_id,
    u_i4 role_id)
{
    GSTATUS err = GSTAT_OK;
    DDS_ID_LIST *id_list, *tmp;

    G_ASSERT(users_id == NULL || roles_id == NULL, E_DF0042_DDS_MEM_NOT_INIT);
    G_ASSERT(user_id < 0 ||
             user_id > dds_user_max ||
             users_id[user_id] == NULL,
             E_DF0045_DDS_UNKNOWN_USER);
    G_ASSERT(role_id < 0 ||
         role_id > dds_role_max ||
         roles_id[role_id] == NULL,
         E_DF0046_DDS_UNKNOWN_ROLE);

    id_list = users_id[user_id]->roles;
    while (err == GSTAT_OK && id_list != NULL)
    {
        tmp = id_list;
        id_list = id_list->next;
        if (tmp->id == role_id)
        {
            if (tmp->previous != NULL)
                tmp->previous->next = tmp->next;
            else
                users_id[user_id]->roles = tmp->next;

            if (tmp->next != NULL)
                tmp->next->previous = tmp->previous;

            if (users_id[user_id]->requested > 0)
            {
                DDS_RIGHT *right = roles_id[role_id]->rights;
                while (err == GSTAT_OK && right != NULL)
                {
                    err = DDSMemUnInitCurrentRight(users_id[user_id], right);
                    right = right->next;
                }
            }

            MEfree((PTR)tmp);
        }
    }
    return(err);
}

/*
** Name: DDSPerformDBId()	- find the first available id
**
** Description:
**	
**
** Inputs:
**
** Outputs:
**	u_i4*		: available id
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
DDSPerformDBId (
	u_i4 *id) 
{
	GSTATUS err = GSTAT_OK;
	u_i4	i = 1;

	for (i = 1; i <= dds_db_max && dbs_id[i] != NULL; i++) ;

	*id = i;
return(err);
}

/*
** Name: DDSMemCreateDB()	- Create a new user into the memory 
**
** Description:
**	
**
** Inputs:
**	u_i4	: db id
**	char*	: name
**	char*	: dbname
**	u_i4	: database user id
**	i4	: flags
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
DDSMemCreateDB (
	u_i4 id, 
	char *name, 
	char *dbname, 
	u_i4 dbuser_id,
	i4 flags) 
{
	GSTATUS err = GSTAT_OK;
	DDS_DB	*db = NULL;
	u_i4	i;

	G_ASSERT(dbs_id == NULL, E_DF0042_DDS_MEM_NOT_INIT);
	G_ASSERT(id < 0, E_DF0053_DDS_INV_ID);
	G_ASSERT(id <= dds_db_max &&
			 dbs_id[id] != NULL, 
			 E_DF0057_DDS_EXISTING_DB);

	for (i = 0; i <= dds_db_max; i++)
		G_ASSERT(dbs_id[i] != NULL &&
				 STcompare(dbs_id[i]->name, name) == 0, 
				 E_DF0057_DDS_EXISTING_DB);

	if (id > dds_db_max)
	{
		DDS_DB* *tmp = dbs_id;
		err = G_ME_REQ_MEM(
					dds_memory_tag, 
					dbs_id, 
					DDS_DB*, 
					id + DDS_DEFAULT_DB);

		if (err == GSTAT_OK && tmp != NULL) 
			MECOPY_CONST_MACRO(tmp, sizeof(DDS_DB*)*(dds_db_max + 1), dbs_id);
		if (err == GSTAT_OK)
		{
			MEfree((PTR)tmp);
			dds_db_max = id + DDS_DEFAULT_USER;
		}
		else
			dbs_id = tmp;
	}

	if (err == GSTAT_OK) 
	{
		err = G_ME_REQ_MEM(dds_memory_tag, db, DDS_DB, 1);
		if (err == GSTAT_OK) 
		{
			u_i4 length;
			length = STlength(name)+1;
			err = G_ME_REQ_MEM(dds_memory_tag, db->name, char, length);
			if (err == GSTAT_OK) 
			{
				MECOPY_CONST_MACRO(name, length, db->name);
				length = STlength(dbname)+1;
				err = G_ME_REQ_MEM(dds_memory_tag,db->dbname,char,length);
				if (err == GSTAT_OK) 
				{
					MECOPY_CONST_MACRO(dbname, length, db->dbname);
					db->id = id;
					db->flags = flags;
					db->dbuser = dbusers_id[dbuser_id];
				}
			}
		}
	}
	if (err == GSTAT_OK) 
	{
		dbs_id[id] = db;
	}
	else
	{
		if (db != NULL)
		{
			MEfree((PTR)db->name);
			MEfree((PTR)db->dbname);
		}
		MEfree((PTR)db);
	}
return(err);
}

/*
** Name: DDSMemUpdateDB()	- Update a user from the memory 
**
** Description:
**	
**
** Inputs:
**	u_i4	: db id
**	char*	: name
**	char*	: dbname
**	u_i4	: database user id
**	i4	: flags
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
DDSMemUpdateDB (
	u_i4 id, 
	char *name, 
	char *dbname, 
	u_i4 dbuser_id,
	i4 flags) 
{
	GSTATUS err = GSTAT_OK;
	DDS_DB *db;
	char *tmp_name;
	char *tmp_dbname;
	i4 tmp_flags;
	DDS_DBUSER* tmp_dbuser;
	u_i4	i;
	u_i4 length;

	G_ASSERT(dbs_id == NULL, E_DF0042_DDS_MEM_NOT_INIT);
	G_ASSERT(id < 0 || 
			 id > dds_db_max ||
			 dbs_id[id] == NULL, 
			 E_DF0058_DDS_UNKNOWN_DB);

	for (i = 0; i <= dds_db_max; i++)
		if (i != id)
			G_ASSERT(i != id && dbs_id[i] != NULL &&
				 STcompare(dbs_id[i]->name, name) == 0, 
				 E_DF0057_DDS_EXISTING_DB);

	db = dbs_id[id];
	tmp_name = db->name;
	tmp_dbname = db->dbname;
	tmp_flags = db->flags;
	tmp_dbuser = db->dbuser;
	db->name = NULL;
	db->dbname= NULL;

	length = STlength(name)+1;
	err = G_ME_REQ_MEM(dds_memory_tag, db->name, char, length);
	if (err == GSTAT_OK) 
	{
		MECOPY_CONST_MACRO(name, length, db->name);
		length = STlength(dbname)+1;
		err = G_ME_REQ_MEM(dds_memory_tag, db->dbname, char, length);
		if (err == GSTAT_OK)
			MECOPY_CONST_MACRO(dbname, length, db->dbname);
	}
	if (err == GSTAT_OK) 
	{
		db->dbuser = dbusers_id[dbuser_id];
		db->flags = flags;
		MEfree((PTR)tmp_name);
		MEfree((PTR)tmp_dbname);
	} 
	else 
	{
		MEfree((PTR)db->dbname);
		MEfree((PTR)db->name);
		db->dbname = tmp_dbname;
		db->name = tmp_name;
		db->flags = tmp_flags;
		db->dbuser = tmp_dbuser;
	}
return(err);
}

/*
** Name: DDSMemDeleteDB()	- Delete a DB from the memory
**
** Description:
**	
**
** Inputs:
**	u_i4	: db id
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
DDSMemDeleteDB (
	u_i4 id) 
{
	GSTATUS err = GSTAT_OK;
	DDS_ID_LIST	*id_list = NULL;
	DDS_ID_LIST	*tmp = NULL;
	u_i4		i = 0;

	G_ASSERT(dbs_id == NULL, E_DF0042_DDS_MEM_NOT_INIT);
	G_ASSERT(id < 0 || 
			 id > dds_db_max ||
			 dbs_id[id] == NULL,
			 E_DF0058_DDS_UNKNOWN_DB);

	for (i = 0; err == GSTAT_OK && i <= dds_user_max; i++)
		if (users_id[i] != NULL)
		{
			id_list = users_id[i]->dbs;
			while (err == GSTAT_OK && id_list != NULL)
			{
				tmp = id_list;
				id_list = id_list->next;
				if (tmp->id == id)
					err = DDFStatusAlloc( E_DF0059_DDS_DB_USED );
			}
		}

	if (err == GSTAT_OK)
		if (dbs_id[id] != NULL) 
		{
			MEfree((PTR)dbs_id[id]->name);
			MEfree((PTR)dbs_id[id]->dbname);
			MEfree((PTR)dbs_id[id]);
			dbs_id[id] = NULL;
		}
return(err);
}

/*
** Name: DDSMemCreateAssUserDB()	- create a relation between user and DB
**
** Description:
**	
**
** Inputs:
**	u_i4		: user id
**	u_i4		: DB id
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
**      04-Oct-2001 (fanra01)
**          Add range test for valid database user id.
*/

GSTATUS 
DDSMemCreateAssUserDB (
    u_i4 user_id,
    u_i4 db_id)
{
    GSTATUS err = GSTAT_OK;
    DDS_ID_LIST *id_list;

    G_ASSERT(users_id == NULL || roles_id == NULL, E_DF0042_DDS_MEM_NOT_INIT);
    G_ASSERT(user_id < 0 ||
             user_id > dds_user_max ||
             users_id[user_id] == NULL,
             E_DF0045_DDS_UNKNOWN_USER);
    G_ASSERT(db_id < 0 ||
             db_id > dds_db_max ||
             dbs_id[db_id] == NULL,
             E_DF0058_DDS_UNKNOWN_DB);

    err = G_ME_REQ_MEM(dds_memory_tag, id_list, DDS_ID_LIST, 1);
    if (err == GSTAT_OK)
    {
        id_list->id = db_id;
        id_list->next = users_id[user_id]->dbs;
        if (users_id[user_id]->dbs != NULL)
            users_id[user_id]->dbs->previous = id_list;
        users_id[user_id]->dbs = id_list;
    }
    return(err);
}

/*
** Name: DDSMemCheckAssUserDB()	- Check if the user/DB relation exists
**
** Description:
**	
**
** Inputs:
**	u_i4		: user id
**	u_i4		: db id
**
** Outputs:
**
** Returns:
**	GSTATUS		: 
**				  E_DF0060_DDS_EXIST_USER_ROLE
**				  GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
**      04-Oct-2001 (fanra01)
**          Add range test for valid database user id.
*/

GSTATUS 
DDSMemCheckAssUserDB (
    u_i4 user_id,
    u_i4 db_id)
{
    GSTATUS err = GSTAT_OK;
    DDS_ID_LIST *id_list;

    G_ASSERT(users_id == NULL || dbs_id == NULL, E_DF0042_DDS_MEM_NOT_INIT);
    G_ASSERT(user_id < 0 ||
             user_id > dds_user_max ||
             users_id[user_id] == NULL,
             E_DF0045_DDS_UNKNOWN_USER);
    G_ASSERT(db_id < 0 ||
             db_id > dds_db_max ||
             dbs_id[db_id] == NULL,
             E_DF0058_DDS_UNKNOWN_DB);

    id_list = users_id[user_id]->dbs;

    if (id_list != NULL)
    {
        while (err == GSTAT_OK && id_list != NULL)
            if (id_list->id == db_id)
                err = DDFStatusAlloc( E_DF0061_DDS_EXIST_USER_DB );
            else
                id_list = id_list->next;
    }
    return(err);
}

/*
** Name: DDSMemSelectAssUserDB()	- select user/DB relation
**
** Description:
**	
**
** Inputs:
**	u_i4		: user id
**
** Outputs:
**	u_i4*		: role id
**  char**		: role name
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
DDSMemSelectAssUserDB (
	PTR		*cursor,
	u_i4	user_id, 
	u_i4	**db_id,
	char	**db_name) 
{
	GSTATUS err = GSTAT_OK;
	DDS_ID_LIST *id_list;	
	u_i4 length;

	if (*cursor == NULL)
	{
		G_ASSERT(users_id == NULL || dbs_id == NULL, E_DF0042_DDS_MEM_NOT_INIT);
		G_ASSERT(user_id < 0 || 
			 user_id > dds_user_max ||
			 users_id[user_id] == NULL, 
			 E_DF0045_DDS_UNKNOWN_USER);

		id_list = users_id[user_id]->dbs;
	}
	else
	{
		id_list = (DDS_ID_LIST *) *cursor;
		id_list = id_list->next;
	}

	if (id_list != NULL)
	{
		err = GAlloc((PTR*)db_id, sizeof(id_list->id), FALSE);
		if (err == GSTAT_OK)
		{
			**db_id = id_list->id;
			length = STlength(dbs_id[id_list->id]->name) + 1;
			GAlloc((PTR*)db_name, length, FALSE);
			if (err == GSTAT_OK)
				MECOPY_CONST_MACRO(dbs_id[id_list->id]->name, length, *db_name);
		}
	}

	*cursor = (PTR) id_list;
return(err);
}

/*
** Name: DDSMemDeleteAssUserDB()	- delete a relation between user and DB
**
** Description:
**	
**
** Inputs:
**	u_i4		: user id
**	u_i4		: db id
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
**      08-Jul-2002 (fanra01)
**          Add range check on the database id.
*/

GSTATUS 
DDSMemDeleteAssUserDB (
	u_i4 user_id, 
	u_i4 db_id) 
{
	GSTATUS err = GSTAT_OK;
	DDS_ID_LIST *id_list, *tmp;

	G_ASSERT(users_id == NULL || roles_id == NULL, E_DF0042_DDS_MEM_NOT_INIT);
	G_ASSERT(user_id < 0 || 
			 user_id > dds_user_max ||
			 users_id[user_id] == NULL, 
			 E_DF0045_DDS_UNKNOWN_USER);

	G_ASSERT(db_id < 0 ||
			 db_id > dds_db_max ||
			 dbs_id[db_id] == NULL,
			 E_DF0058_DDS_UNKNOWN_DB);

	id_list = users_id[user_id]->dbs;
	while (err == GSTAT_OK && id_list != NULL)
	{
		tmp = id_list;
		id_list = id_list->next;
		if (tmp->id == db_id)
		{
			if (tmp->previous != NULL)
				tmp->previous->next = tmp->next;
			else
				users_id[user_id]->dbs = tmp->next;

			if (tmp->next != NULL)
				tmp->next->previous = tmp->previous;
			MEfree((PTR)tmp);
		}
	}
return(err);
}

/*
** Name: DDSMemSelectResource()	- select the resources assign to one role or role
**
** Description:
**	
**
** Inputs:
**	u_i4		: type (RIGHT_TYPE_ROLE | RIGHT_TYPE_USER)
**	char*		: resource name
**
** Outputs:
**	u_i4*		: role id
**  i4*	: flag
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
DDSMemSelectResource (
	PTR		*cursor,
	u_i4	type,
	char*	resource,
	u_i4	*id,
	i4	*flag) 
{
	GSTATUS err = GSTAT_OK;
	DDS_RIGHT *list;	

	if (*cursor == NULL)
		list = global_rights;
	else
	{
		list = (DDS_RIGHT *) *cursor;
		list = list->next_global;
	}

	while (list != NULL && 
		   (STcompare(list->resource, resource) != 0 ||
		   list->type != type))
			list = list->next_global;

	if (list != NULL)
	{
		*id = list->id;
		*flag = list->right;
	}
	*cursor = (PTR) list;
return(err);
}

/*
** Name: DDSMemDeleteResource()	- Delete all access to a resource
**
** Description:
**	
**
** Inputs:
**	char*		: resource name
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
DDSMemDeleteResource(
	char*	resource) 
{
	GSTATUS err = GSTAT_OK;
	DDS_RIGHT *list, *tmp;

	list = global_rights;
	while (err == GSTAT_OK && list != NULL)
	{
		if (STcompare(list->resource, resource) == 0)
		{
			tmp = list;
			list = list->next_global;
			err = DDSMemPurgeRight(tmp);
		}
		else
			list = list->next_global;
	}
return(err);
}

/*
** Name: DDSMemGetDBUserName()	- 
**
** Description:
**	
**
** Inputs:
**	u_i4		: id 
**
** Outputs:
**	char**		: name
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
DDSMemGetDBUserName(
	u_i4 id, 
	char** name)
{
	GSTATUS err = GSTAT_OK;
	u_i4 length;

	G_ASSERT(dbusers_id == NULL, E_DF0042_DDS_MEM_NOT_INIT);
	G_ASSERT(id < 0 || 
			 id > dds_dbuser_max ||
			 dbusers_id[id] == NULL, 
			 E_DF0051_DDS_UNKNOWN_DBUSER);

	if (name != NULL)
	{
		length = STlength(dbusers_id[id]->name) + 1;
		err = GAlloc((PTR*)name, length, FALSE);
		if (err == GSTAT_OK) 
		{
			MECOPY_VAR_MACRO(
				dbusers_id[id]->name, 
				length, 
				*name);
		}
	}
return(err);
}

/*
** Name: DDSMemGetDBName()    -
**
** Description:
**
**
** Inputs:
**    u_i4        : id
**
** Outputs:
**    char**        : name
**
** Returns:
**    GSTATUS        :
**                  GSTAT_OK
**
** Exceptions:
**    None
**
** Side Effects:
**    None
**
** History:
**      23-Jul-2002 (fanra01)
**          Adjust range check to exclude 0 from valid range.
*/
GSTATUS 
DDSMemGetDBName( u_i4 id, char** name )
{
    GSTATUS err = GSTAT_OK;
    u_i4 length;

    G_ASSERT(dbs_id == NULL, E_DF0042_DDS_MEM_NOT_INIT);
    G_ASSERT(id < 1 ||
             id > dds_db_max ||
             dbs_id[id] == NULL,
             E_DF0058_DDS_UNKNOWN_DB);

    if (name != NULL)
    {
        length = STlength(dbs_id[id]->name) + 1;
        err = GAlloc((PTR*)name, length, FALSE);
        if (err == GSTAT_OK)
        {
            MECOPY_VAR_MACRO( dbs_id[id]->name, length, *name );
        }
    }
    return(err);
}

/*
** Name: DDSMemGetUserName()	- 
**
** Description:
**	
**
** Inputs:
**	u_i4		: id 
**
** Outputs:
**	char**		: name
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
DDSMemGetUserName(
	u_i4 id, 
	char** name)
{
	GSTATUS err = GSTAT_OK;
	u_i4 length;

	G_ASSERT(users_id == NULL, E_DF0042_DDS_MEM_NOT_INIT);
	G_ASSERT(id < 0 || 
			 id > dds_user_max ||
			 users_id[id] == NULL, 
			 E_DF0045_DDS_UNKNOWN_USER);

	if (name != NULL)
	{
		length = STlength(users_id[id]->name) + 1;
		err = GAlloc((PTR*)name, length, FALSE);
		if (err == GSTAT_OK) 
		{
			MECOPY_VAR_MACRO(
				users_id[id]->name, 
				length, 
				*name);
		}
	}
return(err);
}

/*
** Name: DDSMemGetRoleName()	- 
**
** Description:
**	
**
** Inputs:
**	u_i4		: id 
**
** Outputs:
**	char**		: name
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
DDSMemGetRoleName(
	u_i4 id, 
	char** name)
{
	GSTATUS err = GSTAT_OK;
	u_i4 length;

	G_ASSERT(roles_id == NULL, E_DF0042_DDS_MEM_NOT_INIT);
	G_ASSERT(id < 0 || 
			 id > dds_role_max ||
			 roles_id[id] == NULL, 
			 E_DF0046_DDS_UNKNOWN_ROLE);

	if (name != NULL)
	{
		length = STlength(roles_id[id]->name) + 1;
		err = GAlloc((PTR*)name, length, FALSE);
		if (err == GSTAT_OK) 
		{
			MECOPY_VAR_MACRO(
				roles_id[id]->name, 
				length, 
				*name);
		}
	}
return(err);
}

/*
** Name: DDSMemGetUserTimeout()	- 
**
** Description:
**	
**
** Inputs:
**	u_i4		: id 
**
** Outputs:
**	i4*	: timeout
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
DDSMemGetUserTimeout(
	u_i4 id, 
	i4* timeout)
{
	GSTATUS err = GSTAT_OK;

	G_ASSERT(users_id == NULL, E_DF0042_DDS_MEM_NOT_INIT);
	G_ASSERT(id < 0 || 
			 id > dds_user_max ||
			 users_id[id] == NULL, 
			 E_DF0045_DDS_UNKNOWN_USER);

	*timeout = users_id[id]->timeout;
return(err);
}

/*
** Name: DDSMemGetDBUserPassword()	- 
**
** Description:
**	
**
** Inputs:
**	char*		: name
**
** Outputs:
**	char**	: password
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
DDSMemGetDBUserPassword(
	char*	alias, 
	char*	*dbuser,
	char* *password)
{
	GSTATUS err = GSTAT_OK;
	u_i4 i;
	G_ASSERT(dbusers_id == NULL, E_DF0042_DDS_MEM_NOT_INIT);

	*password = NULL;

	for (i = 1; i <= dds_dbuser_max; i++)
		if (dbusers_id[i] != NULL &&
				STcompare(dbusers_id[i]->alias, alias) == 0)
		{
			*dbuser = dbusers_id[i]->name;
			*password = dbusers_id[i]->password;
			return(GSTAT_OK);
		}
	
G_ASSERT(TRUE, E_DF0051_DDS_UNKNOWN_DBUSER);
}

/*
** Name: DDSMemGetDBUserInfo()
**
** Description:
**      Returns the information for the specified dbuser.
**
** Inputs:
**      char*   dbuser
**
** Outputs:
**      char**  alias
**      char**  password
**
** Returns:
**      GSTAT_OK    complete successfully
**
** Exceptions:
**    None
**
** Side Effects:
**    None
**
** History:
**      14-Dec-1998 (fanra01)
**          Created.
*/

GSTATUS 
DDSMemGetDBUserInfo(
    char*   dbuser,
    char*   *alias,
    char*   *password)
{
    GSTATUS err = GSTAT_OK;
    u_i4 i;
    G_ASSERT(dbusers_id == NULL, E_DF0042_DDS_MEM_NOT_INIT);

    *password = NULL;

    for (i = 1; i <= dds_dbuser_max; i++)
    {
        if (dbusers_id[i] != NULL &&
                STcompare(dbusers_id[i]->name, dbuser) == 0)
        {
            *alias = dbusers_id[i]->alias;
            *password = dbusers_id[i]->password;
            return(GSTAT_OK);
        }
    }
    G_ASSERT(TRUE, E_DF0051_DDS_UNKNOWN_DBUSER);
}

/*
** Name: DDSMemGetAuthType(u_i4 id, i4* authtype)
**
** Description:
**      Returns the information for the specified dbuser.
**
** Inputs:
**      char*   dbuser
**
** Outputs:
**      char**  alias
**      char**  password
**
** Returns:
**      GSTAT_OK    complete successfully
**
** Exceptions:
**    None
**
** Side Effects:
**    None
**
** History:
**      14-Dec-1998 (fanra01)
**          Created.
*/
GSTATUS
DDSMemGetAuthType(u_i4 id, i4* authtype)
{
    GSTATUS err = GSTAT_OK;

    G_ASSERT(users_id == NULL, E_DF0042_DDS_MEM_NOT_INIT);
    G_ASSERT(id < 0 ||
             id > dds_user_max ||
             users_id[id] == NULL,
             E_DF0045_DDS_UNKNOWN_USER);

    *authtype = users_id[id]->authtype;
    return (err);
}
