/*
**	Copyright (c) 2004 Ingres Corporation
*/
# include	<compat.h>
# include	<cv.h>
# include	<st.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	"ftframe.h"
# include	<afe.h>
# include	<frscnsts.h>
# include	<frsctblk.h>
# include	"ftfuncs.h"
# include	<cm.h>
# include	<te.h>
# include	<er.h>
# include	<erft.h>
# include	<scroll.h>
# include	<ug.h>
# include	<termdr.h>

/**
** Name:    ftbrowse.c  -  The BROWSE Mode of the Frame Driver
**
**	This routine constitutes the BROWSE mode of the frame driver.  All
**	commands accepted by the BROWSE mode are processed here.  Those
**	commands that are legal for the BROWSE mode are processed here.
**	If the command is not a BROWSE command but legal in BROWSE mode, it
**	is returned to the upper level program.
**
**	The BROWSE mode only allows the user to move from field to field and
**	move the cursor. If inside a table field it returns when switching
**	columns as may have exited a readonly column.
**
**	NOTE - Not using IIUGmsg in this file since it does not
**	restore the message line.
**
**	Arguments: frm	-  frame to drive
**
**	History: JEN - 27 Aug 1981  (written)
**		 NCG - 27 Jul 1983  added column specific moves.
**		 NML - 20 Oct 1983  Increased size of buffer for text -
**				    use DB_MAXSTRING for max. input value as
**				    defined in ingconst.h
**		NCG  - 16 May 1984  added  query only fields.
**	10/20/86 (KY)  -- Changed CH.h to CM.h.
**	12/08/86 (KY)  -- Changed FTTDgetch return KEYSTRUCT structure.
**	12/24/86 (dkh) - Added support for new activations.
**	03/06/87 (dkh) - Added support for ADTs.
**	16-mar-87 (bruceb)
**		place cursor at the proper 'beginning' of a window
**		depending on whether the field is right-to-left or
**		left-to-right.	also, added fdopSPACE, acting like
**		fdopLEFT or fdopRIGHT depending on the RL/LR status
**		of the field.  Used in Hebrew project.
**	05/02/87 (dkh) - Changed to call FD routines via passed pointers.
**	06/19/87 (dkh) - Code cleanup.
**	08/14/87 (dkh) - ER changes.
**	24-sep-87 (bruceb)
**		Added call on FTshell for fdopSHELL key.
**	16-nov-87 (bruceb)
**		Added code to handle scrolling fields.  This includes the
**		FTbfldattr routine, akin to the FTfldattr routine in
**		insrt.c
**	12/14/87 (dkh) - Added message on using IIUGmsg.
**	09-feb-88 (bruceb)
**		Scrolling flag now sits in fhd2flags; can't use most
**		significant bit of fhdflags.
**	02/25/88 (dkh) - Fixed jup bug 1853.
**	02/27/88 (dkh) - Added support for nextitem command.
**	05/11/88 (dkh) - Added hooks for floating popups.
**	05/20/88 (dkh) - Added support for inquiring on current position.
**	11-nov-88 (bruceb)
**		Added support for timeout; return fdopTMOUT.
**		Decreased numeric buffer size from 4K to 100 bytes.
**	2-mar-89 (Mike S)
**		Added conditional graphics code for SHELL case.
**	07/13/89 (dkh) - Added support for emerald internal hooks.
**	09/01/89 (dkh) - Put in support for per display loop keystroke act.
**	09/28/89 (dkh) - Added on_menu parameter on call to intercept routine.
**	10/19/89 (dkh) - Changed code to eliminate duplicate FT files
**			 in GT directory.
**	11/24/89 (dkh) - Added support for goto's from wview.
**	12/27/89 (dkh) - Added support for hot spot trim.
**	01-feb-90 (bruceb)
**		Added support for Single Character Find.  Removed ability
**		to specify number of repetitions.  (e.g. 4<tab>).
**		FTbfldattr() now returns pointer to the table field, not
**		a boolean indicating that field is a TF.  Also returns
**		boolean indicating whether or not a char column.
**		Moved TEflush's into FTbell.
**	16-mar-90 (bruceb)
**		Added support for locator events.  FKmapkey() is now called
**		with a function pointer used to determine what affect a locator
**		event should have.
**	04/03/90 (dkh) - Integrated MACWS changes.
**	12-apr-90 (bruceb)
**		Removed fdopSKPFLD code since this is basically obsolete.
**	04/14/90 (dkh) - Changed "# if defined" to # ifdef".
**	24-apr-90 (bruceb)
**		Added support for 'click-on-row to select' for listpicks.
**		This will not work for WindowView since that product requires
**		TF positioning for scrolls.
**	08/15/90 (dkh) - Fixed bug 21670.
**	08/29/90 (dkh) - Integrated porting change ingres6202p/131520.
**	10/14/90 (dkh) - Integrated set 4 of macws changes.
**	08-jun-92 (leighb) DeskTop Porting Change:
**		if NO1CLKACT defined, don't Activate on Single Mouse Clicks;
**		this is for double click support in the CL.
**	07/01/92 (dkh) - Added missing parameter on call to FTmove().
**	02-may-97 (cohmi01)
**	    For MWS - Add fdopSCRTO case for scroll bar button movement.
**	    If a 'loadtable' has been done, refresh Scroll Bar (bug 81574).
**	19-dec-97 (kitch01)
**		Bug 87110. If the form contains more than 1 tablefiled then a scroll
**		action from Upfront via MWS can scroll the wrong tablefield. This is 
**		because IIcurtbl has not been updated to reflect the tablefield with
**		focus before the call to IItscroll. I amended the code to go back to 
**		IItputfrm so that this action can be handled there.
**	23-dec-97 (kitch01)
**		Bugs 87709, 87808, 87812, 87841.
**		All related to scrolling tablfields in Upfront. I have redesigned the
**		code so that a refreshscrollbar message is sent to Upfront providing
**		this is the first time the form has been displayed. This ensures that
**		all tablefields have the scrollbar correctly positioned initially. Any
**		subsequent scroll of the tablefield will have the message sent by the 
**		scrolling code.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	26-Aug-2009 (kschendel) 121804
**	    Need termdr.h to satisfy gcc 4.3.
*/

# define	NUM_BUFLEN	100

FUNC_EXTERN	i4		FTmove();
FUNC_EXTERN	KEYSTRUCT	*FTTDgetch();
FUNC_EXTERN	i4		TDrefresh();
FUNC_EXTERN	VOID		FTprnscr();
FUNC_EXTERN	VOID		FTword();
FUNC_EXTERN	VOID		FTbotsav();
FUNC_EXTERN	VOID		FTbotrst();
FUNC_EXTERN	VOID		IIFTsiiSetItemInfo();
FUNC_EXTERN	VOID		IIFTsciSetCursorInfo();
FUNC_EXTERN	bool		IIFTtoTimeOut();
FUNC_EXTERN	VOID		IIFTfldFrmLocDecode();
FUNC_EXTERN	VOID		IIFTsmShiftMenu();

GLOBALREF	i4		FTfldno;
GLOBALREF	bool		IITDlsLocSupport;

static		i4		ftoldcol();		/* save old column */
static		i4		ftchgcol();		/* has column changed */
static		SCROLL_DATA	*scroll_fld = NULL;

/*{
** Name:	FTbfldattr	- Determine fld specific attributes
**
** Description:
**	Determine whether a field is reversed (RL) or scrollable.
**
** Inputs:
**	frm	Currently active frame.
**
** Outputs:
**	reverse	Indicate if current field is left-to-right or RL.
**	tblfld	Indicate if current field is a table field.
**		(Sets tblfld to NULL or points to the tblfld structure.)
**	charcol	Indicate if a column of character datatype.
**
**	Returns:
**		VOID
**
**	Exceptions:
**		None
**
** Side Effects:
**	scroll_fld is also set to point to a field's scroll structure
**	if the field is scrollable.
**
** History:
**	09-nov-87 (bruceb)	Initial implementation.
*/
static VOID
FTbfldattr(frm, reverse, tblfld, charcol)
FRAME	*frm;
bool	*reverse;
TBLFLD	**tblfld;
bool	*charcol;
{
	FIELD	*fld;
	FLDHDR	*hdr;
	FLDCOL	*col;
	i4	class;

	fld = frm->frfld[FTfldno];
	hdr = (*FTgethdr)(fld);

	if (hdr->fhdflags & fdREVDIR)
		*reverse = TRUE;
	else
		*reverse = FALSE;

	if (hdr->fhd2flags & fdSCRLFD)
		scroll_fld = IIFTgssGetScrollStruct(fld);
	else
		scroll_fld = NULL;

# ifdef DATAVIEW
	/*
	**  All the field scrolling is done by MWS, so we want
	**  to disable field scrolling in FT.
	*/
	if (IIMWimIsMws())
		scroll_fld = NULL;
# endif	/* DATAVIEW */

	if (fld->fltag == FTABLE)
	{
		*tblfld = fld->fld_var.fltblfld;
		col = (*tblfld)->tfflds[(*tblfld)->tfcurcol];
		IIAFddcDetermineDatatypeClass(col->fltype.ftdatatype, &class);
		if (class == CHAR_DTYPE)
		    *charcol = TRUE;
		else
		    *charcol = FALSE;
	}
	else
	{
		*tblfld = NULL;
		*charcol = FALSE;
	}
}

i4
FTbrowse(frm, mode, frscb)
FRAME	*frm;
i4	mode;
FRS_CB	*frscb;
{
	WINDOW			*win;
	reg KEYSTRUCT		*ks;
	i4			code;
	i4			flg;
	i4			repts;
	u_char			dummy;
	i4			mvcode;
	i4			ofldno;
	i4			ocolno; /* old col number if using tblfld */
	WINDOW			*FTgetwin();
	bool			prerrmsg = TRUE;
	bool			activate = FALSE;
	bool			reverse = FALSE;
	bool			charcol;
	TBLFLD			*tblfld;
	FRS_EVCB		*evcb;
	i4			(*kyint)();
	i4			keytype;
	i4			keyaction;
	STATUS			retval;
	bool			keyfind;
	i4			event;
# ifdef DATAVIEW
	bool			do_here = TRUE;
# endif	/* DATAVIEW */

	FTfldno = frm->frcurfld;
	ofldno = FTfldno;
	ocolno = ftoldcol(frm, FTfldno);		/* save old column */
	if ((win = FTgetwin(frm, &FTfldno)) == NULL)
		return (fdopMENU);

	evcb = frscb->frs_event;

	IIFTsiiSetItemInfo(win->_begx + FTwin->_startx + 1,
		win->_begy + FTwin->_starty + 1, win->_maxx, win->_maxy);

	FTbfldattr(frm, &reverse, &tblfld, &charcol);
	/* move cursor to the beginning of the window */
	if (reverse)
	{
		TDmove(win, (i4)0, win->_maxx - 1);
	}
	else
	{
		TDmove(win, (i4)0, (i4)0);
	}
	TDrefresh(win);

	if (frscb->frs_globs->intr_frskey0)
	{
		kyint = frscb->frs_globs->key_intcp;
	}
	else
	{
		kyint = NULL;
	}

	if (frscb->frs_globs->enabled & KEYFIND)
	{
		keyfind = TRUE;
	}
	else
	{
		keyfind = FALSE;
	}

	(*FTdmpmsg)(ERx("BROWSE loop:\n"));
	(*FTdmpcur)();

	repts = 1;
	for(;;)
	{
		IIFTsciSetCursorInfo(evcb);

# ifdef DATAVIEW
		if (IIMWimIsMws() && do_here)
		{
			/* Bugs 87709, 87808, 87812, 87841. We now only send the
			** refreshscrollbar message if this is the first time the
			** form has been displayed.
			*/
			if (IIMWisrInitScrollbarReqd(frm))
			{
				IItscroll_init();
				IIMWsiScrollbarInitialized(frm);
			}

			if ((IIMWscfSetCurFld(frm, FTfldno) == FAIL) ||
				(IIMWscmSetCurMode(FTcurdisp(), fdcmBRWS,
					kyint != NULL, keyfind,
					(tblfld != NULL) && charcol) == FAIL))
			{
				return(fdNOCODE);
			}
		}
# endif	/* DATAVIEW */

		ks = FTTDgetch(win);

# ifdef DATAVIEW
		/*
		**  If there is an error while getting the
		**  user response, ks would be returned as
		**  NULL.  If this happens, we just return
		**  to higher level, so that we don't end up
		**  referencing through a NULL ptr.
		*/
		if (IIMWimIsMws() && (ks == (KEYSTRUCT *) NULL))
			return(fdNOCODE);
# endif	/* DATAVIEW */

		if (IIFTtoTimeOut(evcb))
			return(fdopTMOUT);
		/*
		**  Please note that there will be no validation
		**  done in BROWSE mode.
		*/
		retval = FKmapkey(ks, IIFTfldFrmLocDecode, &code, &flg,
			&activate, &dummy, evcb);

		/*
		** Determine keystroke type.  Useful both for keystroke
		** intercept and for single keystroke find.
		*/
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
			    ** the value used by (*kyint)().
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
		**  If keystroke intercept is active, return keystroke
		**  info.
		*/
		if (kyint)
		{
			keyaction = (*kyint)(event, keytype, ks->ks_ch,
				(bool) FALSE);
			if (keyaction == FRS_IGNORE)
			{
# ifdef DATAVIEW
				if (IIMWdlkDoLastKey(TRUE, &do_here) == FAIL)
					return(fdNOCODE);
# endif	/* DATAVIEW */
				continue;
			}
			else if (keyaction == FRS_RETURN)
# ifdef	NO1CLKACT
				if (TElastKeyWasMouse() == 0)
# endif /* NO1CLKACT */
			{
				evcb->event = fdopFRSK;
				evcb->val_event = FRS_NO;
				evcb->act_event = FRS_NO;
				evcb->intr_val = frscb->frs_globs->intr_frskey0;
				return(evcb->intr_val);
			}

			/*
			**  Otherwise, fall through and do normal processing.
			*/
		}

		if (keyfind && (keytype == NORM_KEY))
		{
		    /*
		    ** Single keystroke find is active, so store away the
		    ** key and return fdopFIND if in a character column.
		    */
		    if (tblfld && charcol)
		    {
			_VOID_ STlcopy(ks->ks_ch, evcb->ks_ch, (u_i4)2);
			return(fdopFIND);
		    }

		    /*
		    ** Turn 'special' keys into fdopERR keys.  These keys
		    ** include H, J, K, L, <space>, N and P.
		    */
		    code = fdopERR;
		}

		if (retval != OK)
		{
			FTbell();
			continue;
		}

# ifdef DATAVIEW
		if (IIMWdlkDoLastKey(FALSE, &do_here) == FAIL)
			return(fdNOCODE);
		if ( ! do_here)
			continue;
# endif	/* DATAVIEW */

		if (!(flg & fdcmBRWS))
		{
			code = fdopERR;
		}

		switch(code)		/* Interpret code */
		{
		  case fdopNULL:
		  case fdopHSPOT:
			break;

		  case fdopSPACE: /* Move left/right based on reverse status */
			if (reverse)
				code = fdopLEFT;
			else
				code = fdopRIGHT;
			/* Fall through */
		  case fdopLEFT:	/* Move cursor left in field */
		  case fdopRIGHT:	/* Move cursor right in field */
			if (scroll_fld)
			{
				if (code == fdopLEFT)
				{
					IIFTmlsMvLeftScroll(win, scroll_fld,
						reverse);
				}
				else /* fdopRIGHT */
				{
					IIFTmrsMvRightScroll(win, scroll_fld,
						reverse);
				}
				TDrefresh(win);
				ofldno = FTfldno;
				break;
			}
			/* else, fall through */
		  case fdopGOTO:	/* goto a fld from wview */
			if (code == fdopGOTO)
			{
				if (FTfldno == evcb->gotofld)
				{
				    if ((!tblfld)
					|| ((tblfld->tfcurcol == evcb->gotocol)
					    && (tblfld->tfcurrow
						== evcb->gotorow)))
				    {
					/* Not for WindowView. */
					if (IITDlsLocSupport && kyint)
					{
					    keyaction = (*kyint)(fdopSELECT,
						keytype, ks->ks_ch,
						(bool) FALSE);
					    if (keyaction == FRS_RETURN)
# ifdef	NO1CLKACT
						if (TElastKeyWasMouse() == 0)
# endif /* NO1CLKACT */
					    {
						evcb->event = fdopFRSK;
						evcb->val_event = FRS_NO;
						evcb->act_event = FRS_NO;
						evcb->intr_val
						    = frscb->frs_globs->intr_frskey0;
						FTvisclumu();
						return(evcb->intr_val);
					    }
					}
					break;
				    }
				}
				if (!(*IIFTgtcGoToChk)(frm, evcb))
				{
					FTbell();
					break;
				}
			}
		  case fdopNXITEM:	/* next item command */
		  case fdopPREV: /* Go to previous field */
		  case fdopNROW:
		  case fdopRET:
		  case fdopNEXT: /* Go to next field or next row in tbl fld. */
			if (scroll_fld)
			{
				IIFTsfrScrollFldReset(win, scroll_fld, reverse);
				TDrefresh(win);
			}

			IIFTsciSetCursorInfo(evcb);

			/*
			** If you have not changed fields, but have changed
			** columns in the table field, you may have a read
			** only column so return to calling routine.  If
			** fdchcol() is TRUE on the same table field
			** then this return is required.  If you change
			** fields, then fdchgcol() has no effect.
			*/
			mvcode = FTmove(code, frm, &FTfldno, &win, repts, evcb);
			if ((mvcode != fdNOCODE) && (mvcode != fdopBRWSGO))
			{
				return(mvcode);
			}
			/* Not for WindowView. */
			if (code == fdopGOTO && IITDlsLocSupport && kyint)
			{
			    keyaction = (*kyint)(fdopSELECT, keytype,
				ks->ks_ch, (bool) FALSE);
			    if (keyaction == FRS_RETURN)
# ifdef	NO1CLKACT
				if (TElastKeyWasMouse() == 0)
# endif /* NO1CLKACT */
			    {
				evcb->event = fdopFRSK;
				evcb->val_event = FRS_NO;
				evcb->act_event = FRS_NO;
				evcb->intr_val = frscb->frs_globs->intr_frskey0;
				FTvisclumu();
				return(evcb->intr_val);
			    }
			}
			if (frm->frmode != fdcmBRWS
			    && (FTfldno != ofldno
				|| ftchgcol(frm, ocolno, FTfldno)))
			{
				return(fdNOCODE);
			}
			FTbfldattr(frm, &reverse, &tblfld, &charcol);
			ofldno = FTfldno;
			break;

		  case fdopSCRUP:
			if (tblfld)
			{
				repts = tblfld->tfrows - 1;
				if (tblfld->tfcurrow != tblfld->tfrows - 1)
				{
				    repts += tblfld->tfrows - 1
						- tblfld->tfcurrow;
				}

				/*
				**  Fix for BUG 9320. (dkh)
				*/
				win->_cury = win->_maxy - 1;

				if (scroll_fld)
				{
					IIFTsfrScrollFldReset(win, scroll_fld,
						reverse);
					TDrefresh(win);
				}

				IIFTsciSetCursorInfo(evcb);

				if ((mvcode = FTmove(fdopDOWN, frm, &FTfldno,
					&win, repts, evcb)))
				{
					return(mvcode);
				}
				if (frm->frmode != fdcmBRWS
				    && (FTfldno != ofldno
					|| ftchgcol(frm, ocolno, FTfldno)))
				{
					return(fdNOCODE);
				}
				FTbfldattr(frm, &reverse, &tblfld, &charcol);
				ofldno = FTfldno;
				repts = 1;
			}
			else
			{
				TDscrlup(FTwin, win);
			}
			break;

		  case fdopSCRDN:
			if (tblfld)
			{
				repts = tblfld->tfrows - 1;
				if (tblfld->tfcurrow != 0)
				{
				    repts += tblfld->tfcurrow;
				}

				/*
				**  Fix for BUG 9320. (dkh)
				*/
				win->_cury = 0;

				if (scroll_fld)
				{
					IIFTsfrScrollFldReset(win, scroll_fld,
						reverse);
					TDrefresh(win);
				}

				IIFTsciSetCursorInfo(evcb);

				if ((mvcode = FTmove(fdopUP, frm, &FTfldno,
					&win, repts, evcb)))
				{
					return(mvcode);
				}
				if (frm->frmode != fdcmBRWS
				    && (FTfldno != ofldno
					|| ftchgcol(frm, ocolno, FTfldno)))
				{
					return(fdNOCODE);
				}
				FTbfldattr(frm, &reverse, &tblfld, &charcol);
				ofldno = FTfldno;
				repts = 1;
			}
			else
			{
				TDscrldn(FTwin, win);
			}
			break;

		  case fdopSCRLT:
			TDscrllt(FTwin, win);
			break;

		  case fdopSCRRT:
			TDscrlrt(FTwin, win);
			break;

# ifdef DATAVIEW
		  case fdopSCRTO:
			/* Scroll bar button was moved */
			/* Bug 87710. Return to caller to pass through its tablefield
			** operation code to ensure correct tablefield is scrolled
			*/
			
			return (code);
#endif

		  case fdopPRSCR:
		  {
			char	msgline[MAX_TERM_SIZE + 1];

			TDsaveLast(msgline);
			FTprnscr(NULL);
			TDwinmsg(msgline);
			TDrefresh(win);
			break;
		  }

		  case fdopSKPFLD:
		  {
			i4     skipcnt;
			i4     scode;
			char    pbuf[512];
# ifdef DATAVIEW
			if (IIMWimIsMws())
			{
				if (IIMWgscGetSkipCnt(frm, FTfldno,
					&skipcnt) != OK)
				{
					break;
				}
				goto doskip;
			}
# endif /* DATAVIEW */

			pbuf[0] = '\0';
			FTprmthdlr(ERget(S_FT0001_Number_of_fields_to_s),
				(u_char *) pbuf, (i4) TRUE, NULL);
			if (pbuf[0] == '\0')
			{
				break;
			}
			if (CVan(pbuf, &skipcnt) != OK)
			{
				FTbotsav();
				IIUGerr(E_FT0002_Bad_value_entered_for,
					UG_ERR_ERROR, 0);
				FTbotrst();
				break;
			}

# ifdef DATAVIEW
doskip:
# endif /* DATAVIEW */

			if (skipcnt == 0)
			{
				break;
			}
			if (skipcnt < 0)
			{
				scode = fdopPREV;
				skipcnt = -skipcnt;
			}
			else
			{
				scode = fdopNEXT;
			}

			if (scroll_fld)
			{
				IIFTsfrScrollFldReset(win, scroll_fld, reverse);
				TDrefresh(win);
			}

			IIFTsciSetCursorInfo(evcb);

# ifdef DATAVIEW
			if (IIMWimIsMws())
				scode = fdopSKPFLD;
# endif /* DATAVIEW */

			if ((mvcode = FTmove(scode, frm, &FTfldno, &win,
				skipcnt, evcb)) != fdNOCODE)
			{
				return(mvcode);
			}
			if (frm->frmode != fdcmBRWS
			    && (FTfldno != ofldno
				|| ftchgcol(frm, ocolno, FTfldno)))
			{
				return(fdNOCODE);
			}
			FTbfldattr(frm, &reverse, &tblfld);
			ofldno = FTfldno;
			break;
		  }

		  case fdopREFR:	/* refresh the screen */
		  {
			/* if graphics on frame, erase graphics screen */
			if ((FTgtflg & FTGTACT) != 0 && iigtclrfunc)
				(*iigtclrfunc)();
			TDrefresh(curscr);
			/* if graphics on frame, refresh graphics */
			if ((FTgtflg & FTGTACT) != 0 && iigtreffunc)
				(*iigtreffunc)();

# ifdef DATAVIEW
			_VOID_ IIMWrRefresh();
# endif	/* DATAVIEW */

			break;
		  }


		  case fdopUP:		/* Move cursor up in field */
		  case fdopDOWN:	/* Move cursor down in field */
			if (scroll_fld && tblfld)
			{
				if (((code == fdopUP) && (win->_cury == 0))
				    || ((code == fdopDOWN)
					&& (win->_cury == win->_maxy - 1)))
				{
					IIFTsfrScrollFldReset(win, scroll_fld,
						reverse);
					TDrefresh(win);
				}
			}

# ifdef DATAVIEW
			/*
			**  If MWS returned this code while in a
			**  table, the user must be at an extreme
			**  row.  Set the window parameters to
			**  reflect this so that appropriate
			**  things happen in FTmove.
			*/
			if (tblfld && IIMWimIsMws())
			{
				if (code == fdopUP)
					win->_cury = 0;
				else if (code == fdopDOWN)
					win->_cury = win->_maxy - 1;
			}
# endif	/* DATAVIEW */

			IIFTsciSetCursorInfo(evcb);

			if ((mvcode = FTmove(code, frm, &FTfldno,
				&win, repts, evcb)) != fdNOCODE)
			{
				return(mvcode);
			}

			/* no col check */

			if (frm->frmode != fdcmBRWS && FTfldno != ofldno)
			{
				return(fdNOCODE);
			}
			FTbfldattr(frm, &reverse, &tblfld, &charcol);
			ofldno = FTfldno;
			break;

		  case fdopBEGF:
		  case fdopENDF:
		  case fdopERR:		/* Unknown character or something
					** that doesn't belong in edit mode
					*/
			if (prerrmsg)
			{
				prerrmsg = FALSE;
				FTbotsav();
				IIUGerr(E_FT003B_INVALID_COMMAND,
					UG_ERR_ERROR, 0);
				FTbotrst();
				TDrefresh(win);
			}
			continue;

		  case fdopWORD:
			FTword(frm, FTfldno, win, 1);
			TDrefresh(win);
			break;

		  case fdopBKWORD:
			FTword(frm, FTfldno, win, -1);
			TDrefresh(win);
			break;

		  case fdopSHELL:
			/* if graphics on frame, erase graphics screen */
			if ((FTgtflg & FTGTACT) != 0 && iigtclrfunc)
				(*iigtclrfunc)();
			FTshell();
			/* if graphics on frame, refresh graphics */
			if ((FTgtflg & FTGTACT) != 0 && iigtreffunc)
				(*iigtreffunc)();
			break;

		  case fdopMENU:
			event = evcb->event;
			if ((event == fdopSCRLT) || (event == fdopSCRRT))
			{
			    IIFTsmShiftMenu(event);
			    evcb->event = fdopMENU;
			}
			(*FTdmpmsg)(ERx("MENU KEY selected\n"));
		  default:
			if ( code != fdopMENU )
			{
				(*FTdmpmsg)(ERx("Activated control key %s\n"),
							CMunctrl(ks->ks_ch));
			}
			if (scroll_fld)
			{
				IIFTsfrScrollFldReset(win, scroll_fld, reverse);
				TDrefresh(win);
				FTvisclumu();
			}
			return (code);
		}
		prerrmsg = TRUE;
	}
}

/*
** fdoldcol()	-	If in a table field then save the current column to
**			see if a change in columns later.
*/
static	i4
ftoldcol(frm, fldno)
FRAME	*frm;
i4	fldno;
{
	FIELD	*fd;

	fd = frm->frfld[fldno];
	if (fd->fltag == FTABLE)
	{
		return (fd->fld_var.fltblfld->tfcurcol);
	}
	return (-1);		/* nonexistant column will not have effect */
}

/*
** fdchgcol()	-	If changed the column in the table field (and haven't
**			changed fields, otherwise has no effect) then return
**			TRUE.
*/
static i4
ftchgcol(frm, oldcolumn, fldno)
FRAME	*frm;
i4	oldcolumn;
i4	fldno;
{
	FIELD	*fd;

	fd = frm->frfld[fldno];
	if (fd->fltag == FTABLE)
	{
		return (oldcolumn != fd->fld_var.fltblfld->tfcurcol);
	}
	return (TRUE);
}
