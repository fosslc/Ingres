/*
**  scrmsg.c
**
**  Copyright (c) 2004 Ingres Corporation
**
**  static	char	Sccsid[] = "@(#)scrmsg.c	30.2	2/4/85";
*/

/*
**	SCRMSG -
**		Message to be placed on the bottom of the user's
**		screen.	 Message can be in the form of the printf
**		function.  Window is static and can only be used
**		by this function.
**
**	Parameters:
**		str - printf type format string
**		args - arguments to scrmsg
**
**	Side effects:
**		stdmsg is updated in this routine
**
**	Error messages:
**		none
**
**	Calls:
**		TDnewwin, TDsprintw
**
**  History:
**  10-6-84
**    Reworked for FT project - dkh
**	08/14/87 (dkh) - ER changes.
**  11-jan-1996 (toumi01; from 1.1 axp_osf port)
**        Changed arguments in scrmsg(), winerr(), wwinerr(),
**        and swinerr() from i4  to PTR for 64-bit portability.
**        (axp_osf port)
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
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<er.h>
# include	"erfd.h"



/*
**	9/21/83 (nml) -
**	New constant added, LINELEN = 256 so scrmsg() allocates
**	a buffer of that size instead of 2K for the msg. buffer.
*/

bool
scrmsg(str, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10)	/* SCREEN MESSAGE */
char	*str;						/* printf format */
PTR	a1, a2, a3, a4, a5, a6, a7, a8, a9, a10;	/* arguments */
{
	FTmessage(str, FALSE, FALSE, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10);
	return (TRUE);
}

bool
winerr(str, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10)	/* ERROR MESSAGE */
char	*str;						/* printf format */
PTR	a1, a2, a3, a4, a5, a6, a7, a8, a9, a10;	/* arguments */
{
	FTwinerr(str, FALSE, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10);
	return (TRUE);
}

bool
wwinerr(str, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10)	/* ERROR MESSAGE */
char	*str;						/* printf format */
PTR	a1, a2, a3, a4, a5, a6, a7, a8, a9, a10;	/* arguments */
{
	FTwinerr(str, TRUE, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10);
	return (TRUE);
}


/* silent err msg */
bool
swinerr(str, wait, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10)
char	*str;
bool	wait;
PTR	a1, a2, a3, a4, a5, a6, a7, a8, a9, a10;
{
	char	*cp = str;
	char	buf[250];

# ifdef FT3270
/*
**	 Don't talk about the RETURN key when using 3270's
*/
	cp = str;
# else
	if (wait)
	{
		STcopy(str, buf);
		STcat(buf, ERget(F_FD0001_HIT_RETURN));
		cp = buf;
	}
# endif
	FTswinerr(cp, wait, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10);
	return(TRUE);
}
