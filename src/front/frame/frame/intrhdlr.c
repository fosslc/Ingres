/*
**	intrhdlr.c
**
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:	intrhdlr.c - Interrupt handler for front ends.
**
** Description:
**	The exception handler for forms based front ends is
**	defined here.
**	Routines defined:
**	- FDhandler - Exception handler for forms based front ends.
**
** History:
**	02/15/87 (dkh) - Added header.
**	08/14/87 (dkh) - ER changes.
**	09/26/89 (dkh) - Added return type to FDhandler().
**	11/02/90 (dkh) - Replaced IILIBQgdata with IILQgdata().
**/

# include	<compat.h>
# include	<pc.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<si.h>
# include	<ex.h>
# include	<er.h>
# include	<lqgdata.h>
# include	"erfd.h"

/*
**
*/


/*{
** Name:	FDhandler - Exception handler for front ends.
**
** Description:
**	This routine is the exception handler for forms based
**	front ends.  It is called when one of our
**	frontends is interrupted (ie - a user
**	types cntrl-C to exit or some other interrupt
**	character).  It moves the cursor to the bottom
**	of the screen and uses TErestore() to exit
**	gracefully.  The terminal will not be left in
**	a weird state (like raw mode) if this routine
**	is used to handle interrupts.  To use this routine
**	the following call was put in the main() routine
**	of the frontends:
**		EXdeclare(FDhandler, &context);
**	Exceptions that this routine handles are:
**	- Interrupt
**	- Kill
**	- Quit
**	- Out of input
**	- Bus error (AV on VMS)
**	- Segmentation violation (AV on VMS)
**
** Inputs:
**	ex	Exception arguments to process.
**
** Outputs:
**	Returns:
**		EXRESIGNAL	To resignal the exception for someone
**				else to handle if it does not want
**				to handle it.
**	Exceptions:
**		None.
**
** Side Effects:
**	The process will exit if the exception is handled here.
**
** History:
**	??/??/?? (dkh) - Initial version.
**	Added 6/20/83  (nml)
**	Modified 10/27/83 (nml) Put in call to INcleanup() to fix bug #1629.
**	02/15/87 (dkh) - Added procedure header.
**	07/22/87 (dkh) - Fix for jup bug 552.  Now resets terminal when
**			 EXBUSERR and EXSEGVIO is found.
*/
i4
FDhandler(ex)
EX_ARGS *ex;
{
	char	*cp;

	if (ex->exarg_num == EXINTR || ex->exarg_num == EXKILL ||
		ex->exarg_num == EXQUIT || ex->exarg_num == EXTEEOF ||
		ex->exarg_num == EXBUSERR || ex->exarg_num == EXSEGVIO)
	{
		if (IILQgdata()->form_on)
		{
			FTclose(ERget(S_FD000E_Exiting));
		}
		else
		{
			SIfprintf(stdout, ERget(S_FD000E_Exiting));
			SIflush(stdout);
		}
		if (ex->exarg_num == EXINTR)
		{
			cp = ERget(S_FD000F_INTERRUPT);
		}
		else if (ex->exarg_num == EXKILL)
		{
			cp = ERget(S_FD0010_KILLED);
		}
		else if (ex->exarg_num == EXQUIT)
		{
			cp = ERget(S_FD0011_QUIT_signal);
		}
		else if (ex->exarg_num == EXTEEOF)
		{
			cp = ERget(S_FD0012_out_of_input);
		}
		else if (ex->exarg_num == EXBUSERR || ex->exarg_num == EXSEGVIO)
		{
			cp = ERget(S_FD0013_Access_Viola);
		}
		SIfprintf(stdout, cp);
		SIflush(stdout);
		IIresync();
		IIexit();
		PCexit(0);
	}

	return(EXRESIGNAL);
}
