/*
**    Copyright (c) 1983, 2000 Ingres Corporation
*/
#include	<compat.h>  
#include	<gl.h>
#include	<lo.h>  
#include	<er.h>  
#include	"lolocal.h"

/*LOlast
**
**	Location last modified.
**
**	Get last modification date of a LOCATION.
**
**		History
**			03/09/83 -- (mmm)
**				written
**			09/27/83 -- (dd) VMS CL
**			10/89 -- (Mike S) Use LOinfo
**      16-jul-93 (ed)
**	    added gl.h
**	01-dec-2000	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from VMS CL as the use is no longer allowed
*/

/* static char	*Sccsid = "@(#)LOlast.c	1.6  6/1/83"; */

STATUS
LOlast(loc,last_mod)
register LOCATION	*loc;
register SYSTIME	*last_mod;

{
	STATUS 		status;
	LOINFORMATION 	info;
	i4		flags;

	/* Set up default return */
	last_mod->TM_secs = -1;
	last_mod->TM_msecs = -1;

	/* Set up flag for time info only */
	flags = LO_I_LAST;
	status = LOinfo(loc, &flags, &info);

	/* return the data and/or status */
	if (status != OK) 
	{
		return(status);
	}
	else if ((flags & LO_I_LAST) == 0)
	{
		return (LO_CANT_TELL);
	}
	else
	{
		STRUCT_ASSIGN_MACRO(info.li_last, *last_mod);
		return (OK);
	}
}
