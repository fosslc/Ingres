/******************************************************************************
**
**Copyright (c) 2004 Ingres Corporation
**
******************************************************************************/

#include <wsfvar.h>
#include <ddfsem.h>
#include <ddgexec.h>

/*
**
**  Name: wsfvar.c - System variable
**
**  Description:
**		Define what is a variable
**		provide functions to manage the system hashtable
**	
**  History:    
**	5-Feb-98 (marol01)
**	    created
**      10-Sep-1998 (fanra01)
**          Fixup compiler warnings on unix.
**      02-Oct-1998 (fanra01)
**          Use local buffer pointer to prevent heap corruption.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

GLOBALREF DDG_SESSION	wsf_session;	/* repository connection */
GLOBALDEF DDFHASHTABLE system_var = NULL;
static SEMAPHORE wsfvar_semaphore;

/*
** Name: WSFVARInitialize() - Initialization of the WSFVar module
**
** Description:
**	Intialize variables and semaphore.
**	Load repository information into the memory.
**
** Inputs:
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
**      02-Oct-1998 (fanra01)
**          Modified to prevent heap corruption by freeing invalid pointer.
*/

GSTATUS 
WSFVARInitialize ()
{
	GSTATUS err = GSTAT_OK;

	bool			exist = FALSE;
	char			*hostname = NULL;
	char			*name = NULL;
	char*			value = NULL;
	u_i4			size = 50;
	char			szConfig[CONF_STR_SIZE];
	WSF_VAR		*var = NULL;

	err = DDFSemCreate(&wsfvar_semaphore, "WCS");
	if (err == GSTAT_OK)
	{
        char *buffer;
		STprintf (szConfig, CONF_SVRVAR_SIZE, hostname);
		if (err == GSTAT_OK && PMget( szConfig, &buffer ) == OK && buffer != NULL)
			CVan(buffer, (i4*)&size);

		err = DDFmkhash(size, &system_var);
	}

	if (err == GSTAT_OK)
	{
		/* loading information about Server variable */
		err = DDGSelectAll(&wsf_session, WSS_OBJ_SYSTEM_VAR);
		while (err == GSTAT_OK) 
		{
			err = DDGNext(&wsf_session, &exist);
			if (err != GSTAT_OK || exist == FALSE)
				break;
			err = DDGProperties(&wsf_session, "%s%s", &name, &value);
			if (err == GSTAT_OK)
			{
				err = WSFCreateVariable(0, name, STlength(name), value, STlength(value), &var);
				if (err == GSTAT_OK && var != NULL) 
					err = DDFputhash(system_var, var->name, STlength(var->name), (PTR) var);
			}
		}
		CLEAR_STATUS(DDGClose(&wsf_session))
	}
	
	if (err == GSTAT_OK)
		err = DDGCommit(&wsf_session);
	else
		CLEAR_STATUS(DDGRollback(&wsf_session));

	MEfree((PTR)name);
	MEfree((PTR)value);
return(err);
}

/*
** Name: WSFVARTerminate() - Terminate of the WSF Var module
**
** Description:
**	Frees memory and destroys semphore.
**
** Inputs:
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
WSFVARTerminate() 
{
	GSTATUS err = GSTAT_OK;

	CLEAR_STATUS(WSFCleanVariable(system_var));
	CLEAR_STATUS(DDFSemDestroy(&wsfvar_semaphore) );
return(err);
}

/*
** Name: WSFDestroyVariable() - destroy a variable
**
** Description:
**	clean the memory
**
** Inputs:
**	WSF_PVAR *		: the variable
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
WSFDestroyVariable (
	WSF_PVAR *var) 
{
	WSF_PVAR tmp = *var;

	*var = NULL;
	if (tmp != NULL)
	{
		MEfree((PTR)tmp->value);
		MEfree((PTR)tmp->name);
	}
	MEfree((PTR)tmp);
return(GSTAT_OK);
}

/*
** Name: WSFCreateVariable() - create a variable
**
** Description:
**	allocate the memory
**
** Inputs:
**	u_i2	: memory tag
**	char*	: variable name
**	u_nat*	: length of the variable name 
**	char*	: varaible value
**	u_i4	: length of the variable value
**
** Outputs:
**	WSF_PVAR *		: the variable
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
WSFCreateVariable (
	u_i2 tag,
	char *name, 
	u_i4 nlen, 
	char *value, 
	u_i4 vlen,
	WSF_PVAR *var) 
{
	GSTATUS err = GSTAT_OK;
	WSF_VAR *tmp = NULL;

	err = G_ME_REQ_MEM(tag, tmp, WSF_VAR, 1);
	if (err == GSTAT_OK) 
	{
		err = G_ME_REQ_MEM(tag, tmp->name, char, nlen+1);
		if (err == GSTAT_OK) 
		{
			MECOPY_VAR_MACRO(name, nlen, tmp->name);
			tmp->name[nlen] = EOS;
			if (value != NULL)
			{
				err = G_ME_REQ_MEM(tag, tmp->value, char, vlen+1);
				if (err == GSTAT_OK) 
				{
					MECOPY_VAR_MACRO(value, vlen, tmp->value);
					tmp->value[vlen] = EOS;
				}
			}
		}
	}
	if (err == GSTAT_OK)
		*var = tmp;
	else
	{
		if (tmp != NULL)
		{
			MEfree((PTR)tmp->value);
			MEfree((PTR)tmp->name);
		}
		MEfree((PTR)tmp);
	}
return(err);
}

/*
** Name: WSFAddVariable() - add a variable into the system level hash table
**
** Description:
**	allocate the memory
**
** Inputs:
**	char*	: variable name
**	u_nat*	: length of the variable name 
**	char*	: variable value
**	u_i4	: length of the variable value
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
WSFAddVariable (
	char *name, 
	u_i4 nlen, 
	char *value, 
	u_i4 vlen) 
{
	GSTATUS err = GSTAT_OK;
	WSF_VAR *var = NULL;

	if (err == GSTAT_OK)
		err = DDFSemOpen(&wsfvar_semaphore, TRUE);

	if (err == GSTAT_OK)
	{
		err = DDFgethash(system_var, name, nlen, (PTR*) &var);

		if (err == GSTAT_OK && var != NULL)
		{
			char* tmp = NULL;

			if (value != NULL && vlen > 0)
			{
				err = G_ME_REQ_MEM(0, tmp, char, vlen+1);
				if (err == GSTAT_OK) 
				{
					MECOPY_VAR_MACRO(value, vlen, tmp);
					tmp[vlen] = EOS;
				}
			}

			if (err == GSTAT_OK)
			{
				err = DDGUpdate(
							&wsf_session, 
							WSS_OBJ_SYSTEM_VAR, 
							"%s%s", 
							tmp,
							var->name);

				if (err == GSTAT_OK) 
				{
					err = DDGCommit(&wsf_session);
					var->value = tmp;					
				}
				else
				{
					MEfree(tmp);
					CLEAR_STATUS(DDGRollback(&wsf_session));
				}
			}		
		}
		else
		{
			err = WSFCreateVariable(0, name, nlen, value, vlen, &var);
			if (err == GSTAT_OK && var != NULL) 
			{
				err = DDGInsert(
							&wsf_session, 
							WSS_OBJ_SYSTEM_VAR, 
							"%s%s", 
							var->name,
							var->value);

				if (err == GSTAT_OK) 
					err = DDFputhash(system_var, var->name, STlength(var->name), (PTR) var);

				if (err == GSTAT_OK) 
					err = DDGCommit(&wsf_session);
				else
				{
					CLEAR_STATUS(DDGRollback(&wsf_session));
					CLEAR_STATUS(WSFDestroyVariable(&var));
				}
			}
		}
		DDFSemClose(&wsfvar_semaphore);
	}
return(err);
}

/*
** Name: WSFGetVariable() - retrieve a variable from the system level hash table
**							and return the value
**
** Description:
**
** Inputs:
**	char*	: variable name
**	u_nat*	: length of the variable name 
**
** Outputs:
**	char**	: variable value
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
WSFGetVariable (	
	char *name, 
	u_i4 nlen, 
	char **value) 
{
	GSTATUS err = GSTAT_OK;
	struct __WSF_VAR *var = NULL;

	*value = NULL;

	if (system_var != NULL) 
	{
		err = DDFSemOpen(&wsfvar_semaphore, TRUE);
		if (err == GSTAT_OK)
		{
			err = DDFgethash(system_var, name, nlen, (PTR*) &var);

			if (var != NULL)
				*value = var->value;
			DDFSemClose(&wsfvar_semaphore);
		}
	}
return(err);
}

/*
** Name: WSFDelVariable() - remove a variable from the system level hash table
**
** Description:
**
** Inputs:
**	char*	: variable name
**	u_nat*	: length of the variable name 
**
** Outputs:
**	char**	: variable value
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
WSFDelVariable (	
	char *name, 
	u_i4 nlen) 
{
	GSTATUS err = GSTAT_OK;
	struct __WSF_VAR *var = NULL;

	if (system_var != NULL) 
	{
		err = DDFSemOpen(&wsfvar_semaphore, TRUE);
		if (err == GSTAT_OK)
		{
			err = DDFdelhash(system_var, name, nlen, (PTR*) &var);

			if (err == GSTAT_OK && var != NULL) 
			{
				err = DDGDelete(
							&wsf_session, 
							WSS_OBJ_SYSTEM_VAR, 
							"%s", 
							var->name);

				if (err == GSTAT_OK) 
				{
					CLEAR_STATUS( WSFDestroyVariable(&var) );
					err = DDGCommit(&wsf_session);
				}
				else
				{
					CLEAR_STATUS(DDFputhash(system_var, name, nlen, (PTR) var));
					CLEAR_STATUS(DDGRollback(&wsf_session));
				}
			}
			DDFSemClose(&wsfvar_semaphore);
		}
	}
return(err);
}

/*
** Name: WSFCleanVariable() - remove a variable from the system level hash table
**
** Description:
**
** Inputs:
**	DDFHASHTABLE vars
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
WSFCleanVariable (	
	DDFHASHTABLE vars) 
{
	GSTATUS err = GSTAT_OK;
	
	if (vars != NULL)
	{
		DDFHASHSCAN sc;
		WSF_VAR *var = NULL;

		DDF_INIT_SCAN(sc, vars);
		while (DDF_SCAN(sc, var) == TRUE && var != NULL)
		{
			CLEAR_STATUS(WSFDestroyVariable(&var));
		}
	}
return(err);
}
