/*
**Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include	<gl.h>
#include	<clconfig.h>
#include	"lolocal.h"
#include	<lo.h>
#include	<ex.h>
#include	<cs.h>
#include	<si.h>
#include 	<pc.h>
#include 	<st.h>
#include	<errno.h>

/* Externs */

/**
** Name:	logt.c -	Get Location Routine.
**/

GLOBALDEF bool	iiloChanged = TRUE;   /* referenced in lochange.c */

/*{
** Name:	LOgt() -	Get Location.
**
** Description:
**	Get the current directory of the process and return it as a
**	location using the input buffer and location reference.
**
** Input:
**	buf	{char *}  Buffer of at least MAX_LOC bytes used
**				for the location.
**
** Output:
**	loc	{LOCATION *}  The address of the location to contain the path
**				of the current directory.
**
** History:
**	03/09/83 (mmm) -- written
**	11/12/85 (gb)  -- raise EX_NO_FD
**	05/17/88 (jhw) -- Modified to save pathname and re-use it rather than
**			always calling 'popen()'.  The variable 'iiloChanged'
**			will be set by 'LOchange()' when the current directory
**			is changed and 'popen()' must be called again.
**	02/23/89 (seng)-- rename LO.h to lolocal.h
**	23-may-90 (blaise)
**	    Integrated changes from ingresug:
**		Add "include <clconfig.h>";
**		Remove EX_NO_FD stuff leftover from 5.0.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	30-jul-1997 (canor01)
**	    Call getcwd() instead of the overhead with popen().  If used
**	    with an os-threaded server, popen() will call vfork() that
**	    interrupts all suspended threads.
**      05-Jun-2000 (hanal04) Bug 101667 INGDBA 65
**          Added LOfakepwd() which returns './' as the PWD in order to
**          overcome pwd's inability to determine the PWD if the user does
**          not have the appropriate permissions on directories higher up
**          the directory path.
**      25-Jan-2001 (wansh01)                      
**          define LOfakepwd() for UNIX  not xCL_067_GETCWD_EXISTS only       
*/

static char path[MAX_LOC+1];

#ifdef UNIX 
/*{
** Name:	LOfakepwd() -	Get Faked Location.
**
** Description:
**	Treat the pwd as "./" and return it as a location using the 
**      input buffer and location reference.
**
** Input:
**	buf	{char *}  Buffer of at least MAX_LOC bytes used
**				for the location.
**
** Output:
**	loc	{LOCATION *}  The address of the location to contain the path
**				of the current directory.
**
** History:
**	05-Jun-2000 (hanal04) Bug 101667 INGDBA 65
**           The OS call to pwd can fail if the user has permissions for the	
**           current working directory but does not have appropriate 
**           permissions higher up the absolute path. To avoid failure
**           some utilities can make use of this new function which returns
**           the present working directory as './'.
**     23-Sep-2005 (hanje04)
**	     Quiet compiler warnings.
**	23-Jun-2008 (bonro01)
**	    Remove xCL_067_GETCWD_EXISTS since getcwd is now available on
**	    all supported platforms.
*/

static char	fakepwd[MAX_LOC+1];

STATUS
LOfakepwd ( buf, loc )
char		*buf;
LOCATION	*loc;
{
        static bool	first_time = TRUE;
	STATUS	status = OK;
	

	if ( first_time )
	{ 
		i4 		count;

		count = 3;
		STcopy("./", (char *)&fakepwd);
		fakepwd[count-1] = EOS;

		if ( count >= MAX_LOC+1 )
                {
                    status = LO_TOO_LONG;
                }
                else
                { /* get rid of new line from string in buf */
                    fakepwd[count-1] = EOS;
                }
	}

	if ( status == OK )
	{
		first_time = FALSE;

		STcopy( fakepwd, buf );
		(VOID)LOfroms( PATH, buf, loc );
	}

	return status;
}  
# endif
  

STATUS
LOgt ( char *buf, LOCATION *loc )
{
	STATUS	status = OK;
	char    *cp;
	char    locpath[MAX_LOC+1];

	cp = getcwd( locpath, MAX_LOC );

	if ( cp == NULL )
	{
		if ( errno == ERANGE )
			status = LO_TOO_LONG;
		else
			status = FAIL;
	}
	else
	{
		status = OK;
	}
	
	if ( status == OK )
	{
		STcopy( locpath, buf );
		(VOID)LOfroms( PATH, buf, loc );
	}

	return status;
}

