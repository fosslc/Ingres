/******************************************************************************
**
** Copyright (c) 2004 Ingres Corporation
**
******************************************************************************/

#include <ddgexec.h>
#include <ddsmem.h>

/**
**
**  Name: ddscontrol.c - Data Dictionary Security Service Facility
**
**  Description:
**		This file contains all available functions to control accdds.
**
**  History:    
**	5-Feb-98 (marol01)
**	    created
**	13-may-1998 (canor01)
**	    Change GC_L_PASSWORD to DDS_PASSWORD_LENGTH.
**	    Replace gcn_encrypt with gcu_encode.
**      08-Jan-1999 (fanra01)
**          Move encode of password in DDSCheckUser into DDSMemCheckUser.
**      06-Aug-1999 (fanra01)
**          Change nat to i4, u_nat to u_i4, longnat to i4, u_longnat to u_i4
**      05-May-2000 (fanra01)
**          Bug 101346
**          Add DDSCheckRightRef function for testing access and reference
**          count.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	28-Jul-2004 (hanje04)
**	    Remove references to gcu_encode
**/

GLOBALREF SEMAPHORE dds_semaphore;
GLOBALREF DDG_SESSION dds_session;

/*
** Name: DDSRequestUser()- Compile user information for a late CheckRight call
**
** Description:
**
** Inputs:
**	u_nat		: user id
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
DDSRequestUser (
	u_i4 user_id) 
{
	GSTATUS err = GSTAT_OK;

	err = DDFSemOpen(&dds_semaphore, FALSE);
	if (err == GSTAT_OK) 
	{
		err = DDSMemRequestUser(user_id);
		CLEAR_STATUS( DDFSemClose(&dds_semaphore) );
	}
return(err);
}

/*
** Name: DDSCheckUser()	- Check accdds and find the corresponding id 
**
** Description:
**	
**
** Inputs:
**	char*		: user name
**	char*		: user password
**
** Outputs:
**	u_nat		: user id 
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
**      08-Jan-1999 (fanra01)
**          Move encode of password into DDSMemCheckUser.
*/

GSTATUS 
DDSCheckUser (
	char *name, 
	char *password, 
	u_i4 *user_id) 
{
	GSTATUS err = GSTAT_OK;

	err = DDFSemOpen(&dds_semaphore, FALSE);
	if (err == GSTAT_OK) 
	{
		err = DDSMemCheckUser(name, password, user_id);
		CLEAR_STATUS( DDFSemClose(&dds_semaphore) );
	}
return(err);
}

/*
** Name: DDSCheckUserFlag()	- Match parameter flag with user's flags
**
** Description:
**
** Inputs:
**	u_nat		: user id
**  u_nat		: flag
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
DDSCheckUserFlag (
	u_i4 user_id,
	u_i4 flag,
	bool *result)
{
	GSTATUS err = GSTAT_OK;

	err = DDFSemOpen(&dds_semaphore, FALSE);
	if (err == GSTAT_OK) 
	{
		err = DDSMemCheckUserFlag(user_id, flag, result);
		CLEAR_STATUS( DDFSemClose(&dds_semaphore) );
	}
return(err);
}

/*
** Name: DDSReleaseUser()	- Release a user previously requested
**
** Description:
**	
**
** Inputs:
**	u_nat		: user id
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
DDSReleaseUser (
	u_i4 user_id) 
{
	GSTATUS err = GSTAT_OK;

	err = DDFSemOpen(&dds_semaphore, FALSE);
	if (err == GSTAT_OK) 
	{
		err = DDSMemReleaseUser(user_id);
		CLEAR_STATUS( DDFSemClose(&dds_semaphore) );
	}
return(err);
}

/*
** Name: DDSCheckRight()	- Check right accdds
**
** Description:
**	
**
** Inputs:
**	u_nat		: user id
**	char*		: resource name
**	u_nat		: right level
**
** Outputs:
**	bool		: result
**
** Returns:
**	GSTATUS		: 
**				  E_OI0043_WSM_UNAUTH_ACCDDS
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
DDSCheckRight (
	u_i4	user, 
	char*	resource, 
	u_i4	right,
	bool	*result) 
{
	GSTATUS err = GSTAT_OK;

	err = DDFSemOpen(&dds_semaphore, FALSE);
	if (err == GSTAT_OK) 
	{
		err = DDSMemCheckRight(user, resource, right, result);
		CLEAR_STATUS( DDFSemClose(&dds_semaphore) );
	}
return(err);
}

/*
** Name: DDSCheckRightRef
**
** Description:
**      Function returns the result from the memory test for both rights and
**      reference count.
**
** Inputs:
**      user
**      resource
**      right
**
** Outputs:
**      ref
**      result
**
** Returns:
**      E_DF0042_DDS_MEM_NOT_INIT   users hashtable not initialised
**      E_DF0045_DDS_UNKNOWN_USER   user id exceeds the available range
**      GSTAT_OK                    function completed successfully.
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
DDSCheckRightRef(
    u_i4    user,
    char*   resource,
    i4      right,
    i4*     refs,
    bool    *result )
{
    GSTATUS err = GSTAT_OK;

    err = DDFSemOpen(&dds_semaphore, FALSE);
    if (err == GSTAT_OK)
    {
        err = DDSMemCheckRightRef(user, resource, right, refs, result);
        CLEAR_STATUS( DDFSemClose(&dds_semaphore) );
    }
    return(err);
}

/*
** Name: DDSDeleteResource()	- Delete all access to a resource
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
DDSDeleteResource(
	char*	resource) 
{
	GSTATUS err = GSTAT_OK;

	err = DDFSemOpen(&dds_semaphore, FALSE);
	if (err == GSTAT_OK) 
	{
		err = DDGExecute(&dds_session, DDS_DEL_RES_USER, "%s", resource);
		CLEAR_STATUS( DDGClose(&dds_session) );

		if (err == GSTAT_OK)
		{
			err = DDGExecute(&dds_session, DDS_DEL_RES_ROLE, "%s", resource);
			CLEAR_STATUS( DDGClose(&dds_session) );
		}

		if (err == GSTAT_OK)
			err = DDSMemDeleteResource(resource);

		if (err == GSTAT_OK)
			err = DDGCommit(&dds_session);
		else
			CLEAR_STATUS(DDGRollback(&dds_session));

		CLEAR_STATUS( DDFSemClose(&dds_semaphore) );
	}
return(err);
}
