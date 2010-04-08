/*
** Copyright (c) 1990, Ingres Corporation 
*/
# include	<compat.h>
#include    <gl.h>
# include	<me.h>

/*
**  meactual.c
**
**  History
**
**	4/91 (Mike S)
**		Keep ME statistics; these can be used for debugging.
**	20-nov-92 (pearl)
**		Add #ifdef for "CL_PROTOTYPED" and prototyped function
**		headers.
**      8-jun-93 (ed)
**              changed to use CL_PROTOTYPED
**      16-jul-93 (ed)
**	    added gl.h
**	11-aug-93 (ed)
**	    unconditional prototypes
**	16-oct-01 (kinte01)
**	    meactual & meuser should be SIZE_TYPE
**	25-oct-2001 (somsa01)
**	    The return type of MEactual() is void.
*/
GLOBALDEF SIZE_TYPE	i_meactual = 0;
GLOBALDEF SIZE_TYPE	i_meuser = 0;

void
MEactual(
	SIZE_TYPE	*user,
	SIZE_TYPE	*actual)
{
    /* 
    ** On VMS we don't distinguish how much the user has asked for 
    ** from how much ME allocated.
    */
    *user = i_meactual;
    *actual = i_meactual;
}
