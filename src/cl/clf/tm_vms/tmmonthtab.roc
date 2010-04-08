/*
**    Copyright (c) 1987, Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# ifdef UNIX
# include	<systypes.h>
# endif
# include	"tmlocal.h"

/**
** Name: TMMONTH.C - TM global variables.
**
** Description:
**      This file contains global variables used by tmparse.c
**    
** History:
Revision 1.1  88/08/05  13:46:52  roger
UNIX 6.0 Compatibility Library as of 12:23 p.m. August 5, 1988.

**      20-jul-87 (mmm)
**          Updated to meet jupiter coding standards.      
**	28-Feb-1989 (fredv)
**	    Include <systypes.h>.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/* externs */

GLOBALDEF i4			TM_Dmsize[12] = {31, 28, 31, 30, 31, 30, 
						 31, 31, 30, 31, 30, 31};


GLOBALDEF TM_MONTHTAB 		TM_Monthtab[] =
{
	"jan",		1,
	"feb",		2,
	"mar",		3,
	"apr",		4,
	"may",		5,
	"jun",		6,
	"jul",		7,
	"aug",		8,
	"sep",		9,
	"oct",		10,
	"nov",		11,
	"dec",		12,
	0
};
