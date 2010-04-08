/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/*
**
LIBRARY = IMPFRAMELIBDATA
**
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<er.h>

/**
** Name:	cfglobs.c -	CopyForm global variables
**
** Description:
**	Contains global variables used by copyform routines.
**
** History:
**	27-nov-89 (sandyd)
**		Created.  These globals used to be contained in copyform.c,
**		which is the "main" for copyform.  But many other facilities
**		utilize copyform routines, which have soft GLOBALREF's (via
**		cp.h) to these globals and that causes copyform.c to get 
**		dragged in on VMS, causing duplicate definitions of "main" 
**		(i.e., "compatmain").  In the past, non-copyform executables
**		have worked around this in the past by hacking in their own 
**		GLOBALDEF's of Cfversion and Cf_silent.  But this should be 
**		cleaner.
**	12/23/89 (dkh) - Added support for owner.table by changing version
**			 number to 6.4.
**	29-Sep-2004 (drivi01)
**	    Added LIBRARY jam hint to put this file into IMPFRAMELIBDATA
**	    in the Jamfile.
**/

GLOBALDEF char	Cfversion[] = ERx("6.4");
GLOBALDEF bool	Cf_silent = FALSE;
