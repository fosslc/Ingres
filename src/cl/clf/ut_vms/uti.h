/*
** static char	Sccsid[] = "%W%	%G%";
*/

/*
 *	Copyright (c) 1983 Ingres Corporation
 *
 *	Name:
 *		UTi.h
 *
 *	Function:
 *		None
 *
 *	Arguments:
 *		None
 *
 *	Result:
 *		declares the UT module global UTstatus.
 *		defines the debugging macro REPORT_ERR which calls UTerror to print
 *			the error message associated with an error status returned
 *			by a UT routine.
 *
 *	Side Effects:
 *		None
 *
 *	History:
 *		03/83 -- (gb)
 *			written
 *
 *
 */



extern		STATUS			UTstatus;


# define	REPORT_ERR(routine)	if ((UTstatus = routine) != OK) UTerror(UTstatus)

#define	DBGprintf	if(UTdebug!=NULL)SIprintf
