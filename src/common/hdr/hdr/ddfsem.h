/******************************************************************************
**
** Copyright (c) 2004 Ingres Corporation
**
******************************************************************************/

/*
** Name: ddfsem.h - Data Dictionary Facility. Semaphore Management
**
** Description:
**  This file contains the Semaphore API used by DDF.
**	This semaphore API offers the capability to have SHARED and EXCLUSIVE 
**	Semaphore on every plateform
**
** History: 
**      02-mar-98 (marol01)
**          Created
**	29-Jul-1998 (fanra01)
**	    Corrected syntax errors.
**	12-Mar-1999 (schte01)
**       Moved #defines to column 1.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

#ifndef ICESEM_INCLUDED
#define ICESEM_INCLUDED

#include <ddfcom.h>

#ifdef NT_GENERIC
	typedef struct __SEMAPHORE 
	{
      CS_SEMAPHORE	semaphore;
	  CS_CONDITION	cond;
      i4			have;
	} SEMAPHORE, *PSEMAPHORE;

	GSTATUS 
	DDFSemCreate (
		PSEMAPHORE	sem, 
		char		*name);

	GSTATUS 
	DDFSemOpen (
		PSEMAPHORE	sem, 
		bool		exclusive);

	GSTATUS 
	DDFSemClose (
		PSEMAPHORE sem);

	GSTATUS 
	DDFSemDestroy (
		PSEMAPHORE sem);

#else
#define SEMAPHORE CS_SEMAPHORE
#define PSEMAPHORE *CS_SEMAPHORE

#define DDFSemCreate(X,Y) (CSw_semaphore(X, CS_SEM_SINGLE, Y) == OK)	?\
								 GSTAT_OK : \
								 DDFStatusAlloc ( E_DF0007_SEM_CANNOT_CREATE) 

#define DDFSemOpen(X, Y)	(CSp_semaphore(Y, X) == OK)					? \
								 GSTAT_OK : \
								 DDFStatusAlloc( E_DF0008_SEM_CANNOT_OPEN)

#define DDFSemClose(X)		(CSv_semaphore(X) == OK)					? \
								 GSTAT_OK : \
								 DDFStatusAlloc( E_DF0009_SEM_CANNOT_CLOSE)

#define DDFSemDestroy(X)	(CSr_semaphore(X) == OK)					?  (GSTATUS)GSTAT_OK : (GSTATUS)GSTAT_OK
#endif

#endif
