/*
**  totop.c
**
**  Copyright (c) 2004 Ingres Corporation
**
**  History:
**	19-jun-87 (bab)	Code cleanup.
**	08/14/87 (dkh) - ER changes.
**	01-oct-87 (bab)
**		Code added to handle rows on the terminal being
**		made unavailable to the forms program.
**	04/13/88 (dkh) - Fixed jup bug 584.
**	15-apr-88 (bruceb)	Further 584 changes.
**	2-mar-87 (bab)	Fix for bug 11628
**		TDscrllt() changed to set diff to
**		(datawin->_begx + datawin->_curx) instead of just _begx.
**		This allows wide fields to scroll-left in a manner comparable
**		to scroll-right.  This does, however, change the behavior
**		when scrolling within narrower fields; specifically,
**		if the cursor isn't in the first column of the field when
**		scroll-left is typed, the scroll will position the column with
**		the cursor at the left of the screen rather than with the first
**		column of the field at the edge of the screen.
**		Also changed TDscrlrt() a little so as not to first
**		subtract curscr->_begx and then to add it back in.
**	08/15/90 (dkh) - Fixed bug 21670.
**	08/28/90 (dkh) - Integrated porting change ingres6202p/131215.
**	10/02/90 (dkh) - Fixed bug 33605.
**	11/08/90 (dkh) - Fixed bug 16627.
**	08/20/93 (dkh) - Fixing NT compile problems.  Changed _leave
**			 to lvcursor for the WINDOW structure.
**	14-mar-95 (peeje01) Cross integration of 6500db_su4_us42 changes
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	28-Mar-2005 (lakvi01)
**	    Corrected function prototypes.
*/

# include	<compat.h>
# include	<me.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ftdefs.h>
# include	<si.h>
# include	<termdr.h> 

FUNC_EXTERN	i4	TDrefresh();

VOID
TDutil(frame, datawin, y, x, buf)
WINDOW	*frame;
WINDOW	*datawin;
i4	y;
i4	x;
char	*buf;
{
# ifdef DOUBLEBYTE
	char    dx_buf[MAX_TERM_SIZE + 1];
# endif

	MEcopy(curscr->_y[LINES - 1], (u_i2)COLS, buf);
# ifdef DOUBLEBYTE
	MEcopy(curscr->_dx[LINES - 1], (u_i2)COLS, dx_buf);
# endif
	TDclear(curscr);
	MEcopy(buf, (u_i2)COLS, curscr->_y[LINES - 1]);
# ifdef DOUBLEBYTE
	MEcopy(dx_buf, (u_i2)COLS, curscr->_dx[LINES - 1]);
# endif
	TDrefresh(curscr);
	TDtouchwin(frame);
	curscr->_begy = y;
	curscr->_begx = x;

	/*
	**  Fix for BUG 7813. (dkh)
	**  Needed since TDclear will call TDerase which sets
	**  cury, curx to zero.  The next statements will match
	**  everything up with physical cursor on terminal.
	*/
	curscr->_cury = -y;
	curscr->_curx = -x;

	TDrefresh(frame);
	if (datawin != NULL)
	{
		TDrefresh(datawin);
	}
}

VOID
TDscrlup(frame, datawin)
WINDOW	*frame;
WINDOW	*datawin;
{
	i4	diff;
	bool	leaveflg;

	/*
	**  Only can move if the data window does not begin on
	**  the first line of the form.
	*/

	if ((diff = datawin->_begy) > -curscr->_begy)
	{
		/*
		**  If we are going to be greater than
		**  the form window then cut back till
		**  it fits.  Can only move LINES - 2
		**  number of lines to account for the
		**  menu line and to leave the current
		**  line still on the screen.
		**  Zero indexing also comes into play.
		*/

		if (LINES - 2 - frame->_starty + diff > frame->_maxy - 1)
		{
			diff = frame->_maxy - 1 - (LINES - 2 - frame->_starty);
		}
		diff = -diff;

		/*
		**  Original test in next if statement was "!=", it is
		**  now "<".
		**  Analog of fix for BUG 7514 applied to scrolling
		**  up a form. (dkh)
		*/
		if (diff < curscr->_begy)
		{
			frame->_flags |= _DONTCTR;
			datawin->_flags |= _DONTCTR;
			frame->_cury = LINES - 2 - frame->_starty - diff;
			frame->_curx = datawin->_begx + datawin->_curx +
				frame->_startx;
			leaveflg = frame->lvcursor;
			frame->lvcursor = FALSE;
			TDrefresh(frame);
			frame->lvcursor = leaveflg;
			TDrefresh(datawin);
			frame->_flags &= ~_DONTCTR;
			datawin->_flags &= ~_DONTCTR;
		}
	}
}

/*
** Scroll is <= (num lines on screen - menu_line - num lines unused
** at top of screen - one line for visual continuity).
**
** Cursor is placed on the last row of the form or the usable
** portion of the screen (see above), whichever comes first.
*/
VOID
TDupvfsl(win, lastline)
WINDOW	*win;
i4	lastline;
{
	i4	diff;

	/*
	** diff = line number of last line on usable screen,
	**         + max number of lines that can be scrolled.
	*/
	diff = (LINES - 2 - IITDflu_firstlineused) - curscr->_begy
		+ (LINES - 2 - IITDflu_firstlineused);
	if (diff > lastline)
	{
		diff = lastline;
	}
	if (diff != win->_cury)
	{
		win->_cury = diff;
		win->_flags |= _DONTCTR;
		TDrefresh(win);
		win->_flags &= ~_DONTCTR;
	}
}

VOID
TDscrldn(frame, datawin)
WINDOW	*frame;
WINDOW	*datawin;
{
	i4	diff;
	bool	leaveflg;

	if (curscr->_begy < 0)
	{
		/*
		**  Possibility a scroll can happen.
		**  First figure out how many lines we have to move.
		**  We subtract 2 from LINES due to one line for
		**  the menu and we want the current line to remain
		**  on the screen.  Therefore, for a 24 line terminal
		**  we can only move at most, 22 lines (24-2).
		**  Zero indexing also comes into play.
		*/
		diff = LINES - 2 -
			(datawin->_begy + frame->_starty +
			datawin->_cury + curscr->_begy);
		if ((diff += curscr->_begy) > 0)
		{
			diff = 0;
		}
		if (diff != curscr->_begy)
		{
			frame->_flags |= _DONTCTR;
			datawin->_flags |= _DONTCTR;
			/*
			**  WARNING: DIFF SHOULD ONLY BE ZERO
			**  OR NEGATIVE.
			*/
			frame->_cury = (diff == 0 ? diff : -diff);
			frame->_curx = datawin->_begx + datawin->_curx +
				frame->_startx;
			leaveflg = frame->lvcursor;
			frame->lvcursor = FALSE;
			TDrefresh(frame);
			frame->lvcursor = leaveflg;
			TDrefresh(datawin);
			frame->_flags &= ~_DONTCTR;
			datawin->_flags &= ~_DONTCTR;
		}
	}
}


/*
** Scroll is <= (num lines on screen - menu_line - num lines unused
** at top of screen - one line for visual continuity).
**
** If any scroll occurs, cursor is placed on the first row of the form
** or the usable portion of the screen (see above), whichever comes first.
*/
VOID
TDdnvfsl(win)
WINDOW	*win;
{
	i4	diff;

	if (curscr->_begy < 0)
	{
		diff = -curscr->_begy - (LINES - 2 - IITDflu_firstlineused);
		if ((win->_cury = diff) < 0)
		{
			win->_cury = 0;
		}
		win->_flags |= _DONTCTR;
		TDrefresh(win);
		win->_flags &= ~_DONTCTR;
	}
}

VOID
TDscrllt(frame, datawin)
WINDOW	*frame;
WINDOW	*datawin;
{
	i4	diff;
	char	buf[MAX_TERM_SIZE + 1];

	/*
	**  Can only move if the data window does not begin
	**  at the left edge of the form.
	*/

	if ((diff = (datawin->_begx + datawin->_curx)) != 0)
	{
		/*
		**  If scrolling will cause us to exceed
		**  boundaries of form then bring it back.
		*/
		if (COLS - 1 + diff >= frame->_maxx - 1)
		{
			diff = frame->_maxx - 1 - (COLS - 1);
		}
		diff = -diff;

		/*
		**  The following statement was changed from a "!=" test
		**  to a "<" test.  This is necessary since we only want
		**  to redraw if we are going more negative (i.e., scrolling
		**  the form to the left.  The value of "diff" may be less
		**  than curscr->_begx due to the fact that we may have a
		**  very wide field and that the beginning of the field has
		**  long since scroll out of the way.
		**  Fix for BUG 7514. (dkh)
		*/
		if (diff < curscr->_begx)
		{
			frame->_flags |= _DONTCTR;
			datawin->_flags |= _DONTCTR;
			TDutil(frame, datawin, curscr->_begy, diff, buf);
			frame->_flags &= ~_DONTCTR;
			datawin->_flags &= ~_DONTCTR;
		}
	}
}

STATUS
TDltvfsl(win, lastcol, expand)
WINDOW	*win;
i4	lastcol;
bool	expand;
{
	i4	diff;
	char	buf[MAX_TERM_SIZE + 1];

	diff = COLS - 1 - (win->_curx + curscr->_begx);
	diff += COLS - 1;
	if (diff + win->_curx >= lastcol)
	{
		if (expand)
		{
			win->_curx = lastcol + 1;
			return(FALSE);
		}
		diff = lastcol - win->_curx;
	}
	if (diff != 0)
	{
		win->_curx += diff;
		diff = -win->_curx + COLS - 1;
		if (diff != curscr->_begx)
		{
			TDutil(win, NULL, curscr->_begy, diff, buf);
		}
	}
	return(TRUE);
}

VOID
TDscrlrt(frame, datawin)
WINDOW	*frame;
WINDOW	*datawin;
{
	i4	diff;
	char	buf[MAX_TERM_SIZE + 1];

	if (curscr->_begx < 0)
	{
		/*
		**  Possibility a scroll can happen.
		*/
		diff = COLS - 1 -
			(datawin->_begx + datawin->_curx);
		if (diff > 0)
		{
			diff = 0;
		}
		if (diff != curscr->_begx)
		{
			frame->_flags |= _DONTCTR;
			datawin->_flags |= _DONTCTR;
			TDutil(frame, datawin, curscr->_begy, diff, buf);
			frame->_flags &= ~_DONTCTR;
			datawin->_flags &= ~_DONTCTR;
		}
	}
}

VOID
TDrtvfsl(win)
WINDOW	*win;
{
	i4	diff;
	char	buf[MAX_TERM_SIZE + 1];

	if (curscr->_begx < 0)
	{
		diff = COLS - 1 + curscr->_begx;
		if (diff > 0)
		{
			win->_curx = diff = 0;
		}
		else
		{
			win->_curx = -diff;
		}
		if (diff != curscr->_begx)
		{
			TDutil(win, NULL, curscr->_begy, diff, buf);
		}
	}
}
