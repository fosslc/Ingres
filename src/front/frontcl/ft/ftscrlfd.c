/*
**	Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	"ftframe.h"
# include	<scroll.h>
# include	<er.h>
# include	<erft.h>
# include	<me.h>
# include	<cm.h>
# include	<te.h>


/**
** Name:	ftscrlfd.c -	Routines to handle scrolling fields
**
** Description:
**	This file defines:
**
**	IIFTmlsMvLeftScroll		Move cursor left in window
**	IIFTmrsMvRightScroll		Move cursor right in window
**	IIFTdcsDelCharScroll		Delete current character
**	IIFTmdsMvDelcharScroll		Move to prev character, than delete it
**	IIFTrbsRuboutScroll		Delete char next to current character
**	IIFTcsfClrScrollFld		Clear entire field
**	IIFTcesClrEndScrollFld		Clear to end of field from current char
**	IIFTsacScrollAddChar		Add character in scrolling field
**	IIFTsicScrollInsertChar		Insert character in scrolling field
**	IIFTnwsNextWordScroll		Move to next word in scrolling field
**	IIFTpwsPrevWordScroll		Move to prev word in scrolling field
**	IIFTsfrScrollFldReset		Display field from start of data
**	IIFTsfeScrollFldEnd		Display end of field
**	IIFTgssGetScrollStruct		Return pointer to a SCROLL_DATA struct
**	IIFTsfsScrollFldStart		Is user at start of the scroll buffer?
**
**	Internal routines:
**		IIFTmfsMvForwardScroll	Underlying routine for mvleft/mvright
**		IIFTmbsMvBackScroll	Underlying routine for mvleft/mvright
**		IIFTsbfShiftBuf		Shift partial contents of buffer
**		IIFTfssFixScr_start	Reset start of visible window within
**					scroll buffer
**		IIFTlswLastInScrollWin	Determine whether current position will
**					permit further movement to the right
**					within the visible window
**		IIFTsfaScrollFldAddstr	Call TDxaddstr, first setting the
**					window's cury and curx.
**		IIFTsdbScrollDebug	Print debugging info for scroll fields.
**
**	All the routines in this file refer to the SCROLL_DATA struct,
**	defined in scroll.h (at least originally) as:
**		typedef struct
**		{
**			char	*left;
**			char	*right;
**			char	*scr_start;
**			char	*cur_pos;
**		} SCROLL_DATA;
**
**	The pointers in the SCROLL_DATA struct point at an underlying buffer
**	(referred to as under_buf) which contains the entire contents of
**	the field, both visible and scrolled.  The pointers have the following
**	purposes: left and right point to the ends of under_buf.  scr_start
**	points at the character that is the first visible character in the
**	field's window, and cur_pos points to the character on which the
**	cursor is resting within the window (equivalent in intent to the
**	window's _cury, _curx.)
**
**	Note:  As an implementation aid, under_buf will extend one byte past
**	the 'right' pointer.  This is so that TDxaddstr will point to valid
**	(blanked) memory even when the end of under_buf is one byte in from
**	the end of the visible window due to double byte character display.
**
** History:
**	03-nov-87 (bruceb)	Initial implementation.
**	06-may-88 (bruceb)
**		Make arg to CMbytecnt be a pointer, not a char.
**	05/20/88 (dkh) - ER changes.
**	20-apr-89 (bruceb)
**		Added IIFTsfsScrollFldStart routine.
**	01-feb-90 (bruceb)
**		Moved TEflush's into FTbell.
**	31-may-90 (rog)
**		Included <me.h> so MEfill() is defined correctly.
**	05-jun-96 (chech02)
**		cast arguments to correct data types in 
**		IIFTsbfShiftBuf() and IIFTlswLastInScrollWin() calls.
**      23-sep-96 (mcgem01)
**              Global data moved to ftdata.c
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/* # define's */
# define	LEFT	0	/* indicates scrolling data to the left */
# define	RIGHT	1	/* indicates scrolling data to the right */

/*
** This is defined here for now (until product has been out for awhile).
** Should be removed once satisfied that product has stabalized.
*/
# define	scrlDEBUG

/* GLOBALREF's */
/*
** IIFTdsi is short for IIFTdsiDumpScrollInfo.  It's short since
** I don't want to type the entire string constantly to turn on
** and off debugging info.  It's a i4  instead of a bool because
** it's easier to modify a i4  quantity in the debugger than it
** is to modify a one byte quantity. (bruceb)
*/
# ifdef	scrlDEBUG
GLOBALREF	i4	IIFTdsi;
# endif

/* extern's */
FUNC_EXTERN	bool	IIFTlswLastInScrollWin();
FUNC_EXTERN	VOID	IIFTmfsMvForwardScroll();
FUNC_EXTERN	VOID	IIFTmbsMvBackScroll();
FUNC_EXTERN	VOID	IIFTsbfShiftBuf();
FUNC_EXTERN	VOID	IIFTfssFixScr_start();
FUNC_EXTERN	VOID	IIFTsfaScrollFldAddstr();
# ifdef	scrlDEBUG
FUNC_EXTERN	VOID	IIFTsdbScrollDebug();
# endif

/* static's */

/*{
** Name:	IIFTmlsMvLeftScroll	- Move cursor left in window
**
** Description:
**	Move cursor left one character in the window.
**
** Inputs:
**	win		Window for the field.
**	scroll		Scroll struct for currently active field.
**	reverse		Indicate direction in which characters are read.
**
** Outputs:
**
**	Returns:
**		VOID
**
**	Exceptions:
**		None
**
** Side Effects:
**	Cursor moves one character left in the window.
**
** History:
**	03-nov-87 (bruceb)	Initial implementation.
*/
VOID
IIFTmlsMvLeftScroll(win, scroll, reverse)
WINDOW		*win;
SCROLL_DATA	*scroll;
bool		reverse;
{
	if (reverse)
		IIFTmfsMvForwardScroll(win, scroll, reverse);
	else
		IIFTmbsMvBackScroll(win, scroll, reverse);
}

/*{
** Name:	IIFTmrsMvRightScroll	- Move cursor right in window
**
** Description:
**	Move cursor right one character in the window.
**
** Inputs:
**	win		Window for the field.
**	scroll		Scroll struct for currently active field.
**	reverse		Indicate direction in which characters are read.
**
** Outputs:
**
**	Returns:
**		VOID
**
**	Exceptions:
**		None
**
** Side Effects:
**	None
**
** History:
**	03-nov-87 (bruceb)	Initial implementation.
*/
VOID
IIFTmrsMvRightScroll(win, scroll, reverse)
WINDOW		*win;
SCROLL_DATA	*scroll;
bool		reverse;
{
	if (reverse)
		IIFTmbsMvBackScroll(win, scroll, reverse);
	else
		IIFTmfsMvForwardScroll(win, scroll, reverse);
}

/*{
** Name:	IIFTmfsMvForwardScroll	- Underlying routine for mvleft/mvright
**
** Description:
**	Move cursor forward one character in the window.
**	Special cases:
**	*	If movement results in cursor being out of view, scroll
**		underlying data.
**	*	If cursor is already at end of under_buf, do nothing. 
**
** Inputs:
**	win		Window for the field.
**	scroll		Scroll struct for currently active field.
**	reverse		Indicate direction in which characters are read.
**
** Outputs:
**
**	Returns:
**		VOID
**
**	Exceptions:
**		None
**
** Side Effects:
**	Cursor moves one character towards the end of the window.
**	Data redrawn if movement caused cursor to leave visible window.
**
** History:
**	03-nov-87 (bruceb)	Initial implementation.
*/
VOID
IIFTmfsMvForwardScroll(win, scroll, reverse)
WINDOW		*win;
SCROLL_DATA	*scroll;
bool		reverse;
{
	bool		dblchar;
	i4		dispwidth;
	i4		cury;
	i4		curx;

# ifdef	scrlDEBUG
	IIFTsdbScrollDebug(win, scroll, reverse);
# endif

	dispwidth = win->_maxy * win->_maxx;

	if (reverse)
	{
		/* Check if cursor is at the last position of the visible win */
		if ((win->_cury == win->_maxy - 1) && (win->_curx == 0))
		{
			if (scroll->cur_pos != scroll->left) /* can scroll */
			{
				scroll->scr_start--;
				scroll->cur_pos--;
				IIFTsfaScrollFldAddstr(win, dispwidth,
					(scroll->scr_start - dispwidth + 1));
				win->_cury = win->_maxy - 1;
				win->_curx = 0;
			}
		}
		else	/* just move cursor */
		{
			scroll->cur_pos--;
			TDrmvleft(win, 1);
		}
	}
	else	/* left-to-right field */
	{
		dblchar = CMdbl1st(scroll->cur_pos);

		if (IIFTlswLastInScrollWin(win, dblchar, scroll))
		{
			if ((scroll->cur_pos < (scroll->right - 1))
				|| ((scroll->cur_pos == (scroll->right - 1))
					&& !dblchar))
			{	/* can scroll */
				CMnext(scroll->cur_pos);
				IIFTfssFixScr_start(win, scroll, &cury, &curx);
				IIFTsfaScrollFldAddstr(win, dispwidth,
					scroll->scr_start);
				win->_cury = cury;
				win->_curx = curx;
			}
		}
		else	/* just move cursor */
		{
			CMnext(scroll->cur_pos);
			TDmvright(win, 1);
		}
	}
}

/*{
** Name:	IIFTmbsMvBackScroll	- Underlying routine for mvleft/mvright
**
** Description:
**	Move cursor backward one character in the window.
**	Special cases:
**	*	If movement results in cursor being out of view, scroll
**		underlying data.
**	*	If cursor is already at start of under_buf, do nothing. 
**
** Inputs:
**	win		Window for the field.
**	scroll		Scroll struct for currently active field.
**	reverse		Indicate direction in which characters are read.
**
** Outputs:
**
**	Returns:
**		VOID
**
**	Exceptions:
**		None
**
** Side Effects:
**	Cursor moves one character towards the start of the window.
**	Data redrawn if movement caused cursor to leave visible window.
**
** History:
**	03-nov-87 (bruceb)	Initial implementation.
*/
VOID
IIFTmbsMvBackScroll(win, scroll, reverse)
WINDOW		*win;
SCROLL_DATA	*scroll;
bool		reverse;
{
	i4		dispwidth;

# ifdef	scrlDEBUG
	IIFTsdbScrollDebug(win, scroll, reverse);
# endif

	dispwidth = win->_maxy * win->_maxx;

	if (reverse)
	{
		/* Check if cursor is at the beginning of the visible win */
		if ((win->_cury == 0) && (win->_curx == win->_maxx - 1))
		{
			if (scroll->cur_pos != scroll->right) /* can scroll */
			{
				scroll->scr_start++;
				scroll->cur_pos++;
				IIFTsfaScrollFldAddstr(win, dispwidth,
					(scroll->scr_start - dispwidth + 1));
				win->_cury = 0;
				win->_curx = win->_maxx - 1;
			}
		}
		else	/* just move cursor */
		{
			scroll->cur_pos++;
			TDrmvright(win, 1);
		}
	}
	else	/* left-to-right field */
	{
		if ((win->_cury == 0) && (win->_curx == 0))
		{
			if (scroll->cur_pos != scroll->left) /* can scroll */
			{
				CMprev(scroll->scr_start, scroll->left);
				scroll->cur_pos = scroll->scr_start;
				IIFTsfaScrollFldAddstr(win, dispwidth,
					scroll->scr_start);
				win->_cury = 0;
				win->_curx = 0;
			}
		}
		else	/* just move cursor */
		{
			CMprev(scroll->cur_pos, scroll->scr_start);
			TDmvleft(win, 1);
		}
	}
}

/*{
** Name:	IIFTdcsDelCharScroll	- Delete current character
**
** Description:
**	Delete the current character by means of shifting successive
**	characters over on top of current position.  Addstr the string
**	after the shift.
**	Special case:
**	*	If current position is at last byte of window,
**		on a ascii character, and character to be shifted
**		in is a double byte character, than scr_start is
**		reset prior to adding the string to the window.
**
** Inputs:
**	win		Window for the field.
**	scroll		Scroll struct for currently active field.
**	reverse		Indicate direction in which characters are read.
**
** Outputs:
**
**	Returns:
**		VOID
**
**	Exceptions:
**		None
**
** Side Effects:
**	Screen image of window is updated.
**
** History:
**	03-nov-87 (bruceb)	Initial implementation.
**	05-sep-2008 (gupsh01)
**	    Don't assume that the byte count for a double byte character
**	    cannot be greater than 2. (Bug 120865).
*/
VOID
IIFTdcsDelCharScroll(win, scroll, reverse)
WINDOW		*win;
SCROLL_DATA	*scroll;
bool		reverse;
{
	i4		offset;
	i4		cury;
	i4		curx;
	i4		dispwidth;

# ifdef	scrlDEBUG
	IIFTsdbScrollDebug(win, scroll, reverse);
# endif

	dispwidth = win->_maxy * win->_maxx;

	cury = win->_cury;
	curx = win->_curx;

	offset = CMbytecnt(scroll->cur_pos);
	
	if (reverse)
	{
		/* Scroll from the end of under_buf onto top of current pos */
		IIFTsbfShiftBuf((i4)RIGHT, scroll->left, scroll->cur_pos, offset,
			(bool)TRUE, (bool)FALSE);
		IIFTsfaScrollFldAddstr(win, dispwidth,
			(scroll->scr_start - dispwidth + 1));
		win->_cury = cury;
		win->_curx = curx;
	}
	else
	{
		/* Scroll from the end of under_buf onto top of current pos */
		IIFTsbfShiftBuf((i4)LEFT, scroll->right, scroll->cur_pos, offset,
			(bool)TRUE, (bool)FALSE);
		/*
		** if cursor on last byte of the window (on ascii character),
		** and character scrolled in is a double byte character,
		** then need to reset scr_start.
		*/
		if ((win->_cury == win->_maxy - 1)
			&& (win->_curx == win->_maxx - 1)
			&& (CMbytecnt(scroll->cur_pos) > 1))
		{
			IIFTfssFixScr_start(win, scroll, &cury, &curx);
		}
		IIFTsfaScrollFldAddstr(win, dispwidth, scroll->scr_start);
		win->_cury = cury;
		win->_curx = curx;
	}
}

/*{
** Name:	IIFTmdsMvDelcharScroll	- Move to prev character, than delete it
**
** Description:
**	Move to character just prior to the current character, then delete it.
**	Used to patch up cur_pos to match the altered cury, curx in fdopRUB,
**	before calling IIFTdcsDelCharScroll to do the real work.
**
** Inputs:
**	win		Window for the field.
**	scroll		Scroll struct for currently active field.
**	reverse		Indicate direction in which characters are read.
**
** Outputs:
**
**	Returns:
**		VOID
**
**	Exceptions:
**		None
**
** Side Effects:
**	Screen image of window is updated.
**
** History:
**	06-nov-87 (bruceb)	Initial implementation.
*/
VOID
IIFTmdsMvDelcharScroll(win, scroll, reverse)
WINDOW		*win;
SCROLL_DATA	*scroll;
bool		reverse;
{
	if (reverse)
		scroll->cur_pos++;
	else
		scroll->cur_pos -= CMbytecnt(&win->_y[win->_cury][win->_curx]);
	IIFTdcsDelCharScroll(win, scroll, reverse);
}

/*{
** Name:	IIFTrbsRuboutScroll	- Delete char next to current character
**
** Description:
**	Only called if cursor is at leading edge of visible window.  If
**	cursor was in the middle of the visible window, than
**	IIFTdcsDelCharScroll was instead called from FTinsert.
**
**	Shift contents of under_buf, reset scr_start and cur_pos pointers,
**	but do nothing to the screen.  This is because the character to be
**	deleted is invisible--off the screen.
**	Special case:
**	*	If current position is also at the first byte of under_buf,
**		then beep and return.
**
** Inputs:
**	scroll		Scroll struct for currently active field.
**	reverse		Indicate direction in which characters are read.
**
** Outputs:
**
**	Returns:
**		VOID
**
**	Exceptions:
**		None
**
** Side Effects:
**	None
**
** History:
**	03-nov-87 (bruceb)	Initial implementation.
*/
VOID
IIFTrbsRuboutScroll(scroll, reverse)
SCROLL_DATA	*scroll;
bool		reverse;
{
	i4		offset;

	/* if at start of under_buf, then nothing to delete, so leave */
	if ((reverse && (scroll->scr_start == scroll->right))
		|| (!reverse && (scroll->scr_start == scroll->left)))
	{
		FTbell();
	}
	else	/* shift buffer only */
	{
		if (reverse)
		{
			scroll->scr_start++;
			scroll->cur_pos++;
			IIFTsbfShiftBuf((i4)RIGHT, scroll->left, scroll->scr_start,
				(i4)1, (bool)TRUE, (bool)FALSE);
		}
		else
		{
			CMprev(scroll->scr_start, scroll->left);
			offset = CMbytecnt(scroll->scr_start);
			IIFTsbfShiftBuf((i4)LEFT, scroll->right, scroll->scr_start,
				offset, (bool)TRUE, (bool)FALSE);
			/*
			** pointing to same character as before so OK
			** to simply subtract rather than calling CMprev.
			*/
			scroll->cur_pos -= offset;
		}
	}
}

/*{
** Name:	IIFTcsfClrScrollFld	- Clear entire field
**
** Description:
**	Blank out entire contents of under_buf.  Visible window
**	has already been cleared out by FTinsert.
**
** Inputs:
**	scroll		Scroll struct for currently active field.
**	reverse		Indicate direction in which characters are read.
**
** Outputs:
**
**	Returns:
**		VOID
**
**	Exceptions:
**		None
**
** Side Effects:
**	Reset scr_start and cur_pos to the start of the buffer.
**
** History:
**	03-nov-87 (bruceb)	Initial implementation.
*/
VOID
IIFTcsfClrScrollFld(scroll, reverse)
SCROLL_DATA	*scroll;
bool		reverse;
{
	/* clear out under_buf */
	MEfill((scroll->right - scroll->left + 1), ' ', scroll->left);

	if (reverse)
		scroll->scr_start = scroll->right;
	else
		scroll->scr_start = scroll->left;
	scroll->cur_pos = scroll->scr_start;
}

/*{
** Name:	IIFTcesClrEndScrollFld	- Clear to end of field from cur char
**
** Description:
**	Blank out contents of under_buf from cur_pos to end of the buffer.
**	Visible window has already been modified by FTinsert.
**
** Inputs:
**	scroll		Scroll struct for currently active field.
**	reverse		Indicate direction in which characters are read.
**
** Outputs:
**
**	Returns:
**		VOID
**
**	Exceptions:
**		None
**
** Side Effects:
**	None
**
** History:
**	03-nov-87 (bruceb)	Initial implementation.
*/
VOID
IIFTcesClrEndScrollFld(scroll, reverse)
SCROLL_DATA	*scroll;
bool		reverse;
{
	/* clear out tail end of under_buf */
	if (reverse)
	{
		MEfill((scroll->cur_pos - scroll->left + 1), ' ',
			scroll->left);
	}
	else
	{
		MEfill((scroll->right - scroll->cur_pos + 1), ' ',
			scroll->cur_pos);
	}
}

/*{
** Name:	IIFTsacScrollAddChar	- Add character in scrolling field
**
** Description:
**	Overstrike the current character in under_buf, addstr'ing
**	to window from scr_start, advancing cur_pos to next character.
**	Special cases:
**	*	If overwrote just first byte of a double byte character,
**		blank the second byte of that character.
**	*	If old position was last in the window, reset scr_start
**		before addstr'ing.
**	*	If at end of under_buf, on an ascii character, adding a
**		double byte character, do nothing and return.  (Beep will
**		be produced by FTinsert.)
**	*	If at end of under_buf, and old and new characters are
**		the same size, cur_pos is not advanced, and indicator is
**		returned to caller for possible auto_tab.
**	*	If adding an ascii character, current position is at the
**		start of a row, and previous position in the window has a
**		phantom space (indicating that current character is a double
**		byte character), then insert the ascii character before
**		cur_pos (equivalent to overstriking the phantom space).
**
** Inputs:
**	win		Window for the field.
**	scroll		Scroll struct for currently active field.
**	reverse		Indicate direction in which characters are read.
**	addch		Character to be added.
**
** Outputs:
**	nospace		Indicate that attempt made to add double byte
**			character in last byte of under_buf.
**
**	Returns:
**		OK
**		FAIL	Indication of situation for possible auto_tab.
**		(Return type should be STATUS, but it's i4  instead
**		for consistancy with addch.)
**
**	Exceptions:
**		None
**
** Side Effects:
**	Screen image of window is updated.
**
** History:
**	03-nov-87 (bab)	Initial implementation.
**	05-oct-89 (kathryn) KANJI - Changed dblnew,dblold from bool to
**		i4 to workaround a compiler optimization bug.
*/
i4
IIFTsacScrollAddChar(win, scroll, reverse, addch, nospace)
WINDOW		*win;
SCROLL_DATA	*scroll;
bool		reverse;
char		*addch;
bool		*nospace;
{
	i4		dblnew;
	i4		dblold;
	bool		atwinend;	/* indicate if at end of window */
	i4		dispwidth;
	i4		cury;
	i4		curx;
	i4		retval = OK;

# ifdef	scrlDEBUG
	IIFTsdbScrollDebug(win, scroll, reverse);
# endif

	*nospace = FALSE;
	dispwidth = win->_maxy * win->_maxx;
	cury = win->_cury;
	curx = win->_curx;

	if (reverse)
	{
		*(scroll->cur_pos) = *addch;

		if (scroll->cur_pos > scroll->left)
		{
			scroll->cur_pos--;
			/* check if moved off visible window */
			if (atwinend = (cury == win->_maxy - 1 && curx == 0))
				scroll->scr_start--;
		}
		else
		{
			retval = FAIL;
			atwinend = FALSE;
		}

		IIFTsfaScrollFldAddstr(win, dispwidth,
			(scroll->scr_start - dispwidth + 1));
		win->_cury = cury;
		win->_curx = curx;

		if (!atwinend)
			TDrmvleft(win, 1);	/* advance cury, curx */
	}
	else	/* LR field */
	{
		dblnew = CMdbl1st(addch);
	
		if (dblnew && (scroll->cur_pos == scroll->right))
		{
			*nospace = TRUE;
			return(OK);
		}
		
		if (!dblnew && curx == 0 && cury > 0
			&& (win->_dx[cury - 1][win->_maxx - 1] & _PS))
		{
			/*
			** Handle the special case of typing an ascii character
			** over a double byte character after a phantom space
			*/
			IIFTsbfShiftBuf((i4)RIGHT, scroll->cur_pos, scroll->right,
				(i4)1, (bool)TRUE, (bool)TRUE);
		}
	
		dblold = CMdbl1st(scroll->cur_pos);
		*(scroll->cur_pos) = *addch;
		if (!dblnew)
		{
			if (dblold)
			{
				/*
				** overwrote first byte of double byte
				** character
				*/
				*(scroll->cur_pos + 1) = ' ';
			}
		}
		else	/* dblnew */
		{
			/*
			** ((cur_pos + 1) <= right), else would have returned
			** to calling routine--see above.
			*/
			if (!dblold && CMdbl1st(scroll->cur_pos + 1))
			{
				/*
				** overwrote first byte of double byte
				** character
				*/
				*(scroll->cur_pos + 2) = ' ';
			}
			*(scroll->cur_pos + 1) = *(addch + 1);
		}
	
		atwinend = IIFTlswLastInScrollWin(win, (bool)dblnew, scroll);
	
		/* Don't advance position if at end of buffer */
		if ((scroll->cur_pos < (scroll->right - 1))
		    || ((scroll->cur_pos == (scroll->right - 1))
			&& !dblnew))
		{
			CMnext(scroll->cur_pos);
		}
		else
		{
			/* At end of under_buf, so indicate possible auto_tab */
			retval = FAIL;
		}
	
		if (atwinend)
			IIFTfssFixScr_start(win, scroll, &cury, &curx);
		
		IIFTsfaScrollFldAddstr(win, dispwidth, scroll->scr_start);
		win->_cury = cury;
		win->_curx = curx;
	
		if (!atwinend)
			TDmvright(win, 1);	/* advance cury, curx */
	}

	return(retval);
}

/*{
** Name:	IIFTsicScrollInsertChar	- Insert character in scrolling field
**
** Description:
**	Insert character prior to current character in under_buf,
**	addstr'ing from scr_start, advancing cur_pos to next character.
**	Special cases:
**	*	If old position was last in the window, reset scr_start
**		before addstr'ing.
**	*	If at end of under_buf, on an ascii character, inserting
**		a double byte character, do nothing and return.  (Beep
**		will be produced by FTinsert.)
**	*	If at end of under_buf, and old and new characters are
**		the same size, cur_pos is not advanced.
**	*	If inserting an ascii character, current position
**		is at the start of a row, and previous window position
**		has a phantom space (indicating that current character
**		is a double byte character), then the new character
**		will be placed where the phantom space was previously
**		positioned.
**
** Inputs:
**	win		Window for the field.
**	scroll		Scroll struct for currently active field.
**	reverse		Indicate direction in which characters are read.
**	insch		Character to be inserted.
**
** Outputs:
**	nospace		Indicate that attempt made to insert double byte
**			character in last byte of under_buf.
**
**	Returns:
**		OK
**		(Return type should be STATUS, but it's i4  instead
**		for consistancy with insch.)
**
**	Exceptions:
**		None
**
** Side Effects:
**	Screen image of window is updated.
**
** History:
**	03-nov-87 (bab)	Initial implementation.
**	05-oct-89 (kathryn) KANJI - Changed type of dblnew from bool to i4
**		to work around a compiler optimization bug.
*/
i4
IIFTsicScrollInsertChar(win, scroll, reverse, insch, nospace)
WINDOW		*win;
SCROLL_DATA	*scroll;
bool		reverse;
char		*insch;
bool		*nospace;
{
	i4	dblnew;
	bool	atwinend;	/* indicate if at end of window */
	i4	dispwidth;
	i4	cury;
	i4	curx;

# ifdef	scrlDEBUG
	IIFTsdbScrollDebug(win, scroll, reverse);
# endif

	*nospace = FALSE;
	dispwidth = win->_maxy * win->_maxx;
	cury = win->_cury;
	curx = win->_curx;

	if (reverse)
	{
		IIFTsbfShiftBuf((i4)LEFT, scroll->cur_pos, scroll->left, (i4)1,
			(bool)FALSE, (bool)FALSE);
		*(scroll->cur_pos) = *insch;

		if (scroll->cur_pos > scroll->left)
		{
			scroll->cur_pos--;
			/* check if moved off visible window */
			if (atwinend = (cury == win->_maxy - 1 && curx == 0))
				scroll->scr_start--;
		}
		else
		{
			atwinend = FALSE;
		}

		IIFTsfaScrollFldAddstr(win, dispwidth,
			(scroll->scr_start - dispwidth + 1));
		win->_cury = cury;
		win->_curx = curx;
		if (!atwinend)
			TDrmvleft(win, 1);	/* advance cury, curx */
	}
	else	/* LR field */
	{
		dblnew = CMdbl1st(insch);
	
		if (dblnew && (scroll->cur_pos == scroll->right))
		{
			*nospace = TRUE;
			return(OK);
		}
		
		IIFTsbfShiftBuf((i4)RIGHT, scroll->cur_pos, scroll->right,
			CMbytecnt(insch), (bool)FALSE, (bool)TRUE);
		
		*(scroll->cur_pos) = *insch;
		if (dblnew)
			*(scroll->cur_pos + 1) = *(insch + 1);
	
		atwinend = IIFTlswLastInScrollWin(win, (bool)dblnew, scroll);
	
		/* only advance if not at end of buffer */
		if ((scroll->cur_pos < (scroll->right - 1))
		    || ((scroll->cur_pos == (scroll->right - 1)) && !dblnew))
		{
			CMnext(scroll->cur_pos);
		}
	
		if (atwinend)
			IIFTfssFixScr_start(win, scroll, &cury, &curx);
		
		IIFTsfaScrollFldAddstr(win, dispwidth, scroll->scr_start);
		win->_cury = cury;
		win->_curx = curx;
	
		if (!atwinend)
			TDmvright(win, 1);	/* advance cury, curx */
	}

	return(OK);
}

/*{
** Name:	IIFTnwsNextWordScroll	- Move to next word in scrolling field
**
** Description:
**	Move to the next word in a scrolling field.  The definition of a word
**	is as for FTword, the caller of this routine.  Calculations are
**	performed against under_buf, later addstr'ing the result.
**
** Inputs:
**	win		Window for the field.
**	scroll		Scroll struct for currently active field.
**	reverse		Indicate direction in which characters are read.
**
** Outputs:
**
**	Returns:
**		VOID
**
**	Exceptions:
**		None
**
** Side Effects:
**	Cursor is moved within the window, and screen image of the
**	window may be updated.
**
** History:
**	03-nov-87 (bruceb)	Initial implementation.
**	05-sep-2008 (gupsh01)
**	    Don't assume that the byte count for a double byte character
**	    cannot be greater than 2. (Bug 120865).
*/
VOID
IIFTnwsNextWordScroll(win, scroll, reverse)
WINDOW		*win;
SCROLL_DATA	*scroll;
bool		reverse;
{
	i4		dispwidth;
	i4		cury;
	i4		curx;
	register char	*cur_ptr;

# ifdef	scrlDEBUG
	IIFTsdbScrollDebug(win, scroll, reverse);
# endif

	dispwidth = win->_maxy * win->_maxx;
	cur_ptr = scroll->cur_pos;
	
	if (reverse)
	{
		/* If not on a space, skip non-spaces */
		while (!CMwhite(cur_ptr) && (cur_ptr != scroll->left))
			cur_ptr--;
		/* Skip spaces to next word */
		while (CMwhite(cur_ptr) && (cur_ptr != scroll->left))
			cur_ptr--;

		/* if past the end of the window */
		if (cur_ptr < (scroll->scr_start - dispwidth + 1))
		{
			scroll->scr_start = (cur_ptr + dispwidth - 1);
			IIFTsfaScrollFldAddstr(win, dispwidth, cur_ptr);
			/* at end of window */
			win->_cury = win->_maxy - 1;
			win->_curx = 0;
		}
		else
		{
			/* DEALING only with single line window */
			win->_curx -= (scroll->cur_pos - cur_ptr);
		}
	}
	else	/* LR field */
	{
		/* If not on a space, skip non-spaces */
		while (!CMwhite(cur_ptr) && (cur_ptr != scroll->right))
			CMnext(cur_ptr);
		/* Skip spaces to next word; stop at end of buffer. */
		while (CMwhite(cur_ptr) && (cur_ptr != scroll->right)
			&& !((CMbytecnt(cur_ptr) > 1)
			    && (cur_ptr == scroll->right - 1)) )
		{
			CMnext(cur_ptr);
		}
		    
		/* SINGLE LINE ASSUMPTIONS */
		/* If current position has moved off the visible window */
		if ((cur_ptr > (scroll->scr_start + dispwidth - 1))
			|| ((cur_ptr == (scroll->scr_start + dispwidth - 1))
				&& (CMbytecnt(cur_ptr) > 1)))
		{
			scroll->cur_pos = cur_ptr;
			IIFTfssFixScr_start(win, scroll, &cury, &curx);
			IIFTsfaScrollFldAddstr(win, dispwidth,
				scroll->scr_start);
			win->_cury = cury;
			win->_curx = curx;
		}
		else
		{
			/* win->_cury has only one possible value */
			win->_curx += (cur_ptr - scroll->cur_pos);
		}
	}
		
	scroll->cur_pos = cur_ptr;
}

/*{
** Name:	IIFTpwsPrevWordScroll	- Move to prev word in scrolling field
**
** Description:
**	Move to the previous word in a scrolling field.  The definition of
**	a word is as for FTword, the caller of this routine.  Calculations are
**	performed against under_buf, later addstr'ing the result.
**
** Inputs:
**	win		Window for the field.
**	scroll		Scroll struct for currently active field.
**	reverse		Indicate direction in which characters are read.
**
** Outputs:
**
**	Returns:
**		VOID
**
**	Exceptions:
**		None
**
** Side Effects:
**	Cursor is moved within the window, and screen image of the
**	window may be updated.
**
** History:
**	03-nov-87 (bruceb)	Initial implementation.
*/
VOID
IIFTpwsPrevWordScroll(win, scroll, reverse)
WINDOW		*win;
SCROLL_DATA	*scroll;
bool		reverse;
{
	i4		dispwidth;
	i4		cury;
	i4		curx;
	register char	*cur_ptr;

# ifdef	scrlDEBUG
	IIFTsdbScrollDebug(win, scroll, reverse);
# endif

	dispwidth = win->_maxy * win->_maxx;
	cur_ptr = scroll->cur_pos;
	
	if (reverse)
	{
		/* Start with previous character */
		if (cur_ptr != scroll->right)
			cur_ptr++;
		/* Skip spaces to end of previous word */
		while (CMwhite(cur_ptr) && (cur_ptr != scroll->right))
			cur_ptr++;
		/* Skip non-spaces to just before word */
		while (!CMwhite(cur_ptr) && (cur_ptr != scroll->right))
			cur_ptr++;
		/* Pointing at space before word; advance */
		if (cur_ptr != scroll->right)
			cur_ptr--;

		/* if past the start of the window */
		if (cur_ptr > scroll->scr_start)
		{
			scroll->scr_start = cur_ptr;
			IIFTsfaScrollFldAddstr(win, dispwidth,
				(cur_ptr - dispwidth + 1));
			/* at start of window */
			win->_cury = 0;
			win->_curx = win->_maxx - 1;
		}
		else
		{
			/*
			** Calculation is based on a delta, and doesn't
			** apply to movement over row boundaries.
			*/
			win->_curx += (cur_ptr - scroll->cur_pos);
		}
	}
	else	/* LR field */
	{
		/* Start with previous character */
		if (cur_ptr != scroll->left)
			CMprev(cur_ptr, scroll->left);
		/* Skip spaces to end of previous word */
		while (CMwhite(cur_ptr) && (cur_ptr != scroll->left))
			CMprev(cur_ptr, scroll->left);
		/* Skip non-spaces to just before word */
		while (!CMwhite(cur_ptr) && (cur_ptr != scroll->left))
			CMprev(cur_ptr, scroll->left);
		/* Pointing at space before word; advance */
		if (cur_ptr != scroll->left)
			CMnext(cur_ptr);

		/* if past the start of the window */
		if (cur_ptr < scroll->scr_start)
		{
			scroll->scr_start = cur_ptr;
			IIFTsfaScrollFldAddstr(win, dispwidth, cur_ptr);
			/* at start of window */
			win->_cury = 0;
			win->_curx = 0;
		}
		else
		{
			/* DEALING only with single line window */
			win->_curx -= (scroll->cur_pos - cur_ptr);
		}
	}

	scroll->cur_pos = cur_ptr;
}

/*{
** Name:	IIFTsfrScrollFldReset	- Display field from start of data
**
** Description:
**	Display a field's underlying data from the start of that data.
**	Effectively, scroll the window so the first byte of the underlying
**	data is visible.
**
** Inputs:
**	win		Window for the field.
**	scroll		Scroll struct for currently active field.
**	reverse		Indicate direction in which characters are read.
**
** Outputs:
**
**	Returns:
**		VOID
**
**	Exceptions:
**		None
**
** Side Effects:
**	None
**
** History:
**	03-nov-87 (bruceb)	Initial implementation.
*/
VOID
IIFTsfrScrollFldReset(win, scroll, reverse)
WINDOW		*win;
SCROLL_DATA	*scroll;
bool		reverse;
{
	i4		dispwidth;

# ifdef	scrlDEBUG
	IIFTsdbScrollDebug(win, scroll, reverse);
# endif

	dispwidth = win->_maxy * win->_maxx;

	if (reverse)
	{
		scroll->scr_start = scroll->right;
		IIFTsfaScrollFldAddstr(win, dispwidth,
			(scroll->scr_start - dispwidth + 1));
		win->_cury = 0;
		win->_curx = win->_maxx - 1;
	}
	else
	{
		scroll->scr_start = scroll->left;
		IIFTsfaScrollFldAddstr(win, dispwidth, scroll->scr_start);
		win->_cury = 0;
		win->_curx = 0;
	}
	scroll->cur_pos = scroll->scr_start;
}

/*{
** Name:	IIFTsfeScrollFldEnd	- Display end of field
**
** Description:
**	Display a field's underlying data towards the end of that data.
**	Effectively, scroll the window so the last byte of the underlying
**	data is visible.
**
** Inputs:
**	win		Window for the field.
**	scroll		Scroll struct for currently active field.
**	reverse		Indicate direction in which characters are read.
**
** Outputs:
**
**	Returns:
**		VOID
**
**	Exceptions:
**		None
**
** Side Effects:
**	None
**
** History:
**	25-nov-87 (bruceb)	Initial implementation.
*/
VOID
IIFTsfeScrollFldEnd(win, scroll, reverse)
WINDOW		*win;
SCROLL_DATA	*scroll;
bool		reverse;
{
	i4		dispwidth;
	i4		cury;
	i4		curx;
	char		*ptr;

# ifdef	scrlDEBUG
	IIFTsdbScrollDebug(win, scroll, reverse);
# endif

	dispwidth = win->_maxy * win->_maxx;

	if (reverse)
	{
		/* point to the last char in an RL buffer */
		scroll->cur_pos = scroll->left;
		scroll->scr_start = scroll->left + dispwidth - 1;
		IIFTsfaScrollFldAddstr(win, dispwidth, scroll->left);
		win->_cury = win->_maxy - 1;
		win->_curx = 0;
	}
	else
	{
		/* point to the last known char in the scrolling buffer */
		ptr = scroll->cur_pos;
		while (ptr + CMbytecnt(ptr) <= scroll->right)
			CMnext(ptr);
		
		/* ptr now points to the last char in the buffer */
		scroll->cur_pos = ptr;
		IIFTfssFixScr_start(win, scroll, &cury, &curx);
		IIFTsfaScrollFldAddstr(win, dispwidth, scroll->scr_start);
		win->_cury = cury;
		win->_curx = curx;
	}
}

/*{
** Name:	IIFTgssGetScrollStruct	- Return pointer to a scroll struct
**
** Description:
**	Return a pointer to the SCROLL_DATA structure associated with
**	the current field.
**
** Inputs:
**	fld		Field on which the cursor is currently sitting.
**
** Outputs:
**
**	Returns:
**		Pointer to the SCROLL_DATA structure for the passed in field.
**
**	Exceptions:
**		None
**
** Side Effects:
**	None
**
** History:
**	03-nov-87 (bruceb)	Initial implementation.
*/
SCROLL_DATA *
IIFTgssGetScrollStruct(fld)
FIELD	*fld;
{
	FLDVAL		*val;

	val = (*FTgetval)(fld);

	return((SCROLL_DATA *)(val->fvdatawin));
}

/*{
** Name:	IIFTsfsScrollFldStart	- Is user at start of the scroll buf?
**
** Description:
**	Determine if the user is at the start of the scroll buffer.  Used
**	in insrt.c to support clearrest semantics on a mandatory field.
**
** Inputs:
**	scroll		Scroll structure for the field.
**	reverse		Is the field right-to-left or left-to-right?
**
** Outputs:
**
**	Returns:
**		TRUE/FALSE	Is user at start of the scroll buffer?
**
**	Exceptions:
**
** Side Effects:
**	None
**
** History:
**	20-apr-89 (bruceb)	Written.
*/
bool
IIFTsfsScrollFldStart(scroll, reverse)
SCROLL_DATA	*scroll;
bool		reverse;
{
    bool	retval = FALSE;

    if ((!reverse && scroll->cur_pos == scroll->left)
	|| (reverse && scroll->cur_pos == scroll->right))
    {
	retval = TRUE;
    }

    return(retval);
}

/*{
** Name:	IIFTsbfShiftBuf	- Shift partial contents of buffer
**
** Description:
**	Shift a region of buffer, in the direction specified, by the number
**	of bytes specified.
**	Special case:
**	*	If the shift would result in half of a double byte character
**		sitting at the end of the buffer, then clear that byte.
**
** Inputs:
**	direction	Direction in which to move the data.
**	from		Point from which data is being moved.
**	to		Point towards which data is being moved.
**	numbytes	Number of bytes by which the data is being shifted.
**	blankpad	Indicate if newly opened region should be blanked.
**	clrhalfdbl	Indicate need for checking whether will shift half a
**			double byte character off end of buffer (and if so,
**			clear it.)
**
** Outputs:
**
**	Returns:
**		VOID
**
**	Exceptions:
**		None
**
** Side Effects:
**	Characters are shifted off the end of the buffer.  Double byte
**	characters may be shifted half off of the end, and subsequently
**	need to have the remaining half blanked out.
**
** History:
**	03-nov-87 (bruceb)	Initial implementation.
**	05-sep-2008 (gupsh01)
**	    Don't assume that the byte count for a double byte character
**	    cannot be greater than 2. (Bug 120865).
*/
VOID
IIFTsbfShiftBuf(direction, from, to, numbytes, blankpad, clrhalfdbl)
i4	direction;
char	*from;
char	*to;
i4	numbytes;
bool	blankpad;
bool	clrhalfdbl;
{
	char	*i;
	char	*j;
	
	if (direction == LEFT)
	{
		/*
		** buffer:  ====================
		**            ^            ^
		**            to  <------  from
		*/

		/*
		** clrhalfdbl is not used for going LEFT.  RL fields
		** never contain double byte chars, and this will never
		** be called for shifting off left end of the buffer for
		** an LR field.
		*/

		for (i = to, j = to + numbytes; j <= from; *i++ = *j++)
			;

		if (blankpad)
		{
			for (; i <= from; *i++ = ' ')
				;
		}
	}
	else	/* direction == RIGHT */
	{
		/*
		** buffer:  ====================
		**            ^              ^
		**            from  ------>  to
		*/

		j = to - numbytes;

		/*
		** When clrhalfdbl is specified, 'to' is equivalent
		** to the right-hand end of the buffer.
		*/
		if (clrhalfdbl)
		{
			/*
			** Check that the byte that will end up in the
			** last position, (to - numbytes), is the first
			** byte of a double byte character.  If so,
			** blank it.
			*/
			i = from;
			while (i < j)
				CMnext(i);
			
			if ((i == j) && (CMbytecnt(i) > 1))
				*i = ' ';
		}

		for (i = to; j >= from; *i-- = *j--)
			;

		if (blankpad)
		{
			for (; i >= from; *i-- = ' ')
				;
		}
	}
}

/*{
** Name:	IIFTfssFixScr_start	- Reset scr_start so that cur_pos
**					  character is again visible in window
**
** Description:
**	Only called for normal (LR) fields.
**	When the current character (that pointed to by cur_pos) is
**	no longer visible due to moving off the tail end of the window,
**	this routine is called to calculate a new scr_start position
**	such that the current character is minimally visible at the
**	end of the window.  Minimally visible means that scr_start
**	is advanced no more than is necessary to cause the current
**	character to show.  This routine also returns new values for
**	the window's _cury and _curx members; the window struct isn't
**	modified since typically an addstr will occur immediately after
**	this routine returns, thus changing the structure's values.
**
**	NOTE:
**	This routine is very dependent on a single line fields
**	implementation.  It will have to be modified to work with
**	multi-line fields, although the interface should suffice.
**
** Inputs:
**	win	Window of current field.
**	scroll	Scroll struct for currently active field.
**
** Outputs:
**	cury	Set to the _cury value for the window after scrolling occured.
**		THIS parameter is only going to start being useful when this
**		routine is able to handle multi-line windows.  Until that
**		time, simply set to original value in window (which is safe
**		for a single line window).
**	curx	Set to the _curx value for the window after scrolling occured.
**	scroll
**	    .scr_start	Point to the character now first in the display.
**
**	Returns:
**		VOID
**
**	Exceptions:
**		None
**
** Side Effects:
**	None
**
** History:
**	03-nov-87 (bruceb)	Initial implementation.
*/
VOID
IIFTfssFixScr_start(win, scroll, cury, curx)
WINDOW		*win;
SCROLL_DATA	*scroll;
i4		*cury;
i4		*curx;
{
	register i4	dispwidth;	/* Number bytes in the window */
	register char	*first_ptr;
	register char	*fit_endpos;

	dispwidth = win->_maxy * win->_maxx;

	/*
	** Point to the last byte of the underlying buffer that must fit
	** in the window.  This is either the current character if an
	** ascii character, or the second byte of the current double byte
	** character.
	*/
	fit_endpos = scroll->cur_pos;
	if (CMdbl1st(fit_endpos))
		fit_endpos++;	/* Points to second byte of the character */

	for (first_ptr = scroll->scr_start;
		(first_ptr + dispwidth - 1) < fit_endpos; CMnext(first_ptr))
		;

	scroll->scr_start = first_ptr;
	*curx = scroll->cur_pos - first_ptr;
	*cury = win->_cury;
}

/*{
** Name:	IIFTlswLastInScrollWin	- Determine if current position is
**					  last in visible window
**
** Description:
**	Only called for normal (LR) fields.
**	Determine whether current position will permit further movement to
**	the right within the visible window.
**
**	Unable to simply check the win struct since it is possible that the
**	end of under_buf will be displayed in from the end of the visible
**	window due to display considerations for double byte characters.
**
** Inputs:
**	win	Window of current field.
**	dblchar	Indicate if character relevent to the check is a double byte
**		character.
**	scroll	Scroll struct for currently active field.
**
** Outputs:
**
**	Returns:
**		TRUE	Current position is last usable point in visible window.
**		FALSE	Current position is not the last usable point in the
**			visible window.
**
**	Exceptions:
**		None
**
** Side Effects:
**	None
**
** History:
**	03-nov-87 (bruceb)	Initial implementation.
*/
bool
IIFTlswLastInScrollWin(win, dblchar, scroll)
WINDOW		*win;
bool		dblchar;
SCROLL_DATA	*scroll;
{
	register i4	curx = win->_curx;
	register i4	maxx = win->_maxx;
	i4		cury = win->_cury;

	/*
	** TRUE if last line of window, and within that line,
	**	either the last byte,
	**	or one byte in from end
	**		and a double char, a phantom space or early
	**		end to under_buf,
	**	or two bytes in from end and a double character,
	**		and a phantom space or early end to under_buf.
	*/
	return((cury == win->_maxy - 1)
	       && ((curx == maxx - 1)
	           || ((curx == maxx - 2)
		       && (dblchar
			  || (scroll->cur_pos == scroll->right)
			  || (win->_dx[win->_cury][maxx - 1] & _PS)))
	           || ((curx == maxx - 3)
		       && dblchar
		       && ((scroll->cur_pos == (scroll->right - 1))
		           || (win->_dx[win->_cury][maxx - 1] & _PS)))));
}

/*{
** Name:	IIFTsfaScrollFldAddstr	- Call TDxaddstr, first setting the
**					  window's cury and curx.
**
** Description:
**	Call TDxaddstr after first setting win->_cury and win->_curx to 0.
**	This is so that the string is added from the start of the window.
**	True even for reversed fields since the buffer is RL in such
**	cases, and the addstr occurs from a suitable earlier point in the
**	buffer.
**
**	Ignore return value from TDxaddstr since the only error that
**	routine returns indicates inability to emit all the bytes,
**	and I expect that to happen quite frequently.
**
** Inputs:
**	win		Window for the field.
**	width		Maximum number of bytes (not characters) to add.
**	string		The characters to add.
**
** Outputs:
**
**	Returns:
**		VOID
**
**	Exceptions:
**		None
**
** Side Effects:
**	Window is updated resulting in possible screen modification.
**
** History:
**	06-nov-87 (bruceb)	Initial implementation.
*/
VOID
IIFTsfaScrollFldAddstr(win, width, string)
WINDOW	*win;
i4	width;
char	*string;
{
	win->_cury = 0;
	win->_curx = 0;
	_VOID_ TDxaddstr(win, width, string);
}

/*{
** Name:	IIFTsdbScrollDebug	- Print debugging information for
**					  scrolling fields.
**
** Description:
**	Print out debugging information for a scrolling field.
**	Information includes visible contents of the window,
**	_cury and _curx, _maxy and _maxx, contents of under_buf,
**	scr_start and cur_pos, and whether the field is reversed.
**	Information only printed if IIFTdsiDumpScrollInfo is set
**	to TRUE; this can be set in a debugger at runtime.
**
** Inputs:
**	win		Window for the field.
**	scroll		Scroll struct for currently active field.
**	reverse		Indicate direction in which characters are read.
**
** Outputs:
**
**	Returns:
**		VOID
**
**	Exceptions:
**		None
**
** Side Effects:
**	Prints debugging information on to the screen.
**
** History:
**	06-nov-87 (bruceb)	Initial implementation.
*/
# ifdef	scrlDEBUG
VOID
IIFTsdbScrollDebug(win, scroll, reverse)
WINDOW		*win;
SCROLL_DATA	*scroll;
bool		reverse;
{
	i4	y;

	if (IIFTdsi)
	{
		SIprintf(ERx("Scroll Window\n"));
		for (y = 0; y < win->_maxy; y++)
		{
			SIprintf(ERx("%*s\n"), win->_maxx, win->_y[y]);
		}
		SIprintf(ERx("cury, curx = (%d, %d), maxy, maxx = (%d, %d)\n"),
			win->_cury, win->_curx, win->_maxy, win->_maxx);
		
		SIprintf(ERx("Under_buf: width = %d\n"),
			(scroll->right - scroll->left + 1));
		SIprintf(ERx("%*s\n"), (scroll->right - scroll->left + 1),
			scroll->left);
		SIprintf(ERx("scr_start = %d, cur_pos = %d, reverse = %c\n"),
			(scroll->scr_start - scroll->left),
			(scroll->cur_pos - scroll->left),
			(reverse ? 'T' : 'F'));
	}
}
# endif
