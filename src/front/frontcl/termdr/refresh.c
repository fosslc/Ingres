/*
**  Copyright (c) 2004 Ingres Corporation
*/
# include	<compat.h>
# include	<st.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ftdefs.h>
# include	<me.h>
# include	<si.h>
# include	<te.h>
# include	<ex.h>
# include	<cm.h>
# include	<er.h>
# include	"it.h"
# include	<termdr.h>

/*
**  Make the current screen look like "win" over the area covered by
**  win.
**
**  History:
**	modified 6/10/82 (jrc) 	To fix problems when refreshing the current
**				screen.
**	8/27/86 (bruceb)
**		handle transfer between two forms via display submenu
**		where display attributes are last thing on line, ...
**		see TDmakech().
**	09/17/86(KY) -- added CM library
**	12/03/86(KY) -- added for handling double Bytes attributes (_dx)
**	19-jun-87 (bruceb)	Code cleanup.
**	08/14/87 (dkh) - ER changes.
**	10-sep-87 (bruceb)
**		Changed from use of ERR to FAIL (for DG.)
**	24-sep-87 (bruceb)
**		Lots of (mostly cosmetic) changes intended to improve
**		legibility of the file.
**	01-oct-87 (bruceb)
**		Take into account the _starty WINDOW structure member when
**		placing the cursor or text onto the screen.  This is so
**		that the top line of the screen can be reserved from (made
**		unusable by) the forms system.  Currently used to allow
**		display of the DG CEO status line.  When this is made more
**		general, allowing for reserving multiple lines, the
**		TDclrTop code will need rewriting to deal with more than
**		just the top line.  As it is, TDclrTop merely blanks
**		the top line to prevent placement of garbage.
**
**		Note: _startx is NOT being used at this time, so reserving
**		columns at the left side of the screen will not work.
**	02-oct-87 (bruceb)
**		Added calls on TElock.
**	05-oct-87 (bruceb)
**		Added inclrtop static to enable clearing top line of the
**		terminal.  Otherwise, top line not cleared since the blanks
**		attempting to place on the screen are equivalent to the
**		comparison character (a space char) in TDmakech.
**	12/23/87 (dkh) - Performance changes.
**	02/24/88 (dkh) - Added fixes for 5.0 bugs 14224 and 14366.
**	12-apr-88 (bruceb)
**		In TDrefresh, reset lx on return from TDscroll if 'xl'
**	05/03/88 (dkh) - Venus changes.
**	05/09/88 (dkh) - Cleaned up Venus code.
**	04/26/88 (dkh) - Added call to TDboxfix for box/line support.
**	05/28/88 (dkh) - Integrated Tom Bishop's Venus code changes.
**	27-jul-88 (bruceb)	Fix for bug 3040.
**		If end of form (both x and y) would be off the edge of the
**		screen, recalculate cscr->_beg[yx] so that the form-end is
**		right on the screen edge.
**	09/03/88 (dkh) - Fixed venus bug 3203.
**	10/22/88 (dkh) - Performance changes.
**	02/21/89 (dkh) - Added include of cm.h and cleaned up ifdefs.
**	09/15/89 (dkh) - Fixed bug 7488.
**	09/22/89 (dkh) - Porting changes integration.
**	09/28/89 (dkh) - Added patches to better handle displaying big
**			 forms for the first time.
**	11/15/89 (dkh) - Put in change to make submenus in Vifred
**			 display correctly when editing a form that
**			 has scrolled vertically.
**	19-jan-90 (brett) - Moved the calls ti IITDxoff and IITDyoff to the end
**			    of TDrefresh().  Added 2 static variables to reduce
**			    the number of calls to the routines.  THis location
**			    of xoff and yoff will guarantee things work.
**	01/24/90 (dkh) - Integrated above change (ingres6202p #130401) into
**			 6.3 code line.
**	03/28/90 (dkh) - Fixed bug 20604 (calculate correct cursor position
**			 for a form that has scrolled horizontally displayed
**			 in a window that has _leave set).
**	31-may-90 (rog)
**		Included <me.h> so MEfill() is defined correctly.
**	08/15/90 (dkh) - Fixed bug 21670.
**	08/28/90 (dkh) - Integrated porting change ingres6202p/131215.
**	08/30/90 (dkh) - Integrated various porting change to support
**			 HP terminals in HP mode.
**	10/02/90 (dkh) - Fixed bug 33605.
**	03/24/91 (dkh) - Integrated changes from PC group.
**	08/19/91 (dkh) - Integrated changes from Pc group.
**	19-dec-91 (leighb) DeskTop Porting Change:
**		Added return of 1st chararcter's attribute to TDsaveLast();
**		Added 2nd argument to TDaddLast() to indicate attribute to
**		set entire last line to; made corresponding changes to callers
**		of TDsaveLast() & TDaddLast().
**	26-may-92 (leighb) DeskTop Porting Change:
**		Changed PMFE ifdefs to MSDOS ifdefs to ease OS/2 port.
**		Increased IIcurscr_firstch to support 200 lines.
**	02-feb-93 (johnr) 
**		hp70092 termcap specific change to correct erroneous cursor
**		positioning for field data types and reverse video.
**	14-Mar-93 (fredb) hp9_mpe
**		Integrated 6.2/03p change:
**      	16-Nov-90 (fredb)
**              Fixed vertical (y relative) scrolling bug.  TDscroll uses
**              \n's to scroll the screen up which on some platforms sends
**              the cursor to column 0.  The adjustment was being made only
**              if 'XL' is TRUE.  It should be (XL || !NONL), at least until
**              we find out what the 'xl' termcap really means.
**	07/16/93 (dkh) - Fixed bug 43406.  Adjusted the check for left most
**			 blank character without attributes on a line.  This
**			 allows the use of the CE (clear to end of line)
**			 terminal attribute to start at the correct location.
**	08/20/93 (dkh) - Fixing NT compile problems.  Changed _leave
**			 to lvcursor for the WINDOW structure.
**	28-mar-95 (peeje01)
**	    Cross Integration of doublebyte changes from label: 6500db_su4_us42
**      14-Jun-95 (fanra01)
**              Add NT_GENERIC to all MSDOS sections which relate to screen
**              updates.
**              Also add section to get screen location. Lifted from 6.4.
**	24-sep-95 (tutto01)
**		Removed obsolete call to TEscreenloc.
**	04-apr-95 (tsale01)
**		Fix problem with visual tools (qbf, abf etc) where 2nd byte
**		of double byte character occurs as the 1st byte on left edge
**		of screen the display is wrong. This is due to 2 places which
**		duplicate each others effort. Took out later code.
**      24-sep-96 (mcgem01)
**              Global data moved to termdrdata.c
**      31-may-2001 (horda03) Bug 104819
**              Display a space if a popup menu being displayed overlays a 
**              dbl-byte character such that only the second byte would be
**              displayed.
**      19-sep-2001 (horda03) Bug 104819
**              Above attempt to resolve this problem, didn't handle the
**              case where the screen is being refreshed.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	05-aug-2002 (abbjo03)
**	    Fix scrolldown operation on platforms where CAS is NULL.
**      01-aug-2006 (huazh01)
**          On TDmakech(), always calls TDdaout() and write out attribute 
**          info if it is a HP terminal on HP mode. This fixes 116440.
**	26-Aug-2009 (kschendel) b121804
**	    Remove function defns now in headers.
**      12-nov-2009 (huazh01)
**          On TDmakech(), instead of using current display attribute 'cda-1'
**          to update working attribute 'zda', we now use previous display 
**          attribute 'pda' to update 'zda'. (b122581)
**      24-nov-2009 (huazh01)
**          don't update current cursor position if it is a non printable
**          character and don't store it into display buffer (*csp++ = *nsp) 
**          as well. (b122948)
**      17-dec-2009 (coomi01) b123066
**          Reduce the output of cursor re-positioning sequences where direct
**          substring output is more efficient.
**      12-feb-2010 (huazh01)
**          For double byte installation on HP terminal, output display 
**          attribute if it is not the same as previous one.  (b123288)
**      16-feb-2010 (coomi01) b123294
**          Further reduction in cursor re-positioning sequences. This time
**          looking for those which apparently jump onto themselves. Rather
**          than compare co-ordinates before jumping call a wrapper TDsmvcur1()
**          routine to do it for us. Convert calls to this routine sparingly
**          for it may be legitimate to do a zero jump where we are asserting
**          our notion of cursor position over, say, H/W scrolling. 
**      22-Mar-2010 (coomi01) b123449
**          Backout previous two changes, they prejudiced operation of UTF8
**          installations. Changed routine TDfgoto() to compensate.
**      20-Apr-2010 (coomi01) b123602
**          Make DOUBLE-BYTE a runtime test and bring into line with
**          routine TDfgoto()
**      27-may-2010 (huazh01)
**          after finish making the change, on HP terminal (XS), output 
**          display attribute if current display attribute is not the same as 
**          previous on. (b123820)
**      28-jul-2010 (huazh01)
**          only apply the fix to b123820 if the previous display attribute
**          is reverse video. (b124090)
**      23-Aug-2010 (huazh01)
**          On HP-UX, clean up display attribute if previous frame has a reverse 
**          video field. (b124215)
**      19-nov-2010 (huazh01)
**          Ensure reverse video display attribute has been cleaned up properly 
**          by writing a 'RE' to output buffer.
**          (b124610)
*/
u_char		TDsaveLast();


void            TDsaveLast_dx();
void            TDaddLast_dx();

# ifndef min
# define		min(x,y)	(x < y ? x : y)
# define		max(x,y)	(x > y ? x : y)
# define		abs(x)		(x < 0 ? -x : x)
# endif

/*
**  ly and lx are the absolute cursor positions
*/


GLOBALREF i4	ly, lx;

GLOBALREF bool	_curwin;

GLOBALREF u_char	pfonts;
GLOBALREF u_char	pcolor;
GLOBALREF u_char	pda;

/*
** [HP terminals in HP mode only!]
** IITDcda_prev is used to keep track of the display enhancements
** used on the character to the left of where we are working
** now in the TDmakech function.
*/
GLOBALREF char		IITDcda_prev;

/*
** [HP terminals in HP mode only!]
** IITDptr_prev is used to look at the display enhancement(s) we
** we using prior to skipping a number of characters that are
** the same on the screen as what we want to write and have the
** same display enhancements. We need to decide if anything
** needs to be turned off before we move the cursor. IITDptr_prev
** saves the value of the cda pointer. It is used in the
** TDsmvcur and TDmakech functions.
*/
GLOBALREF char		*IITDptr_prev;

GLOBALREF WINDOW	*_win;
GLOBALREF WINDOW	*_pwin;	/* points to the top parent window */

GLOBALREF i2	depth;

GLOBALREF bool	IITDfcFromClear;

static	bool	inclrtop = FALSE;
static	i4	oldbegx = 0;
static	i4	oldbegy = 0;

# if defined(MSDOS) || defined(NT_GENERIC)
GLOBALREF  i4	IIcurscr_firstch[200];	/* 1st char changed on nth line */
# endif /* MSDOS */

/*
** [Used for HP Terminal support.]
** Static to keep track of clear to end line status. Cleared in TDrefresh,
** set in TDmakech, and tested in TDrefresh. Static pointer to WINDOW
** structure used to save what we cleared outside our subwindow.
*/

static	bool	clr_eol = FALSE;
static	WINDOW	*xs_savewin = NULL;

IITDctsClearToScroll(win, lastline, lastline_dx)
WINDOW	*win;
char	*lastline;
char	*lastline_dx;
{
	u_char	saveattr;

	void    TDsaveLast_dx();
	void    TDaddLast_dx();

	if (!(win->_flags & _TDSPDRAW))
	{
	    if (CMGETDBL)
	    {
		TDsaveLast_dx(lastline_dx);
	    }
	    saveattr = TDsaveLast(lastline);
	    TDerase(curscr);
	    TDrefresh(curscr);
	}
	TDtouchwin(win);
	TDrefresh(win);
	if (!(win->_flags & _TDSPDRAW))
	{

	    if (CMGETDBL)
	    {
		TDaddLast_dx(lastline_dx);
	    }

	    TDaddLast(lastline, saveattr);
	    TDclrTop();
	}
}



i4
TDrefresh(awin)
WINDOW	*awin;
{
	reg	WINDOW	*win = awin;
	reg	WINDOW	*cscr = curscr;
	reg	i4	wy;		/* a loop index */
	reg	i4	retval;
			/*
			** The beginning and ending y for the window's portion
			** which is on the screen
			*/
	reg	i4	wyend;
	reg	i4	wybeg;
	reg	i4	presy;
	reg	i4	presx;
	reg	i4	offset;	/* number lines or columns scrolled */
	reg	i4	absx;
	reg	i4	absy;
	reg	i4	lines;	/* first line just off the screen */
	reg	i4	cols;	/* first column just off the screen */
	reg	i4	tbegx;
	reg	i4	tbegy;
	reg	i4	ocury;
	reg	i4	ocurx;
	reg	i4	xcnt;
	reg	WINDOW	*twin;
	reg	bool	change = FALSE;		/* true if change made */
	reg	char	*cp;
		i4	heightadj;
		i4	widthadj;
		bool	dont_center = FALSE;
		bool	onmuline = FALSE;
		char	lastLine[MAX_TERM_SIZE + 1];
		char	lastLine_dx[MAX_TERM_SIZE + 1];
		char	scp[20];
		char	ucp[20];
		char	outbuf[TDBUFSIZE + 1];
		char	xbuf[3072 + 1];
		u_char	saveLattr;	/* save attribute of last line */

# if defined(MSDOS) || defined(NT_GENERIC)
	/*
	** Clear 1st-char-changed on each line.
	** Notify TE that refresh is beginning;
	** TE will not write anything until refresh finished;
	** When TEerfresh() is called, it will update video RAM directly.
	** Dave - PC group should get these new TE routines into CL spec.
	*/
	for (wy = LINES; wy--; )
		IIcurscr_firstch[wy] = IITDfcFromClear ? 0 : 200;
	TEbrefresh();
# endif /* MSDOS */

	/*
	**  [HP terminals in HP mode only]
	**  clear flag used to determine if we should rebuild
	**  the screen image after a refresh and all variables
	**  used to keep track of display enhancement status
	*/

	if (XS)		/* HP terminals only */
	{
		clr_eol = FALSE;
		IITDcda_prev = EOS;
		IITDptr_prev = NULL;
		pda = EOS;
		pfonts = EOS;
		pcolor = EOS;
	}

	if (win->_flags & _DONTCTR)
	{
		dont_center = TRUE;
	}

	if (dont_center)
	{
		heightadj = LINES;
		widthadj = COLS/4;
	}
	else
	{
		heightadj = LINES/2;
		widthadj = COLS/2;
	}

	/*
	**  fixup spoke representation of box characters into
	**  the terminal's line drawing characters (if necessary)
	*/
	if (win->_flags & _BOXFIX)
	{
		TDboxfix(win);
	}

	/*
	** make sure were in visual state
	*/
	if (depth == 0)
	{
		TDobptr = outbuf;
		TDoutbuf = outbuf;
		TDoutcount = TDBUFSIZE;
# ifdef DGC_AOS
		TElock(TRUE, (i4) 0, (i4) 0);
# endif /* DGC_AOS */
	}
	depth++;
/*
**  Code taken out since condition should never occur. (dkh)
**
**	if (_endwin)
**	{
**		_puts(VS);
**		_puts(TI);
**		if (LD != NULL && !XD)
**		{
**			_puts(LD);
**		}
**		_endwin = FALSE;
**	}
*/

	/*
	**  Needed when refreshing last line of screen (i.e., menuline)
	**  but the form has scrolled horizontally. (dkh)
	*/

	if (win->_begy == LINES - 1)
	{
		/*
		**  Fix for BUG 8408. (dkh)
		*/
		WINDOW	*mwin;

		for (mwin = win; mwin->_flags & _SUBWIN; mwin = mwin->_parent)
		{
		}

		if (mwin->_begy == LINES - 1)
		{
			onmuline = TRUE;
			/*
			**  Next line part of fix for BUG 7165. (dkh)
			*/
			cscr->_curx += cscr->_begx;
			cscr->_cury += cscr->_begy;

			tbegx = cscr->_begx;
			cscr->_begx = 0;
			tbegy = cscr->_begy;
			cscr->_begy = 0;
		}
	}
	/*
	** check to see if window is to be relatively or
	** or absolutely displayed
	*/
	if (!win->_relative)
	{
		/*
		** Convert the absolute window coordinates to
		** relative coordinates.  The absolute coordinates are
		** saved so they may put back in the window
		*/
		absy = win->_begy;
		absx = win->_begx;
		win->_begy = absy - cscr->_begy;
		win->_begx = absx - cscr->_begx;
	}
	/*
	** initialize loop parameters
	** ly and lx are the absolute screen position of the cursor.
	*/
	if (cscr->_cury + cscr->_begy >= 0)
		ly = cscr->_cury + cscr->_begy;
	if (curscr->_curx + curscr->_begx >= 0)
		lx = cscr->_curx + cscr->_begx;
	wy = 0;
	_win = win;
	/*
	** obtain pointer to the top parent window--needed for
	** referencing the parent's _starty member.
	*/
	for (_pwin=win; _pwin->_flags & _SUBWIN; _pwin=_pwin->_parent)
	{
	}
	_curwin = (win == curscr);

	if (_curwin)
	{
		ocury = cscr->_cury;
		ocurx = cscr->_curx;
	}
	/*
	** If the window is not the current screen, and leave is not set,
	** then we need to check if the window must scroll.
	** If the window scrolls, the current screen's coordinates are changed
	** to show the extent of the scroll.
	**	e.g.
	**		if the window scrolls 3 lines, the current screens
	**		beginning y is decremented by 3.
	**	The code maintains this
	**		curscr->_cury + curscr->_begy == absolute cursor
	*/
	if(!_curwin && !win->lvcursor)
	{
		/* check for horizontal scrolling */
		presx = win->_curx + win->_begx + cscr->_begx;
		/*
		** presy is the relative position the cursor should go to.
		*/
		presy = win->_cury + win->_begy + cscr->_begy;
		/* CAVEAT:  sideways scrolling doesn't use the _startx member */
		if ((presx >= COLS || presx < 0) && win->_relative)
		{
			/* do side-ways scrolling */
			twin = win;
			win = _win = _pwin;
			if (twin->_flags & _SUBWIN)
			{
				win->_curx = presx;
			}
			if (presx >= COLS)
			{
				offset = win->_begx + cscr->_begx;
				cols = max(0, (COLS)-offset);
				cscr->_begx = cscr->_begx -
					(presx - offset - cols) - widthadj;
				/* make sure end of form is edge of screen */
				if (cscr->_begx + win->_maxx < COLS)
					cscr->_begx = COLS - win->_maxx;
			}
			else
			{
				/*
				**  Fix for BUG 7765. (dkh)
				*/
				win->_curx = twin->_curx +
					twin->_begx - win->_begx;
				if (win->_curx + win->_begx < widthadj)
				{
					cscr->_begx = 0;
				}
				else
				{
					cscr->_begx = widthadj -
						(win->_curx + win->_begx);
				}
			}

			if (presy >= LINES - 1 - win->_starty)
			{
				if (twin->_flags & _SUBWIN)
				{
					win->_cury = presy;
				}
				offset = win->_begy + cscr->_begy;
				lines = max(0, (LINES - 1) - offset);
				cscr->_begy = cscr->_begy -
					(presy - offset - lines) - LINES/2;
				/* make sure end of form is end of screen */
				if (cscr->_begy + win->_maxy
					< (LINES - 1 - win->_starty))
				{
					cscr->_begy = (LINES - 1 - win->_starty)
						- win->_maxy;
				}
			}
			else if (presy < 0)
			{
				if (twin->_flags & _SUBWIN)
				{
					win->_cury = presy;
				}
				offset = win->_begy + cscr->_begy;
				lines = min(win->_maxy - 1, (-1 - offset));
				if (win->_cury + win->_begy < LINES/2)
				{
					cscr->_begy = 0;
				}
				else
				{
					cscr->_begy = LINES/2 -
						(win->_cury + win->_begy);
				}
			}

			if (CMGETDBL)
			{
			    IITDctsClearToScroll(win, lastLine, lastLine_dx);
			}
			else
			{
			    IITDctsClearToScroll(win, lastLine,0);
			}

			win = twin;
			_win = win;
		}
		/*
		** Check for a scroll up (the cursor moves below the bottom		6.4_PC_80x86
		** line, so the screen moves up)
		*/
		else if (presy >= LINES - 1 - _pwin->_starty && win->_relative)
		{
			/*
			** find the parent window so the offset can be
			** calculated from it
			*/
			twin = win;
			win = _win = _pwin;
			offset = win->_begy + cscr->_begy;
			lines = max(0, (LINES - 1 - win->_starty) - offset);
			if (presy - offset - lines >= heightadj)
			{
				cscr->_begy = cscr->_begy -
					(presy - offset - lines) - LINES/2;
				/* make sure end of form is end of screen */
				if (cscr->_begy + win->_maxy
					< (LINES - 1 - win->_starty))
				{
					cscr->_begy = (LINES - 1 - win->_starty)
						- win->_maxy;
				}

				if (CMGETDBL)
				{
				    IITDctsClearToScroll(win, lastLine, lastLine_dx);
				}
				else
				{
				    IITDctsClearToScroll(win, lastLine,0);
				}

				win = twin;
				_win = win;
			}
			else
			{
			    if (win->_flags & _TDSPDRAW)
			    {
				for (wy = lines;
				    wy <= presy - offset +
				    (dont_center ? 0 : LINES/2) &&
				    wy < win->_maxy; wy++)
				{
					cscr->_begy--;
				}
				TDtouchwin(win);
				TDrefresh(win);
				win = twin;
				_win = win;
			    }
			    else
			    {
				if (CAS == NULL)
				{
				    if (CMGETDBL)
				    {
					TDsaveLast_dx(lastLine_dx);
				    }
				    saveLattr = TDsaveLast(lastLine);
				}
				TDsmvcur((i4) 0, COLS - 1, LINES - 2, lx);
				cscr->_cury = ly = LINES - 2;
				cscr->_curx = lx;
				/*
				** Determine how many lines are off the screen
				** and scroll that number of lines.
				** For each line scrolled, the beginning y of
				** the current screen must be decremented.
				** The code could be optimized to not do one
				** line scrolls if the number of lines to scroll
				** is great.
				**
				** The loop maintains the invariants
				** cury == LINES - 2 and ly == LINES - 2
				*/
				if (CAS != NULL)
				{
					cp = TDtgoto(CAS, LINES - 1,
						1 + IITDflu_firstlineused);
					STcopy(cp, scp);
					cp = TDtgoto(CAS, LINES, (i4) 1);
					STcopy(cp, ucp);
					_puts(scp);
					TDsmvcur((i4) 0, COLS - 1, LINES - 2,
						lx);	/* lx was 0 */
				}
				for (wy = lines;
				    wy <= presy - offset +
				    (dont_center ? 0 : LINES/2) &&
				    wy < win->_maxy; wy++)
				{
					if (TDscroll(cscr) == FAIL)
					{
						win = twin;
						_win = win;
						retval = FAIL;
						goto ret;
					}
					/*
					** TDscroll generates \n's to scroll the
					** the terminal screen.  If the terminal
					** positions the cursor at the left
					** screen margin on a \n (see XL) then
					** need to reset lx to the cursor
					** location.  (bruceb)
					**
					** See also: TDgettmode in crtty.c
					**	(fredb - 16-Nov-90)
					*/
					if (XL || !NONL)
						lx = 0;
					win->_firstch[wy] = 0;
					win->_lastch[wy] = win->_maxx - 1;
					cscr->_begy--;
					cscr->_cury = LINES - 2;
					if (TDmakech(win, wy) == FAIL)
					{
						win = twin;
						_win = win;
						retval = FAIL;
						goto ret;
					}
					else
					{
						win->_firstch[wy] = _NOCHANGE;
					}
					if (lx > COLS - 1 && CAS != NULL)
					{
						TDsmvcur((i4) 0, COLS - 1,
							cscr->_cury,
							cscr->_curx);
					}
					else
					{
						TDsmvcur(ly, lx, cscr->_cury,
							cscr->_curx);
					}
					ly = cscr->_cury;
					lx = cscr->_curx;
				}
				win = twin;
				_win = win;
				if (CAS != NULL)
				{
					_puts(ucp);
					TDsmvcur((i4) 0, COLS - 1, cscr->_cury,
						cscr->_curx);
				}
				else
				{
				    if (CMGETDBL)
				    {
					TDaddLast_dx(lastLine_dx);
				    }
				    TDaddLast(lastLine, saveLattr);
				    TDclrTop();
				}
			    }
			}
		}
		/*
		** Check for a scroll down.		6.4_PC_80x86
		*/
		else if (presy < 0)
		{
			twin = win;
			win = _win = _pwin;
			offset = win->_begy + cscr->_begy;
			lines = min(win->_maxy - 1, (-1 - offset));
			if (CAS != NULL && lines - (presy - offset) < heightadj)
			{
			    if (win->_flags & _TDSPDRAW)
			    {
				for (wy = lines;
				    wy >= presy - offset -
				    (dont_center ? 0 : LINES/2) && wy >= 0;
				    wy--)
				{
					cscr->_begy++;
				}
				TDtouchwin(win);
				TDrefresh(win);
				win = twin;
				_win = win;
			    }
			    else
			    {
				cscr->_cury = ly = win->_starty;
				cscr->_curx = lx;
				cp = TDtgoto(CAS, LINES - 1,
					1 + IITDflu_firstlineused);
				_puts(cp);
				TDsmvcur(LINES - 1, COLS, win->_starty, lx);
				for (wy = lines;
				    wy >= presy - offset -
				    (dont_center ? 0 : LINES/2) && wy >= 0;
				    wy--)
				{
					if (TDrscroll(cscr) == FAIL)
					{
						win = twin;
						_win = win;
						retval = FAIL;
						goto ret;
					}
					win->_firstch[wy] = 0;
					win->_lastch[wy] = win->_maxx - 1;
					cscr->_begy++;
					cscr->_cury = win->_starty;
					if (TDmakech(win, wy) == FAIL)
					{
						win = twin;
						_win = win;
						retval = FAIL;
						goto ret;
					}
					else
					{
						win->_firstch[wy] = _NOCHANGE;
					}
					TDsmvcur(ly, lx, cscr->_cury,
						cscr->_curx);
					ly = cscr->_cury;
					lx = cscr->_curx;
				}
				win = twin;
				_win = win;
				cp = TDtgoto(CAS, LINES, (i4) 1);
				_puts(cp);
				TDsmvcur(LINES - 1, COLS, cscr->_cury,
					cscr->_curx);
			    }
			}
			else
			{
			    if (CMGETDBL)
			    {
				TDsaveLast_dx(lastLine_dx);
			    }

				saveLattr = TDsaveLast(lastLine);
				win->_cury = twin->_cury + twin->_begy -
					win->_begy;
				win->_curx = twin->_curx + twin->_begx -
					win->_begx;

				if (win->_cury + win->_begy < LINES/2)
				{
					cscr->_begy = 0;
				}
				else
				{
					cscr->_begy =
						(dont_center ? 0 : LINES/2) -
						(win->_cury + win->_begy);
				}

				if (CMGETDBL)
				{
				    IITDctsClearToScroll(win, lastLine, lastLine_dx);
				}
				else
				{
				    IITDctsClearToScroll(win, lastLine,0);
				}

				win = twin;
				_win = win;
			}
		}
	}
	/*
	** If the window is to be cleared, determine a easy way to
	** do it.
	** The first time the current screen is refreshed, a clear is
	** done.
	*/
	if (win->_clear || curscr->_clear || _curwin)
	{
		if (_curwin)
		{
			/*
			** Clear out terminal display attribute
			** setting.  This allows users to sync
			** things up if they switch between
			** different sessions.
			*/
			_puts(EA);
			_puts(LE);
			_puts(YA);
			pda = 0;
			pfonts = 0;
			pcolor = 0;
			IITDcda_prev = EOS;
			IITDptr_prev = NULL;

			if (LD && !XD)
			{
				_puts(LD);
			}
			if (KY)
			{
				_puts(KS);
			}
# if defined(MSDOS) || defined(NT_GENERIC)
			/*
			** Clear 1st-char-changed on each line
			*/
			for (wy = LINES; wy--; )
				IIcurscr_firstch[wy] = 0;
# endif /* MSDOS */
		}
		if ((win->_flags & _FULLWIN) || curscr->_clear)
		{
			_puts(CL);
			/*
			** after a CL cursor is at 0,0
			*/
			ly = 0;
			lx = 0;
			if(cscr->_begy > 0)
			{
				cscr->_cury = cscr->_begy;
			}
			else if (cscr->_begy < 0)
			{
				cscr->_cury = -cscr->_begy;
			}
			if (cscr->_begx > 0)
			{
				cscr->_curx = cscr->_begx;
			}
			else if (cscr->_begx < 0)
			{
				cscr->_curx = -cscr->_begx;
			}
			if (cscr->_begy > 0 || cscr->_begx > 0)
			{
				TDsmvcur((i4) 0, (i4) 0, cscr->_begy,
					cscr->_begx);
			}
			cscr->_clear = FALSE;
			if (!_curwin)
				TDerase(cscr);
			TDtouchwin(win);
		}
		win->_clear = FALSE;
	}
	if (!_curwin && win->_relative)
	{
		/*
		** Find the portion of the window which is on the screen.
		*/
		if(cscr->_begy + win->_begy < 0)
			wybeg = -(win->_begy + cscr->_begy);
		else
			wybeg = 0;
		wyend = min((LINES - 1 - _pwin->_starty) - (cscr->_begy + win->_begy), win->_maxy);
	}
	else
	{
		wybeg = 0;
		wyend = win->_maxy;
	}
	/*
	** For the portion of the window on the screen refresh the lines
	** which have been changed
	*/

	if (IITDfcFromClear)
	{
		/*
		**  Note that we don't clear out the _firstch
		**  for curscr when called from TDclear.  This
		**  should be OK since it is not critical and
		**  they will eventually get sync up when someone
		**  does a ^W.
		*/
		change = TRUE;
	}
	else
	{
		for (wy = wybeg; wy < wyend; wy++)
		{
			if (win->_firstch[wy] != _NOCHANGE)
			{
				if (TDmakech(win, wy) == FAIL)
				{
					retval = FAIL;
					goto ret;
				}
				else
				{
					change = TRUE;
					win->_firstch[wy] = _NOCHANGE;
				}
			}
		}
	}
	/*
	** If we are running on an HP terminal in HP mode (XS) and
	** we've done a clear to end of line (clr_eol) and
	** we are working in a sub window (_SUBWIN) and
	** the sub window does not extend to the physical right edge
	** of the screen (_maxx != COLS) THEN redisplay what we have
	** erased outside our window by calling TDrefresh for the special
	** save window that is setup and maintained in TDmakech.
	*/
	if (XS && clr_eol && (win->_flags & _SUBWIN) &&
		((_pwin->_startx + win->_begx + win->_maxx) != COLS))
	{
		/*
		** set current x & y screen position before
		** trying to redisplay the part of the screen
		** outside our subwindow.
		*/
		cscr->_cury = ly - (cscr->_begy);
		cscr->_curx = lx;
		TDrefresh(xs_savewin);
                /* set cursor position */
                ly = cscr->_cury;
                lx = cscr->_curx;
	}

	if (win->lvcursor)
	{
		/*
		** (jrc) If a change was made to the screen, then the cursor
		** will be positioned after the last change.
		** This window has leave set, so we want to change the
		** cursor location if and only if a change was made.
		*/
		if (change)
		{
			/*
			** ly is an absolute screen position.
			** cury should be relative.
			*/
			cscr->_cury = ly - (cscr->_begy);
			cscr->_curx = lx - (cscr->_begx);
		}
		ly -= (win->_begy + _pwin->_starty);
		lx -= (win->_begx + _pwin->_startx);
		if (ly >= 0 && ly < win->_maxy && lx >= 0 && lx < win->_maxx)
		{
			win->_cury = ly;
			win->_curx = lx;
		}
		else
		{
			win->_cury = win->_curx = 0;
		}
	}
	else if (!_curwin)
	{
		TDsmvcur(ly, lx,
			win->_cury + win->_begy + cscr->_begy + _pwin->_starty,
			win->_curx + win->_begx + cscr->_begx+ _pwin->_startx);
		cscr->_cury = win->_cury + win->_begy + _pwin->_starty;
		cscr->_curx = win->_curx + win->_begx + _pwin->_startx;
	}
	else
	{
		/* next 4 lines for scroll debug */
		if (depth <= 1)
		{
			cscr->_cury = ocury;
			cscr->_curx = ocurx;
		}
		TDsmvcur(ly, lx, cscr->_cury + cscr->_begy,
			cscr->_curx + cscr->_begx);
	}
	retval = OK;
ret:
	depth--;
	if (depth == 0)
	{
		if (TDoutcount != TDBUFSIZE)
		{
			if (ZZB)
			{
				*TDobptr = '\0';
				xcnt = ITxout(TDoutbuf, xbuf,
					TDBUFSIZE - TDoutcount);
				TEwrite(xbuf, xcnt);
			}
			else
			{
				TEwrite(TDoutbuf, TDBUFSIZE - TDoutcount);
			}
			TEflush();
		}
	}
	/*
	** Reset absolute window coordinates.
	*/
	if (!win->_relative)
	{
		win->_begy = absy;
		win->_begx = absx;
	}

	/*
	**  Fix for BUG 8408. (dkh)
	*/
	if (onmuline)
	{
		/*
		**  Next line part of fix for BUG 7165. (dkh)
		*/
		cscr->_curx -= tbegx;
		cscr->_cury -= tbegy;

		cscr->_begx = tbegx;
		cscr->_begy = tbegy;
	}
	_win = NULL;
	if (depth == 0)
	{
# ifdef	DGC_AOS
		TDclrattr();	/* turn off disp attrs for CEO status line */
		TElock(FALSE, (cscr->_begy + cscr->_cury),
			(cscr->_begx + cscr->_curx));
# endif /* DGC_AOS */
	}

	/*
	** brett, X offset windex command string
	** Note: send new X screen offset to windex
	*/
	if (oldbegx != cscr->_begx)
	{
		IITDxoff(-cscr->_begx);
		oldbegx = cscr->_begx;
	}
	/*
	** brett, Y offset windex command string
	** Note: Give new Y screen offset to windex.
	*/
	if (oldbegy != cscr->_begy)
	{
		IITDyoff(-cscr->_begy);
		oldbegy = cscr->_begy;
	}

# if defined(MSDOS) || defined(NT_GENERIC)
	/*
	** Notify TE that refresh had ended.
	** TEerefresh() will position the cursor
	** and update video RAM directly from curscr WINDOW buffers.
	*/
	TEerefresh(curscr->_cury + curscr->_begy,
		curscr->_curx + curscr->_begx,
		curscr->_y, curscr->_da, IIcurscr_firstch);
# endif /* MSDOS || NT_GENERIC */

	return(retval);
}



/*
**  Put a color to the terminal.
*/

TDputcolor(color)
u_char	color;
{
	reg	char	*cp;

	if (color == _COLOR1)
	{
		cp = YB;
	}
	else if (color == _COLOR2)
	{
		cp = YC;
	}
	else if (color == _COLOR3)
	{
		cp = YD;
	}
	else if (color == _COLOR4)
	{
		cp = YE;
	}
	else if (color == _COLOR5)
	{
		cp = YF;
	}
	else if (color == _COLOR6)
	{
		cp = YG;
	}
	else if (color == _COLOR7)
	{
		cp = YH;
	}
	else
	{
		cp = YA;
	}
	_puts(cp);
}



/*
**  Put a display attribute to the terminal.
*/

TDdaput(da)
u_char	da;
{
	reg	char	*cp;

	switch(da)
	{
		case _CHGINT:
			cp = BO;
			break;

		case _UNLN:
			cp = US;
			break;

		case _BLINK:
			cp = BL;
			break;

		case _RVVID:
			cp = RV;
			break;

		case _CIULBLRV:
			cp = ZA;
			break;

		case _CI_UL:
			cp = ZB;
			break;

		case _CI_BL:
			cp = ZC;
			break;

		case _CI_RV:
			cp = ZD;
			break;

		case _UL_BL:
			cp = ZE;
			break;

		case _UL_RV:
			cp = ZF;
			break;

		case _BL_RV:
			cp = ZG;
			break;

		case _CI_UL_BL:
			cp = ZH;
			break;

		case _UL_BL_RV:
			cp = ZI;
			break;

		case _BL_RV_CI:
			cp = ZJ;
			break;

		case _RV_CI_UL:
			cp = ZK;
			break;

		default:
			return;
	}
	_puts(cp);
}


/*
**  Static to keep track of previously processed display attribute.
*/

# ifdef TRACING
static	FILE	*dbgout = NULL;
# endif /* TRACING */

/*
** make a change on the screen
*/

TDmakech(awin, wy)
WINDOW	*awin;
i4	wy;
{
	reg	char	*nda;
	reg	char	*cda;
	reg	char	*nsp;
	reg	char	*csp;
	reg	WINDOW	*win = awin;
	reg	WINDOW	*cscr = curscr;
	reg	char	ndaval;
	reg	bool	lcurwin;
	reg	char	*ce;
	reg	i4	wx;
	reg	i4	svx;
	reg	i4	lch;
	reg	i4	y;
	reg	i4	x;
	reg	i4	winflags;
	reg	i4	winxbeg;
	reg	i4	winxmax;
	reg	i4	winymax;
	reg	i4	nlsp;	/* last space in lines */
	reg	i4	clsp;	/* last space in lines */
	reg	char	*ndx;
	reg	char	*cdx;
	reg	char	*ceda;
	reg	i4	invcx;
	reg	i4	invwx;
	reg	i4	invc_wx;
	reg	u_char	da;
	reg	u_char	fonts;
	reg	u_char	color;
	reg	i4	bytecnt;
	reg	i4	cols_1;
	reg	bool	xn_loc;
		char	zda;		/* work var for attr/font cleanup */
		i4	wrk;		/* work var for HP term mumbo-jumbo */
                bool    printAble = TRUE; 
                bool    clr_da;         /* true if display attribute needs 
                                        ** to be cleared. */

# ifdef TRACING
	if (dbgout == NULL)
	{
		dbgout = fopen("xcs.debug", "w");
	}

	SIfprintf(dbgout, "Top of makech\n");
# endif /* TRACING */

	lcurwin = _curwin;
	winflags = win->_flags;
	winxbeg = win->_begx;
	winxmax = win->_maxx;
	winymax = win->_maxy;
	xn_loc = XN;
	cols_1 = COLS - 1;

	wx = win->_firstch[wy];

	invcx = (lcurwin ? 0 : cscr->_begx);
	invwx = (lcurwin ? 0 : winxbeg + _pwin->_startx);
	invc_wx = invcx + invwx;

	if (lcurwin)
	{
		y = wy;
		x = wx;
		lch = win->_lastch[wy];
		/* use of "+" is to enable clearing top line of the terminal */
		if (inclrtop)
			csp = ERx("+");
		else
			csp = ERx(" ");
		/*
		** Need 2 0's so IITDptr_prev+1 is valid in TDmakech/TDsmvcur.
		*/
		cda = ERx("\0\0");
		cdx = ERx("\0");
		ce = NULL;
	}
	else
	{
		i4	tx;

		y = wy + win->_begy + cscr->_begy + _pwin->_starty;
		x = wx + winxbeg + cscr->_begx + _pwin->_startx;
		if ((tx = wx + winxbeg + invcx + _pwin->_startx) < 0)
		{
			wx -= tx;
			x -= tx;
		}
		if (win->_lastch[wy] + winxbeg + invcx +
			_pwin->_startx > cols_1)
		{
			lch = cols_1 - winxbeg - invcx - _pwin->_startx;
		}
		else
		{
			lch = win->_lastch[wy];
		}
		csp = &curscr->_y[y][x];
		cda = &curscr->_da[y][x];
		cdx = &curscr->_dx[y][x];

		if (CE)
		{
			/* check for end of da also */
			nsp = &win->_y[wy][win->_maxx - 1];
			nda = &win->_da[wy][win->_maxx - 1];
			ndx = win->_y[wy];

			/*
			**  Removed autodecrement for nsp and nda since
			**  that causes nsp to be decremented before
			**  the attribute values (nda) are determined
			**  to be zero.  This potentially causes nsp
			**  to be one less than the position of the
			**  character that is not blank or has
			**  attributes.
			*/
			while (*nsp == ' ' && nsp > ndx && *nda == 0)
			{
				nsp--;
				nda--;
			}
			/*
			**  If the current character is not a blank or
			**  has attributes, we exited the above loop
			**  because nsp equals ndx.  In this case,
			**  increment nlsp to point to the left most
			**  character that is blank and has no attributes.
			*/
			if (*nsp != ' ' || *nda != 0)
			{
				nsp++;
			}

			nlsp = nsp - ndx;
		}
		ce = CE;
	}
# if defined(MSDOS) || defined(NT_GENERIC)
	/*
	** Mark line 'y' as changed beginning at position 'x'.
	*/
	IIcurscr_firstch[y] =
		(x < IIcurscr_firstch[y]) ? x : IIcurscr_firstch[y];
# endif /* MSDOS */
	nsp = &win->_y[wy][wx];
	nda = &win->_da[wy][wx];
	ndx = &win->_dx[wy][wx];

	/*
	** set IITDcda_prev to 0 for left edge of screen, otherwise
	** use the previous value of *cda. In any case, nullify the
	** pointer to where the previous cda was.
	** (Only used by HP terminals in HP mode)
	*/
	if (csp == curscr->_y[y])
	{
		IITDcda_prev = EOS;
	}
	else
	{
		IITDcda_prev = *(cda-1);
	}
	IITDptr_prev = NULL;

	while (wx <= lch)
	{
                printAble = TRUE;

# ifdef TRACING
		SIfprintf(dbgout, "Top of loop\n");
# endif /* TRACING */

		/* 
		** Use ternary to choose which (Db/Sb) boolean expression to use 
		*/
                if (
		    (CMGETDBL) ?
		    (CMcmpcase(nsp,csp) || (*nda != *cda))  :
		    (*nsp != *csp ||  *nda != *cda)
		    )
		{
# ifdef TRACING
			SIfprintf(dbgout, "Data different\n");
# endif /* TRACING */
			TDsmvcur(ly, lx, y, wx + invc_wx);
			ly = y;
			lx = wx + invc_wx;

			/*
			** For HP terminals in HP mode (XS) and
			** if the current screen position is not the
			** left edge, THEN save the display attributes
			** of the previous screen position.
			*/
			if (XS && csp != curscr->_y[y])
			{
				zda = pda;
				pfonts = zda & _LINEMASK;
				pcolor = zda & _COLORMASK;
				IITDcda_prev = zda;
			}


			while ( (wx <= lch) && (
				    (CMGETDBL) ?
				    (CMcmpcase(nsp,csp) || (*nda != *cda))  :
				    (*nsp != *csp ||  *nda != *cda)
				    )
			    )
			{
				if (ce != NULL && wx >= nlsp
					&& *nsp == ' ' && *nda == 0)
				{
					ce = nsp;
					ceda = nda;
					/*
					** check for clear to end-of-line
					*/
					nsp = &curscr->_y[ly][cols_1];
					nda = &curscr->_da[ly][cols_1];

					/*
					**  Make sure we don't decrement
					**  before we checked that both
					**  the character is a blank and
					**  and there are no attributes.
					*/
					while (*nsp == ' ' && nsp > csp &&
						*nda == 0)
					{
						nsp--;
						nda--;
					}
					/*
					**  If the current character is
					**  not a blank or has attributes
					**  then incresemnt nsp to point
					**  to left most character that is
					**  blank and has attributes.
					*/
					if (*nsp != ' ' || *nda != 0)
					{
						nsp++;
					}
					clsp = nsp - curscr->_y[ly] - winxbeg;

					/*
					**  Compensate for a scrolled
					**  background form.
					*/
					if (!win->_relative)
					{
						clsp -= curscr->_begx;
					}
					if (clsp - nlsp >= STlength(CE)
						&& clsp < winxmax)
					{
						_puts(CE);

						/*
						** Fix for BUG 7983. (dkh)
						*/
						lx = wx + invc_wx;

						if (wx <= lch)
						{
							MEfill((u_i2) (lch - wx + 1), (u_char) ' ', csp);
							MEfill((u_i2) (lch - wx + 1), (u_char) '\0', cda);
							MEfill((u_i2) (lch - wx + 1), (u_char) '\0', cdx);
						}
						goto ret;
					}
					nsp = ce;
					nda = ceda;
					ce = NULL;
				}

				fonts = (ndaval = *nda) & _LINEMASK;
				da = ndaval & _DAMASK;
				color = ndaval & _COLORMASK;

				/*
				** Since display attribute and font
				** switching can cause 'marks' to
				** accumulate in HP terminals'
				** memory, we'll clean up by doing
				** a clear to EOL if there is any
				** indication that there may be a
				** problem. IITDcda_prev is the display
				** attributes
				** used on the previous screen position, before
				** any changes. zda is the previous attribute
				** byte, cda is the current (on screen now)
				** attribute byte, and ndaval is the new
				** (desired) attribute byte. A zero in any
				** of these means default attributes, font,
				** and color.
				** We also load the previous (pda, pfonts,
				** and pcolor) variables with what we believe
				** to be the actual values used to the left of
				** where we are going to write on the screen.
				*/
				if (XS && !lcurwin)
				{
				    zda = (pda | pfonts | pcolor);

                                    /* clear reverse video display */
                                    clr_da = (*cda == _RVVID && 
                                              *cda != (ndaval & _DAMASK)); 

				    if ((!IITDcda_prev &&
					*cda && !(!zda && ndaval)) ||
					(IITDcda_prev &&
					 ((zda && !*cda && ndaval) ||
					  (!zda && *cda && !ndaval) ||
					  (ndaval && (IITDcda_prev != *cda) &&
					   (ndaval != *cda))))
                                        ||
                                        clr_da
                                       )
				    {
					/*
					** Handle the case of clearing
					** stuff outside our subwindow
					*/
					if ((win->_flags & _SUBWIN) &&
					    ((_pwin->_startx+win->_begx+
					    win->_maxx) != COLS))
					{
					    /*
					    ** make new window, if
					    ** not already done
					    */
					    if (!xs_savewin)
					    {
						xs_savewin = TDnewwin(LINES,
						    COLS, 0,0);
					    }
					    /*
					    ** copy current screen
					    ** into our special
					    ** window, if this is
					    ** the first clear to
					    ** EOL for this refresh
					    */
					    if (!clr_eol)
					    {
						TDoverwrite (cscr, xs_savewin);
						/*
						** set special
						** window for
						** 'nothing has
						** changed'
						** BE SAFE,
						** start at y=0.
						*/
						for (wrk = 0; wrk < cscr->_maxy;
						    wrk++)
						{
						    xs_savewin->_firstch[wrk]=
							_NOCHANGE;
						    xs_savewin->_lastch[wrk]=
							cscr->_maxx;
						}
						xs_savewin->_clear = FALSE;
					    }
					    /*
					    ** mark start of area
					    ** outside our sub-
					    ** window as needing to
					    ** be redrawn
					    */
					    xs_savewin->_firstch[y] =
						_pwin->_startx + win->_begx +
						win->_maxx;
					}
					_puts(CE);

                                        /* end reverse video display (RE) after
                                        ** we clear the rest of the line (CE). 
                                        ** Note that this will also clear all
                                        ** enhance video display attributes coz 
                                        ** HP70092's termcap define RE, BE, EB,
                                        ** UE, EA and EAW to the same value. 
                                        */
                                        if (clr_da) 
                                           _puts(RE); 

					clr_eol = TRUE;
					MEfill((u_i2) (cscr->_maxx -
					    (wx + invc_wx)), (u_char) ' ', csp);
					MEfill((u_i2) (cscr->_maxx -
					    (wx + invc_wx)), (u_char) '\0',cda);
					MEfill((u_i2) (cscr->_maxx -
					    (wx + invc_wx)), (u_char) '\0',cdx);
					if (csp != curscr->_y[y])
					{
					    zda = pda;
					    pfonts = zda & _LINEMASK;
					    pcolor = zda & _COLORMASK;
					}
				    }
				    /*
				    ** IF we are working on the first character
				    ** on the line OR some other character on
				    ** the line that will have a different
				    ** display attribute than the character
				    ** before it, THEN
				    ** Force previous display attribute and font
				    ** to be what is on the screen if the new
				    ** attribute/font is not the same as the
				    ** attribute/font that is already on the
				    ** screen.
				    */
				    else if (csp == curscr->_y[y] ||
					*cda != IITDcda_prev)
				    {
					if ((ndaval & _DAMASK) !=
					    (*cda & _DAMASK))
					{
					    pda = *cda & _DAMASK;
					}
					if ((ndaval & _LINEMASK) !=
					    (*cda & _LINEMASK))
					{
					    pfonts = *cda & _LINEMASK;
					}
				    }
				}

				if (XS || da != pda || fonts != pfonts ||
					pcolor != color)
				{
					TDdaout(fonts, da, color);
				}

				if (CMGETDBL)
				{
				    /** Do Double Byte Processing **/
				/* save current position for using later */
				svx = wx;
				/*
				** Current cursor position is on the left edge
				** of the screen and there is only a 2nd byte
				** of a Double Byte character, then 'wx' should
				** be incremented by just one.  Otherwise 'wx'
				** is incremented by one or two depending on
				** the character.
				*/
				if (*ndx & _DBL2ND)
				{
					wx++;
				}
				else
				{
                                     if (CMprint(nsp))
					CMbyteinc(wx, nsp);
                                     else
                                        printAble = FALSE; 
				}

				if (wx > winxmax && wy == winymax - 1)
				{
					if (win->_scroll)
					{
					    if (!_curwin)
					    {
						if (CMdbl1st(nsp))
						{
						    if (*(ndx + 1) & _DBL2ND)
							TDput(*nsp);
						    else
							TDput(OVERLAYSP);
						    fonts = (ndaval = *(nda + 1)) & _LINEMASK;
						    da = ndaval & _DAMASK;
						    color = ndaval & _COLORMASK;
						    if (da != pda || fonts != pfonts || pcolor != color)
						    {
							TDdaout(fonts, da, color);
						    }

						    TDput(*(nsp + 1));
						    *csp = *nsp;
						    *cda = *nda;
						    *cdx = *ndx;

						    *(csp + 1) = *(nsp + 1);
						    *(cda + 1) = *(nda + 1);
						    *(cdx + 1) = *(ndx + 1);
					    }
					    else
					    {
						TDput(*nsp);
						if (*(ndx + 1) & _DBL2ND)
						TDput(OVERLAYSP);
						*csp = *nsp;
						*cda = *nda;
						*cdx = *ndx;
						}
					    }
					    else
					    {
						if (CMdbl1st(nsp))
						{
						   if (*(ndx + 1) & _DBL2ND)
							TDput(*nsp);
						   else
							TDput(OVERLAYSP);
						   fonts = (ndaval = *(nda + 1)) & _LINEMASK;
						   da = ndaval & _DAMASK;
						   color = ndaval & _COLORMASK;
						   if (da != pda || fonts != pfonts || pcolor != color)
						   {
						       TDdaout(fonts, da, color);
						   }

						   TDput(*(nsp + 1));
						}
						else
						{
						    TDput(*nsp);
						    if (*(ndx + 1) & _DBL2ND)
							TDput(OVERLAYSP);
						}
					    }
					    TDscroll(win);
					    if (winflags & _FULLWIN && !_curwin)
					    {
						TDscroll(cscr);
					    }
					    ly = win->_begy + win->_cury +
						cscr->_begy;
					    lx = winxbeg + win->_curx +
						cscr->_begx;
					    if (pfonts == _LINEDRAW)
					    {
						_puts(LE);
					    }
					    return(OK);
					}
					else if (winflags & _SCROLLWIN)
					{
					    lx = --wx;
					    if (pfonts == _LINEDRAW)
					    {
						_puts(LE);
					    }
					    return(FAIL);
					}
				}
				if (!_curwin)
				{
					i4	sx;

					/*
					** Just 2nd byte of a Double Byte
					** character is on the left edge of
					** screen, then put a dummy space on
					** that position instead of 2nd byte
					** of a character.
					**
					**	'sx' is the absolute position
					**	on the screen.
					**
					**		+----------
					**	    abcK|1efg
					**		+----------
					**	--->
					**		+----------
					**	    abcK| efg
					**		+^---------
					*/
					sx = svx + invc_wx;
					if (*ndx & _DBL2ND)
					{
                                           if (sx == 0)
                                           {
						/*
						** LSP is a left space character
						** instead of the 2nd byte of a
						** character.
						*/
						TDput(LSP);
						*csp++ = *nsp;
						*cda++ = *nda;
						*cdx++ = *ndx;
					   }
                                           else
                                           {
                                              /* Bug 104819 - Popup menu has overlaid DBLbyte character
                                              ** such that the first character AFTER the Right Hand Side
                                              ** border is the 2nd byte of a double-byte character. So
                                              ** replace this character with a space.
                                              */
                                              TDput(OVERLAYSP);
                                           }
                                        }
					else
					{
						/*
						** Just 1st byte of a Double
						** Byte character is on the
						** right edge of screen, then
						** put a dummy space on that
						** position instead of 1st byte
						** of a character.
						**
						**	'sx' is the absolute
						**	position on the screen.
						**
						**	----------+
						**	      abcK|1def
						**	----------+
						**   --->
						**	----------+
						**	      abc |1def
						**	---------^+
						*/
						if (sx == cols_1
							&& CMdbl1st(nsp))
						{
							/*
							** RSP is a right space
							** character instead of
							** the 1st byte of a
							** character.
							*/
							TDput(RSP);
						}
						else
						{
							if (CMdbl1st(nsp))
							{
							   if (*(ndx + 1) & _DBL2ND)
								TDput(*nsp);
							   else
								TDput(OVERLAYSP);
							   fonts = (ndaval = *(nda + 1)) & _LINEMASK;
							   da = ndaval & _DAMASK;
							   color = ndaval & _COLORMASK;
							   if (da != pda || fonts != pfonts || pcolor != color)
							   {
							       TDdaout(fonts, da, color);
							   }

							   TDput(*(nsp + 1));
							   *cda++ = *nda;
							   *cdx++ = *ndx;
							   *cda++ = *(nda + 1);
							   *cdx++ = *(ndx + 1);
							   CMcpychar(nsp, csp);
							   CMnext(csp);
							}
							else
							{
								TDput(*nsp);

                                                                if (CMprint(nsp))
                                                                   *csp++ = *nsp;
                                                                else
                                                                   printAble = FALSE; 

								*cda++ = *nda;
								*cdx++ = *ndx;
								if (*(ndx + 1) & _DBL2ND)
								{
									*csp++ = *(nsp + 1);
									*cda++ = *(nda + 1);
									*cdx++ = *(ndx + 1);
								}
							}
						}
						/*
						** When you are editing a data
						** or a trim field in the forms
						** screen editor (VIFRED, RBF),
						** and a field you are editing
						** expands over a next field,
						** and only the 1st byte of a
						** Double Byte character is
						** overwritten temporarily,
						** then the 2nd byte of a Double
						** Byte character changes to a
						** dummy space on the screen.
						**
						**    +-----------------------+
						**    |			      |
						**    |    abc   K1K2 c_      |
						**    |    ^  		      |
						**    +----|------------------+
						**	    current field
						**
						** ---> insert "1234"
						**    +-----------------------+
						**    |			      |
						**    |    1234abc K2 c_      |
						**    |	          ^	      |
						**    +-----------------------+
						**
						** ---> after editing
						**    +-----------------------+
						**    |			      |
						**    |    1234abc K1K2 c_    |
						**    |			      |
						**    +-----------------------+
						*/
						if (*cdx & _DBL2ND)
						{
							*csp = ' ';
							*cda = *(nda + 1);
							fonts = (ndaval = *(nda + 1)) & _LINEMASK;
							da = ndaval & _DAMASK;
							color = ndaval & _COLORMASK;
							if (da != pda || fonts != pfonts || pcolor != color)
							{
							    TDdaout(fonts, da, color);
							}
							TDput(' ');
							TDput('\b');
						}
					}
					if (XN)
					{
						sx = wx - 1 + invc_wx;
						if (sx == cols_1)
						{
							TDsmvcur(ly, sx, ly,
								sx - 1);
							TDsmvcur(ly, sx - 1, ly,
								sx);
						}
					}
				}
				else
				{
					i4	sx;

					/*
					** see the comment above about left
					** edge handling.
					*/
					sx = svx + invc_wx;
					if (*ndx & _DBL2ND)
					{
                                           if (sx == 0)
                                           {
						/*
						** LSP is a left space character
						** instead of the 2nd byte of a
						** character.
						*/
						TDput(LSP);
                                           }
                                           else
                                           {
                                               /* Bug 104819 - Display a space. Not
                                               ** the second byte of a DBL char.
                                               */
                                               TDput(OVERLAYSP);
                                           }
					}
					else
					{
						/*
						** see the comment above about
						** right edge handling.
						*/
						if (sx == cols_1
							&& CMdbl1st(nsp))
						{
							/*
							** RSP is a right space
							** character instead of
							** the 1st byte of a
							** character.
							*/
							TDput(RSP);
						}
						else
						{
							if(CMdbl1st(nsp))
							{
							if (*(ndx + 1) & _DBL2ND)
							{
								TDput(*nsp);	
								TDput(*(nsp + 1));
							}
							else
							{
								TDput(OVERLAYSP);
								fonts = (ndaval = *(nda + 1)) & _LINEMASK;
								da = ndaval & _DAMASK;
								color = ndaval & _COLORMASK;
								if (da != pda || fonts != pfonts || pcolor != color)
								{
								    TDdaout(fonts, da, color);
								}
								TDput(*(nsp+1));
							}
							}
							else
							    TDput(*nsp);
						}
					}
				}
				if (*ndx & _DBL2ND)
				{
					/* this code executes at most once */
					nda++;
					ndx++;
					nsp++;
				}
				else
				{
					CMbyteinc(nda,nsp);
					CMbyteinc(ndx,nsp);
					CMnext(nsp);
				}
                            }
                            else
                            {
                                /** Do Single Byte Processing **/

				wx++;
				if (wx > winxmax && wy == winymax - 1)
				{
					if (win->_scroll)
					{
						if (!lcurwin)
						{
							TDput(*csp = *nsp);
							*cda = *nda;
						}
						else
						{
							TDput(*nsp);
						}
						TDscroll(win);
						if (winflags & _FULLWIN &&
							!lcurwin)
						{
							TDscroll(cscr);
						}
						ly = win->_begy + win->_cury +
							cscr->_begy;
						lx = win->_begx + win->_curx +
							cscr->_begx;
						if (pfonts == _LINEDRAW)
						{
							_puts(LE);
						}
						return(OK);
					}
					else if (winflags & _SCROLLWIN)
					{
						lx = --wx;
						if (pfonts == _LINEDRAW)
						{
							_puts(LE);
						}
						return(FAIL);
					}
				}

				/*
				** need to save the current value
				** of *cda and cda so we can 'look
				** around' later to make decisions
				** about what codes to send to the
				** terminal. Note that IITDcda_prev
				** contains the old attribute while
				** IITDptr_prev points to the new attribute.
				** (Used only for HP terminals in HP mode)
				*/
				IITDcda_prev = *cda; /* save attributes */
				IITDptr_prev = cda;  /* and place */

				if (!lcurwin)
				{
					i4	sx;

# ifdef TRACING
					SIfprintf(dbgout, "Diff and !curwin\n");
# endif /* TRACING */
					TDput(*csp++ = *nsp);
					*cda++ = *nda;
					if (xn_loc)
					{
						sx = wx - 1 + invc_wx;
						if (sx == cols_1)
						{
							TDsmvcur(ly, sx, ly,
								sx - 1);
							TDsmvcur(ly, sx - 1,
								ly, sx);
						}
					}
				}
				else
				{
# ifdef TRACING
					SIfprintf(dbgout, "Diff and curwin\n");
# endif /* TRACING */
					TDput(*nsp);
				}
				nsp++;
				nda++;
                           } /** Double, else, Single Processing Ends */

			}
			if (lx == wx + invc_wx && printAble) /* if no change */
				break;
			lx = wx + invc_wx;

			/*
			** IF we are running on an HP terminal in HP mode and
			** we have reached the end of the window (wx > lch) THEN
			** IF we are also at the end of the screen THEN any
			** display attributes have also ended, so we must
			** clear pda and pfonts. ELSE IF we did something with
			** an attribute/font that we needed to save the
			** pointer for, THEN turn off the attribute/font IF
			** it was not continued to the next character.
			*/
			if (XS && wx > lch)
			{

				/* output display attribute if previous
                                ** one is not the same as the current one.
                                */
                                if (pda != *nda  &&
                                   (pda & _DAMASK) == _RVVID)
                                {
                                   TDsmvcur(ly, lx, y, wx + invc_wx);
                                   ly = y;
                                   lx = wx + invc_wx;
                                   fonts = (ndaval = *nda) & _LINEMASK;
                                   da = ndaval & _DAMASK;
                                   color = ndaval & _COLORMASK;

                                   TDdaout(fonts, da, color);
                                }

				if ((wx + invc_wx) > cols_1)
				{
					pda = EOS;
					pfonts = EOS;
				}
				else if (IITDptr_prev)
				{
					if (pda && (pda !=
						(*(IITDptr_prev+1) & _DAMASK)))
					{
						_puts(EA);
						pda = EOS;
					}
					if (pfonts &&
						(pfonts !=
						(*(IITDptr_prev+1)& _LINEMASK)))
					{
						_puts(LE);
						pfonts = EOS;
					}
				}
			}

		}
		else if (wx < lch)
		{
# ifdef TRACING
			SIfprintf(dbgout, "Data same\n");
# endif /* TRACING */
			if (CMGETDBL)
			{


                        if (XS && pda != *nda)
                        {
                           TDsmvcur(ly, lx, y, wx + invc_wx);
                           ly = y;
                           lx = wx + invc_wx;
                           fonts = (ndaval = *nda) & _LINEMASK;
                           da = ndaval & _DAMASK;
                           color = ndaval & _COLORMASK;

                           TDdaout(fonts, da, color);
                        }

			while (!CMcmpcase(nsp,csp) && *nda == *cda && wx < lch)
			{
				if (!_curwin)
				{
					CMnext(csp);
					bytecnt = CMbytecnt(nsp);
/*					*cda++ = *nda; */
					cda++;
					*cdx++ = *ndx;
					if (bytecnt == 2)
					{
/*						*cda++ = *(nda + 1); */
						cda++;
						*cdx++ = *(ndx + 1);
					}
				}
				CMbyteinc(nda,nsp);
				CMbyteinc(ndx,nsp);

                                if (CMprint(nsp))
                                   CMbyteinc(wx,nsp);
                                else
                                   printAble = FALSE; 

				CMnext(nsp);
			}
			}
			else
			{
			if (!lcurwin)
			{
				while (*nsp == *csp && *nda == *cda && wx < lch)
				{
# ifdef TRACING
					SIfprintf(dbgout, "Top of s loop 1\n");
# endif /* TRACING */
					csp++;
					cda++;
				    /*
					*cda++ = *nda;
				    */
					nsp++;
					nda++;
					++wx;
				}
			}
			else
			{
				while (*nsp == *csp && *nda == *cda && wx < lch)
				{
# ifdef TRACING
					SIfprintf(dbgout, "Top of s loop 2\n");
# endif /* TRACING */
					nsp++;
					nda++;
					++wx;
				}
			}
			}
		}
		else
		{
# ifdef TRACING
			SIfprintf(dbgout, "Last else\n");
# endif /* TRACING */

			/*
			** IF we are running on an HP terminal in HP mode and
			** we did something with an attribute/font that we
			** needed to save the pointer for, THEN turn off the
			** attribute/font IF it was not continued to the
			** next character. This handles the case in which
			** the new line and the old are the same from where
			** we were working to the end of the line. The
			** 'sameness' is probably unenhanced blanks.
			*/
			if (XS && IITDptr_prev)     /* HP terminal in HP mode */
			{
				if (pda && (pda !=
					(*(IITDptr_prev+1) & _DAMASK)))
				{
					_puts(EA);
					pda = EOS;
				}
				if (pfonts && (pfonts !=
					(*(IITDptr_prev+1) & _LINEMASK)))
				{
					_puts(LE);
					pfonts = EOS;
				}
			}
			break;
		}
	}
ret:
	return(OK);
}

/*
** these two routines save the last line of the current screen
** if it gets scrolled.
** the global lastLine contains the image of the last line
*/

u_char
TDsaveLast(char *lastLine)
{
	reg	char	*src;
	reg	char	*dest;

	src = curscr->_y[curscr->_maxy - 1];
	dest = lastLine;
	MEcopy(src, (u_i2)(curscr->_maxx), dest);
	return(*(curscr->_da[curscr->_maxy - 1]));
}

/*
** have to save the globals so that the cursor gets position right
** we are in the middle of a refresh, and the screen has been scrolled
** save old globals.  Force a cursor address to lower corner, put
** all blanks there and then put good string.
*/

TDaddLast(lastLine, attr)
char	*lastLine;
u_char	attr;
{
	bool	owin;
	reg	char	*src,
			*dest;
		i4	maxy;
		WINDOW	*o_win;
		WINDOW	*o_pwin;

	o_win = _win;
	o_pwin = _pwin;
	_pwin = _win = curscr;
	owin = _curwin;
	_curwin = 1;
	ly = LINES - 1;
	lx = 0;
	TDsmvcur((i4) 0, COLS - 1, LINES - 1, (i4) 0);
	maxy = curscr->_maxy - 1;
	dest = curscr->_y[maxy];
	src = lastLine;
	MEcopy(src, (u_i2)(curscr->_maxx), dest);
	curscr->_firstch[maxy] = 0;
	curscr->_lastch[maxy] = curscr->_maxx - 1;
	MEfill((u_i2)(curscr->_maxx), attr, curscr->_da[maxy]);
	TDmakech(curscr, maxy);
	_curwin = owin;
	_win = o_win;
	_pwin = o_pwin;
}


TDclrTop()
{
	bool	owin;
	i4	miny;
	WINDOW	*o_win;
	WINDOW	*o_pwin;

	if (IITDflu_firstlineused != 0)
	{
	    o_win = _win;
	    o_pwin = _pwin;
	    _pwin = _win = curscr;
	    owin = _curwin;
	    _curwin = 1;
	    ly = 0;
	    lx = 0;
	    TDsmvcur(LINES - 1, COLS - 1, (i4) 0, (i4) 0);
	    miny = 0;
	    MEfill((u_i2)(curscr->_maxx), ' ', curscr->_y[miny]);
	    curscr->_firstch[miny] = 0;
	    curscr->_lastch[miny] = curscr->_maxx - 1;
	    inclrtop = TRUE;
	    TDmakech(curscr, miny);
	    inclrtop = FALSE;
	    _curwin = owin;
	    _win = o_win;
	    _pwin = o_pwin;
	}
}

/*
** The 2 routines that follows saves the cdx LINE needed
** for doublebyte MENU displays.
*/

void
TDsaveLast_dx(lastline_dx)
char    *lastline_dx;
{
	MEcopy( curscr->_dx[curscr->_maxy - 1],
		(u_i2)curscr->_maxx,
		lastline_dx );
}

void
TDaddLast_dx(lastline_dx)
char    *lastline_dx;
{
	MEcopy( lastline_dx,
		(u_i2)curscr->_maxx,
		curscr->_dx[curscr->_maxy - 1] );
}

