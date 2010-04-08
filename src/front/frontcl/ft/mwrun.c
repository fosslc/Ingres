/*
** 20-jul-90 (sandyd)
**	Work around VAXC optimizer bug, encountered in VAXC V3.0-031.
NO_OPTIM = vax_vms
**
**	mwrun.c
**	"@(#)mwrun.c	1.25"
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
# include <kst.h>
# include <mapping.h>
# include <fe.h>
# include "mwmws.h"
# include "mwhost.h"
# include "mwform.h"
# include "mwintrnl.h"

/**
** Name:	mwrun.c - User Routines to Support MacWorkStation.
**
** Description:
**	File contains routines to user specific MacWorkStation commands.
**	This will allow standard MacWorkStation responses to be handled
**	by user implemented frontends.
**	The 'MWS_'...  routines exist for the MWhost and can be stubbed out
**	by the INGRES frontends.
**
** Usage:
**	INGRES FE system and MWhost on Macintosh.
**
**	Routines defined here are:
**
**	  - IIMWgsiGetSysInfo	Get information about the system on the Mac.
**	  - IIMWdrDoRun		The main event loop for interacting with MWS.
**
** History:
**	05/5/89 (RK)  - Initial definition.
**	05/19/89 (RGS) - Initial implementation.
**	04/14/90 (dkh) - Eliminated use of "goto"'s where safe to do so.
**	05/15/90 (dkh) - Put ifdef UNIX around entire file to sidestep
**			 compilation problems on VMS.
**	05/22/90 (dkh) - Changed NULLPTR to NULL for VMS.
**	07/10/90 (dkh) - Integrated changes into r63 code line.
**	10/10/90 (nasser) - Changed IIMWdrDoRun() to return status and
**		return the key code as a parameter. Changed return
**		codes from fdopERR and fdNOCODE to ERROR, IGNORE, and
**		RETURN.
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

/* Return values used by functions called by DoRun() */
# define ERROR	-1
# define IGNORE	0
# define RETURN	1


static i4		*result;
static i4		*fld_flags;
static char		*t_buf;
static KEYSTRUCT	*ks;

static i4	doAlert();
static i4	doCursor();
static i4	doDialog();
static i4	doExec();
static i4	doFile();
static i4	doGraphic();
static i4	doList();
static i4	doMenu();
static i4	doProcess();
static i4	doText();
static i4	doWindow();


/*{
** Name:	IIMWgsiGetSysInfo - Get information about the system on the Mac.
**
** Description:
**	Get system and version info, etc from the Mac.
**
** Inputs:
**	None.
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
IIMWgsiGetSysInfo()
{
	TPMsg	theMsg;

	/* get Macintosh host info */
	if ((IIMWpcPutCmd("P005", "1") == FAIL) ||
		((theMsg = IIMWgseGetSpecificEvent('P', 260)) == (TPMsg) NULL) ||
		(doProcess(theMsg, 260) == ERROR))
	{
		return(FAIL);
	}

	/* get the MWS version number */
	if ((IIMWpcPutCmd("P006", "") == FAIL) ||
		((theMsg = IIMWgseGetSpecificEvent('P', 261)) == (TPMsg) NULL) ||
		(doProcess(theMsg, 261) == ERROR))
	{
		return(FAIL);
	}

	/* get Macintosh environment info */
	if ((IIMWpcPutCmd("P007", "") == FAIL) ||
		((theMsg = IIMWgseGetSpecificEvent('P', 262)) == (TPMsg) NULL) ||
		(doProcess(theMsg, 262) == ERROR))
	{
		return(FAIL);
	}

	/* create the menu bar */
	if (IIMWcmmCreateMacMenus() == FAIL)
		return(FAIL);

	return(OK);

}

/*{
** Name: IIMWdrDoRun - The main event loop for interacting with MWS.
**
** Description:
**	This routine is the main event loop for interacting with MWS.
**	It, and the event handling routines that it calls, replaces
**	much of the functionality currently provided by FTrun() and
**	the numerous support routines it calls to support Browse
**	and Insert modes
**	
**	MWS operates autonomously until an event occurs that requires
**	the host's attention. Further user input should be inhibited
**	while the host handles an event.
**
** Inputs:
**	None.
**
** Outputs:
**	res		result of the operation
**	flags		flags
**	buf		new value of the field
**	ks		key structure containing some info
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
**	16-sep-90 (nasser) - Added kstruct parameter.
**	10-oct-90 (nasser) - Added res parameter and changed return
**		to status.
*/
STATUS
IIMWdrDoRun(res, flags, buf, kstruct)
i4		*res;
i4		*flags;
char		*buf;
KEYSTRUCT	*kstruct;
{
	i4	class;
	i4	id;
	i4	status = IGNORE;
	TPMsg	theMsg;

	/*
	**  Copy the pointers passed to us, so that the
	**  functions we call can set values.
	*/
	result = res;
	t_buf = buf;
	fld_flags = flags;
	ks = kstruct;

	/* wait for a significant event */
	while (status == IGNORE)
	{
		theMsg = IIMWgeGetEvent(&class,&id);
		if (theMsg == (TPMsg) NULL)
		{
			status = ERROR;
			break;
		}

		switch (class)
		{
		case kCmdAlert :
			status = doAlert(theMsg,id);
			break;

		case kCmdCursor :
			status = doCursor(theMsg,id);
			break;

		case kCmdDialog :
			status = doDialog(theMsg,id);
			break;

		case kCmdFile :
			status = doFile(theMsg,id);
			break;

		case kCmdGraphic :
			status = doGraphic(theMsg,id);
			break;

		case kCmdList :
			status = doList(theMsg,id);
			break;

		case kCmdMenu :
			status = doMenu(theMsg,id);
			break;

		case kCmdProcess :
			status = doProcess(theMsg,id);
			break;

		case kCmdText :
			status = doText(theMsg,id);
			break;

		case kCmdWindow :
			status = doWindow(theMsg,id);
			break;

		case kCmdExec :
			status = doExec(theMsg,id);
			break;

		default :
			/* complain about bad message class */
			break;

		}	/* switch */
	}	/* while */

	if (status == ERROR)
		return(FAIL);
	else
		return(OK);
}

/*{
** Name:  doAlert -- Act on event returned by the Alert Director.
**
** Description:
**	The Alert Director returned an event.  Take appropriate
**	action.
**
** Inputs:
**	theMsg	Pointer to message received from MWS
**	id	Id code of message received
**
** Outputs:
** 	Returns:
**		status info; ERROR, IGNORE or RETURN.
** 	Exceptions:
**		None.
**
** Side Effects:
**
** History:
**	25-sep-89 (nasser)
**		Initial definition.
*/
static i4
doAlert(theMsg,id)
TPMsg	theMsg;
i4	id;
{
	switch(id) 
	{

	default:
		IIMWplPrintLog(mwLOG_DIR_MSG, theMsg->msgData);
		break;

	}
	return(IGNORE);
}

/*{
** Name:  doCursor -- Act on event returned by the Cursor Director.
**
** Description:
**	The Cursor Director returned an event.  Take appropriate
**	action.
**
** Inputs:
**	theMsg	Pointer to message received from MWS
**	id	Id code of message received
**
** Outputs:
** 	Returns:
**		status info; ERROR, IGNORE or RETURN.
** 	Exceptions:
**		None.
**
** Side Effects:
**
** History:
**	25-sep-89 (nasser)
**		Initial definition.
*/
static i4
doCursor(theMsg,id)
TPMsg	theMsg;
i4	id;
{
	switch(id) 
	{

	default:
		IIMWplPrintLog(mwLOG_DIR_MSG, theMsg->msgData);
		break;

	}
	return(IGNORE);
}

/*{
** Name:  doDialog -- Act on event returned by the Dialog Director.
**
** Description:
**	The Dialog Director returned an event.  Take appropriate
**	action.
**
** Inputs:
**	theMsg	Pointer to message received from MWS
**	id	Id code of message received
**
** Outputs:
** 	Returns:
**		status info; ERROR, IGNORE or RETURN.
** 	Exceptions:
**		None.
**
** Side Effects:
**
** History:
**	25-sep-89 (nasser)
**		Initial definition.
*/
static i4
doDialog(theMsg,id)
TPMsg	theMsg;
i4	id;
{
	switch(id) 
	{

	case 257:		/* user pushed a dialog control*/
		MWS_D_Select(theMsg);
		break;

	default:
		IIMWplPrintLog(mwLOG_DIR_MSG, theMsg->msgData);
		break;

	}
	return(IGNORE);
}

/*{
** Name: doExec - process an MWSX event.
**
** Description:
**	The Exec Director returned an event.  Take appropriate
**	action.
**	Many of the events returned by the Exec Director are
**	handled in mwcmds.c and mwfcmds.c.
**
** Inputs:
**	theMsg	Pointer to message received from MWS
**	id	Id code of message received
**
** Outputs:
**	Returns:
**		Key		key pressed if id == 450 or id == 451
**		MenuItem	if id == 320
**		ERROR		If error occurred
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
static i4
doExec(theMsg,id)
TPMsg	theMsg;
i4	id;
{
	i4	status = IGNORE;
	i4	dummy;

	switch (id)
	{

		/*	MWEvent, cases 310, 333, 334 handled in MWcmds.c */
	case 340:	/* query host variable */
		if (IIMWqhvQueryHostVar(theMsg) == FAIL)
			status = ERROR;
		break;

	case 341:	/* set host variable */
		if (IIMWshvSetHostVar(theMsg) == FAIL)
			status = ERROR;
		break;

		/*	MWformEvent, cases 420,422,424&426 handled in MWfcmds.c	*/
	case 451: 	/* returns a key event */
		if (ks != (KEYSTRUCT *) NULL)
		{
			if ((IIMWmgdMsgGetDec(theMsg, result, ';') == OK) &&
			    (IIMWmgdMsgGetDec(theMsg, &ks->ks_p1, ',') == OK) &&
			    (IIMWmgdMsgGetDec(theMsg, &ks->ks_p2, ',') == OK) &&
			    (IIMWmgdMsgGetDec(theMsg, &ks->ks_p3, ';') == OK) &&
			    (IIMWmgdMsgGetDec(theMsg, &dummy, ';') == OK))
			{
				status = RETURN;
				if (fld_flags != NULL)
					*fld_flags = dummy;
				if ((t_buf != NULL) && 
				    (IIMWmgsMsgGetStr(theMsg, t_buf, NULLCHAR)
					!= OK))
				{
					status = ERROR;
				}
			}
			else
			{
				status = ERROR;
			}
		}
		else
		{
			/* This case should never happen */
			status = ERROR;
		}
		break;

	default:
		/*	Load Exec Module Result, case 256 handled above	*/
		IIMWplPrintLog(mwLOG_EXTRA_MSG, "DoExec");
		break;

	}
	return(status);
}

/*{
** Name:  doFile -- Act on event returned by the File Director.
**
** Description:
**	The File Director returned an event.  Take appropriate
**	action.
**
** Inputs:
**	theMsg	Pointer to message received from MWS
**	id	Id code of message received
**
** Outputs:
** 	Returns:
**		status info; ERROR, IGNORE or RETURN.
** 	Exceptions:
**		None.
**
** Side Effects:
**
** History:
**	25-sep-89 (nasser)
**		Initial definition.
*/
static i4
doFile(theMsg,id)
TPMsg	theMsg;
i4		id;
{
	switch(id) 
	{

	default:
		IIMWplPrintLog(mwLOG_DIR_MSG, theMsg->msgData);
		break;

	}
	return(IGNORE);
}

/*{
** Name:  doGraphic -- Act on event returned by the Graphic Director.
**
** Description:
**	The Graphic Director returned an event.  Take appropriate
**	action.
**
** Inputs:
**	theMsg	Pointer to message received from MWS
**	id	Id code of message received
**
** Outputs:
** 	Returns:
**		status info; ERROR, IGNORE or RETURN.
** 	Exceptions:
**		None.
**
** Side Effects:
**
** History:
**	25-sep-89 (nasser)
**		Initial definition.
*/
static i4
doGraphic(theMsg,id)
TPMsg	theMsg;
i4		id;
{
	switch(id)
	{

	case 256:		/* user double clicked an icon */
		MWS_I_Select(theMsg);
		break;

	default:
		IIMWplPrintLog(mwLOG_DIR_MSG, theMsg->msgData);
		break;

	}
	return(IGNORE);
}

/*{
** Name:  doList -- Act on event returned by the List Director.
**
** Description:
**	The List Director returned an event.  Take appropriate
**	action.
**
** Inputs:
**	theMsg	Pointer to message received from MWS
**	id	Id code of message received
**
** Outputs:
** 	Returns:
**		status info; ERROR, IGNORE or RETURN.
** 	Exceptions:
**		None.
**
** Side Effects:
**
** History:
**	25-sep-89 (nasser)
**		Initial definition.
*/
static i4
doList(theMsg,id)
TPMsg	theMsg;
i4		id;
{
	switch(id)
	{

	case 256:		/* user doubleclicked a list item */
		MWS_L_Select(theMsg);
		break;

	default:
		IIMWplPrintLog(mwLOG_DIR_MSG, theMsg->msgData);
		break;

	}
	return(IGNORE);
}

/*{
** Name:  doMenu -- Act on event returned by the Menu Director.
**
** Description:
**	The Menu Director returned an event.  Take appropriate
**	action.
**
** Inputs:
**	theMsg	Pointer to message received from MWS
**	id	Id code of message received
**
** Outputs:
** 	Returns:
**		status info; ERROR, IGNORE or RETURN.
** 	Exceptions:
**		None.
**
** Side Effects:
**
** History:
**	25-sep-89 (nasser)
**		Initial definition.
*/
static i4
doMenu(theMsg,id)
TPMsg	theMsg;
i4		id;
{
	switch(id)
	{

	case 256:		/* user selected one of my menus */
		MWS_M_Select(theMsg);
		break;

	default:
		IIMWplPrintLog(mwLOG_DIR_MSG, theMsg->msgData);
		break;

	}
	return(IGNORE);
}

/*{
** Name:  doProcess -- Act on events returned by the Process Director.
**
** Description:
**	The Process Director returned an event.  Take appropriate
**	action.
**
** Inputs:
**	theMsg	Pointer to message received from MWS
**	id	Id code of message received
**
** Outputs:
** 	Returns:
**		status info; ERROR, IGNORE or RETURN.
** 	Exceptions:
**		None.
**
** Side Effects:
**
** History:
**	25-sep-89 (nasser)
**		Initial definition.
*/
static i4
doProcess(theMsg,id)
TPMsg	theMsg;
i4		id;
{
	i4	status = IGNORE;
	switch(id)
	{

	case 256:	/* MacWorkStation just logged in */
		MWS_P_Init(theMsg);
		break;

	case 257:	/* MacWorkStation just logged out */
		MWS_P_Disconnect();
		break;

	case 258:	/* user selected "Quit" from "File" menu */
		MWS_P_Quit(theMsg);
		break;

	case 259:	/* prime response - synch is id of next script to play */
		MWS_P_Prime(theMsg);
		break;

	case 260:	/* dispatch resp. - name returned is of script document */
		if (IIMWdvvDViewVersion(theMsg) == FAIL)
			status = ERROR;
		break;

	case 261:	/* MacWorkStation version response */
		if (IIMWmvMwsVersion(theMsg) == FAIL)
			status = ERROR;
		break;

	case 262:	/* system version response */
		if (IIMWsvSysVersion(theMsg) == FAIL)
			status = ERROR;
		break;

	default:
		IIMWplPrintLog(mwLOG_DIR_MSG, theMsg->msgData);
		break;

	}					
	return(status);
}

/*{
** Name:  doText -- Act on event returned by the Text Director.
**
** Description:
**	The Text Director returned an event.  Take appropriate
**	action.
**
** Inputs:
**	theMsg	Pointer to message received from MWS
**	id	Id code of message received
**
** Outputs:
** 	Returns:
**		Status info; ERROR, IGNORE or RETURN.
** 	Exceptions:
**		None.
**
** Side Effects:
**
** History:
**	25-sep-89 (nasser)
**		Initial definition.
*/
static i4
doText(theMsg,id)
TPMsg	theMsg;
i4	id;
{
	switch(id) 
	{

	default:
		IIMWplPrintLog(mwLOG_DIR_MSG, theMsg->msgData);
		break;

	}
	return(IGNORE);
}

/*{
** Name:  doWindow -- Act on event returned by the Window Director.
**
** Description:
**	The Window Director returned an event.  Take appropriate
**	action.
**
** Inputs:
**	theMsg	Pointer to message received from MWS
**	id	Id code of message received
**
** Outputs:
** 	Returns:
**		Status info; ERROR, IGNORE or RETURN.
** 	Exceptions:
**		None.
**
** Side Effects:
**
** History:
**	25-sep-89 (nasser)
**		Initial definition.
*/
static i4
doWindow(theMsg,id)
TPMsg	theMsg;
i4	id;
{
	switch(id) 
	{

	default:
		IIMWplPrintLog(mwLOG_DIR_MSG, theMsg->msgData);
		break;

	}
	return(IGNORE);
}

# endif /* DATAVIEW */
