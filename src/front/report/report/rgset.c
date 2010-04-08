/*
** Copyright (c) 2004 Ingres Corporation
*/
# include	<compat.h>
# include	<me.h>
# include	<st.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<adf.h>
# include	<adfops.h>
# include	<fmt.h>
# include	<rtype.h> 
# include	<rglob.h> 
# include	<cm.h> 
# include   <st.h>
/*
**   R_G_SET - set of routines for reading in tokens from the RTEXT
**	strings.  These routines can be used to return alphanumeric
**	strings, numbers, or names.  A global character pointer
**	is used to handle the locations on the line.
**	The line must be ended with an endofstring ('\0' character). 
**	Internal end-of-line's are ignored and counted as white space,
**	as are commas.
**
**	The following routines are included in this set:
**		r_g_set - set up the internal pointers, flags, etc.
**			This must be called to preset the line.
**		r_g_skip - skip white spaces.  This 
**			can be used also to detect the type of token.
**		r_g_string - return a string.
**		r_g_name - return an alphanumeric.
**		r_g_nname = return a number, as a string.
**		r_g_long - find a i4  integer;
**		r_g_double - find a f8 number.
**		r_g_op - return an operator id.
**		r_g_paren - return string enclosed by parentheses.
**
**		also in this set (though not this file)
**
**		r_g_format - find a format specification;
**
**	Parameters:
**		string - start of the string to start the token routines.
**
**	Returns:
**		none.
**
**	Side Effects:
**		Sets up Tokchar, the global character pointer.
**
**	Trace Flags:
**		2.0, 2.5.
**
**	History:
**		3/29/81 (ps) - written.
**		25-nov-1986 (yamamoto)
**			Modified for double byte characters.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
*/

VOID
r_g_set(string)
char	*string;		/* start of line */
{
	/* start of routine */


	Tokchar = string;
	return;
}


/*
**   R_G_SKIP - skip white spaces in the line.  White spaces are considered
**	to be blanks, tabs, and newlines. 
**
**	Parameters:
**		none.
**
**	Returns:
**		type - code for type of token next on the line.
**			Values are TK_NUMBER,TK_ALPHA,TK_SIGN,
**			TK_ENDSTRING,TK_SQUOTE,TK_QUOTE,TK_OPAREN,TK_CPAREN,
**			TK_COMMA, TK_PERIOD, TK_SEMICOLON, TK_COLON,
**			TK_EQUALS, TK_DOLLAR, TK_RELOP, TK_MULTOP,
**			TK_POWER, TK_HEXLIT, or TK_OTHER.
**
**	Side Effects:
**		May update Tokchar if white space is to be skipped.
**		Note that this routine will not advance Tokchar
**		unless it detects white space.  Therefore, you
**		can call this any number of times, if not on 
**		white space, and it will return the same result.
**
**	Trace Flags:
**		2.0, 2.5.
**
**	History:
**		3/29/81 (ps) - written.
**		12/19/83 (gac)	added new types.
**		14-aug-1986 (Joe)
**			Added support for TK_SQUOTE.
**		23-mar-1987 (yamamoto)
**			Changed CH macros to CM.  This routine also skips
**			double byte blanks.
**		17-may-1993 (rdrane)
**			Add support for hex literals.
**      19-Aug-1997 (rodjo04) bug 84614
**          Fixed parsing of decimal values. Also added 
**          support for commas as decimal values. 
**          (if II_DECIMAL=,)
**      19-Sep-1997 (rodjo04) bug 84614
**          Amended above fix because CMnext() will treat
**          '\0' as a token. Changed call to STbcompare to
**          pass length of zero.
**      05-may-98 (rodjo04)
**          Backed out II_DECIMAL part of changes in bug 84614. 
**          This will eliminate all problems that were introduced
**          with II_DECIMAL = ,
*/

i4
r_g_skip()
{

	/* internal declarations */

    i4	type;			/* temp storage for return value */
    char decimal_sym[1];
    decimal_sym[0] = '.';
    /* start of routine */


	while (CMwhite(Tokchar))
		CMnext(Tokchar);

	if (*Tokchar == '\0')
	{
		type = TK_ENDSTRING;
	}
	else if (CMdigit(Tokchar) ||
	         (*Tokchar == decimal_sym[0]  && CMdigit(Tokchar+1)))
	{
        char *bookmark;
        bool Is_decsym = FALSE;
        bool Found_alpha = FALSE;
        bool Found_decsym = FALSE;
        
        bookmark = Tokchar;
        while (CMnmchar(Tokchar) || (Is_decsym = !STbcompare(Tokchar , 0, decimal_sym, 0, TRUE)))
        {
            if (!CMdigit(Tokchar) && !Is_decsym)
            {
                type = TK_ALPHA;
                Found_alpha = TRUE;
                break;
            }

            if (Is_decsym)
            {
                if (!Found_decsym)
                    Found_decsym = TRUE;
                else
                    break;
            }

            CMnext(Tokchar);
            Is_decsym = FALSE;
			if (*Tokchar == '\0')
				break;
        }
        if (!Found_alpha)
            type = TK_NUMBER;
       
        Tokchar = bookmark;
	}
	else if (CMnmstart(Tokchar) || *Tokchar == '_')
	{
		type = TK_ALPHA;
		if  (((*Tokchar == 'X') || (*Tokchar == 'x')) &&
		(*(Tokchar + 1) == '\''))
		{
			type = TK_HEXLIT;
		}
	}
	else
	{
		switch (*Tokchar)
		{
			case(','):
				type = TK_COMMA;
				break;

			case('.'):
				type = TK_PERIOD;
				break;

			case('('):
				type = TK_OPAREN;
				break;

			case(')'):
				type = TK_CPAREN;
				break;

			case('{'):
				type = TK_LCURLY;
				break;

			case('}'):
				type = TK_RCURLY;
				break;

			case('\"'):
				type = TK_QUOTE;
				break;

			case('\''):
				type = TK_SQUOTE;
				break;

			case(';'):
				type = TK_SEMICOLON;
				break;

			case(':'):
				type = TK_COLON;
				break;

			case('='):
				type = TK_EQUALS;
				break;

			case('$'):
				type = TK_DOLLAR;
				break;

			case('+'):
			case('-'):
				type = TK_SIGN;
				break;

			case('*'):
				if (*(Tokchar+1) == '*')
				{
					type = TK_POWER;
				}
				else
				{
					type = TK_MULTOP;
				}
				break;

			case('/'):
				type = TK_MULTOP;
				break;

			case('<'):
			case('>'):
				type = TK_RELOP;
				break;

			case('!'):
				if (*(Tokchar+1) == '=')
				{
					type = TK_RELOP;
				}
				else
				{
					type = TK_OTHER;
				}
				break;

			default:
				type = TK_OTHER;

		}
	}


	return(type);

}



/*
**   R_G_STRING - pull out a string from the internal line.  The string
**	will be all characters up to a quote character.
**
**	Parameters:
**		none.
**
**	Returns:
**		pointer to permanent storage spot of string.
**
**	Trace Flags:
**		2.0, 2.6.
**
**	History:
**		3/29/81 (ps) - written.
**		2/13/86 (mgw) - Fix bug 7966 - added arg stripslash to tell
**				r_g_string whether to strip backslashes here or
**				pass the unstripped string on so r_wild_card
**				can distinguish between wild card literals and
**				genuine wild card chars.
**		14-may-1986 (mgw) bug # 9375, 9312, 8838
**				Fix previous fix. Want backslashes always
**				stripped for non-wildcard characters (eg.
**				quote). Wild card chars are *?[] and if they
**				are proceeded with a backslash then r_wild_
**				card must know not to treat them as wild
**				card chars.
**		6-aug-1986 (Joe)
**			Added support for single quoted strings.
**			A single quoted string is the SQL default string
**			constant.  In a single quoted string, no characters
**			have any significance besides their value.  That
**			means to represent a character in a single quoted
**			string, you just put the character.  A single quote
**			is escaped by doubling it.
**
**			In order to make single quoted and double quoted
**			strings work together, I assume that strings work
**			as follows:
**				The strings in the DB are exactly as
**				the user typed it in.  Thus if
**				the user typed in "\\\"" that is
**				what is in the DB.
**
**				This routine does not remove backslashes.
**				It depends on r_wild_card or r_g_expr to
**				do it, since the interpretation of the
**				backslash depends on context it is being
**				used in.
**
**			For single quoted strings, this routine will
**			turn it into a double quoted string adding
**			\ as needed.  For example,
**				'This " string' =>  "This \" string"
**				'This '' string' => "This ' string"
**				'This \ string' =>  "This \\ string"
**
**		8-aug-1986 (Joe)
**			Allow backslash to escape $ and pattern
**			matching characters in single quoted strings.
**		2/28/90 elein 20305
**			Ensure that '''xxx''' is parsed correctly. The
**			inner '' are escaped single quotes.  Also added
**			break; to prevent the second character past a
**			backslashed * ? [ or ] from being doubled. 
**		9/21/90 (elein) 33146
**			Check for MAXSTRING! long parameters will over run it.
**			If we are going to run over the maximum w/this one,
**			just get out and leave the remainder of the string
**			to generate syntax errors.  This *should* only happen
**			when called w/very long params in rcrkpar.c.
**		5-mar-1992 (mgw) Bug #41778
**			In a related issue to bug 41778, don't double up
**			backslashes before wildcard characters when they're
**			in single quoted strings in ".if" stmts so the
**			wildcard characters can have special meaning there.
*/

char	*
r_g_string(tokdelimiter)
i4	tokdelimiter;
{
	/* internal declarations */

	char		temp[MAXSTRING];        /* temp storage for string */
	register char	*t;			/* ptr to temp */
	register char	*tok;			/* ptr into Tokchar */
	char		*newspot;		/* permanent location of string */
	char		*end;		/* permanent location of string */
	char		delimiter;

	/* start of routine */


	if (tokdelimiter == TK_QUOTE)
		delimiter = '\"';
	else
		delimiter = '\'';

	CMnext(Tokchar);

	t = temp;
	end = &temp[MAXSTRING-1]; 

	while (*Tokchar != '\0' && t != end)
	{
		/*
		** 20305: Case for '''xxx''' where the internal
		** '' are "escaped" single quotes.  
		*/
		if( *Tokchar == delimiter)
		{
			/* end double quotes */
			if( tokdelimiter != TK_SQUOTE )
			{
				break;
			}
			/* end single quote--it is not escaped */
			else if( *(Tokchar + 1) != '\'')
			{
				break;
			}
			/* single quote--escaped */
			else
			{
				*t++ = *Tokchar++;
				Tokchar++;
			}

		}
		else if (*Tokchar == '\\')
		{
			/** wont fit, get out **/
			if( t+1 == end)
			{
				*t++ = *Tokchar++;
				break;
			}
			if (tokdelimiter == TK_SQUOTE)
			{
				switch (*(Tokchar + 1))
				{
				  case '*':
				  case '?':
				  case '[':
				  case ']':
					*t++ = *Tokchar;

					/*
					** Bug 41778 - wildcard characters
					** only significant in an if stmt so
					** double up backslash in other cases.
					*/
					if (STequal(Cact_command, "if") != 0)
						Tokchar++;

					*t++ = *Tokchar++;

					/* 20305 */
					break;

				  default:
					*t++ = *Tokchar;
					*t++ = *Tokchar++;
				}
			}
			else
			{
				*t++ = *Tokchar++;
				if (*Tokchar != '\0')
				{
					CMcpyinc(Tokchar, t);
				}
			}
		}
		else if (tokdelimiter == TK_SQUOTE && *Tokchar == '\"')
		{
			*t++ = '\\';
			CMcpyinc(Tokchar, t);
		}
		else
		{
			CMcpyinc(Tokchar, t);
		}
	}

	*t = '\0';
	newspot = (char *) STalloc(temp);
	if (*Tokchar != '\0')
	{
		CMnext(Tokchar);
	}


	return(newspot);
}


/*
**   R_G_NAME - get an alphanumeric string from the internal line.  An
**	alphanumeric can contain letters, digits or underscore characters
**	(_).
**
**	Parameters:
**		none.
**
**	Returns:
**		pointer to name.
**
**	Trace Flags:
**		2.0, 2.7.
**
**	History:
**		3/29/81 (ps) - written.
**		23-jan-1989 (danielt)
**			changed to use MEreqmem()
*/

char	*
r_g_name()
{

	/* internal declarations */

	char	*start;			/* starting position of name */
	char	*newspot;		/* perm location of name */

	/* start of routine */


	start = Tokchar;

	while (CMnmchar(Tokchar))
	{
		CMnext(Tokchar);
	}

	newspot = (char *) MEreqmem(0, Tokchar-start+1,TRUE,(STATUS *) NULL);
	STmove(start, ' ', Tokchar-start, newspot);	/* (mmm) changed from pmove */
	*(newspot+(Tokchar-start)) = '\0';


	return(newspot);
}



/*
**   R_G_NNAME - get a string, representing a number, from the internal line.
**	A string is a leading sign, and digits and decimals.
**
**	Parameters:
**		none.
**
**	Returns:
**		pointer to nname.
**
**	Trace Flags:
**		2.0, 2.7.
**
**	History:
**		6/15/81 (ps) - written.
**
**   19-Aug-1997 (rodjo04) bug 84614.
**      Modified r_g_nname() so that commas can be used as 
**      decimal symbols if II_DECIMAL is set to ','.
**   05-may-98 (rodjo04)
**      Backed out II_DECIMAL part of changes in bug 84614. 
**      This will eliminate all problems that were introduced
**      with II_DECIMAL = ,
*/

char	*
r_g_nname()
{

	/* internal declarations */

	char	linebuf[50];		/* temp store for number string */
	register char	*c;		/* ptr to linebuf */
	char	*newspot;		/* perm location of name */
    char decimal_sym[1];
    decimal_sym[0] = '.';
	/* start of routine */


	MEfill((u_i2)50, 0, (PTR)linebuf);
	c = linebuf;

	if (r_g_skip() == TK_SIGN)
	{
		CMcpyinc(Tokchar, c);
	}

	r_g_skip();

	while (CMdigit(Tokchar) || *Tokchar == decimal_sym[0] )
	{
       	        CMcpyinc(Tokchar, c);
	}

	if ((*Tokchar == 'e') || (*Tokchar == 'E'))
	{
		*c++ = *Tokchar++;
	}

	if (r_g_skip() == TK_SIGN)
	{
		CMcpyinc(Tokchar, c);
	}

	r_g_skip();

	while (CMdigit(Tokchar))
	{
		CMcpyinc(Tokchar, c);
	}

	newspot = (char *) STalloc(linebuf);


	return(newspot);

}


/*
**   R_G_LONG - return a number (i4 integer)  from
**	the next word in the internal line.
**
**
**	Parameters:
**		number - address of i4  var.
**
**	Returns:
**		error - negative one for illegal number, 0 for ok.
**
**	Trace Flags:
**		2.0, 2.8.
**
**	History:
**		4/20/81 (ps) - written.
*/

i4
r_g_long(number)
i4	*number;
{
	/* internal declarations */

	i4	sign;		/* -1 if negative number, +1 if positive */

	/* start of routine */




	switch (r_g_skip())
	{
		case(TK_SIGN):
			sign = (*Tokchar == '+') ? 1 : -1;
			CMnext(Tokchar);
			switch(r_g_skip())
			{
				case(TK_NUMBER):
					break;

				default:
					/* bad number */
					return(-1);
			}
			break;

		case(TK_NUMBER):
			sign = 1;
			break;

		default:
			return(-1);

	}

	/* at this point we'll convert the number to numeric */

	*number = 0;
	while (CMdigit(Tokchar))
	{

		*number = (*number * 10) + (*Tokchar - '0');
		CMnext(Tokchar);
	}

	*number *= sign;


	return(0);
}
/*
**   R_G_OP - return the operator id for the next operator in RTEXT.
**
**
**	Parameters:
**		none.
**
**	Returns:
**		operator id (ADI_..._OP)
**
**	Trace Flags:
**		none.
**
**	History:
**		12/19/83 (gac)	written.
**		3/15/90 (elein) b20466 et al. a series of <>  = != and
**			space problems...  Changed r_g_op to assign the
**			AD operator ID directly because adi_ops() did not
**			recognize <> as a valid operator construct.  If
**			it is added to adi in the future, we can change
**			this routine to call adi_ops() to fetch the 
**			operator id again.
*/

ADI_OP_ID
r_g_op()
{
	ADI_OP_ID	opid;
	ADI_OP_NAME	opname;
	char		*c;

	c = opname.adi_opname;

	switch(*Tokchar++)
	{
		case('+'):
			return(ADI_ADD_OP);
			break;
		case('-'):
			return(ADI_SUB_OP);
			break;
		case('/'):
			return(ADI_DIV_OP);
			break;
		case('='):
			return(ADI_EQ_OP);
			break;

		case('*'):
			if (*Tokchar == '*')
			{
				Tokchar++;
				return(ADI_POW_OP);
				break;
			}
			return(ADI_MUL_OP);
			break;

		case('<'):
			if (*Tokchar == '>')
			{
				Tokchar++;
				return(ADI_NE_OP);
				break;
			}
			if (*Tokchar == '=')
			{
				Tokchar++;
				return(ADI_LE_OP);
				break;
			}
			return(ADI_LT_OP);
			break;
		case('!'):
			if (*Tokchar == '=')
			{
				Tokchar++;
				return(ADI_NE_OP);
				break;
			}
			break;
		case('>'):
			if (*Tokchar == '=')
			{
				Tokchar++;
				return(ADI_GE_OP);
				break;
			}
			return(ADI_GT_OP);
			break;
	}

	return(ADI_NOOP);
}

/*
**   R_G_PAREN - return the string enclosed in parentheses, including the
**		parentheses.
**
**
**	Parameters:
**		none.
**
**	Returns:
**		pointer to parenthesized string.
**
**	History:
**		17-jun-87 (grant)	written.
*/

char *
r_g_paren()
{
	/* internal declarations */

	char		temp[MAXSTRING];	/* temp storage for string */
	register char	*t;			/* ptr to temp */
	char		*newspot;		/* permanent location of string */

	/* start of routine */

	t = temp;

	while (*Tokchar != ')' && *Tokchar != '\0')
	{
		CMcpyinc(Tokchar, t);
	}

	if (*Tokchar == ')')
	{
		CMcpyinc(Tokchar, t);
	}

	*t = '\0';
	newspot = (char *) STalloc(temp);

	return(newspot);
}
