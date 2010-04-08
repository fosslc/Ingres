/*
**
**	rtsisys.c
**
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:	rtsisys.c - Support routines for set/inquire_frs frs.
**
** Description:
**	This file contains the routines to support the set/inquire_frs frs
**	statements.  Routines in this file are:
**	 - RTivalsys() - Inquire on validation options.
**	 - RTsvalsys() - Set validation options.
**	 - RTvaldecode() - Does real work of processing validation options.
**	 - RTchkcmd() - Check a passed in command for validity.
**	 - RTimap() - Inquire on a mapped command.
**	 - RTilabel() - Inquire on a label.
**	 - RTsmap() - Map a command.
**	 - RTslabel() - Map label
**	 - RTinqsys() - Entry point for inquire_frs frs statements.
**	 - RTsetsys() - Entry point for set_frs frs statements.
**
** History:
**	02/05/85 (JEN) - Initial version.
**	12/23/86 (dkh) - Added support for new activations.
**	02/14/87 (dkh) - Added header.
**	08/14/87 (dkh) - ER changes.
**	08/26/87 (dkh) - Changes for 8000 series error numbers.
**	09/01/87 (dkh) - Changed <eqforms.h> to "setinq.h"
**	09/01/87 (dkh) - Added explicit include of <frserrno.h>.
**	09/12/87 (dkh) - Added support for inquiring on error text.
**	10/02/87 (dkh) - Changed IIfderr to IIFRerr.
**	08-jan-88 (bruceb)
**		RTsetsys and RTinqsys now get 'nat *ptr1', not 'nat ptr1'.
**	11-nov-88 (bruceb)
**		Handle set and inquire on timeout.  Also handle inquire
**		on command when timeout occurred.
**		Swapped return values for frsHEIGHT and frsWIDTH.
**	02/18/89 (dkh) - Fixed venus bug 4546.
**	09-mar-89 (bruceb)
**		Handle set and inquire on ability to entry activate.
**	24-apr-89 (bruceb)
**		Handle set and inquire of shell key enable.
**	08/04/89 (dkh) - Added support for mapping arrow keys.
**	08/27/89 (dkh) - Added support for 80/132.
**	08/28/89 (dkh) - Added support for inquiring on frskey that
**			 caused activation.
**	27-sep-89 (bruceb)
**		Added set/inquire of editor enable/disable.  Changed code
**		for set/inquire of shell enable/disable.
**	09/23/89 (dkh) - Porting changes integration.
**	09/28/89 (dkh) - Put in change so that turning on/off the menumap
**			 option will cause the menuline to be recalculated.
**	07-feb-90 (bruceb)
**		Added support for Getmessage display/supression.
**	13-feb-90 (bruceb)
**		Modified the above support after LRC/FRC decisions.
**	03/20/91 (dkh) - Added support for (alerter) event in FRS.
**	04/20/91 (dkh) - Changed support for alerter to conform to LRC
**			 approved semantics.
**	09/03/92 (dkh) - Addded support for frsODMSG (controlling behavior
**			 of "Out of Data" message when scrolling table
**			 fields).
**	11-jan-1996 (toumi01; from 1.1 axp_osf port)
**		Added 09/10/93 (kchin)
**		Changed type of 'data' to PTR in RTsetsys(), since it holds
**		pointers.
**	25-apr-1996 (chech02)
**	    	Added FAR to IIfrscb for windows 3.1 port.
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


# include	<compat.h>
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
# include	"setinq.h"
# include	<fsicnsts.h>
# include	<er.h>
# include	<rtvars.h>

# define	MAX_FKEYS	40

FUNC_EXTERN	i4	IIFRerr();
FUNC_EXTERN	char	*IIFDgetGetErrorText();
FUNC_EXTERN	VOID	FTinqmap();
FUNC_EXTERN	i4	FDlastcmd();
FUNC_EXTERN	VOID	IIFRrmReformatMenu();
FUNC_EXTERN	VOID	FTterminfo();
FUNC_EXTERN	VOID	IIFDaeiAlerterEventIo();

#ifdef WIN16
GLOBALREF	FRS_CB	*FAR IIfrscb;
#else
GLOBALREF	FRS_CB	*IIfrscb;
#endif
GLOBALREF	char	*IIsimapfile;

GLOBALREF	FT_TERMINFO	frterminfo;

/*{
** Name:	RTivalsys - Entry point for inquiring on validation options.
**
** Description:
**	RTivalsys() is the entry point for inquring on whether a
**	particular validation option is set or not.  The real work
**	is done by RTvaldecode().  A one is returned if the option
**	is set, zero otherwise.
**
** Inputs:
**	flags	Validation option inquiring on.
**
** Outputs:
**	Returns:
**		on_off	Returns one if validation is ON.  Zero if OFF.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	08/30/85 (dkh) - Initial version.
**	02/14/87 (dkh) - Added procedure header.
**	
*/
i4
RTivalsys(i4 flags)
{
	return(RTvaldecode(FALSE, flags, 0));
}



/*{
** Name:	RTsvalsys - Support for setting a validtion option.
**
** Description:
**	This routine is the entry point to support setting/resetting
**	a validation option.  The real work is done by RTvaldecode().
**	Nothing happens if the value to set for the option is not
**	zero or one.
**
** Inputs:
**	flag	Validation option to set.
**	value	The value to set the option to.  Must be zero or one.
**
** Outputs:
**	Returns:
**		Nothing.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	08/30/85 (dkh) - Initial version.
**	02/14/87 (dkh) - Added procedure header.
*/
VOID
RTsvalsys(i4 flag, i4 value)
{
	i4	validate;

	if (value == 0 || value == 1)
	{
		validate = value;
	}
	else
	{
		return;
	}
	_VOID_ RTvaldecode(TRUE, flag, validate);
}




/*{
** Name:	RTvaldecode
**
** Description:
**	Utility routine used by RTivalsys() and RTsvalsys() to
**	process a set/inquire on the various validation options.
**	If argument "set" is positive then a set request is in
**	progress.  For an inquire request, the validation option
**	setting is the routine's return value.  Currently supported
**	validation option are:
**	 - Validate current field when going to the next field/column.
**	 - Validate current field when going to the previous field/column.
**	 - Validate current field when the menu key is pressed.
**	 - Validate current field when any frs key is selected.
**	 - Validate current field when any menu item is selected.
**	 - Activate on current field going to next field/column.
**	 - Activate on current field going to previous field/column.
**	 - Activate on current field when menu key is pressed.
**	 - Activate on current field when any frs key is selected.
**	 - Activate on current field when any menu item is selected.
**	 - Activate on entry to field.
**
** Inputs:
**	set	Tells routine if a set or inquire is to be processed.
**	flag	Validation option to process.
**	value	Value to set validation option to.
**
** Outputs:
**	Returns:
**		on_off	Validation option setting.  One if on, zero otherwise.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	08/30/85 (dkh) - Initial version.
**	12/23/86 (dkh) - Added support for new activations.
**	02/14/87 (dkh) - Added procedure header.
*/
i4
RTvaldecode(i4 set, i4 flag, i4 value)
{
	i4		retval = 0;
	FRS_AVCB	*avcb;
	bool		*ptr;

	avcb = IIfrscb->frs_actval;
	switch(flag)
	{
		case frsVALNXT:
			ptr = &(avcb->val_next);
			break;

		case frsVALPRV:
			ptr = &(avcb->val_prev);
			break;

		case frsVMUKY:
			ptr = &(avcb->val_mu);
			break;

		case frsVKEYS:
			ptr = &(avcb->val_frsk);
			break;

		case frsVMENU:
			ptr = &(avcb->val_mi);
			break;

		case frsACTNXT:
			ptr = &(avcb->act_next);
			break;

		case frsACTPRV:
			ptr = &(avcb->act_prev);
			break;

		case frsAMUKY:
			ptr = &(avcb->act_mu);
			break;

		case frsAKEYS:
			ptr = &(avcb->act_frsk);
			break;

		case frsAMUI:
			ptr = &(avcb->act_mi);
			break;
		
		case frsENTACT:
			ptr = &(avcb->act_entry);
			break;

		default:
			break;
	}
	if (set)
	{
		*ptr = (bool) value;
	}
	else
	{
		retval = (i4) *ptr;
	}
	return(retval);
}


/*{
** Name:	RTchkcmd - Check a command encoding for validity.
**
** Description:
**	Utility routine used by RTimap(), RTilabel(), RTsmap() and
**	RTslabel() to check that the encoding of a command is
**	known to the forms system.  Currently known commands are:
**	 - The entire set of frs commands (e.g., nextfield).
**	 - A set of frs keys (40 maximum).
**	 - A set of menu items (25 maximum).
**
** Inputs:
**	cmd	A numeric encoding of a forms system command to check.
**
** Outputs:
**	Returns:
**		TRUE	If the passed in command is known to the system.
**		FALSE	If command is unknown to the system.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	08/30/85 (dkh) - Initial version.
**	02/14/87 (dkh) - Added procedure header.
*/
i4
RTchkcmd(i4 cmd)
{
	i4	maxmu;
	i4	maxcmd;

	maxmu = FsiMU_OFST + MAX_MENUITEMS - 1;
	maxcmd = FsiFRS_OFST + MAX_FRS_KEYS - 1;

	if ((cmd < FsiNEXTFLD) || (cmd > maxcmd) ||
		(cmd > FsiCMDMAX && cmd < FsiMU_OFST) ||
		(cmd > maxmu && cmd < FsiFRS_OFST))
	{
		IIFDerror(SIBADCMD, 0);
		return(FALSE);
	}
	return(TRUE);
}



/*{
** Name:	RTimap - Inquire on what a command is mapped to.
**
** Description:
**	Entry point to support inquire_frs frs (map) statements.
**	Returns a textual representation of the key that the
**	passed in command is mapped to.
**
** Inputs:
**	cmd	Numeric encoding of command to find mapping for.
**
** Outputs:
**	string	Character buffer space to place textual representation
**		of what key the command is mapped to (e.g., control-K).
**		A command that is not mapped to anything will return an
**		empty string.
**		Buffer space is assumed to be large enough and the result
**		is null terminated.
**	Returns:
**		TRUE	Inquiry was completed.
**		FALSE	An unknown command was passed in.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	08/30/85 (dkh) - Initial version.
**	02/14/87 (dkh) - Added procedure header.
*/
i4
RTimap(i4 cmd, char *string)
{
	if (!RTchkcmd(cmd))
	{
		return(FALSE);
	}
	FTinqmap(cmd, string);
	return(TRUE);
}



/*{
** Name:	RTilabel - Find the label for a command.
**
** Description:
**	Entry point for supporting the inquire_frs frs (label)
**	statements.  Returns any label that has been attached
**	to a command known to the forms system.
**
** Inputs:
**	cmd	Numeric encoding of command to find label for.
**
** Outputs:
**	string	Character buffer space to place result in.  It is
**		assumed that the buffer is large enough and the
**		result will be null terminated.  An empty string
**		is returned if no label exists for the passed in
**		command.
**	Returns:
**		TRUE	Inquiry completed.
**		FALSE	If an unkown command was passed in.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	08/30/85 (dkh) - Initial version.
**	02/14/87 (dkh) - Added procedure header.
*/
i4
RTilabel(i4 cmd, char *string)
{
	if (!RTchkcmd(cmd))
	{
		return(FALSE);
	}
	FTinqlabel(cmd, string);
	return(TRUE);
}



/*{
** Name:	RTsmap - Set (map) a command to a key.
**
** Description:
**	Entry point to support set_frs frs (map) statements.
**	Maps a known forms system command to keys that are
**	supported by the system.  Supported keys are:
**	 - controlA to controlZ.
**	 - controlDEL and controlESC.
**	 - PF1 to PF40.
**
** Inputs:
**	cmd	Numeric encoding of a forms system command.
**	value	Numeric encoding of a key to map command to.
**
** Outputs:
**	Returns:
**		TRUE	If the command was mapped successfully.
**		FALSE	If an unknown command or bad key value
**			was passed in.
**	Exceptions:
**		None.
**
** Side Effects:
**	Various data structures in the FT layer will be changed
**	to reflect the new command/key mapping.
**
** History:
**	08/30/85 (dkh) - Initial version.
**	02/14/87 (dkh) - Added procedure header.
*/
i4
RTsmap(i4 cmd, i4 value)
{
	i4	maxpfkey;

	maxpfkey = FsiPF_OFST + MAX_FKEYS - 1;

	if (!RTchkcmd(cmd))
	{
		return(FALSE);
	}
	if ((value < FsiA_CTRL) ||
		(value > FsiDEL_CTRL && value < FsiPF_OFST) ||
		(value > maxpfkey && value < FsiUPARROW) ||
		(value > FsiLTARROW))
	{
		IIFDerror(SICTPFKY, 0);
		return(FALSE);
	}

	FTsetmap(cmd, value);

	IIFRrmReformatMenu((bool) TRUE);

	return(TRUE);
}



/*{
** Name:	RTslabel - Set (map) a label to a command.
**
** Description:
**	Entry point to support set_frs frs (label) statements.
**	A label may be attached to forms commands so that they
**	appear on the menu line or in the help facility to aid
**	users in finding the right key to use for selecting
**	the correct commands.
**	No length restriction exists for the label but it should
**	not be too long to cause an overflow of the menu line.
**
** Inputs:
**	cmd	Numeric encoding of forms system command.
**	string	Label to attached to above command.  No non-printing
**		characters should be embedded in the label.
**
** Outputs:
**	Returns:
**		TRUE	If label was successfully attached.
**		FALSE	If an unknown command was passed in.
**	Exceptions:
**		None.
**
** Side Effects:
**	Various data structures in the FT layer is updated to reflect
**	the new command and label mapping.
**
** History:
**	08/30/85 (dkh) - Initial version.
**	02/14/87 (dkh) - Added procedure header.
*/
i4
RTslabel(i4 cmd, char *string)
{
	if (!RTchkcmd(cmd))
	{
		return(FALSE);
	}
	FTsetlabel(cmd, string);

	IIFRrmReformatMenu((bool) TRUE);

	return(TRUE);
}


/*{
** Name:	IIFRstvSetTmoutVal - Set the timeout timer value.
**
** Description:
**	Set the number of seconds until forms system input times out.
**	If the number of seconds specified is out of the allowable range
**	(0 - 32767) then generate an error message and adjust the value
**	to the (appropriate) endpoint.
**
** Inputs:
**	seconds	Number of seconds until timeout.
**
** Outputs:
**	Returns:
**		VOID
**
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	29-nov-88 (bruceb)	Written.
*/
VOID
IIFRstvSetTmoutVal(i4 seconds)
{
    i4	numsecs = seconds;
    i4	errmax;
    i4	errmin;

    if (numsecs > MAXI2)
    {
	errmax = MAXI2;
	IIFDerror(SIBADTOV, 2, &errmax, &errmax);
	numsecs = MAXI2;
    }
    else if (numsecs < 0)
    {
	errmax = MAXI2;
	errmin = 0;
	IIFDerror(SIBADTOV, 2, &errmax, &errmin);
	numsecs = 0;
    }

    IIfrscb->frs_event->timeout = numsecs;
}


/*{
** Name:	RTinqsys - Support for inquire_frs frs statements.
**
** Description:
**	Main entry point for all inquire_frs frs statements.
**	Options supported are:
**	 - Last error number. (integer)
**	 - Terminal name user is using. (character)
**	 - The validation/activation options. (integer)
**	 - Are labels being displayed on the menu line. (integer)
**	 - Mapping between a command and a key. (character)
**	 - The label attached to a command. (character)
**	 - The current mapping file. (character)
**	 - Last command (that caused an activation). (integer)
**	 - Number of seconds on timeout command. (integer)
**	 - Shell key on/off. (integer)
**	 - Editor key on/off. (integer)
**	 - Getmessage display on/off. (integer)
**	Unsupported options:
**	 - Lotus menus.
**	 - Status line.
**
** Inputs:
**	ptr1	Numeric encoding of command to use for inquiring on
**		the key or label mapping.
**	ptr2	Pointer to the current form.  Will be used when
**		inquiring on the LAST COMMAND.
**	flag	Option inquiring on.
**
** Outputs:
**	data	Data space where result is to be placed.  If the result
**		is to be a character string, the data space is assumed
**		to be large enough to hold the result and the null
**		terminator.
**	Returns:
**		TRUE	If inquiry was completed.
**		FALSE	If an unknown option was passed in.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	02/05/85 (jen) - Initial version.
**	08/30/85 (dkh) - Added 4.0 features.
**	12/23/86 (dkh) - Added support for new activations.
**	02/14/87 (dkh) - Added procedure header.
**	23-mar-87 (mgw)	Bug # 12029
**		Don't STcopy() a NULL pointer in case where you've done an
**		inquire_forms frs (field = mapfile) without first having done
**		a set_forms frs (mapfile = 'filename').
*/
i4
RTinqsys(i4 *ptr1, FRAME *ptr2, i4 flag, i4 *data)
{
	i4		cmd = *ptr1;
	char		*cp;
	FT_TERMINFO	terminfo;

	switch (flag)
	{
		case frsERRNO:
			*data = IIFRerr();
			break;

		case frsERRTXT:
			STcopy(IIFDgetGetErrorText(), (char *) data);
			break;

		case frsTERM:
			STcopy(frterminfo.name, (char *) data);
			break;

		case frsVALNXT:
		case frsVALPRV:
		case frsVMUKY:
		case frsVKEYS:
		case frsVMENU:
		case frsACTNXT:
		case frsACTPRV:
		case frsAMUKY:
		case frsAKEYS:
		case frsAMUI:
		case frsENTACT:
			*data = RTivalsys(flag);
			break;

		case frsMENUMAP:
			*data = FTinqmmap();
			break;

		case frsMAP:
			RTimap(cmd, (char *)data);
			break;

		case frsLABEL:
			RTilabel(cmd, (char *) data);
			break;

		case frsMAPFILE:
			/* Bug 12029 - Don't STcopy a NULL pointer */
			if (IIsimapfile != NULL)
			{
				STcopy(IIsimapfile, (char *) data);
			}
			else
			{
				cp = (char *) data;
				*cp = EOS;
			}
			break;

		case frsPCSTYLE:
		case frsSTATLN:
		case frsVFRSKY:
			*data = 0;
			break;

		case frsLCMD:
			/*
			** Value should be CMD_UNDEF until commands have been
			** issued, at which time the latest command value is
			** returned, unless a timeout occurred, in which case
			** value is CMD_TMOUT.
			*/
			*data = FDlastcmd(IIfrscb->frs_event);
			break;

		case frsLFRSK:
			/*
			**  If last event was a frskey activation, then
			**  return actual frskey number.
			**  Otherwise, return -1.
			*/
			if (IIfrscb->frs_event->event == fdopFRSK)
			{
				*data = IIfrscb->frs_event->mf_num + 1;
			}
			else
			{
				*data = -1;
			}
			break;

		case frsCROW:
			*data = (IIfrscb->frs_event)->ycursor + 1;
			break;

		case frsCCOL:
			*data = (IIfrscb->frs_event)->xcursor + 1;
			break;

		case frsHEIGHT:
			FTterminfo(&terminfo);
			*data = terminfo.lines;
			break;

		case frsWIDTH:
			FTterminfo(&terminfo);
			*data = terminfo.cols;
			break;

		case frsTMOUT:
			*data = IIfrscb->frs_event->timeout;
			break;

		case frsSHELL:
			*data = ((IIfrscb->frs_globs->enabled & SHELL_FN)
								== SHELL_FN);
			break;

		case frsEDITOR:
			*data = ((IIfrscb->frs_globs->enabled & EDITOR_FN)
								== EDITOR_FN);
			break;

		case frsGMSGS:
			*data = ((IIfrscb->frs_globs->enabled & GETMSGS_OFF)
							    != GETMSGS_OFF);
			break;

		case frsODMSG:
			if (IIfrscb->frs_globs->enabled & ODMSG_OFF)
			{
				/*
				**  Return 0 if messaging is turned off.
				*/
				*data = 0;
			}
			else if (IIfrscb->frs_globs->enabled & ODMSG_BELL)
			{
				/*
				**  Return 2 if messaging is set to ring
				**  the bell.
				*/
				*data = 2;
			}
			else
			{
				/*
				**  Return 1 if messaging is on.
				*/
				*data = 1;
			}

		default:
			return (FALSE);
	}
	return (TRUE);
}



/*{
** Name:	RTsetsys - Support for set_frs frs statements.
**
** Description:
**	Main entry point to support the set_frs frs statements.
**	Supported options are:
**	 - The validation/activation settings.
**	 - Setting/resetting the display of the menu labels.
**	 - Mapping a command to a key.
**	 - Mapping a label to a command.
**	 - Setting a new mapping file.
**	 - Setting the timeout value.
**	 - Enabling the shell key (on/off).
**	 - Enabling the editor key (on/off).
**	 - Enabling getmessage display (on/off).
**	Unsupported options:
**	 - Lotus menus.
**	 - Status line.
**
** Inputs:
**	ptr1	Numeric encoding of command to use when setting
**		(mapping) a command to a key or label.
**	ptr2	Unused pointer to a FRAME structure.
**	flag	Option to set.
**	data	Value to use in setting the option.
**
** Outputs:
**	Returns:
**		TRUE	If requested option was completed.
**		FALSE	If an unknown option was passed in.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	02/05/85 (jen) - Initial versions.
**	08/30/85 (dkh) - Added 4.0 features.
**	12/23/86 (dkh) - Added support for new activations.
**	02/14/87 (dkh) - Added procedure header.
*/
i4
RTsetsys(i4 *ptr1, FRAME *ptr2, i4 flag, PTR data)
{
	i4	cmd;
	i4	value;

	cmd = *ptr1;

	switch(flag)
	{
		case frsVALNXT:
		case frsVALPRV:
		case frsVMUKY:
		case frsVKEYS:
		case frsVMENU:
		case frsACTNXT:
		case frsACTPRV:
		case frsAMUKY:
		case frsAKEYS:
		case frsAMUI:
		case frsENTACT:
			RTsvalsys(flag, *((i4 *) data));
			break;

		case frsMENUMAP:
			FTsetmmap(*((i4 *) data));
			IIFRrmReformatMenu((bool) TRUE);
			break;

		case frsMAP:
			RTsmap(cmd, *((i4 *) data));
			break;

		case frsLABEL:
			RTslabel(cmd, data);
			break;

		case frsMAPFILE:
			IImapfile(data);
			IIFRrmReformatMenu((bool) TRUE);
			break;

		case frsTMOUT:
			IIFRstvSetTmoutVal(*((i4 *) data));
			break;

		case frsSHELL:
			value = *((i4 *) data);
			if (value == 0)
				IIfrscb->frs_globs->enabled &= ~SHELL_FN;
			else if (value == 1)
				IIfrscb->frs_globs->enabled |= SHELL_FN;
			break;

		case frsEDITOR:
			value = *((i4 *) data);
			if (value == 0)
				IIfrscb->frs_globs->enabled &= ~EDITOR_FN;
			else if (value == 1)
				IIfrscb->frs_globs->enabled |= EDITOR_FN;
			break;

		case frsGMSGS:
			value = *((i4 *) data);
			if (value == 0)
				IIfrscb->frs_globs->enabled |= GETMSGS_OFF;
			else if (value == 1)
				IIfrscb->frs_globs->enabled &= ~GETMSGS_OFF;
			break;

		case frsPCSTYLE:
		case frsSTATLN:
		case frsVFRSKY:
			return(FALSE);

		case frsODMSG:
			value = *((i4 *) data);
			if (value == 0)
			{
				/*
				**  Set flag to turn off messaging.
				**  Also, turn off bell flag.
				*/
				IIfrscb->frs_globs->enabled |= ODMSG_OFF;
				IIfrscb->frs_globs->enabled &= ~ODMSG_BELL;
			}
			else if (value == 1)
			{
				/*
				**  Turn off flags that disable messaging
				**  or ring the bell.
				*/
				IIfrscb->frs_globs->enabled &=
					~(ODMSG_OFF | ODMSG_BELL);
			}
			else if (value == 2)
			{
				/*
				**  Enable ringing the bell.
				**  Also, turn off the disable messaging flag.
				*/
				IIfrscb->frs_globs->enabled |= ODMSG_BELL;
				IIfrscb->frs_globs->enabled &= ~ODMSG_OFF;
			}
			else
			{
				/*
				**  Not a valid value.
				*/
				return(FALSE);
			}

		default:
			return (FALSE);
	}
	return(TRUE);
}

