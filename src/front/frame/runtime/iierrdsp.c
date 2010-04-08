/*
**	Copyright (c) 2004 Ingres Corporation
*/
/*
**	IIERRDISP.c  -  INGRES Window Error Message Routine
**	
**	IIerrdisp() receives an error message from INGRES and prints it out
**	on to a window at the bottom of the user's screen.  Provides a nice
**	neat way of printing out INGRES errors when using curses.
**	
**	Returns:  1 - If no INGRES error
**		  0 - If INGRES error occured and printed out successfully
**		 -1 - If error message unsuccessfully printed out
**
**	Side Effects: 1) A new static window is created when this routine is
**			 called.
**		      2) The standard INGRES printing routines must be replaced
**			 with Paul's routines in order to work.  IItest_err()
**			 now returns a string containing the error message.
**	(c) 1981  RTI
**	
**  History:
**	23 Oct 1981  -  written (jen)
**	27 Jan 1982  -  added wrap to error message
**	09/05/87 (dkh) - Removed IIerrmsg() as no one uses it.
**	04/09/88 (dkh) - Changed for VENUS.
**	11/02/90 (dkh) - Replaced IILIBQgdata with IILQgdata().
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	18-Aug-2009 (kschendel) 121804
**	    Update some of the function declarations to fix gcc 4.3 problems.
*/


# include	<compat.h>
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
# include	<rtvars.h>
# include	<lqgdata.h>


void
IIerrdisp(char *message)			/* IIERRDISP: */
{
	if (IILQgdata()->form_on)
	{
		FTboxerr(NULL, message, IIfrscb);
	}
	else
	{
		SIfprintf(stdout, message);
	}
}
