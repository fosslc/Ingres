/*
**Copyright (c) 2004 Ingres Corporation
*/
# include	<compat.h>
# include	<gl.h>
# include	<clconfig.h>
# include	"lolocal.h"
# include	<lo.h>
# include	<st.h>

/*{ Name:	LOfaddpath	-	Add a directory to a path.
**
** Description:
**	LOfaddpath is like LOaddpath, except the second argument is not
**	a LOCATION containing a path, but it is a string which is the
**	name of a directory to add to the end of the path given by head.
**
**	LOfaddpath adds the directory named by the string in 'tail' to
**	the path given by 'head'.  'Tail' can only contain a single
**	directory name.  The resulting path is put in the LOCATION 'result'.
**	'head' and 'result' can be the same location -- in this case LOfaddpath
**	will add 'tail' to 'head' leaving a the resulting path in 'head'.
**	'head', 'tail' and 'result' must be non-NULL.  Also, the string
**	given by 'tail' must be non-NULL.
**
**	The resulting path can be used in other calls to other LO routines,
**	but in particular it can be used in calls to LOcreate.  This means
**	the resulting path does not have to exist.  In this way, callers
**	can build a path and then create the path.
**
**	It is needed because on VMS, producing a LOCATION for the second
**	argument to LOaddpath may go through some translations that aren't
**	desired.  This routine gives a way to pass a string to build
**	a path up, and to forgo any possible translation.
**
** Inputs:
**	head	A LOCATION containing a path.
**
**	tail	A string naming a directory to add
**		to the path in head.
** 
** Outputs:
**	pathloc	A LOCATION to hold the resulting path.
**		Can be the same location as head.
**
**	Returns:
**		STATUS
**
** History:
**	15-jul-1986 (Joe)
**		First Written for an ABF bug fix.
**	02/23/89 (daveb, seng)
**		rename LO.h to lolocal.h
**	23-may-90 (blaise)
**	    Integrated changes from ingresug:
**		Add "include <clconfig.h>".
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	11-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
*/

STATUS
LOfaddpath(head, tailstring, pathloc)
LOCATION	*head;
char		*tailstring;
LOCATION	*pathloc;
{
	LOCATION	tailloc;
	STATUS		rval;

	
	if (tailstring == NULL || *tailstring == '\0')
		return LO_NULL_ARG;
	if ((rval = LOfroms(PATH, tailstring, &tailloc)) != OK)
		return rval;
	return LOaddpath(head, &tailloc, pathloc);
}
