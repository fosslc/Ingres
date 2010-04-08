/*
**Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: DMFMERGE.H - Used for DMF merge proc
**
** Description:
**	This file contains the header information of the dmfmerge.c
**	module.
**
** History:
**	 20-Oct-1993 (rmuth)
**	    Created for PM startup resource.
**  	26-Jan-1994 (daveb) 58733
**          changed priv resource from "config.privileges." to
**  	    "privileges.user.", and the priv to be checked from
**  	    "SERVER_STARTUP" to "SERVER_CONTROL", as per approved
**  	    LRC proposal of 10-Nov-93.
**/


/*
** PM defines used to check that user is allowed to start a process
**
** An example would be
**
**	ii.medfly.privileges.user.ingres: SERVER_CONTROL, more
*/
#define PM_START_PRIV_RES "ii.$.privileges.user."
#define PM_START_PRIV_LEN (sizeof(PM_START_PRIV_RES)-1)
#define PM_START_PRIVILEGE   "SERVER_CONTROL"

