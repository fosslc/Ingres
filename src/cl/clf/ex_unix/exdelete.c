# include	<compat.h>
# include	<gl.h>
# include	<ex.h>
# include	"exi.h"

/*
**Copyright (c) 2004 Ingres Corporation
*/

/*
**
** EXdelete.c
**
** DECLARES
**	
**	EXdelete()
**
**		Deletes the current contexts' handler.  
**
**		Routine is specified to be a NO-OP if no handler
**		Is declared for the current scope, but we can't implement
**		that portably.  We actually pop the top of the 
**		the top of the exception stack.  This can be disastrous
**		if there is no handler left.
**
** RETURNS
**	NOTHING.
**
** IMPLEMENTATION
**	Pops the exception stack of all LARGER or EQUAL scopes.
**
** History:
**	27-Dec-2000 (jenjo02)
**	    i_EXtop() now returns **EX_CONTEXT rather than *EX_CONTEXT,
**	    i_EXpop() takes **EX_CONTEXT as parameter. For OS threads,
**	    this reduces the number of ME_tls_get() calls from 3 to 1.
*/

STATUS
EXdelete()
{
	EX_CONTEXT	**excp;
	
	if ( *(excp = i_EXtop()) )
		i_EXpop(excp);
	return(OK);
}
