/*
** Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<pc.h>		/* 6-x_PC_80x86 */
# include	<er.h>
# include	<si.h>
# include	<st.h> 
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<iisqlca.h>
# include	<iilibq.h>
# include	<lqgdata.h>

/**
+*  Name: iisyserr.c - Syserror file for front end support.
**
**  Defines:
**	syserr - Syserr function.
-*
**/

/*{
+*  Name: syserr - Routine to generate SYSTEM ERRORS for front ends.
**
**  Description:
**	Patterned after the syserr function in gutil.  It takes
**	parameters like those passed to SIprintf, and prints them
**	on standard output.  It then signals a fatal error.
**
**  Inputs:
**	p0 - p5 -	the SIprintf-like parameters
**
**  Outputs:
**	Returns:
**	    Nothing
-*
**  Side Effects:
**	
**  History:
**	02-apr-1983 	- Written. (lichtman)
**	16-jun-1983 	- Placed in libq for compatlib. (jen)
**	04-nov-1983     - Rewrote for interface to forms system. (ncg)
**	29-jun-87 (bab)	Added VARARGS comment to help quiet lint.
**	18-feb-1988	- Added PCexit call to follow IIabort which no longer
**			  exits.  (marge) 
**	04-nov-90 (bruceb)
**		Added ref of IILIBQgdata so file would compile.
**	18-Dec-97 (gordy)
**	    Added support for multi-threaded applications.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

GLOBALREF LIBQGDATA	*IILIBQgdata;

/* VARARGS */
void
syserr(p0, p1, p2, p3, p4, p5)
char	*p0, *p1, *p2, *p3, *p4, *p5;
{
    static    i4	firstcall = TRUE;

    if (IILIBQgdata->form_on && IILIBQgdata->f_end)
    {
	(*IILIBQgdata->f_end)( TRUE );
	IILIBQgdata->form_on = FALSE;
    }

    if (firstcall)
    {
	/*
	** IIabort may do something that will cause recursive errors.
	** To avoid recursive syserrors reset flag.  There is a small
	** multi-threading hole here, but we are more worried about
	** recursion than a race condition.
	*/
	firstcall = FALSE;
	SIprintf(ERx("\nSYSTEM ERROR: "));	/* Do NOT use ERget here */
	SIprintf(p0, p1, p2, p3, p4, p5);
	SIprintf(ERx("\n\n"));
	SIflush(stdout);
	IIabort();
	PCexit( -1 );
    }
    else	/* Being called recursively */
    {
	PCexit( -1 );
    }
}
