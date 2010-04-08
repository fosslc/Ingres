#include	<compat.h>
#include	<gl.h>
#include	<st.h>
#include        <clconfig.h>
#include	"lolocal.h"
#include	<lo.h>

/*LOstfile
**
**	LOstfile gets fname field from filename and stores it in fnamefield.
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
**	18-Aug-2004 (hanje04)
**   	    BUG 112844
**	    If filename or filename->fname are NULL return fail otherwise
**	    STcopy will SEGV
**	11-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
*/

/* static char	*Sccsid = "@(#)LOstfile.c	1.3  4/21/83"; */

STATUS
LOstfile(filename,loc)
register LOCATION	*filename;
register LOCATION	*loc;

{	
	if ( filename == NULL || filename->fname == NULL )
	    return( FAIL );

	if (loc->fname == NULL)
	{
		/* empty filename location */
		if (loc->path == NULL)
			loc->fname = loc->string;
		else
			loc->fname = loc->path + (i4)(STlength(loc->path));
	}
	if ((loc->fname != loc->string) && 	/* not just a single filename in buf */
	    (*(loc->fname - 1) != SLASH)&& 	/* loc doesn't already have a slash */
	    (filename->fname != NULL)   && 	/* adding non null filename */
	    (*(filename->fname) != '\0'))	/* adding non null filename */
	{
		/* filename of a path with no slash at end */
		/* so add the slash at the end */
		*(loc->fname++) = SLASH;
		*(loc->fname) = '\0';
	}

	STcopy(filename->fname, loc->fname) ;

	/* update loc info */
	loc->desc &= FILENAME;

	return(OK);
}
