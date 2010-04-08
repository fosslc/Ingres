/*
**	iitbputf.c
**
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:	iitbputf.c - Run a form and trap scroll actions.
**
** Description:
**	Puts the frame and responds to all runtime calls to scroll the
**	table.	(Runtime scrolls are cursor scrolls.)
**	Routines defined:
**	- tbchkscr - Check for scroll from a resume next.
**	- IItbputfrm - Run a form.
**	- tbscrhdlr - Table field scroll processing.
**	- tbputf - Do actual table field scrolling.
**
**	NOTE - This file does not use IIUGmsg since it does not
**	restore the message line.
**
** History:
**	04-mar-1983	- written (ncg)
**	12/23/86 (dkh) - Added support for new activations.
**	02/17/87 (dkh) - Added header.
**	19-jun-87 (bruceb) Code cleanup.
**	08/14/87 (dkh) - ER changes.
**	12/12/87 (dkh) - Added comment on using IIUGmsg.
**	09-mar-89 (bruceb)
**		Added support for entry activation.  On return from
**		FDputfrm(), set the frame's storage of the current
**		dataset row (NULL if not a table field).  FDputfrm() now called
**		with new param., a pointer to function that determines
**		if an EA is applicable based on a changed dataset row.
**		That function, IITBeaEntAct(), is also defined.
**		IITBgdcGetDsCur() defined to do work of obtaining the
**		dataset row value to be set and compared.
**	07-jul-89 (bruceb)
**		FDputfrm() no longer called with function pointer param.
**		Instead, FDputfrm is using a function pointer within a control
**		block passed down to frame by IIforms.
**	01-feb-90 (bruceb)
**		Added support for single keystroke find.
**	04/07/90 (dkh) - Added ugly code to allow user to get off of
**			 a fake row created at end of tf (us #196).
**	03/20/91 (dkh) - Added support for (alerter) event in FRS.
**	08/11/91 (dkh) - Added change to prevent alerter events from
**			 being triggered when no menuitems were selected
**			 or in between a column/scroll combination.
**	06/29/92 (dkh) - Added support for chaging the behavior of the
**			 "Out of Data" message when scrolling a table field.
**	03/19/96 (chech02) -
**			 Added function prototypes for win 3.1 port.
**	19-dec-97 (kitch01)
**			Bug 87710. Added support for fdopSCRTO in tbophdlr. This allows a tablefield
**			to be scrolled via MWS SCRTO command sent from Upfront. Doing the work
**			here ensures we scroll the correct tablefield.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	21-Aug-2009 (kschendel) 121804
**	    Update some of the function declarations to fix gcc 4.3 problems.
**/


# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ft.h>
# include	<fmt.h>
# include	<adf.h>
# include	<frame.h>
# include	<menu.h>
# include	<runtime.h>
# include	<er.h>
# include	"ertb.h"
# include	<rtvars.h>
# include	<st.h>


/*
**	Modified so as not to give repeated "out of data" messages on the
**	same error. (If at top of the table, and use 4K, we only want one
**	message). "sc" flags were added to implement this.
*/

# define	scNOUP	1		/* out of data scrolling up */
# define	scNODWN 2		/* out of data scrolling down */
# define	scNONE	3		/* good scroll or no scroll */

static	i2		tbputf();		/* executes scroll tester */
static	i4		tbscrhdlr();
static	i2		lastsc = scNONE;	/* last bad scroll executed */
static	TBSTRUCT	*lasttb = NULL;		/* last table not scrolled  */
static	TBROW		*lastdt = NULL;		/* last top displayed row */
static	DATSET		*lastds = NULL;		/* last data set */



FUNC_EXTERN	VOID	IITBscfSingleChrFind();
FUNC_EXTERN	i4	IIFDaeAlerterEvent();

GLOBALREF	TBSTRUCT	*IIcurtbl;

char		*FDfldnm();
i4		FDputfrm();
i4		IITBeaEntAct();
PTR		IITBgdcGetDsCur();
static i4       tbophdlr();




/*{
** Name:	tbchkscr - Check for a scroll from a resume next.
**
** Description:
**	Internal routine that checks to see if a scroll needs
**	to be done as a result of a resume next that was
**	executed earlier or just completing a regular scroll.
**	The resume next situation can happen when the user
**	has an activation on the column of a table field
**	and he does a DOWNLINE command at the bottom
**	of the table field.  An UPLINE command at the top
**	of the table field will also cause a scroll to
**	be initiated as a result of a resume next from an
**	activate column block.	For the regular scroll
**	case, there are no activates on scroll so the code
**	simply completes the scroll.  This is necessary
**	since tbacc is the only place (currently) that handles
**	the dataset for a table field.
**
** Inputs:
**	frm	Form to run check on.
**
** Outputs:
**	rval	Return value indicating what needs to be done.
**		Possible return values are:
**		- fdopSCRUP - Need to process a scroll up.
**		- fdopSCRDN - Need to process a scroll down.
**		- fdNOCODE - No scrolling needs to be done.
**	Returns:
**		None.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	02/17/87 (dkh) - Added procedure header.
*/
static VOID
tbchkscr(frm, rval)
FRAME	*frm;
i4	*rval;
{
	i4		oper;
	FIELD		*fld;
	TBLFLD		*tbl;
	TBSTRUCT	*tb;

	*rval = fdNOCODE;

	/*
	**  If there is no resume next out of a scroll activate
	**  check on regular scrolling.
	*/

	if ((oper = FDchkscroll()) == 0)
	{
		tb = IItfind(IIstkfrm, FDfldnm(frm));
		if (frm->frfldno > 0)
		{
			fld = frm->frfld[frm->frcurfld];
			if (fld->fltag == FTABLE)
			{
				tbl = fld->fld_var.fltblfld;
				switch(tbl->tfstate)
				{
					case tfSCRUP:
						/*
						**  The field is scrolling up.
						**  The user has already got
						**  control, and hopefully
						**  entered a value of the new
						**  row.  If tfputrow is true,
						**  the user has entered a
						**  value.  Otherwise, he has
						**  not.
						*/
						if (tb != NULL &&
						    tb->dataset == NULL &&
						    tbl->tfcurrow - tbl->tfrows > 0)
						{
							/*
							**  No scrolling for
							**  bare table fields.
							*/
							tbl->tfcurrow = 0;
							FTbell();
							swinerr(ERget(F_TB0001_No_scrolling_allowed), TRUE);
							break;
						}
						if (--tbl->tfcurrow > tbl->tflastrow)
						{
							*rval = tbl->tfscrup;
							break;
						}
						if (tbl->tfcurrow < tbl->tflastrow)
						{
							tbl->tfcurrow = tbl->tflastrow;
						}
						break;

					case tfSCRDOWN:
						if (tb != NULL &&
						    tb->dataset == NULL &&
						    tbl->tfcurrow < -1)
						{
							/*
							**  No scrolling for
							**  bare table fields.
							*/
							tbl->tfcurrow = 0;
							FTbell();
							swinerr(ERget(F_TB0002_No_scrolling_allowed), TRUE);
							break;
						}
						if (++tbl->tfcurrow < 0)
						{
							*rval = tbl->tfscrdown;
							break;
						}

						/*
						**  BUG 1429 - make sure the
						**  row numbers cannot be
						**  inconsistent, if a call
						**  forced a change in the
						**  current row during an
						**  activate scroll.
						*/
						if (tbl->tfcurrow > 0)
						{
							tbl->tfcurrow = 0;
						}
						break;

					case tfFILL:
						if (tbl->tfinsert == fdNOCODE)
						{
							break;
						}
						tbl->tfstate = tfINSERT;
						tbl->tfcurrow = 0;
						*rval = tbl->tfinsert;
						break;

					case tfINSERT:
						/*
						**  When we get here, currow
						**  should already have been
						**  entered.
						*/
						if (++tbl->tfcurrow >= tbl->tfrows)
						{
							tbl->tfcurrow = 0;
							break;
						}
						*rval = tbl->tfinsert;
						break;

					default:
						break;
				}
				if (*rval == fdNOCODE)
				{
					FTmarkfield(frm, frm->frcurfld, FT_UPDATE, FT_TBLFLD, -1, -1);
					tbl->tfstate = tfNORMAL;
				}
			}
		}
	}
	else
	{
		*rval = oper;
	}
}





/*{
** Name:	IItputfrm - Run a form.
**
** Description:
**	Call FDputfrm() and get the returned value.  Call tbputf()
**	with this value and either return control to "display" loop or
**	back to local "for" loop.  Routine will continue looping
**	until something interesting happens.  Interesting events are
**	some sort of activation needs to happen (menu item, frskey,
**	field, column, scrollup and scrolldown) or the caller needs
**	to allow user to select a menu item.  The call to tbchkscr()
**	checks to see if we must handle a table field scroll.  If not,
**	call FDputfrm() to allow user access to form and wait for
**	interesting event to occur.  Call to tbscrhdlr() is finally
**	made, in either case, to handle scroll processing.
**
** Inputs:
**	frm	Form to run.
**
** Outputs:
**	Returns:
**		fdopMENU	Allow caller to process menu key selection.
**		>0		Interrupt value for an activation block.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	02/17/87 (dkh) - Added procedure header.
*/
i4
IItputfrm(frm)
FRAME	*frm;
{
	i4		rval = fdNOCODE;	/* returned from FDputfrm */

	for (;;)
	{
		tbchkscr(frm, &rval);
		if (rval == fdNOCODE)
		{
			frm->frmflags &= ~fdIN_SCRLL;

			/*
			**  If alerter block is present, check to see
			**  if one is waiting.  If so, then return.
			**
			**  But don't check if user only pressed the
			**  return key when on the menuline.
			*/
			if (frm->frmflags & fdNO_MI_SEL)
			{
				frm->frmflags &= ~fdNO_MI_SEL;
			}
			else
			{
				if (IIstkfrm->alev_intr && IIFDaeAlerterEvent())
				{
					return(fdopALEVENT);
				}
			}
			lastds = NULL;
			rval = (i4) FDputfrm(frm, IIfrscb);

			/*
			** savetblpos is set by FDputfrm() if returning because
			** no fields are currently reachable (returning to the
			** menu line).  Need to leave the old dataset row
			** untouched for proper determination of EA when
			** eventually returning to the form.
			*/
			if (frm->frres2->savetblpos == FALSE)
			{
			    /*
			    ** Set value of current dataset row for Entry Act.
			    ** Will be NULL if not a table field with a data
			    ** set.
			    */
			    frm->frres2->origdsrow = IITBgdcGetDsCur();
			}
			frm->frres2->savetblpos = FALSE;
		}
		if (tbophdlr(frm, &rval))
		{
			break;
		}
	}
	return (rval);
}



/*{
** Name:	tbophdlr - Table field operations processing.
**
** Description:
**	Do table field scroll processing.  This is the upper layer
**	for handling a scroll{up|down},find and fake row
**	deletion in a table field.
**
**	Calls tbputf() to do the actual scroll.
**
** Inputs:
**	frm	Form containing table field to check.
**	rval	Type of scroll to process.
**
** Outputs:
**	rval	Interrupt value for scroll activation, if any.
**	Returns:
**		TRUE	If an activate needs to occur.
**		FALSE	No scroll activation found, normal
**			scroll done.  Continue in loop in
**			IItputfrm().
**	Exceptions:
**		None.
**
** Side Effects:
**	An unknown scroll command will cause "lasttb" and "lasttc"
**	to be reset.
**
** History:
**	02/17/87 (dkh) - Added procedure header.
**	12-dec-97 (kitch01) 
**		Bug 87710. Added support for the Upfront sent SCRTO command.
**		Doing the work here ensures that the correct tablefield is scrolled.
*/
static i4
tbophdlr(frm, rval)
FRAME	*frm;
i4	*rval;
{
	i4	outofloop = TRUE;
	i4	intrval;
	TBSTRUCT *tb;
	i2	actflg;

	if (lastds == NULL && (*rval == fdopSCRUP || *rval == fdopSCRDN))
	{
		tb = IItfind(IIstkfrm, FDfldnm(frm));
		lastds = tb->dataset;
		if (lastds)
		    lastdt = lastds->distop;
	}

	switch(*rval)
	{
		case fdopSCRUP:
			/*
			** If activated then check scrintrp[0]
			** and fill intrval with interrupt value,
			** otherwise scroll "UP", in frame frm.
			** Print "out of data" if last error was not scNOUP.
			*/
			actflg = tbputf(0, ERx("UP"), scNOUP, frm, &intrval);

			if (actflg < 0)
			{
				*rval = fdopERR;
				break;		/* bug error */
			}

			/* activate on interrupt value */
			if (actflg == 0)
			{
				/*
				**  Fix for BUG 8177. (dkh)
				*/
				if (lastds != NULL && lastds->distop == NULL)
				{
					tb->tb_fld->tfcurrow = tb->tb_fld->tflastrow;
					tb->tb_fld->tfstate = tfNORMAL;
					outofloop = FALSE;
					swinerr(ERget(F_TB0003_Nothing_to_scroll), TRUE);
					break;
				}
				*rval = intrval;
				break;
			}
			/* regular scroll executed */
			outofloop = FALSE;
			break;

		case fdopSCRDN:
			/*
			** If activated then check scrintrp[1]
			** and fill intrval with interrupt value,
			** otherwise scroll "DOWN", in frame frm.
			** Print "out of data" if last error was not scNODWN.
			*/
			actflg = tbputf(1, ERx("DOWN"), scNODWN, frm, &intrval);

			if (actflg < 0)
			{
				*rval = fdopERR;
				break;		/* bug error */
			}

			/* activate on interrupt value */
			if (actflg == 0)
			{
				*rval = intrval;
				break;
			}
			/* regular scroll executed or validation fail */
			outofloop = FALSE;
			break;

		case fdopFIND:
			lasttb = NULL;
			lastsc = scNONE;
			IITBscfSingleChrFind(frm, IIfrscb->frs_event->ks_ch);
			outofloop = FALSE;
			break;

		case fdopDELROW:
			/*
			**  If no scrolling action, check to see
			**  if it is a special situation that requires
			**  us to delete a fake row.  YES, THIS IS
			**  UGLY ARCHITECTURALLY, BUT I PLEAD INNOCENCE
			**  DUE TO DURESS.
			**
			**  Ignoring returns values to IItbsetio and
			**  IItdelrow since we should have no problem
			**  at this point.
			*/
			(VOID) IItbsetio(cmDELETER, frm->frname, FDfldnm(frm),
				rowCURR);
			(VOID) IItdelrow(0);
			lasttb = NULL;
			lastsc = scNONE;
			outofloop = FALSE;
			break;

		case fdopSCRTO:
			/* Upfront can send a 'scroll to' message via MWS to scroll
			** a tablefield to a given row. This should always succeed
			** as Upfront should not send a 'scroll to n' where n is
			** outside the range of the tablefield rows.
			*/
			actflg = tbputf(1, ERx("TO"), scNONE, frm, &intrval);
			
			if (actflg < 0)
			{
				*rval - fdopERR;
				break;		/* bug error */
			}
			
			/* activate on interrupt value */
			/* Currently disabled for SCRTO in tbputf. May need to be
			** revisited later
			if (actflg == 0)
			{
				*rval = intrval;
				break;
			}
			*/
			/* regular scroll executed or validation fail */
			outofloop = FALSE;
			break;

		default:
			lasttb = NULL;
			lastsc = scNONE;
			break;
	}
	return(outofloop);
}



/*{
** Name:	tbputf - Do actual table field scrolling.
**
** Description:
**	Actually does the local put frame using the current table field.
**	This does the actual work of scrolling a table field.  The
**	scroll{up|down} interrupt value for the table field can be
**	found in member "scrintrp" of the TBSTRUCT structure.  If
**	any interrupt is set, return this information immediately.
**	It is now up to user code to complete the scroll.  If no
**	interrupt set, just scroll the table field (by calling
**	IItbsmode() and IItscroll()) and return.
**
**	Two important things to note.  Variables "lasttb" and "lastsc"
**	are used to prevent repeated "Out of Data" messages from
**	appearing on the terminal.  "Lasttb" points to the last
**	table field that a scroll was processed on.  As for "lastsc",
**	it holds the direction of the scroll that hit a boundary.
**	"Lasttb" and "lastsc" will be set to NULL and scNONE, respectively,
**	when an interrupt is to occur or the scroll completed.
**
** Inputs:
**	intrpflg	Index into table field's interrupt array.
**			Zero for a scroll up and one for a scroll down.
**	scrmd		Direction to scroll (UP or DOWN).
**	scflag		Value to assign to "lastsc" if there is no more
**			data to scroll.
**	frm		Form that contains table field.
**
** Outputs:
**	intrval Interrupt value for the activate scroll{up|down}
**		block in user code.
**
**	Returns:
**		-1	Error (table not found).
**		0	Activated on an interrupt value.
**		1	Regular scroll executed, or bad validation.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	02/17/87 (dkh) - Added procedure header.
**  19-dec-97 (kitch01)
**		bug 87710. Added support for Upfront SCRTO message.
*/
static i2
tbputf(intrpflg,  scrmd, scflag, frm,  intrval)
i2		intrpflg;		/* index into table's interrupt arr */
char		*scrmd;			/* scroll mode if needed */
i2		scflag;			/* in case of bad scroll is set */
FRAME		*frm;			/* displayed frame */
i4		*intrval;		/* content of interrupt value */
{
	register TBSTRUCT	*tb;	/* current table */

	/* get current table in displayed frame */
	tb = IItfind(IIstkfrm, FDfldnm(frm));

	tb->tb_state = tbSCINTRP;

	/*
	** if an activate statement was set on this table then set the UP/DOWN
	** interrupt value and return the activate flag.
	*/
	/* Bug 87710. Since the code prior to this change did not acknowledge this
	** situation I am disabling it for Upfront SCRTO messages here. This may
	** need to be revisited in the future.
	*/
	if ((scflag != scNONE) && ((*intrval = tb->scrintrp[intrpflg]) > 0))
	{
		lasttb = NULL;		/* update */
		lastsc = scNONE;
		return (0);
	}
	/*
	 * No interrupt value, execute regular scroll.
	 */

	IIcurtbl = tb;
	IIfrmio = IIstkfrm;

	/*
	**  Set up frame info for possible table field traversal.
	**  Necessitated by FT. (dkh)
	*/

	FDFTsetiofrm(IIfrmio->fdrunfrm);

	IItbsmode(scrmd);		/* internal call to routine */

	switch(*scrmd)
	{
		case 'U':
		case 'D':

			if (IItscroll((i4) 0, (i4) rowNONE))
			{
				lasttb = NULL;		/* no problems on scroll */
				lastsc = scNONE;
			}
			else	/* no data to scroll */
			{
				TBLFLD	*tbl;

				/*
				**  If we can't scroll, then reset values to
				**  eliminate unnecessary passes through the
				**  code.
				*/
				tbl = tb->tb_fld;
				if (tbl->tfstate == tfSCRUP)
				{
					tbl->tfcurrow = tbl->tflastrow;
					tbl->tfstate = tfNORMAL;
				}
				else if (tbl->tfstate == tfSCRDOWN)
				{
					tbl->tfcurrow = 0;
					tbl->tfstate = tfNORMAL;
				}

				/* check that not repeated error message */
				if ((lastsc != scflag) || (tb != lasttb))
				{
					lasttb = tb;
					lastsc = scflag;

					/*
					**  If the display has not changed then put
					**  out silent error message.  But if we ran
					**  out of rows to scroll and the display
					**  has changed, then reset lasttb and lastsc
					**  so that if we scroll again we will get the
					**  error message. (dkh)
					**  Make sure there is a data-set. (ncg)
					*/
					if (tb->dataset && lastds == tb->dataset &&
						lastdt == tb->dataset->distop)
					{
						if (IIfrscb->frs_globs->enabled & ODMSG_BELL)
						{
							FTbell();
						}
						else if (!(IIfrscb->frs_globs->enabled & ODMSG_OFF))
						{
							swinerr(ERget(F_TB0004_Out_of_data),
								FALSE);
						}
					}
					else
					{
						lasttb = NULL;
						lastsc = scNONE;
					}
				}
			}
			break;

		case 'T':
			_VOID_ IItscroll((i4) 0, (i4) IIfrscb->frs_event->gotorow);
			break;

		default:
			/* Error invalid scrmd */
			return(-1);
	}
	return (1);
}

/*{
** Name:	IITBeaEntAct - Determine if dataset row has changed.
**
** Description:
**	Determine if an EA should be triggered based on a change in
**	the (current) table field's dataset row.  This routine is called
**	by FDputfrm() only after determining that EA is enabled for the
**	application, that the field has EA defined for it, and that
**	neither the field, nor the column have changed.  This is the
**	last resort for determining if an EA is applicable.
**
**	Note that bare table fields, having no dataset, will not trigger
**	an EA through this routine.
**
** Inputs:
**
** Outputs:
**
**	Returns:
**		TRUE	Entry activation should occur (dataset row has changed.)
**		FALSE	Entry activation should not occur.
**
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	09-mar-89 (bruceb)	Written.
*/
i4
IITBeaEntAct()
{
    i4		retval = FALSE;
    FRAME	*frm = IIstkfrm->fdrunfrm;
    PTR		crow;

    crow = IITBgdcGetDsCur();
    if (crow != NULL && crow != frm->frres2->origdsrow)
	retval = TRUE;

    return(retval);
}

/*{
** Name:	IITBgdcGetDsCur - Obtain the dataset row (if one exists).
**
** Description:
**	Returns the currently occupied dataset row for the current table
**	field.  Returns NULL if not a table field, no data set, or some
**	unknown error (should never happen--but not fatal here) occurs.
**
** Inputs:
**
** Outputs:
**
**	Returns:
**		Dataset row pointer or NULL.
**
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	23-mar-89 (bruceb)	Written.
*/
PTR
IITBgdcGetDsCur()
{
    i4		offset;	/* number of rows from top of tblfld display area */
    TBSTRUCT	*tb;	/* current table */
    TBROW	*crow = NULL;
    FRAME	*frm = IIstkfrm->fdrunfrm;

    /* get current table in displayed frame */
    tb = IItfind(IIstkfrm, FDfldnm(frm));

    if (tb && tb->dataset)
    {
	offset = frm->frres2->origrow;
	crow = tb->dataset->distop;
	for (; (offset > 0 && crow != NULL); offset--, crow = crow->nxtr)
	    ;
    }

    return((PTR)crow);
}




/*{
** Name:	IITBldrLastDSRow - Are we on last dataset row?
**
** Description:
**	This routine returns TRUE if the current row in the
**	table field is the last row in the dataset.  Otherwise,
**	FALSE is returned.
**
** Inputs:
**	tf	Pointer to table field to check against.
**
** Outputs:
**
**	Returns:
**		TRUE	If current row in table field is last row in dataset.
**		FALSE	If current row is not last dataset row.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	04/10/90 (dkh) - Initial version.
*/
i4
IITBldrLastDsRow(tfld)
TBLFLD	*tfld;
{
	TBSTRUCT	*tb;
	i4		i;
	i4		currow;
	TBROW		*cr;

	tb = IItfind(IIstkfrm, tfld->tfhdr.fhdname);

	if (tb && tb->dataset)
	{
		currow = tfld->tfcurrow;
		for (i = 0, cr = tb->dataset->distop; i < currow && cr != NULL;
			i++, cr = cr->nxtr)
		{
			;
		}
		if (cr == tb->dataset->bot)
		{
			return(TRUE);
		}
	}

	return(FALSE);
}
