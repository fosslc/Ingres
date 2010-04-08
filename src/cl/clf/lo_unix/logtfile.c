#include	<compat.h>
#include	<gl.h>
#include	<clconfig.h>
#include	"lolocal.h"
#include	<lo.h>
#include	<st.h>

/*LOgtfile
**
**	LOgtfile gets fname field from loc and stores it in fnamefield.
**
**		Copyright (c) 1983, 1997 Ingres Corporation
**		History
**			03/09/83 -- (mmm)
**				written
**			02/23/89 -- (daveb, seng)
**				rename LO.h to lolocal.h
**	23-may-90 (blaise)
**	    Integrated changes from ingresug:
**		Add "include <clconfig.h>".
**	27-may-97 (mcgem01)
**	    Clean up compiler warnings.
*/

/* static char	*Sccsid = "@(#)LOgtfile.c	3.1  9/30/83"; */

STATUS
LOgtfile(loc, fileloc)
register LOCATION	*loc;
register LOCATION	*fileloc;

{	
	if ((fileloc->fname == NULL) && (fileloc->path == NULL))
	{
		/* empty location */
		fileloc->fname = fileloc->string;
	}

	STcopy(loc->fname, fileloc->fname);

	/* update fileloc info */
	fileloc->desc &= FILENAME;

	return(OK);
}
