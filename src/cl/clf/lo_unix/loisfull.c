# include	<lo.h>
# include	<compat.h>
# include	<gl.h>

/*
**Copyright (c) 2004 Ingres Corporation
**
**  LOisfull - return TRUE iff the given LOCATION is a full path.  On UNIX
**		this means that there is a path which starts with a slash.
**
**	Parameters:
**		loc	- pointer to the location to test
**
**	Returns:
**		TRUE if given LOCATION is full path, FALSE if not
**
**	History:
**		11/18/83 (lichtman) - written
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
*/

bool
LOisfull(loc)
LOCATION	*loc;
{
	return ( loc->path != NULL && loc->path[0] == '/' ); 
}
