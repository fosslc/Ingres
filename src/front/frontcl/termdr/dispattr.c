/*
**  Dispattr.c - file to handle turning on/off of display, fonts
**  and color attributes.
**
**  Copyright (c) 2004 Ingres Corporation
**
**  History:
**	Created 2-18-83 (dkh)
**	19-jun-87 (bab)	Code cleanup.
**	10/22/88 (dkh) - Performance changes.
**	11/01/88 (dkh) - More perf changes.
**	31-may-90 (rog)
**		Included <me.h> so MEfill() is defined correctly.
**	08/12/90 (dkh) - Changed TDcolor semantics to handle multiple
**			 colors that can occur as a result of supporting
**			 table field cell highlighting.
**	08/28/90 (dkh) - Integrated porting change ingres6202p/131215.
**      24-sep-96 (mcgem01)
**              Global data moved to termdrdata.c
**      01-oct-96 (mcgem01)
**              extern changed to GLOBALREF for global data project.
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
# include	<me.h>
# include	<te.h>
# include	<fe.h>
# include	<termdr.h>
# include	<ft.h>
# include	<fmt.h>
# include	<adf.h>
# include	<frame.h>


GLOBALREF char	dispattr;
GLOBALREF char	fontattr;
GLOBALREF char	colorattr;
GLOBALREF u_char	pda;
GLOBALREF u_char	pfonts;
GLOBALREF u_char	pcolor;

VOID
TDersdispattr(win)
reg	WINDOW	*win;
{
	win->_da[win->_cury][win->_curx] = '\0';
	win->_dx[win->_cury][win->_curx] = '\0';
}

VOID
TDadddispattr(win)
reg	WINDOW	*win;
{
	win->_da[win->_cury][win->_curx] |= (dispattr | colorattr);
}

VOID
TDclrattr()
{
	char	outbuf[TDBUFSIZE];

	TDobptr = outbuf;
	TDoutbuf = outbuf;
	TDoutcount = TDBUFSIZE;
	_puts(EA);
	_puts(LE);
	_puts(YA);
	TEwrite(TDoutbuf, (i4)(TDBUFSIZE - TDoutcount));
	TEflush();
	pda = 0;
	pfonts = 0;
	pcolor = 0;
}



#ifdef CMS
/*
**	TDcolor must be a static under CMS to avoid a name conflict
*/
static
#endif
char
TDcolor(attributes)
i4	attributes;
{
	i4	attr;
	char	color = 0;

	attr = attributes & fdCOLOR;
	if (attr & fd1COLOR)
	{
		color = _COLOR1;
	}
	else if (attr & fd2COLOR)
	{
		color = _COLOR2;
	}
	else if (attr & fd3COLOR)
	{
		color = _COLOR3;
	}
	else if (attr & fd4COLOR)
	{
		color = _COLOR4;
	}
	else if (attr & fd5COLOR)
	{
		color = _COLOR5;
	}
	else if (attr & fd6COLOR)
	{
		color = _COLOR6;
	}
	else if (attr & fd7COLOR)
	{
		color = _COLOR7;
	}
	return(color);
}


VOID
TDsetdispattr(attributes)
i4	attributes;
{
	dispattr = '\0';
	colorattr = '\0';
	if (RV && attributes & fdRVVID)
	{
		dispattr |= _RVVID;
	}
	if (BL && attributes & fdBLINK)
	{
		dispattr |= _BLINK;
	}
	if (US && attributes & fdUNLN)
	{
		dispattr |= _UNLN;
	}
	if (BO && attributes & fdCHGINT)
	{
		dispattr |= _CHGINT;
	}
	if (YA && attributes & fdCOLOR)
	{
		colorattr = TDcolor(attributes);
	}
}

VOID
TDclrdispattr()
{
	dispattr = '\0';
}

VOID
TDsetattr(win, attributes)
WINDOW	*win;
i4	attributes;
{
	reg	char	da;
	reg	char	*dp;
	reg	i4	x;
	reg	i4	y;

	da = '\0';
	if (RV && attributes & fdRVVID)
	{
		da |= _RVVID;
	}
	if (BL && attributes & fdBLINK)
	{
		da |= _BLINK;
	}
	if (US && attributes & fdUNLN)
	{
		da |= _UNLN;
	}
	if (BO && attributes & fdCHGINT)
	{
		da |= _CHGINT;
	}
	if (YA && attributes & fdCOLOR)
	{
		da |= TDcolor(attributes);
	}
	for (y = 0; y < win->_maxy; y++)
	{
		for (x = 0, dp = win->_da[y]; x < win->_maxx; x++)
		{
			*dp++ |= da;
		}
	}
}




/*{
** Name:	TDputattr - Put attributes into window.
**
** Description:
**	This routine puts the passed in attributes into the window
**	and overwrites whatever attributes was previously set.
**
** Inputs:
**	win		Window to set attributes in.
**	attributes	Attributes to set into window.
**
** Outputs:
**	None.
**
**	Returns:
**		None.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	11/01/88 (dkh) - Initial version.
*/
VOID
TDputattr(win, attributes)
WINDOW	*win;
i4	attributes;
{
	reg	i4	y;
	reg	i4	maxy;
	reg	i4	maxx;
		char	da;

	da = '\0';
	if (RV && attributes & fdRVVID)
	{
		da |= _RVVID;
	}
	if (BL && attributes & fdBLINK)
	{
		da |= _BLINK;
	}
	if (US && attributes & fdUNLN)
	{
		da |= _UNLN;
	}
	if (BO && attributes & fdCHGINT)
	{
		da |= _CHGINT;
	}
	if (YA && attributes & fdCOLOR)
	{
		da |= TDcolor(attributes);
	}
	maxy = win->_maxy;
	maxx = win->_maxx;
	for (y = 0; y < maxy; y++)
	{
		MEfill(maxx, da, win->_da[y]);
	}
}
