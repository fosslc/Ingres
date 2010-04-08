/*
**  sbox.c
**
**  Copyright (c) 2004 Ingres Corporation
**
**  History:
**  Special boxing routine only used by VIFRED.  Taken
**  from box.c in termdr. (dkh)
**	19-jun-87 (bab)	Code cleanup.
**	18-apr-01 (hayke02)
**		Required change to avoid conflict with reserved word restrict.
**		Identified after latest compiler OSFCMPLRS440 installed on
**		axp.osf which includes support for this based on the C9X
**		review draft.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	19-may-2001 (somsa01)
**	    Changed 'restrict' to 'iirestrict' from previous cross-integration.
**	28-Mar-2005 (lakvi01)
**	    Corrected function prototypes.
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<termdr.h>

extern	i4	tdprform;

VOID
TDsbox(awin, by, li, bx, co, vert, hor, corner, attr, hiattr, iirestrict)
WINDOW	*awin;
i4	by;
i4	li;
i4	bx;
i4	co;
char	vert;
char	hor;
char	corner;
i4	attr;
i4	hiattr;		/* hilight attribute (applied to the corners) */
bool	iirestrict;
{
	reg	WINDOW	*win = awin;
	reg	i4	endx;
	reg	i4	endy;
	reg	i4	i;
	reg	char	*fchrs;
	reg	char	*lchrs;
	reg	char	*fatrs;
	reg	char	*latrs;

	endx = bx + co - 1;
	endy = by + li - 1;

	/* test for cases of degenerate boxes */

	if (li == 1)				/* horz. line */
	{
		TDhline(awin, by, bx, co, attr, hiattr);
	}
	else if (co == 1)			/* vert. line */
	{
		TDvline(awin, by, bx, li, attr, hiattr);
	}
	else					/* else regular box */
	{
		fchrs = win->_y[by];
		lchrs = win->_y[endy];
		fatrs = win->_da[by];
		latrs = win->_da[endy];

		TDhv(win, by, endy, bx, endx + 1, vert, hor, attr, iirestrict);

		TDsetcorners(fchrs, lchrs, fatrs, latrs, 
			bx, endx, corner, hiattr);

	}
	for (i = by; i <= endy; i++)
	{
		if (win->_firstch[i] == _NOCHANGE)
		{
			win->_firstch[i] = bx;
			win->_lastch[i] = endx;
		}
		else
		{
			if (bx < win->_firstch[i])
			{
				win->_firstch[i] = bx;
			}
			if (endx > win->_lastch[i])
			{
				win->_lastch[i] = endx;
			}
		}
	}
}
