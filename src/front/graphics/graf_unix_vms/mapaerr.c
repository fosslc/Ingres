/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ergr.h>
# include	<er.h>
# include	<grerr.h>
# include	<grmap.h>

GLOBALREF char iigrBadColName[];

/**
** Name:	mapaerr.c -	Return Data Map Error Arguments.
**
** Description:
**	Contains a routine that returns the argument to be used when printing
**	out a data mapping error message.
**
**	map_err_arg()	return data map error argument.
**
** History:
**	86/05/06  11:04:55  wong
**		Initial revision.
**	10/02/92 (dkh) - Added support for owner.table, delimids and
**			 filtering out unsupported datatypes.
**      03-jun-1996 (canor01)
**              Rename "errno" to "errnum" to avoid conflicts with system
**              variable name.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

char *
map_err_arg (errnum, map)
register i4	errnum;
GR_MAP		*map;
{
	register char	*ident;

	if (errnum == GDNOTEMP)
		ident = NULL;
	else if (errnum == GDNOTAB || errnum == GDNOROWS || errnum == GDBADTBL ||
	    errnum == GDTABBAD)
		ident = map->gr_view;
	else if (errnum == GDBADCOL)
		ident = iigrBadColName;
	else if (errnum == GDNOX || GDXBAD)
		ident = map->indep;
	else if (errnum == GDNOY || GDYBAD)
		ident = map->dep;
	else	/* errnum == GDNOZ or GDZBAD */
		ident = map->series;

	return ident;
}
