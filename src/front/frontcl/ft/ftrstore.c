/*
**  FTrestore
**
**  Copyright (c) 2004 Ingres Corporation
**
**  static	char	*Sccsid = "@(#)ftrstore.c	1.2	10/15/90";
**
**  History:
**	??/??/?? (dkh) - Initial version.
**	08/21/87 (dkh) - Changed restore alogrithm.
**	09/04/87 (dkh) - Added new argument to TDrestore() call.
**	04/04/90 (dkh) - Integrated MACWS changes.
**	04/18/90 (brett) - Added call to IITDfalsestop() for
**		a special windex mode.
**	05/22/90 (dkh) - Enabled MACWS changes.
**	08/29/90 (dkh) - Integrated porting change ingres6202p/131520.
**	10/17/90 (dkh) - Integrated round 4 of macws changes.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<te.h>
# include	"ftframe.h"

GLOBALREF	bool	IIFTscScreenCleared;

static	i4	cur_mode = FT_FORMS;

VOID
FTrestore(mode)
i4	mode;
{
	if (mode == cur_mode)
		return;

	if (mode == FT_FORMS)
	{
		TDrestore(TE_FORMS, FALSE);
		TDclear(curscr);
		TDrefresh(curscr);
		IIFTscScreenCleared = TRUE;

# ifdef DATAVIEW
		_VOID_ IIMWrmwResumeMW();
# endif	/* DATAVIEW */

	}
	else
	{
		/*
		** brett, False stop windex command string. Since
		** the shell call will put out the VE termcap before
		** the shell call and VS after the shell returns, we
		** must stop windex from closing down when it receives
		** the VE string and restarting when it receives the
		** Vs string.  This routine will cause a disable,
		** enable instead.
		*/
		IITDfalsestop();

# ifdef DATAVIEW
		_VOID_ IIMWsmwSuspendMW();
# endif	/* DATAVIEW */

		TDrestore(TE_NORMAL, FALSE);
	}
	cur_mode = mode;
}

/*{
** Name:	FTprtrestore	-	Partially restore the forms system.
**
** Description:
**	Partially restores the forms systems so that forms calls
**	can be made, but the terminal will not be cleared.  The only
**	forms call that is guarenteed to work is a prompt.  Calls
**	to this must be followed as some point by a call to FTrestore(FT_FORMS).**	It is used by ABF and the switcher to display a prompt, but leave
**	any possible operating system errors on the screen.
**
** Side Effects:
**	Won't clear the screen.  Caller can do a prompt after.  Must
**	be followed by a call to FTrestore(FT_FORMS);
**
** History:
**	16-Feb-1986 (joe)
**		First Written
*/
FTprtrestore()
{
	TErestore(TE_FORMS);
}
