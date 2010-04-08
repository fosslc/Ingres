/******************************************************************************
**
** Copyright (c) 1998, 2004 Ingres Corporation
**
******************************************************************************/

#include <ddfsem.h>
#include <erddf.h>

/**
**
**  Name: ddfsem.c - Data Dictionary Services Facility Semaphore management
**
**  Description:
**	 The DDF module has to use shared ans exclusive semaphore.
**	 This file is composed of semaphore management procedure based on CL for NT
**	 Because CL on NT only provide exclusive semphore
**
**  History:    
**	5-Feb-98 (marol01)
**	    created
**	29-Jul-1998 (fanra01)
**	    ifdef'ed to build on unix
**	22-jan-2004 (somsa01)
**	    Updated parameters for CScnd_signal().
**/

# ifdef NT_GENERIC

/*
** Name: DDFSemCreate()	- Create and initialize an DDF semaphore
**
** Description:
**
** Inputs:
**	PSEMAPHORE	: semaphore pointer
**	char*		: semaphore name
**
** Outputs:
**	none
**
** Returns:
**	GSTATUS		:	E_DF0007_SEM_CANNOT_CREATE
**					GSTAT_OK
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
DDFSemCreate (
	PSEMAPHORE sem, 
	char *name) 
{
    GSTATUS err = GSTAT_OK;
    STATUS	status;

    G_ASSERT(!sem, 1);

    status = CSw_semaphore(&(sem->semaphore), CS_SEM_SINGLE, name);
    if (status == OK)
	status = CScnd_init(&sem->cond);
    if (status == OK)
	status = CScnd_name(&sem->cond, "DDF R/W semaphore");
    sem->have = 0;
    if (status != OK) 
	err = DDFStatusAlloc (E_DF0007_SEM_CANNOT_CREATE);
    return(err);
}

/*
** Name: DDFSemOpen()	- Open an DDF semaphore
**
** Description:
**
** Inputs:
**	PSEMAPHORE	: semaphore pointer
**	bool		: TRUE means exclusive, FALSE means shared
**
** Outputs:
**	none
**
** Returns:
**	GSTATUS		:	E_DF0008_SEM_CANNOT_OPEN
**					GSTAT_OK
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
DDFSemOpen (
	PSEMAPHORE sem, 
	bool exclusive) 
{
    GSTATUS err = GSTAT_OK;
    STATUS	status;

    if ((status = CSp_semaphore(TRUE, &sem->semaphore)) == OK)
    {
	while (status == OK && 
	       ((exclusive == TRUE) ? sem->have != 0 : sem->have < 0))
	    status = CScnd_wait(&sem->cond, &sem->semaphore);

	if (status == OK) 
	{
	    if (exclusive == TRUE)
		sem->have--;
	    else
		sem->have++;
	    status = CSv_semaphore(&sem->semaphore);
	}
	else if (status != E_CS000F_REQUEST_ABORTED)
	{
	    CSv_semaphore(&sem->semaphore);
	    err = DDFStatusAlloc (E_DF0008_SEM_CANNOT_OPEN);
	}
    }
    return(err);
}

/*
** Name: DDFSemClose()	- Close an DDF semaphore
**
** Description:
**
** Inputs:
**	PSEMAPHORE	: semaphore pointer
**
** Outputs:
**	none
**
** Returns:
**	GSTATUS		:	E_DF0010_SEM_BAD_INIT
**					E_DF0009_SEM_CANNOT_CLOSE
**					GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
**	22-jan-2004 (somsa01)
**	    Added (CS_SID)NULL to CScnd_signal prototype.
*/

GSTATUS 
DDFSemClose (
	PSEMAPHORE sem) 
{
    GSTATUS err = GSTAT_OK;
    STATUS	status;

    G_ASSERT(!sem, E_DF0010_SEM_BAD_INIT);

    if ((status = CSp_semaphore(TRUE, &sem->semaphore)) == OK) 
    {
	if (sem->have < 0) 
	{
	    sem->have = 0;
	    /* when releaseing an exclusive - several shares may wake */
	    status = CScnd_broadcast(&sem->cond);
	} 
	else 
	{
	    sem->have--;
	    if (sem->have == 0) 
	    {
		/* when releaseing a share - at most one exclusive may wake */
		status = CScnd_signal(&sem->cond, (CS_SID)NULL);
	    }
	}
	if (status == OK) 
	{
	    if (CSv_semaphore(&sem->semaphore) != OK)
		err = DDFStatusAlloc (E_DF0009_SEM_CANNOT_CLOSE);
	} 
	else 
	{
	    CSv_semaphore(&sem->semaphore);
	}
    }
    return(err);
}

/*
** Name: DDFSemDestroy()	- Destroy an DDF semaphore
**
** Description:
**
** Inputs:
**	PSEMAPHORE	: semaphore pointer
**
** Outputs:
**	none
**
** Returns:
**	GSTATUS		:	E_DF0010_SEM_BAD_INIT
**					E_DF0009_SEM_CANNOT_CLOSE
**					GSTAT_OK
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
DDFSemDestroy (
	PSEMAPHORE sem) 
{
    GSTATUS err = GSTAT_OK;
    CScnd_free(&sem->cond);
    CSr_semaphore(&sem->semaphore);
    return(err);
}

# endif /* NT_GENERIC */
