# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<me.h>
# include	<fe.h>
# include	<termdr.h>

/*
**  This routine scrolls the window up a line.
**
**  Copyright (c) 2004 Ingres Corporation
**
**  History:
**	08/14/87 (dkh) - ER changes.
**	10-sep-87 (bab)
**		Changed from use of ERR to FAIL (for DG.)
**	01-oct-87 (bab)
**		Code added to allow for unusable top of screen.
**	31-may-90 (rog)
**		Included <me.h> so MEfill() is defined correctly.
**	08/19/91 (dkh) - Integrated changes from PC group.
**	26-may-92 (leighb) DeskTop Porting Change:
**		Changed PMFE ifdefs to MSDOS ifdefs to ease OS/2 port.
**      14-Jun-95 (fanra01)
**              Added NT_GENERIC to presentation related MSDOS sections.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

# if defined(MSDOS) || defined(NT_GENERIC)
GLOBALREF i4	IIcurscr_firstch[];	/* 1st char changed on nth line */
# endif	/* MSDOS */

TDrscroll(awin)
WINDOW	*awin;
{
	reg	WINDOW	*win = awin;
	reg	char	*da;
	reg	char	*dx;
	reg	i4	i;
	reg	i4	j;
		char	*temp;
		char	*tda;
		char	*tdx;
		i4	topline;

	if (!win->_scroll)
	{
		return(FAIL);
	}
	if (win == curscr)
	{
		topline = IITDflu_firstlineused;
# if defined(MSDOS) || defined(NT_GENERIC)
		/*
		** Clear 1st-char-changed on each line
		*/
		for (i = LINES; i--; )
			IIcurscr_firstch[i] = 0;
# endif	/* MSDOS */
	}
	else
	{
		topline = 0;
	}
	if (CAS != NULL)
	{
		j = win->_maxy - 2;
	}
	else
	{
		j = win->_maxy - 1;
	}
	temp = win->_y[j];
	tda = win->_da[j];
	tdx = win->_dx[j];
	for (i = j; i > topline; i--)
	{
		win->_y[i] = win->_y[i-1];
		win->_da[i] = win->_da[i-1];
		win->_dx[i] = win->_dx[i-1];
	}
	da = tda;
	dx = tdx;
	MEfill((u_i2) win->_maxx, (u_char) ' ', temp);
	MEfill((u_i2) win->_maxx, (u_char) '\0', da);
	MEfill((u_i2) win->_maxx, (u_char) '\0', dx);
	win->_y[topline] = temp;
	win->_da[topline] = tda;
	win->_dx[topline] = tdx;
	win->_cury++;
	if (win == curscr)
	{
		if (SR == NULL)
		{
			return(FAIL);
		}
		_puts(SR);
		if (!NONL)
		{
			win->_curx = 0;
		}
	}
	return(OK);
}


TDscroll(awin)
WINDOW	*awin;
{
	reg	WINDOW	*win = awin;
	reg	char	*da;
	reg	char	*dx;
	reg	i4	i;
	reg	i4	j;
		char	*temp;
		char	*tda;
		char	*tdx;
		i4	topline;

	if (!win->_scroll)
		return(FAIL);
	if (win == curscr)
	{
		topline = IITDflu_firstlineused;
# if defined(MSDOS) || defined(NT_GENERIC)
		/*
		** Clear 1st-char-changed on each line
		*/
		for (i = LINES; i--; )
			IIcurscr_firstch[i] = 0;
# endif	/* MSDOS */
	}
	else
	{
		topline = 0;
	}
	temp = win->_y[topline];
	tda = win->_da[topline];
	tdx = win->_dx[topline];
	if (CAS != NULL)
	{
		j = win->_maxy - 2;
	}
	else
	{
		j = win->_maxy - 1;
	}
	for (i = topline; i < j; i++)
	{
		win->_y[i] = win->_y[i+1];
		win->_da[i] = win->_da[i+1];
		win->_dx[i] = win->_dx[i+1];
	}
	da = tda;
	dx = tdx;
	MEfill((u_i2) win->_maxx, (u_char) ' ', temp);
	MEfill((u_i2) win->_maxx, (u_char) '\0', da);
	MEfill((u_i2) win->_maxx, (u_char) '\0', dx);
	win->_y[j] = temp;
	win->_da[j] = tda;
	win->_dx[j] = tdx;
	win->_cury--;
	if (win == curscr)
	{
		TDput('\n');
		if(CAS == NULL && win->_cury <= LINES -2)
			TDput('\n');
		if (CAS == NULL)
		{
			TDsmvcur(win->_cury + 2, win->_curx,
				win->_cury + 1, win->_curx);
		}
		if (!NONL)
			win->_curx = 0;
	}
	return(OK);
}
