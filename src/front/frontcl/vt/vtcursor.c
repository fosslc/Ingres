/*
**  VTcursor.c
**
**  Copyright (c) 2004 Ingres Corporation
**
**  History:
**	12/03/84 (dkh) - Initial version.
**	12/31/86 (dkh) - Added support for new activations.
**	13-jan-87 (bruceb)	fix for bug 11279
**		When determining if anything exists on the screen onto
**		which the cursor can tab (or ^P), check for non-sequence
**		fields in addition to trim and normal fields.
**	10-sep-87 (bruceb)
**		Changed from use of ERR to FAIL (for DG.)
**	24-sep-87 (bruceb)
**		Added call on FTshell for the shell function key.
**	11/11/87 (dkh) - Code cleanup.
**	05/25/88 (tom) - added arguments ret_tab_exit and simple_move
**			to support box and form move and resize
**	02/01/89 (brett) - added windex command string calls.
**	09/22/89 (dkh) - Porting changes integration.
**	12/05/89 (dkh) - Changed interface to VTcusor() so VTLIB could
**			 be moved into ii_framelib shared lib.
**	12/30/89 (dkh) - Put in better support for popups on top of layout.
**	01-feb-90 (bruceb)
**		Moved TEflush's into FTbell.
**	01-feb-90 (bruceb)
**		Moved TEflush's into FTbell.
**	16-mar-90 (bruceb)
**		Added locator support.  FKmapkey() is now called with a
**		function pointer used to determine legal locator regions on
**		the screen and the effect of clicking on those regions.
**	05-apr-90 (bruceb)
**		In VTgetScrCurs(), adjust VTglobx if > endxFrm.  Done so that
**		user requesting location on VIFRED screen will get accurate
**		data.
**	09-may-90 (bruceb)
**		Added 'no_menu' arg for locator support.
**	08/15/90 (dkh) - Fixed bug 21670.
**	19-dec-91 (leighb) DeskTop Porting Change:
**		Changes to allow menu line to be at either top or bottom.
**	26-may-92 (rogerf) DeskTop Porting Change:
**		Added scrollbar processing inside SCROLLBARS ifdef.
**	05-jun-92 (leighb) DeskTop Porting Change:
**		Changed COORD to IICOORD to avoid conflict
**		with a structure in wincon.h for Windows/NT
**      24-sep-96 (mcgem01)
**              Global data moved to vtdata.c
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	26-Aug-2009 (kschendel) 121804
**	    Need termdr.h to satisfy gcc 4.3.
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<kst.h>
# include	"vtframe.h"
# include	<tdkeys.h>
# include	<frsctblk.h>
# include	<te.h>
# include	<st.h>
# include	<er.h>
# include	<ft.h>
# include	<menu.h>
# ifdef SCROLLBARS
# include	<wn.h>
# endif
# include	<termdr.h>

GLOBALREF	i4	VTgloby;
GLOBALREF	i4	VTglobx;
GLOBALREF	i4	IIVTcbegy;
GLOBALREF	i4	IIVTcbegx;
GLOBALREF   	i4     mu_where;       /* Either MU_TOP or MU_BOTTOM       */
GLOBALREF	i4	IITDflu_firstlineused;
static	WINDOW	*VTwin = NULL;
static	i4	VTendFrame = 0;
static	i4	VTendxFrame = 0;
static	bool	VTRBF = FALSE;
static	bool	click_return = FALSE;

FUNC_EXTERN	KEYSTRUCT	*FTgetch();
FUNC_EXTERN	VOID		IITDpciPrtCoordInfo();
FUNC_EXTERN	VOID		IITDposPrtObjectSelected();
FUNC_EXTERN	i4		IITDlioLocInObj();
FUNC_EXTERN	VOID		IIFTmldMenuLocDecode();
FUNC_EXTERN	VOID		IIFTsmShiftMenu();

VOID	IIVTfldFrmLocDecode();
VOID	IIVTcgCursorGoto();

/*
**  move the cursor by calling the passed move command
**  Please note that no validation is done here since
**  there are no fields to validate in the editing phase
**  of VIFRED.
**
**  NOTE: vffrmatr.c code relies on the fact that this function
**	 has a very low reliance on the frame ptr which is passed in.
**
*/

i4
VTcursor(frm, globy, globx, lastline, lastcol, poscount, posarray, evcb,
	find_func, rbf, expand, ret_tab_exit, simple_move, no_menu)
FRAME	*frm;
i4	*globy;
i4	*globx;
i4	lastline;
i4	lastcol;
i4	poscount;
FLD_POS	*posarray;
FRS_EVCB *evcb;
i4	(*find_func)();
bool	rbf;
bool	expand;
bool	ret_tab_exit;	/* does simple return or tab key cause exit?? */
bool	simple_move;	/* is it a simple movement request */
bool	no_menu;	/* is there a menu associated with this call? */
{
	reg	WINDOW	*win;
	i4		code;
	i4		retcode;
	i4		flg;
	KEYSTRUCT	*ks;
	i4		event;
	u_char		dummy;
	char		msgline[MAX_TERM_SIZE + 1];
	bool		activate = FALSE;
	i4		border = 0;
	IICOORD		obj;

        /*
	** brett, Enable vifred mode windex command string.
	** Put windex emulator into the special vifred editing mode.
	*/
        IITDenvifred();

	VTwin = win = frm->frscr;
	VTgloby = *globy;
	VTglobx = *globx;
	VTendFrame = lastline;
	VTendxFrame = lastcol;
	VTRBF = rbf;

	/*
	** Obj is used only for tracing info.  Doesn't account for _start[yx]
	** or for form scrolling.  Just indicates form size.
	*/
	obj.begy = obj.begx = 0;
	obj.endy = VTendFrame;
	obj.endx = VTendxFrame;
	IITDpciPrtCoordInfo(ERx("VTcursor"), ERx("VTwin"), &obj);

	click_return = no_menu;

	TDmove(win, VTgloby, VTglobx);
	TDtouchwin(win);
	TDrefresh(win);

	for (;;)
	{
		ks = FTgetch();

		/* special check for return or tab key during
		   no-menu operation see enter.c vfedbox.c in vifred  */
		if (ret_tab_exit && ks->ks_fk == 0)
		{
			if (ks->ks_ch[0] == '\r')
			{
				retcode	= evcb->event = fdopMENU;
				break;
			}
			if (ks->ks_ch[0] == '\t')
			{
				retcode = evcb->event = fdopNEXT;
				break;
			}
		}
		if (FKmapkey(ks, IIVTfldFrmLocDecode, &code, &flg, &activate,
			&dummy, evcb) != OK)
		{
			FTbell();
			continue;
		}
		event = evcb->event;
		if (event == fdopMENU || event == fdopMUITEM ||
			event == fdopFRSK)
		{
			retcode = event;
			break;
		}
		else if (code == fdopMENU)	/* Mouse click on '<' or '>'. */
		{
			IIFTsmShiftMenu(event);
			evcb->event = retcode = fdopMENU;
			break;
		}

		if (!(flg & fdcmBRWS))
			code = fdopERR;

		retcode = fdNOCODE;

		/*
		** This first switch handles simple movement such as
		** is required for vifred movement and resize of a form.
		*/

		switch (code)
		{
		case fdopUP:
			vfcuUp();
			goto return_pt;

		case fdopDOWN:
			vfcuDown();
			goto return_pt;

		case fdopLEFT:
			vfcuLeft();
			goto return_pt;

		case fdopRIGHT:
			if (vfcuRight(lastcol, expand) == fdopSCRLT)
			{
				retcode = fdopSCRLT;
			}
			goto return_pt;

		case fdopGOTO:
			IIVTcgCursorGoto(evcb);
			goto return_pt;

		case fdopREFR:
			TDrefresh(curscr);
			TDtouchwin(win);
			goto return_pt;

		}

		if (simple_move == FALSE)
		{
			/*
			** Since it is not a simple movement request
			** this switch handles the other possible
			** keystroke commands.
			*/
			switch (code)
			{
			case fdopNEXT:
				/*
				** check to make sure something is on screen
				*/
				if (frm->frtrimno + frm->frfldno
					+ frm->frnsno <= 0)
				{
					break;
				}
				(*find_func)(VTgloby, VTglobx, VF_NEXT,
					&VTgloby, &VTglobx);
				break;

			case fdopPREV:
				/*
				** check to make sure something is on screen
				*/
				if (frm->frtrimno + frm->frfldno
					+ frm->frnsno <= 0)
				{
					break;
				}
				(*find_func)(VTgloby, VTglobx, VF_PREV,
					&VTgloby, &VTglobx);
				break;

			case fdopBEGF:
				(*find_func)(VTgloby, VTglobx, VF_BEGFLD,
					&VTgloby, &VTglobx);
				break;

			case fdopENDF:
				(*find_func)(VTgloby, VTglobx, VF_ENDFLD,
					&VTgloby, &VTglobx);
				break;

			case fdopSCRUP:
				TDupvfsl(win, lastline);
				VTgloby = win->_cury;
				break;

			case fdopSCRDN:
				TDdnvfsl(win);
				VTgloby = win->_cury;
				break;

			case fdopSCRLT:
				if (!TDltvfsl(win, lastcol, expand))
				{
					/*
					**  Fix for BUG 8406. (dkh)
					*/
					win->_curx = lastcol;
					VTglobx = win->_curx;
				}
				else
				{
					VTglobx = win->_curx;
				}
				break;

			case fdopSCRRT:
				TDrtvfsl(win);
				VTglobx = win->_curx;
				break;

			case fdopPRSCR:
			{
				i4	omaxy;
				i4	omaxx;

				TDsaveLast(msgline);
				omaxy = win->_maxy;
				omaxx = win->_maxx;
				win->_maxy = lastline + 1;
				win->_maxx = lastcol + 1;
				FTprvfwin(win);
				FTprnscr(NULL);
				win->_maxy = omaxy;
				win->_maxx = omaxx;
				TDwinmsg(msgline);
				break;
			}

			case fdopSHELL:
				FTshell();
				break;

			default:
				FTbell();
				break;

			}
		}
	 return_pt:

#ifdef SCROLLBARS

        	/* note... +1 allows for "End of Form" frame at margin(s) */

	        WNFbarCreate(FALSE, 		/* not on "real" form */
			     win->_cury,        /* current row */
        	             VTendFrame+1,      /* max form rows */
                	     win->_curx,        /* current column */
	                     VTendxFrame+1);    /* max form columns */
#endif /* SCROLLBARS */


		if (retcode == fdopSCRLT)
		{
			VTgloby = win->_cury;
			VTglobx = win->_curx;
			break;
		}
		TDmove(win, VTgloby, VTglobx);
		TDrefresh(win);
	}
	*globy = VTgloby;
	*globx = VTglobx;

        /*
	** brett, Disable vifred mode windex command string.
	** Take windex emulator out of the special vifred editing mode.
	*/
        IITDdsvifred();

	IIVTcbegy = curscr->_begy;
	IIVTcbegx = curscr->_begx;

	return (retcode);
}



/*{
** Name:	VTgetScrCurs	- get the screen relative cursor position
**
** Description:
**	Return the screen relative 0 based cursor position
**
** Inputs:
**	i4	endx		- Edge of form
**	i4	*row, *col;	- row and column to be posted
**
** Outputs:
**
**	Returns:
**		none
**
**	Exceptions:
**
** Side Effects:
**
** History:
**	06/01/88  (tom) - written
*/
VOID
VTgetScrCurs(endx, row, col)
i4	endx;
i4	*row;
i4	*col;
{
	if (VTglobx > endx)
	    VTglobx = endx;
	*row = VTgloby + curscr->_begy;
	*col = VTglobx + curscr->_begx;
}

/*
** move the cursor for the cursor mode
**/

i4
vfcuUp()
{
	TDmvup(VTwin, 1);
	VTgloby = VTwin->_cury;
	VTglobx = VTwin->_curx;
	if (VTRBF && VTgloby == 0)
	{
		TDrefresh(VTwin);
		VTgloby++;
	}
}

i4
vfcuDown()
{
	if (VTgloby < VTendFrame)
	{
		TDmvdown(VTwin, 1);
		VTgloby = VTwin->_cury;
		VTglobx = VTwin->_curx;
	}
}



i4
vfcuLeft()
{
	if (VTglobx == VTwin->_begx)
		return;
	TDmvleft(VTwin, 1);
	VTgloby = VTwin->_cury;
	VTglobx = VTwin->_curx;
}

i4
vfcuRight(lastcol, expand)
i4	lastcol;
bool	expand;
{
	if (VTglobx >= lastcol)
	{
		if (expand)
		{
			VTwin->_curx = lastcol + 1;

			/*
			**  Changed to use fdNOCODE (instead of fdopSCRLT)
			**  so that we
			**  don't automatically expand the width
			**  of the form.  Rest of the code in
			**  VIFRED and here left intact in case
			**  we want to resurrect this stuff. (dkh)
			*/
			return (fdNOCODE);
		}
		else
		{
			return(fdNOCODE);
		}
	}
	TDmvright(VTwin, 1);
	VTgloby = VTwin->_cury;
	VTglobx = VTwin->_curx;

	return(fdNOCODE);
}


/*{
** Name:	IIVTcgCursorGoto	- Move the cursor under the locator.
**
** Description:
**	Move the cursor to the (already validated) position of the locator.
**	The single exception is that RBF doesn't allow the cursor to land on
**	the top line of the report form and so adjusts the cursor down one line.
**
** Inputs:
**	evcb
**	    .gotorow	Row on the screen.
**	    .gotocol	Col on the screen.
**
** Outputs:
**
**	Returns:
**		None.
**
**	Exceptions:
**
** Side Effects:
**	As part of moving the cursor, updates VTwin's _cury, _curx and
**	VTgloby, VTglobx.
**
** History:
**	16-mar-90 (bruceb)	Written.
*/
VOID
IIVTcgCursorGoto(evcb)
FRS_EVCB	*evcb;
{
	char	buf[50];

	/*
	VTgloby = VTwin->_cury = VTwin->_begy + evcb->gotorow
	    - (curscr->_begy + curscr->_starty);
	VTglobx = VTwin->_curx = VTwin->_begx + evcb->gotocol
	    - (curscr->_begx + curscr->_startx);
	*/

	VTgloby = VTwin->_cury = evcb->gotorow
	    - (curscr->_begy + curscr->_starty);
	VTglobx = VTwin->_curx = evcb->gotocol
	    - (curscr->_begx + curscr->_startx);

	if (VTRBF && VTgloby == 0)
	{
	    TDrefresh(VTwin);
	    VTgloby++;
	}

	STprintf(buf, ERx("VTwin's window coords (%d,%d)"), VTgloby, VTglobx);
	IITDposPrtObjSelected(buf);
}


/*{
** Name:	IIVTfldFrmLocDecode	- Determine effect of locator click.
**
** Description:
**	Determine whether or not the user clicked on a legal location.
**	Legal locations are any region on the screen that fall between the
**	curscr->_start[yx] top corner and the VTendframe/VTendxframe bottom
**	corner.
**
** Inputs:
**	key	User input.
**
** Outputs:
**	evcb
**	    .event	fdNOCODE if illegal location; fdopGOTO if legal.
**	    .gotorow	Row on the screen if legal location.
**	    .gotocol	Col on the screen if legal location.
**
**	Returns:
**		None.
**
**	Exceptions:
**
** Side Effects:
**
** History:
**	16-mar-90 (bruceb)	Written.
*/
VOID
IIVTfldFrmLocDecode(key, evcb)
KEYSTRUCT	*key;
FRS_EVCB	*evcb;
{
	i4	row;
	i4	col;
	IICOORD	frm;
	char	buf[50];

	row = key->ks_p1;
	col = key->ks_p2;

	/*
	** If user has clicked on the menu line, call menu decode routine.
	** Exception is that if there is no menu, than this is a case of
	** clicking on the menu line to return.
	*/
	if (row == ((LINES - 1) * (mu_where == MU_BOTTOM)))
	{
	    if (click_return)
	    {
		evcb->event = fdopMENU;
	    }
	    else
	    {
		IIFTmldMenuLocDecode(key, evcb);
	    }
	    return;
	}

	row -= IITDflu_firstlineused;

	frm.endy = frm.begy = curscr->_starty;
	frm.endx = frm.begx = curscr->_startx;
	frm.endy += VTendFrame + curscr->_begy;
	frm.endx += VTendxFrame + curscr->_begx;

	if (!IITDlioLocInObj(&frm, row, col))
	{
	    evcb->event = fdNOCODE;
	}
	else
	{
	    evcb->event = fdopGOTO;
	    evcb->gotorow = row;
	    evcb->gotocol = col;

	    STprintf(buf, ERx("VTwin's screen coords (%d,%d)"), row, col);
	    IITDposPrtObjSelected(buf);
	}
}
