/*
**	Copyright (c) 1983 Ingres Corporation
*/
#include	<compat.h>
#include	<gl.h>
#include	<clconfig.h>
#include	"lolocal.h"
#include	<lo.h>

/*LOtos.c
**	Converts a LOCATION to a string.
**
**		History
**			03/09/83 -- (mmm)
**				written
**			02/23/89 -- (daveb, seng)
**				rename LO.h to lolocal.h
**		19-may-95 (emmag)
**		    Branched for NT.
*/

VOID
LOtos(loc, string)
register LOCATION	*loc;	/* location to be read from */
register char		**string;/* string will point to location in string form */
{
	*string = loc->string;
}
