# include	<compat.h>
# include	<er.h>
# include	<si.h>
# include	<cm.h>
# include	<equtils.h>
# include	<eqscan.h>
# include	<equel.h>
# include	<eqlang.h>
# include	<eqsym.h>		/* For eqgr.h */
# include	<eqgr.h>
# include	<ereq.h>

/*
+* Filename:	SCOPERATR.C 
**
** Procedure:	sc_operator
** Purpose:	Process a token starting with an operator.
**
** Parameters:	mode - level of processing.
** Returns:	i4    - Lexical token of what was found, or SC_CONTINUE 
**			  on error.
**
** An operator has been read. Process the operator and return the corresponding
** lexical value.  In some cases calls lower level scanner routines to process
-* different objects.
**
** Side Effects:
**	1. May strip comments from program if comment operator detected.
**
** Notes:
**      1. Special Cases.
**	1.1. The dereferencing character calls for the next word to be eaten.
**	1.2. String constant delimiters may be " or ' and are handled locally.
**	1.3. Comment delimiters may be different and are handled locally.
**	1.4. Floats without a preceding digit are processed as numbers.
**      1.5. Periods followed a newline in Cobol return a terminator token.
**	2. Tries to scan off large operators (2-Char) first, before attempting
**	   to scan off 1-Char operators.
**	3. If a language has trouble with an operator then it can 
**	   use GR_OPERATOR.
**
** History:	19-nov-1984 - Modified from the original. (ncg)
**		19-may-1987 - Modified for new scanner. (ncg)
**		14-apr-1994 - Add support for C++ style comments. (teresal)
**      12-Dec-95 (fanra01)
**          Added definitions for extracting data for DLLs on windows NT
**      06-Feb-96 (fanra01)
**              Made data extraction generic.
**	18-dec-96 (chech02)
**		Fixed redefined symbol errors: sc_linecont.
**
** Copyright (c) 2004 Ingres Corporation
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	05-apr-2007 (toumi01)
**	    Add support for the Ada ' tick operator.
*/

/* Handle operators which indicate line to be continued (see BASIC scanner) */
GLOBALREF	i4	(*sc_linecont)();

i4
sc_operator(mode)
i4	mode;				/* TRUE iff called from sc_eat */
{
    register KEY_ELM	*opkey;
    register i4	oplen;
    char		curch, peekch, prevch;
    char		opbuf[ 3 ];
    i4			opretval;   /* What to return in case of operator */

    /* Operators work only if single byte characters */
    curch = *SC_PTR;
    peekch = *(SC_PTR+1);
    prevch = *(SC_PTR-1);
    if (mode != SC_RAW)
    {
	if (curch == '.')
	{
	    /* Floating point without leading digit? */
	    if (CMdigit(&peekch))
		return sc_number(mode);

	    /* Special look-ahead for terminating period of Cobol */
	    if (dml_islang(DML_EQUEL) && eq_islang(EQ_COBOL) &&
		CMwhite(&peekch))
	    {
		SC_PTR++;				/* Skip the '.' */
		sc_addtok( SC_YYSTR, ERx(".") );
		return tok_special[ TOK_TERMINATE ];
	    }
	}
	else if (curch == '#' && dml_islang(DML_EQUEL))
	{
	    /* next object should be a word, and is scanned without lookup */
	    SC_PTR++;				/* Skip the '#' */
	    return sc_deref();
	}
	if (sc_linecont)
	{	
	    /* Is operator line continuation char? (ie, &\n) */
	    if ((*sc_linecont)() == 1)
		return SC_CONTINUE;
	}
    }

    opbuf[0] = curch;
    SC_PTR++;				/* Skip the current operator */
    if (CMoper(&peekch))		/* 2-Char operator */
    {
	SC_PTR++;			/* Skip next operator */
	opbuf[1] = peekch;
	opbuf[2] = '\0';
	oplen = 2;
    }
    else				/* 1-Char operator */
    {
	opbuf[1] = '\0';
	oplen = 1;
    }

    /* 
    ** The questioned operator is reduced to its smallest length.  This
    ** loop is executed at most twice, in the case when there are two 
    ** 1-Char operators following each other. Only the first one will be
    ** detected, after it is reduced from a 2-Char operator to 1-Char.
    ** At this point the operator is the smallest length of sequential 
    ** operators.
    */
    for (;;)
    {
	opkey = sc_getkey( tok_optab, tok_opnum, opbuf );
      /* Stop search if either found or length was one */
	if (opkey != KEYNULL || oplen == 1)
	    break;
      /* Reduce 2-Char operator to 1-Char and search again */
	SC_PTR--;			/* Back it up */
	opbuf[1] = '\0';
	oplen = 1;
    }

    if (opkey == KEYNULL)		/* No operator found */
    {
	if (mode != SC_COOKED)
	{
	    sc_addtok( SC_YYSTR, str_add(STRNULL,opbuf) );
	    return tok_special[TOK_NAME];
	} else
	{
	    er_write( E_EQ0221_scOPERATOR, EQ_ERROR, 1, opbuf );
	    return SC_CONTINUE;
	}
    }

    /*
    ** attempt to discriminate between ' as quote and ' as Ada tick
    ** purely by scanner context - good luck!
    */
    if (eq_islang(EQ_ADA))
	if (curch == '\'')
	    if (CMnmchar(&prevch))
	    {
		sc_addtok( SC_YYSTR, ERx("\'") );
		return tok_special[TOK_ATICK];
	    }

    if (opkey->key_tok == tok_special[TOK_QUOTE])	/* Scan string */
	return sc_string( opkey, mode );

    if (opkey->key_tok == tok_special[TOK_COMMENT])	/* Scan comment */
	return sc_comment( opkey->key_term );
	
    /*	
    ** Special hack for C++ comment - we need to do this because
    ** C++ doesn't have it's own token file yet.
    */
    if (eq_islang(EQ_CPLUSPLUS))			/* C++ '//' comment  */
    {
	if (opkey->key_tok == tok_special[TOK_SLASH] && peekch == '/')
	    return sc_comment( ERx("//") );
    }	

    /*
    ** In languages that have operators at the beginning of the line
    ** (Pl1) we want to avoid conflicts with Equel statements that can have
    ** optional operators (ie: target lists begin with a '(' ). 
    ** This is done via the GR_OPERATOR mechanism.
    */
    opretval = opkey->key_tok;				      /* Default */
    gr_mechanism( GR_OPERATOR, opkey->key_term, &opretval );  /* May set */
    sc_addtok( SC_YYSTR, opkey->key_term );
    return opretval;
}
