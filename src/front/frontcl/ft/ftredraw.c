/*
**  FTredraw
**
**  Copyright (c) 2004 Ingres Corporation
**
**  ftredraw.c
**
**  History:
**	05/06/88 (dkh) - Updated for Venus.
**	07/23/88 (dkh) - Modified to handle scrolled forms.
**	10/22/88 (dkh) - Performance changes.
**	13-jan-89 (bruceb)
**		If redrawing a single frame, and that frame has the
**		fdREBUILD flag set, than call FTbuild.
**	04/10/89 (dkh) - Reset IIFTscScreenCleared once screen updated.
**	08/04/89 (dkh) - Added support for 80/132 runtime switching.
**	08/15/89 (dkh) - Fixed problem with ABF calling another process
**			 from a popup.  On return from subprocess, all
**			 visible forms are now redisplayed as opposed to
**			 just the popup being redisplayed.
**	08/30/89 (dkh) - Put in some patches to better handle displaying
**			 big forms for the first time.
**	10/02/89 (dkh) - Changed IIFTscSizeChg() to IIFTcsChgSize().
**	12/30/89 (dkh) - Changed screen update scheme to make it look more
**			 atomic.
**	04/21/90 (dkh) - Fixed us #595 by changing positioning scheme.
**	11/14/90 (dkh) - Fixed bug 34449.  Removed call to center
**			 destination since it duplicates code that
**			 is now in TDrefresh().
**	08/17/91 (dkh) - Fixed bug 38952.  Screen updates for large forms
**			 that have scrolled vertically should now draw
**			 with less odd behavior.
**	08/18/91 (dkh) - Fixed bug 38752.  Put in some workarounds to
**			 make things work for the compressed view of
**			 a vision visual query.  The real fix should
**			 be to change vision to create more sociable forms.
**	10-jun-92 (rogerf) - Added FTWNRedraw function and call to it if
**			 SCROLLBARS is defined.
**	01/26/93 (dkh) - Added ifdef around forward declaration of FTWNRedraw()
**			 so it does not become a link problem for VMS.
**      14-mar-95 (peeje01: jmbp) Cross-integration from 6500db_su4_us42
**		Original comment follows:
** **	06-aug-92 (kirke)
** **              For DOUBLEBYTE ports always force a redraw of all fields
** **              so that DB characters underneath the border get redrawn.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**    25-Oct-2005 (hanje04)
**        Add prototype for fix_start() to prevent compiler
**        errors with GCC 4.0 due to conflict with implicit declaration.
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	"ftframe.h"
# ifdef 	SCROLLBARS
# include	<wn.h>
# endif

FUNC_EXTERN	WINDOW	*FTfindwin();
FUNC_EXTERN	VOID	IIFTrRedisp();
FUNC_EXTERN	VOID	IIFTgdiGetDispInfo();
FUNC_EXTERN	VOID	IIFTcsChgSize();
# ifdef SCROLLBARS
FUNC_EXTERN	VOID	FTWNRedraw();
# endif

static fix_start(
		WINDOW	*win);

GLOBALREF	bool	IIFTscScreenCleared;

/*{
** Name:	FTredraw - Redraw passed in form on the terminal screen.
**
** Description:
**	Given a form, redraw it to the terminal screen.  The mode to
**	display the form in determines whether the terminal is updateable
**	or not.  Terminal cursor is left put at field designated by
**	the field number.  If argument "all" is TRUE, then update screen
**	with "all" possibly visible forms, including the passed in one.
**
** Inputs:
**	frm	Pointer to form to display.
**	dispmode	Display mode of form.
**	fldno		Number indicating field where cursor is to be left.
**	all		Boolean to display "all" possibly visible fields.
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
**	Screen display is updated.
**
** History:
**	05/06/88 (dkh) - Updated for Venus.
*/
VOID
FTredraw(frm, dispmode, fldno, all)
FRAME	*frm;
i4	dispmode;
i4	fldno;
i4	all;
{
	FIELD	*fld;
	WINDOW	*win = NULL;
	i4	isfull;
	i4	yoffset;
	i4	xoffset;
	i4	getout = FALSE;
	i4	rebuild = FALSE;
	i4	start_modified = FALSE;

	IIFTcsChgSize();


# ifdef DOUBLEBYTE
	all = TRUE;
# endif

	if (all)
	{
		/*
		**  Display all possibly visible forms.
		*/
		IIFTrRedisp();
		frm->frmflags &= ~(fdDISPRBLD | fdREDRAW | fdREBUILD);
	}
	else
	{
		IIFTgdiGetDispInfo(&isfull, &yoffset, &xoffset);
		if (isfull)
		{
			curscr->_begy = yoffset;
			curscr->_begx = xoffset;
		}

		if (frm->frmflags & fdREBUILD)
		{
			rebuild = TRUE;
		}

		if (!isfull && (frm->frmflags & fdDISPRBLD))
		{
			IIFTrRedisp();
			frm->frmflags &= ~(fdDISPRBLD | fdREDRAW | fdREBUILD);
			getout = TRUE;
		}

		if (fldno != -1)
		{
			fld = frm->frfld[fldno];

			/*
			**  This means that the form must be a fullscreen.
			**  Otherwise, this flag would have been cleared
			**  above.
			**
			**  If we are dealing with a vision compress view
			**  form that is large, alway rebuild and set curscr
			**  offsets to get it to display correctly.
			*/
			if (frm->frmflags & (fdDISPRBLD | fdREBUILD | fdVIS_CMPVW))
			{
				/*
				**  Rebuild the fullscreen form image.
				*/
				FTbuild(frm, (bool)FALSE, FTwin, (bool)FALSE);
				TDboxfix(FTwin);
				frm->frmflags &=
					~(fdDISPRBLD | fdREDRAW | fdREBUILD);
				win = FTfindwin(fld, FTwin, FTutilwin);
				fix_start(win);
				start_modified = TRUE;
			}
			else
			{
				win = FTfindwin(fld, FTwin, FTutilwin);
			}
			TDmove(win, (i4) 0, (i4) 0);

			/*
			**  Only refresh to current field if we have not
			**  modified the start location for curscr.
			*/
			if (!start_modified)
			{
				TDrefresh(win);
			}
		}

		if (getout)
		{
#ifdef SCROLLBARS
			FTWNRedraw(frm, fldno);
#endif
			return;
		}

		/*
		**  If screen has cleared and current form is a
		**  popup, then need to rebuild complete screen
		**  image.  Rebuilding of complete screen image
		**  will cause forms to be completely rebuilt, so
		**  special display sync flags can be reset.
		*/
		if (IIFTscScreenCleared && !isfull)
		{
			IIFTrRedisp();
			frm->frmflags &= ~fdREDRAW;
			IIFTscScreenCleared = FALSE;
#ifdef SCROLLBARS
			FTWNRedraw(frm, fldno);
#endif
			return;
		}

		/*
		**  If there is a parent, then we have a form that
		**  has popup style with borders.  Use parent so
		**  borders will show up as well.
		*/
		win = (FTwin->_parent != NULL) ? FTwin->_parent : FTwin;

		if (IIFTscScreenCleared || frm->frmflags & fdREDRAW)
		{
			TDtouchwin(win);
			frm->frmflags &= ~fdREDRAW;
			IIFTscScreenCleared = FALSE;
		}
		if (rebuild)
		{
			/*
			** FTbuild into FTwin.  However, touch the parent
			** form (if any) so as to handle popups with
			** borders.
			**
			** FTbuild turns off the fdREBUILD flag.
			*/
			if (frm->frmflags & fdREBUILD)
			{
				FTbuild(frm, (bool)FALSE, FTwin, (bool)FALSE);
				TDboxfix(FTwin);
			}
			TDtouchwin(win);
		}
		TDrefresh(win);

		/*
		**  Set scroll coordinates if the frame is a vision
		**  compress view frame.  This allows the compressed
		**  frame to actually stayed scroll if we are viewing
		**  a large visual query with "zoomout".
		**
		**  A much better fix is to fix vision to build forms
		**  similar to the application flow diagram that does
		**  not need to scroll.
		*/
		if (frm->frmflags & fdVIS_CMPVW)
		{
			IIFTsccSetCurCoord();
		}
	}
#ifdef SCROLLBARS
	FTWNRedraw(frm, fldno);
#endif
}


static
fix_start(win)
WINDOW	*win;
{
	i4	first_visible;
	i4	middle;
	i4	available;

	/*
	**  If starting location for window is not visible given the
	**  current values of curscr->_begy, then adjust _begy till
	**  the window is visible in the middle of the screen.  But
	**  not beyond the edges of the form.
	**
	**  We don't adjust for _begx since that doesn't cause
	**  scroll behaviors, the window is simply repainted.
	*/

	first_visible = -curscr->_begy;
	available = LINES - IITDflu_firstlineused - 1;
	middle = available/2;

	/*
	**  If row is above screen, scroll view back up or
	**  scroll data down.
	*/
	if (win->_begy < first_visible)
	{
		/*
		**  Push starting location all the way to the top
		**  if the starting location is normally visible
		**  with no vertical scrolling of form.
		**  Otherwise, just put starting point
		**  at middle of screen.  The -1 in the
		**  if statement is to go to zero based addressing.
		*/
		if (win->_begy < available - 1)
		{
			curscr->_begy = 0;
		}
		else if ((curscr->_begy += middle) > 0)
		{
			/*
			**  This is redundant, but if
			**  we move back to much, just
			**  set to no vertical scrolling.
			*/
			curscr->_begy = 0;
		}
	}
	else if (win->_begy > (first_visible + available - 1))
	{
		/*
		**  If row is below screen, scroll view down or
		**  data up.  Note that the -1 in the above if
		**  statement is needed since we to know the
		**  last visible row.
		*/
		if (win->_begy + middle - 1 > FTwin->_maxy - 1)
		{
			curscr->_begy = -(FTwin->_maxy - available);
		}
		else
		{
			curscr->_begy = -(win->_begy - (available - middle));
		}
	}
}

#ifdef SCROLLBARS

/*{
** Name:	FTWNredraw - Continue FTredraw by adding ScrollBars
**
** Description:
**		Determine if Horizontal and/or vertical Scroll Bars are
**		needed in a GUI environment.
**
** Inputs:
**	frm	Pointer to form to display.
**	fldno	Number indicating field where cursor is to be left.
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
**	Form scrollbar(s) are generated if needed.
**
** History:
**	10-jun-92 (rogerf) Written.
*/
VOID
FTWNRedraw(frm, fldno)
FRAME *frm;
i4    fldno;
{
    FLDHDR *fldHdr;
    FIELD  **frfld;
    i2     i;
    i2     j;
    i2     vis;
    i2	   wid;

    /* note... field must be fully visible on current screen */
    /* COUNT *VISIBLE* FIELDS, IF ONLY 1, DISABLE VERTICAL SCROLLBAR */
    /* CHECK IF ANY FIELD IS WIDER THAN SCREEN, IF NOT DISABLE HORIZ S-BAR */
    for (j = frm->frfldno, frfld = frm->frfld, i = vis = wid = 0;
           i < j;
             i++)
    {
        FIELD *fld;
        fld = (FIELD*) *frfld++;
        if (fld->fltag == FREGULAR)
        {
            if (!(fld->fld_var.flregfld->flhdr.fhdflags & fdINVISIBLE) &&
                !(fld->fld_var.flregfld->flhdr.fhd2flags & fdREADONLY))
                ++vis;
            if (fld->fld_var.flregfld->flhdr.fhposx +
		fld->fld_var.flregfld->flhdr.fhmaxx >= COLS)
		wid = 1;
        }
        else
        {
            if (!(fld->fld_var.fltblfld->tfhdr.fhdflags & (fdINVISIBLE | fdTFINVIS)))
                ++vis;
            if (fld->fld_var.fltblfld->tfhdr.fhposx +
		fld->fld_var.fltblfld->tfhdr.fhmaxx >= COLS)
		wid = 1;
        }
    }

    /* CURRENT FIELD? */
    if (fldno >= 0)
    {
        /* VISIBLE FIELD(S) */
        if (vis > 0)
        {
            i4  posx;
            fldHdr = (FLDHDR*) frm->frfld[fldno]->fld_var.flregfld;

            /* IF TABLE FIELD... ADJUST OFFSET FOR RELATIVE COLUMN */
            posx = fldHdr->fhposx;
            if (frm->frfld[fldno]->fltag == FTABLE)
            {
                    TBLFLD  *tf = frm->frfld[fldno]->fld_var.fltblfld;
                    FLDCOL  **fc = tf->tfflds;
                    i2  i;

                    for (i = 0; i < tf->tfcurcol; i++, ++fc)
                        posx += (*fc)->flhdr.fhmaxx;

            }

            if	    ((vis == 1) && (wid > 0))	/* 1 vis fld + fld > form width */
	    {
                WNFbarCreate(TRUE, 0, 0,        /* suppress vert scroll bar */
                             posx,		/* current field column */
                             frm->frmaxx);	/* max form columns */
		return;
	    }
            else if ((vis > 1) && (wid > 0))	/* many flds + fld > form width */
	    {
                WNFbarCreate(TRUE,
			     fldHdr->fhposy,    /* current field row */
                             frm->frmaxy,       /* max form rows */
                             posx,              /* current field column */
                             frm->frmaxx);      /* max form columns */
		return;
	    }
    	    else if ((vis > 1) && (wid == 0))	/* many flds, all < form width */
	    {
                WNFbarCreate(TRUE,
			     fldHdr->fhposy,    /* current field row */
                             frm->frmaxy,       /* max form rows */
                             0, 0);		/* suppress horiz scroll bar */
		return;
	    }
	}
    }
    WNFbarDestroy();
    return;
}
#endif
