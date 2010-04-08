/*
**  hilite.c
**
**  Highlight a portion of a window with the passed in attribute.
**  This only highlights a portion on the first line of the window.
**
**  Copyright (c) 2004 Ingres Corporation
**
**  History
**
**  5/10/85 -
**    Written - dkh
**  7/12/85 -
**    Added routine to highlight an entire row in a window. (dkh)
**  19-jun-87 (bab)	Code cleanup.
**  31-may-90 (rog)
**    Included <me.h> so MEfill() is defined correctly.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	28-Mar-2205 (lakvi01)
**	    Corrected function prototypes.
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<me.h>
# include	<fe.h>
# include	<termdr.h>


VOID
TDhilite(win, beg, end, attr)
WINDOW		*win;
i4		beg;
i4		end;
u_char	attr;
{
	char	*dp;

	dp = win->_da[0];
	for (dp += beg; beg <= end && beg < win->_maxx; beg++)
	{
		*dp++ = attr;
	}
}

VOID
TDrowhilite(win, row, attr)
WINDOW		*win;
i4		row;
u_char	attr;
{
	char	*dp;

	dp = win->_da[row];
	MEfill(win->_maxx, attr, dp);
	win->_firstch[row] = 0;
	win->_lastch[row] = win->_maxx - 1;
}
