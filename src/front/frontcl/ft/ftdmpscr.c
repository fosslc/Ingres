/*
**  FTdumpscr
**
**  Copyright (c) 2004 Ingres Corporation
**
**  ftdmpscr.c
**
**  History:
**	6-aug-87 (mgw) Bug # 12989
**		Call TDonewin() from FTwinbld(), not TDnewwin, so that we
**		can allocate enough space for a form that's larger than
**		the default. NOTE: I took this code from Dave H. from 6.0
**		code. His is more correct, but here, the TDdelwin()'s in
**		FTdumpscr() cause an AV in TDsubwin() if you go to re-edit
**		table field on the form after dumping it. So I'm leaving them
**		out here for 5.0, but this will result in a slow memory leak
**		after each printing of a form.
**	05-jan-89 (bruceb)
**		Additional parameter to FTdumpscr, FTwinbld and FTbuild
**		to indicate whether original caller was Printform.
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	"ftframe.h"
# include	<si.h>

GLOBALREF	WINDOW	*FTutilwin;
GLOBALREF	WINDOW	*FTcurwin;

static
WINDOW *
FTwinbld(frm, fromPrintform)
FRAME	*frm;
bool	fromPrintform;
{
	WINDOW	*win;

	if (TDonewin(&win, &FTutilwin, &FTcurwin, frm->frmaxy, frm->frmaxx)
		!= OK)
	{
		return(NULL);
	}
	FTbuild(frm, TRUE, win, fromPrintform);
	return(win);
}


FTdumpscr(frm, file, cursor, fromPrintform)
FRAME	*frm;
FILE	*file;
bool	cursor;
bool	fromPrintform;
{
	WINDOW	*win;
	WINDOW	*tutilwin;
	WINDOW	*tcurwin;

	if (frm == NULL)
	{
		win = curscr;
	}
	else
	{
		tutilwin = FTutilwin;
		tcurwin = FTcurwin;

		if ((win = FTwinbld(frm, fromPrintform)) == NULL)
		{
			return;
		}

		/*
		**  Fix up box graphic stuff since we are
		**  not going to do a refresh call.
		*/
		TDboxfix(win);
	}
	TDdumpwin(win, file, cursor);

	if (frm != NULL)
	{
		TDdelwin(win);
		FTutilwin->_flags |= _SUBWIN;
		FTcurwin->_flags |= _SUBWIN;
		TDdelwin(FTutilwin);
		TDdelwin(FTcurwin);
		FTutilwin = tutilwin;
		FTcurwin = tcurwin;
	}
}
