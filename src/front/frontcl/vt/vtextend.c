/*
**  vtextend.c
**
**  Copyright (c) 2004 Ingres Corporation
**
**  History:
**	08/14/87 (dkh) - ER changes.
**	01-oct-87 (bab)
**		Allow for operation when running with unusable top line(s).
**	08/20/93 (dkh) - Fixing NT compile problems.  Changed _leave
**			 to lvcursor for the WINDOW structure.
**	01-oct-96 (mcgem01)
**		unLast and unImage changed from extern to GLOBALREF for
**		global data project.
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
# include	<fe.h>
# include	"vtframe.h"
# include	<er.h>

GLOBALREF	WINDOW	*unLast;
GLOBALREF	WINDOW	*unImage;
FUNC_EXTERN	WINDOW	*TDextend();

VOID
VTextend(frm, size)
FRAME	*frm;
i4	size;
{
	if ((frm->frscr = TDextend(frm->frscr, size)) == NULL)
	{
		IIUGbmaBadMemoryAllocation(ERx("VTextend"));
	}
	if ((frm->untwo = TDextend(frm->untwo, size)) == NULL)
	{
		IIUGbmaBadMemoryAllocation(ERx("VTextend"));
	}
	TDoverwrite(unLast, frm->untwo);

	if ((unLast = TDextend(unLast, size)) == NULL)
	{
		IIUGbmaBadMemoryAllocation(ERx("VTextend"));
	}
	if ((unImage = TDextend(unImage, size)) == NULL)
	{
		IIUGbmaBadMemoryAllocation(ERx("VTextend"));
	}
	TDoverwrite(frm->untwo, unLast);
}

VOID
VTwider(frm, numcols)
FRAME	*frm;
i4	numcols;
{
	VTwide1(frm, numcols);
	VTwide2(frm, numcols);
}

VOID
VTwide1(frm, numcols)
FRAME	*frm;
i4	numcols;
{
	WINDOW	*win;
	WINDOW	*uwin;
	i4	width;

	width = frm->frmaxx + numcols;
	if ((win = TDnewwin(frm->frmaxy, width, (i4)0, (i4)0)) == NULL)
	{
		IIUGbmaBadMemoryAllocation(ERx("VTwide1"));
	}
	uwin = frm->frscr;
	TDoverwrite(uwin, win);
	win->lvcursor = FALSE;
	win->_clear = FALSE;
	win->_cury = uwin->_cury;
	win->_curx = uwin->_curx;
	win->_starty = uwin->_starty;
	TDdelwin(uwin);
	frm->frscr = win;
}

VOID
VTwide2(frm, numcols)
FRAME	*frm;
i4	numcols;
{
	WINDOW	*win;
	WINDOW	*uwin;
	i4	width;

	width = frm->frmaxx + numcols;
	if ((uwin = TDnewwin(frm->frmaxy, width, (i4)0, (i4)0)) == NULL)
	{
		IIUGbmaBadMemoryAllocation(ERx("VTwide2"));
	}
	uwin->lvcursor = TRUE;
	TDoverwrite(unLast, uwin);
	uwin->_starty = unLast->_starty;

	if ((win = TDnewwin(frm->frmaxy, width, (i4)0, (i4)0)) == NULL)
	{
		IIUGbmaBadMemoryAllocation(ERx("VTwide2"));
	}
	TDdelwin(frm->untwo);
	TDdelwin(unLast);
	unLast = win;
	frm->untwo = uwin;
	TDoverwrite(frm->untwo, unLast);
	unLast->_starty = uwin->_starty;

	if ((uwin = TDnewwin(frm->frmaxy, width, (i4)0, (i4)0)) == NULL)
	{
		IIUGbmaBadMemoryAllocation(ERx("VTwide2"));
	}
	TDoverwrite(unImage, uwin);
	uwin->_starty = unImage->_starty;
	TDdelwin(unImage);
	unImage = uwin;
}
