
#include	<compat.h>
#include	<gl.h>
#include        <clconfig.h>
#include	"lolocal.h"
#include	<lo.h>
#include	<st.h>

/*
** Copyright (c) 1995 Ingres Corporation
**
** LOaddpath -- Add path to location.
**
**	Add a head and tail component of a directory path.
**
**	success:	return OK
**	failure:	return BAD_SYNTAX--(ie. if tail begins at root)
**
**
**	History:
**	19-may-95 (emmag)
**	    Branched for NT.	
**
*/


STATUS
LOaddpath(head, tail, pathloc)
register LOCATION	*head;
register LOCATION	*tail;
LOCATION		*pathloc;
{
	register char	*string;


	if (head == (LOCATION *)NULL || tail == (LOCATION *)NULL ||
			pathloc == (LOCATION *)NULL ||
			head->path == (char *)NULL||tail->path == (char *)NULL)
	{
		/* arguments must not be null */
		return LO_NULL_ARG;
	}

	if ((*tail->path == SLASH) || (*(tail->path+1) == COLON))
	{
		/* tails can't start at root */
		return LO_ADD_BAD_SYN;
	}

	/* INIT */

	string = pathloc->string;

	/* cat head of path to pathloc */

	if (head == pathloc)
	{
		/* user is using head as result location */
		string = head->fname;
	}
	else
	{
		STcopy(head->path, string);
		string += STLENGTH_MACRO(string);
	}

	if (((*head->path == SLASH) && (head->path[1] == '\0')) ||
            ((head->path[1] == COLON) && (head->path[2] == SLASH)
                                        && (head->path[3] == '\0')))
	{
		/* special case for root */
		/* you don't need to add slash in this case */
		/* don't do anything */;
	}
	else
	{
		/* add slash between non null head and tail paths */
		*string++ = SLASH;
	}

	/* cat tail of path to pathloc */

	STcopy(tail->path, string);
	string += STlength(string);

	/* pathloc-> fname will point to null of the path name */

	pathloc->path  = pathloc->string;
	pathloc->fname = string;

	return OK;
}
