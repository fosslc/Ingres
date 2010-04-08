/*
** Copyright (c) 1984, 2008 Ingres Corporation
*/

#include	<compat.h>
# include	<cv.h>		 
#include	<si.h>
#include	<cm.h>
#include	<st.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<adf.h>
#include	<oslconf.h>
#include	<oslkw.h>
#include	<oserr.h>
#include	<osfiles.h>

# ifndef YYDEBUG
# define YYDEBUG
# endif

# ifdef YYDEBUG
    extern i4   yydebug;
# endif

/**
** Name:    scan.c -	OSL Parser Lexical Scanner Module.
**
** Description:
**	Contains the lexical scanner used for OSL and also routines that
**	access the scanner state.  Defines:
**
**	yylex()		osl parser lexical scanner.
**	osscnlno()	return line number of current line.
**	oserrline()	print current line for error report.
**	yyerror()	report syntax error from parser.
**	osSerrcnt()	return number of syntax errors.
**
** History:
**	Revision 3.0  joe
**	Initial revision.
**
**	Revision 4.0  85/09/11  15:30:37  arthur
**	Check for new value 'OSBUFFER' in osscandump().  85/08
**	Added new tokens LSQBRK and RSQBRK for [] so that they can
**	easily be changed in the scanner to a two character delimiter.
**	This undid any previous changes to allow () instead of [].  85/07  joe
**	Made IBM syntax changes  85/07  joe
**	SQL Integration.  85/07  joe
**	Added COLID and changes to aggregate syntax for SQL
**	QUEL extensions.  85/07  joe
**	Revision 40.5  85/07/08  12:21:33  joe
**	Made references to tokens go through osscankw a
**	new table.  85/06  joe
**
**	Revision 5.0  86/05  joe
**	86/02/13  09:45:30  joe
**	Fixed up scanner for long string constants.
**	Added check for long string constants.
**
**	86/02/06  13:45:49  joe
**	Fix for bug 8059.  The scanner nows correctly maintains all
**	information during lookahead.  lastsym and scanmode are set correctly.
**
**	85/12/20  15:36:21  joe
**	Added (| and |).
**
**	85/11/21  14:05:45  joe
**	Fix for two "end" keywords in a row.
**
**	Revision 5.1  86/12/03  18:49:12  wong
**	Mark position of token not white-space.
**	Scope conflict on 'cp' in 'errorcom()'.
**	(11/86)  Modified to recognize double keywords, and for efficiency.
**		
**	18-jul-1986 (Joe)
**		Added support for single quoted strings.  This added to
**		static variable 'acc_squote', the routine 'osacc_squote()'.
**
**	14-jul-86 (marian)	bug # 9591
**		Changed find_comment_end() so osgetc() is only called once
**		instead of twice.  Added new temp variable tmpch to hold the
**		new char.
**
**	Revision 6.0  87/06  wong
**	Added support for SQL tokens (hexadecimal strings.)  Also, backwards
**	compatibility in SQL for double-quote strings.
**
**	Revision 6.3/03/00  90/03  wong
**	Added support for counting block terminators (BEGIN, END, {}, () )
**	in visual query sections.
**	11/6/89 (Mike S)  Make "-g" error listing resemble "-l" listing.  For
**	full descriptions of the error reporting format, see "vq!vqerrfil.c"
**	89/07  wong  Added support for visual query frame errors and severity
**	report for 'oserrline()'.
**	9/90 (Mike S)
**		Porting change 130896 from griffin
**		Avoid optimization for hp3_us5
**	11/26/90 (emerson)
**		Disabled support for counting block terminators
**		in visual query sections because the implementation 
**		didn't account for the fact that certain tokens
**		that are usually block terminators aren't always
**		block terminators (e.g. the "end" in "scroll tf to end;").
**		This fixes bug 34524.  Note that instead of deleting lines,
**		I've bracketed them with #ifdef COUNTBLOCKTERM ... #endif
**		in case we decide to reinstate support (presumably refined)
**		for counting block terminators.
**
**	Revision 6.4
**	03/13/91 (emerson)
**		Get the decimal point character from the logical symbol
**		II_4GL_DECIMAL (whose value is now in osDecimal),
**		instead of II_DECIMAL.
**	25-jul-91 (johnr)
**		hp3_us5 no longer needs NO_OPTIM
**
**	04-mar-92 (leighb) DeskTop Porting Change:
**		Moved function declaration outside of calling function.
**		This is so function prototypes will work.
**	28-apr-92 (davel) bug 43702
**		Accept exponent in scientific notation with a unary plus; e.g.
**		2.3e+5.
**
**	Revision 6.5
**	17-jun-92 (davel)
**		Added scanner support for decimal datatype.
**	12-aug-92 (davel)
**		Add support for delimited identifiers.
**	10-nov-92 (davel)
**		Fix bug 47668: allow either 'X' or 'x' as hex string indicator.
**
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	15-feb-2002 (toumi01)
**	    Support keyword retry for ABF to avoid keyword problems.
**      17-dec-2008 (joea)
**          Replace READONLY/WSCREADONLY by const.
**	18-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
*/

GLOBALREF bool	osUser;
GLOBALREF char  osDecimal;
GLOBALREF char  osFloatLiteral;

GLOBALDEF char	*oslastsym;

VOID	yyerror();

/* 
** If the compiler is ever changed to compile multiple files,
** these statics will have to be re-initialized.
*/
static i4	line = 0;
/* report syntax errors */
static bool	report_errors = TRUE;

static i4	begin_section = 0;
static i4	end_section = 0;
static char	section[OSBUFSIZE] = ERx("");

# ifdef COUNTBLOCKTERM
static i4	brackets = 0;	/* statement block brackets, BEGIN, END */
static i4	parentheses = 0;
# endif

/*
** YACC Interface.
**
**	Extracted from "y.tab.h" (or "pgtab.h"), these are the relevant
**	definitions necessary to interface this module with the Yacc parser
**	without including the DML language dependent definitions file.
*/
typedef union  {
	char		*st_name;
	i4		st_nat;
} YYSTYPE;

GLOBALREF YYSTYPE yylval;
GLOBALREF bool	  yyreswd;

/*{
** Name:    yylex() -	OSL Parser Lexical Scanner.
**
** Description:
**	Lexically scans the input for tokens accepted by OSL or OSL/SQL.
**	Keywords (including double ones) are recognized by this routine, which
**	looks up identifiers returned by the 'lex()' routine keeping track of
**	look-ahead for double keywords.  Other tokens are recognized by the
**	'lex()' routine, and returned from this routine.
**
** Returns:
**	{nat}	The token number.
**
** Side Effects:
**	yylval.st_name	{char *}  Set for multiple-character tokens (especially
**				  double keywords.)
**
** History:
**	3-oct-86 (Joe) -- Written.
**	5-nov-86 (jhw) -- Modified.
*/

/*
** Save state variables required by look-ahead for double keywords.
**
**    next.token    {nat}  Next token found during look-ahead, if not -1.
**    next.sym	    {char *}  Symbol for token in 'next.token'.
**    next.lval	    {char *}  Next return value.
*/
static struct {
	    i4     token;
	    char   *sym;
	    char   *lval;
} next = {-1, NULL, NULL};

static i4	lex();

i4
yylex ()
{
    register i4	token;
    register KW		*kw;

    KW		*iskeyword();
    KW		*isdblkeyword();

    if (next.token == -1)
	token = lex();
    else
    { /* have next token by look-ahead */
	token = next.token;
	oslastsym = next.sym;
	yylval.st_name = next.lval;
	/* restore position */
	next.token = -1;
    }

    /* Check for keyword */

    yyreswd = FALSE;
    if (token == osscankw[OSIDENT] && (kw = iskeyword(yylval.st_name)) != NULL)
    { /* keyword */
	i4	sym_line = line;	/* current symbol line */

	if (kw->kwdbltab != NULL)
	{ /* Check for double keyword */
	    register KW		*dkw;
	    char		*lastsym;

	    lastsym = yylval.st_name;
	    /*save position*/
	    next.token = lex();
	    if (next.token == osscankw[OSIDENT] &&
		    (dkw = isdblkeyword(yylval.st_name, kw->kwdbltab)) != NULL)
	    { /* Double keyword */
		kw = dkw;
		yylval.st_name =
		     STpolycat(3, lastsym, ERx(" "), yylval.st_name, oslastsym);
		next.token = -1;
	    }
	    else
	    { /* not a double, setup look-ahead */
		next.lval = yylval.st_name;
		next.sym = oslastsym;
		/*save position in next var's*/
		yylval.st_name = oslastsym = lastsym;	/* restore ID/keyword */
	    }
	    /*restore old save position*/
	}

	/* check if keyword (otherwise it's an identifier) */
	if ( kw->kwtok != 0 )
	{
	    token = kw->kwtok;
	    yyreswd = TRUE;
# ifdef COUNTBLOCKTERM
	    if ( end_section < begin_section && sym_line >= begin_section )
	    { /* Check for BEGIN, END keywords */
		if ( token == osscankw[OSBEGIN] )
			++brackets;
		else if ( token == osscankw[OSEND] )
			if ( --brackets < 0 )
				yyerror("syntax error");
	    }
# endif
	}
    }

# ifdef YYDEBUG
    if (yydebug)
    {
        SIprintf( ERx("yylex() = %d, yylval = '%s'\n"), token, yylval.st_name);
        SIflush(stdout);
    }
# endif

    return token;
}

/*
** Name:    lex() -		Scan for Individual Tokens.
**
** Description:
**	Scan the input for individual tokens.  Keywords (both double and single)
**	are not recognized at this level so that look-ahead can be ignored.
**	All other tokens are recognized by this routine.
**
** Returns:
**	{nat}	The token number.
**
** Side Effects:
**	yylval.st_name	{char *}  Set for multiple-character tokens (e.g.,
**				  SCONST, ':=', ID, etc.)
**
** History:
*/

static VOID	markpos();
static VOID	comment();
static i4	_hexvchar();
static i4	_varchar();
static i4	_text();
static i4	_2char();

/*}
** Name:    DBLCH -		Double-Character Token Stucture Definition.
**
** Description:
**	An acceptable second character and the token symbol for the
**	recognized two character token.
*/
typedef struct {
	    char    _2ndch;	/* second character of 2 character token */
	    i4	    sym;	/* symbol for recognized 2 character token */
} DBLCH;

static char	*cp = ERx(" ");	/* input character pointer */

/*
** Name:    nextch() -	Get Next Character from Input.
**
** Description:
**	Gets the next character from the input by incrementing the input
**	character pointer, 'cp', or setting it to the new input line.
**	Implemented as a macro for efficiency.  Calls 'input_line()' to
**	read the next input line.
*/
static char	*input_line();

#define nextch() {CMnext(cp); if (*cp == EOS) cp = input_line();}

/*
** Name:    copych() -	Copies Input Character to Buffer
**				    And Gets Next Input Character.
** Description:
**	Copies the input character to the buffer and gets the next input
**	character.  Implemented as a macro for efficiency.
*/
#define copych(sp) {CMcpychar(cp, (sp)); CMnext(sp); nextch();}

/*
** Lexical Symbol.
*/
static char	symbuf[OSBUFSIZE];

char	*iiIG_string();			 
ADF_CB	*FEadfcb();			 

static i4
lex ()
{
    register char	*sp = symbuf;
    register char	*str = NULL;
    register i4	token;

    oslastsym = symbuf;
    symbuf[0] = EOS;
    for (token = -1 ; token < 0 ;)
    {
	/* Eat white space */
	while ( CMwhite(cp) )
	    nextch();

	if ( *cp == EOS )
	    token = 0;	/* EOF */
	else
	{ /* Start token collection */
	    markpos(cp);	/* mark position of token */

	    if (CMnmstart(cp))
	    { /* identifier, keyword, or hexadecimal string */
		do
		{
		    copych(sp);
		} while (CMnmchar(cp));
    
		if (QUEL || *cp != '\'' || 
		    (symbuf[0] != 'X' && symbuf[0] != 'x') || sp != symbuf + 1)
		{ /* identifier */
		    str = symbuf;
		    token = osscankw[OSIDENT];
		}
		else
		{
		    copych(sp);
		    return _hexvchar();	/* hexadecimal string constant */
		}
	    }
	    else if (!QUEL && *cp == '\"')
	    { /* delimitted identifier */
		copych(sp);	/* copy first double quote */
		for(;;)
		{
		    if (*cp == '\n')
		    {
	    		oscerr(OSUNTERMDELIM, 0);
	    		break;
		    }
		    else if (*cp == '\"')
		    { /* possible delimiter */
			copych(sp);
			if (*cp != '\"')
			{
			    break;
			}
		    }
		    copych(sp);
		}
		str = symbuf;
		token = osscankw[OSIDENT];
   	    } 
	    /*
	    ** Allow for floating point constants that
	    ** begin with a decimal point (usually '.'.)
	    */
	    else if ( CMdigit(cp) || *cp == osDecimal )
	    { /* numeric constant */
		i4	type;
   		i4	num_digits = 0;

		if ( *cp != osDecimal )
			type = OSICONST;
		else
		{ /* possible float/packed-decimal starting with a decimal pt */
			type = OSDCONST;
			copych(sp);
		}
		if ( !CMdigit(cp) )
			token = osDecimal;
		else
		{ /* numeric constant */
			bool	exp = FALSE;
			i4 ltemp;

			str = symbuf;
			for (;;)
			{
				do {
					num_digits++;
					copych(sp);
				} while ( CMdigit(cp) );
				if ( type == OSICONST && *cp == osDecimal )
				{
					type = OSDCONST;
					num_digits--;
				}
				/*
				** BUG 5694
				**
				** Recognize scientific notation for
				** floating point constants.
				*/
				else if ( !exp && *cp == 'e' || *cp == 'E' )
				{
					type = OSFCONST;
					copych(sp);
					if ( *cp != '-' &&
					     *cp != '+' &&
					     !CMdigit(cp)
					   )
						break;	/* error */
					exp = TRUE;
					/* note: digit count is no longer
					** needed, as the literal cannot
					** be a decimal literal.
					*/
				}
				else
					break;
			} /* end numeric for */

			/* promote integer literal to decimal if it would 
			** overflow a i4. Note this can only 
			** happen if the string contains 10 or more digits ( 
			** assuming that i4 is always at least an i4) -
			** this is only a little performance boost so we
			** don't needlessly CVal reasonable-sized int literals.
			*/ 
			*sp = EOS;	/* needed for CVal call below */
			if (num_digits >= 10 && type == OSICONST &&
			    CVal(str, &ltemp) != OK)
			{
				type = OSDCONST;
			}

			/* also promote decimal to float if more than 
			** DB_MAX_DECPREC digits are specified
			*/
			if (num_digits > DB_MAX_DECPREC && type == OSDCONST)
			{
				type = OSFCONST;
			}

			/* check backward compatibility with respect to float
			** or decimal literals (global osFloatLiteral set
			** from logical II_NUMERIC_LITERAL in osl.c).
			*/
			if (osFloatLiteral && type == OSDCONST)
			{
				type = OSFCONST;
			}

			token = osscankw[type];
		}
	    }
	    else switch (*cp)
	    {
	    /*
	    ** Look for all one and two letter delimiters.
	    */
		case '/':
		    nextch();
		    if (*cp != '*')
			token = *sp++ = '/';
		    else
		    { /* start of comment */
			comment();
			continue;
		    }
		    break;
    
		case '\'':
		    nextch();
		    if (QUEL)
			oscerr(OSSNGLQUOTE, 1, (PTR)&line);
		    return _varchar();
		    break;
    
		case '"':
		    nextch();
		    return _text(osldml);
    
		/*
		** BUG 3987
		**
		** Allow for dereferencing so that OSL can recognize
		** Ingres symbolic constants such as "usercode"
		*/
		case '#':		/* start of DEREFERENCE */
		    do {
			copych(sp);
		    } while (CMalpha(cp));
		    str = symbuf + 1;
		    token = osscankw[OSDEREFERENCE];
		    break;
    
		/*
		** All two character delimiters go to a special routine
		** to determine if the two letter symbol was found.
		*/
		case ':':
		{
		    static const DBLCH   colon[] = {{'=', OSCOLEQ}, EOS};
    
		    if ((token = _2char(colon)) != ':' || !CMnmstart(cp))
			return token;
		    else
		    { /* disambiguated identifier */
			*sp++ = ':';
			do
			{
			    copych(sp);
			} while ( CMnmchar(cp) );
			str = symbuf + 1;
			token = osscankw[OSCOLID];
		    }
		    break;
		}
    
		case '$':
                {
		    *sp++ = '$';
		    nextch();
		    if ( !CMnmstart(cp) )
			token = '$';
		    else
		    { /* check for "$ingres" */
			register char	*tmp = cp;

			/* Use a temporary input pointer in case it's not
			** "$ingres" and we have to push back the input.
			*/
			do
			{
				CMcpyinc(tmp, sp);
			} while ( CMalpha(tmp) );
			*sp = EOS;
			if ( STbcompare(symbuf, 0, "$ingres", 0, TRUE) != 0 )
			{ /* not $ingres */
				symbuf[1] = EOS;
				token = '$';
			}
			else
			{ /* $ingres */
				/* Update input pointer */
				cp = tmp;
				if ( *cp == EOS )
					cp = input_line();
				/* Set token */
				str = symbuf;
				token = osscankw[OSINGRES];
			}
		    }
		    break;
                }
                    
		case '^':
		{
		    static const DBLCH   caret[] = {
						{'=', OSNOTEQ},
						{'<', OSGTE},
						{'>', OSLTE},
						EOS
				    };
    
		    return _2char(caret);
		}
    
		case '!':
		{
		    static const DBLCH   exclaim[] = {{'=', OSNOTEQ}, EOS};
    
		    return _2char(exclaim);
		}
    
		case '<':
		{
		    static const DBLCH   less[] = {
						{'=', OSLTE},
						{'>', OSNOTEQ},
						EOS
				    };
    
		    return _2char(less);
		}
    
		case '>':
		{
		    static const DBLCH   greater[] = {{'=', OSGTE}, EOS};
    
		    return _2char(greater);
		}
    
		case '*':
		{
		    static const DBLCH   star[] = {{'*', OSEXP}, EOS};
    
		    return _2char(star);
		}
    
		case '(':
		{
		    static const DBLCH   lparen[] = {{'|', OSLSQBRK}, EOS};
    
# ifdef COUNTBLOCKTERM
		    if ( (token = _2char(lparen)) == '('
				&& end_section < begin_section )
			++parentheses;
		    return token;
# else
		    return _2char(lparen);
# endif
		}
    
		case '|':
		{
		    static const DBLCH   bar[] = {{')', OSRSQBRK}, EOS};
    
		    return _2char(bar);
		}
    
		case '[':
		    *sp++ = '[';
		    nextch();
		    token = osscankw[OSLSQBRK];
		    break;
    
		case ']':
		    *sp++ = ']';
		    nextch();
		    token = osscankw[OSRSQBRK];
		    break;
    
		case ')':
		    token = *cp;
		    copych(sp);
# ifdef COUNTBLOCKTERM
		    if ( end_section < begin_section )
			if ( --parentheses < 0 )
				yyerror("syntax error");
# endif
		    break;

		case '{':
		    token = *cp;
		    copych(sp);
# ifdef COUNTBLOCKTERM
		    if ( end_section < begin_section )
			++brackets;
# endif
		    break;
		case '}':
		    token = *cp;
		    copych(sp);
# ifdef COUNTBLOCKTERM
		    if ( end_section < begin_section )
			if ( --brackets < 0 )
				yyerror("syntax error");
# endif
		    break;

		default:
		    token = *cp;
		    copych(sp);
		    break;
	    } /* end else switch */
	} /* end token collection */
    } /* end for */
    *sp = EOS;

    if (str != NULL)
    {
	if ((yylval.st_name = iiIG_string(str)) == NULL)
	    osuerr(OSNOMEM, 0);
    }
    return token;
}

/*
** Name:    _hexvchar() -	Input Hexadecimal VARCHAR Constant String.
**
** Description:
**	This routine inputs a hexadecimal constant string of type VARCHAR,
**	delimited by "X'" and terminated by another single-quote, into the
**	symbol buffer and string table.  Only an even number of hexadecimal
**	digits are accepted.
**
** Returns:
**	{nat}  Hex VARCHAR string constant token number.
**
** History:
**	02/87 (jhw) -- Written.
*/
static i4
_hexvchar ()
{
    register char   *sp = symbuf + 1;
    register bool   over = FALSE;
    register bool   badch = FALSE;

    char	*iiIG_string();

    *sp++ = '\'';
    while (*cp != '\'')
    {
	if (*cp == '\n')
	{
	    oscerr(OSUNTERMSTR, 0);
	    break;
	}
	else if (!CMhex(cp))
	{
	    if (!badch)
	    {
		char    errb[2];

		badch = TRUE;
		errb[0] = *cp; errb[1] = EOS;
		oscerr(OSBADHEX, 1, errb);
	    }
	}
	else if (!over && !badch)
	{ /* hexadecimal digit */
	    if (sp >= &symbuf[sizeof(symbuf)-2])
		over = TRUE;
	    else
		*sp++ = *cp;
	}
	nextch();
    } /* end while */
    *sp = EOS;
    /*
    ** Enter string constants into the string table.
    */
    if ((yylval.st_name = iiIG_string(&symbuf[2])) == NULL)
	osuerr(OSNOMEM, 0);

    *sp++ = '\'';
    *sp = EOS;
    if (over)
	oscerr(OSLNGHEX, 1, symbuf);
    else if (!badch && (sp - 1 - symbuf) % 2 != 0)	/* not even */
	oscerr(OSBADHEXSTR, 1, symbuf);

    nextch();
    return osscankw[OSXCONST];
}

/*
** Name:    _varchar() -	Input VARCHAR Constant String.
**
** Description:
**	This routine inputs a constant string of type VARCHAR, delimited by
**	single-quotes, into the symbol buffer and string table.  The single-
**	quote delimiter can be escaped within the string by doubling.
**
** Returns:
**	{nat}  String constant token number.
**
** History:
**	02/87 (jhw) -- Modified from 'lex()' embedded code.
*/
static i4
_varchar ()
{
    register char   *sp = symbuf;
    register bool   over = FALSE;

    char	*iiIG_string();

    *sp++ = '\'';
    for (;;)
    {
	/*
	** If the string is too long, gather it with out adding
	** to the symbol buffer, then print out an error.
	*/
	if (*cp == '\n')
	{
	    oscerr(OSUNTERMSTR, 0);
	    break;
	}
	else if (*cp == '\'')
	{ /* possible delimiter */
	    nextch();
	    if (*cp != '\'')
		break;	/* end of string */
	    else if (!over)
	    { /* escaped '\'' */
		if (sp >= &symbuf[sizeof(symbuf)-3])
		    over = TRUE;
		else
		{
		    *sp++ = '\'';
		    *sp++ = '\'';
		}
	    }
	}
	else if (!over)
	{
	    if (sp >= &symbuf[sizeof(symbuf)-CMbytecnt(cp)-1])
		over = TRUE;
	    else
	    {
		CMcpychar(cp, sp);
		CMnext(sp);
	    }
	}
	nextch();
    } /* end for */
    *sp = EOS;
    /*
    ** Enter string constants into the string table.
    */
    if ((yylval.st_name = iiIG_string(&symbuf[1])) == NULL)
	osuerr(OSNOMEM, 0);

    *sp++ = '\'';
    *sp = EOS;
    if (over)
	oscerr(OSLNGSQUOTE, 1, symbuf);

    return osscankw[OSSCONST];
}

/*
** Name:    _text() -	Input TEXT Constant String.
**
** Description:
**	This routine inputs a constant string of type TEXT, delimited by
**	double-quotes, into the symbol buffer and string table.  The double-
**	quote delimiter can be escaped with a backslash (\.)
**
** Input:
**	dml	{nat}  DML type.
**
** Returns:
**	{nat}  String constant token number.
**
** History:
**	02/87 (jhw) -- Modified from 'lex()' embedded code.
**	05/87 (jhw) -- Added support for SQL.
*/

static i4
_text (dml)
i4	dml;
{
    register char   *sp = symbuf;
    bool	    over = FALSE;
    char	    transbuf[2*sizeof(symbuf)];
    char	    *str;
    i4		    token;

    char	*iiIG_string();

    if (dml == OSLSQL)
	oswarn(OSDBLQUOTE, 0);

    *sp++ = '"';
    while (*cp != '"')
    {
        /*
        ** If the string is too long, gather it with out
        ** adding to the buffer, then print out an error.
        */
        if (*cp == '\n')
        {
    	    oscerr(OSUNTERMSTR, 0);
    	    break;
        }
	else if (*cp == '\\')
    	{
	    nextch();
	    if (!over)
	    {
    	        if (sp >= &symbuf[sizeof(symbuf)-CMbytecnt(cp)-2])
		    over = TRUE;
		else
		{
		    *sp++ = '\\';
		    CMcpychar(cp, sp);
		    CMnext(sp);
		}
	    }
    	}
	else if (!over)
	{
            if (sp >= &symbuf[sizeof(symbuf)-CMbytecnt(cp)-1])
		over = TRUE;
	    else
	    {
		CMcpychar(cp, sp);
		CMnext(sp);
	    }
	}
	nextch();
    } /* end while */
    *sp = EOS;

    /*
    ** Translate for SQL.
    */
    if (dml != OSLSQL)
    { /* QUEL */
	token = osscankw[OSSCONST];
	str = &symbuf[1];
    }
    else
    { /* translate for SQL */
	register char	*cp;
	register char	*tp = transbuf;

	for (cp = &symbuf[1]; (cp = STindex(cp, ERx("\\"), 0)) != NULL; cp += 2)
	{
	    if (STindex(ERx("ntbrf01234567"), cp+1, 0) != NULL)
		break;
	}
	if (cp != NULL)
	{ /* translate to hexadecimal */
	    char   buf[sizeof(symbuf)];

	    str = transbuf;
	    token = osscankw[OSXCONST];
	    ostxtconst(FALSE, &symbuf[1], buf, sizeof(buf)-1);
	    for (cp = buf ; *cp != EOS ; ++cp)
	    {
		static char	hexmap[] = ERx("0123456789ABCDEF");

		*tp++ = hexmap[*cp / 0x10];
		*tp++ = hexmap[*cp % 0x10];
	    }
	    *tp = EOS;
	    if (tp - transbuf > sizeof(symbuf)-1)
		over = TRUE;
	}
	else if (STindex(&symbuf[1], ERx("\\"), 0) != NULL ||
			STindex(&symbuf[1], ERx("'"), 0) != NULL)
	{ /* translate to single-quote delimited string */
	    str = transbuf;
	    token = osscankw[OSSCONST];
	    for (cp = &symbuf[1] ; *cp != EOS ;)
	    {
		if (*cp == '\'')
		{ /* `escape' single-quote */
		    *tp++ = *cp;
		    *tp++ = *cp++;
		}
		else if (*cp == '\\')
		{ /* \x ==> x */
		    if (*++cp == '\\')
			*tp++ = *cp++;	/* special case for \ */
		}
		else
		    CMcpyinc(cp, tp);
	    }
	    *tp = EOS;
	    if (tp - transbuf > sizeof(symbuf)-1)
		over = TRUE;
	}
	else
	{ /* no translation required */
	    str = &symbuf[1];
	    token = osscankw[OSSCONST];
	}
    }

    /*
    ** Enter string constants into the string table.
    */
    if ((yylval.st_name = iiIG_string(str)) == NULL)
        osuerr(OSNOMEM, 0);

    *sp++ = '"';
    *sp = EOS;
    if (over)
        oscerr(OSLNGSTR, 1, symbuf);

    nextch();
    return token;
}

/*
** Name:    _2char() -	Scan for Two-Character Token.
**
** Description:
**	Determines whether the current character is part of a two-character
**	token for OSL.  This is done by comparing the second character against
**	a list of acceptable second characters and returning the appropriate
**	token.
**
** Input:
**	dblp	{DBLCH *}  Array of acceptable following characters and tokens.
**
** Return:
**	{nat}	The token for the whatever is recognized.
**
** History:
**	?/? (jrc) -- Written.
**	02/87 (jhw) -- Modified to use acceptable 2nd character array.
*/
static i4
_2char (dblp)
register DBLCH	*dblp;
{
    register char   *sp = symbuf;
    char	    firstch = *cp;
    i4		    token;

    token = firstch;
    copych(sp);
    do
    {
	if (*cp == dblp->_2ndch)
	{
	    copych(sp);
	    token = osscankw[dblp->sym];
	    break;
	}
    } while ((++dblp)->_2ndch != EOS);
    *sp = EOS;

    return token;
}

/*
** Name:    comment() -	Scan Comment.
**
** Description:
**	This routine scans an OSL comment line for the scanner, skipping it.
**	As a side effect, if the comment contains an embedded command, it is
**	processed.
**
** History:
**	10/86 (jhw) -- Created from in-line scanner ('lex()') code.
*/
static VOID	getword();
static VOID	errorcom();
static VOID	replacecom();

static VOID
comment ()
{
    i4	newlineno = -1;

    nextch();
    /*
    ** Check for special comment with command in it.
    */
    if (*cp == OSCOMCHAR)
    {
	char	w[OSBUFSIZE];

	nextch();
	getword(w);
	if ( !osUser && STequal(w, ERx("BEGIN")) )
	{
		nextch();
		getword(w);
		STcopy(w, section);
		begin_section = line;
	}
	else if ( !osUser && STequal(w, ERx("END")) )
	{
		end_section = line;

# ifdef COUNTBLOCKTERM
		if ( brackets > 0 || parentheses > 0 )
		{
			yyerror("syntax error");
		}
		brackets = parentheses = 0;
# endif
	}
	else if (STequal(w, ERx("ERROR")))
	{
	    errorcom();
	}
	else if (STequal(w, ERx("REPLACE")))
	{
	    /*
	    ** That is how many lines it takes.
	    */
	    replacecom();
	}
	else if (STequal(w, ERx("NOERRREP")))
	{
	    i4	OSerrbase;

	    nextch();
	    getword(w);
	    if (CVan(w, &OSerrbase) != OK)
		oscerr(OSBADLINE, 1, w, 0);
	    --line;
	    report_errors = FALSE;	/* turn off syntax error reporting */
	}
    }
    /*
    ** Find end of comment
    */
    for (;;)
    {
	if (*cp != '*')
	{
	    nextch();
	}
	else
	{ /* check for end */
	    nextch();
	    if (*cp == '/')
	    {
		nextch();
		break;	/* end of comment */
	    }
	}
	if (*cp == EOS)
	{
	    oscerr(OSCOMUNTERM, 0);
	    break;
	}
    } /* end for */

    /* Set line number */
    if (newlineno >= 0)
    {
	line = newlineno;
	osreprm(line - 1);
    }
}

/*
** Name:    getword() -	Get Word from Comment Embedded Command.
**
** Description:
**	This routine reads a word (delimited by white-space) from
**	the input.  It is used only for comment embedded commands.
**
** Output:
**	buf	{char *}  The buffer containing the word.
*/
static VOID
getword (buf)
register char	*buf;
{
    while (CMwhite(cp))
	nextch();
    do {
	copych(buf);
    } while (!CMwhite(cp));
    *buf = EOS;
}

/*
** Name:    errorcom() -	Get Error Text from Special Comment.
**
** Description:
**	Scan an error out of a special comment.
**	The comment line will contain:
**		ERROR err# numargs#	a1	a2 ..
**		data lines {These contain things to dump to the screen}
*/
static VOID
errorcom ()
{
    i4	    err;
    i4	    num;
    char    *p[6];
    char    buf[OSBUFSIZE];
    register char   *bp;
    register i4     i;

    nextch();
    getword(buf);
    if (CVan(buf, &err) != OK)
	oscerr(OSBADERR, 1, buf);
    getword(buf);
    if (CVan(buf, &num) != OK || num >= (sizeof(p)/sizeof(char *)))
    {
	oscerr(OSBADNUM, 1, buf);
	num = 0;
    }
    bp = buf;
    for (i = 0 ; i < num ; ++i)
    {
	p[i] = bp;
	while (*cp == '\t')
	    nextch();
	do {
	    copych(bp);
	} while (*cp != '\t');
	*bp++ = EOS;
    }
    oscerr(err, num, p[0], p[1], p[2], p[3], p[4], p[5]);
}

/*
** Name:    replacecom() -
**
**	A special replace comment was found.
*/
static VOID
replacecom ()
{
    i4		num;
    char	buf[OSBUFSIZE];

    VOID	osrepaddline();

    nextch();
    getword(buf);
    if (CVan(buf, &num) != OK)
	oscerr(OSBADERR, 1, buf);
    /*
    ** This assumes that the replace comment has a certain
    ** syntax which makes sure that the scanner has not already
    ** read the next line.  We do that here.
    */
    SIgetrec(buf, sizeof(buf)-1, osIfile);
    osrepaddline(buf, num);
}

/*
** OSL Lexical Scanner Internal Input Line Buffer Module.
**
**	Reads whole lines to make things faster.
**
**  returns zero (?) on EOF.
*/
typedef char	LINE_BUF[OSBUFSIZE];
static LINE_BUF	buf1 = {EOS};
static LINE_BUF	buf2 = {EOS};
static LINE_BUF	*last_line = (LINE_BUF *)buf2;
static LINE_BUF	*current_line = (LINE_BUF *)buf1;
static i4	scanpos = 0;
static char	*scanbuf = (char *)buf1;
static bool	printed = FALSE;

/*
** Name:    input_line() -	Read Next Input Line.
**
** Description:
**	Reads the next input line.
*/
static char *
input_line ()
{
    register char	*cp;

    do
    {
	cp = *last_line;
	last_line = current_line;
	current_line = (LINE_BUF *)cp;
	if ( !printed && osLisfile != NULL )
	{
		/* Note:  Use 'SIfprintf(fp, "%s", ...)' since the line
		** as input by the user may contain '%' characters.
		*/
		SIfprintf(osLisfile, ERx("%s"), *last_line);
	}
	if (SIgetrec(current_line, sizeof(*current_line), osIfile) != OK)
	    return ERx("");
	printed = FALSE;
	++line;
    } while (*cp == EOS);

    return cp;
}

/*
** Name:    markpos() -	Mark Current Symbol Position.
**
** Description:
**	Mark position in the current input line of symbol being scanned.
**
** Input:
**	pos	{char *}  Current position in line.
**
** Side Effects:
**	Sets 'scanpos' and 'scanbuf'.
*/
static VOID
markpos (pos)
char	*pos;
{
    scanpos = pos - *current_line;
    scanbuf = *current_line;
}

/*{
** osscnlno
*/
i4
osscnlno()
{
    return line;
}

/*{
** Name:	oserrline() -	Report Error/Warning Context.
**
** Description:
**	Prints the line for which an error or warning is to be reported
**	along with an indicator for where in the line the error or warning
**	was detected.  This indicator is simply a caret (^) printed under
**	the current location in the input line.
**
** Input:
**	fp		{FILE *}  The output file.
**	severity	{ER_MSGID}  The severity message.
**
** History:
**	07/89 (jhw) -- Added severity message output.
*/
VOID
oserrline ( fp, severity )
register FILE	*fp;
ER_MSGID	severity;
{
	register i4	count;
	register char	*outline;
	bool		print_listing;
	bool		known_section;

	char	*osrepfindline();

	print_listing = (osLisfile != NULL && osLisfile != fp);
	if ( (outline = osrepfindline(scanbuf == *current_line ? line : line-1))
			!= NULL )
	{
		SIfprintf(fp, outline);
		count = STlength(outline);
	}
	else
	{
		/* Note:  Use 'SIfprintf(fp, "%s", ...)' since the line
		** as input by the user may contain '%' characters.
		*/
		if (scanbuf != *current_line)
		{
			outline = *last_line;
			if (osLisfile == NULL || print_listing)
				SIfprintf(fp, ERx("%s"), *last_line);
		}
		else
		{
			outline = *current_line;
			if (osLisfile == NULL || !printed)
			{
				if (print_listing)
				{
					SIfprintf( osLisfile, ERx("%s"),
							*current_line
					);
				}
				SIfprintf(fp, ERx("%s"), *current_line);
				printed = TRUE;
			}
		}
		count = scanpos;
	}
	{	/* Print pointer ('^') to current position */
		LINE_BUF	buf;
		register char	*bufp = buf;
		register char	*cp;

		for (cp = outline ; count > 0 ; --count)
			*bufp++ = (*cp++ == '\t' ? '\t' : ' ');
		*bufp++ = '^';
		*bufp++ = '\n';
		*bufp++ = EOS;
		if (print_listing)
			SIfprintf(osLisfile, buf);
		SIfprintf(fp, buf);
	}
	SIputc('\n', fp);

	/* 
	** If we have a "-g" listing, and a section name, output the
	** section name.
	*/
	if (!osUser)
	{
		known_section = TRUE;

		/* We need a different message number */
		switch(severity)
		{
		    case _ErrorSeverity:
			severity = _SecErrorSeverity;
			break;

		    case _WarningSeverity:
			severity = _SecWarningSeverity;
			break;

		    case _FatalSeverity:
			severity = _SecFatalSeverity;
			break;

		    default:
			/* 
			** We don't know the message number which prints 
			** section names for this type of error.
			*/
			known_section = FALSE;
			break;
		}
	}
	else
	{
		known_section = FALSE;
	}
	if (known_section)
		SIfprintf(fp, ERget(severity), 
		(*section != EOS) ? section : ERget(F_OS0019_NoSection),
		line - begin_section);
	else
		SIfprintf(fp, ERget(severity), line);
}

/*{
** Name:    yyerror() -	Report Syntax From OSL Parser.
*/
static i4	serrcnt = 0;

VOID
yyerror (s)
char	*s;
{
    if (*s == 'y')	/* "yacc ..." */
	SIfprintf(stderr, s);
    else if (report_errors)
    {
	if (*s == 's')	/* "syntax error" */
	    if (*cp == EOS)
		oscerr(OSUNXEOF, 1, (PTR)&line);
	    else
		oscerr(OSSYNTAX, 2, (PTR)&line, oslastsym);
	else
	    oscerr(OSESYNTAX, 3, (PTR)&line, oslastsym, s);
    }
    ++serrcnt;
}

/*{
** Name:    osSerrcnt() -	Return the number of syntax errors
**
** Description:
**	Returns the number of syntax errors encountered in this file.
**	Simply returns the value in serrcnt.
**
** Returns:
**	{nat}  The number of syntax errors.
**
** History:
**	1-jul-1986 (joe)
**		First Written
*/
osSerrcnt()
{
    return serrcnt;
}
