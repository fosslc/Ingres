/*
**	Copyright (c) 2004 Ingres Corporation
*/
# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<er.h>
# include	<erft.h>
# include	"ftframe.h"

/*
**  ftgtctl.c
**	called on entering or leaving a form with graphics.
**	disp = FALSE means leaving - clear graphics from screen.
**	disp = TRUE should be called from initialize block of form.
**
**  History:
**	08/14/87 (dkh) - ER changes.
**	10/19/89 (dkh) - Changed code to eliminate duplicate FT files
**			 in GT directory.  Essentially moved body
**			 of code to gt!gtedit.c
*/
VOID
FTgtctl (disp,iact)
bool disp;		/* graphics display on form */
bool iact;		/* interactive graphics form */
{
	if (iigtctlfunc)
	{
		(*iigtctlfunc)(disp, iact);
	}
	else
	{
		syserr (ERget(E_FT000A_FTgtctl_called_when_l));
	}
}
