/*
**    Copyright (c) 1983, 2000 Ingres Corporation
*/
#include	<compat.h>  
#include	<gl.h>
#include	<lo.h>  
#include	<st.h>  
#include	<er.h>  
#include	"lolocal.h"

/*LOstfile
**
**	LOstfile gets fname field from filename and stores it in fnamefield.
**
**		History
**			03/09/83 -- (mmm)
**				written
**			09/13/83 -- VMS CL (dd)
**			06/10/85 -- Modified (jrc)
**				Make it call LOfstfile so a change could
**				be localized to one place.
**			10/17/89 (Mike S)
**				Use new LOCATION fields.
**      16-jul-93 (ed)
**	    added gl.h
**      11-aug-93 (ed)
**          added missing includes
**	01-dec-2000	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from VMS CL as the use is no longer allowed
*/

/* static char	*Sccsid = "@(#)LOstfile.c	1.3  4/21/83"; */

STATUS
LOstfile(filename,loc)
register LOCATION	*filename;
register LOCATION	*loc;

{	
	char		buffer[MAX_LOC+1];
	i4		length;

	/* Copy filename part */
	length = filename->namelen + filename->typelen + filename->verlen;
	if (length <= 0)
		*buffer = EOS;		/* No name in LOCATION */
	else
		STlcopy(filename->nameptr, buffer, length);
					/* Copy name from LOCATION */
	
   	return LOfstfile(buffer, loc);
}
