# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<termdr.h>
# include	<cm.h>
# include	<er.h>

/*
**  insch.c
**
**  This routine performs an insert-char on the line, moving
**  (_cury,_curx) to the next position in the window
**  as appropriate for standard vs. reverse fields.
**
**  Copyright (c) 2004 Ingres Corporation
**
**  History:
**	22-apr-87 (bruceb)
**		Added TDrinsstr routine to handle insertion in
**		the reverse (right-to-left) direction.  This
**		routine will not work for two-byte characters.
**	19-jun-87 (bruceb)	Code cleanup.
**	08/14/87 (dkh) - ER changes.
**	10-sep-87 (bruceb)
**		Changed from use of ERR to FAIL (for DG.)
**	10/19/87 (dkh) - Fixed jup bug 1161.
**	05-apr-89 (bruceb)	Fixed bug 4716.
**		When deciding whether any room exists to copy contents
**		back from tempwin, use position from before the addstr().
**	09/21/89 (dkh) - Porting changes integration.
**	13-sep-93 (dianeh)
**		Removed NO_OPTIM setting for obsolete Ultrix port (ds3_ulx).
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

/*
**  This routine performs insertion of a character, handling both
**  Double byte and single byte characters on the line,
**  moving (_cury,_curx) to the next position of the window in a
**  left-to-right direction.
**
**	History:
**		mm/dd/yy (RTI) -- created for 5.0.
**		01/21/87 (KY)  -- Added CM.h
**				    and changed to handle only strings.
*/

i4
TDinsstr(awin, string)
WINDOW	*awin;
char	*string;
{
	reg	WINDOW	*win = awin;
	reg	char	*temp1;
	reg	char	*temp2;
	reg	char	*dx1;
	reg	char	*dx2;
	reg	char	*end1;
	reg	char	*end2;
	reg	i4	x;
	reg	i4	y;
	reg	i4	newx;
	reg	i4	ty = 0;
	reg	i4	tx = 0;
	reg	u_char	*insstr;
		u_char	buf[3];
    
	buf[0] = '\0';
	buf[1] = '\0';
	buf[2] = '\0';
	CMcpychar(string, buf);
	insstr = buf;
	y = win->_cury;
	x = win->_curx;

	switch(*insstr)
	{
		case '\t':
			for(newx = x+(8-(x&07)); x < newx; x++)
			{
				if(TDinsstr(win, ERx(" ")) == FAIL)
					return(FAIL);
			}
			return(OK);

		case '\r':
		case '\n':
			for(newx = win->_maxx; x < newx; x++)
			{
				if(TDinsstr(win, ERx(" ")) == FAIL)
					return(FAIL);
			}
			return(OK);

		default:    /*
			    **	to insert 'insstr' string into the window,
			    **  shift the data in the window right.
			    **	temporary window 'tempwin' is needed for saving
			    **  an original line because except using 'tempwin',
			    **  inserting characters causes overwriting
			    **  characters which are not shifted yet.
			    **
			    **	window 'win' indicated by x, y, temp1 and dx1.
			    **	window 'tmpwin' indicated by tx, ty, temp2 and
			    **  dx2.
			    */

			ty = -1;    /* '-1' is used for initial pass flag */
			tx = 0;
			while(y < win->_maxy)
			{
				TDtwincpy(win, tempwin, y, 1);
				if (ty < 0)
				{
					ty = y;
					end2 = &tempwin->_y[y][win->_maxx - 1];
					temp2 = &tempwin->_y[y][x];
					dx2 = &tempwin->_dx[y][x];
					if (x >= win->_maxx - CMbytecnt(insstr)
						&& y < win->_maxy - 1)
					{
						/*
						** in the case that after insert
						** 'insstr' the cursor moved
						** out of the line, 'tempwin'
						** has to be created to save the
						** next line.
						*/
						TDtwincpy(win, tempwin, y+1, 1);
					}
					if (y > 0 && x == 0
					   && win->_dx[y-1][win->_maxx-1] & _PS)
					{
						win->_cury = y - 1;
						win->_curx = win->_maxx - 1;
					}
					x = win->_curx;
					y = win->_cury;
					TDaddstr(win, insstr);
					/*
					**  Nothing to copy if at the last
					**  or near last position before the
					**  TDaddstr().
					*/
					if (y == win->_maxy - 1 &&
					    (x == win->_maxx - 1 ||
					    (x == win->_maxx - 2 &&
					    win->_dx[y][win->_maxx -1] & _PS)))
					{
						break;
					}
					x = win->_curx;
					y = win->_cury;
				}
				if (ty == y && tx == x)
				{
					/*
					** current positions on 'win' and
					** 'tempwin' are same.  it means the
					** remaining data doesn't need any
					** more change.
					*/
					break;
				}

				end1 = &win->_y[y][win->_maxx - 1];
				temp1 = &win->_y[y][x];
				dx1 = &win->_dx[y][x];
				while (temp1 <= end1)
				{
					/*
					** copy data from 'tempwin' to 'win'
					*/
					*temp1++ = *temp2++;
					*dx1++ = *dx2++;
					tx++;
					if (temp2 > end2 || *dx2 & _PS)
					{
						/*
						** move 'tempwin' pointer to
						** next line.  always next line
						** data was saved into 'tempwin'
						** before.
						*/
						tx = 0;
						ty++;
						end2 = &tempwin->_y[ty][win->_maxx - 1];
						temp2 = &tempwin->_y[ty][0];
						dx2 = &tempwin->_dx[ty][0];
					}
				}
				if (tx != 0 && (*dx2 & _DBL2ND))
				{
					*(temp1 - 1) = PSP;
					*(dx1 - 1) = _PS;
					temp2--;
					dx2--;
				}
				win->_lastch[y] = win->_maxx - 1;
				if (win->_firstch[y] == _NOCHANGE
						|| win->_firstch[y] > x)
				{
					win->_firstch[y] = x;
				}
				x = 0;
				y++;
			}
			if (win->_cury + 1 < win->_maxy
				&& win->_dx[win->_cury][win->_curx] & _PS)
			{
				win->_cury += 1;
				win->_curx = 0;
			}
			break;
	}
	TDmove(win, win->_cury, win->_curx);
	return(OK);
}


/*
**  Insert a 'character' in the reverse (right-to-left) direction.
**  Second arg is a char pointer to match TDinsstr interface, even
**  though will only be passed single byte chars.
*/

i4
TDrinsstr(awin, string)
WINDOW	*awin;
char	*string;
{
	reg	WINDOW	*win = awin;
	reg	char	*temp1;
	reg	char	*temp2;
	reg	char	*end;
	reg	char	thold;
	reg	i4	x;
	reg	i4	y;
	reg	i4	newx;
	reg	u_char	*insstr;
		u_char	buf[3];
    
	buf[0] = '\0';
	buf[1] = '\0';
	buf[2] = '\0';
	CMcpychar(string, buf);
	insstr = buf;
	y = win->_cury;
	x = win->_curx;

	switch(*insstr)
	{
		case '\t':
			for (newx = x - (x & 07); x > newx; x--)
			{
				if (TDrinsstr(win, ERx(" ")) == FAIL)
					return(FAIL);
			}
			return(OK);

		case '\r':
		case '\n':
			for (; x >= 0; x--)
			{
				if (TDrinsstr(win, ERx(" ")) == FAIL)
					return(FAIL);
			}
			return(OK);

		default:
			while (y < win->_maxy)
			{
				end = &win->_y[y][x];
				temp1 = &win->_y[y][0];
				temp2 = temp1 + 1;
				thold = *temp1;
				while (temp1 < end)
				{
					*temp1++ = *temp2++;
				}
				*temp1 = *insstr;	/* single byte char */
				win->_firstch[y] = 0;
				if ((win->_lastch[y] == _NOCHANGE)
					|| (win->_lastch[y] < x))
				{
					win->_lastch[y] = x;
				}
				x = win->_maxx - 1;
				y++;
				*insstr = thold;
			}
			break;
	}
	TDrmove(win, win->_cury, win->_curx - 1);
	return(OK);
}
