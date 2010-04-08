# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<termdr.h>

/*
**  tdmove.c
**
**  Copyright (c) 2004 Ingres Corporation
**
**  History:
**	22-apr-87 (bab) Added movement for right-to-left fields.
**	08/14/87 (dkh) - ER changes.
**	10-sep-87 (bab)
**		Changed from use of ERR to FAIL (for DG.)
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

# define	FORWARDS	1
# define	BACKWARDS	-1


/*
**  This routine moves the cursor to the given point.
*/

i4
TDmove(win, y, x)
reg	WINDOW	*win;
reg	i4	y;
reg	i4	x;
{
	if (x >= win->_maxx)
	{
		x = 0; y++;
	}
	else if (x < 0)
	{
		x = win->_maxx -1; y--;
	}
	if (x >= win->_maxx || y >= win->_maxy || x < 0 || y < 0)
		return(FAIL);
	win->_curx = x;
	win->_cury = y;
	return(OK);
}


i4
TDrmove(win, y, x)
reg	WINDOW	*win;
reg	i4	y;
reg	i4	x;
{
	if (x >= win->_maxx)
	{
		x = 0; y--;
	}
	else if (x < 0)
	{
		x = win->_maxx -1; y++;
	}
	if (x >= win->_maxx || y >= win->_maxy || x < 0 || y < 0)
		return(FAIL);
	win->_curx = x;
	win->_cury = y;
	return(OK);
}
