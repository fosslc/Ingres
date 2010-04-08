/*
**  FTrefresh
**
**  Copyright (c) 2004 Ingres Corporation
**
**  History:
**	04/04/90 (dkh) - Integrated MACWS changes.
**	08/29/90 (dkh) - Integrated porting change ingres6202p/131520.
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	"ftframe.h"
# include	<menu.h>


FTrefresh(frm, menuptr)
FRAME	*frm;
MENU	menuptr;		/* needed??? */
{
	TDtouchwin(FTwin);
	TDrefresh(FTwin);

# ifdef DATAVIEW
	_VOID_ IIMWrRefresh();
# endif	/* DATAVIEW */
}


/*
**  Move cursor to home position.  "Home" position in this case means
**  the top left hand corner of the screen.
*/

FThome()
{
	TDrstcur(curscr->_cury, curscr->_curx, 0, 0);
}
