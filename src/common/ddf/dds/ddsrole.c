/******************************************************************************
**
** Copyright (c) 2004 Ingres Corporation
**
******************************************************************************/

#include <ddgexec.h>
#include <ddsmem.h>

/**
**
**  Name: ddsrole.c - Data Dictionary Services Facility Security Service
**
**  Description:
**		This file contains all available functions to manage roles.
**
**  History:    
**	5-Feb-98 (marol01)
**	    created
**      18-Jan-1999 (fanra01)
**          Cleaned up compiler warning on unix.
**      06-Aug-1999 (fanra01)
**          Change nat to i4, u_nat to u_i4, longnat to i4, u_longnat to u_i4
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	11-oct-2002 (somsa01)
**	    Changed DDGInsert(), DDGUpdate(), DDGKeySelect(), DDGExecute()
**	    and DDGDelete() such that we pass the addresses of int and float
**	    arguments.
**/

GLOBALREF SEMAPHORE dds_semaphore;
GLOBALREF DDG_SESSION dds_session;

/*
** Name: DDSCreateRole() - role creation in the Data Dictionary and in memory
**
** Description:
**	
**
** Inputs:
**	char*		: user name
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
*/

GSTATUS 
DDSCreateRole (
	char *name, 
	char *comment, 
	u_i4 *id) 
{
	GSTATUS err = GSTAT_OK;

	err = DDFSemOpen(&dds_semaphore, TRUE);
	if (err == GSTAT_OK) 
	{
		err = DDSPerformRoleId (id);
		if (err == GSTAT_OK)
		{
			err = DDGInsert(
					&dds_session, 
					DDS_OBJ_ROLE, 
					"%d%s%s", 
					id,
					name, 
					comment);

			if (err == GSTAT_OK)
				err = DDSMemCreateRole(*id, name);

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
** Name: DDSUpdateRole()	- role update in the Data Dictionary and in memory
**
** Description:
**	
**
** Inputs:
**	u_nat		: role id
**	char*		: role name
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
DDSUpdateRole (
	u_i4 id, 
	char *name, 
	char *comment) 
{
	GSTATUS err = GSTAT_OK;
	err = DDFSemOpen(&dds_semaphore, TRUE);
	if (err == GSTAT_OK) 
	{
		err = DDGUpdate(
				&dds_session, 
				DDS_OBJ_ROLE, 
				"%s%s%d", 
				name, 
				comment, 
				&id);

		if (err == GSTAT_OK)
			err = DDSMemUpdateRole(id, name);

		if (err == GSTAT_OK) 
			err = DDGCommit(&dds_session);
		else 
			CLEAR_STATUS(DDGRollback(&dds_session));

		CLEAR_STATUS(DDFSemClose(&dds_semaphore));
	}
return(err);
}

/*
** Name: DDSDeleteRole()	- role delete in the Data Dictionary and in memory
**
** Description:
**	
**
** Inputs:
**	u_nat		: role id
**	char*		: role name
**	char*		: comment
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
DDSDeleteRole (
	u_i4 id) 
{
	GSTATUS err = GSTAT_OK;
	err = DDFSemOpen(&dds_semaphore, TRUE);
	if (err == GSTAT_OK) 
	{
		err = DDGExecute(&dds_session, DDS_DEL_ROLE_RIGHT, "%d", &id);
		CLEAR_STATUS( DDGClose(&dds_session) );

		if (err == GSTAT_OK)
			err = DDGDelete(&dds_session, DDS_OBJ_ROLE, "%d", &id);
		if (err == GSTAT_OK) 
			err = DDSMemDeleteRole(id);

		if (err == GSTAT_OK) 
			err = DDGCommit(&dds_session);
		else 
			CLEAR_STATUS(DDGRollback(&dds_session));

		CLEAR_STATUS(DDFSemClose(&dds_semaphore));
	}
return(err);
}

/*
** Name: DDSCreateAssUserRole()	- create relation between a user and a role
**
** Description:
**	
**
** Inputs:
**	u_nat		: user id
**	u_nat		: role id
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
DDSCreateAssUserRole (
	u_i4 user_id, 
	u_i4 role_id) 
{
	GSTATUS err = GSTAT_OK;

	err = DDFSemOpen(&dds_semaphore, TRUE);
	if (err == GSTAT_OK) 
	{
		err = DDSMemCheckAssUserRole(user_id, role_id);
		if (err == GSTAT_OK)
		{
			err = DDGInsert(
					&dds_session, 
					DDS_OBJ_ASS_USER_ROLE, 
					"%d%d", 
					&user_id, 
					&role_id);

			if (err == GSTAT_OK)
				err = DDSMemCreateAssUserRole(user_id, role_id);

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
** Name: DDSDeleteAssUserRole()	- delete relation between a user and a role
**
** Description:
**	
**
** Inputs:
**	u_nat		: user id
**	u_nat		: role id
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
DDSDeleteAssUserRole (
	u_i4 user_id, 
	u_i4 role_id) 
{
	GSTATUS err = GSTAT_OK;

	err = DDFSemOpen(&dds_semaphore, TRUE);
	if (err == GSTAT_OK) 
	{
		err = DDGDelete(
				&dds_session, 
				DDS_OBJ_ASS_USER_ROLE, 
				"%d%d", 
				&user_id, 
				&role_id);

		if (err == GSTAT_OK)
			err = DDSMemDeleteAssUserRole(user_id, role_id);

		if (err == GSTAT_OK) 
			err = DDGCommit(&dds_session);
		else 
			CLEAR_STATUS(DDGRollback(&dds_session));

		CLEAR_STATUS(DDFSemClose(&dds_semaphore));
	}
return(err);
}

/*
** Name: DDSAssignRoleRight() - create an accdds right between a role 
**								and a resource
**
** Description:
**	
**
** Inputs:
**	u_nat		: role id
**	char*		: resource name
**	u_nat		: right number
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
DDSAssignRoleRight (
	u_i4 role, 
	char* resource, 
	u_i4 right) 
{
	GSTATUS err = GSTAT_OK;
	bool	created;
	i4 tmp = right;

	err = DDFSemOpen(&dds_semaphore, TRUE);
	if (err == GSTAT_OK) 
	{
		err = DDSMemAssignRoleRight(role, resource, &tmp, &created);
		if (err == GSTAT_OK)
			if (created == TRUE)
				err = DDGInsert(
						&dds_session, 
						DDS_OBJ_ROLE_RIGHT, 
						"%d%s%d", 
						&role, 
						resource, 
						&tmp);
			else
				err = DDGUpdate(
						&dds_session, 
						DDS_OBJ_ROLE_RIGHT, 
						"%d%d%s", 
						&tmp,
						&role, 
						resource);

		if (err == GSTAT_OK) 
			err = DDGCommit(&dds_session);
		else 
		{
			CLEAR_STATUS(DDGRollback(&dds_session));
			CLEAR_STATUS(DDSMemDeleteRoleRight(role, resource, (i4*)&right, &created));
		}
		CLEAR_STATUS(DDFSemClose(&dds_semaphore));
	}
return(err);
}

/*
** Name: DDSDeleteRoleRight() - delete an access right between a role 
**								and a resource
**
** Description:
**	
**
** Inputs:
**	u_nat		: role id
**	char*		: resource name
**	u_nat		: right number
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
DDSDeleteRoleRight (
	u_i4 role, 
	char* resource, 
	u_i4 right) 
{
	GSTATUS err = GSTAT_OK;
	bool	deleted;
	i4 tmp = right;

	err = DDFSemOpen(&dds_semaphore, TRUE);
	if (err == GSTAT_OK) 
	{
		err = DDSMemDeleteRoleRight(role, resource, &tmp, &deleted);
		if (err == GSTAT_OK)
			if (deleted == TRUE)
				err = DDGDelete(
						&dds_session, 
						DDS_OBJ_ROLE_RIGHT, 
						"%d%s", 
						&role,
						resource);
			else
				err = DDGUpdate(
						&dds_session, 
						DDS_OBJ_ROLE_RIGHT, 
						"%d%d%s", 
						&tmp,
						&role, 
						resource);

		if (err == GSTAT_OK) 
			err = DDGCommit(&dds_session);
		else 
		{
			CLEAR_STATUS(DDGRollback(&dds_session));
			CLEAR_STATUS(DDSMemAssignRoleRight(role, resource, (i4*)&right, &deleted));
		}
		CLEAR_STATUS(DDFSemClose(&dds_semaphore));
	}
return(err);
}

/*
** Name: DDSOpenRole()	- Open a cursor on the Role list
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
DDSOpenRole (PTR		*cursor) 
{
	GSTATUS err = GSTAT_OK;

	err = DDFSemOpen(&dds_semaphore, TRUE);
	if (err == GSTAT_OK) 
		err = DDGSelectAll(&dds_session, DDS_OBJ_ROLE);
return(err);
}

/*
** Name: DDSRetrieveRole()	- Select a unique Role
**
** Description:
**	
**
** Inputs:
**	u_nat : Role id
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
DDSRetrieveRole (
	u_i4	id, 
	char	**name, 
	char	**comment) 
{
	GSTATUS err = GSTAT_OK;

	err = DDFSemOpen(&dds_semaphore, TRUE);
	if (err == GSTAT_OK) 
	{
		err = DDGKeySelect(
				&dds_session, 
				DDS_OBJ_ROLE, 
				"%d%s%s", 
				&id,
				name, 
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
** Name: DDSGetRole()	- get the current Role
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
DDSGetRole (
	PTR		*cursor,
	u_i4	**id, 
	char	**name, 
	char	**comment) 
{
	GSTATUS err = GSTAT_OK;
	bool	exist;

	err = DDGNext(&dds_session, &exist);
	if (err == GSTAT_OK && exist == TRUE)
			err = DDGProperties(
	   				&dds_session, 
					"%d%s%s", 
					id,
					name, 
					comment);
	if (err != GSTAT_OK)
		exist = FALSE;
	*cursor = (char*) (SCALARP)exist;
return(err);
}

/*
** Name: DDSCloseRole()	- close the cursor on the Role list
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
DDSCloseRole (PTR		*cursor) 
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
** Name: DDSOpenUserRole()	- Open a cursor on the User/Role relation
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
DDSOpenUserRole (PTR *cursor) 
{
return(DDFSemOpen(&dds_semaphore, FALSE));
}

/*
** Name: DDSGetUserRole()	- 
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
DDSGetUserRole (
	PTR		*cursor,
	u_i4	user_id, 
	u_i4	**role_id,
	char	**role_name) 
{
return(DDSMemSelectAssUserRole(cursor, user_id, role_id, role_name));
}

/*
** Name: DDSCloseUserRole()	- close the cursor on the Role list
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
DDSCloseUserRole (PTR *cursor) 
{
	CLEAR_STATUS(DDFSemClose(&dds_semaphore));
return(GSTAT_OK);
}

/*
** Name: DDSOpenRoleResource()	- Open a cursor on the Resource/Role relation
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
DDSOpenRoleResource (PTR *cursor, char* resource) 
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
** Name: DDSGetRoleResource()	- 
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
DDSGetRoleResource (
	PTR		*cursor,
	u_i4	*id,
	i4 *right) 
{
	GSTATUS err = GSTAT_OK;
	DDS_SCANNER *scan = (DDS_SCANNER*) *cursor;
	err = DDSMemSelectResource (&scan->current, RIGHT_TYPE_ROLE, scan->name, id, right);
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
DDSCloseRoleResource (PTR *cursor) 
{
	CLEAR_STATUS(DDFSemClose(&dds_semaphore));
return(GSTAT_OK);
}

/*
** Name: DDSGetRoleName()	- return the role name
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
DDSGetRoleName(
	u_i4	id, 
	char** name)
{
	GSTATUS err = GSTAT_OK;

	err = DDFSemOpen(&dds_semaphore, FALSE);
	if (err == GSTAT_OK) 
	{
		err = DDSMemGetRoleName(id, name);
		CLEAR_STATUS(DDFSemClose(&dds_semaphore));
	}
return(err);
}
