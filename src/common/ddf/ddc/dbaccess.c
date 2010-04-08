/******************************************************************************
**
** Copyright (c) 2004 Ingres Corporation
**
******************************************************************************/

#include <dbaccess.h>
#include <erddf.h>

#include <ddcmo.h>

/*
**
**  Name: dbaccess.c - Data Dictionary Services Facility Driver Connection
**
**  Description:
**	The following functions aims at providing a way to load multiple drivers
**
**  History:    
**	5-Feb-98 (marol01)
**	    created
**	4-mar-98 (marol01)
**	    Extra flag parameter for DLprepare_loc
**      06-Aug-1999 (fanra01)
**          Change nat to i4, u_nat to u_i4, longnat to i4, u_longnat to u_i4
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      05-Apr-2001 (fanra01)
**          Bug 104422
**          Add differentiation of database connections to include user name.
**      27-Mar-2003 (fanra01)
**          Bug 109934
**          When manipulating the available session queue pointers the queue
**          was being broken and attached to the wrong point.
**          Active sessions are being removed prematurely.
**/

GLOBALDEF PDB_ACCESS_DEF	*drivers = NULL; /* define the driver drivers */
static u_i4							driver_max = 0;
static PDB_CACHED_SESSION	oldest = NULL;
static PDB_CACHED_SESSION	youngest = NULL;

GLOBALDEF SEMAPHORE				globalDCM;

PDB_ACCESS_DEF DDFGetDriver(u_i4 number) { return drivers[number]; }

/*
** Name: DDFInitialize()	- Initialization 
**
** Description:
**
** Inputs:
**
** Outputs:
**
** Returns:
**	GSTATUS		: E_OG0001_OUT_OF_MEMORY
**							GSTAT_OK
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
DDFInitialize()
{
    GSTATUS err = GSTAT_OK;

    ddc_define();
    err = DDFSemCreate(&globalDCM, "DCM");

    err = G_ME_REQ_MEM(0, drivers, PDB_ACCESS_DEF, 1);
    if (err == GSTAT_OK)
        driver_max = 1;
    return(err);
}

/*
** Name: DDFTerminate()	- Unload the dll from memory
**
** Description:
**
** Inputs:
**	u_nat		: driver number
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
DDFTerminate() 
{
	u_i4	i;

	if (drivers != NULL)
	{
		for (i = 0; i < driver_max; i++)
			if (drivers[i] != NULL)
			{
				DDFHASHSCAN sc;
				PDB_CACHED_SESSION_SET object = NULL;

				if (drivers[i]->cached_sessions != NULL)
				{
					DDF_INIT_SCAN(sc, drivers[i]->cached_sessions);
					while (DDF_SCAN(sc, object) == TRUE && object != NULL)
					{
						while (object->sessions != NULL)
							CLEAR_STATUS(DBRemove(object->sessions, FALSE));
						MEfree(object->name);
						MEfree((PTR)object);
					}
				}
				
				CLEAR_STATUS(DDFrmhash(&drivers[i]->cached_sessions));

				if (drivers[i]->handle != NULL)
				{
					CL_ERR_DESC err_desc;
					DLunload(drivers[i]->handle, &err_desc); 
					drivers[i]->handle = NULL;
				}
				MEfree(drivers[i]->name);
				MEfree((PTR)drivers[i]);
				drivers[i] = NULL;
			}
		MEfree((PTR)drivers);
	}
return(GSTAT_OK);
}

/*
** Name: DriverLoad()	- Load a specific driver
**
** Description:
**	the goal of this function is to load the dll and to  insert it 
**	into the driver table list
**
** Inputs:
**	char*		: dll driver name
**  char*		: path where the system will find the dll
**
** Outputs:
**	u_nat		: driver number (this value has to user to call method) 
**
** Returns:
**	GSTATUS		: E_OD0022_PROC_DRV_ERROR
**				  E_OD0021_OPEN_DRV_ERROR	
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
**      14-Jan-1999 (fanra01)
**          Assume DLprepare_loc will attempt default library paths if no
**          location is provided.
*/

GSTATUS 
DriverLoad(
	char *name, 
	u_i4 *number) 
{
	GSTATUS err = GSTAT_OK;
	char *tmp1 = NULL;
	char *tmp2 = NULL;
	bool found = FALSE;
	u_i4 newID = 0;
	u_i4 i;

	G_ASSERT(drivers == NULL, E_DF0064_UNINITLIZED_DRIVER) ;

	for (i = 0; i < driver_max; i++) 
	{
		if (found == FALSE && drivers[i] == NULL) 
		{
			found = TRUE;
			newID = i;
		}
		if (drivers[i] != NULL) 
		{
			if (strcmp(drivers[i]->name, name) == 0) 
			{
				*number = i;
				return(GSTAT_OK);
			}
		}
	}
	if (newID == driver_max) 
	{
		PDB_ACCESS_DEF *tmp = drivers;
		err = G_ME_REQ_MEM(0, drivers, PDB_ACCESS_DEF, driver_max+1);
		if (err != GSTAT_OK)
			drivers = tmp;
		else 
		{
			if (tmp != NULL) 
			{
				MECOPY_VAR_MACRO(tmp, 
					sizeof(PDB_ACCESS_DEF) * driver_max, 
					drivers);
				MEfree((PTR)tmp);
			}
			newID = driver_max++;
		}
	}

	if (err == GSTAT_OK && drivers[newID] == NULL) 
	{
		CL_ERR_DESC err_desc;
		err = G_ME_REQ_MEM(0, drivers[newID], DB_ACCESS_DEF, 1);
		if (err == GSTAT_OK)
		{
			STATUS  status;
			
			if (DLprepare_loc(NULL, 
							name, 
							NULL, 
							NULL,
							0,
							&drivers[newID]->handle, 
							&err_desc) != OK)
				err = DDFStatusAlloc (E_DF0018_OPEN_DRV_ERROR);
			else
			{
				status = DLbind(
							drivers[newID]->handle, 
							"PDBDriverName", 
							(PTR *) &(drivers[newID]->PDBDriverName), 
							&err_desc);

				if (status == OK)
					status = DLbind(
							drivers[newID]->handle, 
							"PDBConnect", 
							(PTR *) &(drivers[newID]->PDBConnect), 
							&err_desc);

				if (status == OK)
					status = DLbind(
							drivers[newID]->handle, 
							"PDBCommit", 
							(PTR *) &(drivers[newID]->PDBCommit), 
							&err_desc);

				if (status == OK)
					status = DLbind(
							drivers[newID]->handle, 
							"PDBRollback", 
							(PTR *) &(drivers[newID]->PDBRollback), 
							&err_desc);

				if (status == OK)
					status = DLbind(
							drivers[newID]->handle, 
							"PDBDisconnect", 
							(PTR *) &(drivers[newID]->PDBDisconnect), 
							&err_desc);

				if (status == OK)
					status = DLbind(
							drivers[newID]->handle, 
							"PDBExecute", 
							(PTR *) &(drivers[newID]->PDBExecute), 
							&err_desc);

				if (status == OK)
					status = DLbind(
							drivers[newID]->handle, 
							"PDBPrepare", 
							(PTR *) &(drivers[newID]->PDBPrepare), 
							&err_desc);

				if (status == OK)
					status = DLbind(
							drivers[newID]->handle, 
							"PDBGetStatement", 
							(PTR *) &(drivers[newID]->PDBGetStatement), 
							&err_desc);

				if (status == OK)
					status = DLbind(
							drivers[newID]->handle, 
							"PDBParams", 
							(PTR *) &(drivers[newID]->PDBParams), 
							&err_desc);

				if (status == OK)
					status = DLbind(
							drivers[newID]->handle, 
							"PDBParamsFromTab", 
							(PTR *) &(drivers[newID]->PDBParamsFromTab), 
							&err_desc);

				if (status == OK)
					status = DLbind(
							drivers[newID]->handle, 
							"PDBNumberOfProperties", 
							(PTR *) &(drivers[newID]->PDBNumberOfProperties), 
							&err_desc);

				if (status == OK)
					status = DLbind(
							drivers[newID]->handle, 
							"PDBName", 
							(PTR *) &(drivers[newID]->PDBName), 
							&err_desc);

				if (status == OK)
					status = DLbind(
							drivers[newID]->handle, 
							"PDBType", 
							(PTR *) &(drivers[newID]->PDBType), 
							&err_desc);

				if (status == OK)
					status = DLbind(
							drivers[newID]->handle, 
							"PDBText", 
							(PTR *) &(drivers[newID]->PDBText), 
							&err_desc);

				if (status == OK)
					status = DLbind(
							drivers[newID]->handle, 
							"PDBInteger", 
							(PTR *) &(drivers[newID]->PDBInteger), 
							&err_desc);

				if (status == OK)
					status = DLbind(
							drivers[newID]->handle, "PDBFloat", 
							(PTR *) &(drivers[newID]->PDBFloat), 
							&err_desc);

				if (status == OK)
					status = DLbind(
							drivers[newID]->handle, 
							"PDBDate", 
							(PTR *) &(drivers[newID]->PDBDate), 
							&err_desc);

				if (status == OK)
					status = DLbind(
							drivers[newID]->handle, 
							"PDBBlob", 
							(PTR *) &(drivers[newID]->PDBBlob), 
							&err_desc);

				if (status == OK)
					status = DLbind(
							drivers[newID]->handle, 
							"PDBNext", 
							(PTR *) &(drivers[newID]->PDBNext), 
							&err_desc);

				if (status == OK)
					status = DLbind(
							drivers[newID]->handle, 
							"PDBClose", 
							(PTR *) &(drivers[newID]->PDBClose), 
							&err_desc);

				if (status == OK)
					status = DLbind(
							drivers[newID]->handle, 
							"PDBRelease", 
							(PTR *) &(drivers[newID]->PDBRelease), 
							&err_desc);

				if (status == OK)
					status = DLbind(
							drivers[newID]->handle, 
							"PDBRepInsert", 
							(PTR *) &(drivers[newID]->PDBRepInsert), 
							&err_desc);

				if (status == OK)
					status = DLbind(
							drivers[newID]->handle, 
							"PDBRepUpdate", 
							(PTR *) &(drivers[newID]->PDBRepUpdate), 
							&err_desc);

				if (status == OK)
					status = DLbind(
							drivers[newID]->handle, 
							"PDBRepDelete", 
							(PTR *) &(drivers[newID]->PDBRepDelete), 
							&err_desc);

				if (status == OK)
					status = DLbind(
							drivers[newID]->handle, 
							"PDBRepSelect", 
							(PTR *) &(drivers[newID]->PDBRepSelect), 
							&err_desc);

				if (status == OK)
					status = DLbind(
							drivers[newID]->handle, 
							"PDBRepKeySelect", 
							(PTR *) &(drivers[newID]->PDBRepKeySelect), 
							&err_desc);

				if (status == OK)
					status = DLbind(
							drivers[newID]->handle, 
							"PDBRepObjects", 
							(PTR *) &(drivers[newID]->PDBRepObjects), 
							&err_desc);

				if (status == OK)
					status = DLbind(
							drivers[newID]->handle, 
							"PDBRepProperties", 
							(PTR *) &(drivers[newID]->PDBRepProperties), 
							&err_desc);

				if (status == OK)
					status = DLbind(
							drivers[newID]->handle, 
							"PDBRepStatements", 
							(PTR *) &(drivers[newID]->PDBRepStatements), 
							&err_desc);

				if (status == OK)
					status = DLbind(
							drivers[newID]->handle, 
							"PDBGenerate", 
							(PTR *) &(drivers[newID]->PDBGenerate), 
							&err_desc);

				if (status != OK)
					err = DDFStatusAlloc (E_DF0019_PROC_DRV_ERROR);

			}
		}
		if (err == GSTAT_OK)
		{
			err = DDFmkhash(10, &drivers[newID]->cached_sessions);
			if (err == GSTAT_OK)
			{
				G_ST_ALLOC(drivers[newID]->name, name);
				*number = newID;
			}
		}
	}
return(err);
}

/*
** Name: DBRemoveTimeout() - Remove the session from the timeout list
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
DBRemoveTimeout(
	PDB_CACHED_SESSION session) 
{
	GSTATUS err = GSTAT_OK;

	if (session == oldest)
	{
		oldest = session->next;
		if (oldest != NULL)
			oldest->previous = NULL;
	}
	else if (session->previous != NULL)
		session->previous->next = session->next;

	if (session== youngest)
	{
		youngest = session->previous;
		if (youngest != NULL)
			youngest->next = NULL;
	}
	else if (session->next != NULL)
		session->next->previous = session->previous;

	session->previous = NULL;
	session->next = NULL;
return(err);
}

/*
** Name: DBSetTimeout() - Set the timeout system
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
DBSetTimeout(
	PDB_CACHED_SESSION session) 
{
	GSTATUS err = GSTAT_OK;

	err = DBRemoveTimeout(session);
	if (err == GSTAT_OK)
	{
		if (session->used == TRUE)
			session->timeout_end = 0;
		else
			session->timeout_end = TMsecs() + session->timeout;

		if (youngest == NULL)
		{
			youngest = session;
			youngest->next = NULL;
			oldest = session;
			oldest->previous = NULL;
		}
		else
		{
			if (session->timeout_end == 0)
			{
				youngest->next = session;
				session->previous = youngest;
				youngest = session;
				youngest->next = NULL;
			}
			else
			{
				PDB_CACHED_SESSION tmp = youngest;
				while (tmp != NULL && tmp->timeout_end > session->timeout_end)
					tmp = tmp->previous;

				if (tmp == NULL)
				{
					oldest->previous = session;
					session->next = oldest;
					oldest = session;
					oldest->previous = NULL;
				}
				else
				{
					if (tmp == youngest)
						youngest = session;
					else
					{
						session->next = tmp->next;
						tmp->next->previous = session;
					}
					tmp->next = session;
					session->previous = tmp;
				}
			}
		}
	}
return(err);
}

/*
** Name: DBCachedConnect()	- Connection Cache System
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
**      27-Mar-2003 (fanra01)
**          Changed manipulation of available sessions to use the correct
**          queue pointer.
**	06-Jun-2002 (fanra01)
**	    Test user for existance before trying to allocate memory for it on
**	    first connect.
*/
GSTATUS 
DBCachedConnect(
    u_i4                type,
    char*               node,
    char*               dbname,
    char*               svrClass,
    char*               flag,
    char*               user,
    char*               password,
    i4                  timeout,
    PDB_CACHED_SESSION  *session)
{
    GSTATUS                 err = GSTAT_OK;
    PDB_CACHED_SESSION_SET  tmp = NULL;
    char                    nullstr[] = "";
    PDB_ACCESS_DEF          driver = DDFGetDriver(type);
    u_i4                    length;
    char*                   name = NULL;
    bool                    openConnection = FALSE;

    G_ASSERT(dbname == NULL || dbname[0] == EOS, E_DF0039_DB_INCORRECT_NAME);

    if (node == NULL)        node = nullstr;
    if (svrClass == NULL)    svrClass = nullstr;
    if (flag == NULL)        flag = nullstr;

    length = STlength(node) + STlength(dbname) + STlength(svrClass) +
             STlength(flag) + 5;

    err = DDFSemOpen(&globalDCM, TRUE);
    if (err == GSTAT_OK)
    {
        err = G_ME_REQ_MEM( 0, name, char, length);

        if (err == GSTAT_OK)
        {
            if (node != NULL && node[0] != EOS)
            {
                STcat(name, node);
                STcat(name, "::");
            }
            STcat(name, dbname);
            if (svrClass != NULL && svrClass[0] != EOS)
            {
                STcat(name, "/");
                STcat(name, svrClass);
            }
            if(flag != NULL && flag[0] != EOS)
            {
                STcat(name, "?");
                STcat(name, flag);
            }
            err = DDFgethash( driver->cached_sessions, name, HASH_ALL,
                (PTR*) &tmp );

            if (err == GSTAT_OK && tmp == NULL)
            {
                err = G_ME_REQ_MEM( 0, tmp, DB_CACHED_SESSION_SET, 1);
                if (err == GSTAT_OK)
                {
                    tmp->name = name;
                    err = DDFputhash(
                            driver->cached_sessions,
                            tmp->name,
                            HASH_ALL,
                            (PTR)tmp);
                    if (err != GSTAT_OK)
                    {
                        MEfree((PTR)name);
                        MEfree((PTR)tmp);
                    }
                }
            }
            else
                MEfree((PTR)name);

            if (err == GSTAT_OK)
            {
                PDB_CACHED_SESSION scan = tmp->sessions;
                PDB_CACHED_SESSION active = NULL;

                /*
                ** if there are already sessions open to the database
                */
                if (scan != NULL)
                {
                    /*
                    ** scan the list for a matching criteria
                    */
                    while (scan != NULL)
                    {
                        /*
                        ** if user for connection is the admin user and the
                        ** requested user is also admin then found.
                        */
                        if ((user == NULL) && (scan->dbuser == NULL))
                        {
                            break;
                        }
                        else
                        {
                            /*
                            ** if neither of the users is the admin user
                            ** then compare the dbuser names.
                            */
                            if ((scan->dbuser != NULL) && (user != NULL))
                            {
                                if (STcompare( scan->dbuser, user ) != 0)
                                {
                                    scan = scan->next_in_set;
                                }
                                else
                                {
                                    break;
                                }
                            }
                            else
                            {
                                scan = scan->next_in_set;
                            }
                        }
                    }
                    /*
                    ** if found a match remove the entry from the list of
                    ** available sessions.
                    */
                    if ((active = scan) != NULL)
                    {
                        PDB_CACHED_SESSION lstprev = active->previous_in_set;
                        PDB_CACHED_SESSION lstnext = active->next_in_set;

                        /*
                        ** Use correct queue pointer for set
                        */
                        if (lstprev != NULL)
                        {
                            lstprev->next_in_set = active->next_in_set;
                        }
                        if (lstnext != NULL)
                        {
                            lstnext->previous_in_set = lstprev;
                        }
                        active->next_in_set = NULL;
                        active->previous_in_set = NULL;
                        if (active == tmp->sessions)
                        {
                            if ((tmp->sessions = lstnext) != NULL)
                            {
                                lstnext->previous_in_set = NULL;
                            }
                        }
                    }
                    *session = active;
                }
                if (active == NULL)
                {
                    err = G_ME_REQ_MEM( 0, *session, DB_CACHED_SESSION, 1);
                    if (err == GSTAT_OK)
                    {
                        (*session)->driver = type;
                        (*session)->set = tmp;
                        (*session)->timeout = timeout;
                        (*session)->dbuser = NULL;
                        if ((user != NULL) && (*user != 0))
                        {
                            (*session)->dbuser = STalloc( user );
                        }
                        (*session)->connflags = 0;
                        openConnection = TRUE;
                    }
                }
                if (err == GSTAT_OK)
                {
                    (*session)->used = TRUE;
                    err = DBSetTimeout(*session);
                }
            }
        }
        CLEAR_STATUS(DDFSemClose(&globalDCM));
    }
    if (err == GSTAT_OK && openConnection == TRUE)
    {
        err = driver->PDBConnect(
            &(*session)->session,
            node,
            dbname,
            svrClass,
            user,
            password,
            timeout);

        if (err == GSTAT_OK)
            ddc_sess_attach (*session);
    }
    if (err != GSTAT_OK)
    {
        if (*session != NULL)
        {
            CLEAR_STATUS(DDFSemOpen(&globalDCM, TRUE));
            CLEAR_STATUS(DBRemoveTimeout(*session));
            CLEAR_STATUS(DDFSemClose(&globalDCM));
            MEfree((PTR) *session);
        }
        *session = NULL;
    }
return(err);
}

/*
** Name: DBCachedDisconnect()	- Connection Cache System
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
DBCachedDisconnect (
	PDB_CACHED_SESSION	*session)
{
	GSTATUS err = GSTAT_OK;
	PDB_ACCESS_DEF	driver = DDFGetDriver((*session)->driver);

	if (session != NULL && *session != NULL)
	{
		err = driver->PDBRollback(&(*session)->session);
		if (err == GSTAT_OK)
		{
			(*session)->used = FALSE;
			err = DDFSemOpen(&globalDCM, TRUE);
			if (err == GSTAT_OK)
			{
				err = DBSetTimeout(*session);
				if (err == GSTAT_OK)
				{
					(*session)->next_in_set = (*session)->set->sessions;
					if ((*session)->set->sessions != NULL)
						(*session)->set->sessions->previous_in_set = (*session);
					(*session)->set->sessions = (*session);
				}
				CLEAR_STATUS(DDFSemClose(&globalDCM));
			}

		}
	}
	*session = NULL;
return(err);
}

/*
** Name: DBDelete() - Remove a session
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
**      04-Apr-2001 (fanra01)
**          Add free of database user name created in DBCachedConnect.
*/
GSTATUS
DBDelete( PDB_CACHED_SESSION cache_session )
{
    GSTATUS err = GSTAT_OK;

    if (cache_session->used == FALSE)
    {
        PDB_ACCESS_DEF    driver = DDFGetDriver(cache_session->driver);
        err = DBRemoveTimeout(cache_session);
        if (err == GSTAT_OK)
        {
            if (cache_session->next_in_set != NULL)
                cache_session->next_in_set->previous_in_set = cache_session->previous_in_set;

            if (cache_session->previous_in_set != NULL)
                cache_session->previous_in_set->next_in_set = cache_session->next_in_set;

            if (cache_session == cache_session->set->sessions)
                cache_session->set->sessions = cache_session->next_in_set;

            ddc_sess_detach (cache_session);
            err = driver->PDBDisconnect(&cache_session->session);
            if (cache_session->dbuser != NULL)
            {
                MEfree( cache_session->dbuser );
            }
            MEfree((PTR) cache_session);
        }
    }
    return(err);
}

/*
** Name: DBRemove() - Remove a session
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
DBRemove(
	PDB_CACHED_SESSION cache_session, 
	bool			   checkAdd)
{
	GSTATUS err = GSTAT_OK;

	err = DDFSemOpen(&globalDCM, TRUE);
	if (err == GSTAT_OK)
	{
		if (checkAdd == TRUE)
		{
			PDB_CACHED_SESSION tmp = oldest;
			while (tmp != NULL && tmp != cache_session)
				tmp = tmp->next;
			if (tmp == NULL)
				err = DDFStatusAlloc (E_DF0063_INCORRECT_KEY);
		}
		if (err == GSTAT_OK)
        {
			err = DBDelete(cache_session);
        }
		CLEAR_STATUS(DDFSemClose(&globalDCM));
	}
return(err);
}

/*
** Name: DBCleanTimeout() - Clean the timeout system
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
**      27-Mar-2003 (fanra01)
**          Modified test for removing idle sessions.
*/
GSTATUS
DBCleanTimeout() 
{
	GSTATUS err = GSTAT_OK;
	PDB_CACHED_SESSION tmp = oldest;
	PDB_CACHED_SESSION next;
	i4 limit = TMsecs();

	err = DDFSemOpen(&globalDCM, TRUE);
	if (err == GSTAT_OK)
	{
		while (err == GSTAT_OK &&
			   tmp != NULL)
		{
			next = tmp->next;
            /*
            ** Only remove sessions that have timed out
            ** A timeout_end value of zero should indicate that the
            ** session is still active.
            */
            if ((tmp->timeout_end != 0) && (tmp->timeout_end < limit))
            {
                err = DBDelete(tmp);
            }
			tmp = next;
		}
		CLEAR_STATUS(DDFSemClose(&globalDCM));
	}
return(err);
}

/*
** Name: DBBrowse() - 
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
DBBrowse(
	PTR		*cursor,
	PDB_CACHED_SESSION *conn)
{
	GSTATUS err = GSTAT_OK;

	if (*cursor == NULL)
	{
		err = DDFSemOpen(&globalDCM, FALSE);
		if (err == GSTAT_OK)
		{
			*cursor = (PTR) oldest;
			*conn = oldest;
		}
	}
	else
	{
		*conn = (PDB_CACHED_SESSION) *cursor;
		*conn = (*conn)->next;
		*cursor = (PTR) *conn;
	}

	if (*conn == NULL)
		CLEAR_STATUS(DDFSemClose(&globalDCM));
return(err);
}
