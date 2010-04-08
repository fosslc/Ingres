/*
**	putfrm.c
**
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:	putfrm.c - Put a form to screen and let user enter values.
**
** Description:
**	File contains routines for doing final preparations for displaying
**	a form on the screen and finally letting users enter values
**	into fields.
**	Routines defined in this file are:
**	- FDchkscroll - Check for table field scrolling.
**	- FDinitsp - Special form initialization.
**	- FDputfrm - Put form to screen and run it.
**	- FDredisp - Redisplay a form.
**	- FDrstfrm - Restore current form context.
**
** History:
**	02/15/87 (dkh) - Added header.
**	19-jun-87 (bruceb)	Code cleanup.
**	08/14/87 (dkh) - ER changes.
**	08/26/87 (dkh) - Changes for 8000 series error numbers.
**	09-nov-87 (bruceb)
**		Properly handle addition of default values to
**		scrolling and reverse fields.
**	11/25/87 (dkh) - Fixed default value overflow problem.
**	05-feb-88 (bruceb)
**		FDaddns: calculation of dslen (based on db_length
**		minus sizeof(DB_TEXT_STRING) plus 1) doesn't work
**		on systems where the structure sizes are padded
**		rather than being the sum of the sizes of the contents.
**		Use (db_length - DB_CNTSIZE) instead.
**	05/05/88 (dkh) - Updated for Venus.
**	02-dec-88 (bruceb)
**		Retain timeout value from event control block when
**		about to run a frame.
**	02-mar-89 (bruceb)
**		Return entry activation interrupt value up to the user if
**		this is a new location and EA is enabled for the application.
**	14-mar-89 (bruceb)
**		Save the event value in FDputfrm().  If returning up
**		for entry activation, restore that event value.  Note
**		that IIrunform() will copy that event value into lastcmd.
**	07-jul-89 (bruceb)
**		Use an IIFDrfcb function pointer in FDputfrm() in place
**		of the (removed) eact function pointer.
**	14-jul-89 (bruceb)
**		If form in fill, update or read mode, FDinitsp() will
**		evaluate any constant derivations for simple fields.
**		Data will be displayed only in validated fields/columns
**		(for update/read mode startup), if field/column is either
**		a derived field or a derivation source field.
**	08/03/89 (dkh) - Updated with interface change to adc_lenchk().
**	10/23/90 (dkh) - Rearranged code so that screen is not updated
**			 until we know that no entry activations will
**			 occur.  This prevents spurious screen updates
**			 when a cascade of entry activations is being
**			 processed.
**	08/18/91 (dkh) - Fixed bug 38752.  Put in some workarounds to
**			 make things work for the compressed view of
**			 a vision visual query.  The real fix should
**			 be to change vision to create more sociable forms.
**	05/02/92 (dkh) - Modified includes to be more efficient for
**			 non-pc environments.  The pc changes cause
**			 unnecessary stuff to get dragged in.
**	07/01/92 (dkh) - Added support for input edit masks.
**	06/18/93 (dkh) - Fixed bug 51506.  Fields that perform input
**			 masking will now be properly cleaned up when
**			 the form is displayed in fill mode.
**	07/16/93 (dkh) - Fixed bug 43978.  Before entry activation
**			 causes FDputfrm() to return, make sure that
**			 any pending screen updates are forced to the
**			 terminal via a call to FDmodedraw().
**	07/19/93 (dkh) - Changed call to adc_lenchk() to match
**			 interface change.  The second parameter is
**			 now a i4  instead of a bool.
**	08/31/93 (dkh) - Fixed up compiler complaints due to prototyping.
**	23-apr-96 (chech02)
**	               - Added function prototypes for windows 3.1 port.
**  01-apr-97 (rodjo04)
**            FDaddns: If the flag fdKPREV is set and there was
**            a previous valus set, then keep the previous value.
**	19-dec-97 (kitch01)
**			Bug 87710. Change FDputfrm to only refresh tablefield if
**			scroll was a SCRTO.
**  27-Jul-98 (islsa01)
**      Passing TRUE instead of FALSE value to FDmodedraw function, which
**      fixes the similar bug 74500.
**  02-sep-99 (kitch01)
**    The above fix for bug 74500 causes bug 94962. By passing TRUE to
**    FDmodedraw the entire screen is redrawn. This can cause the screen
**    to flash when the frame is larger than the screen.
**	The above fix for bug 74500 causes bug 94962. By passing TRUE to
**	FDmodedraw the entire screen is redrawn. This can cause the screen
**	to flash when the frame is larger than the screen.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**  10-aug-2000 (rigka01)
**    revert to fix for bug 74500 that causes bug 94962.  The fix to bug
**    94962 is cosmetic and introduces bug 101316/102136.  
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	26-Aug-2009 (kschendel) b121804
**	    Remove function defns now in headers.
**/


# include	<compat.h>
# include	<me.h>
# include	<st.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<int.h>
# include	<cm.h>
# include	<ft.h>
# include	<fmt.h>
# include	<adf.h>
# include	<frame.h>
# include	<frserrno.h>
# include	<menu.h>
# ifdef PMFE
# include	<runtime.h>
# else
# include	<frsctblk.h>
# endif
# include	"fdfuncs.h"
# include	<er.h>
# include	<erfi.h>
# ifdef PMFE
# include	<rtvars.h>
# endif

GLOBALREF	FRAME	*frcurfrm;

/*
**	Since this is the main routine we pull in for a forms program
**	this is where we can declare routines needed for FT. The arguments
**	are dummy arguments.	(john) 1/10/86
*/
static
FDsatisfyFT()
{
	bool	dummy;

	FDvalidate(NULL, 0, 0, 0, 0, &dummy);
	FDfieldval(NULL, 0, 0, 0, 0, 0);
}


/*
**	PUTFRM.c  -  The Frame Driver
**
**	These two routines, FDinitfrm() and FDputfrm(), constitute the upper
**	level interface to the frame driver.  Fdinitfrm() initializes the
**	frame and displays it.	Fdputfrm() displays iterative changes in the
**	frame driver and interfaces with the user.
**
**	FDINITFRM  -  Initialize and display frame.
**	Arguments:  frm -      pointer to frame to be initialized
**		    redisp -   flag to indicate if the frame is to be cleared
**		    dispmode - There are 3 modes to run frame
**				fdmdADD: fill in frame and check data
**				fdmdFILL: fill in frame and don't check data
**				fdmdUPD: edit data already filled in
**
**	FDPUTFRM  -  Run the frame in what ever mode is indicated in the frame
**	Arguments:  frm - frame to be driven
**
**	History:  JEN - 1 Nov 1981  (written)
**		  NML -20 Oct 1983  Keep previous values now work for fields
**				    -added in dkh's changes from VMS.
**		  NCG  - 23 Mar 1984  added  query only fields.
**		  DRH  22 Apr 1985  Added additional calls to FDdmpcur and new
**				    calls to FDdmpmsg for .scr changes
**		12/23/86 (dkh) - Added support for new activations.
**
*/

i4	FDaddinit();
i4	FDaddns();
i4	FDersfld();
#ifdef WIN16 
static VOID	FDmodedraw(FRAME *,nat,nat);
#endif
static VOID	FDmodedraw();
static i4	FDddDispData();

FUNC_EXTERN	i4	FDfixstate();
FUNC_EXTERN	i4	FDfldtrv();
FUNC_EXTERN	i4	FDclr();
FUNC_EXTERN	FIELD	*FDfldofmd();
FUNC_EXTERN	FLDHDR	*FDgethdr();
FUNC_EXTERN	FLDTYPE	*FDgettype();
FUNC_EXTERN	FLDVAL	*FDgetval();
FUNC_EXTERN	STATUS	IIFVedEvalDerive();
FUNC_EXTERN	VOID	IIFDidfInvalidDeriveFld();
FUNC_EXTERN	VOID	IIFDudfUpdateDeriveFld();
FUNC_EXTERN	i4	FDputdata();
FUNC_EXTERN	FRAME	*FDFTgetiofrm();

GLOBALREF	FRS_RFCB	*IIFDrfcb;

# define	ESC	033

static	i4	curdispmode = 0;
static	i4	scrollval = 0;
static	i4	tblscrolled = -1;
static  bool modedrawdone = FALSE;

/*
** Return current display mode, as frm->frmode is always fdcmINSRT from
** the Runtime FRS.
** History:
**	23-mar-1984	- Added for QueryOnly fields/columns (ncg)
*/

FDsetscroll(val)
i4	val;
{
	scrollval = val;
}




/*{
** Name:	FDchkscroll - Check for a table field scroll.
**
** Description:
**	Return static indicator that a table field scroll is to
**	happen as a result of a resume next.  This is possible
**	when user hits the down key at the bottom of a table field
**	(or up key at the top of a table field).  The value returned
**	is the interrupt value for the scroll activation.  It
**	is set when a resume next is processed, via a call to
**	FDsetscroll().  The static indicator is cleared before
**	exiting from this routine.
**
** Inputs:
**	None.
**
** Outputs:
**	Returns:
**		0	If no activation is pending.
**		val	Scroll activation interrupt value.
**	Exceptions:
**		None.
**
** Side Effects:
**	The indicator is cleared before exiting the routine.
**
** History:
**	02/15/87 (dkh) - Added procedure header.
*/
i4
FDchkscroll()
{
	i4	val = 0;

	if (scrollval != 0)
	{
		val = scrollval;
		scrollval = 0;
	}
	return(val);
}


i4
FDcurdisp()
{
	return(curdispmode);
}



/*{
** Name:	FDinitsp - Special form initialization
**
** Description:
**	Initialize a form in preparation for running it.  This
**	is one of the last steps in getting a form ready to
**	run in a display loop.  Much of the work is based on
**	what to do to the fields depending on the
**	passed in display mode.
**	Modes supported are:
**	- fdmdADD - Fill mode, clear display buffers and set to system
**		    default or default value.
**	- fdmdUPD - Update/read mode, display values in fields.
**	- fdmdFILL - Query mode, clear display buffers and
**		     set field values to system defaults.
**	Note that only REGULAR fields are processed.  There is
**	another mechanism for table fields.
**
** Inputs:
**	frm		Form to initialize.
**	redsip		Flag for clearing the screen first.
**	dispmode	Display mode to initialize fields for.
**
** Outputs:
**	Returns:
**		TRUE	When initialization completed.
**		FALSE	If an unknown display mode passed in.
**	Exceptions:
**		None.
**
** Side Effects:
**
** History:
**	02/17/87 (dkh) - Added procedure header.
*/
i4
FDinitsp(frm, redisp, dispmode)		/* INITIALIZE FRAME (Special): */
reg FRAME	*frm;		/* pointer to frame to initialize */
i4		redisp;		/* flag to clear screen */
i4		dispmode;	/* mode of frame display */
{
	reg i4		fldno;		/* index to current field */
	reg FIELD	*fld;		/* pointer to current flag */
	FRAME		*ofrm;
	FLDHDR		*hdr;
	FLDTYPE		*type;
	FLDVAL		*val;
	i4		ret;

	frm->frcurfld = 0;		/* initialize frame */
	frm->frchange = FALSE;
	frcurfrm = frm;			/* set current flag */
	curdispmode = dispmode;		/* set current display mode */
	tblscrolled = -1;

	if (redisp)
		FTclear();

	frm->frcurfld = 0;	/* set the current field to the first field */

	switch (dispmode)
	{
	  case fdmdADD:		/* fill in field according to the ADD mode */
		/* for each field in the frame */
		for (fldno = 0; fldno < frm->frfldno; fldno++)
		{
		    fld = frm->frfld[fldno];
		    /*
		    ** Table fields were cleared when fdputtbl() was
		    ** called earlier.
		    */
		    if (fld->fltag != FTABLE)
		    {
			hdr = FDgethdr(fld);
			if ((hdr->fhd2flags & fdDERIVED)
			    && (hdr->fhdrv->drvflags & fhDRVCNST))
			{
			    /* Clear out the change bit flags. */
			    IIFDfccb(frm, hdr->fhdname);

			    type = FDgettype(fld);
			    val = FDgetval(fld);
			    if ((IIFVedEvalDerive(frm, hdr, type, val->fvdbv))
				== FAIL)
			    {
				IIFDidfInvalidDeriveFld(frm, fld, BADCOLNO);
			    }
			    else
			    {
				IIFDudfUpdateDeriveFld(frm, fld, BADCOLNO);
			    }
			}
			else
			{   
			    FDaddinit(frm, fldno, FT_UPDATE, FT_REGFLD,
				(i4) 0, (i4) 0);
			}
		    }
		}
		/* for each non-seq field in the frame */
		for (fldno = 0; fldno < frm->frnsno; fldno++)
		{
		    FDaddns(frm, fldno, FT_DISPONLY, FT_REGFLD,
			(i4) 0, (i4) 0);
		}
		break;

	  case fdmdUPD:	/* fill in fields with data already in storage areas */
		ofrm = FDFTgetiofrm();
		FDFTsetiofrm(frm);

		FDFTsmode(FT_UPDATE);

		/* for each field */
		for (fldno = 0; fldno < frm->frfldno; fldno++)
		{
		    fld = frm->frfld[fldno];
		    hdr = FDgethdr(fld);
		    if ((fld->fltag != FTABLE) && (hdr->fhd2flags & fdDERIVED)
			&& (hdr->fhdrv->drvflags & fhDRVCNST))
		    {
			/* Clear out the change bit flags. */
			IIFDfccb(frm, hdr->fhdname);

			type = FDgettype(fld);
			val = FDgetval(fld);
			if ((IIFVedEvalDerive(frm, hdr, type, val->fvdbv))
			    == FAIL)
			{
			    IIFDidfInvalidDeriveFld(frm, fld, BADCOLNO);
			}
			else
			{
			    IIFDudfUpdateDeriveFld(frm, fld, BADCOLNO);
			}
		    }
		    else
		    {
			/*
			** Display only validated data if involved in
			** derivations, else all data.
			*/
			ret = FDddDispData(frm, fldno);
			if ((fld->fltag != FTABLE) && (ret == FALSE))
			{
			    /*
			    ** The field value wasn't displayed, so
			    ** erase the display.
			    */
			    FDersfld(frm, fldno, FT_UPDATE, FT_REGFLD,
				(i4) 0, (i4) 0);
			}
		    }
		}

		FDFTsmode(FT_DISPONLY);

		/* for each non-sequenced field */
		for (fldno = 0; fldno < frm->frnsno; fldno++)
		{
		    FDersfld(frm, fldno, FT_DISPONLY, FT_REGFLD,
			(i4) 0, (i4) 0);
		    /* display data */
		    FDfldtrv(fldno, (natfunction)FDputdata, FDDISPROW);
		}

		FDFTsetiofrm(ofrm);
		break;

	  case fdmdFILL:		/* clear all fields */
		for (fldno = 0; fldno < frm->frfldno; fldno++)
		{
		    fld = frm->frfld[fldno];
		    if (fld->fltag != FTABLE)
		    {
			FDersfld(frm, fldno, FT_UPDATE, FT_REGFLD,
			    (i4) 0, (i4) 0);
		    }
		}
		for (fldno = 0; fldno < frm->frnsno; fldno++)
		{
		    FDersfld(frm, fldno, FT_DISPONLY, FT_REGFLD,
			(i4) 0, (i4) 0);
		}
		break;

	  default:
		IIFDerror(FDBADMODE, 1, frm->frname);
    		return(FALSE);
	}
	return(TRUE);
}

FDgetprev(frm, dispmode)
FRAME	**frm;
i4	*dispmode;
{
	*frm = frcurfrm;		/* get current flag */
	*dispmode = curdispmode;	/* get current display mode */
}



/*{
** Name:	FDrstfrm - Restore current form context.
**
** Description:
**	Restore "frcurfrm" and "curdispmode" from the passed in
**	values.  This is useful when exiting an inner display
**	loop to an outer one.  The sense of the current form and
**	display mode is restored to that of the outer display
**	loop so that the outer loop can run again.
**
** Inputs:
**	frm		Form to assign to "frcurfrm".
**	dispmode	Display mode to assign to "curdispmode".
**
** Outputs:
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
VOID
FDrstfrm(frm, dispmode)		/* RESET FRAME: */
reg FRAME	*frm;		/* pointer to frame to initialize */
i4		dispmode;	/* mode of frame display */
{
	frcurfrm = frm;		/* set current flag */
	curdispmode = dispmode;		/* set current display mode */
}




/*{
** Name:	FDredisp - Redisplay a form.
**
** Description:
**	Redisplay a form on the terminal screen, keeping the
**	current field visible.  The form to be redisplayed
**	must be the same as "frcurfrm".  Calls FTmodedraw()
**	to do the work.
**
** Inputs:
**	frm	Form to redisplay.
**
** Outputs:
**	Returns:
**		TRUE	Form was successfully displayed.
**		FALSE	Form to redisplay not same as "frcurfrm".
**	Exceptions:
**		None.
**
** Side Effects:
**	Screen may be cleared before redisplay takes place.
**	Various messages may be dumped to the testing files if
**	testing mode is enabled.
**
** History:
**	02/17/87 (dkh) - Added procedure header.
*/
i4
FDredisp(frm)		/* REDISPLAY FRAME: */
reg FRAME	*frm;		/* frame to be displayed */
{
	reg	i4	fldno = -1;

	if (frm != frcurfrm)	/* if not the frame initialized, ERROR */
	{
		IIFDerror(FRMNOINIT, 1, frm->frname);
		return ((i4) FALSE);
	}
	if (frm->frfldno > 0)
	{
		fldno = FDstart(frm, &frm->frcurfld);
	}

	FDmodedraw(frm, fldno, TRUE);

	FDdmpmsg(ERx("FORM REDISPLAYED\n"));
	FDdmpcur();

	return((i4) TRUE);
}




/*{
** Name:	FDputfrm - Put form to screen and run it.
**
** Description:
**	This is the entry point (in the FRAME layer) for running
**	a form in a display loop.  Calls FTrun() to handle screen
**	manipulation only if there is a field to place cursor
**	on.  If not, just pretend user hit the menu key by
**	returning "fdopMENU" to caller.  If "fdopSCRUP" or
**	"fdopSCRDN" is resturend by FTrun, then save the current
**	field in "tblscrolled".  This is a special hook for
**	the IBM version of the system to only refresh the
**	table field and not the entire form.
**
** Inputs:
**	frm	Form to run.  Must be the same as "frcurfrm" (set
**		by a call to FDinitsp()).
**	frscb	A forms system control block to use in running
**		the form.
**
** Outputs:
**	Returns:
**		fdopERR		Form to run not same as "frcurfrm".
**		fdopMENU	No fields to place curson on.  User
**				is restricted to the menu line.
**		cmd		Interrupt values for various activations
**				and forms system commands returned by FTrun().
**	Exceptions:
**		None.
**
** Side Effects:
**	Field values may have been updated by the user.
**
** History:
**	02/17/87 (dkh) - Added procedure header.
**	17-aug-91 (leighb) DeskTop Porting Change:
**		Cleanup includes;
**		Call special routine to handle forms with no fields.
**  19-dec-97 (kitch01)
**		Add fdopSCRTO to list of situations where only tablefield
**		is refreshed rather than entire screen.
**  27-Jul-98 (islsa01)
**      Passing TRUE instead of FALSE value to FDmodedraw function, which
**      fixes the similar bug 74500.
**  02-sep-99 (kitch01)
**    The above fix for bug 74500 causes bug 94962. By passing TRUE to
**    FDmodedraw the entire screen is redrawn. This can cause the screen
**    to flash when the frame is larger than the screen.
**  10-aug-2000 (rigka01)
**    revert to fix for bug 74500 that causes bug 94962.  The fix to bug
**    94962 is cosmetic and introduces bug 101316/102136.  
*/
i4
FDputfrm(frm, frscb)		/* PUT FRAME: */
reg FRAME	*frm;		/* frame to be displayed */
FRS_CB		*frscb;
{
	reg	i4	code;	/* holds returned code from the mode drivers */
	i4		fldno = -1;
	i4		FTmode;
	i4		timeout = frscb->frs_event->timeout;
	bool		new_fld = FALSE;
	FIELD		*fld;
	FLDHDR		*hdr;
	i4		event = frscb->frs_event->event;
	i4		row;
	TBLFLD		*tblfld;
	bool		eval_aggs = frscb->frs_event->eval_aggs;

	MEfill((u_i2) sizeof(struct frs_event_cb), EOS, frscb->frs_event);
	frscb->frs_event->timeout = timeout;
	frscb->frs_event->eval_aggs = eval_aggs;

	if (frm != frcurfrm)	/* if not the frame initialized, ERROR */
	{
		IIFDerror(FRMNOINIT, 1, frm->frname);
		return (fdopERR);
	}
	IIFTsnfSetNbrFields(frm->frfldno);
	if (frm->frfldno > 0)
	{
		fldno = FDstart(frm, &frm->frcurfld);
		if (fldno == -1)
		{
			FDmodedraw(frm, fldno, FALSE);
			frscb->frs_event->event = fdopMENU;
			/*
			** Don't modify the dataset row in this instance.
			** Wish to properly determine later when to
			** generate an EA based on the original location.
			*/
			frm->frres2->savetblpos = TRUE;
			return (fdopMENU);
		}

		/* Is entry activation enabled for the application? */
		if (frscb->frs_actval->act_entry)
		{
			fld = frm->frfld[fldno];
			hdr = FDgethdr(fld);

			/* Is entry activation defined for this location? */
			if (hdr->fhenint != 0)
			{
				if (frm->frres2->origfld != fldno)
				{
					new_fld = TRUE;
					frm->frres2->origfld = fldno;
				}
				if (fld->fltag == FTABLE)
				{
					tblfld = fld->fld_var.fltblfld;
					row = tblfld->tfcurrow;
					if (row < 0)
					{
						row = 0;
					}
					else if (row > tblfld->tflastrow)
					{
						row = tblfld->tflastrow;
					}

					if ((new_fld) || (frm->frres2->origcol
						!= tblfld->tfcurcol)
						|| (frm->frres2->origrow != row)
						|| ((*IIFDrfcb->newDSrow)()))
					{
						/*
						** If a new field, or the
						** same field but a different
						** location in the
						** field, then set the row/col
						** values.
						*/
						new_fld = TRUE;
						frm->frres2->origrow = row;
						frm->frres2->origcol =
							tblfld->tfcurcol;
						/*
						**  Dataset row pointer is set
						**  up in tbacc.
						*/
					}
				}

				/*
				** If new field then return the entry activation
				** value.  Restore event value from cause of
				** initial return to user code.
				*/
				if (new_fld)
				{
					frscb->frs_event->entry_act = TRUE;
					frscb->frs_event->event = event;

					/*
					**  Force a screen update if we are
					**  dealing with a vision compress
					**  view frame.  This allows the
					**  user to see portions of the
					**  compressed visual query that
					**  is below the screen.
					**
					**  A much better fix is to fix
					**  vision to build forms similar
					**  to the application diagram
					**  that does not need to scroll.
					**
					**  We now always call FDmodedraw
					**  to make sure any pending screen
					**  updates are forced to the terminal.
					**  The text of the former if statement
					**  is kept in case there is need to
					**  do something special for the
					**  compressed view in vision.  None
					**  anticipated for the moment.
					if (frm->frmflags & fdVIS_CMPVW)
					{
					*/
						FDmodedraw(frm, fldno, FALSE);
					/*
					}
					*/

					return(hdr->fhenint);
				}
			}
		}

		/*
		**  Added so that when a table field scrolls in IBM land,
		**  only the table field is refreshed, not the entire screen.
		*/

		if (tblscrolled != -1)
		{
			FTmarkfield(frm, tblscrolled, FT_UPDATE, FT_TBLFLD,
				(i4) -1, (i4) -1);
			FTrefresh(frm, (MENU) NULL);
			tblscrolled = -1;
		}
		else
		{
			FDmodedraw(frm, fldno, TRUE);
		}
	}
	else
	{
		FDmodedraw(frm, fldno, FALSE);
		FDdmpmsg(ERx("REDRAWING FRAME\n"));
		FDdmpcur(frm);
#ifdef	PMFE
		/* Call special routine to handle forms with no fields.
		** It will handle input if there is a keystroke handler
		** (loaded by IIFRkeystroke() in iiforms.c)
		** and Ring Menus are being used.  Otherwise it will
		** just return fdopMENU's (as in the #else below).
		*/
		/* This is a candidate to make into an Ingres-wide change
		** at some future time - it involves only a little overhead.
		*/
		return(FTrunNoFld(IIstkfrm->fdrunmu, IIfrscb->frs_event));
#else
		frscb->frs_event->event = fdopMENU;
		return(fdopMENU);
# endif
	}


	FTmode = FDcurdisp();
	if (frm->frmode == fdcmBRWS)
	{
		FTmode = fdmdBROWSE;
	}

	code = FTrun(frm, FTmode, frm->frcurfld, frscb);

	/*
	**  Added so that if a table field is scrolled in IBM land,
	**  only the table field is refreshed and not the entire screen.
	*/

	if (code == fdopSCRUP || code == fdopSCRDN || code == fdopSCRTO)
	{
		tblscrolled = frm->frcurfld;
	}

	frm->frres2->origfld = frm->frcurfld;
	fld = frm->frfld[frm->frcurfld];
	if (fld->fltag == FTABLE)
	{
	    /*
	    ** savetblpos is set by FT if position has changed during
	    ** movement resulting in the return of a scroll command.
	    ** If the scroll command fails (e.g. last row of an 'update'
	    ** mode table field), still need to trigger an EA if the
	    ** location has changed.
	    */
	    if (frm->frres2->savetblpos == FALSE)
	    {
		tblfld = fld->fld_var.fltblfld;
		row = tblfld->tfcurrow;
		if (row < 0)
		{
		    row = 0;
		}
		else if (row > tblfld->tflastrow)
		{
		    row = tblfld->tflastrow;
		}
		frm->frres2->origrow = row;
		frm->frres2->origcol = tblfld->tfcurcol;
	    }
	    frm->frres2->savetblpos = FALSE;
	    /*
	    ** Dataset row pointer is saved up in tbacc.
	    ** Do so regardless of whether or not FT set the row and
	    ** column values--still need the current ds row.
	    */
	}

	return (code);
}

i4
FDaddinit(frm, fldno, disponly, fldtype, col, row)
FRAME	*frm;
i4	fldno;
i4	disponly;
i4	fldtype;
i4	col;
i4	row;
{
	/* FDaddinit() and FDaddns() are exactly alike */
	_VOID_ FDaddns(frm, fldno, disponly, fldtype, col, row);

	return(TRUE);
}

i4
FDaddns(frm, fldno, disponly, fldtype, col, row)
FRAME	*frm;
i4	fldno;
i4	disponly;
i4	fldtype;
i4	col;
i4	row;
{
	DB_DATA_VALUE	*ddbv;
	DB_DATA_VALUE	*dbv;
	DB_DATA_VALUE	sdbv;
	DB_TEXT_STRING	*text;
	FLDVAL		*val;
	FLDTYPE 	*type;
	FLDHDR		*hdr;
	TBLFLD		*tbl;
	FIELD		*fld;
	u_char		*ptr;
	i4		ocol;
	i4		orow;
	i4		i = 0;
	char		*string = NULL;
	i4		width;
	char		tmpstr[51]; /* larger than vifred's default string */
	i4		bytecnt;
	i4		dslen;
        char buf[80];

	fld = FDfldofmd(frm, fldno, disponly);

	type = FDgettype(fld);
	hdr = FDgethdr(fld);
	if (fldtype == FT_REGFLD)
	{
		val = FDgetval(fld);
	}
	else
	{
		tbl = fld->fld_var.fltblfld;
		ocol = tbl->tfcurcol;
		orow = tbl->tfcurrow;
		tbl->tfcurcol = col;
		tbl->tfcurrow = row;
		val = FDgetval(fld);
		tbl->tfcurcol = ocol;
		tbl->tfcurrow = orow;
	}

    /* fill in default fields */
    if (type->ftdefault != NULL && *(type->ftdefault) != EOS)
	{
        DB_DATA_VALUE   *prevddbv;
        DB_TEXT_STRING  *prevtext;
     
        prevddbv = val->fvdsdbv;
        prevtext = (DB_TEXT_STRING *) prevddbv->db_data;

        if ((prevtext->db_t_count > 0) && (hdr->fhdflags & fdKPREV))
        {
            STlcopy(prevtext->db_t_text,buf,prevtext->db_t_count);
            string = buf;
        }
        else
		string = type->ftdefault;
	}
	/* keep previous field if necessary */
	else if (!(hdr->fhdflags & fdKPREV))
	{
		string = ERx("");
	}

	if (string != NULL)
	{
		/*
		**  FIXME - need to make sure the default string
		**  is compatible with the edit mask, if it exists.
		**  by calling fmt_stvalid().  If OK, then use the
		**  result from fmt_stvalid() as the string to
		**  put into the field.
		*/
		ddbv = val->fvdsdbv;
		text = (DB_TEXT_STRING *) ddbv->db_data;
		/* determine space available for default string */
		dslen = ddbv->db_length - DB_CNTSIZE;

		if (hdr->fhdflags & fdREVDIR)
		{
			_VOID_ adc_lenchk(FEadfcb(), FALSE, ddbv, &sdbv);

			width = sdbv.db_length;
			/* Add to end of display buffer */
			ptr = text->db_t_text + width - 1;
			STcopy(string, tmpstr);
			FTswapparens(tmpstr);
			string = tmpstr;
			while (*string != EOS)
			{
				if (i + 1 > dslen)
				{
					IIFDerror(E_FI21D0_8656, 1,
						hdr->fhdname);
					break;
				}
				*ptr-- = *string++;
				i++;
			}
			if (i == 0)
			{
				text->db_t_count = 0;
			}
			else
			{
				/* Blank out rest of buffer */
				MEfill((u_i2)(width - i), ' ', text->db_t_text);
				text->db_t_count = width;
			}
		}
		else	/* LR field */
		{
			if (*string == EOS)
			{
				i4	fmttype;

				fmttype = type->ftfmt->fmt_type;

				/*
				**  We assume that things in range mode
				**  and query mode don't come through here.
				*/
				if (hdr->fhd2flags & fdUSEEMASK)
				{
					bool	dummy;
					u_char	buf[DB_GW4_MAXSTRING + 1];
					u_char	buf2[DB_GW4_MAXSTRING + 1];
					u_char	*from;
					u_char	*to;
					i4	fmttype;

					/*
					**  Blow away the field value since
					**  we don't support auto-duplicate
					**  for input mask fields.  This
					**  prevents old values from appearing
					**  for a form displayed in fill mode.
					*/
					adc_getempty(FEadfcb(), val->fvdbv);
					if (fmt_format(FEadfcb(), type->ftfmt,
						val->fvdbv, ddbv,
						(bool) TRUE) == OK)
					{
						fmttype = type->ftfmt->fmt_type;
						MEcopy((PTR) text->db_t_text,
							text->db_t_count,
							(PTR) buf);
						buf[text->db_t_count] = EOS;
						if (fmttype == F_ST ||
							fmttype == F_DT)
						{
						 (void) fmt_stvalid(type->ftfmt,
							buf,
							type->ftfmt->fmt_width,
							0, &dummy);
						}

						from = buf;
						to = text->db_t_text;
						dslen = STlength((char *) buf);
						if (dslen > (ddbv->db_length - DB_CNTSIZE))
						{
							dslen = ddbv->db_length - DB_CNTSIZE;
						}
						for (i = 0; i < dslen; i++)
						{
							/*
							**  No need to worry
							**  about double byte
							**  here since we are
							**  just moving bytes
							**  around and not
							**  interepeting them.
							*/
							*to++ = *from++;
						}
						text->db_t_count = dslen;
					}
					else
					{
						text->db_t_count = 0;
					}
				}
				else
				{
					text->db_t_count = 0;
				}
			}
			else
			{
				ptr = text->db_t_text;
				while (*string != EOS)
				{
					bytecnt = CMbytecnt(string);
					if (i + bytecnt > dslen)
					{
						IIFDerror(E_FI21D0_8656, 1,
							hdr->fhdname);
						break;
					}
					CMcpyinc(string, ptr);
					i += bytecnt;
				}
				text->db_t_count = i;
			}
		}
		FTfldupd(frm, fldno, disponly, fldtype, col, row);
	}

	return(TRUE);
}

i4
FDersfld(frm, fldno, disponly, fldtype, col, row)
FRAME	*frm;
i4	fldno;
i4	disponly;
i4	fldtype;
i4	col;
i4	row;
{
	FDclr(frm, fldno, disponly, fldtype, col, row);

/*
**	FTclrfld(frm, fldno, disponly, fldtype, col, row);
*/

	return(TRUE);
}


/*{
** Name:	FDmodedraw - Draw a form (or all visible forms) to the screen.
**
** Description:
**	This routine will call FTredraw to draw a particular form (or
**	if argument all is set, all visible forms) on the terminal
**	screen.  The mode of the current form is passed so that IBM
**	can know if the 3270 terminal should be locked or not.
**
**	For environments that do not support popup style forms, then
**	the "all" argument can be ignored since the current form is
**	all that can be seen at any one time.
**
** Inputs:
**	frm	Current (top) form.
**	mode	Mode of current form.
**	fldno	Current field number in current form.  Leave cursor
**		on this field unless it is a -1.  Indicating
**		that there is no current field.  Just draw the whole form.
**	all	Controls whether all visible or just the current
**		form should be drawn on the terminal.  If this is TRUE,
**		then "fldno" has no meaning.
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
**	Terminal screen information is redrawn.
**
** History:
**	05/06/88 (dkh) - Updated for Venus.
*/
static	VOID
FDmodedraw(frm, fldno, all)
FRAME	*frm;
i4	fldno;
i4	all;
{
	i4	mode;

	if (frm->frmode == fdcmBRWS)
	{
		mode = fdmdBROWSE;
	}
	else
	{
		mode = FDcurdisp();
	}
	FTredraw(frm, mode, fldno, all);
}


/*{
** Name:	FDddDispData - Call FDputdata on fields/cols at form startup
**			       in read/update modes.
**
** Description:
**	Display the data in fields and columns at read/update mode startup
**	time.  If the fields/columns aren't involved in derivations (as
**	source or destination) then always display contents, else only
**	display validated data or constant-value derivations.  Exception
**	is for constant value derivations which are always shown.
**
** Inputs:
**	frm	Frame containing field whose contents are to be displayed.
**	fldno	Field number.
**
** Outputs:
**	None.
**
**	Returns:
**		TRUE	normally,
**		FALSE	if simple field will not be displayed.
**
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	28-jul-89 (bruceb)	Written.
*/
static	i4
FDddDispData(frm, fldno)
FRAME	*frm;
i4	fldno;
{
    FIELD	*fld;
    FLDHDR	*hdr;
    FLDCOL	**col;
    TBLFLD	*tbl;
    i4		i;
    i4		j;
    i4		maxcol;
    i4		lastrow;
    i4		*flgs;
    bool	d_involved;
    i4		retval = TRUE;

    fld = frm->frfld[fldno];

    if (fld->fltag == FTABLE)
    {
	tbl = fld->fld_var.fltblfld;
	lastrow = tbl->tflastrow + 1;
	maxcol = tbl->tfcols;
	col = tbl->tfflds;
	for (j = 0; j < maxcol; j++, col++)
	{
	    hdr = &(*col)->flhdr;
	    d_involved = (hdr->fhdrv != NULL);
	    flgs = tbl->tffflags + j;
	    for (i = 0; i < lastrow; i++, flgs += maxcol)
	    {
		/*
		** If not involved in a derivation, or if so,
		** a valid value, display the column's contents.
		*/
		if (!d_involved || (*flgs & fdVALCHKED))
		{
		    if (!FDputdata(frm, fldno, FT_UPDATE, FT_TBLFLD, j, i))
			return(TRUE);
		}
	    }
	}
    }
    else
    {
	hdr = FDgethdr(fld);
	/*
	** If not involved in a derivation, or if so,
	** a valid value, display the field's contents.
	*/
	if (!hdr->fhdrv || (hdr->fhdflags & fdVALCHKED))
	{
	    if (!FDputdata(frm, fldno, FT_UPDATE, FT_REGFLD, (i4) 0, (i4) 0))
		retval = FALSE;	/* Field not displayed. */
	}
	else
	{
	    retval = FALSE;	/* Field not displayed. */
	}
    }
    return(retval);
}
