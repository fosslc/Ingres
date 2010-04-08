
/*
**  VTputstr.c
**
**  Copyright (c) 2004 Ingres Corporation
**
**  History:
**	12/03/84 (dkh) - Initial version.
**	05/08/87 (dkh) - Fixed kanji introduced problem with no
**			 display attributes for strings other than 1st char.
**	19-jun-87 (bab)	Code cleanup.
**	11/01/88  (tom) - performance change
**	11/15/88  (tom) - fixed bug introduced during perf. change
**	04-sep-90 (bruceb)
**		Added dummy params to VTputstring() and VTputvstr()--for
**		use only in FT3270 version.
**	14-mar-95 (peeje01) - Cross integration from 6500db_su4_us5
**		Original comments appear below:
** **      06-jul-92 (kirke)
** **              Fixed VTputvstr() so that strings containing multi-byte
** **              characters are handled properly.
** **		   Still need to fix VTdelvstr()
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
# include	<cm.h>
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	"vtframe.h"

GLOBALREF char dispattr;
GLOBALREF char colorattr;

VOID
VTputstring(frm, y, x, string, attributes, hilight, dummy)
FRAME	*frm;
i4	y;
i4	x;
char	*string;
i4	attributes;
bool	hilight;
bool	dummy;
{
	char	*cp;
	char	*daptr;
	char	*dxptr;
	i4	atr;
	i4	i;
	WINDOW	*win;
	i4	VTdistinguish();

	TDmove(frm->frscr, y, x);
	if (hilight)
	{
		if (LD == NULL)
		{
			TDadch(frm->frscr, '$');
			for (i = STlength(string) - 1, cp = string + 1;
				i > 1;  i--, cp++)
			{
				TDadch(frm->frscr, *cp);
			}
			if (i == 1)
			{
				TDadch(frm->frscr, '$');
			}
			return;
		}
		else
		{
			attributes = VTdistinguish(attributes);
		}
	}
	TDsetdispattr(attributes);
	atr = dispattr | colorattr;
	win = frm->frscr;

	/* the following code has changed for performance reasons..
	   where there used to be 3 function calls in the loop 
	   now there are none, but rather we just clear the attributes
	   by passing a pointer through the screen memory */
	daptr = &win->_da[y][x];
	dxptr = &win->_dx[y][x];
	for (cp = string; *cp; cp++)
	{
		*dxptr++ = '\0';
		*daptr++ = atr;
	}
	TDaddstr(win, string);

	TDclrdispattr();
}

VOID
VTputvstr(frm, y, x, string, attributes, hilight, dummy)
FRAME	*frm;
i4	y;
i4	x;
char	*string;
i4	attributes;
bool	hilight;
bool	dummy;
{
	char	*cp;
	WINDOW	*win;

	win = frm->frscr;
	TDmove(win, y, x);
	TDsetdispattr(attributes);
	for (cp = string; *cp; CMnext(cp))
	{
		TDersdispattr(win);
		TDadddispattr(win);
# ifdef DOUBLEBYTE
		if (CMdbl1st(cp))
                         TDxaddstr(win, CMbytecnt(cp), cp);
		else
# endif
		TDadch(win, *cp);
		TDmove(win, ++y, x);
	}
	TDclrdispattr();
}


VTdelvstr(frm, y, x, length)
FRAME	*frm;
i4	y;
i4	x;
i4	length;
{
	i4	i;
	WINDOW	*win;

	win = frm->frscr;
	TDmove(win, y, x);
	for (i = 0; i < length; i++)
	{
		TDmove(win, i, x);
		TDadch(win, ' ');
	}
}
