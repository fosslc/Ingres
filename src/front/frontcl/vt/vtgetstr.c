/*
**  VTgetstr.c
**
**  Copyright (c) 2004 Ingres Corporation
**
**  vtgetstr.c
**
**  History:
**	02/04/85 (dkh) - Initial version.
**	12/31/86 (dkh) - Added support for new activations.
**	10/13/86 (KY)  -- Changed CH.h to CM.h.
**	28-apr-87 (bruceb)
**		Added second parameter to calls on TDclrbot and
**		TDaddchar to indicate that they are affecting
**		standard, left-to-right fields.
**	08/14/87 (dkh) - ER changes.
**	10-sep-87 (bruceb)
**		Changed from use of ERR to FAIL (for DG.)
**	24-sep-87 (bruceb)
**		Added calls on FTshell for the shell function key.
**	12/18/87 (dkh) - Fixed jup bug 1611.
**	12/19/87 (dkh) - Fixed problem with screen cursor moving
**			 to menu line when a function key that
**			 is mapped to a menuitem or frskey is pressed.
**	02/05/88 (dkh) - Fixed above problem for VTgetnew().
**	02/25/88 (dkh) - Fixed jup bug 2064.
**	21-jul-88 (bruceb)	Fix for bug 2969.
**		Removed 4096 byte restriction on the width of a form.
**		Now able to edit text to a maximum width of the new
**		parameter 'maxlen'.  Creation of text now uses same
**		code as does editing text, and thus has enhanced
**		editing capabilities.  (Removed VTgetnew)  Collapsed
**		VTgetedit into VTgetstring.
**	09/07/88 (tom) - Fixed a bug due to overwriting of attributes
**	26-apr-89 (bruceb)
**		If field too close to border, ensure at least 5 spaces
**		allocated for the edit window.
**	07/11/89 (dkh) - Fixed bug 6196.
**	01-feb-90 (bruceb)
**		TEflush's moved into FTbell.
**	09-feb-90 (bruceb)	Fix for bug 9948.
**		No longer guaranteed minimum of 5 spaces; if the form is
**		smaller than that (small popup), most that is available is the
**		width of that form.
**	12-feb-90 (bruceb)	Fix for bug 9775.
**		'Guaranteed' minimum editting space is now 20 instead of 5.
**	13-feb-90 (bruceb)	Fix for bug 9954.
**		Take account the possible shift-left for minimum editting
**		space when deciding what of the original window contents
**		are replaced to the form after 'getting' the string.
**		--Also, change from 20 to 15 spaces.  (It looked too much.)
**	16-mar-90 (bruceb)
**		Added support for locator events.  FKmapkey() is now called
**		with a NULL function pointer to indicate that no area of the
**		screen is a legal locator region.
**	23-apr-90 (bruceb)
**		Now able to click on the menu line.  Clicking on the 'mode'
**		information will switch between overtype and insert.  Clicking
**		elsewhere on that line will generate a MENU key.
**	05-jun-92 (leighb) DeskTop Porting Change:
**		Changed COORD to IICOORD to avoid conflict
**		with a structure in wincon.h for Windows/NT
**	08/20/93 (dkh) - Fixing NT compile problems.  Changed _leave
**			 to lvcursor for the WINDOW structure.
**	08/31/93 (dkh) - Fixed up compiler complaints due to prototyping.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	28-Mar-2005 (lakvi01)
**	    Corrected function prototypes.
*/

# include	<compat.h>
# include	<st.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<kst.h>
# include	"vtframe.h"
# include	<tdkeys.h>
# include	<frsctblk.h>
# include	<cm.h>
# include	<te.h>
# include	<me.h>
# include	<ctrl.h>
# include	<er.h>
# include	"ervt.h"

/*
**  If you change the value of VFBUFSIZ here, YOU must also change it
**  in the VIFRED and RFVIFRED directories. (dkh)
**  This size also reflects a column width in ii_trim.
*/
# define	VFBUFSIZ	150

/*
**  'Guaranteed' minimum text editting window size.  May be smaller if
**  window is narrow, or maximum edit size is specified as smaller.
*/
# define	MIN_EDTSZE	15


GLOBALREF	WINDOW		*frmodewin;
FUNC_EXTERN	KEYSTRUCT	*FTget_char();
FUNC_EXTERN	WINDOW		*TDtmpwin();
FUNC_EXTERN	KEYSTRUCT	*FTTDgetch();
FUNC_EXTERN	VOID	IITDpciPrtCoordInfo();
FUNC_EXTERN	VOID	IITDposPrtObjSelected();
FUNC_EXTERN	i4	IITDlioLocInObj();

VOID	IIVTgldGetstrLocDecode();

/* Is user in insert or overstrike mode?  Default is overstrike. */
static	i4	vfinsmode = 0;

static	IICOORD	*done = NULL;
static	IICOORD	*mode = NULL;

VOID
VTshowmode(str)
reg	char	*str;				/* driver mode string */
{
	char	bufr[256];

	TDerase(frmodewin);				/* erase old mode */

	STprintf(bufr, ERget(F_VT0004_input), str);
	TDaddstr(frmodewin, bufr);

	TDtouchwin(stdmsg);
	TDrefresh(stdmsg);
}


/*
** Name:	VTgetstring	- Get a string from the user.
**
** Description:
**	Allow a user to create a new piece of text (trim, data formats, etc.)
**	or edit an existing item.  Many of the standard cursor and editing
**	commands are available through this interface.
**
** Inputs:
**	frm	The current frame, parent of the window (form) being created.
**	stry	The y coordinate of the start of the string.
**	strx	The x coordinate of the start of the string.
**	lastx	The position of the right edge of the form being created.
**	old_string	The string being edited, or if this is a new string
**			being created (edit == VF_GET_NEW) than "".
**	edit	(No longer used.)  VF_GET_NEW or VF_GET_EDIT if new or
**		existing string respectively.
**	maxlen	The maximum number of bytes allowed for this type of text.
**		If maxlen is <= 0, than set to a default of VFBUFSIZ.
**		Also do so if maxlen > VFBUFSIZ (although should never be so.)
**
** Outputs:
**	result	The resultant piece of text.
**
**	Returns:
**		None
**
**	Exceptions:
**		None
**
** Side Effects:
**	Screen is modified.
**
** History:
**	21-jul-88 (bruceb)	Added this header.
**	17-aug-91 (leighb) DeskTop Porting Change:
**		Make NextItem act the same as the Menu Key on IBM/PC.
*/

VOID
VTgetstring(frm, stry, strx, lastx, old_string, result, edit, maxlen)
FRAME	*frm;
i4	stry;
i4	strx;
i4	lastx;
char	*old_string;
u_char	*result;
i4	edit;
i4	maxlen;
{
    reg	WINDOW		*win;
    reg	KEYSTRUCT	*ks;
	i4		code;
	i4		x, y;	/* hold position of field */
	i4		flg;	/* flag associated with characters */
	i4		pmaxx;	/* maximum length of input/edit window */
	i4		px;	/* starting position of input/edit window */
	i4		strlength;	/* length of the result string */
	i4		editsz;
	FRS_EVCB	evcb;
	u_char		dummy;
	bool		activate = FALSE;
	char		buf[VFBUFSIZ + 1];
	char		bufa[VFBUFSIZ + 1];
	char		bufx[VFBUFSIZ + 1];
	i4		tx;
	i4		len;
	IICOORD		ldone;
	IICOORD		lmode;

	MEfill(sizeof(FRS_EVCB), '\0', &evcb);

	win = frm->frscr;
	editsz = max(MIN_EDTSZE, STlength(old_string));
	tx = min(strx, lastx + 1 - editsz);
	/* Make sure that start of edit window is ON the form. */
	if (tx < win->_begx)
	    tx = win->_begx;
	px = x = tx;

	if (maxlen <= 0 || maxlen > VFBUFSIZ)
		maxlen = VFBUFSIZ;

	pmaxx = min(maxlen, lastx + 1 - x);

	MEcopy(&(win->_y[stry][px]), pmaxx, buf);
	/* note: the saving of the _da and _dx array must be done because
	   we may have a box/line already painted under us..
	   !!!!! this is an important point to keep in mind whenever
	   painting attributes into the image.  See the comments in
	   ..td/box.c */

	MEcopy(&(win->_da[stry][px]), pmaxx, bufa);
	MEcopy(&(win->_dx[stry][px]), pmaxx, bufx);

	if ((win = TDsubwin(frm->frscr, 1, pmaxx, stry, px, NULL)) == NULL)
	{
		IIUGbmaBadMemoryAllocation(ERx("VTgetstring"));
	}

	MEfill((u_i2)win->_maxx, (u_char)' ', win->_y[0]);
	MEfill((u_i2)win->_maxx, (u_char)0, win->_da[0]);
	MEfill((u_i2)win->_maxx, (u_char)0, win->_dx[0]);

	/*
	** get temporary window area for inserting a character
	*/
	tempwin = TDtmpwin(tempwin, win->_maxy, win->_maxx);
	if ((win->_maxx = STlength(old_string)) == 0)
	{
		win->_maxx = 1;
	}

	/*
	**  Fix for BUG 7498. (dkh)
	*/
	frm->frscr->lvcursor = TRUE;

	TDmvstrwadd(win, 0, 0, old_string);
	TDmove(win, 0, 0);
	TDrefresh(win);
	if (vfinsmode)
	{
		VTshowmode(ERget(F_VT0001_INSERT));
	}
	else
	{
		VTshowmode(ERget(F_VT0002_OVSTRK));
	}
	TDrefresh(win);

	done = &ldone;
	mode = &lmode;
	done->begy = done->endy = mode->begy = mode->endy = LINES - 1;
	mode->begx = frmodewin->_begx;
	mode->endx = frmodewin->_begx + frmodewin->_maxx - 1;
	IITDpciPrtCoordInfo(ERx("VTgetstring"), ERx("mode"), mode);
	done->begx = stdmsg->_begx;
	done->endx = stdmsg->_begx + stdmsg->_maxx - 1;
	IITDpciPrtCoordInfo(ERx("VTgetstring"), ERx("done"), done);

	for(;;)		/* for ever */
	{
		evcb.event = fdNOCODE;
		ks = FTTDgetch(win);

#ifndef	PMFE
		/* Folding the newline key into the CR key for this routine */
		if (ks->ks_ch[0] == '\n')
			ks->ks_ch[0] = '\r';
#endif

		if (FKmapkey(ks, IIVTgldGetstrLocDecode, &code, &flg,
		    &activate, &dummy, &evcb) != OK)
		{
			FTbell();
			continue;
		}
		if (evcb.event == fdopMUITEM || evcb.event == fdopFRSK)
		{
			FTbell();
			/*
			**  Move screen cursor back to where user is
			**  editing.  It was moved somewhere else
			**  by FTvisclumu().
			*/
			TDrefresh(win);
			continue;
		}

		if(!(flg & fdcmINSRT))	/* if it isn't accepted by INSERT */
		{
			code = fdopERR;		/* then bad code, add as is */
			flg = fdcmNULL;
		}

		/*
		**  Using if-then-else construct to reduce code size. (dkh)
		*/

		if (code == fdopRET)
		{
			frm->frchange = TRUE;
			y = win->_cury;
			x = win->_curx;
			TDclrbot(win, (bool) FALSE);
			TDmove(win, y, x);

			break;
		}
		else if (code == fdopDELF)
		{
			/*
			** Delete from the present position
			** to the end of the field
			*/

			frm->frchange = TRUE;
			TDdelch(win);
			TDrefresh(win);
		}
		else if (code == fdopCLRF)
		{
			/* Clear the entire field */
			frm->frchange = TRUE;
			TDclear(win);
			TDrefresh(win);
		}
		else if (code == fdopRUB)
		{
			/* Delete the character to the left of the cursor */
			frm->frchange = TRUE;
			y = win->_cury;
			x = win->_curx;
			if (x == win->_maxx - 1
				&& ((win->_y[y][x] & 0177) != ' '))
			{
				TDdelch(win);
			}
			else if (TDmvleft(win, 1) != FAIL)
			{
				TDdelch(win);
			}
			else
			{
				FTbell();
			}
			TDrefresh(win);
		}
		else if (code == fdopREFR)
		{
			/* Refresh the screen */
			TDrefresh(curscr);
			TDtouchwin(win);
			TDrefresh(win);
		}
		else if (code == fdopWORD)
		{
			/*
			**  Second arg is 0 since it is never
			**  used by FTword when called by VIFRED.
			*/
			FTword(frm, 0, win, 1);
			TDrefresh(win);
		}
		else if (code == fdopBKWORD)
		{
			/*
			**  Second arg is 0 since it is never
			**  used by FTword when called by VIFRED.
			*/
			FTword(frm, 0, win, -1);
			TDrefresh(win);
		}
		else if (code == fdopLEFT)
		{
			/* move the cursor left in the field
			*/
			TDmvleft(win, 1);
			TDrefresh(win);
		}
		else if (code == fdopRIGHT)
		{
			/* move the cursor right in the field
			*/
			TDmvright(win, 1);
			TDrefresh(win);
		}
		else if (code == fdopERR)
		{
			/* Character is unrecognized, add it
			** to the field as is.
			*/

			if (CMcntrl(ks->ks_ch))
			{
				/*
				** If the character is a control character
				** then error, ignore it.
				*/
				FTbell();
				continue;
			}
			frm->frchange = TRUE;
			y = win->_cury;
			x = win->_curx;
			if (vfinsmode)
			{
				/*
				** have to set '2' for a Double Bytes chracter
				** because of adjtTab's extend of window maxx
				*/
				adjtTab(win, ks->ks_ch[0],
						CMbytecnt(ks->ks_ch), pmaxx);
				TDinsstr(win, ks->ks_ch);
			}
			else
			{
				if (x >= win->_maxx - CMbytecnt(ks->ks_ch))
				{
					adjtTab(win, ks->ks_ch[0],
						CMbytecnt(ks->ks_ch), pmaxx);
				}
				else
				{
					adjtTab(win, ks->ks_ch[0], 0, pmaxx);
				}
				TDaddchar(win, ks->ks_ch, (bool) FALSE);
				TDtouchwin(win);
			}
			TDrefresh(win); /* display the change */
		}
		else if ((code == fdopMENU) || (code == fdopNXITEM))
		{
			break;
		}
		else if (code == fdopMODE)
		{
			vfinsmode = 1 - vfinsmode;
			if (vfinsmode)
			{
				VTshowmode(ERget(F_VT0001_INSERT));
			}
			else
			{
				VTshowmode(ERget(F_VT0002_OVSTRK));
			}
			TDrefresh(win);
		}
		else if (code == fdopSHELL)
		{
			FTshell();
		}
		else
		{
			FTbell();
		}
	}

	/* Retrieve the new text */
	vfwinstr(win, result);

	win = frm->frscr;

	strlength = STlength((char *) result);

	if (px == strx)
	{
	    MEcopy((buf + strlength), (pmaxx - strlength),
		(&(win->_y[stry][px]) + strlength));
	    MEcopy((bufa + strlength), (pmaxx - strlength),
		(&(win->_da[stry][px]) + strlength));
	    MEcopy((bufx + strlength), (pmaxx - strlength),
		(&(win->_dx[stry][px]) + strlength));
	}
	else
	{
	    /* Window was shifted left for editting elbow-room. */
	    MEcopy(buf, pmaxx, &(win->_y[stry][px]));
	    MEcopy(bufa, pmaxx, &(win->_da[stry][px]));
	    MEcopy(bufx, pmaxx, &(win->_dx[stry][px]));

	    /*
	    ** Now put down the string--may not even overlap the above
	    ** regions, hence the seperate handling.
	    */
	    len = min(strlength, (lastx - strx + 1));
	    MEcopy((PTR) result, (u_i2) len, (PTR) &(win->_y[stry][strx]));
	    MEfill((u_i2)len, (u_char)0, &(win->_da[stry][strx]));
	    MEfill((u_i2)len, (u_char)0, &(win->_dx[stry][strx]));
	}

	win->_firstch[stry] = _NOCHANGE;
	win->_lastch[stry] = _NOCHANGE;

	/*
	**  Fix for BUG 7498. (dkh)
	*/
	frm->frscr->lvcursor = FALSE;
}



/*
** Name:	adjtTab	- Set up for extension of window size if user typed
**			  past current end of window.
**
** Description:
**	Set up for extension of window size (maxx) if user typed past the
**	current end of the window.  If character typed is a tab then add
**	a number of spaces up to the next 'tab stop'.
**
** Inputs:
**	win	The window being used, possibly being expanded.
**	ch	Used to determine if the character typed was a tab character.
**	def	Number of bytes being entered if a non-tab character.
**	pmaxx	Maximum width the window can get.
**
** Outputs:
**
**	Returns:
**		None
**
**	Exceptions:
**		None
**
** Side Effects:
**	Window's width may change.
**
** History:
**	21-jul-88 (bruceb)	Added this header.
*/

VOID
adjtTab(win, ch, def, pmaxx)
WINDOW	*win;
char	ch;
i4	def;
i4	pmaxx;
{
	i4	x,
		amt;

	if (ch == '\t')
	{
		x = win->_curx;
		amt = x + (8 - (x & 07));
	}
	else
	{
		amt = def;
	}
	adjtWin(win, amt, pmaxx);
}

/*
** Name:	adjtWin	- Increase the maxx of the window.
**
** Description:
**	If the window can still get wider than it currently is (if it's
**	still less than the maximum allowed), than expand it.
**
** Inputs:
**	win	The window being used, possibly being expanded.
**	amt	Number of bytes to expand the window.
**	pmaxx	Maximum width the window can get.
**
** Outputs:
**
**	Returns:
**		None
**
**	Exceptions:
**		None
**
** Side Effects:
**	Window's width may change.
**
** History:
**	21-jul-88 (bruceb)	Added this header.
*/

VOID
adjtWin(win, amt, pmaxx)
WINDOW	*win;
i4	amt;
i4	pmaxx;
{
	amt = win->_maxx + amt;
	if (amt > pmaxx)
		amt = pmaxx;
	win->_maxx = amt;
}


/*
** Name:	vfwinstr	- Get the string from the window.
**
** Description:
**	Get the entered or edited string from the window.  Remove any
**	trailing blanks, and delete the window.
**
** Inputs:
**	win	The window from whence the string comes.
**
** Outputs:
**	result	The buffer in which the string will be returned.
**
**	Returns:
**		None
**
**	Exceptions:
**		None
**
** Side Effects:
**	None
**
** History:
**	21-jul-88 (bruceb)	Added this header.
**	19-aug-92 (leighb) 	DeskTop Porting Change:
**			check that 'cp' is not less than 'result'
*/

VOID
vfwinstr(win, result)
reg WINDOW	*win;
u_char		*result;
{
	register i4	i;
	register u_char *cp;

	STcopy(ERx(""), result);
	cp = result;
	for (i = 0; i < win->_maxx; i++)
	{
		TDmove(win, 0, i);
		*cp++ = TDinch(win);
	}
	do {
		*cp-- = (u_char)'\0';
	} while ((*cp == (u_char)' ') && (cp >= result));
	TDdelwin(win);
}


/*{
** Name:	IIVTgldGetstrLocDecode	- Determine effect of locator click.
**
** Description:
**	Determine whether or not the user clicked on a legal location.
**	Only 'mode' trim and rest of menu line are legal.
**
** Inputs:
**	key	User input.
**
** Outputs:
**	evcb
**	    .event	fdNOCODE if illegal location; fdopMODE and fdonMENU
**			if legal.
**
**	Returns:
**		None.
**
**	Exceptions:
**
** Side Effects:
**
** History:
**	23-apr-90 (bruceb)	Written.
*/
VOID
IIVTgldGetstrLocDecode(key, evcb)
KEYSTRUCT	*key;
FRS_EVCB	*evcb;
{
    i4		row;
    i4		col;

    row = key->ks_p1;
    col = key->ks_p2;

    /*
    ** Must check 'mode' before 'done' because the two regions overlap.
    */
    if (IITDlioLocInObj(mode, row, col))
    {
	evcb->event = fdopMODE;
	IITDposPrtObjSelected(ERx("mode"));
    }
    else if (IITDlioLocInObj(done, row, col))
    {
	evcb->event = fdopMENU;
	IITDposPrtObjSelected(ERx("done"));
    }
    else
    {
	evcb->event = fdNOCODE;
    }
}
