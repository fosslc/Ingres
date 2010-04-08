# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<termdr.h>
# include	<cm.h>

/*
**  delch.c
**
**  Copyright (c) 2004 Ingres Corporation
**
**  History:
**	21-apr-87 (bab)
**		Added TDrdelch to support reversed (right-to-left)
**		fields.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

/*
**  This routine performs a delete-char on the line in the standard
**  (left-to-right) direction, leaving (_cury,_curx) unchanged.
**
**  History:
**	    xx/xx/xx	-- Created.
**	    12/03/86(KY) -- Added for handling Double Byte attributes (_dx).
*/

TDdelch(win)
reg	WINDOW	*win;
{
	reg	char	*temp1;
	reg	char	*temp2;
	reg	char	*end;
	reg	i4	y;
	reg	i4	x;
	reg	char	*dx1;
	reg	char	*dx2;
	reg	i4	offset;

	y = win->_cury;
	x = win->_curx;
	offset = CMbytecnt(&win->_y[y][x]);
	/*
	** when here is first position of line and previous char is a
	** phantom space and next cursor position is on a single byte
	** character then a phantom space have to be deleted and squeeze it.
	*/
	if (x == 0 && y > 0 && win->_dx[y-1][win->_maxx-1] & _PS)
	{
		/*
		**	+--------+			+-------*+
		**	|abcdefg^|	delete	'K1'	|abcdefgh|
		**	|K1hijk  |			|ijk     |
		**	+*-------+			+--------+
		**
		**  A character on current cursor position should be
		**  a Double Byte character.
		*/
		win->_cury = y - 1;
		win->_curx = win->_maxx - 1;
		offset += TDfillch(win, y-1, win->_maxx-1, 2);
		win->_lastch[y-1] = win->_maxx - 1;
		if (win->_firstch[y-1] == _NOCHANGE
			|| win->_firstch[y-1] > win->_maxx-1)
		{
			win->_firstch[y-1] = win->_maxx - 1;
		}
	}

	while (y < win->_maxy && offset > 0)
	{
		end = &win->_y[y][win->_maxx - 1];
		temp1 = &win->_y[y][x];
		temp2 = temp1 + offset;
		dx1 = &win->_dx[y][x];
		dx2 = dx1 + offset;
		while (temp2 <= end)
		{
			*temp1++ = *temp2++;
			*dx1++   = *dx2++;
		}
		if (*(dx1-1) & _PS)
		{
			*(dx1-1) = '\0';
			offset += 1;
		}
		temp1 = &win->_y[y][win->_maxx - offset];

		if (y < win->_maxy - 1)
		{
			offset = TDfillch(win, y, win->_maxx - offset, 0);
		}
		else
		{
			for (; offset > 0; offset--)
			{
				*temp1++ = ' ';
				*dx1++ = '\0';
			}
		}
		win->_lastch[y] = win->_maxx - 1;
		if (win->_firstch[y] == _NOCHANGE || win->_firstch[y] > x)
		{
			win->_firstch[y] = x;
		}
		y++;
		x = 0;
	}

	if (win->_dx[win->_cury][win->_curx] & _PS)
	{
		if (win->_cury < win->_maxy - 1)
		{
			win->_cury += 1;
			win->_curx = 0;
		}
	}
	return(OK);
}

/*
** put characters which are on line (y+1) and beginning of (fromx), into
** window beginning at (x,y) and return number of bytes can put into line
** (y+1) from line (y+2).
*/
TDfillch(win, y, x, fromx)
reg	WINDOW	*win;
reg	i4	y;
reg	i4	x;
reg	i4	fromx;	/* if not zero fill char to (x,y) from (fromx,y+1) */
{
	reg	char	*temp1;
	reg	char	*temp2;
	reg	char	*end;
	reg	char	*dx1;
	reg	char	*dx2;
	reg	i4	offset;

	end  = &win->_y[y][win->_maxx - 1];
	temp1 = &win->_y[y][x];
	temp2 = &win->_y[y + 1][fromx];
	offset = end - temp1 + 1;
	dx1 = &win->_dx[y][x];
	dx2 = &win->_dx[y + 1][fromx];
	while (temp1 <= end)
	{
		*temp1++ = *temp2++;
		*dx1++   = *dx2++;
	}
	/*
	** 2nd byte code of a Double Byte character includes both of 1st and
	** 2nd byte code then need checking whether 2nd byte or not.
	*/
	if (CMdbl1st(end) && !(*(dx1-1) & _DBL2ND))
	{
		offset -= 1;
		*end = PSP;
		win->_dx[y][win->_maxx - 1] = _PS;
	}
	return(offset);
}

/*
**  This routine performs a delete-char on the line in the reverse
**  (right-to-left) direction, leaving (_cury,_curx) unchanged.
**
**  This routine does not support two-byte characters; dx is updated
**  only to maintain a clean buffer.
*/

TDrdelch(win)
reg	WINDOW	*win;
{
	reg	char	*temp1;
	reg	char	*temp2;
	reg	char	*end;
	reg	i4	y;
	reg	i4	x;
	reg	char	*dx1;
	reg	char	*dx2;

	y = win->_cury;
	x = win->_curx;

	while (y < win->_maxy)
	{
		end = &win->_y[y][0];
		temp1 = &win->_y[y][x];
		temp2 = temp1 - 1;
		dx1 = &win->_dx[y][x];
		dx2 = dx1 - 1;
		while (temp2 >= end)
		{
			*temp1-- = *temp2--;
			*dx1--   = *dx2--;
		}

		/*
		** check which line y is on, and get the next char
		** from the next line if NOT the last line
		*/
		if (y < win->_maxy - 1)
		{
			*temp1 = win->_y[y + 1][win->_maxx - 1];
			*dx1 = win->_dx[y + 1][win->_maxx - 1];
		}
		else
		{
			*temp1 = ' ';
			*dx1 = '\0';
		}
		win->_firstch[y] = 0;
		if (win->_lastch[y] == _NOCHANGE || win->_lastch[y] < x)
		{
			win->_lastch[y] = x;
		}
		y++;
		x = win->_maxx - 1;
	}

	return(OK);
}
