/*
**	ftsyndsp.c
**
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:	ftsyncdsp.c - Syncrhonize window with form.
**
** Description:
**	This file contains routines to synchronize a window with a
**	form.  Also, routines to handle popup style forms are
**	located in this file.
**
** History:
**	04/20/88 (dkh) - Initial version for Venus.
**	05/11/88 (dkh) - Added floating popup calculation and
**			 boundary tests on size of a form.
**	05/12/88 (dkh) - Fixed floating popup algorithm.
**	05/17/88 (dkh) - Adjusted rounding problems in float algorithm.
**	05/27/88 (dkh) - Fixed problem with finding old pointers.
**	05/27/88 (dkh) - Fixed so that popups with no borders also work.
**	06/15/88 (dkh) - Fixed venus bug 2759.
**	06/20/88 (dkh) - Fixed problem with popup as first form displayed.
**	06/22/88 (dkh) - Took back some optimization so that full
**			 screen forms are rebuilt more often.
**	07/09/88 (dkh) - Added new error messages.
**	07/09/88 (dkh) - Changed code to always rebuild display.
**	07/14/88 (dkh) - Changed floating algorithm per FRC request.
**	07/21/88 (dkh) - Added invisiblity to make "help" work in vifred.
**	07/23/88 (dkh) - Modified for printscreen support, etc.
**	09/05/88 (dkh) - Fixed typos to set curscr->_beg{x,y} correctly
**			 when removing a popup and to correctly calculate
**			 the last line occupied by a popup.
**	09/13/88 (dkh) - Fixed venus bug 3278.
**	09/23/88 (dkh) - Fixed venus bug 3384.
**	10/13/88 (dkh) - Added code to clear display list if a NULL frame
**			 pointer is passed to FTsyncdisp.
**	05-jan-89 (bruceb)
**		Add additional parameter to FTbuild call to indicate that
**		its caller didn't originate with Printform.
**	14-jan-89 (bruceb)
**		IIFTrdRebuildDisplay calls FTbuild on popups that have
**		fdREBUILD set.  This is so that field may be made
**		(in)visible within an activation prior to returning to
**		the display loop.
**	01/19/89 (dkh) - Added fix to always set up box/trim graphics
**			 correctly for a popup.  Also fixed stale
**			 pointer problem for ftfldupd.c.
**	02/01/89 (dkh) - Fixed venus bug 4582.
**	02/01/89 (dkh) - Fixed venus bug 4583.
**	02/02/89 (dkh) - Fixed venus bug 4406.
**	03/23/89 (dkh) - Changed ovwin to be an absolute window so
**			 redisplay of a (vertically scrolled) form works.
**	04/10/89 (dkh) - Optimized IIFTrRedisp() to not rebuild screen
**			 image if most recent display is a full screen form.
**	26-apr-89 (bruceb)	Fix for bug 5399.
**		If displaying form for 'first' time, and fullscreen style,
**		set curscr->_begy and _begx to 0.  Just in case the
**		underlying fullscreen form is scrolled.
**	05/17/89 (dkh) - Fixed bug 3757.
**	08/04/89 (dkh) - Added support for 80/132 runtime switching.
**	08/15/89 (dkh) - Added better support for calling popups from layout.
**	18-aug-89 (bruceb)
**		Process aggregate dependencies on a ##redisplay.
**	08/30/89 (dkh) - Removed optimization in IIFTrRedisp() to fix bug 7587.
**	08/30/89 (dkh) - Put in some patches to better handle displaying
**			 big forms for the first time.
**	09/01/89 (dkh) - Changed to force a popup form to fullscreen if
**			 it is too big to display as a popup.
**	09/23/89 (dkh) - Integrated bug fix from emerald.
**	09/23/89 (dkh) - Porting changes integration.
**	09/29/89 (dkh) - Code cleanup.
**	10/02/89 (dkh) - Changed IIFTscSizeChg() to IIFTcsChgSize().
**	11/14/89 (dkh) - Fix IIFTcsChgSize() so it also works if dlcur is NULL.
**	12/27/89 (dkh) - Put in check so fixed position popups don't overlap
**			 DG/CEO line.
**	12/28/89 (dkh) - Fixed drawing problems with scrolled forms.
**	12/30/89 (dkh) - Fixed problem with values in popup not being
**			 displayed if they were set from a fullscreen
**			 display loop covering the popup.
**	01/04/90 (dkh) - Fix from 12/28/89 also fixes jup bug 8869.
**	01/10/90 (dkh) - Changed IIFTrdRebuildDisplay to handle invisiblity
**			 of a form even if it is a popup.
**	01/19/90 (dkh) - Integrated above fix from 6.4 to fix bug 8869.
**	01/20/90 (dkh) - Integrated 12/30/89 fix from 6.4 into 6.3.
**	01/24/90 (dkh) - If form is behind vifred layout or has fdDISPRBLD set,
**			 then force a IIFTrdRebuildDisplay() rather than
**			 just refreshing FTwin.
**	03/01/90 (dkh) - Adjust size of ovwin so that it is in sync with
**			 curscr width.  This is necessary for 80/132 support.
**	03/26/90 (dkh) - Fixed IIFTrRedisp() to restore evcb->eval_aggs
**			 correctly before exiting.
**	03/27/90 (dkh) - Fixed bug 20323.
**	02-apr-90 (bruceb)
**		FTfrm set on successful FTsyncdisp.  Used for locator support.
**	04/04/90 (dkh) - Integrated MACWS changes.
**	04/14/90 (dkh) - Integrated changes from 6.4 to fix bug 9798.
**	04/27/90 (dkh) - Added support for popups in scrollable output.
**	31-may-90 (rog)
**		Included <me.h> so MEfill() is defined correctly.
**	07/04/90 (dkh) - Fixed up popups on top of vifred layout so they
**			 look and behave like normal popups.
**	07/10/90 (dkh) - Added code to check if prwin needs to be reallocated.
**			 This becomes necessary if FTfullwin grows in size.
**			 "prwin" needs to have larger arrays in this case
**	07/24/90 (dkh) - A more complete fix for bug 30408.
**	08/29/90 (dkh) - Integrated porting change ingres6202p/131520.
**	14-sep-90 (bruceb)
**		When making a form invisible, turn off the fdDISPRBLD
**		flag so that VIFRED won't clear the layout form on the
**		first 'create' attempt.
**	19-jun-91 (mgw) Bug #7587
**		Re-implement 08/30/89 fix to remove optimization in
**		IIFTrRedisp(). It was accidentally undone when integrating
**		ingres64 changes into the ingres63p line.
**	08/17/91 (dkh) - Rolled back above change for bug 7587 due to
**			 impact on testing.  Fix for the problem is now
**			 in runtime!iiredisp.c which should reduce the
**			 number of test cases that are broken.
**	09/18/92 (dkh) - Fixed bug 46486.  Printing the screen with a
**			 popup as the top most form will now capture
**			 the entered string into the current field even
**			 if the string has not been converted into an
**			 internal value.
**	11/18/92 (dkh) - Fixed bug 47981.  Added check to account for
**			 a NULL parent for popup windows so we don't
**			 try to reference through a NULL pointer.
**	07/10/93 (dkh) - Fixed bug 50811.  Added check for popups with
**			 no borders to correctly build the image for
**			 the printscreen command.
**      11/30/94 (harpa06) - Integrated bug fix #64984 in 6.4 codeline by 
**                           cwaldman: ABF popup frames appear overlayed in 
**                           screendump. Reset twin each time this routine
**                           is called (instead of first time only). Thus,
**                           twin contains top popup only and overlay is
**                           avoided. (Module IIFTpbPrscrBld())
**      23-sep-96 (mcgem01)
**              Global data moved to ftdata.c
**               
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	22-Mar-2005 (lakvi01)
**	    Type-casted NULL to (WINDOW *) in call to TDsubwin.
**/

# include	<compat.h>
# include	<nm.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	"ftframe.h"
# include	<frsctblk.h>
# include	<me.h>
# include	<si.h>
# include	<er.h>
# include	<erft.h>
# include	<scroll.h>
# include	<ug.h>

typedef struct _ftdlist
{
	FRAME		*frm;		/* form attached to a display */
	WINDOW		*win;		/* window to display form in */
	struct _ftdlist	*prev;		/* previous (parent) display */
	struct _ftdlist	*next;		/* next (child) display */
	i4		cbegy;		/* curscr begy offset - 0 based*/
	i4		cbegx;		/* curscr begx offset - 0 based*/
	i4		iposy;		/* current item start posy - 0 based*/
	i4		iposx;		/* current item start posx - 0 based*/
	i4		imaxy;		/* current item size (y) in form */
	i4		imaxx;		/* current item size (x) in form */
	i4		flags;		/* attribute flag */
} FTDLIST;

static	FTDLIST	*dlfreelist = NULL;
static	FTDLIST	*dlcur = NULL;
static	WINDOW	*ovwin = NULL;

static	bool	rebuild_disp = FALSE;

FUNC_EXTERN	VOID	IIFTrfuoResetFldUpdOpt();
FUNC_EXTERN	FTDLIST	*IIFTdlaDispListAlloc();
FUNC_EXTERN	FTDLIST	*IIFTfsfFullScreenForm();
FUNC_EXTERN	VOID	IIFTfdlFreeDispList();
FUNC_EXTERN	VOID	IIFTpdlPopDispList();
FUNC_EXTERN	VOID	IIFTgscGetScrSize();
FUNC_EXTERN	VOID	IIFTmscMenuSizeChg();
FUNC_EXTERN	i4	IITDscSizeChg();

GLOBALREF	i4	IITDflu_firstlineused;
GLOBALREF	WINDOW	*FTfullwin;
GLOBALREF	FRS_EVCB	*IIFTevcb;
GLOBALREF	bool	IITDiswide;
GLOBALREF	bool	IITDccsCanChgSize;
GLOBALREF	i4	IITDAllocSize;
GLOBALREF	i4	YN;
GLOBALREF	i4	YO;
GLOBALREF	FRAME	*FTfrm;

static	i4	poplastline = 0;
static	i4	prscrmaxy = 0;

/*
**  Pointer to call back routine to display vifred layout.
*/
GLOBALREF	WINDOW	*(*IIFTdvl)();

/*
**  Flag to indicate the display structure is for
**  a popup style form or an opaque display.
*/
# define	FTISPOPUP	01
# define	FTISINVIS	02
# define	FTNARDISP	04
# define	FTWIDDISP	010


GLOBALREF	bool	samefrm;



/*{
** Name:	IIFTsccSetCurCoord - Set Current Screen Coordiantes.
**
** Description:
**	This routine saves away the current screen beginning coordinates
**	_begy and _begx into the current FTDLIST structure member
**	pointed to by "dlcur".  This allows us to draw the screen correctly if
**	a full screen style form (that has scrolled) calls a popup style form.
**
** Inputs:
**	None.
**
** Outputs:
**	None.
**
**	Returns:
**		None.
**	Exceptions:
**		None.
**
** Side Effects:
**	Updates the cbegy and cbegx values in the structure pointed
**	to by var dlcur.
**
** History:
**	05/06/88 (dkh) - Initial version.
*/
VOID
IIFTsccSetCurCoord()
{
	dlcur->cbegx = curscr->_begx;
	dlcur->cbegy = curscr->_begy;
}


/*{
** Name:	IIFTdlaDispListAlloc - Allocate a FTDLIST structure.
**
** Description:
**	This routine allocates a FTDLIST structure.  It will
**	try to get it from a free list if possible.  Otherwise
**	it allocates it via a call to FEreqmem().
**
** Inputs:
**	None.
**
** Outputs:
**	Returns:
**		ptr	Pointer to a FTDLIST structure.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	05/01/88 (dkh) - Initial version.
*/
FTDLIST *
IIFTdlaDispListAlloc()
{
	FTDLIST	*dptr;

	if (dlfreelist == NULL)
	{
		if ((dptr = (FTDLIST *) MEreqmem((u_i4) 0,
			(u_i4) sizeof(FTDLIST), TRUE,
			(STATUS *) NULL)) == NULL)
		{
			IIUGbmaBadMemoryAllocation(ERx("IIFTdla"));
			return(NULL);
		}
	}
	else
	{
		dptr = dlfreelist;
		dlfreelist = dptr->next;
		MEfill((u_i2) sizeof(FTDLIST), (u_char) '\0', (PTR) dptr);
	}

	return(dptr);
}



/*{
** Name:	IIFTfdlFreeDispList - Free a FTDLIST structure.
**
** Description:
**	This routine frees the FTDLIST structure that is passed to it.
**	The FTDLIST structure is simply added to the free list.
**
** Inputs:
**	ptr	Pointer to FTDLIST structure to free.
**
** Outputs:
**	None.
**
**	Returns:
**		None.
**	Exceptions:
**		None.
**
** Side Effects:
**	Free list for FTDLIST structures is updated.
**
** History:
**	05/02/88 (dkh) - Initial version for Venus.
*/
VOID
IIFTfdlFreeDispList(disp)
FTDLIST	*disp;
{
	if (dlfreelist == NULL)
	{
		dlfreelist = disp;
		dlfreelist->next = NULL;
	}
	else
	{
		disp->next = dlfreelist;
		dlfreelist = disp;
	}
}



/*{
** Name:	IIFTpdlPopDispList - Pop off current display list.
**
** Description:
**	Pop (i.e., remove) the passed in FTDLIST structure.  The
**	structure itself is free'ed by calling IIFTfdlFreeDispList().
**	The window for a popup style window is freed by calling
**	TDdelwin().
**
** Inputs:
**	disp	Pointer to FTDLIST structure to free.
**
** Outputs:
**	None.
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
**	05/01/88 (dkh) - Initial version for Venus.
*/
VOID
IIFTpdlPopDispList(disp)
FTDLIST	*disp;
{
	WINDOW	*win;

	/*
	**  Remove special rebuild display flags if we are
	**  popping off the form.
	*/
	disp->frm->frmflags &= ~fdDISPRBLD;

	if (disp->flags & FTISPOPUP)
	{
		/*
		**  Reset optimization information in
		**  in FTfldupd().
		*/
		IIFTrfuoResetFldUpdOpt(disp->win);

		/*
		**  Have to free the parent if it exists
		**  since that's where the real allocation
		**  was made from.
		*/
		if (disp->win->_parent)
		{
			win = disp->win->_parent;
		}
		else
		{
			win = disp->win;
		}

		TDdelwin(win);
	}

# ifdef DATAVIEW
	_VOID_ IIMWdfDelForm(disp->frm);
# endif	/* DATAVIEW */

	IIFTfdlFreeDispList(disp);
}


/*{
** Name:	FTfrminvis - Make current display invisible.
**
** Description:
**	Make the current display invisible so that it will be skip on any
**	rebuilds.
**
** Inputs:
**	None.
**
** Outputs:
**	None.
**
**	Returns:
**		None.
**	Exceptions:
**		None.
**
** Side Effects:
**
** History:
**	07/20/88 (dkh) - Initial version.
**	16-feb-89 (mgw) Bug #4769
**		check for NULL dlcur before referencing through it to prevent
**		Access Violation.
*/
VOID
FTfrminvis()
{
	if (dlcur != NULL)
	{
		dlcur->flags |= FTISINVIS;
		if (dlcur->frm)
		{
		    dlcur->frm->frmflags &= ~fdDISPRBLD;
		}
	}
}



/*{
** Name:	FTfrmvis - Make the current display visible again.
**
** Description:
**	Make the current display visible again so that it will be
**	part of any rebuilds.
**
** Inputs:
**	None.
**
** Outputs:
**	None.
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
**	07/20/88 (dkh) - Initial version.
**	16-feb-89 (mgw) Bug #4769
**		check for NULL dlcur before referencing through it to prevent
**		Access Violation.
*/
VOID
FTfrmvis()
{
	if (dlcur != NULL)
		dlcur->flags &= ~FTISINVIS;
}





/*{
** Name:	IIFTfsfFullScreenForm - Find FTDLIST structure for full screen.
**
** Description:
**	Find the most recent full screen form and return the FTDLIST
**	structure associated with it.  If there are no full screen
**	forms being displayed a NULL pointer is returned.
**	If parameter "stopon_opaque" is TRUE, then stop at either the
**	a full screen form or a "opaque" display, whichever occurs
**	first.  A "opaque" display is a trick for vifred/rbf which
**	escapes out of the forms system now and then.
**
** Inputs:
**	stopon_psuedo	Can stop at a "opaque" display also.
**
** Outputs:
**
**	Returns:
**		ptr	Pointer to FTDLIST structure for most recent full
**			screen form.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	05/02/88 (dkh) - Initial version for Venus.
*/
FTDLIST *
IIFTfsfFullScreenForm()
{
	FTDLIST	*disp;
	FTDLIST	*prev = NULL;

	for (disp = dlcur; disp != NULL && disp->flags & FTISPOPUP; )
	{
		prev = disp;
		disp = disp->prev;
	}

	if (disp == NULL)
	{
		disp = prev;
	}

	return(disp);
}




/*{
** Name:	IIFTgdiGetDispInfo - Get Current Display Information.
**
** Description:
**	Returns information about the current display.  Information
**	consists of whether the display is full/popup and the curscr
**	offsets.
**
** Inputs:
**	None.
**
** Outputs:
**	isfull	Whether the current display is full/popup.
**	yoffset	The curscr offset for the y coordinate.
**	xoffset	The curscr offset for the x coordinate.
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
**	07/23/88 (dkh) - Initial version.
*/
VOID
IIFTgdiGetDispInfo(isfull, yoffset, xoffset)
i4	*isfull;
i4	*yoffset;
i4	*xoffset;
{
	if (dlcur->flags & FTISPOPUP)
	{
		*isfull = FALSE;
	}
	else
	{
		*isfull = TRUE;
		*yoffset = dlcur->cbegy;
		*xoffset = dlcur->cbegx;

	}
}



/*{
** Name:	IIFTwffWindowForForm - Find display window for a form.
**
** Description:
**	Given a form, find and return its associated display window.
**	A NULL pointer is returned if the passed in form can not be
**	found in the display list that is displayed after the most
**	recent full screen form.  This means that the form can not
**	be visible.  Routine IIFTfsfFullScreenForm() is called to
**	find the most recent full screen form.
**
**
** Inputs:
**	frm	Pointer to form for finding its associated display window.
**	ignore_full	Indicates if we should ignore a full screen form
**			in scan.
**
** Outputs:
**
**	Returns:
**		win	Window used for displaying passed in form.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	05/04/88 (dkh) - Initial version for Venus.
*/
WINDOW *
IIFTwffWindowForForm(frm, ignore_full)
FRAME	*frm;
i4	ignore_full;
{
	FTDLIST	*disp;
	WINDOW	*win = NULL;

	if (ignore_full)
	{
		disp = dlcur;
		while (disp != NULL)
		{
			if (disp->frm == frm)
			{
				win = disp->win;
				break;
			}
			disp = disp->prev;
		}
	}
	else
	{
		if ((disp = IIFTfsfFullScreenForm()) != NULL)
		{
			/*
			**  Look for form until we run out.
			*/
			while (disp != NULL)
			{
				if (disp->frm == frm)
				{
					win = disp->win;
					break;
				}
				disp = disp->next;
			}
		}
	}

	return(win);
}



/*{
** Name:	IIFTrdRebuildDisplay - Rebuild display for all visible forms.
**
** Description:
**	Rebuild the display for all forms that are displayed after the
**	most recent full screen form.  The display is built into
**	window "ovwin".
**
** Inputs:
**	None.
**
** Outputs:
**
**	Returns:
**		OK	Rebuilding of the display completed.
**		FAIL	Rebuilding the display failed.
**	Exceptions:
**		None.
**
** Side Effects:
**	Memory for window ovwin may be allocated.
**
** History:
**	05/06/88 (dkh) - Initial version for Venus.
*/
STATUS
IIFTrdRebuildDisplay()
{
	FTDLIST	*disp;
	FTDLIST	*idisp;
	WINDOW	*win;
	WINDOW	*svwin;
	i4	sy;
	i4	sx;
	WINDOW	*vfwin;
	i4	omaxy;

	prscrmaxy = 0;
	if (ovwin == NULL)
	{
		if ((ovwin = TDnewwin(LINES - 1, IITDAllocSize, 0, 0)) == NULL)
		{
			return(FAIL);
		}
		ovwin->_relative = FALSE;
		ovwin->_starty = IITDflu_firstlineused;
	}

	ovwin->_maxx = curscr->_maxx;

	/*
	**  Find the most recent full screen form.
	*/
	disp = IIFTfsfFullScreenForm();

	/*
	**  Check for most recent invisible form which may
	**  be on top of a fullscreen form.
	*/
	for (idisp = disp; idisp; idisp = idisp->next)
	{
		if (idisp->flags & FTISINVIS)
		{
			disp = idisp;
			break;
		}
	}

	TDerase(ovwin);

	idisp = disp;

	for (;;)
	{
		/*
		**  Make distinction between visible/invisible form.
		**  If we have an invisible form or if there is NO
		**  fullscreen backgroun form, then check to see
		**  if there is a special pointer for an alternate
		**  background form.  The latter case is to conver
		**  scrollable output that will not have any real
		**  forms system fullscreen form behind a popup.
		*/
		if (disp->flags & FTISINVIS || ((disp->flags & FTISPOPUP) &&
			disp->prev == NULL))
		{
			/*
			**  If there is a call back pointer for
			**  Vifred, call it to get window for
			**  layout frame and overlay it into ovwin.
			*/
			if (IIFTdvl)
			{
				vfwin = (*IIFTdvl)();
				/*
				**  Overlay vifred window into "ovwin".
				**  Assumes that curscr has correct
				**  offset.
				curscr->_begy = vfwin->_starty;
				curscr->_begx = vfwin->_startx;
				*/
				TDoverlay(vfwin, -vfwin->_starty,
					-vfwin->_startx, ovwin, 0, 0);

				/*
				**  Reset since this information is
				**  only used to do overlay.  Routine
				**  pointed to by IIFTdvl will set
				**  these values again when called.
				*/
				vfwin->_starty = vfwin->_startx = 0;
			}
		}
		if ((disp->flags & FTISPOPUP && disp->prev == NULL) ||
			!(disp->flags & FTISINVIS))
		{
			/*
			**  Overlay the forms into "ovwin".  This should
			**  be what the terminal screen should look like.
			*/
			win = disp->win;
			if (disp->flags & FTISPOPUP)
			{
				if (win->_parent)
				{
					omaxy = win->_parent->_begy +
						win->_parent->_maxy;
				}
				else
				{
					omaxy = win->_starty + win->_maxy;
				}
				if (omaxy > prscrmaxy)
				{
					prscrmaxy = omaxy;
				}
				sy = win->_starty;
				sx = win->_startx;
				svwin = win;
				if (win->_parent)
				{
					sy--;
					sx--;
					win = win->_parent;
				}
				/*
				** In order to display tf highlighting changes,
				** etc. for a popup, we need to be less
				** optimal and always rebuild popups too.
				*/
				disp->frm->frmflags |= fdREBUILD;
				if (FTbuild(disp->frm, FALSE, svwin,
					(bool)FALSE) == OK)
				{
					TDboxfix(svwin);
					TDoverlay(win, 0, 0, ovwin,
					    sy - ovwin->_starty, sx);
				}
			}
			else
			{
				/*
				**  Need to rebuild full screen display
				**  since we are just sharing one window
				**  for all full screen forms.
				*/
				if (FTbuild(disp->frm, FALSE, win,
				    (bool)FALSE) == OK)
				{
					TDboxfix(win);
					TDoverlay(win, -disp->cbegy,
						-disp->cbegx, ovwin, 0, 0);
				}
			}
		}
		if (disp == dlcur)
		{
			break;
		}

		disp = disp->next;
	}

	return(OK);
}




/*{
** Name:	IIFTrRedisp - Redisplay all visible form.
**
** Description:
**	This routines redisplays all visible forms to the terminal screen.
**	It calls IIFTrdRebuilddisplay() to do the work of building an
**	image of all the visible forms.
**
** Inputs:
**	None.
**
** Outputs:
**	None.
**
**	Returns:
**		None.
**	Exceptions:
**		None.
**
** Side Effects:
**	Window "ovwin" is updated.
**
** History:
**	05/06/88 (dkh) - Initial version for Venus.
**	19-jun-1991 (mgw) Bug #7587
**		Remove optimization for proper highlighting after REDISPLAY.
*/
VOID
IIFTrRedisp()
{
	FTDLIST	*disp;
	bool	eval_aggs = IIFTevcb->eval_aggs;

	IIFTevcb->eval_aggs = TRUE;

	if (IIFTdvl)
	{
		if (IIFTrdRebuildDisplay() == OK)
		{
			TDtouchwin(ovwin);
			ovwin->_flags |= _BOXFIX;
			TDmove(ovwin, 0, 0);
			TDrefresh(ovwin);
		}
	}
	else if (!(dlcur->flags & (FTISPOPUP | FTISINVIS)) &&
		!(dlcur->frm->frmflags & fdREBUILD) &&
		!(dlcur->frm->frmflags & fdDISPRBLD))
	{
		(*IIFTpadProcessAggDeps)(dlcur->frm);
		TDtouchwin(FTwin);
		TDrefresh(FTwin);
	}
	else if (IIFTrdRebuildDisplay() == OK)
	{
		/*
		** Process aggregates for all displayed forms, even if
		** invisible.
		*/
		for (disp = dlcur;
			(disp != NULL) && (disp->flags & FTISPOPUP);
			disp = disp->prev)
		{
			(*IIFTpadProcessAggDeps)(disp->frm);
		}
		/* If not NULL, then it's the full-screen form. */
		if (disp != NULL)
		{
			(*IIFTpadProcessAggDeps)(disp->frm);
		}

		TDtouchwin(ovwin);

		/*
		**  Set box fixup flag in case user does
		**  a redisplay before we have had a chance
		**  to display the form.
		*/
		ovwin->_flags |= _BOXFIX;
		TDmove(ovwin, 0, 0);
		TDrefresh(ovwin);
	}

	IIFTevcb->eval_aggs = eval_aggs;
}



/*{
** Name:	IIFTsiiSetItemInfo - Set current item information.
**
** Description:
**	Save positional and size information about the curren item
**	into the FTDLIST structure.  Passed in values are ZERO based
**	and relative to the form.  When these values are used, they
**	will be translated into screen coordinates.
**
** Inputs:
**	iposx	X position of current item in form.
**	iposy	Y position of current item in form.
**	imaxx	X size of current item in form.
**	imaxy	Y size of current item in form.
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
**	05/11/88 (dkh) - Initial version for Venus.
*/
VOID
IIFTsiiSetItemInfo(iposx, iposy, imaxx, imaxy)
i4	iposx;
i4	iposy;
i4	imaxx;
i4	imaxy;
{
	dlcur->iposx = iposx;
	dlcur->iposy = iposy;
	dlcur->imaxx = imaxx;
	dlcur->imaxy = imaxy;
}



/*{
** Name:	IIFTfpfFitPopupForm - Fit popup form in screen display.
**
** Description:
**	This routine will check the passed in form to see if it
**	will actually fit on the screen as a popup form.  If
**	not, it will try to adjust the starting location.  If
**	that fails, then we give up.  Note that if the starting
**	location is 0, 0, we don't adjust the starting location.
**	Another routine will be used to handle floating positions.
**	We also assume that we are passed a popup style form.
**
**	Note that calculations in this routine are ONE based.
**
**	Return values are defined in frame.h
**
** Inputs:
**	frm	Form to check.
**
** Outputs:
**
**	Returns:
**		PFRMFITS	Form fits where it is.
**		PFRMMOVE	Form had to be moved but still fits.
**		PFRMSIZE	Form is too large to be used as a popup.
**	Exceptions:
**		None.
**
** Side Effects:
**	Starting location of form may be changed.
**
** History:
**	05/11/88 (dkh) - Initial version for Venus.
*/
i4
IIFTfpfFitPopupForm(frm)
FRAME	*frm;
{
	i4	retval = PFRMFITS;
	i4	maxx;
	i4	maxy;
	i4	posx;
	i4	posy;
	i4	endx;
	i4	endy;

	/*
	**  Determine size of popup form.
	*/
	maxx = frm->frmaxx;
	maxy = frm->frmaxy;
	if (frm->frmflags & fdBOXFR)
	{
		maxx += 2;
		maxy += 2;
	}

	/*
	**  Form is too large, we give up.
	*/
	if (maxx > COLS || maxy > LINES - 1 - IITDflu_firstlineused)
	{
		return(PFRMSIZE);
	}

	/*
	**  If start locations are negative, make them floating
	**  locations.
	*/
	if ((posy = frm->frposy) < 0)
	{
		posy = frm->frposy = 0;
	}

	if ((posx = frm->frposx) < 0)
	{
		posx = frm->frposx = 0;
	}

	/*
	**  If fixed starting location overlaps DG/CEO line, then
	**  shift it down.  Floating calculations already take the
	**  DG/CEO line into account when calculating starting location.
	*/
	if (posy > 0 && posy <= IITDflu_firstlineused)
	{
		posy = frm->frposy = IITDflu_firstlineused + 1;
	}

	if (posy == 0 || posx == 0)
	{
		/*
		**  One of the coordinates is floating, return good news.
		*/
		return(retval);
	}

	/*
	**  Now check to see if form fits at desired location.
	*/
	endy = posy + maxy;
	endx = posx + maxx;

	if (endx > COLS + 1)
	{
		frm->frposx = posx - (endx - COLS + 1);
		retval = PFRMMOVE;
	}
	if (endy > LINES - 1 + 1)
	{
		frm->frposy = posy - (endy - (LINES - 1 + 1));
		retval = PFRMMOVE;
	}

	return(retval);
}



/*{
** Name:	IIFTcfCheckFit - Check if pass in coordinates fit on screen.
**
** Description:
**	Given starting coordinates and lengths, check to see if
**	the calculated endpoints will fit on the screen.
**
**	Note that calculations in this routine are ONE based.
**
** Inputs:
**	posx	X start corrdinate.
**	posy	Y start coordinate.
**	maxx	X length.
**	maxy	Y length.
**
** Outputs:
**
**	Returns:
**		TRUE	If the calculated endpoints fit.
**		FALSE	One or both the endpoints will not fit.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	05/11/88 (dkh) - Initial version for Venus.
*/
bool
IIFTcfCheckFit(posx, posy, maxx, maxy)
i4	posx;
i4	posy;
i4	maxx;
i4	maxy;
{
	if (posx + maxx > COLS + 1)
	{
		return(FALSE);
	}

	if (posy + maxy > LINES - 1 + 1)
	{
		return(FALSE);
	}

	return(TRUE);
}



/*{
** Name:	IIFTsfpSetFloatPos - Set floating position for a form.
**
** Description:
**	This routine will set the floating position of the passed in
**	form.  Note that only one of the coordinates need to be
**	floating.  But we will also adjust the non-floating one
**	to make sure the forms fits on the screen.  In this case
**	the retrun value is PFRMMOVE.
**
**	Note that calculations in this routine are ONE based.
**	Position that is set before return are ZERO based.
**
**	Return values are defined in frame.h
**
** Inputs:
**	form	Form to set floating position on.
**
** Outputs:
**
**	Returns:
**		PFRMFITS	Form fits with no problem.
**		PFRMMOVE	A non-floating coordinate was moved to fit.
**	Exceptions:
**		None.
**
** Side Effects:
**	May changed started location of form.
**
** History:
**	05/11/88 (dkh) - Initial version for Venus.
*/
i4
IIFTsfpSetFloatPos(frm)
FRAME	*frm;
{
	FTDLIST	*prev;
	i4	retval = PFRMFITS;
	i4	maxx;
	i4	maxy;
	i4	posx;
	i4	posy;
	i4	imaxx = 0;
	i4	imaxy = 0;
	i4	iposx = 0;
	i4	iposy = 0;
	i4	endx;
	i4	endy;
static	i4	floatoption = 0;
static	i4	readoption = FALSE;
	char	*floatopt;

	if (!readoption)
	{
		NMgtAt(ERx("II_FLOAT_OPT"), &floatopt);
		if (floatopt != NULL)
		{
			/*
			**  Only accept options 0 or 1.
			*/
			if (*floatopt == 'A')
			{
				floatoption = 0;
			}
			else if (*floatopt == 'B')
			{
				floatoption = 1;
			}
		}
		readoption = TRUE;
	}

	maxx = frm->frmaxx;
	maxy = frm->frmaxy;
	if (frm->frmflags & fdBOXFR)
	{
		maxx += 2;
		maxy += 2;
	}

	posy = frm->frposy;
	posx = frm->frposx;

	prev = dlcur->prev;

	/*
	**  Values in iposx and iposy are already
	**  in 1 index scheme, if they exist.
	*/
	if (prev != NULL)
	{
		iposx = prev->iposx + prev->cbegx;
		iposy = prev->iposy + prev->cbegy;
		imaxx = prev->imaxx;
		imaxy = prev->imaxy;
	}

	if (posy == 0 && posx == 0)
	{
		/*
		**  Adjust both coordinates at same time.
		*/

		if (prev == NULL || imaxy == 0)
		{
			/*
			**  If no previous form or if the parent form
			**  contained no fields that cursor can land
			**  on then put at upper left corner.
			*/

			/*
			**  Go to zero based positioning before return.
			*/
			frm->frposy = IITDflu_firstlineused;
			frm->frposx = 0;
			return(retval);
		}
		else
		{
		    if (!floatoption)
		    {
			/*
			**  First try popup below current item in
			**  previous form.
			*/
			if ((posy = iposy + imaxy) < 1)
			{
				posy = 1;
			}

			/*
			**  If beginning of field is off screen
			**  right now, bring popup into view by
			**  placing it at left edge of terminal
			**  screen.
			*/
			if ((posx = iposx) < 1)
			{
				posx = 1;
			}

			if (IIFTcfCheckFit(posx, posy, maxx, maxy))
			{

				/*
				**  Go to zero based positioning before return.
				*/
				frm->frposx = posx - 1;
				frm->frposy = posy - 1;
				return(retval);
			}
			else
			{
				/*
				**  Check to see if we fit vertically.
				**  If so, we just slide form horizontally
				**  to fit at right edge of terminal
				**  screen.
				*/
				if (posy + maxy <= LINES)
				{
					posx = COLS - maxx + 1;
					/*
					**  Go to zero based positioning
					**  before return.
					*/
					frm->frposx = posx - 1;
					frm->frposy = posy - 1;
					return(retval);
				}
			}

			/*
			**  Try putting it to the right.
			**  The popup is centered with
			**  respect to current item in
			**  parent form.
			*/
			if (maxy >= imaxy)
			{
				posy = iposy - ((maxy - imaxy)/2 + 1);
			}
			else
			{
				posy = iposy + (imaxy - maxy)/2 + 1;
			}

			/*
			**  Check boundaries above and below.
			*/
			if (posy <= IITDflu_firstlineused)
			{
				posy = IITDflu_firstlineused + 1;
			}
			else if (posy + maxy > LINES)
			{
				posy = LINES - 1 - maxy + 1;
			}

			posx = iposx + imaxx;
			if ((posx + maxx) <= COLS)
			{
				/*
				**  Go to zero based positioning before return.
				*/
				frm->frposx = posx - 1;
				frm->frposy = posy - 1;
				return(retval);
			}

			/*
			**  Try putting popup above.
			*/
			if ((posy = iposy - maxy) > IITDflu_firstlineused)
			{
				if ((posx = iposx) < 1)
				{
					posx = 1;
				}
				if (IIFTcfCheckFit(posx, posy, maxx, maxy))
				{
					/*
					**  Go to zero based positioning
					**  before return.
					*/
					frm->frposx = posx - 1;
					frm->frposy = posy - 1;
					return(retval);
				}
				else
				{
					/*
					**  We must fit above, otherwise
					**  we would not be here.  So
					**  adjust horizontally.
					*/
					posx = COLS - maxx + 1;

					frm->frposx = posx - 1;
					frm->frposy = posy - 1;
					return(retval);
				}
			}

			/*
			**  Will put to right even if it overlaps.
			**  Popup is centered vertically with respect
			**  to current item in parent form.
			*/
			posx = COLS - maxx + 1;

			if (maxy >= imaxy)
			{
				posy = iposy - ((maxy - imaxy)/2 + 1);
			}
			else
			{
				posy = iposy + (imaxy - maxy)/2 + 1;
			}

			/*
			**  Check boundaries above and below.
			*/
			if (posy <= IITDflu_firstlineused)
			{
				posy = IITDflu_firstlineused + 1;
			}
			else if (posy + maxy > LINES)
			{
				posy = LINES - 1 - maxy + 1;
			}

			/*
			**  Go to zero based positioning before return.
			*/
			frm->frposx = posx - 1;
			frm->frposy = posy - 1;
			return(retval);
		    }
		    else
		    {
			/*
			**  First try putting it to the right.
			**  The popup is centered with
			**  respect to current item in
			**  parent form.
			*/
			if (maxy >= imaxy)
			{
				posy = iposy - ((maxy - imaxy)/2 + 1);
			}
			else
			{
				posy = iposy + (imaxy - maxy)/2 + 1;
			}

			/*
			**  Check boundaries above and below.
			*/
			if (posy <= IITDflu_firstlineused)
			{
				posy = IITDflu_firstlineused + 1;
			}
			else if (posy + maxy > LINES)
			{
				posy = LINES - 1 - maxy + 1;
			}

			posx = iposx + imaxx;
			if ((posx + maxx) <= COLS)
			{
				/*
				**  Go to zero based positioning before return.
				*/
				frm->frposx = posx - 1;
				frm->frposy = posy - 1;
				return(retval);
			}

			/*
			**  Now try popup below current item in
			**  previous form.
			*/
			posy = iposy + imaxy;

			/*
			**  If beginning of field is off screen
			**  right now, bring popup into view by
			**  placing it at left edge of terminal
			**  screen.
			*/
			if ((posx = iposx) < 1)
			{
				posx = 1;
			}

			if (IIFTcfCheckFit(posx, posy, maxx, maxy))
			{

				/*
				**  Go to zero based positioning before return.
				*/
				frm->frposx = posx - 1;
				frm->frposy = posy - 1;
				return(retval);
			}
			else
			{
				/*
				**  Check to see if we fit vertically.
				**  If so, we just slide form horizontally
				**  to fit at right edge of terminal
				**  screen.
				*/
				if (posy + maxy <= LINES)
				{
					posx = COLS - maxx + 1;
					/*
					**  Go to zero based positioning
					**  before return.
					*/
					frm->frposx = posx - 1;
					frm->frposy = posy - 1;
					return(retval);
				}
			}

			/*
			**  Try putting popup above.
			*/
			if ((posy = iposy - maxy) > IITDflu_firstlineused)
			{
				if ((posx = iposx) < 1)
				{
					posx = 1;
				}
				if (IIFTcfCheckFit(posx, posy, maxx, maxy))
				{
					/*
					**  Go to zero based positioning
					**  before return.
					*/
					frm->frposx = posx - 1;
					frm->frposy = posy - 1;
					return(retval);
				}
				else
				{
					/*
					**  We must fit above, otherwise
					**  we would not be here.  So
					**  adjust horizontally.
					*/
					posx = COLS - maxx + 1;

					frm->frposx = posx - 1;
					frm->frposy = posy - 1;
					return(retval);
				}
			}

			/*
			**  Will put to right even if it overlaps.
			**  Popup is centered vertically with respect
			**  to current item in parent form.
			*/
			posx = COLS - maxx + 1;

			if (maxy >= imaxy)
			{
				posy = iposy - ((maxy - imaxy)/2 + 1);
			}
			else
			{
				posy = iposy + (imaxy - maxy)/2 + 1;
			}

			if (posy <= IITDflu_firstlineused)
			{
				posy = IITDflu_firstlineused + 1;
			}
			else if (posy + maxy > LINES)
			{
				posy = LINES - 1 - maxy + 1;
			}

			/*
			**  Go to zero based positioning before return.
			*/
			frm->frposx = posx - 1;
			frm->frposy = posy - 1;
			return(retval);
		    }
		}
	}
	else
	{
		/*
		**  Only need to adjust one of the coordinates.
		**  The end position is ONE based.
		*/
		endx = posx + maxx;
		endy = posy + maxy;

		if (posx == 0)
		{
			/*
			**  If no previous form, put
			**  at left edge of terminal screen.
			*/
			if (prev == NULL)
			{
				posx = 1;
			}
			else
			{
				/*
				**  Get floating start for x coordinate.
				**  If it does not fit under current
				**  item in parent form, right justify.
				*/
				if ((posx = iposx) < 1)
				{
					/*
					**  If start position of current
					**  item in parent form
					**  is off screen, just put
					**  at left edge of screen.
					*/
					posx = 1;
				}
				if ((posx + maxx) > COLS + 1)
				{
					posx = COLS - maxx + 1;
				}
			}
		}
		else
		{
			/*
			**  Not floating, but check x coordinate for fit.
			*/
			if (endx > COLS + 1)
			{
				frm->frposx = COLS - maxx + 1;
				retval = PFRMMOVE;
			}
		}

		if (posy == 0)
		{
			/*
			**  If no preivous form, put
			**  at top of terminal screen.
			*/
			if (prev == NULL)
			{
				posy = IITDflu_firstlineused + 1;
			}
			else
			{
				/*
				**  Get floating start for y coordinate.
				**  Try to center it with respect to
				**  current item in parent form.  If
				**  part of popup will be offscreen.
				**  bring it back into view.
				*/
				if (maxy >= imaxy)
				{
					posy = iposy - ((maxy - imaxy)/2 + 1);
				}
				else
				{
					posy = iposy + (imaxy - maxy)/2 + 1;
				}

				/*
				**  Check boundaries above and below.
				*/
				if (posy <= IITDflu_firstlineused)
				{
					posy = IITDflu_firstlineused + 1;
				}
				else if (posy + maxy > LINES)
				{
					posy = LINES - 1 - maxy + 1;
				}
			}
		}
		else
		{
			/*
			**  Not floating, but check y coordinate for fit.
			*/
			if (endy > LINES - 1 + 1)
			{
				frm->frposy = LINES - 1 - maxy + 1;
				retval = PFRMMOVE;
			}
		}

		/*
		**  Go to zero based positioning before return.
		*/
		frm->frposx = posx - 1;
		frm->frposy = posy - 1;
	}

	return(retval);
}


/*{
** Name:	FTsyncdisp - Synchronize form into a display window.
**
** Description:
**	Synchronize the image of a form into a display window.  Code must
**	take care of which window to use depending on the style of the
**	form.  Additionally, if the style is a popup check to see if
**	it will fit at desired location, if it is too big for a popup
**	or if the location was changed to fit on the terminal screen.
**
** Inputs:
**	frm	Form to synchronize display window with.
**	first	Indicates if we are synchronizing for starting a display loop.
**
** Outputs:
**
**	Returns:
**		OK	If synchronization completed.
**		FAIL	If synchronization failed.
**	Exceptions:
**		None.
**
** Side Effects:
**	Display list may be updated.
**
** History:
**	xx/xx/83 (dkh) - Written for FT split.
**	04/18/88 (dkh) - Updated for Venus.
*/
STATUS
FTsyncdisp(frm, first)
FRAME	*frm;
bool	first;
{
	WINDOW	*usewin;
	WINDOW	*win;
	WINDOW	*swin;
	FTDLIST	*curdisp;
	FTDLIST	*disp;
	i4	begy;
	i4	begx;
	i4	maxy;
	i4	maxx;
	i4	omaxy;
	i4	omaxx;
	i4	offset = 0;
	i4	formfits = 0;

	poplastline = 0;
	if (!first || frm == NULL)
	{
		/*
		**  Pop off display structs till we find the one
		**  that matches the frame that is passed in.
		*/
		while (dlcur != NULL)
		{
			if (dlcur->frm == frm)
			{
				break;
			}

			/*
			**  If we are going to pop off a popup form,
			**  figure out the last screen line the popup
			**  occupies.  This will be used later to
			**  handle popups that extend beyond the bottom
			**  of a short full screen form.
			*/

			if (dlcur->flags & FTISPOPUP)
			{
				win = dlcur->win;
				/*
				**  Code below has been optimized to not
				**  do a +1 (to go to one based indexing)
				**  and a -1 (one of the lines occupies
				**  the starting line).
				*/
				if (win->_parent != NULL)
				{
					/*
					**  Calculate based on parent
					**  size.
					*/
					win = win->_parent;
					omaxy = win->_begy + win->_maxy;
				}
				else
				{
					omaxy = win->_starty + win->_maxy;
				}
				if (omaxy > poplastline)
				{
					poplastline = omaxy;
				}
			}

			curdisp = dlcur->prev;
			IIFTpdlPopDispList(dlcur);
			dlcur = curdisp;
			if (dlcur)
			{
				dlcur->next = NULL;
			}
		}

		/*
		**  We've popped everything, if passed in frm is not
		**  NULL something must be wrong.
		*/
		if (dlcur == NULL && frm != NULL)
		{
			IIUGerr(E_FT003C_NODISP, UG_ERR_FATAL, 0);
		}
	}

	if (frm == NULL)
	{
		IIFTspfSetPrevFrm(NULL);
		return(OK);
	}

	/*
	**  For the time being, we will rebuild a popup as well.
	*/

	/*
	**  If form is to be a popup, check to see if it is
	**  small enough.  If not, force it to be a full screen
	**  one.
	*/
	if (frm->frmflags & fdISPOPUP)
	{
		/*
		**  If form will NOT fit, turn off popup flag so
		**  form will be displayed full screen.
		**
		**  NB: This leaves frm->frposx and frm->frpoxy alone
		**  and assumes that runtime!iiactfrm.c will take
		**  care of saving the values and setting them to
		**  zero temporarily.
		*/
		if ((formfits = IIFTfpfFitPopupForm(frm)) == PFRMSIZE)
		{
			frm->frmflags &= ~fdISPOPUP;
		}
	}

	/*
	**  Now determine if form to be displayed
	**  is a full screen one or not.
	*/
	if (frm->frmflags & fdISPOPUP)
	{
		/*
		**  We have a popup form.
		**  For now, we assume that we just have
		**  a fixed position popup.  Once
		**  this works, then we will deal with
		**  floating popups.
		*/

		if (first)
		{
			/*
			**  First check to see if popup will work.
			**  If not, return an error.  If it will
			**  fit, but needs to be moved, return a
			**  warning.  We only check the first time
			**  a popup is built since popups don't
			**  move once they are displayed, at least
			**  not yet.  CHECK DONE ABOVE.
			if ((formfits = IIFTfpfFitPopupForm(frm)) == PFRMSIZE)
			{
				return(formfits);
			}
			*/

			/*
			**  Find starting position for popup style
			**  and check to see if it will fit.
			**  If not, try moving it.  Otherwise,
			**  it is an error.
			*/
			begy = frm->frposy;
			begx = frm->frposx;

			/*
			**  Get new display structure and link it up.
			*/
			curdisp = IIFTdlaDispListAlloc();
			if (dlcur)
			{
				dlcur->next = curdisp;
			}
			curdisp->prev = dlcur;
			curdisp->next = NULL;
			dlcur = curdisp;
			curdisp->frm = frm;
			curdisp->flags |= FTISPOPUP;

			if (IITDccsCanChgSize)
			{
				if (IITDiswide)
				{
					curdisp->flags |= FTWIDDISP;
				}
				else
				{
					curdisp->flags |= FTNARDISP;
				}
			}

			if (begy && begx)
			{
				/*
				**  Have start coordinates already,
				**  just decrement to go to
				**  zero index system.
				*/
				begy--;
				begx--;
			}
			else
			{
				/*
				**  Need to find start coordinates
				**  for x and/or y.  Values that
				**  are set are already ZERO based.
				*/
				formfits = IIFTsfpSetFloatPos(frm);
				begy = frm->frposy;
				begx = frm->frposx;
			}

			/*
			**  Get new window structure based on size of
			**  form and set location.
			*/
			omaxy = maxy = frm->frmaxy;
			omaxx = maxx = frm->frmaxx;
			if (frm->frmflags & fdBOXFR)
			{
				maxy += 2;
				maxx += 2;
				offset = 1;
			}
			if ((win = TDnewwin(maxy, maxx, begy, begx)) == NULL)
			{
				IIUGerr(E_FT003D_NOWIN, UG_ERR_FATAL, 0);
			}
			if (offset)
			{
				TDbox(win, '|', '-', '+');

				/*
				**  Fix up box graphic characters for
				**  the borders of a popup in case
				**  the popup never gets a chance to
				**  be displayed to the terminal.  This
				**  can happen if another "display"
				**  statement is run from the "initialize"
				**  block of a popup "display" loop.
				*/
				TDboxfix(win);

				if ((swin = TDsubwin(win, omaxy, omaxx,
					begy + offset, begx + offset,
					(WINDOW *) NULL)) == NULL)
				{
					IIUGerr(E_FT003E_NOSWIN,UG_ERR_FATAL,0);
				}
				/*
				**  Trick system to use subwindow
				**  as a normal window.
				*/
				swin->_starty = begy + 1;
				swin->_startx = begx + 1;
				swin->_begy = 0;
				swin->_begx = 0;
				swin->_flags &= ~_SUBWIN;

				dlcur->win = swin;
			}
			else
			{
				win->_starty = begy;
				win->_startx = begx;
				win->_begy = 0;
				win->_begx = 0;

				dlcur->win = win;
			}
		}
		usewin = dlcur->win;

		/*
		**  Reset begin position of curscr in case
		**  previous form was a full screen form
		**  that has scrolled.
		*/
		curscr->_cury += curscr->_begy;
		curscr->_curx += curscr->_begx;
		curscr->_begy = curscr->_begx = 0;
	}
	else
	{
		/*
		**  We now have a full screen form.
		**  Set things up so we can use FTfullwin.
		*/

		if (first)
		{
			/*
			**  Get new structure and link it up.
			*/
			curdisp = IIFTdlaDispListAlloc();
			if (dlcur)
			{
				dlcur->next = curdisp;
			}
			curdisp->prev = dlcur;
			curdisp->next = NULL;
			dlcur = curdisp;
			curdisp->frm = frm;
			curdisp->flags &= ~FTISPOPUP;
			curdisp->win = FTfullwin;

			if (IITDccsCanChgSize)
			{
				if (frm->frmflags & fdWSCRWID)
				{
					curdisp->flags |= FTWIDDISP;
				}
				else if (frm->frmflags & fdNSCRWID)
				{
					curdisp->flags |= FTNARDISP;
				}
				else
				{
					if (IITDiswide)
					{
						curdisp->flags |= FTWIDDISP;
					}
					else
					{
						curdisp->flags |= FTNARDISP;
					}
				}
			}

			/*
			**  Reset begin position of curscr in case
			**  previous form was a full screen form
			**  that has scrolled.
			*/
			curscr->_begy = curscr->_begx = 0;
		}
		else
		{
			/*
			**  Put back begy and begx values for
			**  curscr in case we are coming back
			**  from a popup to a full screen
			**  that was scrolled.
			curscr->_begx = dlcur->cbegx;
			curscr->_begy = dlcur->cbegy;
			curscr->_cury -= dlcur->cbegy;
			curscr->_curx -= dlcur->cbegx;
			*/
		}
		usewin = FTfullwin;
	}

	FTwin = usewin;

	FTsetiofrm(frm);

	/*
	**  Find most recent full screen form or opaque display.
	*/
	curdisp = IIFTfsfFullScreenForm();

	(VOID) FTbuild(frm, first, usewin, (bool)FALSE);
	if (!samefrm && !first)
	{
		frm->frmflags |= fdDISPRBLD;
		rebuild_disp = TRUE;
	}

	if (!(dlcur->flags & FTISPOPUP) && FTwin != FTfullwin)
	{
		/*
		**  Full screen style form has changed size.
		*/
		FTwin = FTfullwin;
		for (disp = dlcur; disp != NULL; disp = disp->prev)
		{
			if (!(disp->flags & FTISPOPUP))
			{
				disp->win = FTwin;
			}
		}
	}

	FTfrm = frm;
	return(formfits);
}



/*{
** Name:	IIFTpbPrscrBld - Build display for printscreen.
**
** Description:
**	This routine will build the necessary information for
**	the printscreen.  If the current display is a full
**	screen form, then just return pointer to FTwin which
**	is pointing to FTfullwin.  Otherwise, build up
**	set of popups and overlay into FTfullwin.  If the
**	most recent full screen form is invisible, then just
**	return pointer to ovwin.
**
** Inputs:
**	None.
**
** Outputs:
**
**	Returns:
**		win	Window for use by printscreen.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	07/14/88 (dkh) - Initial version.
**      11/30/94 (harpa06) - Integrated bug fix #64984 in 6.4 codeline by 
**                           cwaldman: ABF popup frames appear overlayed in 
**                           screendump. Reset twin each time this routine
**                           is called (instead of first time only). Thus,
**                           twin contains top popup only and overlay is
**                           avoided.
**               
*/
WINDOW *
IIFTpbPrscrBld()
{
	FTDLIST	*disp;
	i4	ty;
static	WINDOW	*twin = NULL;
	WINDOW	*pwin;
	FILE    *file;

	/*
	**  If no forms are presently displayed, return
	**  pointer to curscr so printscreen will print
	**  out what is displayed on the terminal.
	*/
	if (dlcur == NULL)
	{
		return(curscr);
	}

	prscrmaxy = 0;
	if (dlcur->flags & FTISPOPUP)
	{
		/*
		**  Grab copy of popup screen before it is destroyed
		**  by call to IIFTrdRebuildDisplay().
		*/
		if ((pwin = FTwin->_parent) == NULL)
		{
			pwin = FTwin;
		}

		twin = TDnewwin(pwin->_maxy, pwin->_maxx, 0, 0);


/* Integrated bug fix # 64984 by cwaldman (harpa06) */
		/*
		**  If can't allocate new window, just return the standard
		**  window.
		*/
		if (twin==NULL) {
			prscrmaxy = 0;
			return(FTwin);
		}

		TDoverlay(pwin, 0, 0, twin, 0, 0);

		if (IIFTrdRebuildDisplay() == OK)
		{
			i4	boffset = 0;

			/*
			**  If the popup has borders, then
			**  need to change offsets to include
			**  the borders in the result.
			*/
			if (dlcur->frm->frmflags & fdBOXFR)
			{
				boffset = 1;
			}

			/*
			**  Now overlay the window for the current
			**  popup into the result window.  We need
			**  to do this in case the user had typed
			**  something into a field but had not
			**  performed some action (like tab) to cause
			**  the input to be translated into internal
			**  DBVs.
			*/
			TDoverlay(twin, 0, 0, ovwin,
				dlcur->win->_starty - ovwin->_starty - boffset,
				dlcur->win->_startx - boffset);

			disp = IIFTfsfFullScreenForm();
			/*
			**  Need to account for vifred layout
			**  situation later.
			*/
			if (disp == NULL || disp->flags & FTISINVIS)
			{
				prscrmaxy = 0;
				return(ovwin);
			}
			if (prscrmaxy > FTfullwin->_maxy)
			{
				ty = FTfullwin->_maxy;
				FTfullwin->_maxy = prscrmaxy;
				prscrmaxy = ty;
			}
			else
			{
				prscrmaxy = 0;
			}
			TDoverlay(ovwin, 0, 0, FTfullwin,
				-disp->cbegy, -disp->cbegx);
			return(FTfullwin);
		}
		else
		{
			prscrmaxy = 0;
			return(FTwin);
		}
	}
	else
	{
		/*
		**  Current display is a full screen form, just
		**  return FTwin which points to FTfullwin.
		*/
		return(FTwin);
	}
}



/*{
** Name:	IIFTpcPrscrCleanup - Cleanup after a call to IIFTpbPrscrBld()
**
** Description:
**	This routine will reset (if necessary) the _maxy for
**	FTfullwin.  The value to reset to is in prscrmaxy.
**	Nothing is done if prscrmaxy is 0.  This cleanup is
**	necessary in case a printscreen is called with a short
**	full screen form and a popup that extends below the
**	short full screen form.  IIFTpbPrscrBld() will adjust
**	the _maxy for FTfullwin downward so the popup is
**	printed correctly for "printscreen".  This routine
**	will reset _maxy (for FTfullwin) to the original
**	value once printing is done.
**
**	Note that simply extending FTfullwin is OK since FTfullwin
**	is always as big as the terminal size and popups are never
**	bigger than the terminal size.
**
** Inputs:
**	None.
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
**	02/01/89 (dkh) - Initial version.
*/
VOID
IIFTpcPrscrCleanup()
{
	if (prscrmaxy)
	{
		FTfullwin->_maxy = prscrmaxy;
	}
}



/*{
** Name:	IIFTdcDispChk - Check if we need to update the display.
**
** Description:
**	This routine checks to see if it is necessary to rebuild the
**	screen image and display it.  This is now necessary since
**	screen updates are no longer done at the end of a display
**	loop.
**
** Inputs:
**	None.
**
** Outputs:
**
**	Returns:
**		None.
**	Exceptions:
**		None.
**
** Side Effects:
**	Screen image may be updated.
**
** History:
**	08/27/89 (dkh) - Initial vesrion.
*/
VOID
IIFTdcDispChk()
{
	FRAME	*frm;

	if (dlcur == NULL)
	{
		return;
	}

	frm = dlcur->frm;

	if (frm->frmflags & fdDISPRBLD)
	{
		/*
		**  Restore curscr values if we are displaying
		**  a fullscreen form.
		*/
		if (dlcur->frm->frmflags & fdISPOPUP)
		{
			curscr->_begx = dlcur->cbegx;
			curscr->_begy = dlcur->cbegy;
			curscr->_cury -= dlcur->cbegy;
			curscr->_curx -= dlcur->cbegx;
		}

		IIFTsccSetCurCoord();
		if (IIFTrdRebuildDisplay() == OK)
		{
			TDtouchwin(ovwin);
			TDrefresh(ovwin);
		}
		rebuild_disp = FALSE;
		frm->frmflags &= ~fdDISPRBLD;
	}
}


/*{
** Name:	IIFTcsChgSize - Change the terminal screen size.
**
** Description:
**	This routine simply causes the terminal screen size to change
**	if necessary/possible.
**
** Inputs:
**	None.
**
** Outputs:
**
**	Returns:
**		None.
**	Exceptions:
**		None.
**
** Side Effects:
**	May cause the physical terminal screen size to change.
**
** History:
**	08/24/89 (dkh) - Initial version.
*/
VOID
IIFTcsChgSize()
{
	i4	iswide;

	if (IITDccsCanChgSize && dlcur)
	{
		iswide = (dlcur->flags & FTWIDDISP) ? 1 : 0;
		if (IITDscSizeChg(iswide))
		{
			IIFTmscMenuSizeChg(iswide);
		}
	}
}


/*{
** Name:	IIFTsvlSetVifredLayout - Set call back routine for Vifred.
**
** Description:
**	This simply saves away a call back routine pointer for displaying
**	the vifred layout form.  This makes the vifred layout and popup
**	form look and behave much more like the forms system.
**
** Inputs:
**	func	Call back routine.
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
**	08/24/89 (dkh) - Initial version.
*/
VOID
IIFTsvlSetVifredLayout(func)
WINDOW	*(*func)();
{
	IIFTdvl = func;
}


/*{
** Name:	IIFTgscGetScrSize - Get terminal screen size.
**
** Description:
**	This routine gets the logical size of the terminal screen.
**	The physical terminal may not be the returned size at
**	the moment, but it would be that size if the current
**	form in the display list was displayed.
**
** Inputs:
**	None.
**
** Outputs:
**	lcols		Number of logical columns of the display.
**	llines		Number of logical lines of the display.
**
**	Returns:
**		None.
**	Exceptions:
**		None.
**
** Side Effects:
**
** History:
**	08/24/89 (dkh) - Initial version.
*/
VOID
IIFTgscGetScrSize(lcols, llines)
i4	*lcols;
i4	*llines;
{
	FTDLIST	*curdisp;

	/*
	**  Since the number of lines for the terminal does not
	**  currently change dynamically, just set it to LINES for now.
	*/
	*llines = LINES;

	/*
	**  Now, find the number of columns, the terminal is currently
	**  set to.  Just return COLS if there are no forms active,
	**  or the termcap description indicates that the terminal
	**  can not change size dynamically.
	*/
	if (!IITDccsCanChgSize || dlcur == NULL)
	{
		*lcols = COLS;
	}
	else
	{
		/*
		**  We now need to find how wide the terminal screen
		**  is.  This is based on the most recent full screen
		**  form that is displayed.  If only popups are displayed,
		**  then, COLS is used instead.
		**
		**  We first need to find the most recent full screen
		**  form or opaque display, if one exists.
		*/
		if ((curdisp = IIFTfsfFullScreenForm()) == NULL)
		{
			*lcols = COLS;
		}
		else
		{
			if (curdisp->flags & FTNARDISP)
			{
				/*
				**  Set to narrow size since form
				**  is displayed with anrrow attribute.
				*/
				*lcols = YN;
			}
			else if (curdisp->flags & FTWIDDISP)
			{
				/*
				**  Set to "wide" size since
				**  form is displayed with wide attribute.
				*/
				*lcols = YO;
			}
			else
			{
				/*
				**  Form must be displayed in
				**  current size, just use COLS.
				*/
				*lcols = COLS;
			}
		}
	}
}
