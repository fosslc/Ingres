/*
**  Copyright (c) 2010 Ingres Corporation
*/
# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<cm.h>
# include	<fe.h>
# include	<si.h>
# include	<st.h>
# include	<termdr.h>

/*
**  crput.c
**
**  History:
**	19-jun-87 (bruceb)	Code cleanup.
**	08/14/87 (dkh) - ER changes.
**	24-sep-87 (bruceb)
**		Addition of the XL capability code.  Also cosmetic changes.
**	01-oct-87 (bruceb)
**		Take _starty into account when determining character
**		and display attributes in TDplod.  Also uses _startx,
**		although at this time, that structure member has yet be 
**		anything other than the default 0.
**	10/05/87 (dkh) - Changed TDsmvcur to handle color and fonts too.
**	07/25/88 (dkh) - Added XC to support color on Tektronix.
**	09/05/88 (dkh) - Fixed output when window being refreshed does
**			 not cover the area cursor is moving over.
**	10/22/88 (dkh) - Performance changes.
**	05/19/89 (dkh) - Fixed bug 6009.
**	07/11/89 (dkh) - Rearranged the order attributes are sent out.
**	21-dec-89 (bruceb)
**		If DOUBLEBYTE, always perform cursor motion using the
**		CM string (if CA), since otherwise risk placing the second
**		byte of a two byte character into the print buffer.
**	08/28/90 (dkh) - Integrated porting change ingres6202p/131215.
**	08/30/90 (dkh) - Integrated porting change ingres6202/130036.
**			 Supports HP terminals in HP mode.
**	08/19/91 (dkh) - Integrated changes from PC group.
**	14-Mar-93 (fredb) hp9_mpe
**		Integrated 6.2/03p change:
**      		14-sep-89 (fredb) - Changed use of NONL in TDplod
**					    to be consistent with TDscroll
**					    and TDrscroll.
**      29-sep-95 (kch)
**              In the function TDfgoto(), we now test for destcol and outcol
**              greater than COLS, rather than cols_1. Previously, if destcol
**              and outcol were equal to COLS, they would be set to zero by
**              the '%= COLS' code, and destline and outline would be
**              incremented. This change fixes bug 60190.
**      24-sep-96 (mcgem01)
**              Global data moved to termdrdata.c
**	30-jun-97 (consi01) bug 82712
**		Backed out part of change for 29-sep-95 (kch) as it caused
**		bug 82712
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      01-aug-2006 (huazh01)
**          On TDdaout(), always write attribute info if it is a HP
**          terminals on HP mode. This fixes 116440. 
**      12-nov-2009 (huazh01)
**          For HP terminals, TDdaout() now always write display attribute 
**          regardless if it is same as previous one. (b122581)
**      22-Mar-2010 (coomi01) b123449
**          Change #ifdef DOUBLEBYTE into runtime if (CMGETDBL) statement.
**          The CM flag is set by the installation wide use of a double byte
**          character set.
*/

FUNC_EXTERN char	*TDtgoto();
void			TDplodput();
i4			TDplod();

GLOBALREF u_char	pfonts;
GLOBALREF u_char	pcolor;
GLOBALREF u_char	pda;
GLOBALREF i4		IITDgotolen;
GLOBALREF char		*IITDptr_prev;

/*
** Terminal driving and line formatting routines.
** Basic motion optimizations are done here as well
** as formatting of lines (printing of control characters,
** line numbering and the like).
**
** Copyright (c) 2004 Ingres Corporation
**
*/

/*
**  Sync the position of the output cursor.
**  Most work here is rounding for terminal boundaries getting the
**  column position implied by wraparound or the lack thereof and
**  rolling up the screen to get destline on the screen.
*/

GLOBALREF i4	outcol;
GLOBALREF i4	outline;
GLOBALREF i4	destcol;
GLOBALREF i4	destline;
GLOBALREF i4	plodcnt;
GLOBALREF bool	plodflg;
GLOBALREF WINDOW	*_win;
GLOBALREF WINDOW	*_pwin;
GLOBALREF bool		_curwin;

TDmvcur(ly, lx, y, x)
i4	ly;
i4	lx;
i4	y;
i4	x;
{

	destcol = x;
	destline = y;
	outcol = lx;
	outline = ly;
	TDfgoto();
}

TDfgoto()
{

	reg	char	*cgp;
	reg	i4	l;
	reg	i4	c;
	reg	i4	gotolen;
		i4	cols_1 = COLS - 1;
		i4	lines_1 = LINES - 1;
		i4	_TDputchar();

	if (destcol > COLS)
	{
		destline += destcol / COLS;
		destcol %= COLS;
	}
	if (outcol > cols_1 && outline < lines_1 - 1)
	{
		l = (outcol + 1) / COLS;
		outline += l;
		outcol %= COLS;
		if (AM == 0)
		{
			while (l > 0)
			{
				if (XL)
					TDput(XL);
				else
					TDput('\n');
				if (_pfast)
					TDput('\r');
				l--;
			}
			outcol = 0;
		}
		if (outline > lines_1)
		{
			destline -= outline - (lines_1);
			outline = lines_1;
		}
	}
	if (destline > lines_1)
	{
		l = destline;
		destline = lines_1;
		if (outline < lines_1)
		{
			c = destcol;
			if (_pfast == 0 && !CA)
				destcol = 0;
			TDfgoto();
			destcol = c;
		}
		while (l > lines_1)
		{
			if (XL)
				TDput(XL);
			else
				TDput('\n');
			l--;
			if (_pfast == 0)
				outcol = 0;
		}
	}
	if (destline < outline && !(CA || UP != NULL))
		destline = outline;
	cgp = TDtgoto(CM, destcol, destline);
	if (!(gotolen = IITDgotolen))
	{
		gotolen = STlength(cgp);
	}
	if (CA)
	{
	    if (CMGETDBL)
	    {
		TDtputs(cgp, (i4) 0, _TDputchar);
	    }
	    else
	    {
		if (destline < outline && UP == NULL)
			TDtputs(cgp, (i4) 0, _TDputchar);
		else if (TDplod(gotolen) > 0)
			TDplod((i4) 0);
		else
			TDtputs(cgp, (i4) 0, _TDputchar);
	    }
	}
	else
	{
		TDplod((i4) 0);
	}
	outline = destline;
	outcol = destcol;
}

i4
_TDputchar(c)
reg	char	c;
{
	SIputc(c, stdout);
}


i4
TDplod(cnt)
reg	i4	cnt;
{
	reg	i4	i;
	reg	i4	j;
	reg	i4	k;
	reg	i4	soutcol;
	reg	i4	soutline;
	reg	char	c;
	reg	u_char	fonts;
	reg	u_char	da;
	reg	u_char	color;
	reg	char	*winy;
	reg	char	*winda;
	reg	i4	line;
	reg	i4	col;
		WINDOW	*twin;

	plodcnt = plodflg = cnt;
/*	plodcnt = cnt;	*/
	if (cnt)
	{
		plodflg = TRUE;
	}
	soutcol = outcol;
	soutline = outline;
	if (HO)
	{
		i = destcol;
	        if (destcol >= outcol)
		{
			j = destcol - outcol;
	        }
		else
			if (outcol - destcol <= i && (BS || BC))
				i = j = outcol - destcol;
			else
				j = i + 1;
		k = outline - destline;
		if (k < 0)
			k = -k;
		j += k;
		if (i + destline < j)
		{
			TDtputs(HO, (i4) 0, TDplodput);
			outcol = outline = 0;
		}
		else if (LL)
		{
			k = (LINES - 1) - destline;
			if (i + k + 2 < j)
			{
				TDtputs(LL, (i4) 0, TDplodput);
				outcol = 0;
				outline = LINES - 1;
			}
		}
	}
	i = destcol;

	j = outcol - destcol;

	while (outline < destline)
	{
		outline++;
		if (XL)
		{
		/*
			TDplodput(XL);
		*/
			if (plodflg)
			{
				plodcnt--;
			}
			else
			{
				TDput(XL);
			}
		}
		else
		{
		/*
			TDplodput('\n');
		*/
			if (plodflg)
			{
				plodcnt--;
			}
			else
			{
				TDput('\n');
			}
		}
		if (plodcnt < 0)
			goto out;
		if (!NONL || _pfast == 0)
			outcol = 0;
	}
	if (BT)
		k = STlength(BT);
	while (outcol > destcol)
	{
		if (plodcnt < 0)
			goto out;

		outcol--;
		if (BC)
		{
			TDtputs(BC, (i4) 0, TDplodput);
		}
		else
		{
		/*
			TDplodput('\b');
		*/
			if (plodflg)
			{
				plodcnt--;
			}
			else
			{
				TDput('\b');
			}
		}
	}
	while (outline > destline)
	{
		outline--;
		TDtputs(UP, (i4) 0, TDplodput);
		if (plodcnt < 0)
			goto out;
	}
	if (_win != NULL)
	{
		twin = _win;
		if (_curwin)
		{
			line = outline;
			col = outcol;
		}
		else
		{
			line = outline - (curscr->_begy + _win->_begy +
				_pwin->_starty);
			col = outcol - (curscr->_begx + _win->_begx +
				_pwin->_startx);
			if (line < 0 || col < 0)
			{
				line = outline;
				col = outcol;
				twin = curscr;
			}
		}
		winy = &(twin->_y[line][col]);
		winda = &(twin->_da[line][col]);
	}
	while (outcol < destcol)
	{
		if (_win != NULL)	/* implies that _pwin also non-NULL */
		{
			if (plodflg)	/* avoid a complex calculation */
			{
				/*
				** For HP terminals (XS) force an early
				** termination to this function now. It is
				** getting a little too hairy for us here ...
				*/
				if (XS)
				{
					plodcnt = -1;
				}
				else
				{
					plodcnt--;
				}
			}
			else
			{
				c = *winy++;
				da = *winda++;

				fonts = da & _LINEMASK;
				color = da & _COLORMASK;
				da &= _DAMASK;
				TDdaout(fonts, da, color);
				TDput(c);
			}
		}
		else if (ND)
		{
			TDtputs(ND, (i4) 0, TDplodput);
		}
		else
		{
		/*
			TDplodput(' ');
		*/
			if (plodflg)
			{
				plodcnt--;
			}
			else
			{
				TDput(' ');
			}
		}
		outcol++;
		if (plodcnt < 0)
			goto out;
	}
out:
	if (plodflg)
	{
		outcol = soutcol;
		outline = soutline;
	}
	return(plodcnt);
}

/*
** Move (slowly) to destination.
** Hard thing here is using home cursor on really deficient terminals.
** Otherwise just use cursor motions, hacking use of tabs and overtabbing
** and backspace.
*/

void
TDplodput(c)
reg	char	c;
{

	if (plodflg)
		plodcnt--;
	else
	{
		TDput(c);
	}
}



/*
**  TDsmvcur
**
**  Routine to handle the save moving of the cursor
**  on terminals that stays in high lighted mode
**  (such as verse video) when the cursor is moved
**  via "cm".  Fix for BUG 5774. (dkh)
*/

TDsmvcur(oy, ox, ny, nx)
i4	oy;
i4	ox;
i4	ny;
i4	nx;
{
	if (!MS)
	{
		/*
		** IF we are running on an HP terminal (XS) AND
		** IF we were writing enhanced text (pda) and we know where
		** we were doing it (IITDptr_prev) and the enhancement is not
		** continued to the next position from where we were
		** OR we were writing enhanced text (pda) on a non-HP
		** terminal (!XS) THEN turn off the enhancement before moving
		** the cursor.
		*/
		if ((XS && pda && IITDptr_prev &&
			(pda != (*(IITDptr_prev+1) & _DAMASK))) ||
			(pda && !XS))
		{
			_puts(EA);
			pda = '\0';
			if (XC)
			{
				_puts(YA);
				pcolor = '\0';
			}
		}
		if (pcolor)
		{
			_puts(YA);
			pcolor = '\0';
		}
		/*
		** IF we are running on an HP terminal (XS) AND
		** IF we were doing line drawing (pfonts) and we know where
		** we were doing it (IITDptr_prev) and the line drawing is not
		** continued to the next position from where we were
		** OR we were doing line drawing (pfonts) on a non-HP
		** terminal (!XS) THEN turn off line drawing before moving
		** the cursor.
		*/
		if ((XS && pfonts && IITDptr_prev &&
			(pfonts != (*(IITDptr_prev+1) & _LINEMASK))) ||
			(pfonts && !XS))
		{
			_puts(LE);
			pfonts = '\0';
		}
	}
	TDmvcur(oy, ox, ny, nx);
}


/*
**  Output any code necessary to set/reset display attributes.
*/

TDdaout(fonts, da, color)
u_char	fonts;
u_char	da;
u_char	color;
{
	if ((LD != NULL) && XD)
	{
		if (fonts == _LINEDRAW)
		{
			_puts(LS);
		}
		pfonts = fonts;
	}
	else if ((LD != NULL) && (pfonts != fonts || XS))
	{
		if (fonts == _LINEDRAW)
		{
			_puts(LS);
		}
		else
		{
			_puts(LE);
		}
		pfonts = fonts;
	}
	if (XC)
	{
		if (pda != da)
		{
			if (pda != '\0')
			{
				_puts(EA);
				_puts(YA);
				pcolor = '\0';
			}
			TDdaput(da);
			pda = da;
		}
		if ((YA != NULL) && (pcolor != color))
		{
			TDputcolor(color);
			pcolor = color;
		}
	}
	else
	{
		if ((YA != NULL) && (pcolor != color))
		{
			TDputcolor(color);
			pcolor = color;
		}
		if (pda != da || XS)
		{
			if (pda != '\0')
			{
				_puts(EA);
			}
			TDdaput(da);
			pda = da;
		}
	}
}
