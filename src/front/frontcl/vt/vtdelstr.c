/*
**  VTdelstr.c
**
**  Copyright (c) 2004 Ingres Corporation
**
**  static	char	Sccsid[] = "@(#)vtdelstr.c	30.1	12/3/84";
**
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	"vtframe.h"

VOID
VTdelstring(frm, y, x, length)
FRAME	*frm;
i4	y;
i4	x;
i4	length;
{
	reg	WINDOW	*win;
	reg	i4	i;

	win = frm->frscr;
	TDmove(win, y, x);
	for (i = 0; i < length; i++)
	{
		TDersdispattr(win);
		TDadch(win, ' ');
	}
}
