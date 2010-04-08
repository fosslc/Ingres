/******************************************************************************
**
**Copyright (c) 2004 Ingres Corporation
**
******************************************************************************/

#include <ddfsem.h>
#include <ddfhash.h>
#include <erwsf.h>
#include <wssprofile.h>
#include <dds.h>
#include <ddgexec.h>
#include <wsf.h>

/**
**
**  Name: wssprofile.c - Web auto declaration profile
**
**  Description:
**
**  History:    
**	5-Feb-98 (marol01)
**	    created
**      25-Jan-1999 (fanra01)
**          Correct allocate memory for timeout writing over flags.
**      07-Mar-2000 (fanra01)
**          Bug 100767
**          Replaced the G_ASSERT macro where a semaphore has been taken
**          to release the semaphore before returning error.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      04-Oct-2001 (fanra01)
**          Bug 105962
**          Add range checking of the id before attempting to use the value.
**      18-Jun-2002 (fanra01)
**          Bug 108065
**          Add range check to WSSMemCheckProfilesRole to prevent
**          derferencing past the end of the profile list.
**          Bug 108077
**          Add a call to DDSGetDBName to check the validity of the database
**          id prior to attempting to delete it from a profile.
**	11-oct-2002 (somsa01)
**	    Changed DDGInsert(), DDGUpdate(), DDGExecute() and DDGDelete()
**	    such that we pass the address of int and float arguments.
**      04-Dec-2002 (fanra01)
**          Bug 109242
**          Add a vailidity check on role_id in WSSMemDeleteProfileRole.
**	13-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
**/

GLOBALREF DDG_SESSION	wsf_session;	/* repository connection */
static WSS_PPROFILE		*profiles = NULL;
static u_i4					wss_profile_max = 0;

GLOBALDEF SEMAPHORE wss_profile;

/*
** Name: WSSPerformProfileId()	- find the first available id
**
** Description:
**	
**
** Inputs:
**
** Outputs:
**	u_nat*		: available id
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
WSSPerformProfileId (
	u_i4 *id) 
{
	GSTATUS err = GSTAT_OK;
	u_i4	i = 1;

	for (i = 1; i <= wss_profile_max && profiles[i] != NULL; i++) ;

	*id = i;
return(err);
}

/*
** Name: WSSMemGetProfile()	- 
**
** Description:
**	
**
** Inputs:
**	char*	: name
**
** Outputs:
**	WSS_PPROFILE *profile
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
WSSMemGetProfile(
	char* name, 
	WSS_PPROFILE *profile)
{
	u_i4 i;

	*profile = NULL;
	for (i = 1; i <= wss_profile_max; i++) 
		if (profiles[i] != NULL && 
			STcompare(profiles[i]->name, name) == 0)
		{
			*profile = profiles[i];
			break;
		}
return(GSTAT_OK);
}

/*
** Name: WSSMemCheckProfilesRole()	- 
**
** Description:
**	
**
** Inputs:
**	u_i4		: profile id
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
**          Add range check to profile id before trying to dereference it.
*/
GSTATUS
WSSMemCheckProfilesRole(
    u_i4    profile_id,
    u_i4    role_id)
{
    GSTATUS err = GSTAT_OK;
    WSS_ID_LIST *id_list;

    /*
    ** Add range checking of profile_id to ensure it is valid before trying
    ** to dereference it.
    */
    G_ASSERT(profile_id < 1 ||
             profile_id > wss_profile_max,
             E_WS0076_WSS_BAD_PROFILE);

    if (profiles[profile_id] != NULL)
    {
        id_list = profiles[profile_id]->roles;
        while (err == GSTAT_OK && id_list != NULL)
            if (id_list->id == role_id)
                err = DDFStatusAlloc( E_WS0080_WSS_EXIST_PROF_ROLE );
            else
                id_list = id_list->next;
    }
    return(err);
}

/*
** Name: WSSMemCheckProfilesDB()	- 
**
** Description:
**	
**
** Inputs:
**	u_i4		: profile id
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
**          Add range checking to the profile id before attempting to use it.
*/

GSTATUS 
WSSMemCheckProfilesDB(
    u_i4    profile_id,
    u_i4    db_id)
{
    GSTATUS err = GSTAT_OK;
    WSS_ID_LIST *id_list;
    /*
    ** Add range checking of profile_id to ensure it is valid before trying
    ** to dereference it.
    */
    G_ASSERT(profile_id < 1 ||
             profile_id > wss_profile_max ||
             profiles[profile_id] == NULL,
             E_WS0076_WSS_BAD_PROFILE);

    if (profiles[profile_id] != NULL)
    {
        id_list = profiles[profile_id]->dbs;
        while (err == GSTAT_OK && id_list != NULL)
            if (id_list->id == db_id)
                err = DDFStatusAlloc( E_WS0081_WSS_EXIST_PROF_DB );
            else
                id_list = id_list->next;
    }
    return(err);
}

/*
** Name: WSSMemCreateProfile()	- 
**
** Description:
**	
**
** Inputs:
**	u_i4	: id
**	char*	: name
**	u_i4	: database user id
**	i4	: flags
**	i4	: timeout
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
WSSMemCreateProfile (
	u_i4 id,
	char *name, 
	u_i4 dbuser_id, 
	i4 flags,
	i4 timeout) 
{
	GSTATUS err = GSTAT_OK;
	WSS_PPROFILE tmp = NULL;
	u_i4 length;

	err = WSSMemGetProfile(name, &tmp);
	G_ASSERT(tmp != NULL, E_WS0075_WSS_EXISTING_PROFILE);

	if (id > wss_profile_max) 
	{
		WSS_PPROFILE *keep = profiles;
		err = G_ME_REQ_MEM(
						0, 
						profiles, 
						WSS_PPROFILE, 
						id+1);

		if (err == GSTAT_OK && keep != NULL) 
		{
			MECOPY_CONST_MACRO(keep, sizeof(WSS_PPROFILE*)*(wss_profile_max+1), profiles);
			MEfree((PTR)keep);
		}

		if (err == GSTAT_OK)
			wss_profile_max = id;
		else
			profiles = keep;
	}

	if (err == GSTAT_OK) 
	{
		err = G_ME_REQ_MEM(0, tmp, WSS_PROFILE, 1);
		if (err == GSTAT_OK) 
		{
			length = STlength(name)+1;
			err = G_ME_REQ_MEM(0, tmp->name, char, length);
			if (err == GSTAT_OK) 
			{
				MECOPY_CONST_MACRO(name, length, tmp->name);
				tmp->dbuser_id = dbuser_id;
				tmp->flags = flags;
				tmp->timeout = timeout;
				tmp->id = id;
				profiles[id] = tmp;
			}
		}
	}
return(err);
}

/*
** Name: WSSMemUpdateProfile()	- 
**
** Description:
**	
**
** Inputs:
**	u_i4	: id
**	char*	: name
**	u_i4	: database user id
**	i4	: flags
**	i4	: timeout
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
WSSMemUpdateProfile (
	u_i4 profile_id,
	char *name, 
	u_i4 dbuser_id, 
	i4 flags,
	i4 timeout) 
{
	GSTATUS err = GSTAT_OK;
	WSS_PPROFILE tmp = NULL;
	u_i4 length;

	G_ASSERT(profile_id < 0 || 
			 profile_id > wss_profile_max ||
			 profiles[profile_id] == NULL, 
			 E_WS0076_WSS_BAD_PROFILE);

	tmp = profiles[profile_id];

	if (err == GSTAT_OK) 
	{
		length = STlength(name)+1;
		err = G_ME_REQ_MEM(0, tmp->name, char, length);
		if (err == GSTAT_OK) 
		{
			MECOPY_CONST_MACRO(name, length, tmp->name);
			tmp->dbuser_id = dbuser_id;
			tmp->flags = flags;
			tmp->timeout = timeout;
		}
	}
return(err);
}

/*
** Name: WSSMemDeleteProfile()	- 
**
** Description:
**	
**
** Inputs:
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
**      25-Apr-2000 (chika01)
**          Display error message when deleting a profile that doesn't exist.
*/

GSTATUS 
WSSMemDeleteProfile (
	u_i4	id)
{
	GSTATUS err = GSTAT_OK;

	if (id > 0 && id <= wss_profile_max)
	{
        G_ASSERT(profiles[id] == NULL, 
		    E_WS0076_WSS_BAD_PROFILE);
        
        MEfree((PTR)profiles[id]->name);
		MEfree((PTR)profiles[id]);
		profiles[id] = NULL;
	}
return(err);
}

/*
** Name: WSSMemCreateProfileRole()	- 
**
** Description:
**	
**
** Inputs:
**	u_i4		profile id
**	u_i4		role id
**
** Outputs:
**
** Returns:
**	GSTATUS		: GSTAT_OK
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
WSSMemCreateProfileRole (
	u_i4 profile_id, 
	u_i4 role_id) 
{
	GSTATUS err = GSTAT_OK;
	WSS_ID_LIST *id_list;

	G_ASSERT(profile_id < 0 || 
			 profile_id > wss_profile_max ||
			 profiles[profile_id] == NULL, 
			 E_WS0076_WSS_BAD_PROFILE);

	err = DDSGetRoleName(role_id, NULL);
	if (err == GSTAT_OK)
	{
		err = G_ME_REQ_MEM(0, id_list, WSS_ID_LIST, 1);
		if (err == GSTAT_OK) 
		{
			id_list->id = role_id;
			id_list->next = profiles[profile_id]->roles;
			if (profiles[profile_id]->roles != NULL)
				profiles[profile_id]->roles->previous = id_list;
			profiles[profile_id]->roles = id_list;
		}
	}
return(err);
}

/*
** Name: WSSMemDeleteProfileRole()	- 
**
** Description:
**	
**
** Inputs:
**	u_i4		: profile id
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
**      04-Dec-2002 (fanra01)
**          Add a check on the vailidity of role_id prior to a delete.
*/

GSTATUS 
WSSMemDeleteProfileRole (
	u_i4	profile_id, 
	u_i4	role_id) 
{
	GSTATUS err = GSTAT_OK;
	WSS_ID_LIST *id_list, *tmp;

	G_ASSERT(profile_id < 0 || 
			 profile_id > wss_profile_max ||
			 profiles[profile_id] == NULL, 
			 E_WS0076_WSS_BAD_PROFILE);
    
    if ((err = DDSGetRoleName( role_id, NULL )) == GSTAT_OK)
    {
        id_list = profiles[profile_id]->roles;	

        while (err == GSTAT_OK && id_list != NULL)
        {
            tmp = id_list;
            id_list = id_list->next;
            if (tmp->id == role_id)
            {
                if (tmp->previous != NULL)
                    tmp->previous->next = tmp->next;
                else
                    profiles[profile_id]->roles = tmp->next;

                if (tmp->next != NULL)
                    tmp->next->previous = tmp->previous;
                MEfree((PTR)tmp);
            }
        }
    }
    return(err);
}

/*
** Name: WSSMemCreateProfileDB()	- 
**
** Description:
**	
**
** Inputs:
**	u_i4		profile id
**	u_i4		db id
**
** Outputs:
**
** Returns:
**	GSTATUS		: GSTAT_OK
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
WSSMemCreateProfileDB (
	u_i4 profile_id,
	u_i4 db_id) 
{
	GSTATUS err = GSTAT_OK;
	WSS_ID_LIST *id_list;

	G_ASSERT(profile_id < 0 || 
			 profile_id > wss_profile_max ||
			 profiles[profile_id] == NULL, 
			 E_WS0076_WSS_BAD_PROFILE);

	err = DDSGetDBName(db_id, NULL);
	if (err == GSTAT_OK)
	{
		err = G_ME_REQ_MEM(0, id_list, WSS_ID_LIST, 1);
		if (err == GSTAT_OK) 
		{
			id_list->id = db_id;
			id_list->next = profiles[profile_id]->dbs;
			if (profiles[profile_id]->dbs != NULL)
				profiles[profile_id]->dbs->previous = id_list;
			profiles[profile_id]->dbs = id_list;
		}
	}
return(err);
}

/*
** Name: WSSMemDeleteProfileDB()	- 
**
** Description:
**	
**
** Inputs:
**	u_i4		: profile id
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
**      19-Jun-2002 (fanra01)
**          Add call to DDSGetDBName to check the validity of the database
**          id db_id before attempting to remove it.
*/
GSTATUS
WSSMemDeleteProfileDB (
    u_i4 profile_id,
    u_i4    db_id)
{
    GSTATUS err = GSTAT_OK;
    WSS_ID_LIST *id_list, *tmp;
    WSS_PPROFILE profile = NULL;

    G_ASSERT(profile_id < 0 ||
             profile_id > wss_profile_max ||
             profiles[profile_id] == NULL,
             E_WS0076_WSS_BAD_PROFILE);

    if ((err = DDSGetDBName(db_id, NULL)) == GSTAT_OK)
    {
        id_list = profiles[profile_id]->dbs;

        while (err == GSTAT_OK && id_list != NULL)
        {
            tmp = id_list;
            id_list = id_list->next;
            if (tmp->id == db_id)
            {
                if (tmp->previous != NULL)
                    tmp->previous->next = tmp->next;
                else
                    profiles[profile_id]->dbs = tmp->next;

                if (tmp->next != NULL)
                    tmp->next->previous = tmp->previous;
                MEfree((PTR)tmp);
            }
        }
    }
    return(err);
}

/*
** Name: WSSGetProfile()	- 
**
** Description:
**	
**
** Inputs:
**	char*	: name
**
** Outputs:
**	u_nat*	: profile id
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
WSSGetProfile(
	char* name, 
	u_i4* id)
{
	GSTATUS err = GSTAT_OK;
	WSS_PPROFILE profile = NULL;
	err = DDFSemOpen(&wss_profile, FALSE);
	if (err == GSTAT_OK)
	{
		err = WSSMemGetProfile(name, &profile);
		*id = profile->id;
		CLEAR_STATUS(DDFSemClose(&wss_profile));
	}
return(GSTAT_OK);
}

/*
** Name: WSSCreateProfile()	- 
**
** Description:
**	
**
** Inputs:
**	char*	: name
**	u_i4	: database user id
**	i4	: flags
**	i4	: timeout
**
** Outputs:
**	u_nat*	: profile id
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
WSSCreateProfile (
	char *name, 
	u_i4 dbuser_id, 
	i4 flags,
	i4 timeout,
	u_i4*	id) 
{
	GSTATUS err = GSTAT_OK;
	WSS_PPROFILE tmp = NULL;

	err = DDFSemOpen(&wss_profile, TRUE);
	if (err == GSTAT_OK)
	{
		err = WSSPerformProfileId(id);
		if (err == GSTAT_OK)
		{
			err = WSSMemCreateProfile(*id, name, dbuser_id, flags, timeout);
			if (err == GSTAT_OK)
			{
				err = DDGInsert(
						&wsf_session, 
						WSS_OBJ_PROFILE, 
						"%d%s%d%d%d", 
						id,
						name, 
						&dbuser_id, 
						&flags, 
						&timeout);

				if (err == GSTAT_OK) 
					err = DDGCommit(&wsf_session);
				else
				{
					CLEAR_STATUS(DDGRollback(&wsf_session));
					CLEAR_STATUS(WSSMemDeleteProfile(*id));
				}
			}
		}
		CLEAR_STATUS(DDFSemClose(&wss_profile));
	}
return(err);
}

/*
** Name: WSSUpdateProfile()	- 
**
** Description:
**	
**
** Inputs:
**	u_i4	: profile id
**	char*	: name
**	u_i4	: database user id
**	i4	: flags
**	i4	: timeout
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
WSSUpdateProfile (
	u_i4	id,
	char*	name, 
	u_i4	dbuser_id, 
	i4 flags,
	i4 timeout) 
{
	GSTATUS err = GSTAT_OK;
	WSS_PPROFILE tmp = NULL;

	err = DDFSemOpen(&wss_profile, TRUE);
	if (err == GSTAT_OK)
	{
		err = DDGUpdate(
				&wsf_session, 
				WSS_OBJ_PROFILE, 
				"%s%d%d%d%d", 
				name, 
				&dbuser_id, 
				&flags, 
				&timeout,
				&id);

		if (err == GSTAT_OK)
			err = WSSMemUpdateProfile(id, name, dbuser_id, flags, timeout);
		
		if (err == GSTAT_OK) 
			err = DDGCommit(&wsf_session);
		else
			CLEAR_STATUS(DDGRollback(&wsf_session));
		CLEAR_STATUS(DDFSemClose(&wss_profile));
	}
return(err);
}

/*
** Name: WSSSelectProfile()	- 
**
** Description:
**	
**
** Inputs:
**	PTR*		: cursor
**
** Outputs:
**	u_nat**		: profile id
**	char**		: profile name
**	u_nat**		: dbuser id
**  i4**	: flags
**  i4**	: timeout
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
WSSSelectProfile (
	PTR		*cursor,
	u_i4	**id,
	char	**name, 
	u_i4	**dbuser_id,
	i4	**flags,
	i4	**timeout) 
{
	GSTATUS err = GSTAT_OK;
	WSS_PPROFILE tmp = NULL;
	u_i4 position;

	if (*cursor == NULL)
	{
		err = DDFSemOpen(&wss_profile, TRUE);
		if (err == GSTAT_OK)
			position = 1;	
	}
	else
	{
		position = (u_i4)(SCALARP) *cursor;
		position++;
	}

	while (position <= wss_profile_max &&
		   profiles[position] == NULL)
		   position++;

	if (position <= wss_profile_max)
		tmp = profiles[position];
	else
		position = 0;

	if (tmp != NULL)
	{
		u_i4 length = STlength(tmp->name) + 1;
		err = GAlloc((PTR*)name, length, FALSE);
		if (err == GSTAT_OK)
		{
			MECOPY_CONST_MACRO(tmp->name, length, *name);
			err = GAlloc((PTR*)dbuser_id, sizeof(tmp->dbuser_id), FALSE);
			if (err == GSTAT_OK)
			{
				**dbuser_id = tmp->dbuser_id;
				err = GAlloc((PTR*)flags, sizeof(tmp->flags), FALSE);
				if (err == GSTAT_OK)
				{
					**flags = tmp->flags;
					err = GAlloc((PTR*)timeout, sizeof(tmp->timeout), FALSE);
					if (err == GSTAT_OK)
					{
						**timeout = tmp->timeout;
						err = GAlloc((PTR*)id, sizeof(tmp->id), FALSE);
						if (err == GSTAT_OK)
							**id = tmp->id;
					}
				}
			}
		}
	}
	else
		CLEAR_STATUS(DDFSemClose(&wss_profile));
	*cursor = (PTR)(SCALARP) position;
return(err);
}

/*
** Name: WSSRetrieveProfile()	- 
**
** Description:
**	
**
** Inputs:
**	u_i4		: profile id
**
** Outputs:
**	char**		: profile name
**	u_nat**		: dbuser id
**  i4**	: flags
**  i4**	: timeout
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
**      25-Jan-1999 (fanra01)
**          Correct allocate memory for timeout writing over flags.
**      07-Mar-2000 (fanra01)
**          Replaced G_ASSERT macro with if statement and include a release
**          of the semaphore before returning error.
*/
GSTATUS
WSSRetrieveProfile (
    u_i4    profile_id,
    char    **name,
    u_i4    **dbuser_id,
    i4      **flags,
    i4      **timeout)
{
    GSTATUS err = GSTAT_OK;
    WSS_PPROFILE tmp = NULL;
    u_i4 length;

    err = DDFSemOpen(&wss_profile, TRUE);
    if (err == GSTAT_OK)
    {
        if ((profile_id < 0) ||
            (profile_id > wss_profile_max) ||
            (profiles[profile_id] == NULL))
        {
            CLEAR_STATUS(DDFSemClose( &wss_profile ));
            return(DDFStatusAlloc( E_WS0076_WSS_BAD_PROFILE ));
        }

        tmp = profiles[profile_id];
        length = STlength(tmp->name) + 1;
        err = GAlloc((PTR*)name, length, FALSE);
        if (err == GSTAT_OK)
        {
            MECOPY_CONST_MACRO(tmp->name, length, *name);
            err = GAlloc((PTR*)dbuser_id, sizeof(tmp->dbuser_id), FALSE);
            if (err == GSTAT_OK)
            {
                **dbuser_id = tmp->dbuser_id;
                err = GAlloc((PTR*)flags, sizeof(tmp->flags), FALSE);
                if (err == GSTAT_OK)
                {
                    **flags = tmp->flags;
                    err = GAlloc((PTR*)timeout, sizeof(tmp->timeout), FALSE);
                    if (err == GSTAT_OK)
                        **timeout = tmp->timeout;
                }
            }
        }
        CLEAR_STATUS(DDFSemClose(&wss_profile));
    }
    return(err);
}

/*
** Name: WSSDeleteProfile()	- 
**
** Description:
**	
**
** Inputs:
**	u_i4	: profile id
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
WSSDeleteProfile (
	u_i4 profile_id)
{
	GSTATUS err = GSTAT_OK;

	err = DDFSemOpen(&wss_profile, TRUE);
	if (err == GSTAT_OK)
	{
		err = DDGExecute(&wsf_session, WSS_DEL_PROFILE_ROLE, "%d",
				 &profile_id);
		CLEAR_STATUS( DDGClose(&wsf_session) );

		if (err == GSTAT_OK)
		{
			err = DDGExecute(&wsf_session, WSS_DEL_PROFILE_DB,
					 "%d", &profile_id);
			CLEAR_STATUS( DDGClose(&wsf_session) );
		}

		if (err == GSTAT_OK)
			err = DDGDelete(
					&wsf_session, 
					WSS_OBJ_PROFILE, 
					"%d", 
					&profile_id);

		if (err == GSTAT_OK)
				err = WSSMemDeleteProfile(profile_id);

		if (err == GSTAT_OK) 
			err = DDGCommit(&wsf_session);
		else 
			CLEAR_STATUS(DDGRollback(&wsf_session));
		CLEAR_STATUS(DDFSemClose(&wss_profile));
	}
return(err);
}

/*
** Name: WSSCreateProfileRole()	- 
**
** Description:
**	
**
** Inputs:
**	u_i4		profile id
**	u_i4		role id
**
** Outputs:
**
** Returns:
**	GSTATUS		: GSTAT_OK
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
WSSCreateProfileRole (
	u_i4 profile_id, 
	u_i4 role_id) 
{
	GSTATUS err = GSTAT_OK;
	WSS_PPROFILE tmp = NULL;

	err = DDFSemOpen(&wss_profile, FALSE);
	if (err == GSTAT_OK)
	{
		err = WSSMemCheckProfilesRole(profile_id, role_id);
		if (err == GSTAT_OK)
		{
			err = DDGInsert(
					&wsf_session, 
					WSS_OBJ_PROFILE_ROLE, 
					"%d%d", 
					&profile_id,
					&role_id);

			if (err == GSTAT_OK)
				err = WSSMemCreateProfileRole(profile_id, role_id);

			if (err == GSTAT_OK) 
				err = DDGCommit(&wsf_session);
			else 
				CLEAR_STATUS(DDGRollback(&wsf_session));
		}
		CLEAR_STATUS(DDFSemClose(&wss_profile));
	}
return(err);
}

/*
** Name: WSSProfilesUseRole()	- 
**
** Description:
**	
**
** Inputs:
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
WSSProfilesUseRole(
	u_i4	role_id) 
{
	GSTATUS err = GSTAT_OK;
	u_i4	profile_id;
	WSS_ID_LIST *id_list;

	err = DDFSemOpen(&wss_profile, FALSE);
	if (err == GSTAT_OK)
	{
		for (profile_id = 1; err == GSTAT_OK && profile_id <= wss_profile_max; profile_id++)
			if (profiles[profile_id] != NULL)
			{
				id_list = profiles[profile_id]->roles;	
				while (err == GSTAT_OK && id_list != NULL)
					if (id_list->id == role_id)
						err = DDFStatusAlloc( E_DF0056_DDS_ROLE_USED );
					else
						id_list = id_list->next;
			}
		CLEAR_STATUS(DDFSemClose(&wss_profile));
	}
return(err);
}

/*
** Name: WSSSelectProfileRole()	- 
**
** Description:
**	
**
** Inputs:
**	u_i4		: profile id
**
** Outputs:
**	u_nat*		: role id
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
**      07-Mar-2000 (fanra01)
**          Replaced G_ASSERT macro with if statement and include a release
**          of the semaphore before returning error.
*/
GSTATUS
WSSSelectProfileRole(
    PTR     *cursor,
    u_i4    profile_id,
    u_i4    **role_id,
    char    **role_name )
{
    GSTATUS err = GSTAT_OK;
    WSS_ID_LIST *id_list;
    WSS_PPROFILE tmp = NULL;

    if (*cursor == NULL)
    {
        err = DDFSemOpen(&wss_profile, TRUE);
        if (err == GSTAT_OK)
        {
            if ((profile_id < 0) ||
                (profile_id > wss_profile_max) ||
                (profiles[profile_id] == NULL))
            {
                CLEAR_STATUS(DDFSemClose(&wss_profile));
                return(DDFStatusAlloc(E_WS0076_WSS_BAD_PROFILE));
            }
            id_list = profiles[profile_id]->roles;
        }
    }
    else
    {
        id_list = (WSS_ID_LIST *) *cursor;
        id_list = id_list->next;
    }

    if (id_list != NULL)
    {
        err = GAlloc((PTR*)role_id, sizeof(id_list->id), FALSE);
        if (err == GSTAT_OK)
        {
            **role_id = id_list->id;
            err = DDSGetRoleName(id_list->id, role_name);
        }

    }
    else
        CLEAR_STATUS(DDFSemClose(&wss_profile));
    *cursor = (char*) id_list;
    return(err);
}

/*
** Name: WSSDeleteProfileRole()	- 
**
** Description:
**	
**
** Inputs:
**	u_i4		: profile id
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
WSSDeleteProfileRole (
	u_i4	profile_id, 
	u_i4	role_id) 
{
	GSTATUS err = GSTAT_OK;
	WSS_PPROFILE profile = NULL;

	err = DDFSemOpen(&wss_profile, TRUE);
	if (err == GSTAT_OK)
	{
		err = DDGDelete(
				&wsf_session, 
				WSS_OBJ_PROFILE_ROLE, 
				"%d%d", 
				&profile_id,
				&role_id);

		if (err == GSTAT_OK)
			err = WSSMemDeleteProfileRole(profile_id, role_id);

		if (err == GSTAT_OK) 
			err = DDGCommit(&wsf_session);
		else 
			CLEAR_STATUS(DDGRollback(&wsf_session));

		CLEAR_STATUS(DDFSemClose(&wss_profile));
	}
return(err);
}

/*
** Name: WSSCreateProfileDB()	- 
**
** Description:
**	
**
** Inputs:
**	u_i4		: profile id
**	u_i4		: db id
**
** Outputs:
**
** Returns:
**	GSTATUS		: GSTAT_OK
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
WSSCreateProfileDB (
	u_i4	profile_id, 
	u_i4 db_id) 
{
	GSTATUS err = GSTAT_OK;
	WSS_PPROFILE tmp = NULL;

	err = DDFSemOpen(&wss_profile, TRUE);
	if (err == GSTAT_OK)
	{
		err = WSSMemCheckProfilesDB(profile_id, db_id);
		if (err == GSTAT_OK)
		{
			err = DDGInsert(
					&wsf_session, 
					WSS_OBJ_PROFILE_DB, 
					"%d%d", 
					&profile_id,
					&db_id);

			if (err == GSTAT_OK)
				err = WSSMemCreateProfileDB(profile_id, db_id);

			if (err == GSTAT_OK) 
				err = DDGCommit(&wsf_session);
			else 
				CLEAR_STATUS(DDGRollback(&wsf_session));
		}
		CLEAR_STATUS(DDFSemClose(&wss_profile));
	}
return(err);
}

/*
** Name: WSSProfilesUseDB()	- 
**
** Description:
**	
**
** Inputs:
**	u_i4		: profile id
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
WSSProfilesUseDB(
	u_i4	db_id) 
{
	GSTATUS err = GSTAT_OK;
	u_i4	profile_id;
	WSS_ID_LIST *id_list;

	err = DDFSemOpen(&wss_profile, FALSE);
	if (err == GSTAT_OK)
	{
		for (profile_id = 1; err == GSTAT_OK && profile_id <= wss_profile_max; profile_id++)
			if (profiles[profile_id] != NULL)
			{
				id_list = profiles[profile_id]->dbs;	
				while (err == GSTAT_OK && id_list != NULL)
					if (id_list->id == db_id)
						err = DDFStatusAlloc( E_DF0059_DDS_DB_USED );
					else
						id_list = id_list->next;
			}
		CLEAR_STATUS(DDFSemClose(&wss_profile));
	}
return(err);
}

/*
** Name: WSSSelectProfileDB()	- 
**
** Description:
**	
**
** Inputs:
**	u_i4		: profile id
**
** Outputs:
**	u_nat*		: db id
**  char**		: db name
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
**      07-Mar-2000 (fanra01)
**          Replaced G_ASSERT macro with if statement and include a release
**          of the semaphore before returning error.
*/
GSTATUS
WSSSelectProfileDB(
    PTR     *cursor,
    u_i4    profile_id,
    u_i4    **db_id,
    char    **db_name )
{
    GSTATUS err = GSTAT_OK;
    WSS_ID_LIST *id_list;
    WSS_PPROFILE tmp = NULL;

    if (*cursor == NULL)
    {
        err = DDFSemOpen(&wss_profile, FALSE);
        if (err == GSTAT_OK)
        {
            if ((profile_id < 0) ||
                (profile_id > wss_profile_max) ||
                (profiles[profile_id] == NULL))
            {
                CLEAR_STATUS(DDFSemClose(&wss_profile));
                return(DDFStatusAlloc(E_WS0076_WSS_BAD_PROFILE));
            }
            id_list = profiles[profile_id]->dbs;
        }
    }
    else
    {
        id_list = (WSS_ID_LIST *) *cursor;
        id_list = id_list->next;
    }

    if (id_list != NULL)
    {
        err = GAlloc((PTR*)db_id, sizeof(id_list->id), FALSE);
        if (err == GSTAT_OK)
        {
            **db_id = id_list->id;
            err = DDSGetDBName(id_list->id, db_name);
        }

    }
    else
        CLEAR_STATUS(DDFSemClose(&wss_profile));
    *cursor = (char*) id_list;
    return(err);
}

/*
** Name: WSSDeleteProfileDB()	- 
**
** Description:
**	
**
** Inputs:
**	u_i4		: profile id
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
*/

GSTATUS 
WSSDeleteProfileDB (
	u_i4	profile_id, 
	u_i4	db_id) 
{
	GSTATUS err = GSTAT_OK;
	WSS_PPROFILE profile = NULL;

	err = DDFSemOpen(&wss_profile, TRUE);
	if (err == GSTAT_OK)
	{
		err = DDGDelete(
				&wsf_session, 
				WSS_OBJ_PROFILE_DB, 
				"%d%d", 
				&profile_id,
				&db_id);

		if (err == GSTAT_OK)
			err = WSSMemDeleteProfileDB(profile_id, db_id);

		if (err == GSTAT_OK) 
			err = DDGCommit(&wsf_session);
		else 
			CLEAR_STATUS(DDGRollback(&wsf_session));
		CLEAR_STATUS(DDFSemClose(&wss_profile));
	}
return(err);
}

/*
** Name: WSSProfileInitialize() - Initialisze the profile memory space
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
WSSProfileInitialize ()
{
	GSTATUS err = GSTAT_OK;
	err = DDFSemCreate(&wss_profile, "WSS for profiles");
return(err);
}

/*
** Name: WSSProfileTerminate() - free the profile memory space
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
WSSProfileTerminate () 
{
	GSTATUS err = GSTAT_OK;
	u_i4	id;
	for (id = 1; id <= wss_profile_max; id++)
	{
		if (profiles[id] != NULL)
		{
			MEfree((PTR)profiles[id]->name);
			MEfree((PTR)profiles[id]);
			profiles[id] = NULL;
		}
	}

	CLEAR_STATUS ( DDFSemDestroy(&wss_profile));
return(err);
}
