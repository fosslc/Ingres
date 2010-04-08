/*
**Copyright (c) 2004 Ingres Corporation
*/
#include    <compat.h>
#include    <dbms.h>
#include    <me.h>
#include    <ulf.h>
#include    <ulm.h>

/*
**
**  Name: ULMCOPY.C - Handle large memory copies.
**
**  Description:
**	This routine is obsolete and should no longer be used.
**
**	This file contains functions to handle copying large amounts
**	of data from one memory location to another.
**
**  History:
**      13-oct-1993 (rog)
**          Created from code written by anitap in opcadf.c.
**      28-jan-1994 (rog)
**          Change 2 to 2L in MAXUI2 macro to force correct storage allocation.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/


/*
**  Forward and/or External function references.
*/

#define	MAXUI2		(2L * MAXI2 + 1)		/* Largest u_i2 */


/*{
** Name: ulm_copy	- Copy large amounts of memory.
**
** Description:
**
**	This routine is obsolete and should not be used.
**
**	This function copies large amounts of data from one memory location
**	to another by repeatedly calling MEcopy().  MEcopy() used to
**	only copy 64k bytes at a time, hence the need for this function.
**	We no longer port to 80286's and no current memcpy has a 64K limit.
**
** Inputs:
**	source				PTR to the memory to copy
**	nbytes				The number of bytes to copy
**
** Outputs:
**	dest				PTR to the memory to be written
**
** Returns:
**	None
**
** Exceptions:
**	None
**
** History:
**      13-oct-1993 (rog)
**          Created from code written by anitap in opcadf.c.
*/

VOID
ulm_copy( PTR source, i4 nbytes, PTR dest )
{
    MEcopy(source, nbytes, dest);
}
