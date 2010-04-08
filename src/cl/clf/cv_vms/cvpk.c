/*
** Copyright (c) 1988, 2002 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <clfloat.h>
#include    <cv.h>
#include    <me.h>
#include    <mh.h>
#include    <cm.h>

/**
**
**  Name: CVPK.C - CV routines for packed decimal.
**
**  Description:
**      This file contains all of the CV routines necessary to convert 
**      packed decimal values into/from other entities.
**
**	This file defines the following routines:
**
**          CVpka() - Convert packed decimal to string.
**          CVapk() - Convert string to packed decimal.
**          CVpkf() - Convert packed decimal to float.
**	    CVfpk() - Convert float to packed decimal.
**
**  History:
**      03-may-88 (thurston)
**          Initial creation.
**	24-jul-89 (jrb)
**	    Re-wrote CVpka for new specification; changed how CVfpk works, and
**	    made all overflow or syntax errors zero the result if it's decimal.
**	01-sep-89 (jrb)
**	    Removed all the CV routines that are now implemented in MACRO.
**	06-mar-90 (jrb)
**	    Fixed some lint complaints (casting and removed an unused variable).
**	11/14/92 (dkh) - Grabbed unix version of CVfpk to elminate more
**			 macro code for the alpha port.  This is potentially
**			 usable for the Vax but it  will probably not be as
**			 efficient.
**      10-feb-1993 (stevet)
**          Fixed problem with CVpkf where incorrect result can return
**          if scale=prec.
**	06-apr-1993 (stevet)
**	    Avoid round problem when CVfpk calling CVfa by raising the
**	    scale to its maximum values.
**	20-jul-1993 (stevet)
**	    My last change turned out to be insufficient to eliminate
**	    rounding problem for float to decimal.  Further adjusted
**	    buffer size, precision and scale to eliminate roundings.
**	02-jul-93 (walt)
**	    Made CV0decerr nolonger static so it could be seen by some of the
**	    routines formerly in this file that have been split off into
**	    their own files.
**      16-jul-93 (ed)
**	    added gl.h
**      11-aug-93 (ed)
**          added missing includes
**	30-dec-99 (kinte01)
**	    clean up compiler warnings for ANSI C compiler
**	19-jul-2000 (kinte01)
**	    Correct prototype definitions
**      22-Nov-2000 (zhahu02) (from main by inkdo01)
**          New algorithm which only does one divide (for bug 102529).
**	01-dec-2000	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from VMS CL as the use is no longer allowed
**/



/*{
** Name: CVpka() - Convert packed decimal to string.
**
** Description:
**      Converts a packed decimal value into a null terminated character
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
**          By default (i.e. not using CV_PKROUND), if there are more
**	    scale digits in the decimal value than fractional digits
**	    requested, the extra digits will be truncated.
**
**          By default (i.e. not using CV_PKLEFTJUST), the output string
**          will be right justified, with blank padding on the left if
**	    necessary.  No blank padding on the right will be done when
**	    using CV_PKLEFTJUST; the null terminator will immediately
**	    follow the last displayed digit.
**
**          By default (i.e. not using CV_PKZEROFILL), leading zeros are
**          converted to blanks.  However, if all digits in integral
**	    portion of number are 0 then a single 0 is output, unless
**	    that would cause the CV_OVERFLOW condition.  Note that this option
**	    has no effect if CV_PKLEFTJUST was requested too.
**
**	Examples:
**	---------
**	    pk (prec,scale) = 10.456 (5,3)
**	    field_wid = 10    frac_digs = 2
**	    options = 0;
**			status will be:  CV_TRUNCATE
**			string will be:  "     10.45"
**
**	    pk (prec,scale) = 10.456 (5,3)
**	    field_wid = 10    frac_digs = 2
**	    options = CV_PKLEFTJUST;
**			status will be:  CV_TRUNCATE
**			string will be:  "10.45"
**
**	    pk (prec,scale) = -10.456 (5,3)
**	    field_wid = 10    frac_digs = 2
**	    options = CV_PKLEFTJUST;
**			status will be:  CV_TRUNCATE
**			string will be:  "-10.45"
**
**	    pk (prec,scale) = 10.456 (5,3)
**	    field_wid = 5     frac_digs = 2
**	    options = CV_PKLEFTJUST;
**			status will be:  CV_TRUNCATE
**			string will be:  "10.45"
**
**	    pk (prec,scale) = -10.456 (5,3)
**	    field_wid = 5     frac_digs = 2
**	    options = CV_PKLEFTJUST;
**			status will be:  CV_OVERFLOW
**			string will be:  <undefined>
**
**	    pk (prec,scale) = 10.456 (5,3)
**	    field_wid = 10    frac_digs = 2
**	    options = CV_PKZEROFILL;
**			status will be:  CV_TRUNCATE
**			string will be:  "0000010.45"
**
**	    pk (prec,scale) = -10.456 (5,3)
**	    field_wid = 10    frac_digs = 2
**	    options = CV_PKZEROFILL;
**			status will be:  CV_TRUNCATE
**			string will be:  "-000010.45"
**
**	    pk (prec,scale) = -10.456 (5,3)
**	    field_wid = 10    frac_digs = 2
**	    options = CV_PKLEFTJUST | CV_PKZEROFILL;
**			status will be:  CV_TRUNCATE
**			string will be:  "-10.45"
**
**	    pk (prec,scale) = .45 (2,2)
**	    field_wid = 10    frac_digs = 2
**	    options = 0;
**			status will be:  OK
**			string will be:  "      0.45"
**
**	    pk (prec,scale) = .45 (2,2)
**	    field_wid = 3     frac_digs = 2
**	    options = 0;
**			status will be:  OK
**			string will be:  ".45"
**
**	    pk (prec,scale) = .456 (3,3)
**	    field_wid = 3     frac_digs = 2
**	    options = CV_PKROUND;
**			status will be:  CV_TRUNCATE
**			string will be:  ".46"
**
**
** Inputs:
**      pk                              Pointer to the decimal value.
**      prec                            Its precision:  1 <= prec <= 39.
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
**					combination of the following
**					may be "OR'ed" together:
**		CV_PKROUND		Do rounding instead of truncation, which
**					is the default.
**		CV_PKLEFTJUST		Left justify the output instead of right
**					justifying, which is the default.
**		CV_PKZEROFILL		Fill up the entire field width by
**					placing however many leading zeros
**					required to do so.  Default is to blank
**					pad.  As an example, the number -10.453
**					placed into an output string where
**					`field_wid' = 8 and `frac_digs' = 2
**					will normally be displayed as
**					"  -10.45".  If the CV_PKZEROFILL option
**					is being used it will be displayed as
**					"-0010.45".
**
** Outputs: 
**      str                             The null terminated string
**                                      representing the decimal value
**					in the format specified.
**
**	res_wid				Number of characters used in str.
**
**	Returns:
**	    OK				Success.
**
**	    CV_TRUNCATE			Some fractional digits were
**					either truncated or rounded in
**					order to use `frac_digs'.
**
**	    CV_OVERFLOW			String was not big enough to
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

/* forward declarations */
STATUS CV0decerr();

STATUS
CVpka(
PTR	    pk,
i4	    prec,
i4	    scale,
char	    decpt,
i4	    field_wid,
i4	    frac_digs,
i4	    options,
char	    *str,
i4	    *res_wid)
{
    char	dpoint = (decpt ? decpt : '.');
    char	*p = pk;
    i4		pr = prec;
    i4		sc = scale;
    i4		nz_pr;
    i4		remain_wid;
    i4		isneg;
    char	*s = str;
    u_char	sign;
    

    /* is decimal number negative? */
    sign  = (p[prec/2] & 0xf);
    isneg = (sign == MH_PK_MINUS  ||  sign == MH_PK_AMINUS);

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
	MEfill((u_i2)field_wid, (u_char)'*', (PTR)str);
	str[field_wid] = EOS;
	*res_wid = field_wid;
	return(CV_OVERFLOW);
    }

    /* at this point remain_wid contains the amount of leading space available
    ** for filling with blanks or zeros depending on what is desired (and
    ** neither if we are left-justifying)
    */
    if ((options & CV_PKLEFTJUST) == 0)
    {
	if (options & CV_PKZEROFILL)
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
	    i4	    want_lead = (pr == sc  &&  frac_digs > 0);

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
    if (options & CV_PKROUND)
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

		if (!CMdigit(ts))
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
	    if (ts < str  ||  !CMdigit(ts))
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
		    MEfill((u_i2)field_wid, (u_char)'*', (PTR)str);
		    str[field_wid] = EOS;
		    *res_wid = field_wid;
		    return(CV_OVERFLOW);
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
	    return(CV_TRUNCATE);
    }

    return(OK);
}


/*{
** Name: CVapk() - Convert string to packed decimal.
**
** Description:
**      Converts a null terminated character string into a packed decimal value
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
**	The status returned will always be the most severe status encountered;
**	all statuses are listed below in order of increasing severity.
**
** Inputs:
**      string                          The null terminated string representing
**                                      the decimal value.
**      decpt                           Decimal character in use.  If this
**					is '\0' then it is set to '.'
**      prec                            Precision for result:  1 <= prec <= 39.
**      scale                           Scale for result:  0 <= scale <= prec.
**
** Outputs:
**      pk                              The resulting packed decimal value is
**					placed here.
**
**	Returns:
**	    OK				Success.
**
**	    CV_TRUNCATE			Some fractional digits had to be
**					truncated in order to use scale.
**
**	    CV_OVERFLOW			Integral part has too many digits
**					(not including leading zeros)
**					i.e. more than (prec - scale) of them.
**
**	    CV_SYNTAX			Could not convert due to syntax error.
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
**	    Now calls CV0decerr before returning so that result will be zeroed
**	    if serious error was encountered.
*/

STATUS
CVapk(
char	    *string,
char	    decpt,
i4	    prec,
i4	    scale,
PTR	    pk)
{
    char	*x = string;		/* for traipsing thru string */
    char	*y;			/* ditto */
    char	dpoint = (decpt ? decpt : '.');
    u_char	*pkdec = (u_char *)pk;	/* pointer to output variable */
    i4		nlz;			/* number of leading zeros */
    i4		i = prec;		/* for counting digits */
    i4		neg = FALSE;		/* true if number is negative */
    i4		founddig = FALSE;	/* true if digits have been found */
    STATUS	ret_stat = OK;		/* most severe return status so far */

    /* zero out first byte in case prec is even */
    *pkdec = (u_char)0;

    /* strip leading whitespace */
    while (CMwhite(x))
	CMnext(x);

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
	*pkdec = MH_PK_PLUS;
	
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
	    if (!CMdigit(x) && *x != dpoint)
		return(CV0decerr(CV_SYNTAX, pk, prec));
    }
    
    /* strip leading zeros */
    if (*x == '0')
    {
	founddig = TRUE;
	while (*x == '0')
	    x++;
    }

    /* mark off integral part using x and y */
    for(y = x; CMdigit(y); y++)
	;
    founddig = founddig || (x < y);

    /* calculate number of leading zeros and do error-check */
    if ((nlz = (prec - scale) - (y - x)) < 0)
    {
	ret_stat = CV_OVERFLOW;
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
	if (*x != '\0'  &&  !CMwhite(x))
	    return(CV0decerr(CV_SYNTAX, pk, prec));

    /* a packed decimal number must have at least one digit */
    if (!founddig  &&  !CMdigit(x))
	return(CV0decerr(CV_SYNTAX, pk, prec));

    /* copy fractional part of string into pkdec */
    while (CMdigit(x)  &&  i > 0)
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
	*pkdec |= MH_PK_MINUS;
    else
	*pkdec |= MH_PK_PLUS;

    /* if there are still digits left, ignore them */
    if (CMdigit(x))
    {
	if (ret_stat == OK)
	    ret_stat = CV_TRUNCATE;
	while (CMdigit(x))
	    x++;
    }

    while (CMwhite(x))
	CMnext(x);
    
    if (*x != '\0')
	ret_stat = CV_SYNTAX;

    return(CV0decerr(ret_stat, pk, prec));
}


/*{
** Name: CVpkf() - Convert packed decimal to float.
**
** Description:
**      Converts a packed decimal value into an eight byte float.
**
** Inputs:
**      pk                              Pointer to the decimal value.
**      prec                            Its precision:  1 <= prec <= 39.
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
**      10-feb-1993 (stevet)
**          Fixed problem with CVpkf where incorrect result can return
**          if scale=prec.
**      22-Nov-2000 (zhahu02) (from main by inkdo01)
**          New algorithm which only does one divide (for bug 102529).
*/

STATUS
CVpkf(pk, prec, scale, fnum)
PTR	    pk;
i4	    prec;
i4	    scale;
f8	    *fnum;
{
    f8		val = 0.0;		/* accumulator for fnum */
    u_char	*p = (u_char *)pk;	/* for walking thru pk */
    i4		pr = prec;
    u_char	sign;			/* sign code found in pk */
    i4          temp;
    i4          int_accumulator;
    i4          digits;

    /* read the sign code from pk */
    sign = p[prec / 2] & 0xf;

    /* skip past leading pairs of zeros for the sake of efficiency */
    if ((pr & 1) == 0  &&  (*p & 0xf) == (u_char)0 && pr > scale)
    {
	p++;
	pr--;
    }
    while (*p == (u_char)0  &&  pr > scale + 1)
    {
	p++;
	pr -= 2;	/* skip down to next odd number */
    }

    /* Accumulate integral value, 9 digits at a time and
    ** roll into f8 accumulator. */
    while (pr > 0)
    {
        int_accumulator = 0;
        digits = 0;
        while (pr > 0 && digits < 9)
        {
            temp = (pr-- & 1  ?  *p >> 4 : *p++) & 0xf;
            int_accumulator = 10 * int_accumulator + temp;
            digits++;
        }

        if (digits == 9)
            val = 1e9*val + (f8)int_accumulator;        /* roll into float total
 */
        else
            val = MHipow(10.0, (i4)digits)*val + (f8)int_accumulator;
    }

    /* Divide once by appropriate power of 10. */
    if (scale > 0) val /= MHipow(10.0, (i4)scale);

    /* set the sign to minus if appropriate */
    if (sign == MH_PK_MINUS  ||  sign == MH_PK_AMINUS)
        val = -val;

    *fnum = val;
    return(OK);
}




/*{
 ** Name: CVfpk() - Convert float to packed decimal.
 **
 ** Description:
 **      Converts an eight byte float into a packed decimal value of the
 **      requested precision and scale.
 **
 ** Inputs:
 **      fnum                            The eight byte float.
 **      prec                            Precision for result:  1 <= prec <= 39.
 **      scale                           Scale for result:  0 <= scale <= prec.
 **
 ** Outputs:
 **      pk                              The resulting packed decimal value is
 **                                      placed here.
 **
 **      Returns:
 **          OK                          Success.
 **          CV_OVERFLOW                 Too many non-fractional digits;
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
**	06-apr-1993 (stevet)
**	    Avoid round problem when CVfpk calling CVfa by raising the
**	    scale to its maximum values.
**	14-feb-2007 (dougi)
**	    Add support for increased decimal precision.
 */
 
 STATUS
 CVfpk(fnum, prec, scale, pk)
 f8          fnum;
 i4         prec;
 i4         scale;
 PTR         pk;
{
    /*
    **	Maximum buff = CL_MAX_DECPREC + DBL_DIG + 3
    **
    ** We need buffer to be big enough so that we can avoid rounding,
    ** DECIMAL does not round but truncate.  Have to hard code
    ** DB_MAX_DECPREC since it is in common.h (yuk).
    */
    u_char	buff[DBL_DIG + CL_MAX_DECPREC + 3];
    i2		res_wid;
    i4		digs;

    if (CVfa(fnum, DBL_DIG + CL_MAX_DECPREC + 2, DBL_DIG, 'f', '.', 
				(char *)buff, &res_wid)
	    != OK)
        return(CV_OVERFLOW);

    return(CV0decerr(CVapk((char *)buff, '.', prec, scale, pk), pk, prec));
}


/*{
** Name: CV0decerr() - Check return status and zero out result if error
**
** Description:
**	This routine checks the return status and if it is either CV_OVERFLOW
**	or CV_SYNTAX then it zeros out the result decimal value.  It always
**	returns the same status that was given to it.
**	
** Inputs:
**	status			Return status of calling routine
**	pkbuff			Output decimal buffer to be zeroed
**	prec			Precision of the output decimal buffer
**	
** Outputs:
**      pkbuff			Is filled with 0's
**
**      Returns:
**	    status		whatever was sent to it, it sends back
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
**	02-jul-93 (walt)
**	    Made CV0decerr nolonger static so it could be seen by some of the
**	    routines formerly in this file that have been split off into
**	    their own files.
*/
STATUS
CV0decerr(status, pkbuff, prec)
STATUS	status;
PTR	pkbuff;
i4	prec;
{
    if (status == CV_OVERFLOW  ||  status == CV_SYNTAX)
    {
	/* zero out all but the last byte; then set the last byte with a + sign */
	MEfill((u_i2)(prec / 2), (u_char)0, pkbuff);
	*((u_char *)pkbuff + prec / 2) = MH_PK_PLUS;
    }
    
    return(status);
}
