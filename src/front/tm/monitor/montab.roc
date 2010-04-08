/*
**	Copyright (c) 2004 Ingres Corporation
*/
/*
**  MONTAB.ROC -- Controlwords and Syscntrlwds moved to (shared) text space.
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<er.h>
# include	"monitor.h"
# include	"montab.h"

/*
**  COMMAND TABLE
**	To add synonyms for commands, add entries to this table
**  History:
**	16-feb-1988 (neil) removed [no]echo and [no]semicolon.
**	20-apr-1989 (kathryn) added [no]continue, abbreviated [no]co,
**		with Controlwords C_CONTINUE,C_NOCONTINUE.
**		Added "\sc" as abbreviation for "\script".
**	17-Mar-1999 (wanfr01) added \suppress for suppressing query output
**		to get accurate qe90 results
**	11-Aug-2009 (maspa05) trac 397, SIR 122324
**		added commands associated with silent mode, namely
**              \silent, \nosilent, \titles, \notitles, \vdelim, \trim, \notrim
*/

GLOBALDEF struct cntrlwd	Controlwords[] =
{
	ERx("a"),		C_APPEND,
	ERx("append"),		C_APPEND,
	ERx("b"),		C_BRANCH,
	ERx("branch"),		C_BRANCH,
	ERx("bell"),		C_BEEP,
	ERx("nobell"),		C_NOBEEP,
	ERx("cd"),		C_CHDIR,
	ERx("chdir"),		C_CHDIR,
	ERx("e"),		C_EDIT,
	ERx("ed"),		C_EDIT,
	ERx("edit"),		C_EDIT,
	ERx("editor"),		C_EDIT,
	ERx("format"),		C_FORMAT,
	ERx("noformat"),	C_NOFORMAT,
	ERx("g"),		C_GO,
	ERx("go"),		C_GO,
	ERx("i"),		C_INCLUDE,
	ERx("include"),		C_INCLUDE,
	ERx("read"),		C_INCLUDE,
	ERx("k"),		C_MARK,
	ERx("mark"),		C_MARK,
	ERx("l"),		C_LIST,
	ERx("list"),		C_LIST,
	ERx("p"),		C_PRINT,
	ERx("print"),		C_PRINT,
	ERx("q"),		C_QUIT,
	ERx("quit"),		C_QUIT,
	ERx("r"),		C_RESET,
	ERx("reset"),		C_RESET,
	ERx("s"),		C_SHELL,
	ERx("sh"),		C_SHELL,
	ERx("shell"),		C_SHELL,
#ifdef CMS
	ERx("sub"),		C_SHELL,      /* subset mode in CMS */
	ERx("subset"),		C_SHELL,      /* subset mode in CMS */
#endif
	ERx("script"),		C_SCRIPT,
	ERx("sc"),		C_SCRIPT,
	ERx("suppress"),	C_SUPPRESS,
	ERx("nosuppress"),	C_NOSUPPRESS,
	ERx("t"),		C_TIME,
	ERx("time"),		C_TIME,
	ERx("date"),		C_TIME,
	ERx("v"),		C_EVAL,
	ERx("eval"),		C_EVAL,
	ERx("w"),		C_WRITE,
	ERx("write"),		C_WRITE,
#ifdef CMS
	ERx("x"),		C_EDIT,
	ERx("xedit"),		C_EDIT,
#endif
	ERx("macro"),		C_MACRO,
	ERx("nomacro"),		C_NOMACRO,
	ERx("quel"),		C_QUEL,
	ERx("sql"),		C_SQL,
	ERx("continue"),	C_CONTINUE,
	ERx("co"),		C_CONTINUE,
	ERx("nocontinue"),	C_NOCONTINUE,
	ERx("noco"),		C_NOCONTINUE,
	ERx("silent"),		C_SILENT,
	ERx("sil"),		C_SILENT,
	ERx("nosilent"),	C_NOSILENT,
	ERx("nosil"),		C_NOSILENT,
	ERx("vdelimiter"),	C_VDELIM,
	ERx("vdelim"),		C_VDELIM,
	ERx("titles"),		C_TITLES,
	ERx("notitles"),	C_NOTITLES,
	ERx("trim"),		C_NOPADDING,
	ERx("notrim"),		C_PADDING,
	ERx("nopadding"),	C_NOPADDING,
	ERx("padding"),		C_PADDING,
/* 
** Not part of product - these do not work.
**
	ERx("echo"),		C_ECHO,
	ERx("noecho"),		C_NOECHO,
	ERx("semicolon"),	C_SEMICOLON,
	ERx("nosemicolon"),	C_NOSEMICOLON,
**
*/
	0
};

GLOBALDEF struct cntrlwd	Syscntrlwds[] =
{
	0, 0,
	ERx("t"),		SC_TRACE,
	ERx("trace"),		SC_TRACE,
	ERx("r"),		SC_RESET,
	ERx("reset"),		SC_RESET,
	0
};
