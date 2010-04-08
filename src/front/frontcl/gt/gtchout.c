/*
**	Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<er.h>
#include <te.h>

/* function for termcap call.  TEput is a macro!  TE.h conflicts with gks.h */
i4
GTchout(c)
char c;
{
	TEput (c);
	return (0);
}

# ifdef DGC_AOS
/* Cover for TElock.  Keep track of levels of locking */
static i4  lock_level = 0;

VOID GTlock(lock, row, col)
bool lock;
i4  row;
i4  col;
{
	if (lock)
	{
		lock_level++;
		if (lock_level == 1)
			TElock(lock, row, col);
	}
	else
	{
		if (lock_level > 0)
		{
			lock_level--;
			if (lock_level == 0)
				TElock(lock, row, col);
		}
	}
}
# endif
