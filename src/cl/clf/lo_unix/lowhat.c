#include	<compat.h>
#include	<gl.h>
#include	<clconfig.h>
#include	"lolocal.h"
#include	<lo.h>

/*LOwhat
**
**	Returns type of location. (ie. NODE, PATH, FILENAME, PATH & FILENAME ...)
**	The location type returned is determined by location not the real world.
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

/* static char	*Sccsid = "@(#)LOwhat.c	3.1  9/30/83"; */

STATUS
LOwhat(loc, loctype)
register LOCATION	*loc;
register LOCTYPE	*loctype;
{	


	if ((loc == NULL) || (loctype == NULL))
	{
		return(LO_NULL_ARG);
	}

	*loctype = loc->desc;

	return(OK);
}
