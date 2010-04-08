/*
**	Copyright (c) 2004 Ingres Corporation
*/
# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<termdr.h> 
# include	<cm.h>
# include	<itline.h>
# include	"itdrloc.h"

/*
**  ITLINEWIN.C -- get string from backend and put into window structure.
**
**	ITlinewindow -- get string from backend and put into window structure
**			with convertion of internal characters for solid
**			lines (line drawing char set) to actual code.
**
**		Caveats:
**			This routine is only called from MTdraw for iquel.
**			It is assumed to be called with lines less than or
**			equal in length to COLS (_maxx), but may conceivably
**			be fooled, being called with lines containing
**			expanding characters (such as \t.)
**
**	Parameters:
**		win   -- WINDOW structure.
**		instr -- string for placement into window.
**		len   -- length of 'instr' string (exclude terminater '\0')
**
**	Returns:
**		OK    -- put all 'insrt' string to the window structure
**		FAIL  -- put only fragment of 'instr' string to the
**			 window structure and cut the overflow string.
**
**	History:
**		01/19/87(KY)	-- Created.
**		10-sep-87 (bruceb)
**			Changed from use of ERR to FAIL (for DG.)
**		27-may-88 (bruceb)	Fix for bug 2670
**			This routine only called by MTdraw() for
**			the full screen terminal monitor (iquel.)
**			and only with lines <= maxx in length.
**			Therefore, no longer account for lines longer
**			than maxx chars, and don't go to next line on
**			\n.  An exception to this is that \t can expand
**			the line past maxx.  Doesn't bother setting
**			_cury, _curx on return, since MTdraw does that
**			work for itself.  Basically, for now, just chop!
**		10-oct-88 (bruceb)
**			Grab line drawing characters from drchtbl array
**			rather than directly from QA, QB, .. QK variables--
**			which will not be set on many terminals.
**	04/20/91 (dkh) - Added special handling for the formfeed character
**			 (gets turned into a blank) so that scrollable
**			 output for report writer will behave correctly.
**
**	Side Effects:
**		Changes data on structure win
**
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

ITlinewindow(win, string)
WINDOW	*win;
u_char	*string;
{
	reg	i4	x;
	reg	i4	y;
	reg	char	*cp;
	reg	i4	*firstch;
	reg	char	*addstr = (char *) string;

	x = win->_curx;
	y = win->_cury;
	if (y >= win->_maxy || x >= win->_maxx || y < 0 || x < 0)
		return(FAIL);
	firstch = &(win->_firstch[y]);
	cp = &(win->_y[y][x]);
	for (; *addstr; CMnext(addstr))
	{
		switch (*addstr)
		{
			case '\t':
			{
				reg	i4	newx;

				win->_curx = x;
				win->_cury = y;
				for (newx = x + (8 - (x & 07)); x < newx; x++)
				{
					if (TDadch(win, ' ') == FAIL)
					{
						return(FAIL);
					}
				}
				x = win->_curx;
				y = win->_cury;
				cp = &(win->_y[y][x]);
				break;
		  	}

			case DRCH_LR:
				*addstr = *drchtbl[DRQA].drchar;
				goto linedraw;

			case DRCH_UR:
				*addstr = *drchtbl[DRQB].drchar;
				goto linedraw;

			case DRCH_UL:
				*addstr = *drchtbl[DRQC].drchar;
				goto linedraw;

			case DRCH_LL:
				*addstr = *drchtbl[DRQD].drchar;
				goto linedraw;

			case DRCH_X:
				*addstr = *drchtbl[DRQE].drchar;
				goto linedraw;

			case DRCH_H:
				*addstr = *drchtbl[DRQF].drchar;
				goto linedraw;

			case DRCH_LT:
				*addstr = *drchtbl[DRQG].drchar;
				goto linedraw;

			case DRCH_RT:
				*addstr = *drchtbl[DRQH].drchar;
				goto linedraw;

			case DRCH_BT:
				*addstr = *drchtbl[DRQI].drchar;
				goto linedraw;

			case DRCH_TT:
				*addstr = *drchtbl[DRQJ].drchar;
				goto linedraw;

			case DRCH_V:
				*addstr = *drchtbl[DRQK].drchar;
			linedraw:
				win->_da[y][x] = _LINEDRAW;
			    /* go through below process */

			default:
				/*
				**  Change formfeeds into blanks first.
				*/
				if (*addstr == '\f')
				{
					*addstr = ' ';
				}
				if (CMcmpcase(cp,addstr))
				{
					if (*firstch == _NOCHANGE)
						*firstch = win->_lastch[y] = x;
					else if (x < *firstch)
						*firstch = x;
					else if (x > win->_lastch[y])
						win->_lastch[y] = x;
				}
				CMbyteinc(x, addstr);
				if (x <= win->_maxx)
				{
					i4	cnt;

					cnt = CMbytecnt(addstr);
					CMcpychar(addstr, cp);
					win->_dx[y][x-cnt] = '\0';
					if (cnt > 1)
					{
						win->_dx[y][x-1] = _DBL2ND;
					}
					CMnext(cp);
				}
				if (x >= win->_maxx)
				{
					if (x > win->_maxx)
					/* Double-byte char */
					{
						win->_lastch[y] += 1;

						/*
						** Same psuedo-char as MTdraw
						** rather than phantom space.
						*/
						*cp = '%';
					}
					/* If full screen used, return */
					return(OK);
				}
				break;

			case '\n':
				win->_curx = x;
				win->_cury = y;
				TDclreol(win);
				/* fall through */
			case '\r':
				x = 0;
				cp = &(win->_y[y][0]);
				break;
		}
	}
	return(OK);
}
