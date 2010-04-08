# include	<compat.h>
# include	<er.h>
# include	<si.h>
# include	<cm.h>
# include	<equel.h>
# include	<equtils.h>
# include	<eqscan.h>
# include	<eqlang.h>
# include	<eqsym.h>	/* for <eqgen.h> */
# include	<eqgen.h>
# include	<ereq.h>

/*
+* Filename:	SCSQSTRING.C
** Purpose:	Scan or generate a string constant.
**
** Defines:	
**		sc_string()		- Read an SQL string constant.
**		gen_sqmakesconst()	- Generate an SQL string constant.
** Locals:	
**		sc_sqstring()		- ESQL version
**		sc_hoststring()		- ESQL host string version
**		sc_eqstring()		- EQUEL version
**		sc_sput()		- Add to local string buffer.
-*		sc_str_reset()		- Reset local string buffer.
**
** Side Effects:
**	Strips string from Equel/ESQL code and buffers away.
**
** Notes:
**  1. If we're called through sc_eat or sc_tokeat then mode will be
**	raw or rare and we will preserve the string delimiters.
**  2. A string may be at most MAXSTRING characters long.
** SQL Notes:
**  1. Strings are kept internally exactly as their external appearance, 
**     except for the outermost delimiter, and except that doubled ticks
**     are made single again and host-specific line breaks are elided.
**  2. Strings in declaration sections don't fold doubled ticks.
** EQUEL Notes:
**  1. Strings are kept internally exactly as their external appearance, 
**     except for the outermost delimiter.
**  2. A string may have escaped characters - \X including a \ before a newline.
**
** History: 	15-nov-1984 -	Modified from original. (ncg)
**		06-may-1986 -	Modified for 6.0 ESQL. (mrw)
**		29-jun-87 (bab)	Code cleanup.
**	14-dec-1992 (lan)
**		Added support for delimited identifiers.  Changed the behavior
**		when the wrong quotes are encountered.  Double quotes are now
**		accepted.  If double quotes are found at a wrong place, ESQL
**		now gives a syntax error instead of error E_EQ0229.  ESQL no
**		longer makes the assumption that the user meant single quotes
**		but typed double quotes.
**	15-dec-1992 (lan)
**		Fixed bugs where EQC gave an error on ERx("xyz") used in a
**		form statement, and ESQLC did not accept a double quoted
**		string in a #define within a begin/end declare section.  Also
**		fixed a problem where ESQL did not preserve all double quotes
**		within a delimited identifier.
**	12-apr-1992 (geri)
**		Bug 54017: Insure that delimited ids within BEGIN/END DECLARE
**		statements are flagged correctly as TOK_DELIMID
**
** Copyright (c) 2004 Ingres Corporation
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

static char	buf[ SC_STRMAX + 4 ] ZERO_FILL;	/* Buffers string */
static char	*cp ZERO_FILL;			/* Bufferer */
static i4	errs ZERO_FILL;			/* Overflow count per string */
static void	sc_sqstring();
static void	sc_hoststring();
static void	sc_eqstring();
static void	sc_sput();
static void	sc_str_reset();

# define	SC_ESCAPE	'\\'

/* 
** Max size of converted string - allow some extra space over scanner for
** extra escaped characters required.
*/

# define 	G_MAXSCONST	300	
 
/* 
+* Procedure:	sc_string
** Purpose:	Scan and save string constant.
** Description:
**		Either an SQL string or an EQUEL string is scanned;
**		if SQL, then either it is a string in a declaration
**		section and will be stored as-is, or it is an SQL string
**		and SQL rules will be applied.  In any case, the delimiters
**		are stored if and only if "mode" is SC_COOKED.
**
** Parameters:	quotekey - KEY_ELM * - Pointer to string quote operator in 
**				       operator table.
**		mode	 - i4	     - level of processing
** Returns:	i4	 - Scanner String constant - String scanned (SCONST),
-*			   (Scanner Eof - No string terminator (SC_EOF))
*/

i4
sc_string( quotekey, mode )
KEY_ELM	*quotekey;
i4	mode;
{
    char		sdelim;				/* Closing quote */

    sdelim = *quotekey->key_term;		/* Known to be non-Kanji */
    sc_str_reset();				/* Reset sc_sput */
    if (mode == SC_RAW)
	*cp++ = sdelim;
    *cp = '\0';

    if (dml_islang(DML_ESQL))
    {
	if ((eq->eq_flags & EQ_INDECL) || mode == SC_RAW )
	    sc_hoststring( quotekey );
	else
	    sc_sqstring( quotekey );
    } else
	sc_eqstring( quotekey );

    if (mode == SC_RAW)
	*cp++ = sdelim;
    *cp = '\0';
    sc_addtok( SC_YYSTR, str_add(STRNULL, buf) );
    if ((sdelim == '"') && dml_islang(DML_ESQL)
		&& (!(eq->eq_flags & EQ_INDECL)  
		    || (eq->eq_flags & EQ_INDECL && dml->dm_exec & DML_EXEC))
		&& (mode != SC_RAW))
    	return tok_special[ TOK_DELIMID ];
    else
	return tok_special[ TOK_STRING ];
}

/* 
+* Procedure:	sc_sqstring
** Purpose:	Scan and save SQL string constant.
** Description:
**		Scan an SQL string, using SQL rules.  Line breaks are
**		removed, following host language rules, and doubled delimiters
**		are folded into single delimiters.
**		If the wrong delimiter is used then we issue a warning and
**		and assume the user meant to switch delimiters for this string.
**		Note that host language escape characters have no meaning
**		in this routine (except for backslash-newline in C).
** Note:	eq_sql_quote, eq_host_quote, and eq_quote_esc are known
**		to be non-Kanji.
** Parameters:	quotekey - KEY_ELM * - Pointer to string quote operator in 
**				       operator table.
-* Returns:	Nothing.
*/

static void
sc_sqstring( quotekey )
KEY_ELM	*quotekey;
{
    char		c;			/* Input characters */
    char		sdelim;			/* Closing quote */
    bool		saw_quote = FALSE;	/* Saw embedded quote? */

    sdelim = *quotekey->key_term;		/* Known to be non-Kanji */

    for (;;)	/* forever */
    {
	/*
	** Look at the next byte of the string.
	** We'll check if it's the first byte of a two-byte Kanji character
	** after checking for all of the special one-byte characters.
	**
	** If it's an embedded double-quote then remember it, in case
	** this is BASIC, where that's illegal.  Doubled (and thus embedded)
	** delimiter double-quotes are detected in the following IF statement.
	*/
	c = *SC_PTR;
	if (c == '"' && c != sdelim)
	    saw_quote = TRUE;

	/*
	** SQL strings are always SQL strings, even if the wrong delimiter
	** is used, and the rule for embedding the SQL delimiter is to
	** double it.  Host language embedding rules don't apply.
	** If the wrong delimiter is used then we pretend (for this string
	** only) that they switch the sql quote.
	**
	** This is independent of the host language.
	*/
	if (c == sdelim)
	{
	    SC_PTR++;			/* eat the delimiter */

	  /* A doubled delimiter preserves only one of them. */
	    if (*SC_PTR == sdelim)
	    {
		sc_sput( &c );
		if (*SC_PTR == '"')
		    sc_sput( SC_PTR );
		SC_PTR++;		/* eat the doubled delimiter */
	      /* And remember if it was a double-quote, for BASIC */
		if (c == '"')
		    saw_quote = TRUE;
	    } else			/* we're done */
	    {
		break;
	    }
	} else if (c == '\n')
	{
	    sc_readline();
	  /* (*dm_strcont) will skip margin for us if it is continuation */
	    if (dml->dm_strcontin && (*dml->dm_strcontin)(sdelim))
		continue;
	    *cp = '\0';
	    er_write( E_EQ0219_scNLSTR, EQ_ERROR, 1, buf );
	    break;
	} else if (c == SC_EOFCH)
	{
	    /*
	    ** Check for SC_EOFCH before eq_quote_esc as languages without
	    ** a delimiter escape mechanism (ie, BASIC) set the host quote
	    ** escape char to SC_EOFCH.
	    */
	    *cp = '\0';
	    er_write( E_EQ0206_scEOFSTR, EQ_ERROR, 1, buf );
	    break;
	} else if (c == '\\')	/* Just for C, so check explicitly */
	{
	    SC_PTR++;

	    /*
	    ** host escape is interpreted only for C, and then only
	    ** for backslash-newline, and that only because we don't
	    ** currently handle C string continuation through a DML
	    ** routine as we do for all the other languages.
	    */
	    if (eq_islang(EQ_C) && *SC_PTR == '\n')
		sc_readline();
	    else
		sc_sput( &c );
	} else		/* not any sort of special character */
	{
	    sc_sput(SC_PTR);
	    CMnext(SC_PTR);
	}
    } 

    if (saw_quote && eq_islang(EQ_BASIC))
	er_write( E_EQ0230_scEMBEDQUOTE, EQ_ERROR, 1, ERx("\"") );
}

/* 
+* Procedure:	sc_hoststring
** Purpose:	Scan and save host string constant.
** Description:
**		Scan a string using host-language rules for delimiting
**		the string and for continuing across lines.  Store the
**		string as written, except for removing line breaks.
**		Host language rules for embedding the delimiter are to
**		double it, except for C where it is to backslash it.
**		Note that BASIC doesn't allow embedding the delimiter.
**		We allow any delimiter without complaining.
** Notes:	This routine is special-cased to work for the Ada character
**		constant for an apostrophe, which is 3 ticks: '''
**		Backslash-newline is now allowed only for C, although we used
**		to allow it for other languages.  Only host-language
**		continuation is legal now.
**
** Parameters:	quotekey - KEY_ELM * - Pointer to string quote operator in 
**				       operator table.
-* Returns:	Nothing.
*/

static void
sc_hoststring( quotekey )
KEY_ELM	*quotekey;
{
    char		c,				/* Input characters */
			peekc;				/* lookahead chars */
    char		sdelim;				/* Closing quote */

    sdelim = *quotekey->key_term;		/* Known to be non-Kanji */

    for (;;)	/* forever */
    {
	if ((c = *SC_PTR) == sdelim)
	{
	    SC_PTR++;

	    /*
	    ** In C, escaped delimiters are handled below;
	    ** BASIC doesn't have escaped delimters
	    */
	    if (eq_islang(EQ_C) || eq_islang(EQ_BASIC))
		break;

	    if ((peekc = *SC_PTR) != sdelim)
		break;

	      /* Doubled delimiter is escaped, except maybe in Ada */
	    sc_sput( &c );
	    sc_sput( &peekc );
	    SC_PTR++;
	    if (eq_islang(EQ_ADA) && (c=='\''))
	    {
	      /* In Ada, ''' means the character constant ' */
		if ((c = *SC_PTR) == peekc)
		{
		    sc_sput( &c );
		    SC_PTR++;
		}
	    }
	} else if ((c == '\\') && eq_islang(EQ_C))
	{
	    SC_PTR++;

	  /* In C, \x is ignored if x == '\n', else swallowed whole */
	    if ((peekc = *SC_PTR) != '\n')
	    {
		sc_sput( &c );
		sc_sput(SC_PTR);
		CMnext(SC_PTR);
	    } else
		sc_readline();
	} else if (c == SC_EOFCH)
	{
	    *cp = '\0';
	    er_write( E_EQ0206_scEOFSTR, EQ_ERROR, 1, buf );
	    break;
	} else if (c == '\n')
	{
	    sc_readline();
	  /* (*dm_strcont) will skip margin for us if it is continuation */
	    if (dml->dm_strcontin && (*dml->dm_strcontin)(sdelim))
		continue;
	    *cp = '\0';
	    er_write( E_EQ0219_scNLSTR, EQ_ERROR, 1, buf );
	    break;
	} else		/* not any sort of special character */
	{
	    sc_sput(SC_PTR);
	    CMnext(SC_PTR);
	}
    } 
}

/*  
+* Procedure:	sc_eqstring
** Purpose:	Scan and save string constant.
** Description:
**		The rather strange EQUEL string rules are followed:
**		* Backslash newline is ignored (in all languages!)
**		* The line is saved as written (except for line breaks)
**		* Host language rules are used for embedding the delimiter
**		  (backslash for C, illegal for BASIC, doubled for others)
**		* Any delimiter pair is allowed.
**
** Parameters:	quotekey - KEY_ELM * - Pointer to string quote operator in 
**				       operator table.
-* Returns:	Nothing.
** History:
**		03-oct-1988 - Removed "break" after scanning nested quotes (ncg)
*/

static void
sc_eqstring( quotekey )
KEY_ELM	*quotekey;
{
    char		c;				/* Input characters */
    char		sdelim;				/* Closing quote */

    sdelim = *quotekey->key_term;		/* Known to be non-Kanji */

    for (;;)	/* forever */
    {
	if ((c = *SC_PTR) == SC_ESCAPE)
	{
	    SC_PTR++;
	    if (*SC_PTR != '\n')
	    {
		sc_sput(&c);
		sc_sput(SC_PTR);
		CMnext(SC_PTR);
	    } else
		sc_readline();
	} else if (c == '\n')
	{
	    sc_readline();
	    *cp = '\0';
	    er_write( E_EQ0219_scNLSTR, EQ_ERROR, 1, buf );
	    break;
	} else if (c == SC_EOFCH)
	{
	    *cp = '\0';
	    er_write( E_EQ0206_scEOFSTR, EQ_ERROR, 1, buf );
	    break;
	} else if (c == sdelim)
	{
	    SC_PTR++;
	    if (!eq_islang(EQ_C) && !eq_islang(EQ_BASIC) && *SC_PTR == sdelim)
	    {
		SC_PTR++;
		sc_sput( &c );
		sc_sput( &c );
	    }
	    else  /* BUG 3621 - "Break" only if not nested quote */
		break;
	} else
	{
	    sc_sput(SC_PTR);
	    CMnext(SC_PTR);
	}
    } 
}

/*
+* Procedure:	sc_sput
** Purpose:	Add a character to static buffer (but do not cascade errors).
**
** Parameters:	c - char * - Character to add.
-* Returns:	Nothing.
*/

static void
sc_sput( c )
char	*c;
{

    if (errs)
	return;

    if (cp - buf < SC_STRMAX)
    {
	CMcpychar(c, cp);
	CMnext( cp );
    } else
    {
	*cp = '\0';
	er_write( E_EQ0215_scSTRLONG, EQ_ERROR, 2, er_na(SC_STRMAX), buf );
	errs++;
    }
}

/*{
+*  Name: sc_str_reset - Reset the string output buffer and flag.
**
**  Description:
**	Reset the sc_sput common output pointer and error counter.
**
**  Inputs:
**	None.
**
**  Outputs:
**	Returns:
**	    Nothing.
**	Errors:
**	    None.
-*
**  Side Effects:
**	Prepares for processing the next string.
**  History:
**	03-jun-1987 - Writen (mrw)
*/

static void
sc_str_reset()
{
    cp = buf;
    errs = 0;
}

/*
+* Procedure:	gen_sqmakesconst 
** Purpose:	Make an SQL host language string constant.
**
** Parameters:	flag  - i4      - G_INSTRING  - Nested in another string (DB 
**					       string),
**		        	  G_REGSTRING - String sent by itself in a call.
**		instr - char *  - Incoming string.
**		diff  - i4  *   - Mark if there were changes made to string.
-* Returns:	char *          - Pointer to static converted string.
**
** Notes:
** 1. Special Characters.
** 1.1. We always generate ticks ('\'') as the DBMS quote, regardless of
**	what the user's DBMS quote is.
** 1.2. Double the tick ('\'') if going to the database.
** 1.3. Escape the host quote and the host escape if needed for the compiler.
** 1.4. Note that the above two conditions can interact.
** 2. Note that this code works for all languages, without needing to check
**    which language it is.
** 3. We are guaranteed that eq->eq_host_quote and eq->eq_quote_esc are not
**    Kanji.
** History:
**	17-apr-1987 -   Generate single ticks as the outside delimiters for
**			nested ESQL strings.
**	11-may-1987 -	Rewritten for SQL. (mrw)
**	04-jun-1993 (kathryn)
**	    Don't escape a single quote within a delimited ID.
**	    Only escape double quotes if they are the host quote.
*/

char *
gen_sqmakesconst( flag, instr, diff )
i4	flag;
char	*instr;
i4	*diff;
{
    static	char	obuf[ G_MAXSCONST + 4 ];	/* Converted string */
    register	char	*icp;				/* To input */
    register	char	*ocp = obuf;			/* To output */
    register	char	host_quote = eq->eq_host_quote;
    register	char	quote_esc = eq->eq_quote_esc;

    *diff = 0;

  /* DBMS strings must be quoted with the tick */
    if (flag == G_INSTRING)
    {
      /* If the host quote is a tick then we must escape it. */
	if (host_quote == '\'')
	    *ocp++ = quote_esc;
	*ocp++ = '\'';
	*diff = 1;
    }
    if (flag == G_DELSTRING)
    {
	if (host_quote == '"')
	    *ocp++ = quote_esc;
	*ocp++ = '"';
	*diff = 1;
    }

    for (icp = instr; *icp && (ocp < &obuf[G_MAXSCONST]);)
    {
	if (*icp == '\'')
	{
	    icp++;
	    if (flag == G_INSTRING)
	    {
		if (host_quote == '\'' || quote_esc == '\'')
		    *ocp++ = quote_esc;	/* Escape for compiler */
		*ocp++ = '\'';		/* Escape for DBMS */
		*diff = 1;
	    }
	    if (host_quote == '\'' || quote_esc == '\'')
	    {
		*ocp++ = quote_esc;	/* Escape for compiler */
		*diff = 1;
	    }
	    *ocp++ = '\'';
	} else if (*icp == host_quote || *icp == quote_esc)
	{
	    *ocp++ = quote_esc;		/* Escape for compiler */
	    *ocp++ = *icp++;
	    *diff = 1;
	} else
	{
	    CMcpyinc( icp, ocp );	/* Might be Kanji */
	}
    }

    if (flag == G_INSTRING)
    {
      /* If the host quote is a tick then we must escape it. */
	if (host_quote == '\'')
	    *ocp++ = quote_esc;
	*ocp++ = '\'';
	*diff = 1;
    }
    if (flag == G_DELSTRING)
    {
	if (host_quote == '"')
	    *ocp++ = quote_esc;
	*ocp++ = '"';
	*diff = 1;
    }

    *ocp = '\0';
    return obuf;
}
