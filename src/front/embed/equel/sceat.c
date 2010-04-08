# include	<compat.h>
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<er.h>
# include	<si.h>
# include	<cm.h>
# include	<eqscan.h>
# include	<equel.h>
# include	<eqsym.h>
# include	<eqgr.h>
# include	<ereq.h>
# include	<eqrun.h>
# include	<eqstmt.h>

/*
+* Filename:	SCEAT.C
** Purpose:	Eat up text from the input file, and pass to a function.
**
** Defines:	
**		sc_eat()	- Nasty text chomper.
**		sc_tokeat()	- Nasty token chomper.
** Locals:	
**		sc_oneof()	- Is an input char in the terminating set ?
**
** Side Effects:
**		Eats up data out of SC_PTR so grammar rules may not be
-*		so obvious after they have called this function.
** Notes:
** 1. The scanner must be synched up with the grammar rule that calls this.
**    This routine must know that there are no tokens in the grammar look-ahead
**    area, otherwise the initial state is corrupt.  Characters like a left
**    bracket and a left paren are easy to force into a rule by themselves, that
**    do not require look-ahead.
** 2. The data being eaten is buffered upto SC_LINEMAX.
**
** History:	10-dec-1984	Rewritten. (ncg)
**		26-may-1987	Modified for new scanner. (ncg)
**		29-jun-87 (bab)	Code cleanup.
**		03-may-88 	Added sc_tokeat, modified sc_eat (whc)
**		15-feb-94 (lan)
**		    Fixed bug #56528 - On CREATE PROCEDURE, delimiters are not
**		    preserved by the preprocessor on parameter names.  Fix in
**		    sc_tokeat.
**              12-Dec-95 (fanra01)
**                  Modified yylval from extern to GLOBALREF.
**		13-mar-96 (thoda04)
**		    Corrected return type of sc_eat()'s storfunc parameter 
**		    Upgraded sc_eat()'s function def to ANSI C style to avoid new/old mix. 
**		    since all functions (gen_code() and id_add()) are return type void.
**		    Added eqstmt.h header for db_ function prototype checking.
**
** Copyright (c) 2004 Ingres Corporation
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	21-aug-01 (inkdo01)
**	    Added 2nd left nested token parm to sc_tokeat to allow "begin" or "case"
**	    to incorporate nested "end" in procedure definition.
**      13-feb-03 (chash01) x-integrate change#461908
**         In sc_tokeat()
**         compiler complains about uninitialized variable sc_token,
**         set it to 0.     
*/

# define	BUF_SIZE	(3*SC_LINEMAX+2+1)

static bool	sc_oneof(char ch, char *str);

/*
+* Procedure:	sc_eat
** Purpose:	Eat up text, lexical components at a time.
**
** Parameters:	storfunc  - i4  (*)()	- Requires a string as its argument.
**		flag 	  - i4		- SC_NEST  - Allow nested constructs.
**		       			  SC_STRIP - Strip off delimiters.
**		       			  SC_BACK  - Backup rightmost character.
**		term_str  - char *	- String of termination characters.
**					  Each one must be 1-char operator.
**		l_nest_ch - char	- Left nesting character.
**		r_nest_ch - char	- Right nesting character.
-* Returns: 	None
*/

void
sc_eat( 
void	(*storfunc)(),
i4	flag,
char	*term_str,
char	l_nest_ch,
char	r_nest_ch)
{
    register i4	level = 0;
    register i4	len = 0;
    register i4	sc_token;	/* Token type of yylval */
    char		buf[BUF_SIZE];	/* Storage for input string */
    register char	*cp = buf;	/* Points to next free spot in buffer */
    register char	ch;		/* Current input char */
    GLOBALREF GR_YYSTYPE	yylval;
    i4			saveline = eq->eq_line;	/* Line num on entry */
    
    if (flag & SC_SEEN)
    {
	if (flag & SC_NEST)
	    level++;
	if ((flag & SC_STRIP) == 0)		/* Include limits */
	    *cp++ = l_nest_ch;
    }

    /*
    ** Get a token from yylval:
    ** - If the token is an operator inspect it
    **   to see if it one of the "terminating or nesting" characters.
    ** - The EOF token is returned and handled as a special case.
    */
    for (;;)
    {
	*cp = '\0';			/* Null terminate current buffer */

	sc_token = (*dml->dm_lex)(SC_RAW);

	if (sc_token == SC_EOF)		/* Must be EOF */
	{
	    char	ebuf[21];

	    STlcopy(buf, ebuf, 20);
	    er_write( E_EQ0218_scMISSRIGHT, EQ_ERROR, 3, term_str, 
		    er_na(saveline), ebuf );
	    break;
	} 

	if (CMoper(yylval.s))
	{
	    /*
	    ** The token is an operator.
	    ** Check the character against the "nesting characters" to update
	    ** the "nesting level" numbers.
	    ** Then test the character against the set of "terminating
	    ** characters" with no nesting level.  If it is a "terminator"
	    ** then based on the input flag, either add it or leave it, but
	    ** always return.
	    */
	    ch = *(yylval.s);
	    if (flag & SC_NEST)		/* Update levels */
	    {
		if (ch == l_nest_ch)
		    level++;
		else if ((ch == r_nest_ch) && (level > 0))
		    level--;
	    }

	    if (level == 0 && sc_oneof(ch, term_str))	/* Terminator */
	    {
		/*
		** Do we want the actual token in the string? In most cases
		** if we used SC_BACK then we never used SC_STRIP too (ie,
		** eat till a semicolon, but don't add it to the buffer as my 
		** grammar needs the semicolon to reduce, and that's where I
		** spit it out).
		** That is, SC_BACK means push back the right delimiter AND
		** don't add it!  It implies nothing for the left delimiter.
		*/
		if (flag & SC_BACK)	/* Want to see the token again ? */
		    *--SC_PTR = ch;
		else if ((flag & SC_STRIP) == 0)
		{
		    *cp++ = ch;
		    *cp = '\0';
		}
		break;
	    } /* level is zero and terminator */

	    /*
	    ** This character is not a terminator and must be added to the
	    ** buffer, so fall out and add the character.
	    */
	}

	/*
	** Fallen out of 'if' - we must add the current token to buffer.
	** If the buffer is too small, then send what we have, and add it
	*/
	len = STlength( yylval.s );
	if (cp + len >= buf + BUF_SIZE)
	{
	    if (storfunc)
		(*storfunc)(buf);
	    cp = buf;
	}
	STcopy(yylval.s, cp);
	cp += len;
    } /* for getting tokens */

    if (*buf && storfunc)
	(*storfunc)(buf);
}

/*
+* Procedure:	sc_oneof
** Purpose:	Is a character one of a set of terminating characters.
**
** Parameters:	ch  - char	- Input character.
**		str - char *	- Set of characters.
-* Returns:	bool 		- TRUE if the character was found.
*/

static bool
sc_oneof(register char ch, register char *str )
{
    while (*str)
    {
	if (ch == *str++)
	    return TRUE;
    }
    return FALSE;
}

/*
+* Procedure:	sc_tokeat
** Purpose:	Eat up tokens
**
** Parameters:	flag 	  - i4		- SC_NEST  - Allow nested constructs.
**					- SC_SEEN - already seen leftmost delim.
**		term_tok  - i4		- terminating token.
**		l1_nest_tok - i4	- 1st left nesting token.
**		l2_nest_tok - i4	- 2nd left nesting token.
**		r_nest_tok - i4	- Right nesting token.
-* Returns: 	None
**
** History:
**     13-feb-2003 (chash01)
**         compiler complains about uninitialized variable sc_token,
**         set it to 0.     
*/

void
sc_tokeat( flag, term_tok, l1_nest_tok, l2_nest_tok, r_nest_tok )
i4	flag;
i4	term_tok;
i4	l1_nest_tok;
i4	l2_nest_tok;
i4	r_nest_tok;
{
    i4			level = 0;
    i4			sc_token = 0;	/* Token type of yylval */
    GLOBALREF GR_YYSTYPE	yylval;
    i4			saveline = eq->eq_line;	/* Line num on entry */
    i4			len = 0;
    char 		*ostr = NULL;
    
    /* Have we already seen the leftmost token in a nesting set? */
    if (flag & SC_SEEN)
	level++;

    /* keep going until we see a non-nested terminator */
    while ( !(level == 0 && sc_token == term_tok))
    {
	sc_token = (*dml->dm_lex)(SC_RARE);
	ostr = yylval.s;

	if (sc_token == SC_EOF)		
	{
	    /* uh-oh, we hit EOF without finding the right-hand delimitor. */
	    er_write( E_EQ0231_scMISSEND, EQ_ERROR, 1, er_na(saveline) );
	    break;
	} 

	if (flag & SC_NEST)
	{
	    /* Update nesting-level */
	    if (sc_token == l1_nest_tok || sc_token == l2_nest_tok)
		level++;
	    else if ((sc_token == r_nest_tok) && (level > 0))
		level--;
	}

	/* handle embedded strings */
	if (sc_token == tok_special[TOK_STRING])
	{
	    db_sconst( ostr ); 
	    continue;
	}

	/* Fix bug #56528 - preserve delimiters */
	if (sc_token == tok_special[TOK_DELIMID])
	{
	    db_delimid( ostr );
	    continue;
	}

	/* write it out. The right-hand delimitor always gets written, too. */

	if (CMcntrl(ostr))
	{
	    switch (*ostr)
	    {
	      case '\n':
	      case '\r':
	      case '\f':
		db_linesend();
		if (*ostr == '\f')
		    db_linesend();
		ostr = NULL;
		len = 0;
		break;

	      case '\t':
		/* create enough blanks to get to the next tabstop. */
		ostr = ERx("        ");
		ostr += ( len % 8 );
		break;

	      default:
		/* ignore all other control chars. */
		break;
	    }
	}

	if (ostr != NULL)
	{
	    len += STlength( ostr );

	    db_op(ostr);
	}
    } /* while */

    /* send it all out now, with a final newline. */
    db_linesend();
}
