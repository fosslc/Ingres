/******************************************************************************
**
** Copyright (c) 2004 Ingres Corporation
**
******************************************************************************/

#include <ddgexec.h>
#include <ddsmem.h>

/**
**
**  Name: ddsinit.c - Data Dictionary Security Service Facility
**
**  Description:
**		This file contains all available functions to manage the DDS module.
**		DDS aims at offering a complete way to manage security 
**		based on a database Data Dictionary
**		This file controls communication with the Data Dictionary system 
**		and garantees the coherence
**
**  History:    
**	5-Feb-98 (marol01)
**	    created
**	14-Feb-98 (marol01)
**	    change parameters of the DDGInitialize function
**      28-May-98 (fanra01)
**          Add timeout parameter to methods.
**      18-Jan-1999 (fanra01)
**          Cleaned up compiler warnings on unix.
**      05-May-2000 (fanra01)
**          Bug 101346
**          Add user rights reference property.  Grant access to business unit
**          while the user holds permissions to one or more documents.
**
**      06-Aug-1999 (fanra01)
**          Change nat to i4, u_nat to u_i4, longnat to i4, u_longnat to u_i4
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

GLOBALDEF SEMAPHORE		dds_semaphore;
GLOBALDEF DDG_SESSION	dds_session;
static DDG_DESCR			descr;
/*
** Name: DDSInitialize()	- Initialization of the module 
**
** Description:
**	(have to be called only one time per executable)
**
**
** Inputs:
**	char*		: dll name
**	char*		: node name
**	char*		: database name
**	char*		: server class
**	char*		: user name
**	char*		: user password
**  u_nat		: hash table size for user
**	u_nat		: hash table size for role
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
**      05-May-2000 (fanra01)
**          Add refs property to the user right.
**          Updated DDSMemAssignUserRight to take the refs count.
*/
GSTATUS
DDSInitialize (
	char*	dllname,
	char*	node,
	char*	dictionary_name,
	char*	svrClass,
	char*	user_name,
	char*   password,
	u_i4	right_hash, 
	u_i4	user_hash)
{
    GSTATUS err = GSTAT_OK;
    bool exist;
    char *resource = NULL;
    u_i4 *id = NULL;
    u_i4 *id2 = NULL;
    u_i4 *right = NULL;
    u_i4 *refs = NULL;
    i4 *flags = NULL;
    i4 *timeout = NULL;
    i4 *authtype = NULL;
    char *name = NULL;
    char *dbname = NULL;
    char *pass = NULL;
    char *comment = NULL;
    bool status;

    err = DDFSemCreate(&dds_semaphore, "DDS");
    if (err == GSTAT_OK)
    {
        err = DDFSemOpen(&dds_semaphore, TRUE);
        if (err == GSTAT_OK)
        {
            err = DDSMemInitialize(right_hash, user_hash);
            if (err == GSTAT_OK)
                err = DDGInitialize(0, dllname, node, dictionary_name, svrClass, user_name, password, &descr);
            if (err == GSTAT_OK)
                err = DDGConnect(&descr, user_name, password, &dds_session);

            if (err == GSTAT_OK)
            {
                if (err == GSTAT_OK)
                {
                    err = DDGSelectAll(&dds_session, DDS_OBJ_DBUSER);
                    while (err == GSTAT_OK)
                    {
                        err = DDGNext(&dds_session, &exist);
                        if (err != GSTAT_OK || exist == FALSE)
                            break;
                        err = DDGProperties(
                                &dds_session,
                                "%d%s%s%s%s",
                                &id,
                                &name,
                                &dbname,
                                &pass,
                                &comment);

                        if (err == GSTAT_OK)
                            CLEAR_STATUS(DDSMemCreateDBUser (*id, name, dbname, pass));
                    }
                    CLEAR_STATUS( DDGClose(&dds_session) );
                }

                if (err == GSTAT_OK)
                {
                    err = DDGSelectAll(&dds_session, DDS_OBJ_USER);
                    while (err == GSTAT_OK)
                    {
                        err = DDGNext(&dds_session, &exist);
                        if (err != GSTAT_OK || exist == FALSE)
                            break;
                        err = DDGProperties(
                                &dds_session,
                                "%d%s%s%d%d%d%s%d",
                                &id,
                                &name,
                                &pass,
                                &id2,
                                &flags,
                                &timeout,
                                &comment,
                                                                &authtype);

                        if (err == GSTAT_OK)
                            CLEAR_STATUS(DDSMemCreateUser (
                                *id,
                                name,
                                pass,
                                *flags,
                                *timeout,
                                *id2,
                                                                *authtype));
                    }
                    CLEAR_STATUS( DDGClose(&dds_session) );
                }

                if (err == GSTAT_OK)
                {
                    err = DDGSelectAll(&dds_session, DDS_OBJ_USER_RIGHT);
                    while (err == GSTAT_OK)
                    {
                        err = DDGNext(&dds_session, &exist);
                        if (err != GSTAT_OK || exist == FALSE)
                            break;
                        err = DDGProperties(
                                &dds_session,
                                "%d%s%d%d",
                                &id,
                                &resource,
                                &right,
                                &refs);

                        if (err == GSTAT_OK)
                            CLEAR_STATUS(DDSMemAssignUserRight (
                                *id,
                                resource,
                                (i4*)right,
                                refs,
                                &status));
                    }
                    CLEAR_STATUS( DDGClose(&dds_session) );
                }

                if (err == GSTAT_OK)
                {
                    err = DDGSelectAll(&dds_session, DDS_OBJ_ROLE);
                    while (err == GSTAT_OK)
                    {
                        err = DDGNext(&dds_session, &exist);
                        if (err != GSTAT_OK || exist == FALSE)
                            break;
                        CLEAR_STATUS(DDGProperties(
                            &dds_session,
                            "%d%s%s",
                            &id,
                            &name,
                            &comment));

                        if (err == GSTAT_OK)
                            err = DDSMemCreateRole (*id, name);
                    }
                    CLEAR_STATUS( DDGClose(&dds_session) );
                }

                if (err == GSTAT_OK)
                {
                    err = DDGSelectAll(&dds_session, DDS_OBJ_ROLE_RIGHT);
                    while (err == GSTAT_OK)
                    {
                        err = DDGNext(&dds_session, &exist);
                        if (err != GSTAT_OK || exist == FALSE)
                            break;
                        err = DDGProperties(
                                &dds_session,
                                "%d%s%d",
                                &id,
                                &resource,
                                &right);

                        if (err == GSTAT_OK)
                            CLEAR_STATUS(DDSMemAssignRoleRight(
                                *id,
                                resource,
                                (i4*)right,
                                &status));
                    }
                    CLEAR_STATUS( DDGClose(&dds_session) );
                }

                if (err == GSTAT_OK)
                {
                    err = DDGSelectAll(&dds_session, DDS_OBJ_ASS_USER_ROLE);
                    while (err == GSTAT_OK)
                    {
                        err = DDGNext(&dds_session, &exist);
                        if (err != GSTAT_OK || exist == FALSE)
                            break;
                        err = DDGProperties(
                                &dds_session,
                                "%d%d",
                                &id,
                                &id2);

                        if (err == GSTAT_OK)
                            CLEAR_STATUS(DDSMemCreateAssUserRole(
                                *id,
                                *id2));
                    }
                    CLEAR_STATUS( DDGClose(&dds_session) );
                }

                if (err == GSTAT_OK)
                {
                    err = DDGSelectAll(&dds_session, DDS_OBJ_DB);
                    while (err == GSTAT_OK)
                    {
                        err = DDGNext(&dds_session, &exist);
                        if (err != GSTAT_OK || exist == FALSE)
                            break;
                        err = DDGProperties(
                                &dds_session,
                                "%d%s%s%d%d%s",
                                &id,
                                &name,
                                &dbname,
                                &id2,
                                &flags,
                                &comment);

                        if (err == GSTAT_OK)
                            CLEAR_STATUS(DDSMemCreateDB(
                                *id,
                                name,
                                dbname,
                                *id2,
                                *flags));
                    }
                    CLEAR_STATUS( DDGClose(&dds_session) );
                }

                if (err == GSTAT_OK)
                {
                    err = DDGSelectAll(&dds_session, DDS_OBJ_ASS_USER_DB);
                    while (err == GSTAT_OK)
                    {
                        err = DDGNext(&dds_session, &exist);
                        if (err != GSTAT_OK || exist == FALSE)
                            break;
                        err = DDGProperties(
                                &dds_session,
                                "%d%d",
                                &id,
                                &id2);

                        if (err == GSTAT_OK)
                            CLEAR_STATUS(DDSMemCreateAssUserDB(
                                *id,
                                *id2));
                    }
                    CLEAR_STATUS( DDGClose(&dds_session) );
                }

                if (err == GSTAT_OK)
                    err = DDGCommit(&dds_session);
                else
                    CLEAR_STATUS(DDGRollback(&dds_session));
            }
            CLEAR_STATUS(DDFSemClose(&dds_semaphore));
        }
    }

    MEfree((PTR)resource);
    MEfree((PTR)id);
    MEfree((PTR)id2);
    MEfree((PTR)right);
    MEfree((PTR)name);
    MEfree((PTR)dbname);
    MEfree((PTR)pass);
    MEfree((PTR)comment);
    return(err);
}

/*
** Name: DDSTerminate()	- Terminate the DDS module and free memory
**
** Description:
**	look for the first user id available.
**
** Inputs:
**
** Outputs:
**	u_nat		: available id 
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
DDSTerminate () 
{
	GSTATUS err1 = GSTAT_OK;
	GSTATUS err2 = GSTAT_OK;

	err1 = DDFSemOpen(&dds_semaphore, TRUE);
	if (err1 == GSTAT_OK) 
	{
		err1 = DDSMemTerminate();
		err2 = DDGDisconnect(&dds_session);
		if (err2 == GSTAT_OK)
			err2 = DDGTerminate(&descr);

		err1 = (err2) ? err2 : err1;
		CLEAR_STATUS(DDFSemClose(&dds_semaphore));
	}
	CLEAR_STATUS( DDFSemDestroy(&dds_semaphore) );
return(err1);
}
