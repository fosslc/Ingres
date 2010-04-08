#include        <compat.h>
#include        <gl.h>
#include        <clconfig.h>
#include	<cm.h>
#include        <lo.h>
#include        <ex.h>
#include        <si.h>
#include	<st.h>
#include        "lolocal.h"
#include        <errno.h>

/*
**	Copyright (c) 1983, 1999 Ingres Corporation
**
**  LOisfull - return TRUE if the given LOCATION is a full path.  On UNIX
**		this means that there is a path which starts with a slash.
**
**	Parameters:
**		loc	- pointer to the location to test
**
**	Returns:
**		TRUE if given LOCATION is full path, FALSE if not
**
**	History:
**	    19-may-95 (emmag)
**		Branched for NT.
**	    08-dec-1997 (canor01)
**	        Check for valid UNC name on NT.
**	    03-mar-1999 (canor01)
**	        Correct call to STindex.
*/

bool
LOisfull(loc)
LOCATION	*loc;
{
    bool ret_val = FALSE;

    if ( loc->path )
    {
	if ( ( CMalpha( &loc->path[0] ) && 
	       loc->path[1] == COLON &&
	       loc->path[2] == SLASH ) ||
	     ( loc->path[0] == SLASH && 
	       loc->path[1] == SLASH &&
	       loc->path[2] != SLASH &&
	       STindex( &loc->path[3], SLASHSTRING, 0 ) != NULL ) )
	    ret_val = TRUE;
    }

    return (ret_val);
}
