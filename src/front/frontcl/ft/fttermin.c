/*
**  FTtrminfo.c
**
**  Copyright (c) 2004 Ingres Corporation
**
**  Routine to get certain information about a
**  user's terminal.
**
**  History:
**	08/83 (dkh) - The epoch.
**	09/07/89 (dkh) - Changed to support 80/132 switching.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
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
# include	<ftdefs.h>

FUNC_EXTERN	VOID	IIFTgscGetScrSize();

VOID
FTterminfo(terminfo)
FT_TERMINFO	*terminfo;
{
	i4	lcols;
	i4	llines;

	IIFTgscGetScrSize(&lcols, &llines);
	terminfo->name = ttytype;
	terminfo->cols = lcols;
	terminfo->lines = llines;
}
