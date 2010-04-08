/*
**  Allocate space for FTwin and FTcurwin
**
**  Copyright (c) 2004 Ingres Corporation
**
**  onewin.c
**
**  History:
**	01-oct-87 (bab)
**		Code added for _starty structure member.
**	08/20/93 (dkh) - Fixing NT compile problems.  Changed _leave
**			 to lvcursor for the WINDOW structure.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/


# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<termdr.h>

WINDOW	*TDmakenew();
WINDOW	*TDnewwin();

STATUS
TDonewin(bigwin, smwin, utilwin, y, x)
WINDOW	**bigwin;
WINDOW	**smwin;
WINDOW	**utilwin;
i4	y;
i4	x;
{
	if ((*bigwin = TDnewwin(y, x, (i4) 0, (i4) 0)) == (WINDOW *) NULL)
	{
		return(FAIL);
	}

	(*bigwin)->lvcursor = TRUE;
	(*bigwin)->_clear = FALSE;
	(*bigwin)->_starty = IITDflu_firstlineused;

	if ((*smwin = TDmakenew(y, x, (i4) 0, (i4) 0)) == (WINDOW *) NULL)
	{
		return(FAIL);
	}

	(*smwin)->_flags |= _SUBWIN;
	(*smwin)->_parent = *bigwin;
	(*smwin)->_flags &= ~(_FULLWIN | _SCROLLWIN);

	if ((*utilwin = TDmakenew(y, x, (i4) 0, (i4) 0)) == (WINDOW *) NULL)
	{
		return(FAIL);
	}

	(*utilwin)->_flags |= _SUBWIN;
	(*utilwin)->_parent = *bigwin;
	(*utilwin)->_flags &= ~(_FULLWIN | _SCROLLWIN);

	return(OK);
}
