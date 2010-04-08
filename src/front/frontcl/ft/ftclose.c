/*
**  FTclose
**
**  Copyright (c) 2004 Ingres Corporation
**
**  History:
**	04/03/90 (dkh) - Integrated MACWS changes.
**	05/22/90 (dkh) - Enabled MACWS changes.
**	08/29/90 (dkh) - Integrated porting change ingres6202p/131520.
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<te.h>
# include	"ftframe.h"

i4
FTclose(msg)
char	*msg;
{
	if (msg)
	{
		TDwinmsg(msg);
	}

# ifdef DATAVIEW
	_VOID_ IIMWcClose(msg);
# endif	/* DATAVIEW */

	TDendwin();
}
