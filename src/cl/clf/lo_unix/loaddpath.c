
#include	<compat.h>
#include	<gl.h>
#include        <clconfig.h>
#include	"lolocal.h"
#include	<lo.h>
#include	<st.h>

/*
**Copyright (c) 2004 Ingres Corporation
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
**	
 * Revision 1.3  90/05/23  21:08:42  source
 * sc
 * 
 * Revision 1.2  90/05/23  15:56:40  source
 * sc
 * 
 * Revision 1.1  90/03/09  09:15:27  source
 * Initialized as new code.
 * 
 * Revision 1.2  90/02/12  09:42:48  source
 * sc
 * 
 * Revision 1.1  89/05/26  15:52:07  source
 * sc
 * 
 * Revision 1.1  89/05/11  01:12:54  source
 * sc
 * 
 * Revision 1.2  89/03/06  14:29:17  source
 * sc
 * 
 * Revision 1.1  89/01/13  17:12:28  source
 * This is the new stuff.  We go dancing in.
 * 
 * Revision 1.1  88/08/05  13:40:36  roger
 * UNIX 6.0 Compatibility Library as of 12:23 p.m. August 5, 1988.
 * 
 * Revision 60.3  87/10/09  15:52:59  bobm
 * no change
 * 
 * Revision 60.2  86/12/02  09:49:31  joe
 * Initial60 edit of files
 * 
 * Revision 60.1  86/12/02  09:49:16  joe
 * *** empty log message ***
 * 
**Revision 30.1  85/08/14  19:12:23  source
**llama code freeze 08/14/85
**
**		Revision 3.0  85/05/10  21:49:38  wong
**		'STcopy()' no longer returns length.  Increment 'string'
**		using 'STlength()'.
**		
**		03/09/83 -- (mmm)
**			written
**		02/23/89 (daveb, seng)
**			rename LO.h to lolocal.h
**	23-may-90 (blaise)
**	    Integrated changes from 61 and ingresug:
**		Add "include <clconfig.h>" and moved "lolocal.h" include
**		to follow it;
**		Replace STlength with STLENGTH_MACRO.
**	23-may-90 (rog)
**	    Included <st.h> to pick up the definition of STLENGTH_MACRO.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
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
			head->path == (char *)NULL || tail->path == (char *)NULL)
	{
		/* arguments must not be null */
		return LO_NULL_ARG;
	}

	if (*(tail->path) == SLASH)
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

	if ((*head->path == SLASH) && (*(head->path +1) == '\0'))
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
	string += STLENGTH_MACRO(string);

	/* init rest of pathloc location */
	/* null terminate */

	*string = '\0';

	/* pathloc-> fname will point to null of the path name */

	pathloc->path  = pathloc->string;
	pathloc->fname = string;

	return OK;
}
