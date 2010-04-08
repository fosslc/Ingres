/*
**  EXTEND.c  -  Extend the bottom of a Window
**
**  History:  written - 1 Dec 1981  (jen)
**
**  Copyright (c) 2004 Ingres Corporation
**
**  History:
**	xx/xx/xx	-- Created.
**	12/03/86(KY) -- Added for handling of double Bytes attributes (_dx)
**	13-jul-87 (bab)	Changed memory allocation to use [FM]Ereqmem.
**	03/27/90 (fraser) - Changed to use different allocation routines
**			    IITD1waWinAlloc() and iITD2waWinAlloc().
**	05/04/90 (dkh) - Integrated changes from PC into 6.4 codeline.
*/


# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<termdr.h>
# include	<me.h>

FUNC_EXTERN	VOID	IITD1dDelwin();
FUNC_EXTERN	STATUS	IITD1waWinAlloc();
FUNC_EXTERN	STATUS	IITD2waWinAlloc();


WINDOW *
TDextend(awin, nolines)
WINDOW	*awin;
i4	nolines;
{
	reg	WINDOW	*win = awin;
	reg	i4	i;
	reg	i4	cn;
	reg	i4	ln;
	reg	i4	omaxy;
	reg	WINDOW	*winptr;
		WINDOW	lwin;

	if (win == NULL)
		return(NULL);
	if (win->_parent != NULL)
		return(NULL);

	if (nolines <= 0)
		return(win);

	omaxy = win->_maxy;
	ln = omaxy + nolines;
	cn = win->_maxx;

	winptr = &lwin;

	winptr->_maxy = ln;
	winptr->_maxx = cn;

	/*
	**  Allocate arrays used in window.
	*/
	if (IITD1waWinAlloc(winptr) != OK)
	{
		return(NULL);
	}

	/*
	**  Allocate memory for each character cell.
	*/
	if (IITD2waWinAlloc(winptr, omaxy) != OK)
	{
		IITD1dDelwin(winptr);
		return(NULL);
	}

	for (i = 0; i < omaxy; i++)
	{
		winptr->_firstch[i] = win->_firstch[i];
		winptr->_lastch[i] = win->_lastch[i];
		winptr->_y[i] = win->_y[i];
		winptr->_da[i] = win->_da[i];
		winptr->_dx[i] = win->_dx[i];
		winptr->_freeptr[i] = win->_freeptr[i];
	}

	/*
	**  Free up arrays for old window size.
	*/
	IITD1dDelwin(win);

	win->_firstch = winptr->_firstch;
	win->_lastch = winptr->_lastch;
	win->_y = winptr->_y;
	win->_da = winptr->_da;
	win->_dx = winptr->_dx;
	win->_freeptr = winptr->_freeptr;

	win->_maxy = ln;
	return(win);
}
