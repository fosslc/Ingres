#include	<compat.h>
#include	<gl.h>
#include	<clconfig.h>
#include	"lolocal.h"
#include	<lo.h>

/*LOtos.c
**	Converts a LOCATION to a string.
**
**Copyright (c) 2004 Ingres Corporation
**		History
**			03/09/83 -- (mmm)
**				written
**			02/23/89 -- (daveb, seng)
**				rename LO.h to lolocal.h
**	23-may-90 (blaise)
**	    Integrated changes from ingresug:
**		Add "include <clconfig.h>".
*/

/* static char	*Sccsid = "@(#)LOtos.c	3.1  9/30/83"; */

VOID
LOtos(loc, string)
register LOCATION	*loc;	/* location to be read from */
register char		**string;/* string will point to location in string form */
{
	*string = loc->string;
}
