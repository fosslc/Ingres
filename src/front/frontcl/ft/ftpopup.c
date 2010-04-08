/*
**	ftpopup.c
**
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<te.h>
# include	"ftframe.h"
# include	<frsctblk.h>
# include	<uigdata.h>

/**
** Name:	ftpopup.c - Popup messages and prompts.
**
** Description:
**	This file contains the FT interface routines for
**	handling popup messages and prompts.
**
** History:
**	04/09/88 (dkh) - Initial version.
**	05/27/88 (dkh) - Added new parameter on call to IIFTpumPopUpMessage().
**	04/03/90 (dkh) - Integrated MACWS changes.
**	04/14/90 (dkh) - Changed "# if defined" to "# ifdef".
**	08/29/90 (dkh) - Integrated porting change ingres6202p/131520.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/*{
** Name:	IIFTpmsg - External entry point to display a popup message.
**
** Description:
**	This is the external entry point for displaying a popup message.
**	Note that we are not providing any parameter substitution here.
**	Message must have been formatted before being passed to us.
**
** Inputs:
**	msg	Message to display.
**	popinfo	Popup information such as location and size.
**	cb	Forms system control block.
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
**	Menu line is cleared.
**
** History:
**	04/09/88 (dkh) - Initial version.
**	28-aug-1990 (Joe)
**	    Changed IIUIgdata to a function.
**	30-aug-1990 (Joe)
**	    Changed the name of IIUIgdata to IIUIfedata.
*/
VOID
IIFTpmsg(msg, popinfo, cb)
char	*msg;
POPINFO	*popinfo;
FRS_CB	*cb;
{
	/*
	**  Flush typeahead so popup message does not flash by
	**  before user can read it.
	*/
	if (!IIUIfedata()->testing)
	{
		TEinflush();

# ifdef	DATAVIEW
		_VOID_ IIMWfiFlushInput();
# endif	/* DATAVIEW */
	}

	/*
	**  Clear out menu line so user is not mislead by presence
	**  of a menu line.
	*/
	TDclear(stdmsg);
	TDrefresh(stdmsg);

	_VOID_ IIFTpumPopUpMessage(msg, FALSE, popinfo->begy, popinfo->begx,
		popinfo->maxy, popinfo->maxx, cb, FALSE);
}


/*{
** Name:	IIFTpprompt - External entry point to display a popup prompt.
**
** Description:
**	This is the external entry point for displaying a popup prompt.
**
** Inputs:
**	pmsg	Prompt message to display.
**	echo	Toggle switch for echoing user input.
**	popinfo	Pipup information such as location and size.
**	cb	Forms system control block.
**
** Outputs:
**	None.
**
**	Returns:
**		result	Buffer for saving user input.
**	Exceptions:
**		None.
**
** Side Effects:
**	Menu line is cleared.
**
** History:
**	04/09/88 (dkh) - Initial version.
*/
VOID
IIFTpprompt(pmsg, echo, popinfo, cb, result)
char	*pmsg;
i4	echo;
POPINFO	*popinfo;
FRS_CB	*cb;
char	*result;
{
	/*
	**  Clear out menu line so user is not mislead by presence
	**  of a menu line.
	*/
	TDclear(stdmsg);
	TDrefresh(stdmsg);

	_VOID_ IIFTpupPopUpPrompt(pmsg, popinfo->begy, popinfo->begx,
		popinfo->maxy, popinfo->maxx, echo, cb, result);
}
