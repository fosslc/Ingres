# include	<compat.h>
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
**  Clear Input Line
**
**	This routine removes the newline following a monitor command
**	(such as \t, \g,  etc.)	 Any other characters are processed.
**	Hence, \t\g\t will work.  It also maintains the
**	Newline flag on command lines.	It will make certain that
**	the current line in the query buffer ends with a newline.
**
**	The flag 'noprompt' if will disable the prompt character if set.
**	Otherwise, it is automatically printed out.
**
**	Uses trace flag 8
**
** History:
**	19-mar-90 (teresal)
**		Removed 'Query buffer full' warning message - no longer needed
**		with dynamically allocated query buffer. (bugs 9489 9037)
**	aug-2009 (stephenb)
**		Prototyping front-ends
*/

FUNC_EXTERN i4 getch();

void
clrline(noprompt)
int	noprompt;
{
	register char	c;

	if (!Newline)
		q_putc(Qryiop, '\n');

	Newline = TRUE;

	/* if char following is a newline, throw it away */

	c = (char)getch();
	P_rompt = c == '\n';

	if (!P_rompt)
	{
		Peekch = c;
	}
	else
	{
		if (!noprompt)
			prompt((char *)NULL);
	}

	return;
}
