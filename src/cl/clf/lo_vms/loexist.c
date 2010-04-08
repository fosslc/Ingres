/*
**    Copyright (c) 1983, 2000 Ingres Corporation
*/
# include	<compat.h>
# include	<gl.h>
# include	<lo.h>
# include 	<er.h>  
# include	"lolocal.h"

/*LOexist
**	Does location exist?
**
**		success: returns YES if location exists.
**			 returns NO if location does not exist.
**
**		failure: returns CANT_TELL if procedure can't tell if
**			 location exists.
**
**		History
**			03/09/83 -- (mmm)
**				written
**			09/27/83 -- (dd) VMS CL
**			25-jun-84 (fhc) -- fixed problem where vms/rms/... holds
**				network object channel if sys$parse is not called
**				to clear out the link.  may sometimes cause problems
**				on non-network IO_ACCESS calls as well.
**			4/19/89 (Mike S) Use LOparse and LOclean_fab
**			10/89 (Mike S) Rewrite in terms of LOinfo
**      16-jul-93 (ed)
**	    added gl.h
**	01-dec-2000	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from VMS CL as the use is no longer allowed
*/

/* static char	*Sccsid = "@(#)LOexist.c	1.4  6/1/83"; */

STATUS
LOexist (loc)
register LOCATION	*loc;	/* location of file or directory */

{
	LOINFORMATION	dummy;
	i4		flags = 0;	/* No information needed */

	return (LOinfo(loc, &flags, &dummy));
}
