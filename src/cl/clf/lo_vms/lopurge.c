#include	<compat.h>  
#include	<gl.h>
#include	<st.h>  
#include	<lo.h>  
#include 	<er.h>  
#include	"lolocal.h"


/*LOpurge
**
**  	Purge a location.
**
**	The file or directory specified by loc is purged if possible.
**	Uses PCcmdnoout to run a purge command.  This is NOT server-callable.
**
** PARAMETERS
**	LOCATION	*loc
**		The location to purge. If the location is a directory
**		then the directory is purged.  If the location is a file
**		then only that file is purged.
**
**	int		keep;
**		Number of versions to keep.  If keep == 0 then
**		if loc is a directory, delete all files in the directory,
**		and if loc is a file delete all version of that file.
**
**		Copyright (c) 1983 Ingres Corporation
**		History
**			08/06/84 -- (jrc)
**				written
**			09/24/84 -- (jrc)
**				modified per CL requests.
**			10/89 -- (Mike S) Use LOdelete, where possible
**      16-jul-93 (ed)
**	    added gl.h
**      11-aug-93 (ed)
**          added missing includes
**	19-jul-2000 (kinte01)
**	   Correct prototype definitions by adding external function
**	   references
*/

/* static char	*Sccsid = "@(#)LOdelete.c	1.5  6/1/83"; */

FUNC_EXTERN STATUS PCcmdnoout();

STATUS
LOpurge(loc, keep)
LOCATION	*loc;
int		keep;
{
	char	cmd[MAX_LOC+30];

	if ((loc->string == NULL) || (*(loc->string) == NULL))
	{	
		/* no path in location */
		return(LO_NO_SUCH);
	}
	if (keep == 0)
	{
		if ((loc->desc | FILENAME) == FILENAME)
		{
			return (LOdelete(loc));
		}
		else
		{
			PCcmdnoout(NULL, "delete :=");
			STpolycat(3, ERx("delete "), loc->string, ERx("*.*;*"),
				  cmd);
		}
	}
	else
	{
		PCcmdnoout(NULL, ERx("purge :="));
		STprintf(cmd, ERx("purge/keep=%d %s"), keep, loc->string);
	}
	return PCcmdnoout(NULL, cmd);
}
