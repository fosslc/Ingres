/*
** Copyright (c) 2004 Ingres Corporation
** All Rights Reserved.
*/

# include 	<compat.h>
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<si.h>
# include	<cm.h>
# include	<er.h>
# include 	<eqscan.h>
# include 	<eqscsql.h>
# include 	<equel.h>
# include	<eqlang.h>
# include	<eqsym.h>	/* Because of eqgr.h */
# include	<eqgr.h>
# include	<eqgen.h>
# include	<ereq.h>

/*
NO_OPTIM =      dg8_us5
*/

/*
+* Filename:	esqyylex.c
** Purpose:	Interface between grammar and scanner through YY defined 
**		routines and variables for YACC -- ESQL version.
**
** Defines:	yyesqlex()	   - Scanner called by YACC (yyparse by yylex).
**		sc_moresql()       - Called by slave grammar to indicate
**				     we just finished parsing an SQL statement,
**				     but we need to check for SQL comments.
**		sc_execstmt	   - Scan for EXEC SQL|FRS|4GL
** Locals:
**		sc_esqishost( mode ) - What kind of a line is this?
** Uses:
**		yylval		   - Yacc stack element filled by scanner.
-*		yydebug		   - Yacc debug flag.
**
** History:	11-Dec-1984	   - Rewritten (mrw)
** 		20-Aug-1985	   - Modified to allow calling from ESQL (mrw)
** 		19-May-1986	   - Modified to fake FORTRAN terminators (bjb)
**		19-May-1987	   - Modified for Kanji/new scanner design (mrw)
**		29-jun-87 (bab)	Code cleanup.
**		03-May-1988	   - Scan in raw/rare/cooked mode (whc)
**		02-aug-1989	   - Flag FRS statements for FIPS-compat. (bjb)
**		29-Aug-1989	   - Modified for COBOL sequence no. (teresal)	
**              29-may-1992 (kevinm) B43426
**                      Check to see if *SC_PTR is not equal to SC_EOFCH in a
**                      do/while loop for call sc_readline.
**		28-jul-1992	   - Added global sc_hostvar; this is set
**				     by the slave grammer when it sees a 
**				     host variable reference.  sc_word was 
**				     modified to check for this global and 
**				     to supress keyword lookups. (larrym)
**	02-oct-1992 (larrym)
**		Added new global, sc_saveptr.  If we're currently scanning a 
**		host variable reference (i.e., not doing keyword lookups), 
**		then the current SC_PTR is saved here before calling sc_word.  
**		If the slave grammer determines that the last word should have 
**		been scanned with keyword lookups enabled, then it will 
**		enable them, and call sc_popscptr, which I have also just 
**		added.  sc_popscptr will move SC_PTR back to the previously
**		saved address.  This will allow the word to be rescanned with
**		keyword lookups on.  For more details on this, look in any
**		slave grammer file under HOST VARIABLE USAGE.
**	20-nov-1992 (Mike S)
**		Check for EXEC 4GL as well as EXEC SQL and EXEC FRS
**	09-feb-1993 (teresal)
**		Add flexible placement of SQL statements within a host 
**		language. You can now mix SQL statements and host
**		language statements on a single line. This support
**		is only for ESQL/ADA and ESQL/C.
**	10-jun-1993 (kathryn)
**		Add flexible placement of SQL stmts for ESQL/FORTRAN for FIPS.
**		sc_scanhost() will now return TRUE in Fortran, when an SQL stmt
**	        appears on the same line as a Fortran "logical if" stmt.
**		EX:  if (x .eq. 0) exec sql commit  (only place allowed).
**		Since only one executable stmt is allowed in a Fortran 
**		"logical If" stmt, and we generate multiple stmts from one
**		SQL stmt, we must generate a "block if" fortran stmt. 
**		When sc_scanhost returns TRUE, add keyword "then" to the
**	        sc_hostbuf. eq_flags will be set to EQ_ENDIF so that at
**		the end of processing the SQL stmt we will generate "end if".
**		Added new routine sc_execstmt() to check for embedded stmts.
**	21-jun-1993 (lan)
**		Fixed a NIST bug where COBOL identifiers that begin with
**		digits were not allowed.
**	26-jul-1993 (lan)
**		Removed a check where an error was generated for EXEC 4GL
**		in non-C languages.  EXEC 4GL is now supported in all the
**		other languages.
**	22-Jun-1995 (walro03/mosjo01)
**		Added NO_OPTIM dg8_us5.
**		Symptom: loop in esql.
**		Works in DG/UX 3.00 MU05; fails in DG/UX 3.10 MU02.
**      12-Dec-95 (fanra01)
**              Added definitions for extracting data for DLLs on windows NT
**      06-Feb-96 (fanra01)
**              Made data extraction generic.
**	15-mar-1996 (thoda04)
**		Added include eqgen.h for prototype for out_emit().
**		Corrected parameter type for local sc_esqishost().
**		Added function parm prototype for sc_numname().
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	4-oct-01 (inkdo01)
**	    define yyreswd for parser reserved word retry logic.
**	23-oct-2001 (somsa01)
**	    Moved define of yyreswd to eqdata.c.
**	07-Nov-05 (toumi01) SIR 115505
**	    Support hex constants in format 0x696E67636F7270 as well as the
**	    traditional X'6279656279654341' to aid application portability.
**	24-Mar-2007 (toumi01)
**	    Add support for multi-threaded Ada.
**	27-Apr-2007 (toumi01)
**	    Logically delete TASK BODY Ada scanning for now.
*/ 

/* sc_esqishost wants to stop on newlines */
# define	yy_isspace(c)	(CMwhite(c) && (*(c) != '\n'))

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
** Used to determine if the last statement parsed was an SQL statement.
** It is set in the slave grammar when a SQL Terminator is found. This
** will only be set for languages that support flexible placement of
** host language statements like ESQL Ada and C.
*/
static i4	lex_more_sql;		/* Do we need to parse for SQL commts */

/* Buffer to store a partial host line */
static char	sc_hostbuf[SC_LINEMAX + 3] ZERO_FILL;

/* This variable is defined in eqglob.c.  It is used to tell scword that we
** are expecting a colon and not to do keyword lookups.
*/
GLOBALREF i4	sc_hostvar;		/* FALSE means do keyword lookup */

/*
** If we're scanning a host variable reference (sc_hostvar = TRUE) then we
** save the current SC_PTR here before calling sc_word.  See history for 
** details
*/
GLOBALREF char	*sc_saveptr;  /* save SC_PTR before scanning word */

/*
+* Procedure:	yyesqlex 
** Purpose:	Scanner for Yacc. Called by yyparse when it wants a token.
**
** Parameters:	mode - SC_RAW, SC_RARE, or SC_COOKED to indicate level of 
**			processing.
** Returns:	i4 - Token value
** 
** Side Effects:
-*  1. When scanning for a token, sc_addtok is called, which fills yylval.
**
*/

i4
yyesqlex(mode)
i4  mode;
{
    register i4	lex_tok;
    char		*cp;
    static bool		was_term;
    i4			len;
    i4  		sc_esqishost(i4 mode);
    void		inc_push_file();
    i4			inc_is_nested();
    bool		lex_isempty();
    bool		sc_numname(void);

    lex_tok = SC_CONTINUE;
    while (lex_tok == SC_CONTINUE)
    {
	if (lex_need_input)
	{
	  /* Skip all irrelevant input lines */
	    do {
		sc_readline();
	    } while ( (*SC_PTR != SC_EOFCH) &&
			lex_isempty(dml->dm_exec == DML_HOST));
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
	    lex_more_sql = FALSE;
	    if (*SC_PTR == SC_EOFCH)
	    {
		i4 nestcode = inc_is_nested();

		/* assume not in include if raw */
		if (mode != SC_COOKED)
		    nestcode = SC_NOINC;

		switch (nestcode)      /* maybe end of an include file */
		{
		  /* 
		  ** EOF on an inline file is transparent; just pop and
		  ** continue.  If there are syntax errors in last statement,
		  ** file and line may be off.
		  */
		  case SC_INLINE:	/* can't happen for SQL - may for ADA */
		    inc_pop_file();
		    lex_need_input = TRUE;
		    continue;

		  /* 
		  ** EOF on non-inline files terminates statements. We pop
		  ** files here because syntax errors on EOF will go looking
		  ** for a host terminator; if we didn't pop here, but had
		  ** a syntax error on the EOF then we would loop looking
		  ** for a terminator but getting only EOF.
		  ** On normal include EOFs any previous host code or
		  ** terminators will have already been reduced.
		  **
		  ** FORTRAN and BASIC pretend there was an SQL terminator
		  ** at the end of an EXEC SQL statement for the upper
		  ** grammar.  On the next call to the scanner we'll come
		  ** back here, but we'll no longer be in EXEC mode.  However,
		  ** just to be safe, we check that the last token returned
		  ** was not a terminator.
		  **
		  ** Emit a newline in case the included file ends in a
		  ** display loop w/o a finalize; this ensures that the last
		  ** ("else") line is properly terminated.
		  */
		  case SC_INC:	
		    if (  (dml->dm_exec & DML_EXEC) 
			&&(eq_islang(EQ_FORTRAN) || eq_islang(EQ_BASIC))
			&&(!was_term))
		    {
			lex_is_newline = TRUE;
			sc_addtok( SC_YYSTR, ERx(";") );
			lex_tok = tok_special[TOK_TERMINATE];
			/* dml->dm_exec = DML_HOST;   Ensure not in EXEC mode */
			continue;
		    }
		    out_emit( ERx("\n") );
		    inc_pop_file();
		    sc_addtok( SC_YYSTR, ERx("<End-Of-File>") );
		    lex_tok = tok_special[TOK_EOFINC];
		    lex_need_input = TRUE;
		    continue;

		  /*
		  ** EOF on the main file.  We shouldn't get called again
		  ** after return SC_EOF.
		  */
		  case SC_NOINC:
		    if (  (dml->dm_exec & DML_EXEC) 
			&&(eq_islang(EQ_FORTRAN) || eq_islang(EQ_BASIC))
			&&(!was_term))
		    {
			lex_is_newline = TRUE;
			sc_addtok( SC_YYSTR, ERx(";") );
			lex_tok = tok_special[TOK_TERMINATE];
			/* dml->dm_exec = DML_HOST;   Ensure not in EXEC mode */
			continue;
		    }
		    lex_is_newline = TRUE;
		    sc_addtok( SC_YYSTR, ERx("<End-Of-File>") );
		    lex_tok = SC_EOF;		/* EOF to the grammar */
		    continue;
		} /* switch is-nested */
	    } /* if eof */
	    
	  /* Is this a host line? */
	    switch (sc_esqishost(mode))
	    {
	      case SC_SQL:
		dml->dm_exec = DML_EXEC | DML__SQL;
		break;
	      case SC_FRS:
		dml->dm_exec = DML_EXEC | DML__FRS;
		break;
	      case SC_4GL:
		dml->dm_exec = DML_EXEC | DML__4GL;
		break;
	      case SC_HOST:
	      /* Return the input buffer line - and go to next line */
	      /*
	      ** For Languages that allow multiple statements on one
	      **   line separated by terminators, like C and ADA,
	      **   don't just put out the whole line, but look for a 
	      **   terminator. sc_scanhost will return true if 
	      **   it looks like there might be a line with stuff on
	      **   it after the terminator. 
	      ** For Fortran sc_scanhost will return true when we have found an
	      **   SQL stmt on the same line as a Fortran host "IF" stmt.
	      **   (The only time Fortran allows two stmts on one line, is for
	      **   the IF stmt.) Since SQL stmts generate more than one Fortran
	      **   stmt, we must change the IF stmt to a fortran IF BLOCK by 
	      **   adding "THEN" to the host language IF stmt token.
	      **   sc_scanhost has set a flag so that after the SQL stmt is
	      **   pre-processed, an "END IF" will be generated by the grammar.
	      ** Note: Other languages always return FALSE.
	      **
	      */
		cp = SC_PTR;
		if (sc_scanhost(TRUE))
		{
		    lex_is_newline = TRUE;
		    /* sc_scanhost will reset SC_PTR to point to newline */
		    /* Only return a partial line */
		    len = CMcopy(cp, SC_PTR-cp, sc_hostbuf);
		    if (eq_islang (EQ_FORTRAN))
		    {
			STcopy(ERx("then"), &(sc_hostbuf[len]));
			len += 4;
		    }
		    sc_hostbuf[len] = '\0';
		    /* Put in hook here to put out /nl if code generated? */
		    cp = sc_hostbuf;
		}
		else
		{
		    lex_need_input = TRUE;
		}
		sc_addtok( SC_YYSTR, cp );
		lex_tok = tok_special[TOK_CODE];   /* grammar will emit line */
		dml->dm_exec = DML_HOST;
		break;
	      case SC_CONT:
		continue;
	      case SC_DEBUG:	/* $_cmd : pretend nothing ever happened */
		lex_need_input = TRUE;
		continue;
	      case SC_TERM:
		/* 
		** Avoid endlessly pushing back fake terminators.  This can
		** happen when the grammar encounters an error in declaration 
		** section, but scanner is in EXEC mode.
		*/
		if (!was_term)
		{
		    lex_is_newline = TRUE;
		    sc_addtok( SC_YYSTR, ERx(";") );
		    lex_tok = tok_special[TOK_TERMINATE];
		}
		else
		{
		    continue;
		}
		break;
	    } /* switch is-host */
	} else /* not lex_is_newline */
	{
	    /*
	    ** For Ada and C only: (lex_more_sql is FALSE for other languages)
	    ** If last statement was an SQL statement we might not be done
	    ** Need to eat white space and look for a comment.
	    */
	    if(dml->dm_exec == DML_HOST && lex_more_sql)
	    {
		/* Eat white space or host comments until '\n' and then look 
		** for SQL comment */
		cp = SC_PTR;
		while (*cp != '\n' && *cp != SC_EOFCH)
		{
		    if (yy_isspace(cp))
			CMnext(cp);
		    else if (sc_eatcomment(&cp, TRUE))
			continue;
		    else
			break;
		}
		SC_PTR = cp;

		/* look for first non-white character, if other
		** than newline, SQL comment, or EOF, this could
		** be host code or another SQL statement. Need to
		** Process rest of line as a new statement.
		*/
		if (*cp != '\n' && !(*cp == '-' && *(cp+1) == '-' ))
		{
		    lex_is_newline = TRUE;
		    continue;
		}
	    }
	    if (CMnmstart(SC_PTR))
	    {
		if (sc_hostvar)
		{
		    /* scanning a host variable reference; save SC_PTR */
		    sc_saveptr = SC_PTR;
		}
		lex_tok = sc_word( mode );
		if (eq->eq_flags & EQ_FIPS)
		    yyreswd = FALSE;		/* no reswd retry */
	    }
	    else if (CMdigit(SC_PTR))
	    {
		/* if we're in a begin-end declare section or if we're
		   scanning a host variable reference, and the host language
		   is COBOL, we need to allow digits to start an identifier */
		if (((eq->eq_flags & EQ_INDECL) || sc_hostvar)
						&& eq_islang( EQ_COBOL ))
		{
		    if (sc_numname())
			lex_tok = sc_word( mode );
		    else
			lex_tok = sc_number( mode );
		    if (eq->eq_flags & EQ_FIPS)
			yyreswd = FALSE;	/* no reswd retry */
		}
		else
		{
		    if ( *(SC_PTR) == '0' && (*(SC_PTR+1) == 'X' || *(SC_PTR+1) == 'x') )
			lex_tok = sc_word( mode );
		    else
			lex_tok = sc_number( mode );
		}
	    }
	    else if (CMoper(SC_PTR))
	    {
		lex_tok = sc_operator( mode );
	    }
	    else if (mode != SC_COOKED)
	    {
		static char    cbuf[3]; 
		char	*p = cbuf;

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
    /* 
    ** Remember that current token is the fake terminator to avoid repeatedly
    ** pushing back terminators.
    */
    was_term = (lex_tok == tok_special[TOK_TERMINATE]);

# ifdef YYDEBUG
    if (yydebug)
    {
	char *s = ERx("cooked");

	if (mode == SC_RAW)
	    s = ERx("raw");
	else if (mode == SC_RARE)
	    s = ERx("rare");
	SIprintf( ERx("yylex(%s) = %d, yylval = '%s'\n"),s, lex_tok, yylval.s);
    }
# endif /* YYDEBUG */

    return lex_tok;		/* Hopefully Yacc can use it */
}

/*
+* Procedure:	sc_esqishost 
** Purpose:	Return the type of the (new) current line.
**
** Parameters:	bool  - mode	- level of cookedness.
-* Returns:	i4 - SC_CONTINUE/SC_SQL/SC_FRS/SC_HOST/SC_4GL
** Notes:
**		If the line starts with "$_" then do the debugging call
**		and return SC_DEBUG.
**		Else if in EXEC mode then return SC_CONT if this is a continued
**		line, else SC_TERM if we should fake a terminator (only BASIC
**		or FORTRAN).
**		Else if in DECL mode then return SC_CONT (declaration lines
**		are implicitly continued).
**		Else if it starts with "EXEC SQL" return SC_SQL.
**		Else if it starts with "EXEC FRS" return SC_FRS.
**		Else if it starts with "EXEC 4GL" return SC_4GL.
**		Else return SC_HOST.
*/

i4
sc_esqishost( mode )
i4	mode;
{
    char	*cp;
    char	*lbl;
    char	*sc_label();
    i4		tok;

  /* SC_PTR points at first char of line - after lines have been skipped */

  /* special debugging hack -- can't always wait for "exec sql"! */
    if (SC_PTR[0]=='$' && SC_PTR[1]=='_')
    {
	SC_PTR += 2;
	sc_dbgcomment();
	return SC_DEBUG;
    }

    if (mode != SC_COOKED)
    {
	if (mode == SC_RARE)
	    (void) sc_newline();
	return SC_CONT;
    }

    /* If we get this far, it's because we're in COOKED mode. */

    cp = SC_PTR;
    if (dml->dm_exec & DML_EXEC)
    {
	/* see if this line is continued */
	if (sc_newline())	
	    return SC_CONT;
	return SC_TERM;			/* Fake a terminator */
    }

    /* For all languages except C and Fortran this will always be false.
    ** In "C":  Check if the current line is within a multi-line
    ** 		host string or host comment. 
    ** In Fortran: Check if this is a continuation line.
    */
    if (sc_inhostsc())
	return SC_HOST;

    /*
     * Looking for "^{white}[label]{white}EXEC{white}+(SQL|FRS)"
     *
     * COBOL calls the slave grammar routine sc_label to look
     * for a sequence number even if in a declaraction section. This will 
     * allow sequence numbers to be found on any EXEC statements in a declare
     * section (such as EXEC SQL END DECLARE statement and any EXEC SQL INCLUDE
     * statements).
     */

    if ((dml->dm_exec != DML_DECL)
	       ||(eq_islang(EQ_COBOL))) /* COBOL hook to catch sequence no. */
	cp = sc_label( &lbl );		/* scan an optional label */
    else
	lbl = (char *)0;

    /*
    ** looking for "^{white}EXEC{white}+(SQL|FRS|4GL)"
    */

    if ((tok = sc_execstmt(&cp)) != SC_EOF)
    {
	if (lbl)
	{
	    out_emit( lbl );
	    out_emit( ERx("\n") );
	}

	if ((tok == SC_FRS) && (eq->eq_flags & EQ_FIPS))
	    er_write( E_EQ050B_FIPS_FRS, EQ_WARN, 0 );

        else if (tok == SC_4GL)
	{
            if (eq->eq_flags & EQ_FIPS)
                 er_write( E_EQ051C_FIPS_4GL, EQ_WARN, 0 );
	}

	SC_PTR = cp;
	return tok;	
    }

    /*
     * Line doesn't start with
     *	"^{white}[label]{white}EXEC{white}+(SQL|FRS)".
     * 
     * We might have a continuation indicator (or we might not).
     * Either way, eat up the margin (with continuation if present).
     */
    if (dml->dm_exec == DML_DECL)    /* DECL continues to process tokens */
    {
	sc_skipmargin();
	return SC_CONT;
    }

    /*
     *	For Ada multi-threading support check whether line starts with
     *	"^{white}*TASK{white}+BODY".
     */
#if 0
/*
** Based on client experience with multi-tasked embedded Ada, include
** the TLS-based sqlca triggered by "EXEC SQL BEGIN DECLARE SECTION;"
** rather than on task body definition. This may be revisited, so leave
** (dormant) code in place to support scanning for "TASK BODY".
*/
    if ((eq_islang(EQ_ADA)) && (eq->eq_flags & EQ_MULTI))
	if ((tok = sc_ataskbody(cp)) == SC_ATASKBODY)
	    eq->eq_flags |= EQ_ATASKBODY;
#endif

    return SC_HOST;
}

/*
** sc_popscptr - Move SC_PTR back one word.  
**
**	This function is called from the slave grammers when it is determined
**	that a word needs to be rescanned with keyword lookups enabled.
**
**	Parameters:
**		None
**
**	Returns:
**		None
**
**	Assumptions:
**		This function assumes that a non-null sc_saveptr points to a 
**		valid address within the scanner buffer.
**
**	Side effects:
**		Two globals, SC_PTR and sc_saveptr, are modified by this 
**		routine.
**
**	Notes:
**		Here are the comments from the slave grammer on why this 
**		function is needed:
** -----
** Now we allow the use of reserved words as host variables.  The way we do
** this is set a global (sc_hostvar) to TRUE when we see a Ccolon (or a
** Csqlda_colon).  The global tells scword (in the scanner) to ignore keyword
** lookups and just returne a tNAME.  We turn it off when we're at the end
** of the hostvar reference.  There's a problem, however, in that sometimes
** the grammer doesn't know to turn the global off until after we've scanned
** a real keyword.  For instance, in the statement:
**      EXEC SQL CONNECT :connect SESSION :session;
** by the time the grammer turns off the global for :connect we've already
** scanned SESSION, and it has been returned as a tNAME instead of tSESSION.
** what we do here is to introduce a new rule that allows a hostvar to be a
** variable or a variable followed by a tNAME.  In either case, we turn off
** the sc_hostvar global.  Additionally if we see a variable followed by a
** tNAME we call a new function, sc_popscptr, which sets up the scanner to
** rescan the word with keyword lookups enabled.  So in our example above
** SESSION will get "scanned" twice, once as a tNAME and then as tSESSION.
** the same thing is done for sqlda variables.
** -----
**
** History:
**	02-oct-1992 (larrym)
**		written.
*/
void
sc_popscptr()
{
	if( sc_saveptr )
	{
	    SC_PTR = sc_saveptr;
	    sc_saveptr = (char *) 0;
	}
	else
	{
	    /* this should never happen! */
	    er_write( E_EQ0003_eqNULL, EQ_ERROR, 1, ERx("sc_popscptr") );
	}
	return;
}

/* {
** Name: sc_moresql - Do we have more SQL code to parse?
**
** Description:
**	This is called by the slave grammer to indicate that
**	it has just finished parsing an SQL statement, and
**	we now need to look for an SQL comment.
**
** Inputs:
**      None.
**
** Outputs:
**      None.
**
** Returns:
**      None.
**
** Side Effects:
**      Sets the lex_more_sql flag which is used in the scanner.
**
** History:
**      09-feb-1993     (teresal)
**          Written.
*/

void
sc_moresql()
{
    lex_more_sql = TRUE;
}



/* {
** Name: sc_execstmt - Scan for EXEC SQL|FRS|4GL.
**
** Description:
**	Scan a given string for an embedded sql statement:
**		"^{white}EXEC{white}+(SQL|FRS|4GL)".
**	If an embedded statement is found then the stmt token is returned and
**	the string pointer updated.  If the string does not contain an
**	embedded statement, then SC_EOF token is returned and the string
**	pointer is not updated.
**
** Inputs:
**      cptr:	Address of pointer to string.
**
** Outputs:
**      cptr:	Updated to address of character following Embedded keywords.
**
** Returns:
**      The token of the statement found:   SC_SQL, SC_FRS, or SC_4GL
**	Or if no embedded statement was found: SC_EOF
**
** Side Effects:
**      None.
**
** History:
**    	10-jun-1993     (kathryn)
**	    Written.
**	25-jun-1993 	(kathryn)
**	    Correct for loop code, so that it doesn't attempt to reference
**	    through null pointer on host declarations.
*/


static KEY_ELM exec_kword[] = {
	"sql" ,    SC_SQL,
	"frs" ,    SC_FRS,
	"4gl" ,    SC_4GL,
	(char *)0, SC_EOF
};

i4
sc_execstmt(char **cptr)
{
    char	*cp = *cptr;
    KEY_ELM	*kptr;

    
    while (yy_isspace(cp))
        CMnext( cp );

    if ((STbcompare(cp,4,ERx("exec"),4,TRUE)==0) && !CMnmchar(&cp[4]))
    {
        cp += 4;
        while (yy_isspace(cp))
            CMnext( cp );
	for (kptr = exec_kword; kptr->key_term != NULL; kptr++)
        {
            if ((STbcompare(kptr->key_term, 3, cp, 3, TRUE)==0) &&
            	!CMnmchar(&cp[3]))
            {
	        cp += 3;
		*cptr = cp;
            	return (kptr->key_tok);
            }
        }
    }
    return SC_EOF;
}

/* {
** Name: sc_ataskbody - Scan for Ada TASK BODY.
**
** Description:
**	Scan a given string for the Ada host language statement:
**		"^{white}*TASK{white}+BODY".
**
** Inputs:
**	cptr:	Address of pointer to string.
**
** Outputs:
**	None.
**
** Returns:
**	SC_ATASKBODY if "TASK BODY" was scanned, else SC_EOF
**
** Side Effects:
**	None.
**
** History:
**	24-Mar-2007 (toumi01)
**	    Written for Ada multi-thread (multi-task) support.
*/

#if 0
sc_ataskbody(char *cp)
{
    while (yy_isspace(cp))
	CMnext( cp );

    if ((STbcompare(cp,4,ERx("task"),4,TRUE)==0) && !CMnmchar(&cp[4]))
    {
	cp += 4;
	while (yy_isspace(cp))
	    CMnext( cp );
	if ((STbcompare(cp,4,ERx("body"),4,TRUE)==0) && !CMnmchar(&cp[4]))
	    return SC_ATASKBODY;
    }
    return SC_EOF;
}
#endif

/* {
** Name: sc_numname - Determine whether it's an identifier or a number.
**
** Description:
**	This routine is called when SC_PTR is pointing to a digit.  It scans
**	till white space and determines whether the string up to the white
**	space should be treated as a word or a number.
**
** Inputs:
**	None.
**
** Outputs:
**	None.
**
** Returns:
**	bool	- TRUE if (we think) it's an identifier.
**		  FALSE if (we think) it's a number.
**
** Side Effects:
**	None.
**
** History:
**	21-jun-1993 (lan)
**		Written.
**	18-aug-1993 (kathryn) Bug 54224
**	    Return FALSE when first non-name chars are found, as we may be at 
**	    end of token. This allows us to correctly parse numbers which
**	    may not be separated by whitespace such as "(12)".
*/

bool
sc_numname()
{
    char	*cp;
    bool	retval;

    cp = SC_PTR;
    retval = FALSE;	/* we start with a digit, so it may be just a number */
    while (CMnmchar(cp) || *cp == '-')
    {
	if (!CMdigit(cp))
	{
	    if (*cp == 'E' || *cp == 'e')
	    {
		cp++;
		if (*cp == '-')		/* could still be either id or num */
		    cp++; 
		else if (*cp == '+')   /* "+" not allowed in identifier */
		    return FALSE;

		if (!CMdigit(cp))	/* not exponent, so not a number */
		{
		    return TRUE;
		}
	    }
	    else if (*cp == '.')
		return FALSE;		/* it can still be a number,
					   but we let sc_number deal with it */
	    else
		return TRUE;		/* it may an illegal character,
					   but we let sc_word deal with it */
	}
	cp++;
    }
    return retval;
}
