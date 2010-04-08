# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<er.h>
# include	"ermo.h"
# include	"monitor.h"

#ifdef SEP

# include <tc.h>

GLOBALREF   TCFILE *IITMincomm;
GLOBALREF   TCFILE *IITMoutcomm;

#endif /* SEP */

/*
** Copyright (c) 2004 Ingres Corporation
**
**  OUTPUT PROMPT CHARACTER
**
**	The prompt is output to the standard output.  It will not be
**	output if -s mode is set or if we are not at a newline.
**
**	The parameter is printed out if non-zero.
**
**      10-may-89 (Eduardo)
**              Added interface to SEP tool
**	aug-2009 (stephenb)
**		Prototyping front-ends
*/
GLOBALDEF	char	*beep = ERx("");

prompt(msg)
char	*msg;
{
	if (P_rompt && (Peekch != -1) && (Nodayfile >= 0))
	{
		if (msg)
			putprintf(ERx("%s%s\n"), beep, msg);

		/*
		** May not use putprintf since we don't want a newline.
		*/

#ifdef SEP
		if (IITMoutcomm != NULL)
		{
		    TCputc('*',IITMoutcomm);
		    TCputc(' ',IITMoutcomm);
		}
		else
		{
#endif /* SEP */
		    SIprintf(ERx("* "));
		    SIflush(stdout);
#ifdef SEP
		}
#endif /* SEP */

	}
}


/*
**  PROMPT WITH CONTINUE OR GO
*/

void
cgprompt()
{
	prompt(Notnull ? ERget(F_MO0019_continue) : ERget(F_MO0029_go));
}

/*
**	fix the question "where's the beep"
*/
put_beep()
{
#ifndef FT3270
	beep = ERx("\007");
#endif
}

take_beep()
{
	beep = ERx("");
}
