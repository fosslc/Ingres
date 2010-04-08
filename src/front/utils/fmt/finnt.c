
/*
**	Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<er.h>
# include	<adf.h>
# include	<afe.h>
# include	<fmt.h>
# include	<cm.h>
# include	<multi.h>
# include	"format.h"
# include	<st.h>

/**
** Name:	finnt.c -	This file contains the routine for converting
**				strings formatted with a numeric template
**				into a number.
**
** Description:
**	This file defines:
**
**	f_innt		Convert a long text string into a number
**			using a numeric template format as its guide.
**
**	f_escchrin	Find whether an escaped character is in a string.
**
**	f_substr	Find whether a string is a substring of another string.
**
** History:
**	31-dec-86 (grant)	created.
**	6-jun-87 (danielt) removed unused len variable in f_innt
**	01/04/89 (dkh) - Changed to call "afe_cvinto()" instead of
**			 "adc_cvinto()".
**	7/24/90 (elein)- 30903
**		Allow escaped "d" and "c" characters.
**	01/08/91 (elein) 34760 
**		Dont allow spaces in the middle of a number to be ignored.
**		Put out editing error message instead.
**	01/21/92 (tom)
**		- strip trailing space from input string before parsing
**		  this avoids the logic implemented for 34760 from 
**		  determining that the string is bad just because it
**		  has white space at the end (as it does have if it's
**		  format string has trailing negative indicators).
**		- the following is not true.. we rolled this "fix" out on 1-30-92
**		  in fact the scan of the template string always looks at the
**		  thousands seperator, and is not dependent on the decimal sym.
**		  fix the recognition of the thousands separator
**		  if the decimal character is a comma.
**	01/30/92 (tom)
**		- rollback the "fix" above.
**	22-dec-1998 (crido01) Bugs 68819, 56301
**		Hand integrate a change (274705) from the OpenROAD 3.5 codeline 
**		that allows blanks in user input for numeric types if blanks
**		exist int the numeric template. 
**		
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/* # define's */
/* extern's */
/* static's */

/*{
** Name:	f_innt	- Converts a long text string into a number
**			  using a numeric template format.
**
** Description:
**	This routine converts a long text string into a number value using
**	a numeric template format as a guide.
**
** Inputs:
**	cb		Control block for ADF routine call.
**	fmt		Pointer to FMT structure which is a numeric template.
**
**	display		Points to the long text string.
**
** Outputs:
**	value		Points to the converted number.
**
**	Returns:
**		E_DB_OK
**		E_DB_ERROR
**
** History:
**	31-dec-86 (grant)	implemented.
*/

STATUS
f_innt(cb, fmt, display, value)
ADF_CB		*cb;
FMT		*fmt;
DB_DATA_VALUE	*display;
DB_DATA_VALUE	*value;
{
    AFE_DCL_TXT_MACRO(100)	buffer;
    DB_TEXT_STRING	*number;
    u_char		*n;
    DB_DATA_VALUE	dvfrom;
    DB_TEXT_STRING	*disp;
    u_char		*d;
    u_char		*dstart;
    u_char		*dend;
    char		*t;
    bool		negative = FALSE;
    bool		cr_found;
    bool		db_found;
    bool		escaped_minus_found;
    bool		minus_found;
    bool		escaped_oparen_found;
    bool		oparen_found;
    bool		escaped_cparen_found;
    bool		cparen_found;
    bool		escaped_oangle_found;
    bool		oangle_found;
    bool		escaped_cangle_found;
    bool		cangle_found;
    bool		escaped_ocurly_found;
    bool		ocurly_found;
    bool		escaped_ccurly_found;
    bool		ccurly_found;
    bool		escaped_osquare_found;
    bool		osquare_found;
    bool		escaped_csquare_found;
    bool		csquare_found;
    bool		plus_found;
    bool		star_found;
    bool		mny_found;
    bool		thous_found;
    bool		blank_found;
    char		*m;
    char		*mny_sym;
    char		decimal_sym;
    char		thous_sym;

    bool		f_escchrin();
    bool		f_substr();

    /* start of routine */

    /* set up numeric and money symbols */

    mny_sym = cb->adf_mfmt.db_mny_sym;
    decimal_sym = cb->adf_decimal.db_decimal;

    thous_sym = (decimal_sym == FC_DECIMAL)? FC_SEPARATOR : FC_DECIMAL;

    t = fmt->fmt_var.fmt_template;
    if (t == NULL)
    {
	return afe_error(cb, E_FM6001_NO_TEMPLATE, 0);
    }

    disp = (DB_TEXT_STRING *) display->db_data;
    dstart = disp->db_t_text;
    dend = dstart + disp->db_t_count;

    /* we must strip white space off the end of the number because
       positive instances of formats which have trailing negative
       indicators will mistakenly be flagged as an error */
    while (dend > dstart && CMwhite(dend - 1))
    {
	if (dend - 1 > dstart && CMdbl1st(dend - 2))
		break;
	dend--;
    }

    number = (DB_TEXT_STRING *) &buffer;
    n = number->db_t_text;

    /* determine if special chars are in template */

    escaped_minus_found = f_escchrin(t, FCP_MINUS);
    minus_found = (STindex(t, FCP_MINUS, 0) != NULL);
    escaped_oparen_found = f_escchrin(t, FCP_OPAREN);
    oparen_found = (STindex(t, FCP_OPAREN, 0) != NULL);
    escaped_cparen_found = f_escchrin(t, FCP_CPAREN);
    cparen_found = (STindex(t, FCP_CPAREN, 0) != NULL);
    escaped_oangle_found = f_escchrin(t, FCP_OANGLE);
    oangle_found = (STindex(t, FCP_OANGLE, 0) != NULL);
    escaped_cangle_found = f_escchrin(t, FCP_CANGLE);
    cangle_found = (STindex(t, FCP_CANGLE, 0) != NULL);
    escaped_ocurly_found = f_escchrin(t, FCP_OCURLY);
    ocurly_found = (STindex(t, FCP_OCURLY, 0) != NULL);
    escaped_ccurly_found = f_escchrin(t, FCP_CCURLY);
    ccurly_found = (STindex(t, FCP_CCURLY, 0) != NULL);
    escaped_osquare_found = f_escchrin(t, FCP_OSQUARE);
    osquare_found = (STindex(t, FCP_OSQUARE, 0) != NULL);
    escaped_csquare_found = f_escchrin(t, FCP_CSQUARE);
    csquare_found = (STindex(t, FCP_CSQUARE, 0) != NULL);
    plus_found = (STindex(t, FCP_PLUS, 0) != NULL);
    star_found = (STindex(t, FCP_STAR, 0) != NULL);
    mny_found = (STindex(t, FCP_CURRENCY, 0) != NULL);
    thous_found = (STindex(t, FCP_SEPARATOR, 0) != NULL);
    cr_found = f_substr(t, ERx("cr"));
    db_found = f_substr(t, ERx("db"));
    blank_found = (STindex(t, FCP_BLANK, 0) != NULL);

    /*
    ** Pick off digits and decimal point.
    ** If anything else is found, make sure it was specified in the template.
    */

    for (d = dstart; d < dend;)
    {
	switch (*d)
	{
	case 'd':
	case 'D':
	    if (db_found)
	    {
		CMnext(d);
		if (d < dend && CMcmpnocase(d, ERx("b")) == 0)
		{
		    negative = TRUE;
		}
		else
		{
		    return afe_error(cb, E_FM2000_EXPECTED_CHAR_INSTEAD, 4,
				     1, (PTR) ERx("b"), CMbytecnt(d), (PTR) d);
		}
	    }
	    /* template doesn't contain this as escaped character */
	    else if (!f_escchrin(t, d))
	    {
		return afe_error(cb, E_FM2006_BAD_CHAR, 2,
				 CMbytecnt(d), (PTR) d);
	    }
	    break;

	case 'c':
	case 'C':
	    if (cr_found)
	    {
		CMnext(d);
		if (d < dend && CMcmpnocase(d, ERx("r")) == 0)
		{
		    negative = TRUE;
		}
		else
		{
		    return afe_error(cb, E_FM2000_EXPECTED_CHAR_INSTEAD, 4,
				     1, (PTR) ERx("r"), CMbytecnt(d), (PTR) d);
		}
	    }
	    /* template doesn't contain this as escaped character */
	    else if (!f_escchrin(t, d))
	    {
		return afe_error(cb, E_FM2006_BAD_CHAR, 2,
				 CMbytecnt(d), (PTR) d);
	    }
	    break;

	case FC_MINUS:
	    if (escaped_minus_found)
	    {
		/* ignore it */
	    }
	    else if (minus_found || plus_found)
	    {
		negative = TRUE;
	    }
	    else
	    {
		return afe_error(cb, E_FM2006_BAD_CHAR, 2,
				 CMbytecnt(d), (PTR) d);
	    }
	    break;

	case FC_OPAREN:
	    if (escaped_oparen_found)
	    {
		/* ignore it */
	    }
	    else if (oparen_found)
	    {
		negative = TRUE;
	    }
	    else
	    {
		return afe_error(cb, E_FM2006_BAD_CHAR, 2,
				 CMbytecnt(d), (PTR) d);
	    }
	    break;

	case FC_CPAREN:
	    if (escaped_cparen_found)
	    {
		/* ignore it */
	    }
	    else if (cparen_found)
	    {
		negative = TRUE;
	    }
	    else
	    {
		return afe_error(cb, E_FM2006_BAD_CHAR, 2,
				 CMbytecnt(d), (PTR) d);
	    }
	    break;

	case FC_OANGLE:
	    if (escaped_oangle_found)
	    {
		/* ignore it */
	    }
	    else if (oangle_found)
	    {
		negative = TRUE;
	    }
	    else
	    {
		return afe_error(cb, E_FM2006_BAD_CHAR, 2,
				 CMbytecnt(d), (PTR) d);
	    }
	    break;

	case FC_CANGLE:
	    if (escaped_cangle_found)
	    {
		/* ignore it */
	    }
	    else if (cangle_found)
	    {
		negative = TRUE;
	    }
	    else
	    {
		return afe_error(cb, E_FM2006_BAD_CHAR, 2,
				 CMbytecnt(d), (PTR) d);
	    }
	    break;

	case FC_OCURLY:
	    if (escaped_ocurly_found)
	    {
		/* ignore it */
	    }
	    else if (ocurly_found)
	    {
		negative = TRUE;
	    }
	    else
	    {
		return afe_error(cb, E_FM2006_BAD_CHAR, 2,
				 CMbytecnt(d), (PTR) d);
	    }
	    break;

	case FC_CCURLY:
	    if (escaped_ccurly_found)
	    {
		/* ignore it */
	    }
	    else if (ccurly_found)
	    {
		negative = TRUE;
	    }
	    else
	    {
		return afe_error(cb, E_FM2006_BAD_CHAR, 2,
				 CMbytecnt(d), (PTR) d);
	    }
	    break;

	case FC_OSQUARE:
	    if (escaped_osquare_found)
	    {
		/* ignore it */
	    }
	    else if (osquare_found)
	    {
		negative = TRUE;
	    }
	    else
	    {
		return afe_error(cb, E_FM2006_BAD_CHAR, 2,
				 CMbytecnt(d), (PTR) d);
	    }
	    break;

	case FC_CSQUARE:
	    if (escaped_csquare_found)
	    {
		/* ignore it */
	    }
	    else if (csquare_found)
	    {
		negative = TRUE;
	    }
	    else
	    {
		return afe_error(cb, E_FM2006_BAD_CHAR, 2,
				 CMbytecnt(d), (PTR) d);
	    }
	    break;

	case FC_STAR:
	    if (!star_found)
	    {
		return afe_error(cb, E_FM2006_BAD_CHAR, 2,
				 CMbytecnt(d), (PTR) d);
	    }
	    break;

	case FC_PLUS:
	    if (!plus_found)
	    {
		return afe_error(cb, E_FM2006_BAD_CHAR, 2,
				 CMbytecnt(d), (PTR) d);
	    }
	    break;

	default:
	    if (CMdigit(d) || *d == decimal_sym)
	    {
		*n++ = *d;
	    }
	    else if (CMwhite(d))
	    {
		if (blank_found && CMspace(d))
		{
			; /* blanks are OK if they're int the template */
		}
		/*
		** 34760: Dont allow spaces in the middle of
		** a number to be ignored.
		*/
		else if( n != number->db_t_text )
		{
			return afe_error(cb, E_FM2006_BAD_CHAR, 2,
				 CMbytecnt(d), (PTR) d);
		}
	    }
	    else if (CMcmpcase(d, mny_sym) == 0)
	    {
		if (mny_found)
		{
		    /* compare entire currency symbol */
		    m = mny_sym;
		    for (CMnext(d), CMnext(m); d < dend && CMcmpcase(d, m) == 0;
			 CMnext(d), CMnext(m))
			;

		    if (*m != EOS)
		    {	/* input was different from currency symbol */
			if (d < dend)
			{   /* input differs after the first char of symbol */
			    return afe_error(cb, E_FM2000_EXPECTED_CHAR_INSTEAD,
					     4, (i4) STlength(m), (PTR) m,
					     CMbytecnt(d), (PTR) d);
			}
			else
			{   /* input was cut off */
			    return afe_error(cb, E_FM2000_EXPECTED_CHAR_INSTEAD,
					     4, (i4) STlength(m), (PTR) m,
					     0, NULL);
			}
		    }
		    continue;	/* d is already incremented */
		}
		else
		{
		    return afe_error(cb, E_FM2006_BAD_CHAR, 2,
				     CMbytecnt(d), (PTR) d);
		}
	    }
	    else if (*d == thous_sym)
	    {
		if (!thous_found)
		{
		    return afe_error(cb, E_FM2006_BAD_CHAR, 2,
				     CMbytecnt(d), (PTR) d);
		}
	    }
	    else if (!f_escchrin(t, d))
	    {	/* template doesn't contain this as escaped character */
		return afe_error(cb, E_FM2006_BAD_CHAR, 2,
				 CMbytecnt(d), (PTR) d);
	    }
	    break;
	}
	CMnext(d);
    }

    number->db_t_count = n - number->db_t_text;

    if (negative)
    {	/* prepend a '-' before the number */
	f_strrgt(number->db_t_text, number->db_t_count,
		 number->db_t_text, number->db_t_count+1);
	number->db_t_count++;
	number->db_t_text[0] = FC_MINUS;
    }

    /* finally convert from double float to the type of the target value */

    dvfrom.db_datatype = DB_LTXT_TYPE;
    dvfrom.db_length = sizeof(buffer);
    dvfrom.db_data = (PTR) &buffer;

    return afe_cvinto(cb, &dvfrom, value);
}

/*{
** Name:	f_escchrin	- find whether an escaped character is
**				  in a string.
**
** Description:
**	This routine determines whether a given character (preceded by
**	the escape character FC_ESCAPE) is in a given string, case ignored.
**
** Inputs:
**	string		The null-terminated string in which to look
**			for the character.
**
**	character	Pointer to the character to look for.
**
** Outputs:
**	Returns:
**		TRUE if the escape character and the given character was
**			found in the string.
**		FALSE otherwise.
**
** History:
**	5-jan-1987 (grant)	implemented.
*/

bool
f_escchrin(string, character)
char	*string;
char	*character;
{
    for (; *string; CMnext(string))
    {
	if (*string == FC_ESCAPE)
	{
	    CMnext(string);
	    if (CMcmpnocase(string, character) == 0)
	    {
		return TRUE;
	    }
	}
    }

    return FALSE;
}


/*{
** Name:	f_substr	- find whether a string is in another string.
**
** Description:
**	This routine determines whether a given string is a substring of
**	another string, case ignored.
**
** Inputs:
**	string		The null-terminated string in which to look
**			for the other string.
**
**	substring	The null-terminated substring to look for.
**			This must be lower case.
**
** Outputs:
**	Returns:
**		TRUE if the substring is found in the string.
**
** History:
**	5-jan-1987 (grant)	implemented.
*/

bool
f_substr(string, substring)
char	*string;
char	*substring;
{
    char	*s;
    char	*ss;

    for (; *string; CMnext(string))
    {
	if (*string == FC_ESCAPE)
	{
	    CMnext(string);
	    continue;
	}

	for (s = string, ss = substring; *ss; CMnext(s), CMnext(ss))
	{
	    if (CMcmpnocase(s, ss) != 0)
	    {
		break;
	    }
	}

	if (*ss == EOS)
	{
	    return TRUE;
	}
    }

    return FALSE;
}
