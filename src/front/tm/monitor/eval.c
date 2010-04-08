# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	"monitor.h"
# include	<er.h>
# include	"ermo.h"
# include	<ex.h>

/*
** Copyright (c) 2004 Ingres Corporation
**
**  DO MACRO EVALUATION OF QUERY BUFFER
**
**	The logical query buffer is read and passed through the macro
**	processor.  The main purpose of this is to evaluate {define}'s.
**	If the 'pr' flag is set, the result is printed on the terminal,
**	and so becomes a post-evaluation version of print.
**
**	Uses trace flag 12
**
**	9/10/80 (djf)	changed unlink() to remove() and link() to _rename().
**	1/16/85 (lichtman) -- added exception handler to close query buffer on
**		interrupt (fix of bug #4738).
**      19-mar-90 (teresal)
**              Removed 'Query buffer full' warning message - no longer needed
**              with dynamically allocated query buffer. (bugs 9489 9037)
**	24-feb-92 (kevinq) OS/2
**		Corrected references to EX functions..
**	26-Aug-2009 (kschendel) b121804
**	    Remove function defns now in headers.
*/

static struct qbuf	*tfile ZERO_FILL;
static STATUS		eval_handler();

eval(pr)
i4	pr;
{
	EX_CONTEXT		ex_context;
	register i4		c;
	extern i4		Do_macros;
	i4			old_macros;
	char			buf[2];
	FUNC_EXTERN i4		q_getc();

	buf[1] = EOS;

	if(EXdeclare(eval_handler, &ex_context))
		ipanic(E_MO0049_1500500);

	old_macros = Do_macros; /* save macro state. Turn on temporarily */
	Do_macros = TRUE;
	Autoclear = 0;

	clrline(1);

	/* open temp file and reopen query buffer for reading */

	if (!pr)
	{
		if ((tfile = q_open('w')) == NULL)
		{
			EXdelete();
			/* eval: q_open */
			ipanic(E_MO0049_1500500);
		}
	}

	if (q_ropen(Qryiop, 'r') == (struct qbuf *)NULL)
	{
		EXdelete();
		/* eval: q_ropen 1 */
		ipanic(E_MO004A_1500501);
	}

	/* COPY FILE */

	macinit(q_getc, (char **)Qryiop);

	while ((c = macgetch()) > 0)
	{
		if (pr)
		{
			buf[0] = c;
			putprintf(buf);
		}
		else
			if (q_putc(tfile, (char) c))
			{
				EXdelete();
				Do_macros = old_macros;
				return;
			}
	}

	if (pr)
	{
		if (q_ropen(Qryiop, 'w') == NULL)
		{
			EXdelete();
			/* eval: q_ropen 2 */
			ipanic(E_MO004C_1500503);
		}
	}
	else
	{
		q_close(Qryiop);
		Qryiop = tfile;
	}

	cgprompt();

	Do_macros = old_macros;
	EXdelete();
}

static STATUS
eval_handler(exargs)
EX_ARGS *exargs;
{
	/* close query buffer on interrupt */

	if (EXmatch(exargs->exarg_num, 1, EXINTR) == 1)
		q_close(tfile);

	return (EXRESIGNAL);
}
