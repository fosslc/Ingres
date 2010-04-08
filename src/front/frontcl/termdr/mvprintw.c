# include	<compat.h>
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<termdr.h>

/*
**  Implement the mvprintw commands.  Due to the variable number of
**  arguments, they cannot be macros.  Sigh....
**
**  Copyright (c) 2004 Ingres Corporation
**
**  History:
**	10-sep-87 (bab)
**		Changed from use of ERR to FAIL (for DG.)
**      11-jan-1996 (toumi01; from 1.1 axp_osf port)
**              Changed args agr1-arg8 from i4  to PTR for pointer
**              portability (axp_osf port)
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/


/*
**  Routine TDmvprintw does not appear to be used. (dkh)
*/

/*
TDmvprintw(y, x, fmt, args)
reg	i4	y;
reg	i4	x;
	char	*fmt;
	i4	args;
{

	return(TDsmove(y, x) == OK ? _TDsprintw(stdscr, fmt, &args) : FAIL);
}
*/

/* VARARGS4 */
TDmvwprintw(win, y, x, fmt, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8)
reg	WINDOW	*win;
reg	i4	y;
reg	i4	x;
char	*fmt;
PTR	arg1;
PTR	arg2;
PTR	arg3;
PTR	arg4;
PTR	arg5;
PTR	arg6;
PTR	arg7;
PTR	arg8;
{
	char	bufr[512];

	if (TDmove(win, y, x) != OK)
	{
		return(FAIL);
	}
	STprintf(bufr, fmt, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8);
	return(TDaddstr(win, bufr));

/*
	return(TDmove(win, y, x) == OK ? _TDsprintw(win, fmt, &args) : FAIL);
*/
}
