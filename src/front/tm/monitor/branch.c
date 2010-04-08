#ifndef NT_GENERIC
#include	<unistd.h>
#endif
# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<st.h>
# include	<er.h>
# include	"ermo.h"
# include	"monitor.h"

FUNC_EXTERN	i4	getch();
FUNC_EXTERN	i4	expr();
FUNC_EXTERN	char   *getfilenm();
FUNC_EXTERN	char   *macro();

/*
** Copyright (c) 2004 Ingres Corporation
**
**  BRANCH
**
**	The "filename" following the \b op must match the "filename"
**	which follows some \k command somewhere in this same file.
**	The input pointer is placed at that point if possible.	If
**	the label does not exist, an error is printed and the next
**	character read is an EOF.
**
**	Uses trace flag 16
**
**	History:
**		12/10/84 (lichtman)	-- added code to handle \nomacro command.
**					Temporarily turn on macro processing when
**					branching
**	aug-2009 (stephenb)
**		Prototyping front-ends
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
*/


void
branch()
{
	FUNC_EXTERN	i4	getch();
	register char	c;
	register i4	i;
	i4		savedomac;


	/* Temporarily turn on macro processing (if it is off) */
	savedomac = Do_macros;
	Do_macros = TRUE;

	/* see if conditional */

	while ((c = (char)getch()) > 0)
		if (c != ' ' && c != '\t')
			break;

	if (c != '?')
	{
		Peekch = c;
	}
	else
	{
		/* got a conditional; evaluate it */

		Oneline = TRUE;

		macinit(getch, (char **)0);

		i = expr();

		if (i <= 0)
		{
			/* no branch */

			getfilenm();

			/* Restore macro processing mode */
			Do_macros = savedomac;

			return;
		}
	}

	/* get the target label */

	if (branchto(getfilenm()) == 0)
		if (branchto(macro(ERx("{default}"))) == 0)
		{
			Peekch = -1;

			putprintf(ERget(F_MO0007_Cannot_branch_n));
		}

	/* Restore macro processing mode */
	Do_macros = savedomac;

	return;
}


int
branchto(label)
char	*label;
{
	char		target[100];
	register char	c;
	register i4	status;
	char		errbuf[ER_MAX_LEN];

	if (label == 0)
		return (0);

	STcopy(label, target);

	if (SIterminal(Input))
	{
		putprintf(ERget(F_MO0008_Cannot_branch_on_term));
		return (1);
	}
	else if (Idepth == 0)
	{
		putprintf(ERget(F_MO0009_no_branch_at_toplevel));
		return(1);
	}
	/* search for the label */

	SIclose(Input);

	if (status = SIopen(&Ifile_loc[Idepth], ERx("r"), &Input))
	{
		ERreport(status, errbuf);
		/* In branch: %s\n */
		ipanic(E_MO0041_1500100, errbuf);
	}

	while ((c = (char)getch()) > 0)
	{
		if (c != '\\')
			continue;

		if (getescape(0) != C_MARK)
			continue;

		if (!STcompare(getfilenm(), target))
			return (1);
	}

	return (0);
}
