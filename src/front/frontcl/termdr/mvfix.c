/*
**  MOVEFIX.c - Fix moves for vms c compiler.
**
**  Copyright (c) 2004 Ingres Corporation
**
**  History:
**	10-sep-87 (bab)
**		Changed from use of ERR to FAIL (for DG.)
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	28-Mar-2005 (lakvi01)
**	    Corrected function prototypes.
*/


# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<termdr.h>

STATUS
TDmvchwadd(win, y, x, ch)
WINDOW	*win;
i4	y;
i4	x;
char	ch;
{
	return(TDmove(win, y, x) == FAIL ? FAIL : TDadch(win, ch));
}

/*
STATUS
TDmvwgetch(win, y, x)
{
	return(TDmove(win, y, x) == FAIL ? FAIL : TDgetch(win));
}
*/

STATUS
TDmvstrwadd(win, y, x, str)
WINDOW	*win;
i4	y;
i4	x;
char	*str;
{
	return(TDmove(win, y, x) == FAIL ? FAIL : TDaddstr(win, str));
}

/*
STATUS
TDmvwgetstr(win, y, x)
{
	return(TDmove(win, y, x) == FAIL ? FAIL : TDgetstr(win));
}
*/

/*
STATUS
TDmvwinch(win, y, x)
{
	return(TDmove(win, y, x) == FAIL ? FAIL : TDinch(win));
}
*/

/*
STATUS
TDmvwdelch(win, y, x)
{
	return(TDmove(win, y, x) == FAIL ? FAIL : TDdelch(win));
}
*/

/*
STATUS
TDmvwinsch(win, y, x, c)
{
	return(TDmove(win, y, x) == FAIL ? FAIL : TDinsch(win, c));
}
*/
