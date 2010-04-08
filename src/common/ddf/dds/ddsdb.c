/******************************************************************************
**
** Copyright (c) 2004 Ingres Corporation
**
******************************************************************************/

#include <ddgexec.h>
#include <ddsmem.h>

/**
**
**  Name: ddsdb.c - Data Dictionary Security Service Facility
**
**  Description:
**		This file contains all available functions to manage virtual database
**
**  History:    
**	5-Feb-98 (marol01)
**	    created
**	13-may-1998 (canor01)
**	    Replace GC_L_PASSWORD with DDS_PASSWORD_LENGTH.
**      06-Aug-1999 (fanra01)
**          Change nat to i4, u_nat to u_i4, longnat to i4, u_longnat to u_i4
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      10-Jan-2001 (fanra01)
**          Bug 103681
**          User id and flag fields were transposed leading to incorrect
**          dbuser associated with a database within the repository.
**	11-Oct-2002 (somsa01)
**	    Changed DDGInsert(), DDGUpdate(), DDGKeySelect() and DDGDelete()
**	    so that we pass addresses of int and float arguments.
**	27-Jul-2004 (hanje04)
**	    Move DDSGetDBForUser to ddsdbuser.c so that calls to gcn_decrypt()
**	    can be replaced with calls to DDSdecrypt() which is statically
**	    declared in ddsdbuser.c
**/

GLOBALREF SEMAPHORE dds_semaphore;
GLOBALREF DDG_SESSION dds_session;
STATUS gcn_decrypt(char*, char*, char*);

/*
** Name: DDSCreateDB() - DB creation in the Data Dictionary and in memory
**
** Description:
**	
**
** Inputs:
**	char*		: db name
**	char*		: physical dbname
**	u_nat		: db user id
**	longnat		: flag
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
**      10-Jan-2001 (fanra01)
**          Transpose flags and dbuser_id in DDSMemCreateDB call.
*/

GSTATUS 
DDSCreateDB (
    char *name,
    char *dbname,
    u_i4 dbuser_id,
    i4 flags,
    char *comment,
    u_i4 *id)
{
    GSTATUS err = GSTAT_OK;

    err = DDFSemOpen(&dds_semaphore, TRUE);
    if (err == GSTAT_OK)
    {
        err = DDSPerformDBId(id);
        if (err == GSTAT_OK)
        {
            err = DDGInsert(
                    &dds_session,
                    DDS_OBJ_DB,
                    "%d%s%s%d%d%s",
                    id,
                    name,
                    dbname,
                    &dbuser_id,
                    &flags,
                    comment);
            if (err == GSTAT_OK)
                err = DDSMemCreateDB( *id, name, dbname, dbuser_id, flags );

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
** Name: DDSUpdateDB()	- DB change in the Data Dictionary and in memory
**
** Description:
**	
**
** Inputs:
**	u_nat		: db id
**	char*		: dbname
**	char*		: physical dbname
**	u_nat		: db user id
**	longnat		: flag
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
DDSUpdateDB (
	u_i4	id, 
	char	*name, 
	char	*dbname, 
	u_i4	dbuser_id, 
	i4 flags,
	char *comment) 
{
	GSTATUS err = GSTAT_OK;

	err = DDFSemOpen(&dds_semaphore, TRUE);
	if (err == GSTAT_OK) 
	{
		err = DDGUpdate(
					&dds_session, 
					DDS_OBJ_DB, 
					"%s%s%d%d%s%d", 
					name, 
					dbname, 
					&dbuser_id, 
					&flags, 
					comment,
					&id);

		if (err == GSTAT_OK)
			err = DDSMemUpdateDB(id, name, dbname, flags, dbuser_id);

		if (err == GSTAT_OK) 
			err = DDGCommit(&dds_session);
		else 
			CLEAR_STATUS(DDGRollback(&dds_session));
		CLEAR_STATUS(DDFSemClose(&dds_semaphore));
	}
return(err);
}

/*
** Name: DDSDeleteDB() - DB delete in the Data Dictionary and in memory
**
** Description:
**	
**
** Inputs:
**	u_nat		: db id
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
DDSDeleteDB (
	u_i4 id) 
{
	GSTATUS err = GSTAT_OK;

	err = DDFSemOpen(&dds_semaphore, TRUE);
	if (err == GSTAT_OK) 
	{
		err = DDGDelete(&dds_session, DDS_OBJ_DB, "%d", &id);

		if (err == GSTAT_OK) 
			err = DDSMemDeleteDB(id);

		if (err == GSTAT_OK) 
			err = DDGCommit(&dds_session);
		else 
			CLEAR_STATUS(DDGRollback(&dds_session));

		CLEAR_STATUS(DDFSemClose(&dds_semaphore));
	}
return(err);
}

/*
** Name: DDSCreateAssUserDB()	- create relation between a user and a db
**
** Description:
**	
**
** Inputs:
**	u_nat		: user id
**	u_nat		: db id
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
DDSCreateAssUserDB (
	u_i4 user_id, 
	u_i4 db_id) 
{
	GSTATUS err = GSTAT_OK;

	err = DDFSemOpen(&dds_semaphore, TRUE);
	if (err == GSTAT_OK) 
	{
		err = DDSMemCheckAssUserDB(user_id, db_id);
		if (err == GSTAT_OK) 
		{
			err = DDGInsert(
					&dds_session, 
					DDS_OBJ_ASS_USER_DB, 
					"%d%d", 
					&user_id, 
					&db_id);

			if (err == GSTAT_OK)
				err = DDSMemCreateAssUserDB(user_id, db_id);

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
** Name: DDSDeleteAssUserDB()	- delete relation between a user and a db
**
** Description:
**	
**
** Inputs:
**	u_nat		: user id
**	u_nat		: db id
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
DDSDeleteAssUserDB (
	u_i4 user_id, 
	u_i4 db_id) 
{
	GSTATUS err = GSTAT_OK;

	err = DDFSemOpen(&dds_semaphore, TRUE);
	if (err == GSTAT_OK) 
	{
		err = DDGDelete(
				&dds_session, 
				DDS_OBJ_ASS_USER_DB, 
				"%d%d", 
				&user_id, 
				&db_id);

		if (err == GSTAT_OK)
			err = DDSMemDeleteAssUserDB(user_id, db_id);

		if (err == GSTAT_OK) 
			err = DDGCommit(&dds_session);
		else 
			CLEAR_STATUS(DDGRollback(&dds_session));

		CLEAR_STATUS(DDFSemClose(&dds_semaphore));
	}
return(err);
}

/*
** Name: DDSOpenDB()	- Open a cursor on the DB list
**
** Description:
**	
**
** Inputs:
**	PTR		*cursor
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
DDSOpenDB (PTR		*cursor) 
{
	GSTATUS err = GSTAT_OK;

	err = DDFSemOpen(&dds_semaphore, TRUE);
	if (err == GSTAT_OK) 
		err = DDGSelectAll(&dds_session, DDS_OBJ_DB);
return(err);
}

/*
** Name: DDSRetrieveDB()	- Select a unique DB
**
** Description:
**	
**
** Inputs:
**	u_nat : DB id
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
DDSRetrieveDB (
	u_i4	id, 
	char	**name, 
	char	**dbname, 
	u_i4	**dbuser_id, 
	i4 	**flags,
	char	**comment) 
{
	GSTATUS err = GSTAT_OK;

	err = DDFSemOpen(&dds_semaphore, TRUE);
	if (err == GSTAT_OK) 
	{
		err = DDGKeySelect(
				&dds_session, 
				DDS_OBJ_DB, 
				"%d%s%s%d%d%s",
				&id,
				name, 
				dbname, 
				dbuser_id, 
				flags, 
				comment);

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
** Name: DDSGetDB()	- get the current DB
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
**	13-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
*/

GSTATUS 
DDSGetDB (
	PTR		*cursor,
	u_i4	**id, 
	char	**name, 
	char	**dbname, 
	u_i4	**dbuser_id, 
	i4 **flags,
	char	**comment) 
{
	GSTATUS err = GSTAT_OK;
	bool	exist;

	err = DDGNext(&dds_session, &exist);
	if (err == GSTAT_OK && exist == TRUE)
		err = DDGProperties(
	   			&dds_session, 
				"%d%s%s%d%d%s", 
				id,
				name, 
				dbname, 
				dbuser_id, 
				flags, 
				comment);
	if (err != GSTAT_OK)
		exist = FALSE;
	*cursor = (char*) (SCALARP)exist;
return(err);
}

/*
** Name: DDSCloseDB()	- close the cursor on the DB list
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
DDSCloseDB (PTR		*cursor) 
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
** Name: DDSOpenUserDB()	- Open a cursor on the User/DB relation
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
DDSOpenUserDB (PTR *cursor) 
{
return(DDFSemOpen(&dds_semaphore, FALSE));
}

/*
** Name: DDSGetUserDB()	- 
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
DDSGetUserDB (
	PTR		*cursor,
	u_i4	user_id, 
	u_i4	**db_id,
	char	**db_name) 
{
return(DDSMemSelectAssUserDB(cursor, user_id, db_id, db_name));
}

/*
** Name: DDSCloseUserDB()	- close the cursor on the DB list
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
DDSCloseUserDB (PTR *cursor) 
{
	CLEAR_STATUS(DDFSemClose(&dds_semaphore));
return(GSTAT_OK);
}

/*
** Name: DDSGetDBName()	- return the DB name
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
DDSGetDBName(
	u_i4	id, 
	char** name)
{
	GSTATUS err = GSTAT_OK;

	err = DDFSemOpen(&dds_semaphore, FALSE);
	if (err == GSTAT_OK) 
	{
		err = DDSMemGetDBName(id, name);
		CLEAR_STATUS(DDFSemClose(&dds_semaphore));
	}
return(err);
}
