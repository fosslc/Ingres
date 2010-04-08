# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	 <termdr.h> 

/*
**  Make it look like the whole window has been changed.
**
**
**  Copyright (c) 2004 Ingres Corporation
**
**  static	char	*Sccsid = "%W%	%G%";
*/


TDtouchwin(win)
reg	WINDOW	*win;
{
	reg	i4	y;
	reg	i4	maxy;
	reg	i4	maxx;

	maxy = win->_maxy;
	maxx = win->_maxx - 1;
	for (y = 0; y < maxy; y++)
	{
		win->_firstch[y] = 0;
		win->_lastch[y] = maxx;
	}
}
