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
# include	<ug.h>
# include	<graf.h>
# include	<grerr.h>
# include	<grmap.h>

/**
** Name:	maperr.c -	Print errors from graphics data mapping
**
** Description:
**	This file defines:
**
**	maperr()	Print errors from visual graphics editor data mapping
**	maprecover()	Recover from errors from data mapping
**
** History:
**	86/04/15  11:18:42  robin
**		Initial revision.
**
**	86/05/06  11:03:51  wong
**		Abstracted determination of error argument to 'map_err_arg()'.
**	10/28/87 (dkh) - Changed FEgeterrmsg to ERget and IIUGfmt.
**	4/90 (Mike S)  If the table or column name is an SQL keyword, do a full
**		IIUGerr.  This needs a long message, and there won't be graphics
**		on the screen.
**	10/02/92 (dkh) - Added support for owner.table, delimids and
**			 filtering out unsupported datatypes.
**	03-jun-1996 (canor01)
**		Rename "errno" to "errnum" to avoid conflicts with system
**		variable name.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

VOID
maperr( errnum, map )
i4	errnum;
GR_MAP	*map;
{
	register char	*ident;
	char		mbuf[80];

	char	*map_err_arg();

	ident = map_err_arg(errnum, map);

	switch (errnum)
	{
	    case GDBADTBL:
	    case GDBADCOL:
	    case GDTABBAD:
	    case GDXBAD:
	    case GDYBAD:
	    case GDZBAD:
		IIUGerr(errnum, 0, 1, ident);
		break;

	    default:
		IIUGfmt(mbuf, sizeof(mbuf), ERget(errnum),
			(ident == NULL ? 0 : 1), ident);
		FEmsg(mbuf, TRUE);	/* skip leading \t */
		break;
	}
	return;
}


/*{
** Name:	maprecover	-	Recover from a mapdata failure
**
** Description:
**	This routine attempts to 'recover' when a map_frame returns a
**	failure status.	 It prints a message that default data will be
**	used, blanks out the relevant portions of the map structure so
**	that map-frame will use default data, and calls map_frame again.
**	If it gets another failure (which should never happen, except
**	maybe for out-of-memory type conditions), it produces a fatal error
**
** Inputs:
**	map		Pointer to the map structure
**
** Exceptions:
**	Will fatal error if this map_frame call fails.
**
** Side Effects:
**	Blanks out portions of the Map structure.
**
** History:
**	14-apr-1986 (drh)	Created.
*/

VOID
maprecover( frameptr, mapptr )
GR_FRAME	*frameptr;
GR_MAP		*mapptr;
{
	i4	*warnings;
	STATUS	retval;

	/*
	**  Print the message that default data will be used
	*/

	FEmsg(ERget(GDDEFLT), (bool) TRUE);

	/*
	**  Blank out the table/view name and re-map
	*/

	mapptr->gr_view = ERx("") ;

	if ( (retval = ( map_frame( frameptr, mapptr, &warnings ) )) != OK )
	{
		IIUGerr(S_GR0013_MAPRECOVER, UG_ERR_FATAL, 0);
	}
	else if ( warnings != (i4 *) NULL )
	{
		mapwarn( warnings );
	}

	return;
}
