#include	  <compat.h>  
#include    <gl.h>
#include	  <lo.h>  
# include 	  <er.h>  
#include	  "lolocal.h"  

/*LOwhat
**
**	Returns type of location. (ie. NODE, PATH, FILENAME, PATH & FILENAME ...)
**	The location type returned is determined by location not the real world.
**
**		Copyright (c) 1983 Ingres Corporation
**		History
**			03/09/83 -- (mmm)
**				written
			09/13/83 -- (dd) VMS CL
**      16-jul-93 (ed)
**	    added gl.h
*/

/* static char	*Sccsid = "@(#)LOwhat.c	1.3  4/9/83"; */
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
