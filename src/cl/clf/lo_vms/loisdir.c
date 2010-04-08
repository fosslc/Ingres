/*
**    Copyright (c) 1983, 2000 Ingres Corporation
*/
# include	<compat.h> 
# include	<gl.h>
# include	<lo.h>  
# include	<er.h>  
# include	"lolocal.h"

/*
 *
 *	Name:
 *		LOisdir
 *
 *	Function:
 *		LOisdir makes a system call to determine if a location is a
 *		directory.  Return fail if location is non-existatnt.
 *
 *	Arguments:
 *		loc -- a pointer to the location.
 *		flag -- set to ISDIR if location id directory. Set to
 *			ISFILE if location is file.
 *	Assumptions:
 *		location is complete; the path is not left as a default.
 *	
 *	Result:
 *		Returns OK if location exists.
 *
 *	Side Effects:
 *
 *	History:
 *		4/4/83 -- (mmm) written
 *		9/26/83 -- (dd) VMS CL
 *		4/19/89 -- (Mike S) use LOparse and LOclean_fab
 *		10/89   -- (Mike S) Use loinfo
 *
 *
**      16-jul-93 (ed)
**	    added gl.h
**	01-dec-2000	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from VMS CL as the use is no longer allowed
 */

STATUS
LOisdir(loc, flag)
register LOCATION 	*loc;
register i2		*flag;
{

	LOINFORMATION info;
	i4	      i_flags;	
	STATUS	      status;

	/* We want type information */
	i_flags = LO_I_TYPE;

	/* Find it out */
	status = LOinfo(loc, &i_flags, &info);

	/* Return status and/or data */
	if (status != OK)
	{
		*flag = NULL;
		return (status);
	}
	else if ((i_flags & LO_I_TYPE) == 0)
	{
		*flag = NULL;
		return (LO_CANT_TELL);
	}
	else
	{
		switch(info.li_type)		
		{
		    case LO_IS_DIR:
			*flag = ISDIR;
			return (OK);
		    case LO_IS_FILE:
			*flag = ISFILE;
			return (OK);
		    default:
			*flag = NULL;
			return (FAIL);
		}
	}
}
