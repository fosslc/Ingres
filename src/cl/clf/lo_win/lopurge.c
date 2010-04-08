/*
**	Copyright (c) 1984 Ingres Corporation
*/

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
**
**		History
**			19-may-95 (emmag)
**			    Branched for NT.
*/

STATUS
LOpurge(loc, keep)
LOCATION	*loc;
i4		keep;
{
    if(!loc)
        return FAIL;
    return (keep ? OK : LOdelete(loc));
}
