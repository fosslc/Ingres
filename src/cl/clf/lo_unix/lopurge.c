
/* static char	*Sccsid = "@(#)LOpurge.c	%W%	%G%"; */


# include	<compat.h> 
# include	<gl.h> 
# include	<st.h> 
# include	<lo.h> 
# include	<pc.h> 
# include	<er.h> 
# include	"LOerr.h"


/*
** LOpurge
**
**  	Purge a location.
**
**	Not relevant on UNIX, except when the parameter keep == 0.
**	In this case, uses PCdocmdline to run an rm command on the
**	file or directory specified by loc.
**
** PARAMETERS
**	LOCATION	*loc
**		The location to purge.
**
**	int		keep;
**		Number of versions to keep.  Only relevant if keep == 0,
**		when file (or all files in directory) are deleted.
**
**Copyright (c) 2004 Ingres Corporation
**
**		History
**			08/06/84 -- (jrc) written
**			09/24/84 -- (agh) revised for UNIX CL
**			08-jan-90 (sylviap)
**				Added PCcmdline parameters.
**      		08-mar-90 (sylviap)
**              		Added a parameter to PCcmdline call to pass an 
**				CL_ERR_DESC.
**			11-Jun-90 (Mike S)
**				Call LOdelete to delete a single file.  Redirect
**				error from PCcmdline to avoid displaying an
**				error from rm.
**	11-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
*/

STATUS
LOpurge(loc, keep)
LOCATION	*loc;
i4		keep;
{
	char	    cmd[MAX_LOC+10];
	CL_ERR_DESC err_code;
	LOCATION nlloc;
	char nlloc_buffer[MAX_LOC+1];

	if ((loc->string == NULL) || (*(loc->string) == '\0'))
	{	
		/* no path in location */
		return(LO_NO_SUCH);
	}

	if (keep != 0)
		return OK;

	switch (loc->desc)
	{
	    case FILENAME:
	    case FILENAME & PATH:
	    case FILENAME & NODE:
	    case FILENAME & PATH & NODE:
		return LOdelete(loc);
		break;

	    case PATH:
	    case PATH & NODE: 
		STcopy(ERx("/dev/null"), nlloc_buffer);
		LOfroms(FILENAME&PATH, nlloc_buffer, &nlloc);
		STpolycat(3, "rm ", loc->string, "/*", cmd);
		return PCcmdline((LOCATION *) NULL, cmd, PC_WAIT, &nlloc, 
				 &err_code);
		break;

	    default:
		return LO_CANT_TELL;
		break;
	}
}
