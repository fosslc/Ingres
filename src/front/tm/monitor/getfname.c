# include	<compat.h>
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<er.h>
# include	"ermo.h"
# include	"monitor.h"

/*
** Copyright (c) 2004 Ingres Corporation
**
**  GET FILE NAME
**
**	This routine collects a file name up to a newline and returns a
**	pointer to it.	Keep in mind that it is stored in a static
**	buffer.
**
**	Uses trace flag 17
**
**	06/11/85 Change routine name from getfilenm to getfname
**	for CMS.  This is another name space problem. (gwb)
**	08/25/87 (scl) Do an SIflush to stdout before getting input
**	11-Sep-2009 (maspa05) trac397, SIR 122324
**		Change check for non-zero char so that we can read
**		e.g. utf8 chars
*/
char *
#ifdef CMS
getfname()
#else
getfilenm()
#endif
{
	static char	filename[81];
	i4     c; /* this should be an int, not char (scl) */
	register i4	i;
	register char	*p;

	FUNC_EXTERN i4	getch();

#ifdef MVS
	SIflush(stdout);
#endif

	Oneline = TRUE;
	macinit(getch, (char **)0);

	/* skip initial spaces */

	while ((c = macgetch()) == ' ' || c == '\t')
		;

	i = 0;

	for (p = filename; c != 0; )
	{
		if (i++ <= 80)
			*p++ = c;

		c = macgetch();
	}

	*p = '\0';
	STtrmwhite(filename);
	P_rompt = Newline = TRUE;

#	ifdef xMTR2
	if (tTf(17, 0))
		putprintf(ERget(F_MO0017_filename), filename);
#	endif

	Oneline = FALSE;
	Peekch	= '\0';

	return (filename);
}
