/*
**  VTxydraw.c
**
**  Copyright (c) 2004 Ingres Corporation
**
**  History:
**	08/30/89 (dkh) - Added support for 80/132 feature.
**	10/01/89 (dkh) - Code cleanup.
**	12/30/89 (dkh) - Put in better support for popups on top of layout.
**	07/04/90 (dkh) - Fixed up popups on top of layout frame to work like
**			 normal popups.  Also, put in static option for RBF.
**	01/13/91 (dkh) - Changed IIVTlf() to only update _cury and _curx for
**			 curscr if curscr is actually scrolled.  This will
**			 prevent bad scrolling behavior when cancelling
**			 from the save submenu in layout.
**	06/04/93 (dkh) - Fixed up trigraph complaint from acc.
**      24-sep-96 (mcgem01)
**              Global data moved to vtdata.c
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	"vtframe.h"
# include	<er.h>


static	FRAME	*tfrm = NULL;	/* frame currently displayed in layout */

static	WINDOW	*rbf_bg	= NULL;	/* background for rbf */
static	bool	for_rbf = FALSE;

GLOBALREF	i4	IIVTcbegy;
GLOBALREF	i4	IIVTcbegx;



/*{
** Name:	VTxydraw - Update vifred layout and move cursor to (x,y).
**
** Description:
**	Updated terminal screen with form image and move screen cursor
**	to (x,y).
**
** Inputs:
**	frm	Form to display,
**	y	y coordinate of cursor position.  If -1 then don't move cursor.
**	x	x coordinate of cursor position.
**
** Outputs:
**
**	Returns:
**		None.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	xx/xx/xx (dkh) - Original version.
*/
VOID
VTxydraw(frm, y, x)
FRAME	*frm;
i4	y;
i4	x;
{
	if (y != -1)
	{
		TDmove(frm->frscr, y, x);
	}
	TDtouchwin(frm->frscr);
	TDrefresh(frm->frscr);
}


/*{
** Name:	IIVTgl - Get layout window of vifred.
**
** Description:
**	This routine returns the window for the vifred layout display.
**
** Inputs:
**	None.
**
** Outputs:
**
**	Returns:
**		window	Layout window of vifred.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	08/30/89 (dkh) - Initial version.
*/
WINDOW *
IIVTgl()
{
	WINDOW	*win;

	if (for_rbf)
	{
		win = rbf_bg;
	}
	else
	{
		win = tfrm->frscr;
		win->_starty = IIVTcbegy;
		win->_startx = IIVTcbegx;
	}

	return(win);
}


/*{
** Name:	IIVTlf - Set up to return window for vifred layout display.
**
** Description:
**	Set up to return the window for the vifred layout display
**	to the FT layer.  This will help vifred layout to look and
**	feel more like the normal forms system.
**
** Inputs:
**	frm	Form being displayed in vifred layout.
**	set	TRUE if we are setting up to pass layout window to FT.
**		FALSE if we want to not return layout window.
**	rbf	Special RBF background handling if TRUE.
**
** Outputs:
**
**	Returns:
**		None.
**	Exceptions:
**		None.
**
** Side Effects:
**	tfrm is updated if "set" is TRUE.
**
** History:
**	08/30/89 (dkh) - Initial version.
*/
VOID
IIVTlf(frm, set, rbf)
FRAME	*frm;
i4	set;
i4	rbf;
{
	/*
	**  It should be noted that this code does not support
	**  nesting very well.  This will have to be re-written
	**  if nesting support is desired.  Right now, RBF
	**  doesn't need nesting and Vifred has a limited form
	**  of nesting (when formattr calls visualladjust.
	**  However, since visualladjust is only for popups and
	**  popups are restricted to screen size, we don't
	**  have a problem, IIVTcbegy{x,y} will be set to zero
	**  if visualadjust is called.  This is OK since we
	**  aren't scrolled in this situation any way.
	*/
	if (set)
	{
		IIFTsvlSetVifredLayout(IIVTgl);
		tfrm = frm;
		tfrm->frscr->_starty = IIVTcbegy;
		tfrm->frscr->_startx = IIVTcbegx;

		for_rbf = FALSE;

		if (rbf)
		{
			for_rbf = TRUE;
			if (rbf_bg == NULL)
			{
				if ((rbf_bg = TDnewwin(LINES - 1, COLS, 0,
					0)) == NULL)
				{
				    IIUGbmaBadMemoryAllocation(ERx("IIVTlf"));
				}
			}
			TDoverlay(curscr, 0, 0, rbf_bg, 0, 0);
		}
	}
	else
	{
		IIFTsvlSetVifredLayout(NULL);
		tfrm->frscr->_starty = 0;
		tfrm->frscr->_startx = 0;

		/*
		**  Only update _cur{x,y} if curscr was actually
		**  scrolled (i.e., IIVTcbeg(x,y} were negative).
		*/
		if (((curscr->_begy = IIVTcbegy) < curscr->_cury) &&
			IIVTcbegy < 0)
		{
			curscr->_cury = - IIVTcbegy;
		}
		if (((curscr->_begx = IIVTcbegx) < curscr->_curx) &&
			IIVTcbegx < 0)
		{
			curscr->_curx = - IIVTcbegx;
		}
	}
}
