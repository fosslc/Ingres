/*
**  VThilite.c
**
**  Copyright (c) 2004 Ingres Corporation
**
**  static	char	Sccsid[] = "@(#)vthilite.c	30.1	12/3/84";
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
*/

# include	<compat.h>
# include	<pc.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	 <te.h>
# include	"vtframe.h"

/*
**  hi light the passed position
*/

VOID
VThilite(frm, starty, startx, endy, endx)
reg	FRAME	*frm;
reg	i4	starty;
reg	i4	startx;
reg	i4	endy;
reg	i4	endx;
{
	char	c1;
	char	c2;
	char	c3;
	char	c4;
	char	STANDOUT = '$';

/*
	if (frm == NULL)
		return;
*/
	TEput('\007');
	TDmove(frm->frscr, starty, startx);
#ifndef WSC
	c1 = TDinch(frm->frscr);
#else
	c1 = TDinch(((struct _win_st *)(frm->frscr)));
#endif
	TDmove(frm->frscr, starty, endx);
#ifndef WSC
	c2 = TDinch(frm->frscr);
#else
	c2 = TDinch(((struct _win_st *)(frm->frscr)));
#endif
	TDmove(frm->frscr, endy, startx);
#ifndef WSC
	c3 = TDinch(frm->frscr);
#else
	c3 = TDinch(((struct _win_st *)(frm->frscr)));
#endif
	TDmove(frm->frscr, endy, endx);
#ifndef WSC
	c4 = TDinch(frm->frscr);
#else
	c4 = TDinch(((struct _win_st *)(frm->frscr)));
#endif
	TDmvchwadd(frm->frscr, starty, startx, STANDOUT);
	TDmvchwadd(frm->frscr, starty, endx, STANDOUT);
	TDmvchwadd(frm->frscr, endy, startx, STANDOUT);
	TDmvchwadd(frm->frscr, endy, endx, STANDOUT);
	TDmove(frm->frscr, VTgloby, VTglobx);
	TDrefresh(frm->frscr);
	PCsleep(1000);	/* was 500 (dkh) */
	TEput('\007');
	TDmvchwadd(frm->frscr, starty, startx, c1);
	TDmvchwadd(frm->frscr, starty, endx, c2);
	TDmvchwadd(frm->frscr, endy, startx, c3);
	TDmvchwadd(frm->frscr, endy, endx, c4);
	TDmove(frm->frscr, VTgloby, VTglobx);
	TDrefresh(frm->frscr);
}
