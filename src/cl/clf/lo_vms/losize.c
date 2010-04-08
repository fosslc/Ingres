/*
**    Copyright (c) 1983, 2000 Ingres Corporation
*/
#include	<compat.h>  
#include	<gl.h>
#include	<lo.h>  
#include	<er.h>  
#include	"lolocal.h"

/*LOsize
**
**	Get location size.
**
**	Get LOCATION size in bytes.
**
**		History
**			03/09/83 -- (mmm)
**				written
**			09/23/83 -- (dd) VMS CL
**			10/89    -- (Mike S) Rewrite in terms of LOinfo
**      16-jul-93 (ed)
**	    added gl.h
**	19-jul-2000 (kinte01)
**	   Correct prototype definitions by adding missing includes
**	01-dec-2000	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from VMS CL as the use is no longer allowed
*/

/* static char	*Sccsid = "@(#)LOsize.c	1.3  6/1/83"; */

STATUS
LOsize(
register LOCATION	*loc,
register OFFSET_TYPE	*loc_size)

{
	LOINFORMATION 	info;
	STATUS		status;
	i4		flags;

	/* We want size info only */
	flags = LO_I_SIZE;
	
	/* Call LOinfo to get the info */
	status = LOinfo(loc, &flags, &info); 

	/* return the data and/or status */
	if (status != OK) 
	{
		return(status);
	}
	else if ((flags & LO_I_SIZE) == 0)
	{
		return (LO_CANT_TELL);
	}
	else
	{
		*loc_size = (OFFSET_TYPE) info.li_size;
		return (OK);
	}
}
