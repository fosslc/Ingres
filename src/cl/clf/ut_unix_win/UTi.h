/*
** Copyright (c) 2005 Ingres Corporation
**
**	Name:
**	    UTi.h
**
**	Function:
**	    None
**
**	Arguments:
**	    None
**
**	Result:
**	    declares the UT module global UTstatus.
**	    defines the debugging macro REPORT_ERR which calls UTerror to print
**	    the error message associated with an error status returned
**	    by a UT routine.
**
**	Side Effects:
**	    None
**
**	History:
**	    03/83 -- (gb)
**	        written
**          06-Feb-96 (fanra01)
**              Changed extern to GLOBALREF.
**	    29-Apr-2005 (hanje04)
**		GLOBALREF UTdebug as it causes multiple defn errors
**		on Mac OS X.
**
**
*/

/* static char	*Sccsid = "@(#)UTi.h	4.5	12/18/84"; */


# include	<compat.h>	/* compatability library types */


GLOBALREF		STATUS			UTstatus;


# define	REPORT_ERR(routine)	if ((UTstatus = routine) != OK) UTerror(UTstatus)

GLOBALREF		char	*UTdebug;	
#define	DBGprintf	if(UTdebug!=NULL)SIprintf
