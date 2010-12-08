/*
** Copyright (c) 1985, 2008 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <me.h>
#include    <cv.h>
#include    <cm.h>
#include    <ex.h>
#include    <float.h>
#include    <st.h>
#include    <ots$routines.h>

/**
**
**  Name: CV.C - CV compatibility library routines.
**
**  Description:
**      This module implements the conversion routines for the compatibility
**	library.
**
**          CVaf - Convert ascii to floating.
**          CVan - Convert ascii to i4.
**          CVal - Convert ascii to long.
**          CVal8 - Convert ascii to i8.
**          CVahxl - Convert ascii hex to long.
**          CVuahxl - Convert ascii hex to unsigned long.
**          CVna - Convert i4 to ascii.
**          CVla - Convert long to ascii.
**          CVula - Convert unsigned long to ascii.
**	    CVlx - Convert long to ascii hex.
**          CVlower - Lowercase a character string.
**          CVol - Convert octal to long.
**          CVupper - Uppercase a character string.
**
**  History:
**      01-oct-1985 (derek)    
**          Created new for Jupiter.
**	03-jun-86 (eda)
**	    Removed statement that was setting v=1.0 if there was no
**	    mantissa.  The case where no mantissa is found in the string,
**	    it can be assumed to be zero.  (bug #9145)
**	18-aug-96 (jeff)
**	    Added decimal character parameter to CVaf() and CVfa().
**	08-sep-86 (thurston)
**	    Re-did Jeff's decimal parameter integration to be more efficient.
**	31-oct-88 (thurston, jrb)
**	    Modified for Kanji support and did various cleaning up.
**	20-mar-89 (jrb)
**	    Fixed bug in CVfa where overflow was being checked incorrectly.
**      01-apr-92 (stevet)
**          B42692: Fixed CVfa() rounding problem when using E_FORMAT.
**	11/14/92 (dkh) - Updated to run on alpha.  Specifically, CVfa()
**			 move to its own source file.  Also replaced
**			 CVepxten() with CVexp10() from UNIX.  This
**			 eliminates macro code that was based on D floating
**			 point numbers while alpha will now use IEEE floats.
**      16-jul-93 (ed)
**	    added gl.h
**      11-aug-93 (ed)
**          added missing includes
**	30-mar-95 (albany)
**	    Integrate changes from VAX CL:
**		14-dec-94 (stephenb)
**	    	    Fix CVal to return CV_OVERFLOW (bug# 60384)
**	20-apr-95(albany)
**	    Changed i4 *result to u_longnat *result on CVahxl() to
**	    agree with prototype in JPT_GL_HDR:cv.h.
**	09-feb-98 (kinte01)
**	    Added new routine CVuahxl for change 432424
**      11-mar-98 (kinte01)
**          Changed readonly to be READONLY so compat.h defn is used. Did this
**          as I'm addressing what would need to be changed to compile with the
**          complete ascii compiler instead of in VAXC mode.
**	19-jul-2000 (kinte01)
**	    Correct prototype definitions by adding missing includes
**	01-dec-2000	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from VMS CL as the use is no longer allowed
**      30-jul-2002 (horda03) Bug 106435
**          Added CVula(). CVla plus modifications for unsigned data.
**      11-Sep-2002 (horda03)
**          Unknown exceptions are being raised when dealing with very small
**          floating point values (e.g. 2.345e-308). Also, the range of values
**          that can be represented by Ingres is smaller than the IEEE range.
**          There is a problem converting a 1.0e308 + value, so need to limit
**          the upper bound as 9.9999e+307.
**          Bug 78947.
**	22-jan-2003 (abbjo03)
**	    Add CVla8 for 8-byte conversions (originally added to UNIX CL on
**	    17-jan-1996, a mere seven years ago--better late than never).
**	05-sep-2003 (abbjo03)
**	    CVla8's first parameter ought to be a longlong.
**      10-jun-2004 (loera01)
**          Added CVal8().
**      18-dec-2008 (joea)
**          Replace READONLY/WSCREADONLY by const.
**      14-Aug-2009 (horda03) B122474
**          Cause of the problem converting 1.0e308 + values. So remove this 
**          restriction.
**	9-Feb-2010 (kschendel) SIR 123275
**	    Update CVal, CVal8 with latest unix/win speed tweaks.
**      19-feb-2010 (joea)
**          Change #ifdef ALPHA to support both VMS variants.
**      06-Dec-2010 (horda03) SIR 124685
**          Fix VMS build problems, 
*/

static	i4	cverrnum = 0;

i4
CVexhandler(exargs)
EX_ARGS	*exargs;
{
	/*
	**  This handler is only designed to handle under/overflows.
	**  Anything else causes a resignal.
	*/
	switch(exargs->exarg_num)
	{
	  case EXFLTOVF:
	  case EXINTOVF:
		cverrnum = CV_OVERFLOW;
		return(EXDECLARE);

	  case EXFLTUND:
		cverrnum = CV_UNDERFLOW;
		return(EXDECLARE);

	  default:
	  {
	     char buf [300];
	     CL_SYS_ERR err;
	     i4 i;

	     STprintf(buf, "CVexhandler() - Exarg_num = %d\n", exargs->exarg_num);
	     ERlog(buf, STlength(buf), &err);

	     if ( !EXsys_report( exargs, buf ))
	     {
	        STprintf( buf, "Exception is %x (%d.)",
                          exargs->exarg_num, exargs->exarg_num );
	        ERlog(buf, STlength(buf), &err);

	        for ( i = 0; i < exargs->exarg_count; i++ )
	        {
	           STprintf( buf + STlength( buf ), ",a[%d]=%x",
	                     i, exargs->exarg_array [i] );
	           ERlog(buf, STlength(buf), &err);
	        }
	     }

	     return(EXRESIGNAL);
           }
	}
}



/*{
** Name: CVaf	- Convert ascii to floating.
**
** Description:
**	Convert a string containing a floating point number to floating
**	point.  The syntax of the number is as follows:
**(	    {<sp>} [+|-] {<sp>} {<digit>} [.{digit}] {<sp>} [<exp>]
**)
** Inputs:
**      string                          String containing the number.
**	decchar				Decimal character to use
**
** Outputs:
**      result                          Where to store the result.
**
**	Returns:
**	    OK
**	    CV_UNDERFLOW
**	    CV_OVERFLOW
**	    CV_SYNTAX
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	01-oct-1985 (derek)
**          Created new for Jupiter.
**	08-sep-86 (thurston)
**	    Re-did Jeff's decimal parameter integration to be more efficient.
**	31-oct-88 (thurston, jrb)
**	    Converted for Kanji and cleaned up.
*/

# define	AFTER_DP	1
# define	NEGATIVE_NUMBER	2
# define	GOT_MANTISSA	4
# define	MAXEXPONENT	38
# define	MINEXPONENT	-38

/* Range of float (excluding sign)
**
**     1.1754E-38 < = float < = 3.4028E+38
*/
#define MINEXP_MANTISSA 1.1754
#define MAXEXP_MANTISSA 3.4028

# ifdef	IEEE_FLOAT
# define	SIGNIF_DIGITS	15
# else
# define	SIGNIF_DIGITS	16
# endif

#if defined(axm_vms) || defined(i64_vms)
# ifdef __IEEE_FLOAT
# undef MAXEXPONENT
# undef MINEXPONENT
# undef SIGNIF_DIGITS


/* Range of the IEEE float8 is (excluding sign)
**
**     2.225074E-308 < = float < = 1.797693e+308
**/
# define      MAXEXPONENT     308
# define        MINEXPONENT     -308

#define MINEXP_MANTISSA 2.225074
#define MAXEXP_MANTISSA 1.797693

# define	SIGNIF_DIGITS	__T_FLT_DIG
# endif
# endif

/*
**  Forward and/or External function references.
*/

VOID
CVexp10(exp, val)
i4    exp;
f8    *val;
{
      f8      ten = 10.0;
      f8      exponent = 1.0;
      i4      large_exp = 0;

      if (exp > 0)
      {
       do
       {
          if (exp > MAXEXPONENT)
          {
             large_exp = exp - MAXEXPONENT;

             exp = MAXEXPONENT;
          }

          do
          {
               exponent *= ten;
          } while (--exp > 0);
          *val *= exponent;

          if (large_exp)
          {
             exp = large_exp;

             exponent = 1.0;
          }
       }
       while (exp);
       
      }
      else
      {
         if ( exp < -MAXEXPONENT)
         {
            /* We're need to store a very small number, but due to the
            ** way that the value has been calculated, the exponent
            ** required to multiply the value is greater than the
            ** maximum value that can be stored by an F8. But we also 
            ** know that the final value is within range.
            */
            while ( exp++ < -MAXEXPONENT)
            {
               exponent *= ten;
            }

            *val /= exponent;

            exponent = ten;
         } 

       while (exp++ < 0)
       {
               exponent *= ten;
       }
       *val /= exponent;
      }
}


STATUS
CVaf(
char	*str,
char	decimal,
f8	*val)
{
    register char   *p = str;
    register int    cnt = 0;
    register int    status = 0;
    char	    decchar = (decimal ? decimal : '.');
    int		    frac_digits = 0;
    int		    exponent = 0;
    int		    digits = 0;
    i4		    zero_cnt;	    /* number of consecutive zeros found */
    i4		    exp_adj = 0;    /* number to increase exponent */
    double	    v = 0.0;
    int		    problem;
    i4              pow, n;
    EX_CONTEXT	    context;
    int		    normalizer = 0; /* exponent multiplier
				    ** to normalize data
				    */

    /* Scan past blanks collecting the sign and
    ** stop after first non-blank after sign.
    */

    while (CMspace(p))
	CMnext(p);
    if (*p == '-' || *p == '+')
    {
	if (*p == '-')
	    status |= NEGATIVE_NUMBER;
	CMnext(p);
	while (CMspace(p))
	    CMnext(p);
    }

    while(*p == '0')
	CMnext(p);

    /* Collect the number:  Remember number of digits
    ** after decimal point and suppress trailing zero's.
    */

    for (cnt = 0;; CMnext(p), status |= GOT_MANTISSA)
    {
	if (CMdigit(p))
	{
	    if (*p != '0' && (digits + frac_digits) < SIGNIF_DIGITS)
	    {
		/* If this number is less than one, remember the
		** number of leading zeros after the exponent.
		** These will not be multiplied here. They will
		** be subtracted from the exponent.
		*/
		if (cnt  &&  v == 0.0  &&  (status & AFTER_DP))
		{
		    normalizer = -cnt;
		    zero_cnt = 0;
		    cnt = 0;
		}
		else
		{
		    for (zero_cnt = cnt; cnt > 0; v *= 10.0, cnt--)
			;
		}
	    }
	    else
	    {
		if (!(status & AFTER_DP))
		    exp_adj++;
		cnt++;
		continue;
	    }
	    if (!(status & AFTER_DP))
	    {
		exp_adj = 0;
		digits += zero_cnt + 1;
	    }
	    else
	    {
		/* Subtract from frac those
		** zeros in the whole number.
		*/
		frac_digits += zero_cnt + 1 - exp_adj;
		exp_adj = 0;
	    }
	    v = v * 10.0 + (*p - '0');
	}
	else if (*p == decchar)
	{
	    if (status & AFTER_DP)
	    {
		break;
	    }
	    /* Add in the trailing whole number zeros.
	    ** Subtracted out if no number > 0 in the
	    ** fraction.
	    */
	    digits += cnt;
	    status |= AFTER_DP;
	}
	else
	{
	    if ((status & AFTER_DP) == 0)
	    {
		/* Add in the trailing whole number zeros.
		** Subtracted out if no number > 0 in the
		** fraction.
		*/
		digits += cnt;
	    }
	    break;
	}
    }

    /* Skip blanks before possible exponent */
    while (CMspace(p))
	CMnext(p);

    /* Test for exponent */

    if (*p == 'e' || *p == 'E')
    {
	CMnext(p);
	if (CVal(p, &exponent))
	    return (CV_SYNTAX);
/*	if ((status & GOT_MANTISSA) == 0)
**	    v = 1.0;
*/
    }
    else if (*p != EOS)
	return (CV_SYNTAX);
    exponent += exp_adj;
    digits -= exp_adj;
    normalizer += digits - 1;	/* Size of implied exponent.
				** 10e20 is really 1e21.
				*/

    /* If the normalizer is less than 0, then digits is zero.  If the 
    ** number were greater than 1, digits whould be greater than 1 and
    ** the normalizer would be greater than 1.
    */
    if (normalizer < 0)
    {
	exponent += normalizer;
	frac_digits -= 1;	/* We have added to the exponent.
				** Don't subtract as much later.
				*/
	normalizer = 0;
    }
    /* Exponent + number of decimals to normalize number */
    if ( (exponent + normalizer) == MAXEXPONENT)
    {
       for (pow = 1, n = frac_digits; n-- > 0; pow *= 10);

       if ( (v / pow) > MAXEXP_MANTISSA) return (CV_OVERFLOW);
    }
    else if ( (exponent + normalizer) > MAXEXPONENT)
        return (CV_OVERFLOW);

    /* If x > 1e-38 then its OK.  After normalization the mantissa is always
    ** greater than 1.  Therefore, we only need to check the exponent.
    */
    if ((exponent + normalizer) == MINEXPONENT)
    {
       for (pow = 1, n = frac_digits; n-- > 0; pow *= 10);

       if ( (v / pow) < MINEXP_MANTISSA) return (CV_UNDERFLOW);
    }
    else if ( (exponent + normalizer) < MINEXPONENT)
        return (CV_UNDERFLOW);

    /* Compensate for fractional digits */
    if (exponent - frac_digits)
    {
	if (EXdeclare(CVexhandler, &context) != OK)
	{
                EXdelete(); /* remove the exception handler ! */

		/*
		**  Return indication that under/over-flow occurred.
		*/
		return(cverrnum);
	}

	CVexp10((i4) (exponent - frac_digits), &v);

	EXdelete();
    }

    /* store the result and exit */
    if (status & NEGATIVE_NUMBER)
	    v = -v;
    *val = v;
    return (OK);
}

/*{
** Name: CVan	- Convert ascii to i4.
**
** Description:
**(		<space>* [+-] <space>* <digit>* <space>*
**)
**	`string' is a pointer to the character string, `num' is a
**	pointer to the natural integer which is to contain the result.
**
** Inputs:
**      string                          String holding ascii representation
**					of a natural integer.
**
** Outputs:
**      num                             The natural integer represented in the
**					input string will be placed here.
**
**	Returns:
**	    OK
**	    CV_SYNTAX
**	    CV_OVERFLOW
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	12-jun-87 (thurston)
**          Created new for Jupiter.
*/

STATUS
CVan(string, num)
register char	*string;
i4		*num;
{
    return CVal(string, num);
}

/*{
** Name:	CVal - CHARACTER STRING TO 32-BIT INTEGER
**		       CONVERSION
**
** Description:
**    ASCII CHARACTER STRING TO 32-BIT INTEGER CONVERSION
**
**	  Convert a character string pointed to by `str' to an
**	i4 pointed to by `result'. A valid string is of the form:
**
**		{<sp>} [+|-] {<sp>} {<digit>} {<sp>}
**
**      NOTE that historicly, Ingres accepts an empty string or
**           isolated sign as zero
**
** Inputs:
**	str	char pointer, contains string to be converted.
**
** Outputs:
**	result	i4 pointer, will contain the converted i4 value
**		Only if status is OK will result be updated.
**
**   Returns:
**	OK:		succesful conversion; `result' contains the
**			integer
**	CV_OVERFLOW:	numeric overflow;
**	CV_UNDERFLOW:	numeric underflow;
**	CV_SYNTAX:	syntax error;
**	
** History:
**	04-Jan-2008 (kiria01) b119694
**	    Copy of UNIX/Windows version
**	9-Feb-2010 (kschendel) SIR 123275
**	    Revise to use CVal8 and down-cast the result.  CVal8 is
**	    now essentially as fast as CVal was anyway, since it runs
**	    in i4 mode for the first 9 digits.
**	13-Jan-2010 (wanfr01) Bug 123139
**	    Optimizations for single byte
*/
STATUS
CVal_DB(char *str, i4 *result)
{
    i8 val8;
    STATUS sts;

    sts = CVal8(str, &val8);
    if (sts != OK)
	return (sts);

    if (val8 > MAXI4 || val8 < MINI4LL)
    {
	/* underflow / overflow */
	return (val8 < 0) ? CV_UNDERFLOW : CV_OVERFLOW;
    }

    *result = (i4) val8;
    return OK;
}


STATUS
CVal_SB(char *str, i4 *result)
{
    register char *p = str;	/* Pointer to current char */
    i4 negative = FALSE;	/* Flag to indicate the sign */
    u_i4 val = 0;		/* Holds the integer being formed */

    /* Skip leading blanks */
    while (CMspace_SB(p))
        CMnext_SB(p);

    /* Check for sign */
    switch (*p)
    {
    case '-':
	negative = TRUE;
	/*FALLTHROUGH*/
    case '+':
	CMnext_SB(p);
        /* Skip any whitespace after sign */
	while (CMspace_SB(p))
	    CMnext_SB(p);
    }

    /* At this point everything had better be numeric ... */
    while (CMdigit_SB(p))
    {
	i4 digit = *p++ - '0';

	/* check for overflow - note that in 2s complement the
	** magnitude of -MAX will be one greater than +MAX
	*/
	if (val > MAXI4/10 ||
		val == MAXI4/10 && digit > (MAXI4%10)+negative)
	{
	    /* underflow / overflow */
	    return negative ? CV_UNDERFLOW : CV_OVERFLOW;
	}
	val = val * 10 + digit;
    }

    /* ... or trailing spaces */
    while (CMspace_SB(p))
        CMnext_SB(p);

    if (*p)
	return CV_SYNTAX;

    if (negative)
        *result = -(i4)val;
    else
        *result = (i4)val;

    return OK;
}

/*{
** Name:	CVal8 - CHARACTER STRING TO 64-BIT INTEGER
**		       CONVERSION
**
** Description:
**    ASCII CHARACTER STRING TO 64-BIT INTEGER CONVERSION
**
**	  Convert a character string pointed to by `str' to an
**	i8 pointed to by `result'. A valid string is of the form:
**
**		{<sp>} [+|-] {<sp>} {<digit>} {<sp>}
**
**      NOTE that historicly, Ingres accepts an empty string or
**           isolated sign as zero
**
**	CVal8 is the most important char -> int converter for runtime
**	performance, as all the ADF char -> int coercions use CVal8
**	regardless of result size.
**
** Inputs:
**	str	char pointer, contains string to be converted.
**
** Outputs:
**	result	i8 pointer, will contain the converted i8 value
**		Only if status is OK will result be updated.
**
**   Returns:
**	OK:		succesful conversion; `result' contains the
**			integer
**	CV_OVERFLOW:	numeric overflow;
**	CV_UNDERFLOW:	numeric underflow;
**	CV_SYNTAX:	syntax error;
**	
** History:
**	04-Jan-2008 (kiria01) b119694
**	    Copy of UNIX/Windows version
**	9-Feb-2010 (kschendel) SIR 123275
**	    Performance tweaking:
**	    - eliminate use of CMdigit in the conversion loops;  we're
**	      assuming ascii digits anyway, so CMdigit is pointless.
**	    - Run the first 9 digits in i4 mode without overflow checking.
**	      Only switch to i8 if the number is larger than 9 digits.
**	13-Jan-2010 (wanfr01) Bug 123139
**	    Optimizations for single byte
*/
STATUS
CVal8_DB(char *str, i8 *result)
{
    register u_char *p = (u_char *) str; /* Pointer to current char */
    i4 negative = FALSE;	/* Flag to indicate the sign */
    i4 i;
    register u_i4 digit;
    register u_i4 val4 = 0;
    u_i8 val8;			/* Holds the integer being formed */

    /* Skip leading blanks */
    while (CMspace(p))
        CMnext(p);

    /* Check for sign */
    switch (*p)
    {
    case '-':
	negative = TRUE;
	/*FALLTHROUGH*/
    case '+':
	CMnext(p);
        /* Skip any whitespace after sign */
	while (CMspace(p))
	    CMnext(p);
    }

    /* Run the digit loop for either 9 digits, or a non-digit, whichever
    ** happens first.  9 digits will fit in an i4 without overflow.
    */
    i = 9;
    for (;;)
    {
	digit = *p;
	if (digit < '0' || digit > '9')
	    break;
	val4 = val4 * 10 + digit - '0';
	++p;
	if (--i == 0) break;
    }
    val8 = (u_i8) val4;
    if (CMdigit(p))
    {
	/* broke out of i4 loop, finish up with i8 and overflow testing. */
	do
	{
	    digit = *p++ - '0';

	    /* check for overflow - note that in 2s complement the
	    ** magnitude of -MAX will be one greater than +MAX
	    */
	    if (val8 > MAXI8/10 ||
		    val8 == MAXI8/10 && digit > (MAXI8%10)+negative)
	    {
		/* underflow / overflow */
		return negative ? CV_UNDERFLOW : CV_OVERFLOW;
	    }
	    val8 = val8 * 10 + digit;
	} while (*p >= '0' && *p <= '9');
    }

    /* ... or trailing spaces */
    while (CMspace(p))
        CMnext(p);

    if (*p)
	return CV_SYNTAX;

    if (negative)
        *result = -(i8)val8;
    else
        *result = (i8)val8;

    return OK;
}


STATUS
CVal8_SB(char *str, i8 *result)
{
    register char *p = str;	/* Pointer to current char */
    i4 negative = FALSE;	/* Flag to indicate the sign */
    u_i8 val = 0;		/* Holds the integer being formed */

    /* Skip leading blanks */
    while (CMspace_SB(p))
        CMnext_SB(p);

    /* Check for sign */
    switch (*p)
    {
    case '-':
	negative = TRUE;
	/*FALLTHROUGH*/
    case '+':
	CMnext_SB(p);
        /* Skip any whitespace after sign */
	while (CMspace_SB(p))
	    CMnext_SB(p);
    }

    /* At this point everything had better be numeric ... */
    while (CMdigit_SB(p))
    {
	i4 digit = *p++ - '0';

	/* check for overflow - note that in 2s complement the
	** magnitude of -MAX will be one greater than +MAX
	*/
	if (val > MAXI8/10 ||
		val == MAXI8/10 && digit > (MAXI8%10)+negative)
	{
	    /* underflow / overflow */
	    return negative ? CV_UNDERFLOW : CV_OVERFLOW;
	}
	val = val * 10 + digit;
    }

    /* ... or trailing spaces */
    while (CMspace_SB(p))
        CMnext_SB(p);

    if (*p)
	return CV_SYNTAX;

    if (negative)
        *result = -(i8)val;
    else
        *result = (i8)val;

    return OK;
}

/*{
** Name: CVahxl	- Convert hex ascii to long.
**
** Description:
**
** Inputs:
**
** Outputs:
**	Returns:
**	    OK
**	    CV_SYNTAX
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	01-oct-1985 (derek)
**          Created new for Jupiter.
*/
STATUS
CVahxl(str, result)
register char	*str;
u_i4	*result;
{
    int		    j;
    char	    *p;
    struct
    {
	int	str_len;
	char	*str_str;
    }   numstr;

    numstr.str_str = str;
    for (j = 0, p = str; *p++ != EOS; j++)
	;
    numstr.str_len = j;
    if ((ots$cvt_tz_l(&numstr, result, 4, 1) & 1) == 0)
	return (CV_SYNTAX);
    return (OK);
}
/*{
** Name:        CVuahxl - HEX CHARACTER STRING TO UNSIGNED 32-BIT
**                        INTEGER CONVERSION
**
** Description:
**        Convert a hex character string pointed to by `str' to
**      a u_i4 pointed to by `result'. A valid string is of the
**      form:
**
**              {<sp>} {<digit>|<a-f>} {<sp>}
**
** Inputs:
**      str     char pointer, contains string to be converted.
**
** Outputs:
**      result  u_i4 pointer, will contain the converted unsigned long value
**
**   Returns:
**      OK:             succesful conversion; `i' contains the
**                      integer
**      CV_OVERFLOW:    numeric overflow; `i' is unchanged
**      CV_SYNTAX:      syntax error; `i' is unchanged
**
** History:
**      16-mar-90 (fredp)
**              Written.
*/
STATUS
CVuahxl(str, result)
register char   *str;
u_i4            *result;
{
        register u_i4   overflow_mask = (u_i4)0xf0000000;
        register u_i4   num;
        char            tmp_buf[3];
        char            a;


        num = 0;

        /* skip leading blanks */
        for (; *str == ' '; str++)
                ;

        /* at this point everything should be a-f or 0-9 */
        while (*str)
        {
                CMtolower(str, tmp_buf);

                if (CMdigit(tmp_buf))
                {
                        /* check for overflow */

                        if ( (num & overflow_mask) != 0 )
                        {
                                return(CV_OVERFLOW);
                        }
                        num = num * 16 + (*str - '0');
                }
                else if (tmp_buf[0] >= 'a' && tmp_buf[0] <= 'f')
                {
                        /* check for overflow */

                        if ( (num & overflow_mask) != 0 )
                        {
                                return(CV_OVERFLOW);
                        }
                        num = num * 16 + (tmp_buf[0] - 'a' + 10);
                }
                else if (tmp_buf[0] == ' ')     /* there'd better be only */
                {
                        break;          /* blanks left in this case */
                }
                else
                {
                        return (CV_SYNTAX);
                }
                str++;
        }

        /* Done with number, better be EOS or all blanks from here */
        while (a = *str++)
                if (a != ' ')                   /* syntax error */
                        return (CV_SYNTAX);

        *result = num;

        return (OK);
}

/*{
** Name: CVna	- Convert i4 to ascii.
**
** Description:
**	Converts the natural integer num to ascii and stores the
**	result in string, which is assumed to be large enough.
**
** Inputs:
**      num                             Natural integer to be converted to an
**					ascii string.
**
** Outputs:
**      string                          Ascii string representing the integer.
**
**	Returns:
**	    VOID
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	12-jun-87 (thurston)
**          Created new for Jupiter.
*/

VOID
CVna(num, string)
register i4	num;
register char	*string;
{
	CVla(num, string);
}

/*{
** Name: CVla	- Convert long to ascii.
**
** Description:
**
** Inputs:
**
** Outputs:
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	01-oct-1985 (derek)
**          Created new for Jupiter.
**	31-oct-88 (jrb)
**	    Kanji mods and clean up.
*/
VOID
CVla(i, a)
register i4	i;
register char	*a;
{
	register char	*j;
	static const char	min_integer[] = " -2147483648";
	char		b[12];

    if (i == MINI4)
    {
	j = min_integer;
    }
    else
    {	
	if (i < 0)
	{
		*a++ = '-';
		i = -i;
	}
	j = &b[11];
	*j-- = EOS;
	do
	{
		*j-- = i % 10 + '0';
		i /= 10;
	} while (i != 0);
    }
    do
    {
	*a++ = *++j;
    } while (*j != EOS);
}

/*
** On axp_osf the following are defined in <machine/machlimits.h>
** LONG_MAX      9223372036854775807  max value for a long
** ULONG_MAX    18446744073709551615U  max value for an unsigned long
*/
void
CVla8(
i8	i,
char	*a)
{
    char	*j;
    char	b[23];

    if (i < 0)
    {
	*a++ = '-';
	i = -i;
    }
    j = &b[22];
    *j-- = 0;

    /* Need to accomadate the most negative number (-2147483648).  Since
    ** -(-2147483648) = -2147483648 in 2's complement, i is still negative
    ** and i % 10 is negative also.  Therefore use -(i % 10) for this case,
    ** and divide by -10 to get absolute value of i.
    **
    ** Note that more negative numbers wrap around to positive numbers when
    ** negated on the VAX as expected, but are mapped to (-2147483648) on
    ** the SUN.  1/23/84 (tpc)
    */

    if (i < 0)      /* catch -2147483648 */
    {
	*j-- = -(i % 10) + '0' ;
	i /= -10;
    }

    do
    {
	*j-- = i % 10 + '0' ;
	i /= 10;
    } while (i);

    do
    {
	*a++ = *++j;
    } while (*j);
}

/*{
** Name: CVula	- Convert unsigned long to ascii.
**
** Description:
**
** Inputs:
**
**      i	number to convert.
**
** Outputs:
**      a	ascii representation
**
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      30-jul-2002
**          Created.
*/
VOID
CVula(i, a)
register u_i4	i;
register char	*a;
{
	register char	*j;
	char		b[12];

	j = &b[11];
	*j-- = EOS;
	do
	{
		*j-- = i % 10 + '0';
		i /= 10;
	} while (i);

	do
	{
		*a++ = *++j;
	} while (*j);
}

/*{
** Name: CVlx	- Convert long to ascii hex.
**
** Description:
**
** Inputs:
**
** Outputs:
**	Returns:
**	Exceptions:
**
** Side Effects:
**
** History:
**	{@dd-mmm-yy (login)@}
{@history_line@}...
*/
VOID
CVlx(num, string)
u_i4               num;
char               *string;
{
    i4		    i;

    for (i = 8; --i >= 0; )
    {
	string[i] = (num & 0xf) + '0';
	if (string[i] > '9')
	    string[i] += 'A' - '9' - 1;
	num >>= 4;
    }
    string[8] = EOS;
}

/*{
** Name: CVlower	- Convert characters string to lowercase.
**
** Description:
**
** Inputs:
**
** Outputs:
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	01-oct-1985 (derek)
**          Created new for Jupiter.
**	31-oct-88 (jrb)
**	    Converted for Kanji.
**	13-Jan-2010 (wanfr01) Bug 123139
**	    Optimizations for single byte
*/
VOID
CVlower_DB(string)
register char	*string;
{
	while (*string != EOS)
	{
		CMtolower(string, string);
		CMnext(string);
	}
}


VOID
CVlower_SB(string)
register char	*string;
{
	while (*string != EOS)
	{
		CMtolower_SB(string, string);
		CMnext_SB(string);
	}
}

/*{
** Name: CVol	- Convert octal to long.
**
** Description:
**
** Inputs:
**
** Outputs:
**	Returns:
**	    OK
**	    CV_SYNTAX
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	01-oct-1985 (derek)
**          Created new for Jupiter.
*/
STATUS
CVol(octal, result)
char	octal[];
i4	*result;
{
	register	i4	i;
	register	i4	n;


	n = 0;

	for (i = 0; octal[i] >= '0' && octal[i] <= '7'; ++i)
		n = 8 * n + octal[i] - '0';

	*result = n;
	return(OK);
}

/*{
** Name: CVupper	- Convert character string to uppercase.
**
** Description:
**
** Inputs:
**
** Outputs:
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	01-oct-1985 (derek)
**          Created new for Jupiter.
**	31-oct-88 (jrb)
**	    Converted for Kanji.
**	13-Jan-2010 (wanfr01) Bug 123139
**	    Optimizations for single byte
*/
VOID
CVupper_DB(string)
register char	*string;
{
	while (*string != EOS)
	{
		CMtoupper(string, string);
		CMnext(string);
	}
}

VOID
CVupper_SB(string)
register char	*string;
{
	while (*string != EOS)
	{
		CMtoupper_SB(string, string);
		CMnext_SB(string);
	}
}
