/*
**  FTuserinp.c
**
**  Copyright (c) 2004 Ingres Corporation
**
**  History
**	    01/27/86 (garyb) Change name of routine from FTdumpsp to
**		    FTdmpsp due to namespace problems.
**	12/23/86 (dkh) - Added support for new activations.
**
**	10/20/86 (KY)  -- Changed CH.h to CM.h.
**	06/25/86 (a.dea) -- Change (void) to _VOID_ for IBM/CMS.
**	10/01/86 (a.dea) -- #ifdef KFE around KFcountout calls.
**	06/19/87 (dkh) - Code cleanup.
**	24-sep-87 (bruceb)
**		Added calls on FTshell for fdopSHELL key.
**	29-sep-87 (bruceb)
**		Additional code to improve screen appearance after
**		use of shell key.
**	30-sep-87 (bruceb)
**		TEflush after use of shell key for VMS.
**	22-jan-88 (bruceb)
**		Allow completion on \n as well as on \r.
**	09-nov-88 (bruceb)
**		Set number of seconds until timeout.
**		If timeout then occurs while in FTuserinp(), set event to
**		fdopTMOUT and return an empty buffer.
**	20-dec-88 (brett)
**		Added windex library calls, these are used in a special
**		windowing emulator for mouse activation on ingres forms.
**	31-jan-89 (brett)
**		Correct the actions sent to the windex interface used
**		in screen mouse selection.  This had to do with when
**		mouse selection from the screen and menu line was allowed.
**	3-mar-89 (Mike S)
**		Clear graphics for fdopSHEL
**	07/12/89 (dkh) - Added support for emerald internal hooks.
**	07/22/89 (dkh) - Fixed bug 6732.
**	09/02/89 (dkh) - Put in support for per display loop keystroke act.
**	09/22/89 (dkh) - Porting changes integration.
**	09/28/89 (dkh) - Added on_menu parameter on call to intercept routine.
**	10/19/89 (dkh) - Changed code to eliminate duplicate FT files
**			 in GT directory.
**	12/27/89 (dkh) - Added support for hot spot trim.
**	01-feb-90 (bruceb)
**		Moved TEflush's into FTbell.
**	02/23/90 (dkh) - Fixed bug 9961.
**	16-mar-90 (bruceb)
**		Support added for locators.  Call FKmapkey with function
**		pointer that determines effect of a locator event.  Function
**		is different for menus and for prompts (for which no locator
**		event is legal.)
**	04/04/90 (dkh) - Integrated MACWS changes.
**	18-Apr-90 (brett)
**	    Move IITDfalsestop() from FTusrinp to FTshell.
**	30-apr-90 (bruceb)
**		Made clicking on prompts legal; equivalent to using MENU key.
**	08/15/90 (dkh) - Fixed bug 21670.
**	08/29/90 (dkh) - Integrated porting change ingres6202p/131520.
**	09/04/90 (dkh) - Removed KFE stuff which has been superseded by SEP.
**	10/14/90 (dkh) - Integrated round 4 of macws changes.
**	05-jun-92 (leighb) DeskTop Porting Change:
**		Changed COORD to IICOORD to avoid conflict
**		with a structure in wincon.h for Windows/NT
**      23-sep-96 (mcgem01)
**              Global data moved to ftdata.c
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	26-Aug-2009 (kschendel) 121804
**	    Need termdr.h to satisfy gcc 4.3.
*/

# include	<compat.h>
# include	<st.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	"ftframe.h"
# include	<kst.h>
# include	<ctrl.h>
# include	<cm.h>
# include	<si.h>
# include	<te.h>
# include	<ft.h>
# include	<frscnsts.h>
# include	<frsctblk.h>
# include	<er.h>
# include	<termdr.h>

FUNC_EXTERN	bool		FTgotmenu();
FUNC_EXTERN	bool		FTgotfrsk();
FUNC_EXTERN	KEYSTRUCT	*FTTDgetch();
FUNC_EXTERN	VOID		FTprnscr();
FUNC_EXTERN	VOID		KFcountout();
FUNC_EXTERN	bool		IIFTtoTimeOut();
FUNC_EXTERN	VOID		IIFTstsSetTmoutSecs();
FUNC_EXTERN	VOID		IIFTmldMenuLocDecode();
FUNC_EXTERN	VOID		IITDpciPrtCoordInfo();
FUNC_EXTERN	i4		IITDlioLocInObj();
FUNC_EXTERN	VOID		IITDposPrtObjSelected();

VOID		IIFTpldPromptLocDecode();
VOID		IIFTsplSetPromptLoc();

GLOBALREF	FILE	*FTdumpfile;

static	WINDOW	*promptwin = NULL;
static	IICOORD	*promptloc = NULL;


/*
**  Fix for BUG 7547. (dkh)
*/
GLOBALREF	bool	fdrecover;
GLOBALREF	bool	Rcv_disabled;
GLOBALREF	FRS_GLCB	*IIFTglcb;

/*
**  Special routine for vifred testing. (dkh)
**
**  History
**	    01/27/86 (garyb) Change name of routine from FTdumpsp to
**		    FTdmpsp due to namespace problems.
**
*/

FTdmpsp(fptr)
FILE	*fptr;
{
	FTdumpfile = fptr;
}


/*
**  Reset the cursor to the correct spot in the input window since
**  FKmapkey sets the cursor to the beginning of the menu line
**  when a key that is mapped to a menuitem or frskey is selected.
*/

FTinpreset(win, echo)
WINDOW	*win;
i4	echo;
{
	i4	ox;

	if (!echo)
	{
		ox = win->_curx;
		win->_curx = 0;
	}
	TDrefresh(win);
	if (!echo)
	{
		win->_curx = ox;
	}
}


/*{
** Name:	IIFTofOnlyFrskeys - Only accept frskey input.
**
** Description:
**	This routine just accepts and ignores input (via a terminal
**	bell) except for frskeys, the menu key or the return key.
**	The reason for all this work is because a user may have
**	no real menuitem activations, but can have frskey activations.
**	Yes, this is a strange case, but that's what the user wants
**	and it is somewhat reasonable.
**
**	Note timeout is also handled properly here.
**
** Inputs:
**	win	Window to position the cursor on.
**
** Outputs:
**	evcb.event	Event (i.e., frskey/menu/return key) that occurred.
**	evcb.mf_num	The specific frskey that was pressed.
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
**	02/22/90 (dkh) - Initial version.
*/
VOID
IIFTofOnlyFrskeys(win, evcb)
WINDOW		*win;
FRS_EVCB	*evcb;
{
	KEYSTRUCT	*ks;
	i4		code;
	i4		flg;
	bool		activate = FALSE;
	u_char		dummy;

	/*
	**  Move cursor to menu line.
	*/
	TDrefresh(win);

	IIFTstsSetTmoutSecs(evcb);

	for(;;)				/* for ever (until RETURN) */
	{
# ifdef DATAVIEW
		if (IIMWscmSetCurMode((i4 *) NULL, fdcmNULL,
			FALSE, FALSE, FALSE) == FAIL)
		{
			return;
		}
# endif	/* DATAVIEW */

		ks = FTTDgetch(win);	/* get a character from the window */

# ifdef DATAVIEW
		/*
		**  If there is an error while getting the
		**  user response, ks would be returned as
		**  NULL.  If this happens, we just return
		**  to higher level, so that we don't end up
		**  referencing through a NULL ptr.
		*/
		if (IIMWimIsMws() && (ks == (KEYSTRUCT *) NULL))
			return;
# endif	/* DATAVIEW */

		if (IIFTtoTimeOut(evcb))	/* if timeout has occurred */
		{
			/* event set to fdopTMOUT */
			return;
		}

		if ((ks->ks_ch[0] == '\r') || (ks->ks_ch[0] == '\n'))
		{
			/*
			**  Fake out the system be pretending the
			**  return and new line keys do the right thing.
			**  No matter what those control keys may be set to.
			*/
			code = fdopRET;
			evcb->event = fdNOCODE;
			return;
		}
		else
		{
			_VOID_ FKmapkey(ks, NULL, &code, &flg, &activate,
				&dummy, evcb);
		}

		if (code == fdopMENU)
		{
			return;
		}
		else if (evcb->event == fdopFRSK)
		{
			return;
		}
# ifdef	DATAVIEW
		else if (IIMWimIsMws())
		{
			return;
		}
# endif	/* DATAVIEW */
		else
		{
			FTbell();
		}
	}
}


/*
**  FTuserinp
**
**  User input routine.  Get a character from the user for a menu/prompt.
**  Please note that a menu item may only be selected from a
**  control/function key if we accepting input for the menu system
**  and if the input area is currently empty.
**  No validation check is done if a menu item is
**  selected through a control/function key here since
**  this will be done at a higher level.
**
**  9/21/84 -
**    Stolen from IImenuinp. (dkh)
**  5/8/85 -
**    Added support for long menus. (dkh)
**  25-nov-86 (bruceb)	Fix for bug 10891
**    Don't check for char > MAX_8B_ASCII until after checking for
**    specific operations (such as fdopRUB, fdopCLRF).
*/

u_char *
FTuserinp(win, promptstr, type, echo, evcb)		/* MENUINP: */
WINDOW	*win;				/* Menu to pass to */
char	*promptstr;
i4	type;
i4	echo;
FRS_EVCB *evcb;
{
	static u_char  buf[MAX_TERM_SIZE+1];/* static buffer to pass as input */
	register u_char	*cp = buf;	/* fast pointer to buffer */
	i4		promptlen;	/* length of prompt */
	KEYSTRUCT	*ks;
	i4		code;
	i4		flg;
	bool		activate = FALSE;
	u_char		dummy;
	i4		(*kyint)();
	i4		keytype;
	i4		keyaction;
	u_i2		svflag;
	VOID		(*locdecode)();
	i4		event;
	IICOORD		ploc;
# ifdef DATAVIEW
	bool		do_here = TRUE;
# endif	/* DATAVIEW */

	if ((FTgtflg & FTGTREF) != 0)
	{
		(*iigtreffunc)();
		if ((FTgtflg & FTGTEDIT) != 0)
		{
			(*iigtslimfunc)(-1);
			(*iigtsupdfunc)(TRUE);
		}
		FTgtflg &= ~FTGTREF;
	}

	/*
	**  Only do key intercept for menu input.
	*/
	if (type == FT_MENUIN && IIFTglcb->intr_frskey0)
	{
		kyint = IIFTglcb->key_intcp;
	}
	else
	{
		kyint = NULL;
	}

	IIFTstsSetTmoutSecs(evcb);

# ifdef DATAVIEW
	if ((IIMWimIsMws()) && (type == FT_PROMPTIN))
	{
		_VOID_ IIMWguiGetUsrInp(promptstr,
			echo, evcb, (char *) buf);
		return(buf);
	}
# endif	/* DATAVIEW */

	if (win == NULL)
	{
		win = stdmsg;
	}

	/*
	**  Determine the type of locator event processing to perform.
	*/
	if (type == FT_MENUIN)
	{
	    locdecode = IIFTmldMenuLocDecode;
	}
	else
	{
	    locdecode = IIFTpldPromptLocDecode;

	    ploc.begy = promptwin->_begy;
	    ploc.begx = promptwin->_begx;
	    ploc.endy = promptwin->_begy + promptwin->_maxy - 1;
	    ploc.endx = promptwin->_begx + promptwin->_maxx - 1;
	    promptloc = &ploc;
	    IITDpciPrtCoordInfo(ERx("FTuserinp"), ERx("ploc"), promptloc);
	}

	buf[0] = '\0';
	promptlen = STlength(promptstr);	/* set length of prompt */
	TDerase(win);			/* erase the previous contents */
	TDaddstr(win, promptstr);	/* add the prompt */

	TDrefresh(win);			/* refresh the window */
	for(;;)				/* for ever (until RETURN) */
	{
# ifdef DATAVIEW
		if (IIMWimIsMws() && do_here)
		{
			if (IIMWscmSetCurMode((i4 *) NULL, fdcmNULL,
				(kyint != NULL), FALSE, FALSE) == FAIL)
			{
				return(NULL);
			}
		}
# endif	/* DATAVIEW */

		ks = FTTDgetch(win);	/* get a character from the window */

# ifdef DATAVIEW
		/*
		**  If there is an error while getting the
		**  user response, ks would be returned as
		**  NULL.  If this happens, we just return
		**  to higher level, so that we don't end up
		**  referencing through a NULL ptr.
		*/
		if (IIMWimIsMws() && (ks == (KEYSTRUCT *) NULL))
		{
			return(NULL);
		}
# endif	/* DATAVIEW */

		if (IIFTtoTimeOut(evcb))	/* if timeout has occurred */
		{
		    /* event set to fdopTMOUT */
		    buf[0] = EOS;
		    goto done;
		}

		_VOID_ FKmapkey(ks, locdecode, &code, &flg, &activate,
		    &dummy, evcb);

		if (kyint)
		{
			event = evcb->event;

			if (ks->ks_fk != 0)
			{
				if (event == fdopHSPOT)
				{
					keytype = evcb->mf_num;
				}
				else
				{
					keytype = FUNCTION_KEY;
					if (code == fdopMENU)
					{
					    /*
					    ** Special casing for locators.
					    ** Not changing evcb->event, just
					    ** that value using by (*kyint)().
					    */
					    event = fdopMENU;
					}
				}
			}
			else if (CMcntrl(ks->ks_ch))
			{
				keytype = CONTROL_KEY;
			}
			else
			{
				keytype = NORM_KEY;
			}

			/*
			**  Passing TRUE for on_menu parameter since
			**  keystroke interception is only enabled
			**  for menu input.
			*/
			keyaction = (*kyint)(event, keytype, ks->ks_ch,
				(bool) TRUE);
			if (keyaction == FRS_IGNORE)
			{
# ifdef DATAVIEW
				if (IIMWdlkDoLastKey(TRUE, &do_here) == FAIL)
					return(NULL);
# endif	/* DATAVIEW */
				continue;
			}
			else if (keyaction == FRS_RETURN)
			{
				evcb->event = fdopFRSK;
				evcb->val_event = FRS_NO;
				evcb->act_event = FRS_NO;
				evcb->intr_val = IIFTglcb->intr_frskey0;
				buf[0] = EOS;
				return(buf);
			}

			/*
			**  Otherwise, drop through for normal processing.
			*/
		}

# ifdef DATAVIEW
		if (IIMWdlkDoLastKey(FALSE, &do_here) == FAIL)
			return(NULL);
		if ( ! do_here)
			continue;
# endif	/* DATAVIEW */

		if ((type == FT_MENUIN)
		    && (ks->ks_ch[0] == '<' || ks->ks_ch[0] == '>')
		    && (cp == buf))
		{
			/*
			**  Fix for BUG 7547. (dkh)
			*/
			if (!KY && fdrecover)
			{
				Rcv_disabled = FALSE;
				TDrcvstr(ks->ks_ch);
				Rcv_disabled = TRUE;
			}
			*cp = (u_char) ks->ks_ch[0];
			*++cp = '\0';
			goto done;
		}
		if ((ks->ks_ch[0] == '\r') || (ks->ks_ch[0] == '\n'))
		{
			/*
			**  Fake out the system be pretending the
			**  return and new line keys do the right thing.
			**  No matter what those control keys may be set to.
			*/
			code = fdopRET;
			evcb->event = fdNOCODE;
		}
		if (code == fdopMENU)
		{
			if (cp == buf)
			{
				if (type == FT_MENUIN)
				{
					/*
					**  Menu Key pressed with no
					**  characters in input window.
					**  So when scroll menu leftward.
					*/
					if (evcb->event == fdopMENU)
					{
						/*
						**  Fix for BUG 7547. (dkh)
						*/
						if (!KY && fdrecover)
						{
							Rcv_disabled = FALSE;
							TDrcvstr(ks->ks_ch);
							Rcv_disabled = TRUE;
						}
						*cp++ = '>';
					}
					else if (evcb->event == fdopSCRLT)
					{
					    *cp++ = '>';
					}
					else if (evcb->event == fdopSCRRT)
					{
					    *cp++ = '<';
					}
					/*
					** ':', fdopRET, simply returns to
					** FTgetmenu().
					*/

					/*
					**  A control/pf key assigned to a
					**  menu position was pressed with
					**  no chars in input area.  So
					**  we go back to FTgetmenu and
					**  that takes care of things for
					**  us.  We need to put char info
					**  out now since  FTgetmenu will
					**  have no idea what char was
					**  pressed.  Fix for BUG 8410. (dkh)
					*/
					if (!KY && fdrecover)
					{
						Rcv_disabled = FALSE;
						if (!ks->ks_fk)
						{
							TDrcvstr(ks->ks_ch);
						}
						Rcv_disabled = TRUE;
					}
					*cp = '\0';
					goto done;
				}
				else
				{
					if (evcb->event == fdopMUITEM)
					{
						FTinpreset(win, echo);
						FTbell();
						continue;
					}
					*cp = '\0';
					goto done;
				}
			}
			else
			{
				if (evcb->event == fdopMUITEM)
				{
					FTinpreset(win, echo);
					FTbell();
					continue;
				}
				else if ((evcb->event == fdopSCRLT)
				    || (evcb->event == fdopSCRRT)
				    || (evcb->event == fdopRET))
				{
				    /*
				    ** '<', '>' and ':' are illegal when the
				    ** user has already entered text.
				    */
				    FTbell();
				    continue;
				}
				*cp = '\0';
				goto done;
			}
		}
		else if (evcb->event == fdopFRSK)
		{

			/*
			**  Got a frskey activate.  Only valid
			**  when accepting menu input and
			**  no characters in input area.
			*/
			if (type == FT_MENUIN && cp == buf)
			{
				buf[0] = '\0';
				goto done;
			}
			else
			{
				FTinpreset(win, echo);
				FTbell();
				continue;
			}
		}
		if (ks->ks_fk != 0)
		{
			if (code == fdopSHELL)
			{
				if ((FTgtflg & FTGTACT) != 0)
					(*iigtclrfunc)();
				FTshell();
				if ((FTgtflg & FTGTACT) != 0)
				{
					/*
					** refresh with EDIT bit off, so that
					** we don't try to menu refresh.
					** FTgtrefmu screws up bottom line.
					*/
					svflag = FTgtflg;
					FTgtflg &= ~FTGTEDIT;
					(*iigtreffunc)();
					if ((svflag & FTGTEDIT) != 0)
					{
						(*iigtslimfunc)(-1);
						(*iigtsupdfunc)(TRUE);
					}
					TEflush();
					FTgtflg = svflag;
				}
			}
			else if (type != FT_MENUIN)
			{
				FTbell();
				continue;
			}
			else
			{
				if (evcb->event == fdNOCODE)
				{
					/*
					**  If PF key was not mapped
					**  set up for ringing bell
					**  below.
					*/
					code = fdNOCODE;
				}
			}
		}
		else
		{
			CMcpychar(ks->ks_ch, cp);
		}
		switch (code)
		{
			case fdopNULL:
			case fdopHSPOT:
				break;

			case fdNOCODE:
				/*
				**  PF/contol key was not mapped
				**  to anything.
				*/
				FTbell();
				break;

			case fdopCLRF:	/* Erase the line */
				TDerase(win);
				TDaddstr(win, promptstr);
				cp = buf;
				break;

			case fdopREFR:
				/*
				** refresh with EDIT bit off, so that
				** we don't try to menu refresh.
				** FTgtrefmu screws it up.
				*/
				svflag = FTgtflg;
				FTgtflg &= ~FTGTEDIT;
				if ((FTgtflg & FTGTACT) != 0)
					(*iigtclrfunc)();
				TDrefresh(curscr);
				/* if graphics on frame, refresh graphics */
				if ((FTgtflg & FTGTACT) != 0)
				{
					(*iigtreffunc)();
					if ((svflag & FTGTEDIT) != 0)
					{
						(*iigtslimfunc)(-1);
						(*iigtsupdfunc)(TRUE);
					}
				}
				FTgtflg = svflag;
				break;

			/*
			**  Fix for BUG 7589. (dkh)
			*/
			case fdopPRSCR:
			{
				char	msgline[MAX_TERM_SIZE + 1];

				/*
				**  Fix for BUG 8407. (dkh)
				*/
				if (!KY && fdrecover)
				{
					Rcv_disabled = FALSE;
					TDrcvstr(ks->ks_ch);
				}

				TDsaveLast(msgline);
				FTprnscr(NULL);
				TDwinmsg(msgline);
				TDrefresh(win);

				/*
				**  Fix for BUG 8407. (dkh)
				*/
				if (!KY && fdrecover)
				{
					Rcv_disabled = TRUE;
				}
				break;
			}

			case fdopRUB:	/* delete a character */
				if (cp <= buf)
				{
					break;
				}
				CMprev(cp, buf);
				if (!echo)
				{
					if (win->_curx > 0)
					{
						CMbytedec(win->_curx, cp);
					}
					break;
				}
				if (win->_curx <= promptlen)
				{
					break;
				}
				TDmvleft(win, 1);
				TDdelch(win);
                		/*
				** brett, Enable menus windex command string,
				** If no input yet or user has deleted all
				** input, then menu line selection is ok.
				*/
                		if (cp == buf)
                        		IITDenmenus();
				break;

			case fdopRET:
				*cp = '\0';
				if (FTdumpfile)
				{
					TDdumpwin(curscr, FTdumpfile, TRUE);
				}
				goto done;

			case fdopSHELL:
				if ((FTgtflg & FTGTACT) != 0)
					(*iigtclrfunc)();
				FTshell();
				if ((FTgtflg & FTGTACT) != 0)
				{
					/*
					** refresh with EDIT bit off, so that
					** we don't try to menu refresh.
					** FTgtrefmu screws up bottom line.
					*/
					svflag = FTgtflg;
					FTgtflg &= ~FTGTEDIT;
					(*iigtreffunc)();
					if ((svflag & FTGTEDIT) != 0)
					{
						(*iigtslimfunc)(-1);
						(*iigtsupdfunc)(TRUE);
					}
					TEflush();
					FTgtflg = svflag;
				}
				break;

			default:	/* other characters add to string */
                                /*
                                ** brett, Disable menus windex command string,
                                ** If the user has types on the menu line then
				** disable menu mouse selection.
                                */
                                IITDdsmenus();

				if (CMcntrl(cp) || ks->ks_fk ||
				    win->_curx >= win->_maxx - CMbytecnt(cp))
				{
					FTbell();
					break;
				}

				/*
				**  Since we allow no control chars, we
				**  will use win->_curx to still keep
				**  track of where we are in window,
				**  even for noecho prompts.
				*/

				if (echo)
				{
					TDaddstr(win, CMunctrl(cp));
				}
				else
				{
					CMbyteinc(win->_curx, cp);
				}
				CMnext(cp);
				break;
		}
		if (echo)
		{
			TDrefresh(win);	/* refresh the window after changes */
		}
	}

done:

	return(buf);
}


/*{
** Name:	IIFTpldPromptLocDecode     - Determine effect of locator click.
**
** Description:
**	Determine whether or not the user clicked on a legal location.
**	Only legal location is the prompt line (if single line prompt)
**	or the prompt popup.
**
** Inputs:
**	key	User input.
**
** Outputs:
**	evcb
**	    .event	fdNOCODE if illegal location; fdopMENU otherwise.
**
**	Returns:
**		None.
**
**	Exceptions:
**
** Side Effects:
**
** History:
**	30-apr-90 (bruceb)	Written.
*/
VOID
IIFTpldPromptLocDecode(key, evcb)
KEYSTRUCT	*key;
FRS_EVCB	*evcb;
{
    if (IITDlioLocInObj(promptloc, key->ks_p1, key->ks_p2))
    {
	IITDposPrtObjSelected(ERx("ploc"));
	evcb->event = fdopMENU;
    }
    else
    {
	evcb->event = fdNOCODE;
    }
}


/*{
** Name:	IIFTsplSetPromptLoc     - Save location for the prompt.
**
** Description:
**	Point to the prompt window indicated for use in determining if a
**	locator click is legal.
**
** Inputs:
**	win	Prompt window.
**
** Outputs:
**
**	Returns:
**		None.
**
**	Exceptions:
**
** Side Effects:
**
** History:
**	30-apr-90 (bruceb)	Written.
*/
VOID
IIFTsplSetPromptLoc(win)
WINDOW	*win;
{
    promptwin = win;
}
