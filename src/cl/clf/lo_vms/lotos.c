#include	<compat.h>  
#include    <gl.h>
#include	<lo.h>  
# include 	<er.h>  
#include	"lolocal.h"

/*LOtos.c
**	Converts a LOCATION to a string.
**
**		Copyright (c) 1983 Ingres Corporation
**		History
**			03/09/83 -- (mmm)
**				written
**			09/13/83 -- (dd) VMS CL
**			08-jan-86 (jeff)
**			   changed to a VOID function to conform to LO.h
**			10/89 (Mike S)
**			   changed back to a STATUS function to conform with
**			   spec.
**	18-jun-93 (ed)
**	    changed to a VOID to conform to spec
**      16-jul-93 (ed)
**	    added gl.h
*/

/* static char	*Sccsid = "@(#)LOtos.c	1.2  4/9/83"; */

VOID
LOtos(loc,string)
register LOCATION	*loc;	/* location to be read from */
register char		**string;/* string will point to location in string form */

{
	if (loc == NULL)
		return ;

	*string = loc->string;
}
