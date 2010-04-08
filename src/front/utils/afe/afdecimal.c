/*
**	Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<cm.h>
# include	<adf.h>
# include	<afe.h>

/**
** Name:	afdecimal.c -	packed-decimal utilities
**
** Description:
**	This file contains and defines:
**
**	afe_dec_info	Returns the length, precision and scale for a 
**			decimal constant, given a str representation.
**
** History:
**	24-jun-1988	(billc)	Written
**	17-aug-1988 	(bjb)	Refined acceptable syntax
**	13-jul-1990 (barbara)
**	    Resurrected this file for 6.5/Orion decimal support.
**	22-feb-93 (davel)
**		Bug 49605 - return FAIL if precision > DB_MAX_DECPREC.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/*{
** Name:afe_dec_info()	-	find precision/scale of a decimal constant,
**				given a string representation.
**
** Description:
**	Finds the length, precision, and scale of a DECIMAL constant and
**	puts the info through the appropriate pointers. 
**
**	Acceptable syntax is:
**	    [ + | - ] digit {digit} [ . {digit} ]
**	Examples:
**	    +4.67
**	    -50
**	    95.
**	or
**	    [ + | - ] . digit {digit }
**	Examples:
**	    +.97
**	    -.4
**	    .999
**
** Inputs:
**	decstr			A string representation of a decimal literal.
**
**	decpt			Decimal point used in string.
**
** Outputs:
**	lenp			The byte length of the internal format of
**				the decimal value.
**	precp			The precision (number of digits) in the
**				decimal literal constant.
**	scalep			The scale (number of digits following the
**				decimal point) in the decimal literal constant.
** Returns:
**	OK			Success.
**	FAIL			Badly-formed string.
**
** History:
**	24-jun-1988	(billc)	Written.
**	17-aug-1988	(bjb)	Refined acceptable syntax.
*/


STATUS
afe_dec_info(decstr, decpt, lenp, precp, scalep)
char 	*decstr;
char	decpt;
i4  	*lenp;
i4  	*precp;
i4  	*scalep;
{
    i4  	scale = 0;
    i4  	prec = 0;

    *lenp = *precp = *scalep = 0;

    if (decstr == NULL)
	return FAIL;

    while (CMwhite(decstr))		/* Skip leading white space */
	CMnext(decstr);

    if (*decstr == '-' || *decstr == '+')	/* Skip leading sign */
	CMnext(decstr);

    while (CMdigit(decstr))		/* Count digits before dec point */
    {
	CMnext(decstr);
	prec++;
    }
    if (*decstr == decpt)
    {
	CMnext(decstr);			/* Count digits after dec point */
	while (CMdigit(decstr))
	{
	    CMnext(decstr);
	    scale++;
	    prec++;
	}
    }
    if (prec == 0 || prec > DB_MAX_DECPREC)	/* ensure valid precision */
	return FAIL;

    while (CMwhite(decstr))		/* Skip trailing white space */
	CMnext(decstr);

    if (*decstr != EOS)
	return FAIL;

    *precp = prec;
    *scalep = scale;
    *lenp = DB_PREC_TO_LEN_MACRO(prec);

    return OK;
}

