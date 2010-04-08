# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<termdr.h>

/*
**  This routine erases everything on the window.
**
**  Copyright (c) 2004 Ingres Corporation
**
**  CLRTOBOT.c
**
**  TDclrbot -- This routine erases everything on the window.
**
**  History:
**	mm/dd/yy (RTI) -- created for 5.0.
**	11/04/86 (KY)  -- added to erase attributes on the window.
**	21-apr-87 (bab)
**		allow for erasing of reversed (right-to-left) fields.
**	28-Mar-2005 (lakvi01)
**		Corrected function prototypes.
*/

VOID
TDclrbot(win, reverse)
reg	WINDOW	*win;
bool		reverse;
{
	if (reverse)
		TDrclreol(win);
	else
		TDclreol(win);

	win->_curx = 0;
	/*
	** modify the actual win-struct members since TDclreol uses
	** _curx and _cury values to do its work
	*/
	for (win->_cury++; win->_cury < win->_maxy; win->_cury++)
	{
		TDclreol(win);
	}
	win->_cury = 0;

	if (reverse)
		win->_curx = win->_maxx - 1;
	/* else, _curx = 0; already done before the for-loop */
}
