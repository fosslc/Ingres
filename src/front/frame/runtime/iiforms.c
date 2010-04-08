/*	iiforms.c
**
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<nm.h>		/* 6-x_PC_80x86 */
# include	<pc.h>		/* 6-x_PC_80x86 */
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
# include	<me.h>
# include	<er.h>
# include	<rtvars.h>
# include	<lqgdata.h>

/**
** Name:	iiforms.c	-	Start the forms system
**
** Description:
**
**	Public (extern) routines defined:
**		IIforms()	-	Start the forms system
**		IIisfrm()	-	Test if form currently displayed
**		IIsctlblk()	-	Set up forms control block
**
**	Private (static) routines defined:
**
** History:
**	16-feb-83  -  Extracted from the original runtime.c (jen).
**	18-oct-85  -  Added IIisfrm to check if a form is currently
**		      displayed.
**	12/23/86 (dkh) - Added support for new activations.
**	04-apr-1987 (peter)
**		Added call to iiugfrs_setting to turn on display
**		routines for errors and messages.
**	13-jul-87 (bruceb)	Changed memory allocation to use [FM]Ereqmem.
**	08/14/87 (dkh) - ER changes.
**	08/26/87 (dkh) - Changes for 8000 series error numbers.
**	09/01/87 (dkh) - Added explicit include of <frserrno.h>.
**	09/05/87 (dkh) - Added reference to IIerrdisp().
**	12/18/87 (dkh) - Moved iiugfrs_setting() to after forms system is up.
**			 This allows low level routines to use IIUGerr
**			 to print out errors.
**	05/20/88 (dkh) - Updated for dealing with control block changes.
**	28-nov-88 (bruceb)
**		Pass IIfrscb->frs_event down to initfd() so that routines in
**		FT can use the global event control block (for timeout).
**		Requires that global control block be allocated before
**		calling initfd.
**	23-mar-89 (bruceb)
**		Set 'entry activation allowed' as default condition.
**		Instead of passing frs_event to initfd(), pass down
**		IIfrscb.  Allows use of frs_actval in ft (for EA.)
**	07-jul-89 (bruceb)
**		Pass down IIFRrfcb to initfd()--for entry activation and
**		derivation processing.
**	07/12/89 (dkh) - Added support for emerald internal hooks.
**	08/01/89 (dkh) - Added support for aggregate processing via
**			 callback routines.
**	27-sep-89 (bruceb)
**		Default the field edit function on, shell key off.
**      12-oct-90 (stevet)
**              Added IIUGinit call to initialize character set attribute
**              table.
**	11/02/90 (dkh) - Replaced IILIBQgdata with IILQgdata().
**	01/27/92 (dkh) - Changed the way IIFDdsrDSRecords is set up so that
**			 it is not a link problem for various forms utilities.
**	04/23/94 (dkh) - Changed code to only allow field exit via up/down
**			 arrow keys if the user requests it.  This is for
**			 backwards compatibility so existing applications
**			 won't break.
**      04/23/96 (chech02)
**         fixed compiler complaint for function prototypes. (windows 3.1 port).
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	18-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
**	18-Aug-2009 (kschendel) 121804
**	    Update some of the function declarations to fix gcc 4.3 problems.
**/

FUNC_EXTERN	i4	IIlang();
FUNC_EXTERN	STATUS	IIsctlblk();
FUNC_EXTERN	VOID	FTmessage();	/* Address of forms msg routine */
FUNC_EXTERN	i4	IIboxerr();	/* Address of forms err routine */
FUNC_EXTERN	i4	IITBeaEntAct();
FUNC_EXTERN	i4	IITBgrGetRow();
FUNC_EXTERN	i4	*IITBgfGetFlags();
FUNC_EXTERN	DB_DATA_VALUE	*IITBgidGetIDBV();

FUNC_EXTERN	STATUS	IITBasAggSetup();
FUNC_EXTERN	VOID	IITBanvAggNxVal();
FUNC_EXTERN	VOID	IIFVddfDeriveDbgFlg();

FUNC_EXTERN	i4	IITBldrLastDsRow();
GLOBALREF	i4	(*IIFDldrLastDsRow)();

FUNC_EXTERN	VOID    IITBdsrDSRecords();
GLOBALREF	void	(*IIFDdsrDSRecords)();

static	FRS_RFCB	*IIFRrfcb;
static  VOID 		IIFRdbprhandler();

/*{
** Name:	IIforms	-	Start the forms system
**
** Description:
**	This routine sets up everything necessary to run the forms system.
**	It determines the language.  Sets up the stdio ports.  Initializes
**	the frame driver.  A global variable, IILIBQgdata->form_on, is set 
**	for each routine to inquire whether the forms system is turned on 
**	or not.
**
**	This routine is part of RUNTIME's external interface.
**	
** Inputs:
**	language	Code for the host language
**
** Outputs:
**
** Returns:
**	i4	TRUE	If forms system started successfully
**		FALSE	If forms system could not be started
**
** Exceptions:
**	none
**
** Example and Code Generation:
**	## forms
**
**	IIforms(EQ_PASCAL);
**
** Side Effects:
**
** History:
**	9-nov-93 (robf)
**           Add dbprompthandler initialization
**	
*/

IIforms(language)
i4	language;
{
	if (!IILQgdata()->form_on)		/* If the forms aren't on yet*/
	{
	        if (IIUGinit() != OK)
		{
		        PCexit(FAIL);
		}

		IIFRgpinit();
		IIlang(language);	/* Set host lang for FRAME, LIBQ */
		IImain();			/* Open Libq System I/O */
		IItestfrs((i4) TRUE);		/* Start up testing (if used) */

		/*
		**  Set up global control block.
		**  Do so before initfd() so can pass down IIfrscb
		**  and IIFRrfcb.
		*/
		if (IIsctlblk() != OK)
		{
			return(FALSE);
		}

		IIFDldrLastDsRow = IITBldrLastDsRow;
		IIFDdsrDSRecords = IITBdsrDSRecords;

		if (initfd(IIfrscb, IIFRrfcb) == 0)	/* Start frame driver */
		{
			IILQgdata()->form_on = FALSE;
			FTclose(NULL);
			IIFDerror(RTTRMIN, 0, NULL);
			return (FALSE);
		}
		else
		{
			/* Set up routines for msgs */
			iiugfrs_setting(FTmessage, IIboxerr);

			IILQgdata()->form_on = TRUE;
			IILQgdata()->win_err = IIerrdisp;
			IILQgdata()->f_end = II_fend;	/* forms end for libq */
		}
		/*
		** Set dbprompthandler
		*/
		IILQshSetHandler(14, IIFRdbprhandler);
	}
	return (TRUE);
}



/*{
** Name:	IIisfrm	-	Is a form currently displayed
**
** Description:
**	Tests to see if these is a frame on the stack, and returns
**	TRUE if there is one.
**
** Inputs:
**
** Outputs:
**
** Returns:
**	bool	TRUE	if there is a frame on the stack
**		FALSE	if the stack is empty
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
IIisfrm()
{
	if (IIstkfrm != NULL)
	{
		return(TRUE);
	}
	return(FALSE);
}

/*{
** Name:	IIsctlblk	- Set up frs control block.
**
** Description:
**	Set up the control block that will be used by the forms
**	system to control activities in the system.  This is
**	new for 6.0.  The overall frs control block points
**	to two other control blocks.  One control block
**	contains information about what global activation
**	and validation options are set to for the frs.
**	The other control block gathers informtion about
**	interesting events that was triggered by user input.
**	
**	Also, set up the FRS_RFCB control block.
**
** Inputs:
**	None.
**
** Outputs:
**	None.
**	Returns:
**		OK	If control block was set up correctly.
**		FAIL	If control block could not be set up.
**	Exceptions:
**		None.
**
** Side Effects:
**	Global "IIfrscb" is set to point to allocated memory.
**	Static "IIFRrfcb" is set to point to allocated memory.
**
** History:
**	12/23/86 (dkh) - Initial version.
*/
STATUS
IIsctlblk()
{
	char	*sp;
	FRS_AVCB *frsav;

	/*
	**  Allocate memory for the various control blocks.
	*/
	if ((IIfrscb = (struct frs_cb *)MEreqmem((u_i4)0,
	    (u_i4)sizeof(struct frs_cb), TRUE, (STATUS *)NULL)) == NULL)
	{
		IIFDerror(RTNOCB, 0);
		return(FAIL);
	}

	if ((IIfrscb->frs_actval = (struct frs_actval_cb *)MEreqmem((u_i4)0,
	    (u_i4)sizeof(struct frs_actval_cb), TRUE, (STATUS *)NULL)) == NULL)
	{
		IIFDerror(RTNOCB, 0);
		return(FAIL);
	}
	if ((IIfrscb->frs_event = (struct frs_event_cb *)MEreqmem((u_i4)0,
	    (u_i4)sizeof(struct frs_event_cb), TRUE, (STATUS *)NULL)) == NULL)
	{
		IIFDerror(RTNOCB, 0);
		return(FAIL);
	}
	if ((IIfrscb->frs_globs = (struct frs_glob_cb *)MEreqmem((u_i4)0,
	    (u_i4)sizeof(struct frs_glob_cb), TRUE, (STATUS *)NULL)) == NULL)
	{
		IIFDerror(RTNOCB, 0);
		return(FAIL);
	}
	IIfrscb->frs_globs->enabled = EDITOR_FN; /* shell key off, editor on. */

	/*
	**  Check if user wants to enable field exits with up/down arrow keys.
	*/
	NMgtAt(ERx("II_FORMS_SET"), &sp);
	if (sp != NULL && STbcompare(sp, 0, ERx("exit_field_with_arrow_keys"),
		0, TRUE) == 0)
	{
		IIfrscb->frs_globs->enabled |= UP_DOWN_EXIT;
	}

	/*
	**  Set up initial setting for the activation and validation
	**  states.
	*/
	NMgtAt(ERx("II_FRS_ACTIVATE"), &sp);
	frsav = IIfrscb->frs_actval;
	if (sp != NULL && STcompare(sp, ERx("60")) == 0)
	{
		frsav->val_prev = frsav->val_mu =
		frsav->val_mi = frsav->val_frsk =
		frsav->act_prev = frsav->act_mu =
		frsav->act_mi = frsav->act_frsk = 1;
	}
	frsav->val_next = frsav->act_next = frsav->act_entry = 1;

	if ((IIFRrfcb = (struct frs_func_cb *)MEreqmem((u_i4)0,
	    (u_i4)sizeof(struct frs_func_cb), TRUE, (STATUS *)NULL)) == NULL)
	{
		IIFDerror(RTNOCB, 0);
		return(FAIL);
	}

	/*
	**  Set up function pointers.
	*/
	IIFRrfcb->newDSrow = IITBeaEntAct;
	IIFRrfcb->getTFrow = IITBgrGetRow;
	IIFRrfcb->getTFflags = IITBgfGetFlags;
	IIFRrfcb->getTFdbv = IITBgidGetIDBV;

	/*
	**  Set up call back routine for aggregate
	**  processing of a table field dataset.
	*/
	IIFRrfcb->aggsetup = IITBasAggSetup;

	/*
	**  Set up call back routine for getting
	**  the next value in table field dataset as
	**  part of aggregate processing.
	*/
	IIFRrfcb->aggnxval = IITBanvAggNxVal;

	return(OK);
}


/*{
** Name:	IIFRkeystroke - Internal hook to capture keystrokes.
**
** Description:
**	This is an internal hook (specifically for emerald) to allow
**	an emerald component to capture all keystrokes typed by the user.
**	THIS IS NOT (REPEAT NOT) FOR GENERAL USERS.
**
** Inputs:
**	fptr	Pointer to routine to send keytroke info.  If NULL
**		is passed, keystroke capture is cancelled.
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
**	07/12/89 (dkh) - Initial version.
**	09/05/89 (dkh) - Changes for per display loop activation.
*/
i4  (*IIFRkeystroke(fptr))()
i4	(*fptr)();
{
	i4	(*old)();

	old = IIfrscb->frs_globs->key_intcp;
	IIfrscb->frs_globs->key_intcp = fptr;

	return(old);
}
/*
** Name: IIFRdbprhandler
**
** Description:
**	This is the default dbprompthandler set by IIforms, it is essentially
**	a wrapper for IIprmptio() 
**
** History:
**	9-nov-93 (robf)
**	  Created.
*/
static VOID
IIFRdbprhandler(
	char 	*mesg,
	i4	noecho,
	char	*reply,
	i4	replen
)
{
	IIprmptio(noecho, mesg, 0, 1, DB_CHA_TYPE, replen, reply);
}
