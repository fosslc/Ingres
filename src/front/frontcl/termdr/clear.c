# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<me.h>
# include	<fe.h>
# include	<cm.h>
# include	<termdr.h>

/*
**  This routine clears the window.
**
**  Copyright (c) 2004 Ingres Corporation
**
**  History:
**	08/14/87 (dkh) - ER changes.
**	10/28/88 (dkh) - Performance changes.
**	02/21/89 (dkh) - Added include of cm.h and cleaned up ifdefs and
**			 fixed typo for IITDbgGetBegin() to IITDgbGetBegin().
**	31-may-90 (rog)
**		Included <me.h> so MEfill() is defined correctly.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

FUNC_EXTERN	i4	TDrefresh();

GLOBALREF	bool	IITDfcFromClear;
GLOBALREF	i4	IITDAllocSize;

char *
IITDgbGetBegin(cpp, num)
char	**cpp;
i4	num;
{
	i4	i;
	char	*cp;

	cp = *cpp;

	for (i = 1, cpp++; i < num; i++, cpp++)
	{
		if (cp > *cpp)
		{
			cp = *cpp;
		}
	}

	return(cp);
}


i4
TDclear(win)
reg	WINDOW	*win;
{
	i4	retval = OK;
	i4	size;
	char	*cp;

	if (win == curscr)
	{
		IITDfcFromClear = TRUE;
		/*
		**  Do a quick erase of "curscr" since
		**  we know its one chunk of memory and
		**  probably less than an u_i2.
		*/
		size = IITDAllocSize * LINES;
		cp = IITDgbGetBegin(win->_y, win->_maxy);
		MEfill((u_i2) size, ' ', cp);
		cp = IITDgbGetBegin(win->_da, win->_maxy);
		MEfill((u_i2) size, EOS, cp);
# ifdef DOUBLEBYTE
		cp = IITDgbGetBegin(win->_dx, win->_maxy);
		MEfill((u_i2) size, EOS, cp);
# endif /* DOUBLEBYTE */
		TDtouchwin(win);

		win->_begy = win->_begx = win->_cury = win->_curx = 0;
	}
	else
	{
		TDerase(win);
	}
	retval = TDrefresh(win);

	if (win != curscr)
	{
		win->_clear = TRUE;
	}

	IITDfcFromClear = FALSE;

	return(retval);
}
