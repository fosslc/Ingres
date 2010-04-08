/*
**  FTbell
**
**  Copyright (c) 2004 Ingres Corporation
**
**  ftbell.c
**
**  History:
**	01-feb-90 (bruceb)
**		Added call on TEflush().
**	04/03/90 (dkh) - Integrated MACWS changes.
**	05/22/90 (dkh) - Enabled MACWS changes.
**	08/29/90 (dkh) - Integrated porting change ingres6202p/131520.
**	16-jul-98 (kitch01)
**		Bug 92002. If MWS is running only send bell via MWS protocol. 
**      14-Mar-2006 (hanal04) Bug 115611
**              Always turn off line mode before writing BELL. Failure to do
**              so causes BELL to corrupt the display if we were still in
**              line mode.
**      21-Dec-2007 (hanal04) Bug 119648
**              Remove change to turn off line mode before issuing bell.
**              The bug caused bug 119648 and retesting of Bug 115611
**              no longer reproduced with the previous fix removed.
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include       <st.h>
# include	<te.h>
# include	"ftframe.h"

# ifndef BELL

# define	BELL		'\07'

# endif


VOID
FTbell()
{
# ifdef DATAVIEW
	if (IIMWimIsMws())
	{
		_VOID_ IIMWsbSoundBell();
		return;
	}
# endif	/* DATAVIEW */

	TEput(BELL);
	TEflush();
}
