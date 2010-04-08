/*
**  VTdmpinit.c
**
**  Copyright (c) 2004 Ingres Corporation
**
**  static	char	Sccsid[] = "@(#)vtdumpset.c	30.1	12/3/84";
**
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	 <si.h> 

VOID
VTdumpset(fptr)
FILE	*fptr;
{
	FTdmpsp(fptr);
}
