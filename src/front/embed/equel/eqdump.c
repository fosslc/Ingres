# include	<compat.h>
# include	<er.h>
# include	<si.h>
# include	<equel.h>
# include	<ereq.h>

/*
+* Filename:	eqdump.c
** 
** Procedure:	eq_dump
** Purpose:	Dump the global eq structure for debugging.
**
** Parameters:	None
-* Returns:	None
**
** History:
**		26-jan-1985	 - Written (mrw)
**	16-dec-1992 (larrym)
**		Updated with more flags.  Changed fmask from i4  to i4,
**		since there are so many flags now.
**
** Copyright (c) 2004 Ingres Corporation
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

void
eq_dump()
{
    register FILE
		*dp = eq->eq_dumpfile;
    static struct flgs {
	i4	fmask;
	char	*fname;
    } fnames[] = {
	{ EQ_IIDEBUG, 	ERx("iidebug") },
	{ EQ_COMPATLIB,	ERx("compatlib") },
	{ EQ_STDIN,	ERx("stdin") },
	{ EQ_STDOUT,	ERx("stdout") },
	{ EQ_LIST,	ERx("list") },
	{ EQ_LSTOUT,	ERx("lstout") },
	{ EQ_SETLINE,	ERx("setline") },
	{ EQ_2POSL,	ERx("2passosl") },
	{ EQ_FMTANSI,	ERx("fmtansi") },
	{ EQ_NOREPEAT,  ERx("norepeat") },
	{ EQ_QRYTST,	ERx("qrytest") },
	{ EQ_INCNOWRT,	ERx("inc_nowrite") },
	{ EQ_VERSWARN,	ERx("versionwarn") },
	{ EQ_INDECL,	ERx("SQL declare mode") },
	{ EQ_COMMON,	ERx("OpenSQL warn") },
	{ EQ_FIPS,	ERx("Entry-level FIPS warn") },
	{ EQ_SQLSTATE,	ERx("SQLSTATE available") },
	{ EQ_SQLCODE,	ERx("SQLCODE available") },
	{ EQ_NOSQLCODE,	ERx("SQLCODE ignored") },
	{ 0,		(char *)0 }
    };
    register struct flgs *f;
    i4			flag_count;

    SIfprintf( dp, ERx("EQ_DUMP:\n") );
    SIfprintf( dp, ERx(" dml: dml_lang = %d, dml_exec = 0%o\n"),
	      dml->dm_lang, dml->dm_exec );
    SIfprintf( dp, ERx("      dml_left = %d, dml_right = %d\n"),
	      dml->dm_left, dml->dm_right );
    SIfprintf( dp, ERx(" eq: lang = %d, line = %d,\n"), eq->eq_lang, eq->eq_line );
    SIfprintf( dp, ERx("     filename = '%s', ext = '%s', outname = '%s',\n"), 
	      eq->eq_filename, eq->eq_ext, eq->eq_outname );
    SIfprintf( dp, ERx("     in_ext = '%s', out_ext = '%s'\n"), 
	      eq->eq_in_ext, eq->eq_out_ext );
    SIfprintf( dp, ERx("     def_in = '%s', def_out = '%s'\n"), 
	      eq->eq_def_in, eq->eq_def_out );
    SIfprintf( dp, ERx("     sql_quote = <%c>, host_quote = <%c>, "),
	      eq->eq_sql_quote, eq->eq_host_quote );
    SIfprintf( dp, ERx("quote_esc = <%c>\n"), eq->eq_quote_esc );
    SIfprintf( dp, ERx("     eq_flags = ") ); 
    flag_count = 0;
    for (f = fnames; f->fname; f++)
    {
	if (eq->eq_flags & f->fmask)
	    SIfprintf( dp, ERx("%s%s"), flag_count++ ? ERx(", ") : ERx(""), f->fname );
    }
    SIfprintf( dp, ERx("\n\n") );
    SIflush( dp );
}
