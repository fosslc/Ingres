/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:    osmem.h -	OSL Parser Memory Module.
**
** Description:
**	Contains the interface definition for the memory module of the parser.
**	This module starts and ends memory allocation for the OSL parser.
**
**	osMem()		start memory allocation.
**	osAlloc()	allocate block of memory.
**	osCalloc()	allocate cleared block of memory.
**	osFree()	free all memory.
**
** History:
**	Revision 6.0  88/03  wong
**	Initial revision.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

extern u_i4	osmem;		/* memory tag */

/*{
** Name:	osMem() -	Start Memory Allocation.
**
** Description:
**	Starts memory allocation by allocating a tag.
**
** History:
**	03/88 (jhw) -- Written.
*/
#define	osMem()	{osmem = FEgettag();}

/*{
** Name:	osAlloc() -	Allocate Block of Memory.
**
** Description:
**	Allocates a block of memory.
**
** History:
**	03/88 (jhw) -- Written.
*/
#define	osAlloc(size, status)	FEreqmem(osmem, (size), FALSE, (status))

/*{
** Name:	osCalloc() -	Allocate Cleared Block of Memory.
**
** Description:
**	Allocates a cleared block of memory.
**
** History:
**	03/88 (jhw) -- Written.
*/
#define	osCalloc(size, status)	FEreqmem(osmem, (size), TRUE, (status))

/*{
** Name:	osFree() -	Free All Memory.
**
** Description:
**	Ends memory allocation by freeing all memory allocated directly
**	by the OSL parsers.
**
** History:
**	03/88 (jhw) -- Written.
*/
#define	osFree()	{FEfree(osmem);}
