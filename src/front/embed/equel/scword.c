/*
** Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<er.h>
# include	<si.h>
# include	<cm.h>
# include	<equtils.h>
# include	<eqscan.h>
# include	<equel.h>
# include	<eqsym.h>
# include	<eqgr.h>
# include	<eqlang.h>
# include	<ereq.h>

/*
+* Filename:	SCWORD.C  
** Purpose:	Process an identifier or keyword token.
**
** Defines:	
**		sc_word()		- Process a word.
**		sc_deref()		- Process a dereferenced '#' word.
** Locals:
**		scan_doub()		- Look ahead for (unreserved) dbl words
**
** An alphabetic character has been detected. Process the following word as
** a user defined name or as predeclared word.  User defined names are up to
-* a maximum of SC_WORDMAX.
**
** Notes:
**    1. Special Cases: Always look ahead for unreserved double keys.
**    2. Make sure SC_WORDMAX is greater than the maximum keyword size (usually
**	 this will be the case).
**    3. The tokens table tok_keytab was split into two tables for ESQL;
**       tok_keytab for master keywords, and tok_skeytab for slave keywords.
**       EQUEL uses only tok_keytab for its keywords, however, tok_skeytab and
**       tok_skynum are still need to be defined here as sc_word references 
**	 them.
**    4. We now allow host variables to be reserved words.  To facilitate this
**	 we have introduced a new global, sc_hostvar.  If the grammer detects
**	 that we're scanning a hostvar, it will set sc_hostvar to TRUE.  If
**	 sc_hostvar is TRUE, we suppres keyword lookups and just return
**	 a tNAME.  One exception to this is the COBOL keywords IN and OF,
**	 which are used in referencing structure members (See below).
**
** Side Effects:
**    1. Adds a token to the string space if its not a keyword.
**
** History:
**	    21-nov-1984	- Rewritten. (ncg)
**	    19-may-1987	- Modified to use new scanner. (ncg)
**	    28-jul-1992	- Added check for new global sc_hostvar.  If set,
**			  keyword lookups are supressed. (larrym)
**	    30-jul-1992 - added support for separate master/slave keyword
**			  lookup tables.  When in EXEC mode, tok_keytab is
**			  used; when in DECL mode, tok_skeytab is used.
**			  (larrym)
**	02-oct-1992 (larrym)
**		Changed Copywrite notice to conform to standard (also using
**		new history comment format).
**		Modified the way we use sc_hostvar in this file.  We used
**		to reset after every word; now we let the grammer control
**		it.  Also, added nasty hack below for always doing a keyword
**		lookup (regardless of sc_hostvar) if we're in COBOL and the
**		the scanned word is IN or OF, since these tokens are actually
**		a part of a COBOL host variable reference (other langs use
**		only tNAMES and operators).
**	05-oct-1992  (kathryn)
**	    Correct usage of YYDEBUG -- YYDEBUG is always defined in eqgr.h.
**	    The extern i4  yydebug, declared in IYACC (Ingres YACC) should
**	    be used to determine whether or not debugging is on.  Note that
**	    yydebug is turned on via yy_dodebug().
**	20-nov-1992 (Mike S)
**	    For EXEC 4GL, check 4gl-specific keyword table first.
**      12-Dec-95 (fanra01)
**          Modfied all externs to GLOBALREFs
**	11-Jun-96 (abowler)
**	    Bug 76736: Fix for NIST - a previous "nasty hack" was
**	    added to support use of "in" as a structure element
**	    indicator. Minor fix so that if we hit "...in (..."
**	    we always treat the "in" as an SQL "in". 
**	4-oct-01 (inkdo01)
**	    Set yyreswd for keywords to enable parser reserved word retry 
**	    logic.
**	07-Nov-05 (toumi01) SIR 115505
**	    Support hex constants in format 0x696E67636F7270 as well as the
**	    traditional X'6279656279654341' to aid application portability.
**	7-dec-05 (inkdo01)
**	    Add support for Unicode literals (SIR 115589).
**	 8-jan-06 (hayke02 for toumi01)
**	    Add support for first/top look ahead. If we don't find a digit
**	    after first/top, then treat it as a user defined name (column
**	    name or schema owner name) e.g. 'select first into :varname ...',
**	    'select top.colname into :varname ...'. This change fixes bug
**	    117404.
**	 9-jan-07 (hayke02)
**	    Modify the previous change so that the STbcompare() length
**	    specifiers are 0 rather than 5 (for first) and 3 (for top).
**	    This forces STbcompare() to fail for strings of unequal length,
**	    thus preventing "top" being matched with "to".
**	3-aug-07 (toumi01)
**	    Hack the FIRST N hack to look for FIRST FROM for scrollable
**	    cursors. Liberal use of gotos is an editorial comment on how
**	    much I despise trying to work around ANSI keyword limits.
**	15-aug-2007 (dougi)
**	    Add support for date/time literals.
*/

# ifdef YYDEBUG
    GLOBALREF i4   yydebug;
# endif /* YYDEBUG */


/* Special routine to scan ahead for double keys */
i4		scan_doub();

/* This variable is defined in eqglob.c.  It is set by esqyylex to tell scword 
** that we are expecting a colon and not to do keyword lookups.
*/
GLOBALREF i4       sc_hostvar;             /* FALSE means do keyword lookup */
GLOBALREF bool	    yyreswd;		/* for parser reserved word retry */

static char	*dtlits[] = {"date", "time", "timestamp"};

/*
+* Procedure:	sc_word
** Purpose:	Process a word when a character has been recognized.
**
** Parameters:	mode 	- i4	     - level of processing
** Returns:	i4	- TOK_NAME    - User-defined name.
**			- TOK_DEREF   - Dereferenced word.
**			- Other	- Lexical codes for keywords,
-*			   	- Grammar synchronized return values.
** Notes:
** 1. This routine assumes that it is being called with SC_PTR pointing
**    at a the first character that can begin a name (CMnmstart).
** 2. When leaving this routine, SC_PTR is pointing at first non-name-char.
*/

i4
sc_word(mode )
i4	mode;					/* TRUE iff from sc_eat */
{
    register char  *cp, *sp;
    char	   wbuf[ SC_WORDMAX + 2 ];	/* Word buffer */
    KEY_ELM	   *key;			/* Keyword to return */
    i4		   minus;			/* For Cobol */
    i4		   lex_usrval;			/* To return for user names */
    i4		   doub_tok;			/* To return double toks */
    i4	           ignore_keywords = sc_hostvar; /* make a local copy */

    /* Check if this a hex constant (x followed by a quote) */
    if (   (*SC_PTR == 'x' || *SC_PTR == 'X')
	&& (   (*(SC_PTR+1) == '"'  && dml_islang(DML_EQUEL))
	    || (*(SC_PTR+1) == '\'' && dml_islang(DML_ESQL))
	   )
       )
    {
	return sc_hexconst();
    }

    /* Check if this a hex constant (0 followed by an x) */
    if (   (*SC_PTR == '0') &&
	   (*(SC_PTR+1) == 'X' || (*(SC_PTR+1) == 'x'))
       )
    {
	return sc_hexconst();
    }

    /* Check if this a Unicode constant (U&' ... '). */
    if (   (*SC_PTR == 'u' || *SC_PTR == 'U')
	&& *(SC_PTR+1) == '&'  && dml_islang(DML_ESQL)
	&& *(SC_PTR+2) == '\'')
    {
	return sc_uconst();
    }

    sp = SC_PTR;		/* save start of token */

    /* Cobol defined names, and some Cobol keywords have '-' embedded */
    minus = eq_islang( EQ_COBOL );
    for (cp = wbuf; (cp - wbuf < SC_WORDMAX) &&
		    (CMnmchar(SC_PTR) || (minus && *SC_PTR=='-')); )
    {
	CMcpyinc(SC_PTR, cp);
    }
    *cp = '\0';

# ifdef YYDEBUG
    if (yydebug)
        SIprintf("wbuf is %s, HV=%d, SC_PTR=%d\n",wbuf,ignore_keywords,SC_PTR);
# endif
    /* 
    ** Dropped out of loop, either because SC_PTR not Alpha or maybe too big. 
    ** (Too big if we've filled buffer to SC_WORDMAX and we just got an 
    ** extra alpha.)
    */
    if (   (cp - wbuf >= SC_WORDMAX)
	&& (CMnmchar(SC_PTR) || (minus && *SC_PTR == '-')))
    {
	/* Error, chomp to end of identifier, return only first part */
	er_write( E_EQ0216_scWRDLONG, EQ_ERROR, 2, er_na(SC_WORDMAX), wbuf );
	while (CMnmchar(SC_PTR) || (minus && *SC_PTR == '-'))
	    CMnext(SC_PTR);
	/* SC_PTR character is not Alpha */
	sc_addtok( SC_YYSTR, str_add( STRNULL, wbuf) );
	/*
	** See below for use of GR_LOOKUP - do it here so as not to cascade
	** errors - and for what we return for raw mode.
	*/
	lex_usrval = tok_special[ TOK_NAME ];
	if (mode == SC_COOKED)
	    gr_mechanism( GR_LOOKUP, wbuf, &lex_usrval );
	return lex_usrval;
    }
    /* Within word size and last character is not part of word */

    /* Check here for date/time literal delimiters - name followed by
    ** quote triggers loop to match one of the literal types. If they
    ** match, call sc_dtlit() to finish the job. */
    if (dml_islang(DML_ESQL) && *SC_PTR == '\'')
    {
	i4	i;

	for (i = 0; i < 3; i++)
	 if (STbcompare(wbuf, 0, dtlits[i], 0, TRUE) == 0)
	    break;			/* got a match */

	if (i < 3)
	    return sc_dtconst(wbuf, (SC_PTR - sp));
    }

    /* Test for unreserved double keywords (unless we're in a hostvar) */
    if ( (!ignore_keywords)
       && (doub_tok = scan_doub(wbuf, mode)) != 0)
	return doub_tok;

    /* 
    ** Nasty hack:  Cobol allows the reserved words IN | OF in its variable
    ** usage,e.g.,  if ELM is an element of record REC, it can be referenced
    ** either as REC.ELM or ELM IN REC or ELM OF REC.  So if the lang is 
    ** COBOL and we're in a hostvar, and the word is either IN or OF, we
    ** need to return the correct token, not just a tNAME.  So we set
    ** ignore_keywords to FALSE for this word (it's a local, remember?)
    ** and the proper token will be returned.  The global sc_hostvar will
    ** remain set.   
    */
    if (eq_islang( EQ_COBOL ) && ignore_keywords) 
    {
	/* 
	** check for "IN" or "OF" (case in-sensitive).  If so, turn ON 
	** keyword lookups FOR THIS WORD ONLY.
	*/
	if (  (STbcompare(wbuf,(i4)0,ERx("in"),(i4)0,TRUE)==0)
	   || (STbcompare(wbuf,(i4)0,ERx("of"),(i4)0,TRUE)==0) )
        {
	    /* Bug 76736. If we are COBOL and hit an "in"
	    ** followed by a '(', we should assume that this is
	    ** in fact an SQL "in" predicate rather than a Cobol
	    ** structure reference "in". Where there is no left
	    ** bracket, there is an ambiguity, so we'll go with
	    ** what was there before - a Cobol "in".
	    */
	    char* bp = SC_PTR;
	    while (CMwhite(bp))
		CMnext(bp);
	    if (*bp != '(')
	        ignore_keywords = FALSE;
		    
# ifdef YYDEBUG
    if (yydebug)
	SIprintf("scword: found IN or OF\n");
# endif
	}
    }
    /* Test for an Equel/ESQL keyword */
    if (!ignore_keywords)  /* if we're not scanning a hostvar */
    {
       if ((dml_islang(DML_ESQL)) 
	  && (dml->dm_exec == DML_DECL)) /* which keywords to use? */
       {
	  /* Language is SQL and we're in DECL mode, use slave keywords */
          key = sc_getkey(tok_skeytab, tok_skynum, wbuf); 
       }
       else 
       {
	  /* 
	  ** Either language is QUEL, or it's SQL and we're not in DECL mode.
	  ** If we're in EXEC 4GL, first use 4GL-specific keyword table.
	  ** Otherwise go straight to master keyword table.
	  */
	  key = KEYNULL;
	  if (dml->dm_exec & DML__4GL)
	      key = sc_getkey(tok_4glkeytab, tok_4glkynum, wbuf);
	  if (key == KEYNULL)
	      key = sc_getkey(tok_keytab, tok_kynum, wbuf); 
       }
       if (key != KEYNULL)
       {
	char *keyptr;

	if (mode == SC_COOKED)
	    keyptr = key->key_term;	/* Regular keyword */
	else
	    keyptr = str_add( STRNULL, wbuf);
	/*
	** if FIRST FROM (cursor fetch) then FIRST is a keyword
	** else if FIRST N then FIRST is a keyword
	** else FIRST is a user name
	**
	** This sort of scan ahead hack is a good argument for NOT
	** trying to work around the limits of ANSI keywords! :-P
	*/
	if ((STbcompare("first", 0, wbuf, 0, TRUE) == 0)
	    ||
	    (STbcompare("top", 0, wbuf, 0, TRUE) == 0))
	{
	    char	*lookahead = SC_PTR;
	    char	*look4from;
	    while (CMwhite(lookahead))
		CMnext(lookahead);
	    if (STbcompare("first", 0, wbuf, 0, TRUE) == 0)
	    {
	        look4from = lookahead;
		if (*look4from != 'F' && *look4from != 'f') goto notfrom;
		CMnext(look4from);
		if (*look4from != 'R' && *look4from != 'r') goto notfrom;
		CMnext(look4from);
		if (*look4from != 'O' && *look4from != 'o') goto notfrom;
		CMnext(look4from);
		if (*look4from != 'M' && *look4from != 'm') goto notfrom;
		goto notusername;
	    }
notfrom:
	    if (!CMdigit(lookahead))
		goto username;
	}
notusername:
	sc_addtok( SC_YYSTR, keyptr );
	yyreswd = TRUE;
	return key->key_tok;
       } 
    } 
    /* User-defined name */
username:
    sc_addtok( SC_YYSTR, str_add( STRNULL, wbuf) );

    /* 
    ** If C or Pascal we have to be able to parse structure members correctly
    ** without causing a conflict between the language membership '.' and
    ** the Ingres range '.'  In other languages we must be able to return
    ** the special token if names are allowed at the start of declarations.
    ** Ask the grammar what this is, based on local information it has.
    */
    lex_usrval = tok_special[ TOK_NAME ];
    if (mode == SC_COOKED)
	gr_mechanism( GR_LOOKUP, wbuf, &lex_usrval );
    return lex_usrval;
}


/*
+* Procedure:	scan_doub
** Purpose:	Scan for double keywords.
** Parameters:
**	usrkey	- *char	- Word/name already scanned.
**	mode  - i4  - level of cookedness.
** Return Values: Token number of double key, or zero if not a double key.
-*	
** Notes:
** 1.	All words/names scanned by sc_word will go through this routine.
** 2.   A scanned word (usrkey) is passed in and a small ordered table of the 
** 	first-of-double words (tok_1st_doubk) is linearly searched for a 
** 	match.  (Note: if at any time this table gets large, we should search 
**	it more efficiently.)  If there is no match, zero is returned.
** 3.   If a match between usrkey and tok_1st_doubk is found, scan_doub
**	scans off the next word/name from the input.  Using index info
**	from tok_1st_doubk, it searches a portion of a table of legal 
**	second-of-double words (tok_2nd_doubk).  If there is no match,
**	zero is returned.
** 4.   If a match between the scanned second word and tok_2nd_doubk is
**	found, the double word string is pushed on the parser's stack
**	and the appropriate token is returned.
**
** Imports modified:
**	Scanner's position (SC_PTR) in input file may be changed.
*/

i4
scan_doub( usrkey, mode )
char 	*usrkey;		/* Name already scanned */
i4  	mode;
{
    ESC_1DBLKEY	*sc_1k;		/* Pointer to tok_1st_doubk */
    ESC_2DBLKEY *sc_2k;		/* Pointer to tok_2nd_doubk */
    ESC_2DBLKEY *stop;		/* Marker */
    i4		cmp;	/* Result of word comparison */
    char	secwdbuf[SC_WORDMAX +2];  /* Buf for scanning 2nd word */
    char	*cp; 		/* Pointer for scanning second word */
    char	*loc_sc_ptr;	/* Local SC_PTR because we may not move */
    
    /* 
    ** Search ordered table of first of double words until we have gone 
    ** beyond usrkey or we fall off the end. 
    */
    for (sc_1k = tok_1st_doubk; sc_1k->name != (char *)0; sc_1k++)
    {
	if ((cmp = STbcompare(usrkey, 0, sc_1k->name, 0, TRUE)) <= 0)
	    break;
    }
    if (cmp < 0 || sc_1k->name == (char *)0)	/* No match? */
	return 0;

    /*
    ** First word matched.  Save scanner spot, and now scan second word,
    ** skipping inteverning white space. If we don't drop out at a name
    ** return.
    */

    loc_sc_ptr = SC_PTR;
    while (CMspace(loc_sc_ptr) || *loc_sc_ptr == '\t')
	CMnext(loc_sc_ptr);

    if (!CMalpha(loc_sc_ptr))		/* Reserved words begin with Alpha */
	return 0;

    for (cp = secwdbuf; (cp - secwdbuf < SC_WORDMAX) && CMnmchar(loc_sc_ptr); )
	CMcpyinc(loc_sc_ptr, cp);
    *cp = '\0';

    /* 
    ** Fell out of loop because complete word scanned? 
    */
    if (cp - secwdbuf < SC_WORDMAX)
    {
	/* 
	** Compare word scanned to table of second of double words 
	*/
    	stop = tok_2nd_doubk + sc_1k->upper;
        for (sc_2k = tok_2nd_doubk + sc_1k->lower; sc_2k <= stop; sc_2k++)
	{
	    if ((cmp = STbcompare(secwdbuf, 0, sc_2k->name, 0, TRUE)) <= 0)
		break;
	}
	if (sc_2k <= stop && cmp == 0)		/* Matched second word? */
	{
	    if (mode == SC_COOKED)
	    {
		sc_addtok( SC_YYSTR, sc_2k->dblname );	/* Stack double words */
	    }
	    else
	    {
		char tbuf[ (SC_WORDMAX * 2) + 2];

		STprintf(tbuf, ERx("%s %s"), usrkey, secwdbuf);
		sc_addtok( SC_YYSTR, str_add( STRNULL, tbuf) );
	    }
	    SC_PTR = loc_sc_ptr;			/* Advance SC_PTR */
	    return sc_2k->tok;
	}
    }
    return 0;						/* Leave SC_PTR */
}


/*
+* Procedure:	sc_deref 
** Purpose:	Scan off a word after detecting a '#'.
**
** Parameters:	None
-* Returns:	i4 - Lexical token for a Name.
**
** Requires that the SC_PTR is the one after the '#'.
*/

i4
sc_deref()
{
    register char	*cp;			/* Buffer */
    register i4	nerrs = 0;		/* Count of junk after '#' */
    char		wbuf[ SC_WORDMAX +1 ]; 	/* Look ahead buffer */
    i4		   	minus;			/* For Cobol */

    /* Skip spaces */
    while (CMspace(SC_PTR) || *SC_PTR == '\t')
	CMnext(SC_PTR);

    /* Skip all words till name - No comments allowed here */
    while (!CMnmstart(SC_PTR) && *SC_PTR != '\n')
    {
	CMnext(SC_PTR);		/* Something between # and name and not space */
	nerrs++;
    }

    if (nerrs)
	er_write( E_EQ0202_scBADDEREF, EQ_ERROR, 0 );

    if (CMnmstart(SC_PTR))	/* Not a newline */
    {
	/* Cobol defined names may have '-' embedded */
	minus = eq_islang( EQ_COBOL );
	for (cp = wbuf; (cp - wbuf < SC_WORDMAX) &&
			(CMnmchar(SC_PTR) || (minus && *SC_PTR=='-')); )
	{
	    CMcpyinc(SC_PTR, cp);
	}
	*cp = '\0';
	/* Dropped out of loop, either because of not Alpha or too big */
	if (   (cp - wbuf >= SC_WORDMAX)
	    && (CMnmchar(SC_PTR) || (minus && *SC_PTR == '-')))
	{
	    /* Save first part, chomp till end of word */
	    er_write( E_EQ0216_scWRDLONG, EQ_ERROR, 2, er_na(SC_WORDMAX), wbuf );
	    while (CMnmchar(SC_PTR) || (minus && *SC_PTR == '-'))
		CMnext(SC_PTR);
	}
    }
    else			/* SC_PTR is non-alphabetic character */
    {
	er_write( E_EQ0203_scDEREF, EQ_ERROR, 0 );
	STcopy( ERx("IIerr"), wbuf );
    }
    sc_addtok( SC_YYSTR, str_add( STRNULL, wbuf) );
    return tok_special[ TOK_DEREF ];
}
