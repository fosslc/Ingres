/*
**    Copyright (c) 1987, 2000 Ingres Corporation
*/

#include    <compat.h>
#include    <cv.h>
#include    <gl.h>
#include    <st.h>
#include    <cs.h>
#include    <mh.h>
#include    <me.h>
/**
** Name: CVFCVT.C - Float conversion routines
**
** Description:
**      This file contains the following external routines:
**    
**      CVfcvt()           -  float conversion to f format
**      CVecvt()           -  float conversion to e format
**	lecvt()		   -  re-entrant version of ecvt
**	lfcvt()		   -  re-entrant version of fcvt
**	CVround()	   -  round ascii string.
**	CVmodf()	   -  modulus function.
**
** History:
 * Revision 1.4  90/11/19  10:14:26  source
 * sc
 * 
 * Revision 1.3  90/10/08  07:58:06  source
 * sc
 * 
 * Revision 1.2  90/05/11  14:23:50  source
 * sc
 * 
 * Revision 1.1  90/03/09  09:14:23  source
 * Initialized as new code.
 * 
 * Revision 1.2  90/02/12  09:39:52  source
 * sc
 * 
 * Revision 1.1  89/05/26  15:45:33  source
 * sc
 * 
 * Revision 1.1  89/05/11  01:05:17  source
 * sc
 * 
 * Revision 1.1  89/01/13  17:11:21  source
 * This is the new stuff.  We go dancing in.
 * 
 * Revision 1.1  88/08/05  13:28:47  roger
 * UNIX 6.0 Compatibility Library as of 12:23 p.m. August 5, 1988.
 * 
**      Revision 1.2  87/11/10  12:37:22  mikem
**      Initial integration of 6.0 changes into 50.04 cl to create 
**      6.0 baseline cl
**      
**      Revision 1.3  87/07/27  11:23:35  mikem
**      Updated to conform to jupiter coding standards.
**      
**      20-jul-87 (mmm)
**          Updated to meet jupiter coding standards.      
**	21-June-89 (harry)
**	    We don't need any "ifdef VAX"s in this file, since cvdecimal.s
**	    no longer exists. Because of that, we can also remove the
**	    define for CVfcvt_default.
**	11-may-90 (blaise)
**	    Integrated changes from 61 into 63p library.
**	28-sep-1990 (bryanp)
**	    Bug #33308
**	    Negative 'ndigit' arguments passed to the 'fcvt' in SunOS 4.1 do
**	    not work properly. In particular, fcvt may suffer a segmentation
**	    fault with certain floating point values (one known example is
**	    fcvt(1.0e+19,-6,&exp,&sign)). Fixed by not passing negative values.
**      1-june-90 (jkb)
**          integrate rexl's reintrant changes
**	4-nov-92 (peterw)
**	    Integrated change from ingres63p to add mh.h for MHdfinite().
**      16-apr-1993 (stevet)
**          Changed MAXPREC to 15 for IEEE platforms (B39932).
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	16-aug-93 (ed)
**	    add <st.h>
**	24-jul-95 (albany, for wschris)
**	    Modified this UNIX routine for use on AXP/VMS. Coded re-entrant 
**	    versions of ecvt and fcvt. Eliminate CL semaphore. Use __IEEE_FLOAT
**	    not IEEE_FLOAT.
**	23-sep-95 (dougb)
**	    Don't use MTH$ functions -- error handling is incorrect for our
**	    expectations and G-float doesn't have the range of T-float.
**	    Make local routines static.  Use ANSI-style fn prototypes and
**	    declarations.
**      23-oct-97 (horda03)  X-Integration of change 276403.
**          28-aug-97 (horda03)
**               Need to consider the fractional part of the value when determining
**               the integer part when the value is being displayed without a decimal
**               point, if abs(value) < 1.0. [0.5, 1.0) should be rounded to 1.
**               (B83617)
**	30-dec-1999 (kinte01)
**	    Add me.h to quiet compiler warnings due to missing prototype
**	01-dec-2000	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from VMS CL as the use is no longer allowed
**      22-dec-2008 (stegr01)
**          Itanium VMS port.
**      14-Aug-2009 (horda03) B122474
**          Increase size of buffer used to store converted float
**          to prevent buffer overrun and subsequent stack corruption.
**/

# if __IEEE_FLOAT == 1
# define 	MAXPREC	15
# else
# define	MAXPREC 16
# endif

#define	todigit(c)	((c) - '0')
#define	tochar(n)	((n) + '0')

# define	MAX_PREC	16	/* for ieee floats */

/* typedefs */
/* forward references */

static void lecvt( f8 value, i4 ndigits, i4 *decpt,
		  i4 *sign, char *res_buffer );
static void lfcvt( f8 value, i4 ndigits, i4 *decpt,
		  i4 *sign, char *res_buffer );
static char *CVround( f8 fract, i4 *exp, char *start,
		     char *end, char ch, i4 *signp );
static f8 CVmodf( f8 val, f8 *intp );

/* externs */

#if (!defined(ALPHA) && !defined(IA64))
GLOBALREF       CS_SEMAPHORE    CL_misc_sem;
#endif


/* statics */

/*{
** Name: CVfcvt - c version to call unix fcvt
**
** Description:
**    A C language version of CVfcvt that was previously written in assembly 
**    language.  This uses the standard UNIX fcvt programs.
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
**	18-jun-87 (mgw)
**		Update *digits with the proper value for later use in CVfa().
**      20-jul-87 (mmm)
**          Updated to meet jupiter coding standards.
**	28-sep-1990 (bryanp)
**	    Bug #33308
**	    Negative 'ndigit' arguments passed to the 'fcvt' in SunOS 4.1 do
**	    not work properly. In particular, fcvt may suffer a segmentation
**	    fault with certain floating point values (one known example is
**	    fcvt(1.0e+19,-6,&exp,&sign)). Fixed by not passing negative values.
**      26-aug-89 (integrated by jkb for rexl)
**          Added calls to semaphore functions to protect static return from
**          call to C library function fcvt().
**	18-may-94 (vijay)
**	    AIX and hp8 do weird stuff with fcvt. Allow for low numbers
**	    returning zero length strings.
**	23-sep-95 (dougb)
**	    Use ANSI-style fn declarations.
*/
STATUS
CVfcvt( f8	value,
       char	*buffer,
       i4	*digits,
       i4	*exp,
       i4	prec )
{
	i4	sign;
	char	*temp;
	i4	length;
	i4	i;

#if (!defined(ALPHA) && !defined(IA64))
        gen_Psem( &CL_misc_sem );
#endif
        if (*digits > MAXPREC)
                *digits = MAXPREC;

	/* Convert leaving space for sign. */
	temp = &buffer[1];
	lfcvt(value, prec, exp, &sign, temp);

	/* Don't allow fcvt to give us more precision than
	** is possible. At least on the pyramid machines, the
	** precision specified is the number of digits
	** beyond the decimal point. If the number is, say, 3000,
	** fcvt will give us a number implying a precision of 19
	** (4 prior to the decimal, 15 after).
	**
	** The above-mentioned bug on Pyramid appears no longer to be
	** true. In particular, passing 3000 to fcvt no longer returns
	** a string of length 19. However, fcvt DOES have very different
	** results on different machines. The intention of the following
	** code is as follows: if the string returned from our first fcvt
	** call is longer than MAXPREC ("more precision than is possible"),
	** then if the user specified a VERY long precision (e.g., 15), then
	** 'prec - length + MAXPREC' should be the longest possible precision
	** without exceeding MAXPREC. If that is greater than (or equal to) 0,
	** then make another fcvt call with that precision to try to get
	** the 'max reasonable' precision. If 'prec - length + MAXPREC'
	** is < 0, then this value cannot be formatted in 'f' format, so we
	** must go directly to 'e' format. We don't try to call 'fcvt' in
	** the case of a negative ndigit value because various machines do
	** WILDLY different things in this case (e.g., SunOS4.1 gets a SIGSEGV).
	**
	** Several miscellaneous notes, as long as I'm here:
	** 1) MAXPREC is NOT system dependent, but rather is based off of
	**	IEEE_FLOAT. That seems wrong, but I don't know what's right.
	** 2) Some machines will happily format a double to arbitrary
	**	precision (e.g., fcvt(1.0e+19,22,&arg,&sign) will return
	**	a string of length 42) whereas others seem to have an internal
	**	'max fcvt length' longer than which a returned string will
	**	never be. This internal max length, however, ALSO varies from
	**	machine to machine...
	** 3) Machines vary HIDEOUSLY in their behavior with negative
	**	ndigit values. In particular, SunOS 4.1 on a Sparc when given
	**	fcvt(3000,-1,&exp,&sign) produces '0000'!
	** 4) see the note below.
	*/
	if ((length = STlength(temp)) > MAXPREC)
	{
		if ((prec - length + MAXPREC) >= 0) {
			lfcvt(value, prec - length + MAXPREC, exp, &sign, temp);
		}
		if ((length = STlength(temp)) > MAXPREC)
			lecvt(value, MAXPREC, exp, &sign, temp);
	}

	buffer[0] = (sign ? '-' : ' ');

	/*
	** On AIX and hp8, the string returned by fcvt starts with the first
 	** nonzero digit in the number. ndigit specifies the max precision. If
	** the value is less than 1.0, the number of digits in the returned
	** string is equal to (ndigit - position of the first non-zero digit).
	** If the position of the first nonzero digit in the number is greater
	** than ndigit, a null string is returned.
	**	eg: fcvt(2300.002, 5, &dec, &sign) -->returns "230000200",dec=4
	**	    fcvt(0.002, 5, &dec, &sign) --> returns "200", dec=-2
	**	    fcvt(0.0002, 5, &dec, &sign)-> returns "20", dec=-3
	**	    fcvt(0.00002, 5, &dec, &sign)-> returns "2", dec=-4
	**	    fcvt(0.000002, 5, ..      ) --> returns "", dec= -5
	**	    fcvt(0.0000002, 5, ..      ) --> returns "", dec= -6
	**
	** If this is the case, we will round off the number to all zeros.
	** ie. the number is insignificant. This is the behavior on SunOS.
	** SunOS and Solaris return "00000", dec = 0.
	*/

	if ( length == 0 )
	{
	     *exp = 0;
	     i = min(prec, MAXPREC);
	     MEfill(i, '0', buffer + 1); 
	     *(buffer + i + 1) = '\0';
	}

	*digits = STlength(temp);

#if (!defined(ALPHA) && !defined(IA64))
        gen_Vsem( &CL_misc_sem );
#endif
	return (OK);
}

/*{
** Name: CVecvt - c version to call unix ecvt
**
** Description:
**    A C language version of CVecvt that was previously written in assembly 
**    language.  This uses lecvt, a local version of the standard UNIX ecvt.
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
**      20-jul-87 (mmm)
**          Updated to meet jupiter coding standards.
**      26-aug-89 (rexl)
**          Added calls to semaphore functions to protect static return from
**          call to C library ecvt().
**	23-sep-95 (dougb)
**	    Use ANSI-style fn declarations.
*/
STATUS
CVecvt( f8	value,
       char	*buffer,
       i4	*digits,
       i4	*exp )
{
	i4	sign;
	char	*temp;

#if (!defined(ALPHA) && !defined(IA64))
        gen_Psem( &CL_misc_sem );
#endif
	if (*digits > MAXPREC)
		*digits = MAXPREC;

	temp = &buffer[1];
	lecvt(value, *digits, exp, &sign, temp );
	buffer[0] = (sign ? '-' : ' ');
	*digits = STlength(temp);

#if (!defined(ALPHA) && !defined(IA64))
        gen_Vsem( &CL_misc_sem );
#endif
	return (OK);
}

/*{
** Name: lecvt - Re-entrant version of C run time library ecvt routine.
**
** Description:
**	Re-entrant version of C run time library ecvt routine. One additional
**	parameter: buffer.
**
** Inputs:
**	value				    value to be converted.
**	ndigits				    # digits in result
**
** Output:
**	decpt				    position of decimal point relative
**					     to 1st char in string. If value is
**					     0, decpt will be 1.
**	sign				    0 if value is positive, else -1
**	buffer				    ascii string: this buffer must have
**						ndigits + 1 (terminator) chars.
**
**      Returns:
**          VOID
**
** History:
**	18-jul-95 (wschris)
**	    Created. Code borrowed from AXP/VMS cvfa.c module.
**	23-sep-95 (dougb)
**	    Make local routines static and use ANSI-style fn declarations.
*/
static void lecvt( f8 value, i4 ndigits, i4 *decpt,
		  i4 *sign, char *res_buffer )
{
	char	tmpbuf[CV_MAXDIGITS+1];/* Temp buffer for integer part.*/
	char	*startp;		/* Place to start output.*/
	char	*endp;			/* End of output string */
	register char	*p;		/* Points to string for integer part at end of buffer */
	register char	*t;		/* Points to result string as it is built in startp buffer. */
	register f8	fract = 0;	/* Fractional part of number */
	i4		expcnt;		/* # of digits in integer. */
	i4		isnegative = 0;	/* -1 if value is negative. */
	f8		integer;	/* Integer part of number. */
	f8		tmp;

	expcnt = 0;

	/* Sign: return -1 if value is negative, else 0. Also, use abs(value):
	 * CVmodf doesn't work with negative values. */
	if (value < 0)
	{
		isnegative = -1;
		value = -value;		/* Use absolute value */
	}
	*sign = isnegative;
	
	/* Split up number into fractional and integer parts. */
	fract = CVmodf(value, &integer);

	/*
        ** The plan:
        **      - Convert integer portion of number starting with the least
        **      significant digit, put at end of buffer.
        **      - Move the value from the end to the beginning of the buffer.
        **
	**  get integer portion of number; put into the end of the buffer; the
	**  .01 is added for CVmodf(356.0 / 10, &integer) returning .59999999...
	*/
	endp = &tmpbuf[sizeof(tmpbuf)];	/* 1 char past end of buffer */
	for (p = endp - 1; integer; ++expcnt)
	{
		tmp = CVmodf(integer / 10, &integer);
		*p-- = tochar((i4)((tmp + .01) * 10));
	}

	/* State:
	** expcnt - # of digits of integer part of number.
        ** tmpbuf - the ascii version of the integer part is at end of buffer.
 	*/

	/* get an extra slot for CVround to round 9...9 to 10...0. */
	startp = tmpbuf;
	t = ++startp;

	/* If number has an integer part (>=1.0), move integer part
	 * to buffer. */
	if (expcnt)
	{
		/* Move number from end of buffer to beginning of
		 * buffer, 1 character at a time. Note: we can't fill in the
		 * result buffer yet: rounding may cause a additional digit
		 * to prefix the current result: 99 -> 100. */
		for(; ndigits > 0 && ++p < endp; --ndigits)
		{
			*t++ = *p;
		}
		/*
		**  if done ndigits and more of the integer component,
		**  round using next integer digit; adjust fract so we don't 
		**  re-round later. Note: startp and expcnt may change if we 
		**  round 9...9 to 10...9. Note: *sign may be cleared if the 
		**  string is all 0s: if we only want 2 digits of -0.004, we
		**  don't want negative -0.00.
		*/
		if (!ndigits && ++p < endp)
		{
		    /*
		    ** ??? If we round from 999.9 up to 1000.0, have we
		    ** ??? exceeded the ndigit restriction?  Should t get
		    ** ??? backed up one character?
		    */
			fract = 0;
			startp = CVround((f8)0, &expcnt, startp,
					t - 1, *p, sign);
		}
		/* State:
		** We have finished the integer part. We will do the fraction
		** part below.
		*/
	}
	/* No integer part of number (<1.0). Process fraction part of
	 * number. Until first fractional digit, decrement exponent */
	else if (fract)
	{
		/* Grab 1st non-zero digit to right of decimal point.
		** adjust expcnt for digit in front of decimal */
		for (;; --expcnt)
		{
			fract = CVmodf(fract * 10, &tmp);
			if (tmp)
			{
				break;
			}
		}
		*t++ = tochar((i4)tmp);
		ndigits--;
	}
	else
	{
		/* Number was 0. */
	    /* ??? This causes us to map 0.0 into " 0.0e001" -- check Unix. */
		expcnt = 1;	/* Hack to match ecvt: if value==0, decpt=1.*/
	}
	/* if requires more precision and some fraction left... */
	if (fract)
	{
		if (ndigits > 0)
		{
			do
			{
				fract = CVmodf(fract * 10, &tmp);
				*t++ = tochar((i4)tmp);
			} while (--ndigits && fract);
		}
		/* Round last digit. */
		if (fract)
		{
		    /*
		    ** ??? If we round from 999.9 up to 1000.0, have we
		    ** ??? exceeded the ndigit restriction?  Should t get
		    ** ??? backed up one character?
		    */
			startp = CVround(fract, &expcnt, startp,
			    t - 1, (char)0, sign);
		}
	}
	/* Add trailing zeroes if more digits required. */
	for (; ndigits--; *t++ = '0');
	*t = '\0';			/* Terminate the string. */

	/* Copy result */
	STcopy( startp, res_buffer );
	*decpt = expcnt;		/* return position of decimal point */
}

/*{
** Name: CVround	- Round ASCII string.
**
** Description:
**	Fix up the given ASCII string to round the (given) value (to the
**	correct number of digits).
**
** Inputs:
**	fract	- Remaining fraction of number we're trying to convert
**		  (portion of the number not already included in string).
**	exp	- Pointer to Current exponent which will eventually be
**		  displayed in the string.
**	start	- Current beginning of ASCII string we're rounding.
**	end	- End of the ASCII string we're rounding.
**	ch	- If fract was 0.0, ASCII character containing the last digit
**		  to round.
**	signp	- Indication of the sign of the number.
**
** Outputs:
**	exp	- Resulting exponent.  (If input exponent was provided, this
**		  may be 1 higher than it before.)
**	signp	- Indication of the sign of the number may be cleared if the
**		  value rounds to 0.0.
**
** Returns:
**	start	- New beginning of ASCII string (may be 1 character earlier
**		  if exp pointer was NULL).
**
** Exceptions:
**	none
**
** Side Effects:
**	Characters of string are modified to reflect rounding.
**
** History:
**	23-sep-95 (dougb)
**	    Add function description.  Make local routines static and use
**	    ANSI-style fn declarations.
*/
static char *
CVround( f8 fract,	/* Fraction to use for rounding.  If zero, use ch */
	i4 *exp,	/*
			** Pointer to the exponent count.  If NULL, if
			** number is all 9s, round up will cause a 1 to be
			** prefixed to 0s.  If non-NULL, result will be all
			** 0s, and *exp will be incremented.
			*/
	char *start,	/* Beginning of ascii number to be rounded */
	char *end,	/* End of ascii number */
	char ch,	/* last digit at end of number: if > '4' round */
	i4 *signp ) /* Sign: '-' or 0. If result is all 0s, *signp set to 0. */
{
	f8 tmp;
 
	/* If fract != 0, use the first digit to round last digit of #. */
	if (fract)
	{
		(void)CVmodf(fract * 10, &tmp);
	}
	else
	{
		/* fract == 0, use ch to round number. */
		tmp = todigit(ch);
	}

	if (tmp > 4)
	{
		/* We have to round the string up. Start at end, rounding. If
		 * we find a 9, we have to add 1 to previous digit. If 1st 
		 * digit is 9, we have to add an additional digit and change
		 * the exponent. */
		for (;; --end)
		{
			if (*end == '.')	/* skip decimals */
			{
				--end;
			}
			if (++*end <= '9')	/* Add 1, if not '0' break. */
			{
				break;
			}
			*end = '0';
			/* All chars were 9's, and were changed to 0s, we need
			** to prefix the 0s with a 1. E.g., "999" becomes
			** "1000". Add 1 to exp.
			*/
			if (end == start)
			{
				if (exp)
				{	/* e/E; increment exponent */
					*end = '1';
					++*exp;
				}
				else
				{		/* f; add extra digit */
					*--end = '1';
					--start;
				}
				break;
			}
		}
	}
	/* No need to round, but if you have all zeroes, clear the sign:
	** ``"%.3f", (f8)-0.0004'' gives you a negative 0. */
	else if (*signp != 0)
	{
		for (;; --end)			/* Start at string end */
		{
			if (*end == '.')	/* skip decimal */
			{
				--end;
			}
			if (*end != '0')	/* If anything non-zero, OK */
			{
				break;
			}
			if (end == start)	/* All zeroes: clear sign */
			{
				*signp = 0;
			}
		}
	}
	return(start);
}

/*{
** Name: CVmodf
**
** Description:
**	Split the passed double into its integer and fraction portions.
**
** Inputs:
**	val		The double to take the integer part of.
**
** Outputs:
**	intp		Integer part of val (as an f8).
**
** Returns:
**	Fraction portion of val.
**
** Exceptions:
**	none
**
** Side Effects:
**	none
**
** History:
**	18-jul-95 (wschris)
**	    Created.  Modulus function grabbed from 6.4/05 AXP/VMS/02 cvfa.c
**	23-sep-95 (dougb)
**	    Don't use MTH$ functions -- error handling is incorrect for our
**	    expectations and G-float doesn't have the range of T-float.  Also,
**	    make local routines static and use ANSI-style fn declarations.
**	    Added history comments.
*/
static f8 CVmodf( f8 val, f8 *intp )
{
    register f8 intval;
 
    intval = MHfdint( val );
    *intp = intval;

    return ( val - intval );
}

/*{
** Name: lfcvt - Re-entrant version of C run time library fcvt routine.
**
** Description:
**	Re-entrant version of C run time library fcvt routine. One additional
**	parameter: buffer.
**
**	If abs(value) > 1.0 (value has an integer part)
**	then 
**		fcvt returns all digits from the integer part plus ndigits
**		 from the decimal part.
**	else
**		fcvt returns the 1st ndigits of the fraction without leading
**		zeroes. If the first ndigits are all zeroes, fcvt returns
**		ndigits+1 zeroes with decpt = 1 (In effect there are ndigits
**		after the decimal with a leading integer 0).
**
** Inputs:
**	value				    value to be converted.
**	ndigits				    max # digits to the right of
**					     decimal to output to buffer.
**
** Output:
**	decpt				    position of decimal point relative
**					     to 1st char in string.
**	sign				    0 if value is positive, else 1
**	buffer				    ascii string: this buffer must be
**					     large enough to hold result.
**					     Result can be 308 + 1 (EOS) char.
**
**      Returns:
**          OK                         	    Function completed normally. 
**          FAIL                            Function completed abnormally.
**
** History:
**	18-jul-95 (wschris)
**	    Created. Code borrowed from AXP/VMS cvfa.c module.
**	23-sep-95 (dougb)
**	    Make local routines static and use ANSI-style fn declarations.
**      28-aug-97 (horda03)
**          Need to consider the fractional part of the value when determining
**          the integer part when the value is being displayed without a decimal
**          point, if abs(value) < 1.0. [0.5, 1.0) should be rounded to 1.
**          (B83617)
*/
static void lfcvt( f8 value, i4 ndigits, i4 *decpt,
		  i4 *sign, char *res_buffer )
{
	char	tmpbuf[CV_MAXDIGITS + 1];/* Temp buffer for integer part.*/
	char	*startp;		/* Place to start output.*/
	char	*endp;			/* End of output string */
	register char	*p;		/* Points to string for integer part at end of buffer */
	register char	*t;		/* Points to result string as it is built in startp buffer. */
	register f8	fract = 0;	/* Fractional part of number */
	i4		expcnt;		/* # of digits in integer. */
	i4		maxdigits = MAX_PREC;	/* Max digits of precision+1 */
	i4		pdigits = ndigits;	/* # digits to right of decimal to convert */
	i4		isnegative = 0;	/* -1 if value is negative. */
	f8		integer;	/* Integer part of number. */
	f8		tmp;
        i4             largeval = -1;   /* 0 if abs(value) < 1.0 */

	if (ndigits < 0) {		/* garbage in, garbage out */
		*res_buffer = '\0';
		return;
	}

	expcnt = 0;

	/* Sign: return -1 if value is negative, else 0. Also, use abs(value):
	 * CVmodf doesn't work with negative values. */
	if (value < 0)
	{
		isnegative = 1;
		value = -value;		/* Use absolute value */
	}
	
	/* Split up number into fractional and integer parts. */
	fract = CVmodf(value, &integer);

	/*
	** The plan:
	**	- Convert integer portion of number starting with the least
	**	significant digit, put at end of buffer.
	**	- Move the value from the end to the beginning of the buffer.
	**
	**  get integer portion of number; put into the end of the buffer; the
	**  .01 is added for CVmodf(356.0 / 10, &integer) returning .59999999...
	*/
	endp = &tmpbuf[sizeof(tmpbuf)];	/* 1 char past end of buffer */
	for (p = endp - 1; integer; ++expcnt)
	{
		tmp = CVmodf(integer / 10, &integer);
		*p-- = tochar((i4)((tmp + .01) * 10));
	}

	/* State:
	** expcnt - # of digits of integer part of number.
	** tmpbuf - the ascii version of the integer part is at end of buffer.
	*/

	/* get an extra slot for CVround to round 9...9 to 10...0. */
	startp = tmpbuf;
	t = ++startp;

	/* If number has an integer part (>=1.0), move integer part
	 * to buffer. */
	if (expcnt)
	{
	    /*
	    ** ??? I think it's incorrect to make any use of maxdigits here.
	    ** ??? ndigits indicates the precision our caller was interested
	    ** ??? in.  Don't give them too much more than that!  At least,
	    ** ??? ndigits should be decremented in the loop.  And, the
	    ** ??? loop should terminate when ndigits is exceeded!
	    ** ??? Large numbers simply fill the entire buffer with numbers,
	    ** ??? leading to stack corruption when the STcopy() is done.
	    ** ??? ndigits is left incorrect for the "trailing zeroes" loop
	    ** ??? further down.
	    */
		/* Move the entire integer part from end of buffer to 
		** beginning of buffer, 1 character at a time. Round the last
		** digit of precision (IEEE: 16th digit). Zero fill past the
		** last digit of precision. */
		for(; ++p < endp; --maxdigits)
		{
			if (maxdigits > 0) 
			{
				*t++ = *p;
			}
			else if (maxdigits == 0)
			{
			    /*
			    ** ??? If we round from 999.9 up to 1000.0, have we
			    ** ??? exceeded the MAX_PREC restriction?  Should t
			    ** ??? get backed up one character?
			    */
				/* 1 past the last digit. Use it to round the
				** last digit. */
				startp = CVround((f8)0, &expcnt, startp,
				    t - 1, *p, sign);
			}
			else
			{
				/* Past precision: fill with 0s. */
				*t++ = '0';
			}
		}
		/* State:
		** We have finished the integer part. We will now grab ndigits
		** of the fraction. */
	}
	else if (ndigits > 0)		/*
					** ??? This test seems incorrect.
					** ??? Shouldn't we be testing fract?
					*/
	{
		/* No integer part.
		** Grab 1st non-zero digit to right of decimal point.
		** adjust expcnt for digit in front of decimal */
		for (; ; --expcnt)
		{
			fract = CVmodf(fract * 10, &tmp);
			if (tmp)
			{
				*t++ = tochar((i4)tmp);
				--ndigits;
				--maxdigits;
				break;
			}
			if (!fract || --ndigits == 0) {
			    /*
			    ** ??? Why not set isnegative to 0 here? Check Unix.
			    ** ??? maxdigits may also need to be set to 0
			    ** ??? or decremented in this area.
			    */
 
                               if (fract) {
 
                                /* We still have a fraction but have run out
                                of ndigits. Use fraction for possible
                                rounding that could make the value non-zero.
                                (Bug 77284) */
 
                                      (void)CVmodf(fract * 10, &tmp);
                                       if (tmp > 4) 
                                       {
                                              *t++ = '1';           
                                              --maxdigits;
                                              /* we've done the rounding */
                                              fract = 0;
                                              break;
                                       }
                               } 
 
				/* no significant digits in first ndigits: fill
				** with ndigits+1 zeroes. */
				for ( ndigits = pdigits+1; ndigits; --ndigits)
					*t++ = '0';
				fract = 0;	/* make sure no rounding */
				expcnt = 1;	/* decimal after 1st zero */
				/*return -0: isnegative = 0;*/	/* clear the sign */
				break;
			}
		}
	}
	else if (fract < 0.5) 
	{
		/* ndigits = 0 and no integer part: clear the sign. */
		isnegative = 0;
                largeval = 0;
        }
        else
        {
                /* ndigits = 0, but rounding required; sign maintained. */
                largeval = 0;
                expcnt++;
                *t++ = '1';
	}

	/* if requires more precision and some fraction left and we haven't
	** used maxdigits+1 (16+1) of IEEE precision yet. */
	if (largeval && fract)
	{
		if (ndigits > 0 && maxdigits > 0)
		{
			do
			{
				fract = CVmodf(fract * 10, &tmp);
				*t++ = tochar((i4)tmp);
				--maxdigits;
			} while (--ndigits && maxdigits && fract);
		}
		/* If we have processed the last digit desired, round it.
		** Don't round if we have used all 16 bits of IEEE precision.
		** Also don't round if we haven't any digits (t==startp): can
		** happen if initially ndigits<=0 and no integer part. */
		if (fract && maxdigits > 0 && t!=startp)
		{
		    /*
		    ** ??? If we round from 999.9 up to 1000.0, have we
		    ** ??? exceeded the ndigit restriction?  Should t get
		    ** ??? backed up one character?
		    */
			startp = CVround(fract, &expcnt, startp,
			    t - 1, (char)0, sign);
		}
	}
	/* Add trailing zeroes if more digits required. */
	for (; ndigits-->0; *t++ = '0');
	*t = '\0';			/* Terminate the string. */

	/* Copy result */
	STcopy( startp, res_buffer );
	*decpt = expcnt;		/* return position of decimal point */
	*sign = isnegative;		/* return sign */
}
