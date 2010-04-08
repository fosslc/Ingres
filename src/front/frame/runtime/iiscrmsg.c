/*
**	iiscrmsg.c
**
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<si.h>
# include	<ft.h>
# include	<fmt.h>
# include	<adf.h>
# include	<frame.h>
# include	<menu.h>
# include	<runtime.h>
# include	<er.h>
# include	<fsicnsts.h>
# include	<erfi.h>
# include	"iigpdef.h"
# include	<lqgdata.h>
# include	<rtvars.h>

/**
** Name:	iiscrmsg.c	-	Print a message
**
** Description:
**
**	Public (extern) routines defined:
**		IIsmessage()	-	C printf style args
**		IImessage()	-	string
**	Private (static) routines defined:
**
** History:
**	08/14/87 (dkh) - ER changes.
**	06-dec-88 (bruceb)
**		Initialize event to fdNOCODE before calling FT message
**		routines.  On return, code should be either fdNOCODE or
**		fdopTMOUT.
**	10-mar-89 (bruceb)
**		Splitting up 'event' and 'last command'.  Now, event doesn't
**		change here.  It's the last command that gets set to fdNOCODE
**		or fdopTMOUT.  Not modifying the last command if a non-popup
**		message.
**	01/11/90 (dkh) - Before doing message, check to see if we need to
**			 do a redisplay.
**	01/17/90 (dkh) - Only force a redisplay for a popup style message.
**	11/02/90 (dkh) - Replaced IILIBQgdata with IILQgdata().
**	08/15/91 (dkh) - Put in changes to not redraw entire screen when
**			 executing a message in the (runtime) context
**			 of an initialize block.
**	25-jan-1996 (toumi01; from 1.1 axp_osf port) (schte01)
**		Added 09/10/93 (kchin) - Changed type of args:
**		a1-a10 from i4  to PTR in IIsmessage(). As these args
**		are being used to pass addresses.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	24-Aug-2009 (kschendel) 121804
**	    Update some of the function declarations to fix gcc 4.3 problems.
**/


/*{
** Name:	IIsmessage	-	Print a message
**
** Description:
**	Print a message at the bottom of the screen.  Takes C printf
**	style arguments.  Generates a call to FTmessage.
**
**	EQUEL will generate a call to this routine for a '## MESSAGE'
**	statement if the host language is C.
**
**	This routine is part of RUNTIME's external interface.
**	
** Inputs:
**	buf	The message format string
**	a1-a10	The arguments 
**
** Outputs:
**	
**
** Returns:
**
** Exceptions:
**	none
**
** Example and Code Generation:
**	## message ("Name '%s' not found", charptr); -- unsupported
**
**	IISmessage("Name '%s' not found", charptr);
**
** Side Effects:
**
** History:
**	4/88 (bobm)	changes for popups.  Also, increased temp buffer
**			sizes to allow for larger messages which are likely
**			with popups.  Note that this method of handling
**			a variable argument list is a violation of standards
**			which may eventually have to be changed.
**	
*/

IISmessage(buf, a1,a2,a3,a4,a5,a6,a7,a8,a9,a10)
char	*buf;
PTR	a1,a2,a3,a4,a5,a6,a7,a8,a9,a10;
{
	reg	char	*s;
	char	mbuf[DB_MAXSTRING+2];
	char	mb2[DB_MAXSTRING+2];
	POPINFO	pop;
	i4	style;
	i4	event;

	if (buf == NULL)
		return;

	s = IIstrconv(II_TRIM, buf, mbuf, (i4)DB_MAXSTRING);

	if (!IILQgdata()->form_on)
	{
		/* BUG 4976 - Allow for backward compatibilty (ncg) */
		SIprintf( s, a1,a2,a3,a4,a5,a6,a7,a8,a9,a10 );
		SIprintf( ERx("\n") );		
		SIflush( stdout ); /* BUG 5968 - Don't fall through (ncg) */
		return;
	}

	style = GP_INTEGER(FSP_STYLE);
	pop.begy = GP_INTEGER(FSP_SROW);
	pop.begx = GP_INTEGER(FSP_SCOL);
	pop.maxy = GP_INTEGER(FSP_ROWS);
	pop.maxx = GP_INTEGER(FSP_COLS);

	IIFRgpcontrol(FSPS_RESET,0);

	if (style == FSSTY_POP)
	{
		/*
		**  Check to see if we need to do a redisplay before
		**  displaying the message.
		*/
		if (IIstkfrm && !(IIstkfrm->rfrmflags & RTACTNORUN) &&
			IIstkfrm->fdrunfrm->frmflags & fdDISPRBLD)
		{
			IIredisp();
		}

		event = IIfrscb->frs_event->event;
		IIfrscb->frs_event->event = fdNOCODE;

		if (IIFRbserror(&pop, MSG_MINROW, MSG_MINCOL,
			ERget(F_FI0001_Message)))
		{
			/* Restore event and set 'last command' to fdNOCODE. */
			IIfrscb->frs_event->lastcmd = fdNOCODE;
			IIfrscb->frs_event->event = event;
			return;
		}
		STprintf(mb2, s, a1,a2,a3,a4,a5,a6,a7,a8,a9,a10);
		IIFTpmsg(mb2, &pop, IIfrscb);

		/*
		** Restore event and set the 'last command'
		** (fdNOCODE or fdopTMOUT).
		*/
		IIfrscb->frs_event->lastcmd = IIfrscb->frs_event->event;
		IIfrscb->frs_event->event = event;
	}
	else
	{
		FTmessage(s, FALSE, FALSE, a1,a2,a3,a4,a5,a6,a7,a8,a9,a10);
	}
}

/*{
** Name:	IImessage	-	Print a message
**
** Description:
**	Print a string as a message.
**
**	EQUEL will generate a call to this routine for a '## MESSAGE'
**	statement if the host language is anything other than C.
**
** Inputs:
**	buf		String containing the message to print
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
**	## message "Employee not found"; -- non-C
**
**	IImessage("Employee not found");
**
** Side Effects:
**
** History:
**	4/88 (bobm)	since the routine basically called IISmessage before,
**			saved some string shuffling by not reduplicating work
**			and passing string as an argument with "%s" rather
**			than converting "%" to "%%" in a temp buffer. Old
**			code also returned value of IISmessage which was VOID.
**			Changed to reflect what I assume was intended.
*/

IImessage( buf )
char	*buf;
{
	if (buf == NULL)
		return(FALSE);

	IISmessage(ERx("%s"), buf);
	return(TRUE);
}
