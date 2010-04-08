/*
**
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<si.h> 
# include	<er.h> 
# include	<rcons.h> 

/*{
** Name:	get_space() -	Allocate an Empty Block of Memory.
**
** Description:
**   GET_SPACE - Allocates a zeroed block of memory which was dynamically
**	allocated by FEcalloc.
**
** Parameters:
**	nelements - number of elements.
**	size - size of one element.
**
** Returns:
**	Pointer (i4) to allocated space of size nelements*size.
**	A null pointer will be returned if an error occurs, e.g., no space
**	available.
**
** History:
**	3/10/81 (ps) - written.
**	9/1/83 (gac)	increased BUFSIZE to 2048.
**	11/15/85 (rlk) - Rewrote using FEcalloc.
**	08-dec-88 (sylviap)
**		Changed memory allocation to use FEreqmem.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

i4  *
get_space (nelements, size)
i2	nelements;
i2	size;
{
	i4	*block;

	return (block = (i4 *)FEreqmem((u_i4)0, (u_i4)(nelements * size), 
		TRUE, (STATUS *)NULL));
}
