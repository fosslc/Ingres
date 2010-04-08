# include 	<compat.h>
# include	<er.h>
# include	<si.h>
# include	<cm.h>
# include 	<eqscan.h>
# include 	<equel.h>
# include	<eqsym.h>	/* Because of eqgr.h */
# include	<eqgr.h>
# include	<eqlang.h>
# include	<ereq.h>

/*
+* Filename:	SCEQULEX.C
** Purpose:	Interface between grammar and scanner through YY defined 
**		routines and variables for YACC -- EQUEL version.
**
** Defines:	yyequlex()	   - Scanner called by YACC (yyparse by yylex).
** Locals:
**		sc_ishost()	   - What kind of a line is this?
** Uses:
**		yylval		   - Yacc stack element filled by scanner.
-*		yydebug		   - Yacc debug flag.
**
** History:	11-dec-1984 - Rewritten (mrw)
**		20-may-1987 - Modified for kanji/new scanner (mrw)
**		03-may-1988 - Modified to do raw/rare/cooked scanning (whc)
**              12-Dec-95 (fanra01)
**                  Modfied all externs to GLOBALREFs
** 
** Copyright (c) 2004 Ingres Corporation
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	4-oct-01 (inkdo01)
**	    define yyreswd for parser reserved word retry logic.
**	22-oct-01 (inkdo01)
**	    change GLOBALDEF to GLOBALREF for yyreswd (already GLOBALDEFed)
*/

/* really a union of different pointers and values */
GLOBALREF	GR_YYSTYPE	yylval;
GLOBALREF	bool		yyreswd;

# ifdef	YYDEBUG
    GLOBALREF i4  	yydebug;
# endif /* YYDEBUG */

/*
** These two variables are defined in scrdline.c
** Used to check if we have started to process the current line.
*/
GLOBALREF i4	lex_need_input;		/* Do we need to read in a new line? */
GLOBALREF i4	lex_is_newline;		/* Do we need to process the line? */


/*
+* Procedure:	yyequlex 
** Purpose:	Scanner for Yacc. Called by yyparse when it wants a token.
**
** Parameters:	
**		mode - SC_RAW, SC_RARE, or SC_COOKED for level of processing.
** Returns:	i4 - Token value
** 
** Side Effects:
-*  1. When scanning for a token, sc_addtok is called, which fills yylval.
*/

i4
yyequlex(mode)
i4  mode;
{
    register i4	lex_tok;
    i4			sc_ishost();
    void		inc_push_file();
    i4			inc_is_nested();
    bool		lex_isempty();

    lex_tok = SC_CONTINUE;
    while (lex_tok == SC_CONTINUE)
    {
	if (lex_need_input)
	{
	  /* Skip all irrelevant input lines */
	    do {
		sc_readline();
	    } while (lex_isempty(dml->dm_exec == DML_HOST));
	    lex_is_newline = TRUE;	/* We've read a new line */
	    lex_need_input = FALSE;	/* We don't need to read another */
	}

	if (lex_is_newline)
	{
	    /*
	    ** If we have a new unprocessed line then then check to see if we
	    ** have an include line.
	    ** If we don't have an include line, then check to see if we have
	    ** a host line. If not, then scan as ESQL code.
	    */

	    lex_is_newline = FALSE;
	    if (*SC_PTR == SC_EOFCH)
	    {
		i4 nestcode = inc_is_nested();

		/* if raw, assume we're not nested. */
		if (mode != SC_COOKED)
		    nestcode = SC_NOINC;

		switch (nestcode)      /* maybe end of an include file */
		{
		  /* 
		  ** EOF on an inline file is transparent; just pop and
		  ** continue.  If there are syntax errors in last statement,
		  ** file and line may be off.
		  */
		  case SC_INLINE:
		    inc_pop_file();
		    lex_need_input = TRUE;
		    continue;

		  /* 
		  ** EOF on non-inline files terminates statements.
		  ** Grammar will pop file.
		  */
		  case SC_INC:	
		    sc_addtok( SC_YYSTR, ERx("<End-Of-File>") );
		    lex_tok = tok_special[TOK_INCLUDE];
		    lex_need_input = TRUE;
		    continue;

		  /*
		  ** EOF on the main file.  We shouldn't get called again
		  ** after return SC_EOF.
		  */
		  case SC_NOINC:
		    lex_is_newline = TRUE;
		    sc_addtok( SC_YYSTR, ERx("<End-Of-File>") );
		    lex_tok = SC_EOF;		/* EOF to the grammar */
		    continue;
		} /* switch is-nested */
	    } /* if eof */

	  /* Remove and save (BASIC) line number from buffer */
	    if (dml->dm_scan_lnum)
		(*dml->dm_scan_lnum)();		/* Scan line number (BASIC) */
	    
	    if (mode != SC_RAW)
	    {
		/* if not raw, check for include files */

		switch (inc_tst_include())
		{
		  /*
		  ** Return the input buffer line
		  ** grammar will call inc_push_file - and go to next line
		  */
		  case SC_INC:
		    /* tell grammar to push */
		    sc_addtok( SC_YYSTR, ERx("include") );	
		    lex_tok = tok_special[TOK_INCLUDE];	/* success */
		    lex_need_input = TRUE;
		    continue;

		  /* 
		  ** Switch input file to new file; don't worry if it fails.
		  ** If there are syntax errors in last statement, file and 
		  ** line may be off.
		  */
		  case SC_INLINE:
		    inc_push_file();
		    lex_need_input = TRUE;
		    continue;			/* start on new file */

		  case SC_BADINC:
		    sc_addtok( SC_YYSTR, SC_PTR );
		    lex_tok = tok_special[TOK_CODE];
		    lex_need_input = TRUE;
		    continue;

		  case SC_NOINC:
		    break;
		}
	    } /* if not raw */

	  /* Is this a host line? */
	    switch (sc_ishost(mode))
	    {
	      case SC_HOST:
		dml->dm_exec = DML_HOST;
	      /* Return the input buffer line - and go to next line */
		sc_addtok( SC_YYSTR, SC_PTR );
		lex_tok = tok_special[TOK_CODE];   /* grammar will emit line */
		lex_need_input = TRUE;
		break;
	      case SC_DEBUG:	/* $_cmd -- pretend nothing ever happened. */
		lex_need_input = TRUE;
		continue;
	      case SC_EQUEL:
		dml->dm_exec = DML_QUEL;
		break;
	    }
	} else /* not lex_is_newline */
	{
	    if (CMnmstart(SC_PTR))
		lex_tok = sc_word( mode );
	    else if (CMdigit(SC_PTR))
		lex_tok = sc_number( mode );
	    else if (CMoper(SC_PTR))
		lex_tok = sc_operator( mode );
	    else if (mode != SC_COOKED)
	    {
		static char cbuf[3];
		char 	    *p = cbuf;

		/* in raw mode we create whitespace tokens. */
		if (*SC_PTR == '\n')
		    lex_need_input = TRUE;
		CMcpyinc( SC_PTR, p );
		*p = '\0';
		sc_addtok( SC_YYSTR, cbuf );
		lex_tok = tok_special[TOK_WHITESPACE];
	    }
	    else if (*SC_PTR == '\n')
		lex_need_input = TRUE;
	    else
		CMnext( SC_PTR );
	    if (*SC_PTR == SC_EOFCH)
		lex_is_newline = TRUE;
	}
    } /* while lex_tok == continue */
# ifdef YYDEBUG
    if (yydebug)
    {
	SIprintf( ERx("yylex(%s) = %d, yylval = '%s'\n"), 
		(mode == SC_COOKED ? ERx("cooked") : ERx("raw")), 
		lex_tok, yylval.s );
    }
# endif /* YYDEBUG */
    return lex_tok;		/* Hopefully Yacc can use it */
}

/*
+* Procedure:	sc_ishost 
** Purpose:	Return TRUE iff the current line is a host code line.
**
** Parameters:	mode -- cookedness.  we always return true if raw.
-* Returns:	i4  - SC_HOST/SC_EQUEL:  Host language code/Equel code '##'.
*/

i4
sc_ishost(mode)
i4  mode;
{
    /* debug hack: */
    if (*SC_PTR == '$' && *(SC_PTR+1) == '_')
    {
	SC_PTR += 2;
	sc_dbgcomment();
	return (SC_DEBUG);
    }

  /* SC_PTR pointing at first char of line - after lines have been skipped */
    if (*SC_PTR == '#' && *(SC_PTR+1) == '#')
    {
	SC_PTR += 2;
	return SC_EQUEL;
    }
    if (mode != SC_COOKED)
	return (SC_EQUEL);
    return SC_HOST;
}
