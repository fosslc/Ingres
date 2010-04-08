/*
**	MWmsg.c
**	"@(#)mwmsg.c	1.11"
**
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include <compat.h>
# include <st.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include <adf.h>
# include <fmt.h>
# include <ft.h>
# include <frame.h>
# include <frsctblk.h>
# include <fe.h>
# include "mwproto.h"
# include "mwmws.h"
# include "mwintrnl.h"

/**
** Name: MWmsg.c - MacWorkStation Host Routines.
**
** Usage:
**	INGRES FE system and MWhost on Macintosh.
**
** Description:
**	Supporting the INGRES frontends involves several situations
**	where a message or prompt needs to be displayed before the
**	user. Some of these situations may  require that the user
**	respond to this message in some manner. These situations 
**	require special functions that are not provided as a standard
**	part of the MWS functional set. The message management
**	portion of the INGRES MWSX exec module compartmentalizes
**	these special functions to provide easy access for the 
**	frontend utilities. This INGRES module implements a
**	message display and  user prompt mechanism that is not
**	provided by the Apple-supplied version of MacWorkStation.
**	
**	Routines defined here are:
**		IIMWdmDisplayMsg	Display a message with OK button
**		IIMWfmFlashMsg		Display a message for a while
**		IIMWguiGetUsrInp	Get input from user
**		IIMWguvGetUsrVer	Get yes/no response from user
**		IIMWmwhMsgWithHelp	Display msg with help option
**
** History:
**	05/5/89 (RK)  - Initial definition.
**	05/19/89 (RGS) - Initial implementation.
**	05/22/90 (dkh) - Changed NULLPTR to NULL for VMS.
**	08/29/90 (nasser) - Added IIMWguvGetUsrVer().
**	06/08/92 (fredb) - Enclosed file in 'ifdef DATAVIEW' to protect
**		ports that are not using MacWorkStation from extraneous
**		code and data symbols.  Had to include 'fe.h' to get
**		DATAVIEW (possibly) defined.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

# ifdef DATAVIEW

/*{
** Name:  IIMWdmDisplayMsg -- Display a message.
**
** Description:
** 	Send command to display message with an OK button.
**
** Inputs:
**	msg	Message to be displayed.
**	bell	Boolean indicating whether bell should be rung.
**	evcb	Event control block.
**
** Outputs:
** 	Returns:
**		STATUS
** 	Exceptions:
**		None.
**
** Side Effects:
**
** History:
**	20-sep-89 (nasser)
**		Initial definition.
*/
STATUS
IIMWdmDisplayMsg(msg, bell, evcb)
char		*msg;
i4		 bell;
FRS_EVCB	*evcb;
{
	if ( ! IIMWmws)
		return(OK);

	if ((msg == NULL) || (STskipblank(msg, (i4) STlength(msg)) == NULL))
		return(OK);
	return(IIMWpdmPDisplayMsg(msg, bell));
}

/*{
** Name:  IIMWfmFlashMsg -- Display the message for some time.
**
** Description:
**	Send command to flash message.  The message is displayed
**	for a while and then removed without any user intervention.
**
** Inputs:
**	msg	Message to be displayed.
**	bell	Boolean value indiacating whether bell should be rung.
**
** Outputs:
** 	Returns:
**		STATUS.
** 	Exceptions:
**		None.
**
** Side Effects:
**
** History:
**	20-sep-89 (nasser)
**		Initial definition.
*/
STATUS
IIMWfmFlashMsg(msg, bell)
char	*msg;
i4	 bell;
{
	if ( ! IIMWmws)
		return(OK);

	if ((msg == NULL) || (STskipblank(msg, (i4) STlength(msg)) == NULL))
		return(OK);
	return(IIMWpfmPFlashMsg(msg, bell, 0));
}

/*{
** Name:  IIMWguiGetUsrInp -- Prompt the user.
**
** Description:
**	Prompt the user.  Whether the user response is displayed
**	on the screen is controlled by the parameter 'echo.'
**
** Inputs:
**	prompt	Text of the prompt
**	echo	Boolean value indicating whether to echo user response
**	evcb	Event control block
**
** Outputs:
**	result	Buffer with the response
** 	Returns:
**		STATUS
** 	Exceptions:
**		None.
**
** Side Effects:
**
** History:
**	20-sep-89 (nasser)
**		Initial definition.
*/
STATUS
IIMWguiGetUsrInp(prompt, echo, evcb, result)
char		*prompt;
i4		 echo;
FRS_EVCB	*evcb;
char		*result;
{
	char	*buf;

	if ( ! IIMWmws)
		return(OK);

	if (echo)
		buf = IIMWpuPromptUser(prompt, FALSE);
	else
		buf = IIMWgpGetPassword(prompt, FALSE);
	if (buf == NULL)
	{	
		return(FAIL);
	}
	else
	{
		STcopy(buf, result);
		return(OK);
	}
}

/*{
** Name:  IIMWguvGetUsrVer -- Prompt the user for yes/no response.
**
** Description:
**	Prompt the user for a yes/no response.
**
** Inputs:
**	prompt	Text of the prompt
**
** Outputs:
** 	Returns:
**		TRUE if user said yes. FALSE otherwise.
** 	Exceptions:
**		None.
**
** Side Effects:
**
** History:
**	29-aug-90 (nasser) -- Initial definition.
*/
bool
IIMWguvGetUsrVer(prompt)
char	*prompt;
{
	bool	response;

	if ( ! IIMWmws)
		return(OK);

	if (IIMWynqYNQues(prompt, FALSE, &response) != OK)
		return(FALSE);
	return(response);
}

/*{
** Name:  IIMWmwhMsgWithHelp -- Display a message with help.
**
** Description:
**	Display the short message; the user can then decide
**	whether to look at the the long message.
**
** Inputs:
**	short_msg	text of the short message
**	long_msg	text of the long message
**	evcb		Event control block
**
** Outputs:
** 	Returns:
**		STATUS
** 	Exceptions:
**		None.
**
** Side Effects:
**
** History:
**	20-sep-89 (nasser)
**		Initial definition.
*/
STATUS
IIMWmwhMsgWithHelp(short_msg, long_msg, evcb)
char		*short_msg;
char		*long_msg;
FRS_EVCB	*evcb;
{
	i4	 cmplen;
	STATUS	 ret_val;

	if ( ! IIMWmws)
		return(OK);

	if (short_msg == NULL)
	{
		/* FT rings bell only if short_msg != NULL */
		ret_val = IIMWdmDisplayMsg(long_msg, FALSE, evcb);
	}
	else if (long_msg == NULL)
	{
		ret_val = IIMWdmDisplayMsg(short_msg, TRUE, evcb);
	}
	else
	{
		/* Display the long message only if the two are similar */
		cmplen = (i4) STlength(short_msg) * 3 / 4;
		if (STbcompare(short_msg, cmplen, long_msg, cmplen, FALSE) == 0)
			ret_val = IIMWdmDisplayMsg(long_msg, TRUE, evcb);
		else
			ret_val = IIMWpmhPMsgHelp(short_msg, long_msg, TRUE);
	}
	return(ret_val);
}

# endif /* DATAVIEW */
