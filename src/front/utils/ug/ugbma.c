/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<ug.h>

/**
** Name:    ugbadmem.c -	Front-End Utility Memory Allocation Failure
**					Error Routine.
** Description:
**	Contains the routine that reports a memory allocation failure and
**	terminates program execution.  Defines:
**
**	IIUGbmaBadMemoryAllocation	report memory allocation failure.
**
** History:
**	5-jan-1987 (peter)	Taken from runtime.
**	5-aug-1987 (peter)	Add declaration and name.
**/


/*{
** Name:    IIUGbmaBadMemoryAllocation() -     Report Memory Allocation Failure.
**
** Description:
**	This routine writes out a standard syserr on a bad memory allocation
**	and exits.
**
** Inputs:
**	char	*routine_name		Name of routine in which err occurs.
**
** Returns:
**	does not return.
**
** History:
**	5-jan-1987 (peter)	Written.
**	5-aug-1987 (peter)	Add call to IIUGerr.
*/
VOID
IIUGbmaBadMemoryAllocation (routine)
char	*routine;
{
    IIUGerr(E_UG0009_Bad_Memory_Allocation, UG_ERR_FATAL, 1, routine);
}

/* The following is here for shortterm compatibility and should
** be removed. */
VOID
iiugbma_BadMemoryAllocation()
{
    IIUGbmaBadMemoryAllocation(ERx("unknown"));
}
