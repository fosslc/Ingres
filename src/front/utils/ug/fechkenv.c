/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include	<si.h>
#include	<lo.h>
#include	<nm.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>

/**
** Name:    fechkenv.c -	Front-End Utility Check Environment Module.
**
** Description:
**	Contains a routine that is used to check information about a user 
**	environment.  Defines:
**
**	fechkenv()	check environment attributes.
**
** History:	
**	Revision 6.0  86/12/15  peter
**	Initial revision.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/


/*{
** Name:    fechkenv() -	Check Environment Attributes.
**
** Description:
**	This routine is used to check attributes of a user environment.
**	It is called with a flag (or mask of flags) and returns TRUE
**	if all of the flags are TRUE.
**
**
**	FENV_TWRITE	- writable ING_TEMP area.  This is used to
**			  check early to see if the user environment
**			  has a writable temp area so that programs
**			  make users fix it before going too far.
**
** Inputs:
**	mask		mask of flags in a i4  (ORed together).  ALL
**			flags must be TRUE to pass.
**
** Returns:
**	{STATUS} OK if ALL flags are true.
**		FAIL otherwise.
**
** History:
**	15-dec-1986 (peter)	Written.
*/

STATUS
FEchkenv (flags)
i4	flags;
{
    if ((flags & FENV_TWRITE) != 0)
    {	
	LOCATION	tloc;	/* Location to put temp file */
	LOINFORMATION	loinfo;
	i4		lflags = LO_I_PERMS;

	/* NMloc is strange, it doesn't take a buffer arg. */
	NMloc(TEMP, PATH, NULL, &tloc);

	if (LOinfo(&tloc, &lflags, &loinfo) != OK
	  || (loinfo.li_perms & LO_P_WRITE) == 0)
	{
		return FAIL;
	}
    }
    return OK;
}
