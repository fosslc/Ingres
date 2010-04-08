/*
**  FTscroll.c
**
**  Copyright (c) 2004 Ingres Corporation
**
**  static	char	*Sccsid = "%W%	%G%";
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	 <termdr.h> 

bool
FTscrolled()
{
	if (curscr->_begy < 0)
	{
		return(TRUE);
	}
	return(FALSE);
}
