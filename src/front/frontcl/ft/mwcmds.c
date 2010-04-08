/*
**	MWcmds.c
**	"@(#)mwcmds.c	1.26"
**
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include <compat.h>
# include <st.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include <fe.h>
# include "mwproto.h"
# include "mwmws.h"
# include "mwintrnl.h"

/*
** Name:	MWcmds.c - Commands for MWS except Form Director.
**
** Usage:
**	INGRES FE system and MWhost on Macintosh.
**
** Description:
**
**	This file contains the functions that send commands to and
**	receive events from the MacWorkStation application running
**	on the Macintosh.  This file does not contain the functions
**	that communicate with the Form Director as those are
**	contained in mwfcmds.c.
**	
**	Routines defined here are:
**	- X010 IIMWbfsBegFormSystem	go into forms system mode.
**	- X011 IIMWefsEndFormSystem	end forms system mode.
**	- X012 IIMWsfsSuspendFormSystem	suspend forms system mode.
**	- X013 IIMWrfsResumeFormSystem	resume forms system mode.
**	- X016 IIMWdccDoCompatCheck	do compatibility check.
**
**	- X020 IIMWsimSetIngresMenu	send an INGRES form menu.
**	- X021 IIMWsrmSetRunMode	set mode of operation.
**	- X022 IIMWplkProcLastKey	process the last key.
**	- X025 IIMWfiFlushInput		flush type-ahead and mouse-ahead.
**	- X026 IIMWfkcFKClear		clear an FRS keymap table.
**	- X027 IIMWfksFKSet		set an FRS key entry.
**	- X028 IIMWfkmFKMode		set keypad to numeric mode or
**					function key mode.
**	- X030 IIMWpfmPFlashMsg		display a msg for a period of time.
**	- X031 IIMWpdmPDisplayMsg	display a msg dialog with an OK dismiss.
**	- X032 IIMWpmhPMsgHelp		display a message with OK and
**					Help for more information.
**	- X033 IIMWpuPromptUser		prompt the user for input.
**	- X034 IIMWgpGetPassword	prompt the user for a password.
**	- X035 IIMWsbSoundBell		ring the bell.
**	- X036 IIMWynqYNQues		prompt the user for yes/no response.
**
**	- X040 IIMWhvrHostVarResp	the host application variable
**					query or modify response.
**
** History:
**	05/5/89 (RK)  - Initial definition.
**	05/19/89 (RGS) - Initial implementation.
**	10/06/89 (nasser)
**		Pulled functions from other files into this one file.
**	05/22/90 (dkh) - Changed NULLPTR to NULL for VMS.
**	07/10/90 (dkh) - Integrated into r63 code line.
**	08/22/90 (nasser) - Replaced IIMWpc() with IIMWpxc().
**	08/23/90 (nasser) - Send X011 if failure during initialization.
**	08/29/90 (nasser) - Added IIMWynqYNQues().
**	09/16/90 (nasser) - Replaced IIMWscm() with IIMWsrm(). IIMWsrm()
**		is an internal function to the MW module. IIMWscm() is
**		now in mwhost.c.
**	09/16/90 (nasser) - Added IIMWdccDoCompatCheck() &
**		IIMWplkProcLastKey(). Also modified IIMWsrmSetRunMode()
**		to include flags.
**	06/08/92 (fredb) - Enclosed file in 'ifdef DATAVIEW' to protect
**		ports that are not using MacWorkStation from extraneous
**		code and data symbols.  Had to include 'fe.h' to get
**		DATAVIEW (possibly) defined.  Also 'dbms.h' to support
**		'fe.h'.
**	07-jul-97 (kitch01) 
**		Bug 79873 - Increase params length on various messages
**		to prevent crashes observed by clients using CACI-Upfront.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/


# ifdef DATAVIEW
/*{
** Name:  IIMWbfsBegFormSystem (X010) - go into forms system mode.
**
** Description:
**	IIMWbfsBeginFormSystem sends the X010 message to MWS to tell
**	it to start the forms system mode.  When MWS first starts up
**	on the Mac, it is in a tty-based shell mode.  The X010
**	command tells MWS that a forms-based INGRES application is
**	taking over, and will now be displaying forms etc.  If MWS
**	is already in the forms mode, X010 indicates that a forms-based
**	INGRES application has called another forms-based application.
**	In this case, MWS should put away the data about the current
**	application, and start a new data set.
**
** Inputs:
**	None.
**
** Sends:
**	"X010"
** Receives:
**	"X310"
**
** Outputs:
** 	Returns:
**		STAUS
** 	Exceptions:
**		None.
**
** Side Effects:
**
** History:
**	14-dec-89 (nasser) - Initial definition.
**	23-aug-90 (nasser) - Send X011 if failure during initialization.
*/
STATUS
IIMWbfsBegFormSystem()
{
	if (IIMWpxcPutXCmd("X010", "", mwMSG_FLUSH|mwMSG_SEQ_FORCE) != OK)
		return(FAIL);
	if (IIMWgseGetSpecificEvent('X',310) == (TPMsg) NULL)
	{
		_VOID_ IIMWefsEndFormSystem();
		return(FAIL);
	}
	return(OK);
}

/*{
** Name:  IIMWefsEndFormSystem (X011) - end forms system mode.
**
** Description:
**	IIMWefsEndFormSystem sends the X011 message to MWS to tell
**	it to end the forms system mode.  If X010 commands have
**	previously stacked on the MWS side, then MWS will restore
**	the previous data set.  Otherwise, MWS will go to the
**	tty-based shell mode.
**
** Inputs:
**	None.
**
** Sends:
**	"X011"
**
** Outputs:
** 	Returns:
**		STAUS
** 	Exceptions:
**		None.
**
** Side Effects:
**
** History:
**	14-dec-89 (nasser)
**		Initial definition.
*/
STATUS
IIMWefsEndFormSystem()
{
	return(IIMWpxcPutXCmd("X011", "", mwMSG_FLUSH|mwMSG_SEQ_FORCE));
}

/*{
** Name:  IIMWsfsSuspendFormSystem (X012) - suspend forms system mode.
**
** Description:
**	IIMWsfsSuspendFormSystem sends the X012 message to MWS to tell
**	it to suspend the forms system mode.  This causes MWS to go to
**	the tty-based shell mode.
**
** Inputs:
**	None.
**
** Sends:
**	"X012"
**
** Outputs:
** 	Returns:
**		STAUS
** 	Exceptions:
**		None.
**
** Side Effects:
**
** History:
**	14-dec-89 (nasser)
**		Initial definition.
*/
STATUS
IIMWsfsSuspendFormSystem()
{
	return(IIMWpxcPutXCmd("X012", "", mwMSG_FLUSH|mwMSG_SEQ_FORCE));
}

/*{
** Name:  IIMWrfsResumeFormSystem (X013) - resume forms system mode.
**
** Description:
**	IIMWrfsResumeFormSystem sends the X013 message to MWS to tell
**	it to resume the forms system mode.  MWS will resume the
**	forms session previously suspended.
**
** Inputs:
**	None.
**
** Sends:
**	"X013"
**
** Outputs:
** 	Returns:
**		STAUS
** 	Exceptions:
**		None.
**
** Side Effects:
**
** History:
**	14-dec-89 (nasser)
**		Initial definition.
*/
STATUS
IIMWrfsResumeFormSystem()
{
	return(IIMWpxcPutXCmd("X013", "", mwMSG_FLUSH|mwMSG_SEQ_FORCE));
}

/*{
** Name:  IIMWdccDoCompatCheck (X016) - do compatibility check.
**
** Description:
**	This function sends compatibility info and host version
**	info to MWS. If MWS and host application are not compatible,
**	the user will be informed and the session terminated. If
**	there is doubt about compatiblity, the user will be asked
**	if he wants to continue. MWS repsonds with X316 event
**	which indicates whether to proceed or abort.
**
** Inputs:
**	compat_info	host's view of compatibility.
**	ingres_ver	host version info.
**
** Sends:
**	"X016	compatibility_info; host_ver"
** Receives:
**	"X316	proceed"
**
** Outputs:
**	proceed		whether to proceed or abort.
** 	Returns:
**		STATUS
** 	Exceptions:
**
** Side Effects:
**
** History:
**	16-sep-90 (nasser) - Initial definition.
**	11-aug-97 (kitch01)
**		Bug 84391. ingres_ver is now a nat.
*/
STATUS
IIMWdccDoCompatCheck(compat_info, ingres_ver, proceed)
i4	 compat_info;
i4	 ingres_ver;
bool	*proceed;
{
    char	params[64];
    TPMsg	theMsg;
    i4		answer;

    _VOID_ STprintf(params, "%d;%d", compat_info, ingres_ver);
    if((IIMWpxcPutXCmd("X016", params, mwMSG_FLUSH) == OK) &&
	((theMsg = IIMWgseGetSpecificEvent('X', 316)) != (TPMsg) NULL) &&
	(IIMWmgdMsgGetDec(theMsg, &answer, NULLCHAR) == OK))
    {
	*proceed = (answer == 1);
	return(OK);
    }
    else
    {
	return(FAIL);
    }
}

/*{
** Name: IIMWsimSetIngresMenu (X020) - send INGRES menu.
**
** Description:
**	This routine sets up the INGRES menu.
**	
** Inputs:
**	count		 Integer (menu item count)
**	theMenu		 String  (text of menu items and labels)
**
** Sends:
**	"X020	count; menuItem [;menuItem...]"
**
** Outputs:
**	Returns:
**		OK/FAIL
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	05/5/89 (RK)  - Initial definition.
**	05/19/89 (RGS) - Initial implementation.
**	07-jul-97 (kitch01) 
**				Bug 79873 - Increase params length.
*/
STATUS
IIMWsimSetIngresMenu(count,theMenu)
i4  	 count;
u_char	*theMenu;
{
	char params[2048];

	_VOID_ STprintf(params,"%d;%s",count,theMenu);
	return(IIMWpxcPutXCmd("X020", params, 0));
}

/*{
** Name: IIMWsrmSetRunMode (X021) - set mode of operation.
**
** Description:
**	IIMWscmSetCurMode allows the host application to define the
**	mode, given by curMode, of operation. The mode defines
**	whether the user can modify the textual value of the
**	current item on the form, or if the user can only make a
**	menu selection. 
**	
** Inputs:
**	runMode		run mode
**	itemMode	current item mode
**	flags		key intercept and key find info
**
** Sends:
**	"X021	runMode, curMode, flags"
**
** Outputs:
**	Returns:
**		OK/FAIL.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	05/5/89 (RK)  - Initial definition.
**	05/19/89 (RGS) - Initial implementation.
**	16-sep-90 (nasser) - Added flags to the message.
*/
STATUS
IIMWsrmSetRunMode(runMode, curMode, flags)
i4	runMode;
i4	curMode;
i4	flags;
{
    char params[32];

    _VOID_ STprintf(params, "%d,%d;%x", runMode, curMode, flags);
    return(IIMWpxcPutXCmd("X021", params, mwMSG_FLUSH));
}

/*{
** Name:  IIMWplkProcLastKey (X022) - process the last key.
**
** Description:
**	In key intercept or key find modes, MWS passes the key
**	back to the host. This message tells the MWS that MWS
**	should process the key. However if the ignore parameter
**	is set, then MWS ignores the key.
**
** Inputs:
**	ignore		ignore the last key.
**
** Sends:
**	"X022	ignore"
**
** Outputs:
** 	Returns:
**		STATUS
** 	Exceptions:
**
** Side Effects:
**
** History:
**	16-sep-90 (nasser) - Initial definition.
*/
STATUS
IIMWplkProcLastKey(ignore)
bool	ignore;
{
    char	params[16];

    _VOID_ STprintf(params, "%d", ignore);
    return(IIMWpxcPutXCmd("X022", params, mwMSG_FLUSH));
}

/*{
** Name: IIMWfiFlushInput (X025) - flush type-ahead and mouse-ahead.
**
** Description:
**	If INGRES encounters an unexpected situation, it may
**	want to discard all act-ahead from the user.  If so,
**	it can send the X025 command to tell MWS to discard
**	all type-ahead and mouse-ahead.
**	
** Inputs:
**	None.
**
** Sends:
**	"X025"
**
** Outputs:
**	Returns:
**		OK/FAIL.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	14-dec-89 (nasser)
**		Initial definition.
*/
STATUS
IIMWfiFlushInput()
{
	if ( ! IIMWmws)
		return(OK);

	return(IIMWpxcPutXCmd("X025", "", mwMSG_FLUSH));
}

/*{
** Name: IIMWfkcFKClear (X026) - clear an FRS keymap table.
**
** Description:
**	This routine clears an FRS keymap table.
**	
** Inputs:
**	map_type	char (which table)
**
** Sends:
**	"X026	map_type"
**
** Outputs:
**	Returns:
**		OK/FAIL.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	05/5/89 (RK)  - Initial definition.
**	05/19/89 (RGS) - Initial implementation.
*/
STATUS
IIMWfkcFKClear(map_type)
char	map_type;
{
	char params[64];

	_VOID_ STprintf(params,"%c",map_type);
	return(IIMWpxcPutXCmd("X026", params, 0));
}

/*{
** Name: IIMWfksFKSet (X027) - set an FKS key entry.
**
** Description:
**	This routine sets an FKS key entry to a new value.
**	
** Inputs:
**	map_type	Char (which table)
**	item		Integer (which table item)
**	value		Integer (the FRS value)
**	flag		Integer (the FRS flags)
**		
** Sends:
**	"X027	map_type, item; value, flag"
**
** Outputs:
**	Returns:
**		OK/FAIL.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	05/5/89 (RK)  - Initial definition.
**	05/19/89 (RGS) - Initial implementation.
*/
STATUS
IIMWfksFKSet(map_type,item,val,flag)
char	map_type;
i4	item;
i2	val;
i1	flag;
{
	char params[256];

	_VOID_ STprintf(params,"%c,%d;%d,%d",map_type,item,val,flag);
	return(IIMWpxcPutXCmd("X027", params, 0));
}

/*{
** Name: IIMWfkmFKMode (X028) - set keypad mode.
**
** Description:
**	This routine sets the keypad to numeric mode or function key mode.
**	
** Inputs:
**	formAlias	Integer (alias of form)
**	mode		Integer (function key mode)
**
** Sends:
**	"X028	formAlias; mode"
**
** Outputs:
**	Returns:
**		OK/FAIL.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	05/5/89 (RK)  - Initial definition.
**	05/19/89 (RGS) - Initial implementation.
*/
STATUS
IIMWfkmFKMode(mode)
i4  mode;
{
	char params[256];

	if ( ! IIMWmws)
		return(OK);

	_VOID_ STprintf(params,"%d", mode);
	return(IIMWpxcPutXCmd("X028", params, 0));
}

/*{
** Name: IIMWpfmPFlashMsg (X030) - display a message for a specified time.
**
** Description:
**	This routine creates a modal text window (shape = 1) in
**	which to display a text message. The time parameter signifies
**	the time (in 1/10 seconds) that the window should remain on
**	the screen before it is tossed.  The dialog box is dismissed 
**	without user interaction when the time expires. bell
**	specifies whether the bell should be rung as the window is drawn.
**	
** Inputs:
**	msg	StringPtr (text of message)
**	bell	Integer (1 = bell before displaying the dialog)
**	time	Integer (alias for form)
**
** Sends:
**	"X030	time, bell; message"
**
** Outputs:
**	Returns:
**		OK/FAIL.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	05/5/89 (RK)  - Initial definition.
**	05/19/89 (RGS) - Initial implementation.
*/
STATUS
IIMWpfmPFlashMsg(msg,bell,time)
PTR	msg;
bool	bell;
i4	time;
{
	char params[1024];

	_VOID_ STprintf(params,"%d,%d;%s",time,bell,msg);
	return(IIMWpxcPutXCmd("X030", params, mwMSG_FLUSH));
}

/*{
** Name: IIMWpdmPDisplayMsg (X031) - display a message with user dismiss.
**
** Description:
**	Message X031 (Display Message) displays a message in a dialog
**	box with a single button, labled OK, which the user can click
**	to dismiss the dialog.
**	
** Inputs:
**	msg	StringPtr (text of message)
**	bell	Integer (1 = bell before displaying the dialog)
**
** Sends:
**	"X031	bell; message"
**
** Outputs:
**	Returns:
**		OK/FAIL.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	05/5/89 (RK)  - Initial definition.
**	05/19/89 (RGS) - Initial implementation.
**	07-jul-97 (kitch01) 
**				Bug 79873 - Increase params length.
*/
STATUS
IIMWpdmPDisplayMsg(msg,bell)
PTR 	msg;
bool	bell;
{
	char params[2048];

	_VOID_ STprintf(params,"%d;%s",bell,msg);
	return(IIMWpxcPutXCmd("X031", params, mwMSG_FLUSH));
}

/*{
** Name: IIMWpmhPMsgHelp (X032) - display a message with Help.
**
** Description:
**	Message X032 (Display Message with Help) Displays a
**	brief message along with two buttons labeled OK and Help.
**	The user can dismiss the dialog by pressing the OK button.
**	However, if users would like further information, they can
**	press the help button to display a window containing
**	additional information.
**	
** Inputs:
**	msg	StringPtr (text of message)
**	bell	Integer (1 = bell before displaying the dialog)
**
** Sends:
**	"X032	bell; short_message, long_msg"
**
** Outputs:
**	Returns:
**		OK/FAIL.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	05/5/89 (RK)  - Initial definition.
**	05/19/89 (RGS) - Initial implementation.
**	07-jul-97 (kitch01) 
**				Bug 79873 - Increase params length.
*/
STATUS
IIMWpmhPMsgHelp(short_msg,long_msg,bell)
PTR 	short_msg,long_msg;
bool	bell;
{
	char params[2048];

	_VOID_ STprintf(params,"%d;%s;%s",bell,short_msg,long_msg);
	return(IIMWpxcPutXCmd("X032", params, mwMSG_FLUSH));
}

/*{
** Name: IIMWpuPromptUser (X033) - prompt the user for input.
**
** Description:
**	This routine creates a dialog box with a text edit item,
**	a static text item and an OK button. The text edit field
**	is set to the value of the textual parameter that was
**	passed in. The user responds to the textual prompt by
**	entering a response in a text edit item. When the user
**	hits the OK button or presses the return, the characters
**	the user actually typed will be sent back to the host
**	through an X333 message.  This routine returns to the
**	host the textual response typed by the user.
**	
** Inputs:
**	prompt	StringPtr (text of prompt)
**	bell	Integer (1 = bell before displaying the dialog)
**
** Sends:
**	"X033	bell; prompt"
** Receives:
**	"X333	input"
**
** Outputs:
**	Returns:
**		StringPtr	text of response
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	05/5/89 (RK)  - Initial definition.
**	05/19/89 (RGS) - Initial implementation.
**	07-jul-97 (kitch01) 
**				Bug 79873 - Increase params length.
*/
PTR
IIMWpuPromptUser(prompt, bell)
PTR 	prompt;
bool	bell;
{
	char params[1024];
	TPMsg theMsg;

	_VOID_ STprintf(params, "%d;%s", bell, prompt);
	if ((IIMWpxcPutXCmd("X033", params, mwMSG_FLUSH) == OK) &&
		((theMsg = IIMWgseGetSpecificEvent('X',333)) != (TPMsg) NULL))
	{
		return((PTR) &theMsg->msgData[theMsg->msgIndex]);
	}
	else
	{
		return(NULL);
	}
}

/*{
** Name: IIMWgpGetPassword (X034) - prompt the user for a password.
**
** Description:
**	This routine creates a dialog box with a text edit item, a
**	static text item and an OK button. The text edit field is
**	set to the value of the textual parameter that was passed in.
**	As the user types into the text edit item each character is
**	displayed as an asterisk. When the user hits the OK button or
**	presses the return, the characters the user actually typed
**	will be sent back to the host through an X334 message.  This
**	routine returns to the host the textual password typed by
**	the user. 
**	
** Inputs:
**	prompt	StringPtr (text of password prompt)
**	bell	Integer (1 = bell before displaying the dialog)
**
** Sends:
**	"X034	bell; prompt"
** Receives:
**	"X334	input"
**
** Outputs:
**	Returns:
**		StringPtr	text of password
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	05/5/89 (RK)  - Initial definition.
**	05/19/89 (RGS) - Initial implementation.
*/
PTR
IIMWgpGetPassword(prompt, bell)
PTR 	prompt;
bool	bell;
{
	char params[512];
	TPMsg theMsg;

	_VOID_ STprintf(params, "%d;%s", bell, prompt);
	if ((IIMWpxcPutXCmd("X034", params, mwMSG_FLUSH) == OK) &&
		((theMsg = IIMWgseGetSpecificEvent('X',334)) != (TPMsg) NULL))
	{
		return((PTR) &theMsg->msgData[theMsg->msgIndex]);
	}
	else
	{
		return(NULL);
	}
}

/*{
** Name: IIMWsbSoundBell (X035) - ring the bell.
**
** Description:
**	Message X035 (Bell Message) rings the bell.
**	
** Inputs:
**	None.
**
** Sends:
**	"X035"
**
** Outputs:
**	Returns:
**		STATUS.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	05/5/89 (RK)  - Initial definition.
**	05/19/89 (RGS) - Initial implementation.
*/
STATUS
IIMWsbSoundBell()
{
	if ( ! IIMWmws)
		return(OK);
	return(IIMWpxcPutXCmd("X035", "", mwMSG_FLUSH));
}

/*{
** Name: IIMWynqYNQues (X036) - prompt the user for y/n response.
**
** Description:
**	This routine creates a dialog box with a static text item
**	and YES and NO buttons. If the user presses the YES
**	button, a value of 1 is sent back via the 336 event; if 
**	NO is pressed, a value of 0 is sent back.
**	
** Inputs:
**	prompt	StringPtr (text of prompt)
**	bell	Integer (1 = bell before displaying the dialog)
**
** Sends:
**	"X036	bell; prompt"
** Receives:
**	"X336	input"
**
** Outputs:
**	response	TRUE if user responded YES
**	Returns:
**		STATUS
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	29-aug-90 (nasser)  - Initial definition.
**	07-jul-97 (kitch01) 
**				Bug 79873 - Increase params length.
*/
STATUS
IIMWynqYNQues(prompt, bell, response)
PTR 	 prompt;
bool	 bell;
bool	*response;
{
	char	params[1024];
	TPMsg	theMsg;
	i4	answer;

	_VOID_ STprintf(params, "%d;%s", bell, prompt);
	if ((IIMWpxcPutXCmd("X036", params, mwMSG_FLUSH) == OK) &&
		((theMsg = IIMWgseGetSpecificEvent('X',336)) != (TPMsg) NULL) &&
		(IIMWmgdMsgGetDec(theMsg, &answer, NULLCHAR) == OK))
	{
		*response = (answer == 1);
		return(OK);
	}
	else
	{
		return(FAIL);
	}
}

/*{
** Name: IIMWhvrHostVarResp (X040) - the host application variable
**					query or modify response.
**
** Description:
**	This routine sends the response from the host to the
**	QueryHostVariable and SetHostVariable events initiated
**	from MWS. This protocol allows an engineer testing MWS
**	exec module code to query or modify the state of the
**	application running on the remote host.
**	Variable is an integer that defines the variable the
**	message refers to. The second parameter is a
**	string. The value of both of these parameters is understood
**	by the debugging code on both MWS and the remote host, but
**	it is outside the scope of this message protocol. This
**	allows extensibility so that an engineer can use this low
**	level facility to access any remote variables without
**	modifying the event/message protocol.
**	
** Inputs:
**	variable	Integer (id of variable)
**	text		String  (text of message)
**
** Sends:
**	"X040	variable; text"
**
** Outputs:
**	Returns:
**		STATUS
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	05/5/89 (RK)  - Initial definition.
**	05/19/89 (RGS) - Initial implementation.
*/
STATUS
IIMWhvrHostVarResp(variable, text)
i4	variable;
PTR	text;
{
	char	params[256];

	_VOID_ STprintf(params, "%d;%s", variable, text);
	return(IIMWpxcPutXCmd("X040", params, mwMSG_FLUSH));
}

# endif /* DATAVIEW */
