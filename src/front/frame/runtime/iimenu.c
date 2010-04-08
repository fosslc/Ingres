/*
**	iimenu.c
**
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

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
# include	<frserrno.h>
# include	<frscnsts.h>
# include	<cm.h>
# include	<si.h>
# include	<st.h>
# include	<me.h>
# include	<ug.h>
# include	<er.h>
# include	"erfr.h"
# include	<rtvars.h>
# include	"setinq.h"

/**
** Name:	iimenu.c	-	Menu handling routines
**
** Description:
**
**	Public (extern) routines defined:
**		IIinitmu()
**		IIactmu()
**		IIendmu()
**		IImuonly()
**		IIrunmu()
**		IInestmu()
**		IIendnest()
**
**	Private (static) routines defined:
**
** History:
**	16-feb-1983  -	Extracted from original runtime.c (jen)
**	06-dec-1983  -	Added the "menu only" option for nested
**			menus. (ncg)
**	5/8/85 - Added support for long menu.s (dkh)
**	7/15/85 - Changed to add support for control/function
**		  key mapping to a menu position. (dkh)
**	9/13/85 - Added IIgtcurmu so Grant can find out the
**		  current menu and provide help on it. (dkh)
**	13-jun-86 (mgw) bug # 9582
**		took out comments around the MEfree in IImuhdlr()
**		to prevent slow memory growth when entering and
**		exiting multiple submenus.
**	12/04/86 (drh) Added nested menu support.
**	12/23/86 (dkh) - Added support for new activations.
**	10/20/86 (KY)  -- Changed CH.h to CM.h.
**	06/13/87 (dkh) - Fixed so that calling a display submenu
**			 works from a plain submenu.
**	19-jun-87 (bruceb) Code cleanup.
**	13-jul-87 (bruceb) Changed memory allocation to use [FM]Ereqmem.
**	08/14/87 (dkh) - ER changes.
**	08/26/87 (dkh) - Changes for 8000 series error numbers.
**	09/01/87 (dkh) - Added explicit include of <frserrno.h>.
**	09/03/87 (dkh) - Moved RTMUCNT into erfi.h.
**	11/12/87 (dkh) - Put in change for IIrunmu.
**	02/01/88 (dkh) - Fixed jup bug 1898.
**	09/24/88 (dkh) - Fixed IIrunmu() to check for value
**			 of IIstkfrm before use.
**	09/24/88 (dkh) - Fixed jup bug 1852.
**	09-nov-89 (bruceb)
**		Save away and restore entry activation codes.
**	11-nov-88 (bruceb)
**		Handle timeout.  Return the (possibly existing) activate
**		timeout value.  When running display submenu, save away
**		the parent display loop's timeout activation value for
**		later restoration.  Non-display submenu's use IIFRtovTmOutVal
**		to store their activation values.
**	02/18/89 (dkh) - Fixed venus bug 4546.
**	02/20/90 (dkh) - Fixed bug 9771.
**	07/05/90 (dkh) - Delayed calling IIresfrsk() in IIrunmu() so
**			 help/keys for submenus works correctly.
**	07/26/90 (dkh) - Fixed bug 31958.
**	07/27/90 (dkh) - Modified fix for 31958 so that screen update
**			 is only if we just exited out of a popup form.
**			 This is necessary so we don't repaint fullscreen
**			 forms unnecessarily.
**	03/19/91 (dkh) - Added support for alerter events in FRS.
**	08/08/91 (dkh) - Fixed bug 36327.  Added code to check for
**			 IIprev being NULL before trying to de-reference
**			 through it.
**	06/24/92 (dkh) - Added support for renaming and enabling/disabling
**			 menuitems.
**	02/16/93 (dkh) - Fixed IIFRmsmenustat() to exit correctly when a
**			 menuitem name has been found.
**	05/25/93 (rudyw)
**		Fix bug 51153. Memory allocated for certain menuitem strings
**		for nested menus in IInmuact needs to be allocated as tagged
**		memory so that it gets freed properly in routine IIendnest.
**	08/31/93 (dkh) - Fixed up compiler complaints due to prototyping.
**	11/01/93 (connie) Bug #49733
**		Reset the frame driver result (IIresult) in IIinitmu
**      24-sep-96 (hanch04)
**              Global data moved to data.c
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	18-Aug-2009 (kschendel) 121804
**	    Update some of the function declarations to fix gcc 4.3 problems.
**/


GLOBALREF i4	form_mu ;		/* true if menu on a form	*/
/* Stores the timeout return value for the current ##submenu.  */
GLOBALREF	i4	IIFRtovTmOutVal ;
GLOBALREF	bool	IIexted_submu;
GLOBALREF	char	dmpbuf[256];

static	MENU	IImenuonly = NULL;	/* buffer for menu only		*/
static	i4	ishelp = 0;

static	bool	mu_reformat = FALSE;

GLOBALREF	i4		IImenuptr;
GLOBALREF	FT_TERMINFO	frterminfo;
GLOBALREF	bool		IIFRpfwp;
FUNC_EXTERN	STATUS	IImumatch();

/*
**  Structures used for saving nested menu activation information
*/

typedef struct act_save
{
	i2		savetag;	/* tag for save area allocations */
	struct fld_save *savflds;	/* field and column activations */
	struct tbl_save *savtbls;	/* up/down scroll activations for tfs */
	FRSKY		*savfrskey;	/* FRS key activations */
	i4		savmunum;	/* no. of menu items */
	FRS_EVCB	sav_evcb;	/* area to save event control block */
	i4		savtoval;	/* timeout activation */
	i4		savfrsk0;	/* activate value for frskey0 */
	i4		savalev;	/* activate value for (alerter) event */
	i4		savintr_ky;	/* pending intr value for kybd event */
	i4		savret_ky;	/* pending retval for IIrunform() */
} ACT_SAVE;

typedef struct fld_save
{
	struct fld_save *fsnext;	/* nest field saved in list */
	FLDHDR		*fshdr;		/* field header */
	i4		fsintrp;	/* field activation code */
	i4		fsenint;	/* field entry activation code */
} FLD_SAVE;

typedef struct tbl_save
{
	struct tbl_save *tbnext;	/* nest tbl field saved in list */
	TBSTRUCT	*tblptr;	/* table field ptr */
	i4		scrintrp[2];	/* scroll activations */
} TBL_SAVE;


static i4 IIaddact( i2 acttag, FLD_SAVE **flist,
	i4 actcode, i4 enactcode, FLDHDR *fldhdr );


/*{
** Name:	IIissubmu	-	Is current menu a submenu
**
** Description:
**	Routine to find out if the current menu is a 'normal' menu
**	(i.e. attached to a form), or a submenu.
**
** Inputs:
**
** Outputs:
**
** Returns:
**	BOOL	TRUE if a submenu, FALSE otherwise
**		FALSE
**
** Exceptions:
**	none
**
** Side Effects:
**
** History:
**
*/

bool
IIissubmu(void)
{
	return((bool)(form_mu ? FALSE : TRUE));
}


/*{
** Name:	IIFRrmReformatMenu - Reformat Menu Display.
**
** Description:
**	This routines sets a static variable to indicate whether
**	menus need to be reformatted or not.  In addition, the
**	current menu (for the currently displayed form) is also
**	marked for reformatting if the passed in value is TRUE.
**
**	This is necessary when a new mapping file or set statements
**	for "map" and "label" is executed by a program.  Note that
**	submenus need not be handled since they are a one shot
**	deal and their image will be rebuilt everytime a submenu
**	is re-executed in a program loop.
**
** Inputs:
*	val	Value to set static to, TRUE if menus need to
**		be reformatted.  FALSE if not.
**
** Outputs:
**	Returns:
**		None.
**	Exceptions:
*		None.
**
** Side Effects:
**	None.
**
** History:
**	02/17/89 (dkh) - Initial version.
*/
VOID
IIFRrmReformatMenu(val)
bool	val;
{
	mu_reformat = val;
	if (val && IIstkfrm != NULL && IIstkfrm->fdmunum > 0 &&
		IIstkfrm->fdrunmu->mu_active)
	{
		IIstkfrm->fdrunmu->mu_flags |= MU_REFORMAT;
	}
}


/*{
** Name:	IIFRmrMenuReformat - Need to Reformat Menus.
**
** Description:
**	Returns value in static variable mu_reformat to indicate
**	if menus need to be reformatted or not.  TRUE indicates
**	that a menu needs to be reforamtted.  FALSE if no
**	reformatting is necessary.
**
** Inputs:
**	None.
**
** Outputs:
**	Returns:
**		val	Static variable mu_reformat.
**	Exceptions:
*		None.
**
** Side Effects:
**	None.
**
** History:
**	02/18/89 (dkh) - Initial version.
*/
bool
IIFRmrMenuReformat()
{
	return(mu_reformat);
}


/*{
** Name:	IIgtcurmu	-	??
**
** Description:
**  Special routine for Grant to find out what the current menu
**  is so that help can be obtained.
**
** Inputs:
**
** Outputs:
**
** Returns:
**	Ptr to the current menu
**
** Exceptions:
**	none
**
** Side Effects:
**
** History:
**
*/

MENU
IIgtcurmu()
{
	MENU	curmu = NULL;

	if (form_mu)
	{
		if (IIstkfrm != NULL && IIstkfrm->fdmunum > 0 &&
			IIstkfrm->fdrunmu->mu_active)
		{
			curmu = IIstkfrm->fdrunmu;
		}
	}
	else
	{
		curmu = IImenuonly;
	}

	return(curmu);
}

/*{
** Name:	IImumatch	-
**
** Description:
**	Given a menu and a menu position, fill in
**	actviate, validate and interrupt value parts of an
**	event control block.
**
** Inputs:
**	mu	Ptr to amenu
**	evcb	Ptr to an event control block
**	intrval Interrupt code
**
** Outputs:
**
** Returns:
**	STATUS
**
** Exceptions:
**	none
**
** Side Effects:
**
** History:
**
*/

STATUS
IImumatch(mu, evcb, intrval)
MENU		mu;
FRS_EVCB	*evcb;
i4		*intrval;
{
	long	num;
	i4	flags;
	i4	act_val = FRS_UF;
	struct com_tab	*ct;
	i4	i;
	i4	munum;
	i4	visible;

	num = evcb->mf_num;
	if (num < 0 || num >= mu->mu_last)
	{
		num++;
		IIUGerr(E_FR0158_Menuitem_does_not, UG_ERR_ERROR, 1, &num);
		return(FAIL);
	}

	/*
	**  We need to figure out which menuitem was selected.  This
	**  is no longer just a quick index (using num) into the mu_coms
	**  array due to dynamic menus.  "num" gives us the position
	**  of the enabled menuitems.  So we must skip over the disabled
	**  ones to find the correct entry in mu_coms.
	*/
	munum = mu->mu_last;
	visible = 0;
	ct = mu->mu_coms;
	for (i = 0; i < munum; i++, ct++)
	{
		if (ct->ct_flags & CT_DISABLED)
		{
			continue;
		}
		if (visible == num)
		{
			break;
		}
		visible++;
	}

	ct = &(mu->mu_coms[i]);

	evcb->intr_val = *intrval = ct->ct_enum;
	flags = ct->ct_flags;
	if (flags & CT_YES_VALID)
	{
		act_val = FRS_YES;
	}
	else if (flags & CT_NO_VALID)
	{
		act_val = FRS_NO;
	}
	evcb->val_event = act_val;
	act_val = FRS_UF;
	if (flags & CT_ACTYES)
	{
		act_val = FRS_YES;
	}
	else if (flags & CT_ACTNO)
	{
		act_val = FRS_NO;
	}
	evcb->act_event = act_val;
	FDdmpmsg(ERx("MENU: %s"), dmpbuf);
	FDdmpmsg(ERx("MENU AT POSITION `%d' SELECTED VIA A FUNCTION/PF KEY\n"),
		num);
	FDdmpmsg(ERx("MENU AT POSITION `%d' is \"%s\"\n"), num, ct->ct_name);

	/*
	**  Call routine pointed to by pointer.
	**  This is special for VIFRED and OLDOSL
	**  and DOES NOT obey the validate/activate rules.
	*/
	if (ct->ct_func != NULL)
	{
		(*(ct->ct_func))();
	}

	return(OK);
}

/*{
** Name:	IIresmu		-	Resume to menu
**
** Description:
**	Handle an EQUEL "## RESUME MENU" statement.  Resuming to the
**	menu in a submenu is not allowed.
**
**	This routine is part of RUNTIME's external interface.
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
** Example and Code Generation:
**	## resume menu
**
**	IIresmu();
**
** Side Effects:
**
** History:
**
*/

i4
IIresmu()
{
	/*
	**  If menu not attached to a form or form
	**  is not OK, then return FALSE.
	*/

	if (!form_mu || !RTchkfrs(IIstkfrm))
	{
		return(FALSE);
	}
	if (IIstkfrm->fdmunum > 0 &&
		IIstkfrm->fdrunmu->mu_active)
	{
		IIstkfrm->rfrmflags |= RTRESMENU;
	}

	return(TRUE);
}

/*{
** Name:	IIinitmu	-	Initialize a menu
**
** Description:
**	Call generated by EQUEL to initialize an empty menu, before
**	subsequent calls add menu items to it.
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
**	## activate menuitem "menu1" (explanation = "hi", activate = on)
**	## {
**	## }
**	## activate menuitem "menu2"
**	## {
**	## }
**
**   IImuInit1:
**	if (IIinitmu() == 0) goto IIfdE1;
**	if (IInmuact("menu1", "hi", 2, ON, 1) == 0) goto IIfdE1;
**	if (IInmuact("menu2", (char *)0, 2, 2, 2) == 0) goto IIfdE1;
**	if (IIendmu() == 0) goto IIfdE1;
**	goto IIfdInit1;
**
** Side Effects:
**
** History:
**
*/

IIinitmu()
{
	/*
	**	Reset menu structure index for regular menu.
	*/
	IImenuptr = 0;

	/* Reset the frame driver result for each menu */
	IIresult = 0;
	return (TRUE);
}

/*{
** Name:	IIactmenu	-	Add a menu item activation
**
** Description:
**	Cover routine for IInmuact to provide compatability for pre 6.0
**	EQUEL programs without re-preprocessing the code.
**
**	This routine is part of the external interface to RUNTIME.
**
** Inputs:
**	strvar
**	exp
**	val
**	intrp
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
**
*/

i4
IIactmenu(char *strvar, char *exp, i4 val, i4 intrp)
{
	return(IInmuact(strvar, exp, val, FRS_UF, intrp));
}

/*{
** Name:	IIactmu		-	Add a menu item activation
**
** Description:
**
**	Cover routine for call to IInmuact.
**
** Inputs:
**	strvar
**	intrp
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
**
*/

i4
IIactmu(char *strvar, i4 intrp)
{
	return(IInmuact(strvar, NULL, FRS_UF, FRS_UF, intrp));
}


/*{
** Name:	IInactmenu	-	Add a menu item activation
**
** Description:
**	Handle an EQUEL "## ACTIVATE MENUITEM" statement for release
**	6.0 and later.
**
**	This routine is part of the external interface to RUNTIME.
**
** Inputs:
**	strvar
**	exp
**	val
**	act
**	intrp
**
** Outputs:
**
** Returns:
**	i4	TRUE
**		FALSE
**
** Example and Code Generation: (see IIinitmu)
**
** Exceptions:
**	none
**
** Side Effects:
**
** History:
**
*/

i4
IInactmenu(char *strvar, char *exp, i4 val, i4 act, i4 intrp)
{
	return(IInmuact(strvar, exp, val, act, intrp));
}

/*{
** Name:	IInmuact	-	Handle menuitem activation
**
** Description:
**
** Inputs:
**	strvar
**	exp
**	val
**	act
**	intrp
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
** Side Effects:
**
** History:
**
*/

i4
IInmuact(char *strvar, char *exp, i4 val, i4 act, i4 intrp)
{
	i2		savetag;
	i4		temp;
	struct menuType *runmu;
	struct com_tab	*ct;
	char		*oldexp;
	char		*oldcomm;
	register char	*command;
	char		*cp;
	char		mbuf[201];
	bool		ctrlfound = FALSE;

	/*
	**	Check all variables make sure they are valid.
	**	Also check to see that the forms system has been
	**	initialized and that there is a current frame.
	*/
	if (strvar == NULL)
		return (TRUE);
	command = IIstrconv(II_TRIM, strvar, mbuf, (i4)200);


	/*
	**	If command string is NULL, skip.
	*/
	if (command == NULL || *command == '\0')
		return (TRUE);

	/*
	**  Make sure we don't have too many menuitems, max of 25.
	*/
	if (IImenuptr >= MAX_MENUITEMS)
	{
		i4	mucnt;
		char	smubuf[100];

		if (form_mu)
		{
			STcopy(IIstkfrm->fdfrmnm, smubuf);
		}
		else
		{
			STcopy(ERget(F_FR0002_SUBMENU), smubuf);
		}
		mucnt = MAX_MENUITEMS;
		IIFDerror(RTMUCNT, 2, (PTR) &mucnt, (PTR) smubuf);
		return (FALSE);
	}

	/*
	**  Check to make sure there are no control chars
	**  in the menuitem.  If there are, just change them
	**  to blanks.
	*/

	for (cp = command; *cp; CMnext(cp))
	{
		if (CMcntrl(cp))
		{
			ctrlfound = TRUE;
			*cp = ' ';
		}
	}

	if (ctrlfound)
	{
		IIFDerror(RTBADCHMU, 0);
	}

	/*
	**	Get the structures allocated when the frame or menu
	**	were initialized.
	*/
	if (form_mu)
		runmu = IIstkfrm->fdrunmu;
	else
		runmu = IImenuonly;

	ct = &(runmu->mu_coms[IImenuptr]);
	oldcomm = ct->ct_name;
	oldexp = ct->description;

	/*
	** Check if the current running menu has a previous menu pointer.
	** If so, we have a nested menu for which we must grab the tag id in
	** order to allocate tagged memory that can be properly freed. 51153
	*/
	savetag = 0;
	if ( runmu->mu_prev != NULL && runmu->mu_prev->mu_act != NULL )
	{
		savetag = *((i2 *)runmu->mu_prev->mu_act);
	}

	/*
	**	Is there a previously defined command?	If so check to
	**	see if it is the same as this command.	Otherwise allocate
	**	a new string.
	*/
	if (oldcomm != NULL)
	{
		ct->ct_flags = 0;
		if (STcompare(oldcomm, command) != 0)
		{
			ct->ct_name = STalloc(command);
			/*
			** Bug 9582 - uncommented this MEfree(). Hope
			** there wasn't a good reason for having it
			** commented out.
			*/
			MEfree(oldcomm);
		}
	}
	/*
	**	There is no old command so, allocate space for a new one.
	*/
	else
	{
		/*
		** Nested menus requiring tagged memory allocation will
		** always fall through to this section which is why only
		** this part of the conditional checks savetag.
		*/
		ct->ct_name = ( (savetag>0) ? STtalloc(savetag,command) :
					      STalloc(command) );
	}

	if (oldexp != NULL)
	{
		MEfree(oldexp);
	}

	/*
	** Do tagged allocation if in a nested menu - non-tagged otherwise
	*/
	if ( savetag > 0 )
	{
		ct->description = ((exp == NULL) ? exp : STtalloc(savetag,exp));
	}
	else
	{
		ct->description = ((exp == NULL) ? exp : STalloc(exp));
	}

	/*
	**	Add new return values.
	*/
	ct->ct_func = NULL;
	ct->ct_enum = intrp;
	if (val == FRS_YES)
	{
		temp = CT_YES_VALID;
	}
	else if (val == FRS_NO)
	{
		temp = CT_NO_VALID;
	}
	else
	{
		temp = CT_UF_VALID;
	}
	ct->ct_flags |= temp;

	if (act == FRS_YES)
	{
		temp = CT_ACTYES;
	}
	else if (act == FRS_NO)
	{
		temp = CT_ACTNO;
	}
	else
	{
		temp = CT_ACTUF;
	}
	ct->ct_flags |= temp;

	IImenuptr++;

	return (TRUE);
}

/*{
** Name:	IIendmu		-	End a menu
**
** Description:
**	End a menu and restore the previous one
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
** Side Effects:
**
** History:
**
*/

IIendmu()
{
	struct menuType *runmu;
	struct com_tab	*comtab;

	/*
	**	Check all variables make sure they are valid.
	**	Also check to see that the forms system has been
	**	initialized and that there is a current frame.
	*/
	if (form_mu)
	{
		runmu = IIstkfrm->fdrunmu;
		IIstkfrm->fdmunum = IImenuptr;
	}
	else
		runmu = IImenuonly;

	runmu->mu_dfirst = 0;
	runmu->mu_dsecond = 0;
	runmu->mu_last = IImenuptr;
	runmu->mu_flags = 1;	/* NULLRET */
	runmu->mu_active = IImenuptr;

	/*
	**  If menu not attached to form, then set SUBMENU flag.
	**  The test is not correct. (dkh)
	*/
	if (!form_mu)
	{
		runmu->mu_flags |= MU_SUBMENU;
	}

	/*
	**	Get the menu structure and terminate the structures
	*/
	comtab = runmu->mu_coms;
	comtab[IImenuptr].ct_name = NULL;
	comtab[IImenuptr].ct_enum = 0;
	if (form_mu)
		IIstkfrm->fdmunum = IImenuptr;

	/*
	**  Set flag indicating that this is a new menu. (dkh)
	*/

	runmu->mu_flags |= MU_NEW;

	return (TRUE);
}

/*{
** Name:	IImuonly	-	Initialize a submenu
**
** Description:
**	Initialize a submenu.  Note that this routine does NOT handle
**	DISPLAY SUBMENU.
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
** Side Effects:
**
** History:
**
*/

IImuonly()
{
	MENU	runmu;

	/*
	**	Check to see if this menu needs to be allocated.
	*/
	if (IImenuonly == NULL)
	{
		if ((runmu = (MENU)MEreqmem((u_i4)0,
		    (u_i4)sizeof(struct menuType),
		    TRUE, (STATUS *)NULL)) == NULL)
		{
			IIUGbmaBadMemoryAllocation(ERx("IImuonly-1"));
		}
		if ((runmu->mu_coms = (struct com_tab *)MEreqmem((u_i4)0,
		    (u_i4)((MAX_MENUITEMS + 1) * sizeof(struct com_tab)), TRUE,
		    (STATUS *)NULL)) == NULL)
		{
			IIUGbmaBadMemoryAllocation(ERx("IImuonly-2"));
		}
		runmu->mu_flags = 1;
		runmu->mu_prompt = NULL;
		IImenuonly = runmu;
	}
	form_mu = FALSE;
	FTclrfrsk();
	IIFRtovTmOutVal = 0;	/* 0 until set by activate timeout code */
	return (TRUE);
}


/*{
** Name:	IIishelp - Indicates that help facility is running.
**
** Description:
**	Special entry point to allow the help facility to run
**	correctly when a help/help is done.  This is due to
**	the way help/help is implemented.  The help facility should
**	really be revamped.
**
** Inputs:
**	Value to set flag "ishelp" to.
**
** Outputs:
**	None.
**	Returns:
**		None.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	11/12/87 (dkh) - Initial version.
*/
i4
IIishelp(i4 val)
{
	i4	old = ishelp;

	ishelp = val;

	return(old);
}


/*{
** Name:	IIrunmu		-	Run a menu
**
** Description:
**	Display the current menu
**
**	This routine is part of RUNTIME's external interface.
**
** Inputs:
**	dispflg
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
** Side Effects:
**
** History:
**
*/
i4
IIrunmu(i4 dispflg)
{
	i4	dummy;
	FRS_EVCB *evcb;

	/*
	**  Check to see if we have changed to a different form.
	**  This can happen if we are in a forever display
	**  submenu loop and go to display a longer form than
	**  the one the submenu was attched to.  Only do this
	**  if vifred/rbf is NOT running as they may not always
	**  run with a form.
	*/
	if (!ishelp && IIstkfrm != NULL && IIstkfrm->fdrunfrm != IIprev)
	{
		if (IIprev && !(IIprev->frmflags & fdISPOPUP))
			IIclrscr();
		IIprev = IIstkfrm->fdrunfrm;
	}

	/*
	**  Force a screen update if we have exited a display loop and
	**  haven't fixed up the screen yet.  This can happen if we
	**  a submenu calls a popup and the submenu is inside a forever
	**  loop.  Exiting the popup will drop back into the submenu
	**  and not a display loop so we don't have a chance to clean
	**  up the screen display.  This check should solve this problem.
	*/
	if (IIFRpfwp && IIstkfrm && IIstkfrm->fdrunfrm->frmflags & fdDISPRBLD)
	{
		IIredisp();
		IIstkfrm->fdrunfrm->frmflags &= ~fdDISPRBLD;
	}

	evcb = IIfrscb->frs_event;
	/*
	**	Put out current menu and collect result.
	*/
	if (IImenuptr == 0)
		return (TRUE);

	/*
	**	If DISPLAY SUBMENU was used then return to form and set
	**	menu only if <ESC> was hit.
	*/
	if (dispflg && IIstkfrm != NULL)
	{
		if (IIstkfrm->rfrmflags & RTACTNORUN)
		{
			/*
			**  Fix for BUG 8013. (dkh)
			**  We may be in a display submenu loop
			**  inside the initialize block of a
			**  display loop.  This will take care
			**  of things if we drop into the real
			**  part of the display loop.
			*/
			IIstkfrm->rfrmflags &= ~RTACTNORUN;
			if (IIprev && !(IIprev->frmflags & fdISPOPUP))
				IIclrscr();
			IIprev = IIstkfrm->fdrunfrm;
		}
		/*
		**  Can do this since user has had no chance to
		**  do anything that would change the way the
		**  menu looks.
		*/
		FTputmenu(IImenuonly);	/* Only needed once */

		for (;;)
		{
			IIresult = IItputfrm(IIstkfrm->fdrunfrm);
			if (IIresult == fdopESC || IIresult == fdopMENU)
			{
				/*
				**  If the menu key was hit, then
				**  break out.
				*/
				if (evcb->event == fdopMENU)
				{
					break;
				}
				else
				{
					/* must be a menu item, find it */
					if (IImumatch(IImenuonly, evcb,
						&dummy) != OK)
					{
						break;
					}
				/*
				**  For old display submenus, we will
				**  not support the special validate/
				**  activate options.
					intrval = evcb->intr_val;
					if (IIavchk(IIstkfrm) < 0)
					{
						break;
					}
				*/
					IIresult = evcb->intr_val;
					IIexted_submu = TRUE;
					IIfrscb->frs_globs->intr_frskey0 =
						IIstkfrm->fdrunfrm->frintrp;
					return(TRUE);
				}
			}
			else if (IIresult == fdopTMOUT)
			{
				IIresult = IIFRgtoGetTOact();
				IIexted_submu = TRUE;
				IIfrscb->frs_globs->intr_frskey0 =
					IIstkfrm->fdrunfrm->frintrp;
				return(TRUE);
			}
			else if (evcb->event == fdopFRSK)
			{
				IIexted_submu = TRUE;
				IIfrscb->frs_globs->intr_frskey0 =
					IIstkfrm->fdrunfrm->frintrp;
				return(TRUE);
			}
			else
			{
				/*
				**  activated on field/column
				**  Clear out event control
				**  block first so IIresnext()
				**  will work correctly.
				*/
				MEfill(sizeof(FRS_EVCB), '\0', evcb);
				IIresnext();
			}
		}
	}
	FTputmenu(IImenuonly);

	for (;;)
	{
		/*
		**  Fix for BUG 7123. (dkh)
		*/
		switch (IIresult = FTgetmenu(IImenuonly, evcb))
		{
		  case 0:
			/*
			** BUG 4182
			**
			** Allow users to return to the form with a
			** carriage return (when ## display submenu used)
			*/
			if (!dispflg)
				continue;
			break;

		  case fdopTMOUT:
			IIresult = IIFRgtoGetTOact();
			break;

		  default:
			/*
			**  This is here just as a precaution.
			*/
			if (evcb->event == fdopFRSK)
			{
				IIresult = evcb->intr_val;
				break;
			}
			if (IImumatch(IImenuonly, evcb, &dummy) != OK)
			{
				continue;
			}
			break;
		}
		break;
	}
	IIexted_submu = TRUE;
	if (IIstkfrm)
	{
		IIfrscb->frs_globs->intr_frskey0 = IIstkfrm->fdrunfrm->frintrp;
	}
	else
	{
		IIfrscb->frs_globs->intr_frskey0 = 0;
	}
	return(TRUE);
}


/*{
** Name:	IIaddact		-	Add an activation to save list
**
** Description:
**	Called as part of creating the saved activation information for
**	a nested menu, this routine allocates a structure to hold
**	activation information for a simple field or column, fills it in,
**	and adds it to the field list.
**
** Inputs:
**	acttag		Tag to use for allocations
**	list		List to add save information to
**	actcode		Activation code to save
**	enactcode	Entry activation code to save
**	fldhdr		Field header for field the acivation belongs to
**
** Outputs:
**
** Returns:
**	i4		1 if added successfully
**			0 if error
**
** Exceptions:
**	none
**
** Side Effects:
**
** History:
**	12/03/86 (drh)	Created.
*/

static i4
IIaddact( i2 acttag, FLD_SAVE **flist, i4 actcode, i4 enactcode, FLDHDR *fldhdr )
{
	FLD_SAVE	*fldsave;

	if ((fldsave = (FLD_SAVE *)FEreqmem((u_i4)acttag,
	    (u_i4)sizeof(FLD_SAVE), TRUE, (STATUS *)NULL)) == NULL)
	{
		return(0);
	}

	fldsave->fshdr = fldhdr;
	fldsave->fsintrp = actcode;
	fldsave->fsenint = enactcode;
	if ( *flist == NULL )
	{
		fldsave->fsnext = NULL;
		*flist = fldsave;
	}
	else
	{
		fldsave->fsnext = *flist;
		*flist = fldsave;
	}

	return(1);
}

/*{
** Name:	IInestmu	-		Set up a nested menu
**
** Description:
**
**	Since nested menus can be nested, and they allow new activations for
**	fields and FRSkeys to be defined, setting up a new nested menu requires
**	that the current menu and activations associated with it be
**	saved in a list.
**
**	Get a tag to be used for all nested menu structures.
**	Allocate a new menu, and store a pointer to the current menu as the
**	'previous menu'.  Allocate structure to hold all current
**	activation information (fields, columns, tablefield scrolls),
**	copy activation info into it, and store the pointer to
**	the activation save area in the previous menu.
**	Store the current FRSkey array ptr in the activation save area.
**	Allocate a new FRSkey array, clear it, and store it's pointer
**	in the currently running frame.
**
**	This routine is part of RUNTIME's external interface.
**
** Inputs:
**	none
**
** Outputs:
**	none
**
** Returns:
**	i4	-	1 if nested menu set up correctly
**			0 if there was a problem in setting up the menu
**
** Exceptions:
**	none
**
** Side Effects:
**
** History:
**	12/01/86 (drh)	Created.
**	01/07/86 (dkh) - Added code to save an event control block.
*/

i4
IInestmu(void)
{
	i4		i;
	i2		savetag;	/* tag for save area allocations */
	MENU		newmu;
	ACT_SAVE	*savptr;	/* ptr to activation save area */
	FIELD		*fldptr;
	TBSTRUCT	*tblptr;
	FLDCOL		*colptr;
	TBL_SAVE	*tblsave;

	if (!RTchkfrs(IIstkfrm))
	{
		return(FALSE);
	}
	savetag = FEgettag();

	/*
	**  Allocate a new menu, and store a pointer to the current menu
	**  in it as the 'previous menu'
	*/

	if ((newmu = (MENU)FEreqmem((u_i4)savetag,
	    (u_i4)sizeof(struct menuType), TRUE, (STATUS *)NULL)) == NULL)
	{
		return(FALSE);
	}

	if ((newmu->mu_coms = (struct com_tab *)FEreqmem((u_i4)savetag,
	    (u_i4)((MAX_MENUITEMS + 1) * sizeof(struct com_tab)),
	    TRUE, (STATUS *)NULL)) == NULL)
	{
		return(FALSE);
	}

	if ((IIstkfrm->fdrunmu->mu_act = (i4 *)FEreqmem((u_i4)savetag,
	    (u_i4)sizeof(ACT_SAVE), TRUE, (STATUS *)NULL)) == NULL)
	{
		return(FALSE);
	}

	savptr = (ACT_SAVE *) IIstkfrm->fdrunmu->mu_act;
	savptr->savetag = savetag;
	newmu->mu_prev = IIstkfrm->fdrunmu;
	savptr->savmunum = IIstkfrm->fdmunum;

	/*
	**  Save entry/exit activation codes for simple fields
	*/

	for ( i = 0; i < IIstkfrm->fdrunfrm->frfldno; i++ )
	{
	      fldptr = IIstkfrm->fdrunfrm->frfld[i];
	      if ( fldptr->fltag == FREGULAR )
	      {
			if ((fldptr->flintrp > 0)
			    || (fldptr->fld_var.flregfld->flhdr.fhenint > 0))
			{
				if (IIaddact( savetag, &savptr->savflds,
				    fldptr->flintrp,
				    fldptr->fld_var.flregfld->flhdr.fhenint,
				    &(fldptr->fld_var.flregfld->flhdr)) == 0)
				{
					return(FALSE);
				}
				fldptr->flintrp = 0;
				fldptr->fld_var.flregfld->flhdr.fhenint = 0;
			}
	      }
	}

	/*
	**  Save activation codes for tablefields
	*/

	for ( tblptr = IIstkfrm->fdruntb; tblptr != NULL;
		tblptr = tblptr->tb_nxttb )
	{
		/*
		**  Save entry/exit activation codes for columns
		*/

		for ( i = 0; i < tblptr->tb_fld->tfcols; i++ )
		{
			colptr = tblptr->tb_fld->tfflds[i];
			if ((colptr->flhdr.fhintrp > 0)
			    || (colptr->flhdr.fhenint > 0))
			{
				if (IIaddact( savetag, &savptr->savflds,
				    colptr->flhdr.fhintrp,
				    colptr->flhdr.fhenint,
				    &(colptr->flhdr)) == 0)
				{
					return(FALSE);
				}
				colptr->flhdr.fhintrp = 0;
				colptr->flhdr.fhenint = 0;
			}
		}

		/*
		**  Save scroll up and down activation
		*/

		if ( tblptr->scrintrp[0] > 0 || tblptr->scrintrp[1] > 0 )
		{
			if ((tblsave = (TBL_SAVE *)FEreqmem((u_i4)savetag,
			    (u_i4)sizeof(TBL_SAVE),
			    TRUE, (STATUS *)NULL)) == NULL)
			{
				return(FALSE);
			}

			tblsave->tblptr = tblptr;
			for ( i = 0; i < 2; i++ )
			{
				tblsave->scrintrp[i] = tblptr->scrintrp[i];
				tblptr->scrintrp[i] = 0;
			}

			if ( savptr->savtbls == NULL )
			{
				tblsave->tbnext = NULL;
				savptr->savtbls = tblsave;
			}
			else
			{
				tblsave->tbnext = savptr->savtbls;
				savptr->savtbls = tblsave;
			}
		}
	}

	/*
	**  Store the new menu as the current menu in the running frame
	*/

	IIstkfrm->fdrunmu = newmu;
	IIstkfrm->fdmunum = 0;

	/*
	**  Save FRSkey activations.
	*/

	savptr->savfrskey = IIstkfrm->fdfrskey;

	if ((IIstkfrm->fdfrskey = (struct frskey *)FEreqmem((u_i4)savetag,
	    (u_i4)(MAX_FRS_KEYS * sizeof(struct frskey)),
	    TRUE, (STATUS *)NULL)) == NULL)
	{
		return(FALSE);
	}

	IIresfrsk();

	/*
	**  Save the event control block and clear the one
	**  pointed to by IIstkfrm.
	*/
	MEcopy((PTR) IIstkfrm->fdevcb, (u_i2) sizeof(FRS_EVCB),
		(PTR) &savptr->sav_evcb);
	MEfill(sizeof(FRS_EVCB), '\0', IIstkfrm->fdevcb);

	/*
	**  Save the timeout activation value.
	*/
	savptr->savtoval = IIstkfrm->tmout_val;
	IIstkfrm->tmout_val = 0;

	/*
	**  Save away the activate value for frskey0.
	*/
	savptr->savfrsk0 = IIstkfrm->fdrunfrm->frintrp;
	IIstkfrm->fdrunfrm->frintrp = IIfrscb->frs_globs->intr_frskey0 = 0;

	/*
	**  Save away the alerter info.
	*/
	savptr->savalev = IIstkfrm->alev_intr;
	IIstkfrm->alev_intr = 0;

	savptr->savintr_ky = IIstkfrm->intr_kybdev;
	IIstkfrm->intr_kybdev = 0;

	savptr->savret_ky = IIstkfrm->ret_kybdev;
	IIstkfrm->ret_kybdev = 0;

	/*
	**  Set form_mu to TRUE so that menuitems for the
	**  nested submenus are added correctly.  The original
	**  value of form_mu is not important since plain
	**  submenus are only good for one selection and
	**  that has been used to call a nested submenu.
	*/
	form_mu = TRUE;

	/*
	**  Nested menu setup is complete
	*/

	return(TRUE);
}

/*{
** Name:	IIendnest	-	End a nested menu
**
** Description:
**	Called at the conclusion of the display of a nested menu, this routine
**	restores the previous menu, and the field and FRSkey activations
**	associated with it.  It then frees all nested menu structures using
**	the saved tag.
**
**	This routine is part of RUNTIME's external interface.
**
** Inputs:
**	none
**
** Outputs:
**	none
**
** Returns:
**	VOID
**
** Exceptions:
**	none
**
** Side Effects:
**
** History:
**	12/03/86 (drh)	Created.
**	01/07/86 (dkh) - Added code to restore an event control block.
*/
VOID
IIendnest(void)
{
	i4	i;
	MENU		oldmu;		/* menu to restore */
	ACT_SAVE	*savptr;	/* activations to restore */
	FLD_SAVE	*fldsave;
	TBL_SAVE	*tblsave;
	FIELD		*fldptr;
	TBSTRUCT	*tblptr;

	oldmu = IIstkfrm->fdrunmu->mu_prev;	/* menu to restore */
	if ( oldmu == NULL )
		return;

	savptr = (ACT_SAVE *) oldmu->mu_act;	/* saved activations */
	if ( savptr == NULL )
		return;


	/*
	**  Initialize any field activations left over from the nested menu
	*/

	for ( i = 0; i < IIstkfrm->fdrunfrm->frfldno; i++ )
	{
		fldptr = IIstkfrm->fdrunfrm->frfld[i];
		if ( fldptr->fltag == FREGULAR )
		{
			fldptr->flintrp = 0;
			fldptr->fld_var.flregfld->flhdr.fhenint = 0;
		}
	}

	for ( tblptr = IIstkfrm->fdruntb; tblptr != NULL;
		tblptr = tblptr->tb_nxttb )
	{
		for ( i = 0; i < tblptr->tb_fld->tfcols; i++ )
		{
			tblptr->tb_fld->tfflds[i]->flhdr.fhintrp = 0;
			tblptr->tb_fld->tfflds[i]->flhdr.fhenint = 0;
		}

		tblptr->scrintrp[0] = 0;
		tblptr->scrintrp[1] = 0;
	}

	/*
	**  Restore simple field and column entry/exit activations
	*/

	for ( fldsave = savptr->savflds; fldsave != NULL;
		fldsave = fldsave->fsnext )
	{
		fldsave->fshdr->fhintrp = fldsave->fsintrp;
		fldsave->fshdr->fhenint = fldsave->fsenint;
	}

	/*
	**  Restore table field scroll activations
	*/

	for ( tblsave = savptr->savtbls; tblsave != NULL;
		tblsave = tblsave-> tbnext )
	{
		tblsave->tblptr->scrintrp[0] = tblsave->scrintrp[0];
		tblsave->tblptr->scrintrp[1] = tblsave->scrintrp[1];
	}

	/*
	**  Restore the old menu
	*/

	IIstkfrm->fdrunmu = oldmu;
	IIstkfrm->fdmunum = savptr->savmunum;

	/*
	**  Set menu format flag,if necessary.
	*/
	if (IIFRmrMenuReformat() && IIstkfrm->fdmunum > 0 &&
		IIstkfrm->fdrunmu->mu_active)
	{
		IIstkfrm->fdrunmu->mu_flags |= MU_REFORMAT;
	}

	/*
	**  Restore FRSkey activations
	*/

	IIstkfrm->fdfrskey = savptr->savfrskey;
	IIresfrsk();

	/*
	**  Restore the event control block.
	*/
	MEcopy((PTR) &(savptr->sav_evcb), (u_i2) sizeof(FRS_EVCB),
		(PTR) IIstkfrm->fdevcb);

	/*
	**  Restore the timeout activation value.
	*/
	IIstkfrm->tmout_val = savptr->savtoval;

	/*
	**  Put back activate value for frskey0.
	*/
	IIstkfrm->fdrunfrm->frintrp = 
		IIfrscb->frs_globs->intr_frskey0 = savptr->savfrsk0;

	/*
	**  Restore the alerter info.
	*/
	IIstkfrm->alev_intr = savptr->savalev;
	IIstkfrm->intr_kybdev = savptr->savintr_ky;
	IIstkfrm->ret_kybdev = savptr->savret_ky;

	/*
	**  Free all save areas from nested menu
	*/

	FEfree( savptr->savetag );

	/*
	**  Put up a NULL menu (pointer) to indicate that no
	**  current menu is active.  Allows FTgtrefmu to work
	**  when forms system is switching between display
	**  loops.
	*/
	FTputmenu(NULL);

	return;
}

/*{
** Name:	IIrunnest	-	Run a nested menu
**
** Description:
**	Run a nested menu.  This is the same as running a display
**	loop (so far).
**
**	This routine is part of RUNTIME's external interface.
**
** Inputs:
**	none
**
** Outputs:
**	none
**
** Returns:
**	i4
**
** Exceptions:
**	none
**
** Side Effects:
**
** History:
**	12/04/86 (drh)	Created,
*/
i4
IIrunnest(void)
{
	return( IIrunform() );
}



/*{
** Name:	IIFRmenurename - Rename a menuitem.
**
** Description:
**	This routine is invoked by the user to rename a menuitem
**	via '## set_frs frs (rename_menu(x) = y)
**
** Inputs:
**	rfrm		RUNFRM structure to find menu to operate on.
**	old_name	The original menuitem name to look for.
**	new_name	The new name to use for the menuitem.
**
** Outputs:
**
**	Returns:
**		VOID
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	06/24/92 (dkh) - Initial version.
*/
void
IIFRmenurename(rfrm, old_name, new_name)
RUNFRM	*rfrm;
char	*old_name;
char	*new_name;
{
	i4		i;
	i4		maxitems;
	struct com_tab	*ct;
	struct menuType	*mu;

	/*
	**  Only do work if we actually have a set of menus for a form.
	*/
	if ((maxitems = rfrm->fdmunum) == 0)
	{
		IIFDerror(E_FI1FF4_8180, 1, rfrm->fdrunfrm->frname);
		return;
	}

	mu = rfrm->fdrunmu;
	ct = mu->mu_coms;
	for (i = 0; i < maxitems; i++, ct++)
	{
		if (STequal(ct->ct_name, old_name))
		{
			MEfree(ct->ct_name);
			ct->ct_name = STalloc(new_name);
			if (!(ct->ct_flags & CT_DISABLED))
			{
				mu->mu_flags |= MU_REFORMAT;
			}
			return;
		}
	}

	/*
	**  If we are here, then we didn't find the menuitem in
	**  the menu list.
	*/
	IIFDerror(E_FI1FF5_8181, 2, old_name, rfrm->fdrunfrm->frname);
}



/*{
** Name:	IIFRmcMuChg - Enable/disable a menuitem
**
** Description:
**	Enable/disable a menuitem.  This enables (makes visible)
**	and disables (makes invisible and unselectable) a menuitem
**	on the menu line.
**
**	When an item is enable/disabled, the key mappings are redistributed
**	to the new set of available menuitems.
**
** Inputs:
**	rfrm		RUNFRM structure to find menu to operate on.
**	menu_name	Name of the menuitem to enable/disable.
**	to_state	0 if to disable.
**			1 if to enable.
**
** Outputs:
**
**	Returns:
**		VOID
**	Exceptions:
**		None.
**
** Side Effects:
**	See Descriptions above.
**
** History:
**	06/26/92 (dkh) - Initial version.
**	31-Jan-2007 (kibro01) b117462
**		If a menu item has the same interrupt as a function key,
**		the function key should be activated/deactivated by any
**		change to the menu item's status.
**	10-Nov-2008 (kibro01) b121205
**		Remove fix for bug 117462 - it causes bug 121205, and is no
**		longer required since the new fix for the function key problems
**		is contained in ftptmenu.c and mapkey.c.
*/
void
IIFRmcMuChg(RUNFRM *rfrm, char *menu_name, i4 to_state)
{
	i4		i;
	i4		maxitems;
	struct com_tab	*ct;
	struct menuType	*mu;

	/*
	**  Only do work if we actually have a set of menus for a form.
	*/
	if ((maxitems = rfrm->fdmunum) == 0)
	{
		IIFDerror(E_FI1FF4_8180, 1, rfrm->fdrunfrm->frname);
		return;
	}

	mu = rfrm->fdrunmu;
	ct = mu->mu_coms;
	for (i = 0; i < maxitems; i++, ct++)
	{
		if (STequal(ct->ct_name, menu_name))
		{
			/*
			**  Only disable if currently enabled.
			**  or only enable if currently disabled.
			*/
			if (to_state == 0 &&
				!(ct->ct_flags & CT_DISABLED))
			{
				ct->ct_flags |= CT_DISABLED;
				mu->mu_flags |= MU_REFORMAT;
				mu->mu_active--;
			}
			else if (to_state == 1 &&
				ct->ct_flags & CT_DISABLED)
			{
				ct->ct_flags &= ~CT_DISABLED;
				mu->mu_flags |= MU_REFORMAT;
				mu->mu_active++;
			}
			return;
		}
	}

	/*
	**  If we are here, then we didn't find the menuitem in
	**  the menu list.
	*/
	IIFDerror(E_FI1FF5_8181, 2, menu_name, rfrm->fdrunfrm->frname);
}



/*{
** Name:	IIFRmsmenustat - Find out the state of a menuitem.
**
** Description:
**	This is a support routine that allows users to find out
**	whether a menuitem is on or off.
**
** Inputs:
**	rfrm		Pointer to a RUNFRM structure containing the
**			menu list to look in for the menuitem.
**	name		The name of the menuitem to check for.
**
** Outputs:
**
**	data		Pointer to integer where to return the information.
**			Zero (0) if menu is off.  One (1) if menu is on.
**	Returns:
**		None.
**
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	06/27/92 (dkh) - Initial version.
*/
void
IIFRmsmenustat(rfrm, name, data)
RUNFRM	*rfrm;
char	*name;
i4	*data;
{
	i4		i;
	i4		maxitems;
	struct com_tab	*ct;
	struct menuType	*mu;

	if ((maxitems = rfrm->fdmunum) == 0)
	{
		IIFDerror(E_FI1FF4_8180, 1, rfrm->fdrunfrm->frname);
		return;
	}

	mu = rfrm->fdrunmu;
	ct = mu->mu_coms;

	for (i = 0; i < maxitems; i++, ct++)
	{
		if (STequal(ct->ct_name, name))
		{
			if (ct->ct_flags & CT_DISABLED)
			{
				*data = 0;
			}
			else
			{
				*data = 1;
			}
			return;
		}
	}

	/*
	**  If we are here, that means that the menuitem name was
	**  not found.  Issue error.
	*/
	IIFDerror(E_FI1FF5_8181, 2, name, rfrm->fdrunfrm->frname);
}



/*{
** Name:	IIFRrarunfrmactive - Determines if the passed RUNFRM is active.
**
** Description:
**	This is a utility routine that is used to determine if a RUNFRM
**	pointer is on the active list.  That is, is it currently running.
**
** Inputs:
**	rfrm		RUNFRM pointer to check.
**
** Outputs:
**
**	Returns:
**		0	If rfrm is not active
**		1	If rfrm is active
**
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	06/30/92 (dkh) - Initial version.
*/
i4
IIFRrarunfrmactive(rfrm)
RUNFRM	*rfrm;
{
	i4	found = FALSE;
	RUNFRM	*runf;

	for (runf = IIstkfrm; runf != NULL; runf = runf->fdrunnxt)
	{
		if (runf == rfrm)
		{
			found = 1;
			break;
		}
	}

	if (!found)
	{
		return(FALSE);
	}

	return(TRUE);
}


/*{
** Name:	IIRTinqmu - "Inquire" support for dynamic menus.
**
** Description:
**	This routine provides the support that allows users to inquire
**	on the properties of a menu.
**
** Inputs:
**	rfrm		The RUNFRM structure that contains the menu to
**			inquire on.
**	mu_name		The name of the menu item to inquire on.
**	rtcode		The inquire operation to perform.
**			Supported values are:
**			  frsMUONOF - is menu item active or not.
**
** Outputs:
**	data		Pointer to data area to put result.
**
**	Returns:
**		TRUE	If inquire operation is known.
**		FALSE	If passed an unknown inquire operation.
**
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	09/01/92 (dkh) - Initial version.
*/
i4
IIRTinqmu(RUNFRM *rfrm, char *mu_name, i4 rtcode, i4 *data)
{
	if (rfrm == NULL)
	{
		/*
		**  This shouldn't happen but just in case.
		*/
		IIFDerror(RTFRACT, 0, NULL);
		return(FALSE);
	}

	if (!IIFRrarunfrmactive(rfrm))
	{
		IIFDerror(E_FI1FF6_8182, 1, rfrm->fdrunfrm->frname);
		return(FALSE);
	}

	switch(rtcode)
	{
		case frsMUONOF:
			IIFRmsmenustat(rfrm, mu_name, data);
			break;

		default:
			return(FALSE);
	}

	return(TRUE);
}



/*{
** Name:	IIRTsetmu - "Set" support for dynamic menus
**
** Description:
**	This routine provides the support that allows users to set
**	the properties of a menu.
**
** Inputs:
**	rfrm		The RUNFRM structure that contains the menu to
**			inquire on.
**	mu_name		The name of the menu item to inquire on.
**	rtcode		The inquire operation to perform.
**			Supported values are:
**			  frsMUONOF - is menu item active or not.
**			  frsMURNME - rename a menu item.
**	data		New name for menu item of rtcode is frsMURNME.
**			Zero or one if rtcode is frsMUONOF.
**
** Outputs:
**	None.
**
**	Returns:
**		TRUE	If inquire operation is known.
**		FALSE	If passed an unknown inquire operation.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	09/01/92 (dkh) - Initial version.
*/
i4
IIRTsetmu(RUNFRM *rfrm, char *mu_name, i4 rtcode, i4 *data)
{
	if (rfrm == NULL)
	{
		/*
		**  This shouldn't happen but just in case.
		*/
		IIFDerror(RTFRACT, 0, NULL);
		return(FALSE);
	}

	if (!IIFRrarunfrmactive(rfrm))
	{
		IIFDerror(E_FI1FF6_8182, 1, rfrm->fdrunfrm->frname);
		return(FALSE);
	}

	switch(rtcode)
	{
		case frsMUONOF:
			IIFRmcMuChg(rfrm, mu_name, *data);
			break;

		case frsMURNME:
			IIFRmenurename(rfrm, mu_name, (char *) data);
			break;

		default:
			return(FALSE);
	}

	return(TRUE);
}
