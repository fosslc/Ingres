/*
** Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<si.h>
# include	<cm.h>
# include	<equtils.h>
# include	<eqscan.h>
# include	<equel.h>
# include	<eqlang.h>
# include	<ereq.h>
# include       <er.h>

/**
+*  Name: schex.c - Parse a hex string.
**
**  Defines:
**	sc_hexconst - Parse and return a hex string.
**	sc_uconst   - Parse and return a Unicode literal.
**	sc_dtconst  - Parse & return a date/time literal.
**
**  Notes:
**	Format of a hex string is:
**
**	  X|x'hex_pair{hex_pair}'
**	  or
**	  0X|xhex_pair{hex_pair}
**
**	Format of a Unicode literal is:
**	  U&'ASCII data intermixed with \f12c escape sequences'
**	  where the escape sequences explicitly define code points as
**	  a string of 4 hex digits.
-*
**  History:
**	20-jan-1987	- Written
**	21-mar-94 (smc) Bug #60829
**		Added #include header(s) required to define types passed
**		in prototyped external function calls.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	07-Nov-05 (toumi01) SIR 115505
**	    Support hex constants in format 0x696E67636F7270 as well as the
**	    traditional X'6279656279654341' to aid application portability.
**	8-dec-05 (inkdo01)
**	    Add sc_uconst() to handle Unicode literals (SIR 115589). The 
**	    code is here because I'm too lazy to create a new module.
**	16-aug-2007 (dougi)
**	   Add sc_dtconst() to handle date/time literals.
**/

/*{
+*  Name: sc_hexconst - Scan a hex string.
**
**  Description:
**	This routine scans a hex string constant.  Much like normal string
**	scanning, but more restrictive, it allows continuation of lines.
**
**  Inputs:
**	none.
**
**  Outputs:
**	Returns:
**	    TOK_HEXCONST - (tHEXCONST) 
**	Errors:
**	    EscHEXCHAR- Illegal character in hex string constant.
**	    EscHEXODD - Odd number of hex characters.
**	    EscNLSTR  - New line in string.
**	    EscLONGOBJ- Hex constant too long.
**	    EscEOFSTR - EOF in string constant.
**
**  Notes:
**	1. Assumes caller has checked that SC_PTR points at an X followed
**	   by the single quote.
**	2. On exit SC_PTR is pointing at next spot in scanner.
-*
**  History:
**	21-jan-1987	- Written (ncg)
**	19-may-1987	- Modified for new scanner (ncg)
**	23-aug-2007 (dougi)
**	    Fumble fingers repeated a line incorrectly.
*/

i4
sc_hexconst()
{
    register char  *cp;
    char	   quote;			/* Closing/opening quote */
    char	   hexbuf[SC_STRMAX + 5];	/* Hex string + x+'+'+null */
    char	   errbuf[3];			/* For illegal character */
    i4		   done;			/* Loop terminator:   */
    bool	   hexconst_0xNN = FALSE;	/* new support for 0xNNNNNN constants */
#    define		HX_OK	0		/* Okay while reading */
#    define		HX_DONE	1		/* Read closing quote */
#    define		HX_ERR	2		/* Error encountered  */

    cp = &hexbuf[0];
    hexconst_0xNN = (*SC_PTR == '0');
    *cp++ = *SC_PTR++;				/* Add x or 0 */
    *cp++ = quote = *SC_PTR++;			/* Add quote or x */

    done = HX_OK;
    while (done == HX_OK)
    {
	*cp = '\0';			/* Clean out buffer spot */

	/* Escaped newline allowed in EQUEL/ALL or ESQL/C */
	if (   (*SC_PTR == '\\' && *(SC_PTR+1) == '\n')
	    && (dml_islang(DML_EQUEL) || eq_islang(EQ_C)))
	{
	    sc_readline();		/* Read a new line */
	    continue;
	}
	else if (*SC_PTR == '\n')
	{
	    sc_readline();
	    if (dml_islang(DML_ESQL))
	    {
		/* (*dm_strcont) will skip string continuation */
		if (dml->dm_strcontin && (*dml->dm_strcontin)('\''))
		    continue;
	    }
	    er_write( E_EQ0219_scNLSTR, EQ_ERROR, 1, hexbuf);
	    done = HX_ERR;
	}
	else if (*SC_PTR == quote && !hexconst_0xNN)	/* Done with string */
	{
	    CMcpyinc(SC_PTR, cp);		/* Add it and	      */
	    *cp = '\0';				/* Clear closing spot */
	    if (   (cp == hexbuf+3)		/* Bad number of chars ? */
		|| ((cp - 1 - hexbuf) % 2 != 0))
	    {
		er_write( E_EQ0228_scHEXODD, EQ_ERROR, 1, hexbuf);
		/* Fall through to completion code */
	    }
	    done = HX_DONE;
	}
	else if (CMhex(SC_PTR))			/* Add hex character */
	{
	    if (cp - hexbuf < SC_STRMAX)
		CMcpyinc(SC_PTR, cp);
	    else				/* String too big */
	    {
	      /* We're guaranteed these two bytes */
		*cp = quote;			/* Help error message */
		*(cp+1) = '\0';
		er_write( E_EQ0213_scHEXLONG, EQ_ERROR, 2, er_na(SC_STRMAX), 
			hexbuf );
		done = HX_ERR;
	    }
	}
	else				/* Non-hex character */
	{
	    if (hexconst_0xNN)
	    {
		*cp = '\0';			/* Clear closing spot */
		if (   (cp == hexbuf+3)		/* Bad number of chars ? */
		    || ((cp - hexbuf) % 2 != 0))
		{
		    er_write( E_EQ0228_scHEXODD, EQ_ERROR, 1, hexbuf);
		    /* Fall through to completion code */
		}
		done = HX_DONE;
	    }
	    else
	    {
		if (*SC_PTR == SC_EOF)
		    er_write( E_EQ0206_scEOFSTR, EQ_ERROR, 1, hexbuf );
		else
		{
		    char *p = errbuf;

		    CMcpychar(SC_PTR, errbuf);
		    CMnext( p );
		    *p = '\0';
		    er_write( E_EQ0227_scHEXCHAR, EQ_ERROR, 1, errbuf );
		}
	    done = HX_ERR;
	    }
	}
    }

    /*
    ** Dropped out of loop, either because of normal scanning (HX_DONE) or
    ** error already reported (HX_ERR). If an error then skip to the
    ** end of the string.
    */
    if (done == HX_ERR)
    {
	while (*SC_PTR != quote && *SC_PTR != '\n' && *SC_PTR != SC_EOF)
	    CMnext(SC_PTR);
	if (*SC_PTR == quote)
	    CMnext(SC_PTR);
	*cp++ = quote;			/* Add closing quote for completeness */
	*cp = '\0';
    }

    sc_addtok(SC_YYSTR, str_add(STRNULL, hexbuf));
    return tok_special[ TOK_HEXCONST ];
}

/*{
+*  Name: sc_uconst - Scan a Unicode literal.
**
**  Description:
**	This routine scans a Unicode literal. Much like normal string
**	scanning, but more restrictive, it allows continuation of lines.
**
**  Inputs:
**	none.
**
**  Outputs:
**	Returns:
**	    TOK_UCONST - (tUCONST) 
**	Errors:
**	    EscUESC   - Illegal Unicode escape sequence.
**	    EscNLSTR  - New line in string.
**	    EscLONGOBJ- Unicode literal too long.
**	    EscEOFSTR - EOF in literal.
**
**  Notes:
**	1. Assumes caller has checked that SC_PTR points at the sequence 
**	   U&'.
**	2. On exit SC_PTR is pointing at next spot in scanner.
-*
**  History:
**	8-dec-05 (inkdo01)
**	    Cloned from sc_hexconst.
*/

i4
sc_uconst()
{
    register char  *cp;
    char	   quote;			/* Closing/opening quote */
    char	   ubuf[SC_STRMAX + 5];	/* Hex string + x+'+'+null */
    char	   errbuf[3];			/* For illegal character */
    i4		   hcount;			/* Count of hexits in escape
						** sequence */
    i4		   done;			/* Loop terminator:   */
#    define		U_OK	0		/* Okay while reading */
#    define		U_DONE	1		/* Read closing quote */
#    define		U_ERR	2		/* Error encountered  */

    cp = &ubuf[0];
    *cp++ = *SC_PTR++;				/* Add U */
    *cp++ = *SC_PTR++;				/* Add & */
    *cp++ = quote = *SC_PTR++;			/* Add quote */

    done = U_OK;
    while (done == U_OK)
    {
	*cp = '\0';			/* Clean out buffer spot */

	/* Escaped newline allowed in ESQL/C */
	if (   (*SC_PTR == '\\' && *(SC_PTR+1) == '\n')
	    && eq_islang(EQ_C))
	{
	    sc_readline();		/* Read a new line */
	    continue;
	}
	else if (*SC_PTR == '\n')
	{
	    sc_readline();
	    if (dml_islang(DML_ESQL))
	    {
		if (dml->dm_strcontin && (*dml->dm_strcontin)('\''))
		    continue;
	    }
	    er_write( E_EQ0219_scNLSTR, EQ_ERROR, 1, ubuf);
	    done = U_ERR;
	}
	else if (*SC_PTR == quote && *(SC_PTR + 1) == quote)
	{
	    /* Double quote - copy 'em both */
	    CMcpyinc(SC_PTR, cp);		/* Add them both    */
	    CMcpyinc(SC_PTR, cp);
	    *cp = '\0';				/* Clear closing spot */
	    continue;
	}
	else if (*SC_PTR == quote)		/* Done with string */
	{
	    CMcpyinc(SC_PTR, cp);		/* Add it and	      */
	    *cp = '\0';				/* Clear closing spot */
	    done = U_DONE;
	}
	else if (*SC_PTR == '\\' && *(SC_PTR + 1) == '\\')
	{
	    /* Double backslash - copy 'em both. */
	    CMcpyinc(SC_PTR, cp);		/* Add them both */
	    CMcpyinc(SC_PTR, cp);
	    *cp = '\0';				/* Clear closing spot */
	    continue;
	}
	else if (*SC_PTR == '\\')
	{
	    /* Single backslash - this is an escape sequence. */
	    CMcpyinc(SC_PTR, cp);		/* Add the \	      */
	    if (*SC_PTR == '+')
	    {
		CMcpyinc(SC_PTR, cp);		/* Add the + 	      */
		hcount = 6;
	    }
	    else hcount = 4;

	    for (; hcount > 0; hcount--)
	     if (CMhex(SC_PTR))
	     {
		if (cp - ubuf < SC_STRMAX)
		    CMcpyinc(SC_PTR, cp);	/* copy it */
		else
		{
		    *cp = quote;			/* Help error message */
		    *(cp+1) = '\0';
		    er_write(E_EQ0213_scHEXLONG, EQ_ERROR, 2, er_na(SC_STRMAX), 
			ubuf );
		    done = U_ERR;
		    break;
		}
	     }
	     else
	     {
		er_write( E_EQ0232_scUESC, EQ_ERROR, 0);
		done = U_ERR;
		break;
	     }
	}
	else if (*SC_PTR == SC_EOF)
	{
	    er_write( E_EQ0206_scEOFSTR, EQ_ERROR, 1, ubuf );
	    done = U_ERR;
	}
	else CMcpyinc(SC_PTR, cp);		/* Add ASCII char */
    }

    /*
    ** Dropped out of loop, either because of normal scanning (U_DONE) or
    ** error already reported (U_ERR). If an error then skip to the
    ** end of the string.
    */
    if (done == U_ERR)
    {
	while (*SC_PTR != quote && *SC_PTR != '\n' && *SC_PTR != SC_EOF)
	    CMnext(SC_PTR);
	if (*SC_PTR == quote)
	    CMnext(SC_PTR);
	*cp++ = quote;			/* Add closing quote for completeness */
	*cp = '\0';
    }

    sc_addtok(SC_YYSTR, str_add(STRNULL, ubuf));
    return tok_special[ TOK_UCONST ];
}

/*{
+*  Name: sc_dtconst - Scan a date/time literal.
**
**  Description:
**	This routine scans a date/time literal. Little is done other
**	than to skip to the trailing quote. That is, it makes no 
**	attempt to validate the contents of the literal. 
**
**  Inputs:
**	lbuf		- ptr to work buffer containing literal
**	qoff		- offset in lbuf of opening quote
**
**  Outputs:
**	Returns:
**	    TOK_DTCONST - (tDTCONST) 
**	Errors:
**	    EscNLSTR  - New line in string.
**	    EscEOFSTR - EOF in literal.
**
**  Notes:
**	1. Assumes caller has checked that SC_PTR points at the sequence 
**	   <date/time type>'
**	2. On exit SC_PTR is pointing at next spot in scanner.
-*
**  History:
**	15-aug-2007 (dougi)
**	    Cloned (very loosely) from sc_hexconst.
*/

i4
sc_dtconst(lbuf, qoff)
char	*lbuf;
i4	qoff;

{
    char	*cp = &lbuf[qoff];

    /* Skip over the opening quote, then loop, looking for the closing quote.
    ** If it is an INTERVAL, separate parsing rules will process it. */

    SC_PTR++;
    *cp++ = '\'';

    while (*SC_PTR != '\'')
	CMcpyinc(SC_PTR, cp);

    SC_PTR++;
    *cp++ = '\'';
    *cp = '\0';
    sc_addtok(SC_YYSTR, str_add(STRNULL, lbuf));

    return tok_special[ TOK_DTCONST ];
}
