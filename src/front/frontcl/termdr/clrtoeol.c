# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<termdr.h>

/*
**  clrtoeol.c
**
**  This routine clears up to the end of line,
**  where end of line is defined as (_maxx-1) for standard,
**  left-to-right fields, and as 0 for reversed fields.
**
**  Copyright (c) 2004 Ingres Corporation
**
**  History:
**	21-apr-87 (bab)
**		added TDrclreol to support clearing of reversed
**		(right-to-left) fields.
**	08/14/87 (dkh) - ER changes.
**	15-mar-90 (griffin)
**		Turned optimization off for hp3_us5 port.
**	23-jan-91 (rog)
**		Turned optimization back on for hp3_us5 6.3 port.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/


/*
**  This routine clears up to the end of line in the standard
**  left-to-right direction.
**
**  History:
**    xx/xx/xx	-- Created.
**    12/03/86(KY) -- added for handling Double Bytes attributes (_dx)
*/

TDclreol(win)
reg	WINDOW	*win;
{
	reg	char	*sp;
	reg	char	*dx;
	reg	char	*end;
	reg	i4	y;
	reg	i4	x;
	reg	char	*maxx;
	reg	i4	minx;

	y = win->_cury;
	x = win->_curx;
	end = &win->_y[y][win->_maxx];
	minx = _NOCHANGE;
	maxx = &win->_y[y][x];
	dx = &win->_dx[y][x];
	for (sp = maxx; sp < end; sp++, dx++)
	{
		if (*sp != ' ')
		{
			maxx = sp;
			if (minx == _NOCHANGE)
				minx = sp - win->_y[y];
			*sp = ' ';
			*dx = '\0';
		}
	}


	/*
	** update firstch and lastch for the line
	*/
	if (minx != _NOCHANGE)
	{
		if (win->_firstch[y] > minx || win->_firstch[y] == _NOCHANGE)
			win->_firstch[y] = minx;
		if (win->_lastch[y] < maxx - win->_y[y])
			win->_lastch[y] = maxx - win->_y[y];
	}
}


/*
**  This routine clears up to the end of line in the reversed
**  right-to-left direction.
*/

TDrclreol(win)
reg	WINDOW	*win;
{
	reg	char	*sp;
	reg	char	*dx;
	reg	char	*end;
	reg	i4	y;
	reg	i4	x;
	reg	char	*minx;
	reg	i4	maxx;

	y = win->_cury;
	x = win->_curx;
	end = &win->_y[y][0];
	maxx = _NOCHANGE;
	minx = &win->_y[y][x];
	dx = &win->_dx[y][x];
	for (sp = minx; sp >= end; sp--, dx--)
	{
		if (*sp != ' ')
		{
			minx = sp;
			if (maxx == _NOCHANGE)
				maxx = sp - win->_y[y];
			*sp = ' ';
			*dx = '\0';
		}
	}


	/*
	** update firstch and lastch for the line
	*/
	if (maxx != _NOCHANGE)
	{
		if ((win->_firstch[y] > minx - win->_y[y])
			|| (win->_firstch[y] == _NOCHANGE))
		{
			win->_firstch[y] = minx - win->_y[y];
		}
		if (win->_lastch[y] < maxx)
		{
			win->_lastch[y] = maxx;
		}
	}
}
