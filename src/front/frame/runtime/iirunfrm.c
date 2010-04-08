/*
**	iirunfrm.c
**
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<me.h>		/* 6-x_PC_80x86 */
# include	<st.h>		/* 6-x_PC_80x86 */
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
# include	<frserrno.h>
# include	<frscnsts.h>
# include	<er.h>
# include	<rtvars.h>

/**
** Name:	iirunfrm.c
**
** Description:
**
**	Public (extern) routines defined:
**		IIrunform()
**	Private (static) routines defined:
**		include()	-	For the PC version
**
** History:
**	19-jun-87 (bruceb)	Code cleanup.
**	08/26/87 (dkh) - Changes for 8000 series error numbers.
**	09/01/87 (dkh) - Added explicit include of <frserrno.h>.
**	06/22/88 (dkh) - Fixed so that we know when a popup form
**			 (even one that was forced to be a popup)
**			 was displayed.
**	10-nov-88 (bruceb)
**		Handle timeout.  Return the activate timeout value when
**		timeout occurs (whether on the form or on the menuline.)
**		FOR NOW, since timeout value is same for all frames, set
**		value in frame's event control block before wiping out
**		contents of global event control block.
**	02/18/89 (dkh) - Fixed venus bug 4546.
**	06-mar-89 (bruceb)
**		Save and restore timeout value after a 'resume menu'.
**	08-mar-89 (bruceb)
**		Turn off entry activation flag at top of run loop.
**	10-mar-89 (bruceb)
**		Save event code in 'lastcmd' inside of RetSetCB().
**	04/06/89 (dkh) - Changed to handle breakdisplay out of the
**			 init block for a popup style form.
**	11-jul-89 (bruceb)
**		Set up IIfrmio at start of routine.  Useful for derivation
**		processing.  Set evcb->eval_aggs on entry to IIrunform,
**		off on exit.
**	01/19/90 (dkh) - Integrated from 6.4 fix for screen update
**			 problem with serialized popup display loops.
**	02/22/90 (dkh) - Fixed bug 9961.
**	03/21/90 (dkh) - Restricted fix of 01/19/90 to just deal with
**			 popups.  The original change applied to fullscreen
**			 forms as well which caused extra output to appear
**			 in test suites.
**	03/22/90 (dkh) - Undo 6.3 integration of changes from 01/19/90 since
**			 it came from 6.4 in the first place.
**	29-mar-90 (bruceb)
**		Added locator support.  Turn on menu flag MU_FRSFRM to
**		indicate the menu is attached to a 'real' form.
**	04/14/90 (dkh) - Integrated changes from 6.4 to fix bug 9798.
**	06/27/90 (dkh) - Optimized fixed to bug 9798 by only doing
**			 a redisplay if it is not the first time through.
**	07/05/90 (dkh) - Put in check to call IIresfrsk() if we exited
**			 from a submenu.
**	07/20/90 (dkh) - Fixed bug 31825.
**	03/19/91 (dkh) - Added support for alerter events in FRS.
**	04/01/91 (dkh) - Removed IIFRsaiSubmuAeIntr as it is no longer used.
**	08/11/91 (dkh) - Added changed to prevent alerter events from
**			 being triggered when no menuitems were selected
**			 or in between a column/scroll activation combination.
**	06/27/92 (dkh) - Added support for renaming and enabling/disabling
**			 menuitems.
**	08/31/93 (dkh) - Fixed up compiler complaints due to prototyping.
**	17-nov-93 (connie) Bug #42387
**		Fixed the checking for previous frame as a popup using
**		the global variable IIFRpfwp
**	04-feb-94 (connie) Bug #42387
**		Backed out the previous fix which had resulted in a very
**		different FRS output
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	18-Aug-2009 (kschendel) 121804
**	    Update some of the function declarations to fix gcc 4.3 problems.
**/

typedef	char	*(*cfunction)();

FUNC_EXTERN	STATUS	IImumatch();
FUNC_EXTERN	bool	IIFRmrMenuReformat();
FUNC_EXTERN	VOID	IIFDpadProcessAggDeps();

i4	IIavchk();
static		i4	RetSetCB(i4);

GLOBALREF	i4	form_mu;
GLOBALREF	bool	IIFRpfwp;
GLOBALREF	char	IIFRpfnPrevFormName[];
GLOBALREF	bool	IIexted_submu;

/*{
** Name:	IIrunform	-	Run the frame driver
**
** Description:
**	This routine runs the frame driver.  It is put into a loop known
**	as the display loop.  IIrunfrm() calls IItputfrm() which is the
**	the routine that actually runs a form.  After the call to IItputfrm(),
**	IIrunfrm() runs the menu driver if an escape is hit.  IIrunfrm then
**	returns the appropriate interrupt code to be tested for in the activate
**	statements.
**
**	This routine is part of RUNTIME's external interface.
**	
** Inputs:
**
** Outputs:
**
** Returns:
**	i4	TRUE
**		FALSE
**
** Exceptions:
**	none
**
** Example and Code Generation:
**
**	## display f1 
**	## finalize
**	
**	if (IIdispfrm("f1","f") == 0) goto IIfdEnd1;
**	while (IIrunform() != 0) {
**	}
**
** Side Effects:
**
** History:
**	22-feb-1983  -  Extracted from original runtime.c (jen)
**	25-feb-1983  -  Added table field implementation. (ncg)
**	12/24/86 (dkh) - Added support for new activations.
**	05-jan-1987 (peter)	Changed RTfndfld to FDfndfld.
**
*/

# ifdef	PCINGRES
/*
**	Pull these in for PC Ingres so that we can split the runtime
**	diretories into a separate library.
*/
static
include()
{
	FDfndfld();
	FDfndcol();
}
# endif	/* PCINGRES */



/*{
** Name:	anyfrskeys - Are there any frskey activations.
**
** Description:
**	This routine merely checks to see if there are any frskey
**	activations based on the passed in FRSKY structure array.
**	TRUE if any frskey activations are set.  FALSE if NONE.
**	Activations are distinguished by structure member frs_int
**	having positive values.
**
** Inputs:
**	frskey	Pointer to an array of FRSKY structures.
**
** Outputs:
**
**	Returns:
**		TRUE	If any frskey activations are found.
**		FALSE	If NO frskey activations are found.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	02/22/90 (dkh) - Initial version.
*/
static i4
anyfrskeys(frskey)
FRSKY	*frskey;
{
	i4	i;

	for (i = 0; i < MAX_FRS_KEYS; i++, frskey++)
	{
		if (frskey->frs_int > 0)
		{
			return(TRUE);
		}
	}
	return(FALSE);
}


IIrunform()
{
	FRAME	*frm;				/* pointer to current frame */
	MENU	runmu;				/* pointer to current menu */
	i4	intrval;
	i4	act;
	i4	dummy;
	FRS_EVCB *evcb;
	bool	first;
 	i4	timeout;
	char	*pname = "";
	FRAME	*pfrm;
	bool	rebuild = FALSE;
	i4	retval;

	/*
	**	Check all variables make sure they are valid.
	**	Also check to see that the forms system has been
	**	initialized and that there is a current frame.
	*/
	if (!RTchkfrs(IIstkfrm))
		return (FALSE);

	IIfrmio = IIstkfrm;

	first = ((IIstkfrm->rfrmflags & RTACTNORUN) ? TRUE : FALSE);

	IIstkfrm->rfrmflags &= ~RTACTNORUN;

	frm = IIstkfrm->fdrunfrm;

	if (frm->frmflags & fdDISPRBLD)
	{
		rebuild = TRUE;
	}

	frm->frmflags &= ~fdDISPRBLD;

	/*
	**	If not the previous displayed frame, clear the screen,
	**	before continuing the display loop. This avoids the possibilty
	**	that parts of the previous frame window are left around.
	**
	**	If popup, don't erase screen.
	*/
	if (frm != IIprev)
	{
		if (first)
		{
			if (IIprev)
			{
				pname = IIFRpfnPrevFormName;
			}

			/*
			**  Do check to to see if we are displaying
			**  the same logical form again (it may not be
			**  be the same physically).  If it is the
			**  same logical form (based on the name), then
			**  don't clear the screen.
			*/
			if (!(frm->frmflags & fdISPOPUP) &&
				STcompare(frm->frname, pname) != 0)
			{
				IIclrscr();
			}
		}
		else
		{
			/*
			**  Clear the screen if previous form was
			**  not a popup (i.e., a fullscreen).
			*/
			if (!IIFRpfwp)
			{
				IIclrscr();
			}

			/*
			**  Check to see if menu needs to be reformatted.
			*/
			if (IIstkfrm->fdmunum > 0 && IIFRmrMenuReformat() &&
				IIstkfrm->fdrunmu->mu_active)
			{
				IIstkfrm->fdrunmu->mu_flags |= MU_REFORMAT;
			}
		}
		IIprev = frm;

		/*
		**  If we are displaying a popup, redisplay everything to
		**  make sure the screen looks correct.
		*/
		if ((IIstkfrm->fdrunfrm->frmflags & fdISPOPUP) &&
			(!first || rebuild))
		{
			IIredisp();
		}
		else if (!first)
		{
			FTsyncdisp(frm, FALSE);
		}

		if (IIstkfrm->fdrunnxt)
		{
			IIstkfrm->fdrunnxt->fdrunfrm->frmflags &= ~fdDISPRBLD;
		}
	}

	evcb = IIfrscb->frs_event;
	/*
	** 'continued' is only modified by iiresnext() which works with
	** IIfrscb AFTER IIfrscb->frs_event is copied to IIstkfrm->fdevcb.
	** Therefore, need to copy the value from IIfrscb to IIstkfrm before
	** entirely wiping out prior contents of IIfrscb with this frame's
	** event control block.
	**
	** 'timeout' is being copied since only global event control block's
	** value is guaranteed correct (in the original implementation.)
	*/
	IIstkfrm->fdevcb->continued = evcb->continued;
	IIstkfrm->fdevcb->timeout = evcb->timeout;
	MEcopy((PTR) IIstkfrm->fdevcb, (u_i2) sizeof(FRS_EVCB), (PTR) evcb);

	evcb->entry_act = FALSE;	/* No longer IN an entry activation. */
	if (frm->frres2->formmode != fdrtQRY)
	{
	    /* Aggregate derivation evaluation allowed if not in query mode. */
	    evcb->eval_aggs = TRUE;
	    IIFDpadProcessAggDeps(frm);
	}

	/*
	**  If we exited from a submenu, put back this display loop's
	**  frskey activations.
	*/
	if (IIexted_submu)
	{
		IIresfrsk();
		IIexted_submu = FALSE;
	}

	for (;;)
	{
		form_mu = TRUE;

		/*
		**	If any menu commands were defined, display
		**	menu.  This also serves as the menu initialization
		**	the first time through.
		*/
		if (IIstkfrm->fdmunum > 0 && IIstkfrm->fdrunmu->mu_active)
		{
			runmu = IIstkfrm->fdrunmu;
			FTputmenu(runmu);
		}
		else
		{
			runmu = NULL;
			FTputmenu(NULL);
		}

		/*
		**  If we have a pending activation, fire that
		**  off.
		*/
		if (IIstkfrm->rfrmflags & RTACT_PENDING)
		{
			IIstkfrm->rfrmflags &= ~RTACT_PENDING;
			IIresult = IIstkfrm->intr_kybdev;
			return(RetSetCB(IIstkfrm->ret_kybdev));
		}

		/*
		** If 'continued' is set, then processing a 'resume next'
		** where the next operation is to go to the menu line,
		** or to process an FRSkey or menuitem.
		*/
		intrval = 0;
		if (evcb->continued)
		{
			switch (evcb->event)
			{
				case fdopMENU:
					IIstkfrm->rfrmflags |= RTRESMENU;
					break;

				case fdopFRSK:
				case fdopMUITEM:
					intrval = evcb->intr_val;
					break;
			}
			evcb->continued = FALSE;
			evcb->processed = TRUE;
		}
		if (intrval)
		{
			IIresult = intrval;
			return(RetSetCB(TRUE));
		}

		/*
		**	This is the call to the frame driver, the center
		**	of the "display loop."  The frame driver returns
		**	a status, which may be the command returned or
		**	a field that has been interrupted or an error.
		**	If an escape has been hit, this is a special case.
		**	Commands interrupts and field interrupts are as they
		**	defined by IIaddcomm() and IIactfld().
		*/

		if (IIstkfrm->rfrmflags & RTRESMENU)
		{
			IIstkfrm->rfrmflags &= ~RTRESMENU;
			if(IIstkfrm->fdmunum > 0 &&
				IIstkfrm->fdrunmu->mu_active)
			{
				IIredisp();
				IIresult = fdopMENU;
				/*
				**  Clear out event block.
				**  and set it to menu key.
 				**  Save and restore timeout value.
				*/
 				timeout = evcb->timeout;
				MEfill((u_i2) sizeof(FRS_EVCB), '\0', evcb);
				evcb->event = fdopMENU;
 				evcb->timeout = timeout;
			}
			else
			{
				IIFDerror(RTNOMURES, 0);
				IIresult = IItputfrm(frm);
			}
		}
		else
		{
	 		IIresult = IItputfrm(frm);
		}

		IIsetferr((ER_MSGID) 0); /* clear FRS error flag in loop */

		/*
		**	Check the result of the return from the frame
		**	driver and IItputfrm().
		**
		**	The result of either IItputfrm() or the menu driver
		**	is placed in the global variable IIresult to be
		**	tested for each of the activate statements.
		*/
		switch (IIresult)
		{
		  /*
		  **	ESCAPE means either a menu or exit the frame
		  **	gracefully.  If there is a menu, drive the menu
		  **	and return the interrupt value for the menu item.
		  */
		  case	fdopESC:
		  case  fdopMENU:
			if ((IIstkfrm->fdmunum > 0 &&
				IIstkfrm->fdrunmu->mu_active) ||
				anyfrskeys(IIstkfrm->fdfrskey))
			{
				if (evcb->event == fdopMENU)
				{
					if (runmu)
					    runmu->mu_flags |= MU_FRSFRM;
					intrval = FTgetmenu(runmu, evcb);
					if (intrval == 0)
					{
						/*
						** User has typed CR to a
						** menu prompt.  Return to
						** the form.  But we need
						** to set special flag so
						** alerter event doesn't
						** get checked since
						** we haven't activated
						** anything.
						*/
						frm->frmflags |= fdNO_MI_SEL;
						break;
					}
					else if (intrval == fdopTMOUT)
					{
						IIresult = IIFRgtoGetTOact();

						/*
						**  Check for (alerter) event,
						**  if necessary.
						*/
						if (IIstkfrm->alev_intr &&
							IIFDaeAlerterEvent())
						{
							IIstkfrm->rfrmflags |= RTACT_PENDING;
							IIstkfrm->intr_kybdev = IIresult;
							IIresult = IIstkfrm->alev_intr;
							IIstkfrm->ret_kybdev = TRUE;
						}
						return(RetSetCB(TRUE));
					}
				}
				if (evcb->event != fdopFRSK)
				{
					if (IImumatch(runmu, evcb, &dummy)
						!= OK)
					{
						break;
					}
					intrval = evcb->intr_val;
				}
				act = IIavchk(IIstkfrm);

				/*
				**  Set processed flag to be
				**  on in case no field activations
				**  will take place.
				*/
				evcb->processed = TRUE;

				if (act == 0)
				{
					IIresult = intrval;
					/*
					**  Check for (alerter) event,
					**  if necessary.
					*/
					if (IIstkfrm->alev_intr &&
						IIFDaeAlerterEvent())
					{
						IIstkfrm->rfrmflags |= RTACT_PENDING;
						IIstkfrm->intr_kybdev = IIresult;
						IIresult = IIstkfrm->alev_intr;
						IIstkfrm->ret_kybdev = TRUE;
					}
					return(RetSetCB(TRUE));
				}
				if (act > 0)
				{
					/*
					**  Unset processed flag since
					**  a field activation will
					**  take place.
					*/
					evcb->processed = FALSE;

					evcb->intr_val = intrval;
					IIresult = act;
					/*
					**  Check for (alerter) event,
					**  if necessary.
					*/
					if (IIstkfrm->alev_intr &&
						IIFDaeAlerterEvent())
					{
						IIstkfrm->rfrmflags |= RTACT_PENDING;
						IIstkfrm->intr_kybdev = IIresult;
						IIresult = IIstkfrm->alev_intr;
						IIstkfrm->ret_kybdev = TRUE;
					}
					return(RetSetCB(TRUE));
				}

				break;
			}
			/* else, ERROR!; fall through */

		  /*
		  **	ERROR! Abort the display loop.
		  */
		  case	fdopERR:
			IIresult = 0;
			return(RetSetCB(FALSE));

		  case fdopALEVENT:
			IIresult = IIstkfrm->alev_intr;
			evcb->processed = TRUE;
			/*
			**  Set event to be nothing so that inquire on
			**  last_command will return undefined since
			**  an alerter event is NOT a forms command.
			*/
			IIfrscb->frs_event->event = fdNOCODE;
			return(RetSetCB(TRUE));
		  /*
		  **	Timed out.  IIresult is the activate timeout value.
		  **	Returning FALSE if no activate timeout AND no
		  **	menuitems.  This is so that timeout will break out
		  **	of the display loop even if no activates exist
		  **	(slight overkill since there may be some activates
		  **	even though not activate menuitem's.)
		  */
		  case  fdopTMOUT:
			IIresult = IIFRgtoGetTOact();
			if ((IIresult == 0) && (IIstkfrm->fdmunum == 0 ||
				IIstkfrm->fdrunmu->mu_active == 0))
			{
				retval = FALSE;
			}
			else
			{
				retval = TRUE;
			}
			/*
			**  Check for (alerter) event,
			**  if necessary.
			*/
			if (IIstkfrm->alev_intr && IIFDaeAlerterEvent())
			{
				IIstkfrm->rfrmflags |= RTACT_PENDING;
				IIstkfrm->intr_kybdev = IIresult;
				IIresult = IIstkfrm->alev_intr;
				IIstkfrm->ret_kybdev = retval;
			}
			return(RetSetCB(TRUE));
		  
		  /*
		  **	Anything else should be a legal interrupt.
		  */
		  default:
			if (evcb->event == fdopFRSK)
			{
				act = IIavchk(IIstkfrm);
				/*
				**  If there is a field interrupt,
				**  save frs key interrupt in event
				**  block and set IIresult to field
				**  interrupt.  Otherwise, if
				**  something went wrong (< 0) then
				**  cancel the frs key selection by
				**  breaking out.  If equal to zero,
				**  just proceed with frs key selection.
				**  Set processed flag in case no
				**  field activation takes place.
				*/
				evcb->processed = TRUE;
				if (act > 0)
				{
					/*
					**  Reset processed flag since
					**  there is a field activation.
					*/
					evcb->processed = FALSE;
					evcb->intr_val = IIresult;
					IIresult = act;
				}
				else if (act < 0)
				{
					break;
				}
			}
			/*
			**  Check for (alerter) event, if necessary.
			**  Don't check if we are in middle of a
			**  column/scroll activation combination.
			*/
			if (frm->frmflags & fdIN_SCRLL)
			{
				frm->frmflags &= ~fdIN_SCRLL;
			}
			else
			{
				if (IIstkfrm->alev_intr && IIFDaeAlerterEvent())
				{
					IIstkfrm->rfrmflags |= RTACT_PENDING;
					IIstkfrm->intr_kybdev = IIresult;
					IIresult = IIstkfrm->alev_intr;
					IIstkfrm->ret_kybdev = TRUE;
				}
			}
			return(RetSetCB(TRUE));
		}
	}
}


i4
IIavchk(RUNFRM *rfrm)
{
	FRAME	*frm;
	i4	isqry = FALSE;

	frm = rfrm->fdrunfrm;
	if (frm->frmode == fdcmBRWS || frm->frfldno == 0)
	{
		return(0);
	}
	if (rfrm->fdrunmd == fdrtQRY)
	{
		isqry = TRUE;
	}

	return(FDavchk(frm, IIfrscb, isqry));
}

/*{
** Name:	RetSetCB	- Set frame evcb, then return passed value.
**
** Description:
**	Copy the global event control block (IIfrscb->frs_event) to the
**	current frame's evcb.  This saves the current frame's event state
**	in the case that this frame gets pushed down on the active frame
**	list.  Then return the passed in value.  This is called by IIrunform
**	in place of simple return statements; return(TRUE) is replaced by
**	return(RetSetCB(TRUE)).
**	
** Inputs:
**	retval	Value to return.
**
** Outputs:
**
** Returns:
**	retval
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
**	22-nov-88 (bruceb)	Written.
*/
static i4
RetSetCB(i4 retval)
{
    IIfrscb->frs_event->lastcmd = IIfrscb->frs_event->event;
    IIfrscb->frs_event->eval_aggs = FALSE;
    MEcopy((PTR) IIfrscb->frs_event, (u_i2) sizeof(FRS_EVCB),
	(PTR) IIstkfrm->fdevcb);
    return(retval);
}



/*{
** Name:	IIFRaeAlerterEvent - Register "alerter" event block.
**
** Description:
**	Runtime interface routine for registering an alerter event block.
**
**	Note that this support needs to change when async libq comes
**	into existence.  When that occurs, a mechanism for poking
**	the FRS when an async (alerter) event is delivered must be
**	invented.
**
** Inputs:
**	intr_val	Interrupt value to return when (alerter) event
**			is discovered by the FRS.
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
**	03/19/91 (dkh) - Initial version.
*/
i4
IIFRaeAlerterEvent(i4 intr_val)
{
/*
	if (IIissubmu())
	{
		IIFRsaiSubmuAeIntr = intr_val;
	}
	else
*/
	{
		IIstkfrm->alev_intr = intr_val;
	}

	return(TRUE);
}
