/*
** Copyright (c) 2004 Ingres Corporation
*/

/* static char	Sccsid[] = "@(#)rfdelete.c	30.1	11/14/84"; */

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	 <lo.h> 
# include	 <rcons.h> 

/*
**   R_FDELETE - delete the named file from the system catalogs.
**
**	Parameters:
**		name - name of file (simple name).
**		(mmm) now name is a pointer to a LOCATION
**
**	Returns:
**		OK	if successful
**
**	Trace Flags:
**		2.0, 2.14.
**
**	History:
**		6/1/81 (ps) - written.
**		4/6/83 (mmm)- added locations.
**		11-sep-1992 (rdrane)
**			Return the STATUS value from LOdelete
**			directly.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/



STATUS
r_fdelete(name)
char	*name;
{
	/* external declarations */

	FUNC_EXTERN	STATUS	LOdelete();	/* routine to delete files */

	/* internal declarations */

	i4		retcode;		/* code for return */
	STATUS		status;
	LOCATION	loc;			/* (mmm) added location */

	/* start of routine */



	LOfroms(PATH & FILENAME, name, &loc);
	status  = LOdelete(&loc);

	return(status);
}
