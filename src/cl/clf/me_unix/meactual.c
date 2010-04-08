/*
** Copyright (c) 1993, 2001 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<clconfig.h>
# include	<meprivate.h>
# include	<me.h>

/*
** Name: meactual.c
**
** History
**	18-jan-1993 (pearl)
**	    Add copyright statement.
**          Add #ifdef for "CL_BUILD_WITH_PROTOS" and prototyped function
**          headers.
**	08-feb-1993 (pearl)
**	    Add #include of me.h.
**      8-jun-93 (ed)
**              changed to use CL_PROTOTYPED
**	11-aug-93 (ed)
**	    unconditional prototypes
**      15-may-1995 (thoro01)
**          Added NO_OPTIM  hp8_us5 to file.
**      19-jun-1995 (hanch04)
**          remove NO_OPTIM, installed HP patch PHSS_5504 for c89
**      29-Nov-1999 (hanch04)
**          First stage of updating ME calls to use OS memory management.
**          Make sure me.h is included.  Use SIZE_TYPE for lengths.
**	25-oct-2001 (somsa01)
**	    The return type of MEactual() is void.
*/


GLOBALDEF SIZE_TYPE i_meactual = 0;
GLOBALDEF SIZE_TYPE i_meuser = 0;

void
MEactual(
	SIZE_TYPE	*user,
	SIZE_TYPE	*actual)
{
    *user = i_meuser;
    *actual = i_meactual;
}
