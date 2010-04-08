/*
**  FTclear
**
**  Copyright (c) 2004 Ingres Corporation
**
**  static	char	*Sccsid = "%W%	%G%";
**
**  History:
**	10/27/88 (dkh) - Performance changes.
**      23-sep-96 (mcgem01)
**              Global data moved to ftdata.c
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<termdr.h> 


GLOBALREF	bool	IIFTscScreenCleared;

VOID
FTclear()
{
	TDclear(curscr);

	IIFTscScreenCleared = TRUE;

	/*
	**  TDclear already calls TDrefresh.
	TDrefresh(curscr);
	*/
}
