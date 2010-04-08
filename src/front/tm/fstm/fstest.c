/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<lo.h>
# include	<si.h>
# include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>

/**
** Name:	fstest.c -	Testing routines for fstm
**
** Description:
**
**	Copied directly, with routine and variable names changed, from
**	mqtest.c (qbf).
**
** History:
**	Revision 4.0  85/04  stevel
**	Initial revision.
**	25-jan-90 (teresal)
**		Modified to use location buffer for LOfroms call.
**/

static FILE	*fs_ofile = NULL;
static bool	keydmp = FALSE;

VOID
fstest ( dump, keystroke )
char	*dump;
char	*keystroke;
{
	if ( keystroke != NULL && *keystroke != EOS )
	{
		FDrcvalloc( keystroke );
		keydmp = TRUE;
	}
	if ( dump != NULL && *dump != EOS )
	{
		LOCATION	loc;
		char		locbuf[MAX_LOC +1];

		STcopy(dump, locbuf);
		if ( LOfroms(FILENAME & PATH, locbuf, &loc) != OK ||
				SIopen(&loc, ERx("w"), &fs_ofile) != OK )
		{
			fs_ofile = NULL;
		}
		else
		{
			FDdmpinit(fs_ofile);
		}
	}
}

VOID
fstestexit()
{
	if ( keydmp )
		FDrcvclose(FALSE);
	if ( fs_ofile != NULL )
		SIclose(fs_ofile);
}
