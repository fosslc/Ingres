/*
**	iiactfrm.c
**
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<cv.h>		/* 6-x_PC_80x86 */
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
# include	<cm.h>
# include	<me.h>
# include	<er.h>
# include	<fsicnsts.h>
# include	<erfi.h>
# include	<lqgdata.h>
# include	"iigpdef.h"
# include	<rtvars.h>

/**
** Name:	iiactfrm.c
**
** Description:
**	Activate an initialized frame.
**
**	Public (extern) routines defined:
**	Private (static) routines defined:
**
** History:
**	16-feb-1983  -  Extracted from original runtime.c (jen)
**	21-oct-1983  -  Added ncg's activate column changes (nml)
**	22-mar-1984  -	Added QueryOnly Field option (ncg)
**	10-dec-1985  -  Changed name to iifrmact.c (prs)
**	05-jan-1987  -  Changed calls to IIbadmem. (prs)
**	10/20/86 (KY)  -- Changed CH.h to CM.h.
**	05/02/87 (dkh) - Integrated change bit code.
**	25-jun-87 (bruceb)	Code cleanup.
**	13-jul-87 (bruceb)	Changed memory allocation to use [FM]Ereqmem.
**	08/14/87 (dkh) - ER changes.
**	08/26/87 (dkh) - Changes for 8000 series error numbers.
**	09/01/87 (dkh) - Added explicit include of <frserrno.h>.
**	09/29/87 (dkh) - Changed iiugbma* to IIUGbma*.
**	09/30/87 (dkh) - Added procname as param to IIUGbma*.
**	11/23/88 (dkh) - Changed to not call FTsyncdisp if we are
**			 starting a formdata display loop.
**	07-dec-88 (bruceb)
**		Initialize RUNFRM's tmout_val to 0.
**	05/19/89 (dkh) - Fixed bug 5700.
**	08/08/89 (dkh) - Clear out entry activation values on frame activation.
**	08/04/89 (dkh) - Added support for 80/132 runtime switching.
**	09/01/89 (dkh) - Added changes for per display loop keystroke act.
**	09/02/89 (dkh) - Put in emerald group requested change to not
**			 clear menu line when starting new frame.  Call
**			 to FTputmenu() in IIrunform() should take care of this.
**	01/10/90 (dkh) - Put in handling for serialized popup display loops.
**	02/26/90 (dkh) - Removed warning message about form begin too
**			 large to be a popup and being forced to fullscreen (us
**			 item 286).
**	11/02/90 (dkh) - Replaced IILIBQgdata with IILQgdata().
**	01/25/91 (dkh) - Fixed bug 35580.
**	03/19/91 (dkh) - Added support for alerter event in FRS.
**	09/20/92 (dkh) - Fixed bug 41902.  If the forms system has not been
**			 enabled, don't do anything.  Doing partial work
**			 leads to clean up issues which can cause IIstkfrm
**			 to be set to a temp RUNFRM struct which causes other
**			 code to be confused later on.
**	08/31/93 (dkh) - Fixed up compiler complaints due to prototyping.
**	01-oct-96 (mcgem01)
**		form_mu changed to GLOBALREF for global data project.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	18-Aug-2009 (kschendel) 121804
**	    Update some of the function declarations to fix gcc 4.3 problems.
**/

static void loc_error(RUNFRM *junk, ER_MSGID errnum, i4 numargs, char *arg);

GLOBALREF	i4	form_mu;

/*{
** Name:	IIdispfrm	-	Display a frame
**
** Description:
**	Provided with the name of an initialized frame, and a mode, 
**	display the frame.  It may not be displayed more than once,
**	which would imply a nested display.
**
**	This routine is part of RUNTIME's external interface.
**	
** Inputs:
**	nm		Name of the (initialized) frame to display  
**	md		Mode to use
**
** Outputs:
**
** Returns:
**	i4		TRUE
**			FALSE
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
**	???
**
** History:
*/
IIdispfrm(nm, md)
char	*nm;
char	*md;
{
	reg	RUNFRM		*runf;
	reg	RUNFRM		*stkf;
	reg	i4		i;
		COMMS		*runcm;
		FRSKY		*frskey;
		FRAME		*frm;
		char		*name;
		char		*mode;
		RUNFRM		*junk;
		RUNFRM		*orunf;
		TBSTRUCT	*tb;
		TBLFLD		*tfld;
		char		fbuf[MAXFRSNAME+1];
		char		mbuf[MAXFRSNAME+1];
		i4		style;
		i4		begx;
		i4		begy;
		i4		bord;
		i4		scrw;

	/*
	**  If the forms system has not been started, don't do anything.
	*/
	if(!IILQgdata()->form_on)
	{
		IIFDerror(RTNOFRM, 0, NULL);
		return (FALSE);
	}

	/*
	** get global parameters and reset.
	*/
	style = GP_INTEGER(FSP_STYLE);
	bord = GP_INTEGER(FSP_BORDER);
	begy = GP_INTEGER(FSP_SROW);
	begx = GP_INTEGER(FSP_SCOL);
	scrw = GP_INTEGER(FSP_SCRWID);

	IIFRgpcontrol(FSPS_RESET,0);


	/*
	**	Allocate a junk runtime structure to get rid of later
	**	in IIendfrm().	(or here, if no errors occur.)
	*/
	if ((junk = (RUNFRM *)MEreqmem((u_i4)0, (u_i4)sizeof(RUNFRM), TRUE,
	    (STATUS *)NULL)) == NULL)
	{
		IIUGbmaBadMemoryAllocation(ERx("IIdispfrm"));
	}

	junk->fdrunnxt = IIstkfrm;

	/*
	**	Check all variables make sure they are valid.
	**	Also check to see that the forms system has been
	**	initialized.
	*/
	name = IIstrconv(II_CONV, nm, fbuf, (i4)MAXFRSNAME);
	mode = IIstrconv(II_CONV, md, mbuf, (i4)MAXFRSNAME);
	if ( name == NULL || mode == NULL )
	{
		loc_error(junk, (ER_MSGID)0, (i4)0, NULL);
		return (FALSE);
	}

	/* Search for frame in list of initialized frames */
	if ((runf = RTfindfrm(name)) == NULL)
	{
		loc_error(junk, RTFRIN, 1, name);
		return (FALSE);
	}

	/*
	**	Make sure frame is not already current
	*/
	for (stkf = IIstkfrm; stkf != NULL; stkf = stkf->fdrunnxt)
	{
		if (STcompare(name, stkf->fdfrmnm) == 0)
		{
			loc_error(junk, RTFREX, 1, name);
			return (FALSE);
		}
	}

	frm = runf->fdrunfrm;

	/*
	**  Save the FRS' evcb into IIstkfrm so that we can
	**  restore it when the nested display call exits.
	*/
	if (IIstkfrm != NULL)
	{
		MEcopy((PTR) IIfrscb->frs_event, (u_i2) sizeof(FRS_EVCB),
			(PTR) IIstkfrm->fdevcb);
	}

	/*
	** save original form parameters
	*/
	runf->saveflags = frm->frmflags;
	runf->begx = frm->frposx;
	runf->begy = frm->frposy;

	/*
	** modify if specific style requested
	** Setting FSSTY_POP implies border=SINGLE unless specifically
	** set otherwise in the equel code.
	*/
	switch (style)
	{
	case FSSTY_POP:
		frm->frmflags |= fdISPOPUP;
		if (bord == FSBD_DEF)
			frm->frmflags |= fdBOXFR;
		break;
	case FSSTY_NOPOP:
		/*
		**  Check if user specified a size
		**  to display in.
		*/
		switch (scrw)
		{
			case FSSW_CUR:
				/*
				**  Remove any setting specified with Vifred.
				*/
				frm->frmflags &= ~(fdNSCRWID | fdWSCRWID);
				break;

			case FSSW_DEF:
				/*
				**  No need to do anything here.  If
				**  was a full screen in Vifred, then
				**  use that definition.
				*/
				break;

			case FSSW_NARROW:
				frm->frmflags &= ~fdWSCRWID;
				frm->frmflags |= fdNSCRWID;
				break;

			case FSSW_WIDE:
				frm->frmflags &= ~fdNSCRWID;
				frm->frmflags |= fdWSCRWID;
				break;
		}
		frm->frmflags &= ~fdISPOPUP;
		break;
	default:
		break;
	}

	/*
	** doesn't currently matter if form is not popup, but
	** doesn't hurt to set it.
	*/
	switch (bord)
	{
	case FSBD_SINGLE:
		frm->frmflags |= fdBOXFR;
		break;
	case FSBD_NONE:
		frm->frmflags &= ~fdBOXFR;
		break;
	default:
		break;
	}

	/*
	** if not defaulted, set starting position.
	** <0 used to force 0 ("FLOATING").  0 indicates use of whatever
	** value is already on the form.
	*/
	if (begx != 0)
		frm->frposx = begx < 0 ? 0 : begx;
	if (begy != 0)
		frm->frposy = begy < 0 ? 0 : begy;

	orunf = IIstkfrm;
	runf->fdrunnxt = IIstkfrm;
	IIstkfrm = runf;
	IIfrmio = runf;

	/*
	**  Call FTsyncdisp to put trim, titles, etc into the one
	**  window we are displaying everything through. (dkh)
	*/

	CVlower(mode);
	if (STcompare(mode, ERx("names")) != 0)
	{
		i = FTsyncdisp(IIfrmio->fdrunfrm, TRUE);
		switch(i)
		{
		case PFRMMOVE:
			IIFDerror(E_FI2262_PopMove, 1, (PTR) name);
			break;
		case PFRMSIZE:
			frm->frposx = 0;
			frm->frposy = 0;
			break;
		default:
			break;
		}
	}

	/*
	**  Set up frame info for possible table field traversal.
	**  Needed due to FT. (dkh)
	*/

	FDFTsetiofrm(IIfrmio->fdrunfrm);

	if (!RTsetmode(TRUE, runf, mode))
	{
		IIFRroldform(frm,runf);
		return (FALSE);
	}
	FDschgbit(runf->fdrunfrm, 0, FALSE);
	FDschgbit(runf->fdrunfrm, 1, TRUE);

	/*
	**	Reset field interrupts for activate statements
	**	can only activate on the frfld[] array as frnsfld[] is
	**	display only.
	*/
	for(i = 0; i < frm->frfldno; i++)
	{
		frm->frfld[i]->flintrp =
			frm->frfld[i]->fld_var.flregfld->flhdr.fhenint = 0;
	}

	/*
	** 	Reset all activate TDscrollup/TDscrolldown on table fields,
	**	and table field current rows that are linked to the new
	**	frame. Reset column activates.
	*/
	for (tb = runf->fdruntb; tb != NULL; tb = tb->tb_nxttb)
	{
		tb->scrintrp[0] = 0;
		tb->scrintrp[1] = 0;
		tfld = tb->tb_fld;
		tfld->tfcurrow = 0;
		tfld->tfcurcol = 0;
		for(i = 0; i < tfld->tfcols; i++)
		{
			tfld->tfflds[i]->flhdr.fhintrp =
				tfld->tfflds[i]->flhdr.fhenint = 0;
		}
	}
	/*
	**	Reset current field number to the first field
	*/
	frm->frcurfld = 0;
	runf->fdruncur = 0;
	frm->frintrp = IIfrscb->frs_globs->intr_frskey0 = 0;

	/*
	**	Reset control character command sequences
	*/
# ifndef	PCINGRES
	runcm = runf->fdruncm;
 
	/*
	**  Fix for BUG 8858. (dkh)
	*/
	for (i = 0; i < c_MAX; i++)
	{
		runcm[i].c_val = fdopERR;
		runcm[i].c_comm = FALSE;

		/* New interface for FT. (dkh) */
# ifdef	ATTCURSES
		FTaddcomm((i2) IIvalcomms[i], (i4) fdopERR);
# else
		FTaddcomm((u_char) IIvalcomms[i], (i4) fdopERR);
# endif
	}
# endif		/* PCINGRES */

	frskey = runf->fdfrskey;
	for (i = 0; i < MAX_FRS_KEYS; i++)
	{
		frskey[i].frs_int = fdopERR;
		frskey[i].frs_exp = NULL;
		frskey[i].frs_val = FRS_NO;
		frskey[i].frs_act = FRS_NO;
	}
	FTclrfrsk();


	/*
	** 	Clean up menu interface
	FTmessage(ERx("   "), FALSE, FALSE);
	*/
	runf->fdmunum = 0;

	runf->fdfrs->fdrunfld = NULL;
	MEfree((PTR)junk);
	IIsetferr((ER_MSGID) 0);		/* clear FRS error flag */

	form_mu = TRUE;

	runf->rfrmflags |= RTACTNORUN;

	/* Initialize to 'no timeout handler' for this frame */
	IIstkfrm->tmout_val = 0;

	/* Clear out values for (alerter) event handling */
	IIstkfrm->alev_intr = 0;
	IIstkfrm->intr_kybdev = 0;
	IIstkfrm->ret_kybdev = 0;

	if (orunf == NULL || orunf->fdrunfrm->frmflags & fdDISPRBLD)
	{
		IIstkfrm->fdrunfrm->frmflags |= fdDISPRBLD;
		if (orunf)
		{
			orunf->fdrunfrm->frmflags &= ~fdDISPRBLD;
		}
	}

	/*
	**  Clear out field number so field entry activation works
	**  from resume field in initialize block of display loop.
	*/
	IIstkfrm->fdrunfrm->frres2->origfld = BADFLDNO;

	return (TRUE);
}

/*
**      06-Jul-2000 (inifa01)
**      Bug 102027
**      User gets segmentation violation in abf when esqlc statement: exec frs
**      message :disperr with style = popup; is executed in nested frame.
**      IISmessage() will attempt to access member of global IIstkfrm structure
**      while structure is temporarily assigned junk runtime structure for cleanup.
**      Fix was to initialize IIstkfrm->fdrunfrm to current value to make it
**      available in IISmessage().
**      14-Nov-2008 (hanal04) Bug 121288
**          The fix for Bug 102027 does not deal with junk->fdrunnxt == NULL.
**          Avoid SEGVs by checking for NULL fdrunnxt when trying to assign
**          fdrunfrm.
*/
void
loc_error(RUNFRM *junk, ER_MSGID errnum, i4 numargs, char *arg)
{
	IIstkfrm = junk;
	IIstkfrm->fdrunfrm = (IIstkfrm->fdrunnxt != NULL) ?
                              IIstkfrm->fdrunnxt->fdrunfrm : NULL;
	IIfrmio = junk;
	if (errnum != 0)	/* if really have error msg to print */
		IIFDerror(errnum, numargs, arg);
}
