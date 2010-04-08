/*
** static char	Sccsid[] = %W%  %G%
*/

/*
** Copyright (c) 1985 Ingres Corporation
**
**	Name:
**		UT.c -- UT definitions module.
**
**	Function:
**		None
**
**	Arguments:
**		None
**
**	Result:
**		define the UT module global UTstatus.
**		define the UT module global UT_CO_DEBUG.
**
**	Side Effects:
**		None
**
**	History:
**		03/83 -- (gb)
**			written
**
**		12/84 (jhw) -- Moved definition of `UT_CO_DEBUG' from
**			"UTcompile.c".
*/


# include	<compat.h>
#include    <gl.h>


GLOBALDEF STATUS		UTstatus = OK;

# ifdef UT_DEBUG
GLOBALDEF bool		UT_CO_DEBUG = FALSE;
# endif
