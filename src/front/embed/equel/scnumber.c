# include	<compat.h>
# include	<cv.h>		/* 6-x_PC_80x86 */
# include	<er.h>
# include	<si.h>
# include	<cm.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<equtils.h>
# include	<eqsym.h>
# include	<eqgr.h>
# include	<eqscan.h>
# include	<equel.h>
# include	<eqlang.h>
# include	<ereq.h>

/*
+* Filename:	SCNUMBER.C 
** Purpose:	Process a numeric token either as a Float, an Integer, or
**		a Decimal.  
**
** Defines: 	
**		sc_number( mode )		- Process number.
** Locals:
**		scan_digits( cp, numbuf )	- Eats up string of digits.
** Notes:
**  1. Special cases exist for :
**  1.1. PASCAL, ADA range statements LOW..HIGH.
**  1.2. BASIC integer constants terminated with a %.
**  1.3. COBOL terminating periods followed by newlines.
**  1.4. ADA underscores in numbers.
**  2. The numeric strings accepted are:
**         D*[.D*[(e|E)[+|-]D*]]
**     where
**         D	means any decimal digit
**         []	means optional
**         ()	means grouping
**         |	means alternation
**         +-.	are literals
**         *	changes the previous expression from "1 of" to "0 or more of"
**  2.1 We can be called only by yylex() (if the string starts with 'D')
**      or by sc_operator() (if the string starts with '.D'), thus excluding
**	   .[(e|E)[+|-]D*]
**      Of course, although the empty string would also be accepted, we'll
**      never be called in that case.
**  2.2 This is how we distinguish between types of literal numbers: 
**		 o Integer is a string of (at most 10) digits whose value is 
**		   not greater than the legal range for an integer. 
**		 o Decimal is a string of digits with a decimal point.
**		   Decimal numbers can have at most 31 digits.	
**		 o Float literals are specified in scientific notation,
**		   i.e., it contains an 'E' or 'e'.  If a decimal literal
**		   has more than 31 digits, it is a float.	
**		 o A large integer, i.e., value exceeds what fits into a 
**		   4 byte integer, is marked as a decimal and passed as a 
**		   decimal character string.  Note the DBMS treats large 
**		   integers as floats.
-*
**
** History:
**	    19-nov-1984 - Modified from original. (ncg)
**	    12-dec-1984 - Documented which numeric strings are accepted. (mrw)
**	    04-mar-1986 - Added underscore scanning to ADA. (ncg)
**	    07-apr-1986 - Added raw arguments. (ncg)
**	    19-may-1987	- Modified for 6.0. (ncg)
**	    13-jun-1990 - Added decimal support. (teresal)	
**	    13-jun-1990 (thoda04)
**	        Corrected sc_number()'s parm; mode is passed, used as i4, not bool.
**	        Added function parameter prototype for scan_digits().
**
** Copyright (c) 2004 Ingres Corporation
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/	

/* Local routine to eat off integers */
static i4	scan_digits(char **cp, char *numbuf, i4  *count);

/*
+* Procedure:	sc_number
** Purpose:	Process a numeric token.
**
** Parameters:	mode - i4  - level of processing.
-* Returns:	i4 - Lexical code for the appropriate type of number.
** Notes:
**	Assumes that it is being called because SC_PTR is pointing at a digit
**	or a decimal point.
*/

i4
sc_number(mode)
i4  	mode;
{
    char	numbuf[ SC_NUMMAX + 4 ]; /* '.' + 'e' + '+|-' + null */
    char	*cp;
    f8		f8temp;
    i4		i4temp;
    char	dectemp[ DB_MAX_DECLEN ]; /* Allow for maximum decimal value */ 
    i4		prec;
    i4		scale;
    i4		intretval;   /* What to return in case of i4  constant */

    /*
    ** In languages that have level numbers at the beginning of the line
    ** (COBOL or PL1) we want to avoid conflicts with EQUEL statements that
    ** terminate with a integers (deleterow).  This is done via the GR_NUMBER
    ** mechanism.
    */
    intretval = tok_special[ TOK_I4NUM ];		/* Default */
    scale = 0;						/* Default */
    prec = 0;						/* Initialize */
    gr_mechanism( GR_NUMBER, &intretval );		/* Set if required */

    cp = numbuf;
    if (CMdigit(SC_PTR))			/* First is digit */
    {
	if (!scan_digits(&cp, numbuf, &prec))	/* Eat off integer part */
	    return intretval;			/* Error occurred */
    }

    /* 
    ** Do rest of type determination.  If first part was an integer then it
    ** can be followed by nothing, a '.' [ followed by integer ], or followed
    ** with the 'E' description.  If the first part was just a '.' then it
    ** should be followed by an integer.
    **
    ** Scan_digits puts all digits into 'numbuf', leaving 'cp' pointing at
    ** next available cell (possibly will be null terminated); SC_PTR is left
    ** pointing at next non-digit in scanner buffer.
    */

    switch (*SC_PTR)
    {
      case '.':
	/*
	** Special test here for:
	** 	1. PASCAL or ADA subrange type A..B - Leave SC_PTR pointing at
	**	   first '.', and convert the low integer range (A).
	**	2. COBOL terminator .\n - Leave SC_PTR pointing at period, and
	**	   convert the integer value.
	*/
	if (   (   (eq_islang(EQ_PASCAL) || eq_islang(EQ_ADA))
	        && (*(SC_PTR+1) == '.'))
	    || (    eq_islang(EQ_COBOL) && *(SC_PTR+1) == '\n'))
	{
	    goto conv_int;
	}

	/*
	** Floating point or Decimal - first copy the decimal point, then if 
	** followed by digits, scan them off.
	*/
	CMcpyinc(SC_PTR, cp);
	if (CMdigit(SC_PTR))			/* Eat off integer part */
	{
	    if (!scan_digits(&cp, numbuf, &scale))
		return intretval;		/* Error occurred */
	}

	/* If digits are not followed by an 'E' then simple number.number, */
	/* i.e., it's a decimal - calculate precision then convert */
	prec += scale;
	if (*SC_PTR != 'e' && *SC_PTR != 'E')
	    goto conv_dec;

	/* FALL THROUGH - for 'E' description */

      case 'e':
      case 'E':
	/*
	** Current character is 'E' - Is this the exponential 'E'? (Check by
	** seeing if the next character is a sign or a digit).
	*/
	if (*(SC_PTR+1) == '-' || *(SC_PTR+1) == '+')
	{
	    *cp++ = *SC_PTR++;				/* Add the 'E' */
	    *cp++ = *SC_PTR++;				/* Add the sign */
	    if (!scan_digits(&cp, numbuf, &prec))	/* Get exponent */
		return intretval;			/* Error occurred */
	}
	else if (CMdigit(SC_PTR+1))
	{
	    *cp++ = *SC_PTR++;				/* Add the 'E' */
	    if (!scan_digits(&cp, numbuf, &prec))	/* Get exponent */
		return intretval;			/* Error occurred */
	}
	else /* Not a digit and not a sign - leave the 'E' */
	    goto conv_int;

	/* SC_PTR either points at after the exponent */

conv_float:
	*cp = '\0';
	sc_addtok( SC_YYSTR, str_add( STRNULL, numbuf) );
	if (CVaf(numbuf, '.', &f8temp) != OK)
	    er_write( E_EQ0220_scNUMCONV, EQ_ERROR, 1, numbuf );
	return tok_special[ TOK_F8NUM ];	

      case '%':
	/* 
	** Special case for BASIC integer constants that end with '%'
	** Advance past the character.
	*/
	if (eq_islang(EQ_BASIC) && mode == SC_RAW)
	    SC_PTR++;

	/* FALL THROUGH */

      default:						 /* Integer */

conv_int:
	*cp = '\0';
	/* If too big to fit in an integer, try converting to decimal */
	if (CVal(numbuf, &i4temp) != OK)
	    goto conv_dec;
	sc_addtok( SC_YYSTR, str_add( STRNULL, numbuf) );
	return intretval;

conv_dec:
	*cp = '\0';
	/* If too big to fit into a decimal, try converting to float */
	if (prec > 31)
	    goto conv_float;
	if (CVapk(numbuf, '.', prec, scale, dectemp) != OK)
	    er_write( E_EQ0220_scNUMCONV, EQ_ERROR, 1, numbuf );
	sc_addtok( SC_YYSTR, str_add( STRNULL, numbuf) );
	return tok_special[ TOK_DECIMAL ];	
    }
}

/*
+* Procedure:	scan_digits 
** Purpose:	Eat away at digits, till non-digit. Check for overflow.
**
** Parameters:	cp 	- char * - First point to start at,
**		numbuf 	- char * - Buffer to fill.
**		count   - i4   * - Number of digits eaten.
-* Returns:	i4 	- 1 / 0 : Success / Failure (overflow)
** Notes:
** 	Scan_digits puts all digits into 'numbuf', leaving 'cp' pointing at
** 	next available cell; SC_PTR is left pointing at next non-digit in
** 	scanner buffer (digits may be followed by anything - not neccessarily
**	a space).  Keep track of how many digits were 'eaten' so precision
**	and scale can be determined if this is a decimal literal.
*/

static i4
scan_digits( cp, numbuf, count )
char		**cp;
register char	*numbuf;
register i4	*count;
{
    register	char	*cp1 = *cp;
    i4			underscore = eq_islang(EQ_ADA);

    while (CMdigit(SC_PTR))
    {
	if (cp1 - numbuf >= SC_NUMMAX)
	{
	    *cp1 = '\0';
	    er_write( E_EQ0214_scNUMLONG, EQ_ERROR, 2, er_na(SC_NUMMAX), 
		    numbuf );
	    sc_addtok( SC_YYSTR, ERx("0") );
	    /* Strip to end of digit stream */
	    while (CMdigit(SC_PTR) || (underscore && *SC_PTR == '_'))
		CMnext(SC_PTR);
	    return 0;
	}
	CMcpyinc(SC_PTR, cp1);			/* Copy the digit */
	(*count)++;				/* Increment digit count */
	while (underscore && *SC_PTR == '_') 	/* Skip ADA underscores */
	    CMnext(SC_PTR);
    }
    *cp = cp1;
    return 1;	/* cp points at next slot, SC_PTR points at next non-digit */
}
