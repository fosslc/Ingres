#include	<compat.h>
#include	<gl.h>
#include        <clconfig.h>
#include	"lolocal.h"
#include	<lo.h>
#include        <st.h>

/*LOfstfile
**
**	LOfstfile gets fname as a string and stores it in fnamefield
**	of loc.
**
**Copyright (c) 2004 Ingres Corporation
**	History
**		03/09/83 -- (mmm) written
**		9/29/87 -- (daveb)  Don't incorrectly redeclare STlength.
**			   Eliminate silly cast.
**		02/23/89 (daveb, seng)
**			rename LO.h to lolocal.h
**	23-may-90 (blaise)
**	    Integrated changes from 61 and ingresug:
**		Add "include <clconfig.h>";
**		Replace STlength with STLENGTH_MACRO.
**	11-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
*/

STATUS
LOfstfile(filename, loc)
char			*filename;
register LOCATION	*loc;
{	
	/* check to make sure the file name is the right length */

	if (STLENGTH_MACRO(filename) > LO_NM_LEN)
	{
		return(FAIL);
	}

	if (loc->fname == NULL)
	{
		/* empty filename location */
		if (loc->path == NULL)
			loc->fname = loc->string;
		else
			loc->fname = &loc->path[ STLENGTH_MACRO(loc->path) ];
	}

	if ((loc->fname != loc->string)  && 	/* not just a single filename in buf */
	    (*(loc->fname - 1) != SLASH) && 	/* loc doesn't already have a slash */
	    (filename    != NULL)  	 && 	/* adding non null filename */
	    (*(filename) != '\0'))		/* adding non null filename */
	{
		/* filename of a path with no slash at end */
		/* so add the slash at the end */

		*(loc->fname++)	= SLASH;
		*(loc->fname)	= '\0';
	}

	STcopy(filename, loc->fname);

	/* update loc info */

	loc->desc &= FILENAME;

	return(OK);
}
