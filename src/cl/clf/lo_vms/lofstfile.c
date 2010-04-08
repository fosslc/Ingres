/*
**    Copyright (c) 1983, 2000 Ingres Corporation
*/
#include	<compat.h>  
#include	<gl.h>
#include	<lo.h>  
#include	<st.h>  
#include	<er.h>  
#include	"lolocal.h"

/*LOfstfile
**
**	LOfstfile gets fname as a string and stores it in fnamefield
**	of loc.
**
**		History
**			03/09/83 -- (mmm)
**				written
**			09/13/83 -- VMS CL (dd)
**			06/10/85 -- Modified (jrc)
**				Changed so that if the location doesn't
**				have a COLON or a CLSBRK, a COLON is
**				placed between them since it might be
**				a device only.
**			10/89   -- (Mike S) Use LOCATION pointers to find file 
**      16-jul-93 (ed)
**	    added gl.h
**	01-dec-2000	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from VMS CL as the use is no longer allowed
*/

/* static char	*Sccsid = "@(#)LOfstfile.c	1.2  5/24/83"; */

LOfstfile(filename,loc)
char			*filename;
register LOCATION	*loc;

{	
	LOCATION	tmploc;
        char		buffer[MAX_LOC+1];
	i4		length;
 	STATUS 		status;

	/* 
	** Construct a string with loc's node, device, and directory, and
	** filename.
	*/
	length = loc->nameptr - loc->string;
	if (length == 0)
		*buffer = EOS;
	else
		STlcopy(loc->string, buffer, length);
	STcat(buffer, filename);

	/* Make a new location */
	status = LOfroms(loc->desc & FILENAME, buffer, &tmploc);
	if (status != OK) return status;

	/* Copy it */
	LOcopy(&tmploc, loc->string, loc);
	return (OK);
}
