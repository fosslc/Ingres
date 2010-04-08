/*
**Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<me.h>
# include	<st.h>
# include	<cm.h>
# include	<cv.h>
# include	<nm.h>
# include	<er.h>


/**
** Name: CVFA.C - floating point to ascii conversion
**
** Description:
**      This file contains the following external routines:
**    
**      CVfa()        	-  floating point to ascii conversion
**
** History:
 * Revision 1.2  90/09/04  10:49:27  source
 * sc
 * 
 * Revision 1.1  90/03/09  09:14:23  source
 * Initialized as new code.
 *
 * Revision 1.2  90/02/12  09:39:47  source
 * sc
 * 
 * Revision 1.1  89/05/26  15:45:30  source
 * sc
 * 
 * Revision 1.1  89/05/11  01:05:15  source
 * sc
 * 
 * Revision 1.2  89/02/23  01:00:32  source
 * sc
 * 
 * Revision 1.2  89/02/03  16:39:09  roger
 * Include <me.h> since ME is used.
 * 
 * Revision 1.1  88/08/05  13:28:46  roger
 * UNIX 6.0 Compatibility Library as of 12:23 p.m. August 5, 1988.
 * 
**      Revision 1.3  87/11/17  16:54:37  mikem
**      changed to not use CH anymore
**      
**      Revision 1.2  87/11/10  12:37:17  mikem
**      Initial integration of 6.0 changes into 50.04 cl to create 
**      6.0 baseline cl
**      
**      Revision 1.3  87/07/27  11:23:30  mikem
**      Updated to conform to jupiter coding standards.
**      
**      20-jul-87 (mmm)
**          Updated to meet jupiter coding standards.      
**	12-jun-90 (jkb)
**	    for negative exponents check "exp + prec > 0" rather than 
**	    "exp <= awidth - prec" to determine whether to use exponential
**	    notation
**      08-mar-91 (rudyw)
**          Comment out text following #endif
**      01-apr-92 (stevet)
**          B42218: Normally a postive floating point number is displayed 
**          with a leading blank.  However, if the width of the output field
**          can just fit the output number (i.e. display 1234.5678 with
**          f8e10.3 format), then the number will be displayed without
**          the leading blank.  CVfa() does not round the floating point
**          number correctly in the latter situation.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	27-dec-94 (forky01)
**	    Remove changes associated with piccolo change #415203, as they
**	    introduced a slew of problems for conversions.  Correctly fixed
**	    bug #63296, which as a result fixes bug #66056.  Also incorporated
**	    a few changes to make it operate the same way as the VMS version
**	    for consistency which fixes bug #66119.  Also fixed floating point
**	    point alignment for 'G' format which output only 4 spaces in non-exp
**	    conversion for right side padding, even though on IEEE_FLOAT 
**	    machines the format E+000 (5 spaces are needed).
**    2-Apr-97 consi01 bug 77993
**       If underflow occurs when using the N_FORMAT the output is 
**       converted to exponential form.
**
**       eg  displaying 0.04 using n8.1 gives 4.0e-002
**
**       The report manual states that n formats should perform as f
**       formats unless the value 'will not fit' (overflow occured).
**
**       ie  displaying 0.04 using f8.1 gives      0.0
**
**       This change gives the output in decimal format when N_FORMAT
**       and underflow occurs.
**	03-jun-97 (dacri01)
**          Bug #82721: When we try to fit the number into a 
**		non-exponential form, cases where the total number of digits
**              was the same as the number of digits after the decimal place 
**		was being ignored.  
**			Ex.  displaying .999 using .nnn would result in .000.
**              Included such cases for conversion.
**	23-may-97 (mcgem01)
**	    Clean up compiler warnings.
**      25-may-1997 (kosma01)
**          moved MAXDIGITS to cvcl.h and renamed it CV_MAXDIGITS. This was
**          so handy/iicl.c could use it for reentrant fcvt_r function on
**          AIX 4.1 (rs4_us5).
**      27-apr-1999 (hanch04)
**          replace STindex with STchr
**	1-dec-1999 (rigka01)
**	    Enabled code to function the same as before the fix to bug 77993
**	    by setting environment variable II_FLOAT_UNDERFLOW 
**	    to EXPONENTIAL
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	29-jan-2001 (hayke02)
**	    Modify above change so that the comments above the test for
**	    small_exponential are now delimited properly - this allows
**	    the test for -1, 0 or 1 to be made and prevents II_FLOAT_OVERFLOW
**	    being checked for during the processing of every float value.
**	    This change fixes bug 103756.
**	20-Jul-2004 (lakvi01)
**		SIR# 112703, cleaned-up warnings.
**      26-Jul-2004 (lakvi01)
**          Backed-out the above change to keep the open-source stable.
**          Will be revisited and submitted at a later date. 
**/

/* # defines */

# define	E_FORMAT	('e' & 0xf)
# define	F_FORMAT	('f' & 0xf)
# define	G_FORMAT	('g' & 0xf)
# define	N_FORMAT	('n' & 0xf)

/* typedefs */
/* forward references */
/* externs */
/* statics */
static int small_exponential=-1; 


/*{
** Name: CVfa - floating point to ascii conversion
**
** Description:
**    'Value' is converted to an ascii character string and stored into 'ascii'.
**    Ascii should have room for at least 'width' + 1 characters.
**    'Width' is the width of the output field (max).
**    'Prec' is the number of characters to put after the decimal point.
**    The format of the output string is controlled by 'format'.
**    'res_width' is the width of the output field (sometimes shorter than
**    'width').
**
**    'Format' can be:
**    	e : "E" format output
**    	f :  "F" format output
**    	g :  "F" format output if it will fit, otherwise use "E" format.
**    	n :  same as G, but decimal points will not always be aligned.
**
** Inputs:
**	value				    value to be converted
**	width				    the width of the formatted item
**	prec				    number of digits of precision
**	format				    format type (e,f,g,n)
**	decchar				    Character to use as decimal char.
**
** Output:
**	ascii				    where to put the converted value
**	res_width			    where to put width of output field
**
**      Returns:
**	    OK				    return on normal exit
**	    CV_TRUNCATE			    return if output field is not wide 
**					    enough for output
**
**	Normal return is the width of the output field 
**	(sometimes shorter than 'width').
**
**
** History:
**	03/09/83 -- (mmm)
**		brought code from vms and modified.   
**	08/11/85 -- (ac)
** 		Bug# 6179. Fit the number into non-exponential 
**		form. Use exponent form if G/N format and 
**		the value will have significant digits 
**		after prec zeros in the fraction.
**	08/04/85 -- (jrc)
**		Added decchar for international support.
**
**      20-jul-87 (mmm)
**          Updated to meet jupiter coding standards.
**
**		Revision 60.3  87/04/14  12:32:47  bobm
**		ST[r]index changes for Kanji
** 
**		Revision 30.4  85/10/17  15:34:20  shelby
**		Added fix for float rounding errors in F format.
**		
**		Revision 30.3  85/09/28  12:35:15  cgd
**		International integration
**		
**      01-apr-92 (stevet)
**          Changed the calculation of 'digits' to fix B42218.
**	1-dec-1999 (rigka01)
**	    Enabled code to function the same as before the fix to bug 77993
**	    by setting environment variable II_FLOAT_UNDERFLOW 
**	    to EXPONENTIAL
*/
STATUS
CVfa(
  f8	value,		/*	value to be converted			*/
  i4	width,		/*	the width of the formatted item		*/
  i4	prec,		/*	number of digits of precision		*/
  char	format,		/*	format type (e,f,g,n)			*/
  char	decchar,
  char	*ascii,		/*	where to put the converted value	*/
  i2	*res_width	/*	where to put width of the output field  */ )
{
	i4	overflow_width;		/* # of chars to fill with '*' on overflow	*/
	i4	exp;			/* exponent */
	i4	digits;			/* number of digits to convert	*/
	char	echar;			/* either 'e' or 'E' */
	union	{
		char	buf[CV_MAXDIGITS+2];/* ascii digits "+" sign and 0	*/
		i4	dummy;		/* dummy for byte alignment	*/
		}	buffer;
	char	*cp;
	char	format_buf[3];

	buffer.dummy = 0;		/* always initialize--clear */
	format_buf[0] = format;

	overflow_width = width;

	/* Before bug fix 77993, exponential format was used to 
	** display numbers that were too small to fit in the display
	** area; after the fix, zero's were displayed in these cases.
	** Some client applications are dependent on the previous 
	** implementation.  Setting II_FLOAT_UNDERFLOW
	** to EXPONENTIAL now causes exponential to be displayed. 
	** format.
	*/
	if (small_exponential == -1)  /* env var has not yet been read */
	{
	    char *env=0; 
	    NMgtAt(ERx("II_FLOAT_UNDERFLOW"),&env);
	    if (env && *env &&
                (!STcompare(env,"EXPONENTIAL")))
	        small_exponential=1; /* use exponential form */
	    else
	        small_exponential=0; /* use zeros not exponential form */
	}


	/* determine the character to use as the exponent */
	if (CMupper(format_buf))
		echar = 'E';
	else
		echar = 'e';

	format &= 0xf;			/* normalize the format */

	if (prec > 0)
		width--;		/* charge width with decimal point */
	if (value < 0.0)
		width--;		/*	account for the sign				*/

	for (;;)			/* to break out of */
	{

		if (format == G_FORMAT)		/*	shorten 'g' format to align decimal points	*/
# ifdef IEEE_FLOAT
			digits = width - 5;     /* for E+000 format */
# else
			digits = width - 4;     /* for E+00 format */
# endif
		else
			digits = width;

		if (format != E_FORMAT)		/*	if we would rather not use exponential form	*/
		{
			/*
			** Try and fit the number into non-exponential form. 
			** We will use exponent form if G format and the value
			** will have significant digits after prec zeros
			** in the fraction. These semantics are per PB.
			*/
			f8	tmpexp = 1.0;

			CVexp10(-prec,&tmpexp);

/* consi01 bug 77993 != G_FORMAT was == F_FORMAT */
		if (((small_exponential==1) &&
		     ((digits && (digits >= prec)) &&
		      (format == F_FORMAT || value >= tmpexp|| 
		       value <= -(tmpexp) || value == 0.0 ))) ||
		   ((small_exponential==0) &&
		     ((digits && (digits >= prec)) &&
		      (format != G_FORMAT || value >= tmpexp|| 
		       value <= -(tmpexp) || value == 0.0 )))) 
			{
				i4	awidth;
		                i4      tmp;


				awidth = digits;
				CVfcvt(value, buffer.buf, &digits, &exp, prec);

				/* at this point digits is the number of digits in the      */
				/* buffer.buf string and exp is the number of decimal       */
				/* places to shift the decimal point (assuming the decimal  */
				/* point is currently at the left extreme of the number in  */
				/* buffer.buf)                                              */
				tmp = (exp < 0)  ?  0  : exp;
								
				/* check to see if the number will fit: this is calculated  */
				/* by taking the available width, subtracting the digits to */
				/* the right of the decimal point (prec) and subtracting    */
				/* the digits to the left of the decimal point (tmp).       */
				/* Remember, room for the decimal point and a negative sign */
				/* have already been taken into account above.              */
				if (awidth - prec - tmp >= 0)
				{
					if (format != G_FORMAT)	/* will fit: change 'n' to 'f'*/
						format = F_FORMAT;
					break;			/* Write out the number	*/
				}
			}
		}
		/* We go through here if we need the exponential form */
		if (format != F_FORMAT)
		{
# ifdef			IEEE_FLOAT			/* for ieee allow 5 characters because max exponent is 308 */
			width -= 5;			/* allow for e+000			*/
			if (prec > 16)			/* In case this precision causes overflow on IEEE machines */
				prec--; 		/* reduce the precision to that of the machine		*/
# else
			width -= 4;			/*	account for e+00				*/
# endif
			/* We can use exponential form if we have positive width
			** and we have width enough for the precision 
			*/
			if( width - prec > 1 && value >= 0)
			    digits = width - 1;
			else
			    digits = width;

			if (digits > 0 && width > prec)
			{
				CVecvt(value, buffer.buf, &digits, &exp);
				format = E_FORMAT;	/*	make it 'e' format for the ascii builder	*/
				break;			/*	Write the ascii string				*/
			}
		}
		MEfill(overflow_width, '*', ascii);	/*	dumb user: star his field			*/
		ascii[overflow_width] = '\0';
		*res_width = (i2) overflow_width;	
		return(CV_TRUNCATE);
	}

	/*
	**	At this point:
	**		format is 'e', 'f', 'g'(which means f format with 4(5 for IEEE) blank fill).
	**		digits is the number of significant digits to be printed
	**		width  is the the width of the field (+4(5 for IEEE) if this is 'g')
	**		prec   is the number of digits to follow the '.'
	*/

	{
		register char	*p = buffer.buf;
		register char	*q = ascii;
		register i4	move;

		if (digits > 0 && *p++ == '-')
		{
			*q++ = '-';				/*	deposit the sign if '-'				*/
		}
		/* Don't print a leading blank if we need the space for a leading digit. */
		else if (format == E_FORMAT && width - prec > 1)
		{
			*q++ = ' ';
			width--;				/* account for the loss of space. The '-' was accounted for above*/
		}
		for (move = 1;;)				/*	always put a leading digit			*/
		{
			if (value == 0.0)			/*	special case for zero				*/
				break;
			if (format != E_FORMAT)
			{
				if (exp <= 0)
					break;			/*	if 'f'|'g' and no digits before '.' force '0'	*/
				move = exp;			/*	move the digits before the '.'			*/
			}
			else
				move = width - prec, exp -= move;	/*	move the number of digits that will fit	*/
			for (; move > 0 && digits > 0; --move, --digits)
				*q++ = *p++;
			break;
		}
		while (--move >= 0)				/*	fill with '0' beyond precision before '.'	*/
			*q++ = '0';
		if ((move = prec) > 0)				/*	add precision if requested			*/
		{
			*q++ = '.';
			if (format != E_FORMAT)
				for (; exp < 0 && move > 0; *q++ = '0', ++exp, --move)
					;			/*	fill 'f' with '0' before precision after '.'	*/
			for (; move > 0 && digits > 0; --move, -- digits)		
				*q++ = *p++;

			while (--move >= 0)			/*	pad precision with '0' 				*/
				*q++ = '0';
		}
		if (format == G_FORMAT)
		{
			*q++ = ' ', *q++ = ' ', *q++ = ' ', *q++ = ' ';
# ifdef                 IEEE_FLOAT
			*q++ = ' ';
# endif
		}
		else if (format == E_FORMAT)
		{
			*q++ = echar, *q++ = '+';
			if (exp < 0)
				q[-1] = '-', exp = -exp;
# ifdef			IEEE_FLOAT		/* ieee can have 3 digit exponent */
			{
				i4	t_exp = exp;

				move = t_exp/100;
				*q++ = move + '0';
				t_exp -= 100 * move;
				move = t_exp/10;
				*q++ = move + '0';
				t_exp -= 10 * move;
				*q++ = t_exp + '0';
			}
# else
			move = exp / 10;
			*q++ = move + '0';
			*q++ = (exp - move * 10) + '0';
# endif			/* IEE_FLOAT */

		}
		*q = '\0';
		*res_width = (q - ascii);
	}
	/*
	** Replace the decimal point with the right character.
	*/
	if (decchar != '\0' && (cp = STchr(ascii, '.')) != NULL)
		*cp = decchar;

	return(OK);
}
