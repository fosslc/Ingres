# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<termdr.h>
# include	<st.h>
# include	<cm.h>
# include	<fmt.h>

/*
**  This routine adds the character to the current position.
**
**  Copyright (c) 2004 Ingres Corporation
**
**  History:
**	03/04/87 (dkh) - Added support for ADTs.
**	21-apr-87 (bruceb)
**		Added support for right-to-left data entry.
**		Includes 'reverse' code in TDaddhdlr, and addition
**		of reverse routines, TDradch, TDraddstr.
**	05/01/87 (dkh) - Put in fixes to handle single character windows.
**	10-sep-87 (bruceb)
**		Changed from use of ERR to FAIL (for DG.)
**	07-oct-87 (bruceb)
**		Parenthesize the ?: statement since DG's compiler can't
**		handle assignment statements within the ?: stmt.
**	12/23/87 (dkh) - Performance changes.
**	10/22/88 (dkh) - More perf changes.
**	11/10/88 (dkh) - Temporarily turn off DOUBLEBYTE.
**	02/21/89 (dkh) - Added include of cm.h and cleaned up ifdefs.
**	05/12/89 (dkh) - Fixed bug 5069.
**	07/27/89 (dkh) - Added check to turn F_BYTE1_MARKER into
**			 a phantom space.
**	01-dec-89 (bruceb)
**		Fix for bug:  allow characters to be added to last column
**		of each row of reversed fields.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/


# define	TDCHADD		(i4)0
# define	TDSTRADD	(i4)1

/*
**  TDaddhdlr -- This routine adds the character to the current position.
**
**	TDadch() and TDaddstr() are defined below
**	as using this routine.
**
**  Parameters:
**	win  -- Window for handling.
**	type -- TDCHADD to insert a single byte character - 'c'.
**		TDSTRADD to insert string - 'string'.
**	count	Number of bytes to insert.
**	c    -- Character to insert.
**	string -- Pointer of string to insert.
**	reverse -- Indicates if character(s) to be added left-to-right
**		   or right-to-left.
**
**  Returns:
**	NONE
**
**  Side Effects:
**	Changes data on structure win
**
**  History:
**	mm/dd/yy (RTI) -- Created.
**	09/18/86 (KY) -- Added CM.h for Double Byte character handling.
**	05/01/87 (dkh) - Put in fixes to handle single character windows.
**	08/18/89 (kathryn) -- 5069 changed phantom space to be PSP rather
**			than blank.
*/

GLOBALREF	char	PSP;

i4
TDaddhdlr(win, type, count, c, string, reverse)
reg	WINDOW	*win;
i4		type;
i4		count;
u_char		c;
char		*string;
bool		reverse;
{
	reg	i4	x;
	reg	i4	y;
	reg	i4	i;
	reg	char	*cp;
	reg	i4	*firstch;
	reg	i4	*lastch;
	reg	i4	maxx;
	reg	i4	inc = 0;
	reg	u_char	*addstr = (u_char *) string;
		u_char	buf[3];
		i4	startpos = 0;
		i4	cnt;
		i4	isphantom;


/*
	buf[0] = '\0';
	buf[1] = '\0';
	buf[2] = '\0';
*/
	maxx = win->_maxx;

	if (reverse)
		startpos = maxx - 1;
	x = win->_curx;
	y = win->_cury;
	if (y >= win->_maxy || x >= maxx || y < 0 || x < 0)
		return(FAIL);
	if (type == TDCHADD)
	{
		addstr = buf;
		*addstr++ = c;
		*addstr = '\0';
		addstr = buf;
	}
	firstch = &(win->_firstch[y]);
	lastch = &(win->_lastch[y]);
	cp = &(win->_y[y][x]);
	for (i = 0; i < count; )
	{
		switch (*addstr)
		{
			case '\t':
			{
				reg	i4	newx;

				win->_curx = x;
				win->_cury = y;
				if (reverse)
				{
					for (newx = x - (x & 07); x > newx; x--)
					{
						if (TDradch(win, ' ') == FAIL)
						{
							return(FAIL);
						}
					}
				}
				else
				{
					for (newx = x + (8 - (x & 07));
								x < newx; x++)
					{
						if (TDadch(win, ' ') == FAIL)
						{
							return(FAIL);
						}
					}
				}
				x = win->_curx;
				y = win->_cury;
				cp = &(win->_y[y][x]);
				break;
		  	}

			default:
				if (*addstr == F_BYTE1_MARKER)
				{
					isphantom = TRUE;
					*addstr = PSP;
				}
				else
				{
					isphantom = FALSE;
				}
				if (CMcmpcase(cp,addstr))
				{
					if (*firstch == _NOCHANGE)
						*firstch = *lastch = x;
					else if (x < *firstch)
						*firstch = x;
					else if (x > *lastch)
						*lastch = x;
				}
				if (reverse)
				{
					/*
					** using x-- rather than something
					** like CMbytedec since this code
					** is designed for 1-byte char sets.
					*/
					x--;
				}
				else
				{
					CMbyteinc(x, addstr);
				}
				/*
				** x can be negative only if reverse,
				** and >= _maxx only if !reverse.
				*/
				if (0 <= x && x < maxx)
				{
					cnt = CMbytecnt(addstr);
					CMcpychar(addstr, cp);
					if (reverse)
					{
						if (isphantom)
						{
							win->_dx[y][x+1] = _PS;
						}
						else
						{
							win->_dx[y][x+1] = '\0';
						}
						/*
						** cp-- since cp points
						** directly into the window,
						** and chars are being added
						** to the window from right
						** to left.
						*/
						cp--;
					}
					else
					{
# ifdef DOUBLEBYTE
					    if (isphantom)
					    {
						win->_dx[y][x-1] = _PS;
					    }
					    else
					    {
						win->_dx[y][x-cnt] = '\0';
						if (cnt > 1)
						{
						    win->_dx[y][x-1] = _DBL2ND;
						}
					    }
# endif /* DOUBLEBYTE */
					    CMnext(cp);
					}
				}
				else
				{
					/*
					** gets here if x >= _maxx (and
					** !reverse), or x < 0 (and reverse)
					*/
					if (x > maxx)
					{
						*lastch += 1;
						win->_dx[y][x-2] = _PS;
						*cp = PSP;
						if (y+1 < win->_maxy)
						{
							inc = 1;
						}
					}
					else if (x == maxx)
					{
						/*
						**  Things will just fit.
						*/
						cnt = CMbytecnt(addstr);
						CMcpychar(addstr, cp);
# ifdef DOUBLEBYTE
						if (isphantom)
						{
						    win->_dx[y][x-1] = _PS;
						}
						else
						{
						    win->_dx[y][x-cnt] = '\0';
						    if (cnt > 1)
						    {
							win->_dx[y][x-1] =
								_DBL2ND;
						    }
						}
# endif /* DOUBLEBYTE */
					}
					else /* x < 0  &&  reverse */
					{
						CMcpychar(addstr, cp);
						win->_dx[y][0] = '\0';
					}
					x = startpos;
newline:
					if (++y >= win->_maxy)
					{
						if (reverse)
						{
							win->_curx = 0;
						}
						else
						{
							/*
							**  If only a one
							**  character field no
							**  chance of putting
							**  two-byte chars,
							**  such as Kanji, into
							**  the field.
							*/
							if (maxx == 1)
							{
								win->_curx = maxx - 1;
							}
							else
							{
								win->_curx = maxx - CMbytecnt(&(win->_y[y-1][maxx-2]));
							}
						}
						return(FAIL);
					}

					firstch = &(win->_firstch[y]);
					lastch = &(win->_lastch[y]);
					cp = &(win->_y[y][x]);
				}
				break;
			case '\r':
			case '\n':
				win->_curx = x;
				win->_cury = y;
				if (reverse)
					TDrclreol(win);
				else
					TDclreol(win);
				if (!NONL)
					x = startpos;
				goto newline;
		}
		if (inc)
		{
			inc = 0;
		}
		else
		{
			CMbyteinc(i, addstr);
			CMnext(addstr);
		}
	}
	win->_curx = x;
	win->_cury = y;
	return(OK);
}



i4
TDadch(win, c)
WINDOW	*win;
char	c;
{
	return(TDaddhdlr(win, TDCHADD, 1, (u_char) c, NULL, FALSE));
}


i4
TDradch(win, c)
WINDOW	*win;
char	c;
{
	return(TDaddhdlr(win, TDCHADD, 1, (u_char) c, NULL, TRUE));
}


i4
TDaddstr(win, str)
WINDOW	*win;
char	*str;
{
	return(TDaddhdlr(win, TDSTRADD, STlength(str), '\0', str, FALSE));
}


i4
TDraddstr(win, str)
WINDOW	*win;
char	*str;
{
	return(TDaddhdlr(win, TDSTRADD, STlength(str), '\0', str, TRUE));
}


i4
TDxaddstr(win, count, str)
WINDOW	*win;
i4	count;
u_char	*str;
{
	return(TDaddhdlr(win, TDSTRADD, count, '\0', (char *) str, FALSE));
}
