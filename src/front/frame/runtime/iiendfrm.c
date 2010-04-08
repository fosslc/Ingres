/*
**	iiendfrm.c
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
# include	<frscnsts.h>
# include	<frserrno.h>
# include	<lqgdata.h>
# include	<er.h>
# include	<rtvars.h>

/**
** Name:	iiendfrm.c	-	End display of a frame
**
** Description:
**
**	Public (extern) routines defined:
**		IIqbfclr()
**		IIendfrm()	-	End current display
**		IIpopfrm()	-	Pop until named display
**		IIrebld()	-	Rebuild current frame
**	Private (static) routines defined:
**
** History:
**	22-feb-1983  -	Extracted from original runtime.c (jen)
**	04-may-1983  -	Added reference to fdfrs (ncg)
**	01-may-1984  -	Added IIpopfrm() for other front ends (ncg)
**	19-jun-87 (bruceb)	Code cleanup.
**	12-aug-87 (bruceb)	Removed _VOID_ from call on CVlower.
**	11/11/87 (dkh) - Code cleanup.
**	06/22/88 (dkh) - Fixed so that we know when a popup form
**			 (even one that was forced to be a popup)
**			 was displayed.
**	07/09/88 (dkh) - Changed to cleanup row highlighting when
**			 leaving a form.
**	14-jul-88 (bruceb)
**		When ending a display loop, only check for row
**		hilighting and pop-up forms if the currently running
**		form is 'real'.  Decision is dependent on whether value
**		of IIstkfrm->fdfrs is NULL.
**	07/15/88 (dkh) - Fixed problem with Help/Help display getting
**			 cleared out.
**	07/23/88 (dkh) - Fixed problem with forms that have no fields.
**	09/23/88 (dkh) - Clear screen if we exit last form.
**	10/13/88 (dkh) - Added code to clear off display list if
**			 there are no more forms to display.
**	11/23/88 (dkh) - Changed to not call FTsyncdisp if we are
**			 exiting a formdata display loop.
**	02/18/89 (dkh) - Fixed venus bug 4546.
**	06-apr-89 (bruceb)	Fixed bug 5263.
**		Don't turn off table field row hilighting if called from
**		a formdata display loop.
**	05/19/89 (dkh) - Fixed bug 5700.
**	09/14/89 (dkh) - Fixed bug 7598.
**	11/17/89 (dkh) - Restore frskey0 activation on display loop exit.
**	12/28/89 (dkh) - Fixed bug 9430.
**	01/12/90 (dkh) - IIendfrm() now checks to see if forms is running.
**	01/13/90 (dkh) - Integrated fix for bug 9430 from phoenix.
**	01/19/90 (dkh) - Integrated from 6.4 fix for screen update
**			 problem with serialized popup display loops.
**	03/21/90 (dkh) - Restricted above fix (01/19/90) to just deal with
**			 popups.  The original change applied to fullscreen
**			 forms as well which caused extra output to appear
**			 in test suites.
**	03/22/90 (dkh) - Undo 6.3 integration of changes from 01/19/90 since
**			 it came from 6.4 in the first place.
**	04/14/90 (dkh) - Integrated changes from 6.4 to fix bug 9798.
**	04/16/90 (dkh) - Only copy old form name if we are not exiting
**			 due to an error.
**	04/18/90 (dkh) - Integrated IBM porting changes.
**	07-may-90 (bruceb)	Fix for bug 21531.
**		Don't use oldf if it's the junk struct.
**	11/02/90 (dkh) - Replaced IILIBQgdata with IILQgdata().
**	12/26/90 (dkh) - Fixed bug 35028.
**	09/20/92 (dkh) - Companion change to bug fix 41902.  This ensure that
**			 we don't try to reference through a NULL pointer.
**	08/31/93 (dkh) - Fixed up compiler complaints due to prototyping.
**      24-sep-96 (hanch04)
**          Global data moved to data.c
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
**/

FUNC_EXTERN	VOID	CVlower();
FUNC_EXTERN	VOID	IIFRrmReformatMenu();

GLOBALREF	bool	IIclrqbf ;
GLOBALREF	bool	IIFRpfwp ;
GLOBALREF	char	IIFRpfnPrevFormName[] ;


/*{
** Name:	IIqbfclr	-	??
**
** Description:
**	<comments>
**
** Inputs:
**	val	-	Value to set in IIclrqbf global
**
** Outputs:
**	
**
** Returns:
**	
**
** Exceptions:
**	none
**
** Side Effects:
**
** History:
**	?????
*/

IIqbfclr(val)
bool	val;
{
	IIclrqbf = val;
}


/*{
** Name:	IIFRuthUnsetTFHighlight - Unset table field highlighting
**
** Description:
**	This routine will unset the table field highlighting for a
**	table field if it is the current field in the passed in
**	form.  This should eliminate problems with the row
**	highlighting jumping around when re-using the form.
**
** Inputs:
*	form	Form that check row highlighting in.
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
**	07/09/88 (dkh) - Initial version.
*/
VOID
IIFRuthUnsetTFHighlight(frm)
FRAME	*frm;
{
	i4	i;
	i4	j;
	FIELD	*fld;
	TBLFLD	*tbl;
	FLDHDR	*hdr;
	i4	*pflag;

	if (frm->frfldno == 0)
	{
		return;
	}
	fld = frm->frfld[frm->frcurfld];

	if (fld->fltag != FTABLE)
	{
		return;
	}

	tbl = fld->fld_var.fltblfld;

	hdr = &(tbl->tfhdr);

	if (!(hdr->fhdflags & fdROWHLIT))
	{
		return;
	}

	for (i = 0; i < tbl->tfrows; i++)
	{
		pflag = tbl->tffflags + (i * tbl->tfcols);
		if (*pflag & fdROWHLIT)
		{
			for (j = 0; j < tbl->tfcols; j++)
			{
				pflag = tbl->tffflags + (i * tbl->tfcols) + j;
				*pflag &= ~fdROWHLIT;
			}
		}
	}
}



/*{
** Name:	IIendfrm	-	End display of a frame
**
** Description:
**	This routine is the opposite of IIactfrm().  IIendfrm() removes
**	a frame from the active list.  It then deallocates those structures
**	allocated at activation time.  Finally, it restores the previously
**	current frame.
**
**	This routine is part of RUNTIME's external interface.
**	A call to this routine will be generated by EQUEL to terminate
**	a display loop.
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
**	## display form1
**	## finalize;
**
**	if (IIdispfrm("f1","f") == 0) goto IIfdE1;
**  IIfdB1:
**	while (IIrunform() != 0) {
**	}
**	if (IIchkfrm() == 0) goto IIfdB1;
**	if (IIfsetio((char *)0) == 0) goto IIfdE1;
**  IIfdE1:
**	IIendfrm();
**
** Side Effects:
**
** History:
*/

IIendfrm()
{
	RUNFRM		*oldf;
	COMMS		*runcm;
	FRSKY		*frskey;
	i4		i;
	i4		mode;
	i4		rfrmflags;
	bool		wasfromfnames = FALSE;
	i4		otimeout;
	bool		junkstruct;

	if (!IILQgdata()->form_on)
	{
		IIFDerror(RTNOFRM, 0, NULL);
		return(FALSE);
	}

	rfrmflags = IIstkfrm->rfrmflags;

	/*
	**	Reset the runtime variable references.
	**	fdfrs may be NULL if the current stack frame was aborted
	**	in IIactfrm, in which case there isn't a 'current' form.
	*/
	if ((IIstkfrm->fdfrs) != NULL)
	{
		IIfrsvars(IIstkfrm->fdfrs);

		/* Don't unset tblfld hilite if only a formdata loop. */
		if (!IIfromfnames)
		{
		    /*
		    **  Unset any table field highlighting before
		    **  we get out of the form so that the next
		    **  time we enter the form, we will not have
		    **  the row highlight jump around in the
		    **  table field.
		    */
		    IIFRuthUnsetTFHighlight(IIstkfrm->fdrunfrm);
		}

		/*
		**	Unlink the frame from the active list.
		*/
		IIFRpfwp = ((IIstkfrm->fdrunfrm)->frmflags & fdISPOPUP) != 0;
		IIFRroldform(IIstkfrm->fdrunfrm,IIstkfrm);
	}

	oldf = IIstkfrm;
	IIstkfrm = IIstkfrm->fdrunnxt;
	IIfrmio = IIstkfrm;
	oldf->fdrunnxt = NULL;

	junkstruct = (oldf->fdfrs == NULL);

	if (junkstruct)
	{
		/* This is a 'junk' RUNFRM struct, allocated in IIdispfrm */
		MEfree((PTR)oldf);
	}
	else
	{
		/*
		**  Save away form name if this is NOT a 'junk' RUNFRM struct.
		*/
		STcopy(oldf->fdrunfrm->frname, IIFRpfnPrevFormName);
	}

	if (IIstkfrm == NULL)
	{
		FDFTsetiofrm(NULL);

		/*
		**  Call FTsyncdisp to clear off display list in FT.
		*/
		FTsyncdisp(NULL, FALSE);

		/*
		**  Unset menu reformatting flag.
		*/
		IIFRrmReformatMenu((bool) FALSE);

		/*
		**  Reset indicator if exiting from a formdata loop.
		*/
		if (IIfromfnames)
		{
			IIfromfnames = 0;
		}
	}
	else
	{
		/*
		**  Put the evcb for the form back into the FRS' copy.
		*/
		otimeout = IIfrscb->frs_event->timeout;
		MEcopy((PTR) IIstkfrm->fdevcb, (u_i2) sizeof(FRS_EVCB),
			(PTR) IIfrscb->frs_event);
		IIfrscb->frs_event->timeout = otimeout;

		/*
		**  If we are exiting a FORMDATA loop then rebuild
		**  previous frame so things look correct.  (dkh)
		*/

		if (IIfromfnames)
		{
			wasfromfnames = TRUE;
			IIfromfnames = 0;
		}

		/*
		**  Need to set up frame info for possible table field
		**  traversal.	This is due to FT. (dkh)
		*/
		FDFTsetiofrm(IIfrmio->fdrunfrm);

		/*
		**	Reset the frame driver control character command
		**	interrupt list.
		*/
# ifndef	PCINGRES
		runcm = IIstkfrm->fdruncm;
		if (runcm == NULL)
			return (FALSE);
		for (i = 0; i < c_MAX; i++)
		{
			/* New form for FT interface. (dkh) */
# ifdef	ATTCURSES
			FTaddcomm((i2) IIvalcomms[i], (i4) runcm[i].c_val);
# else
			FTaddcomm((u_char) IIvalcomms[i],
				(i4) runcm[i].c_val);
# endif
		}
# endif		/* PCINGRES */
		
		/*
		**  Fixed so that a form gets rebuilt before we get
		**  to an outer display loop.  This should solve
		**  problems with a ## redisplay statement immediately
		**  after we exit an inner display loop.  A symptom
		**  of the one window stuff.
		*/

		if (wasfromfnames)
		{
			/*
			**  If we are exiting for a FORMDATA loop, then
			**  no need to update screen image with FTsyncdisp
			**  calls since IIdispfrm() does not call FTsyncdisp
			**  for a FORMDATA loop.
			**
			**  NOT calling FTsyncdisp is OK since all one is
			**  allowed to do inside a FORMDATA loop is to
			**  run inquire_frs statements.  The only problem
			**  may be compatibility with applications that
			**  call other routines to display another form
			**  within the FORMDATA loop.
			*/
			;
		}
		else if (IIstkfrm->fdrunfrm != IIprev)
		{
			/*
			**  Screen clearing is delayed till
			**  IIrunform() is called.
			*/

			FTsyncdisp(IIstkfrm->fdrunfrm, FALSE);
		}
		else if (rfrmflags & RTACTNORUN)
		{
			IIprev = NULL;

			/*
			**  Fix for BUG 8013. (dkh)
			**  Need to synchronize forms here since we
			**  may go back into a submenu loop and have
			**  no chance at that point.
			*/
			FTsyncdisp(IIstkfrm->fdrunfrm, FALSE);
		}

		frskey = IIstkfrm->fdfrskey;
		if (frskey == NULL)
			return(FALSE);


		IIresfrsk();

		/*
		**  Restore frskey0 activation value.
		*/
		IIfrscb->frs_globs->intr_frskey0 = IIstkfrm->fdrunfrm->frintrp;
		
		/*
		** BUG 3915
		** Took out IIsetferr(0) so errors occuring in IIactfrm
		** stay until after the display loop ends.
		*/

		/*
		**	Reset what the frame driver thinks is the current
		**	frame.
		*/

		/*
		**  Fix to set the right mode when popping out of a form.
		**  (dkh)
		*/
		mode = IIstkfrm->fdrunmd;
		switch (mode)
		{
			case fdrtUPD:
				mode = fdmdUPD;
				break;

			case fdrtINS:
				mode = fdmdADD;
				break;

			case fdrtQRY:
				mode = fdmdFILL;
				break;

			case fdrtRD:
				mode = fdmdUPD;
				break;

			default:
				break;
		}
		FDrstfrm(IIstkfrm->fdrunfrm, mode);

		if (!junkstruct && (oldf->fdrunfrm->frmflags & fdISPOPUP))
		{
			IIstkfrm->fdrunfrm->frmflags |= fdDISPRBLD;
		}
	}
	return (TRUE);
}

/*{
** Name:	IIpopfrm	-	Pop the frame stack 
**
** Description:
**	Pops the frame stack until the named frame is on the top
**	(i.e. becomes the displayed frame).
**
**	Called from frontends that allow user forms to be displayed
**	on top of internally defined forms.
**
** Inputs:
**	popname		Name of frame to find on the frame stack 
**			and display
**
** Outputs:
**
** Returns:
**	i4	TRUE	Named frame was found and displayed
**		FALSE	Error occurred in frame popping
**
** Exceptions:
**	none
**
** Side Effects:
**
** History:
*/

i4
IIpopfrm(popname)
char	*popname;
{
	char	namebuf[ MAXFRSNAME ];

	if (!IILQgdata()->form_on)
		return (FALSE);

	STcopy(popname, namebuf);
	STtrmwhite(namebuf);
	CVlower(namebuf);

	while (IIstkfrm != NULL)
	{
		if (STcompare(IIstkfrm->fdfrmnm, namebuf) == 0)
			return (TRUE);
		if (!IIendfrm())		/* internal error occurred */
			break;
	}
	return (FALSE);
}



/*{
** Name:	IIrebld	-	Rebuild the current frame
**
** Description:
** 	Rebuild the current frame since the help facility was called
**	from a submenu and probably has trashed the form that help
**	was being obtained on.
**
** Inputs:
**
** Outputs:
**
** Returns:
**
** Exceptions:
**	none
**
** Side Effects:
**
** History:
*/

IIrebld()
{
	if (IIstkfrm != NULL && IIstkfrm->fdrunfrm != NULL)
	{
		IIprev = IIstkfrm->fdrunfrm;
		FTsyncdisp(IIstkfrm->fdrunfrm, FALSE);
	}
}

/*{
** Name:	IIFRroldform	- recover old form parameters.
**
** Description:
**	internal routine to restore form parameters possibly changed by
**	"with" clause parameters (popup display, and so on)
**
** Inputs:
**	frm - frame.
**	runf - runtime structure.
**
** History:
**	4/88 (bobm) written
*/

IIFRroldform(frm,runf)
FRAME *frm;
RUNFRM *runf;
{
	frm->frmflags = runf->saveflags;
	frm->frposx = runf->begx;
	frm->frposy = runf->begy;
}
