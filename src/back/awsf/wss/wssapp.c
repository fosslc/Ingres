/******************************************************************************
**
**Copyright (c) 2004 Ingres Corporation
**
******************************************************************************/

#include <wssapp.h>
#include <ddfhash.h>
#include <ddfsem.h>
#include <erwsf.h>
#include <wsf.h>
#include <ddgexec.h>

/*
**
**  Name: wssapp.c - Application management
**
**  Description:
**	
**
**  History:    
**	20-Aug-98 (marol01)
**	    created
**      11-Sep-1998 (fanra01)
**          Fixup compiler warnings on unix.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	11-oct-2002 (somsa01)
**	    Changed DDGInsert(), DDGUpdate() and DDGDelete() such that we
**	    pass the address of int and float arguments.
**	13-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
**/

typedef struct __WSS_APPLICATION
{
	u_i4					id;
	char					*name;
} WSS_APPLICATION;

GLOBALREF DDG_SESSION	wsf_session;	/* repository connection */
static DDFHASHTABLE			applications = NULL;
static WSS_APPLICATION	**applications_id = NULL;
static u_i4						wss_application_max = 0;
GLOBALDEF SEMAPHORE			wss_application;

/*
** Name: WSSAppInitialize() - Initialisze the application memory space
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
WSSAppInitialize ()
{
	GSTATUS err = GSTAT_OK;
	char*	size= NULL;
	char	szConfig[CONF_STR_SIZE];
	u_i4	numberOfApp= 5;

	err = DDFSemCreate(&wss_application, "WSS for applications");
	STprintf (szConfig, CONF_APPLICATION_SIZE, PMhost());
	if (PMget( szConfig, &size ) == OK && size != NULL)
		CVan(size, (i4*)&numberOfApp);

	err = DDFmkhash(numberOfApp, &applications);
	if (err == GSTAT_OK)
	{
		err = G_ME_REQ_MEM(
				0, 
				applications_id, 
				WSS_APPLICATION*, 
				WSS_DEFAULT_APPLICATION);

		wss_application_max = WSS_DEFAULT_APPLICATION;
	}
return(err);
}

/*
** Name: WSSAppTerminate() - free the application memory space
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
WSSAppTerminate () 
{
	GSTATUS err = GSTAT_OK;
	
	err = DDFSemOpen(&wss_application, TRUE);
	if (err == GSTAT_OK)
	{
		if (applications_id != NULL)
		{
			u_i4 i;
			WSS_APPLICATION *object = NULL;
			for (i = 0; i < wss_application_max; i++)
				if (applications_id[i] != NULL)
				{
					err = DDFdelhash(applications, 
														applications_id[i]->name, 
														HASH_ALL, 
														(PTR*) &object);

					if (err == GSTAT_OK && object != NULL) 
					{
						MEfree((PTR)object->name);
						applications_id[i] = NULL;
					}
				}
		}
		CLEAR_STATUS( DDFSemClose(&wss_application));
	}
	MEfree((PTR)applications_id);
	CLEAR_STATUS (DDFrmhash(&applications));
	CLEAR_STATUS ( DDFSemDestroy(&wss_application));
return(err);
}

/*
** Name: WSSPerformAppId() - Generate a new application id
**
** Description:
**
** Inputs:
**
** Outputs:
**	u_nat*	: application id
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
WSSPerformAppId (
	u_i4 *id) 
{
	GSTATUS err = GSTAT_OK;
	u_i4	i = 1;

	for (i = 1; i < wss_application_max && applications_id[i] != NULL; i++) ;

	*id = i;
return(err);
}

/*
** Name: WSSAppInit() - Initialize an application
**
** Description:
**
** Inputs:
**	WSS_APPLICATION*	: object
**	u_i4			: application id
**	char*			: application name
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
WSSAppInit (
	WSS_APPLICATION	*object,
	u_i4			id,
	char*			name) 
{
	GSTATUS err = GSTAT_OK;
	u_i4		length;

	length = STlength(name) + 1;
	err = G_ME_REQ_MEM(0, object->name, char, length);
	if (err == GSTAT_OK) 
	{
		MECOPY_VAR_MACRO(name, length, object->name);
		object->id = id;
	}
return(err);
}

/*
** Name: WSSAppLoad() - load a new application into the ICE system
**
** Description:
**
** Inputs:
**	u_i4 : application id
**	char* : application name
**
** Outputs:
**
** Returns:
**	GSTATUS	:	...
**				GSTAT_OK
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
WSSAppLoad (
	u_i4 id,
	char* name) 
{
	GSTATUS err = GSTAT_OK;
	WSS_APPLICATION *object = NULL;

	if (id >= wss_application_max) 
	{
		WSS_APPLICATION* *tmp = applications_id;
		err = G_ME_REQ_MEM(
										0, 
										applications_id, 
										WSS_APPLICATION*, 
										id + WSS_DEFAULT_APPLICATION);

		if (err == GSTAT_OK && tmp != NULL) 
			MECOPY_CONST_MACRO(
										tmp, 
										sizeof(WSS_APPLICATION*)*wss_application_max, 
										applications_id);

		if (err == GSTAT_OK)
		{
			MEfree((PTR)tmp);
			wss_application_max = id + WSS_DEFAULT_APPLICATION;
		}
		else
			applications_id = tmp;
	}

	if (err == GSTAT_OK)
	{
		err = G_ME_REQ_MEM(0, object, WSS_APPLICATION, 1);
		if (err == GSTAT_OK)
			err = WSSAppInit (object, id, name);

		if (err == GSTAT_OK) 
		{
			applications_id[id] = object;
			err = DDFputhash(applications, object->name, HASH_ALL,(PTR)object);
		}
	}
return(err);
}

/*
** Name: WSSCreateApp() - Create a new application into the ICE system
**
** Description:
**	Update the repository and the memory
**
** Inputs:
**	char* : application name
**
** Outputs:
**	u_nat* : application id
**
** Returns:
**	GSTATUS	:	...
**				GSTAT_OK
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
WSSAppCreate (
	u_i4 *id,
	char* name) 
{
	GSTATUS err = GSTAT_OK;
	
	err = DDFSemOpen(&wss_application, TRUE);
	if (err == GSTAT_OK) 
	{
		err = WSSPerformAppId(id);
		if (err == GSTAT_OK)
		{
			err = DDGInsert(
					&wsf_session, 
					WSS_OBJ_APPLICATION,
					"%d%s", 
					id, 
					name);

			if (err == GSTAT_OK)
				err = WSSAppLoad(*id, name);

			if (err == GSTAT_OK) 
				err = DDGCommit(&wsf_session);
			else 
				CLEAR_STATUS(DDGRollback(&wsf_session));
		}
		CLEAR_STATUS( DDFSemClose(&wss_application));
	}
return(err);
}

/*
** Name: WSSAppUpdate() - update a application into the ICE system
**
** Description:
**	Update the repository and the memory
**
** Inputs:
**	u_i4 :	unit id
**	char* : application name
**
** Outputs:
**
** Returns:
**	GSTATUS	:	...
**				GSTAT_OK
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
WSSAppUpdate (
	u_i4	id,
	char*	name) 
{
	GSTATUS err = GSTAT_OK;

	err = DDFSemOpen(&wss_application, TRUE);
	if (err == GSTAT_OK) 
	{
		if (id < 1 || 
			 id >= wss_application_max ||
			 applications_id[id] == NULL)
			 err = DDFStatusAlloc(E_WS0089_WSS_BAD_APP);
		else
		{
			err = DDGUpdate(
					&wsf_session, 
					WSS_OBJ_APPLICATION,
					"%s%d", 
					name,
					&id);

			if (err == GSTAT_OK)
			{
				WSS_APPLICATION *object = NULL;
				err = DDFdelhash(	applications, 
														applications_id[id]->name, 
														HASH_ALL, 
														(PTR*) &object);

				if (err == GSTAT_OK && object != NULL)
				{
					char		*tmp_name = object->name;
					object->name = NULL;

					err = WSSAppInit (object,id,name);
					if (err != GSTAT_OK)
					{
						MEfree(object->name);
						CLEAR_STATUS (DDFputhash(	applications, 
																			object->name, 
																			HASH_ALL, 
																			(PTR) object));
					}
					else
					{
						err = DDFputhash(	applications, 
															object->name, 
															HASH_ALL, 
															(PTR) object);
						MEfree(tmp_name);
					}
				}
			}

			if (err == GSTAT_OK) 
				err = DDGCommit(&wsf_session);
			else 
				CLEAR_STATUS(DDGRollback(&wsf_session));
		}
		CLEAR_STATUS( DDFSemClose(&wss_application));
	}
return(err);
}

/*
** Name: WSSAppDelete() - delete a application into the ICE system
**
** Description:
**	Update the repository and the memory
**
** Inputs:
**	u_nat* : application id
**
** Outputs:
**
** Returns:
**	GSTATUS	:	...
**				GSTAT_OK
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
WSSAppDelete (
	u_i4	id) 
{
	GSTATUS err = GSTAT_OK;

	err = DDFSemOpen(&wss_application, TRUE);
	if (err == GSTAT_OK) 
	{
		if (id < 1 || 
			 id >= wss_application_max ||
			 applications_id[id] == NULL)
			 err = DDFStatusAlloc(E_WS0089_WSS_BAD_APP);
		else
		{
			err = DDGDelete(
					&wsf_session, 
					WSS_OBJ_APPLICATION,
					"%d", 
					&id);

			if (err == GSTAT_OK)
			{
				WSS_APPLICATION *object = NULL;
				err = DDFdelhash(applications, 
													applications_id[id]->name, 
													HASH_ALL, 
													(PTR*) &object);

				if (err == GSTAT_OK && object != NULL) 
				{
					MEfree((PTR)object->name);
					applications_id[id] = NULL;
				}
			}

			if (err == GSTAT_OK) 
				err = DDGCommit(&wsf_session);
			else 
				CLEAR_STATUS(DDGRollback(&wsf_session));
		}
		CLEAR_STATUS( DDFSemClose(&wss_application));
	}
return(err);
}

/*
** Name: WSSAppExist() - Check if an application exist
**
** Description:
**
** Inputs:
**	char*			: application name
**
** Outputs:
**	bool*			: result
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
WSSAppExist (
	char *name,
	bool *result)
{
	GSTATUS err = GSTAT_OK;
	WSS_APPLICATION *object = NULL;

	err = DDFSemOpen(&wss_application, TRUE);
	if (err == GSTAT_OK) 
	{
		err = DDFgethash(applications, name, HASH_ALL, (PTR*) &object);
		CLEAR_STATUS( DDFSemClose(&wss_application));
	}
	*result = (object != NULL);
return(err);
}

/*
** Name: WSSAppBrowse() - list applications and thier information into variables
**
** Description:
**
** Inputs:
**	PTR*			: cursor
**
** Outputs:
**	u_nat**			: application id
**	u_nat**			: unit id
**	char**			: application name
**	char**			: application suffix
**	longnat**		: application flag
**	u_nat**			: application owner
**	u_nat**			: external location
**	char**			: external file
**	char**			: external suffix
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
WSSAppBrowse (
	PTR *cursor,
	u_i4	**id,
	char	**name) 
{
	GSTATUS err = GSTAT_OK;
	u_i4 pos = (u_i4)(SCALARP) *cursor;

	if (pos == 0)
		err = DDFSemOpen(&wss_application, TRUE);

	if (err == GSTAT_OK)
	{
		while (pos < wss_application_max &&
		   applications_id[pos] == NULL) 
		   pos++;

		if (pos < wss_application_max &&
				applications_id[pos] != NULL)
		{
			u_i4 length = sizeof(u_i4);
			err = GAlloc((PTR*)id, length, FALSE);
			if (err == GSTAT_OK)
				**id = pos;
			if (err == GSTAT_OK)
			{
				length = STlength(applications_id[pos]->name) + 1;
				err = GAlloc(name, length, FALSE);
				if (err == GSTAT_OK) 
					MECOPY_VAR_MACRO(applications_id[pos]->name, length, *name);
			}
			pos++;
		}
		else
		{
			pos = 0;
			CLEAR_STATUS( DDFSemClose(&wss_application));
		}
	}
	*cursor = (PTR)(SCALARP) pos;
return(err);
}


/*
** Name: WSSRetrieveApp() - Fetch the application
**
** Description:
**
** Inputs:
**	u_i4	: application id
**
** Outputs:
**	char**	: application name
**
** Returns:
**	GSTATUS	:	...
**				GSTAT_OK
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
WSSAppRetrieve (
	u_i4	id,
	char	**name) 
{
	GSTATUS err = GSTAT_OK;

	err = DDFSemOpen(&wss_application, FALSE);
	if (err == GSTAT_OK) 
	{
		if (id < 1 || 
			 id >= wss_application_max ||
			 applications_id[id] == NULL)
			 err = DDFStatusAlloc(E_WS0089_WSS_BAD_APP);
		else
		{
			u_i4 length = STlength(applications_id[id]->name) + 1;
			err = GAlloc(name, length, FALSE);
			if (err == GSTAT_OK) 
				MECOPY_VAR_MACRO(applications_id[id]->name, length, *name);
		}
		CLEAR_STATUS(DDFSemClose(&wss_application));
	}
return(err);
}
