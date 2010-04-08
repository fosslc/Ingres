/*
**    Copyright (c) 1983, 2000 Ingres Corporation
*/
#include 	<compat.h>  
#include	<gl.h>
#include	<lo.h>  
#include	<st.h>  
#include	<er.h>  
#include	"lolocal.h"

/*LOaddpath
**
**	Add path.
**
**	Add a head an tail component of a directory path.
**	If head has a filename it will be written over.
**	If tail has a filename it will be included.
**
**	success:	return OK
**	failure:	return BAD_SYNTAX--(ie. if tail begins at root)
**
**	
**		History
**			03/09/83 -- (mmm)
**				written
**			09/12/83 -- VMS CL (dd)
**			10/89 -- Clean Up (Mike S)
**      16-jul-93 (ed)
**	    added gl.h
**      11-aug-93 (ed)
**          added missing includes
**	01-dec-2000	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from VMS CL as the use is no longer allowed
*/

/* static char	*Sccsid = "@(#)LOaddpath.c	1.1  3/22/83"; */

STATUS
LOaddpath(head, tail, result)
LOCATION	*head;
LOCATION	*tail;
LOCATION	*result;
{
	char	hbuffer[MAX_LOC+1];	/* Head directory */
	char	tbuffer[MAX_LOC+1];	/* Tail directory */
	char	rbuffer[MAX_LOC+1];	/* Result of combination */
	i4	length;
	STATUS	status;
	LOCATION tmploc;
	LOCTYPE  what;

	if (tail == NULL || head == NULL || result == NULL ||
	    tail->string == NULL || head->string == NULL || 
	    result->string == NULL)
		return(LO_NULL_ARG);

	/* Make the strings to add */
	if ((length = head->nodelen + head->devlen + head->dirlen) == 0)
		return (LO_NULL_ARG);
	STlcopy(head->nodeptr, hbuffer, length);

	if (tail->dirlen == 0)
		return (LO_NULL_ARG);
	STlcopy(tail->dirptr, tbuffer, tail->dirlen);

	/* Combine the two */
	if ((status = LOcombine_paths(hbuffer, tbuffer, rbuffer)) != OK)
		return (status);

	/* Make the combined string */
	length = tail->namelen + tail->typelen + tail->verlen;
	if (length > 0)
		STcat(rbuffer, tail->nameptr);

	/* Make a new location */
	if (head->nodelen > 0)
		what = NODE & PATH;
	else
		what = PATH;
	if (length >0)
		what &= FILENAME;
	if ((status = LOfroms(what, rbuffer, &tmploc)) != OK)
		return (status);
	LOcopy(&tmploc, result->string, result);
	return (OK);
}

/*{
** Name:	LOfaddpath	-	Add a directory to a path.
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
**	result A LOCATION to hold the resulting path.
**		Can be the same location as head.
**
**	Returns:
**		STATUS
**
** History:
**	15-jul-1986 (Joe)
**		First Written for an ABF bug fix.
*/
STATUS
LOfaddpath(head, tailstring, result)
LOCATION	*head;
char		*tailstring;
LOCATION	*result;
{
	char	rbuffer[MAX_LOC+1];
	char	tbuffer[MAX_LOC+1];
	char	hbuffer[MAX_LOC+1];
	LOCATION	tmploc;
	i4	length;
	LOCTYPE	what;
	STATUS	status;

	if (tailstring == NULL || *tailstring == NULL || head == NULL ||
	    head->string == NULL || result == NULL || result->string == NULL)
		return LO_NULL_ARG;
	/*
	** The string has to get sent to be in
	** the form [.tailstring], because that is
	** what a path LOCATION's string would look
	** like.
	*/
	tbuffer[0] = '[';
	tbuffer[1] = '.';

	if ((length = STlength(tailstring)) > MAX_LOC-3)
		return(LO_ADD_BAD_SYN);
	STcopy(tailstring, &(tbuffer[2]));
	tbuffer[length+2] = ']';
	tbuffer[length+3] = '\0';

	if ((length = head->nodelen + head->devlen + head->dirlen) == 0)
		return (LO_NULL_ARG);
	STlcopy(head->nodeptr, hbuffer, length);

	/* Combine the two */
	if ((status = LOcombine_paths(hbuffer, tbuffer, rbuffer)) != OK)
		return (status);

	/* Make a new location */
	if (head->nodelen > 0)
		what = NODE & PATH;
	else
		what = PATH;
	if ((status = LOfroms(what, rbuffer, &tmploc)) != OK)
		return (status);
	LOcopy(&tmploc, result->string, result);
	return (OK);
}
