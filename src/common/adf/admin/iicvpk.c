/*
** Copyright (c) 2004 Ingres Corporation
*/

#include    "iipk.h"
#include    <iiadd.h>

/**
**
**  Name: IICVPK.C - IICV routines for packed decimal.
**
**  Description:
**      This file contains all of the IICV routines necessary to convert 
**      packed decimal values into/from other entities.
**
**	This file defines the following routines:
**
**          IICVpka() - Convert packed decimal to string.
**          IICVapk() - Convert string to packed decimal.
**          IICVpkf() - Convert packed decimal to float.
**
**  History:
**      03-may-88 (thurston)
**          Initial creation.
**      17-feb-1994 (stevet)
**          Minor changes to fix ANSI C compiler warnings.
**      13-Sep-2002 (hanal04) Bug 108637
**          Mark 2232 build problems found on DR6. Include iiadd.h
**          from local directory, not include path.
**      07-Jan-2003 (hanch04)  
**          Back out change for bug 108637
**          Previous change breaks platforms that need ALIGNMENT_REQUIRED.
**          iiadd.h should be taken from $II_SYSTEM/ingres/files/iiadd.h
**      29-Aug-2004 (kodse01)
**          BUG 112917
**          Added function IICVl8pk to convert 8 byte integer to packed
**          decimal.
**	26-Oct-2005 (hanje04)
**	    Add prototype for IICV0decerr, to prevent compiler errors
**	    under GCC 4.0 due to conflicts with implicit prototypes.
**/

/* local prototypes */
static II_STATUS IICV0decerr(
			II_STATUS	status,
			PTR	pkbuff,
			int	prec);
/*
**{
** Name:    byte_copy	- copy bytes from one place to another
**
** Description:
**	Simply copies one string of bytes from one place to another.
**
*/
static void
byte_fill(length, c_from, c_to)
u_i2		length;
u_char		c_from;
u_char		*c_to;
{
    u_i2    i;

    for (i = 0;
	    i < length;
	    i++, c_to++)
    {
	*c_to = c_from;
    }
}

/*
[@forward_type_references@]
[@forward_function_references@]
[@#defines_of_other_constants@]
[@type_definitions@]
[@global_variable_definitions@]
[@static_variable_or_function_definitions@]
*/

/* IICMdigit - test for digit */
#define     IICMdigit(str)        (*str >= '0' && *str <= '9' ? TRUE : FALSE)
/* IICMnext - increment a character string pointer */
#define     IICMnext(str)         (++(str))
/* IICMwhite - check white space */
#define     IICMwhite(str)        (*str == ' ' ? TRUE : FALSE)



/*{
** Name: IICVpka() - Convert packed decimal to string.
**
** Description:
**      Converts a packed decimal value into a null termiinted character
**      string.  The caller describes how the output string should be
**	formatted by supplying information on the field width to use,
**	the number of fractional digits, whether to truncate or round,
**	whether to right or left justify, and whether to use `zero
**	filling' or not.
**
**	Usage notes:
**	------------
**	    The output string is assumed to be at least
**	    `field_wid' + 1 bytes long.
**
**	    Negative zero is printed as positive 0.
**
**	    No sign will be displayed for positive numbers.  For negative
**	    numbers, a minus sign will be displayed just to the left of the
**	    left most digit displayed.
**
**          No decimal point will be displayed if the number of
**          fractional digits being requested is zero.
**
**          By default (i.e. not using IICV_PKROUND), if there are more
**	    scale digits in the decimal value than fractional digits
**	    requested, the extra digits will be truncated.
**
**          By default (i.e. not using IICV_PKLEFTJUST), the output string
**          will be right justified, with blank padding on the left if
**	    necessary.  No blank padding on the right will be done when
**	    using IICV_PKLEFTJUST; the null termiintor will immediately
**	    follow the last displayed digit.
**
**          By default (i.e. not using IICV_PKZEROFILL), leading zeros are
**          converted to blanks.  However, if all digits in integral
**	    portion of number are 0 then a single 0 is output, unless
**	    that would cause the IICV_OVERFLOW condition.  Note that this option
**	    has no effect if IICV_PKLEFTJUST was requested too.
**
**	Examples:
**	---------
**	    pk (prec,scale) = 10.456 (5,3)
**	    field_wid = 10    frac_digs = 2
**	    options = 0;
**			II_STATUS will be:  IICV_TRUNCATE
**			string will be:  "     10.45"
**
**	    pk (prec,scale) = 10.456 (5,3)
**	    field_wid = 10    frac_digs = 2
**	    options = IICV_PKLEFTJUST;
**			II_STATUS will be:  IICV_TRUNCATE
**			string will be:  "10.45"
**
**	    pk (prec,scale) = -10.456 (5,3)
**	    field_wid = 10    frac_digs = 2
**	    options = IICV_PKLEFTJUST;
**			II_STATUS will be:  IICV_TRUNCATE
**			string will be:  "-10.45"
**
**	    pk (prec,scale) = 10.456 (5,3)
**	    field_wid = 5     frac_digs = 2
**	    options = IICV_PKLEFTJUST;
**			II_STATUS will be:  IICV_TRUNCATE
**			string will be:  "10.45"
**
**	    pk (prec,scale) = -10.456 (5,3)
**	    field_wid = 5     frac_digs = 2
**	    options = IICV_PKLEFTJUST;
**			II_STATUS will be:  IICV_OVERFLOW
**			string will be:  <undefined>
**
**	    pk (prec,scale) = 10.456 (5,3)
**	    field_wid = 10    frac_digs = 2
**	    options = IICV_PKZEROFILL;
**			II_STATUS will be:  IICV_TRUNCATE
**			string will be:  "0000010.45"
**
**	    pk (prec,scale) = -10.456 (5,3)
**	    field_wid = 10    frac_digs = 2
**	    options = IICV_PKZEROFILL;
**			II_STATUS will be:  IICV_TRUNCATE
**			string will be:  "-000010.45"
**
**	    pk (prec,scale) = -10.456 (5,3)
**	    field_wid = 10    frac_digs = 2
**	    options = IICV_PKLEFTJUST | IICV_PKZEROFILL;
**			II_STATUS will be:  IICV_TRUNCATE
**			string will be:  "-10.45"
**
**	    pk (prec,scale) = .45 (2,2)
**	    field_wid = 10    frac_digs = 2
**	    options = 0;
**			II_STATUS will be:  OK
**			string will be:  "      0.45"
**
**	    pk (prec,scale) = .45 (2,2)
**	    field_wid = 3     frac_digs = 2
**	    options = 0;
**			II_STATUS will be:  OK
**			string will be:  ".45"
**
**	    pk (prec,scale) = .456 (3,3)
**	    field_wid = 3     frac_digs = 2
**	    options = IICV_PKROUND;
**			II_STATUS will be:  IICV_TRUNCATE
**			string will be:  ".46"
**
**
** Inputs:
**      pk                              Pointer to the decimal value.
**      prec                            Its precision:  1 <= prec <= 31.
**      scale                           Its scale:  0 <= scale <= prec.
**      decpt                           Decimal character in use.  If
**					this is zero then '.' is used.
**	field_wid			Output field width; that is, the
**					number of characters to use in
**					displaying the decimal value.
**					It is assumed that `string' is
**					at least `field_wid' + 1 bytes
**					long.
**	frac_digs			The number of digits to place to
**					the right of the decimal point.
**	options				Formatting options; any
**					combiintion of the following
**					may be "OR'ed" together:
**		IICV_PKROUND		Do rounding instead of truncation, which
**					is the default.
**		IICV_PKLEFTJUST		Left justify the output instead of right
**					justifying, which is the default.
**		IICV_PKZEROFILL		Fill up the entire field width by
**					placing however many leading zeros
**					required to do so.  Default is to blank
**					pad.  As an example, the number -10.453
**					placed into an output string where
**					`field_wid' = 8 and `frac_digs' = 2
**					will normally be displayed as
**					"  -10.45".  If the IICV_PKZEROFILL option
**					is being used it will be displayed as
**					"-0010.45".
**
** Outputs: 
**      str                             The null termiinted string
**                                      representing the decimal value
**					in the format specified.
**
**	res_wid				Number of characters used in str.
**
**	Returns:
**	    OK				Success.
**
**	    IICV_TRUNCATE			Some fractional digits were
**					either truncated or rounded in
**					order to use `frac_digs'.
**
**	    IICV_OVERFLOW			String was not big enough to
**					hold the output in the desired
**					format.
**
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      05-may-88 (jrb)
**          Initial creation and coding.
**      26-may-88 (thurston)
**          Respecified the interface to allow for much more flexibility
**	    in specifying the output format.
**	20-jul-89 (jrb)
**	    Re-wrote according to CL-approved spec.
*/

II_STATUS
IICVpka(pk, prec, scale, decpt, field_wid, frac_digs, options, str, res_wid)
PTR	    pk;
int	    prec;
int	    scale;
char	    decpt;
int	    field_wid;
int	    frac_digs;
int	    options;
char	    *str;
int	    *res_wid;
{
    char	dpoint = (decpt ? decpt : '.');
    char	*p = pk;
    int		pr = prec;
    int		sc = scale;
    int		nz_pr;
    int		remain_wid;
    int		isneg;
    char	*s = str;
    u_char	sign;
    

    /* is decimal number negative? */
    sign  = (p[prec/2] & 0xf);
    isneg = (sign == IIMH_PK_MINUS  ||  sign == IIMH_PK_AMINUS);

    /* now locate the first non-zero digit in the packed decimal input
    ** (scanning from left to right)
    */
    while (pr)
    {
	if (pr & 1)
	{
	    if (*p & 0xf0)
		break;
	}
	else
	{
	    if (*p & 0xf)
		break;
	    p++;
	}
	pr--;
    }		

    /* remember the position of the first non-zero digit */
    nz_pr = pr;

    /* if the number was zero, make sure it's positive zero */
    if (pr == 0)
	isneg = FALSE;

    /* figure remaining width by deducting 1 for '-' sign and 1 for decpt
    ** and 1 if both frac_digs and the number itself were 0 (because then we're
    ** going to need a single 0 on output)
    */
    remain_wid =
	field_wid - isneg - (frac_digs > 0) - (nz_pr == 0  &&  frac_digs == 0);

    /* now if first non-zero digit was in the fractional portion of the
    ** number, reset pointers to the first fractional digit of the number
    */
    if (pr < sc)
    {
	p  = (char *)pk + prec/2 - scale/2;
	pr = sc;
    }

    /* figure out how much remaining space is left (formula is: remaining
    ** width minus integral digits and fractional digits)
    */
    remain_wid -= (pr - sc) + frac_digs;

    if (remain_wid < 0)
    {
	/* could not fit number into field; fill with asterisks */
	byte_fill((u_i2)field_wid, (u_char)'*', (PTR)str);
	str[field_wid] = EOS;
	*res_wid = field_wid;
	return(IICV_OVERFLOW);
    }

    /* at this point remain_wid contains the amount of leading space available
    ** for filling with blanks or zeros depending on what is desired (and
    ** neither if we are left-justifying)
    */
    if ((options & IICV_PKLEFTJUST) == 0)
    {
	if (options & IICV_PKZEROFILL)
	{
	    if (isneg)
		*s++ = '-';
		
	    while (remain_wid)
	    {
		*s++ = '0';
		remain_wid--;
	    }
	}
	else
	{
	    /* in this mode we want to reserve a space for a leading zero if
	    ** there aren't any leading non-zero digits and there is room for
	    ** a leading zero
	    */
	    int	    want_lead = (pr == sc  &&  frac_digs > 0);

	    while (remain_wid > want_lead)
	    {
		*s++ = ' ';
		remain_wid--;
	    }

	    if (isneg)
		*s++ = '-';

	    if (remain_wid > 0  &&  want_lead)
		*s++ = '0';
	}
    }
    else
    {
	/* if left-justifying, just put sign and put leading zero if no non-zero
	** integral digits and sufficient room exists and there will be some
	** fractional digits
	*/
	if (isneg)
	    *s++ = '-';

	if (pr == sc  &&  remain_wid > 0  &&  frac_digs > 0)
	    *s++ = '0';
    }

    /* we must put at least one digit out; if the number is zero and the
    ** number of fractional digits requested was zero, then put a 0 (room for
    ** this was already accounted for in remain_wid above)
    */
    if (nz_pr == 0  &&  frac_digs == 0)
	*s++ = '0';

    /* we won't use remain_wid anymore; from here on we should just put out
    ** the number and everything should fit correctly
    */
    while (pr > sc - frac_digs)
    {
	if (pr == sc)
	    *s++ = dpoint;

	if (pr > 0)
	    *s++ = '0' + (((pr & 1)  ?  *p >> 4  :  *p++) & 0xf);
	else
	    *s++ = '0';

	pr--;
    }

    /* now do rounding if indicated; note that this code is inelegant, but
    ** I decided to do it this way since the special cases which this code must
    ** handle probably won't occur too frequently, and I didn't want the rest
    ** of this routine to pay the performance penalty involved with checking
    ** for these special cases earlier
    */
    if (options & IICV_PKROUND)
    {
	char	*ts = s;

	/* if any digits left, check if next one is >= 5 */
	if (pr > 0  &&  (((pr & 1)  ?  *p >> 4  :  *p) & 0xf) >= 5)
	{
	    /* go back thru str adding 1 and carrying when appropriate */
	    while (--ts >= str)
	    {
		if (*ts == dpoint)
		    ts--;

		if (!IICMdigit(ts))
		    break;
		    
		if (*ts < '9')
		{
		    (*ts)++;
		    break;
		}
		*ts = '0';
	    }

	    /* four cases are possible here:
	    **	 1) we broke out of previous loop after rounding (so exit)
	    **   2) we ran off the end of str (so overflow)
	    **	 3) we hit a '-' sign (so move it over 1 if we can, else ovf)
	    **	 4) we hit a ' ' (so replace it with a 1 if we can, else ovf)
	    */
	    if (ts < str  ||  !IICMdigit(ts))
	    {
		if (ts > str  &&  *ts == '-')
		{
		    *ts-- = '1';
		    *ts   = '-';
		}
		else if (ts >= str  &&  *ts == ' ')
		{
		    *ts = '1';
		}
		else
		{
		    /* could not fit number into field; fill with asterisks */
		    byte_fill((u_i2)field_wid, (u_char)'*', (PTR)str);
		    str[field_wid] = EOS;
		    *res_wid = field_wid;
		    return(IICV_OVERFLOW);
		}
	    }
	}
    }

    *s = EOS;
    *res_wid = s - str;

    /* find out if we truncated/rounded anything */
    while (pr > 0)
    {
	if (((pr-- & 1)  ?  *p >> 4  :  *p++) & 0xf)
	    return(IICV_TRUNCATE);
    }

    return(OK);
}


/*{
** Name: IICVapk() - Convert string to packed decimal.
**
** Description:
**      Converts a null termiinted character string into a packed decimal value
**	with the requested precision and scale.  Acceptable syntax is:
**
**		[ '+' | '-' ] digit {digit} [ '.' {digit} ]
**
**			Examples:   1
**				    +1234
**				    12.
**				    -12.345
**
**	... or ...
**
**		[ '+' | '-' ] '.' digit {digit}
**
**			Examples:   .1
**				    +.1234
**				    -.12
**
**	Leading and trailing white space and leading zeros are ignored.
**	The II_STATUS returned will always be the most severe II_STATUS encountered;
**	all II_STATUSes are listed below in order of increasing severity.
**
** Inputs:
**      string                          The null termiinted string representing
**                                      the decimal value.
**      decpt                           Decimal character in use.  If this
**					is '\0' then it is set to '.'
**      prec                            Precision for result:  1 <= prec <= 31.
**      scale                           Scale for result:  0 <= scale <= prec.
**
** Outputs:
**      pk                              The resulting packed decimal value is
**					placed here.
**
**	Returns:
**	    OK				Success.
**
**	    IICV_TRUNCATE			Some fractional digits had to be
**					truncated in order to use scale.
**
**	    IICV_OVERFLOW			Integral part has too many digits
**					(not including leading zeros)
**					i.e. more than (prec - scale) of them.
**
**	    IICV_SYNTAX			Could not convert due to syntax error.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      09-may-88 (jrb)
**          Initial creation and coding.
**	24-jul-89 (jrb)
**	    Now calls IICV0decerr before returning so that result will be zeroed
**	    if serious error was encountered.
*/

II_STATUS
IICVapk(string, decpt, prec, scale, pk)
char	    *string;
char	    decpt;
int	    prec;
int	    scale;
PTR	    pk;
{
    char	*x = string;		/* for traipsing thru string */
    char	*y;			/* ditto */
    char	dpoint = (decpt ? decpt : '.');
    u_char	*pkdec = (u_char *)pk;	/* pointer to output variable */
    int		nlz;			/* number of leading zeros */
    int		i = prec;		/* for counting digits */
    int		neg = FALSE;		/* true if number is negative */
    int		founddig = FALSE;	/* true if digits have been found */
    II_STATUS	ret_stat = OK;		/* most severe return II_STATUS so far */

    /* zero out first byte in case prec is even */
    *pkdec = (u_char)0;

    /* strip leading whitespace */
    while (IICMwhite(x))
	IICMnext(x);

    /* if nothing but whitespace then return 0 */
    if (*x == '\0')
    {
	while (i)
	{
	    if (i-- & 1)
		*pkdec = (u_char)0;
	    else
		pkdec++;
	}
	*pkdec = IIMH_PK_PLUS;
	
	return(OK);
    }

    /* check for leading sign */
    if (*x == '-')
    {
	neg = TRUE;
	x++;
    }
    else
    {
	if (*x == '+')
	    x++;
	else
	    if (!IICMdigit(x) && *x != dpoint)
		return(IICV0decerr(IICV_SYNTAX, pk, prec));
    }
    
    /* strip leading zeros */
    if (*x == '0')
    {
	founddig = TRUE;
	while (*x == '0')
	    x++;
    }

    /* mark off integral part using x and y */
    for(y = x; IICMdigit(y); y++)
	;
    founddig = founddig || (x < y);

    /* calculate number of leading zeros and do error-check */
    if ((nlz = (prec - scale) - (y - x)) < 0)
    {
	ret_stat = IICV_OVERFLOW;
	x = y;
	i = -1;		/* disallow any further digits into pkdec */
    }
    else
    {
	/* pad pkdec with zeros */
	while (i > prec - nlz)
	    if (i-- & 1)
		*pkdec = (u_char)0;
	    else
		pkdec++;

	/* now copy digits from string (if any) into pkdec */
	while (x < y)
	    if (i-- & 1)
		*pkdec = (u_char)(((*x++ - '0') << 4) & 0xf0);
	    else
		*pkdec++ |= (u_char)(*x++ - '0');
    }

    /* skip over decimal point */
    if (*x == dpoint)
	x++;
    else
	if (*x != '\0'  &&  !IICMwhite(x))
	    return(IICV0decerr(IICV_SYNTAX, pk, prec));

    /* a packed decimal number must have at least one digit */
    if (!founddig  &&  !IICMdigit(x))
	return(IICV0decerr(IICV_SYNTAX, pk, prec));

    /* copy fractional part of string into pkdec */
    while (IICMdigit(x)  &&  i > 0)
	if (i-- & 1)
	    *pkdec = (u_char)(((*x++ - '0') << 4) & 0xf0);
	else
	    *pkdec++ |= (u_char)(*x++ - '0');
	
    /* now pad with trailing zeros if room exists */
    while (i > 0)
	if (i-- & 1)
	    *pkdec = (u_char)0;
	else
	    pkdec++;
	
    /* set proper sign now that pkdec points to last byte of output */
    if (neg)
	*pkdec |= IIMH_PK_MINUS;
    else
	*pkdec |= IIMH_PK_PLUS;

    /* if there are still digits left, ignore them */
    if (IICMdigit(x))
    {
	if (ret_stat == OK)
	    ret_stat = IICV_TRUNCATE;
	while (IICMdigit(x))
	    x++;
    }

    while (IICMwhite(x))
	IICMnext(x);
    
    if (*x != '\0')
	ret_stat = IICV_SYNTAX;

    return(IICV0decerr(ret_stat, pk, prec));
}


/*{
** Name: IICVpkf() - Convert packed decimal to float.
**
** Description:
**      Converts a packed decimal value into an eight byte float.
**
** Inputs:
**      pk                              Pointer to the decimal value.
**      prec                            Its precision:  1 <= prec <= 31.
**      scale                           Its scale:  0 <= scale <= prec.
**
** Outputs:
**      fnum                            The eight byte float result.
**
**	Returns:
**	    OK				Success.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      11-may-88 (jrb)
**          Initial creation and coding.
**	21-feb-89 (jrb)
**	    Fixed a bug where too many leading zeros were being skipped before
**	    we processed the number.
**	04-feb-91 (jrb)
**	    Changed code to get around a C compiler bug on SUN4 that generates
**	    incorrect code when coercing unsigned to f8 (for Dave Hung).
**      17-feb-1994 (stevet)
**          Minor changes to fix ANSI C compiler warnings.
*/

II_STATUS
IICVpkf(pk, prec, scale, fnum)
PTR	    pk;
int	    prec;
int	    scale;
f8	    *fnum;
{
    f8		val = 0.0;		/* accumulator for fnum */
    f8		npt = 0.1;		/* negative powers of ten */
    u_char	*p = (u_char *)pk;	/* for walking thru pk */
    int		pr = prec;
    u_char	sign;			/* sign code found in pk */
    int		temp;

    /* read the sign code from pk */
    sign = p[prec / 2] & 0xf;

    /* skip past leading pairs of zeros for the sake of efficiency */
    if ((pr & 1) == 0  &&  (*p & 0xf) == (u_char)0)
    {
	p++;
	pr--;
    }
    while (*p == (u_char)0  &&  pr > scale + 1)
    {
	p++;
	pr -= 2;	/* skip down to next odd number */
    }

    /* first process integral part of decimal number */
    while (pr > scale)
    {
	temp = (pr-- & 1  ?  *p >> 4 : *p++) & 0xf;
	val = val * 10.0 + temp;
    }

    /* and now add the fractional digits */
    while (pr > 0)
    {
 	temp = (pr-- & 1  ?  *p >> 4 : *p++) & 0xf;
	val += npt * temp;
	npt /= 10.0;
    }

    /* set the sign to minus if appropriate */
    if (sign == IIMH_PK_MINUS  ||  sign == IIMH_PK_AMINUS)
	val = -val;

    *fnum = val;
    return(OK);
}


/*{
** Name: IICV0decerr() - Check return II_STATUS and zero out result if error
**
** Description:
**	This routine checks the return II_STATUS and if it is either IICV_OVERFLOW
**	or IICV_SYNTAX then it zeros out the result decimal value.  It always
**	returns the same II_STATUS that was given to it.
**	
** Inputs:
**	II_STATUS			Return II_STATUS of calling routine
**	pkbuff			Output decimal buffer to be zeroed
**	prec			Precision of the output decimal buffer
**	
** Outputs:
**      pkbuff			Is filled with 0's
**
**      Returns:
**	    II_STATUS		whatever was sent to it, it sends back
**  
**      Exceptions:
**	    none
**
** Side Effects:
**          none
**
** History:
**      24-jul-89 (jrb)
**          Initial creation and coding.
*/
static II_STATUS
IICV0decerr(status, pkbuff, prec)
II_STATUS	status;
PTR	pkbuff;
int	prec;
{
    if (status == IICV_OVERFLOW  ||  status == IICV_SYNTAX)
    {
	/* zero out all but the last byte; then set the last byte with a + sign */
	byte_fill((u_i2)(prec / 2), (u_char)0, pkbuff);
	*((u_char *)pkbuff + prec / 2) = IIMH_PK_PLUS;
    }
    
    return(status);
}

/*{
** Name: IICVpkl() - Convert packed decimal to long integer.
**
** Description:
**      Converts a packed decimal value into a four byte integer.
**
** Inputs:
**      pk                              Pointer to the decimal value.
**      prec                            Its precision:  1 <= prec <= 31.
**      scale                           Its scale:  0 <= scale <= prec.
**
** Outputs:
**      num                             The four byte integer result.  This is
**                                      derived by truncating the fractional
**                                      part of the packed decimal, not by
**                                      rounding. 
**
**      Returns:
**          OK                          Success.
**          IICV_OVERFLOW                 Number too big to fit in an i4.
**
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      10-may-88 (jrb)
**          Initial creation and coding.
**      17-feb-1994 (stevet)
**          Minor changes to fix ANSI C compiler warnings.
*/

II_STATUS
IICVpkl(pk, prec, scale, num)
PTR         pk;
int         prec;
int         scale;
int         *num;
{
    long     val = 0;                /* accumulator for num */
    long     ovrchk;                 /* for checking overflow */
    u_char      *p = (u_char *)pk;      /* for walking thru pk */
    int         pr = prec;
    u_char      sign;                   /* sign code found in pk */
    u_char      dig;                    /* digit extracted from pk */

    /* read the sign code from pk */
    sign = p[prec / 2] & 0xf;

    /* skip past leading pairs of zeros for the sake of efficiency */
    while (*p == (u_char)0 && pr > scale)
    {
        p++;
        pr -= pr & 1 ? 2 : 1;   /* skip down to next odd precision */
    }

    /* this routine could be coded more compactly, but I was shooting for */
    /* efficiency instead */
    
    /* if sign code is minus, do subtraction loop, else do addition loop */
    if (sign == IIMH_PK_MINUS  ||  sign == IIMH_PK_AMINUS)
    {
        ovrchk = MINI4 / 10;

        while (pr > scale)
        {
            dig = (pr-- & 1  ?  *p >> 4 : *p++) & 0xf;

            if (val > ovrchk || (val == ovrchk && dig <= ((u_char) abs(MINI4 % 10))))
                val = val * 10 - dig;
            else
                return(IICV_OVERFLOW);
        }
    }
    else
    {
        ovrchk = MAXI4 / 10;

        while (pr > scale)
        {
            dig = (pr-- & 1  ?  *p >> 4 : *p++) & 0xf;

            if (val < ovrchk || (val == ovrchk && dig <= ((u_char)(MAXI4 % 10))))
                val = val * 10 + dig;
            else
                return(IICV_OVERFLOW);
        }
    }
    *num = val;
    return(OK);
}


/*{
 ** Name: IICVlpk() - Convert long integer to packed decimal.
 **
 ** Description:
 **      Converts a four byte integer into a packed decimal value of the
 **      requested precision and scale.
 **
 ** Inputs:
 **      num                             The four byte integer.
 **      prec                            Precision for result:  1 <= prec <= 31.
 **      scale                           Scale for result:  0 <= scale <= prec.
 **
 ** Outputs:
 **      pk                              The resulting packed decimal value is
 **                                      placed here.
 **
 **      Returns:
 **          OK                          Success.
 **          IICV_OVERFLOW                 Too many non-fractional digits;
 **                                      i.e. more than (prec - scale) of them.
 **
 **      Exceptions:
 **          none
 **
 ** Side Effects:
 **          none
 **
 ** History:
 **      10-may-88 (jrb)
 **          Initial creation and coding.
 **      24-jul-89 (jrb)
 **          Now zeroes result on overflow.
 */
 
 II_STATUS
 IICVlpk(num, prec, scale, pk)
 int          num;
 int         prec;
 int         scale;
 PTR         pk;
 {
     int         pr = scale + 1;         /* no. of nybbles from the left */
     u_char      *p;                     /* points into pk */
 
     /* zero out entire decimal number */
     byte_fill((u_i2)(prec/2+1), (u_char)0, pk);
 
     /* insert sign code into pk and make num negative if it isn't already   */
     /* (we convert everything to negative instead of positive because MINI4 */
     /* has no positive counterpart)                                         */
     if (num < 0)
     {
         *((u_char *)pk + prec/2) = IIMH_PK_MINUS;
     }
     else
     {
         *((u_char *)pk + prec/2) = IIMH_PK_PLUS;
         num = -num;
     }
 
     /* set p to byte containing rightmost nybble of integral part of pk */
     p = (u_char *)pk + prec/2 - pr/2;
 
     /* insert digits from right to left into pk until full or num is 0 */
     while (num < 0 && pr <= prec)
     {
         if (pr++ & 1)
             *p-- |= (u_char)((-(num % 10) << 4) & 0xf0);
         else
             *p = (u_char)(-(num % 10));
         
         num /= 10;
     }
 
     /* if there's anything left, it was too big to fit */
     if (num < 0)
         return(IICV0decerr(IICV_OVERFLOW, pk, prec));
 
     return(II_OK);
}

/*{
** Name: IICVl8pk() - Convert long integer (eight byte) to packed decimal.
**
** Description:
**      Converts an eight byte integer into a packed decimal value of the
**      requested precision and scale.
**
** Inputs:
**      num                             The four byte integer.
**      prec                            Precision for result:  1 <= prec <= 31.
**      scale                           Scale for result:  0 <= scale <= prec.
**
** Outputs:
**      pk                              The resulting packed decimal value is
**                                      placed here.
**
**      Returns:
**          OK                          Success.
**          IICV_OVERFLOW                 Too many non-fractional digits;
**                                      i.e. more than (prec - scale) of them.
**
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      29-Aug-2004 (kodse01)
**          BUG 112917
**          Initial creation and coding. 
**          Same as IICVlpk except the type of parameter num is i8.
*/
 
II_STATUS
IICVl8pk(num, prec, scale, pk)
i8          num;
int         prec;
int         scale;
PTR         pk;
{
    int         pr = scale + 1;         /* no. of nybbles from the left */
    u_char      *p;                     /* points into pk */

    /* zero out entire decimal number */
    byte_fill((u_i2)(prec/2+1), (u_char)0, pk);

    /* insert sign code into pk and make num negative if it isn't already   */
    /* (we convert everything to negative instead of positive because MINI4 */
    /* has no positive counterpart)                                         */
    if (num < 0)
    {
        *((u_char *)pk + prec/2) = IIMH_PK_MINUS;
    }
    else
    {
        *((u_char *)pk + prec/2) = IIMH_PK_PLUS;
        num = -num;
    }

    /* set p to byte containing rightmost nybble of integral part of pk */
    p = (u_char *)pk + prec/2 - pr/2;

    /* insert digits from right to left into pk until full or num is 0 */
    while (num < 0 && pr <= prec)
    {
        if (pr++ & 1)
            *p-- |= (u_char)((-(num % 10) << 4) & 0xf0);
        else
            *p = (u_char)(-(num % 10));
        
        num /= 10;
    }

    /* if there's anything left, it was too big to fit */
    if (num < 0)
        return(IICV0decerr(IICV_OVERFLOW, pk, prec));

    return(II_OK);
}

/*{
 ** Name: IICVfpk() - Convert float to packed decimal.
 **
 ** Description:
 **      Converts an eight byte float into a packed decimal value of the
 **      requested precision and scale.
 **
 ** Inputs:
 **      fnum                            The eight byte float.
 **      prec                            Precision for result:  1 <= prec <= 31.
 **      scale                           Scale for result:  0 <= scale <= prec.
 **
 ** Outputs:
 **      pk                              The resulting packed decimal value is
 **                                      placed here.
 **
 **      Returns:
 **          OK                          Success.
 **          IICV_OVERFLOW                 Too many non-fractional digits;
 **                                      i.e. more than (prec - scale) of them.
 **
 **      Exceptions:
 **          none
 **
 ** Side Effects:
 **          none
 **
 ** History:
 **      17-may-88 (jrb)
 **          Initial creation and coding.
 **      27-feb-89 (jrb)
 **          Changed to pad with zeros when precision was > 15.  This is because
 **          we shouldn't try to get more than 15 digits of precision from an f8.
 **      19-jul-89 (jrb)
 **          More problems with accuracy.  I am now convinced that the only right
 **          way to do this in a machine-independent way is to convert to ascii
 **          first (which inevitably calls some OS function) and then to decimal.
 **          This is sort of slow, so this routine should be re-written for each
 **          environment to be machine-specific for better performance.
 **      24-jul-89 (jrb)
 **          Now zeroes result on overflow.
 */
 
 II_STATUS
 IICVfpk(fnum, prec, scale, pk)
 f8          fnum;
 int         prec;
 int         scale;
 PTR         pk;
 {
     u_char      buff[31+6];
     short       res_wid;
     int         digs;

    if (IICVfa(fnum, prec+5, scale, 'f', '.', buff, &res_wid) != OK)
        return(IICV_OVERFLOW);

    return(IICV0decerr(IICVapk(buff, '.', prec, scale, pk), pk, prec));
}


/*{
** Name: IICVpkpk() - Convert packed decimal to packed decimal.
**
** Description:
**      Converts a packed decimal value with one precision and scale to another
**      packed decimal value with a possibly different precision and scale.
**      Excess fractional digits are truncated.
**
** Inputs:
**      pk_in                           Pointer to the input decimal value.
**      prec_in                         Its precision:  1 <= prec_in <= 31.
**      scale_in                        Its scale:  0 <= scale_in <= prec_in.
**      prec_out                        Precision for result:
**                                      1 <= prec_out <= 31.
**      scale_out                       Scale for result:
**                                      0 <= scale_out <= prec_out.
**
**      Return II_STATUSes are listed in order of increasing severity; the
**      most severe II_STATUS encountered is always the one returned.
**
**
** Outputs:
**      pk_out                          The resulting packed decimal value is
**                                      placed here.
**
**      Returns:
**          OK                          Success.
**          IICV_OVERFLOW                 Too many non-fractional digits (not
**                                      counting leading zeros)
**                                      i.e. more than (prec_out - scale_out)
**                                      of them.
**
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      11-may-88 (jrb)
**          Initial creation and coding.
**      24-jul-89 (jrb)
**          Now zeroes result on overflow.
**      22-sep-89 (jrb)
**          Removed IICV_TRUNCATE as possible return II_STATUS.  It was not used
**          and had performance impacts.
*/

II_STATUS
IICVpkpk(pk_in, prec_in, scale_in, prec_out, scale_out, pk_out)
PTR         pk_in;
int         prec_in;
int         scale_in;
int         prec_out;
int         scale_out;
PTR         pk_out;
{
    u_char      *pi = (u_char *)pk_in;
    u_char      *po = (u_char *)pk_out;
    int         pin = prec_in;
    int         pout = prec_out;
    int         nlz;                            /* number of leading zeros */
    u_char      sign;

    /* get sign code of pk_in */
    sign = pi[pin/2] & (u_char)0xf;

    /* zero first byte of pk_out in case pout is even */
    *po = (u_char)0;

    /* skip leading zeros on pk_in (before decimal point) */
    while (pin > scale_in)
    {
        if (((pin & 1  ?  *pi >> 4  :  *pi) & 0xf) != (u_char)0)
            break;

        if ((pin-- & 1) == 0)
            pi++;
    }

    /* calculate nlz for pk_out and see if there's enough room */
    if ((nlz = (pout - scale_out) - (pin - scale_in)) < 0)
        return(IICV0decerr(IICV_OVERFLOW, pk_out, prec_out));

    /* put leading zeros into pk_out */
    while (nlz-- > 0)
        if (pout-- & 1)
            *po = (u_char)0;
        else
            po++;

    /* copy digits from pk_in to pk_out until one of them is full */
    while (pin > 0  &&  pout > 0)
        if (pout-- & 1)
            *po = (pin-- & 1  ?  *pi  :  *pi++ << 4) & 0xf0;
        else
            *po++ |= (pin-- & 1  ?  *pi >> 4  :  *pi++) & 0xf;

    /* if pk_in was full, pad pk_out with zeros to the right */
    if (pin == 0)
        while (pout > 0)
            if (pout-- & 1)
                *po = (u_char)0;
            else
                po++;
            
    /* now set the sign code on pk_out */
    *po |= sign;

    return(OK);
}


#define     MAXPREC             16
/* cmcpyinc - copy one character to another */
#define     cmcpyinc(src, dst)  (*(dst)++ = *(src)++)
/* cmupper - test for uppercases */
#define     cmupper(str)        (*str >= 'A' && *str <= 'Z' ? TRUE : FALSE)



/*{
** Name: cvfa	- Convert floating to ascii
**
** Description:
**
** Inputs:
**
** Outputs:
**
**	Returns:
**	    OK
**	    CV_TOO_WIDE
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	01-oct-1985 (derek)
**          Created new.
*/

# define	MAXDIGITS	128
# define	E_FORMAT	('e' & 0xf)
# define	F_FORMAT	('f' & 0xf)
# define	G_FORMAT	('g' & 0xf)
# define	N_FORMAT	('n' & 0xf)

II_STATUS
IICVfa(value, width, prec, format, decimal, ascii, res_width)
f8	value;		/*	value to be converted			*/
int	width;		/*	the width of the formatted item		*/
int	prec;		/*	number of digits of precision		*/
char	format;		/*	format type (e,f,g,n)			*/
char	decimal;	/*	decimal character to use		*/
char	*ascii;		/*	where to put the converted value	*/
short	*res_width;	/*	where to put width of the output field  */
{
    char	decchar = (decimal ? decimal : '.');
    int	overflow_width;	    /* # chars to fill with '*' on overflow */
    int	exp;		    /* exponent				    */
    int	digits;		    /* number of digits to convert	    */
    int	echar;		    /* either 'e' or 'E'		    */
    union	{
	char	buf[MAXDIGITS+2];   /* ascii digits "+" sign and 0  */
	int	dummy;		    /* dummy for byte alignment	    */
	}	buffer;

    buffer.dummy = 0;		/* always initialize--clear */

    overflow_width = width;

    /* determine the character to use as the exponent */
    echar = 'e';
    if (cmupper(&format))
	echar = 'E';

    format &= 0xf;	    /* normalize the format */

    if (prec > 0)
	width--;	    /* charge width with decimal point */
    if (value < 0.0)
	width--;	    /* account for the sign */

    for (;;)		    /* to break out of */
    {

	if (format == G_FORMAT)	    /* shorten 'g' fmt to align decimal pnts */
	    digits = width - 4;
	else
	    digits = width;

	if (format != E_FORMAT)	    /* if we would rather not
				    ** use exponential form
				    */
	{
	    /*
	    ** Try and fit the number into non-exponential form. 
	    ** We will use exponent form if G/N format and the value
	    ** will have significant digits after (prec-1) zeros
	    ** in the fraction.
	    */
	    if ((digits && (digits > prec)) &&
	       (format == F_FORMAT || value >= 0.1 || 
		value <= -0.1 || value == 0.0 ))
	    {
		int	awidth;
		int	tmp;

		awidth = digits;
		IICVfcvt(value, buffer.buf, &digits, &exp, prec);

		/* at this point digits is the number of digits in the	    */
		/* buffer.buf string and exp is the number of decimal	    */
		/* places to shift the decimal point (assuming the decimal  */
		/* point is currently at the left extreme of the number in  */
		/* buffer.buf)						    */
		tmp = (exp < 0)  ?  0  : exp;
						
		/* check to see if the number will fit: this is calculated  */
		/* by taking the available width, subtracting the digits to */
		/* the right of the decimal point (prec) and subtracting    */
		/* the digits to the left of the decimal point (tmp).	    */
		/* Remember, room for the decimal point and a negative sign */
		/* have already been taken into account above.		    */
		if (awidth - prec - tmp >= 0)
		{
		    if (format != G_FORMAT) /* will fit: change 'n' to 'f' */
			format = F_FORMAT;
		    break;			/* Write out the number */
		}
	    }
	}
	/* We go through here if we need the exponential form */
	if (format != F_FORMAT)
	{
	    width -= 4;		/* account for e+00 */
	    /* We can use exponential form if we have positive width
	    ** and we have width enough for the precision 
	    */
	    if ((digits = width) > 0 && width > prec)
	    {
		IICVecvt(value, buffer.buf, &digits, &exp);
		format = E_FORMAT;	/* make it 'e' format for
					** the ascii builder
					*/
		break;			/* Write the ascii string */
	    }
	}
	memset(ascii, (int)'*', overflow_width);	/* dumb user: star
							** his field
							*/
	ascii[overflow_width] = EOS;
	*res_width = overflow_width;	
	return(IICV_TRUNCATE);
    }

    /*
    **	At this point:
    **		format is 'e', 'f', 'g'(which means f format with 4 blank fill).
    **		digits is the number of significant digits to be printed
    **		width  is the the width of the field (+4 if this is 'g')
    **		prec   is the number of digits to follow the decimal pnt
    */

    {
	register char	*p = buffer.buf;
	register char	*q = ascii;
	register int	move;

	if (digits > 0  &&  *p++ == '-')
	{
	    *q++ = '-';				/* deposit the sign if '-' */
	}
	/* Don't print a leading blank if we
	** need the space for a leading digit.
	*/
	else if (format == E_FORMAT && width - prec > 1)
	{
	    *q++ = ' ';
	    width--;		/* account for the loss of space.
				** The '-' was accounted for above.
				*/
	}
	for (move = 1;;)			/* always put a leading digit */
	{
	    if (value == 0.0)			/* special case for zero */
		break;
	    if (format != E_FORMAT)
	    {
		if (exp <= 0)
		    break;	    /* if 'f'|'g' and no digits
				    ** before decimal pnt force '0'
				    */
		move = exp;	/* move the digits before the decimal pnt */
	    }
	    else
		move = width - prec, exp -= move;   /* move the number of
						    ** digits that will fit
						    */
	    for (; move > 0 && digits > 0; --move, --digits)
		cmcpyinc(p, q);
	    break;
	}
	while (--move >= 0)	    /* fill with '0' beyond precision
				    ** before decimal point
				    */
	    *q++ = '0';
	if ((move = prec) > 0)	    /* add precision if requested */
	{
	    *q++ = decchar;
	    if (format != E_FORMAT)
		for (; exp < 0 && move > 0; *q++ = '0', ++exp, --move)
		    ;			/* fill 'f' with '0' before
					** precision after decimal point
					*/
	    for (; move > 0 && digits > 0; --move, --digits)		
		cmcpyinc(p, q);
	    while (--move >= 0)			/* pad precision with '0' */
		*q++ = '0';
	}
	if (format == G_FORMAT)
	{
	    *q++ = ' ';
	    *q++ = ' ';
	    *q++ = ' ';
	    *q++ = ' ';
	}
	else if (format == E_FORMAT)
	{
	    *q++ = echar;
	    *q++ = '+';
	    if (exp < 0)
		q[-1] = '-', exp = -exp;
	    move = exp / 10;
	    *q++ = move + '0';
	    *q++ = (exp - move * 10) + '0';
	}
	*q = EOS;
	*res_width = (q - ascii);
    }

    return(OK);
}


/*{
** Name: cvfcvt - convert floating point to ascii ('f' format)
**
** Description:
**    This uses the standard fcvt programs.
**
** Inputs:
**	value				    value to be converted.
**	prec
**
** Output:
**	buffer				    ascii string 
**	digits
**	exp
**
**      Returns:
**          OK                         	    Function completed normally. 
**          FAIL                            Function completed abnormally.
**
** History:
**	01-oct-1985 (derek)
**          Created new.
*/

II_STATUS
IICVfcvt(value, buffer, digits, exp, prec)
f8	value;
char	*buffer;
int	*digits;
int	*exp;
int	prec;
{
	int	sign;
	char	*temp;
	int	length;
	char	*fcvt();
	char	*ecvt();

        if (*digits > MAXPREC)
                *digits = MAXPREC;

	temp = fcvt(value, prec, exp, &sign);

	if ((length = strlen(temp)) > MAXPREC)
	{
		if ((prec - length + MAXPREC) >= 0)
			temp = fcvt(value, prec - length + MAXPREC, exp, &sign);
		if ((length = strlen(temp)) > MAXPREC)
			temp = ecvt(value, MAXPREC, exp, &sign);
	}

	if (sign)
		strcpy(buffer, "-");
	else
		strcpy(buffer, " ");

	strcat(buffer, temp);
	*digits = strlen(temp);
}


/*{
** Name: cvecvt - convert floating point to ascii ('e' format)
**
** Description:
**    This uses the standard fcvt programs.
**
** Inputs:
**	value				    value to be converted.
**
** Output:
**	buffer				    ascii string 
**	digits
**	exp
**
**      Returns:
**          OK                         	    Function completed normally. 
**          FAIL                            Function completed abnormally.
**
** History:
**	01-oct-1985 (derek)
**          Created new.
*/
II_STATUS
IICVecvt(value, buffer, digits, exp)
f8	value;
char	*buffer;
int	*digits;
int	*exp;
{
	int	sign;
	char	*temp;

	char	*ecvt();

	if (*digits > MAXPREC)
		*digits = MAXPREC;

	temp = ecvt(value, *digits, exp, &sign);

	if (sign)
		strcpy(buffer, "-");
	else
		strcpy(buffer, " ");

	strcat(buffer, temp);
	*digits = strlen(temp);
}
